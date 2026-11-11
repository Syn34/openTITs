// Game code, large-function sweep part 6: a character's animation state
// switch (003D8C10).
#include "big6.h"

using namespace sly4;
using namespace sly4::big6;

namespace {
// Character.
constexpr u32 kChAnimOwner = 68;   // vfn[60]: (owner) -> Animator*
constexpr u32 kChTarget = 80;      // {+208 -> {+120 f32 length}} or 0
constexpr u32 kChState = 96;       // s32
constexpr u32 kChProgress = 108;   // f32, clamped to 1
constexpr u32 kChProgress2 = 116;  // f32, advanced with +108
constexpr u32 kChSpeed = 120;      // f32
constexpr u32 kChCount = 148;      // s32
constexpr u32 kChFlags = 152;      // bit 0: variant of state 6
constexpr u32 kChVariant = 168;    // s32: 0..3, indexes the per-state hash tables
constexpr u32 kChBusy = 175;       // u8

constexpr u32 kC = 0x003D8BE8u;  // .text: f32 1, 0.25, 0, 0.1; f64 0.1f; f32 1/65535; f64 1
constexpr u32 kRate = 0x01088EC8u;      // f32 tuning: animation rate
constexpr u32 kSpeedMin = 0x010894B8u;  // f32 tuning
constexpr u32 kSpeedMax = 0x01089508u;  // f32 tuning
constexpr u32 kStep = 0x01089058u;      // f32 tuning: progress per target length
constexpr u32 kRandLast = 0x01164888u;  // u64: the last 16-bit random value
constexpr u32 kRandSeed = 0x01164890u;  // u32

constexpr u32 kAnimRestart = 0x002775A8u;  // (Animator*, key, 0, 0, 0, -1, 0, 0, -, -, 0, -1; f32 x4) -> Track*
constexpr u32 kAnimRate = 0x00276E4Cu;     // (Animator*, -, 0, f32 rate)
constexpr u32 kVfnAnimator = 60;

f32 F(u32 a) { return load<f32>(a); }
u32 Rotl(u32 x, u32 n) { return (x << n) | (x >> (32 - n)); }

// Restart animation `key` on animator `an` (weights c[0] / c[1] / c[2] /
// c[2] of the constants at cb); when the tuning rate is not c[0] it is
// applied, clamped to >= c[3] (the rate call's r4 is what the restart left
// there; the callee ignores it). Arguments 9 / 10 (sp+112 / +120) are never
// written by the original: their current words are passed back. f1
// afterwards: the rate, or what the rate call leaves.
void PlayOn(PPUContext& ctx, u32 sp, u32 cb, u32 an, u32 key, u64 r7 = 0, u64 r8 = ~0ull, u64 a11 = 0) {
    callf(ctx, kAnimRestart, {an, key, 0, 0, r7, r8, 0, 0, load<u64>(sp + 112), load<u64>(sp + 120), a11, ~0ull},
          {F(cb), F(cb + 4), F(cb + 8), F(cb + 8)});
    const u32 rw = load<u32>(kRate);
    ctx.fpr[1] = std::bit_cast<f64>(LfsBits(rw));
    const f32 rate = std::bit_cast<f32>(rw);
    if (static_cast<f64>(rate) != static_cast<f64>(F(cb))) {
        const f32 d = rate - F(cb + 12);
        const f32 r = static_cast<f32>(static_cast<f64>(d) >= 0.0 ? static_cast<f64>(rate) : load<f64>(cb + 16));
        callf(ctx, kAnimRate, {an, ctx.gpr[4], 0}, {r});
    }
}
void Play(PPUContext& ctx, u32 sp, u32 cb, u32 owner, u32 key, u64 r7 = 0, u64 r8 = ~0ull) {
    PlayOn(ctx, sp, cb, static_cast<u32>(VCall(ctx, owner, kVfnAnimator, {owner})), key, r7, r8);
}
// The table states load the OPD into r5 and its entry into r4 (the other
// way round from VCall) and leave table words / their own values in r6..:
// `regs` are those r6.. values.
void PlayT(PPUContext& ctx, u32 sp, u32 cb, u32 owner, u32 key, std::initializer_list<u64> regs) {
    const u32 opd = load<u32>(load<u32>(owner) + kVfnAnimator), ent = load<u32>(opd);
    u64 a[8] = {owner, ent, opd, 0, 0, 0, 0, 0};
    u32 n = 3;
    for (u64 r : regs) a[n++] = r;
    u32 an;
    switch (n) {
    case 5: an = static_cast<u32>(call_ptr(ctx, opd, {a[0], a[1], a[2], a[3], a[4]})); break;
    case 6: an = static_cast<u32>(call_ptr(ctx, opd, {a[0], a[1], a[2], a[3], a[4], a[5]})); break;
    default: an = static_cast<u32>(call_ptr(ctx, opd, {a[0], a[1], a[2], a[3], a[4], a[5], a[6], a[7]})); break;
    }
    PlayOn(ctx, sp, cb, an, key);
}
// The per-state hash tables are built on the stack at the original's
// offsets: the variant index is not checked, so an out-of-range one reads
// the neighbouring frame words as the original does.
void Table(u32 at, u32 base) {
    store<u32>(at, base);
    store<u32>(at + 12, base + 3);
    store<u32>(at + 4, base + 1);
    store<u32>(at + 8, base + 2);
}
}  // namespace

// u32 Char_SetAnimState(Char* c, s32 s)
// Plays the animation of state s (0..8; others only record it):
// 0 0xB461C8E5, 1 a random C7C116DA+v walk (v advanced by 1..3 mod 4,
// speed = random between the min / max tuning, progress +108 / +116
// advanced by the step over the target's length, clamped to 1), 2
// 92E06C0E+v then count = 0, 3 0xA1A31C84 then count /= 2, 4 ACDA2FD8+v
// with count + 1, 5 0xD795C0C6 with count + 1, 6 0xA6C90455 (0x32ADE76E
// for flag bit 0) with count = 0, 7 0xB462C357, 8 0x9FDF3C9F. Then +96 = s,
// +175 = 0; returns 0.
DECOMP(003D8C10, Char_SetAnimState) {
    const u32 c = arg<u32>(ctx, 0);
    const s32 s = arg<s32>(ctx, 1);
    Stack frame(ctx, 240);
    const u32 sp = frame.addr();
    const u32 owner = load<u32>(c + kChAnimOwner);
    switch (s) {
    case 0:
        Play(ctx, sp, kC, owner, 0xB461C8E5u);
        break;
    case 1: {
        const u64 old = load<u64>(kRandLast);
        store<s32>(c + kChCount, load<s32>(c + kChCount) + 1);
        const u32 seed = load<u32>(kRandSeed);
        constexpr u64 kK = 0xFFFFFFFF87654321ull;
        const u64 b = (static_cast<u64>(Rotl(seed, 4)) + Rotl(seed, 16)) ^ kK;
        const u32 r8 = Rotl(static_cast<u32>(b), 16);
        const u32 q3 = r8 / 3;  // mulhwu by 0xAAAAAAAB, >> 1
        const u32 step = r8 - 3 * q3 + 1;
        const u64 d = (static_cast<u64>(Rotl(static_cast<u32>(b), 20)) + b) ^ kK;
        store<u32>(kRandSeed, Rotl(static_cast<u32>(d), 16));
        const f32 mn = F(kSpeedMin);
        const f32 range = F(kSpeedMax) - mn;
        const f32 rnd = static_cast<f32>(static_cast<f64>(static_cast<s64>(old))) * F(kC + 24);
        const s32 v = static_cast<s32>(load<u32>(c + kChVariant) + step) % 4;
        const u32 tgt = load<u32>(c + kChTarget);
        store<u64>(kRandLast, d & 0xFFFF);
        store<s32>(c + kChVariant, v);
        store<f32>(c + kChSpeed, std::fma(range, rnd, mn));
        if (tgt != 0) {
            const f32 inc = static_cast<f32>(static_cast<f64>(F(kStep)) / static_cast<f64>(F(load<u32>(tgt + 208) + 120)));
            const f32 p = F(c + kChProgress), p2 = F(c + kChProgress2);
            f32 t = p + inc;
            const f32 room = F(kC) - t;
            t = static_cast<f32>(static_cast<f64>(room) >= 0.0 ? static_cast<f64>(t) : load<f64>(kC + 32));
            const f32 dt = t - p;
            store<f32>(c + kChProgress, p + dt);
            store<f32>(c + kChProgress2, p2 + dt);
        }
        const u32 tab = sp + 184;
        Table(tab, 0xC7C116DAu);
        // r8 / r9 / r10 at the call: the random word, the new seed, 2 * (r8 / 3).
        PlayT(ctx, sp, kC, owner, load<u32>(tab + (static_cast<u32>(v) << 2)),
              {Sx(0xC7C116DBu), Sx(0xC7C116DCu), r8, Rotl(static_cast<u32>(d), 16), 2ull * q3});
        break;
    }
    case 2: {
        const u32 tab = sp + 168;
        Table(tab, 0x92E06C0Eu);
        PlayT(ctx, sp, kC, owner, load<u32>(tab + (load<u32>(c + kChVariant) << 2)), {Sx(0x92E06C11u), Sx(0x92E00000u)});
        store<u32>(c + kChCount, 0);
        break;
    }
    case 3:
        Play(ctx, sp, kC, owner, 0xA1A31C84u);
        store<s32>(c + kChCount, load<s32>(c + kChCount) / 2);
        break;
    case 4: {
        const u32 tab = sp + 152;
        Table(tab, 0xACDA2FD8u);
        const u64 cnt = static_cast<u64>(load<u32>(c + kChCount)) + 1;  // r7 (64-bit)
        store<u32>(c + kChCount, static_cast<u32>(cnt));
        PlayT(ctx, sp, kC, owner, load<u32>(tab + (load<u32>(c + kChVariant) << 2)), {Sx(0xACDA2FDBu), cnt, Sx(0xACDA2FDAu)});
        break;
    }
    case 5:
        store<s32>(c + kChCount, load<s32>(c + kChCount) + 1);
        Play(ctx, sp, kC, owner, 0xD795C0C6u);
        break;
    case 6:
        store<u32>(c + kChCount, 0);
        Play(ctx, sp, kC, owner, (load<u32>(c + kChFlags) & 1) != 0 ? 0x32ADE76Eu : 0xA6C90455u, ~0ull, 0);  // r7 / r8 swapped
        break;
    case 7:
        Play(ctx, sp, kC, owner, 0xB462C357u);
        break;
    case 8:
        Play(ctx, sp, kC, owner, 0x9FDF3C9Fu);
        break;
    default:
        break;
    }
    store<s32>(c + kChState, s);
    store<u8>(c + kChBusy, 0);
    ctx.gpr[3] = 0;
}

namespace {
// 003D95E0: the second character's fields (as 003D8C10's, shifted).
constexpr u32 kC2 = 0x003D95B8u;  // .text: f32 1, 0.25, 0, 0.1; f64 0.1f; f32 0.75; f64 0
constexpr u32 kCh2Owner = 64;      // vfn[60]: (owner) -> Animator*
constexpr u32 kCh2Sound = 72;      // vfn[200]: (o, key, 0, 0, -, -, 0; f32 x3) (name?)
constexpr u32 kCh2State = 92;
constexpr u32 kCh2TrackTime = 124; // f32
constexpr u32 kCh2Zero128 = 128, kCh2Zero132 = 132, kCh2Zero144 = 144;
constexpr u32 kCh2IdleCount = 152; // s32; bit 0 also picks state 2's variant
constexpr u32 kCh2Count4 = 156;    // s32
constexpr u32 kCh2Pick = 160;      // u32: random in [min, max]
constexpr u32 kCh2Flag172 = 172, kCh2Flag173 = 173;
constexpr u32 kIdleThreshold = 0x01089648u;  // s32 tuning
constexpr u32 kIdleEnable = 0x01089698u;     // u8 tuning
constexpr u32 kPickMin = 0x01089378u, kPickMax = 0x010893C8u;  // u32 tuning
constexpr u32 kStep2 = 0x01089008u;          // f32 tuning
constexpr u32 kVfnSound = 200;
constexpr u32 kSetTrackTime = 0x00276A20u;   // (Animator*, -, 0, 0; f32 t)

// +116 -= step over the target's length, clamped to >= 0; +108 moves by the same amount.
void Regress(u32 c, u32 tgt) {
    const f32 inc = static_cast<f32>(static_cast<f64>(F(kStep2)) / static_cast<f64>(F(load<u32>(tgt + 208) + 120)));
    const f32 p2 = F(c + kChProgress2), p = F(c + kChProgress);
    f32 t = p2 - inc;
    t = static_cast<f32>(static_cast<f64>(t) >= 0.0 ? static_cast<f64>(t) : load<f64>(kC2 + 32));
    const f32 d = t - p2;
    store<f32>(c + kChProgress2, p2 + d);
    store<f32>(c + kChProgress, p + d);
}
}  // namespace

// u32 Char2_SetAnimState(Char2* c, s32 s)
// As Char_SetAnimState for the second character: 0 0xB461C8E5; 1 idle:
// count +152 up to the threshold (then reset, and with +108 < 0.75 +160 =
// random in [min, max]), variant +168 advanced by 1..3 mod 4, +116 / +108
// regressed, C7C116DA+v; 2 0x32ADE76E / 0xA6C90455 by bit 0 of +152; 3
// regress then 0xA1A31C84; 4 +156 + 1, +132 = 0, ACDA2FD8+v, then the
// track time +124 set on the (re-fetched) animator; 5 0xD795C0C6; 6 +132 =
// 0, 6EB7F0A3+v; 7 0x872BE0D3; 8 0x9FDF3C9F then +72's vfn[200](0xB6947C85)
// and +128 = +144 = 0. Then +92 = s, +172 = +173 = 0; returns 0. States 4
// and 6 pass the restart an r7 / r8 of -1 / (the vcall's leftover) and
// never write its stack argument 11 (sp+128), whose current word is passed.
DECOMP(003D95E0, Char2_SetAnimState) {
    const u32 c = arg<u32>(ctx, 0);
    const s32 s = arg<s32>(ctx, 1);
    Stack frame(ctx, 272);
    const u32 sp = frame.addr();
    const u32 owner = load<u32>(c + kCh2Owner);
    constexpr u64 kK = 0xFFFFFFFF87654321ull;
    switch (s) {
    case 0:
        Play(ctx, sp, kC2, owner, 0xB461C8E5u);
        break;
    case 1: {
        const u32 cnt = load<u32>(c + kCh2IdleCount) + 1;
        store<u32>(c + kCh2IdleCount, cnt);
        if (static_cast<s32>(cnt) >= load<s32>(kIdleThreshold) && load<u8>(kIdleEnable) != 0) {
            store<u32>(c + kCh2IdleCount, 0);
            if (static_cast<f64>(F(c + kChProgress)) < static_cast<f64>(F(kC2 + 24))) {
                const u32 mn = load<u32>(kPickMin), range = load<u32>(kPickMax) - mn;
                const u32 seed = load<u32>(kRandSeed);
                const u32 n = range + 1;
                const u32 q = n != 0 ? seed / n : 0;  // divwu (the original's result for n == 0 is undefined)
                const u32 rem = seed - (q * range + q);
                const u64 b = (static_cast<u64>(Rotl(seed, 4)) + Rotl(seed, 16)) ^ kK;
                store<u32>(kRandSeed, Rotl(static_cast<u32>(b), 16));
                store<u32>(c + kCh2Pick, mn + rem);
                store<u64>(kRandLast, b & 0xFFFF);
            }
        }
        const u32 seed = load<u32>(kRandSeed);
        const u32 step = seed - 3 * (seed / 3) + 1;  // mulhwu by 0xAAAAAAAB
        const u64 b = (static_cast<u64>(Rotl(seed, 4)) + Rotl(seed, 16)) ^ kK;
        store<u32>(kRandSeed, Rotl(static_cast<u32>(b), 16));
        const s32 v = static_cast<s32>(load<u32>(c + kChVariant) + step) % 4;
        store<u64>(kRandLast, b & 0xFFFF);
        const u32 tgt = load<u32>(c + kChTarget);
        store<s32>(c + kChVariant, v);
        if (tgt != 0) Regress(c, tgt);
        const u32 tab = sp + 176;
        Table(tab, 0xC7C116DAu);
        Play(ctx, sp, kC2, owner, load<u32>(tab + (static_cast<u32>(v) << 2)));
        break;
    }
    case 2:
        Play(ctx, sp, kC2, owner, (load<u32>(c + kCh2IdleCount) & 1) != 0 ? 0x32ADE76Eu : 0xA6C90455u);
        break;
    case 3:
        if (const u32 tgt = load<u32>(c + kChTarget); tgt != 0) Regress(c, tgt);
        Play(ctx, sp, kC2, owner, 0xA1A31C84u);
        break;
    case 4: {
        store<u32>(c + kCh2Count4, load<u32>(c + kCh2Count4) + 1);
        store<u32>(c + kCh2Zero132, 0);
        const u32 tab = sp + 160;
        store<u32>(tab + 8, 0xACDA2FDAu);
        store<u32>(tab, 0xACDA2FD8u);
        store<u32>(tab + 4, 0xACDA2FD9u);
        store<u32>(tab + 12, 0xACDA2FDBu);
        const u32 key = load<u32>(tab + (load<u32>(c + kChVariant) << 2));
        const u32 an = static_cast<u32>(VCall(ctx, owner, kVfnAnimator, {owner}));
        PlayOn(ctx, sp, kC2, an, key, ~0ull, ctx.gpr[8], load<u64>(sp + 128));
        const u32 an2 = static_cast<u32>(VCall(ctx, owner, kVfnAnimator, {owner}));
        callf(ctx, kSetTrackTime, {an2, ctx.gpr[4], 0, 0}, {F(c + kCh2TrackTime)});
        break;
    }
    case 5:
        Play(ctx, sp, kC2, owner, 0xD795C0C6u);
        break;
    case 6: {
        const u32 tab = sp + 144;
        store<u32>(tab + 8, 0x6EB7F0A5u);
        store<u32>(c + kCh2Zero132, 0);
        store<u32>(tab, 0x6EB7F0A3u);
        store<u32>(tab + 4, 0x6EB7F0A4u);
        store<u32>(tab + 12, 0x6EB7F0A6u);
        const u32 key = load<u32>(tab + (load<u32>(c + kChVariant) << 2));
        const u32 an = static_cast<u32>(VCall(ctx, owner, kVfnAnimator, {owner}));
        PlayOn(ctx, sp, kC2, an, key, ~0ull, ctx.gpr[8], load<u64>(sp + 128));
        break;
    }
    case 7:
        Play(ctx, sp, kC2, owner, 0x872BE0D3u);
        break;
    case 8: {
        Play(ctx, sp, kC2, owner, 0x9FDF3C9Fu);
        const u32 snd = load<u32>(c + kCh2Sound);
        const u32 opd = load<u32>(load<u32>(snd) + kVfnSound), ent = load<u32>(opd);
        ctx.fpr[1] = static_cast<f64>(F(kC2));
        ctx.fpr[2] = static_cast<f64>(F(kC2 + 4));
        ctx.fpr[3] = static_cast<f64>(F(kC2 + 8));
        // r10: what the restart / rate call left (a float slot, unread).
        call_ptr(ctx, opd, {snd, 0xB6947C85u, 0, 0, opd, ent, 0, ctx.gpr[10]});
        store<u32>(c + kCh2Zero128, 0);
        store<u32>(c + kCh2Zero144, 0);
        break;
    }
    default:
        break;
    }
    store<s32>(c + kCh2State, s);
    store<u8>(c + kCh2Flag172, 0);
    store<u8>(c + kCh2Flag173, 0);
    ctx.gpr[3] = 0;
}
