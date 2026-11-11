// Game code, part 41 (continued): more of the small-function sweep, from
// the 88 instruction functions up. Each was checked against the original in
// the PPC simulator (memory writes, calls and their arguments, r3 and f1).
#include "obj41.h"

using namespace sly4;
using namespace sly4::obj41;

// ---------------------------------------------------------------- sweep: 88 instructions
// void f(T* o, u8 keep): o is unlinked from the node chains of the
// registry 0x010BB738 (u16 count +2, 8-byte {key, holder} entries +8;
// matched nodes {obj, next} get obj = 0; a holder left empty is dropped
// with SortedMap_Remove786A18(reg, &key) and Mem_FreeFrom(holder, 0), the
// same index looked at again); then keep clears bit 13 of u16 o->+176,
// otherwise u8 o->+4 = 0. r3 as the original leaves it.
DECOMP(0048F298, Registry_Unlink_0048F298) {
    const u32 o = arg<u32>(ctx, 0), keep = arg<u32>(ctx, 1) & 0xFFu;
    constexpr u32 R = 0x010BB738u;
    for (s32 i = 0; i < static_cast<s32>(load<u16>(R + 2)); ++i) {
        const u32 d = load<u32>(R + 8);
        const u32 hold = load<u32>(d + 8 * static_cast<u32>(i) + 4);
        u32 head = load<u32>(hold);
        if (load<u32>(head) == o) {
            const u32 nx = load<u32>(head + 4);
            store<u32>(head, 0);
            store<u32>(hold, nx);
            head = nx;
        } else {
            u32 prev = head;
            for (u32 q = load<u32>(head + 4); q != 0; prev = q, q = load<u32>(prev + 4)) {
                if (load<u32>(q) != o) continue;
                const u32 nx = load<u32>(q + 4);
                store<u32>(q, 0);
                store<u32>(prev + 4, nx);
                break;
            }
        }
        if (head == 0) {
            Stack fr(ctx, 16);  // the key at sp+112
            store<u32>(fr.addr(), load<u32>(d + 8 * static_cast<u32>(i)));
            call(ctx, 0x00786A18u, {R, fr.addr()});
            call(ctx, 0x0054EE6Cu, {hold, 0});
            --i;
        }
    }
    if (keep != 0) {
        const u32 h = load<u16>(o + 176) & ~0x2000u;
        store<u16>(o + 176, static_cast<u16>(h));
        ret(ctx, h);
        return;
    }
    store<u8>(o + 4, 0);
    ret(ctx, 0u);
}

// T* New(?, XmlCursor* x): a 28-byte object (Mem_Alloc(28, 0, 16); r3 = 0
// on failure), u16 +4 = 1, +12..+20 = 0, vtable 0x00B22178; read: hash
// 0x8A93973F into +16 (then, when that read returned nonzero, the s32
// 0x5C8998CD), hash 0x93116591 into +12 (then the s32 0x39F3EE1F), hash
// 0x00255BF3 into
// +20 and the bool 0x6334AE6F into u8 +24 (all default 0). r3 = the
// object.
DECOMP(004C1D0C, NewXmlB22178_004C1D0C) {
    const u32 x = arg<u32>(ctx, 1);
    const u32 p = static_cast<u32>(call(ctx, 0x0054ED38u, {28, 0, 16}));
    if (p == 0) {
        ret(ctx, 0u);
        return;
    }
    store<u32>(p + 12, 0);
    store<u16>(p + 4, 1);
    store<u32>(p + 16, 0);
    store<u32>(p + 20, 0);
    store<u32>(p, 0x00B22178u);
    if (call(ctx, 0x004FA14Cu, {x, 0x8A93973Fu, p + 16, 0}) != 0) call(ctx, 0x004F99BCu, {x, 0x5C8998CDu, p + 16, 0});
    if (call(ctx, 0x004FA14Cu, {x, 0x93116591u, p + 12, 0}) != 0) call(ctx, 0x004F99BCu, {x, 0x39F3EE1Fu, p + 12, 0});
    call(ctx, 0x004FA14Cu, {x, 0x00255BF3u, p + 20, 0});
    call(ctx, 0x004F986Cu, {x, 0x6334AE6Fu, p + 24, 0});
    ret(ctx, p);
}

// u32 f(T* o, XmlCursor* x): u16 +4 = 1, +12..+20 = 0, vtable 0x00B223F8,
// u8 +24 = 0; read: hash 0xE42AC27C into +12 (default 0), 0xDA98E416 into
// +16, the f32 0x04DA9951 into +20 (default the current values) and the
// hash 0x0932E547 (default 0), which selects u8 +24 = 4 / 3 / 1 / 2 for
// 0x78367079 / 0x559E9624 / 0x0000FC81 / 0xB3D90B14. r3 = that value (the
// hash when none matched).
DECOMP(004CA888, XmlInit_B223F8_004CA888) {
    const u32 o = arg<u32>(ctx, 0), x = arg<u32>(ctx, 1);
    store<u16>(o + 4, 1);
    store<u32>(o + 12, 0);
    store<u32>(o + 16, 0);
    store<u32>(o + 20, 0);
    store<u32>(o, 0x00B223F8u);
    store<u8>(o + 24, 0);
    call(ctx, 0x004FA14Cu, {x, 0xE42AC27Cu, o + 12, 0});
    call(ctx, 0x004FA14Cu, {x, 0xDA98E416u, o + 16, load<u32>(o + 16)});
    callf(ctx, 0x004F9B20u, {x, 0x04DA9951u, o + 20}, {Lfs40(o + 20)});
    Stack fr(ctx, 16);  // the hash at sp+112
    store<u32>(fr.addr(), 0);
    call(ctx, 0x004FA14Cu, {x, 0x0932E547u, fr.addr(), 0});
    const u32 h = load<u32>(fr.addr());
    u32 v;
    switch (h) {
    case 0x78367079u: v = 4; break;
    case 0x559E9624u: v = 3; break;
    case 0x0000FC81u: v = 1; break;
    case 0xB3D90B14u: v = 2; break;
    default: ret(ctx, h); return;
    }
    store<u8>(o + 24, static_cast<u8>(v));
    ret(ctx, v);
}

// u32 f(T* o, Effect* x): the level f32 o->+16 is refreshed from x's
// component o->+20 of kind 1 (vt[32]) through its vt[20] (0x004CC924 when
// the component is missing; nothing without +20) and cleared below
// 0x004CC924; then the bound part b = Effect_Bind4BB7E0(x, o->+12, u8
// o->+25) and (with u8 o->+24) its children (b->+272, then the chain
// +268) of type 10 (bits 10..15 of +4) pass it to ((b->+208)->+112)->+16.
// r3 as the original leaves it; f1 = the level.
DECOMP(004CC928, Effect_PushLevel_004CC928) {
    const u32 o = arg<u32>(ctx, 0), x = arg<u32>(ctx, 1);
    const u32 c = load<u32>(o + 20);
    f64 f = Lfs40(o + 16);
    if (c != 0) {
        const u32 e = static_cast<u32>(call(ctx, 0x004BB4B4u, {x, c}));
        if (e != 0 && static_cast<u32>(VCall(ctx, e, 32, {e})) == 1) {
            VCall(ctx, e, 20, {e});
            f = ctx.fpr[1];
        } else {
            f = Lfs40(0x004CC924u);
        }
        store<u32>(o + 16, W41(f));
    }
    ctx.fpr[1] = f;
    if (f < Lfs40(0x004CC924u)) store<u32>(o + 16, 0);
    u32 b = static_cast<u32>(call(ctx, 0x004BB7E0u, {x, load<u32>(o + 12), load<u8>(o + 25)}));
    const u32 fw = load<u32>(o + 16);
    ctx.fpr[1] = static_cast<f64>(std::bit_cast<f32>(fw));
    if (b == 0) {
        ret(ctx, 0u);
        return;
    }
    const bool once = load<u8>(o + 24) == 0;
    for (bool first = true; b != 0; first = false) {
        if (((load<u32>(b + 4) >> 10) & 0x3Fu) == 10) {
            const u32 t = load<u32>(b + 208);
            if (t != 0) {
                const u32 u = load<u32>(t + 112);
                if (u != 0) store<u32>(u + 16, fw);
            }
        }
        if (once) {
            ret(ctx, b);
            return;
        }
        b = load<u32>(b + (first ? 272 : 268));
    }
    ret(ctx, 0u);
}

// Node* f(Tree* o, Node* n): unless n is visited (u8 +33): a leaf (n->+8
// == 0) is pushed on the slot list o + 104 (SlotList_PushFront_0079A6D4),
// otherwise {n->+8, n} is appended to the 8-byte array o + 32 (u16
// capacity +32, count +34, high-water +36, flags +38 |= 1, data +40;
// doubled by Array_Reserve when full) and an unmarked one (u8 n->+32 == 0)
// counted in +140; then n is marked and its children (n->+20, count
// (n->+16)->+28) visited the same way (recursively). r3 = n.
DECOMP(004F480C, Tree_Visit_004F480C) {
    const u32 o = arg<u32>(ctx, 0), n = arg<u32>(ctx, 1);
    Stack fr(ctx, 16);  // n at sp+112, the push result at sp+116
    store<u32>(fr.addr(), n);
    if (load<u8>(n + 33) != 0) {
        ret(ctx, n);
        return;
    }
    u32 m;
    if (load<u32>(n + 8) == 0) {
        call(ctx, 0x0079A6D4u, {fr.addr() + 4, o + 104, fr.addr()});
        m = load<u32>(fr.addr());
    } else {
        const u32 cap = load<u16>(o + 32);
        const u32 v8 = load<u32>(n + 8);
        if (load<u16>(o + 34) == cap) call(ctx, 0x00558F3Cu, {o + 32, (cap << 1) & 0xFFFEu});
        m = load<u32>(fr.addr());
        const u32 d = load<u32>(o + 40);
        const u32 c = load<u16>(o + 34);
        store<u32>(d + ((c << 3) & 0x7FFF8u), v8);
        const u32 c1 = c + 1;
        const u32 hw = load<u16>(o + 36);
        store<u16>(o + 34, static_cast<u16>(c1));
        store<u32>(d + (c << 3) + 4, n);
        store<u16>(o + 38, static_cast<u16>(load<u16>(o + 38) | 1u));
        const u32 mark = load<u8>(m + 32);
        store<u16>(o + 36, static_cast<u16>(static_cast<s32>(hw) > static_cast<s32>(c1 & 0xFFFFu) ? hw : c1));
        if (mark == 0) store<u32>(o + 140, load<u32>(o + 140) + 1);
    }
    store<u8>(m + 33, 1);
    for (u32 i = 0;; ++i) {
        const u32 q = load<u32>(fr.addr());
        if (i >= load<u32>(load<u32>(q + 16) + 28)) {
            ret(ctx, q);
            return;
        }
        call(ctx, 0x004F480Cu, {o, load<u32>(load<u32>(q + 20) + 4 * i)});
    }
}

// Node* New(s16 id): a 24-byte node 0x00B1FD28 with the bindings
// 0x00B1FD38 (5, 0), 0x00B1FD41 (5, 1; +76 = the string 0x00B1FD48) and
// 0x00B1FD49 (4, 1) (NodeNew3Bind41).
DECOMP(007776B8, NodeNew_B1FD28_007776B8) {
    NodeNew3Bind41(ctx, 0x00B1FD28u, {0x00B1FD38u, 0x00B1FD41u, 0x00B1FD49u}, {5, 5, 4}, {0, 1, 1}, {0, 0x00B1FD48u, 0});
}

// ---------------------------------------------------------------- sweep: 89 instructions
// u32 f(T* o, u32 ev): the step +368 counts up, wrapping to 0 at +360;
// then o->vt[220](o, k) with k = 4 / 3 / 2 / 7 / 10 for the events
// 0x04B38BFF / 0x040A726F / 0x002237D4 / 0xFB4F8FFE / 0xFB497E59 and 2
// for any other (r3 = its).
DECOMP(000296BC, Obj_EventPose_000296BC) {
    const u32 o = arg<u32>(ctx, 0), ev = arg<u32>(ctx, 1);
    const u32 n = load<u32>(o + 368) + 1;
    const u32 lim = load<u32>(o + 360);
    store<u32>(o + 368, n);
    if (!(n < lim)) store<u32>(o + 368, 0);
    u32 k;
    switch (ev) {
    case 0x04B38BFFu: k = 4; break;
    case 0x040A726Fu: k = 3; break;
    case 0x002237D4u: k = 2; break;
    case 0xFB4F8FFEu: k = 7; break;
    case 0xFB497E59u: k = 10; break;
    default: k = 2; break;
    }
    VCall(ctx, o, 220, {o, k});
}

// u32 f(T* o, a): bit 3 of u8 +305 set, bit 1 of u8 +464 set and bit 0
// cleared; the basis is rebuilt from the reversed direction -(+160..+168)
// (Basis_FromForward_00015F90(o, &v)); Queue_Push_004705F8(q, q,
// 0xE0F679CC, 0, 1) with q = o->+48; the shared settings 0x011DE094 are set up on first
// use (u8 0x011DE280: +20 / +24 = 0, bits 7 / 6 of u8 +32 cleared, 400
// bytes at +92 cleared, *0x011972E8 = it); then sub_002696E0(o->+68, a)
// and o->vt[292](o) with f1 = f32 0x011DE098 (r3 = its).
DECOMP(00033A40, Obj_TurnAround_00033A40) {
    const u32 o = arg<u32>(ctx, 0), a = arg<u32>(ctx, 1);
    const u32 b464 = load<u8>(o + 464), b305 = load<u8>(o + 305);
    store<u8>(o + 305, static_cast<u8>(b305 | 8u));
    store<u8>(o + 464, static_cast<u8>((b464 | 2u) & 0xFEu));
    {
        Stack fr(ctx, 16);  // the vector at sp+112
        for (u32 k = 0; k < 3; ++k) store<u32>(fr.addr() + 4 * k, load<u32>(o + 160 + 4 * k) ^ 0x80000000u);  // fneg
        call(ctx, 0x00015F90u, {o, fr.addr()});
    }
    const u32 q = load<u32>(o + 48);
    call(ctx, 0x004705F8u, {q, q, 0xE0F679CCu, 0, 1});
    constexpr u32 G = 0x011DE094u;
    if (load<u8>(0x011DE280u) == 0) {
        const u32 b = load<u8>(G + 32);
        store<u32>(G + 20, 0);
        store<u32>(G + 24, 0);
        store<u8>(G + 32, static_cast<u8>(b & 0x3Fu));
        call(ctx, 0x00726E54u, {G + 92, 0, 400});
        store<u32>(0x011972E8u, G);
        store<u8>(0x011DE280u, 1);
    }
    const f64 f = Lfs40(G + 4);
    callf(ctx, 0x002696E0u, {load<u32>(o + 68), a}, {f});
    ctx.fpr[1] = f;
    VCall(ctx, o, 292, {o});
}

// T* New(?, XmlCursor* x): NewXml6_41 with the vtable 0x00B0F680.
DECOMP(00075144, NewXmlB0F680_00075144) { NewXml6_41(ctx, 0x00B0F680u); }

// T* New(?, XmlCursor* x): a 32-byte object (Mem_Alloc(32, 0, 16); r3 = 0
// on failure) made by Obj_InitVt_B21C98, vtable 0x00B0F770, +12 = 0, f32
// +16 = 1.0, +20 / +24 = 0, u8 +28 = 0; read: hash 0x7F247172 into +12
// (default 0); only when that read returned 0: the f32 0x00210FAA /
// 0x00000A7B / 0xB7EA3D94 into +16 / +20 / +24 (defaults 0x00076FC0 /
// 0x00076FC4 / 0x00076FC4) and the bool 0xF5897C08 into u8 +28 (default
// 0). r3 = the object.
DECOMP(00076FC8, NewXmlB0F770_00076FC8) {
    const u32 x = arg<u32>(ctx, 1);
    const u32 p = static_cast<u32>(call(ctx, 0x0054ED38u, {32, 0, 16}));
    if (p == 0) {
        ret(ctx, 0u);
        return;
    }
    call(ctx, 0x004BB3B0u, {p, x});
    store<u32>(p + 12, 0);
    store<u32>(p, 0x00B0F770u);
    const f64 d0 = Lfs40(0x00076FC0u), d1 = Lfs40(0x00076FC4u);
    store<u32>(p + 16, 0x3F800000u);
    store<u32>(p + 20, 0);
    store<u32>(p + 24, 0);
    store<u8>(p + 28, 0);
    if (call(ctx, 0x004FA14Cu, {x, 0x7F247172u, p + 12, 0}) == 0) {
        callf(ctx, 0x004F9B20u, {x, 0x00210FAAu, p + 16}, {d0});
        callf(ctx, 0x004F9B20u, {x, 0x00000A7Bu, p + 20}, {d1});
        callf(ctx, 0x004F9B20u, {x, 0xB7EA3D94u, p + 24}, {d1});
        call(ctx, 0x004F986Cu, {x, 0xF5897C08u, p + 28, 0});
    }
    ret(ctx, p);
}

// T* New(?, XmlCursor* x): a 36-byte object (Mem_Alloc(36, 0, 16); r3 = 0
// on failure) made by Obj_InitVt_B21C98, vtable 0x00B102D8, +12..+20 = 0,
// +24 = -1, +28 = 0, u8 +32 = 0; read: hashes 0x00014B07 (default 0) /
// 0x7A30D7E3 / 0x03D1FCFC into +12..+20, the s32 0xA212C671 into +24, the
// hash 0x52A72CDD into +28 and the bool 0x469D7C52 into u8 +32 (default
// the current values). r3 = the object.
DECOMP(00088330, NewXmlB102D8_00088330) {
    const u32 x = arg<u32>(ctx, 1);
    const u32 p = static_cast<u32>(call(ctx, 0x0054ED38u, {36, 0, 16}));
    if (p == 0) {
        ret(ctx, 0u);
        return;
    }
    call(ctx, 0x004BB3B0u, {p, x});
    store<u32>(p + 12, 0);
    store<u32>(p + 16, 0);
    store<u32>(p + 20, 0);
    store<u32>(p, 0x00B102D8u);
    store<u32>(p + 24, 0xFFFFFFFFu);
    store<u32>(p + 28, 0);
    store<u8>(p + 32, 0);
    call(ctx, 0x004FA14Cu, {x, 0x00014B07u, p + 12, 0});
    call(ctx, 0x004FA14Cu, {x, 0x7A30D7E3u, p + 16, load<u32>(p + 16)});
    call(ctx, 0x004FA14Cu, {x, 0x03D1FCFCu, p + 20, load<u32>(p + 20)});
    call(ctx, 0x004F99BCu, {x, 0xA212C671u, p + 24, Sx40(load<u32>(p + 24))});
    call(ctx, 0x004FA14Cu, {x, 0x52A72CDDu, p + 28, load<u32>(p + 28)});
    call(ctx, 0x004F986Cu, {x, 0x469D7C52u, p + 32, load<u8>(p + 32)});
    ret(ctx, p);
}

// T* New(?, XmlCursor* x): a 24-byte object (Mem_Alloc(24, 0, 16); r3 = 0
// on failure) made by Obj_InitVt_B21C98, vtable 0x00B10800, +12 / +16 =
// 0, u8 +20..+23 = 0; read: the s32 0x3683141C (through a local, default
// 0) gives +12 = max(n - 1, 0) when that read returned 0; hash 0xE33D410A
// into +16; the bools 0x03C5CC6D / 0x7A281983 into u8 +20 / +22 (u8 +21
// / +23 = 1 when the read returned 0). r3 = the object.
DECOMP(0008DB0C, NewXmlB10800_0008DB0C) {
    const u32 x = arg<u32>(ctx, 1);
    const u32 p = static_cast<u32>(call(ctx, 0x0054ED38u, {24, 0, 16}));
    if (p == 0) {
        ret(ctx, 0u);
        return;
    }
    call(ctx, 0x004BB3B0u, {p, x});
    store<u32>(p + 12, 0);
    store<u32>(p + 16, 0);
    store<u8>(p + 20, 0);
    store<u8>(p + 21, 0);
    store<u8>(p + 22, 0);
    store<u32>(p, 0x00B10800u);
    store<u8>(p + 23, 0);
    {
        Stack fr(ctx, 16);  // the s32 at sp+112
        store<u32>(fr.addr(), 0);
        if (call(ctx, 0x004F99BCu, {x, 0x3683141Cu, fr.addr(), 0}) == 0) {
            const s32 v = static_cast<s32>(load<u32>(fr.addr()) - 1);
            store<u32>(p + 12, v < 0 ? 0u : static_cast<u32>(v));
        }
    }
    call(ctx, 0x004FA14Cu, {x, 0xE33D410Au, p + 16, 0});
    if (call(ctx, 0x004F986Cu, {x, 0x03C5CC6Du, p + 20, 0}) == 0) store<u8>(p + 21, 1);
    if (call(ctx, 0x004F986Cu, {x, 0x7A281983u, p + 22, 0}) == 0) store<u8>(p + 23, 1);
    ret(ctx, p);
}

// u32 f(T* o, Effect* x): the target t (EffTarget41) with bit 27 of
// +196 set is linked to the spline (*0x00F8F158)->+16 entry for o->+20
// (SplineMap_Find; Obj_SetLink_001D3968(t, it), when +20 and the entry are
// set) and toggled (Flag_Toggle972_001D3A00(t)) when u8 o->+24 is set. r3
// as the original leaves it.
DECOMP(0009057C, EffCmd_Link_0009057C) {
    const u32 o = arg<u32>(ctx, 0), x = arg<u32>(ctx, 1);
    const u32 t = EffTarget41(ctx, o, x);
    if (t == 0 || (load<u32>(t + 196) & 0x08000000u) == 0) {
        ret(ctx, 0u);
        return;
    }
    const u32 s = load<u32>(o + 20);
    if (s != 0) {
        Stack fr(ctx, 16);  // the key at sp+112
        store<u32>(fr.addr(), s);
        const u32 r = static_cast<u32>(call(ctx, 0x007335C4u, {load<u32>(load<u32>(0x00F8F158u) + 16), fr.addr()}));
        const u32 v = load<u32>(r);
        if (v != 0) call(ctx, 0x001D3968u, {t, v});
    }
    if (load<u8>(o + 24) == 0) {
        ret(ctx, 0u);
        return;
    }
    call(ctx, 0x001D3A00u, {t});
}

// T* New(?, XmlCursor* x): NewXml6_41 with the vtable 0x00B10E40.
DECOMP(00099B64, NewXmlB10E40_00099B64) { NewXml6_41(ctx, 0x00B10E40u); }

// u32 f(T* o, u32 key): each 8-byte entry e of the u16 counted array
// o->+52 (count +60) whose object e->+0 is set and either lacks bit 18 of
// +172 or has +168 == key is removed (the last entry moves into its place,
// count - 1, the same index looked at again); unless o->+148 is set its
// e->+4 is first appended to the pointer vector o + 92 (u16 capacity +96,
// count +100, high-water +102; doubled with PtrVector_Reserve). r3 as the
// original leaves it.
DECOMP(000B7574, Obj_FlushEntries_000B7574) {
    const u32 o = arg<u32>(ctx, 0), key = arg<u32>(ctx, 1);
    u64 r3 = o;
    for (s32 i = 0; i < static_cast<s32>(load<u16>(o + 60)); ++i) {
        u32 arr = load<u32>(o + 52);
        r3 = arr;
        const u32 e = arr + 8 * static_cast<u32>(i);
        const u32 a = load<u32>(e);
        if (a == 0) continue;
        if ((load<u32>(a + 172) & 0x40000u) != 0 && load<u32>(a + 168) != key) continue;
        u32 n = load<u16>(o + 60);
        if (load<u32>(o + 148) == 0) {
            const u32 cap = load<u16>(o + 96);
            if (load<u16>(o + 100) == cap) call(ctx, 0x00551718u, {o + 92, (cap << 1) & 0x1FFFEu});
            const u32 c = load<u16>(o + 100);
            store<u32>(load<u32>(o + 92) + ((c << 2) & 0x3FFFCu), load<u32>(e + 4));
            const u32 c1 = load<u16>(o + 100) + 1;
            const u32 hw = load<u16>(o + 102);
            arr = load<u32>(o + 52);
            r3 = arr;
            store<u16>(o + 100, static_cast<u16>(c1));
            n = load<u16>(o + 60);
            store<u16>(o + 102, static_cast<u16>(static_cast<s32>(hw) > static_cast<s32>(c1 & 0xFFFFu) ? hw : c1));
        }
        const u32 n1 = n - 1;
        store<u16>(o + 60, static_cast<u16>(n1));
        if (i != static_cast<s32>(n1 & 0xFFFFu)) {
            const u32 last = arr + ((n1 << 3) & 0x7FFF8u);
            const u32 dst = arr + 8 * static_cast<u32>(i);
            const u32 w0 = load<u32>(last), w1 = load<u32>(last + 4);
            store<u32>(dst, w0);
            store<u32>(dst + 4, w1);
            r3 = w1;
        }
        --i;
    }
    ctx.gpr[3] = r3;
}

// u32 f(T* o): Nop_000D8FC0(o); while bit 4 of u8 +968 is set (and not
// bit 5 / 6): with bit 8 of +172 Obj_UpdateFlagged_00123F70(o); bit 2 of
// +968 = bit 8 of +172 and o->vt[88](o, 0, 1, 1, 0) (r3 = its). With bit
// 5 or 6 of +968: with bit 8 of +172 Obj_ApplyTree_000D8F08(o, o + 969,
// bit 5), then bit 2 = bit 8 and o->vt[88](o, 0, 1, 1, 0), and the test
// repeats. r3 as the original leaves it.
DECOMP(00124034, Obj_SyncTree_00124034) {
    const u32 o = arg<u32>(ctx, 0);
    call(ctx, 0x000D8FC0u, {o});
    u32 b = load<u8>(o + 968);
    u32 w = load<u32>(o + 172);
    auto push = [&]() {
        const u32 nb = (b & ~4u) | ((w & 0x100u) != 0 ? 4u : 0u);
        store<u8>(o + 968, static_cast<u8>(nb));
        return VCall(ctx, o, 88, {o, 0, 1, 1, 0});
    };
    if ((b & 0x60u) != 0) {
        if ((w & 0x100u) != 0) {
            call(ctx, 0x000D8F08u, {o, o + 969, (b & 0x20u) != 0 ? 1u : 0u});
            b = load<u8>(o + 968);
            w = load<u32>(o + 172);
        }
        push();
        b = load<u8>(o + 968);
        w = load<u32>(o + 172);
    }
    if ((b & 0x10u) == 0) {
        ret(ctx, w);
        return;
    }
    if ((w & 0x100u) != 0) {
        call(ctx, 0x00123F70u, {o});
        w = load<u32>(o + 172);
        b = load<u8>(o + 968);
    }
    push();
}

// u32 f(T* o, u32 key): SetPickEffect41 on +416.
DECOMP(00177078, Obj_PickSetEffect_00177078) { SetPickEffect41(ctx, 416); }

// u32 f(T* o, u32 key): SetPickEffect41 on +420.
DECOMP(001771DC, Obj_PickSetEffect_001771DC) { SetPickEffect41(ctx, 420); }
