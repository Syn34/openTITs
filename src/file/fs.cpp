// Files outside the archives: path resolution and FsFile creation.
#include "zip.h"

using namespace sly4;
namespace fs = sly4::files;

namespace {

constexpr u32 kStrSlashFmt = 0x00AF44E8;  // "%s/%s"
constexpr u32 kStrOpenFmt = 0x00AF44F0;   // "%s - Open\n"
constexpr u32 kStrXmb = 0x00AF4250;       // "xmb"
constexpr u32 kStrXml = 0x00AF4258;       // "xml"

// FsFile (392 bytes, vtable 0x00B24530).
constexpr u32 kFsFileSize = 392;
constexpr u32 kFsHandle = 280;      // from Fs_Open, 0 on failure
constexpr u32 kFsTextFlags = 284;   // 4 for .xmb / .xml files
constexpr u32 kFsSize = 288;        // u64
constexpr u32 kFsUnk296 = 296;
constexpr u32 kFsCache = 304;       // ReadCache: +0 u32, +4 u32 count (2), +8 u32, +16 entries[count] of 32 bytes
constexpr u32 kFsCacheCount = 308;
constexpr u32 kFsCacheUnk312 = 312;
constexpr u32 kFsCacheEntries = 320;
constexpr u32 kFsCacheEntrySize = 32;
constexpr u32 kFsWriteBuffer = 384;  // WriteBuffer* (write modes)
constexpr u32 kFsCacheSlots = 2;
constexpr u32 kReadCacheSize = 0x8000;
constexpr u32 kTextFlag = 4;
constexpr u32 kFsOpenFailed = 0x80010024;  // error kept when Fs_Open returns 0

// WriteBuffer (24 bytes): { u8* data; u32 capacity; u64 used; }
constexpr u32 kWriteBufferSize = 0x8000;
constexpr u32 kWriteBufferPool = 1;

// Fs_Open's options, passed by value (24 bytes in r5-r7).
constexpr u32 kFsOptionQuiet = 16;  // set while the file-probe depth counter is non-zero (name?)

u32 file_depth() { return (ref<u8>(g::file_flags) >> 5) & 3; }

}  // namespace

// const char* File_ResolvePath(const char* path)
// Drops a "device:" prefix (copying the rest to a static buffer) and
// prepends the file root unless the path already contains it.
DECOMP(0055B7D4, File_ResolvePath) {
    u32 path = arg<u32>(ctx, 0);
    const u32 colon = static_cast<u32>(call(ctx, fs::libc::strchr, {path, ':'}));
    if (colon != 0) {
        call(ctx, fs::libc::strcpy, {fs::kColonBuffer, colon + 1});
        path = fs::kColonBuffer;
    }
    if (static_cast<u32>(call(ctx, fs::libc::strstr, {path, fs::kFileRoot})) == 0)
        path = static_cast<u32>(call(ctx, fs::fn::String_Format, {fs::kStrPathFmt, fs::kFileRoot, path}));
    ret(ctx, path);
}

// bool Mode_IsRead(const char* mode)
DECOMP(0055BD00, Mode_IsRead) {
    ret(ctx, call(ctx, fs::libc::strchr, {arg<u32>(ctx, 0), 'r'}) != 0 ? 1u : 0u);
}

// FsFile* File_OpenFs(const char* path, const char* mode)
// Opens a file on the disc / host. Read modes get a 32 KiB read cache and
// the size up front, write modes a 32 KiB write buffer. .xmb / .xml files
// are flagged as text. Returns 0 (after deleting the object) on failure.
DECOMP(005706FC, File_OpenFs) {
    const u32 path = arg<u32>(ctx, 0);
    const u32 mode = arg<u32>(ctx, 1);

    Stack frame(ctx, 272);  // the original's local buffer at 112(r1)
    const u32 local = frame.addr();
    const u32 resolved = static_cast<u32>(call(ctx, fs::fn::File_ResolvePath, {path}));
    call(ctx, fs::fn::String_CopyTo, {local, resolved});

    u32 root = fs::kContentPath;
    if (ref<u8>(fs::kBootChecked) == 0) {
        ref<u8>(fs::kBootChecked) = 1;
        if (static_cast<s32>(call(ctx, fs::fn::Game_BootCheck)) < 0) root = 0;  // printed as %s anyway
    }
    call(ctx, fs::fn::String_Printf, {fs::kFsPathBuffer, kStrSlashFmt, root, local});
    const s32 len = static_cast<s32>(call(ctx, fs::fn::String_Length, {fs::kFsPathBuffer}));
    if (len != 0) {
        for (s32 i = len - 1;; --i) {
            if (ref<u8>(fs::kFsPathBuffer + i) == '\\') ref<u8>(fs::kFsPathBuffer + i) = '/';
            if (i == 0) break;
        }
    }
    call(ctx, fs::fn::String_Format, {kStrOpenFmt, path});  // log line, result dropped

    u32 file = 0;
    const u32 obj = static_cast<u32>(call(ctx, sly4::fn::Mem_Alloc, {kFsFileSize, 0, 16}));
    if (obj != 0) {
        call(ctx, fs::fn::FileBase_Construct, {obj, path});
        field<u32>(obj, kFsHandle) = 0;
        field<u32>(obj, kFsTextFlags) = 0;
        field<u32>(obj, kFsUnk296) = 0;
        field<u32>(obj, kFsCache) = 0;
        field<u32>(obj, 0) = fs::kFsFileVtable;
        field<u64>(obj, kFsSize) = 0;
        field<u32>(obj, kFsCacheCount) = kFsCacheSlots;
        field<u32>(obj, kFsCacheUnk312) = 0;
        for (u32 i = 0; i < field<u32>(obj, kFsCacheCount); ++i) {
            const u32 e = obj + kFsCacheEntries + kFsCacheEntrySize * i;
            store<u32>(e + 0, 0);
            store<u64>(e + 8, ~0ull);
            store<u64>(e + 16, ~0ull);
            store<u32>(e + 24, 0);
        }
        field<u32>(obj, kFsWriteBuffer) = 0;
        file = obj;
    }

    const bool reading = static_cast<s32>(call(ctx, fs::fn::Mode_IsRead, {mode})) != 0;
    // Unchecked from here on: a failed allocation writes at guest 0x118 etc.
    const u32 options = file_depth() != 0 ? kFsOptionQuiet : 0;
    const u64 opt0 = (static_cast<u64>(options) << 32) | 1;  // { options, 1, 0, 0, 0, 0 }
    field<u32>(file, kFsHandle) = static_cast<u32>(call(ctx, fs::fn::Fs_Open, {path, mode, opt0, 0, 0}));
    const u32 ext = static_cast<u32>(call(ctx, fs::fn::Path_GetExtension, {path}));
    u32 text = kTextFlag;
    if (static_cast<s32>(call(ctx, fs::fn::String_CompareNoCase, {ext, kStrXmb})) != 0 &&
        static_cast<s32>(call(ctx, fs::fn::String_CompareNoCase, {ext, kStrXml})) != 0)
        text = 0;
    field<u32>(file, kFsTextFlags) = text;
    const u32 error = field<u32>(file, kFsHandle) != 0 ? 0 : kFsOpenFailed;

    if (reading) {
        if (error == 0) {
            field<u64>(file, kFsSize) = fs::file_vcall(ctx, file, fs::kFileSize, {file});
            call(ctx, fs::fn::ReadCache_Init, {file + kFsCache, kReadCacheSize});
        }
    } else {
        u32 buffer = 0;
        const u32 b = static_cast<u32>(call(ctx, sly4::fn::Mem_Alloc, {24, 0, 16}));
        if (b != 0) {
            store<u32>(b, 0);
            store<u64>(b + 8, 0);
            store<u32>(b + 4, kWriteBufferSize);
            store<u32>(b, static_cast<u32>(call(ctx, sly4::fn::Mem_Alloc, {kWriteBufferSize, kWriteBufferPool, 16})));
            buffer = b;
        }
        field<u32>(file, kFsWriteBuffer) = buffer;
    }

    if (error == 0) return ret(ctx, file);
    if (file != 0) fs::file_vcall(ctx, file, fs::kFileDelete, {file});
    ret(ctx, 0u);
}
