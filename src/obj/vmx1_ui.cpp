// Game code, VMX sweep part 1 (see vmx1.h): UI widget hit tests (the
// point taken into the widget's space with the inverse of its matrix).
#include "../sly4.h"
#include "vmx1.h"

using namespace sly4;
using namespace sly4::vmx1;

namespace {
constexpr u32 kWidgetFlags = 136;      // u32: bit 31 has children to test first, bit 29 skip self
constexpr u32 kWidgetFirstChild = 152;
constexpr u32 kWidgetNext = 156;
constexpr u32 kVfnHitTest = 176;       // Widget* (Widget*, const f32 pt[2])
constexpr u32 kVfnGetFrame = 160;      // (Widget*, f32 min[2], f32 max[2], f32 m[4][4])

// The widget's own test: its frame (min, max, matrix) from vfn[40]; the
// point (z = 0) taken through the inverse of the matrix (rotation columns
// built with vmrghw / vmrglw / vperm from the mask pool at `masks`,
// translation -t) and checked against [min, max] in x then y. f1 is left
// holding the last coordinate compared, as the original.
bool WidgetHitSelf(PPUContext& ctx, u32 w, u32 pt, u32 masks) {
    Stack frame(ctx, 176);
    const u32 sp = frame.addr() - 112;  // the original's sp (r1 - 288): +112 point, +128 matrix, +192 min, +200 max
    const f32 px = Lvlx(pt).e[0], py = Lvlx(pt + 4).e[0];
    store<u32>(sp + 112, load<u32>(pt));
    store<u32>(sp + 116, load<u32>(pt + 4));
    store<u32>(sp + 120, 0);
    VCall(ctx, w, kVfnGetFrame, {w, sp + 192, sp + 200, sp + 128});
    const V4 r0 = Lvx(sp + 128), r1 = Lvx(sp + 144), r2 = Lvx(sp + 160);
    const V4 nt = Sub4(V4{{0.0f, 0.0f, 0.0f, 0.0f}}, Lvx(sp + 176));
    const V4 hi = Mrghw(r0, r2);
    const V4 col0 = Mrghw(hi, r1);
    const V4 col1 = Perm(hi, r1, Lvx(masks));
    const V4 col2 = Perm(Mrglw(r0, r2), r1, Lvx(masks + 16));
    V4 res;
    for (u32 k = 0; k < 4; ++k) {
        const f32 a = Vmadd(col2.e[k], 0.0f, Vmadd(col1.e[k], py, Vmadd(col0.e[k], px, 0.0f)));
        const f32 b = Vmadd(col2.e[k], nt.e[2], Vmadd(col1.e[k], nt.e[1], Vmadd(col0.e[k], nt.e[0], 0.0f)));
        res.e[k] = b + a;
    }
    Stvx(sp + 112, res);
    const f32 x = F(sp + 112);
    ctx.fpr[1] = x;
    if (!(x >= F(sp + 192)) || !(x <= F(sp + 200))) return false;
    const f32 y = F(sp + 116);
    ctx.fpr[1] = y;
    return y >= F(sp + 196) && y <= F(sp + 204);
}
}  // namespace

// Widget* f(Widget* w, const f32 pt[2]) (a callback): the widget under pt.
// With flag bit 31 every child (+152, then +156) is asked (vfn[44]); the
// last non-null answer wins. Without a hit, and unless flag bit 29 is set,
// w itself when pt is inside it. r3 = the hit or 0.
DECOMP(004E4FB0, Widget_HitTest_004E4FB0) {
    const u32 w = arg<u32>(ctx, 0);
    const u32 pt = arg<u32>(ctx, 1);
    u32 hit = 0;
    if ((load<u32>(w + kWidgetFlags) >> 31) != 0)
        for (u32 c = load<u32>(w + kWidgetFirstChild); c != 0;) {
            const u32 r = static_cast<u32>(VCall(ctx, c, kVfnHitTest, {c, pt}));
            c = load<u32>(c + kWidgetNext);
            if (r != 0) hit = r;
        }
    if (((load<u32>(w + kWidgetFlags) >> 29) & 1) == 0 && hit == 0 && WidgetHitSelf(ctx, w, pt, 0x004E4F90)) hit = w;
    ret(ctx, hit);
}

// Widget* f(Widget* w, const f32 pt[2]) (a callback): the widget under pt
// for widgets that are only hit when flag bit 31 is set and the byte +135
// is non-zero (else 0): the children (+152 / +156, vfn[44], last non-null
// answer) first; without one, w itself when pt is inside it.
DECOMP(004E1140, Widget_HitTestEnabled_004E1140) {
    const u32 w = arg<u32>(ctx, 0);
    const u32 pt = arg<u32>(ctx, 1);
    u32 hit = 0;
    if ((load<u32>(w + kWidgetFlags) >> 31) != 0 && load<u8>(w + 135) != 0) {
        for (u32 c = load<u32>(w + kWidgetFirstChild); c != 0;) {
            const u32 r = static_cast<u32>(VCall(ctx, c, kVfnHitTest, {c, pt}));
            c = load<u32>(c + kWidgetNext);
            if (r != 0) hit = r;
        }
        if (hit == 0 && WidgetHitSelf(ctx, w, pt, 0x004E1120)) hit = w;
    }
    ret(ctx, hit);
}
