// World_HandleEvent callees, part 10k: the camera shake update.
#include <cmath>

#include "hevent10.h"

using namespace sly4;
using namespace sly4::hevent;
using namespace sly4::hevent7;
using namespace sly4::hevent10;

namespace sly4 {
namespace hevent10k {
namespace kfn10 {
constexpr u32 Shake_Pick2EE488 = 0x002EE488;       // (Seq*, s32* index, f32* amplitude) (name?)
constexpr u32 Math_Sinf = 0x007345F0;              // (f32) -> f32
constexpr u32 Matrix_FromAxisAngle = 0x0056A410;   // (Mat33* out, const vec3* axis; f32 angle)
constexpr u32 RenderTarget_SetMatrix = 0x005201B0; // (RenderTarget*, const Mat34* m)
}  // namespace kfn10

// Shake set: +36 96-byte shakes, +44 u16 count. Shake: +0 f32 time, +4 f32
// duration, +24 / +28 / +32 f32 amplitudes (degrees), +36 / +40 / +44 f32
// frequencies (Hz); 0..48, 64..72 and 80 are copied when one is removed.
constexpr u32 kSetShakes = 36, kSetCount = 44, kShakeSize = 96;
constexpr u32 kShTime = 0, kShDuration = 4, kShAmp = 24, kShFreq = 36;
constexpr u32 kRtOffset = 0x128168;      // World +0x128168: RenderTarget* (as hevent9_l.cpp)
constexpr u32 kRtCamera = 752;           // RenderTarget +752: camera matrix rows (16 apart)
constexpr u32 kAxes = 0x01164B50;        // vec3 x / y / z axes, 16 apart (after kZeroVec)
constexpr u32 kConsts = 0x002EE7B0;      // f32 pi / 180, 2 pi (code segment)
// The original's 448-byte frame: the camera rows at 112, the three
// rotations (rows at 176 / 240 / 304, translations at 224 / 288 / 352), the
// picked index and amplitude at 368 / 372. The w lanes the VMX code reads
// are whatever the stack holds there, as in the original.
constexpr u32 kFrame = 448, kM0 = 112, kR1 = 176, kR2 = 240, kR3 = 304, kPick = 368, kAmp = 372;
}  // namespace hevent10k
}  // namespace sly4

using namespace sly4::hevent10k;

namespace {
struct V4 {
    f32 e[4];
};
V4 lvx(u32 ea) {
    V4 v;
    for (u32 k = 0; k < 4; ++k) v.e[k] = load<f32>((ea & ~15u) + 4 * k);
    return v;
}
// rows' = rows * {r0, r1, r2, t}: row i = r2 * row.z + (r1 * row.y + (r0 *
// row.x + 0)) lane by lane (vmaddfp), plus t for row 3 (vaddfp).
void mul(V4 rows[4], const V4& r0, const V4& r1, const V4& r2, const V4& t) {
    for (u32 i = 0; i < 4; ++i) {
        V4 o;
        for (u32 k = 0; k < 4; ++k) {
            f32 x = std::fma(r0.e[k], rows[i].e[0], 0.0f);
            x = std::fma(r1.e[k], rows[i].e[1], x);
            x = std::fma(r2.e[k], rows[i].e[2], x);
            o.e[k] = i == 3 ? t.e[k] + x : x;
        }
        rows[i] = o;
    }
}
}  // namespace

// void Seq_Update2EE7D0(ShakeSet* s, f32 dt) (name?): with shakes running,
// the picked one (0x002EE488 gives its index and amplitude) rotates the
// render target's camera matrix by amp * a_k * sin(2 pi f_k t) degrees about
// x, y and z (RenderTarget_SetMatrix). Then every shake's time advances by
// dt; finished ones are swap-removed. r3 / f1 as the original leaves them.
DECOMP(002EE7D0, Seq_Update2EE7D0) {
    const u32 s = arg<u32>(ctx, 0);
    const f64 dt = farg(ctx, 0);
    Stack frame(ctx, kFrame);
    const u32 b = frame.addr();
    const u32 cam = load<u32>(load<u32>(kCurrentWorld) + kRtOffset) + kRtCamera;
    for (u32 r = 0; r < 4; ++r)
        for (u32 k = 0; k < 3; ++k) store<u32>(b + kM0 + 16 * r + 4 * k, load<u32>(cam + 16 * r + 4 * k));
    f64 f1 = lfs_f64(cam + 56);
    if (load<u16>(s + kSetCount) != 0) {
        store<u32>(b + kPick, 0);
        store<u32>(b + kAmp, 0);
        call(ctx, kfn10::Shake_Pick2EE488, {s, b + kPick, b + kAmp});
        const u32 e = load<u32>(s + kSetShakes) + load<u32>(b + kPick) * kShakeSize;
        const f64 deg = lfs_f64(kConsts), two_pi = lfs_f64(kConsts + 4);
        f64 ang[3];
        for (u32 k = 0; k < 3; ++k) {
            const f32 a = fmuls(lfs_f64(e + kShAmp + 4 * k), deg);
            const f32 x = fmuls(fmuls(lfs_f64(e + kShFreq + 4 * k), lfs_f64(e + kShTime)), two_pi);
            const f64 sn = callf(ctx, kfn10::Math_Sinf, {}, {x});
            ang[k] = fmuls(fmuls(a, sn), lfs_f64(b + kAmp));
        }
        const u32 rot[3] = {kR1, kR2, kR3};
        for (u32 k = 0; k < 3; ++k) {
            const u32 m = b + rot[k];
            callf(ctx, kfn10::Matrix_FromAxisAngle, {m, kAxes + 16 * k}, {ang[k]});
            for (u32 j = 0; j < 3; ++j) store<u32>(m + 48 + 4 * j, load<u32>(kZeroVec + 4 * j));
        }
        f1 = lfs_f64(kZeroVec + 8);
        V4 rows[4];
        for (u32 r = 0; r < 4; ++r) rows[r] = lvx(b + kM0 + 16 * r);
        mul(rows, lvx(b + kR1), lvx(b + kR1 + 16), lvx(b + kR1 + 32), lvx(b + kR1 + 48));
        mul(rows, lvx(b + kR2), lvx(b + kR2 + 16), lvx(b + kR2 + 32), lvx(b + kR2 + 48));
        // The last translation comes from a vperm: x, y and w from the
        // stack copy, z from kZeroVec itself (the same value).
        V4 t3 = lvx(b + kR3 + 48);
        t3.e[2] = load<f32>(kZeroVec + 8);
        mul(rows, lvx(b + kR3), lvx(b + kR3 + 16), lvx(b + kR3 + 32), t3);
        for (u32 r = 0; r < 4; ++r)
            for (u32 k = 0; k < 4; ++k) store<f32>(b + kM0 + 16 * r + 4 * k, rows[r].e[k]);
        call(ctx, kfn10::RenderTarget_SetMatrix, {load<u32>(load<u32>(kCurrentWorld) + kRtOffset), b + kM0});
    }
    u32 off = 0;
    for (s32 i = 0; i < static_cast<s32>(load<u16>(s + kSetCount)); ++i, off += kShakeSize) {
        const u32 t = load<u32>(s + kSetShakes) + off + kShTime;
        store<f32>(t, static_cast<f32>(lfs_f64(t) + dt));
        const u32 data = load<u32>(s + kSetShakes);
        const u32 e = data + off;
        f1 = lfs_f64(e + kShTime);
        if (!(f1 > lfs_f64(e + kShDuration) || f1 == lfs_f64(e + kShDuration))) continue;
        const u32 n = static_cast<u32>(load<u16>(s + kSetCount)) - 1;
        const u32 last = n & 0xFFFF;
        store<u16>(s + kSetCount, static_cast<u16>(n));
        if (i != static_cast<s32>(last)) {
            const u32 src = data + last * kShakeSize;
            for (const u32 k : {0u, 4u, 8u, 12u, 16u, 20u, 24u, 28u, 32u, 36u, 40u, 44u, 48u, 64u, 68u, 72u, 80u})
                store<u32>(e + k, load<u32>(src + k));
            f1 = lfs_f64(src + 80);
        }
        --i;
        off -= kShakeSize;
    }
    ctx.fpr[1] = f1;
    ctx.gpr[3] = off;
}
