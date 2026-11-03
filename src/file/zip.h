// File layer: zip archives (ZipArchive), files opened from them
// (ArchiveFile) and files on the disc / host (FsFile).
//
// Names live in sly4::files; .cpp files refer to them through an alias so
// they never clash with sly4::fn or sly4::platform::fn.
#pragma once

#include "../sly4.h"

namespace sly4::files {

namespace libc {
constexpr u32 strlen = 0x0071ED08;
constexpr u32 strstr = 0x0071EEF8;
constexpr u32 strchr = 0x00723678;
constexpr u32 strcpy = 0x007239D8;
constexpr u32 strcmp = 0x00723884;
}  // namespace libc

namespace fn {
// Decompiled in src/file/zip.cpp, fs.cpp, archivefile.cpp.
constexpr u32 Archive_Find = 0x0055B3C8;     // (ZipArchive*, path, mode) -> 1 found, 0 path too long, -100 not found
constexpr u32 ZipArchive_Open = 0x0055D494;  // (ZipArchive*, path, const char* prefix or 0) -> bool
constexpr u32 File_ResolvePath = 0x0055B7D4; // (path) -> path without "dev:", with the file root
constexpr u32 Mode_IsRead = 0x0055BD00;      // (mode) -> strchr(mode, 'r') != 0
constexpr u32 File_OpenFs = 0x005706FC;      // (path, mode) -> FsFile* or 0
// Still recompiled.
constexpr u32 Zip_LocateEntry = 0x0055A8E4;    // (ZipArchive*) reads the entry at +32 (696 instr) (name?)
constexpr u32 Zip_ReadDirectory = 0x0055CF1C;  // (ZipArchive*, const char* sca, s32* ok, u32* end) (name?)
constexpr u32 Zip_BuildIndex = 0x0055C01C;     // (ZipArchive*) fills the hash buckets (473 instr) (name?)
constexpr u32 Zip_StreamRead = 0x00567938;     // (ZipArchive*, void* buf, u32 size) -> s32 (380 instr) (name?)
constexpr u32 Zip_InflateEnd = 0x005DC7C0;     // (z_stream*) (name?)
constexpr u32 String_Format = 0x005534D0;      // (fmt, ...) -> rotating static buffer
constexpr u32 String_Printf = 0x00551540;      // sprintf
constexpr u32 String_Length = 0x00555FA4;
constexpr u32 String_CopyTo = 0x00555F80;      // (dst, src) -> strcpy (claimed elsewhere)
constexpr u32 String_CompareNoCase = 0x00555DE8;  // (a, b) -> 0 if equal (name?)
constexpr u32 Path_GetExtension = 0x005611DC;  // (path) -> extension (name?)
constexpr u32 FileBase_Construct = 0x005594C4; // (File*, path) (name?)
constexpr u32 Fs_Open = 0x005618B8;            // (path, mode, FsOpenOptions by value in r5-r7) -> handle (name?)
constexpr u32 ReadCache_Init = 0x00568490;     // (ReadCache*, u32 size) (name?)
constexpr u32 Game_BootCheck = 0x0056C8E4;
constexpr u32 Mem_FreePool = 0x0056C730;
}  // namespace fn

// ---------------------------------------------------------------- shared state
constexpr u32 kFileRoot = g::file_flags + 1;  // char[]: root prefixed to archive paths
constexpr u32 kPathBuffer = 0x0116B050;       // char[]: normalised lookup path
constexpr u32 kColonBuffer = 0x0116B150;      // char[]: path after a "dev:" prefix
constexpr u32 kFsPathBuffer = 0x0116BC00;     // char[]: File_OpenFs' "<content>/<path>"
constexpr u32 kContentPath = 0x0116B3D8;      // char[]: game content root
constexpr u32 kBootChecked = 0x00F8876C;      // u8 once-flag around Game_BootCheck
constexpr u32 kCtypeTable = 0x00F94594;       // u32 -> u16 C library character class table
constexpr u16 kCtypeUpper = 0x0002;
constexpr u32 kStrDotSlash = 0x00AF3780;      // "./"
constexpr u32 kStrPathFmt = 0x00AF3784;       // "%s%s"
constexpr u32 kStrModeRead = 0x00AF37C0;      // "rb"

constexpr u32 kFileBaseVtable = 0x00B242E8;
constexpr u32 kArchiveFileVtable = 0x00B243D0;
constexpr u32 kFsFileVtable = 0x00B24530;

// Virtual slots shared by every File.
enum FileSlot : u32 {
    kFileDtor = 0,
    kFileDelete = 4,  // deleting destructor
    kFileClose = 8,
    kFileRead = 12,
    kFileWrite = 16,
    kFileSeek = 20,
    kFileSize = 24,
};
inline u64 file_vcall(PPUContext& ctx, u32 file, u32 slot, std::initializer_list<u64> args) {
    return call_ptr(ctx, load<u32>(load<u32>(file) + slot), args);
}

// ---------------------------------------------------------------- ZipArchive (1188 bytes)
namespace zip {
constexpr u32 kFile = 4;             // File* of the .zip
constexpr u32 kBufferSize = 12;      // 0x4000 when mounted by Archive_MountImageZip
constexpr u32 kDataBase = 24;        // end - (cd_size + cd_offset)
constexpr u32 kEntryIndex = 28;      // saved/restored around lookups (name?)
constexpr u32 kEntryOffset = 32;     // directory offset of the current entry
constexpr u32 kEntryValid = 36;      // non-zero while the current entry is usable
constexpr u32 kEnd = 40;
constexpr u32 kDirOffset = 44;    // central directory size/offset: which is which is a guess
constexpr u32 kDirSize = 48;
constexpr u32 kStream = 120;         // ZipStream* of the file being read
constexpr u32 kCurrentName = 124;    // char[]: name of the current entry ("" if none)
constexpr u32 kBuckets = 416;        // ZipEntry* [128]
constexpr u32 kPrefix = 928;         // char[256]: path prefix inside the archive
constexpr u32 kFlag1184 = 1184;      // u8 cleared on open
constexpr u32 kBucketMask = 0x1FC;   // (hash & 127) * 4
// ZipEntry: +0 directory offset, +4 char name[256], +260 next.
constexpr u32 kEntryName = 4, kEntryNext = 260;
constexpr s32 kNotFound = -100;
constexpr s32 kMaxPath = 256;
}  // namespace zip

// Copy `path` into kPathBuffer with '\\' -> '/' (strlen taken on every
// iteration, as in the original), then skip past the first "./".
inline u32 normalise_path(PPUContext& ctx, u32 path) {
    for (s32 i = 0; i < static_cast<s32>(call(ctx, libc::strlen, {path})); ++i) {
        const u8 c = ref<u8>(path + i);
        ref<u8>(kPathBuffer + i) = c == '\\' ? '/' : c;
    }
    ref<u8>(kPathBuffer + static_cast<s32>(call(ctx, libc::strlen, {path}))) = 0;
    const u32 dot = static_cast<u32>(call(ctx, libc::strstr, {kPathBuffer, kStrDotSlash}));
    return dot != 0 ? dot + 2 : kPathBuffer;
}

// h = h * 37 + c, upper case folded through the C library's class table.
inline u32 name_hash(u32 name) {
    const u32 ctype = load<u32>(kCtypeTable);
    u32 h = 0;
    for (u32 p = name; ref<u8>(p) != 0; ++p) {
        s32 c = static_cast<s8>(ref<u8>(p));
        if (load<u16>(ctype + 2 * static_cast<u32>(c)) & kCtypeUpper) c += 32;
        h = h * 37 + static_cast<u32>(c);
    }
    return h;
}

// mode 1: strcmp; anything else: ASCII case-insensitive (signed chars).
inline s32 name_compare(PPUContext& ctx, u32 a, u32 b, s32 mode) {
    if (mode == 1) return static_cast<s32>(call(ctx, libc::strcmp, {a, b}));
    for (u32 i = 0;; ++i) {
        const s32 x = static_cast<s8>(ref<u8>(a + i)), y = static_cast<s8>(ref<u8>(b + i));
        if (x == 0 && y == 0) return 0;
        const s32 lx = x >= 'A' && x <= 'Z' ? x + 32 : x, ly = y >= 'A' && y <= 'Z' ? y + 32 : y;
        if (lx < ly) return -1;
        if (lx > ly) return 1;
    }
}

}  // namespace sly4::files
