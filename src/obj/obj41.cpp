// Game code, part 41: the small-function sweep continued from the 86
// instruction functions (size order; obj40*.cpp did 83-86). Each was
// checked against the original in the PPC simulator (memory writes, calls
// and their arguments, r3 and f1).
#include "obj41.h"

using namespace sly4;
using namespace sly4::obj41;

// ---------------------------------------------------------------- sweep: 86 instructions
// void f(T* o, u8 b): +44 = +48 = 0, u8 +72 = 0; for each e of the u16
// counted pointer array o->+52 (count +60): u8 e->+57 = e->+58 = 0; for
// each f of e->+36 (count u16 e->+44): f->+56 = 0, u8 f->+84 = 0, f->+60 =
// f->+48 = 0 and each g of f->+64 (count u16 f->+72) gets g->vt[12](g, b).
// The counts are re-read every step. r3 = the final u16 o->+60.
DECOMP(002F92C0, Obj_ResetTree_002F92C0) {
    const u32 o = arg<u32>(ctx, 0), b = arg<u32>(ctx, 1) & 0xFFu;
    store<u32>(o + 44, 0);
    store<u32>(o + 48, 0);
    store<u8>(o + 72, 0);
    for (u32 i = 0;; ++i) {
        const u32 n = load<u16>(o + 60);
        if (i >= n) {
            ctx.gpr[3] = n;
            return;
        }
        const u32 e = load<u32>(load<u32>(o + 52) + 4 * i);
        store<u8>(e + 57, 0);
        store<u8>(e + 58, 0);
        for (u32 j = 0; j < load<u16>(e + 44); ++j) {
            const u32 f = load<u32>(load<u32>(e + 36) + 4 * j);
            store<u32>(f + 56, 0);
            store<u8>(f + 84, 0);
            store<u32>(f + 60, 0);
            store<u32>(f + 48, 0);
            for (u32 k = 0; k < load<u16>(f + 72); ++k) {
                const u32 g = load<u32>(load<u32>(f + 64) + 4 * k);
                VCall(ctx, g, 12, {g, b});
            }
        }
    }
}

// T::T(o, a1, const f32* v, a3, f32 x, f32 y, f32 z, f32 w, b9 .. b15)
// (b9 .. b15 on the stack, bit 0 of each): the base
// Obj_Construct_00769BD8(o, a1), vtable 0x00B1EA08; +2576..+2584 = v[0..2],
// +2592 = a3, f32 +2596 = x * x, +2600 = y * y, +2604 = z, +2608 = w,
// +2616 / +2620 / +2624 = 0, +2628 = -1.0f; the top byte of +2612 becomes
// b9..b15 (bits 31..25, bit 24 clear). r3 = that word, f1 = v[2].
DECOMP(0033162C, Obj_CtorB1EA08_0033162C) {
    const u32 o = arg<u32>(ctx, 0), v = arg<u32>(ctx, 2), a3 = arg<u32>(ctx, 3);
    const u64 a1 = ctx.gpr[4];
    const f64 x = farg(ctx, 0), y = farg(ctx, 1), z = farg(ctx, 2), w = farg(ctx, 3);
    u32 bits = 0;
    for (u32 k = 0; k < 7; ++k) bits |= (StkArg34(ctx, 116 + 8 * k) & 1u) << (31 - k);
    call(ctx, 0x00769BD8u, {o, a1});
    const u32 t = load<u32>(o + 2612);
    store<u32>(o + 2576, load<u32>(v));
    store<u32>(o + 2592, a3);
    store<u32>(o + 2580, load<u32>(v + 4));
    store<u32>(o, 0x00B1EA08u);
    const u32 v2 = load<u32>(v + 8);
    store<u32>(o + 2584, v2);
    store<u32>(o + 2596, W41(F40(x * x)));
    store<u32>(o + 2600, W41(F40(y * y)));
    store<u32>(o + 2604, W41(z));
    store<u32>(o + 2608, W41(w));
    const u32 word = (t & 0x00FFFFFFu) | bits;
    store<u32>(o + 2616, 0);
    store<u32>(o + 2620, 0);
    store<u32>(o + 2624, 0);
    store<u32>(o + 2612, word);
    store<u32>(o + 2628, 0xBF800000u);
    ctx.fpr[1] = static_cast<f64>(std::bit_cast<f32>(v2));
    ret(ctx, word);
}

// u32 f(T* o, a1, const s32* ev): unless u8 o->+18 is 4 this is
// sub_003252D8(o, a1, ev). Otherwise, with the target q = o->+2600 (bit
// 0x40000 of q->+172 set) and *ev == 6: the new level nv = f32 q->+348 is
// compared with the old one o->+2604 (both divided by q->+344 when u8
// o->+2596 is set and q->+344 > 0x00332AA0); when it fell below, the first
// threshold i (1-based) of the u16 counted f32 array o->+2572 (count
// +2580) with nv <= t <= old sends String_HashNoCase(String_Format(
// 0x00ADB954, i)) to the effect o->+2224 (Effect_SetHash4CF988, when set).
// o->+2604 = nv whenever the level was read. r3 = 5.
DECOMP(00332AA8, Obj_LevelCross_00332AA8) {
    const u32 o = arg<u32>(ctx, 0), ev = arg<u32>(ctx, 2);
    if (load<u8>(o + 18) != 4) {
        call(ctx, 0x003252D8u, {o, ctx.gpr[4], ev});
        return;
    }
    const u32 q = load<u32>(o + 2600);
    if (q == 0 || (load<u32>(q + 172) & 0x00040000u) == 0 || load<u32>(ev) != 6) {
        ret(ctx, 5u);
        return;
    }
    u32 outw = load<u32>(q + 348);
    f64 nv = static_cast<f64>(std::bit_cast<f32>(outw));
    f64 cur = Lfs40(o + 2604);
    ctx.fpr[1] = cur;
    if (nv < cur) {
        const f64 d = Lfs40(q + 344);
        if (load<u8>(o + 2596) != 0 && d > Lfs40(0x00332AA0u)) {
            cur = F40(cur / d);
            nv = F40(nv / d);
            outw = W41(nv);
            ctx.fpr[1] = cur;
        }
        for (u32 i = 0; i < load<u16>(o + 2580);) {
            const f64 t = Lfs40(load<u32>(o + 2572) + 4 * i);
            ++i;
            if (cur >= t && nv <= t) {
                const u32 e = load<u32>(o + 2224);
                if (e != 0) {
                    const u32 s = static_cast<u32>(call(ctx, 0x005534D0u, {0x00ADB954u, i}));
                    const u32 h = static_cast<u32>(call(ctx, 0x0056A710u, {s}));
                    call(ctx, 0x004CF988u, {e, h});
                }
                break;
            }
        }
    }
    store<u32>(o + 2604, outw);
    ret(ctx, 5u);
}

// void f(T* o): collects up to 32 entries into o->+2772 (count +2900): the
// hash h of String_Format(0x00ADB994, o + 2740, 1) is looked up in
// (*0x00F8F154)->+16 (PointMap_Find); while found (and the count below
// 32) the value is appended and h moves on: the count 9 / 19 / 29 takes
// the hash of the name with 10 / 20 / 30, any other count h + 1. r3 = the
// last find result (or 32 when full).
DECOMP(00338560, Obj_CollectHashes_00338560) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 s = o + 2740;
    auto hash = [&](u32 k) {
        const u32 p = static_cast<u32>(call(ctx, 0x005534D0u, {0x00ADB994u, s, k}));
        return static_cast<u32>(call(ctx, 0x0056A710u, {p}));
    };
    u32 h = hash(1);
    store<u32>(o + 2900, 0);
    Stack fr(ctx, 16);  // the key at sp+112
    for (;;) {
        const u32 m = load<u32>(0x00F8F154u);
        store<u32>(fr.addr(), h);
        const u32 r = static_cast<u32>(call(ctx, 0x0073343Cu, {load<u32>(m + 16), fr.addr()}));
        const u32 v = load<u32>(r);
        if (v == 0) {
            ret(ctx, r);
            return;
        }
        const u32 n = load<u32>(o + 2900);
        if (n == 32) {
            ret(ctx, 32u);
            return;
        }
        store<u32>(o + (n << 2) + 2772, v);
        const u32 n1 = load<u32>(o + 2900) + 1;
        store<u32>(o + 2900, n1);
        if (n1 == 29) h = hash(30);
        else if (n1 == 19) h = hash(20);
        else if (n1 == 9) h = hash(10);
        else h += 1;
    }
}

// void f(AnimNode* o): Owner_AddName(o, 0x00AEE358, 1),
// AnimClipNode_BindParams_004B2DA0(o); the binding b = Node_Bind(o,
// 0x00AEE910, 5, 0, -1, 0) is reserved for n = (*0x00F65F84)->+268 names
// (Binding_Reserve) and for each i < n the name *(table +264 + 8 i) (read
// at address 0 when s16 i is out of range, quirk) is copied
// (String_CopyN, 100 bytes) into a new Mem_Alloc(100, 0, 16) slot of the
// string array b->+68 (count b->+0) while the count is below b->+72. r3 =
// the count (Binding_Reserve's result for n == 0).
DECOMP(004B5274, AnimNode_BindNames_004B5274) {
    const u32 o = arg<u32>(ctx, 0);
    call(ctx, 0x004D3378u, {o, 0x00AEE358u, 1});
    call(ctx, 0x004B2DA0u, {o});
    const u32 b = static_cast<u32>(call(ctx, 0x004D3088u, {o, 0x00AEE910u, 5, 0, ~0ull, 0}));
    const u32 n = load<u32>(load<u32>(0x00F65F84u) + 268);
    call(ctx, 0x004D2BB8u, {b, n});
    if (n == 0) return;
    u32 cnt = load<u32>(b);
    for (u32 i = 0; i < n; ++i) {
        const u32 g = load<u32>(0x00F65F84u);
        const s32 k = static_cast<s16>(i);
        const u32 ent = (k < 0 || k >= static_cast<s32>(load<u32>(g + 268))) ? 0u : load<u32>(g + 264) + (static_cast<u32>(k) << 3);
        const u32 name = load<u32>(ent);
        if (cnt < load<u32>(b + 72)) {
            const u32 p = static_cast<u32>(call(ctx, 0x0054ED38u, {100, 0, 16}));
            const u32 c = load<u32>(b);
            store<u32>(load<u32>(b + 68) + (c << 2), p);
            call(ctx, 0x00553638u, {load<u32>(load<u32>(b + 68) + (c << 2)), name, 100});
            cnt = load<u32>(b) + 1;
            store<u32>(b, cnt);
        }
    }
    ret(ctx, cnt);
}

// Destructor: vtable 0x00B22358; until the tree root *(o->+36) is empty,
// its leftmost node (the path above it kept in the s16 stack o->+40,
// depth u16 +120, reset each round) is noted in +124 and its key +4
// removed from the AVL tree o + 24 (TreeFind_0073AF5C / Avl_Remove, count
// +28 decremented when found); then +124 = 0 and the base vtable
// 0x00B11B20. r3 = that vtable.
DECOMP(004C71F8, Obj_DtorB22358_004C71F8) {
    const u32 o = arg<u32>(ctx, 0);
    store<u32>(o, 0x00B22358u);
    Stack fr(ctx, 16);  // the find result at sp+112, the u8 flag at sp+116
    for (;;) {
        const u32 root = load<u32>(o + 36);
        store<u16>(o + 120, 0);
        u32 cur = load<u32>(root);
        if (cur == 0) break;
        for (u32 c; (c = load<u32>(cur + 12)) != 0; cur = c) {
            const s32 k = static_cast<s16>(load<u16>(o + 120));
            store<u16>(o + 120, static_cast<u16>(k + 1));
            store<u32>(o + 40 + (static_cast<u32>(k) << 2), cur);
        }
        const u32 key = load<u32>(cur + 4);
        store<u32>(o + 124, cur);
        const u32 t = load<u32>(o + 24);
        store<u8>(fr.addr() + 4, 0);
        if (call(ctx, 0x0073AF5Cu, {o + 24, key, fr.addr(), t}) != 0) {
            call(ctx, 0x0073A36Cu, {o + 24, o + 24, key, fr.addr() + 4});
            store<u32>(o + 28, load<u32>(o + 28) - 1);
        }
    }
    store<u32>(o + 124, 0);
    store<u32>(o, 0x00B11B20u);
    ret(ctx, 0x00B11B20u);
}

// Deleting destructor (o = r3; r3 = 0 for o == 0): vtable 0x00B11E50; the
// pointer vectors +560, +412, +364, +316, +296, +276, +256, +212 and +192
// are cleared (PtrVector_Clear, Nop); vtable 0x00B11D98, Obj_BaseDtor_000C2DA0(o) and
// SmallFree(o, 784) (r3 = its).
DECOMP(00743F94, Obj_DeleteB11E50_00743F94) {
    const u32 o = arg<u32>(ctx, 0);
    if (o == 0) {
        ret(ctx, 0u);
        return;
    }
    store<u32>(o, 0x00B11E50u);
    for (u32 a : {o + 560, o + 412, o + 364, o + 316, o + 296, o + 276, o + 256, o + 212, o + 192}) {
        call(ctx, 0x0054EC1Cu, {a});
        call(ctx, 0x0054EC18u, {a});
    }
    store<u32>(o, 0x00B11D98u);
    call(ctx, 0x000C2DA0u, {o});
    call(ctx, 0x00552420u, {o, 784});
}

// Node* New(s16 id): a 24-byte node 0x00B1FCE4 with the bindings
// 0x00B1FCF4 (2, 0), 0x00B1FCFE (2, 0) and 0x00B1FD08 (2, 1)
// (NodeNew3Bind41).
DECOMP(00777300, NodeNew_B1FCE4_00777300) {
    NodeNew3Bind41(ctx, 0x00B1FCE4u, {0x00B1FCF4u, 0x00B1FCFEu, 0x00B1FD08u}, {2, 2, 2}, {0, 0, 1}, {0, 0, 0});
}

// Node* New(s16 id): a 24-byte node 0x00B1FF71 with the bindings
// 0x00B1FF83 (1, 1), 0x00B1FF91 (3, 1) and 0x00B1FF9A (3, 1)
// (NodeNew3Bind41).
DECOMP(00778FCC, NodeNew_B1FF71_00778FCC) {
    NodeNew3Bind41(ctx, 0x00B1FF71u, {0x00B1FF83u, 0x00B1FF91u, 0x00B1FF9Au}, {1, 3, 3}, {1, 1, 1}, {0, 0, 0});
}

// Node* New(s16 id): a 24-byte node 0x00B1FFA3 with the bindings
// 0x00B1FFB1, 0x00B1FFB6 and 0x00B1FFC1 (all 2, 1) (NodeNew3Bind41).
DECOMP(007791D8, NodeNew_B1FFA3_007791D8) {
    NodeNew3Bind41(ctx, 0x00B1FFA3u, {0x00B1FFB1u, 0x00B1FFB6u, 0x00B1FFC1u}, {2, 2, 2}, {1, 1, 1}, {0, 0, 0});
}

// ---------------------------------------------------------------- sweep: 87 instructions
// T* New(?, XmlCursor* x): a 28-byte object (Mem_Alloc(28, 0, 16); r3 = 0
// on failure) made by Obj_InitVt_B21C98, vtable 0x00B0F888, +12 / +16 /
// +20 = 0 and bits 31 / 30 of +24 clear; read: hash 0x8A93973F into +12
// (default 0), 0x7A30D7E3 into +16 and 0x59D213A5 into +20 (default the
// current value), the bool 0xBD75D672 (default bit 30) into bit 30 of +24;
// bit 31 of +24 is set when that read returned 0. r3 = the object.
DECOMP(0007A0E4, NewXmlB0F888_0007A0E4) {
    const u32 x = arg<u32>(ctx, 1);
    const u32 p = static_cast<u32>(call(ctx, 0x0054ED38u, {28, 0, 16}));
    if (p == 0) {
        ret(ctx, 0u);
        return;
    }
    call(ctx, 0x004BB3B0u, {p, x});
    const u32 t = load<u32>(p + 24);
    store<u32>(p + 12, 0);
    store<u32>(p + 16, 0);
    store<u32>(p, 0x00B0F888u);
    store<u32>(p + 20, 0);
    store<u32>(p + 24, t & 0x3FFFFFFFu);
    call(ctx, 0x004FA14Cu, {x, 0x8A93973Fu, p + 12, 0});
    call(ctx, 0x004FA14Cu, {x, 0x7A30D7E3u, p + 16, load<u32>(p + 16)});
    call(ctx, 0x004FA14Cu, {x, 0x59D213A5u, p + 20, load<u32>(p + 20)});
    const u32 flag = (load<u32>(p + 24) & 0x40000000u) != 0 ? 1u : 0u;
    Stack fr(ctx, 16);  // the bool at sp+112
    store<u8>(fr.addr(), static_cast<u8>(flag));
    const u32 r = static_cast<u32>(call(ctx, 0x004F986Cu, {x, 0xBD75D672u, fr.addr(), flag}));
    u32 w = (load<u32>(p + 24) & 0x7FFFFFFFu) | (r == 0 ? 0x80000000u : 0u);
    store<u32>(p + 24, w);
    w = (w & ~0x40000000u) | ((static_cast<u32>(load<u8>(fr.addr())) & 1u) << 30);
    store<u32>(p + 24, w);
    ret(ctx, p);
}

// u32 f(T* o, Effect* x): when o->+12 is one of the nine ids of the table
// 0x00AC4D80 (index i) and o->+16 is set: flag = whether a type 23
// component of the object (*0x00F65CA0)->+4216 (Obj_FindComponent, walked
// with the previous one as the start) has bits 25..29 of +24 equal to i;
// the component e = Effect_FindComponent(x, o->+16) with e->vt[32](e) ==
// 2 gets +20 = flag. r3 as the original leaves it.
DECOMP(0007BB18, Obj_SyncSlotFlag_0007BB18) {
    const u32 o = arg<u32>(ctx, 0), x = arg<u32>(ctx, 1);
    const u32 c = load<u32>(o + 12);
    ctx.gpr[3] = c;
    if (c == 0 || load<u32>(o + 16) == 0) return;
    s32 i = 0;
    while (load<u32>(0x00AC4D80u + 4 * static_cast<u32>(i)) != c) {
        if (++i >= 9) return;
    }
    u32 flag = 0;
    const u32 w = load<u32>(load<u32>(0x00F65CA0u) + 4216);
    if (w != 0) {
        for (u32 k = static_cast<u32>(call(ctx, 0x000D1288u, {w, 23, 0})); k != 0; k = static_cast<u32>(call(ctx, 0x000D1288u, {w, 23, k}))) {
            if (static_cast<s32>((load<u32>(k + 24) >> 25) & 0x1Fu) == i) {
                flag = 1;
                break;
            }
        }
    }
    const u32 e = static_cast<u32>(call(ctx, 0x004BB4B4u, {x, load<u32>(o + 16)}));
    if (e == 0) return;
    if (static_cast<u32>(VCall(ctx, e, 32, {e})) == 2) store<u32>(e + 20, flag);
}

// u32 f(T* o): with o->+12 >= 0 and o->+16 set, the user object o->+12 is
// refreshed (User_RefreshObject(*0x00F65CA0, o->+12, &m)) with the 4x3
// matrix m (rows of 16 bytes) of the PointMap entry for key o->+16 in
// (*0x00F8F154)->+16 (its +16), else the default matrix 0x01164BD0. r3
// as the original leaves it; f1 = the last copied element.
DECOMP(0008D564, User_RefreshFromMap_0008D564) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 a = load<u32>(o + 12);
    if (static_cast<s32>(a) < 0) {
        ret(ctx, a);
        return;
    }
    const u32 key = load<u32>(o + 16);
    if (key == 0) {
        ret(ctx, 0u);
        return;
    }
    Stack fr(ctx, 80);  // the matrix at sp+112..+168, the key at sp+176
    const u32 m = fr.addr();
    auto copy = [&](u32 src) {
        for (u32 r = 0; r < 4; ++r)
            for (u32 k = 0; k < 3; ++k) store<u32>(m + 16 * r + 4 * k, load<u32>(src + 16 * r + 4 * k));
    };
    copy(0x01164BD0u);
    store<u32>(m + 64, key);
    const u32 r = static_cast<u32>(call(ctx, 0x0073343Cu, {load<u32>(load<u32>(0x00F8F154u) + 16), m + 64}));
    const u32 v = load<u32>(r);
    const u32 u = load<u32>(o + 12);
    if (v != 0) copy(v + 16);
    ctx.fpr[1] = v != 0 ? Lfs40(v + 16 + 56) : Lfs40(0x01164BD0u + 52);  // the last f1 load
    call(ctx, 0x00488298u, {load<u32>(0x00F65CA0u), u, m});
}

// T* New(?, XmlCursor* x): a 32-byte anim command (Mem_Alloc(32, 0, 16);
// r3 = 0 on failure; AnimCmd_ConstructBase), vtable 0x00B11280, +12..+24 =
// 0, u8 +28 = 0; read: hash 0xA60090EC into +12 (default 0), 0x93116591
// into +16 and 0x1502BCC1 into +20 (default the current value); the s32
// 0x21B3BD9E (default 0) gives +24 = n - 1 (0 for n <= 0) when that read
// returned 0; the bool 0x9F451E77 into u8 +28. r3 = the object.
DECOMP(0009DBF8, NewAnimCmdB11280_0009DBF8) {
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
    store<u32>(p, 0x00B11280u);
    store<u8>(p + 28, 0);
    call(ctx, 0x004FA14Cu, {x, 0xA60090ECu, p + 12, 0});
    call(ctx, 0x004FA14Cu, {x, 0x93116591u, p + 16, load<u32>(p + 16)});
    call(ctx, 0x004FA14Cu, {x, 0x1502BCC1u, p + 20, load<u32>(p + 20)});
    {
        Stack fr(ctx, 16);  // the s32 at sp+112
        store<u32>(fr.addr(), 0);
        if (call(ctx, 0x004F99BCu, {x, 0x21B3BD9Eu, fr.addr(), 0}) == 0) {
            const s32 n = static_cast<s32>(load<u32>(fr.addr()));
            store<u32>(p + 24, n > 0 ? static_cast<u32>(n - 1) : 0u);
        }
    }
    call(ctx, 0x004F986Cu, {x, 0x9F451E77u, p + 28, load<u8>(p + 28)});
    ret(ctx, p);
}

// u32 f(T* o): the state s = o->vt[32](o, 2): 0 sends the effect o->+356
// (when set) hash 0x6D6A39BB and calls o->vt[28](o, 2, 1); 1 sets the
// track k = Anim_TrackSet36_0029879C(o->+720, 0, 1) (with +720, else
// Anim_TrackSet164_000D8760(o, 0, 1)), calls Obj_Call354198_002988EC(
// o->+720, 0) when k and +720 are set, and with k applies the key
// (Obj_ApplyTrackKey_000B6A58(o)) when the time f32 (o->+12)->+8 reached
// the key a = o->+20 + u16 o->+6 (f32 +0) or u8 a->+4 is set. r3 / f1 as
// the original leaves them.
DECOMP(000BE030, Obj_TrackState_000BE030) {
    const u32 o = arg<u32>(ctx, 0);
    const s32 s = static_cast<s32>(static_cast<u32>(VCall(ctx, o, 32, {o, 2})));
    ctx.gpr[3] = static_cast<u32>(s);
    if (s < 0 || s > 1) return;
    if (s == 0) {
        const u32 e = load<u32>(o + 356);
        if (e != 0) call(ctx, 0x004CF988u, {e, 0x6D6A39BBu});
        VCall(ctx, o, 28, {o, 2, 1});
        return;
    }
    const u32 t = load<u32>(o + 720);
    const u32 k = static_cast<u32>(t != 0 ? call(ctx, 0x0029879Cu, {t, 0, 1}) : call(ctx, 0x000D8760u, {o, 0, 1}));
    if (k != 0) {
        const u32 t2 = load<u32>(o + 720);
        if (t2 != 0) call(ctx, 0x002988ECu, {t2, 0});
    }
    const u32 a = load<u32>(o + 20) + load<u16>(o + 6);
    const f64 now = Lfs40(load<u32>(o + 12) + 8);
    ctx.fpr[1] = now;
    u32 r3 = a;
    if (!(now >= Lfs40(a))) {
        r3 = load<u8>(a + 4);
        if (r3 == 0) {
            ret(ctx, 0u);
            return;
        }
    }
    if (k == 0) {
        ret(ctx, r3);
        return;
    }
    call(ctx, 0x000B6A58u, {o});
}

// u32 f(T* o): RegAddUnique41 on the registry 0x01013B5C with the
// warning 0x00AC6AD0.
DECOMP(000D6614, Registry_AddUnique_000D6614) { RegAddUnique41(ctx, 0x01013B5Cu, 0x00AC6AD0u); }

// u32 f(T* o): RegAddUnique41 on the registry 0x01013B70 with the
// warning 0x00AC6B58.
DECOMP(000D6770, Registry_AddUnique_000D6770) { RegAddUnique41(ctx, 0x01013B70u, 0x00AC6B58u); }

// u32 f(T* o, f32 dt): Obj_SyncWorld_000D85A4(o); the current part
// c = o->+364[u8 o->+345] (when the index and c are nonzero) gets
// c->vt[52](c, o + 480) with f1 = dt (the OPD in r4, the entry in r6);
// sub_000FB450(o, dt * f32 o->+212). With bit 31 of +344 set:
// o->vt[84](o, o + 480), then the block +544..+579 is copied to
// +356..+467 (f32 +356 / +360, u8 +440, f32 +444 / +448, +452, u8 +456,
// f32 +460, the 4 bytes +464) and sub_000FAE00(o, 0) (r3 = its; 0
// otherwise; f1 as the original leaves it).
DECOMP(000FBA18, Obj_StepParts_000FBA18) {
    const u32 o = arg<u32>(ctx, 0);
    const f64 dt = farg(ctx, 0);
    call(ctx, 0x000D85A4u, {o});
    const u32 b = load<u8>(o + 345);
    if (b != 0) {
        const u32 c = load<u32>(o + 364 + ((b << 2) & 0x3FCu));
        if (c != 0) {
            const u32 opd = load<u32>(load<u32>(c) + 52);
            ctx.fpr[1] = dt;
            call_ptr(ctx, opd, {c, opd, o + 480, load<u32>(opd)});
        }
    }
    callf(ctx, 0x000FB450u, {o}, {F40(dt * Lfs40(o + 212))});
    if ((load<u32>(o + 344) & 0x80000000u) == 0) {
        ret(ctx, 0u);
        return;
    }
    VCall(ctx, o, 84, {o, o + 480});
    store<u32>(o + 356, load<u32>(o + 544));
    store<u32>(o + 360, load<u32>(o + 548));
    store<u8>(o + 440, load<u8>(o + 552));
    store<u32>(o + 444, load<u32>(o + 556));
    store<u32>(o + 448, load<u32>(o + 560));
    store<u32>(o + 452, load<u32>(o + 564));
    store<u8>(o + 456, load<u8>(o + 568));
    store<u32>(o + 460, load<u32>(o + 572));
    for (u32 k = 0; k < 4; ++k) store<u8>(o + 464 + k, load<u8>(o + 576 + k));
    ctx.fpr[1] = Lfs40(o + 572);
    call(ctx, 0x000FAE00u, {o, 0});
}

// u32 f(T* a, Obj* b, s32 kind, d): 0 for kind 3 when bit 0 of the 64-bit
// b->+184 is set, a's target definition (Target_GetDef0D951C) has +68 <= 1,
// (a->+216)->+304 is clear and the team record (*0x00F65CA0) + 4208 +
// 0x20C80 * s8 b->+172 has f32 +108 <= 0x001DC3A4, or (b set) bit 3 of
// b->+192 is set and b's type 23 component has bits 25..29 of +24 == 1
// (the latter test only when +108 is above the limit); otherwise
// sub_000E7FF0(a, b, kind, d). f1 as the original leaves it.
DECOMP(001DC3A8, Obj_TeamGate_001DC3A8) {
    const u32 a = arg<u32>(ctx, 0), b = arg<u32>(ctx, 1), kind = arg<u32>(ctx, 2), d = arg<u32>(ctx, 3);
    auto dflt = [&]() { call(ctx, 0x000E7FF0u, {a, b, kind, d}); };
    if (kind != 3 || (load<u32>(b + 188) & 1u) == 0) return dflt();
    if (call(ctx, 0x000D951Cu, {a}) == 0) return dflt();
    const u32 t = static_cast<u32>(call(ctx, 0x000D951Cu, {a}));
    const s32 v = static_cast<s32>(load<u32>(t + 68));
    const s32 team = static_cast<s8>(load<u8>(b + 172));
    if (v > 1) return dflt();
    const u32 rec = load<u32>(0x00F65CA0u) + static_cast<u32>(team * 0x20C80) + 4208;
    if (load<u32>(load<u32>(a + 216) + 304) != 0) return dflt();
    const f64 f = Lfs40(rec + 108);
    ctx.fpr[1] = f;
    if (!(f <= Lfs40(0x001DC3A4u))) {
        if (b == 0 || (load<u32>(b + 192) & 8u) == 0) return dflt();
        const u32 k = static_cast<u32>(call(ctx, 0x000D1288u, {b, 23, 0}));
        if (k == 0 || ((load<u32>(k + 24) >> 25) & 0x1Fu) != 1) return dflt();
    }
    ret(ctx, 0u);
}

// Vec3* f(Vec3* out, Obj* o): o->vt[80](out, o) when CallK2_000D93AC(o) is
// set, or o->+416 is set and the 12 bytes (o->+216)->+208 differ from
// 0x01164B40; with +416 (and equal bytes) out = f32 (o->+416)->+320..+328
// (r3 = out + 8), without it out = o->+144..+152 (r3 = o + 144). f1 = the
// last copied value.
DECOMP(001FC570, Obj_GetAimPos_001FC570) {
    const u32 out = arg<u32>(ctx, 0), o = arg<u32>(ctx, 1);
    const u32 q = load<u32>(o + 216);
    if (call(ctx, 0x000D93ACu, {o}) != 0) {
        VCall(ctx, o, 80, {out, o});
        return;
    }
    const u32 s = load<u32>(o + 416);
    if (s == 0) {
        const u32 src = o + 144;
        for (u32 k = 0; k < 3; ++k) store<u32>(out + 4 * k, load<u32>(src + 4 * k));
        ctx.fpr[1] = Lfs40(src + 8);
        ret(ctx, src);
        return;
    }
    const u32 p = q + 208;
    if (load<u64>(p) != load<u64>(0x01164B40u) || load<u32>(p + 8) != load<u32>(0x01164B48u)) {
        VCall(ctx, o, 80, {out, o});
        return;
    }
    const u32 src = s + 320;
    for (u32 k = 0; k < 3; ++k) store<u32>(out + 4 * k, load<u32>(src + 4 * k));
    ctx.fpr[1] = Lfs40(src + 8);
    ret(ctx, out + 8);
}

// Destructor: vtable 0x00B1C578; the buffer +100 (u16 count +96, low 15
// bits, 192-byte entries) is freed (SmallFree) when set;
// World_ListRemove_0046B7A4(*0x00F65CA0, 0, 0x00F352C8, o, o->+8), vtable
// 0x00B1BE30, Unlist1056F1C39(o) and List1013B5C_Remove(o) (r3 = its).
DECOMP(002841BC, Obj_DtorB1C578_002841BC) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 p = load<u32>(o + 100);
    store<u32>(o, 0x00B1C578u);
    if (p != 0) call(ctx, 0x00552420u, {p, (static_cast<u32>(load<u16>(o + 96)) & 0x7FFFu) * 192});
    call(ctx, 0x0046B7A4u, {load<u32>(0x00F65CA0u), 0, 0x00F352C8u, o, load<u32>(o + 8)});
    store<u32>(o, 0x00B1BE30u);
    Unlist1056F1C39(o);
    call(ctx, 0x000D6940u, {o});
}

// u32 f(T* o): FlushWorldReq41 (two identical copies, 0x002E2E78 /
// 0x002E2FD4).
DECOMP(002E2E78, Obj_FlushWorldReq_002E2E78) { FlushWorldReq41(ctx); }

// u32 f(T* o): FlushWorldReq41 (the second copy).
DECOMP(002E2FD4, Obj_FlushWorldReq_002E2FD4) { FlushWorldReq41(ctx); }

// u32 f(T* o, a): the effect o->+52 is released (Effect_Release(e, a));
// when o is one of the three slots +900 of its owner t = o->+56 (index
// i), the two parts *(t + 8 i + 916) (only when t->+432 == 1) and
// *(t + 8 i + 920) get vt[128](part, a, 1); then the effect +52, unless
// Effect_Check4CF620(e, 1), gets Effect_SetTime(e, 0x002F953C). r3 as the
// original leaves it.
DECOMP(002F9540, Obj_ReleaseParts_002F9540) {
    const u32 o = arg<u32>(ctx, 0), a = arg<u32>(ctx, 1);
    if (load<u32>(o + 52) != 0) call(ctx, 0x004CF3C4u, {load<u32>(o + 52), a});
    const u32 t = load<u32>(o + 56);
    if (t != 0) {
        for (u32 i = 0; i < 3; ++i) {
            if (load<u32>(t + 900 + 4 * i) != o) continue;
            const u32 v = load<u32>(t + 432);
            const u32 ti = t + (i << 3);
            if (v == 1) {
                const u32 c = load<u32>(ti + 916);
                if (c != 0) VCall(ctx, c, 128, {c, a, 1});
            }
            const u32 d = load<u32>(ti + 920);
            if (d != 0) VCall(ctx, d, 128, {d, a, 1});
            break;
        }
    }
    const u32 e = load<u32>(o + 52);
    if (e == 0) {
        ret(ctx, 0u);
        return;
    }
    if (call(ctx, 0x004CF620u, {e, 1}) != 0) return;
    callf(ctx, 0x004CF710u, {load<u32>(o + 52)}, {Lfs40(0x002F953Cu)});
}

// u32 f(T* o, const char* name, u8 now, ?, u8 c, a5, a6, f32 x): unless o
// is 0 or bit 25 of +396 is set: o->vt[12](o), bit 30 of +396 set; with
// now Obj_Play363890(o, name, c, a5, a6, x); otherwise, unless bit 27 is
// set or +160 >> 1 is 4, bit 26 is cleared and bit 27 set and a nonempty
// name copied to +164 (String_CopyN, 128 bytes). Then
// Obj_AddToList36341C(o) (r3 = its; 0 / 1 on the early exits).
DECOMP(003679C8, Obj_Start_003679C8) {
    const u32 o = arg<u32>(ctx, 0), name = arg<u32>(ctx, 1), now = arg<u32>(ctx, 2) & 0xFFu;
    const u32 c = arg<u32>(ctx, 4) & 0xFFu, a5 = arg<u32>(ctx, 5), a6 = arg<u32>(ctx, 6);
    const f64 x = farg(ctx, 0);
    if (o == 0) {
        ret(ctx, 0u);
        return;
    }
    if ((load<u32>(o + 396) & 0x02000000u) != 0) {
        ret(ctx, 1u);
        return;
    }
    VCall(ctx, o, 12, {o});
    u32 w = load<u32>(o + 396) | 0x40000000u;
    store<u32>(o + 396, w);
    if (now != 0) {
        callf(ctx, 0x00363890u, {o, name, ctx.gpr[5], c, a5, a6}, {x});  // r5 as the virtual call left it (not read)
    } else if ((w & 0x02000000u) == 0 && (w & 0x08000000u) == 0 && ((load<u32>(o + 160) >> 1) & 0x7FFFFFFFu) != 4) {
        w = (w & ~0x04000000u) | 0x08000000u;
        store<u32>(o + 396, w);
        if (name != 0 && load<u8>(name) != 0) call(ctx, 0x00553638u, {o + 164, name, 128});
    }
    call(ctx, 0x0036341Cu, {o});
}

// u32 f(T* o, Node* x): NodeSetRemove41 with the offset 0x003EFE38.
DECOMP(003EFE40, NodeSet_Remove_003EFE40) { NodeSetRemove41(ctx, 0x003EFE38u); }

// u32 f(T* o, Node* x): NodeSetRemove41 with the offset 0x003EFF9C.
DECOMP(003EFFA0, NodeSet_Remove_003EFFA0) { NodeSetRemove41(ctx, 0x003EFF9Cu); }

// u32 f(T* o): when u8 o->+68 is set: World_ListRemove_0046B7A4(
// *0x00F65CA0, 0, 0x00F38648, o, 0), Handle_Notify36742C(0x811E9BA0, 0,
// 1, 0), SetSelect_491000(o, 0x93330CE9), (o->+300)->vt[88](it, 0, 1, 1,
// 1); the layer stack L = *(o->vt[76](o) + 0x1281E0) pops o->+60 with
// 0x003FE914 (Obj_PopLayer, when Obj_HasWord368_000FC5DC(L, o->+60));
// Handle_Notify36742C(0x569F957F, 0, 1, 0) (r3 = its; 0 otherwise).
DECOMP(003FE918, Obj_CloseMenu_003FE918) {
    const u32 o = arg<u32>(ctx, 0);
    if (load<u8>(o + 68) == 0) {
        ret(ctx, 0u);
        return;
    }
    call(ctx, 0x0046B7A4u, {load<u32>(0x00F65CA0u), 0, 0x00F38648u, o, 0});
    call(ctx, 0x0036742Cu, {0x811E9BA0u, 0, 1, 0});
    call(ctx, 0x00491000u, {o, 0x93330CE9u});
    const u32 s = load<u32>(o + 300);
    VCall(ctx, s, 88, {s, 0, 1, 1, 1});
    const u32 g = static_cast<u32>(VCall(ctx, o, 76, {o}));
    const u32 l = load<u32>(g + 0x001281E0u);
    if (l != 0 && call(ctx, 0x000FC5DCu, {l, load<u32>(o + 60)}) != 0) {
        const u32 k = load<u32>(o + 60);
        callf(ctx, 0x000FBB88u, {l, k, k}, {Lfs40(0x003FE914u)});
    }
    call(ctx, 0x0036742Cu, {0x569F957Fu, 0, 1, 0});
}

// u32 f(XmlCursor* x): the settings record t = *0x010C2C28 (r3 = 0
// without) reads the f32 "0x00AF0168" / "0x00AF0170" / "0x00AF0178" into
// +604 / +608 / +612 (default 0x004ED8A4), the color 0x00AF0180 into +616
// (default 0x01164CA0) and the vectors 0x00AF0188 (default (0, 0, 1)) /
// 0x00AF0190 (default 0) into +624 / +640; u8 +600 = 1. r3 = 1.
DECOMP(004ED8A8, Settings_Read_004ED8A8) {
    const u32 x = arg<u32>(ctx, 0);
    const u32 t = load<u32>(0x010C2C28u);
    if (t == 0) {
        ret(ctx, 0u);
        return;
    }
    const f64 dflt = Lfs40(0x004ED8A4u);
    callf(ctx, 0x004FA5C4u, {x, 0x00AF0168u, t + 604}, {dflt});
    callf(ctx, 0x004FA5C4u, {x, 0x00AF0170u, t + 608}, {dflt});
    callf(ctx, 0x004FA5C4u, {x, 0x00AF0178u, t + 612}, {dflt});
    call(ctx, 0x004FA684u, {x, 0x00AF0180u, t + 616, 0x01164CA0u});
    Stack fr(ctx, 16);  // the default vector at sp+112
    const u32 v = fr.addr();
    store<u32>(v, 0);
    store<u32>(v + 4, 0);
    store<u32>(v + 8, 0x3F800000u);
    call(ctx, 0x004FA624u, {x, 0x00AF0188u, t + 624, v});
    store<u32>(v, 0);
    store<u32>(v + 4, 0);
    store<u32>(v + 8, 0);
    call(ctx, 0x004FA624u, {x, 0x00AF0190u, t + 640, v});
    store<u8>(t + 600, 1);
    ret(ctx, 1u);
}

// Node* New(s16 id): a 24-byte node 0x00B20003 with the bindings
// 0x00B20017 (3, 0), 0x00B2001E (4, 0) and 0x00B2002C (4, 1; +76 = 1)
// (NodeNew3Bind41).
DECOMP(0077979C, NodeNew_B20003_0077979C) {
    NodeNew3Bind41(ctx, 0x00B20003u, {0x00B20017u, 0x00B2001Eu, 0x00B2002Cu}, {3, 4, 4}, {0, 0, 1}, {0, 0, 1});
}

// ---------------------------------------------------------------- sweep: 88 instructions
// T* New(?, XmlCursor* x): a 32-byte object (Mem_Alloc(32, 0, 16); r3 = 0
// on failure) made by Obj_InitVt_B21C98, vtable 0x00B0F108, +12..+24 = 0,
// u8 +28 = 1; read: hash 0x8A93973F into +12 (default 0); only when that
// read returned 0: hash 0x9F2AF1E9 into +16, the f32 0xFF48945E into +20,
// the s32 0xE955C5C0 into +24 (through a local that starts at 0, default
// the current value) and the bool 0x2FCB2115 into u8 +28. r3 = the object.
DECOMP(0006C698, NewXmlB0F108_0006C698) {
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
    store<u32>(p, 0x00B0F108u);
    store<u32>(p + 24, 0);
    store<u8>(p + 28, 1);
    if (call(ctx, 0x004FA14Cu, {x, 0x8A93973Fu, p + 12, 0}) == 0) {
        call(ctx, 0x004FA14Cu, {x, 0x9F2AF1E9u, p + 16, load<u32>(p + 16)});
        callf(ctx, 0x004F9B20u, {x, 0xFF48945Eu, p + 20}, {Lfs40(p + 20)});
        Stack fr(ctx, 16);  // the s32 at sp+112
        store<u32>(fr.addr(), 0);
        call(ctx, 0x004F99BCu, {x, 0xE955C5C0u, fr.addr(), Sx40(load<u32>(p + 24))});
        store<u32>(p + 24, load<u32>(fr.addr()));
        call(ctx, 0x004F986Cu, {x, 0x2FCB2115u, p + 28, load<u8>(p + 28)});
    }
    ret(ctx, p);
}

// T* New(?, XmlCursor* x): a 32-byte object (Mem_Alloc(32, 0, 16); r3 = 0
// on failure) made by Obj_InitVt_B21C98, vtable 0x00B0FC70, +12 / +16 = 0,
// f32 +24 = -1.0, u8 +28..+30 = 0; read: hash 0x7A30D7E3 into +16
// (default 0), 0x8A93973F into +12, the f32 0x9F48C90D into +24 (u8 +29
// = 1 when that read returned 0) and the bools 0xB3106A84 / 0x989ABB84
// into u8 +28 / +30. r3 = the object.
DECOMP(00080010, NewXmlB0FC70_00080010) {
    const u32 x = arg<u32>(ctx, 1);
    const u32 p = static_cast<u32>(call(ctx, 0x0054ED38u, {32, 0, 16}));
    if (p == 0) {
        ret(ctx, 0u);
        return;
    }
    call(ctx, 0x004BB3B0u, {p, x});
    store<u32>(p + 12, 0);
    store<u32>(p + 16, 0);
    store<u32>(p + 24, 0xBF800000u);
    store<u32>(p, 0x00B0FC70u);
    store<u8>(p + 28, 0);
    store<u8>(p + 29, 0);
    store<u8>(p + 30, 0);
    call(ctx, 0x004FA14Cu, {x, 0x7A30D7E3u, p + 16, 0});
    call(ctx, 0x004FA14Cu, {x, 0x8A93973Fu, p + 12, load<u32>(p + 12)});
    callf(ctx, 0x004F9B20u, {x, 0x9F48C90Du, p + 24}, {Lfs40(p + 24)});
    const u32 r = static_cast<u32>(ctx.gpr[3]);
    const u32 b = load<u8>(p + 28);
    if (r == 0) store<u8>(p + 29, 1);
    call(ctx, 0x004F986Cu, {x, 0xB3106A84u, p + 28, b});
    call(ctx, 0x004F986Cu, {x, 0x989ABB84u, p + 30, load<u8>(p + 30)});
    ret(ctx, p);
}

// u32 f(T* o, Effect* x): the target t (EffTarget41) gets the effect
// object f = List_FindAfter_000D92B0(t, o->+20, 0): with kind f->+12 == 27
// Obj_SetEffect108_002B78F0(f, o->+24); a missing f formats the warning
// 0x00AC507C with Hash_ToString(o->+20). r3 as the original leaves it.
DECOMP(00088160, EffCmd_SetEffect_00088160) {
    const u32 o = arg<u32>(ctx, 0), x = arg<u32>(ctx, 1);
    const u32 t = EffTarget41(ctx, o, x);
    if (t == 0) {
        ret(ctx, 0u);
        return;
    }
    const u32 f = static_cast<u32>(call(ctx, 0x000D92B0u, {t, load<u32>(o + 20), 0}));
    if (f == 0) {
        const u32 s = static_cast<u32>(call(ctx, 0x0056A6E0u, {load<u32>(o + 20)}));
        call(ctx, 0x005534D0u, {0x00AC507Cu, s});
        return;
    }
    const u32 k = load<u32>(f + 12);
    if (k != 27) {
        ret(ctx, k);
        return;
    }
    call(ctx, 0x002B78F0u, {f, load<u32>(o + 24)});
}

// u32 f(T* o, Effect* x): the target t (EffTarget41) with bit 30 of +196
// set is hit by the team record (*0x00F65CA0) + 4208 + 0x20C80 * o->+20:
// Obj_OnTargetHit_001E0F4C(t, rec->+8) when u8 o->+24 is set, else
// sub_001E4688(t, rec->+8). r3 as the original leaves it.
DECOMP(0008D8C4, EffCmd_TeamHit_0008D8C4) {
    const u32 o = arg<u32>(ctx, 0), x = arg<u32>(ctx, 1);
    const u32 t = EffTarget41(ctx, o, x);
    if (t == 0 || (load<u32>(t + 196) & 0x40000000u) == 0) {
        ret(ctx, 0u);
        return;
    }
    const u32 rec = load<u32>(0x00F65CA0u) + load<u32>(o + 20) * 0x20C80u + 4208;
    call(ctx, load<u8>(o + 24) != 0 ? 0x001E0F4Cu : 0x001E4688u, {t, load<u32>(rec + 8)});
}

// Effect* f(?, a, const Mat43* m, u8 start): 0 for a == 0; otherwise a
// 140-byte effect (SmallAlloc(140), Effect_Construct(p, a); p = 0 on a
// failed allocation); its component 0x001E8ECC of kind 4 (vt[32]) takes
// the 4x3 matrix m (rows of 16 bytes) at +32; with start, +136 |= 0x200
// and Effect_Start(p) (through null for a failed allocation, quirk). r3 =
// p.
DECOMP(000DCECC, Effect_NewAt_000DCECC) {
    const u32 a = arg<u32>(ctx, 1), m = arg<u32>(ctx, 2), start = arg<u32>(ctx, 3) & 0xFFu;
    if (a == 0) {
        ret(ctx, 0u);
        return;
    }
    u32 p = static_cast<u32>(call(ctx, 0x0055238Cu, {140}));
    if (p != 0) call(ctx, 0x004C7A68u, {p, a});
    const u32 c = static_cast<u32>(call(ctx, 0x004BB4B4u, {p, 0x001E8ECCu}));
    if (c != 0 && static_cast<u32>(VCall(ctx, c, 32, {c})) == 4) {
        for (u32 r = 0; r < 4; ++r)
            for (u32 k = 0; k < 3; ++k) store<u32>(c + 32 + 16 * r + 4 * k, load<u32>(m + 16 * r + 4 * k));
        ctx.fpr[1] = Lfs40(m + 56);
    }
    if (start != 0) {
        store<u32>(p + 136, load<u32>(p + 136) | 0x200u);
        call(ctx, 0x004C753Cu, {p});
    }
    ret(ctx, p);
}

// u32 f(T* o, Src* src): while f32 o->+656 < o->+672, the 4x3 matrix
// +448 (rows of 16 bytes) is set to the identity 0x01164BD0 with the
// translation src->vt[284](&v, src) (when src is set) and passed on to
// (o->+440)->vt[84](it, o + 448) when +440 is set. r3 / f1 as the
// original leaves them.
DECOMP(000F4094, Obj_SetPivot_000F4094) {
    const u32 o = arg<u32>(ctx, 0), src = arg<u32>(ctx, 1);
    ctx.fpr[1] = Lfs40(o + 656);
    if (!(Lfs40(o + 656) < Lfs40(o + 672))) {
        ret(ctx, src);
        return;
    }
    constexpr u32 I = 0x01164BD0u;
    u32 m[9];  // the rows are read before the virtual call
    for (u32 r = 0; r < 3; ++r)
        for (u32 k = 0; k < 3; ++k) m[3 * r + k] = load<u32>(I + 16 * r + 4 * k);
    u32 t[3] = {load<u32>(I + 48), load<u32>(I + 52), load<u32>(I + 56)};
    if (src != 0) {
        Stack fr(ctx, 16);  // the vector at sp+112
        VCall(ctx, src, 284, {fr.addr(), src});
        for (u32 k = 0; k < 3; ++k) t[k] = load<u32>(fr.addr() + 4 * k);
    }
    for (u32 r = 0; r < 3; ++r)
        for (u32 k = 0; k < 3; ++k) store<u32>(o + 448 + 16 * r + 4 * k, m[3 * r + k]);
    const u32 q = load<u32>(o + 440);
    for (u32 k = 0; k < 3; ++k) store<u32>(o + 496 + 4 * k, t[k]);
    ctx.fpr[1] = static_cast<f64>(std::bit_cast<f32>(t[0]));
    if (q == 0) {
        ret(ctx, 0u);
        return;
    }
    VCall(ctx, q, 84, {q, o + 448});
}

// u32 f(T* o): unless bit 5 of u8 +572 is set: once f32 +564 reaches
// 0x001BF8E0 the state +556 becomes 1 (+568 = +564, o->vt[420](o, 1, 0));
// otherwise the state 1 becomes 0 and 0 becomes 1 (others return). Then
// bit 7 of +572 copies into bit 5, and the event (24 bytes on the stack,
// EventInit_0046528C(&ev, o, 0, 0) for state 1, else EventInit_00465308)
// is queried in the world *0x010B19E0 (World_Query482D14(w, o->+560,
// &ev)) and bit 3 of +572 set. r3 / f1 as the original leaves them.
DECOMP(001BF8E8, Obj_StateEvent_001BF8E8) {
    const u32 o = arg<u32>(ctx, 0);
    u32 b = load<u8>(o + 572);
    if ((b & 0x20u) != 0) {
        ret(ctx, b);
        return;
    }
    const u32 fw = load<u32>(o + 564);
    ctx.fpr[1] = static_cast<f64>(std::bit_cast<f32>(fw));
    u32 st;
    if (static_cast<f64>(std::bit_cast<f32>(fw)) >= Lfs40(0x001BF8E0u)) {
        store<u32>(o + 556, 1);
        store<u32>(o + 568, fw);
        VCall(ctx, o, 420, {o, 1, 0});
        b = load<u8>(o + 572);
        st = load<u32>(o + 556);
    } else {
        const s32 s = static_cast<s32>(load<u32>(o + 556));
        if (s > 1 || s < 0) {
            ret(ctx, b);
            return;
        }
        st = s == 1 ? 0u : 1u;
        store<u32>(o + 556, st);
    }
    if ((b & 0x80u) != 0) store<u8>(o + 572, static_cast<u8>(b | 0x20u));
    const u32 w = load<u32>(0x010B19E0u);
    Stack fr(ctx, 32);  // the event at sp+112 .. sp+135
    for (u32 k = 0; k < 24; k += 4) store<u32>(fr.addr() + k, 0);
    call(ctx, st == 1 ? 0x0046528Cu : 0x00465308u, {fr.addr(), o, 0, 0});
    call(ctx, 0x00482D14u, {w, load<u32>(o + 560), fr.addr()});
    const u32 b2 = load<u8>(o + 572) | 0x08u;
    store<u8>(o + 572, static_cast<u8>(b2));
    ret(ctx, b2);
}

// T* f(a, b, c, d): a 320-byte spawn query (SmallAlloc(320); 0 on
// failure) built by Wrapper_0CC4D4(p, c, d, 0, 0x01164BD0, 0x01164B30, 1,
// 0, 0, -, 0) with f1 = 0x001F60D0 (args 9 and 11 on the stack), then
// vtable 0x00B188F0, +256..+264 = 0x01164B40, +272..+284 = 0x01164C80,
// +236 / +240 / +288..+300 = 0, u8 +304 = 0, bit 2 of +4 and bit 4 of +12
// set; XmlDef_Read_001F5E50(a, b, p) either way. r3 = p.
DECOMP(001F60D8, SpawnQuery_New_001F60D8) {
    const u32 a = arg<u32>(ctx, 0), b = arg<u32>(ctx, 1), c = arg<u32>(ctx, 2), d = arg<u32>(ctx, 3);
    const u32 p = static_cast<u32>(call(ctx, 0x0055238Cu, {320}));
    if (p != 0) {
        // arg 10 (sp+120) is left unwritten by the original; the callee reads only args 9 and 11
        callf(ctx, 0x000CC4D4u, {p, c, d, 0, 0x01164BD0u, 0x01164B30u, 1, 0, 0, 0, 0}, {Lfs40(0x001F60D0u)});
        store<u32>(p + 256, load<u32>(0x01164B40u));
        const u32 w4 = load<u32>(p + 4);
        store<u32>(p + 236, 0);
        store<u32>(p + 260, load<u32>(0x01164B44u));
        store<u32>(p + 4, w4 | 4u);
        store<u32>(p + 264, load<u32>(0x01164B48u));
        const u32 f272 = load<u32>(0x01164C80u);
        store<u32>(p + 240, 0);
        store<u32>(p + 272, f272);
        store<u32>(p, 0x00B188F0u);
        store<u32>(p + 276, load<u32>(0x01164C84u));
        const u32 f280 = load<u32>(0x01164C88u);
        const u32 w12 = load<u32>(p + 12);
        store<u32>(p + 280, f280);
        const u32 f284 = load<u32>(0x01164C8Cu);
        store<u32>(p + 288, 0);
        store<u32>(p + 284, f284);
        store<u32>(p + 292, 0);
        store<u32>(p + 296, 0);
        store<u32>(p + 300, 0);
        store<u8>(p + 304, 0);
        store<u32>(p + 12, w12 | 0x10u);
        ctx.fpr[1] = static_cast<f64>(load<f32>(0x01164C8Cu));
    }
    call(ctx, 0x001F5E50u, {a, b, p});
    ret(ctx, p);
}

// T* f(a, b, Src* s, c): DefNewSmall41 (3.0f, 0, +32 = 1 kept over the
// read, name 0x00AD37F8; reader Def_ReadBase20BFE8; vtable 0x00B1A9F0).
DECOMP(002203F0, XmlDef_NewB1A9F0_002203F0) { DefNewSmall41(ctx, 0x0020BFE8u, 0x40400000u, 0, 1, true, 0x00AD37F8u, 0x00B1A9F0u); }

// T* f(a, b, Src* s, c): DefNewSmall41 (0x00400000, 32, name 0x00AD3838;
// reader Def_ReadBase20BFE8; vtable 0x00B1AB78).
DECOMP(002210C8, XmlDef_NewB1AB78_002210C8) { DefNewSmall41(ctx, 0x0020BFE8u, 0x00400000u, 32, 0, false, 0x00AD3838u, 0x00B1AB78u); }

// T* f(a, b, Src* s, c): DefNewSmall41 (0x00400000, 128, name 0x00AD38DC;
// reader Xml_Load_0022515C; vtable 0x00B1B150).
DECOMP(002251DC, XmlDef_NewB1B150_002251DC) { DefNewSmall41(ctx, 0x0022515Cu, 0x00400000u, 128, 0, false, 0x00AD38DCu, 0x00B1B150u); }

// u32 f(Obj* o, Msg* m): unless the global flag u8 0x010388F0 is set, bits
// 0x20000 / 0x40000 of o->+188 are set, o's owner (Obj_Owner0D93FC) has
// bit 12 of +40 set or f32 o->+380 > 0x0022948C: when Obj_TestE0BCC(o, m)
// and the reach (o->+216)->+228 (for m->+8 == 0x04E8CCF7, else +220) is
// above that limit, o->vt[352](o, m->+4, m->+8 or 0x811B1D5E) with f1 =
// m->+32 * reach (the OPD in r4, the entry in r7); bit 8 of u16 m->+14 =
// whether it returned 0. r3 / f1 as the original leaves them.
DECOMP(00229490, Obj_TryReact_00229490) {
    const u32 o = arg<u32>(ctx, 0), m = arg<u32>(ctx, 1);
    const u32 g = load<u8>(0x010388F0u);
    if (g != 0) {
        ret(ctx, g);
        return;
    }
    const u64 d = load<u64>(o + 184);
    if ((d & 0x20000u) != 0) {
        ctx.gpr[3] = d;
        return;
    }
    if ((d & 0x40000u) != 0) {
        ret(ctx, static_cast<u32>(d) & 0x40000u);
        return;
    }
    if (call(ctx, 0x000D93FCu, {o}) != 0) {
        const u32 w = static_cast<u32>(call(ctx, 0x000D93FCu, {o}));
        if ((load<u32>(w + 40) & 0x1000u) != 0) {
            ret(ctx, 1u);
            return;
        }
    }
    const f64 lim = Lfs40(0x0022948Cu);
    const f64 f = Lfs40(o + 380);
    ctx.fpr[1] = f;
    if (f > lim) {
        ret(ctx, 0x0022948Cu);
        return;
    }
    const u32 r = static_cast<u32>(call(ctx, 0x000E0BCCu, {o, m}));
    const u32 vt = load<u32>(o);
    ctx.gpr[3] = vt;
    if (r == 0) return;
    const u32 k = load<u32>(m + 8);
    const u32 q = load<u32>(o + 216);
    const f64 reach = Lfs40(q + (k == 0x04E8CCF7u ? 228 : 220));
    ctx.fpr[1] = reach;
    if (!(reach > lim)) return;
    const u32 h = k != 0 ? k : 0x811B1D5Eu;
    const u32 opd = load<u32>(vt + 352);
    ctx.fpr[1] = F40(Lfs40(m + 32) * reach);
    const u32 res = static_cast<u32>(call_ptr(ctx, opd, {o, opd, load<u32>(m + 4), h, load<u32>(opd)}));
    const u32 z = std::countl_zero(res);
    store<u16>(m + 14, static_cast<u16>((load<u16>(m + 14) & ~0x100u) | ((z << 3) & 0x100u)));
    ret(ctx, z);
}

// u32 f(T* o): +780 = 0; with bit 31 of +384: the flag f = bit 12 of
// *((o->+952)->vt[12](it) + 36); with the holder h = o->+948: the first
// target of CallK2_000D93D4(o) (Target_Next271C08 walk) other than o is
// kept in +780, bit 30 of h->+1268 = f and Obj_SetTarget_00296758(h,
// +780, 0); the effect +748 then switches to the instance +752 (f) / +756
// (Effect_SetInstance_004CF330) unless it already has it (+4). r3 as the
// original leaves it.
DECOMP(0023B870, Obj_PickTarget_0023B870) {
    const u32 o = arg<u32>(ctx, 0);
    store<u32>(o + 780, 0);
    if ((load<u32>(o + 384) & 0x80000000u) == 0) {
        ret(ctx, 0u);
        return;
    }
    const u32 a = load<u32>(o + 952);
    const u32 r = static_cast<u32>(VCall(ctx, a, 12, {a}));
    const u32 f = load<u32>(r + 36) & 0x1000u;
    if (load<u32>(o + 948) != 0) {
        const u32 t = static_cast<u32>(call(ctx, 0x000D93D4u, {o}));
        if (t != 0) {
            for (u32 n = static_cast<u32>(call(ctx, 0x00271C08u, {t, 0, 0})); n != 0; n = static_cast<u32>(call(ctx, 0x00271C08u, {t, 0, n}))) {
                if (n != o) {
                    store<u32>(o + 780, n);
                    break;
                }
            }
        }
        const u32 h = load<u32>(o + 948);
        store<u32>(h + 1268, (load<u32>(h + 1268) & ~0x40000000u) | (f != 0 ? 0x40000000u : 0u));
        call(ctx, 0x00296758u, {load<u32>(o + 948), load<u32>(o + 780), 0});
    }
    const u32 e = load<u32>(o + 748);
    if (e == 0) {
        ret(ctx, 0u);
        return;
    }
    const u32 v = load<u32>(o + (f != 0 ? 752 : 756));
    if (v == load<u32>(e + 4)) {
        ret(ctx, e);
        return;
    }
    call(ctx, 0x004CF330u, {e, v});
}

// u32 f(T* o): once the timer f32 o->+992 reaches (o->+396)->+1588: the
// effect o->+764 (when set) is spawned (SmallAlloc(140) +
// Effect_Construct; null on failure, quirk), its component 0x0027469E of
// kind 5 (vt[32]) takes the reference o->+160 (+28; the old one gets
// vt[8], the new one's u16 +4 counted up) and it is started (+136 |=
// 0x200, Effect_Start); +992 = 0. r3 / f1 as the original leaves them.
DECOMP(0023BCA0, Obj_TimedEffect_0023BCA0) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 p = load<u32>(o + 396) + 1584;
    const f64 t = Lfs40(o + 992);
    ctx.fpr[1] = t;
    if (!(Lfs40(p + 4) <= t)) {
        ret(ctx, p);
        return;
    }
    if (load<u32>(o + 764) != 0) {
        const u32 s = static_cast<u32>(call(ctx, 0x0055238Cu, {140}));
        u32 e = 0;
        if (s != 0) {
            call(ctx, 0x004C7A68u, {s, load<u32>(o + 764)});
            e = s;
        }
        const u32 c = static_cast<u32>(call(ctx, 0x004BB4B4u, {e, 0x0027469Eu}));
        if (c != 0 && static_cast<u32>(VCall(ctx, c, 32, {c})) == 5) {
            const u32 old = load<u32>(c + 28);
            const u32 nw = load<u32>(o + 160);
            if (old != 0) VCall(ctx, old, 8, {old});
            store<u32>(c + 28, nw);
            if (nw != 0) store<u16>(nw + 4, static_cast<u16>(load<u16>(nw + 4) + 1));
        }
        store<u32>(e + 136, load<u32>(e + 136) | 0x200u);
        call(ctx, 0x004C753Cu, {e});
    }
    store<u32>(o + 992, 0);
    ret(ctx, 0u);
}

// Deleting destructor (o = r3; r3 = 0 for o == 0): vtable 0x00B1C0D0,
// List1013B5C_Remove(o); the reference +92 gets vt[8] and is cleared;
// vtable 0x00B1BE30, Unlist1056F1C39(o), List1013B5C_Remove(o) and
// SmallFree(o, 128) (r3 = its).
DECOMP(0027BC64, Obj_DeleteB1C0D0_0027BC64) {
    const u32 o = arg<u32>(ctx, 0);
    if (o == 0) {
        ret(ctx, 0u);
        return;
    }
    store<u32>(o, 0x00B1C0D0u);
    call(ctx, 0x000D6940u, {o});
    const u32 c = load<u32>(o + 92);
    if (c != 0) {
        VCall(ctx, c, 8, {c});
        store<u32>(o + 92, 0);
    }
    store<u32>(o, 0x00B1BE30u);
    Unlist1056F1C39(o);
    call(ctx, 0x000D6940u, {o});
    call(ctx, 0x00552420u, {o, 128});
}

// u32 f(T* o, Msg* m): for m->+8 == 0x69988172 while bit 30 of +92 is
// clear: a key m->+0 matching the next expected step (the 256-byte entry
// +64 of o->+68, its +208; +64 below the count +36) advances +64 (bit 31
// of +92 set at the end); otherwise, unless +56 is 2, a key found among
// the 240-byte records (+192) of the list (o->+8)->+340 (made on first use:
// SmallAlloc(20), PtrVector_Construct, u16 +6 = 240; read through null on
// failure, quirk) sets bit 30 and clears bit 31 of +92. r3 as the original
// leaves it.
DECOMP(002D4030, Seq_Step_002D4030) {
    const u32 o = arg<u32>(ctx, 0), m = arg<u32>(ctx, 1);
    if (load<u32>(m + 8) != 0x69988172u) {
        ret(ctx, 0x69988172u);
        return;
    }
    const u32 w = load<u32>(o + 92);
    if ((w & 0x40000000u) != 0) {
        ret(ctx, 1u);
        return;
    }
    const s32 i = static_cast<s32>(load<u32>(o + 64)), n = static_cast<s32>(load<u32>(o + 36));
    if (i < n && load<u32>(m) == load<u32>(load<u32>(o + 68) + ((static_cast<u32>(i) << 8) & 0xFFFFFF00u) + 208)) {
        const s32 i1 = i + 1;
        store<u32>(o + 64, static_cast<u32>(i1));
        if (n != i1) {
            ret(ctx, static_cast<u32>(n));
            return;
        }
        store<u32>(o + 92, w | 0x80000000u);
        ret(ctx, w | 0x80000000u);
        return;
    }
    if (load<u32>(o + 56) == 2) {
        ret(ctx, 2u);
        return;
    }
    const u32 q = load<u32>(o + 8) + 340;
    u32 v = load<u32>(q);
    if (v == 0) {
        v = static_cast<u32>(call(ctx, 0x0055238Cu, {20}));
        store<u32>(q, v);
        if (v != 0) {
            call(ctx, 0x005516E0u, {v});
            store<u16>(v + 6, 240);
            v = load<u32>(q);
        }
    }
    u32 off = 0;
    for (u32 j = 0; j < load<u16>(v + 8); ++j, off += 240) {
        if (load<u32>(m) == load<u32>(load<u32>(v) + off + 192)) {
            const u32 x = (load<u32>(o + 92) | 0x40000000u) & 0x7FFFFFFFu;
            store<u32>(o + 92, x);
            ret(ctx, x);
            return;
        }
    }
    ret(ctx, off);
}

// T::T(o): u8 +404 = 0; the vectors +408 (element 4, u16 +414 = 129), +428
// (16, +434 = 76, flag 1) and +448 (8, +454 = 12) are constructed and set
// up (PtrVector_Construct, PtrVector_Init(v, size, 0, flag)); +468..+484 =
// 0, +488 = 0xD433715A, +492 = 0, +496 = -1.0, u8 +500..+506 = 0 except
// +505 = 1, +508 / +512 = 0, +516 = Mem_DefaultHeap(), +520..+532 = 0 and
// the first bytes of the strings +0 / +65 / +130 / +259 / +388 = 0. r3 =
// the heap.
DECOMP(002D9740, Obj_CtorTables_002D9740) {
    const u32 o = arg<u32>(ctx, 0);
    store<u8>(o + 404, 0);
    const u32 vs[3][4] = {{408, 129, 4, 0}, {428, 76, 16, 1}, {448, 12, 8, 0}};
    for (const auto& e : vs) {
        call(ctx, 0x005516E0u, {o + e[0]});
        store<u16>(o + e[0] + 6, static_cast<u16>(e[1]));
        call(ctx, 0x00551598u, {o + e[0], e[2], 0, e[3]});
    }
    for (u32 k = 468; k <= 484; k += 4) store<u32>(o + k, 0);
    store<u32>(o + 488, 0xD433715Au);
    store<u32>(o + 492, 0);
    store<u32>(o + 496, 0xBF800000u);
    for (u32 k = 500; k <= 506; ++k) store<u8>(o + k, k == 505 ? 1 : 0);
    store<u32>(o + 508, 0);
    store<u32>(o + 512, 0);
    const u32 h = static_cast<u32>(call(ctx, 0x00550E70u, {}));
    store<u32>(o + 516, h);
    for (u32 k = 520; k <= 532; k += 4) store<u32>(o + k, 0);
    for (u32 k : {0u, 65u, 130u, 259u, 388u}) store<u8>(o + k, 0);
    ret(ctx, h);
}

// u32 f(T* o, a): with u8 o->+18 == 4 and Obj_CanShow2E2104(*0x00F64C20,
// o): each active entry e (u8 +12) of the 20-byte array (o->+2272)->+0
// (u16 count +8, re-read each step) gets u8 +11 = 0,
// sub_00321324(e, 1) and sub_003218B0(e, a, 1); then each child c of the
// chain o->+2576 (next c->+2564) gets c->vt[36](c, o). r3 as the original
// leaves it.
DECOMP(00327684, Obj_RefreshSlots_00327684) {
    const u32 o = arg<u32>(ctx, 0), a = arg<u32>(ctx, 1);
    u64 r3 = load<u8>(o + 18);
    if (r3 == 4) {
        r3 = static_cast<u32>(call(ctx, 0x002E2104u, {load<u32>(0x00F64C20u), o}));
        if (r3 != 0) {
            for (u32 i = 0, off = 0;; ++i, off += 20) {
                const u32 v = load<u32>(o + 2272);
                const u32 n = v != 0 ? load<u16>(v + 8) : 0u;
                if (i >= n) {
                    r3 = n;
                    break;
                }
                const u32 e = load<u32>(v) + off;
                if (load<u8>(e + 12) == 0) continue;
                store<u8>(e + 11, 0);
                call(ctx, 0x00321324u, {e, 1});
                call(ctx, 0x003218B0u, {e, a, 1});
            }
        }
    }
    for (u32 c = load<u32>(o + 2576); c != 0; c = load<u32>(c + 2564)) r3 = VCall(ctx, c, 36, {c, o});
    ctx.gpr[3] = r3;
}

// void f(): registers the nine 'KH' chunk handlers 'FSKH' / 'HSKH' /
// 'DSKH' / 'FMKH' / 'HMKH' / 'DMKH' / 'FAKH' / 'HAKH' / 'DAKH'
// (Iff_RegisterChunk(id, 0x00F373E8 / ... / 0x00F373E0, 0, 0, 0)). r3 as
// the last call leaves it.
DECOMP(003489F4, Iff_RegisterKH_003489F4) {
    static constexpr u32 T[9][2] = {
        {0x46534B48u, 0x00F373E8u}, {0x48534B48u, 0x00F373F0u}, {0x44534B48u, 0x00F373F8u},
        {0x464D4B48u, 0x00F37400u}, {0x484D4B48u, 0x00F37408u}, {0x444D4B48u, 0x00F37410u},
        {0x46414B48u, 0x00F373D0u}, {0x48414B48u, 0x00F373D8u}, {0x44414B48u, 0x00F373E0u},
    };
    for (const auto& e : T) call(ctx, 0x0055902Cu, {e[0], e[1], 0, 0, 0});
}

// u32 f(T* o, u8 on): (o->+24)->vt[40](it, on, 1) when set; each part e of
// the 28-byte array (o->+8)->+180 (count +184, e at +12) gets
// e->vt[88](e, on); turning off also sends vt[40](x, 0, 1) to the six
// slots +280..+300 that are set. r3 as the original leaves it.
DECOMP(0037ECC0, Obj_SetVisible_0037ECC0) {
    const u32 o = arg<u32>(ctx, 0), on = arg<u32>(ctx, 1) & 0xFFu;
    const u32 c = load<u32>(o + 24);
    if (c != 0) VCall(ctx, c, 40, {c, on, 1});
    const u32 s = load<u32>(o + 8);
    u64 r3 = load<u32>(s + 184);
    for (u32 i = 0; i < static_cast<u32>(r3); ++i) {
        const u32 e = load<u32>(load<u32>(s + 180) + 28 * i + 12);
        if (e == 0) continue;
        VCall(ctx, e, 88, {e, on});
        r3 = load<u32>(s + 184);
    }
    if (on == 0) {
        for (u32 j = 0; j < 6; ++j) {
            const u32 x = load<u32>(o + 280 + 4 * j);
            r3 = x;
            if (x != 0) r3 = VCall(ctx, x, 40, {x, 0, 1});
        }
    }
    ctx.gpr[3] = r3;
}

// bool f(T* o, ?, u8 b, c, f32 x): 0 when the state o->vt[32](o, 0) is 7,
// or it is 3 and the track o->+96 is past both limits (time / length
// (Anim_TrackTime_000DC488 / Anim_TrackLength164_000DC418) >= f32
// 0x01086708 and >= 0x01086758); otherwise f32 +128 = x, u8 +159 = b,
// o->vt[28](o, 0, 8), +116 = c and 1. f1 as the original leaves it.
DECOMP(003AE8B0, Obj_StartState_003AE8B0) {
    const u32 o = arg<u32>(ctx, 0), b = arg<u32>(ctx, 2), c = arg<u32>(ctx, 3);
    const f64 x = farg(ctx, 0);
    if (static_cast<u32>(VCall(ctx, o, 32, {o, 0})) == 7) {
        ret(ctx, 0u);
        return;
    }
    f64 f1 = x;
    if (static_cast<u32>(VCall(ctx, o, 32, {o, 0})) == 3) {
        call(ctx, 0x000DC488u, {load<u32>(o + 96), 0, 0});
        const f64 t = ctx.fpr[1];
        call(ctx, 0x000DC418u, {load<u32>(o + 96), 0});
        f1 = F40(t / ctx.fpr[1]);
        if (f1 >= Lfs40(0x01086708u) && f1 >= Lfs40(0x01086758u)) {
            ctx.fpr[1] = f1;
            ret(ctx, 0u);
            return;
        }
    }
    store<u32>(o + 128, W41(x));
    store<u8>(o + 159, static_cast<u8>(b));
    ctx.fpr[1] = f1;
    VCall(ctx, o, 28, {o, 0, 8});
    store<u32>(o + 116, c);
    ret(ctx, 1u);
}

// void f(Cmd* o): the object Obj_FindByHandle42C2B0(o->+12, bit 27 of
// +20), narrowed by Obj_FindByHash_000DB290(it, o->+16) when +16 is set,
// gets vt[88](it, bit 31, bit 30, bit 29, bit 28 of +20) (the entry in
// r8, the OPD in r9); a missing one formats the warning 0x00AE2678 with
// the three names of +12, +16 and *(o->+4) unless bit 27 is set. Then
// Flag_Set8_40000000(o) (r3 = its).
DECOMP(00438DDC, ObjCmd_Apply_00438DDC) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 w = load<u32>(o + 20);
    u32 t = static_cast<u32>(call(ctx, 0x0042C2B0u, {load<u32>(o + 12), (w & 0x08000000u) != 0 ? 1u : 0u}));
    if (t != 0 && load<u32>(o + 16) != 0) t = static_cast<u32>(call(ctx, 0x000DB290u, {t, load<u32>(o + 16)}));
    if (t == 0) {
        if ((load<u32>(o + 20) & 0x08000000u) == 0) {
            const u32 s1 = static_cast<u32>(call(ctx, 0x0056A6E0u, {load<u32>(o + 12)}));
            const u32 s2 = static_cast<u32>(call(ctx, 0x0056A6E0u, {load<u32>(o + 16)}));
            const u32 s3 = static_cast<u32>(call(ctx, 0x0056A6E0u, {load<u32>(load<u32>(o + 4))}));
            call(ctx, 0x005534D0u, {0x00AE2678u, s1, s2, s3});
        }
    } else {
        const u32 f = load<u32>(o + 20);
        auto bit = [f](u32 m) -> u64 { return (f & m) != 0 ? 1u : 0u; };
        VCallEO36(ctx, load<u32>(t), 88, {t, bit(0x80000000u), bit(0x40000000u), bit(0x20000000u), bit(0x10000000u)});
    }
    call(ctx, 0x0042CA60u, {o});
}

// u32 f(T* o, u8 hide): for each group p of the chain o->+32 (next +72)
// whose flag byte (p->+48)->+12[4 * (*0x010AB1B8)->+64] is set: the count
// +24 is taken (cleared) and each member e of p->+20 with bit 18 of
// e->+172 gets e->vt[88](e, 0, 1, 1, 1) (with hide), p->+32 counted down
// and e->vt[40](e, 1). r3 as the original leaves it.
DECOMP(004563A0, Group_Release_004563A0) {
    const u32 hide = arg<u32>(ctx, 1) & 0xFFu;
    u64 r3 = ctx.gpr[3];
    for (u32 p = load<u32>(arg<u32>(ctx, 0) + 32); p != 0; p = load<u32>(p + 72)) {
        r3 = load<u8>(load<u32>(load<u32>(p + 48) + 12) + (load<u32>(load<u32>(0x010AB1B8u) + 64) << 2));
        if (r3 == 0) continue;
        const s32 n = static_cast<s32>(load<u32>(p + 24));
        r3 = 0;
        store<u32>(p + 24, 0);
        for (s32 i = 0; i < n; ++i) {
            const u32 e = load<u32>(load<u32>(p + 20) + 4 * static_cast<u32>(i));
            r3 = (load<u32>(e + 172) >> 18) & 1u;
            if (r3 == 0) continue;
            if (hide != 0) VCall(ctx, e, 88, {e, 0, 1, 1, 1});
            store<u32>(p + 32, load<u32>(p + 32) - 1);
            r3 = VCall(ctx, e, 40, {e, 1});
        }
    }
    ctx.gpr[3] = r3;
}
