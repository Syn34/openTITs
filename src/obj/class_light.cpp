// ApeLightInstance: the light objects (kind 9, vtable 0x00B248A8). The
// slots it shares with ApeGeomInstance are in obj2_vt.cpp / obj3.cpp /
// obj4.cpp.
#include "classes.h"

using namespace sly4;
using namespace sly4::cls;
using namespace sly4::obj4;

// void LightInstance_Destruct(LightInstance* o)  (slot +0)
// Takes a light with a slot out of its slot list (type 2: the end of the
// first list, types 0 / 1: the start of the second, moving that entry into
// its slot), refreshes the config's light mode copy (+96 to +100 for class
// modes 0 / 1, else to +104), then the base destructor.
DECOMP(005A21B4, LightInstance_Destruct) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 slot = load<u16>(o + obj2::kObjWord284);
    store<u32>(o, kVtblLightInstance);
    if (slot != 0xFFFF) {
        const s32 type = load<s32>(o + kLightType);
        if (type == 2) {
            const u32 end = (load<u16>(kLightSlotLists + 4) - 1u) & 0xFFFF;
            call(ctx, lfn::LightSlot_Move, {slot, end});
            store<u16>(kLightSlotLists + 4, static_cast<u16>(load<u16>(kLightSlotLists + 4) - 1));
        } else if (type == 0 || type == 1) {
            const u32 start = (load<u16>(kLightSlotLists + 6) + 1u) & 0xFFFF;
            call(ctx, lfn::LightSlot_Move, {slot, start});
            store<u16>(kLightSlotLists + 6, static_cast<u16>(load<u16>(kLightSlotLists + 6) + 1));
        }
    }
    const s32 mode = load<s32>(load<u32>(o + kObjClassPtr) + 128);
    const u32 v = load<u32>(kConfigLights + 96);
    store<u32>(kConfigLights + ((mode > 1 || mode <= -1) ? 104 : 100), v);
    call(ctx, ifn::WorldObj_DestructBase, {o});
}

// void LightInstance_DeletingDestruct(LightInstance* o)  (slot +4)
DECOMP(005A22AC, LightInstance_DeletingDestruct) {
    const u32 o = arg<u32>(ctx, 0);
    if (o == 0) return;
    call(ctx, 0x005A21B4, {o});
    call(ctx, ifn::Mem_FreeFrom, {o, 0});
}

// void LightInstance_Release(LightInstance* o)  (slot +8)
// Drops a reference; on the last one: removes its light record (+382),
// unlinks it, removes each child, releases its resources and gives it to
// the deferred destructor (0x00F3C8D8).
DECOMP(005A22FC, LightInstance_Release) {
    const u32 o = arg<u32>(ctx, 0);
    const u16 refs = static_cast<u16>(load<u16>(o + kObjRefs) - 1);
    store<u16>(o + kObjRefs, refs);
    if (static_cast<s16>(refs) > 0) return;
    const u32 idx = load<u16>(o + kLightRec);
    if (idx != 0xFFFF) RemoveRecord(ctx, load<u32>(load<u32>(kLightRecs)), idx, kLightRecSize, 4, 68, kLightRec);
    call(ctx, ifn::WorldObj_Unlink, {o});
    RemoveChildren(ctx, o);
    call(ctx, ifn::WorldObj_ReleaseRes, {o});
    call(ctx, ifn::DeferredDestroy, {o, kLightDestroyOpd});
}

// void LightInstance_Changed(LightInstance* o)  (slot +36)
// Updates the light (0x005A31C8); a type-2 light with a record copies its
// +240 vec4 to the record's +16; then the peer copy every Changed does.
DECOMP(005A3448, LightInstance_Changed) {
    const u32 o = arg<u32>(ctx, 0);
    call(ctx, lfn::LightInstance_Update, {o});
    const u32 idx = load<u16>(o + kLightRec);
    const s32 type = load<s32>(o + kLightType);
    const u32 recs = load<u32>(load<u32>(kLightRecs));
    if (idx != 0xFFFF && type == 2) {
        const u32 rec = load<u32>(recs) + idx * kLightRecSize;
        CopyVec4(rec + 16, o + kObjAnchor);
    }
    CopyAnchorToPeer(ctx, o);
}

// void LightInstance_SetClass(LightInstance* o, ApeLightDef* def)  (slot +44)
// The base swap, then the light parameters from the definition (+112
// colour, +128 type, +132, +136 range (squared), +140, +144), or the
// defaults (type 3, white, 1, 1.0, 127, {0, 1.0}) without one.
DECOMP(005A2FDC, LightInstance_SetClass) {
    const u32 o = arg<u32>(ctx, 0), def = arg<u32>(ctx, 1);
    call(ctx, ifn::ApeObj_SetClass, {o, def});
    if (def != 0) {
        store<u32>(o + kLightType, load<u32>(def + 128));
        for (u32 k = 0; k < 12; k += 4) store<u32>(o + kLightColor + k, load<u32>(def + 112 + k));
        store<u32>(o + kLight372, load<u32>(def + 132));
        const f64 r = static_cast<f64>(load<f32>(def + 136));
        store<f32>(o + kLightRangeSq, static_cast<f32>(r * r));
        store<u8>(o + kLight380, load<u8>(def + 140));
        for (u32 k = 0; k < 12; k += 4) store<u32>(o + kLight384 + k, load<u32>(def + 144 + k));
        return;
    }
    store<u32>(o + kLightType, 3);
    for (u32 k = 0; k < 12; k += 4) store<u32>(o + kLightColor + k, load<u32>(obj::kOneVec3 + k));
    store<u32>(o + kLight372, 1);
    store<u32>(o + kLightRangeSq, kOneF);
    store<u8>(o + kLight380, 127);
    store<u32>(o + kLight384, 0);
    store<u32>(o + kLight384 + 4, kOneF);
}

// LightInstance* LightInstance_Clone(const LightInstance* o, u32 flags)  (slot +56)
// A new light of o's definition with o's state and light parameters;
// children cloned with kCloneChildren; added to the world unless o has a
// parent or kCloneNoWorld; registered (0x005A2528) unless flag 0x10.
DECOMP(005A2E50, LightInstance_Clone) {
    const u32 o = arg<u32>(ctx, 0), flags = arg<u32>(ctx, 1);
    const u32 n = static_cast<u32>(call(ctx, lfn::LightInstance_New, {load<u32>(o + kObjClassPtr)}));
    call(ctx, ifn::WorldObj_CopyFrom, {n, o, 0});
    store<u32>(n + kLightType, load<u32>(o + kLightType));
    for (u32 k = 0; k < 12; k += 4) store<u32>(n + kLightColor + k, load<u32>(o + kLightColor + k));
    store<u32>(n + kLight372, load<u32>(o + kLight372));
    store<u32>(n + kLightRangeSq, load<u32>(o + kLightRangeSq));
    store<u8>(n + kLight380, load<u8>(o + kLight380));
    for (u32 k = 0; k < 12; k += 4) store<u32>(n + kLight384 + k, load<u32>(o + kLight384 + k));
    if ((flags & kCloneChildren) != 0) CloneChildren(ctx, o, n, flags);
    if (load<u32>(o + kObjParentLink) == 0 && (flags & kCloneNoWorld) == 0) call(ctx, ifn::WorldObj_AddToWorld, {n});
    if ((flags & 0x10) == 0) call(ctx, lfn::LightInstance_Register, {n});
    ret(ctx, n);
}

// void LightInstance_Dump(LightInstance* o, s32 depth, u32 flags)  (slot +60)
DECOMP(005A30CC, LightInstance_Dump) { DumpInstance(ctx, arg<u32>(ctx, 0), ctx.gpr[4], ctx.gpr[5]); }

// void Instance_SetBoundRadius(Obj* o, f32 radius)  (slot +64 of the light / emitter / camera instances)
// A tail call of WorldObj_SetBoundRadius; f1 passes through.
DECOMP(007A9A44, Instance_SetBoundRadius) { call(ctx, ifn::WorldObj_SetBoundRadius, {arg<u32>(ctx, 0)}); }
