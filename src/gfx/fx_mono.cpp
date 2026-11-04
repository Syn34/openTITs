// The monochrome screen effect: a tinted grey-scale (or inverted) copy
// of the screen, or a flat fill, blended in one of four ways.
#include "fx.h"

using namespace sly4;

namespace {

namespace dfn = sly4::draw::fn;
namespace ffn = sly4::fx::fn;
namespace fg = sly4::fx::g;
using namespace sly4::fx;  // helpers and method names (fx::g / fx::fn are spelt out)

constexpr u32 kStats = sly4::g::mem_stats;
constexpr u32 kConfig = sly4::g::config;
constexpr u32 vp_mono = 0x01164138, fp_mono = 0x0116413C;      // PostProc_Monochrome
constexpr u32 vp_inv = 0x01164140, fp_inv = 0x01164144;        // PostProc_InvMonochrome
constexpr u32 vp_fill = 0x01164088, fp_fill = 0x0116408C;      // PostProc_Fill
constexpr u32 kUsed = 0x00F679FD;      // u8 set once the effect has drawn (name?)
constexpr u32 kBadKind = 0x011647F2;   // u8 set when e +48 is out of range (name?)
constexpr u32 kSrcAlpha = 770, kOneMinusSrcAlpha = 771, kOne = 1, kZero = 0;
constexpr u32 kFuncAdd = 0x8006, kFuncReverseSubtract = 0x800B;

u32 pair(u32 v) { return v | (v << 16); }

// Vertex constant 1: the tint e +32..+40 / 255 and the amount e +20.
void tint(PPUContext& ctx, u32 e) {
    const f32 k = load_f32(0x0053F048);  // 255.0
    Stack frame(ctx, 16);
    const u32 v = frame.addr();
    store<u32>(v, bits(load_f32(e + 32) / k));
    store<u32>(v + 4, bits(load_f32(e + 36) / k));
    store<u32>(v + 8, bits(load_f32(e + 40) / k));
    store<u32>(v + 12, load<u32>(e + 20));
    call(ctx, ffn::cellGcmSetVertexProgramConstants, {load<u32>(fg::state), 1, 4, v});
}

}  // namespace

// void Monochrome_Draw(Monochrome* e)   (vtable slot 8)
// Skipped unless vtable slot 12 returns true. e +48 picks the program:
// 0 PostProc_Monochrome and 1 PostProc_InvMonochrome of a copy of
// colour target 0 (the scene texture), 2 PostProc_Fill; others only set
// a flag. e +52 picks the blend: 2 alpha, 3 additive, 4 reverse
// subtract, anything else off.
DECOMP(0053F050, Monochrome_Draw) {
    const u32 e = arg<u32>(ctx, 0);
    if (static_cast<u32>(vcall(ctx, e, 12, {e})) == 0) {
        ctx.gpr[3] = 0;
        return;
    }
    if (ref<u8>(kUsed) == 0) ref<u8>(kUsed) = 1;

    const s32 mode = load<s32>(e + 52);
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

    const s32 kind = load<s32>(e + 48);
    if (kind < 0 || kind > 2) {
        if (ref<u8>(kBadKind) != 0) {
            ctx.gpr[3] = ref<u8>(kBadKind);
            return;
        }
        ref<u8>(kBadKind) = 1;
        ctx.gpr[3] = 0x01160000;  // what the original leaves in r3
        return;
    }
    if (kind == 2) {
        blend_state();
        bind_vertex(ctx, vp_fill);
        bind_fragment(fp_fill);
        tint(ctx, e);
        call(ctx, dfn::Draw_ResetState, {0, 0});
        ctx.gpr[3] = draw_quad(ctx);
        return;
    }
    const u32 tex = load<u32>(kStats + (field<u8>(kConfig, 34) != 0 ? 100 : 84));
    copy_colour_to(ctx, tex);
    if (kind == 0) {
        bind_vertex(ctx, vp_mono);
        bind_fragment(fp_mono);
    } else {
        bind_vertex(ctx, vp_inv);
        bind_fragment(fp_inv);
    }
    tint(ctx, e);
    blend_state();
    call(ctx, dfn::Draw_ResetState, {0, 0});
    call(ctx, dfn::Draw_SetTexture0, {tex, 0, 1});
    ctx.gpr[3] = draw_quad(ctx);
}
