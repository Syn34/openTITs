// Game code, part 47 (continued): more of the small-function sweep, 102
// instructions and up. Simulator-checked like obj47.cpp.
#include "obj47.h"

using namespace sly4;
using namespace sly4::obj47;

// ---------------------------------------------------------------- sweep: 102 instructions
// bool f(T* o, const Vec3* p): whether the navigation query o + 216 can
// reach p: a stack query (NavQuery_Towards6308AC(q, &copy of p, o + 216,
// 0x011AD1AC)); when valid its point (NavQuery_Point62D25C) minus p must
// have dy within the .text limits [0x00015C84, 0x00015C88] and dx^2 +
// dz^2 (+ the third constant squared) below the fourth. The query is freed
// (NavQuery_Free62D08C) on every path after it was built. f1 as left.
DECOMP(00015C98, Nav_CanReach_00015C98) {
    const u32 o = arg<u32>(ctx, 0), p = arg<u32>(ctx, 1);
    const u32 q = o + 216;
    if (static_cast<u32>(call(ctx, 0x0062E164u, {q})) == 0) {
        ret(ctx, 0u);
        return;
    }
    Stack fr(ctx, 32);  // sp+112 the query, sp+120 the target, sp+132 the point
    const u32 nq = fr.addr(), v = nq + 8, pt = nq + 20;
    Cp47(v, p);
    Cp47(v + 4, p + 4);
    Cp47(v + 8, p + 8);
    ctx.fpr[1] = Lfs47(p + 8);  // as the copy leaves it
    call(ctx, 0x006308ACu, {nq, v, q, 0x011AD1ACu});
    if (static_cast<u32>(call(ctx, 0x0062E164u, {nq})) == 0) {
        call(ctx, 0x0062D08Cu, {nq});
        ret(ctx, 0u);
        return;
    }
    call(ctx, 0x0062D25Cu, {pt, nq, v, 0});
    const f64 px = Lfs47(pt), py = Lfs47(pt + 4), pz = Lfs47(pt + 8);
    call(ctx, 0x0062E094u, {nq});
    constexpr u32 K = 0x00015C84u;
    const f64 dy = F45(py - Lfs47(p + 4));
    const f64 dx = F45(px - Lfs47(p));
    const f64 dz = F45(pz - Lfs47(p + 8));
    if (dy < Lfs47(K) || dy > Lfs47(K + 4)) {
        ctx.fpr[1] = dy;
        call(ctx, 0x0062D08Cu, {nq});
        ret(ctx, 0u);
        return;
    }
    f64 d = F45(Lfs47(K + 8) * Lfs47(K + 8));
    d = Fma45(dx, dx, d);
    d = Fma45(dz, dz, d);
    const u32 r = d < Lfs47(K + 12) ? 1u : 0u;
    ctx.fpr[1] = d;
    call(ctx, 0x0062D08Cu, {nq});
    ret(ctx, r);
}

// Obj* f(Group* o, const Vec3* p, -, flag (r6); f32 r): of the s32 o->+0
// members (pointers at +592) that pass vslot 120 and fail vslot 156 (and
// pass vslot 88 when the byte flag is set), the body (+48) whose position
// +144 is nearest to p and within r (squared distances, fmadds); r3 = it
// or 0, f1 = the last distance computed.
DECOMP(0001AE6C, Group_NearestMember_0001AE6C) {
    const u32 o = arg<u32>(ctx, 0), p = arg<u32>(ctx, 1), flag = arg<u32>(ctx, 3) & 0xFFu;
    const f64 r = farg(ctx, 0);
    f64 best2 = F45(r * r);
    u32 best = 0;
    for (s32 i = 0; i < static_cast<s32>(load<u32>(o)); ++i) {
        const u32 slot = o + 592 + 4 * static_cast<u32>(i);
        const u32 e = load<u32>(slot);
        if (static_cast<u32>(VCall(ctx, e, 120, {e})) == 0) continue;
        const u32 e2 = load<u32>(slot);
        if (static_cast<u32>(VCall(ctx, e2, 156, {e2})) != 0) continue;
        if (flag != 0) {
            const u32 e3 = load<u32>(slot);
            if (static_cast<u32>(VCall(ctx, e3, 88, {e3})) == 0) continue;
        }
        const u32 t = load<u32>(load<u32>(slot) + 48);
        const f64 dy = F45(Lfs47(p + 4) - Lfs47(t + 148));
        const f64 dx = F45(Lfs47(p) - Lfs47(t + 144));
        const f64 dz = F45(Lfs47(p + 8) - Lfs47(t + 152));
        f64 d = F45(dy * dy);
        d = Fma45(dx, dx, d);
        d = Fma45(dz, dz, d);
        ctx.fpr[1] = d;
        if (best2 > d) {
            best = t;
            best2 = d;
        }
    }
    ret(ctx, best);
}

// u32 f(Obj* o, u8 on): the effect o->+356. On: when not there yet,
// Effect_New(0x5135BEBC) to +356 (0 -> r3 = 0), its component 0x0027469E
// (Effect_FindComponent; vslot 32 == 5) bound to (o->+48)->+160 (the old
// binding released through its vslot 8, the new one's u16 +4 counted),
// +136 bit 0x200 cleared and 0x400 set, Effect_SetTime(e; f1 = the .text
// f32 0x00027E90) (r3 = its result; already there: r3 = it). Off: +136
// |= 0x200, &= ~0x400, Effect_SetHash4CF988(e, 0x879F2BDA), +356 = 0.
DECOMP(00027E98, Obj_SetEffect356_00027E98) {
    const u32 o = arg<u32>(ctx, 0), on = arg<u32>(ctx, 1) & 0xFFu;
    if (on == 0) {
        const u32 e = load<u32>(o + 356);
        if (e == 0) {
            ret(ctx, 0u);
            return;
        }
        store<u32>(e + 136, load<u32>(e + 136) | 0x200u);
        const u32 e2 = load<u32>(o + 356);
        const u32 w = load<u32>(e2 + 136) & ~0x400u;
        store<u32>(e2 + 136, w);
        call(ctx, 0x004CF988u, {load<u32>(o + 356), 0x879F2BDAu, w});  // r5 as the flag update leaves it
        store<u32>(o + 356, 0);
        return;
    }
    if (const u32 e0 = load<u32>(o + 356); e0 != 0) {
        ret(ctx, e0);
        return;
    }
    const u32 e = static_cast<u32>(call(ctx, 0x004CFB24u, {0x5135BEBCu}));
    store<u32>(o + 356, e);
    if (e == 0) {
        ret(ctx, 0u);
        return;
    }
    const u32 c = static_cast<u32>(call(ctx, 0x004BB4B4u, {e, 0x0027469Eu}));
    if (c != 0 && static_cast<s32>(VCall(ctx, c, 32, {c})) == 5) {
        const u32 v = load<u32>(load<u32>(o + 48) + 160);
        const u32 old = load<u32>(c + 28);
        if (old != 0) VCall(ctx, old, 8, {old});
        store<u32>(c + 28, v);
        if (v != 0) store<u16>(v + 4, static_cast<u16>(load<u16>(v + 4) + 1));
    }
    const u32 e2 = load<u32>(o + 356);
    store<u32>(e2 + 136, load<u32>(e2 + 136) & ~0x200u);
    const u32 e3 = load<u32>(o + 356);
    store<u32>(e3 + 136, load<u32>(e3 + 136) | 0x400u);
    // r4 = the constant's address, as the lfs leaves it
    callf(ctx, 0x004CF710u, {load<u32>(o + 356), 0x00027E90u}, {Lfs47(0x00027E90u)});
}

// u32 f(Obj* o): the camera target o->+420 for the player body t
// (*(*(0x00F65CA0) + 4216) when its +192 has bit 2, else 0; read at
// t->+148 unchecked). Without one and with state o->+4 0 / 1: a 224-byte
// CamTarget_Construct115B90(m, o->+416, t, o->+48, o->+48; f1 = t->+148)
// (SmallAlloc; 0 on failure) to +420 and Camera_PushTarget0FC040(o->+416,
// m, o->+416, 0; f1 = the .text f32 0x0005C840). With one:
// SetRef_00115B0C(cur, t), cur->+144 / +208 = t->+148; for other states
// Obj_PopLayer(o->+416, cur; f1 = the constant), cur->vslot 4(cur), +420 = 0.
DECOMP(0005C848, Obj_UpdateCamTarget_0005C848) {
    const u32 o = arg<u32>(ctx, 0);
    constexpr u32 kC = 0x0005C840u;
    const u32 t0 = load<u32>(load<u32>(0x00F65CA0u) + 4216);
    const u32 cur = load<u32>(o + 420), st = load<u32>(o + 4);
    const u32 t = (t0 != 0 && (load<u32>(t0 + 192) & 0x2u) != 0) ? t0 : 0u;
    if (cur == 0) {
        if (st != 0 && st != 1) {
            ret(ctx, 0u);
            return;
        }
        const u32 m = static_cast<u32>(call(ctx, 0x0055238Cu, {224}));
        u32 r = 0;
        if (m != 0) {
            callf(ctx, 0x00115B90u, {m, load<u32>(o + 416), t, load<u32>(o + 48), load<u32>(o + 48)}, {Lfs47(t + 148)});
            r = m;
        }
        store<u32>(o + 420, r);
        callf(ctx, 0x000FC040u, {load<u32>(o + 416), r, load<u32>(o + 416), 0}, {Lfs47(kC)});
        return;
    }
    call(ctx, 0x00115B0Cu, {cur, t});
    const u32 c2 = load<u32>(o + 420);
    const u32 f = load<u32>(t + 148);
    store<u32>(c2 + 144, f);
    store<u32>(c2 + 208, f);
    ctx.fpr[1] = Lfs47(t + 148);
    const u32 s2 = load<u32>(o + 4);
    if (s2 == 0 || s2 == 1) {
        ret(ctx, c2);
        return;
    }
    // r5 = the constant's address, as the lfs leaves it
    callf(ctx, 0x000FBB88u, {load<u32>(o + 416), c2, kC}, {Lfs47(kC)});
    const u32 x = load<u32>(o + 420);
    if (x != 0) VCall(ctx, x, 4, {x});
    else ret(ctx, 0u);
    store<u32>(o + 420, 0);
}

// u32 f(Cmd* c, X* x) (like Cmd_ApplyFlagBits_00075880): the target t
// (CompTarget34(x, c->+12), else Obj_FindById(c->+16)); its def d =
// Target_GetDef0D951C(t); none -> r3 = 0. With u8 +22 and +20 set and the
// player body (*(*(0x00F65CA0) + 4216)) having +192 bit 8,
// 0x0012D0F0(body, d->+8); then 0x002B72DC(d, body) when u8 +20 is set,
// else Obj_Release_002B719C(d); bit 0x10000000 of d->+124 = the low bit
// of u8 +21 (r3 = that byte).
DECOMP(0006B918, Cmd_AttachTarget_0006B918) {
    const u32 c = arg<u32>(ctx, 0), x = arg<u32>(ctx, 1);
    u32 t = 0;
    if (load<u32>(c + 12) != 0) t = CompTarget34(ctx, x, load<u32>(c + 12));
    if (t == 0) {
        const u32 id = load<u32>(c + 16);
        if (id != 0) t = static_cast<u32>(call(ctx, 0x000D6A28u, {id}));
    }
    if (t == 0) {
        ret(ctx, 0u);
        return;
    }
    const u32 d = static_cast<u32>(call(ctx, 0x000D951Cu, {t}));
    if (d == 0) {
        ret(ctx, 0u);
        return;
    }
    const u32 p = load<u32>(load<u32>(0x00F65CA0u) + 4216);
    if (load<u8>(c + 22) != 0 && load<u8>(c + 20) != 0 && p != 0 && (load<u32>(p + 192) & 0x8u) != 0)
        call(ctx, 0x0012D0F0u, {p, load<u32>(d + 8)});
    if (load<u8>(c + 20) != 0) call(ctx, 0x002B72DCu, {d, p});
    else call(ctx, 0x002B719Cu, {d});
    const u32 b = load<u8>(c + 21);
    store<u32>(d + 124, (load<u32>(d + 124) & ~0x10000000u) | ((b & 1u) << 28));
    ret(ctx, b);
}

// u32 f(Cmd* c, X* x): the object r: the target's +160 (CompTarget34(x,
// c->+12) unless u8 c->+21 is set, else Obj_FindById(c->+16)), or with no
// target and u8 +21 set Effect_Bind4BB7E0(x, c->+12, 1). Unless r is of
// kind 8 ((+4 >> 10) & 63; read before r's null check), nonzero, and its
// bit 0x80 of u8 +322 already equals u8 c->+20, that bit is set to it and
// r->vslot 36(r) called. r3 as the original leaves it.
DECOMP(0009084C, Cmd_SetFlag322_0009084C) {
    const u32 c = arg<u32>(ctx, 0), x = arg<u32>(ctx, 1);
    const u32 c12 = load<u32>(c + 12);
    u32 t = 0;
    if (c12 != 0 && load<u8>(c + 21) == 0) t = CompTarget34(ctx, x, c12);
    if (t == 0) {
        const u32 id = load<u32>(c + 16);
        if (id != 0) t = static_cast<u32>(call(ctx, 0x000D6A28u, {id}));
    }
    u32 r;
    if (t != 0) {
        r = load<u32>(t + 160);
        if (((load<u32>(r + 4) >> 10) & 0x3Fu) != 8) {
            ret(ctx, r);
            return;
        }
    } else {
        const u32 k = load<u32>(c + 12);
        if (k == 0) {
            ret(ctx, 0u);
            return;
        }
        if (load<u8>(c + 21) == 0) {
            ret(ctx, k);
            return;
        }
        r = static_cast<u32>(call(ctx, 0x004BB7E0u, {x, k, 1}));
        if (((load<u32>(r + 4) >> 10) & 0x3Fu) != 8) {
            ret(ctx, r);
            return;
        }
    }
    if (r == 0) {
        ret(ctx, 0u);
        return;
    }
    const u32 b = load<u8>(c + 20);
    const u32 w = load<u8>(r + 322);
    if (static_cast<s32>((w >> 7) & 1u) == static_cast<s32>(b)) {
        ret(ctx, r);
        return;
    }
    store<u8>(r + 322, static_cast<u8>((w & ~0x80u) | ((b & 1u) << 7)));
    VCall(ctx, r, 36, {r});
}

// bool f(Npc* o, a, b, u8 flag (r6), -, -, r9; f1, f2, f3; stack arg 9
// v): plays the line a (0 -> r3 = 0) on the speaker o->+164 (0 -> 0): the
// animator Anim_Find34CC34(spk, b) -> Speaker_Say350958(t, a, flag, ...,
// r9, 0; f1, f2 (the global 0x01021C38 when below the .text f32
// 0x000DBE70), that constant) and Obj_SetLastVoice_00350860(t, v); without
// one Speaker_Say3514AC(spk, a, flag, ..., r9, 0, v; same floats). With a
// track and f3 above the constant: Node_SetSignRec_000DBD7C(o, -, b; f1 =
// Track_Length353748(t) / f3). r3 = 1. (r4 / r6..r8 are passed as the
// previous call leaves them; the callees do not read them.)
DECOMP(000DBE78, Npc_Say_000DBE78) {
    const u32 o = arg<u32>(ctx, 0), a = arg<u32>(ctx, 1), b = arg<u32>(ctx, 2), flag = arg<u32>(ctx, 3) & 0xFFu;
    const u64 r9 = ctx.gpr[9];
    const u32 v = StkW47(ctx, 116);
    const f64 f1 = farg(ctx, 0), f3 = farg(ctx, 2);
    f64 f2 = farg(ctx, 1);
    const f64 k = Lfs47(0x000DBE70u);
    if (f2 < k) f2 = Lfs47(0x01021C38u);
    const u32 spk = load<u32>(o + 164);
    if (spk == 0 || a == 0) {
        ret(ctx, 0u);
        return;
    }
    u32 t = static_cast<u32>(call(ctx, 0x0034CC34u, {spk, b}));
    if (t != 0) {
        callf(ctx, 0x00350958u, {t, a, flag, ctx.gpr[6], ctx.gpr[7], ctx.gpr[8], r9, 0}, {f1, f2, k});
        call(ctx, 0x00350860u, {t, v});
    } else {
        callf(ctx, 0x003514ACu, {load<u32>(o + 164), a, flag, ctx.gpr[6], ctx.gpr[7], ctx.gpr[8], r9, 0, v}, {f1, f2, k});
        t = static_cast<u32>(ctx.gpr[3]);
    }
    if (t != 0 && f3 > k) {
        const f64 len = callf(ctx, 0x00353748u, {t}, {});
        callf(ctx, 0x000DBD7Cu, {o, ctx.gpr[4], b}, {F45(len / f3)});
    }
    ret(ctx, 1u);
}

// u32 f(Obj* o, -, Mtx* out): out = o's 4x3 rows +112/+128/+144/+160
// (row stride 16); a stack copy whose translation row becomes T - R v (v
// = Field_Get216_320v_001EB0BC(&v, o->+16), fmadds) is handed to (o->+16)
// ->vslot 84(.., &copy) (r3 = its result; f1 = the last component). The
// copy's fourth words are never written (stale stack in the original).
DECOMP(001126D4, Obj_WorldToLocalMtx_001126D4) {
    const u32 o = arg<u32>(ctx, 0), out = arg<u32>(ctx, 2);
    for (u32 r = 0; r < 4; ++r)
        for (u32 k = 0; k < 3; ++k) Cp47(out + 16 * r + 4 * k, o + 112 + 16 * r + 4 * k);
    Stack fr(ctx, 80);  // sp+112 v, sp+128 the 4x4 copy
    const u32 v = fr.addr(), m = v + 16;
    for (u32 r = 0; r < 4; ++r)
        for (u32 k = 0; k < 3; ++k) Cp47(m + 16 * r + 4 * k, o + 112 + 16 * r + 4 * k);
    call(ctx, 0x001EB0BCu, {v, load<u32>(o + 16)});
    const f64 nx = -Lfs47(v), ny = -Lfs47(v + 4), nz = -Lfs47(v + 8);
    f64 x = Lfs47(m + 48), y = Lfs47(m + 52), z = Lfs47(m + 56);
    x = Fma45(Lfs47(m + 0), nx, x);
    y = Fma45(Lfs47(m + 4), nx, y);
    z = Fma45(Lfs47(m + 8), nx, z);
    x = Fma45(Lfs47(m + 16), ny, x);
    y = Fma45(Lfs47(m + 20), ny, y);
    z = Fma45(Lfs47(m + 24), ny, z);
    x = Fma45(Lfs47(m + 32), nz, x);
    y = Fma45(Lfs47(m + 36), nz, y);
    z = Fma45(Lfs47(m + 40), nz, z);
    store<u32>(m + 48, W47(x));
    store<u32>(m + 52, W47(y));
    store<u32>(m + 56, W47(z));
    ctx.fpr[1] = z;
    const u32 s = load<u32>(o + 16);
    VCall(ctx, s, 84, {s, m});
}

// T::~T(T* o): vtables 0x00B14DE0 / +380 0x00B14F88 / +384 0x00B14FB0,
// List1013B48_Remove(o); when *(*(0x00F65CA0) + 0x001281F4) has +52 bit
// 0x40000000: the leader +616 (with +172 bit 0x40000) released
// (0x00191944(leader, o), +616 = 0 first) and each member of the
// u16-counted vector +596 (count +604) with +172 bit 0x40000 detached
// (+664 / +616 / +624 = 0; u16 +176 |= 0x1000 when u8 +672 is set). Then
// +664's +1312 = 0 and +664 = 0, PtrVector_Clear / Nop(+596), the base
// vtables 0x00B126B8 / 0x00B1200C / 0x00B12528 and VtB12550_Dtor_000DF934(o).
DECOMP(00191E54, VtB14DE0_Dtor_00191E54) {
    const u32 o = arg<u32>(ctx, 0);
    store<u32>(o, 0x00B14DE0u);
    store<u32>(o + 380, 0x00B14F88u);
    store<u32>(o + 384, 0x00B14FB0u);
    call(ctx, 0x000D68CCu, {o});
    const u32 q = load<u32>(load<u32>(0x00F65CA0u) + 0x001281F4u);
    if (q != 0 && (load<u32>(q + 52) & 0x40000000u) != 0) {
        const u32 x = load<u32>(o + 616);
        if (x != 0 && (load<u32>(x + 172) & 0x40000u) != 0) {
            store<u32>(o + 616, 0);
            call(ctx, 0x00191944u, {x, o});
        }
        for (u32 i = 0; i < load<u16>(o + 604); ++i) {
            const u32 e = load<u32>(load<u32>(o + 596) + 4 * i);
            if ((load<u32>(e + 172) & 0x40000u) == 0) continue;
            store<u32>(e + 664, 0);
            store<u32>(e + 616, 0);
            store<u32>(e + 624, 0);
            const u32 e2 = load<u32>(load<u32>(o + 596) + 4 * i);
            if (load<u8>(e2 + 672) != 0) store<u16>(e2 + 176, static_cast<u16>(load<u16>(e2 + 176) | 0x1000u));
        }
    }
    if (const u32 y = load<u32>(o + 664); y != 0) {
        store<u32>(y + 1312, 0);
        store<u32>(o + 664, 0);
    }
    call(ctx, 0x0054EC1Cu, {o + 596});
    call(ctx, 0x0054EC18u, {o + 596});
    store<u32>(o + 384, 0x00B12528u);
    store<u32>(o + 380, 0x00B1200Cu);
    store<u32>(o, 0x00B126B8u);
    call(ctx, 0x000DF934u, {o});
}

// T::T(T* o, const Src* s): Thunk_000C2B90(o, s), vtable 0x00B14FD8, the
// vec3 vector +140 (PtrVector_Construct, u16 +146 = 12,
// PtrVector_Init(+140, 16, 0, 1)), +160 / +164 / +208 = s->+96 / +100 /
// +144, vec3s +176 / +192 = s->+112 / +128, then s's vec3 array +76 (u16
// count +84) appended (grown with PtrVector_Reserve(+140, 2 * capacity)
// when full; the high-water u16 +150 raised). r3 = the last new count
// (32-bit), or the old count when nothing was added.
DECOMP(001A44E0, VtB14FD8_Construct_001A44E0) {
    const u32 o = arg<u32>(ctx, 0), s = arg<u32>(ctx, 1);
    call(ctx, 0x000C2B90u, {o, s});
    store<u32>(o, 0x00B14FD8u);
    call(ctx, 0x005516E0u, {o + 140});
    store<u16>(o + 146, 12);
    call(ctx, 0x00551598u, {o + 140, 16, 0, 1});
    Cp47(o + 176, s + 112);
    Cp47(o + 160, s + 96);
    Cp47(o + 180, s + 116);
    Cp47(o + 184, s + 120);
    Cp47(o + 164, s + 100);
    Cp47(o + 192, s + 128);
    Cp47(o + 196, s + 132);
    Cp47(o + 208, s + 144);
    Cp47(o + 200, s + 136);
    ctx.fpr[1] = Lfs47(s + 136);  // f1 as the copies leave it
    u32 r3 = load<u16>(o + 148);
    for (u32 i = 0; i < load<u16>(s + 84); ++i) {
        const u32 src = load<u32>(s + 76) + 12 * i;
        const u32 cap = load<u16>(o + 144);
        if (load<u16>(o + 148) == cap) call(ctx, 0x00551718u, {o + 140, (cap << 1) & 0x1FFFEu});
        const u32 d = load<u32>(o + 140) + 12 * static_cast<u32>(load<u16>(o + 148));
        Cp47(d, src);
        Cp47(d + 4, src + 4);
        Cp47(d + 8, src + 8);
        ctx.fpr[1] = Lfs47(src + 8);
        const u32 n1 = static_cast<u32>(load<u16>(o + 148)) + 1;
        const u32 hi = load<u16>(o + 150);
        store<u16>(o + 148, static_cast<u16>(n1));
        store<u16>(o + 150, static_cast<u16>(static_cast<s32>(hi) > static_cast<s32>(n1 & 0xFFFFu) ? hi : n1));
        r3 = n1;
    }
    ret(ctx, r3);
}

// T* f(a, b, c, Def* d): a 320-byte object m (SmallAlloc; 0 on failure):
// Wrapper_0CC4D4(m, c, d, 0, 0x01164BD0, 0x01164B30, 1, 0, 0, -, 0; f1 =
// the .text f32 0x001ED354), +236 = 0, +4 |= 4, vtable 0x00B186B0, the
// vec2 0x01164AFC at +240 and +248, +256 / +260 / +264 / +276 / +288 / +304
// = d->+308 / +336 / +340 / +420 / +416 / +424, +268..+300 otherwise 0,
// bytes +308..+315 = 0, 1, 0, 1, 1, 0, 0, 0, +12 |= 8; then
// XmlDef_Read_001ED004(a, b, m). r3 = m.
DECOMP(001ED358, XmlDef_New320_001ED358) {
    const u32 a = arg<u32>(ctx, 0), b = arg<u32>(ctx, 1), c = arg<u32>(ctx, 2), d = arg<u32>(ctx, 3);
    const u32 m = static_cast<u32>(call(ctx, 0x0055238Cu, {320}));
    u32 r = 0;
    if (m != 0) {
        // stack slot 9 is never written (stale in the original; the callee reads 8 and 10)
        callf(ctx, 0x000CC4D4u, {m, c, d, 0, 0x01164BD0u, 0x01164B30u, 1, 0, 0, 0, 0}, {Lfs47(0x001ED354u)});
        const u32 w4 = load<u32>(m + 4) | 4u;
        store<u32>(m + 236, 0);
        store<u32>(m + 4, w4);
        store<u32>(m, 0x00B186B0u);
        Cp47(m + 240, 0x01164AFCu);
        Cp47(m + 244, 0x01164B00u);
        Cp47(m + 248, 0x01164AFCu);
        Cp47(m + 252, 0x01164B00u);
        Cp47(m + 256, d + 308);
        Cp47(m + 260, d + 336);
        store<u32>(m + 268, 0);
        Cp47(m + 264, d + 340);
        store<u32>(m + 272, 0);
        store<u32>(m + 280, 0);
        Cp47(m + 276, d + 420);
        store<u32>(m + 284, 0);
        store<u32>(m + 292, 0);
        Cp47(m + 288, d + 416);
        store<u32>(m + 296, 0);
        store<u32>(m + 300, 0);
        store<u8>(m + 308, 0);
        Cp47(m + 304, d + 424);
        const u32 w12 = load<u32>(m + 12) | 8u;
        store<u8>(m + 310, 0);
        store<u8>(m + 309, 1);
        store<u8>(m + 311, 1);
        store<u8>(m + 313, 0);
        store<u8>(m + 312, 1);
        store<u8>(m + 314, 0);
        store<u8>(m + 315, 0);
        store<u32>(m + 12, w12);
        ctx.fpr[1] = Lfs47(d + 424);  // as the copy leaves it
        r = m;
    }
    call(ctx, 0x001ED004u, {a, b, r});
    ret(ctx, r);
}

// T::~T(T* o): vtable 0x00B1D528; the ((u16 +24 >> 1) & 31) parts +116..
// released (vslot 8); with +12: +20 (when set) and +16 released, +12
// ->vslot 40(.., 1, 0) when u16 +24 has bit 1, +12 released; Mem_FreeFrom(
// +32, 0) when set; the base vtable 0x00B1D4D8 (r3 = it).
DECOMP(002EDB10, VtB1D528_Dtor_002EDB10) {
    const u32 o = arg<u32>(ctx, 0);
    store<u32>(o, 0x00B1D528u);
    for (u32 i = 0; i < ((static_cast<u32>(load<u16>(o + 24)) >> 1) & 0x1Fu); ++i) {
        const u32 x = load<u32>(o + 116 + 4 * i);
        if (x != 0) VCall(ctx, x, 8, {x});
    }
    if (load<u32>(o + 12) != 0) {
        if (const u32 a = load<u32>(o + 20); a != 0) VCall(ctx, a, 8, {a});
        const u32 b = load<u32>(o + 16);
        VCall(ctx, b, 8, {b});
        if ((load<u16>(o + 24) & 1u) != 0) {
            const u32 c = load<u32>(o + 12);
            VCall(ctx, c, 40, {c, 1, 0});
        }
        const u32 c = load<u32>(o + 12);
        VCall(ctx, c, 8, {c});
    }
    if (const u32 p = load<u32>(o + 32); p != 0) call(ctx, 0x0054EE6Cu, {p, 0});
    store<u32>(o, 0x00B1D4D8u);
    ret(ctx, 0x00B1D4D8u);
}

// u32 f(Obj* o, name): the effect o->+40 replaced: the old one destroyed
// (Effect_Destruct, SmallFree(e, 140)); d = EffectDef_Find(name, 0, 1)
// (none -> r3 = 0); a 140-byte Effect_Construct(m, d) (SmallAlloc; a
// failed alloc is used unchecked) to +40 with +136 bit 0x200 cleared, its
// component 0x0027469E (vslot 32 == 5) bound to (o->+32)->+160 (the old
// binding released through vslot 8, the new one's u16 +4 counted), then
// Effect_SetHash4CF988(e, 0x04B6FB5B) (r3 = its result).
DECOMP(003044B0, Obj_SetEffect40_003044B0) {
    const u32 o = arg<u32>(ctx, 0), name = arg<u32>(ctx, 1);
    if (const u32 e = load<u32>(o + 40); e != 0) {
        call(ctx, 0x004C8E80u, {e});
        call(ctx, 0x00552420u, {e, 140});
        store<u32>(o + 40, 0);
    }
    const u32 d = static_cast<u32>(call(ctx, 0x004BDEDCu, {name, 0, 1}));
    if (d == 0) {
        ret(ctx, 0u);
        return;
    }
    const u32 m = static_cast<u32>(call(ctx, 0x0055238Cu, {140}));
    u32 e = 0;
    if (m != 0) {
        call(ctx, 0x004C7A68u, {m, d});
        e = m;
    }
    store<u32>(o + 40, e);
    store<u32>(e + 136, load<u32>(e + 136) & ~0x200u);
    const u32 v = load<u32>(load<u32>(o + 32) + 160);
    const u32 c = static_cast<u32>(call(ctx, 0x004BB4B4u, {load<u32>(o + 40), 0x0027469Eu}));
    if (c != 0 && static_cast<s32>(VCall(ctx, c, 32, {c})) == 5) {
        const u32 old = load<u32>(c + 28);
        if (old != 0) VCall(ctx, old, 8, {old});
        store<u32>(c + 28, v);
        if (v != 0) store<u16>(v + 4, static_cast<u16>(load<u16>(v + 4) + 1));
    }
    call(ctx, 0x004CF988u, {load<u32>(o + 40), 0x04B6FB5Bu});
}

// T* f(T* o) (leaf): six counters keyed by the global serial g =
// *(0x00F65AAC) (incremented): vtable 0x00B21444 with +4 = the word g of
// the table 0x00F6592C, +8 / +12 = 1, +16 = 0, +20 = g + 1; then five
// 24-byte parts at +24 / +48 / +72 / +96 (vtable 0x00B0DC20) and +120
// (vtable 0x00B0DC0C), each {table word g (tables 0x00F6596C, 0x00F659AC,
// 0x00F659EC, 0x00F65A2C, 0x00F65A6C), kind, 1, 0, value (0.5f, 0.5f, 0,
// 0, word: the "g was 0" flag in the top byte over the low bytes of the
// table address, as the original's rlwimi leaves it)}. r3 = o.
DECOMP(0043E040, StatBlock_Construct_0043E040) {
    const u32 o = arg<u32>(ctx, 0);
    constexpr u32 G = 0x00F65AACu;
    const u32 g = load<u32>(G);
    auto T = [&](u32 tbl) { return load<u32>(tbl + 4 * load<u32>(G)); };
    store<u32>(o + 8, 1);
    store<u32>(o + 12, 1);
    store<u32>(o + 16, 0);
    store<u32>(o + 4, load<u32>(0x00F6592Cu + 4 * g));
    store<u32>(o + 20, g + 1);
    store<u32>(o + 0, 0x00B21444u);
    store<u32>(o + 36, 1);
    store<u32>(o + 32, 3);
    store<u32>(o + 40, 0);
    store<u32>(o + 28, T(0x00F6596Cu));
    store<u32>(o + 44, 0x3F000000u);
    store<u32>(o + 24, 0x00B0DC20u);
    store<u32>(o + 56, 3);
    store<u32>(o + 60, 1);
    store<u32>(o + 64, 0);
    store<u32>(o + 52, T(0x00F659ACu));
    store<u32>(o + 68, 0x3F000000u);
    store<u32>(o + 48, 0x00B0DC20u);
    store<u32>(o + 80, 3);
    store<u32>(o + 84, 1);
    store<u32>(o + 88, 0);
    store<u32>(o + 76, T(0x00F659ECu));
    store<u32>(o + 92, 0);
    store<u32>(o + 72, 0x00B0DC20u);
    store<u32>(o + 104, 3);
    store<u32>(o + 108, 1);
    store<u32>(o + 112, 0);
    store<u32>(o + 100, T(0x00F65A2Cu));
    store<u32>(o + 116, 0);
    store<u32>(o + 96, 0x00B0DC20u);
    const u32 g2 = load<u32>(G);
    store<u32>(o + 124, load<u32>(0x00F65A6Cu + 4 * g2));
    store<u32>(o + 132, 1);
    store<u32>(o + 128, 4);
    store<u32>(o + 136, 0);
    store<u32>(o + 140, ((g2 == 0 ? 1u : 0u) << 24) | 0x00F65A6Cu);  // cntlzw / rlwimi into the address register
    store<u32>(o + 120, 0x00B0DC0Cu);
    store<u32>(G, g2 + 1);
}

// u32 f(T* d, const T* s): d's fields +0..+8, +16..+24, +32..+56 copied
// from s (the bytes +44..+47 one by one); the references +60 / +64
// replaced by s's (the old ones ->vslot 40(.., 0), the new ones' u16 +178
// counted). f1 = s->+24 as the copies leave it; r3 as the original leaves it.
DECOMP(00442FB8, SpawnParams_Copy_00442FB8) {
    const u32 d = arg<u32>(ctx, 0), s = arg<u32>(ctx, 1);
    Cp47(d, s);
    Cp47(d + 4, s + 4);
    Cp47(d + 8, s + 8);
    Cp47(d + 36, s + 36);
    Cp47(d + 32, s + 32);
    Cp47(d + 40, s + 40);
    Cp47(d + 16, s + 16);
    Cp47(d + 48, s + 48);
    Cp47(d + 20, s + 20);
    Cp47(d + 52, s + 52);
    Cp47(d + 24, s + 24);
    for (u32 i = 0; i < 4; ++i) store<u8>(d + 44 + i, load<u8>(s + 44 + i));
    Cp47(d + 56, s + 56);
    ctx.fpr[1] = Lfs47(s + 24);
    const u32 old = load<u32>(d + 60);
    const u32 v = load<u32>(s + 60);
    if (old != 0) VCall(ctx, old, 40, {old, 0});
    if (v != 0) store<u16>(v + 178, static_cast<u16>(load<u16>(v + 178) + 1));
    const u32 old2 = load<u32>(d + 64);
    store<u32>(d + 60, v);
    const u32 w = load<u32>(s + 64);
    u32 r3 = old2;
    if (old2 != 0) r3 = static_cast<u32>(VCall(ctx, old2, 40, {old2, 0}));
    if (w != 0) {
        store<u16>(w + 178, static_cast<u16>(load<u16>(w + 178) + 1));
        r3 = w + 178;
    }
    store<u32>(d + 64, w);
    ret(ctx, r3);
}

// u32 f(-, XmlCursor* x, u8 b): x's first child c must be the element
// 0xC53F7E26 (hash +20 when u8 +24 has 0x80; else r3 = the hash or 0);
// with the context {u8 b, u32 0} each child of c goes through 0x004F6B60
// and then each again through Xml_ApplyAssociations_004F6D94; a failure
// in either pass reports String_Format(0x00AF06F4 / 0x00AF0744,
// XmlCursor_DocPath(x)) (r3 = its result for the second; else 0).
DECOMP(004F4B28, XmlAssoc_Apply_004F4B28) {
    const u32 x = arg<u32>(ctx, 1), b = arg<u32>(ctx, 2);
    const u32 c = static_cast<u32>(call(ctx, 0x004F9140u, {x}));
    if (c == 0) {
        ret(ctx, 0u);
        return;
    }
    const u32 h = (load<u8>(c + 24) & 0x80u) != 0 ? load<u32>(c + 20) : 0u;
    if (h != 0xC53F7E26u) {
        ret(ctx, h);
        return;
    }
    Stack fr(ctx, 16);  // sp+112: u8 b (+1..+3 padding), u32 0
    const u32 rec = fr.addr();
    store<u8>(rec, static_cast<u8>(b));
    store<u32>(rec + 4, 0);
    if (u32 k = static_cast<u32>(call(ctx, 0x004F9140u, {c})); k != 0) {
        bool ok = true;
        do {
            if (static_cast<u32>(call(ctx, 0x004F6B60u, {k, rec})) == 0) ok = false;
            k = static_cast<u32>(call(ctx, 0x004F8D74u, {k}));
        } while (k != 0);
        if (!ok) {
            const u32 p = static_cast<u32>(call(ctx, 0x004F8944u, {x}));
            call(ctx, 0x005534D0u, {0x00AF06F4u, p});
        }
    }
    u32 k = static_cast<u32>(call(ctx, 0x004F9140u, {c}));
    if (k == 0) {
        ret(ctx, 0u);
        return;
    }
    bool ok = true;
    do {
        if (static_cast<u32>(call(ctx, 0x004F6D94u, {k, rec})) == 0) ok = false;
        k = static_cast<u32>(call(ctx, 0x004F8D74u, {k}));
    } while (k != 0);
    if (!ok) {
        const u32 p = static_cast<u32>(call(ctx, 0x004F8944u, {x}));
        call(ctx, 0x005534D0u, {0x00AF0744u, p});
        return;
    }
    ret(ctx, 0u);
}

// s32 f(Table* t, const u32* key): the 20-byte rows t->+8 (u16 count +2),
// first sorted (qsort(rows, n, 20, t->+32)) when u16 +6 has bit 1 (which
// is cleared); then a binary search for *key: with the default comparator
// 0x00F3A028 by the u32 row key (unsigned), else through the comparator
// OPD (row, &probe {key, -FLT_MAX, u8 0, 0, 0}; r5 = entry, r6 = OPD).
// r3 = the index or -1.
DECOMP(00789A1C, SortedTbl_Find_00789A1C) {
    const u32 t = arg<u32>(ctx, 0), k = arg<u32>(ctx, 1);
    const u32 h = load<u16>(t + 6);
    if ((h & 1u) != 0) {
        const u32 n = load<u16>(t + 2);
        store<u16>(t + 6, static_cast<u16>(h & 0xFFFEu));
        if (static_cast<s32>(n) > 0) call(ctx, 0x0071EC28u, {load<u32>(t + 8), n, 20, load<u32>(t + 32)});
    }
    s32 hi = static_cast<s32>(load<u16>(t + 2)) - 1, lo = 0;
    const u32 arr = load<u32>(t + 8);
    if (load<u32>(t + 32) == 0x00F3A028u) {
        while (hi >= lo) {
            const s32 mid = (hi + lo) >> 1;
            const u32 key = load<u32>(k);
            const u32 v = load<u32>(arr + 20 * static_cast<u32>(mid));
            if (v > key) hi = mid - 1;
            else if (v < key) lo = mid + 1;
            else {
                ret(ctx, mid);
                return;
            }
        }
        ret(ctx, -1);
        return;
    }
    Stack fr(ctx, 32);  // the probe at sp+112 (bytes +9..+11 padding)
    const u32 rec = fr.addr();
    store<u8>(rec + 8, 0);
    store<u32>(rec + 4, 0xFF7FFFFFu);
    store<u32>(rec + 12, 0);
    store<u32>(rec + 16, 0);
    store<u32>(rec, load<u32>(k));
    while (hi >= lo) {
        const s32 mid = (hi + lo) >> 1;
        const u32 opd = load<u32>(t + 32);
        const s32 r = static_cast<s32>(call_ptr(ctx, opd, {arr + 20 * static_cast<u32>(mid), rec, load<u32>(opd), opd}));
        if (r > 0) hi = mid - 1;
        else if (r < 0) lo = mid + 1;
        else {
            ret(ctx, mid);
            return;
        }
    }
    ret(ctx, -1);
}

// ---------------------------------------------------------------- sweep: 103 instructions
// Pops the state stack of 00024230: the s32 count +24 decremented when
// nonzero, then o->vslot 220(o, the word +8 + 4 * count).
static void StatePop47(PPUContext& ctx, u32 o) {
    u32 n = load<u32>(o + 24);
    if (n != 0) {
        --n;
        store<u32>(o + 24, n);
    }
    VCall(ctx, o, 220, {o, load<u32>(o + 8 + 4 * n)});
}

// u32 f(Actor* o, u8 keep): unless keep, by the state o->+4: 1 -> the
// previous state restored (StatePop47); 12 -> o->vslot 220(o, 13), the
// pop, Obj_SetEffectHash_000DAEC8(o->+48, 0x0C544DCF); 13 -> the pop and
// the effect. r3 = the last call's result, o (keep) or the state.
DECOMP(00024230, Actor_PopState_00024230) {
    const u32 o = arg<u32>(ctx, 0), keep = arg<u32>(ctx, 1) & 0xFFu;
    if (keep != 0) return;
    const s32 s = static_cast<s32>(load<u32>(o + 4));
    if (s == 1) {
        StatePop47(ctx, o);
        return;
    }
    if (s == 12 || s == 13) {
        if (s == 12) VCall(ctx, o, 220, {o, 13});
        StatePop47(ctx, o);
        call(ctx, 0x000DAEC8u, {load<u32>(o + 48), 0x0C544DCFu});
        return;
    }
    ret(ctx, static_cast<u32>(s));
}

// u32 f(Actor* o, Obj* t, a): +436 = 0, +388 = a, the direction +192 =
// t->+176 - o->+176 (y zeroed unless u8 +305 has 0x20, when it has 0x40
// or vslot 132 fails), normalized (the .text 1.0f 0x00039170 /
// fsqrts); when |Vec_SignedAngle16680(+192, +160)| exceeds the .text
// 0x00039174, o->vslot 256(o) and +324 = 13 (r3 = 13), else o->vslot
// 220(o, 13). f1 = that absolute angle.
DECOMP(00039178, Actor_FaceTarget_00039178) {
    const u32 o = arg<u32>(ctx, 0), t = arg<u32>(ctx, 1), a = arg<u32>(ctx, 2);
    store<u32>(o + 436, 0);
    store<u32>(o + 388, a);
    const f64 dx = F45(Lfs47(t + 176) - Lfs47(o + 176));
    const f64 dy = F45(Lfs47(t + 180) - Lfs47(o + 180));
    const f64 dz = F45(Lfs47(t + 184) - Lfs47(o + 184));
    const u32 b = load<u8>(o + 305);
    store<u32>(o + 192, W47(dx));
    store<u32>(o + 196, W47(dy));
    store<u32>(o + 200, W47(dz));
    bool flat;
    if ((b & 0x20u) != 0) flat = false;
    else if ((b & 0x40u) != 0) flat = true;
    else flat = static_cast<u32>(VCall(ctx, o, 132, {o})) == 0;
    if (flat) store<u32>(o + 196, 0);
    constexpr u32 K = 0x00039170u;
    const f64 y = Lfs47(o + 196), x = Lfs47(o + 192), z = Lfs47(o + 200);
    f64 d = F45(y * y);
    d = Fma45(x, x, d);
    d = Fma45(z, z, d);
    const f64 q = F45(Lfs47(K) / F45(std::sqrt(d)));
    const f64 xq = F45(x * q), yq = F45(y * q), zq = F45(z * q);
    store<u32>(o + 192, W47(xq));
    store<u32>(o + 196, W47(yq));
    store<u32>(o + 200, W47(zq));
    ctx.fpr[1] = yq;  // f1..f3 as the normalization leaves them
    ctx.fpr[2] = xq;
    ctx.fpr[3] = zq;
    call(ctx, 0x00016680u, {o + 192, o + 160});
    const f64 aa = std::fabs(ctx.fpr[1]);
    ctx.fpr[1] = aa;
    if (aa > Lfs47(K + 4)) {
        VCall(ctx, o, 256, {o});
        store<u32>(o + 324, 13);
        ret(ctx, 13u);
        return;
    }
    VCall(ctx, o, 220, {o, 13});
}

// T::T(T* o, Xml* x): Obj_InitVt_B21C98(o, x), vtable 0x00B0FC98, f32s
// +12 = 1, +16 = 0, +20 = 1 read from 0x8F123DDD / 0xB7EA3D94 /
// 0xBBD6EAAF (defaults from the .text pool 0x00080330: 1, 0, then by the
// hash 0x03D808CF when present: 0x8105148C -> 0.5, 0x00010DFD -> 2,
// otherwise 1; absent: the current +20); a negative +16 becomes 0 (r3 =
// 0). f1 = +16.
DECOMP(00080340, Cmd_Construct_B0FC98_00080340) {
    const u32 o = arg<u32>(ctx, 0), x = arg<u32>(ctx, 1);
    constexpr u32 K = 0x00080330u;
    call(ctx, 0x004BB3B0u, {o, x});
    store<u32>(o, 0x00B0FC98u);
    store<u32>(o + 12, 0x3F800000u);
    store<u32>(o + 16, 0);
    store<u32>(o + 20, 0x3F800000u);
    callf(ctx, kReadF32_47, {x, 0x8F123DDDu, o + 12}, {Lfs47(K)});
    callf(ctx, kReadF32_47, {x, 0xB7EA3D94u, o + 16}, {Lfs47(K + 4)});
    Stack fr(ctx, 16);  // the hash at sp+112
    const u32 hp = fr.addr();
    const u32 r = static_cast<u32>(call(ctx, kReadHash47, {x, 0x03D808CFu, hp, 0}));
    f64 f1 = Lfs47(o + 20);
    if (r == 0) {
        const s32 h = static_cast<s32>(load<u32>(hp));
        if (h > static_cast<s32>(0x8105148Cu)) {
            if (h == 0x00010DFD) {
                store<u32>(o + 20, 0x40000000u);
                f1 = Lfs47(K + 12);
            } else {
                store<u32>(o + 20, 0x3F800000u);
                f1 = Lfs47(K);
            }
        } else if (h > static_cast<s32>(0x8105148Bu)) {
            store<u32>(o + 20, 0x3F000000u);
            f1 = Lfs47(K + 8);
        } else {
            store<u32>(o + 20, 0x3F800000u);
            f1 = Lfs47(K);
        }
    }
    callf(ctx, kReadF32_47, {x, 0xBBD6EAAFu, o + 20}, {f1});
    const f64 g = Lfs47(o + 16);
    ctx.fpr[1] = g;
    if (g < Lfs47(K + 4)) {
        store<u32>(o + 16, 0);
        ret(ctx, 0u);
    }
}

// u32 f(Cmd* c, X* x): the object t (Effect_Bind4BB7E0(x, c->+16, 0), then
// with create = 1; else Obj_FindById(c->+12)->+160, Hash_ToString(c->+12)
// when not found); with a name c->+20 the child found by
// WorldObj_ForEach(t, 0x00F2FAF8, &{name, 0}) (missing: Hash_ToString(name),
// t->vslot 12(t), then the record's result again); a kind-8 object is
// removed from the player body's records (Recs_Remove_002A8F18(
// Obj_Notify26(body), obj); body = *(*(0x00F65CA0) + 4216)). r3 as the
// original leaves it.
DECOMP(0008546C, Cmd_DetachRecs_0008546C) {
    const u32 c = arg<u32>(ctx, 0), x = arg<u32>(ctx, 1);
    u32 t = 0, r3 = 0;
    if (load<u32>(c + 16) != 0) {
        t = static_cast<u32>(call(ctx, 0x004BB7E0u, {x, load<u32>(c + 16), 0}));
        if (t == 0) t = static_cast<u32>(call(ctx, 0x004BB7E0u, {x, load<u32>(c + 16), 1}));
        r3 = t;
    }
    if (t == 0) {
        const u32 id = load<u32>(c + 12);
        r3 = id;
        if (id != 0) {
            const u32 ob = static_cast<u32>(call(ctx, 0x000D6A28u, {id}));
            r3 = ob;
            if (ob != 0) t = load<u32>(ob + 160);
            else r3 = static_cast<u32>(call(ctx, 0x0056A6E0u, {load<u32>(c + 12)}));
        }
    }
    if (t == 0) {
        ret(ctx, r3);
        return;
    }
    u32 e = t;
    r3 = load<u32>(c + 20);
    if (r3 != 0) {
        Stack fr(ctx, 16);  // the search record at sp+112 {name, result}
        const u32 rec = fr.addr();
        store<u32>(rec, r3);
        store<u32>(rec + 4, 0);
        r3 = static_cast<u32>(call(ctx, 0x0057F5D8u, {t, 0x00F2FAF8u, rec, 0}));
        e = load<u32>(rec + 4);
        if (e == 0) {
            call(ctx, 0x0056A6E0u, {load<u32>(c + 20)});
            r3 = static_cast<u32>(VCall(ctx, t, 12, {t}));
            e = load<u32>(rec + 4);
        }
    }
    if (e == 0) {
        ret(ctx, r3);
        return;
    }
    const u32 kind = (load<u32>(e + 4) >> 10) & 0x3Fu;
    if (kind != 8) {
        ret(ctx, kind);
        return;
    }
    const u32 p = load<u32>(load<u32>(0x00F65CA0u) + 4216);
    if (p == 0) {
        ret(ctx, 0u);
        return;
    }
    const u32 n = static_cast<u32>(call(ctx, 0x000D94CCu, {p}));
    if (n == 0) {
        ret(ctx, 0u);
        return;
    }
    call(ctx, 0x002A8F18u, {n, e});
}

// u32 f(Obj* o, Msg* m): message hooks by the hash m->+8 (0x2FE47F7F:
// Obj_RandRange_00257F10 on Obj_FindComponent(o, 29, 0); 0x21595665 /
// 0x00C73980: o->+220 ->vslot 24() / vslot 28(.., 0); 0xD77F98D6:
// Effect_SetHash4CF988(o->vslot 272(o), m->+12) when that is set), then
// always forwarded to o->+244 ->vslot 48(.., m) when set (r3 = its result,
// else as the hook leaves it).
DECOMP(000DAF24, Obj_OnMessage_000DAF24) {
    const u32 o = arg<u32>(ctx, 0), m = arg<u32>(ctx, 1);
    const u32 h = load<u32>(m + 8);
    u32 r3 = h;
    if (h == 0x2FE47F7Fu) {
        r3 = static_cast<u32>(call(ctx, 0x000D1288u, {o, 29, 0}));
        if (r3 != 0) r3 = static_cast<u32>(call(ctx, 0x00257F10u, {r3}));
    } else if (h == 0x21595665u) {
        r3 = load<u32>(o + 220);
        if (r3 != 0) r3 = static_cast<u32>(VCall(ctx, r3, 24, {r3}));
    } else if (h == 0x00C73980u) {
        r3 = load<u32>(o + 220);
        if (r3 != 0) r3 = static_cast<u32>(VCall(ctx, r3, 28, {r3, 0}));
    } else if (h == 0xD77F98D6u) {
        const u32 v = load<u32>(m + 12);
        r3 = static_cast<u32>(VCall(ctx, o, 272, {o}));
        if (r3 != 0) r3 = static_cast<u32>(call(ctx, 0x004CF988u, {r3, v}));
    }
    const u32 p = load<u32>(o + 244);
    if (p == 0) {
        ret(ctx, r3);
        return;
    }
    VCall(ctx, p, 48, {p, m});
}

// u32 f(Obj* o, u8 on): with on, o->vslot 416(o, 4, 0) and, for each
// 48-byte row of the u16-counted list o->+388 (re-read after each row
// handled) with a skeleton s (+28): every bone (Skel_BoneAt_0091A7D8(s, j)
// for j below Skel_BoneCount_0091A2CC(s)) released (AnimSkel_Release7D4EA8),
// the properties 0xA734004D / 0x91C7CF5B removed (Hk_RemoveProperty93FED8
// into stack slots) and s released. Then Obj_SetRagdollBase(o, on) (r3 =
// its result).
DECOMP(000E4544, Obj_SetRagdoll_000E4544) {
    const u32 o = arg<u32>(ctx, 0), on = arg<u32>(ctx, 1) & 0xFFu;
    if (on != 0) {
        VCall(ctx, o, 416, {o, 4, 0});
        u32 l = load<u32>(o + 388);
        if (l != 0 && load<u16>(l + 8) != 0) {
            Stack fr(ctx, 16);  // the two result slots at sp+112 / sp+120
            const u32 sp = fr.addr();
            for (u32 i = 0; i < (l != 0 ? static_cast<u32>(load<u16>(l + 8)) : 0u); ++i) {
                const u32 s = load<u32>(load<u32>(l) + 48 * i + 28);
                if (s == 0) continue;
                for (s32 j = 0; j < static_cast<s32>(call(ctx, 0x0091A2CCu, {s})); ++j) {
                    const u32 b = static_cast<u32>(call(ctx, 0x0091A7D8u, {s, static_cast<u32>(j)}));
                    call(ctx, 0x007D4EA8u, {b});
                }
                call(ctx, 0x0093FED8u, {sp, s, 0xA734004Du});
                call(ctx, 0x0093FED8u, {sp + 8, s, 0x91C7CF5Bu});
                call(ctx, 0x007D4EA8u, {s});
                l = load<u32>(o + 388);
            }
        }
    }
    call(ctx, 0x000E02F8u, {o, on});
}

// u32 f(Obj* o): the launch speed factor o->+552 from the aim vector v
// (o->vslot 140(&v, o)): with a horizontal length sqrt(vx^2 + vz^2 + C0^2)
// above C4, the horizontal distance d (o->+496 vs +144) over it;
// otherwise from the gravity g = (o->+388)->+8->+20 * (o->+216)->+192:
// when |g| exceeds |vy| * C8, vy / g + sqrt((vy / g)^2 - dy * C12 / g)
// (stored only for a non-negative root argument), else (dy / vy) * (g dy
// / (C12 vy^2) + C16) (C0.. the .text pool 0x000EF3B4; fsqrts / fmsubs).
// r3 = o + 496 on the first path, else the pool address; f1 as left.
DECOMP(000EF3C8, Obj_LaunchFactor_000EF3C8) {
    const u32 o = arg<u32>(ctx, 0);
    constexpr u32 K = 0x000EF3B4u;
    const u32 t = load<u32>(o + 216);
    Stack fr(ctx, 16);  // the vector at sp+112
    const u32 v = fr.addr();
    VCall(ctx, o, 140, {v, o});
    const f64 c0 = Lfs47(K);
    const f64 vx = Lfs47(v), vy = Lfs47(v + 4), vz = Lfs47(v + 8);
    const f64 c00 = F45(c0 * c0);
    const f64 h = F45(std::sqrt(Fma45(vz, vz, Fma45(vx, vx, c00))));
    const f64 ex = F45(Lfs47(o + 496) - Lfs47(o + 144));
    const f64 ez = F45(Lfs47(o + 504) - Lfs47(o + 152));
    const f64 dy = F45(Lfs47(o + 500) - Lfs47(o + 148));
    const f64 dist = F45(std::sqrt(Fma45(ez, ez, Fma45(ex, ex, c00))));
    if (h > Lfs47(K + 4)) {
        const f64 r = F45(dist / h);
        store<u32>(o + 552, W47(r));
        ctx.fpr[1] = r;
        ret(ctx, o + 496);
        return;
    }
    const u32 ga = load<u32>(load<u32>(o + 388) + 8) + 20;
    const f64 lim = F45(std::fabs(vy) * Lfs47(K + 8));
    const f64 g = F45(Lfs47(ga) * Lfs47(t + 192));
    if (std::fabs(g) > lim) {
        f64 a = F45(vy / g);
        const f64 b = F45(F45(dy * Lfs47(K + 12)) / g);
        const f64 disc = F45(std::fma(a, a, -b));
        if (disc >= c0) {
            a = F45(a + F45(std::sqrt(disc)));
            store<u32>(o + 552, W47(a));
        }
        ctx.fpr[1] = a;
        ret(ctx, K);  // r3 = the pool address, as the lfs sequence leaves it
        return;
    }
    const f64 p = F45(g * dy);
    const f64 q = F45(dy / vy);
    const f64 w = F45(F45(vy * Lfs47(K + 12)) * vy);
    const f64 r = F45(q * F45(F45(p / w) + Lfs47(K + 16)));
    store<u32>(o + 552, W47(r));
    ctx.fpr[1] = r;
    ret(ctx, K);
}

// u32 f(T* o, f32 x): the f32 o->+348 clamped between the .text f32
// 0x0016F348 and o->+344: for state o->+772 other than 1, +348 =
// clamp(+348 + x); for
// state 1, unless u8 (*(0x00F65CA0))->+29 is 1, the player body p
// (*(g + 4216); none -> r3 = 0) gets p->vslot 316(p), then +348 =
// clamp(q->+348) with q the other player's body (*(g + (1 - +772) *
// 0x20C80 + 4216), 0 when u8 +29 is 1; read unchecked). r3 = the
// constant's address 0x0016F348 after either min (1 / 0 on the early
// returns); f1 = the min when it is above the floor, else +344.
DECOMP(0016F350, Obj_ClampSpeed_0016F350) {
    const u32 o = arg<u32>(ctx, 0);
    const f64 x = farg(ctx, 0);
    const f64 c = Lfs47(0x0016F348u);
    const u32 s = load<u32>(o + 772);
    if (s != 1) {
        const f64 a = F45(Lfs47(o + 348) + x);
        const f64 mx = Lfs47(o + 344);
        const f64 m = a < mx ? a : mx;
        ctx.fpr[1] = m > c ? m : mx;
        store<u32>(o + 348, W47(m > c ? m : c));
        ret(ctx, 0x0016F348u);  // r3 = the constant's address, as the lfs leaves it
        return;
    }
    const u32 g = load<u32>(0x00F65CA0u);
    if (load<u8>(g + 29) == 1) {
        ret(ctx, 1u);  // the state
        return;
    }
    const u32 p = load<u32>(g + (1u - s) * 0x20C80u + 4216);
    if (p == 0) {
        ret(ctx, 0u);
        return;
    }
    VCall(ctx, p, 316, {p});
    const f64 mx = Lfs47(o + 344);
    const u32 g2 = load<u32>(0x00F65CA0u);
    u32 q = 0;
    if (load<u8>(g2 + 29) != 1) q = load<u32>(g2 + (1u - load<u32>(o + 772)) * 0x20C80u + 4216);
    const f64 a = Lfs47(q + 348);
    const f64 m = a < mx ? a : mx;
    ctx.fpr[1] = m > c ? m : mx;
    store<u32>(o + 348, W47(m > c ? m : c));
    ret(ctx, 0x0016F348u);
}

// u32 f(Obj* o, u8 on): with on, the six 20-byte limb slots (+584 body,
// +588 link): a set link is released (->vslot 40(.., 1) when its +172 has
// 0x40000) and cleared, the body removed (Body_Remove931F00(body->+8)),
// released (AnimSkel_Release7D4EA8) and cleared; +556 ->vslot 40(.., 0)
// and cleared when set; then +536 = 0 and the seven emitters +428 + 16 k
// stopped (Emitter_Stop(.., 1)) with +428 / +432 cleared. Then
// Obj_SetRagdoll_000E4544(o, on) (r3 = its result).
DECOMP(001C70C8, Obj_SetLimbsRagdoll_001C70C8) {
    const u32 o = arg<u32>(ctx, 0), on = arg<u32>(ctx, 1) & 0xFFu;
    if (on != 0) {
        for (u32 i = 0; i < 6; ++i) {
            const u32 e = o + 20 * i;
            const u32 x = load<u32>(e + 588);
            if (x == 0) continue;
            if ((load<u32>(x + 172) & 0x40000u) != 0) VCall(ctx, x, 40, {x, 1});
            store<u32>(e + 588, 0);
            const u32 body = load<u32>(e + 584);
            call(ctx, 0x00931F00u, {load<u32>(body + 8), body});  // r4 = the body, as the load leaves it (the callee reads it)
            call(ctx, 0x007D4EA8u, {load<u32>(e + 584)});
            store<u32>(e + 584, 0);
        }
        if (const u32 y = load<u32>(o + 556); y != 0) {
            VCall(ctx, y, 40, {y, 0});
            store<u32>(o + 556, 0);
        }
        store<u32>(o + 536, 0);
        for (u32 k = 0; k < 7; ++k) {
            const u32 e = o + 16 * k;
            call(ctx, 0x00443424u, {load<u32>(e + 428), 1});
            store<u32>(e + 428, 0);
            store<u32>(e + 432, 0);
        }
    }
    call(ctx, 0x000E4544u, {o, on});
}

// bool f(Obj* o, a, b, c): o->vslot 32(o, 0) set or no +496 -> 0; with
// the f32 +508 at most the .text f32 0x001F6800 the target o->vslot
// 272(o) must exist and its component 0xDD61613C (Effect_FindComponent on
// a second vslot 272 call) report kind 2 (vslot 32) and pass vslot 24
// (else 0); then Obj_CanTakeHitScaledE7FF0(o, a, b, c) (r3 = its result).
DECOMP(001F6808, Obj_CanTakeHit_001F6808) {
    const u32 o = arg<u32>(ctx, 0), a = arg<u32>(ctx, 1), b = arg<u32>(ctx, 2), c = arg<u32>(ctx, 3);
    if (static_cast<u32>(VCall(ctx, o, 32, {o, 0})) != 0 || load<u32>(o + 496) == 0) {
        ret(ctx, 0u);
        return;
    }
    const f64 v = Lfs47(o + 508);
    ctx.fpr[1] = v;
    if (v <= Lfs47(0x001F6800u)) {
        if (static_cast<u32>(VCall(ctx, o, 272, {o})) == 0) {
            ret(ctx, 0u);
            return;
        }
        const u32 e = static_cast<u32>(VCall(ctx, o, 272, {o}));
        const u32 k = static_cast<u32>(call(ctx, 0x004BB4B4u, {e, 0xDD61613Cu}));
        if (k == 0 || static_cast<s32>(VCall(ctx, k, 32, {k})) != 2 || static_cast<u32>(VCall(ctx, k, 24, {k})) == 0) {
            ret(ctx, 0u);
            return;
        }
    }
    call(ctx, 0x000E7FF0u, {o, a, b, c});
}

// T* New(-, Xml* x, y): a 192-byte query (Mem_Alloc(192, 0, 16);
// Query_BaseCtor_00746148(m, y, 0, 0, 1, 0, 1, 0, 0, 0); a failed alloc is
// used unchecked) whose flag byte +184 bits 0x20 / 0x10 / 0x08 / 0x02 are
// read as the bools 0xBEF25C9F / 0x1EF4F9F5 / 0x2D407057 / 0x778A1C92
// (each defaulting to the current bit) through a stack byte. r3 = m.
DECOMP(0020D0C8, Query_New_0020D0C8) {
    const u32 x = arg<u32>(ctx, 1), y = arg<u32>(ctx, 2);
    const u32 m = static_cast<u32>(call(ctx, 0x0054ED38u, {192, 0, 16}));
    u32 q = 0;
    if (m != 0) {
        call(ctx, 0x00746148u, {m, y, 0, 0, 1, 0, 1, 0, 0, 0});
        q = m;
    }
    Stack fr(ctx, 16);  // the bool at sp+128 (frame of 176 bytes)
    const u32 bp = fr.addr();
    store<u8>(bp, 0);
    constexpr u32 kHash[4] = {0xBEF25C9Fu, 0x1EF4F9F5u, 0x2D407057u, 0x778A1C92u};
    constexpr u32 kBit[4] = {0x20u, 0x10u, 0x08u, 0x02u};
    constexpr u32 kSh[4] = {5, 4, 3, 1};
    call(ctx, kReadBool47, {x, kHash[0], bp, (load<u8>(q + 184) & kBit[0]) != 0 ? 1u : 0u});
    for (u32 k = 0; k < 4; ++k) {
        const u32 b = (load<u8>(q + 184) & ~kBit[k]) | ((load<u8>(bp) & 1u) << kSh[k]);
        store<u8>(q + 184, static_cast<u8>(b));
        if (k < 3) call(ctx, kReadBool47, {x, kHash[k + 1], bp, (b & kBit[k + 1]) != 0 ? 1u : 0u});
    }
    ret(ctx, q);
}

// u32 f(-, Mesh* b, w, h): a (w x (h + 1)) grid: +0 = 6, u16 +58 = w * h + h
// vertices (two Buffer_Alloc52F198 vertex buffers +4 / +20 of 24 bytes
// each, align 128), u16 +60 = 0, u16 +56 = 2 w (h + 1) indices (+52 = 1,
// buffer +36 of u16s; its +8 bits 0xE set to 4); the strip indices
// written to the index buffer's data: per column x, the pairs (x h + k,
// x h + k + h) for k < h and then (x h, x h + h). r3 = the write pointer.
DECOMP(0025CF78, Mesh_InitGrid_0025CF78) {
    const u32 b = arg<u32>(ctx, 1), w = arg<u32>(ctx, 2), h = arg<u32>(ctx, 3);
    const u32 nv = w * h + h;
    store<u32>(b, 6);
    store<u16>(b + 58, static_cast<u16>(nv));
    const u32 vb = 24 * (nv & 0xFFFFu);
    store<u16>(b + 60, 0);
    call(ctx, 0x0052F198u, {b + 4, vb, 128, 1, 2});
    call(ctx, 0x0052F198u, {b + 20, vb, 128, 1, 2});
    const u32 ni = w * (h << 1) + (w << 1);
    store<u16>(b + 56, static_cast<u16>(ni));
    store<u32>(b + 52, 1);
    const u32 ib = b + 36;
    call(ctx, 0x0052F198u, {ib, (ni << 1) & 0x1FFFEu, 128, 0, 2});
    store<u32>(ib + 8, (load<u32>(ib + 8) & ~0xEu) | 0x4u);
    u32 p = load<u32>(ib);  // the original reads it on both arms of a bit-0 test of +4
    if (w != 0) {
        u32 base = 0;
        for (u32 x = 0; x < w; ++x, base += h) {
            if (h != 0) {
                u32 r6 = base, r5 = base + h;
                s32 n = static_cast<s32>(h);
                do {
                    store<u16>(p, static_cast<u16>(r6));
                    store<u16>(p + 2, static_cast<u16>(r5));
                    p += 4;
                    ++r6;
                    ++r5;
                    --n;
                } while (n > 0);
            }
            store<u16>(p, static_cast<u16>(base));
            store<u16>(p + 2, static_cast<u16>(base + h));
            p += 4;
        }
    }
    ret(ctx, p);
}

// u32 f(T* o, Item* x): for each entry e of the u16-counted list o->+92
// (re-read every step) with a bank (+24): every sound s (+16[j], s32 count
// +20) whose owner s->+40 matches x (any for x 0) is released
// (Bank_Release_A459B8(bank, s); e->+32 / +36 cleared when they hold it);
// the bank's +36 = its count when x is set and some owner had a +12 of
// kind 4, else 0. Bit 0x100 of u16 o->+40 is cleared for every entry.
// r3 = the final count.
DECOMP(0034A1FC, SoundSet_Release_0034A1FC) {
    const u32 o = arg<u32>(ctx, 0), x = arg<u32>(ctx, 1);
    u32 r3 = 0;
    for (u32 i = 0;; ++i) {
        const u32 l = load<u32>(o + 92);
        r3 = l != 0 ? static_cast<u32>(load<u16>(l + 8)) : 0u;
        if (i >= r3) break;
        const u32 e = load<u32>(load<u32>(l) + 4 * i);
        const u32 bank = load<u32>(e + 24);
        if (bank != 0) {
            bool k4 = false;
            u32 n = 0;
            for (s32 j = 0;; ++j) {
                n = load<u32>(bank + 20);
                if (!(j < static_cast<s32>(n))) break;
                const u32 s = load<u32>(load<u32>(bank + 16) + 4 * static_cast<u32>(j));
                const u32 t = load<u32>(s + 40);
                if (t != 0 && load<u32>(t + 12) != 0 && load<u32>(load<u32>(t + 12) + 8) == 4) k4 = true;
                if (x == 0 || x == t) {
                    if (s == load<u32>(e + 32)) store<u32>(e + 32, 0);
                    if (s == load<u32>(e + 36)) store<u32>(e + 36, 0);
                    call(ctx, 0x00A459B8u, {bank, s});
                    --j;
                }
            }
            store<u32>(bank + 36, (x != 0 && k4) ? n : 0u);
        }
        store<u16>(o + 40, static_cast<u16>(load<u16>(o + 40) & ~0x100u));
    }
    ret(ctx, r3);
}

// u32 f(Npc* o, u8 on): the alert state ((o->+160 >> 1); low bit kept).
// On, from state 0: o->+24 ->vslot 40(.., 1, 1), Obj_Attach364408(o,
// +256) when set, state 1, Speaker_SayId4A5C90(o->+28, o->+192, 0; f1 =
// the .text f32 0x0036E2E8), Effect_SetHash4CF988(o->+20, 0x04296E4C);
// then always Obj_AddToList36341C(o) (r3 = its result). Off, from a state
// other than 0 / 3: the same vslot call, state 3, the line +200 and the
// effect 0x048129EB (r3 as the last call leaves it; else the word +160).
DECOMP(0036E2F0, Npc_SetAlert_0036E2F0) {
    const u32 o = arg<u32>(ctx, 0), on = arg<u32>(ctx, 1) & 0xFFu;
    constexpr u32 kC = 0x0036E2E8u;
    if (on != 0) {
        if (((load<u32>(o + 160) >> 1) & 0x7FFFFFFFu) == 0) {
            if (const u32 a = load<u32>(o + 24); a != 0) VCall(ctx, a, 40, {a, 1, 1});
            if (const u32 y = load<u32>(o + 256); y != 0) call(ctx, 0x00364408u, {o, y});
            store<u32>(o + 160, (load<u32>(o + 160) & 1u) | 2u);
            // r6 = the constant's address, as the lfs leaves it
            callf(ctx, 0x004A5C90u, {load<u32>(o + 28), load<u32>(o + 192), 0, kC}, {Lfs47(kC)});
            if (const u32 e = load<u32>(o + 20); e != 0) call(ctx, 0x004CF988u, {e, 0x04296E4Cu});
        }
        call(ctx, 0x0036341Cu, {o});
        return;
    }
    const u32 w = load<u32>(o + 160);
    const u32 s = (w >> 1) & 0x7FFFFFFFu;
    if (s == 0 || s == 3) {
        ret(ctx, w);
        return;
    }
    if (const u32 a = load<u32>(o + 24); a != 0) VCall(ctx, a, 40, {a, 1, 1});
    store<u32>(o + 160, (load<u32>(o + 160) & 1u) | 6u);
    const u32 v = load<u32>(o + 200);
    callf(ctx, 0x004A5C90u, {load<u32>(o + 28), v, 0, v}, {Lfs47(kC)});
    if (const u32 e = load<u32>(o + 20); e != 0) call(ctx, 0x004CF988u, {e, 0x048129EBu});
}
