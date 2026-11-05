// World_HandleEvent callees, part 19f: the animator's look-at base record
// (Anim_Base3525A0), with its VMX vector normalisation.
#include <bit>
#include <cmath>

#include "opentit/vmx.h"

#include "hevent19.h"

using namespace sly4;
using namespace sly4::hevent;
using namespace sly4::hevent19;

namespace {
using hevent7::lfs_f64;

constexpr u32 kRig = 8, kRigSkeleton = 28;
// Animator +12 skeleton instance: +16 48-byte bone transforms (+16 the
// rotation), +28 u32 flags per bone (bit 1: transform needs 0x00A571A8);
// +116 Base* (176 bytes, made on first use).
constexpr u32 kAnimSkelInst = 12, kInstPose = 16, kInstFlags = 28, kInstDirty = 2, kTransformSize = 48, kTransformRot = 16;
constexpr u32 kAnimBase = 116, kBaseSize = 176;
// Base: +0 f32 axis[4] (normalised), +16 f32 up[4], +32 rotated axis
// (normalised), +48 / +72 / +76 / +64 / +68 f32 angles (degrees to
// radians), +80 rotated side, +96 v128 0, +112 {0, 0, 0, 1}, +128 {1, 1,
// 1, 1}, +144 f32[3] zero, +160 0, +164 s32 bone, +168 / +172 0.
constexpr u32 kVecW1 = 0x00B2C390, kVecOnes = 0x00B2C1E0, kZeroVec = 0x01164B40;
// Constants at 0x00352590: f32 0.5 (Newton step), pi / 180, 1.
constexpr u32 kConsts = 0x00352590;
constexpr u32 kTransformGet = 0x00A571A8;  // (SkelInst*, s32 bone) -> transform (name?)
constexpr u32 kRotateVec = 0x007D1758;     // (f32 out[4], const f32 quat[4], const f32 v[4]) (name?)

inline v128 vload(u32 ea) {
    v128 v;
    ea &= ~15u;
    for (int i = 0; i < 4; ++i) v.uw(i) = load<u32>(ea + 4 * static_cast<u32>(i));
    return v;
}
inline void vstore(u32 ea, const v128& v) {  // stvx
    ea &= ~15u;
    for (int i = 0; i < 4; ++i) store<u32>(ea + 4 * static_cast<u32>(i), v.uw(i));
}
// (x, y, z, 0) of three words (the splat / vsel / vandc sequence).
inline v128 xyz0(u32 x, u32 y, u32 z) {
    v128 v;
    v.uw(0) = x;
    v.uw(1) = y;
    v.uw(2) = z;
    v.uw(3) = 0;
    return v;
}
inline v128 xyz0(u32 ea) { return xyz0(load<u32>(ea), load<u32>(ea + 4), load<u32>(ea + 8)); }
// v scaled by the refined reciprocal length of its xyz part, as the VMX
// code: squares fused with a zero addend, (x + y) + z, the vrsqrtefp
// estimate and one Newton step (0 when the squared length is <= 0).
inline v128 normalize(const v128& v, f32 half) {
    const v128 z = v128::zero();
    v128 hv;
    for (auto& e : hv.f32_) e = half;
    const v128 sq = vmx::vmaddfp(v, z, v);
    const v128 len2 = vmx::vaddfp(vmx::vaddfp(vmx::vspltw(sq, 0), vmx::vspltw(sq, 1)), vmx::vspltw(sq, 2));
    const v128 e = vmx::vrsqrtefp(len2);
    const v128 h = vmx::vmaddfp(len2, z, hv);
    const v128 nonpos = vmx::vcmpgefp(z, len2);
    const v128 e2 = vmx::vmaddfp(e, z, e);
    const v128 step = vmx::vnmsubfp(h, hv, e2);
    const v128 k = vmx::vandc(vmx::vmaddfp(e, e, step), nonpos);
    return vmx::vmaddfp(v, z, k);
}
inline f32 lfs(u32 ea) { return static_cast<f32>(lfs_f64(ea)); }
}  // namespace

// Base* Anim_Base3525A0(Animator* a, const char* bone, const f32 axis[3],
// const f32 side[3], const f32 up[3]; f32 a0..a4 degrees) (name?): makes
// the animator's look-at base (+116) for bone once: the normalised axis,
// the up vector, the angles in radians, and the axis and the side vector
// (axis x (side x axis) after normalising both) rotated by the bone's
// rotation. A failed allocation still writes through address 0 (as the
// original).
DECOMP(003525A0, Anim_Base3525A0) {
    const u32 a = arg<u32>(ctx, 0), bone = arg<u32>(ctx, 1), axis = arg<u32>(ctx, 2), side = arg<u32>(ctx, 3), up = arg<u32>(ctx, 4);
    const f64 fa[5] = {farg(ctx, 0), farg(ctx, 1), farg(ctx, 2), farg(ctx, 3), farg(ctx, 4)};
    if (const u32 cur = load<u32>(a + kAnimBase); cur != 0) return ret(ctx, cur);
    const u32 rig = load<u32>(a + kRig);
    if (rig == 0) return ret(ctx, 0u);
    const u32 skel = load<u32>(rig + kRigSkeleton);
    if (skel == 0) return ret(ctx, 0u);
    if (static_cast<s32>(call(ctx, h19fn::Rig_FindBoneA59468, {skel, bone, 0})) == -1) return;  // r3 = the lookup's -1
    const u32 blk = static_cast<u32>(call(ctx, hevent7::h7fn::SmallAlloc, {kBaseSize}));
    if (blk != 0) {
        vstore(blk + 16, v128::zero());
        vstore(blk + 96, v128::zero());
        vstore(blk + 112, vload(kVecW1));
        vstore(blk + 128, vload(kVecOnes));
        store<u32>(blk + 144, load<u32>(kZeroVec));
        store<u32>(blk + 148, load<u32>(kZeroVec + 4));
        store<u32>(blk + 160, 0);
        store<u32>(blk + 152, load<u32>(kZeroVec + 8));
        store<u32>(blk + 168, 0);
        store<u32>(blk + 164, 0xFFFFFFFFu);
        store<u32>(blk + 172, 0);
    }
    store<u32>(a + kAnimBase, blk);
    const f32 half = lfs(kConsts);
    vstore(blk, xyz0(axis));
    const u32 b = load<u32>(a + kAnimBase);
    vstore(b, normalize(vload(b), half));
    vstore(b + 16, xyz0(up));
    const f64 rad = lfs_f64(kConsts + 4);
    store<f32>(b + 48, fmuls(fa[0], rad));
    store<f32>(b + 72, fmuls(fa[1], rad));
    store<f32>(b + 76, fmuls(fa[2], rad));
    store<f32>(b + 64, fmuls(fa[3], rad));
    store<f32>(b + 68, fmuls(fa[4], rad));
    const u64 idx = call(ctx, h19fn::Rig_FindBoneA59468, {skel, bone, 0});
    const u32 base = load<u32>(a + kAnimBase);
    store<u32>(base + 164, static_cast<u32>(idx));
    // n0 = axis / |axis|, n1 = side / |side|, c = n0 x n1, d = n0-major cross of c.
    const f64 x0 = lfs_f64(axis), y0 = lfs_f64(axis + 4), z0 = lfs_f64(axis + 8);
    const f64 x1 = lfs_f64(side), y1 = lfs_f64(side + 4), z1 = lfs_f64(side + 8);
    const f64 one = lfs_f64(kConsts + 8);
    f64 l0 = fmuls(y0, y0), l1 = fmuls(y1, y1);
    l0 = static_cast<f32>(std::fma(x0, x0, l0));
    l1 = static_cast<f32>(std::fma(x1, x1, l1));
    l0 = static_cast<f32>(std::fma(z0, z0, l0));
    l1 = static_cast<f32>(std::fma(z1, z1, l1));
    l0 = static_cast<f32>(std::sqrt(l0));
    l1 = static_cast<f32>(std::sqrt(l1));
    const f64 r0 = static_cast<f32>(one / l0), r1 = static_cast<f32>(one / l1);
    const f64 ny0 = fmuls(y0, r0), nx0 = fmuls(x0, r0), nz0 = fmuls(z0, r0);
    const f64 nx1 = fmuls(r1, x1), ny1 = fmuls(r1, y1), nz1 = fmuls(r1, z1);
    const f64 t7 = fmuls(ny0, nx1), t9 = fmuls(nz0, ny1), t8 = fmuls(nx0, nz1);
    const f64 cz = static_cast<f32>(std::fma(nx0, ny1, -t7));
    const f64 cx = static_cast<f32>(std::fma(ny0, nz1, -t9));
    const f64 cy = static_cast<f32>(std::fma(nz0, nx1, -t8));
    const f64 u7 = fmuls(ny0, cz), u8 = fmuls(nz0, cx), u9 = fmuls(nx0, cy);
    const f64 dx = static_cast<f32>(std::fma(nz0, cy, -u7));
    const f64 dy = static_cast<f32>(std::fma(nx0, cz, -u8));
    const f64 dz = static_cast<f32>(std::fma(ny0, cx, -u9));
    ctx.fpr[1] = dz;  // f1 as the original leaves it
    Stack frame(ctx, 32);  // r1 +112: the normalised side, +128: the splat scratch
    const u32 tmp = frame.addr();
    for (u32 k = 0; k < 16; k += 4) store<f32>(tmp + 16 + k, static_cast<f32>(dz));
    vstore(tmp, normalize(xyz0(std::bit_cast<u32>(static_cast<f32>(dx)), std::bit_cast<u32>(static_cast<f32>(dy)),
                               std::bit_cast<u32>(static_cast<f32>(dz))),
                          half));
    auto transform = [&](u32 b2) {
        const u32 inst = load<u32>(a + kAnimSkelInst);
        const s32 i = static_cast<s32>(load<u32>(b2 + 164));
        if ((load<u32>(load<u32>(inst + kInstFlags) + static_cast<u32>(i) * 4) & kInstDirty) == 0)
            return load<u32>(inst + kInstPose) + static_cast<u32>(i) * kTransformSize;
        return static_cast<u32>(call(ctx, kTransformGet, {inst, static_cast<u64>(static_cast<s64>(i))}));
    };
    {
        const u32 t = transform(base);
        call(ctx, kRotateVec, {base + 32, t + kTransformRot, load<u32>(a + kAnimBase)});
    }
    const u32 b3 = load<u32>(a + kAnimBase);
    vstore(b3 + 32, normalize(vload(b3 + 32), half));
    {
        const u32 t = transform(b3);
        call(ctx, kRotateVec, {b3 + 80, t + kTransformRot, tmp});
    }
    const u32 b4 = load<u32>(a + kAnimBase);
    store<u32>(b4 + 172, 0);
    store<u32>(b4 + 168, 0);
    ret(ctx, b4);
}
