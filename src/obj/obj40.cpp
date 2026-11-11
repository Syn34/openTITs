// Game code, part 40: the small-function sweep continued from the 83
// instruction functions (size order; obj39*.cpp did 80-83). Each was
// checked against the original in the PPC simulator (memory writes, calls
// and their arguments, r3 and f1).
#include "obj40.h"

using namespace sly4;
using namespace sly4::obj40;

// ---------------------------------------------------------------- sweep: 83 instructions
// void f(T* o, s32 key): the first of the 8 40-byte slots e = o + 40 +
// 40 i with e->+0 == key is refreshed from the animation owner o->+36
// (Obj_FindComp34CC34(o->+36, key) each time): u8 e->+32 =
// sub_00353EB0(c, 0) (1 without owner or component), f32 e->+12 =
// Track_Time353780(c) (0.0), e->+36 = Field_Get32_68_00353E88(c) (0).
// r3 as the original leaves it.
DECOMP(00298D98, AnimSlots_Refresh_00298D98) {
    const u32 o = arg<u32>(ctx, 0), key = arg<u32>(ctx, 1);
    u32 i = 0;
    while (load<u32>(o + 40 + 40 * i) != key) {
        ++i;
        if (i >= 8) {
            ctx.gpr[3] = static_cast<u64>(o) + 320;  // the scan pointer
            return;
        }
    }
    const u32 e = o + 40 * i + 40;
    ctx.gpr[3] = o + 40 * i;
    if (e == 0) return;
    u64 r3;
    u32 t = load<u32>(o + 36);
    r3 = 1;
    if (t != 0) {
        const u32 c = static_cast<u32>(call(ctx, 0x0034CC34u, {t, key}));
        if (c != 0) r3 = call(ctx, 0x00353EB0u, {c, 0});
    }
    store<u8>(e + 32, static_cast<u8>(r3));
    u32 f = load<u32>(0x00298D94u);  // 0.0f, kept raw
    t = load<u32>(o + 36);
    r3 = t;
    if (t != 0) {
        const u32 c = static_cast<u32>(call(ctx, 0x0034CC34u, {t, key}));
        r3 = c;
        if (c != 0) {
            r3 = call(ctx, 0x00353780u, {c});
            f = std::bit_cast<u32>(static_cast<f32>(ctx.fpr[1]));
        }
    }
    store<u32>(e + 12, f);
    u32 v = 0;
    t = load<u32>(o + 36);
    if (t != 0) {
        const u32 c = static_cast<u32>(call(ctx, 0x0034CC34u, {t, key}));
        r3 = c;
        if (c != 0) {
            v = static_cast<u32>(call(ctx, 0x00353E88u, {c}));
            r3 = v;
        }
    }
    store<u32>(e + 36, v);
    ctx.gpr[3] = r3;
}

// bool f(T* o, f32 x): with the view p = o->+0 (0 without), the point
// View_Project2D_004A5854(&s, p + 64) is checked: before the first hit (u8
// o->+16 clear) a point inside (0, 1) x (0, 1) sets f32 o->+4 = x when it
// was -1.0, raises o->+12 to x and returns 1; outside, x > o->+12 > 0.0
// only sets o->+16 (r3 0). Afterwards: 1 when o->+4 <= x <= o->+12.
DECOMP(003F0118, View_InWindow_003F0118) {
    const u32 o = arg<u32>(ctx, 0);
    const f64 x = farg(ctx, 0);
    const u32 p = load<u32>(o);
    if (p == 0) {
        ret(ctx, 0u);
        return;
    }
    Stack fr(ctx, 32);  // the point at sp+112
    call(ctx, 0x004A5854u, {fr.addr(), p + 64});
    const f64 a = Lfs40(fr.addr()), b = Lfs40(fr.addr() + 4);
    const f64 hi = Lfs40(o + 12);
    ctx.fpr[1] = hi;  // left in f1
    if (load<u8>(o + 16) != 0) {
        ret(ctx, (Lfs40(o + 4) <= x && hi >= x) ? 1u : 0u);
        return;
    }
    if (a > 0.0 && a < 1.0 && b > 0.0 && b < 1.0) {
        if (Lfs40(o + 4) == -1.0) store<f32>(o + 4, static_cast<f32>(x));
        if (x > hi) store<f32>(o + 12, static_cast<f32>(x));
        ret(ctx, 1u);
        return;
    }
    if (hi > 0.0 && x > hi) store<u8>(o + 16, 1);
    ret(ctx, 0u);
}

// T::T(o, u32 kind, a2, a3, a4, a5, u8 base, a7, a9, a10) (args 9 and 10
// on the stack): the base Obj_InitVt_B20CC8(o, base) (it reads r3 / r4
// only), vtable 0x00B211F0, +16 = a4, +20 = a5,
// +24 = a10, +28 = 0 and the word +12: bits 0x10 (type 1), 0x8 (a2),
// 0x4 (a3), 0x2 (a7), 0x1 (a9); the type (bits 4..31) is then 2 for kind
// 0x3A2B1B5E, 1 for 0x055F2BDD and 0 for 0x80FE3297. r3 as left.
DECOMP(00437F24, Obj_Ctor_B211F0_00437F24) {
    const u32 o = arg<u32>(ctx, 0), kind = arg<u32>(ctx, 1), a2 = arg<u32>(ctx, 2), a3 = arg<u32>(ctx, 3);
    const u32 a4 = arg<u32>(ctx, 4), a5 = arg<u32>(ctx, 5), base = arg<u32>(ctx, 6), a7 = arg<u32>(ctx, 7);
    const u32 sp = static_cast<u32>(ctx.gpr[1]);
    const u32 a9 = load<u32>(sp + 116), a10 = load<u32>(sp + 124);
    call(ctx, 0x0042C9D0u, {o, base & 0xFFu});
    u32 w = load<u32>(o + 12);
    w = (w & 0xFu) | 0x10u;
    w = (w & ~0x8u) | ((a2 << 3) & 0x8u);
    w = (w & ~0x4u) | ((a3 << 2) & 0x4u);
    store<u32>(o, 0x00B211F0u);
    w = (w & ~0x2u) | ((a7 << 1) & 0x2u);
    store<u32>(o + 16, a4);
    store<u32>(o + 20, a5);
    store<u32>(o + 24, a10);
    store<u32>(o + 28, 0);
    w = (w & ~1u) | (a9 & 1u);
    store<u32>(o + 12, w);
    ctx.gpr[3] = kind;
    if (kind == 0x3A2B1B5Eu) {
        store<u32>(o + 12, (w & 0xFu) | 0x20u);
        ctx.gpr[3] = 2;
    } else if (kind == 0x055F2BDDu) {
        store<u32>(o + 12, (w & 0xFu) | 0x10u);
        ctx.gpr[3] = 1;
    } else if (kind == 0x80FE3297u) {
        store<u32>(o + 12, w & 0xFu);
        ctx.gpr[3] = w & 0xFu;
    }
}

// u32 f(T* o, Effect* x, a) (no direct caller): the component c =
// Effect_FindComponent(x, o->+16) with c->vt[12](c) != 0: its value
// c->vt[20](c) (f1) inside [o->+20, o->+24] compared with the u8 o->+48;
// when they differ every one of the u16 o->+36 handlers h (o->+28) gets
// h->vt[12](h, x, a) (OPD / entry after the arguments). r3 = 0, the flag
// or the count.
DECOMP(004C6E00, EffectCmd_RangeNotify_004C6E00) {
    const u32 o = arg<u32>(ctx, 0), x = arg<u32>(ctx, 1), a = arg<u32>(ctx, 2);
    const u32 c = static_cast<u32>(call(ctx, 0x004BB4B4u, {x, load<u32>(o + 16)}));
    if (c == 0) return;
    if (static_cast<u32>(VCall(ctx, c, 12, {c})) == 0) return;
    VCall(ctx, c, 20, {c});
    const f64 v = ctx.fpr[1];
    const u32 flag = load<u8>(o + 48);
    u32 in = 0;
    if (v >= Lfs40(o + 20) && v <= Lfs40(o + 24)) in = 1;
    ctx.gpr[3] = flag;
    if (in == flag) return;
    u32 i = 0;
    for (;;) {
        const u32 n = load<u16>(o + 36);
        ctx.gpr[3] = n;
        if (!(i < n)) break;
        const u32 h = load<u32>(load<u32>(o + 28) + 4 * i);
        VCall(ctx, h, 12, {h, x, a});
        ++i;
    }
}

// void* f(T* o) (no direct caller): deleting destructor: vtable
// 0x00B22F60; the entries of the list *0x00F668A0 (next +28) whose +4 is
// o get e->vt[4](e); o is unlinked from the list *0x00F6689C (next +52);
// the chain o->+48 (next +8) gets x->vt[4](x) one by one (o->+48 advanced
// after each); then Mem_FreeFrom(o, 0) (r3 = its; 0 for o == 0).
DECOMP(004E33D4, Obj_DeleteB22F60_004E33D4) {
    const u32 o = arg<u32>(ctx, 0);
    if (o == 0) {
        ret(ctx, 0u);
        return;
    }
    store<u32>(o, 0x00B22F60u);
    for (u32 e = load<u32>(0x00F668A0u); e != 0;) {
        const u32 nx = load<u32>(e + 28);
        if (load<u32>(e + 4) == o) VCall(ctx, e, 4, {e});
        e = nx;
    }
    u32 prev = 0;
    for (u32 q = load<u32>(0x00F6689Cu); q != 0; prev = q, q = load<u32>(q + 52)) {
        if (q != o) continue;
        if (prev == 0)
            store<u32>(0x00F6689Cu, load<u32>(o + 52));
        else
            store<u32>(prev + 52, load<u32>(o + 52));
        break;
    }
    for (u32 x = load<u32>(o + 48); x != 0;) {
        const u32 nx = load<u32>(x + 8);
        VCall(ctx, x, 4, {x});
        x = nx;
        store<u32>(o + 48, nx);
    }
    call(ctx, 0x0054EE6Cu, {o, 0});
}

// u16 f(SortedArr* a, const u32* key, u32 val): {key, val} goes into the
// 8-byte entry array (u16 count +2, u16 capacity +0, data +8): grown to
// twice the capacity when full (Array_Reserve), first sorted (qsort with
// the comparator +20) when the u16 flags +6 bit 0 is set (cleared), then
// inserted after the last entry whose key is not above *key (unsigned);
// the count goes up and the high-water mark +4 follows it. r3 = the old
// high-water mark.
DECOMP(00784490, SortedArr_Insert_00784490) {
    const u32 a = arg<u32>(ctx, 0), p = arg<u32>(ctx, 1), val = arg<u32>(ctx, 2);
    u32 n = load<u16>(a + 2);
    if (n == load<u16>(a)) {
        call(ctx, 0x00558F3Cu, {a, (static_cast<u32>(load<u16>(a)) << 1) & 0xFFFEu});
        n = load<u16>(a + 2);
    }
    const u32 f = load<u16>(a + 6);
    if (f & 1u) {
        store<u16>(a + 6, static_cast<u16>(f & 0xFFFEu));
        if (static_cast<s32>(n) > 0) {
            call(ctx, 0x0071EC28u, {load<u32>(a + 8), n, 8, load<u32>(a + 20)});
            n = load<u16>(a + 2);
        }
    }
    const u32 d = load<u32>(a + 8);
    s32 i = static_cast<s32>(n);
    while (i > 0) {
        const u32 e = d + 8 * static_cast<u32>(i);
        const u32 k = load<u32>(e - 8);
        if (!(load<u32>(p) < k)) break;
        store<u32>(e, k);
        store<u32>(e + 4, load<u32>(e - 4));
        --i;
    }
    store<u32>(d + 8 * static_cast<u32>(i), load<u32>(p));
    store<u32>(d + 8 * static_cast<u32>(i) + 4, val);
    const u32 n1 = load<u16>(a + 2) + 1;
    const u32 hw = load<u16>(a + 4);
    store<u16>(a + 2, static_cast<u16>(n1));
    store<u16>(a + 4, static_cast<u16>(static_cast<s32>(hw) > static_cast<s32>(n1 & 0xFFFFu) ? hw : n1));
    ctx.gpr[3] = hw;
}

// ---------------------------------------------------------------- sweep: 84 instructions
// T* f(?, XmlCursor* x) (no direct caller): a 44-byte anim command
// (Mem_Alloc(44, 0, 16); r3 = 0 on failure): AnimCmd_ConstructBase(p),
// vtable 0x00B0FFB8, the pointer vector p + 12 (PtrVector_Construct, u16
// +18 = 76, PtrVector_Init(v, *0x00F65C7C, 0, 1)), +32 / +36 / +40 = 0
// and the hashes 0x9178C4CB / 0xB4FE4E5C / 0xB4FE29BA read into them
// (default 0); then XmlQuery_ReadAll_0049EF40(x, v). r3 = p.
DECOMP(0008496C, AnimCmd_New44_0008496C) {
    const u32 x = arg<u32>(ctx, 1);
    const u32 p = static_cast<u32>(call(ctx, 0x0054ED38u, {44, 0, 16}));
    if (p == 0) {
        ret(ctx, 0u);
        return;
    }
    call(ctx, 0x004B9CB8u, {p});
    store<u32>(p, 0x00B0FFB8u);
    const u32 pool = load<u32>(0x00F65C7Cu);
    const u32 v = p + 12;
    call(ctx, 0x005516E0u, {v});
    store<u16>(p + 18, 76);
    call(ctx, 0x00551598u, {v, pool, 0, 1});
    store<u32>(p + 32, 0);
    store<u32>(p + 36, 0);
    store<u32>(p + 40, 0);
    call(ctx, 0x004FA14Cu, {x, 0x9178C4CBu, p + 32, 0});
    call(ctx, 0x004FA14Cu, {x, 0xB4FE4E5Cu, p + 36, 0});
    call(ctx, 0x004FA14Cu, {x, 0xB4FE29BAu, p + 40, 0});
    call(ctx, 0x0049EF40u, {x, v});
    ret(ctx, p);
}

// u32 f(T* o, Effect* x) (no direct caller): the target t is the
// component Effect_FindComponent(x, o->+16)'s +28 when c->vt[32](c) is 5
// and t's kind ((t->+4 >> 10) & 63) is 12, else
// *PointMap_Find((*0x00F8F154)->+16, &o->+16). With the player q =
// ((*0x00F65CA0) + 4208)->+8 whose (q + 192)->+8 has bit 0x1000: t set
// -> Obj_SetVecAB_00244900(q, t + 128, o->+20 squared, o->+24) and
// Field592_Reset_002448C0(q, u8 o->+12); t null and o->+16 set ->
// Field592_Reset_002448C0(q, 0). r3 as the original leaves it.
DECOMP(0008BAE0, EffectCmd_PlayerVec_0008BAE0) {
    const u32 o = arg<u32>(ctx, 0), x = arg<u32>(ctx, 1);
    const u32 c = static_cast<u32>(call(ctx, 0x004BB4B4u, {x, load<u32>(o + 16)}));
    u32 t = 0;
    bool found = false;
    if (c != 0 && static_cast<u32>(VCall(ctx, c, 32, {c})) == 5) {
        t = load<u32>(c + 28);
        found = t != 0 && ((load<u32>(t + 4) >> 10) & 63u) == 12;
    }
    if (!found) {
        Stack fr(ctx, 16);  // the key at sp+112
        store<u32>(fr.addr(), load<u32>(o + 16));
        const u32 r = static_cast<u32>(call(ctx, 0x0073343Cu, {load<u32>(load<u32>(0x00F8F154u) + 16), fr.addr()}));
        t = load<u32>(r);
    }
    ctx.gpr[3] = t;
    const u32 q = load<u32>(load<u32>(0x00F65CA0u) + 4208 + 8);
    if (q == 0) return;
    if ((load<u32>(q + 192 + 8) & 0x1000u) == 0) return;
    if (t != 0) {
        const f64 s = Lfs40(o + 20);
        callf(ctx, 0x00244900u, {q, t + 128}, {F40(s * s), Lfs40(o + 24)});
        call(ctx, 0x002448C0u, {q, load<u8>(o + 12)});
        return;
    }
    const u32 k = load<u32>(o + 16);
    ctx.gpr[3] = k;
    if (k == 0) return;
    call(ctx, 0x002448C0u, {q, 0});
}

// T* f(?, XmlCursor* x) (no direct caller): a 32-byte node (Mem_Alloc(32,
// 0, 16); r3 = 0 on failure): Obj_InitVt_B21C98(p, x), +12..+28 = 0,
// vtable 0x00B10AA8; the hash 0x8A93973F -> +12 (default 0); when that
// finds nothing: hash 0x0481FA93 -> +16, f32 0x8FA16092 -> +20, f32
// 0xB7EA3D94 -> +24, s32 0x002431A3 -> +28 (defaults the current values).
// r3 = p.
DECOMP(00092D94, XmlNode32_New_00092D94) {
    const u32 x = arg<u32>(ctx, 1);
    const u32 p = static_cast<u32>(call(ctx, 0x0054ED38u, {32, 0, 16}));
    if (p == 0) {
        ret(ctx, 0u);
        return;
    }
    call(ctx, 0x004BB3B0u, {p, x});
    store<u32>(p + 12, 0);
    store<u32>(p + 16, 0);
    store<u32>(p + 20, 0);
    store<u32>(p + 24, 0);
    store<u32>(p, 0x00B10AA8u);
    store<u32>(p + 28, 0);
    if (static_cast<u32>(call(ctx, 0x004FA14Cu, {x, 0x8A93973Fu, p + 12, 0})) == 0) {
        call(ctx, 0x004FA14Cu, {x, 0x0481FA93u, p + 16, load<u32>(p + 16)});
        callf(ctx, 0x004F9B20u, {x, 0x8FA16092u, p + 20}, {Lfs40(p + 20)});
        callf(ctx, 0x004F9B20u, {x, 0xB7EA3D94u, p + 24}, {Lfs40(p + 24)});
        call(ctx, 0x004F99BCu, {x, 0x002431A3u, p + 28, Sx40(load<u32>(p + 28))});
    }
    ret(ctx, p);
}

// T::T(o, XmlCursor* x): Obj_InitVt_B21C98(o, x), vtable 0x00B11AD0, the
// pointer vector o + 12 (PtrVector_Construct, u16 +18 = 4); every child c
// of x (XmlCursor_FirstChildAny / NextSiblingAny) with u8 c->+24 bit 0x80
// and c->+20 == 0x0001403F whose hash 0x0024728B (default 0) is set
// appends it (grown to twice the capacity u16 +16 when full,
// PtrVector_Reserve; u16 count +20, high-water u16 +22). r3 = 0.
DECOMP(000A7244, XmlHashList_Ctor_000A7244) {
    const u32 o = arg<u32>(ctx, 0), x = arg<u32>(ctx, 1);
    call(ctx, 0x004BB3B0u, {o, x});
    const u32 v = o + 12;
    store<u32>(o, 0x00B11AD0u);
    call(ctx, 0x005516E0u, {v});
    store<u16>(o + 18, 4);
    for (u32 c = static_cast<u32>(call(ctx, 0x004F9140u, {x})); c != 0;
         c = static_cast<u32>(call(ctx, 0x004F8D74u, {c}))) {
        if ((load<u8>(c + 24) & 0x80u) == 0 || load<u32>(c + 20) != 0x0001403Fu) continue;
        Stack fr(ctx, 16);  // the hash at sp+112
        store<u32>(fr.addr(), 0);
        call(ctx, 0x004FA14Cu, {c, 0x0024728Bu, fr.addr(), 0});
        if (load<u32>(fr.addr()) == 0) continue;
        u32 h = load<u32>(fr.addr());
        u32 n = load<u16>(o + 20);
        const u32 cap = load<u16>(o + 16);
        if (n == cap) {
            call(ctx, 0x00551718u, {v, (cap << 1) & 0x1FFFEu});
            h = load<u32>(fr.addr());
            n = load<u16>(o + 20);
        }
        const u32 d = load<u32>(o + 12);
        const u32 n7 = load<u16>(o + 20);
        const u32 hw = load<u16>(o + 22);
        store<u32>(d + ((n << 2) & 0x3FFFCu), h);
        const u32 n1 = n7 + 1;
        store<u16>(o + 20, static_cast<u16>(n1));
        store<u16>(o + 22, static_cast<u16>(static_cast<s32>(hw) > static_cast<s32>(n1 & 0xFFFFu) ? hw : n1));
    }
    ret(ctx, 0u);
}

// s32 f(Out* a, ?, Desc* d) (no direct caller): by the state s =
// (*0x00F64584)->+7344: 0 fills d (+0 = 1, +44 = *0x0101278C, +40 / +36
// = *0x01012790, +8 = 2, +4 = 0) and a->+0 = 0; 1 fills d (+0 = 1, +4 /
// +8 = 0, the 16 bytes of 0x00AC5DFA at +12, +32 = 0, +28 = 0x00F64588,
// +36 / +40 = (*0x00F64584)->+7324, +44 = 0x010127A0) and a->+0 = 0; any
// other a->+0 = 1. Then the state counts up and a->+4 = r3 = (s32)(100 *
// state) / 3.
DECOMP(000B1CF4, State_Step_000B1CF4) {
    const u32 a = arg<u32>(ctx, 0), d = arg<u32>(ctx, 2);
    const s32 s = load<s32>(load<u32>(0x00F64584u) + 7344);
    if (s == 0) {
        store<u32>(d, 1);
        const u32 w0 = load<u32>(0x0101278Cu), w1 = load<u32>(0x01012790u);
        store<u32>(d + 44, w0);
        store<u32>(d + 40, w1);
        store<u32>(d + 36, w1);
        store<u32>(d + 8, 2);
        store<u32>(d + 4, 0);
        store<u32>(a, 0);
    } else if (s == 1) {
        store<u32>(d, 1);
        store<u32>(d + 4, 0);
        store<u32>(d + 8, 0);
        call(ctx, 0x0071C3F0u, {d + 12, 0x00AC5DFAu, 16});
        store<u32>(d + 32, 0);
        const u32 g = load<u32>(0x00F64584u);
        store<u32>(d + 28, 0x00F64588u);
        store<u32>(d + 36, load<u32>(g + 7324));
        const u32 w = load<u32>(g + 7324);
        store<u32>(d + 44, 0x010127A0u);
        store<u32>(d + 40, w);
        store<u32>(a, 0);
    } else {
        store<u32>(a, 1);
    }
    const u32 g = load<u32>(0x00F64584u);
    store<u32>(g + 7344, load<u32>(g + 7344) + 1);
    const s32 q = static_cast<s32>(load<u32>(g + 7344) * 100u) / 3;
    store<u32>(a + 4, static_cast<u32>(q));
    ret(ctx, q);
}

// void* f(T* o) (no direct caller): deleting destructor: vtable
// 0x00B11E00; the effect e = o->+4->vt[272](o->+4) and its component c =
// Effect_FindComponent(e, 0xB0F92970) with c->vt[32](c) == 5 get their
// target released (c->+28->vt[8](c->+28) when set; c->+28 = c->+36 = 0);
// o->+212->vt[40](o->+212, 0) when set; vtable 0x00B11DC4 and SmallFree(o,
// 240) (r3 = its; 0 for o == 0).
DECOMP(000B4A34, Obj_Delete240_000B4A34) {
    const u32 o = arg<u32>(ctx, 0);
    if (o == 0) {
        ret(ctx, 0u);
        return;
    }
    const u32 x = load<u32>(o + 4);
    store<u32>(o, 0x00B11E00u);
    const u32 e = static_cast<u32>(VCall(ctx, x, 272, {x}));
    if (e != 0) {
        const u32 c = static_cast<u32>(call(ctx, 0x004BB4B4u, {e, 0xB0F92970u}));
        if (c != 0 && static_cast<u32>(VCall(ctx, c, 32, {c})) == 5) {
            const u32 t = load<u32>(c + 28);
            if (t != 0) VCall(ctx, t, 8, {t});
            store<u32>(c + 28, 0);
            store<u32>(c + 36, 0);
        }
    }
    const u32 m = load<u32>(o + 212);
    if (m != 0) VCall(ctx, m, 40, {m, 0});
    store<u32>(o, 0x00B11DC4u);
    call(ctx, 0x00552420u, {o, 240});
}

// void f(T* o) (no direct caller): the parts o->vt[68](o) and
// o->vt[64](o) get p->vt[44](p, 0) when set; o's world peer vector flag
// 0x20 is set (PeerVecFlag36); the effect o->+352 (when set) gets
// Effect_SetHash4CF988(e, 0xE69B8987) and bit 0x100 of its +136 cleared.
// r3 = o->+352.
DECOMP(000DF500, Obj_Activate_000DF500) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 a = static_cast<u32>(VCall(ctx, o, 68, {o}));
    if (a != 0) VCall(ctx, a, 44, {a, 0});
    const u32 b = static_cast<u32>(VCall(ctx, o, 64, {o}));
    if (b != 0) VCall(ctx, b, 44, {b, 0});
    PeerVecFlag36(ctx, o, true);
    const u32 e = load<u32>(o + 352);
    ctx.gpr[3] = e;
    if (e == 0) return;
    call(ctx, 0x004CF988u, {e, 0xE69B8987u});
    const u32 e2 = load<u32>(o + 352);
    store<u32>(e2 + 136, load<u32>(e2 + 136) & ~0x100u);
    ctx.gpr[3] = e2;
}

// void f(const Vec3* d, Mat* m) (Cam_LookAt101A60 etc.): when |d|^2 >=
// 1e-5 the camera basis: forward n = d / |d| -> m + 32; side s = up x n
// normalised -> m + 0, up' = n x s -> m + 16 (the up vector 0x01164BE0,
// or 0x01164B70 when n is within 0.999 of it and |n.y| >= 0.999); m + 48
// is rewritten with itself. r3 / f1 as the original leaves them.
DECOMP(000FD040, Cam_Basis_000FD040) {
    const u32 d = arg<u32>(ctx, 0), m = arg<u32>(ctx, 1);
    const f64 y = Lfs40(d + 4), x = Lfs40(d), z = Lfs40(d + 8);
    const f64 c0 = Lfs40(0x000FD030u), c1 = Lfs40(0x000FD034u), c2 = Lfs40(0x000FD038u);  // 1e-5, 1.0, 0.999
    f64 ux = Lfs40(0x01164BE0u), uy = Lfs40(0x01164BE4u), uz = Lfs40(0x01164BE8u);
    f64 l = F40(y * y);
    l = F40(std::fma(x, x, l));
    l = F40(std::fma(z, z, l));
    if (!(l >= c0)) {
        ctx.fpr[1] = ux;
        ctx.gpr[3] = 0x01164BD0u;  // as left
        return;
    }
    const f64 r = F40(c1 / F40(std::sqrt(l)));
    const f64 nx = F40(x * r), ny = F40(y * r), nz = F40(z * r);
    if (std::fabs(ny) >= c2) {
        f64 t = F40(ny * uy);
        t = F40(std::fma(nx, ux, t));
        t = F40(std::fma(nz, uz, t));
        if (std::fabs(t) > c2) {
            ux = Lfs40(0x01164B70u);
            uy = Lfs40(0x01164B74u);
            uz = Lfs40(0x01164B78u);
        }
    }
    const f64 t7 = F40(ux * nz), t9 = F40(uz * ny), t10 = F40(uy * nx);
    f64 a3 = F40(std::fma(uz, nx, -t7));
    f64 a2 = F40(std::fma(uy, nz, -t9));
    f64 a1 = F40(std::fma(ux, ny, -t10));
    f64 s = F40(a3 * a3);
    const u32 w0 = load<u32>(m + 48), w1 = load<u32>(m + 52), w2 = load<u32>(m + 56);
    store<f32>(m + 32, static_cast<f32>(nx));
    s = F40(std::fma(a2, a2, s));
    store<f32>(m + 36, static_cast<f32>(ny));
    store<f32>(m + 40, static_cast<f32>(nz));
    store<u32>(m + 48, w0);
    store<u32>(m + 52, w1);
    store<u32>(m + 56, w2);
    s = F40(std::fma(a1, a1, s));
    s = F40(std::sqrt(s));
    s = F40(c1 / s);
    a2 = F40(a2 * s);
    a3 = F40(a3 * s);
    a1 = F40(a1 * s);
    const f64 p7 = F40(ny * a2), p8 = F40(nz * a3), p9 = F40(nx * a1);
    store<f32>(m, static_cast<f32>(a2));
    store<f32>(m + 4, static_cast<f32>(a3));
    store<f32>(m + 8, static_cast<f32>(a1));
    const f64 b3 = F40(std::fma(nx, a3, -p7));
    const f64 b1 = F40(std::fma(ny, a1, -p8));
    const f64 b2 = F40(std::fma(nz, a2, -p9));
    store<f32>(m + 24, static_cast<f32>(b3));
    store<f32>(m + 16, static_cast<f32>(b1));
    store<f32>(m + 20, static_cast<f32>(b2));
    ctx.fpr[1] = b1;
    ctx.gpr[3] = m + 48;  // as left
}

// void f(T* o, const u8 col[4], f32 t, u8 now) (no direct caller): the
// colour goes to o + 548 and t to f32 o->+556; with `now` also to o +
// 544 / f32 o->+552 and Player_Blend11FFF0(o, o + 544, 1; 0.0) (r6 =
// 0x00120330 as left); then every one of the u16 (o->+340)->+8 240-byte
// entries' +192 object x gets x->vt[120](x, col, OPD, now, entry; t) (the
// OPD in r5, the entry in r7, as the original leaves them). r3 = o->+340.
DECOMP(00120338, Obj_SetColour_00120338) {
    const u32 o = arg<u32>(ctx, 0), p = arg<u32>(ctx, 1), now = arg<u32>(ctx, 3) & 0xFFu;
    const f64 t = farg(ctx, 0);
    for (u32 k = 0; k < 4; ++k) store<u8>(o + 548 + k, load<u8>(p + k));
    store<f32>(o + 556, static_cast<f32>(t));
    if (now != 0) {
        for (u32 k = 0; k < 4; ++k) store<u8>(o + 544 + k, load<u8>(p + k));
        store<f32>(o + 552, static_cast<f32>(t));
        callf(ctx, 0x0011FFF0u, {o, o + 544, 1, 0x00120330u}, {Lfs40(0x00120330u)});
    }
    for (u32 i = 0;; ++i) {
        const u32 a = load<u32>(o + 340);
        ctx.gpr[3] = a;
        const u32 n = a != 0 ? load<u16>(a + 8) : 0u;
        if (!(i < n)) break;
        const u32 x = load<u32>(load<u32>(a) + 240 * i + 192);
        const u32 opd = load<u32>(load<u32>(x) + 120);
        ctx.fpr[1] = t;
        call_ptr(ctx, opd, {x, p, opd, now, load<u32>(opd)});
    }
}

// u32 f(T* o) (f_001E2088): with bit 0x80 of u8 o->+744 set and the
// target definition's (Target_GetDef0D951C(o)) +64 owner w set: the player
// q = (*0x00F65CA0 + (s8 w->+172) * 0x20C80 + 4208)->+8 (only when its
// +192 has bit 8), Obj_Release_002B719C(def), q->vt[28](q, 0, 2) and
// q->vt[28](q, 1, 1), o->vt[28](o, 0, 12); the bit is then cleared (also
// without an owner). r3 = 0 (the definition when the bit is clear).
DECOMP(001E1F20, Target_Release_001E1F20) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 d = static_cast<u32>(call(ctx, 0x000D951Cu, {o}));
    const u32 w = load<u32>(d + 64);
    if ((load<u8>(o + 744) & 0x80u) == 0) return;
    if (w != 0) {
        const s32 idx = static_cast<s8>(load<u8>(w + 172));
        const u32 q0 = load<u32>(load<u32>(0x00F65CA0u) + static_cast<u32>(idx) * 0x20C80u + 4208 + 8);
        u32 q = 0;
        if (q0 != 0 && (load<u32>(q0 + 192) & 8u) != 0) q = q0;
        call(ctx, 0x002B719Cu, {d});
        if (q != 0) {
            VCall(ctx, q, 28, {q, 0, 2});
            VCall(ctx, q, 28, {q, 1, 1});
        }
        VCall(ctx, o, 28, {o, 0, 12});
    }
    store<u8>(o + 744, static_cast<u8>(load<u8>(o + 744) & ~0x80u));
    ret(ctx, 0u);
}

// bool f(T* o, f32 dt) (no direct caller): 0 when Obj_UpdateTeamBase(o;
// dt) fails, 1 otherwise; with bit 0x80000000 of o->+412: each entry e of the
// u16-counted list o + 344 (i < 32 has the bit 1 << i) gets
// sub_0021B0E0(o, e, o->+452; o->+456) when its bit is set in o->+448
// (cleared when that returns nonzero), then sub_0021B0E0(o, e, o->+452;
// o->+460 * dt) when that product is above 0.0.
DECOMP(0021B5E0, Team_UpdateEntries_0021B5E0) {
    const u32 o = arg<u32>(ctx, 0);
    const f64 dt = farg(ctx, 0);
    callf(ctx, 0x0020DFD8u, {o}, {dt});  // it reads f1 too
    if (static_cast<u32>(ctx.gpr[3]) == 0) {
        ret(ctx, 0u);
        return;
    }
    if ((load<u32>(o + 412) & 0x80000000u) == 0) {
        ret(ctx, 1u);
        return;
    }
    const f64 s = F40(Lfs40(o + 460) * dt);
    const f64 c = Lfs40(0x0021B5DCu);  // 0.0
    const u32 v = o + 344;
    for (u32 i = 0; i < load<u16>(v + 8); ++i) {
        const u32 e = load<u32>(load<u32>(v) + 4 * i);
        const u32 bit = (i & 0x3Fu) >= 32 ? 0u : (1u << (i & 31u));
        if ((load<u32>(o + 448) & bit) != 0) {
            callf(ctx, 0x0021B0E0u, {o, e, load<u32>(o + 452)}, {Lfs40(o + 456)});
            if (static_cast<u32>(ctx.gpr[3]) != 0) store<u32>(o + 448, load<u32>(o + 448) & ~bit);
        }
        if (s > c) callf(ctx, 0x0021B0E0u, {o, e, load<u32>(o + 452)}, {s});
    }
    ret(ctx, 1u);
}

// T** f(PtrSorted* o, T* x) (TmplRead_002666C0): x goes into the pointer
// vector o + 20 (data +20, u16 capacity +24, count +28, high-water +30),
// before the first entry e with x->+52 < e->+52 (unsigned), else at the
// end: appended (grown to twice the capacity when full,
// PtrVector_Reserve), the later entries moved up one and x stored at its
// place. r3 = &data[pos], or the old high-water mark when appended.
DECOMP(00266570, PtrSorted_Insert_00266570) {
    const u32 o = arg<u32>(ctx, 0), x = arg<u32>(ctx, 1);
    s32 pos = -1;
    u32 d = load<u32>(o + 20);
    u32 n = load<u16>(o + 28);
    const u32 cap = load<u16>(o + 24);
    u32 i = 0;
    if (n != 0) {
        for (; i < n; ++i)
            if (load<u32>(x + 52) < load<u32>(load<u32>(d + 4 * i) + 52)) {
                pos = static_cast<s32>(i);
                break;
            }
    }
    if (n == cap) {
        call(ctx, 0x00551718u, {o + 20, (cap << 1) & 0x1FFFEu});
        n = load<u16>(o + 28);
        d = load<u32>(o + 20);
    }
    store<u32>(d + ((n << 2) & 0x3FFFCu), x);
    const u32 n1 = load<u16>(o + 28) + 1;
    const u32 hw = load<u16>(o + 30);
    store<u16>(o + 28, static_cast<u16>(n1));
    store<u16>(o + 30, static_cast<u16>(static_cast<s32>(hw) > static_cast<s32>(n1 & 0xFFFFu) ? hw : n1));
    if (pos == -1) {
        ctx.gpr[3] = hw;
        return;
    }
    const u32 last = (n1 & 0xFFFFu) - 1;
    if (i < last) {
        s32 k = static_cast<s32>(last - i);
        u32 j = last;
        do {
            const u32 dd = load<u32>(o + 20);
            --k;
            store<u32>(dd + 4 * j, load<u32>(dd + 4 * (j - 1)));
            --j;
        } while (k > 0);
    }
    const u32 p = load<u32>(o + 20) + 4 * static_cast<u32>(pos);
    store<u32>(p, x);
    ctx.gpr[3] = p;
}

// void f(T* o, u8 a, u8 b) (no direct caller): for each of the u16 o->+36
// groups g (o->+28): g->+52->vt[116](g->+52, a, b, 1, 0) when set, then
// every set item q of the u16 g->+16 8-byte entries (g->+8) gets
// q->vt[116](q, a, b, 1, 0). r3 = the group count.
DECOMP(00267E90, Groups_Notify_00267E90) {
    const u32 o = arg<u32>(ctx, 0), a = arg<u32>(ctx, 1) & 0xFFu, b = arg<u32>(ctx, 2) & 0xFFu;
    for (u32 i = 0;; ++i) {
        const u32 n = load<u16>(o + 36);
        ctx.gpr[3] = n;
        if (!(i < n)) break;
        const u32 g = load<u32>(load<u32>(o + 28) + 4 * i);
        const u32 p = load<u32>(g + 52);
        if (p != 0) VCall(ctx, p, 116, {p, a, b, 1, 0});
        for (u32 j = 0; j < load<u16>(g + 16); ++j) {
            const u32 q = load<u32>(load<u32>(g + 8) + 8 * j);
            if (q != 0) VCall(ctx, q, 116, {q, a, b, 1, 0});
        }
    }
}

// void f(T* o, u32 k) (many callers): the 5-bit field (o->+24 >> 20) & 31
// = k; unless bit 0x80000 of o->+24 is set: when the u8 0x0105BC20 is set
// and o->+28 has bit (o->+24 >> 25) & 31 or bit k & 31, sub_000D6770(o)
// and the bit 0x80000 set, else sub_00275110(o). Then
// Obj_SetFlag968_00124370(o->+8, 1) (when set and its +192 has bit 8)
// once for state ((o->+24 >> 25) & 31) 4 and once more for field values 2
// and 4. r3 as the original leaves it.
DECOMP(00276558, Obj_SetField20_00276558) {
    const u32 o = arg<u32>(ctx, 0), k = arg<u32>(ctx, 1);
    u32 w = (load<u32>(o + 24) & ~0x01F00000u) | ((k << 20) & 0x01F00000u);
    store<u32>(o + 24, w);
    if ((w & 0x00080000u) == 0) {
        bool hit = false;
        if (load<u8>(0x0105BBD8u + 72) != 0) {
            const u32 m = load<u32>(o + 28);
            hit = (m & (1u << ((w >> 25) & 31u))) != 0 || (m & (1u << (k & 31u))) != 0;
        }
        if (hit) {
            call(ctx, 0x000D6770u, {o});
            w = load<u32>(o + 24) | 0x00080000u;
            store<u32>(o + 24, w);
        } else {
            call(ctx, 0x00275110u, {o});
            w = load<u32>(o + 24);
        }
    }
    if (((w >> 25) & 31u) == 4) {
        const u32 t = load<u32>(o + 8);
        if (t != 0 && (load<u32>(t + 192) & 8u) != 0) {
            call(ctx, 0x00124370u, {t, 1});
            w = load<u32>(o + 24);
        }
    }
    const u32 q = (w >> 20) & 31u;
    ctx.gpr[3] = q;
    if (q != 2 && q != 4) return;
    const u32 t = load<u32>(o + 8);
    if (t == 0) return;
    const u32 f = load<u32>(t + 192) & 8u;
    ctx.gpr[3] = f;
    if (f == 0) return;
    call(ctx, 0x00124370u, {t, 1});
}

// Deleting destructors (no direct caller) of two 272-byte world objects:
// DeleteWorld272_40 with vtable 0x00B1C4C8, kind 0, list 0x00F35278 /
// vtable 0x00B1C730, kind 1, list 0x00F353E0.
DECOMP(002808B4, WorldObj_Delete272a_002808B4) { DeleteWorld272_40(ctx, 0x00B1C4C8u, 0, 0x00F35278u); }
DECOMP(0028F3E4, WorldObj_Delete272b_0028F3E4) { DeleteWorld272_40(ctx, 0x00B1C730u, 1, 0x00F353E0u); }

// s32 f(T* o) (no direct caller): with p = o->+32: sub_00308F5C(p->+872,
// 0); each set slot x of p + 776 (3 words) gets sub_001F4460(x) and
// x->vt[88](x, 1, 0, 0, 0) (the slot read again);
// Field_SetByteClear4_004A090C(p + 640, 0); each set slot of (o->+32) +
// 776 whose Target_GetDef0D951C(x) is set gets Obj_SetStateFx266E6C(def,
// 0); o->+64 = -1. r3 = -1.
DECOMP(0031771C, Obj_ResetSlots_0031771C) {
    const u32 o = arg<u32>(ctx, 0);
    call(ctx, 0x00308F5Cu, {load<u32>(load<u32>(o + 32) + 872), 0});
    const u32 p = load<u32>(o + 32);
    for (u32 i = 0; i < 3; ++i) {
        const u32 slot = p + 776 + 4 * i;
        const u32 x = load<u32>(slot);
        if (x == 0) continue;
        call(ctx, 0x001F4460u, {x});
        const u32 y = load<u32>(slot);
        VCall(ctx, y, 88, {y, 1, 0, 0, 0});
    }
    call(ctx, 0x004A090Cu, {p + 640, 0});
    const u32 p2 = load<u32>(o + 32);
    for (u32 j = 0; j < 3; ++j) {
        const u32 x = load<u32>(p2 + 776 + 4 * j);
        if (x == 0) continue;
        const u32 d = static_cast<u32>(call(ctx, 0x000D951Cu, {x}));
        if (d != 0) call(ctx, 0x00266E6Cu, {d, 0});
    }
    store<u32>(o + 64, 0xFFFFFFFFu);
    ret(ctx, -1);
}

// Entry* f(T* o, const Vec3* p) (f_003B5720): the nearest (in x/z) of the
// u16 o->+64 64-byte entries e (o->+56) with an object x = e->+32, no
// e->+48 and no x->+244 of kind 8 (its +4): its point
// Obj_OffsetPoint_003B5170(o, x, &s) at squared distance d < the best so
// far (from FLT_MAX) and d <= (o->+96)->+256. r3 = it or 0 (f1 the last
// d).
DECOMP(003B55B8, Spots_FindNearest_003B55B8) {
    const u32 o = arg<u32>(ctx, 0), p = arg<u32>(ctx, 1);
    if (load<u16>(o + 64) == 0) {
        ret(ctx, 0u);
        return;
    }
    const f64 c1 = Lfs40(0x003B55B4u);  // 0.0
    const f64 base = F40(c1 * c1);
    f64 best = Lfs40(0x003B55B0u);  // FLT_MAX
    u32 r = 0;
    Stack fr(ctx, 16);  // the point at sp+112
    for (s32 i = 0; i < static_cast<s32>(load<u16>(o + 64)); ++i) {
        const u32 e = load<u32>(o + 56) + 64 * static_cast<u32>(i);
        const u32 x = load<u32>(e + 32);
        if (x == 0 || load<u32>(e + 48) != 0) continue;
        const u32 y = load<u32>(x + 244);
        if (y != 0 && load<u32>(y + 4) == 8) continue;
        call(ctx, 0x003B5170u, {o, x, fr.addr()});
        const f64 dx = F40(Lfs40(fr.addr()) - Lfs40(p));
        const f64 dz = F40(Lfs40(fr.addr() + 8) - Lfs40(p + 8));
        f64 d = F40(std::fma(dx, dx, base));
        d = F40(std::fma(dz, dz, d));
        ctx.fpr[1] = d;  // left in f1
        if (best > d && d <= Lfs40(load<u32>(o + 96) + 256)) {
            best = d;
            r = e;
        }
    }
    ret(ctx, r);
}

// void f() (no direct caller): with the object w = *(*0x00F65CA0 +
// 0x001281E0) and Global_Test10C5A20_0046C7B0() set: the transform
// descriptor on the stack (the 4 x 3 floats of 0x01164BD0 (rows 16 bytes
// apart), 55.0, 7.5, zeros, the 4 bytes of 0x01164CA8) goes to
// Obj_GetXform_000FA5D0(w, &desc), then with +64 = *0x01088C48 to
// Obj_ApplyDesc_000FB31C(w, &desc, 0). r3 as the original leaves it.
DECOMP(003B8980, Player_ResetXform_003B8980) {
    const u32 g = load<u32>(0x00F65CA0u);
    const u32 w = load<u32>(g + 0x001281E0u);
    ctx.gpr[3] = g;
    if (w == 0) return;
    if (static_cast<u32>(call(ctx, 0x0046C7B0u, {g, 0x00120000u})) == 0) return;
    Stack fr(ctx, 112);  // the descriptor at sp+112 .. sp+211
    const u32 s = fr.addr();
    for (u32 row = 0; row < 4; ++row)
        for (u32 k = 0; k < 3; ++k) store<u32>(s + 16 * row + 4 * k, load<u32>(0x01164BD0u + 16 * row + 4 * k));
    store<u32>(s + 64, 0x425C0000u);  // 55.0f
    store<u32>(s + 68, 0x40F00000u);  // 7.5f
    store<u8>(s + 72, 0);
    store<u32>(s + 76, 0);
    store<u32>(s + 80, 0);
    store<u32>(s + 84, 0);
    store<u8>(s + 88, 0);
    store<u32>(s + 92, 0);
    for (u32 k = 0; k < 4; ++k) store<u8>(s + 96 + k, load<u8>(0x01164CA8u + k));
    call(ctx, 0x000FA5D0u, {w, s});
    store<u32>(s + 64, load<u32>(0x01088C00u + 72));
    ctx.fpr[1] = Lfs40(0x01088C00u + 72);  // left in f1
    call(ctx, 0x000FB31Cu, {w, s, 0});
}

// f32 f(u32 h) (f_003D75A8): the longest time of the voice in slot h >> 16
// of 0x010B0200 (s32 count +24, 272-byte entries +28) when the u8
// 0x010AEC60 is set, the id h & 0xFFFF is nonzero and matches the entry's
// (+256 >> 15) & 0xFFFF and the entry's +256 has bit 0x80000000 clear: the
// largest Sub_GetTime6F9A18(Voice_GetItem_006D6514(v, i), &t, 0) over the
// Voice_GetValue(v) items of v = entry->+16, from 0.0. r3 as left.
DECOMP(00444108, Voice_MaxTime_00444108) {
    const u32 h = arg<u32>(ctx, 0);
    const u32 id = h & 0xFFFFu;
    ctx.gpr[3] = 0x00444100u;
    bool ok = load<u8>(0x010AEC60u) != 0 && id != 0;
    u32 data = 0, idx = 0;
    if (ok) {
        idx = h >> 16;
        ok = static_cast<s32>(idx) < load<s32>(0x010B0200u + 24);
        if (ok) {
            data = load<u32>(0x010B0200u + 28);
            const u32 e = data + idx * 272 + 256;
            ok = ((load<u32>(e) >> 15) & 0xFFFFu) == id && (load<u8>(e) & 0x80u) == 0;
        }
    }
    if (!ok) {
        ctx.fpr[1] = Lfs40(0x00444100u);  // 0.0
        return;
    }
    const u32 vp = data + idx * 272 + 16;
    const u32 n = static_cast<u32>(call(ctx, 0x006D64ACu, {load<u32>(vp)}));
    f64 best = Lfs40(0x00444100u);
    ctx.gpr[3] = 0x00444100u;
    if (n != 0) {
        Stack fr(ctx, 16);  // the time at sp+112
        for (u32 i = 0; i < n; ++i) {
            const u32 it = static_cast<u32>(call(ctx, 0x006D6514u, {load<u32>(vp), Sx40(static_cast<u32>(static_cast<s32>(static_cast<s16>(i))))}));
            call(ctx, 0x006F9A18u, {it, fr.addr(), 0});
            const f64 t = Lfs40(fr.addr());
            if (t > best) best = t;
        }
    }
    ctx.fpr[1] = best;
}

// void f(T* o) (no direct caller): destructor (RegistryUnlink40).
DECOMP(0048F3F8, Registry_Unlink_0048F3F8) { RegistryUnlink40(ctx); }
// The second copy (no direct caller).
DECOMP(0048F6B0, Registry_Unlink2_0048F6B0) { RegistryUnlink40(ctx); }

// void f(Vec2* out, T* o) (no direct caller): out = o->+140 / +144; when
// o->+136 has bit 0x02000000 the size is recomputed (bit cleared, stored
// to out and o->+140 / +144): with bit 0x20000000 from o->+148's
// s->vt[140](&v, s) or else the u16 screen size at 0x010C5A24 / 0x010C5A26;
// without it o->vt[168](&v, o) scaled by o->+168 / o->+172. r3 = 0 (o->+136
// when nothing changes); f1 = out->y.
DECOMP(004E687C, Widget_GetSize_004E687C) {
    const u32 out = arg<u32>(ctx, 0), o = arg<u32>(ctx, 1);
    store<u32>(out, load<u32>(o + 140));
    const u32 y0 = load<u32>(o + 144);
    store<u32>(out + 4, y0);
    const u32 w = load<u32>(o + 136);
    ctx.fpr[1] = static_cast<f64>(std::bit_cast<f32>(y0));
    ctx.gpr[3] = w;
    if ((w & 0x02000000u) == 0) return;
    Stack fr(ctx, 32);  // v at sp+112 (fr) and sp+128 (fr + 16)
    u32 x, y;
    if ((w & 0x20000000u) != 0) {
        const u32 s = load<u32>(o + 148);
        if (s != 0) {
            VCall(ctx, s, 140, {fr.addr() + 16, s});
            x = load<u32>(fr.addr() + 16);
            y = load<u32>(fr.addr() + 20);
        } else {
            x = std::bit_cast<u32>(static_cast<f32>(static_cast<f64>(load<u16>(0x010C5A20u + 4))));
            y = std::bit_cast<u32>(static_cast<f32>(static_cast<f64>(load<u16>(0x010C5A20u + 6))));
        }
    } else {
        VCall(ctx, o, 168, {fr.addr(), o});
        x = std::bit_cast<u32>(static_cast<f32>(Lfs40(o + 168) * Lfs40(fr.addr())));
        y = std::bit_cast<u32>(static_cast<f32>(Lfs40(o + 172) * Lfs40(fr.addr() + 4)));
    }
    store<u32>(out + 4, y);
    store<u32>(out, x);
    const u32 w2 = load<u32>(o + 136);
    store<u32>(o + 140, x);
    const u32 y2 = load<u32>(out + 4);
    store<u32>(o + 136, w2 & ~0x02000000u);
    store<u32>(o + 144, y2);
    ctx.fpr[1] = static_cast<f64>(std::bit_cast<f32>(y2));
    ret(ctx, 0u);
}

// Destructors (no direct caller) of two shape objects: DtorShapeArrays40
// with vtable 0x00B19950 / base sub_002108A8 and vtable 0x00B19CC8 / base
// sub_00213398 (r4 = the vtable as left).
DECOMP(0075A3AC, ShapeObj_Dtor_0075A3AC) { DtorShapeArrays40(ctx, 0x00B19950u, 0x002108A8u); }
DECOMP(0075AF24, ShapeObj_Dtor2_0075AF24) { DtorShapeArrays40(ctx, 0x00B19CC8u, 0x00213398u); }

// u32 f(Vec48* v, const Elem* e) (f_0033E7D8): e appended to the 48-byte
// element vector v (data +0, u16 capacity +4, count +8, high-water +10;
// grown to twice the capacity when full, PtrVector_Reserve): words +0..+8,
// +16..+32 and the top 9 bits of +36 (merged one at a time, as the
// original). r3 = the new element's index (f1 = e->+8 as left).
DECOMP(0076AD3C, Vec48_Push_0076AD3C) {
    const u32 v = arg<u32>(ctx, 0), e = arg<u32>(ctx, 1);
    u32 n = load<u16>(v + 8);
    const u32 cap = load<u16>(v + 4);
    if (n == cap) {
        call(ctx, 0x00551718u, {v, (cap << 1) & 0x1FFFEu});
        n = load<u16>(v + 8);
    }
    const u32 d = load<u32>(v) + 48 * n;
    for (u32 k = 0; k < 28; k += 4) {
        if (k == 12) continue;
        store<u32>(d + k, load<u32>(e + k));
    }
    ctx.fpr[1] = Lfs40(e + 8);  // left in f1
    const u32 s28 = load<u32>(e + 28);
    const u32 old36 = load<u32>(d + 36);
    store<u32>(d + 28, s28);
    u32 w = old36 & 0x7FFFFFFFu;
    store<u32>(d + 32, load<u32>(e + 32));
    w |= load<u32>(e + 36) & 0x80000000u;
    store<u32>(d + 36, w);
    for (u32 b = 1; b <= 8; ++b) {
        const u32 m = 0x80000000u >> b;
        w = (w & ~m) | (load<u32>(e + 36) & m);
        store<u32>(d + 36, w);
    }
    const u32 n1 = load<u16>(v + 8) + 1;
    const u32 hw = load<u16>(v + 10);
    store<u16>(v + 8, static_cast<u16>(n1));
    store<u16>(v + 10, static_cast<u16>(static_cast<s32>(hw) > static_cast<s32>(n1 & 0xFFFFu) ? hw : n1));
    ret(ctx, (n1 & 0xFFFFu) - 1);
}

// ---------------------------------------------------------------- sweep: 85 instructions
// u32 f(T* o) (no direct caller): by the state o->+4: 1 and 13 ->
// Obj_SetProfile_00254040(c, 0) for component 36 and c->vt[44](c, 1) for
// component 3 of o->+48 (Obj_FindComponent(o->+48, k, 0), when found); 2, 3,
// 5, 8, 9 clear bits 0x4 / 0x10 of u8 o->+304, 10 only 0x4; 4 clears both
// and the result q of o->+68->vt[8](o->+68) gets bit 0x80000000 of +168
// cleared. r3 as the original leaves it.
DECOMP(0002742C, State_Leave_0002742C) {
    const u32 o = arg<u32>(ctx, 0);
    const s32 s = load<s32>(o + 4);
    ctx.gpr[3] = static_cast<u32>(s);
    if (s == 1 || s == 13) {
        const u32 c = static_cast<u32>(call(ctx, 0x000D1288u, {load<u32>(o + 48), 36, 0}));
        if (c != 0) call(ctx, 0x00254040u, {c, 0});
        const u32 c2 = static_cast<u32>(call(ctx, 0x000D1288u, {load<u32>(o + 48), 3, 0}));
        if (c2 != 0) VCall(ctx, c2, 44, {c2, 1});
    } else if (s == 4) {
        const u8 b = static_cast<u8>(load<u8>(o + 304) & ~0x14u);
        const u32 x = load<u32>(o + 68);
        store<u8>(o + 304, b);
        const u32 q = static_cast<u32>(VCall(ctx, x, 8, {x}));
        store<u32>(q + 168, load<u32>(q + 168) & 0x7FFFFFFFu);
    } else if (s == 2 || s == 3 || s == 5 || s == 8 || s == 9) {
        store<u8>(o + 304, static_cast<u8>(load<u8>(o + 304) & ~0x14u));
        ctx.gpr[3] = 0;
    } else if (s == 10) {
        store<u8>(o + 304, static_cast<u8>(load<u8>(o + 304) & ~0x4u));
        ctx.gpr[3] = 0;
    }
}

// bool f(T* o, T* t, u32 key) (f_00039470): 1 when o->+436 is above the
// tuning value A (the table (o->+308)->+12 indexed by the ids in
// (0x00F94C4C / 0x00F94C64 / 0x00F94C7C)->+16->+64), the squared distance
// to t (o->+56 when t is o->+52, else between t + 176 and o + 176) lies in
// (B^2, C^2), Handle_HasKey_00298B44(o->+64, key) holds, key is
// 0x13BF9082 and the state o->+4 is 0, 12 or 13. f1 as left.
DECOMP(00039314, Ai_InRange_00039314) {
    const u32 o = arg<u32>(ctx, 0), t = arg<u32>(ctx, 1), key = arg<u32>(ctx, 2);
    const u32 tab = load<u32>(load<u32>(o + 308) + 12);
    auto idx = [](u32 g) { return load<u32>(load<u32>(g + 16) + 64) << 2; };
    f64 f1 = Lfs40(o + 436);
    const u32 h = load<u32>(o + 64);
    ctx.fpr[1] = f1;
    ret(ctx, 0u);
    if (!(f1 > Lfs40(tab + idx(0x00F94C4Cu)))) return;
    f64 d = Lfs40(o + 56);
    const u32 i1 = idx(0x00F94C64u), i2 = idx(0x00F94C7Cu);
    const u32 near = load<u32>(o + 52);
    const f64 a = Lfs40(tab + i1), b = Lfs40(tab + i2);
    const f64 lo = F40(a * a);
    f1 = F40(b * b);
    ctx.fpr[1] = f1;
    if (t != near) {
        const f64 dy = F40(Lfs40(t + 176 + 4) - Lfs40(o + 176 + 4));
        const f64 dx = F40(Lfs40(t + 176) - Lfs40(o + 176));
        const f64 dz = F40(Lfs40(t + 176 + 8) - Lfs40(o + 176 + 8));
        d = F40(dy * dy);
        d = F40(std::fma(dx, dx, d));
        d = F40(std::fma(dz, dz, d));
    }
    if (!(d < f1) || !(d > lo)) return;
    if (static_cast<u32>(call(ctx, 0x00298B44u, {h, key})) == 0) return;
    ret(ctx, 0u);
    if (key != 0x13BF9082u) return;
    const s32 s = load<s32>(o + 4);
    if (s == 12 || s == 0 || s == 13) ret(ctx, 1u);
}

// u32 f(T* o) (no direct caller): the parent a = Obj_FindById(o->+12)
// (when its +196 has bit 0x02000000) and the child b = Obj_FindById(o->+16)
// (when its +192 has bit 1) or, without that id, the player o->+20 (not 9;
// (*0x00F65CA0 + k * 0x20C80 + 4208)->+8, same bit test): with a found
// Obj_AddChild_001CC174(a, b) when the u8 o->+24 is set, else
// Obj_ResetList_001CC214(a, b). r3 as the original leaves it.
DECOMP(0007A58C, EffectCmd_Attach_0007A58C) {
    const u32 o = arg<u32>(ctx, 0);
    u32 a = 0, b = 0;
    const u32 id = load<u32>(o + 12);
    ctx.gpr[3] = id;
    if (id != 0) {
        const u32 x = static_cast<u32>(call(ctx, 0x000D6A28u, {id}));
        if (x != 0 && (load<u32>(x + 196) & 0x02000000u) != 0) a = x;
    }
    const u32 id2 = load<u32>(o + 16);
    if (id2 != 0) {
        const u32 y = static_cast<u32>(call(ctx, 0x000D6A28u, {id2}));
        if (y != 0 && (load<u32>(y + 192) & 1u) != 0) b = y;
    } else {
        const u32 k = load<u32>(o + 20);
        ctx.gpr[3] = k;
        if (k != 9) {
            const u32 q = load<u32>(load<u32>(0x00F65CA0u) + k * 0x20C80u + 4208 + 8);
            ctx.gpr[3] = q;
            if (q != 0 && (load<u32>(q + 192) & 1u) != 0) b = q;
        }
    }
    if (a == 0) return;
    if (load<u8>(o + 24) != 0)
        call(ctx, 0x001CC174u, {a, b});
    else
        call(ctx, 0x001CC214u, {a, b});
}

// u32 f(T* o, Effect* x) (no direct caller): on the id set *0x00F557F0:
// kind o->+20 0x03D3BE2D clears it (Obj_ClearField_0001B2E8); else the
// object Obj_FindById(id) (id = o->+12, or the component
// Effect_FindComponent(x, o->+16)'s c->vt[24](c) when c->vt[32](c) is 2
// and that is nonzero) is removed (kind 0x8FDDEC84, IntSet_Remove_0001B274)
// or pushed (Arr3700_Push_0001B238); a missing one goes to
// Hash_ToString(id). r3 = the last call's.
DECOMP(00084778, EffectCmd_IdSet_00084778) {
    const u32 o = arg<u32>(ctx, 0), x = arg<u32>(ctx, 1);
    if (load<u32>(o + 20) == 0x03D3BE2Du) {
        call(ctx, 0x0001B2E8u, {load<u32>(0x00F557F0u)});
        return;
    }
    u32 id = load<u32>(o + 12);
    const u32 k = load<u32>(o + 16);
    if (k != 0) {
        const u32 c = static_cast<u32>(call(ctx, 0x004BB4B4u, {x, k}));
        if (c != 0 && static_cast<u32>(VCall(ctx, c, 32, {c})) == 2) {
            const u32 r = static_cast<u32>(VCall(ctx, c, 24, {c}));
            if (r != 0) id = r;
        }
    }
    const u32 y = static_cast<u32>(call(ctx, 0x000D6A28u, {id}));
    if (y == 0) {
        call(ctx, 0x0056A6E0u, {id});
        return;
    }
    if (load<u32>(o + 20) == 0x8FDDEC84u)
        call(ctx, 0x0001B274u, {load<u32>(0x00F557F0u), y});
    else
        call(ctx, 0x0001B238u, {load<u32>(0x00F557F0u), y});
}

// u32 f(T* o, Effect* x) (no direct caller): with the slot o->+16 !=
// 0xFFFF its 12-byte record at *0x00F65CA0 + 0x128588 gets f32 o->+20,
// f32 o->+24 (+4) and the u8 flag bit 0x8000 of u16 o->+18 (+8); else the
// bound object (component Effect_FindComponent(x, o->+12) with
// c->vt[32](c) == 5, its +28 of type 0x02235C33 (+276), that one's +280)
// gets Obj_SetTimeTree_000DAB90(q, 0x02235C33, 0; o->+20) (r4 as the
// original leaves it). r3 / f1 as left.
DECOMP(00092014, EffectCmd_SetTime_00092014) {
    const u32 o = arg<u32>(ctx, 0), x = arg<u32>(ctx, 1);
    const u32 idx = load<u16>(o + 16);
    if (idx != 0xFFFFu) {
        const u32 r = load<u32>(0x00F65CA0u) + 12 * idx;
        const u32 h = load<u16>(o + 18);
        const u32 a = load<u32>(o + 20), b = load<u32>(o + 24);
        store<u32>(r + 0x00128588u, a);
        store<u8>(r + 0x00128590u, static_cast<u8>((h >> 15) & 1u));
        store<u32>(r + 0x0012858Cu, b);
        ctx.fpr[1] = static_cast<f64>(std::bit_cast<f32>(a));
        ctx.gpr[3] = r + 0x0012858Cu;
        return;
    }
    const u32 k = load<u32>(o + 12);
    ctx.gpr[3] = k;
    if (k == 0) return;
    const u32 c = static_cast<u32>(call(ctx, 0x004BB4B4u, {x, k}));
    if (c == 0) return;
    if (static_cast<u32>(VCall(ctx, c, 32, {c})) != 5) return;
    const u32 t = load<u32>(c + 28);
    if (t == 0) return;
    const u32 ty = load<u32>(t + 276);
    ctx.gpr[3] = 0x02235C33u;
    if (ty != 0x02235C33u) return;
    const u32 q = load<u32>(t + 280);
    if (q == 0) return;
    callf(ctx, 0x000DAB90u, {q, ty, 0}, {Lfs40(o + 20)});
}

// T* f(?, XmlCursor* x) (no direct caller): a 36-byte node (Mem_Alloc(36,
// 0, 16); r3 = 0 on failure): Obj_InitVt_B21C98(p, x), u8 +12 = 0, vtable
// 0x00B110A0, +16 = 0, +20 = 1.0, +24 / +28 = 0; bool 0x7A281983 -> +12
// (default 0) and f32 0x3FD77BF5 / 0x00264412 / 0xDE55FE5F / 0x4E047B49
// -> +16 / +20 / +24 / +28 (default 0.0 = *0x0009C070 each). r3 = p.
DECOMP(0009C078, XmlNode36_New_0009C078) {
    const u32 x = arg<u32>(ctx, 1);
    const u32 p = static_cast<u32>(call(ctx, 0x0054ED38u, {36, 0, 16}));
    if (p == 0) {
        ret(ctx, 0u);
        return;
    }
    call(ctx, 0x004BB3B0u, {p, x});
    store<u8>(p + 12, 0);
    store<u32>(p, 0x00B110A0u);
    const f64 def = Lfs40(0x0009C070u);
    store<u32>(p + 16, 0);
    store<u32>(p + 20, 0x3F800000u);
    store<u32>(p + 24, 0);
    store<u32>(p + 28, 0);
    call(ctx, 0x004F986Cu, {x, 0x7A281983u, p + 12, 0});
    callf(ctx, 0x004F9B20u, {x, 0x3FD77BF5u, p + 16}, {def});
    callf(ctx, 0x004F9B20u, {x, 0x00264412u, p + 20}, {def});
    callf(ctx, 0x004F9B20u, {x, 0xDE55FE5Fu, p + 24}, {def});
    callf(ctx, 0x004F9B20u, {x, 0x4E047B49u, p + 28}, {def});
    ret(ctx, p);
}

// T* f(?, XmlCursor* x) (no direct caller): a 32-byte anim command
// (Mem_Alloc(32, 0, 16); r3 = 0 on failure): AnimCmd_ConstructBase(p),
// +12..+28 = 0, vtable 0x00B11620; s32 0x368328E5 (default -1, read into a
// stack word; -1 becomes 0) -> +12, hashes 0x75751B32 / 0x0F7D01C3 /
// 0x862E6E1C -> +16 / +20 / +24 and f32 0x3D5411C2 -> +28 (defaults the
// current values). r3 = p.
DECOMP(000A2B34, AnimCmd_New32_000A2B34) {
    const u32 x = arg<u32>(ctx, 1);
    const u32 p = static_cast<u32>(call(ctx, 0x0054ED38u, {32, 0, 16}));
    if (p == 0) {
        ret(ctx, 0u);
        return;
    }
    call(ctx, 0x004B9CB8u, {p});
    store<u32>(p + 12, 0);
    store<u32>(p + 16, 0);
    store<u32>(p + 20, 0);
    store<u32>(p + 24, 0);
    store<u32>(p, 0x00B11620u);
    store<u32>(p + 28, 0);
    {
        Stack fr(ctx, 16);  // the s32 at sp+112
        store<u32>(fr.addr(), 0xFFFFFFFFu);
        call(ctx, 0x004F99BCu, {x, 0x368328E5u, fr.addr(), Sx40(0xFFFFFFFFu)});
        u32 v = load<u32>(fr.addr());
        const u32 h16 = load<u32>(p + 16);
        if (v == 0xFFFFFFFFu) {
            v = 0;
            store<u32>(fr.addr(), 0);
        }
        store<u32>(p + 12, v);
        call(ctx, 0x004FA14Cu, {x, 0x75751B32u, p + 16, h16});
    }
    call(ctx, 0x004FA14Cu, {x, 0x0F7D01C3u, p + 20, load<u32>(p + 20)});
    call(ctx, 0x004FA14Cu, {x, 0x862E6E1Cu, p + 24, load<u32>(p + 24)});
    callf(ctx, 0x004F9B20u, {x, 0x3D5411C2u, p + 28}, {Lfs40(p + 28)});
    ret(ctx, p);
}

// void f(Obj* o) (f_000FBA18 etc.): with a world object w = o->+160 and a
// nonempty o->+340 list but no peer (WorldObj_GetPeer(w)),
// WorldObj_UpdateLod(w, 0, 0); Obj_PlaceAttachments(o) when w is set and
// any of: o->+172 bit 8, w of kind 8 ((w->+4 >> 10) & 63) seen this frame
// (WorldObj_SeenThisFrame), w not of kind 8, u16 o->+176 bit 0x100 or 0x40,
// o->+244 set. With bit 0x100 of u16 o->+176: o->vt[220](o, bit 0x80) and
// both bits cleared (r3 = 0; else r3 = that u16).
DECOMP(000D85A4, Obj_SyncWorld_000D85A4) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 w0 = load<u32>(o + 160);
    if (w0 != 0) {
        const u32 a = load<u32>(o + 340);
        if (a != 0 && load<u16>(a + 8) != 0) {
            if (static_cast<u32>(call(ctx, 0x0057A428u, {w0})) == 0) call(ctx, 0x0057EE00u, {load<u32>(o + 160), 0, 0});
        }
    }
    u32 w = load<u32>(o + 160);
    if (w != 0) {
        bool place = (load<u32>(o + 172) & 8u) != 0;
        if (!place) {
            if (((load<u32>(w + 4) >> 10) & 63u) == 8) {
                if (static_cast<u32>(call(ctx, 0x00583EB8u, {w})) != 0)
                    place = true;
                else
                    w = load<u32>(o + 160);
            }
        }
        if (!place) {
            const u32 h = load<u16>(o + 176);
            place = ((load<u32>(w + 4) >> 10) & 63u) != 8 || (h & 0x100u) != 0 || (h & 0x40u) != 0 || load<u32>(o + 244) != 0;
        }
        if (place) call(ctx, 0x000D7A00u, {o});
    }
    const u32 h = load<u16>(o + 176);
    ctx.gpr[3] = h;
    if ((h & 0x100u) == 0) return;
    VCall(ctx, o, 220, {o, (h >> 7) & 1u});
    store<u16>(o + 176, static_cast<u16>(load<u16>(o + 176) & ~0x180u));
    ret(ctx, 0u);
}

// u32 f(Obj* o, u32 key) (Obj_TargetCallD99D8_000972CC etc.): template
// attach of the kinds 18 / 28 / 30 (ObjAttachTmpl40).
DECOMP(000D99D8, Obj_AttachTmpl18_000D99D8) { ObjAttachTmpl40(ctx, 18); }
DECOMP(000D9B30, Obj_AttachTmpl28_000D9B30) { ObjAttachTmpl40(ctx, 28); }
DECOMP(000D9C88, Obj_AttachTmpl30_000D9C88) { ObjAttachTmpl40(ctx, 30); }

// u32 f(T* o) (no direct caller): the follow point: the target o->+12's
// position p = t->vt[156](&p, t, 0); with the spline o->+24 its nearest
// parameter, the spline point q there (WorldObj_SplinePointAt(s, ?, &q,
// 0; that parameter)) and p moved to q + o->+64 * (p - q); then the
// nearest parameter on o->+24 (or on o->+20 without it) -> f32 o->+96, p
// -> o->+80..+88 and bit 0x20000000 of o->+176 set. r3 / f1 as left.
DECOMP(0010B0E4, Follower_Update_0010B0E4) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 t = load<u32>(o + 12);
    if (t == 0) {
        ret(ctx, 0u);
        return;
    }
    Stack fr(ctx, 48);  // p at sp+112 (fr), t's result at sp+128 (fr + 16), q at sp+144 (fr + 32)
    const u32 p = fr.addr(), r = fr.addr() + 16, q = fr.addr() + 32;
    VCall(ctx, t, 156, {r, t, 0});
    store<u32>(p, load<u32>(r));
    store<u32>(p + 4, load<u32>(r + 4));
    store<u32>(p + 8, load<u32>(r + 8));
    const u32 s = load<u32>(o + 24);
    if (s != 0) {
        call(ctx, 0x005927FCu, {s, p, 0, 0});
        callf(ctx, 0x00592748u, {load<u32>(o + 24), ctx.gpr[4], q, 0}, {ctx.fpr[1]});  // r4 as left
        const f64 k = Lfs40(o + 64);
        f64 v[3];
        for (u32 i = 0; i < 3; ++i) {
            const f64 b = Lfs40(q + 4 * i);
            v[i] = F40(std::fma(k, F40(Lfs40(p + 4 * i) - b), b));
        }
        for (u32 i = 0; i < 3; ++i) store<f32>(p + 4 * i, static_cast<f32>(v[i]));
        call(ctx, 0x005927FCu, {load<u32>(o + 24), p, 0, 0});
    } else {
        call(ctx, 0x005927FCu, {load<u32>(o + 20), p, 0, 0});
    }
    const u32 w = load<u32>(o + 176) | 0x20000000u;
    store<f32>(o + 96, static_cast<f32>(ctx.fpr[1]));
    store<u32>(o + 80, load<u32>(p));
    store<u32>(o + 84, load<u32>(p + 4));
    store<u32>(o + 88, load<u32>(p + 8));
    store<u32>(o + 176, w);
    ctx.fpr[1] = Lfs40(p + 4);
    ctx.gpr[3] = w;
}

// bool f(Vec28* v, Obj* x, u32 a, s32 n; f32 f1..f4) (f_001D2EA8): for 1 <=
// n <= the 15-bit count of x's model (x->+208)->+112, a 28-byte record {a,
// n - 1, the 64-byte entry (model->+116)[n - 1]'s +48, |f1|, |f2|, |f3|,
// |f4|} is appended to v (data +0, u16 capacity +4, count +8, high-water
// +10; grown to twice the capacity when full, PtrVector_Reserve). r3 = 1,
// else 0.
DECOMP(001D2D50, Vec28_PushPart_001D2D50) {
    const u32 v = arg<u32>(ctx, 0), x = arg<u32>(ctx, 1), a = arg<u32>(ctx, 2);
    if (v == 0 || x == 0) {
        ret(ctx, 0u);
        return;
    }
    const s32 i = arg<s32>(ctx, 3) - 1;
    if (i < 0) {
        ret(ctx, 0u);
        return;
    }
    const u32 m = load<u32>(x + 208);
    if (!(i < static_cast<s32>(load<u32>(m + 112) & 0x7FFFu))) {
        ret(ctx, 0u);
        return;
    }
    const u32 h = load<u32>(load<u32>(m + 116) + (static_cast<u32>(i) << 6) + 48);
    const f64 f[4] = {std::fabs(farg(ctx, 0)), std::fabs(farg(ctx, 1)), std::fabs(farg(ctx, 2)), std::fabs(farg(ctx, 3))};
    u32 n = load<u16>(v + 8);
    const u32 cap = load<u16>(v + 4);
    if (n == cap) {
        call(ctx, 0x00551718u, {v, (cap << 1) & 0x1FFFEu});
        n = load<u16>(v + 8);
    }
    const u32 p = load<u32>(v) + 28 * n;
    store<u32>(p, a);
    store<u32>(p + 4, static_cast<u32>(i));
    store<u32>(p + 8, h);
    for (u32 k = 0; k < 4; ++k) store<f32>(p + 12 + 4 * k, static_cast<f32>(f[k]));
    const u32 n1 = load<u16>(v + 8) + 1;
    const u32 hw = load<u16>(v + 10);
    store<u16>(v + 8, static_cast<u16>(n1));
    store<u16>(v + 10, static_cast<u16>(static_cast<s32>(hw) > static_cast<s32>(n1 & 0xFFFFu) ? hw : n1));
    ret(ctx, 1u);
}

// void f(f32* out, Ring* r; f32 x) (Aim_Update_00239DA4): x goes into the
// float ring r (begin +0, end +4, write +8, read +12, capacity +16, count
// +20; the oldest dropped when full; a null write pointer skips the
// store), then *out = the average of the stored values (read to write,
// written out as the sum after each one). f1 = it.
DECOMP(00239C50, Ring_PushAverage_00239C50) {
    const u32 out = arg<u32>(ctx, 0), it = arg<u32>(ctx, 1);
    const f64 x = farg(ctx, 0);
    u32 cnt = load<u32>(it + 20);
    const u32 cap = load<u32>(it + 16);
    const u32 wp = load<u32>(it + 8), end = load<u32>(it + 4), beg = load<u32>(it);
    u32 rp = load<u32>(it + 12);
    auto wrap = [&](u32 p) { return static_cast<s32>(p - end) < 0 ? p : beg; };
    if (cnt == cap) {
        rp = wrap(rp + 4);
        --cnt;
        store<u32>(it + 20, cnt);
        store<u32>(it + 12, rp);
    }
    const u32 wp2 = wrap(wp + 4);
    store<u32>(it + 20, cnt + 1);
    store<u32>(it + 8, wp2);
    if (wp != 0) store<f32>(wp, static_cast<f32>(x));
    f64 sum = Lfs40(0x00239C44u);  // 0.0
    const f64 one = Lfs40(0x00239C48u);  // 1.0
    store<u32>(out, 0);
    u32 p = cnt + 1 != 0 ? rp : 0u;
    f64 num = sum;
    do {
        if (p != 0) {
            const f64 e = Lfs40(p);
            num = F40(num + one);
            sum = F40(sum + e);
            store<f32>(out, static_cast<f32>(sum));
        }
        const u32 q = wrap(p + 4);
        p = q != wp2 ? q : 0u;
    } while (p != 0);
    if (num > one) {
        sum = F40(sum * F40(one / num));
        store<f32>(out, static_cast<f32>(sum));
    }
    ctx.fpr[1] = sum;
}

// void f(Obj* o; f32 t) (f_002C7340): the time scale x = t (1.0 for t <=
// 0) clamped to [0.001, 1000] -> f32 o->+392 and the record at
// *0x00F65CA0 + 0x128588 (x, -1.0, u8 1); Rec_Set_0048EF50(*0x00F65CA0 +
// 0x1282C0; 1.0, x, -1.0, 0.0); Obj_SetTimeTree_000DAB90(o->+276, ?, 0;
// 1.0) and, when the bit 1 of (o->+276)->+188 is set and the player
// *(*0x00F65CA0 + 0x1281E0) exists, Obj_SetTimeTree_000DAB90(it,
// 0x001281E0, 0; 1 / x) (r4 as the original leaves it). r3 / f1 as left.
DECOMP(002C65E0, Obj_SetTimeScale_002C65E0) {
    const u32 o = arg<u32>(ctx, 0);
    constexpr u32 K = 0x002C65B0u;  // 0.0f, 1.0f, 1000.0f, 1000.0, 0.001f, (double)0.001f, -1.0f
    f64 x = farg(ctx, 0);
    if (x <= Lfs40(K)) x = Lfs40(K + 4);
    const f64 d16 = std::bit_cast<f64>(load<u64>(K + 16)), d32 = std::bit_cast<f64>(load<u64>(K + 32));
    f64 c = F40(Lfs40(K + 8) - x);
    x = F40(c >= 0.0 ? x : d16);
    c = F40(x - Lfs40(K + 24));
    x = F40(c >= 0.0 ? x : d32);
    store<f32>(o + 392, static_cast<f32>(x));
    const u32 g = load<u32>(0x00F65CA0u);
    store<f32>(g + 0x00128588u, static_cast<f32>(x));
    const f64 m1 = Lfs40(K + 40);
    store<u32>(g + 0x0012858Cu, 0xBF800000u);
    store<u8>(g + 0x00128590u, 1);
    const f64 one = Lfs40(K + 4);
    callf(ctx, 0x0048EF50u, {load<u32>(0x00F65CA0u) + 0x001282C0u}, {one, Lfs40(o + 392), m1, Lfs40(K)});
    callf(ctx, 0x000DAB90u, {load<u32>(o + 276), ctx.gpr[4], 0}, {one});  // r4 as left
    const u32 t = load<u32>(o + 276);
    const f64 inv = F40(one / Lfs40(o + 392));
    const u32 b = load<u32>(t + 188) & 1u;
    ctx.gpr[3] = b;
    if (b == 0) return;
    const u32 w = load<u32>(load<u32>(0x00F65CA0u) + 0x001281E0u);
    ctx.gpr[3] = w;
    if (w == 0) return;
    callf(ctx, 0x000DAB90u, {w, 0x001281E0u, 0}, {inv});
}

// u32 f(Sect* o, T* p) (no direct caller): for an active section (u8
// o->+18 == 4): each active child c (u8 +18 == 4) of the chain o->+2576
// (next +2564) gets c->vt[60](c, x) until o->+2572 becomes 6; x is p when
// o->+2588 is clear, else a stack pointer vector (PtrVector_Construct,
// u16 +6 = 128) that then goes to Obj_PushNearest327A58(o, p, &vec) and is
// released (PtrVector_Clear / Nop). r3 as the original leaves it.
DECOMP(00327CB8, Sect_CollectActive_00327CB8) {
    const u32 o = arg<u32>(ctx, 0), p = arg<u32>(ctx, 1);
    const u32 st = load<u8>(o + 18);
    ctx.gpr[3] = st;
    if (st != 4) return;
    const u32 f = load<u32>(o + 2588);
    ctx.gpr[3] = f;
    if (f == 0) {
        for (u32 c = load<u32>(o + 2576); c != 0; c = load<u32>(c + 2564)) {
            const u32 s = load<u8>(c + 18);
            ctx.gpr[3] = s;
            if (s != 4) continue;
            VCall(ctx, c, 60, {c, p});
            const u32 k = load<u32>(o + 2572);
            ctx.gpr[3] = k;
            if (k == 6) return;
        }
        return;
    }
    Stack fr(ctx, 32);  // the vector at sp+112
    const u32 v = fr.addr();
    call(ctx, 0x005516E0u, {v});
    store<u16>(v + 6, 128);
    for (u32 c = load<u32>(o + 2576); c != 0; c = load<u32>(c + 2564)) {
        if (load<u8>(c + 18) != 4) continue;
        VCall(ctx, c, 60, {c, v});
        if (load<u32>(o + 2572) == 6) break;
    }
    call(ctx, 0x00327A58u, {o, p, v});
    call(ctx, 0x0054EC1Cu, {v});
    call(ctx, 0x0054EC18u, {v});
}

// T::T(o, a, const Def* d) (XmlDef_New_00332898): Obj_Construct_00769BD8(o,
// a), vtable 0x00B1EA80, the f32 vector o + 2572 (PtrVector_Construct, u16
// +2578 = 4, PtrVector_Init(v, 4, 0, 0)), +2592 = d->+20, u8 +2596 =
// d->+24, +2600 / +2604 = 0, then the u16 d->+8 words of d->+0 appended
// (grown to twice the capacity u16 +2576 when full, PtrVector_Reserve;
// count +2580, high-water +2582). r3 / f1 as the original leaves them.
DECOMP(0033273C, SectDef_Ctor_0033273C) {
    const u32 o = arg<u32>(ctx, 0), a = arg<u32>(ctx, 1), d = arg<u32>(ctx, 2);
    call(ctx, 0x00769BD8u, {o, a});
    const u32 v = o + 2572;
    store<u32>(o, 0x00B1EA80u);
    call(ctx, 0x005516E0u, {v});
    store<u16>(o + 2578, 4);
    call(ctx, 0x00551598u, {v, 4, 0, 0});
    u32 r27 = load<u16>(o + 2580);
    u32 r3 = r27;
    store<u32>(o + 2592, 0);
    store<u8>(o + 2596, 0);
    const u32 w20 = load<u32>(d + 20);
    store<u32>(o + 2600, 0);
    store<u32>(o + 2604, 0);
    const u8 b24 = load<u8>(d + 24);
    store<u32>(o + 2592, w20);
    store<u8>(o + 2596, b24);
    for (u32 i = 0; i < load<u16>(d + 8); ++i) {
        const u32 src = load<u32>(d) + 4 * i;
        const u32 cap = load<u16>(o + 2576);
        if ((r27 & 0xFFFFu) == cap) {
            call(ctx, 0x00551718u, {v, (cap << 1) & 0x1FFFEu});
            r3 = load<u16>(o + 2580);
        }
        store<u32>(load<u32>(o + 2572) + ((r3 << 2) & 0x3FFFCu), load<u32>(src));
        ctx.fpr[1] = Lfs40(src);  // left in f1
        r3 = load<u16>(o + 2580) + 1;
        const u32 hw = load<u16>(o + 2582);
        r27 = r3 & 0xFFFFu;
        store<u16>(o + 2580, static_cast<u16>(r3));
        u32 nh = hw;
        if (!(static_cast<s32>(hw) > static_cast<s32>(r27))) {
            nh = r3;
            r27 = r3;
        }
        store<u16>(o + 2582, static_cast<u16>(nh));
    }
    ctx.gpr[3] = r3;
}

// T::~T(o) (Delete_0036DE88): vtable 0x00B1F790, the vector o + 164
// released (PtrVector_Clear / Nop), vtable 0x00B1F678; the effect o->+20
// gets Effect_Release(e, 0x00010CBB) and bit 0x200 of its +136; the
// object o->+24 (its +276 / +280 cleared when +276 is 0x7DF841DC) gets
// x->vt[40](x, 0, 1) and x->vt[8](x); o->+28->vt[4](o->+28) and o->+28 =
// 0; then VtB21B58_DtorTrivial_004A0F70(o + 48) (r3 = its).
DECOMP(0036DD30, Obj_Dtor_B1F790_0036DD30) {
    const u32 o = arg<u32>(ctx, 0);
    store<u32>(o, 0x00B1F790u);
    call(ctx, 0x0054EC1Cu, {o + 164});
    call(ctx, 0x0054EC18u, {o + 164});
    const u32 e = load<u32>(o + 20);
    store<u32>(o, 0x00B1F678u);
    if (e != 0) {
        call(ctx, 0x004CF3C4u, {e, 0x00010CBBu});
        const u32 e2 = load<u32>(o + 20);
        store<u32>(e2 + 136, load<u32>(e2 + 136) | 0x200u);
    }
    const u32 x = load<u32>(o + 24);
    if (x != 0) {
        if (load<u32>(x + 276) == 0x7DF841DCu) {
            store<u32>(x + 276, 0);
            store<u32>(x + 280, 0);
        }
        VCall(ctx, x, 40, {x, 0, 1});
        const u32 x2 = load<u32>(o + 24);
        VCall(ctx, x2, 8, {x2});
    }
    const u32 y = load<u32>(o + 28);
    if (y != 0) {
        VCall(ctx, y, 4, {y});
        store<u32>(o + 28, 0);
    }
    call(ctx, 0x004A0F70u, {o + 48});
}

// u32 f(T* o, Node* n, u8 now) (no direct caller): o->vt[28](o, c, now)
// for n's children n->+268 and n->+272 (when set); without `now` and with
// bit 0x8000 of n->+164, n is appended to o's vector o + 164 (grown to
// twice the capacity u16 +168 when full, PtrVector_Reserve; count +172,
// high-water +174) and n->vt[40](n, 0, 0) (n's vtable read before the
// last store). r3 as the original leaves it.
DECOMP(0036E868, Obj_AddNode_0036E868) {
    const u32 o = arg<u32>(ctx, 0), n = arg<u32>(ctx, 1), now = arg<u32>(ctx, 2) & 0xFFu;
    const u32 a = load<u32>(n + 268);
    if (a != 0) VCall(ctx, o, 28, {o, a, now});
    const u32 b = load<u32>(n + 272);
    ctx.gpr[3] = b;
    if (b != 0) VCall(ctx, o, 28, {o, b, now});
    if (now != 0) return;
    const u32 bit = (load<u32>(n + 164) >> 15) & 1u;
    if (bit == 0) {
        ret(ctx, 0u);
        return;
    }
    u32 c = load<u16>(o + 172);
    const u32 cap = load<u16>(o + 168);
    if (c == cap) {
        call(ctx, 0x00551718u, {o + 164, (cap << 1) & 0x1FFFEu});
        c = load<u16>(o + 172);
    }
    store<u32>(load<u32>(o + 164) + ((c << 2) & 0x3FFFCu), n);
    const u32 c1 = load<u16>(o + 172) + 1;
    const u32 hw = load<u16>(o + 174);
    const u32 vt = load<u32>(n);
    store<u16>(o + 172, static_cast<u16>(c1));
    store<u16>(o + 174, static_cast<u16>(static_cast<s32>(hw) > static_cast<s32>(c1 & 0xFFFFu) ? hw : c1));
    VCallVt(ctx, vt, 40, {n, 0, 0});
}

// u32 f(T* o) (Obj_Update_003D280C): with the player
// *(o->vt[76](o) + 0x001281E0) set, its view o->+56 gets the camera
// definition 0x8080B444 (CameraDefMap_Find((*0x00F8F134)->+16, &key)):
// +80 = def->+176 (read through null without one, a quirk) and the 4 x 3
// rows +16.. = the def's +112.. rows with rows 0 and 2 negated (the rows
// of 0x01164BD0 without a def). r3 / f1 as the original leaves them.
DECOMP(003D26B8, Cam_ApplyDef_003D26B8) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 g = static_cast<u32>(VCall(ctx, o, 76, {o}));
    const u32 w = load<u32>(g + 0x001281E0u);
    ctx.gpr[3] = w;
    if (w == 0) return;
    u32 d;
    {
        Stack fr(ctx, 16);  // the key at sp+112
        store<u32>(fr.addr(), 0x8080B444u);
        const u32 r = static_cast<u32>(call(ctx, 0x0073374Cu, {load<u32>(load<u32>(0x00F8F134u) + 16), fr.addr()}));
        d = load<u32>(r);
    }
    u32 m[12];
    for (u32 k = 0; k < 12; ++k) m[k] = load<u32>(0x01164BD0u + 16 * (k / 3) + 4 * (k % 3));
    const u32 f1 = load<u32>(d + 176);
    const u32 x = load<u32>(o + 56);
    ctx.gpr[3] = 0;
    if (d != 0) {
        for (u32 k = 0; k < 12; ++k) {
            m[k] = load<u32>(d + 112 + 16 * (k / 3) + 4 * (k % 3));
            if (k / 3 == 0 || k / 3 == 2) m[k] = std::bit_cast<u32>(-std::bit_cast<f32>(m[k]));  // fneg (sign flip)
        }
        ctx.gpr[3] = d + 112;
    }
    store<u32>(x + 80, f1);
    for (u32 k = 0; k < 12; ++k) store<u32>(x + 16 + 16 * (k / 3) + 4 * (k % 3), m[k]);
    ctx.fpr[1] = static_cast<f64>(std::bit_cast<f32>(f1));
}

// u32 f(XmlCursor* x) (no direct caller): StrPoolPush40 into the vectors
// 0x010B7854 / 0x010B783C.
DECOMP(00483A34, StrPool_PushName_00483A34) { StrPoolPush40(ctx, 0x010B7854u); }
DECOMP(0048414C, StrPool_PushName2_0048414C) { StrPoolPush40(ctx, 0x010B783Cu); }

// Entry* f(T* o, Table* t) (f_004B453C): a weighted random pick: the PRNG
// at 0x01164888 (u64 state +0, u32 +8) steps (x = (rotl(w, 4) + rotl(w,
// 16)) ^ 0x87654321, +8 = rotl(x, 16), +0 = x & 0xFFFF) and the old state
// scaled by 1.52590219e-5 and o->+64 gives the threshold; the weights
// (entry +24) of the o->+60 entries from s16 o->+68 (76-byte entries
// t->+12, count u16 (t->+4)->+2; out of range reads null) are summed until
// the sum reaches it. r3 = that entry + 4 (else the first one's + 4, 4
// when out of range, 0 without entries); f1 = the sum.
DECOMP(004B43E8, Table_PickWeighted_004B43E8) {
    const u32 o = arg<u32>(ctx, 0), t = arg<u32>(ctx, 1);
    constexpr u32 G = 0x01164888u;
    const u64 old = load<u64>(G);
    const f64 c0 = Lfs40(0x004B43E0u);
    const u32 w = load<u32>(G + 8);
    const f64 f3 = Lfs40(o + 64);
    const u32 xr = (std::rotl(w, 4) + std::rotl(w, 16)) ^ 0x87654321u;
    const u32 h = static_cast<u32>(static_cast<s32>(static_cast<s16>(load<u16>(o + 68))));
    store<u32>(G + 8, std::rotl(xr, 16));
    store<u64>(G, xr & 0xFFFFu);
    const u32 n = load<u32>(o + 60);
    const f64 f4 = F40(static_cast<f64>(static_cast<s64>(old)));
    f64 sum = Lfs40(0x004B43E4u);  // 0.0
    const u32 end = h + n;
    const f64 lim = F40(f3 * F40(f4 * c0));
    auto entry = [&](u32 i16) -> u32 {
        const s32 i = static_cast<s16>(i16);
        if (i < 0 || !(i < static_cast<s32>(load<u16>(load<u32>(t + 4) + 2)))) return 0;
        return load<u32>(t + 12) + 76 * static_cast<u32>(i);
    };
    ctx.fpr[1] = sum;
    if (h < end) {
        for (u32 r8 = h; r8 < end; ++r8) {
            const u32 e = entry(r8) + 4;
            sum = F40(sum + Lfs40(e + 20));
            ctx.fpr[1] = sum;
            if (sum >= lim) {
                ret(ctx, e);
                return;
            }
        }
    }
    if (n == 0) {
        ret(ctx, 0u);
        return;
    }
    ret(ctx, entry(h) + 4);
}

// u32 f(T* o, Effect* x) (no direct caller): the component c =
// Effect_FindComponent(x, o->+12) of kind 4 (c->vt[32](c)) takes the 4 x 3
// transform rows of the bound object b (Effect_Bind4BB7E0(x, o->+16, 0),
// else *PointMap_Find((*0x00F8F154)->+16, &o->+16)) from b + 80 into c +
// 32; a missing component or object goes to Hash_ToString(o->+12 /
// o->+16). r3 / f1 as the original leaves them.
DECOMP(004CB1B4, EffectCmd_CopyXform_004CB1B4) {
    const u32 o = arg<u32>(ctx, 0), x = arg<u32>(ctx, 1);
    const u32 c = static_cast<u32>(call(ctx, 0x004BB4B4u, {x, load<u32>(o + 12)}));
    if (c == 0 || static_cast<u32>(VCall(ctx, c, 32, {c})) != 4) {
        call(ctx, 0x0056A6E0u, {load<u32>(o + 12)});
        return;
    }
    u32 b = static_cast<u32>(call(ctx, 0x004BB7E0u, {x, load<u32>(o + 16), 0}));
    if (b == 0) {
        Stack fr(ctx, 16);  // the key at sp+112
        store<u32>(fr.addr(), load<u32>(o + 16));
        const u32 r = static_cast<u32>(call(ctx, 0x0073343Cu, {load<u32>(load<u32>(0x00F8F154u) + 16), fr.addr()}));
        b = load<u32>(r);
    }
    if (b == 0) {
        call(ctx, 0x0056A6E0u, {load<u32>(o + 16)});
        return;
    }
    const u32 s = b + 80;
    for (u32 row = 0; row < 4; ++row)
        for (u32 k = 0; k < 3; ++k) store<u32>(c + 32 + 16 * row + 4 * k, load<u32>(s + 16 * row + 4 * k));
    ctx.fpr[1] = Lfs40(s + 56);
    ctx.gpr[3] = s;
}

// T* f(?, XmlCursor* x) (no direct caller): a 20-byte node (Mem_Alloc(20,
// 0, 16); r3 = 0 on failure): +12 = 0, u16 +4 = 1, u8 +16..+19 = 0,
// vtable 0x00B223A8; the hash 0x8A93973F -> +12 (default 0; when found the
// s32 0x5C8998CD is read over it, default 0), bools 0x54FEB607 -> +16 and
// 0x7A281983 -> +17 (u8 +18 = 1 when the latter is missing), 0x1EC7407C ->
// +19 (defaults the current values). r3 = p.
DECOMP(004CCD18, XmlNode20_New_004CCD18) {
    const u32 x = arg<u32>(ctx, 1);
    const u32 p = static_cast<u32>(call(ctx, 0x0054ED38u, {20, 0, 16}));
    if (p == 0) {
        ret(ctx, 0u);
        return;
    }
    store<u32>(p + 12, 0);
    store<u16>(p + 4, 1);
    store<u8>(p + 16, 0);
    store<u8>(p + 17, 0);
    store<u8>(p + 18, 0);
    store<u32>(p, 0x00B223A8u);
    store<u8>(p + 19, 0);
    if (static_cast<u32>(call(ctx, 0x004FA14Cu, {x, 0x8A93973Fu, p + 12, 0})) != 0)
        call(ctx, 0x004F99BCu, {x, 0x5C8998CDu, p + 12, 0});
    call(ctx, 0x004F986Cu, {x, 0x54FEB607u, p + 16, load<u8>(p + 16)});
    const u32 r = static_cast<u32>(call(ctx, 0x004F986Cu, {x, 0x7A281983u, p + 17, load<u8>(p + 17)}));
    const u32 d19 = load<u8>(p + 19);
    if (r == 0) store<u8>(p + 18, 1);
    call(ctx, 0x004F986Cu, {x, 0x1EC7407Cu, p + 19, d19});
    ret(ctx, p);
}

// void f(T* o) (no direct caller): destructor: +40 = 0; unless bit
// 0x80000000 of o->+44 is set, vtable 0x00B1B940 and the buffer o->+36
// freed through the allocator 0x011DF31C (alloc->vt[20](alloc, p, count *
// 4), entry before OPD); +36 = 0, +44 = 0x80000000, +20 = 0; unless bit
// 0x80000000 of o->+24, vtable 0x00F29900 and o->+16 freed the same way
// (size o->+24 & 0x3FFFFFFF, OPD before entry); +16 = 0, vtable
// 0x00B13974, +24 = 0x80000000; then the TLS router's (r13 - 28332)
// +44 a gets a->vt[12](a, o, u16 o->+4). r3 = its (0 for o == 0).
DECOMP(0075D7B4, Buf_Dtor_0075D7B4) {
    const u32 o = arg<u32>(ctx, 0);
    if (o == 0) {
        ret(ctx, 0u);
        return;
    }
    constexpr u32 AL = 0x011DF31Cu;
    store<u32>(o + 40, 0);
    const u32 w = load<u32>(o + 44);
    if ((w & 0x80000000u) == 0) {
        store<u32>(o, 0x00B1B940u);
        VCallEO36(ctx, load<u32>(AL), 20, {AL, load<u32>(o + 36), Sx40((w << 2) & 0xFFFFFFFCu)});
    }
    const u32 w2 = load<u32>(o + 24);
    store<u32>(o + 36, 0);
    store<u32>(o + 44, 0x80000000u);
    store<u32>(o + 20, 0);
    if ((w2 & 0x80000000u) == 0) {
        store<u32>(o, 0x00F29900u);
        VCallVt(ctx, load<u32>(AL), 20, {AL, load<u32>(o + 16), w2 & 0x3FFFFFFFu});
    }
    store<u32>(o + 16, 0);
    store<u32>(o, 0x00B13974u);
    const u32 t = load<u32>(static_cast<u32>(ctx.gpr[13]) - 28332);
    const u32 h = load<u16>(o + 4);
    store<u32>(o + 24, 0x80000000u);
    const u32 a = load<u32>(t + 44);
    VCall(ctx, a, 12, {a, o, h});
}
