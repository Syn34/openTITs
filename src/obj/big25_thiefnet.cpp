// Game code, large-function sweep part 25: the ThiefNet shop page fill (six
// rows of name / price labels from a scrolling list of upgrades).
#include "../sly4.h"
#include "big25.h"

using namespace sly4;
using namespace sly4::big25;

namespace {
constexpr u32 kItemAt = 0x0049E704;        // Table10BE8BC_At_0049E704 (u32 id) -> item or 0 (+12 price, +16 name text)
constexpr u32 kStringFormat = 0x005534D0;  // String_Format (fmt, ...) -> rotating static buffer
constexpr u32 kHashNoCase = 0x0056A710;    // String_HashNoCase (const char*) -> u32
constexpr u32 kRowLabels = 0x00F64DD0;     // const char* [6][2]: each row's name / price label names
constexpr u32 kEmpty = 0x00ADCAA8;         // ""
constexpr u32 kFmtInt = 0x00ADC94C;        // "%d"
constexpr u32 kSold = 0x00ADCBE4;          // "$HIDEOUT_THIEFNET_SOLD"
constexpr u32 kNoUpgrades = 0x00ADCBFC;    // "$THIEFNET_NO_UPGRADES_AVAILABLE"
constexpr u32 kNoUpgradesLabel = 0x001FF691u;
constexpr u32 kHintLabel = 0x968EF501u;

// The menu's label (records of 28 bytes at +184, count +188, the hash at
// +0 and the widget at +20) named hash gets vfn[188](text, 0) and then
// vfn[148](); only the first record with the hash counts. Skipped for a
// null text or hash. Returns whether the widget calls were made.
bool SetLabel(PPUContext& ctx, u32 menu, u32 hash, u32 text) {
    if (text == 0 || hash == 0) return false;
    const u32 n = load<u32>(menu + 188);
    for (u32 j = 0; j < n; ++j) {
        if (load<u32>(load<u32>(menu + 184) + 28 * j) != hash) continue;
        const u32 w = load<u32>(load<u32>(menu + 184) + 28 * j + 20);
        if (w == 0) return false;
        VCall(ctx, w, 188, {w, text, 0});
        const u32 w2 = load<u32>(load<u32>(menu + 184) + 28 * j + 20);
        VCall(ctx, w2, 148, {w2});
        return true;
    }
    return false;
}

u32 Hash(PPUContext& ctx, u32 s) { return static_cast<u32>(call(ctx, kHashNoCase, {s})); }
}  // namespace

// void f(ShopPage* o) (called by 003780E0, 00378540). o: +8 the menu, +256
// the scroll position, +260 the selected row, +400 the item ids, +408 (u16)
// their count. Row i (0..5) shows item (+256 + count + i - +260) mod count
// when that index is in range or there are at least 6 items: the name label
// gets the item's +16 text and the price label "%d" of its +12, or
// "$HIDEOUT_THIEFNET_SOLD" when bit 2 of the save's byte (world +4028 +
// id) is set; a missing item leaves the row alone; otherwise both labels
// are cleared. With no items the label 0x001FF691 says
// "$THIEFNET_NO_UPGRADES_AVAILABLE" and 0x968EF501 is cleared.
DECOMP(0037744C, ThiefNet_FillPage_0037744C) {
    const u32 o = arg<u32>(ctx, 0);
    Stack frame(ctx, 208);
    const u32 world = load<u32>(b25fn::Game);

    for (u32 i = 0; i < 6; ++i) {
        const u32 n = load<u16>(o + 408);
        const u32 labels = kRowLabels + 8 * i;
        bool show = false;
        u32 idx = 0;
        if (n != 0) {
            const u32 d = i - load<u32>(o + 260);
            const s32 at = static_cast<s32>(load<u32>(o + 256) + d);
            if (!(at < 0 || at >= static_cast<s32>(n)) || n >= 6) {
                idx = (load<u32>(o + 256) + n + d) % n;
                show = idx < n;
                if (!show) continue;
            }
        }
        if (!show) {
            const u32 h0 = Hash(ctx, load<u32>(labels));
            SetLabel(ctx, load<u32>(o + 8), h0, kEmpty);
            const u32 h1 = Hash(ctx, load<u32>(labels + 4));
            SetLabel(ctx, load<u32>(o + 8), h1, kEmpty);
            continue;
        }
        const u32 item = static_cast<u32>(call(ctx, kItemAt, {load<u32>(load<u32>(o + 400) + 4 * idx)}));
        if (item == 0) continue;
        const u32 h0 = Hash(ctx, load<u32>(labels));
        SetLabel(ctx, load<u32>(o + 8), h0, item + 16);
        const u32 id = load<u32>(load<u32>(o + 400) + 4 * idx);
        u32 price;
        if (load<u8>(world + 4028 + id) & 2) price = kSold;
        else price = static_cast<u32>(call(ctx, kStringFormat, {kFmtInt, Sw(item + 12)}));
        const u32 h1 = Hash(ctx, load<u32>(labels + 4));
        SetLabel(ctx, load<u32>(o + 8), h1, price);
    }

    const u32 n = load<u16>(o + 408);
    if (n != 0) {
        ctx.gpr[3] = n;  // as the original leaves r3
        return;
    }
    SetLabel(ctx, load<u32>(o + 8), kNoUpgradesLabel, kNoUpgrades);
    if (!SetLabel(ctx, load<u32>(o + 8), kHintLabel, kEmpty)) ctx.gpr[3] = kEmpty;  // as the original leaves r3
}
