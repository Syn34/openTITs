// Game code, VMX sweep part 3 (see vmx3.h): UI cursor code.
#include "../sly4.h"
#include "vmx3.h"

using namespace sly4;
using namespace sly4::vmx3;

namespace {
constexpr u32 kPool241500 = 0x002414D8;  // f32 -1, 0, 1, pad, f64 1.0, f64 0.0, f32 0.01
}  // namespace

// f32 f(Ui* o) (called by 002418B0): the cursor's move direction and speed.
// With queued moves (u16 +536 of them, 16-byte {x, y, z} entries at +528):
// +448 = the zero vector plus their sum, then the queue is emptied (+536 =
// 0) and +448 becomes that sum's xz direction (y forced to 0); returns the
// def's (+216) +824. Without: no camera -> -1.0; else the input direction
// (Cursor_ReadInput2411B0) has its length clamped to [0, 1] (+464); above
// 0.01, the camera's rows (+96) take {-x, -y, z} and +448 = the result's xz
// direction (y 0), else +448 = the zero vector. Returns the def's speed
// range (+900 / +904 when +556 > 0, else +820 / +824) lerped by the length.
// The running sum's z is stored at the top of each loop pass, as the
// original does. r3 is left as the constant pool address (no caller reads
// it).
DECOMP(00241500, Cursor_MoveDir_00241500) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 K = kPool241500;
    const u16 n = load<u16>(o + 536);
    if (n != 0) {
        const u32 def = load<u32>(o + 216);
        f32 x = F(kV3Zero3);
        CopyW(o + 448, kV3Zero3);
        f32 y = F(kV3Zero3 + 4);
        const u32 q = load<u32>(o + 528);
        CopyW(o + 452, kV3Zero3 + 4);
        f32 z = F(kV3Zero3 + 8);
        const u32 res = load<u32>(def + 824);
        for (u32 i = 0; i < n; ++i) {
            const u32 e = q + 16 * i;
            store<f32>(o + 456, z);
            x = Adds(x, F(e));
            store<f32>(o + 448, x);
            y = Adds(y, F(e + 4));
            store<f32>(o + 452, y);
            z = Adds(z, F(e + 8));
        }
        (void)y;  // the sum's y is dropped (replaced by the pool's 0)
        const f32 k0 = F(K + 4);
        store<u16>(o + 536, 0);
        const f32 inv = Divs(F(K + 8), Sqrts(Fmadds(z, z, Fmadds(x, x, Muls(k0, k0)))));
        store<f32>(o + 448, Muls(x, inv));
        store<f32>(o + 452, Muls(k0, inv));
        store<f32>(o + 456, Muls(z, inv));
        SetF1Bits(ctx, res);
        return;
    }
    const u32 cam = load<u32>(load<u32>(kV3WorldPtr) + kV3WorldCam);
    if (cam == 0) {
        SetF1Bits(ctx, load<u32>(K));
        return;
    }
    Stack frame(ctx, 176);
    const u32 sp = frame.addr();
    const u32 def = load<u32>(o + 216);
    CopyW(sp + 112, kV3Zero3);
    CopyW(sp + 116, kV3Zero3 + 4);
    CopyW(sp + 120, kV3Zero3 + 8);
    call(ctx, v3fn::Cursor_ReadInput, {o, sp + 112});
    const f32 ix = F(sp + 112), iy = F(sp + 116), iz = F(sp + 120);
    const f32 one = F(K + 8);
    const f32 len = Len3s(ix, iy, iz);
    f32 t = Fsels(Subs(one, len), len, Lfd(K + 16));
    t = Fsels(t, t, Lfd(K + 24));
    const f32 sel = F(o + 556);
    if (t > F(K + 32)) {
        const V4 v = Madd4(Lvx(cam + 128), Fill4(iz), Madd4(Lvx(cam + 112), Fill4(-iy), Madd4(Lvx(cam + 96), Fill4(-ix), Zero4())));
        const f32 k0 = F(K + 4);
        const f32 inv = Divs(one, Sqrts(Fmadds(v.e[2], v.e[2], Fmadds(v.e[0], v.e[0], Muls(k0, k0)))));
        store<f32>(o + 448, Muls(v.e[0], inv));
        store<f32>(o + 452, Muls(k0, inv));
        store<f32>(o + 456, Muls(v.e[2], inv));
    } else {
        CopyW(o + 448, kV3Zero3);
        CopyW(o + 452, kV3Zero3 + 4);
        CopyW(o + 456, kV3Zero3 + 8);
    }
    store<f32>(o + 464, t);
    u32 lo = def + 820;
    if (sel > F(K + 4)) lo = def + 900;
    const f32 a = F(lo), b = F(lo + 4);
    ctx.fpr[1] = Fmadds(t, Subs(b, a), a);
}
