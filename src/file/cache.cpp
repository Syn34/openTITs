// FsFile's read cache (fixed slots with prefetch) and write buffer.
#include "fsio.h"

using namespace sly4;
namespace io = sly4::fsio;

namespace {

constexpr u32 kCacheBuffer = 0, kCacheCount = 4, kCacheSlotSize = 8;
constexpr u32 kCachePool = 1;
constexpr u32 kStrWrite = 0x00AF481C;  // "%s - Write %d bytes to %x\n"

u32 slot_at(u32 cache, s32 index) { return cache + io::kCacheSlots + static_cast<u32>(index) * sizeof(io::CacheSlot); }

}  // namespace

// void ReadCache_Init(ReadCache* cache, u32 slot_size)
// One allocation of count * slot_size bytes from pool 1, split into the slots.
// (The original inlines Mem_Alloc's lazy set-up before calling
// Mem_AllocPool(16, size, 1, 0); calling Mem_Alloc does the same.)
DECOMP(00568490, ReadCache_Init) {
    const u32 cache = arg<u32>(ctx, 0);
    const u32 slot_size = arg<u32>(ctx, 1);
    const u32 total = slot_size * field<u32>(cache, kCacheCount);
    field<u32>(cache, kCacheSlotSize) = slot_size;
    field<u32>(cache, kCacheBuffer) =
        static_cast<u32>(call(ctx, sly4::fn::Mem_Alloc, {total, kCachePool, 16}));
    for (u32 i = 0; i < field<u32>(cache, kCacheCount); ++i)
        ref<io::CacheSlot>(slot_at(cache, static_cast<s32>(i))).buffer =
            field<u32>(cache, kCacheBuffer) + i * field<u32>(cache, kCacheSlotSize);
}

// void ReadCache_Destruct(ReadCache* cache)
// Cancels / waits for any prefetch in flight, empties the slots and frees
// the buffer.
DECOMP(005682B4, ReadCache_Destruct) {
    const u32 cache = arg<u32>(ctx, 0);
    if (field<u32>(cache, kCacheCount) != 0) {
        for (u32 i = 0; i < field<u32>(cache, kCacheCount); ++i) {
            io::CacheSlot& slot = ref<io::CacheSlot>(slot_at(cache, static_cast<s32>(i)));
            if (slot.op != 0) {
                if (static_cast<s32>(call(ctx, io::fn::FsOp_GetError, {slot.op.get()})) == io::kErrBusy)
                    call(ctx, io::fn::FsOp_Cancel, {slot.op.get()});
                io::op_finish(ctx, slot.op);
                const u32 op = slot.op;
                if (op != 0) io::op_delete(ctx, op);
                slot.op = 0;
            }
            slot.buffer = 0;
            slot.start = ~0ull;
            slot.end = ~0ull;
        }
    }
    const u32 buffer = field<u32>(cache, kCacheBuffer);
    if (buffer != 0) {
        call(ctx, io::fn::Mem_FreePool, {buffer, kCachePool, 0});
        field<u32>(cache, kCacheBuffer) = 0;
    }
}

// u64 ReadCache_Read(ReadCache* cache, FsFile* file, void* buf, u64 size)
// Reads up to `size` bytes at the file's position (which it doesn't move)
// through the cache slots, filling a slot with a synchronous read on a miss
// and starting a prefetch for what comes next. Returns the bytes copied.
DECOMP(007A5840, ReadCache_Read) {
    const u32 cache = arg<u32>(ctx, 0);
    const u32 file = arg<u32>(ctx, 1);
    u32 buf = arg<u32>(ctx, 2);
    u64 n = ctx.gpr[6];
    u64 pos = field<u64>(file, io::kFilePosition);
    const u64 remain = field<u64>(file, io::kFileSize) - pos;
    if (!(n < remain)) n = remain;

    u64 total = 0;
    s32 slot = -1, hint = -1;
    Stack frame(ctx, 16);
    const u32 op_list = frame.addr(), next_pos = frame.addr() + 8;
    store<u64>(next_pos, 0);  // the original zeroes Cache_NextPrefetch's out-parameter
    while (n != 0) {
        if (slot < 0) slot = static_cast<s32>(call(ctx, io::fn::Cache_Find, {cache, pos, 1}));
        if (slot < 0) {
            // Miss: take a slot (dropping any prefetch it has) and fill it.
            slot = static_cast<s32>(call(ctx, io::fn::Cache_PickSlot, {cache, pos, static_cast<u64>(static_cast<s64>(hint))}));
            u64 want = field<u64>(file, io::kFileSize) - pos;
            const u32 slot_size = field<u32>(cache, kCacheSlotSize);
            if (!(want < slot_size)) want = slot_size;
            io::CacheSlot& s = ref<io::CacheSlot>(slot_at(cache, slot));
            if (s.op != 0) {
                const u32 fop = load<u32>(s.op);
                bool cancel = load<u32>(fop + 152) == 0;  // read after an isync in the original
                if (!cancel) {
                    store<u32>(op_list, fop);
                    cancel = static_cast<s32>(call(ctx, io::fn::Fios_Wait, {1, op_list, 0})) == io::kErrBusy;
                }
                if (cancel) {
                    const u32 f = load<u32>(s.op);
                    call(ctx, io::fn::Fios_CancelOp, {load<u32>(f + 156), f});
                }
                call(ctx, io::fn::FsOp_Sync, {s.op.get()});
                const u32 op = s.op;
                if (op != 0) {
                    call(ctx, io::fn::FsOp_Release, {op});
                    const u32 f = load<u32>(op);
                    if (f != 0) call(ctx, io::fn::Fios_FreeOp, {load<u32>(f + 156), f});
                    call(ctx, io::fn::Mem_FreeFrom, {op, 0});
                }
                s.op = 0;
            }
            const u64 got = call(ctx, io::fn::Fs_Read,
                                 {field<u32>(file, io::kFileHandle), s.buffer.get(), static_cast<u32>(want), pos,
                                  io::kDefaultOptions, 0, 0});
            s.start = pos;
            s.end = pos + got;
            if (static_cast<u32>(got) == 0) return ret(ctx, total);
            continue;
        }
        // Hit: copy what this slot has.
        const io::CacheSlot& s = ref<io::CacheSlot>(slot_at(cache, slot));
        u64 take = n;
        const u64 avail = s.end - pos;
        if (avail < take) take = avail;
        call(ctx, io::fn::memmove, {buf, static_cast<u32>(s.buffer + (pos - s.start)), static_cast<u32>(take)});
        n -= take;
        buf += static_cast<u32>(take);
        total += take;
        pos += take;
        hint = slot;
        slot = -1;
    }
    if (static_cast<s64>(pos) < static_cast<s64>(field<u64>(file, io::kFileSize).get())) {
        const s32 next = static_cast<s32>(
            call(ctx, io::fn::Cache_NextPrefetch, {cache, pos, static_cast<u64>(static_cast<s64>(hint)), next_pos}));
        if (next >= 0)
            call(ctx, io::fn::Cache_Prefetch, {cache, file, load<u64>(next_pos), static_cast<u64>(static_cast<s64>(next))});
    }
    ret(ctx, total);
}

// u64 WriteBuffer_Write(WriteBuffer* wb, FsFile* file, const void* buf, u64 size)
// Appends to the 32 KiB buffer, writing it out to the file whenever it
// fills. Returns `size`.
DECOMP(007A5B58, WriteBuffer_Write) {
    const u32 wb = arg<u32>(ctx, 0);
    const u32 file = arg<u32>(ctx, 1);
    u32 buf = arg<u32>(ctx, 2);
    const u64 size = ctx.gpr[6];
    if (size == 0) return ret(ctx, 0u);

    u64 used = load<u64>(wb + 8);
    u32 n = static_cast<u32>(size);
    if (load<u32>(wb + 4) - static_cast<u32>(used) > n) {
        call(ctx, io::fn::memmove, {static_cast<u32>(load<u32>(wb) + used), buf, n});
        store<u64>(wb + 8, load<u64>(wb + 8) + n);
        ctx.gpr[3] = size;
        return;
    }
    while (n != 0) {
        u32 space = load<u32>(wb + 4) - static_cast<u32>(used);
        u64 cur = used;
        if (space != 0) {
            const u32 take = space > n ? n : space;
            call(ctx, io::fn::memmove, {static_cast<u32>(load<u32>(wb) + used), buf, take});
            cur = load<u64>(wb + 8) + take;
            buf += take;
            n -= take;
            used = cur;
            store<u64>(wb + 8, cur);
            space -= take;
        }
        if (space == 0 && static_cast<s64>(cur) > 0) {
            const u32 count = static_cast<u32>(used);
            const u32 data = load<u32>(wb);
            call(ctx, io::fn::String_Format, {kStrWrite, file + io::kFileName, count, data});
            const u64 written = call(ctx, io::fn::Fs_Write,
                                     {field<u32>(file, io::kFileHandle), data, count, field<u64>(file, io::kFilePosition),
                                      io::kDefaultOptions, 0, 0});
            field<u64>(file, io::kFilePosition) += written;
            store<u64>(wb + 8, 0);
            used = 0;
        }
    }
    ctx.gpr[3] = size;
}
