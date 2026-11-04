// The full-screen blur: the scene down-sampled, blurred once and blended
// back over itself.
#include "fx.h"

using namespace sly4;

namespace {

namespace dfn = sly4::draw::fn;
namespace pfn = sly4::post::fn;
namespace fg = sly4::fx::g;
using namespace sly4::fx;  // helpers and method names (fx::g is spelt out)

constexpr u32 vp_up4x = 0x011640A8, fp_up4x = 0x011640AC;  // PostProc_UpSample4x

void pass(PPUContext& ctx, u32 target, u32 vertex, u32 fragment, u32 offsets, u32 texture, u32 filter) {
    call(ctx, dfn::Draw_SetTarget, {target, 0});
    invalidate_textures(ctx);
    bind_vertex(ctx, vertex);
    bind_fragment(fragment);
    if (offsets != 0) call(ctx, offsets);
    call(ctx, dfn::Draw_ResetState, {1, 0});
    call(ctx, dfn::Draw_SetTexture0, {texture, 0, filter});
    draw_quad(ctx);
}

}  // namespace

// void PostProc_DrawBlur(Effect* e)   (vtable slot 24, the class of PostProc_DrawFade)
// With no depth target bound: the scene colour texture is down-sampled
// 4x into work target 3 and blurred once (11-tap horizontal into work 1,
// vertical into work 3), then work target 2 is up-sampled and alpha
// blended (alpha e +24) back onto the scene's render target. Rebinds
// the display's depth target and the scene target.
DECOMP(0053E3B8, PostProc_DrawBlur) {
    const u32 e = arg<u32>(ctx, 0);
    const u32 st = fg::state;
    if (load<u32>(st + 84) != 0) set_depth_target(ctx, 0);
    const u32 src = scene_colour();
    u32 half = load<u32>(fg::work2);
    pass(ctx, load<u32>(fg::work3), fg::vp_down4x, fg::fp_down4x, 0, src, 1);
    const u32 h = load<u32>(fg::work0);
    pass(ctx, load<u32>(fg::work1), fg::vp_blur11h, fg::fp_blur11h, pfn::Blur11H_SetOffsets, half, 0);
    half = load<u32>(fg::work2);
    pass(ctx, load<u32>(fg::work3), fg::vp_blur11v, fg::fp_blur11v, pfn::Blur11V_SetOffsets, h, 0);

    call(ctx, dfn::Draw_SetTarget, {load<u32>(scene_colour() + 40), 0});
    invalidate_textures(ctx);
    bind_vertex(ctx, vp_up4x);
    bind_fragment(fp_up4x);
    set_fconst(0, load<u64>(0x0053E3B0) /* f32 { 1.0, 1.0 } */, (u64{0x3F800000} << 32) | load<u32>(e + 24));
    blend_on(kSrcAlpha2, kOneMinusSrcAlpha2);
    call(ctx, dfn::Draw_ResetState, {0, 0});
    call(ctx, dfn::Draw_SetTexture0, {half, 0, 1});
    draw_quad(ctx);

    const u32 depth = display_depth();
    if (load<u32>(st + 84) != depth) set_depth_target(ctx, depth);
    ctx.gpr[3] = call(ctx, dfn::Draw_SetTarget, {load<u32>(scene_colour() + 40), 0});
}
