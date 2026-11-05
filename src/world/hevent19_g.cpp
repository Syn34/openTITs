// World_HandleEvent callees, part 19g: animation set layer lookups (up the
// set's parent chain) and the trophy unlock request.
#include "hevent19.h"

using namespace sly4;
using namespace sly4::hevent;
using namespace sly4::hevent19;

namespace {
// Set: +0 parent Set*, +4 u32 (copied into a description's +32 / +72),
// +8 layer map (by u32 key), +36 u32 context for the layer callbacks.
constexpr u32 kSetParent = 0, kSet4 = 4, kSetLayers = 8, kSetCtx = 36;
constexpr u32 kSortedMapFind75E570 = 0x0075E570;  // (map, const u32* key) -> value slot (symbols.txt)
// Layer: +0 s16 type (20-byte entries at 0x00F65F88, +16 query OPD), +8
// u32 key. Description: +8 Layer*, +32 / +72 u32.
constexpr u32 kLayerType = 0, kLayerKey = 8, kLayerTypes = 0x00F65F88, kLayerTypeSize = 20, kTypeQuery = 16;
constexpr u32 kDescLayer = 8, kDesc32 = 32, kDesc72 = 72;

// Trophy manager: +8 u8 ready, +24 request queue, +40 s32 queued, +48 s32
// capacity, +60 lock, +164 u32 last queued count.
constexpr u32 kTrReady = 8, kTrQueue = 24, kTrQueued = 40, kTrQueueCap = 48, kTrLock = 60, kTrLastCount = 164;
constexpr u32 kLock = 0x005E6CC0, kUnlock = 0x005E6C68;  // (lock*) (name?)
constexpr u32 kQueuePush = 0x0079BD1C;                   // (out*, Queue*, const s32* value) (name?)
}  // namespace

// Layer* Set_FindLayer7606C0(Set* s, u32 key, Set** out) (name?): the
// layer with key in s's map or, failing that, its parent's (recursively);
// *out = the set that had it. 0 when no set in the chain has it.
DECOMP(007606C0, Set_FindLayer7606C0) {
    const u32 s = arg<u32>(ctx, 0), key = arg<u32>(ctx, 1), out = arg<u32>(ctx, 2);
    Stack frame(ctx, 16);  // the key word (r1 +112)
    store<u32>(frame.addr(), key);
    if (const u32 m = load<u32>(s + kSetLayers); m != 0) {
        const u32 slot = static_cast<u32>(call(ctx, kSortedMapFind75E570, {m, frame.addr()}));
        if (const u32 layer = load<u32>(slot); layer != 0) {
            store<u32>(out, s);
            return ret(ctx, layer);
        }
    }
    const u32 parent = load<u32>(s + kSetParent);
    if (parent == 0) return ret(ctx, 0u);
    call(ctx, 0x007606C0, {parent, load<u32>(frame.addr()), out});
}

// void Layer_Query4B6760(Set* s, Layer* l, Desc* d, Info* i, s32 n)
// (name?): fills d / i for l through the layer type's query callback (with
// the set's +36 context) when s or no parent has l in its map; otherwise
// asks the parent (n = -1) and then lets s's +4 override d +32 / +72.
DECOMP(004B6760, Layer_Query4B6760) {
    const u32 s = arg<u32>(ctx, 0), l = arg<u32>(ctx, 1), d = arg<u32>(ctx, 2), info = arg<u32>(ctx, 3);
    const u64 n = ctx.gpr[7];
    bool own = false;
    if (const u32 m = load<u32>(s + kSetLayers); m != 0) {
        Stack frame(ctx, 16);  // the key word (r1 +112)
        store<u32>(frame.addr(), load<u32>(l + kLayerKey));
        own = load<u32>(static_cast<u32>(call(ctx, kSortedMapFind75E570, {m, frame.addr()}))) != 0;
    }
    const u32 parent = load<u32>(s + kSetParent);
    if (!own && parent != 0) {
        call(ctx, 0x004B6760, {parent, l, d, info, static_cast<u64>(static_cast<s64>(-1))});
        if (const u32 v = load<u32>(s + kSet4); v != 0) {
            store<u32>(d + kDesc32, v);
            store<u32>(d + kDesc72, v);
        }
        return;
    }
    const s32 t = static_cast<s16>(load<u16>(l + kLayerType));
    const u32 opd = load<u32>(kLayerTypes + static_cast<u32>(t) * kLayerTypeSize + kTypeQuery);
    hevent7::opdcall(ctx, opd, {l, load<u32>(s + kSetCtx), d, info, n});
    store<u32>(d + kDescLayer, l);
}

// bool Trophy_Unlock4FB4A0(Trophy* t, s32 number) (name?): queues number
// for unlocking (under the manager's lock) when the manager is ready and
// the queue has room; +164 = the queued count either way. Returns whether
// it was queued.
DECOMP(004FB4A0, Trophy_Unlock4FB4A0) {
    const u32 t = arg<u32>(ctx, 0), number = arg<u32>(ctx, 1);
    if (load<u8>(t + kTrReady) == 0 || load<u8>(t + kTrReady) == 0) return ret(ctx, 0u);
    Stack frame(ctx, 16);  // r1 +112 the number, +116 the push result
    call(ctx, kLock, {t + kTrLock});
    store<u32>(frame.addr(), number);
    u32 queued = 0;
    if (static_cast<s32>(load<u32>(t + kTrQueued)) < static_cast<s32>(load<u32>(t + kTrQueueCap))) {
        call(ctx, kQueuePush, {frame.addr() + 4, t + kTrQueue, frame.addr()});
        queued = 1;
    }
    store<u32>(t + kTrLastCount, load<u32>(t + kTrQueued));
    call(ctx, kUnlock, {t + kTrLock});
    ret(ctx, queued);
}
