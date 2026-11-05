// World objects, part 19: the shared bodies of the template instances
// (functions that differ only in their constants: vtables, callees, sizes,
// this-adjustments) found by the project-wide survey of normalised
// disassembly. Each instance in obj19_tmpl*.cpp is one DECOMP line calling
// one of these with the constants scanned from its own disassembly.
//
// Register notes: where the original leaves a value in a volatile register
// that a caller could read (r4 = the vtable it stored, ...), the helper sets
// it explicitly, as the comments say.
#pragma once

#include <bit>

#include "opentit/decomp.h"

namespace sly4 {
namespace obj19 {

using namespace opentit;
using namespace opentit::decomp;

namespace o19fn {
constexpr u32 Mem_Alloc = 0x0054ED38;     // (size, pool, align) -> pointer or 0
constexpr u32 Mem_FreeFrom = 0x0054EE6C;  // (p, pool)
constexpr u32 SmallFree = 0x00552420;     // (p, size)
constexpr u32 PtrVector_Clear = 0x0054EC1C;  // (PtrVector*)
constexpr u32 SmallAlloc = 0x0055238C;     // (size) -> pointer or 0
constexpr u32 Nop = 0x0054EC18;              // the shared empty function (the vector's inlined dtor tail)
}  // namespace o19fn

// ---------------------------------------------------------------- trivial bodies

// A tail / plain wrapper: the target with the same register arguments
// (r3-r10 untouched), its r3 / f1 left as the result.
inline void TThunk(PPUContext& ctx, u32 target) { call(ctx, target, {ctx.gpr[3]}); }

// A this-adjusting wrapper (the secondary-base entry of a virtual): the
// target with this + delta (32-bit) and the other arguments untouched.
inline void TAdjThunk(PPUContext& ctx, s32 delta, u32 target) {
    call(ctx, target, {static_cast<u32>(arg<u32>(ctx, 0) + static_cast<u32>(delta))});
}

// ---------------------------------------------------------------- destructors

// ~T(): *this = T's vtable, then the base destructor (r4 = the vtable, as
// the original's call sequence leaves it).
inline void TDtor(PPUContext& ctx, u32 vt, u32 base) {
    const u32 o = arg<u32>(ctx, 0);
    store<u32>(o, vt);
    call(ctx, base, {o, vt});
}

// A destructor with nothing but the vtable store (r4 = the vtable left).
inline void TSetVtbl(PPUContext& ctx, u32 vt) {
    store<u32>(arg<u32>(ctx, 0), vt);
    ctx.gpr[4] = vt;
}

// The deleting destructor: nothing for a null this (r3 = 0), else ~T()
// and the free (Mem_FreeFrom(this, 0) or SmallFree(this, size)).
inline void TDelete(PPUContext& ctx, u32 vt, u32 base, u32 free_fn, u32 size) {
    const u32 o = arg<u32>(ctx, 0);
    if (o == 0) return;
    store<u32>(o, vt);
    call(ctx, base, {o, vt});
    call(ctx, free_fn, {o, size});
}

// The deleting destructor that calls the out-of-line ~T() (r4 untouched
// into it), then the free.
inline void TDeleteCall(PPUContext& ctx, u32 dtor, u32 free_fn, u32 size) {
    const u32 o = arg<u32>(ctx, 0);
    if (o == 0) return;
    call(ctx, dtor, {o});
    call(ctx, free_fn, {o, size});
}

// The same entered through a secondary base: this + delta first (r3 = the
// adjusted pointer, 0 included, when nothing is freed).
inline void TAdjDeleteCall(PPUContext& ctx, s32 delta, u32 dtor, u32 free_fn, u32 size) {
    const u32 o = static_cast<u32>(arg<u32>(ctx, 0) + static_cast<u32>(delta));
    if (o == 0) return;
    call(ctx, dtor, {o});
    call(ctx, free_fn, {o, size});
}

// The deleting destructor of a class without a base destructor: r5 / r6 =
// this and the vtable as the original leaves them for the free; a null
// this leaves r4 = 0.
inline void TDeleteNoBase(PPUContext& ctx, u32 vt, u32 free_fn, u32 size) {
    const u32 o = arg<u32>(ctx, 0);
    if (o == 0) {
        ctx.gpr[4] = 0;
        return;
    }
    store<u32>(o, vt);
    call(ctx, free_fn, {o, size, o, vt});
}

// ---------------------------------------------------------------- factories

// T* New(a1): Mem_Alloc(size, 0, 16), then T(p, a1) when it worked;
// returns the object or 0.
inline void TNew1(PPUContext& ctx, u32 size, u32 ctor) {
    const u32 a1 = arg<u32>(ctx, 1);
    const u32 p = static_cast<u32>(call(ctx, o19fn::Mem_Alloc, {size, 0, 16}));
    if (p != 0) call(ctx, ctor, {p, a1});
    ret(ctx, p);
}

// ---------------------------------------------------------------- batch 2

// Returns a 32-bit constant (lis / ori: zero-extended).
inline void TRetU32(PPUContext& ctx, u32 v) { ctx.gpr[3] = v; }

// Returns this + k (32-bit), e.g. the address of a member or base.
inline void TRetAdj(PPUContext& ctx, s32 k) { ctx.gpr[3] = static_cast<u32>(arg<u32>(ctx, 0) + static_cast<u32>(k)); }

// f32 Get(): the float at a fixed address (r3 = the address, as left).
inline void TRetF32At(PPUContext& ctx, u32 ea) {
    ctx.gpr[3] = ea;
    ctx.fpr[1] = static_cast<f64>(std::bit_cast<f32>(load<u32>(ea)));
}

// f32 Get(this): this->+12 (a float array) indexed by the current index at
// (*(holder + 16))->+64. r3 / r4 = the array and the byte offset, as left.
inline void TF32ByIndex(PPUContext& ctx, u32 holder) {
    const u32 arr = load<u32>(arg<u32>(ctx, 0) + 12);
    const u32 off = load<u32>(load<u32>(holder + 16) + 64) << 2;
    ctx.gpr[3] = arr;
    ctx.gpr[4] = off;
    ctx.fpr[1] = static_cast<f64>(std::bit_cast<f32>(load<u32>(arr + off)));
}

// A wrapper passing its second argument as the first (r4 untouched).
inline void TThunkArg1(PPUContext& ctx, u32 target) { call(ctx, target, {arg<u32>(ctx, 1)}); }

// The same, null-checked: 0 when the argument is null.
inline void TThunkArg1NZ(PPUContext& ctx, u32 target) {
    const u32 a1 = arg<u32>(ctx, 1);
    ctx.gpr[3] = a1;
    if (a1 != 0) call(ctx, target, {a1});
}

// target(this, (u8)a1): a bool / byte argument narrowed.
inline void TThunkU8(PPUContext& ctx, u32 target) { call(ctx, target, {arg<u32>(ctx, 0), arg<u32>(ctx, 1) & 0xFFu}); }

// target(a1, this + delta): r5 = the 64-bit sum the original leaves.
inline void TSwapAdj(PPUContext& ctx, s32 delta, u32 target) {
    const u64 sum = ctx.gpr[3] + static_cast<u64>(static_cast<s64>(delta));
    call(ctx, target, {arg<u32>(ctx, 1), static_cast<u32>(sum), sum});
}

// target(this, k, a1, a2, 0).
inline void TInsertK(PPUContext& ctx, u32 target, u32 k) {
    call(ctx, target, {arg<u32>(ctx, 0), k, arg<u32>(ctx, 1), arg<u32>(ctx, 2), 0});
}

// ~T() of a class whose base destructor is called for two vtables in turn
// (the class, then its base): r4 = the vtable stored before each call.
inline void TDtor2(PPUContext& ctx, u32 vt1, u32 base1, u32 vt2, u32 base2) {
    const u32 o = arg<u32>(ctx, 0);
    store<u32>(o, vt1);
    call(ctx, base1, {o, vt1});
    store<u32>(o, vt2);
    call(ctx, base2, {o, vt2});
}

// Its deleting version (nothing for a null this).
inline void TDelete2(PPUContext& ctx, u32 vt1, u32 base1, u32 vt2, u32 base2, u32 free_fn, u32 size) {
    const u32 o = arg<u32>(ctx, 0);
    if (o == 0) return;
    TDtor2(ctx, vt1, base1, vt2, base2);
    call(ctx, free_fn, {o, size});
}

// ~T() entered through a secondary base: p = this + delta, the three
// vtables of the full object stored (+0, +off1, +off2), then the base
// destructor (r4-r6 = the third, first and second vtable, as left).
inline void TAdjDtorVt3(PPUContext& ctx, s32 delta, u32 v0, u32 off1, u32 v1, u32 off2, u32 v2, u32 base) {
    const u32 o = static_cast<u32>(arg<u32>(ctx, 0) + static_cast<u32>(delta));
    store<u32>(o, v0);
    store<u32>(o + off1, v1);
    store<u32>(o + off2, v2);
    call(ctx, base, {o, v2, v0, v1});
}

// ---------------------------------------------------------------- batch 3: classes holding PtrVectors

// The inlined ~PtrVector(): Clear(v), then the shared empty function.
inline void VecDtor(PPUContext& ctx, u32 v) {
    call(ctx, o19fn::PtrVector_Clear, {v});
    call(ctx, o19fn::Nop, {v});
}

// ~T() of a class with one PtrVector member at +off: the vtable, the
// vector, then the base destructor (r4 / r5 = this + off and the vtable
// into Clear, as the original leaves them).
inline void TDtorVec(PPUContext& ctx, u32 vt, u32 off, u32 base) {
    const u32 o = arg<u32>(ctx, 0);
    store<u32>(o, vt);
    call(ctx, o19fn::PtrVector_Clear, {o + off, static_cast<u64>(o) + off, vt});
    call(ctx, o19fn::Nop, {o + off});
    call(ctx, base, {o});
}

// Its deleting version.
inline void TDeleteVec(PPUContext& ctx, u32 vt, u32 off, u32 base, u32 free_fn, u32 size) {
    const u32 o = arg<u32>(ctx, 0);
    if (o == 0) return;
    TDtorVec(ctx, vt, off, base);
    call(ctx, free_fn, {o, size});
}

// ~T() of a class with PtrVectors at +off1 / +off2 and a base holding an
// owned 20-byte PtrVector* at +poff: the vtable, both vectors, the base's
// vtable, then the owned vector (freed with SmallFree(p, 20)). Without one
// r3 = the base vtable, as left.
inline void TDtorVec2Ptr(PPUContext& ctx, u32 vt, u32 off1, u32 off2, u32 vtbase, u32 poff) {
    const u32 o = arg<u32>(ctx, 0);
    store<u32>(o, vt);
    call(ctx, o19fn::PtrVector_Clear, {o + off1, static_cast<u64>(o) + off1, vt});
    call(ctx, o19fn::Nop, {o + off1});
    VecDtor(ctx, o + off2);
    const u32 p = load<u32>(o + poff);
    store<u32>(o, vtbase);
    if (p == 0) {
        ctx.gpr[3] = vtbase;
        return;
    }
    VecDtor(ctx, p);
    call(ctx, o19fn::SmallFree, {load<u32>(o + poff), 20});
}

// Its deleting version.
inline void TDeleteVec2Ptr(PPUContext& ctx, u32 vt, u32 off1, u32 off2, u32 vtbase, u32 poff, u32 free_fn, u32 size) {
    const u32 o = arg<u32>(ctx, 0);
    if (o == 0) return;
    TDtorVec2Ptr(ctx, vt, off1, off2, vtbase, poff);
    call(ctx, free_fn, {o, size});
}

// The vtables of a three-base object (+0 / +380 / +384).
struct Vt3 {
    u32 v0, v380, v384;
};

// ~T() through a secondary base (p = this + delta) of the class owning a
// 20-byte PtrVector* at +388: when there is one, the class's vtables, the
// vector and its free; then the base class's vtables and its destructor
// (r4 / r5 = its +380 and +0 vtables, as left).
inline void AdjVecPtrBody(PPUContext& ctx, u32 o, Vt3 a, Vt3 b, u32 base) {
    const u32 q = load<u32>(o + 388);
    if (q != 0) {
        store<u32>(o + 384, a.v384);
        store<u32>(o + 380, a.v380);
        store<u32>(o, a.v0);
        call(ctx, o19fn::PtrVector_Clear, {q, a.v380, a.v0, a.v384});
        call(ctx, o19fn::Nop, {q});
        call(ctx, o19fn::SmallFree, {load<u32>(o + 388), 20});
    }
    store<u32>(o + 384, b.v384);
    store<u32>(o + 380, b.v380);
    store<u32>(o, b.v0);
    call(ctx, base, {o, b.v380, b.v0});
}
inline void TAdjDtorVecPtr(PPUContext& ctx, s32 delta, Vt3 a, Vt3 b, u32 base) {
    AdjVecPtrBody(ctx, static_cast<u32>(arg<u32>(ctx, 0) + static_cast<u32>(delta)), a, b, base);
}
// Its deleting version: nothing for a null p (r3 untouched), the free last.
inline void TAdjDeleteVecPtr(PPUContext& ctx, s32 delta, Vt3 a, Vt3 b, u32 base, u32 free_fn, u32 size) {
    const u32 o = static_cast<u32>(arg<u32>(ctx, 0) + static_cast<u32>(delta));
    if (o == 0) return;
    AdjVecPtrBody(ctx, o, a, b, base);
    call(ctx, free_fn, {o, size});
}

// The deleting version through a secondary base of a three-vtable class:
// nothing for a null p (r3 untouched), else the vtables, the base
// destructor (r4-r6 = the +384, +0 and +380 vtables) and the free.
inline void TAdjDeleteVt3(PPUContext& ctx, s32 delta, u32 v0, u32 off1, u32 v1, u32 off2, u32 v2, u32 base, u32 free_fn, u32 size) {
    const u32 o = static_cast<u32>(arg<u32>(ctx, 0) + static_cast<u32>(delta));
    if (o == 0) return;
    store<u32>(o, v0);
    store<u32>(o + off1, v1);
    store<u32>(o + off2, v2);
    call(ctx, base, {o, v2, v0, v1});
    call(ctx, free_fn, {o, size});
}

// T* New(): Mem_Alloc(size, 0, 16), the base constructor, then the class
// vtable; returns the object (r4 = the vtable) or 0.
inline void TNew0Vt(PPUContext& ctx, u32 size, u32 ctor, u32 vt) {
    const u32 p = static_cast<u32>(call(ctx, o19fn::Mem_Alloc, {size, 0, 16}));
    if (p == 0) {
        ctx.gpr[3] = 0;  // reloaded from the zeroed stack temporary
        return;
    }
    call(ctx, ctor, {p});
    store<u32>(p, vt);
    ctx.gpr[3] = p;
    ctx.gpr[4] = vt;
}

// s16 Push(u16 v, Owner** h): the next entry of the 132-byte entry array
// of (*h - 152) (+144 the u16 count, entries from +152): count + 1, the
// entry's +0 = 7, then (while it reads < 10) its +6 flags 0x8000 / 0x4000
// cleared and +4 = v. Returns the old count (sign-extended, r4 too).
// Quirk kept: the not-taken compare (never false for a 16-bit count) and a
// failed check write the u16 at address 4.
inline void TPushEntry7(PPUContext& ctx) {
    const u32 v = arg<u32>(ctx, 0);
    const u32 b = load<u32>(arg<u32>(ctx, 1)) - 152;
    const u16 c = load<u16>(b + 144);
    const s32 idx = static_cast<s16>(c);
    store<u16>(b + 144, static_cast<u16>(c + 1));
    const u32 entry = b + 152 + static_cast<u32>(idx) * 132;
    store<u32>(entry, 7);
    u32 e = 0;
    u64 r6 = static_cast<u64>(static_cast<s64>(idx)), r8 = 7;
    if (idx < static_cast<s32>((c + 1) & 0xFFFF)) {
        r8 = static_cast<u64>(static_cast<s64>(idx)) << 7;
        r6 = load<u32>(entry);
        if (static_cast<s32>(r6) < 10) e = entry;
    }
    e += 4;
    if (e != 0) {
        r6 = load<u16>(e + 2) & 0x3FFFu;
        store<u16>(e + 2, static_cast<u16>(r6));
    }
    store<u16>(e, static_cast<u16>(v));
    ctx.gpr[3] = ctx.gpr[4] = static_cast<u64>(static_cast<s64>(idx));
    // r5-r9 as the original leaves them
    ctx.gpr[5] = v;
    ctx.gpr[6] = r6;
    ctx.gpr[7] = e;
    ctx.gpr[8] = r8;
    ctx.gpr[9] = entry;
}

// ---------------------------------------------------------------- batch 4

// ~T() of the objects registered in a global table (+2 the u16 count, +8
// the array of 8-byte entries, +4 of each the object): the vtable, the
// entry removed (the later ones moved down, the count re-read after each
// move), then the base destructor. `early`: the count is re-read between
// the two word moves (the deleting version's order).
inline void TUnregister(u32 o, u32 table, bool early) {
    const u32 n = load<u16>(table + 2);
    const u32 arr = load<u32>(table + 8);
    if (n == 0) return;
    u32 i = 0;
    while (load<u32>(arr + 8 * i + 4) != o) {
        if (++i >= n) return;
    }
    u32 cnt = n;
    if (static_cast<s32>(n) >= 2 && static_cast<s32>(i) < static_cast<s32>(n - 1)) {
        do {
            const u32 e = arr + 8 * i;
            store<u32>(e, load<u32>(e + 8));
            if (early) cnt = load<u16>(table + 2);
            store<u32>(e + 4, load<u32>(e + 12));
            if (!early) cnt = load<u16>(table + 2);
            ++i;
        } while (static_cast<s32>(i) < static_cast<s32>(cnt - 1));
    }
    store<u16>(table + 2, static_cast<u16>(cnt - 1));
}
inline void TDtorUnregister(PPUContext& ctx, u32 vt, u32 table, u32 base) {
    const u32 o = arg<u32>(ctx, 0);
    store<u32>(o, vt);
    TUnregister(o, table, false);
    call(ctx, base, {o});
}
inline void TDeleteUnregister(PPUContext& ctx, u32 vt, u32 table, u32 base, u32 free_fn, u32 size) {
    const u32 o = arg<u32>(ctx, 0);
    if (o == 0) return;
    store<u32>(o, vt);
    TUnregister(o, table, true);
    call(ctx, base, {o});
    call(ctx, free_fn, {o, size});
}

// target((s16)a0, a1, &local): a 16-byte local of the wrapper's frame
// (the address the original passes: its r1 + 112).
inline void TCallS16Local(PPUContext& ctx, u32 target) {
    Stack frame(ctx, 16);
    call(ctx, target, {static_cast<u64>(static_cast<s64>(static_cast<s16>(arg<u32>(ctx, 0)))), ctx.gpr[4], frame.addr()});
}

// *(u8*)this = k (r4 = k, as left).
inline void TStoreU8K(PPUContext& ctx, u32 k) {
    store<u8>(arg<u32>(ctx, 0), static_cast<u8>(k));
    ctx.gpr[4] = k;
}

// T* New(a0, a1): SmallAlloc(size), the base constructor (p, a1, a0),
// then the class vtable; returns the object or 0.
inline void TNewSmall2Vt(PPUContext& ctx, u32 size, u32 ctor, u32 vt) {
    const u32 a0 = arg<u32>(ctx, 0), a1 = arg<u32>(ctx, 1);
    const u32 p = static_cast<u32>(call(ctx, o19fn::SmallAlloc, {size}));
    if (p != 0) {
        call(ctx, ctor, {p, a1, a0});
        store<u32>(p, vt);
    }
    ret(ctx, p);
}

// ---------------------------------------------------------------- batch 5

// Virtual call through slot `slot` of o's vtable with the given arguments;
// the OPD and its entry address follow them in the registers, as the
// original call sequence leaves them.
inline u64 VCall(PPUContext& ctx, u32 o, u32 slot, std::initializer_list<u64> args) {
    const u32 opd = load<u32>(load<u32>(o) + slot);
    u64 r[8] = {};
    u32 n = 0;
    for (u64 a : args) r[n++] = a;
    r[n++] = opd;
    r[n++] = load<u32>(opd);
    switch (n) {
    case 2: return call_ptr(ctx, opd, {r[0], r[1]});
    case 3: return call_ptr(ctx, opd, {r[0], r[1], r[2]});
    case 4: return call_ptr(ctx, opd, {r[0], r[1], r[2], r[3]});
    case 5: return call_ptr(ctx, opd, {r[0], r[1], r[2], r[3], r[4]});
    case 6: return call_ptr(ctx, opd, {r[0], r[1], r[2], r[3], r[4], r[5]});
    case 7: return call_ptr(ctx, opd, {r[0], r[1], r[2], r[3], r[4], r[5], r[6]});
    default: return call_ptr(ctx, opd, {r[0], r[1], r[2], r[3], r[4], r[5], r[6], r[7]});
    }
}

// target(this, k1, k2).
inline void TCallK2(PPUContext& ctx, u32 target, u32 k1, u32 k2) { call(ctx, target, {arg<u32>(ctx, 0), k1, k2}); }

// bool: either of two global flags (u8 at +off) set (r4 = the last byte read).
inline void TAnyFlag2(PPUContext& ctx, u32 a, u32 b, u32 off) {
    u32 f = load<u8>(a + off);
    if (f == 0) f = load<u8>(b + off);
    ctx.gpr[4] = f;
    ctx.gpr[3] = f != 0 ? 1 : 0;
}

// bool: this->vfn[slot](this, 0) == k.
inline void TVIsK(PPUContext& ctx, u32 slot, u32 k) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 r = static_cast<u32>(VCall(ctx, o, slot, {o, 0}));
    ctx.gpr[3] = r == k ? 1 : 0;
}

// The 16-byte-element array init (Init(o, u16 count, Allocator* a, ?, x)):
// +0 the count, +2 / +4 0, +6 the element size 16, +8 the data, +12 the
// allocator (the default one, alloc_default(count, 16, 0, 0, x), for a
// null a), +16 0, +20 x; the data a->vfn[0](a, count * size, 0, 16) for a
// non-zero count, then init(o). Returns 0.
inline void THeapArrayInit(PPUContext& ctx, u32 alloc_default, u32 init) {
    const u32 o = arg<u32>(ctx, 0);
    const u64 n64 = ctx.gpr[4];
    u32 a = arg<u32>(ctx, 2);
    const u32 x = arg<u32>(ctx, 4);
    store<u16>(o, static_cast<u16>(n64));
    store<u16>(o + 2, 0);
    store<u16>(o + 4, 0);
    store<u16>(o + 6, 16);
    store<u32>(o + 8, 0);
    u32 n = static_cast<u32>(n64), sz = 16;
    if (a == 0) {
        a = static_cast<u32>(call(ctx, alloc_default, {n64, 16, 0, 0, x}));
        n = load<u16>(o);
        sz = load<u16>(o + 6);
    }
    n &= 0xFFFF;
    store<u32>(o + 12, a);
    if (n != 0) store<u32>(o + 8, static_cast<u32>(VCall(ctx, a, 0, {a, n * ((sz >> 1) & 0x7FFF), 0, 16})));
    call(ctx, init, {o});
    store<u32>(o + 20, x);
    store<u32>(o + 16, 0);
    ctx.gpr[3] = 0;
}

// ---------------------------------------------------------------- batch 6

// The deleting destructor of a pooled class: nothing for a null this, else
// the vtable, then the pool (the r13-relative global's +44) takes the
// object back: pool->vfn[3](pool, this, this->+4 (u16)).
inline void TDeletePooled(PPUContext& ctx, u32 vt, s32 sda_off) {
    const u32 o = arg<u32>(ctx, 0);
    if (o == 0) {
        ctx.gpr[4] = 0;
        return;
    }
    store<u32>(o, vt);
    const u32 pool = load<u32>(load<u32>(static_cast<u32>(ctx.gpr[13]) + static_cast<u32>(sda_off)) + 44);
    VCall(ctx, pool, 12, {pool, o, load<u16>(o + 4)});
}

// u32 Lookup(?, idx): the table at `table` (r4 = idx * 4, as left).
inline void TTableU32(PPUContext& ctx, u32 table) {
    const u32 off = arg<u32>(ctx, 1) << 2;
    ctx.gpr[4] = off;
    ctx.gpr[3] = load<u32>(table + off);
}

// u8 Get(this, s32 i): ((u8*)(this->+off + k))[i] (r4 = i sign-extended).
inline void TByteAt(PPUContext& ctx, u32 off, s32 k) {
    const s32 i = arg<s32>(ctx, 1);
    ctx.gpr[4] = static_cast<u64>(static_cast<s64>(i));
    ctx.gpr[3] = load<u8>(load<u32>(arg<u32>(ctx, 0) + off) + static_cast<u32>(k) + static_cast<u32>(i));
}

// u32* At(this, i): this->+0 + i * 4 (r4 = i * 4, as left).
inline void TIndexAddr4(PPUContext& ctx) {
    const u32 off = arg<u32>(ctx, 1) << 2;
    ctx.gpr[4] = off;
    ctx.gpr[3] = load<u32>(arg<u32>(ctx, 0)) + off;
}

// T* New(): SmallAlloc(size), then T(p, k) when it worked; returns p or 0.
inline void TNewSmallK(PPUContext& ctx, u32 size, u32 ctor, u32 k) {
    const u32 p = static_cast<u32>(call(ctx, o19fn::SmallAlloc, {size}));
    if (p != 0) call(ctx, ctor, {p, k});
    ret(ctx, p);
}

// T* New(a1): Mem_Alloc(size, 0, 16), the base constructor (p, a1), then
// the class vtable; returns p or 0.
inline void TNew1Vt(PPUContext& ctx, u32 size, u32 ctor, u32 vt) {
    const u32 a1 = arg<u32>(ctx, 1);
    const u32 p = static_cast<u32>(call(ctx, o19fn::Mem_Alloc, {size, 0, 16}));
    if (p != 0) {
        call(ctx, ctor, {p, a1});
        store<u32>(p, vt);
    }
    ret(ctx, p);
}

// ~T() of a class owning an object at +40 (destroyed and freed with
// SmallFree(q, qsize), then the field cleared) and a PtrVector at +44: the vtable,
// the owned object, the vector, then the base destructor.
inline void TDtorOwnVec(PPUContext& ctx, u32 vt, u32 qdtor, u32 qsize, u32 base) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 q = load<u32>(o + 40);
    store<u32>(o, vt);
    if (q != 0) {
        call(ctx, qdtor, {q});
        call(ctx, o19fn::SmallFree, {q, qsize});
        store<u32>(o + 40, 0);
    }
    VecDtor(ctx, o + 44);
    call(ctx, base, {o});
}
inline void TDeleteOwnVec(PPUContext& ctx, u32 vt, u32 qdtor, u32 qsize, u32 base, u32 free_fn, u32 size) {
    const u32 o = arg<u32>(ctx, 0);
    if (o == 0) return;
    TDtorOwnVec(ctx, vt, qdtor, qsize, base);
    call(ctx, free_fn, {o, size});
}

// ---------------------------------------------------------------- batch 7

// ~Base() of the classes owning a 20-byte PtrVector* at +poff: the vtable,
// then the vector and its free when there is one (else r4 = the vtable).
inline void TDtorVecPtr(PPUContext& ctx, u32 vt, u32 poff) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 p = load<u32>(o + poff);
    store<u32>(o, vt);
    if (p == 0) {
        ctx.gpr[4] = vt;
        return;
    }
    VecDtor(ctx, p);
    call(ctx, o19fn::SmallFree, {load<u32>(o + poff), 20});
}
inline void TDeleteVecPtr(PPUContext& ctx, u32 vt, u32 poff, u32 free_fn, u32 size) {
    const u32 o = arg<u32>(ctx, 0);
    if (o == 0) return;
    TDtorVecPtr(ctx, vt, poff);
    call(ctx, free_fn, {o, size});
}

// The event-record init (o, src, a, b, c): +20 src, +0 kind, +4 the
// current time (*(*(*clock) + 12)), +8 a or src->+216->+52, +12 b or
// src->+168, +16 c or src->+336->+168 (0 for a null src / link).
// r4 / r5 / r8 / r9 / f1 as the original leaves them.
inline void TEventInit(PPUContext& ctx, u32 kind, u32 clock) {
    const u32 o = arg<u32>(ctx, 0), src = arg<u32>(ctx, 1);
    const u32 a = arg<u32>(ctx, 2), b = arg<u32>(ctx, 3), c = arg<u32>(ctx, 4);
    store<u32>(o + 20, src);
    const u32 g = load<u32>(clock);
    store<u32>(o, kind);
    const u32 tp = load<u32>(g + 12);
    const u32 tw = load<u32>(tp);
    store<u32>(o + 4, tw);
    u64 r8 = tp;
    u32 v = a;
    if (v == 0 && src != 0) {
        const u32 q = load<u32>(src + 216);
        v = q != 0 ? load<u32>(q + 52) : 0;
        r8 = v;
    }
    store<u32>(o + 8, v);
    v = b;
    if (v == 0 && src != 0) v = load<u32>(src + 168);
    store<u32>(o + 12, v);
    u64 r4 = ctx.gpr[4];
    v = c;
    if (v == 0 && src != 0) {
        const u32 l = load<u32>(src + 336);
        r4 = l;
        v = l != 0 ? load<u32>(l + 168) : 0;
    }
    store<u32>(o + 16, v);
    ctx.gpr[4] = r4;
    ctx.gpr[5] = v;
    ctx.gpr[8] = r8;
    ctx.gpr[9] = kind;
    ctx.fpr[1] = static_cast<f64>(std::bit_cast<f32>(tw));
}

// ~T() whose only work is a member's destructor: the vtable, then
// fn(this + off) (r4 = the vtable, r5 = this + off, as left).
inline void TDtorMember(PPUContext& ctx, u32 vt, u32 off, u32 fn) {
    const u32 o = arg<u32>(ctx, 0);
    store<u32>(o, vt);
    call(ctx, fn, {o + off, vt, static_cast<u64>(o) + off});
}

// Vec3 copy: dst[0..2] = src[0..2] (the words as they are; f1 = the last,
// r3-r7 the addresses the original leaves).
inline void TCopyVec3(PPUContext& ctx) {
    const u32 d = arg<u32>(ctx, 0), s = arg<u32>(ctx, 1);
    u32 w = 0;
    for (u32 k = 0; k < 12; k += 4) {
        w = load<u32>(s + k);
        store<u32>(d + k, w);
    }
    ctx.fpr[1] = static_cast<f64>(std::bit_cast<f32>(w));
    ctx.gpr[3] = s + 8;
    ctx.gpr[4] = static_cast<u64>(s) + 8;
    ctx.gpr[5] = s + 4;
    ctx.gpr[6] = d + 4;
    ctx.gpr[7] = d + 8;
}

// ---------------------------------------------------------------- batch 8

// T(this, a1): the base constructor fn(this, k), the u8 flag at +9 = a1,
// then the class vtable (r3 = the vtable, as left).
inline void TCtorFlagVt(PPUContext& ctx, u32 fn, u32 k, u32 vt) {
    const u32 o = arg<u32>(ctx, 0), a1 = arg<u32>(ctx, 1);
    call(ctx, fn, {o, k});
    store<u8>(o + 9, static_cast<u8>(a1));
    store<u32>(o, vt);
    ctx.gpr[3] = vt;
}

// The deleting destructor whose only work is a member's destructor:
// nothing for a null this, else the vtable, fn(this + off) (r4 / r5 =
// this + off and the vtable, as left), then the free.
inline void TDeleteMember(PPUContext& ctx, u32 vt, u32 off, u32 fn, u32 free_fn, u32 size) {
    const u32 o = arg<u32>(ctx, 0);
    if (o == 0) return;
    store<u32>(o, vt);
    call(ctx, fn, {o + off, static_cast<u64>(o) + off, vt});
    call(ctx, free_fn, {o, size});
}

// ---------------------------------------------------------------- batch 9

// p = SmallAlloc(size); T(p, this->+boff (u8)) when it worked; then
// reg(p or 0), whose result is returned.
inline void TNewByteReg(PPUContext& ctx, u32 size, u32 ctor, u32 boff, u32 reg) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 p = static_cast<u32>(call(ctx, o19fn::SmallAlloc, {size}));
    if (p != 0) call(ctx, ctor, {p, load<u8>(o + boff)});
    call(ctx, reg, {p});
}

// bool Claim(this, Slot* s): 0 (and s->+4 = this) when this->+204 is the
// slot's key, or there is none and Key(this->vfn[3]()) is; else 1.
inline void TClaimKey(PPUContext& ctx, u32 keyfn) {
    const u32 o = arg<u32>(ctx, 0), sl = arg<u32>(ctx, 1);
    const u32 cur = load<u32>(o + 204);
    u32 key = load<u32>(sl);
    bool match = cur == key;
    if (!match && cur == 0) {
        const u32 r = static_cast<u32>(VCall(ctx, o, 12, {o}));
        const u32 k = static_cast<u32>(call(ctx, keyfn, {r}));
        key = load<u32>(sl);
        match = k == key;
    }
    ctx.gpr[4] = key;  // the key the original compared last
    if (!match) {
        ctx.gpr[3] = 1;
        return;
    }
    store<u32>(sl + 4, o);
    ctx.gpr[3] = 0;
}

// The per-index table getters: this->+12[(*(holder + 16))->+64] as u32 /
// u8 (r4 = the byte offset, as left).
inline u32 IndexOff(u32 holder) { return load<u32>(load<u32>(holder + 16) + 64) << 2; }
inline void TU32ByIndex(PPUContext& ctx, u32 holder) {
    const u32 arr = load<u32>(arg<u32>(ctx, 0) + 12);
    const u32 off = IndexOff(holder);
    ctx.gpr[4] = off;
    ctx.gpr[3] = load<u32>(arr + off);
}
inline void TU8ByIndex(PPUContext& ctx, u32 holder) {
    const u32 arr = load<u32>(arg<u32>(ctx, 0) + 12);
    const u32 off = IndexOff(holder);
    ctx.gpr[4] = off;
    ctx.gpr[3] = load<u8>(arr + off);
}

// ---------------------------------------------------------------- batch 10

// p = SmallAlloc(size); T(p, this->+8, this + off, hash, this) when it
// worked; returns attach(this->+8 as read first, p or 0).
inline void TNewAttach(PPUContext& ctx, u32 size, u32 ctor, u32 off, u32 hash, u32 attach) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 owner = load<u32>(o + 8);
    const u32 p = static_cast<u32>(call(ctx, o19fn::SmallAlloc, {size}));
    if (p != 0) call(ctx, ctor, {p, load<u32>(o + 8), o + off, hash, o});
    call(ctx, attach, {owner, p});
}

// A 16-byte property object (the base T(p, a1), then the class vtable)
// bound to a1's field `hash`: bind(a1, hash, &p->+12, 0) with the u32 at
// +12 = 0 (TNewPropU32), or bind(a1, hash, &p->+12, 1) with the u8 at +12
// = 1 (TNewPropU8). Returns p or 0.
inline void TNewPropU32(PPUContext& ctx, u32 base, u32 vt, u32 hash, u32 bind) {
    const u32 a1 = arg<u32>(ctx, 1);
    const u32 p = static_cast<u32>(call(ctx, o19fn::Mem_Alloc, {16, 0, 16}));
    if (p != 0) {
        call(ctx, base, {p, a1});
        store<u32>(p + 12, 0);
        store<u32>(p, vt);
        call(ctx, bind, {a1, hash, p + 12, 0});
    }
    ret(ctx, p);
}
inline void TNewPropU8(PPUContext& ctx, u32 base, u32 vt, u32 hash, u32 bind) {
    const u32 a1 = arg<u32>(ctx, 1);
    const u32 p = static_cast<u32>(call(ctx, o19fn::Mem_Alloc, {16, 0, 16}));
    if (p != 0) {
        call(ctx, base, {p, a1});
        store<u8>(p + 12, 1);
        store<u32>(p, vt);
        call(ctx, bind, {a1, hash, p + 12, 1});
    }
    ret(ctx, p);
}

// bool Find(?, u32 key, u32* out, Node* n): the binary tree walk (+4 the
// key, +8 the value, +12 / +16 the lower / higher child); *out = the
// value when found. r4-r7 as the original leaves them.
inline void TTreeFind(PPUContext& ctx) {
    const u32 key = arg<u32>(ctx, 1), out = arg<u32>(ctx, 2);
    u32 n = arg<u32>(ctx, 3);
    ctx.gpr[7] = ctx.gpr[4];
    ctx.gpr[4] = out;
    ctx.gpr[5] = key;
    while (n != 0) {
        const u32 k = load<u32>(n + 4);
        ctx.gpr[6] = k;
        if (k == key) {
            const u32 v = load<u32>(n + 8);
            store<u32>(out, v);
            ctx.gpr[5] = v;
            ctx.gpr[3] = 1;
            return;
        }
        n = load<u32>(n + (key < k ? 12 : 16));
    }
    ctx.gpr[3] = 0;
}

// u32 Get(this): this->+off.
inline void TLoadU32(PPUContext& ctx, u32 off) { ctx.gpr[3] = load<u32>(arg<u32>(ctx, 0) + off); }

// PushBack(Array* a, const u32* v) of the u32 arrays (+0 the data, +4 the
// u16 capacity, +8 the u16 count, +10 the u16 high-water mark): grow(a,
// capacity * 2) when full, the element stored, the count and the mark
// updated. Returns the old mark (r3), r4 / r5 = the new count and the
// value, as left.
inline void TPushU32(PPUContext& ctx, u32 grow) {
    const u32 a = arg<u32>(ctx, 0), pv = arg<u32>(ctx, 1);
    u32 c = load<u16>(a + 8);
    const u32 cap = load<u16>(a + 4);
    if (c == cap) {
        call(ctx, grow, {a, (cap << 1) & 0x1FFFE});
        c = load<u16>(a + 8);
    }
    const u32 data = load<u32>(a);
    const u32 v = load<u32>(pv);
    store<u32>(data + ((c << 2) & 0x3FFFC), v);
    const u32 n = load<u16>(a + 8) + 1;
    const u32 m = load<u16>(a + 10);
    store<u16>(a + 8, static_cast<u16>(n));
    store<u16>(a + 10, static_cast<u16>(static_cast<s32>(m) > static_cast<s32>(n & 0xFFFF) ? m : n));
    ctx.gpr[3] = m;
    ctx.gpr[4] = n & 0xFFFF;
    ctx.gpr[5] = v;
}

// ---------------------------------------------------------------- batch 11: small accessors

// dst[0..n) = src->+soff[0..n) (the words as they are; f1 = the last one).
inline void TCopyFields(PPUContext& ctx, u32 soff, u32 n) {
    const u32 d = arg<u32>(ctx, 0), sp = arg<u32>(ctx, 1) + soff;
    u32 w = 0;
    for (u32 k = 0; k < n; ++k) {
        w = load<u32>(sp + 4 * k);
        store<u32>(d + 4 * k, w);
    }
    ctx.fpr[1] = static_cast<f64>(std::bit_cast<f32>(w));
}

// dst[0..2] = the global Vec3 at src (r4 = its high half, r5 = src).
inline void TCopyGlobalVec3(PPUContext& ctx, u32 src) {
    const u32 d = arg<u32>(ctx, 0);
    u32 w = 0;
    for (u32 k = 0; k < 12; k += 4) {
        w = load<u32>(src + k);
        store<u32>(d + k, w);
    }
    ctx.gpr[4] = src & 0xFFFF0000u;
    ctx.gpr[5] = src;
    ctx.fpr[1] = static_cast<f64>(std::bit_cast<f32>(w));
}

// f32 Get(this): (this->+off1)->+off2 (r3 = the intermediate, as left).
inline void TF32Chain(PPUContext& ctx, u32 off1, u32 off2) {
    const u32 p = load<u32>(arg<u32>(ctx, 0) + off1);
    ctx.gpr[3] = p;
    ctx.fpr[1] = static_cast<f64>(std::bit_cast<f32>(load<u32>(p + off2)));
}

// f32 Get(this, i): ((f32*)(this + k))[i] (r3 / r4 = the base and the
// sign-extended byte offset, as left).
inline void TF32At(PPUContext& ctx, s32 k) {
    const u32 b = arg<u32>(ctx, 0) + static_cast<u32>(k);
    const s32 off = static_cast<s32>(arg<u32>(ctx, 1) << 2);
    ctx.gpr[3] = b;
    ctx.gpr[4] = static_cast<u64>(static_cast<s64>(off));
    ctx.fpr[1] = static_cast<f64>(std::bit_cast<f32>(load<u32>(b + static_cast<u32>(off))));
}

// p = this + delta: the five vtables of a multiply-inherited object
// (stored at +12, +20, +8, +16, +0 in that order). r4-r8 as left.
inline void TAdjSetVt5(PPUContext& ctx, s32 delta, u32 v12, u32 v20, u32 v8, u32 v16, u32 v0) {
    const u32 o = static_cast<u32>(arg<u32>(ctx, 0) + static_cast<u32>(delta));
    store<u32>(o + 12, v12);
    store<u32>(o + 20, v20);
    store<u32>(o + 8, v8);
    store<u32>(o + 16, v16);
    store<u32>(o, v0);
    ctx.gpr[3] = o;
    ctx.gpr[4] = v0;
    ctx.gpr[5] = v20;
    ctx.gpr[6] = v8;
    ctx.gpr[7] = v16;
    ctx.gpr[8] = v0 & 0xFFFF0000u;
}

// *(u8*)(this + off) = k (r4 = k).
inline void TStoreU8At(PPUContext& ctx, u32 off, u32 k) {
    store<u8>(arg<u32>(ctx, 0) + off, static_cast<u8>(k));
    ctx.gpr[4] = k;
}

// this->+off |= k (r4 = the new value).
inline void TOrU32(PPUContext& ctx, u32 off, u32 k) {
    const u32 v = load<u32>(arg<u32>(ctx, 0) + off) | k;
    store<u32>(arg<u32>(ctx, 0) + off, v);
    ctx.gpr[4] = v;
}

// *(u8*)a1 = 0; returns a1 (r5 = 0).
inline void TClearByteRet(PPUContext& ctx) {
    const u32 p = arg<u32>(ctx, 1);
    store<u8>(p, 0);
    ctx.gpr[3] = p;
    ctx.gpr[5] = 0;
}

// &this->+0[i] for elem-size 1 << sh (r4 = the byte offset, as left).
inline void TIndexAddr(PPUContext& ctx, u32 sh) {
    const u32 off = arg<u32>(ctx, 1) << sh;
    ctx.gpr[4] = off;
    ctx.gpr[3] = load<u32>(arg<u32>(ctx, 0)) + off;
}

// if (this->+12 == a1) this->+4 = (f32)f1 (r5 = the field, as left).
inline void TSetF32IfKey(PPUContext& ctx) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 k = load<u32>(o + 12);
    ctx.gpr[5] = k;
    if (k == arg<u32>(ctx, 1)) store<u32>(o + 4, std::bit_cast<u32>(static_cast<f32>(ctx.fpr[1])));
}

// if ((u8)a1) this->+off (u8) = 0 (r4 = (u8)a1 or 0, as left).
inline void TClearByteIf(PPUContext& ctx, u32 off) {
    const u32 f = arg<u32>(ctx, 1) & 0xFF;
    ctx.gpr[4] = f;
    if (f != 0) {
        store<u8>(arg<u32>(ctx, 0) + off, 0);
        ctx.gpr[4] = 0;
    }
}

// ---------------------------------------------------------------- batch 12: named nodes

namespace o19fn {
constexpr u32 Node_Init = 0x00391694;     // (p, s16 id, const char* name)
constexpr u32 Node_Bind = 0x004D3088;     // (owner, const char* name, k5, k6, -1, 0) -> binding
constexpr u32 kNodeTable = 0x00F65038;    // (+24 the 128-byte records)
constexpr u32 kFlagOffset = 0x0106CF82;   // u16 added to the node's +20
}  // namespace o19fn

struct NBind {
    u32 name, k5, k6;
};

// Node* New(s16 id): Mem_Alloc(24, 0, 16), Node_Init(p, id, name); the
// node's record (+16 the s16 index into kNodeTable->+24; 16 stored at its
// +0 -- quirk kept: at address 0 for index -1); then each binding
// Node_Bind(p->+12, name, k5, k6, -1, 0): its +76 cleared and its +64
// copied as a byte to (p->+20 + *kFlagOffset)[i]. Returns p or 0.
inline void TNewNamed(PPUContext& ctx, u32 name1, std::initializer_list<NBind> binds) {
    const u64 a0 = ctx.gpr[3];
    const u32 p = static_cast<u32>(call(ctx, o19fn::Mem_Alloc, {24, 0, 16}));
    if (p == 0) {
        ctx.gpr[3] = 0;
        return;
    }
    call(ctx, o19fn::Node_Init, {p, static_cast<u64>(static_cast<s64>(static_cast<s16>(a0))), name1});
    const s32 idx = static_cast<s16>(load<u16>(p + 16));
    const u32 tbl = load<u32>(o19fn::kNodeTable);
    const u32 rec = idx == -1 ? 0 : load<u32>(tbl + 24) + (static_cast<u32>(idx) << 7);
    store<u16>(rec, 16);
    const u32 flags = load<u32>(p + 20) + load<u16>(o19fn::kFlagOffset);
    u32 i = 0;
    for (const NBind& b : binds) {
        const u32 r = static_cast<u32>(call(ctx, o19fn::Node_Bind, {load<u32>(p + 12), b.name, b.k5, b.k6, ~0ull, 0}));
        const u32 v = load<u32>(r + 64);
        store<u32>(r + 76, 0);
        store<u8>(flags + i, static_cast<u8>(v));
        ctx.gpr[4] = v;  // the last binding's +64, as left
        ++i;
    }
    ret(ctx, p);
}

// ---------------------------------------------------------------- batch 13

// this->vfn[slot](this); returns 1.
inline void TVCallRet1(PPUContext& ctx, u32 slot) {
    const u32 o = arg<u32>(ctx, 0);
    VCall(ctx, o, slot, {o});
    ctx.gpr[3] = 1;
}

// dst[0..2] = (a1->+388 ? a1->+388 + 432 : the global default Vec3 dflt).
// r3-r7 as the original leaves them.
inline void TCopyPosOrDefault(PPUContext& ctx, u32 dflt) {
    const u32 d = arg<u32>(ctx, 0);
    const u32 q = load<u32>(arg<u32>(ctx, 1) + 388);
    if (q == 0) {
        TCopyGlobalVec3(ctx, dflt);
        return;
    }
    const u32 sp = q + 432;
    u32 w = 0;
    for (u32 k = 0; k < 12; k += 4) {
        w = load<u32>(sp + k);
        store<u32>(d + k, w);
    }
    ctx.fpr[1] = static_cast<f64>(std::bit_cast<f32>(w));
    ctx.gpr[3] = d + 8;
    ctx.gpr[4] = sp + 8;
    ctx.gpr[5] = d + 4;
    ctx.gpr[6] = sp + 4;
    ctx.gpr[7] = static_cast<u64>(d) + 8;
}

// T* Clone(this): p = SmallAlloc(24) (+4 = 1, +8 = 0, the vtable, +20 =
// 0), then +20 / +8 copied from this. Quirk kept: a failed allocation
// still writes the copies at addresses 20 / 8. r4 / r5 = the copied words.
inline void TClone24(PPUContext& ctx, u32 vt) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 p = static_cast<u32>(call(ctx, o19fn::SmallAlloc, {24}));
    if (p != 0) {
        store<u16>(p + 4, 1);
        store<u32>(p + 8, 0);
        store<u32>(p, vt);
        store<u32>(p + 20, 0);
    }
    const u32 w20 = load<u32>(o + 20), w8 = load<u32>(o + 8);
    store<u32>(p + 20, w20);
    store<u32>(p + 8, w8);
    ctx.gpr[3] = p;
    ctx.gpr[4] = w20;
    ctx.gpr[5] = w8;
}

// p = SmallAlloc(size); T(p, this->+8, this + off, hash, this) when it
// worked; returns p or 0.
inline void TNewOwned(PPUContext& ctx, u32 size, u32 ctor, u32 off, u32 hash) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 p = static_cast<u32>(call(ctx, o19fn::SmallAlloc, {size}));
    if (p != 0) call(ctx, ctor, {p, load<u32>(o + 8), o + off, hash, o});
    ret(ctx, p);
}

// ---------------------------------------------------------------- batch 14

// bool Gate(this, f1, f2): 1 when the flag 0x20000000 of +552 is set, or
// not (f2 < *kmin) and f1 < f2 (unordered included); else
// this->vfn[105](this, k4, k5) and 0.
inline void TFloatGate(PPUContext& ctx, u32 kmin, u32 k4, u32 k5) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 fl = load<u32>(o + 552);
    ctx.gpr[4] = load<u32>(o);  // the vtable and the flag bit / kmin, as left
    ctx.gpr[5] = (fl >> 29) & 1;
    if ((fl >> 29) & 1) {
        ctx.gpr[3] = 1;
        return;
    }
    ctx.gpr[5] = kmin;
    const f64 f1 = ctx.fpr[1], f2 = ctx.fpr[2];
    const f64 f3 = static_cast<f64>(std::bit_cast<f32>(load<u32>(kmin)));
    ctx.fpr[3] = f3;
    if (!(f2 < f3) && !(f1 > f2 || f1 == f2)) {
        ctx.gpr[3] = 1;
        return;
    }
    VCall(ctx, o, 420, {o, k4, k5});
    ctx.gpr[3] = 0;
}

// ~T() through a secondary base (p = this + delta): the class's three
// vtables, the member's destructor mfn(p + moff) (r4-r7 = p + moff and the
// three vtables), the base class's vtables, then its destructor (r4 / r5 =
// its +380 and +0 vtables).
inline void AdjDtorMemberBody(PPUContext& ctx, u32 o, Vt3 a, u32 mfn, u32 moff, Vt3 b, u32 base) {
    store<u32>(o, a.v0);
    store<u32>(o + 380, a.v380);
    store<u32>(o + 384, a.v384);
    call(ctx, mfn, {o + moff, static_cast<u64>(o) + moff, a.v0, a.v380, a.v384});
    store<u32>(o + 384, b.v384);
    store<u32>(o + 380, b.v380);
    store<u32>(o, b.v0);
    call(ctx, base, {o, b.v380, b.v0});
}
inline void TAdjDtorMember(PPUContext& ctx, s32 delta, Vt3 a, u32 mfn, u32 moff, Vt3 b, u32 base) {
    AdjDtorMemberBody(ctx, static_cast<u32>(arg<u32>(ctx, 0) + static_cast<u32>(delta)), a, mfn, moff, b, base);
}
inline void TAdjDeleteMember(PPUContext& ctx, s32 delta, Vt3 a, u32 mfn, u32 moff, Vt3 b, u32 base, u32 free_fn, u32 size) {
    const u32 o = static_cast<u32>(arg<u32>(ctx, 0) + static_cast<u32>(delta));
    if (o == 0) return;
    AdjDtorMemberBody(ctx, o, a, mfn, moff, b, base);
    call(ctx, free_fn, {o, size});
}

// T* New(a0, a1): p = SmallAlloc(24) (+4 = 1, the vtable, +8 = 0, +20 =
// 0); r = find(a1, key); if r: p->+8 = conv(r). Returns p or 0.
inline void TNewLookup24(PPUContext& ctx, u32 vt, u32 find, u32 key, u32 conv) {
    const u32 a1 = arg<u32>(ctx, 1);
    const u32 p = static_cast<u32>(call(ctx, o19fn::SmallAlloc, {24}));
    if (p != 0) {
        store<u16>(p + 4, 1);
        store<u32>(p, vt);
        store<u32>(p + 8, 0);
        store<u32>(p + 20, 0);
        const u32 r = static_cast<u32>(call(ctx, find, {a1, key}));
        if (r != 0) store<u32>(p + 8, static_cast<u32>(call(ctx, conv, {r})));
    }
    ret(ctx, p);
}

// ---------------------------------------------------------------- batch 15

// V Get(K key): *find(map, &key) -- the key spilled to a 16-byte local of
// the wrapper's frame (its r1 + 112, as the original passes it).
inline void TMapGetByValue(PPUContext& ctx, u32 find, u32 map) {
    Stack frame(ctx, 16);
    store<u32>(frame.addr(), arg<u32>(ctx, 0));
    const u32 r = static_cast<u32>(call(ctx, find, {map, frame.addr()}));
    ctx.gpr[3] = load<u32>(r);
}

// ~T(): the vtable; the link at +4 (when its +172 has 0x40000, its u16
// +176 gets 0x1000) cleared. r4 / r5 as the original leaves them.
inline void TDtorUnlink(PPUContext& ctx, u32 vt) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 q = load<u32>(o + 4);
    store<u32>(o, vt);
    u64 r5 = vt;
    if (q != 0) {
        r5 = (load<u32>(q + 172) >> 18) & 1;
        if (r5 != 0) {
            r5 = load<u16>(q + 176) | 0x1000u;
            store<u16>(q + 176, static_cast<u16>(r5));
        }
    }
    store<u32>(o + 4, 0);
    ctx.gpr[4] = 0;
    ctx.gpr[5] = r5;
}

// a1 ? set(this, conv(a1), k) : 0.
inline void TSetConverted(PPUContext& ctx, u32 conv, u32 set, u32 k) {
    const u32 o = arg<u32>(ctx, 0), a1 = arg<u32>(ctx, 1);
    ctx.gpr[3] = a1;
    if (a1 == 0) return;
    const u32 r = static_cast<u32>(call(ctx, conv, {a1}));
    call(ctx, set, {o, r, k});
}

// ~T(): the buffer at +off freed (Mem_FreeFrom(p, 0)) and cleared, with
// the class vtable stored first; then the base vtable (r3 = it, as left).
inline void TDtorFreeBuf(PPUContext& ctx, u32 vt, u32 off, u32 vtbase) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 q = load<u32>(o + off);
    if (q != 0) {
        store<u32>(o, vt);
        call(ctx, o19fn::Mem_FreeFrom, {q, 0, vt});
        store<u32>(o + off, 0);
    } else {
        ctx.gpr[4] = vt;
    }
    store<u32>(o, vtbase);
    ctx.gpr[3] = vtbase;
}

// ---------------------------------------------------------------- batch 16

// The pooled deleting destructor with a fixed size: nothing for a null
// this (r4 = 0), else the vtable and pool->vfn[3](pool, this, size).
inline void TDeletePooledK(PPUContext& ctx, u32 vt, s32 sda_off, u32 size) {
    const u32 o = arg<u32>(ctx, 0);
    ctx.gpr[4] = o;
    if (o == 0) return;
    store<u32>(o, vt);
    const u32 pool = load<u32>(load<u32>(static_cast<u32>(ctx.gpr[13]) + static_cast<u32>(sda_off)) + 44);
    VCall(ctx, pool, 12, {pool, o, size});
}

// The deleting version of TDtorUnlink: SmallFree(this, size) last (r5 /
// r6 = this and 0, as the original leaves them).
inline void TDeleteUnlink(PPUContext& ctx, u32 vt, u32 size) {
    const u32 o = arg<u32>(ctx, 0);
    ctx.gpr[4] = o;
    if (o == 0) return;
    const u32 q = load<u32>(o + 4);
    store<u32>(o, vt);
    if (q != 0 && ((load<u32>(q + 172) >> 18) & 1) != 0)
        store<u16>(q + 176, static_cast<u16>(load<u16>(q + 176) | 0x1000u));
    store<u32>(o + 4, 0);
    call(ctx, o19fn::SmallFree, {o, size, o, 0});
}

// The short event-record init (o, src, b, c): +20 src, +0 kind, +8 0, +4
// the current time, +12 b or src->+168, +16 c or src->+336->+168.
inline void TEventInitShort(PPUContext& ctx, u32 kind, u32 clock) {
    const u32 o = arg<u32>(ctx, 0), src = arg<u32>(ctx, 1);
    const u32 b = arg<u32>(ctx, 2), c = arg<u32>(ctx, 3);
    store<u32>(o + 20, src);
    const u32 g = load<u32>(clock);
    store<u32>(o, kind);
    const u32 tp = load<u32>(g + 12);
    const u32 tw = load<u32>(tp);
    store<u32>(o + 8, 0);
    store<u32>(o + 4, tw);
    u32 v = b;
    if (v == 0 && src != 0) v = load<u32>(src + 168);
    store<u32>(o + 12, v);
    u64 r4 = ctx.gpr[4];
    v = c;
    if (v == 0 && src != 0) {
        const u32 l = load<u32>(src + 336);
        r4 = l;
        v = l != 0 ? load<u32>(l + 168) : 0;
    }
    store<u32>(o + 16, v);
    ctx.gpr[4] = r4;
    ctx.gpr[5] = 0;
    ctx.gpr[7] = v;
    ctx.gpr[8] = tp;
    ctx.gpr[9] = kind;
    ctx.fpr[1] = static_cast<f64>(std::bit_cast<f32>(tw));
}

// The deleting version of TDtorFreeBuf: Mem_FreeFrom(this, 0) last.
inline void TDeleteFreeBuf(PPUContext& ctx, u32 vt, u32 off, u32 vtbase) {
    const u32 o = arg<u32>(ctx, 0);
    if (o == 0) return;
    const u32 q = load<u32>(o + off);
    if (q != 0) {
        store<u32>(o, vt);
        call(ctx, o19fn::Mem_FreeFrom, {q, 0, vt});
        store<u32>(o + off, 0);
    }
    store<u32>(o, vtbase);
    call(ctx, o19fn::Mem_FreeFrom, {o, 0, vtbase});
}

// ~T() of an owner of objects in a PtrVector at +8 (+16 the u16 count):
// the vtable; while there are any, the last one's vfn[2](obj) and the
// count decremented (the original's move of the element at the new count
// into the slot before it runs only when that count read back as 0 --
// quirk kept); then ~PtrVector(+8) and the base vtable (r3 = it).
inline void TDtorOwnerVec(PPUContext& ctx, u32 vt, u32 vtbase) {
    const u32 o = arg<u32>(ctx, 0);
    store<u32>(o, vt);
    for (u32 n; (n = load<u16>(o + 16)) != 0;) {
        const u32 obj = load<u32>(load<u32>(o + 8) + ((n - 1) << 2));
        VCall(ctx, obj, 8, {obj});
        const u32 m = static_cast<u32>(load<u16>(o + 16)) - 1;
        store<u16>(o + 16, static_cast<u16>(m));
        if (static_cast<s32>(m) != static_cast<s32>(m & 0xFFFF)) {
            const u32 d = load<u32>(o + 8);
            store<u32>(d + (m << 2), load<u32>(d + ((m << 2) & 0x3FFFC)));
        }
    }
    VecDtor(ctx, o + 8);
    store<u32>(o, vtbase);
    ctx.gpr[3] = vtbase;
}

// ---------------------------------------------------------------- batch 17: field accessors (any offset)

inline void TLoadU8(PPUContext& ctx, u32 off) { ctx.gpr[3] = load<u8>(arg<u32>(ctx, 0) + off); }
inline void TLoadF32(PPUContext& ctx, u32 off) {
    ctx.fpr[1] = static_cast<f64>(std::bit_cast<f32>(load<u32>(arg<u32>(ctx, 0) + off)));
}
inline void TLoadGlobalU32(PPUContext& ctx, u32 ea) { ctx.gpr[3] = load<u32>(ea); }
inline void TStoreU32Arg(PPUContext& ctx, u32 off) { store<u32>(arg<u32>(ctx, 0) + off, arg<u32>(ctx, 1)); }
inline void TStoreU8Arg(PPUContext& ctx, u32 off) { store<u8>(arg<u32>(ctx, 0) + off, static_cast<u8>(arg<u32>(ctx, 1))); }

// bool: bit (32 - sh) of the word at this + off (rlwinm r, r, sh, 31, 31).
inline void TFlagBit(PPUContext& ctx, u32 off, u32 sh) {
    const u32 w = load<u32>(arg<u32>(ctx, 0) + off);
    ctx.gpr[3] = ((w << sh) | (w >> ((32 - sh) & 31))) & 1;
}

// (this + doff)[0..n) = (a1 + soff)[0..n) (words as they are; f1 = the last).
inline void TCopyFields2(PPUContext& ctx, u32 doff, u32 soff, u32 n) {
    const u32 d = arg<u32>(ctx, 0) + doff, sp = arg<u32>(ctx, 1) + soff;
    u32 w = 0;
    for (u32 k = 0; k < n; ++k) {
        w = load<u32>(sp + 4 * k);
        store<u32>(d + 4 * k, w);
    }
    ctx.fpr[1] = static_cast<f64>(std::bit_cast<f32>(w));
}

// ---------------------------------------------------------------- batch 18: AVL rotations

// The node height fix-up: +2 = 1 + max(height(+12), height(+16)) and +0 =
// height(+16) - height(+12) (s16 heights at the children's +2, 0 for a
// null child). Returns the registers the original leaves: the balance
// (sign-extended), the new height and the right height.
struct AvlRegs {
    u64 diff, h, hr;
};
inline AvlRegs AvlFix(u32 x) {
    s64 hl = 0, hr = 0;
    const u32 l = load<u32>(x + 12);
    if (l != 0) hl = static_cast<s16>(load<u16>(l + 2));
    const u32 r = load<u32>(x + 16);
    if (r != 0) hr = static_cast<s16>(load<u16>(r + 2));
    const s64 m = static_cast<s32>(hl) > static_cast<s32>(hr) ? hl : hr;
    store<u16>(x + 2, static_cast<u16>(m + 1));
    store<u16>(x, static_cast<u16>(hr - hl));
    return {static_cast<u64>(hr - hl), static_cast<u64>(m + 1), static_cast<u64>(hr)};
}

// Rotate(?, Node** pp): the child at +a takes the node's place (its +b
// child becomes the node's +a child, the node its +b child), then the
// heights of the new root's children (+12, +16) and of the new root are
// fixed. a / b = 12 / 16 or 16 / 12 for the two directions.
inline void TAvlRotate(PPUContext& ctx, u32 a, u32 b) {
    const u32 pp = arg<u32>(ctx, 1);
    const u32 n = load<u32>(pp);
    const u32 c = load<u32>(n + a);
    store<u32>(pp, c);
    if (c == 0) {
        ctx.gpr[3] = n;
        ctx.gpr[5] = 0;
        return;
    }
    store<u32>(n + a, load<u32>(c + b));
    store<u32>(load<u32>(pp) + b, n);
    for (u32 off : {12u, 16u}) {
        const u32 r3 = load<u32>(pp);
        const u32 x = load<u32>(r3 + off);
        ctx.gpr[3] = r3;
        ctx.gpr[5] = x;
        if (x == 0) continue;
        const AvlRegs q = AvlFix(x);
        ctx.gpr[3] = q.diff;
        ctx.gpr[6] = q.h;
        ctx.gpr[7] = q.hr;
    }
    const u32 root = load<u32>(pp);
    ctx.gpr[3] = root;
    if (root == 0) return;
    const AvlRegs q = AvlFix(root);
    ctx.gpr[4] = q.h;
    ctx.gpr[5] = q.diff;
    ctx.gpr[6] = q.hr;
}

// ---------------------------------------------------------------- batch 19

// Find(): the object registered under the global key word at gkey (0 when
// it is 0): *find(map, &key) when its +4 word has `mask`, else 0. The key
// goes in a 16-byte local of the wrapper's frame (its r1 + 112).
inline void TFindFlagged(PPUContext& ctx, u32 gkey, u32 find, u32 map, u32 mask) {
    const u32 key = load<u32>(gkey);
    if (key == 0) {
        ctx.gpr[3] = 0;
        return;
    }
    Stack frame(ctx, 16);
    store<u32>(frame.addr(), key);
    const u32 p = load<u32>(static_cast<u32>(call(ctx, find, {map, frame.addr(), key})));
    ctx.gpr[3] = 0;
    if (p == 0) return;
    const u32 f = load<u32>(p + 4) & mask;
    ctx.gpr[4] = f;  // the masked flags, as left
    if (f != 0) ctx.gpr[3] = p;
}

// Copies a 4x3 block: rows of three words 16 bytes apart (the w words
// skipped), from a1 + soff to this + doff. f1 = the last word.
inline void TCopyMat43(PPUContext& ctx, u32 doff, u32 soff) {
    const u32 d = arg<u32>(ctx, 0) + doff, sp = arg<u32>(ctx, 1) + soff;
    u32 w = 0;
    for (u32 r = 0; r < 64; r += 16)
        for (u32 k = 0; k < 12; k += 4) {
            w = load<u32>(sp + r + k);
            store<u32>(d + r + k, w);
        }
    ctx.fpr[1] = static_cast<f64>(std::bit_cast<f32>(w));
}

// *(u32*)ea = 0, then fn(a0, 0, hi, 0) (r5 = the lis base of ea, as
// left); returns 1.
inline void TClearGlobalCall(PPUContext& ctx, u32 ea, u32 hi, u32 fn) {
    store<u32>(ea, 0);
    call(ctx, fn, {arg<u32>(ctx, 0), 0, hi, 0});
    ctx.gpr[3] = 1;
}

// ~T(): the member object at +off released with rel(q) (the class vtable
// stored first, r4 = it), then the base vtable and destructor.
inline void TDtorRelease(PPUContext& ctx, u32 vt, u32 off, u32 rel, u32 vtbase, u32 base) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 q = load<u32>(o + off);
    if (q != 0) {
        store<u32>(o, vt);
        call(ctx, rel, {q, vt});
    }
    store<u32>(o, vtbase);
    call(ctx, base, {o, vtbase});
}
inline void TDeleteRelease(PPUContext& ctx, u32 vt, u32 off, u32 rel, u32 vtbase, u32 base, u32 free_fn, u32 size) {
    const u32 o = arg<u32>(ctx, 0);
    if (o == 0) return;
    TDtorRelease(ctx, vt, off, rel, vtbase, base);
    call(ctx, free_fn, {o, size});
}

// ---------------------------------------------------------------- batch 20

// ~T(): q = this->+off; the vtable; fn(q, vt); then the base destructor.
inline void TDtorCallMember(PPUContext& ctx, u32 vt, u32 off, u32 fn, u32 base) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 q = load<u32>(o + off);
    store<u32>(o, vt);
    call(ctx, fn, {q, vt});
    call(ctx, base, {o});
}
// Its deleting version (fn(q, q) there, as the original's registers are).
inline void TDeleteCallMember(PPUContext& ctx, u32 vt, u32 off, u32 fn, u32 base, u32 free_fn, u32 size) {
    const u32 o = arg<u32>(ctx, 0);
    if (o == 0) return;
    const u32 q = load<u32>(o + off);
    store<u32>(o, vt);
    call(ctx, fn, {q, q});
    call(ctx, base, {o});
    call(ctx, free_fn, {o, size});
}

// bool: bit (32 - sh) of the word at (*clock) + k is clear.
inline void TGlobalBitClear(PPUContext& ctx, u32 clock, u32 k, u32 sh) {
    const u32 w = load<u32>(load<u32>(clock) + k);
    ctx.gpr[4] = k;  // as left
    ctx.gpr[3] = (((w << sh) | (w >> ((32 - sh) & 31))) & 1) == 0 ? 1 : 0;
}

// this->vfn[slot / 4](this): its result returned.
inline void TVCallThis(PPUContext& ctx, u32 slot) {
    const u32 o = arg<u32>(ctx, 0);
    VCall(ctx, o, slot, {o});
}

// ---------------------------------------------------------------- batch 21

// this->+off[0..1] = a1[0..1], then this->vfn[slot / 4](this).
inline void TSetVec2VCall(PPUContext& ctx, u32 off, u32 slot) {
    const u32 o = arg<u32>(ctx, 0), sp = arg<u32>(ctx, 1);
    const u32 w0 = load<u32>(sp);
    store<u32>(o + off, w0);
    const u32 w1 = load<u32>(sp + 4);
    store<u32>(o + off + 4, w1);
    ctx.fpr[1] = static_cast<f64>(std::bit_cast<f32>(w1));
    VCall(ctx, o, slot, {o});
}

// The same lookup as TFindFlagged with the key passed in (0 for a null key).
inline void TFindFlaggedArg(PPUContext& ctx, u32 find, u32 map, u32 mask) {
    const u32 key = arg<u32>(ctx, 0);
    ctx.gpr[4] = key;
    if (key == 0) {
        ctx.gpr[3] = 0;
        return;
    }
    Stack frame(ctx, 16);
    store<u32>(frame.addr(), key);
    const u32 p = load<u32>(static_cast<u32>(call(ctx, find, {map, frame.addr(), key})));
    ctx.gpr[3] = 0;
    if (p == 0) return;
    const u32 f = load<u32>(p + 4) & mask;
    ctx.gpr[4] = f;
    if (f != 0) ctx.gpr[3] = p;
}

// f32 Get(Node* n): from the node records (kNodeTable->+24, 128 bytes,
// by n's s16 +4 / +2 indices, -1 for none): the byte at boff of record 2's
// sub-block (+*0x0106CF7E, when its +2 byte is set) indexes the floats of
// record 1 (+*0x0106CF70). Quirk kept: a missing record reads address 2.
inline void TNodeF32(PPUContext& ctx, u32 boff) {
    const u32 n = arg<u32>(ctx, 0);
    const u32 tbl = load<u32>(o19fn::kNodeTable);
    const s32 i1 = static_cast<s16>(load<u16>(n + 4));
    const u32 i2raw = load<u16>(n + 2);
    u64 rec1 = 0;
    if (i1 != -1) rec1 = static_cast<u64>(load<u32>(tbl + 24)) + (static_cast<u64>(static_cast<s64>(i1)) << 7);
    const s32 i2 = static_cast<s16>(i2raw);
    u32 rec2 = 0;
    if (i2 != -1) rec2 = load<u32>(tbl + 24) + (static_cast<u32>(i2) << 7);
    const u32 b = load<u8>(rec2 + 2);
    u32 base = 0;
    if (static_cast<s32>(b) > 0) base = rec2 + load<u16>(0x0106CF7E);
    const u32 idx = static_cast<u32>(load<u8>(base + boff)) << 2;
    const u32 fo = load<u16>(0x0106CF70);
    const u32 ea = static_cast<u32>(rec1 + fo + idx);
    ctx.gpr[3] = ea;
    ctx.gpr[4] = rec1;
    ctx.gpr[5] = fo;
    ctx.gpr[6] = b;
    ctx.gpr[7] = static_cast<u64>(static_cast<s64>(i2));
    ctx.fpr[1] = static_cast<f64>(std::bit_cast<f32>(load<u32>(ea)));
}

// ~T(): the vtable; the buffer at +off freed (Mem_FreeFrom(p, 0)) and
// cleared (r3 = 0, r4 = the vtable without one).
inline void TDtorFreeField(PPUContext& ctx, u32 vt, u32 off) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 q = load<u32>(o + off);
    store<u32>(o, vt);
    ctx.gpr[3] = q;
    ctx.gpr[4] = vt;
    if (q == 0) return;
    call(ctx, o19fn::Mem_FreeFrom, {q, 0});
    store<u32>(o + off, 0);
    ctx.gpr[3] = 0;
}

// ---------------------------------------------------------------- batch 22

inline void TDeleteFreeField(PPUContext& ctx, u32 vt, u32 off, u32 free_fn, u32 size) {
    const u32 o = arg<u32>(ctx, 0);
    if (o == 0) return;
    TDtorFreeField(ctx, vt, off);
    call(ctx, free_fn, {o, size});
}

// Refresh(this): when this->+a is set and test(this->+(a+8)) fails, the
// old +(a+8) released and remade from make(this->+a, this->+(a+4)).
inline void TRemake(PPUContext& ctx, u32 a, u32 test, u32 rel, u32 make) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 src = load<u32>(o + a);
    ctx.gpr[3] = src;
    if (src == 0) return;
    if (static_cast<u32>(call(ctx, test, {load<u32>(o + a + 8)})) != 0) return;
    call(ctx, rel, {load<u32>(o + a + 8)});
    const u32 r = static_cast<u32>(call(ctx, make, {load<u32>(o + a), load<u32>(o + a + 4)}));
    store<u32>(o + a + 8, r);
}

// bool: any of the floats at (this->+216) + offs differs from *kaddr
// (unordered counts as different). r4 / f1 / f2 as left.
inline void TAnyF32Ne(PPUContext& ctx, u32 kaddr, std::initializer_list<u32> offs) {
    const u32 p = load<u32>(arg<u32>(ctx, 0) + 216);
    const f64 k = static_cast<f64>(std::bit_cast<f32>(load<u32>(kaddr)));
    ctx.gpr[4] = kaddr;
    ctx.fpr[1] = k;
    ctx.gpr[3] = p;
    for (u32 off : offs) {
        const f64 v = static_cast<f64>(std::bit_cast<f32>(load<u32>(p + off)));
        ctx.fpr[2] = v;
        if (!(v == k)) {
            ctx.gpr[3] = 1;
            return;
        }
    }
    ctx.gpr[3] = 0;
}

// ---------------------------------------------------------------- batch 23

// fn(k): a wrapper passing a constant (an address) as the only argument.
inline void TCallK(PPUContext& ctx, u32 fn, u32 k) { call(ctx, fn, {k}); }

// The three-vtable store through a secondary base in the order +0, +384,
// +380, then base(p) with r4-r6 = the +384, +0 and +380 vtables.
inline void TAdjDtorVt3b(PPUContext& ctx, s32 delta, u32 v0, u32 v384, u32 v380, u32 base) {
    const u32 o = static_cast<u32>(arg<u32>(ctx, 0) + static_cast<u32>(delta));
    store<u32>(o, v0);
    store<u32>(o + 384, v384);
    store<u32>(o + 380, v380);
    call(ctx, base, {o, v384, v0, v380});
}

}  // namespace obj19
}  // namespace sly4
