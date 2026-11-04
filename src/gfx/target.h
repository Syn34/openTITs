// Render targets: the 60-byte target/buffer object, its allocation in RSX
// memory, copies of it at another size, and binding one to a 52-byte
// surface record.
//
// Target (60 bytes, made by Mem_Alloc(60, 0, 16) and Target_Construct):
//   +0  u32 address of the pixels
//   +4  u32 RSX offset << 1 | location (1 = main memory, 0 = local)
//   +8  u32 size in bytes << 4 | flags (bits 1..3 copied, bit 0 set on copies)
//   +12 u32 top nibble: kind (4 = allocated by Target_Init)
//   +16 u8  1
//   +20 u32 AA mode (word 0 of the AA table entry), +24 s32 format,
//   +28 u32 word 20 of the format table entry, +32 u32 pitch,
//   +36 u16 width, +38 u16 height, +40 u16 0, +42 u16 rows skipped (copies),
//   +44 u8  bits per pixel, +45 s8 tile slot or -1, +46 u8 0, +47 u8 -1,
//   +48 u32 0, +52 s32 compression tag base or -1, +56 u8 1 on copies.
#pragma once

#include "../sly4.h"

namespace sly4::tgt {

namespace imp {
constexpr u32 cellGcmAddressToOffset = 0x00ABD5E4;
}  // namespace imp

namespace fn {
constexpr u32 Surface_Create = 0x005304D4;     // decompiled: src/gfx/target.cpp
constexpr u32 Target_Init = 0x00530DB4;        // decompiled: src/gfx/target.cpp
constexpr u32 Target_Attach = 0x00532E70;      // decompiled: src/gfx/target.cpp
constexpr u32 Surface_SetBuffer = 0x00532D5C;  // decompiled: src/gfx/target.cpp
// Still recompiled.
constexpr u32 Target_Construct = 0x0053044C;  // (Target*) leaf: the default 60-byte target (name?)
constexpr u32 Target_BindTile = 0x0052F32C;   // (Target** slot, u32 flag) cellGcmSetTileInfo + cellGcmBindTile (name?)
constexpr u32 Surface_GetSize = 0x00530318;   // decompiled: src/gfx/init_surface.cpp
constexpr u32 Mem_AllocPool = 0x0056C514;     // (align, size, pool, flags)
}  // namespace fn

namespace g {
constexpr u32 format_table = 0x00F67254;  // 24-byte entries: +4 u32 (low byte: surface type), +8 bits per pixel,
                                          // +16 u32, +20 u32
constexpr u32 aa_table = 0x00F6741C;      // 28-byte entries: +0 mode, +8 width factor, +12 height factor, +20 compressed
constexpr u32 tile_slots = 0x00F67244;    // u16: bit i set when tile slot i (0..13) is taken
constexpr u32 tag_cursor = 0x00F67248;    // u32: next compression tag, in 64 KiB units
}  // namespace g

}  // namespace sly4::tgt
