// Game code, part 37 (continued): the rest of the 75 instruction functions
// and up. Each was checked against the original in the PPC simulator
// (memory writes, calls and their arguments, r3 and f1).
#include "obj37.h"

using namespace sly4;
using namespace sly4::obj37;

// ---------------------------------------------------------------- sweep: 75 instructions (continued)
// void f(T* o): u8 +483 = 0; Handle_Call363A3C(0x251B46E4, 0, 1, -, 0, 0,
// 0x01164CA4; f1 = -1) (r6, the float's slot, as left); SetSelect_491000(o,
// 0x9DF71241); o->+468->vt[88](it, 1, 1, 1, 1); for the player object t
// = (o->vt[76](o))->+0x1281E0 (when set) that does not yet hold o->+60
// (Obj_HasWord368_000FC5DC): Camera_PushTarget0FC040(t, o->+60, -, 0; f1
// = -1) (r5 as left). r3 = the last call's.
DECOMP(0041EF90, Obj_Focus_0041EF90) {
    const u32 o = arg<u32>(ctx, 0);
    const f64 k = Lfs32(0x0041EF8Cu);
    store<u8>(o + 483, 0);
    callf(ctx, 0x0036735Cu, {0x251B46E4u, 0, 1, ctx.gpr[6], 0, 0, 0x01164CA4u}, {k});
    call(ctx, 0x00491000u, {o, 0x9DF71241u});
    const u32 p = load<u32>(o + 468);
    VCall(ctx, p, 88, {p, 1, 1, 1, 1});
    const u32 g = static_cast<u32>(VCall(ctx, o, 76, {o}));
    const u32 t = load<u32>(g + 0x1281E0u);
    ctx.fpr[1] = k;
    if (t == 0) return;  // r3 = that vt[76] result
    if (static_cast<u32>(call(ctx, 0x000FC5DCu, {t, load<u32>(o + 60)})) != 0) return;
    callf(ctx, 0x000FC040u, {t, load<u32>(o + 60), ctx.gpr[5], 0}, {k});
}

// void f(T* o): the deleting destructor of an import-handle set: nothing
// for o = 0 (r3 = 0); vtable 0x00B20CB0, then for each 12-byte record of
// o->+24 (o->+16 of them, re-read after each call) whose +8 is >= 1 the
// stack request {o->+4, u16 o->+8, 8, 292 zero bytes} is sent to its
// handle (s32 (o->+28)[i + 1], when o->+28 is set) through
// Import_ABDDC4(h, &req, 304, 0); Mem_FreeFrom(o, 0) (r3 = its).
DECOMP(00427D00, ImportSet_Delete_00427D00) {
    const u32 o = arg<u32>(ctx, 0);
    if (o == 0) return;
    Stack fr(ctx, 304);
    const u32 rq = fr.addr();
    store<u32>(rq, 0);
    store<u32>(o, 0x00B20CB0u);
    store<u16>(rq + 4, 0);
    store<u32>(rq + 8, 8);
    call(ctx, 0x00726E54u, {rq + 12, 0, 292});
    for (u32 i = 0; i < load<u32>(o + 16); ++i) {
        if (load<s32>(load<u32>(o + 24) + 12 * i + 8) < 1) continue;
        store<u32>(rq, load<u32>(o + 4));
        store<u16>(rq + 4, load<u16>(o + 8));
        const u32 q = load<u32>(o + 28);
        if (q == 0) continue;
        call(ctx, 0x00ABDDC4u, {Sx33(load<u32>(q + ((i << 2) & 0x3FFFCu) + 4)), rq, 304, 0});
    }
    call(ctx, 0x0054EE6Cu, {o, 0});
}

// void f(u32 id): the first entry e of the pointer vector 0x010AA694
// (data +0, u16 count +8) without bit 0x20000000 of +24 whose +0 is id
// (nothing for id 0 / none: r3 = id): each set object of its vector +4
// (u16 count +12) gets vt[4](), the count = 0 and +24 = (+24 & 0x7FFFFFFF)
// | 0x20000000. r3 = that word.
DECOMP(0042C00C, Group_ReleaseById_0042C00C) {
    const u32 id = arg<u32>(ctx, 0);
    if (id == 0) return;
    constexpr u32 A = 0x010AA694u;
    const u32 d = load<u32>(A);
    const u32 n = load<u16>(A + 8);
    for (u32 i = 0; i < n; ++i) {
        const u32 e = load<u32>(d + 4 * i);
        if ((load<u32>(e + 24) & 0x20000000u) != 0 || load<u32>(e) != id) continue;
        const u32 x = load<u32>(d + 4 * i);
        if (x == 0) {
            ret(ctx, d + 4 * i);
            return;
        }
        for (u32 j = 0; j < load<u16>(x + 12); ++j) {
            const u32 p = load<u32>(load<u32>(x + 4) + 4 * j);
            if (p != 0) VCall(ctx, p, 4, {p});
        }
        const u32 w = load<u32>(x + 24);
        store<u16>(x + 12, 0);
        const u32 w2 = (w & 0x7FFFFFFFu) | 0x20000000u;
        store<u32>(x + 24, w2);
        ret(ctx, w2);
        return;
    }
}

// T* f(T* o): constructor: u16 +4 = 1, vtable 0x00B220D8, the pointer
// vector +12 (PtrVector_Construct, element size u16 +18 = 12,
// PtrVector_Init(v, 32, 0, 0)), the array +32 (u16 capacity 4, +34 / +36
// = 0, element size +38 = 16, data +40 = 0) with the default allocator a
// (+44) and, when the capacity (re-read) is not 0, data = a->vt[0](a,
// cap * (size >> 1), 0, 16) (entry / OPD after the arguments);
// Array_Init(o + 32); +52 = the OPD 0x00F35C30, +48 / +56 = 0, bit 31 of
// +60 cleared. r3 = 0.
DECOMP(004B9DA4, VtB220D8_Ctor_004B9DA4) {
    const u32 o = arg<u32>(ctx, 0);
    store<u16>(o + 4, 1);
    store<u32>(o, 0x00B220D8u);
    call(ctx, 0x005516E0u, {o + 12});
    store<u16>(o + 18, 12);
    call(ctx, 0x00551598u, {o + 12, 32, 0, 0});
    store<u16>(o + 32, 4);
    store<u16>(o + 34, 0);
    store<u16>(o + 36, 0);
    store<u16>(o + 38, 16);
    store<u32>(o + 40, 0);
    const u32 a = static_cast<u32>(call(ctx, 0x00550EE8u));
    const u32 cap = load<u16>(o + 32);
    store<u32>(o + 44, a);
    if (cap != 0) {
        const u32 n = cap * ((static_cast<u32>(load<u16>(o + 38)) >> 1) & 0x7FFFu);
        store<u32>(o + 40, static_cast<u32>(VCallEO36(ctx, load<u32>(a), 0, {a, n, 0, 16})));
    }
    call(ctx, 0x00558F38u, {o + 32});
    const u32 w = load<u32>(o + 60);
    store<u32>(o + 52, 0x00F35C30u);
    store<u32>(o + 48, 0);
    store<u32>(o + 56, 0);
    store<u32>(o + 60, w & 0x7FFFFFFFu);
    ret(ctx, 0u);
}

// T* New(-, Xml* x): a 40-byte object (Mem_Alloc(40, 0, 16); 0 on
// failure): u16 +4 = 1, vtable 0x00B21FD8, the pointer vector +8
// (PtrVector_Construct, element size u16 +14 = 4, PtrVector_Init(v, 0, 0,
// 0)), +28 / +36 = 0, vtable 0x00B22078, +32 = 0x0376EC5C; +36 = the
// String_HashNoCase of the attribute "object" when present, else the s32
// 0x5C8998CD (default 0). r3 = m.
DECOMP(004C1B7C, NewObjectRef_004C1B7C) {
    const u32 x = arg<u32>(ctx, 1);
    const u32 m = static_cast<u32>(call(ctx, 0x0054ED38u, {40, 0, 16}));
    if (m == 0) {
        ret(ctx, 0u);
        return;
    }
    store<u16>(m + 4, 1);
    store<u32>(m, 0x00B21FD8u);
    call(ctx, 0x005516E0u, {m + 8});
    store<u16>(m + 14, 4);
    call(ctx, 0x00551598u, {m + 8, 0, 0, 0});
    store<u32>(m + 28, 0);
    store<u32>(m + 36, 0);
    store<u32>(m, 0x00B22078u);
    store<u32>(m + 32, 0x0376EC5Cu);
    const u32 s = static_cast<u32>(call(ctx, 0x004F9628u, {x, 0x00AEF030u}));
    if (s != 0) store<u32>(m + 36, static_cast<u32>(call(ctx, 0x0056A710u, {s})));
    else call(ctx, 0x004F99BCu, {x, 0x5C8998CDu, m + 36, 0});
    ret(ctx, m);
}

// T* New(-, Xml* x): a 24-byte object (Mem_Alloc(24, 0, 16); 0 on
// failure): u16 +4 = 1, vtable 0x00B22290, +12 / +16 = 0, +20 = 1.0f; the
// hash 0x8A93973F to +12 (default 0) and, when that is found, the s32
// 0x5C8998CD to +12; the floats 0x23B769E2 (+16) and 0x7520EA88 (+20)
// with the default 0 (0x004CCAF0; f1). r3 = m.
DECOMP(004CCAF8, NewF32Pair_004CCAF8) {
    const u32 x = arg<u32>(ctx, 1);
    const u32 m = static_cast<u32>(call(ctx, 0x0054ED38u, {24, 0, 16}));
    if (m == 0) {
        ret(ctx, 0u);
        return;
    }
    store<u16>(m + 4, 1);
    store<u32>(m, 0x00B22290u);
    store<u32>(m + 12, 0);
    const f64 k = Lfs32(0x004CCAF0u);
    store<u32>(m + 16, 0);
    store<u32>(m + 20, 0x3F800000u);
    if (static_cast<u32>(call(ctx, 0x004FA14Cu, {x, 0x8A93973Fu, m + 12, 0})) != 0)
        call(ctx, 0x004F99BCu, {x, 0x5C8998CDu, m + 12, 0});
    callf(ctx, 0x004F9B20u, {x, 0x23B769E2u, m + 16}, {k});
    callf(ctx, 0x004F9B20u, {x, 0x7520EA88u, m + 20}, {k});
    ret(ctx, m);
}

// The pool setup of 0x00733DD0 / 0x0077AC14 (void f(Pool* p, u32 n, c,
// Allocator* a)) with esz-byte elements: nothing for n = 0 (r3 = p); the
// block size s = (n * esz (+ n * 4 with c) + 31) & ~15 is allocated
// (a->vt[0](a, s, 0, 16)) to p->+0 / +4; the header gets u16 +2 / +4 / +8
// = 0 (when allocated), u16 +0 = n, +8 = c, +4 = s, +12 = a (quirk: the
// header writes are not checked); p->+8 = block + 16 with c (else 0) and
// p->+12 = block + 16 (+ n * 4 with c) (r3 = it).
static void PoolSetup37(PPUContext& ctx, u32 esz) {
    const u32 p = arg<u32>(ctx, 0), n = arg<u32>(ctx, 1), c = arg<u32>(ctx, 2), a = arg<u32>(ctx, 3);
    if (n == 0) return;
    const u32 s = (n * esz + (c != 0 ? n << 2 : 0u) + 31u) & 0xFFFFFFF0u;
    const u32 m = static_cast<u32>(VCall(ctx, a, 0, {a, s, 0, 16}));
    store<u32>(p, m);
    store<u32>(p + 4, m);
    if (m != 0) {
        store<u16>(m + 2, 0);
        store<u32>(m + 4, 0);
        store<u32>(m + 8, 0);
    }
    store<u16>(m, static_cast<u16>(n));
    const u32 h = load<u32>(p + 4);
    store<u32>(h + 8, c);
    store<u32>(h + 4, s);
    store<u32>(h + 12, a);
    const u32 b = load<u32>(p);
    const u32 l = c != 0 ? m + 16 : 0u;
    store<u32>(p + 8, l);
    const u32 e = l != 0 ? b + (static_cast<u32>(load<u16>(h)) << 2) + 16 : b + 16;
    store<u32>(p + 12, e);
    ret(ctx, e);
}

// void f(Pool* p, u32 n, c, Allocator* a): PoolSetup37 with 20-byte
// elements.
DECOMP(00733DD0, Pool20_Setup_00733DD0) { PoolSetup37(ctx, 20); }

// void f(Pool* p, u32 n, c, Allocator* a): PoolSetup37 with 112-byte
// elements.
DECOMP(0077AC14, Pool112_Setup_0077AC14) { PoolSetup37(ctx, 112); }

// T* New(s16 id): a 24-byte "RSetAnimSpeed" node (Mem_Alloc(24, 0, 16); 0
// on failure; Node_Init(m, id, name)): its record (the 128-byte entry
// s16 m->+16 of (*0x00F65038)->+24; address 0 for -1, quirk) gets u16 +0 =
// 16; the bindings "Speed" (+76 = 1.0f) and "Time" (+76 = 0) of m->+12
// (Node_Bind(it, name, 3, 1, -1, 0)) give their +64 bytes to m->+20 + u16
// *0x0106CF82 +0 / +1. r3 = m.
DECOMP(00779CB0, NewSetAnimSpeed_00779CB0) {
    const u32 id = arg<u32>(ctx, 0);
    const u32 m = static_cast<u32>(call(ctx, 0x0054ED38u, {24, 0, 16}));
    if (m == 0) {
        ret(ctx, 0u);
        return;
    }
    call(ctx, 0x00391694u, {m, Sx33(static_cast<u32>(static_cast<s32>(static_cast<s16>(id)))), 0x00B2006Bu});
    const s32 s = static_cast<s16>(load<u16>(m + 16));
    const u32 g = load<u32>(0x00F65038u);
    const u32 rec = s != -1 ? load<u32>(g + 24) + (static_cast<u32>(s) << 7) : 0u;
    store<u16>(rec, 16);
    const u32 dst = load<u32>(m + 20) + load<u16>(0x0106CF82u);
    const u32 b1 = static_cast<u32>(call(ctx, 0x004D3088u, {load<u32>(m + 12), 0x00B20079u, 3, 1, ~0ull, 0}));
    const u32 v1 = load<u32>(b1 + 64);
    store<u32>(b1 + 76, 0x3F800000u);
    store<u8>(dst, static_cast<u8>(v1));
    const u32 b2 = static_cast<u32>(call(ctx, 0x004D3088u, {load<u32>(m + 12), 0x00B2007Fu, 3, 1, ~0ull, 0}));
    const u32 v2 = load<u32>(b2 + 64);
    store<u32>(b2 + 76, 0);
    store<u8>(dst + 1, static_cast<u8>(v2));
    ret(ctx, m);
}

// ---------------------------------------------------------------- sweep: 76 instructions
// void f(T* o, u32 ev): the events 0x2DAE1199 / 0xF3626E10 / 0xDE32106A
// give o->vt[220](o, 27 / 25 / 29); 0xB9EDFAB5 / 0xB9EDFAB4 / 0xB9EDFAB3
// set +716 = 2 / 1 / 0 and call 0x0003E280(o, o). r3 as left.
DECOMP(000400D4, Obj_OnEvent_000400D4) {
    const u32 o = arg<u32>(ctx, 0), ev = arg<u32>(ctx, 1);
    switch (ev) {
    case 0x2DAE1199u: VCall(ctx, o, 220, {o, 27}); return;
    case 0xF3626E10u: VCall(ctx, o, 220, {o, 25}); return;
    case 0xDE32106Au: VCall(ctx, o, 220, {o, 29}); return;
    case 0xB9EDFAB5u: store<u32>(o + 716, 2); break;
    case 0xB9EDFAB4u: store<u32>(o + 716, 1); break;
    case 0xB9EDFAB3u: store<u32>(o + 716, 0); break;
    default: return;
    }
    call(ctx, 0x0003E280u, {o, o});
}

// T* New(-, Xml* x): a 28-byte object (Mem_Alloc(28, 0, 16); 0 on
// failure): Obj_InitVt_B21C98(m, x), u8 +12 = 0, vtable 0x00B0F4F0, +16 =
// 0, +20 = 1.0f, +24 = 0; the bool 0x7A281983 (+12, default 0) and the
// floats 0x3FD77BF5 (+16), 0x00264412 (+20), 0xDE55FE5F (+24) with the
// default 0 (0x00072CC4; f1). r3 = m.
DECOMP(00072CC8, NewB0F4F0_00072CC8) {
    const u32 x = arg<u32>(ctx, 1);
    const u32 m = static_cast<u32>(call(ctx, 0x0054ED38u, {28, 0, 16}));
    if (m == 0) {
        ret(ctx, 0u);
        return;
    }
    call(ctx, 0x004BB3B0u, {m, x});
    store<u8>(m + 12, 0);
    store<u32>(m, 0x00B0F4F0u);
    const f64 k = Lfs32(0x00072CC4u);
    store<u32>(m + 16, 0);
    store<u32>(m + 20, 0x3F800000u);
    store<u32>(m + 24, 0);
    ctx.fpr[1] = k;
    call(ctx, 0x004F986Cu, {x, 0x7A281983u, m + 12, 0});
    callf(ctx, 0x004F9B20u, {x, 0x3FD77BF5u, m + 16}, {k});
    callf(ctx, 0x004F9B20u, {x, 0x00264412u, m + 20}, {k});
    callf(ctx, 0x004F9B20u, {x, 0xDE55FE5Fu, m + 24}, {k});
    ret(ctx, m);
}

// void f(Def* o, X* x, c): for the kind-5 component target t of x named
// o->+12 (as CompTarget34) with bit 8 of +192: bit 0x20 of u8 t->+968 =
// (Colour_Fade_00077D90(o, x, c, &rgba) is 0) and the 4 bytes of rgba to
// t->+969..+972. r3 as the original leaves it (the last byte when done).
DECOMP(00077EB0, Obj_FadeColour_00077EB0) {
    const u32 o = arg<u32>(ctx, 0), x = arg<u32>(ctx, 1), c = arg<u32>(ctx, 2);
    const u32 k = load<u32>(o + 12);
    if (k == 0) return;
    const u32 cp = static_cast<u32>(call(ctx, 0x004BB4B4u, {x, k}));
    if (cp == 0 || static_cast<s32>(VCall(ctx, cp, 32, {cp})) != 5) return;
    const u32 c2 = load<u32>(cp + 28);
    if (c2 == 0) return;  // r3 = 5
    ctx.gpr[3] = 0x02235C33u;
    if (load<u32>(c2 + 276) != 0x02235C33u) return;
    const u32 t = load<u32>(c2 + 280);
    if (t == 0) return;
    const u32 b = load<u32>(t + 192) & 8u;
    if (b == 0) {
        ret(ctx, 0u);
        return;
    }
    Stack fr(ctx, 16);
    const u32 r = static_cast<u32>(call(ctx, 0x00077D90u, {o, x, c, fr.addr()}));
    store<u8>(t + 968, static_cast<u8>((load<u8>(t + 968) & ~0x20u) | (r == 0 ? 0x20u : 0u)));
    for (u32 i = 0; i < 4; ++i) store<u8>(t + 969 + i, load<u8>(fr.addr() + i));
    ret(ctx, static_cast<u32>(load<u8>(fr.addr() + 3)));
}

// ~T() thunk for the +80 base: DtorUnlist37(this - 80) with vtables
// 0x00B1C258 / 0x00B1C2A8.
DECOMP(0027D200, VtB1C258_DtorThunk80_0027D200) { DtorUnlist37(ctx, 0x00B1C258u, 0x00B1C2A8u, 80); }

// ~T() thunk for the +80 base: DtorUnlist37(this - 80) with vtables
// 0x00B1C328 / 0x00B1C378.
DECOMP(0027ED34, VtB1C328_DtorThunk80_0027ED34) { DtorUnlist37(ctx, 0x00B1C328u, 0x00B1C378u, 80); }

// ~T() thunk for the +80 base: DtorUnlist37(this - 80) with vtables
// 0x00B1C3F8 / 0x00B1C444.
DECOMP(002804E4, VtB1C3F8_DtorThunk80_002804E4) { DtorUnlist37(ctx, 0x00B1C3F8u, 0x00B1C444u, 80); }

// bool f(u32* out, const SaveList* t, SaveInfo* info): whether one of the
// t->+4 48-byte entries of t->+8 starts with "BCUS98247_SAVE_"
// (Mem_Compare with its String_Length) goes to the byte 0x01012788;
// info: +12 = t->+8, +8 = min(t->+4, 2048), +0 = 1, +4 / +20 / +16 = 0;
// *out = 0. r3 = 1.
DECOMP(000B1310, Save_ListInfo_000B1310) {
    const u32 out = arg<u32>(ctx, 0), t = arg<u32>(ctx, 1), info = arg<u32>(ctx, 2);
    constexpr u32 kPrefix = 0x00AC6210u;
    const u32 len = static_cast<u32>(call(ctx, 0x00555FA4u, {kPrefix}));
    u32 found = 0;
    for (u32 i = 0; i < load<u32>(t + 4); ++i) {
        if (static_cast<u32>(call(ctx, 0x00555DB8u, {load<u32>(t + 8) + 48 * i, kPrefix, len})) == 0) {
            found = 1;
            break;
        }
    }
    const u32 n = load<u32>(t + 4);
    const u32 d = load<u32>(t + 8);
    store<u8>(0x01012788u, static_cast<u8>(found));
    store<u32>(info + 12, d);
    store<u32>(info + 8, n > 0x800u ? 0x800u : n);
    store<u32>(info, 1);
    store<u32>(info + 4, 0);
    store<u32>(info + 20, 0);
    store<u32>(info + 16, 0);
    store<u32>(out, 0);
    ret(ctx, 1u);
}

// void f(Cam* o): with a target t = o->+84 whose bit 0x400 of +200 is
// set: from t->+216's frame (+240): +656..+664 (position, raw) to
// o->+96..+104, +672..+680 (up, raw) to +112..+120, +688 to +80; the
// basis o->+48 (forward f = -position normalized), +16 (r = normalize(up
// x f)) and +32 (f x r) (single precision, fmsubs; constant 1 at
// 0x001134D4). r3 = o + 32, f1 = f.x (r3 = o without a target).
DECOMP(001134D8, Cam_BasisFromTarget_001134D8) {
    const u32 o = arg<u32>(ctx, 0);
    auto f = [](f64 x) { return static_cast<f64>(static_cast<f32>(x)); };
    auto ms = [&](f64 a, f64 c, f64 b) { return f(std::fma(a, c, -b)); };
    const u32 t = load<u32>(o + 84);
    if (t == 0 || (load<u32>(t + 200) & 0x400u) == 0) return;
    const u32 m = load<u32>(t + 216) + 240;
    const u32 wa = load<u32>(m + 416);
    store<u32>(o + 96, wa);
    const u32 wb = load<u32>(m + 420);
    store<u32>(o + 100, wb);
    const u32 wc = load<u32>(m + 424);
    store<u32>(o + 104, wc);
    const u32 wd = load<u32>(m + 432);
    store<u32>(o + 112, wd);
    const u32 we = load<u32>(m + 436);
    store<u32>(o + 116, we);
    const u32 wg = load<u32>(m + 440);
    store<u32>(o + 120, wg);
    store<u32>(o + 80, load<u32>(m + 448));
    auto fl = [](u32 w) { return static_cast<f64>(std::bit_cast<f32>(w)); };
    const f64 one = Lfs32(0x001134D4u);
    f64 fx = -fl(wa), fy = -fl(wb), fz = -fl(wc);
    f64 l = f(fy * fy);
    l = f(std::fma(fx, fx, l));
    l = f(std::fma(fz, fz, l));
    l = f(one / f(std::sqrt(l)));
    fz = f(fz * l);
    fx = f(fx * l);
    fy = f(fy * l);
    const f64 d = fl(wd), e = fl(we), g = fl(wg);
    f64 ry = ms(g, fx, f(d * fz)), rz = ms(d, fy, f(e * fx)), rx = ms(e, fz, f(g * fy));
    f64 k = f(ry * ry);
    k = f(std::fma(rx, rx, k));
    k = f(std::fma(rz, rz, k));
    k = f(one / f(std::sqrt(k)));
    rx = f(rx * k);
    ry = f(ry * k);
    rz = f(rz * k);
    const f64 a7 = f(fy * rx), a8 = f(fz * ry), a9 = f(fx * rz);
    Stfs32(o + 16, rx);
    Stfs32(o + 20, ry);
    Stfs32(o + 24, rz);
    Stfs32(o + 40, ms(fx, ry, a7));
    Stfs32(o + 32, ms(fy, rz, a8));
    Stfs32(o + 36, ms(fz, rx, a9));
    Stfs32(o + 48, fx);
    Stfs32(o + 52, fy);
    Stfs32(o + 56, fz);
    ctx.fpr[1] = fx;
    ret(ctx, o + 32);
}

// void f(T* o, Obj* m): o->+4 = Obj_FirstSet_00296898(CallK2_000D94F4(m)),
// Obj_SetEntries_00296840(CallK2_000D94F4(m), o->+4 + 3); unless m->+348
// <= 0 with the state +692 already 0x03DE9A54 (r3 = m's vtable word):
// Anim_Restart2775A8(m->vt[60](m), 0xD467C476, 0, 0, 0, -1, 0, 0, -, -, 0,
// -1; f1 = 1, f2 = 0.25, f3 = f4 = 0) (constants 0x0016D710; the float
// slots 9 / 10 passed as 0) and +692 = 0xD467C476 (r3).
DECOMP(0016D720, Obj_PlayIdle_0016D720) {
    const u32 o = arg<u32>(ctx, 0), m = arg<u32>(ctx, 1);
    constexpr u32 K = 0x0016D710u;
    const u32 x = static_cast<u32>(call(ctx, 0x000D94F4u, {m}));
    store<u32>(o + 4, static_cast<u32>(call(ctx, 0x00296898u, {x})));
    const u32 x2 = static_cast<u32>(call(ctx, 0x000D94F4u, {m}));
    call(ctx, 0x00296840u, {x2, Sx33(load<u32>(o + 4) + 3u)});
    const f64 k0 = Lfs32(K);
    const f64 v = Lfs32(m + 348);
    const u32 vt = load<u32>(m);
    ctx.fpr[1] = v;
    if ((v < k0 || v == k0) && load<u32>(m + 692) == 0x03DE9A54u) {
        ret(ctx, vt);
        return;
    }
    const u32 r = static_cast<u32>(VCall(ctx, m, 60, {m}));
    callf(ctx, 0x002775A8u, {r, 0xD467C476u, 0, 0, 0, ~0ull, 0, 0, 0, 0, 0, ~0ull}, {Lfs32(K + 4), Lfs32(K + 8), k0, k0});
    store<u32>(m + 692, 0xD467C476u);
    ret(ctx, 0xD467C476u);
}

// void f(T* o, bool on): Flag_Set172_80_000DA0CC(o, on); for the body x =
// o->+388 whose world word x->+8 is set exactly when on: on removes it
// (Hk_RemoveEntity92FFC8(&res, x->+8, x)); off adds it to the world's
// physics ((*0x00F65CA0)->+0x1281D4->+4, Hk_AddEntity92F2A8(w, x, 1))
// when o->+396 is 6, and then o->+392 (when set) through 0x0092E9D8(w,
// it). r3 as the original leaves it.
DECOMP(0016F04C, Obj_SetPhysics_0016F04C) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 on = arg<u32>(ctx, 1) & 0xFFu;
    call(ctx, 0x000DA0CCu, {o, on});
    const u32 x = load<u32>(o + 388);
    ctx.gpr[3] = x;
    if (x == 0) return;
    const u32 w8 = load<u32>(x + 8);
    if ((w8 != 0) != (on != 0)) {
        ret(ctx, w8);
        return;
    }
    if (on != 0) {
        Stack fr(ctx, 16);
        call(ctx, 0x0092FFC8u, {fr.addr(), w8, x});
        return;
    }
    const u32 w = load<u32>(load<u32>(load<u32>(0x00F65CA0u) + 0x1281D4u) + 4);
    if (load<s32>(o + 396) != 6) return;
    call(ctx, 0x0092F2A8u, {w, x, 1});
    const u32 y = load<u32>(o + 392);
    if (y != 0) call(ctx, 0x0092E9D8u, {w, y});
}

// bool f(T* o; f1 = a, f2 = b): true (keep going) unless the timer +568
// is <= 0 (0x001BFEDC), bit 0x20000000 of +552 is clear, b >= 0 and a >=
// b; then o->vt[420](o, 5, 0), +556 = 0 when +564 >= 0, the target def
// (Target_GetDef0D951C, when set) gets Obj_SetStateFx266E6C(it, +556 - 1
// == 0 ? 0 : 4), bit 8 of u8 +572 cleared and false. f1 = a / +564.
DECOMP(001BFEE0, Obj_CheckDone_001BFEE0) {
    const u32 o = arg<u32>(ctx, 0);
    const f64 a = farg(ctx, 0), b = farg(ctx, 1);
    const f64 k = Lfs32(0x001BFEDCu);
    const f64 t = Lfs32(o + 568);
    if (!(t < k || t == k) || (load<u32>(o + 552) & 0x20000000u) != 0 || !(b > k || b == k) || !(a > b || a == b)) {
        ret(ctx, 1u);
        return;
    }
    VCall(ctx, o, 420, {o, 5, 0});
    ctx.fpr[1] = Lfs32(o + 564);
    if (ctx.fpr[1] > k || ctx.fpr[1] == k) store<u32>(o + 556, 0);
    if (static_cast<u32>(call(ctx, 0x000D951Cu, {o})) != 0) {
        const u32 d = static_cast<u32>(call(ctx, 0x000D951Cu, {o}));
        call(ctx, 0x00266E6Cu, {d, load<u32>(o + 556) - 1u == 0 ? 0u : 4u});
    }
    store<u8>(o + 572, static_cast<u8>(load<u8>(o + 572) & ~0x08u));
    ret(ctx, 0u);
}

// void f(Volume* o, bool on): with the state +428 0 or 3 and on false, a
// set u8 +480 is cleared with o->vt[288](o, 0); then as
// Volume_SetActive_00220190 without its effect hashes: when bit 31 of
// +412 changes it becomes on, and on: Volume_Activate20C2BC(o); off:
// Volume_Notify112D8(*0x00F557EC, o + 424), Volume_Prepare20C670(o)
// unless bit 0x02000000 of +412, o->vt[96](o, 0). r3 = +412 when
// unchanged, else the last call's.
DECOMP(0020FA6C, Volume_SetActiveB_0020FA6C) {
    const u32 o = arg<u32>(ctx, 0), v = arg<u32>(ctx, 1);
    const u32 b = v & 0xFFu;
    const s32 k = load<s32>(o + 428);
    if ((k == 0 || k == 3) && b == 0 && load<u8>(o + 480) != 0) {
        store<u8>(o + 480, 0);
        VCall(ctx, o, 288, {o, 0});
    }
    u32 w = load<u32>(o + 412);
    if ((w >> 31) == b) {
        ret(ctx, w);
        return;
    }
    w = (w & 0x7FFFFFFFu) | ((v & 1u) << 31);
    store<u32>(o + 412, w);
    if (b != 0) {
        call(ctx, 0x0020C2BCu, {o});
        return;
    }
    call(ctx, 0x000112D8u, {load<u32>(0x00F557ECu), o + 424});
    if ((load<u32>(o + 412) & 0x02000000u) == 0) call(ctx, 0x0020C670u, {o});
    VCall(ctx, o, 96, {o, 0});
}

// bool f(Area* o, T* m): for m with bit 8 of +192, bit 1 of +188 set and
// bit 8 of +188 clear, which has no active part (Obj_IsPartActive_00129894)
// unless u8 o->+216->+176 is set: a zeroed 24-byte stack event
// (Event3_Init_00464F88(&ev, o, 0, 0, 0)) goes to World_Query482D14(
// *0x00F65CA0, s8 m->+172, &ev). Then Area_Enter(o, m) != 0.
DECOMP(00212C28, Area_EnterQuery_00212C28) {
    const u32 o = arg<u32>(ctx, 0), m = arg<u32>(ctx, 1);
    const u32 g = load<u32>(o + 216);
    if (m != 0 && (load<u32>(m + 192) & 8u) != 0) {
        const u32 w = load<u32>(m + 188);
        if ((w & 1u) != 0 && (w & 8u) == 0 &&
            (load<u8>(g + 176) != 0 || static_cast<u32>(call(ctx, 0x00129894u, {m})) == 0)) {
            Stack fr(ctx, 32);
            const u32 ev = fr.addr();
            for (u32 k = 0; k < 24; k += 4) store<u32>(ev + k, 0);
            call(ctx, 0x00464F88u, {ev, o, 0, 0, 0});
            const u32 id = static_cast<u32>(static_cast<s32>(static_cast<s8>(load<u8>(m + 172))));
            call(ctx, 0x00482D14u, {load<u32>(0x00F65CA0u), id, ev});
        }
    }
    ret(ctx, static_cast<u32>(call(ctx, 0x0020E7A4u, {o, m})) != 0 ? 1u : 0u);
}

// void f(T* o, Obj* m): for the kind-5 component c 0x2C73353E of the
// effect o->+428 (Effect_FindComponent): the old target c->+28 gets
// vt[8](), c->+28 = m->+160 (0 without m; its u16 +4 count increased)
// and c->+36 = m->+164 (or 0). With the effect, Effect_SetHash4CF988(it,
// 0xF89061F1) (r3 = its; r3 = o without one).
DECOMP(00220024, Volume_SetTarget_00220024) {
    const u32 o = arg<u32>(ctx, 0), m = arg<u32>(ctx, 1);
    const u32 e = load<u32>(o + 428);
    if (e == 0) return;
    const u32 c = static_cast<u32>(call(ctx, 0x004BB4B4u, {e, 0x2C73353Eu}));
    if (c != 0 && static_cast<s32>(VCall(ctx, c, 32, {c})) == 5) {
        const u32 nt = m != 0 ? load<u32>(m + 160) : 0u;
        const u32 p = load<u32>(c + 28);
        if (p != 0) VCall(ctx, p, 8, {p});
        store<u32>(c + 28, nt);
        if (nt != 0) store<u16>(nt + 4, static_cast<u16>(load<u16>(nt + 4) + 1));
        store<u32>(c + 36, m != 0 ? load<u32>(m + 164) : 0u);
    }
    call(ctx, 0x004CF988u, {load<u32>(o + 428), 0xF89061F1u});
}

// u32 f(Volume* o): with the effect key h = o->+440 set and the
// definition +432 or the effect +428 missing: a set effect gets bit 0x200
// of +136 and +428 = 0; +432 = EffectDef_Find(h, 0, 1) and, when found,
// +428 = Obj_CreateEffectDCA20(o, def, 0, 1); a new effect gets bit
// 0x200 of +136 cleared, o->+468 bit 0x20000000 set, and its component
// 0x70D68D3A (Effect_FindComponent), when its vt[32]() is 15, +20 = o.
// r3 as the original leaves it (h when nothing is done).
DECOMP(002202BC, Volume_EnsureEffect_002202BC) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 h = load<u32>(o + 440), def = load<u32>(o + 432), eff = load<u32>(o + 428);
    ctx.gpr[3] = h;
    if (h == 0 || (def != 0 && eff != 0)) return;
    u32 key = h;
    if (eff != 0) {
        store<u32>(eff + 136, load<u32>(eff + 136) | 0x200u);
        store<u32>(o + 428, 0);
        key = load<u32>(o + 440);
    }
    const u32 d = static_cast<u32>(call(ctx, 0x004BDEDCu, {key, 0, 1}));
    store<u32>(o + 432, d);
    if (d == 0) return;
    const u32 e = static_cast<u32>(call(ctx, 0x000DCA20u, {o, d, 0, 1}));
    store<u32>(o + 428, e);
    if (e == 0) return;
    store<u32>(e + 136, load<u32>(e + 136) & ~0x200u);
    const u32 w = load<u32>(o + 468) | 0x20000000u;
    const u32 e2 = load<u32>(o + 428);
    store<u32>(o + 468, w);
    const u32 c = static_cast<u32>(call(ctx, 0x004BB4B4u, {e2, 0x70D68D3Au}));
    if (c == 0) return;
    if (static_cast<s32>(VCall(ctx, c, 32, {c})) == 15) store<u32>(c + 20, o);
}

// void f(Section* o, u32 mode, bool force): unless mode is o's u8 +18
// already (without force): mode 5 / 4 send vt[28](x, 1, 0 / 7) to the
// object x of the 0x20C80-byte record o->+2584 of o->+8 (its +4216, when
// set), and mode 4 also copies +2580 to +2576 (raw; f1). Then
// Section_SetModeBase_003260F0(o, mode, force) (r3 = its).
DECOMP(0032D8E4, Section_SetMode_0032D8E4) {
    const u32 o = arg<u32>(ctx, 0), mode = arg<u32>(ctx, 1);
    const u32 force = arg<u32>(ctx, 2) & 0xFFu;
    if ((force != 0 || mode != load<u8>(o + 18)) && (mode == 4 || mode == 5)) {
        const u32 x = load<u32>(load<u32>(o + 8) + load<u32>(o + 2584) * 0x20C80u + 4208 + 8);
        if (x != 0) VCall(ctx, x, 28, {x, 1, mode == 4 ? 7u : 0u});
        if (mode == 4) {
            const u32 v = load<u32>(o + 2580);
            store<u32>(o + 2576, v);
            ctx.fpr[1] = static_cast<f64>(std::bit_cast<f32>(v));
        }
    }
    call(ctx, 0x003260F0u, {o, mode, force});
}

// void f(T* o): registers o in the sorted map 0x0106C240 (u16 capacity
// +0, count +2, high-water +4, flags +6, 8-byte entries {key, T*} at +8)
// under its key (o->+12, else o->+8->+68 or 0) unless SortedMap_Find3
// finds one already (r3 = it): appended (capacity doubled by
// Array_Reserve, masked to 16 bits, when full) and bit 0 of the flags set
// (r3 = the flags).
DECOMP(00362BEC, Map_Register_00362BEC) {
    const u32 o = arg<u32>(ctx, 0);
    if (o == 0) return;
    constexpr u32 M = 0x0106C240u;
    auto key = [&]() {
        const u32 k = load<u32>(o + 12);
        if (k != 0) return k;
        const u32 p = load<u32>(o + 8);
        return p != 0 ? load<u32>(p + 68) : 0u;
    };
    {
        Stack fr(ctx, 16);
        store<u32>(fr.addr(), key());
        const u32 f = load<u32>(static_cast<u32>(call(ctx, 0x0076611Cu, {M, fr.addr()})));
        if (f != 0) {
            ret(ctx, f);
            return;
        }
    }
    const u32 k = key();
    u32 cnt = load<u16>(M + 2);
    const u32 cap = load<u16>(M);
    if (cnt == cap) {
        call(ctx, 0x00558F3Cu, {M, (cap << 1) & 0xFFFEu});
        cnt = load<u16>(M + 2);
    }
    const u32 d = load<u32>(M + 8);
    const u32 c2 = load<u16>(M + 2);
    store<u32>(d + ((cnt << 3) & 0x7FFF8u), k);
    const u32 hw = load<u16>(M + 4);
    store<u32>(d + (c2 << 3) + 4, o);
    const u32 n1 = c2 + 1u;
    const u32 fl = load<u16>(M + 6) | 1u;
    store<u16>(M + 2, static_cast<u16>(n1));
    store<u16>(M + 4, static_cast<u16>(static_cast<s32>(hw) > static_cast<s32>(n1 & 0xFFFFu) ? hw : n1));
    store<u16>(M + 6, static_cast<u16>(fl));
    ret(ctx, fl);
}
