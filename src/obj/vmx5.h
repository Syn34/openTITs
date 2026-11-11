// Game code, VMX (AltiVec) function sweep part 5: helpers shared by
// vmx5_*.cpp (game functions the size sweeps skipped because they use VMX).
// Builds on vmx4.h (StkW / StkB / Lfs4 / InvRigid4 / W3 / St3 / HandleRec)
// and, through it, vmx3.h / vmx2.h / vmx1.h and the big5..big17 lane helpers.
#pragma once

#include <bit>
#include <cmath>

#include "vmx4.h"

namespace sly4 {
namespace vmx5 {

using namespace opentit;
using namespace opentit::decomp;
using namespace sly4::vmx4;
using big16::Frsp;

namespace v5fn {
constexpr u32 Matrix_NormalizeInPlace = 0x0056D374;  // (f32 m[4][4]) (reads r3 only)
constexpr u32 Curve_Ease = 0x004A83D8;  // f32 (f32 v, f32 target, f32 limit, f32 dt) (reads f1..f4)
constexpr u32 Matrix_RotationX = 0x00553D18;  // (f32 out[4][4]; f1 angle)
constexpr u32 Matrix_RotationY = 0x00553E18;  // (f32 out[4][4]; f1 angle)
constexpr u32 Math_Cosf = 0x007344E0;  // f1 -> f1 (reads f1 only)
constexpr u32 Math_Sinf = 0x007345F0;  // f1 -> f1 (reads f1 only)
constexpr u32 Libm_Unary5E6578 = 0x005E6578;  // f32 (f32) (reads f1 only)
constexpr u32 Rec80_Find = 0x005848CC;  // ({?, data, u16 count}*, key) -> 80-byte record or 0
constexpr u32 Ui_Find = 0x004D5544;  // (const char* path, parent) -> widget or 0
constexpr u32 SmallAlloc = 0x0055238C;  // (u32 size) -> p or 0
constexpr u32 PtrVector_Reserve = 0x00551718;  // (PtrVector*, u32 capacity)
constexpr u32 String_Assign = 0x00555F80;  // (dst, src): strcpy
constexpr u32 Nav_ClosestPoint147B8 = 0x000147B8;  // (f32 out[3], Obj*, const f32 target[3])
constexpr u32 Obj_RandomSpawnPoint = 0x0001E068;  // (f32 out[4], Obj*)
constexpr u32 Obj_SubAddr_00452024 = 0x00452024;  // T* (T* o): o->+60 + 80 (reads r3 only)
constexpr u32 AwayPoint1E420 = 0x0001E420;  // (f32 out[3], Obj*, const f32 dir[3])
constexpr u32 Vec_SignedAngle16680 = 0x00016680;  // f32 (const f32 a[3], const f32 b[3])
constexpr u32 Path_Query6C0AA4 = 0x006C0AA4;  // (path, const f32 p[3], ctx)
constexpr u32 Path_Point6C1B94 = 0x006C1B94;  // (f32 out[3], path)
constexpr u32 HkBoxShape_Ctor = 0x00899720;  // (hkpBoxShape*, const hkVector4* halfExtents; f1 radius)
constexpr u32 HkCapsuleShape_Ctor = 0x00899B24;  // (hkpCapsuleShape*, const hkVector4* a, const hkVector4* b; f1 radius) (name?)
constexpr u32 HkSphereShape_Ctor = 0x0089FE70;  // (hkpSphereShape*; f1 radius)
constexpr u32 Obj_TestE0BCC = 0x000E0BCC;  // bool (Obj*, const HitEvent*)
constexpr u32 HkHits_Sort8871E8 = 0x008871E8;  // (collector*): sort 48-byte hits by fraction
constexpr u32 Obj_FindBone_000DA9D0 = 0x000DA9D0;  // Bone* (T* o, u32 name)
constexpr u32 Mat34_FromRotPos553B4C = 0x00553B4C;  // (Mat34* out, const Mat33*, const f32 p[3])
constexpr u32 Aim_SolveLaunch = 0x004A2690;  // bool (const AimQuery*, f32 dir[3], f32* time)
constexpr u32 Obj_SpawnDef0EDE30 = 0x000EDE30;  // Obj* (def, a, rot, dir, -, s, owner, frame, [9th on the stack]; f1)
constexpr u32 LauncherAimEE310 = 0x000EE310;  // bool (f32 dir[3], Launcher*)
constexpr u32 Ballistic_Solve4A2D78 = 0x004A2D78;  // bool (Shot*, f32 vel[3], f32* speed, f32 pos[3])
constexpr u32 LauncherAimBallisticEEB08 = 0x000EEB08;  // bool (f32 vel[4], Launcher*)
constexpr u32 View_ProjectPoint = 0x00522750;  // (view, f32 out[4], const f32 in[3])
constexpr u32 Hud_Track_002BB168 = 0x002BB168;  // (T* t, const f32 d[3]) (name?)
constexpr u32 Hud_Track_002BAC40 = 0x002BAC40;  // (T* t, const f32 d[3]; f1) (name?)
}  // namespace v5fn

constexpr u32 kUpAxis5 = 0x01164B60;  // f32[3] the up axis

}  // namespace vmx5
}  // namespace sly4
