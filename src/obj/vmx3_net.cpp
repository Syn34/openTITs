// Game code, VMX sweep part 3 (see vmx3.h): network setup whose only VMX
// use is zeroing a sockaddr with a vector store.
#include "../sly4.h"
#include "vmx3.h"

using namespace sly4;
using namespace sly4::vmx3;

namespace {
constexpr u32 kSysmoduleRequire = 0x0056C020;  // Sysmodule_Require (id)
constexpr u32 sys_net_initialize_network_ex = 0x00ABDC24;  // import stub (const sys_net_initialize_parameter_t*)
constexpr u32 cellNetCtlInit = 0x00ABDE64;   // import stub ()
constexpr u32 sys_net_socket3 = 0x00ABDD44;  // import stub (family, type, protocol)
constexpr u32 sys_net_setsockopt3 = 0x00ABDD04;  // import stub (s, level, name, value*, len)
constexpr u32 sys_net_bind3 = 0x00ABDD64;    // import stub (s, sockaddr*, len)
constexpr u32 kNetPool = 0x0108A1B0;         // the network memory pool (128 KB)
constexpr u32 kNetUdp = 0x00F65854;          // UdpSock* : {+0 s32 socket, +4 u8[16], +20 u32, +24 u8[5]}
constexpr u32 kFmtIp = 0x00AE212C;           // "%d.%d.%d.%d"
constexpr u32 kUdpPort = 14120;              // 0x3728

s64 Fd3(u32 p) { return static_cast<s32>(load<u32>(p)); }
}  // namespace

// s32 f() (called by World_Construct): network start-up and the LAN UDP
// socket. Requires sysmodules 0 and 20, sys_net_initialize_network_ex with
// the 128 KB pool 0x0108A1B0 and cellNetCtlInit (either < 0: returns it).
// A 32-byte record (Mem_Alloc(32, 0, 16); null on failure, the socket is
// then read from address 0) gets +0 = 0, +20 = 0, the socket(AF_INET,
// SOCK_DGRAM, IPPROTO_UDP) and zeroed bytes +4..+19 / +24..+28, and becomes
// 0x00F65854. setsockopt(SOL_SOCKET 0x1100 = 1), (SOL_SOCKET SO_BROADCAST
// = 1), (level 6, option 1 = 1) (a non-zero result returns it); then bind
// to {AF_INET, port 14120, the console's IP (cellNetCtlGetInfo 16 parsed
// with "%d.%d.%d.%d", else 0)}. r3: the last call's result.
DECOMP(00426DD4, Net_OpenUdp_00426DD4) {
    Stack frame(ctx, 448);
    const u32 sp = frame.addr();
    call(ctx, kSysmoduleRequire, {0});
    call(ctx, kSysmoduleRequire, {20});
    store<u32>(sp + 152, 0x00020000u);
    store<u32>(sp + 148, kNetPool);
    store<u32>(sp + 156, 0);
    if (static_cast<s32>(call(ctx, sys_net_initialize_network_ex, {sp + 148})) < 0) return;
    if (static_cast<s32>(call(ctx, cellNetCtlInit, {})) < 0) return;
    const u32 s = static_cast<u32>(call(ctx, sly4::fn::Mem_Alloc, {32, 0, 16}));
    if (s != 0) {
        store<u32>(s, 0);
        store<u32>(s + 20, 0);
        store<u32>(s, static_cast<u32>(call(ctx, sys_net_socket3, {2, 2, 17})));
        for (u32 k = 0; k < 16; ++k) store<u8>(s + 4 + k, 0);
        for (u32 k = 0; k < 5; ++k) store<u8>(s + 24 + k, 0);
    }
    store<u32>(sp + 136, 1);
    store<u32>(kNetUdp, s);
    if (static_cast<s32>(call(ctx, sys_net_setsockopt3, {static_cast<u64>(Fd3(s)), 0xFFFF, 4352, sp + 136, 4})) != 0) return;
    store<u32>(sp + 132, 1);
    if (static_cast<s32>(call(ctx, sys_net_setsockopt3, {static_cast<u64>(Fd3(load<u32>(kNetUdp))), 0xFFFF, 32, sp + 132, 4})) != 0) return;
    store<u32>(sp + 128, 1);
    if (static_cast<s32>(call(ctx, sys_net_setsockopt3, {static_cast<u64>(Fd3(load<u32>(kNetUdp))), 6, 1, sp + 128, 4})) != 0) return;
    const u32 u = load<u32>(kNetUdp);
    store<u32>(sp + 140, 0);
    if (static_cast<s32>(call(ctx, v1fn::cellNetCtlGetInfo, {16, sp + 176})) == 0 &&
        static_cast<s32>(call(ctx, v1fn::sscanf, {sp + 176, kFmtIp, sp + 160, sp + 164, sp + 168, sp + 172})) == 4) {
        store<u8>(sp + 140, static_cast<u8>(load<u32>(sp + 160)));
        store<u16>(sp + 144, 0);
        store<u8>(sp + 141, static_cast<u8>(load<u32>(sp + 164)));
        store<u8>(sp + 142, static_cast<u8>(load<u32>(sp + 168)));
        store<u8>(sp + 143, static_cast<u8>(load<u32>(sp + 172)));
    }
    const u32 ip = load<u32>(sp + 140);  // the four bytes, big-endian
    Stvx(sp + 112, Zero4());
    store<u8>(sp + 113, 2);
    store<u16>(sp + 114, kUdpPort);
    store<u32>(sp + 116, ip);
    call(ctx, sys_net_bind3, {static_cast<u64>(Fd3(u)), sp + 112, 16});
}
