// Game code, part 54: helpers shared by obj54*.cpp (the remaining small
// game functions that use VMX, smallest first).
#pragma once

#include "native/vec.h"

namespace sly4::obj {

namespace o54fn {
constexpr u32 Hk_Activate = 0x0091A4C8;    // (hkpRigidBody*)
constexpr u32 Hk_GetBody = 0x009D19C8;     // (hkpCharacterRigidBody*) -> hkpRigidBody*
constexpr u32 Hk_AddEntity = 0x0092F2A8;   // (world, body, 1)
constexpr u32 Force_Apply = 0x004AC210;    // u32 (Force* t)
}  // namespace o54fn

// hkpRigidBody (the parts used here).
struct HkRigidBody {
    pad<8> _0;
    be_u32 world;      // +8: the hkpWorld it is in, or 0
    pad<212> _12;
    be_u32 motion;     // +224: the embedded motion (its vtable; setters at 68 / 88)
    pad<4> _228;
    u8 motion_type;    // +232: 4 / 5 = fixed / keyframed
};
static_assert(offsetof(HkRigidBody, motion) == 224 && offsetof(HkRigidBody, motion_type) == 232);

// The rigid body's vector setter (the compiler's inline sequence): the
// {x, y, z, 0} rows (RowSelW with the three lane masks at k) of the f32
// triples a (and b, when nonzero) on the stack, Hk_Activate(rb), then the
// motion's vfunc `slot`(motion, &ra[, &rb]).
inline void HkSetVecs54(HkRigidBody* rb, const Vec3* a, const Vec3* b, u32 k, u32 slot) {
    GuestFrame fr(32);
    Quad* ra = fr.as<Quad>(0);
    Quad* rv = fr.as<Quad>(16);
    const Masks3 mk = LoadMasks3(k);
    Stvx(ra, RowSelW(&a->x, &a->y, &a->z, mk));
    if (b != nullptr) Stvx(rv, RowSelW(&b->x, &b->y, &b->z, mk));
    call(o54fn::Hk_Activate, rb);
    be_u32* h = &rb->motion;
    if (b != nullptr)
        vcall(h, slot, ra, rv);
    else
        vcall(h, slot, ra);
}

}  // namespace sly4::obj
