// Game code, medium sweep part 6: a QueryEx constructor variant and the
// sized-block allocator used by 0038DB04.
#include "mid6.h"

using namespace sly4;
using namespace sly4::mid6;

namespace {

namespace qfn6 {
constexpr u32 Query_Init0CDD28 = 0x000CDD28;   // 22 arguments (see src/world/hevent17_b.cpp); f1, f2
constexpr u32 PtrVector_Construct = 0x005516E0;  // (v)
constexpr u32 PtrVector_Init = 0x00551598;       // (v, n, 0, 1)
constexpr u32 Mem_Alloc = 0x0054ED38;            // (size, pool, align)
constexpr u32 String_HashNoCase = 0x0056A710;    // (s) -> u32
}  // namespace qfn6

constexpr u32 kQueryDefaults0E4 = 0x000E40D4;  // f32 {the init's f1 / f2, +184 default} just before 000E40E0
constexpr u32 kQueryName0E4 = 0x00AC6790;      // the init's 7th argument (a name / string)

// A 76-byte-record PtrVector: construct, +6 = 76, Init(v, 16, 0, 1).
void RecVecNew(PPUContext& ctx, u32 v) {
    call(ctx, qfn6::PtrVector_Construct, {v});
    store<u16>(v + 6, 76);
    call(ctx, qfn6::PtrVector_Init, {v, 16, 0, 1});
}

}  // namespace

// u32 QueryEx_Construct_000E40E0(QueryEx* q, a1, Def* def, a3, a4, a5,
// a6, a7, stack s9..s12): Query_Init0CDD28(q, 4, 128, a1, def, a3, name,
// a4, a5, def->+300 bits 31 / 30, 0, (u8)a6, (u8)a7, 0, 0, (u8)s9, s10,
// -1, -, -, 0; f1 = f2 = constant), q->+4 |= 1, the record vectors +144 /
// +164; +184 = def ? def->+140 : constant, +188 / +192 = def->+148 / +152
// (0 without), +196 = !def || def->+188 has 0x8000, +200 / +204 = s11 /
// s12, +4 |= 6. Quirk: def->+300 / +148 / +152 are read even when def is
// 0. Returns +4 (r4 = the +196 byte).
DECOMP(000E40E0, QueryEx_Construct_000E40E0) {
    const u32 q = arg<u32>(ctx, 0), def = arg<u32>(ctx, 2);
    const u64 a1 = ctx.gpr[4], a3 = ctx.gpr[6], a4 = ctx.gpr[7], a5 = ctx.gpr[8];
    const u32 a6 = arg<u32>(ctx, 6), a7 = arg<u32>(ctx, 7);
    const u32 sp = static_cast<u32>(ctx.gpr[1]);
    const auto sa = [&](u32 k) { return load<u32>(sp + 112 + 8 * k + 4); };
    const u32 s9 = sa(0), s10 = sa(1), s11 = sa(2), s12 = sa(3);
    const u32 w = load<u32>(def + 300);
    const f64 f = std::bit_cast<f64>(LfsBits(load<u32>(kQueryDefaults0E4)));
    callf(ctx, qfn6::Query_Init0CDD28,
          {q, 4, 128, a1, def, a3, kQueryName0E4, a4, a5, (w >> 31) & 1u, (w >> 30) & 1u, 0, a6 & 0xFFu, a7 & 0xFFu, 0, 0, s9 & 0xFFu, s10,
           0xFFFFFFFFull, 0, 0, 0},
          {f, f});
    store<u32>(q + 4, load<u32>(q + 4) | 1u);
    RecVecNew(ctx, q + 144);
    RecVecNew(ctx, q + 164);
    const u32 d148 = load<u32>(def + 148), d152 = load<u32>(def + 152);
    const u32 flags = load<u32>(q + 4);
    const u32 r = def != 0 ? load<u32>(def + 140) : load<u32>(kQueryDefaults0E4 + 4);
    ctx.fpr[1] = std::bit_cast<f64>(LfsBits(r));
    store<u32>(q + 184, r);
    store<u32>(q + 188, def != 0 ? d148 : 0);
    store<u32>(q + 192, def != 0 ? d152 : 0);
    const u32 b = def != 0 ? (load<u16>(def + 188) >> 15) & 1u : 1u;
    store<u32>(q + 200, s11);
    store<u32>(q + 204, s12);
    store<u32>(q + 4, flags | 6);
    store<u8>(q + 196, static_cast<u8>(b != 0));
    ctx.gpr[3] = flags | 6;
    ctx.gpr[4] = b != 0;
}

// u32 Block_Alloc_0038D91C(Owner* o, const char* name, u16 a, u16 b, u32 c):
// one Mem_Alloc block (+0 hash, +4.. flags, +8 size, +146 a, 152.. the
// tables) of the size the three counts give; o->+4 = it, o->+0 = its
// +152; the hash of name, two four-u16 record headers {count, 8, stride
// + 8, ...} after the a 132-byte records. Quirk: the header writes go
// through a null block when the allocation fails. Returns o->+0.
DECOMP(0038D91C, Block_Alloc_0038D91C) {
    const u32 o = arg<u32>(ctx, 0), name = arg<u32>(ctx, 1);
    const u32 a = arg<u32>(ctx, 2) & 0xFFFF, b = arg<u32>(ctx, 3) & 0xFFFF;
    const u32 n6 = arg<u32>(ctx, 3), n7 = arg<u32>(ctx, 4);
    const u32 size = (132 * a + ((12 * b + 23) & ~15u) + ((((n7 << 3) & 0x7FFF8u) + 21) & ~15u) + 167) & ~15u;
    u32 p = static_cast<u32>(call(ctx, qfn6::Mem_Alloc, {size, 0, 16}));
    store<u32>(o + 4, p);
    if (p != 0) {
        const u32 f6 = load<u16>(p + 6);
        store<u32>(p, 0);
        store<u16>(p + 144, 0);
        store<u8>(p + 4, 0);
        store<u8>(p + 5, 0);
        store<u8>(p + 6, 0);
        store<u32>(p + 12, 0);
        store<u32>(p + 148, 0);
        store<u32>(p + 8, 0);
        store<u16>(p + 6, static_cast<u16>(f6 & 0x000F));
        p = load<u32>(o + 4);
    }
    store<u32>(o, p + 152);
    const u32 f6 = load<u16>(p + 6);
    store<u16>(p + 146, static_cast<u16>(a));
    store<u16>(p + 6, static_cast<u16>(f6 | 0x80));
    const u32 h = static_cast<u32>(call(ctx, qfn6::String_HashNoCase, {name}));
    u32 r = p + 152 + 132 * load<u16>(p + 146);
    store<u16>(r, static_cast<u16>(n6));
    store<u16>(r + 2, 8);
    store<u16>(r + 4, static_cast<u16>((n6 << 2) + 8));
    store<u16>(r + 6, static_cast<u16>((n6 << 3) + 8));
    const u32 recs = 132 * load<u16>(p + 146);
    store<u32>(p, h);
    store<u32>(p + 8, size);
    const u32 ret0 = load<u32>(o);
    const u32 cnt = load<u16>(p + 152 + recs);
    r = p + 152 + recs + ((12 * cnt + 23) & ~15u);
    store<u16>(r, static_cast<u16>(n7));
    store<u16>(r + 2, 8);
    store<u16>(r + 4, static_cast<u16>(((n7 << 2) & 0x3FFFCu) + 8));
    ctx.gpr[3] = ret0;
}
