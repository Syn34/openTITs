// World objects, part 15: the rest of the still-recompiled functions of
// 0x00550000-0x005AA000 (banks, file readers, IFF loaders, draw helpers,
// ...) and the callees part 14 named but left recompiled.
#pragma once

#include <bit>
#include <cmath>

#include "obj14.h"

namespace sly4 {
namespace obj15 {

using namespace opentit;

namespace o15fn {
constexpr u32 Mem_Alloc = 0x0054ED38;            // (size, pool, align)
constexpr u32 Mem_FreeFrom = 0x0054EE6C;         // (p, 0)
constexpr u32 Iff_ReadU8 = 0x005643C4;           // (IffReader*, u8* out, name) -> bytes
constexpr u32 Iff_ReadU16 = 0x00564340;          // (IffReader*, u16* out, name) -> bytes
constexpr u32 Bank_SetState = 0x00564F44;        // (v)
constexpr u32 DrawRecordList_Init = 0x004FD214;  // (list)
constexpr u32 GeomInstance_DrawLod = 0x0058AC70; // (obj, u8 lod, u8, u8)
}  // namespace o15fn

// The bank globals (0x01193060): +64 the quad array (112-byte entries),
// +68 u16 its count, +5200 u16, +5248 f32[3] (name?).
constexpr u32 kBankGlobals = 0x01193060;
constexpr u32 kBgQuads = 64, kBgQuadCount = 68, kBg5200 = 5200, kBgOrigin = 5248;
constexpr u32 kBgQuadSize = 112;
constexpr u32 kBankFlags = 64;                   // bank u8: 0x80 running, 0x20 / 0x10 (name?)
constexpr u32 kBankState = 68;                   // the Bank_SetState record (name?)
constexpr u32 kSlotTableUse = 0x01193004 + 10;   // obj4.h kSlotTableBase +10
constexpr u32 kDirtyListOff = 0x0116BD04;        // obj.h
constexpr u32 kDirtyListOff2 = 0x0116BD05;       // u8 (name?)
constexpr u32 kRenderFlags = 0x010DDC88;         // +8 u8 (name?)

// FileReader (obj14): +4 the File, +8 u32 the bytes read so far.
constexpr u32 kFrFile = 4, kFrCount = 8;
constexpr u32 kVfnFileRead = 12;                 // file/zip.h kFileRead: (file, buf, size, 0) -> bytes

}  // namespace obj15
}  // namespace sly4
