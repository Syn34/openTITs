// Game code, medium sweep part 14: tuning copies, parameter registration and
// small callbacks.
#include "mid14.h"

using namespace sly4;
using namespace sly4::mid14;

namespace {
constexpr u32 kTuning33488 = 0x01033488;  // 76-byte tuning values (f32 / u32 at +72)
constexpr u32 Obj_RegisterParam = 0x004F7128;  // (o, const ParamDesc*)
constexpr u32 Binding_Reserve = 0x004D2BB8;    // (binding, n)
constexpr u32 Cb_Apply_0012BD94 = 0x0012BD94;  // (o, bool, u32) (reads r3..r5) (name?)
}  // namespace

// Tuning* Tuning_Copy119D8C(Tuning* t)
// (called by 0001BFE0) t+4 + 4k = the value (+72) of the 76-byte tuning
// entry k at 0x01033488 for k = 0..37 (words copied as they are; +88 is
// written after +116 in the original). Returns t.
DECOMP(00119D8C, Tuning_Copy119D8C) {
    const u32 t = arg<u32>(ctx, 0);
    auto put = [&](u32 off) { Cw(t + off, kTuning33488 + 76 * (off / 4 - 1) + 72); };
    for (u32 off = 4; off <= 84; off += 4) put(off);
    for (u32 off = 92; off <= 116; off += 4) put(off);
    put(88);
    for (u32 off = 120; off <= 152; off += 4) put(off);
    SetF1(ctx, kTuning33488 + 76 * 36 + 72);  // leftover: the last lfs (entry for +148)
    ret(ctx, t);
}

// AnimBlend::Sample4B5EB0(AnimBlend* o, Pool* pool, Sample* out, const Ctx* cx, s32 idx)
// (virtual; no direct callers) Bits of byte out+4: 0x80 cleared, 0x20 =
// o+66. With o+60 children (first record index u16 o+64 into the pool's
// 76-byte records {+4 header (+2 u16 count), +12 data}): idx >= 0 (or
// below the count) picks child idx directly; otherwise the first child
// whose f32 +24 reaches cx+8 is the upper key and the one before it the
// lower. out+0 = 1 and the lower key's +8..+20 to out+20..+32, flags of
// byte o+20 (bit 6 -> out+41, bit 2 -> out+42), o+21 -> out+40 and o+24 /
// +28 / +32 -> out+36 / +44 / +52. With an upper key too: out+0 = 2, its
// +8..+20 to out+60..+72, the same fields at out+76..+92, its +28 at out+100
// and the blend weight w = (cx+8 - lo) / (hi - lo) clamped to [0, 1]
// (fsel) in out+96, 1 - w in out+56. Out-of-range children read record 0
// (the original's address 4 quirk). Returns o, or 0x004B5E98 (its
// constants' address, left in r3) when two keys were written.
DECOMP(004B5EB0, AnimBlend_Sample4B5EB0) {
    const u32 o = arg<u32>(ctx, 0), pool = arg<u32>(ctx, 1), out = arg<u32>(ctx, 2), cx = arg<u32>(ctx, 3);
    const s32 idx = static_cast<s32>(arg<u32>(ctx, 4));
    u32 b = load<u8>(out + 4) & ~0x80u;
    store<u8>(out + 4, static_cast<u8>(b));
    b = (b & ~0x20u) | ((static_cast<u32>(load<u8>(o + 66)) << 5) & 0x20u);
    store<u8>(out + 4, static_cast<u8>(b));
    const u32 n = load<u32>(o + 60), first = load<u16>(o + 64);
    if (n == 0) {
        ret(ctx, o);
        return;
    }
    // Record k of the pool (0 when k is out of range), plus 4.
    auto key = [&](u32 k16) -> u32 {
        const s32 k = static_cast<s16>(static_cast<u16>(k16));
        u32 e = 0;
        if (k >= 0 && k < static_cast<s32>(load<u16>(load<u32>(pool + 4) + 2))) e = load<u32>(pool + 12) + 76u * static_cast<u32>(k);
        return e + 4;
    };
    u32 lo = 0, hi = 0;
    if (idx >= 0 || idx < static_cast<s32>(n)) {
        lo = key(first + static_cast<u32>(idx));
    } else {
        const f64 f = LfsF(load<u32>(cx + 8));
        for (u32 i = 0; i < n; ++i) {
            const u32 e = key(first + i);
            if (const f64 v = LfsF(load<u32>(e + 20)); v > f || v == f) {
                hi = e;
                break;
            }
            lo = e;
        }
    }
    if (lo == 0) {
        lo = hi;
        hi = 0;
    }
    store<u32>(out, 1);
    for (u32 k = 0; k < 4; ++k) Cw(out + 20 + 4 * k, lo + 4 + 4 * k);
    const u32 w28 = load<u32>(o + 28);
    store<u8>(out + 41, static_cast<u8>((load<u8>(o + 20) >> 6) & 1));
    const u32 b20 = load<u8>(o + 20);
    store<u32>(out + 44, w28);
    const u32 w24 = load<u32>(o + 24);
    store<u32>(out + 36, w24);
    const u32 w32 = load<u32>(o + 32);
    store<u32>(out + 52, w32);
    store<u8>(out + 42, static_cast<u8>((b20 >> 2) & 1));
    const u8 b21 = load<u8>(o + 21);
    store<u8>(out + 40, b21);
    ctx.fpr[1] = LfsF(w32);  // leftover f1
    if (hi == 0) {
        ret(ctx, o);
        return;
    }
    store<u32>(out, 2);
    for (u32 k = 0; k < 4; ++k) Cw(out + 60 + 4 * k, hi + 4 + 4 * k);
    const u32 r3 = (load<u8>(o + 20) >> 6) & 1;
    store<u32>(out + 84, w28);
    store<u32>(out + 76, w24);
    store<u32>(out + 92, w32);
    store<u8>(out + 80, b21);
    const f64 f = LfsF(load<u32>(cx + 8));
    store<u8>(out + 81, static_cast<u8>(r3));
    Cw(out + 100, hi + 24);
    const f64 vh = LfsF(load<u32>(hi + 20)), vl = LfsF(load<u32>(lo + 20));
    const f64 den = static_cast<f32>(vh - vl), num = static_cast<f32>(f - vl);
    f64 w = static_cast<f32>(num / den);
    const u32 kk = 0x004B5E98;  // f32 1, f64 1, f64 0
    const f64 one = LfsF(load<u32>(kk));
    const f64 d1 = std::bit_cast<f64>(load<u64>(kk + 8)), d0 = std::bit_cast<f64>(load<u64>(kk + 16));
    const f64 r5 = static_cast<f32>(one - w);
    w = static_cast<f32>(r5 >= 0.0 ? w : d1);
    w = static_cast<f32>(w >= 0.0 ? w : d0);
    const f64 rest = static_cast<f32>(one - w);
    StoreF(out + 96, w);
    StoreF(out + 56, rest);
    ctx.fpr[1] = w;
    (void)r3;  // computed into r3, then r3 is reused for the constants' address
    ret(ctx, kk);
}

// u32 AnimCb_Select396068(const Ctrl* self, Event* ev)
// (callback; no direct callers) o = ev+12; nothing (returns 0) unless o is
// set with bit 0x8 of its word +192. Rows a / b of the table at
// (*0x00F65038)+24 (128 bytes each) come from the s16 indices self+4 /
// self+2 (-1: none, row 0 at address 0). Three bytes of row b (at
// +u16 0x0106CF7E when its byte +2 > 0, else address 0) index words of row
// a (+u16 0x0106CF70 + 4 * byte): A, B and C. Calls 0x0012BD94(o,
// A == 0x04A5C9FC, C when bit 0 of o+188 and C != 0, else B). Returns 1.
DECOMP(00396068, AnimCb_Select396068) {
    const u32 self = arg<u32>(ctx, 0);
    const u32 o = load<u32>(arg<u32>(ctx, 1) + 12);
    if (o == 0 || (load<u32>(o + 192) & 0x8u) == 0) {
        ret(ctx, 0u);
        return;
    }
    const u32 tbl = load<u32>(0x00F65038u);
    const s32 ia = static_cast<s16>(load<u16>(self + 4)), ib = static_cast<s16>(load<u16>(self + 2));
    auto rowA = [&]() -> u32 { return ia != -1 ? load<u32>(tbl + 24) + (static_cast<u32>(ia) << 7) : 0u; };
    auto selB = [&]() -> u32 {
        const u32 rb = ib != -1 ? load<u32>(tbl + 24) + (static_cast<u32>(ib) << 7) : 0u;
        return static_cast<s32>(load<u8>(rb + 2)) > 0 ? rb + load<u16>(0x0106CF7Eu) : 0u;
    };
    const u32 base = load<u16>(0x0106CF70u);
    const u32 a = load<u32>(rowA() + base + 4u * load<u8>(selB()));
    const u32 bb = load<u32>(rowA() + base + 4u * load<u8>(selB() + 1));
    const u32 sel2 = selB();
    const u32 bit = load<u32>(o + 188) & 1u;
    const u32 c = load<u32>(rowA() + base + 4u * load<u8>(sel2 + 2));
    const u32 v = (bit == 0 || c == 0) ? bb : c;
    call(ctx, Cb_Apply_0012BD94, {o, a == 0x04A5C9FCu ? 1u : 0u, v});
    ret(ctx, 1u);
}

// u32 ParamsA_Register1CE40(Obj* o)
// (registration callback; no direct callers) Registers the 34 parameter
// descriptors at 0x00F94784 (24 bytes each). Returns the last call's r3.
DECOMP(0001CE40, ParamsA_Register1CE40) {
    const u32 o = arg<u32>(ctx, 0);
    u64 r = 0;
    for (u32 k = 0; k < 34; ++k) r = call(ctx, Obj_RegisterParam, {o, 0x00F94784u + 24 * k});
    ret(ctx, r);
}

// u32 ParamsB_Register43E1D8(Obj* o)
// (registration callback; no direct callers) Registers the descriptors at
// 0x010ABEA8 + 24k (k = 0..8), 0x010ABF98, 0x010ABFB0 and 0x010ABF80; on
// the binding at +16 of the last: Binding_Reserve(2) and, while its count
// +0 is below +72, a 100-byte copy (Mem_Alloc(100, 0, 16) into array +68,
// String_CopyN) of each of the two names at 0x00F65924. Then 16 groups of
// six descriptors at 0x010ABFC8 + 144i (+0, +24, +72, +48, +96, +120).
// Last: byte +40 of o+8 |= 0x80 with its +44 = 0x00AE37A8, and o+88 =
// 0x00F33600 (returned).
DECOMP(0043E1D8, ParamsB_Register43E1D8) {
    const u32 o = arg<u32>(ctx, 0);
    for (u32 k = 0; k < 9; ++k) call(ctx, Obj_RegisterParam, {o, 0x010ABEA8u + 24 * k});
    call(ctx, Obj_RegisterParam, {o, 0x010ABF98u});
    call(ctx, Obj_RegisterParam, {o, 0x010ABFB0u});
    const u32 pd = 0x010ABF80u;
    call(ctx, Obj_RegisterParam, {o, pd});
    call(ctx, Binding_Reserve, {load<u32>(pd + 16), 2});
    for (u32 k = 0; k < 2; ++k) {
        const u32 name = load<u32>(0x00F65924u + 4 * k);
        const u32 bd = load<u32>(pd + 16);
        if (load<u32>(bd) < load<u32>(bd + 72)) {
            const u32 p = static_cast<u32>(call(ctx, m1fn::Mem_Alloc, {100, 0, 16}));
            store<u32>(load<u32>(bd + 68) + (load<u32>(bd) << 2), p);
            call(ctx, m1fn::String_CopyN, {load<u32>(load<u32>(bd + 68) + (load<u32>(bd) << 2)), name, 100});
            store<u32>(bd, load<u32>(bd) + 1);
        }
    }
    for (u32 i = 0; i < 16; ++i) {
        const u32 g = 0x010ABFC8u + 144 * i;
        for (u32 off : {0u, 24u, 72u, 48u, 96u, 120u}) call(ctx, Obj_RegisterParam, {o, g + off});
    }
    const u32 x = load<u32>(o + 8);
    const u8 b = static_cast<u8>(load<u8>(x + 40) | 0x80u);
    store<u32>(x + 44, 0x00AE37A8u);
    store<u8>(x + 40, b);
    store<u32>(o + 88, 0x00F33600u);
    ret(ctx, 0x00F33600u);
}

// u32 RPlayDirScript_Check394840(const Ctrl* self, Owner* o)
// (callback; no direct callers) Slot k = s16 self+6: the 132-byte record
// o+152 + 132k when k is below u16 o+144 and its s32 +0 is below 10, else
// none (0). With rows a / b from self+4 / self+2 as in AnimCb_Select396068,
// word A of row a: when A is 0, prints "RPlayDirScriptDef: No dir script
// name pass in!" (String_Format 0x00ADDBEC) and then (0x00ADD2EC) with the
// name at ((*(0x00F65100 + 44 * s16 slot+4))+40) (or "NULL") and that
// message, sets bit 0x8000 of u16 slot+6 and clears the word at slot +
// u16 0x0106CF80 + 4 (both through address 0 when there is no slot, the
// original's quirk). Returns k (A set) or the new u16.
DECOMP(00394840, RPlayDirScript_Check394840) {
    const u32 self = arg<u32>(ctx, 0), o = arg<u32>(ctx, 1);
    const s32 k = static_cast<s16>(load<u16>(self + 6));
    const bool in = k < static_cast<s32>(load<u16>(o + 144));
    auto slot = [&]() -> u32 {
        if (!in) return 0;
        const u32 r = o + 152 + static_cast<u32>(k) * 132;
        return static_cast<s32>(load<u32>(r)) < 10 ? r : 0u;
    };
    const s32 ia = static_cast<s16>(load<u16>(self + 4)), ib = static_cast<s16>(load<u16>(self + 2));
    const u32 clr = slot() + load<u16>(0x0106CF80u) + 4;
    const u32 tbl = load<u32>(0x00F65038u);
    const u32 ra = ia != -1 ? load<u32>(tbl + 24) + (static_cast<u32>(ia) << 7) : 0u;
    const u32 rb = ib != -1 ? load<u32>(tbl + 24) + (static_cast<u32>(ib) << 7) : 0u;
    const u32 pb = static_cast<s32>(load<u8>(rb + 2)) > 0 ? rb + load<u16>(0x0106CF7Eu) : 0u;
    const u32 a = load<u32>(ra + load<u16>(0x0106CF70u) + 4u * load<u8>(pb));
    if (a != 0) {
        ret(ctx, static_cast<u32>(k));
        return;
    }
    const u32 sl = slot() + 4;
    const u32 msg = static_cast<u32>(call(ctx, m1fn::String_Format, {0x00ADDBECu}));
    const u32 rec = 0x00F65100u + 44u * static_cast<u32>(static_cast<s32>(static_cast<s16>(load<u16>(sl))));
    u32 nm = load<u32>(load<u32>(rec + 40));
    if (nm == 0) nm = 0x00B1FBACu;  // "NULL"
    call(ctx, m1fn::String_Format, {0x00ADD2ECu, nm, msg});
    const u32 h = load<u16>(sl + 2) | 0x8000u;
    store<u16>(sl + 2, static_cast<u16>(h));
    store<u32>(clr, 0);
    ret(ctx, h);
}

// bool ScriptCond_Compare390220(const Ctrl* self, Owner* o)
// (callback; no direct callers) Slot as in RPlayDirScript_Check394840
// (s16 self+6 below u16 o+144, record s32 +0 below 10); its value index is
// the s16 at slot + u16 0x0106CF78 + 4 (address 0 based without a slot).
// The value: f32 at block + u16 block+4 + 4 * index, block = o + 152 +
// 132 * u16 o+146 + ((12 * u16 count + 23) & ~15) (count at that record).
// Rows a / b from self+4 / self+2; row b's bytes at +u16 0x0106CF76 (when
// its byte +3 > 0) index words of row a (+u16 0x0106CF70): byte 2 gives
// the operator (0xBC3BED1B: <=, else >=), byte 1 the f32 to compare with.
// Returns the comparison (unordered: 0).
DECOMP(00390220, ScriptCond_Compare390220) {
    const u32 self = arg<u32>(ctx, 0), o = arg<u32>(ctx, 1);
    const s32 k = static_cast<s16>(load<u16>(self + 6));
    u32 slot = 0;
    if (k < static_cast<s32>(load<u16>(o + 144))) {
        slot = o + 152 + static_cast<u32>(k) * 132;
        if (static_cast<s32>(load<u32>(slot)) >= 10) slot = 0;
    }
    const s32 ia = static_cast<s16>(load<u16>(self + 4)), ib = static_cast<s16>(load<u16>(self + 2));
    const u32 vi = slot + load<u16>(0x0106CF78u) + 4;
    const u32 tbl = load<u32>(0x00F65038u);
    auto rowA = [&]() -> u32 { return ia != -1 ? load<u32>(tbl + 24) + (static_cast<u32>(ia) << 7) : 0u; };
    auto selB = [&]() -> u32 {
        const u32 rb = ib != -1 ? load<u32>(tbl + 24) + (static_cast<u32>(ib) << 7) : 0u;
        return static_cast<s32>(load<u8>(rb + 3)) > 0 ? rb + load<u16>(0x0106CF76u) : 0u;
    };
    const u32 ra = rowA();
    const u32 base = load<u16>(0x0106CF70u);
    const bool le = load<u32>(ra + base + 4u * load<u8>(selB() + 2)) == 0xBC3BED1Bu;
    const u32 j = load<u16>(o + 146);
    const s32 idx = static_cast<s16>(load<u16>(vi));
    const u32 rec = 132 * j;
    const u32 blk = o + rec + (((load<u16>(o + 152 + rec) * 12u) + 23u) & 0xFFFFFFF0u) + 152;
    const f64 v = LfsF(load<u32>(blk + load<u16>(blk + 4) + (static_cast<u32>(idx) << 2)));
    const u32 ra2 = rowA();
    const f64 w = LfsF(load<u32>(ra2 + base + 4u * load<u8>(selB() + 1)));
    ctx.fpr[1] = v;  // leftovers: f1 / f2 the compared values
    ctx.fpr[2] = w;
    ret(ctx, (le ? (v < w || v == w) : (v > w || v == w)) ? 1u : 0u);
}

namespace {
void Reg(PPUContext& ctx, u32 o, u32 pd) { call(ctx, Obj_RegisterParam, {o, pd}); }

// Registers the descriptor pd and fills its binding (+16) with copies of
// the names: Binding_Reserve(n), then per name, while the binding's count
// +0 is below +72, a 100-byte Mem_Alloc(100, 0, 16) stored in its array +68
// and String_CopyN(that, name, 100), count + 1.
void RegBind(PPUContext& ctx, u32 o, u32 pd, std::initializer_list<u32> names) {
    Reg(ctx, o, pd);
    call(ctx, Binding_Reserve, {load<u32>(pd + 16), static_cast<u32>(names.size())});
    for (u32 name : names) {
        const u32 bd = load<u32>(pd + 16);
        if (load<u32>(bd) < load<u32>(bd + 72)) {
            const u32 p = static_cast<u32>(call(ctx, m1fn::Mem_Alloc, {100, 0, 16}));
            store<u32>(load<u32>(bd + 68) + (load<u32>(bd) << 2), p);
            call(ctx, m1fn::String_CopyN, {load<u32>(load<u32>(bd + 68) + (load<u32>(bd) << 2)), name, 100});
            store<u32>(bd, load<u32>(bd) + 1);
        }
    }
}

// The two names of the table at t.
void RegBind2(PPUContext& ctx, u32 o, u32 pd, u32 t) { RegBind(ctx, o, pd, {load<u32>(t), load<u32>(t + 4)}); }

// The common end: byte +40 of x = o+8 |= 0x80 with its +44 = k44, o+88 =
// k88. Returns k88, or x for the variants that load x into r3.
void ParamTail(PPUContext& ctx, u32 o, u32 k44, u32 k88, bool retX = false) {
    const u32 x = load<u32>(o + 8);
    const u8 b = static_cast<u8>(load<u8>(x + 40) | 0x80u);
    store<u32>(x + 44, k44);
    store<u8>(x + 40, b);
    store<u32>(o + 88, k88);
    ret(ctx, retX ? x : k88);
}
}  // namespace

// u32 ParamsC_Register43C6EC(Obj* o)
// (registration callback) Descriptors 0x010AB5C0, 0x010AB5D8 (binding: the
// two names at 0x00F657CC), 0x010AB5F0, 0x010AB608 (names 0x00F657EC),
// then 0x010AB620 + 24k (k = 0..7). byte o+8 -> +40 |= 0x80, +44 =
// 0x00AE3730; o+88 = 0x00F38258. Returns o+8.
DECOMP(0043C6EC, ParamsC_Register43C6EC) {
    const u32 o = arg<u32>(ctx, 0);
    Reg(ctx, o, 0x010AB5C0u);
    RegBind2(ctx, o, 0x010AB5D8u, 0x00F657CCu);
    Reg(ctx, o, 0x010AB5F0u);
    RegBind2(ctx, o, 0x010AB608u, 0x00F657ECu);
    for (u32 k = 0; k < 8; ++k) Reg(ctx, o, 0x010AB620u + 24 * k);
    ParamTail(ctx, o, 0x00AE3730u, 0x00F38258u, true);
}

// u32 ParamsD_Register43B450(Obj* o)
// (registration callback) 0x010AADB4, 0x010AADCC (binding: 0x00AE3624,
// 0x00AE3634, 0x00AE3644), 0x010AADE4 + 24k (k = 0..16); tail 0x00AE3654 /
// 0x00F2D790 (returns o+8).
DECOMP(0043B450, ParamsD_Register43B450) {
    const u32 o = arg<u32>(ctx, 0);
    Reg(ctx, o, 0x010AADB4u);
    RegBind(ctx, o, 0x010AADCCu, {0x00AE3624u, 0x00AE3634u, 0x00AE3644u});
    for (u32 k = 0; k < 17; ++k) Reg(ctx, o, 0x010AADE4u + 24 * k);
    ParamTail(ctx, o, 0x00AE3654u, 0x00F2D790u, true);
}

// u32 ParamsE_Register43DAAC(Obj* o)
// (registration callback) 0x010ABD28 (binding: 0x00AE367C, 0x00AE3770,
// 0x00AE3780), 0x010ABD58 + 24k (k = 0..4), 0x010ABD40 (binding: 0x00AE36B0,
// 0x00AE36C0, 0x00AE36D8); tail 0x00AE3788 / 0x00F39B48.
DECOMP(0043DAAC, ParamsE_Register43DAAC) {
    const u32 o = arg<u32>(ctx, 0);
    RegBind(ctx, o, 0x010ABD28u, {0x00AE367Cu, 0x00AE3770u, 0x00AE3780u});
    for (u32 k = 0; k < 5; ++k) Reg(ctx, o, 0x010ABD58u + 24 * k);
    RegBind(ctx, o, 0x010ABD40u, {0x00AE36B0u, 0x00AE36C0u, 0x00AE36D8u});
    ParamTail(ctx, o, 0x00AE3788u, 0x00F39B48u);
}

// u32 ParamsF_Register43B800(Obj* o)
// (registration callback) 0x010AAF7C, 0x010AAF94, 0x010AAFAC (binding:
// 0x00AE3664, 0x00AE366C, 0x00AE367C), 0x010AAFC4 (binding: 0x00AE3684,
// 0x00AE368C, 0x00AE3694, 0x00AE369C), 0x010AAFDC; tail 0x00AE36A4 /
// 0x00F2DF58 (returns o+8).
DECOMP(0043B800, ParamsF_Register43B800) {
    const u32 o = arg<u32>(ctx, 0);
    Reg(ctx, o, 0x010AAF7Cu);
    Reg(ctx, o, 0x010AAF94u);
    RegBind(ctx, o, 0x010AAFACu, {0x00AE3664u, 0x00AE366Cu, 0x00AE367Cu});
    RegBind(ctx, o, 0x010AAFC4u, {0x00AE3684u, 0x00AE368Cu, 0x00AE3694u, 0x00AE369Cu});
    Reg(ctx, o, 0x010AAFDCu);
    ParamTail(ctx, o, 0x00AE36A4u, 0x00F2DF58u, true);
}

// u32 ParamsG_Register43D508(Obj* o)
// (registration callback) 0x010ABBA8, 0x010ABBC0 / 0x010ABBD8 / 0x010ABBF0
// (bindings: the name pairs at 0x00F657CC / 0x00F657DC / 0x00F657EC),
// 0x010ABC08 + 24k (k = 0..11), then three groups 0x010ABAD0 + 72i (+0,
// +24, +48); tail 0x00AE3760 / 0x00F38268.
DECOMP(0043D508, ParamsG_Register43D508) {
    const u32 o = arg<u32>(ctx, 0);
    Reg(ctx, o, 0x010ABBA8u);
    RegBind2(ctx, o, 0x010ABBC0u, 0x00F657CCu);
    RegBind2(ctx, o, 0x010ABBD8u, 0x00F657DCu);
    RegBind2(ctx, o, 0x010ABBF0u, 0x00F657ECu);
    for (u32 k = 0; k < 12; ++k) Reg(ctx, o, 0x010ABC08u + 24 * k);
    for (u32 i = 0; i < 3; ++i)
        for (u32 off : {0u, 24u, 48u}) Reg(ctx, o, 0x010ABAD0u + 72 * i + off);
    ParamTail(ctx, o, 0x00AE3760u, 0x00F38268u);
}
