// Game code, large-function sweep part 33: the extras menu refreshed: the
// "Extra01".."Extra08" entries of its "Menu", "Cost" and "Data" groups get
// on / off / unknown, cost and description strings, plus the mask count.
#include "../sly4.h"
#include "big33.h"

using namespace sly4;
using namespace sly4::big33;

namespace {
constexpr u32 kStrMenu = 0x00AE8800;       // "Menu"
constexpr u32 kStrCost = 0x00AE8A84;       // "Cost"
constexpr u32 kStrData = 0x00AE8890;       // "Data"
constexpr u32 kFmtExtra = 0x00AE8A64;      // "Extra%02d"
constexpr u32 kStrOn = 0x00AE8818;         // "$STR_ON"
constexpr u32 kStrOff = 0x00AE8820;        // "$STR_OFF"
constexpr u32 kStrUnknown = 0x00AE8A74;    // "$STR_UNKNOWN"
constexpr u32 kFmtD = 0x00AE8A8C;          // "%d"
constexpr u32 kStrMaskCount = 0x00AE8A94;  // "MaskCount"
constexpr u32 kFmtDOfD = 0x00AE8AA4;       // "%d/%d"
constexpr u32 kExtras = 0x00AE6A60;        // 8 x {u32 cost, const char* desc, ...} (12 bytes)
constexpr u32 kAllExtrasFlag = 0x010BB849;  // u8: extras 0, 1 and 4 open while set (as Unlock_IsOpen_0046C7E0)
constexpr u32 kSession = 0x00F64C20;       // Session*
constexpr u32 Session_Count2DDC5C = 0x002DDC5C;  // (session) -> u32 (what an extra's cost is compared with)
constexpr u32 Rec_CountRefs = 0x002E57A8;        // (session) -> u32 (the mask total)
constexpr u32 Ui_Find = 0x004D5544;              // (path, parent) -> element or 0
constexpr u32 String_Format = 0x005534D0;        // (fmt, ...) -> const char*

// An extra is shown (not unknown) when it is 0, 1 or 4 with the flag set,
// or when have >= its cost.
bool ExtraKnown(u32 i, u32 have) {
    if ((i <= 1 || i == 4) && load<u8>(kAllExtrasFlag) != 0) return true;
    return !(have < load<u32>(kExtras + 12 * i));
}

// Element "Extra%02d" (i + 1) of group g when it is in state 3 (vt slot 12),
// else 0.
u32 Entry(PPUContext& ctx, u32 g, u32 i) {
    const u32 name = static_cast<u32>(call(ctx, String_Format, {kFmtExtra, i + 1}));
    const u32 e = static_cast<u32>(call(ctx, Ui_Find, {name, g}));
    if (e == 0) return 0;
    return static_cast<u32>(VCall(ctx, e, 12, {e})) == 3 ? e : 0;
}

void SetText(PPUContext& ctx, u32 e, u32 s) { VCall(ctx, e, 188, {e, s, 0}); }
}  // namespace

// void f(World* t, const Event* x) (called by f_004734E0 and f_00476C70). For
// the local player's record (t + 4056 * the u8 at the page t + 4208 + u8 t+30
// * 0x20C80), have = Session_Count2DDC5C(session), total =
// Rec_CountRefs(session). Under x+12: "Menu" entries get "$STR_ON" / "$STR_OFF"
// (bit 0x2 of the record's u8 +4112 + i) or "$STR_UNKNOWN"; "Cost" entries
// get "%d" of the extra's cost, and "MaskCount" "%d/%d" of min(have, total)
// and total; "Data" entries get the extra's description or "$STR_UNKNOWN".
// Text goes through vt slot 188 (e, s, 0); r3 is left as the last call's.
DECOMP(0046DB28, Menu_RefreshExtras_0046DB28) {
    const u32 t = arg<u32>(ctx, 0), x = arg<u32>(ctx, 1);
    const u32 page = t + load<u8>(t + 30) * 0x20C80u + 4208;
    const u32 rec = t + load<u8>(page) * 4056u;
    const u32 have = static_cast<u32>(call(ctx, Session_Count2DDC5C, {load<u32>(kSession)}));
    const u32 total = static_cast<u32>(call(ctx, Rec_CountRefs, {load<u32>(kSession)}));

    const u32 menu = static_cast<u32>(call(ctx, Ui_Find, {kStrMenu, load<u32>(x + 12)}));
    if (menu != 0) {
        for (u32 i = 0; i < 8; ++i) {
            const u32 e = Entry(ctx, menu, i);
            if (e == 0) continue;
            const u32 s = !ExtraKnown(i, have)            ? kStrUnknown
                          : (load<u8>(rec + 4112 + i) & 2) ? kStrOn
                                                           : kStrOff;
            SetText(ctx, e, s);
        }
    }
    const u32 cost = static_cast<u32>(call(ctx, Ui_Find, {kStrCost, load<u32>(x + 12)}));
    if (cost != 0) {
        for (u32 i = 0; i < 8; ++i) {
            const u32 e = Entry(ctx, cost, i);
            if (e == 0) continue;
            SetText(ctx, e, static_cast<u32>(call(ctx, String_Format, {kFmtD, load<u32>(kExtras + 12 * i)})));
        }
        const u32 m = static_cast<u32>(call(ctx, Ui_Find, {kStrMaskCount, cost}));
        if (m != 0 && static_cast<u32>(VCall(ctx, m, 12, {m})) == 3) {
            const u32 s = static_cast<u32>(call(ctx, String_Format, {kFmtDOfD, have > total ? total : have, total}));
            SetText(ctx, m, s);
        }
    }
    const u32 data = static_cast<u32>(call(ctx, Ui_Find, {kStrData, load<u32>(x + 12)}));
    if (data != 0) {
        for (u32 i = 0; i < 8; ++i) {
            const u32 e = Entry(ctx, data, i);
            if (e == 0) continue;
            SetText(ctx, e, ExtraKnown(i, have) ? load<u32>(kExtras + 12 * i + 4) : kStrUnknown);
        }
    }
}
