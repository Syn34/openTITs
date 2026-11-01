// Opening files: mounted archives first (for reads), then the disc / host
// file system.
#include "../string/str.h"

using namespace sly4;

namespace {

namespace fs {
constexpr u32 Archive_Contains = 0x0055BA70;  // (Archive*, const char* path, 0) -> bool; '\\' -> '/' (name?)
constexpr u32 File_OpenArchived = 0x0055B87C; // (path, mode) -> File*; prints "%s - Open" (name?)
constexpr u32 File_OpenDirect = 0x00570B64;   // (path, mode) -> File*; disc / host fs (name?)
}  // namespace fs

constexpr u8 kFileLogOpens = 0x80;              // bit of g::file_flags: call the open hook
constexpr u32 kFileOpenHook = g::file_flags + 260;  // u32: OPD of a hook called before read opens (name?)

// The mounted archives: { Archive** entries; u32 ?; u16 count; }
constexpr u32 kArchiveList = 0x01164E14;
constexpr u32 kArchiveCount = kArchiveList + 8;  // u16

constexpr u32 kStrParentDir = 0x00AF3798;  // "../"
constexpr u32 kStrEmpty = 0x00B2426A;      // ""

}  // namespace

// File* File_Open(const char* path, const char* mode)
// Read modes look the path up in every mounted archive (unless it contains
// "../") and open it from the first archive that has it; everything else
// goes straight to the file system.
DECOMP(0055BD3C, File_Open) {
    const u32 path = arg<u32>(ctx, 0);
    const u32 mode = arg<u32>(ctx, 1);

    if (static_cast<u32>(call(ctx, libc::strchr, {mode, 'r'})) != 0) {
        if (ref<u8>(g::file_flags) & kFileLogOpens) {
            // The original also takes strlen(path) here and drops the result.
            const u32 hook = load<u32>(kFileOpenHook);
            if (hook != 0) call_ptr(ctx, hook, {kStrEmpty, hook, load<u32>(hook)});
        }
        const u16 count = load<u16>(kArchiveCount);
        if (count != 0 && static_cast<u32>(call(ctx, libc::strstr, {path, kStrParentDir})) == 0) {
            u8 i = 0;  // an 8-bit index, as in the original
            do {
                const u32 archive = load<u32>(load<u32>(kArchiveList) + 4u * i);
                if (static_cast<u32>(call(ctx, fs::Archive_Contains, {archive, path, 0})) != 0) {
                    ctx.gpr[3] = call(ctx, fs::File_OpenArchived, {path, mode});
                    return;
                }
                ++i;
            } while (static_cast<s32>(i) < static_cast<s32>(count));
        }
    }
    ctx.gpr[3] = call(ctx, fs::File_OpenDirect, {path, mode});
}
