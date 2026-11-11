// Game code, part 40 (continued): the small-function sweep, 86
// instructions and up. Each was checked against the original in the PPC
// simulator (memory writes, calls and their arguments, r3 and f1).
#include "obj40.h"

using namespace sly4;
using namespace sly4::obj40;

// ---------------------------------------------------------------- sweep: 86 instructions
// u32 f(T* o, u8 on) (f_00041B78): unless the u16 *(*0x00F65CA0 +
// 0x00128214) is set, for state 25 (o->+4): o->+656 / o->+660 get
// p->vt[40](p, !on, 1) and every one of the u16 o->+316 112-byte entries'
// +80 effect (o->+308) Effect_SetHash4CF988(e, on ? 0x8C3F8D2E :
// 0x28298107). r3 as the original leaves it.
DECOMP(00041A08, Obj_SetGlow_00041A08) {
    const u32 o = arg<u32>(ctx, 0), on = arg<u32>(ctx, 1) & 0xFFu;
    const u32 busy = load<u16>(load<u32>(0x00F65CA0u) + 0x00128214u);
    ctx.gpr[3] = busy;
    if (busy != 0) return;
    const u32 s = load<u32>(o + 4);
    ctx.gpr[3] = s;
    if (s != 25) return;
    for (u32 off : {656u, 660u}) {
        const u32 p = load<u32>(o + off);
        if (p != 0) VCall(ctx, p, 40, {p, on == 0 ? 1u : 0u, 1});
    }
    for (u32 i = 0;; ++i) {
        const u32 n = load<u16>(o + 316);
        ctx.gpr[3] = n;
        if (!(i < n)) break;
        const u32 e = load<u32>(load<u32>(o + 308) + 112 * i + 80);
        if (e == 0) continue;
        call(ctx, 0x004CF988u, {e, on != 0 ? 0x8C3F8D2Eu : 0x28298107u});
    }
}

// T* f(?, XmlCursor* x) (no direct caller): a 24-byte node (Mem_Alloc(24,
// 0, 16); r3 = 0 on failure): Obj_InitVt_B21C98(p, x), +12 / +16 = 0, u8
// +22 = 0, vtable 0x00B0F068, u8 +20 / +21 = 1; hashes 0x7A30D7E3 -> +16
// and 0x7F247172 -> +12 (default 0; when either is missing the bool
// 0xCBF1C4F3 -> +20, default 1), bools 0xD6F80AE3 -> +21 and 0x95EC9D6C ->
// +22 (defaults the current values). r3 = p.
DECOMP(0006B7C0, XmlNode24_New_0006B7C0) {
    const u32 x = arg<u32>(ctx, 1);
    const u32 p = static_cast<u32>(call(ctx, 0x0054ED38u, {24, 0, 16}));
    if (p == 0) {
        ret(ctx, 0u);
        return;
    }
    call(ctx, 0x004BB3B0u, {p, x});
    store<u32>(p + 12, 0);
    store<u32>(p + 16, 0);
    store<u8>(p + 22, 0);
    store<u32>(p, 0x00B0F068u);
    store<u8>(p + 20, 1);
    store<u8>(p + 21, 1);
    if (static_cast<u32>(call(ctx, 0x004FA14Cu, {x, 0x7A30D7E3u, p + 16, 0})) == 0 ||
        static_cast<u32>(call(ctx, 0x004FA14Cu, {x, 0x7F247172u, p + 12, 0})) == 0)
        call(ctx, 0x004F986Cu, {x, 0xCBF1C4F3u, p + 20, 1});
    call(ctx, 0x004F986Cu, {x, 0xD6F80AE3u, p + 21, load<u8>(p + 21)});
    call(ctx, 0x004F986Cu, {x, 0x95EC9D6Cu, p + 22, load<u8>(p + 22)});
    ret(ctx, p);
}

// u32 f(T* o, Effect* x, U* y) (no direct caller): with o->+24 >= 0.0 the
// light t (+28 of the component Effect_FindComponent(x, o->+12) with
// c->vt[32](c) == 5, of kind 9) gets the radius sqrt(v) with v = o->+24,
// or o->+20 + (o->+24 - o->+20) * y->+32 / o->+28 when o->+28 >= 1e-5 and
// y->+32 < o->+28; when that changes f32 t->+376: stored,
// Config_CopyLight108(t) unless t->+368 is 2, t->vt[36](t). r3 / f1 as
// the original leaves them.
DECOMP(000774D8, EffectCmd_LightRadius_000774D8) {
    const u32 o = arg<u32>(ctx, 0), x = arg<u32>(ctx, 1), y = arg<u32>(ctx, 2);
    const f64 v0 = Lfs40(o + 24);
    ctx.fpr[1] = v0;
    if (v0 < Lfs40(0x000774CCu)) return;  // 0.0
    const u32 c = static_cast<u32>(call(ctx, 0x004BB4B4u, {x, load<u32>(o + 12)}));
    if (c == 0) return;
    if (static_cast<u32>(VCall(ctx, c, 32, {c})) != 5) return;
    const u32 t = load<u32>(c + 28);
    if (t == 0) return;
    const u32 k = (load<u32>(t + 4) >> 10) & 63u;
    ctx.gpr[3] = k;
    if (k != 9) return;
    ctx.gpr[3] = 0x000774CCu;  // the constants' address, as left
    const f64 d = Lfs40(o + 28);
    f64 v = Lfs40(o + 24);
    if (d >= Lfs40(0x000774D0u)) {  // 1e-5
        const f64 s = Lfs40(y + 32);
        if (s < d) {
            const f64 a = Lfs40(o + 20);
            v = F40(a + F40(F40(F40(v - a) * s) / d));
        }
    }
    v = F40(std::sqrt(v));
    ctx.fpr[1] = v;
    if (Lfs40(t + 376) == v) return;
    const u32 m = load<u32>(t + 368);
    store<f32>(t + 376, static_cast<f32>(v));
    if (m != 2) call(ctx, 0x005913E0u, {t});
    VCall(ctx, t, 36, {t});
}

// u32 f(T* o, a, b) (no direct caller): for the named object n =
// Name_Resolve_002E1220(*0x00F64C20, o->+12): when u8 o->+45 is set, its
// slot byte (SlotRec_Byte464_002D98A4(n)) must be o->+20 and the player's
// definition (((*0x00F65CA0 + 4208)->+8)->+216)->+52 must be o->+16 (when
// that is set); otherwise whether n is (*0x00F64C20)->+40 must equal u8
// o->+44. Then each of the u16 o->+32 handlers h (o->+24) gets
// h->vt[12](h, a, b). r3 as the original leaves it.
DECOMP(0007EE24, EffectCmd_NamedNotify_0007EE24) {
    const u32 o = arg<u32>(ctx, 0), a = arg<u32>(ctx, 1), b = arg<u32>(ctx, 2);
    const u32 n = static_cast<u32>(call(ctx, 0x002E1220u, {load<u32>(0x00F64C20u), load<u32>(o + 12)}));
    if (n == 0) return;
    if (load<u8>(o + 45) == 0) {
        const u32 is = load<u32>(load<u32>(0x00F64C20u) + 40) == n ? 1u : 0u;
        ctx.gpr[3] = is;
        if (is != load<u8>(o + 44)) return;
    } else {
        const u32 dv = static_cast<u32>(call(ctx, 0x002D98A4u, {n})) - load<u32>(o + 20);
        ctx.gpr[3] = dv;
        if (dv != 0) return;
        const u32 k = load<u32>(o + 16);
        if (k != 0) {
            const u32 q = load<u32>(load<u32>(0x00F65CA0u) + 4208 + 8);
            ctx.gpr[3] = q;
            if (q == 0) return;
            const u32 d = load<u32>(q + 216);
            ctx.gpr[3] = d;
            if ((d != 0 ? load<u32>(d + 52) : 0u) != k) return;
        }
    }
    for (u32 i = 0;; ++i) {
        const u32 cnt = load<u16>(o + 32);
        ctx.gpr[3] = cnt;
        if (!(i < cnt)) break;
        const u32 h = load<u32>(load<u32>(o + 24) + 4 * i);
        VCall(ctx, h, 12, {h, a, b});
    }
}

// u32 f(T* o, Effect* x) (no direct caller): with o->+12 set, the target
// *PointMap_Find((*0x00F8F154)->+16, &id) (id = o->+16, or the component
// Effect_FindComponent(x, o->+20)'s c->vt[24](c) when c->vt[32](c) is 2)
// gets a 36-byte command (SmallAlloc(36); VtB20FB0_Ctor_00431748(p,
// o->+12, t, o->+24, ?, u8 o->+32, u8 o->+33; o->+28), r7 as left) pushed
// with CmdList_Push_0042C138(p) (also when the allocation fails). r3 as
// the original leaves it.
DECOMP(0009F5F8, EffectCmd_PushCmd_0009F5F8) {
    const u32 o = arg<u32>(ctx, 0), x = arg<u32>(ctx, 1);
    const u32 a = load<u32>(o + 12);
    ctx.gpr[3] = a;
    if (a == 0) return;
    u32 id = load<u32>(o + 16);
    const u32 k = load<u32>(o + 20);
    ctx.gpr[3] = k;
    if (k != 0) {
        const u32 c = static_cast<u32>(call(ctx, 0x004BB4B4u, {x, k}));
        if (c != 0 && static_cast<u32>(VCall(ctx, c, 32, {c})) == 2) id = static_cast<u32>(VCall(ctx, c, 24, {c}));
    }
    if (id == 0) return;
    u32 t;
    {
        Stack fr(ctx, 16);  // the id at sp+112
        store<u32>(fr.addr(), id);
        const u32 r = static_cast<u32>(call(ctx, 0x0073343Cu, {load<u32>(load<u32>(0x00F8F154u) + 16), fr.addr()}));
        t = load<u32>(r);
    }
    if (t == 0) return;  // r3 = the find result
    const u32 p = static_cast<u32>(call(ctx, 0x0055238Cu, {36}));
    if (p != 0)
        callf(ctx, 0x00431748u, {p, load<u32>(o + 12), t, load<u32>(o + 24), ctx.gpr[7], load<u8>(o + 32), load<u8>(o + 33)}, {Lfs40(o + 28)});
    call(ctx, 0x0042C138u, {p});
}

// T* f(?, XmlCursor* x) (no direct caller): a 28-byte anim command
// (Mem_Alloc(28, 0, 16); r3 = 0 on failure): AnimCmd_ConstructBase(p),
// +12..+20 = 0, u8 +25 = 0, vtable 0x00B11918, u8 +24 = 1; hashes
// 0x0464F605 -> +12, 0x001EFE3C -> +16 (default 0), bools 0x83F172B0 ->
// +24, 0x0138B87F -> +25; with +12 set and Ptr_DerefNonZero(+20) clear:
// RefNode_Release(+20), +20 = Node_PushF65B7C(+12, +16). r3 = p.
DECOMP(000A55C4, AnimCmd_New28_000A55C4) {
    const u32 x = arg<u32>(ctx, 1);
    const u32 p = static_cast<u32>(call(ctx, 0x0054ED38u, {28, 0, 16}));
    if (p == 0) {
        ret(ctx, 0u);
        return;
    }
    call(ctx, 0x004B9CB8u, {p});
    store<u32>(p + 12, 0);
    store<u32>(p + 16, 0);
    store<u32>(p + 20, 0);
    store<u8>(p + 25, 0);
    store<u32>(p, 0x00B11918u);
    store<u8>(p + 24, 1);
    call(ctx, 0x004FA14Cu, {x, 0x0464F605u, p + 12, 0});
    call(ctx, 0x004FA14Cu, {x, 0x001EFE3Cu, p + 16, 0});
    call(ctx, 0x004F986Cu, {x, 0x83F172B0u, p + 24, load<u8>(p + 24)});
    call(ctx, 0x004F986Cu, {x, 0x0138B87Fu, p + 25, load<u8>(p + 25)});
    if (load<u32>(p + 12) != 0 && static_cast<u32>(call(ctx, 0x00446960u, {load<u32>(p + 20)})) == 0) {
        call(ctx, 0x004432DCu, {load<u32>(p + 20)});
        store<u32>(p + 20, static_cast<u32>(call(ctx, 0x00446984u, {load<u32>(p + 12), load<u32>(p + 16)})));
    }
    ret(ctx, p);
}

// u32 f(T* o, Effect* x) (no direct caller): the group key h (the component
// Effect_FindComponent(x, o->+16)'s c->vt[24](c) when c->vt[32](c) is 2 and
// that is nonzero, else o->+12; 0 becomes 0x0A95F036, with
// Hash_ToString(**0x00F6585C) when that is set): the group
// Obj_FindByKey_0042BD0C(h) is released (List_ReleaseAll_0042BF50), a
// 16-byte command (SmallAlloc(16); Ctor_B213A0_0043A620(p, h)) pushed with
// CmdList_Push_0042C138 (also when the allocation fails) and
// Group_Push_0042BAE0(h) (r3 = its).
DECOMP(000A6B4C, EffectCmd_GroupReset_000A6B4C) {
    const u32 o = arg<u32>(ctx, 0), x = arg<u32>(ctx, 1);
    u32 h = 0;
    const u32 k = load<u32>(o + 16);
    if (k != 0) {
        const u32 c = static_cast<u32>(call(ctx, 0x004BB4B4u, {x, k}));
        if (c != 0 && static_cast<u32>(VCall(ctx, c, 32, {c})) == 2) h = static_cast<u32>(VCall(ctx, c, 24, {c}));
    }
    if (h == 0) h = load<u32>(o + 12);
    if (h == 0) {
        h = 0x0A95F036u;
        const u32 s = load<u32>(0x00F6585Cu);
        if (s != 0) call(ctx, 0x0056A6E0u, {load<u32>(s)});
    }
    const u32 f = static_cast<u32>(call(ctx, 0x0042BD0Cu, {h}));
    if (f != 0) call(ctx, 0x0042BF50u, {f});
    const u32 p = static_cast<u32>(call(ctx, 0x0055238Cu, {16}));
    if (p != 0) call(ctx, 0x0043A620u, {p, h});
    call(ctx, 0x0042C138u, {p});
    call(ctx, 0x0042BAE0u, {h});
}

// u32 f(Obj* o) (Obj_EffectOff_0020BB2C): the parts o->vt[68](o) and
// o->vt[64](o) get p->vt[44](p, 1) when set; with a world peer (o->+160,
// WorldObj_GetPeer twice, its +136 set) the peer vector flag 0x20 (byte
// +28) is set to bit 0x80 of u8 +320 of component 32 (Obj_FindComponent(o,
// 32, 0); 0 without). r3 as the original leaves it.
DECOMP(000DD620, Obj_PartsOn_000DD620) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 a = static_cast<u32>(VCall(ctx, o, 68, {o}));
    if (a != 0) VCall(ctx, a, 44, {a, 1});
    const u32 b = static_cast<u32>(VCall(ctx, o, 64, {o}));
    if (b != 0) VCall(ctx, b, 44, {b, 1});
    const u32 w = load<u32>(o + 160);
    ctx.gpr[3] = w;
    if (w == 0) return;
    if (static_cast<u32>(call(ctx, 0x0057A428u, {w})) == 0) return;
    const u32 p = static_cast<u32>(call(ctx, 0x0057A428u, {load<u32>(o + 160)}));
    const u32 k = load<u32>(p + 136);
    ctx.gpr[3] = k;
    if (k == 0) return;
    const u32 c = static_cast<u32>(call(ctx, 0x000D1288u, {o, 32, 0}));
    const u32 w2 = load<u32>(o + 160);
    u32 on = 0;
    if (c != 0) on = (load<u8>(c + 320) >> 7) & 1u;
    const u32 q = static_cast<u32>(call(ctx, 0x0057A428u, {w2}));
    const u32 v = static_cast<u32>(call(ctx, 0x0057BE98u, {q}));
    store<u8>(v + 28, static_cast<u8>((load<u8>(v + 28) & ~0x20u) | (on << 5)));
}

// bool f(Obj* o; f32 dt) (no direct caller): 0 when
// Obj_KeyframeBodies(o; dt) fails; with bit 0x40000000 of o->+568 the
// spline o->+512 is looked up first (the component
// Effect_FindComponent(o->+356, def->+304) (def = o->+216) with
// c->vt[32](c) == 5 whose +28 is of kind 13, else
// Node_FindKind13_004A21F8(o->+160, def->+304), else
// *SplineMap_Find((*0x00F8F158)->+16, &def->+304)) and the bit cleared;
// then sub_001B2A48(o; dt). r3 = 1.
DECOMP(001B2D84, Obj_FollowSpline_001B2D84) {
    const u32 o = arg<u32>(ctx, 0);
    const f64 dt = farg(ctx, 0);
    callf(ctx, 0x000E4720u, {o}, {dt});  // it reads f1 too
    if (static_cast<u32>(ctx.gpr[3]) == 0) {
        ret(ctx, 0u);
        return;
    }
    const u32 w = load<u32>(o + 568);
    const u32 d = load<u32>(o + 216);
    if ((w & 0x40000000u) != 0) {
        const u32 e = load<u32>(o + 356);
        if (e != 0) {
            const u32 c = static_cast<u32>(call(ctx, 0x004BB4B4u, {e, load<u32>(d + 304)}));
            if (c != 0 && static_cast<u32>(VCall(ctx, c, 32, {c})) == 5) {
                const u32 t = load<u32>(c + 28);
                if (t != 0 && ((load<u32>(t + 4) >> 10) & 63u) == 13) store<u32>(o + 512, t);
            }
        }
        u32 s = load<u32>(o + 512);
        if (s == 0) {
            s = static_cast<u32>(call(ctx, 0x004A21F8u, {load<u32>(o + 160), load<u32>(d + 304)}));
            store<u32>(o + 512, s);
        }
        if (s == 0) {
            Stack fr(ctx, 16);  // the key at sp+112
            store<u32>(fr.addr(), load<u32>(d + 304));
            const u32 r = static_cast<u32>(call(ctx, 0x007335C4u, {load<u32>(load<u32>(0x00F8F158u) + 16), fr.addr()}));
            store<u32>(o + 512, load<u32>(r));
        }
        store<u32>(o + 568, load<u32>(o + 568) & ~0x40000000u);
    }
    callf(ctx, 0x001B2A48u, {o}, {dt});
    ret(ctx, 1u);
}

// T::T(o, const Def* d) (DamageColDef_NewBED98): the base Thunk_000E27FC(o,
// d), vtable 0x00B15970; from d: +288..+296 -> +368..+376, +304 / +308 ->
// +384 / +388, +312 -> +392, bit 0x80 of u8 +316 into u8 +396, the 7
// {word, f32} pairs +224.. -> +304..; then +320 -> +400, vtable
// 0x00B16630, +324 -> +404, u8 +328 / +329 -> +408 / +409. r3 / f1 as the
// original leaves them.
DECOMP(001BE8CC, DamageColDef_Ctor_001BE8CC) {
    const u32 o = arg<u32>(ctx, 0), d = arg<u32>(ctx, 1);
    call(ctx, 0x000E27FCu, {o, d});
    store<u32>(o, 0x00B15970u);
    store<u32>(o + 368, load<u32>(d + 288));
    store<u32>(o + 372, load<u32>(d + 292));
    store<u32>(o + 376, load<u32>(d + 296));
    store<u32>(o + 384, load<u32>(d + 304));
    const u32 b = load<u8>(o + 396);
    store<u32>(o + 388, load<u32>(d + 308));
    store<u32>(o + 392, load<u32>(d + 312));
    store<u8>(o + 396, static_cast<u8>((b & 0x7Fu) | (load<u8>(d + 316) & 0x80u)));
    const u32 w320 = load<u32>(d + 320), w324 = load<u32>(d + 324);
    const u8 b328 = load<u8>(d + 328), b329 = load<u8>(d + 329);
    u32 last = 0;
    for (u32 k = 0; k < 7; ++k) {
        store<u32>(o + 304 + 8 * k, load<u32>(d + 224 + 8 * k));
        last = load<u32>(d + 228 + 8 * k);
        store<u32>(o + 308 + 8 * k, last);
    }
    store<u32>(o + 400, w320);
    store<u32>(o, 0x00B16630u);
    store<u32>(o + 404, w324);
    store<u8>(o + 408, b328);
    store<u8>(o + 409, b329);
    ctx.fpr[1] = static_cast<f64>(std::bit_cast<f32>(w324));
    ctx.gpr[3] = b329;
}

// T::T(o, const Def* d) (Owner_New1C0A10): the base
// Owner_Construct1ADD08(o, d), vtables 0x00B168E0 / 0x00B16AB4 (+380) /
// 0x00B16ADC (+384); +556 / +560 = 0, +564 = -1, +568 = d->+240, +572 = 0;
// the flag word +576: bit 0x80000000 cleared, 0x40000000 / 0x20000000 =
// bits 0x80000000 / 0x40000000 of d->+244, 0x10000000 / 0x08000000 /
// 0x04000000 cleared (stored piecewise as the original); +556 = the +28 of
// the 48-byte entry (d->+32)->+400 of the list o->+388 (0 out of range);
// 0x04000000 set again when (d->+32)->+404 is. r3 = 0 / 1 (that flag);
// f1 = d->+240.
DECOMP(001C08AC, Owner_Ctor_001C08AC) {
    const u32 o = arg<u32>(ctx, 0), d = arg<u32>(ctx, 1);
    call(ctx, 0x001ADD08u, {o, d});
    store<u32>(o + 380, 0x00B16AB4u);
    store<u32>(o + 384, 0x00B16ADCu);
    store<u32>(o, 0x00B168E0u);
    u32 b = load<u8>(o + 576);
    store<u32>(o + 556, 0);
    store<u32>(o + 560, 0);
    const u32 f = load<u32>(d + 240);
    b &= 0x7Fu;
    store<u32>(o + 564, 0xFFFFFFFFu);
    store<u32>(o + 568, f);
    store<u32>(o + 572, 0);
    u32 w = load<u32>(o + 576);
    store<u8>(o + 576, static_cast<u8>(b));
    w = (w & ~0xFF000000u) | (b << 24);
    w = (w & ~0x40000000u) | ((load<u32>(d + 244) >> 1) & 0x40000000u);
    store<u32>(o + 576, w);
    w = (w & ~0x20000000u) | ((load<u32>(d + 244) >> 1) & 0x20000000u);
    const u32 top = ((w >> 24) & 0xFFu) & ~0x10u;
    store<u32>(o + 576, w);
    const u32 r4 = top & ~0x08u;
    const u32 l = load<u32>(o + 388);
    store<u8>(o + 576, static_cast<u8>(r4 & ~0x04u));
    const u32 dd = load<u32>(d + 32);
    const u32 idx = load<u32>(dd + 400), on = load<u32>(dd + 404);
    u32 v = 0;
    if (l != 0 && idx < load<u16>(l + 8)) v = load<u32>(load<u32>(l) + 48 * idx + 28);
    store<u32>(o + 556, v);  // stored again either way
    ctx.fpr[1] = static_cast<f64>(std::bit_cast<f32>(f));
    ctx.gpr[3] = on;
    if (on == 0) return;
    store<u8>(o + 576, static_cast<u8>(r4 | 0x04u));
    ret(ctx, 1u);
}

// T* f(a, b, Src* s, c) (XmlDef_NewFromNodeCA038): the 84-byte definition
// on the stack (+0 = 0x00400000, +12 = s->+12, +16 / +20 = 0x00AD366C,
// -1.0 at +36 / +40 / +72, +76 = 0x00AD3670, u8 +80 = 1, zeros elsewhere)
// is read by Def_ReadBase20BFE8(a, b, s, c, &def), then copied into a
// 176-byte object (Mem_Alloc(176, 0, 16); r3 = 0 on failure):
// Thunk_000C2B90(p, &def), u8 +172..+175 = def +80..+83, vtable
// 0x00B19510, String_CopyN(p + 140, def +76, 31). r3 = p.
DECOMP(0020C164, XmlDef_New176_0020C164) {
    const u32 a = arg<u32>(ctx, 0), b = arg<u32>(ctx, 1), s = arg<u32>(ctx, 2), c = arg<u32>(ctx, 3);
    Stack fr(ctx, 96);  // the definition at sp+112 .. sp+195
    const u32 t = fr.addr();
    store<u32>(t + 16, 0x00AD366Cu);
    store<u32>(t + 12, load<u32>(s + 12));
    store<u32>(t + 20, 0x00AD366Cu);
    for (u32 k = 24; k < 32; ++k) store<u8>(t + k, 0);
    store<u32>(t + 32, 0);
    store<u32>(t + 36, 0xBF800000u);
    store<u32>(t + 40, 0xBF800000u);
    store<u8>(t + 44, 0);
    for (u32 k = 48; k < 72; k += 4) store<u32>(t + k, 0);
    store<u32>(t + 72, 0xBF800000u);
    store<u32>(t + 4, 0);
    store<u32>(t + 8, 0);
    store<u32>(t + 76, 0x00AD3670u);
    store<u8>(t + 80, 1);
    store<u8>(t + 81, 0);
    store<u8>(t + 82, 0);
    store<u8>(t + 83, 0);
    store<u32>(t, 0x00400000u);
    call(ctx, 0x0020BFE8u, {a, b, s, c, t});
    const u32 p = static_cast<u32>(call(ctx, 0x0054ED38u, {176, 0, 16}));
    if (p == 0) {
        ret(ctx, 0u);
        return;
    }
    call(ctx, 0x000C2B90u, {p, t});
    store<u8>(p + 172, load<u8>(t + 80));
    store<u8>(p + 173, load<u8>(t + 81));
    store<u32>(p, 0x00B19510u);
    const u8 b82 = load<u8>(t + 82);
    const u32 str = load<u32>(t + 76);
    store<u8>(p + 174, b82);
    store<u8>(p + 175, load<u8>(t + 83));
    call(ctx, 0x00553638u, {p + 140, str, 31});
    ret(ctx, p);
}

// void f() (no direct caller; a static destructor): the tuning variables
// 0x01056E30 / 0x01056DE4 / 0x01056D00 / 0x01056CB4 / 0x01056C68 /
// 0x01056C1C (vtable 0x00B0D99C) and 0x01056D98 / 0x01056D4C (vtable
// 0x00B0D968) and last 0x01056E7C (vtable 0x00B0D934) go through
// TuningVar_DestructBase_004FC1A0; before the last, the array 0x01056C00
// frees its data (alloc +12 ->vt[8](alloc, data, count * size)) and is
// destructed (Array_Destruct). r3 as left.
DECOMP(0024EEDC, Tuning_StaticDtor_0024EEDC) {
    constexpr u32 A = 0x00B0D99Cu, B = 0x00B0D968u;
    const u32 vars[8][2] = {{0x01056E30u, A}, {0x01056DE4u, A}, {0x01056D98u, B}, {0x01056D4Cu, B},
                            {0x01056D00u, A}, {0x01056CB4u, A}, {0x01056C68u, A}, {0x01056C1Cu, A}};
    for (const auto& v : vars) {
        store<u32>(v[0], v[1]);
        call(ctx, 0x004FC1A0u, {v[0]});
    }
    constexpr u32 R = 0x01056C00u;
    const u32 bytes = static_cast<u32>(load<u16>(R)) * ((static_cast<u32>(load<u16>(R + 6)) >> 1) & 0x7FFFu);
    const u32 d = load<u32>(R + 8);
    if (d != 0) {
        const u32 al = load<u32>(R + 12);
        VCall(ctx, al, 8, {al, d, bytes});
    }
    call(ctx, 0x0054EC94u, {R});
    store<u32>(0x01056E7Cu, 0x00B0D934u);
    call(ctx, 0x004FC1A0u, {0x01056E7Cu});
}

// T::~T(o) (no direct caller): vtable 0x00B1CE78, the vectors o + 44 and
// o + 24 released (PtrVector_Clear / Nop), vtable 0x00B1BE30,
// Unlist1056F1C39(o) and List1013B5C_Remove(o) (r3 = its).
DECOMP(002B8798, Obj_Dtor_B1CE78_002B8798) {
    const u32 o = arg<u32>(ctx, 0);
    store<u32>(o, 0x00B1CE78u);
    call(ctx, 0x0054EC1Cu, {o + 44});
    call(ctx, 0x0054EC18u, {o + 44});
    call(ctx, 0x0054EC1Cu, {o + 24});
    call(ctx, 0x0054EC18u, {o + 24});
    store<u32>(o, 0x00B1BE30u);
    Unlist1056F1C39(o);
    call(ctx, 0x000D6940u, {o});
}

// u32 f(T* o, Node* n) (itself, Call_2F466C_12): unless bit 0x8000 of
// n->+164 and u8 o->+16 are both clear, n goes into the pair buffer
// (*(*0x00F65CA0 + 0x001285E8))->+8 (PairBuffer_Push(buf, 0x00F3C868, n))
// unless one of the u8 (n->+208)->+112 slots' (+132, 8 bytes apart)
// entries' items (u16 +52, 8-byte entries +48) has the key 0x64D38214
// (item +4 -> +16 -> +8); then, with u8 o->+17 set, every kind-8 child c
// (+4 >> 10 & 63) of the chain n->+272 (next +268) is visited (recursion).
// r3 = 0.
DECOMP(002F466C, Node_CollectTree_002F466C) {
    const u32 o = arg<u32>(ctx, 0), n = arg<u32>(ctx, 1);
    if ((load<u32>(n + 164) & 0x8000u) != 0 || load<u8>(o + 16) != 0) {
        const u32 m = load<u32>(n + 208);
        const u32 cnt = load<u8>(m + 112);
        bool skip = false;
        for (u32 i = 0; i < cnt && !skip; ++i) {
            const u32 e = load<u32>(m + 8 * i + 132);
            if (e == 0) continue;
            const u32 c2 = load<u16>(e + 52);
            for (u32 j = 0; j < c2; ++j) {
                if (load<u32>(load<u32>(load<u32>(load<u32>(e + 48) + 8 * j + 4) + 16) + 8) == 0x64D38214u) {
                    skip = true;
                    break;
                }
            }
        }
        if (!skip) call(ctx, 0x00564FF8u, {load<u32>(load<u32>(load<u32>(0x00F65CA0u) + 0x001285E8u) + 8), 0x00F3C868u, n});
    }
    if (load<u8>(o + 17) != 0) {
        for (u32 c = load<u32>(n + 272); c != 0;) {
            const u32 k = (load<u32>(c + 4) >> 10) & 63u;
            const u32 nx = load<u32>(c + 268);
            if (k == 8) call(ctx, 0x002F466Cu, {o, c});
            c = nx;
        }
    }
    ret(ctx, 0u);
}
