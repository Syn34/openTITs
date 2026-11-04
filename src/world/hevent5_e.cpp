// World_HandleEvent callees, part 5: 154..155 instructions.
#include "hevent.h"

using namespace sly4;
using namespace sly4::hevent;

namespace sly4 {
namespace hevent5e {
namespace efn {
constexpr u32 Emitter_Stop = 0x00443424;      // (handle, bool fade)
constexpr u32 SmallFree = 0x00552420;         // (p, size)
constexpr u32 Gpu_Submit7E5028 = 0x007E5028;  // (dev, const Desc*, 0, dev) (name?)
}  // namespace efn

constexpr u32 kAudioReady = 0x010AEC60;       // u8
constexpr u32 kAudioMuted = 0x010B0950 + 72;  // u8 (tuning 0x010B0950 +72): stop instead (name?)
constexpr u32 kMusicHandle = 0x010AFEA4;      // u32 emitter handle (hevent.h kEmitterHandle10AFEA4)
constexpr u32 kMusicNode = 0x00F65B74;        // u32 RefNode*: the current one (name?)
constexpr u32 kRefListHead = 0x00F65B7C;      // u32 first RefNode (24 bytes: +4 next, +8 prev, +20 refs)
constexpr u32 kMusicArgA = 0x010AFEA8, kMusicArgB = 0x010AFEA9;  // u8 (name?)
constexpr u32 kRefNodeSize = 24;

constexpr s32 kAllocatorR13 = -28332;         // r13-relative u32: object whose +44 is an allocator (name?)
constexpr u32 kGpuDevice = 0x00F64C94;        // u32 (name?)
constexpr u32 kGpuQueue = 0x00F64C90;         // u32 object: vfunc +16(q, dev, 21), +20(q) (name?)
constexpr u32 kUploadHeap = 0x011DF31C;       // object: vfunc +20(heap, addr, s32 size) (name?)
constexpr u32 kUploadIdle = 0x80000000u;
}  // namespace hevent5e
}  // namespace sly4

using namespace sly4::hevent5e;

// u32 Music_SetNode449E48(RefNode* n, u8 a, u8 b) (name?): with audio up: when
// the tuning flag 0x010B0998 is set, stops the music emitter (Emitter_Stop(h,
// 1)); otherwise releases the current node (0x00F65B74: refs - 1, unlinked
// from 0x00F65B7C and freed at 0), makes n current (refs + 1) and stores a, b
// at 0x010AFEA8/9. (Two always-null locals of the original get vfunc +40 calls
// on paths that are never taken; they are left out.)
DECOMP(00449E48, Music_SetNode449E48) {
    const u32 n = arg<u32>(ctx, 0);
    const u32 a = arg<u32>(ctx, 1), b = arg<u32>(ctx, 2);
    if (load<u8>(kAudioReady) == 0) return;
    if (load<u8>(kAudioMuted) != 0) {
        call(ctx, efn::Emitter_Stop, {load<u32>(kMusicHandle), 1});
        return;
    }
    const u32 cur = load<u32>(kMusicNode);
    if (cur != 0) {
        const s32 refs = load<s32>(cur + 20);
        if (refs > 0) {
            store<s32>(cur + 20, refs - 1);
            if (refs - 1 == 0) {
                const u32 prev = load<u32>(cur + 8);
                if (prev != 0) store<u32>(prev + 4, load<u32>(cur + 4));
                else store<u32>(kRefListHead, load<u32>(cur + 4));
                const u32 next = load<u32>(cur + 4);
                if (next != 0) store<u32>(next + 8, load<u32>(cur + 8));
                call(ctx, efn::SmallFree, {cur, kRefNodeSize});
            }
        }
    }
    store<u32>(kMusicNode, n);
    if (n != 0) store<u32>(n + 20, load<u32>(n + 20) + 1);
    store<u8>(kMusicArgA, static_cast<u8>(a));
    store<u8>(kMusicArgB, static_cast<u8>(b));
    ret(ctx, 0x010B0000u);  // r3 as the original leaves it (the base of 0x010AFEA8)
}

// void Upload_Flush34688C(Upload* u) (name?): with +60 bytes pending, a 16-byte
// block from the r13 allocator (+44 vfunc +8(alloc, 16), zeroed) gets the
// count of 64-byte units ((size - 1) / 64 + 1; 1 for 0) at +0 (written even
// when the allocation failed, as the original does), and a descriptor {1, 3,
// 1, 48, -1, 0, block, 0, (u +4) +112, 64, +56, +60, 0, 0, (u +4) +88} is
// submitted with Gpu_Submit7E5028, the queue kicked (vfunc +16(q, dev, 21),
// vfunc +20(q)) and the block freed (vfunc +12(alloc, block, 16)). Then +60 =
// 0; unless +64 has the idle bit, vfunc +20(heap, +56, +64 << 6) on the upload
// heap 0x011DF31C; +56 = 0, +64 = idle.
DECOMP(0034688C, Upload_Flush34688C) {
    const u32 u = arg<u32>(ctx, 0);
    if (load<s32>(u + 60) > 0) {
        const u32 holder = static_cast<u32>(ctx.gpr[13]) + static_cast<u32>(kAllocatorR13);
        const u32 al = load<u32>(load<u32>(holder) + 44);
        const u32 blk = static_cast<u32>(vcall(ctx, al, 8, {al, 16}));
        if (blk != 0) {
            for (u32 k = 0; k < 16; ++k) store<u8>(blk + k, 0);
        }
        const u32 src = load<u32>(u + 4);
        const s32 size = load<s32>(u + 60);
        const u32 addr = load<u32>(u + 56);
        const u32 w112 = load<u32>(src + 112);
        Stack frame(ctx, 48);  // the descriptor at r1+112..159
        const u32 d = frame.addr();
        store<u8>(d + 0, 1);
        store<u8>(d + 1, 3);
        store<u16>(d + 4, 48);
        store<u16>(d + 6, 0xFFFF);
        store<u32>(d + 16, 0);
        store<u8>(d + 2, 1);
        store<u32>(d + 20, w112);
        store<u32>(d + 12, blk);
        store<u32>(d + 24, 64);
        store<u32>(d + 8, 0);
        store<u32>(d + 28, addr);
        store<u32>(d + 32, static_cast<u32>(size));
        s32 units = 1;
        if (size != 0) units = (size - 1) / 64 + 1;
        store<u32>(blk, static_cast<u32>(units));  // unchecked: a failed allocation writes to 0
        store<u32>(d + 36, 0);
        store<u32>(d + 40, 0);
        store<u32>(d + 44, load<u32>(src + 88));
        const u32 dev = load<u32>(kGpuDevice);
        call(ctx, efn::Gpu_Submit7E5028, {dev, d, 0, dev});
        const u32 q = load<u32>(kGpuQueue);
        vcall(ctx, q, 16, {q, load<u32>(kGpuDevice), 21});
        const u32 q2 = load<u32>(kGpuQueue);
        vcall(ctx, q2, 20, {q2});
        if (blk != 0) {
            const u32 al2 = load<u32>(load<u32>(static_cast<u32>(ctx.gpr[13]) + static_cast<u32>(kAllocatorR13)) + 44);
            vcall(ctx, al2, 12, {al2, blk, 16});
        }
    }
    store<u32>(u + 60, 0);
    const u32 f = load<u32>(u + 64);
    if (!(f & kUploadIdle)) {
        const u32 opd = load<u32>(load<u32>(kUploadHeap) + 20);
        ctx.gpr[6] = opd;
        ctx.gpr[7] = load<u32>(opd);
        call_ptr(ctx, opd, {kUploadHeap, load<u32>(u + 56), static_cast<u64>(static_cast<s64>(static_cast<s32>(f << 6)))});
    }
    store<u32>(u + 56, 0);
    store<u32>(u + 64, kUploadIdle);
    ret(ctx, 0);
}
