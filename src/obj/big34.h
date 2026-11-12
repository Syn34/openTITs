// Game code, large-function sweep part 34: helpers shared by big34_*.cpp.
#pragma once

#include "native/objtypes.h"

namespace sly4::obj {

// Addresses of callees used by several big34 files.
namespace b34fn {
constexpr u32 Cursor_Alloc = 0x0056DF9C;            // (cursor, align, size) -> p or 0
constexpr u32 cellGcmAddressToOffset = 0x00ABD5E4;  // import stub (address, u32* offset)
}  // namespace b34fn

}  // namespace sly4::obj
