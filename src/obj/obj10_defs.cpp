// World objects, part 10: the small slots of the definition classes'
// vtables (ApeGeomDef 0x00B246D8, ApeLightDef 0x00B24700, ApeCameraDef
// 0x00B247B8, ApeEmitterDef 0x00B247E0, ApePointDef 0x00B24808,
// ApeSplineDef 0x00B24830) and their deferred-destroy callbacks: slot +8
// Release, +20 a query (name?) and +24 (def, s32 depth) (name?), which
// most definitions leave empty.
#include "obj10.h"

using namespace sly4;

namespace {

constexpr u32 kDeferredDestroy = 0x005082F4;  // (obj, destroy opd)
constexpr u32 kDefRefs = 4;                   // u16

// The destroy OPDs the Release slots defer to.
constexpr u32 kGeomDefDestroyOpd = 0x00F3C6F8;     // 0x0059BD24 (name?)
constexpr u32 kLightDefDestroyOpd = 0x00F3C710;    // 0x0059C5FC (name?)
constexpr u32 kCameraDefDestroyOpd = 0x00F3C750;   // 0x0059D1A8 (name?)
constexpr u32 kEmitterDefDestroyOpd = 0x00F3C7A0;  // EmitterDef_DeferredFree
constexpr u32 kPointDefDestroyOpd = 0x00F3C7D8;    // 0x0059E7FC (name?)
constexpr u32 kSplineDefDestroyOpd = 0x00F3C818;   // (name?)

constexpr u32 kEmitterDef_Free = 0x0059DE50;       // (def): back to its free list (name?)
constexpr u32 kEdIndex = 122;                      // u16: the definition's index in the emitter table

// Drops a reference; at zero hands the definition to the deferred
// destructor. r5 / r6 = the new count (zero- / sign-extended), as the
// original leaves them.
void DefRelease(PPUContext& ctx, u32 d, u32 destroy_opd) {
    const u64 refs = static_cast<u64>(load<u16>(d + kDefRefs)) - 1;
    store<u16>(d + kDefRefs, static_cast<u16>(refs));
    const s16 n = static_cast<s16>(refs);
    if (n > 0) {
        ctx.gpr[5] = refs;
        ctx.gpr[6] = static_cast<u64>(static_cast<s64>(n));
        return;
    }
    call(ctx, kDeferredDestroy, {d, destroy_opd});
}

}  // namespace

// void GeomDef_Release(ApeGeomDef* d)
DECOMP(00585890, GeomDef_Release) { DefRelease(ctx, arg<u32>(ctx, 0), kGeomDefDestroyOpd); }
// void LightDef_Release(ApeLightDef* d)
DECOMP(005863F8, LightDef_Release) { DefRelease(ctx, arg<u32>(ctx, 0), kLightDefDestroyOpd); }
// void CameraDef_Release(ApeCameraDef* d)
DECOMP(0059CA84, CameraDef_Release) { DefRelease(ctx, arg<u32>(ctx, 0), kCameraDefDestroyOpd); }
// void EmitterDef_Release(ApeEmitterDef* d)
DECOMP(0059D52C, EmitterDef_Release) { DefRelease(ctx, arg<u32>(ctx, 0), kEmitterDefDestroyOpd); }
// void PointDef_Release(ApePointDef* d)
DECOMP(0059E1C0, PointDef_Release) { DefRelease(ctx, arg<u32>(ctx, 0), kPointDefDestroyOpd); }
// void SplineDef_Release(ApeSplineDef* d)
DECOMP(0059F204, SplineDef_Release) { DefRelease(ctx, arg<u32>(ctx, 0), kSplineDefDestroyOpd); }

// bool LightDef_Vfn20(ApeLightDef*, ...) / CameraDef_Vfn20: always 0 (slot +20).
DECOMP(005863EC, LightDef_Vfn20) { ret(ctx, 0u); }
DECOMP(0059CA7C, CameraDef_Vfn20) { ret(ctx, 0u); }

// void XxxDef_Vfn24(def, s32 depth): empty for these definitions (slot +24;
// ApeGeomDef passes depth - 1 on to its topologies).
DECOMP(0058643C, LightDef_Vfn24) {}
DECOMP(0059CAC8, CameraDef_Vfn24) {}
DECOMP(0059D6B8, EmitterDef_Vfn24) {}
DECOMP(0059E204, PointDef_Vfn24) {}
DECOMP(0059F5CC, SplineDef_Vfn24) {}

// void EmitterDef_SetIndex(ApeEmitterDef* d, u16 index)
// The emitter table's "moved" callback: the definition's index (+122).
DECOMP(0059D524, EmitterDef_SetIndex) { store<u16>(arg<u32>(ctx, 0) + kEdIndex, static_cast<u16>(arg<u32>(ctx, 1))); }

// void EmitterDef_DeferredFree(ApeEmitterDef* d)
// The deferred destructor EmitterDef_Release hands the definition to.
DECOMP(0059E028, EmitterDef_DeferredFree) { call(ctx, kEmitterDef_Free, {arg<u32>(ctx, 0)}); }
