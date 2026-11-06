// The renderer range's next recompiled functions (gfx23*.cpp): the last
// undone functions of 0x004FB000 - 0x005E0000 (the quad visibility tests,
// ...).
#pragma once

#include "gfx22.h"

namespace sly4::render {
namespace gfx23 {

namespace fn {
// Still recompiled or decompiled elsewhere (called from gfx23*.cpp).
constexpr u32 Matrix2_TransformPoint = 0x00577D8C;  // (const f32 m[2][4][4], const f32 p[3], f32 a[4], f32 b[4]) (obj/obj14_small.cpp)
constexpr u32 Vec4_Transform = 0x005539E4;          // (f32 v[4] in / out, const Matrix*) (view/camera.h)
constexpr u32 Tri_OverlapsBox = 0x00577788;         // (c[3], h[3], tri[3][4]) -> bool (gfx/gfx21.cpp)
constexpr u32 Quad_IntersectSegment = 0x005784E8;   // (quad[4][4], a[3], b[3], f32* t) -> bool (obj/obj15_small.cpp)
constexpr u32 Planes_ClipRay = 0x005786C0;          // (planes[6][4], p[3], d[3], f32* t0, f32* t1) -> bool (obj/obj15_small.cpp)
constexpr u32 Light_Accepts = 0x00591948;           // (LightInstance*, u32 1) -> bool (obj/obj7.cpp)
constexpr u32 LightAnim_Init = 0x00591820;          // (LightAnim* rec, LightInstance*) (obj/obj7.cpp)
constexpr u32 WorldObj_UpdateLod = 0x0057EE00;      // (obj, -, bool keep_skel_sphere) (obj/obj8_lod.cpp)
constexpr u32 PtrVector_Reserve = 0x00551718;       // (PtrVector*, u32 capacity) (container/vector.cpp)
constexpr u32 Matrix_Translation = 0x00553C70;      // (f32 m[4][4], const f32 t[3])
constexpr u32 Matrix_ScaleDiag = 0x00553BC4;        // (f32 m[4][4], const f32 s[3]) (obj/obj14_small.cpp)
constexpr u32 Matrix_RotationZ = 0x00553F18;        // (f32 m[4][4]; f1 angle) (gfx7_math.cpp)
constexpr u32 Matrix_MultiplyTo = 0x0055499C;       // (out[16], a[16], b[16])
constexpr u32 Frustum_FromMatrix = 0x004FDEA0;      // (Frustum*, const f32 m[16])
constexpr u32 atan2f = 0x005E665C;                  // (f1 y, f2 x) -> f1
constexpr u32 Mat4_Ortho = 0x0056A358;              // (out; f1 l, f2 r, f3 b, f4 t, f5 n, f6 f) (view/view2.cpp)
constexpr u32 Matrix_Perspective = 0x0056A200;      // (f32 out[16]; f1 fov, f2 aspect, f3 near, f4 far)
constexpr u32 Plane_Transform = 0x0055E940;         // (f32 plane[4], const f32 m[4][4])
constexpr u32 Matrix_ObliqueClip = 0x0056A578;      // (f32 m[16], const f32 plane[4])
constexpr u32 Matrix_FromAxisAngle = 0x0056A410;    // (f32 out[16], const f32 axis[3]; f1 angle) (view/matrix.cpp)
}  // namespace fn

// Quad_InFrustum_00577E40: a quad is four 16-byte points (x, y, z, -).
// The frustum block is Quads_Prepare's sp+272: six planes (+0) then, at
// +96, the two 4x4 matrices Matrix2_TransformPoint takes (lanes a.xyzw,
// b.xy are the six plane distances, b.zw unused). The clipped polygon is
// projected through viewproj and tested against the clip-space box
// centre kClipBoxCentre (0, 0, 0.5) / half extents kClipBoxHalf (1, 1,
// 0.5), function-local statics guarded by bytes.
namespace quad {
constexpr u32 kPointSize = 16, kPlanesMatrices = 96;
constexpr u32 kConsts = 0x00577E30;          // f32 0, 1, 1e-5
constexpr u32 kClipBoxCentreGuard = 0x01196950;
constexpr u32 kClipBoxHalfGuard = 0x01196958;
constexpr u32 kClipBoxCentre = 0x01196960;   // f32[3]
constexpr u32 kClipBoxHalf = 0x01196970;     // f32[3]
// Quad_InClip_00578868: the clip volume (Quads_ClipSetup's): the Frustum
// layout above, then its eight corners (16 bytes each) at +224.
constexpr u32 kClipCorners = 224;
constexpr u32 kClipConsts = 0x0057885C;      // f32 1, 0
}  // namespace quad

// ViewCrop_Fit_0052D0F0: the view's +48 matrix (rows +48 .. +96) and
// its frustum (+112); kConsts f32 1, 0.5, 1e-3, -FLT_MAX, FLT_MAX, 2.
namespace crop {
constexpr u32 kMatrix = 48, kFrustum = 112;
constexpr u32 kConsts = 0x0052D0D4;
constexpr u32 kOne = 0x3F800000u, kMinusOne = 0xBF800000u;
}  // namespace crop

// View_Update_005115F0: the view's projection and derived matrices.
// +48 u32 (bits 13..15: the screen rotation in quarter turns), +49 u8
// projection (0 ortho, 2 perspective with the oblique clip plane +496,
// else perspective), +50 u8 (0x10: aspect-correct a quarter-turned
// screen with u16 +102 / +100), +52 u8 (0x80 dirty), +56 fov, +60 near,
// +64 far, +80 .. +92 the ortho box, +112 aspect, +272 the frustum, +752
// the view matrix (rows), +880 view x projection (+944 transposed),
// +1008 the projection, +1072 the view's 3x3 transposed (+1120 its
// translation, xyz from kViewPos), +1136 the roll matrix, +1264 the raw
// projection, +1328 its product with the view matrix. vfunc +16 runs
// between the two products.
namespace view {
constexpr u32 kRotBits = 48, kProjection = 49, kFlags50 = 50, kFlags52 = 52, kDirty = 0x80, kAspectFix = 0x10;
constexpr u32 kFov = 56, kNear = 60, kFar = 64, kOrthoL = 80, kOrthoT = 84, kOrthoR = 88, kOrthoB = 92;
constexpr u32 kAspect = 112, kAspectNum = 102, kAspectDen = 100, kClipPlane = 496;
constexpr u32 kFrustum = 272, kViewMat = 752, kViewProj = 880, kViewProjT = 944, kProj = 1008;
constexpr u32 kViewRot = 1072, kViewTrans = 1120, kRoll = 1136, kProjRaw = 1264, kProjView = 1328;
constexpr u32 kVUpdated = 16;
constexpr u32 kConsts = 0x005115C0;  // f32 pi/2, 1, -, -, then two vperm masks (+16, +32)
constexpr u32 kViewPos = 0x01164B40, kRollAxis = 0x01164B60;
}  // namespace view

// LightInstances_Start_00591A30: the bank's light animation records
// (obj6.h kLa*, 80 bytes each) for the accepted light instances. The
// holder (12 bytes, Bank +0): +0 the records, u16 +4 count, +6 count +
// 255, +8 capacity (count + 256). A light's +256 is its object, whose
// +256 is the parent; +164 flags 0x800 (registered) / 0x400 (in the
// zone vector), +260 the LOD target; the light keeps its record index
// at u16 +382.
namespace lightinst {
constexpr u32 kRecSize = 80, kHolderSize = 12, kSpare = 256;
constexpr u32 kHolderRecs = 0, kHolderCount = 4, kHolderSpare = 6, kHolderCap = 8;
constexpr u32 kObj = 256, kParent = 256, kFlags = 164, kLodTarget = 260, kRecIndex = 382;
constexpr u32 kRegistered = 0x800, kInVector = 0x400;
constexpr u32 kZoneVector = 0x0116BD00;      // PtrVector* {+0 data, u16 +4 cap, +8 count, +10 high}
constexpr u32 kZoneVectorOff = 0x0116BD04;   // u8: no vector registration
constexpr u32 kConsts = 0x005919F0;          // f32 1/65535, 0, 0.5, 2, 2pi, -, f64 pi, -pi, f32 pi/2, pi, 4/pi, -4/pi^2, 0.225, 1
constexpr u32 kRandomLast = 0x01164888;      // u64 (game/update.h)
constexpr u32 kRandomState = 0x01164890;     // u32
constexpr u32 kRandomMix = 0x87654321u;
constexpr u32 kConfigLights = 0x010C5A20;    // +108 = +96 when a non-type-2 light changes range
constexpr u32 kOwnerRange = 376, kOwnerType = 368, kOwnerVChanged = 36;
// The record (obj6.h's kLa* layout).
constexpr u32 kLaFlags = 12, kLaPhase = 32, kLaPeriod = 36, kLaType = 40, kLaDir = 44, kLaNoise = 48;
constexpr u32 kLaRandA = 52, kLaRandB = 56, kLaBase = 60, kLaAmp = 64, kLaOwner = 68;
// The record defaults: +0 25 (fade distance), +4 625, +8 400 (squared).
constexpr u32 kFade = 0x41C80000u, kCullSq = 0x441C4000u, kFullSq = 0x43C80000u;
}  // namespace lightinst

}  // namespace gfx23
}  // namespace sly4::render
