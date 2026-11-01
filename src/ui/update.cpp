// Ui_Update: per-frame focus handling, button events and element updates.
#include "ui.h"

using namespace sly4;
using namespace sly4::ui;

namespace {

// Builds kUiNavDirs on first use: index = direction bits (1 up, 2 down,
// 4 left, 8 right); impossible combinations stay (0, 0).
void Ui_InitNavDirs() {
    if (ref<u8>(kUiNavDirsGuard) != 0) return;
    const f32 d = std::bit_cast<f32>(0x3F34FDF4u);  // 0.70707..., as in the original
    const f32 table[16][2] = {
        {0, 0},  {0, -1}, {0, 1},  {0, 0},  {-1, 0}, {-d, -d}, {-d, d}, {0, 0},
        {1, 0},  {d, -d}, {d, d},  {0, 0},  {0, 0},  {0, 0},   {0, 0},  {0, 0},
    };
    for (u32 k = 0; k < 16; ++k) {
        store<f32>(kUiNavDirs + k * 8, table[k][0]);
        store<f32>(kUiNavDirs + k * 8 + 4, table[k][1]);
    }
    ref<u8>(kUiNavDirsGuard) = 1;
}

// Moves the focus by direction: every root is searched (Ui_FindInDirection)
// and the last hit wins. Sends kUiEventNavigated for a hit.
u32 Ui_Navigate(PPUContext& ctx, u32 user, u32 input, u32 dir, u32 best) {
    const auto& in = ref<UiInput>(input);
    const u32 buttons = in.buttons;
    const u32 nav = load<u32>(kUiNavButtons);
    if ((buttons & in.pressed & nav) == 0) return 0;
    Ui_InitNavDirs();
    // Not masked to four bits: other navigation bits index past the table, as in the original.
    const u32 entry = kUiNavDirs + (((buttons & nav) << 3) & ~7u);
    store<u32>(dir, load<u32>(entry));  // lfs/stfs pairs: copied as raw words
    store<u32>(dir + 4, load<u32>(entry + 4));
    store<u32>(best, kFltMaxBits);
    u32 element = load<u32>(kUiRoots);
    if (element == 0) return 0;

    const u32 slot = kUiFocus + user * 4;
    u32 found = 0;
    while (element != 0) {
        const u32 hit = static_cast<u32>(
            call(ctx, ufn::Ui_FindInDirection, {element, user, load<u32>(slot), dir, best}));
        element = load<u32>(element + kElemNext);
        if (hit != 0) found = hit;
    }
    if (found != 0) Ui_Notify(ctx, kUiEventNavigated, user, found, load<u32>(slot));
    return found;
}

// Picks the element under the pointer: the first root whose search hits.
u32 Ui_Point(PPUContext& ctx, u32 user, u32 input, u32 point, u32 best) {
    const auto& in = ref<UiInput>(input);
    const f32 width = static_cast<f32>(field<u16>(g::config, 4).get());
    const f32 height = static_cast<f32>(field<u16>(g::config, 6).get());
    store<u32>(best, kFltMaxBits);
    store<f32>(point, fmuls(fmuls(fadds(in.x, 1.0f), width), 0.5f));
    store<f32>(point + 4, fmuls(fmuls(fadds(in.y, 1.0f), height), 0.5f));

    u32 found = 0;
    for (u32 element = load<u32>(kUiRoots); element != 0 && found == 0;) {
        const u32 hit = static_cast<u32>(call(ctx, ufn::Ui_FindAtPoint, {element, user, point, best}));
        element = load<u32>(element + kElemNext);
        if (hit != 0) found = hit;
    }
    return found;
}

}  // namespace

// void Ui_Update(f32 dt)
//
// For each of the four users the input handler reports buttons and pointer.
// The focus then moves by direction (when navigation is on and a navigation
// button went down) or to the element under the pointer; otherwise it is
// cleared. Focus changes send kUiEventUnfocus / kUiEventFocus, and pressed
// buttons send events 5-11 for the focused element. Finally the timelines
// run (finished auto-delete ones are deleted) and every root element updates.
DECOMP(004D4538, Ui_Update) {
    const f64 dt = farg(ctx, 0);
    Stack frame(ctx, sizeof(UiInput) + 24);
    const u32 input = frame.addr();
    const u32 dir = input + sizeof(UiInput);  // f32[2]
    const u32 point = dir + 8;                 // f32[2]
    const u32 best_dir = point + 8;            // f32
    const u32 best_point = best_dir + 4;       // f32
    auto& in = ref<UiInput>(input);

    for (u32 user = 0; user < kUiUsers; ++user) {
        in.x = 0.0f;
        in.y = 0.0f;
        in.arg = load<u32>(kUiInputArg);
        in.buttons = 0;
        in.pressed = 0;
        in.flags = 0;
        const u32 handler = load<u32>(kUiInputHandler);
        if (handler == 0) continue;
        if (static_cast<u32>(call_ptr(ctx, handler, {user, input})) == 0) continue;

        u32 found = 0;
        if (ref<u8>(kUiNavEnabled) != 0) found = Ui_Navigate(ctx, user, input, dir, best_dir);
        if (found == 0 && (in.flags & kInputPointerValid) != 0) found = Ui_Point(ctx, user, input, point, best_point);

        const u32 slot = kUiFocus + user * 4;
        const u32 old = load<u32>(slot);
        if (old != found) {
            if (old != 0) Ui_Notify(ctx, kUiEventUnfocus, user, old, 0);
            if (found != 0) Ui_Notify(ctx, kUiEventFocus, user, found, 0);
            store<u32>(slot, found);
        }

        auto down = [&] { return in.buttons & in.pressed; };
        if (down() & 0x80) Ui_Notify(ctx, kUiEventButton80, user, found, 0);
        if (down() & 0x40) Ui_Notify(ctx, kUiEventButton40, user, found, 0);
        if (down() & 0x10) Ui_Notify(ctx, kUiEventButton10, user, found, 0);
        if (down() & 0x20) Ui_Notify(ctx, kUiEventButton20, user, found, 0);
        if (down() & 0xC)
            Ui_Notify(ctx, (in.buttons & 0x8) ? kUiEventButton8 : kUiEventButton4, user, found, 0);
        if (down() & 0x4000) Ui_Notify(ctx, kUiEventButton4000, user, found, 0);
    }

    for (u32 tl = load<u32>(kUiTimelines); tl != 0;) {
        const u32 next = load<u32>(tl + kTlNext);
        ctx.fpr[1] = dt;
        if (static_cast<u32>(call(ctx, ufn::Ui_UpdateTimeline, {tl})) == 0 && ref<u8>(tl + kTlAutoDelete) != 0)
            call_ptr(ctx, vfunc(tl, kVfnDelete), {tl});
        tl = next;
    }
    for (u32 element = load<u32>(kUiRoots); element != 0; element = load<u32>(element + kElemNext)) {
        ctx.fpr[1] = dt;
        call_ptr(ctx, vfunc(element, kElemVfnUpdate), {element});
    }
}
