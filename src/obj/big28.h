// Game code, large-function sweep part 28: helpers shared by big28_*.cpp.
#pragma once

#include <bit>
#include <cmath>

#include "big27.h"

namespace sly4 {
namespace big28 {

using namespace opentit;
using namespace opentit::decomp;
using big17::SetF1;
using big19::VCall;
using big23::Adds;
using big23::Muls;
using big23::Subs;
using big27::Fw27;
using big27::VCallEO27;
using big5::Lvx;
using big5::Stvx;
using big5::V4;
using big7::Andc;
using big7::Sel;
using mid15::Fb;
using mid15::Fmadds;
using mid15::Fmsubs;
using mid13::LfsF;
using mid15::Lf;
using mid15::S;
using mid16::Divs;
using mid16::Sqrts;
using mid15::Sf;
using vmx2::Fill4;
using vmx2::LoadMasks3;
using vmx2::Masks3;
using vmx2::RowSel;
using vmx2::RowSelW;
using vmx2::SplatW;
using vmx2::Zero4;

namespace b28fn {
constexpr u32 HkBody_SetPosition_0091DFF0 = 0x0091DFF0;  // (hkpRigidBody*, const Vec4*) (through its motion +224 vfn[48]) (name?)
constexpr u32 HkBody_SetPosRot = 0x0091ECA8;    // (hkpRigidBody*, const Vec4* pos, const Vec4* rot)
constexpr u32 HkHits_Sort = 0x008871E8;         // (Collector*) sort the 48-byte hits
constexpr u32 Hk_Activate = 0x0091A4C8;         // (body)
constexpr u32 HkHeap = 0x011DF31C;              // Havok heap allocator object (vfn[20] (p, bytes) frees)
constexpr u32 ZeroVec = 0x01164B40;             // the global zero vector
constexpr u32 AxisZ = 0x01164B70;               // the global vector after UpVec ({0, 0, 1})
constexpr u32 VtHitCol = 0x00F1DE58;            // ray hit collector vtable (48-byte hit array)
constexpr u32 VtHitColEnd = 0x00B13C84;         // stored over it when the collector ends
}  // namespace b28fn

// vsel row of four words: x / y / z / w from lanes 0 / 1 / 2 / 3 through
// the three lane masks of a pool ({y}, {z}, {w}).
inline V4 RowSel4(const V4& x, const V4& y, const V4& z, const V4& w, const Masks3& k) {
    return Sel(Sel(Sel(x, y, k.m[0]), z, k.m[1]), w, k.m[2]);
}
// All four lanes the f32 the double rounds to (stfs x4 + lvx).
inline V4 Fill4D(f64 x) { return Fill4(static_cast<f32>(x)); }

}  // namespace big28
}  // namespace sly4
