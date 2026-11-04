// FsOp: wait, cancel, error and cleanup for one FIOS request.
#include "fsop.h"

using namespace sly4;
namespace op = sly4::fsop;

namespace {

// fios_Wait(1, &fop, 0): waits for one FIOS op.
s32 wait_one(PPUContext& ctx, u32 fop) {
    Stack frame(ctx, 8);
    store<u32>(frame.addr(), fop);
    return static_cast<s32>(call(ctx, op::fn::fios_Wait, {1, frame.addr(), 0}));
}

// FsOp_GetError's body: busy until the op has been issued, else its status.
s32 get_error(PPUContext& ctx, u32 o) {
    const u32 fop = load<u32>(o);
    if (load<u32>(fop + op::kFiosOpDone) == 0) return op::kErrBusy;  // read after an isync
    return wait_one(ctx, fop);
}

}  // namespace

// void FsOp_Init(FsOp* op, u32 target, u32 kind, FsOptions options in r6-r8)
DECOMP(00570B9C, FsOp_Init) {
    const u32 o = arg<u32>(ctx, 0);
    const u64 o0 = ctx.gpr[6], o1 = ctx.gpr[7];
    op::FsOp& f = ref<op::FsOp>(o);
    f.fios_op = 0;
    f.kind = arg<u32>(ctx, 2);
    f.target = arg<u32>(ctx, 1);
    f.opt2 = static_cast<u32>(o1 >> 32);
    f.opt3 = static_cast<u32>(o1);
    f.report = (static_cast<u32>(o0 >> 32) & 0x10) == 0 ? 1 : 0;
}

// s32 FsOp_Wait(FsOp* op)
DECOMP(00570BD4, FsOp_Wait) {
    ret(ctx, wait_one(ctx, load<u32>(arg<u32>(ctx, 0))));
}

// void FsOp_Cancel(FsOp* op)
DECOMP(00570C0C, FsOp_Cancel) {
    const u32 fop = load<u32>(arg<u32>(ctx, 0));
    ctx.gpr[3] = call(ctx, op::fn::fios_Cancel, {load<u32>(fop + op::kFiosOpScheduler), fop});
}

// s32 FsOp_GetError(FsOp* op): 0x80010701 while not yet issued.
DECOMP(00570C38, FsOp_GetError) {
    ret(ctx, get_error(ctx, arg<u32>(ctx, 0)));
}

// const char* FsOp_ErrorName(FsOp* op): the name of the op's status, or
// "Unknown:0x%x" (String_Format's rotating buffer).
DECOMP(00570C98, FsOp_ErrorName) {
    const u32 err = static_cast<u32>(get_error(ctx, arg<u32>(ctx, 0)));
    for (u32 i = 0; i < 24; ++i)
        if (load<u32>(op::kErrorNames + 8 * i) == err) return ret(ctx, load<u32>(op::kErrorNames + 8 * i + 4));
    ctx.gpr[3] = call(ctx, op::fn::String_Format, {op::kStrUnknownError, err});
}

// u32 FsOp_GetResult(FsOp* op): bytes transferred.
DECOMP(00570D54, FsOp_GetResult) {
    ret(ctx, static_cast<u32>(call(ctx, op::fn::fios_GetResult, {load<u32>(arg<u32>(ctx, 0))})));
}

// const char* FsOp_GetPath(FsOp* op): 0 if empty.
DECOMP(00570D80, FsOp_GetPath) {
    const u32 path = load<u32>(load<u32>(arg<u32>(ctx, 0)) + op::kFiosOpPath);
    ret(ctx, ref<u8>(path) != 0 ? path : 0u);
}

// void FsOp_Destruct(FsOp* op)
DECOMP(00570DA8, FsOp_Destruct) {
    const u32 o = arg<u32>(ctx, 0);
    call(ctx, op::fn::FsOp_Release, {o});
    const u32 fop = load<u32>(o);
    if (fop == 0) return ret(ctx, 0u);
    ctx.gpr[3] = call(ctx, op::fn::fios_Free, {load<u32>(fop + op::kFiosOpScheduler), fop});
}

// void FsOp_Sync(FsOp* op)
// Waits for the op; if it failed (and wants reporting) formats the report:
// "Unknown:0x%x" for an unknown kind, the path and the error name.
DECOMP(00561450, FsOp_Sync) {
    const u32 o = arg<u32>(ctx, 0);
    call(ctx, op::fn::FsOp_Wait, {o});
    if (ref<u8>(o + 20) == 0) return;
    ref<u8>(o + 20) = 0;
    const s32 err = static_cast<s32>(call(ctx, op::fn::FsOp_GetError, {o}));
    if (err == 0 || err == op::kErrIgnore) return;
    const s32 kind = load<s32>(o + 4);
    bool known = false;
    for (u32 i = 0; i < 7 && !known; ++i) known = load<s32>(op::kKindNames + 8 * i) == kind;
    if (!known) call(ctx, op::fn::String_Format, {op::kStrUnknownKind, static_cast<u64>(static_cast<s64>(kind))});
    call(ctx, op::fn::FsOp_GetPath, {o});
    ctx.gpr[3] = call(ctx, op::fn::FsOp_ErrorName, {o});
}

// void FsOp_Release(FsOp* op)
// For opens and closes: cancels / syncs an op still in flight, then frees
// the FsHandle after a failed open or a successful close.
DECOMP(00561514, FsOp_Release) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 kind = load<u32>(o + 4);
    if (kind != op::kOpOpen && kind != op::kOpClose) return;
    s32 err = static_cast<s32>(call(ctx, op::fn::FsOp_GetError, {o}));
    if (err == op::kErrBusy) {
        call(ctx, op::fn::FsOp_Cancel, {o});
        call(ctx, op::fn::FsOp_Sync, {o});  // inlined in the original
        err = static_cast<s32>(call(ctx, op::fn::FsOp_GetError, {o}));
    }
    auto free_handle = [&] {
        const u32 h = load<u32>(o + 8);
        if (h == 0) return;
        call(ctx, 0x00571D98, {h});  // FsHandle_Destruct
        call(ctx, op::fn::Mem_FreePool, {h, 0, 0});
    };
    if (err != 0 && load<u32>(o + 4) == op::kOpOpen) free_handle();
    if (err == 0 && load<u32>(o + 4) == op::kOpClose) free_handle();
}

// FsHandle::FsHandle(): { fios filehandle* fh = 0; }
DECOMP(00571D80, FsHandle_Construct) {
    store<u32>(arg<u32>(ctx, 0), 0);
    ctx.gpr[4] = 0;
}

// FsHandle::~FsHandle(): empty.
DECOMP(00571D98, FsHandle_Destruct) {}

// bool String_HasChar(const char* s, char c)
DECOMP(00561270, String_HasChar) {
    const u64 c = static_cast<u64>(static_cast<s64>(arg<s8>(ctx, 1)));
    ret(ctx, call(ctx, op::fn::strchr, {arg<u32>(ctx, 0), c}) != 0 ? 1u : 0u);
}
