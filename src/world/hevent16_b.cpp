// World_HandleEvent callees, part 16b: the spline near-search filter and the
// coach's widget refresh.
#include <cmath>

#include "hevent16.h"

using namespace sly4;
using namespace sly4::hevent;
using namespace sly4::hevent16;

namespace {
constexpr u32 kSplineInstance_GetMode = 0x005A8BA8;  // (inst) -> record +94 or 0 (symbols.txt)

// SplineInstance: +164 flags (bit 15 0x8000: usable), +260 parent instance,
// +80 4x4 world matrix (rows; row 3 translation), +208 shape (+48 vec3
// sphere centre, +60 f32 radius).
constexpr u32 kSiFlags = 164, kSiUsable = 0x8000, kSiParent = 260, kSiMatrix = 80, kSiShape = 208;
constexpr u32 kShapeCentre = 48, kShapeRadius = 60;
constexpr u32 kModeSphere = 16;
// Query: +0 vec3 point, +32 f32 max distance.
constexpr u32 kQueryPoint = 0, kQueryMaxDist = 32;

// Coach widget slots, each Widget* (vt +40 show(Widget*, 0, 1)).
constexpr u32 kCoachRowA = 2624, kCoachRowB = 2672, kCoachRowC = 2720, kCoachRows = 12;
constexpr u32 kCoachUsers = 2588, kCoachNumUsers = 9;
constexpr u32 kCoachItems = 2768, kCoachNumItems = 32;
constexpr u32 kCoachExtraA = 2900, kCoachExtraB = 2904, kCoachExtraC = 2896;
constexpr u32 kWidgetVtShow = 40;

f32 lane(u32 ea) { return load<f32>(ea); }
}  // namespace

// u8 Spline_Filter19FB9C(SplineInstance* s, Query* q) (name?): the spline
// near-search filter (OPD 0x00F32FC8). True when s (and its parent +260, if
// any) is usable (+164 bit 15), its shape is a sphere (mode 16) and the
// sphere's world-space surface is within q +32 of the point q.
DECOMP(0019FB9C, Spline_Filter19FB9C) {
    const u32 s = arg<u32>(ctx, 0), q = arg<u32>(ctx, 1);
    const u32 parent = load<u32>(s + kSiParent);
    if (parent != 0 && (load<u32>(parent + kSiFlags) & kSiUsable) == 0) return ret(ctx, 0u);
    if ((load<u32>(s + kSiFlags) & kSiUsable) == 0) return ret(ctx, 0u);
    if (static_cast<s32>(call(ctx, kSplineInstance_GetMode, {s})) != static_cast<s32>(kModeSphere)) return ret(ctx, 0u);
    const u32 shape = load<u32>(s + kSiShape);
    const u32 c = shape + kShapeCentre, m = s + kSiMatrix;
    const f32 x = lane(c), y = lane(c + 4), z = lane(c + 8);
    // vmaddfp chain: row0 * x + 0, + row1 * y, + row2 * z, then vaddfp row3.
    f32 p[3];
    for (u32 k = 0; k < 3; ++k) {
        f32 t = std::fma(lane(m + 4 * k), x, 0.0f);
        t = std::fma(lane(m + 16 + 4 * k), y, t);
        t = std::fma(lane(m + 32 + 4 * k), z, t);
        p[k] = lane(m + 48 + 4 * k) + t;
    }
    const f64 dy = static_cast<f32>(static_cast<f64>(p[1]) - lane(q + kQueryPoint + 4));
    const f64 dx = static_cast<f32>(static_cast<f64>(p[0]) - lane(q + kQueryPoint));
    const f64 dz = static_cast<f32>(static_cast<f64>(p[2]) - lane(q + kQueryPoint + 8));
    f64 d = static_cast<f32>(dy * dy);
    d = static_cast<f32>(std::fma(dx, dx, d));
    d = static_cast<f32>(std::fma(dz, dz, d));
    d = static_cast<f32>(std::sqrt(d));
    d = static_cast<f32>(d - lane(shape + kShapeRadius));
    const f64 maxd = lane(q + kQueryMaxDist);
    // f1..f3 left as the original leaves them (distance, radius, max distance).
    ctx.fpr[1] = d;
    ctx.fpr[2] = hevent7::lfs_f64(shape + kShapeRadius);
    ctx.fpr[3] = hevent7::lfs_f64(q + kQueryMaxDist);
    ret(ctx, d <= maxd ? 1u : 0u);  // NaN: false
}

// void Coach_Refresh2A0F5C(Coach* c) (name?): calls show (vt +40, (w, 0, 1))
// on every widget the coach holds: the three 12-entry rows (interleaved per
// index), the 9 user slots, the 32 item slots, then +2900, +2904, +2896.
DECOMP(002A0F5C, Coach_Refresh2A0F5C) {
    const u32 c = arg<u32>(ctx, 0);
    u64 r3 = 0;
    auto show = [&](u32 slot) {
        const u32 w = load<u32>(slot);
        r3 = w != 0 ? vcall(ctx, w, kWidgetVtShow, {w, 0, 1}) : 0;
    };
    for (u32 k = 0; k < kCoachRows; ++k) {
        show(c + kCoachRowA + 4 * k);
        show(c + kCoachRowB + 4 * k);
        show(c + kCoachRowC + 4 * k);
    }
    for (u32 k = 0; k < kCoachNumUsers; ++k) show(c + kCoachUsers + 4 * k);
    for (u32 k = 0; k < kCoachNumItems; ++k) show(c + kCoachItems + 4 * k);
    show(c + kCoachExtraA);
    show(c + kCoachExtraB);
    // The last slot is loaded into r31, so r3 keeps the previous value when it is empty.
    const u32 w = load<u32>(c + kCoachExtraC);
    if (w != 0) r3 = vcall(ctx, w, kWidgetVtShow, {w, 0, 1});
    ctx.gpr[3] = r3;
}
