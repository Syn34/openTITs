// FIOS request wrappers: each builds a fios_opattr from the game's options
// block, issues the request on the scheduler and stores the fios_op in the
// FsOp. Plus the synchronous Fs_Read and the asynchronous Fs_ReadAsync.
#include "fsop.h"
#include "../memory/mem.h"

namespace sly4::fsop {
using namespace opentit::native;

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

struct FlagMap {
    be_u32 mask;
    be_u32 fios_flag;
};
constexpr u32 kFlagTable = 0x00AF425C;  // FlagMap[7]
constexpr u32 kCallback = 0x00F3C2D8;   // completion callback OPD
constexpr u64 kDeadlineNever = 0x7FFFFFFFFFFFFFFFull;  // stored before each wrapper's code
constexpr u32 kFlagExtra = 0x20, kFlagExtraOpen = 0x40;
constexpr u32 kFiosFlagGpuMemory = 0x20;  // set for RSX / mapped buffers

// fios_opattr (40 bytes).
struct OpAttr {
    be_u64 deadline;   // +0
    u8 priority;       // +8
    u8 _9[3];          // +9 (uninitialised stack in the original; 0 here)
    be_u32 callback;   // +12 OPD
    be_u32 w16;        // +16
    be_u32 flags;      // +20
    be_u32 w24;        // +24
    be_u32 user;       // +28 the FsOp
    be_u32 extra;      // +32
    be_u32 w36;        // +36
};
static_assert(sizeof(OpAttr) == 40);

u32 fios_flags(u32 flags) {
    const FlagMap* table = host<FlagMap>(kFlagTable);
    u32 f = 0;
    for (u32 i = 0; i < 7; ++i) {
        const u32 mask = table[i].mask;
        if ((flags & mask) == mask) f |= table[i].fios_flag;
    }
    return f;
}

void build_attr(OpAttr& a, const Options& o, FsOp* user, u32 flags) {
    const bool high = static_cast<s32>(o.w[1]) == 1;
    a.deadline = high ? 1 : kDeadlineNever;
    a.priority = high ? 127 : 0x80;
    a._9[0] = a._9[1] = a._9[2] = 0;
    a.callback = o.w[2] != 0 ? kCallback : 0;
    a.w16 = 0u;
    a.flags = flags;
    a.w24 = 0u;
    a.user = guest(user);
    a.extra = (o.w[0] & kFlagExtra) ? o.w[4] : 0;
    a.w36 = 0u;
}

bool gpu_buffer(u32 p) {
    if (p == 0) return false;
    if (p >= 0xC0000000u && p <= 0xCFFFFFFFu) return true;
    return p >= 0xE0000000u;
}

u32 scheduler() { return global<be_u32>(kScheduler); }

// Read/write share their body.
bool transfer(u32 fios_fn, be_u32* handle, FsOp* op, u32 data, u64 size, u64 pos, const Options& o) {
    u32 flags = fios_flags(o.w[0]);
    if (gpu_buffer(data)) flags |= kFiosFlagGpuMemory;
    GuestFrame frame(48);
    OpAttr* attr = frame.as<OpAttr>();
    build_attr(*attr, o, op, flags);
    const u32 fop = call<u32>(fios_fn, scheduler(), attr, gref(handle).get(), data, size, pos);
    gref(op).fios_op = fop;
    return fop != 0;
}

}  // namespace

// bool Fios_OpenFile(FsOp* op, FsHandle* handle, const char* path, const char* mode, options in r7-r9)
bool Fios_OpenFile(FsOp* op, be_u32* handle, u32 path, u32 mode, u64 o0, u64 o1, u64 o2) {
    const Options o(o0, o1, o2);
    GuestFrame frame(48);
    OpAttr* attr = frame.as<OpAttr>();
    build_attr(*attr, o, op, fios_flags(o.w[0]));
    u32 open_flags = 1;  // read
    if (call<s32>(fn::Mode_IsRead, mode) == 0) {
        open_flags = 26;  // write | create | truncate
        if (String_HasChar(mode, 'a')) open_flags = 14;  // append
    }
    if (o.w[0] & kFlagExtra) open_flags |= 0x100;
    if (o.w[0] & kFlagExtraOpen) open_flags |= 0x200;
    const u32 fop = call<u32>(fn::fios_OpenFile, scheduler(), attr, path, open_flags, handle);
    gref(op).fios_op = fop;
    return fop != 0;
}

// bool Fios_CloseFile(FsHandle* handle, FsOp* op, options in r5-r7)
bool Fios_CloseFile(be_u32* handle, FsOp* op, u64 o0, u64 o1, u64 o2) {
    const Options o(o0, o1, o2);
    GuestFrame frame(48);
    OpAttr* attr = frame.as<OpAttr>();
    build_attr(*attr, o, op, fios_flags(o.w[0]));
    const u32 fop = call<u32>(fn::fios_CloseFile, scheduler(), attr, gref(handle).get());
    gref(op).fios_op = fop;
    return fop != 0;
}

// bool Fios_ReadFile(FsHandle*, FsOp*, void* buf, u32 size, u64 pos, options in r8-r10)
bool Fios_ReadFile(be_u32* handle, FsOp* op, u32 buf, u64 size, u64 pos, u64 o0, u64 o1, u64 o2) {
    return transfer(fn::fios_ReadFile, handle, op, buf, size, pos, Options(o0, o1, o2));
}

// bool Fios_WriteFile(FsHandle*, FsOp*, const void* data, u32 size, u64 pos, options in r8-r10)
bool Fios_WriteFile(be_u32* handle, FsOp* op, u32 data, u64 size, u64 pos, u64 o0, u64 o1, u64 o2) {
    return transfer(fn::fios_WriteFile, handle, op, data, size, pos, Options(o0, o1, o2));
}

// bool Fios_GetFileSize(FsHandle* handle, FsOp* op, u64* out, options in r6-r8)
// Stats the handle's path (0 if empty).
bool Fios_GetFileSize(be_u32* handle, FsOp* op, u32 out, u64 o0, u64 o1, u64 o2) {
    const Options o(o0, o1, o2);
    GuestFrame frame(48);
    OpAttr* attr = frame.as<OpAttr>();
    build_attr(*attr, o, op, fios_flags(o.w[0]));
    const u32 path = gref<be_u32>(gref(handle) + kFiosHandlePath);
    const u32 fop = call<u32>(fn::fios_Stat, scheduler(), attr, gref<u8>(path) != 0 ? path : 0u, out, 3u);
    gref(op).fios_op = fop;
    return fop != 0;
}

// u32 Fs_Read(FsHandle* handle, void* buf, u32 size, u64 pos, options in r7-r9)
// Synchronous read; returns the bytes read (0 on failure).
u64 Fs_Read(be_u32* handle, u64 buf, u64 size, u64 pos, u64 o0, u64 o1, u64 o2) {
    FsOp* op = static_cast<FsOp*>(memory::Mem_New(sizeof(FsOp)));
    if (!op) return 0;
    FsOp_Init(op, guest(handle), kOpRead, o0, o1);
    if (!Fios_ReadFile(handle, op, static_cast<u32>(buf), size, pos, o0, o1, o2)) {
        FsOp_Destruct(op);
        memory::Mem_FreePool(op, 0, 0);
        return 0;
    }
    FsOp_Sync(op);  // inlined in the original
    const u64 n = FsOp_GetResult(op);
    FsOp_Destruct(op);
    memory::Mem_FreePool(op, 0, 0);
    return n;
}

// FsOp* Fs_ReadAsync(FsHandle* handle, void* buf, u32 size, u64 pos, options in r7-r9)
// Starts a read and returns its FsOp (0 on failure).
FsOp* Fs_ReadAsync(be_u32* handle, u64 buf, u64 size, u64 pos, u64 o0, u64 o1, u64 o2) {
    FsOp* op = static_cast<FsOp*>(memory::Mem_New(sizeof(FsOp)));
    if (!op) return nullptr;
    FsOp_Init(op, guest(handle), kOpRead, o0, o1);
    if (!Fios_ReadFile(handle, op, static_cast<u32>(buf), size, pos, o0, o1, o2)) {
        FsOp_Destruct(op);
        memory::Mem_FreePool(op, 0, 0);
        return nullptr;
    }
    return op;
}

}  // namespace sly4::fsop

NATIVE(00571178, sly4::fsop::Fios_OpenFile)
NATIVE(00571350, sly4::fsop::Fios_CloseFile)
NATIVE(005714B8, sly4::fsop::Fios_ReadFile)
NATIVE(00571688, sly4::fsop::Fios_WriteFile)
NATIVE(00571858, sly4::fsop::Fios_GetFileSize)
NATIVE(00561FE8, sly4::fsop::Fs_Read)
NATIVE(00561E74, sly4::fsop::Fs_ReadAsync)
