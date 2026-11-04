// Per-frame post-effect draws (called through effect vtables).
#include "effect_draw.h"

#include "effect_init.h"

using namespace sly4;

namespace {

namespace efn = sly4::effect::fn;

void begin_pass(PPUContext& ctx) {
    call(ctx, efn::Gcm_Sub5E3E64, {load<u32>(post::g::gcm_state), 1});
}

void draw_quad(PPUContext& ctx) {
    call(ctx, efn::Draw_FullscreenQuad);
    ctx.gpr[3] = call(ctx, efn::Draw_EndQuad);
}

}  // namespace

// void PostProc_DrawCopyBlend(Effect* e)
// Copies texture e +152 with PostProc_Copy, alpha blending on
// (BLEND_ENABLE 1, src/dst factors 0x0302 / 0x0001, equation FUNC_ADD).
DECOMP(005433AC, PostProc_DrawCopyBlend) {
    const u32 e = arg<u32>(ctx, 0);
    call(ctx, efn::Draw_Sub513038, {e});
    begin_pass(ctx);
    effect::bind_vertex(ctx, 0x01164080);
    effect::bind_fragment(0x01164084);
    call(ctx, efn::Draw_SetDepthMode, {0, 0});
    effect::emit({0x00040310, 1});                       // BLEND_ENABLE
    effect::emit({0x00080314, 0x03020302, 0x00010001});  // BLEND_FUNC_SFACTOR / DFACTOR
    effect::emit({0x00040320, 0x80068006});              // BLEND_EQUATION: FUNC_ADD
    // Draw_SetTexture0 reads r4 / r5, which the original leaves as 0 / 1
    // from the command writes above: passed explicitly.
    call(ctx, efn::Draw_SetTexture0, {load<u32>(e + 152), 0, 1});
    draw_quad(ctx);
}

// void Blur03_Draw(Effect* e)
// The 3-tap Gaussian blur: horizontal pass from texture e +148 into target
// e +140, then the vertical pass from e +152 into e +136.
DECOMP(00542D2C, Blur03_Draw) {
    const u32 e = arg<u32>(ctx, 0);
    call(ctx, efn::Draw_SetTarget, {load<u32>(e + 140), 0});
    begin_pass(ctx);
    effect::bind_vertex(ctx, 0x011640C0);  // PostProc_GuassianBlur03H
    effect::bind_fragment(0x011640C4);
    call(ctx, efn::Draw_SetDepthMode, {1, 0});
    call(ctx, efn::Draw_SetTexture0, {load<u32>(e + 148), 0, 0});
    draw_quad(ctx);
    call(ctx, efn::Draw_SetTarget, {load<u32>(e + 136), 0});
    begin_pass(ctx);
    effect::bind_vertex(ctx, 0x011640C8);  // PostProc_GuassianBlur03V
    effect::bind_fragment(0x011640CC);
    call(ctx, efn::Draw_SetDepthMode, {1, 0});
    call(ctx, efn::Draw_SetTexture0, {load<u32>(e + 152), 0, 0});
    draw_quad(ctx);
}
