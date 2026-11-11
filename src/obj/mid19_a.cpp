// Game code, medium sweep part 19: spawns from defs, spline-driven object
// states and their dispatchers.
#include "mid19.h"

using namespace sly4;
using namespace sly4::mid19;

namespace {
constexpr u32 kStringFormat = 0x005534D0u;          // String_Format(fmt, ...) -> static buffer
constexpr u32 kSpawnQueryConstruct = 0x000CC188u;   // SpawnQuery_Construct0CC188
constexpr u32 kPtrVectorClear = 0x0054EC1Cu;        // PtrVector_Clear
constexpr u32 kNop = 0x0054EC18u;                   // Nop
constexpr u32 kSmallFree = 0x00552420u;             // SmallFree(p, size)
constexpr u32 kProjCounter = 0x00F64738u;           // u32 running projectile name counter
constexpr u32 kProjNameFmt = 0x00AC6BD0u;           // "EntityCompWeapon_Proj_%i"
constexpr u32 kQueryPos = 0x01164B30u;              // f32[4] passed as the query position
constexpr u32 kSpawnQueryVt = 0x00B12CB8u;          // the spawn query's vtable
constexpr u32 kQueryVt2 = 0x00B12868u, kQueryVt1 = 0x00B12260u;  // base vtables restored on destruction
}  // namespace

// Obj* Obj_SpawnDef0EDE30(Def* b, Obj* a, const Mat34* rot, const Vec3* dir,
// - (r7 unused), u32 s, Obj* owner, const Mat34* frame, u32 flag (9th
// argument, low byte); f32 v): null b or b->400 gives 0. A spawn query on
// the stack (q = sp + 144, 280 bytes) is built with the name
// String_Format("EntityCompWeapon_Proj_%i", counter++) (*0x00F64738),
// SpawnQuery_Construct0CC188(q, name, b, 0, rot, 0x01164B30, 1, 0, 9th = 0,
// 11th = 0 (the 10th slot is not written); f1 = *(f32*)0x000EDE2C), then
// vtable 0x00B12CB8, q+240 = dir * v, q+256 = b+240 (Vec3), q+4 |= 0x2000,
// q+272 = b->328, q+276 = a, q+136 = s. o = b->vfn[28](b, q); an object
// with flag 0x2000 in +192 is kept: owner change (old o->436 vfn[40](0),
// o->436 = owner with its u16 +178 counted, and the same for o->620's +48
// when the owner has bit 0x40000 in +172), o->440 vfn[88](frame != 0, 1, 1,
// 1), then without frame o->vfn[376](owner), else o->448 = frame (3x4) and
// o->440 vfn[84](o + 448); then o->616 = v when the flag byte is set, o->512
// = rot+48 and o->vfn[380](). The query is destroyed (vtables 0x00B12868 /
// 0x00B12260, PtrVector_Clear + Nop of q+200 / q+180 and of q+16, then
// SmallFree(q+16, 20) when present). Returns the kept object or 0.
DECOMP(000EDE30, Obj_SpawnDef0EDE30) {
    const u32 b = arg<u32>(ctx, 0), a = arg<u32>(ctx, 1), rot = arg<u32>(ctx, 2), dir = arg<u32>(ctx, 3);
    const u32 s = arg<u32>(ctx, 5), owner = arg<u32>(ctx, 6), frm = arg<u32>(ctx, 7);
    const f64 v = ctx.fpr[1];
    if (b == 0 || load<u32>(b + 400) == 0) {
        ret(ctx, 0);
        return;
    }
    const u32 flag = load<u32>(static_cast<u32>(ctx.gpr[1]) + 116);  // 9th argument (low byte tested)
    const f64 dx = Lf(dir), dy = Lf(dir + 4), dz = Lf(dir + 8);
    Stack frame(ctx, 448);
    const u32 sp = frame.addr() - 112;  // the original's r1: locals at their original offsets
    const u32 q = sp + 144;
    const u32 cnt = load<u32>(kProjCounter);
    store<u32>(kProjCounter, cnt + 1);
    const u32 name = static_cast<u32>(call(ctx, kStringFormat, {kProjNameFmt, cnt}));
    const u32 b328 = load<u32>(b + 328);
    callf(ctx, kSpawnQueryConstruct, {q, name, b, 0, rot, kQueryPos, 1, 0, 0, 0, 0}, {Lf(0x000EDE2Cu)});
    const u32 w4 = load<u32>(q + 4);
    store<u32>(q + 0, kSpawnQueryVt);
    Sf(q + 240, S(dx * v));
    Sf(q + 244, S(dy * v));
    Sf(q + 248, S(dz * v));
    Cw(q + 256, b + 240);
    Cw(q + 260, b + 244);
    store<u32>(q + 4, w4 | 0x2000);
    Cw(q + 264, b + 248);
    store<u32>(q + 272, b328);
    store<u32>(q + 276, a);
    store<u32>(q + 136, s);
    ctx.fpr[1] = Lf(b + 248);  // f1 as the original leaves it
    const u32 got = static_cast<u32>(VCall(ctx, b, 28, {b, q}));
    u32 o = 0;
    if (got != 0 && (load<u32>(got + 192) & 0x2000) != 0) {
        o = got;
        const u32 cur = load<u32>(o + 436);
        if (owner != cur) {
            if (cur != 0) VCall(ctx, cur, 40, {cur, 0});
            store<u32>(o + 436, owner);
            if (owner != 0) store<u16>(owner + 178, static_cast<u16>(load<u16>(owner + 178) + 1));
            const u32 l = load<u32>(o + 620);
            if (l != 0) {
                const u32 old = load<u32>(l + 48);
                const u32 nw = load<u32>(o + 436);
                if (old != 0) VCall(ctx, old, 40, {old, 0});
                store<u32>(l + 48, nw);
                if (nw != 0 && (load<u32>(nw + 172) & 0x40000) != 0) store<u16>(nw + 178, static_cast<u16>(load<u16>(nw + 178) + 1));
            }
        }
        const u32 g = load<u32>(o + 440);
        if (g != 0) VCall(ctx, g, 88, {g, frm != 0 ? 1u : 0u, 1, 1, 1});
        if (frm == 0) {
            VCall(ctx, o, 376, {o, owner});
        } else {
            CopyMat34(o + 448, frm);
            ctx.fpr[1] = Lf(frm + 56);  // f1 as the original leaves it
            const u32 g2 = load<u32>(o + 440);
            if (g2 != 0) VCall(ctx, g2, 84, {g2, o + 448});
        }
        if ((flag & 0xFF) != 0) Sf(o + 616, v);
        CopyVec3(o + 512, rot + 48);
        ctx.fpr[1] = Lf(rot + 56);  // f1 as the original leaves it
        VCall(ctx, o, 380, {o});
    }
    // the query's destructor
    store<u32>(q + 0, kQueryVt2);
    call(ctx, kPtrVectorClear, {sp + 344});
    call(ctx, kNop, {sp + 344});
    call(ctx, kPtrVectorClear, {sp + 324});
    call(ctx, kNop, {sp + 324});
    const u32 vec = load<u32>(q + 16);
    if (vec != 0) {
        store<u32>(q + 0, kQueryVt1);
        call(ctx, kPtrVectorClear, {vec});
        call(ctx, kNop, {vec});
        call(ctx, kSmallFree, {load<u32>(q + 16), 20});
    }
    ret(ctx, o);
}

namespace {
constexpr u32 kFindBone = 0x000DA9D0u;           // Obj_FindBone_000DA9D0(body, hash)
constexpr u32 kAnimRestart = 0x002775A8u;        // Anim_Restart2775A8(Animator*, key, ... ; f1..f4)
constexpr u32 kAnimFind = 0x00298874u;           // Anim_Find298874(Animator*, key) -> u8
constexpr u32 kAnimTrackSet = 0x0029879Cu;       // Anim_TrackSet36_0029879C(a, s32 id, u8 v) -> bool
constexpr u32 kEffectSpawn = 0x004A5FC8u;        // Effect_Spawn(hash, 0, 0, 1)
constexpr u32 kSplinePointAt = 0x00592748u;      // WorldObj_SplinePointAt(spl, -, Vec3* out, 0; f32 t)
constexpr u32 kSetSplinePos = 0x003CCC68u;       // Obj_SetSplinePos3CCC68(o, const Vec3*)
constexpr u32 kStateStart = 0x003CCA60u;         // Obj_StateStart3CCA60(o; dt)
constexpr u32 kGameWorld = 0x00F65CA0u;          // World* (+4208 + 0x20C80 * slot: slot records)
constexpr u32 kGameCtx = 0x00F65844u;            // g_GameCtx_00F65844 (+581 byte flag)
constexpr u32 kZoneTable = 0x010B19E0u;          // 11968-byte zone records

// The body's animator: o->4 vfn[60]().
inline u32 Animator(PPUContext& ctx, u32 o) {
    const u32 a = load<u32>(o + 4);
    return static_cast<u32>(VCall(ctx, a, 60, {a}));
}

// Anim_Restart2775A8(a, h, 0, 0, r7, r8, 0, 0, -, -, 0, -1; f1..f4): the
// callee reads neither r7 nor r8 nor the never-written 9th / 10th stack
// slots; they are passed as the original leaves them.
inline void Restart(PPUContext& ctx, u32 a, u32 h, u64 r7, u64 r8, f64 f1, f64 f2, f64 f3, f64 f4) {
    callf(ctx, kAnimRestart, {a, h, 0, 0, r7, r8, 0, 0, 0, 0, 0, ~0ull}, {f1, f2, f3, f4});
}

// The end of every tick: the zone record of the world slot o->84 (byte +1 of
// world +4208 + 0x20C80 * slot) with bit 0x40 in both +184 and +188 sets the
// byte g_GameCtx->581. r3 as the original leaves it (the context or 0).
inline void ZoneFlagCheck(PPUContext& ctx, u32 o) {
    const u32 p = load<u32>(kGameWorld) + load<u32>(o + 84) * 0x20C80u + 4208;
    const u32 e = kZoneTable + static_cast<u32>(load<u8>(p + 1)) * 11968u + 176;
    const u32 a = load<u32>(e + 8), b = load<u32>(e + 12);
    if ((a & 0x40) == 0 || (b & 0x40) == 0) {
        ctx.gpr[3] = 0;
        return;
    }
    const u32 g = load<u32>(kGameCtx);
    store<u8>(g + 581, 1);
    ctx.gpr[3] = g;
}
}  // namespace

// Obj_StateTick3CF648(Obj* o; f32 dt) (called by 003D2514): the byte o->124 =
// 0, Obj_FindBone_000DA9D0(o->4, 0xB3521F36), then by the state o->76: 0
// starts the state o->80 (o->76 = o->80, o->96 = 0, o->112->108 vfn[88](0, 1,
// 1, 1), spline o->108 point at c[0] (c = 0x003CF638) -> Obj_SetSplinePos3CCC68,
// o->96 / byte o->116 / o->100 = 0); 1 -> o->112->108 vfn[88](0, 1, 1, 1),
// 003CF0F8; 2 -> 003CC068; 3 -> 003CE2C0; 4 -> byte o->116 = 0, 003CD4A0;
// 5 -> Obj_StateStart3CCA60; 6 -> on the first tick (o->96 == c[1]) anim
// 0xE1C691BF (f1..f4 = c[2], c[3], c[1], c[1]) into o->128 and, unless o->88,
// Effect_Spawn(0x3019063C); stops (o->80 = 4, o->76 = o->96 = 0,
// *(o->112) = 0) when the anim is no longer found as o->128 or its track
// finished; 7 -> 003CC720; 8 -> 003CC420 (all with dt). Then ZoneFlagCheck.
DECOMP(003CF648, Obj_StateTick3CF648) {
    const u32 o = arg<u32>(ctx, 0);
    const f64 dt = ctx.fpr[1];
    constexpr u32 C = 0x003CF638u;
    store<u8>(o + 124, 0);
    const u64 h = static_cast<u64>(static_cast<s64>(static_cast<s32>(0xB3521F36u)));  // lis/ori: sign-extended
    call(ctx, kFindBone, {load<u32>(o + 4), h, h});  // r5 left holding the hash (not read)
    const s32 s = load<s32>(o + 76);
    switch (s) {
    case 0: {
        Stack frame(ctx, 80);
        const u32 sp = frame.addr() - 112;  // the original's r1
        store<u32>(o + 76, load<u32>(o + 80));
        store<u32>(o + 96, 0);
        const u32 b = load<u32>(load<u32>(o + 112) + 108);
        VCall(ctx, b, 88, {b, 0, 1, 1, 1});
        const u32 spl = load<u32>(o + 108);
        callf(ctx, kSplinePointAt, {spl, spl, sp + 144, 0}, {Lf(C)});  // r4 left holding spl (not read)
        call(ctx, kSetSplinePos, {o, sp + 144});
        store<u32>(o + 96, 0);
        store<u8>(o + 116, 0);
        store<u32>(o + 100, 0);
        break;
    }
    case 1: {
        const u32 b = load<u32>(load<u32>(o + 112) + 108);
        VCall(ctx, b, 88, {b, 0, 1, 1, 1});
        callf(ctx, 0x003CF0F8u, {o}, {dt});
        break;
    }
    case 2: callf(ctx, 0x003CC068u, {o}, {dt}); break;
    case 3: callf(ctx, 0x003CE2C0u, {o}, {dt}); break;
    case 4:
        store<u8>(o + 116, 0);
        callf(ctx, 0x003CD4A0u, {o}, {dt});
        break;
    case 5: callf(ctx, kStateStart, {o}, {dt}); break;
    case 6: {
        const f64 c1 = Lf(C + 4);
        ctx.fpr[1] = Lf(o + 96);  // f1 as the original leaves it
        if (Lf(o + 96) == c1) {
            const u32 a = Animator(ctx, o);
            Restart(ctx, a, 0xE1C691BFu, ~0ull, 0, Lf(C + 8), Lf(C + 12), c1, c1);
            store<u32>(o + 128, 0xE1C691BFu);
            if (load<u32>(o + 88) == 0) call(ctx, kEffectSpawn, {0x3019063Cu, 0, 0, 1});
        }
        bool stop = true;
        if (static_cast<u32>(call(ctx, kAnimFind, {Animator(ctx, o), 0})) == 0 && load<u32>(o + 128) == 0xE1C691BFu)
            stop = static_cast<u32>(call(ctx, kAnimTrackSet, {Animator(ctx, o), 0, 0})) != 0;
        if (stop) {
            store<u32>(o + 80, 4);
            store<u32>(o + 76, 0);
            store<u32>(o + 96, 0);
            store<u32>(load<u32>(o + 112), 0);
        }
        break;
    }
    case 7: callf(ctx, 0x003CC720u, {o}, {dt}); break;
    case 8: callf(ctx, 0x003CC420u, {o}, {dt}); break;
    default: break;
    }
    ZoneFlagCheck(ctx, o);
}

// Obj_StateTick3D1AD8(Obj* o; f32 dt) (called by 003D2514): by the state
// o->76: 0 starts the state o->80 (o->96 = 0, o->76 = o->80, for state 1
// o->112->108 vfn[88](0, 1, 1, 1), spline o->108 point at c[0] (c =
// 0x003D1AC0) -> Obj_SetSplinePos3CCC68, o->96 / byte o->116 / o->100 = 0;
// r3 = 0); 1 -> o->112->108 vfn[88](0, 1, 1, 1), 003D1268; 2 ->
// Carry_FollowBone_003CFA18; 3 -> 003D0598; 4 -> byte o->116 = 0, spline
// point at c[0] into o+48, 003D03A0; 5 -> Obj_StateStart3D0158; 6 -> on the
// first tick (o->96 == c[1]) anim 0xE1C691BF (f1..f4 = c[2], c[3], c[1],
// c[1]) into o->128, o->12 += c[3], o->8 += c[4]; stops (o->80 = 4, o->76 =
// o->96 = 0, byte o->94 = 1, *(o->112) = 0; r3 = 4) when the anim is no
// longer found as o->128 or its track finished; 7 -> Obj_StateIdle3CFF28; 8
// -> Obj_StateIdle3CFD10 (all with dt). r3 / f1 as the original leaves them
// (o->76 and dt when nothing runs).
DECOMP(003D1AD8, Obj_StateTick3D1AD8) {
    const u32 o = arg<u32>(ctx, 0);
    const f64 dt = ctx.fpr[1];
    constexpr u32 C = 0x003D1AC0u;
    const s32 s = load<s32>(o + 76);
    ctx.gpr[3] = static_cast<u32>(s);
    switch (s) {
    case 0: {
        Stack frame(ctx, 80);
        const u32 sp = frame.addr() - 112;  // the original's r1
        const u32 next = load<u32>(o + 80);
        store<u32>(o + 96, 0);
        store<u32>(o + 76, next);
        if (static_cast<s32>(next) == 1) {
            const u32 b = load<u32>(load<u32>(o + 112) + 108);
            VCall(ctx, b, 88, {b, 0, 1, 1, 1});
        }
        const u32 spl = load<u32>(o + 108);
        callf(ctx, kSplinePointAt, {spl, spl, sp + 144, 0}, {Lf(C)});  // r4 left holding spl (not read)
        call(ctx, kSetSplinePos, {o, sp + 144});
        store<u32>(o + 96, 0);
        store<u8>(o + 116, 0);
        store<u32>(o + 100, 0);
        ctx.gpr[3] = 0;
        break;
    }
    case 1: {
        const u32 b = load<u32>(load<u32>(o + 112) + 108);
        VCall(ctx, b, 88, {b, 0, 1, 1, 1});
        callf(ctx, 0x003D1268u, {o}, {dt});
        break;
    }
    case 2: callf(ctx, 0x003CFA18u, {o}, {dt}); break;  // Carry_FollowBone_003CFA18
    case 3: callf(ctx, 0x003D0598u, {o}, {dt}); break;
    case 4:
        store<u8>(o + 116, 0);
        callf(ctx, kSplinePointAt, {load<u32>(o + 108), C, o + 48, 0}, {Lf(C)});  // r4 left holding c (not read)
        callf(ctx, 0x003D03A0u, {o}, {dt});
        break;
    case 5: callf(ctx, 0x003D0158u, {o}, {dt}); break;  // Obj_StateStart3D0158
    case 6: {
        const f64 c1 = Lf(C + 4);
        ctx.fpr[1] = Lf(o + 96);  // f1 as the original leaves it
        if (Lf(o + 96) == c1) {
            const u32 a = Animator(ctx, o);
            const f64 c3 = Lf(C + 12);
            Restart(ctx, a, 0xE1C691BFu, 0, ~0ull, Lf(C + 8), c3, c1, c1);
            const f64 v12 = Lf(o + 12), v8 = Lf(o + 8);
            store<u32>(o + 128, 0xE1C691BFu);
            Sf(o + 12, S(v12 + c3));
            const f64 n8 = S(v8 + Lf(C + 16));
            Sf(o + 8, n8);
            ctx.fpr[1] = n8;  // f1 as the original leaves it
        }
        bool stop = true;
        if (static_cast<u32>(call(ctx, kAnimFind, {Animator(ctx, o), 0})) == 0 && load<u32>(o + 128) == 0xE1C691BFu)
            stop = static_cast<u32>(call(ctx, kAnimTrackSet, {Animator(ctx, o), 0, 0})) != 0;
        if (stop) {
            const u32 p = load<u32>(o + 112);
            store<u32>(o + 80, 4);
            store<u32>(o + 76, 0);
            store<u32>(o + 96, 0);
            store<u8>(o + 94, 1);
            store<u32>(p, 0);
            ctx.gpr[3] = 4;
        }
        break;
    }
    case 7: callf(ctx, 0x003CFF28u, {o}, {dt}); break;  // Obj_StateIdle3CFF28
    case 8: callf(ctx, 0x003CFD10u, {o}, {dt}); break;  // Obj_StateIdle3CFD10
    default: break;
    }
}

namespace {
// The zone record of the world slot o->84 (see ZoneFlagCheck).
inline u32 ZoneRec(u32 o) {
    const u32 p = load<u32>(kGameWorld) + load<u32>(o + 84) * 0x20C80u + 4208;
    return kZoneTable + static_cast<u32>(load<u8>(p + 1)) * 11968u + 176;
}

// The switch to the idle animation h (0xE1C7FA75): unless Anim_Find298874(a,
// 0) is set (twice, a = Animator(o) each time) or Anim_TrackSet36_0029879C(a,
// 0, 0) is 0, h is started unless o->128 already holds it: o->128 = h,
// Restart(Animator(o), h, 0, -1; f1..f4).
inline void AnimSwitch(PPUContext& ctx, u32 o, u32 h, f64 f1, f64 f2, f64 f3, f64 f4) {
    if (static_cast<u32>(call(ctx, kAnimFind, {Animator(ctx, o), 0})) == 0) {
        if (static_cast<u32>(call(ctx, kAnimFind, {Animator(ctx, o), 0})) != 0) return;
        if (static_cast<u32>(call(ctx, kAnimTrackSet, {Animator(ctx, o), 0, 0})) == 0) return;
    }
    if (load<u32>(o + 128) == h) return;
    store<u32>(o + 128, h);
    const u32 a = Animator(ctx, o);
    Restart(ctx, a, h, 0, ~0ull, f1, f2, f3, f4);
}

// A state's first tick (o->96 == c[0]): unless o->88, Effect_Spawn(e1) and
// (e2); anim h (Restart(Animator(o), h, 0, -1; c[1], c[2], c[0], c[0])) into
// o->128. Then AnimSwitch to the idle 0xE1C7FA75 with the same arguments.
inline void StateAnim(PPUContext& ctx, u32 o, u32 C, u32 e1, u32 e2, u32 h) {
    const f64 c0 = Lf(C);
    ctx.fpr[1] = Lf(o + 96);  // f1 as the original leaves it
    if (Lf(o + 96) == c0) {
        if (load<u32>(o + 88) == 0) {
            call(ctx, kEffectSpawn, {e1, 0, 0, 1});
            call(ctx, kEffectSpawn, {e2, 0, 0, 1});
        }
        const u32 a = Animator(ctx, o);
        Restart(ctx, a, h, 0, ~0ull, Lf(C + 4), Lf(C + 8), c0, c0);
        store<u32>(o + 128, h);
    }
    AnimSwitch(ctx, o, 0xE1C7FA75u, Lf(C + 4), Lf(C + 8), c0, c0);
}

// The hand-over between o and its partner t = o->120 (reloaded for each
// access): o->80 = so, o->76 = o->96 = 0, *(o->112) = 0, byte o->94 = bo,
// t->80 = st, t->76 = t->96 = 0, byte t->94 = bt.
inline void HandOver(u32 o, u32 so, u32 bo, u32 st, u32 bt) {
    const u32 p = load<u32>(o + 112);
    store<u32>(o + 80, so);
    store<u32>(o + 76, 0);
    store<u32>(o + 96, 0);
    store<u32>(p, 0);
    const u32 t = load<u32>(o + 120);
    store<u8>(o + 94, static_cast<u8>(bo));
    store<u32>(t + 80, st);
    const u32 t2 = load<u32>(o + 120);
    store<u32>(t2 + 76, 0);
    store<u32>(t2 + 96, 0);
    store<u8>(load<u32>(o + 120) + 94, static_cast<u8>(bt));
}
}  // namespace

// Obj_StateThrow3CC420(Obj* o) (called by Obj_StateTick3CF648, state 8):
// StateAnim (c = 0x003CC414; effects 0xBCA67A7F / 0x25E11BEA, anim
// 0xE1C691BF); when the zone record (ZoneRec) has bit 0x80 in both +184 and
// +188, HandOver(o, 4, 1, partner 1, 0). r3 as the original leaves it
// (o->120, the +188 word or 0).
DECOMP(003CC420, Obj_StateThrow3CC420) {
    const u32 o = arg<u32>(ctx, 0);
    StateAnim(ctx, o, 0x003CC414u, 0xBCA67A7Fu, 0x25E11BEAu, 0xE1C691BFu);
    const u32 e = ZoneRec(o);
    const u32 a = load<u32>(e + 8), b = load<u32>(e + 12);
    if ((a & 0x80) == 0 || (b & 0x80) == 0) {
        ctx.gpr[3] = (a & 0x80) == 0 ? b : 0u;  // as the original leaves r3 (+188 unmasked when +184 fails)
        return;
    }
    HandOver(o, 4, 1, 1, 0);
    ctx.gpr[3] = load<u32>(o + 120);
}

// Obj_StateCatch3CC720(Obj* o) (called by Obj_StateTick3CF648, state 7):
// StateAnim (c = 0x003CC710; effects 0x3FE47F12 / 0xACB71673, anim
// 0x1E7563B1); with bit 0x80 in both zone words HandOver(o, 1, 0, partner 4,
// 1); then *(o->112) = 0 and o->112->108 vfn[88](0, 1, 1, 1).
DECOMP(003CC720, Obj_StateCatch3CC720) {
    const u32 o = arg<u32>(ctx, 0);
    StateAnim(ctx, o, 0x003CC710u, 0x3FE47F12u, 0xACB71673u, 0x1E7563B1u);
    const u32 e = ZoneRec(o);
    const u32 a = load<u32>(e + 8), b = load<u32>(e + 12);
    if ((a & 0x80) != 0 && (b & 0x80) != 0) HandOver(o, 1, 0, 4, 1);
    const u32 p = load<u32>(o + 112);
    const u32 bd = load<u32>(p + 108);
    store<u32>(p, 0);
    VCall(ctx, bd, 88, {bd, 0, 1, 1, 1});
}
