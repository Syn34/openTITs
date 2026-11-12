// Ui_Update: per-frame focus handling, button events and element updates.
#include "ui.h"

namespace sly4::ui {
using namespace opentit::native;

namespace {

// Builds kUiNavDirs on first use: index = direction bits (1 up, 2 down,
// 4 left, 8 right); impossible combinations stay (0, 0).
void Ui_InitNavDirs() {
    u8& guard = global<u8>(kUiNavDirsGuard);
    if (guard != 0) return;
    const f32 d = std::bit_cast<f32>(0x3F34FDF4u);  // 0.70707..., as in the original
    const f32 table[16][2] = {
        {0, 0},  {0, -1}, {0, 1},  {0, 0},  {-1, 0}, {-d, -d}, {-d, d}, {0, 0},
        {1, 0},  {d, -d}, {d, d},  {0, 0},  {0, 0},  {0, 0},   {0, 0},  {0, 0},
    };
    be_f32* dirs = host<be_f32>(kUiNavDirs);
    for (u32 k = 0; k < 16; ++k) {
        dirs[2 * k] = table[k][0];
        dirs[2 * k + 1] = table[k][1];
    }
    guard = 1;
}

gp<Element>& focus_slot(u32 user) { return host<gp<Element>>(kUiFocus)[user]; }  // 32-bit index wrap not needed: user < 4

// Moves the focus by direction: every root is searched (Ui_FindInDirection)
// and the last hit wins. Sends kUiEventNavigated for a hit.
Element* Ui_Navigate(u32 user, const UiInput* in, be_f32* dir, be_f32* best) {
    const u32 buttons = in->buttons;
    const u32 nav = global<be_u32>(kUiNavButtons);
    if ((buttons & in->pressed & nav) == 0) return nullptr;
    Ui_InitNavDirs();
    // Not masked to four bits: other navigation bits index past the table, as in the original.
    const be_f32* entry = host<be_f32>(kUiNavDirs + (((buttons & nav) << 3) & ~7u));
    dir[0] = entry[0];  // lfs/stfs pairs: copied as raw words
    dir[1] = entry[1];
    *reinterpret_cast<be_u32*>(best) = kFltMaxBits;
    Element* element = global<gp<Element>>(kUiRoots);
    if (!element) return nullptr;
    gp<Element>& slot = focus_slot(user);
    Element* found = nullptr;
    while (element) {
        Element* hit = Ui_FindInDirection(element, user, slot, dir, best);
        element = element->next;
        if (hit) found = hit;
    }
    if (found) Ui_Notify(kUiEventNavigated, user, guest(found), slot.ea);
    return found;
}

// Picks the element under the pointer: the first root whose search hits.
Element* Ui_Point(u32 user, const UiInput* in, be_f32* point, be_f32* best) {
    const f32 width = static_cast<f32>(host<be_u16>(g::config + 4)->get());
    const f32 height = static_cast<f32>(host<be_u16>(g::config + 6)->get());
    *reinterpret_cast<be_u32*>(best) = kFltMaxBits;
    point[0] = fmuls(fmuls(fadds(in->x, 1.0f), width), 0.5f);
    point[1] = fmuls(fmuls(fadds(in->y, 1.0f), height), 0.5f);
    Element* found = nullptr;
    for (Element* element = global<gp<Element>>(kUiRoots); element && !found;) {
        Element* hit = call<Element*>(ufn::Ui_FindAtPoint, element, user, point, best);
        element = element->next;
        if (hit) found = hit;
    }
    return found;
}

}  // namespace

// For each of the four users the input handler reports buttons and pointer.
// The focus then moves by direction (when navigation is on and a navigation
// button went down) or to the element under the pointer; otherwise it is
// cleared. Focus changes send kUiEventUnfocus / kUiEventFocus, and pressed
// buttons send events 5-11 for the focused element. Finally the timelines
// run (finished auto-delete ones are deleted) and every root element updates.
void Ui_Update(f64 dt) {
    GuestFrame frame(sizeof(UiInput) + 24);
    UiInput* in = frame.as<UiInput>(0);
    be_f32* dir = frame.as<be_f32>(sizeof(UiInput));  // [2]
    be_f32* point = dir + 2;                            // [2]
    be_f32* best_dir = point + 2;
    be_f32* best_point = best_dir + 1;
    for (u32 user = 0; user < kUiUsers; ++user) {
        in->x = 0.0f;
        in->y = 0.0f;
        in->arg = global<be_u32>(kUiInputArg).get();
        in->buttons = 0;
        in->pressed = 0;
        in->flags = 0;
        const u32 handler = global<be_u32>(kUiInputHandler);
        if (handler == 0) continue;
        if (call_ptr<u32>(handler, user, in) == 0) continue;
        Element* found = nullptr;
        if (global<u8>(kUiNavEnabled) != 0) found = Ui_Navigate(user, in, dir, best_dir);
        if (!found && (in->flags & kInputPointerValid) != 0) found = Ui_Point(user, in, point, best_point);
        gp<Element>& slot = focus_slot(user);
        Element* old = slot;
        if (old != found) {
            if (old) Ui_Notify(kUiEventUnfocus, user, guest(old), 0);
            if (found) Ui_Notify(kUiEventFocus, user, guest(found), 0);
            slot = found;
        }
        const u32 f = guest(found);
        auto down = [&] { return in->buttons & in->pressed; };
        if (down() & 0x80) Ui_Notify(kUiEventButton80, user, f, 0);
        if (down() & 0x40) Ui_Notify(kUiEventButton40, user, f, 0);
        if (down() & 0x10) Ui_Notify(kUiEventButton10, user, f, 0);
        if (down() & 0x20) Ui_Notify(kUiEventButton20, user, f, 0);
        if (down() & 0xC) Ui_Notify((in->buttons & 0x8) ? kUiEventButton8 : kUiEventButton4, user, f, 0);
        if (down() & 0x4000) Ui_Notify(kUiEventButton4000, user, f, 0);
    }
    for (Timeline* tl = global<gp<Timeline>>(kUiTimelines); tl;) {
        Timeline* next = tl->next;
        if (Ui_UpdateTimeline(tl, dt) == 0 && tl->auto_delete != 0) vcall(tl, kVfnDelete);
        tl = next;
    }
    for (Element* element = global<gp<Element>>(kUiRoots); element; element = element->next)
        vcall(element, kElemVfnUpdate, dt);
}

}  // namespace sly4::ui

NATIVE(004D4538, sly4::ui::Ui_Update)
