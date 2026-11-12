// FIOS (Sony's file I/O library) set-up: the disc/host media, an optional
// system-cache layer, the .psarc dearchiver and an overlay on top.
#include "platform.h"

#include "../memory/mem.h"
#include "../system/subsystems.h"

namespace sly4::platform {
using namespace opentit::native;
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

struct FiosParams {
    pad<8> _0;
    be_u32 allocator;  // +8 allocator object (just a vtable)
    pad<56> _12;
};
static_assert(sizeof(FiosParams) == kFiosParamsSize);

void* fios_new(u32 size) { return call<void*>(pf::fn::Fios_New, size, kFiosAllocFlags, kFiosAllocArg); }

void* new_media(char* root) {
    void* media = fios_new(kPs3MediaSize);
    if (!media) return nullptr;
    call(pf::fn::Ps3Media_Construct, media, root);
    return media;
}

}  // namespace

// Starts FIOS with the game's allocator, checks the boot (once), and builds
// the media stack: ps3media over the content path, a scheduler cache in the
// system cache if one could be mounted, a dearchiver for .psarc and
// .psarc.edat (Edge decompression on the "ApeCore" SPURS), and an overlay.
// Returns false only if the boot check fails.
bool Fios_Init() {
    GuestFrame frame(80 + kCacheParamSize + 12 + 16);
    FiosParams* params = frame.as<FiosParams>(0);
    u8* cache = frame.as<u8>(80);
    gp<u8>* decompressors = frame.as<gp<u8>>(80 + ((kCacheParamSize + 15) & ~15u));  // [1]
    be_u32* extensions = reinterpret_cast<be_u32*>(decompressors + 1);                  // const char*[2]

    call(pf::libc::memmove, params, kFiosParamsTemplate, kFiosParamsSize);
    u32 allocator = 0;
    if (be_u32* obj = static_cast<be_u32*>(memory::Mem_Alloc(4, 0, 16))) {
        *obj = kFiosAllocatorVtable;
        allocator = guest(obj);
    }
    params->allocator = allocator;
    call(pf::fn::FIOSInit, params);  // result unchecked

    u8& boot_checked = global<u8>(kBootChecked);
    if (boot_checked == 0) {
        boot_checked = 1;
        if (system::Game_BootCheck() < 0) return false;
    }
    // (The original also tests kContentPath against 0, which can't fail.)
    void* media = new_media(host<char>(kContentPath));

    u32 cache_id = *host<be_u32>(g::config + kConfigCacheId);
    if (cache_id == 0) cache_id = kStrDefaultCacheId;
    call(pf::libc::strcpy, cache + kCacheIdOffset, cache_id);
    *reinterpret_cast<be_u32*>(cache + kCacheReservedOffset) = 0;
    const s32 rc = call<s32>(pf::imp::cellSysCacheMount, cache);
    if (rc == CELL_SYSCACHE_RET_OK_CLEARED || rc == CELL_SYSCACHE_RET_OK_RELAYED) {
        char* cache_path = reinterpret_cast<char*>(cache + kCachePathOffset);
        void* cache_media = new_media(cache_path);
        const u32 cache_scheduler = call<u32>(pf::fn::Scheduler_CreateForMedia, cache_media, 0u);
        u64 key = global<be_u64>(kDefaultCacheKey);
        const be_u64& config_key = *host<be_u64>(g::config + kConfigCacheKey);
        if (config_key != 0) key = config_key;
        void* layer = nullptr;
        if (void* c = fios_new(kSchedulerCacheSize)) {
            // The 9th argument goes on the stack (call<> handles that).
            call(pf::fn::SchedulerCache_Construct, c, media, cache_scheduler, cache_path, key, kCacheArg5, kCacheArg6,
                 kCacheArg7, kCacheArg8);
            layer = c;
        }
        media = layer;
    }

    void* decompressor = nullptr;
    if (void* d = call<void*>(pf::fn::Mem_NewAligned, kEdgeDecompressorSize, kEdgeDecompressorAlign)) {
        call(pf::fn::EdgeDecompressor_Construct, d, global<be_u32>(kSpursCore).get());
        decompressor = d;
    }
    decompressors[0] = static_cast<u8*>(decompressor);
    extensions[0] = kStrPsarc;
    extensions[1] = kStrPsarcEdat;
    void* dearchiver = nullptr;
    if (void* a = fios_new(kDearchiverSize)) {
        call(pf::fn::Dearchiver_Construct, a, media, 1u, decompressors, 1u, 0u, 2u, extensions);
        dearchiver = a;
    }

    void* overlay = nullptr;
    if (void* o = fios_new(kOverlaySize)) {
        call(pf::fn::Overlay_Construct, o, dearchiver);
        overlay = o;
    }
    global<gp<u8>>(kOverlay) = static_cast<u8*>(overlay);
    global<be_u32>(kScheduler) = call<u32>(pf::fn::Scheduler_CreateForMedia, overlay, 0u);
    call(pf::fn::Fios_Unknown5FCB1C);
    return true;
}

}  // namespace sly4::platform

NATIVE(00570E08, sly4::platform::Fios_Init)
