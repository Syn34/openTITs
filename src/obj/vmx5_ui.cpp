// Game code, VMX sweep part 5 (see vmx5.h): HUD / UI functions.
#include "../sly4.h"
#include "vmx5.h"

using namespace sly4;
using namespace sly4::vmx5;

namespace {
constexpr u32 kK2BB3F0 = 0x002BB3C0;  // f32 1.0, 0.0; vperm masks (+16, +32)
constexpr u32 kCam2BB3F0 = 0x00128168, kView2BB3F0 = 0x0012816C;  // World offsets: camera / view object
}  // namespace

// void f(T* t) (called through a pointer): updates a target pointer. d =
// the zero vector, s = 1.0. When the target t->+44 is set with bit 0x40000
// of its +172: its position (+144..+152) taken through the camera rows
// (World->+0x128168 ->+752) and then the view rows (World->+0x12816C
// ->+816) to u; with B the view translation, d = B + (u - B) * (-B.z /
// (u.z - B.z)) and s = (B.z - u.z) / |(u.x - B.x, u.z - B.z)|; when
// View_ProjectPoint(camera, &out, target+144) gives out.z < 0, d.x / d.y are
// negated. Then 002BB168(t, &d) and 002BAC40(t, &d; s). The w lanes of the
// rows come from the stack. r3 / f1 are left as the original leaves them.
DECOMP(002BB3F0, Hud_TrackTarget_002BB3F0) {
    const u32 t = arg<u32>(ctx, 0);
    Stack frame(ctx, 432);
    const u32 sp = static_cast<u32>(ctx.gpr[1]), d = sp + 112;
    for (u32 k = 0; k < 3; ++k) store<u32>(d + 4 * k, load<u32>(kV3Zero3 + 4 * k));
    f64 s = Lfs4(kK2BB3F0);
    const u32 p = load<u32>(t + 44);
    if (p != 0 && ((load<u32>(p + 172) >> 18) & 1u) != 0) {
        const u32 w = load<u32>(kV3WorldPtr);
        const u32 A = load<u32>(w + kCam2BB3F0) + 752;
        // The rows' x/y/z words at sp+144.. / sp+272.. (w lanes: the stack's).
        CopyMat43W(sp + 144, A);
        const u32 pp = p + 144;
        for (u32 k = 0; k < 3; ++k) store<u32>(sp + 256 + 4 * k, load<u32>(pp + 4 * k));
        const V4 q = Add4(Lvx(sp + 192), Row3(Lvx(sp + 144), Lvx(sp + 160), Lvx(sp + 176), Splat(Lvlx(pp), 0).e[0], Splat(Lvlx(pp + 4), 0).e[0],
                                               Splat(Lvlx(pp + 8), 0).e[0]));
        const u32 B = load<u32>(w + kView2BB3F0) + 816;
        CopyMat43W(sp + 272, B);
        const V4 u = Add4(Lvx(sp + 320), Row3(Lvx(sp + 272), Lvx(sp + 288), Lvx(sp + 304), q.e[0], q.e[1], q.e[2]));
        const f32 b0 = F(B + 48), b1 = F(B + 52), b2 = F(B + 56);
        Stvx(sp + 256, u);
        const f32 dz = Subs(F(sp + 264), b2), dx = Subs(F(sp + 256), b0), dy = Subs(F(sp + 260), b1);
        const f32 k = Divs(-b2, dz);
        store<f32>(d, Fmadds(k, dx, b0));
        store<f32>(d + 4, Fmadds(k, dy, b1));
        store<f32>(d + 8, Fmadds(k, dz, b2));
        const f32 z = F(kK2BB3F0 + 4);
        s = Divs(-dz, Sqrts(Fmadds(dz, dz, Fmadds(dx, dx, Muls(z, z)))));
        call(ctx, v5fn::View_ProjectPoint, {load<u32>(w + kCam2BB3F0), sp + 128, pp});
        if (F(sp + 136) < z) {
            store<u32>(d, load<u32>(d) ^ 0x80000000u);
            store<u32>(d + 4, load<u32>(d + 4) ^ 0x80000000u);
        }
    }
    call(ctx, v5fn::Hud_Track_002BB168, {t, d});
    callf(ctx, v5fn::Hud_Track_002BAC40, {t, d}, {s});
}
