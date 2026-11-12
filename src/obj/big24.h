// Game code, large-function sweep part 24: helpers shared by big24_*.cpp.
#pragma once

#include "native/objtypes.h"

namespace sly4::obj {

namespace b24fn {
constexpr u32 NetSocket = 0x00ABDD44;      // import stub sys_net socket (family, type, protocol)
constexpr u32 NetSetsockopt = 0x00ABDD04;  // import stub sys_net setsockopt (s, level, name, value*, len)
constexpr u32 NetConnect = 0x00ABDCC4;     // import stub sys_net connect (s, sockaddr*, len)
constexpr u32 NetClose = 0x00ABDCE4;       // import stub sys_net socketclose (s)
constexpr u32 NetErrnoLoc = 0x00ABDCA4;    // import stub _sys_net_errno_loc () -> s32*
constexpr u32 Net_GetIpAddress = 0x00426CE0;  // (u8 ip[4])
}  // namespace b24fn

// big24_net.cpp: the AR server probe (0/1, also stored to the u8 0x00F558E2).
u8 Ar_ProbeSubnet_000A88C0();

}  // namespace sly4::obj
