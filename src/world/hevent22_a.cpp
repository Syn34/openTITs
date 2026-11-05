// World_HandleEvent callees, part 22a: the object mode setters named by the
// hevent21_a.cpp functions.
#include "hevent22.h"

using namespace sly4;
using namespace sly4::hevent;
using namespace sly4::hevent22;

namespace {
constexpr u32 kOneF36876C = 0x0036876C;  // f32 1.0
constexpr u32 kOneF368AC4 = 0x00368AC4;  // f32 1.0
constexpr u32 kMinusOne = 0xBF800000u;   // f32 -1.0 bits
// {u32 key, Object*} map of objects switched off with a pending request
// (name?); same layout as hevent.h kHandleMap.
constexpr u32 kModeOffMap = 0x0106C240;
}  // namespace

// u32 Obj_ModeIdle368770(Object* o) (name?): +532 |= 0x40000000, mode 1;
// the +24 object (if any) gets vt +40 (p, 1, 1). Then
// Speaker_SayId4A5C90(+28, +164[mode] text id, 0; 1.0) into +532 bit
// 0x00800000 and, with an effect +20, Effect_SetHash4CF988(e,
// 0x00F64FB0[mode]) into bit 0x00400000. The mode is re-read after the
// virtual call. Returns the last call's result (0 without an effect).
DECOMP(00368770, Obj_ModeIdle368770) {
    const u32 o = arg<u32>(ctx, 0);
    store<u32>(o + kObjModeFlags, load<u32>(o + kObjModeFlags) | 0x40000000u);
    store<u32>(o + kObjMode, (load<u32>(o + kObjMode) & 1u) | (1u << 1));
    const u32 p = load<u32>(o + 24);
    if (p != 0) vcall(ctx, p, 40, {p, 1, 1});
    // rldicr: the 31-bit mode times 4 as a 64-bit offset, the sum wrapped to 32 bits.
    const u32 off = (load<u32>(o + kObjMode) >> 1) << 2;
    const u32 id = load<u32>(o + 164 + off);
    callf(ctx, h22fn::Speaker_SayId, {load<u32>(o + 28), id, 0}, {hevent7::lfs_f64(kOneF36876C)});
    const u32 said = static_cast<u32>(ctx.gpr[3]);
    store<u32>(o + kObjModeFlags, (load<u32>(o + kObjModeFlags) & ~0x00800000u) | ((said & 1u) << 23));
    const u32 e = load<u32>(o + 20);
    if (e == 0) {
        ctx.gpr[3] = 0;
        return;
    }
    const u32 r = static_cast<u32>(call(ctx, h22fn::Effect_SetHash, {e, load<u32>(kModeHashes + off)}));
    store<u32>(o + kObjModeFlags, (load<u32>(o + kObjModeFlags) & ~0x00400000u) | ((r & 1u) << 22));
}

// void Obj_ModeOff368AC8(Object* o, u8 a, u8 b) (name?): only while +532
// has bit 0x40000000 and neither of 0x03000000 (else returns with r3 = +12).
// Modes other than 0 / 4: +532 bit 0x20000000 = a, bit 0x10000000 = b,
// mode 4, +196 = -1.0, Speaker_SayId4A5C90(+28, +180, 0; 1.0) into +532
// bit 0x00800000, Effect_SetHash4CF988(+20, 0x00F64FB0[4]) into bit
// 0x00400000 when there is an effect, and vt +40 (p, 1, 1) on the +24
// object. Mode 0 with a (or +532 bit 0x20000000): vt +12 (o). Mode 0 with b
// (or bit 0x10000000): unless the key (+12, else the +8 def's +68, else 0)
// is already in the 0x0106C240 map, {+12 (else the def's +68), o} is
// appended to it. Every path past the gate clears +204 and returns 0.
DECOMP(00368AC8, Obj_ModeOff368AC8) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 a = arg<u32>(ctx, 1), b = arg<u32>(ctx, 2);
    const u32 flags = load<u32>(o + kObjModeFlags);
    const u32 key0 = load<u32>(o + 12);
    const u32 def = load<u32>(o + 8);
    ctx.gpr[3] = key0;
    if ((flags & 0x02000000u) != 0 || (flags & 0x01000000u) != 0 || (flags & 0x40000000u) == 0) return;
    const u32 mode_word = load<u32>(o + kObjMode);
    const u32 mode = mode_word >> 1;
    if (mode != 0 && mode != 4) {
        u32 f = (flags & ~0x20000000u) | ((a & 1u) << 29);
        f = (f & ~0x10000000u) | ((b & 1u) << 28);
        store<u32>(o + kObjModeFlags, f);
        store<u32>(o + kObjMode, (mode_word & 1u) | (4u << 1));
        store<u32>(o + 196, kMinusOne);
        const u32 id = load<u32>(o + 180);
        ctx.gpr[6] = id;
        callf(ctx, h22fn::Speaker_SayId, {load<u32>(o + 28), id, 0}, {hevent7::lfs_f64(kOneF368AC4)});
        const u32 said = static_cast<u32>(ctx.gpr[3]);
        store<u32>(o + kObjModeFlags, (load<u32>(o + kObjModeFlags) & ~0x00800000u) | ((said & 1u) << 23));
        const u32 e = load<u32>(o + 20);
        if (e != 0) {
            const u32 r = static_cast<u32>(call(ctx, h22fn::Effect_SetHash, {e, load<u32>(kModeHashes + 16)}));
            store<u32>(o + kObjModeFlags, (load<u32>(o + kObjModeFlags) & ~0x00400000u) | ((r & 1u) << 22));
        }
        const u32 p = load<u32>(o + 24);
        if (p != 0) vcall(ctx, p, 40, {p, 1, 1});
    } else if (((flags & 0x20000000u) != 0 || (a & 0xFF) != 0) && mode == 0) {
        vcall(ctx, o, 12, {o});
    } else if (((flags & 0x10000000u) != 0 || (b & 0xFF) != 0) && mode == 0 && o != 0) {
        const u32 key = key0 != 0 ? key0 : (def != 0 ? load<u32>(def + 68) : 0);
        Stack frame(ctx, 16);
        store<u32>(frame.addr(), key);
        const u32 slot = static_cast<u32>(call(ctx, h22fn::SortedMap_Find3, {kModeOffMap, frame.addr()}));
        if (load<u32>(slot) == 0) {
            u32 k = load<u32>(o + 12);
            if (k == 0) {
                const u32 d = load<u32>(o + 8);
                k = d != 0 ? load<u32>(d + 68) : 0;
            }
            map_append(ctx, kModeOffMap, k, o);
        }
    }
    store<u32>(o + 204, 0);
    ctx.gpr[3] = 0;
}

namespace {
constexpr u32 kOneF367C1C = 0x00367C1C;          // f32 1.0
constexpr u32 kIdleHashes = 0x00F64F9C;          // u32[mode] effect hashes for Effect_SetHash4CF988
constexpr u32 kSessionPtr = 0x00F64C20;          // u32 Session* (as Session_PartDone2E2740's callers)
constexpr u32 kWorldGetSaveObj160 = 0x002E29B0;  // () -> u32 (symbols.txt World_GetSaveObj160)
constexpr u32 kSessionPartDone = 0x002E2740;     // (Session*, s32 key) -> bool (symbols.txt Session_PartDone2E2740)
constexpr u32 kItemOwned49E628 = 0x0049E628;     // (u32 kind, u32 key) -> bool (name?)
constexpr u32 kWorldPartObj = 0x128164;          // World +0x128164: part object (+5572 bit 0x08000000)
constexpr u32 kEmptyStr = 0x00ADCAA8;            // ""
constexpr u32 kStrCostNotAvailable = 0x00ADCB48; // "$STR_COST_NOT_AVAILABLE"
constexpr u32 kF180 = 0x43340000u;               // f32 180.0 bits
// Def +216 kind hashes and the save data words (World +80 +...) they name.
constexpr u32 kKindPart = 0x63C971C0u;  // key from World_GetSaveObj160; ownership by Session_PartDone2E2740
constexpr u32 kKind300 = 0x1D8C2817u, kKind304 = 0xDF0611F9u, kKind296 = 0x88397820u;
// Def +148: 56-byte items {+12 u32, +16 key, +24 text}, +152 s32 count;
// +196: 28-byte widget slots {+8 hash, +20 Widget*}, +200 u32 count.
constexpr u32 kDefItems = 148, kDefItemCount = 152, kItemSize = 56;
constexpr u32 kDefSlots = 196, kDefSlotCount = 200, kSlotSize = 28;
constexpr u32 kSlotPrice = 0x00225CAE, kSlotStatus = 0x04293944;

// The first widget slot of def named hash, or -1.
s32 find_slot(u32 def, u32 hash) {
    const u32 n = load<u32>(def + kDefSlots + 4);
    for (u32 k = 0; k < n; ++k)
        if (load<u32>(load<u32>(def + kDefSlots) + k * kSlotSize + 8) == hash) return static_cast<s32>(k);
    return -1;
}
// Widget k of def: vt +188 (w, text, 0) then, the widget re-read, vt +148.
// Returns false when the slot has no widget.
bool set_text(PPUContext& ctx, u32 def, u32 k, u32 text) {
    const u32 w = load<u32>(load<u32>(def + kDefSlots) + k * kSlotSize + 20);
    if (w == 0) return false;
    vcall(ctx, w, 188, {w, text, 0});
    const u32 w2 = load<u32>(load<u32>(def + kDefSlots) + k * kSlotSize + 20);
    vcall(ctx, w2, 148, {w2});
    return true;
}
}  // namespace

// u32 Obj_ModeIdle367C20(ShopObj* o) (name?): +216 |= 0x40000000, mode 1,
// +24 vt +40 (p, 1, 1), Speaker_SayId4A5C90(+28, +180[mode], 0; 1.0) into
// +216 bit 0x04000000 and Effect_SetHash4CF988(+20, 0x00F64F9C[mode]) into
// bit 0x02000000. Then the def's (+8) item list is matched against the
// save value its kind names (+172 = the last match, +212 = its +12 word)
// and the item is checked as owned: 0x0049E628(kind, key) (a zero key
// counts as owned), or for the part kind Session_PartDone2E2740. Not owned:
// the price widget gets "" and, for the part kind, the status widget
// "$STR_COST_NOT_AVAILABLE"; owned: the price widget gets the item's text
// and the status widget "". +208[+172] (12-byte entries) gets {0, 0}
// (not owned, returns 0) or {180.0, 180.0} (owned, returns its bits).
DECOMP(00367C20, Obj_ModeIdle367C20) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 def0 = load<u32>(o + 8);  // the item list's def: read before the calls
    store<u32>(o + 216, load<u32>(o + 216) | 0x40000000u);
    store<u32>(o + kObjMode, (load<u32>(o + kObjMode) & 1u) | (1u << 1));
    const u32 p = load<u32>(o + 24);
    if (p != 0) vcall(ctx, p, 40, {p, 1, 1});
    const u32 off = (load<u32>(o + kObjMode) >> 1) << 2;
    callf(ctx, h22fn::Speaker_SayId, {load<u32>(o + 28), load<u32>(o + 180 + off), 0}, {hevent7::lfs_f64(kOneF367C1C)});
    const u32 said = static_cast<u32>(ctx.gpr[3]);
    store<u32>(o + 216, (load<u32>(o + 216) & ~0x04000000u) | ((said & 1u) << 26));
    const u32 e = load<u32>(o + 20);
    if (e != 0) {
        const u32 r = static_cast<u32>(call(ctx, h22fn::Effect_SetHash, {e, load<u32>(kIdleHashes + off)}));
        store<u32>(o + 216, (load<u32>(o + 216) & ~0x02000000u) | ((r & 1u) << 25));
    }

    u32 def1 = load<u32>(o + 8);
    u32 idx = 0;
    store<u32>(o + 172, 0);
    const u32 save = load<u32>(kCurrentWorld) + 80;
    const u32 kind = load<u32>(def1 + 216);
    u32 want = 0;
    if (kind == kKindPart) {
        want = static_cast<u32>(call(ctx, kWorldGetSaveObj160, {load<u32>(kSessionPtr)}));
        idx = load<u32>(o + 172);
        def1 = load<u32>(o + 8);
    } else if (kind == kKind300) {
        want = load<u32>(save + 300);
    } else if (kind == kKind304) {
        want = load<u32>(save + 304);
    } else if (kind == kKind296) {
        want = load<u32>(save + 296);
    }
    const s32 n = static_cast<s32>(load<u32>(def0 + kDefItemCount));
    for (s32 k = 0; k < n; ++k) {
        if (load<u32>(load<u32>(def0 + kDefItems) + static_cast<u32>(k) * kItemSize + 16) == want) {
            idx = static_cast<u32>(k);
            store<u32>(o + 172, idx);
        }
    }
    const u32 items = load<u32>(def0 + kDefItems);
    store<u32>(o + 212, load<u32>(items + idx * kItemSize + 12));
    const u32 kind2 = load<u32>(def1 + 216);
    const u32 key = load<u32>(items + idx * kItemSize + 16);
    bool owned;
    if (kind2 != kKindPart) {
        owned = key == 0 || static_cast<u32>(call(ctx, kItemOwned49E628, {kind2, key})) != 0;
    } else {
        const u32 sess = load<u32>(kSessionPtr);
        const u32 player = load<u32>(sess + 36);
        u32 w = 0;
        if (player == 0 || (w = load<u32>(load<u32>(kCurrentWorld) + kWorldPartObj)) == 0 || (load<u32>(w + 5572) & 0x08000000u) != 0)
            owned = static_cast<u32>(call(ctx, kSessionPartDone, {sess, key})) != 0;
        else if (load<u32>(player + 812) == key)
            owned = static_cast<u32>(call(ctx, kSessionPartDone, {sess, key})) != 0;
        else
            owned = false;
    }

    u32 value = 0;
    if (!owned) {
        u32 def2 = load<u32>(o + 8);
        if (load<u32>(def2 + kDefSlotCount) != 0) {
            const s32 k = find_slot(def2, kSlotPrice);
            if (k >= 0 && set_text(ctx, def2, static_cast<u32>(k), kEmptyStr)) def2 = load<u32>(o + 8);
        }
        if (load<u32>(def0 + 216) == kKindPart && load<u32>(def2 + kDefSlotCount) != 0) {
            const s32 k = find_slot(def2, kSlotStatus);
            if (k >= 0) set_text(ctx, def2, static_cast<u32>(k), kStrCostNotAvailable);
        }
    } else {
        const u32 text = load<u32>(def0 + kDefItems) + load<u32>(o + 172) * kItemSize + 24;
        u32 def3 = load<u32>(o + 8);
        if (text != 0 && load<u32>(def3 + kDefSlotCount) != 0) {
            const s32 k = find_slot(def3, kSlotPrice);
            if (k >= 0 && set_text(ctx, def3, static_cast<u32>(k), text)) def3 = load<u32>(o + 8);
        }
        if (load<u32>(def3 + kDefSlotCount) != 0) {
            const s32 k = find_slot(def3, kSlotStatus);
            if (k >= 0) set_text(ctx, def3, static_cast<u32>(k), kEmptyStr);
        }
        value = kF180;
    }
    const u32 arr = load<u32>(o + 208) + load<u32>(o + 172) * 12;
    store<u32>(arr, value);
    store<u32>(arr + 4, value);
    ctx.gpr[3] = value;
}
