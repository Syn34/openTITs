// Game code, part 26: the continued sweep of small still-recompiled game
// functions (in size order, from 32 instructions up). Most have no known
// caller context yet, so their names describe what they do and carry the
// address. Each was checked against the original in the PPC simulator.
//
// Register notes: where a known caller reads a volatile register the
// original leaves behind (regflow liveout), the value is set the same way,
// as the comments say ("r4 left ...").
#include "obj26.h"

using namespace sly4;
using namespace sly4::obj26;

// ---------------------------------------------------------------- sweep: 32 instructions
// u32 f(T* o, Msg* m, s32* out): the +388 match, else 0x000DA85C
DECOMP(0017BFDC, Obj_OnMsg388_0017BFDC) { MsgMatchOrBase(ctx, 388); }
// u32 f(T* o, Msg* m, s32* out): the +416 match, else 0x000DA85C
DECOMP(001FD57C, Obj_OnMsg416_001FD57C) { MsgMatchOrBase(ctx, 416); }
// u32 f(T* o, Msg* m, s32* out): the +388 match, else 0x000DA85C
DECOMP(00208D60, Obj_OnMsg388_00208D60) { MsgMatchOrBase(ctx, 388); }

// u32 f(T* o, Msg* m): when 0x000E0BCC(o, m) is nonzero, either sets bit
// 0x20000000 of +468 (when +468 has 0x40000000 and the u16 m->+12 has
// 0x20; returns the new word) or returns 0x000E8678(o, m); else 0
DECOMP(001AC0D4, Obj_OnMsgFlag468_001AC0D4) {
    const u32 o = arg<u32>(ctx, 0), m = arg<u32>(ctx, 1);
    if (static_cast<s32>(call(ctx, 0x000E0BCC, {o, m})) == 0) {
        ctx.gpr[3] = 0;
        return;
    }
    const u32 v = load<u32>(o + 468);
    if ((v & 0x40000000u) != 0 && (load<u16>(m + 12) & 0x20u) != 0) {
        store<u32>(o + 468, v | 0x20000000u);
        ctx.gpr[3] = v | 0x20000000u;
        return;
    }
    ctx.gpr[3] = call(ctx, 0x000E8678, {o, m});
}

// u32 f(T* o, s32 key): the index of key in the 7-word table at
// 0x00AD139C; when found, o->vfn[420](o, index, 0) (else r3 = o)
DECOMP(001AE9C8, Obj_CallByKey420_001AE9C8) {
    constexpr u32 kTable = 0x00AD139C;
    const u32 o = arg<u32>(ctx, 0), key = arg<u32>(ctx, 1);
    for (u32 i = 0; i < 7; ++i) {
        if (load<u32>(kTable + 4 * i) == key) {
            ctx.gpr[3] = VCall(ctx, o, 420, {o, i, 0});
            return;
        }
    }
}

// T::~T: vtables 0x00B17270 / 0x00B17444 (+380) / 0x00B1746C (+384), the
// u16 +820 = 0, ~PtrVector(+812), then the base destructor 0x001AE468
DECOMP(001CBE10, VtB17270_Dtor) {
    const u32 o = arg<u32>(ctx, 0);
    store<u16>(o + 820, 0);
    store<u32>(o, 0x00B17270);
    store<u32>(o + 380, 0x00B17444);
    store<u32>(o + 384, 0x00B1746C);
    VecDtor(ctx, o + 812);
    call(ctx, 0x001AE468, {o});
}

// T::~T deleting form: vtable 0x00B19148, AnimSkel_Release(+200),
// RefNode_Release(+204), then the 0x00B11D98 base and Mem_FreeFrom(o, 0).
// Null does nothing (r3 = 0).
DECOMP(00203678, VtB19148_Delete) {
    const u32 o = arg<u32>(ctx, 0);
    if (o == 0) {
        ctx.gpr[3] = 0;
        return;
    }
    const u32 skel = load<u32>(o + 200);
    store<u32>(o, 0x00B19148);
    call(ctx, o26fn::AnimSkel_Release, {skel});
    call(ctx, o24fn::RefNode_Release, {load<u32>(o + 204)});
    DeleteTailB11D98(ctx, o);
}

// bool f(T* o, a, b, c) (f1 passed on): when the f32 +348 is above the
// constant at 0x00208A64, 0x000E1738(o, b, b, c, f1); then whether +348
// (read again) is still above it
DECOMP(00208A68, Obj_TickIfAbove348_00208A68) {
    const u32 o = arg<u32>(ctx, 0), b = arg<u32>(ctx, 2), c = arg<u32>(ctx, 3);
    const f32 k = load<f32>(0x00208A64);
    ctx.gpr[4] = b;
    if (load<f32>(o + 348) > k) callf(ctx, 0x000E1738, {o, b, b, c}, {farg(ctx, 0)});
    ctx.gpr[3] = load<f32>(o + 348) > k ? 1u : 0u;
}

// u32 f(T* o, u8 a, u8 b): 0x000DA348(o, a, b, 1, 0); when o->+420 = p is
// set, the u8 p->+75 = a and the u8 +103 of the 128-byte record p->+240 in
// the table 0x011950AC->* = a (returns that record; else 0)
DECOMP(0020B8DC, Obj_SetByteAndRecord_0020B8DC) {
    const u32 o = arg<u32>(ctx, 0), a = arg<u32>(ctx, 1) & 0xFFu, b = arg<u32>(ctx, 2) & 0xFFu;
    call(ctx, 0x000DA348, {o, a, b, 1, 0});
    const u32 p = load<u32>(o + 420);
    ctx.gpr[3] = p;
    if (p == 0) return;
    const u32 idx = load<u32>(p + 240);
    store<u8>(p + 75, static_cast<u8>(a));
    const u32 rec = load<u32>(0x011950A0 + 12) + (idx << 7);
    store<u8>(rec + 103, static_cast<u8>(a));
    ctx.gpr[3] = rec;
}

// The XML load shape (cursor, a, b, node, out): fn(cursor, a, b, node,
// out); out->+32 = 1; returns 1 when node is null, the bit (node->+24 &
// mask) when set, else XmlCursor_DocPath(cursor).
static void XmlLoadThen(PPUContext& ctx, u32 fn, u32 mask) {
    const u32 cur = arg<u32>(ctx, 0), node = arg<u32>(ctx, 3), out = arg<u32>(ctx, 4);
    call(ctx, fn, {cur, ctx.gpr[4], ctx.gpr[5], node, out});
    store<u32>(out + 32, 1);
    ctx.gpr[3] = 1;
    if (node == 0) return;
    const u32 bit = load<u32>(node + 24) & mask;
    ctx.gpr[3] = bit;
    if (bit != 0) return;
    ctx.gpr[3] = call(ctx, o26fn::XmlCursor_DocPath, {cur});
}
// u32 f(cursor, a, b, node, out): 0x0021DEE0, then the bit-0x1 check
DECOMP(00221760, Xml_Load_00221760) { XmlLoadThen(ctx, 0x0021DEE0, 0x1u); }
// u32 f(cursor, a, b, node, out): 0x0020BFE8, then the bit-0x80 check
DECOMP(0022515C, Xml_Load_0022515C) { XmlLoadThen(ctx, 0x0020BFE8, 0x80u); }

// T::~T deleting form: when +736 is set, vtable 0x00B1B4C8 and
// Mem_FreeIfSet(+736, +732) (the vtable store is skipped otherwise); then
// the 0x00B11D98 base and Mem_FreeFrom(o, 0). Null does nothing (r3 = 0).
DECOMP(00226EE8, VtB1B4C8_Delete) {
    const u32 o = arg<u32>(ctx, 0);
    if (o == 0) {
        ctx.gpr[3] = 0;
        return;
    }
    const u32 p = load<u32>(o + 736);
    if (p != 0) {
        const u32 a = load<u32>(o + 732);
        store<u32>(o, 0x00B1B4C8);
        call(ctx, o26fn::Mem_FreeIfSet, {p, a});
    }
    DeleteTailB11D98(ctx, o);
}

// void f(f32 dst[3], T* o): dst = the 3 words at o->+612->+24 + 320 when
// both are set, else the 3 words at 0x01164B40 (r3 / f1 left as the
// original's last copy leaves them)
DECOMP(0023C65C, Obj_GetVec612_0023C65C) {
    const u32 d = arg<u32>(ctx, 0), o = arg<u32>(ctx, 1);
    u32 s = 0x01164B40;
    u32 q = load<u32>(o + 612);
    if (q != 0) q = load<u32>(q + 24);
    if (q != 0) {
        s = q + 320;
        ctx.gpr[3] = d + 8;
    } else {
        ctx.gpr[3] = d;
    }
    Cp(d, s);
    Cp(d + 4, s + 4);
    Cp(d + 8, s + 8);
    ctx.fpr[1] = LfsAt(s + 8);
}

// void f(T* o, const f32 v[3], f32 a, f32 b): +576 = v, +592 = a, +596 =
// b, +600 = 0, the u8 +604 = 1; +592 = 1.0f when a is at most the constant
// at 0x002448F8 (then f1 = the constant at 0x002448FC), and again when f1
// is at most it
DECOMP(00244900, Obj_SetVecAB_00244900) {
    constexpr u32 kOne = 0x3F800000;
    const u32 o = arg<u32>(ctx, 0), v = arg<u32>(ctx, 1);
    f64 a = farg(ctx, 0);
    const f64 b = farg(ctx, 1);
    Cp(o + 576, v);
    Cp(o + 580, v + 4);
    const u32 v2 = load<u32>(v + 8);
    store<f32>(o + 592, static_cast<f32>(a));
    store<u32>(o + 584, v2);
    store<f32>(o + 596, static_cast<f32>(b));
    const f64 k = LfsAt(0x002448F8);
    if (a <= k) {
        a = LfsAt(0x002448FC);
        store<u32>(o + 592, kOne);
    }
    store<u32>(o + 600, 0);
    store<u8>(o + 604, 1);
    ctx.gpr[4] = 0;
    ctx.gpr[5] = 1;
    if (a <= k) {
        store<u32>(o + 592, kOne);
        ctx.gpr[4] = kOne;
    }
    ctx.fpr[1] = a;
    ctx.fpr[2] = k;
    ctx.fpr[3] = static_cast<f64>(std::bit_cast<f32>(v2));
}

// The deleting destructor shape: when +poff is set, vtable vt and
// Mem_FreeIfSet(+poff, +aoff) (the vtable store is skipped otherwise); then
// the 0x00B11D98 base and SmallFree(o, size). Null does nothing (r3 = 0).
static void DeleteFreeIfSetSmall(PPUContext& ctx, u32 vt, u32 poff, u32 aoff, u32 size) {
    const u32 o = arg<u32>(ctx, 0);
    if (o == 0) {
        ctx.gpr[3] = 0;
        return;
    }
    const u32 p = load<u32>(o + poff);
    if (p != 0) {
        const u32 a = load<u32>(o + aoff);
        store<u32>(o, vt);
        call(ctx, o26fn::Mem_FreeIfSet, {p, a});
    }
    store<u32>(o, 0x00B11D98);
    call(ctx, o26fn::Obj_BaseDtor_0C2DA0, {o});
    call(ctx, 0x00552420, {o, size});
}
// T::~T deleting form (1040 bytes): vtable 0x00B1B4C8 and
// Mem_FreeIfSet(+736, +732), then the 0x00B11D98 base
DECOMP(00244C54, VtB1B4C8_Delete1040) { DeleteFreeIfSetSmall(ctx, 0x00B1B4C8, 736, 732, 1040); }

// s32 f(T* o, a): 0x0075EBFC(a, o + 112 + 128 i) for each i below the s32
// +2160 (read again each time); returns the count
DECOMP(0029E7A0, Obj_ForEach128_0029E7A0) {
    const u32 o = arg<u32>(ctx, 0), a = arg<u32>(ctx, 1);
    s32 n = load<s32>(o + 2160);
    for (s32 i = 0; i < n; ++i) {
        call(ctx, 0x0075EBFC, {a, o + 112 + 128u * static_cast<u32>(i)});
        n = load<s32>(o + 2160);
    }
    ctx.gpr[3] = static_cast<u32>(n);
}

// u32 f(_, T* p): when p, p->+48 and p->+64 are set and p->+48->+164 has
// bit 0x8000, PairBuffer_Push(world->+0x1285E8->+8, 0x00F3C860, p->+64);
// r3 is the last value tested otherwise
DECOMP(002AB76C, Obj_PushPair_002AB76C) {
    const u32 p = arg<u32>(ctx, 1);
    ctx.gpr[3] = p;
    if (p == 0) return;
    const u32 q = load<u32>(p + 48);
    if (q == 0) return;
    if ((load<u32>(q + 164) & 0x8000u) == 0) return;
    const u32 r = load<u32>(p + 64);
    ctx.gpr[3] = r;
    if (r == 0) return;
    const u32 w = load<u32>(kWorldPtr);
    ctx.gpr[3] = call(ctx, 0x00564FF8, {load<u32>(load<u32>(w + 0x1285E8u) + 8), 0x00F3C860, r});
}

// u32 f(T* o, Event* e) for the subobject at o - 24: event 0x2FBD3D56 ->
// 0x002B0480(o - 24, e->+0, e->+12); event 0x340866A4 -> (o - 24)->+76 = 0
// (r3 = 0; 0x340866A4 for higher events, e for lower ones)
DECOMP(002B0654, Obj_OnEvent_002B0654) {
    const u32 t = arg<u32>(ctx, 0) - 24, e = arg<u32>(ctx, 1);
    const s32 h = load<s32>(e + 8);
    ctx.gpr[3] = e;
    ctx.gpr[4] = t;
    ctx.gpr[5] = static_cast<u32>(h);
    ctx.gpr[6] = 0x2FBD3D56;
    if (h > 0x2FBD3D56) {
        ctx.gpr[3] = 0x340866A4;
        if (h != 0x340866A4) return;
        store<u32>(t + 76, 0);
        ctx.gpr[3] = 0;
        return;
    }
    ctx.gpr[6] = 0x2FBD3D55;
    if (h > 0x2FBD3D55) ctx.gpr[3] = call(ctx, 0x002B0480, {t, load<u32>(e), load<u32>(e + 12)});
}

// bool f(T* o, Rec* r): finds the 76-byte record in the u16 +436 records
// at +428 whose +68 equals r->+68 and copies 74 bytes of it to r
DECOMP(002D9994, Obj_FindRecord76_002D9994) {
    const u32 o = arg<u32>(ctx, 0), r = arg<u32>(ctx, 1);
    const u32 n = load<u16>(o + 436);
    u32 p = load<u32>(o + 428);
    for (u32 i = 0; i < n; ++i, p += 76) {
        if (load<u32>(p + 68) == load<u32>(r + 68)) {
            call(ctx, 0x0071C3F0, {r, p, 74});  // memmove
            ctx.gpr[3] = 1;
            return;
        }
    }
    ctx.gpr[3] = 0;
}

// T::~T: when +8 is set and the u8 +25 too, vtable 0x00B1F338 and
// (+8)->vfn[8](+8); then +8 = 0 and vtable 0x00B13974
DECOMP(003431C0, VtB1F338_Dtor) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 p = load<u32>(o + 8);
    if (p != 0 && load<u8>(o + 25) != 0) {
        const u32 vt = load<u32>(p);  // read before the store
        store<u32>(o, 0x00B1F338);
        VCallVt(ctx, vt, 8, {p});
    }
    store<u32>(o + 8, 0);
    store<u32>(o, 0x00B13974);
    ctx.gpr[3] = 0x00B13974;
    ctx.gpr[4] = 0;
}

// bool f(T* o): o->vfn[16](&ok, o); unless ok (u8) and +8 are set,
// returns 1; else the u64 +8->+264 is copied to +16 and 0 returned
DECOMP(00347CB8, Obj_Fetch264_00347CB8) {
    const u32 o = arg<u32>(ctx, 0);
    Stack frame(ctx, 16);
    VCall(ctx, o, 16, {frame.addr(), o});
    const u32 p = load<u32>(o + 8);
    if (load<u8>(frame.addr()) == 0 || p == 0) {
        ctx.gpr[3] = 1;
        return;
    }
    const u64 v = load<u64>(p + 264);
    store<u64>(o + 16, v);
    ctx.gpr[4] = v;
    ctx.gpr[3] = 0;
}

// u32 f(T* o, a, b): the index of the (a, b) pair in the 8-byte pairs at
// +80 (count: the u16 +72->+2); when found, 0x00770060(o + 68, index)
// (else r3 = o)
DECOMP(0034F580, Obj_FindPair_0034F580) {
    const u32 o = arg<u32>(ctx, 0), a = arg<u32>(ctx, 1), b = arg<u32>(ctx, 2);
    const s32 n = load<u16>(load<u32>(o + 72) + 2);
    for (s32 i = 0; i < n; ++i) {
        const u32 e = load<u32>(o + 80) + 8u * static_cast<u32>(i);
        if (load<u32>(e) == a && load<u32>(e + 4) == b) {
            ctx.gpr[3] = call(ctx, 0x00770060, {o + 68, static_cast<u32>(i)});
            return;
        }
    }
}

// void f(T* o): unless the u8 +212 is set: o->vfn[20](o, 0),
// Obj_Place3626E8(o, 0x01164B40, 1) and +160 = (+160 & 1) | 1
DECOMP(0036C818, Obj_Reset212_0036C818) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 f = load<u8>(o + 212);
    ctx.gpr[3] = f;
    if (f != 0) return;
    VCall(ctx, o, 20, {o, 0});
    call(ctx, 0x003626E8, {o, 0x01164B40, 1});
    const u32 v = (load<u32>(o + 160) & 1u) | 1u;
    store<u32>(o + 160, v);
    ctx.gpr[3] = v;
}

// u32 f(T* o): an offset into the 132-byte records at +152: record +146's
// u16 count c gives s = (12 c + 23) & ~15, record +144's base plus s plus
// the 16-aligned 8 d + 21 for the u16 d at s in record +146, plus the u16
// at 0x00F8F0EC, plus 152
DECOMP(0038B990, Obj_RecordOffset_0038B990) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 a = load<u16>(o + 146) * 132u, b = load<u16>(o + 144);
    const u32 c = load<u16>(o + a + 152);
    const u32 s = (c * 12u + 23u) & ~15u;
    const u32 d = load<u16>(o + a + s + 152);
    const u32 g = load<u16>(0x00F8F0EC);
    const u32 r4 = b * 132u + s;
    ctx.gpr[4] = r4;
    ctx.gpr[5] = g;
    ctx.gpr[6] = b * 4u;
    ctx.gpr[7] = 0x00F90000;
    ctx.gpr[3] = r4 + ((((d << 3) & 0x7FFF8u) + 21u) & ~15u) + g + 152u;
}

// u32 f(Ref* a, T* o): the 132-byte record (s16 a->+6) at o + 152 when
// that is below the u16 +144 and its first word is below 10 (else 0), then
// Field_Inc20_00446170(*(that + the u16 at 0x0106CF80 + 4))
DECOMP(00394174, Obj_IncRecordField_00394174) {
    const u32 a = arg<u32>(ctx, 0), o = arg<u32>(ctx, 1);
    const s32 i = load<s16>(a + 6);
    u32 r = 0;
    if (i < static_cast<s32>(load<u16>(o + 144))) {
        r = o + 152 + static_cast<u32>(i) * 132u;
        if (!(load<s32>(r) < 10)) r = 0;
    }
    ctx.gpr[3] = call(ctx, 0x00446170, {load<u32>(r + load<u16>(0x0106CF80) + 4)});
}

// T::T(o, a, b, u8 c, u8 d): the 0x00B20CC8 base (Obj_InitVt_B20CC8(o,
// 1)), +12 = a, +16 = b, +20 = c, +21 = d, vtable 0x00B20E60
DECOMP(0042EA6C, VtB20E60_Ctor) {
    const u32 o = arg<u32>(ctx, 0), a = arg<u32>(ctx, 1), b = arg<u32>(ctx, 2);
    const u32 c = arg<u32>(ctx, 3), d = arg<u32>(ctx, 4);
    call(ctx, o24fn::Obj_InitVt_B20CC8, {o, 1});
    store<u32>(o + 12, a);
    store<u32>(o + 16, b);
    store<u8>(o + 20, static_cast<u8>(c));
    store<u8>(o + 21, static_cast<u8>(d));
    store<u32>(o, 0x00B20E60);
    ctx.gpr[3] = 0x00B20E60;
}

// bool f(T* o): with an effect at +44: unless Effect_Check4CF620(it, 0)
// is 0, Effect_Release(it, 0x10CBB), its +136 |= 0x200 and +44 = 0;
// returns 0 (1 when there is none)
DECOMP(004333A4, Obj_ReleaseEffect44_004333A4) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 e = load<u32>(o + 44);
    if (e == 0) {
        ctx.gpr[3] = 1;
        return;
    }
    if (static_cast<s32>(call(ctx, 0x004CF620, {e, 0})) != 0) {
        call(ctx, 0x004CF3C4, {load<u32>(o + 44), 0x10CBB});
        const u32 p = load<u32>(o + 44);
        const u32 f = load<u32>(p + 136) | 0x200u;
        store<u32>(p + 136, f);
        store<u32>(o + 44, 0);
        ctx.gpr[4] = 0;
        ctx.gpr[5] = f;
    }
    ctx.gpr[3] = 0;
}

// void f(T* o): +12 = FindFlagged_00368DB0(0xF6B16611), then
// 0x003679C8(it, 0, 1, r6 as that call leaves it, 0, 0, 0x01164CA4, f1 =
// the constant at 0x004334EC) and Flag_Set8_40000000(o)
DECOMP(004334F0, Obj_Start_004334F0) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 p = static_cast<u32>(call(ctx, 0x00368DB0, {0xF6B16611u}));
    store<u32>(o + 12, p);
    // r6 is not set by the original (0x003679C8 does not read it)
    callf(ctx, 0x003679C8, {p, 0, 1, ctx.gpr[6], 0, 0, 0x01164CA4}, {LfsAt(0x004334EC)});
    ctx.gpr[3] = call(ctx, o24fn::Flag_Set8_40000000, {o});
}

// void f(T* o): Fade_Start(world + 0x128290, o + 9, o + 13, r6 as passed
// in, the top bit of +24, 0, f1 = the f32 +20), then
// Flag_Set8_40000000(o)
DECOMP(00434238, Obj_StartFade_00434238) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 w = load<u32>(kWorldPtr);
    const u32 top = load<u32>(o + 24) >> 31;
    // r6 is not set by the original (Fade_Start does not read it)
    callf(ctx, 0x00468C40, {w + 0x128290u, o + 9, o + 13, ctx.gpr[6], top, 0}, {LfsAt(o + 20)});
    ctx.gpr[3] = call(ctx, o24fn::Flag_Set8_40000000, {o});
}

// bool f(T* o): with p = +12: when p->+24 or p->+16 is set returns 0;
// otherwise p->vfn[4](p) and +12 = 0; returns 1 (also when p is null)
DECOMP(00438B00, Obj_DropIdle12_00438B00) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 p = load<u32>(o + 12);
    if (p != 0) {
        if (load<u32>(p + 24) != 0 || load<u32>(p + 16) != 0) {
            ctx.gpr[3] = 0;
            return;
        }
        VCall(ctx, p, 4, {p});
        store<u32>(o + 12, 0);
    }
    ctx.gpr[3] = 1;
}

// u32 f(u32 id, u8 b): when the u8 0x010AEC60 is set,
// 0x0044CEE8(index, b) for the index of id (0x782CF148 -> 2, 0x0464F605
// -> 1, 0x00014065 -> 0, else -1); otherwise r3 = id
DECOMP(0044D168, Id_Lookup_0044D168) {
    const u32 id = arg<u32>(ctx, 0), b = arg<u32>(ctx, 1) & 0xFFu;
    ctx.gpr[4] = b;
    const u32 f = load<u8>(0x010AEC60);
    ctx.gpr[5] = f;
    if (f == 0) return;
    s32 k;
    if (id == 0x782CF148u) {
        k = 2;
        ctx.gpr[5] = 0x782CF148u;
    } else if (id == 0x0464F605u) {
        k = 1;
        ctx.gpr[5] = 0x0464F605u;
    } else {
        ctx.gpr[5] = 0x00014065u;
        k = id == 0x00014065u ? 0 : -1;
    }
    ctx.gpr[3] = call(ctx, 0x0044CEE8, {Sx(static_cast<u32>(k)), b});
}

// bool f(Iff* f): the static u8 0x010B1690 (guard 0x010B1688) = 6,
// Iff_ReadU8(f, &it, the name at 0x00B218C8), then the s32 0x00F65B3C =
// it; returns 1
DECOMP(0044F040, Iff_ReadByteSetting_0044F040) {
    constexpr u32 kGuard = 0x010B1688, kVal = 0x010B1690;
    const u32 f = arg<u32>(ctx, 0);
    if (load<u8>(kGuard) == 0) {
        store<u8>(kVal, 0);
        store<u8>(kGuard, 1);
    }
    store<u8>(kVal, 6);
    call(ctx, 0x005643C4, {f, kVal, 0x00B218C8});
    const u32 v = load<u8>(kVal);
    store<u32>(0x00F65B3C, v);
    ctx.gpr[5] = v;
    ctx.gpr[3] = 1;
}

// The event record shape: r->+20 = o, +0 = kind, +4 = the f32 world->+12->+0,
// +8 = o->+216->+52, +12 = o->+168, +16 = o->+336->+168 (0 where a link
// is null); returns r
static void FillEventRec(PPUContext& ctx, u32 kind) {
    const u32 r = arg<u32>(ctx, 0), o = arg<u32>(ctx, 1);
    store<u32>(r + 20, o);
    const u32 w = load<u32>(kWorldPtr);
    store<u32>(r, kind);
    const u32 t = load<u32>(w + 12);
    const u32 tv = load<u32>(t);
    store<u32>(r + 4, tv);
    ctx.fpr[1] = static_cast<f64>(std::bit_cast<f32>(tv));
    u32 a = 0, b = 0, c = 0;
    if (o != 0) {
        const u32 p = load<u32>(o + 216);
        if (p != 0) a = load<u32>(p + 52);
    }
    store<u32>(r + 8, a);
    if (o != 0) b = load<u32>(o + 168);
    store<u32>(r + 12, b);
    if (o != 0) {
        const u32 q = load<u32>(o + 336);
        if (q != 0) c = load<u32>(q + 168);
    }
    store<u32>(r + 16, c);
    ctx.gpr[5] = c;
    ctx.gpr[6] = b;
    ctx.gpr[7] = a;
    ctx.gpr[8] = w;
    ctx.gpr[3] = r;
}
// void f(Rec* r, T* o): the event record of kind 29
DECOMP(00465AB0, EventRec_Fill29_00465AB0) { FillEventRec(ctx, 29); }
// void f(Rec* r, T* o): the event record of kind 30
DECOMP(00465B30, EventRec_Fill30_00465B30) { FillEventRec(ctx, 30); }

// u32 f(o, a4, a5, a6, a7, a8, a9, a10): when o->+0x12847C = p is set,
// 0x00424FB8(p, a4, a5, a7, a7, a8, a9, a10) (f1 passed on); else a10
DECOMP(00489DF8, World_Forward12847C_00489DF8) {
    const u32 p = load<u32>(arg<u32>(ctx, 0) + 0x12847Cu);
    const u64 a4 = ctx.gpr[4], a5 = ctx.gpr[5], a7 = ctx.gpr[7], a8 = ctx.gpr[8], a9 = ctx.gpr[9], a10 = ctx.gpr[10];
    if (p == 0) {
        ctx.gpr[3] = a10;
        return;
    }
    callf(ctx, 0x00424FB8, {p, a4, a5, a7, a7, a8, a9, a10}, {farg(ctx, 0)});  // r3 as it returns
}

// The deleting destructor shape vt1, ~PtrVector(+off), vt2, then free_fn(o,
// size). Null does nothing (r3 = 0).
static void DeleteVecVt2(PPUContext& ctx, u32 vt1, u32 off, u32 vt2, u32 free_fn, u32 size) {
    const u32 o = arg<u32>(ctx, 0);
    if (o == 0) {
        ctx.gpr[3] = 0;
        return;
    }
    store<u32>(o, vt1);
    VecDtor(ctx, o + off);
    store<u32>(o, vt2);
    call(ctx, free_fn, {o, size});
}
// T::~T deleting form: vtable 0x00B22498, ~PtrVector(+12), vtable
// 0x00B11B20, Mem_FreeFrom(o, 0)
DECOMP(004CBDB8, VtB22498_Delete) { DeleteVecVt2(ctx, 0x00B22498, 12, 0x00B11B20, o25fn::Mem_FreeFrom, 0); }
// T::~T deleting form (56 bytes): vtable 0x00B11DE0, ~PtrVector(+12),
// vtable 0x00B11DC4
DECOMP(00743984, VtB11DE0_Delete) { DeleteVecVt2(ctx, 0x00B11DE0, 12, 0x00B11DC4, 0x00552420, 56); }
// T::~T deleting form (96 bytes): vtable 0x00B1BEC8, ~PtrVector(+64),
// vtable 0x00B1BE78
DECOMP(00762C38, VtB1BEC8_Delete) { DeleteVecVt2(ctx, 0x00B1BEC8, 64, 0x00B1BE78, 0x00552420, 96); }

// T* f(a, b): new (36 bytes, align 16) T, constructed by 0x004F75B4(p, b,
// a); 0 when the allocation fails
DECOMP(004F7778, New36_004F7778) {
    const u32 a = arg<u32>(ctx, 0), b = arg<u32>(ctx, 1);
    const u32 p = static_cast<u32>(call(ctx, kMemAlloc, {36, 0, 16}));
    if (p != 0) call(ctx, 0x004F75B4, {p, b, a});
    ctx.gpr[3] = p;
}

// f32 floorf(f32 x) (the f32 floor): x itself when already integral or
// at least 2^23 in magnitude; r3 / r4 left as the original's
DECOMP(00748C50, Math_Floorf_00748C50) {
    const f64 x = farg(ctx, 0);
    s64 t;
    if (std::isnan(x) || x <= -9223372036854775808.0) t = INT64_MIN;
    else if (x >= 9223372036854775808.0) t = INT64_MAX;
    else t = static_cast<s64>(x);
    const f32 f2 = static_cast<f32>(static_cast<f64>(t));
    const s32 w = static_cast<s32>(rt::fctiw(x, true));
    ctx.gpr[4] = static_cast<u32>(w);
    const u32 d = std::bit_cast<u32>(static_cast<f32>(static_cast<f64>(f2) - x));
    ctx.gpr[3] = d;
    if (d == 0) return;
    const u32 xb = std::bit_cast<u32>(static_cast<f32>(x));
    ctx.gpr[3] = xb;
    ctx.gpr[5] = 0x4B800000;
    ctx.gpr[6] = xb & 0x7F800000u;
    if (!(static_cast<s32>(xb & 0x7F800000u) < 0x4B800000)) return;
    if ((xb & 0x80000000u) != 0) {
        const s32 m = static_cast<s32>(static_cast<u32>(w) - 1u);
        ctx.gpr[3] = Sx(static_cast<u32>(m));
        ctx.fpr[1] = static_cast<f64>(static_cast<f32>(static_cast<f64>(m)));
    } else {
        ctx.gpr[3] = 0;
        ctx.fpr[1] = static_cast<f64>(f2);
    }
}

// void f(T* o, Ref* p): releases the old +28 (vfn[8]) and stores p,
// adding one to the u16 p->+4
DECOMP(0074C8B4, Obj_SetRef28_0074C8B4) {
    const u32 o = arg<u32>(ctx, 0), p = arg<u32>(ctx, 1);
    const u32 q = load<u32>(o + 28);
    ctx.gpr[3] = q;
    if (q != 0) ctx.gpr[3] = VCall(ctx, q, 8, {q});
    store<u32>(o + 28, p);
    if (p == 0) return;
    const u32 c = static_cast<u16>(load<u16>(p + 4) + 1);
    store<u16>(p + 4, static_cast<u16>(c));
    ctx.gpr[3] = p + 4;
    ctx.gpr[4] = c;
}

// void f(T* o, const f32 v[2]): o->vfn[28](o, {v[0] + +64, v[1] + +68,
// +72})
DECOMP(00795DAC, Obj_MoveBy_00795DAC) {
    const u32 o = arg<u32>(ctx, 0), v = arg<u32>(ctx, 1);
    Stack frame(ctx, 16);
    const u32 l = frame.addr();
    const f32 y = static_cast<f32>(static_cast<f64>(load<f32>(v + 4)) + static_cast<f64>(load<f32>(o + 68)));
    const f32 x = static_cast<f32>(static_cast<f64>(load<f32>(v)) + static_cast<f64>(load<f32>(o + 64)));
    const u32 vt = load<u32>(o);
    Cp(l + 8, o + 72);
    store<f32>(l + 4, y);
    store<f32>(l, x);
    // the sums are still in f1 / f2 (and +72 in f3) at the call
    ctx.fpr[1] = static_cast<f64>(x);
    ctx.fpr[2] = static_cast<f64>(y);
    ctx.fpr[3] = LfsAt(o + 72);
    VCallVt(ctx, vt, 28, {o, l});
}

// ---------------------------------------------------------------- sweep: 33 instructions
// Track* f(T* o, u32 layer): Anim_Restart2775A8(o->+64, layer, 0, 0, 0, -1,
// 0, 0, -, -, 0, -1; the f32 constants at 0x00013F68 / +4 / +8 / +8); +32
// = layer when it starts one
DECOMP(00013F78, Obj_RestartAnim_00013F78) {
    constexpr u32 kK = 0x00013F68;
    const u32 o = arg<u32>(ctx, 0), layer = arg<u32>(ctx, 1);
    const f64 c = LfsAt(kK + 8);
    // arguments 9 / 10 (r1 +112 / +120) are not written; the callee does not read them
    callf(ctx, 0x002775A8, {load<u32>(o + 64), layer, 0, 0, 0, ~0ull, 0, 0, 0, 0, 0, ~0ull},
          {LfsAt(kK), LfsAt(kK + 4), c, c});
    if (static_cast<u32>(ctx.gpr[3]) != 0) store<u32>(o + 32, layer);
}

// T::~T deleting form: vtable 0x00B0E5B8, 0x0002EC04(o), +64->+36's +52 /
// +56 = 0, vtable 0x00B0DC70, NavQuery_Free62D08C(o + 216), Mem_FreeFrom(o,
// 0). Null does nothing (r3 = 0).
DECOMP(0002ED84, VtB0E5B8_Delete) {
    const u32 o = arg<u32>(ctx, 0);
    if (o == 0) {
        ctx.gpr[3] = 0;
        return;
    }
    store<u32>(o, 0x00B0E5B8);
    call(ctx, 0x0002EC04, {o});
    const u32 p = load<u32>(load<u32>(o + 64) + 36);
    store<u32>(p + 52, 0);
    store<u32>(p + 56, 0);
    store<u32>(o, 0x00B0DC70);
    call(ctx, 0x0062D08C, {o + 216});
    call(ctx, o25fn::Mem_FreeFrom, {o, 0});
}

// void f(T* o): the u8 +318 = 1 and +316 = 0; then by the kind +4: 1 / 2
// clear bits 0x14 of the u8 +304, 3 clears the u8 +317, 4 calls
// 0x002E2E78(the global 0x00F64C20)
DECOMP(0003DE48, Obj_ResetByKind_0003DE48) {
    const u32 o = arg<u32>(ctx, 0);
    const s32 k = load<s32>(o + 4);
    store<u8>(o + 318, 1);
    store<u8>(o + 316, 0);
    ctx.gpr[4] = 1;
    ctx.gpr[5] = 0;
    ctx.gpr[6] = static_cast<u32>(k);
    if (k > 2) {
        if (k == 3) {
            store<u8>(o + 317, 0);
            ctx.gpr[4] = 0;
        } else if (k == 4) {
            call(ctx, 0x002E2E78, {load<u32>(0x00F64C20), 1, 0});
        }
        return;
    }
    if (k <= 0) return;
    const u32 b = load<u8>(o + 304) & ~0x14u;
    store<u8>(o + 304, static_cast<u8>(b));
    ctx.gpr[4] = 0;
    ctx.gpr[5] = b;
}

// The static initializer shape: 3 zero words at z, the f32 fdst = the
// constant at fsrc, then the tuning variable var (name, vtable 0x00B0D99C,
// +72 = v72)
static void InitTuningVar(PPUContext& ctx, u32 z, u32 fdst, u32 fsrc, u32 var, u32 name, u32 v72) {
    store<u32>(z, 0);
    store<u32>(z + 4, 0);
    const u32 f = load<u32>(fsrc);
    store<u32>(z + 8, 0);
    store<u32>(fdst, f);
    ctx.fpr[1] = static_cast<f64>(std::bit_cast<f32>(f));
    call(ctx, o24fn::Ctor_Base4FBFA8, {var, name});
    store<u32>(var + 72, v72);
    store<u32>(var, 0x00B0D99C);
    ctx.gpr[3] = 0x00B0D99C;
    ctx.gpr[4] = v72;
}
// static init: the 3 words at 0x00F9CD88 = 0, the f32 0x00F9CD30 = the
// constant at 0x00AC47E0, and the tuning variable at 0x00F9CD38 (name at
// 0x00AC5874, +72 = 250.0f)
DECOMP(0009A108, Init_Tuning_0009A108) {
    InitTuningVar(ctx, 0x00F9CD88, 0x00F9CD30, 0x00AC47E0, 0x00F9CD38, 0x00AC5874, 0x437A0000);
}

// void f(T* o): pushes a new 0x0042D72C (112 bytes) made from (+12, o + 16,
// the u8 +112)
DECOMP(0009B708, Cmd_Push42D72C_0009B708) {
    const u32 o = arg<u32>(ctx, 0);
    PushNew26(ctx, 112, [&](u32 p) { call(ctx, 0x0042D72C, {p, load<u32>(o + 12), o + 16, load<u8>(o + 112)}); });
}
// void f(T* o): pushes a new VtB20E60 (24 bytes) made from (+12, +16, the
// u8 +20, the u8 +21)
DECOMP(0009CD1C, Cmd_PushB20E60_0009CD1C) {
    const u32 o = arg<u32>(ctx, 0);
    PushNew26(ctx, 24, [&](u32 p) {
        call(ctx, 0x0042EA6C, {p, load<u32>(o + 12), load<u32>(o + 16), load<u8>(o + 20), load<u8>(o + 21)});
    });
}
// void f(T* o): when +12 is set, pushes a new VtB20F50 (20 bytes) made from
// (+12, the u8 +16)
DECOMP(0009EB1C, Cmd_PushB20F50_0009EB1C) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 a = load<u32>(o + 12);
    ctx.gpr[3] = a;
    if (a == 0) return;
    PushNew26(ctx, 20, [&](u32 p) { call(ctx, 0x0042FDC8, {p, load<u32>(o + 12), load<u8>(o + 16)}); });
}
// void f(T* o): pushes a new 0x00434E0C (112 bytes) made from (+12; f32
// +16, +20, +24)
DECOMP(000A300C, Cmd_Push434E0C_000A300C) {
    const u32 o = arg<u32>(ctx, 0);
    PushNew26(ctx, 112, [&](u32 p) {
        callf(ctx, 0x00434E0C, {p, load<u32>(o + 12)}, {LfsAt(o + 16), LfsAt(o + 20), LfsAt(o + 24)});
    });
}
// void f(T* o): when +12 is set, pushes a new VtB212F8 (20 bytes) made from
// (+12, the u8 +16)
DECOMP(000A5480, Cmd_PushB212F8_000A5480) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 a = load<u32>(o + 12);
    ctx.gpr[3] = a;
    if (a == 0) return;
    PushNew26(ctx, 20, [&](u32 p) { call(ctx, 0x00438F3C, {p, load<u32>(o + 12), load<u8>(o + 16)}); });
}
// void f(T* o): when the u16 +20 is set, pushes a new 0x0043ABA0 (32
// bytes) made from (o + 12)
DECOMP(000A7404, Cmd_Push43ABA0_000A7404) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 n = load<u16>(o + 20);
    ctx.gpr[3] = n;
    if (n == 0) return;
    PushNew26(ctx, 32, [&](u32 p) { call(ctx, 0x0043ABA0, {p, o + 12}); });
}

// void f(): Obj_Release_0044DBC4(the global 0x00F5590C), the words
// 0x00F558E8 / EC / F0 = 0, and the global 0x00F55908 released (vfn[8])
// and cleared when set
DECOMP(000AC03C, Globals_Release_000AC03C) {
    call(ctx, 0x0044DBC4, {load<u32>(0x00F5590C)});
    store<u32>(0x00F558E8, 0);
    store<u32>(0x00F558EC, 0);
    store<u32>(0x00F558F0, 0);
    const u32 p = load<u32>(0x00F55908);
    ctx.gpr[3] = p;
    if (p == 0) return;
    VCall(ctx, p, 8, {p});
    store<u32>(0x00F55908, 0);
    ctx.gpr[3] = 0x00F50000;
    ctx.gpr[4] = 0;
}

// f32 f(T* o, key): the track key's time / length (Anim_Find34CC34 on the
// animator +164), else the constant at 0x000DC69C
DECOMP(000DC6A0, Obj_AnimPhase_000DC6A0) {
    const u32 o = arg<u32>(ctx, 0), key = arg<u32>(ctx, 1);
    const u32 a = load<u32>(o + 164);
    if (a != 0) {
        const u32 t = static_cast<u32>(call(ctx, 0x0034CC34, {a, key}));
        if (t != 0) {
            call(ctx, 0x00353780, {t});
            const f64 time = ctx.fpr[1];
            call(ctx, 0x00353748, {t});
            ctx.fpr[1] = static_cast<f64>(static_cast<f32>(time / ctx.fpr[1]));
            return;
        }
    }
    ctx.gpr[3] = 0x000DC69C;
    ctx.fpr[1] = LfsAt(0x000DC69C);
}

// u32 f(T* o, a): with r = o->vfn[64](o): when r's 3-bit field +416 >> 28
// is 1 or 3, returns 0x0012B908(o, a); else the field (0 without r)
DECOMP(0012BB9C, Obj_DispatchKind_0012BB9C) {
    const u32 o = arg<u32>(ctx, 0), a = arg<u32>(ctx, 1);
    const u32 r = static_cast<u32>(VCall(ctx, o, 64, {o}));
    ctx.gpr[3] = r;
    if (r == 0) return;
    const u32 k = (load<u32>(r + 416) >> 28) & 7u;
    ctx.gpr[3] = k;
    if (k == 1 || k == 3) ctx.gpr[3] = call(ctx, 0x0012B908, {o, a});
}

// u32 f(Src* s, T* o): with p = CallK2_000D94F4(o): p->+1224 / +1228 =
// the f32 s->+4, 0x0025DD60(p), p->+1232 = the f1 it returns, o->+728 =
// s->+8, then returns 0x002971BC(p, s->+8) (0 without p)
DECOMP(0016D068, Obj_ApplySrc_0016D068) {
    const u32 s = arg<u32>(ctx, 0), o = arg<u32>(ctx, 1);
    const u32 p = static_cast<u32>(call(ctx, 0x000D94F4, {o, o}));
    if (p == 0) return;
    const u32 f = load<u32>(s + 4);
    store<u32>(p + 1224, f);
    store<u32>(p + 1228, f);
    ctx.fpr[1] = static_cast<f64>(std::bit_cast<f32>(f));
    call(ctx, 0x0025DD60, {p});
    const u32 v = load<u32>(s + 8);
    store<f32>(p + 1232, static_cast<f32>(ctx.fpr[1]));
    store<u32>(o + 728, v);
    ctx.gpr[3] = call(ctx, 0x002971BC, {p, v});
}

// T::~T: vtables 0x00B14968 / 0x00B14B00 (+380) / 0x00B14B28 (+384),
// List1013B48_Remove(o), the base vtables 0x00B12528 (+384) / 0x00B1200C
// (+380) / 0x00B126B8, then VtB12550_Dtor
DECOMP(00178060, VtB14968_Dtor) {
    const u32 o = arg<u32>(ctx, 0);
    store<u32>(o, 0x00B14968);
    store<u32>(o + 380, 0x00B14B00);
    store<u32>(o + 384, 0x00B14B28);
    call(ctx, 0x000D68CC, {o});
    store<u32>(o + 384, 0x00B12528);
    store<u32>(o + 380, 0x00B1200C);
    store<u32>(o, 0x00B126B8);
    call(ctx, o23fn::VtB12550_Dtor, {o});
}

// bool f(T* o, f32 t, U* p): 1 unless +464 is 0, t is above the constant
// at 0x001AC154, +172 lacks bit 0x4, p is set and p's u64 +184 has bit 0;
// then +452 += 1 and whether the f32 +348 is above the constant
DECOMP(001AC158, Obj_CanTick_001AC158) {
    const u32 o = arg<u32>(ctx, 0), p = arg<u32>(ctx, 2);
    ctx.gpr[3] = 1;
    const u32 a = load<u32>(o + 464);
    ctx.gpr[4] = a;
    if (a != 0) return;
    const f64 t = farg(ctx, 0);
    const f64 k = LfsAt(0x001AC154);
    ctx.fpr[1] = k;
    ctx.fpr[2] = t;
    ctx.gpr[4] = 0x001AC154;
    if (t <= k) return;
    const u32 b = (load<u32>(o + 172) >> 2) & 1u;
    ctx.gpr[4] = b;
    if (b != 0) return;
    if (p == 0) return;
    const u32 c = static_cast<u32>(load<u64>(p + 184) & 1u);
    ctx.gpr[4] = c;
    if (c == 0) return;
    const f64 x = LfsAt(o + 348);
    const u32 n = load<u32>(o + 452) + 1;
    store<u32>(o + 452, n);
    ctx.gpr[4] = o;
    ctx.gpr[5] = n;
    ctx.fpr[2] = x;
    ctx.gpr[3] = x > k ? 1u : 0u;
}

// The VtB17270 destructor body for an adjusted this (store order: +0,
// +820, +380, +384)
static void VtB17270DtorAt(PPUContext& ctx, u32 o) {
    store<u32>(o, 0x00B17270);
    store<u16>(o + 820, 0);
    store<u32>(o + 380, 0x00B17444);
    store<u32>(o + 384, 0x00B1746C);
    VecDtor(ctx, o + 812);
    call(ctx, 0x001AE468, {o});
}
// T::~T thunk for the base at +384
DECOMP(001CC06C, VtB17270_DtorThunk384) { VtB17270DtorAt(ctx, arg<u32>(ctx, 0) - 384); }
// T::~T thunk for the base at +380
DECOMP(001CC0F0, VtB17270_DtorThunk380) { VtB17270DtorAt(ctx, arg<u32>(ctx, 0) - 380); }

// u32 f(T* o): +484 = 0; when q = +216 has no live node at +184
// (Ptr_DerefNonZero), RefNode_Release(it) and +184 = Node_PushF65B7C(q->+180,
// q->+176); then returns 0x000D82BC(o)
DECOMP(00210CA4, Obj_RenewNode_00210CA4) {
    const u32 o = arg<u32>(ctx, 0);
    store<u32>(o + 484, 0);
    const u32 q = load<u32>(o + 216);
    if (static_cast<s32>(call(ctx, 0x00446960, {load<u32>(q + 184)})) == 0) {
        call(ctx, o24fn::RefNode_Release, {load<u32>(q + 184)});
        const u32 n = static_cast<u32>(call(ctx, 0x00446984, {load<u32>(q + 180), load<u32>(q + 176)}));
        store<u32>(q + 184, n);
    }
    ctx.gpr[3] = call(ctx, 0x000D82BC, {o});
}

// f32 f(T* o, key): the track key's time / length (Anim_Find34CC34 on the
// animator +36), else the constant at 0x00298944
DECOMP(00298948, Obj_AnimPhase_00298948) {
    const u32 o = arg<u32>(ctx, 0), key = arg<u32>(ctx, 1);
    const u32 a = load<u32>(o + 36);
    if (a != 0) {
        const u32 t = static_cast<u32>(call(ctx, 0x0034CC34, {a, key}));
        if (t != 0) {
            call(ctx, 0x00353780, {t});
            const f64 time = ctx.fpr[1];
            call(ctx, 0x00353748, {t});
            ctx.fpr[1] = static_cast<f64>(static_cast<f32>(time / ctx.fpr[1]));
            return;
        }
    }
    ctx.gpr[3] = 0x00298944;
    ctx.fpr[1] = LfsAt(0x00298944);
}

// T::~T deleting form: vtable 0x00B1D020, the entry for
// String_HashNoCase(o + 4) removed from the map at 0x0105ABB8, then
// Mem_FreeFrom(o, 0). Null does nothing (r3 = 0).
DECOMP(002C0E14, VtB1D020_Delete) {
    const u32 o = arg<u32>(ctx, 0);
    if (o == 0) {
        ctx.gpr[3] = 0;
        return;
    }
    store<u32>(o, 0x00B1D020);
    const u32 h = static_cast<u32>(call(ctx, 0x0056A710, {o + 4}));
    Stack frame(ctx, 16);
    store<u32>(frame.addr(), h);
    call(ctx, 0x0075E06C, {0x0105ABB8, frame.addr()});
    call(ctx, o25fn::Mem_FreeFrom, {o, 0});
}

// u32 f(T* o): the number of bits set in the u16 at world + 4056 * (the u8
// world->+4208) + 204 + 2 * (the u8 o->+825); 0 when that is 255
DECOMP(002DA9D0, Obj_CountBits_002DA9D0) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 w = load<u32>(kWorldPtr);
    const u32 b = load<u8>(o + 825);
    const u32 base = w + load<u8>(w + 4208) * 4056u;
    ctx.gpr[6] = w + 4208;
    ctx.gpr[5] = w;
    ctx.gpr[4] = b;
    ctx.gpr[3] = 0;
    if (b == 255) return;
    u32 v = load<u16>(base + ((b << 1) & 0x1FEu) + 204);
    u32 n = 0;
    while (v != 0) {
        ctx.gpr[5] = v - 1;
        v &= v - 1;
        ++n;
    }
    ctx.gpr[4] = 0;
    ctx.gpr[3] = n;
}

// T::~T deleting form (20 bytes): when +12 is set, vtable 0x00B1D578 and
// (+12)->vfn[8](+12); then vtable 0x00B1D4D8. Null does nothing (r3 = 0).
DECOMP(002F45E8, VtB1D578_Delete) {
    const u32 o = arg<u32>(ctx, 0);
    if (o == 0) {
        ctx.gpr[3] = 0;
        return;
    }
    const u32 p = load<u32>(o + 12);
    if (p != 0) {
        const u32 vt = load<u32>(p);  // read before the store
        store<u32>(o, 0x00B1D578);
        VCallVt(ctx, vt, 8, {p});
    }
    store<u32>(o, 0x00B1D4D8);
    call(ctx, 0x00552420, {o, 20});
}
// T::~T deleting form (132 bytes): vtable 0x00B1D5A0, (+12)->vfn[8](+12),
// vtable 0x00B1D4D8. Null does nothing (r3 = 0).
DECOMP(002F490C, VtB1D5A0_Delete) {
    const u32 o = arg<u32>(ctx, 0);
    if (o == 0) {
        ctx.gpr[3] = 0;
        return;
    }
    const u32 p = load<u32>(o + 12);
    store<u32>(o, 0x00B1D5A0);
    VCall(ctx, p, 8, {p});
    store<u32>(o, 0x00B1D4D8);
    call(ctx, 0x00552420, {o, 132});
}

// static init: the 3 words at 0x0106B2B0 = 0, the f32 0x0106B260 = the
// constant at 0x00ADB5F0, and the tuning variable at 0x0106B264 (name at
// 0x00ADB7F4, +72 = 1.0f)
DECOMP(00328644, Init_Tuning_00328644) {
    InitTuningVar(ctx, 0x0106B2B0, 0x0106B260, 0x00ADB5F0, 0x0106B264, 0x00ADB7F4, 0x3F800000);
}

// u32 f(T* o, u8 b): +2604 = 0; the object at +2600 released
// (vfn[40](it, 0)) and cleared; returns Recs_Reset325A30(o, b)
DECOMP(00332C00, Obj_ResetRecs_00332C00) {
    const u32 o = arg<u32>(ctx, 0), b = arg<u32>(ctx, 1) & 0xFFu;
    const u32 p = load<u32>(o + 2600);
    store<u32>(o + 2604, 0);
    if (p != 0) {
        VCall(ctx, p, 40, {p, 0});
        store<u32>(o + 2600, 0);
    }
    ctx.gpr[3] = call(ctx, 0x00325A30, {o, b});
}

// void f(T* o): o->vfn[20](o, 0), Obj_Place3626E8(o, 0x01164B40, 1), +328
// = 1.0f, +316 = +308 = 0 and +160 = (+160 & 1) | 1
DECOMP(0036E184, Obj_Reset_0036E184) {
    const u32 o = arg<u32>(ctx, 0);
    VCall(ctx, o, 20, {o, 0});
    call(ctx, 0x003626E8, {o, 0x01164B40, 1});
    const u32 v = (load<u32>(o + 160) & 1u) | 1u;
    store<u32>(o + 328, 0x3F800000);
    store<u32>(o + 316, 0);
    store<u32>(o + 308, 0);
    store<u32>(o + 160, v);
    ctx.gpr[3] = v;
    ctx.gpr[4] = 0x3F800000;
    ctx.gpr[5] = 0;
}

// u32 f(T* o): o->vfn[20](o, 0), Obj_Place3626E8(o, 0x01164B40, 1), +160
// &= 1 and the top bit of +184 = bit 0 of the u8 +8->+145 (returned)
DECOMP(0036EF3C, Obj_Reset_0036EF3C) {
    const u32 o = arg<u32>(ctx, 0);
    VCall(ctx, o, 20, {o, 0});
    call(ctx, 0x003626E8, {o, 0x01164B40, 1});
    const u32 q = load<u32>(o + 8);
    const u32 a = load<u32>(o + 184) & 0x7FFFFFFFu;
    const u32 c = load<u32>(o + 160) & 1u;
    const u32 b = load<u8>(q + 145);
    const u32 v = a | ((b & 1u) << 31);
    store<u32>(o + 160, c);
    store<u32>(o + 184, v);
    ctx.gpr[3] = b;
    ctx.gpr[4] = v;
    ctx.gpr[5] = c;
}

// void f(_, T* o): 0x0040D428(o + 192), then the 3 f32 at +240 are copied
// to +304 of each of the u16 +172 entries of the array +164
DECOMP(00413088, Obj_SpreadVec_00413088) {
    const u32 o = arg<u32>(ctx, 1);
    call(ctx, 0x0040D428, {o + 192});
    const u32 x = load<u32>(o + 240), y = load<u32>(o + 244), z = load<u32>(o + 248);
    ctx.fpr[3] = static_cast<f64>(std::bit_cast<f32>(x));
    ctx.fpr[2] = static_cast<f64>(std::bit_cast<f32>(y));
    ctx.fpr[1] = static_cast<f64>(std::bit_cast<f32>(z));
    u32 i = 0;
    for (;;) {
        const u32 n = load<u16>(o + 172);
        ctx.gpr[5] = n;
        if (!(i < n)) break;
        const u32 e = load<u32>(load<u32>(o + 164) + 4 * i);
        ++i;
        store<u32>(e + 304, x);
        store<u32>(e + 308, y);
        store<u32>(e + 312, z);
    }
    ctx.gpr[3] = 4 * i;
    ctx.gpr[4] = i;
}

// void f(T* o): Player_StartAction(world, world->+4208->+8, 0x00AE2568,
// 0x00AE24FC, 0x00F38EF0, o, 0x00F38EF8, o, 1, 1), then
// Flag_Set8_40000000(o)
DECOMP(00434D08, Obj_StartAction_00434D08) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 w = load<u32>(kWorldPtr);
    call(ctx, 0x00489A60, {w, load<u32>(w + 4208 + 8), 0x00AE2568, 0x00AE24FC, 0x00F38EF0, o, 0x00F38EF8, o, 1, 1});
    ctx.gpr[3] = call(ctx, o24fn::Flag_Set8_40000000, {o});
}

// T::T(o, Ref* a, u8 b, u8 c): the 0x00B20CC8 base (Obj_InitVt_B20CC8(o,
// 0)), +12 = a, +16 = b, +17 = c, vtable 0x00B212C8; Field_Inc20_00446170(a)
// when a is set
DECOMP(00438B84, VtB212C8_Ctor) {
    const u32 o = arg<u32>(ctx, 0), a = arg<u32>(ctx, 1), b = arg<u32>(ctx, 2), c = arg<u32>(ctx, 3);
    call(ctx, o24fn::Obj_InitVt_B20CC8, {o, 0});
    store<u32>(o + 12, a);
    store<u8>(o + 16, static_cast<u8>(b));
    store<u8>(o + 17, static_cast<u8>(c));
    store<u32>(o, 0x00B212C8);
    ctx.gpr[3] = 0x00B212C8;
    if (a != 0) ctx.gpr[3] = call(ctx, 0x00446170, {a});
}

// T::T(o, a, b, bool c): the 0x00B20CC8 base (Obj_InitVt_B20CC8(o, 0)),
// +16 = a, vtable 0x00B21340, +12 = 0, +20 = b, the top bit of +24 = c
DECOMP(00439C28, VtB21340_Ctor) {
    const u32 o = arg<u32>(ctx, 0), a = arg<u32>(ctx, 1), b = arg<u32>(ctx, 2), c = arg<u32>(ctx, 3);
    call(ctx, o24fn::Obj_InitVt_B20CC8, {o, 0});
    const u32 f = (load<u32>(o + 24) & 0x7FFFFFFFu) | ((c & 1u) << 31);
    store<u32>(o + 16, a);
    store<u32>(o, 0x00B21340);
    store<u32>(o + 12, 0);
    store<u32>(o + 20, b);
    store<u32>(o + 24, f);
    ctx.gpr[3] = 0x00B21340;
    ctx.gpr[4] = f;
    ctx.gpr[5] = 0;
}

// The VtB21988 destructor body: SmallFree(+80, +84) when set, +84 = +80 =
// 0; Mem_FreeFrom(+20, 0) and +20 = 0 when set; then 0x0048F6B0(o)
static void VtB21988Dtor(PPUContext& ctx) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 p = load<u32>(o + 80);
    store<u32>(o, 0x00B21988);
    if (p != 0) call(ctx, 0x00552420, {p, load<u32>(o + 84)});
    const u32 q = load<u32>(o + 20);
    store<u32>(o + 84, 0);
    store<u32>(o + 80, 0);
    if (q != 0) {
        call(ctx, o25fn::Mem_FreeFrom, {q, 0});
        store<u32>(o + 20, 0);
    }
    ctx.gpr[3] = call(ctx, 0x0048F6B0, {o});
}
// T::~T (vtable 0x00B21988)
DECOMP(00455974, VtB21988_Dtor) { VtB21988Dtor(ctx); }
// T::~T (vtable 0x00B21988), a second copy
DECOMP(00455A94, VtB21988_Dtor2) { VtB21988Dtor(ctx); }

// static init: the 3 words at 0x010B1718 = 0, the f32 0x010B16C0 = the
// constant at 0x00AE50CC, and the tuning variable at 0x010B16C8 (name at
// 0x00AE5344, +72 = 20.0f)
DECOMP(00456710, Init_Tuning_00456710) {
    InitTuningVar(ctx, 0x010B1718, 0x010B16C0, 0x00AE50CC, 0x010B16C8, 0x00AE5344, 0x41A00000);
}

// T::~T deleting form: vtable 0x00B219C0, (+28)->vfn[32](+28, 0) and +28 =
// 0 when set, then Mem_FreeFrom(o, 0). Null does nothing (r3 = 0).
DECOMP(004643F8, VtB219C0_Delete) {
    const u32 o = arg<u32>(ctx, 0);
    if (o == 0) {
        ctx.gpr[3] = 0;
        return;
    }
    const u32 p = load<u32>(o + 28);
    store<u32>(o, 0x00B219C0);
    if (p != 0) {
        VCall(ctx, p, 32, {p, 0});
        store<u32>(o + 28, 0);
    }
    call(ctx, o25fn::Mem_FreeFrom, {o, 0});
}

// T::~T deleting form (40 bytes): when +28 is set, vtable 0x00B21DC8 and
// (+28)->vfn[8](+28); then vtable 0x00B11B20. Null does nothing (r3 = 0).
DECOMP(004BEFAC, VtB21DC8_Delete) {
    const u32 o = arg<u32>(ctx, 0);
    if (o == 0) {
        ctx.gpr[3] = 0;
        return;
    }
    const u32 p = load<u32>(o + 28);
    if (p != 0) {
        const u32 vt = load<u32>(p);  // read before the store
        store<u32>(o, 0x00B21DC8);
        VCallVt(ctx, vt, 8, {p});
    }
    store<u32>(o, 0x00B11B20);
    call(ctx, 0x00552420, {o, 40});
}

// s32 f(_, u32 id): 0x6C5D01A7 -> 3, 0x6C5B2FC2 -> 4, 0x152D006A -> 2,
// 0x0F49FCD2 -> 1, else 0 (r4 = the last id compared)
DECOMP(004E2B60, Id_Index_004E2B60) {
    const u32 id = arg<u32>(ctx, 1);
    static constexpr u32 kIds[5] = {0x6C5D01A7u, 0x6C5B2FC2u, 0x152D006Au, 0x0F4BCEB7u, 0x0F49FCD2u};
    static constexpr u32 kVals[5] = {3, 4, 2, 0, 1};
    for (u32 i = 0; i < 5; ++i) {
        ctx.gpr[4] = kIds[i];
        if (id == kIds[i]) {
            ctx.gpr[3] = kVals[i];
            return;
        }
    }
    ctx.gpr[3] = 0;
}

// void f(T* o, const Mtx* m): the 3x3 part of the 16-byte rows of m to the
// rows at +16, then WorldObj_MarkDirty(o)
DECOMP(00735C58, WorldObj_SetRot_00735C58) {
    const u32 o = arg<u32>(ctx, 0), m = arg<u32>(ctx, 1);
    u32 w = 0;
    for (u32 r = 0; r < 4; ++r)
        for (u32 c = 0; c < 3; ++c) w = Cp(o + 16 + 16 * r + 4 * c, m + 16 * r + 4 * c);
    ctx.fpr[1] = static_cast<f64>(std::bit_cast<f32>(w));
    ctx.gpr[3] = call(ctx, 0x0057D000, {o});
}

// The T::~T deleting form body (vtables 0x00B1F4C0 (+12) / 0x00B1F4E0 (+8)
// / 0x00B13974, then the r13 pool (-28332)->+44->vfn[12](pool, o, the u16
// +4)) for o; null does nothing (r3 = 0).
static void VtB1F4C0DeleteAt(PPUContext& ctx, u32 o) {
    if (o == 0) {
        ctx.gpr[3] = 0;
        return;
    }
    store<u32>(o + 12, 0x00B1F4C0);
    store<u32>(o + 8, 0x00B1F4E0);
    store<u32>(o, 0x00B13974);
    const u32 pool = Pool28332(ctx);
    VCall(ctx, pool, 12, {pool, o, load<u16>(o + 4)});
}
// T::~T deleting form (vtable 0x00B1F4C0)
DECOMP(00771958, Mi3VtB1F4C0_Delete) { VtB1F4C0DeleteAt(ctx, arg<u32>(ctx, 0)); }

// ---------------------------------------------------------------- sweep: 34 instructions
// T::~T deleting form: when q = +48->+220 and q->+80 are set, vtable
// 0x00B0E428 and 0x0002B618(q->+80, q); then vtable 0x00B0DC70,
// NavQuery_Free62D08C(o + 216), Mem_FreeFrom(o, 0). Null does nothing.
DECOMP(0002BAA4, VtB0E428_Delete) {
    const u32 o = arg<u32>(ctx, 0);
    if (o == 0) {
        ctx.gpr[3] = 0;
        return;
    }
    const u32 q = load<u32>(load<u32>(o + 48) + 220);
    if (q != 0) {
        const u32 r = load<u32>(q + 80);
        if (r != 0) {
            store<u32>(o, 0x00B0E428);
            call(ctx, 0x0002B618, {r, q});
        }
    }
    store<u32>(o, 0x00B0DC70);
    call(ctx, 0x0062D08C, {o + 216});
    call(ctx, o25fn::Mem_FreeFrom, {o, 0});
}

// u32 f(T* o, Event* e): for the event 0x03DE9A54, 0x0002B618(q->+80, q)
// when q = +48->+220 and q->+80 are set; then returns 0x000133FC(o, e)
DECOMP(0002BDB8, Obj_OnEvent_0002BDB8) {
    const u32 o = arg<u32>(ctx, 0), e = arg<u32>(ctx, 1);
    const s32 h = load<s32>(e + 8);
    if (h > static_cast<s32>(0xDD92E72Bu) && h == 0x03DE9A54) {
        const u32 q = load<u32>(load<u32>(o + 48) + 220);
        if (q != 0) {
            const u32 r = load<u32>(q + 80);
            if (r != 0) call(ctx, 0x0002B618, {r, q});
        }
    }
    ctx.gpr[3] = call(ctx, 0x000133FC, {o, e});
}

// u32 f(T* o, f32 t, Ref* x): when t is above the constant at 0x0002BE40
// and q = +48 is set with bit 0x2 in +192: q->vfn[352](q, -, x->+4, 0)
// (r4 holds the OPD there, as the original leaves it)
DECOMP(0002BE48, Obj_Notify352_0002BE48) {
    const u32 o = arg<u32>(ctx, 0), x = arg<u32>(ctx, 2);
    ctx.gpr[4] = x;
    ctx.fpr[2] = LfsAt(0x0002BE40);
    if (!(farg(ctx, 0) > ctx.fpr[2])) return;
    const u32 q = load<u32>(o + 48);
    ctx.gpr[3] = q;
    if (q == 0) return;
    const u32 f = load<u32>(q + 192) & 0x2u;
    ctx.gpr[5] = f;
    if (f == 0) return;
    const u32 vt = load<u32>(q);
    const u32 a = load<u32>(x + 4);
    const u32 opd = load<u32>(vt + 352);
    call_ptr(ctx, opd, {q, opd, a, 0, load<u32>(opd)});
}

// u32 f(T* o): when bit 0 of the u8 +304 is set and the kind +4 is not
// 11: Obj_SetEffectHash_000DAEC8(+48, 0x39F5F435), 0x0002EC04(o), then
// o->vfn[220](o, 18)
DECOMP(000329A8, Obj_Stop_000329A8) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 f = load<u8>(o + 304) & 1u;
    ctx.gpr[3] = f;
    if (f == 0) return;
    const u32 k = load<u32>(o + 4);
    ctx.gpr[3] = k;
    if (k == 11) return;
    call(ctx, 0x000DAEC8, {load<u32>(o + 48), 0x39F5F435});
    call(ctx, 0x0002EC04, {o});
    ctx.gpr[3] = VCall(ctx, o, 220, {o, 18});
}

// u32 f(T* o): when h = Handle_ResolveBit1(+12) is set and +16 is
// 0x002237D4 or 0x048129EB: Field_SetByKind_0036C124(h, 3, +20, the u8
// +24)
DECOMP(0007D2B8, Obj_SetField_0007D2B8) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 h = static_cast<u32>(call(ctx, 0x003568E0, {load<u32>(o + 12)}));
    if (h == 0) return;
    const s32 k = load<s32>(o + 16);
    ctx.gpr[4] = static_cast<u32>(k);
    if (k > 0x2237D4) {
        ctx.gpr[5] = 0x048129EB;
        if (k != 0x048129EB) return;
    } else {
        ctx.gpr[5] = 0x2237D3;
        if (!(k > 0x2237D3)) return;
    }
    ctx.gpr[3] = call(ctx, 0x0036C124, {h, 3, load<u32>(o + 20), load<u8>(o + 24)});
}

// u32 f(T* o, a): with e = Effect_Bind4BB7E0(a, +12, 0): when e->+276 is
// 0x02235C33 and e->+280 = p is set, the f32 +84 of Obj_FindComponent(p,
// 34, 0) = +16 (when found)
DECOMP(0008A058, Obj_SetCompF84_0008A058) {
    const u32 o = arg<u32>(ctx, 0), a = arg<u32>(ctx, 1);
    const u32 e = static_cast<u32>(call(ctx, 0x004BB7E0, {a, load<u32>(o + 12), 0}));
    if (e == 0) return;
    const u32 k = load<u32>(e + 276);
    ctx.gpr[4] = 0x02235C33;
    ctx.gpr[5] = k;
    if (k != 0x02235C33u) return;
    const u32 p = load<u32>(e + 280);
    ctx.gpr[3] = p;
    if (p == 0) return;
    const u32 c = static_cast<u32>(call(ctx, 0x000D1288, {p, 34, 0}));
    if (c == 0) return;
    const u32 v = load<u32>(o + 16);
    store<u32>(c + 84, v);
    ctx.fpr[1] = static_cast<f64>(std::bit_cast<f32>(v));
}

// void f(T* o): pushes a new 0x0042E040 (36 bytes) made from (the u8 +12;
// f32 +16, +20, +24, +28)
DECOMP(0009C1CC, Cmd_Push42E040_0009C1CC) {
    const u32 o = arg<u32>(ctx, 0);
    PushNew26(ctx, 36, [&](u32 p) {
        callf(ctx, 0x0042E040, {p, load<u8>(o + 12)}, {LfsAt(o + 16), LfsAt(o + 20), LfsAt(o + 24), LfsAt(o + 28)});
    });
}
// void f(T* o): pushes a new Obj_InitVt_B21088 (16 bytes) made from (the
// top bit of +12)
DECOMP(000A10E4, Cmd_PushB21088_000A10E4) {
    const u32 o = arg<u32>(ctx, 0);
    PushNew26(ctx, 16, [&](u32 p) { call(ctx, 0x00433424, {p, load<u32>(o + 12) >> 31}); });
}
// void f(T* o): pushes a new 0x00434A10 (40 bytes) made from (+12, +16,
// +20, +24; f32 +28)
DECOMP(000A2C88, Cmd_Push434A10_000A2C88) {
    const u32 o = arg<u32>(ctx, 0);
    PushNew26(ctx, 40, [&](u32 p) {
        callf(ctx, 0x00434A10, {p, load<u32>(o + 12), load<u32>(o + 16), load<u32>(o + 20), load<u32>(o + 24)},
              {LfsAt(o + 28)});
    });
}

// void f(T* o, u8 on): unless bit 0x4 of +172 already equals on: sets it
// to bit 0 of on, then o->vfn[240](o) when on is odd, else o->vfn[244](o)
DECOMP(000CFEC4, Obj_SetFlag4_000CFEC4) {
    const u32 o = arg<u32>(ctx, 0), on = arg<u32>(ctx, 1);
    const u32 v = load<u32>(o + 172);
    ctx.gpr[6] = on & 0xFFu;
    ctx.gpr[7] = (v >> 2) & 1u;
    if (((v >> 2) & 1u) == (on & 0xFFu)) return;
    store<u32>(o + 172, (v & ~0x4u) | ((on << 2) & 0x4u));
    ctx.gpr[3] = VCall(ctx, o, (on & 1u) != 0 ? 240 : 244, {o});
}

// T* f(T* o, s32 key): the object +160 when its +204 is key; else, when
// it has children (+272), the first one WorldObj_ForEach(it, the OPD
// 0x00F31D20, {key, result}, 0) finds; 0 otherwise
DECOMP(000D1454, Obj_FindByKey_000D1454) {
    const u32 o = arg<u32>(ctx, 0), key = arg<u32>(ctx, 1);
    const u32 p = load<u32>(o + 160);
    ctx.gpr[3] = 0;
    if (p == 0) return;
    if (load<u32>(p + 204) == key) {
        ctx.gpr[3] = p;
        return;
    }
    if (load<u32>(p + 272) == 0) return;
    Stack frame(ctx, 16);
    store<u32>(frame.addr(), key);
    store<u32>(frame.addr() + 4, 0);
    call(ctx, 0x0057F5D8, {p, 0x00F31D20, frame.addr(), 0});
    ctx.gpr[3] = load<u32>(frame.addr() + 4);
}

// bool f(T* o, u32* out): when world->+0x1281D4->+4 is set, finds the
// first of the u16 (+388)->+8 48-byte entries at (+388)->+0 whose +28 is
// set; *out = its +44 & 31 and returns 1 (else 0)
DECOMP(000E96A0, Obj_FirstEntry_000E96A0) {
    const u32 o = arg<u32>(ctx, 0), out = arg<u32>(ctx, 1);
    const u32 w = load<u32>(kWorldPtr);
    const u32 x = load<u32>(load<u32>(w + 0x1281D4u) + 4);
    ctx.gpr[5] = x;
    ctx.gpr[3] = 0;
    if (x == 0) return;
    const u32 l = load<u32>(o + 388);
    ctx.gpr[5] = 0;
    u32 i = 0, off = 0;
    for (;;) {
        const u32 n = l != 0 ? load<u16>(l + 8) : 0;
        if (!(i < n)) {
            ctx.gpr[6] = off;
            ctx.gpr[7] = i;
            ctx.gpr[8] = n;
            ctx.gpr[3] = 0;
            return;
        }
        const u32 e = load<u32>(load<u32>(l) + off + 28);
        if (e != 0) {
            const u32 v = load<u32>(e + 44) & 31u;
            store<u32>(out, v);
            ctx.gpr[5] = v;
            ctx.gpr[6] = off;
            ctx.gpr[7] = i;
            ctx.gpr[8] = e;
            ctx.gpr[3] = 1;
            return;
        }
        off += 48;
        ++i;
    }
}

// T::~T deleting form (368 bytes): vtable 0x00B13718, Emitter_Stop(+348,
// 1), RefNode_Release(+356), the base destructor 0x000D7024. Null does
// nothing (r3 = 0).
DECOMP(000F5F68, VtB13718_Delete) {
    const u32 o = arg<u32>(ctx, 0);
    if (o == 0) {
        ctx.gpr[3] = 0;
        return;
    }
    store<u32>(o, 0x00B13718);
    call(ctx, o25fn::Emitter_Stop, {load<u32>(o + 348), 1});
    call(ctx, o24fn::RefNode_Release, {load<u32>(o + 356)});
    call(ctx, 0x000D7024, {o});
    call(ctx, 0x00552420, {o, 368});
}

// T::~T: vtable 0x00B13E58, (+12)->vfn[40](+12, 0) when set and +12 = 0,
// 0x009F6A60(o + 16), then vtable 0x00B0DAEC
DECOMP(00111840, VtB13E58_Dtor) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 p = load<u32>(o + 12);
    store<u32>(o, 0x00B13E58);
    if (p != 0) VCall(ctx, p, 40, {p, 0});
    store<u32>(o + 12, 0);
    call(ctx, 0x009F6A60, {o + 16});
    store<u32>(o, 0x00B0DAEC);
    ctx.gpr[3] = 0x00B0DAEC;
}

// T::T(o, a): +4 = a, vtable 0x00B13F18; the vector 0x01164B40 at +32 and
// +64; +52 = -1.0f, +56 = +100 = 5.0f, +84 = 1.0f; the u8 +8 / +80 and
// +12, +16, +48, +60, +96 = 0
DECOMP(00112BC8, VtB13F18_Ctor) {
    constexpr u32 kV = 0x01164B40;
    const u32 o = arg<u32>(ctx, 0), a = arg<u32>(ctx, 1);
    store<u32>(o + 4, a);
    store<u8>(o + 8, 0);
    store<u32>(o, 0x00B13F18);
    store<u32>(o + 12, 0);
    store<u32>(o + 16, 0);
    Cp(o + 32, kV);
    Cp(o + 36, kV + 4);
    u32 z = load<u32>(kV + 8);
    store<u32>(o + 48, 0);
    store<u32>(o + 40, z);
    store<u32>(o + 52, 0xBF800000);  // -1.0f
    store<u32>(o + 56, 0x40A00000);  // 5.0f
    const u32 x = load<u32>(kV);
    store<u32>(o + 60, 0);
    store<u32>(o + 64, x);
    const u32 y = load<u32>(kV + 4);
    store<u8>(o + 80, 0);
    store<u32>(o + 68, y);
    z = load<u32>(kV + 8);
    store<u32>(o + 84, 0x3F800000);  // 1.0f
    store<u32>(o + 72, z);
    store<u32>(o + 96, 0);
    store<u32>(o + 100, 0x40A00000);
    ctx.fpr[1] = static_cast<f64>(std::bit_cast<f32>(z));
    ctx.gpr[3] = o;
    ctx.gpr[4] = kV;
    ctx.gpr[5] = 0x3F800000;
    ctx.gpr[6] = 0x01160000;
    ctx.gpr[7] = 0;
    ctx.gpr[8] = 0x40A00000;
}

// The position getter shape: r = o->vfn[64](o); fn(out, r) when set, else
// out = the vector at 0x01164B40.
static void PosOrZero(PPUContext& ctx, u32 fn) {
    constexpr u32 kV = 0x01164B40;
    const u32 out = arg<u32>(ctx, 0), o = arg<u32>(ctx, 1);
    const u32 r = static_cast<u32>(VCall(ctx, o, 64, {o}));
    if (r != 0) {
        ctx.gpr[3] = call(ctx, fn, {out, r});
        return;
    }
    Cp(out, kV);
    Cp(out + 4, kV + 4);
    const u32 z = Cp(out + 8, kV + 8);
    ctx.fpr[1] = static_cast<f64>(std::bit_cast<f32>(z));
    ctx.gpr[3] = 0x01160000;
    ctx.gpr[4] = kV;
}
// void f(Vec3* out, T* o): Obj_RefPos26A42C(out, o->vfn[64](o)), or zero
DECOMP(00127AB4, Obj_GetRefPos_00127AB4) { PosOrZero(ctx, 0x0026A42C); }
// void f(Vec3* out, T* o): 0x0026D8F4(out, o->vfn[64](o)), or zero
DECOMP(00127B3C, Obj_GetRefPos_00127B3C) { PosOrZero(ctx, 0x0026D8F4); }

// u32 f(T* o, const Vec3* v): with r = o->vfn[64](o): r->+672 = v, bit
// 0x00800000 of +416 set, then Body_SetMode(r, 2) (0 without r)
DECOMP(0012BC20, Obj_SetBodyTarget_0012BC20) {
    const u32 o = arg<u32>(ctx, 0), v = arg<u32>(ctx, 1);
    const u32 r = static_cast<u32>(VCall(ctx, o, 64, {o}));
    ctx.gpr[3] = r;
    if (r == 0) return;
    const u32 f = load<u32>(r + 416) | 0x00800000u;
    Cp(r + 672, v);
    Cp(r + 676, v + 4);
    const u32 z = Cp(r + 680, v + 8);
    store<u32>(r + 416, f);
    ctx.fpr[1] = static_cast<f64>(std::bit_cast<f32>(z));
    ctx.gpr[3] = call(ctx, 0x00261328, {r, 2, r, f});
}

// The effect-owner destructor shape: when +4 is set, vtable vt, its +136
// gets bit 0x200 and loses 0x400, Effect_Delete(it) and +4 = 0; then the
// base vtable 0x00B145E0.
static void EffectOwnerDtor(PPUContext& ctx, u32 vt) {
    const u32 o = arg<u32>(ctx, 0);
    u32 p = load<u32>(o + 4);
    if (p != 0) {
        store<u32>(o, vt);
        store<u32>(p + 136, load<u32>(p + 136) | 0x200u);
        p = load<u32>(o + 4);
        store<u32>(p + 136, load<u32>(p + 136) & ~0x400u);
        call(ctx, 0x004D0280, {load<u32>(o + 4)});
        store<u32>(o + 4, 0);
    }
    store<u32>(o, 0x00B145E0);
    ctx.gpr[3] = 0x00B145E0;
}
// T::~T (vtable 0x00B146D0)
DECOMP(0016D8A0, VtB146D0_Dtor) { EffectOwnerDtor(ctx, 0x00B146D0); }
// T::~T (vtable 0x00B14700)
DECOMP(0016E7C8, VtB14700_Dtor) { EffectOwnerDtor(ctx, 0x00B14700); }

// The VtB14968 destructor body (see VtB14968_Dtor) for an adjusted this
static void VtB14968DtorAt(PPUContext& ctx, u32 o) {
    store<u32>(o, 0x00B14968);
    store<u32>(o + 380, 0x00B14B00);
    store<u32>(o + 384, 0x00B14B28);
    call(ctx, 0x000D68CC, {o});
    store<u32>(o + 384, 0x00B12528);
    store<u32>(o + 380, 0x00B1200C);
    store<u32>(o, 0x00B126B8);
    call(ctx, o23fn::VtB12550_Dtor, {o});
}
// T::~T thunk for the base at +384
DECOMP(001782D0, VtB14968_DtorThunk384) { VtB14968DtorAt(ctx, arg<u32>(ctx, 0) - 384); }
// T::~T thunk for the base at +380
DECOMP(00178358, VtB14968_DtorThunk380) { VtB14968DtorAt(ctx, arg<u32>(ctx, 0) - 380); }

// u32 f(T* o, D* d): Thunk_000E44C0(o, d), then d->+208.. = the 5 f32 at
// +240.. and the top two bits of d->+228 = those of +260
DECOMP(001ABA70, Obj_CopyState_001ABA70) {
    const u32 o = arg<u32>(ctx, 0), d = arg<u32>(ctx, 1);
    call(ctx, 0x000E44C0, {o, d});
    Cp(d + 208, o + 240);
    Cp(d + 212, o + 244);
    Cp(d + 216, o + 248);
    const u32 w = load<u32>(o + 252);
    u32 f = load<u32>(d + 228);
    store<u32>(d + 220, w);
    const u32 w2 = load<u32>(o + 256);
    f &= 0x7FFFFFFFu;
    store<u32>(d + 224, w2);
    f |= load<u32>(o + 260) & 0x80000000u;
    store<u32>(d + 228, f);
    const u32 g = load<u32>(o + 260);
    f = (f & ~0x40000000u) | (g & 0x40000000u);
    store<u32>(d + 228, f);
    ctx.fpr[1] = static_cast<f64>(std::bit_cast<f32>(w2));
    ctx.gpr[3] = f;
    ctx.gpr[4] = g;
}

// void f(T* o): 0x000E1420(o), then the 7 emitters at +428 (16 apart)
// stopped (Emitter_Stop(h, 1)) and cleared
DECOMP(001AE768, Obj_StopEmitters_001AE768) {
    const u32 o = arg<u32>(ctx, 0);
    call(ctx, 0x000E1420, {o});
    for (u32 i = 0; i < 7; ++i) {
        const u32 e = o + 428 + 16 * i;
        call(ctx, o25fn::Emitter_Stop, {load<u32>(e), 1});
        store<u32>(e, 0);
    }
}

// T::~T: vtable 0x00B16058, AnimSkel_Release(+228) when set,
// ~PtrVector(+192), then the 0x00B11D98 base destructor
DECOMP(001B3AAC, VtB16058_Dtor) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 p = load<u32>(o + 228);
    store<u32>(o, 0x00B16058);
    if (p != 0) call(ctx, o26fn::AnimSkel_Release, {p});
    VecDtor(ctx, o + 192);
    store<u32>(o, 0x00B11D98);
    call(ctx, o26fn::Obj_BaseDtor_0C2DA0, {o});
}

// u32 f(T* o, a, s32 k, b): 0 when k is 4, or k is 0 and the u16 +584 has
// bit 0x8000; else 0x000DF960(o, a, k, b)
DECOMP(001B900C, Obj_Dispatch_001B900C) {
    const u32 o = arg<u32>(ctx, 0), a = arg<u32>(ctx, 1), b = arg<u32>(ctx, 3);
    const s32 k = arg<s32>(ctx, 2);
    bool skip;
    if (k > 0) skip = k == 4;
    else skip = k > -1 && (load<u16>(o + 584) & 0x8000u) != 0;
    if (skip) {
        ctx.gpr[3] = 0;
        return;
    }
    ctx.gpr[3] = call(ctx, 0x000DF960, {o, a, Sx(static_cast<u32>(k)), b});
}

// u32 f(T* o, u8 on): when on and +552 is not 0x03F0D3DA: +552 = it and
// Effect_SetHash4CF988(+548, it) when +548 is set; then 0x000DB0C0(o, on)
DECOMP(00243EC4, Obj_SetEffect552_00243EC4) {
    constexpr u32 kHash = 0x03F0D3DA;
    const u32 o = arg<u32>(ctx, 0), on = arg<u32>(ctx, 1) & 0xFFu;
    if (on != 0 && load<u32>(o + 552) != kHash) {
        const u32 e = load<u32>(o + 548);
        store<u32>(o + 552, kHash);
        if (e != 0) call(ctx, 0x004CF988, {e, kHash});
    }
    ctx.gpr[3] = call(ctx, 0x000DB0C0, {o, on});
}

// T::~T: vtable 0x00B1EEE0, ~PtrVector of the 6 vectors at +2888 down to
// +2588 (60 apart), then the base destructor 0x003286D8
DECOMP(00339248, VtB1EEE0_Dtor) {
    const u32 o = arg<u32>(ctx, 0);
    store<u32>(o, 0x00B1EEE0);
    for (s32 i = 5; i >= 0; --i) VecDtor(ctx, o + 2888 - 60u * static_cast<u32>(5 - i));
    ctx.gpr[3] = call(ctx, 0x003286D8, {o});
}

// void f(T* o): o->vfn[20](o, 0), Obj_Place3626E8(o, 0x01164B40, 1), +240
// = +244 = 1.0f, +228 = +220 = 0 and +160 = (+160 & 1) | 1
DECOMP(0036D874, Obj_Reset_0036D874) {
    const u32 o = arg<u32>(ctx, 0);
    VCall(ctx, o, 20, {o, 0});
    call(ctx, 0x003626E8, {o, 0x01164B40, 1});
    const u32 v = (load<u32>(o + 160) & 1u) | 1u;
    store<u32>(o + 240, 0x3F800000);
    store<u32>(o + 244, 0x3F800000);
    store<u32>(o + 228, 0);
    store<u32>(o + 220, 0);
    store<u32>(o + 160, v);
    ctx.gpr[3] = v;
    ctx.gpr[4] = 0x3F800000;
    ctx.gpr[5] = 0;
}

// void f(T* o): Obj_NotifyA362F1C(o, 0, 0), 0x0036BCAC(o), +160 &= 1,
// +408 = -1, +404 = +400 = +340 = 0, then o->vfn[20](o, 0)
DECOMP(00381758, Obj_Reset_00381758) {
    const u32 o = arg<u32>(ctx, 0);
    call(ctx, 0x00362F1C, {o, 0, 0});
    call(ctx, 0x0036BCAC, {o});
    const u32 v = load<u32>(o + 160) & 1u;
    const u32 vt = load<u32>(o);  // read before the stores
    store<s32>(o + 408, -1);
    store<u32>(o + 160, v);
    store<u32>(o + 404, 0);
    store<u32>(o + 400, 0);
    const u32 opd = load<u32>(vt + 20);
    store<u32>(o + 340, 0);
    call_ptr(ctx, opd, {o, 0, opd, load<u32>(opd)});
}

// T* f(u32 key): the first of the u16 0x010AA694->+8 objects in the array
// at 0x010AA694 without bit 0x20000000 in +24 whose +0 is key (0 for key
// 0)
DECOMP(0042BD0C, Obj_FindByKey_0042BD0C) {
    const u32 key = arg<u32>(ctx, 0);
    ctx.gpr[3] = 0;
    if (key == 0) return;
    const u32 arr = load<u32>(0x010AA694);
    const u32 n = load<u16>(0x010AA694 + 8);
    for (u32 i = 0; i < n; ++i) {
        const u32 e = load<u32>(arr + 4 * i);
        if ((load<u32>(e + 24) & 0x20000000u) == 0 && load<u32>(e) == key) {
            ctx.gpr[3] = load<u32>(arr + 4 * i);
            return;
        }
    }
}

// void f(T* o): with r = Obj_FindByHandle42C2B0(+12, 0), +80 = the vector
// r->+144; then 0x002F2140(the global 0x00F64C38, o + 16) and
// Flag_Set8_40000000(o)
DECOMP(0042D814, Obj_Apply_0042D814) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 r = static_cast<u32>(call(ctx, 0x0042C2B0, {load<u32>(o + 12), 0}));
    if (r != 0) {
        Cp(o + 80, r + 144);
        Cp(o + 84, r + 148);
        ctx.fpr[1] = static_cast<f64>(std::bit_cast<f32>(Cp(o + 88, r + 152)));
    }
    call(ctx, 0x002F2140, {load<u32>(0x00F64C38), o + 16});
    ctx.gpr[3] = call(ctx, o24fn::Flag_Set8_40000000, {o});
}

// T::T(o, a, b, c, u8 d, u8 on): the 0x00B20CC8 base
// (Obj_InitVt_B20CC8(o, on)), +16 = a, +20 = b, +24 = c, +12 = 0, vtable
// 0x00B20F38, +28 = d
DECOMP(0042FACC, VtB20F38_Ctor) {
    const u32 o = arg<u32>(ctx, 0), a = arg<u32>(ctx, 1), b = arg<u32>(ctx, 2), c = arg<u32>(ctx, 3);
    const u32 d = arg<u32>(ctx, 4), on = arg<u32>(ctx, 5) & 0xFFu;
    call(ctx, o24fn::Obj_InitVt_B20CC8, {o, on});
    store<u32>(o + 16, a);
    store<u32>(o + 20, b);
    store<u32>(o + 24, c);
    store<u32>(o + 12, 0);
    store<u32>(o, 0x00B20F38);
    store<u8>(o + 28, static_cast<u8>(d));
    ctx.gpr[3] = 0x00B20F38;
    ctx.gpr[4] = 0;
}

// void f(T* o): Obj_RegisterParam(o, p) for the 5 parameter records at
// 0x010AB400, 0x010AB420, 0x010AB460, 0x010AB4A0, 0x010AB4E0
DECOMP(0043C3E4, Obj_RegisterParams_0043C3E4) {
    const u32 o = arg<u32>(ctx, 0);
    for (u32 p : {0x010AB400u, 0x010AB420u, 0x010AB460u, 0x010AB4A0u, 0x010AB4E0u}) ctx.gpr[3] = call(ctx, 0x004F7128, {o, p});
}

// void f(T* o): Obj_RegisterParam(o, p) for 0x010AB578, 0x010AB590 and
// 0x010AB5A8; then +8->+44 = 0x00AE3720, bit 0x80 of the u8 +8->+40 set,
// and +88 = 0x00F38248
DECOMP(0043C5DC, Obj_RegisterParams_0043C5DC) {
    const u32 o = arg<u32>(ctx, 0);
    for (u32 p : {0x010AB578u, 0x010AB590u, 0x010AB5A8u}) call(ctx, 0x004F7128, {o, p});
    const u32 q = load<u32>(o + 8);
    const u32 b = load<u8>(q + 40) | 0x80u;
    store<u32>(q + 44, 0x00AE3720);
    store<u8>(q + 40, static_cast<u8>(b));
    store<u32>(o + 88, 0x00F38248);
    ctx.gpr[3] = q;
    ctx.gpr[4] = 0x00F38248;
    ctx.gpr[5] = 0x00AE3720;
    ctx.gpr[6] = b;
}

// bool Audio_IsEmitterValid(u32 h): whether the emitter handle h (index h >> 16, serial h & 0xFFFF)
// names a live record: the u8 0x010AEC60 set, a nonzero serial, the index
// below 0x010B0200->+24 and the 272-byte record at +28 + 256 holding the
// serial in bits 15..30 with its top bit clear
DECOMP(004437CC, Audio_IsEmitterValid) {
    const u32 h = arg<u32>(ctx, 0);
    ctx.gpr[3] = 0;
    if (load<u8>(0x010AEC60) == 0) return;
    const u32 lo = h & 0xFFFFu;
    if (lo == 0) return;
    const u32 hi = h >> 16;
    constexpr u32 kTab = 0x010B0200;
    if (!(static_cast<s32>(hi) < load<s32>(kTab + 24))) return;
    const u32 rec = load<u32>(kTab + 28) + hi * 272u + 256u;
    if (((load<u32>(rec) >> 15) & 0xFFFFu) != lo) return;
    if ((load<u8>(rec) & 0x80u) != 0) return;
    ctx.gpr[3] = 1;
}

// s32 f(File* f): fios_CloseFile(Fios_GetScheduler5FC90C(), &attr, f)
// with attr {u64 1, u8 127, 0...}, then -5 when 0x005F8A38(op, 0) fails
DECOMP(0044C374, File_Close_0044C374) {
    const u32 f = arg<u32>(ctx, 0);
    Stack frame(ctx, 32);
    const u32 a = frame.addr();
    store<u64>(a, 1);
    store<u32>(a + 20, 0);
    store<u32>(a + 12, 0);
    store<u32>(a + 16, 0);
    store<u8>(a + 8, 127);
    store<u32>(a + 28, 0);
    store<u32>(a + 24, 0);
    // r3 = 127, r4 = 1, r5 = 0 at the call, as the original leaves them
    const u32 s = static_cast<u32>(call(ctx, 0x005FC90C, {127, 1, 0}));
    const u32 op = static_cast<u32>(call(ctx, 0x006001D4, {s, a, f}));
    const u32 r = static_cast<u32>(call(ctx, 0x005F8A38, {op, 0}));
    ctx.gpr[3] = r != 0 ? static_cast<u64>(-5) : 0;
}

// void f(void* p): Mem_FreeFrom(p, 0) when p is set, adding the change of
// Global_Get10F0EDC(0x010B19E0) across it to the u32 at 0x010AC934
// (returned)
DECOMP(0044DCC0, Mem_FreeTimed_0044DCC0) {
    constexpr u32 kClock = 0x010B19E0, kTotal = 0x010AC934;
    const u32 p = arg<u32>(ctx, 0);
    const u32 t0 = static_cast<u32>(call(ctx, 0x0046458C, {kClock}));
    if (p != 0) call(ctx, o25fn::Mem_FreeFrom, {p, 0});
    const u32 t1 = static_cast<u32>(call(ctx, 0x0046458C, {kClock}));
    const u32 old = load<u32>(kTotal);
    const u32 v = old + (t1 - t0);
    store<u32>(kTotal, v);
    ctx.gpr[3] = v;
    ctx.gpr[4] = 0x010B0000;
    ctx.gpr[5] = old;
}

// The 2-bit field array of the level record (world + 4056 * the u8 rec->+0,
// words at +4016): bit k of entry idx set (on) or cleared; returns the
// word's address
static void LevelBits(PPUContext& ctx, u32 k) {
    const u32 rec = arg<u32>(ctx, 0), idx = arg<u32>(ctx, 1), on = arg<u32>(ctx, 2) & 0xFFu;
    const u32 base = load<u32>(kWorldPtr) + load<u8>(rec) * 4056u;
    const u32 off = (idx >> 2) & 0x3FFFFFFCu;
    const u32 sh = (idx << 1) & 0x1Eu;
    const u32 ea = base + off + 4016;
    u32 w = load<u32>(ea);
    if (on != 0) {
        w |= k << sh;
        ctx.gpr[6] = k << sh;
        ctx.gpr[7] = sh;
    } else {
        w &= ~(k << sh);
        ctx.gpr[6] = ~(k << sh);
        ctx.gpr[7] = k;
    }
    store<u32>(ea, w);
    ctx.gpr[3] = ea;
    ctx.gpr[4] = off;
    ctx.gpr[5] = w;
}
// void f(Rec* rec, u32 idx, u8 on): bit 0 of the level record's 2-bit entry
DECOMP(00465D88, Level_SetBit1_00465D88) { LevelBits(ctx, 1); }
// void f(Rec* rec, u32 idx, u8 on): bit 1 of the level record's 2-bit entry
DECOMP(00465EA0, Level_SetBit2_00465EA0) { LevelBits(ctx, 2); }

// u32 f(T* o, M* m, a, b): 0x0024F758(o, m, a, b) when m->+20 is
// 0xBDDD8ECD (read only when the u8 m->+24 has bit 0x80); else 0
DECOMP(0048CCC8, Obj_OnMsg_0048CCC8) {
    const u32 o = arg<u32>(ctx, 0), m = arg<u32>(ctx, 1), a = arg<u32>(ctx, 2), b = arg<u32>(ctx, 3);
    u32 k = 0;
    if ((load<u8>(m + 24) & 0x80u) != 0) k = load<u32>(m + 20);
    ctx.gpr[3] = 0;
    if (k != 0xBDDD8ECDu) return;
    ctx.gpr[3] = call(ctx, 0x0024F758, {o, m, a, b});
}

// void f(T* o, a): Obj_SetColourD6F90(+160, a, o) (f1 passed on), or, when
// bit 0x2 of +184 is set, Obj_SetColourD6F90(+160, &{0, 0, 0, 0}, 0,
// 0x00743650) with f1 = the constant there
DECOMP(00743658, Obj_SetColour_00743658) {
    const u32 o = arg<u32>(ctx, 0), a = arg<u32>(ctx, 1);
    if ((load<u64>(o + 184) & (1ull << 33)) == 0) {
        callf(ctx, 0x000D6F90, {load<u32>(o + 160), a, o}, {farg(ctx, 0)});
        return;
    }
    Stack frame(ctx, 16);
    const u32 l = frame.addr();
    store<u8>(l, 0);
    store<u8>(l + 1, 0);
    store<u8>(l + 2, 0);
    store<u8>(l + 3, 0);
    callf(ctx, 0x000D6F90, {load<u32>(o + 160), l, 0, 0x00743650}, {LfsAt(0x00743650)});
}

// u32 f(T* o): the +28 of the first 48-byte entry of the list +388 when it
// holds exactly one entry; else 0
DECOMP(00747B90, Obj_OnlyEntry_00747B90) {
    const u32 o = arg<u32>(ctx, 0);
    ctx.gpr[3] = 0;
    const u32 l = load<u32>(o + 388);
    ctx.gpr[4] = l;
    if (l == 0) return;
    const u32 n = load<u16>(l + 8);
    ctx.gpr[5] = n;
    if (n != 1) return;
    const u32 e = load<u32>(l);
    ctx.gpr[3] = load<u32>(e + 28);
    ctx.gpr[4] = e;
    ctx.gpr[5] = l;
    ctx.gpr[6] = 0;
    ctx.gpr[7] = 1;
}

// The VtB18B48 destructor: vtables 0x00B18B48 / 0x00B18CE0 (+380) /
// 0x00B18D08 (+384), Shape_Release638498(o + 436), the base vtables
// 0x00B12528 (+384) / 0x00B1200C (+380) / 0x00B126B8, then VtB12550_Dtor
static void VtB18B48Dtor(PPUContext& ctx) {
    const u32 o = arg<u32>(ctx, 0);
    store<u32>(o, 0x00B18B48);
    store<u32>(o + 380, 0x00B18CE0);
    store<u32>(o + 384, 0x00B18D08);
    call(ctx, 0x00638498, {o + 436});
    store<u32>(o + 384, 0x00B12528);
    store<u32>(o + 380, 0x00B1200C);
    store<u32>(o, 0x00B126B8);
    call(ctx, o23fn::VtB12550_Dtor, {o});
}
// T::~T (vtable 0x00B18B48)
DECOMP(00758248, VtB18B48_Dtor) { VtB18B48Dtor(ctx); }
// T::~T (vtable 0x00B18B48), a second copy
DECOMP(007586A8, VtB18B48_Dtor2) { VtB18B48Dtor(ctx); }
// T::~T (vtable 0x00B18B48), a third copy
DECOMP(00758B88, VtB18B48_Dtor3) { VtB18B48Dtor(ctx); }

// T::~T: vtable 0x00B1D4B8, 0x002EA464(o), Mem_FreeFrom(+36, 0) and +36 =
// 0 when set, ~PtrVector(+76), then the base destructor 0x002E87D4
DECOMP(007672EC, VtB1D4B8_Dtor) {
    const u32 o = arg<u32>(ctx, 0);
    store<u32>(o, 0x00B1D4B8);
    call(ctx, 0x002EA464, {o});
    const u32 p = load<u32>(o + 36);
    if (p != 0) {
        call(ctx, o25fn::Mem_FreeFrom, {p, 0});
        store<u32>(o + 36, 0);
    }
    VecDtor(ctx, o + 76);
    ctx.gpr[3] = call(ctx, 0x002E87D4, {o});
}

// T::~T deleting form thunk for the base at +12
DECOMP(007719DC, Mi3VtB1F4C0_DeleteThunk12) { VtB1F4C0DeleteAt(ctx, arg<u32>(ctx, 0) - 12); }
// T::~T deleting form thunk for the base at +8
DECOMP(00771A64, Mi3VtB1F4C0_DeleteThunk8) { VtB1F4C0DeleteAt(ctx, arg<u32>(ctx, 0) - 8); }
