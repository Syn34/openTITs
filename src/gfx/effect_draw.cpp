// Per-frame post-effect draws (called through effect vtables).
#include "rsx.h"

namespace sly4::gfx {

namespace {

// The texture / target slots of the effects drawn here.
struct CopyBlendEffect {
    pad<136> _0;
    gp<Target> target_v;  // +136: vertical pass target (Blur03)
    gp<Target> target_h;  // +140: horizontal pass target (Blur03)
    pad<4> _144;
    gp<Texture> tex_h;    // +148: horizontal pass source (Blur03)
    gp<Texture> tex;      // +152: source (CopyBlend), vertical pass source (Blur03)
};
static_assert(offsetof(CopyBlendEffect, tex) == 152);

constexpr u32 vp_copy = 0x01164080, fp_copy = 0x01164084;      // PostProc_Copy
constexpr u32 vp_blur03h = 0x011640C0, fp_blur03h = 0x011640C4;  // PostProc_GuassianBlur03H
constexpr u32 vp_blur03v = 0x011640C8, fp_blur03v = 0x011640CC;  // PostProc_GuassianBlur03V

}  // namespace

// void PostProc_DrawCopyBlend(Effect* e)
// Copies texture e +152 with PostProc_Copy, alpha blending on
// (BLEND_ENABLE 1, src/dst factors 0x0302 / 0x0001, equation FUNC_ADD).
void PostProc_DrawCopyBlend(CopyBlendEffect* e) {
    Draw_BindScreenTargets(e);
    invalidate_textures();
    bind_vertex(vprog(vp_copy));
    bind_fragment(fprog(fp_copy));
    Draw_ResetState(false, 0);
    blend_on(kSrcAlpha2, kOne2);
    Draw_SetTexture0(e->tex, 0, 1);
    draw_quad();
}

// void Blur03_Draw(Effect* e)
// The 3-tap Gaussian blur: horizontal pass from texture e +148 into target
// e +140, then the vertical pass from e +152 into e +136.
void Blur03_Draw(CopyBlendEffect* e) {
    Draw_SetTarget(e->target_h, 0);
    invalidate_textures();
    bind_vertex(vprog(vp_blur03h));
    bind_fragment(fprog(fp_blur03h));
    Draw_ResetState(true, 0);
    Draw_SetTexture0(e->tex_h, 0, 0);
    draw_quad();
    Draw_SetTarget(e->target_v, 0);
    invalidate_textures();
    bind_vertex(vprog(vp_blur03v));
    bind_fragment(fprog(fp_blur03v));
    Draw_ResetState(true, 0);
    Draw_SetTexture0(e->tex, 0, 0);
    draw_quad();
}

}  // namespace sly4::gfx

NATIVE(005433AC, sly4::gfx::PostProc_DrawCopyBlend)
NATIVE(00542D2C, sly4::gfx::Blur03_Draw)
