// Mounted archives: lookups by path, opening a file from an archive, and
// mounting Image.zip at start-up.
#include "../platform/platform.h"

using namespace sly4;
using namespace sly4::platform;
namespace pf = sly4::platform;  // its fn/libc/imp/sc sit beside sly4's

namespace {

constexpr u32 kPathBuffer = 0x0116B050;   // char[]: Archive_Contains' normalised path
constexpr u32 kColonBuffer = 0x0116B150;  // char[]: File_OpenArchived's path after a "device:" prefix
constexpr u32 kCtypeTable = 0x00F94594;   // u32 -> u16 C library character class table
constexpr u16 kCtypeUpper = 0x0002;       // class bit tested before lower-casing

constexpr u32 kStrDotSlash = 0x00AF3780;  // "./"
constexpr u32 kStrPathFmt = 0x00AF3784;   // "%s%s"
constexpr u32 kStrOpenFmt = 0x00AF378C;   // "%s - Open\n"
constexpr u32 kStrEmpty = 0x00AF37D4;     // ""
constexpr u32 kStrImageZip = 0x00AF37DC;  // "Image.zip"

// An archive's directory: 128 hash buckets of singly linked entries.
constexpr u32 kArchiveBuckets = 416;   // Archive+416: ArchiveEntry* [128]
constexpr u32 kArchiveUnk120 = 120;    // copied into an opened file at +284
constexpr u32 kEntryName = 4;          // ArchiveEntry+4: char[256]
constexpr u32 kEntryNext = 260;        // ArchiveEntry+260: ArchiveEntry*
constexpr u32 kBucketMask = 0x1FC;     // (hash & 127) * 4

// The file object File_OpenArchived returns (288 bytes).
constexpr u32 kArchiveFileSize = 288;
constexpr u32 kFileBaseVtable = 0x00B242E8;
constexpr u32 kArchiveFileVtable = 0x00B243D0;
struct ArchiveFile {
    gptr vtable;         // +0
    char path[260];      // +4
    be_u64 unk264;       // +264 = 0
    be_u64 open_time;    // +272 timebase at open (never 0 in its low word)
    gptr archive;        // +280
    be_u32 archive_unk;  // +284 Archive+120
};
static_assert(sizeof(ArchiveFile) == kArchiveFileSize);

// ZipArchive as allocated by Archive_MountImageZip (1188 bytes).
constexpr u32 kZipArchiveSize = 1188;
constexpr u32 kZipBufferSize = 0x4000;  // stored at +12

constexpr u32 kConfigSkipImageZip = 0x5C;  // u8 in g::config (name?)

s32 lower_ascii(s32 c) { return c >= 'A' && c <= 'Z' ? c + 32 : c; }

}  // namespace

// bool Archive_Contains(Archive* archive, const char* path, int mode)
// Normalises `path` ('\\' -> '/', skips past the first "./") and looks it
// up in the archive's hash table. mode 1 compares exactly (strcmp), any
// other mode ignores ASCII case. The hash lower-cases with the C library's
// character table: h = h * 37 + c.
DECOMP(0055BA70, Archive_Contains) {
    const u32 archive = arg<u32>(ctx, 0);
    const u32 path = arg<u32>(ctx, 1);
    const s32 mode = arg<s32>(ctx, 2);

    // The original takes strlen(path) on every iteration.
    for (s32 i = 0; i < static_cast<s32>(call(ctx, pf::libc::strlen, {path})); ++i) {
        const u8 c = ref<u8>(path + i);
        ref<u8>(kPathBuffer + i) = c == '\\' ? '/' : c;
    }
    ref<u8>(kPathBuffer + static_cast<s32>(call(ctx, pf::libc::strlen, {path}))) = 0;

    u32 name = kPathBuffer;
    const u32 dot = static_cast<u32>(call(ctx, pf::libc::strstr, {kPathBuffer, kStrDotSlash}));
    if (dot != 0) name = dot + 2;

    u32 hash = 0;
    const u32 ctype = load<u32>(kCtypeTable);
    for (u32 p = name; ref<u8>(p) != 0; ++p) {
        s32 c = static_cast<s8>(ref<u8>(p));
        if (load<u16>(ctype + 2 * static_cast<u32>(c)) & kCtypeUpper) c += 32;
        hash = hash * 37 + static_cast<u32>(c);
    }

    for (u32 entry = load<u32>(archive + kArchiveBuckets + ((hash << 2) & kBucketMask)); entry != 0;
         entry = load<u32>(entry + kEntryNext)) {
        s32 cmp;
        if (mode == 1) {
            cmp = static_cast<s32>(call(ctx, pf::libc::strcmp, {name, entry + kEntryName}));
        } else {
            cmp = 0;
            for (u32 i = 0;; ++i) {
                const s32 a = static_cast<s8>(ref<u8>(name + i));
                const s32 b = static_cast<s8>(ref<u8>(entry + kEntryName + i));
                if (a == 0 && b == 0) break;
                const s32 la = lower_ascii(a), lb = lower_ascii(b);
                if (la < lb) { cmp = -1; break; }
                if (la > lb) { cmp = 1; break; }
            }
        }
        if (cmp == 0) return ret(ctx, 1u);
    }
    ret(ctx, 0u);
}

// File* File_OpenArchived(const char* path, const char* mode)
// Opens `path` (any "device:" prefix dropped, the file root prepended if
// missing) from the first mounted archive that has it. The mode is unused.
DECOMP(0055B87C, File_OpenArchived) {
    u32 path = arg<u32>(ctx, 0);
    const u32 colon = static_cast<u32>(call(ctx, pf::libc::strchr, {path, ':'}));
    if (colon != 0) {
        call(ctx, pf::libc::strcpy, {kColonBuffer, colon + 1});
        path = kColonBuffer;
    }
    if (static_cast<u32>(call(ctx, pf::libc::strstr, {path, kFileRoot})) == 0)
        path = static_cast<u32>(call(ctx, pf::fn::String_Format, {kStrPathFmt, kFileRoot, path}));
    call(ctx, pf::fn::String_Format, {kStrOpenFmt, path});  // a log line; the result is dropped

    const PtrVector& list = ref<PtrVector>(kArchiveList);
    const u16 count = list.count;
    if (static_cast<s32>(count) <= 0) return ret(ctx, 0u);
    u8 i = 0;  // an 8-bit index, as in the original
    while (static_cast<u32>(call(ctx, pf::fn::Archive_Find, {load<u32>(list.data + 4u * i), path, 0})) != 1) {
        ++i;
        if (static_cast<s32>(i) >= static_cast<s32>(count)) return ret(ctx, 0u);
    }
    if (static_cast<s32>(call(ctx, pf::fn::Archive_Prepare, {load<u32>(list.data + 4u * i)})) != 1) return ret(ctx, 0u);

    const u32 obj = static_cast<u32>(call(ctx, sly4::fn::Mem_New, {kArchiveFileSize}));
    u32 file = 0;
    if (obj != 0) {
        ArchiveFile& f = ref<ArchiveFile>(obj);
        f.vtable = kFileBaseVtable;
        f.unk264 = 0;
        call(ctx, pf::libc::strcpy, {obj + 4, path});
        u64 tb;
        do tb = read_timebase();  // mftb until the low word is non-zero
        while (static_cast<u32>(tb) == 0);
        f.open_time = tb;
        call(ctx, pf::fn::SystemState_CallHook);
        f.archive_unk = 0;
        f.archive = 0;
        f.vtable = kArchiveFileVtable;
        file = obj;
    }
    // Unchecked: a failed allocation writes these at guest address 0x118/0x11C.
    const u32 archive = load<u32>(list.data + 4u * i);
    field<u32>(file, 284) = load<u32>(archive + kArchiveUnk120);
    field<u32>(file, 280) = load<u32>(list.data + 4u * i);
    ret(ctx, file);
}

// File* File_OpenDirect(const char* path, const char* mode)
// Opens a file outside the archives (disc / host file system).
DECOMP(00570B64, File_OpenDirect) {
    ctx.gpr[3] = call(ctx, pf::fn::File_OpenFs, {ctx.gpr[3], ctx.gpr[4]});
}

// bool Archive_MountImageZip()
// Clears the file root, then (unless config +0x5C is set) mounts Image.zip
// as an archive if the file exists. Always returns true.
DECOMP(0055D830, Archive_MountImageZip) {
    call(ctx, pf::libc::strcpy, {kFileRoot, kStrEmpty});
    call(ctx, pf::fn::Image_Nop);
    if (field<u8>(g::config, kConfigSkipImageZip) != 0) return ret(ctx, 1u);

    adjust_file_depth(+1);
    const u32 probe = static_cast<u32>(call(ctx, sly4::fn::File_Open, {kStrImageZip, kModeRead}));
    if (probe == 0) {
        adjust_file_depth(-1);
        return ret(ctx, 1u);
    }
    file_close(ctx, probe);
    adjust_file_depth(-1);

    u32 zip = static_cast<u32>(call(ctx, sly4::fn::Mem_New, {kZipArchiveSize}));
    if (zip != 0) {
        field<u32>(zip, 4) = 0;
        field<u32>(zip, 8) = 0;
        field<u32>(zip, 12) = kZipBufferSize;
    }
    // Called even if the allocation failed (with 0).
    if (static_cast<s32>(call(ctx, pf::fn::ZipArchive_Open, {zip, kStrImageZip, 0})) != 1) {
        call(ctx, pf::fn::Mem_FreePool, {zip, 0, 0});  // no destructor
        store<u16>(kImageArchiveCount, 0);
        return ret(ctx, 1u);
    }

    PtrVector& list = ref<PtrVector>(kArchiveList);
    if (list.count == list.capacity)
        call(ctx, pf::fn::PtrVector_Reserve, {kArchiveList, (static_cast<u32>(list.capacity) << 1) & 0x1FFFE});
    const u16 index = list.count;
    store<u32>(list.data + 4u * index, zip);
    const u16 count = static_cast<u16>(index + 1);
    const u16 high = list.high_water;
    list.count = count;
    list.high_water = high > count ? high : count;
    store<u16>(kImageArchiveCount, count);
    ret(ctx, 1u);
}
