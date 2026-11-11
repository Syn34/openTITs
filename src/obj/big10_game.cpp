// Game code, large-function sweep part 10: the game-mode object's
// effect-spline rebuild (000533A0, called by Game_SetPhase phases 3 / 8).
#include "big10.h"

using namespace sly4;
using namespace sly4::big10;

namespace {
constexpr u32 kFindBone = 0x005A93D8u;      // Skeleton_FindBone (skeleton, u32 hash) -> index
constexpr u32 kSplineRebuild = 0x0059F110u; // SplineDef_Rebuild (spline, 0)
constexpr u32 kBoneHash = 0x047052BEu;      // the root bone of the effect's chain (name?)
constexpr u32 kSplK = 0x00053370u;          // .text: f32 0 (+0), 0.2 (+4), 3 (+8); vperm masks (+16, +32)
constexpr u32 kZero3 = 0x01164B40u;         // the zero vector (the first segment's start)
constexpr u32 kEffectCur = 1752;            // the current effect object (+96 its 4x3 matrix, +160 its model)
constexpr u32 kSplines = 5084;              // SplineDef*[5] (+116 the 64-byte points, +120 f32 length)
constexpr u32 kPoints = 37;                 // points per spline: bones root+36 down to root
// Lateral offsets of the five splines (f32 -8, -4, 0, 4, 8, built on the stack).
constexpr u32 kOffsets[5] = {0xC1000000u, 0xC0800000u, 0u, 0x40800000u, 0x41000000u};

inline f32 Vmadd(f32 a, f32 c, f32 b) { return static_cast<f32>(std::fma(static_cast<f64>(a), static_cast<f64>(c), static_cast<f64>(b))); }

u32 Model(PPUContext& ctx, u32 o) {
    const u32 m = load<u32>(load<u32>(o + kEffectCur) + 160);
    return static_cast<u32>(VCall(ctx, m, 28, {m}));   // model: skeleton
}
}  // namespace

// void Game_BuildEffectSplines(GameObj* o)
// For each of the five splines at +5084: 37 points along the current
// effect's bone chain (the bone matrix times the effect matrix), each point
// = the transformed row 3 + row 0 * the spline's lateral offset + row 1 *
// 0.2, with +-tangents (row 2 * a third of the distance from the previous
// point); the spline is rebuilt and its +120 set to the summed distances.
DECOMP(000533A0, Game_BuildEffectSplines533A0) {
    const u32 o = arg<u32>(ctx, 0);
    f32 last = 0.0f;
    u32 spl = 0;
    for (u32 j = 0; j < 5; ++j) {
        const u32 mat = load<u32>(o + kEffectCur) + 96;
        f32 R[4][3];
        for (u32 r = 0; r < 4; ++r)
            for (u32 c = 0; c < 3; ++c) R[r][c] = F(mat + 16 * r + 4 * c);
        const u32 bone = static_cast<u32>(call(ctx, kFindBone, {Model(ctx, o), kBoneHash}));
        f32 prev[3] = {F(kZero3), F(kZero3 + 4), F(kZero3 + 8)};
        f32 total = F(kSplK);
        const f32 side = F(kSplK + 4), third = F(kSplK + 8);
        const f32 off = std::bit_cast<f32>(kOffsets[j]);
        u32 bofs = bone << 6;
        for (u32 i = 0; i < kPoints; ++i, bofs -= 64) {
            const u32 b = load<u32>(Model(ctx, o) + 40) + bofs + 2304;
            f32 B[16];
            for (u32 k = 0; k < 16; ++k) B[k] = F(b + 4 * k);
            // The bone rows times the effect rows (vector unit, lanes 0..2).
            f32 C[4][3];
            for (u32 c = 0; c < 3; ++c) {
                for (u32 r = 0; r < 4; ++r) {
                    f32 x = Vmadd(R[0][c], B[4 * r + 0], 0.0f);
                    x = Vmadd(R[1][c], B[4 * r + 1], x);
                    C[r][c] = Vmadd(R[2][c], B[4 * r + 2], x);
                }
                C[3][c] = C[3][c] + R[3][c];
            }
            const u32 p = load<u32>(load<u32>(o + kSplines + 4 * j) + 116) + 64 * i;
            const f32 px = Fmadds(C[1][0], side, Fmadds(C[0][0], off, C[3][0]));
            const f32 py = Fmadds(C[1][1], side, Fmadds(C[0][1], off, C[3][1]));
            const f32 pz = Fmadds(C[1][2], side, Fmadds(C[0][2], off, C[3][2]));
            store<f32>(p + 0, px);
            store<f32>(p + 4, py);
            store<f32>(p + 8, pz);
            f32 d = F(kSplK);
            if (static_cast<s32>(i) > 0) {
                const f32 dy = py - prev[1], dx = px - prev[0], dz = pz - prev[2];
                f32 s = dy * dy;
                s = Fmadds(dx, dx, s);
                s = Fmadds(dz, dz, s);
                d = static_cast<f32>(std::sqrt(static_cast<f64>(s)));
            }
            const f32 t = d / third;
            total = d + total;
            prev[0] = px; prev[1] = py; prev[2] = pz;
            const f32 nt = -t;
            store<f32>(p + 16, C[2][0] * t);
            store<f32>(p + 20, C[2][1] * t);
            store<f32>(p + 24, C[2][2] * t);
            store<f32>(p + 32, C[2][0] * nt);
            store<f32>(p + 36, C[2][1] * nt);
            last = C[2][2] * nt;
            store<f32>(p + 40, last);
        }
        call(ctx, kSplineRebuild, {load<u32>(o + kSplines + 4 * j), 0});
        spl = load<u32>(o + kSplines + 4 * j);
        store<f32>(spl + 120, total);
    }
    // Left in r3 / f1 by the original (the last spline, the last tangent lane).
    ctx.gpr[3] = spl;
    ctx.fpr[1] = last;
}
