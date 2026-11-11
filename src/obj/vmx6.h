// Game code, VMX (AltiVec) function sweep part 6: helpers shared by
// vmx6_*.cpp (game functions the size sweeps skipped because they use VMX).
// Builds on vmx5.h (v5fn, kUpAxis5, Frsp) and, through it, vmx4.h ..
// vmx1.h and the big5..big17 lane helpers.
#pragma once

#include <bit>
#include <cmath>

#include "vmx5.h"

namespace sly4 {
namespace vmx6 {

using namespace opentit;
using namespace opentit::decomp;
using namespace sly4::vmx5;
using big17::HkPropIndex;
using big17::HkPropValue;

namespace v6fn {
constexpr u32 Registry_Add_000D64F4 = 0x000D64F4;  // void (T* o) (reads r3 only)
constexpr u32 Obj_UpdateBase = 0x000D83C4;  // bool (Obj* o; f1 dt)
constexpr u32 HkWorld_CastRay92E2E8 = 0x0092E2E8;  // (hkpWorld*, const hkpWorldRayCastInput*, hkpWorldRayCastOutput*)
constexpr u32 Effect_FindComponent = 0x004BB4B4;  // (Effect*, u32 type hash) -> component or 0
constexpr u32 Obj_FindById = 0x000D6A28;  // (u32 id) -> object or 0 (reads r3 only)
constexpr u32 Vec3List_Push_00244810 = 0x00244810;  // void (T* o, const Vec3* v): 16-byte record
constexpr u32 Obj_SendEvent_000E1C28 = 0x000E1C28;  // (const Event* e) (reads r3 only) (name?)
}  // namespace v6fn

constexpr u32 kEffectTargetKind6 = 0x02235C33;  // an effect binding's kind (+276) whose +280 is the target object

// The quaternion {x, y, z, w} of the Euler half-angles a / b / c from their
// cosines and sines (the compiler's inline sequence: products rounded to
// single, fmadds / fmsubs for the last step), stored at q.
inline void EulerQuat6(u32 q, f64 ca, f64 cb, f64 cc, f64 sa, f64 sb, f64 sc) {
    const f32 p2 = Muls(sa, sb), p3 = Muls(ca, cb), p4 = Muls(cb, sa), p5 = Muls(ca, sb);
    const f32 p6 = Muls(p2, sc), p7 = Muls(p3, sc), p8 = Muls(p4, sc), p1 = Muls(p5, sc);
    store<f32>(q + 12, Fmsubs(p3, cc, p6));
    store<f32>(q + 0, Fmadds(p2, cc, p7));
    store<f32>(q + 8, Fmsubs(p5, cc, p8));
    store<f32>(q + 4, Fmadds(p4, cc, p1));
}

// Word copies dst[k] = src[k], k < n (lfs / stfs pairs, raw bits).
inline void CopyW6(u32 dst, u32 src, u32 n) {
    for (u32 k = 0; k < n; ++k) store<u32>(dst + 4 * k, load<u32>(src + 4 * k));
}

}  // namespace vmx6
}  // namespace sly4
