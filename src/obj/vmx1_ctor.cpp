// Game code, VMX sweep part 1 (see vmx1.h): constructors whose only VMX
// use is zeroing a block with vector stores.
#include "../sly4.h"
#include "vmx1.h"

using namespace sly4;
using namespace sly4::vmx1;

namespace {
constexpr u32 kPhysObjCtor = 0x001FA238;     // u32 (Obj* o, const Def* d): the physics object base
constexpr u32 kHkUpdateEntityGroup = 0x00930B70;  // (world list, entity, 0, 1)
constexpr u32 kVtB18F58 = 0x00B18F58;        // the class's vtable block (+8 primary, +416 / +456 secondary)
constexpr u32 kZeroVec = 0x01164B40;         // (0, 0, 0)
constexpr u32 kIdentityQuat = 0x01164C90;    // (0, 0, 0, 1)
constexpr u32 kCtorZero = 0x001FEF34;        // 0.0f
}  // namespace

// u32 f(Obj* o, const Def* d) (called by 001FF16C): constructor on top of
// PhysObj_Ctor_001FA238: vtables (+0, +380, +384), +448 = 0 vector, +464 /
// +480 = identity quaternions, +560..+572 = 0, +576 = 2.0f, +580 = 1.0f,
// +584 / +588 = 0, +440 = -1, +444 = 0, +592 bits 31..27: d->+252 bit 23
// as bit 28, the rest cleared; the 64 bytes at (o+496) & ~15 zeroed (stvx).
// When the template (o->+216) +452 has bit 22 and the body o->+416 a world
// (+8): the body's +44 low half = 29 and Hk_UpdateEntityGroup(world, body,
// 0, 1). With +452 bit 28, an empty +564 takes the level's object
// (*0x00F65CA0)->+4216 (its u16 +178 count incremented). +588 = template
// +424 and +432 bit 6 = (that > 0). r3 = the u16 +432, f1 the +424 value.
DECOMP(001FEF38, PhysObj_Ctor_001FEF38) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 d = arg<u32>(ctx, 1);
    call(ctx, kPhysObjCtor, {o, d, 0x00200000});  // r5: the original's leftover constant-pool high half
    store<u32>(o + 380, kVtB18F58 + 416);
    store<u32>(o + 384, kVtB18F58 + 456);
    for (u32 k = 0; k < 12; k += 4) store<u32>(o + 448 + k, load<u32>(kZeroVec + k));
    store<u32>(o + 464, load<u32>(kIdentityQuat));
    store<u32>(o + 0, kVtB18F58 + 8);
    for (u32 k = 4; k < 16; k += 4) store<u32>(o + 464 + k, load<u32>(kIdentityQuat + k));
    for (u32 k = 0; k < 16; k += 4) store<u32>(o + 480 + k, load<u32>(kIdentityQuat + k));
    u32 bits = load<u32>(o + 592) & 0x1FFFFFFFu;
    store<u32>(o + 560, 0);
    store<u32>(o + 564, 0);
    store<u32>(o + 568, 0);
    store<u32>(o + 572, 0);
    store<u32>(o + 576, 0x40000000u);  // 2.0f
    store<u32>(o + 580, 0x3F800000u);  // 1.0f
    store<u32>(o + 584, 0);
    store<u32>(o + 588, 0);
    store<u32>(o + 592, bits);
    const u32 flags = load<u32>(d + 252);
    store<u32>(o + 440, 0xFFFFFFFFu);
    bits = (bits & ~0x10000000u) | (std::rotl(flags, 5) & 0x10000000u);
    store<u32>(o + 444, 0);
    bits &= ~0x08000000u;
    store<u32>(o + 592, bits);
    for (u32 k = 0; k < 64; k += 4) store<u32>(((o + 496) & ~15u) + k, 0);  // four stvx of a zero vector
    const f32 zero = F(kCtorZero);
    const u32 tmpl = load<u32>(o + 216);
    u32 tf = load<u32>(tmpl + 452);
    if ((tf & 0x00400000u) != 0) {
        const u32 body = load<u32>(o + 416);
        if (body != 0 && load<u32>(body + 8) != 0) {
            const u32 world = load<u32>(body + 8);
            const u32 g = (load<u32>(body + 44) & 0xFFFF0000u) | 29;
            store<u32>(body + 44, g);
            call(ctx, kHkUpdateEntityGroup, {world, body, 0, 1, body, 29, g});
            tf = load<u32>(tmpl + 452);
        }
    }
    if ((tf & 0x10000000u) != 0) {
        const u32 lvl = load<u32>(load<u32>(0x00F65CA0) + 4216);
        if (load<u32>(o + 564) == 0 && lvl != 0) {
            store<u32>(o + 564, lvl);
            store<u16>(lvl + 178, static_cast<u16>(load<u16>(lvl + 178) + 1));
        }
    }
    const u32 w = load<u32>(tmpl + 424);
    store<u32>(o + 588, w);
    SetF1Bits(ctx, w);
    const u32 h = (load<u16>(o + 432) & ~0x40u) | (std::bit_cast<f32>(w) > zero ? 0x40u : 0u);
    store<u16>(o + 432, static_cast<u16>(h));
    ret(ctx, h);
}
