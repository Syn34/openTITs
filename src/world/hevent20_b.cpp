// World_HandleEvent callees, part 20b: small leaves under the level enter /
// travel paths (track pool refresh, a mode flag, timed callbacks, a user
// block reset, ref-counted slot setters and the pool reset under its lock).
#include "hevent19.h"

using namespace sly4;
using namespace sly4::hevent;

namespace {
constexpr u32 kMemset = 0x00726E54;               // (dst, value, size) (symbols.txt memset)
constexpr u32 kTrackPoolUpdate390724 = 0x00390724; // (Pool*) (name?)
constexpr u32 kBuffer_Sync566318 = 0x00566318;   // (Buffer*, u32 data) (name?)
constexpr u32 kObjSetMode368870 = 0x00368870;    // (Obj*, u32 flags, 0, 0; f32) (name?)
constexpr u32 kCallback441AC0 = 0x00441AC0;      // (u32 a, u32* b) -> handle (symbols.txt sub_00441AC0)
constexpr u32 kSysTimeGetSystemTime = 0x00ABF2C4; // sysPrxForUser import stub
constexpr u32 kSysLwmutexLock = 0x00ABF0E4, kSysLwmutexUnlock = 0x00ABF104;  // import stubs
constexpr u32 kPoolCreate = 0x00565AA8;          // (a, b, c, 1) (symbols.txt Pool_Create)
constexpr u32 kSlotRefresh108480 = 0x00108480;   // (Obj*) (name?)
constexpr u32 kSlotRefresh10DA30 = 0x0010DA30;   // (Obj*) (name?)

constexpr u32 kTrackPoolPtr = 0x00F65798;        // u32 track object pool (as hevent19.h kTrackPool)
constexpr u32 kBufferPtr = 0x00F65038;           // u32 Buffer*: +8 data, +24 base, +28 / +32 cursor, +0x18024 s16 index
constexpr u32 kModeF1 = 0x003689D0;              // f32 -1.0 (text constant)
constexpr u32 kCallbackList = 0x00F65B7C;        // u32 first node {+0 handle, +4 next, +12, +16}
constexpr u32 kPoolLock = 0x0106BFC8;            // sys_lwmutex_t guarding the pool below
constexpr u32 kPoolArgA = 0x0106BFE0, kPoolArgB = 0x00F64CA0, kPoolArgC = 0x0106BD68;
constexpr u32 kPoolReady = 0x0106BFE4, kPoolWordA = 0x0106BFEC, kPoolWordB = 0x0106BFF4;
constexpr u32 kLockTimeoutUs = 100000;

// The ref-counted slot setters: old slot's vt +40 (old, 0), slot = x, and x's
// u16 +178 use count incremented. Returns the new count's address in r4.
inline bool set_ref_slot(PPUContext& ctx, u32 o, u32 off, u32 x) {
    const u32 old = load<u32>(o + off);
    if (old != 0) vcall(ctx, old, 40, {old, 0});
    else ctx.gpr[3] = 0;
    store<u32>(o + off, x);
    if (x == 0) return false;
    const u32 c = load<u16>(x + 178) + 1;
    store<u16>(x + 178, static_cast<u16>(c));
    ctx.gpr[3] = x + 178;
    ctx.gpr[4] = c;
    return true;
}
}  // namespace

// void TrackPool_Refresh390BAC() (name?): 0x00390724(track pool), then the
// buffer at 0x00F65038: 0x00566318(b, b +8); b +28 = (s16) b +0x18024 and
// b +32 = b +24 + that x 128. r3 is left at the index.
DECOMP(00390BAC, TrackPool_Refresh390BAC) {
    call(ctx, kTrackPoolUpdate390724, {load<u32>(kTrackPoolPtr)});
    const u32 b = load<u32>(kBufferPtr);
    call(ctx, kBuffer_Sync566318, {b, load<u32>(b + 8)});
    const s32 idx = static_cast<s16>(load<u16>(b + 0x18024));
    const u32 cur = load<u32>(b + 24) + (static_cast<u32>(idx) << 7);
    store<u32>(b + 28, static_cast<u32>(idx));
    store<u32>(b + 32, cur);
    ctx.gpr[4] = cur;
    ctx.gpr[5] = static_cast<u32>(idx) << 7;
    ctx.gpr[3] = static_cast<u64>(static_cast<s64>(idx));
}

// void Obj_EnterMode3689D8(Obj* o) (name?): when o is set and neither +532
// bit 0x02000000 nor 0x01000000 is: +532 |= 0x40000000, then
// 0x00368870(o, flags, 0, 0; -1.0).
DECOMP(003689D8, Obj_EnterMode3689D8) {
    const u32 o = arg<u32>(ctx, 0);
    if (o == 0) {
        ctx.gpr[4] = 0;
        return;
    }
    const u32 w = load<u32>(o + 532);
    ctx.gpr[4] = w;
    if ((w & 0x02000000u) != 0 || (w & 0x01000000u) != 0) {
        ctx.gpr[5] = (w & 0x02000000u) != 0 ? 1 : 0;
        return;
    }
    const u32 nw = w | 0x40000000u;
    store<u32>(o + 532, nw);
    callf(ctx, kObjSetMode368870, {o, nw, 0, 0, kModeF1}, {hevent7::lfs_f64(kModeF1)});
}

// void Callbacks_Bind44BD74() (name?): every node of the list at 0x00F65B7C
// (next +4) without a handle (+0) gets 0x00441AC0(node +16, node +12). The
// original reads the system time before and after (results unused).
DECOMP(0044BD74, Callbacks_Bind44BD74) {
    u32 n = load<u32>(kCallbackList);
    call(ctx, kSysTimeGetSystemTime, {});
    for (; n != 0; n = load<u32>(n + 4)) {
        if (load<u32>(n) != 0) continue;
        const u32 h = static_cast<u32>(call(ctx, kCallback441AC0, {load<u32>(n + 16), n + 12}));
        store<u32>(n, h);
    }
    call(ctx, kSysTimeGetSystemTime, {});
}

// void UserBlock_Reset45C7A8(UserBlock* b): +112 (0x20404 bytes) and
// +0x20474 (2052 bytes) zeroed, +0 bit 0x80 cleared. (symbols.txt name.)
DECOMP(0045C7A8, UserBlock_Reset45C7A8) {
    const u32 b = arg<u32>(ctx, 0);
    call(ctx, kMemset, {b + 112, 0, 0x20404});
    call(ctx, kMemset, {b + 0x20474, 0, 2052});
    const u32 w = load<u32>(b) & ~0x80u;
    store<u32>(b, w);
    ctx.gpr[4] = w;
    ctx.gpr[3] = 0;
}

// void Obj_SetRef11135C(Obj* o, Ref* x) (name?): o +12 = x (old vt +40
// (old, 0), x's +178 count incremented).
DECOMP(0011135C, Obj_SetRef11135C) {
    set_ref_slot(ctx, arg<u32>(ctx, 0), 12, arg<u32>(ctx, 1));
}

// void Obj_SetRef1088E4(Obj* o, Ref* x) (name?): as 0x0011135C with o +20,
// then 0x00108480(o) when x is set.
DECOMP(001088E4, Obj_SetRef1088E4) {
    const u32 o = arg<u32>(ctx, 0);
    if (set_ref_slot(ctx, o, 20, arg<u32>(ctx, 1))) {
        ctx.gpr[5] = ctx.gpr[4];
        ctx.gpr[4] = ctx.gpr[3];
        call(ctx, kSlotRefresh108480, {o, ctx.gpr[4], ctx.gpr[5]});
    }
}

// void Obj_SetRef10DD60(Obj* o, Ref* x) (name?): as 0x0011135C with o
// +592, then 0x0010DA30(o) when x is set.
DECOMP(0010DD60, Obj_SetRef10DD60) {
    const u32 o = arg<u32>(ctx, 0);
    if (set_ref_slot(ctx, o, 592, arg<u32>(ctx, 1))) {
        ctx.gpr[5] = ctx.gpr[4];
        ctx.gpr[4] = ctx.gpr[3];
        call(ctx, kSlotRefresh10DA30, {o, ctx.gpr[4], ctx.gpr[5]});
    }
}

// void Pool_Reset355174() (name?): under the lwmutex at 0x0106BFC8:
// Pool_Create(*0x0106BFE0, *0x00F64CA0, *0x0106BD68, 1), u8 0x0106BFE4 = 1,
// 0x0106BFEC and 0x0106BFF4 = 0.
DECOMP(00355174, Pool_Reset355174) {
    call(ctx, kSysLwmutexLock, {kPoolLock, kLockTimeoutUs});
    call(ctx, kPoolCreate, {load<u32>(kPoolArgA), load<u32>(kPoolArgB), load<u32>(kPoolArgC), 1});
    store<u8>(kPoolReady, 1);
    store<u32>(kPoolWordA, 0);
    store<u32>(kPoolWordB, 0);
    call(ctx, kSysLwmutexUnlock, {kPoolLock});
}
