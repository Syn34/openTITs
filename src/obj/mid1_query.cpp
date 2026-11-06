// Game code, medium sweep part 1: copying a spawn query's parameters out of
// an object, and the constructor of the query with two 76-byte record
// vectors (+144 / +164).
#include "mid1.h"

using namespace sly4;
using namespace sly4::mid1;

namespace m1q {
constexpr u32 Query_Init0CDD28 = 0x000CDD28;  // 22 arguments (see src/world/hevent17_b.cpp); f1, f2
constexpr u32 Vector_Add733964 = 0x00733964;  // (Vector*, const Rec* 192 bytes) -> index
constexpr u32 memmove = 0x0071C3F0;           // (dst, src, n)
constexpr u32 kQueryDefaults = 0x000DFDF0;    // f32 {-1.0, 100.0}
}  // namespace m1q

// u32 Obj_CopyQuery_000CDB28(const Obj* o, Query* d): the query block of o
// copied into d: the 4x3 transform rows o->+32 / +48 / +64 / +80 (x y z) to
// d->+48 / +64 / +80 / +96, o->+96..+104 to d->+112..+120, +112 to +36, +116
// to +128, +120 to +32, +124 / +128 to +132 / +136; flags d->+20 bits
// 0x2000 / 0x1000 / 0x800 = o->+132 bits 31 / 30 / 29 and d->+24 = o->+132
// >> 21; d->+140 = o->+136 when set. Then the 192-byte records of the
// vector o->+16 are appended to the vector d->+0 (emptied first; made on
// first use as a 20-byte vector of 192-byte elements). r3 = the record
// count (0 / d->+140 on the early exits), f1 = o->+128.
DECOMP(000CDB28, Obj_CopyQuery_000CDB28) {
    const u32 o = arg<u32>(ctx, 0), d = arg<u32>(ctx, 1);
    for (u32 row = 0; row < 4; ++row)
        for (u32 k = 0; k < 3; ++k) store<u32>(d + 48 + 16 * row + 4 * k, load<u32>(o + 32 + 16 * row + 4 * k));  // lfs / stfs
    store<u32>(d + 112, load<u32>(o + 96));
    const u32 w100 = load<u32>(o + 100);
    const u32 w112 = load<u32>(o + 112);
    store<u32>(d + 116, w100);
    const u32 w104 = load<u32>(o + 104);
    store<u32>(d + 36, w112);
    store<u32>(d + 120, w104);
    u32 fl = load<u32>(d + 20);
    fl = (fl & ~0x2000u) | ((load<u32>(o + 132) >> 31) & 1u) << 13;
    store<u32>(d + 20, fl);
    store<u32>(d + 128, load<u32>(o + 116));
    store<u32>(d + 32, load<u32>(o + 120));
    fl = (fl & ~0x1000u) | ((load<u32>(o + 132) >> 30) & 1u) << 12;
    store<u32>(d + 20, fl);
    store<u8>(d + 24, static_cast<u8>(load<u32>(o + 132) >> 21));
    store<u32>(d + 132, load<u32>(o + 124));
    const u32 w128 = load<u32>(o + 128);
    store<u32>(d + 136, w128);
    ctx.set_fpr_bits(1, LfsBits(w128));
    u32 dv = load<u32>(d + 0);
    fl = (fl & ~0x800u) | ((load<u32>(o + 132) >> 29) & 1u) << 11;
    store<u32>(d + 20, fl);
    u32 k140 = load<u32>(o + 136);
    u32 src = load<u32>(o + 16);
    if (k140 == 0) k140 = load<u32>(d + 140);
    store<u32>(d + 140, k140);
    if (src == 0) {
        ret(ctx, k140);
        return;
    }
    if (load<u16>(src + 8) == 0) {
        ret(ctx, 0u);
        return;
    }
    if (dv != 0) store<u16>(dv + 8, 0);
    for (u32 j = 0;; ++j) {
        const u32 count = src != 0 ? load<u16>(src + 8) : 0u;
        if (!(j < count)) {
            ret(ctx, count);
            return;
        }
        const u32 rec = load<u32>(src) + 192 * j;
        if (dv == 0) {
            const u32 v = static_cast<u32>(call(ctx, m1fn::SmallAlloc, {20}));
            store<u32>(d + 0, v);
            dv = v;
            if (v != 0) {
                call(ctx, m1fn::PtrVector_Construct, {v});
                store<u16>(v + 6, 192);
                dv = load<u32>(d + 0);
            }
        }
        call(ctx, m1q::Vector_Add733964, {dv, rec});  // dv == 0 when the allocation failed (as the original)
        src = load<u32>(o + 16);
        dv = load<u32>(d + 0);
    }
}

// u32 Obj_CopyQueryEx_000DFBFC(const Obj* o, QueryEx* d): Obj_CopyQuery_000CDB28,
// then d->+184..+196 = o->+220..+232 (+196 a byte) and the 76-byte records
// of o->+180 (count +188) and o->+200 (count +208) appended (74 bytes each)
// to d's vectors +144 and +164. r3 = o->+208's count, f1 = o->+220.
DECOMP(000DFBFC, Obj_CopyQueryEx_000DFBFC) {
    const u32 o = arg<u32>(ctx, 0), d = arg<u32>(ctx, 1);
    call(ctx, 0x000CDB28, {o, d});
    const u32 w224 = load<u32>(o + 224), w220 = load<u32>(o + 220);
    store<u32>(d + 188, w224);
    store<u32>(d + 184, w220);  // lfs / stfs
    ctx.set_fpr_bits(1, LfsBits(w220));
    const u32 w228 = load<u32>(o + 228);
    const u32 b232 = load<u8>(o + 232);
    store<u32>(d + 192, w228);
    store<u8>(d + 196, static_cast<u8>(b232));
    // +144: the count is kept in a register across the memmove (as the
    // original), read again from memory after a grow.
    u32 cnt = load<u16>(d + 152);
    for (u32 i = 0; i < load<u16>(o + 188); ++i) {
        const u32 rec = load<u32>(o + 180) + 76 * i;
        const u32 cap = load<u16>(d + 148);
        if ((cnt & 0xFFFFu) == cap) {
            call(ctx, m1fn::PtrVector_Reserve, {d + 144, (cap << 1) & 0x1FFFEu});
            cnt = load<u16>(d + 152);
        }
        call(ctx, m1q::memmove, {load<u32>(d + 144) + 76 * (cnt & 0xFFFFu), rec, 74});
        const u32 n = load<u16>(d + 152) + 1u;
        const u32 m = load<u16>(d + 154);
        store<u16>(d + 152, static_cast<u16>(n));
        store<u16>(d + 154, static_cast<u16>(static_cast<s32>(m) > static_cast<s32>(n & 0xFFFFu) ? m : n));
        cnt = n;
    }
    for (u32 i = 0;; ++i) {
        const u32 total = load<u16>(o + 208);
        if (!(i < total)) {
            ret(ctx, total);
            return;
        }
        const u32 rec = load<u32>(o + 200) + 76 * i;
        u32 c = load<u16>(d + 172);
        const u32 cap = load<u16>(d + 168);
        if (c == cap) {
            call(ctx, m1fn::PtrVector_Reserve, {d + 164, (cap << 1) & 0x1FFFEu});
            c = load<u16>(d + 172);
        }
        call(ctx, m1q::memmove, {load<u32>(d + 164) + 76 * c, rec, 74});
        const u32 n = load<u16>(d + 172) + 1u;
        const u32 m = load<u16>(d + 174);
        store<u16>(d + 172, static_cast<u16>(n));
        store<u16>(d + 174, static_cast<u16>(static_cast<s32>(m) > static_cast<s32>(n & 0xFFFFu) ? m : n));
    }
}

// u32 QueryEx_Construct_000DFDF8(QueryEx* q, the 21 Query_Init0CDD28
// arguments (r4..r10 and 13 on the stack), Def* def = r7): Query_Init0CDD28
// with the stack arguments re-passed (u8s narrowed, the two it never reads
// left 0) and f1 = f2 = -1.0; q->+4 |= 1; the two 76-byte record vectors
// +144 / +164 (capacity 16); +184 = def ? def->+140 : 100.0, +188 / +192 =
// def ? def->+148 / +152 : 0, +196 = !def || def->+188 has 0x8000; q->+4
// |= 2. Returns q->+4 (r4 = the +196 byte).
DECOMP(000DFDF8, QueryEx_Construct_000DFDF8) {
    const u32 q = arg<u32>(ctx, 0), def = arg<u32>(ctx, 4);
    const u32 sp = static_cast<u32>(ctx.gpr[1]);
    auto sa = [&](u32 k) { return load<u32>(sp + 112 + 8 * k + 4); };
    const f64 dflt = std::bit_cast<f64>(LfsBits(load<u32>(m1q::kQueryDefaults)));
    callf(ctx, m1q::Query_Init0CDD28,
          {q, ctx.gpr[4], ctx.gpr[5], ctx.gpr[6], def, ctx.gpr[8], ctx.gpr[9], ctx.gpr[10], sa(0), sa(1) & 0xFFu, sa(2) & 0xFFu, sa(3) & 0xFFu, sa(4) & 0xFFu,
           sa(5) & 0xFFu, sa(6) & 0xFFu, sa(7) & 0xFFu, sa(8) & 0xFFu, sa(9), sa(10), 0, 0, 0},
          {dflt, dflt});
    store<u32>(q + 4, load<u32>(q + 4) | 1u);
    PtrVecNew(ctx, q + 144, 16, 76, 1);
    PtrVecNew(ctx, q + 164, 16, 76, 1);
    const u32 flags = load<u32>(q + 4);
    const u32 f = def != 0 ? load<u32>(def + 140) : load<u32>(m1q::kQueryDefaults + 4);
    ctx.set_fpr_bits(1, LfsBits(f));
    store<u32>(q + 184, f);  // lfs / stfs
    store<u32>(q + 188, def != 0 ? load<u32>(def + 148) : 0u);
    store<u32>(q + 192, def != 0 ? load<u32>(def + 152) : 0u);
    const u32 b = def != 0 ? (load<u16>(def + 188) >> 15) & 1u : 1u;
    store<u32>(q + 4, flags | 2u);
    store<u8>(q + 196, static_cast<u8>(b));
    ctx.gpr[4] = b;
    ret(ctx, flags | 2u);
}
