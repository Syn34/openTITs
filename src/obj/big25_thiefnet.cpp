// Game code, large-function sweep part 25: the ThiefNet shop page fill (six
// rows of name / price labels from a scrolling list of upgrades).
#include "big25.h"

namespace sly4::obj {

namespace {
constexpr u32 kItemAt = 0x0049E704;        // Table10BE8BC_At_0049E704 (u32 id) -> item or 0 (+12 price, +16 name text)
constexpr u32 kHashNoCase = 0x0056A710;    // String_HashNoCase (const char*) -> u32
constexpr u32 kRowLabels = 0x00F64DD0;     // const char* [6][2]: each row's name / price label names
constexpr u32 kEmpty = 0x00ADCAA8;         // ""
constexpr u32 kFmtInt = 0x00ADC94C;        // "%d"
constexpr u32 kSold = 0x00ADCBE4;          // "$HIDEOUT_THIEFNET_SOLD"
constexpr u32 kNoUpgrades = 0x00ADCBFC;    // "$THIEFNET_NO_UPGRADES_AVAILABLE"
constexpr u32 kNoUpgradesLabel = 0x001FF691u;
constexpr u32 kHintLabel = 0x968EF501u;

// A menu label record (28 bytes): +0 the name hash, +20 the widget.
struct MenuLabel {
    be_u32 hash;    // +0
    pad<16> _4;
    be_u32 widget;  // +20
    pad<4> _24;
};
static_assert(sizeof(MenuLabel) == 28);
struct ShopMenu {
    pad<184> _0;
    be_u32 labels;  // +184: MenuLabel[]
    be_u32 count;   // +188
};
struct ShopPage {
    pad<8> _0;
    be_u32 menu;     // +8: ShopMenu*
    pad<244> _12;
    be_u32 scroll;   // +256
    be_u32 sel;      // +260
    pad<136> _264;
    be_u32 ids;      // +400: u32[]
    pad<4> _404;
    be_u16 count;    // +408
};
static_assert(offsetof(ShopPage, scroll) == 256 && offsetof(ShopPage, ids) == 400 && offsetof(ShopPage, count) == 408);
// A shop item: +12 its price, +16 its name text.
struct ShopItem {
    pad<12> _0;
    be_s32 price;  // +12
    pad<4> name;   // +16
};

// The menu's label named hash gets vfn[188](text, 0) and then vfn[148]();
// only the first record with the hash counts (count and table re-read).
// Skipped for a null text or hash. Returns whether the widget calls were
// made.
bool SetLabel(u32 menu, u32 hash, u32 text) {
    if (text == 0 || hash == 0) return false;
    const ShopMenu& m = gm<ShopMenu>(menu);
    const u32 n = m.count;
    for (u32 j = 0; j < n; ++j) {
        if (gm<MenuLabel>(m.labels + 28 * j).hash != hash) continue;
        const u32 w = gm<MenuLabel>(m.labels + 28 * j).widget;
        if (w == 0) return false;
        vcall(&gm<u8>(w), 188, text, 0u);
        vcall(&gm<u8>(gm<MenuLabel>(m.labels + 28 * j).widget), 148);
        return true;
    }
    return false;
}

u32 Hash(u32 s) { return call<u32>(kHashNoCase, s); }
}  // namespace

// void f(ShopPage* o) (called by 003780E0, 00378540). o: +8 the menu, +256
// the scroll position, +260 the selected row, +400 the item ids, +408 (u16)
// their count. Row i (0..5) shows item (+256 + count + i - +260) mod count
// when that index is in range or there are at least 6 items: the name label
// gets the item's +16 text and the price label "%d" of its +12, or
// "$HIDEOUT_THIEFNET_SOLD" when bit 2 of the save's byte (world +4028 +
// id) is set; a missing item leaves the row alone; otherwise both labels
// are cleared. With no items the label 0x001FF691 says
// "$THIEFNET_NO_UPGRADES_AVAILABLE" and 0x968EF501 is cleared. No caller
// reads r3 (r3.sh).
void ThiefNet_FillPage_0037744C(ShopPage* o) {
    const u32 world = global<be_u32>(kWorld);
    for (u32 i = 0; i < 6; ++i) {
        const u32 n = o->count;
        const u32 labels = kRowLabels + 8 * i;
        bool show = false;
        u32 idx = 0;
        if (n != 0) {
            const u32 d = i - o->sel;
            const s32 at = static_cast<s32>(o->scroll + d);
            if (!(at < 0 || at >= static_cast<s32>(n)) || n >= 6) {
                idx = (o->scroll + n + d) % n;
                show = idx < n;
                if (!show) continue;
            }
        }
        if (!show) {
            const u32 h0 = Hash(global<be_u32>(labels));
            SetLabel(o->menu, h0, kEmpty);
            const u32 h1 = Hash(global<be_u32>(labels + 4));
            SetLabel(o->menu, h1, kEmpty);
            continue;
        }
        const u32 item = call<u32>(kItemAt, gm<be_u32>(o->ids + 4 * idx).get());
        if (item == 0) continue;
        const u32 h0 = Hash(global<be_u32>(labels));
        SetLabel(o->menu, h0, item + 16);
        const u32 id = gm<be_u32>(o->ids + 4 * idx);
        u32 price;
        if (gm<u8>(world + 4028 + id) & 2) price = kSold;
        else price = call<u32>(fn::String_Format, kFmtInt, gm<ShopItem>(item).price.get());
        const u32 h1 = Hash(global<be_u32>(labels + 4));
        SetLabel(o->menu, h1, price);
    }
    if (o->count != 0) return;
    SetLabel(o->menu, kNoUpgradesLabel, kNoUpgrades);
    SetLabel(o->menu, kHintLabel, kEmpty);
}

}  // namespace sly4::obj

NATIVE(0037744C, sly4::obj::ThiefNet_FillPage_0037744C)
