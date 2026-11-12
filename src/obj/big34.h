// Game code, large-function sweep part 34: helpers shared by big34_*.cpp.
#pragma once

#include <bit>
#include <cmath>

#include "big33.h"

namespace sly4 {
namespace big34 {

using namespace opentit;
using namespace opentit::decomp;
using big31::Lfs;
using big32::W32f;
using big29::Adds;
using big29::Divs;
using big29::Fmadds;
using big29::Fmsubs;
using big29::Muls;
using big29::S;
using big29::Sqrts;
using big29::Subs;
using big29::StackArg;
using big19::VCall;

// Addresses of callees used by several big34 files.
namespace b34fn {
constexpr u32 Cursor_Alloc = 0x0056DF9C;            // (cursor, align, size) -> p or 0
constexpr u32 cellGcmAddressToOffset = 0x00ABD5E4;  // import stub (address, u32* offset)
}  // namespace b34fn

// A script node's parameter k: the node's s16 indices +4 (a) and +2 (b) pick
// 128-byte rows of the table at *0x00F65038 (+24; -1: no row, address 0).
// Row b's selector bytes sit at +*(u16*)0x0106CF7E when its byte +2 is
// non-zero (else at address 0); selector k, times 4, plus the u16 at
// 0x0106CF70 indexes row a. Returns the parameter's address.
inline u32 NodeParamAddr(u32 node, u32 k) {
    const u32 t = load<u32>(0x00F65038);
    const s32 a = static_cast<s16>(load<u16>(node + 4));
    const s32 b = static_cast<s16>(load<u16>(node + 2));
    const u32 rowA = a != -1 ? load<u32>(t + 24) + (static_cast<u32>(a) << 7) : 0u;
    const u32 rowB = b != -1 ? load<u32>(t + 24) + (static_cast<u32>(b) << 7) : 0u;
    const u32 sel = load<u8>(rowB + 2) != 0 ? rowB + load<u16>(0x0106CF7E) : 0u;
    return rowA + load<u16>(0x0106CF70) + (static_cast<u32>(load<u8>(sel + k)) << 2);
}

}  // namespace big34
}  // namespace sly4
