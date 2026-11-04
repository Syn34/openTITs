// World_HandleEvent callees, part 12h: the target reach test used by
// Hud_UpdatePrompts48FCC0.
#include <cmath>

#include "hevent12.h"

using namespace sly4;
using namespace sly4::hevent7;
using namespace sly4::hevent12;

namespace {
// Target def: +8 parent object (+96 Mat44 rows), +32 vec3 position, +48 vec3
// rotation (radians, x y z), +68 s32 kind (> 1: never in reach), +96 f32
// squared reach (0: none), +100 f32 max angle (< 0: none), +104 f32 cone
// half-angle against the viewer's forward (0: none), +120 u32 user mask, +124
// u32 flags (0x02000000: facing side only).
constexpr u32 kTdParent = 8, kTdPos = 32, kTdRot = 48, kTdKind = 68, kTdReach2 = 96, kTdAngle = 100;
constexpr u32 kTdCone = 104, kTdUsers = 120, kTdFlags = 124;
constexpr u32 kTdBackOnly = 0x02000000u;
constexpr u32 kParentMatrix = 96;
constexpr u32 kUserSlot = 416;  // viewer +416: bits 28..30 user index
// Viewer Mat34: +32 forward, +48 position.
constexpr u32 kMForward = 32, kMPos = 48;
constexpr u32 kQuatToMatrix = 0x00553A90;  // Quat_ToMatrix (f32 out[4][4], const f32 q[4], const f32 pos[3])
constexpr u32 kMathCosf = 0x007344E0;      // Math_Cosf (f1)
constexpr u32 kMathSinf = 0x007345F0;      // Math_Sinf (f1)
constexpr u32 kAcosf = 0x005E649C;         // acosf (f1)
constexpr f64 kPi = 3.1415927410125732;    // 0x002B79F0 (f32 pi)
// Stack block at r1+112: +0 quat, +16 Mat44 (row 2 at +48, row 3 at +64).
constexpr u32 kSpQuat = 0, kSpMat = 16, kSpRow2 = 48, kSpRow3 = 64, kSpSize = 80;

f64 cosf_(PPUContext& ctx, f64 x) { return callf(ctx, kMathCosf, {}, {x}); }
f64 sinf_(PPUContext& ctx, f64 x) { return callf(ctx, kMathSinf, {}, {x}); }
f64 fmadds(f64 a, f64 c, f64 b) { return static_cast<f32>(std::fma(a, c, b)); }
f64 fmsubs(f64 a, f64 c, f64 b) { return static_cast<f32>(std::fma(a, c, -b)); }
f64 fsubs(f64 a, f64 b) { return static_cast<f32>(a - b); }
f64 dot3(f64 x, f64 y, f64 z, f64 ax, f64 ay, f64 az) { return fmadds(z, az, fmadds(x, ax, hevent::fmuls(y, ay))); }
f32 lane(u32 row, u32 k) { return load<f32>((row & ~15u) + 4 * k); }
}  // namespace

// bool Target_InReach2B79F8(TargetDef* d, const Mat34* m, Viewer* v) (name?):
// the target (kind <= 1, allowed for v's user) is in reach when, with its
// world matrix (rotation quaternion from +48 at the +32 position, times the
// parent's matrix), the squared distance to m's position is within +96 (if
// set) and, with +100 set, the angle between its forward and the direction
// to m is within +100 (facing-side targets fail when m is in front), and the
// direction is inside the viewer's +104 cone. f1 is left as the original
// leaves it.
DECOMP(002B79F8, Target_InReach2B79F8) {
    const u32 d = arg<u32>(ctx, 0), m = arg<u32>(ctx, 1), v = arg<u32>(ctx, 2);
    if (static_cast<s32>(load<u32>(d + kTdKind)) > 1) return ret(ctx, 0u);
    if (v != 0 && (load<u32>(d + kTdUsers) & (1u << ((load<u32>(v + kUserSlot) >> 28) & 7))) == 0) return ret(ctx, 0u);
    bool in_range = true, in_angle = true;
    f64 f1 = lfs_f64(d + kTdReach2);
    bool test = f1 > 0.0;
    if (!test) {
        f1 = lfs_f64(d + kTdAngle);
        test = f1 >= 0.0;
    }
    if (test) {
        Stack frame(ctx, kSpSize);
        const u32 sp = frame.addr();
        const f64 hy = hevent::fmuls(lfs_f64(d + kTdRot + 4), 0.5);
        const f64 cy = cosf_(ctx, hy);
        const f64 hz = hevent::fmuls(lfs_f64(d + kTdRot + 8), 0.5);
        const f64 cz = cosf_(ctx, hz);
        const f64 hx = hevent::fmuls(lfs_f64(d + kTdRot), 0.5);
        const f64 cx = cosf_(ctx, hx);
        const f64 sy = sinf_(ctx, hy);
        const f64 sz = sinf_(ctx, hz);
        const f64 sx = sinf_(ctx, hx);
        const f64 a = hevent::fmuls(sy, sz), b = hevent::fmuls(cy, cz), c = hevent::fmuls(cz, sy), e = hevent::fmuls(cy, sz);
        store<f32>(sp + kSpQuat + 12, static_cast<f32>(fmsubs(b, cx, hevent::fmuls(a, sx))));
        store<f32>(sp + kSpQuat + 0, static_cast<f32>(fmadds(a, cx, hevent::fmuls(b, sx))));
        store<f32>(sp + kSpQuat + 8, static_cast<f32>(fmsubs(e, cx, hevent::fmuls(c, sx))));
        store<f32>(sp + kSpQuat + 4, static_cast<f32>(fmadds(c, cx, hevent::fmuls(e, sx))));
        call(ctx, kQuatToMatrix, {sp + kSpMat, sp + kSpQuat, d + kTdPos});
        const u32 parent = load<u32>(d + kTdParent);
        const f64 reach2 = lfs_f64(d + kTdReach2);
        f1 = lfs_f64(d + kTdAngle);
        if (parent != 0) {
            // Rows 2 and 3 times the parent's rows (vmaddfp lane by lane, plus
            // its translation for row 3); rows 0 and 1 are not used again.
            const u32 pm = parent + kParentMatrix;
            for (const u32 off : {kSpRow2, kSpRow3}) {
                const u32 row = sp + off;
                const f32 x = load<f32>(row), y = load<f32>(row + 4), z = load<f32>(row + 8);
                f32 o[3];
                for (u32 k = 0; k < 3; ++k) {
                    f32 t = std::fma(lane(pm, k), x, 0.0f);
                    t = std::fma(lane(pm + 16, k), y, t);
                    t = std::fma(lane(pm + 32, k), z, t);
                    o[k] = off == kSpRow3 ? lane(pm + 48, k) + t : t;
                }
                for (u32 k = 0; k < 3; ++k) store<f32>(row + 4 * k, o[k]);
            }
        }
        f64 dx = fsubs(load<f32>(sp + kSpRow3), lfs_f64(m + kMPos));
        f64 dy = fsubs(load<f32>(sp + kSpRow3 + 4), lfs_f64(m + kMPos + 4));
        f64 dz = fsubs(load<f32>(sp + kSpRow3 + 8), lfs_f64(m + kMPos + 8));
        if (reach2 > 0.0) in_range = dot3(dx, dy, dz, dx, dy, dz) <= reach2;
        if (f1 >= 0.0 && in_range) {
            const f64 len = static_cast<f32>(std::sqrt(dot3(dx, dy, dz, dx, dy, dz)));
            f1 = 1.0;
            const f64 inv = static_cast<f32>(1.0 / len);
            dy = hevent::fmuls(inv, dy);
            dx = hevent::fmuls(inv, dx);
            dz = hevent::fmuls(inv, dz);
            const f64 dot = dot3(dx, dy, dz, load<f32>(sp + kSpRow2), load<f32>(sp + kSpRow2 + 4), load<f32>(sp + kSpRow2 + 8));
            if (!(dot < 0.0) && (load<u32>(d + kTdFlags) & kTdBackOnly) != 0) {
                in_angle = false;
            } else {
                f64 x = dot < 1.0 ? dot : 1.0;
                if (!(x > -1.0)) x = -1.0;
                const f64 y = static_cast<f32>(static_cast<f32>(x + 1.0) >= 0.0 ? x : -1.0);
                const f64 z = static_cast<f32>(static_cast<f32>(y - 1.0) >= 0.0 ? 1.0 : y);
                const f64 ac = callf(ctx, kAcosf, {}, {z});
                f1 = fsubs(kPi, ac);
                in_angle = f1 <= lfs_f64(d + kTdAngle);
                if (in_angle) {
                    f1 = lfs_f64(d + kTdCone);
                    if (f1 > 0.0) {
                        const f64 fwd = dot3(dx, dy, dz, lfs_f64(m + kMForward), lfs_f64(m + kMForward + 4), lfs_f64(m + kMForward + 8));
                        f1 = cosf_(ctx, f1);
                        if (fwd < f1) in_angle = false;
                    }
                }
            }
        }
    }
    ctx.fpr[1] = f1;
    ret(ctx, in_range && in_angle ? 1u : 0u);
}
