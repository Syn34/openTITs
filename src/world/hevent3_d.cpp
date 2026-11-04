// World_HandleEvent callees, part 3: helpers and 79..81 instructions.
#include "hevent.h"

using namespace sly4;
using namespace sly4::hevent;

namespace sly4 {
namespace hevent3d {
namespace dfn {
constexpr u32 memmove = 0x0071C3F0;
constexpr u32 String_Format = 0x005534D0;    // (fmt, ...) -> rotating static buffer
constexpr u32 String_Dup = 0x00556038;       // (const char*) -> pooled copy (name?)
constexpr u32 PtrVector_Reserve = 0x00551718; // (PtrVector*, u32 capacity)
}  // namespace dfn

constexpr u32 kJobQueueMutex = 0x0116B580;   // sys_lwmutex_t
constexpr u32 kJobQueue = 0x0116B600;
constexpr u32 kJobQueueLockTimeoutUs = 100000;

// Streaming .ftd files.
constexpr u32 kFtdNames = 0x00AD940C;        // const char*[26] ("Bentley_Animation", ...)
constexpr u32 kFtdNameCount = 26;
constexpr u32 kFmtFtdPath = 0x00AD947C;      // "%s/%s%s"
constexpr u32 kStrStreaming = 0x00AD9484;    // "Streaming"
constexpr u32 kStrFtd = 0x00AD9494;          // ".ftd"
constexpr u32 kStreamFlag = 0x010687C0;      // u8 (0x01068778 + 72): streaming from .ftd files (name?)
constexpr u32 kFtdPaths = 0x0106870C;        // PtrVector: +0 data, +4 u16 capacity, +8 u16 count, +10 u16 high water

// Group_ResolveLod: the global object whose +16 -> +64 is the index used (name?)
constexpr u32 kLodSource = 0x010ABD58;

// Emitter pool (game/frame.h).
constexpr u32 kAudioReady = 0x010AEC60;      // u8
constexpr u32 kEmitters = 0x010B0200;
constexpr u32 kPoolFirst = 8, kPoolLast = 12, kPoolUsed = 16, kPoolNodes = 28;
constexpr u32 kNodeSize = 272, kNodeNext = 4, kNodeSerial = 256;
constexpr u32 kAudioKeyMap = 0x010AFEF8;     // sorted map, u16 count at +2
}  // namespace hevent3d
}  // namespace sly4

using namespace sly4::hevent3d;

// s32 JobQueue_Lock(): sys_lwmutex_lock(0x0116B580, 100000)
DECOMP(0056DFD4, JobQueue_Lock) {
    call(ctx, imp::sys_lwmutex_lock, {kJobQueueMutex, kJobQueueLockTimeoutUs});
}

// JobQueue* JobQueue_Get(): 0x0116B600
DECOMP(0056E038, JobQueue_Get) { ret(ctx, kJobQueue); }

// s32 JobQueue_Unlock(): sys_lwmutex_unlock(0x0116B580)
DECOMP(0056E008, JobQueue_Unlock) { call(ctx, imp::sys_lwmutex_unlock, {kJobQueueMutex}); }

// void Material_CopyAnim(AglMaterialAnim* dst, const AglMaterialAnim* src):
// +0 word, +4 byte, then 3 layers of 48 bytes at +8: f32 +0..+28 and +36..+44
// copied, and of the word at +32 the top 20 bits (in three parts, src re-read
// each time); dst's low 12 bits stay (name?)
DECOMP(00508FBC, Material_CopyAnim) {
    const u32 dst = arg<u32>(ctx, 0), src = arg<u32>(ctx, 1);
    store<u32>(dst, load<u32>(src));
    store<u8>(dst + 4, load<u8>(src + 4));
    u32 s = 0, d = 0, w = 0, last = 0;
    for (u32 k = 0; k < 3; ++k) {
        s = src + 8 + k * 48;
        d = dst + 8 + k * 48;
        for (u32 j = 0; j < 24; j += 4) store<u32>(d + j, load<u32>(s + j));
        const u32 f24 = load<u32>(s + 24);
        w = load<u32>(d + 32) & 0x0FFFFFFFu;
        store<u32>(d + 24, f24);
        store<u32>(d + 28, load<u32>(s + 28));
        w |= load<u32>(s + 32) & 0xF0000000u;
        store<u32>(d + 32, w);
        w = (w & ~0x0FF00000u) | (load<u32>(s + 32) & 0x0FF00000u);
        store<u32>(d + 32, w);
        last = load<u32>(s + 32);
        w = (w & ~0x000FF000u) | (last & 0x000FF000u);
        store<u32>(d + 32, w);
        for (u32 j = 36; j < 48; j += 4) store<u32>(d + j, load<u32>(s + j));
    }
    // Registers as the original leaves them (its only caller, Material_CopyFrom,
    // reads none of them).
    ctx.gpr[3] = src + 144;
    ctx.gpr[4] = dst + 144;
    ctx.gpr[5] = s;
    ctx.gpr[6] = d;
    ctx.gpr[7] = w;
    ctx.gpr[8] = last;
    ctx.fpr[1] = static_cast<f64>(load<f32>(s + 44));
    ctx.ctr = 0;
}

// void Streaming_AddFtdPaths(): when streaming is on (u8 0x010687C0), adds
// String_Dup("Streaming/<name>.ftd") for the 26 names at 0x00AD940C to the
// vector at 0x0106870C (doubling its capacity when full) (name?)
DECOMP(002E9D48, Streaming_AddFtdPaths) {
    Stack frame(ctx, 104);  // the name table copied to r1+112
    const u32 names = frame.addr();
    call(ctx, dfn::memmove, {names, kFtdNames, kFtdNameCount * 4});
    if (load<u8>(kStreamFlag) == 0) return;
    u32 high = 0;
    for (u32 i = 0; i < kFtdNameCount; ++i) {
        call(ctx, dfn::String_Format, {kFmtFtdPath, kStrStreaming, load<u32>(names + i * 4), kStrFtd});
        const u32 path = static_cast<u32>(call(ctx, dfn::String_Dup));  // r3 = the formatted string
        u32 n = load<u16>(kFtdPaths + 8);
        const u32 cap = load<u16>(kFtdPaths + 4);
        if (n == cap) {
            call(ctx, dfn::PtrVector_Reserve, {kFtdPaths, (cap << 1) & 0x1FFFEu});
            n = load<u16>(kFtdPaths + 8);
        }
        store<u32>(load<u32>(kFtdPaths) + ((n << 2) & 0x3FFFCu), path);
        u32 cnt = load<u16>(kFtdPaths + 8) + 1;
        high = load<u16>(kFtdPaths + 10);
        store<u16>(kFtdPaths + 8, static_cast<u16>(cnt));
        if (static_cast<s32>(high) > static_cast<s32>(cnt & 0xFFFF)) cnt = high;
        store<u16>(kFtdPaths + 10, static_cast<u16>(cnt));
    }
    ret(ctx, high);  // r3 = the last high-water mark read
}

// u32 Group_ResolveLod(Group* g): +0 = 1 (and left set, unlike
// Group_UpdateAll); vfunc +12() on every non-null object of the first list
// (+12 entries {?, Object*}, +6 u16 count); then for every object e of the
// second (+36, +30): e->+12 = (e->+0 -> +12)[*(*(0x010ABD58 + 16) + 64)], and
// when that is 0, the +36 of e->+36 nodes linked by +72 from e->+32 are summed
// into it. Returns 8 * the second count (name?)
DECOMP(00451CC8, Group_ResolveLod) {
    const u32 g = arg<u32>(ctx, 0);
    store<u8>(g, 1);
    for (u32 i = 0; i < load<u16>(g + 6); ++i) {
        const u32 e = load<u32>(load<u32>(g + 12) + i * 8 + 4);
        if (e != 0) vcall(ctx, e, 12, {e});
    }
    u32 i = 0;
    for (; i < load<u16>(g + 30); ++i) {
        const u32 e = load<u32>(load<u32>(g + 36) + i * 8 + 4);
        const u32 idx = load<u32>(load<u32>(kLodSource + 16) + 64);
        u32 v = load<u32>(load<u32>(load<u32>(e) + 12) + (idx << 2));
        store<u32>(e + 12, v);
        if (v != 0) continue;
        u32 node = load<u32>(e + 32);
        for (s32 n = load<s32>(e + 36); n > 0; --n) {
            v += load<u32>(node + 36);
            store<u32>(e + 12, v);
            node = load<u32>(node + 72);
        }
    }
    ret(ctx, i * 8);
}

// u32 Emitters_StopAll(): with audio up, Emitter_Stop(handle, 0) for every
// emitter in the pool's list (the next handle is taken before each stop), then
// the key map's count (0x010AFEF8 +2) = 0 (name?)
// Handles are {u16 index, u16 serial} with serial = (node +256 >> 15) & 0xFFFF.
DECOMP(00449950, Emitters_StopAll) {
    if (load<u8>(kAudioReady) == 0) return ret(ctx, 0u);
    Stack frame(ctx, 16);  // r1+112 current handle, r1+116 first handle
    u32 h = 0;
    if (load<u32>(kEmitters + kPoolUsed) != 0) {
        const u32 first = load<u32>(kEmitters + kPoolFirst);  // the node address uses all 32 bits
        const u32 node = load<u32>(kEmitters + kPoolNodes) + first * kNodeSize;
        h = ((first & 0xFFFF) << 16) | ((load<u32>(node + kNodeSerial) >> 15) & 0xFFFF);
    }
    store<u32>(frame.addr() + 4, h);
    store<u32>(frame.addr(), h);
    while ((h & 0xFFFF) != 0) {
        const u32 index = h >> 16;
        u32 next = 0;
        if (static_cast<s32>(index) != load<s32>(kEmitters + kPoolLast)) {
            const u32 n = load<u32>(load<u32>(kEmitters + kPoolNodes) + index * kNodeSize + kNodeNext);
            next = ((n << 16) & 0xFFFF0000u) | ((load<u32>(load<u32>(kEmitters + kPoolNodes) + n * kNodeSize + kNodeSerial) >> 15) & 0xFFFF);
        }
        call(ctx, hfn::Emitter_Stop, {h, 0});
        h = next;
        store<u32>(frame.addr(), h);
    }
    store<u16>(kAudioKeyMap + 2, 0);
    ret(ctx, kAudioKeyMap);
}
