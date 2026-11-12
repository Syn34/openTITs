// Game code, large-function sweep part 26: helpers shared by big26_*.cpp.
#pragma once

#include <bit>
#include <cmath>

#include "big25.h"
#include "vmx6.h"

namespace sly4 {
namespace big26 {

using namespace opentit;
using namespace opentit::decomp;
using big17::HkAlloc;
using big19::VCall;
using big23::Adds;
using big23::Fsel;
using big23::Muls;
using big23::Subs;
using big5::CopyMat43W;
using big5::Lvx;
using big5::Stvx;
using big5::V4;
using big6::Lvlx;
using big6::Splat;
using big7::Andc;
using big7::Sel;
using mid15::Fb;
using mid15::Fmadds;
using mid15::Fmsubs;
using mid15::Lf;
using mid15::S;
using mid15::Sf;
using mid16::Divs;
using mid16::Sqrts;
using vmx2::Fill4;
using vmx2::LoadMasks3;
using vmx2::Masks3;
using vmx2::RowSelW;
using vmx2::Zero4;

namespace b26fn {
constexpr u32 Hk_RigidBodyCinfoCtor = 0x0091FB50;     // (hkpRigidBodyCinfo*) (Hk_ShapeCinfoCtor91FB50)
constexpr u32 Hk_CinfoFromBody = 0x0091D208;          // (body, hkpRigidBodyCinfo*)
constexpr u32 Hk_SphereShapeCtor = 0x0089FE70;        // (hkpSphereShape*; f1 radius)
constexpr u32 Hk_TranslateShapeCtor = 0x00898B30;     // (shape, 10, -, child, u8; f1 child radius); r5 unread
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
constexpr u32 Game = 0x00F65CA0;                      // Game*
constexpr u32 GamePhys = 0x001281D4;                  // Game +0x1281D4: the physics holder (+4 the hkpWorld)
}  // namespace b26fn

// vand a & m.
inline V4 And26(const V4& a, const V4& m) {
    V4 o;
    for (u32 k = 0; k < 4; ++k) o.e[k] = std::bit_cast<f32>(std::bit_cast<u32>(a.e[k]) & std::bit_cast<u32>(m.e[k]));
    return o;
}

// One lane of vmaddfp a * c + b (fused, single) with the AltiVec NaN
// priority: a NaN a, else b, else c comes out quieted (the host fma may
// pick another operand when two are NaN).
inline f32 VmaddLane26(f32 a, f32 c, f32 b) {
    auto q = [](f32 x) { return std::bit_cast<f32>(std::bit_cast<u32>(x) | 0x00400000u); };
    if (a != a) return q(a);
    if (b != b) return q(b);
    if (c != c) return q(c);
    return std::fma(a, c, b);
}
// big5::Row3 with that NaN priority: c2 * s2 + (c1 * s1 + (c0 * s0 + 0)).
inline V4 Row3N26(const V4& c0, const V4& c1, const V4& c2, f32 s0, f32 s1, f32 s2) {
    V4 o;
    for (u32 k = 0; k < 4; ++k) o.e[k] = VmaddLane26(c2.e[k], s2, VmaddLane26(c1.e[k], s1, VmaddLane26(c0.e[k], s0, 0.0f)));
    return o;
}
// vaddfp a + b with a NaN a (else b) quieted.
inline V4 Add4N26(const V4& a, const V4& b) {
    V4 o;
    for (u32 k = 0; k < 4; ++k) {
        const f32 x = a.e[k], y = b.e[k];
        o.e[k] = x != x ? std::bit_cast<f32>(std::bit_cast<u32>(x) | 0x00400000u)
                 : y != y ? std::bit_cast<f32>(std::bit_cast<u32>(y) | 0x00400000u)
                          : x + y;
    }
    return o;
}

}  // namespace big26
}  // namespace sly4
