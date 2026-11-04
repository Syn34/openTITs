// The bloom chain: threshold down-sample, one 11-tap blur pass and the
// additive up-sample back onto the target.
#include "fx.h"

using namespace sly4;

namespace {

namespace dfn = sly4::draw::fn;
namespace pfn = sly4::post::fn;
namespace fg = sly4::fx::g;
using namespace sly4::fx;  // helpers and method names (fx::g / fx::fn are spelt out)

constexpr u32 kBloomConsts = 0x0053D198;  // f32 { 255.0, 6.0, 0.25, 0.0, 1.0, 1.0 }
constexpr u32 vp_bloom_down = 0x01164128, fp_bloom_down = 0x0116412C;  // PostProc_BloomDownSample4x
constexpr u32 vp_bloom_up = 0x01164130, fp_bloom_up = 0x01164134;      // PostProc_BloomUpSample4x

void bind(PPUContext& ctx, u32 vertex, u32 fragment) {
    bind_vertex(ctx, vertex);
    bind_fragment(fragment);
}

void reset_state(PPUContext& ctx, u32 no_blend) { call(ctx, dfn::Draw_ResetState, {no_blend, 0}); }

void target(PPUContext& ctx, u32 t) {
    call(ctx, dfn::Draw_SetTarget, {t, 0});
    invalidate_textures(ctx);
}

}  // namespace

// void Bloom_Chain(BloomParams* b)
// b: +0 f32[3] threshold colour (0..255), +16 f32 strength, +20 Target*
// source, +24 Target* destination. With no depth target bound: the
// source is down-sampled 4x (threshold b +0 / 255) into work target 3,
// blurred once (horizontal into work 1, vertical into work 3), then work
// target 2 is added onto b +24 (strength b +16 / 6 * 0.25) and the
// screen targets are bound again.
DECOMP(0053D1B0, Bloom_Chain) {
    const u32 b = arg<u32>(ctx, 0);
    const u32 st = fg::state;
    const u32 k = kBloomConsts;
    if (load<u32>(st + 84) != 0) set_depth_target(ctx, 0);

    const u32 src = load<u32>(b + 20);
    u32 half = load<u32>(fg::work2);
    target(ctx, load<u32>(fg::work3));
    bind(ctx, vp_bloom_down, fp_bloom_down);
    const f32 scale = load_f32(k);
    set_fconst(0, bits(load_f32(b) / scale), bits(load_f32(b + 4) / scale), bits(load_f32(b + 8) / scale), 0);
    reset_state(ctx, 1);
    call(ctx, dfn::Draw_SetTexture0, {src, 0, 1});
    draw_quad(ctx);

    for (u32 i = 0; i < 1; ++i) {
        const u32 h = load<u32>(fg::work0);
        target(ctx, load<u32>(fg::work1));
        bind(ctx, fg::vp_blur11h, fg::fp_blur11h);
        call(ctx, pfn::Blur11H_SetOffsets);
        reset_state(ctx, 1);
        call(ctx, dfn::Draw_SetTexture0, {half, 0, 0});
        draw_quad(ctx);
        half = load<u32>(fg::work2);
        target(ctx, load<u32>(fg::work3));
        bind(ctx, fg::vp_blur11v, fg::fp_blur11v);
        call(ctx, pfn::Blur11V_SetOffsets);
        reset_state(ctx, 1);
        call(ctx, dfn::Draw_SetTexture0, {h, 0, 0});
        draw_quad(ctx);
    }

    target(ctx, load<u32>(b + 24));
    bind(ctx, vp_bloom_up, fp_bloom_up);
    const f32 strength = load_f32(b + 16) / load_f32(k + 4) * load_f32(k + 8);
    set_fconst(0, load<u64>(k + 16), (u64{0x3F800000} << 32) | bits(strength));
    reset_state(ctx, 0);
    blend_on(kSrcAlpha2, kOne2);
    call(ctx, dfn::Draw_SetTexture0, {half, 0, 1});
    const u64 r3 = draw_quad(ctx);
    ctx.gpr[3] = call(ctx, dfn::Draw_BindScreenTargets, {r3});
}
