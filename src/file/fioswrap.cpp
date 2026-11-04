// FIOS request wrappers: each builds a fios_opattr from the game's options
// block, issues the request on the scheduler and stores the fios_op in the
// FsOp. Plus the synchronous Fs_Read and the asynchronous Fs_ReadAsync.
#include "fsop.h"

using namespace sly4;
namespace op = sly4::fsop;

namespace {

// Options block (24 bytes, passed by value in three registers): words
// w0 (flags), w1 (1 = high priority), w2 (non-zero = completion callback),
// w3, w4 (extra, used with flag 0x20), w5.
struct Options {
    u32 w[6];
    Options(u64 a, u64 b, u64 c)
        : w{static_cast<u32>(a >> 32), static_cast<u32>(a), static_cast<u32>(b >> 32),
            static_cast<u32>(b), static_cast<u32>(c >> 32), static_cast<u32>(c)} {}
};

constexpr u32 kFlagTable = 0x00AF425C;  // { u32 mask; u32 fios_flag; } [7]
constexpr u32 kCallback = 0x00F3C2D8;   // completion callback OPD
constexpr u64 kDeadlineNever = 0x7FFFFFFFFFFFFFFFull;  // stored before each wrapper's code
constexpr u32 kFlagExtra = 0x20, kFlagExtraOpen = 0x40;
constexpr u32 kFiosFlagGpuMemory = 0x20;  // set for RSX / mapped buffers

u32 fios_flags(u32 flags) {
    u32 f = 0;
    for (u32 i = 0; i < 7; ++i) {
        const u32 mask = load<u32>(kFlagTable + 8 * i);
        if ((flags & mask) == mask) f |= load<u32>(kFlagTable + 8 * i + 4);
    }
    return f;
}

// fios_opattr (40 bytes) at `attr`.
void build_attr(u32 attr, const Options& o, u32 user, u32 flags) {
    const bool high = static_cast<s32>(o.w[1]) == 1;
    store<u64>(attr + 0, high ? 1 : kDeadlineNever);
    ref<u8>(attr + 8) = high ? 127 : 0x80;
    // Bytes 9-11 are uninitialised stack in the original.
    ref<u8>(attr + 9) = 0;
    ref<u8>(attr + 10) = 0;
    ref<u8>(attr + 11) = 0;
    store<u32>(attr + 12, o.w[2] != 0 ? kCallback : 0);
    store<u32>(attr + 16, 0);
    store<u32>(attr + 20, flags);
    store<u32>(attr + 24, 0);
    store<u32>(attr + 28, user);
    store<u32>(attr + 32, (o.w[0] & kFlagExtra) ? o.w[4] : 0);
    store<u32>(attr + 36, 0);
}

bool gpu_buffer(u32 p) {
    if (p == 0) return false;
    if (p >= 0xC0000000u && p <= 0xCFFFFFFFu) return true;
    return p >= 0xE0000000u;
}

u32 scheduler() { return load<u32>(op::kScheduler); }

// Read/write share their body.
u64 transfer(PPUContext& ctx, u32 fios_fn) {
    const u32 handle = arg<u32>(ctx, 0), fsop = arg<u32>(ctx, 1), data = arg<u32>(ctx, 2);
    const u64 size = ctx.gpr[6], pos = ctx.gpr[7];
    const Options o(ctx.gpr[8], ctx.gpr[9], ctx.gpr[10]);
    u32 flags = fios_flags(o.w[0]);
    if (gpu_buffer(data)) flags |= kFiosFlagGpuMemory;
    Stack frame(ctx, 48);
    build_attr(frame.addr(), o, fsop, flags);
    const u32 fop = static_cast<u32>(call(ctx, fios_fn, {scheduler(), frame.addr(), load<u32>(handle), data, size, pos}));
    store<u32>(fsop, fop);
    return fop != 0 ? 1 : 0;
}

}  // namespace

// bool Fios_OpenFile(FsOp* op, FsHandle* handle, const char* path, const char* mode, options in r7-r9)
DECOMP(00571178, Fios_OpenFile) {
    const u32 fsop = arg<u32>(ctx, 0), handle = arg<u32>(ctx, 1), path = arg<u32>(ctx, 2), mode = arg<u32>(ctx, 3);
    const Options o(ctx.gpr[7], ctx.gpr[8], ctx.gpr[9]);
    Stack frame(ctx, 48);
    build_attr(frame.addr(), o, fsop, fios_flags(o.w[0]));
    u32 open_flags = 1;  // read
    if (static_cast<s32>(call(ctx, op::fn::Mode_IsRead, {mode})) == 0) {
        open_flags = 26;  // write | create | truncate
        if (static_cast<s32>(call(ctx, op::fn::String_HasChar, {mode, 'a'})) != 0) open_flags = 14;  // append
    }
    if (o.w[0] & kFlagExtra) open_flags |= 0x100;
    if (o.w[0] & kFlagExtraOpen) open_flags |= 0x200;
    const u32 fop = static_cast<u32>(call(ctx, op::fn::fios_OpenFile, {scheduler(), frame.addr(), path, open_flags, handle}));
    store<u32>(fsop, fop);
    ret(ctx, fop != 0 ? 1u : 0u);
}

// bool Fios_CloseFile(FsHandle* handle, FsOp* op, options in r5-r7)
DECOMP(00571350, Fios_CloseFile) {
    const u32 handle = arg<u32>(ctx, 0), fsop = arg<u32>(ctx, 1);
    const Options o(ctx.gpr[5], ctx.gpr[6], ctx.gpr[7]);
    Stack frame(ctx, 48);
    build_attr(frame.addr(), o, fsop, fios_flags(o.w[0]));
    const u32 fop = static_cast<u32>(call(ctx, op::fn::fios_CloseFile, {scheduler(), frame.addr(), load<u32>(handle)}));
    store<u32>(fsop, fop);
    ret(ctx, fop != 0 ? 1u : 0u);
}

// bool Fios_ReadFile(FsHandle*, FsOp*, void* buf, u32 size, u64 pos, options in r8-r10)
DECOMP(005714B8, Fios_ReadFile) {
    ret(ctx, transfer(ctx, op::fn::fios_ReadFile));
}

// bool Fios_WriteFile(FsHandle*, FsOp*, const void* data, u32 size, u64 pos, options in r8-r10)
DECOMP(00571688, Fios_WriteFile) {
    ret(ctx, transfer(ctx, op::fn::fios_WriteFile));
}

// bool Fios_GetFileSize(FsHandle* handle, FsOp* op, u64* out, options in r6-r8)
// Stats the handle's path (0 if empty).
DECOMP(00571858, Fios_GetFileSize) {
    const u32 handle = arg<u32>(ctx, 0), fsop = arg<u32>(ctx, 1), out = arg<u32>(ctx, 2);
    const Options o(ctx.gpr[6], ctx.gpr[7], ctx.gpr[8]);
    Stack frame(ctx, 48);
    build_attr(frame.addr(), o, fsop, fios_flags(o.w[0]));
    const u32 path = load<u32>(load<u32>(handle) + op::kFiosHandlePath);
    const u32 fop = static_cast<u32>(
        call(ctx, op::fn::fios_Stat, {scheduler(), frame.addr(), ref<u8>(path) != 0 ? path : 0u, out, 3}));
    store<u32>(fsop, fop);
    ret(ctx, fop != 0 ? 1u : 0u);
}

// u32 Fs_Read(FsHandle* handle, void* buf, u32 size, u64 pos, options in r7-r9)
// Synchronous read; returns the bytes read (0 on failure).
DECOMP(00561FE8, Fs_Read) {
    const u32 handle = arg<u32>(ctx, 0);
    const u64 buf = ctx.gpr[4], size = ctx.gpr[5], pos = ctx.gpr[6];
    const u64 o0 = ctx.gpr[7], o1 = ctx.gpr[8], o2 = ctx.gpr[9];
    const u32 fsop = static_cast<u32>(call(ctx, sly4::fn::Mem_New, {sizeof(op::FsOp)}));
    if (fsop == 0) return ret(ctx, 0u);
    call(ctx, op::fn::FsOp_Init, {fsop, handle, op::kOpRead, o0, o1, o2});
    if (static_cast<s32>(call(ctx, op::fn::Fios_ReadFile, {handle, fsop, buf, size, pos, o0, o1, o2})) == 0) {
        call(ctx, op::fn::FsOp_Destruct, {fsop});
        call(ctx, op::fn::Mem_FreePool, {fsop, 0, 0});
        return ret(ctx, 0u);
    }
    call(ctx, op::fn::FsOp_Sync, {fsop});  // inlined in the original
    const u64 n = call(ctx, op::fn::FsOp_GetResult, {fsop});
    call(ctx, op::fn::FsOp_Destruct, {fsop});
    call(ctx, op::fn::Mem_FreePool, {fsop, 0, 0});
    ctx.gpr[3] = n;
}

// FsOp* Fs_ReadAsync(FsHandle* handle, void* buf, u32 size, u64 pos, options in r7-r9)
// Starts a read and returns its FsOp (0 on failure).
DECOMP(00561E74, Fs_ReadAsync) {
    const u32 handle = arg<u32>(ctx, 0);
    const u64 buf = ctx.gpr[4], size = ctx.gpr[5], pos = ctx.gpr[6];
    const u64 o0 = ctx.gpr[7], o1 = ctx.gpr[8], o2 = ctx.gpr[9];
    const u32 fsop = static_cast<u32>(call(ctx, sly4::fn::Mem_New, {sizeof(op::FsOp)}));
    if (fsop == 0) return ret(ctx, 0u);
    call(ctx, op::fn::FsOp_Init, {fsop, handle, op::kOpRead, o0, o1, o2});
    if (static_cast<s32>(call(ctx, op::fn::Fios_ReadFile, {handle, fsop, buf, size, pos, o0, o1, o2})) == 0) {
        call(ctx, op::fn::FsOp_Destruct, {fsop});
        call(ctx, op::fn::Mem_FreePool, {fsop, 0, 0});
        return ret(ctx, 0u);
    }
    ret(ctx, fsop);
}
