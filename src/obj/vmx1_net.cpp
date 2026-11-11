// Game code, VMX sweep part 1 (see vmx1.h): network helpers whose only VMX
// use is zeroing a sockaddr with a vector store.
#include "../sly4.h"
#include "vmx1.h"

using namespace sly4;
using namespace sly4::vmx1;

namespace {
constexpr u32 kNetSocketPtr = 0x00F65854;  // s32* : the LAN socket (null when closed)
constexpr u32 kIpFormat = 0x00AE212C;      // "%d.%d.%d.%d"
constexpr u32 kLanPort = 14120;            // 0x3728
constexpr u32 kAfInet = 2;
}  // namespace

// s32 f(const void* packet) (called by 00428228): sends the 304-byte packet
// to every other host of the console's /24 subnet: the IP from
// cellNetCtlGetInfo(16 = CELL_NET_CTL_INFO_IP_ADDRESS) parsed with
// "%d.%d.%d.%d" (0.0.0.0 when unavailable), then for host 1..256 except our
// own the address is formatted, parsed back and sent to on port 14120 with
// sendto(*kNetSocketPtr, packet, 304, 0, &sockaddr_in, 16). Nothing when
// there's no socket (r3 = 0). r3 is left as the last call's result.
// Quirk: a failed parse of a host address reuses the previous one (the
// first time, whatever the stack held).
DECOMP(004277A8, Net_BroadcastSubnet_004277A8) {
    const u32 packet = arg<u32>(ctx, 0);
    if (load<u32>(kNetSocketPtr) == 0) {
        ctx.gpr[3] = 0;
        return;
    }
    Stack frame(ctx, 336);
    const u32 sp = frame.addr() - 112;  // the original's sp: sockaddr +112, host +128, ip +136, ints +144 / +160, info +176
    store<u32>(sp + 136, 0);
    if (static_cast<s32>(call(ctx, v1fn::cellNetCtlGetInfo, {16, sp + 176, 0})) == 0 &&
        static_cast<s32>(call(ctx, v1fn::sscanf, {sp + 176, kIpFormat, sp + 160, sp + 164, sp + 168, sp + 172})) == 4) {
        store<u8>(sp + 136, static_cast<u8>(load<u32>(sp + 160)));
        store<u16>(sp + 140, 0);
        store<u8>(sp + 137, static_cast<u8>(load<u32>(sp + 164)));
        store<u8>(sp + 138, static_cast<u8>(load<u32>(sp + 168)));
        store<u8>(sp + 139, static_cast<u8>(load<u32>(sp + 172)));
    }
    const u32 a = load<u8>(sp + 136), b = load<u8>(sp + 137), c = load<u8>(sp + 138);
    const s32 self = static_cast<s32>(load<u8>(sp + 139)) - 1;
    for (s32 i = 0; i < 256; ++i) {
        if (i == self) continue;
        const u64 s = call(ctx, v1fn::String_Format, {kIpFormat, a, b, c, static_cast<u32>(i + 1)});
        const s32 n = static_cast<s32>(call(ctx, v1fn::sscanf, {s, kIpFormat, sp + 144, sp + 148, sp + 152, sp + 156}));
        const s32 sock = static_cast<s32>(load<u32>(load<u32>(kNetSocketPtr)));
        if (n == 4)
            for (u32 k = 0; k < 4; ++k) store<u8>(sp + 128 + k, static_cast<u8>(load<u32>(sp + 144 + 4 * k)));
        for (u32 k = 0; k < 16; k += 4) store<u32>(sp + 112 + k, 0);  // stvx of a zero vector
        const u32 host = load<u32>(sp + 128);
        store<u8>(sp + 113, kAfInet);
        store<u32>(sp + 116, host);
        store<u16>(sp + 114, kLanPort);
        call(ctx, v1fn::sys_net_sendto, {static_cast<u64>(static_cast<s64>(sock)), packet, 304, 0, sp + 112, 16});
    }
}
