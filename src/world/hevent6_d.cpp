// World_HandleEvent callees, part 6: the subtitle line tick and the object
// highlight reset (114..126 instructions).
#include <bit>
#include <cmath>

#include "hevent.h"

using namespace sly4;
using namespace sly4::hevent;

namespace sly4 {
namespace hevent6d {
namespace dfn {
constexpr u32 Sound_IsDone4431A0 = 0x004431A0;  // (Sound*) -> bool (name?)
constexpr u32 Sound_Time444260 = 0x00444260;    // (Sound*) -> f32 seconds (name?)
constexpr u32 Audio_Release446384 = 0x00446384; // (SoundMgr*, Sound*)
constexpr u32 Line_Scroll4E20A8 = 0x004E20A8;   // (Line*) -> f32 per-row offset (name?)
}  // namespace dfn

constexpr u32 kAudioMgr = 0x010ACC40;           // as hevent3_h.cpp
constexpr u32 kSubStart = 0x010C1CB4;           // f32 start time (name?)
constexpr u32 kSubHalf = 0x004A8AE0;            // f32 0.5
constexpr u32 kSubHalfD = 0x004A8AE8;           // f64 0.5
constexpr u32 kSubZero = 0x004A8AE4;            // f32 0.0
constexpr u32 kSubBase = 0x010C1D4C;            // f32 added to the line time (name?)
constexpr u32 kSubRows = 0x010C1D00;            // u32 visible rows (name?)

constexpr u32 kZeroVec = 0x01164B40;            // vec3 0
constexpr u32 kWhite = 0x01164CA0;              // u8[4], as uielem/build6.h
}  // namespace hevent6d
}  // namespace sly4

using namespace sly4::hevent6d;

// void Subtitle_Tick4A8B08(Subtitle* s) (name?): when the voice sound (+144)
// has finished, the line (+136) gets +224 = max(time - start, 0.5) / rows
// (+212) and the subtitle +140 = rows * that + 0x010C1D4C (or the time with
// no rows); the sound is released and +144 cleared. Then the line is
// scrolled: its position (vfunc +24) with y = -(int(+220) - visible rows) *
// row offset (0x004E20A8) when more rows than visible, else 0, set back by
// vfunc +28.
DECOMP(004A8B08, Subtitle_Tick4A8B08) {
    const u32 s = arg<u32>(ctx, 0);
    if (load<u32>(s + 144) != 0 && call(ctx, dfn::Sound_IsDone4431A0, {load<u32>(s + 144)}) != 0) {
        const f64 time = callf(ctx, dfn::Sound_Time444260, {load<u32>(s + 144)}, {});
        const u32 line = load<u32>(s + 136);
        const u32 rows = load<u32>(line + 212);
        const f64 t = static_cast<f32>(time - static_cast<f64>(load<f32>(kSubStart)));
        const f64 d = static_cast<f32>(static_cast<f64>(load<f32>(kSubHalf)) - t);
        f64 v = static_cast<f32>(d >= 0.0 ? load<f64>(kSubHalfD) : t);  // fsel
        const f64 c = static_cast<f32>(static_cast<f64>(rows));
        const f32 ratio = static_cast<f32>(v / c);
        store<f32>(line + 224, ratio);
        const u32 snd = load<u32>(s + 144);
        if (rows != 0) v = static_cast<f32>(std::fma(c, static_cast<f64>(ratio), static_cast<f64>(load<f32>(kSubBase))));
        store<f32>(s + 140, static_cast<f32>(v));
        call(ctx, dfn::Audio_Release446384, {kAudioMgr, snd});
        store<u32>(s + 144, 0);
    }
    const u32 line = load<u32>(s + 136);
    if (line == 0) {
        ret(ctx, 0u);  // r3 as the original leaves it
        return;
    }
    const u64 row = rt::fctid(static_cast<f64>(load<f32>(line + 220)), true);
    ctx.fpr[1] = std::bit_cast<f64>(row);  // f1 as the original leaves it for the call
    const f64 off = callf(ctx, dfn::Line_Scroll4E20A8, {line}, {});
    const u32 l2 = load<u32>(s + 136);
    f64 y = load<f32>(kSubZero);
    const u32 visible = load<u32>(kSubRows);
    const u32 opd = load<u32>(load<u32>(l2) + 24);
    const u32 r = static_cast<u32>(row);
    if (r > visible) {
        const f64 diff = static_cast<f32>(static_cast<f64>(r - visible));
        y = static_cast<f32>(-std::fma(diff, off, -y));  // fnmsubs
    }
    ctx.gpr[4] = load<u32>(opd);  // this call sequence leaves entry in r4, OPD in r5
    ctx.gpr[5] = opd;
    const u32 p = static_cast<u32>(call_ptr(ctx, opd, {l2}));
    Stack frame(ctx, 16);  // vec3 at r1+112
    store<u32>(frame.addr(), load<u32>(p));
    store<u32>(frame.addr() + 4, load<u32>(p + 4));
    store<f32>(frame.addr() + 4, static_cast<f32>(y));
    store<u32>(frame.addr() + 8, load<u32>(p + 8));
    const u32 l3 = load<u32>(s + 136);
    ctx.fpr[1] = load<f32>(p + 8);
    vcall(ctx, l3, 28, {l3, frame.addr()});
}

// void Objects_Reset35B470(Highlight* h) (name?) (h = 0x0106C330): with a
// target (+104) whose colour / offset differ from the defaults (+80 vec3 not
// zero), restores the target's renderer (+328 +12, when its desc +8 +152 is
// set): colour +64 by value (vfunc +56), then offset +80 (vfunc +40). Then
// clears +0..63, colour = white, +80 = 0 vec3, +96 / +108..+128 / +132, and
// notifies +100 (vfunc +12).
DECOMP(0035B470, Objects_Reset35B470) {
    const u32 h = arg<u32>(ctx, 0);
    u32 tgt = load<u32>(h + 104);
    const u32 off = h + 80;
    if (tgt != 0 && (load<u64>(off) != load<u64>(kZeroVec) || load<u32>(off + 8) != load<u32>(kZeroVec + 8))) {
        if (load<u32>(load<u32>(tgt + 8) + 152) != 0) {
            const u32 q = load<u32>(load<u32>(tgt + 328) + 12);
            if (q != 0) {
                const u32 opd = load<u32>(load<u32>(q) + 56);
                const u32 b0 = load<u8>(h + 64), b1 = load<u8>(h + 65), b2 = load<u8>(h + 66), b3 = load<u8>(h + 67);
                ctx.gpr[5] = opd;  // r5 / r6 / r7 as the original leaves them
                ctx.gpr[6] = b3;
                ctx.gpr[7] = b2;
                call_ptr(ctx, opd, {q, (b0 << 24) | (b1 << 16) | (b2 << 8) | b3});
                tgt = load<u32>(h + 104);
            }
        }
        if (load<u32>(load<u32>(tgt + 8) + 152) != 0) {
            const u32 q = load<u32>(load<u32>(tgt + 328) + 12);
            if (q != 0) vcall(ctx, q, 40, {q, off});
        }
    }
    for (u32 k = 0; k < 4; ++k) {  // stvx: 16-byte aligned stores
        const u32 a = (h + 16 * k) & ~15u;
        store<u64>(a, 0);
        store<u64>(a + 8, 0);
    }
    for (u32 k = 0; k < 4; ++k) store<u8>(h + 64 + k, load<u8>(kWhite + k));
    store<u32>(h + 80, load<u32>(kZeroVec));
    store<u32>(h + 96, 0);
    store<u32>(h + 84, load<u32>(kZeroVec + 4));
    store<u32>(h + 108, 0);
    store<u32>(h + 88, load<u32>(kZeroVec + 8));
    for (u32 o : {112u, 116u, 120u, 124u, 128u}) store<u32>(h + o, 0);
    store<u8>(h + 132, 0);
    ctx.fpr[1] = load<f32>(kZeroVec + 8);  // f1 as the original leaves it
    ctx.gpr[3] = kZeroVec;                  // r3 likewise
    const u32 n = load<u32>(h + 100);
    if (n != 0) vcall(ctx, n, 12, {n});
}
