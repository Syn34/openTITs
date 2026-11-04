// World objects, part 13: the spline follower's distance-matched advance
// (Follower_AdvanceAlt) and the callees part 12 left recompiled (the
// object light cache steps, the draw packet / record helpers, the
// topology applies and the spline tangent evaluation).
#pragma once

#include <bit>
#include <cmath>

#include "obj12.h"

namespace sly4 {
namespace obj13 {

using namespace opentit;

namespace o13fn {
constexpr u32 Spline_PointTangent = 0x005606C8;  // (SplineData*, -, f32 pos[4], f32 tan[4], u8 hint, 0; f1 t) -> u8 hint (name?)
}  // namespace o13fn

// ---------------------------------------------------------------- spline followers (obj11.h kFo*)
// Both advances read +100 as the spline record they follow (+0 its world
// matrix, +80 SplineData*); Follower_Update tests the same word for
// "active" and reads +104 as a second record for the look direction.
constexpr u32 kFoPhase = 64, kFoOffset = 68, kFoSpeed = 72, kFoScale = 76;
constexpr u32 kFoLength = 88;                   // f32: the spline's length in parameter units (name?)
constexpr u32 kFoWrapSrc = 92, kFoWrapDst = 96;
constexpr u32 kFoRecordA = 100;                 // the followed spline record (nonzero = active)
constexpr u32 kFoOrient = 0x80;                 // +108: turn the rotation onto the travel direction
constexpr u32 kFoUp = 16, kFoForward = 32;      // matrix rows

// Follower_AdvanceAlt's constants: f32 {10000, 32, 1, 0.5, 0.05, 0.001, 0.0001}.
constexpr u32 kAaConsts = 0x0057AF00;
constexpr u32 kAaBig = 0, kAaSteps = 4, kAaOne = 8, kAaHalf = 12, kAaLook = 16, kAaParallel = 20, kAaTiny = 24;
constexpr u32 kAaLookTries = 10;

// ---------------------------------------------------------------- the object light cache (obj +288, obj12.h kLc*)
// +0 f32[3] the ambient colour, then per applied light (4 entries) +16
// vec4 the colour, +80 vec4 the position (or the negated direction), +144
// vec4 the falloff parameters, +208 u8 the type (3 = none); +215 u8 the
// entries in use, +220 the ambient LightInstance*, +224 u32[4] the
// lights (LightInstance* for ObjLight_Blend, 1-based light slot indices
// for ObjLight_Apply) (name?).
constexpr u32 kLcAmbient = 0, kLcColour = 16, kLcPos = 80, kLcParams = 144, kLcType = 208;
constexpr u32 kLcUsed = 215, kLcAmbientLight = 220, kLcLights = 224;
constexpr u32 kLcEntries = 4, kLcStride = 16;
constexpr u8 kLcTypeNone = 3;
constexpr u32 kLcNoColour = obj10::kOrigin;     // f32[3] zero: an empty entry's colour
constexpr u32 kLcNoPos = 0x01164B50;            // f32[3]: an empty entry's position (name?)
constexpr u32 kLcBlendOne = 0x00582840;         // f32 1: the ambient scale's cap

// ---------------------------------------------------------------- light slots (obj12.h kLightSlotSize)
// +0 vec4 the position, +16 vec4 the falloff parameters, +32 f32[3] the
// colour, +61 u8 the light type (LightInstance_Update).
constexpr u32 kLsPos = 0, kLsParams = 16, kLsColour = 32, kLsType = 61;
constexpr u8 kLightSpot = 2;                    // classes.h kLightType 2 (name?)
constexpr s32 kLightDirectional = 1;            // kLightType 1: the direction is world row 2 (+112) (name?)
constexpr u32 kObjForward = 112;                // obj f32[3]: world matrix row 2

}  // namespace obj13
}  // namespace sly4
