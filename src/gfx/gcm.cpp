// libgcm / RSX start-up.
#include "gcm.h"

#include <atomic>
#include <cstdlib>

using namespace sly4;
namespace gc = sly4::gcm;

namespace {

constexpr u32 kStats = g::mem_stats;  // MemStats (see src/memory/mem.h); fields named by offset
constexpr u32 kConfig = g::config;

constexpr u32 CELL_SYSMODULE_GCM_SYS = 0x10;
constexpr u32 kCommandSize = 0x11000;
constexpr u32 kDefaultIoSize = 0x200000, kDefaultMainSize = 0x1000000, kDefaultLocalPool = 0x80000;
constexpr u32 kMb = 0x100000;
constexpr u32 kVBlankFrequency = 2;
constexpr u32 kVBlankHandler = 0x00F3BCE0;  // OPD
constexpr s32 CELL_VIDEO_OUT_ERROR_VALUE_IS_NOT_SET = static_cast<s32>(0x8002B228);
constexpr u32 kStrScreenSizeFailed = 0x00AF33CC;  // "cellVideoOutGetScreenSize: Failed - Not set in UI?"
constexpr u32 kStrGcmError = 0x00AF2CAC;          // "Error initializing libgcm: %i\n"
constexpr u32 kStderr = 0x00F94668;

// Globals touched here.
constexpr u32 kVBlankCounterSrc = 0x011972EC, kVBlankCounter = 0x010F10A0;
constexpr u32 kModeByte = 0x00F671DC, kLabel64 = 0x00F671D8;
constexpr u32 kCommandBuffer = 0x010F0E08, kUnk010F0E1C = 0x010F0E1C, kUnk010F0E04 = 0x010F0E04;
constexpr u32 kFormats = 0x010F1020, kFormatSource = 0x00F67924;  // 12-byte entries, terminated by [4] == 0
constexpr u32 kLocalPool = 0x010F1338;  // { u32 size; u32 0; u32 a; u32 b; u32 size-512; u32 size; }
constexpr u32 kUnk01151880 = 0x01151880, kAtomic01151900 = 0x01151900, kUnk01151980 = 0x01151980;
constexpr u32 kJobChain = 0x010F1400;   // CellSpursJobChain; attr at +272, guard at +896, commands at +1024..
constexpr u32 kJobBuffer2 = 0x01121880;
constexpr u32 kStrJobChain = 0x00B24248;  // "DoubleBufferedJobList - Job Chain"
constexpr u32 kGfxObj = 0x01151A80;
constexpr u32 kSurfaceInfo = 0x01163FE8;  // { w, h, 0, bpp/8 }
constexpr u32 kSurfaceSrc = 0x00F6729C;
constexpr u32 kSurfaceHeap = 0x01163FC8, kSurfaceSize = 0x01163FD4;
constexpr u32 kQueueFlag = 0x01163F80, kQueue = 0x01163F84, kQueueBuffer = 0x01163FB8;
constexpr u32 kQueueUnk1 = 0x01163FBC, kQueueUnk2 = 0x01163FC0;
constexpr u32 kQueueBufferSize = 22528, kQueueLabel = 251;
constexpr u32 kLabels = 0x010F0FA0;  // u32* [21] for labels 126..146
constexpr u32 kFirstLabel = 126, kLabelCount = 21;

// A size from g::config: 0 means `fallback`, negative means 0.
u32 config_size(u32 offset, u32 fallback) {
    const s32 v = static_cast<s32>(field<u32>(kConfig, offset).get());
    return v == 0 ? fallback : v > 0 ? static_cast<u32>(v) : 0;
}

u32 label(PPUContext& ctx, u32 index) {
    return static_cast<u32>(call(ctx, gc::imp::cellGcmGetLabelAddress, {index}));
}

[[noreturn]] void fail(PPUContext& ctx) {
    call(ctx, gc::fn::exit, {static_cast<u64>(-1)});
    std::abort();  // the original spins forever after exit(), which doesn't return
}

}  // namespace

// bool Gcm_Init()
// Initialises libgcm (command buffer, I/O and RSX memory, VBlank), the
// screen parameters, the render command buffer, the SPURS job chain
// "DoubleBufferedJobList - Job Chain", the surface heap, a job queue and
// the RSX labels the renderer polls. Exits the process if libgcm can't
// start. Returns true.
DECOMP(0053AC38, Gcm_Init) {
    call(ctx, gc::imp::sys_ppu_thread_get_id, {kStats + 144});
    store<u64>(kStats + 152, load<u64>(kStats + 144));
    if (static_cast<s32>(call(ctx, gc::fn::Sysmodule_Require, {CELL_SYSMODULE_GCM_SYS})) != 1) fail(ctx);
    call(ctx, gc::imp::cellGcmInitDefaultFifoMode, {1});

    const u32 io_size = config_size(472, kDefaultIoSize);
    const u32 io = static_cast<u32>(call(ctx, gc::fn::Mem_AllocPool, {kMb, io_size, 0, 0}));
    const s32 rc = static_cast<s32>(call(ctx, gc::fn::Gcm_InitBody, {kCommandSize, io_size, io}));
    if (rc != 0) {
        call(ctx, gc::fn::fprintf, {kStderr, kStrGcmError, static_cast<u64>(static_cast<s64>(rc))});
        fail(ctx);
    }
    call(ctx, gc::imp::cellGcmGetConfiguration, {kStats + 32});
    call(ctx, gc::fn::RsxLocal_Init, {load<u32>(kStats + 32), load<u32>(kStats + 40)});
    const u32 main_size = config_size(464, kDefaultMainSize);
    const u32 main = static_cast<u32>(call(ctx, gc::fn::Mem_AllocPool, {kMb, main_size, 0, 0}));
    call(ctx, gc::fn::RsxMain_Init, {main, main_size});
    store<u32>(kVBlankCounter, load<u32>(kVBlankCounterSrc));
    call(ctx, gc::imp::cellGcmSetVBlankFrequency, {kVBlankFrequency});
    call(ctx, gc::imp::cellGcmSetVBlankHandler, {kVBlankHandler});
    call(ctx, gc::fn::Gfx_Init52FD2C);

    // Screen parameters.
    {
        const u8 mode = field<u8>(kConfig, 34);
        const u16 width = load<u16>(kStats + 20), height = load<u16>(kStats + 22);
        store<u16>(kStats + 176, width);
        store<u16>(kStats + 178, height);
        ref<u8>(kModeByte) = mode;
        store<u32>(kStats + 160, 0x42200000);  // 40.0f
        store<u32>(kStats + 164, 0x40CCCCCD);  // 6.4f
        store<u16>(kStats + 180, width);
        store<u32>(kStats + 168, 0x40800000);  // 4.0f
        store<u32>(kStats + 172, 0x3C23D70A);  // 0.01f
        store<u16>(kStats + 182, height == 1470 ? 720 : height);  // 3D frame packing -> 720
    }
    if (field<u8>(kConfig, 35) != 0 &&
        static_cast<s32>(call(ctx, gc::imp::cellVideoOutGetScreenSize, {0, kStats + 160})) ==
            CELL_VIDEO_OUT_ERROR_VALUE_IS_NOT_SET) {
        call(ctx, gc::fn::String_Format, {kStrScreenSizeFailed});
        field<u8>(kConfig, 35) = 0;
    }
    call(ctx, gc::fn::Gfx_Init531084);
    call(ctx, gc::fn::Gfx_Init5312E4, {ref<u8>(kStats + 8), 0, 1});
    call(ctx, gc::fn::Gfx_Init5333CC);

    // Render command buffer of half the I/O size.
    {
        const u32 half = config_size(472, kDefaultIoSize) >> 1;
        u32 buffer = 0;
        const u32 obj = static_cast<u32>(call(ctx, gc::fn::Mem_NewAligned, {384, 128}));
        if (obj != 0) {
            call(ctx, gc::fn::CommandBuffer_Construct, {obj, half});
            buffer = obj;
        }
        store<u32>(kCommandBuffer, buffer);
        store<u32>(kUnk010F0E1C, 0);
    }

    // Copy the format table and build the mask of slots it uses.
    {
        store<u16>(kFormats + 120, 24);
        store<u16>(kFormats + 126, 0);
        store<u16>(kFormats + 124, static_cast<u16>(load<u16>(kFormats + 124) | 0x8000));
        u32 src = kFormatSource, dst = kFormats;
        for (;;) {
            const u32 a = load<u32>(src + 4);
            const u32 b = load<u32>(src);
            store<u32>(dst + 4, a);
            store<u32>(dst, b);
            ref<u8>(dst + 8) = ref<u8>(src + 8);
            const u8 shift_add = ref<u8>(src + 9);
            ref<u8>(dst + 9) = shift_add;
            ref<u8>(dst + 10) = ref<u8>(src + 10);
            if (a == 0) break;
            const u32 amount = (b + shift_add) & 63;  // slw: 32..63 shift everything out
            const u32 bit = amount >= 32 ? 0 : 1u << amount;
            store<u16>(kFormats + 126, static_cast<u16>(load<u16>(kFormats + 126) | bit));
            src += 12;
            dst += 12;
        }
    }

    // Local-memory pool (two blocks from pool 5).
    {
        const u32 size = config_size(468, kDefaultLocalPool);
        store<u32>(kLocalPool, size);
        field<u32>(kLocalPool, 8) = static_cast<u32>(call(ctx, gc::fn::Mem_AllocPool, {128, size, 5, 5}));
        field<u32>(kLocalPool, 12) =
            static_cast<u32>(call(ctx, gc::fn::Mem_AllocPool, {128, load<u32>(kLocalPool), 5, 5}));
        const u32 s = load<u32>(kLocalPool);
        field<u32>(kLocalPool, 20) = s;
        const u8 flags = static_cast<u8>(field<u8>(kConfig, 152) | 0x20);
        field<u32>(kLocalPool, 4) = 0;
        field<u8>(kConfig, 152) = flags;
        field<u32>(kLocalPool, 16) = s - 512;
    }

    {
        const u32 l = label(ctx, 64);
        store<u32>(kUnk010F0E04, 0);
        store<u32>(kLabel64, l);
        store<u32>(l, 0);
        store<u32>(kUnk01151880, 0);
        // lwarx / stwcx. loop in the original: an atomic store of 0.
        std::atomic_ref<u32>(*reinterpret_cast<u32*>(mem::ptr(kAtomic01151900))).store(0);
        store<u32>(kUnk01151980, 0);
    }

    // SPURS job chain: two command lists (job, sync, next) at +1024.
    {
        const u32 jc = kJobChain, guard = jc + 896, attr = jc + 272;
        const u64 job = (static_cast<u64>(guard) & ~0x7Full) | 15;
        store<u64>(jc + 1056, job);
        store<u64>(jc + 1064, (static_cast<u64>(jc + 1024) & ~7ull) | 6);
        store<u64>(jc + 1072, 2);
        store<u32>(jc + 1028, 256);
        store<u64>(jc + 1088, job);
        store<u64>(jc + 1080, (static_cast<u64>(jc + 1088) & ~7ull) | 3);
        store<u64>(jc + 1032, jc + 1152);
        store<u64>(jc + 1096, (static_cast<u64>(jc + 1040) & ~7ull) | 6);
        store<u64>(jc + 1104, 2);
        store<u32>(jc + 1044, 256);
        store<u64>(jc + 1112, (static_cast<u64>(jc + 1056) & ~7ull) | 3);
        store<u64>(jc + 1048, kJobBuffer2);

        Stack frame(ctx, 16);
        static constexpr u8 kPriorities[8] = {1, 1, 1, 0, 0, 0, 0, 0};
        mem::write_bytes(frame.addr(), kPriorities, sizeof(kPriorities));
        call(ctx, gc::imp::_cellSpursJobChainAttributeInitialize,
             {3, 0x330000, attr, jc + 1056, 256, 16, frame.addr(), 5,
              /* stack: */ 1, 0, 1, 0, 256, 0});
        call(ctx, gc::imp::cellSpursJobChainAttributeSetName, {attr, kStrJobChain});
        call(ctx, gc::imp::cellSpursJobChainAttributeSetHaltOnError, {attr});
        call(ctx, gc::imp::cellSpursCreateJobChainWithAttribute, {load<u32>(kStats + 76), jc, attr});
        call(ctx, gc::imp::cellSpursJobGuardInitialize, {jc, guard, 1, 0, 1});
        call(ctx, gc::imp::cellSpursRunJobChain, {jc});
    }
    store<u32>(kStats + 364, 0);
    call(ctx, gc::fn::Gfx_Init537F28, {kGfxObj});
    call(ctx, gc::fn::Gfx_Init5E403C, {load<u32>(kVBlankCounter), 1, 1});

    // Surface heap.
    {
        const u32 w = field<u16>(kConfig, 4), h = field<u16>(kConfig, 6);
        store<u32>(kSurfaceInfo, w);
        store<u32>(kSurfaceInfo + 4, h);
        const s32 bits = load<s32>(kSurfaceSrc + 8);
        store<u32>(kSurfaceInfo + 8, 0);
        store<s32>(kSurfaceInfo + 12, bits / 8);
        const u32 bytes = static_cast<u32>(call(ctx, gc::fn::Surface_GetSize, {w, h, 3, 0, 0, kSurfaceInfo + 8}));
        const u32 rounded = bytes + 0xFFFFF;
        store<u32>(kSurfaceSize, rounded & 0xFFF00000u);
        const u32 base = static_cast<u32>(call(ctx, gc::fn::Mem_AllocPool, {kMb, (rounded << 1) & 0xFFE00000u, 5, 0}));
        call(ctx, gc::fn::Heap_InitRegion, {kSurfaceHeap, base, (load<u32>(kSurfaceSize) << 1) & ~1u});
    }

    // Job queue.
    {
        ref<u8>(kQueueFlag) = 1;
        Stack frame(ctx, 16);
        for (u32 i = 0; i < 8; ++i) ref<u8>(frame.addr() + i) = 12;
        const u32 buffer = static_cast<u32>(call(ctx, gc::fn::Mem_AllocPool, {128, kQueueBufferSize, 6, 0}));
        store<u32>(kQueueBuffer, buffer);
        call(ctx, gc::fn::JobQueue_Init,
             {kQueue, 5, load<u32>(kStats + 76), frame.addr(), kQueueLabel, buffer, kQueueBufferSize});
        store<u32>(kQueueUnk1, 0);
        store<u32>(kQueueUnk2, 0);
        store<u32>(label(ctx, kQueueLabel), 0);
    }

    // Labels 126..146: record their addresses, then clear them.
    for (u32 i = 0; i < kLabelCount; ++i) store<u32>(kLabels + 4 * i, label(ctx, kFirstLabel + i));
    for (u32 i = 0; i < kLabelCount; ++i) store<u32>(load<u32>(kLabels + 4 * i), 0);
    ret(ctx, 1u);
}
