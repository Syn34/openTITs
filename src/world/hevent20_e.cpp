// World_HandleEvent callees, part 20e: a two-slot ref setter, an object
// constructor, the sound pause group mask, the session reset, an object
// slot's release and the handle map broadcast.
#include "hevent19.h"

using namespace sly4;
using namespace sly4::hevent;

namespace {
constexpr u32 kInit342E00 = 0x00342E00;        // () (reads no argument registers) (name?)
constexpr u32 kStart342A30 = 0x00342A30;       // (Obj*) (name?)
constexpr u32 kVtable3430F8 = 0x00B1F328;      // the constructed object's vtable
constexpr u32 kDefaultPos = 0x01164B40;        // f32[3] default position (as hevent20_a.cpp)
constexpr u32 kSndPauseGroups = 0x006D530C;    // (u32 bits) (symbols.txt Snd_PauseGroups)
constexpr u32 kSndResumeGroups = 0x006D53F0;   // (u32 bits) (symbols.txt Snd_ResumeGroups)
constexpr u32 kPauseMask = 0x010AF1C0;         // u32 requested pause bits
constexpr u32 kPauseEnabled = 0x010AEC60;      // u8: the mask is applied
constexpr u32 kPauseAll = 0x010AF1BD;          // u8: every group paused
constexpr u32 kPauseBit1 = 0x010AF1C4;         // u8: group bit 0x2 paused
constexpr u32 kPauseApplied = 0x00F65BE0;      // u32 bits currently paused
constexpr u32 kSessionStop2DDAB0 = 0x002DDAB0; // (Session*, u8) (name?)
constexpr u32 kSetFind = 0x004BCE90;           // (u32 hash) -> Set* (symbols.txt Set_Find)
constexpr u32 kSetStop = 0x004BD1CC;           // (Set*) (symbols.txt Set_Stop4BD1CC)
constexpr u32 kUserBlockReset = 0x0045C7A8;    // (UserBlock*) (symbols.txt UserBlock_Reset45C7A8)
constexpr u32 kLevelSet = 0x7719A548;          // the level set's hash
constexpr u32 kWorldUserCount = 29;            // World + u8 number of user blocks
constexpr u32 kObjectsReset = 0x0035B470;      // (Highlight*) (symbols.txt Objects_Reset35B470)
constexpr u32 kHighlight = 0x0106C330;         // its argument
constexpr u32 kBroadcastFlag = 0x00F64CC4;     // u8: the broadcast's first argument

// Apply the pause request mask (0x010AF1C0) when enabled: the groups newly
// paused go to Snd_PauseGroups, those released to Snd_ResumeGroups.
inline void apply_pause(PPUContext& ctx, u64 req) {
    store<u32>(kPauseMask, static_cast<u32>(req));
    if (load<u8>(kPauseEnabled) == 0) {
        ctx.gpr[3] = req;
        return;
    }
    u64 want = (load<u8>(kPauseAll) != 0 ? ~0ull : 0ull) | req;
    if (load<u8>(kPauseBit1) != 0) want |= 2;
    const u32 cur = load<u32>(kPauseApplied);
    const u64 diff = want ^ cur;
    const u32 resume = static_cast<u32>(diff & cur);
    const u32 pause = static_cast<u32>(diff & ~static_cast<u64>(cur));
    if (pause != 0) call(ctx, kSndPauseGroups, {pause, diff, ~static_cast<u64>(cur)});
    if (resume != 0) call(ctx, kSndResumeGroups, {resume});
    store<u32>(kPauseApplied, static_cast<u32>(want));
    ctx.gpr[3] = kPauseApplied & 0xFFFF0000u;  // lis r3, 0xF6 (left)
}
}  // namespace

// void Obj_SetRefs108A34(Obj* o, Ref* x) (name?): o +12 = x (old vt +40
// (old, 0)); x's u16 +178 incremented; when x is 0, +16 is released the same
// way and cleared.
DECOMP(00108A34, Obj_SetRefs108A34) {
    const u32 o = arg<u32>(ctx, 0), x = arg<u32>(ctx, 1);
    const u32 old = load<u32>(o + 12);
    if (old != 0) vcall(ctx, old, 40, {old, 0});
    store<u32>(o + 12, x);
    if (x != 0) {
        const u32 c = load<u16>(x + 178) + 1;
        store<u16>(x + 178, static_cast<u16>(c));
        ctx.gpr[4] = c;
        ret(ctx, x + 178);
        return;
    }
    const u32 b = load<u32>(o + 16);
    if (b != 0) {
        vcall(ctx, b, 40, {b, 0});
        store<u32>(o + 16, 0);
    }
    ret(ctx, 0);
}

// void Obj_Construct3430F8(Obj* o, u8 start) (name?): vtable 0x00B1F328,
// +16 / +28 / +40 / +52 / +64 = 0x80000000, the other words up to +60 and
// +96 zeroed, +80 the default position (0x01164B40); then 0x00342E00() and,
// when start is set, 0x00342A30(o).
DECOMP(003430F8, Obj_Construct3430F8) {
    const u32 o = arg<u32>(ctx, 0), start = arg<u32>(ctx, 1);
    constexpr u32 kTop = 0x80000000u;
    store<u32>(o + 16, kTop);
    store<u32>(o + 28, kTop);
    store<u32>(o, kVtable3430F8);
    store<u32>(o + 4, 0);
    store<u32>(o + 8, 0);
    store<u32>(o + 12, 0);
    store<u32>(o + 20, 0);
    store<u32>(o + 24, 0);
    store<u32>(o + 32, 0);
    store<u32>(o + 36, 0);
    store<u32>(o + 40, kTop);
    store<u32>(o + 80, load<u32>(kDefaultPos));
    store<u32>(o + 44, 0);
    store<u32>(o + 48, 0);
    store<u32>(o + 52, kTop);
    store<u32>(o + 56, 0);
    store<u32>(o + 60, 0);
    store<u32>(o + 64, kTop);
    store<u32>(o + 84, load<u32>(kDefaultPos + 4));
    store<u32>(o + 96, 0);
    store<u32>(o + 88, load<u32>(kDefaultPos + 8));
    // r3..r7 / f1 as the original leaves them; 0x00342E00 reads none.
    ctx.fpr[1] = hevent7::lfs_f64(kDefaultPos + 8);
    call(ctx, kInit342E00, {kVtable3430F8, kDefaultPos & 0xFFFF0000u, kTop, 0, kDefaultPos});
    if ((start & 0xFF) != 0) call(ctx, kStart342A30, {o});
    else ctx.gpr[3] = 0;
}

// void Pause_Request44BB48(u32 bits) (name?): the request mask |= bits, then
// applied (the sound groups paused / resumed to match).
DECOMP(0044BB48, Pause_Request44BB48) {
    apply_pause(ctx, static_cast<u64>(load<u32>(kPauseMask)) | ctx.gpr[3]);
}

// void Pause_Release44BC10(u32 bits) (name?): the request mask &= ~bits,
// then applied as 0x0044BB48.
DECOMP(0044BC10, Pause_Release44BC10) {
    apply_pause(ctx, static_cast<u64>(load<u32>(kPauseMask)) & ~ctx.gpr[3]);
}

// void Session_Reset2E0860(Session* s, u8 keep) (name?): 0x002DDAB0(s,
// keep), the level set stopped, +40 / +72 / +48 = 0, every user block of the
// world reset (UserBlock_Reset45C7A8); +36 = 0 unless keep.
DECOMP(002E0860, Session_Reset2E0860) {
    const u32 s = arg<u32>(ctx, 0);
    const u64 keep = ctx.gpr[4];
    call(ctx, kSessionStop2DDAB0, {s, keep});
    const u32 set = static_cast<u32>(call(ctx, kSetFind, {kLevelSet}));
    if (set != 0) call(ctx, kSetStop, {set});
    store<u32>(s + 40, 0);
    store<u32>(s + 72, 0);
    store<u32>(s + 48, 0);
    for (u32 i = 0, off = 0;; ++i, off += kUserBlockSize) {
        const u32 w = load<u32>(kCurrentWorld);
        if (!(i < load<u8>(w + kWorldUserCount))) break;
        call(ctx, kUserBlockReset, {w + off + kUserBlocks});
    }
    if ((keep & 0xFF) == 0) {
        store<u32>(s + 36, 0);
        ret(ctx, 0);
        return;
    }
    ret(ctx, keep & 0xFF);
}

// void Obj_SlotSet36901C(Obj* o, u32 x, u32 i) (name?): when x is set and i
// is below the definition's (+8) count (+152): the 16-byte entry i (+148)
// object (+12) gets vt +188 (obj, x, 0), then (re-read) vt +148 (obj).
DECOMP(0036901C, Obj_SlotSet36901C) {
    const u32 d = load<u32>(arg<u32>(ctx, 0) + 8);
    const u32 x = arg<u32>(ctx, 1), i = arg<u32>(ctx, 2);
    ctx.gpr[3] = ctx.gpr[4];
    if (x == 0) return;
    if (!(i < load<u32>(d + 152))) return;
    const u32 e = load<u32>(load<u32>(d + 148) + (i << 4) + 12);
    if (e == 0) return;
    vcall(ctx, e, 188, {e, x, 0});
    const u32 e2 = load<u32>(load<u32>(d + 148) + (i << 4) + 12);
    vcall(ctx, e2, 148, {e2});
}

// void Game_SetPaused(u8 a, u8 b) (name?): u8 0x00F64CC4 = a; every
// object of the handle map (0x0106C228) gets vt +24 (obj, a, b); then, when
// 0x00F64CC4 is (still) 0, Objects_Reset35B470(0x0106C330).
DECOMP(00365A54, Game_SetPaused) {
    const u32 a = arg<u32>(ctx, 0) & 0xFF, b = arg<u32>(ctx, 1) & 0xFF;
    store<u8>(kBroadcastFlag, static_cast<u8>(a));
    for (s32 i = 0; i < static_cast<s32>(load<u16>(kHandleMap + 2)); ++i) {
        const u32 obj = load<u32>(load<u32>(kHandleMap + 8) + 8 * i + 4);
        vcall(ctx, obj, 24, {obj, a, b});
    }
    const u32 f = load<u8>(kBroadcastFlag);
    if (f != 0) {
        ret(ctx, f);
        return;
    }
    call(ctx, kObjectsReset, {kHighlight});
}
