// Game code, medium sweep part 12: helpers shared by mid12_*.cpp (still
// recompiled game functions of 120-400 instructions: the damage / collision
// def constructors from their defaults records, readers and other game code).
#pragma once

#include <bit>
#include <cmath>
#include <initializer_list>
#include <utility>

#include "mid11.h"

namespace sly4 {
namespace mid12 {

using namespace opentit;
using namespace opentit::decomp;
using mid10::LoadF;
using mid10::PutBit;
using mid10::SArg;
using mid10::StoreF;
using mid6::F32;
using mid6::W32;
using mid9::CopyW;
namespace m1fn = mid1::m1fn;
namespace m11fn = mid11::m11fn;
namespace xfn = mid5::xfn;

namespace m12fn {
constexpr u32 Memmove = 0x0071C3F0;      // (dst, src, n)
constexpr u32 String_Alloc = 0x0055578C;  // (n) -> char* (reads r3 only)
constexpr u32 Mem_FreeIfSet = 0x00555CC0;  // (p) (reads r3 only)
}  // namespace m12fn

constexpr u32 kDmgVt = 0x00B15970;  // the damage def vtable the constructors store first

// f1 as an lfs of the word at ea leaves it (bit for bit, NaNs not quieted).
inline void SetF1(PPUContext& ctx, u32 ea) { ctx.fpr[1] = std::bit_cast<f64>(mid1::LfsBits(load<u32>(ea))); }

// One ordered field copy dst + d <- src + s of n bytes (an lwz / stw, lfs /
// stfs, lhz / sth or lbz / stb pair).
struct Cp {
    u8 n;
    u16 d, s;
};
inline void CopyOne(u32 dst, u32 src, const Cp& c) {
    switch (c.n) {
        case 1: store<u8>(dst + c.d, load<u8>(src + c.s)); break;
        case 2: store<u16>(dst + c.d, load<u16>(src + c.s)); break;
        default: store<u32>(dst + c.d, load<u32>(src + c.s)); break;
    }
}
inline void CopySeq(u32 dst, u32 src, std::initializer_list<Cp> rows) {
    for (const Cp& c : rows) CopyOne(dst, src, c);
}

// The damage def constructors' copy of the record's 0x80000 group
// (DamageColDef_ReadAD1F8): f32 +368..+376 = rec +288..+296, +384 / +388 =
// rec +304 / +308, f32 +392 = rec +312, bit 0x80 of byte +396 from rec
// +316; the vtable kDmgVt stored before copy number vtAt (0..7); then the
// seven {+304 hash, +308 f32} pairs from rec +224 / +228.
inline void DmgAdCopy(u32 p, u32 rec, u32 vtAt) {
    static constexpr Cp kRows[6] = {{4, 368, 288}, {4, 372, 292}, {4, 376, 296}, {4, 384, 304}, {4, 388, 308}, {4, 392, 312}};
    for (u32 k = 0; k < 7; ++k) {
        if (k == vtAt) store<u32>(p, kDmgVt);
        if (k < 6) CopyOne(p, rec, kRows[k]);
        else store<u8>(p + 396, static_cast<u8>((load<u8>(p + 396) & 0x7Fu) | (load<u8>(rec + 316) & 0x80u)));
    }
    for (u32 k = 0; k < 56; k += 8) {
        CopyW(p + 304 + k, rec + 224 + k);
        CopyW(p + 308 + k, rec + 228 + k);
    }
}

// PushBack of a 20-byte element (five words copied from src, in order)
// onto the PtrVector v {+0 data, +4 u16 capacity, +8 u16 count, +10 u16
// high-water mark}: grown to twice the capacity when full.
inline void VecPush20(PPUContext& ctx, u32 v, u32 src) {
    u32 c = load<u16>(v + 8);
    const u32 cap = load<u16>(v + 4);
    if (c == cap) {
        call(ctx, m1fn::PtrVector_Reserve, {v, (cap << 1) & 0x1FFFEu});
        c = load<u16>(v + 8);
    }
    const u32 e = load<u32>(v) + c * 20;
    for (u32 k = 0; k < 20; k += 4) CopyW(e + k, src + k);
    const u32 n = (load<u16>(v + 8) + 1u) & 0xFFFFu;
    const u32 m = load<u16>(v + 10);
    store<u16>(v + 8, static_cast<u16>(n));
    store<u16>(v + 10, static_cast<u16>(static_cast<s32>(m) > static_cast<s32>(n) ? m : n));
}

// The tag of an XML child node: its hash +20 when bit 0x80 of byte +24 is
// set, else 0.
inline u32 NodeTag(u32 n) { return (load<u8>(n + 24) & 0x80u) ? load<u32>(n + 20) : 0u; }

// (f32)(a * b), an fmuls.
inline f64 MulS(f64 a, f64 b) { return static_cast<f64>(static_cast<f32>(a * b)); }

// The inlined euler (radians, already scaled) to quaternion: half angles
// h = e * half, Math_Cosf of y, z, x then Math_Sinf of y, z, x; q[0..3]
// stored in the order +12, +0, +8, +4; f1 = q[1] as left.
inline void EulerQuatAt(PPUContext& ctx, u32 q, f64 ex, f64 ey, f64 ez, f64 half) {
    const f64 hy = MulS(ey, half), hz = MulS(ez, half), hx = MulS(ex, half);
    const f64 cy = callf(ctx, m11fn::Math_Cosf, {}, {hy});
    const f64 cz = callf(ctx, m11fn::Math_Cosf, {}, {hz});
    const f64 cx = callf(ctx, m11fn::Math_Cosf, {}, {hx});
    const f64 sy = callf(ctx, m11fn::Math_Sinf, {}, {hy});
    const f64 sz = callf(ctx, m11fn::Math_Sinf, {}, {hz});
    const f64 sx = callf(ctx, m11fn::Math_Sinf, {}, {hx});
    const f64 a = MulS(sy, sz), b = MulS(cy, cz), d = MulS(cz, sy), e = MulS(cy, sz);
    const f64 a1 = MulS(a, sx), b1 = MulS(b, sx), d1 = MulS(d, sx), e1 = MulS(e, sx);
    const f64 qw = static_cast<f32>(std::fma(b, cx, -a1));
    const f64 qx = static_cast<f32>(std::fma(a, cx, b1));
    const f64 qz = static_cast<f32>(std::fma(e, cx, -d1));
    const f64 qy = static_cast<f32>(std::fma(d, cx, e1));
    StoreF(q + 12, qw);
    StoreF(q, qx);
    StoreF(q + 8, qz);
    StoreF(q + 4, qy);
    ctx.fpr[1] = qy;
}

}  // namespace mid12
}  // namespace sly4
