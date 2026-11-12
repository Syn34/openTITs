// Game code, large-function sweep part 25: one step of a reward sequence
// (stages of (giver, argument) pairs; each giver hands out an item / coin
// amount that is spawned as pickups).
#include "big25.h"

namespace sly4::obj {

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

// The objects involved (owner, player, step objects, spawned pickups).
struct RwObj {
    pad<96> _0;
    Mtx44 m;            // +96 (row 3 = the position, +144)
    pad<12> _160;
    WordBytes w172;     // +172: flags (bit 0x40000); the player's s8 slot index is its top byte
    pad<8> _176;
    be_u64 d184;        // +184: the player's flags (bit 0: save slot)
    be_u32 flags;       // +192
    pad<20> _196;
    be_u32 w216;        // +216
    pad<212> _220;
    be_u16 h432;        // +432
    pad<154> _434;
    be_u32 life;        // +588
    be_u32 w592;        // +592
    pad<113> _596;
    u8 b709;            // +709
};
static_assert(offsetof(RwObj, w172) == 172 && offsetof(RwObj, flags) == 192 && offsetof(RwObj, w216) == 216);
static_assert(offsetof(RwObj, h432) == 432 && offsetof(RwObj, life) == 588 && offsetof(RwObj, b709) == 709);
struct RwGiver {
    pad<52> _0;
    be_u32 amt1;  // +52
    be_u32 amt2;  // +56
};
struct RwPair {
    be_u32 giver;  // +0: RwGiver* (vfn[8] busy, vfn[12] the reward hash)
    be_u32 arg;    // +4
};
struct RwDef {
    pad<36> _0;
    be_u32 w36;   // +36: its u8 +40 blocks the completion
    be_u32 x;     // +40: the completion's argument
    be_u32 want;  // +44
};
struct RwStage {
    pad<8> _0;
    be_u32 objs;    // +8: 8-byte entries, word 0 the step's object
    pad<16> _12;
    be_u32 pairs;   // +28: RwPair[]
    pad<4> _32;
    be_u16 steps;   // +36
    pad<10> _38;
    be_u32 def;     // +48: RwDef*
    be_u32 obj;     // +52: RwObj*
    be_u32 cur;     // +56
    u8 flags;       // +60: 0x80 done
};
static_assert(offsetof(RwStage, pairs) == 28 && offsetof(RwStage, def) == 48 && offsetof(RwStage, flags) == 60);
struct RwSeq {
    pad<8> _0;
    be_u32 owner;   // +8: RwObj*
    pad<16> _12;
    be_u32 stages;  // +28: RwStage*[]
    pad<4> _32;
    be_u16 count;   // +36
    pad<10> _38;
    be_u32 cur;     // +48
};
static_assert(offsetof(RwSeq, stages) == 28 && offsetof(RwSeq, cur) == 48);

// The spawn frame (128 bytes on the stack).
struct RwFrame {
    Mtx44 m;     // +0: the x / y / z words of the matrix
    Vec4 v64;    // +64: the zero vector
    Vec4 q80;    // +80: the f32[4] at 0x01164C90
    Vec4 v96;    // +96: the zero vector
    Vec4 v112;   // +112: the zero vector
};
static_assert(sizeof(RwFrame) == 128);
// The reward descriptor the spawners take.
struct RwInfo {
    be_u32 w0;    // +0
    be_u32 hash;  // +4
    be_u32 amt1;  // +8
    be_u32 amt2;  // +12
    be_u32 w16;   // +16
};
// The PtrVector the spawners fill: +0 data, +6 (u16) the element size,
// +8 (u16) the count.
struct RwVec {
    gp<be_u32> data;  // +0
    pad<2> _4;
    be_u16 esize;     // +6
    be_u16 count;     // +8
    pad<10> _10;
};
// The original's locals (sp offsets).
struct RwLocals {
    Vec4 dir;       // sp+112
    Mtx44 mat;      // sp+128
    RwFrame sf1;    // sp+192
    RwFrame sf2;    // sp+320
    be_u32 hash;    // sp+448
    RwInfo info;    // sp+452
    RwVec vec;      // sp+472
};
static_assert(offsetof(RwLocals, sf2) == 208 && offsetof(RwLocals, hash) == 336 && offsetof(RwLocals, vec) == 360);

f64 C(u32 off) { return lf(global<be_f32>(kCReward + off)); }
RwObj& Obj(u32 ea) { return gm<RwObj>(ea); }

// v + (u32 of the s64 trunc of) max(1, f32(v) * k).
u32 ScaleAdd(u32 v, f64 k, f64 one) {
    const f64 prod = S(S(static_cast<f64>(v)) * k);
    return v + static_cast<u32>(Fctidz(Fsel(Subs(one, prod), one, prod)));
}

// The spawn frame: the x / y / z words of the 4x4 matrix m, the zero
// vector at +64 / +96 / +112, the f32[4] at 0x01164C90 at +80.
void SpawnFrame(RwFrame& f, const Mtx44& m) {
    CopyMtxXYZ(f.m, m);
    const Vec3& z = global<Vec3>(kZeroVec);
    CopyXYZ(f.v64, z);
    const Vec4& q = global<Vec4>(kVec4_01164C90);
    CopyXYZ(f.q80, q);
    set_bits(f.q80.w, bits(q.w));
    CopyXYZ(f.v96, z);
    CopyXYZ(f.v112, z);
}

u32 Busy(u32 g) { return vcall<u32>(&gm<u8>(g), 8); }
}  // namespace

// bool f(Seq* o, Obj* player, u8 check) (called by 0013E6F8, 00145F00,
// which read r3). o: +8 the owner, +28 the stage pointers, +36 (u16) their
// count, +48 the current stage. Stage s: +8 per-step objects and +28
// per-step (giver, argument) pairs (8 bytes each), +36 (u16) the step
// count, +48 its def, +52 its object, +56 the current step, +60 bit 0x80
// done.
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
// 0x40 of +432 (life > 0) and bit 0x08000000 of +592. The result is bit 0
// of the giver's vfn[8]() when something spawned, or'ed with whether some
// flagged pickup's +216 has +436. Then the step advances: the next step's
// object gets vfn[88](1, 1, 1, 1); past the last the stage is marked done,
// the sequence advances, the stage def's completion (0x0026621C, its
// object vfn[40](1) first when flagged 0x40000) runs when the stage
// object's +216 +52 matches the def's +44; past the last stage the owner's
// def gets state 4.
bool RewardSeq_Step_00268300(RwSeq* o, RwObj* player, u8 check) {
    GuestFrame frame(624);
    RwLocals& L = *frame.as<RwLocals>(112);

    RwStage* stage = &gm<RwStage>(gm<be_u32>(o->stages + o->cur * 4));
    if (stage->flags & 0x80) return false;
    const RwPair& pair = gm<RwPair>(stage->pairs + stage->cur * 8);
    u32 owner = o->owner;
    if (pair.giver != 0u && check != 0 && Busy(pair.giver) != 0) return false;
    const u32 slot = stage->objs + stage->cur * 8;
    if (const u32 s = gm<be_u32>(slot)) vcall(&gm<u8>(s), 88, 0u, 1u, 1u, 1u);

    u32 result = 0;
    if (pair.giver != 0u) {
        const u32 so = gm<be_u32>(slot);
        CopyMtxXYZ(L.mat, Obj(so != 0 ? so : owner).m);
        call(kMatrixNormalize, &L.mat);
        SpawnFrame(L.sf1, L.mat);
        const u32 giver = pair.giver;
        u32 amt1 = gm<RwGiver>(giver).amt1, amt2 = gm<RwGiver>(giver).amt2;
        L.hash = 0u;
        vcall(&gm<u8>(giver), 12, &L.hash, 0u, pair.arg.get());
        const u32 local = call<u32>(kIsLocalPlayerSlot, player);
        if (amt2 > 1 && Busy(pair.giver) == 0) {
            const f64 one = C(0);
            if (local != 0) {
                if (const u32 prof = call<u32>(kProfileCached)) {
                    L.hash = gm<be_u32>(prof + 52).get();
                    amt1 = ScaleAdd(amt1, call<f64>(kGlobalGet10C2478), one);
                    amt2 = ScaleAdd(amt2, call<f64>(kGlobalGet10C2478), one);
                }
            } else {
                if (owner != 0 && (Obj(owner).flags & 8) != 0) {
                    const u32 b = Obj(owner).b709;
                    if ((b & 0x80) != 0 && (b & 0x40) == 0) {
                        amt1 = amt2 + 1;
                        amt2 <<= 1;
                    }
                }
                if (player != nullptr && (player->d184 & 1) != 0) {
                    const u32 sv = global<be_u32>(kWorld) + static_cast<u32>(static_cast<s32>(static_cast<s8>(player->w172.b[0])) * 0x20C80) + 4208;
                    if (call<u32>(kSaveSlotBits2Is1, sv, 1u) != 0) {
                        const f64 k = lf(global<be_f32>(kSaveMul));
                        amt1 = ScaleAdd(amt1, k, one);
                        amt2 = ScaleAdd(amt2, k, one);
                    }
                }
            }
        }

        if (const u32 h = L.hash) {
            L.info.w0 = result;
            L.info.hash = h;
            L.info.amt1 = amt1;
            L.info.amt2 = amt2;
            L.info.w16 = result;
            const u32 idle = Busy(pair.giver);
            if (idle != 0) call(kCollectMarkFound, o, L.hash.get());
            call(kPtrVectorConstruct, &L.vec);
            L.vec.esize = 4;
            if (player != nullptr) {
                const Vec4& pa = player->m.r[3];
                const Vec4& pb = Obj(owner).m.r[3];
                const f64 dx = Subs(lf(pa.x), lf(pb.x)), dz = Subs(lf(pa.z), lf(pb.z));
                L.dir.x = static_cast<f32>(dx);
                L.dir.z = static_cast<f32>(dz);
                set_bits(L.dir.y, result);  // the word 0
                const f64 dy = C(4), eps = C(32);
                const Vec3& z = global<Vec3>(kZeroVec);
                if (!(std::fabs(Subs(dx, lf(z.x))) < eps && std::fabs(Subs(dy, lf(z.y))) < eps && std::fabs(Subs(dz, lf(z.z))) < eps)) {
                    const f64 inv = Divs(C(0), Sqrts(Fmadds(dz, dz, Fmadds(dx, dx, S(dy * dy)))));
                    L.dir.x = static_cast<f32>(S(dx * inv));
                    L.dir.y = static_cast<f32>(S(dy * inv));
                    L.dir.z = static_cast<f32>(S(dz * inv));
                }
                call(kApplyKind2, &L.sf1, &L.info, kKindDrop, &L.dir, owner, &L.vec);
            } else {
                SpawnFrame(L.sf2, L.mat);
                const u32 kind = call<u32>(kReturnK);
                call(kSpawnAt, &L.sf2, &L.info, kind, &L.vec);
            }
            u32 cnt = L.vec.count;
            if (cnt != 0) result = 1;
            result = idle & result;
            const f64 zero = C(4);
            for (u32 i = 0; i < cnt; ++i) {
                const u32 e = L.vec.data[i];
                if (e == 0 || (Obj(e).flags & 0x40) == 0) continue;
                RwObj& p = Obj(e);
                p.life = global<be_u32>(kSpawnLife).get();
                const u32 bit = lf(global<be_f32>(kSpawnLife)) > zero ? 1u : 0u;
                p.h432 = static_cast<u16>((p.h432 & ~0x40u) | (bit << 6));
                p.w592 = p.w592 | 0x08000000u;
                const u32 r7 = gm<be_u32>(p.w216 + 436);
                cnt = L.vec.count;
                if (r7 != 0) result = 1;
            }
            call(kPtrVectorClear, &L.vec);
            call(kNop, &L.vec);
        }
    }

    // Advance the step.
    const u32 n = stage->cur + 1;
    const u32 steps = stage->steps;
    stage->cur = n;
    if (n < steps) {
        if (const u32 s = gm<be_u32>(stage->objs + n * 8)) vcall(&gm<u8>(s), 88, 1u, 1u, 1u, 1u);
        return result != 0;
    }
    stage->flags = static_cast<u8>(stage->flags | 0x80);
    o->cur = o->cur + 1;
    if (const u32 c = stage->obj) {
        u32 def = stage->def;
        const u32 want = gm<RwDef>(def).want;
        if (want != 0) {
            u32 have = 0;
            if (const u32 r6 = Obj(c).w216) have = gm<be_u32>(r6 + 52);
            const u32 w36 = def != 0 ? gm<RwDef>(def).w36.get() : 0u;
            if (have == want && (def == 0 || w36 == 0 || gm<u8>(w36 + 40) == 0)) {
                if (Obj(c).w172.w & 0x40000u) {
                    vcall(&gm<u8>(c), 40, 1u);
                    def = stage->def;
                }
                const u32 x = gm<RwDef>(def).x;
                if (x != 0) call(kStageDone, stage, owner, x);
            }
        }
    }
    if (o->cur >= o->count) {
        owner = call<u32>(kTargetGetDef, owner);
        if (owner != 0) call(kSetStateFx, owner, 4u);
    }
    return result != 0;
}

}  // namespace sly4::obj

NATIVE(00268300, sly4::obj::RewardSeq_Step_00268300)
