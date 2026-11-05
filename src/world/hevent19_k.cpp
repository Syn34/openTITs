// World_HandleEvent callees, part 19k: more small leaves under the World
// event and state handlers (definition entries, release, instance names,
// flag requests, table lookups, Havok start-up, the AR discovery thread).
#include "hevent19.h"

using namespace sly4;
using namespace sly4::hevent;
using namespace sly4::hevent19;

namespace {
constexpr u32 kDeferredDestroy = 0x005082F4;  // (obj, destroy OPD) (symbols.txt)
constexpr u32 kDestroyOpd4D22B0 = 0x00F3AA08;
constexpr u32 kStringFormat = 0x005534D0;     // String_Format (symbols.txt)
constexpr u32 kInstFormat = 0x00AC68F0;       // "%s_inst%05d"
constexpr u32 kRequests = 0x0106C330;         // request block (+132 u8 busy) (name?)
constexpr u32 kRequest369D98 = 0x00369D98;    // (block, a, -, c; f1) (name?)
constexpr u32 kHashTable = 0x00AC9A1C;        // 107 28-byte entries {u32 hash, ...} (name?)
constexpr u32 kHashTableCount = 107, kHashTableStride = 28;
constexpr u32 kHavokInit[] = {0x0085F3C0, 0x00943AC0, 0x00949B30, 0x00955694, 0x009DD0EC, 0x008680E0, 0x00987BA4};
constexpr u32 kSysPpuThreadCreate = 0x00ABF144;  // sysPrxForUser import stub
constexpr u32 kDiscoverOpd = 0x00F30F90, kDiscoverName = 0x00AC5C5C;  // "Discover AR Server"
constexpr u32 kDiscoverPrio = 1536, kDiscoverStack = 8192, kDiscoverFlags = 1;  // SYS_PPU_THREAD_CREATE_JOINABLE
}  // namespace

// u32 Obj_EntryAt3769C4(Obj* o, s32 i) (name?): +16 of the definition's
// (+8) 56-byte entry i (+148 table, +152 s32 count), or 0 out of range.
DECOMP(003769C4, Obj_EntryAt3769C4) {
    const s32 i = arg<s32>(ctx, 1);
    const u32 d = load<u32>(arg<u32>(ctx, 0) + 8);
    if (i < 0 || !(i < static_cast<s32>(load<u32>(d + 152)))) return ret(ctx, 0u);
    ret(ctx, load<u32>(load<u32>(d + 148) + static_cast<u32>(i) * 56 + 16));
}

// void Obj_Release4D22B0(Obj* o) (name?): drops a reference (+4 u16); at
// zero (or below) the object goes to DeferredDestroy.
DECOMP(004D22B0, Obj_Release4D22B0) {
    const u32 o = arg<u32>(ctx, 0);
    const u16 c = static_cast<u16>(load<u16>(o + 4) - 1);
    ctx.gpr[5] = static_cast<u64>(load<u16>(o + 4)) - 1;
    ctx.gpr[6] = static_cast<u64>(static_cast<s64>(static_cast<s16>(c)));
    store<u16>(o + 4, c);
    if (static_cast<s16>(c) > 0) return;  // r3 = o
    call(ctx, kDeferredDestroy, {o, kDestroyOpd4D22B0});
}

// const char* Obj_InstanceName0CBA40(Obj* o) (name?): bumps the instance
// counter (+12) and formats "<name +56>_inst<counter>".
DECOMP(000CBA40, Obj_InstanceName0CBA40) {
    const u32 o = arg<u32>(ctx, 0);
    const u64 n = static_cast<u64>(load<u32>(o + 12)) + 1;
    store<u32>(o + 12, static_cast<u32>(n));
    ctx.gpr[7] = n;
    call(ctx, kStringFormat, {kInstFormat, o + 56, static_cast<u32>(n)});
}

// void Request_Post36A050(u32 a, -, u32 c; f1) (name?): forwards (a, c) to
// 0x00369D98 on the request block unless it is busy (+132). The second
// argument is not used.
DECOMP(0036A050, Request_Post36A050) {
    const u32 a = arg<u32>(ctx, 0);
    const u64 c = ctx.gpr[5];
    ctx.gpr[4] = a;
    if (load<u8>(kRequests + 132) != 0) {
        ctx.gpr[5] = kRequests;
        ctx.gpr[6] = load<u8>(kRequests + 132);
        return ret(ctx, c);
    }
    call(ctx, kRequest369D98, {kRequests, a, kRequests, c});
}

// bool Obj_RequestFlag3736C4(Obj* o, u32 v) (name?): for an object with
// +360 bit 31 set, bit 27 clear and +160 >> 1 == 5: sets bit 28 and
// bit 30 = v & 1. Returns whether it did.
DECOMP(003736C4, Obj_RequestFlag3736C4) {
    const u32 o = arg<u32>(ctx, 0), v = arg<u32>(ctx, 1);
    const u32 w = load<u32>(o + 360);
    ctx.gpr[5] = w;  // r5 / r6 as the original leaves them
    if ((w & 0x08000000u) != 0) {
        ctx.gpr[6] = 1;
        return ret(ctx, 0u);
    }
    if ((w & 0x80000000u) == 0) {
        ctx.gpr[6] = 0;
        return ret(ctx, 0u);
    }
    const u32 k = (load<u32>(o + 160) >> 1) & 0x7FFFFFFFu;
    ctx.gpr[6] = k;
    if (k != 5) return ret(ctx, 0u);
    const u32 nw = ((w | 0x10000000u) & ~0x40000000u) | ((v & 1) << 30);
    ctx.gpr[5] = nw;
    store<u32>(o + 360, nw);
    ret(ctx, 1u);
}

// s32 Table_Index1205AC(u32 hash) (name?): the index of hash in the
// 107-entry table at 0x00AC9A1C, or -1 (also for hash 0).
DECOMP(001205AC, Table_Index1205AC) {
    const u32 h = arg<u32>(ctx, 0);
    if (h != 0)
        for (u32 i = 0; i < kHashTableCount; ++i)
            if (load<u32>(kHashTable + i * kHashTableStride) == h) return ret(ctx, i);
    ret(ctx, -1);
}

// u32 Obj_SlotIndex121B68(Obj* o, u32 hash) (name?): the index of hash in
// the four words at +1420 (stopping at the first 0), or 0xFFFFFFFF.
DECOMP(00121B68, Obj_SlotIndex121B68) {
    const u32 base = arg<u32>(ctx, 0) + 1420, h = arg<u32>(ctx, 1);
    for (u32 i = 0; i < 4; ++i) {
        const u32 v = load<u32>(base + 4 * i);
        if (v == 0) break;
        if (v == h) return ret(ctx, i);
    }
    ret(ctx, 0xFFFFFFFFu);
}

// void Havok_Init491678() (name?): runs the seven Havok start-up calls in
// order (registers passed through as the original leaves them).
DECOMP(00491678, Havok_Init491678) {
    for (const u32 f : kHavokInit) call(ctx, f);
}

// s32 Ar_StartDiscoveryA9670() (name?): creates the "Discover AR Server"
// thread (entry OPD 0x00F30F90, arg 0, priority 1536, 8 KB stack,
// joinable). Returns sys_ppu_thread_create's result.
DECOMP(000A9670, Ar_StartDiscoveryA9670) {
    Stack frame(ctx, 16);  // the thread id (r1 +112)
    call(ctx, kSysPpuThreadCreate, {frame.addr(), kDiscoverOpd, 0, kDiscoverPrio, kDiscoverStack, kDiscoverFlags, kDiscoverName});
}
