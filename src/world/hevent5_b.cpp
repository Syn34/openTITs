// World_HandleEvent callees, part 5: 144..145 instructions.
#include "hevent.h"

using namespace sly4;
using namespace sly4::hevent;

namespace sly4 {
namespace hevent5b {
namespace bfn {
constexpr u32 FsOp_Wait5612B8 = 0x005612B8;   // (FsOp*) (name?)
constexpr u32 FsOp_GetError = 0x00570C38;     // (FsOp*) -> s32
constexpr u32 FsOp_GetPath = 0x00570D80;      // (FsOp*) -> const char*
constexpr u32 FsOp_ErrorName = 0x00570C98;    // (FsOp*) -> const char*
constexpr u32 FsOp_Destruct = 0x00570DA8;     // (FsOp*)
constexpr u32 FsOp_Begin5627D4 = 0x005627D4;  // (path, 0, s64 size, 24-byte options by value) -> FsOp* (name?)
constexpr u32 Mem_FreeFrom = 0x0054EE6C;      // (p, pool)
constexpr u32 Ui_IsBusy3695A0 = 0x003695A0;   // () -> nonzero blocks (name?)
constexpr u32 Handle_ResolveBit40 = 0x00365EAC;
constexpr u32 Game_CanAct2AB3E0 = 0x002AB3E0; // () -> nonzero to go on (name?)
}  // namespace bfn

// The registry 0x010686A8: +8 {key, Object*}[] of 8 bytes, count at +24.
constexpr u32 kRegistry = 0x010686A8, kRegistryCount = 0x010686C0;
constexpr u32 kStreamOp = 0x00F64C34;   // u32 FsOp*: the running .ftd load (name?)
constexpr u32 kFtdPaths = 0x0106870C;   // PtrVector of char*: +0 data, +8 u16 count
constexpr u32 kSizeAll2E9680 = 0x002E9680;  // s64 0x7FFFFFFFFFFFFFFF
constexpr u32 kFsOpCancelled = 0x80010708u, kFsOpNotFound = 0x80010701u;  // errors not reported (name?)
constexpr u32 kFsOpDone = 152;          // FsOp vtable? +0 -> +152 u32: nonzero when finished (name?)

constexpr u32 kSession = 0x00F64C20;
constexpr u32 kPlayerHandle = 0x00F64D04;  // u32 handle (name?)
constexpr u32 kFadeUi = 0x00F65C88;        // u32 Ui* of "Common.Fade"
constexpr u32 kWorldCutscene = 0x128224;
constexpr u32 kWorldLock128221 = 0x128221;  // u8 (name?)
constexpr u32 kWorldBusy = 0x1282A0;        // u32: 0x40000000 = busy
constexpr u32 kWorldChild1281F4 = 0x1281F4; // u32 Object*: vfunc +68(o, flag) decides last

// Virtual call with the OPD and its entry left in the given registers, as the
// original sequence leaves them.
inline u64 vcall_r(PPUContext& ctx, u32 obj, u32 slot, std::initializer_list<u64> args, int ropd, int rent) {
    const u32 opd = load<u32>(load<u32>(obj) + slot);
    ctx.gpr[ropd] = opd;
    ctx.gpr[rent] = load<u32>(opd);
    return call_ptr(ctx, opd, args);
}
}  // namespace hevent5b
}  // namespace sly4

using namespace sly4::hevent5b;

// void Streaming_Update2E9688(f32 dt) (name?): every registry object gets vfunc
// +8(o, -, 0; dt). Then the running .ftd load (0x00F64C34), once its done flag
// is up, is waited for (FsOp_Wait5612B8), its error looked at (path and error
// name are fetched, unused, for errors other than 0x80010708 / 0x80010701),
// destroyed and freed, and 0x00F64C34 cleared. With no load running, the last
// path of the .ftd vector is popped and, when non-null, started with
// FsOp_Begin5627D4(path, 0, INT64_MAX, {16, 0, 0, 0, 0, 0}) into 0x00F64C34; the
// path string is freed.
DECOMP(002E9688, Streaming_Update2E9688) {
    const f64 dt = ctx.fpr[1];
    for (u32 i = 0; i < load<u32>(kRegistryCount); ++i) {
        const u32 o = load<u32>(load<u32>(kRegistry + 8) + i * 8 + 4);
        const u32 opd = load<u32>(load<u32>(o) + 8);
        ctx.fpr[1] = dt;
        ctx.gpr[6] = load<u32>(opd);  // the original leaves the entry in r6
        call_ptr(ctx, opd, {o, opd, 0});
    }
    const u32 op = load<u32>(kStreamOp);
    if (op != 0) {
        if (load<u32>(load<u32>(op) + kFsOpDone) == 0) {
            ret(ctx, op);  // r3 as the original leaves it
            return;
        }
        call(ctx, bfn::FsOp_Wait5612B8, {op});
        const u32 err = static_cast<u32>(call(ctx, bfn::FsOp_GetError, {load<u32>(kStreamOp)}));
        if (err != 0 && err != kFsOpCancelled && err != kFsOpNotFound) {
            // The original fetches these for a message that isn't printed in this build.
            call(ctx, bfn::FsOp_GetPath, {load<u32>(kStreamOp)});
            call(ctx, bfn::FsOp_ErrorName, {load<u32>(kStreamOp)});
        }
        const u32 p = load<u32>(kStreamOp);
        if (p != 0) {
            call(ctx, bfn::FsOp_Destruct, {p});
            call(ctx, bfn::Mem_FreeFrom, {p, 0});
            store<u32>(kStreamOp, 0);
            ret(ctx, kStreamOp & 0xFFFF0000u);  // r3 as the original leaves it
        }
        return;
    }
    const u32 n = load<u16>(kFtdPaths + 8);
    if (n == 0) {
        ret(ctx, kFtdPaths);  // r3 as the original leaves it
        return;
    }
    // Pop the last entry. (The original then moves the last entry into the
    // popped slot, which is the same slot, so nothing changes.)
    const u32 data = load<u32>(kFtdPaths);
    store<u16>(kFtdPaths + 8, static_cast<u16>(n - 1));
    const u32 path = load<u32>(data + (n - 1) * 4);
    if (path == 0) {
        ret(ctx, data);  // r3 as the original leaves it
        return;
    }
    const u64 opts0 = static_cast<u64>(16) << 32;  // {16, 0, 0, 0, 0, 0} by value in r6..r8
    const u32 nop = static_cast<u32>(call(ctx, bfn::FsOp_Begin5627D4, {path, 0, load<u64>(kSizeAll2E9680), opts0, 0, 0}));
    store<u32>(kStreamOp, nop);
    call(ctx, bfn::Mem_FreeFrom, {path, 0});
}

// bool World_CanStartAction(World* w, u8 flag) (name?): 0 while a cutscene runs,
// while the world's vfunc +32(w, 0) is 1 and vfunc +32(w, 1) is 12, while
// Ui_IsBusy3695A0(), +0x128221 is set, the session (+76 set or +16 bit 29) says
// so, or the player's object (+24) has +164 bit 15. Without flag: 0 also while
// the world is busy (0x1282A0 bit 30) or the fade UI is up (vfunc +84) and its
// vfunc +52 state has byte 3 set. Then 0 unless Game_CanAct2AB3E0(); with a
// child at +0x1281F4 its vfunc +68(child, flag) decides, else 1.
DECOMP(0048981C, World_CanStartAction) {
    const u32 w = arg<u32>(ctx, 0);
    const u32 flag = arg<u32>(ctx, 1) & 0xFF;
    if (load<u32>(w + kWorldCutscene) != 0) {
        ret(ctx, 0);
        return;
    }
    if (static_cast<s32>(vcall_r(ctx, w, 32, {w, 0}, 5, 6)) == 1 &&
        static_cast<s32>(vcall_r(ctx, w, 32, {w, 1}, 5, 6)) == 12) {
        ret(ctx, 0);
        return;
    }
    if (call(ctx, bfn::Ui_IsBusy3695A0) != 0 || load<u8>(w + kWorldLock128221) != 0) {
        ret(ctx, 0);
        return;
    }
    const u32 s = load<u32>(kSession);
    if (load<u32>(s + 76) != 0 || (load<u32>(s + 16) & 0x20000000u)) {
        ret(ctx, 0);
        return;
    }
    const u32 h = static_cast<u32>(call(ctx, bfn::Handle_ResolveBit40, {load<u32>(kPlayerHandle)}));
    if (h != 0) {
        const u32 o = load<u32>(h + 24);
        if (o != 0 && (load<u32>(o + 164) & 0x00008000u)) {
            ret(ctx, 0);
            return;
        }
    }
    if (flag == 0) {
        if (load<u32>(w + kWorldBusy) & 0x40000000u) {
            ret(ctx, 0);
            return;
        }
        const u32 fade = load<u32>(kFadeUi);
        if (fade != 0 && vcall_r(ctx, fade, 84, {fade}, 4, 5) != 0) {
            Stack frame(ctx, 16);
            vcall_r(ctx, fade, 52, {frame.addr(), fade}, 5, 6);
            if (load<u8>(frame.addr() + 3) != 0) {
                ret(ctx, 0);
                return;
            }
        }
    }
    if (call(ctx, bfn::Game_CanAct2AB3E0) == 0) {
        ret(ctx, 0);
        return;
    }
    const u32 c = load<u32>(w + kWorldChild1281F4);
    if (c == 0) {
        ret(ctx, 1);
        return;
    }
    vcall_r(ctx, c, 68, {c, flag}, 5, 6);
}
