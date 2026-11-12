// Game code, part 51 (b): the small-function sweep continued with the 113
// instruction functions and up (size order). Each was checked against the
// original in the PPC simulator (memory writes, calls and their arguments,
// r3 and f1).
#include "obj51.h"

using namespace sly4;
using namespace sly4::obj51;

// ---------------------------------------------------------------- sweep: 113 instructions
// T* f(?, XmlCursor* x): a 44-byte def (Mem_Alloc(44, 0, 16); r3 = 0 when
// that fails): Obj_InitVt_B21C98(d, x), vtable 0x00B0F450, +12..+24 = 0,
// +28 = -1.0f, +32 = 2.0f, +36 = 0, +40 = 0x3FC90FDB (pi / 2); then the hash
// 0x8A93973F -> +12 (default 0) and the f32s 0xB7EA3D94, 0x79792193,
// 0xB5AB2880, 0xF1E9AC63, 0x8FA16092, 0x00210E78, 0x5CD304CC -> +16..+40
// (XmlCursor_ReadF32, the old values as defaults). r3 = d.
DECOMP(00072138, Def44_ReadXml_00072138) {
    const u32 x = arg<u32>(ctx, 1);
    const u32 d = static_cast<u32>(call(ctx, 0x0054ED38u, {44, 0, 16}));
    if (d == 0) {
        ret(ctx, 0u);
        return;
    }
    call(ctx, 0x004BB3B0u, {d, x});
    store<u32>(d + 12, 0);
    store<u32>(d + 16, 0);
    store<u32>(d + 20, 0);
    store<u32>(d, 0x00B0F450u);
    store<u32>(d + 24, 0);
    store<u32>(d + 28, 0xBF800000u);
    store<u32>(d + 32, 0x40000000u);
    store<u32>(d + 36, 0);
    store<u32>(d + 40, 0x3FC90FDBu);
    call(ctx, 0x004FA14Cu, {x, 0x8A93973Fu, d + 12, 0});
    static constexpr u32 kKeys[7] = {0xB7EA3D94u, 0x79792193u, 0xB5AB2880u, 0xF1E9AC63u,
                                     0x8FA16092u, 0x00210E78u, 0x5CD304CCu};
    for (u32 k = 0; k < 7; ++k) callf(ctx, 0x004F9B20u, {x, kKeys[k], d + 16 + 4 * k}, {Lfs51(d + 16 + 4 * k)});
    ret(ctx, d);
}

// T* f(?, XmlCursor* x): a 40-byte def (Mem_Alloc(40, 0, 16); r3 = 0 when
// that fails): Obj_InitVt_B21C98(d, x), vtable 0x00B116E8, +12..+32 = 0,
// u8 +36 = u8 +37 = 0; the bool 0x8488565B -> u8 +36 (default 0), the
// hashes 0x7A30D7E3, 0x04B8CC71, 0x001F9B50, 0x93116591, 0x757BAFD6 ->
// +12..+28 (default 0), the f32 0x04DA9951 -> +32 (default the .text pool
// 0x000A36F8) and the bool 0x80116C17 -> u8 +37 (its old value as
// default). r3 = d.
DECOMP(000A3700, Def40_ReadXml_000A3700) {
    const u32 x = arg<u32>(ctx, 1);
    const u32 d = static_cast<u32>(call(ctx, 0x0054ED38u, {40, 0, 16}));
    if (d == 0) {
        ret(ctx, 0u);
        return;
    }
    call(ctx, 0x004BB3B0u, {d, x});
    store<u32>(d + 12, 0);
    store<u32>(d + 16, 0);
    store<u32>(d + 20, 0);
    store<u32>(d + 24, 0);
    store<u32>(d + 28, 0);
    store<u32>(d, 0x00B116E8u);
    const f64 k = Lfs51(0x000A36F8u);
    store<u32>(d + 32, 0);
    store<u8>(d + 36, 0);
    store<u8>(d + 37, 0);
    call(ctx, 0x004F986Cu, {x, 0x8488565Bu, d + 36, 0});
    static constexpr u32 kKeys[5] = {0x7A30D7E3u, 0x04B8CC71u, 0x001F9B50u, 0x93116591u, 0x757BAFD6u};
    for (u32 i = 0; i < 5; ++i) call(ctx, 0x004FA14Cu, {x, kKeys[i], d + 12 + 4 * i, 0});
    callf(ctx, 0x004F9B20u, {x, 0x04DA9951u, d + 32}, {k});
    call(ctx, 0x004F986Cu, {x, 0x80116C17u, d + 37, load<u8>(d + 37)});
    ret(ctx, d);
}

// void f(T* t, Obj* owner, Def* def, u32 a): t->4 = owner, the top bit of
// t->8 cleared, vtable 0x00B11DE0, PtrVector +12 (capacity 4;
// PtrVector_Init(v, 4, 0, 0)), +32 = def, +36 = a, +48 = -1, +40 = +44 =
// +52 = 0. The profile Save_Profile0C9FC8(def->20) goes to +40 when its
// bit 0x00200000 of +20 is set (else Hash_ToString(def->20), for a set
// hash). With a key def->16: +44 = Rec80_Find(owner->160->208->108, key)
// (when that chain is set); without a record +48 = Obj_FindBone0D14DC(
// owner, key) and, when that is -1, +52 = Obj_FindByKey_000D1454(owner,
// key). r3 as the original leaves it.
DECOMP(000B2C00, Link_Init_000B2C00) {
    const u32 t = arg<u32>(ctx, 0), owner = arg<u32>(ctx, 1), def = arg<u32>(ctx, 2), a = arg<u32>(ctx, 3);
    const u32 f8 = load<u32>(t + 8);
    store<u32>(t + 4, owner);
    store<u32>(t + 8, f8 & 0x7FFFFFFFu);
    store<u32>(t, 0x00B11DE0u);
    call(ctx, 0x005516E0u, {t + 12});
    store<u16>(t + 18, 4);
    call(ctx, 0x00551598u, {t + 12, 4, 0, 0});
    store<u32>(t + 32, def);
    store<u32>(t + 36, a);
    store<u32>(t + 48, 0xFFFFFFFFu);
    store<u32>(t + 40, 0);
    store<u32>(t + 44, 0);
    const u32 h = load<u32>(def + 20);
    store<u32>(t + 52, 0);
    const u32 p = static_cast<u32>(call(ctx, 0x000C9FC8u, {h}));
    if (p == 0) {
        if (const u32 h2 = load<u32>(load<u32>(t + 32) + 20); h2 != 0) call(ctx, 0x0056A6E0u, {h2});
    } else {
        const u32 d = load<u32>(t + 32);
        const u32 r = (load<u32>(p + 20) & 0x00200000u) != 0 ? p : 0;
        store<u32>(t + 40, r);
        if (r == 0) call(ctx, 0x0056A6E0u, {load<u32>(d + 20)});
    }
    u32 d = load<u32>(t + 32);
    const u32 key = load<u32>(d + 16);
    if (key == 0) {
        ret(ctx, d);
        return;
    }
    if (const u32 a1 = load<u32>(owner + 160); a1 != 0) {
        if (const u32 a2 = load<u32>(a1 + 208); a2 != 0) {
            const u32 a3 = load<u32>(a2 + 108);
            u32 rec = 0;
            if (a3 != 0) {
                rec = static_cast<u32>(call(ctx, 0x005848CCu, {a3, key}));
                d = load<u32>(t + 32);
            }
            store<u32>(t + 44, rec);
            if (rec != 0) {
                ret(ctx, d);
                return;
            }
        }
    }
    const u32 b = static_cast<u32>(call(ctx, 0x000D14DCu, {owner, load<u32>(d + 16)}));
    store<u32>(t + 48, b);
    if (b != 0xFFFFFFFFu) return;
    store<u32>(t + 52, static_cast<u32>(call(ctx, 0x000D1454u, {owner, load<u32>(load<u32>(t + 32) + 16)})));
}

// bool f(Actor* o, u32 key, u8 a, ?, u8 b, f32 t): with a speaker o->608:
// Speaker_SetLineByKey_00350764(sp, key, 1, 1) and l = o->vslot 188(o,
// key) (r3 = 0 without one); then the line is said with f1 = t, f2 = f3 =
// the .text pool 0x000FCE14: Speaker_Say350958(Anim_Find34CC34(sp, 0), l,
// a, .., .., .., 0, 0) when that finds one, else Speaker_Say3514AC(sp, l,
// a, sp, .., .., 0, 0, 0) (r6..r8 as the callee left them). The 12 floats
// at 0x01164BD0 (+0..8, +16..24, +32..40, +48..56) go to o->624..680 (a
// matrix), u8 o->604 = 1, u8 o->605 = b. r3 = 1, f1 = the last float.
DECOMP(000FCE18, Actor_SayLine_000FCE18) {
    const u32 o = arg<u32>(ctx, 0), key = arg<u32>(ctx, 1);
    const u32 a = arg<u32>(ctx, 2) & 0xFFu, b = arg<u32>(ctx, 4);
    const f64 t = ctx.fpr[1];
    const u32 sp = load<u32>(o + 608);
    if (sp == 0) {
        ret(ctx, 0u);
        return;
    }
    call(ctx, 0x00350764u, {sp, key, 1, 1});
    const u32 l = static_cast<u32>(VCall(ctx, o, 188, {o, key}));
    if (l == 0 || load<u32>(o + 608) == 0) {
        ret(ctx, 0u);
        return;
    }
    const u32 an = static_cast<u32>(call(ctx, 0x0034CC34u, {load<u32>(o + 608), 0}));
    const f64 k = Lfs51(0x000FCE14u);
    if (an != 0) {
        // r6..r8 as Anim_Find34CC34 left them (the callee does not read them)
        callf(ctx, 0x00350958u, {an, l, a, ctx.gpr[6], ctx.gpr[7], ctx.gpr[8], 0, 0}, {t, k, k});
    } else {
        const u32 s = load<u32>(o + 608);
        callf(ctx, 0x003514ACu, {s, l, a, s, ctx.gpr[7], ctx.gpr[8], 0, 0, 0}, {t, k, k});
    }
    constexpr u32 M = 0x01164BD0u;
    Cp50(o + 624, M);
    Cp50(o + 628, M + 4);
    Cp50(o + 632, M + 8);
    Cp50(o + 640, M + 16);
    Cp50(o + 644, M + 20);
    Cp50(o + 648, M + 24);
    store<u8>(o + 604, 1);
    Cp50(o + 656, M + 32);
    store<u8>(o + 605, static_cast<u8>(b));
    Cp50(o + 660, M + 36);
    Cp50(o + 664, M + 40);
    Cp50(o + 672, M + 48);
    Cp50(o + 676, M + 52);
    Cp50(o + 680, M + 56);
    ctx.fpr[1] = Lfs51(M + 56);
    ret(ctx, 1u);
}

// bool f(Path* p): finds the first pair of non-adjacent segments of the
// polyline p (16-byte points p->0, u16 count p->8, both re-read after each
// test) that cross: segment i (points i, i + 1, for i < count - 3) against
// each later segment j (j from i + 2, j < count - 1) longer than the .text
// pool 0x00182B60, by Seg2_IntersectXZ_001829B0(&a, &b, &c, &d, &hit) on
// stack copies; p->56 = p->60 = i and r3 = 1 on a hit, else 0 (also for
// fewer than 3 points). f1 as the original leaves it.
DECOMP(00182B68, Path_FindSelfCross_00182B68) {
    const u32 p = arg<u32>(ctx, 0);
    u32 cnt = load<u16>(p + 8);
    if (cnt < 3) {
        ret(ctx, 0u);
        return;
    }
    const f64 k = Lfs51(0x00182B60u);
    Stack fr(ctx, 80);  // hit sp+112, a +128, b +144, c +160, d +176
    const u32 hit = fr.addr(), va = hit + 16, vb = hit + 32, vc = hit + 48, vd = hit + 64;
    u32 d = load<u32>(p);
    for (u32 i = 0; i < cnt - 3; ++i) {
        for (u32 c = 0; c < 3; ++c) store<u32>(va + 4 * c, load<u32>(d + 16 * i + 4 * c));
        for (u32 c = 0; c < 3; ++c) store<u32>(vb + 4 * c, load<u32>(d + 16 * (i + 1) + 4 * c));
        ctx.fpr[1] = Lfs51(vb + 8);
        for (u32 j = i + 2; j < cnt - 1; ++j) {
            const u32 s0 = d + 16 * j, s1 = d + 16 * (j + 1);
            for (u32 c = 0; c < 3; ++c) Cp50(vc + 4 * c, s0 + 4 * c);
            for (u32 c = 0; c < 3; ++c) Cp50(vd + 4 * c, s1 + 4 * c);
            const f64 x0 = Lfs51(vc), y0 = Lfs51(vc + 4), z0 = Lfs51(vc + 8);
            const f64 x1 = Lfs51(vd), y1 = Lfs51(vd + 4), z1 = Lfs51(vd + 8);
            const f64 dx = F51(x1 - x0), dy = F51(y1 - y0), dz = F51(z1 - z0);
            f64 q = F51(dy * dy);
            q = F51(std::fma(dx, dx, q));
            q = F51(std::fma(dz, dz, q));
            const f64 len = F51(std::sqrt(q));
            ctx.fpr[1] = len;
            if (!(len > k)) continue;
            const u32 r = static_cast<u32>(call(ctx, 0x001829B0u, {va, vb, vc, vd, hit}));
            d = load<u32>(p);
            cnt = load<u16>(p + 8);
            if (r != 0) {
                store<u32>(p + 56, i);
                store<u32>(p + 60, i);
                ret(ctx, 1u);
                return;
            }
        }
    }
    ret(ctx, 0u);
}

// T* f(?, XmlCursor* x, a, Obj* tmpl): a 288-byte def (SmallAlloc(288)):
// Wrapper_0CC4D4(d, a, tmpl, 0, 0x01164BD0, 0x01164B30, 1, 0, 0, .., 0;
// f1 = the .text pool 0x001E6D6C [0]), vtable 0x00B18020, +4 |= 4, +236 =
// 0, +240 = tmpl->432, +256 / +260 / +264 = tmpl->448..456 (0, 0, pool [1]
// without tmpl), +272 = tmpl->464 (0), +8 |= 0x10000000. Then (also for a
// failed allocation, on 0) XmlDef_ReadBase_000CBE10(?, x, d), the hash
// 0x06556689 -> +240, the vec3 0xC88E4511 -> +256 and the hash 0x675C8395
// -> +272 (the old values as defaults). r3 = d.
DECOMP(001E6D78, Def288_ReadXml_001E6D78) {
    const u32 a0 = arg<u32>(ctx, 0), x = arg<u32>(ctx, 1), a = arg<u32>(ctx, 2), tm = arg<u32>(ctx, 3);
    u32 d = static_cast<u32>(call(ctx, 0x0055238Cu, {288}));
    if (d != 0) {
        constexpr u32 K = 0x001E6D6Cu;
        callf(ctx, 0x000CC4D4u, {d, a, tm, 0, 0x01164BD0u, 0x01164B30u, 1, 0, 0, 0, 0}, {Lfs51(K)});
        const u32 f4 = load<u32>(d + 4);
        store<u32>(d + 236, 0);
        const u32 f8 = load<u32>(d + 8);
        store<u32>(d, 0x00B18020u);
        store<u32>(d + 4, f4 | 4u);
        store<u32>(d + 240, tm != 0 ? load<u32>(tm + 432) : 0);
        u32 z = K + 4;
        if (tm == 0) {
            store<u32>(d + 256, 0);
            store<u32>(d + 260, 0);
        } else {
            Cp50(d + 256, tm + 448);
            Cp50(d + 260, tm + 452);
            z = tm + 456;
        }
        ctx.fpr[1] = Lfs51(z);
        Cp50(d + 264, z);
        store<u32>(d + 272, tm != 0 ? load<u32>(tm + 464) : 0);
        store<u32>(d + 8, f8 | 0x10000000u);
    }
    call(ctx, 0x000CBE10u, {a0, x, d});
    call(ctx, 0x004FA14Cu, {x, 0x06556689u, d + 240, load<u32>(d + 240)});
    call(ctx, 0x004F9E0Cu, {x, 0xC88E4511u, d + 256, d + 256});
    call(ctx, 0x004FA14Cu, {x, 0x675C8395u, d + 272, load<u32>(d + 272)});
    ret(ctx, d);
}

// T* f(A* a, B* b): a 448-byte 0x0020A688 object built from a stack query:
// Obj_Init22(q, 0, 0, a->vslot 32(a, b), a, 0, vt 0x00B193B8, 0x01164BD0,
// 0x01164B30, 0, 0, 0, 1, 1, 0 x4, the u32 -1; f1 = f2 = the .text pool
// 0x0020A9C0), q->144 = 0, u8 q->148 = 0, q->12 |= 0x80, q->20 |= 0x280,
// b->vslot 8(b, q); m = SmallAlloc(448) and the ctor when that worked;
// m->vslot 8(m) (on null too, as the original); then q's lazily allocated
// 20-byte PtrVector (+0) is freed. r3 = m.
DECOMP(0020A9C8, Obj20A688_Spawn_0020A9C8) {
    const u32 a = arg<u32>(ctx, 0), b = arg<u32>(ctx, 1);
    const u32 v = static_cast<u32>(VCall(ctx, a, 32, {a, b}));
    const f64 k = Lfs51(0x0020A9C0u);
    Stack fr(ctx, 192);  // the query at sp+224..415 (frame of 416 bytes)
    const u32 q = fr.addr();
    // clang-format off
    callf(ctx, 0x000CDFB0u, {q, 0, 0, v, a, 0, 0x00B193B8u, 0x01164BD0u, 0x01164B30u, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0xFFFFFFFFu, 0, 0, 0}, {k, k});
    // clang-format on
    const u32 f12 = load<u32>(q + 12), f20 = load<u32>(q + 20);
    store<u32>(q + 144, 0);
    store<u8>(q + 148, 0);
    store<u32>(q + 12, f12 | 0x80u);
    store<u32>(q + 20, f20 | 0x280u);
    VCall(ctx, b, 8, {b, q});
    const u32 m = static_cast<u32>(call(ctx, 0x0055238Cu, {448}));
    u32 r = 0;
    if (m != 0) {
        call(ctx, 0x0020A688u, {m, q});
        r = m;
    }
    VCall(ctx, r, 8, {r});
    FreeVec0_50(ctx, q);
    ret(ctx, r);
}

// u32 f(T* o, u8 sel, u8 on): for each group g of o (o->28, u16 count
// o->36): each set item (g->8, 8-byte entries, u16 count g->16) gets
// item->vslot 88(item, cur, on, 1, 1) with cur = the item being g's current
// one (g->56, unless bit 0x80 of u8 g->60); g's head g->52 (when set) gets
// head->vslot 88(head, x, on, 1, 1) with x = 1 when sel is set and the
// current entry (g->56, or the last one with bit 0x80) of the table g->28
// is negative or set with a clear u8 +60. r3 = o's group count.
DECOMP(00267CCC, Groups_Activate_00267CCC) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 sel = arg<u32>(ctx, 1) & 0xFFu, on = arg<u32>(ctx, 2) & 0xFFu;
    u32 n;
    for (u32 i = 0;; ++i) {
        n = load<u16>(o + 36);
        if (!(i < n)) break;
        const u32 g = load<u32>(load<u32>(o + 28) + 4 * i);
        u32 m;
        for (u32 j = 0;; ++j) {
            m = load<u16>(g + 16);
            if (!(j < m)) break;
            const u32 it = load<u32>(load<u32>(g + 8) + 8 * j);
            if (it == 0) continue;
            u32 cur = 0;
            if ((load<u8>(g + 60) & 0x80u) == 0 && load<u32>(g + 56) == j) cur = 1;
            VCall(ctx, it, 88, {it, cur, on, 1, 1});
        }
        const u32 h = load<u32>(g + 52);
        if (h == 0) continue;
        u32 idx = m - 1;
        if ((load<u8>(g + 60) & 0x80u) == 0) idx = load<u32>(g + 56);
        u32 x = 0;
        if (sel != 0) {
            if (static_cast<s32>(idx) < 0) {
                x = 1;
            } else if (const u32 e = load<u32>(load<u32>(g + 28) + (idx << 3)); e != 0 && load<u8>(e + 60) == 0) {
                x = 1;
            }
        }
        VCall(ctx, h, 88, {h, x, on, 1, 1});
    }
    ret(ctx, n);
}

// The countdown tail of 0x002914D0: the timer at the entry of o->8's s8
// slot +172 in the 0x20C80-byte blocks of *(0x00F65CA0) (+4208; f32 +108,
// flags +0) drops by rate * dt, floored at the pool value lo; flags |=
// 0x40; Obj_SetField20_00276558(o, 0) when it reached lo. f1 = the
// unfloored value.
static void Countdown51(PPUContext& ctx, u32 o, u32 tgt, f64 rate, f64 dt, f64 lo) {
    const s32 slot = static_cast<s8>(load<u8>(tgt + 172));
    const u32 e = load<u32>(0x00F65CA0u) + static_cast<u32>(slot) * 0x00020C80u + 4208;
    const f64 v = F51(-std::fma(rate, dt, -Lfs51(e + 108)));
    ctx.fpr[1] = v;
    const f64 c = v < lo ? lo : v;
    store<f32>(e + 108, static_cast<f32>(c));
    store<u32>(e, load<u32>(e) | 0x40u);
    if (c == lo) call(ctx, 0x00276558u, {o, 0});
}

// void f(T* o, f32 dt): for state 1 (top 2 bits of o->24): o->32 += dt;
// with a duration o->36 >= the .text pool 0x002914C8 that has run out, it
// ends (Obj_SetField20_00276558(o, 0)) unless the target o->8 (one with
// bit 0x8 of +192) has a time +348 above the pool value. Otherwise kind 1 /
// 7 (bits 25..29 of o->24) with bit 0 of the target's +188 counts the
// target's timer down by o->192 / o->348 (Countdown51). r3 / f1 as the
// original leaves them.
DECOMP(002914D0, Timer_Tick_002914D0) {
    const u32 o = arg<u32>(ctx, 0);
    const f64 dt = ctx.fpr[1];
    const u32 f = load<u32>(o + 24);
    if ((f >> 30) != 1) return;
    const u32 tgt = load<u32>(o + 8);
    const u32 obj = tgt != 0 && (load<u32>(tgt + 192) & 0x8u) != 0 ? tgt : 0;
    const f64 dur = Lfs51(o + 36);
    const f64 t = F51(Lfs51(o + 32) + dt);
    store<f32>(o + 32, static_cast<f32>(t));
    const f64 lo = Lfs51(0x002914C8u);
    if (dur >= lo && t >= dur && (obj == 0 || !(Lfs51(obj + 348) <= lo))) {
        call(ctx, 0x00276558u, {o, 0});
        return;
    }
    const s32 kind = static_cast<s32>((f >> 25) & 0x1Fu);
    if (kind > 1) {
        if (kind != 7 || (load<u32>(tgt + 188) & 1u) == 0) return;
        Countdown51(ctx, o, tgt, Lfs51(o + 348), dt, lo);
    } else {
        if (kind != 1 || (load<u32>(tgt + 188) & 1u) == 0) return;
        Countdown51(ctx, o, tgt, Lfs51(o + 192), dt, lo);
    }
}

// T* f(XmlCursor* doc, XmlCursor* x, u8 flag): as 0x002AF0D4 with kind 41
// and vtable 0x00B1CC00 (+24 left): u8 +16 = flag, the hash 0x9178C4CB ->
// +20 (its old value as default) and the bool 0xCC2ABC8B -> +24 (default
// 0).
DECOMP(002B1628, Rec28b_ReadXml_002B1628) {
    const u32 doc = arg<u32>(ctx, 0), x = arg<u32>(ctx, 1), flag = arg<u32>(ctx, 2);
    const u32 attr = static_cast<u32>(call(ctx, 0x004F9628u, {x, 0x00AD4C0Cu}));
    if (attr == 0) {
        call(ctx, 0x004F8944u, {doc});
        ret(ctx, 0u);
        return;
    }
    const u32 p = Rec28New51(ctx, attr, 41, 0x00B1CC00u, false);
    store<u8>(p + 16, static_cast<u8>(flag));
    call(ctx, 0x004FA14Cu, {x, 0x9178C4CBu, p + 20, load<u32>(p + 20)});
    call(ctx, 0x004F986Cu, {x, 0xCC2ABC8Bu, p + 24, 0});
    ret(ctx, p);
}

// u32 f(T* o): after 0x002EA92C(o) (r3 = 0 when it fails) the o->72 * 2
// words at o->40 are byte-swapped in place (the count re-read per word);
// o's 16-byte PtrVector +76 (u16 capacity +80, count +84, high-water +86)
// gets room for o->72 (PtrVector_Reserve), its count reset, and an entry
// {w0, 0, (u64) w1 + o->64 (u64)} per 8-byte record {w0, w1} of o->40;
// then 0x002EA728(o, 4 * o->72, o->56 (u64)) and, when that worked, o->32
// = 4 (r3 = 4; else 0).
DECOMP(002EB070, Table_Load_002EB070) {
    const u32 o = arg<u32>(ctx, 0);
    if (static_cast<u32>(call(ctx, 0x002EA92Cu, {o})) == 0) return;
    u32 n = load<u32>(o + 72);
    u32 src = load<u32>(o + 40);
    if ((n << 1) != 0) {
        for (u32 k = 0;;) {
            const u32 w = src + 4 * k;
            ++k;
            const u32 b0 = load<u8>(w), b1 = load<u8>(w + 1), b2 = load<u8>(w + 2), b3 = load<u8>(w + 3);
            store<u32>(w, (b3 << 24) | (b2 << 16) | (b1 << 8) | b0);
            n = load<u32>(o + 72);
            if (!(k < (n << 1))) break;
        }
    }
    if (n != 0) {
        call(ctx, 0x00551718u, {o + 76, n});
        n = load<u32>(o + 72);
    }
    store<u16>(o + 84, 0);
    u32 cnt = 0, r3v = 0;  // the count as kept in r25 / r3 between entries
    for (u32 m = 0; n != 0;) {
        const u32 w0 = load<u32>(src);
        const u64 addr = static_cast<u64>(load<u32>(src + 4)) + load<u64>(o + 64);
        const u32 cap = load<u16>(o + 80);
        if ((cnt & 0xFFFFu) == cap) {
            call(ctx, 0x00551718u, {o + 76, (cap << 1) & 0x1FFFEu});
            r3v = load<u16>(o + 84);
        }
        const u32 e = load<u32>(o + 76) + ((r3v << 4) & 0xFFFF0u);
        store<u64>(e + 8, addr);
        const u32 c1 = load<u16>(o + 84) + 1u;
        store<u32>(e, w0);
        const u32 hi = load<u16>(o + 86);
        store<u32>(e + 4, 0);
        store<u16>(o + 84, static_cast<u16>(c1));
        n = load<u32>(o + 72);
        cnt = static_cast<s32>(hi) > static_cast<s32>(c1 & 0xFFFFu) ? (c1 & 0xFFFFu) : c1;
        r3v = c1;
        store<u16>(o + 86, static_cast<u16>(static_cast<s32>(hi) > static_cast<s32>(c1 & 0xFFFFu) ? hi : c1));
        ++m;
        src += 8;
        if (!(m < n)) break;
    }
    if (static_cast<u32>(call(ctx, 0x002EA728u, {o, n << 2, load<u64>(o + 56)})) == 0) return;
    store<u32>(o + 32, 4);
    ret(ctx, 4u);
}

// void f(T* o): tears down o's groups (o->52, u16 count o->60; each set
// group g, 60 bytes, holds the sets g->36 (u16 count g->44; each set h, 88
// bytes, holds the items h->64, u16 count h->72): every set item gets
// item->vslot 4(item), then each PtrVector is cleared (ClearVec50) and the
// set / group freed (SmallFree(h, 88), SmallFree(g, 60)); finally o's
// vector +52 is cleared. r3 as the last call leaves it.
DECOMP(002F6054, Groups_Destroy_002F6054) {
    const u32 o = arg<u32>(ctx, 0);
    for (u32 i = 0; i < load<u16>(o + 60); ++i) {
        const u32 g = load<u32>(load<u32>(o + 52) + 4 * i);
        if (g == 0) continue;
        for (u32 j = 0; j < load<u16>(g + 44); ++j) {
            const u32 h = load<u32>(load<u32>(g + 36) + 4 * j);
            if (h == 0) continue;
            for (u32 k = 0; k < load<u16>(h + 72); ++k) {
                const u32 e = load<u32>(load<u32>(h + 64) + 4 * k);
                if (e != 0) VCall(ctx, e, 4, {e});
            }
            ClearVec50(ctx, h + 64);
            call(ctx, 0x00552420u, {h, 88});
        }
        ClearVec50(ctx, g + 36);
        call(ctx, 0x00552420u, {g, 60});
    }
    ClearVec50(ctx, o + 52);
}

// T* f(T* o, a, Obj* b, c): o->0 = 0, +4 = a, +8 = b, +12 = c; four
// Ctor_B21B88_004A0DB0 members (+48, +84, +120, +156); Vec3_Set_004A11CC(
// o + 192, .., f1 = f2 = the .text pool 0x0030581C [0], f3 = *(0x0106A178));
// +16..+44 = 0; Obj_AttachCam_003055E8(o, 0, o + 16, 1; f1 = pool [1]).
// Then o->0 = a new 112-byte camera link (SmallAlloc(112); 0 when that
// fails): vtable 0x00B13B38, +4 = b, u8 +8 = 0, the 4x3 rows of b->96
// (+0..8, +16..24, +32..40, +48..56) at +16.., +80 = b->356, the 9 top
// bits of +88 set to 0 / 1, +84 = +92 = +96 = +100 = 0. r3 = the link, f1
// = b->356.
DECOMP(00305828, CamLink_Init_00305828) {
    const u32 o = arg<u32>(ctx, 0), a = arg<u32>(ctx, 1), b = arg<u32>(ctx, 2), c = arg<u32>(ctx, 3);
    constexpr u32 K = 0x0030581Cu;
    store<u32>(o, 0);
    store<u32>(o + 4, a);
    store<u32>(o + 8, b);
    store<u32>(o + 12, c);
    call(ctx, 0x004A0DB0u, {o + 48});
    call(ctx, 0x004A0DB0u, {o + 84});
    call(ctx, 0x004A0DB0u, {o + 120});
    call(ctx, 0x004A0DB0u, {o + 156});
    const f64 k0 = Lfs51(K);
    // r4 = 0x0106A130 as the address computation leaves it (the callee does not read it)
    callf(ctx, 0x004A11CCu, {o + 192, 0x0106A130u}, {k0, k0, Lfs51(0x0106A178u)});
    for (u32 k = 0; k < 8; ++k) store<u32>(o + 16 + 4 * k, 0);
    callf(ctx, 0x003055E8u, {o, 0, o + 16, 1}, {Lfs51(K + 4)});
    const u32 p = static_cast<u32>(call(ctx, 0x0055238Cu, {112}));
    if (p != 0) {
        const u32 s = load<u32>(o + 8);
        const u32 m = s + 96;
        const u32 f = load<u32>(s + 356);
        store<u32>(p + 4, s);
        store<u8>(p + 8, 0);
        store<u32>(p, 0x00B13B38u);
        Cp50(p + 16, m);
        Cp50(p + 20, m + 4);
        Cp50(p + 24, m + 8);
        Cp50(p + 32, m + 16);
        Cp50(p + 36, m + 20);
        Cp50(p + 40, m + 24);
        Cp50(p + 48, m + 32);
        Cp50(p + 52, m + 36);
        const u32 w88 = load<u32>(p + 88);
        Cp50(p + 56, m + 40);
        Cp50(p + 64, m + 48);
        Cp50(p + 68, m + 52);
        store<u32>(p + 80, f);
        Cp50(p + 72, m + 56);
        store<u32>(p + 92, 0);
        store<u32>(p + 88, (w88 & 0x7FFFFFFFu & ~0x7F800000u) | (1u << 23));  // rlwinm 0,1,31 + rlwimi of 1 into bits 1..8
        store<u32>(p + 96, 0);
        store<u32>(p + 100, 0);
        store<u32>(p + 84, 0);
        ctx.fpr[1] = static_cast<f64>(std::bit_cast<f32>(f));
    }
    store<u32>(o, p);
    ret(ctx, p);
}

// T* f(?, XmlCursor* x, Obj* owner): reads the kind 0x368328E5 (s32, 0
// unless 0..8), the hash 0x75751B32, the hash 0x2A7C3553 (its index in
// the 8-word table 0x00ADB81C, or -1) and the f32 0x0B3E876F (default
// the .text pool 0x00340CC0 [0], clamped by fsel against pool [0] and the
// doubles pool +8 / +16); a 2588-byte object (SmallAlloc(2588); r3 = 0
// when that fails) built by Obj_Construct_00769BD8(p, owner) gets them at
// +2572..+2584 and vtable 0x00B1F138. r3 = p.
DECOMP(00340CD8, ObjKind_ReadXml_00340CD8) {
    const u32 x = arg<u32>(ctx, 1), owner = arg<u32>(ctx, 2);
    Stack fr(ctx, 16);  // kind sp+112, hash +116, hash2 +120, f32 +124
    const u32 v = fr.addr();
    store<u32>(v, 0);
    call(ctx, 0x004F99BCu, {x, 0x368328E5u, v, 0});
    if (const s32 kd = static_cast<s32>(load<u32>(v)); kd < 0 || !(kd < 9)) store<u32>(v, 0);
    store<u32>(v + 4, 0);
    call(ctx, 0x004FA14Cu, {x, 0x75751B32u, v + 4, 0});
    store<u32>(v + 8, 0);
    call(ctx, 0x004FA14Cu, {x, 0x2A7C3553u, v + 8, 0});
    const u32 h2 = load<u32>(v + 8);
    u32 idx = 0xFFFFFFFFu;
    for (u32 i = 0; i < 8; ++i)
        if (load<u32>(0x00ADB81Cu + 4 * i) == h2) {
            idx = i;
            break;
        }
    constexpr u32 K = 0x00340CC0u;
    const f64 k0 = Lfs51(K);
    store<u32>(v + 12, 0x3F800000u);
    callf(ctx, 0x004F9B20u, {x, 0x0B3E876Fu, v + 12}, {k0});
    const f64 f = Lfs51(v + 12);
    const f64 d = F51(k0 - f);
    f64 r = F51(d >= 0.0 ? f : load<f64>(K + 8));
    r = F51(r >= 0.0 ? r : load<f64>(K + 16));
    store<f32>(v + 12, static_cast<f32>(r));
    ctx.fpr[1] = r;
    const u32 p = static_cast<u32>(call(ctx, 0x0055238Cu, {2588}));
    if (p == 0) {
        ret(ctx, 0u);
        return;
    }
    const u32 kd = load<u32>(v), h1 = load<u32>(v + 4), fv = load<u32>(v + 12);
    call(ctx, 0x00769BD8u, {p, owner});
    store<u32>(p + 2572, kd);
    store<u32>(p + 2576, h1);
    store<u32>(p + 2580, idx);
    store<u32>(p + 2584, fv);
    store<u32>(p, 0x00B1F138u);
    ret(ctx, p);
}

// void f(T* o, ?, ?, r6): u8 o->363 = 0; Handle_Call363A3C(0x811E9BA0, 0,
// 1, r6 (the caller's), 0, 0, 0x01164CA4; f1 = the .text pool 0x003FE74C);
// SetSelect_491000(o, 0xE3B75305); o->300->vslot 88(.., 1, 1, 1, 1) and,
// when it has the child 0x435538CB (Obj_FindByHash_000DB290), that
// child's vslot 88(.., 0, 1, 1, 1); the camera at o->vslot 76(o) + 0x1281E0
// (when set and not yet on o->60, Obj_HasWord368_000FC5DC) gets
// Camera_PushTarget0FC040(cam, o->60, .., 0; f1 = pool); then
// Handle_Call363A3C(0x569F957F, 0, 1, .., 0, 0, 0x01164CA4; f1 = pool)
// (r3 = its; r5 / r6 as the previous call left them).
DECOMP(003FE750, Obj_EnterState_003FE750) {
    const u32 o = arg<u32>(ctx, 0);
    const u64 r6 = ctx.gpr[6];
    constexpr u32 H = 0x01164CA4u;
    store<u8>(o + 363, 0);
    const f64 k = Lfs51(0x003FE74Cu);
    // r6 is the caller's (the callee does not read it)
    callf(ctx, 0x0036735Cu, {0x811E9BA0u, 0, 1, r6, 0, 0, H}, {k});
    call(ctx, 0x00491000u, {o, 0xE3B75305u});
    const u32 p = load<u32>(o + 300);
    VCall(ctx, p, 88, {p, 1, 1, 1, 1});
    if (static_cast<u32>(call(ctx, 0x000DB290u, {load<u32>(o + 300), 0x435538CBu})) != 0) {
        const u32 q = static_cast<u32>(call(ctx, 0x000DB290u, {load<u32>(o + 300), 0x435538CBu}));
        VCall(ctx, q, 88, {q, 0, 1, 1, 1});
    }
    const u32 r = static_cast<u32>(VCall(ctx, o, 76, {o}));
    if (const u32 cam = load<u32>(r + 0x001281E0u); cam != 0) {
        if (static_cast<u32>(call(ctx, 0x000FC5DCu, {cam, load<u32>(o + 60)})) == 0) {
            // r5 as Obj_HasWord368_000FC5DC left it (the callee does not read it)
            callf(ctx, 0x000FC040u, {cam, load<u32>(o + 60), ctx.gpr[5], 0}, {k});
        }
    }
    // r6 as the previous call left it (the callee does not read it)
    callf(ctx, 0x0036735Cu, {0x569F957Fu, 0, 1, ctx.gpr[6], 0, 0, H}, {k});
}

// u32 f(Net* o, Cfg* c): vtable 0x00B20CB0, +4 = +12 = 0, u16 +8 = 0, +16
// = c->4, +20 = +24 = 0, +32 = *(0x0108A1AC); the console's IP address
// (cellNetCtlGetInfo(16, &info) parsed with "%d.%d.%d.%d" (sscanf), as
// four bytes; 0 when unavailable) at +4, the u16 port c->0 at +8;
// +28 = NetServer_Open_004274D0(o + 4); then c->4 12-byte slots (Mem_Alloc(
// 12 n, 0, 16)) at +24, each {0, u16 0, 0} (a null slot address skipped).
// r3 as the original leaves it.
DECOMP(00427A0C, Net_Init_00427A0C) {
    const u32 o = arg<u32>(ctx, 0), c = arg<u32>(ctx, 1);
    store<u32>(o, 0x00B20CB0u);
    store<u32>(o + 4, 0);
    store<u32>(o + 12, 0);
    const u32 n0 = load<u32>(c + 4);
    store<u16>(o + 8, 0);
    store<u32>(o + 16, n0);
    store<u32>(o + 20, 0);
    store<u32>(o + 24, 0);
    Stack frame(ctx, 288);
    const u32 b = frame.addr();  // sp+112: the bytes; +8..+20: the four ints; +24: the info union
    const u32 v = b + 8, info = b + 24;
    const u32 f32w = load<u32>(0x0108A1ACu);
    store<u32>(b, 0);
    store<u32>(o + 32, f32w);
    ctx.fpr[1] = Lfs51(0x0108A1ACu);
    // r5 = 0x0108A164 is left over from the address computation
    if (static_cast<u32>(call(ctx, 0x00ABDE44u, {16, info, 0x0108A164u})) == 0 &&
        static_cast<u32>(call(ctx, 0x0071EC78u, {info, 0x00AE212Cu, v, v + 4, v + 8, v + 12})) == 4) {
        store<u16>(b + 4, 0);
        store<u8>(b, static_cast<u8>(load<u32>(v)));
        store<u8>(b + 1, static_cast<u8>(load<u32>(v + 4)));
        store<u8>(b + 2, static_cast<u8>(load<u32>(v + 8)));
        store<u8>(b + 3, static_cast<u8>(load<u32>(v + 12)));
    }
    store<u32>(o + 4, load<u32>(b));
    const u32 port = load<u16>(c);
    store<u16>(o + 8, static_cast<u16>(port));
    // r4 = the port as the copy leaves it (the callee does not read it)
    store<u32>(o + 28, static_cast<u32>(call(ctx, 0x004274D0u, {o + 4, port})));
    const u32 n = load<u32>(o + 16);
    if (n == 0) return;
    const u64 p = call(ctx, 0x0054ED38u, {static_cast<u64>(n) * 12, 0, 16});
    store<u32>(o + 24, static_cast<u32>(p));
    const u32 m = load<u32>(o + 16);
    u64 e = p;
    for (u32 k = 0; k < m; ++k, e += 12) {
        const u32 s = static_cast<u32>(e);
        if (s == 0) continue;
        store<u32>(s, 0);
        store<u16>(s + 4, 0);
        store<u32>(s + 8, 0);
    }
    ctx.gpr[3] = e;
}

// void f(Emitter* o): with u8 o->13 clear, when o's sound state (+16 ..
// +72) differs from the last one sent (+76 ..: f32 +16 / +32 / +36 more
// than the .text pool 0x00442248 [1] apart, words +60 / +64 / +52, u8 +72
// and, while o->4->36 is above pool [0], f32 +56) and the voice o->0 is
// playing (Voice_IsPlaying): a 220-byte param block (memset, +0 = 220)
// from Emitter_BuildParams(&blk, o + 16, o->4, o + 76) is sent
// (Voice_SetParams_006D612C(o->0, &blk)) after the state is copied to +76
// (memmove 57 bytes). u8 o->192 = 0. r3 / f1 as the original leaves them.
DECOMP(00442250, Emitter_Update_00442250) {
    const u32 o = arg<u32>(ctx, 0);
    constexpr u32 K = 0x00442248u;
    const f64 k0 = Lfs51(K);
    const bool loud = Lfs51(load<u32>(o + 4) + 36) > k0;
    const u32 voice = load<u32>(o);
    ctx.fpr[1] = k0;
    ctx.gpr[3] = voice;
    if (load<u8>(o + 13) == 0) {
        const f64 k1 = Lfs51(K + 4);
        ctx.fpr[1] = k1;
        auto near = [&](u32 a, u32 b) { return std::fabs(F51(Lfs51(o + a) - Lfs51(o + b))) < k1; };
        bool same = near(16, 76) && near(32, 92) && near(36, 96) && load<u32>(o + 60) == load<u32>(o + 120) &&
                    load<u32>(o + 64) == load<u32>(o + 124) && load<u32>(o + 52) == load<u32>(o + 112) &&
                    load<u8>(o + 72) == load<u8>(o + 132) && (!loud || near(56, 116));
        if (!same && static_cast<u32>(call(ctx, 0x006D5E10u, {voice})) != 0) {
            Stack fr(ctx, 224);  // the param block at sp+112..331
            const u32 blk = fr.addr();
            call(ctx, 0x00726E54u, {blk, 0, 220});
            store<u32>(blk, 220);
            call(ctx, 0x00441D68u, {blk, o + 16, load<u32>(o + 4), o + 76});
            call(ctx, 0x0071C3F0u, {o + 16, o + 76, 57});
            call(ctx, 0x006D612Cu, {load<u32>(o), blk});
        }
    }
    store<u8>(o + 192, 0);
}

// p < end (as a signed difference) ? p : begin (the ring wrap of 0x004A69C0).
static u32 Wrap51(u32 p, u32 end, u32 begin) {
    const u32 m = static_cast<u32>(static_cast<s32>(p - end) >> 31);
    return (p & m) | (begin & ~m);
}

// void f(Vec3* out, Ring* r, const Vec3* v): pushes v into the ring of
// 16-byte samples r (+0 begin, +4 end, +8 write, +12 read, +16 capacity,
// +20 count; the oldest dropped when full, a null write slot not stored)
// and writes the samples' mean to out (the sum from the zero vector
// 0x01164B40, divided by the count from the .text pool 0x004A69B4 [0] in
// steps of [1] when above [1]). f1 = out->z.
DECOMP(004A69C0, Ring_PushMean_004A69C0) {
    const u32 out = arg<u32>(ctx, 0), r = arg<u32>(ctx, 1), v = arg<u32>(ctx, 2);
    u32 cnt = load<u32>(r + 20);
    const u32 cap = load<u32>(r + 16), wp = load<u32>(r + 8), begin = load<u32>(r), end = load<u32>(r + 4);
    u32 rp = load<u32>(r + 12);
    if (cnt == cap) {
        rp = Wrap51(rp + 16, end, begin);
        --cnt;
        store<u32>(r + 20, cnt);
        store<u32>(r + 12, rp);
    }
    // count == capacity cannot hold here any more (the original's other path is dead)
    ++cnt;
    const u32 nwp = Wrap51(wp + 16, end, begin);
    store<u32>(r + 20, cnt);
    store<u32>(r + 8, nwp);
    if (wp != 0) {
        Cp50(wp, v);
        Cp50(wp + 4, v + 4);
        Cp50(wp + 8, v + 8);
    }
    constexpr u32 Z = 0x01164B40u, K = 0x004A69B4u;
    f64 x = Lfs51(Z);
    store<f32>(out, static_cast<f32>(x));
    f64 y = Lfs51(Z + 4);
    store<f32>(out + 4, static_cast<f32>(y));
    f64 z = Lfs51(Z + 8);
    store<f32>(out + 8, static_cast<f32>(z));
    f64 n = Lfs51(K);
    const f64 one = Lfs51(K + 4);
    u32 p = cnt != 0 ? rp : 0;
    while (p != 0) {
        x = F51(x + Lfs51(p));
        n = F51(n + one);
        store<f32>(out, static_cast<f32>(x));
        y = F51(y + Lfs51(p + 4));
        store<f32>(out + 4, static_cast<f32>(y));
        z = F51(z + Lfs51(p + 8));
        store<f32>(out + 8, static_cast<f32>(z));
        p = Wrap51(p + 16, end, begin);
        const u32 m = static_cast<u32>((static_cast<s32>(p - nwp) >> 31) | (static_cast<s32>(nwp - p) >> 31));
        p &= m;
    }
    if (n > one) {
        const f64 s = F51(one / n);
        x = F51(x * s);
        y = F51(y * s);
        z = F51(z * s);
        store<f32>(out, static_cast<f32>(x));
        store<f32>(out + 4, static_cast<f32>(y));
        store<f32>(out + 8, static_cast<f32>(z));
    }
    ctx.fpr[1] = z;
}

// The global arrays of 0x004E9DB4 ({u16 esize +0, u16 cap2 +6, data +8,
// allocator +12}): the block (esize * (cap2 >> 1) bytes) returned through
// alloc->vslot 8(alloc, data, size) when set, then Array_Destruct(a).
static void FreeArr51(PPUContext& ctx, u32 a) {
    const u32 sz = static_cast<u32>(load<u16>(a)) * ((static_cast<u32>(load<u16>(a + 6)) >> 1) & 0x7FFFu);
    if (const u32 d = load<u32>(a + 8); d != 0) {
        const u32 al = load<u32>(a + 12);
        VCall(ctx, al, 8, {al, d, sz});
    }
    call(ctx, 0x0054EC94u, {a});
}

// void f(): static teardown: the vtable 0x00B22698 into the global objects
// 0x010C2AC8 / 0x010C2ABC, the array 0x010C2B08 freed (FreeArr51), the
// vtable into 0x010C2AB0 / 0x010C2AA4, the array 0x010C2B20 freed, the
// vtable into the 11 objects 0x010C2A98 .. 0x010C2A20, the array
// 0x010C2AF0 freed. r3 = Array_Destruct's.
DECOMP(004E9DB4, StaticDtor_004E9DB4) {
    constexpr u32 V = 0x00B22698u;
    store<u32>(0x010C2AC8u, V);
    store<u32>(0x010C2ABCu, V);
    FreeArr51(ctx, 0x010C2B08u);
    store<u32>(0x010C2AB0u, V);
    store<u32>(0x010C2AA4u, V);
    FreeArr51(ctx, 0x010C2B20u);
    static constexpr u32 kObjs[11] = {0x010C2A98u, 0x010C2A8Cu, 0x010C2A68u, 0x010C2A80u, 0x010C2A5Cu, 0x010C2A74u,
                                      0x010C2A50u, 0x010C2A44u, 0x010C2A38u, 0x010C2A2Cu, 0x010C2A20u};
    for (u32 a : kObjs) store<u32>(a, V);
    FreeArr51(ctx, 0x010C2AF0u);
}

// u32 f(T* o, u32 key, const char* s): the string list of key in o's
// table +560 (Table_Find(o + 560, &key); when missing a new 20-byte
// PtrVector (Mem_Alloc(20, 0, 16), capacity 4; 0 when that fails) entered
// with ArrInsert8_51) gets a copy of s (Mem_Alloc(len + 1, 0, 16) +
// String_Assign) pushed (PtrVec_Push50). r3 = the list's old high-water
// count.
DECOMP(004ECF08, StrTable_Add_004ECF08) {
    const u32 o = arg<u32>(ctx, 0), s = arg<u32>(ctx, 2);
    Stack fr(ctx, 16);  // the key at sp+112
    const u32 kp = fr.addr();
    store<u32>(kp, arg<u32>(ctx, 1));
    u32 l = load<u32>(static_cast<u32>(call(ctx, 0x00799474u, {o + 560, kp})));
    if (l == 0) {
        const u32 p = static_cast<u32>(call(ctx, 0x0054ED38u, {20, 0, 16}));
        if (p != 0) {
            call(ctx, 0x005516E0u, {p});
            l = p;
            store<u16>(p + 6, 4);
        }
        ArrInsert8_51(ctx, o + 560, kp, l);
    }
    const u32 len = static_cast<u32>(call(ctx, 0x00555FA4u, {s}));
    const u32 buf = static_cast<u32>(call(ctx, 0x0054ED38u, {len + 1, 0, 16}));
    call(ctx, 0x00555F80u, {buf, s});
    const u32 len2 = static_cast<u32>(call(ctx, 0x00555FA4u, {s}));
    store<u8>(buf + len2, 0);
    ret(ctx, PtrVec_Push50(ctx, l, buf));
}

// u32 f(T* o, const char* name, Def* d): vtable 0x00B23050, +16 = d, +4 =
// +8 = +12 = +20 = +24 = +28 = 0, u8 +32 = u8 +33 = 0; zeroed blocks of 4 *
// d->8->48 (+12), 4 * d->28 (+20) and 4 * d->8->48 (+24) words (SmallAlloc
// + memset, each only when non-empty); with a name, +4 = StrBuf_Append(
// *(0x00F668D8)->4, name) and +8 = String_HashNoCase(name); then the
// d->28 words of +20 and the u16 d->8->16 words of +24 are cleared again.
// r3 = 4 * the last count.
DECOMP(004F75B4, Inst_Init_004F75B4) {
    const u32 o = arg<u32>(ctx, 0), name = arg<u32>(ctx, 1), d = arg<u32>(ctx, 2);
    store<u32>(o, 0x00B23050u);
    store<u32>(o + 4, 0);
    store<u32>(o + 12, 0);
    store<u32>(o + 16, d);
    store<u32>(o + 20, 0);
    store<u32>(o + 24, 0);
    store<u32>(o + 8, 0);
    const u32 d8 = load<u32>(d + 8);
    store<u32>(o + 28, 0);
    store<u8>(o + 32, 0);
    store<u8>(o + 33, 0);
    u32 cur = d;
    if (const u32 n = load<u32>(d8 + 48) << 2; n != 0) {
        const u32 p = static_cast<u32>(call(ctx, 0x0055238Cu, {n}));
        store<u32>(o + 12, p);
        call(ctx, 0x00726E54u, {p, 0, n});
        cur = load<u32>(o + 16);
    }
    if (const u32 n = load<u32>(cur + 28) << 2; n != 0) {
        const u32 p = static_cast<u32>(call(ctx, 0x0055238Cu, {n}));
        store<u32>(o + 20, p);
        call(ctx, 0x00726E54u, {p, 0, n});
        cur = load<u32>(o + 16);
    }
    if (const u32 n = load<u32>(load<u32>(cur + 8) + 48) << 2; n != 0) {
        const u32 p = static_cast<u32>(call(ctx, 0x0055238Cu, {n}));
        store<u32>(o + 24, p);
        call(ctx, 0x00726E54u, {p, 0, n});
    }
    if (name != 0) {
        store<u32>(o + 4, static_cast<u32>(call(ctx, 0x00566F1Cu, {load<u32>(load<u32>(0x00F668D8u) + 4), name})));
        store<u32>(o + 8, static_cast<u32>(call(ctx, 0x0056A710u, {name})));
    }
    u32 off = 0;
    for (u32 i = 0; i < load<u32>(load<u32>(o + 16) + 28); ++i, off += 4) store<u32>(load<u32>(o + 20) + off, 0);
    off = 0;
    u32 i = 0;
    for (; i < load<u16>(load<u32>(d + 8) + 16); ++i, off += 4) store<u32>(load<u32>(o + 24) + off, 0);
    ctx.gpr[4] = i;
    ret(ctx, off);
}

// ---------------------------------------------------------------- sweep: 114 instructions
// void f(Tuning* t): u8 t->120 = 0, t->124 = 0, then 28 tuning values (the
// f32 +72 of the variables in the 0x0103xxxx table, two of them other
// fields) into t (+0..+116, +12 / +28 untouched). f1 = the last value.
DECOMP(000FD974, Tuning_Copy_000FD974) {
    const u32 t = arg<u32>(ctx, 0);
    store<u8>(t + 120, 0);
    store<u32>(t + 124, 0);
    static constexpr u32 kSrc[][2] = {
        {0, 0x01032418u},   {4, 0x010324B0u},   {8, 0x010324FCu},   {16, 0x01032550u},  {20, 0x01032554u},
        {24, 0x01032558u},  {32, 0x010325A8u},  {36, 0x010325F4u},  {40, 0x01032640u},  {44, 0x0103268Cu},
        {48, 0x010326D8u},  {52, 0x010327BCu},  {56, 0x01032808u},  {60, 0x01032724u},  {64, 0x01032770u},
        {68, 0x01032854u},  {72, 0x010328A0u},  {76, 0x010328ECu},  {80, 0x01032938u},  {84, 0x01032984u},
        {88, 0x010329D0u},  {92, 0x01032A1Cu},  {96, 0x01035410u},  {100, 0x01032FE0u}, {104, 0x0103302Cu},
        {108, 0x01033078u}, {112, 0x010330C4u}, {116, 0x01033110u},
    };
    for (const auto& e : kSrc) Cp50(t + e[0], e[1]);
    ctx.fpr[1] = Lfs51(0x01033110u);
}

// Index of the 16-byte property with key k in the list of skeleton s
// (+124 entries, s32 count +128), or -1.
static s32 SkelProp51(u32 s, u32 k) {
    for (s32 i = 0; i < static_cast<s32>(load<u32>(s + 128)); ++i)
        if (load<u32>(load<u32>(s + 124) + 16 * static_cast<u32>(i)) == k) return i;
    return -1;
}

// void f(Obj* o, u8 full): for a full shutdown: u16 o->390 = o->366 = 0;
// o->vslot 288(o, first) for the first child (o->344) while u16 o->352 is
// set (the OPD after the entry in the registers); the body o->416->8 is
// removed (Body_Remove931F00) unless bit 0x04000000 of o->412 is set or
// there is none; the properties 0xA734004D / 0x91C7CF5B are removed from
// the skeleton o->416 (Hk_RemoveProperty93FED8(&tmp, skel, key)) when it
// has them; with o->420 the listener (+8) is removed from the skeleton
// and o->420 released; the skeleton released (AnimSkel_Release7D4EA8).
// Then r3 = Obj_ShutdownBase_000D74C0(o, full).
DECOMP(0020D5D4, Obj_Shutdown_0020D5D4) {
    const u32 o = arg<u32>(ctx, 0), full = arg<u32>(ctx, 1) & 0xFFu;
    if (full != 0) {
        store<u16>(o + 390, 0);
        store<u16>(o + 366, 0);
        while (load<u16>(o + 352) != 0) {
            const u32 opd = load<u32>(load<u32>(o) + 288);
            const u32 first = load<u32>(load<u32>(o + 344));
            call_ptr(ctx, opd, {o, first, load<u32>(opd), opd});
        }
        u32 sk = load<u32>(o + 416);
        if ((load<u32>(o + 412) & 0x04000000u) != 0 && load<u32>(sk + 8) != 0) {
            // r4 = the skeleton (read by the callee)
            call(ctx, 0x00931F00u, {load<u32>(sk + 8), sk});
            sk = load<u32>(o + 416);
        }
        Stack fr(ctx, 16);  // the results at sp+112 / sp+120
        if (SkelProp51(sk, 0xA734004Du) >= 0) {
            call(ctx, 0x0093FED8u, {fr.addr(), sk, 0xA734004Du});
            sk = load<u32>(o + 416);
        }
        if (SkelProp51(sk, 0x91C7CF5Bu) >= 0) {
            call(ctx, 0x0093FED8u, {fr.addr() + 8, sk, 0x91C7CF5Bu});
            sk = load<u32>(o + 416);
        }
        if (const u32 l = load<u32>(o + 420); l != 0) {
            call(ctx, 0x00928828u, {sk, l + 8});
            call(ctx, 0x007D4EA8u, {load<u32>(o + 420)});
            sk = load<u32>(o + 416);
        }
        call(ctx, 0x007D4EA8u, {sk});
    }
    call(ctx, 0x000D74C0u, {o, full});
}

// void f(T* o, Def* d): QuerySpawn_BaseCtor_0020C868(o, d, 32, 1), vtable
// 0x00B1A2D8; the rotation from the Euler angles d->164 / d->168 / d->160
// (each times the .text pool 0x0021A510; Math_Cosf / Math_Sinf) as the
// quaternion o->432..444; o->448 = 0, +452 = d->176, +456..+464 =
// d->180..188, and bits 31..27 of o->468 from the bytes d->192..196
// (stored after each). r3 = u8 d->196, f1 = d->188.
DECOMP(0021A518, Spawn_InitRot_0021A518) {
    const u32 o = arg<u32>(ctx, 0), d = arg<u32>(ctx, 1);
    call(ctx, 0x0020C868u, {o, d, 32, 1});
    store<u32>(o, 0x00B1A2D8u);
    const f64 k = Lfs51(0x0021A510u);
    const f64 a = F51(Lfs51(d + 164) * k);
    const f64 c0 = Lfs51(d + 160);
    const f64 b0 = Lfs51(d + 168);
    const f64 ca = callf(ctx, 0x007344E0u, {}, {a});
    const f64 b = F51(b0 * k);
    const f64 cb = callf(ctx, 0x007344E0u, {}, {b});
    const f64 c = F51(c0 * k);
    const f64 cc = callf(ctx, 0x007344E0u, {}, {c});
    const f64 sa = callf(ctx, 0x007345F0u, {}, {a});
    const f64 sb = callf(ctx, 0x007345F0u, {}, {b});
    const f64 sc = callf(ctx, 0x007345F0u, {}, {c});
    const f64 p2 = F51(sa * sb), p3 = F51(ca * cb), p4 = F51(cb * sa), p5 = F51(ca * sb);
    u32 w = load<u32>(o + 468);
    store<u32>(o + 448, 0);
    store<u32>(o + 452, load<u32>(d + 176));
    w = (w & 0x7FFFFFFFu) | ((static_cast<u32>(load<u8>(d + 192)) & 1u) << 31);
    const f64 q6 = F51(p2 * sc), q7 = F51(p3 * sc), q8 = F51(p4 * sc), q1 = F51(p5 * sc);
    store<f32>(o + 444, static_cast<f32>(F51(std::fma(p3, cc, -q6))));
    store<f32>(o + 432, static_cast<f32>(F51(std::fma(p2, cc, q7))));
    store<f32>(o + 440, static_cast<f32>(F51(std::fma(p5, cc, -q8))));
    store<f32>(o + 436, static_cast<f32>(F51(std::fma(p4, cc, q1))));
    Cp50(o + 456, d + 180);
    Cp50(o + 460, d + 184);
    store<u32>(o + 468, w);
    Cp50(o + 464, d + 188);
    ctx.fpr[1] = Lfs51(d + 188);
    u32 bt = 0;
    for (u32 i = 1; i <= 4; ++i) {
        bt = load<u8>(d + 192 + i);
        w = (w & ~(0x80000000u >> i)) | ((bt & 1u) << (31 - i));
        store<u32>(o + 468, w);
    }
    ret(ctx, bt);
}

// void f(Area* a, Obj* b): Area_Leave(a, b); b is taken out of a's 8-byte
// entry list (+432, u16 count +440; the last entry's 5 bytes moved into
// its slot) when bit 0 of b->188 is set or b (with bit 0x8 of +192) lacks
// bit 0x10. Then with bit 0 of b->188: r3 = Obj_EndScenes_002257B4(a, b);
// otherwise, when bit 0x8 of u8 b->216->32 is set, b->vslot 96(b, 0)
// unless b has bit 0x2 of +192 and a time +348 not above the .text pool
// 0x00225C84 (r3 = b then; 0 in the other cases).
DECOMP(00225C88, Area_Remove_00225C88) {
    const u32 a = arg<u32>(ctx, 0), b = arg<u32>(ctx, 1);
    call(ctx, 0x0020EBC8u, {a, b});
    const bool on = (load<u32>(b + 188) & 1u) != 0;
    bool rm = on;
    if (b != 0 && (load<u32>(b + 192) & 0x8u) != 0 && (load<u32>(b + 192) & 0x10u) == 0) rm = true;
    ctx.gpr[3] = 0;
    if (rm) {
        for (u32 i = 0;; ++i) {
            const u32 n = load<u16>(a + 440);
            if (!(i < n)) break;
            const u32 d = load<u32>(a + 432);
            if (load<u32>(d + 8 * i) != b) continue;
            const u32 m = n - 1;
            store<u16>(a + 440, static_cast<u16>(m));
            if (i != (m & 0xFFFFu)) {
                const u32 src = d + ((m << 3) & 0x7FFF8u), dst = d + 8 * i;
                for (u32 k = 0; k < 4; ++k) store<u8>(dst + k, load<u8>(src + k));
                store<u8>(dst + 4, load<u8>(src + 4));
            }
            break;
        }
    }
    if ((load<u32>(b + 188) & 1u) != 0) {
        call(ctx, 0x002257B4u, {a, b});
        return;
    }
    if ((load<u8>(load<u32>(b + 216) + 32) & 0x8u) == 0) return;
    if ((load<u32>(b + 192) & 0x2u) != 0) {
        ctx.gpr[3] = b;
        ctx.fpr[1] = Lfs51(b + 348);
        if (!(ctx.fpr[1] > Lfs51(0x00225C84u))) return;
    }
    VCall(ctx, b, 96, {b, 0});
}

// void f(Obj* o): stops the three emitters o->864..872 (Emitter_Stop(e, 1))
// and clears them and +876..+892 (u8 +892); the record o->612->156 (when
// set) gets +8 = +12 = 0, u8 +16 = u8 +17 = 0; bit 0x40000000 of
// o->948->1268 cleared; +932 = +936 = +964 = +880 = 0; o->864 = the handle
// from Sound_Play44C474(&h, o->896, 0 x9); 0x002300D0(o), then o->vslot
// 136(o, 0x01164B40) and o->vslot 384(o, 0x01164B40) (r3 = its).
DECOMP(002307C8, Obj_ResetSounds_002307C8) {
    const u32 o = arg<u32>(ctx, 0);
    for (u32 k = 0; k < 3; ++k) call(ctx, 0x00443424u, {load<u32>(o + 864 + 4 * k), 1});
    store<u32>(o + 864, 0);
    store<u32>(o + 868, 0);
    store<u32>(o + 872, 0);
    store<u32>(o + 876, 0);
    const u32 r = load<u32>(o + 612);
    store<u32>(o + 880, 0);
    store<u32>(o + 884, 0);
    store<u32>(o + 888, 0);
    store<u8>(o + 892, 0);
    const u32 g = load<u32>(o + 948);
    if (r != 0) {
        if (const u32 q = load<u32>(r + 156); q != 0) {
            store<u32>(q + 8, 0);
            store<u32>(q + 12, 0);
            store<u8>(q + 16, 0);
            store<u8>(q + 17, 0);
        }
    }
    if (g != 0) store<u32>(g + 1268, load<u32>(g + 1268) & ~0x40000000u);
    store<u32>(o + 932, 0);
    store<u32>(o + 936, 0);
    store<u32>(o + 964, 0);
    const u32 snd = load<u32>(o + 896);
    store<u32>(o + 880, 0);
    Stack fr(ctx, 32);  // the handle at sp+144
    call(ctx, 0x0044C474u, {fr.addr(), snd, 0, 0, 0, 0, 0, 0, 0, 0, 0});
    store<u32>(o + 864, load<u32>(fr.addr()));
    call(ctx, 0x002300D0u, {o});
    VCall(ctx, o, 136, {o, 0x01164B40u});
    VCall(ctx, o, 384, {o, 0x01164B40u});
}

// void f(Obj* o): o->2560 = 3; without o->2564: o->960 = 0 and the model
// o->948 (when set) gets its tint: m->vslot 188(m, 0x01059854 / 0x0105991C,
// 0) for bit 0x100 / 0x80 of o->244, then the colour from m->vslot 52 (a
// 4-byte struct returned through &c) passed back as m->vslot 56(m, rgb0)
// (alpha byte 0). Then the effects +196 / +200 / +204 are bound:
// Effect_Bind4A6558(o + 196, 0x00AD4B7C, 0x00AD4B7C, o->48, o->52, 1),
// Effect_Bind4A63FC(o + 200, o->164, 0, o->48, o->52, 1), and +204 as +196
// (r3 = its).
DECOMP(002A4FF0, Obj_BindEffects_002A4FF0) {
    const u32 o = arg<u32>(ctx, 0);
    store<u32>(o + 2560, 3);
    if (load<u32>(o + 2564) == 0) {
        store<u32>(o + 960, 0);
        if (const u32 m = load<u32>(o + 948); m != 0) {
            const u32 fl = load<u32>(o + 244);
            if ((fl & 0x100u) != 0)
                VCall(ctx, m, 188, {m, 0x01059854u, 0});
            else if ((fl & 0x80u) != 0)
                VCall(ctx, m, 188, {m, 0x0105991Cu, 0});
            Stack fr(ctx, 16);  // the colour at sp+112
            const u32 c = fr.addr();
            const u32 m1 = load<u32>(o + 948);
            const u32 opd = load<u32>(load<u32>(m1) + 52);
            call_ptr(ctx, opd, {c, m1, opd, load<u32>(opd)});
            const u32 m2 = load<u32>(o + 948);
            const u32 c0 = load<u8>(c);
            store<u8>(c + 3, 0);
            const u32 opd2 = load<u32>(load<u32>(m2) + 56);
            const u32 rgb = (c0 << 24) | (static_cast<u32>(load<u8>(c + 1)) << 16) | (static_cast<u32>(load<u8>(c + 2)) << 8);
            // r4 = the colour word, r5 = the entry (the OPD register is reused)
            call_ptr(ctx, opd2, {m2, rgb, load<u32>(opd2)});
        }
    }
    constexpr u32 E = 0x00AD4B7Cu;
    call(ctx, 0x004A6558u, {o + 196, E, E, load<u32>(o + 48), load<u32>(o + 52), 1});
    call(ctx, 0x004A63FCu, {o + 200, load<u32>(o + 164), 0, load<u32>(o + 48), load<u32>(o + 52), 1});
    call(ctx, 0x004A6558u, {o + 204, E, E, load<u32>(o + 48), load<u32>(o + 52), 1});
}

// Effect_SetHash4CF988(e, h) unless e's current def (e->4->8) already has h.
static void SetHashIfNew51(PPUContext& ctx, u32 e, u32 h) {
    const u32 d = load<u32>(e + 4);
    const u32 cur = d != 0 ? load<u32>(d + 8) : 0;
    if (cur != h) call(ctx, 0x004CF988u, {e, h});
}

// u32 f(T* o, u8 on): o->64 = 0; the effects o->96 / o->100 set to
// 0x4A5CA6A4 (Effect_SetHash4CF988); the owner's effect o->4->940 (when
// set) switched to 0xD6E3A3F3 (on) / 0x0590D9BB (off) unless it has it;
// on: u8 o->104 = 0, o->76 = 0, o->80..88 = the vector 0x01164B60; then
// o->0->vslot 40(o->0, on, 1) (when set) and, off, the effects +96 / +100
// set again; u8 o->105 = 0. r3 = 0.
DECOMP(00308F5C, Obj_SetGlow_00308F5C) {
    const u32 o = arg<u32>(ctx, 0), on = arg<u32>(ctx, 1) & 0xFFu;
    store<u32>(o + 64, 0);
    auto both = [&] {
        if (const u32 e = load<u32>(o + 96); e != 0) call(ctx, 0x004CF988u, {e, 0x4A5CA6A4u});
        if (const u32 e = load<u32>(o + 100); e != 0) call(ctx, 0x004CF988u, {e, 0x4A5CA6A4u});
    };
    both();
    if (const u32 e = load<u32>(load<u32>(o + 4) + 940); e != 0) SetHashIfNew51(ctx, e, on != 0 ? 0xD6E3A3F3u : 0x0590D9BBu);
    if (on != 0) {
        constexpr u32 V = 0x01164B60u;
        store<u8>(o + 104, 0);
        Cp50(o + 80, V);
        Cp50(o + 84, V + 4);
        store<u32>(o + 76, 0);
        Cp50(o + 88, V + 8);
        ctx.fpr[1] = Lfs51(V + 8);
    }
    if (const u32 p = load<u32>(o); p != 0) {
        VCall(ctx, p, 40, {p, on, 1});
        if (on == 0) both();
    }
    store<u8>(o + 105, 0);
    ret(ctx, 0u);
}

// The skeleton holder teardown of 0x00346F84 (holder h on the stack:
// vtable 0x00B1F480 and AnimSkel_Release7D4EA8(h->8), h->8 = 0, when set;
// then vtable 0x00B13974).
static void HolderDtor51(PPUContext& ctx, u32 h) {
    if (const u32 s = load<u32>(h + 8); s != 0) {
        store<u32>(h, 0x00B1F480u);
        call(ctx, 0x007D4EA8u, {s});
        store<u32>(h + 8, 0);
    }
    store<u32>(h, 0x00B13974u);
}

// bool f(Iff* iff): with the skeleton set *(0x00F64C98): the chunk is
// loaded (Iff_ChunkSize; SkelHolder_Init_0034640C(&h, iff, 4096),
// Stream_Init7CE648(&s), Hk_Load802E08(&h, &out, {0, 0, 0}) on the stack);
// a loaded resource goes to 0x00343CF4(set, *(0x00F64CA4), res, size, 0)
// and is released; when that worked (and the set is still there) the last
// entry of set->20 (count set->24) goes to *(0x00F64CAC). Always
// *(0x00F64CA4) = 0 and r3 = 1.
DECOMP(00346F84, SkelSet_LoadChunk_00346F84) {
    const u32 iff = arg<u32>(ctx, 0);
    const u32 set = load<u32>(0x00F64C98u);
    if (set != 0) {
        const u32 cur = load<u32>(0x00F64CA4u);
        const u32 size = static_cast<u32>(call(ctx, 0x005637D4u, {iff}));
        Stack fr(ctx, 32);  // out sp+112, stream +116, holder +120..135
        const u32 out = fr.addr(), strm = out + 4, h = out + 8;
        call(ctx, 0x0034640Cu, {h, iff, 4096});
        store<u32>(out, 0);
        call(ctx, 0x007CE648u, {strm});
        // the {0, 0, 0} struct goes in r5 / r6 (the low word of r6 is
        // unwritten stack in the original)
        const u32 res = static_cast<u32>(call(ctx, 0x00802E08u, {h, out, 0, 0}));
        call(ctx, 0x007CEA58u, {strm});
        HolderDtor51(ctx, h);
        if (res != 0) {
            const u32 ok = static_cast<u32>(call(ctx, 0x00343CF4u, {set, cur, res, size, 0}));
            call(ctx, 0x007D4EA8u, {res});
            if (ok != 0) {
                if (const u32 s2 = load<u32>(0x00F64C98u); s2 != 0)
                    store<u32>(0x00F64CACu, load<u32>(load<u32>(s2 + 20) + (load<u32>(s2 + 24) << 2) - 4));
            }
        }
    }
    store<u32>(0x00F64CA4u, 0);
    ret(ctx, 1u);
}

// u32 f(Pool* pool, List* l, Obj* o, a): for a list: h = o->204 (else
// String_HashNoCase(o->vslot 12(o))); a slot index from the pool's free
// counter (pool->20: u16 limit +0, u16 next +2; pool->24[idx] = 0 when
// that table is set; -1 when exhausted); the 336-byte rig pool->28 + 336
// idx (Rig_Reset39BFC4 when non-null) gets +272 = a, +280 = o, +284 = l;
// o (when set) gets +280 = rig, +276 = 0x044DC6AB; {h, s16 idx} is
// appended to l's 8-byte array +76 (u16 capacity +76, count +78,
// high-water +80, flags +82 |= 1, data +84). r3 = the new flags (pool
// without a list).
DECOMP(003A1BFC, RigPool_Attach_003A1BFC) {
    const u32 pool = arg<u32>(ctx, 0), l = arg<u32>(ctx, 1), o = arg<u32>(ctx, 2), a = arg<u32>(ctx, 3);
    if (l == 0) return;
    u32 h = load<u32>(o + 204);
    if (h == 0) h = static_cast<u32>(call(ctx, 0x0056A710u, {static_cast<u32>(VCall(ctx, o, 12, {o}))}));
    u32 idx = 0xFFFFFFFFu;
    if (const u32 fc = load<u32>(pool + 20); static_cast<s32>(load<u16>(fc + 2)) < static_cast<s32>(load<u16>(fc))) {
        const u32 nx = load<u16>(fc + 2);
        if (const u32 t = load<u32>(pool + 24); t != 0) store<u32>(t + ((nx << 2) & 0x3FFFCu), 0);
        const u32 fc2 = load<u32>(pool + 20);
        store<u16>(fc2 + 2, static_cast<u16>(load<u16>(fc2 + 2) + 1));
        idx = nx;
    }
    const s32 si = static_cast<s16>(static_cast<u16>(idx));
    const u32 rig = load<u32>(pool + 28) + static_cast<u32>(si) * 336u;
    if (rig != 0) call(ctx, 0x0039BFC4u, {rig});
    store<u32>(rig + 272, a);
    store<u32>(rig + 280, o);
    store<u32>(rig + 284, l);
    if (o != 0) {
        store<u32>(o + 280, rig);
        store<u32>(o + 276, 0x044DC6ABu);
    }
    const u32 A = l + 76;
    const u32 cap = load<u16>(A);
    if (load<u16>(A + 2) == cap) call(ctx, 0x00558F3Cu, {A, (cap << 1) & 0xFFFEu});
    const u32 d = load<u32>(A + 8);
    store<u32>(d + ((static_cast<u32>(load<u16>(A + 2)) << 3) & 0x7FFF8u), h);
    store<u16>(d + (static_cast<u32>(load<u16>(A + 2)) << 3) + 4, static_cast<u16>(si));
    const u32 n = load<u16>(A + 2) + 1u;
    const u32 hi = load<u16>(A + 4);
    const u32 fl = load<u16>(A + 6) | 1u;
    store<u16>(A + 2, static_cast<u16>(n));
    store<u16>(A + 4, static_cast<u16>(static_cast<s32>(hi) > static_cast<s32>(n & 0xFFFFu) ? hi : n));
    store<u16>(A + 6, static_cast<u16>(fl));
    ret(ctx, fl);
}

// u32 f(T* o): for each 16-byte entry {model, .., anim +8, anim2 +12} of
// o->80 (u16 count o->88) with a model and an animation (+12 first, with
// flag 1 and f1 = the .text pool 0x004356F8 [2]; else +8, flag 0, f1 =
// [1]): Anim_Restart2775A8(model->vslot 60(model), anim, 0, flag, -1, ..,
// 0, 0, .., .., 0, -1; f2 = [0], f3 = f4 = [1]) when the model has a
// track, else model->vslot 200(model, anim, 0, flag, .., .., 0, .., 0; f1,
// f2 = [0], f3 = [1]); when that worked, bit 0x2000 of model->172 is
// cleared. r3 = the entry count.
DECOMP(00435708, Models_RestartAnims_00435708) {
    const u32 o = arg<u32>(ctx, 0);
    constexpr u32 K = 0x004356F8u;
    const f64 k0 = Lfs51(K), k1 = Lfs51(K + 4);
    u32 n;
    for (u32 i = 0;; ++i) {
        n = load<u16>(o + 88);
        if (!(i < n)) break;
        const u32 e = load<u32>(o + 80) + 16 * i;
        const u32 m = load<u32>(e);
        if (m == 0) continue;
        u32 an = load<u32>(e + 12), fl = 1;
        f64 f = Lfs51(K + 8);
        if (an == 0) {
            an = load<u32>(e + 8);
            fl = 0;
            f = k1;
            if (an == 0) continue;
        }
        const u32 tr = static_cast<u32>(VCall(ctx, m, 60, {m}));
        u32 r;
        if (tr != 0) {
            // r8 as the vslot 60 call left it (the callee does not read it)
            callf(ctx, 0x002775A8u, {tr, an, 0, fl, ~0ull, ctx.gpr[8], 0, 0, 0, 0, 0, ~0ull}, {f, k0, k1, k1});
            r = static_cast<u32>(ctx.gpr[3]);
        } else {
            const u32 m2 = load<u32>(load<u32>(o + 80) + 16 * i);
            const u32 opd = load<u32>(load<u32>(m2) + 200);
            ctx.fpr[1] = f;
            ctx.fpr[2] = k0;
            ctx.fpr[3] = k1;
            // r7 / r8 = the OPD / entry, r10 as the vslot 60 call left it
            r = static_cast<u32>(call_ptr(ctx, opd, {m2, an, 0, fl, opd, load<u32>(opd), 0, ctx.gpr[10], 0}));
        }
        if (r == 0) continue;
        const u32 m3 = load<u32>(load<u32>(o + 80) + 16 * i);
        store<u32>(m3 + 172, load<u32>(m3 + 172) & ~0x2000u);
    }
    ret(ctx, n);
}

// u32 f(Op* op, Obj* obj): applies op (u8 +20: 0 set, 1 add, 2 sub, 3 mul,
// 4 div (skipped for 0)) with the value op->16 to the int property op->12
// of obj (Effect_FindComponent; kind vslot 32 == 2: the value vslot 24,
// result in +20); a kind 12 property only takes a plain set; a missing
// property, another kind or another op on kind 12 only Hash_ToString(
// op->12). r3 as the original leaves it.
DECOMP(004CAEB8, Prop_ApplyOp_004CAEB8) {
    const u32 op = arg<u32>(ctx, 0), obj = arg<u32>(ctx, 1);
    const u32 c = static_cast<u32>(call(ctx, 0x004BB4B4u, {obj, load<u32>(op + 12)}));
    auto miss = [&] { call(ctx, 0x0056A6E0u, {load<u32>(op + 12)}); };
    if (c == 0) {
        miss();
        return;
    }
    if (static_cast<u32>(VCall(ctx, c, 32, {c})) == 2) {
        const u32 v = static_cast<u32>(VCall(ctx, c, 24, {c}));
        const u32 k = load<u8>(op + 20);
        u32 r = v;
        switch (k) {
        case 4:
            if (const u32 d = load<u32>(op + 16); d != 0) {
                r = static_cast<u32>(static_cast<s32>(static_cast<s64>(static_cast<s32>(v)) / static_cast<s32>(d)));
                store<u32>(c + 20, r);
            }
            break;
        case 3:
            r = v * load<u32>(op + 16);
            store<u32>(c + 20, r);
            break;
        case 2:
            r = v - load<u32>(op + 16);
            store<u32>(c + 20, r);
            break;
        case 1:
            r = load<u32>(op + 16) + v;
            store<u32>(c + 20, r);
            break;
        case 0:
            r = load<u32>(op + 16);
            store<u32>(c + 20, r);
            break;
        default:
            break;
        }
        ret(ctx, r);
        return;
    }
    if (static_cast<u32>(VCall(ctx, c, 32, {c})) != 12) {
        miss();
        return;
    }
    const u32 k = load<u8>(op + 20);
    if (k >= 1 && k <= 4) {
        miss();
        return;
    }
    if (k == 0) {
        const u32 r = load<u32>(op + 16);
        store<u32>(c + 20, r);
        ret(ctx, r);
        return;
    }
    ret(ctx, k);
}

// ---------------------------------------------------------------- sweep: 115 instructions
// bool f(Npc* o): r3 = 1 at once when o->256 is set without bit 0 of its
// +188. Otherwise a 32-byte path request on the stack ({0, 0xFFFF, -1,
// o->vslot 244(o), u8 0, 0, 0, f32 o->vslot 240(o)}); without a valid nav
// query at o + 216 one is made from the position o->176
// (NavQuery_Init630F94(&q, &pos, 0, &req), NavQuery_Assign62D414,
// NavQuery_Free62D08C). r3 = 1 if the query is still invalid; else, with
// the nav world *(0x00F557EC) enabled (u8 +20) and both queries (o + 216
// and world + 12) valid, r3 = 0x006320AC(o + 216, world + 12, &req) (0
// otherwise).
DECOMP(00015AAC, Npc_PathCheck_00015AAC) {
    const u32 o = arg<u32>(ctx, 0);
    if (const u32 t = load<u32>(o + 256); t != 0 && (load<u32>(t + 188) & 1u) == 0) {
        ret(ctx, 1u);
        return;
    }
    Stack fr(ctx, 64);  // query sp+112, position +120, request +132..163
    const u32 q = fr.addr(), pos = q + 8, req = q + 20;
    store<u32>(req, 0);
    store<u32>(req + 4, 0xFFFFu);
    store<u32>(req + 8, 0xFFFFFFFFu);
    store<u32>(req + 12, 0xFFFFFFFFu);
    store<u8>(req + 16, 0);
    store<u32>(req + 20, 0);
    store<u32>(req + 24, 0);
    store<u32>(req + 28, 0);
    store<u32>(req + 12, static_cast<u32>(VCall(ctx, o, 244, {o})));
    VCall(ctx, o, 240, {o});
    store<f32>(req + 28, static_cast<f32>(ctx.fpr[1]));
    const u32 nq = o + 216;
    if (static_cast<u32>(call(ctx, 0x0062E164u, {nq})) == 0) {
        Cp50(pos, o + 176);
        Cp50(pos + 4, o + 180);
        Cp50(pos + 8, o + 184);
        ctx.fpr[1] = Lfs51(o + 184);
        call(ctx, 0x00630F94u, {q, pos, 0, req});
        call(ctx, 0x0062D414u, {nq, q});
        call(ctx, 0x0062D08Cu, {q});
    }
    if (static_cast<u32>(call(ctx, 0x0062E164u, {nq})) == 0) {
        ret(ctx, 1u);
        return;
    }
    const u32 w = load<u32>(0x00F557ECu);
    if (load<u8>(w + 20) == 0 || static_cast<u32>(call(ctx, 0x0062E164u, {nq})) == 0 ||
        static_cast<u32>(call(ctx, 0x0062E164u, {w + 12})) == 0) {
        ret(ctx, 0u);
        return;
    }
    call(ctx, 0x006320ACu, {nq, w + 12, req});
}

// void f(Obj* o): once (bit 0x10 of u8 o->466, then set; r3 = 1 when
// already done): o->48->vslot 128(.., 0x03DE9A54, 0); a 24-byte event on
// the stack (Event1_Init_00464ED4(&ev, o->60, 0 x5, r10 as left))
// queried with World_Query482D14(*(0x00F65CA0), -1, &ev);
// Obj_Detach_0002EC04(o); Scene_RemoveObj12544(*(0x00F557F0), o); the
// group CallK2_000D935C(o->60) (when found) gets 0x00268CC8(g, o->4 == 44)
// and g->vslot 48(g, 0, 1); Obj_SetPhysicsActive(o->68, 0) when set;
// bit 0x2 of u8 o->464 cleared; CallK2_000D956C(o->60)->vslot 48(.., 0, 1)
// when found. r3 as the last call leaves it.
DECOMP(00030AD0, Obj_Despawn_00030AD0) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 b = load<u8>(o + 466);
    if ((b & 0x10u) != 0) {
        ret(ctx, 1u);
        return;
    }
    store<u8>(o + 466, static_cast<u8>(b | 0x10u));
    const u32 p = load<u32>(o + 48);
    VCall(ctx, p, 128, {p, 0x03DE9A54u, 0});
    Stack fr(ctx, 32);  // the event at sp+112..135
    const u32 ev = fr.addr();
    store<u32>(ev, 0);
    store<u32>(ev + 20, 0);
    store<u32>(ev + 4, 0);
    store<u32>(ev + 8, 0);
    store<u32>(ev + 12, 0);
    store<u32>(ev + 16, 0);
    // r10 as the vslot 128 call left it
    call(ctx, 0x00464ED4u, {ev, load<u32>(o + 60), 0, 0, 0, 0, 0, ctx.gpr[10]});
    call(ctx, 0x00482D14u, {load<u32>(0x00F65CA0u), ~0ull, ev});
    call(ctx, 0x0002EC04u, {o});
    call(ctx, 0x00012544u, {load<u32>(0x00F557F0u), o});
    if (const u32 g = static_cast<u32>(call(ctx, 0x000D935Cu, {load<u32>(o + 60)})); g != 0) {
        call(ctx, 0x00268CC8u, {g, load<u32>(o + 4) == 44 ? 1u : 0u});
        VCall(ctx, g, 48, {g, 0, 1});
    }
    if (const u32 ph = load<u32>(o + 68); ph != 0) call(ctx, 0x00260C14u, {ph, 0});
    const u32 b2 = load<u8>(o + 464);
    const u32 k = load<u32>(o + 60);
    store<u8>(o + 464, static_cast<u8>(b2 & ~0x2u));
    if (const u32 g2 = static_cast<u32>(call(ctx, 0x000D956Cu, {k})); g2 != 0) VCall(ctx, g2, 48, {g2, 0, 1});
}

// void f(Op* op, Obj* obj, Ctx* c): the kind-5 component op->12 of obj
// (Effect_FindComponent, vslot 32) and its target t (+28; with a name
// op->16 the object of that hash found by WorldObj_ForEach(t, 0x00F2FAF8,
// &{hash, found}, 0), else Hash_ToString(op->16) and t->vslot 12(t) and a
// null target): when t's type (bits 10..15 of t->4) is 8, t->vslot 52(t,
// (u8) (int) (v * the .text pool 0x000778B4 [1]), u8 op->32) with v =
// op->24, or, when op->28 >= pool [0] and c->32 < op->28, op->20 + (op->24
// - op->20) * c->32 / op->28 (r3 = the type otherwise). f1 as the
// fctiwz leaves it.
DECOMP(000778C0, Comp_SetLevel_000778C0) {
    const u32 op = arg<u32>(ctx, 0), obj = arg<u32>(ctx, 1), c = arg<u32>(ctx, 2);
    const u32 key = load<u32>(op + 12);
    if (key == 0) {
        ret(ctx, 0u);
        return;
    }
    const u32 cp = static_cast<u32>(call(ctx, 0x004BB4B4u, {obj, key}));
    if (cp == 0) return;
    if (static_cast<u32>(VCall(ctx, cp, 32, {cp})) != 5) return;
    u32 t = load<u32>(cp + 28);
    if (t == 0) return;
    if (const u32 nm = load<u32>(op + 16); nm != 0) {
        Stack fr(ctx, 16);  // {hash, found} at sp+112, the fctiwz slot at sp+120
        store<u32>(fr.addr(), nm);
        store<u32>(fr.addr() + 4, 0);
        call(ctx, 0x0057F5D8u, {t, 0x00F2FAF8u, fr.addr(), 0});
        u32 f = load<u32>(fr.addr() + 4);
        if (f == 0) {
            call(ctx, 0x0056A6E0u, {load<u32>(op + 16)});
            VCall(ctx, t, 12, {t});
            f = load<u32>(fr.addr() + 4);
        }
        t = f;
    }
    constexpr u32 K = 0x000778B4u;
    const f64 dur = Lfs51(op + 28);
    f64 v = Lfs51(op + 24);
    ctx.fpr[1] = Lfs51(K);
    if (dur >= ctx.fpr[1]) {
        const f64 at = Lfs51(c + 32);
        ctx.fpr[1] = at;
        if (at < dur) {
            const f64 a = Lfs51(op + 20);
            ctx.fpr[1] = F51(F51(v - a) * at);
            v = F51(a + F51(ctx.fpr[1] / dur));
        }
    }
    if (const u32 ty = (load<u32>(t + 4) >> 10) & 0x3Fu; ty != 8) {
        ret(ctx, ty);
        return;
    }
    const u64 iv = rt::fctiw(F51(v * Lfs51(K + 4)), true);
    // f1 holds the fctiwz result (the high word as the simulator models it)
    ctx.fpr[1] = std::bit_cast<f64>(iv | 0xFFF8000000000000ull);
    VCall(ctx, t, 52, {t, static_cast<u32>(iv) & 0xFFu, load<u8>(op + 32)});
}

// T* f(?, XmlCursor* x): a 44-byte def (Mem_Alloc(44, 0, 16); r3 = 0 when
// that fails): Obj_InitVt_B21C98(d, x), vtable 0x00B0FA18, +12..+40 = 0
// but +20 = -1; the hashes 0x8A93973F -> +12, 0x7A30D7E3 -> +16 and the
// s32 0xA212C671 -> +20; when any of those is missing the hashes
// 0x87208BC1, 0x29C248C8, 0x29C2BD27, 0x29C33186, 0x29C3A5E5 -> +24..+40
// (all default 0). r3 = d.
DECOMP(0007BD70, Def44b_ReadXml_0007BD70) {
    const u32 x = arg<u32>(ctx, 1);
    const u32 d = static_cast<u32>(call(ctx, 0x0054ED38u, {44, 0, 16}));
    if (d == 0) {
        ret(ctx, 0u);
        return;
    }
    call(ctx, 0x004BB3B0u, {d, x});
    store<u32>(d + 12, 0);
    store<u32>(d + 16, 0);
    store<u32>(d + 24, 0);
    store<u32>(d + 28, 0);
    store<u32>(d + 20, 0xFFFFFFFFu);
    store<u32>(d, 0x00B0FA18u);
    store<u32>(d + 32, 0);
    store<u32>(d + 36, 0);
    store<u32>(d + 40, 0);
    if (static_cast<u32>(call(ctx, 0x004FA14Cu, {x, 0x8A93973Fu, d + 12, 0})) == 0 ||
        static_cast<u32>(call(ctx, 0x004FA14Cu, {x, 0x7A30D7E3u, d + 16, 0})) == 0 ||
        static_cast<u32>(call(ctx, 0x004F99BCu, {x, 0xA212C671u, d + 20, 0})) == 0) {
        static constexpr u32 kKeys[5] = {0x87208BC1u, 0x29C248C8u, 0x29C2BD27u, 0x29C33186u, 0x29C3A5E5u};
        for (u32 i = 0; i < 5; ++i) call(ctx, 0x004FA14Cu, {x, kKeys[i], d + 24 + 4 * i, 0});
    }
    ret(ctx, d);
}

// T* f(?, XmlCursor* x): a 36-byte def (Mem_Alloc(36, 0, 16); r3 = 0 when
// that fails): Obj_InitVt_B21C98(d, x), vtable 0x00B111E0, +12..+28 = 0,
// u8 +32..+34 = 0; the hashes 0x3A1B33FA -> +12 (default 0), 0x001EFE3C ->
// +16, 0x7E6EFA38 -> +24, the s32 0x5CFBABDC -> +28 and the bools
// 0x6D18DE7D / 0x8488565B / 0xDB1F009E -> u8 +32..+34 (the old values as
// defaults); with +12 set and the node +20 empty (Ptr_DerefNonZero) it is
// released (RefNode_Release) and +20 = Node_PushF65B7C(+12, +16). r3 = d.
DECOMP(0009CFF4, Def36_ReadXml_0009CFF4) {
    const u32 x = arg<u32>(ctx, 1);
    const u32 d = static_cast<u32>(call(ctx, 0x0054ED38u, {36, 0, 16}));
    if (d == 0) {
        ret(ctx, 0u);
        return;
    }
    call(ctx, 0x004BB3B0u, {d, x});
    store<u32>(d + 12, 0);
    store<u32>(d + 16, 0);
    store<u32>(d + 20, 0);
    store<u32>(d + 24, 0);
    store<u32>(d, 0x00B111E0u);
    store<u32>(d + 28, 0);
    store<u8>(d + 32, 0);
    store<u8>(d + 33, 0);
    store<u8>(d + 34, 0);
    call(ctx, 0x004FA14Cu, {x, 0x3A1B33FAu, d + 12, 0});
    call(ctx, 0x004FA14Cu, {x, 0x001EFE3Cu, d + 16, load<u32>(d + 16)});
    call(ctx, 0x004FA14Cu, {x, 0x7E6EFA38u, d + 24, load<u32>(d + 24)});
    call(ctx, 0x004F99BCu, {x, 0x5CFBABDCu, d + 28, static_cast<u64>(static_cast<s64>(static_cast<s32>(load<u32>(d + 28))))});
    call(ctx, 0x004F986Cu, {x, 0x6D18DE7Du, d + 32, load<u8>(d + 32)});
    call(ctx, 0x004F986Cu, {x, 0x8488565Bu, d + 33, load<u8>(d + 33)});
    call(ctx, 0x004F986Cu, {x, 0xDB1F009Eu, d + 34, load<u8>(d + 34)});
    if (load<u32>(d + 12) != 0 && static_cast<u32>(call(ctx, 0x00446960u, {load<u32>(d + 20)})) == 0) {
        call(ctx, 0x004432DCu, {load<u32>(d + 20)});
        store<u32>(d + 20, static_cast<u32>(call(ctx, 0x00446984u, {load<u32>(d + 12), load<u32>(d + 16)})));
    }
    ret(ctx, d);
}

// T* f(?, XmlCursor* x, a, Obj* tmpl): a 288-byte def (SmallAlloc(288)):
// Wrapper_0CC4D4(d, a, tmpl, 0, 0x01164BD0, 0x01164B30, 1, 0, 0, .., 0;
// f1 = the .text pool 0x001C0E64 [0]), +8 |= 0x80000, +4 |= 4, +268 =
// +236 = 0, +240..+264 = -FLT_MAX, vtable 0x00B168C8, +272 = tmpl->408
// (pool [1] without tmpl), bits 31 / 30 of +276 = bits 31 / 30 of
// tmpl->412 (0 / 1 without tmpl), +8 |= 0x200000. Then (also for a failed
// allocation, on 0) XmlDef_Read_001C0C64(?, x, d). r3 = d.
DECOMP(001C0E70, Def288b_ReadXml_001C0E70) {
    const u32 a0 = arg<u32>(ctx, 0), x = arg<u32>(ctx, 1), a = arg<u32>(ctx, 2), tm = arg<u32>(ctx, 3);
    u32 d = static_cast<u32>(call(ctx, 0x0055238Cu, {288}));
    if (d != 0) {
        constexpr u32 K = 0x001C0E64u;
        callf(ctx, 0x000CC4D4u, {d, a, tm, 0, 0x01164BD0u, 0x01164B30u, 1, 0, 0, 0, 0}, {Lfs51(K)});
        const u32 f8 = load<u32>(d + 8), f4 = load<u32>(d + 4);
        store<u32>(d + 8, f8 | 0x00080000u);
        store<u32>(d + 268, 0);
        store<u32>(d + 4, f4 | 4u);
        store<u32>(d + 236, 0);
        for (u32 k = 0; k < 7; ++k) store<u32>(d + 240 + 4 * k, 0xFF7FFFFFu);
        store<u32>(d, 0x00B168C8u);
        const u32 f8b = load<u32>(d + 8);
        u32 w = load<u32>(d + 276);
        const u32 z = tm != 0 ? tm + 408 : K + 4;
        ctx.fpr[1] = Lfs51(z);
        Cp50(d + 272, z);
        w &= 0x7FFFFFFFu;
        const u32 b31 = tm != 0 ? load<u32>(tm + 412) >> 31 : 0;
        w |= b31 << 31;
        u32 b30 = 1;
        if (tm != 0) {
            store<u32>(d + 276, w);
            b30 = (load<u32>(tm + 412) >> 30) & 1u;
        }
        w = (w & ~0x40000000u) | (b30 << 30);
        store<u32>(d + 276, w);
        store<u32>(d + 8, f8b | 0x00200000u);
    }
    call(ctx, 0x001C0C64u, {a0, x, d});
    ret(ctx, d);
}

// void f(Obj* o, const Mat34* m): Obj_SyncBodies_000E5EF0(o, m); m (rows
// at +0 / +16 / +32 / +48) copied to o->624.. and on to o->560..; o->704..712
// = o->592..600 (the y row; its y zeroed when bit 0x80 of u8 o->784 is
// set), normalised when its squared length is at least the .text pool
// 0x001CC434 [0] (times pool [1] / length); o->720..728 = o->688..696 =
// that vector. r3 = the pool address, f1 = the z before normalising.
DECOMP(001CC440, Obj_SetMatrix_001CC440) {
    const u32 o = arg<u32>(ctx, 0), m = arg<u32>(ctx, 1);
    call(ctx, 0x000E5EF0u, {o, m});
    static constexpr u32 kOff[12] = {0, 4, 8, 16, 20, 24, 32, 36, 40, 48, 52, 56};
    for (u32 k : kOff) Cp50(o + 624 + k, m + k);
    for (u32 k : kOff) Cp50(o + 560 + k, o + 624 + k);
    Cp50(o + 704, o + 592);
    Cp50(o + 708, o + 596);
    Cp50(o + 712, o + 600);
    if ((load<u8>(o + 784) & 0x80u) != 0) store<u32>(o + 708, 0);
    const f64 x = Lfs51(o + 704), y = Lfs51(o + 708), z = Lfs51(o + 712);
    constexpr u32 K = 0x001CC434u;
    f64 q = F51(y * y);
    q = F51(std::fma(x, x, q));
    q = F51(std::fma(z, z, q));
    if (q >= Lfs51(K)) {
        const f64 s = F51(Lfs51(K + 4) / F51(std::sqrt(q)));
        store<f32>(o + 704, static_cast<f32>(F51(x * s)));
        store<f32>(o + 708, static_cast<f32>(F51(y * s)));
        store<f32>(o + 712, static_cast<f32>(F51(z * s)));
    }
    const u32 vx = load<u32>(o + 704), vy = load<u32>(o + 708), vz = load<u32>(o + 712);
    store<u32>(o + 720, vx);
    store<u32>(o + 724, vy);
    store<u32>(o + 728, vz);
    store<u32>(o + 688, vx);
    store<u32>(o + 692, vy);
    store<u32>(o + 696, vz);
    ctx.fpr[1] = z;
    ret(ctx, K);
}

// T* f(a, b, Src* x, d): as 0x001EB2B8 (obj50b.cpp) for the 320-byte kind:
// the damage-collision defaults record on the stack (Def_Construct74FD80(
// rec, x->12, x->16, x->20, r7 = the pool 0x001DBE6C, r8 = x->20,
// 0x9260C26E, 0x01164B40; the same float layout), rec->224 = 0, rec->8 |=
// 4, DamageColDef_ReadDBC28(a, b, x, d, rec), then a 320-byte object
// (SmallAlloc; Thunk_000E27FC(m, rec); vtable 0x00B17B88, +304 =
// rec->224) and the record's vector +124 freed. r3 = m (0 on failure).
DECOMP(001DBE80, DamageCol320_New_001DBE80) {
    const u32 a = arg<u32>(ctx, 0), b = arg<u32>(ctx, 1), x = arg<u32>(ctx, 2), d = arg<u32>(ctx, 3);
    constexpr u32 K = 0x001DBE6Cu;
    const f64 k0 = Lfs51(K), k4 = Lfs51(K + 4), k8 = Lfs51(K + 8), k12 = Lfs51(K + 12), k16 = Lfs51(K + 16);
    const u64 f3w = load<u32>(K + 12), f2w = load<u32>(K + 16);
    const u32 x20 = load<u32>(x + 20);
    Stack fr(ctx, 256);  // the record at sp+368 (frame of 656 bytes)
    const u32 rec = fr.addr();
    // stack slots 8..18 are not set by the original (the callee's FPR slots)
    // clang-format off
    callf(ctx, 0x0074FD80u, {rec, load<u32>(x + 12), load<u32>(x + 16), x20, K, x20, 0x9260C26Eu, 0x01164B40u, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, f3w, f3w, f3w, f3w, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, f2w, f2w, 0}, {k0, k16, k12, k12, k12, k12, k4, k8, k16, k12, k12, k12, k12});
    // clang-format on
    const u32 f8 = load<u32>(rec + 8);
    store<u32>(rec + 224, 0);
    store<u32>(rec + 8, f8 | 4u);
    call(ctx, 0x001DBC28u, {a, b, x, d, rec});
    const u32 m = static_cast<u32>(call(ctx, 0x0055238Cu, {320}));
    u32 r = 0;
    if (m != 0) {
        call(ctx, 0x000E27FCu, {m, rec});
        r = m;
        store<u32>(m, 0x00B17B88u);
        store<u32>(m + 304, load<u32>(rec + 224));
    }
    FreeVec0_50(ctx, rec + 124);
    ret(ctx, r);
}

// void f(Rig* o, u32 i, Mat34* out): the matrix of o's attach point i
// (o->28[i]: bone index +0, or the node +4) into out: the bone's
// (Obj_BoneMatrixCE688(&m, o->8, bone, 1, 1)), else node->80.., else
// o->8->96..; normalised (Matrix_NormalizeInPlace) when bit 0x2000 of
// o->8->160->164 is set. r3 / f1 as the original leaves them.
DECOMP(00268FD8, Rig_PointMatrix_00268FD8) {
    const u32 o = arg<u32>(ctx, 0), i = arg<u32>(ctx, 1), out = arg<u32>(ctx, 2);
    const u32 obj = load<u32>(o + 8);
    const u32 e = load<u32>(load<u32>(o + 28) + (i << 2));
    const u32 bone = load<u32>(e);
    static constexpr u32 kOff[12] = {0, 4, 8, 16, 20, 24, 32, 36, 40, 48, 52, 56};
    auto copy = [&](u32 src) {
        for (u32 k : kOff) Cp50(out + k, src + k);
        ctx.fpr[1] = Lfs51(src + 56);
    };
    if (bone != 0xFFFFFFFFu) {
        Stack fr(ctx, 64);  // the matrix at sp+112
        call(ctx, 0x000CE688u, {fr.addr(), obj, static_cast<u64>(static_cast<s64>(static_cast<s32>(bone))), 1, 1});
        copy(fr.addr());
    } else if (const u32 n = load<u32>(e + 4); n != 0) {
        copy(n + 80);
    } else {
        copy(obj + 96);
    }
    const u32 bit = (load<u32>(load<u32>(obj + 160) + 164) >> 13) & 1u;
    if (bit == 0) {
        ret(ctx, 0u);
        return;
    }
    call(ctx, 0x0056D374u, {out});
}

// void f(T* o): vtable 0x00B1CD58; each set item of o->24 (u16 count
// o->32) gets item->vslot 4(item); the count zeroed, the PtrVectors +44 /
// +24 cleared (ClearVec50); vtable 0x00B1BE30, Unlist1056F1C_51(o) and
// List1013B5C_Remove(o) (r3 = its).
DECOMP(002B4830, Dtor_B1CD58_002B4830) {
    const u32 o = arg<u32>(ctx, 0);
    store<u32>(o, 0x00B1CD58u);
    for (u32 i = 0; i < load<u16>(o + 32); ++i) {
        const u32 e = load<u32>(load<u32>(o + 24) + 4 * i);
        if (e != 0) VCall(ctx, e, 4, {e});
    }
    store<u16>(o + 32, 0);
    ClearVec50(ctx, o + 44);
    ClearVec50(ctx, o + 24);
    store<u32>(o, 0x00B1BE30u);
    Unlist1056F1C_51(o);
    call(ctx, 0x000D6940u, {o});
}
