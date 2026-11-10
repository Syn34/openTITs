// Game code, large-function sweep part 3: the leaf helpers of the damage
// def reader 000C85A8 (big2_dmg.cpp): DamageDef_Construct and ColVec_Push,
// and the 6-instruction base constructor 0048F27C.
#include "big3.h"

using namespace sly4;
using namespace sly4::big3;

namespace b3d {
constexpr u32 PtrVector_Reserve = 0x00551718;  // (vec, u32 capacity)
constexpr u32 kVtObjBase = 0x00B21A30;         // the base vtable 0048F27C installs
constexpr u32 kFm1 = 0xBF800000u;              // -1.0f
constexpr u32 kRecBytes = 160;                 // a ColRecord
}  // namespace b3d

namespace {

// stfs of a value already in single precision (a NaN keeps its payload).
inline void StoreF(u32 a, f64 v) {
    if (std::isnan(v)) {
        const u64 b = std::bit_cast<u64>(v);
        store<u32>(a, static_cast<u32>((b >> 32) & 0x80000000u) | 0x7F800000u | static_cast<u32>((b >> 29) & 0x7FFFFFu));
        return;
    }
    store<u32>(a, std::bit_cast<u32>(static_cast<f32>(v)));
}
inline void Copy32(u32 dst, u32 src) { store<u32>(dst, load<u32>(src)); }

}  // namespace

// void ObjBase_Construct(Obj* o): o->+4 = 0 (u8), then the base vtable.
DECOMP(0048F27C, ObjBase_Construct) {
    const u32 o = arg<u32>(ctx, 0);
    store<u8>(o + 4, 0);
    store<u32>(o + 0, b3d::kVtObjBase);
}

// s32 ColVec_Push(PtrVector* v of 160-byte ColRecords, const ColRecord* r):
// grows the vector to twice its capacity when full, copies r's fields
// (+0..+8, +16..+24, the words +32 / +36, +48..+56, +64..+88, +96..+120,
// the four flag bits 31..28 of +124 (stored once per bit, as the
// original), +128..+136 and +144) into the new element, bumps the count
// (+8) and the high-water mark (+10) and returns the new element's index,
// computed from the 16-bit count. f1 is left with r->+136.
DECOMP(00745564, ColVec_Push) {
    const u32 v = arg<u32>(ctx, 0), r = arg<u32>(ctx, 1);
    if (load<u16>(v + 8) == load<u16>(v + 4))
        call(ctx, b3d::PtrVector_Reserve, {v, (static_cast<u32>(load<u16>(v + 4)) << 1) & 0x1FFFEu});
    const u32 n = load<u16>(v + 8);
    const u32 e = load<u32>(v + 0) + b3d::kRecBytes * n;
    for (const u32 o : {0u, 4u, 8u, 16u, 20u, 24u, 32u, 36u, 48u, 52u, 56u, 64u, 68u, 72u, 76u, 80u, 84u, 88u, 96u, 100u,
                        104u, 108u, 112u, 116u, 120u})
        Copy32(e + o, r + o);
    u32 w = load<u32>(e + 124) & 0x7FFFFFFFu;
    for (const u32 bit : {0x80000000u, 0x40000000u, 0x20000000u, 0x10000000u}) {
        w = (w & ~bit) | (load<u32>(r + 124) & bit);
        store<u32>(e + 124, w);
    }
    for (const u32 o : {128u, 132u, 136u, 144u}) Copy32(e + o, r + o);
    ctx.set_fpr_bits(1, LfsBits(load<u32>(r + 136)));
    const u32 cnt = n + 1;  // (not masked: sth keeps the low half)
    store<u16>(v + 8, static_cast<u16>(cnt));
    const s32 hw = load<u16>(v + 10);
    store<u16>(v + 10, static_cast<u16>(hw > static_cast<s32>(cnt & 0xFFFF) ? static_cast<u32>(hw) : cnt));
    ret(ctx, (cnt & 0xFFFF) - 1);
}

// void DamageDef_Construct(DamageDef* d, u32 a1, const char* a2, const char*
// a3, (r7, r8 unread), u32 a6, const f32 vec[3], f1..f13, then stack
// arguments: slots 11..14 f32s, 15..19 and 23..25 bools (their low bits),
// 20 a 6-byte array or null (slots 21 / 22 unread). Fills d: +0 = 2 then
// |= 4, +12 / +16 / +20 = a1..a3, bytes +24..+31 / +44 cleared, -1.0 at
// +36 / +40 / +72, +76 = f1, +88 + 4i = (f32)(((+120 >> 4) & 15) - i) for
// i < 5, the u16 +120 = (old & 15) | 0x8200, +136 = f2, +140 = a6, +144 =
// vec, +160..+200 = f3..f13, +204..+216 = slots 11..14, +220 bits 31..24
// from the bools (bits 23 / 22 cleared, the rest kept), +128..+133 from
// the array (1 when null), and zeros elsewhere. Every stack argument is
// read before the first store, as the original. f1 is left with slot 14.
DECOMP(00745D88, DamageDef_Construct) {
    const u32 d = arg<u32>(ctx, 0);
    const u32 a1 = arg<u32>(ctx, 1), a2 = arg<u32>(ctx, 2), a3 = arg<u32>(ctx, 3);
    const u32 a6 = arg<u32>(ctx, 6), vec = arg<u32>(ctx, 7);
    const u32 sp = static_cast<u32>(ctx.gpr[1]);
    const auto slot = [&](u32 n) { return load<u32>(sp + 112 + 8 * n + 4); };
    f64 fa[14];
    for (int k = 1; k <= 13; ++k) fa[k] = ctx.fpr[k];
    u32 fs[4];
    for (u32 k = 0; k < 4; ++k) fs[k] = slot(11 + k);
    u32 bools[8];
    {
        const u32 idx[8] = {15, 16, 17, 18, 19, 23, 24, 25};
        for (u32 k = 0; k < 8; ++k) bools[k] = slot(idx[k]);
    }
    const u32 bytes = slot(20);
    const u16 flags = static_cast<u16>((load<u16>(d + 120) & 0xFu) | 0x8200u);

    store<u32>(d + 12, a1);
    store<u32>(d + 16, a2);
    store<u32>(d + 20, a3);
    for (u32 k = 24; k < 32; ++k) store<u8>(d + k, 0);
    store<u32>(d + 32, 0);
    store<u32>(d + 36, b3d::kFm1);
    store<u32>(d + 40, b3d::kFm1);
    store<u8>(d + 44, 0);
    for (u32 k = 48; k <= 68; k += 4) store<u32>(d + k, 0);
    store<u32>(d + 72, b3d::kFm1);
    store<u32>(d + 4, 0);
    store<u32>(d + 8, 0);
    StoreF(d + 76, fa[1]);
    for (const u32 o : {80u, 84u, 108u, 112u, 116u}) store<u32>(d + o, 0);
    store<u16>(d + 120, flags);
    store<u16>(d + 122, 0);
    store<u32>(d + 0, 2);
    for (u32 i = 0; i < 5; ++i) {
        const s32 x = static_cast<s32>((load<u16>(d + 120) >> 4) & 0xFu) - static_cast<s32>(i);
        store<u32>(d + 88 + 4 * i, std::bit_cast<u32>(static_cast<f32>(static_cast<f64>(x))));
    }
    u32 w = load<u32>(d + 220) & 0x7FFFFFFFu;
    StoreF(d + 136, fa[2]);
    const u32 head = load<u32>(d + 0);
    w = (w & ~0x80000000u) | ((bools[0] & 1u) << 31);
    Copy32(d + 144, vec + 0);
    store<u32>(d + 124, 0);
    w = (w & ~0x40000000u) | ((bools[1] & 1u) << 30);
    Copy32(d + 148, vec + 4);
    store<u32>(d + 140, a6);
    w = (w & ~0x20000000u) | ((bools[2] & 1u) << 29);
    StoreF(d + 160, fa[3]);
    w = (w & ~0x10000000u) | ((bools[3] & 1u) << 28);
    Copy32(d + 152, vec + 8);
    for (int k = 4; k <= 13; ++k) StoreF(d + 164 + 4 * static_cast<u32>(k - 4), fa[k]);
    for (u32 k = 4; k < 8; ++k) w = (w & ~(0x80000000u >> k)) | ((bools[k] & 1u) << (31 - k));
    w &= ~0x00C00000u;
    for (u32 k = 0; k < 4; ++k) store<u32>(d + 204 + 4 * k, fs[k]);
    store<u32>(d + 0, head | 4u);
    store<u32>(d + 220, w);
    for (u32 i = 0; i < 6; ++i) store<u8>(d + 128 + i, bytes == 0 ? 1u : load<u8>(bytes + i));
    // (f0 / f1 / f2 as the original leaves them: slot 13, slot 14, vec[2])
    ctx.set_fpr_bits(0, LfsBits(fs[2]));
    ctx.set_fpr_bits(1, LfsBits(fs[3]));
    ctx.set_fpr_bits(2, LfsBits(load<u32>(vec + 8)));
}
