// Game code, large-function sweep part 9: helpers shared by big9_*.cpp
// (callees part 8 named and further still-recompiled game functions of
// 400+ instructions).
#pragma once

#include <bit>
#include <cmath>

#include "big8.h"

namespace sly4 {
namespace big9 {

using namespace opentit;
using namespace opentit::decomp;
using mid1::LfsBits;
using obj19::VCall;
using obj25::Sx;
using big4::CopyW;
using big5::V4;
using big5::Lvx;
using big5::Stvx;
using big6::LaneW;
using big6::Lvlx;
using big7::F;
using big7::Andc;
using big7::Fmadds;
using big7::Fmsubs;
using big7::SetF1Bits;

namespace b9fn {
using namespace big8::b8fn;
constexpr u32 kObjBaseInit = 0x000D3578u;      // (o, tmpl): Object base init (name?)
constexpr u32 kObjSetShadow = 0x000E0000u;     // Obj_SetShadow0E0000 (o, u32 def name) -> u32
constexpr u32 kEffectDefFind = 0x004BDEDCu;    // EffectDef_Find (u32 hash, u32 set_name, u8 search_all) -> def
constexpr u32 kShapeInitBody = 0x00011410u;    // Shape_InitBody (Shape* out, system, body, 0, u32, u16 mask)
constexpr u32 kVolNotify = 0x000112D8u;        // Volume_Notify112D8 (system, volume)
constexpr u32 kVolSetShape = 0x00637D14u;      // Volume_SetShape637D14 (dst, src)
constexpr u32 kShapeRel = 0x00638498u;         // Shape_Release638498 (Shape*)
constexpr u32 kHkAddEntity = 0x0092F2A8u;      // hkpWorld::addEntity(world, entity, 1) (Havok)
constexpr u32 kHkRemoveEntity = 0x0092FFC8u;   // hkpWorld::removeEntity(hkResult* out, world, entity) (Havok)
constexpr u32 kPhysBodyAttach = 0x000E2AB0u;   // (PhysObj* o, u32 index, const f32 axis[4]) (name?)
}  // namespace b9fn

constexpr u32 kShapeSystem = 0x00F557ECu;      // shape / volume system object (Volume_Notify's first argument)
constexpr u32 kWorldPtr = big8::kWorldPtr;     // World*
constexpr u32 kWorldPhys = big8::kWorldPhys;   // World: physics holder (+4 hkpWorld*)
constexpr u32 kZero3 = big8::kZero3;           // the zero vector

inline u32 FBits(f32 x) { return std::bit_cast<u32>(x); }

// vandc chain of a splatted word with three lane masks (raw words).
inline V4 AndcSplat3(u32 w, const V4& m0, const V4& m1, const V4& m2) {
    V4 v;
    for (u32 k = 0; k < 4; ++k) v.e[k] = std::bit_cast<f32>(w & ~LaneW(m0, k) & ~LaneW(m1, k) & ~LaneW(m2, k));
    return v;
}

}  // namespace big9
}  // namespace sly4
