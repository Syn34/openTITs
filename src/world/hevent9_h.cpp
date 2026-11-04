// World_HandleEvent callees, part 9h: the camera clip sequence update.
#include <cmath>

#include "hevent9.h"

using namespace sly4;
using namespace sly4::hevent;
using namespace sly4::hevent7;
using namespace sly4::hevent9;

namespace sly4 {
namespace hevent9h {
namespace hfn9 {
constexpr u32 Seq_Next767618 = 0x00767618;       // (u32* out, Seq*, u32 cur) (name?)
constexpr u32 Seq_Update2EE7D0 = 0x002EE7D0;     // (Seq*; f32 dt) (name?)
constexpr u32 Matrix_FromAxisAngle = 0x0056A410; // (f32 out[3][4], const vec3* axis; f32 angle)
constexpr u32 RenderTarget_SetMatrix = 0x005201B0;  // (RenderTarget*, const f32 m[4][4])
}  // namespace hfn9

constexpr u32 kSeqPaused = 0x01068890;   // u8 (0x01068848 + 72) (name?)
// Seq: +8 u32 current index, +12 u32 last index, +16 u32 count, +28 16-byte
// entries {+4 u32 next index, +8 Clip*, +12 u32 (bits 15..30: sub index)},
// +56 f32 roll in degrees.
constexpr u32 kSeqCur = 8, kSeqLast = 12, kSeqCount = 16, kSeqEntries = 28, kSeqRoll = 56;
constexpr u32 kEntNext = 4, kEntClip = 8, kEntSub = 12;
// Clip vtable: +4 release, +8 update (f32), +16 done; Clip +4 f32 speed.
constexpr u32 kVRelease = 4, kVUpdate = 8, kVDone = 16, kClipSpeed = 4;
constexpr u32 kConsts = 0x002EED40;      // f32 1e-5, f32 pi/180, +16 vperm control
constexpr u32 kRollAxis = 0x01164B70;    // vec3 (name?)
constexpr u32 kWorldRenderTarget = 0x128168;
constexpr u32 kRtMatrix = 752;           // RenderTarget +752: 4x3 view matrix (rows at stride 16)
}  // namespace hevent9h
}  // namespace sly4

using namespace sly4::hevent9h;

namespace {
// The (index << 16 | sub) word for entry idx's next entry.
inline u32 next_word(u32 ents, u32 idx) {
    const u32 nx = load<u32>(ents + ((idx << 4) & 0x000FFFF0u) + kEntNext);
    const u32 hi = load<u32>(ents + (idx << 4) + kEntNext) << 16;
    return hi | ((load<u32>(ents + (nx << 4) + kEntSub) >> 15) & 0xFFFF);
}
}  // namespace

// void Seq_Update2EED60(Seq* q, f32 dt) (name?): unless paused, steps the
// clip chain from the current entry (one pass per entry): a done clip
// (vtable +16) gives the next position (0x00767618) and is released; a
// running one is updated with dt * its speed and the position moves to its
// next entry (0 after the last). Then 0x002EE7D0(q, dt); with a roll above
// 1e-5 degrees the world render target's view matrix is rotated by it
// (Matrix_FromAxisAngle, the VMX product as in the original) and set again.
DECOMP(002EED60, Seq_Update2EED60) {
    const u32 q = arg<u32>(ctx, 0);
    const f64 dt = farg(ctx, 0);
    const u32 paused = load<u8>(kSeqPaused);
    if (paused != 0) {
        ctx.gpr[3] = paused;
        return;
    }
    // Locals at the original's r1 offsets (base = r1+112): +128 pos word,
    // +132 out word, +136 {idx, sub}.
    Stack frame(ctx, 144);
    const u32 fb = frame.addr();
    const u32 n = load<u32>(q + kSeqCount);
    if (n != 0) {
        const u32 idx = load<u32>(q + kSeqCur);
        const u32 sub = (load<u32>(load<u32>(q + kSeqEntries) + ((idx << 4) & 0xFFFFFFF0u) + kEntSub) >> 15) & 0xFFFF;
        store<u32>(fb + 136, (idx << 16) | sub);
    } else {
        store<u32>(fb + 136, 0);
    }
    store<u32>(fb + 128, load<u32>(fb + 136));
    for (u32 k = 0; n != 0 && k < n; ++k) {
        const u32 idx = load<u16>(fb + 128);
        const u32 slot = load<u32>(q + kSeqEntries) + ((idx << 4) & 0xFFFFFFF0u) + kEntClip;
        const u32 c = load<u32>(slot);
        if (static_cast<u32>(vcall(ctx, c, kVDone, {c})) != 0) {
            call(ctx, hfn9::Seq_Next767618, {fb + 132, q, load<u32>(fb + 128)});
            store<u32>(fb + 128, load<u32>(fb + 132));
            const u32 c2 = load<u32>(slot);
            if (c2 != 0) {
                vcall(ctx, c2, kVRelease, {c2});
                store<u32>(slot, 0);
            }
            continue;
        }
        const u32 c2 = load<u32>(slot);
        ctx.fpr[1] = fmuls(dt, lfs_f64(c2 + kClipSpeed));
        vcall(ctx, c2, kVUpdate, {c2});
        u32 pos = 0;
        if (idx != load<u32>(q + kSeqLast)) pos = next_word(load<u32>(q + kSeqEntries), idx);
        store<u32>(fb + 128, pos);
    }
    callf(ctx, hfn9::Seq_Update2EE7D0, {q}, {dt});
    const f64 roll = lfs_f64(q + kSeqRoll);
    if (!(std::fabs(roll) > lfs_f64(kConsts))) {
        ctx.gpr[3] = kConsts;  // r3 / f1 as the original leaves them
        ctx.fpr[1] = std::fabs(roll);
        return;
    }

    Stack mf(ctx, 128);  // m at +0 (r1+112), rot at +64 (r1+176), z at +112 (r1+224)
    const u32 m = mf.addr(), rot = m + 64, zv = m + 112;
    const u32 src = load<u32>(load<u32>(kCurrentWorld) + kWorldRenderTarget) + kRtMatrix;
    for (u32 row = 0; row < 4; ++row)
        for (u32 k = 0; k < 12; k += 4) store<u32>(m + 16 * row + k, load<u32>(src + 16 * row + k));
    callf(ctx, hfn9::Matrix_FromAxisAngle, {rot, kRollAxis}, {fmuls(roll, lfs_f64(kConsts + 4))});
    store<u32>(zv + 0, load<u32>(kZeroVec + 0));
    store<u32>(zv + 4, load<u32>(kZeroVec + 4));
    store<u32>(zv + 8, load<u32>(kZeroVec + 8));
    // vperm(z, lvlx(kZeroVec + 8), ctrl): {z0, z1, kZeroVec[2], z's lane 3
    // (stack left unset, as in the original)}.
    f32 tr[4];
    for (u32 k = 0; k < 4; ++k) tr[k] = load<f32>(zv + 4 * k);
    tr[2] = load<f32>(kZeroVec + 8);
    f32 r[3][4], out[4][4];
    for (u32 i = 0; i < 3; ++i)
        for (u32 k = 0; k < 4; ++k) r[i][k] = load<f32>(rot + 16 * i + 4 * k);
    for (u32 row = 0; row < 4; ++row) {
        const f32 x = load<f32>(m + 16 * row + 0), y = load<f32>(m + 16 * row + 4), z = load<f32>(m + 16 * row + 8);
        for (u32 k = 0; k < 4; ++k) {
            f32 v = std::fma(r[0][k], x, 0.0f);  // vmaddfp chain on a +0 vector
            v = std::fma(r[1][k], y, v);
            v = std::fma(r[2][k], z, v);
            out[row][k] = row == 3 ? tr[k] + v : v;
        }
    }
    for (u32 row = 0; row < 4; ++row)
        for (u32 k = 0; k < 4; ++k) store<f32>(m + 16 * row + 4 * k, out[row][k]);
    ctx.fpr[1] = lfs_f64(kZeroVec + 8);  // f1 / f2 as the original's copy of the zero vector leaves them
    ctx.fpr[2] = lfs_f64(kZeroVec + 4);
    call(ctx, hfn9::RenderTarget_SetMatrix, {load<u32>(load<u32>(kCurrentWorld) + kWorldRenderTarget), m});
}
