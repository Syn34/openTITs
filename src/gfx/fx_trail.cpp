// The motion trail: a ping-pong pair of screen copies, offset by the
// camera motion each frame and smeared onto the screen targets.
#include "fx.h"

#include <cmath>

using namespace sly4;

namespace {

namespace dfn = sly4::draw::fn;
namespace ffn = sly4::fx::fn;
namespace fg = sly4::fx::g;
using namespace sly4::fx;  // helpers and method names (fx::g / fx::fn are spelt out)

constexpr u32 vp_smear = 0x010F0B48, fp_smear = 0x010F0B4C;  // PostProc_MotionTrailSmear
constexpr u32 kTextureAddress0 = 0x00041A08;  // TEXTURE_ADDRESS unit 0
constexpr u32 kAddressMode = 0x00030404;      // u / v wrap mode 4 (border?), w clamp

namespace tfn {
constexpr u32 Trail_Sub51279C = 0x0051279C;  // () (unknown)
}  // namespace tfn
constexpr u32 kRenderer = 0x010DDACC;  // object*: vtable slot 16 is called after the trail copy (name?)

// Clears the bound colour target to 0 (depth / stencil 0xFFFFFF00).
void clear(PPUContext& ctx) {
    const u32 st = fg::state;
    call(ctx, ffn::cellGcmSetClearColor, {load<u32>(st), 0});
    call(ctx, ffn::cellGcmSetClearDepthStencil, {load<u32>(st), 0xFFFFFF00u});
    call(ctx, ffn::cellGcmSetStencilMask, {load<u32>(st), 255});
    call(ctx, ffn::cellGcmSetClearSurface, {load<u32>(st), 240});  // R | G | B | A
    call(ctx, ffn::cellGcmSetStencilMask, {load<u32>(st), load<u32>(st + 168)});
}

// Blending, alpha / depth / stencil test and culling off.
void plain_state() {
    emit({kBlendEnable, 0});
    emit({kAlphaTestEnable, 0});
    emit({kDepthMask, 0});
    emit({kDepthTestEnable, 0});
    emit({kStencilTestEnable, 0});
    emit({kCullFaceEnable, 0});
}

}  // namespace

// void MotionTrail_Draw(MotionTrail* e, const f32* motion, bool notify)   (vtable slot 32)
// e: +96 f32[4] colour, +128 f32 offset scale, +132 f32 smear scale,
// +136 f32 threshold, +140 Target* depth, +144 / +152 Target*[2] the
// trail targets / textures, +160 u32 current index.
// Clears the other trail target and copies the current one into it
// (PostProc_CopyScale, shifted by -motion[0..1] * e +128 in vertex
// constant 0). If |motion.xy|^2 > e +136: vtable slot 36 (when notify),
// Trail_Sub51279C, rebind the other target with depth e +140 and call
// the renderer's slot 16. Finally smears the other texture into the
// current target with PostProc_MotionTrailSmear, colour motion * e +132.
DECOMP(00544120, MotionTrail_Draw) {
    const u32 e = arg<u32>(ctx, 0), motion = arg<u32>(ctx, 1);
    const bool notify = (arg<u32>(ctx, 2) & 0xFF) != 0;
    const u32 st = fg::state;
    Stack frame(ctx, 16);  // vertex constant 0: {0, 0, -dx, -dy}
    const u32 vc = frame.addr();
    store<u32>(vc, 0);
    store<u32>(vc + 4, 0);
    const f32 k = load_f32(e + 128);
    const u32 index = load<u32>(e + 160);
    const u32 other = e + ((index ^ 1) << 2), current = e + (index << 2);
    store<u32>(vc + 8, bits(-(load_f32(motion) * k)));
    store<u32>(vc + 12, bits(-(load_f32(motion + 4) * k)));

    call(ctx, ffn::Draw_BindTargets, {0, load<u32>(other + 144), 0});
    clear(ctx);
    bind_vertex(ctx, fg::vp_copy_scale);
    bind_fragment(fg::fp_copy_scale);
    call(ctx, ffn::cellGcmSetVertexProgramConstants, {load<u32>(st), 0, 4, vc});
    set_fconst_from(0, e + 96);
    plain_state();
    call(ctx, dfn::Draw_SetTexture0, {load<u32>(current + 152), 0, 1});
    emit({kTextureAddress0, kAddressMode});
    draw_quad(ctx);

    const f32 vx = load_f32(motion), vy = load_f32(motion + 4);
    const f32 len2 = static_cast<f32>(std::fma(static_cast<f64>(vx), static_cast<f64>(vx), static_cast<f64>(vy * vy)));
    if (len2 > load_f32(e + 136)) {
        if (notify) vcall(ctx, e, 36, {e});
        call(ctx, tfn::Trail_Sub51279C);
        call(ctx, ffn::Draw_BindTargets, {load<u32>(e + 140), load<u32>(other + 144), 0});
        invalidate_textures(ctx);
        const u32 r = load<u32>(kRenderer);
        vcall(ctx, r, 16, {r});
    }

    const f32 s = load_f32(e + 132);
    const u32 c0 = bits(load_f32(motion) * s), c1 = bits(load_f32(motion + 4) * s);
    const u32 c2 = bits(load_f32(motion + 8) * s), c3 = bits(load_f32(motion + 12) * s);
    call(ctx, ffn::Draw_BindTargets, {0, load<u32>(current + 144), 0});
    bind_vertex(ctx, vp_smear);
    bind_fragment(fp_smear);
    set_fconst(0, c0, c1, c2, c3);
    plain_state();
    call(ctx, dfn::Draw_SetTexture0, {load<u32>(other + 152), 0, 1});
    emit({kTextureAddress0, kAddressMode});
    ctx.gpr[3] = draw_quad(ctx);
}
