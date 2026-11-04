// Thermal vision: the screen copied into the effect's texture and drawn
// back through PostProc_ThermalVision with up to three textures.
#include "fx.h"

using namespace sly4;

namespace {

namespace dfn = sly4::draw::fn;
namespace ffn = sly4::fx::fn;
namespace fg = sly4::fx::g;
using namespace sly4::fx;  // helpers and method names (fx::g / fx::fn are spelt out)

constexpr u32 vp_thermal = 0x01164178;  // PostProc_ThermalVision
constexpr u32 fp_thermal = 0x01164180;  // FProgram*[2]: PostProc_ThermalVision, the _MN variant
constexpr u32 kClamp = 0x00030303, kWrapUV = 0x00030101;

}  // namespace

// void ThermalVision_Draw(ThermalVision* e)
// e: +20 / +24 / +28 objects whose +56 is a texture (the screen copy,
// unit 2, unit 1), +32 u8 use the _MN program, +33 u8[4] colour,
// +48 / +80 / +96 f32[4] constants, +64 f32[4] vertex constant 1.
// Copies colour target 0 into e +20's texture, binds the program
// (vertex constants 0 = 0 and 1 = e +64; fragment records 0..3 = the
// colour / 255, e +48, e +80, e +96), alpha blending on, then binds the
// textures that exist and draws the quad.
DECOMP(00545B60, ThermalVision_Draw) {
    const u32 e = arg<u32>(ctx, 0);
    const u32 st = fg::state;
    copy_colour_to(ctx, load<u32>(e + 20) + 56);  // no null check here, unlike the binds below
    bind_vertex(ctx, vp_thermal);
    bind_fragment(fp_thermal + (ref<u8>(e + 32) != 0 ? 4 : 0));
    call(ctx, ffn::cellGcmSetVertexProgramConstants, {load<u32>(st), 0, 4, fg::zero_vconsts});
    call(ctx, ffn::cellGcmSetVertexProgramConstants, {load<u32>(st), 1, 4, e + 64});

    const f32 k = load_f32(0x00545B58);  // 1 / 255
    u32 c[4];
    for (u32 i = 0; i < 4; ++i)
        c[i] = bits(static_cast<f32>(static_cast<f64>(static_cast<s64>(ref<u8>(e + 33 + i)))) * k);
    set_fconst(0, c[0], c[1], c[2], c[3]);
    set_fconst_from(1, e + 48);
    set_fconst_from(2, e + 80);
    set_fconst_from(3, e + 96);

    emit({kBlendEnable, 1});  // sent twice, as in the original
    blend_on(kSrcAlpha2, kOneMinusSrcAlpha2);
    call(ctx, dfn::Draw_ResetState, {0, 0});
    if (const u32 a = load<u32>(e + 20); a != 0) bind_texture(ctx, 0, a + 56, kClamp, 1);
    if (const u32 b = load<u32>(e + 28); b != 0) bind_texture(ctx, 1, b + 56, kWrapUV, 0);
    if (const u32 d = load<u32>(e + 24); d != 0) bind_texture(ctx, 2, d + 56, kClamp, 1);
    ctx.gpr[3] = draw_quad(ctx);
}
