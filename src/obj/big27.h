// Game code, large-function sweep part 27: helpers shared by big27_*.cpp.
#pragma once

#include <bit>
#include <cmath>

#include "big26.h"

namespace sly4 {
namespace big27 {

using namespace opentit;
using namespace opentit::decomp;
using big17::HkAlloc;
using big17::HkPropIndex;
using big17::HkPropValue;
using big17::SetF1;
using big19::VCall;
using big23::Adds;
using big23::Muls;
using big23::Subs;
using big26::Add4N26;
using big26::And26;
using big26::Row3N26;
using big5::Lvx;
using big5::Stvx;
using big5::V4;
using big7::Andc;
using big7::Sel;
using big7::SetF1Bits;
using mid15::Fb;
using mid15::Fmadds;
using mid13::LfsF;
using mid15::Lf;
using mid15::S;
using mid16::Divs;
using mid16::Sqrts;
using mid15::Sf;
using vmx2::Fill4;
using vmx2::LoadMasks3;
using vmx2::Masks3;
using vmx2::SplatW;
using vmx2::Zero4;

namespace b27fn {
constexpr u32 Def_BaseCtor = 0x000C3078;            // (Def*, rec) (Thunk_000C3078)
constexpr u32 Obj_FindName = 0x0034654C;            // (table, name) -> object or 0
constexpr u32 Hk_RigidBodyCinfoCtor = 0x0091FB50;   // (hkpRigidBodyCinfo*)
constexpr u32 Hk_CinfoFromBody = 0x0091D208;        // (body, hkpRigidBodyCinfo*)
constexpr u32 Hk_RigidBodyCtor_0091E100 = 0x0091E100;  // (hkpRigidBody*, const Cinfo*) (Hk_ShapeCtor91E100) (name?)
constexpr u32 Hk_RemoveEntity = 0x0092FFC8;         // (out*, world, entity)
constexpr u32 Hk_BoxShapeCtor = 0x00899720;         // (hkpBoxShape*, const hkVector4* halfExtents; f1 radius)
constexpr u32 Hk_TranslateShapeCtor = 0x00898B30;   // (shape, 10, -, child, u8; f1 child radius); r5 unread
constexpr u32 HkBody_SetName = 0x007CE950;          // (body + 120, const char*)
constexpr u32 Hk_RemoveReference = 0x007D4EA8;      // (hkReferencedObject*)
constexpr u32 Hk_SetMotion = 0x0091D470;            // (body, type, 1, 0)
constexpr u32 Hk_SetMass = 0x0091D618;              // (body; f1 mass)
constexpr u32 Hk_AddProperty = 0x0093FE00;          // (entity, u32 key, u64 value)
constexpr u32 Game = 0x00F65CA0;                    // Game*
constexpr u32 GamePhys = 0x001281D4;                // Game +0x1281D4: the named physics objects table
constexpr u32 VtTranslate = 0x00F1E7F8;             // stored over a translate shape after its constructor
constexpr u32 BoxRadius = 0x00F9203C;               // f32 0.05 (box convex radius)
constexpr u32 ZeroVec = 0x01164B40;                 // the global zero vector
constexpr u32 Obj_IndexByHash = 0x000D14DC;         // (owner, u32 hash) -> bone index
constexpr u32 Obj_BoneMatrix = 0x000CE688;          // (f32 out[4][4], owner, s32 bone, u8 world, u8 raw)
constexpr u32 Rec80_Find = 0x005848CC;              // (table, key) -> 80-byte record or 0
constexpr u32 Matrix_Multiply = 0x005541F0;         // (Mat34* io, const Mat34*)
constexpr u32 HkShape_FromDesc = 0x00277C90;        // (const ShapeDesc*) -> hkpShape* or 0
constexpr u32 Hk_ColliderCtor_009265D4 = 0x009265D4;  // (body, shape, const hkTransform*, u32 filter) (name?)
constexpr u32 Body_Add = 0x00931D10;                // (world, body)
constexpr u32 Obj_RefPos = 0x0026A42C;              // (f32 out[3], Obj*)
constexpr u32 Matrix_SplitScale = 0x00554858;       // (const f32 m[16], f32 rot[16], f32 scale[3])
constexpr u32 Hk_SetBodyTransform = 0x00929E98;     // (hkpRigidBody*, const f32 rows[4][4])
constexpr u32 HkHeap = 0x011DF31C;                  // Havok heap allocator object (vfn[20] (p, bytes) frees)
constexpr u32 GetU8 = 0x004A08C0;                   // (const u8*) -> *p
constexpr u32 Obj_Check = 0x001F465C;               // (obj) -> bool
constexpr u32 Anim_SetTarget = 0x0030DBF0;          // (o, x, -, u8 force; f32 t)
constexpr u32 AnimCtl_Play_0030BE48 = 0x0030BE48;   // (anim player, anim, -, -, const f32* params, const Mat44* at, u8; f1, f2) (name?)
constexpr u32 Effect_Spawn = 0x004A5FC8;            // (u32 name, owner, value, u8 start) -> Effect*
constexpr u32 PtrVector_Reserve = 0x00551718;       // (vec, u32 n)
constexpr u32 Matrix_RotationY = 0x00553E18;        // (Mat34*; f1 angle)
constexpr u32 String_Assign = 0x00555F80;           // (String*, const char*)
constexpr u32 String_Length = 0x00555FA4;           // (const char*) -> u32
constexpr u32 Obj_Detach = 0x000D72C0;              // (holder, Object*)
constexpr u32 Matrix_FromDir = 0x004A54F8;          // (const vec3* dir, Matrix* m)
constexpr u32 Effect_SetHash = 0x004CF988;          // (Effect*, u32 hash) -> bool
constexpr u32 Hk_Activate = 0x0091A4C8;             // (body)
constexpr u32 HkBody_AddListener_0091A82C = 0x0091A82C;  // (body, listener)
constexpr u32 HkBody_AddListener_0091A5F8 = 0x0091A5F8;  // (body, listener)
constexpr u32 Array_Reserve = 0x00558F3C;           // (array, u32 n)
constexpr u32 Rng = 0x01164888;                     // u64 +0 the last output (16 bits); u32 +8 the state
constexpr u32 SmallAlloc = 0x0055238C;              // (u32 size) -> p or 0
constexpr u32 HkWorld_CastRay = 0x0092E2E8;         // (hkpWorld*, const hkpWorldRayCastInput*, hkpWorldRayCastOutput*)
constexpr u32 Marker_Init_003087C0 = 0x003087C0;    // (Marker*, const f32 pos[3]) (name?)
constexpr u32 Marker_SetState_003081C8 = 0x003081C8;  // (Marker*, u32 state) (name?)
constexpr u32 UpVec = 0x01164B60;                   // the global up vector
constexpr u32 IdentityMat = 0x01164BD0;             // the global identity matrix (4 rows of 16)
}  // namespace b27fn

// Call through slot `slot` of o's vtable with the entry address in the
// register after the arguments and the OPD in the one after that (the
// reverse of VCall's order, as some call sequences leave them).
inline u64 VCallEO27(PPUContext& ctx, u32 o, u32 slot, std::initializer_list<u64> args) {
    const u32 opd = load<u32>(load<u32>(o) + slot);
    u64 r[8] = {};
    u32 n = 0;
    for (u64 a : args) r[n++] = a;
    r[n++] = load<u32>(opd);
    r[n++] = opd;
    switch (n) {
    case 2: return call_ptr(ctx, opd, {r[0], r[1]});
    case 3: return call_ptr(ctx, opd, {r[0], r[1], r[2]});
    case 4: return call_ptr(ctx, opd, {r[0], r[1], r[2], r[3]});
    case 5: return call_ptr(ctx, opd, {r[0], r[1], r[2], r[3], r[4]});
    case 6: return call_ptr(ctx, opd, {r[0], r[1], r[2], r[3], r[4], r[5]});
    case 7: return call_ptr(ctx, opd, {r[0], r[1], r[2], r[3], r[4], r[5], r[6]});
    default: return call_ptr(ctx, opd, {r[0], r[1], r[2], r[3], r[4], r[5], r[6], r[7]});
    }
}

// The global generator's mix step (the state is kept rotated by 16).
inline u32 RngMix27(u32 s) { return (std::rotl(s, 4) + std::rotl(s, 16)) ^ 0x87654321u; }

// f32 from a word.
inline f32 Fw27(u32 ea) { return std::bit_cast<f32>(load<u32>(ea)); }

}  // namespace big27
}  // namespace sly4
