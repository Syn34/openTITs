// Game code, large-function sweep part 7: a bone controller's source
// matrix (00250AF0).
#include "big7.h"

using namespace sly4;
using namespace sly4::big7;
using namespace sly4::big7::b7fn;

namespace {
// BoneCtl fields.
constexpr u32 kCtlOwner = 8;          // Obj*: +96 its matrix
constexpr u32 kCtlParams = 208;       // 160-byte parameter blocks, one per k
constexpr u32 kCtlSource = 1244;      // default source object
constexpr u32 kCtlPos = 1248;         // f32[3]: fixed position
constexpr u32 kCtlFlags = 1268;       // 0x20000000: the owner's matrix at the fixed position
// Parameter block.
constexpr u32 kPrmLead = 116;         // f32: lead along the source's velocity (x/z)
constexpr u32 kPrmLeadMax = 120;      // f32: its maximum length
constexpr u32 kPrmRayTop = 124;       // f32: ground ray start / end heights
constexpr u32 kPrmRayBottom = 128;
constexpr u32 kPrmFlags = 144;        // 0x40000000: snap to the ground
constexpr u32 kVfnPosition = 284;     // source (out vec3)
constexpr u32 kVfnVelocity = 140;     // source (out vec3)

constexpr u32 kConst = 0x00250AB0u;   // .text: f32 0.0, f32 1.0; lane masks y / z / w at +16 / +32 / +48
constexpr u32 kWorldPtr = 0x00F65CA0u;  // World*: +0x1281D4 -> {+4 hkpWorld*}
constexpr u32 kWorldPhys = 0x001281D4u;
constexpr u32 kCastRay = 0x0092E2E8u;   // hkpWorld::castRay(world, const hkpWorldRayCastInput*, hkpWorldRayCastOutput*) (Havok)
}  // namespace

// bool BoneCtl_SourceMatrix(BoneCtl* ctl, f32 m[4][4], u32 k, Obj* src)
// With flag 0x20000000: the owner's matrix at the fixed position, false.
// Otherwise the source (or the default one) gives its matrix and position;
// parameter block k may lead the position along the source's x/z velocity
// (scaled by +116, clamped to +120) and snap it to the ground with a ray
// from +124 to +128 above it (hit: y = top + (bottom - top) * fraction).
// False without a source.
DECOMP(00250AF0, BoneCtl_SourceMatrix) {
    const u32 ctl = arg<u32>(ctx, 0);
    const u32 m = arg<u32>(ctx, 1);
    const u32 k = arg<u32>(ctx, 2);
    u32 src = arg<u32>(ctx, 3);
    const u32 flags = load<u32>(ctl + kCtlFlags);
    const u32 owner = load<u32>(ctl + kCtlOwner);
    if (src == 0) src = load<u32>(ctl + kCtlSource);
    if ((flags & 0x20000000u) != 0) {
        CopyMat43W(m, owner + 96);
        CopyW(m + 48, ctl + kCtlPos);
        CopyW(m + 52, ctl + kCtlPos + 4);
        const u32 w = load<u32>(ctl + kCtlPos + 8);
        store<u32>(m + 56, w);
        SetF1Bits(ctx, w);
        ret(ctx, 0u);
        return;
    }
    if (src == 0) {
        ret(ctx, 0u);
        return;
    }
    Stack frame(ctx, 176);  // the original's sp+112..+287
    const u32 sp = frame.addr() - 112;
    CopyMat43W(m, src + 96);
    VCall(ctx, src, kVfnPosition, {sp + 112, src});
    const u32 wx = load<u32>(sp + 112), wy = load<u32>(sp + 116), wz = load<u32>(sp + 120);
    store<u32>(m + 48, wx);
    store<u32>(m + 52, wy);
    store<u32>(m + 56, wz);
    f32 x = std::bit_cast<f32>(wx), y = std::bit_cast<f32>(wy), z = std::bit_cast<f32>(wz);
    const u32 prm = ctl + kCtlParams + 160 * k;
    const f32 zero = F(kConst);
    u32 f1w = load<u32>(prm + kPrmLead);  // what f1 holds (as an lfs of this word)
    if (F(prm + kPrmLead) > zero) {
        f1w = load<u32>(prm + kPrmLeadMax);
        if (F(prm + kPrmLeadMax) > zero) {
            VCall(ctx, src, kVfnVelocity, {sp + 128, src});
            const f32 lead = F(prm + kPrmLead);
            f32 dx = F(sp + 128) * lead;
            f32 dz = F(sp + 136) * lead;
            f32 dy = zero;
            const f32 lim = F(prm + kPrmLeadMax);
            const f32 lim2 = lim * lim;
            const f32 px = F(m + 48), py = F(m + 52);
            f1w = load<u32>(m + 56);
            const f32 pz = std::bit_cast<f32>(f1w);
            f32 len2 = Fmadds(dx, dx, dy * dy);
            len2 = Fmadds(dz, dz, len2);
            if (lim2 < len2) {
                const f32 len = static_cast<f32>(std::sqrt(static_cast<f64>(len2)));
                const f32 inv = static_cast<f32>(static_cast<f64>(F(kConst + 4)) / len);
                dx = dx * inv;
                dy = dy * inv;
                dz = dz * inv;
                dx = dx * lim;
                dy = dy * lim;
                dz = dz * lim;
            }
            x = px + dx;
            y = py + dy;
            z = pz + dz;
            store<f32>(m + 48, x);
            store<f32>(m + 52, y);
            store<f32>(m + 56, z);
        }
    }
    SetF1Bits(ctx, f1w);
    if ((load<u32>(prm + kPrmFlags) & 0x40000000u) == 0) {
        ret(ctx, 1u);
        return;
    }
    // Ground snap: hkpWorldRayCastInput at sp+144 (from, to, +32 u8 0,
    // +36 filter 5, +40 0), hkpWorldRayCastOutput at sp+192 (+16 hit
    // fraction 1.0, +20 / +32 -1, +64 / +68 0; +68 the hit body).
    const f32 fx = x + zero, fz = z + zero;
    const f32 top = y + F(prm + kPrmRayTop);
    const f32 bottom = y + F(prm + kPrmRayBottom);
    store<u8>(sp + 176, 0);
    store<u32>(sp + 184, 0);
    const V4 my = Lvx(kConst + 16), mz = Lvx(kConst + 32), mw = Lvx(kConst + 48);
    Stvx(sp + 144, Andc(Sel(Sel(Splat1(fx), Splat1(top), my), Splat1(fz), mz), mw));
    store<u32>(sp + 212, 0xFFFFFFFFu);
    store<u32>(sp + 180, 5);
    store<u32>(sp + 208, 0x3F800000u);
    store<u32>(sp + 256, 0);
    store<u32>(sp + 260, 0);
    Stvx(sp + 160, Andc(Sel(Sel(Splat1(fx), Splat1(bottom), my), Splat1(fz), mz), mw));
    store<u32>(sp + 224, 0xFFFFFFFFu);
    const u32 world = load<u32>(load<u32>(load<u32>(kWorldPtr) + kWorldPhys) + 4);
    ctx.fpr[1] = bottom;
    call(ctx, kCastRay, {world, sp + 144, sp + 192});
    if (load<u32>(sp + 260) != 0) {
        const f32 b = F(prm + kPrmRayBottom), t = F(prm + kPrmRayTop);
        const f32 hy = Fmadds(static_cast<f32>(b - t), F(sp + 208), t) + F(m + 52);
        store<f32>(m + 52, hy);
        ctx.fpr[1] = hy;
    }
    ret(ctx, 1u);
}
