// Game code, large-function sweep part 17: helpers shared by big17_*.cpp
// (the Havok body list of the damage def built by XmlDef_NewFromNodeCA038,
// and further still-recompiled large game functions).
#pragma once

#include <bit>
#include <cmath>

#include "big16.h"

namespace sly4 {
namespace big17 {

using namespace opentit;
using namespace opentit::decomp;
using big13::LfsX;
using mid1::LfsBits;
using obj19::VCall;

namespace b17fn {
constexpr u32 DefBase_Ctor_000C2E8C = 0x000C2E8C;     // (Def* p, const Record* rec): the shared def base constructor (name?)
constexpr u32 Obj_FindName = 0x0034654C;              // (o, const char* s) -> entity or 0
constexpr u32 Kind_FromHash = 0x003557B8;             // (hash) -> collision layer 0..28
constexpr u32 PtrVector_Construct = 0x005516E0;       // (PtrVector*)
constexpr u32 SmallAlloc = 0x0055238C;                // (size) -> p or 0
constexpr u32 BodyVector_Add_00745720 = 0x00745720;   // (vec, const BodyRec* 160 bytes): push a copy (name?)
constexpr u32 HkBody_SetName = 0x007CE950;            // (body + 120, const char* name) (Havok)
constexpr u32 Hk_RemoveReference = 0x007D4EA8;        // (hkReferencedObject* or 0) (AnimSkel_Release7D4EA8)
constexpr u32 Hk_TranslateShapeCtor_00898B30 = 0x00898B30;  // (shape, 10, child, 1; f1 child radius) (Havok; name?)
constexpr u32 Hk_BoxShapeCtor = 0x00899720;           // (BoxShape*, const Vector4* halfExtents; f1 radius) (Havok)
constexpr u32 Hk_CinfoFromBody = 0x0091D208;          // (body, hkpRigidBodyCinfo*) (Havok)
constexpr u32 Hk_RigidBodyCtor_0091E100 = 0x0091E100; // (body, const hkpRigidBodyCinfo*) (Hk_ShapeCtor91E100; Havok)
constexpr u32 Hk_SetMotionFlags_0091D470 = 0x0091D470; // (body, 1, 1, 0) for keyframed / fixed bodies (Havok; name?)
constexpr u32 Hk_SetMass_0091D618 = 0x0091D618;      // (body; f1 mass) (Havok; name?)
constexpr u32 Hk_RigidBodyCinfoCtor = 0x0091FB50;     // (hkpRigidBodyCinfo*) (Hk_ShapeCinfoCtor91FB50; Havok)
constexpr u32 Hk_RemoveEntity = 0x0092FFC8;           // (u8* out, world, body) (Havok)
constexpr u32 Hk_AddProperty = 0x0093FE00;            // (entity, key, u64 value) (Havok)
}  // namespace b17fn

constexpr u32 kWorldPtr = 0x00F65CA0;      // World*
constexpr u32 kWorldPhys = 0x001281D4;     // World +0x1281D4: the physics holder

// The thread's Havok memory router: *(r13 - 28332) +44 allocator, vtable
// +8 alloc(a, size); the u16 memory size goes to +4 of the result (also when
// the allocation failed: a store to address 4, as the original does).
inline u32 HkAlloc(PPUContext& ctx, u32 size) {
    const u32 a = load<u32>(load<u32>(static_cast<u32>(ctx.gpr[13]) - 28332u) + 44);
    const u32 p = static_cast<u32>(VCall(ctx, a, 8, {a, size}));
    store<u16>(p + 4, static_cast<u16>(size));
    return p;
}

// f1 = the lfs of the word at ea.
inline void SetF1(PPUContext& ctx, u32 ea) { ctx.fpr[1] = std::bit_cast<f64>(LfsBits(load<u32>(ea))); }

// hkpWorldObject property search (inlined twice per use): the index of key
// in the entity's property array (+124 entries of 16 bytes {u32 key, ...,
// u64 value at +8}, +128 s32 count; both re-read every step), or -1.
inline s32 HkPropIndex(u32 ent, u32 key) {
    for (s32 i = 0;; ++i) {
        const s32 n = load<s32>(ent + 128);
        if (i >= n) return -1;
        if (load<u32>(load<u32>(ent + 124) + 16u * static_cast<u32>(i)) == key) return i;
    }
}
inline u64 HkPropValue(u32 ent, s32 i) { return load<u64>(load<u32>(ent + 124) + 16u * static_cast<u32>(i) + 8); }

}  // namespace big17
}  // namespace sly4
