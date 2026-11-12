// Game code, part 53: the small-function sweep finished with the last 119
// instruction functions (size order). Each was checked against the
// original in the PPC simulator (memory writes, calls and their arguments,
// r3 and f1).
#include "obj53.h"

using namespace sly4;
using namespace sly4::obj53;

// ---------------------------------------------------------------- sweep: 119 instructions
// u32 f(Obj* o, a, b): when bit 0x8 of the u64 o->+184 is set and
// Ptr_DerefNonZero(o->+944): Query_Run_000DB94C(&tmp (sp+128), o, o->+944,
// b, 0, 1, 1, 127 * o->+448, 0), then one step of the inlined random
// generator (0x01164888 / 0x01164890) whose old seed picks the column:
// o->+448 += s8 table 0x00ACAB30[seed % 6 + 6 * s8 0x00ACAB66[o->+448]];
// r3 = the new +448. Otherwise, when o->vt[32](o, 0) == 0 and
// Ptr_DerefNonZero(o->+940): Query_Run_000DB94C(sp+132, o, o->+940, b, 0, 1,
// 1, 0, 0) first; then the u64 o->+184 |= (a ? 0x20000000 : 0x10000000), r3
// = its new value.
DECOMP(0012BD94, Obj_StepQueryState_0012BD94) {
    const u32 o = arg<u32>(ctx, 0), a = arg<u32>(ctx, 1), b = arg<u32>(ctx, 2);
    Stack fr(ctx, 32);  // the query results at sp+128 / sp+132
    if ((static_cast<u32>(load<u64>(o + 184)) & 0x8u) != 0 && call(ctx, 0x00446960u, {load<u32>(o + 944)}) != 0) {
        const u32 h = load<u32>(o + 448) * 127u;
        call(ctx, 0x000DB94Cu, {fr.addr() + 16, o, load<u32>(o + 944), b, 0, 1, 1, static_cast<u64>(static_cast<s64>(static_cast<s32>(h))), 0});
        const s32 st = static_cast<s32>(load<u32>(o + 448));
        const u32 seed = load<u32>(0x01164890u);
        const u32 row = static_cast<u32>(static_cast<s32>(static_cast<s8>(load<u8>(0x00ACAB66u + st))));
        const u32 x = (std::rotl(seed, 4) + std::rotl(seed, 16)) ^ 0x87654321u;
        store<u32>(0x01164890u, std::rotl(x, 16));
        store<u64>(0x01164888u, x & 0xFFFFu);
        const s32 d = static_cast<s8>(load<u8>(0x00ACAB30u + seed % 6 + row * 6));
        const u32 v = static_cast<u32>(st + d);
        store<u32>(o + 448, v);
        ret(ctx, v);
        return;
    }
    if (VCall(ctx, o, 32, {o, 0}) == 0 && call(ctx, 0x00446960u, {load<u32>(o + 940)}) != 0)
        call(ctx, 0x000DB94Cu, {fr.addr() + 20, o, load<u32>(o + 940), b, 0, 1, 1, 0, 0});
    const u64 v = load<u64>(o + 184) | (a != 0 ? 0x20000000u : 0x10000000u);
    store<u64>(o + 184, v);
    ctx.gpr[3] = v;
}

// T* f(a, b, c, Src* d): a 272-byte object (SmallAlloc; 0 when that
// fails): Wrapper_0CC4D4(p, c, d, 0, 0x01164BD0, 0x01164B30, 1, 0, 0, -, 0)
// with f1 = the f32 at 0x001B55C0, vtable 0x00B16088, the 8-byte entry
// vector at +236 (PtrVector_Construct, u16 +242 = 8, PtrVector_Init(v, 4,
// 0, 1)), f32 +256 = d->+232 (no d: the f32 at 0x001B55C4), +8 |= 0x20000;
// then the u16 d->+200 8-byte points at d->+192 are pushed (d is read even
// when 0). Either way XmlDef_ReadPoints_001B5438(a, b, p) last; r3 = p (f1
// as the copies leave it).
DECOMP(001B55C8, PointsDef_New_001B55C8) {
    const u32 a = arg<u32>(ctx, 0), b = arg<u32>(ctx, 1), c = arg<u32>(ctx, 2), d = arg<u32>(ctx, 3);
    const u32 p = static_cast<u32>(call(ctx, 0x0055238Cu, {272}));
    if (p != 0) {
        callf(ctx, 0x000CC4D4u, {p, c, d, 0, 0x01164BD0u, 0x01164B30u, 1, 0, 0, 0, 0}, {Lfs53(0x001B55C0u)});
        const u32 v = p + 236;
        store<u32>(p, 0x00B16088u);
        call(ctx, 0x005516E0u, {v});
        store<u16>(p + 242, 8);
        call(ctx, 0x00551598u, {v, 4, 0, 1});
        const u32 f = d != 0 ? load<u32>(d + 232) : load<u32>(0x001B55C4u);
        const u32 fl = load<u32>(p + 8) | 0x20000u;
        store<u32>(p + 256, f);
        store<u32>(p + 8, fl);
        u32 last = f;  // f1 as the copies leave it
        for (u32 i = 0; i < load<u16>(d + 200); ++i) {
            const u32 src = load<u32>(d + 192) + 8 * i;
            Push8_53(ctx, v, src);
            last = load<u32>(src + 4);
        }
        ctx.fpr[1] = static_cast<f64>(std::bit_cast<f32>(last));
    }
    call(ctx, 0x001B5438u, {a, b, p});
    ret(ctx, p);
}

// Def* f(a0, XmlCursor* c, Src* x, tmpl): a default record on the stack
// (sp+116: 0x00400000, +4 = 16, +12 = x->+12, +16 / +20 = 0x00AD366C, +32 =
// 1, f32 +36 / +40 / +72 = -1, +76 the name 0x00AD37A8, +80 = 1, +84 = -1,
// the rest 0) read by Def_ReadBase20BFE8(a0, c, x, tmpl, rec, 0x00400000,
// -1, 16); +32 = 1 again; +84 = the index of the hash 0x51609AAA (default
// 0) in the 7-entry table 0x00AD3FE4 (else -1). A 192-byte def (SmallAlloc;
// 0 when that fails): Thunk_000C2B90(p, rec), bytes +172..+175 = rec
// +80..+83, vtable 0x00B19510, String_CopyN(p + 140, rec +76, 31), +176 =
// rec +84, vtable 0x00B1A5A0. Returns the def.
DECOMP(0021D3CC, JumpDef_New_0021D3CC) {
    const u32 a0 = arg<u32>(ctx, 0), c = arg<u32>(ctx, 1), x = arg<u32>(ctx, 2), tmpl = arg<u32>(ctx, 3);
    Stack fr(ctx, 96);
    const u32 tmp = fr.addr(), t = tmp + 4;  // sp+112 / sp+116
    store<u32>(t + 16, 0x00AD366Cu);
    store<u32>(t + 12, load<u32>(x + 12));
    store<u32>(t + 20, 0x00AD366Cu);
    for (u32 k = 24; k < 32; ++k) store<u8>(t + k, 0);
    store<u32>(t + 36, 0xBF800000u);
    store<u32>(t + 40, 0xBF800000u);
    store<u8>(t + 44, 0);
    for (u32 k = 48; k < 72; k += 4) store<u32>(t + k, 0);
    store<u32>(t + 72, 0xBF800000u);
    store<u32>(t + 8, 0);
    store<u32>(t + 76, 0x00AD37A8u);
    store<u8>(t + 80, 1);
    store<u8>(t + 81, 0);
    store<u8>(t + 82, 0);
    store<u8>(t + 83, 0);
    store<u32>(t, 0x00400000u);
    store<u32>(t + 84, 0xFFFFFFFFu);
    store<u32>(t + 4, 16);
    store<u32>(t + 32, 1);
    call(ctx, 0x0020BFE8u, {a0, c, x, tmpl, t, 0x00400000u, 0xFFFFFFFFFFFFFFFFull, 16});
    store<u32>(t + 32, 1);
    call(ctx, 0x004FA14Cu, {c, 0x51609AAAu, tmp, 0});
    for (u32 i = 0; i < 7; ++i)
        if (load<u32>(tmp) == load<u32>(0x00AD3FE4u + 4 * i)) {
            store<u32>(t + 84, i);
            break;
        }
    const u32 p = static_cast<u32>(call(ctx, 0x0055238Cu, {192}));
    if (p == 0) {
        ret(ctx, 0u);
        return;
    }
    call(ctx, 0x000C2B90u, {p, t});
    store<u8>(p + 172, load<u8>(t + 80));
    store<u8>(p + 173, load<u8>(t + 81));
    store<u32>(p, 0x00B19510u);
    store<u8>(p + 174, load<u8>(t + 82));
    store<u8>(p + 175, load<u8>(t + 83));
    call(ctx, 0x00553638u, {p + 140, load<u32>(t + 76), 31});
    store<u32>(p + 176, load<u32>(t + 84));
    store<u32>(p, 0x00B1A5A0u);
    ret(ctx, p);
}

// void f(T* o) (destructor body): vtable 0x00B1B710; AnimSkel_Release7D4EA8
// of +980 (then 0) when set; for each of the u16 +1212 32-byte groups at
// +1204, each of its u16 +8 352-byte entries (at +0) releases and clears
// its +80 the same way; +1524->vt[8]() (then 0) when set; PtrVector_Clear
// and Nop on +1392, +1204, +824; when +736 is set vtable 0x00B1B4C8 and
// Mem_FreeIfSet(+736, +732); vtable 0x00B11D98 and Obj_BaseDtor_000C2DA0
// (r3 = its result).
DECOMP(0022DD9C, AnimSet_Dtor_0022DD9C) {
    const u32 o = arg<u32>(ctx, 0);
    store<u32>(o, 0x00B1B710u);
    if (load<u32>(o + 980) != 0) {
        call(ctx, 0x007D4EA8u, {load<u32>(o + 980)});
        store<u32>(o + 980, 0);
    }
    const s32 n = load<u16>(o + 1212);
    for (s32 i = 0; i < n; ++i) {
        const u32 g = load<u32>(o + 1204) + 32 * static_cast<u32>(i);
        for (u32 j = 0; j < load<u16>(g + 8); ++j) {
            const u32 e = load<u32>(g) + 352 * j;
            if (load<u32>(e + 80) != 0) {
                call(ctx, 0x007D4EA8u, {load<u32>(e + 80)});
                store<u32>(e + 80, 0);
            }
        }
    }
    const u32 h = load<u32>(o + 1524);
    if (h != 0) {
        VCall(ctx, h, 8, {h});
        store<u32>(o + 1524, 0);
    }
    for (u32 off : {1392u, 1204u, 824u}) {
        call(ctx, 0x0054EC1Cu, {o + off});
        call(ctx, 0x0054EC18u, {o + off});
    }
    const u32 m = load<u32>(o + 736);
    if (m != 0) {
        store<u32>(o, 0x00B1B4C8u);
        call(ctx, 0x00555CC0u, {m, load<u32>(o + 732)});
    }
    store<u32>(o, 0x00B11D98u);
    call(ctx, 0x000C2DA0u, {o});
}

// u32 f(T* o): when the component at +24 reports state 1 or 2
// (+24->vt[32](+24, 0), asked twice when not 1) and the handle at
// (+276->vt[60]())->+32 resolves (HandleRec53) to a record whose object
// (+8) has a scope (+0) with Scope_Lookup_00760C54(scope, 0xC9C4A7A6) set:
// bytes +304 / +316 / +317 / +319 and words +308 / +312 = 0; when byte
// +320 is set it and +324 are cleared and Obj_ChargeShotFx_002C2420(o, 0);
// then +24->vt[28](+24, 0, 3) and r3 = 1. Otherwise r3 = 0.
DECOMP(002C4E9C, Obj_TryReleaseCharge_002C4E9C) {
    const u32 o = arg<u32>(ctx, 0), c = o + 24;
    if (VCall(ctx, c, 32, {c, 0}) != 1 && VCall(ctx, c, 32, {c, 0}) != 2) {
        ret(ctx, 0u);
        return;
    }
    const u32 q = load<u32>(o + 276);
    const u32 r = static_cast<u32>(VCall(ctx, q, 60, {q}));
    const u32 e = HandleRec53(load<u32>(r + 32));
    const u32 obj = e != 0 ? load<u32>(e + 8) : 0;
    const u32 sc = obj != 0 ? load<u32>(obj) : 0;
    if (sc == 0 || call(ctx, 0x00760C54u, {sc, 0xC9C4A7A6u}) == 0) {
        ret(ctx, 0u);
        return;
    }
    store<u8>(o + 304, 0);
    store<u32>(o + 308, 0);
    store<u32>(o + 312, 0);
    store<u8>(o + 316, 0);
    store<u8>(o + 317, 0);
    store<u8>(o + 319, 0);
    if (load<u8>(o + 320) != 0) {
        store<u8>(o + 320, 0);
        store<u32>(o + 324, 0);
        call(ctx, 0x002C2420u, {o, 0});
    }
    VCall(ctx, c, 28, {c, 0, 3});
    ret(ctx, 1u);
}

// void f(T* o, XmlCursor* c): o (a String at +0) = attribute 0x0024728B
// when present (String_Assign), s32s +32 = 0x0489115B and +36 = 0xBE8976CD
// (default 0); for each child 0x21E81828 a 60-byte item list (SmallAlloc;
// 0 when that fails: +32 = o, PtrVector at +36 with u16 +42 = 4, bytes
// +56..+58 = 0, memset(+0, 0, 32)) is read by Def_ReadItems_002F892C(p,
// child) and pushed onto the PtrVector +52. r3 = 0 (the last sibling
// lookup).
DECOMP(002F8AFC, ItemGroup_Read_002F8AFC) {
    const u32 o = arg<u32>(ctx, 0), c = arg<u32>(ctx, 1);
    Stack fr(ctx, 16);
    const u32 t = fr.addr();
    const u32 a = static_cast<u32>(call(ctx, 0x004F9534u, {c, 0x0024728Bu}));
    if (a != 0) call(ctx, 0x00555F80u, {o, a});
    store<u32>(t, 0);
    call(ctx, 0x004F99BCu, {c, 0x0489115Bu, t, 0});
    const u32 v32 = load<u32>(t);
    store<u32>(t, 0);
    store<u32>(o + 32, v32);
    call(ctx, 0x004F99BCu, {c, 0xBE8976CDu, t, 0});
    store<u32>(o + 36, load<u32>(t));
    u32 ch = static_cast<u32>(call(ctx, 0x004F92BCu, {c, 0x21E81828u}));
    while (ch != 0) {
        const u32 p = static_cast<u32>(call(ctx, 0x0055238Cu, {60}));
        if (p != 0) {
            store<u32>(p + 32, o);
            call(ctx, 0x005516E0u, {p + 36});
            store<u16>(p + 42, 4);
            store<u8>(p + 56, 0);
            store<u8>(p + 57, 0);
            store<u8>(p + 58, 0);
            call(ctx, 0x00726E54u, {p, 0, 32});
        }
        call(ctx, 0x002F892Cu, {p, ch});
        PtrVec_Push50(ctx, o + 52, p);
        ch = static_cast<u32>(call(ctx, 0x004F8FC8u, {ch, 0x21E81828u}));
    }
    ret(ctx, ch);
}

// u32 f(State* s) (state enter): s->+28 += 1, +36 = 0, byte +64 = 0; on
// the actor a = s->+32, unless its animation +668 is already 0xE8534CE4:
// f1 = the f32 0x00313F94 (byte a->+700 clear: the f32 at 0x01068FB8),
// a->vt[60](a) (f1 as chosen) and Anim_Restart2775A8(that, 0xE8534CE4, 0,
// 0, -1, r8 as left, 0, 0, -, -, 0, -1) with f1 = 0x00313F98 * the f32
// 0x01069008, f2 = (f1 != 0x00313F94) ? 1 : 0, f3 = f4 = 0x00313F94; then
// a->+700 = 0, +668 = 0xE8534CE4. Then (s->+32 reloaded) +1048 = 1.0,
// +1044 = +1052, +1028 = 0, Motion_Set4A0C20(a + 1024, 0, 0x00313F94) with
// f1 = (0x00313F98 - +1052) * 0x00313F9C; Anim_SetTarget_0030DBF0(a,
// 0xCB1675F3, r5 as left, 0) with f1 = 0x00313F94; r3 = s->vt[28](s, 0,
// 1).
DECOMP(00313FA0, State_Enter_00313FA0) {
    const u32 s = arg<u32>(ctx, 0);
    constexpr u32 K = 0x00313F94u;
    u32 a = load<u32>(s + 32);
    store<u32>(s + 28, load<u32>(s + 28) + 1);
    store<u32>(s + 36, 0);
    store<u8>(s + 64, 0);
    const f64 k0 = Lfs53(K), k1 = Lfs53(K + 4);
    const f64 t = F53(k1 * Lfs53(0x01069008u));
    if (load<u32>(a + 668) != 0xE8534CE4u) {
        const f64 f1 = load<u8>(a + 700) == 0 ? Lfs53(0x01068FB8u) : k0;
        const u32 flag = f1 != k0 ? 1u : 0u;
        ctx.fpr[1] = f1;
        const u32 h = static_cast<u32>(VCall(ctx, a, 60, {a}));
        callf(ctx, 0x002775A8u, {h, 0xE8534CE4u, 0, 0, ~0ull, ctx.gpr[8], 0, 0, 0, 0, 0, ~0ull},
              {t, F53(static_cast<f64>(flag)), k0, k0});
        store<u8>(a + 700, 0);
        store<u32>(a + 668, 0xE8534CE4u);
    }
    a = load<u32>(s + 32);
    const u32 x = load<u32>(a + 1052);
    store<u32>(a + 1048, 0x3F800000u);
    store<u32>(a + 1044, x);
    store<u32>(a + 1028, 0);
    callf(ctx, 0x004A0C20u, {a + 1024, 0, K}, {F53(F53(k1 - static_cast<f64>(std::bit_cast<f32>(x))) * Lfs53(K + 8))});
    callf(ctx, 0x0030DBF0u, {load<u32>(s + 32), 0xCB1675F3u, ctx.gpr[5], 0}, {k0});
    ctx.gpr[3] = VCall(ctx, s, 28, {s, 0, 1});
}

// u32 f(State* s): each of the objects 0x7001C018..+4 that exists
// (Obj_FindById) gets o->vt[88](o, 0, 1, 1, 1) and o->vt[40](o, 1); on the
// actor a = s->+32 Field_SetByteClear4_004A090C(+544, 1) and (+556 / +592
// / +604 / +616 / +628, 0); each set part of a->+776..+784 goes through
// Obj_ReleaseParts1F4460(p) and p->vt[88](p, 1, 0, 0, 0); r3 =
// Field_SetByteClear4_004A090C(a + 640, 0).
DECOMP(003196D4, State_ResetParts_003196D4) {
    const u32 s = arg<u32>(ctx, 0);
    for (u32 i = 0; i < 5; ++i) {
        const u32 x = static_cast<u32>(call(ctx, 0x000D6A28u, {0x7001C018u + i}));
        if (x == 0) continue;
        VCall(ctx, x, 88, {x, 0, 1, 1, 1});
        VCall(ctx, x, 40, {x, 1});
    }
    u32 a = load<u32>(s + 32);
    call(ctx, 0x004A090Cu, {a + 544, 1});
    for (u32 off : {556u, 592u, 604u, 616u, 628u}) call(ctx, 0x004A090Cu, {a + off, 0});
    a = load<u32>(s + 32);
    for (u32 j = 0; j < 3; ++j) {
        const u32 q = a + 776 + 4 * j;
        if (load<u32>(q) == 0) continue;
        call(ctx, 0x001F4460u, {load<u32>(q)});
        const u32 p = load<u32>(q);
        VCall(ctx, p, 88, {p, 1, 0, 0, 0});
    }
    ctx.gpr[3] = call(ctx, 0x004A090Cu, {a + 640, 0});
}

// u32 f(Obj* o) (reset): o->vt[20](o, 0), Obj_Place3626E8(o, 0x01164B40,
// 1); +248 / +244 = 0, +160 &= 1, f32 +240 = -1, byte +164 = 0, +252 &=
// ~0x6D800000 (bits 0x40000000 / 0x20000000 / 0x08000000 / 0x04000000 /
// 0x01000000 / 0x00800000). When bit 0x10000000 of +252 is set (and o != 0), the key k
// = o->+12 (0: (o->+8 ? o->+8->+68 : 0)) is looked up in the sorted map
// 0x0106C240 (SortedMap_Find3, the key at sp+112); when its slot holds 0
// the entry {k (recomputed), o} is appended to the map's 8-byte array
// (+8; u16 cap +0, count +2, high-water +4, Array_Reserve(map, 2 * cap)
// when full) and the u16 flags +6 |= 1 (r3 = the flags). Otherwise r3 =
// o->+8 (bit clear) or the slot's value.
DECOMP(0037DC70, Obj_ResetRegister_0037DC70) {
    const u32 o = arg<u32>(ctx, 0);
    constexpr u32 M = 0x0106C240u;
    VCall(ctx, o, 20, {o, 0});
    call(ctx, 0x003626E8u, {o, 0x01164B40u, 1});
    const u32 w = load<u32>(o + 252) & ~(0x04000000u | 0x08000000u | 0x01000000u | 0x00800000u | 0x40000000u | 0x20000000u);
    const u32 f160 = load<u32>(o + 160) & 1u;
    store<u32>(o + 248, 0);
    store<u32>(o + 244, 0);
    store<u32>(o + 160, f160);
    store<u32>(o + 240, 0xBF800000u);
    store<u8>(o + 164, 0);
    const u32 o12 = load<u32>(o + 12), o8 = load<u32>(o + 8);
    store<u32>(o + 252, w);
    if ((w & 0x10000000u) == 0 || o == 0) {
        ret(ctx, o8);
        return;
    }
    Stack fr(ctx, 16);
    store<u32>(fr.addr(), o12 != 0 ? o12 : (o8 != 0 ? load<u32>(o8 + 68) : 0));
    const u32 v = load<u32>(static_cast<u32>(call(ctx, 0x0076611Cu, {M, fr.addr()})));
    if (v != 0) {
        ret(ctx, v);
        return;
    }
    u32 k = load<u32>(o + 12);
    if (k == 0) {
        const u32 b = load<u32>(o + 8);
        k = b != 0 ? load<u32>(b + 68) : 0;
    }
    const u32 cap = load<u16>(M);
    if (load<u16>(M + 2) == cap) call(ctx, 0x00558F3Cu, {M, (cap << 1) & 0xFFFEu});
    const u32 d = load<u32>(M + 8);
    store<u32>(d + ((static_cast<u32>(load<u16>(M + 2)) << 3) & 0x7FFF8u), k);
    store<u32>(d + (static_cast<u32>(load<u16>(M + 2)) << 3) + 4, o);
    const u32 n = static_cast<u16>(load<u16>(M + 2) + 1);
    const u32 hw = load<u16>(M + 4);
    const u32 fl = load<u16>(M + 6) | 1u;
    store<u16>(M + 2, static_cast<u16>(n));
    store<u16>(M + 4, static_cast<u16>(static_cast<s32>(hw) > static_cast<s32>(n) ? hw : n));
    store<u16>(M + 6, static_cast<u16>(fl));
    ret(ctx, fl);
}

// u32 f(Obj* o): Obj_ResetRegister_0037DC70 (the same code).
DECOMP(00380A8C, Obj_ResetRegister_00380A8C) {
    const u32 o = arg<u32>(ctx, 0);
    constexpr u32 M = 0x0106C240u;
    VCall(ctx, o, 20, {o, 0});
    call(ctx, 0x003626E8u, {o, 0x01164B40u, 1});
    const u32 w = load<u32>(o + 252) & ~(0x04000000u | 0x08000000u | 0x01000000u | 0x00800000u | 0x40000000u | 0x20000000u);
    const u32 f160 = load<u32>(o + 160) & 1u;
    store<u32>(o + 248, 0);
    store<u32>(o + 244, 0);
    store<u32>(o + 160, f160);
    store<u32>(o + 240, 0xBF800000u);
    store<u8>(o + 164, 0);
    const u32 o12 = load<u32>(o + 12), o8 = load<u32>(o + 8);
    store<u32>(o + 252, w);
    if ((w & 0x10000000u) == 0 || o == 0) {
        ret(ctx, o8);
        return;
    }
    Stack fr(ctx, 16);
    store<u32>(fr.addr(), o12 != 0 ? o12 : (o8 != 0 ? load<u32>(o8 + 68) : 0));
    const u32 v = load<u32>(static_cast<u32>(call(ctx, 0x0076611Cu, {M, fr.addr()})));
    if (v != 0) {
        ret(ctx, v);
        return;
    }
    u32 k = load<u32>(o + 12);
    if (k == 0) {
        const u32 b = load<u32>(o + 8);
        k = b != 0 ? load<u32>(b + 68) : 0;
    }
    const u32 cap = load<u16>(M);
    if (load<u16>(M + 2) == cap) call(ctx, 0x00558F3Cu, {M, (cap << 1) & 0xFFFEu});
    const u32 d = load<u32>(M + 8);
    store<u32>(d + ((static_cast<u32>(load<u16>(M + 2)) << 3) & 0x7FFF8u), k);
    store<u32>(d + (static_cast<u32>(load<u16>(M + 2)) << 3) + 4, o);
    const u32 n = static_cast<u16>(load<u16>(M + 2) + 1);
    const u32 hw = load<u16>(M + 4);
    const u32 fl = load<u16>(M + 6) | 1u;
    store<u16>(M + 2, static_cast<u16>(n));
    store<u16>(M + 4, static_cast<u16>(static_cast<s32>(hw) > static_cast<s32>(n) ? hw : n));
    store<u16>(M + 6, static_cast<u16>(fl));
    ret(ctx, fl);
}

// The body of UiSet_Flip_0036C160 / _00389368: for each of the u32
// o->+8->+152 16-byte entries at o->+off (count reloaded after each
// handled one) whose object (+0) has bit 0x8000 of +164 and whose UI
// element u (+8) is set: the object's matrix at +80 with rows 0 and 1
// negated (raw sign flips) goes to u->vt[20](u, &m) (UiElem_SetMatrix: the
// xyz of the four rows at sp+112 / +128 / +144 / +160; the w words are
// left unwritten, as in the original, and not read), then u->vt[152](u)
// with f1 = the f32 at k and u->vt[156](u). r3 as left.
static void UiFlip53(PPUContext& ctx, u32 off, u32 k) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 b = load<u32>(o + 8);
    u32 n = load<u32>(b + 152);
    if (n == 0) {
        ret(ctx, 0u);
        return;
    }
    const f64 f = Lfs53(k);
    Stack fr(ctx, 64);
    const u32 m = fr.addr();
    u64 r3 = n;
    for (u32 i = 0; i < n; ++i) {
        const u32 e = load<u32>(o + off) + 16 * i;
        const u32 u = load<u32>(e + 8);
        if (u == 0) continue;
        const u32 ob = load<u32>(e);
        if (ob == 0 || (load<u32>(ob + 164) & 0x8000u) == 0) continue;
        const u32 src = ob + 80;
        for (u32 row = 0; row < 4; ++row)
            for (u32 c = 0; c < 3; ++c) {
                const u32 w = load<u32>(src + 16 * row + 4 * c);
                store<u32>(m + 16 * row + 4 * c, row < 2 ? Neg52(w) : w);
            }
        ctx.fpr[1] = static_cast<f64>(std::bit_cast<f32>(load<u32>(m)));  // f1 as the copies leave it
        VCall(ctx, u, 20, {u, m});
        const u32 u2 = load<u32>(e + 8);
        ctx.fpr[1] = f;
        VCall(ctx, u2, 152, {u2});
        VCall(ctx, u2, 156, {u2});
        n = load<u32>(b + 152);
        r3 = n;
    }
    ctx.gpr[3] = r3;
}

// u32 f(T* o): UiFlip53 over the entries at +328 (f1 = the f32 0x0036C158).
DECOMP(0036C160, UiSet_Flip_0036C160) { UiFlip53(ctx, 328, 0x0036C158u); }

// u32 f(T* o): UiFlip53 over the entries at +236 (f1 = the f32 0x00389360).
DECOMP(00389368, UiSet_Flip_00389368) { UiFlip53(ctx, 236, 0x00389360u); }

// u32 f(Rec* a, Set* b): the handle h = the word at +4 past u16 0x0106CF80
// in the 132-byte slot b + 152 + 132 * (s16 a->+6) when that index is
// below u16 b->+144 and the slot's s32 +0 is below 10 (otherwise the same
// offset from address 0, as the original reads it). Nothing when h is 0
// or already b's current entry (the 8-byte entries at b + 16, current u8
// b->+5). When one of the u8 b->+4 entries holds h: u16 b->+6 |= 0x40 and
// then the byte b->+6 = its index. Otherwise String_Format(0x00B1FA8C,
// Hash_ToString(h)), u16 b->+6 |= 0x10 and String_Format(0x00ADDA00, the
// name of a's column / row: the 128-byte records at (*0x00F65038)->+24 of
// s16 a->+4 (r) and s16 a->+2 (c; -1: none, address 0): the word at r +
// u16 0x0106CF70 + 4 * the byte at (c->+2 ? c + u16 0x0106CF7E : 0)). r3 =
// 0.
DECOMP(003919A4, Set_SelectHandle_003919A4) {
    const u32 a = arg<u32>(ctx, 0), b = arg<u32>(ctx, 1);
    const s32 idx = static_cast<s16>(load<u16>(a + 6));
    const u32 cnt = load<u8>(b + 4), cur = load<u8>(b + 5), f6 = load<u16>(b + 6);
    u32 base = 0;
    if (idx < static_cast<s32>(load<u16>(b + 144))) {
        const u32 r = b + 152 + 132 * static_cast<u32>(idx);
        if (static_cast<s32>(load<u32>(r)) < 10) base = r;
    }
    const u32 h = load<u32>(base + load<u16>(0x0106CF80u) + 4);
    if (h == 0 || load<u32>(b + 16 + 8 * cur) == h) {
        ret(ctx, 0u);
        return;
    }
    for (u32 i = 0; static_cast<s32>(i) < static_cast<s32>(cnt); i = (i + 1) & 0xFFFFu)
        if (load<u32>(b + 16 + 8 * i) == h) {
            store<u16>(b + 6, static_cast<u16>(f6 | 0x40u));
            store<u8>(b + 6, static_cast<u8>(i));
            ret(ctx, 0u);
            return;
        }
    call(ctx, 0x005534D0u, {0x00B1FA8Cu, call(ctx, 0x0056A6E0u, {h})});
    store<u16>(b + 6, static_cast<u16>(load<u16>(b + 6) | 0x10u));
    const u32 g = load<u32>(0x00F65038u);
    const s32 ri = static_cast<s16>(load<u16>(a + 4)), ci = static_cast<s16>(load<u16>(a + 2));
    const u32 row = ri == -1 ? 0 : load<u32>(g + 24) + (static_cast<u32>(ri) << 7);
    const u32 col = ci == -1 ? 0 : load<u32>(g + 24) + (static_cast<u32>(ci) << 7);
    const u32 sel = load<u8>(col + 2) != 0 ? col + load<u16>(0x0106CF7Eu) : 0;
    const u32 name = load<u32>(row + load<u16>(0x0106CF70u) + (static_cast<u32>(load<u8>(sel)) << 2));
    call(ctx, 0x005534D0u, {0x00ADDA00u, name});
    ret(ctx, 0u);
}

// u32 f(T* o): when o->+12 (an object) or o->+16 (a node) is set, the
// world object w = *(*0x00F65CA0 + 0x001281E0) gets
// Obj_ResetMatrix_000FCD1C(w) and Obj_ApplyDesc_000FB31C(w, &desc, 1) with
// a transform descriptor on the stack (sp+112): the 4 x 3 floats of the
// matrix +12->+112 (rows 0 and 2 negated) or +16->+80 (the w words left
// unwritten, as Player_ResetXform_003B8980's), +64 = f32 o->+20 when above
// the f32 0x0042D27C, else +12->+176 (no +12: the f32 0x0042D280), +68 =
// 7.5, zeros, the 4 bytes of 0x01164CA8. Then r3 = Flag_Set8_40000000(o).
DECOMP(0042D288, Cam_ApplyTarget_0042D288) {
    const u32 o = arg<u32>(ctx, 0);
    if (load<u32>(o + 12) != 0 || load<u32>(o + 16) != 0) {
        const u32 w = load<u32>(load<u32>(0x00F65CA0u) + 0x001281E0u);
        call(ctx, 0x000FCD1Cu, {w});
        const u32 t = load<u32>(o + 12);
        u32 f = load<u32>(o + 20);
        const u32 m = t != 0 ? t + 112 : load<u32>(o + 16) + 80;
        u32 v[12];
        for (u32 row = 0; row < 4; ++row)
            for (u32 c = 0; c < 3; ++c) {
                const u32 x = load<u32>(m + 16 * row + 4 * c);
                v[3 * row + c] = (t != 0 && (row & 1) == 0) ? Neg52(x) : x;
            }
        if (!(Lfs53(o + 20) > Lfs53(0x0042D27Cu))) f = t != 0 ? load<u32>(t + 176) : load<u32>(0x0042D280u);
        Stack fr(ctx, 112);
        const u32 s = fr.addr();
        for (u32 row = 0; row < 4; ++row)
            for (u32 c = 0; c < 3; ++c) store<u32>(s + 16 * row + 4 * c, v[3 * row + c]);
        store<u32>(s + 64, f);
        store<u32>(s + 68, 0x40F00000u);
        store<u8>(s + 72, 0);
        store<u32>(s + 76, 0);
        store<u32>(s + 80, 0);
        store<u32>(s + 84, 0);
        store<u8>(s + 88, 0);
        store<u32>(s + 92, 0);
        for (u32 k = 0; k < 4; ++k) store<u8>(s + 96 + k, load<u8>(0x01164CA8u + k));
        ctx.fpr[1] = static_cast<f64>(std::bit_cast<f32>(v[0]));  // left in f1
        call(ctx, 0x000FB31Cu, {w, s, 1});
    }
    ctx.gpr[3] = call(ctx, 0x0042CA60u, {o});
}

// ---------------------------------------------------------------- listed as VMX, but plain
// u32 f(Obj* o, u32* key): Query_Run_000DB94C(&tmp (sp+128), *key,
// (o->+216)->+176, 0, 0, 0, 0, 0, 0); r3 = its result.
DECOMP(001A98B4, Obj_QueryKey_001A98B4) {
    const u32 o = arg<u32>(ctx, 0), k = arg<u32>(ctx, 1);
    Stack fr(ctx, 32);
    const u32 s = load<u32>(o + 216);
    ctx.gpr[3] = call(ctx, 0x000DB94Cu, {fr.addr() + 16, load<u32>(k), load<u32>(s + 172 + 4), 0, 0, 0, 0, 0, 0});
}

// s32 f(Net* n, u32 i, void* buf, u32 len): sys_net recvfrom (the import
// stub 0x00ABDC44) on the socket n->+4 + 4 * i (sign-extended) into
// buf / len, flags 0, the 16-byte source address at sp+116 and its length
// (16) at sp+112. r3 = the stub's result.
DECOMP(004270C8, Net_RecvFrom_004270C8) {
    const u32 n = arg<u32>(ctx, 0), i = arg<u32>(ctx, 1), buf = arg<u32>(ctx, 2), len = arg<u32>(ctx, 3);
    Stack fr(ctx, 32);
    const u32 alen = fr.addr(), addr = fr.addr() + 4;
    store<u32>(alen, 16);
    const s32 sock = static_cast<s32>(load<u32>(n + (i << 2) + 4));
    ctx.gpr[3] = call(ctx, 0x00ABDC44u, {static_cast<u64>(static_cast<s64>(sock)), buf, len, 0, addr, alen});
}
