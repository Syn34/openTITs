// Game code, large-function sweep part 33: a menu's tab cycled forwards
// (event 10) or backwards (event 11), with the tab sound, the UI events of
// the old and new tab nodes and the two cursors placed on the new node.
#include "big33.h"

namespace sly4::obj {

struct TabMenu {
    pad<30> _0;
    u8 page;               // +30: selects the 0x20C80-byte page (from +4208; its +1 byte the UI id)
    pad<0x1285AC - 31> _31;
    be_u32 tab_node;       // +0x1285AC: the current tab's node
    pad<16> _1285B0;
    be_u32 tab_ui_id;      // +0x1285C0: the page's UI id as last sent
    pad<1> _1285C4;
    u8 tab;                // +0x1285C5: the current tab (0..3)
    u8 c6;                 // +0x1285C6, cleared on a tab change
    u8 c7;                 // +0x1285C7, cleared on a tab change
    pad<0x128604 - 0x1285C8> _1285C8;
    be_u32 tab_sound;      // +0x128604
};
static_assert(offsetof(TabMenu, tab_node) == 0x1285AC && offsetof(TabMenu, tab_ui_id) == 0x1285C0);
static_assert(offsetof(TabMenu, tab) == 0x1285C5 && offsetof(TabMenu, tab_sound) == 0x128604);

struct TabTarget {
    pad<148> _0;
    be_u32 node;  // +148
};
struct TabEvent {
    be_u32 kind;            // +0: 10 next, 11 previous
    be_u32 who;             // +4
    pad<4> _8;
    gp<TabTarget> target;   // +12
};
// A cursor's display block (two cursors of 11968 bytes from kCursors, the
// block at their +176).
struct CursorDisplay {
    pad<32> _0;
    be_f32 x, y;  // +32, +36: the NDC position
};
struct TabPlayer {
    pad<340> _0;
    be_u32 ui;  // +340
};

namespace {
constexpr u32 kPageStride = 0x20C80;
constexpr u32 kCursors = 0x010B19E0;
constexpr u32 kCursorStride = 11968;
constexpr u32 kNdcK = 0x0046EA8C;  // .text: f32 2.0, 1.0

constexpr u32 Sound_Play44C474 = 0x0044C474;  // (out, sound, 0 x 9)
constexpr u32 Player_Get = 0x00365C24;
constexpr u32 Ui_SendEvent = 0x004D3DA4;        // (type, a, b, c)
constexpr u32 Display_GetSize = 0x004D4104;     // (f32 out[2])
constexpr u32 Node_Find_0046E7B0 = 0x0046E7B0;  // (menu, node)
constexpr u32 Table10C2AD8_Set = 0x004D5714;    // (u32 i, u32 v)

u32 PageUiId(const TabMenu* t, u32 page) { return gm<u8>(guest(t) + page * kPageStride + 4208 + 1); }
}  // namespace

// void f(Menu* t, const Event* e) (called by f_004734E0 and f_00476C70).
// Only kind 10 (next tab: (tab + 1) % 4) or 11 (previous: tab - 1, 0 wraps
// to 3) with e->target set does anything. The tab sound plays
// (Sound_Play44C474(out, t->tab_sound, 0, ...)); the tab bytes change; an
// old tab node gets Ui_SendEvent(2, page UI id, node, 0); then
// Ui_SendEvent(16, e->who, the player's +340 or 0, 0) and the node
// Node_Find_0046E7B0(t, e->target->node) becomes the tab node (0 returns).
// Its screen position (vt slot 144, an 8-byte result) scaled to NDC (2 p /
// size - 1, Display_GetSize per cursor) goes to both cursors, each followed
// by tab_ui_id = the page UI id, Ui_SendEvent(1, id, node, 0) and
// Table10C2AD8_Set(id, node).
void Menu_CycleTab_0046EA98(TabMenu* t, const TabEvent* e) {
    const u32 kind = e->kind;
    if (kind != 10 && kind != 11) return;
    if (!e->target) return;
    const bool next = kind == 10;
    GuestFrame frame(56);
    auto* pos = frame.as<be_f32>(0);   // the node position (2 floats)
    auto* snd = frame.as<be_u32>(32);  // the sound handle
    auto* disp = frame.as<be_f32>(40);  // the display size (2 floats)
    call(Sound_Play44C474, snd, t->tab_sound.get(), 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u);
    if (next) {
        const u32 b = (t->tab + 1u) & 0xFF;
        const u32 old = t->tab_node;
        t->c6 = 0;
        t->c7 = 0;
        t->tab = static_cast<u8>(b & 3);
        if (old != 0) call(Ui_SendEvent, 2u, PageUiId(t, t->page), old, 0u);
    } else {
        const u32 page = t->page;
        const u8 cur = t->tab;
        t->tab = static_cast<u8>(cur != 0 ? cur - 1 : 3);
        t->c6 = 0;
        const u32 old = t->tab_node;
        t->c7 = 0;
        if (old != 0) call(Ui_SendEvent, 2u, PageUiId(t, page), old, 0u);
    }
    const u32 who = e->who;
    u32 pl = 0;
    if (call<u32>(Player_Get) != 0) pl = gm<TabPlayer>(call<u32>(Player_Get)).ui;  // the second result unchecked
    call(Ui_SendEvent, 16u, who, pl, 0u);
    const u32 n = call<u32>(Node_Find_0046E7B0, t, e->target->node.get());
    t->tab_node = n;
    if (n == 0) return;
    call_ptr(vfn(host<u8>(n), 144), pos, n);
    const f64 two = lf(gm<be_f32>(kNdcK)), one = lf(gm<be_f32>(kNdcK + 4));
    for (u32 i = 0; i < 2; ++i) {
        CursorDisplay& c = gm<CursorDisplay>(kCursors + 176 + i * kCursorStride);
        call(Display_GetSize, disp);
        const f64 w = lf(disp[0]), h = lf(disp[1]);
        const f64 qx = Divs(Muls(lf(pos[0]), two), w), qy = Divs(Muls(lf(pos[1]), two), h);
        c.x = static_cast<f32>(Subs(qx, one));
        c.y = static_cast<f32>(Subs(qy, one));
        const u32 id = gm<u8>(guest(t) + t->page * kPageStride + 4208 + 1);
        const u32 node = t->tab_node;
        t->tab_ui_id = id;
        call(Ui_SendEvent, 1u, id, node, 0u);
        call(Table10C2AD8_Set, PageUiId(t, t->page), t->tab_node.get());
    }
}

}  // namespace sly4::obj

NATIVE(0046EA98, sly4::obj::Menu_CycleTab_0046EA98)
