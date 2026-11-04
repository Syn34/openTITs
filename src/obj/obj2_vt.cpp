// The small slots of the instanced world object's vtable (0x00B24858,
// kind 8); several are shared with the other object vtables after it.
#include "obj2.h"

using namespace sly4;
using namespace sly4::obj;
using namespace sly4::obj2;

namespace {

namespace vfn {
constexpr u32 WorldObj_DestructBase = 0x00580978;  // (obj) (name?)
constexpr u32 WorldObj_Destruct = 0x005A0158;
constexpr u32 SlotEntry_Move = 0x0057A1D8;         // (u16 to, u16 from)
constexpr u32 SlotEntry_UpdatePeer = 0x0057A46C;   // (u16 slot)
constexpr u32 WorldObj_Unlink = 0x0057FF98;        // (obj) (name?)
constexpr u32 WorldObj_ReleaseRes = 0x00558F34;    // (obj) (name?)
constexpr u32 DeferredDestroy = 0x005082F4;        // (obj, destroy opd)
constexpr u32 WorldObj_ApplyClass = 0x0058049C;    // (obj) (name?)
constexpr u32 WorldObj_ShowChildren = 0x00582C6C;  // (obj, 0) (name?)
}  // namespace vfn

constexpr u32 kViewState = 0x010DDA74;     // +16: the current view index (name?)
constexpr u32 kObjPerView = 304;           // u32[]: one value per view (name?)
constexpr u32 kObjSlot = 284;              // u16: slot in a kind-8 / kind-10 list, 0xFFFF = none
constexpr u32 kSlotLists = 0x01193004;     // +4 u16 kind-8 count, +6 u16 kind-10 start (name?)
constexpr u32 kObjDestroyOpd = 0x00F3C888;  // OPD of the object destructor
constexpr u32 kInstanceVtbl = 0x00B24858;
constexpr u32 kObjClass = 208;             // WorldClass*
constexpr u32 kObjRadiusSq = 312;          // f32 (name?)
constexpr u32 kObjByte168 = 168;           // u8 (name?)
constexpr u32 kObjByte320 = 320;           // u8: the top byte of the group word
constexpr u32 kVfnRemoveChild = 24, kVfnSetByte320 = 48, kVfnSetByte168 = 52;

inline u32 Kind(u32 o) { return (load<u32>(o + kObjWord4) >> 10) & 0x3F; }

}  // namespace

// const char* WorldObj_GetName(Obj* o)  (slot +12)
DECOMP(007A9924, WorldObj_GetName) { ret(ctx, arg<u32>(ctx, 0) + kObjName); }

// u32 WorldObj_Get300(Obj* o)  (slot +28) (name?)
DECOMP(007A9A1C, WorldObj_Get300) { ret(ctx, load<u32>(arg<u32>(ctx, 0) + 300)); }

// void WorldObj_SetPerView(Obj* o, u32 v)  (slot +32) (name?)
// Stores v in the object's +304 array at the current view index. Leaves
// r3 = the address and r5 = index * 4, as the original does.
DECOMP(007A99F8, WorldObj_SetPerView) {
    const u32 o = arg<u32>(ctx, 0), v = arg<u32>(ctx, 1);
    const u32 off = load<u32>(kViewState + 16) << 2;
    const u32 at = o + off + kObjPerView;
    store<u32>(at, v);
    ctx.gpr[5] = off;
    ret(ctx, at);
}

// void WorldObj_Destruct(Obj* o)  (slot +0)
// Takes a kind-8 / kind-10 object out of its slot list (moving the list's
// last / first entry into its slot), then the base destructor.
DECOMP(005A0158, WorldObj_Destruct) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 slot = load<u16>(o + kObjSlot);
    store<u32>(o, kInstanceVtbl);
    if (slot != 0xFFFF) {
        const u32 kind = Kind(o);
        if (kind == 10 || kind == 8) {
            const u32 field = kSlotLists + (kind == 10 ? 6 : 4);
            const u32 other = load<u16>(field) + (kind == 10 ? 1u : 0xFFFFFFFFu);
            call(ctx, vfn::SlotEntry_Move, {slot, other & 0xFFFF});
            if (((slot ^ other) & 0xFFFF) != 0) call(ctx, vfn::SlotEntry_UpdatePeer, {slot});
            store<u16>(field, static_cast<u16>(load<u16>(field) + (kind == 10 ? 1 : -1)));
        }
    }
    call(ctx, vfn::WorldObj_DestructBase, {o});
}

// void WorldObj_DeletingDestruct(Obj* o)  (slot +4)
DECOMP(005A0268, WorldObj_DeletingDestruct) {
    const u32 o = arg<u32>(ctx, 0);
    if (o == 0) return;
    call(ctx, vfn::WorldObj_Destruct, {o});
    call(ctx, o2fn::Mem_FreeFrom, {o, 0});
}

// void WorldObj_Release(Obj* o)  (slot +8)
// Drops a reference (+4 high half); on the last one: unlinks the object,
// removes each child (slot +24), releases its resources and gives it back
// to the free list at 0x00F3C888.
DECOMP(005A02B0, WorldObj_Release) {
    const u32 o = arg<u32>(ctx, 0);
    const u16 refs = static_cast<u16>(load<u16>(o + kObjWord4) - 1);
    store<u16>(o + kObjWord4, refs);
    if (static_cast<s16>(refs) > 0) return;
    call(ctx, vfn::WorldObj_Unlink, {o});
    for (u32 c = load<u32>(o + kObjFirstChild); c != 0;) {
        const u32 next = load<u32>(c + kObjNextSibling);
        const u32 opd = load<u32>(load<u32>(o) + kVfnRemoveChild);
        call_ptr(ctx, opd, {o, c, opd, load<u32>(opd)});  // r5 / r6 = OPD and entry
        c = next;
    }
    call(ctx, vfn::WorldObj_ReleaseRes, {o});
    call(ctx, vfn::DeferredDestroy, {o, kObjDestroyOpd});
}

// void WorldObj_ShowAndNotify(Obj* o, bool show_children)  (slot +16) (name?)
// Calls vfunc +36, then 0x00582C6C(o, 0) when the flag is set.
DECOMP(00582ED0, WorldObj_ShowAndNotify) {
    const u32 o = arg<u32>(ctx, 0);
    const u8 flag = static_cast<u8>(arg<u32>(ctx, 1));
    {
        const u32 opd = load<u32>(load<u32>(o) + kObjVfnChanged);
        call_ptr(ctx, opd, {o, opd, load<u32>(opd)});  // r4 / r5 = OPD and entry
    }
    if (flag != 0) call(ctx, vfn::WorldObj_ShowChildren, {o, 0});
}

// void WorldObj_SetByte320(Obj* o, u8 v, bool recurse)  (slot +48) (name?)
// Stores v in the top byte of +320 and optionally in every descendant's.
DECOMP(00583DC8, WorldObj_SetByte320) {
    const u32 o = arg<u32>(ctx, 0), v = arg<u32>(ctx, 1);
    const u32 recurse = arg<u32>(ctx, 2) & 0xFF;
    store<u8>(o + kObjByte320, static_cast<u8>(v));
    if (recurse == 0) return;
    for (u32 c = load<u32>(o + kObjFirstChild); c != 0; c = load<u32>(c + kObjNextSibling)) {
        const u32 opd = load<u32>(load<u32>(c) + kVfnSetByte320);
        call_ptr(ctx, opd, {c, v, recurse, opd, load<u32>(opd)});  // r6 / r7 = OPD and entry
    }
}

// void WorldObj_SetByte168(Obj* o, u8 v, bool recurse)  (slot +52) (name?)
// Stores v in +168, calling vfunc +36 when it changes, and optionally in
// every descendant's. r3 is left as the original leaves it.
DECOMP(00583EF0, WorldObj_SetByte168) {
    const u32 o = arg<u32>(ctx, 0), v = arg<u32>(ctx, 1) & 0xFF, recurse = arg<u32>(ctx, 2) & 0xFF;
    const u32 cur = load<u8>(o + kObjByte168);
    ret(ctx, cur);
    if (cur != v) {
        store<u8>(o + kObjByte168, static_cast<u8>(v));
        const u32 opd = load<u32>(load<u32>(o) + kObjVfnChanged);
        call_ptr(ctx, opd, {o, opd, load<u32>(opd)});  // r4 / r5 = OPD and entry
    }
    if (recurse == 0) return;
    for (u32 c = load<u32>(o + kObjFirstChild); c != 0; c = load<u32>(c + kObjNextSibling)) {
        const u32 opd = load<u32>(load<u32>(c) + kVfnSetByte168);
        call_ptr(ctx, opd, {c, v, recurse, opd, load<u32>(opd)});  // r6 / r7 = OPD and entry
    }
}

// void WorldObj_ClassChanged(Obj* o)  (slot +64) (name?)
// Applies the class (0x0058049C), then +312 = (class +60 + the last
// topology's f32)^2, then vfunc +36.
DECOMP(005A14A0, WorldObj_ClassChanged) {
    const u32 o = arg<u32>(ctx, 0);
    call(ctx, vfn::WorldObj_ApplyClass, {o});
    const u32 cls = load<u32>(o + kObjClass);
    const f64 a = static_cast<f64>(load<f32>(cls + 60));
    const u32 last = cls + (static_cast<u32>(load<u8>(cls + kClassTopoCount)) << 3) + 124;
    const f64 r = static_cast<f64>(static_cast<f32>(static_cast<f64>(load<f32>(last + 4)) + a));
    store<f32>(o + kObjRadiusSq, static_cast<f32>(r * r));
    const u32 opd = load<u32>(load<u32>(o) + kObjVfnChanged);
    call_ptr(ctx, opd, {o, opd, load<u32>(opd)});  // r4 / r5 = OPD and entry
}
