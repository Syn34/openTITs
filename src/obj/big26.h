// Game code, large-function sweep part 26: helpers shared by big26_*.cpp
// (the Havok body / shape calls).
#pragma once

#include "native/objtypes.h"

namespace sly4::obj {

namespace b26fn {
constexpr u32 Hk_RigidBodyCinfoCtor = 0x0091FB50;     // (hkpRigidBodyCinfo*) (Hk_ShapeCinfoCtor91FB50)
constexpr u32 Hk_CinfoFromBody = 0x0091D208;          // (body, hkpRigidBodyCinfo*)
constexpr u32 Hk_SphereShapeCtor = 0x0089FE70;        // (hkpSphereShape*; f1 radius)
constexpr u32 Hk_TranslateShapeCtor = 0x00898B30;     // (shape, 10; f1 child radius, child, u8)
constexpr u32 Hk_BoxShapeCtor = 0x00899720;           // (hkpBoxShape*, const hkVector4* halfExtents; f1 radius)
constexpr u32 Hk_ListShapeCtor_008947A0 = 0x008947A0; // (hkpListShape*, hkpShape* const* children, n, 0) (name?)
constexpr u32 Hk_CharRigidBodyCtor_009D32E0 = 0x009D32E0;  // (hkpCharacterRigidBody*, const Cinfo*) (name?)
constexpr u32 Hk_CharRigidBodyGetBody = 0x009D19C8;   // (hkpCharacterRigidBody*) -> hkpRigidBody* (Hk_GetBody9D19C8)
constexpr u32 HkBody_SetName = 0x007CE950;            // (body + 120, const char*)
constexpr u32 Hk_SetTransform = 0x0091EAB8;           // (body, const hkTransform*)
constexpr u32 Hk_AddProperty = 0x0093FE00;            // (entity, u32 key, u64 value)
constexpr u32 Hk_AddEntity = 0x0092F2A8;              // (world, entity, 1)
constexpr u32 HkBody_AddListener = 0x0091A82C;        // (entity, listener*)
constexpr u32 Hk_RemoveReference = 0x007D4EA8;        // (hkReferencedObject*) (AnimSkel_Release7D4EA8)
constexpr u32 Obj_FindBone = 0x000DA9D0;              // (Obj*, u32 hash) -> bone or 0 (bone +4: its matrix)
}  // namespace b26fn

// big26_charbody.cpp: a character's Havok rigid body and bone frames.
struct CharObj;
void Char_CreateRigidBody_00247068(CharObj* o);

}  // namespace sly4::obj
