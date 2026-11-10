// Game code, part 27: the continued sweep of small still-recompiled game
// functions (in size order, from 36 instructions up). Most have no known
// caller context yet, so their names describe what they do and carry the
// address. Each was checked against the original in the PPC simulator.
//
// Register notes: where a known caller reads a volatile register the
// original leaves behind (regflow liveout), the value is set the same way,
// as the comments say ("r4 left ...").
#include "obj27.h"

using namespace sly4;
using namespace sly4::obj27;

// ---------------------------------------------------------------- sweep: 36 instructions
// u32 f(T*, u8 on): when the global 0x00F64CBC = g is set, g->+24 has bit
// 0x8000 at +164 and g->+20 = e is set, Effect_SetHash(e, on ? 0xDCDC6875 :
// 0x019A6705); else returns r4 as passed
DECOMP(003697E0, Effect_SetOnOffHash_003697E0) {
    const u32 on = arg<u32>(ctx, 1);
    ctx.gpr[3] = on;
    const u32 g = load<u32>(0x00F64CBC);
    if (g == 0) return;
    const u32 p = load<u32>(g + 24);
    if (p == 0 || (load<u32>(p + 164) & 0x8000u) == 0) return;
    const u32 e = load<u32>(g + 20);
    if (e == 0) return;
    ctx.gpr[3] = call(ctx, o27fn::Effect_SetHash, {e, (on & 0xFFu) != 0 ? 0xDCDC6875u : 0x019A6705u});
}

// void f(u32 v): pushes v onto the global u32 array 0x010AA6A8 (+0 data,
// +4 u16 capacity, +8 u16 count, +10 u16 high-water mark), doubling the
// capacity with PtrVector_Reserve when full. r3 = the old mark.
DECOMP(0042BC44, List_Push010AA6A8) {
    constexpr u32 kList = 0x010AA6A8;
    const u32 v = arg<u32>(ctx, 0);
    const u32 cap = load<u16>(kList + 4);
    if (load<u16>(kList + 8) == cap) call(ctx, o27fn::PtrVector_Reserve, {kList, (cap << 1) & 0x1FFFEu});
    const u32 c = load<u16>(kList + 8);
    store<u32>(load<u32>(kList) + ((c << 2) & 0x3FFFCu), v);
    const u32 n = load<u16>(kList + 8) + 1;
    const u32 m = load<u16>(kList + 10);
    store<u16>(kList + 8, static_cast<u16>(n));
    store<u16>(kList + 10, static_cast<u16>(static_cast<s32>(m) > static_cast<s32>(n & 0xFFFF) ? m : n));
    ctx.gpr[3] = m;
}

// T::T(u32 a, u8 b, u8 c, u8 d, u8 e, bool on): base 0x0042C9D0(o, on),
// +12 = a, the bytes +16..+19 = b..e, vtable 0x00B20E30 (r3 = the vtable)
DECOMP(0042E868, VtB20E30_Ctor) {
    const u32 o = arg<u32>(ctx, 0), a = arg<u32>(ctx, 1);
    const u32 b = arg<u32>(ctx, 2), c = arg<u32>(ctx, 3), d = arg<u32>(ctx, 4), e = arg<u32>(ctx, 5);
    CmdBase27(ctx, o, arg<u32>(ctx, 6));
    store<u32>(o + 12, a);
    store<u8>(o + 16, static_cast<u8>(b));
    store<u8>(o + 17, static_cast<u8>(c));
    store<u8>(o + 18, static_cast<u8>(d));
    store<u32>(o, 0x00B20E30);
    store<u8>(o + 19, static_cast<u8>(e));
    ctx.gpr[3] = 0x00B20E30;
}

// void f(T* o): when +12 is set, finds it with 0x0042C2B0(+12, 0) and
// calls 0x000DB180(found, the u8 +16) (or warns with the format 0x00AE2400
// and the hash name); then Flag_Set8_40000000(o)
DECOMP(0042EC20, Cmd_ApplyByHandle_0042EC20) {
    const u32 o = arg<u32>(ctx, 0);
    if (load<u32>(o + 12) != 0) {
        const u32 r = static_cast<u32>(call(ctx, 0x0042C2B0, {load<u32>(o + 12), 0}));
        if (r != 0) call(ctx, 0x000DB180, {r, load<u8>(o + 16)});
        else WarnHash27(ctx, 0x00AE2400, load<u32>(o + 12));
    }
    ctx.gpr[3] = call(ctx, o27fn::Flag_Set8_40000000, {o});
}

// bool f(T* o): true when the sound +28 is unset or done (0x004431A0);
// false while the sound's u8 +97 is set; otherwise warns (format
// 0x00AE2448, 0x004440E8(+12)) and returns true
DECOMP(0042F0B4, Cmd_IsSoundDone_0042F0B4) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 s = load<u32>(o + 28);
    if (s != 0 && static_cast<s32>(call(ctx, 0x004431A0, {s})) == 0) {
        if (load<u8>(load<u32>(o + 28) + 97) != 0) {
            ctx.gpr[3] = 0;
            return;
        }
        const u32 n = static_cast<u32>(call(ctx, 0x004440E8, {load<u32>(o + 12)}));
        call(ctx, o27fn::String_Format, {0x00AE2448, n});
    }
    ctx.gpr[3] = 1;
}

// T::T(f32 x, f32 y, f32 z, ..., u32 a (r7), u8 b (r8), bool on (r9)):
// base 0x0042C9D0(o, on), +12..+20 = x, y, z, +24 = a, vtable 0x00B20EA8,
// the u8 +28 = b (r3 = the vtable)
DECOMP(0042F144, VtB20EA8_Ctor) {
    const u32 o = arg<u32>(ctx, 0), a = arg<u32>(ctx, 4), b = arg<u32>(ctx, 5);
    const f64 x = farg(ctx, 0), y = farg(ctx, 1), z = farg(ctx, 2);
    CmdBase27(ctx, o, arg<u32>(ctx, 6));
    store<f32>(o + 12, static_cast<f32>(x));
    store<f32>(o + 16, static_cast<f32>(y));
    store<f32>(o + 20, static_cast<f32>(z));
    store<u32>(o + 24, a);
    store<u32>(o, 0x00B20EA8);
    store<u8>(o + 28, static_cast<u8>(b));
    ctx.gpr[3] = 0x00B20EA8;
}

// T::T(f32 x, f32 y, f32 z, f32 w, ..., u8 b (r8), bool on (r9)): base
// 0x0042C9D0(o, on), +12..+24 = x, y, z, w, vtable 0x00B20EC0, the u8 +28
// = b (r3 = the vtable)
DECOMP(0042F264, VtB20EC0_Ctor) {
    const u32 o = arg<u32>(ctx, 0), b = arg<u32>(ctx, 5);
    const f64 x = farg(ctx, 0), y = farg(ctx, 1), z = farg(ctx, 2), w = farg(ctx, 3);
    CmdBase27(ctx, o, arg<u32>(ctx, 6));
    store<f32>(o + 12, static_cast<f32>(x));
    store<f32>(o + 16, static_cast<f32>(y));
    store<f32>(o + 20, static_cast<f32>(z));
    store<f32>(o + 24, static_cast<f32>(w));
    store<u32>(o, 0x00B20EC0);
    store<u8>(o + 28, static_cast<u8>(b));
    ctx.gpr[3] = 0x00B20EC0;
}

// T::T(u32 a, u32 b, u32 c, u32 d, u8 e): base 0x0042C9D0(o, 1), +12..+24
// = a..d, vtable 0x00B20EF0, the u8 +28 = e (r3 = the vtable)
DECOMP(0042F588, VtB20EF0_Ctor) {
    const u32 o = arg<u32>(ctx, 0), a = arg<u32>(ctx, 1), b = arg<u32>(ctx, 2);
    const u32 c = arg<u32>(ctx, 3), d = arg<u32>(ctx, 4), e = arg<u32>(ctx, 5);
    CmdBase27(ctx, o, 1);
    store<u32>(o + 12, a);
    store<u32>(o + 16, b);
    store<u32>(o + 20, c);
    store<u32>(o + 24, d);
    store<u32>(o, 0x00B20EF0);
    store<u8>(o + 28, static_cast<u8>(e));
    ctx.gpr[3] = 0x00B20EF0;
}

// bool f(T* o): whether the handle o->+128 (low 16 bits a serial, high 16
// an index) is dead in the handle table 0x00F64C38->* (+24 count, +28
// 16-byte entries: +8 the object, +12 bit 0x80000000 free and bits 15..30
// the serial): 1 when the serial is 0, the index is out of range, the
// entry's serial differs, it is free or its object is unset; else 0
DECOMP(0042F990, Handle_IsDead_0042F990) {
    const u32 h = load<u32>(arg<u32>(ctx, 0) + 128);
    const u32 t = load<u32>(0x00F64C38);
    const u32 serial = h & 0xFFFFu, idx = h >> 16;
    u32 dead = 1;
    if (serial != 0 && static_cast<s32>(idx) < static_cast<s32>(load<u32>(t + 24))) {
        const u32 e = load<u32>(t + 28) + (idx << 4);
        const u32 w = load<u32>(e + 12);
        if (((w >> 15) & 0xFFFFu) == serial && (w & 0x80000000u) == 0 && load<u32>(e + 8) != 0) dead = 0;
    }
    ctx.gpr[3] = dead;
}

// Node* f(T* o): the cached +40, else the keys o->+20[0..u16 +28) passed
// in turn to Node_FindByKey_004D54FC(key, previous result), each result
// stored at +40 (r4 left = the result)
DECOMP(004BFDDC, Node_FindCached_004BFDDC) {
    const u32 o = arg<u32>(ctx, 0);
    u32 r = load<u32>(o + 40);
    if (r == 0) {
        for (u32 i = 0; i < load<u16>(o + 28); ++i) {
            r = static_cast<u32>(call(ctx, 0x004D54FC, {load<u32>(load<u32>(o + 20) + 4 * i), r}));
            store<u32>(o + 40, r);
        }
    }
    ctx.gpr[4] = r;
    ctx.gpr[3] = r;
}

// f32 f(T* o): when the f32 +40 is above +36, +28 = a random value in
// [+36, +40) from the game's generator (the old low 16 bits times the
// scale at 0x004C1198); returns +28
DECOMP(004C11A0, Range_PickRandom_004C11A0) {
    const u32 o = arg<u32>(ctx, 0);
    const f64 hi = static_cast<f64>(load<f32>(o + 40)), lo = static_cast<f64>(load<f32>(o + 36));
    if (hi > lo) {
        const f32 span = static_cast<f32>(hi - lo);
        const s64 last = load<s64>(update::kRandomLast);
        update::Random_Step();
        const f32 r = static_cast<f32>(static_cast<f64>(static_cast<f32>(static_cast<f64>(last))) * static_cast<f64>(load<f32>(0x004C1198)));
        const f32 v = static_cast<f32>(std::fma(static_cast<f64>(span), static_cast<f64>(r), lo));
        store<f32>(o + 28, v);
        ctx.fpr[1] = static_cast<f64>(v);
        return;
    }
    const u32 w = load<u32>(o + 28);
    store<u32>(o + 28, w);
    ctx.fpr[1] = static_cast<f64>(std::bit_cast<f32>(w));
}

// void f(T* o): releases +272 (0x007B4C20) and clears it, +276 = 2, clears
// (o->+304)->+160 when set, then o->vfn[4](o)
DECOMP(004D23CC, Obj_Reset_004D23CC) {
    const u32 o = arg<u32>(ctx, 0);
    if (load<u32>(o + 272) != 0) {
        call(ctx, 0x007B4C20, {load<u32>(o + 272)});
        store<u32>(o + 272, 0);
    }
    const u32 q = load<u32>(o + 304);
    store<u32>(o + 276, 2);
    if (load<u32>(q + 160) != 0) store<u32>(q + 160, 0);
    ctx.gpr[3] = q;
    if (o != 0) VCall(ctx, o, 4, {o});
}

// u32 f(XmlCursor* c, const char* name): the number of children of c
// named name (any child when name is null)
DECOMP(004F6234, Xml_CountChildren_004F6234) {
    const u32 c = arg<u32>(ctx, 0), name = arg<u32>(ctx, 1);
    u32 r = name != 0 ? static_cast<u32>(call(ctx, 0x004F936C, {c, name})) : static_cast<u32>(call(ctx, 0x004F9140, {c, 0}));
    u32 n = 0;
    while (r != 0) {
        ++n;
        r = name != 0 ? static_cast<u32>(call(ctx, 0x004F9078, {r, name})) : static_cast<u32>(call(ctx, 0x004F8D74, {r}));
    }
    ctx.gpr[3] = n;
}

// u32 f(T* o, U* p): when p->+8 is set, o->vfn[12](o, p) then
// o->vfn[8](o, p); else 0
DECOMP(00746C1C, Obj_VCall12Then8_00746C1C) {
    const u32 o = arg<u32>(ctx, 0), p = arg<u32>(ctx, 1);
    if (load<u32>(p + 8) == 0) {
        ctx.gpr[3] = 0;
        return;
    }
    VCall(ctx, o, 12, {o, p});
    ctx.gpr[3] = VCall(ctx, o, 8, {o, p});
}

// T::~T deleting form (vtable 0x00B123D0, see VtB123D0DtorAt), then
// SmallFree(o, 384). Null does nothing (r3 = 0).
DECOMP(007471F8, VtB123D0_Delete) {
    const u32 o = arg<u32>(ctx, 0);
    if (o == 0) {
        ctx.gpr[3] = 0;
        return;
    }
    VtB123D0DtorAt(ctx, o);
    call(ctx, o19fn::SmallFree, {o, 384});
}

// u32 f(u32 key, u8 warn): the value of key in the map 0x0105752C
// (SortedMap_Find); a null value for a nonzero key warns (format
// 0x00B142F0 and the hash name) when warn is set
DECOMP(0074CBE4, Map0105752C_Get) {
    Stack frame(ctx, 144);
    const u32 slot = frame.addr() + 112;
    store<u32>(slot, arg<u32>(ctx, 0));
    const u32 warn = arg<u32>(ctx, 1);
    const u32 r = static_cast<u32>(call(ctx, o27fn::SortedMap_Find, {0x0105752C, slot}));
    const u32 v = load<u32>(r);
    if ((warn & 0xFFu) != 0 && load<u32>(slot) != 0 && v == 0) WarnHash27(ctx, 0x00B142F0, load<u32>(slot));
    ctx.gpr[3] = v;
}

// f32 f(): the f32 0x011DE098 of the singleton 0x011DE094, first set up
// (+20 / +24 = 0, bits 0xC0 of the u8 +32 cleared, 400 bytes at +92
// zeroed, 0x011972E8 = it, the flag 0x011DE280 = 1). r3 = 0x011DE094.
DECOMP(0074D778, Singleton011DE094_Get) {
    constexpr u32 kObj = 0x011DE094;
    if (load<u8>(0x011DE280) == 0) {
        const u8 b = load<u8>(kObj + 32);
        store<u32>(kObj + 20, 0);
        store<u32>(kObj + 24, 0);
        store<u8>(kObj + 32, static_cast<u8>(b & 0x3Fu));
        call(ctx, 0x00726E54, {kObj + 92, 0, 400});
        store<u32>(0x011972E8, kObj);
        store<u8>(0x011DE280, 1);
    }
    ctx.gpr[3] = kObj;
    ctx.fpr[1] = static_cast<f64>(load<f32>(kObj + 4));
}

// void f(Buf* b): +4 = 0; unless +8 has bit 0x80000000 (not owned), frees
// +0 through the Havok allocator 0x011DF31C (vfn[20](alloc, p, +8 << 6));
// then +0 = 0, +8 = 0x80000000
DECOMP(00760438, HkArray_Free_00760438) {
    constexpr u32 kHkAlloc = 0x011DF31C;
    const u32 b = arg<u32>(ctx, 0);
    const u32 n = load<u32>(b + 8), p = load<u32>(b);
    store<u32>(b + 4, 0);
    if ((n & 0x80000000u) == 0) VCall(ctx, kHkAlloc, 20, {kHkAlloc, p, Sx(n << 6)});
    store<u32>(b, 0);
    store<u32>(b + 8, 0x80000000u);
    ctx.gpr[3] = 0;
    ctx.gpr[4] = 0x80000000u;
}

// T::~T deleting form: vtable 0x00B21160, ~PtrVector(+80), 0x004A0E08(+44),
// 0x0042CA50(o), SmallFree(o, 100). Null does nothing (r3 = 0).
DECOMP(00780BD0, VtB21160_Delete) {
    const u32 o = arg<u32>(ctx, 0);
    if (o == 0) {
        ctx.gpr[3] = 0;
        return;
    }
    store<u32>(o, 0x00B21160);
    VecDtor(ctx, o + 80);
    call(ctx, 0x004A0E08, {o + 44});
    call(ctx, 0x0042CA50, {o});
    call(ctx, o19fn::SmallFree, {o, 100});
}

// ---------------------------------------------------------------- sweep: 37 instructions
// T::~T (vtable 0x00B0DDC0): o->+24->vfn[12](it), then when +24 (read
// again) is set its vfn[4](it), then the base destructor 0x002505A0(o)
static void VtB0DDC0DtorBody(PPUContext& ctx) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 p = load<u32>(o + 24);
    store<u32>(o, 0x00B0DDC0);
    VCall(ctx, p, 12, {p});
    const u32 q = load<u32>(o + 24);
    if (q != 0) VCall(ctx, q, 4, {q});
    ctx.gpr[3] = call(ctx, 0x002505A0, {o});
}
// T::~T: the 0x00B0DDC0 body (one of two identical copies)
DECOMP(00017F84, VtB0DDC0_Dtor_00017F84) { VtB0DDC0DtorBody(ctx); }
// T::~T: the 0x00B0DDC0 body (the other copy)
DECOMP(000180C8, VtB0DDC0_Dtor_000180C8) { VtB0DDC0DtorBody(ctx); }

// void f(T* a, U* b): when the u8 a->+84 is set: when the u8 a->+88 is
// set and Effect_FindComponent(b, a->+80) finds c, 0x0049E8E4(a + 12, c);
// then when 0x00F64C20->*->+40 = w is set, 0x002D98DC(w, a + 12)
// (r3 = b when +84 is clear)
DECOMP(00089048, Obj_AttachEffect_00089048) {
    const u32 a = arg<u32>(ctx, 0), b = arg<u32>(ctx, 1);
    ctx.gpr[3] = b;
    if (load<u8>(a + 84) == 0) return;
    if (load<u8>(a + 88) != 0) {
        const u32 c = static_cast<u32>(call(ctx, 0x004BB4B4, {b, load<u32>(a + 80)}));
        if (c != 0) call(ctx, 0x0049E8E4, {a + 12, c});
    }
    const u32 w = load<u32>(load<u32>(0x00F64C20) + 40);
    ctx.gpr[3] = w;
    if (w != 0) call(ctx, 0x002D98DC, {w, a + 12});
}

// void f(T* o): pushes a new 100-byte 0x0043554C(p, +32, +36, f32 +40..+52,
// o + 12 in r10) command (r6..r9 as SmallAlloc leaves them)
DECOMP(000A3434, Obj_PushNew43554C_000A3434) {
    const u32 o = arg<u32>(ctx, 0);
    PushNew26(ctx, 100, [&](u32 p) {
        callf(ctx, 0x0043554C, {p, load<u32>(o + 32), load<u32>(o + 36), ctx.gpr[6], ctx.gpr[7], ctx.gpr[8], ctx.gpr[9], o + 12},
              {static_cast<f64>(load<f32>(o + 40)), static_cast<f64>(load<f32>(o + 44)), static_cast<f64>(load<f32>(o + 48)),
               static_cast<f64>(load<f32>(o + 52))});
    });
}

// void f(T* o): when +16 is set, pushes a new 36-byte VtB213E8 command
// (0x0043A84C(p, +12, +16, the u8s +20, +21, +22)); else r3 = 0
DECOMP(000A7158, Obj_PushNewB213E8_000A7158) {
    const u32 o = arg<u32>(ctx, 0);
    if (load<u32>(o + 16) == 0) {
        ctx.gpr[3] = 0;
        return;
    }
    PushNew26(ctx, 36, [&](u32 p) {
        call(ctx, 0x0043A84C, {p, load<u32>(o + 12), load<u32>(o + 16), load<u8>(o + 20), load<u8>(o + 21), load<u8>(o + 22)});
    });
}

// void f(?, ?, const T* src (r5), ?, T* dst (r7)): copies the bytes
// +24..+31, the words +32..+40, the u8 +44 and the words +48..+64, +72
// (r3 / r4 / f1 = the +56 / +64 / +72 values, as left)
DECOMP(000C387C, Rec_CopyFields_000C387C) {
    const u32 s = arg<u32>(ctx, 2), d = arg<u32>(ctx, 4);
    for (u32 k = 24; k < 29; ++k) store<u8>(d + k, load<u8>(s + k));
    const u32 w36 = load<u32>(s + 36);
    store<u8>(d + 29, load<u8>(s + 29));
    const u32 w32 = load<u32>(s + 32);
    store<u8>(d + 30, load<u8>(s + 30));
    store<u32>(d + 36, w36);
    store<u8>(d + 31, load<u8>(s + 31));
    store<u32>(d + 32, w32);
    store<u32>(d + 40, load<u32>(s + 40));
    store<u8>(d + 44, load<u8>(s + 44));
    const u32 w64 = load<u32>(s + 64);
    store<u32>(d + 48, load<u32>(s + 48));
    store<u32>(d + 52, load<u32>(s + 52));
    const u32 w60 = load<u32>(s + 60);
    store<u32>(d + 64, w64);
    const u32 w56 = load<u32>(s + 56);
    store<u32>(d + 60, w60);
    store<u32>(d + 56, w56);
    const u32 w72 = load<u32>(s + 72);
    store<u32>(d + 72, w72);
    ctx.gpr[3] = w56;
    ctx.gpr[4] = w64;
    ctx.fpr[1] = static_cast<f64>(std::bit_cast<f32>(w72));
}

// u32 f(T* o): when o->vfn[272](o) is nonzero, (o->vfn[272](o))->+4->+8
// (0 when the +4 is unset); else 0
DECOMP(000DA7C8, Obj_Get272Field8_000DA7C8) {
    const u32 o = arg<u32>(ctx, 0);
    u32 r = 0;
    if (static_cast<u32>(VCall(ctx, o, 272, {o})) != 0) {
        const u32 q = load<u32>(static_cast<u32>(VCall(ctx, o, 272, {o})) + 4);
        if (q != 0) r = load<u32>(q + 8);
    }
    ctx.gpr[3] = r;
}

// void f(T* o, Hit* h): when the f32 h->+60 is above the constant at
// 0x000F5E64, 0x000E1738(o, h, h->+4, 0x03C599D8, f1 = h->+60), then when
// o->+348 is no longer above it, 0x000E9BA0(o, 4, 0) (r3 = o otherwise)
DECOMP(000F5E68, Obj_OnHit_000F5E68) {
    const u32 o = arg<u32>(ctx, 0), h = arg<u32>(ctx, 1);
    const f64 k = static_cast<f64>(load<f32>(0x000F5E64));
    const f64 v = static_cast<f64>(load<f32>(h + 60));
    ctx.gpr[3] = o;
    ctx.fpr[1] = v;  // f1 left = h->+60
    if (!(v > k)) return;
    callf(ctx, 0x000E1738, {o, h, load<u32>(h + 4), 0x03C599D8}, {v});  // r3 = its result
    const f64 w = static_cast<f64>(load<f32>(o + 348));
    ctx.fpr[1] = w;  // f1 left = +348
    if (w <= k) call(ctx, 0x000E9BA0, {o, 4, 0});
}

// Node* f(?, Node* n, s32 key): n when n->+208->+100 is key, else the
// first match in its children (+272, chained by +268), searched the same
// way (recursively); else 0
DECOMP(001918B0, Node_FindByKey_001918B0) {
    const u32 c = arg<u32>(ctx, 0), n = arg<u32>(ctx, 1), key = arg<u32>(ctx, 2);
    if (load<u32>(load<u32>(n + 208) + 100) == key) {
        ctx.gpr[3] = n;
        return;
    }
    for (u32 k = load<u32>(n + 272); k != 0; k = load<u32>(k + 268)) {
        const u32 r = static_cast<u32>(call(ctx, 0x001918B0, {c, k, key}));
        if (r != 0) {
            ctx.gpr[3] = r;
            return;
        }
    }
    ctx.gpr[3] = 0;
}

// bool f(T* o, f32 a, f32 b): 1 when +552 has bit 0x20000000; when b is at
// least the constant at kmin and a at least b, o->vfn[420](o, 5, 0) and 0;
// else 1
static void TryState5(PPUContext& ctx, u32 kmin) {
    const u32 o = arg<u32>(ctx, 0);
    if ((load<u32>(o + 552) & 0x20000000u) != 0) {
        ctx.gpr[3] = 1;
        return;
    }
    const f64 a = farg(ctx, 0), b = farg(ctx, 1);
    if (b >= static_cast<f64>(load<f32>(kmin)) && a >= b) {
        VCall(ctx, o, 420, {o, 5, 0});
        ctx.gpr[3] = 0;
        return;
    }
    ctx.gpr[3] = 1;
}
// bool f(T* o, f32 a, f32 b): TryState5 with the constant at 0x001AF254
DECOMP(001AF258, Obj_TryState5_001AF258) { TryState5(ctx, 0x001AF254); }
// bool f(T* o, f32 a, f32 b): TryState5 with the constant at 0x001C9D9C
DECOMP(001C9DA0, Obj_TryState5_001C9DA0) { TryState5(ctx, 0x001C9D9C); }

// The body of the 0x00B174C0 deleting destructors: vtables 0x00B174C0 /
// 0x00B17694 (+380) / 0x00B176BC (+384), ~PtrVector(+556), the base
// destructor 0x001AE468, SmallFree(o, 976).
static void VtB174C0DeleteAt(PPUContext& ctx, u32 o) {
    store<u32>(o, 0x00B174C0);
    store<u32>(o + 380, 0x00B17694);
    store<u32>(o + 384, 0x00B176BC);
    VecDtor(ctx, o + 556);
    call(ctx, 0x001AE468, {o});
    call(ctx, o19fn::SmallFree, {o, 976});
}
// T::~T deleting form (VtB174C0DeleteAt). Null does nothing (r3 = 0).
DECOMP(001D1E0C, VtB174C0_Delete) {
    const u32 o = arg<u32>(ctx, 0);
    if (o == 0) {
        ctx.gpr[3] = 0;
        return;
    }
    VtB174C0DeleteAt(ctx, o);
}

// void f(T* o): 0x00464488(the 11968-byte record (s8 def->+64->+172) of
// 0x010B19E0, + 176, f32 0x010532C8, f32 0x01053318) where def =
// 0x000D951C(o), then Msg_Post(o, 0x55B67D7C, 0x39F6520D, 0)
DECOMP(001EFD4C, Obj_ApplyRecord_001EFD4C) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 d = static_cast<u32>(call(ctx, 0x000D951C, {o}));
    const s32 i = load<s8>(load<u32>(d + 64) + 172);
    callf(ctx, 0x00464488, {0x010B19E0u + static_cast<u32>(i * 11968) + 176},
          {static_cast<f64>(load<f32>(0x010532C8)), static_cast<f64>(load<f32>(0x01053318))});
    call(ctx, o24fn::Msg_Post, {o, 0x55B67D7C, 0x39F6520D, 0});
}

// f32 f(T* o): with a spawner +336, the f32 +224 of 0x000D155C(+336, o);
// else the u16 +510 of o->vfn[144](o) as the high half of an f32 (the
// constant at 0x001FB768 when that is null)
DECOMP(001FB770, Obj_GetScale_001FB770) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 s = load<u32>(o + 336);
    if (s != 0) {
        const u32 r = static_cast<u32>(call(ctx, 0x000D155C, {s, o}));
        ctx.fpr[1] = static_cast<f64>(load<f32>(r + 224));
        return;
    }
    f32 v = load<f32>(0x001FB768);
    const u32 r = static_cast<u32>(VCall(ctx, o, 144, {o}));
    if (r != 0) {
        const u32 w = static_cast<u32>(load<u16>(r + 510)) << 16;
        v = std::bit_cast<f32>(w);
        ctx.gpr[3] = w;
    }
    ctx.fpr[1] = static_cast<f64>(v);
}

// void f(T* o, Query* q): 0x000DFBFC(o, q), then q->+208.. = the fields
// o->+240.. (f32s +240 / +244 / +248 / +256..+264, words +272 / +276, the
// u8s +280..+282); r3 / r4 / r5 / f1 left as the original's last loads
DECOMP(00204AE0, Obj_CopyQuery_00204AE0) {
    const u32 o = arg<u32>(ctx, 0), q = arg<u32>(ctx, 1);
    call(ctx, 0x000DFBFC, {o, q});
    store<u32>(q + 208, load<u32>(o + 240));
    store<u32>(q + 212, load<u32>(o + 244));
    store<u32>(q + 216, load<u32>(o + 248));
    const u32 f256 = load<u32>(o + 256);
    const u8 b280 = load<u8>(o + 280);
    store<u32>(q + 224, f256);
    const u32 f260 = load<u32>(o + 260);
    store<u8>(q + 248, b280);
    store<u32>(q + 228, f260);
    const u32 f264 = load<u32>(o + 264);
    const u32 w272 = load<u32>(o + 272);
    const u8 b281 = load<u8>(o + 281);
    store<u32>(q + 232, f264);
    const u32 w276 = load<u32>(o + 276);
    store<u32>(q + 240, w272);
    store<u8>(q + 249, b281);
    store<u32>(q + 244, w276);
    const u8 b282 = load<u8>(o + 282);
    store<u8>(q + 250, b282);
    ctx.gpr[3] = b282;
    ctx.gpr[4] = b281;
    ctx.gpr[5] = w276;
    ctx.fpr[1] = static_cast<f64>(std::bit_cast<f32>(f264));
}

// u32 f(T* a, U* b, s32 kind, d): 0x000DF960(a, b, kind, d) when kind is 4,
// or kind is 0 and b is null or has bits 0x00C00000 of +172; else 0
DECOMP(00208AE8, Obj_Forward_00208AE8) {
    const u32 a = arg<u32>(ctx, 0), b = arg<u32>(ctx, 1), d = arg<u32>(ctx, 3);
    const s32 k = arg<s32>(ctx, 2);
    bool go = false;
    if (k > 0) go = k == 4;
    else if (k == 0) go = b == 0 || ((load<u32>(b + 172) >> 22) & 3u) != 0;
    if (!go) {
        ctx.gpr[3] = 0;
        return;
    }
    ctx.gpr[3] = call(ctx, 0x000DF960, {a, b, static_cast<u32>(k), d});
}

// u32 f(T* o, Area* p): Area_Enter(o, p) unless o->+428 is not -1,
// p->vfn[64](p) = r is set and bits 28..30 of r->+416 differ from +428
// (then 0)
DECOMP(0021DE08, Obj_EnterArea_0021DE08) {
    const u32 o = arg<u32>(ctx, 0), p = arg<u32>(ctx, 1);
    if (load<s32>(o + 428) != -1) {
        const u32 r = static_cast<u32>(VCall(ctx, p, 64, {p}));
        if (r != 0 && static_cast<s32>(load<u32>(r + 416) >> 28) != load<s32>(o + 428)) {
            ctx.gpr[3] = 0;
            return;
        }
    }
    ctx.gpr[3] = call(ctx, 0x0020E7A4, {o, p});
}

// u32 f(XmlCursor* c, u32* out): reads the hash 0xEC2CEB42; when it is one
// of the 3 words at 0x00AD38B0, *out = its index. Returns the hash.
DECOMP(00223CA0, Xml_ReadKind_00223CA0) {
    Stack frame(ctx, 128);
    const u32 slot = frame.addr() + 112;
    const u32 c = arg<u32>(ctx, 0), out = arg<u32>(ctx, 1);
    store<u32>(slot, 0);
    call(ctx, 0x004FA14C, {c, 0xEC2CEB42u, slot, 0});
    const u32 v = load<u32>(slot);
    ctx.gpr[3] = v;
    if (v == 0) return;
    for (u32 i = 0; i < 3; ++i) {
        if (load<u32>(0x00AD38B0 + 4 * i) == v) {
            store<u32>(out, i);
            return;
        }
    }
}

// T::~T deleting form: when +44 is set, vtable 0x00B1D628, Effect_Destruct
// (+44), SmallFree(+44, 140), +44 = 0; then vtable 0x00B1D600 and
// SmallFree(o, 48). Null does nothing (r3 = 0).
DECOMP(002F9A20, VtB1D628_Delete) {
    const u32 o = arg<u32>(ctx, 0);
    if (o == 0) {
        ctx.gpr[3] = 0;
        return;
    }
    const u32 e = load<u32>(o + 44);
    if (e != 0) {
        store<u32>(o, 0x00B1D628);
        call(ctx, 0x004C8E80, {e});
        call(ctx, o19fn::SmallFree, {e, 140});
        store<u32>(o + 44, 0);
    }
    store<u32>(o, 0x00B1D600);
    call(ctx, o19fn::SmallFree, {o, 48});
}

// bool f(T* o): unless the u8s +40 and +41 are both set, +41 = 1 and the
// first entry of (o->+4->+0)->+36[0..u16 +44) whose +36 is o->+44 gets its
// u8 +52 = 1. Returns 1.
DECOMP(002FE59C, Obj_MarkEntry_002FE59C) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 p = load<u32>(o + 4);
    if (load<u8>(o + 40) == 0 || load<u8>(o + 41) == 0) {
        store<u8>(o + 41, 1);
        const u32 t = load<u32>(p);
        const u32 key = load<u32>(o + 44);
        for (u32 i = 0; i < load<u16>(t + 44); ++i) {
            const u32 arr = load<u32>(t + 36);
            if (load<u32>(load<u32>(arr + 4 * i) + 36) == key) {
                const u32 w = load<u32>(arr + 4 * i);
                if (w != 0) store<u8>(w + 52, 1);
                break;
            }
        }
    }
    ctx.gpr[3] = 1;
}

// void f(T* o, u32 v): unless v is null, pushes v onto the Havok array
// +32 (+36 count, +40 capacity in the low 30 bits; grown by 0x007CCD60(
// the allocator 0x011DF31C, o + 32, 4) when full). r3 = the new count.
DECOMP(00346604, HkArray_Push_00346604) {
    const u32 o = arg<u32>(ctx, 0), v = arg<u32>(ctx, 1);
    ctx.gpr[3] = o;
    if (v == 0) return;
    if (load<u32>(o + 36) == (load<u32>(o + 40) & 0x3FFFFFFFu)) call(ctx, 0x007CCD60, {0x011DF31C, o + 32, 4});
    store<u32>(load<u32>(o + 32) + (load<u32>(o + 36) << 2), v);
    const u32 n = load<u32>(o + 36) + 1;
    store<u32>(o + 36, n);
    ctx.gpr[3] = n;
}

// T::~T deleting form: when +8 is set, vtable 0x00B1F480,
// AnimSkel_Release(+8), +8 = 0; then vtable 0x00B13974 and the r13 pool
// (-28332)->+44->vfn[12](pool, o, the u16 +4). Null does nothing (r3 = 0).
DECOMP(00347608, VtB1F480_Delete) {
    const u32 o = arg<u32>(ctx, 0);
    if (o == 0) {
        ctx.gpr[3] = 0;
        return;
    }
    const u32 s = load<u32>(o + 8);
    if (s != 0) {
        store<u32>(o, 0x00B1F480);
        call(ctx, o26fn::AnimSkel_Release, {s});
        store<u32>(o + 8, 0);
    }
    store<u32>(o, 0x00B13974);
    const u32 pool = Pool28332(ctx);
    VCall(ctx, pool, 12, {pool, o, load<u16>(o + 4)});
}

// void f(T* o, u32 v): when o->+32 = a is set, pushes v onto its Havok
// array +132 (+136 count, +140 capacity in the low 30 bits; grown by
// 0x007CCD60(the allocator 0x011DF31C, a + 132, 4) when full). r3 = the
// slot written (r4 / r5 = the new / old count), or o without a.
DECOMP(003540BC, HkArray_Push132_003540BC) {
    const u32 a = load<u32>(arg<u32>(ctx, 0) + 32), v = arg<u32>(ctx, 1);
    if (a == 0) return;  // r3 = o
    if (load<u32>(a + 136) == (load<u32>(a + 140) & 0x3FFFFFFFu)) call(ctx, 0x007CCD60, {0x011DF31C, a + 132, 4});
    const u32 slot = load<u32>(a + 132) + (load<u32>(a + 136) << 2);
    const u32 n = load<u32>(a + 136);
    store<u32>(a + 136, n + 1);
    store<u32>(slot, v);
    ctx.gpr[3] = slot;
    ctx.gpr[4] = n + 1;
    ctx.gpr[5] = n;
}

// void f(?, T* o): when o is set and +396 has bit 0x40000000,
// 0x004A6DF0(o->+24, 0), then 0x004A6DF0(child, 1) for each child of
// o->+24 (+272, chained by +268) with bit 0x8000 of +164
DECOMP(0036C36C, Obj_RefreshChildren_0036C36C) {
    const u32 o = arg<u32>(ctx, 1);
    if (o == 0) return;  // r3 as passed
    if ((load<u32>(o + 396) & 0x40000000u) == 0) {
        ctx.gpr[3] = 0;
        return;
    }
    call(ctx, 0x004A6DF0, {load<u32>(o + 24), 0});
    const u32 r = load<u32>(o + 24);
    ctx.gpr[3] = r;
    for (u32 c = load<u32>(r + 272); c != 0; c = load<u32>(c + 268)) {
        ctx.gpr[3] = 0;
        if ((load<u32>(c + 164) & 0x8000u) != 0) call(ctx, 0x004A6DF0, {c, 1});
    }
}

// void f(T* o): o->vfn[28](o, 0, 1) when the f32 +136^2 + +140^2 equals the
// constant at 0x003AEA10, else o->vfn[28](o, 0, 2) (f1..f3 left as the
// length, +136 and the constant)
DECOMP(003AEA18, Obj_SetModeByLen_003AEA18) {
    const u32 o = arg<u32>(ctx, 0);
    const f64 x = static_cast<f64>(load<f32>(o + 140)), y = static_cast<f64>(load<f32>(o + 136));
    const f64 k = static_cast<f64>(load<f32>(0x003AEA10));
    const f32 xx = static_cast<f32>(x * x);
    const f64 len = static_cast<f64>(static_cast<f32>(std::fma(y, y, static_cast<f64>(xx))));
    ctx.fpr[1] = len;
    ctx.fpr[2] = y;
    ctx.fpr[3] = k;
    VCall(ctx, o, 28, {o, 0, len == k ? 1u : 2u});
}

// void f(T* o): when +164 has bit 0x80000000: Effect_SetHash(+156,
// 0x4CF84295) when set; with +136 = t, 0x002E66F4(0x00F64C20->*, t, +132)
// and bit 0x40000000 set in t->+592; then Treasure_Release425BA8(o, 0)
DECOMP(00425E54, Treasure_Collect_00425E54) {
    const u32 o = arg<u32>(ctx, 0);
    if ((load<u32>(o + 164) & 0x80000000u) == 0) {
        ctx.gpr[3] = 0;
        return;
    }
    if (load<u32>(o + 156) != 0) call(ctx, o27fn::Effect_SetHash, {load<u32>(o + 156), 0x4CF84295});
    const u32 t = load<u32>(o + 136);
    if (t != 0) {
        call(ctx, 0x002E66F4, {load<u32>(0x00F64C20), t, load<u32>(o + 132)});
        const u32 u = load<u32>(o + 136);
        store<u32>(u + 592, load<u32>(u + 592) | 0x40000000u);
    }
    call(ctx, 0x00425BA8, {o, 0});
}

// Static destructor: the five TuningVar globals 0x0108A0B0, 0x0108A060,
// 0x0108A010, 0x01089FC0, 0x01089F70 (vtable 0x00B0D99C, then
// TuningVar_DestructBase_004FC1A0; r4 = the high half, as left)
DECOMP(00426B24, TuningVars_Dtor_00426B24) {
    for (const u32 v : {0x0108A0B0u, 0x0108A060u, 0x0108A010u, 0x01089FC0u, 0x01089F70u}) {
        store<u32>(v, 0x00B0D99C);
        call(ctx, 0x004FC1A0, {v, 0x01090000});
    }
}

// Static destructor: the TuningVar globals 0x010AA710 and 0x010AA6C0
// (vtable 0x00B0D934, TuningVar_DestructBase_004FC1A0), then the
// PtrVectors 0x010AA6A8 and 0x010AA694 (r4 = the high halves, as left)
DECOMP(0042CA78, CmdGlobals_Dtor_0042CA78) {
    store<u32>(0x010AA710, 0x00B0D934);
    call(ctx, 0x004FC1A0, {0x010AA710, 0x00B10000});
    store<u32>(0x010AA6C0, 0x00B0D934);
    call(ctx, 0x004FC1A0, {0x010AA6C0, 0x010B0000});
    VecDtor(ctx, 0x010AA6A8);
    VecDtor(ctx, 0x010AA694);
}

// Object* f(u32 h): the object of the live handle h in the table
// 0x010B0200 (+24 count, +28 272-byte entries: +256 bit 0x80000000 free and
// bits 15..30 the serial, +20 -> the object pointer); 0 when the flag
// 0x010AEC60 is clear or the handle is dead
DECOMP(0044CE10, Handle_Get_0044CE10) {
    const u32 h = arg<u32>(ctx, 0);
    const u32 serial = h & 0xFFFFu, idx = h >> 16;
    u32 r = 0;
    if (load<u8>(0x010AEC60) != 0 && serial != 0 && static_cast<s32>(idx) < load<s32>(0x010B0218)) {
        const u32 e = load<u32>(0x010B021C) + idx * 272;
        const u32 w = load<u32>(e + 256);
        if (((w >> 15) & 0xFFFFu) == serial && (w & 0x80000000u) == 0) {
            const u32 p = load<u32>(e + 20);
            if (p != 0) r = load<u32>(p);
        }
    }
    ctx.gpr[3] = r;
}

// bool f(): whether the player (0x00F65CA0->* + 4208 -> +8) has a
// component of kind 23 with bits 25..29 of +24 equal to 7
DECOMP(00488ADC, Player_HasComponent23_00488ADC) {
    const u32 o = load<u32>(load<u32>(0x00F65CA0) + 4208 + 8);
    u32 r = 0;
    if (o != 0) {
        for (u32 c = static_cast<u32>(call(ctx, 0x000D1288, {o, 23, 0})); c != 0; c = static_cast<u32>(call(ctx, 0x000D1288, {o, 23, c}))) {
            if (((load<u32>(c + 24) >> 25) & 0x1Fu) == 7) {
                r = 1;
                break;
            }
        }
    }
    ctx.gpr[3] = r;
}

// bool f(T* o, s32 key): whether an entry of o->+128[0..u16 +136) or of
// o->+148[0..u16 +156) has +28 equal to key
DECOMP(004ACC34, Obj_HasKey_004ACC34) {
    const u32 o = arg<u32>(ctx, 0), key = arg<u32>(ctx, 1);
    const u32 n = load<u16>(o + 136), a = load<u32>(o + 128);
    for (u32 i = 0; i < n; ++i) {
        if (load<u32>(load<u32>(a + 4 * i) + 28) == key) {
            ctx.gpr[3] = 1;
            return;
        }
    }
    for (u32 i = 0; i < load<u16>(o + 156); ++i) {
        if (load<u32>(load<u32>(load<u32>(o + 148) + 4 * i) + 28) == key) {
            ctx.gpr[3] = 1;
            return;
        }
    }
    ctx.gpr[3] = 0;
}

// bool f(T* o, const Matrix* m): o->+32.. = the 3x4 part of m (rows of 3
// at +0 / +16 / +32 / +48 to +32 / +48 / +64 / +80), then
// Matrix_IsScaled(m, the tolerance at 0x004ACCC8)
DECOMP(004ACCD0, Obj_SetMatrix_004ACCD0) {
    const u32 o = arg<u32>(ctx, 0), m = arg<u32>(ctx, 1);
    for (u32 r = 0; r < 4; ++r)
        for (u32 c = 0; c < 3; ++c) store<u32>(o + 32 + 16 * r + 4 * c, load<u32>(m + 16 * r + 4 * c));
    callf(ctx, 0x005544FC, {m}, {static_cast<f64>(load<f32>(0x004ACCC8))});
}

// T::~T deleting form: vtable 0x00B21EF8, +40 = 0 when set,
// ~PtrVector(+20), vtable 0x00B11B20, SmallFree(o, 44). Null does nothing
// (r3 = 0).
DECOMP(004BFD3C, VtB21EF8_Delete) {
    const u32 o = arg<u32>(ctx, 0);
    if (o == 0) {
        ctx.gpr[3] = 0;
        return;
    }
    const u32 c = load<u32>(o + 40);
    store<u32>(o, 0x00B21EF8);
    if (c != 0) store<u32>(o + 40, 0);
    VecDtor(ctx, o + 20);
    store<u32>(o, 0x00B11B20);
    call(ctx, o19fn::SmallFree, {o, 44});
}

// bool f(T* o, Effect* e): clears bit 0x80 of +136; when e is set,
// releases the old instance +4 (vfn[8]) and +4 = Effect_Instance4BBABC(e,
// o), returning 1; else 0
DECOMP(004CF330, Effect_SetInstance_004CF330) {
    const u32 o = arg<u32>(ctx, 0), e = arg<u32>(ctx, 1);
    const u32 w = load<u32>(o + 136) & ~0x80u;
    const u32 old = load<u32>(o + 4);
    store<u32>(o + 136, w);
    if (e == 0) {
        ctx.gpr[3] = 0;
        return;
    }
    if (old != 0) VCall(ctx, old, 8, {old});
    store<u32>(o + 4, static_cast<u32>(call(ctx, 0x004BBABC, {e, o})));
    ctx.gpr[3] = 1;
}

// u32 f(?, const char* name): the value of String_HashNoCase(name) in the
// map 0x010C2B20, else in the map 0x010C2AF0 (HashMap_Find)
DECOMP(004E6074, Ui_LookupByName_004E6074) {
    Stack frame(ctx, 144);
    const u32 name = arg<u32>(ctx, 1);
    const u32 k1 = frame.addr() + 116, k2 = frame.addr() + 112;
    store<u32>(k1, static_cast<u32>(call(ctx, 0x0056A710, {name})));
    u32 v = load<u32>(static_cast<u32>(call(ctx, 0x007948E4, {0x010C2B20, k1})));
    if (v == 0) {
        store<u32>(k2, static_cast<u32>(call(ctx, 0x0056A710, {name})));
        v = load<u32>(static_cast<u32>(call(ctx, 0x007948E4, {0x010C2AF0, k2})));
    }
    ctx.gpr[3] = v;
}

// T::~T deleting form for this = o + 344 (VtB123D0DtorAt, SmallFree(o,
// 384)). A null o does nothing (r3 = this).
DECOMP(00747288, VtB123D0_DeleteAdj344) {
    const u32 o = arg<u32>(ctx, 0) - 344;
    if (o == 0) return;
    VtB123D0DtorAt(ctx, o);
    call(ctx, o19fn::SmallFree, {o, 384});
}

// u8 f(T* o): o->vfn[88](o, 1, 1, 1, 1) when the f32 +348 is at most the
// constant at 0x0074E570 (f1 / f2 = them); then the u8 +1688 = bit 0x100
// of +172 (returned)
DECOMP(0074E578, Obj_UpdateFlag1688_0074E578) {
    const u32 o = arg<u32>(ctx, 0);
    const f64 v = static_cast<f64>(load<f32>(o + 348)), k = static_cast<f64>(load<f32>(0x0074E570));
    ctx.fpr[1] = v;
    ctx.fpr[2] = k;
    if (v <= k) VCall(ctx, o, 88, {o, 1, 1, 1, 1});
    const u32 b = (load<u32>(o + 172) >> 8) & 1u;
    store<u8>(o + 1688, static_cast<u8>(b));
    ctx.gpr[3] = b;
}

// T::~T: vtable 0x00B16B00, ~PtrVector(+420), ~PtrVector(+400), vtable
// 0x00B15970, then the base 0x000E2A34(o)
DECOMP(007547A8, VtB16B00_Dtor) {
    const u32 o = arg<u32>(ctx, 0);
    store<u32>(o, 0x00B16B00);
    VecDtor(ctx, o + 420);
    VecDtor(ctx, o + 400);
    store<u32>(o, 0x00B15970);
    call(ctx, 0x000E2A34, {o});
}

// T::~T deleting form: when +92 is set, vtable 0x00B1CE10,
// Mem_FreeFrom(+92, 0), +92 = 0; vtable 0x00B0DAAC, SortedMap_Remove(
// 0x01056F04, o + 4), SmallFree(o, 96). Null does nothing (r3 = 0).
DECOMP(00765308, VtB1CE10_Delete) {
    const u32 o = arg<u32>(ctx, 0);
    if (o == 0) {
        ctx.gpr[3] = 0;
        return;
    }
    const u32 p = load<u32>(o + 92);
    if (p != 0) {
        store<u32>(o, 0x00B1CE10);
        call(ctx, o25fn::Mem_FreeFrom, {p, 0});
        store<u32>(o + 92, 0);
    }
    store<u32>(o, 0x00B0DAAC);
    call(ctx, 0x007330C0, {0x01056F04, o + 4});
    call(ctx, o19fn::SmallFree, {o, 96});
}

// T::~T: vtable 0x00B1F280, AnimSkel_Release of +104, +88, +84 (each when
// set, then cleared), vtable 0x00B13974 (r3 = it, r4 = 0)
DECOMP(00770524, VtB1F280_Dtor) {
    const u32 o = arg<u32>(ctx, 0);
    store<u32>(o, 0x00B1F280);
    for (const u32 off : {104u, 88u, 84u}) {
        const u32 s = load<u32>(o + off);
        if (s != 0) call(ctx, o26fn::AnimSkel_Release, {s});
        store<u32>(o + off, 0);
    }
    store<u32>(o, 0x00B13974);
    ctx.gpr[3] = 0x00B13974;
    ctx.gpr[4] = 0;
}

// T::~T: when +164 is set, vtable 0x00B1F978, Mem_FreeFrom(+164, 0), +164
// = 0; when +148 is set, vtable 0x00B1F6A0, Mem_FreeFrom(+148, 0), +148 =
// 0; then vtable 0x00B1F668 (r3 = it)
DECOMP(007733D0, VtB1F978_Dtor) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 p = load<u32>(o + 164);
    if (p != 0) {
        store<u32>(o, 0x00B1F978);
        call(ctx, o25fn::Mem_FreeFrom, {p, 0});
        store<u32>(o + 164, 0);
    }
    const u32 q = load<u32>(o + 148);
    if (q != 0) {
        store<u32>(o, 0x00B1F6A0);
        call(ctx, o25fn::Mem_FreeFrom, {q, 0});
        store<u32>(o + 148, 0);
    }
    store<u32>(o, 0x00B1F668);
    ctx.gpr[3] = 0x00B1F668;
}

// u32 f(const Ref* p): with the 128-byte records g->+24 (g = 0x00F65038->*),
// a = record s16 p->+4 (0 for -1) and b = record s16 p->+2 (0 for -1): the
// word at a + 4 * (the u8 +1 of (b + the u16 0x0106CF7E, or 0 when b's u8
// +2 is 0)) + the u16 0x0106CF70 (the original reads near 0 for a missing
// b, as here)
DECOMP(00774E1C, Ref_Lookup_00774E1C) {
    const u32 p = arg<u32>(ctx, 0);
    const u32 g = load<u32>(0x00F65038);
    const s32 ia = load<s16>(p + 4), ib = load<s16>(p + 2);
    const u32 a = ia != -1 ? load<u32>(g + 24) + (static_cast<u32>(ia) << 7) : 0u;
    const u32 b = ib != -1 ? load<u32>(g + 24) + (static_cast<u32>(ib) << 7) : 0u;
    const u32 sel = load<u8>(b + 2) != 0 ? b + load<u16>(0x0106CF7E) : 0u;
    ctx.gpr[3] = load<u32>(a + load<u16>(0x0106CF70) + (static_cast<u32>(load<u8>(sel + 1)) << 2));
}

// char* f(T* o, char* buf): sprintf(buf, 0x00B21E08), then when +28 = c is
// set, sprintf(buf, 0x00B21E12, c->vfn[12](c)); returns buf
DECOMP(00793F4C, Obj_Describe_00793F4C) {
    const u32 o = arg<u32>(ctx, 0), buf = arg<u32>(ctx, 1);
    call(ctx, o23fn::String_Printf, {buf, 0x00B21E08});
    const u32 c = load<u32>(o + 28);
    if (c != 0) {
        const u32 s = static_cast<u32>(VCall(ctx, c, 12, {c}));
        call(ctx, o23fn::String_Printf, {buf, 0x00B21E12, s});
    }
    ctx.gpr[3] = buf;
}

// ---------------------------------------------------------------- sweep: 38 instructions
// The player object: 0x00F65CA0->* + 4208 -> +8 (for player index i, the
// 0x20C80-byte slot i).
static u32 PlayerObj27(u32 i = 0) { return load<u32>(load<u32>(0x00F65CA0) + i * 0x20C80u + 4208 + 8); }

// void f(T* o, u8 a, u8 b): o->+700->vfn[304](it, a) and
// o->+704->vfn[304](it, b), each when set
DECOMP(00043B20, Obj_Set304Pair_00043B20) {
    const u32 o = arg<u32>(ctx, 0), a = arg<u32>(ctx, 1) & 0xFFu, b = arg<u32>(ctx, 2) & 0xFFu;
    const u32 p = load<u32>(o + 700);
    ctx.gpr[3] = p;
    if (p != 0) VCall(ctx, p, 304, {p, a});
    const u32 q = load<u32>(o + 704);
    if (q != 0) VCall(ctx, q, 304, {q, b});
}

// void f(T* o, u32 key): o->+48->vfn[176](it, 0), then 0x00053A80(o, 3) for
// the key 0x0464F605 or 0x00053A80(o, 8) for 0x8839CACD
DECOMP(0005B964, Obj_OnKey_0005B964) {
    const u32 o = arg<u32>(ctx, 0), key = arg<u32>(ctx, 1);
    const u32 m = load<u32>(o + 48);
    VCall(ctx, m, 176, {m, 0});
    if (key == 0x0464F605u) call(ctx, 0x00053A80, {o, 3});
    else if (key == 0x8839CACDu) call(ctx, 0x00053A80, {o, 8});
    else ctx.gpr[3] = 0x8839CACDu;  // the last key compared, as left
}

// void f(): the player's vfn[28](p, 0, 0) then vfn[28](p, 1, 7) (r3 = the
// player slot + 4208 without one)
DECOMP(000818C0, Player_ResetState_000818C0) {
    const u32 p = PlayerObj27();
    ctx.gpr[3] = load<u32>(0x00F65CA0) + 4208;
    if (p == 0) return;
    VCall(ctx, p, 28, {p, 0, 0});
    VCall(ctx, p, 28, {p, 1, 7});
}

// void f(T* o): when the player o->+12 has bit 8 of +192 and its +380 = q
// has kind (+12) 5, Obj_SetKeyBit_0027C914(q, o->+16, the u8 o->+20)
// (r3 = o otherwise)
DECOMP(000830BC, Player_SetKeyBit_000830BC) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 p = PlayerObj27(load<u32>(o + 12));
    if (p == 0 || (load<u32>(p + 192) & 8u) == 0) return;
    const u32 q = load<u32>(p + 380);
    if (q == 0 || load<u32>(q + 12) != 5) return;
    call(ctx, 0x0027C914, {q, load<u32>(o + 16), load<u8>(o + 20)});
}

// void f(T* o): when the player is set and Obj_Notify26(player) = r is
// nonzero, r->+2456 = the index of o->+12 in the 2-word table 0x00F558BC
// (unchanged when absent)
DECOMP(00087588, Player_SetMode_00087588) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 p = PlayerObj27();
    ctx.gpr[3] = p;
    if (p == 0) return;
    const u32 r = static_cast<u32>(call(ctx, 0x000D94CC, {p}));
    if (r == 0) return;
    const u32 key = load<u32>(o + 12);
    for (u32 i = 0; i < 2; ++i) {
        if (load<u32>(0x00F558BC + 4 * i) == key) {
            store<u32>(r + 2456, i);
            return;
        }
    }
}

// void f(T* o): with r = Obj_Notify24(player) set: Field2284_Set(r, the
// f32 +12) when +20 has bit 0x80000000, Field2288_Set(r, +16) when it has
// 0x40000000
DECOMP(0009008C, Player_SetFields_0009008C) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 r = static_cast<u32>(call(ctx, 0x000D949C, {PlayerObj27()}));
    if (r == 0) return;
    if ((load<u32>(o + 20) & 0x80000000u) != 0) callf(ctx, 0x0029F6E8, {r}, {static_cast<f64>(load<f32>(o + 12))});
    if ((load<u32>(o + 20) & 0x40000000u) == 0) {
        ctx.gpr[3] = 0;
        return;
    }
    callf(ctx, 0x0029F724, {r}, {static_cast<f64>(load<f32>(o + 16))});
}

// void f(T* o): when +12 and +16 are set, pushes a new 28-byte VtB21340
// command (0x00439C28(p, +12, +16, the u8 +20)); else r3 = 0
DECOMP(000A604C, Obj_PushNewB21340_000A604C) {
    const u32 o = arg<u32>(ctx, 0);
    if (load<u32>(o + 12) == 0 || load<u32>(o + 16) == 0) {
        ctx.gpr[3] = 0;
        return;
    }
    PushNew26(ctx, 28, [&](u32 p) { call(ctx, 0x00439C28, {p, load<u32>(o + 12), load<u32>(o + 16), load<u8>(o + 20)}); });
}

// T::~T deleting form: vtable 0x00B11B38, Emitter_Stop(the handle +20, 1)
// when either u16 half is set, 0x004B6884(o), SmallFree(o, 24). Null does
// nothing (r3 = 0).
DECOMP(000A76C4, VtB11B38_Delete) {
    const u32 o = arg<u32>(ctx, 0);
    if (o == 0) {
        ctx.gpr[3] = 0;
        return;
    }
    store<u32>(o, 0x00B11B38);
    if (load<u16>(o + 20) != 0 || load<u16>(o + 22) != 0) call(ctx, o25fn::Emitter_Stop, {load<u32>(o + 20), 1});
    call(ctx, 0x004B6884, {o});
    call(ctx, o19fn::SmallFree, {o, 24});
}

// void f(T* o): flags +0 |= 0x0A000000 with 0x05000000 cleared, +4 = 0.5f,
// +28 = 1.0f; then the world's object at +0x1286A4 is released (vfn[4])
// and cleared when set (r3 = the world, r4 = 0, r5 = 0x1286A4)
DECOMP(000ADC5C, Obj_ResetWorld1286A4_000ADC5C) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 w = (load<u32>(o) | 0x0A000000u) & ~0x05000000u;
    store<u32>(o + 4, 0x3F000000);
    store<u32>(o, w);
    const u32 g = load<u32>(0x00F65CA0);
    store<u32>(o + 28, 0x3F800000);
    const u32 p = load<u32>(g + 0x1286A4);
    ctx.gpr[3] = p;
    if (p == 0) return;
    VCall(ctx, p, 4, {p});
    const u32 g2 = load<u32>(0x00F65CA0);
    store<u32>(g2 + 0x1286A4, 0);
    ctx.gpr[3] = g2;
    ctx.gpr[4] = 0;
    ctx.gpr[5] = 0x1286A4;
}

// f32 f(T* o): when the world's +0x1281D4->+4 is set, the u16 +510 of the
// first set +28 among the 48-byte entries o->+388->+0[0..u16 +8), as the
// high half of an f32; else the constant at 0x000E6550
DECOMP(000E6558, Obj_GetEntryScale_000E6558) {
    const u32 o = arg<u32>(ctx, 0);
    ctx.fpr[1] = static_cast<f64>(load<f32>(0x000E6550));
    if (load<u32>(load<u32>(load<u32>(0x00F65CA0) + 0x1281D4) + 4) == 0) return;  // r3 = o
    const u32 a = load<u32>(o + 388);
    ctx.gpr[3] = a;
    for (u32 i = 0; i < (a != 0 ? load<u16>(a + 8) : 0u); ++i) {
        const u32 e = load<u32>(load<u32>(a) + 48 * i + 28);
        if (e != 0) {
            const u32 w = static_cast<u32>(load<u16>(e + 510)) << 16;
            ctx.gpr[3] = w;
            ctx.fpr[1] = static_cast<f64>(std::bit_cast<f32>(w));
            return;
        }
    }
}

// void f(T* o): releases the skeleton +28 (then cleared), then removes the
// Havok entities of +24 and +20 (each when it and its +8 are set) with
// 0x0092FFC8(&scratch byte, it->+8, it)
DECOMP(001062B8, Obj_RemoveBodies_001062B8) {
    Stack frame(ctx, 128);
    const u32 o = arg<u32>(ctx, 0);
    if (load<u32>(o + 28) != 0) {
        call(ctx, o26fn::AnimSkel_Release, {load<u32>(o + 28)});
        store<u32>(o + 28, 0);
    }
    const u32 a = load<u32>(o + 24);
    ctx.gpr[3] = a;
    if (a != 0 && load<u32>(a + 8) != 0) call(ctx, 0x0092FFC8, {frame.addr() + 112, load<u32>(a + 8), a});
    const u32 b = load<u32>(o + 20);
    if (b == 0) return;
    ctx.gpr[3] = load<u32>(b + 8);
    if (load<u32>(b + 8) != 0) call(ctx, 0x0092FFC8, {frame.addr() + 113, load<u32>(b + 8), b});
}

// void f(T* o): with the effect +412 = e: unless e->+4 is unset or has
// kind (+8) 0x10CBB, Effect_Release(e, 0x10CBB) and Effect_SetTime(+412,
// the f32 at 0x00129C10); then bit 0x200 set in (+412)->+136 and +412 = 0
DECOMP(00129C18, Obj_EndEffect_00129C18) {
    const u32 o = arg<u32>(ctx, 0);
    u32 e = load<u32>(o + 412);
    ctx.gpr[3] = e;
    if (e == 0) return;
    const u32 p = load<u32>(e + 4);
    if (p != 0 && load<u32>(p + 8) != 0x00010CBBu) {
        call(ctx, 0x004CF3C4, {e, 0x00010CBB});
        callf(ctx, 0x004CF710, {load<u32>(o + 412)}, {static_cast<f64>(load<f32>(0x00129C10))});
        e = load<u32>(o + 412);
    }
    store<u32>(e + 136, load<u32>(e + 136) | 0x200u);
    store<u32>(o + 412, 0);
    ctx.gpr[3] = e;
    ctx.gpr[4] = 0;
}

// void f(T* o, u32 n): o->+532[i]->vfn[8](it, o) for i = 0..n (n + 1
// calls, unsigned), then +668 = n
DECOMP(00175B60, Obj_NotifySlots_00175B60) {
    const u32 o = arg<u32>(ctx, 0), n = arg<u32>(ctx, 1);
    u32 i = 0;
    do {
        const u32 p = load<u32>(o + 532 + 4 * i);
        VCall(ctx, p, 8, {p, o});
        ++i;
    } while (i <= n);
    store<u32>(o + 668, n);
}

// T::~T deleting form: vtable 0x00B15150, RefNode_Release of the 5 words
// at +172, the base destructor 0x000C2DA0, SmallFree(o, 192). Null does
// nothing (r3 = 0).
DECOMP(001A70D4, VtB15150_Delete) {
    const u32 o = arg<u32>(ctx, 0);
    if (o == 0) {
        ctx.gpr[3] = 0;
        return;
    }
    store<u32>(o, 0x00B15150);
    for (u32 i = 0; i < 5; ++i) call(ctx, o24fn::RefNode_Release, {load<u32>(o + 172 + 4 * i)});
    call(ctx, o26fn::Obj_BaseDtor_0C2DA0, {o});
    call(ctx, o19fn::SmallFree, {o, 192});
}

// bool f(T* o, f32 dt): false unless 0x001AEA50(o); clears bit 0x40 of
// the u8 +572 and calls 0x001BF8E8(o) when it was set; counts the f32 +568
// down by dt while above the constant at 0x001BFA48; true (f1 = +568)
DECOMP(001BFA50, Obj_TickTimer_001BFA50) {
    const u32 o = arg<u32>(ctx, 0);
    const f64 dt = farg(ctx, 0);
    if (static_cast<s32>(call(ctx, 0x001AEA50, {o})) == 0) {
        ctx.gpr[3] = 0;
        return;
    }
    const u8 b = load<u8>(o + 572);
    if ((b & 0x40u) != 0) {
        store<u8>(o + 572, static_cast<u8>(b & ~0x40u));
        call(ctx, 0x001BF8E8, {o});  // f1 as the previous call left it
    }
    f64 f = static_cast<f64>(load<f32>(o + 568));
    ctx.fpr[2] = static_cast<f64>(load<f32>(0x001BFA48));
    if (f > ctx.fpr[2]) {
        f = static_cast<f64>(static_cast<f32>(f - dt));
        store<f32>(o + 568, static_cast<f32>(f));
    }
    ctx.fpr[1] = f;
    ctx.gpr[3] = 1;
}

// The 0x00B16B48 destructor body: vtables 0x00B16B48 / 0x00B16D1C (+380) /
// 0x00B16D44 (+384), ~PtrVector(+576), ~PtrVector(+556), the base
// destructor 0x001AE468.
static void VtB16B48DtorAt(PPUContext& ctx, u32 o) {
    store<u32>(o, 0x00B16B48);
    store<u32>(o + 380, 0x00B16D1C);
    store<u32>(o + 384, 0x00B16D44);
    VecDtor(ctx, o + 576);
    VecDtor(ctx, o + 556);
    call(ctx, 0x001AE468, {o});
}
// T::~T (VtB16B48DtorAt)
DECOMP(001C33C4, VtB16B48_Dtor) { VtB16B48DtorAt(ctx, arg<u32>(ctx, 0)); }

// T::~T deleting form for this = o + 384 (VtB174C0DeleteAt). A null o does
// nothing (r3 = this).
DECOMP(001D1EA0, VtB174C0_DeleteAdj384) {
    const u32 o = arg<u32>(ctx, 0) - 384;
    if (o != 0) VtB174C0DeleteAt(ctx, o);
}
// T::~T deleting form for this = o + 380 (VtB174C0DeleteAt). A null o does
// nothing (r3 = this).
DECOMP(001D1F38, VtB174C0_DeleteAdj380) {
    const u32 o = arg<u32>(ctx, 0) - 380;
    if (o != 0) VtB174C0DeleteAt(ctx, o);
}

// void f(T* o, u32 v): the low 24 bits of +400 = v; when o->+388 = a has
// a body +8, a->+44 = (v << 16) | its low 5 bits and 0x00930B70(body, a,
// 0, 1); then 0x000DA138(o, v)
DECOMP(00207114, Obj_SetId_00207114) {
    const u32 o = arg<u32>(ctx, 0), v = arg<u32>(ctx, 1);
    const u32 w = (load<u32>(o + 400) & 0xFF000000u) | (v & 0x00FFFFFFu);
    const u32 a = load<u32>(o + 388);
    store<u32>(o + 400, w);
    const u32 b = load<u32>(a + 8);
    if (b != 0) {
        store<u32>(a + 44, (load<u32>(a + 44) & 0x1Fu) | (v << 16));
        call(ctx, 0x00930B70, {b, a, 0, 1});
    }
    call(ctx, 0x000DA138, {o, v});
}

// Profile* f(Ref* r): follows the chain of profile refs (+0 the hash, +4
// the resolved profile, the next ref at its +1528): the first resolved +4;
// else resolves +0 with Save_Profile0C9FC8 (kept only when its +28 has bit
// 0x800); a hash that fails gets Hash_ToString (its result returned); 0
// for a null hash
DECOMP(0022D3B4, Profile_Resolve_0022D3B4) {
    u32 r = arg<u32>(ctx, 0);
    for (;;) {
        const u32 p = load<u32>(r + 4);
        if (p != 0) {
            ctx.gpr[3] = p;
            return;
        }
        if (load<u32>(r) == 0) {
            ctx.gpr[3] = 0;
            return;
        }
        const u32 q = static_cast<u32>(call(ctx, 0x000C9FC8, {load<u32>(r)}));
        u32 k = 0;
        if (q != 0) {
            if ((load<u32>(q + 28) & 0x800u) != 0) k = q;
            store<u32>(r + 4, k);
        }
        if (k == 0) {
            call(ctx, o27fn::Hash_ToString, {load<u32>(r)});
            return;
        }
        r = k + 1528;
    }
}

// The game's save record of the current player (0x00F65CA0->* + 4056 * the
// u8 player index at +4208).
static u32 PlayerSave27(u32 g) { return g + 4056u * load<u8>(g + 4208); }

// void f(T* o): counts up the u8 +325 (to at most 255) in the save record
// of the player slot (s8 o->+8->+172) and awards the trophy 0x41F10138
// when it reaches the target at 0x01067AC8
DECOMP(002BCADC, Trophy_CountUp_002BCADC) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 g = load<u32>(0x00F65CA0);
    const s32 i = load<s8>(load<u32>(o + 8) + 172);
    const u32 slot = g + static_cast<u32>(i) * 0x20C80u;
    const u32 r = g + 4056u * load<u8>(slot + 4208) + 80;
    const u32 c = load<u8>(r + 325);
    ctx.gpr[3] = c;
    if (c >= 255) return;
    const u32 n = (c + 1) & 0xFFu;
    store<u8>(r + 325, static_cast<u8>(n));
    const s32 t = load<s32>(0x01067AC8);
    ctx.gpr[3] = static_cast<u32>(t);
    if (t > static_cast<s32>(n)) return;
    call(ctx, 0x000B27B4, {0x41F10138});
}

// void f(T* o, u8 on): with the item o->+36 = c set: sets (on == 1) or
// clears bit 0x20 of the u16 at +496 + 4 * (u8 c->+822) in the player's save
// record, then the record's +240 = c->+812 (returned)
DECOMP(002E26A8, Save_SetItemFlag_002E26A8) {
    const u32 c = load<u32>(arg<u32>(ctx, 0) + 36);
    ctx.gpr[3] = c;
    if (c == 0) return;
    const u32 a = PlayerSave27(load<u32>(0x00F65CA0)) + (static_cast<u32>(load<u8>(c + 822)) << 2) + 496;
    const u32 h = load<u16>(a);
    store<u16>(a, static_cast<u16>((arg<u32>(ctx, 1) & 0xFFu) == 1 ? (h | 0x20u) : (h & ~0x20u)));
    const u32 w = load<u32>(c + 812);
    store<u32>(PlayerSave27(load<u32>(0x00F65CA0)) + 80 + 160, w);
    ctx.gpr[3] = w;
}

// bool f(T* o, u32 out[3]): with +2212 set: out = {+2212, (s64)f32 +2660 +
// +2216 (or (s64)+2644 when the u8 +18 is 5), the u8 +2231 ? (s64)(+2644 +
// (f32)+2216) : 0} (the low words of the truncations) and 1; else 0
DECOMP(0033F8D0, Obj_GetRange_0033F8D0) {
    const u32 o = arg<u32>(ctx, 0), out = arg<u32>(ctx, 1);
    const u32 p = load<u32>(o + 2212);
    if (p == 0) {
        ctx.gpr[3] = 0;
        return;
    }
    const f64 a = static_cast<f64>(load<f32>(o + 2660));
    f64 b = static_cast<f64>(load<f32>(o + 2644));
    const u32 base = load<u32>(o + 2216);
    u32 v = static_cast<u32>(rt::fctid(a, true)) + base;
    if (load<u8>(o + 18) == 5) v = static_cast<u32>(rt::fctid(b, true));
    store<u32>(out, p);
    const u32 flag = load<u8>(o + 2231);
    store<u32>(out + 4, v);
    u32 w = 0;
    ctx.fpr[1] = b;
    if (flag != 0) {
        b = static_cast<f64>(static_cast<f32>(b + static_cast<f64>(static_cast<f32>(static_cast<f64>(static_cast<s32>(base))))));
        const u64 t = rt::fctid(b, true);
        w = static_cast<u32>(t);
        ctx.fpr[1] = std::bit_cast<f64>(t);  // f1 left = the fctidz bits
    }
    store<u32>(out + 8, w);
    ctx.gpr[3] = 1;
}

// void f(const Ref* r, T* o): with e = the 132-byte record s16 r->+6 at
// o + 152 when below the u16 o->+144 and its +0 is below 10 (else 0), the
// word at e + the u16 0x0106CF80 + 4 is released (RefNode_Release) and
// cleared (also for a null e, as in the original)
DECOMP(00393ED4, Obj_ReleaseSlotRef_00393ED4) {
    const u32 r = arg<u32>(ctx, 0), o = arg<u32>(ctx, 1);
    const s32 i = load<s16>(r + 6);
    u32 e = 0;
    if (i < static_cast<s32>(load<u16>(o + 144))) {
        e = o + static_cast<u32>(i) * 132 + 152;
        if (load<s32>(e) >= 10) e = 0;
    }
    const u32 q = e + load<u16>(0x0106CF80) + 4;
    ctx.gpr[3] = q - 4;
    if (q == 0) return;
    call(ctx, o24fn::RefNode_Release, {load<u32>(q)});
    store<u32>(q, 0);
    ctx.gpr[3] = 0;
}

// The point lookup shape (o, key): v = PointMap_Find(0x00F8F154->*->+16,
// &key)->*; fn(0x00F657FC->*, v, o), or a warning (fmt and the hash name)
// when v is null.
static void PointCall27(PPUContext& ctx, u32 fn, u32 fmt) {
    Stack frame(ctx, 144);
    const u32 o = arg<u32>(ctx, 0), key = arg<u32>(ctx, 1);
    const u32 k = frame.addr() + 112;
    const u32 m = load<u32>(load<u32>(0x00F8F154) + 16);
    store<u32>(k, key);
    const u32 v = load<u32>(static_cast<u32>(call(ctx, 0x0073343C, {m, k})));
    if (v == 0) {
        WarnHash27(ctx, fmt, key);
        return;
    }
    call(ctx, fn, {load<u32>(0x00F657FC), v, o});
}
// void f(T* o, u32 key): PointCall27 with 0x003A1DC4 (warning 0x00ADE0E0)
DECOMP(003A200C, Point_Apply_003A200C) { PointCall27(ctx, 0x003A1DC4, 0x00ADE0E0); }
// void f(T* o, u32 key): PointCall27 with 0x003A20F8 (warning 0x00ADE200)
DECOMP(003A233C, Point_Apply_003A233C) { PointCall27(ctx, 0x003A20F8, 0x00ADE200); }

// T::~T: vtable 0x00B20338, 0x004A0E08 on the nine 36-byte members from
// +420 down to +132, ~PtrVector(+112), then the base 0x00490B34(o)
DECOMP(003B0630, VtB20338_Dtor) {
    const u32 o = arg<u32>(ctx, 0);
    store<u32>(o, 0x00B20338);
    for (s32 i = 8; i >= 0; --i) call(ctx, 0x004A0E08, {o + 132 + 36 * static_cast<u32>(i)});
    VecDtor(ctx, o + 112);
    call(ctx, 0x00490B34, {o});
}

// void f(T* o): the import 0x00ABDCE4 on the s32 handle +0 and on each of
// the +20 handles from +4 on; then +0 = 0
DECOMP(0042725C, Handles_Destroy_0042725C) {
    const u32 o = arg<u32>(ctx, 0);
    call(ctx, 0x00ABDCE4, {Sx(load<u32>(o))});
    ctx.gpr[3] = load<u32>(o + 20);
    for (u32 i = 0; i < load<u32>(o + 20); ++i) {
        call(ctx, 0x00ABDCE4, {Sx(load<u32>(o + 4 + 4 * i))});
        ctx.gpr[3] = o + 4 + 4 * i;  // r3 left = the last handle's address
    }
    store<u32>(o, 0);
}

// void f(T* o): +12 = Obj_FindByHandle42C2B0(+16, 0); when found,
// found->vfn[128](found, +20, bit 0x80000000 of +24) (else
// Hash_ToString(+16)); then Flag_Set8_40000000(o)
DECOMP(00439CB0, Cmd_ApplyFound_00439CB0) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 r = static_cast<u32>(call(ctx, 0x0042C2B0, {load<u32>(o + 16), 0}));
    store<u32>(o + 12, r);
    if (r == 0) {
        call(ctx, o27fn::Hash_ToString, {load<u32>(o + 16)});
    } else {
        // the OPD goes in r7 and its entry in r6 here
        const u32 opd = load<u32>(load<u32>(r) + 128);
        call_ptr(ctx, opd, {r, load<u32>(o + 20), load<u32>(o + 24) >> 31, load<u32>(opd), opd});
    }
    ctx.gpr[3] = call(ctx, o27fn::Flag_Set8_40000000, {o});
}

// bool f(?, ?, u32 i): frees the buffer of the 72-byte stream record i at
// 0x010B0BC8 (+4) once its +152 is set (fios_Free on the FIOS scheduler);
// 1 when freed or absent, 0 while still busy
DECOMP(0045017C, Stream_FreeBuffer_0045017C) {
    const u32 e = 0x010B0BC8u + arg<u32>(ctx, 2) * 72;
    const u32 b = load<u32>(e + 4);
    if (b == 0) {
        ctx.gpr[3] = 1;
        return;
    }
    if (load<u32>(b + 152) == 0) {
        ctx.gpr[3] = 0;
        return;
    }
    const u32 s = static_cast<u32>(call(ctx, 0x005FC90C, {}));
    call(ctx, 0x005FE784, {s, load<u32>(e + 4)});
    store<u32>(e + 4, 0);
    ctx.gpr[3] = 1;
}

// T::T(): the base 0x0048F27C(o), vtable 0x00B21988, +8..+24 and +40..+84
// = 0 with +76 = -1, bits 0xC0 of the u8 +88 cleared (r3 = the vtable, r4
// = 0, r5 = the byte, r6 = -1)
DECOMP(004558BC, VtB21988_Ctor) {
    const u32 o = arg<u32>(ctx, 0);
    call(ctx, 0x0048F27C, {o});
    const u32 b = load<u8>(o + 88) & 0x3Fu;
    store<u32>(o + 8, 0);
    store<u32>(o, 0x00B21988);
    for (u32 off = 12; off <= 24; off += 4) store<u32>(o + off, 0);
    for (u32 off = 40; off <= 72; off += 4) store<u32>(o + off, 0);
    store<s32>(o + 76, -1);
    store<u32>(o + 80, 0);
    store<u32>(o + 84, 0);
    store<u8>(o + 88, static_cast<u8>(b));
    ctx.gpr[3] = 0x00B21988;
    ctx.gpr[4] = 0;
    ctx.gpr[5] = b;
    ctx.gpr[6] = static_cast<u64>(-1);
}

// void f(View* v): clears 0x00F6688C / 0x00F66890, FrameBuffer_PushMarker(
// world->+0x1285E4->+12), Arena_Snapshot(0x00F65C90->*->+8, v->+0->+24),
// then world->+0x1285E8 = 0
DECOMP(00459100, View_BeginFrame_00459100) {
    const u32 v = arg<u32>(ctx, 0);
    store<u32>(0x00F6688C, 0);
    store<u32>(0x00F66890, 0);
    const u32 g0 = load<u32>(0x00F65CA0);
    // r4 / r5 = the world and 0x00F60000 as the original leaves them (the callee reads r5)
    call(ctx, 0x005220E8, {load<u32>(load<u32>(g0 + 0x1285E4) + 12), g0, 0x00F60000});
    call(ctx, 0x00565178, {load<u32>(load<u32>(0x00F65C90) + 8), load<u32>(load<u32>(v) + 24)});
    const u32 g = load<u32>(0x00F65CA0);
    store<u32>(g + 0x1285E8, 0);
    ctx.gpr[3] = 0x001285E8;  // r3 / r4 left = the offset and the world
    ctx.gpr[4] = g;
}

// void f(): one frame of the 0x010B19E0 renderer object: Render_BeginFrame,
// 0x00458710(it), Once_Set10BD6C0(it), Obj_MemberVCall_0001BEA0(
// 0x00F557F0->*), then 0x00457BFC(it) (twice around
// RenderState_Invalidate when the u8 0x010C5A42 is set) and
// Render_EndFrame (each passed the previous result in r3)
DECOMP(0049AE40, Game_RenderFrame_0049AE40) {
    constexpr u32 kRen = 0x010B19E0;
    call(ctx, 0x0051008C, {});
    call(ctx, 0x00458710, {load<u32>(kRen)});
    call(ctx, 0x004586E8, {load<u32>(kRen)});
    call(ctx, 0x0001BEA0, {load<u32>(0x00F557F0)});
    if (load<u8>(0x010C5A42) != 0) {
        const u32 r = static_cast<u32>(call(ctx, 0x00457BFC, {load<u32>(kRen)}));
        call(ctx, 0x0051038C, {r});
    }
    const u32 r = static_cast<u32>(call(ctx, 0x00457BFC, {load<u32>(kRen)}));
    call(ctx, 0x005105E0, {r});
}

// f32 f(const Poly* p): the shoelace sum over the u16 +8 16-byte points
// p->+0 (x at +0, z at +8; prev.x * cur.z - cur.x * prev.z) starting from
// the constant at 0x004A7124, times the constant at 0x004A7128 (r3 =
// 0x004A7124)
DECOMP(004A7130, Poly_SignedArea_004A7130) {
    const u32 p = arg<u32>(ctx, 0);
    const s32 n = load<u16>(p + 8);
    f32 s = load<f32>(0x004A7124);
    if (n > 0) {
        const u32 d = load<u32>(p);
        u32 prev = static_cast<u32>(n - 1);
        for (s32 i = 0; i < n; ++i) {
            const u32 c = d + 16 * static_cast<u32>(i), q = d + 16 * prev;
            const f32 t = static_cast<f32>(static_cast<f64>(load<f32>(c)) * static_cast<f64>(load<f32>(q + 8)));
            const f32 u = Fms(load<f32>(q), load<f32>(c + 8), t);
            s = static_cast<f32>(static_cast<f64>(s) + static_cast<f64>(u));
            prev = static_cast<u32>(i);
        }
    }
    ctx.fpr[1] = static_cast<f64>(static_cast<f32>(static_cast<f64>(s) * static_cast<f64>(load<f32>(0x004A7128))));
    ctx.gpr[3] = 0x004A7124;
}

// T::~T: vtable 0x00B21BC0; bit 0 of the word at +184 of the body +28
// cleared (as a 64-bit store), body->vfn[40](body, 0) (its vtable read
// before the store); AnimSkel_Release(+32); vtable 0x00B21B58 at +84 and
// +48; then the base 0x009000A0(o)
DECOMP(004A948C, VtB21BC0_Dtor) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 b = load<u32>(o + 28);
    store<u32>(o, 0x00B21BC0);
    const u64 w = load<u64>(b + 184);
    const u32 vt = load<u32>(b);
    store<u64>(b + 184, w & ~(1ull << 32));
    VCallVt(ctx, vt, 40, {b, 0});
    call(ctx, o26fn::AnimSkel_Release, {load<u32>(o + 32)});
    store<u32>(o + 84, 0x00B21B58);
    store<u32>(o + 48, 0x00B21B58);
    call(ctx, 0x009000A0, {o});
}

// void f(Array* a, u32 v): removes the first v from the u32 array a (+4
// data, +12 u16 count) by moving the last element into its place
// (the original's tail copy for a reloaded count of 0 is kept)
DECOMP(004ACB90, Array_RemoveSwap_004ACB90) {
    const u32 a = arg<u32>(ctx, 0), v = arg<u32>(ctx, 1);
    const u32 n = load<u16>(a + 12), d = load<u32>(a + 4);
    for (u32 i = 0; i < n; ++i) {
        if (load<u32>(d + 4 * i) != v) continue;
        const u32 c = load<u16>(a + 12);
        const u32 last = load<u32>(d + 4 * (n - 1));
        store<u16>(a + 12, static_cast<u16>(c - 1));
        store<u32>(d + 4 * i, last);
        if (c - 1 != ((c - 1) & 0xFFFFu)) {
            const u32 d2 = load<u32>(a + 4);
            store<u32>(d2 + 4 * (c - 1), load<u32>(d2 + (((c - 1) << 2) & 0x3FFFCu)));
            ctx.gpr[3] = d2;
        }
        return;
    }
}

// void f(T* o, Effect* e): r = Effect_Bind4BB7E0(e, o->+12, 0); when set,
// WorldObj_ReleasePeers / WorldObj_AddToWorld(r->+256), then
// WorldObj_ForEach(r, 0x00F3A5C0, &(u8)(bit 0x80 of o->+16), 1)
DECOMP(004CC1BC, Effect_Attach_004CC1BC) {
    Stack frame(ctx, 144);
    const u32 o = arg<u32>(ctx, 0), e = arg<u32>(ctx, 1);
    const u32 r = static_cast<u32>(call(ctx, 0x004BB7E0, {e, load<u32>(o + 12), 0}));
    if (r == 0) return;
    call(ctx, 0x0057DA68, {load<u32>(r + 256)});
    call(ctx, 0x005A0394, {load<u32>(r + 256)});
    const u32 flag = frame.addr() + 112;
    store<u8>(flag, static_cast<u8>(load<u8>(o + 16) >> 7));
    call(ctx, 0x0057F5D8, {r, 0x00F3A5C0, flag, 1});
}

// T::T(const char* name, a, b, c, d): +32 = 0, +36 = c, +40 = a, +44 = d,
// +48 = b, +52..+64 and the u8s +68..+71 = 0; String_Assign(o, name); +32 =
// String_HashNoCase(o), +60 / +64 = 0, the u8s +69 / +68 = 1, +70 = 0
// (r3 = the hash, r4 = 1)
DECOMP(00767C20, NamedEntry_Init_00767C20) {
    const u32 o = arg<u32>(ctx, 0), name = arg<u32>(ctx, 1);
    const u32 a = arg<u32>(ctx, 2), b = arg<u32>(ctx, 3), c = arg<u32>(ctx, 4), d = arg<u32>(ctx, 5);
    store<u32>(o + 32, 0);
    store<u32>(o + 36, c);
    store<u32>(o + 40, a);
    store<u32>(o + 44, d);
    store<u32>(o + 48, b);
    for (u32 off = 52; off <= 64; off += 4) store<u32>(o + off, 0);
    for (u32 off = 68; off <= 71; ++off) store<u8>(o + off, 0);
    call(ctx, 0x00555F80, {o, name});
    const u32 h = static_cast<u32>(call(ctx, 0x0056A710, {o}));
    store<u32>(o + 32, h);
    store<u32>(o + 60, 0);
    store<u32>(o + 64, 0);
    store<u8>(o + 69, 1);
    store<u8>(o + 68, 1);
    store<u8>(o + 70, 0);
    ctx.gpr[3] = h;
    ctx.gpr[4] = 1;
}

// T::~T deleting form: vtables 0x00B1F404 / 0x00B1F434 (+8) / 0x00B1F44C
// (+12) / 0x00B1F464 (+16), HkBase_Dtor940F24(o), then the r13 pool
// (-28332)->+44->vfn[12](pool, o, the u16 +4). Null does nothing (r3 = 0).
DECOMP(0077153C, VtB1F404_Delete) {
    const u32 o = arg<u32>(ctx, 0);
    if (o == 0) {
        ctx.gpr[3] = 0;
        return;
    }
    store<u32>(o, 0x00B1F404);
    store<u32>(o + 8, 0x00B1F434);
    store<u32>(o + 12, 0x00B1F44C);
    store<u32>(o + 16, 0x00B1F464);
    call(ctx, 0x00940F24, {o});
    const u32 pool = Pool28332(ctx);
    VCall(ctx, pool, 12, {pool, o, load<u16>(o + 4)});
}

// ---------------------------------------------------------------- sweep: 39 instructions
// void f(T* o): with the script +224 = s: h = String_HashNoCase(the name
// s->+0->+12[the language index at 0x010AAF94]); 0x00014004(o, h) when
// nonzero; then Msg_Post(o->+48, the u32 0x00F65C54 twice, s->+4->+204)
DECOMP(00016088, Obj_PlayLocalized_00016088) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 s = load<u32>(o + 224);
    ctx.gpr[3] = 0;
    if (s == 0) return;
    const u32 name = load<u32>(load<u32>(load<u32>(s) + 12) + IndexOff(0x010AAF94));
    const u32 h = static_cast<u32>(call(ctx, 0x0056A710, {name}));
    if (h != 0) call(ctx, 0x00014004, {o, h});
    const u32 k = load<u32>(0x00F65C54);
    call(ctx, o24fn::Msg_Post, {load<u32>(o + 48), k, k, load<u32>(load<u32>(load<u32>(o + 224) + 4) + 204)});
}

// void f(WorldObj* o, T* p): when the tint 0x01164CA0 (4 bytes, copied to
// the stack) has a nonzero 4th byte, Obj_SubmitTintedDraws(o, p->+8, 1,
// &tint, 0x01164B30); then WorldObj_SetDrawCallback(o, 0, 0, 0, 0)
DECOMP(0001CDA4, Obj_DrawTinted_0001CDA4) {
    Stack frame(ctx, 128);
    const u32 o = arg<u32>(ctx, 0), p = arg<u32>(ctx, 1);
    const u32 t = frame.addr() + 112;
    for (u32 k = 0; k < 4; ++k) store<u8>(t + k, load<u8>(0x01164CA0 + k));
    if (load<u8>(t + 3) != 0) call(ctx, 0x00572420, {o, load<u32>(p + 8), 1, t, 0x01164B30});
    call(ctx, 0x00583C04, {o, 0, 0, 0, 0});
}

// u32 f(T* o, Msg* m): for the message 0x03DE9A54, o->vfn[220](o, 12)
// unless +4 is already 12; then 0x000133FC(o, m)
DECOMP(00027D50, Obj_OnMsg_00027D50) {
    const u32 o = arg<u32>(ctx, 0), m = arg<u32>(ctx, 1);
    const s32 k = load<s32>(m + 8);
    if (k > static_cast<s32>(0xDD92E72Bu) && k == 0x03DE9A54 && load<s32>(o + 4) != 12) VCall(ctx, o, 220, {o, 12});
    ctx.gpr[3] = call(ctx, 0x000133FC, {o, m});
}

// void f(T* o): unless the target +256 has bit 0x400 of +200, r =
// 0x0001AC80(0x00F557F0->*, o) and o->vfn[284](o, r) when r is set and
// differs from +256
DECOMP(0002F900, Obj_Retarget_0002F900) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 s = load<u32>(o + 256);
    if (s != 0 && (load<u32>(s + 200) & 0x400u) != 0) {
        ctx.gpr[3] = 0x400;
        return;
    }
    const u32 r = static_cast<u32>(call(ctx, 0x0001AC80, {load<u32>(0x00F557F0), o}));
    if (r == 0 || r == load<u32>(o + 256)) return;
    VCall(ctx, o, 284, {o, r});
}

// Object* f(?, u32 id, Effect* e, X* x): the +280 of Effect_Bind4BB7E0(x, e,
// 0) when it has kind (+276) 0x02235C33 and that is set; else
// Obj_FindById(id) for a nonzero id; else 0
DECOMP(0009A284, Obj_ResolveTarget_0009A284) {
    const u32 id = arg<u32>(ctx, 1), e = arg<u32>(ctx, 2), x = arg<u32>(ctx, 3);
    u32 r = 0;
    if (e != 0) {
        const u32 b = static_cast<u32>(call(ctx, 0x004BB7E0, {x, e, 0}));
        if (b != 0 && load<u32>(b + 276) == 0x02235C33u) r = load<u32>(b + 280);
    }
    if (r == 0 && id != 0) r = static_cast<u32>(call(ctx, 0x000D6A28, {id}));
    ctx.gpr[3] = r;
}

// void f(T* o): with +20 set, pushes a new 24-byte VtB20CE0 command
// (0x0042CC10(p, the u8 +12, +20, f1 = +24)); else Hash_ToString(+16)
DECOMP(0009A4F8, Obj_PushNewB20CE0_0009A4F8) {
    const u32 o = arg<u32>(ctx, 0);
    if (load<u32>(o + 20) == 0) {
        call(ctx, o27fn::Hash_ToString, {load<u32>(o + 16)});
        return;
    }
    PushNew26(ctx, 24, [&](u32 p) {
        callf(ctx, 0x0042CC10, {p, load<u8>(o + 12), load<u32>(o + 20)}, {static_cast<f64>(load<f32>(o + 24))});
    });
}

// void f(T* o): pushes a new 128-byte 0x00430A64 command (p, +12, +16, f1..f4
// = +20..+32, +36 in r10, f5 = +40, the 10th argument 1; r6 = 1 and r7..r9
// as left)
DECOMP(0009F360, Obj_PushNew430A64_0009F360) {
    const u32 o = arg<u32>(ctx, 0);
    PushNew26(ctx, 128, [&](u32 p) {
        auto f = [&](u32 off) { return static_cast<f64>(load<f32>(o + off)); };
        // slot 8 (the stack word at 112) belongs to f5 and is not written
        callf(ctx, 0x00430A64, {p, load<u32>(o + 12), load<u32>(o + 16), 1, ctx.gpr[7], ctx.gpr[8], ctx.gpr[9], load<u32>(o + 36), 0, 1},
              {f(20), f(24), f(28), f(32), f(40)});
    });
}

// void f(T* o): with +12 set, pushes a new 24-byte 0x00438D14(p, +12, +16,
// the u8s +20..+24) command; else r3 = 0
DECOMP(000A52D0, Obj_PushNew438D14_000A52D0) {
    const u32 o = arg<u32>(ctx, 0);
    if (load<u32>(o + 12) == 0) {
        ctx.gpr[3] = 0;
        return;
    }
    PushNew26(ctx, 24, [&](u32 p) {
        call(ctx, 0x00438D14, {p, load<u32>(o + 12), load<u32>(o + 16), load<u8>(o + 20), load<u8>(o + 21), load<u8>(o + 22), load<u8>(o + 23), load<u8>(o + 24)});
    });
}

// T* f(const T* o): a copy of the 24-byte VtB11B38 emitter handle (u16 +4 =
// 1, +8 / +20 / u16 +22 = 0, then +20 and +8 copied). A failed SmallAlloc
// still writes the copies to 20 and 8, as in the original.
DECOMP(000A775C, VtB11B38_Clone) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 p = static_cast<u32>(call(ctx, o19fn::SmallAlloc, {24}));
    if (p != 0) {
        store<u32>(p + 20, 0);
        store<u16>(p + 4, 1);
        store<u16>(p + 22, 0);
        store<u32>(p, 0x00B11B38);
        store<u32>(p + 8, 0);
        store<u32>(p + 20, 0);
    }
    store<u32>(p + 20, load<u32>(o + 20));
    store<u32>(p + 8, load<u32>(o + 8));
    ctx.gpr[3] = p;
}

// T* f(const Vec3* pos): the entry of the list 0x01013B34 (+0 data, +8 u16
// count) whose position (+144) is nearest to pos, closer than the
// constant at 0x000D6B60 (squared); else 0
DECOMP(000D6B68, Spawn_FindNearest_000D6B68) {
    const u32 p = arg<u32>(ctx, 0);
    f32 best = load<f32>(0x000D6B60);
    const u32 d = load<u32>(0x01013B34), n = load<u16>(0x01013B3C);
    u32 r = 0;
    for (u32 i = 0; i < n; ++i) {
        const u32 e = load<u32>(d + 4 * i);
        const u32 v = e + 144;
        const f32 dy = static_cast<f32>(static_cast<f64>(load<f32>(v + 4)) - static_cast<f64>(load<f32>(p + 4)));
        const f32 dx = static_cast<f32>(static_cast<f64>(load<f32>(v)) - static_cast<f64>(load<f32>(p)));
        const f32 dz = static_cast<f32>(static_cast<f64>(load<f32>(v + 8)) - static_cast<f64>(load<f32>(p + 8)));
        f32 s = static_cast<f32>(static_cast<f64>(dy) * static_cast<f64>(dy));
        s = static_cast<f32>(std::fma(static_cast<f64>(dx), static_cast<f64>(dx), static_cast<f64>(s)));
        s = static_cast<f32>(std::fma(static_cast<f64>(dz), static_cast<f64>(dz), static_cast<f64>(s)));
        ctx.fpr[1] = static_cast<f64>(s);
        if (best > s) {
            r = e;
            best = s;
        }
    }
    ctx.fpr[2] = static_cast<f64>(best);
    ctx.gpr[3] = r;
}

// void f(T* o, const Matrix* m): o->+96.. = the 3x4 part of m (rows of 3
// from +0 / +16 / +32 / +48), then o->vfn[288](o) with f1 = the last
// value (the entry in r4 and the OPD in r5 here)
DECOMP(000DEF4C, Obj_SetXform_000DEF4C) {
    const u32 o = arg<u32>(ctx, 0), m = arg<u32>(ctx, 1);
    u32 w = 0;
    for (u32 r = 0; r < 4; ++r)
        for (u32 c = 0; c < 3; ++c) {
            w = load<u32>(m + 16 * r + 4 * c);
            store<u32>(o + 96 + 16 * r + 4 * c, w);
        }
    const u32 opd = load<u32>(load<u32>(o) + 288);
    ctx.fpr[1] = static_cast<f64>(std::bit_cast<f32>(w));
    call_ptr(ctx, opd, {o, load<u32>(opd), opd});
}

// void f(T* o, const u32* src, u32 n): when src is set and +372 is unset,
// +372 = Mem_Alloc(4n, 0, 16) and the n words of src are copied in (r3 = 4n
// after a copy, the buffer for n = 0, else o or +372 as left)
DECOMP(000DF330, Obj_CopyWords372_000DF330) {
    const u32 o = arg<u32>(ctx, 0), src = arg<u32>(ctx, 1), n = arg<u32>(ctx, 2);
    if (src == 0) return;  // r3 = o
    const u32 cur = load<u32>(o + 372);
    ctx.gpr[3] = cur;
    if (cur != 0) return;
    const u32 buf = static_cast<u32>(call(ctx, kMemAlloc, {n << 2, 0, 16}));
    store<u32>(o + 372, buf);
    if (n == 0) return;
    s32 k = static_cast<s32>(n);
    u32 i = 0;
    do {
        store<u32>(load<u32>(o + 372) + i, load<u32>(src + i));
        i += 4;
        --k;
    } while (k > 0);
    ctx.gpr[3] = i;
}

// T::~T deleting form: vtable 0x00B13C18; with the skeleton +76 = s: when
// s->+8 is set, Body_Remove931F00(s->+8, s); AnimSkel_Release(+76), +76 = 0;
// vtable 0x00B0DAEC, SmallFree(o, 96). Null does nothing (r3 = 0).
DECOMP(00107698, VtB13C18_Delete) {
    const u32 o = arg<u32>(ctx, 0);
    if (o == 0) {
        ctx.gpr[3] = 0;
        return;
    }
    const u32 s = load<u32>(o + 76);
    store<u32>(o, 0x00B13C18);
    if (s != 0) {
        u32 r = s;
        if (load<u32>(s + 8) != 0) {
            call(ctx, 0x00931F00, {load<u32>(s + 8), s});
            r = load<u32>(o + 76);
        }
        call(ctx, o26fn::AnimSkel_Release, {r});
        store<u32>(o + 76, 0);
    }
    store<u32>(o, 0x00B0DAEC);
    call(ctx, o19fn::SmallFree, {o, 96});
}

// T::~T for this = o + 384 (VtB16B48DtorAt; no null check)
DECOMP(001C3688, VtB16B48_DtorAdj384) { VtB16B48DtorAt(ctx, arg<u32>(ctx, 0) - 384); }
// T::~T for this = o + 380 (VtB16B48DtorAt; no null check)
DECOMP(001C3724, VtB16B48_DtorAdj380) { VtB16B48DtorAt(ctx, arg<u32>(ctx, 0) - 380); }

// T::~T deleting form: the u16 +820 = 0, vtables 0x00B17270 / 0x00B17444
// (+380) / 0x00B1746C (+384), ~PtrVector(+812), the base destructor
// 0x001AE468, SmallFree(o, 864). Null does nothing (r3 = 0).
DECOMP(001CBE90, VtB17270_Delete) {
    const u32 o = arg<u32>(ctx, 0);
    if (o == 0) {
        ctx.gpr[3] = 0;
        return;
    }
    store<u16>(o + 820, 0);
    store<u32>(o, 0x00B17270);
    store<u32>(o + 380, 0x00B17444);
    store<u32>(o + 384, 0x00B1746C);
    VecDtor(ctx, o + 812);
    call(ctx, 0x001AE468, {o});
    call(ctx, o19fn::SmallFree, {o, 864});
}

// void f(T* o, Msg* m): 0x001CEC8C(o) when o has a target +216 whose
// f32 +240 is above the constant at 0x001CED8C, Obj_TestE0BCC(o, m) holds,
// m->+4 is not o and the u8 +854 is set (r3 as the original leaves it on
// the other paths)
DECOMP(001CED90, Obj_OnMsgTarget_001CED90) {
    const u32 o = arg<u32>(ctx, 0), m = arg<u32>(ctx, 1);
    const u32 t = load<u32>(o + 216);
    if (t == 0) return;  // r3 = o
    if (static_cast<s32>(call(ctx, 0x000E0BCC, {o, m})) == 0) {
        ctx.gpr[3] = 0;
        return;
    }
    const u32 s = load<u32>(m + 4);
    if (s == o) {
        ctx.gpr[3] = s;
        return;
    }
    const f64 f = static_cast<f64>(load<f32>(t + 240)), k = static_cast<f64>(load<f32>(0x001CED8C));
    ctx.fpr[1] = f;
    ctx.fpr[2] = k;
    if (f <= k) {
        ctx.gpr[3] = 0x001CED8C;
        return;
    }
    if (load<u8>(o + 854) == 0) {
        ctx.gpr[3] = 0;
        return;
    }
    call(ctx, 0x001CEC8C, {o, ctx.gpr[4]});  // r4 as Obj_TestE0BCC left it
}

// void f(T* o): the two objects +828 / +832 get vfn[40](it, 1) and are
// cleared, each when set
DECOMP(001EDB90, Obj_ReleasePair828_001EDB90) {
    const u32 o = arg<u32>(ctx, 0);
    for (u32 i = 0; i < 2; ++i) {
        const u32 p = load<u32>(o + 828 + 4 * i);
        ctx.gpr[3] = p;
        if (p == 0) continue;
        VCall(ctx, p, 40, {p, 1});
        store<u32>(o + 828 + 4 * i, 0);
    }
}

// u32 f(T* o, u8 a, u8 b, u8 c, u8 d): 0x000D9F1C(o, a, b, c, d); when a is
// 0, clears bit 0x20000000 of +980 and releases (vfn[40](p, 0)) and clears
// +872 (r3 = 0)
DECOMP(001F3FEC, Obj_SetMode_001F3FEC) {
    const u32 o = arg<u32>(ctx, 0), a = arg<u32>(ctx, 1) & 0xFFu;
    call(ctx, 0x000D9F1C, {o, a, arg<u32>(ctx, 2) & 0xFFu, arg<u32>(ctx, 3) & 0xFFu, arg<u32>(ctx, 4) & 0xFFu});
    if (a != 0) return;
    const u32 w = load<u32>(o + 980) & ~0x20000000u;
    const u32 p = load<u32>(o + 872);
    store<u32>(o + 980, w);
    if (p != 0) VCall(ctx, p, 40, {p, 0});
    store<u32>(o + 872, 0);
    ctx.gpr[3] = 0;
}

// void f(T* o, Query* q): 0x000DFBFC(o, q), then q->+208.. = o->+240..
// (+240..+248, +256..+264, +272..+280 to +208..+216, +224..+232,
// +240..+248), bit 0x1000 of q->+20 = bit 0x40000000 of o->+132, the u8
// q->+252 = o->+284 (r3 / r4 / f1 as left)
DECOMP(001FAD7C, Obj_CopyQuery_001FAD7C) {
    const u32 o = arg<u32>(ctx, 0), q = arg<u32>(ctx, 1);
    call(ctx, 0x000DFBFC, {o, q});
    store<u32>(q + 208, load<u32>(o + 240));
    store<u32>(q + 212, load<u32>(o + 244));
    store<u32>(q + 216, load<u32>(o + 248));
    store<u32>(q + 224, load<u32>(o + 256));
    store<u32>(q + 228, load<u32>(o + 260));
    const u32 f264 = load<u32>(o + 264), w272 = load<u32>(o + 272);
    store<u32>(q + 232, f264);
    const u32 f276 = load<u32>(o + 276);
    store<u32>(q + 240, w272);
    store<u32>(q + 244, f276);
    const u32 f280 = load<u32>(o + 280);
    const u32 fl = load<u32>(q + 20);
    store<u32>(q + 248, f280);
    const u32 b = load<u32>(o + 132);
    store<u32>(q + 20, (fl & ~0x1000u) | ((b >> 18) & 0x1000u));
    const u8 c = load<u8>(o + 284);
    store<u8>(q + 252, c);
    ctx.gpr[3] = c;
    ctx.gpr[4] = b;
    ctx.fpr[1] = static_cast<f64>(std::bit_cast<f32>(f280));
}

// bool f(T* o, ?, a, b, f32 v): true when v is at most the constant at
// 0x001FC4C0; else 0x000E1738(o, a, a, b, v), bit 0x400 of the u16 +432
// set when the f32 +348 now equals the constant, and whether +348 is above
// it (f1 = +348)
DECOMP(001FC4C8, Obj_Damage_001FC4C8) {
    const u32 o = arg<u32>(ctx, 0), a = arg<u32>(ctx, 2), b = arg<u32>(ctx, 3);
    const f64 v = farg(ctx, 0), k = static_cast<f64>(load<f32>(0x001FC4C0));
    if (v <= k) {
        ctx.gpr[3] = 1;
        return;
    }
    callf(ctx, 0x000E1738, {o, a, a, b}, {v});
    const f64 h = static_cast<f64>(load<f32>(o + 348));
    if (h == k) store<u16>(o + 432, static_cast<u16>(load<u16>(o + 432) | 0x400u));
    ctx.fpr[1] = h;
    ctx.gpr[3] = h > k ? 1u : 0u;
}

// Node* f(?, Node* n, s32 key): n when n->+208->+100 is key, else the
// match in n->+268 (searched first) or n->+272 (recursively); 0 for none
DECOMP(00280A0C, Node_FindByKey_00280A0C) {
    const u32 c = arg<u32>(ctx, 0), n = arg<u32>(ctx, 1), key = arg<u32>(ctx, 2);
    if (n == 0) {
        ctx.gpr[3] = 0;
        return;
    }
    if (load<u32>(load<u32>(n + 208) + 100) == key) {
        ctx.gpr[3] = n;
        return;
    }
    u32 r = static_cast<u32>(call(ctx, 0x00280A0C, {c, load<u32>(n + 268), key}));
    if (r == 0) r = static_cast<u32>(call(ctx, 0x00280A0C, {c, load<u32>(n + 272), key}));
    ctx.gpr[3] = r;
}

// void f(?, Args* p): x = p->+0->+24: x->vfn[40](x, 1, 1), 0x002BB768(x,
// p->+4), x->vfn[40](x, 0, 1)
DECOMP(002BB960, Obj_Refresh_002BB960) {
    const u32 p = arg<u32>(ctx, 1);
    const u32 x = load<u32>(load<u32>(p) + 24);
    VCall(ctx, x, 40, {x, 1, 1});
    call(ctx, 0x002BB768, {x, load<u32>(p + 4)});
    VCall(ctx, x, 40, {x, 0, 1});
}

// bool f(Job* o): with a path +12, +40 = Fios_OpenAsync(path, 0x00AD9160,
// 1, 0, 0) and the state +32 = 1 (opened) or 6 (failed); then the u8 +28 =
// 1. Returns 1.
DECOMP(002E84FC, File_OpenJob_002E84FC) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 path = load<u32>(o + 12);
    if (path != 0) {
        const u32 r = static_cast<u32>(call(ctx, 0x00561714, {path, 0x00AD9160, 1, 0, 0}));
        store<u32>(o + 40, r);
        store<u32>(o + 32, r != 0 ? 1u : 6u);
    }
    store<u8>(o + 28, 1);
    ctx.gpr[3] = 1;
}

// void f(Table* t, u32 h): the live object of handle h in t (+24 count,
// +28 16-byte entries: +8 the object, +12 bit 0x80000000 free and bits
// 15..30 the serial) gets vfn[20](obj) (r3 as the original leaves it)
DECOMP(002F20A4, Handle_Release_002F20A4) {
    const u32 t = arg<u32>(ctx, 0), h = arg<u32>(ctx, 1);
    const u32 serial = h & 0xFFFFu, idx = h >> 16;
    if (serial == 0 || static_cast<s32>(idx) >= load<s32>(t + 24)) return;  // r3 = t
    const u32 e = load<u32>(t + 28) + (idx << 4);
    ctx.gpr[3] = e;
    const u32 w = load<u32>(e + 12);
    if (((w >> 15) & 0xFFFFu) != serial || (w & 0x80000000u) != 0) return;
    const u32 obj = load<u32>(e + 8);
    VCall(ctx, obj, 20, {obj});
}

// void f(T* o): with c = Effect_FindComponent(o->+8->+52, 0xE9BEF0B6),
// c->+20 = the f32 o->+12 when c->vfn[32](c) is 1
DECOMP(002F96F0, Obj_SetComponentF20_002F96F0) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 a = load<u32>(o + 8);
    if (a == 0) return;  // r3 = o
    const u32 b = load<u32>(a + 52);
    const u32 f = load<u32>(o + 12);
    if (b == 0) return;
    const u32 c = static_cast<u32>(call(ctx, 0x004BB4B4, {b, 0xE9BEF0B6u}));
    if (c == 0) return;
    if (static_cast<s32>(VCall(ctx, c, 32, {c})) == 1) store<u32>(c + 20, f);
}

// void f(char** dst, const char* s): frees *dst (Mem_FreeFrom) and clears
// it; then for a non-empty s, *dst = a Mem_Alloc(strlen(s) + 1, 0, 16) copy
DECOMP(00321288, String_Replace_00321288) {
    const u32 d = arg<u32>(ctx, 0), s = arg<u32>(ctx, 1);
    if (load<u32>(d) != 0) {
        call(ctx, o25fn::Mem_FreeFrom, {load<u32>(d), 0});
        store<u32>(d, 0);
    }
    ctx.gpr[3] = 0;
    if (s == 0 || load<u8>(s) == 0) return;
    const u32 n = static_cast<u32>(call(ctx, 0x0071ED08, {s})) + 1;
    const u32 p = static_cast<u32>(call(ctx, kMemAlloc, {n, 0, 16}));
    store<u32>(d, p);
    call(ctx, 0x007239D8, {p, s});
}

// u32 f(T* o): 0 unless the u8 +18 is 4; the first nonzero vfn[68](c) of
// the kind-4 entries in the chain +2576 (next at +2564); else +2208
DECOMP(00327EDC, Obj_FindActive_00327EDC) {
    const u32 o = arg<u32>(ctx, 0);
    if (load<u8>(o + 18) != 4) {
        ctx.gpr[3] = 0;
        return;
    }
    for (u32 c = load<u32>(o + 2576); c != 0; c = load<u32>(c + 2564)) {
        if (load<u8>(c + 18) != 4) continue;
        const u32 r = static_cast<u32>(VCall(ctx, c, 68, {c}));
        if (r != 0) {
            ctx.gpr[3] = r;
            return;
        }
    }
    ctx.gpr[3] = load<u32>(o + 2208);
}

// void f(T* o, s32 k, u8 force): unless k is 4 or (without force) already
// the u8 +18, o->vfn[28](o, 0) first; then 0x003260F0(o, k, force)
static void SetKind27(PPUContext& ctx) {
    const u32 o = arg<u32>(ctx, 0), k = arg<u32>(ctx, 1), f = arg<u32>(ctx, 2) & 0xFFu;
    if ((f != 0 || k != load<u8>(o + 18)) && static_cast<s32>(k) != 4) VCall(ctx, o, 28, {o, 0});
    call(ctx, 0x003260F0, {o, k, f});
}
// void f(T* o, s32 k, u8 force): SetKind27 (one of two identical copies)
DECOMP(0032F9D8, Obj_SetKind_0032F9D8) { SetKind27(ctx); }
// void f(T* o, s32 k, u8 force): SetKind27 (the other copy)
DECOMP(00330670, Obj_SetKind_00330670) { SetKind27(ctx); }
// void f(T* o, s32 k, u8 force): SetKind27 (a third copy)
DECOMP(00332D58, Obj_SetKind_00332D58) { SetKind27(ctx); }

// void f(T* o, u8 a, b): when a and the u8 +2557 are set, +2660 = 0 and
// o->vfn[36](o, +2568); then 0x0032549C(o, a, b)
DECOMP(0033F834, Obj_Start_0033F834) {
    const u32 o = arg<u32>(ctx, 0), a = arg<u32>(ctx, 1) & 0xFFu, b = arg<u32>(ctx, 2);
    if (a != 0 && load<u8>(o + 2557) != 0) {
        store<u32>(o + 2660, 0);
        VCall(ctx, o, 36, {o, load<u32>(o + 2568)});
    }
    call(ctx, 0x0032549C, {o, a, b});
}

// void f(T* o, u32 v): removes the first v from the array +32 (+36 s32
// count) by moving the last element into its place (r3 as the original
// leaves it)
DECOMP(00346698, Array_RemoveSwap_00346698) {
    const u32 o = arg<u32>(ctx, 0), v = arg<u32>(ctx, 1);
    if (v == 0) return;  // r3 = o
    const s32 n = load<s32>(o + 36);
    const u32 d = load<u32>(o + 32);
    for (s32 i = 0; i < n; ++i) {
        if (load<u32>(d + 4 * static_cast<u32>(i)) != v) continue;
        const s32 m = n - 1;
        store<s32>(o + 36, m);
        ctx.gpr[3] = static_cast<u32>(m);
        if (m == i) return;
        store<u32>(d + 4 * static_cast<u32>(i), load<u32>(d + 4 * static_cast<u32>(m)));
        ctx.gpr[3] = 1;
        return;
    }
}

// void f(T* o, s32 i, u8 b): bit 0x00800000 of +532 = bit 0 of
// Speaker_SayId4A5C90(+28, o->+164[i], b, f1 = the constant at
// 0x003686C8); with the effect +20, bit 0x00400000 = bit 0 of
// Effect_SetHash(+20, the hash 0x00F64FB0[i])
DECOMP(003686D0, Obj_Say_003686D0) {
    const u32 o = arg<u32>(ctx, 0), i = arg<u32>(ctx, 1), b = arg<u32>(ctx, 2) & 0xFFu;
    callf(ctx, 0x004A5C90, {load<u32>(o + 28), load<u32>(o + 164 + (i << 2)), b}, {static_cast<f64>(load<f32>(0x003686C8))});
    const u32 r = static_cast<u32>(ctx.gpr[3]);
    const u32 w = (load<u32>(o + 532) & ~0x00800000u) | ((r & 1u) << 23);
    const u32 e = load<u32>(o + 20);
    store<u32>(o + 532, w);
    ctx.gpr[3] = e;
    if (e == 0) return;
    const u32 r2 = static_cast<u32>(call(ctx, o27fn::Effect_SetHash, {e, load<u32>(0x00F64FB0 + (i << 2))}));
    store<u32>(o + 532, (load<u32>(o + 532) & ~0x00400000u) | ((r2 & 1u) << 22));
}

// u32 f(T*, u8 on): with the global 0x00F64CBC = g set: bit 0x20 of the u8
// g->+192 = bit 0 of on; when g->+24 has bit 0x8000 at +164 and g->+20 = e
// is set, Effect_SetHash(e, on & 1 ? 0xDBDDDABD : 0x009BD94D); else returns
// r4 as passed
DECOMP(00369874, Effect_SetOnOffHash_00369874) {
    const u32 on = arg<u32>(ctx, 1);
    ctx.gpr[3] = on;
    const u32 g = load<u32>(0x00F64CBC);
    if (g == 0) return;
    const u8 b = load<u8>(g + 192);
    const u32 p = load<u32>(g + 24);
    store<u8>(g + 192, static_cast<u8>((b & ~0x20u) | ((on << 5) & 0x20u)));
    if (p == 0 || (load<u32>(p + 164) & 0x8000u) == 0) return;
    const u32 e = load<u32>(g + 20);
    if (e == 0) return;
    ctx.gpr[3] = call(ctx, o27fn::Effect_SetHash, {e, (on & 1u) != 0 ? 0xDBDDDABDu : 0x009BD94Du});
}

// void f(?, T* o): 0x00370A0C(o, e->+4, the u8 e->+8) for each 12-byte
// entry e of o->+164[0..u16 +172) with +4 set (r3 = the count at the end)
DECOMP(00370AE4, Obj_ApplyEntries_00370AE4) {
    const u32 o = arg<u32>(ctx, 1);
    if (o == 0) return;  // r3 as passed
    u32 n = load<u16>(o + 172);
    for (u32 i = 0; i < n; ++i) {
        const u32 e = load<u32>(o + 164) + 12 * i;
        if (load<u32>(e + 4) == 0) continue;
        call(ctx, 0x00370A0C, {o, load<u32>(e + 4), load<u8>(e + 8)});
        n = load<u16>(o + 172);
    }
    ctx.gpr[3] = n;
}

// void f(T* o, u8 paused): Obj_SetPausedBit_00490F88(o, paused); then for
// the handle o->+96->+88: Handle_Notify36742C(h, 1, 1, 0) when unpaused,
// else 0x0036735C(h, 0, 1, o->+96, 0, 0, 0x01164CA4, f1 = the constant at
// 0x003B6F60)
DECOMP(003B6F68, Obj_SetPaused_003B6F68) {
    const u32 o = arg<u32>(ctx, 0), a = arg<u32>(ctx, 1) & 0xFFu;
    call(ctx, 0x00490F88, {o, a});
    const u32 q = load<u32>(o + 96);
    if (a == 0) {
        call(ctx, 0x0036742C, {load<u32>(q + 88), 1, 1, 0});
        return;
    }
    callf(ctx, 0x0036735C, {load<u32>(q + 88), 0, 1, q, 0, 0, 0x01164CA4}, {static_cast<f64>(load<f32>(0x003B6F60))});
}

// void CmdList_Push(Cmd* c): with the current command list 0x00F6585C->* =
// l: c->+4 = l (also for a null c, as in the original) and c pushed onto
// l's PtrVector +4 (+8 u16 capacity, doubled when full; +12 count, +14 the
// high-water mark). r3 = the old mark (c without a list).
DECOMP(0042C138, CmdList_Push_0042C138) {
    const u32 c = arg<u32>(ctx, 0);
    const u32 l = load<u32>(0x00F6585C);
    if (l == 0) return;
    store<u32>(c + 4, l);
    const u32 cap = load<u16>(l + 8);
    if (load<u16>(l + 12) == cap) call(ctx, o27fn::PtrVector_Reserve, {l + 4, (cap << 1) & 0xFFFFFFFEu});
    store<u32>(load<u32>(l + 4) + (static_cast<u32>(load<u16>(l + 12)) << 2), c);
    const u32 n = load<u16>(l + 12) + 1;
    const u32 m = load<u16>(l + 14);
    store<u16>(l + 12, static_cast<u16>(n));
    store<u16>(l + 14, static_cast<u16>(static_cast<s32>(m) > static_cast<s32>(n & 0xFFFF) ? m : n));
    ctx.gpr[3] = m;
    ctx.gpr[4] = n & 0xFFFF;
}

// T::T(a, b, c, d, f32 f): base 0x0042C9D0(o, 1), +12..+24 = a..d, +28 = f,
// vtable 0x00B21118, +32 / the u8 +36 = 0 (r3 = the vtable, r4 = 0)
DECOMP(00434A10, VtB21118_Ctor) {
    const u32 o = arg<u32>(ctx, 0), a = arg<u32>(ctx, 1), b = arg<u32>(ctx, 2), c = arg<u32>(ctx, 3), d = arg<u32>(ctx, 4);
    const f64 f = farg(ctx, 0);
    CmdBase27(ctx, o, 1);
    store<u32>(o + 12, a);
    store<u32>(o + 16, b);
    store<u32>(o + 20, c);
    store<u32>(o + 24, d);
    store<f32>(o + 28, static_cast<f32>(f));
    store<u32>(o, 0x00B21118);
    store<u32>(o + 32, 0);
    store<u8>(o + 36, 0);
    ctx.gpr[3] = 0x00B21118;
    ctx.gpr[4] = 0;
}

// void f(T* o): +40 = 0; each 16-byte entry of o->+80[0..u16 +88) gets
// +0 = Obj_FindByHandle42C2B0(its +4, 0); then 0x00435708(o) and
// Flag_Set8_40000000(o)
DECOMP(004358D0, Cmd_ResolveAll_004358D0) {
    const u32 o = arg<u32>(ctx, 0);
    store<u32>(o + 40, 0);
    for (u32 i = 0; i < load<u16>(o + 88); ++i) {
        const u32 r = static_cast<u32>(call(ctx, 0x0042C2B0, {load<u32>(load<u32>(o + 80) + 16 * i + 4), 0}));
        store<u32>(load<u32>(o + 80) + 16 * i, r);
    }
    call(ctx, 0x00435708, {o});
    ctx.gpr[3] = call(ctx, o27fn::Flag_Set8_40000000, {o});
}

// void f(T* o, Msg* m): the message 0x2BB3C007 sets the u8 +144; 0x4FE5ADE2
// clears it and sets +145; others warn (format 0x00AE2608, the hash name)
DECOMP(004378DC, Cmd_OnMsg_004378DC) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 k = load<u32>(arg<u32>(ctx, 1) + 8);
    if (k == 0x2BB3C007u) {
        store<u8>(o + 144, 1);
        ctx.gpr[4] = 1;
        return;
    }
    if (k == 0x4FE5ADE2u) {
        store<u8>(o + 144, 0);
        store<u8>(o + 145, 1);
        ctx.gpr[4] = 0;
        ctx.gpr[5] = 1;
        return;
    }
    WarnHash27(ctx, 0x00AE2608, k);
}

// T::~T deleting form: vtable 0x00B21988; SmallFree(+80, +84) when set,
// +84 / +80 = 0; Mem_FreeFrom(+20, 0) when set, +20 = 0; the base
// 0x0048F6B0, Mem_FreeFrom(o, 0). Null does nothing (r3 = 0).
DECOMP(004559F8, VtB21988_Delete) {
    const u32 o = arg<u32>(ctx, 0);
    if (o == 0) {
        ctx.gpr[3] = 0;
        return;
    }
    const u32 a = load<u32>(o + 80);
    store<u32>(o, 0x00B21988);
    if (a != 0) call(ctx, o19fn::SmallFree, {a, load<u32>(o + 84)});
    const u32 b = load<u32>(o + 20);
    store<u32>(o + 84, 0);
    store<u32>(o + 80, 0);
    if (b != 0) {
        call(ctx, o25fn::Mem_FreeFrom, {b, 0});
        store<u32>(o + 20, 0);
    }
    call(ctx, 0x0048F6B0, {o});
    call(ctx, o25fn::Mem_FreeFrom, {o, 0});
}
