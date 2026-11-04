// The renderer range's functions of ~60 to ~300 instructions (gfx10*.cpp):
// the remaining destructors, chunk handlers, shader-type creators, render
// pass wrappers and inline RSX command blocks.
#pragma once

#include <bit>

#include "gfx9.h"

namespace sly4::render {
namespace gfx10 {

namespace fn {
// Decompiled in gfx10*.cpp.
constexpr u32 Part_Release_0052F06C = 0x0052F06C;  // (Handle*) tile slot bit + part free (gfx9 Release_0052F06C) (name?)

// Still recompiled (called from gfx10*.cpp).
constexpr u32 Texture_Alloc = 0x0051D368;          // () -> Texture*
constexpr u32 Texture_Init = 0x0053A640;           // (Texture*, data, size, kind, name) -> bool
constexpr u32 String_CopyN = 0x00553638;           // (dst, src, n)
constexpr u32 Iff_ReadU8 = 0x005643C4;             // (IffReader*, u8* out, name)
constexpr u32 Iff_ReadU32 = 0x005642AC;            // (IffReader*, u32* out, name)
constexpr u32 FileReader_ReadF32Swapped = 0x005698F4;   // (FileReader*, f32* out, name)
constexpr u32 FileReader_ReadVec3Swapped = 0x005699F0;  // (FileReader*, f32 out[3], name)
constexpr u32 Libc_printf = 0x007240F4;
constexpr u32 Cursor_Reset = 0x0056DFC8;           // (c)
constexpr u32 Draw_SetTarget = 0x005373A8;         // (Target*, u32)
constexpr u32 Draw_Clear = 0x00531F80;             // (mask, colour, f1 depth, stencil, stencil mask)
constexpr u32 Gcm_PadNops_005E0AF0 = 0x005E0AF0;   // (context, u32 words) (libgcm, name?)
constexpr u32 Gcm_SetCall_005E0600 = 0x005E0600;   // (context, u32 offset) (libgcm, name?)
constexpr u32 Gcm_SetReferenceCommand_005E24F8 = 0x005E24F8;  // (context, u32 value) (libgcm, name?)
// Edge / SPURS (SDK).
constexpr u32 Edge_0060E4B0 = 0x0060E4B0;
constexpr u32 Edge_0060DFD0 = 0x0060DFD0;
constexpr u32 Edge_0060DE84 = 0x0060DE84;
constexpr u32 Edge_0060E01C = 0x0060E01C;
constexpr u32 cellSpursShutdownJobChain = 0x00ABE0A4;
constexpr u32 cellSpursJoinJobChain = 0x00ABE164;
}  // namespace fn

// lfs: a single's bits as the double the FPR holds (NaN payloads kept, not quieted).
inline f64 lfs_value(u32 w) {
    if ((w & 0x7F800000u) == 0x7F800000u && (w & 0x007FFFFFu) != 0)
        return std::bit_cast<f64>((static_cast<u64>(w >> 31) << 63) | (u64{0x7FF} << 52) |
                                  (static_cast<u64>(w & 0x007FFFFFu) << 29));
    return static_cast<f64>(std::bit_cast<f32>(w));
}

}  // namespace gfx10
}  // namespace sly4::render
