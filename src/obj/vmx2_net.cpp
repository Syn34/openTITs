// Game code, VMX sweep part 2 (see vmx2.h): network helpers whose only VMX
// use is zeroing a sockaddr with a vector store.
#include "../sly4.h"
#include "vmx2.h"

using namespace sly4;
using namespace sly4::vmx2;

namespace {
constexpr u32 kMemAlloc = 0x0054ED38;        // Mem_Alloc (size, flags, align)
constexpr u32 sys_net_socket = 0x00ABDD44;   // import stub (family, type, protocol)
constexpr u32 sys_net_setsockopt = 0x00ABDD04;  // import stub (s, level, name, value*, len)
constexpr u32 sys_net_bind = 0x00ABDD64;     // import stub (s, sockaddr*, len)
constexpr u32 sys_net_listen = 0x00ABDC64;   // import stub (s, backlog)
constexpr u32 sys_net_close = 0x00ABDCE4;    // import stub Import_ABDCE4 (s): socketclose (name?)
constexpr u32 kNetServer = 0x00F65850;       // Server* : {+0 s32 listen socket, +4 s32 clients[4], +20 u32 count, +24 u8[5]}
constexpr u32 SOL_SOCKET = 0xFFFF;
constexpr u32 SO_REUSEADDR_PS3 = 4352;       // 0x1100: the option set to 1
constexpr u32 kAfInet = 2;
constexpr u32 kSockStream = 1;
constexpr u32 kIpProtoTcp = 6;
constexpr u32 kListenBacklog = 4;

s64 Fd(u32 p) { return static_cast<s32>(load<u32>(p)); }

// Close the server's listen socket and every client socket (+4.., count
// +20 re-read each time), then clear the listen socket.
void ServerCloseAll(PPUContext& ctx) {
    const u32 s = load<u32>(kNetServer);
    call(ctx, sys_net_close, {static_cast<u64>(Fd(s))});
    for (u32 i = 0; i < load<u32>(s + 20); ++i) call(ctx, sys_net_close, {static_cast<u64>(Fd(s + 4 + 4 * i))});
    store<u32>(s, 0);
}
}  // namespace

// Server* f(const NetAddr* a) (called by 00427A0C): opens the LAN TCP
// server. A 32-byte server (Mem_Alloc(32, 0, 16)) gets +0 = 0, +20 = 0,
// the socket(AF_INET, SOCK_STREAM, IPPROTO_TCP), zeroed client slots +4..
// +19 and bytes +24..+28 (byte stores); it becomes kNetServer (null when
// the allocation failed). Then setsockopt(fd, SOL_SOCKET, 0x1100, &1, 4),
// bind to {AF_INET, port a->+4, ip a->+0} (sockaddr zeroed with stvx) and
// listen(fd, 4); a failed bind / listen closes everything (r3 0). r3: the
// server on success, else 0. Quirk: with a failed allocation the fd is
// read from address 0.
DECOMP(004274D0, NetServer_Open_004274D0) {
    const u32 a = arg<u32>(ctx, 0);
    const u32 s = static_cast<u32>(call(ctx, kMemAlloc, {32, 0, 16}));
    if (s != 0) {
        store<u32>(s, 0);
        store<u32>(s + 20, 0);
        store<u32>(s, static_cast<u32>(call(ctx, sys_net_socket, {kAfInet, kSockStream, kIpProtoTcp})));
        for (u32 k = 0; k < 16; ++k) store<u8>(s + 4 + k, 0);
        for (u32 k = 0; k < 5; ++k) store<u8>(s + 24 + k, 0);
    }
    Stack frame(ctx, 176);
    const u32 sa = frame.addr() + 112, one = frame.addr() + 128;
    store<u32>(one, 1);
    store<u32>(kNetServer, s);
    if (static_cast<s32>(call(ctx, sys_net_setsockopt, {static_cast<u64>(Fd(s)), SOL_SOCKET, SO_REUSEADDR_PS3, one, 4})) != 0) {
        ctx.gpr[3] = 0;
        return;
    }
    Stvx(sa, Zero4());
    store<u8>(sa + 1, kAfInet);
    store<u32>(sa + 4, load<u32>(a));
    store<u16>(sa + 2, load<u16>(a + 4));
    if (static_cast<s32>(call(ctx, sys_net_bind, {static_cast<u64>(Fd(load<u32>(kNetServer))), sa, 16})) != 0 ||
        static_cast<s32>(call(ctx, sys_net_listen, {static_cast<u64>(Fd(load<u32>(kNetServer))), kListenBacklog})) != 0) {
        ServerCloseAll(ctx);
        ctx.gpr[3] = 0;
        return;
    }
    ctx.gpr[3] = load<u32>(kNetServer);
}
