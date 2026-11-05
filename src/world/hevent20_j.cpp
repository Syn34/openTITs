// World_HandleEvent callees, part 20j: the asynchronous save buffer's read
// / write state machine and a session part check.
#include "hevent19.h"

using namespace sly4;
using namespace sly4::hevent;

namespace {
constexpr u32 kAsyncReady = 0x0101278A;  // u8: the async buffer is usable
constexpr u32 kAsyncState = 0x00F9D1AC;  // s32: 0 idle, 1 busy, 2 read done, 3 failed / write done
constexpr u32 kAsyncWordA = 0x00F9D1B4, kAsyncWordB = 0x00F9D1B8;  // u32 cleared when a job starts
constexpr u32 kAsyncBusy = 0x00F9D1C4;   // u8
constexpr u32 kAsyncWrite = 0x00F64538;  // u8: a write is pending
constexpr u32 kAsyncBuffer = 0x010127A0; // the transfer buffer
constexpr u32 kAsyncStart0AF5FC = 0x000AF5FC;  // (s32 write) (name?)
constexpr u32 kAsyncSwap0AF8E4 = 0x000AF8E4;   // (buffer) (name?)
constexpr u32 kMemmove = 0x0071C3F0;           // (dst, src, n) (symbols.txt memmove)
constexpr u32 kWorldPartObj = 0x128164;        // World +: Object* {+5540 s32 key, +5572 flags}
constexpr u32 kWorldUser = 4208;               // World + u8 current user

// The user's save-slot flags word of a part (u8 +822 index): bit 0x20.
inline u32 part_done(u32 w, u32 part) {
    const u32 slot = w + load<u8>(w + kWorldUser) * kSaveSlotSize + 4 * load<u8>(part + 822) + 496;
    return (load<u16>(slot) & 0x20) != 0 ? 1 : 0;
}
}  // namespace

// s32 Async_Read0AFFD0(void* dst, u32 n) (name?): state 0 starts a read
// (0x000AF5FC(0)) -> 1; 1 polls (0x000AF5FC(0)) -> 1; 2 copies the buffer
// out (after 0x000AF8E4) and goes idle -> 2; 3 goes idle -> 3; other states
// are returned as is. 3 when the buffer is unusable.
DECOMP(000AFFD0, Async_Read0AFFD0) {
    const u32 dst = arg<u32>(ctx, 0), n = arg<u32>(ctx, 1);
    if (load<u8>(kAsyncReady) == 0) {
        ret(ctx, 3);
        return;
    }
    const s32 st = static_cast<s32>(load<u32>(kAsyncState));
    switch (st) {
    case 0:
        store<u32>(kAsyncWordA, 0);
        store<u32>(kAsyncWordB, 0);
        store<u32>(kAsyncState, 1);
        store<u8>(kAsyncBusy, 1);
        call(ctx, kAsyncStart0AF5FC, {0});
        ret(ctx, 1);
        return;
    case 1:
        call(ctx, kAsyncStart0AF5FC, {0});
        ret(ctx, 1);
        return;
    case 2:
        call(ctx, kAsyncSwap0AF8E4, {kAsyncBuffer});
        call(ctx, kMemmove, {dst, kAsyncBuffer, n});
        store<u8>(kAsyncBusy, 0);
        store<u32>(kAsyncState, 0);
        ret(ctx, 2);
        return;
    case 3:
        store<u32>(kAsyncState, 0);
        store<u8>(kAsyncBusy, 0);
        ret(ctx, 3);
        return;
    default:
        ret(ctx, static_cast<u64>(static_cast<s64>(st)));
    }
}

// s32 Async_Write0AFE94(const void* src, u32 n) (name?): state 0 copies src
// into the buffer, 0x000AF8E4 and starts a write (0x000AF5FC(1)) -> 1; 1
// polls -> 1; 2 / 3 go idle (write flag cleared) -> 2 / 3; other states are
// returned as is. 3 when the buffer is unusable.
DECOMP(000AFE94, Async_Write0AFE94) {
    const u32 src = arg<u32>(ctx, 0), n = arg<u32>(ctx, 1);
    if (load<u8>(kAsyncReady) == 0) {
        ret(ctx, 3);
        return;
    }
    const s32 st = static_cast<s32>(load<u32>(kAsyncState));
    switch (st) {
    case 0:
        store<u32>(kAsyncWordA, 0);
        store<u32>(kAsyncWordB, 0);
        store<u32>(kAsyncState, 1);
        store<u8>(kAsyncBusy, 1);
        store<u8>(kAsyncWrite, 1);
        call(ctx, kMemmove, {kAsyncBuffer, src, n});
        call(ctx, kAsyncSwap0AF8E4, {kAsyncBuffer});
        call(ctx, kAsyncStart0AF5FC, {1});
        ret(ctx, 1);
        return;
    case 1:
        call(ctx, kAsyncStart0AF5FC, {1});
        ret(ctx, 1);
        return;
    case 2:
    case 3:
        store<u32>(kAsyncState, 0);
        store<u8>(kAsyncBusy, 0);
        store<u8>(kAsyncWrite, 0);
        ret(ctx, static_cast<u32>(st));
        return;
    default:
        ret(ctx, static_cast<u64>(static_cast<s64>(st)));
    }
}

// bool Session_PartDone2E2740(Session* s, s32 key) (name?): key 0 -> the
// current part's (+36) done bit; else 1 when the world's part object (live,
// +5572 bit 0x08000000 clear) has that key, or the done bit of the s +24
// 860-byte part (+20) whose +812 is key (0 when there is none).
DECOMP(002E2740, Session_PartDone2E2740) {
    const u32 s = arg<u32>(ctx, 0);
    const s32 key = arg<s32>(ctx, 1);
    const u32 w = load<u32>(kCurrentWorld);
    if (key == 0) {
        ret(ctx, part_done(w, load<u32>(s + 36)));
        return;
    }
    const u32 p = load<u32>(w + kWorldPartObj);
    if (p != 0 && (load<u32>(p + 5572) & 0x08000000u) == 0 && static_cast<s32>(load<u32>(p + 5540)) == key) {
        ret(ctx, 1);
        return;
    }
    const u32 n = load<u32>(s + 24);
    for (u32 i = 0; i < n; ++i) {
        const u32 e = load<u32>(s + 20) + 860 * i;
        if (static_cast<s32>(load<u32>(e + 812)) == key) {
            ret(ctx, part_done(w, e));
            return;
        }
    }
    ret(ctx, 0);
}
