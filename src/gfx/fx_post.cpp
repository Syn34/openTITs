// Post effects drawn through the effect vtables: texture fades, the bloom
// apply pass, the scaled copies, depth down-sampling, god-ray set-up and
// the depth-of-field blur chain.
#include "fx.h"

using namespace sly4;

namespace {

namespace dfn = sly4::draw::fn;
namespace fg = sly4::fx::g;
namespace ffn = sly4::fx::fn;
using namespace sly4::fx;  // helpers and method names (fx::g is spelt out)

constexpr u32 kStats = sly4::g::mem_stats;  // +88.. per-eye scene targets (name?)
constexpr u32 kConfig = sly4::g::config;
constexpr u32 kDisplay = sly4::draw::g::display;

void reset_state(PPUContext& ctx, u32 no_blend) { call(ctx, dfn::Draw_ResetState, {no_blend, 0}); }

void texture0(PPUContext& ctx, u32 t, u32 filter) { call(ctx, dfn::Draw_SetTexture0, {t, 0, filter}); }

void bind(PPUContext& ctx, u32 vertex, u32 fragment) {
    bind_vertex(ctx, vertex);
    bind_fragment(fragment);
}

void zero_vertex_consts(PPUContext& ctx) {
    call(ctx, ffn::cellGcmSetVertexProgramConstants, {load<u32>(fg::state), 0, 4, fg::zero_vconsts});
}

u64 one_and(u32 w) { return (u64{0x3F800000} << 32) | w; }  // {1.0f, w}

}  // namespace

// void PostProc_DrawFade(Effect* e)   (vtable slot 20)
// Alpha blends work target 0 over the current target with PostProc_RenderTxr,
// colour {1, 1, 1, e +24}.
DECOMP(0053E190, PostProc_DrawFade) {
    const u32 e = arg<u32>(ctx, 0);
    blend_on(kSrcAlpha2, kOneMinusSrcAlpha2);
    const u64 rgb = load<u64>(0x0053E188);  // f32 { 1.0, 1.0 }
    const u32 alpha = load<u32>(e + 24);
    bind(ctx, fg::vp_render_txr, fg::fp_render_txr);
    set_fconst(0, rgb, one_and(alpha));
    reset_state(ctx, 0);
    texture0(ctx, load<u32>(fg::work0), 1);
    ctx.gpr[3] = draw_quad(ctx);
}

// void Bloom_Apply(Bloom* e)   (vtable slot 20)
// Adds work target 0 (the blurred bloom) to the current target with
// PostProc_BloomApply, strength e +48 / 6 * 0.25.
DECOMP(0053CBD0, Bloom_Apply) {
    const u32 e = arg<u32>(ctx, 0);
    constexpr u32 k = 0x0053CBC0;  // f32 { 6.0, 0.25, 1.0, 1.0 }
    blend_on(kSrcAlpha2, kOne2);
    const f32 strength = load_f32(e + 48) / load_f32(k) * load_f32(k + 4);
    const u64 rgb = load<u64>(k + 8);
    bind(ctx, fg::vp_bloom_apply, fg::fp_bloom_apply);
    set_fconst(0, rgb, one_and(bits(strength)));
    reset_state(ctx, 0);
    texture0(ctx, load<u32>(fg::work0), 1);
    ctx.gpr[3] = draw_quad(ctx);
}

// void PostProc_CopyScale(Effect* e, f32 alpha)   (vtable slot 52)
// Adds texture e +152 to the screen targets with PostProc_CopyScale,
// colour {1, 1, 1, alpha}.
DECOMP(00543530, PostProc_CopyScale) {
    const u32 e = arg<u32>(ctx, 0);
    const f32 alpha = static_cast<f32>(farg(ctx, 0));
    call(ctx, dfn::Draw_BindScreenTargets, {e});
    bind(ctx, fg::vp_copy_scale, fg::fp_copy_scale);
    zero_vertex_consts(ctx);
    set_fconst(0, load<u64>(0x00543528) /* f32 { 1.0, 1.0 } */, one_and(bits(alpha)));
    reset_state(ctx, 0);
    blend_on(kSrcAlpha2, kOne2);
    texture0(ctx, load<u32>(e + 152), 1);
    ctx.gpr[3] = draw_quad(ctx);
}

// void PostProc_DownSampleDepth(Effect* e)   (vtable slot 24)
// Draws the scene's depth target (the one for the current eye / buffer
// set) into targets e +132 (depth) / e +136 (colour) with
// PostProc_DownSampleDepth: blending, alpha / stencil test and culling
// off, depth always passes, colour writes off during the draw.
DECOMP(00542960, PostProc_DownSampleDepth) {
    const u32 e = arg<u32>(ctx, 0);
    const u32 st = fg::state;
    const u32 disp = load<u32>(kDisplay);
    u32 src;
    if ((ref<u8>(disp + 148) >> 7) != 0) src = load<u32>(kStats + 132);
    else if (field<u8>(kConfig, 34) != 0) src = load<u32>(kStats + (load<u32>(kStats + 184) == 1 ? 104 : 108));
    else src = load<u32>(kStats + 88);

    constexpr u32 kGuard = 0x01164840, kZero = 0x01164850;  // static f32[4] = {0}
    if (ref<u8>(kGuard) == 0) {
        for (u32 i = 0; i < 16; i += 4) store<u32>(kZero + i, 0);
        ref<u8>(kGuard) = 1;
    }
    call(ctx, ffn::Draw_BindTargets, {load<u32>(e + 132), load<u32>(e + 136), 0});
    bind(ctx, fg::vp_down_depth, fg::fp_down_depth);
    call(ctx, ffn::cellGcmSetVertexProgramConstants, {load<u32>(st), 1, 4, kZero});
    emit({kBlendEnable, 0});
    emit({kAlphaTestEnable, 0});
    emit({kDepthMask, (ref<u8>(st + 93) & 1) != 0 ? 1u : 0u});
    emit({kDepthTestEnable, ref<u8>(st + 93)});
    emit({kDepthFunc, 519});  // ALWAYS
    emit({kStencilTestEnable, 0});
    emit({kCullFaceEnable, 0});
    emit({kColorMask, 0});
    texture0(ctx, src, 0);
    draw_quad(ctx);
    emit({kColorMask, 0x01010101});
    ctx.gpr[3] = load<u32>(st);
}

// void PostProc_DrawOverlay(Overlay* e)   (vtable slot 24)
// Alpha blends texture e +152 [e +160] over the screen targets with
// PostProc_CopyScale, colour e +112. When e +88 bit 0x40 is set it
// first clears target e +144 [e +160] (colour 0, depth / stencil
// 0xFFFFFF00), copies e +64..+72 to e +48..+56 and drops the flag.
DECOMP(00543C44, PostProc_DrawOverlay) {
    const u32 e = arg<u32>(ctx, 0);
    const u32 st = fg::state;
    if ((ref<u8>(e + 88) & 0x40) != 0) {
        call(ctx, ffn::Draw_BindTargets, {0, load<u32>(e + 144 + (load<u32>(e + 160) << 2)), 0});
        call(ctx, ffn::cellGcmSetClearColor, {load<u32>(st), 0});
        call(ctx, ffn::cellGcmSetClearDepthStencil, {load<u32>(st), 0xFFFFFF00u});
        call(ctx, ffn::cellGcmSetStencilMask, {load<u32>(st), 255});
        call(ctx, ffn::cellGcmSetClearSurface, {load<u32>(st), 240});  // R | G | B | A
        call(ctx, ffn::cellGcmSetStencilMask, {load<u32>(st), load<u32>(st + 168)});
        for (u32 i = 0; i < 12; i += 4) store<u32>(e + 48 + i, load<u32>(e + 64 + i));
        ref<u8>(e + 88) = static_cast<u8>(ref<u8>(e + 88) & ~0x40);
    }
    call(ctx, dfn::Draw_BindScreenTargets, {0});  // r3 is 0 on both paths
    bind(ctx, fg::vp_copy_scale, fg::fp_copy_scale);
    zero_vertex_consts(ctx);
    set_fconst_from(0, e + 112);
    reset_state(ctx, 0);
    blend_on(kSrcAlpha2, kOneMinusSrcAlpha2);
    texture0(ctx, load<u32>(e + 152 + (load<u32>(e + 160) << 2)), 1);
    ctx.gpr[3] = draw_quad(ctx);
}

// void GodRays_Draw(Effect* e)   (vtable slot 36)
// Sets up PostProc_GodRaysScatter for the caller's draw: no depth target,
// constants e +64 (record 0), {e +96, e +100, e +104, e +108 * e +84}
// (record 1) and the colour bytes e +112..+115 / 255 (record 2), then
// Draw_ResetState(true).
DECOMP(00542EE8, GodRays_Draw) {
    const u32 e = arg<u32>(ctx, 0);
    const u32 st = fg::state;
    const u32 w0 = load<u32>(e + 96), w1 = load<u32>(e + 100), w2 = load<u32>(e + 104);
    const u32 w3 = bits(load_f32(e + 108) * load_f32(e + 84));
    if (load<u32>(st + 84) != 0) set_depth_target(ctx, 0);
    bind(ctx, fg::vp_god_rays, fg::fp_god_rays);
    set_fconst_from(0, e + 64);
    set_fconst(1, w0, w1, w2, w3);
    const f32 k = load_f32(0x00542EE0);  // 1 / 255
    u32 c[4];
    for (u32 i = 0; i < 4; ++i)
        c[i] = bits(static_cast<f32>(static_cast<f64>(static_cast<s64>(ref<u8>(e + 112 + i)))) * k);
    set_fconst(2, c[0], c[1], c[2], c[3]);
    ctx.gpr[3] = call(ctx, dfn::Draw_ResetState, {1, 0});
}

// void Dof_BlurDown(Dof* e)   (vtable slot 24)
// With no depth target bound: the scene colour target (current eye /
// buffer set) is down-sampled 4x into work target 3, then blurred e +64
// times (11-tap horizontal into work 1, vertical into work 3). Restores
// the depth and colour targets and passes the blurred texture (work 2)
// to vtable slot 28.
DECOMP(005402E4, Dof_BlurDown) {
    const u32 e = arg<u32>(ctx, 0);
    const u32 st = fg::state;
    const u32 depth = load<u32>(st + 84), colour = load<u32>(st + 36);
    if (depth != 0) set_depth_target(ctx, 0);
    const u32 src = scene_colour();
    u32 half = load<u32>(fg::work2);
    call(ctx, dfn::Draw_SetTarget, {load<u32>(fg::work3), 0});
    invalidate_textures(ctx);
    bind(ctx, fg::vp_down4x, fg::fp_down4x);
    reset_state(ctx, 1);
    texture0(ctx, src, 1);
    draw_quad(ctx);
    for (u32 i = 0; i < ref<u8>(e + 64); i = (i + 1) & 0xFF) {
        const u32 h = load<u32>(fg::work0);
        call(ctx, dfn::Draw_SetTarget, {load<u32>(fg::work1), 0});
        invalidate_textures(ctx);
        bind(ctx, fg::vp_blur11h, fg::fp_blur11h);
        reset_state(ctx, 1);
        texture0(ctx, half, 0);
        draw_quad(ctx);
        half = load<u32>(fg::work2);
        call(ctx, dfn::Draw_SetTarget, {load<u32>(fg::work3), 0});
        invalidate_textures(ctx);
        bind(ctx, fg::vp_blur11v, fg::fp_blur11v);
        reset_state(ctx, 1);
        texture0(ctx, h, 0);
        draw_quad(ctx);
    }
    if (load<u32>(st + 84) != depth) set_depth_target(ctx, depth);
    call(ctx, dfn::Draw_SetTarget, {colour, 0});
    ctx.gpr[3] = vcall(ctx, e, 28, {e, half});
}
