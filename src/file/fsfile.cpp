// FsFile (vtable 0x00B24530): a file on the disc / host, opened by
// File_OpenFs. Reads go through a read cache (+304), writes through a
// 32 KiB write buffer (+384) flushed on close.
#include "zip.h"

using namespace sly4;
namespace fs = sly4::files;

namespace {

constexpr u32 kName = 4;        // char[]: path (for the log lines)
constexpr u32 kPosition = 264;  // u64
constexpr u32 kHandle = 280;
constexpr u32 kSize = 288;      // u64, 0 until known
constexpr u32 kCache = 304;
constexpr u32 kWriteBuffer = 384;  // { u8* data; u32 capacity; u64 used; }

namespace callee {
constexpr u32 Mem_FreeFrom = 0x0054EE6C;       // (p, pool) -> Mem_FreePool(p, pool, 0)
constexpr u32 FileBase_Destruct = 0x00560B80;  // (File*)
constexpr u32 ReadCache_Destruct = 0x005682B4; // (ReadCache*) (name?)
constexpr u32 ReadCache_Read = 0x007A5840;     // (ReadCache*, File*, buf, size) -> u32 (name?)
constexpr u32 WriteBuffer_Write = 0x007A5B58;  // (WriteBuffer*, File*, buf, size) -> s32 (name?)
// Fs_* take an options block by value: { u32 flags; u32 one; u32 0 x4 }.
constexpr u32 Fs_Close = 0x00561C78;    // (handle, options in r4-r6) (name?)
constexpr u32 Fs_Write = 0x00562230;    // (handle, data, size, u64 position, options in r7-r9) -> u64 (name?)
constexpr u32 Fs_GetSize = 0x005625C8;  // (handle, options in r4-r6) -> u64 (name?)
}  // namespace callee
constexpr u64 kFsOptions = 1;  // first doubleword of the options block: flags 0, then 1

constexpr u32 kStrClose = 0x00AF483C;    // "%s - Close\n"
constexpr u32 kStrWrite = 0x00AF481C;    // "%s - Write %d bytes to %x\n"
constexpr u32 kStrSeek = 0x00AF484C;     // "%s - Seek - Mode=%s Offset=%d\n"
constexpr u32 kStrSet = 0x00AF486C, kStrCur = 0x00AF4874, kStrEnd = 0x00AF487C;
constexpr u32 kStrGetSize = 0x00AF4880;  // "%s - GetSize\n"

// Common body of both destructors.
u64 destruct(PPUContext& ctx, u32 file) {
    store<u32>(file, fs::kFsFileVtable);
    if (field<u32>(file, kHandle) != 0) call(ctx, 0x00571DC4, {file});  // FsFile_Close
    call(ctx, callee::ReadCache_Destruct, {file + kCache});
    return call(ctx, callee::FileBase_Destruct, {file});
}

}  // namespace

// FsFile::~FsFile() (non-deleting)
DECOMP(007A5DDC, FsFile_Destruct) {
    ctx.gpr[3] = destruct(ctx, arg<u32>(ctx, 0));
}

// FsFile deleting destructor.
DECOMP(007A5E40, FsFile_Delete) {
    const u32 file = arg<u32>(ctx, 0);
    if (file == 0) return;
    destruct(ctx, file);
    ctx.gpr[3] = call(ctx, callee::Mem_FreeFrom, {file, 0});
}

// bool FsFile::Close()
// Flushes and frees the write buffer, closes the handle, then deletes the
// object (virtual slot 1). Always returns true.
DECOMP(00571DC4, FsFile_Close) {
    const u32 file = arg<u32>(ctx, 0);
    call(ctx, fs::fn::String_Format, {kStrClose, file + kName});

    u32 wb = field<u32>(file, kWriteBuffer);
    if (wb != 0 && static_cast<s64>(load<u64>(wb + 8)) > 0) {
        const u32 used = static_cast<u32>(load<u64>(wb + 8));
        const u32 data = load<u32>(wb);
        call(ctx, fs::fn::String_Format, {kStrWrite, file + kName, used, data});
        const u64 written = call(ctx, callee::Fs_Write,
                                 {field<u32>(file, kHandle), data, used, field<u64>(file, kPosition), kFsOptions, 0, 0});
        field<u64>(file, kPosition) += written;
        store<u64>(wb + 8, 0);
        wb = field<u32>(file, kWriteBuffer);
    }
    if (wb != 0) {
        if (load<u32>(wb) != 0) {
            call(ctx, callee::Mem_FreeFrom, {load<u32>(wb), 1});
            store<u32>(wb, 0);
        }
        call(ctx, callee::Mem_FreeFrom, {wb, 0});
        field<u32>(file, kWriteBuffer) = 0;
    }

    call(ctx, callee::Fs_Close, {field<u32>(file, kHandle), kFsOptions, 0, 0});
    field<u32>(file, kHandle) = 0;
    if (file != 0) fs::file_vcall(ctx, file, fs::kFileDelete, {file});
    ret(ctx, 1u);
}

// s32 FsFile::Read(void* buf, u32 size, bool* done)
DECOMP(00571F70, FsFile_Read) {
    const u32 file = arg<u32>(ctx, 0);
    const u32 done = arg<u32>(ctx, 3);
    const u32 n = static_cast<u32>(call(ctx, callee::ReadCache_Read, {file + kCache, file, ctx.gpr[4], ctx.gpr[5]}));
    field<u64>(file, kPosition) += static_cast<u64>(n);
    if (done != 0) ref<u8>(done) = 1;
    ret(ctx, static_cast<s32>(n));
}

// s32 FsFile::Write(const void* buf, u32 size, bool* done)
DECOMP(00571FE4, FsFile_Write) {
    const u32 file = arg<u32>(ctx, 0);
    const u32 done = arg<u32>(ctx, 3);
    const u32 n = static_cast<u32>(
        call(ctx, callee::WriteBuffer_Write, {field<u32>(file, kWriteBuffer), file, ctx.gpr[4], ctx.gpr[5]}));
    if (done != 0) ref<u8>(done) = 1;
    ret(ctx, static_cast<s32>(n));
}

// u64 FsFile::Seek(s64 offset, s32 whence)
// Only moves the position (0 set, 1 current, 2 end); any other whence
// resets it to 0. Returns the new position.
DECOMP(0057203C, FsFile_Seek) {
    const u32 file = arg<u32>(ctx, 0);
    const u64 offset = ctx.gpr[4];
    const s32 whence = arg<s32>(ctx, 2);
    const u32 mode = whence == 0 ? kStrSet : whence == 1 ? kStrCur : kStrEnd;
    call(ctx, fs::fn::String_Format, {kStrSeek, file + kName, mode, offset});
    u64 position = 0;
    if (whence == 0) position = offset;
    else if (whence == 1) position = field<u64>(file, kPosition) + offset;
    else if (whence == 2) position = field<u64>(file, kSize) - offset;
    field<u64>(file, kPosition) = position;
    ctx.gpr[3] = position;
}

// u64 FsFile::Size(): cached after the first query.
DECOMP(00572118, FsFile_Size) {
    const u32 file = arg<u32>(ctx, 0);
    call(ctx, fs::fn::String_Format, {kStrGetSize, file + kName});
    u64 size = field<u64>(file, kSize);
    if (size == 0) {
        size = call(ctx, callee::Fs_GetSize, {field<u32>(file, kHandle), kFsOptions, 0, 0});
        field<u64>(file, kSize) = size;
    }
    ctx.gpr[3] = size;
}
