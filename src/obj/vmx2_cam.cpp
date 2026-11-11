// Game code, VMX sweep part 2 (see vmx2.h): camera visibility tests that
// place spheres through an object's matrix.
#include "../sly4.h"
#include "vmx2.h"

using namespace sly4;
using namespace sly4::vmx2;

namespace {
constexpr u32 kObjRefPos = 0x0026A42C;      // Obj_RefPos26A42C (f32 out[3], Obj*)
constexpr u32 kObjModePos = 0x0026D8F4;     // Obj_ModePos_0026D8F4 (Vec3* out, Obj*)
constexpr u32 kCamSphereInView = 0x0028A0E8;  // Cam_SphereInView28A0E8 (Cam*, f32 sphere[4]) -> bool
constexpr u32 kTwo = 0x0028A2F8;            // 2.0f
}  // namespace

// bool f(Cam* c, Obj* o) (called by 001D2728 / 0028AC68): whether o is in
// view. Without its vfn[16] shape (o->vfn[16](o) null): o->+160's +240
// sphere tested (false without +160). Else a capsule as two spheres of
// radius r = max(e.x, e.z) (e = Obj_RefPos26A42C extent; e.x only when
// greater) around the shape position q (Obj_ModePos_0026D8F4), each centre
// put through o's matrix (+96 rows, +144 translation): first at height
// q.y + (e.y * 2 - r) (fmsubs), then, when that one is not in view, at
// q.y + r. Cam_SphereInView28A0E8(c, &sphere) decides. The extents are
// whatever the two callees write into the stack. f1 / f2 are left as the
// last sphere's z / y, as the original leaves them for the call.
DECOMP(0028A300, Cam_ObjInView_0028A300) {
    const u32 c = arg<u32>(ctx, 0), o = arg<u32>(ctx, 1);
    ctx.gpr[3] = 0;
    if (o == 0) return;
    const u32 shape = static_cast<u32>(VCall(ctx, o, 64, {o}));
    if (shape == 0) {
        const u32 w = load<u32>(o + 160);
        ctx.gpr[3] = 0;
        if (w != 0) call(ctx, kCamSphereInView, {c, w + 240});
        return;
    }
    Stack frame(ctx, 304);
    const u32 sp = frame.addr(), e = sp + 112, q = sp + 128;
    call(ctx, kObjRefPos, {e, shape});
    call(ctx, kObjModePos, {q, shape});
    const f32 r = F(e) > F(e + 8) ? F(e) : F(e + 8);
    const V4 r0 = Lvx(o + 96), r1 = Lvx(o + 112), r2 = Lvx(o + 128), r3 = Lvx(o + 144);
    // one sphere: centre {q.x, y, q.z} through the matrix, radius r; v at +off, sphere at +off+16
    auto sphere = [&](u32 v, u32 s, f32 y) {
        CopyW(v, q);
        store<f32>(v + 4, y);
        CopyW(v + 8, q + 8);
        Stvx(sp + 208, Fill4(y));
        store<f32>(s + 12, r);
        V4 p = Madd4(r0, SplatW(q), Zero4());
        p = Madd4(r1, Lvx(sp + 208), p);
        p = Madd4(r2, SplatW(q + 8), p);
        Stvx(v, Add4(r3, p));
        for (u32 k = 0; k < 3; ++k) CopyW(s + 4 * k, v + 4 * k);
        ctx.fpr[2] = std::bit_cast<f64>(LfsBits(load<u32>(v + 4)));
        SetF1Bits(ctx, load<u32>(v + 8));
        return static_cast<u32>(call(ctx, kCamSphereInView, {c, s}));
    };
    const f32 y0 = Adds(F(q + 4), Fmsubs(F(e + 4), F(kTwo), r));
    if (sphere(sp + 144, sp + 160, y0) != 0) return;
    sphere(sp + 176, sp + 192, Adds(F(q + 4), r));
}
