// Thermal vision: the screen copied into the effect's texture and drawn
// back through PostProc_ThermalVision with up to three textures.
#include "rsx.h"

namespace sly4::gfx {

namespace {

constexpr u32 vp_thermal = 0x01164178;  // PostProc_ThermalVision
constexpr u32 fp_thermal = 0x01164180;  // FProgram*[2]: PostProc_ThermalVision, the _MN variant
constexpr u32 kClamp = 0x00030303, kWrapUV = 0x00030101;
constexpr u32 kInv255 = 0x00545B58;     // f32 1 / 255

// An object holding a texture at +56.
struct TextureHolder {
    pad<56> _0;
    Texture tex;  // +56
};

struct ThermalVision {
    pad<20> _0;
    gp<TextureHolder> screen;  // +20: the screen copy
    gp<TextureHolder> tex2;    // +24: unit 2
    gp<TextureHolder> tex1;    // +28: unit 1
    u8 mn;                     // +32: use the _MN program
    u8 colour[4];              // +33
    pad<11> _37;
    be_u32 c48[4];             // +48: fragment constant 1
    be_u32 vconst1[4];         // +64: vertex constant 1
    be_u32 c80[4];             // +80: fragment constant 2
    be_u32 c96[4];             // +96: fragment constant 3
};
static_assert(offsetof(ThermalVision, c48) == 48 && offsetof(ThermalVision, c96) == 96);

}  // namespace

// void ThermalVision_Draw(ThermalVision* e)
// Copies colour target 0 into e +20's texture, binds the program
// (vertex constants 0 = 0 and 1 = e +64; fragment records 0..3 = the
// colour / 255, e +48, e +80, e +96), alpha blending on, then binds the
// textures that exist and draws the quad.
void ThermalVision_Draw(ThermalVision* e) {
    copy_colour_to(&or_zero_page(e->screen.get())->tex);  // no null check here, unlike the binds below
    bind_vertex(vprog(vp_thermal));
    bind_fragment(fprog(fp_thermal + (e->mn != 0 ? 4 : 0)));
    call(fn::cellGcmSetVertexProgramConstants, gcm(), 0u, 4u, g::zero_vconsts);
    call(fn::cellGcmSetVertexProgramConstants, gcm(), 1u, 4u, e->vconst1);

    const f32 k = global<be_f32>(kInv255);
    u32 c[4];
    for (u32 i = 0; i < 4; ++i) c[i] = bits(int_to_f32(e->colour[i]) * k);
    set_fconst(0, c[0], c[1], c[2], c[3]);
    set_fconst_from(1, e->c48);
    set_fconst_from(2, e->c80);
    set_fconst_from(3, e->c96);

    emit({kBlendEnable, 1});  // sent twice, as in the original
    blend_on(kSrcAlpha2, kOneMinusSrcAlpha2);
    Draw_ResetState(false, 0);
    if (TextureHolder* a = e->screen) bind_texture(0, &a->tex, kClamp, 1);
    if (TextureHolder* b = e->tex1) bind_texture(1, &b->tex, kWrapUV, 0);
    if (TextureHolder* d = e->tex2) bind_texture(2, &d->tex, kClamp, 1);
    draw_quad();
}

}  // namespace sly4::gfx

NATIVE(00545B60, sly4::gfx::ThermalVision_Draw)
