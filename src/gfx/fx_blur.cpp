// The full-screen blur: the scene down-sampled, blurred once and blended
// back over itself.
#include "rsx.h"

namespace sly4::gfx {

namespace {

constexpr u32 vp_up4x = 0x011640A8, fp_up4x = 0x011640AC;      // PostProc_UpSample4x
constexpr u32 vp_down4x = 0x011640A0, fp_down4x = 0x011640A4;  // PostProc_DownSample4x
constexpr u32 vp_blur11h = 0x011640B0, fp_blur11h = 0x011640B4;
constexpr u32 vp_blur11v = 0x011640B8, fp_blur11v = 0x011640BC;

struct BlurEffect {
    pad<24> _0;
    be_u32 alpha;  // +24: f32
};

void pass(Target* target, u32 vertex, u32 fragment, void (*offsets)(), Texture* texture, u32 filter) {
    Draw_SetTarget(target, 0);
    invalidate_textures();
    bind_vertex(vprog(vertex));
    bind_fragment(fprog(fragment));
    if (offsets) offsets();
    Draw_ResetState(true, 0);
    Draw_SetTexture0(texture, 0, filter);
    draw_quad();
}

}  // namespace

// void PostProc_DrawBlur(Effect* e)   (vtable slot 24, the class of PostProc_DrawFade)
// With no depth target bound: the scene colour texture is down-sampled
// 4x into work target 3 and blurred once (11-tap horizontal into work 1,
// vertical into work 3), then work target 2 is up-sampled and alpha
// blended (alpha e +24) back onto the scene's render target. Rebinds
// the display's depth target and the scene target.
void PostProc_DrawBlur(BlurEffect* e) {
    RenderState& st = rs();
    if (st.depth) set_depth_target(nullptr);
    Texture* src = scene_colour();
    Texture* half = work_texture(2);
    pass(work_target(3), vp_down4x, fp_down4x, nullptr, src, 1);
    Texture* h = work_texture(0);
    pass(work_target(1), vp_blur11h, fp_blur11h, Blur11H_SetOffsets, half, 0);
    half = work_texture(2);
    pass(work_target(3), vp_blur11v, fp_blur11v, Blur11V_SetOffsets, h, 0);

    Draw_SetTarget(scene_target(scene_colour()), 0);
    invalidate_textures();
    bind_vertex(vprog(vp_up4x));
    bind_fragment(fprog(fp_up4x));
    set_fconst(0, global<be_u64>(0x0053E3B0) /* f32 { 1.0, 1.0 } */, (u64{0x3F800000} << 32) | e->alpha);
    blend_on(kSrcAlpha2, kOneMinusSrcAlpha2);
    Draw_ResetState(false, 0);
    Draw_SetTexture0(half, 0, 1);
    draw_quad();

    Target* depth = display_depth();
    if (st.depth.get() != depth) set_depth_target(depth);
    Draw_SetTarget(scene_target(scene_colour()), 0);
}

}  // namespace sly4::gfx

NATIVE(0053E3B8, sly4::gfx::PostProc_DrawBlur)
