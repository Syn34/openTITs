// Game code, medium sweep part 25: destructors that release nested
// PtrVector containers (PtrVector_Clear / Nop / Mem_FreeFrom chains).
#include "mid25.h"

using namespace sly4;
using namespace sly4::mid25;

namespace {

// PtrVector_Clear(v); Nop(v).
void ClearV(PPUContext& ctx, u32 v) {
    call(ctx, m25fn::PtrVector_Clear, {v});
    call(ctx, m25fn::Nop, {v});
}

// PtrVector_Clear(v); Nop(v); Mem_FreeFrom(v, 0).
void KillV(PPUContext& ctx, u32 v) {
    ClearV(ctx, v);
    call(ctx, m25fn::Mem_FreeFrom, {v, 0});
}

// The body shared by the 452 / 456-byte destructors: three 116-byte groups
// at o + base (+116 each) whose first vector {data, .., u16 count +8} holds
// vectors of heap vectors; every inner element is killed (Clear / Nop /
// Mem_FreeFrom), each inner vector's count cleared (also when the vector
// pointer is null: a quirk of the original, which stores to 0 + 8) and the
// vector killed when set, then the outer count cleared. Then the five
// vectors of each group (+80, +60, +40, +20, +0; last group first) and the
// vectors o + v0 + 40, + 20, + 0 are cleared.
void NestedVecDtor(PPUContext& ctx, u32 o, u32 base, u32 v0) {
    for (u32 k = 0; k < 3; ++k) {
        const u32 L = o + base + 116 * k;
        for (u32 i = 0; i < load<u16>(L + 8); ++i) {
            const u32 sub = load<u32>(load<u32>(L) + 4 * i);
            for (u32 j = 0; j < load<u16>(sub + 8); ++j) {
                const u32 e = load<u32>(load<u32>(sub) + 4 * j);
                if (e != 0) KillV(ctx, e);
            }
            store<u16>(sub + 8, 0);  // quirk: also for a null sub
            if (sub != 0) KillV(ctx, sub);
        }
        store<u16>(L + 8, 0);
    }
    for (s32 k = 2; k >= 0; --k) {
        const u32 G = o + base + 116 * static_cast<u32>(k);
        ClearV(ctx, G + 80);
        ClearV(ctx, G + 60);
        ClearV(ctx, G + 40);
        ClearV(ctx, G + 20);
        ClearV(ctx, G);
    }
    ClearV(ctx, o + v0 + 40);
    ClearV(ctx, o + v0 + 20);
    ClearV(ctx, o + v0);
}

// Pops the u16-counted vector at o + v (data +0, count +8) until empty: the
// first element is handled by `each` and the last element moved into its
// place (the count decremented first; nothing moved once it reaches 0).
template <class F>
void DrainFront(PPUContext& ctx, u32 o, u32 v, F each) {
    while (load<u16>(o + v + 8) != 0) {
        each(ctx);
        const u32 n = load<u16>(o + v + 8) - 1;
        store<u16>(o + v + 8, static_cast<u16>(n));
        if ((n & 0xFFFF) == 0) continue;
        const u32 d = load<u32>(o + v);
        store<u32>(d, load<u32>(d + ((n << 2) & 0x3FFFCu)));
    }
}

// x = *(*p): x->+220 = 0, then (x re-read) bit 0x1000 of the u16 x->+176 set.
void Detach(u32 p) {
    store<u32>(load<u32>(load<u32>(p)) + 220, 0);
    const u32 x = load<u32>(load<u32>(p));
    store<u16>(x + 176, static_cast<u16>(load<u16>(x + 176) | 0x1000u));
}

}  // namespace

// T::~T(o) (Delete_00062DD0, 452 bytes): vtable 0x00B0EF20; the nested
// vectors of the groups at o + 88 (NestedVecDtor) and the vectors o + 68 /
// 48 / 28 released; vtable 0x00B0DAAC and SortedMap_Remove7330C0(0x01056F04,
// o + 4) (r3 = its).
DECOMP(00062B20, Obj_Dtor_B0EF20_00062B20) {
    const u32 o = arg<u32>(ctx, 0);
    store<u32>(o, 0x00B0EF20u);
    NestedVecDtor(ctx, o, 88, 28);
    store<u32>(o, 0x00B0DAACu);
    call(ctx, 0x007330C0u, {0x01056F04u, o + 4});
}

// T::~T(o) (Delete_00064B8C, 456 bytes): vtable 0x00B0EED8; the nested
// vectors of the groups at o + 92 and the vectors o + 72 / 52 / 32
// released; vtable 0x00B0DDC0; o->+24->vt[12]() (not null-checked), then a
// set o->+24 gets vt[4](); Dtor_B1BE30_002505A0(o) (r3 = its).
DECOMP(00064894, Obj_Dtor_B0EED8_00064894) {
    const u32 o = arg<u32>(ctx, 0);
    store<u32>(o, 0x00B0EED8u);
    NestedVecDtor(ctx, o, 92, 32);
    const u32 a = load<u32>(o + 24);
    store<u32>(o, 0x00B0DDC0u);
    VCall(ctx, a, 12, {a});
    if (const u32 b = load<u32>(o + 24); b != 0) VCall(ctx, b, 4, {b});
    call(ctx, 0x002505A0u, {o});
}

// T::~T(o) (Delete_003EE91C): vtable 0x00B20710; the stack o + 112 is
// drained (an element's +8 object, when set, gets +220 = 0 and bit 0x1000
// of +176; the element is freed); the stack o + 144 likewise (a set
// element's vector +32 cleared, the element freed); the stack o + 164 of
// vectors (each element's object gets +220 = 0 / bit 0x1000 of +176, the
// count cleared, the vector killed); then the three vectors are cleared
// and VtB21988_Dtor2(o) runs (r3 = its).
DECOMP(003EE694, Obj_Dtor_B20710_003EE694) {
    const u32 o = arg<u32>(ctx, 0);
    store<u32>(o, 0x00B20710u);
    DrainFront(ctx, o, 112, [&](PPUContext& c) {
        const u32 d = load<u32>(o + 112);
        if (const u32 x = load<u32>(load<u32>(d) + 8); x != 0) {
            store<u32>(x + 220, 0);
            const u32 x2 = load<u32>(load<u32>(d) + 8);
            store<u16>(x2 + 176, static_cast<u16>(load<u16>(x2 + 176) | 0x1000u));
        }
        const u32 d2 = load<u32>(o + 112);
        if (const u32 e = load<u32>(d2); e != 0) call(c, m25fn::Mem_FreeFrom, {e, 0});
    });
    DrainFront(ctx, o, 144, [&](PPUContext& c) {
        const u32 e = load<u32>(load<u32>(o + 144));
        if (e == 0) return;
        ClearV(c, e + 32);
        call(c, m25fn::Mem_FreeFrom, {e, 0});
    });
    DrainFront(ctx, o, 164, [&](PPUContext& c) {
        const u32 v = load<u32>(load<u32>(o + 164));
        if (v == 0) return;
        for (u32 i = 0; i < load<u16>(v + 8); ++i) Detach(load<u32>(v) + 4 * i);
        store<u16>(v + 8, 0);
        KillV(c, v);
    });
    ClearV(ctx, o + 164);
    ClearV(ctx, o + 144);
    ClearV(ctx, o + 112);
    call(ctx, 0x00455A94u, {o});
}

// void f(T* o) (called by 0x00322D88): the array o->+276 (count at -4, 8-byte
// entries) has its set entries freed from the last down and cleared, then is
// freed itself (from its header) and the word cleared; the five words
// o + 52 + 8k each hold a vector V whose vector V->+0 lists heap vectors:
// each is killed, V->+0 is cleared (PtrVector_Clear / Nop) and
// SmallFree(V->+0, 20), V freed and the word cleared; o->+348 / +336 / +324
// / +312 freed and cleared; the 20-byte vector o->+92 cleared and
// SmallFree'd. r3 as the original leaves it.
DECOMP(007691D4, Obj_Release_007691D4) {
    const u32 o = arg<u32>(ctx, 0);
    if (const u32 a = load<u32>(o + 276); a != 0) {
        for (s32 i = static_cast<s32>(load<u32>(a - 4) - 1); i >= 0; --i) {
            const u32 s = a + 8 * static_cast<u32>(i);
            const u32 e = load<u32>(s);
            if (e == 0) continue;
            call(ctx, m25fn::Mem_FreeFrom, {e, 0});
            store<u32>(s, 0);
        }
        call(ctx, m25fn::Mem_FreeFrom, {a - 4, 0});
        store<u32>(o + 276, 0);
    }
    for (u32 k = 0; k < 5; ++k) {
        const u32 slot = o + 52 + 8 * k;
        u32 V = load<u32>(slot);
        if (V == 0) continue;
        for (u32 j = 0;; ++j) {
            const u32 W = load<u32>(V);
            if (W == 0 || j >= load<u16>(W + 8)) break;
            const u32 e = load<u32>(load<u32>(W) + 4 * j);
            if (e == 0) continue;
            KillV(ctx, e);
            V = load<u32>(slot);
        }
        if (V == 0) continue;
        if (const u32 W = load<u32>(V); W != 0) {
            ClearV(ctx, W);
            call(ctx, m25fn::SmallFree, {load<u32>(V), 20});
        }
        call(ctx, m25fn::Mem_FreeFrom, {V, 0});
        store<u32>(slot, 0);
    }
    u32 r3 = 0;
    for (u32 off : {348u, 336u, 324u, 312u}) {
        const u32 p = load<u32>(o + off);
        r3 = p;
        if (p == 0) continue;
        r3 = static_cast<u32>(call(ctx, m25fn::Mem_FreeFrom, {p, 0}));
        store<u32>(o + off, 0);
    }
    if (const u32 v = load<u32>(o + 92); v != 0) {
        ClearV(ctx, v);
        call(ctx, m25fn::SmallFree, {load<u32>(o + 92), 20});
        return;
    }
    ctx.gpr[3] = r3;
}
