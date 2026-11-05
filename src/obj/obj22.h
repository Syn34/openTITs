// Game code, part 22: helpers shared by obj22*.cpp (the continued sweep of
// small still-recompiled functions, the condition-node init and the hit
// event queue).
#pragma once

#include <bit>
#include <cmath>

#include "obj19.h"
#include "obj21.h"

namespace sly4 {
namespace obj22 {

using namespace opentit;
using namespace opentit::decomp;
using namespace sly4::obj21;

namespace o22fn {
constexpr u32 Mem_Alloc = 0x0054ED38;         // (size, pool, align)
constexpr u32 String_Format = 0x005534D0;     // (fmt, ...)
constexpr u32 String_CopyN = 0x00553638;      // (dst, src, n)
constexpr u32 String_HashNoCase = 0x0056A710; // (s) -> u32
constexpr u32 Node_Bind = 0x004D3088;         // (owner, name, k5, k6, -1, 0) -> binding
constexpr u32 NodeOwner_Init = 0x004D2BFC;    // (p, name, 0, 0): the 52-byte binding owner (name?)
constexpr u32 Binding_Reserve = 0x004D2BB8;   // (binding, n): the binding's name list (+0 count, +68 array, +72 capacity) (name?)
}  // namespace o22fn

// The knowledge-data block table at *0x00F65038: +24 the 128-byte records,
// +28 the count of blocks in use (at most 0x1800), +32 the data cursor
// (128 bytes per block), +36 the 16-byte block entries {cursor, owner,
// type, hash}.
constexpr u32 kKDataTable = 0x00F65038;
constexpr u32 kKDataMax = 0x1800;

// The hit event queue: 512 128-byte records at 0x01022100 (an event copy
// at +0..+100, +112 the kind, +116 the object), the count at 0x01022080;
// the word at 0x01032148 (just past the records) gates it.
constexpr u32 kHitQueue = 0x01022100, kHitQueueCount = 0x01022080, kHitQueueMax = 512;
constexpr u32 kHitQueueGate = 0x01032148;

// lfs / stfs pair: the raw word at s + off copied to d + off; returns it.
inline u32 CopyW(u32 d, u32 s, u32 off) {
    const u32 w = load<u32>(s + off);
    store<u32>(d + off, w);
    return w;
}
inline f64 F32Bits(u32 w) { return static_cast<f64>(std::bit_cast<f32>(w)); }

}  // namespace obj22
}  // namespace sly4
