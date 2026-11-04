// Small helpers used by World_Update / World_HandleEvent / World_Construct
// (src/world/misc.cpp). Guest addresses in the BCES01284 v01.02 EBOOT;
// "?" marks a guess.
#pragma once

#include "../sly4.h"

namespace sly4 {
namespace wmisc {

namespace mfn {
// Lazily sorted {u32 key, u32 value} arrays: +2 u16 count, +6 u16 flags
// (bit 0 = needs sorting), +8 entries, +16 default value, +20 compare OPD.
// Find(map, const u32* key) -> u32* value slot (map + 16 when missing).
constexpr u32 SortedMap_Find = 0x0076611C;   // (name?)
constexpr u32 SortedMap_Find2 = 0x007845DC;  // same code, another instantiation (name?)
constexpr u32 String_Format = 0x005534D0;
}  // namespace mfn

// Globals.
constexpr u32 kHandleMap = 0x0106C228;     // SortedMap: handle -> object (Handle_Resolve)
constexpr u32 kSoundMap = 0x010AFEE0;      // SortedMap: hash -> sound handle (Sound_Find)
constexpr u32 kDisplayConfig = 0x010C5A20; // +4 u16 width, +6 u16 height
constexpr u32 kUiRoots = 0x00F66868;       // u32 first top-level Ui
constexpr u32 kHexFormat = 0x00AF39A8;     // "0x%x"

// Ui tree links.
constexpr u32 kUiParent = 148, kUiFirstChild = 152, kUiNext = 156;

// Object flags word (+4) of a resolved handle: bit 1 = alive (name?)
constexpr u32 kObjFlags = 4, kObjAlive = 0x2;

// View fov (View_SetFov).
constexpr u32 kViewDirty = 52;   // u8, 0x80 = projection needs rebuilding (name?)
constexpr u32 kViewFov = 56;     // f32 radians

}  // namespace wmisc
}  // namespace sly4
