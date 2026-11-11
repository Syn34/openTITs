// Game code, large-function sweep part 24: the AR server probe (a TCP
// connect sweep over the console's /24 subnet).
#include "../sly4.h"
#include "big24.h"

using namespace sly4;
using namespace sly4::big24;

namespace {
constexpr u32 kSolSocket = 0xFFFF;
constexpr u32 kSockOpt = 4352;        // 0x1100, set to 1 on every socket
constexpr u32 kAfInet = 2, kSockStream = 1, kIpProtoTcp = 6;
constexpr u32 kArPort = 333;
constexpr u32 kTbFreq = 0x01164CD8;   // u64 timebase frequency
constexpr u32 kTwo32A = 0x00AC5A64;   // f64 2^32 (first clock read)
constexpr u32 kTwo32B = 0x00AC5A6C;   // f64 2^32 (polling clock reads)
constexpr u32 kMillion = 0x000A8898;  // f64 1e6 (in .text before the function)
constexpr u32 kThousandF = 0x000A88B8;  // f32 1000
constexpr u32 kWaitMsF = 0x000A88BC;    // f32 333
constexpr u32 kErrIsConn = 56;          // the errno that marks a live server
constexpr u32 kBatch = 100;             // sockets per sweep batch
constexpr u32 kArFound = 0x00F558E2;    // u8: the probe result

// mftb until the low word is non-zero.
u64 ReadTb() {
    u64 tb;
    do tb = read_timebase();
    while (static_cast<u32>(tb) == 0);
    return tb;
}

// The clock in ms as the probe derives it: the u64s as 2^32 * hi + lo
// (fmadd of the two zero-extended words), us = tb / (freq / 1e6),
// truncated through fctidz to its low 32 bits (unsigned), rounded to f32
// and divided (single) by 1000.
f64 ClockMs(u64 tb, u32 two32) {
    const f64 c = Lfd(two32);
    const u64 fr = load<u64>(kTbFreq);
    const f64 per_us = std::fma(c, static_cast<f64>(static_cast<u32>(fr >> 32)), static_cast<f64>(static_cast<u32>(fr))) / Lfd(kMillion);
    const f64 us = std::fma(c, static_cast<f64>(static_cast<u32>(tb >> 32)), static_cast<f64>(static_cast<u32>(tb))) / per_us;
    const u32 lo = static_cast<u32>(rt::fctid(us, true));
    return S(S(static_cast<f64>(lo)) / Lf(kThousandF));
}

// sockaddr_in {len 0, family, port, ip} at sa (16 bytes zeroed first).
void SockAddr(u32 sa, u32 ip) {
    for (u32 k = 0; k < 16; ++k) store<u8>(sa + k, 0);
    store<u8>(sa + 1, kAfInet);
    store<u16>(sa + 2, kArPort);
    store<u32>(sa + 4, ip);
}
}  // namespace

// u8 f() (the AR discovery thread body via Thread_Run_000ABAD4; also
// OptionsMenu_HandleInput2_46FBD0): looks for an AR server on TCP port 333
// in the console's /24 (Net_GetIpAddress into a 4-byte buffer).
// 1. Hosts .3 to .254 except the console's own: socket, setsockopt(s,
//    0xFFFF, 0x1100, &1, 4), connect, close, one at a time (results
//    ignored; the 64-bit socket result is passed as is).
// 2. From host .0 on, batches of 100: 100 sockets (all created even when
//    fewer hosts remain), then for each host left (< 255): setsockopt and
//    connect to its own sockaddr slot; wait 333 ms on the timebase; then
//    connect each again and a server is found when errno is 56
//    (EISCONN); all 100 are closed. Repeats until found or host 255.
// The result (0/1) goes to the u8 0x00F558E2 and r3. The buffer's last byte
// tracks the host being tried. f1 is left as the last wait's start time
// (the clock in ms), as the original leaves it.
DECOMP(000A88C0, Ar_ProbeSubnet_000A88C0) {
    using namespace b24fn;
    Stack frame(ctx, 2272);
    const u32 sp = frame.addr();
    const u32 sa1 = sp + 112;      // phase 1 sockaddr
    const u32 addrs = sp + 128;    // phase 2 sockaddrs [100]
    const u32 one2 = sp + 1728, one1 = sp + 1732;
    const u32 ip = sp + 1736;      // u8[4]
    const u32 socks = sp + 1744;   // s32 [100]
    call(ctx, Net_GetIpAddress, {ip});
    const u32 own = load<u8>(ip + 3);
    for (u32 h = 3; h < 255; ++h) {
        if (h == own) continue;
        const u32 a = (load<u32>(ip) & ~0xFFu) | h;
        store<u8>(ip + 3, static_cast<u8>(h));
        SockAddr(sa1, a);
        const u64 s = call(ctx, NetSocket, {kAfInet, kSockStream, kIpProtoTcp});
        store<u32>(one1, 1);
        call(ctx, NetSetsockopt, {s, kSolSocket, kSockOpt, one1, 4});
        call(ctx, NetConnect, {s, sa1, 16});
        call(ctx, NetClose, {s});
    }
    u32 found = 0, next = 0;  // r31, r30
    f64 t0 = 0.0;
    store<u32>(one2, 1);
    do {
        u32 again = next;  // r29
        for (u32 k = 0; k < kBatch; ++k)
            store<u32>(socks + 4 * k, static_cast<u32>(call(ctx, NetSocket, {kAfInet, kSockStream, kIpProtoTcp})));
        for (u32 k = 0; k < kBatch && next < 255; ++k, ++next) {
            const u64 s = Sw(socks + 4 * k);
            call(ctx, NetSetsockopt, {s, kSolSocket, kSockOpt, one2, 4});
            const u32 a = (load<u32>(ip) & ~0xFFu) | (next & 0xFF);
            store<u8>(ip + 3, static_cast<u8>(next));
            SockAddr(addrs + 16 * k, a);
            call(ctx, NetConnect, {s, addrs + 16 * k, 16});
        }
        t0 = ClockMs(ReadTb(), kTwo32A);
        const f64 wait = Lf(kWaitMsF);
        while (S(ClockMs(ReadTb(), kTwo32B) - t0) < wait) {
        }
        for (u32 k = 0; k < kBatch && again < 255; ++k) {
            const u32 a = (load<u32>(ip) & ~0xFFu) | (again & 0xFF);
            store<u8>(ip + 3, static_cast<u8>(again));
            const u64 s = Sw(socks + 4 * k);
            ++again;
            store<u32>(addrs + 16 * k + 4, a);
            call(ctx, NetConnect, {s, addrs + 16 * k, 16});
            if (load<u32>(static_cast<u32>(call(ctx, NetErrnoLoc, {}))) == kErrIsConn) found = 1;
        }
        for (u32 k = 0; k < kBatch; ++k) call(ctx, NetClose, {Sw(socks + 4 * k)});
    } while (found == 0 && static_cast<s32>(next) < 255);
    store<u8>(kArFound, static_cast<u8>(found));
    ctx.fpr[1] = t0;
    ctx.gpr[3] = found;
}
