// Game code, VMX sweep part 3 (see vmx3.h): leaf vector / matrix math.
#include "../sly4.h"
#include "vmx3.h"

using namespace sly4;
using namespace sly4::vmx3;

namespace {
constexpr u32 kPerm1D3520 = 0x001D3500;  // two vperm masks: {a2 b1 a3 b0}, {a0 b2 a1 b0}

// The rigid inverse the compiler builds of the rows at m: the transposed
// rotation (c0 from vmrghw, c1 / c2 through the two vperm masks) and
// -t through it.
struct InvRows3 {
    V4 c[3];
    V4 t;
};
inline InvRows3 InvRigid3(u32 m, const V4& p0, const V4& p1) {
    const V4 r0 = Lvx(m), r1 = Lvx(m + 16), r2 = Lvx(m + 32), r3 = Lvx(m + 48);
    const V4 nt = Sub4(Zero4(), r3);
    const V4 h = Mrghw(r0, r2), l = Mrglw(r0, r2);
    InvRows3 o;
    o.c[0] = Mrghw(h, r1);
    o.c[1] = Perm(h, r1, p0);
    o.c[2] = Perm(l, r1, p1);
    o.t = XformDir(o.c[0], o.c[1], o.c[2], nt);
    return o;
}
// The 4x4 rows at d replaced by themselves through inv (rows 0..2 rotated,
// row 3 rotated plus inv.t), stored row 0, 1, 2, 3.
inline void ApplyInv3(u32 d, const InvRows3& inv) {
    const V4 q0 = Lvx(d), q1 = Lvx(d + 16), q2 = Lvx(d + 32), q3 = Lvx(d + 48);
    const V4 n3 = Add4(inv.t, XformDir(inv.c[0], inv.c[1], inv.c[2], q3));
    Stvx(d, XformDir(inv.c[0], inv.c[1], inv.c[2], q0));
    Stvx(d + 16, XformDir(inv.c[0], inv.c[1], inv.c[2], q1));
    Stvx(d + 32, XformDir(inv.c[0], inv.c[1], inv.c[2], q2));
    Stvx(d + 48, n3);
}
inline void Copy43(u32 d, u32 s) {
    for (u32 r = 0; r < 4; ++r)
        for (u32 k = 0; k < 3; ++k) CopyW(d + 16 * r + 4 * k, s + 16 * r + 4 * k);
}
}  // namespace

// void f(T* o, Pose* a, Pose* b, const f32 m[4][4]) (called by 001D3810):
// with a, a's local matrix (+8 ->) = m's 4x3 words taken into o's frame
// (the rigid inverse of o+160 ->+80 applied), copied (4x3 words) to a's
// second matrix (+4 ->); with b too, that one is further taken into b's
// frame (the rigid inverse of b+8 ->). The vector stores write all four
// lanes of each row. f1 / r3 are left as the original leaves them (the
// last word copied; a's +4 matrix).
DECOMP(001D3520, Pose_ToLocal_001D3520) {
    const u32 o = arg<u32>(ctx, 0), a = arg<u32>(ctx, 1), b = arg<u32>(ctx, 2), m = arg<u32>(ctx, 3);
    if (a == 0) return;
    const u32 p = load<u32>(a + 8);
    Copy43(p, m);
    const V4 k0 = Lvx(kPerm1D3520), k1 = Lvx(kPerm1D3520 + 16);
    ApplyInv3(p, InvRigid3(load<u32>(o + 160) + 80, k0, k1));
    const u32 q = load<u32>(a + 4);
    Copy43(q, p);
    SetF1Bits(ctx, load<u32>(p + 56));
    ctx.gpr[3] = q;
    if (b == 0) return;
    ApplyInv3(q, InvRigid3(load<u32>(b + 8), k0, k1));
}
