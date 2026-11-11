// Game code, large-function sweep part 24: helpers shared by big24_*.cpp.
#pragma once

#include <bit>
#include <cmath>

#include "big23.h"

namespace sly4 {
namespace big24 {

using namespace opentit;
using namespace opentit::decomp;
using big19::VCall;
using big23::Adds;
using big23::Fsel;
using big23::Lfd;
using big23::Muls;
using big23::Subs;
using mid15::Cw;
using mid15::Fb;
using mid15::Fmadds;
using mid15::Fmsubs;
using mid15::Lf;
using mid15::S;
using mid15::Sf;
using mid16::Divs;
using mid16::Sqrts;

namespace b24fn {
constexpr u32 NetSocket = 0x00ABDD44;      // import stub sys_net socket (family, type, protocol)
constexpr u32 NetSetsockopt = 0x00ABDD04;  // import stub sys_net setsockopt (s, level, name, value*, len)
constexpr u32 NetConnect = 0x00ABDCC4;     // import stub sys_net connect (s, sockaddr*, len)
constexpr u32 NetClose = 0x00ABDCE4;       // import stub sys_net socketclose (s)
constexpr u32 NetErrnoLoc = 0x00ABDCA4;    // import stub _sys_net_errno_loc () -> s32*
constexpr u32 Net_GetIpAddress = 0x00426CE0;  // (u8 ip[4])
}  // namespace b24fn

// The s32 in a word, sign-extended (lwz + extsw).
inline u64 Sw(u32 ea) { return static_cast<u64>(static_cast<s64>(static_cast<s32>(load<u32>(ea)))); }

}  // namespace big24
}  // namespace sly4
