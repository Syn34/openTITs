// Game code, large-function sweep part 22: the cooldown-gated animation pick
// of f_0041DEC0 / f_0041E6A8's object (list +104, animator source +468).
#include "big22.h"

using namespace sly4;
using namespace sly4::big22;

namespace {
constexpr u32 kWorld = 0x00F65CA0u;      // world state; u8 +4208 +1 the player index
constexpr u32 kGestureK = 0x0041D148u;   // f32 0.0, 1.0, 0.1
constexpr u32 kPlayerRecs = 0x010B19E0u; // 11968-byte per-player records; +176 +8 / +12 flag words
constexpr u32 kCooldown = 0x3E99999Au;   // f32 0.3

namespace gfn {
constexpr u32 Anim_TrackSet36 = 0x0029879Cu;  // (an, 0, 0) -> bool
constexpr u32 Anim_Find = 0x00298874u;        // (an, 0) -> track or 0
constexpr u32 Item_Matches = 0x0041B200u;     // f_0041B200(item, u32 mode) -> bool
}  // namespace gfn

// The animator: the +468 object's vfn[15].
u32 Animator(PPUContext& ctx, u32 o) {
    const u32 s = load<u32>(o + 468);
    return static_cast<u32>(VCall(ctx, s, 60, {s}));
}

// Anim_Restart2775A8(an, key, 0, 0, -1, r8 as the animator call left it,
// 0, 0, the frame's sp+112 / sp+120 (never written), 0, -1; f32 f1..f4).
void Restart(PPUContext& ctx, u32 o, u32 sp, u32 key, bool alt) {
    const u32 an = Animator(ctx, o);
    const u64 r8 = ctx.gpr[8];
    constexpr u32 K = kGestureK;
    const f64 a = Lf(K + 0), f1 = Lf(K + 4), f2 = alt ? Lf(K + 8) : Lf(K + 0);
    const u64 s9 = load<u64>(sp + 0), s10 = load<u64>(sp + 8);
    callf(ctx, big21::b21fn::Anim_Restart, {an, key, 0, 0, ~0ull, r8, 0, 0, s9, s10, 0, ~0ull}, {f1, f2, a, a});
}
}  // namespace

// u32 Obj_GesturePick41D158(Obj* o; f32 dt): while the cooldown u8 +484 is
// set, counts f32 +488 down by dt and clears both at <= 0.0 (r3 = 0, or the
// constant table's address 0x0041D148 when still running; f1 = the new
// time). Otherwise, with an animator (+468 vfn[15]) that has track 0
// (Anim_Find) or accepts Anim_TrackSet36(0, 0): the local player's record
// (0x010B19E0 + 11968 * u8 world+4209, flag words +184 / +188) picks the
// first mode whose bit is set in both words: 0 (0x10), 1 (0x20), 2 (0x80),
// 3 (0x40); none returns (r3 = the +188 word, or 0 when only bit 6 is
// missing there). Every list item (+104, u16 count +112) matching the mode
// (f_0041B200) marks a match; one with u8 +164 set marks "ready". Ready
// restarts the mode's clip 0x74799DD2 / 0x154F4C1D / 0xE106CE20 /
// 0xFA48404A with f32 (1.0, 0.0, 0.0, 0.0) and starts the 0.3 cooldown (r3
// = its bits); otherwise the clip 0x66DA775B / 0x42C053B0 / 0x7ADF054D /
// 0x83F1A5E3 with (1.0, 0.1, 0.0, 0.0) (r3 = Anim_Restart's result).
DECOMP(0041D158, Obj_GesturePick41D158) {
    const u32 o = arg<u32>(ctx, 0);
    const f64 dt = farg(ctx, 0);
    constexpr u32 K = kGestureK;
    if (load<u8>(o + 484) != 0) {
        const f64 t = S(Lf(o + 488) - dt);
        Sf(o + 488, t);
        ctx.fpr[1] = t;
        if (t < Lf(K + 0) || t == Lf(K + 0)) {
            store<u8>(o + 484, 0);
            store<u32>(o + 488, 0);
            ret(ctx, 0u);
        } else {
            ret(ctx, K);  // r3 left holding the constant table's address
        }
        return;
    }
    Stack frame(ctx, 16);
    const u32 sp = frame.addr();  // stands for the unwritten argument slots sp+112 / sp+120
    if (static_cast<u32>(call(ctx, gfn::Anim_Find, {Animator(ctx, o), 0})) == 0) {
        if (static_cast<u32>(call(ctx, gfn::Anim_TrackSet36, {Animator(ctx, o), 0, 0})) == 0) {
            ret(ctx, 0u);
            return;
        }
    }
    const u32 rec = kPlayerRecs + load<u8>(load<u32>(kWorld) + 4208 + 1) * 11968u + 176;
    const u32 fa = load<u32>(rec + 8), fb = load<u32>(rec + 12);
    u32 mode;
    if ((fa & 0x10u) != 0 && (fb & 0x10u) != 0) {
        mode = 0;
    } else if ((fa & 0x20u) != 0 && (fb & 0x20u) != 0) {
        mode = 1;
    } else if ((fa & 0x80u) != 0 && (fb & 0x80u) != 0) {
        mode = 2;
    } else if ((fa & 0x40u) == 0) {
        ret(ctx, fb);  // r3 = the second flag word
        return;
    } else if ((fb & 0x40u) == 0) {
        ret(ctx, 0u);
        return;
    } else {
        mode = 3;
    }
    bool matched = false, ready = false;
    for (u32 i = 0; i < load<u16>(o + 112); ++i) {
        if (static_cast<u32>(call(ctx, gfn::Item_Matches, {load<u32>(load<u32>(o + 104) + 4 * i), mode})) != 0) {
            matched = true;
            if (load<u8>(load<u32>(load<u32>(o + 104) + 4 * i) + 164) != 0) ready = true;
        }
    }
    static constexpr u32 kReady[4] = {0x74799DD2u, 0x154F4C1Du, 0xE106CE20u, 0xFA48404Au};
    static constexpr u32 kIdle[4] = {0x66DA775Bu, 0x42C053B0u, 0x7ADF054Du, 0x83F1A5E3u};
    Restart(ctx, o, sp, ready ? kReady[mode] : kIdle[mode], !ready);
    if (ready && matched) {
        store<u8>(o + 484, 1);
        store<u32>(o + 488, kCooldown);
        ret(ctx, kCooldown);
    }
}
