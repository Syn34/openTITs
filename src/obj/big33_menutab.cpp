// Game code, large-function sweep part 33: a menu's tab cycled forwards
// (event 10) or backwards (event 11), with the tab sound, the UI events of
// the old and new tab nodes and the two cursors placed on the new node.
#include "../sly4.h"
#include "big33.h"

using namespace sly4;
using namespace sly4::big33;

namespace {
// Offsets in the menu object.
constexpr u32 kTabNode = 0x001285AC;   // u32: the current tab's node
constexpr u32 kTabUiId = 0x001285C0;   // u32: the page's UI id as last sent
constexpr u32 kTab = 0x001285C5;       // u8: the current tab (0..3)
constexpr u32 kTabC6 = 0x001285C6;     // u8, cleared on a tab change
constexpr u32 kTabC7 = 0x001285C7;     // u8, cleared on a tab change
constexpr u32 kTabSound = 0x00128604;  // u32: the tab sound
constexpr u32 kPageStride = 0x20C80;   // pages from +4208 by the u8 +30; u8 page +1 the UI id
constexpr u32 kCursors = 0x010B19E0;   // two cursors of 11968 bytes; +176 the display block, +208 / +212 the NDC position
constexpr u32 kNdcK = 0x0046EA8C;      // .text: f32 2.0, 1.0

constexpr u32 Sound_Play44C474 = 0x0044C474;  // (out, sound, 0 x 9)
constexpr u32 Player_Get = 0x00365C24;
constexpr u32 Ui_SendEvent = 0x004D3DA4;        // (type, a, b, c)
constexpr u32 Display_GetSize = 0x004D4104;     // (f32 out[2])
constexpr u32 Node_Find_0046E7B0 = 0x0046E7B0;  // (menu, node)
constexpr u32 Table10C2AD8_Set = 0x004D5714;    // (u32 i, u32 v)

u32 PageUiId(u32 t, u32 page) { return load<u8>(t + page * kPageStride + 4208 + 1); }
}  // namespace

// void f(Menu* t, const Event* e) (called by f_004734E0 and f_00476C70).
// Only e+0 == 10 (next tab: (tab + 1) % 4) or 11 (previous: tab - 1, 0 wraps
// to 3) with e+12 set does anything (else r3 is left as e+0, or 0). The tab
// sound plays (Sound_Play44C474(out, t+kTabSound, 0, ...)); the tab bytes
// change; an old tab node gets Ui_SendEvent(2, page UI id, node, 0); then
// Ui_SendEvent(16, e+4, the player's +340 or 0, 0) and the node
// Node_Find_0046E7B0(t, e+12 ->+148) becomes the tab node (0 returns). Its
// screen position (vt slot 144, an 8-byte result) scaled to NDC (2 p / size
// - 1, Display_GetSize per cursor) goes to both cursors, each followed by
// kTabUiId = the page UI id, Ui_SendEvent(1, id, node, 0) and
// Table10C2AD8_Set(id, node). r3 is left as the last call's.
DECOMP(0046EA98, Menu_CycleTab_0046EA98) {
    const u32 t = arg<u32>(ctx, 0), e = arg<u32>(ctx, 1);
    const u32 kind = load<u32>(e);
    if (kind != 10 && kind != 11) return ret(ctx, kind);
    if (load<u32>(e + 12) == 0) return ret(ctx, 0u);
    const bool next = kind == 10;
    // sp+144 / +160: the node position (next / previous), +176 / +180 the
    // sound handle, +184 / +192 the display size.
    Stack frame(ctx, 56);
    const u32 s = frame.addr();
    call(ctx, Sound_Play44C474, {s + (next ? 32u : 36u), load<u32>(t + kTabSound), 0, 0, 0, 0, 0, 0, 0, 0, 0});
    if (next) {
        const u32 b = (load<u8>(t + kTab) + 1u) & 0xFF;
        const u32 old = load<u32>(t + kTabNode);
        store<u8>(t + kTabC6, 0);
        store<u8>(t + kTabC7, 0);
        store<u8>(t + kTab, static_cast<u8>(b & 3));
        if (old != 0) call(ctx, Ui_SendEvent, {2u, PageUiId(t, load<u8>(t + 30)), old, 0u});
    } else {
        const u32 page = load<u8>(t + 30);
        const u8 cur = load<u8>(t + kTab);
        store<u8>(t + kTab, static_cast<u8>(cur != 0 ? cur - 1 : 3));
        store<u8>(t + kTabC6, 0);
        const u32 old = load<u32>(t + kTabNode);
        store<u8>(t + kTabC7, 0);
        if (old != 0) call(ctx, Ui_SendEvent, {2u, PageUiId(t, page), old, 0u});
    }
    const u32 who = load<u32>(e + 4);
    u32 pl = 0;
    if (call(ctx, Player_Get) != 0) pl = load<u32>(static_cast<u32>(call(ctx, Player_Get)) + 340);
    call(ctx, Ui_SendEvent, {16u, who, pl, 0u});
    const u32 n = static_cast<u32>(call(ctx, Node_Find_0046E7B0, {t, load<u32>(load<u32>(e + 12) + 148)}));
    store<u32>(t + kTabNode, n);
    if (n == 0) return ret(ctx, 0u);
    const u32 pos = s + (next ? 0u : 16u);
    VCall(ctx, n, 144, {pos, n});
    const f64 two = Lfs(kNdcK), one = Lfs(kNdcK + 4);
    const u32 disp = s + (next ? 40u : 48u);
    for (u32 i = 0; i < 2; ++i) {
        const u32 c = kCursors + 176 + i * 11968;
        call(ctx, Display_GetSize, {disp, c});
        const f64 w = Lfs(disp), h = Lfs(disp + 4);
        const f64 qx = Divs(Muls(Lfs(pos), two), w), qy = Divs(Muls(Lfs(pos + 4), two), h);
        const f64 x = Subs(qx, one), y = Subs(qy, one);
        store<f32>(c + 32, static_cast<f32>(x));
        store<f32>(c + 36, static_cast<f32>(y));
        // f1-f4 as the original leaves them at the next call.
        ctx.fpr[1] = y;
        ctx.fpr[2] = qy;
        ctx.fpr[3] = w;
        ctx.fpr[4] = h;
        const u32 pg = t + load<u8>(t + 30) * kPageStride + 4208;
        const u32 node = load<u32>(t + kTabNode);
        store<u32>(t + kTabUiId, load<u8>(pg + 1));
        call(ctx, Ui_SendEvent, {1u, load<u8>(pg + 1), node, 0u});
        call(ctx, Table10C2AD8_Set, {PageUiId(t, load<u8>(t + 30)), load<u32>(t + kTabNode)});
    }
}
