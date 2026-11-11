// Game code, medium sweep part 19: message handlers and spline-driven
// object updates.
#include "mid19.h"

using namespace sly4;
using namespace sly4::mid19;

namespace {
constexpr u32 kObjOnMsgBase = 0x000133FCu;    // the base message handler (o, m)
constexpr u32 kObjFaceToward = 0x00024670u;   // Obj_FaceToward24670(o, p)
}  // namespace

// u32 Obj_OnMsg248D0(Obj* o, Msg* m) (vtable entry): by the message hash
// m->8: 0x00764FA9 sets bit 0x40 of the byte o->416 and, in mode o->4 == 5,
// pops the o->24 stack (decrement when non-zero) and o->vfn[220](o, o->8[n]);
// 0x7D2BB408 clears that bit, 0x50A63D95 sets 0x20; 0xFAE875CE /
// 0x632B318B / 0xB32B479B / 0x5A7E7E77 set o->348 = 0 / 1 / 2 / 3;
// 0xDD92E72B faces Obj_FaceToward24670 to o+400 when one of the o->24
// entries o->8[] is 11, else to o+112 (mode 7) or o+96. Then returns the
// base handler 0x000133FC(o, m).
DECOMP(000248D0, Obj_OnMsg248D0) {
    const u32 o = arg<u32>(ctx, 0), m = arg<u32>(ctx, 1);
    const u32 k = load<u32>(m + 8);
    switch (k) {
    case 0x00764FA9u:
        store<u8>(o + 416, static_cast<u8>(load<u8>(o + 416) | 0x40));
        if (load<s32>(o + 4) == 5) {
            u32 n = load<u32>(o + 24);
            if (n != 0) {
                n -= 1;
                store<u32>(o + 24, n);
            }
            // vfn[220]: this call sequence leaves the entry in r5 and the OPD in r6
            const u32 opd = load<u32>(load<u32>(o) + 220);
            call_ptr(ctx, opd, {o, load<u32>(o + 8 + 4 * n), load<u32>(opd), opd});
        }
        break;
    case 0x7D2BB408u: store<u8>(o + 416, static_cast<u8>(load<u8>(o + 416) & ~0x40)); break;
    case 0x50A63D95u: store<u8>(o + 416, static_cast<u8>(load<u8>(o + 416) | 0x20)); break;
    case 0xFAE875CEu: store<u32>(o + 348, 0); break;
    case 0x632B318Bu: store<u32>(o + 348, 1); break;
    case 0xB32B479Bu: store<u32>(o + 348, 2); break;
    case 0x5A7E7E77u: store<u32>(o + 348, 3); break;
    case 0xDD92E72Bu: {
        const u32 n = load<u32>(o + 24);
        bool found = false;
        for (u32 i = 0; i < n; ++i)
            if (load<s32>(o + 8 + 4 * i) == 11) {
                found = true;
                break;
            }
        if (found) call(ctx, kObjFaceToward, {o, o + 400});
        else if (load<s32>(o + 4) == 7) call(ctx, kObjFaceToward, {o, o + 112});
        else call(ctx, kObjFaceToward, {o, o + 96});
        break;
    }
    default: break;
    }
    ctx.gpr[3] = call(ctx, kObjOnMsgBase, {o, m});
}

namespace {
constexpr u32 kSetEffectHash = 0x000DAEC8u;  // Obj_SetEffectHash_000DAEC8(o, hash)

// The body b = *slot's current effect hash: b->vfn[272]() -> e, e->4 ?
// e->4->8 : 0; when it differs from h, Obj_SetEffectHash_000DAEC8(*slot
// (reloaded), h). r3 as the original leaves it (h when equal, else the
// call's result).
void EnsureEffect(PPUContext& ctx, u32 slot, u32 h) {
    const u32 b = load<u32>(slot);
    const u32 e = static_cast<u32>(VCall(ctx, b, 272, {b}));
    const u32 p = load<u32>(e + 4);
    const u32 cur = p != 0 ? load<u32>(p + 8) : 0u;
    if (cur == h) ctx.gpr[3] = h;
    else call(ctx, kSetEffectHash, {load<u32>(slot), h});
}
}  // namespace

// Obj_Tick3CB968(Obj* o; f32 dt) (called by Obj_Update_003D280C): t = dt *
// o->104, then by the state o->0: 6 -> 003CB260(o; t); 5 -> effect
// 0x0020D05C when o->104 > *(f32*)0x003CB964 else 0x87E69C4E on the body
// o->108, body vfn[88](1, 1, 1, 1), 003CB808(o; t); 4 / 3 -> body vfn[88](1,
// 1, 1, 1), 003CB430 / Obj_FallTick3CB618(o; t); 1 -> body vfn[88](0, 1, 1,
// 1) and effect 0x03C28089. r3 / f1 as the original leaves them (the state
// and o->104 when nothing runs).
DECOMP(003CB968, Obj_Tick3CB968) {
    const u32 o = arg<u32>(ctx, 0);
    const f64 sp = Lf(o + 104);
    const f64 t = S(ctx.fpr[1] * sp);
    ctx.fpr[1] = sp;
    const s32 st = load<s32>(o + 0);
    ctx.gpr[3] = static_cast<u32>(st);
    switch (st) {
    case 6:
        callf(ctx, 0x003CB260u, {o}, {t});
        break;
    case 5: {
        EnsureEffect(ctx, o + 108, sp > Lf(0x003CB964u) ? 0x0020D05Cu : 0x87E69C4Eu);
        const u32 b2 = load<u32>(o + 108);
        VCall(ctx, b2, 88, {b2, 1, 1, 1, 1});
        callf(ctx, 0x003CB808u, {o}, {t});
        break;
    }
    case 4:
    case 3: {
        const u32 b = load<u32>(o + 108);
        VCall(ctx, b, 88, {b, 1, 1, 1, 1});
        callf(ctx, st == 4 ? 0x003CB430u : 0x003CB618u, {o}, {t});
        break;
    }
    case 1: {
        const u32 b = load<u32>(o + 108);
        VCall(ctx, b, 88, {b, 0, 1, 1, 1});
        EnsureEffect(ctx, o + 108, 0x03C28089u);
        break;
    }
    default: break;
    }
}

namespace {
constexpr u32 kObjCheck1AEA50 = 0x001AEA50u;    // Obj_Check_001AEA50(o; f32 dt) -> bool
constexpr u32 kPathSetActive = 0x001D218Cu;     // 001D218C(o, bool)
constexpr u32 kPathReset = 0x001D0F68u;         // Obj_ResetPath1D0F68(o)
constexpr u32 kPathStep = 0x001D0450u;          // 001D0450(o, 0) -> bool
constexpr u32 kApplyPose = 0x001D0E58u;         // Obj_ApplyPose_001D0E58(o)
constexpr u32 kEffectSetHash = 0x004CF988u;     // Effect_SetHash4CF988(e, hash)
constexpr u32 kSplinePointTangentAt = 0x005A80D8u;  // WorldObj_SplinePointTangentAt(spl, -, pos*, tan*, 0; t)

// The path gate o->708 (u16 +352 non-zero = open) into bit 0x10000000 of
// o->972; returns the new bit (the stored word in *w).
inline u32 PathGate(u32 o, u32 g, u32& w) {
    const u32 f = load<u16>(g + 352) != 0 ? 1u : 0u;
    w = (w & ~0x10000000u) | (f << 28);
    store<u32>(o + 972, w);
    return f;
}
}  // namespace

// u32 Obj_PathTick1D2370(Obj* o; f32 dt) (vtable entry): 0 unless
// Obj_Check_001AEA50(o; dt); 1 at once when o->972 has 0x04000000. First run
// (no 0x02000000 yet): sets it and, with a gate o->708, its state into bit
// 0x10000000 and 001D218C(o, open). With a gate: a change of its state
// restarts (001D218C(o, open), Obj_ResetPath1D0F68(o)). A pending o->804 /
// o->792 forces a pose update. Moving along the spline o->704 (o->972 bit
// 0x08000000): a pause o->896 counts down by dt (to 0), else the speed v =
// (o->952 / record +16 when o->972 has 0x40000000, else o->948 / +12; the
// o field when > 0) * o->944 (record = o->556 + 28 * o->892); move start /
// stop (o->888 zero vs v zero) set the effect o->356 to 0x557CF0A5 /
// 0x557AF4A8; o->888 = v; when moving o->900 = clamp(o->900 + v * dt, 0,
// 1) (c = 0x001D2364) and the pose is updated. Without a pause, reaching
// the record's +8 in the direction of o->944 calls 001D0450(o, 0), which
// updates the pose when it returns non-zero. The pose update (with a
// spline): WorldObj_SplinePointTangentAt(o->704, -, o + 912, o + 928, 0;
// o->900), Obj_ApplyPose_001D0E58(o). Returns 1. f1 as the original leaves
// it.
DECOMP(001D2370, Obj_PathTick1D2370) {
    const u32 o = arg<u32>(ctx, 0);
    const f64 dt = ctx.fpr[1];
    callf(ctx, kObjCheck1AEA50, {o}, {dt});
    if (static_cast<u32>(ctx.gpr[3]) == 0) {
        ret(ctx, 0);
        return;
    }
    u32 w = load<u32>(o + 972);
    if ((w & 0x04000000u) != 0) {
        ret(ctx, 1);
        return;
    }
    if ((w & 0x02000000u) == 0) {
        w |= 0x02000000u;
        const u32 g = load<u32>(o + 708);
        store<u32>(o + 972, w);
        if (g != 0) {
            const u32 f = PathGate(o, g, w);
            call(ctx, kPathSetActive, {o, f});
            w = load<u32>(o + 972);
        }
    }
    const u32 g = load<u32>(o + 708);
    if (g != 0) {
        const u32 old = (w >> 28) & 1;
        const u32 f = PathGate(o, g, w);
        if (old == 0 && f != 0) {
            call(ctx, kPathSetActive, {o, 1});
            call(ctx, kPathReset, {o});
        } else if (old != 0 && f == 0) {
            call(ctx, kPathSetActive, {o, 0});
            call(ctx, kPathReset, {o});
        }
    }
    bool upd = load<u32>(o + 804) != 0 || load<u32>(o + 792) != 0;
    bool step = false;  // 001D0450 returned non-zero
    if (load<u32>(o + 704) != 0 && (load<u32>(o + 972) & 0x08000000u) != 0) {
        constexpr u32 C = 0x001D2364u;
        const f64 c0 = Lf(C);
        const u32 w972 = load<u32>(o + 972);
        const u32 n = load<u32>(o + 892);
        const u32 rec = load<u32>(o + 556) + n * 28;
        f64 v = c0;
        const f64 pause = Lf(o + 896);
        if (pause > c0) {
            const f64 left = S(pause - dt);
            Sf(o + 896, left);
            if (left < c0) store<u32>(o + 896, 0);
        } else {
            f64 sp = (w972 & 0x40000000u) != 0 ? Lf(o + 952) : Lf(o + 948);
            if (!(sp > c0)) sp = (w972 & 0x40000000u) != 0 ? Lf(rec + 16) : Lf(rec + 12);
            v = S(sp * Lf(o + 944));
        }
        const f64 was = Lf(o + 888);
        ctx.fpr[1] = was;  // f1 as the original leaves it
        const u32 fx = load<u32>(o + 356);
        if (was == c0) {
            if (!(v == c0) && fx != 0) call(ctx, kEffectSetHash, {fx, 0x557CF0A5u});
        } else {
            if (v == c0 && fx != 0) call(ctx, kEffectSetHash, {fx, 0x557AF4A8u});
        }
        Sf(o + 888, v);
        if (v != c0) {
            const f64 x = Fmadds(v, dt, Lf(o + 900));
            ctx.fpr[1] = x;  // f1 as the original leaves it
            const f64 hi = Lf(C + 4);
            const f64 m = x < hi ? x : hi;
            Sf(o + 900, m > c0 ? m : c0);
            upd = true;
        }
        const f64 p2 = Lf(o + 896);
        ctx.fpr[1] = p2;  // f1 as the original leaves it
        if (p2 < c0 || p2 == c0) {
            const f64 t = Lf(o + 900), end = Lf(rec + 8);
            bool hit = t == end;
            if (!hit) {
                const f64 dir = Lf(o + 944);
                ctx.fpr[1] = dir;  // f1 as the original leaves it
                hit = (dir > c0 && t > end) || (dir < c0 && t < end);
            }
            if (hit) step = static_cast<u32>(call(ctx, kPathStep, {o, 0})) != 0;
        }
    }
    if ((upd || step) && load<u32>(o + 704) != 0) {
        // r4 is left over (the callee does not read it)
        callf(ctx, kSplinePointTangentAt, {load<u32>(o + 704), ctx.gpr[4], o + 912, o + 928, 0}, {Lf(o + 900)});
        call(ctx, kApplyPose, {o});
    }
    ret(ctx, 1);
}

namespace {
constexpr u32 kHandleTable = 0x00F64C38u;       // Table* (handles)
constexpr u32 kHandleRelease = 0x002F20A4u;     // Handle_Release_002F20A4(table, h)
constexpr u32 kWorld = 0x00F65CA0u;             // World* (+4216 the player, +0x1281E0 the layer owner)
constexpr u32 kSetRef = 0x00121F10u;            // SetRef_00121F10(o, p)
constexpr u32 kFlag13Off = 0x0048F298u;         // Obj_Flag13Off_0048F298(o, 1)
constexpr u32 kStopEffects = 0x0003F260u;       // Obj_StopEffects3F260(o)
constexpr u32 kPopLayer = 0x000FBB88u;          // Obj_PopLayer(owner, Animator*; f32 t)
constexpr u32 kNotify24 = 0x000D949Cu;          // Obj_Notify24(player) -> p
constexpr u32 kFlaggedSetOn = 0x0029F428u;      // Flagged_SetOn_0029F428(-, u8 on)
constexpr u32 kObjReset12928 = 0x00012928u;     // the base reset (o)

// With a held object o->256: unless o->vfn[124](), its vfn[40](0); o->256 =
// 0, o->48 vfn[96](0) and the u16 o->48->164->40 gets 0x400 set and 0x1000
// cleared.
inline void DropHeld(PPUContext& ctx, u32 o) {
    if (load<u32>(o + 256) == 0) return;
    if (static_cast<u32>(VCall(ctx, o, 124, {o})) == 0) {
        const u32 p = load<u32>(o + 256);
        VCall(ctx, p, 40, {p, 0});
    }
    const u32 q = load<u32>(o + 48);
    store<u32>(o + 256, 0);
    VCall(ctx, q, 96, {q, 0});
    const u32 s = load<u32>(load<u32>(o + 48) + 164);
    if (s != 0) store<u16>(s + 40, static_cast<u16>((load<u16>(s + 40) & ~0x1000) | 0x400));
}

// Releases the virtual object in *slot (vfn[k]()) and clears the slot.
inline void DropV(PPUContext& ctx, u32 slot, u32 k) {
    const u32 x = load<u32>(slot);
    if (x == 0) return;
    VCall(ctx, x, k, {x});
    store<u32>(slot, 0);
}

// The player (world +4216) when it has bit 2 in +192, else 0.
inline u32 PlayerIf2() {
    const u32 p = load<u32>(load<u32>(kWorld) + 4208 + 8);
    return p != 0 && (load<u32>(p + 192) & 2) != 0 ? p : 0u;
}
}  // namespace

// u32 Obj_Reset3F4E0(Obj* o) (vtable entry): releases the handles o->544 /
// o->548 (Handle_Release_002F20A4(*0x00F64C38, h), u16 h+2 = 0); with a
// held object o->256: unless o->vfn[124](), its vfn[40](0); o->256 = 0,
// o->48 vfn[96](0) and the u16 o->48->164->40 gets 0x400 set and 0x1000
// cleared. A player with bit 2 (and 8) in +192 gets SetRef_00121F10(p, 0).
// Obj_Flag13Off_0048F298(o->48, 1); o->644 vfn[4](), o->656 / o->660
// vfn[8]() (each cleared); o->696 = 0; the o->316 112-byte entries o->308[]
// drop +68 (vfn[8]()) and +80 (Effect_SetHash4CF988(e, 0x10CBB), +136 |=
// 0x200); o->316 = 0, Obj_StopEffects3F260(o); o->620 is popped from the
// layer owner (world +0x1281E0, Obj_PopLayer(owner, it; *(f32*)0x0003F4DC))
// and released (vfn[4]()); the player with bit 2 gets Obj_Notify24 -> d
// with bit 4 of the byte d->2338 cleared and Flagged_SetOn_0029F428(d, 0).
// Returns 00012928(o).
DECOMP(0003F4E0, Obj_Reset3F4E0) {
    const u32 o = arg<u32>(ctx, 0);
    for (u32 i = 0; i < 2; ++i) {
        const u32 h = o + 544 + 4 * i;
        call(ctx, kHandleRelease, {load<u32>(kHandleTable), load<u32>(h)});
        store<u16>(h + 2, 0);
    }
    DropHeld(ctx, o);
    {
        const u32 p = PlayerIf2();
        if (p != 0 && (load<u32>(p + 192) & 8) != 0) call(ctx, kSetRef, {p, 0});
    }
    call(ctx, kFlag13Off, {load<u32>(o + 48), 1});
    DropV(ctx, o + 644, 4);
    DropV(ctx, o + 656, 8);
    DropV(ctx, o + 660, 8);
    store<u32>(o + 696, 0);
    for (u32 i = 0; i < load<u16>(o + 316); ++i) {
        const u32 e = load<u32>(o + 308) + 112 * i;
        DropV(ctx, e + 68, 8);
        const u32 fx = load<u32>(e + 80);
        if (fx != 0) {
            call(ctx, kEffectSetHash, {fx, 0x10CBB});
            const u32 f2 = load<u32>(e + 80);
            store<u32>(f2 + 136, load<u32>(f2 + 136) | 0x200);
            store<u32>(e + 80, 0);
        }
    }
    store<u16>(o + 316, 0);
    call(ctx, kStopEffects, {o});
    if (load<u32>(o + 620) != 0) {
        const u32 x = load<u32>(o + 620);
        const u32 own = load<u32>(load<u32>(kWorld) + 0x1281E0u);
        if (own != 0) callf(ctx, kPopLayer, {own, x}, {Lf(0x0003F4DCu)});
        DropV(ctx, o + 620, 4);
    }
    {
        const u32 p = PlayerIf2();
        if (p != 0) {
            const u32 d = static_cast<u32>(call(ctx, kNotify24, {p}));
            if (d != 0) {
                store<u8>(d + 2338, static_cast<u8>(load<u8>(d + 2338) & ~4));
                call(ctx, kFlaggedSetOn, {d, 0});
            }
        }
    }
    ctx.gpr[3] = call(ctx, kObjReset12928, {o});
}

namespace {
constexpr u32 kHandleResolveBit1 = 0x003568E0u;  // Handle_ResolveBit1(hash) -> Obj*
constexpr u32 kCallK2 = 0x000D94F4u;             // CallK2_000D94F4(o) -> p
constexpr u32 kAtan2f = 0x005E665Cu;             // atan2f(y; f1, x; f2)
}  // namespace

// Mover_Tick2E130(Mover* o; f32 dt) (vtable entry): o->36 += dt; nothing
// more while the object of handle 0x2508CAD1 has a +24 with bit 0x8000 in
// +164. Then by the mode hash o->4 (each with dt): 0x026F1EA5
// Obj_FaceVelocity_0002D8C8; 0x04102685 Mover_FollowSpline2D9B8; 0x04861257
// clears bit 0x40000000 of o->356 (and the byte +592 of a driver o->48 with
// bit 0x200 in +192), Mover_WobbleAway2D618; 0x7BF02FB1
// Mover_WobbleToward2DE70 then Mover_FacePlayer2DC20; 0x3F5F1D81 0002C518;
// 0x82EC0A56 Mover_Steer2CEB0; 0x809A1AA0 Heading_Jitter2C9A8; 0xD865EA8A
// Mover_FleeNearest2D130; 0xC66F66EF switches to 0x04102685 (o->vfn[220])
// when the player's y - the driver's y is within (k[1], k[2]) (k =
// 0x0002E124); 0x9B04E5C6 clears that o->356 bit and, when CallK2_000D94F4(
// driver), sets bit 0x40000000 of its +1268 and, for a driver d with bit
// 0x200 and d->616, d->480 = atan2f(normalized d->128 x, z) + d->656 (quirk
// kept: without the bit the null object's +616 / +480 are used). r3 / f1 as
// the original leaves them.
DECOMP(0002E130, Mover_Tick2E130) {
    const u32 o = arg<u32>(ctx, 0);
    const f64 dt = ctx.fpr[1];
    const f64 t36 = S(Lf(o + 36) + dt);
    Sf(o + 36, t36);
    ctx.fpr[1] = t36;  // f1 as the original leaves it
    if (static_cast<u32>(call(ctx, kHandleResolveBit1, {0x2508CAD1u})) != 0) {
        const u32 x = load<u32>(static_cast<u32>(call(ctx, kHandleResolveBit1, {0x2508CAD1u})) + 24);
        if (x != 0 && (load<u32>(x + 164) & 0x8000) != 0) {
            ctx.gpr[3] = 1;
            return;
        }
    }
    const u32 s = load<u32>(o + 4);
    ctx.gpr[3] = s;
    switch (s) {
    case 0x026F1EA5u: callf(ctx, 0x0002D8C8u, {o}, {dt}); break;  // Obj_FaceVelocity_0002D8C8
    case 0x04102685u: callf(ctx, 0x0002D9B8u, {o}, {dt}); break;  // Mover_FollowSpline2D9B8
    case 0x04861257u: {
        store<u32>(o + 356, load<u32>(o + 356) & ~0x40000000u);
        const u32 d = load<u32>(o + 48);
        if (d != 0 && (load<u32>(d + 192) & 0x200) != 0) store<u8>(d + 592, 0);
        callf(ctx, 0x0002D618u, {o}, {dt});  // Mover_WobbleAway2D618
        break;
    }
    case 0x7BF02FB1u:
        callf(ctx, 0x0002DE70u, {o}, {dt});  // Mover_WobbleToward2DE70
        callf(ctx, 0x0002DC20u, {o}, {dt});  // Mover_FacePlayer2DC20
        break;
    case 0x3F5F1D81u: callf(ctx, 0x0002C518u, {o}, {dt}); break;
    case 0x82EC0A56u: callf(ctx, 0x0002CEB0u, {o}, {dt}); break;  // Mover_Steer2CEB0
    case 0x809A1AA0u: callf(ctx, 0x0002C9A8u, {o}, {dt}); break;  // Heading_Jitter2C9A8
    case 0xD865EA8Au: callf(ctx, 0x0002D130u, {o}, {dt}); break;  // Mover_FleeNearest2D130
    case 0xC66F66EFu: {
        constexpr u32 K = 0x0002E124u;
        const u32 pl = load<u32>(load<u32>(kWorld) + 4208 + 8);
        if (pl == 0) {
            ctx.gpr[3] = 0;
            break;
        }
        const u32 dp = load<u32>(o + 48) + 144;
        const f64 dy = S(Lf(pl + 148) - Lf(dp + 4));
        ctx.fpr[1] = dy;  // f1 as the original leaves it
        if (!(dy > Lf(K + 4))) {
            ctx.gpr[3] = dp;
            break;
        }
        if (!(dy < Lf(K + 8))) {
            ctx.gpr[3] = K;
            break;
        }
        VCall(ctx, o, 220, {o, 0x04102685u});
        break;
    }
    case 0x9B04E5C6u: {
        store<u32>(o + 356, load<u32>(o + 356) & ~0x40000000u);
        if (static_cast<u32>(call(ctx, kCallK2, {load<u32>(o + 48)})) == 0) break;
        const u32 t = static_cast<u32>(call(ctx, kCallK2, {load<u32>(o + 48)}));
        store<u32>(t + 1268, load<u32>(t + 1268) | 0x40000000u);
        const u32 d = load<u32>(o + 48);
        const u32 q = d != 0 && (load<u32>(d + 192) & 0x200) != 0 ? d : 0u;  // quirk: may be 0
        ctx.gpr[3] = d;
        if (load<u32>(q + 616) == 0) break;
        const f64 x = Lf(d + 128), y = Lf(d + 132), z = Lf(d + 136);
        const f64 inv = Divs(Lf(0x0002E124u), Sqrts(Fmadds(z, z, Fmadds(x, x, S(y * y)))));
        const f64 r = callf(ctx, kAtan2f, {}, {S(x * inv), S(z * inv)});
        const f64 v = S(r + Lf(q + 656));
        Sf(q + 480, v);
        ctx.fpr[1] = v;  // f1 as the original leaves it
        break;
    }
    default: break;
    }
}

namespace {
constexpr u32 kAnimCall = 0x00276E4Cu;          // Obj_AnimCall3541E0_00276E4C(a, -, k; f32 x)
constexpr u32 kGrindStep = 0x003DABB8u;         // 003DABB8(o)
constexpr u32 kPlaceOnSpline = 0x003DAED0u;     // Obj_PlaceOnSpline3DAED0(o)
constexpr u32 kMsgInit13 = 0x00465384u;         // Msg_Init13_00465384(msg, o)
constexpr u32 kMsgInit14 = 0x004653BCu;         // Msg_Init14_004653BC(msg, o)
constexpr u32 kWorldQuery = 0x00482D14u;        // World_Query482D14(w, 0, msg)
constexpr u32 kFindNearest = 0x005A8CF8u;       // SplineInstance_FindNearest(pos, 0, f32* dist, 0, 0) -> spline
constexpr u32 kDetach = 0x000D72C0u;            // Obj_Detach0D72C0(holder, o)
// tuning values (f32 / u8 at +72)
constexpr u32 kTuneLimit = 0x010896A0u + 72;    // s32 limit for o->164
constexpr u32 kTuneRamp = 0x01089948u + 72;     // u8: ramp o->136 up
constexpr u32 kTuneRampTime = 0x010898F8u + 72; // f32
constexpr u32 kTuneRate = 0x01089290u + 72;     // f32 anim rate
constexpr u32 kTuneBlend = 0x01089060u + 72;    // f32 blend factor

// The 6-word message at m (zeroed), Init(m, o), World_Query482D14(w, 0, m).
inline void PostMsg(PPUContext& ctx, u32 m, u32 o, u32 init, bool own_world) {
    for (u32 k = 0; k < 6; ++k) store<u32>(m + 4 * k, 0);
    call(ctx, init, {m, o});
    const u32 w = own_world ? static_cast<u32>(VCall(ctx, o, 76, {o})) : load<u32>(kWorld);
    call(ctx, kWorldQuery, {w, 0, m});
}

// Copies the raw word at s to d with the sign bit flipped (lfs / fneg / stfs).
inline void NegW(u32 d, u32 s) { store<u32>(d, load<u32>(s) ^ 0x80000000u); }
}  // namespace

// Obj_GrindTick3DB140(Obj* o; f32 dt) (called by Obj_OnEvent2_003DC860): when
// o->164 reaches the tuning limit, message 13 (Msg_Init13_00465384) goes to
// World_Query482D14(o->vfn[76](), 0, &m). byte o->177 = 1; with both parts
// o->64 / o->68, o->64 vfn[84](frame) with o->68's matrix (+96) mirrored
// (rows 0 and 2 negated, position + 4 * row 2). o->140 counts ticks; from
// the 5th: without a spline o->80 SplineInstance_FindNearest(o->68 + 144)
// within 5.0 (detaching o->68 from its holder o->68->336 when found) seeds
// o->112 / o->116 = 0.5 - 2 / len and o->104 / o->108 = clamp(that + 4 /
// len, 0, 1); with a spline o->104 / o->112 blend toward o->108 / o->116 by
// the tuning factor and Obj_PlaceOnSpline3DAED0. Then the countdown o->132
// (-= dt; optional ramp of o->136 to 1 by dt / time) plays both parts' anim
// (Obj_AnimCall3541E0_00276E4C(vfn[60](), -, 0; rate >= 0.01 ? rate : 0.01))
// or, when over, at 1.0 with o->136 ramping down (unless o->92 == 6). Then
// 003DABB8(o); once o->64's +348 is <= 0, message 14 to the world and byte
// o->176 = 1 (r3 = 1). Otherwise r3 = o->64, f1 = its +348.
DECOMP(003DB140, Obj_GrindTick3DB140) {
    const u32 o = arg<u32>(ctx, 0);
    f64 dt = ctx.fpr[1];
    constexpr u32 C = 0x003DB108u;
    Stack frame(ctx, 176);
    const u32 sp = frame.addr() - 112;  // the original's r1: locals at their original offsets
    if (!(load<s32>(o + 164) < load<s32>(kTuneLimit))) PostMsg(ctx, sp + 180, o, kMsgInit13, true);
    store<u8>(o + 177, 1);
    const u32 a0 = load<u32>(o + 64);
    if (a0 != 0 && load<u32>(o + 68) != 0) {
        const u32 m = load<u32>(o + 68) + 96, t = sp + 112;
        NegW(t + 0, m + 0);
        NegW(t + 4, m + 4);
        NegW(t + 8, m + 8);
        Cw(t + 16, m + 16);
        Cw(t + 20, m + 20);
        Cw(t + 24, m + 24);
        NegW(t + 32, m + 32);
        NegW(t + 36, m + 36);
        NegW(t + 40, m + 40);
        const f64 k = Lf(C + 32);
        Sf(t + 48, Fmadds(Lf(m + 32), k, Lf(m + 48)));
        Sf(t + 52, Fmadds(Lf(m + 36), k, Lf(m + 52)));
        Sf(t + 56, Fmadds(Lf(m + 40), k, Lf(m + 56)));
        ctx.fpr[1] = -Lf(m + 0);  // f1 as the original leaves it
        VCall(ctx, a0, 84, {a0, t});
    }
    const u32 n = load<u32>(o + 140) + 1;
    store<u32>(o + 140, n);
    if (static_cast<s32>(n) >= 5) {
        u32 s = load<u32>(o + 80);
        if (s == 0) {
            const u32 b = load<u32>(o + 68);
            s = static_cast<u32>(call(ctx, kFindNearest, {b + 144, 0, sp + 176, 0, 0}));
            store<u32>(o + 80, s);
            const f64 dist = Lf(sp + 176);
            ctx.fpr[1] = dist;  // f1 as the original leaves it
            if (dist > Lf(C + 36)) {
                s = 0;
                store<u32>(o + 80, 0);
            }
            if (s != 0) {
                const u32 b2 = load<u32>(o + 68);  // reloaded after the search
                const u32 hold = load<u32>(b2 + 336);
                if (hold != 0) {
                    call(ctx, kDetach, {hold, b2});
                    s = load<u32>(o + 80);
                }
                const f64 inv = Divs(Lf(C + 32), Lf(load<u32>(s + 208) + 120));
                const f64 h = Lf(C + 40);
                const f64 lo = Fnmsubs(inv, h, h);
                const f64 hi = S(lo + inv);
                Sf(o + 116, lo);
                Sf(o + 112, lo);
                const f64 c1 = S(Fsel15(S(Lf(C + 4) - hi), hi, Ld(C + 8)));
                const f64 c2 = S(Fsel15(c1, c1, Ld(C + 48)));
                Sf(o + 108, c2);
                Sf(o + 104, c2);
                ctx.fpr[1] = c2;  // f1 as the original leaves it
            }
        }
        if (s != 0) {
            const f64 k = Lf(kTuneBlend);
            const f64 w0 = S(Lf(C + 4) - k);
            const f64 x = S(Lf(o + 108) * k), y = S(Lf(o + 116) * k);
            const f64 p = Fmadds(Lf(o + 104), w0, x);
            const f64 q = Fmadds(Lf(o + 112), w0, y);
            Sf(o + 104, p);
            Sf(o + 112, q);
            ctx.fpr[1] = p;  // f1 as the original leaves it
            call(ctx, kPlaceOnSpline, {o, kTuneBlend - 72});  // r4 left holding the tuning record (not read)
        }
    }
    const f64 c0 = Lf(C);
    const f64 cd = Lf(o + 132);
    ctx.fpr[1] = cd;  // f1 as the original leaves it
    if (cd > c0) {
        const f64 left = S(cd - dt);
        const u8 ramp = load<u8>(kTuneRamp);
        Sf(o + 132, left);
        ctx.fpr[1] = left;  // f1 as the original leaves it
        if (ramp != 0) {
            const f64 v = S(Lf(o + 136) + Divs(dt, Lf(kTuneRampTime)));
            const f64 r = S(Fsel15(S(Lf(C + 4) - v), v, Ld(C + 8)));
            Sf(o + 136, r);
            ctx.fpr[1] = r;  // f1 as the original leaves it
        }
        const f64 g = Lf(kTuneRate);
        dt = S(Fsel15(S(g - Lf(C + 16)), g, Ld(C + 24)));
        const u32 b = load<u32>(o + 68);
        const u32 ab = static_cast<u32>(VCall(ctx, b, 60, {b}));
        callf(ctx, kAnimCall, {ab, ctx.gpr[4], 0}, {dt});  // r4 left over (not read)
        const u32 a = load<u32>(o + 64);
        const u32 aa = static_cast<u32>(VCall(ctx, a, 60, {a}));
        callf(ctx, kAnimCall, {aa, ctx.gpr[4], 0}, {dt});  // r4 left over (not read)
    } else {
        const f64 one = Lf(C + 4);
        const u32 b = load<u32>(o + 68);
        const u32 ab = static_cast<u32>(VCall(ctx, b, 60, {b}));
        callf(ctx, kAnimCall, {ab, C, 0}, {one});  // r4 left holding c (not read)
        const u32 a = load<u32>(o + 64);
        const u32 aa = static_cast<u32>(VCall(ctx, a, 60, {a}));
        callf(ctx, kAnimCall, {aa, ctx.gpr[4], 0}, {one});  // r4 left over (not read)
        const f64 v = Lf(o + 136);
        ctx.fpr[1] = v;  // f1 as the original leaves it
        if (v > c0 && load<s32>(o + 92) != 6) {
            const f64 r = S(Fsel15(S(v - Divs(dt, Lf(kTuneRampTime))), S(v - Divs(dt, Lf(kTuneRampTime))), Ld(C + 48)));
            Sf(o + 136, r);
            ctx.fpr[1] = r;  // f1 as the original leaves it
        }
    }
    call(ctx, kGrindStep, {o});
    const u32 a = load<u32>(o + 64);
    const f64 hp = Lf(a + 348);
    if (hp < c0 || hp == c0) {
        ctx.fpr[1] = hp;  // f1 as the original leaves it
        PostMsg(ctx, sp + 212, o, kMsgInit14, false);
        store<u8>(o + 176, 1);
        ctx.gpr[3] = 1;
        return;
    }
    ctx.gpr[3] = a;
    ctx.fpr[1] = hp;
}

namespace {
constexpr u32 kCosf = 0x007344E0u;  // Math_Cosf(; f1)
constexpr u32 kSinf = 0x007345F0u;  // Math_Sinf(; f1)

// |(x, y, z)| as the original computes it: sqrts(fma(z, z, fma(x, x, y * y))).
inline f64 Len3(f64 x, f64 y, f64 z) { return Sqrts(Fmadds(z, z, Fmadds(x, x, S(y * y)))); }
}  // namespace

// Mover_CirclePlayer2C518(Mover* o; f32 dt) (called by Mover_Tick2E130, mode
// 0x3F5F1D81): the nearest of the world's players (world +29 slots,
// 0x20C80 apart, player at +4216) to the driver o->48 (+144) gives the
// orbit point player pos + o->320 * (cos(o->324) * row2 + sin(o->324) * row0)
// + sin(o->328) * k[1] * (row0.x, row0.z) (k = 0x0002C4F8; rows +96 /
// +128); when the driver is farther than k[2] in xz, its heading toward it
// atan2f(dir) goes to the driver (d->484, d->548 = 1.0 when d has bit 0x200
// in +192). With o->356 bit 0x20000000 (cleared here) and a velocity o->336
// longer than k[2], the velocity is normalized and a random unit offset
// (three steps of the generator at 0x01164888, scaled by k[4..6], weight
// k[7]) gives another heading to d->484 (quirk kept: d may be null). Always
// o->328 += dt. r3 as the original leaves it; f1 = o->328.
DECOMP(0002C518, Mover_CirclePlayer2C518) {
    const u32 o = arg<u32>(ctx, 0);
    const f64 dt = ctx.fpr[1];
    constexpr u32 K = 0x0002C4F8u;
    const f64 k0 = Lf(K);
    const u32 G = load<u32>(kWorld);
    const u32 n = load<u8>(G + 29);
    ctx.gpr[3] = n;
    u32 best = 0;
    f64 bd = k0;
    for (u32 i = 0; i < n; ++i) {
        const u32 p = load<u32>(G + i * 0x20C80u + 4208 + 8);
        if (p == 0) continue;
        const u32 q = load<u32>(o + 48) + 144;
        const f64 dy = S(Lf(q + 4) - Lf(p + 148)), dx = S(Lf(q) - Lf(p + 144)), dz = S(Lf(q + 8) - Lf(p + 152));
        const f64 d = Len3(dx, dy, dz);
        if (best == 0) {
            best = p;
            bd = d;
        } else if (d < bd) {
            bd = d;
            best = p;
        }
    }
    (void)bd;
    if (best != 0) {
        const u32 p = best;
        const f64 c = callf(ctx, kCosf, {}, {Lf(o + 324)});
        const f64 r2x = Lf(p + 128), r2z = Lf(p + 136);
        const f64 s = callf(ctx, kSinf, {}, {Lf(o + 324)});
        const f64 r0x = Lf(p + 96), r0z = Lf(p + 104);
        const f64 ax = Fmadds(r2x, c, S(r0x * s)), az = Fmadds(r2z, c, S(r0z * s));
        const f64 k1 = Lf(K + 4);
        const f64 rad = Lf(o + 320);
        const f64 wx = S(r0x * k1), wz = S(r0z * k1);
        const f64 bx = Fmadds(ax, rad, Lf(p + 144)), bz = Fmadds(az, rad, Lf(p + 152));
        const f64 s2 = callf(ctx, kSinf, {}, {Lf(o + 328)});
        const f64 tx = Fmadds(wx, s2, bx), tz = Fmadds(wz, s2, bz);
        const u32 q = load<u32>(o + 48) + 144;
        ctx.gpr[3] = q;
        const f64 ex = S(tx - Lf(q)), ez = S(tz - Lf(q + 8));
        const f64 k2 = Lf(K + 8);
        const f64 len = Len3(ex, k0, ez);
        if (len > k2) {
            const f64 k3 = Lf(K + 12);
            const f64 inv = Divs(k3, len);
            const f64 h = callf(ctx, kAtan2f, {}, {S(ex * inv), S(ez * inv)});
            const u32 dr = load<u32>(o + 48);
            u32 d = 0;
            if (dr != 0 && (load<u32>(dr + 192) & 0x200) != 0) {
                d = dr;
                Sf(d + 484, h);
                store<u32>(d + 548, 0x3F800000u);
            }
            u32 w = load<u32>(o + 356);
            ctx.gpr[3] = w;
            if ((w & 0x20000000u) != 0) {
                const f64 vy = Lf(o + 340), vx = Lf(o + 336), vz = Lf(o + 344);
                w &= ~0x20000000u;
                store<u32>(o + 356, w);
                ctx.gpr[3] = w;
                const f64 vl = Len3(vx, vy, vz);
                if (vl > k2) {
                    const f64 vi = Divs(k3, vl);
                    // three steps of the inlined generator
                    const u64 old = load<u64>(kRand15);
                    const u32 m1 = RandMix15(load<u32>(kRand15 + 8));
                    const u32 m2 = RandMix15(std::rotl(m1, 16));
                    const u32 m3 = RandMix15(std::rotl(m2, 16));
                    const f64 nx = S(vx * vi), ny = S(vy * vi), nz = S(vz * vi);
                    Sf(o + 336, nx);
                    Sf(o + 340, ny);
                    Sf(o + 344, nz);
                    const f64 k4 = Lf(K + 16), k5 = Lf(K + 20), k6 = Lf(K + 24);
                    const f64 rx = Fmadds(S(RandF15(old) * k4), k5, k6);
                    const f64 ry = Fmadds(S(RandF15(m1 & 0xFFFFu) * k4), k5, k6);
                    const f64 rz = Fmadds(S(RandF15(m2 & 0xFFFFu) * k4), k5, k6);
                    const f64 i1 = Divs(k3, Len3(rx, ry, rz));
                    store<u32>(kRand15 + 8, std::rotl(m3, 16));
                    store<u64>(kRand15, m3 & 0xFFFFu);
                    const f64 ux = S(rx * i1), uy = S(ry * i1), uz = S(rz * i1);
                    const f64 i2 = Divs(k3, Len3(ux, uy, uz));
                    const f64 k7 = Lf(K + 28);
                    const f64 gy = Fmadds(S(uy * i2), k7, ny);
                    const f64 gz = Fmadds(S(uz * i2), k7, nz);
                    const f64 gx = Fmadds(S(ux * i2), k7, nx);
                    const f64 i3 = Divs(k3, Len3(gx, gy, gz));
                    const f64 h2 = callf(ctx, kAtan2f, {}, {S(gx * i3), S(gz * i3)});
                    Sf(d + 484, h2);  // quirk: d may be 0 (a write to 0x1E4)
                }
            }
        }
    }
    const f64 t = S(Lf(o + 328) + dt);
    Sf(o + 328, t);
    ctx.fpr[1] = t;
}

namespace {
constexpr u32 kNavIsValid = 0x0062E164u;    // NavQuery_IsValid62E164(q) -> bool
constexpr u32 kNavFinish = 0x0062E094u;     // NavQuery_Finish62E094(q)
constexpr u32 kNavRelease = 0x006C0D24u;    // 006C0D24(h) (navigation middleware)
constexpr u32 kAvoidList = 0x00F557F0u;     // List* (+24 count, +3732 entry pointers)
constexpr u32 kAvoidRemove = 0x00012544u;   // 00012544(list, o)
constexpr u32 kEffectDelete = 0x004D0280u;  // Effect_Delete(e)
}  // namespace

// u32 Obj_ResetBase12928(Obj* o) (called by Obj_Reset3F4E0,
// Obj_PopAll_0004B0D0, 000530B8, 0005BDC0, Obj_DropAttachments_00061E98):
// DropHeld(o); bits 0x04 / 0x10 of the byte o->304 cleared; the navigation
// query o+216 finished when valid; o->212 released (006C0D24) and cleared;
// every entry of the list *0x00F557F0 (+3732[], count +24) whose first word
// is the driver o->48 is swap-removed with the last one (count decremented),
// then 00012544(list, o); the driver's +244 = 0; the effect o->72 deleted and
// cleared. Returns 0.
DECOMP(00012928, Obj_ResetBase12928) {
    const u32 o = arg<u32>(ctx, 0);
    DropHeld(ctx, o);
    store<u8>(o + 304, static_cast<u8>(load<u8>(o + 304) & ~0x14));
    const u32 nq = o + 216;
    if (static_cast<u32>(call(ctx, kNavIsValid, {nq})) != 0) call(ctx, kNavFinish, {nq});
    if (load<u32>(o + 212) != 0) {
        call(ctx, kNavRelease, {load<u32>(o + 212)});
        store<u32>(o + 212, 0);
    }
    const u32 L = load<u32>(kAvoidList);
    const u32 d = load<u32>(o + 48);
    const u32 arr = L + 3732;
    for (s32 i = 0; i < load<s32>(L + 24); ++i) {
        if (load<u32>(load<u32>(arr + 4 * i)) != d) continue;
        const s32 n = load<s32>(L + 24) - 1;
        store<u32>(L + 24, static_cast<u32>(n));
        const u32 a = load<u32>(arr + 4 * i);
        const u32 b = load<u32>(arr + 4 * static_cast<u32>(n));
        store<u32>(arr + 4 * static_cast<u32>(n), a);
        store<u32>(arr + 4 * i, b);
        --i;
    }
    call(ctx, kAvoidRemove, {load<u32>(kAvoidList), o});
    const u32 d2 = load<u32>(o + 48);
    const u32 fx = load<u32>(o + 72);
    if (d2 != 0) store<u32>(d2 + 244, 0);
    if (fx != 0) {
        call(ctx, kEffectDelete, {fx});
        store<u32>(o + 72, 0);
    }
    ctx.gpr[3] = 0;
}

namespace {
constexpr u32 kSmallAlloc = 0x0055238Cu;        // SmallAlloc(size)
constexpr u32 kCamPushTarget = 0x000FC040u;     // Camera_PushTarget0FC040(cam, target, -, u8 prio; f32 blend)
constexpr u32 kCamTargetVt = 0x00B13B38u;       // the fixed camera target's vtable
constexpr u32 kUpVec = 0x01164B60u;             // global up vector
// the tuning values used are the f32s at +72 of the records 0x010898A8,
// 0x01089808, 0x01089858, 0x010899E8, 0x01089998, 0x01089A38, 0x01089A88
}  // namespace

// Obj_GrindCamera3DABB8(Obj* o) (called by Obj_GrindTick3DB140): a camera
// frame for the pair o->64 (a, matrix +96, pos +144) / o->68 (b, pos +144):
// eye = (a.pos + b.pos) * 0.5 - row1 * h + row2 * t3 * (1 - o->136) (h =
// lerp of two tuning heights by o->136, t3 a tuning distance, the eye y
// raised by another value), target y = mid y * 0.5 + t4; forward = normalize(
// target - eye) (c = 0x003DABAC: 0.5, 1.0), side = normalize(up x forward),
// up' = forward x side. The frame rows (side, up', forward, eye) go to the
// camera target o->76; without one a 112-byte target is made (SmallAlloc;
// vtable 0x00B13B38, +4 = o->72, +80 a tuning blend, +88 bits 23..30 = 1,
// +84 / +92 / +96 / +100 = 0), stored in o->76 (0 when the allocation
// failed) and pushed with Camera_PushTarget0FC040(o->72, it, -, 0; tuning).
// r3 / f1 as the original leaves them.
DECOMP(003DABB8, Obj_GrindCamera3DABB8) {
    const u32 o = arg<u32>(ctx, 0);
    constexpr u32 K = 0x003DABACu;
    const u32 a = load<u32>(o + 64);
    const u32 m = a + 96, pa = a + 144, pb = load<u32>(o + 68) + 144;
    const f64 t2 = Lf(0x01089850u);
    const f64 h = Fmadds(S(Lf(0x010898F0u) - t2), Lf(o + 136), t2);
    const f64 k0 = Lf(K), k1 = Lf(K + 4);
    const f64 sy = S(Lf(pb + 4) + Lf(pa + 4));
    const f64 w = S(k1 - Lf(o + 136));
    const f64 sx = S(Lf(pb) + Lf(pa));
    const f64 t3 = Lf(0x010898A0u);
    const f64 sz = S(Lf(pb + 8) + Lf(pa + 8));
    const f64 r2y = S(Lf(m + 36) * t3), r2x = S(Lf(m + 32) * t3), r2z = S(Lf(m + 40) * t3);
    const f64 r1y = S(Lf(m + 4) * h), r1x = S(Lf(m) * h), r1z = S(Lf(m + 8) * h);
    const f64 t5 = Lf(0x010899E0u), t4 = Lf(0x01089A30u);
    const f64 ty = Fmadds(sy, k0, t4);
    const f64 ey0 = Fmadds(r2y, w, Fmsubs(sy, k0, r1y));
    const f64 ex = Fmadds(r2x, w, Fmsubs(sx, k0, r1x));
    const f64 ez = Fmadds(r2z, w, Fmsubs(sz, k0, r1z));
    const f64 ey = S(ey0 + t5);
    f64 fx = Fmsubs(sx, k0, ex), fz = Fmsubs(sz, k0, ez), fy = S(ty - ey);
    const f64 i1 = Divs(k1, Sqrts(Fmadds(fz, fz, Fmadds(fx, fx, S(fy * fy)))));
    fz = S(fz * i1);
    fx = S(fx * i1);
    fy = S(fy * i1);
    const f64 ux = Lf(kUpVec), uz = Lf(kUpVec + 8), uy = Lf(kUpVec + 4);
    f64 sdy = Fmsubs(uz, fx, S(ux * fz));
    f64 sdz = Fmsubs(ux, fy, S(uy * fx));
    f64 sdx = Fmsubs(uy, fz, S(uz * fy));
    const f64 i2 = Divs(k1, Sqrts(Fmadds(sdz, sdz, Fmadds(sdx, sdx, S(sdy * sdy)))));
    sdx = S(sdx * i2);
    sdy = S(sdy * i2);
    sdz = S(sdz * i2);
    const f64 vz = Fmsubs(fx, sdy, S(fy * sdx));
    const f64 vx = Fmsubs(fy, sdz, S(fz * sdy));
    const f64 vy = Fmsubs(fz, sdx, S(fx * sdz));
    const f64 rows[12] = {sdx, sdy, sdz, vx, vy, vz, fx, fy, fz, ex, ey, ez};
    const u32 c = load<u32>(o + 76);
    if (c != 0) {
        for (u32 k = 0; k < 12; ++k) Sf(c + 16 + MOff16(k), rows[k]);
        ctx.gpr[3] = c;
        ctx.fpr[1] = i2;  // f1 as the original leaves it
        return;
    }
    ctx.fpr[1] = i2;  // f1 as the original leaves it
    u32 p = static_cast<u32>(call(ctx, kSmallAlloc, {112}));
    if (p != 0) {
        const u32 g = load<u32>(o + 72);
        const u32 w88 = load<u32>(p + 88);
        const f64 blend = Lf(0x01089A80u);
        store<u8>(p + 8, 0);
        store<u32>(p + 4, g);
        store<u32>(p + 0, kCamTargetVt);
        for (u32 k = 0; k < 12; ++k) Sf(p + 16 + MOff16(k), rows[k]);
        Sf(p + 80, blend);
        store<u32>(p + 92, 0);
        store<u32>(p + 88, (w88 & 0x007FFFFFu) | 0x00800000u);
        store<u32>(p + 96, 0);
        store<u32>(p + 100, 0);
        store<u32>(p + 84, 0);
    }
    const u32 cam = load<u32>(o + 72);
    store<u32>(o + 76, p);
    callf(ctx, kCamPushTarget, {cam, p, cam, 0}, {Lf(0x01089AD0u)});  // r5 left holding the camera (not read)
}

// Obj_SetControlled12B78(Obj* o, u8 on) (called by Actor_SetCtl_0002F814):
// with a body o->68: Obj_ResetPos_0026D9D4(it), then its physics follows
// bit 0 of the byte o->305 when on (Obj_SetPhysicsActive(b, bit) unless
// GetFlag_002697F8(b) already equals it) or is switched off. On: o->vfn[224]().
// Off: the driver o->48 vfn[128](0x1D6F9964, 0), o->52 = 0, DropHeld(o),
// o->vfn[232](). r3 as the last virtual call leaves it.
DECOMP(00012B78, Obj_SetControlled12B78) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 on = arg<u32>(ctx, 1) & 0xFF;
    if (load<u32>(o + 68) != 0) {
        call(ctx, 0x0026D9D4u, {load<u32>(o + 68)});  // Obj_ResetPos_0026D9D4
        if (on != 0) {
            const u32 f = static_cast<u32>(call(ctx, 0x002697F8u, {load<u32>(o + 68)}));  // GetFlag_002697F8
            const u32 want = load<u8>(o + 305) & 1;
            if (f != want) call(ctx, 0x00260C14u, {load<u32>(o + 68), want});  // Obj_SetPhysicsActive
        } else {
            call(ctx, 0x00260C14u, {load<u32>(o + 68), 0});  // Obj_SetPhysicsActive
        }
    }
    if (on != 0) {
        VCall(ctx, o, 224, {o});
        return;
    }
    const u32 d = load<u32>(o + 48);
    VCall(ctx, d, 128, {d, 0x1D6F9964u, 0});
    store<u32>(o + 52, 0);
    DropHeld(ctx, o);
    VCall(ctx, o, 232, {o});
}

// Obj_ReleaseFx1EAE88(Obj* o) (vtable entry): o->448 = 0; the layer o->424
// is popped from the layer owner (world +0x1281E0, not null-checked;
// Obj_PopLayer(owner, it; *(f32*)0x001EAE84)) and released (vfn[4]()); the
// body o->428 gets vfn[88](1, 1, 1, 1) and is cleared; o->440 vfn[40](0, 0);
// the parts o->432 / o->436 / o->444 of kind 8 ((+4 >> 10) & 0x3F) get
// vfn[52](255, 0). r3 as the original leaves it.
DECOMP(001EAE88, Obj_ReleaseFx1EAE88) {
    const u32 o = arg<u32>(ctx, 0);
    store<u32>(o + 448, 0);
    if (load<u32>(o + 424) != 0) {
        const u32 x = load<u32>(o + 424);
        callf(ctx, kPopLayer, {load<u32>(load<u32>(kWorld) + 0x1281E0u), x}, {Lf(0x001EAE84u)});
        DropV(ctx, o + 424, 4);
    }
    const u32 y = load<u32>(o + 428);
    if (y != 0) {
        VCall(ctx, y, 88, {y, 1, 1, 1, 1});
        store<u32>(o + 428, 0);
    }
    const u32 z = load<u32>(o + 440);
    if (z != 0) VCall(ctx, z, 40, {z, 0, 0});
    for (u32 slot : {432u, 436u, 444u}) {
        const u32 e = load<u32>(o + slot);
        if (slot == 436) ctx.gpr[3] = e;  // (o->444 is loaded into r31)
        if (e == 0) continue;
        const u32 kind = (load<u32>(e + 4) >> 10) & 0x3F;
        if (kind != 8) {
            if (slot == 444) ctx.gpr[3] = kind;  // as the original leaves r3
            continue;
        }
        VCall(ctx, e, 52, {e, 255, 0});
    }
}

namespace {
constexpr u32 kAnimSetState = 0x0030D2B8u;  // Anim_SetState_0030D2B8(ctl, state)
constexpr u32 kAnimPickState = 0x0030E1E0u; // 0030E1E0(ctl) -> state
constexpr u32 kGetU8_0 = 0x004A08C0u;       // GetU8_0_004A08C0(p) -> u8
}  // namespace

// u32 AnimTask_Update3129E8(Task* o) (vtable entry): 1 unless o->vfn[32](0)
// is 1. With the controller c = o->32: when GetU8_0_004A08C0(c + 556), state
// 4 (Anim_SetState_0030D2B8(c, 4), o->vfn[28](0, 2); 0). While the byte
// o->64 is set, waits (1) for the track c->660 (+92 zero, u16 +60 <= o->68,
// u16 +80 zero) and then clears it; waits (1) while o->72 differs from
// *(f32*)0x003129E0 and *(f32*)o->12 < o->72. Then a new state from
// 0030E1E0(c) different from o->vfn[80]() is set (as above, 0), else
// o->vfn[28](0, 1) and 1.
DECOMP(003129E8, AnimTask_Update3129E8) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 c0 = load<u32>(o + 32);
    const s32 k = static_cast<s32>(VCall(ctx, o, 32, {o, 0}));
    if (k != 1) {
        ret(ctx, 1);
        return;
    }
    const u32 u = static_cast<u32>(call(ctx, kGetU8_0, {c0 + 556}));
    const u32 tp = load<u32>(o + 12);
    if (u != 0) {
        call(ctx, kAnimSetState, {load<u32>(o + 32), 4});
        VCall(ctx, o, 28, {o, 0, 2});
        ret(ctx, 0);
        return;
    }
    const u32 c = load<u32>(o + 32);
    if (load<u8>(o + 64) != 0) {
        const u32 s = load<u32>(c + 660);
        const u32 n = load<u32>(o + 68);
        if (load<u32>(s + 92) != 0 || static_cast<u32>(load<u16>(s + 60)) > n || load<u16>(s + 80) != 0) {
            ret(ctx, 1);
            return;
        }
        store<u8>(o + 64, 0);
    }
    const f64 t = Lf(o + 72);
    ctx.fpr[1] = t;  // f1 as the original leaves it
    if (!(t == Lf(0x003129E0u)) && Lf(tp) < t) {
        ret(ctx, 1);
        return;
    }
    const u32 st = static_cast<u32>(call(ctx, kAnimPickState, {c}));
    const u32 cur = static_cast<u32>(VCall(ctx, o, 80, {o}));
    if (st != cur) {
        call(ctx, kAnimSetState, {load<u32>(o + 32), st});
        VCall(ctx, o, 28, {o, 0, 2});
        ret(ctx, 0);
        return;
    }
    VCall(ctx, o, 28, {o, 0, 1});
    ret(ctx, 1);
}

// Obj_Destruct36F1FC(Obj* o) (called by Delete_0036F3EC): the destructor:
// vtable 0x00B1F800, RefNode_Release(o->200), World_ListRemove_0046B7A4(
// world, 8, 0x00F379A0, o, 0), o->176 / o->180 / o->184 vfn[88](0); then the
// base vtable 0x00B1F678: the effect o->20 released (Effect_Release(e,
// 0x10CBB), +136 |= 0x200); the part o->24 (its +276 / +280 cleared when
// +276 is 0x7DF841DC) vfn[40](0, 1) and vfn[8](); o->28 vfn[4]() and
// cleared; returns VtB21B58_DtorTrivial_004A0F70(o + 48).
DECOMP(0036F1FC, Obj_Destruct36F1FC) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 rn = load<u32>(o + 200);
    store<u32>(o + 0, 0x00B1F800u);
    call(ctx, 0x004432DCu, {rn});  // RefNode_Release
    call(ctx, 0x0046B7A4u, {load<u32>(kWorld), 8, 0x00F379A0u, o, 0});  // World_ListRemove_0046B7A4
    for (u32 slot : {176u, 180u, 184u}) {
        const u32 x = load<u32>(o + slot);
        if (x != 0) VCall(ctx, x, 88, {x, 0});
    }
    const u32 e = load<u32>(o + 20);
    store<u32>(o + 0, 0x00B1F678u);
    if (e != 0) {
        call(ctx, 0x004CF3C4u, {e, 0x10CBB});  // Effect_Release
        const u32 e2 = load<u32>(o + 20);
        store<u32>(e2 + 136, load<u32>(e2 + 136) | 0x200);
    }
    const u32 y = load<u32>(o + 24);
    if (y != 0) {
        if (load<u32>(y + 276) == 0x7DF841DCu) {
            store<u32>(y + 276, 0);
            store<u32>(y + 280, 0);
        }
        VCall(ctx, y, 40, {y, 0, 1});
        const u32 y2 = load<u32>(o + 24);
        VCall(ctx, y2, 8, {y2});
    }
    DropV(ctx, o + 28, 4);
    ctx.gpr[3] = call(ctx, 0x004A0F70u, {o + 48});  // VtB21B58_DtorTrivial_004A0F70
}

// u32 Group_Reset0B7384(Group* o) (vtable entry): clears bit 0x80000000 of
// o->8; every member (+52, 8-byte entries, u16 count +60) with bit 0x40000
// in +172 gets vfn[128](0x879D1B7A, 1) and vfn[40](1); the u16 counts +60 /
// +20 / +40 / +100 = 0; the index list o+92 (u16 count +100, capacity +96,
// high-water +102) is refilled with 0 .. o->80 - 1 (PtrVector_Reserve to
// twice the capacity when full); the effect o->136 (when set) gets
// Obj_SetEffectHash_0039FF34(e, 0xA86F8ED1), Obj_Call4A71D0_0039FEE0(e, 0),
// 0039D32C(e, 0, 1, 0); o->140 / o->144 / o->148 = 0. r3 as the original
// leaves it.
DECOMP(000B7384, Group_Reset0B7384) {
    const u32 o = arg<u32>(ctx, 0);
    store<u32>(o + 8, load<u32>(o + 8) & 0x7FFFFFFFu);
    for (u32 i = 0; i < load<u16>(o + 60); ++i) {
        const u32 x = load<u32>(load<u32>(o + 52) + 8 * i);
        if (x == 0 || (load<u32>(x + 172) & 0x40000) == 0) continue;
        VCall(ctx, x, 128, {x, 0x879D1B7Au, 1});
        VCall(ctx, x, 40, {x, 1});
    }
    store<u16>(o + 60, 0);
    store<u16>(o + 20, 0);
    store<u16>(o + 40, 0);
    store<u16>(o + 100, 0);
    u32 cnt = 0;  // the count as held in a register
    for (u32 j = 0; j < load<u16>(o + 80); ++j) {
        const u32 cap = load<u16>(o + 96);
        if ((cnt & 0xFFFF) == cap) {
            call(ctx, 0x00551718u, {o + 92, (cap << 1) & 0x1FFFEu});  // PtrVector_Reserve
            cnt = load<u16>(o + 100);
        }
        store<u32>(load<u32>(o + 92) + ((cnt << 2) & 0x3FFFCu), j);
        const u32 n = load<u16>(o + 100) + 1u;
        const u32 mx = load<u16>(o + 102);
        store<u16>(o + 100, static_cast<u16>(n));
        store<u16>(o + 102, static_cast<u16>(static_cast<s32>(mx) > static_cast<s32>(n & 0xFFFF) ? mx : n));
        cnt = n;
    }
    const u32 e = load<u32>(o + 136);
    ctx.gpr[3] = e;
    if (e != 0) {
        call(ctx, 0x0039FF34u, {e, 0xA86F8ED1u});  // Obj_SetEffectHash_0039FF34
        call(ctx, 0x0039FEE0u, {load<u32>(o + 136), 0});  // Obj_Call4A71D0_0039FEE0
        call(ctx, 0x0039D32Cu, {load<u32>(o + 136), 0, 1, 0});
    }
    store<u32>(o + 140, 0);
    store<u32>(o + 144, 0);
    store<u32>(o + 148, 0);
}

// u32 Obj_StateTick228C40(Obj* o; f32 dt) (called by 00238F78, 00242B38,
// 0024BA28): 0 unless Obj_UpdateFx_000E0520(o; dt). While o->vfn[32](0) is
// 1: o->vfn[32](1) == 0 calls o->vfn[400]() and o->vfn[28](1, 1); 1.
// Otherwise: with the byte flag 0x01056E7C +72 set, the static object
// 0x01056E7C gets vfn[12](0) and o->vfn[352](-, 0, 0; *(f32*)0x00228C34)
// (this call sequence leaves the entry in r4 and the OPD in r7); the cooldown
// o->380 counts down by dt to k = *(f32*)0x00228C38 (only when above k); a
// pending flag 0x40000000 in o->384 runs o->vfn[404]() and is cleared; 1.
DECOMP(00228C40, Obj_StateTick228C40) {
    const u32 o = arg<u32>(ctx, 0);
    const f64 dt = ctx.fpr[1];
    callf(ctx, 0x000E0520u, {o}, {dt});  // Obj_UpdateFx_000E0520
    if (static_cast<u32>(ctx.gpr[3]) == 0) {
        ret(ctx, 0);
        return;
    }
    if (static_cast<s32>(VCall(ctx, o, 32, {o, 0})) == 1) {
        if (static_cast<u32>(VCall(ctx, o, 32, {o, 1})) == 0) {
            VCall(ctx, o, 400, {o});
            VCall(ctx, o, 28, {o, 1, 1});
        }
        ret(ctx, 1);
        return;
    }
    constexpr u32 kStatic = 0x01056E7Cu;
    if (load<u8>(kStatic + 72) != 0) {
        VCall(ctx, kStatic, 12, {kStatic, 0});
        const u32 opd = load<u32>(load<u32>(o) + 352);
        ctx.fpr[1] = Lf(0x00228C34u);
        call_ptr(ctx, opd, {o, load<u32>(opd), 0, 0, opd});
    }
    const f64 k = Lf(0x00228C38u);
    const f64 v = Lf(o + 380);
    ctx.fpr[1] = k;  // f1 as the original leaves it
    if (v > k) {
        const f64 t = S(v - dt);
        const f64 nv = t > k ? t : k;
        Sf(o + 380, nv);
        ctx.fpr[1] = nv;  // f1 as the original leaves it
    }
    if ((load<u32>(o + 384) & 0x40000000u) != 0) {
        VCall(ctx, o, 404, {o});
        store<u32>(o + 384, load<u32>(o + 384) & ~0x40000000u);
    }
    ret(ctx, 1);
}

// Spawner_Tick408B70(Spawner* o, -, u8 flag; f32 dt) (called by 00409238,
// 00409EF0): below the limit (o->200 < o->88->32) with the timer o->212 >=
// o->208 and the flag set: 004089F0(o; k) (k = *(f32*)0x00408B6C), o->208 =
// 2.0, o->212 = 0. Otherwise each child o->100[] (u16 count +108) is ticked
// with 00407130(c; t) (t = k for a child in state 1 when the flag is clear,
// else dt); children in state 13 / 14 (+112) are released (vfn[4]()) and
// removed (the later ones shift down). Then o->212 += dt. r3 as the original
// leaves it (the count or 0x40000000); f1 = o->212.
DECOMP(00408B70, Spawner_Tick408B70) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 flag = arg<u32>(ctx, 2) & 0xFF;
    const f64 dt = ctx.fpr[1];
    const f64 k = Lf(0x00408B6Cu);
    bool spawned = false;
    if (load<s32>(o + 200) < load<s32>(load<u32>(o + 88) + 32)) {
        const f64 t = Lf(o + 212), lim = Lf(o + 208);
        if ((t > lim || t == lim) && flag != 0) {
            callf(ctx, 0x004089F0u, {o}, {k});
            store<u32>(o + 208, 0x40000000u);
            store<u32>(o + 212, 0);
            ctx.gpr[3] = 0x40000000u;
            spawned = true;
        }
    }
    if (!spawned) {
        u32 i = 0;
        for (;;) {
            const u32 n = load<u16>(o + 108);
            ctx.gpr[3] = n;
            if (i >= n) break;
            const u32 c = load<u32>(load<u32>(o + 100) + 4 * i);
            if (flag == 0 && load<s32>(c + 112) == 1) callf(ctx, 0x00407130u, {c}, {k});
            else callf(ctx, 0x00407130u, {c}, {dt});
            const u32 e = load<u32>(load<u32>(o + 100) + 4 * i);
            const s32 st = load<s32>(e + 112);
            if (st != 13 && st != 14) {
                ++i;
                continue;
            }
            if (e != 0) VCall(ctx, e, 4, {e});
            u32 m = load<u16>(o + 108);
            if (static_cast<s32>(i) != static_cast<s32>(m - 1)) {
                for (u32 j = i & 0xFFFF; static_cast<s32>(j) < static_cast<s32>(m - 1); j = (j + 1) & 0xFFFF) {
                    const u32 arr = load<u32>(o + 100);
                    m = load<u16>(o + 108);
                    store<u32>(arr + 4 * j, load<u32>(arr + 4 * j + 4));
                }
            }
            store<u16>(o + 108, static_cast<u16>(m - 1));
        }
    }
    const f64 t2 = S(Lf(o + 212) + dt);
    Sf(o + 212, t2);
    ctx.fpr[1] = t2;
}

// EffectRule_Apply4CBEBC(Rule* a, Effect* e, Params* p) (vtable entry): with
// an effect e: the threshold t = a->44, or the value of the component
// Effect_FindComponent(e, a->40) when its vfn[32]() is 1 (vfn[20](); f1);
// when p->32 >= t, the value v = a->32 (or, with the byte a->48 set and a
// u16 count a->20, a random entry a->12[rand % count] from the generator at
// 0x01164888) is written to +132 of e, or with a key a->36 of the first
// child e->112[] (u16 count +120) whose +0->8 is the key (none: no write).
// r3 / f1 as the original leaves them.
DECOMP(004CBEBC, EffectRule_Apply4CBEBC) {
    const u32 a = arg<u32>(ctx, 0), e = arg<u32>(ctx, 1), p = arg<u32>(ctx, 2);
    if (e == 0) return;  // r3 = a
    f64 t = Lf(a + 44);
    const u32 h = load<u32>(a + 40);
    if (h != 0) {
        const u32 c = static_cast<u32>(call(ctx, 0x004BB4B4u, {e, h}));  // Effect_FindComponent
        if (c != 0 && static_cast<s32>(VCall(ctx, c, 32, {c})) == 1) {
            VCall(ctx, c, 20, {c});
            t = ctx.fpr[1];  // its f32 result
        }
    }
    const f64 pv = Lf(p + 32);
    ctx.fpr[1] = pv;  // f1 as the original leaves it
    if (!(pv > t || pv == t)) return;
    u32 v = load<u32>(a + 32);
    if (load<u8>(a + 48) != 0 && load<u16>(a + 20) != 0) {
        const u32 n = load<u16>(a + 20);
        const u32 s = load<u32>(kRand15 + 8);
        const u32 idx = s - (s / n) * n;
        const u32 m = RandMix15(s);
        store<u32>(kRand15 + 8, std::rotl(m, 16));
        store<u64>(kRand15, m & 0xFFFFu);
        v = load<u32>(load<u32>(a + 12) + idx * 4);
    }
    ctx.gpr[3] = v;
    u32 dst = e;
    const u32 key = load<u32>(a + 36);
    if (key != 0) {
        dst = 0;
        const s32 n = load<u16>(e + 120);
        for (s32 i = 0; i < n; ++i) {
            const u32 x = load<u32>(load<u32>(e + 112) + 4 * static_cast<u32>(i));
            if (x == 0) continue;
            const u32 y = load<u32>(x);
            if ((y != 0 ? load<u32>(y + 8) : 0u) == key) {
                dst = x;
                break;
            }
        }
    }
    if (dst != 0) store<u32>(dst + 132, v);
}

// Obj_SetTint0DA348(Obj* o, u8 a, u8 b, u8 c, u8 d) (called by
// Obj_SetByteAndRecord_0020B8DC): the part o->160 of kind 8 ((+4 >> 10) &
// 0x3F) gets vfn[52](a, c), or vfn[52](a * part->168 / 255, c) when o->176
// already has 0x20 and d is 0, and o->176 |= 0x20; with b, each record of o->340
// (240-byte entries, u16 count +8) with an object +192 passes vfn[116](a, b,
// d, 0); each child of o->248 (u16 count +8) gets vfn[56](a, b). r3 as the
// original leaves it (o->248).
DECOMP(000DA348, Obj_SetTint0DA348) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 a = arg<u32>(ctx, 1) & 0xFF, b = arg<u32>(ctx, 2) & 0xFF, c = arg<u32>(ctx, 3) & 0xFF, d = arg<u32>(ctx, 4) & 0xFF;
    const u32 x = load<u32>(o + 160);
    if (x != 0 && ((load<u32>(x + 4) >> 10) & 0x3F) == 8) {
        if ((load<u16>(o + 176) & 0x20) != 0 && d == 0) {
            const u32 n = a * load<u8>(x + 168);
            const u32 v = static_cast<u32>((static_cast<u64>(n) * 0x80808081u) >> 32) >> 7;  // n / 255
            VCall(ctx, x, 52, {x, v & 0xFF, c});
        } else {
            VCall(ctx, x, 52, {x, a, c});
        }
        store<u16>(o + 176, static_cast<u16>(load<u16>(o + 176) | 0x20));
    }
    if (b != 0) {
        for (u32 i = 0;; ++i) {
            const u32 l = load<u32>(o + 340);
            if (i >= (l != 0 ? static_cast<u32>(load<u16>(l + 8)) : 0u)) break;
            const u32 y = load<u32>(load<u32>(l) + 240 * i + 192);
            if (y != 0) VCall(ctx, y, 116, {y, a, b, d, 0});
        }
    }
    for (u32 i = 0;; ++i) {
        const u32 l = load<u32>(o + 248);
        ctx.gpr[3] = l;
        if (i >= (l != 0 ? static_cast<u32>(load<u16>(l + 8)) : 0u)) break;
        const u32 z = load<u32>(load<u32>(l) + 4 * i);
        VCall(ctx, z, 56, {z, a, b});
    }
}

namespace {
// The root of a node chain (parent link +12).
inline u32 RootOf(u32 n) {
    while (load<u32>(n + 12) != 0) n = load<u32>(n + 12);
    return n;
}
}  // namespace

// Obj_OnContact48D30(Obj* o, Contact* c) (vtable entry): self = o->60
// vfn[144]() (0 without o->60); the contact's first body c->0 belongs to o
// when its root (+12 chain) has the s8 +24 == 1 and root + (s8)+16 is self:
// then the other body is c->4, else c->0 itself with the normal value
// *(f32*)(c->16 + 20) negated. Other body kind ((root +28) & 0x1F) 8 sets
// c->32 = 1 in states 13..14 (o->4); kinds 1 / 23 with |value| below
// *(f32*)0x00048D28 switch state 13 -> 14 and 3 -> 4 (o->vfn[220]). r3 / f1
// as the original leaves them.
DECOMP(00048D30, Obj_OnContact48D30) {
    const u32 o = arg<u32>(ctx, 0), c = arg<u32>(ctx, 1);
    u32 self = 0;
    const u32 x = load<u32>(o + 60);
    if (x != 0) self = static_cast<u32>(VCall(ctx, x, 144, {x}));
    f64 v = Lf(load<u32>(c + 16) + 20);
    const u32 ra = RootOf(load<u32>(c));
    u32 mine = 0;
    if (static_cast<s8>(load<u8>(ra + 24)) == 1) mine = ra + static_cast<u32>(static_cast<s32>(static_cast<s8>(load<u8>(ra + 16))));
    u32 rb;
    if (mine == self) {
        rb = RootOf(load<u32>(c + 4));
    } else {
        rb = RootOf(load<u32>(c));
        v = -v;
    }
    ctx.fpr[1] = v;  // f1 as the original leaves it
    ctx.gpr[3] = 0;  // the root walk leaves the null link
    const u32 k = load<u32>(rb + 28) & 0x1F;
    if (k == 8) {
        const s32 s = load<s32>(o + 4);
        ctx.gpr[3] = static_cast<u32>(s);
        if (s > 12 && s <= 14) {
            store<u32>(c + 32, 1);
            ctx.gpr[3] = 1;
        }
    }
    if (k != 1 && k != 23) return;
    const f64 av = std::fabs(v);
    ctx.fpr[1] = av;  // f1 as the original leaves it
    ctx.gpr[3] = 0x00048D28u;
    if (!(av < Lf(0x00048D28u))) return;
    const s32 s = load<s32>(o + 4);
    ctx.gpr[3] = static_cast<u32>(s);
    if (s > 3) {
        if (s == 13) VCall(ctx, o, 220, {o, 14});
    } else if (s > 2) {
        VCall(ctx, o, 220, {o, 4});
    }
}

namespace {
constexpr u32 kNodeGraph = 0x00F557F4u;  // Graph* (+4 -> u16 +2 node count, +12 20-byte nodes)

// The graph node with index i (s16), or 0 when out of range.
inline u32 GraphNode(u32 g, s32 i) {
    if (i < 0 || i >= static_cast<s32>(load<u16>(load<u32>(g + 4) + 2))) return 0;
    return load<u32>(g + 12) + static_cast<u32>(i) * 20;
}
}  // namespace

// Walker_Step1574C(Walker* o) (called by 00034598, 0003D0D8): with a node
// o->224: o->vfn[264](); the node's own graph entry (s16 +14) selects the
// key ((entry->0->12)[*(0x010AAFBC)->64]); key 0x8FA5D8C3 moves on with
// 000155E0(node); key 0x152D006A toggles the direction bit 0x02 of the byte
// o->304 at the ends (forward: +10 == +14; backward: +14 == +8 and +10 !=
// +8). Then backward goes to Node_FindRoot_000154AC(node), forward to the
// graph entry of +10 (0 when out of range). Quirk kept: an out-of-range own
// index reads the key through address 0. r3 as the original leaves it.
DECOMP(0001574C, Walker_Step1574C) {
    const u32 o = arg<u32>(ctx, 0);
    if (load<u32>(o + 224) == 0) {
        ret(ctx, 0);
        return;
    }
    VCall(ctx, o, 264, {o});
    u32 g = load<u32>(kNodeGraph);
    const u32 n = load<u32>(o + 224);
    const s32 idx = static_cast<s16>(load<u16>(n + 14));
    const u32 own = GraphNode(g, idx);
    const u32 key = load<u32>(load<u32>(load<u32>(own) + 12) + (load<u32>(load<u32>(0x010AAFBCu) + 64) << 2));
    if (key == 0x8FA5D8C3u) {
        store<u32>(o + 224, static_cast<u32>(call(ctx, 0x000155E0u, {n})));
        return;
    }
    if (key == 0x152D006Au) {
        const u32 b = load<u8>(o + 304);
        const u32 back = (b >> 1) & 1;
        const s32 a10 = static_cast<s16>(load<u16>(n + 10));
        bool flip;
        if (back == 0) flip = a10 - idx == 0;
        else flip = idx == static_cast<s16>(load<u16>(n + 8)) && a10 != static_cast<s16>(load<u16>(n + 8));
        if (flip) {
            store<u8>(o + 304, static_cast<u8>((b & ~2u) | (back == 0 ? 2u : 0u)));
            g = load<u32>(kNodeGraph);
        }
    }
    if ((load<u8>(o + 304) & 2) != 0) {
        store<u32>(o + 224, static_cast<u32>(call(ctx, 0x000154ACu, {n})));  // Node_FindRoot_000154AC
        return;
    }
    const s32 a10 = static_cast<s16>(load<u16>(n + 10));
    const u32 next = GraphNode(g, a10);
    ctx.gpr[3] = next != 0 ? load<u32>(g + 12) : g;  // as the original leaves r3
    store<u32>(o + 224, next);
}

// Obj_UpdateContacts1CE094(Obj* o) (called by 001CE2A8): unless the byte
// o->854 is set and o->vfn[32](0) is 2..4, 001CCF40(o, o + 853, o + 854)
// refreshes the two contact bytes; a change of +853 calls o->vfn[420](2 when
// it rose, else 6, 0), o->vfn[28](1, 1) and o->vfn[28](2, 0); otherwise,
// while +853 is set, a change of +854 calls o->vfn[420](2 / 6, 0). r3 as the
// original leaves it.
DECOMP(001CE094, Obj_UpdateContacts1CE094) {
    const u32 o = arg<u32>(ctx, 0);
    const s32 s = static_cast<s32>(VCall(ctx, o, 32, {o, 0}));
    const u32 p854 = load<u8>(o + 854), p853 = load<u8>(o + 853);
    if (p854 != 0 && (s == 2 || s == 3 || s == 4)) return;
    call(ctx, 0x001CCF40u, {o, o + 853, o + 854});
    const u32 n853 = load<u8>(o + 853), n854 = load<u8>(o + 854);
    ctx.gpr[3] = load<u32>(o);
    const bool rise853 = p853 == 0 && n853 != 0, fall853 = p853 != 0 && n853 == 0;
    const bool rise854 = p854 == 0 && n854 != 0, fall854 = p854 != 0 && n854 == 0;
    if (rise853 || fall853) {
        VCall(ctx, o, 420, {o, rise853 ? 2u : 6u, 0});
        VCall(ctx, o, 28, {o, 1, 1});
        VCall(ctx, o, 28, {o, 2, 0});
        return;
    }
    if (n853 == 0 || (!rise854 && !fall854)) return;
    VCall(ctx, o, 420, {o, rise854 ? 2u : 6u, 0});
}
