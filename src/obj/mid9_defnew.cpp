// Game code, medium sweep part 9: object factories that fill a stack
// record from their owner's settings (the record base built by
// Record_Init_000E44E0), let the def x look at it, then construct the
// object from it.
#include "mid9.h"

using namespace sly4;
using namespace sly4::mid9;

namespace {

constexpr u32 Record_Init_000E44E0 = 0x000E44E0;  // (rec, r, owner, 0, &vecA, &vecB, 1, 0) (name?)
constexpr u32 kVecBD0 = 0x01164BD0, kVecB30 = 0x01164B30;

// The shared body: rec = the original's r1 + 144; r = vfn 32(o, x);
// (fill returns the f32 word left in f1.)
// Record_Init_000E44E0(rec, r, o, 0, 0x01164BD0, 0x01164B30, 1, 0) (with
// four zero stack arguments when zeros); fill(rec); vfn 8(x, rec); after
// (rec) (between that and the allocation); p = SmallAlloc(size) (memAlloc:
// Mem_Alloc(size, 0, 16)), ctor(p, rec) when that succeeded; vfn 8(q, ...)
// on q = p or 0 (quirk: a failed allocation calls through the vtable read
// from address 0); the record's vectors +164 / +144 cleared (PtrVector_Clear,
// Nop) and its heap vector +0 deleted. Returns q. post(ctx, p, rec) runs
// right after the constructor.
auto NoPost = [](PPUContext&, u32, u32) {};

template <typename Fill, typename After, typename Post = decltype(NoPost)>
void OwnerNew(PPUContext& ctx, u32 size, bool memAlloc, u32 ctor, bool zeros, Fill fill, After after, Post post = NoPost) {
    const u32 o = arg<u32>(ctx, 0), x = arg<u32>(ctx, 1);
    Stack frame(ctx, 256);
    const u32 rec = frame.addr();
    const u32 r = static_cast<u32>(obj19::VCall(ctx, o, 32, {o, x}));
    if (zeros)
        call(ctx, Record_Init_000E44E0, {rec, r, o, 0, kVecBD0, kVecB30, 1, 0, 0, 0, 0, 0});
    else
        call(ctx, Record_Init_000E44E0, {rec, r, o, 0, kVecBD0, kVecB30, 1, 0});
    const u32 f1w = fill(o, rec);  // f1 keeps the last f32 loaded (lfs) across the calls
    ctx.fpr[1] = std::bit_cast<f64>(LfsBits(f1w));
    obj19::VCall(ctx, x, 8, {x, rec});
    after(rec);
    const u32 p = memAlloc ? static_cast<u32>(call(ctx, m7fn::Mem_Alloc, {size, 0, 16})) : static_cast<u32>(call(ctx, m7fn::SmallAlloc, {size}));
    u32 q = 0;
    if (p != 0) {
        call(ctx, ctor, {p, rec});
        post(ctx, p, rec);
        q = p;
    }
    obj19::VCall(ctx, q, 8, {q});
    call(ctx, mid1::m1fn::PtrVector_Clear, {rec + 164});
    call(ctx, mid1::m1fn::Nop, {rec + 164});
    call(ctx, mid1::m1fn::PtrVector_Clear, {rec + 144});
    call(ctx, mid1::m1fn::Nop, {rec + 144});
    mid1::PtrVecDeleteSmall(ctx, rec);
    ret(ctx, q);
}

auto NoAfter = [](u32) {};


void OrFlags(u32 rec, u32 bits) { store<u32>(rec + 8, load<u32>(rec + 8) | bits); }

// The f32 field: o's (when o is set) or the constant word k.
u32 OF(u32 o, u32 off, u32 k) { return o != 0 ? load<u32>(o + off) : load<u32>(k); }

// The seven f32 +208.. from o+308 + 8i (or the f32 at k), +236 = 0, the
// flag 0x80000 in +8.
void Fill7(u32 o, u32 rec, u32 k) {
    store<u32>(rec + 236, 0);
    OrFlags(rec, 0x00080000);
    for (u32 i = 0; i < 7; ++i) store<u32>(rec + 208 + 4 * i, OF(o, 308 + 8 * i, k));
}

// Bits 31 / 30 of the record word w from bits b31 / b30 of o's word (o
// null: d31 / d30).
void Bits2(u32 o, u32 off, u32 w, u32 b31, u32 b30, u32 d31, u32 d30) {
    u32 v = load<u32>(w) & 0x7FFFFFFFu;
    const u32 x = o != 0 ? (load<u32>(o + off) >> b31) & 1u : d31;
    v |= x << 31;
    const u32 y = o != 0 ? (load<u32>(o + off) >> b30) & 1u : d30;
    v = (v & ~0x40000000u) | (y << 30);
    store<u32>(w, v);
}

}  // namespace

// Obj* Owner_New1C96B8(Owner* o, Def* x): OwnerNew (656 bytes, ctor
// 0x001C8DB8): Fill7 (constant 0x001C96B4), then the flag 0x00800000.
DECOMP(001C96B8, Owner_New1C96B8) {
    OwnerNew(ctx, 656, false, 0x001C8DB8, false, [](u32 o, u32 rec) {
        Fill7(o, rec, 0x001C96B4);
        OrFlags(rec, 0x00800000);
        return load<u32>(rec + 232);
    }, NoAfter);
}

// Obj* Owner_New1ADF30(Owner* o, Def* x): OwnerNew (Mem_Alloc(560, 0, 16),
// ctor 0x001ADD08): Fill7 (constant 0x001ADF2C).
DECOMP(001ADF30, Owner_New1ADF30) {
    OwnerNew(ctx, 560, true, 0x001ADD08, false, [](u32 o, u32 rec) {
        Fill7(o, rec, 0x001ADF2C);
        return load<u32>(rec + 232);
    }, NoAfter);
}

// Obj* Owner_New1C2E20(Owner* o, Def* x): OwnerNew (608 bytes, ctor
// 0x001C2BE0): Fill7 (constant 0x001C2E18), f32 +240 = o->440 (or the f32
// at 0x001C2E1C), the flag 0x01000000.
DECOMP(001C2E20, Owner_New1C2E20) {
    OwnerNew(ctx, 608, false, 0x001C2BE0, false, [](u32 o, u32 rec) {
        Fill7(o, rec, 0x001C2E18);
        store<u32>(rec + 240, OF(o, 440, 0x001C2E1C));
        OrFlags(rec, 0x01000000);
        return load<u32>(rec + 240);
    }, NoAfter);
}

// Obj* Owner_New1C0A10(Owner* o, Def* x): OwnerNew (592 bytes, ctor
// 0x001C08AC): Fill7 (constant 0x001C0A04), f32 +240 = o->408 (or the f32
// at 0x001C0A08), bits 31 / 30 of +244 = bits 31 / 30 of o->412 (no o: 0 /
// 1), the flag 0x00200000.
DECOMP(001C0A10, Owner_New1C0A10) {
    OwnerNew(ctx, 592, false, 0x001C08AC, false, [](u32 o, u32 rec) {
        Fill7(o, rec, 0x001C0A04);
        store<u32>(rec + 240, OF(o, 408, 0x001C0A08));
        Bits2(o, 412, rec + 244, 31, 30, 0, 1);
        OrFlags(rec, 0x00200000);
        return load<u32>(rec + 240);
    }, NoAfter);
}

// Obj* Owner_New1D40A8(Owner* o, Def* x): OwnerNew (576 bytes, ctor
// 0x001ADD08): Fill7 (constant 0x001D409C), +240 = o->400 (or 0), f32 +244
// = o->404 (or the f32 at 0x001D40A0), the flag 0x04000000. After the
// constructor: vtables 0x00B17728 (+0) / 0x00B178FC (+380) / 0x00B17924
// (+384), +556 0, +560 = rec+240, f32 +564 = (rec+244)^2, the byte +569 0,
// bit 7 of the byte +568 cleared.
DECOMP(001D40A8, Owner_New1D40A8) {
    OwnerNew(ctx, 576, false, 0x001ADD08, false, [](u32 o, u32 rec) {
        Fill7(o, rec, 0x001D409C);
        store<u32>(rec + 240, o != 0 ? load<u32>(o + 400) : 0);
        store<u32>(rec + 244, OF(o, 404, 0x001D40A0));
        OrFlags(rec, 0x04000000);
        return load<u32>(rec + 244);
    }, NoAfter, [](PPUContext& c, u32 p, u32 rec) {
        constexpr u32 kVt = 0x00B17720;
        store<u32>(p + 556, 0);
        store<u32>(p, kVt + 8);
        store<u32>(p + 380, kVt + 476);
        store<u32>(p + 384, kVt + 516);
        store<u32>(p + 560, load<u32>(rec + 240));
        const f32 f = F32(load<u32>(rec + 244));
        const f32 sq = static_cast<f32>(static_cast<f64>(f) * f);
        const u32 b = load<u8>(p + 568);
        store<u8>(p + 569, 0);
        store<u8>(p + 568, static_cast<u8>(b & ~0x80u));
        store<u32>(p + 564, W32(sq));
        c.fpr[1] = static_cast<f64>(sq);
    });
}

// Obj* Owner_New1A9C80(Owner* o, Def* x): OwnerNew (480 bytes, ctor
// 0x001A990C; four zero stack arguments): f32 +208..+220 = o->304..316 (or
// the f32 at 0x001A9C7C), the byte +224 = whether o->320 (u8) is set (no o:
// 0), the flag 0x2000.
DECOMP(001A9C80, Owner_New1A9C80) {
    OwnerNew(ctx, 480, false, 0x001A990C, true, [](u32 o, u32 rec) {
        constexpr u32 k = 0x001A9C7C;
        for (u32 i = 0; i < 4; ++i) store<u32>(rec + 208 + 4 * i, OF(o, 304 + 4 * i, k));
        const u32 b = o != 0 ? load<u8>(o + 320) : 0;
        OrFlags(rec, 0x2000);
        store<u8>(rec + 224, b != 0 ? 1 : 0);
        return load<u32>(rec + 220);
    }, NoAfter);
}

// Obj* Owner_New1B1EE8(Owner* o, Def* x): OwnerNew (576 bytes, ctor
// 0x001B1990; four zero stack arguments): f32 +208..+224 = o->316..332 (or
// the constants k[0], k[1], k[0], k[0], k[2] at 0x001B1EDC), the flag
// 0x10000.
DECOMP(001B1EE8, Owner_New1B1EE8) {
    OwnerNew(ctx, 576, false, 0x001B1990, true, [](u32 o, u32 rec) {
        constexpr u32 k = 0x001B1EDC;
        constexpr u32 ks[5] = {k, k + 4, k, k, k + 8};
        for (u32 i = 0; i < 5; ++i) store<u32>(rec + 208 + 4 * i, OF(o, 316 + 4 * i, ks[i]));
        OrFlags(rec, 0x10000);
        return load<u32>(rec + 224);
    }, NoAfter);
}

// Obj* Owner_New1AAF38(Owner* o, Def* x): OwnerNew (480 bytes, ctor
// 0x001AACEC; four zero stack arguments): f32 +208..+224 = o->304..320 (or
// the f32 at 0x001AAF30), bits 31 / 30 of +228 = bits 31 / 30 of o->328
// (no o: 1 / 0), the flag 0x4000.
DECOMP(001AAF38, Owner_New1AAF38) {
    OwnerNew(ctx, 480, false, 0x001AACEC, true, [](u32 o, u32 rec) {
        constexpr u32 k = 0x001AAF30;
        for (u32 i = 0; i < 5; ++i) store<u32>(rec + 208 + 4 * i, OF(o, 304 + 4 * i, k));
        Bits2(o, 328, rec + 228, 31, 30, 1, 0);
        OrFlags(rec, 0x4000);
        return load<u32>(rec + 224);
    }, NoAfter);
}

// Obj* Owner_New1AF830(Owner* o, Def* x): OwnerNew (480 bytes, ctor
// 0x001AF6A0; four zero stack arguments): f32 +208 / +212 / +216 =
// o->348 / 352 / 360 (or the f32 k[0..2] at 0x001AF824), bits 31 / 30 of
// +220 = bits 23 / 22 of o->372 (no o: 1 / 0), the flag 0x40000.
DECOMP(001AF830, Owner_New1AF830) {
    OwnerNew(ctx, 480, false, 0x001AF6A0, true, [](u32 o, u32 rec) {
        constexpr u32 k = 0x001AF824;
        store<u32>(rec + 208, OF(o, 348, k));
        store<u32>(rec + 212, OF(o, 352, k + 4));
        store<u32>(rec + 216, OF(o, 360, k + 8));
        Bits2(o, 372, rec + 220, 23, 22, 1, 0);
        OrFlags(rec, 0x40000);
        return load<u32>(rec + 216);
    }, NoAfter);
}

// Obj* Owner_New1E60E0(Owner* o, Def* x): OwnerNew (624 bytes, ctor
// 0x001E5CD8): +208 = o->432 (or 0), f32 +224 / +228 / +232 = o->448..456
// (or the f32 at 0x001E60DC), +240 = o->464 (or 0), the flag 0x10000000;
// after vfn 8(x, rec) the flag 0x10000 in +20.
DECOMP(001E60E0, Owner_New1E60E0) {
    OwnerNew(ctx, 624, false, 0x001E5CD8, false, [](u32 o, u32 rec) {
        constexpr u32 k = 0x001E60DC;
        store<u32>(rec + 208, o != 0 ? load<u32>(o + 432) : 0);
        for (u32 i = 0; i < 3; ++i) store<u32>(rec + 224 + 4 * i, OF(o, 448 + 4 * i, k));
        store<u32>(rec + 240, o != 0 ? load<u32>(o + 464) : 0);
        OrFlags(rec, 0x10000000);
        return load<u32>(rec + 224);
    }, [](u32 rec) { store<u32>(rec + 20, load<u32>(rec + 20) | 0x10000); });
}
