// World_HandleEvent callees, part 10e: the volume item batch submit and
// the attach record append.
#include <cmath>

#include "hevent10.h"

using namespace sly4;
using namespace sly4::hevent;
using namespace sly4::hevent7;
using namespace sly4::hevent8;
using namespace sly4::hevent10;

namespace sly4 {
namespace hevent10e {
namespace efn10 {
constexpr u32 Item_Submit399AB0 = 0x00399AB0;  // (a*, b*, from*, const vec3* dir; f32 len, f32 r) (name?)
}  // namespace efn10

// Item batches: up to 256 Item* copied to 0x01084E30 and the 336-byte items
// to 0x0106FE30, processed there and copied back. Item: +48 vec3 from,
// +144 vec3 to, +192 / +256 blocks, +292 f32.
constexpr u32 kPtrScratch = 0x01084E30, kItemScratch = 0x0106FE30;
constexpr u32 kBatch = 256, kItemSize = 336;
constexpr u32 kItemFrom = 48, kItemTo = 144, kItemA = 192, kItemB = 256, kItemR = 292;
constexpr u32 kMinLen = 0x0039C8E4, kOne = 0x0039C8E8;  // f32 constants in the code segment

// Attach record vector (as Obj_Attach0D15C0): +0 data, +4 u16 capacity, +8
// u16 count, +10 u16 high-water mark; 240-byte records.
constexpr u32 kVecData = 0, kVecCap = 4, kVecCount = 8, kVecHigh = 10;
constexpr u32 kRecSize = 240, kRecFlags = 176;
}  // namespace hevent10e
}  // namespace sly4

using namespace sly4::hevent10e;

// void Items_Submit39C8F0({Item** data, u32 count}* list) (name?): in
// batches of 256, snapshots the items into scratch space, calls
// 0x00399AB0(item +192, item +256, &from, &dir; |to - from| or its square,
// item +292) on each copy (dir normalised when the squared length is at
// least the threshold), and copies each item back to its pointer rounded up
// to 16. r3 as the original leaves it (the count).
DECOMP(0039C8F0, Items_Submit39C8F0) {
    const u32 list = arg<u32>(ctx, 0);
    Stack frame(ctx, 16);
    const u32 dir = frame.addr();
    u32 done = 0;
    u32 n;
    for (;;) {
        n = load<u32>(list + 4);
        const u32 data = load<u32>(list + 0);
        if (!(n > done)) break;
        // The last batch is count & 0xFF (done is a multiple of 256).
        const u32 batch = done + kBatch > n ? (n & 0xFF) : kBatch;
        call(ctx, h8fn::Memmove, {kPtrScratch, data + done * 4, (batch * 4 + 15) & ~15u});
        for (u32 k = 0; k < batch; ++k)
            call(ctx, h8fn::Memmove, {kItemScratch + k * kItemSize, load<u32>(kPtrScratch + k * 4), kItemSize});
        if (batch == 0) break;
        const f64 min_len = lfs_f64(kMinLen), one = lfs_f64(kOne);
        for (u32 k = 0; k < batch; ++k) {
            const u32 it = kItemScratch + k * kItemSize;
            const u32 a = it + kItemFrom, b = it + kItemTo;
            f32 dy = static_cast<f32>(lfs_f64(b + 4) - lfs_f64(a + 4));
            f32 dx = static_cast<f32>(lfs_f64(b) - lfs_f64(a));
            f32 dz = static_cast<f32>(lfs_f64(b + 8) - lfs_f64(a + 8));
            f32 len = fmuls(dy, dy);
            store<f32>(dir + 4, dy);
            store<f32>(dir + 0, dx);
            store<f32>(dir + 8, dz);
            len = static_cast<f32>(std::fma(static_cast<f64>(dx), dx, len));
            len = static_cast<f32>(std::fma(static_cast<f64>(dz), dz, len));
            if (len >= min_len) {
                len = static_cast<f32>(std::sqrt(static_cast<f64>(len)));
                const f32 s = static_cast<f32>(one / len);
                dx = fmuls(dx, s);
                dy = fmuls(dy, s);
                dz = fmuls(dz, s);
                store<f32>(dir + 0, dx);
                store<f32>(dir + 4, dy);
                store<f32>(dir + 8, dz);
            }
            callf(ctx, efn10::Item_Submit399AB0, {it + kItemA, it + kItemB, a, dir}, {len, lfs_f64(it + kItemR)});
            const u32 p = load<u32>(load<u32>(list + 0) + (done + k) * 4);
            // The original traps (tw) unless the rounded pointer keeps p's
            // upper bits, which it always does.
            call(ctx, h8fn::Memmove, {(p + 15) & ~15u, it, kItemSize});
        }
        done += batch;
    }
    ctx.gpr[3] = n;
}

// u32 Attach_Add7450E4(Vector* v, const AttachRec* src) (name?): appends a
// copy of src (doubling the capacity when full): bytes 0..127, words
// 128..136, 144..172, flag bits 31..24 of +176 one at a time (its low 24
// bits kept from the slot), +180, 192..224 (140, 184, 188 and 228..239 are
// not copied). Bumps the count and the high-water mark; returns the new
// record's index.
DECOMP(007450E4, Attach_Add7450E4) {
    const u32 v = arg<u32>(ctx, 0);
    const u32 s = arg<u32>(ctx, 1);
    if (load<u16>(v + kVecCount) == load<u16>(v + kVecCap))
        call(ctx, h8fn::PtrVector_Reserve, {v, (static_cast<u32>(load<u16>(v + kVecCap)) << 1) & 0x1FFFEu});
    const u32 r = load<u32>(v + kVecData) + static_cast<u32>(load<u16>(v + kVecCount)) * kRecSize;
    for (u32 i = 0; i < 128; ++i) store<u8>(r + i, load<u8>(s + i));
    for (const u32 off : {128u, 132u, 136u, 144u, 148u, 152u, 156u, 160u, 164u, 168u})
        store<u32>(r + off, load<u32>(s + off));
    u32 w = load<u32>(r + kRecFlags) & 0x7FFFFFFFu;
    store<u32>(r + 172, load<u32>(s + 172));
    for (u32 bit = 0; bit < 8; ++bit) {
        const u32 m = 0x80000000u >> bit;
        w = (w & ~m) | (load<u32>(s + kRecFlags) & m);
        store<u32>(r + kRecFlags, w);
    }
    store<u32>(r + 180, load<u32>(s + 180));
    for (u32 off = 192; off <= 224; off += 4) store<u32>(r + off, load<u32>(s + off));
    ctx.fpr[1] = lfs_f64(s + 224);
    const u32 n = static_cast<u32>(load<u16>(v + kVecCount)) + 1;
    const u32 high = load<u16>(v + kVecHigh);
    store<u16>(v + kVecCount, static_cast<u16>(n));
    store<u16>(v + kVecHigh, static_cast<u16>(static_cast<s32>(high) > static_cast<s32>(n & 0xFFFF) ? high : n));
    ret(ctx, (n & 0xFFFF) - 1);
}
