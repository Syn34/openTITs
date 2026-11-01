// FIOS (Sony's file I/O library) set-up: the disc/host media, an optional
// system-cache layer, the .psarc dearchiver and an overlay on top.
#include "platform.h"

using namespace sly4;
using namespace sly4::platform;
namespace pf = sly4::platform;  // its fn/libc/imp/sc sit beside sly4's

namespace {

constexpr u32 kFiosParamsTemplate = 0x00F888DC;  // fios_parameters defaults (68 bytes)
constexpr u32 kFiosParamsSize = 68;
constexpr u32 kFiosParamsAllocator = 8;          // offset of the allocator pointer
constexpr u32 kFiosAllocatorVtable = 0x00B245D8;  // 4-byte allocator object (just a vtable)

constexpr u32 kBootChecked = 0x00F8876C;   // u8 once-flag around Game_BootCheck
constexpr u32 kContentPath = 0x0116B3D8;   // char[]: game content root (filled by Game_BootCheck?)
constexpr u32 kOverlay = 0x00F88774;       // u32: the FIOS overlay (top media layer)
constexpr u32 kScheduler = 0x00F88770;     // u32: scheduler for kOverlay

constexpr u32 kConfigCacheId = 520;   // g::config: const char* system cache id, or 0
constexpr u32 kConfigCacheKey = 528;  // g::config: u64 cache key, or 0
constexpr u32 kDefaultCacheKey = 0x00570E00;  // u64 stored just before Fios_Init's code
constexpr u32 kStrDefaultCacheId = 0x00AF47E4;  // "ABCD12345"
constexpr u32 kStrPsarc = 0x00AF47FC;      // ".psarc"
constexpr u32 kStrPsarcEdat = 0x00AF4804;  // ".psarc.edat"

// Fios_New(size, kFiosAllocFlags, 3) sizes.
constexpr u32 kFiosAllocFlags = 0x10000100;
constexpr u32 kFiosAllocArg = 3;
constexpr u32 kPs3MediaSize = 1072, kSchedulerCacheSize = 616, kDearchiverSize = 2768, kOverlaySize = 2696;
constexpr u32 kEdgeDecompressorSize = 12032, kEdgeDecompressorAlign = 128;
constexpr u32 kSpursCore = g::mem_stats + 76;  // CellSpurs* "ApeCore" (MemStats::spurs_core)

// CellSysCacheParam
constexpr u32 kCacheIdOffset = 0, kCachePathOffset = 32, kCacheReservedOffset = 1088, kCacheParamSize = 1092;
constexpr s32 CELL_SYSCACHE_RET_OK_CLEARED = 0, CELL_SYSCACHE_RET_OK_RELAYED = 1;

// SchedulerCache_Construct's arguments after the media/scheduler/path.
constexpr u32 kCacheArg5 = 1, kCacheArg6 = 1, kCacheArg7 = 2037, kCacheArg8 = 0x100000;

u32 fios_new(PPUContext& ctx, u32 size) {
    return static_cast<u32>(call(ctx, pf::fn::Fios_New, {size, kFiosAllocFlags, kFiosAllocArg}));
}

u32 new_media(PPUContext& ctx, u32 root) {
    const u32 media = fios_new(ctx, kPs3MediaSize);
    if (media == 0) return 0;
    call(ctx, pf::fn::Ps3Media_Construct, {media, root});
    return media;
}

}  // namespace

// bool Fios_Init()
// Starts FIOS with the game's allocator, checks the boot (once), and builds
// the media stack: ps3media over the content path, a scheduler cache in the
// system cache if one could be mounted, a dearchiver for .psarc and
// .psarc.edat (Edge decompression on the "ApeCore" SPURS), and an overlay.
// Returns false only if the boot check fails.
DECOMP(00570E08, Fios_Init) {
    Stack frame(ctx, 80 + kCacheParamSize + 12 + 16);
    const u32 params = frame.addr();
    const u32 cache = params + 80;
    const u32 decompressors = cache + ((kCacheParamSize + 15) & ~15u);  // u32[1]
    const u32 extensions = decompressors + 4;                           // const char*[2]

    call(ctx, pf::libc::memmove, {params, kFiosParamsTemplate, kFiosParamsSize});
    u32 allocator = 0;
    const u32 obj = static_cast<u32>(call(ctx, sly4::fn::Mem_Alloc, {4, 0, 16}));
    if (obj != 0) {
        store<u32>(obj, kFiosAllocatorVtable);
        allocator = obj;
    }
    store<u32>(params + kFiosParamsAllocator, allocator);
    call(ctx, pf::fn::FIOSInit, {params});  // result unchecked

    if (ref<u8>(kBootChecked) == 0) {
        ref<u8>(kBootChecked) = 1;
        if (static_cast<s32>(call(ctx, pf::fn::Game_BootCheck)) < 0) return ret(ctx, 0u);
    }
    // (The original also tests kContentPath against 0, which can't fail.)
    u32 media = new_media(ctx, kContentPath);

    u32 cache_id = field<u32>(g::config, kConfigCacheId);
    if (cache_id == 0) cache_id = kStrDefaultCacheId;
    call(ctx, pf::libc::strcpy, {cache + kCacheIdOffset, cache_id});
    store<u32>(cache + kCacheReservedOffset, 0);
    const s32 rc = static_cast<s32>(call(ctx, pf::imp::cellSysCacheMount, {cache}));
    if (rc == CELL_SYSCACHE_RET_OK_CLEARED || rc == CELL_SYSCACHE_RET_OK_RELAYED) {
        const u32 cache_path = cache + kCachePathOffset;
        const u32 cache_media = new_media(ctx, cache_path);
        const u32 cache_scheduler = static_cast<u32>(call(ctx, pf::fn::Scheduler_CreateForMedia, {cache_media, 0}));
        u64 key = load<u64>(kDefaultCacheKey);
        if (field<u64>(g::config, kConfigCacheKey) != 0) key = field<u64>(g::config, kConfigCacheKey);
        u32 layer = 0;
        const u32 c = fios_new(ctx, kSchedulerCacheSize);
        if (c != 0) {
            // The 9th argument goes on the stack (call() handles that).
            call(ctx, pf::fn::SchedulerCache_Construct,
                 {c, media, cache_scheduler, cache_path, key, kCacheArg5, kCacheArg6, kCacheArg7, kCacheArg8});
            layer = c;
        }
        media = layer;
    }

    u32 decompressor = 0;
    const u32 d = static_cast<u32>(call(ctx, pf::fn::Mem_NewAligned, {kEdgeDecompressorSize, kEdgeDecompressorAlign}));
    if (d != 0) {
        call(ctx, pf::fn::EdgeDecompressor_Construct, {d, load<u32>(kSpursCore)});
        decompressor = d;
    }
    store<u32>(decompressors, decompressor);
    store<u32>(extensions, kStrPsarc);
    store<u32>(extensions + 4, kStrPsarcEdat);
    u32 dearchiver = 0;
    const u32 a = fios_new(ctx, kDearchiverSize);
    if (a != 0) {
        call(ctx, pf::fn::Dearchiver_Construct, {a, media, 1, decompressors, 1, 0, 2, extensions});
        dearchiver = a;
    }

    u32 overlay = 0;
    const u32 o = fios_new(ctx, kOverlaySize);
    if (o != 0) {
        call(ctx, pf::fn::Overlay_Construct, {o, dearchiver});
        overlay = o;
    }
    store<u32>(kOverlay, overlay);
    store<u32>(kScheduler, static_cast<u32>(call(ctx, pf::fn::Scheduler_CreateForMedia, {overlay, 0})));
    call(ctx, pf::fn::Fios_Unknown5FCB1C);
    ret(ctx, 1u);
}
