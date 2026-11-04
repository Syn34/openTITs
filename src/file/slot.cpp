// ReadCache slot management: finding the slot holding a position, picking
// one to refill, choosing the next prefetch, and starting it.
#include "fsop.h"

using namespace sly4;
namespace op = sly4::fsop;

namespace {

// ReadCache: +4 count, +8 slot size, +16 slots of 32 bytes
// { u8* buffer; u32; s64 start (+8); s64 end (+16); FsOp* op (+24); u32 }.
constexpr u32 kCount = 4, kSlotSize = 8;
constexpr u32 kFilePosOffset = 0, kFileSize = 288, kFileHandle = 280;

u32 slot_ea(u32 cache, s32 i) { return cache + 16 + static_cast<u32>(i) * 32; }
u64 start_of(u32 s) { return load<u64>(s + 8); }
u64 end_of(u32 s) { return load<u64>(s + 16); }

// (x + 1) mod count, computed as the original does (unsigned divide).
s32 next_index(u32 cache, s32 x) {
    const u32 count = field<u32>(cache, kCount);
    const u32 q = count != 0 ? static_cast<u32>(x + 1) / count : 0;  // divwu by 0 is undefined on the PPU
    return static_cast<s32>(static_cast<u32>(x) - q * count + 1);
}

// Drops a slot's FsOp: cancel if still busy, sync, destroy, free.
void drop_op(PPUContext& ctx, u32 s) {
    if (load<u32>(s + 24) == 0) return;
    if (static_cast<s32>(call(ctx, op::fn::FsOp_GetError, {load<u32>(s + 24)})) == op::kErrBusy)
        call(ctx, op::fn::FsOp_Cancel, {load<u32>(s + 24)});
    call(ctx, op::fn::FsOp_Sync, {load<u32>(s + 24)});  // inlined in the original
    const u32 o = load<u32>(s + 24);
    if (o != 0) {
        call(ctx, op::fn::FsOp_Destruct, {o});
        call(ctx, op::fn::Mem_FreePool, {o, 0, 0});
    }
    store<u32>(s + 24, 0);
}

// The first slot with no data, or 0.
s32 free_slot(u32 cache) {
    const u32 count = field<u32>(cache, kCount);
    for (u32 i = 0; i < count; ++i)
        if (start_of(slot_ea(cache, static_cast<s32>(i))) == ~0ull) return static_cast<s32>(i);
    return 0;
}

}  // namespace

// s32 Cache_Find(ReadCache* cache, s32 pos, bool wait)
// The slot whose [start, end) holds `pos` (taken as a signed 32-bit value),
// or -1. With `wait`, a slot whose prefetch is still in flight is waited
// for first; its end becomes start + bytes read (or the slot is emptied if
// the read failed).
DECOMP(00568A28, Cache_Find) {
    const u32 cache = arg<u32>(ctx, 0);
    const s64 pos = static_cast<s64>(arg<s32>(ctx, 1));
    const bool wait = arg<u8>(ctx, 2) != 0;
    if (static_cast<s32>(field<u32>(cache, kCount).get()) <= 0) return ret(ctx, -1);
    for (s32 i = 0; i < static_cast<s32>(field<u32>(cache, kCount).get()); ++i) {
        const u32 s = slot_ea(cache, i);
        if (pos < static_cast<s64>(start_of(s)) || !(pos < static_cast<s64>(end_of(s)))) continue;
        const u32 o = load<u32>(s + 24);
        if (o == 0 || !wait) return ret(ctx, i);
        call(ctx, op::fn::FsOp_Sync, {o});  // inlined in the original
        const s32 err = static_cast<s32>(call(ctx, op::fn::FsOp_GetError, {load<u32>(s + 24)}));
        const u64 got = call(ctx, op::fn::FsOp_GetResult, {load<u32>(s + 24)});
        drop_op(ctx, s);
        if (err != 0) {
            store<u64>(s + 8, ~0ull);
            store<u64>(s + 16, ~0ull);
            continue;
        }
        const u64 start = start_of(s);
        const u64 end = start + got;
        store<u64>(s + 16, end);
        if (pos < static_cast<s64>(start)) continue;
        if (pos < static_cast<s64>(end)) return ret(ctx, i);
    }
    ret(ctx, -1);
}

// s32 Cache_NextPrefetch(ReadCache* cache, u64 pos, s32 hint, u64* out)
// The slot to prefetch into next and (in *out) the position to read:
// after the slot holding `pos` (-1 if the next slot already continues it),
// else after `hint`, else the first empty slot.
DECOMP(00568D30, Cache_NextPrefetch) {
    const u32 cache = arg<u32>(ctx, 0);
    const u64 pos = ctx.gpr[4];
    const s32 hint = arg<s32>(ctx, 2);
    const u32 out = arg<u32>(ctx, 3);
    const s32 s = static_cast<s32>(call(ctx, 0x00568A28, {cache, pos, 0}));
    if (out != 0) store<u64>(out, pos);
    if (s >= 0) {
        if (out != 0) store<u64>(out, end_of(slot_ea(cache, s)));
        s32 next = next_index(cache, s);
        if (end_of(slot_ea(cache, s)) == start_of(slot_ea(cache, next))) next = -1;
        return ret(ctx, next);
    }
    if (hint >= 0) return ret(ctx, next_index(cache, hint));
    ret(ctx, free_slot(cache));
}

// s32 Cache_PickSlot(ReadCache* cache, u64 pos, s32 hint)
// Chooses a slot to refill for `pos` (after `hint`, after the slot holding
// `pos`, or the first empty one; 0 if none), drops its prefetch and empties it.
DECOMP(00568E74, Cache_PickSlot) {
    const u32 cache = arg<u32>(ctx, 0);
    const u64 pos = ctx.gpr[4];
    const s32 hint = arg<s32>(ctx, 2);
    s32 pick;
    if (hint >= 0) {
        pick = next_index(cache, hint);
    } else {
        const s32 s = static_cast<s32>(call(ctx, 0x00568A28, {cache, pos, 0}));
        if (s >= 0) {
            pick = next_index(cache, s);
            if (end_of(slot_ea(cache, s)) == start_of(slot_ea(cache, pick))) pick = -1;
        } else {
            pick = free_slot(cache);  // (the original also tests hint >= 0 again here)
        }
        if (pick < 0) pick = 0;
    }
    const u32 s = slot_ea(cache, pick);
    drop_op(ctx, s);
    store<u64>(s + 8, ~0ull);
    store<u64>(s + 16, ~0ull);
    ret(ctx, pick);
}

// bool Cache_Prefetch(ReadCache* cache, FsFile* file, u64 pos, s32 slot)
// Drops whatever the slot was doing and starts an asynchronous read of up
// to one slot of data at `pos` into it.
DECOMP(007A5684, Cache_Prefetch) {
    const u32 cache = arg<u32>(ctx, 0), file = arg<u32>(ctx, 1);
    const u64 pos = ctx.gpr[5];
    const s32 slot = arg<s32>(ctx, 3);
    const u32 s = cache + static_cast<u32>(slot) * 32 + 16;
    u64 want = field<u64>(file, kFileSize) - pos;
    const u64 slot_size = field<u32>(cache, kSlotSize);
    if (!(want < slot_size)) want = slot_size;
    const u32 n = static_cast<u32>(want);

    if (load<u32>(s + 24) != 0) {
        const u32 fop = load<u32>(load<u32>(s + 24));
        bool cancel = load<u32>(fop + op::kFiosOpDone) == 0;  // read after an isync
        if (!cancel) {
            Stack frame(ctx, 8);
            store<u32>(frame.addr(), fop);
            cancel = static_cast<s32>(call(ctx, op::fn::fios_Wait, {1, frame.addr(), 0})) == op::kErrBusy;
        }
        if (cancel) {
            const u32 f = load<u32>(load<u32>(s + 24));
            call(ctx, op::fn::fios_Cancel, {load<u32>(f + op::kFiosOpScheduler), f});
        }
        call(ctx, op::fn::FsOp_Sync, {load<u32>(s + 24)});
        const u32 o = load<u32>(s + 24);
        if (o != 0) {
            call(ctx, op::fn::FsOp_Release, {o});
            const u32 f = load<u32>(o);
            if (f != 0) call(ctx, op::fn::fios_Free, {load<u32>(f + op::kFiosOpScheduler), f});
            call(ctx, op::fn::Mem_FreeFrom, {o, 0});
        }
        store<u32>(s + 24, 0);
    }
    store<u64>(s + 8, pos);
    const u32 handle = field<u32>(file, kFileHandle), buffer = load<u32>(s);
    store<u64>(s + 16, pos + n);
    const u32 o = static_cast<u32>(call(ctx, op::fn::Fs_ReadAsync, {handle, buffer, n, pos, 1, 0, 0}));
    store<u32>(s + 24, o);
    if (o != 0) return ret(ctx, 1u);
    store<u64>(s + 8, ~0ull);
    store<u64>(s + 16, ~0ull);
    ret(ctx, 0u);
}
