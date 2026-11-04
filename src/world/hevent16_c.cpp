// World_HandleEvent callees, part 16c: the body pose setter under
// Body_SetMode (hevent15_c.cpp).
#include <cmath>

#include "hevent16.h"

using namespace sly4;
using namespace sly4::hevent;
using namespace sly4::hevent16;
using hevent7::lfs_f64;

namespace {
namespace cfn {
constexpr u32 Mat34_FromRotPos553B4C = 0x00553B4C;  // (Mat34* out, const Mat33*, const vec3*) (symbols.txt)
constexpr u32 Quat_FromMatrix = 0x00555478;         // (f32 out[4], const f32 m[4][4]) (symbols.txt)
constexpr u32 Quat_ToMatrix = 0x00553A90;           // (f32 out[4][4], const f32 q[4], const f32 pos[3]) (symbols.txt)
constexpr u32 Hk_GetBody9D19C8 = 0x009D19C8;        // (entity) -> rigid body: Havok range (symbols.txt)
constexpr u32 Hk_SetTransform91EAB8 = 0x0091EAB8;   // (rigid body, const f32 m[4][4]): Havok range (name?)
constexpr u32 Hk_SetAabb924B90 = 0x00924B90;        // (phantom, const vec4 aabb[2]): Havok range (name?)
}  // namespace cfn

// Body: +32 Havok entity, +48 phantom, +80 quat copy (skipped when +232
// bit 25 0x02000000 is set), +464 f32 height offset of the Havok body, +480
// vec3 position, +496 quat, +512 Mat33 rows (16-byte stride), +560 position
// copy, +576 quat copy.
constexpr u32 kBodyEntity = 32, kBodyPhantom = 48, kBodyQuatOut = 80, kBodyFlags232 = 232, kBodyNoQuatOut = 0x02000000;
constexpr u32 kBodyHeight = 464, kBodyPos = 480, kBodyQuat = 496, kBodyRot = 512, kBodyPosCopy = 560, kBodyQuatCopy = 576;
constexpr u32 kZeroVec = 0x01164B40;      // vec3 {0, 0, 0} (as hevent7.h)
constexpr u32 kOne260D30 = 0x00260D30;    // f32 1.0
constexpr u32 kAabbPad = 0x0105C2B0;      // +80 f32 x / z pad, +84 f32 y pad (name?)
constexpr u32 kPadXZ = 80, kPadY = 84;

// Stack: +112 quat, +128 aabb (2 vec4), +160 current matrix (4x4), +224
// Havok matrix (4x4), +288 Quat_ToMatrix result (4x4).
constexpr u32 kFrame = 368, kSpQuat = 112, kSpAabb = 128, kSpCur = 160, kSpHk = 224, kSpMat = 288;

void copy32(u32 dst, u32 src) { store<u32>(dst, load<u32>(src)); }
// Words 0..2 of each of the 4 16-byte rows equal (the subfc / or chain).
bool rows_equal(u32 a, u32 b) {
    for (u32 r = 0; r < 64; r += 16)
        for (u32 c = 0; c < 12; c += 4)
            if (load<u32>(a + r + c) != load<u32>(b + r + c)) return false;
    return true;
}
}  // namespace

// u32 Body_SetPose260D70(Body* b, const f32 m[4][4], u8 update) (name?):
// does nothing (returns 0) when m's 3x4 part equals the body's current pose
// (rotation +512, no translation). Otherwise copies m's position to +480 and
// its normalised quaternion to +496, rebuilds the rotation / position /
// quaternion copies (+512, +560, +576, and +80 unless +232 bit 25), and with
// update and a Havok entity (+32) moves the rigid body (raised by +464 along
// the up row) and the phantom's (+48) box around the position.
DECOMP(00260D70, Body_SetPose260D70) {
    const u32 b = arg<u32>(ctx, 0), m = arg<u32>(ctx, 1);
    const u8 update = arg<u8>(ctx, 2);
    Stack frame(ctx, kFrame);
    const u32 sp = frame.addr();
    call(ctx, cfn::Mat34_FromRotPos553B4C, {sp + kSpCur, b + kBodyRot, kZeroVec});
    if (rows_equal(m, sp + kSpCur)) return ret(ctx, 0u);

    for (u32 k = 0; k < 12; k += 4) copy32(b + kBodyPos + k, m + 48 + k);
    call(ctx, cfn::Quat_FromMatrix, {sp + kSpQuat, m});
    for (u32 k = 0; k < 16; k += 4) copy32(b + kBodyQuat + k, sp + kSpQuat + k);
    // Normalise: q1*q1 first, then q0, q2, q3 fused in.
    f64 n = fmuls(lfs_f64(b + kBodyQuat + 4), lfs_f64(b + kBodyQuat + 4));
    n = static_cast<f32>(std::fma(lfs_f64(b + kBodyQuat), lfs_f64(b + kBodyQuat), n));
    n = static_cast<f32>(std::fma(lfs_f64(b + kBodyQuat + 8), lfs_f64(b + kBodyQuat + 8), n));
    n = static_cast<f32>(std::fma(lfs_f64(b + kBodyQuat + 12), lfs_f64(b + kBodyQuat + 12), n));
    n = static_cast<f32>(std::sqrt(n));
    const f64 inv = static_cast<f32>(lfs_f64(kOne260D30) / n);
    for (u32 k = 0; k < 16; k += 4) store<f32>(b + kBodyQuat + k, fmuls(lfs_f64(sp + kSpQuat + k), inv));
    for (u32 k = 0; k < 16; k += 4) copy32(b + kBodyQuat + k, b + kBodyQuat + k);  // lfs / stfs back (stored twice)
    for (u32 r = 0; r < 48; r += 16)
        for (u32 c = 0; c < 12; c += 4) copy32(b + kBodyRot + r + c, m + r + c);
    for (u32 k = 0; k < 12; k += 4) copy32(b + kBodyPosCopy + k, b + kBodyPos + k);
    for (u32 k = 0; k < 16; k += 4) copy32(b + kBodyQuatCopy + k, b + kBodyQuat + k);
    if ((load<u32>(b + kBodyFlags232) & kBodyNoQuatOut) == 0)
        for (u32 k = 0; k < 16; k += 4) copy32(b + kBodyQuatOut + k, b + kBodyQuat + k);
    ctx.fpr[1] = lfs_f64(b + kBodyQuat + 12);  // f1 left with q3

    const u32 ent = load<u32>(b + kBodyEntity);
    if (update == 0 || ent == 0) return ret(ctx, ent);

    const u32 body = static_cast<u32>(call(ctx, cfn::Hk_GetBody9D19C8, {ent}));
    call(ctx, cfn::Quat_ToMatrix, {sp + kSpMat, b + kBodyQuat, b + kBodyPos});
    // Translation (row 3) raised by +464 along row 1.
    const f64 h = lfs_f64(b + kBodyHeight);
    const u32 mt = sp + kSpMat;
    store<f32>(mt + 48, static_cast<f32>(std::fma(lfs_f64(mt + 16), h, lfs_f64(mt + 48))));
    store<f32>(mt + 52, static_cast<f32>(std::fma(lfs_f64(mt + 20), h, lfs_f64(mt + 52))));
    store<f32>(mt + 56, static_cast<f32>(std::fma(lfs_f64(mt + 24), h, lfs_f64(mt + 56))));
    // Havok matrix: the four rows with w = 0 (vsel / vandc).
    for (u32 r = 0; r < 64; r += 16) {
        for (u32 c = 0; c < 12; c += 4) copy32(sp + kSpHk + r + c, mt + r + c);
        store<u32>(sp + kSpHk + r + 12, 0);
    }
    ctx.fpr[1] = lfs_f64(mt + 56);
    u64 r3 = call(ctx, cfn::Hk_SetTransform91EAB8, {body, sp + kSpHk});

    const u32 ph = load<u32>(b + kBodyPhantom);
    if (ph != 0) {
        const f64 px = lfs_f64(b + kBodyPos), py = lfs_f64(b + kBodyPos + 4), pz = lfs_f64(b + kBodyPos + 8);
        const f64 exz = lfs_f64(kAabbPad + kPadXZ), ey = lfs_f64(kAabbPad + kPadY);
        // The z extent uses the x pad (+80), as in the original.
        const u32 lo = sp + kSpAabb, hi = sp + kSpAabb + 16;
        store<f32>(lo + 0, static_cast<f32>(px - exz));
        store<f32>(lo + 4, static_cast<f32>(py - ey));
        store<f32>(lo + 8, static_cast<f32>(pz - exz));
        store<u32>(lo + 12, 0);
        store<f32>(hi + 0, static_cast<f32>(px + exz));
        store<f32>(hi + 4, static_cast<f32>(py + ey));
        store<f32>(hi + 8, static_cast<f32>(pz + exz));
        store<u32>(hi + 12, 0);
        ctx.fpr[1] = static_cast<f32>(px + exz);
        r3 = call(ctx, cfn::Hk_SetAabb924B90, {ph, lo});
    }
    ctx.gpr[3] = r3;
}
