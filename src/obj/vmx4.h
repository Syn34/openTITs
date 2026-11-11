// Game code, VMX (AltiVec) function sweep part 4: helpers shared by
// vmx4_*.cpp (game functions the size sweeps skipped because they use VMX).
// Builds on vmx3.h (Fsels / Len3s / Rsqrte4 / Cmpge4 / Cmpeq4 / Or4 /
// SqrtNewton4 / Cross4 / HSum3) and, through it, vmx2.h / vmx1.h and the
// big5..big17 lane helpers.
#pragma once

#include <bit>
#include <cmath>

#include "vmx3.h"

namespace sly4 {
namespace vmx4 {

using namespace opentit;
using namespace opentit::decomp;
using namespace sly4::vmx3;

namespace v4fn {
constexpr u32 String_Alloc = 0x0055578C;  // char* (u32 n) (reads r3 only)
constexpr u32 String_CopyN = 0x00553638;  // (dst, src, s32 n)
constexpr u32 String_Format = 0x005534D0;  // (fmt, ...) -> rotating static buffer
constexpr u32 memset = 0x00726E54;         // (dst, c, n)
constexpr u32 Obj_PlayTextAnim = 0x00298270;  // u32 (T* o, u32 text, u32 key, bool b)
constexpr u32 Text_Lookup = 0x0034A398;     // (u32 id) -> text entry or 0
constexpr u32 Obj_FindComp = 0x0034CC34;    // (owner, key) -> component or 0
constexpr u32 Track_Play = 0x003535C4;      // (track, text, bool, -, -, 0; f1, f2)
constexpr u32 Obj_SetF32Via = 0x00354184;   // (T*; f1): T->+36 ->+60 = f1
constexpr u32 Layer_Query = 0x004B6760;     // (Set*, Layer*, Desc*, Info*, s32)
constexpr u32 Scope_Lookup = 0x00760C54;    // T* (Scope*, u32 key)
constexpr u32 WorldObj_MarkDirty = 0x0057D000;  // (obj): reads r3 only
constexpr u32 Obj_BoneMatrix = 0x000CE688;  // (f32 out[4][4], Object*, s32 bone, u8 world, u8 raw)
constexpr u32 Quat_ToMatrix = 0x00553A90;  // (f32 out[4][4], const f32 q[4], const f32 t[3])
constexpr u32 Obj_Pick = 0x00451E08;  // u32 (const T* o) (reads r3 only)
constexpr u32 Field_Get160Body = 0x000DD880;  // f32* (T* o): the 4x4 body matrix
constexpr u32 Rope_Probe = 0x0017F430;  // u32 (a, from[3], 0x01164B90, k, to[3], o+112, f32* 1000, 0, 0; f1 2.0)
constexpr u32 Matrix_FromAxisAngle = 0x0056A410;  // (f32 out[3][4], const f32 axis[3]; f1 angle)
constexpr u32 Box_ContainsPoint = 0x005573D0;  // bool (const Box* b, const f32 p[3])
constexpr u32 Obj_GetChannel = 0x0045B160;  // f32 (const T* o, s32 i)
constexpr u32 acosf = 0x005E649C;  // (f1) -> f1
constexpr u32 Quat_FromMatrix = 0x00555478;  // (f32 out[4], const f32 m[4][4])
constexpr u32 Skeleton_Update = 0x0057E8D0;  // (skel) (reads r3 only)
constexpr u32 Matrix_Multiply = 0x005541F0;  // (Matrix* a, const Matrix* b): a = a x b
constexpr u32 Math_Cosf = 0x007344E0;  // (f1) -> f1
constexpr u32 Obj_BaseCtor = 0x000E9DD8;  // (T* o, const Desc* d)
constexpr u32 Hk_Activate = 0x0091A4C8;  // (body) (reads r3 only)
constexpr u32 HkWorld_CastRay = 0x0092E510;  // (world, const hkpWorldRayCastInput*, hkpRayHitCollector*)
constexpr u32 HkAllHits_Sort = 0x00887E28;  // (hkpAllRayHitCollector*): sort hits by fraction
constexpr u32 Hk_SetAabb = 0x00924B90;  // (body, const hkAabb*)
constexpr u32 Hk_SetTransform = 0x00929E98;  // (body, const f32 rows[4][4]) (name?)
constexpr u32 WorldObj_UpdateLod = 0x0057EE00;  // (obj, 0, 0) (reads r3 / r5)
constexpr u32 hkpKeyFrameUtility_applyHardKeyFrame = 0x009DD41C;  // (const hkVector4& pos, const hkQuaternion& q, -, hkpRigidBody*; f1 1/dt)
}  // namespace v4fn

// The low word of the caller's stack slot at sp + off (stack arguments: the
// 9th integer argument's word is at +116, each further one 8 bytes on).
inline u32 StkW(const PPUContext& ctx, u32 off) { return load<u32>(static_cast<u32>(ctx.gpr[1]) + off); }
inline u8 StkB(const PPUContext& ctx, u32 off) { return load<u8>(static_cast<u32>(ctx.gpr[1]) + off); }
// lfs: the f32 word at ea as the double the FPR holds.
inline f64 Lfs4(u32 ea) { return std::bit_cast<f64>(LfsBits(load<u32>(ea))); }

// The rigid inverse the compiler builds of the 4x4 rows at m (as vmx3's
// Pose_ToLocal): the transposed rotation (c0 from vmrghw, c1 / c2 through
// the two vperm masks p0 / p1) and t = -row3 through it.
struct InvRows4 {
    V4 c[3];
    V4 t;
};
inline InvRows4 InvRigid4(u32 m, const V4& p0, const V4& p1) {
    const V4 r0 = Lvx(m), r1 = Lvx(m + 16), r2 = Lvx(m + 32), r3 = Lvx(m + 48);
    const V4 nt = Sub4(Zero4(), r3);
    const V4 h = Mrghw(r0, r2), l = Mrglw(r0, r2);
    InvRows4 o;
    o.c[0] = Mrghw(h, r1);
    o.c[1] = Perm(h, r1, p0);
    o.c[2] = Perm(l, r1, p1);
    o.t = XformDir(o.c[0], o.c[1], o.c[2], nt);
    return o;
}
// Three f32 words at ea as lanes 0..2 (lane 3 zero; only lanes 0..2 used).
inline V4 W3(u32 ea) { return V4{{F(ea), F(ea + 4), F(ea + 8), 0.0f}}; }
// Lanes 0..2 of v stored as raw words at ea.
inline void St3(u32 ea, const V4& v) {
    for (u32 k = 0; k < 3; ++k) store<u32>(ea + 4 * k, LaneW(v, k));
}

// The game's handle table (*0x00F557F8): a handle h = idx | slot << 16 names
// the 16-byte record tab->+260 + 16 * slot when idx != 0, slot < (s32)
// tab->+256, the record's word +12 has ((w >> 15) & 0xFFFF) == idx and its
// top bit (byte +12 bit 0x80) clear. Returns the record (its +8 the object
// pointer) or 0.
constexpr u32 kHandleTab = 0x00F557F8;
inline u32 HandleRec(u32 tab, u32 h) {
    const u32 idx = h & 0xFFFFu;
    if (idx == 0) return 0;
    const u32 slot = h >> 16;
    if (!(static_cast<s32>(slot) < static_cast<s32>(load<u32>(tab + 256)))) return 0;
    const u32 e = load<u32>(tab + 260) + slot * 16;
    if (((load<u32>(e + 12) >> 15) & 0xFFFFu) != idx) return 0;
    if ((load<u8>(e + 12) & 0x80u) != 0) return 0;
    return e;
}

}  // namespace vmx4
}  // namespace sly4
