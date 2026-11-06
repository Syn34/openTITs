// Game code, part 26 (continued): the sweep of small still-recompiled game
// functions from 35 instructions up. See obj26.cpp.
#include "obj26.h"

using namespace sly4;
using namespace sly4::obj26;

// ---------------------------------------------------------------- sweep: 35 instructions
// u32 f(T* o): an address in the 116-byte record +716 of the table +612 +
// 92: its +80 (when +516 has bit 0x40000000), else its +60 (bit
// 0x80000000), else its +20; plus 44 * +596
DECOMP(0003E040, Obj_RecordAddr_0003E040) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 a = load<u32>(o + 716);
    const u32 tab = load<u32>(o + 612);
    const u32 f = load<u32>(o + 516);
    const u32 rec = tab + a * 116u + 92u;
    u32 base;
    if ((f & 0x40000000u) != 0) base = load<u32>(rec + 80);
    else if ((f & 0x80000000u) != 0) base = load<u32>(rec + 60);
    else base = load<u32>(rec + 20);
    const u32 b = load<u32>(o + 596);
    ctx.gpr[3] = base + b * 44u;
    ctx.gpr[4] = b * 44u;
    ctx.gpr[5] = b * 16u;
    ctx.gpr[6] = tab;
    ctx.gpr[7] = a * 12u;
    ctx.gpr[8] = (f >> 30) & 1u;
}

// void f(T* o): for p = (world + 0x20C80 * +12)->+4208->+8 (when set): bit 0
// of the u8 +16 into bit 0x80 of the u8 p->+968 (when p->+192 has bit 0x8),
// else into bit 0x20000000 of p->+384 (when p->+200 has bit 0x400)
DECOMP(0008328C, Obj_ApplyFlag16_0008328C) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 p = load<u32>(load<u32>(kWorldPtr) + load<u32>(o + 12) * 0x20C80u + 4208 + 8);
    ctx.gpr[4] = p;
    if (p == 0) return;
    if ((load<u32>(p + 192) & 0x8u) != 0) {
        const u32 v = load<u8>(o + 16);
        const u32 b = (load<u8>(p + 968) & 0x7Fu) | ((v & 1u) << 7);
        store<u8>(p + 968, static_cast<u8>(b));
        ctx.gpr[3] = v;
        ctx.gpr[5] = b;
        return;
    }
    const u32 g = load<u32>(p + 200) & 0x400u;
    ctx.gpr[5] = g;
    if (g == 0) return;
    const u32 v = load<u8>(o + 16);
    const u32 w = (load<u32>(p + 384) & ~0x20000000u) | ((v & 1u) << 29);
    store<u32>(p + 384, w);
    ctx.gpr[3] = v;
    ctx.gpr[5] = w;
}

// u32 f(T* o, a): with e = Effect_Bind4BB7E0(a, +12, 0): when e->+276 is
// 0x02235C33 and e->+280 = p is set, 0x00282408(Obj_FindComponent(p, 34,
// 0), +16) when found
DECOMP(00089EB0, Obj_SetComp_00089EB0) {
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
    ctx.gpr[3] = call(ctx, 0x00282408, {c, load<u32>(o + 16)});
}

// void f(T* o): when +12 is set, pushes a new VtB20DB8 (24 bytes) made from
// (+12, +16, the u8 +20)
DECOMP(0009BF88, Cmd_PushB20DB8_0009BF88) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 a = load<u32>(o + 12);
    ctx.gpr[3] = a;
    if (a == 0) return;
    PushNew26(ctx, 24, [&](u32 p) { call(ctx, 0x0042DED0, {p, load<u32>(o + 12), load<u32>(o + 16), load<u8>(o + 20)}); });
}
// void f(T* o): when +12 is set, pushes a new VtB20DE8 (20 bytes) made from
// (+12, the u8 +16, the u8 +17)
DECOMP(0009C394, Cmd_PushB20DE8_0009C394) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 a = load<u32>(o + 12);
    ctx.gpr[3] = a;
    if (a == 0) return;
    PushNew26(ctx, 20, [&](u32 p) { call(ctx, 0x0042E20C, {p, load<u32>(o + 12), load<u8>(o + 16), load<u8>(o + 17)}); });
}
// void f(T* o): pushes a new 0x0042E868 (20 bytes) made from (+12, the u8
// +16 .. +20)
DECOMP(0009C9C4, Cmd_Push42E868_0009C9C4) {
    const u32 o = arg<u32>(ctx, 0);
    PushNew26(ctx, 20, [&](u32 p) {
        call(ctx, 0x0042E868, {p, load<u32>(o + 12), load<u8>(o + 16), load<u8>(o + 17), load<u8>(o + 18),
                               load<u8>(o + 19), load<u8>(o + 20)});
    });
}
// void f(T* o): pushes a new 0x00437D58 (12 bytes) made from (the u8 +12 ..
// +17)
DECOMP(000A3D10, Cmd_Push437D58_000A3D10) {
    const u32 o = arg<u32>(ctx, 0);
    PushNew26(ctx, 12, [&](u32 p) {
        call(ctx, 0x00437D58, {p, load<u8>(o + 12), load<u8>(o + 13), load<u8>(o + 14), load<u8>(o + 15),
                               load<u8>(o + 16), load<u8>(o + 17)});
    });
}

// void f(T* o, u32 x): removes x from the vector +252 (the u16 count +8;
// the last element moves into its place)
DECOMP(000DD1CC, Vec_SwapRemove252_000DD1CC) {
    const u32 o = arg<u32>(ctx, 0), x = arg<u32>(ctx, 1);
    const u32 v = load<u32>(o + 252);
    ctx.gpr[3] = v;
    s32 i = 0;
    u32 arr = 0;
    for (;;) {
        const s32 n = v != 0 ? static_cast<s32>(load<u16>(v + 8)) : 0;
        if (!(i < n)) return;
        arr = load<u32>(v);
        if (load<u32>(arr + 4u * static_cast<u32>(i)) == x) break;
        ++i;
    }
    if (i < 0) return;
    const u32 n1 = load<u16>(v + 8) - 1u;
    store<u16>(v + 8, static_cast<u16>(n1));
    ctx.gpr[3] = n1;
    ctx.gpr[4] = v;
    if (static_cast<u32>(i) == (n1 & 0xFFFFu)) return;
    const u32 last = load<u32>(arr + ((n1 << 2) & 0x3FFFCu));
    store<u32>(arr + 4u * static_cast<u32>(i), last);
    ctx.gpr[3] = last;
}

// T::~T deleting form (752 bytes): vtables 0x00B13528 / 0x00B136C8
// (+380) / 0x00B136F0 (+384), VtB21B58_DtorTrivial of +680 and +644, the
// base destructor 0x000EB0F8. Null does nothing (r3 = 0).
static void VtB13528DeleteAt(PPUContext& ctx, u32 o) {
    if (o == 0) {
        ctx.gpr[3] = 0;
        return;
    }
    store<u32>(o, 0x00B13528);
    store<u32>(o + 380, 0x00B136C8);
    store<u32>(o + 384, 0x00B136F0);
    call(ctx, 0x004A0E08, {o + 680});
    call(ctx, 0x004A0E08, {o + 644});
    call(ctx, 0x000EB0F8, {o});
    call(ctx, 0x00552420, {o, 752});
}
// T::~T deleting form (752 bytes, vtable 0x00B13528)
DECOMP(000F4F40, VtB13528_Delete) { VtB13528DeleteAt(ctx, arg<u32>(ctx, 0)); }

// s32 f(T** a, T** b): String_Compare((*a)->vfn[12](*a), (*b)->vfn[12](*b))
DECOMP(000F8650, Obj_CompareNames_000F8650) {
    const u32 a = load<u32>(arg<u32>(ctx, 0)), b = load<u32>(arg<u32>(ctx, 1));
    const u32 sa = static_cast<u32>(VCall(ctx, a, 12, {a}));
    const u32 sb = static_cast<u32>(VCall(ctx, b, 12, {b}));
    ctx.gpr[3] = call(ctx, 0x00555D94, {sa, sb});
}

// void f(T* o, Ref* p): unless +16 is already p: the old one released
// (vfn[40](it, 0)), +16 = p and the u16 p->+178 += 1
DECOMP(001089A8, Obj_SetRef16_001089A8) {
    const u32 o = arg<u32>(ctx, 0), p = arg<u32>(ctx, 1);
    const u32 q = load<u32>(o + 16);
    ctx.gpr[3] = q;
    if (q == p) return;
    if (q != 0) ctx.gpr[3] = VCall(ctx, q, 40, {q, 0});
    store<u32>(o + 16, p);
    if (p == 0) return;
    const u32 c = static_cast<u16>(load<u16>(p + 178) + 1);
    store<u16>(p + 178, static_cast<u16>(c));
    ctx.gpr[3] = p + 178;
    ctx.gpr[4] = c;
}

// T::~T deleting form (224 bytes): vtable 0x00B13EB8, (+16)->vfn[84](+16,
// o + 32), vtable 0x00B0DAEC. Null does nothing (r3 = 0).
DECOMP(00112648, VtB13EB8_Delete) {
    const u32 o = arg<u32>(ctx, 0);
    if (o == 0) {
        ctx.gpr[3] = 0;
        return;
    }
    const u32 q = load<u32>(o + 16);
    store<u32>(o, 0x00B13EB8);
    VCall(ctx, q, 84, {q, o + 32});
    store<u32>(o, 0x00B0DAEC);
    call(ctx, 0x00552420, {o, 224});
}

// The VtB14138 destructor: when +1416 is set, vtable 0x00B14138 and
// Mem_FreeIfSet(+1416, +1412); the six subobject vtables 0x00B0DB3C
// (+1296, +1136, +992, +772, +720, +240), then the 0x00B11D98 base
static void VtB14138Dtor(PPUContext& ctx) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 p = load<u32>(o + 1416);
    if (p != 0) {
        const u32 a = load<u32>(o + 1412);
        store<u32>(o, 0x00B14138);
        call(ctx, o26fn::Mem_FreeIfSet, {p, a});
    }
    for (u32 off : {1296u, 1136u, 992u, 772u, 720u, 240u}) store<u32>(o + off, 0x00B0DB3C);
    store<u32>(o, 0x00B11D98);
    call(ctx, o26fn::Obj_BaseDtor_0C2DA0, {o});
}
// T::~T (vtable 0x00B14138)
DECOMP(0011BBC4, VtB14138_Dtor) { VtB14138Dtor(ctx); }
// T::~T (vtable 0x00B14138), a second copy
DECOMP(0011BCF4, VtB14138_Dtor2) { VtB14138Dtor(ctx); }

// bool f(T* o, f32 a, f32 b): 1 when +552 has bit 0x20000000, or b is at
// least the constant at 0x001C9D0C and a below b; otherwise
// o->vfn[420](o, 2, 0) and 0
DECOMP(001C9D10, Obj_CheckRange_001C9D10) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 vt = load<u32>(o);
    const f64 a = farg(ctx, 0), b = farg(ctx, 1);
    if ((load<u32>(o + 552) & 0x20000000u) != 0) {
        ctx.gpr[3] = 1;
        return;
    }
    const f64 k = LfsAt(0x001C9D0C);
    ctx.fpr[3] = k;
    if (b < k || a >= b) {
        VCallVt(ctx, vt, 420, {o, 2, 0});
        ctx.gpr[3] = 0;
        return;
    }
    ctx.gpr[3] = 1;
}

// bool f(T* o, Obj* t): unless o->+216->+304 is set: when t has bit 0x8 in
// +192 and its component 23 (Obj_FindComponent) has a 5-bit state (+24 >>
// 25) other than 1, 0x00276558(it, 1); returns 1
DECOMP(001DC504, Obj_Kick23_001DC504) {
    const u32 o = arg<u32>(ctx, 0), t = arg<u32>(ctx, 1);
    if (load<u32>(load<u32>(o + 216) + 304) == 0 && t != 0 && (load<u32>(t + 192) & 0x8u) != 0) {
        const u32 c = static_cast<u32>(call(ctx, 0x000D1288, {t, 23, 0}));
        if (c != 0 && ((load<u32>(c + 24) >> 25) & 0x1Fu) != 1) call(ctx, 0x00276558, {c, 1});
    }
    ctx.gpr[3] = 1;
}

// void f(T* o, Ref* p): unless +380 is already p: the old one released
// (vfn[40](it, 0)), +380 = p and the u16 p->+178 += 1
DECOMP(002CC7F8, Obj_SetRef380_002CC7F8) {
    const u32 o = arg<u32>(ctx, 0), p = arg<u32>(ctx, 1);
    const u32 q = load<u32>(o + 380);
    ctx.gpr[3] = q;
    if (q == p) return;
    if (q != 0) ctx.gpr[3] = VCall(ctx, q, 40, {q, 0});
    store<u32>(o + 380, p);
    if (p == 0) return;
    const u32 c = static_cast<u16>(load<u16>(p + 178) + 1);
    store<u16>(p + 178, static_cast<u16>(c));
    ctx.gpr[3] = p + 178;
    ctx.gpr[4] = c;
}

// u32 f(T* o): the element after o->+72 in the u16 (world + 4208)->+88
// words at (world + 4208)->+80 (0 when not found, last or +72 is 0)
DECOMP(002E2214, Level_NextAfter_002E2214) {
    const u32 key = load<u32>(arg<u32>(ctx, 0) + 72);
    ctx.gpr[3] = 0;
    if (key == 0) return;
    const u32 base = load<u32>(kWorldPtr) + 4208;
    const u32 n = load<u16>(base + 88);
    for (u32 i = 0; i < n;) {
        const u32 arr = load<u32>(base + 80);
        const u32 v = load<u32>(arr + 4 * i);
        ++i;
        if (v == key) {
            if (i < n) ctx.gpr[3] = load<u32>(arr + 4 * i);
            return;
        }
    }
}

// T::~T deleting form (68 bytes): when +48 is set, vtable 0x00B1D730,
// (+48)->vfn[4](+48) and +48 = 0; then vtable 0x00B1D6A0. Null does
// nothing (r3 = 0).
DECOMP(002FD838, VtB1D730_Delete) {
    const u32 o = arg<u32>(ctx, 0);
    if (o == 0) {
        ctx.gpr[3] = 0;
        return;
    }
    const u32 p = load<u32>(o + 48);
    if (p != 0) {
        const u32 vt = load<u32>(p);  // read before the store
        store<u32>(o, 0x00B1D730);
        VCallVt(ctx, vt, 4, {p});
        store<u32>(o + 48, 0);
    }
    store<u32>(o, 0x00B1D6A0);
    call(ctx, 0x00552420, {o, 68});
}

// void f(T* o): when Obj_CanShow2E2104(the global 0x00F64C20, o): for the
// u8 state +18 of 5 (or 4), User_Start325B58(o, 3 (or 4)) and
// Loader_Tick3250A0(o, 3 (or 4))
DECOMP(00325C58, Obj_StartByState_00325C58) {
    const u32 o = arg<u32>(ctx, 0);
    if (static_cast<s32>(call(ctx, 0x002E2104, {load<u32>(0x00F64C20), o})) == 0) return;
    const u32 k = load<u8>(o + 18);
    ctx.gpr[3] = k;
    u32 m;
    if (k == 5) m = 3;
    else if (k == 4) m = 4;
    else return;
    call(ctx, 0x00325B58, {o, m});
    ctx.gpr[3] = call(ctx, 0x003250A0, {o, m});
}

// T* f(_, _, a): new (2580 bytes, SmallAlloc) T made by 0x00769BD8(p, a):
// vtable 0x00B1E458, +2572 = 0, the top two bits of +2576 cleared; 0 when
// the allocation fails
DECOMP(0032A3B4, New_VtB1E458_0032A3B4) {
    const u32 a = arg<u32>(ctx, 2);
    const u32 p = static_cast<u32>(call(ctx, o19fn::SmallAlloc, {2580}));
    if (p != 0) {
        call(ctx, 0x00769BD8, {p, a});
        const u32 f = load<u32>(p + 2576) & 0x3FFFFFFFu;
        store<u32>(p + 2572, 0);
        store<u32>(p, 0x00B1E458);
        store<u32>(p + 2576, f);
        ctx.gpr[4] = 0x00B1E458;
    }
    ctx.gpr[3] = p;
}

// The T::T(o, a, b, bool c, bool d) shape: the 0x00B20CC8 base
// (Obj_InitVt_B20CC8(o, 1)), +12 = a, +16 = b, vtable vt, the top bit of
// +20 = c and the next = d
static void CtorAB2Bits(PPUContext& ctx, u32 vt) {
    const u32 o = arg<u32>(ctx, 0), a = arg<u32>(ctx, 1), b = arg<u32>(ctx, 2), c = arg<u32>(ctx, 3), d = arg<u32>(ctx, 4);
    call(ctx, o24fn::Obj_InitVt_B20CC8, {o, 1});
    u32 f = load<u32>(o + 20);
    store<u32>(o + 12, a);
    store<u32>(o + 16, b);
    f = (f & 0x3FFFFFFFu) | ((c & 1u) << 31) | ((d & 1u) << 30);
    store<u32>(o, vt);
    store<u32>(o + 20, f);
    ctx.gpr[3] = f;
    ctx.gpr[4] = vt;
}
// T::T(o, a, b, bool c, bool d) (vtable 0x00B21208)
DECOMP(00438348, VtB21208_Ctor) { CtorAB2Bits(ctx, 0x00B21208); }
// T::T(o, a, b, bool c, bool d) (vtable 0x00B21310)
DECOMP(004390AC, VtB21310_Ctor) { CtorAB2Bits(ctx, 0x00B21310); }

// T::T(o, a, b, u8 c, u8 d, u8 on): the 0x00B20CC8 base
// (Obj_InitVt_B20CC8(o, on)), +16 = a, +20 = b, +24 = c, +12 = +28 = 0,
// vtable 0x00B213E8, +32 = d
DECOMP(0043A84C, VtB213E8_Ctor) {
    const u32 o = arg<u32>(ctx, 0), a = arg<u32>(ctx, 1), b = arg<u32>(ctx, 2), c = arg<u32>(ctx, 3);
    const u32 d = arg<u32>(ctx, 4), on = arg<u32>(ctx, 5) & 0xFFu;
    call(ctx, o24fn::Obj_InitVt_B20CC8, {o, on});
    store<u32>(o + 16, a);
    store<u32>(o + 20, b);
    store<u8>(o + 24, static_cast<u8>(c));
    store<u32>(o + 12, 0);
    store<u32>(o, 0x00B213E8);
    store<u32>(o + 28, 0);
    store<u8>(o + 32, static_cast<u8>(d));
    ctx.gpr[3] = 0x00B213E8;
    ctx.gpr[4] = 0;
}

// void f(World* w, u32 i): Global_F64CBC_Copy216(i & 0xFF); when w->+0x12847C
// = p is set with the top bit of +164, Treasure_ReleaseIf_0042681C(p, (w +
// 0x20C80 i)->+4208->+8)
DECOMP(0047BC68, World_ReleaseTreasure_0047BC68) {
    const u32 w = arg<u32>(ctx, 0), i = arg<u32>(ctx, 1);
    call(ctx, 0x00369658, {i & 0xFFu});
    const u32 p = load<u32>(w + 0x12847Cu);
    ctx.gpr[3] = p;
    if (p == 0) return;
    if ((load<u32>(p + 164) & 0x80000000u) == 0) return;
    ctx.gpr[3] = call(ctx, 0x0042681C, {p, load<u32>(w + i * 0x20C80u + 4208 + 8)});
}

// void f(): 0x002B7704(q) for q = (world->+20 + the u16 world->+6)->+92
// when set; then the u8 world->+47 = *world->+8 and, by the u8 state
// world->+50: 0 or 12 and up -> StateMachine_SetState(world, 1, 2), else
// the u8 world->+49 = 2
DECOMP(0048A26C, World_Step_0048A26C) {
    u32 w = load<u32>(kWorldPtr);
    const u32 q = load<u32>(load<u32>(w + 20) + load<u16>(w + 6) + 92);
    if (q != 0) {
        call(ctx, 0x002B7704, {q});
        w = load<u32>(kWorldPtr);
    }
    const u32 s = load<u8>(w + 50);
    const u32 b = load<u8>(load<u32>(w + 8));
    store<u8>(w + 47, static_cast<u8>(b));
    if (s == 0 || !(static_cast<s32>(s) < 12)) {
        ctx.gpr[3] = call(ctx, 0x00464D1C, {w, 1, 2});
        return;
    }
    store<u8>(w + 49, 2);
    ctx.gpr[3] = w;
    ctx.gpr[4] = 2;
    ctx.gpr[5] = s;
}

// T* f(const T* s): a copy (28 bytes, SmallAlloc; vtable 0x00B21EB0, u16
// +4 = 1) with +24, +20 and +8 from s. Quirk: the copy is written through
// a null pointer when the allocation fails.
DECOMP(004BF938, VtB21EB0_Clone) {
    const u32 s = arg<u32>(ctx, 0);
    const u32 p = static_cast<u32>(call(ctx, o19fn::SmallAlloc, {28}));
    if (p != 0) {
        store<u16>(p + 4, 1);
        store<u32>(p + 20, 0);
        store<u32>(p, 0x00B21EB0);
        store<s32>(p + 24, -1);
    }
    const u32 a = load<u32>(s + 24), b = load<u32>(s + 20);
    store<u32>(p + 24, a);
    const u32 c = load<u32>(s + 8);
    store<u32>(p + 20, b);
    store<u32>(p + 8, c);
    ctx.gpr[3] = p;
    ctx.gpr[4] = c;
    ctx.gpr[5] = b;
}
// T* f(const T* s): a copy (28 bytes, SmallAlloc; vtable 0x00B21F90, u16
// +4 = 1) with +20, +24 and +8 from s. Quirk: the copy is written through
// a null pointer when the allocation fails.
DECOMP(004C0A98, VtB21F90_Clone) {
    const u32 s = arg<u32>(ctx, 0);
    const u32 p = static_cast<u32>(call(ctx, o19fn::SmallAlloc, {28}));
    if (p != 0) {
        store<u16>(p + 4, 1);
        store<u32>(p + 8, 0);
        store<u32>(p, 0x00B21F90);
        store<u32>(p + 20, 0);
        store<u32>(p + 24, 0);
    }
    const u32 a = load<u32>(s + 20), b = load<u32>(s + 24);
    store<u32>(p + 20, a);
    store<u32>(p + 24, b);
    const u32 c = load<u32>(s + 8);
    store<u32>(p + 8, c);
    ctx.gpr[3] = p;
    ctx.gpr[4] = c;
    ctx.gpr[5] = b;
}

// void f(T* o, Obj* a, b, u16 c, bool d, const Vec3* v, f32 x, f32 y): +4
// = a, +8 = b, +12 = c; the u16 flags +14: 3 in bits 0xE000, 0x800 /
// 0x400 / 0x100 cleared, 0x200 = d, 0x1000 = bit 0 of a's u64 +184;
// vtable 0x00B0DADC, +16 = v, +32 = x, +36 = y
DECOMP(007358D4, Obj_Init_007358D4) {
    const u32 o = arg<u32>(ctx, 0), a = arg<u32>(ctx, 1), b = arg<u32>(ctx, 2), c = arg<u32>(ctx, 3);
    const u32 d = arg<u32>(ctx, 4), v = arg<u32>(ctx, 5);
    u32 f = load<u16>(o + 14);
    store<u32>(o + 4, a);
    f = (f & ~0xE000u) | 0x6000u;
    f &= ~0x800u;
    store<u32>(o + 8, b);
    f &= ~0x400u;
    store<u16>(o + 12, static_cast<u16>(c));
    f = (f & ~0x200u) | ((d & 1u) << 9);
    f &= ~0x100u;
    u32 bit = 0;
    if (a != 0) {
        store<u16>(o + 14, static_cast<u16>(f));
        if ((load<u64>(a + 184) & 1u) != 0) bit = 1;
    }
    f = (f & ~0x1000u) | (bit << 12);
    store<u16>(o + 14, static_cast<u16>(f));
    const u32 x0 = load<u32>(v);
    store<u32>(o, 0x00B0DADC);
    store<u32>(o + 16, x0);
    Cp(o + 20, v + 4);
    const u32 x2 = load<u32>(v + 8);
    store<f32>(o + 32, static_cast<f32>(farg(ctx, 0)));
    store<u32>(o + 24, x2);
    store<f32>(o + 36, static_cast<f32>(farg(ctx, 1)));
    ctx.fpr[3] = static_cast<f64>(std::bit_cast<f32>(x2));
    ctx.gpr[3] = o;
    ctx.gpr[4] = 0x00B0DADC;
    ctx.gpr[5] = bit;
    ctx.gpr[6] = f;
}

// void f(Rec* r, s32 k, const f32* v): r = {k, 0, 0, 0}; then for k = 0
// r->+12, +4, +8 = v[2], v[0], v[1]; k = 1: +4, +8 = v[0], v[1]; k = 2:
// +4, +8, +12 = v[0], v[1], v[2]
DECOMP(007608C4, Rec_SetByKind_007608C4) {
    const u32 r = arg<u32>(ctx, 0), v = arg<u32>(ctx, 2);
    const s32 k = arg<s32>(ctx, 1);
    store<u32>(r, static_cast<u32>(k));
    store<u32>(r + 4, 0);
    store<u32>(r + 8, 0);
    store<u32>(r + 12, 0);
    ctx.gpr[3] = r;
    ctx.gpr[4] = v + 8;
    u32 last = 0;
    bool any = true;
    if (k == 0) {
        Cp(r + 12, v + 8);
        Cp(r + 4, v);
        last = Cp(r + 8, v + 4);
        ctx.gpr[4] = v + 4;
    } else if (k == 1) {
        Cp(r + 4, v);
        last = Cp(r + 8, v + 4);
        ctx.gpr[4] = v + 4;
    } else if (k == 2) {
        Cp(r + 4, v);
        Cp(r + 8, v + 4);
        last = Cp(r + 12, v + 8);
        ctx.gpr[6] = v + 4;
    } else {
        any = false;
    }
    if (any) ctx.fpr[1] = static_cast<f64>(std::bit_cast<f32>(last));
}

// T* f(Scope* o, u32 key): the value for key in the map +8 (SortedMap_Find75E570),
// else the same lookup in the parent +0; 0 at the root
DECOMP(00760C54, Scope_Lookup_00760C54) {
    const u32 o = arg<u32>(ctx, 0);
    Stack frame(ctx, 16);
    const u32 l = frame.addr();
    store<u32>(l, arg<u32>(ctx, 1));
    u32 key = arg<u32>(ctx, 1);
    const u32 m = load<u32>(o + 8);
    if (m != 0) {
        const u32 r = static_cast<u32>(call(ctx, 0x0075E570, {m, l, l}));
        const u32 e = load<u32>(r);
        key = load<u32>(l);
        if (e != 0) {
            ctx.gpr[3] = e;
            return;
        }
    }
    const u32 p = load<u32>(o);
    ctx.gpr[3] = 0;
    if (p == 0) return;
    ctx.gpr[3] = call(ctx, 0x00760C54, {p, key});
}

// void f(T* o, a, b, const Mtx* m, f32 f1..f6, u8 g (the 11th argument)):
// +0 = a, +4 = b, the 3x3 part of m's four 16-byte rows at +16, +80.. =
// f1..f6, the u8 +104 = g (in the original's store order)
DECOMP(00767F08, Obj_SetMtx_00767F08) {
    const u32 o = arg<u32>(ctx, 0), a = arg<u32>(ctx, 1), b = arg<u32>(ctx, 2), m = arg<u32>(ctx, 3);
    const u32 g = load<u32>(static_cast<u32>(ctx.gpr[1]) + 132);
    Cp(o + 16, m);
    Cp(o + 20, m + 4);
    Cp(o + 24, m + 8);
    Cp(o + 32, m + 16);
    Cp(o + 36, m + 20);
    Cp(o + 40, m + 24);
    const u32 x = load<u32>(m + 32);
    store<u32>(o, a);
    store<u32>(o + 48, x);
    const u32 y = load<u32>(m + 36);
    store<u32>(o + 4, b);
    store<u32>(o + 52, y);
    Cp(o + 56, m + 40);
    Cp(o + 64, m + 48);
    Cp(o + 68, m + 52);
    const u32 z = load<u32>(m + 56);
    store<f32>(o + 80, static_cast<f32>(farg(ctx, 0)));
    store<u32>(o + 72, z);
    for (u32 k = 1; k < 6; ++k) store<f32>(o + 80 + 4 * k, static_cast<f32>(farg(ctx, static_cast<int>(k))));
    store<u8>(o + 104, static_cast<u8>(g));
    ctx.gpr[7] = g;
    ctx.fpr[7] = static_cast<f64>(std::bit_cast<f32>(z));
}

// ---------------------------------------------------------------- sweep: 36 instructions
// u32 f(T* o, f32 t, Ref* x): when t is above the constant at 0x00027DEC:
// q->vfn[352](q, -, x->+4, 0) with q = +48 when that has bit 0x2 in +192,
// else null (r4 holds the OPD there). Quirk: the null case still reads
// the vtable at address 0 and calls through it.
DECOMP(00027DF0, Obj_Notify352_00027DF0) {
    const u32 o = arg<u32>(ctx, 0), x = arg<u32>(ctx, 2);
    ctx.gpr[4] = x;
    ctx.fpr[2] = LfsAt(0x00027DEC);
    if (!(farg(ctx, 0) > ctx.fpr[2])) return;
    u32 q = load<u32>(o + 48);
    if (q != 0 && (load<u32>(q + 192) & 0x2u) == 0) q = 0;
    const u32 vt = load<u32>(q);
    const u32 a = load<u32>(x + 4);
    const u32 opd = load<u32>(vt + 352);
    call_ptr(ctx, opd, {q, opd, a, 0, load<u32>(opd)});
}

// u32 f(_, T* o): releases +68 (vfn[8]) and clears it; with an effect at
// +80: Effect_SetHash4CF988(it, 0x10CBB), its +136 |= 0x200 and +80 = 0
DECOMP(0003F444, Obj_DropEffect80_0003F444) {
    const u32 o = arg<u32>(ctx, 1);
    const u32 p = load<u32>(o + 68);
    if (p != 0) {
        VCall(ctx, p, 8, {p});
        store<u32>(o + 68, 0);
    }
    const u32 e = load<u32>(o + 80);
    ctx.gpr[3] = e;
    if (e == 0) return;
    call(ctx, 0x004CF988, {e, 0x10CBB});
    const u32 q = load<u32>(o + 80);
    const u32 f = load<u32>(q + 136) | 0x200u;
    store<u32>(q + 136, f);
    store<u32>(o + 80, 0);
    ctx.gpr[3] = q;
    ctx.gpr[4] = 0;
    ctx.gpr[5] = f;
}

// u32 f(T* o, a): with c = Effect_FindComponent(a, +12) (when +12 is set):
// when c->vfn[32](c) is 2, c->+20 = bit 0 of the u8 world->+44
DECOMP(0007B38C, Obj_SyncFlag44_0007B38C) {
    const u32 o = arg<u32>(ctx, 0), a = arg<u32>(ctx, 1);
    const u32 h = load<u32>(o + 12);
    ctx.gpr[3] = h;
    if (h == 0) return;
    const u32 c = static_cast<u32>(call(ctx, 0x004BB4B4, {a, h}));
    if (c == 0) return;
    if (static_cast<s32>(VCall(ctx, c, 32, {c})) != 2) return;
    const u32 v = load<u8>(load<u32>(kWorldPtr) + 44) & 1u;
    store<u32>(c + 20, v);
    ctx.gpr[3] = v;
}

// void f(T* o): when +12 and +16 are set, pushes a new VtB20DA0 (28 bytes)
// made from (+12, +16)
DECOMP(0009BDB8, Cmd_PushB20DA0_0009BDB8) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 a = load<u32>(o + 12);
    ctx.gpr[3] = a;
    if (a == 0) return;
    const u32 b = load<u32>(o + 16);
    ctx.gpr[3] = b;
    if (b == 0) return;
    PushNew26(ctx, 28, [&](u32 p) { call(ctx, 0x0042DDDC, {p, load<u32>(o + 12), load<u32>(o + 16)}); });
}
// void f(T* o): pushes a new 0x0042E618 (24 bytes) made from (+12, the u8
// +16 .. +21)
DECOMP(0009C788, Cmd_Push42E618_0009C788) {
    const u32 o = arg<u32>(ctx, 0);
    PushNew26(ctx, 24, [&](u32 p) {
        call(ctx, 0x0042E618, {p, load<u32>(o + 12), load<u8>(o + 16), load<u8>(o + 17), load<u8>(o + 18),
                               load<u8>(o + 19), load<u8>(o + 20), load<u8>(o + 21)});
    });
}
// void f(T* o): pushes a new 0x0042F144 (32 bytes) made from (f32 +12, +16,
// +20; s32 +24, the u8 +28, +29); r4..r6 (the float slots) as SmallAlloc
// leaves them
DECOMP(0009D484, Cmd_Push42F144_0009D484) {
    const u32 o = arg<u32>(ctx, 0);
    PushNew26(ctx, 32, [&](u32 p) {
        callf(ctx, 0x0042F144, {p, ctx.gpr[4], ctx.gpr[5], ctx.gpr[6], Sx(load<u32>(o + 24)), load<u8>(o + 28), load<u8>(o + 29)},
              {LfsAt(o + 12), LfsAt(o + 16), LfsAt(o + 20)});
    });
}
// void f(T* o): pushes a new 0x00433938 (156 bytes) made from (o + 12, o +
// 140, o + 148, the u8 +156)
DECOMP(000A1B98, Cmd_Push433938_000A1B98) {
    const u32 o = arg<u32>(ctx, 0);
    PushNew26(ctx, 156, [&](u32 p) { call(ctx, 0x00433938, {p, o + 12, o + 140, o + 148, load<u8>(o + 156)}); });
}
// void f(T* o): pushes a new 0x004340D8 (28 bytes) made from (o + 12, o +
// 16, f32 +20, the u8 +25, the u8 +24); r6 (the float slot) as SmallAlloc
// leaves it
DECOMP(000A23F0, Cmd_Push4340D8_000A23F0) {
    const u32 o = arg<u32>(ctx, 0);
    PushNew26(ctx, 28, [&](u32 p) {
        callf(ctx, 0x004340D8, {p, o + 12, o + 16, ctx.gpr[6], load<u8>(o + 25), load<u8>(o + 24)}, {LfsAt(o + 20)});
    });
}

// T::~T deleting form thunk for the base at +384 (vtable 0x00B13528)
DECOMP(000F4FCC, VtB13528_DeleteThunk384) { VtB13528DeleteAt(ctx, arg<u32>(ctx, 0) - 384); }
// T::~T deleting form thunk for the base at +380 (vtable 0x00B13528)
DECOMP(000F505C, VtB13528_DeleteThunk380) { VtB13528DeleteAt(ctx, arg<u32>(ctx, 0) - 380); }

// void f(Vec3* out, T* o): the vector +592 of o->vfn[64](o), or zero
DECOMP(0012905C, Obj_GetBodyVec_0012905C) {
    constexpr u32 kV = 0x01164B40;
    const u32 out = arg<u32>(ctx, 0), o = arg<u32>(ctx, 1);
    const u32 r = static_cast<u32>(VCall(ctx, o, 64, {o}));
    u32 s = r + 592;
    if (r == 0) {
        s = kV;
        ctx.gpr[3] = 0x01160000;
        ctx.gpr[4] = kV;
    }
    Cp(out, s);
    Cp(out + 4, s + 4);
    ctx.fpr[1] = static_cast<f64>(std::bit_cast<f32>(Cp(out + 8, s + 8)));
}

// u32 f(T* o, D* d): Obj_CopyQuery_000CDB28(o, d), then d->+144 = the
// vector +192, d->+160 = the vector +208, d->+176 / +180 = +224 / +228 and
// the u8 d->+188 = the u8 +232 when that is set
DECOMP(001BB920, Obj_CopyQuery_001BB920) {
    const u32 o = arg<u32>(ctx, 0), d = arg<u32>(ctx, 1);
    call(ctx, 0x000CDB28, {o, d});
    Cp(d + 144, o + 192);
    Cp(d + 148, o + 196);
    Cp(d + 152, o + 200);
    Cp(d + 160, o + 208);
    const u32 y = load<u32>(o + 212);
    const u32 a = load<u32>(o + 224);
    store<u32>(d + 164, y);
    const u32 z = load<u32>(o + 216);
    const u32 b = load<u32>(o + 228);
    store<u32>(d + 168, z);
    store<u32>(d + 176, a);
    store<u32>(d + 180, b);
    const u32 c = load<u8>(o + 232);
    if (c != 0) store<u8>(d + 188, static_cast<u8>(c));
    ctx.fpr[1] = static_cast<f64>(std::bit_cast<f32>(z));
    ctx.gpr[3] = c;
    ctx.gpr[4] = d;
    ctx.gpr[5] = b;
}

// u32 f(T* o, Event* e): event 0x59676D2A sets bit 0x20 of the u8 +572,
// 0x4DA1B5C3 clears it; then Obj_OnEvent_000E5E44(o, e)
DECOMP(001BF850, Obj_OnEvent_001BF850) {
    const u32 o = arg<u32>(ctx, 0), e = arg<u32>(ctx, 1);
    const s32 h = load<s32>(e + 8);
    if (h > 0x4DA1B5C3) {
        if (h == 0x59676D2A) store<u8>(o + 572, static_cast<u8>(load<u8>(o + 572) | 0x20u));
    } else if (h > 0x4DA1B5C2) {
        store<u8>(o + 572, static_cast<u8>(load<u8>(o + 572) & ~0x20u));
    }
    ctx.gpr[3] = call(ctx, 0x000E5E44, {o, e});
}

// bool f(T* o, f32 a, f32 b): 1 when the u8 +569 is set, +552 has bit
// 0x20000000, or b is at least the constant at 0x001D4B8C and a below b;
// otherwise o->vfn[420](o, 1, 0) and 0
DECOMP(001D4B90, Obj_CheckRange_001D4B90) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 vt = load<u32>(o);
    const f64 a = farg(ctx, 0), b = farg(ctx, 1);
    if (load<u8>(o + 569) != 0 || (load<u32>(o + 552) & 0x20000000u) != 0) {
        ctx.gpr[3] = 1;
        return;
    }
    const f64 k = LfsAt(0x001D4B8C);
    ctx.fpr[3] = k;
    if (b < k || a >= b) {
        VCallVt(ctx, vt, 420, {o, 1, 0});
        ctx.gpr[3] = 0;
        return;
    }
    ctx.gpr[3] = 1;
}

// bool f(T* o, const Vec3* pt): for the state +428 0 or 3,
// 0x0020FD30(o, pt); else Box_ContainsPoint(o->vfn[160](&box, o, 1), pt)
DECOMP(00214570, Obj_Contains_00214570) {
    const u32 o = arg<u32>(ctx, 0), pt = arg<u32>(ctx, 1);
    const s32 s = load<s32>(o + 428);
    if (s == 0 || s == 3) {
        ctx.gpr[3] = call(ctx, 0x0020FD30, {o, pt});
        return;
    }
    Stack frame(ctx, 48);
    VCall(ctx, o, 160, {frame.addr(), o, 1});
    ctx.gpr[3] = call(ctx, 0x005573D0, {frame.addr(), pt});
}

// void f(T* o, Area* a): clears the bit of a's index in the u16 +352
// entries at +344 from the mask +448 (none when absent), then
// Area_Leave(o, a)
DECOMP(0021B7F4, Obj_LeaveArea_0021B7F4) {
    const u32 o = arg<u32>(ctx, 0), a = arg<u32>(ctx, 1);
    const s32 n = load<u16>(o + 352);
    const u32 arr = load<u32>(o + 344);
    const u32 m = load<u32>(o + 448);
    s32 i = -1;
    for (s32 k = 0; k < n; ++k)
        if (load<u32>(arr + 4u * static_cast<u32>(k)) == a) {
            i = k;
            break;
        }
    const u32 sh = static_cast<u32>(i) & 0x3Fu;
    const u32 bit = sh >= 32 ? 0u : (1u << sh);
    store<u32>(o + 448, m & ~bit);
    ctx.gpr[3] = call(ctx, 0x0020EBC8, {o, a});
}

// T::~T: when +780 is set, vtable 0x00B1B9B8, AnimSkel_Release(it) and
// +780 = 0; when +736 is set, vtable 0x00B1B4C8 and Mem_FreeIfSet(+736,
// +732); then the 0x00B11D98 base destructor
DECOMP(0023E3E0, VtB1B9B8_Dtor) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 p = load<u32>(o + 780);
    if (p != 0) {
        store<u32>(o, 0x00B1B9B8);
        call(ctx, o26fn::AnimSkel_Release, {p});
        store<u32>(o + 780, 0);
    }
    const u32 q = load<u32>(o + 736);
    if (q != 0) {
        const u32 a = load<u32>(o + 732);
        store<u32>(o, 0x00B1B4C8);
        call(ctx, o26fn::Mem_FreeIfSet, {q, a});
    }
    store<u32>(o, 0x00B11D98);
    call(ctx, o26fn::Obj_BaseDtor_0C2DA0, {o});
}

// void f(T* o): PtrVector_Clear(0x0106882C), Entries_Free2EE200(), the
// global 0x00F64C38 = 0, ~PtrVector(+36), Mem_FreeFrom(+28, 0) and +28 = 0
// when set
DECOMP(002EE3C8, Obj_Shutdown_002EE3C8) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 r = static_cast<u32>(call(ctx, 0x0054EC1C, {0x0106882C}));
    call(ctx, 0x002EE200, {r});
    store<u32>(0x00F64C38, 0);
    VecDtor(ctx, o + 36);
    const u32 p = load<u32>(o + 28);
    ctx.gpr[3] = p;
    if (p == 0) return;
    call(ctx, o25fn::Mem_FreeFrom, {p, 0});
    store<u32>(o + 28, 0);
    ctx.gpr[3] = 0;
}

// void f(T* o): p->vfn[20](p) for each p in the list from +2576 linked by
// +2564
DECOMP(00327440, Obj_ForEachLinked_00327440) {
    const u32 o = arg<u32>(ctx, 0);
    ctx.gpr[3] = o;
    for (u32 p = load<u32>(o + 2576); p != 0; p = load<u32>(p + 2564)) VCall(ctx, p, 20, {p});
}

// void f(T* o): World_SetChild1281F4(o->+8, a new 0x003B04F8 (480 bytes,
// SmallAlloc) made from (+8, 0x7719A548, o), or 0)
DECOMP(0032A440, Obj_NewChild_0032A440) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 w = load<u32>(o + 8);
    const u32 p = static_cast<u32>(call(ctx, o19fn::SmallAlloc, {480}));
    u32 r = 0;
    if (p != 0) {
        call(ctx, 0x003B04F8, {p, load<u32>(o + 8), 0x7719A548, o});
        r = p;
    }
    ctx.gpr[3] = call(ctx, 0x0047C530, {w, r});
}

// Obj* f(a, s32 key, Obj* n): depth-first search below n (children +272,
// siblings +268) for the object whose +204 is key; 0 when none
DECOMP(0034F600, Obj_FindDescendant_0034F600) {
    const u32 a = arg<u32>(ctx, 0), key = arg<u32>(ctx, 1), n = arg<u32>(ctx, 2);
    ctx.gpr[3] = 0;
    if (n == 0) return;
    for (u32 c = load<u32>(n + 272); c != 0; c = load<u32>(c + 268)) {
        if (load<u32>(c + 204) == key) {
            ctx.gpr[3] = c;
            return;
        }
        const u32 r = static_cast<u32>(call(ctx, 0x0034F600, {a, key, c}));
        if (r != 0) {
            ctx.gpr[3] = r;
            return;
        }
    }
    ctx.gpr[3] = 0;
}

// void f(T* o, u32 key, _, u8 both, f32 v): in the 224-byte entries of the
// list +96 (u16 count +8), the f32 +204 of the one keyed key = v, and its
// +200 too when both
DECOMP(00352EDC, Obj_SetEntryF_00352EDC) {
    const u32 o = arg<u32>(ctx, 0), key = arg<u32>(ctx, 1), both = arg<u32>(ctx, 3) & 0xFFu;
    const f32 v = static_cast<f32>(farg(ctx, 0));
    const u32 l = load<u32>(o + 96);
    u32 i = 0;
    for (;;) {
        const u32 n = l != 0 ? load<u16>(l + 8) : 0;
        if (!(i < n)) {
            ctx.gpr[3] = i;
            return;
        }
        const u32 arr = load<u32>(l);
        if (load<u32>(arr + 224 * i) == key) {
            store<f32>(arr + 224 * i + 204, v);
            ctx.gpr[3] = 224 * i;
            if (both != 0) {
                const u32 e = load<u32>(load<u32>(o + 96)) + 224 * i;
                store<f32>(e + 200, v);
                ctx.gpr[3] = e;
            }
            return;
        }
        ++i;
    }
}
