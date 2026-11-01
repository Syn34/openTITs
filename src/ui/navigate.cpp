// Ui_FindInDirection: the directional focus search Ui_Update runs.
#include "ui.h"

using namespace sly4;
using namespace sly4::ui;

namespace {

// -1, 0 or 1 by which side of [-1e-5, 1e-5] v is on.
s32 Ui_Sign(f32 v) { return static_cast<s32>(v > kEpsilon) - static_cast<s32>(v < kMinusEpsilon); }

// Scores `element` as the next focus from `focus` in direction `dir` and
// takes it if it beats *best (and the event handler, if any, agrees).
//
// The offset is from the focused element's position to this one's (both
// from vfunc +160, floats 12 and 13 of the matrix it fills), or the fixed
// kUiNavPoint when nothing is focused. An element behind the focus is
// first moved one screen (config width / height) the way `dir` points, so
// the search wraps around; it must then lie ahead (projection > 0). The
// score is angle / pi + distance / screen height.
bool Ui_TryCandidate(PPUContext& ctx, u32 element, u32 user, u32 focus, u32 dir, u32 best) {
    f32 dx = load<f32>(kUiNavPoint);
    f32 dy = load<f32>(kUiNavPoint + 4);
    f32 proj = 0.0f;
    if (focus != 0) {
        // Two 64-byte matrices and two 8-byte outputs, laid out as in the
        // original frame (plus slack where the original had its event block).
        Stack frame(ctx, 64 + 64 + 8 + 8 + 32);
        const u32 from = frame.addr(), to = from + 64, out_a = to + 64, out_b = out_a + 8;
        call_ptr(ctx, vfunc(focus, kElemVfnTransform), {focus, out_a, out_b, from});
        call_ptr(ctx, vfunc(element, kElemVfnTransform), {element, out_a, out_b, to});
        const f32 fx = load<f32>(from + 48), fy = load<f32>(from + 52);
        const f32 ex = load<f32>(to + 48), ey = load<f32>(to + 52);
        dy = fsubs(ey, fy);
        dx = fsubs(ex, fx);
        const f32 dir_y = load<f32>(dir + 4);
        proj = fmadds(dx, load<f32>(dir), fmuls(dy, dir_y));
        if (proj < 0.0f) {
            const f32 sx = static_cast<f32>(Ui_Sign(load<f32>(dir)));
            const f32 sy = static_cast<f32>(Ui_Sign(dir_y));
            const f32 width = static_cast<f32>(field<u16>(g::config, 4).get());
            const f32 height = static_cast<f32>(field<u16>(g::config, 6).get());
            dx = fsubs(fmadds(sx, width, ex), fx);
            dy = fsubs(fmadds(sy, height, ey), fy);
            proj = fmadds(dx, load<f32>(dir), fmuls(dy, dir_y));
        }
        if (!(proj > 0.0f)) return false;
    }

    const f32 dist = fsqrts(fmadds(dx, dx, fmuls(dy, dy)));
    f32 score;
    if (dist > 0.0f) {
        // cos of the angle, clamped to [-1, 1] with fsel (double constants).
        f64 c = fdivs(proj, dist);
        c = static_cast<f32>(fsel(fadds(c, 1.0f), c, -1.0));
        c = static_cast<f32>(fsel(fsubs(c, 1.0f), 1.0, c));
        const f64 angle = callf(ctx, ufn::Math_Acos, {}, {c});
        const f32 height = static_cast<f32>(field<u16>(g::config, 6).get());
        score = fadds(fdivs(angle, kPi), fdivs(dist, height));
    } else {
        score = fadds(0.0f, dist);
    }
    if (!(score < load<f32>(best))) return false;
    if (load<u32>(kUiEventHandler) != 0 && Ui_Notify(ctx, kUiEventCanFocus, user, element, 0) == 0) return false;
    store<f32>(best, score);
    return true;
}

}  // namespace

// Element* Ui_FindInDirection(Element* e, u32 user, Element* focus, const f32 dir[2], f32* best)
// Searches e and its children (depth first) for the best element to move
// the focus to. Only active elements are searched; e itself is a candidate
// when it's focusable and not the focus. The last hit found wins (a child
// over its parent, a later sibling over an earlier one), which is the best
// scoring since each hit lowers *best.
DECOMP(004D4178, Ui_FindInDirection) {
    const u32 element = arg<u32>(ctx, 0), user = arg<u32>(ctx, 1), focus = arg<u32>(ctx, 2);
    const u32 dir = arg<u32>(ctx, 3), best = arg<u32>(ctx, 4);
    const u32 flags = field<u32>(element, kElemFlags);
    if ((flags & kElemActive) == 0) return ret(ctx, 0u);

    u32 found = 0;
    if ((flags & kElemFocusable) && element != focus && Ui_TryCandidate(ctx, element, user, focus, dir, best))
        found = element;
    for (u32 child = field<u32>(element, kElemFirstChild); child != 0;) {
        const u32 hit = static_cast<u32>(call(ctx, ufn::Ui_FindInDirection, {child, user, focus, dir, best}));
        child = field<u32>(child, kElemNext);
        if (hit != 0) found = hit;
    }
    ret(ctx, found);
}
