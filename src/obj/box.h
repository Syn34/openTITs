// Axis-aligned boxes {f32 min[4], f32 max[4]} (16-byte aligned, the w lanes
// unused) and the zone shapes (six planes) the zone mask tests use.
#pragma once

#include "obj4.h"

namespace sly4 {
namespace box {

using namespace opentit;

constexpr u32 kBoxMin = 0, kBoxMax = 16;
constexpr u32 kHalf = 0x00556F94;          // f32 0.5 (Box_Center)
constexpr u32 kHalfVmx = 0x005570E8;       // f32 0.5 (Box_Transform)
constexpr u32 kXyzMask = 0x00557420;       // u32[4] {0, 0, 0, ~0}: masks the w lane
constexpr u32 kZoneShapeZero = 0x00576E20; // f32 0.0
constexpr u32 kZonesPlaneZero = 0x00577330; // f32 0.0
constexpr u32 kShapePlanes = 6;            // {f32 nx, ny, nz, d} each
constexpr u32 kBoxCorners = 8;

namespace bfn {
constexpr u32 Box_Corners = 0x00557690;    // (const box*, f32 out[8][4], const Matrix*): the 8 corners transformed (name?)
}  // namespace bfn

}  // namespace box
}  // namespace sly4
