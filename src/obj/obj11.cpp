// World objects, part 11: the instance / definition free-list returns,
// the ApeGeomDef / ApeEmitterDef destructors and the view-sphere visit of
// the geometry / emitter instances.
#include "obj11.h"

using namespace sly4;
using namespace sly4::obj8;
using namespace sly4::obj9;
using namespace sly4::obj11;

namespace {

// The original's 160-byte frame keeps the two map keys at sp+112 / sp+116;
// call() adds the 112-byte linkage area below this block.
constexpr u32 kFreeFrameKeys = 160 - 112;

// Drops a reference to a definition's +108 shared block; the last one
// frees its data (allocated 16 bytes in) and the block.
void ReleaseShared(PPUContext& ctx, u32 d) {
    const u32 sh = load<u32>(d + cls::kGeomShared);
    if (sh == 0) return;
    const u32 refs = static_cast<u32>(load<u16>(sh)) - 1;
    store<u16>(sh, static_cast<u16>(refs));
    if (static_cast<s16>(refs) > 0) return;
    const u32 data = load<u32>(sh + 4);
    if (data != 0) {
        call(ctx, cls::cfn::Mem_FreeFrom, {data - 16, 0});
        store<u32>(sh + 4, 0);
    }
    call(ctx, cls::cfn::Mem_FreeFrom, {sh, 0});
}

bool AllGe0(const Vec4& v) {  // vcmpgefp. against 0: every lane >= 0 (NaN fails)
    for (u32 i = 0; i < 4; ++i)
        if (!(v.f(i) >= 0.0f)) return false;
    return true;
}

// Plane distances of the sphere centre (x, y, z splats) for one plane set
// at `planes` (x, y, z coefficient rows, then the distances).
Vec4 PlaneDist(u32 planes, const Vec4& x, const Vec4& y, const Vec4& z) {
    Vec4 d = VMadd(VLoad(planes), x, VLoad(planes + 48));
    d = VMadd(VLoad(planes + 16), y, d);
    return VMadd(VLoad(planes + 32), z, d);
}

}  // namespace

void obj11::GeomBaseDestructBody(PPUContext& ctx, u32 d) {
    store<u32>(d, cls::kVtblGeomBase);
    ReleaseShared(ctx, d);
    call(ctx, obj7::o7fn::ApeRoot_Destruct, {d});
}

void obj11::FreeToList(PPUContext& ctx, u32 o, u32 hdr_ea, u32 vtbl, u32 name, u32 elem_size, u32 find, u32 remove,
                       u32 key_off) {
    if ((load<u32>(o + obj2::kObjWord8) & kInMap) != 0) {
        Stack frame(ctx, kFreeFrameKeys);
        const u32 key = frame.addr(), key2 = key + 4;  // sp+112 / sp+116
        store<u32>(key, load<u32>(o + key_off));
        const u32 slot = static_cast<u32>(call(ctx, find, {load<u32>(load<u32>(hdr_ea) + 16), key}));
        if (load<u32>(slot) == o) {
            store<u32>(key2, load<u32>(o + key_off));
            call(ctx, remove, {load<u32>(load<u32>(hdr_ea) + 16), key2});
        }
        store<u32>(o + obj2::kObjWord8, load<u32>(o + obj2::kObjWord8) & ~kInMap);
    }
    obj4::Vcall0(ctx, o, kVfnDestruct);
    const u32 alloc = obj6::ListAllocator(ctx, hdr_ea, vtbl, name, elem_size);
    obj6::ListFree(ctx, alloc, o);
}

// void EmitterDef_Free(ApeEmitterDef* d)
// Out of the emitter definition table (index +122, then 0xFFFF) and the
// name map (key +96), destructed, back to its free list.
DECOMP(0059DE50, EmitterDef_Free) {
    const u32 d = arg<u32>(ctx, 0);
    const s16 idx = static_cast<s16>(load<u16>(d + obj6::kEdIndex));
    call(ctx, o11fn::PtrTable_Remove, {load<u32>(obj6::kEmitterDefTable), static_cast<u64>(static_cast<s64>(idx))});
    store<u16>(d + obj6::kEdIndex, 0xFFFF);
    FreeToList(ctx, d, obj6::kEmitterDefList, obj6::kEmitterDefListVtbl, obj6::kStrEmitterDefList,
               obj6::kEmitterDefSize, o11fn::MapE_Find, o11fn::MapE_Remove, kEdNameHash);
}

// void GeomInstance_Free(ApeGeomInstance* o) / LightInstance_Free /
// EmitterInstance_Free: out of the class map (key: the name hash),
// destructed, back to the free list.
DECOMP(005A1CE0, GeomInstance_Free) {
    FreeToList(ctx, arg<u32>(ctx, 0), obj::kMapHolderB, kGeomInstListVtbl, kStrGeomInstList, kGeomInstanceSize,
               obj::ofn::MapB_Find, o11fn::MapB_Remove, obj::kObjNameHash);
}
DECOMP(005A3BEC, LightInstance_Free) {
    FreeToList(ctx, arg<u32>(ctx, 0), obj::kMapHolderC, obj5::kLightInstListVtbl, obj5::kStrLightInstList,
               obj5::kLightInstanceSize, obj::ofn::MapC_Find, o11fn::MapC_Remove, obj::kObjNameHash);
}
DECOMP(005A4F28, EmitterInstance_Free) {
    FreeToList(ctx, arg<u32>(ctx, 0), obj::kMapHolderD, obj5::kEmitterInstListVtbl, obj5::kStrEmitterInstList,
               obj5::kEmitterInstanceSize, obj::ofn::MapD_Find, o11fn::MapD_Remove, obj::kObjNameHash);
}

// void GeomDef_Destruct(ApeGeomDef* d)
// Releases the topologies (+132, u8 count at +112, re-read after every
// release) and the extra slot +115 unless it is the last one, the
// skeleton definition (+164; a second release when the first left one
// there), then as the geometry base: the +108 shared block and the root.
DECOMP(00585678, GeomDef_Destruct) {
    const u32 d = arg<u32>(ctx, 0);
    store<u32>(d, cls::kVtblGeomDef);
    u32 n = load<u8>(d + obj2::kClassTopoCount);
    for (u32 i = 0; i < n; ++i) {
        const u32 slot = d + obj2::kClassTopos + i * kGdSubStride;
        const u32 s = load<u32>(slot);
        if (s == 0) continue;
        obj4::Vcall0(ctx, s, kVfnDefRelease);
        store<u32>(slot, 0);
        n = load<u8>(d + obj2::kClassTopoCount);
    }
    const s32 extra = load<u8>(d + obj2::kClassTopoIndex);
    if (extra > 0 && extra != static_cast<s32>((n & 0xFF) - 1)) {
        const u32 slot = d + static_cast<u32>(extra) * kGdSubStride + obj2::kClassTopos;
        const u32 s = load<u32>(slot);
        if (s != 0) {
            obj4::Vcall0(ctx, s, kVfnDefRelease);
            store<u32>(slot, 0);
        }
    }
    u32 sk = load<u32>(d + obj2::kClassRes);
    if (sk != 0) {
        store<u32>(d + obj2::kClassRes, 0);
        call(ctx, cls::cfn::SkeletonDef_Release, {sk});
        sk = load<u32>(d + obj2::kClassRes);
        if (sk != 0) call(ctx, cls::cfn::SkeletonDef_Release, {sk});
    }
    GeomBaseDestructBody(ctx, d);
}

// void EmitterDef_Destruct(ApeEmitterDef* d)
// Releases the emitter data (+112; when that left one there, vfunc +12
// first) and the particle system (+116, vfunc +24, again when one is left
// there), then the data a second time if set, then as the geometry base.
DECOMP(0059D360, EmitterDef_Destruct) {
    const u32 d = arg<u32>(ctx, 0);
    store<u32>(d, kVtblEmitterDef);
    const u32 data = load<u32>(d + obj5::kEdData);
    if (data != 0) {
        store<u32>(d + obj5::kEdData, 0);
        call(ctx, obj5::o5fn::EmitterData_Release, {data, 0});
        if (load<u32>(d + obj5::kEdData) != 0) obj4::Vcall0(ctx, d, kVfnEdReleaseData);
    }
    u32 sys = load<u32>(d + obj5::kEdSystem);
    if (sys != 0) {
        store<u32>(d + obj5::kEdSystem, 0);
        obj4::Vcall0(ctx, sys, obj6::kVfnSystemRelease);
        sys = load<u32>(d + obj5::kEdSystem);
        if (sys != 0) obj4::Vcall0(ctx, sys, obj6::kVfnSystemRelease);
    }
    const u32 data2 = load<u32>(d + obj5::kEdData);
    if (data2 != 0) call(ctx, obj5::o5fn::EmitterData_Release, {data2});
    GeomBaseDestructBody(ctx, d);
}

// void WorldObj_VisitInView(Obj* o, u32 flags)
// For a visible object (+164 bit 0x8000) or with flags bit 2: when its
// bounding sphere (+240) is inside all eight planes of the current view
// (an object whose class's first topology is flagged, +322 bit 0x40,
// retries with the radius grown by Topology_Radius) and its position (the
// slot entry's, else the world matrix's) is nearer the camera than the
// +312 squared radius, and it has a class: the three per-object passes.
DECOMP(00583864, WorldObj_VisitInView) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 flags = arg<u32>(ctx, 1);
    // r3 on each early return as the original leaves it (the last test's
    // result, address or call result).
    if ((load<u32>(o + obj::kObjFlags) & obj4::kObjFlagVisible) == 0 && (flags & 2) == 0) {
        ctx.gpr[3] = 0;
        return;
    }
    const u32 view = static_cast<u32>(call(ctx, o11fn::View_GetCurrent, {}));
    const u32 s = o + kObjBoundSphere;
    const Vec4 x = VSplat(VLoadLeft(s), 0), y = VSplat(VLoadLeft(s + 4), 0), z = VSplat(VLoadLeft(s + 8), 0);
    const Vec4 r = VSplat(VLoadLeft(s + 12), 0);
    const f32 radius = Lf(s + 12);
    bool inside = AllGe0(VAdd(r, PlaneDist(view + kViewPlanesA, x, y, z))) &&
                  AllGe0(VAdd(r, PlaneDist(view + kViewPlanesB, x, y, z)));
    if (!inside) {
        if ((load<u8>(o + obj4::kObjBits322) & 0x40) == 0) {
            ctx.gpr[3] = 0;
            return;
        }
        const u32 topo = load<u32>(load<u32>(o + obj4::kObjClassPtr) + kClassTopo0);
        const f64 grow = callf(ctx, o11fn::Topology_Radius, {topo}, {});
        // (The original splats this through the stack at sp+112.)
        const Vec4 r2 = VFill(std::bit_cast<u32>(Fadds(radius, grow)));
        inside = AllGe0(VAdd(r2, PlaneDist(view + kViewPlanesA, x, y, z))) &&
                 AllGe0(VAdd(r2, PlaneDist(view + kViewPlanesB, x, y, z)));
        if (!inside) return;
    }
    const u32 idx = load<u16>(o + kObjSlotIndex);
    const u32 pos = idx == 0xFFFF ? o + obj6::kObjWorld + kSlotEntryPos
                                  : load<u32>(obj3::kSlotTable) + (idx << kSlotEntryShift) + kSlotEntryPos;
    const f32 dy = Fsubs(Lf(pos + 4), Lf(kCameraPos + 4));
    const f32 dx = Fsubs(Lf(pos), Lf(kCameraPos));
    const f32 dz = Fsubs(Lf(pos + 8), Lf(kCameraPos + 8));
    const f32 d2 = Fmadds(dz, dz, Fmadds(dx, dx, Fmuls(dy, dy)));
    if (d2 >= Lf(o + obj4::kObjRadiusSq)) {
        ctx.gpr[3] = pos;
        return;
    }
    const u32 cl = load<u32>(o + obj4::kObjClassPtr);
    if (cl == 0) {
        ctx.gpr[3] = 0;
        return;
    }
    call(ctx, o11fn::Obj_Visit583518, {cl});  // takes no arguments; r3 = the class here
    call(ctx, o11fn::Obj_Visit5832FC, {o});
    call(ctx, o11fn::Obj_Visit58303C, {o});
}

// void GeomBase_Destruct(ApeGeomBase* d) / LightDef_Destruct(ApeLightDef* d)
// The geometry-base destructor (vtable 0x00B246B0 +0), which ApeLightDef
// (0x00B24700 +0) uses unchanged: the +108 shared block, then the root.
DECOMP(00584A6C, GeomBase_Destruct) { GeomBaseDestructBody(ctx, arg<u32>(ctx, 0)); }
DECOMP(0058627C, LightDef_Destruct) { GeomBaseDestructBody(ctx, arg<u32>(ctx, 0)); }
// void CameraDef_Destruct(ApeCameraDef* d) / PointDef_Destruct(ApePointDef* d)
// Vtables 0x00B247B8 / 0x00B24808 +0: the geometry-base destructor too.
DECOMP(0059C90C, CameraDef_Destruct) { GeomBaseDestructBody(ctx, arg<u32>(ctx, 0)); }
DECOMP(0059E050, PointDef_Destruct) { GeomBaseDestructBody(ctx, arg<u32>(ctx, 0)); }
