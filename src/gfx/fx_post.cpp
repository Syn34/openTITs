// Post effects drawn through the effect vtables: texture fades, the bloom
// apply pass, the scaled copies, depth down-sampling, god-ray set-up and
// the depth-of-field blur chain.
#include "rsx.h"

namespace sly4::gfx {

namespace {

constexpr u32 vp_render_txr = 0x01164090, fp_render_txr = 0x01164094;  // PostProc_RenderTxr
constexpr u32 vp_down4x = 0x011640A0, fp_down4x = 0x011640A4;          // PostProc_DownSample4x
constexpr u32 vp_blur11h = 0x011640B0, fp_blur11h = 0x011640B4;        // PostProc_GuassianBlur11H
constexpr u32 vp_blur11v = 0x011640B8, fp_blur11v = 0x011640BC;        // PostProc_GuassianBlur11V
constexpr u32 vp_down_depth = 0x011640DC, fp_down_depth = 0x011640E0;  // PostProc_DownSampleDepth
constexpr u32 vp_copy_scale = 0x01164110, fp_copy_scale = 0x01164114;  // PostProc_CopyScale
constexpr u32 vp_bloom_apply = 0x01164120, fp_bloom_apply = 0x01164124;  // PostProc_BloomApply
constexpr u32 vp_god_rays = 0x010F0B40, fp_god_rays = 0x010F0B44;      // PostProc_GodRaysScatter

struct FadeEffect {
    pad<24> _0;
    be_u32 alpha;  // +24: f32
};
struct BloomEffect {
    pad<48> _0;
    be_f32 strength;  // +48
};
struct CopyEffect {
    pad<132> _0;
    gp<Target> depth;   // +132
    gp<Target> colour;  // +136
    pad<12> _140;
    gp<Texture> tex;    // +152
};
static_assert(offsetof(CopyEffect, tex) == 152);
struct Overlay {
    pad<48> _0;
    be_u32 shown[3];        // +48
    pad<4> _60;
    be_u32 pending[3];      // +64
    pad<12> _76;
    u8 flags;               // +88: 0x40 clear first
    pad<23> _89;
    be_u32 colour[4];       // +112: f32
    pad<16> _128;
    gp<Target> targets[2];  // +144
    gp<Texture> textures[2];  // +152
    be_u32 index;           // +160
};
static_assert(offsetof(Overlay, colour) == 112 && offsetof(Overlay, index) == 160);
struct GodRays {
    pad<64> _0;
    be_u32 c64[4];   // +64
    pad<4> _80;
    be_f32 scale84;  // +84
    pad<8> _88;
    be_u32 w96[3];   // +96
    be_f32 w108;     // +108
    u8 colour[4];    // +112
};
static_assert(offsetof(GodRays, w108) == 108 && offsetof(GodRays, colour) == 112);
struct DofBlur {
    pad<64> _0;
    u8 passes;  // +64
};

void bind(u32 vertex, u32 fragment) {
    bind_vertex(vprog(vertex));
    bind_fragment(fprog(fragment));
}

void zero_vertex_consts() { call(fn::cellGcmSetVertexProgramConstants, gcm(), 0u, 4u, g::zero_vconsts); }

u64 one_and(u32 w) { return (u64{0x3F800000} << 32) | w; }  // {1.0f, w}

}  // namespace

// void PostProc_DrawFade(Effect* e)   (vtable slot 20)
// Alpha blends work target 0 over the current target with PostProc_RenderTxr,
// colour {1, 1, 1, e +24}.
void PostProc_DrawFade(FadeEffect* e) {
    blend_on(kSrcAlpha2, kOneMinusSrcAlpha2);
    const u64 rgb = global<be_u64>(0x0053E188);  // f32 { 1.0, 1.0 }
    const u32 alpha = e->alpha;
    bind(vp_render_txr, fp_render_txr);
    set_fconst(0, rgb, one_and(alpha));
    Draw_ResetState(false, 0);
    Draw_SetTexture0(work_texture(0), 0, 1);
    draw_quad();
}

// void Bloom_Apply(Bloom* e)   (vtable slot 20)
// Adds work target 0 (the blurred bloom) to the current target with
// PostProc_BloomApply, strength e +48 / 6 * 0.25.
void Bloom_Apply(BloomEffect* e) {
    constexpr u32 kConsts = 0x0053CBC0;  // f32 { 6.0, 0.25, 1.0, 1.0 }
    const be_f32* k = host<be_f32>(kConsts);
    blend_on(kSrcAlpha2, kOne2);
    const f32 strength = e->strength / k[0] * k[1];
    const u64 rgb = global<be_u64>(kConsts + 8);
    bind(vp_bloom_apply, fp_bloom_apply);
    set_fconst(0, rgb, one_and(bits(strength)));
    Draw_ResetState(false, 0);
    Draw_SetTexture0(work_texture(0), 0, 1);
    draw_quad();
}

// void PostProc_CopyScale(Effect* e, f32 alpha)   (vtable slot 52)
// Adds texture e +152 to the screen targets with PostProc_CopyScale,
// colour {1, 1, 1, alpha}.
void PostProc_CopyScale(CopyEffect* e, f32 alpha) {
    Draw_BindScreenTargets(e);
    bind(vp_copy_scale, fp_copy_scale);
    zero_vertex_consts();
    set_fconst(0, global<be_u64>(0x00543528) /* f32 { 1.0, 1.0 } */, one_and(bits(alpha)));
    Draw_ResetState(false, 0);
    blend_on(kSrcAlpha2, kOne2);
    Draw_SetTexture0(e->tex, 0, 1);
    draw_quad();
}

// void PostProc_DownSampleDepth(Effect* e)   (vtable slot 24)
// Draws the scene's depth target (the one for the current eye / buffer
// set) into targets e +132 (depth) / e +136 (colour) with
// PostProc_DownSampleDepth: blending, alpha / stencil test and culling
// off, depth always passes, colour writes off during the draw.
void PostProc_DownSampleDepth(CopyEffect* e) {
    RenderState& st = rs();
    const Display* disp = display();
    MemStats& ms = mem_stats();
    u32 off;
    if (alternate(disp)) off = 132;
    else if (config_u8(34) != 0) off = ms.buffer_select == 1 ? 104 : 108;
    else off = 88;
    Texture* src = *at<gp<Texture>>(&ms, off);

    u8& guard = global<u8>(0x01164840);
    be_u32* zero = host<be_u32>(0x01164850);  // static f32[4] = {0}
    if (guard == 0) {
        for (u32 i = 0; i < 4; ++i) zero[i] = 0u;
        guard = 1;
    }
    Draw_BindTargets(e->depth, e->colour, 0);
    bind(vp_down_depth, fp_down_depth);
    call(fn::cellGcmSetVertexProgramConstants, gcm(), 1u, 4u, zero);
    emit({kBlendEnable, 0});
    emit({kAlphaTestEnable, 0});
    emit({kDepthMask, (st.dirty_b[1] & 1) != 0 ? 1u : 0u});
    emit({kDepthTestEnable, st.dirty_b[1]});
    emit({kDepthFunc, 519});  // ALWAYS
    emit({kStencilTestEnable, 0});
    emit({kCullFaceEnable, 0});
    emit({kColorMask, 0});
    Draw_SetTexture0(src, 0, 0);
    draw_quad();
    emit({kColorMask, 0x01010101});
}

// void PostProc_DrawOverlay(Overlay* e)   (vtable slot 24)
// Alpha blends texture e +152 [e +160] over the screen targets with
// PostProc_CopyScale, colour e +112. When e +88 bit 0x40 is set it
// first clears target e +144 [e +160] (colour 0, depth / stencil
// 0xFFFFFF00), copies e +64..+72 to e +48..+56 and drops the flag.
void PostProc_DrawOverlay(Overlay* e) {
    RenderState& st = rs();
    if ((e->flags & 0x40) != 0) {
        Draw_BindTargets(nullptr, *at<gp<Target>>(e, 144 + (e->index << 2)), 0);
        call(fn::cellGcmSetClearColor, gcm(), 0u);
        call(fn::cellGcmSetClearDepthStencil, gcm(), 0xFFFFFF00u);
        call(fn::cellGcmSetStencilMask, gcm(), 255u);
        call(fn::cellGcmSetClearSurface, gcm(), 240u);  // R | G | B | A
        call(fn::cellGcmSetStencilMask, gcm(), st.stencil_mask.get());
        for (u32 i = 0; i < 3; ++i) e->shown[i] = e->pending[i].get();
        e->flags = static_cast<u8>(e->flags & ~0x40);
    }
    Draw_BindScreenTargets(nullptr);
    bind(vp_copy_scale, fp_copy_scale);
    zero_vertex_consts();
    set_fconst_from(0, e->colour);
    Draw_ResetState(false, 0);
    blend_on(kSrcAlpha2, kOneMinusSrcAlpha2);
    Draw_SetTexture0(*at<gp<Texture>>(e, 152 + (e->index << 2)), 0, 1);
    draw_quad();
}

// void GodRays_Draw(Effect* e)   (vtable slot 36)
// Sets up PostProc_GodRaysScatter for the caller's draw: no depth target,
// constants e +64 (record 0), {e +96, e +100, e +104, e +108 * e +84}
// (record 1) and the colour bytes e +112..+115 / 255 (record 2), then
// Draw_ResetState(true).
void GodRays_Draw(GodRays* e) {
    const u32 w0 = e->w96[0], w1 = e->w96[1], w2 = e->w96[2];
    const u32 w3 = bits(e->w108 * e->scale84);
    if (rs().depth) set_depth_target(nullptr);
    bind(vp_god_rays, fp_god_rays);
    set_fconst_from(0, e->c64);
    set_fconst(1, w0, w1, w2, w3);
    const f32 k = global<be_f32>(0x00542EE0);  // 1 / 255
    u32 c[4];
    for (u32 i = 0; i < 4; ++i) c[i] = bits(int_to_f32(e->colour[i]) * k);
    set_fconst(2, c[0], c[1], c[2], c[3]);
    Draw_ResetState(true, 0);
}

// void Dof_BlurDown(Dof* e)   (vtable slot 24)
// With no depth target bound: the scene colour target (current eye /
// buffer set) is down-sampled 4x into work target 3, then blurred e +64
// times (11-tap horizontal into work 1, vertical into work 3). Restores
// the depth and colour targets and passes the blurred texture (work 2)
// to vtable slot 28.
void Dof_BlurDown(DofBlur* e) {
    RenderState& st = rs();
    Target* depth = st.depth;
    Target* colour = st.colour[0];
    if (depth) set_depth_target(nullptr);
    Texture* src = scene_colour();
    Texture* half = work_texture(2);
    set_target(work_target(3));
    bind(vp_down4x, fp_down4x);
    Draw_ResetState(true, 0);
    Draw_SetTexture0(src, 0, 1);
    draw_quad();
    for (u32 i = 0; i < e->passes; i = (i + 1) & 0xFF) {
        Texture* h = work_texture(0);
        set_target(work_target(1));
        bind(vp_blur11h, fp_blur11h);
        Draw_ResetState(true, 0);
        Draw_SetTexture0(half, 0, 0);
        draw_quad();
        half = work_texture(2);
        set_target(work_target(3));
        bind(vp_blur11v, fp_blur11v);
        Draw_ResetState(true, 0);
        Draw_SetTexture0(h, 0, 0);
        draw_quad();
    }
    if (st.depth.get() != depth) set_depth_target(depth);
    Draw_SetTarget(colour, 0);
    vcall(e, 28, half);
}

}  // namespace sly4::gfx

NATIVE(0053E190, sly4::gfx::PostProc_DrawFade)
NATIVE(0053CBD0, sly4::gfx::Bloom_Apply)
NATIVE(00543530, sly4::gfx::PostProc_CopyScale)
NATIVE(00542960, sly4::gfx::PostProc_DownSampleDepth)
NATIVE(00543C44, sly4::gfx::PostProc_DrawOverlay)
NATIVE(00542EE8, sly4::gfx::GodRays_Draw)
NATIVE(005402E4, sly4::gfx::Dof_BlurDown)
