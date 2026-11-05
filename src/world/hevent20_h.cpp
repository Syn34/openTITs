// World_HandleEvent callees, part 20h: an object's reference position, the
// pending list flush and the session's part reset.
#include "hevent19.h"

using namespace sly4;
using namespace sly4::hevent;

namespace {
constexpr u32 kObjFindComponent = 0x000D1288;  // (obj, u32 kind, 0) -> Component* (symbols.txt Obj_FindComponent)
constexpr u32 kPosKind3 = 0x0105C360;          // f32[3] reference position for kind 4
constexpr u32 kPosKind6 = 0x0105C420;          // f32[3] reference position for kind 5
constexpr u32 kPending = 0x0106C2C0;           // PtrVector of pending objects
constexpr u32 kPendingFlag = 0x0106C3C0;       // u8 cleared with it
constexpr u32 kObjAddToList = 0x0036341C;      // (Object*) (symbols.txt Obj_AddToList36341C)
constexpr u32 kSessionStop2DDAB0 = 0x002DDAB0; // (Session*) (name?)
constexpr u32 kSetFind = 0x004BCE90;           // (u32 hash) -> Set* (symbols.txt Set_Find)
constexpr u32 kSetStop = 0x004BD1CC;           // (Set*) (symbols.txt Set_Stop4BD1CC)
constexpr u32 kUserBlockReset = 0x0045C7A8;    // (UserBlock*) (symbols.txt UserBlock_Reset45C7A8)
constexpr u32 kLevelSet = 0x7719A548;
constexpr u32 kWorldUser = 4208;               // World + u8 current user
constexpr u32 kWorldUserCount = 29;            // World + u8 number of user blocks

// Copy three f32 words (raw); f1 is left at the last, as lfs loads it.
inline void copy_vec3(PPUContext& ctx, u32 dst, u32 src) {
    for (u32 k = 0; k < 12; k += 4) store<u32>(dst + k, load<u32>(src + k));
    ctx.fpr[1] = hevent7::lfs_f64(src + 8);
}
}  // namespace

// f32* Obj_RefPos26A42C(f32 out[3], Obj* o) (name?): by o +416 bits 28..30:
// 4 -> 0x0105C360; 5 -> 0x0105C420; 6 -> the +64 vec3 of the owner's (+8)
// component 44 when there is one; otherwise (0..3, 7) o +432.
DECOMP(0026A42C, Obj_RefPos26A42C) {
    const u32 out = arg<u32>(ctx, 0), o = arg<u32>(ctx, 1);
    const u32 k = (load<u32>(o + 416) >> 28) & 7;
    if (k == 4) {
        copy_vec3(ctx, out, kPosKind3);
        ctx.gpr[3] = kPosKind3 - 80;
        ctx.gpr[4] = kPosKind3;
        return;
    }
    if (k == 5) {
        copy_vec3(ctx, out, kPosKind6);
        ctx.gpr[3] = kPosKind6 - 80;
        ctx.gpr[4] = kPosKind6;
        return;
    }
    ctx.gpr[3] = k;
    if (k == 6) {
        const u32 owner = load<u32>(o + 8);
        ctx.gpr[3] = owner;
        if (owner != 0) {
            const u32 c = static_cast<u32>(call(ctx, kObjFindComponent, {owner, 44, 0}));
            if (c != 0) {
                copy_vec3(ctx, out, c + 64);
                ctx.gpr[3] = c + 64;
                return;
            }
        }
    }
    copy_vec3(ctx, out, o + 432);
}

// void Pending_Flush367018() (name?): when the pending vector at 0x0106C2C0
// is not empty, four passes (0..3) over it from the back: each object whose
// owner (+8) +112 (0 without an owner) equals the pass gets vt +20 (obj, 1)
// and Obj_AddToList36341C (re-read); then the vector's count and u8
// 0x0106C3C0 are cleared.
DECOMP(00367018, Pending_Flush367018) {
    if (load<u16>(kPending + 8) != 0) {
        for (u32 pass = 0; pass < 4; ++pass) {
            for (s32 i = static_cast<s32>(load<u16>(kPending + 8)) - 1; i >= 0; --i) {
                const u32 e = load<u32>(load<u32>(kPending) + 4 * i);
                const u32 owner = load<u32>(e + 8);
                const u32 key = owner != 0 ? load<u32>(owner + 112) : 0;
                if (static_cast<s32>(key) != static_cast<s32>(pass)) continue;
                vcall(ctx, e, 20, {e, 1});
                call(ctx, kObjAddToList, {load<u32>(load<u32>(kPending) + 4 * i)});
            }
        }
    }
    store<u16>(kPending + 8, 0);
    store<u8>(kPendingFlag, 0);
    ctx.gpr[4] = kPendingFlag + 15424;  // lis r4, 0x107 (left)
    ret(ctx, 0);
}

// void Session_ResetPart2E5060(Session* s) (name?): when s +40 is set: the
// current user's flags word for the part (+36) index (u8 +822) at save
// slot +496 gets bit 0x20 unless already set or s +40's u8 +503 is non-zero
// (stored either way); 0x002DDAB0(s), the level set stopped, +40 / +72 /
// +48 = 0 and every user block reset. r3 is left at the world.
DECOMP(002E5060, Session_ResetPart2E5060) {
    const u32 s = arg<u32>(ctx, 0);
    const u32 cur = load<u32>(s + 40);
    if (cur == 0) {
        ret(ctx, 0);
        return;
    }
    {
        const u32 w = load<u32>(kCurrentWorld);
        const u32 part = load<u32>(s + 36);
        const u32 slot = w + load<u8>(w + kWorldUser) * kSaveSlotSize + 4 * load<u8>(part + 822) + 496;
        const u32 h = load<u16>(slot);
        if ((h & 0x20) == 0) store<u16>(slot, static_cast<u16>(load<u8>(cur + 503) == 0 ? (h | 0x20) : h));
    }
    call(ctx, kSessionStop2DDAB0, {s});
    const u32 set = static_cast<u32>(call(ctx, kSetFind, {kLevelSet}));
    if (set != 0) call(ctx, kSetStop, {set});
    store<u32>(s + 40, 0);
    store<u32>(s + 72, 0);
    store<u32>(s + 48, 0);
    for (u32 i = 0, off = 0;; ++i, off += kUserBlockSize) {
        const u32 w = load<u32>(kCurrentWorld);
        ctx.gpr[3] = w;
        if (!(i < load<u8>(w + kWorldUserCount))) break;
        call(ctx, kUserBlockReset, {w + off + kUserBlocks});
    }
}
