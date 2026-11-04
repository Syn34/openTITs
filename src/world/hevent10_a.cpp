// World_HandleEvent callees, part 10a: small attach-vector, user-block and
// effect helpers.
#include "hevent10.h"

using namespace sly4;
using namespace sly4::hevent10;

namespace {
// Effect (as Effects_UpdateDeferred4CFD1C): +0 def (+33 u8), +4 parent
// effect, +49 / +50 u8 state bytes, +112 child Effect*[] (+120 u16 count),
// +136 flags (bit 10: never done).
constexpr u32 kEffDef = 0, kEffParent = 4, kEffStopped = 49, kEffDone = 50;
constexpr u32 kEffChildren = 112, kEffChildCount = 120, kEffFlags = 136;
constexpr u32 kEffFlagKeep = 0x400;
constexpr u32 kDefShared = 33;
// UserBlock: +80 Entry*[] (+88 u16 count); Entry +2560 u32 id.
constexpr u32 kUbEntries = 80, kUbCount = 88, kEntryId = 2560;
}  // namespace

// AttachRec* Spawner_Find0D155C(Object* o, Object* attached) (name?): the
// record of o's attach vector (o +340) whose +192 is attached, or 0 (also
// for a missing vector).
DECOMP(000D155C, Spawner_Find0D155C) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 want = arg<u32>(ctx, 1);
    const u32 vec = load<u32>(o + kObjAttach);
    for (u32 i = 0, off = 0;; ++i, off += kAttachRecSize) {
        const u32 n = vec != 0 ? load<u16>(vec + kAttachCount) : 0;
        if (!(i < n)) break;
        const u32 rec = load<u32>(vec + kAttachData) + off;
        if (load<u32>(rec + kAttachRecObject) == want) {
            ret(ctx, rec);
            return;
        }
    }
    ret(ctx, 0u);
}

// Entry* UserBlock_Find465F30(UserBlock* ub, u32 id) (name?): the first
// entry of ub +80 (u16 count at +88) whose +2560 is id, or 0.
DECOMP(00465F30, UserBlock_Find465F30) {
    const u32 ub = arg<u32>(ctx, 0);
    const u32 id = arg<u32>(ctx, 1);
    const u32 n = load<u16>(ub + kUbCount);
    const u32 data = load<u32>(ub + kUbEntries);
    for (u32 i = 0; i < n; ++i) {
        const u32 e = load<u32>(data + i * 4);
        if (load<u32>(e + kEntryId) == id) {
            ret(ctx, e);
            return;
        }
    }
    ret(ctx, 0u);
}

// void Obj_MoveTo0D7904(Object* o, const vec3* pos, u8 flag) (name?): the
// world object (o +160) gets WorldObj_SetScale(pos, flag); with flag set,
// every attached object (attach vector o +340, record +192) follows
// recursively. r3 as the original leaves it.
DECOMP(000D7904, Obj_MoveTo0D7904) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 pos = arg<u32>(ctx, 1);
    const u32 flag = arg<u32>(ctx, 2) & 0xFF;
    u64 r3 = load<u32>(o + kObjWorldObj);
    if (r3 != 0) r3 = call(ctx, h10fn::WorldObj_SetScale, {r3, pos, flag});
    if (flag != 0) {
        for (u32 i = 0, off = 0;; ++i, off += kAttachRecSize) {
            const u32 vec = load<u32>(o + kObjAttach);
            r3 = vec;
            const u32 n = vec != 0 ? load<u16>(vec + kAttachCount) : 0;
            if (!(i < n)) break;
            const u32 child = load<u32>(load<u32>(vec + kAttachData) + off + kAttachRecObject);
            if (child != 0) call(ctx, 0x000D7904, {child, pos, flag});
        }
    }
    ctx.gpr[3] = r3;
}

// u8 Effect_Check4CF620(Effect* e, u8 self_only) (name?): whether e and its
// parent chain are done. An effect with flag bit 10 never is (0). Unless
// self_only is set or e's def is shared (+33), every child must pass
// Effect_Check(child, 0) first; then for e's parent (+4): none -> 1,
// otherwise 0 if its +49 is set, else its +50. (The check moves to the
// parent without re-testing the parent's own children.)
DECOMP(004CF620, Effect_Check4CF620) {
    const u32 e = arg<u32>(ctx, 0);
    const u32 self_only = arg<u32>(ctx, 1) & 0xFF;
    if ((load<u32>(e + kEffFlags) & kEffFlagKeep) != 0) {
        ret(ctx, 0u);
        return;
    }
    bool children = false;
    if (self_only == 0) {
        const u32 def = load<u32>(e + kEffDef);
        children = def == 0 || load<u8>(def + kDefShared) != 0;
    }
    if (children) {
        for (u32 i = 0; i < load<u16>(e + kEffChildCount); ++i) {
            const u32 c = load<u32>(load<u32>(e + kEffChildren) + i * 4);
            if (static_cast<u32>(call(ctx, 0x004CF620, {c, 0u})) == 0) {
                ret(ctx, 0u);
                return;
            }
        }
    }
    const u32 p = load<u32>(e + kEffParent);
    if (p == 0) {
        ret(ctx, 1u);
        return;
    }
    ret(ctx, load<u8>(p + kEffStopped) != 0 ? 0u : static_cast<u32>(load<u8>(p + kEffDone)));
}
