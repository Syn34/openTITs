// Opening files: mounted archives first (for reads), then the disc / host
// file system.
#include "../string/str.h"

namespace sly4::file {
using namespace opentit::native;

namespace {

namespace fs {
constexpr u32 Archive_Contains = 0x0055BA70;  // (Archive*, const char* path, 0) -> bool; '\\' -> '/' (name?)
constexpr u32 File_OpenArchived = 0x0055B87C; // (path, mode) -> File*; prints "%s - Open" (name?)
constexpr u32 File_OpenDirect = 0x00570B64;   // (path, mode) -> File*; disc / host fs (name?)
}  // namespace fs

constexpr u8 kFileLogOpens = 0x80;              // bit of g::file_flags: call the open hook
constexpr u32 kFileOpenHook = g::file_flags + 260;  // u32: OPD of a hook called before read opens (name?)

// The mounted archives: { Archive** entries; u32 ?; u16 count; }
struct ArchiveList {
    gp<be_u32> entries;  // +0
    pad<4> _4;
    be_u16 count;        // +8
};
constexpr u32 kArchiveList = 0x01164E14;

constexpr u32 kStrParentDir = 0x00AF3798;  // "../"
constexpr u32 kStrEmpty = 0x00B2426A;      // ""

}  // namespace

// File* File_Open(const char* path, const char* mode)
// Read modes look the path up in every mounted archive (unless it contains
// "../") and open it from the first archive that has it; everything else
// goes straight to the file system. (The original also takes strlen(path)
// before calling the hook and drops the result; that pure call is left out.)
u32 File_Open(u32 path, u32 mode) {
    if (call<u32>(libc::strchr, mode, 'r') != 0) {
        if (global<u8>(g::file_flags) & kFileLogOpens) {
            const u32 hook = global<be_u32>(kFileOpenHook);
            if (hook != 0) call_ptr(hook, kStrEmpty, hook, gref<be_u32>(hook).get());
        }
        const ArchiveList& list = global<ArchiveList>(kArchiveList);
        const u16 count = list.count;
        if (count != 0 && call<u32>(libc::strstr, path, kStrParentDir) == 0) {
            u8 i = 0;  // an 8-bit index, as in the original
            do {
                const u32 archive = gref<be_u32>(list.entries.ea + 4u * i);
                if (call<u32>(fs::Archive_Contains, archive, path, 0u) != 0) return call<u32>(fs::File_OpenArchived, path, mode);
                ++i;
            } while (static_cast<s32>(i) < static_cast<s32>(count));
        }
    }
    return call<u32>(fs::File_OpenDirect, path, mode);
}

}  // namespace sly4::file

NATIVE(0055BD3C, sly4::file::File_Open)
