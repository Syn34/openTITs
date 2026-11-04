// ApeEmitterInstance: the particle emitter objects (kind 10, vtable
// 0x00B248F8). The slots it shares with ApeGeomInstance are in
// obj2_vt.cpp / obj3.cpp / obj4.cpp.
#include "classes.h"

using namespace sly4;
using namespace sly4::cls;
using namespace sly4::obj4;

namespace {

namespace efn {
constexpr u32 EmitterInstance_New = 0x005A3FFC;  // (ApeEmitterDef*, bool copy_def) -> instance (name?)
constexpr u32 EmitterDef_Copy = 0x0058E3E4;      // (def, bool) -> a copy (name?)
constexpr u32 SlotEntry_Move = 0x0057A1D8;       // (u16 to, u16 from)
constexpr u32 SlotEntry_UpdatePeer = 0x0057A46C; // (u16 slot)
}  // namespace efn

constexpr u32 kVtblEmitterInstance = 0x00B248F8;
constexpr u32 kEmitterDestroyOpd = 0x00F3C938;
constexpr u32 kEmitterDefData = 112;   // def +112: the emitter data (+42 bit 0x80 world-attached, +56 f32 radius) (name?)
constexpr u32 kEmitterDefSystem = 116; // def +116: the particle system (+24 matrix*, +32 position) (name?)
constexpr u32 kEmitter340 = 340;       // u32, copied by Clone (name?)
constexpr u32 kViewState = 0x010DDA74; // +8 u32 frame, +16 / +20 view indices (name?)

inline bool WorldAttached(u32 def) { return (load<u8>(load<u32>(def + kEmitterDefData) + 42) & 0x80) != 0; }

}  // namespace

// void EmitterInstance_Destruct(EmitterInstance* o)  (slot +0)
// As WorldObj_Destruct: a kind-8 / kind-10 object with a slot leaves its
// slot list (the list's last / first entry moving into its slot), then the
// base destructor.
DECOMP(005A3DC0, EmitterInstance_Destruct) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 slot = load<u16>(o + obj2::kObjWord284);
    store<u32>(o, kVtblEmitterInstance);
    if (slot != 0xFFFF) {
        const u32 kind = Kind(o);
        if (kind == 10 || kind == 8) {
            const u32 field = obj3::kSlotTable + (kind == 10 ? 6 : 4);
            const u32 other = load<u16>(field) + (kind == 10 ? 1u : 0xFFFFFFFFu);
            call(ctx, efn::SlotEntry_Move, {slot, other & 0xFFFF});
            if (((slot ^ other) & 0xFFFF) != 0) call(ctx, efn::SlotEntry_UpdatePeer, {slot});
            store<u16>(field, static_cast<u16>(load<u16>(field) + (kind == 10 ? 1 : -1)));
        }
    }
    call(ctx, ifn::WorldObj_DestructBase, {o});
}

// void EmitterInstance_DeletingDestruct(EmitterInstance* o)  (slot +4)
DECOMP(005A3ED0, EmitterInstance_DeletingDestruct) {
    const u32 o = arg<u32>(ctx, 0);
    if (o == 0) return;
    call(ctx, 0x005A3DC0, {o});
    call(ctx, ifn::Mem_FreeFrom, {o, 0});
}

// void EmitterInstance_Release(EmitterInstance* o)  (slot +8)
// As WorldObj_Release, with the emitter's deferred destructor (0x00F3C938).
DECOMP(005A3F18, EmitterInstance_Release) {
    const u32 o = arg<u32>(ctx, 0);
    const u16 refs = static_cast<u16>(load<u16>(o + kObjRefs) - 1);
    store<u16>(o + kObjRefs, refs);
    if (static_cast<s16>(refs) > 0) return;
    call(ctx, ifn::WorldObj_Unlink, {o});
    RemoveChildren(ctx, o);
    call(ctx, ifn::WorldObj_ReleaseRes, {o});
    call(ctx, ifn::DeferredDestroy, {o, kEmitterDestroyOpd});
}

// void EmitterInstance_Changed(EmitterInstance* o)  (slot +36)
// The same code as WorldObj_Changed.
DECOMP(005A46FC, EmitterInstance_Changed) { ChangedBody(ctx, arg<u32>(ctx, 0)); }

// void EmitterInstance_SetClass(EmitterInstance* o, ApeEmitterDef* def)  (slot +44)
// A world-attached old definition's particle system forgets the object's
// matrix; the base swap; a world-attached new one gets the world matrix
// (+80) and position (+128); the radius^2 from the emitter data's +56;
// vfunc +36. As in the original there is no null check on the new class.
DECOMP(005A44A4, EmitterInstance_SetClass) {
    const u32 o = arg<u32>(ctx, 0), def = arg<u32>(ctx, 1);
    const u32 old = load<u32>(o + kObjClassPtr);
    if (old != 0 && WorldAttached(old)) store<u32>(load<u32>(old + kEmitterDefSystem) + 24, 0);
    call(ctx, ifn::ApeObj_SetClass, {o, def});
    const u32 cls = load<u32>(o + kObjClassPtr);
    const u32 data = load<u32>(cls + kEmitterDefData);
    const f64 r = static_cast<f64>(load<f32>(data + 56));
    if ((load<u8>(data + 42) & 0x80) != 0) {
        const u32 sys = load<u32>(cls + kEmitterDefSystem);
        const u32 m = o + 80;
        store<u32>(sys + 24, m);
        if (m != 0)
            for (u32 k = 0; k < 12; k += 4) store<u32>(sys + 32 + k, load<u32>(o + 128 + k));
    }
    store<f32>(o + kObjRadiusSq, static_cast<f32>(r * r));
    Vcall0(ctx, o, kVfnChanged);
}

// EmitterInstance* EmitterInstance_Clone(const EmitterInstance* o, u32 flags)  (slot +56)
// With kCloneOwnClass, or flag 4 and a world-attached definition, the clone
// gets its own copy of the definition (0x0058E3E4, flag 0x20 passed on);
// otherwise it shares it. Then +340, the children (kCloneChildren) and the
// world (unless o has a parent or kCloneNoWorld).
DECOMP(005A42D0, EmitterInstance_Clone) {
    const u32 o = arg<u32>(ctx, 0), flags = arg<u32>(ctx, 1);
    const u32 def = load<u32>(o + kObjClassPtr);
    const bool own = ((flags & 4) != 0 && WorldAttached(def)) || (flags & kCloneOwnClass) != 0;
    u32 n;
    if (own) {
        const u32 copy = static_cast<u32>(call(ctx, efn::EmitterDef_Copy, {def, (flags & 0x20) != 0 ? 1u : 0u}));
        n = static_cast<u32>(call(ctx, efn::EmitterInstance_New, {copy, 0}));
        Release(ctx, copy);
        call(ctx, ifn::WorldObj_CopyFrom, {n, o, 1});
    } else {
        n = static_cast<u32>(call(ctx, efn::EmitterInstance_New, {def, 0}));
        call(ctx, ifn::WorldObj_CopyFrom, {n, o, 0});
    }
    store<u32>(n + kEmitter340, load<u32>(o + kEmitter340));
    if ((flags & kCloneChildren) != 0) CloneChildren(ctx, o, n, flags);
    if (load<u32>(o + kObjParentLink) == 0 && (flags & kCloneNoWorld) == 0) call(ctx, ifn::WorldObj_AddToWorld, {n});
    ret(ctx, n);
}

// void EmitterInstance_Dump(EmitterInstance* o, s32 depth, u32 flags)  (slot +60)
// As the light's for depth > 0. Otherwise (unless depth is -1 and the
// object's per-view value is below the frame at 0x010DDA7C): the particle
// system's +28, then the object's and its class's names.
DECOMP(005A458C, EmitterInstance_Dump) {
    const u32 o = arg<u32>(ctx, 0);
    const u64 depth = ctx.gpr[4], flags = ctx.gpr[5];
    if (static_cast<s32>(depth) > 0) return DumpInstance(ctx, o, depth, flags);
    if (static_cast<s32>(depth) == -1) {
        const s32 view = static_cast<s32>(load<u32>(kViewState + 20) << 2);
        if (load<u32>(o + obj4::kObjPerView + static_cast<u32>(view)) < load<u32>(kViewState + 8)) return;
    }
    const u32 cls = load<u32>(o + kObjClassPtr);
    if (cls != 0) Vcall0(ctx, load<u32>(cls + kEmitterDefSystem), 28);
    Vcall0(ctx, o, kVfnGetName);
    Vcall0(ctx, load<u32>(o + kObjClassPtr), kVfnGetName);
}
