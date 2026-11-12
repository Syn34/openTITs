// The monochrome screen effect: a tinted grey-scale (or inverted) copy
// of the screen, or a flat fill, blended in one of four ways.
#include "rsx.h"

namespace sly4::gfx {

namespace {

constexpr u32 vp_mono = 0x01164138, fp_mono = 0x0116413C;  // PostProc_Monochrome
constexpr u32 vp_inv = 0x01164140, fp_inv = 0x01164144;    // PostProc_InvMonochrome
constexpr u32 vp_fill = 0x01164088, fp_fill = 0x0116408C;  // PostProc_Fill
constexpr u32 kUsed = 0x00F679FD;     // u8 set once the effect has drawn (name?)
constexpr u32 kBadKind = 0x011647F2;  // u8 set when e +48 is out of range (name?)
constexpr u32 kSrcAlpha = 770, kOneMinusSrcAlpha = 771, kOne = 1, kZero = 0;
constexpr u32 kFuncAdd = 0x8006, kFuncReverseSubtract = 0x800B;

struct Monochrome {
    pad<20> _0;
    be_u32 amount;     // +20: f32
    pad<8> _24;
    be_f32 tint[3];    // +32: 0..255
    pad<4> _44;
    be_s32 kind;       // +48: 0 grey, 1 inverted, 2 fill
    be_s32 blend;      // +52
};
static_assert(offsetof(Monochrome, kind) == 48);

u32 pair(u32 v) { return v | (v << 16); }

// Vertex constant 1: the tint e +32..+40 / 255 and the amount e +20.
void tint(Monochrome* e) {
    const f32 k = global<be_f32>(0x0053F048);  // 255.0
    GuestFrame f(16);
    be_u32* v = f.as<be_u32>();
    v[0] = bits(e->tint[0] / k);
    v[1] = bits(e->tint[1] / k);
    v[2] = bits(e->tint[2] / k);
    v[3] = e->amount.get();
    call(fn::cellGcmSetVertexProgramConstants, gcm(), 1u, 4u, v);
}

}  // namespace

// void Monochrome_Draw(Monochrome* e)   (vtable slot 8)
// Skipped unless vtable slot 12 returns true. e +48 picks the program:
// 0 PostProc_Monochrome and 1 PostProc_InvMonochrome of a copy of
// colour target 0 (the scene texture), 2 PostProc_Fill; others only set
// a flag. e +52 picks the blend: 2 alpha, 3 additive, 4 reverse
// subtract, anything else off.
void Monochrome_Draw(Monochrome* e) {
    if (vcall<u32>(e, 12) == 0) return;
    u8& used = global<u8>(kUsed);
    if (used == 0) used = 1;

    const s32 mode = e->blend;
    bool blend = true;
    u32 sfactor = kSrcAlpha, dfactor = kOneMinusSrcAlpha, equation = kFuncAdd;
    if (mode == 3) {
        dfactor = kOne;
    } else if (mode == 4) {
        dfactor = kOne;
        equation = kFuncReverseSubtract;
    } else if (mode != 2) {
        blend = false;
        sfactor = kOne;
        dfactor = kZero;
    }
    auto blend_state = [&] {
        if (blend) {
            emit({kBlendEnable, 1});
            emit({kBlendFunc, pair(sfactor), pair(dfactor)});
            emit({kBlendEquation, pair(equation)});
        } else {
            emit({kBlendEnable, 0});
        }
    };

    const s32 kind = e->kind;
    if (kind < 0 || kind > 2) {
        u8& bad = global<u8>(kBadKind);
        if (bad == 0) bad = 1;
        return;
    }
    if (kind == 2) {
        blend_state();
        bind_vertex(vprog(vp_fill));
        bind_fragment(fprog(fp_fill));
        tint(e);
        Draw_ResetState(false, 0);
        draw_quad();
        return;
    }
    Texture* tex = stats_texture(config_u8(34) != 0 ? 100 : 84);
    copy_colour_to(tex);
    if (kind == 0) {
        bind_vertex(vprog(vp_mono));
        bind_fragment(fprog(fp_mono));
    } else {
        bind_vertex(vprog(vp_inv));
        bind_fragment(fprog(fp_inv));
    }
    tint(e);
    blend_state();
    Draw_ResetState(false, 0);
    Draw_SetTexture0(tex, 0, 1);
    draw_quad();
}

}  // namespace sly4::gfx

NATIVE(0053F050, sly4::gfx::Monochrome_Draw)
