// Game code, medium sweep part 1: smaller container helpers.
#include "mid1.h"

using namespace sly4;
using namespace sly4::mid1;

// s32 Vector128_Push_0075EBFC(Vector* v, const Rec* r): PushBack of a
// 128-byte record (v: +0 data, +4 u16 capacity, +8 u16 count, +10 mark;
// grown to twice the capacity when full), copied field by field: bytes
// 0..3, words +4 / +8, +16..+24, +32..+40, the 76 bytes +44..+119 as words,
// the top 11 flag bits of +120 one by one (the rest of the slot's +120
// kept), +124. Returns the new element's index; f1 = r->+124.
DECOMP(0075EBFC, Vector128_Push_0075EBFC) {
    const u32 v = arg<u32>(ctx, 0), r = arg<u32>(ctx, 1);
    u32 c = load<u16>(v + 8);
    const u32 cap = load<u16>(v + 4);
    if (c == cap) {
        call(ctx, m1fn::PtrVector_Reserve, {v, (cap << 1) & 0x1FFFEu});
        c = load<u16>(v + 8);
    }
    const u32 e = load<u32>(v) + ((c << 7) & 0x007FFF80u);
    for (u32 k = 0; k < 4; ++k) store<u8>(e + k, load<u8>(r + k));
    for (const u32 off : {4u, 8u, 16u, 20u, 24u, 32u, 36u, 40u}) store<u32>(e + off, load<u32>(r + off));  // +16..+24 lfs / stfs
    for (u32 off = 44; off < 120; off += 4) store<u32>(e + off, load<u32>(r + off));
    u32 fl = (load<u32>(e + 120) & 0x7FFFFFFFu) | (load<u32>(r + 120) & 0x80000000u);
    store<u32>(e + 120, fl);
    for (u32 bit = 1; bit <= 10; ++bit) {
        const u32 m = 0x80000000u >> bit;
        fl = (fl & ~m) | (load<u32>(r + 120) & m);
        store<u32>(e + 120, fl);
    }
    const u32 w124 = load<u32>(r + 124);
    store<u32>(e + 124, w124);  // lfs / stfs
    ctx.set_fpr_bits(1, LfsBits(w124));
    const u32 n = load<u16>(v + 8) + 1u;
    const u32 mark = load<u16>(v + 10);
    store<u16>(v + 8, static_cast<u16>(n));
    store<u16>(v + 10, static_cast<u16>(static_cast<s32>(mark) > static_cast<s32>(n & 0xFFFFu) ? mark : n));
    ret(ctx, static_cast<s32>(n & 0xFFFFu) - 1);
}
