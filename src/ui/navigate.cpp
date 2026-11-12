// Ui_FindInDirection: the directional focus search Ui_Update runs.
#include "ui.h"

#include "../math/math.h"

namespace sly4::ui {
using namespace opentit::native;

namespace {

// -1, 0 or 1 by which side of [-1e-5, 1e-5] v is on.
s32 Ui_Sign(f32 v) { return static_cast<s32>(v > kEpsilon) - static_cast<s32>(v < kMinusEpsilon); }

f32 config_u16(u32 off) { return static_cast<f32>(host<be_u16>(g::config + off)->get()); }

// Scores `element` as the next focus from `focus` in direction `dir` and
// takes it if it beats *best (and the event handler, if any, agrees).
//
// The offset is from the focused element's position to this one's (both
// from vfunc +160, floats 12 and 13 of the matrix it fills), or the fixed
// kUiNavPoint when nothing is focused. An element behind the focus is
// first moved one screen (config width / height) the way `dir` points, so
// the search wraps around; it must then lie ahead (projection > 0). The
// score is angle / pi + distance / screen height.
bool Ui_TryCandidate(Element* element, u32 user, Element* focus, const be_f32* dir, be_f32* best) {
    const be_f32* nav = host<be_f32>(kUiNavPoint);
    f32 dx = nav[0];
    f32 dy = nav[1];
    f32 proj = 0.0f;
    if (focus) {
        // Two 64-byte matrices and two 8-byte outputs, laid out as in the
        // original frame (plus slack where the original had its event block).
        GuestFrame frame(64 + 64 + 8 + 8 + 32);
        be_f32* from = frame.as<be_f32>(0);
        be_f32* to = frame.as<be_f32>(64);
        u8* out_a = frame.as<u8>(128);
        u8* out_b = frame.as<u8>(136);
        vcall(focus, kElemVfnTransform, out_a, out_b, from);
        vcall(element, kElemVfnTransform, out_a, out_b, to);
        const f32 fx = from[12], fy = from[13];
        const f32 ex = to[12], ey = to[13];
        dy = fsubs(ey, fy);
        dx = fsubs(ex, fx);
        const f32 dir_y = dir[1];
        proj = fmadds(dx, dir[0], fmuls(dy, dir_y));
        if (proj < 0.0f) {
            const f32 sx = static_cast<f32>(Ui_Sign(dir[0]));
            const f32 sy = static_cast<f32>(Ui_Sign(dir_y));
            const f32 width = config_u16(4);
            const f32 height = config_u16(6);
            dx = fsubs(fmadds(sx, width, ex), fx);
            dy = fsubs(fmadds(sy, height, ey), fy);
            proj = fmadds(dx, dir[0], fmuls(dy, dir_y));
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
        const f64 angle = math::Acosf(static_cast<f32>(c));
        const f32 height = config_u16(6);
        score = fadds(fdivs(angle, kPi), fdivs(dist, height));
    } else {
        score = fadds(0.0f, dist);
    }
    if (!(score < *best)) return false;
    if (global<be_u32>(kUiEventHandler) != 0 && Ui_Notify(kUiEventCanFocus, user, guest(element), 0) == 0) return false;
    *best = score;
    return true;
}

}  // namespace

// Searches e and its children (depth first) for the best element to move
// the focus to. Only active elements are searched; e itself is a candidate
// when it's focusable and not the focus. The last hit found wins (a child
// over its parent, a later sibling over an earlier one), which is the best
// scoring since each hit lowers *best.
Element* Ui_FindInDirection(Element* element, u32 user, Element* focus, const be_f32* dir, be_f32* best) {
    const u32 flags = element->flags;
    if ((flags & kElemActive) == 0) return nullptr;
    Element* found = nullptr;
    if ((flags & kElemFocusable) && element != focus && Ui_TryCandidate(element, user, focus, dir, best)) found = element;
    for (Element* child = element->first_child; child;) {
        Element* hit = Ui_FindInDirection(child, user, focus, dir, best);
        child = child->next;
        if (hit) found = hit;
    }
    return found;
}

}  // namespace sly4::ui

NATIVE(004D4178, sly4::ui::Ui_FindInDirection)
