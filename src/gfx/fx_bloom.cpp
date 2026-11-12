// The bloom chain: threshold down-sample, one 11-tap blur pass and the
// additive up-sample back onto the target.
#include "rsx.h"

namespace sly4::gfx {

namespace {

constexpr u32 kBloomConsts = 0x0053D198;  // f32 { 255.0, 6.0, 0.25, 0.0, 1.0, 1.0 }
constexpr u32 vp_bloom_down = 0x01164128, fp_bloom_down = 0x0116412C;  // PostProc_BloomDownSample4x
constexpr u32 vp_bloom_up = 0x01164130, fp_bloom_up = 0x01164134;      // PostProc_BloomUpSample4x
constexpr u32 vp_blur11h = 0x011640B0, fp_blur11h = 0x011640B4;
constexpr u32 vp_blur11v = 0x011640B8, fp_blur11v = 0x011640BC;

struct BloomParams {
    be_f32 threshold[3];  // +0: colour, 0..255
    pad<4> _12;
    be_f32 strength;      // +16
    gp<Texture> source;   // +20
    gp<Target> dest;      // +24
};

void bind(u32 vertex, u32 fragment) {
    bind_vertex(vprog(vertex));
    bind_fragment(fprog(fragment));
}

}  // namespace

// void Bloom_Chain(BloomParams* b)
// With no depth target bound: the source is down-sampled 4x (threshold
// b +0 / 255) into work target 3, blurred once (horizontal into work 1,
// vertical into work 3), then work target 2 is added onto b +24
// (strength b +16 / 6 * 0.25) and the screen targets are bound again.
void Bloom_Chain(BloomParams* b) {
    const be_f32* k = host<be_f32>(kBloomConsts);
    if (rs().depth) set_depth_target(nullptr);

    Texture* src = b->source;
    Texture* half = work_texture(2);
    set_target(work_target(3));
    bind(vp_bloom_down, fp_bloom_down);
    const f32 scale = k[0];
    set_fconst(0, bits(b->threshold[0] / scale), bits(b->threshold[1] / scale), bits(b->threshold[2] / scale), 0);
    Draw_ResetState(true, 0);
    Draw_SetTexture0(src, 0, 1);
    draw_quad();

    for (u32 i = 0; i < 1; ++i) {
        Texture* h = work_texture(0);
        set_target(work_target(1));
        bind(vp_blur11h, fp_blur11h);
        Blur11H_SetOffsets();
        Draw_ResetState(true, 0);
        Draw_SetTexture0(half, 0, 0);
        draw_quad();
        half = work_texture(2);
        set_target(work_target(3));
        bind(vp_blur11v, fp_blur11v);
        Blur11V_SetOffsets();
        Draw_ResetState(true, 0);
        Draw_SetTexture0(h, 0, 0);
        draw_quad();
    }

    set_target(b->dest);
    bind(vp_bloom_up, fp_bloom_up);
    const f32 strength = b->strength / k[1] * k[2];
    set_fconst(0, global<be_u64>(kBloomConsts + 16), (u64{0x3F800000} << 32) | bits(strength));
    Draw_ResetState(false, 0);
    blend_on(kSrcAlpha2, kOne2);
    Draw_SetTexture0(half, 0, 1);
    draw_quad();
    Draw_BindScreenTargets(nullptr);
}

}  // namespace sly4::gfx

NATIVE(0053D1B0, sly4::gfx::Bloom_Chain)
