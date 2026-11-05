// World objects, part 18: the next still-recompiled functions of
// 0x00550000-0x005AA000 (bank bounds, the nearest spline instance, ...)
// and the template instances of 0x00730000-0x007A0000.
#pragma once

#include <bit>
#include <cmath>

#include "obj17.h"

namespace sly4 {
namespace obj18 {

using namespace opentit;

namespace o18fn {
constexpr u32 Box_Transform = 0x005570F0;            // (f32 out[8], const f32 in[8], const Matrix*)
constexpr u32 Obj_GetField4 = 0x00556BF0;            // (p) -> p->+4: a free list's first used node
constexpr u32 Listener_LocalDirection = 0x005532E4;  // (f32 out[3], const f32 pos[3], const f32 m[4][4])
constexpr u32 Spline_Nearest = 0x00560A40;           // (SplineData*, const f32 p[3], u32* seg, f32* dist) -> f1 t
constexpr u32 LightGrid_Rebuild = 0x00583518;        // ()
constexpr u32 WorldObj_UpdateLight = 0x005832FC;     // (obj)
constexpr u32 Obj_CollectLodGroup = 0x00590FEC;      // (obj, u8 recurse)
}  // namespace o18fn

// A free list's nodes: a 16-byte header (+0 the next node) and the item.
constexpr u32 kNodeHeader = 16;

// The instance free lists (ApeTListAllocator<T>, header +0 the allocator,
// +4 the capacity): their headers, vtables, names and item sizes.
struct InstList {
    u32 hdr, vtbl, name, size;
};
constexpr InstList kGeomInsts = {0x00F8F144, 0x00B0F008, 0x00AF4B7E, 352};     // ApeGeomInstance
constexpr InstList kLightInsts = {0x00F8F148, 0x00B13920, 0x00AF4BCF, 400};    // ApeLightInstance
constexpr InstList kEmitterInsts = {0x00F8F14C, 0x00B10EF0, 0x00AF4C23, 352};  // ApeEmitterInstance

// Calls fn(item) for every item of the list (the allocator made on first
// use); the next node is read after each call, as the original loops do.
template <typename F>
void ForEachInstance(PPUContext& ctx, const InstList& il, F&& fn) {
    const u32 alloc = obj6::ListAllocator(ctx, il.hdr, il.vtbl, il.name, il.size);
    const u32 first = static_cast<u32>(call(ctx, o18fn::Obj_GetField4, {alloc}));
    for (u32 it = first != 0 ? first + kNodeHeader : 0; it != 0;) {
        fn(it);
        const u32 next = load<u32>(it - kNodeHeader);
        it = next != 0 ? next + kNodeHeader : 0;
    }
}

}  // namespace obj18
}  // namespace sly4
