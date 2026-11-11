// Game code, VMX sweep part 4 (see vmx4.h): input / control state.
#include "../sly4.h"
#include "vmx4.h"

using namespace sly4;
using namespace sly4::vmx4;

namespace {
constexpr u32 kK27C420 = 0x0027C420;  // f32 {0.0, 1.0}, vperm masks at +16 / +32 / +48 / +64, f32 1e-5 at +80
constexpr u32 kPadTab27C478 = 0x00F64A7C;   // 12-byte entries: +0 the button id
constexpr u32 kMaskTab27C478 = 0x00AD429C;  // 16-byte entries by id: +0 the button mask
}  // namespace

// void f(Ctl* o) (called through a pointer): refreshes a controller state
// from its pad. +32, +48, +64 are cleared (stvx); nothing more while the
// world (0x00F65CA0) has +0x128224 set or o's enable byte +16 is 0. Else,
// with the pad c = 0x010B19E0 + 11968 * o->+80 + 176: stick (x, y) =
// Obj_GetChannel(c, 0 / 1) -> raw +32 = (x, 0, y), magnitude +64 =
// min(|(x, 0, y)|, 1.0); the camera-relative direction +48 = (x, 0, y)
// through the inverse of the camera rows (world +0x128168 ->+752), its
// fourth lane assembled from the zero vector by vperm as the original does,
// then +52 = 0, normalised when the magnitude >= 1e-5; with +88 set all of
// that is zeroed instead. Then the 31 two-bit button fields (+68): bit 0 =
// the button's mask hits c->+8 and its block bit (+84) is clear, bit 1 =
// that and the mask hits c->+12; buttons forced by +24 get 1 | (2 when bit
// 0 was clear), and +24 = 0. f1 / r3 are left as the original leaves them.
DECOMP(0027C478, Ctl_Refresh_0027C478) {
    const u32 o = arg<u32>(ctx, 0);
    Stvx(o + 32, Zero4());
    Stvx(o + 48, Zero4());
    Stvx(o + 64, Zero4());
    const u32 w = load<u32>(kV3WorldPtr);
    const u32 busy = load<u32>(w + 0x00128224u);
    if (busy != 0) {
        ctx.gpr[3] = busy;
        return;
    }
    const u32 en = load<u8>(o + 16);
    if (en == 0) {
        ctx.gpr[3] = 0;
        return;
    }
    const u32 c = 0x010B19E0u + load<u32>(o + 80) * 11968u + 176u;
    const f64 xr = callf(ctx, v4fn::Obj_GetChannel, {c, 0}, {});
    const f64 yr = callf(ctx, v4fn::Obj_GetChannel, {c, 1}, {});
    const f32 x = static_cast<f32>(xr), y = static_cast<f32>(yr);
    const f32 K0 = F(kK27C420), K1 = F(kK27C420 + 4);
    store<f32>(o + 32, x);
    store<u32>(o + 36, 0);
    store<f32>(o + 40, y);
    const u32 held = load<u32>(c + 8), rep = load<u32>(c + 12);
    const f32 len = Sqrts(Fmadds(yr, yr, Fmadds(xr, xr, Muls(K0, K0))));
    const f32 mag = len < K1 ? len : K1;
    store<f32>(o + 64, mag);
    {
        const u32 cam = load<u32>(w + 0x00128168u) + 752;
        const V4 r0 = Lvx(cam), r1 = Lvx(cam + 16), r2 = Lvx(cam + 32), r3 = Lvx(cam + 48);
        const V4 nt = Sub4(Zero4(), r3);
        const V4 h = Mrghw(r0, r2), l = Mrglw(r0, r2);
        const V4 c0 = Mrghw(h, r1), c1 = Perm(h, r1, Lvx(kK27C420 + 16)), c2 = Perm(l, r1, Lvx(kK27C420 + 32));
        const V4 zx = Lvlx(kV3Zero3), zy = Lvlx(kV3Zero3 + 4), zz = Lvlx(kV3Zero3 + 8);
        store<f32>(o + 48, x);
        store<u32>(o + 52, 0);
        store<f32>(o + 56, y);
        const V4 t = Madd4(c2, Splat(nt, 2), Madd4(c1, Splat(nt, 1), Madd4(c0, Splat(nt, 0), Zero4())));
        const V4 d = Madd4(c2, Fill4(y), Madd4(c1, Zero4(), Madd4(c0, Fill4(x), Zero4())));
        const V4 m48 = Lvx(kK27C420 + 48), m64 = Lvx(kK27C420 + 64);
        const V4 tw = Perm(Perm(zx, zy, m48), Perm(zz, t, m48), m64);
        Stvx(o + 48, Add4(tw, d));
        store<u32>(o + 52, 0);
    }
    SetF1Bits(ctx, load<u32>(kK27C420 + 80));
    if (F(o + 64) >= F(kK27C420 + 80)) {
        const f32 dx = F(o + 48), dy = F(o + 52), dz = F(o + 56);
        const f32 s = Divs(K1, Len3s(dx, dy, dz));
        const f32 nx = Muls(dx, s), ny = Muls(dy, s), nz = Muls(dz, s);
        store<f32>(o + 48, nx);
        store<f32>(o + 52, ny);
        store<f32>(o + 56, nz);
        ctx.fpr[1] = ny;
    }
    if (load<u8>(o + 88) != 0) {
        CopyW(o + 32, kV3Zero3);
        CopyW(o + 36, kV3Zero3 + 4);
        store<u32>(o + 64, 0);
        CopyW(o + 40, kV3Zero3 + 8);
        CopyW(o + 48, kV3Zero3);
        CopyW(o + 52, kV3Zero3 + 4);
        CopyW(o + 56, kV3Zero3 + 8);
        SetF1Bits(ctx, load<u32>(kV3Zero3 + 8));
    }
    for (u32 k = 0; k < 31; ++k) {
        const u32 bit = 1u << (k & 31);
        const u32 blocked = load<u32>(o + 84 + ((k >> 3) & 0x1FFFFFFCu)) & bit;
        const u32 mask = load<u32>(kMaskTab27C478 + (load<u32>(kPadTab27C478 + 12 * k) << 4));
        const u32 b0 = ((mask & held) != 0 && blocked == 0) ? 1u : 0u;
        const u32 b1 = (b0 != 0 && (mask & rep) != 0) ? 1u : 0u;
        const u32 sh = (k << 1) & 30u;
        const u32 wa = o + 68 + ((k >> 2) & 0x3FFFFFFCu);
        store<u32>(wa, (load<u32>(wa) & ~(3u << sh)) | ((b0 | (b1 << 1)) << sh));
    }
    for (u32 k = 0; k < 31; ++k) {
        if ((load<u32>(o + 24 + ((k >> 3) & 0x1FFFFFFCu)) & (1u << (k & 31))) == 0) continue;
        const u32 sh = (k << 1) & 30u;
        const u32 wa = o + 68 + ((k >> 2) & 0x3FFFFFFCu);
        const u32 was = load<u32>(wa) & (1u << sh);
        store<u32>(wa, (load<u32>(wa) & ~(3u << sh)) | (((was == 0 ? 2u : 0u) | 1u) << sh));
    }
    store<u32>(o + 24, 0);
    ctx.gpr[3] = 31;
}
