// Game code, large-function sweep part 25: one step of a reward sequence
// (stages of (giver, argument) pairs; each giver hands out an item / coin
// amount that is spawned as pickups).
#include "../sly4.h"
#include "big25.h"

using namespace sly4;
using namespace sly4::big25;

namespace {
constexpr u32 kCReward = 0x002682D8;                 // .text f32 table: +0 1.0, +4 0.0, +32 1e-5
constexpr u32 kTargetGetDef = 0x000D951C;      // Target_GetDef0D951C (Object*) -> def or 0
constexpr u32 kSpawnAt = 0x000F7E74;           // (const SpawnFrame*, const RewardInfo*, u32 kind, PtrVector* out)
constexpr u32 kApplyKind2 = 0x000F8058;        // Obj_ApplyKind2_000F8058 (SpawnFrame*, RewardInfo*, u32 kind, const Vec3* dir, Obj* at, PtrVector* out)
constexpr u32 kReturnK = 0x000F815C;           // ReturnK_000F815C () -> 0x99B7AAF2
constexpr u32 kStageDone = 0x0026621C;         // (Stage*, Obj* owner, x) (reads r5)
constexpr u32 kSetStateFx = 0x00266E6C;        // Obj_SetStateFx266E6C (Object*, s32 state)
constexpr u32 kCollectMarkFound = 0x00268158;  // Collect_MarkFound_00268158 (?, u32 hash)
constexpr u32 kSaveSlotBits2Is1 = 0x00465E60;  // SaveSlot_Bits2Is1_00465E60 (const u8* slot, u32 i) -> bool
constexpr u32 kProfileCached = 0x004AE3D0;     // Profile_Cached_F65F78 () -> profile or 0
constexpr u32 kIsLocalPlayerSlot = 0x004AEBE0; // Obj_IsLocalPlayerSlot_004AEBE0 (Obj*) -> bool
constexpr u32 kGlobalGet10C2478 = 0x004AFCA4;  // Global_Get10C2478 () -> f1 (the reward multiplier)
constexpr u32 kNop = 0x0054EC18;               // Nop (vector destructor body)
constexpr u32 kPtrVectorClear = 0x0054EC1C;    // PtrVector_Clear (vec)
constexpr u32 kPtrVectorConstruct = 0x005516E0;  // PtrVector_Construct (vec)
constexpr u32 kMatrixNormalize = 0x0056D374;   // Matrix_NormalizeInPlace (Mat34*)
constexpr u32 kVec4_01164C90 = 0x01164C90;     // a global f32[4] the spawn frame takes (identity quaternion?)
constexpr u32 kSaveMul = 0x0105BF80;           // f32: amount multiplier with the save-slot bit set
constexpr u32 kSpawnLife = 0x0105C160;         // f32: copied into each spawned pickup's +588
constexpr u32 kKindDrop = 0x693B0159u;         // the kind Obj_ApplyKind2 spawns towards the player

f64 C(u32 off) { return Lf(kCReward + off); }

// v += (u32 of the s64 trunc of) max(1, f32(v) * k): the u32 goes through
// the stack slot sp+496 (std, then the fctidz result stfd'ed there).
u32 ScaleAdd(u32 slot, u32 v, f64 k, f64 one, u64* bits_out = nullptr) {
    store<u64>(slot, v);
    const f64 prod = S(S(static_cast<f64>(v)) * k);
    const u64 bits = rt::fctid(big23::Fsel(Subs(one, prod), one, prod), true);
    store<u64>(slot, bits);
    if (bits_out) *bits_out = bits;
    return v + static_cast<u32>(bits);
}

// The spawn frame at f (+0 the 4x4 matrix m's 12 x/y/z words, +64 the zero
// vector, +80 the f32[4] at 0x01164C90, +96 / +112 the zero vector again).
void SpawnFrame(u32 f, u32 m) {
    for (u32 r = 0; r < 4; ++r)
        for (u32 k = 0; k < 3; ++k) Cw(f + 16 * r + 4 * k, m + 16 * r + 4 * k);
    const u32 z = b25fn::ZeroVec;
    Cw(f + 64, z);
    Cw(f + 68, z + 4);
    Cw(f + 72, z + 8);
    for (u32 k = 0; k < 4; ++k) Cw(f + 80 + 4 * k, kVec4_01164C90 + 4 * k);
    for (u32 k = 0; k < 3; ++k) Cw(f + 96 + 4 * k, z + 4 * k);
    for (u32 k = 0; k < 3; ++k) Cw(f + 112 + 4 * k, z + 4 * k);
}
}  // namespace

// bool f(Seq* o, Obj* player, u8 check) (called by 0013E6F8, 00145F00).
// o: +8 the owner, +28 the stage pointers, +36 (u16) their count, +48 the
// current stage. Stage s: +8 per-step objects and +28 per-step (giver,
// argument) pairs (8 bytes each), +36 (u16) the step count, +48 its def,
// +52 its object, +56 the current step, +60 bit 0x80 done.
// A done stage returns 0; with `check`, a giver whose vfn[8]() is set (still
// busy) returns 0. The step's object gets vfn[88](0, 1, 1, 1). With a giver:
// the spawn frame is the step object's (or the owner's) matrix +96,
// normalized; giver->vfn[12](&hash, 0, argument) gives the reward hash, the
// amounts are giver +52 / +56. With an amount > 1 and the giver not busy:
// for the local player both get scaled by Global_Get10C2478 (and the hash
// becomes the profile's +52); otherwise an owner with bit 8 of +192 and
// byte +709 = 0x80 class gets (amount + 1, amount * 2), and a player whose
// save-slot bit is set gets both scaled by 0x0105BF80 (each scaled amount
// added as max(1, x * k) truncated). With a hash: a not-busy giver marks it
// found (Collect_MarkFound); the pickups are spawned towards the player
// (Obj_ApplyKind2 kind 0x693B0159, direction player - owner flattened and
// normalized unless already ~(0, 0, 0)) or at the frame (kind 0x99B7AAF2);
// each spawned pickup with bit 0x40 of +192 gets +588 = 0x0105C160, bit
// 0x40 of +432 (life > 0) and bit 0x08000000 of +592; the result is 1 when
// the giver was not busy, something spawned and some pickup's +216 has
// +436. Then the step advances: the next step's object gets vfn[88](1, 1,
// 1, 1); past the last the stage is marked done, the sequence advances,
// the stage def's completion (0x0026621C, its object vfn[40](1) first when
// flagged 0x40000) runs when the stage object's +216 +52 matches the def's
// +44; past the last stage the owner's def gets state 4.
DECOMP(00268300, RewardSeq_Step_00268300) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 player = arg<u32>(ctx, 1);
    const u32 check = arg<u32>(ctx, 2) & 0xFF;
    Stack frame(ctx, 624);
    const u32 sp = frame.addr();
    const u32 dir = sp + 112, mat = sp + 128, sf1 = sp + 192, sf2 = sp + 320;
    const u32 hash = sp + 448, info = sp + 452, vec = sp + 472, tmp = sp + 496;

    u32 stage = load<u32>(load<u32>(o + 28) + load<u32>(o + 48) * 4);
    if (load<u8>(stage + 60) & 0x80) {
        ctx.gpr[3] = 0;
        return;
    }
    const u32 pair = load<u32>(stage + 28) + load<u32>(stage + 56) * 8;
    u32 owner = load<u32>(o + 8);
    if (load<u32>(pair) != 0 && check != 0) {
        const u32 g = load<u32>(pair);
        if (static_cast<u32>(VCall(ctx, g, 8, {g})) != 0) {
            ctx.gpr[3] = 0;
            return;
        }
    }
    const u32 slot = load<u32>(stage + 8) + load<u32>(stage + 56) * 8;
    if (const u32 s = load<u32>(slot)) VCall(ctx, s, 88, {s, 0, 1, 1, 1});

    u32 result = 0, r20 = 0;
    if (load<u32>(pair) != 0) {
        const u32 so = load<u32>(slot);
        const u32 src = (so != 0 ? so : owner) + 96;
        for (u32 r = 0; r < 4; ++r)
            for (u32 k = 0; k < 3; ++k) Cw(mat + 16 * r + 4 * k, src + 16 * r + 4 * k);
        call(ctx, kMatrixNormalize, {mat});
        SpawnFrame(sf1, mat);
        const u32 giver = load<u32>(pair);
        u32 amt1 = load<u32>(giver + 52), amt2 = load<u32>(giver + 56);
        store<u32>(hash, 0);
        VCall(ctx, giver, 12, {giver, hash, 0, load<u32>(pair + 4)});
        const u32 local = static_cast<u32>(call(ctx, kIsLocalPlayerSlot, {player}));
        if (amt2 > 1) {
            const u32 g = load<u32>(pair);
            if (static_cast<u32>(VCall(ctx, g, 8, {g})) == 0) {
                const f64 one = C(0);
                u64 bits = 0;
                if (local != 0) {
                    const u32 prof = static_cast<u32>(call(ctx, kProfileCached, {}));
                    if (prof != 0) {
                        store<u32>(hash, load<u32>(prof + 52));
                        ctx.fpr[1] = one;
                        call(ctx, kGlobalGet10C2478, {});
                        amt1 = ScaleAdd(tmp, amt1, ctx.fpr[1], one, &bits);
                        ctx.fpr[1] = std::bit_cast<f64>(bits);
                        call(ctx, kGlobalGet10C2478, {});
                        amt2 = ScaleAdd(tmp, amt2, ctx.fpr[1], one, &bits);
                        ctx.fpr[1] = std::bit_cast<f64>(bits);  // as the original leaves f1
                    }
                } else {
                    if (owner != 0 && (load<u32>(owner + 192) & 8) != 0) {
                        const u32 b = load<u8>(owner + 656 + 53);
                        if ((b & 0x80) != 0 && (b & 0x40) == 0) {
                            amt1 = amt2 + 1;
                            amt2 <<= 1;
                        }
                    }
                    if (player != 0 && (load<u64>(player + 184) & 1) != 0) {
                        const u32 sv = load<u32>(b25fn::Game) + static_cast<u32>(static_cast<s32>(static_cast<s8>(load<u8>(player + 172))) * 0x20C80) + 4208;
                        if (static_cast<u32>(call(ctx, kSaveSlotBits2Is1, {sv, 1})) != 0) {
                            const f64 k = Lf(kSaveMul);
                            amt1 = ScaleAdd(tmp, amt1, k, one);
                            amt2 = ScaleAdd(tmp, amt2, k, one, &bits);
                            ctx.fpr[1] = std::bit_cast<f64>(bits);  // as the original leaves f1
                        }
                    }
                }
            }
        }

        const u32 h = load<u32>(hash);
        if (h != 0) {
            store<u32>(info, result);
            store<u32>(info + 4, h);
            store<u32>(info + 8, amt1);
            store<u32>(info + 12, amt2);
            store<u32>(info + 16, result);
            const u32 g = load<u32>(pair);
            const u32 idle = static_cast<u32>(VCall(ctx, g, 8, {g}));
            if (idle != 0) call(ctx, kCollectMarkFound, {o, load<u32>(hash)});
            call(ctx, kPtrVectorConstruct, {vec});
            store<u16>(vec + 6, 4);
            if (player != 0) {
                const u32 pa = player + 144, pb = owner + 144;
                const f64 dx = Subs(Lf(pa), Lf(pb)), dz = Subs(Lf(pa + 8), Lf(pb + 8));
                Sf(dir, dx);
                Sf(dir + 8, dz);
                store<u32>(dir + 4, result);
                const f64 dy = C(4), eps = C(32);
                const u32 z = b25fn::ZeroVec;
                if (!(std::fabs(Subs(dx, Lf(z))) < eps && std::fabs(Subs(dy, Lf(z + 4))) < eps && std::fabs(Subs(dz, Lf(z + 8))) < eps)) {
                    const f64 inv = Divs(C(0), Sqrts(Fmadds(dz, dz, Fmadds(dx, dx, S(dy * dy)))));
                    Sf(dir, S(dx * inv));
                    Sf(dir + 4, S(dy * inv));
                    Sf(dir + 8, S(dz * inv));
                }
                call(ctx, kApplyKind2, {sf1, info, kKindDrop, dir, owner, vec});
            } else {
                SpawnFrame(sf2, mat);
                const u32 kind = static_cast<u32>(call(ctx, kReturnK, {}));
                call(ctx, kSpawnAt, {sf2, info, kind, vec});
            }
            u32 cnt = load<u16>(vec + 8);
            if (cnt != 0) result = 1;
            result = idle & result;
            const f64 zero = C(4);
            ctx.fpr[1] = zero;  // as the original leaves f1
            if (cnt != 0) {
                for (u32 i = 0, off = 0; i < cnt; ++i, off += 4) {
                    const u32 e = load<u32>(load<u32>(vec) + off);
                    if (e == 0 || (load<u32>(e + 192) & 0x40) == 0) continue;
                    const u32 lw = load<u32>(kSpawnLife);
                    store<u32>(e + 588, lw);
                    const u32 bit = Lf(kSpawnLife) > zero ? 1 : r20;
                    const u32 hw = (load<u16>(e + 432) & ~0x40u) | ((bit << 6) & 0x40u);
                    const u32 w = load<u32>(e + 592) | 0x08000000u;
                    store<u16>(e + 432, static_cast<u16>(hw));
                    store<u32>(e + 592, w);
                    const u32 r7 = load<u32>(load<u32>(e + 216) + 436);
                    cnt = load<u16>(vec + 8);
                    const u32 r6 = (r7 != 0 ? 1 : r20) | result;
                    result = r6 != 0 ? 1 : r20;
                }
            }
            call(ctx, kPtrVectorClear, {vec});
            call(ctx, kNop, {vec});
        }
    }

    // Advance the step.
    const u32 n = load<u32>(stage + 56) + 1;
    const u32 steps = load<u16>(stage + 36);
    store<u32>(stage + 56, n);
    if (n < steps) {
        stage = load<u32>(load<u32>(stage + 8) + n * 8);
        if (stage != 0) VCall(ctx, stage, 88, {stage, 1, 1, 1, 1});
        ctx.gpr[3] = result;
        return;
    }
    store<u8>(stage + 60, static_cast<u8>(load<u8>(stage + 60) | 0x80));
    store<u32>(o + 48, load<u32>(o + 48) + 1);
    if (const u32 c = load<u32>(stage + 52)) {
        u32 def = load<u32>(stage + 48);
        const u32 want = load<u32>(def + 44);
        if (want != 0) {
            if (const u32 r6 = load<u32>(c + 216)) r20 = load<u32>(r6 + 52);
            if (r20 == want && (def == 0 || load<u32>(def + 36) == 0 || load<u8>(load<u32>(def + 36) + 40) == 0)) {
                if (load<u32>(c + 172) & 0x40000u) {
                    VCall(ctx, c, 40, {c, 1});
                    def = load<u32>(stage + 48);
                }
                const u32 x = load<u32>(def + 40);
                if (x != 0) call(ctx, kStageDone, {stage, owner, x});  // the callee reads r5
            }
        }
    }
    if (load<u32>(o + 48) >= load<u16>(o + 36)) {
        owner = static_cast<u32>(call(ctx, kTargetGetDef, {owner}));
        if (owner != 0) call(ctx, kSetStateFx, {owner, 4});
    }
    ctx.gpr[3] = result;
}
