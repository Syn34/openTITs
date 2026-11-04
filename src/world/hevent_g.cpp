// World_HandleEvent callees, 48..49 instructions.
#include "hevent.h"

using namespace sly4;
using namespace sly4::hevent;

namespace {
namespace gfn {
constexpr u32 sub_0034E4C8 = 0x0034E4C8;      // (Object*)
constexpr u32 sys_ppu_thread_create = 0x00ABF144;  // import stub
constexpr u32 sub_00447B70 = 0x00447B70;      // () after the push stack changes (name?)
constexpr u32 sub_00362F1C = 0x00362F1C;      // (Object*, u8, u8)
}  // namespace gfn

constexpr u32 kObjList106BC70 = 0x0106BC70;   // {u32 data, u16 count +8} (name?)
constexpr u32 kObjListOn = 0x0106BBE4;        // u8 (name?)
constexpr u32 kSaveTaskState = 0x00F9D1A8;    // u32 shared by the save threads: 0 idle, 1 running, 2 ok, 3 failed? (name?)
constexpr u32 kSaveTaskBuffer = 0x010127A0;   // data a finished task hands back (name?)
constexpr u32 memmove = 0x0071C3F0;
constexpr u32 kDeleteGameFlag = 0x01012788;   // u8 cleared when the thread starts (name?)
constexpr u32 kDeleteGameEntry = 0x00F30FE0;  // thread entry OPD
constexpr u32 kDeleteGameName = 0x00AC5FDC;   // "DeleteGame"
constexpr u32 kPushCount = 0x010AFE90, kPushStack = 0x010AFB84;  // see PushStack_Push
constexpr u32 kSaveTaskPrio = 1001, kSaveTaskStack = 16384;

// The polls of the save threads: state 0 starts the thread (state 1); 2 and 3
// are returned once and reset to 0, 2 first copying `size` bytes of the
// task's buffer to `dst`; other states are returned as they are.
void SaveTaskPoll(PPUContext& ctx, u32 entry, u32 name, u32 dst, u32 size, bool clear_flag) {
    const s32 s = static_cast<s32>(load<u32>(kSaveTaskState));
    if (s == 3) {
        store<u32>(kSaveTaskState, 0);
        ctx.gpr[4] = 0x00FA0000u;
        ctx.gpr[5] = 0;
        return ret(ctx, 3);
    }
    if (s == 2) {
        store<u32>(kSaveTaskState, 0);
        if (size != 0) {
            ctx.gpr[6] = 0x00FA0000u;
            ctx.gpr[7] = 0;
            call(ctx, memmove, {dst, kSaveTaskBuffer, size});
        } else {
            ctx.gpr[4] = 0x00FA0000u;
            ctx.gpr[5] = 0;
        }
        return ret(ctx, 2);
    }
    if (s != 0) {
        if (!clear_flag) ctx.gpr[4] = static_cast<u64>(static_cast<s64>(s));  // the two others keep it in r4
        return ret(ctx, s);
    }
    store<u32>(kSaveTaskState, 1);
    if (clear_flag) ref<u8>(kDeleteGameFlag) = 0;
    Stack frame(ctx, 16);  // sys_ppu_thread_t out
    call(ctx, gfn::sys_ppu_thread_create, {frame.addr(), entry, 0, kSaveTaskPrio, kSaveTaskStack, 0, name});
    const s32 now = static_cast<s32>(load<u32>(kSaveTaskState));
    if (!clear_flag) ctx.gpr[4] = static_cast<u64>(static_cast<s64>(now));
    ret(ctx, now);
}
}  // namespace

// void Objects_Flush34E4C8(): while byte 0x0106BBE4 is set, every object of the
// list at 0x0106BC70 with u16 +40 bit 0x100 gets sub_0034E4C8(obj); then bits
// 0x8000 and 0x100 of +40 are cleared on all of them (name?)
DECOMP(0034F3A0, Objects_Flush34E4C8) {
    if (ref<u8>(kObjListOn) == 0) return ret(ctx, 0u);
    for (u32 i = 0; i < load<u16>(kObjList106BC70 + 8); ++i) {
        const u32 obj = load<u32>(load<u32>(kObjList106BC70) + i * 4);
        u32 h = load<u16>(obj + 40);
        const u32 bit = (h >> 8) & 1;
        ctx.gpr[4] = bit;
        if (bit != 0) {
            call(ctx, gfn::sub_0034E4C8, {obj, bit});
            h = load<u16>(obj + 40);
        }
        store<u16>(obj + 40, static_cast<u16>(h & ~0x8100u));
    }
    ret(ctx, load<u16>(kObjList106BC70 + 8));
}

// void Obj_SetSlot20(Object* o, RefObj* p, s32 i): o->+20[i] = p, taking a
// reference on p (u16 +4) and releasing the old one (vfunc +8); slot 0 also
// copies p's +112 (low half) into u16 +48, or 0.
DECOMP(004FEF48, Obj_SetSlot20) {
    const u32 o = arg<u32>(ctx, 0), p = arg<u32>(ctx, 1);
    const s32 i = static_cast<s32>(arg<u32>(ctx, 2));
    const u32 slot = o + 20 + static_cast<u32>(i) * 4;
    const u32 old = load<u32>(slot);
    ctx.gpr[3] = old;
    if (old != p) {
        store<u32>(slot, p);
        ctx.gpr[4] = slot;
        if (p != 0) {
            const u16 refs = static_cast<u16>(load<u16>(p + 4) + 1);
            store<u16>(p + 4, refs);
            ctx.gpr[4] = p + 4;
            ctx.gpr[5] = refs;
        }
        if (old != 0) vcall(ctx, old, 8, {old});
    }
    if (i != 0) return;
    const u32 v = p != 0 ? load<u32>(p + 112) : 0u;
    store<u16>(o + 48, static_cast<u16>(v));
    ret(ctx, v);
}

// s32 DeleteGame_Poll() / ContinueGame_Poll(SaveSlot* dst) / LoadSettings_Poll(Settings* dst):
// the save threads "DeleteGame", "ContinueGame" (hands back 4056 bytes) and
// "LoadSettings" (36 bytes) (name?)
DECOMP(000AF0E4, DeleteGame_Poll) { SaveTaskPoll(ctx, kDeleteGameEntry, kDeleteGameName, 0, 0, true); }
DECOMP(000AF1A8, ContinueGame_Poll) {
    SaveTaskPoll(ctx, 0x00F30FE8, 0x00AC5FE8, arg<u32>(ctx, 0), kSaveSlotSize, false);
}
DECOMP(000AF530, LoadSettings_Poll) { SaveTaskPoll(ctx, 0x00F30FC0, 0x00AC6098, arg<u32>(ctx, 0), 36, false); }

// void Player2_Reset(): the object of the handle at 0x00F64D44 (flags bit 0x1)
// gets sub_00362F1C(obj, 0, 0), +396 bit 0x40000000 cleared and vfunc
// +20(obj, 0) (name?). Quirk: without one the original goes on with obj = 0.
DECOMP(00367290, Player2_Reset) {
    constexpr u32 kHandle = 0x00F64D44;
    Stack frame(ctx, 16);
    const u32 h = load<u32>(kHandle);
    store<u32>(frame.addr(), h);
    u32 obj = 0;
    if (h != 0) {
        obj = load<u32>(static_cast<u32>(call(ctx, 0x0076611Cu, {kHandleMap, frame.addr()})));
        if (obj != 0 && (load<u32>(obj + 4) & 1u) == 0) obj = 0;
    }
    call(ctx, gfn::sub_00362F1C, {obj, 0, 0});
    const u32 f = load<u32>(obj + 396) & ~0x40000000u;
    store<u32>(obj + 396, f);
    const u32 opd = load<u32>(load<u32>(obj) + 20);
    call_ptr(ctx, opd, {obj, 0, opd, load<u32>(opd)});
}

// void PushStack_Remove(u32* v): removes the entry of the 4-entry stack at
// 0x010AFB84 whose first word equals *v (searching from the top), closes the
// gap and calls sub_00447B70() with r3 = 0x010B0000 (name?)
DECOMP(0044D614, PushStack_Remove) {
    const u32 v = arg<u32>(ctx, 0);
    if (v == 0) return;
    const u32 n = load<u32>(kPushCount);
    const s32 top = static_cast<s32>(n - 1);
    ctx.gpr[4] = static_cast<u64>(static_cast<s64>(top));
    ctx.gpr[5] = n;
    ctx.gpr[6] = static_cast<u64>(static_cast<s64>(top));
    if (top < 0) return;
    const u32 key = load<u32>(v);
    ctx.gpr[7] = key;
    s32 k = top;
    for (;; --k) {
        if (k < 0) {
            ctx.gpr[6] = static_cast<u64>(static_cast<s64>(k));
            ctx.gpr[8] = static_cast<u64>(static_cast<s64>(k) * 4);
            return;
        }
        const u32 e = load<u32>(load<u32>(kPushStack + static_cast<u32>(k) * 4));
        ctx.gpr[8] = static_cast<u64>(static_cast<s64>(k) * 4);
        ctx.gpr[9] = e;
        if (static_cast<s32>(e) == static_cast<s32>(key)) break;
    }
    const u32 next = static_cast<u32>(k + 1);
    ctx.gpr[6] = next;
    if (n > next) {
        u32 off = next * 4;
        s32 left = static_cast<s32>(n - next);
        u32 moved = 0;
        do {
            moved = load<u32>(kPushStack + off);
            store<u32>(kPushStack + off - 4, moved);
            ctx.gpr[8] = off - 4;
            off += 4;
            --left;
        } while (left > 0);
        ctx.gpr[5] = off;
        ctx.gpr[6] = 0;
        ctx.gpr[7] = moved;
    }
    store<u32>(kPushCount, static_cast<u32>(top));
    call(ctx, gfn::sub_00447B70, {0x010B0000u, static_cast<u64>(static_cast<s64>(top))});
}

namespace {
constexpr u32 kAnimPlay = 0x004A5C90;   // (Anim*, clip, 0, clip, f32 blend) (name?)
constexpr u32 kNotify4CF988 = 0x004CF988;

// Object state change shared by Obj_EnterState4 / Obj_EnterState1: the child
// at +24 gets vfunc +40(child, 1, 1); the state (+160 bits 1..) = `state`;
// the animator at +28 plays the clip at `clip_off` with the blend time at
// `blend`; the object at +20 gets sub_004CF988(obj, tag).
void ObjEnterState(PPUContext& ctx, u32 o, u32 state, u32 clip_off, u32 blend, u32 tag) {
    u32 w = load<u32>(o + 160);
    const u32 c = load<u32>(o + 24);
    if (c != 0) {
        const u32 opd = load<u32>(load<u32>(c) + 40);
        call_ptr(ctx, opd, {c, 1, 1, opd, load<u32>(opd)});
        w = load<u32>(o + 160);
    }
    store<u32>(o + 160, (w & 1u) | (state << 1));
    const u32 clip = load<u32>(o + clip_off);
    callf(ctx, kAnimPlay, {load<u32>(o + 28), clip, 0, clip, load<u32>(o + 28)},
          {static_cast<f64>(load<f32>(blend))});
    const u32 p = load<u32>(o + 20);
    if (p != 0) call(ctx, kNotify4CF988, {p, tag});
}
}  // namespace

// void Obj_EnterState4(Object* o): unless already in state 4 (name?)
DECOMP(0036CB58, Obj_EnterState4) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 w = load<u32>(o + 160);
    ctx.gpr[3] = w;
    ctx.gpr[4] = w >> 1;
    if ((w >> 1) == 4) return;
    ObjEnterState(ctx, o, 4, 180, 0x0036CB50, 0x048129EB);
}

// void Obj_EnterState1(Object* o): unless in state 1 or 3 (name?)
DECOMP(0036CA78, Obj_EnterState1) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 w = load<u32>(o + 160);
    ctx.gpr[3] = w;
    ctx.gpr[4] = w >> 1;
    if ((w >> 1) == 1 || (w >> 1) == 3) return;
    ObjEnterState(ctx, o, 1, 168, 0x0036CA74, 0x04296E4C);
}

// void Handle_Call363A3C(u32 handle, a, u8 b, ?, u8 c, d, e, f32 x): for the
// handle's object (flags bit 0x1), sub_00363A3C(obj, a, b, r6, c, d, e, x).
// r6 is whatever SortedMap_Find leaves, as in the original (name?)
DECOMP(0036735C, Handle_Call363A3C) {
    const u32 h = arg<u32>(ctx, 0), a = arg<u32>(ctx, 1);
    const u32 b = arg<u32>(ctx, 2) & 0xFF, c = arg<u32>(ctx, 4) & 0xFF;
    const u64 d = ctx.gpr[8], e = ctx.gpr[9];
    const f64 x = farg(ctx, 0);
    if (h == 0) return ret(ctx, 0u);
    Stack frame(ctx, 16);
    store<u32>(frame.addr(), h);
    const u32 obj = load<u32>(static_cast<u32>(call(ctx, 0x0076611Cu, {kHandleMap, frame.addr(), h})));
    if (obj == 0) return ret(ctx, 0u);
    const u32 live = load<u32>(obj + 4) & 1u;
    ctx.gpr[4] = live;
    if (live == 0) return ret(ctx, obj);  // r3 still holds the object
    ctx.gpr[7] = c;
    ctx.gpr[8] = d;
    ctx.gpr[9] = e;
    callf(ctx, 0x00363A3Cu, {obj, a, b}, {x});
}

// void Obj_ForEachChild31EC38(Object* o): sub_0031EC38(e) for every non-null
// element of every vector in the {key, vector*} table at +32 (u16 count +26) (name?)
DECOMP(003200B4, Obj_ForEachChild31EC38) {
    const u32 o = arg<u32>(ctx, 0);
    for (u32 i = 0; static_cast<s32>(i) < static_cast<s32>(load<u16>(o + 26)); ++i) {
        const u32 v = load<u32>(load<u32>(o + 32) + i * 8 + 4);
        if (v == 0) continue;
        for (u32 j = 0; j < load<u16>(v + 8); ++j) {
            const u32 e = load<u32>(load<u32>(v) + j * 4);
            if (e != 0) call(ctx, 0x0031EC38u, {e});
        }
    }
}
