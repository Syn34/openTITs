// Game code, medium sweep part 22: releases, link activation and layer
// ticks.
#include "mid22.h"

using namespace sly4;
using namespace sly4::mid22;

namespace {
constexpr u32 kFindById = 0x000D6A28u;       // Obj_FindById(id)
constexpr u32 kTargetDef = 0x000D951Cu;      // Target_GetDef0D951C(o)
constexpr u32 kSetActive = 0x001EB814u;      // Obj_SetActive_001EB814(o, b)
constexpr u32 kFlush480 = 0x00129B58u;       // Obj_Flush480_00129B58(o)
constexpr u32 kEndEffect = 0x00129C18u;      // Obj_EndEffect_00129C18(o)
constexpr u32 kNotify24 = 0x000D949Cu;       // Obj_Notify24(o)
constexpr u32 kRemoveKey = 0x0029F7A0u;      // Records_RemoveKey_0029F7A0(o, key)
constexpr u32 kSectionMode = 0x00333BC8u;    // 0x00333BC8(o, 0, 1)
constexpr u32 kRegistryUnlink = 0x0048F298u;  // Registry_Unlink_0048F298(o, keep)
constexpr u32 kFindComponent = 0x004BB4B4u;  // Effect_FindComponent(e, hash)
constexpr u32 kSetHash = 0x004CF988u;        // Effect_SetHash4CF988(fx, hash)
constexpr u32 kBodyRemove = 0x00931F00u;     // Body_Remove931F00(world, body) (Havok)
constexpr u32 kAddEntity = 0x0092F2A8u;      // Hk_AddEntity92F2A8(world, e, 1) (Havok)
constexpr u32 kWorld = 0x00F65CA0u;          // World* g_World
}  // namespace

// Section_ReleaseAll334114(Section* o): when active (u8 o->2680):
// 0x00333BC8(o, 0, 1); every entry x of the u16 o->2640 list o->2632 and of
// the u16 o->2660 list o->2652, then the single o->2672: r = Obj_Notify24(
// o->12->8) (r4 = o->12 in the loops), Records_RemoveKey(r, x) when r, and
// x->vfn[40](x, 0); the counts and o->2672 are cleared. Then
// Registry_Unlink(o, 0) and o->2680 = 0. r3 as the original leaves it.
DECOMP(00334114, Section_ReleaseAll334114) {
    const u32 o = arg<u32>(ctx, 0);
    if (load<u8>(o + 2680) == 0) {
        ctx.gpr[3] = 0;
        return;
    }
    call(ctx, kSectionMode, {o, 0, 1});
    // r = Obj_Notify24(o->12->8), Records_RemoveKey(r, x), then the entry
    // (re-read through slot()) gets vfn[40](0).
    auto drop = [&](u32 x, auto slot, bool last) {
        const u32 q = load<u32>(o + 12);
        // r4: o->12 in the list loops, the previous call's leftover for o->2672
        const u32 r = static_cast<u32>(call(ctx, kNotify24, {load<u32>(q + 8), last ? ctx.gpr[4] : q}));
        if (r != 0) call(ctx, kRemoveKey, {r, x});
        const u32 y = load<u32>(slot());
        VCall(ctx, y, 40, {y, 0});
    };
    for (u32 k = 0; k < load<u16>(o + 2640); ++k)
        drop(load<u32>(load<u32>(o + 2632) + 4 * k), [&] { return load<u32>(o + 2632) + 4 * k; }, false);
    store<u16>(o + 2640, 0);
    for (u32 k = 0; k < load<u16>(o + 2660); ++k)
        drop(load<u32>(load<u32>(o + 2652) + 4 * k), [&] { return load<u32>(o + 2652) + 4 * k; }, false);
    const u32 x = load<u32>(o + 2672);
    store<u16>(o + 2660, 0);
    if (x != 0) {
        drop(x, [&] { return o + 2672; }, true);
        store<u32>(o + 2672, 0);
    }
    call(ctx, kRegistryUnlink, {o, 0});
    store<u8>(o + 2680, 0);
}

// Obj_ActivateLinks3F61DC(Obj* o): the flags o->81..83 are cleared; for the
// objects 0xB225C998 / 0x928F4335 / 0xBA681640 (Obj_FindById) enabled by
// o->84..86 whose target def (Target_GetDef) has +16 set:
// Obj_SetActive_001EB814(x, 0) and the flag o->81+k = 1. Then the world's
// (o->vfn[76]()) slot object +4216 (4208 + 8) with bit 0x08 of +192 gets
// Obj_Flush480 and Obj_EndEffect. r3 as the original leaves it.
DECOMP(003F61DC, Obj_ActivateLinks3F61DC) {
    const u32 o = arg<u32>(ctx, 0);
    static constexpr u32 kIds[3] = {0xB225C998u, 0x928F4335u, 0xBA681640u};
    store<u8>(o + 81, 0);
    store<u8>(o + 82, 0);
    store<u8>(o + 83, 0);
    for (u32 k = 0; k < 3; ++k) {
        if (call(ctx, kFindById, {kIds[k]}) == 0 || load<u8>(o + 84 + k) == 0) continue;
        const u32 x = static_cast<u32>(call(ctx, kFindById, {kIds[k]}));
        const u32 d = static_cast<u32>(call(ctx, kTargetDef, {x}));
        if (d == 0 || load<u8>(d + 16) == 0) continue;
        call(ctx, kSetActive, {x, 0});
        store<u8>(o + 81 + k, 1);
    }
    const u32 w = static_cast<u32>(VCall(ctx, o, 76, {o}));
    const u32 s = load<u32>(w + 4208 + 8);
    ctx.gpr[3] = w + 4208;
    if (s == 0) return;
    ctx.gpr[3] = 0;
    if ((load<u32>(s + 192) & 0x08u) == 0) return;
    call(ctx, kFlush480, {s});
    call(ctx, kEndEffect, {s});
}

// u32 Mode_Event32E560(Mode* o, a1, Event* ev): outside mode 4 (u8 o->18)
// the base handler 0x003252D8(o, a1, ev). For event 6 (ev->0): the cooldown
// o->2572 counts down by ev->24 (floor 0) and the current target c =
// 0x0032E038(o) differing from o->3364 is applied (non-zero): effect
// o->2224's component 0xEB336A2D (kind 2) +20 = u8 o->3368, its component
// 0x93116591 (kind 12 or 2) +20 = c->4->168, cue 0xDC175839; o->2572 =
// o->2576 and o->3364 = c (also for c == 0). r3 = 5 (or the base result).
DECOMP(0032E560, Mode_Event32E560) {
    const u32 o = arg<u32>(ctx, 0), a1 = arg<u32>(ctx, 1), ev = arg<u32>(ctx, 2);
    if (load<u8>(o + 18) != 4) {
        call(ctx, 0x003252D8u, {o, a1, ev});
        return;
    }
    if (load<u32>(ev) != 6) {
        ctx.gpr[3] = 5;
        return;
    }
    const u32 c = static_cast<u32>(call(ctx, 0x0032E038u, {o}));
    const f64 zero = Lf(0x0032E55Cu);  // 0.0f
    f64 cd = Lf(o + 2572);
    const u32 cur = load<u32>(o + 3364);
    const u32 e = load<u32>(o + 2224);
    ctx.fpr[1] = zero;
    if (cd > zero) {
        cd = S(cd - Lf(ev + 24));
        Sf(o + 2572, cd);
        if (cd < zero) store<u32>(o + 2572, 0);
    }
    if (c == cur) {
        ctx.gpr[3] = 5;
        return;
    }
    if (c != 0) {
        const u32 b = load<u8>(o + 3368);
        if (e != 0) {
            const u32 p = static_cast<u32>(call(ctx, kFindComponent, {e, 0xEB336A2Du}));
            if (p != 0 && static_cast<u32>(VCall(ctx, p, 32, {p})) == 2) store<u32>(p + 20, b);
        }
        const u32 x = load<u32>(load<u32>(c + 4) + 168);
        const u32 e2 = load<u32>(o + 2224);
        if (e2 != 0) {
            const u32 p = static_cast<u32>(call(ctx, kFindComponent, {e2, 0x93116591u}));
            if (p != 0) {
                if (static_cast<u32>(VCall(ctx, p, 32, {p})) == 12 || static_cast<u32>(VCall(ctx, p, 32, {p})) == 2)
                    store<u32>(p + 20, x);
            }
        }
        const u32 e3 = load<u32>(o + 2224);
        if (e3 != 0) call(ctx, kSetHash, {e3, 0xDC175839u});
        Cw(o + 2572, o + 2576);
        ctx.fpr[1] = Lf(o + 2576);
    }
    store<u32>(o + 3364, c);
    ctx.gpr[3] = 5;
}

// Obj_Layer2TickBFB78(Obj* o) vtable entry: layer-2 state s = o->vfn[32](o,
// 2). s == 1: when o->12->8 >= the layer's slot float (o->20 + u16 o->6):
// vfn[28](1, 6) and the slot's first byte = 1. s == 0: body o->640
// (vfn[304](0), its +416 record's Body_Remove(r->8, r)), entity o->644
// without +8 added to the world's Havok scene (*(g_World + 0x1281D4)->4),
// cue 0x7E5B6BC5 on o->356, the slot float = def o->216's +360 of the
// 48-byte entry o->724, vfn[28](2, 1). r3 / f1 as the original leaves them.
DECOMP(000BFB78, Obj_Layer2TickBFB78) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 def = load<u32>(o + 216);
    const s32 s = static_cast<s32>(VCall(ctx, o, 32, {o, 2}));
    if (s > 0) {
        if (s > 1) return;
        const u32 p = load<u32>(o + 20) + load<u16>(o + 6);
        ctx.fpr[1] = Lf(load<u32>(o + 12) + 8);
        ctx.gpr[3] = p;
        if (ctx.fpr[1] >= Lf(p)) {
            VCall(ctx, o, 28, {o, 1, 6});
            const u32 q = load<u32>(o + 20) + load<u16>(o + 6);
            store<u8>(q, 1);
            ctx.gpr[3] = q;
        }
        return;
    }
    if (s <= -1) return;
    const u32 b = load<u32>(o + 640);
    if (b != 0) {
        VCall(ctx, b, 304, {b, 0});
        const u32 r = load<u32>(load<u32>(o + 640) + 416);
        if (r != 0 && load<u32>(r + 8) != 0) call(ctx, kBodyRemove, {load<u32>(r + 8), r});
    }
    const u32 q = load<u32>(o + 644);
    if (q != 0 && load<u32>(q + 8) == 0)
        call(ctx, kAddEntity, {load<u32>(load<u32>(load<u32>(kWorld) + 0x1281D4u) + 4), q, 1});
    const u32 e = load<u32>(o + 356);
    if (e != 0) call(ctx, kSetHash, {e, 0x7E5B6BC5u});
    const u32 src = def + load<u32>(o + 724) * 48 + 316;
    const u32 dst = load<u32>(o + 20) + load<u16>(o + 6);
    ctx.fpr[1] = Lf(src + 44);
    Cw(dst, src + 44);
    VCall(ctx, o, 28, {o, 2, 1});
}
