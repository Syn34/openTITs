// The bloom-noise post effect: three rotated, scaled and scrolled layers
// of noise (with an optional bloom of the scene) drawn into a work
// target and added back onto the screen.
#include "render.h"

#include <cmath>

#include "../view/view2.h"

using namespace sly4;

namespace {

namespace dfn = sly4::draw::fn;
namespace pfn = sly4::post::fn;
namespace rfn = sly4::render::fn;
namespace fg = sly4::fx::g;
using namespace sly4::fx;  // helpers and method names (fx::g is spelt out)

constexpr u32 kStats = sly4::g::mem_stats;
constexpr u32 kConsts = 0x00540DC4;  // f32 { 0.0, 6.0, 0.25, 1/255, 255.0 }
constexpr u32 vp_down = 0x01164160, fp_down = 0x01164164;  // PostProc_BloomDownSample4x (BloomNoise_Init)
constexpr u32 vp_noise = 0x01164168;  // PostProc_BloomNoise
constexpr u32 fp_noise = 0x01164170;  // FProgram*[2]: PostProc_BloomNoise_NoBloom, PostProc_BloomNoise
constexpr u32 kAxis = 0x01164B70;     // f32[3]: axis the noise layers rotate about (bss)
constexpr u32 kNoiseAddress = 0x00030101;  // TEXTURE_ADDRESS word for the noise textures
constexpr u32 kNoiseDst = 0x00000302;      // blend dfactor word of the noise pass
constexpr u32 Matrix_FromAxisAngle = 0x0056A410;  // decompiled: src/view/matrix.cpp

// Fragment parameter records of the noise programs (bindings at 0x00F67790).
enum : u32 {
    kIndirectMtx1 = 0,  // gps_vIndirectMtx1[0..2]; layer i uses 3 * i ..
    kWeights = 9,       // gps_vWeights
    kNoiseColorTint = 10,  // gps_vNoiseColorTint
    kMaskParamsX = 11,     // gps_vMaskParamsX
    kMaskParamsY = 12,     // gps_vMaskParamsY
};

bool has_fconst(u32 k) { return load<u32>(load<u32>(fg::state + 28) + 592 + 16 * k) != 0; }
u64 hi_word(u64 v) { return v & 0xFFFFFFFF00000000ull; }  // rldimi with 0: low word cleared

void target(PPUContext& ctx, u32 t) {
    call(ctx, dfn::Draw_SetTarget, {t, 0});
    invalidate_textures(ctx);
}

void reset_state(PPUContext& ctx, u32 no_blend) { call(ctx, dfn::Draw_ResetState, {no_blend, 0}); }

}  // namespace

// void BloomNoise_Draw(BloomNoise* e)   (virtual)
// e: three 24-byte noise layers at +20 {f32 scale x, scale y, scroll x,
// scroll y, angle, weight}, +92 u8[4] tint (0..255), +96 f32 tint scale,
// +100 f32 tint alpha, +104 / +132 objects whose +56 is noise texture
// 0 / 1, +108..+128 mask parameters, +144 f32[3] bloom threshold
// (0..255), +160 f32 bloom amount, +164 f32 mask scale.
// With bloom (e +160 > 0) the scene is down-sampled past the threshold
// into work 3 and blurred once (work 1, work 3) as Bloom_Chain does; the
// blurred result (work 2) is unit 2 of the noise pass. The noise pass
// draws into work 1 with blending (ONE / src alpha); each layer's
// rotation (Matrix_FromAxisAngle about kAxis), scale and the fractional
// part of its rotated scroll go into gps_vIndirectMtx<i>. Work 0 is then
// added onto the scene target (PostProc_Copy), and the display depth
// and scene targets are bound again.
// The xyz-only layer rows leave the w of three constant records
// (stack r1 +236 / +252 / +268) unwritten: the original sends whatever
// the stack held there, so the frame mirrors the original's.
DECOMP(00540DD8, BloomNoise_Draw) {
    const u32 e = arg<u32>(ctx, 0);
    Stack frame(ctx, 416);  // mirrors the original's 528-byte frame
    const u32 sp = frame.addr() - 112;
    const u32 k = kConsts;

    call(ctx, rfn::Draw_SetDepthTarget, {0});
    u32 tex = static_cast<u32>(call(ctx, rfn::Display_SelectBuffer, {kStats}));  // the scene colour
    bool bloom = false;
    if (load_f32(e + 160) > load_f32(k)) {
        const f32 c255 = load_f32(k + 16);
        store<f32>(sp + 176, load_f32(e + 144) / c255);
        store<f32>(sp + 180, load_f32(e + 148) / c255);
        store<f32>(sp + 184, load_f32(e + 152) / c255);
        u32 half = load<u32>(fg::work2);
        target(ctx, load<u32>(fg::work3));
        bind_vertex(ctx, vp_down);
        bind_fragment(fp_down);
        set_fconst(0, load<u64>(sp + 176), hi_word(load<u64>(sp + 184)));
        reset_state(ctx, 1);
        call(ctx, dfn::Draw_SetTexture0, {tex, 0, 1});
        draw_quad(ctx);
        for (u32 i = 0; i < 1; ++i) {
            const u32 h = load<u32>(fg::work0);
            target(ctx, load<u32>(fg::work1));
            bind_vertex(ctx, fg::vp_blur11h);
            bind_fragment(fg::fp_blur11h);
            call(ctx, pfn::Blur11H_SetOffsets);
            reset_state(ctx, 1);
            call(ctx, dfn::Draw_SetTexture0, {half, 0, 0});
            draw_quad(ctx);
            half = load<u32>(fg::work2);
            target(ctx, load<u32>(fg::work3));
            bind_vertex(ctx, fg::vp_blur11v);
            bind_fragment(fg::fp_blur11v);
            call(ctx, pfn::Blur11V_SetOffsets);
            reset_state(ctx, 1);
            call(ctx, dfn::Draw_SetTexture0, {h, 0, 0});
            draw_quad(ctx);
        }
        bloom = true;
        tex = half;
    }

    // The noise pass.
    const u32 ss = system_state::get(ctx);
    const f32 weight = load_f32(e + 160) / load_f32(k + 4) * load_f32(k + 8);
    const f32 s = load_f32(ss + 16);
    const u32 fp = fp_noise + (bloom ? 4u : 0u);
    const u32 screen = load<u32>(fg::work0);
    target(ctx, load<u32>(fg::work1));
    bind_vertex(ctx, vp_noise);
    bind_fragment(fp);
    reset_state(ctx, 1);
    emit({kBlendEnable, 1});
    emit({kBlendFunc, kOne2, kNoiseDst});
    emit({kBlendEquation, kFuncAdd2});
    if (const u32 t0 = load<u32>(e + 104); t0 != 0) bind_texture(ctx, 0, t0 + 56, kNoiseAddress, 1);
    if (const u32 t1 = load<u32>(e + 132); t1 != 0) bind_texture(ctx, 1, t1 + 56, kNoiseAddress, 1);
    if (bloom) call(ctx, dfn::Draw_SetTexture0, {tex, 2, 1});

    const f32 inv255 = load_f32(k + 12);
    const f32 tint = load_f32(e + 96);
    f32 rgba[4];
    for (u32 i = 0; i < 4; ++i) {
        const u8 c = ref<u8>(e + 92 + i);
        store<u64>(sp + 368, c);  // the int -> float conversion slot
        rgba[i] = static_cast<f32>(c) * inv255;
    }
    const f32 m = load_f32(e + 164);
    store<f32>(sp + 128, rgba[0] * tint);
    store<f32>(sp + 132, rgba[1] * tint);
    store<f32>(sp + 136, rgba[2] * tint);
    store<f32>(sp + 140, load_f32(e + 100) * m * rgba[3]);
    store<f32>(sp + 144, load_f32(e + 124) * m);
    store<f32>(sp + 148, load_f32(e + 108) * m);
    store<f32>(sp + 152, load_f32(e + 116) * s);
    store<f32>(sp + 160, load_f32(e + 128) * m);
    store<f32>(sp + 164, load_f32(e + 112) * m);
    store<f32>(sp + 168, load_f32(e + 120) * s);
    if (has_fconst(kWeights)) {
        store<u32>(sp + 112, load<u32>(e + 40));
        store<u32>(sp + 116, load<u32>(e + 64));
        store<u32>(sp + 120, load<u32>(e + 88));
        store<f32>(sp + 124, weight);
        set_fconst(kWeights, load<u64>(sp + 112), load<u64>(sp + 120));
    }
    set_fconst(kNoiseColorTint, load<u64>(sp + 128), load<u64>(sp + 136));
    set_fconst(kMaskParamsX, load<u64>(sp + 144), hi_word(load<u64>(sp + 152)));
    set_fconst(kMaskParamsY, load<u64>(sp + 160), hi_word(load<u64>(sp + 168)));

    // Layers: first parameter record 0, 3, 6 (a table at r1 +336).
    store<u32>(sp + 336, kIndirectMtx1);
    store<u32>(sp + 340, 3);
    store<u32>(sp + 344, 6);
    const u32 mtx = sp + 272;  // f32[4][4]; rows' w never written
    for (u32 i = 0; i < 3; ++i) {
        const u32 layer = e + 24 * i;
        store<u32>(sp + 192, load<u32>(layer + 20));  // scale {x, y, 0}
        store<u32>(sp + 196, load<u32>(layer + 24));
        store<u32>(sp + 200, 0);
        const f32 ox = load_f32(layer + 28) * s, oy = load_f32(layer + 32) * s;
        store<f32>(sp + 208, ox);  // scroll {x, y, 0}
        store<u32>(sp + 216, 0);
        store<f32>(sp + 212, oy);
        callf(ctx, Matrix_FromAxisAngle, {mtx, kAxis}, {static_cast<f64>(load_f32(layer + 36))});

        // The scroll through the rotation (VMX, per lane):
        // row0 * ox + row1 * oy + row2 * 0 + row3.
        for (u32 j = 0; j < 4; ++j) store<f32>(sp + 352 + 4 * j, ox);
        f32 p[4];
        for (u32 j = 0; j < 4; ++j) p[j] = std::fma(load_f32(mtx + 4 * j), ox, 0.0f);
        for (u32 j = 0; j < 4; ++j) store<f32>(sp + 352 + 4 * j, oy);
        for (u32 j = 0; j < 4; ++j) {
            p[j] = std::fma(load_f32(mtx + 16 + 4 * j), oy, p[j]);
            p[j] = std::fma(load_f32(mtx + 32 + 4 * j), 0.0f, p[j]);
            p[j] = load_f32(mtx + 48 + 4 * j) + p[j];
        }
        for (u32 j = 0; j < 4; ++j) store<f32>(sp + 208 + 4 * j, p[j]);

        const f32 px = load_f32(sp + 208), py = load_f32(sp + 212);
        const f32 fx = px - static_cast<f32>(rt::fcfid(rt::fctid(px, true)));  // fractional parts
        const f32 fy = py - static_cast<f32>(rt::fcfid(rt::fctid(py, true)));
        call(ctx, view::v2fn::Matrix_ScaleRows, {mtx, sp + 192});
        const u32 pz = load<u32>(sp + 216);
        store<f32>(sp + 304, fx);  // row 2 becomes {fx, fy, pz}
        store<f32>(sp + 308, fy);
        store<u32>(sp + 312, pz);
        store<u32>(sp + 240, load<u32>(sp + 288));  // row 1 xyz (+252 not written)
        store<u32>(sp + 248, load<u32>(sp + 296));
        store<f32>(sp + 256, fx);  // {fx, fy, pz} (+268 not written)
        store<u32>(sp + 264, pz);
        store<u32>(sp + 244, load<u32>(sp + 292));
        store<f32>(sp + 260, fy);

        const u32 first = load<u32>(sp + 336 + 4 * i);
        const u64 r1a = load<u64>(sp + 240), r1b = load<u64>(sp + 248);
        const u64 r2a = load<u64>(sp + 256), r2b = load<u64>(sp + 264);
        if (has_fconst(first)) {
            store<u32>(sp + 224, load<u32>(mtx));  // row 0 xyz (+236 not written)
            store<u32>(sp + 232, load<u32>(mtx + 8));
            store<u32>(sp + 228, load<u32>(mtx + 4));
            set_fconst(first, load<u64>(sp + 224), load<u64>(sp + 232));
        }
        set_fconst(first + 1, r1a, r1b);
        set_fconst(first + 2, r2a, r2b);
    }
    draw_quad(ctx);

    // Add work 0 onto the scene target.
    target(ctx, load<u32>(scene_colour() + 40));
    bind_vertex(ctx, fg::vp_copy);
    bind_fragment(fg::fp_copy);
    reset_state(ctx, 0);
    emit({kBlendEnable, 1});
    emit({kBlendEnable, 1});  // twice in the original
    emit({kBlendFunc, kOne2, kOneMinusSrcAlpha2});
    emit({kBlendEquation, kFuncAdd2});
    call(ctx, dfn::Draw_SetTexture0, {screen, 0, 1});
    draw_quad(ctx);

    render::set_depth_target(ctx, display_depth());
    ctx.gpr[3] = call(ctx, dfn::Draw_SetTarget, {load<u32>(scene_colour() + 40), 0});
}
