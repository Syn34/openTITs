// Game code, large-function sweep part 24: the AR server probe (a TCP
// connect sweep over the console's /24 subnet).
#include "big24.h"

namespace sly4::obj {

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

// sockaddr_in (16 bytes).
struct SockAddrIn {
    u8 len;          // +0
    u8 family;       // +1
    be_u16 port;     // +2
    be_u32 ip;       // +4
    pad<8> _8;
};
static_assert(sizeof(SockAddrIn) == 16);
// The original's locals (sp offsets).
struct NetLocals {
    SockAddrIn sa1;              // sp+112: phase 1
    SockAddrIn addrs[kBatch];    // sp+128: phase 2
    be_u32 one2;                 // sp+1728
    be_u32 one1;                 // sp+1732
    WordBytes ip;                // sp+1736: u8[4]
    pad<4> _1740;
    be_s32 socks[kBatch];        // sp+1744
};
static_assert(offsetof(NetLocals, one2) == 1616 && offsetof(NetLocals, ip) == 1624 && offsetof(NetLocals, socks) == 1632);

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
    const f64 c = ld(global<be_f64>(two32));
    const u64 fr = global<be_u64>(kTbFreq);
    const f64 per_us = std::fma(c, static_cast<f64>(static_cast<u32>(fr >> 32)), static_cast<f64>(static_cast<u32>(fr))) / ld(global<be_f64>(kMillion));
    const f64 us = std::fma(c, static_cast<f64>(static_cast<u32>(tb >> 32)), static_cast<f64>(static_cast<u32>(tb))) / per_us;
    const u32 lo = static_cast<u32>(Fctidz(us));
    return S(S(static_cast<f64>(lo)) / lf(global<be_f32>(kThousandF)));
}

// sockaddr_in {len 0, family, port, ip} (16 bytes zeroed first).
void SetSockAddr(SockAddrIn& sa, u32 ip) {
    sa = SockAddrIn{};
    sa.family = kAfInet;
    sa.port = kArPort;
    sa.ip = ip;
}
}  // namespace

// u8 f() (the AR discovery thread body via Thread_Run_000ABAD4, which
// reads r3; also OptionsMenu_HandleInput2_46FBD0): looks for an AR server
// on TCP port 333 in the console's /24 (Net_GetIpAddress into a 4-byte
// buffer).
// 1. Hosts .3 to .254 except the console's own: socket, setsockopt(s,
//    0xFFFF, 0x1100, &1, 4), connect, close, one at a time (results
//    ignored; the 64-bit socket result is passed as is).
// 2. From host .0 on, batches of 100: 100 sockets (all created even when
//    fewer hosts remain), then for each host left (< 255): setsockopt and
//    connect to its own sockaddr slot; wait 333 ms on the timebase; then
//    connect each again and a server is found when errno is 56
//    (EISCONN); all 100 are closed. Repeats until found or host 255.
// The result (0/1) goes to the u8 0x00F558E2 and is returned. The
// buffer's last byte tracks the host being tried.
u8 Ar_ProbeSubnet_000A88C0() {
    using namespace b24fn;
    GuestFrame frame(2272);
    NetLocals& L = *frame.as<NetLocals>(112);
    call(Net_GetIpAddress, &L.ip);
    const u32 own = L.ip.b[3];
    for (u32 h = 3; h < 255; ++h) {
        if (h == own) continue;
        const u32 a = (L.ip.w & ~0xFFu) | h;
        L.ip.b[3] = static_cast<u8>(h);
        SetSockAddr(L.sa1, a);
        const u64 s = call<u64>(NetSocket, kAfInet, kSockStream, kIpProtoTcp);
        L.one1 = 1u;
        call(NetSetsockopt, s, kSolSocket, kSockOpt, &L.one1, 4u);
        call(NetConnect, s, &L.sa1, 16u);
        call(NetClose, s);
    }
    u32 found = 0, next = 0;
    L.one2 = 1u;
    do {
        u32 again = next;
        for (u32 k = 0; k < kBatch; ++k) L.socks[k] = static_cast<s32>(call<u32>(NetSocket, kAfInet, kSockStream, kIpProtoTcp));
        for (u32 k = 0; k < kBatch && next < 255; ++k, ++next) {
            const s32 s = L.socks[k];
            call(NetSetsockopt, s, kSolSocket, kSockOpt, &L.one2, 4u);
            const u32 a = (L.ip.w & ~0xFFu) | (next & 0xFF);
            L.ip.b[3] = static_cast<u8>(next);
            SetSockAddr(L.addrs[k], a);
            call(NetConnect, s, &L.addrs[k], 16u);
        }
        const f64 t0 = ClockMs(ReadTb(), kTwo32A);
        const f64 wait = lf(global<be_f32>(kWaitMsF));
        while (S(ClockMs(ReadTb(), kTwo32B) - t0) < wait) {
        }
        for (u32 k = 0; k < kBatch && again < 255; ++k) {
            const u32 a = (L.ip.w & ~0xFFu) | (again & 0xFF);
            L.ip.b[3] = static_cast<u8>(again);
            const s32 s = L.socks[k];
            ++again;
            L.addrs[k].ip = a;
            call(NetConnect, s, &L.addrs[k], 16u);
            if (gm<be_u32>(call<u32>(NetErrnoLoc)) == kErrIsConn) found = 1;
        }
        for (u32 k = 0; k < kBatch; ++k) call(NetClose, L.socks[k].get());
    } while (found == 0 && static_cast<s32>(next) < 255);
    global<u8>(kArFound) = static_cast<u8>(found);
    return static_cast<u8>(found);
}

}  // namespace sly4::obj

NATIVE(000A88C0, sly4::obj::Ar_ProbeSubnet_000A88C0)
