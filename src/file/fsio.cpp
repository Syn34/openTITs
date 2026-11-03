// Synchronous FIOS file operations (open, close, write, size), the File
// base class and two path/string helpers.
#include "fsio.h"

using namespace sly4;
namespace io = sly4::fsio;

namespace {
constexpr u32 kFileBaseVtable = 0x00B242E8;
constexpr u32 kStrcpy = 0x007239D8;
}  // namespace

// FsHandle* Fs_Open(const char* path, const char* mode, FsOptions options)
// The options block (24 bytes) arrives by value in r5-r7. Returns the new
// FsHandle, or 0 on failure (the handle object then leaks, as in the original).
DECOMP(005618B8, Fs_Open) {
    const u32 path = arg<u32>(ctx, 0), mode = arg<u32>(ctx, 1);
    const u64 o0 = ctx.gpr[5], o1 = ctx.gpr[6], o2 = ctx.gpr[7];

    const u32 handle = static_cast<u32>(call(ctx, sly4::fn::Mem_New, {4}));
    if (handle == 0) return ret(ctx, 0u);
    call(ctx, io::fn::FsHandle_Construct, {handle});
    const u32 op = static_cast<u32>(call(ctx, sly4::fn::Mem_New, {io::kFsOpSize}));
    if (op == 0) {
        call(ctx, io::fn::FsHandle_Destruct, {handle});
        call(ctx, io::fn::Mem_FreePool, {handle, 0, 0});
        return ret(ctx, 0u);
    }
    call(ctx, io::fn::FsOp_Init, {op, handle, io::kOpOpen, o0, o1, o2});
    if (static_cast<s32>(call(ctx, io::fn::Fios_OpenFile, {op, handle, path, mode, o0, o1, o2})) == 0) {
        io::op_delete(ctx, op);
        return ret(ctx, 0u);
    }
    io::op_finish(ctx, op);
    u32 result = 0;
    if (static_cast<s32>(call(ctx, io::fn::FsOp_GetError, {op})) == 0) result = load<u32>(op + io::kOpTarget);
    io::op_delete(ctx, op);
    ret(ctx, result);
}

// void Fs_Close(FsHandle* handle, FsOptions options in r4-r6)
DECOMP(00561C78, Fs_Close) {
    const u32 handle = arg<u32>(ctx, 0);
    const u64 o0 = ctx.gpr[4], o1 = ctx.gpr[5], o2 = ctx.gpr[6];
    const u32 op = static_cast<u32>(call(ctx, sly4::fn::Mem_New, {io::kFsOpSize}));
    if (op == 0) return ret(ctx, 0u);
    call(ctx, io::fn::FsOp_Init, {op, handle, io::kOpClose, o0, o1, o2});
    if (static_cast<s32>(call(ctx, io::fn::Fios_CloseFile, {handle, op, o0, o1, o2})) != 0) io::op_finish(ctx, op);
    ctx.gpr[3] = io::op_delete(ctx, op);
}

// u32 Fs_Write(FsHandle* handle, const void* data, u32 size, u64 pos, FsOptions options in r7-r9)
// Returns the bytes written (0 on failure).
DECOMP(00562230, Fs_Write) {
    const u32 handle = arg<u32>(ctx, 0);
    const u64 data = ctx.gpr[4], size = ctx.gpr[5], pos = ctx.gpr[6];
    const u64 o0 = ctx.gpr[7], o1 = ctx.gpr[8], o2 = ctx.gpr[9];
    const u32 op = static_cast<u32>(call(ctx, sly4::fn::Mem_New, {io::kFsOpSize}));
    if (op == 0) return ret(ctx, 0u);
    call(ctx, io::fn::FsOp_Init, {op, handle, io::kOpWrite, o0, o1, o2});
    if (static_cast<s32>(call(ctx, io::fn::Fios_WriteFile, {handle, op, data, size, pos, o0, o1, o2})) == 0) {
        io::op_delete(ctx, op);
        return ret(ctx, 0u);
    }
    io::op_finish(ctx, op);
    const u64 written = call(ctx, io::fn::FsOp_GetResult, {op});
    io::op_delete(ctx, op);
    ctx.gpr[3] = written;
}

// u64 Fs_GetSize(FsHandle* handle, FsOptions options in r4-r6)
DECOMP(005625C8, Fs_GetSize) {
    const u32 handle = arg<u32>(ctx, 0);
    const u64 o0 = ctx.gpr[4], o1 = ctx.gpr[5], o2 = ctx.gpr[6];
    Stack frame(ctx, 8);
    // The original leaves this uninitialised; Fios_GetFileSize fills it in.
    store<u64>(frame.addr(), 0);
    const u32 op = static_cast<u32>(call(ctx, sly4::fn::Mem_New, {io::kFsOpSize}));
    if (op == 0) return ret(ctx, 0u);
    call(ctx, io::fn::FsOp_Init, {op, handle, io::kOpGetSize, o0, o1, o2});
    if (static_cast<s32>(call(ctx, io::fn::Fios_GetFileSize, {handle, op, frame.addr(), o0, o1, o2})) == 0) {
        io::op_delete(ctx, op);
        return ret(ctx, 0u);
    }
    io::op_finish(ctx, op);
    const u64 size = load<u64>(frame.addr());
    io::op_delete(ctx, op);
    ctx.gpr[3] = size;
}

// File::File(const char* path): vtable, position 0, name, open timestamp.
DECOMP(005594C4, FileBase_Construct) {
    const u32 file = arg<u32>(ctx, 0);
    const u32 path = arg<u32>(ctx, 1);
    store<u32>(file, kFileBaseVtable);
    store<u64>(file + io::kFilePosition, 0);
    call(ctx, kStrcpy, {file + io::kFileName, path});
    u64 tb;
    do tb = read_timebase();  // mftb until the low word is non-zero
    while (static_cast<u32>(tb) == 0);
    store<u64>(file + 272, tb);
    ctx.gpr[3] = call(ctx, io::fn::SystemState_CallHook);
}

// File::~File()
DECOMP(00560B80, FileBase_Destruct) {
    store<u32>(arg<u32>(ctx, 0), kFileBaseVtable);
}

// const char* Path_GetExtension(const char* path)
// The text after the last '.'; 0 if the only '.' is the first character,
// and the whole path if there is no '.' at all. 0 for a null path.
DECOMP(005611DC, Path_GetExtension) {
    const u32 path = arg<u32>(ctx, 0);
    if (path == 0) return ret(ctx, 0u);
    s32 i = static_cast<s32>(call(ctx, io::fn::strlen, {path}));
    while (i >= 0 && ref<u8>(path + static_cast<u32>(i)) != '.') --i;  // starts at the terminator
    if (i == 0) return ret(ctx, 0u);
    ret(ctx, path + static_cast<u32>(i) + 1);
}

// s32 String_CompareNoCase(const char* a, const char* b)
// -1 / 0 / 1, ASCII letters folded to lower case (chars are signed).
DECOMP(00555DE8, String_CompareNoCase) {
    const u32 a = arg<u32>(ctx, 0), b = arg<u32>(ctx, 1);
    for (u32 i = 0;; ++i) {
        const s32 x = static_cast<s8>(ref<u8>(a + i)), y = static_cast<s8>(ref<u8>(b + i));
        if (x == 0 && y == 0) return ret(ctx, 0);
        const s32 lx = x >= 'A' && x <= 'Z' ? x + 32 : x;
        const s32 ly = y >= 'A' && y <= 'Z' ? y + 32 : y;
        if (lx < ly) return ret(ctx, -1);
        if (lx > ly) return ret(ctx, 1);
    }
}
