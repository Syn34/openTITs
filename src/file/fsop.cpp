// FsOp: wait, cancel, error and cleanup for one FIOS request.
#include "fsop.h"
#include "../memory/mem.h"

namespace sly4::fsop {
using namespace opentit::native;

namespace {

constexpr u32 kFsHandle_Destruct = 0x00571D98;

struct NameEntry {
    be_s32 code;
    be_u32 name;
};

// The fios op: +52 path, +152 done flag, +156 scheduler.
struct FiosOp {
    pad<52> _0;
    be_u32 path;       // +52
    pad<96> _56;
    be_u32 done;       // +152 (read after an isync)
    be_u32 scheduler;  // +156
};
static_assert(offsetof(FiosOp, scheduler) == kFiosOpScheduler);

FiosOp& Fop(const FsOp* o) { return gref<FiosOp>(gref(o).fios_op.get()); }

// fios_Wait(1, &fop, 0): waits for one FIOS op.
s32 wait_one(u32 fop) {
    GuestFrame frame(8);
    be_u32* ops = frame.as<be_u32>();
    *ops = fop;
    return call<s32>(fn::fios_Wait, 1u, ops, 0u);
}

}  // namespace

// void FsOp_Init(FsOp* op, u32 target, u32 kind, FsOptions options in r6-r8)
void FsOp_Init(FsOp* op, u32 target, u32 kind, u64 o0, u64 o1) {
    FsOp& f = gref(op);
    f.fios_op = 0u;
    f.kind = kind;
    f.target = target;
    f.opt2 = static_cast<u32>(o1 >> 32);
    f.opt3 = static_cast<u32>(o1);
    f.report = (static_cast<u32>(o0 >> 32) & 0x10) == 0 ? 1 : 0;
}

// s32 FsOp_Wait(FsOp* op)
s32 FsOp_Wait(FsOp* op) { return wait_one(gref(op).fios_op); }

// void FsOp_Cancel(FsOp* op) (the r3 fios_Cancel leaves is passed on)
u32 FsOp_Cancel(FsOp* op) {
    const u32 fop = gref(op).fios_op;
    return call<u32>(fn::fios_Cancel, gref<FiosOp>(fop).scheduler.get(), fop);
}

// s32 FsOp_GetError(FsOp* op): 0x80010701 while not yet issued, else its status.
s32 FsOp_GetError(FsOp* op) {
    if (Fop(op).done == 0) return kErrBusy;
    return wait_one(gref(op).fios_op);
}

// const char* FsOp_ErrorName(FsOp* op): the name of the op's status, or
// "Unknown:0x%x" (String_Format's rotating buffer).
u32 FsOp_ErrorName(FsOp* op) {
    const u32 err = static_cast<u32>(FsOp_GetError(op));
    const NameEntry* names = host<NameEntry>(kErrorNames);
    for (u32 i = 0; i < 24; ++i)
        if (static_cast<u32>(names[i].code.get()) == err) return names[i].name;
    return call<u32>(fn::String_Format, kStrUnknownError, err);
}

// u32 FsOp_GetResult(FsOp* op): bytes transferred.
u32 FsOp_GetResult(FsOp* op) { return call<u32>(fn::fios_GetResult, gref(op).fios_op.get()); }

// const char* FsOp_GetPath(FsOp* op): 0 if empty.
u32 FsOp_GetPath(FsOp* op) {
    const u32 path = Fop(op).path;
    return gref<u8>(path) != 0 ? path : 0u;
}

// void FsOp_Destruct(FsOp* op) (returns fios_Free's r3, or 0)
u32 FsOp_Destruct(FsOp* op) {
    FsOp_Release(op);
    const u32 fop = gref(op).fios_op;
    if (fop == 0) return 0;
    return call<u32>(fn::fios_Free, gref<FiosOp>(fop).scheduler.get(), fop);
}

// void FsOp_Sync(FsOp* op)
// Waits for the op; if it failed (and wants reporting) formats the report:
// "Unknown:0x%x" for an unknown kind, the path and the error name.
void FsOp_Sync(FsOp* op) {
    FsOp_Wait(op);
    FsOp& f = gref(op);
    if (f.report == 0) return;
    f.report = 0;
    const s32 err = FsOp_GetError(op);
    if (err == 0 || err == kErrIgnore) return;
    const s32 kind = static_cast<s32>(f.kind.get());
    const NameEntry* names = host<NameEntry>(kKindNames);
    bool known = false;
    for (u32 i = 0; i < 7 && !known; ++i) known = names[i].code == kind;
    if (!known) call(fn::String_Format, kStrUnknownKind, kind);
    FsOp_GetPath(op);
    FsOp_ErrorName(op);
}

// void FsOp_Release(FsOp* op)
// For opens and closes: cancels / syncs an op still in flight, then frees
// the FsHandle after a failed open or a successful close.
void FsOp_Release(FsOp* op) {
    FsOp& f = gref(op);
    const u32 kind = f.kind;
    if (kind != kOpOpen && kind != kOpClose) return;
    s32 err = FsOp_GetError(op);
    if (err == kErrBusy) {
        FsOp_Cancel(op);
        FsOp_Sync(op);  // inlined in the original
        err = FsOp_GetError(op);
    }
    auto free_handle = [&] {
        const u32 h = f.target;
        if (h == 0) return;
        FsHandle_Destruct(host<be_u32>(h));
        memory::Mem_FreePool(host<u8>(h), 0, 0);
    };
    if (err != 0 && f.kind == kOpOpen) free_handle();
    if (err == 0 && f.kind == kOpClose) free_handle();
}

// FsHandle::FsHandle(): { fios filehandle* fh = 0; }
void FsHandle_Construct(be_u32* h) { gref(h) = 0u; }

// FsHandle::~FsHandle(): empty.
void FsHandle_Destruct(be_u32*) {}

// bool String_HasChar(const char* s, char c)
bool String_HasChar(u32 s, s8 c) { return call<u32>(fn::strchr, s, c) != 0; }

}  // namespace sly4::fsop

NATIVE(00570B9C, sly4::fsop::FsOp_Init)
NATIVE(00570BD4, sly4::fsop::FsOp_Wait)
NATIVE(00570C0C, sly4::fsop::FsOp_Cancel)
NATIVE(00570C38, sly4::fsop::FsOp_GetError)
NATIVE(00570C98, sly4::fsop::FsOp_ErrorName)
NATIVE(00570D54, sly4::fsop::FsOp_GetResult)
NATIVE(00570D80, sly4::fsop::FsOp_GetPath)
NATIVE(00570DA8, sly4::fsop::FsOp_Destruct)
NATIVE(00561450, sly4::fsop::FsOp_Sync)
NATIVE(00561514, sly4::fsop::FsOp_Release)
NATIVE(00571D80, sly4::fsop::FsHandle_Construct)
NATIVE(00571D98, sly4::fsop::FsHandle_Destruct)
NATIVE(00561270, sly4::fsop::String_HasChar)
