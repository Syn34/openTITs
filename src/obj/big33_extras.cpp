// Game code, large-function sweep part 33: the extras menu refreshed: the
// "Extra01".."Extra08" entries of its "Menu", "Cost" and "Data" groups get
// on / off / unknown, cost and description strings, plus the mask count.
#include "big33.h"

namespace sly4::obj {

// The extras table (8 entries, 12 bytes).
struct ExtraDef {
    be_u32 cost;  // +0
    be_u32 desc;  // +4: const char*
    pad<4> _8;
};
static_assert(sizeof(ExtraDef) == 12);
struct ExtrasWorld {
    pad<30> _0;
    u8 page;  // +30: selects the 0x20C80-byte page whose +4208 byte is the player
};
struct MenuEvent {
    pad<12> _0;
    gp<UiElem> root;  // +12
};

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
constexpr u32 kExtras = 0x00AE6A60;        // ExtraDef[8]
constexpr u32 kAllExtrasFlag = 0x010BB849;  // u8: extras 0, 1 and 4 open while set (as Unlock_IsOpen_0046C7E0)
constexpr u32 kSession = 0x00F64C20;       // Session*
constexpr u32 Session_Count2DDC5C = 0x002DDC5C;  // (session) -> u32 (what an extra's cost is compared with)
constexpr u32 Rec_CountRefs = 0x002E57A8;        // (session) -> u32 (the mask total)

const ExtraDef& Extra(u32 i) { return host<ExtraDef>(kExtras)[i]; }

// An extra is shown (not unknown) when it is 0, 1 or 4 with the flag set,
// or when have >= its cost.
bool ExtraKnown(u32 i, u32 have) {
    if ((i <= 1 || i == 4) && global<u8>(kAllExtrasFlag) != 0) return true;
    return !(have < Extra(i).cost);
}

// Element "Extra%02d" (i + 1) of group g when it is in state 3, else null.
UiElem* Entry(UiElem* g, u32 i) {
    const u32 name = call<u32>(fn::String_Format, kFmtExtra, i + 1);
    UiElem* e = call<UiElem*>(fn::Ui_Find, name, g);
    if (e == nullptr) return nullptr;
    return UiElem_State(e) == 3 ? e : nullptr;
}
}  // namespace

// void f(World* t, const Event* x) (called by f_004734E0 and f_00476C70). For
// the local player's record (t + 4056 * the u8 at the page t + 4208 + t->page
// * 0x20C80), have = Session_Count2DDC5C(session), total =
// Rec_CountRefs(session). Under x->root: "Menu" entries get "$STR_ON" /
// "$STR_OFF" (bit 0x2 of the record's u8 +4112 + i) or "$STR_UNKNOWN";
// "Cost" entries get "%d" of the extra's cost, and "MaskCount" "%d/%d" of
// min(have, total) and total; "Data" entries get the extra's description or
// "$STR_UNKNOWN".
void Menu_RefreshExtras_0046DB28(ExtrasWorld* t, const MenuEvent* x) {
    const u32 tw = guest(t);
    const u32 page = tw + t->page * 0x20C80u + 4208;
    const u32 rec = tw + gm<u8>(page) * 4056u;
    const u32 have = call<u32>(Session_Count2DDC5C, global<be_u32>(kSession).get());
    const u32 total = call<u32>(Rec_CountRefs, global<be_u32>(kSession).get());

    if (UiElem* menu = call<UiElem*>(fn::Ui_Find, kStrMenu, x->root.get())) {
        for (u32 i = 0; i < 8; ++i) {
            UiElem* e = Entry(menu, i);
            if (e == nullptr) continue;
            const u32 s = !ExtraKnown(i, have)                ? kStrUnknown
                          : (gm<u8>(rec + 4112 + i) & 2) != 0 ? kStrOn
                                                              : kStrOff;
            UiElem_SetText(e, s);
        }
    }
    if (UiElem* cost = call<UiElem*>(fn::Ui_Find, kStrCost, x->root.get())) {
        for (u32 i = 0; i < 8; ++i) {
            UiElem* e = Entry(cost, i);
            if (e == nullptr) continue;
            UiElem_SetText(e, call<u32>(fn::String_Format, kFmtD, Extra(i).cost.get()));
        }
        UiElem* m = call<UiElem*>(fn::Ui_Find, kStrMaskCount, cost);
        if (m != nullptr && UiElem_State(m) == 3) {
            const u32 s = call<u32>(fn::String_Format, kFmtDOfD, have > total ? total : have, total);
            UiElem_SetText(m, s);
        }
    }
    if (UiElem* data = call<UiElem*>(fn::Ui_Find, kStrData, x->root.get())) {
        for (u32 i = 0; i < 8; ++i) {
            UiElem* e = Entry(data, i);
            if (e == nullptr) continue;
            UiElem_SetText(e, ExtraKnown(i, have) ? Extra(i).desc.get() : kStrUnknown);
        }
    }
}

}  // namespace sly4::obj

NATIVE(0046DB28, sly4::obj::Menu_RefreshExtras_0046DB28)
