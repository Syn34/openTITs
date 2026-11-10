// Game code, part 28: the continued sweep of small still-recompiled game
// functions (in size order, from 41 instructions up). Most have no known
// caller context yet, so their names describe what they do and carry the
// address. Each was checked against the original in the PPC simulator.
//
// Register notes: where a known caller reads a volatile register the
// original leaves behind (regflow liveout), the value is set the same way,
// as the comments say ("r4 left ...").
#include "obj28.h"

using namespace sly4;
using namespace sly4::obj28;

// ---------------------------------------------------------------- sweep: 41 instructions
// void f(T* o, f32 t, U* u): when t > *0x0003CEF8 and bit 0x1000 of the
// u16 u->+14 is set, o->vt[220](o, 4), then, when o->+312 is set,
// Queue_Push(o->+48, o->+312, 0x59C04B13, 0, 1)
DECOMP(0003CF00, Obj_OnHitPush_0003CF00) {
    const u32 o = arg<u32>(ctx, 0), u = arg<u32>(ctx, 2);
    if (!(farg(ctx, 0) > LfsAt(0x0003CEF8))) return;
    if ((load<u16>(u + 14) & 0x1000u) == 0) {
        ctx.gpr[3] = 0;
        return;
    }
    VCall(ctx, o, 220, {o, 4});
    const u32 it = load<u32>(o + 312);
    ctx.gpr[3] = it;
    if (it == 0) return;
    ctx.gpr[3] = call(ctx, o28fn::Queue_Push, {load<u32>(o + 48), it, 0x59C04B13, 0, 1});
}

// T* f(?, XmlCursor* c): new 16-byte record (base 0x004BB3B0(p, c), vtable
// 0x00B0F4C8) whose u8 +12 is read from the bool 0x7A281983
DECOMP(00072BAC, NewXmlBool_00072BAC) { NewXmlBool28(ctx, o28fn::Obj_InitVt_B21C98, true, 0x00B0F4C8, 0x7A281983); }

// void f(T* o, U* u): when the u8 o->+84 and the global 0x00F64C20->+40 = g
// are set, 0x002D9994(g, &o->+12), then (unless the u8 +88 is set and the
// u8 +85 clear) Effect_AddArg49F080(u, &o->+12, 0, 0, 0)
DECOMP(0007AE74, Obj_AddRecordArg_0007AE74) {
    const u32 o = arg<u32>(ctx, 0), u = arg<u32>(ctx, 1);
    ctx.gpr[3] = load<u8>(o + 84);
    if (ctx.gpr[3] == 0) return;
    const u32 g = load<u32>(load<u32>(0x00F64C20) + 40);
    ctx.gpr[3] = g;
    if (g == 0) return;
    call(ctx, 0x002D9994, {g, o + 12});
    if (load<u8>(o + 88) != 0 && load<u8>(o + 85) == 0) {
        ctx.gpr[3] = 0;
        return;
    }
    ctx.gpr[3] = call(ctx, 0x0049F080, {u, o + 12, 0, 0, 0});
}

// void f(T* o): the world slot o->+12 (records of 0x20C80 bytes from
// kWorldPtr->* + 4208): q = slot->+8 when its bit 0x8 of +192 is set (else
// 0), then Obj_Reset84_0027C8C8(q->+380 when that is a kind-5 record (u32
// +12) else 0, the u8 o->+16). Quirk: with q = 0 it reads the word at 380.
DECOMP(00082EB4, World_ResetSlot_00082EB4) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 slot = load<u32>(kWorldPtr) + load<u32>(o + 12) * 0x20C80u + 4208;
    const u32 b = load<u8>(o + 16);
    u32 q = load<u32>(slot + 8);
    if (q != 0 && (load<u32>(q + 192) & 0x8u) == 0) q = 0;
    const u32 p = load<u32>(q + 380);
    const u32 r = (p != 0 && load<s32>(p + 12) == 5) ? p : 0;
    ctx.gpr[3] = call(ctx, 0x0027C8C8, {r, b});
}

// T* f(?, XmlCursor* c): new 16-byte record (base 0x004BB3B0(p, c), vtable
// 0x00B10530) whose u8 +12 is read from the bool 0x7A281983
DECOMP(0008AE2C, NewXmlBool_0008AE2C) { NewXmlBool28(ctx, o28fn::Obj_InitVt_B21C98, true, 0x00B10530, 0x7A281983); }

// T* f(?, XmlCursor* c): new 16-byte record (base 0x004BB3B0(p, c), vtable
// 0x00B10A80) whose u8 +12 is read from the bool 0x03FF92CB
DECOMP(00092BD4, NewXmlBool_00092BD4) { NewXmlBool28(ctx, o28fn::Obj_InitVt_B21C98, true, 0x00B10A80, 0x03FF92CB); }

// T* f(?, XmlCursor* c): new 16-byte anim command (AnimCmd_ConstructBase(p),
// vtable 0x00B11A80) whose u8 +12 is read from the bool 0x83F172B0
DECOMP(000A6EA8, NewXmlBool_000A6EA8) { NewXmlBool28(ctx, o28fn::AnimCmd_ConstructBase, false, 0x00B11A80, 0x83F172B0); }

// T::~T() deleting (vtable 0x00B14138): frees the buffer +1416 (pool +1412)
// when set, resets the six sub-object vtables (+240 .. +1296) to 0x00B0DB3C,
// then the 0x00B11D98 base destructor and Mem_FreeFrom(o, 0)
DECOMP(0011BC50, VtB14138_Delete_0011BC50) {
    const u32 o = arg<u32>(ctx, 0);
    if (o == 0) return;
    if (load<u32>(o + 1416) != 0) {
        const u32 pool = load<u32>(o + 1412);
        store<u32>(o, 0x00B14138);
        call(ctx, o26fn::Mem_FreeIfSet, {load<u32>(o + 1416), pool});
    }
    for (u32 off : {1296u, 1136u, 992u, 772u, 720u, 240u}) store<u32>(o + off, 0x00B0DB3C);
    store<u32>(o, 0x00B11D98);
    call(ctx, o26fn::Obj_BaseDtor_0C2DA0, {o});
    ctx.gpr[3] = call(ctx, o25fn::Mem_FreeFrom, {o, 0});
}

// Deleting-destructor thunk (this - 384) of the 0x00B14968 class
DECOMP(00178184, VtB14968_DeleteAdj384) { AdjDelete28(ctx, -384, 0x00B14968, 0x00B14B00, 0x00B14B28); }

// Deleting-destructor thunk (this - 380) of the 0x00B14968 class
DECOMP(00178228, VtB14968_DeleteAdj380) { AdjDelete28(ctx, -380, 0x00B14968, 0x00B14B00, 0x00B14B28); }

// bool f(T* o, f32 a, f32 b): RangeGate28 on the u8 +572, *0x001BFE30
DECOMP(001BFE38, Obj_RangeGate_001BFE38) { RangeGate28(ctx, 572, 0x001BFE30); }

// bool f(T* o, f32 a, f32 b): RangeGate28 on the u8 +576, *0x001C1B4C
DECOMP(001C1B50, Obj_RangeGate_001C1B50) { RangeGate28(ctx, 576, 0x001C1B4C); }

// bool f(T* o, f32 a, f32 b): RangeGate28 on the flag 0x80 of the u8 +604,
// *0x001C42E0
DECOMP(001C42E8, Obj_RangeGate_001C42E8) { RangeGate28(ctx, 604, 0x001C42E0, 0x80u); }

// void f(T* a, U* b): Thunk_000E44C0(a, b), then copies a's +256..+264,
// +272..+296 (floats, raw), +240, +300 and the u8 +304 into b's +224..+232,
// +240..+264, +208, +268 and +272 (r3 = a->+300, f1 = a->+296)
DECOMP(001F6238, Obj_CopyState_001F6238) {
    const u32 a = arg<u32>(ctx, 0), b = arg<u32>(ctx, 1);
    call(ctx, 0x000E44C0, {a, b});
    auto c = [&](u32 d, u32 s) { store<u32>(b + d, load<u32>(a + s)); };
    c(224, 256);
    c(228, 260);
    c(232, 264);
    c(240, 272);
    u32 f = load<u32>(a + 276);
    u32 w = load<u32>(a + 240);
    store<u32>(b + 244, f);
    f = load<u32>(a + 280);
    store<u32>(b + 208, w);
    store<u32>(b + 248, f);
    c(252, 284);
    c(256, 288);
    f = load<u32>(a + 292);
    w = load<u32>(a + 300);
    store<u32>(b + 260, f);
    f = load<u32>(a + 296);
    const u8 by = load<u8>(a + 304);
    store<u32>(b + 264, f);
    store<u32>(b + 268, w);
    store<u8>(b + 272, by);
    ctx.fpr[1] = static_cast<f64>(std::bit_cast<f32>(f));
    ctx.gpr[3] = w;
}

// void f(T* o, u8 on): when on, sets bit 0x200 of +136 on each of the 27
// set pointers o->+428 [3][3][3] (strides 36, 12, 4); then 0x0020D5D4(o, on)
DECOMP(002225B0, Obj_FlagGrid_002225B0) {
    const u32 o = arg<u32>(ctx, 0), on = arg<u32>(ctx, 1) & 0xFFu;
    if (on != 0)
        for (u32 i = 0; i < 3; ++i)
            for (u32 j = 0; j < 3; ++j)
                for (u32 k = 0; k < 3; ++k) {
                    const u32 p = load<u32>(o + 36 * i + 12 * j + 4 * k + 428);
                    if (p != 0) store<u32>(p + 136, load<u32>(p + 136) | 0x200u);
                }
    ctx.gpr[3] = call(ctx, 0x0020D5D4, {o, on});
}

// f32 f(T* o): a random value between lo and hi, where lo = +1224 when
// above *0x0025DD58 (else +1172) and hi = +1228 when above it (else +1176):
// lo + (hi - lo) * (RandStep28's old value * *0x0025DD5C)
DECOMP(0025DD60, Obj_RandRange_0025DD60) {
    const u32 o = arg<u32>(ctx, 0);
    const f64 k0 = LfsAt(0x0025DD58);
    f32 lo = load<f32>(o + 1224);
    const f32 b = load<f32>(o + 1228);
    f32 hi = load<f32>(o + 1176);
    if (!(static_cast<f64>(lo) > k0)) lo = load<f32>(o + 1172);
    if (static_cast<f64>(b) > k0) hi = b;
    const f32 d = static_cast<f32>(static_cast<f64>(hi) - static_cast<f64>(lo));
    const f32 r = static_cast<f32>(static_cast<f64>(static_cast<s64>(RandStep28())));
    const f32 m = static_cast<f32>(static_cast<f64>(r) * LfsAt(0x0025DD5C));
    ctx.fpr[1] = static_cast<f32>(std::fma(static_cast<f64>(d), static_cast<f64>(m), static_cast<f64>(lo)));
    ctx.gpr[3] = 0x01160000;
}

// T::~T() (vtable 0x00B1CD38): when the child +36 has bit 0x40000 of +172,
// child->vt[40](child, 0) and child->vt[40](child, 1), and +36 = 0
DECOMP(002B4264, VtB1CD38_Dtor_002B4264) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 c = load<u32>(o + 36);
    store<u32>(o, 0x00B1CD38);
    ctx.gpr[3] = c;
    if (c == 0 || (load<u32>(c + 172) & 0x40000u) == 0) return;
    VCall(ctx, c, 40, {c, 0});
    const u32 c2 = load<u32>(o + 36);
    VCall(ctx, c2, 40, {c2, 1});
    store<u32>(o + 36, 0);
    ctx.gpr[3] = 0;
}

// T::~T() (vtable 0x00B1D500): child +16 ->vt[40](child, 0) when set, frees
// +24 and +32 (Mem_FreeFrom(p, 0), cleared), base vtable 0x00B1D4D8
DECOMP(002ECC14, VtB1D500_Dtor_002ECC14) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 c = load<u32>(o + 16);
    store<u32>(o, 0x00B1D500);
    if (c != 0) VCall(ctx, c, 40, {c, 0});
    call(ctx, o25fn::Mem_FreeFrom, {load<u32>(o + 24), 0});
    const u32 b = load<u32>(o + 32);
    store<u32>(o + 24, 0);
    call(ctx, o25fn::Mem_FreeFrom, {b, 0});
    store<u32>(o + 32, 0);
    store<u32>(o, 0x00B1D4D8);
    ctx.gpr[3] = 0x00B1D4D8;
}

// void f(T* o, x): when the u8 o->+18 is 4, walks the list o->+2576 (next
// +2564) calling p->vt[72](p, x), stopping early once p's u8 +18 is 4 and
// o->+2572 is 6
DECOMP(00327F78, Obj_ForEachChild72_00327F78) {
    const u32 o = arg<u32>(ctx, 0), x = arg<u32>(ctx, 1);
    ctx.gpr[3] = load<u8>(o + 18);
    if (ctx.gpr[3] != 4) return;
    for (u32 p = load<u32>(o + 2576); p != 0; p = load<u32>(p + 2564)) {
        VCall(ctx, p, 72, {p, x});
        ctx.gpr[3] = load<u8>(p + 18);
        if (ctx.gpr[3] == 4) {
            ctx.gpr[3] = load<u32>(o + 2572);
            if (ctx.gpr[3] == 6) return;
        }
    }
}

// void f(T* o): p = SmallAlloc(448) built by 0x003A3B30(p, o->+8, &o->+2592,
// &o->+3248, 0x7719A548, o) when allocated; World_SetChild1281F4(the old
// o->+8, p or 0)
DECOMP(0032A27C, Obj_NewChild_0032A27C) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 w = load<u32>(o + 8);
    const u32 p = static_cast<u32>(call(ctx, o19fn::SmallAlloc, {448}));
    if (p != 0) call(ctx, 0x003A3B30, {p, load<u32>(o + 8), o + 2592, o + 3248, 0x7719A548, o});
    ctx.gpr[3] = call(ctx, 0x0047C530, {w, p});
}

// Shared body of 0x003323EC / 0x00333578: 0x00325CE4(o); when the u8 +18
// is 4, o->vt[28](o, 0), then, when the cache +cache is unset and the id
// +id set, cache = Obj_FindById(id) and its u16 +178 is incremented
// (r3 = &found->+178 then)
static void UpdateFindCache28(PPUContext& ctx, u32 cache, u32 id) {
    const u32 o = arg<u32>(ctx, 0);
    call(ctx, 0x00325CE4, {o, arg<u32>(ctx, 1)});
    ctx.gpr[3] = load<u8>(o + 18);
    if (ctx.gpr[3] != 4) return;
    VCall(ctx, o, 28, {o, 0});
    ctx.gpr[3] = load<u32>(o + cache);
    if (ctx.gpr[3] != 0) return;
    ctx.gpr[3] = load<u32>(o + id);
    if (ctx.gpr[3] == 0) return;
    const u32 r = static_cast<u32>(call(ctx, 0x000D6A28, {static_cast<u32>(ctx.gpr[3])}));
    store<u32>(o + cache, r);
    ctx.gpr[3] = r;
    if (r == 0) return;
    store<u16>(r + 178, static_cast<u16>(load<u16>(r + 178) + 1));
    ctx.gpr[3] = r + 178;
}

// void f(T* o): UpdateFindCache28 with the cache +2616 and the id +2592
DECOMP(003323EC, Obj_UpdateFindCache_003323EC) { UpdateFindCache28(ctx, 2616, 2592); }

// void f(T* o): UpdateFindCache28 with the cache +2612 and the id +2572
DECOMP(00333578, Obj_UpdateFindCache_00333578) { UpdateFindCache28(ctx, 2612, 2572); }

// T::~T() deleting (vtable 0x00B1EEE0): clears the six PtrVectors +2588 ..
// +2888 (stride 60, from the last), the base destructor 0x003286D8 and
// SmallFree(o, 2948)
DECOMP(003392D0, VtB1EEE0_Delete_003392D0) {
    const u32 o = arg<u32>(ctx, 0);
    if (o == 0) return;
    store<u32>(o, 0x00B1EEE0);
    for (s32 i = 5; i >= 0; --i) {
        const u32 v = o + 2888 - 60 * static_cast<u32>(5 - i);
        call(ctx, o19fn::PtrVector_Clear, {v});
        call(ctx, o19fn::Nop, {v});
    }
    call(ctx, 0x003286D8, {o});
    ctx.gpr[3] = call(ctx, o19fn::SmallFree, {o, 2948});
}

// r f(T* o, const char* s): 0 for a null or empty s; else the first nonzero
// 0x009DEE58(o->+20[i], s) over the o->+24 entries (0 when none)
DECOMP(0034654C, Obj_FindName_0034654C) {
    const u32 o = arg<u32>(ctx, 0), s = arg<u32>(ctx, 1);
    ctx.gpr[3] = 0;
    if (s == 0 || load<u8>(s) == 0) return;
    for (s32 i = 0; i < load<s32>(o + 24); ++i) {
        const u32 r = static_cast<u32>(call(ctx, 0x009DEE58, {load<u32>(load<u32>(o + 20) + 4 * static_cast<u32>(i)), s}));
        if (r != 0) {
            ctx.gpr[3] = r;
            return;
        }
    }
    ctx.gpr[3] = 0;
}

// void f(T* o): empties the u16 count +8 of the record list o->+104 one
// step at a time (the list is reloaded each step). The original's
// swap-with-last copy (64-byte records at +0) only runs when count - 1
// differs from its own low 16 bits, which never happens.
DECOMP(00352D30, List_PopAll_00352D30) {
    const u32 o = arg<u32>(ctx, 0);
    for (;;) {
        const u32 s = load<u32>(o + 104);
        if (s == 0 || load<u16>(s + 8) == 0) return;
        store<u16>(s + 8, static_cast<u16>(load<u16>(s + 8) - 1));
    }
}

// s32 cmp(const s32* a, const s32* b): sort order of two world slot indices
// by the flag 0x4 of the u8 kWorldPtr->* + 4028 + index (flagged first?):
// when the flags match, *a - *b; else 1 when *b's is set, -1 otherwise
DECOMP(00389C5C, World_SlotCmp_00389C5C) {
    const u32 x = load<u32>(arg<u32>(ctx, 1)), y = load<u32>(arg<u32>(ctx, 0));
    const u32 w = load<u32>(kWorldPtr);
    const u32 fx = (load<u8>(w + x + 4028) & 0x4u) != 0 ? 1u : 0u;
    const u32 fy = (load<u8>(w + 80 + y + 3948) & 0x4u) != 0 ? 1u : 0u;
    if (fx == fy) {
        ctx.gpr[3] = Sx(y - x);
        return;
    }
    ctx.gpr[3] = (load<u8>(w + 80 + x + 3948) & 0x4u) != 0 ? 1u : Sx(0xFFFFFFFFu);
}

// void f(T* o, const Vec3* v): pushes *v (12 bytes, raw) onto the 16-byte
// record array o->+40 (+0 data, +4 u16 capacity, +8 u16 count, +10 u16
// high-water mark), doubling it with PtrVector_Reserve when full (r3 = the
// old mark)
DECOMP(003DC940, Vec3List_Push_003DC940) {
    const u32 v = arg<u32>(ctx, 1);
    const u32 s = load<u32>(arg<u32>(ctx, 0) + 40);
    const u32 cap = load<u16>(s + 4);
    if (load<u16>(s + 8) == cap) call(ctx, o27fn::PtrVector_Reserve, {s, cap << 1});
    const u32 d = load<u32>(s) + (static_cast<u32>(load<u16>(s + 8)) << 4);
    store<u32>(d, load<u32>(v));
    store<u32>(d + 4, load<u32>(v + 4));
    const u32 z = load<u32>(v + 8);
    store<u32>(d + 8, z);
    ctx.fpr[1] = static_cast<f64>(std::bit_cast<f32>(z));
    const u32 n = load<u16>(s + 8) + 1u;
    const u32 m = load<u16>(s + 10);
    store<u16>(s + 8, static_cast<u16>(n));
    store<u16>(s + 10, static_cast<u16>(static_cast<s32>(m) > static_cast<s32>(n & 0xFFFFu) ? m : n));
    ctx.gpr[3] = m;
}

// void Cmd::Run(): Rec_Set_0048EF50(kWorldPtr->* + 0x1282A4, the floats
// +12..+24), and the same on + 0x1282C0 when the u8 +28 is set; then
// Flag_Set8_40000000(o)
DECOMP(0042F2F4, Cmd_SetWorldRec_0042F2F4) {
    const u32 o = arg<u32>(ctx, 0);
    auto set = [&](u32 off) {
        const u32 r = load<u32>(kWorldPtr) + off;
        callf(ctx, 0x0048EF50, {r}, {LfsAt(o + 12), LfsAt(o + 16), LfsAt(o + 20), LfsAt(o + 24)});
    };
    set(0x1282A4);
    if (load<u8>(o + 28) != 0) set(0x1282C0);
    ctx.gpr[3] = call(ctx, o27fn::Flag_Set8_40000000, {o});
}

// void T::OnMsg(Msg* m) (this = o - 12): the hash m->+8 0x2BB3C007 sets the
// u8 +144; 0x4FE5ADE2 clears +144 and sets +145; others warn (format
// 0x00AE2608 and the hash name)
DECOMP(00437978, Obj_OnMsgFlag_00437978) {
    const u32 p = arg<u32>(ctx, 0) - 12;
    const s32 h = load<s32>(arg<u32>(ctx, 1) + 8);
    if (h == 0x2BB3C007) {
        store<u8>(p + 144, 1);
        ctx.gpr[3] = 1;
    } else if (h == 0x4FE5ADE2) {
        store<u8>(p + 144, 0);
        store<u8>(p + 145, 1);
        ctx.gpr[3] = 0;
    } else {
        const u32 s = static_cast<u32>(call(ctx, o27fn::Hash_ToString, {static_cast<u32>(h)}));
        ctx.gpr[3] = call(ctx, o27fn::String_Format, {0x00AE2608, s});
    }
}

// void f(s32 i, f32 v, f32 dt): for i in 0..3, the value tables at
// 0x010AF0A8 (current), 0x010AF09C (previous) and 0x010AF0B4 (rate):
// cur[i] = v; when |dt| >= *0x0044A8A0 and dt > *0x0044A8A4, rate[i] =
// (v - prev[i]) / dt (f1 = it); else prev[i] = v and rate[i] = 0 (r3 = 0)
DECOMP(0044A8A8, Track_SetValue_0044A8A8) {
    const s32 i = arg<s32>(ctx, 0);
    if (!(i < 4 && i >= 0)) return;
    const f64 v = farg(ctx, 0), dt = farg(ctx, 1);
    const u32 off = static_cast<u32>(i) * 4;
    store<f32>(0x010AF0A8 + off, static_cast<f32>(v));
    ctx.gpr[3] = 0;
    if (!(std::fabs(dt) < LfsAt(0x0044A8A0)) && dt > LfsAt(0x0044A8A4)) {
        const f32 d = static_cast<f32>(v - static_cast<f64>(load<f32>(0x010AF09C + off)));
        const f32 r = static_cast<f32>(static_cast<f64>(d) / dt);
        store<f32>(0x010AF0B4 + off, r);
        ctx.fpr[1] = static_cast<f64>(r);
        return;
    }
    store<f32>(0x010AF09C + off, static_cast<f32>(v));
    store<u32>(0x010AF0B4 + off, 0);
}

// s64 f(a, b, s32 mode): 0x005FCB70(Fios_GetScheduler5FC90C(), 0, a, b,
// mode 0 -> 0, 2 -> 2, else 1, &out) and out (u64, from 0), or -3 on an
// error
DECOMP(0044C1E4, Fios_Query_0044C1E4) {
    Stack frame(ctx, 144);
    const u32 out = frame.addr() + 112;
    const u32 a = arg<u32>(ctx, 0), b = arg<u32>(ctx, 1);
    const s32 mode = arg<s32>(ctx, 2);
    store<u64>(out, 0);
    const u32 m = mode == 0 ? 0u : mode == 2 ? 2u : 1u;
    const u32 s = static_cast<u32>(call(ctx, 0x005FC90C, {a}));
    if (static_cast<u32>(call(ctx, 0x005FCB70, {s, 0, a, b, m, out})) != 0) {
        ctx.gpr[3] = static_cast<u64>(-3LL);
        return;
    }
    ctx.gpr[3] = load<u64>(out);
}

// u32 f(?, ?, u32 i): frees (fios_Free(Fios_GetScheduler5FC90C(), p)) and
// clears each set pointer of the 16 at 0x010B0BD0 + 72 * i (r3 = 0)
DECOMP(00450580, Fios_FreeSlots_00450580) {
    const u32 base = 0x010B0BC8u + 72u * arg<u32>(ctx, 2);
    for (u32 k = 0; k < 16; ++k) {
        const u32 slot = base + 8 + 4 * k;
        if (load<u32>(slot) == 0) continue;
        const u32 s = static_cast<u32>(call(ctx, 0x005FC90C, {load<u32>(slot)}));
        call(ctx, 0x005FE784, {s, load<u32>(slot)});
        store<u32>(slot, 0);
    }
    ctx.gpr[3] = 0;
}

// The event record fill of 0x00464F88 / 0x004658CC: +20 = obj, +0 = kind,
// +4 = the clock kWorldPtr->*->+12->+0 (f1 left), +8 = a or obj->+216->+52,
// +12 = b or obj->+168, +16 = c or obj->+336->+168 (0 when missing),
// +24 = last
static void EventFill28(PPUContext& ctx, u32 rec, u32 obj, u32 kind, u32 a, u32 b, u32 c, u32 last) {
    store<u32>(rec + 20, obj);
    const u32 w = load<u32>(kWorldPtr);
    store<u32>(rec, kind);
    const u32 t = load<u32>(load<u32>(w + 12));
    store<u32>(rec + 4, t);
    ctx.fpr[1] = static_cast<f64>(std::bit_cast<f32>(t));
    u32 v = a;
    if (a == 0 && obj != 0) {
        const u32 q = load<u32>(obj + 216);
        v = q != 0 ? load<u32>(q + 52) : 0;
    }
    store<u32>(rec + 8, v);
    store<u32>(rec + 12, b != 0 ? b : obj != 0 ? load<u32>(obj + 168) : 0);
    v = c;
    if (c == 0 && obj != 0) {
        const u32 q = load<u32>(obj + 336);
        v = q != 0 ? load<u32>(q + 168) : 0;
    }
    store<u32>(rec + 16, v);
    store<u32>(rec + 24, last);
}

// void Event::Init(obj, a, b, c): kind 3, EventFill28 (+24 = obj)
DECOMP(00464F88, Event3_Init_00464F88) {
    const u32 obj = arg<u32>(ctx, 1);
    EventFill28(ctx, arg<u32>(ctx, 0), obj, 3, arg<u32>(ctx, 2), arg<u32>(ctx, 3), arg<u32>(ctx, 4), obj);
}

// void Event::Init(obj, x, a, b, c): kind 23, EventFill28 (+24 = x)
DECOMP(004658CC, Event23_Init_004658CC) {
    EventFill28(ctx, arg<u32>(ctx, 0), arg<u32>(ctx, 1), 23, arg<u32>(ctx, 3), arg<u32>(ctx, 4), arg<u32>(ctx, 5), arg<u32>(ctx, 2));
}

// T* T::Clone(): p = SmallAlloc(48) (u16 +4 = 1, vtable 0x00B21E20, +32..+40
// = the vec 0x01164B40) then p's +32..+40 and +8 from this. Quirk: the
// copy writes through a null p when the allocation fails.
DECOMP(004BF4A8, VtB21E20_Clone_004BF4A8) {
    const u32 src = arg<u32>(ctx, 0);
    const u32 p = static_cast<u32>(call(ctx, o19fn::SmallAlloc, {48}));
    if (p != 0) {
        store<u16>(p + 4, 1);
        store<u32>(p, 0x00B21E20);
        store<u32>(p + 32, load<u32>(0x01164B40));
        store<u32>(p + 36, load<u32>(0x01164B44));
        store<u32>(p + 40, load<u32>(0x01164B48));
    }
    const u32 x = load<u32>(src + 32), y = load<u32>(src + 36), z = load<u32>(src + 40);
    store<u32>(p + 32, x);
    store<u32>(p + 36, y);
    store<u32>(p + 40, z);
    store<u32>(p + 8, load<u32>(src + 8));
    ctx.fpr[1] = static_cast<f64>(std::bit_cast<f32>(x));
    ctx.fpr[2] = static_cast<f64>(std::bit_cast<f32>(y));
    ctx.fpr[3] = static_cast<f64>(std::bit_cast<f32>(z));
    ctx.gpr[3] = p;
}

// void f(T* o, x): the input device d = (Effect_FindComponent(x, o->+12)
// when its vt[32]() kind is 6) ->+20, else the global 0x00F65FF8; when set,
// InputDevice_SetAxis(d, 0, o->+16, o->+20)
DECOMP(004C5D38, Effect_SetAxis_004C5D38) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 c = static_cast<u32>(call(ctx, 0x004BB4B4, {arg<u32>(ctx, 1), load<u32>(o + 12)}));
    u32 d;
    if (c != 0 && static_cast<u32>(VCall(ctx, c, 32, {c})) == 6) {
        d = load<u32>(c + 20);
        ctx.gpr[3] = 6;
    } else {
        d = load<u32>(0x00F65FF8);
        ctx.gpr[3] = 0x00F60000;
    }
    if (d == 0) return;
    callf(ctx, 0x00563000, {d, 0}, {LfsAt(o + 16), LfsAt(o + 20)});
}

// bool f(WorldObj* o, Counter* c): names o (WorldObj_SetName(o, name, 1))
// with String_Format(0x00AEF35C, the count c->+4 (post-incremented),
// c->+0->vt[12]()); returns true
DECOMP(004CF214, WorldObj_NameNext_004CF214) {
    const u32 o = arg<u32>(ctx, 0), c = arg<u32>(ctx, 1);
    const u32 n = load<u32>(c + 4);
    store<u32>(c + 4, n + 1);
    const u32 x = load<u32>(c);
    const u32 s = static_cast<u32>(VCall(ctx, x, 12, {x}));
    const u32 name = static_cast<u32>(call(ctx, o27fn::String_Format, {0x00AEF35C, n, s}));
    call(ctx, 0x0057D368, {o, name, 1});
    ctx.gpr[3] = 1;
}

// void f(T* o, u32 a, u32 b): pushes the pair (a, b) onto the vector o->+92
// (+96 u16 capacity, +100 u16 count, +102 u16 high-water mark), doubling it
// with PtrVector_Reserve when full (r3 = the old mark)
DECOMP(00755CB4, PairList_Push_00755CB4) {
    const u32 o = arg<u32>(ctx, 0), a = arg<u32>(ctx, 1), b = arg<u32>(ctx, 2);
    const u32 cap = load<u16>(o + 96);
    if (load<u16>(o + 100) == cap) call(ctx, o27fn::PtrVector_Reserve, {o + 92, (cap << 1) & 0x1FFFEu});
    const u32 d = load<u32>(o + 92) + ((static_cast<u32>(load<u16>(o + 100)) << 3) & 0x7FFF8u);
    store<u32>(d, a);
    store<u32>(d + 4, b);
    const u32 n = load<u16>(o + 100) + 1u;
    const u32 m = load<u16>(o + 102);
    store<u16>(o + 100, static_cast<u16>(n));
    store<u16>(o + 102, static_cast<u16>(static_cast<s32>(m) > static_cast<s32>(n & 0xFFFFu) ? m : n));
    ctx.gpr[3] = m;
}

// T::~T() deleting (vtables 0x00B18B48 / +380 0x00B18CE0 / +384
// 0x00B18D08): Shape_Release638498(&o->+436), the base, Mem_FreeFrom(o, 0)
DECOMP(007582D0, VtB18B48_Delete_007582D0) {
    Delete3Vt28(ctx, 0, 0x00B18B48, 0x00B18CE0, 0x00B18D08, 0x00638498, 436, o19fn::Mem_FreeFrom, 0);
}

// Same as 0x007582D0, freed with SmallFree(o, 448)
DECOMP(00758730, VtB18B48_Delete448_00758730) {
    Delete3Vt28(ctx, 0, 0x00B18B48, 0x00B18CE0, 0x00B18D08, 0x00638498, 436, o19fn::SmallFree, 448);
}

// Same as 0x007582D0, freed with SmallFree(o, 608)
DECOMP(00758C10, VtB18B48_Delete608_00758C10) {
    Delete3Vt28(ctx, 0, 0x00B18B48, 0x00B18CE0, 0x00B18D08, 0x00638498, 436, o19fn::SmallFree, 608);
}

// T::~T() (handle record): +20 = 0; unless the top bit of +24 is set, the
// vtable becomes 0x00F29900 and the global 0x011DF31C ->vt[20](it, +16,
// +24 & 0x3FFFFFFF) releases it; then +16 = 0, +24 = 0x80000000, vtable
// 0x00B13974 (r3 = 0)
DECOMP(0075BFA0, Handle_Dtor_0075BFA0) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 w = load<u32>(o + 24);
    store<u32>(o + 20, 0);
    if ((w & 0x80000000u) == 0) {
        store<u32>(o, 0x00F29900);
        VCall(ctx, 0x011DF31C, 20, {0x011DF31C, load<u32>(o + 16), w & 0x3FFFFFFFu});
    }
    store<u32>(o + 16, 0);
    store<u32>(o + 24, 0x80000000u);
    store<u32>(o, 0x00B13974);
    ctx.gpr[3] = 0;
}

// T::~T() deleting (vtable 0x00B1D4B8): 0x002EA464(o), frees +36 when set
// (Mem_FreeFrom, cleared), clears the PtrVector +76, the base destructor
// 0x002E87D4 and Mem_FreeFrom(o, 0)
DECOMP(00767374, VtB1D4B8_Delete_00767374) {
    const u32 o = arg<u32>(ctx, 0);
    if (o == 0) return;
    store<u32>(o, 0x00B1D4B8);
    call(ctx, 0x002EA464, {o});
    if (load<u32>(o + 36) != 0) {
        call(ctx, o25fn::Mem_FreeFrom, {load<u32>(o + 36), 0});
        store<u32>(o + 36, 0);
    }
    call(ctx, o19fn::PtrVector_Clear, {o + 76});
    call(ctx, o19fn::Nop, {o + 76});
    call(ctx, 0x002E87D4, {o});
    ctx.gpr[3] = call(ctx, o25fn::Mem_FreeFrom, {o, 0});
}

// Node* f(s16 id): p = Mem_Alloc(24, 0, 16), Node_Init(p, id, 0x00B20040),
// then the u16 kNodeTable->+24 [p->+16] (128-byte records) = 16. Quirk:
// with the s16 p->+16 = -1 the 16 is written to address 0.
DECOMP(007799AC, Node_New_007799AC) {
    const u32 id = arg<u32>(ctx, 0);
    const u32 p = static_cast<u32>(call(ctx, o19fn::Mem_Alloc, {24, 0, 16}));
    if (p == 0) {
        ctx.gpr[3] = 0;
        return;
    }
    call(ctx, o19fn::Node_Init, {p, Sx(static_cast<u32>(static_cast<s16>(id))), 0x00B20040});
    const s32 i = static_cast<s16>(load<u16>(p + 16));
    const u32 t = load<u32>(o19fn::kNodeTable);
    const u32 d = i != -1 ? load<u32>(t + 24) + (static_cast<u32>(i) << 7) : 0u;
    store<u16>(d, 16);
    ctx.gpr[3] = p;
}

// void f(T* o): the u8 +248 = 1, bit 0x02000000 of +136 set, then
// c->vt[148](c) for each child c of the list +152 (next +156)
DECOMP(007965EC, Obj_NotifyChildren_007965EC) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 w = load<u32>(o + 136);
    store<u8>(o + 248, 1);
    store<u32>(o + 136, w | 0x02000000u);
    for (u32 c = load<u32>(o + 152); c != 0; c = load<u32>(c + 156)) VCall(ctx, c, 148, {c});
}

// ---------------------------------------------------------------- sweep: 42 instructions
// void f(T* o, const Vec3* v): +96..+104 = *v; when the flag 0x10 of the u8
// +304 is set and the body +212 too, the position goes through
// 0x006C0AA4(body, &v copy, *0x011AD1D4) and 0x006C1B94(&out, body) and
// +96..+104 = out (f1..f3 left as the last vector)
DECOMP(00015944, Obj_SetPos_00015944) {
    Stack frame(ctx, 144);
    const u32 o = arg<u32>(ctx, 0), v = arg<u32>(ctx, 1);
    const u32 tmp = frame.addr() + 112, out = frame.addr() + 124;
    const u32 x = load<u32>(v);
    store<u32>(o + 96, x);
    const u32 y = load<u32>(v + 4);
    store<u32>(o + 100, y);
    const u32 z = load<u32>(v + 8);
    store<u32>(o + 104, z);
    auto setf = [&](u32 a, u32 b, u32 c) {
        ctx.fpr[1] = static_cast<f64>(std::bit_cast<f32>(a));
        ctx.fpr[2] = static_cast<f64>(std::bit_cast<f32>(b));
        ctx.fpr[3] = static_cast<f64>(std::bit_cast<f32>(c));
    };
    if ((load<u8>(o + 304) & 0x10u) == 0) {
        setf(z, y, x);
        return;
    }
    if (load<u32>(o + 212) == 0) {
        setf(z, y, x);
        ctx.gpr[3] = 0;
        return;
    }
    store<u32>(tmp, x);
    store<u32>(tmp + 4, y);
    store<u32>(tmp + 8, z);
    call(ctx, 0x006C0AA4, {load<u32>(o + 212), tmp, load<u32>(0x011AD1D4)});
    call(ctx, 0x006C1B94, {out, load<u32>(o + 212)});
    const u32 a = load<u32>(out), b = load<u32>(out + 4), c = load<u32>(out + 8);
    store<u32>(o + 96, a);
    store<u32>(o + 100, b);
    store<u32>(o + 104, c);
    setf(a, b, c);
}

// void f(T* o, u32 anim): when +400 changes to anim and the state +4 is 3,
// Anim_Restart2775A8(o->+64, anim, 0, 0, -1, -, 0, 0, -, -, 0, -1; the
// floats 0x0001F324 [0], [1], [2], [2]) and, when it started, +32 = anim
DECOMP(0001F330, Obj_SetAnim_0001F330) {
    const u32 o = arg<u32>(ctx, 0), anim = arg<u32>(ctx, 1);
    ctx.gpr[3] = load<u32>(o + 400);
    if (static_cast<u32>(ctx.gpr[3]) == anim) return;
    ctx.gpr[3] = load<u32>(o + 4);
    store<u32>(o + 400, anim);
    if (static_cast<u32>(ctx.gpr[3]) != 3) return;
    const u64 r8 = ctx.gpr[8];  // never set by the original; the callee doesn't read it
    const f64 c = LfsAt(0x0001F32C);
    // Arguments 9 / 10 (r1 +112 / +120) are never written (float slots).
    callf(ctx, 0x002775A8, {load<u32>(o + 64), anim, 0, 0, ~0ull, r8, 0, 0, 0, 0, 0, ~0ull}, {LfsAt(0x0001F324), LfsAt(0x0001F328), c, c});
    if (static_cast<u32>(ctx.gpr[3]) != 0) store<u32>(o + 32, anim);
}

// void f(Vec3* out, const Vec3* a, const Vec3* b, const Vec3* d, const
// Vec3* c, f32 t): the cubic Bezier point a, b, c, d at t (u = *0x00064C7C
// - t, k = *0x00064C80): u^3 a + k t u^2 b + k t^2 u c + t^3 d, per
// component in single precision
DECOMP(00064C88, Bezier3_Eval_00064C88) {
    const u32 out = arg<u32>(ctx, 0), a = arg<u32>(ctx, 1), b = arg<u32>(ctx, 2), d = arg<u32>(ctx, 3), c = arg<u32>(ctx, 4);
    const f64 t = farg(ctx, 0);
    const f32 u = Sub28(LfsAt(0x00064C7C), t);
    const f32 kt = Mul28(t, LfsAt(0x00064C80));
    const f32 t3 = Mul28(Mul28(t, t), t);
    const f32 u2 = Mul28(u, u);
    const f32 ktu = Mul28(kt, u);
    const f32 u3 = Mul28(u2, u);
    const f32 wb = Mul28(ktu, u);
    const f32 wc = Mul28(Mul28(kt, t), u);
    f32 r = 0;
    for (u32 k = 0; k < 12; k += 4) {
        const f32 pb = load<f32>(b + k), pa = load<f32>(a + k), pc = load<f32>(c + k), pd = load<f32>(d + k);
        r = Mul28(wb, pb);
        r = Madd28(u3, pa, r);
        r = Madd28(wc, pc, r);
        r = Madd28(t3, pd, r);
        store<f32>(out + k, r);
    }
    ctx.fpr[1] = static_cast<f64>(r);
}

// T* f(?, XmlCursor* c): new 16-byte record (base 0x004BB3B0(p, c), vtable
// 0x00B0F2E8) whose u8 +12 is read from the bool 0x8BAD1360
DECOMP(0006F664, NewXmlBool_0006F664) { NewXmlBool28(ctx, o28fn::Obj_InitVt_B21C98, true, 0x00B0F2E8, 0x8BAD1360); }

// T* f(?, XmlCursor* c): new 16-byte record (base 0x004BB3B0(p, c), vtable
// 0x00B10238) whose u32 +12 is read from the hash 0xEAC7D9D6
DECOMP(00087704, NewXmlHash_00087704) { NewXmlHash28(ctx, 0x00B10238, 0xEAC7D9D6); }

// T* f(?, XmlCursor* c): new 16-byte record (base 0x004BB3B0(p, c), vtable
// 0x00B11710) whose u8 +12 is read from the bool 0xCADB1721
DECOMP(000A3A34, NewXmlBool_000A3A34) { NewXmlBool28(ctx, o28fn::Obj_InitVt_B21C98, true, 0x00B11710, 0xCADB1721); }

// T* T::Clone(): p = SmallAlloc(88) (u16 +4 = 1, the u8 +20 = 0, vtable
// 0x00B11B88), the name +20 copied (String_Copy(&p->+20, &this->+20, 64))
// when nonempty, +8 copied. Quirk: writes through a null p on failure.
DECOMP(000A7928, VtB11B88_Clone_000A7928) {
    const u32 src = arg<u32>(ctx, 0);
    const u32 a = static_cast<u32>(call(ctx, o19fn::SmallAlloc, {88}));
    u32 p = 0;
    if (a != 0) {
        store<u16>(a + 4, 1);
        store<u8>(a + 20, 0);
        p = a;
        store<u32>(a, 0x00B11B88);
    }
    const u32 s = src + 20;
    if (s != 0 && load<u8>(s) != 0) call(ctx, 0x00553684, {p + 20, s, 64});
    store<u32>(p + 8, load<u32>(src + 8));
    ctx.gpr[3] = p;
}

// Static destructor: five global tuning variables (0x00F9D044 / 0x00F9CFF8
// vtable 0x00B0D99C, 0x00F9CFAC 0x00B0D934, 0x00F9D158 0x00B0D968,
// 0x00F9D090 0x00B0DA38) get their vtable back and TuningVar_DestructBase
DECOMP(000B1720, Tuning_StaticDtor_000B1720) {
    const std::pair<u32, u32> vars[] = {{0x00F9D044, 0x00B0D99C}, {0x00F9CFF8, 0x00B0D99C}, {0x00F9CFAC, 0x00B0D934},
                                        {0x00F9D158, 0x00B0D968}, {0x00F9D090, 0x00B0DA38}};
    for (const auto& [v, vt] : vars) {
        store<u32>(v, vt);
        call(ctx, 0x004FC1A0, {v});
    }
}

// void f(T* o, u32 key): removes (swap with last) each entry e of the u32
// array o->+12 (u16 count +20) that is set and either lacks bit 0x40000 of
// +172 or has +168 == key; then bit 31 of +8 = (count != 0)
DECOMP(000B4524, List_RemoveKey_000B4524) {
    const u32 o = arg<u32>(ctx, 0), key = arg<u32>(ctx, 1);
    s32 i = 0;
    u32 off = 0;
    u32 n;
    while (static_cast<s32>(n = load<u16>(o + 20)) > i) {
        const u32 data = load<u32>(o + 12);
        const u32 e = load<u32>(data + off);
        if (e != 0 && ((load<u32>(e + 172) & 0x40000u) == 0 || load<u32>(e + 168) == key)) {
            const u32 m = n - 1;
            store<u16>(o + 20, static_cast<u16>(m));
            if (static_cast<u32>(i) != (m & 0xFFFFu)) store<u32>(data + off, load<u32>(data + ((m << 2) & 0x3FFFCu)));
            off -= 4;
            --i;
        }
        off += 4;
        ++i;
    }
    store<u32>(o + 8, (load<u32>(o + 8) & 0x7FFFFFFFu) | (n != 0 ? 0x80000000u : 0u));
}

// const char* f(T* t, U* o): o's name +140 when nonempty; else
// o->+112->vt[12]() when set; else String_Format(0x00AC68F0, &t->+56,
// ++t->+12)
DECOMP(000CBA88, Obj_GetName_000CBA88) {
    const u32 t = arg<u32>(ctx, 0), o = arg<u32>(ctx, 1);
    const u32 s = o + 140;
    if (s != 0 && load<u8>(s) != 0) {
        ctx.gpr[3] = s;
        return;
    }
    const u32 c = load<u32>(o + 112);
    if (c != 0) {
        VCall(ctx, c, 12, {c});
        return;
    }
    const u32 n = load<u32>(t + 12) + 1;
    store<u32>(t + 12, n);
    call(ctx, o27fn::String_Format, {0x00AC68F0, t + 56, n});
}

// bool f(T* o, f32 v, ?, a (r5), b (r6)): true when v <= *0x000EDAEC; else
// 0x000E1738(o, a, a, b; v), then 0x000E9BA0(o, 4, 0) when the f32 +348 is
// <= the limit, and whether +348 is still above it
DECOMP(000EDAF0, Obj_ApplyAmount_000EDAF0) {
    const u32 o = arg<u32>(ctx, 0), a = arg<u32>(ctx, 2), b = arg<u32>(ctx, 3);
    const f64 v = farg(ctx, 0), k = LfsAt(0x000EDAEC);
    if (v <= k) {
        ctx.gpr[3] = 1;
        return;
    }
    callf(ctx, 0x000E1738, {o, a, a, b}, {v});
    if (LfsAt(o + 348) <= k) call(ctx, 0x000E9BA0, {o, 4, 0});
    const f64 h = LfsAt(o + 348);
    ctx.fpr[1] = h;
    ctx.gpr[3] = h > k ? 1 : 0;
}

// void f(T* o): releases the children +820 and +824 (c->vt[40](c, 1),
// cleared) and clears bit 0x10000000 of +980 (r3 = 0)
DECOMP(001EDC2C, Obj_ReleasePair_001EDC2C) {
    const u32 o = arg<u32>(ctx, 0);
    for (u32 off : {820u, 824u}) {
        const u32 c = load<u32>(o + off);
        if (c == 0) continue;
        VCall(ctx, c, 40, {c, 1});
        store<u32>(o + off, 0);
    }
    store<u32>(o + 980, load<u32>(o + 980) & ~0x10000000u);
    ctx.gpr[3] = 0;
}

// T::~T() deleting (vtable 0x00B1B9B8): releases the skeleton +780
// (AnimSkel_Release, cleared), frees +736 (Mem_FreeIfSet(p, +732), vtable
// 0x00B1B4C8 then), the 0x00B11D98 base destructor and SmallFree(o, 1040)
DECOMP(0023E470, VtB1B9B8_Delete_0023E470) {
    const u32 o = arg<u32>(ctx, 0);
    if (o == 0) return;
    if (load<u32>(o + 780) != 0) {
        store<u32>(o, 0x00B1B9B8);
        call(ctx, o26fn::AnimSkel_Release, {load<u32>(o + 780)});
        store<u32>(o + 780, 0);
    }
    if (load<u32>(o + 736) != 0) {
        const u32 pool = load<u32>(o + 732);
        store<u32>(o, 0x00B1B4C8);
        call(ctx, o26fn::Mem_FreeIfSet, {load<u32>(o + 736), pool});
    }
    store<u32>(o, 0x00B11D98);
    call(ctx, o26fn::Obj_BaseDtor_0C2DA0, {o});
    ctx.gpr[3] = call(ctx, o19fn::SmallFree, {o, 1040});
}

// void f(T* o, const Vec3* v): VecPush28 of *v (16-byte records) on the
// vector o->+528
DECOMP(00244810, Vec3List_Push_00244810) {
    const u32 v = arg<u32>(ctx, 1);
    VecPush28(ctx, arg<u32>(ctx, 0) + 528, 4, [&](u32 d) { CopyVec3F28(ctx, d, v); });
}

// void f(Vec3* out, T* o, s32 i): out = the vector +16 of o's 48-byte record
// i (o->+64) or *0x00264DE4 x 3 for i = -1; but o's default +32 when the
// result equals the vector 0x01164B40. Quirk: the original compares the
// stale stack words (r1 - 16 .. - 8) instead of the result (its float
// stores there were scheduled after the integer loads), so that is kept.
DECOMP(00264DE8, Obj_GetRecordPos_00264DE8) {
    const u32 out = arg<u32>(ctx, 0), o = arg<u32>(ctx, 1);
    const s32 i = arg<s32>(ctx, 2);
    const u32 sp = static_cast<u32>(ctx.gpr[1]);
    u32 x, y, z;
    x = y = z = load<u32>(0x00264DE4);
    if (i != -1) {
        const u32 e = load<u32>(o + 64) + static_cast<u32>(i) * 48u;
        x = load<u32>(e + 16);
        y = load<u32>(e + 20);
        z = load<u32>(e + 24);
    }
    const u32 s2 = load<u32>(sp - 8);
    const u64 s01 = load<u64>(sp - 16);
    const u32 g2 = load<u32>(0x01164B48);
    const u64 g01 = load<u64>(0x01164B40);
    store<u32>(sp - 16, x);
    store<u32>(sp - 12, y);
    store<u32>(sp - 8, z);
    if (((s01 - g01) | static_cast<u32>(s2 - g2)) == 0) {
        x = load<u32>(o + 32);
        y = load<u32>(o + 36);
        z = load<u32>(o + 40);
    }
    store<u32>(out, x);
    store<u32>(out + 4, y);
    store<u32>(out + 8, z);
    ctx.fpr[1] = F32Bits28(x);
    ctx.fpr[2] = F32Bits28(y);
    ctx.fpr[3] = F32Bits28(z);
}

// void f(Mat* m, T* o): m = the global matrix 0x01164BD0 (rows +0, +16,
// +32, +48; three floats each, raw), then 0x00268FD8(o, 0, m) when the u16
// o->+36 is set (r3 = o otherwise)
DECOMP(002691A4, Mat_SetDefault_002691A4) {
    const u32 m = arg<u32>(ctx, 0), o = arg<u32>(ctx, 1);
    u32 w = 0;
    for (u32 r = 0; r < 64; r += 16)
        for (u32 k = 0; k < 12; k += 4) {
            w = load<u32>(0x01164BD0 + r + k);
            store<u32>(m + r + k, w);
        }
    ctx.fpr[1] = F32Bits28(w);
    ctx.gpr[3] = o;
    if (load<u16>(o + 36) != 0) call(ctx, 0x00268FD8, {o, 0, m});
}

// bool f(T* o, U* x): sets the held object +296 (the old one released with
// vt[40](old, 0), the new one's u16 +178 incremented) and the u8 +288 =
// whether one is held (r3 = that; the unchanged object when x is held)
DECOMP(002C2378, Obj_SetHeld_002C2378) {
    const u32 o = arg<u32>(ctx, 0), x = arg<u32>(ctx, 1);
    const u32 c = load<u32>(o + 296);
    ctx.gpr[3] = c;
    if (c == x) return;
    if (c != 0) VCall(ctx, c, 40, {c, 0});
    store<u32>(o + 296, x);
    u32 r = x;
    if (x != 0) {
        store<u16>(x + 178, static_cast<u16>(load<u16>(x + 178) + 1));
        r = load<u32>(o + 296);
    }
    const u32 b = r != 0 ? 1u : 0u;
    store<u8>(o + 288, static_cast<u8>(b));
    ctx.gpr[3] = b;
}

// s8 f(?, const Vec2* v): the direction of v: -1 when x^2 + y^2 <
// *0x002CDC7C, else by the larger component (k = *0x002CDC80): x < k ? 6 :
// 2 when |x| > |y|, y < k ? 0 : 4 otherwise
DECOMP(002CDC88, Vec2_Direction_002CDC88) {
    const u32 v = arg<u32>(ctx, 1);
    const f64 y = LfsAt(v + 4), x = LfsAt(v);
    const f32 d = Madd28(x, x, Mul28(y, y));
    if (static_cast<f64>(d) < LfsAt(0x002CDC7C)) {
        ctx.fpr[1] = static_cast<f64>(d);
        ctx.gpr[3] = Sx(0xFFFFFFFFu);
        return;
    }
    const f64 k = LfsAt(0x002CDC80);
    ctx.fpr[2] = k;
    if (std::fabs(LfsAt(v)) > std::fabs(LfsAt(v + 4))) {
        const f64 a = LfsAt(v);
        ctx.fpr[1] = a;
        ctx.gpr[3] = a < k ? 6 : 2;
        return;
    }
    const f64 b = LfsAt(v + 4);
    ctx.fpr[1] = b;
    ctx.gpr[3] = b < k ? 0 : 4;
}

// void f(T* o): p = Mem_Alloc(196, 0, 16) built by VtB205B8_Ctor(p, o->+8,
// &o->+2580, 0x7719A548, 0, o) when allocated; World_SetChild1281F4(the old
// o->+8, p or 0)
DECOMP(003308B0, Obj_NewChild_003308B0) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 w = load<u32>(o + 8);
    const u32 p = static_cast<u32>(call(ctx, o19fn::Mem_Alloc, {196, 0, 16}));
    if (p != 0) call(ctx, 0x003D8B34, {p, load<u32>(o + 8), o + 2580, 0x7719A548, 0, o});
    ctx.gpr[3] = call(ctx, 0x0047C530, {w, p});
}

// void f(T* o): 0x003AB0B8(o, the no-case hashes of the names
// String_Format(0x00ADF148, &o->+28, 0x00ADF26C) and
// String_Format(0x00ADF158, &o->+60, 0x00ADF26C), 0, 0x01087A20, 1;
// *0x01087A68)
DECOMP(003AB764, Obj_BindNames_003AB764) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 s1 = static_cast<u32>(call(ctx, o27fn::String_Format, {0x00ADF148, o + 28, 0x00ADF26C}));
    const u32 h1 = static_cast<u32>(call(ctx, 0x0056A710, {s1}));
    const u32 s2 = static_cast<u32>(call(ctx, o27fn::String_Format, {0x00ADF158, o + 60, 0x00ADF26C}));
    const u32 h2 = static_cast<u32>(call(ctx, 0x0056A710, {s2}));
    callf(ctx, 0x003AB0B8, {o, h1, h2, 0, 0x01087A20, 1}, {LfsAt(0x01087A68)});
}

// void Cmd::Run(): Rec_Init_0048EF9C(kWorldPtr->* + 0x1282DC,
// Obj_FindByHandle42C2B0(+12, 0), the floats +16..+40; the s32 +44, the u8s
// +48 and +49, 0 as stack arguments), then Flag_Set8_40000000(o)
DECOMP(0042F4BC, Cmd_InitWorldRec_0042F4BC) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 t = static_cast<u32>(call(ctx, 0x0042C2B0, {load<u32>(o + 12), 0}));
    const u64 a44 = Sx(load<u32>(o + 44));
    const u32 r = load<u32>(kWorldPtr) + 0x1282DC;
    const u32 b48 = load<u8>(o + 48), b49 = load<u8>(o + 49);
    // r5..r8 hold copies of the stack arguments (the floats take those
    // slots); r9 / r10 are what the call above left; slot 9 (r1 +112) is
    // never written.
    callf(ctx, 0x0048EF9C, {r, t, b48, a44, 0, b49, ctx.gpr[9], ctx.gpr[10], 0, a44, b48, b49, 0},
          {LfsAt(o + 16), LfsAt(o + 20), LfsAt(o + 24), LfsAt(o + 28), LfsAt(o + 32), LfsAt(o + 36), LfsAt(o + 40)});
    ctx.gpr[3] = call(ctx, o27fn::Flag_Set8_40000000, {o});
}

// T::T(a, b, c, -, bool flag (r8), bool on (r9); f32 f): base
// 0x0042C9D0(o, on), +16 = a, vtable 0x00B20FB0, +12 = 0, +20 = b, +24 =
// c, +28 = f, +32: bit 31 = flag, bit 30 cleared (r3 = the vtable)
DECOMP(00431748, VtB20FB0_Ctor_00431748) {
    const u32 o = arg<u32>(ctx, 0), a = arg<u32>(ctx, 1), b = arg<u32>(ctx, 2), c = arg<u32>(ctx, 3), flag = arg<u32>(ctx, 5);
    const f64 f = farg(ctx, 0);
    CmdBase27(ctx, o, arg<u32>(ctx, 6));
    const u32 w = (load<u32>(o + 32) & 0x7FFFFFFFu) | (flag << 31);
    store<u32>(o + 16, a);
    store<u32>(o, 0x00B20FB0);
    store<u32>(o + 12, 0);
    store<u32>(o + 20, b);
    store<u32>(o + 24, c);
    store<f32>(o + 28, static_cast<f32>(f));
    store<u32>(o + 32, w & ~0x40000000u);
    ctx.gpr[3] = 0x00B20FB0;
}

// void f(T* o): sets the flag 0x40 of the u8 +556, then 0x004890A0(the
// world, &o->+9, the s32 +524, &o->+528, &o->+536, flag 0x04, flag 0x02,
// &o->+548, &o->+552, flag 0x01)
DECOMP(00433FD0, Obj_Submit_00433FD0) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 b = load<u8>(o + 556) | 0x40u;
    const u64 n = Sx(load<u32>(o + 524));
    store<u8>(o + 556, static_cast<u8>(b));
    const u32 w = load<u32>(kWorldPtr);
    ctx.gpr[3] = call(ctx, 0x004890A0, {w, o + 9, n, o + 528, o + 536, (b >> 2) & 1u, (b >> 1) & 1u, o + 548, o + 552, b & 1u});
}

// T::T(a, b, c, d, -, u8 e (r9), bool on (r10); f32 f): base
// 0x0042C9D0(o, on), +12..+24 = a..d, +28 = f, +32 = 0, vtable 0x00B211A8,
// the u8 +36 = e (r3 = the vtable)
DECOMP(00437A1C, VtB211A8_Ctor_00437A1C) {
    const u32 o = arg<u32>(ctx, 0), a = arg<u32>(ctx, 1), b = arg<u32>(ctx, 2), c = arg<u32>(ctx, 3), d = arg<u32>(ctx, 4);
    const u32 e = arg<u32>(ctx, 6);
    const f64 f = farg(ctx, 0);
    CmdBase27(ctx, o, arg<u32>(ctx, 7));
    store<u32>(o + 12, a);
    store<u32>(o + 16, b);
    store<u32>(o + 20, c);
    store<u32>(o + 24, d);
    store<f32>(o + 28, static_cast<f32>(f));
    store<u32>(o + 32, 0);
    store<u32>(o, 0x00B211A8);
    store<u8>(o + 36, static_cast<u8>(e));
    ctx.gpr[3] = 0x00B211A8;
}

// T::T(bool a, b, c, d, e, f): base 0x0042C9D0(o, 0), vtable 0x00B211D8,
// the bits 0x00800000 .. 0x00040000 of +8 = a .. f (r3 = the word)
DECOMP(00437D58, VtB211D8_Ctor_00437D58) {
    const u32 o = arg<u32>(ctx, 0);
    u32 bits[6];
    for (u32 k = 0; k < 6; ++k) bits[k] = arg<u32>(ctx, 1 + k) & 1u;
    CmdBase27(ctx, o, 0);
    u32 w = load<u32>(o + 8);
    for (u32 k = 0; k < 6; ++k) w = (w & ~(0x00800000u >> k)) | (bits[k] << (23 - k));
    store<u32>(o, 0x00B211D8);
    store<u32>(o + 8, w);
    ctx.gpr[3] = w;
}

// void Event::Init(obj, a, b, c, x, y): kind 1, EventFill28 (+24 = x),
// +28 = y
DECOMP(00464ED4, Event1_Init_00464ED4) {
    const u32 rec = arg<u32>(ctx, 0);
    EventFill28(ctx, rec, arg<u32>(ctx, 1), 1, arg<u32>(ctx, 2), arg<u32>(ctx, 3), arg<u32>(ctx, 4), arg<u32>(ctx, 5));
    store<u32>(rec + 28, arg<u32>(ctx, 6));
}

// r f(a, b, c, T* o): 0x0024F758(a, b, c, o) when o->vt[12]() is 100, else 0
DECOMP(0048CD50, Obj_IfKind100_0048CD50) {
    const u32 a = arg<u32>(ctx, 0), b = arg<u32>(ctx, 1), c = arg<u32>(ctx, 2), o = arg<u32>(ctx, 3);
    if (static_cast<u32>(VCall(ctx, o, 12, {o})) != 100) {
        ctx.gpr[3] = 0;
        return;
    }
    ctx.gpr[3] = call(ctx, 0x0024F758, {a, b, c, o});
}

// bool f(const Plane* n, const Vec3* p, const Vec3* q, Vec3* out): the
// intersection of the line p..q with the plane (n.xyz, w): false when
// |n . (p - q)| < *0x004A6C4C; else out = p - t (p - q) with t = (n . p +
// w) / (n . (p - q)), true
DECOMP(004A6C50, Plane_LineHit_004A6C50) {
    const u32 n = arg<u32>(ctx, 0), p = arg<u32>(ctx, 1), q = arg<u32>(ctx, 2), out = arg<u32>(ctx, 3);
    const f64 py = LfsAt(p + 4), px = LfsAt(p), pz = LfsAt(p + 8);
    const f32 dy = Sub28(py, LfsAt(q + 4));
    const f32 dx = Sub28(px, LfsAt(q));
    const f32 dz = Sub28(pz, LfsAt(q + 8));
    f32 den = Mul28(LfsAt(n + 4), dy);
    den = Madd28(LfsAt(n), dx, den);
    den = Madd28(LfsAt(n + 8), dz, den);
    ctx.gpr[4] = 0;
    if (std::fabs(static_cast<f64>(den)) < LfsAt(0x004A6C4C)) {
        ctx.fpr[1] = pz;
        ctx.gpr[3] = 0;
        return;
    }
    f32 s = Mul28(LfsAt(n + 4), py);
    s = Madd28(LfsAt(n), px, s);
    s = Madd28(LfsAt(n + 8), pz, s);
    s = Add28(s, LfsAt(n + 12));
    const f32 t = static_cast<f32>(static_cast<f64>(s) / static_cast<f64>(den));
    auto nms = [&](f64 d, f64 b) { return static_cast<f32>(-std::fma(static_cast<f64>(t), d, -b)); };
    const f32 ox = nms(dx, px), oy = nms(dy, py), oz = nms(dz, pz);
    store<f32>(out, ox);
    store<f32>(out + 4, oy);
    store<f32>(out + 8, oz);
    ctx.fpr[1] = static_cast<f64>(oz);
    ctx.gpr[3] = 1;
}

// bool f(T* o, const Spec* s): p = 0x004B3870(s->+0, s->+4); when found,
// resets it (+4/+8/+12/+20/+28/+32 = 0, the u8s +24 = 0, +25 = 1, +0 = 0,
// +40 = -1) and 0x004B3940(p, o, s->+8, s->+0); true when found
DECOMP(004B3BC8, Obj_InitFromSpec_004B3BC8) {
    const u32 o = arg<u32>(ctx, 0), s = arg<u32>(ctx, 1);
    const u32 p = static_cast<u32>(call(ctx, 0x004B3870, {load<u32>(s), load<u32>(s + 4)}));
    if (p == 0) {
        ctx.gpr[3] = 0;
        return;
    }
    store<u32>(p + 4, 0);
    store<u32>(p + 8, 0);
    store<u32>(p + 12, 0);
    store<u32>(p + 20, 0);
    store<u8>(p + 24, 0);
    store<u8>(p + 25, 1);
    store<u32>(p + 28, 0);
    store<u8>(p, 0);
    const u32 a = load<u32>(s + 8);
    store<u32>(p + 40, 0xFFFFFFFFu);
    const u32 b = load<u32>(s);
    store<u32>(p + 32, 0);
    call(ctx, 0x004B3940, {p, o, a, b});
    ctx.gpr[3] = 1;
}

// void f(T* o): o->+32 = the f32 0x011DE0A4 of the singleton 0x011DE094
// (first set up as in Singleton011DE094_Get), then 0x004C0420(o; that
// value)
DECOMP(004C0728, Obj_SetFromSingleton_004C0728) {
    constexpr u32 kObj = 0x011DE094;
    const u32 o = arg<u32>(ctx, 0);
    if (load<u8>(0x011DE280) == 0) {
        const u8 b = load<u8>(kObj + 32);
        store<u32>(kObj + 20, 0);
        store<u32>(kObj + 24, 0);
        store<u8>(kObj + 32, static_cast<u8>(b & 0x3Fu));
        call(ctx, 0x00726E54, {kObj + 92, 0, 400});
        store<u32>(0x011972E8, kObj);
        store<u8>(0x011DE280, 1);
    }
    const u32 v = load<u32>(kObj + 16);
    store<u32>(o + 32, v);
    callf(ctx, 0x004C0420, {o}, {F32Bits28(v)});
}

// void f(T* o, x): the peer p of Effect_Bind4BB7E0(x, o->+12, the u8 +16)
// (WorldObj_GetPeer): Peer_RewindAnims(p) when the u8 +19 is set, and when
// the u8 +18 is set, bit 0x20 of Peer_GetVec(p, the u8 +17)'s u8 +28 =
// (+17 == 0)
DECOMP(004CCE6C, Effect_PeerSetup_004CCE6C) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 e = static_cast<u32>(call(ctx, 0x004BB7E0, {arg<u32>(ctx, 1), load<u32>(o + 12), load<u8>(o + 16)}));
    if (e == 0) {
        ctx.gpr[3] = 0;
        return;
    }
    const u32 p = static_cast<u32>(call(ctx, 0x0057A428, {e}));
    if (p == 0) {
        ctx.gpr[3] = 0;
        return;
    }
    if (load<u8>(o + 19) != 0) call(ctx, 0x0057CDB8, {p});
    ctx.gpr[3] = load<u8>(o + 18);
    if (ctx.gpr[3] == 0) return;
    const u32 n = load<u8>(o + 17);
    const u32 v = static_cast<u32>(call(ctx, 0x0057BE98, {p, n}));
    store<u8>(v + 28, static_cast<u8>((load<u8>(v + 28) & ~0x20u) | (n == 0 ? 0x20u : 0u)));
    ctx.gpr[3] = v;
}

// bool f(T* o, Def* d): clears bit 0x80 of +136; with d, releases the old
// instance +4 (vt[8]) when set, +4 = Effect_Instance4BBABC(d, o) and
// Effect_SetTime(o, *0x004CF8D4); whether d was set
DECOMP(004CF8D8, Effect_SetDef_004CF8D8) {
    const u32 o = arg<u32>(ctx, 0), d = arg<u32>(ctx, 1);
    const u32 w = load<u32>(o + 136) & ~0x80u;
    const u32 c = load<u32>(o + 4);
    store<u32>(o + 136, w);
    if (d == 0) {
        ctx.gpr[3] = 0;
        return;
    }
    if (c != 0) VCall(ctx, c, 8, {c});
    const u32 inst = static_cast<u32>(call(ctx, 0x004BBABC, {d, o}));
    store<u32>(o + 4, inst);
    callf(ctx, 0x004CF710, {o}, {LfsAt(0x004CF8D4)});
    ctx.gpr[3] = 1;
}

// bool f(T* o): false when bit 0x01 of the u8 +304 is set or o->+308->+12
// has its byte [index 0x00F94B2C] set; true when the kind +4 is outside
// 0x29..0x3A, else whether o->vt[152]() is nonzero
DECOMP(00739024, Obj_CanAct_00739024) {
    const u32 o = arg<u32>(ctx, 0);
    ctx.gpr[3] = 0;
    if ((load<u8>(o + 304) & 1u) != 0) return;
    const u32 t = load<u32>(load<u32>(o + 308) + 12);
    if (load<u8>(t + IndexOff(0x00F94B2C)) != 0) return;
    const u32 k = load<u32>(o + 4);
    if (k < 0x29 || k > 0x3A) {
        ctx.gpr[3] = 1;
        return;
    }
    ctx.gpr[3] = static_cast<u32>(VCall(ctx, o, 152, {o})) != 0 ? 1 : 0;
}

// void f(Vec* v, const void* rec): pushes a 74-byte record (memmove, 76-byte
// stride) onto v (+0 data, +4 u16 capacity, +8 u16 count, +10 u16 mark),
// doubling it with PtrVector_Reserve when full (r3 = the old mark)
DECOMP(00742E4C, Rec74List_Push_00742E4C) {
    const u32 v = arg<u32>(ctx, 0), rec = arg<u32>(ctx, 1);
    const u32 cap = load<u16>(v + 4);
    if (load<u16>(v + 8) == cap) call(ctx, o27fn::PtrVector_Reserve, {v, (cap << 1) & 0x1FFFEu});
    call(ctx, 0x0071C3F0, {load<u32>(v) + 76u * load<u16>(v + 8), rec, 74});
    const u32 n = load<u16>(v + 8) + 1u;
    const u32 m = load<u16>(v + 10);
    store<u16>(v + 8, static_cast<u16>(n));
    store<u16>(v + 10, static_cast<u16>(static_cast<s32>(m) > static_cast<s32>(n & 0xFFFFu) ? m : n));
    ctx.gpr[3] = m;
}

// T::~T() (vtable 0x00B20748): clears the PtrVectors +232, +132 and +112,
// then the base destructor 0x00490B34
DECOMP(0077D178, VtB20748_Dtor_0077D178) {
    const u32 o = arg<u32>(ctx, 0);
    store<u32>(o, 0x00B20748);
    for (u32 off : {232u, 132u, 112u}) {
        call(ctx, o19fn::PtrVector_Clear, {o + off});
        call(ctx, o19fn::Nop, {o + off});
    }
    ctx.gpr[3] = call(ctx, 0x00490B34, {o});
}

// ---------------------------------------------------------------- sweep: 43 instructions
// T* f(?, XmlCursor* c): new 16-byte record (base 0x004BB3B0(p, c), vtable
// 0x00B10378) whose f32 +12 is read from 0x0451539B (default *0x00089130)
DECOMP(00089138, NewXmlF32_00089138) { NewXmlF32_28(ctx, 0x00B10378, 0x0451539B, 0x00089130); }

// void f(T* o, x): for the effect e = Effect_Bind4BB7E0(x, o->+12, 0) of
// kind 0x02235C33 (+276) with an owner +280, its component 34
// (Obj_FindComponent(owner, 34, 0)) gets 0x00282408(c, 0), +132 = o->+16
// and +128 = 100.0f
DECOMP(00089CEC, Effect_SetComp34_00089CEC) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 e = static_cast<u32>(call(ctx, 0x004BB7E0, {arg<u32>(ctx, 1), load<u32>(o + 12), 0}));
    ctx.gpr[3] = e;
    if (e == 0 || load<u32>(e + 276) != 0x02235C33u) return;
    const u32 q = load<u32>(e + 280);
    ctx.gpr[3] = q;
    if (q == 0) return;
    const u32 c = static_cast<u32>(call(ctx, 0x000D1288, {q, 34, 0}));
    ctx.gpr[3] = c;
    if (c == 0) return;
    call(ctx, 0x00282408, {c, 0});
    const u32 f = load<u32>(o + 16);
    store<u32>(c + 132, f);
    store<u32>(c + 128, 0x42C80000u);
    ctx.fpr[1] = F32Bits28(f);
    ctx.gpr[3] = 0x42C80000u;
}

// T* f(?, XmlCursor* c): new 16-byte record (base 0x004BB3B0(p, c), vtable
// 0x00B117B0) whose f32 +12 is read from 0x0451539B (default *0x000A4310)
DECOMP(000A4318, NewXmlF32_000A4318) { NewXmlF32_28(ctx, 0x00B117B0, 0x0451539B, 0x000A4310); }

// T::~T() (vtable 0x00B11D80): destroys and frees the handle table +28
// (Handles_Destroy_0042725C, Mem_FreeFrom), clears it, releases the global
// 0x00F558D0 (vt[8], cleared), then the base destructor 0x00427E2C
DECOMP(000A86D8, VtB11D80_Dtor_000A86D8) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 t = load<u32>(o + 28);
    store<u32>(o, 0x00B11D80);
    if (t != 0) {
        call(ctx, 0x0042725C, {t});
        call(ctx, o25fn::Mem_FreeFrom, {t, 0});
    }
    store<u32>(o + 28, 0);
    const u32 g = load<u32>(0x00F558D0);
    VCall(ctx, g, 8, {g});
    store<u32>(0x00F558D0, 0);
    ctx.gpr[3] = call(ctx, 0x00427E2C, {o});
}

// The search of 0x000CB888 / 0x000D92B0 in the pointer list o->+soff (+0
// data, +8 u16 count): the first entry after `after` (from the start when
// after is null or missing) whose u32 +koff is key, or 0
static void FindAfter28(PPUContext& ctx, u32 soff, u32 koff) {
    const u32 key = arg<u32>(ctx, 1), after = arg<u32>(ctx, 2);
    const u32 s = load<u32>(arg<u32>(ctx, 0) + soff);
    auto count = [&]() -> u32 { return s != 0 ? load<u16>(s + 8) : 0u; };
    u32 i = 0;
    if (after != 0)
        for (u32 j = 0; j < count();)
            if (load<u32>(load<u32>(s) + 4 * j++) == after) {
                i = j;
                break;
            }
    for (; i < count(); ++i) {
        const u32 e = load<u32>(load<u32>(s) + 4 * i);
        if (load<u32>(e + koff) == key) {
            ctx.gpr[3] = e;
            return;
        }
    }
    ctx.gpr[3] = 0;
}

// T* f(T* o, u32 key, T* after): FindAfter28 on the list +16, key +12
DECOMP(000CB888, List_FindAfter_000CB888) { FindAfter28(ctx, 16, 12); }

// T* f(T* o, u32 key, T* after): FindAfter28 on the list +248, key +4
DECOMP(000D92B0, List_FindAfter_000D92B0) { FindAfter28(ctx, 248, 4); }

// void f(T* o, Hit* h): when o->vt[356]() * h->+52 > *0x000EDA38,
// o->vt[352](o, -, h->+4, the u16 h->+12)
DECOMP(000EDA40, Obj_ApplyHit_000EDA40) {
    const u32 o = arg<u32>(ctx, 0), h = arg<u32>(ctx, 1);
    VCall(ctx, o, 356, {o});
    const f32 v = Mul28(ctx.fpr[1], LfsAt(h + 52));
    ctx.fpr[1] = static_cast<f64>(v);
    if (!(static_cast<f64>(v) > LfsAt(0x000EDA38))) return;
    // The OPD is loaded into r4 (an unused argument slot) and its entry
    // into r7.
    const u32 vt = load<u32>(o);
    const u32 a = load<u32>(h + 4), b = load<u16>(h + 12);
    const u32 opd = load<u32>(vt + 352);
    call_ptr(ctx, opd, {o, opd, a, b, load<u32>(opd)});
}

// u32 f(T* o, ?, U* t): bit 0x1 = bit 0x1 of o's word +188; bit 0x8 when t
// targets o (t->+428 with t's flag 0x20 of +192, +392 with 0x1000, or +400
// with 0x4)
DECOMP(00127E20, Obj_TargetFlags_00127E20) {
    const u32 o = arg<u32>(ctx, 0), t = arg<u32>(ctx, 2);
    u32 r = (load<u32>(o + 188) & 1u) != 0 ? 1u : 0u;
    if (t != 0 && (((load<u32>(t + 192) & 0x20u) != 0 && load<u32>(t + 428) == o) ||
                   ((load<u32>(t + 192) & 0x1000u) != 0 && load<u32>(t + 392) == o) ||
                   ((load<u32>(t + 192) & 0x4u) != 0 && load<u32>(t + 400) == o)))
        r |= 8u;
    ctx.gpr[3] = r;
}

// void f(T* a, U* b): Obj_CopyQueryEx_000DFBFC(a, b), then a's +240..+248,
// +256..+264, +280..+292 (floats, raw), +272, +276 into b's +208..+216,
// +224..+232, +248..+260, +240, +244, and bit 0x1000 of b->+20 = bit
// 0x40000000 of a->+132 (r3 = that word, f1 = a->+292)
DECOMP(00177FB4, Obj_CopyState_00177FB4) {
    const u32 a = arg<u32>(ctx, 0), b = arg<u32>(ctx, 1);
    call(ctx, 0x000DFBFC, {a, b});
    auto c = [&](u32 d, u32 s) { store<u32>(b + d, load<u32>(a + s)); };
    c(208, 240);
    c(212, 244);
    c(216, 248);
    c(224, 256);
    c(228, 260);
    u32 f = load<u32>(a + 264);
    const u32 w272 = load<u32>(a + 272);
    store<u32>(b + 232, f);
    f = load<u32>(a + 280);
    const u32 w276 = load<u32>(a + 276);
    store<u32>(b + 248, f);
    store<u32>(b + 240, w272);
    store<u32>(b + 244, w276);
    f = load<u32>(a + 284);
    u32 w20 = load<u32>(b + 20);
    store<u32>(b + 252, f);
    w20 = (w20 & ~0x1000u) | (std::rotl(load<u32>(a + 132), 14) & 0x1000u);
    store<u32>(b + 20, w20);
    c(256, 288);
    f = load<u32>(a + 292);
    store<u32>(b + 260, f);
    ctx.fpr[1] = F32Bits28(f);
    ctx.gpr[3] = w20;
}

// void f(T* o, Msg* m): for the message 0x86B846D5, the effect +356 gets
// the hash 0xCA8F18D3, +464 = 0 and o->vt[28](o, 0, 2); then
// Obj_OnEvent_000E5E44(o, m)
DECOMP(001B17F4, Obj_OnMsg_001B17F4) {
    const u32 o = arg<u32>(ctx, 0), m = arg<u32>(ctx, 1);
    if (load<u32>(m + 8) == 0x86B846D5u) {
        const u32 e = load<u32>(o + 356);
        if (e != 0) call(ctx, o27fn::Effect_SetHash, {e, 0xCA8F18D3u});
        const u32 vt = load<u32>(o);
        store<u32>(o + 464, 0);
        VCallVt(ctx, vt, 28, {o, 0, 2});
    }
    ctx.gpr[3] = call(ctx, 0x000E5E44, {o, m});
}

// bool f(T* o, f32 a, f32 b): false (after o->vt[420](o, 5, 0)) when the
// flag 0x08 of the u8 +576 and bit 0x20000000 of +552 are clear and b >=
// *0x001C1C98 and a >= b; true otherwise
DECOMP(001C1CA0, Obj_RangeGate_001C1CA0) {
    const u32 o = arg<u32>(ctx, 0);
    const f64 a = farg(ctx, 0), b = farg(ctx, 1);
    ctx.gpr[3] = 1;
    if ((load<u8>(o + 576) & 0x08u) != 0 || (load<u32>(o + 552) & 0x20000000u) != 0) return;
    if (!(b >= LfsAt(0x001C1C98)) || !(a >= b)) return;
    VCall(ctx, o, 420, {o, 5, 0});
    ctx.gpr[3] = 0;
}

// void f(T* o): Target_GetDef0D951C(o); with the layer object +620 and
// the world list kWorldPtr->* + 0x1281E0 set, Obj_PopLayer(list, layer;
// *0x01052D44) and the layer released (vt[4], +620 = 0)
DECOMP(001DEC60, Obj_PopLayer_001DEC60) {
    const u32 o = arg<u32>(ctx, 0);
    call(ctx, 0x000D951C, {o});
    const u32 c = load<u32>(o + 620);
    ctx.gpr[3] = c;
    if (c == 0) return;
    const u32 l = load<u32>(load<u32>(kWorldPtr) + 0x1281E0);
    if (l == 0) return;
    callf(ctx, 0x000FBB88, {l, c, l, 0x01052CFC}, {LfsAt(0x01052D44)});
    const u32 c2 = load<u32>(o + 620);
    ctx.gpr[3] = c2;
    if (c2 == 0) return;
    VCall(ctx, c2, 4, {c2});
    store<u32>(o + 620, 0);
    ctx.gpr[3] = 0;
}

// void f(T* a, U* b): Obj_CopyQueryEx_000DFBFC(a, b), then a's +240..+248,
// +256..+264, +276, +280 (floats, raw) and +272 into b's +208..+216,
// +224..+232, +244, +248 and +240; bit 0x1000 of b->+20 = bit 0x40000000
// of a->+132; the u8 b->+252 = the u8 a->+284, then bit 0x00800000 of the
// word b->+252 = that of a->+284 (r3 = a->+284, f1 = a->+280)
DECOMP(001FF404, Obj_CopyState_001FF404) {
    const u32 a = arg<u32>(ctx, 0), b = arg<u32>(ctx, 1);
    call(ctx, 0x000DFBFC, {a, b});
    auto c = [&](u32 d, u32 s) { store<u32>(b + d, load<u32>(a + s)); };
    c(208, 240);
    c(212, 244);
    c(216, 248);
    c(224, 256);
    c(228, 260);
    u32 f = load<u32>(a + 264);
    const u32 w272 = load<u32>(a + 272);
    store<u32>(b + 232, f);
    f = load<u32>(a + 276);
    store<u32>(b + 240, w272);
    store<u32>(b + 244, f);
    f = load<u32>(a + 280);
    u32 w20 = load<u32>(b + 20);
    store<u32>(b + 248, f);
    w20 = (w20 & ~0x1000u) | (std::rotl(load<u32>(a + 132), 14) & 0x1000u);
    store<u32>(b + 20, w20);
    store<u8>(b + 252, load<u8>(a + 284));
    const u32 w = load<u32>(a + 284);
    store<u32>(b + 252, (load<u32>(b + 252) & ~0x00800000u) | (w & 0x00800000u));
    ctx.fpr[1] = F32Bits28(f);
    ctx.gpr[3] = w;
}

// void f(T* o): syncs bit 31 of +32 (in the world) to bit 30 (wanted):
// when they differ, bit 31 = bit 30 and, with the body +28 and the world
// o->+24->+4 set, Body_Add931D10(world, 0x009CEEAC(body)) or
// Body_Remove931F00(world, ...)
DECOMP(002BC128, Body_SyncInWorld_002BC128) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 w = load<u32>(o + 32);
    const u32 want = (w >> 30) & 1u;
    ctx.gpr[3] = w;
    if ((w >> 31) == want) return;
    const u32 c = load<u32>(o + 28);
    store<u32>(o + 32, (w & 0x7FFFFFFFu) | (want << 31));
    ctx.gpr[3] = c;
    if (c == 0) return;
    const u32 world = load<u32>(load<u32>(o + 24) + 4);
    const u32 r = static_cast<u32>(call(ctx, 0x009CEEAC, {c}));
    if (world == 0) return;
    call(ctx, want != 0 ? 0x00931D10u : 0x00931F00u, {world, r});
}

// void f(Rec* o, T* a, b, c, bool d, bool e): releases the old +8
// (Level_Release2D9A14), then +0 = 1, +4 = a, +8 = b, +12 = c, +16 =
// 0x20000000 | d << 31 | e << 30, and sets bit 0x20000 of a's u64 +184
DECOMP(002E1E98, Rec_Set_002E1E98) {
    const u32 o = arg<u32>(ctx, 0), a = arg<u32>(ctx, 1), b = arg<u32>(ctx, 2), c = arg<u32>(ctx, 3);
    const u32 d = arg<u32>(ctx, 4), e = arg<u32>(ctx, 5);
    if (load<u32>(o + 8) != 0) call(ctx, 0x002D9A14, {load<u32>(o + 8)});
    const u32 w = 0x20000000u | (d << 31) | ((e & 1u) << 30);
    store<u32>(o, 1);
    store<u32>(o + 4, a);
    store<u32>(o + 8, b);
    store<u32>(o + 12, c);
    store<u32>(o + 16, w);
    ctx.gpr[3] = w;
    if (a == 0) return;
    const u64 v = load<u64>(a + 184) | 0x20000u;
    store<u64>(a + 184, v);
    ctx.gpr[3] = v;
}

// void f(T* o, const char* fmt, ...): formats into a 256-byte local buffer
// (String_VFormat, cleared first), then +32 = 8. The variadic registers are
// spilled to the caller's parameter area as the original does.
DECOMP(002EA67C, Obj_Printf_002EA67C) {
    const u32 o = arg<u32>(ctx, 0), fmt = arg<u32>(ctx, 1);
    const u32 sp = static_cast<u32>(ctx.gpr[1]);
    store<u64>(sp + 64, ctx.gpr[5]);
    store<u32>(sp + 60, fmt);
    for (u32 k = 6; k <= 10; ++k) store<u64>(sp + 64 + 8 * (k - 5), ctx.gpr[k]);
    Stack frame(ctx, 288);
    const u32 buf = frame.addr();
    store<u8>(buf, 0);
    call(ctx, 0x00726E54, {buf + 1, 0, 255});
    call(ctx, 0x00555D48, {buf, 256, fmt, sp + 64});
    store<u32>(o + 32, 8);
    ctx.gpr[3] = 8;
}

// bool f(T* o): posts Msg_Post(o, 0xA07A77E3, 0x8024237B, the index of o's
// kind +168 among 0x37C9BF3C..0x37C9BF3E or 0x0FFFFFFF); false for null o
DECOMP(00302CF0, Obj_PostKind_00302CF0) {
    const u32 o = arg<u32>(ctx, 0);
    if (o == 0) {
        ctx.gpr[3] = 0;
        return;
    }
    const u32 k = load<u32>(o + 168);
    const u32 v = k == 0x37C9BF3Cu ? 0u : k == 0x37C9BF3Du ? 1u : k == 0x37C9BF3Eu ? 2u : 0x0FFFFFFFu;
    call(ctx, o24fn::Msg_Post, {o, 0xA07A77E3u, 0x8024237Bu, v});
    ctx.gpr[3] = 1;
}

// void f(T* o, x): the five slots +2576.. = the values of the keys +2572 +
// i in the map 0x00F65BE8->* + 4 (SortedMap_Find733F8C), then
// 0x00338560(o) and 0x00325CE4(o, x)
DECOMP(003386C0, Obj_LoadSlots_003386C0) {
    Stack frame(ctx, 160);
    const u32 key = frame.addr() + 112;
    const u32 o = arg<u32>(ctx, 0), x = arg<u32>(ctx, 1);
    for (u32 i = 0; i < 5; ++i) {
        store<u32>(key, load<u32>(o + 2572) + i);
        const u32 r = static_cast<u32>(call(ctx, 0x00733F8C, {load<u32>(0x00F65BE8) + 4, key}));
        store<u32>(o + 4 * i + 2576, load<u32>(r));
    }
    call(ctx, 0x00338560, {o});
    ctx.gpr[3] = call(ctx, 0x00325CE4, {o, x});
}

// void f(T* o, u32 key): when +32 is set, the value v of key in the map
// 0x0106BC40 (SortedMap_Find76DD98; 0 for a null key) goes to the last
// item of o->+24's list (+16 data, +20 count) with Item_SetVoiceA4CC64
DECOMP(00350860, Obj_SetLastVoice_00350860) {
    Stack frame(ctx, 128);
    const u32 slot = frame.addr() + 112;
    const u32 o = arg<u32>(ctx, 0), key = arg<u32>(ctx, 1);
    ctx.gpr[3] = load<u32>(o + 32);
    if (ctx.gpr[3] == 0) return;
    u32 v = 0;
    if (key != 0) {
        store<u32>(slot, key);
        v = load<u32>(static_cast<u32>(call(ctx, 0x0076DD98, {0x0106BC40, slot})));
        ctx.gpr[3] = v;
        if (v == 0) return;
    }
    ctx.gpr[3] = v;
    const u32 p = load<u32>(o + 24);
    const s32 n = load<s32>(p + 20);
    if (n <= 0) return;
    const u32 e = load<u32>(load<u32>(p + 16) + 4 * static_cast<u32>(n - 1));
    if (e == 0) return;
    call(ctx, 0x00A4CC64, {e, v});
}

// void f(T* o, Vec3* out, u32 i): out = the vector 0x01164B30, then for i
// below o->+8->+152 with a node o->+328[i].+12 (16-byte records), out =
// *node->vt[36]() (raw copies; f1 = the last float)
DECOMP(0036336C, Obj_GetNodePos_0036336C) {
    const u32 o = arg<u32>(ctx, 0), out = arg<u32>(ctx, 1), i = arg<u32>(ctx, 2);
    const u32 q = load<u32>(o + 8);
    store<u32>(out, load<u32>(0x01164B30));
    const u32 y = load<u32>(0x01164B34);
    const u32 n = load<u32>(q + 152);
    store<u32>(out + 4, y);
    u32 z = load<u32>(0x01164B38);
    store<u32>(out + 8, z);
    ctx.fpr[1] = F32Bits28(z);
    if (!(i < n)) return;
    const u32 c = load<u32>(load<u32>(o + 328) + (i << 4) + 12);
    ctx.gpr[3] = c;
    if (c == 0) return;
    const u32 v = static_cast<u32>(VCall(ctx, c, 36, {c}));
    CopyVec3F28(ctx, out, v);
}

// void f(T* o, f32 t): v = t * k + k (k = *0x003A67B8) goes to the f32 +44
// of the node a->+208->+132->+48->+4->+128 for each of a = o->+32 and
// o->+36 that is set (when that node is set)
DECOMP(003A67C0, Obj_SetNodeValue_003A67C0) {
    const u32 o = arg<u32>(ctx, 0);
    const f64 k = LfsAt(0x003A67B8);
    const u32 a = load<u32>(o + 32), b = load<u32>(o + 36);
    const f32 v = Madd28(farg(ctx, 0), k, k);
    ctx.fpr[1] = static_cast<f64>(v);
    auto node = [](u32 x) { return load<u32>(load<u32>(load<u32>(load<u32>(load<u32>(x + 208) + 132) + 48) + 4) + 128); };
    if (a != 0) {
        const u32 t = node(a);
        if (t != 0 && t + 8 != 0) store<f32>(t + 44, v);
    }
    ctx.gpr[3] = b;
    if (b == 0) return;
    const u32 t = node(b);
    ctx.gpr[3] = t;
    if (t != 0 && t + 8 != 0) store<f32>(t + 44, v);
}

// f32 f(T* o): the distance between the positions +144 of o->+96 and
// o->+108->+96, or *0x003ABB74 when the difference equals the vector
// 0x01164B40. Quirk: the z word of that check is the stale stack word
// (r1 - 8), read before the difference is stored there, as the original
// does.
DECOMP(003ABD78, Obj_Distance_003ABD78) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 sp = static_cast<u32>(ctx.gpr[1]);
    const u32 q = load<u32>(load<u32>(o + 108) + 96) + 144;
    const u32 p = load<u32>(o + 96) + 144;
    const f32 dx = Sub28(LfsAt(q), LfsAt(p));
    const f32 dy = Sub28(LfsAt(q + 4), LfsAt(p + 4));
    const f32 dz = Sub28(LfsAt(q + 8), LfsAt(p + 8));
    const u32 stale = load<u32>(sp - 8);
    const u32 g2 = load<u32>(0x01164B48);
    store<f32>(sp - 16, dx);
    store<f32>(sp - 12, dy);
    store<f32>(sp - 8, dz);
    const u64 g01 = load<u64>(0x01164B40);
    const u64 r = (load<u64>(sp - 16) - g01) | static_cast<u32>(stale - g2);
    ctx.gpr[3] = r;
    if (r == 0) {
        ctx.fpr[1] = LfsAt(0x003ABB74);
        return;
    }
    f32 s = Mul28(dy, dy);
    s = Madd28(dx, dx, s);
    s = Madd28(dz, dz, s);
    ctx.fpr[1] = static_cast<f64>(static_cast<f32>(std::sqrt(static_cast<f64>(s))));
}

// void f(T* o, u8 on): when on, each 76-byte record of o->+112 (u16 count
// +120) whose first word is 0 gets rec->+40->vt[40](it, 0, 1) (r3 = the
// count)
DECOMP(003D8A80, Obj_ResetRecords_003D8A80) {
    const u32 o = arg<u32>(ctx, 0);
    if ((arg<u32>(ctx, 1) & 0xFFu) == 0) return;
    u32 i = 0;
    for (;; ++i) {
        const u32 n = load<u16>(o + 120);
        if (!(i < n)) {
            ctx.gpr[3] = n;
            return;
        }
        const u32 e = load<u32>(o + 112) + 76 * i;
        if (load<u32>(e) != 0) continue;
        const u32 c = load<u32>(e + 40);
        VCall(ctx, c, 40, {c, 0, 1});
    }
}

// bool f(T* o): true unless the target +12 (with flag 0x8 of +192) has a
// controller +380 whose sub-object +80 ->vt[32](it, 0) is nonzero; when
// that returns 0 the controller becomes +384
DECOMP(0042DA04, Obj_CheckController_0042DA04) {
    const u32 q = load<u32>(arg<u32>(ctx, 0) + 12);
    ctx.gpr[3] = 1;
    if (q == 0 || (load<u32>(q + 192) & 0x8u) == 0) return;
    const u32 s = load<u32>(q + 380);
    if (s == 0) return;
    if (static_cast<u32>(VCall(ctx, s + 80, 32, {s + 80, 0})) == 0) {
        store<u32>(q + 380, load<u32>(q + 384));
        ctx.gpr[3] = 1;
        return;
    }
    ctx.gpr[3] = 0;
}

// void f(T* o, u8 on): the u8 +28 = 0, then c->vt[28](c, on) for each c of
// the list +32 (next +72)
DECOMP(004562F4, Obj_ForEachSet_004562F4) {
    const u32 o = arg<u32>(ctx, 0), on = arg<u32>(ctx, 1) & 0xFFu;
    u32 c = load<u32>(o + 32);
    store<u8>(o + 28, 0);
    for (; c != 0; c = load<u32>(c + 72)) VCall(ctx, c, 28, {c, on});
}

// f32 f(f32 a, f32 b, f32 x, f32 y): a + (b - a) * (k5 - w) with the
// constants k = 0x004A8684: w = the table 0x00AEB6D4 [6 * (s32)(x * k4) +
// clamp((s32)(y * k2 + k3), 1, 6) - 1] for k0 <= x <= k1, else w = x
DECOMP(004A86A0, Curve_Lookup_004A86A0) {
    constexpr u32 k = 0x004A8684;
    const f64 a = farg(ctx, 0), x = farg(ctx, 2), y = farg(ctx, 3);
    const f32 d = Sub28(farg(ctx, 1), a);
    f64 w = x;
    if (!(x < LfsAt(k)) && !(x > LfsAt(k + 4))) {
        s32 i = static_cast<s32>(static_cast<u32>(rt::fctiw(static_cast<f64>(Madd28(y, LfsAt(k + 8), LfsAt(k + 12))), true)));
        if (i < 1) i = 1;
        i -= 1;
        if (!(i < 6)) i = 5;
        const s32 j = static_cast<s32>(static_cast<u32>(rt::fctiw(static_cast<f64>(Mul28(x, LfsAt(k + 16))), true)));
        w = LfsAt(0x00AEB6D4 + 4 * static_cast<u32>(j * 6 + i));
    }
    const f32 m = Sub28(LfsAt(k + 20), w);
    ctx.fpr[1] = static_cast<f64>(Madd28(d, m, a));
    ctx.gpr[3] = k;
}

// Static init: the five list heads 0x010C2888, 0x010C2820, 0x010C28F0,
// 0x010C27B8, 0x010C2750 (+0 / +4 / +100 = 0, +12 = itself, u16 +96 = 0,
// u8 +8 = 1)
DECOMP(004D0BE4, Lists_StaticInit_004D0BE4) {
    constexpr u32 a = 0x010C2888, b = 0x010C2820;
    store<u32>(a, 0);
    store<u32>(b, 0);
    auto head = [](u32 h, bool zero0) {
        if (zero0) store<u32>(h, 0);
        store<u32>(h + 4, 0);
        store<u32>(h + 12, h);
        store<u32>(h + 100, 0);
        store<u16>(h + 96, 0);
        store<u8>(h + 8, 1);
    };
    store<u32>(a + 12, a);
    store<u32>(a + 4, 0);
    store<u32>(a + 100, 0);
    store<u16>(a + 96, 0);
    store<u8>(a + 8, 1);
    store<u32>(b + 4, 0);
    store<u32>(b + 12, b);
    store<u32>(b + 100, 0);
    store<u16>(b + 96, 0);
    store<u8>(b + 8, 1);
    head(0x010C28F0, true);
    head(0x010C27B8, true);
    constexpr u32 e = 0x010C2750;
    store<u32>(e, 0);
    store<u32>(e + 4, 0);
    store<u32>(e + 12, e);
    store<u16>(e + 96, 0);
    store<u32>(e + 100, 0);
    store<u8>(e + 8, 1);
    ctx.gpr[3] = 0x010C0000;
}

// T* f(T* o): o->+16 when the flag 0x80 of the u8 +24 is set; else the
// first c of the chain o, ->+16, ... whose vt[8]() is nonzero, returning
// c->vt[8]() (called again), or 0
DECOMP(004F9434, Xml_FindResolved_004F9434) {
    const u32 o = arg<u32>(ctx, 0);
    if ((load<u8>(o + 24) & 0x80u) != 0) {
        ctx.gpr[3] = load<u32>(o + 16);
        return;
    }
    for (u32 c = o; c != 0; c = load<u32>(c + 16))
        if (static_cast<u32>(VCall(ctx, c, 8, {c})) != 0) {
            VCall(ctx, c, 8, {c});
            return;
        }
    ctx.gpr[3] = 0;
}

// u32 f(XmlCursor* c, u32 hash, u32* out, const u32* table, u32 n, u32
// dflt): reads the hash (XmlCursor_ReadHash) and stores its index in table
// to *out (returns 0); returns the read error, or 3 when not in the table,
// with *out = dflt
DECOMP(004FA458, Xml_ReadEnum_004FA458) {
    Stack frame(ctx, 160);
    const u32 v = frame.addr() + 112;
    const u32 out = arg<u32>(ctx, 2), table = arg<u32>(ctx, 3), n = arg<u32>(ctx, 4), dflt = arg<u32>(ctx, 5);
    u32 rc = static_cast<u32>(call(ctx, o28fn::XmlCursor_ReadHash, {arg<u32>(ctx, 0), arg<u32>(ctx, 1), v, 0}));
    if (rc == 0) {
        for (u32 i = 0; i < n; ++i)
            if (load<u32>(table + 4 * i) == load<u32>(v)) {
                store<u32>(out, i);
                ctx.gpr[3] = 0;
                return;
            }
        rc = 3;
    }
    store<u32>(out, dflt);
    ctx.gpr[3] = rc;
}

// T::~T() deleting (vtable 0x00B16B00): clears the PtrVectors +420 and
// +400, base vtable 0x00B15970 and Thunk_000E2A34(o), SmallFree(o, 448)
DECOMP(0075483C, VtB16B00_Delete_0075483C) {
    const u32 o = arg<u32>(ctx, 0);
    if (o == 0) return;
    store<u32>(o, 0x00B16B00);
    for (u32 off : {420u, 400u}) {
        call(ctx, o19fn::PtrVector_Clear, {o + off});
        call(ctx, o19fn::Nop, {o + off});
    }
    store<u32>(o, 0x00B15970);
    call(ctx, 0x000E2A34, {o});
    ctx.gpr[3] = call(ctx, o19fn::SmallFree, {o, 448});
}

// T::~T() (vtable 0x00B1D370): +376 = 0; unless the top bit of +380 is
// set, the Havok allocator 0x011DF31C ->vt[20](it, +372, +380 << 2) frees
// the buffer; +372 = 0, +380 = 0x80000000, then the base 0x00928008(o)
DECOMP(00761A68, VtB1D370_Dtor_00761A68) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 w = load<u32>(o + 380);
    store<u32>(o, 0x00B1D370);
    store<u32>(o + 376, 0);
    if ((w & 0x80000000u) == 0) VCall(ctx, 0x011DF31C, 20, {0x011DF31C, load<u32>(o + 372), Sx(w << 2)});
    store<u32>(o + 372, 0);
    store<u32>(o + 380, 0x80000000u);
    ctx.gpr[3] = call(ctx, 0x00928008, {o});
}

// r f(a, b, c, Q* d, e, f): 0x009D3F98(a, b, c, d, e, f) unless the byte
// at base + 232 is 7 (r3 = f then), where base = q + the s8 q->+16 for q =
// d->+4 when the s8 q->+24 is 1. Quirk: otherwise base is address 0.
DECOMP(00765DD0, Obj_CallUnless7_00765DD0) {
    const u32 a = arg<u32>(ctx, 0), b = arg<u32>(ctx, 1), c = arg<u32>(ctx, 2), d = arg<u32>(ctx, 3);
    const u32 e = arg<u32>(ctx, 4), f = arg<u32>(ctx, 5);
    const u32 q = load<u32>(d + 4);
    u32 base = 0;
    if (static_cast<s8>(load<u8>(q + 24)) == 1) base = q + static_cast<u32>(static_cast<s32>(static_cast<s8>(load<u8>(q + 16))));
    if (load<u8>(base + 232) == 7) {
        ctx.gpr[3] = f;
        return;
    }
    ctx.gpr[3] = call(ctx, 0x009D3F98, {a, b, c, d, e, f});
}

// T::~T() deleting (vtable 0x00B1F978): frees +164 and then +148
// (Mem_FreeFrom, cleared; vtables 0x00B1F978 / 0x00B1F6A0 while doing so),
// base vtable 0x00B1F668, Mem_FreeFrom(o, 0)
DECOMP(00773464, VtB1F978_Delete_00773464) {
    const u32 o = arg<u32>(ctx, 0);
    if (o == 0) return;
    if (load<u32>(o + 164) != 0) {
        store<u32>(o, 0x00B1F978);
        call(ctx, o25fn::Mem_FreeFrom, {load<u32>(o + 164), 0});
        store<u32>(o + 164, 0);
    }
    if (load<u32>(o + 148) != 0) {
        store<u32>(o, 0x00B1F6A0);
        call(ctx, o25fn::Mem_FreeFrom, {load<u32>(o + 148), 0});
        store<u32>(o + 148, 0);
    }
    store<u32>(o, 0x00B1F668);
    ctx.gpr[3] = call(ctx, o25fn::Mem_FreeFrom, {o, 0});
}
