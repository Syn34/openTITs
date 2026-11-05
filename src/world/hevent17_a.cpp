// World_HandleEvent callees, part 17a: small leaves under the hevent16
// functions (hud icon, query set-up, text lookup, world main object
// refresh / pause / resume, animator layers, target enable, effect slots,
// coach page, object component vector).
#include "hevent17.h"

using namespace sly4;
using namespace sly4::hevent;
using namespace sly4::hevent17;

namespace {
// Hud: +24124 u8 icon shown, +24120 Icon*.
constexpr u32 kHudIconOn = 24124, kHudIcon = 24120;

// Query (24 bytes): +0 u32 kind, +4 f32, +8 / +12 / +16 u32, +20 Object*.
// World +12: tuning record whose first f32 seeds +4.
constexpr u32 kQueryKind = 0, kQueryValue = 4, kQueryObject = 20, kQueryKindObject = 9;
constexpr u32 kWorldTune = 12;

// The text map searched by Text_Lookup34A398.
constexpr u32 kTextMap = 0x0106BBE8;

// World callees.
constexpr u32 kMainRefresh42685C = 0x0042685C;  // (Object*) (name?)
constexpr u32 kMainPause426AD8 = 0x00426AD8;    // (Object*, u8) (name?)
constexpr u32 kAnimSet2549F8 = 0x002549F8;      // (Component*, 1) (name?)
constexpr u32 kSceneResume1B2F4 = 0x0001B2F4;   // (Scene*, u8) (name?)
constexpr u32 kCompType36 = 36;

// Animator: 8 layers of 40 bytes from +40 (+0 u32 key, +4 Layer* (+20 u8
// flags, bit 6 0x40: playing)); +28 u32, +36 Set*.
constexpr u32 kAnimLayers = 40, kAnimLayerSize = 40, kAnimNumLayers = 8;
constexpr u32 kLayerKey = 0, kLayerPtr = 4, kLayerFlags = 20, kLayerPlaying = 0x40;
constexpr u32 kAnimArg28 = 28, kAnimSet = 36;
// Set +92: vector {+0 Entry*[], +8 u16 count}; Entry +8 u32 key.
constexpr u32 kSetLayers = 92, kSetEntryKey = 8;
constexpr u32 kSetLoad351A40 = 0x00351A40;  // (Set*, u32 animator +28) (name?)

// Target: +180 s16 enable count, +182 u16 flags (bits 14, 15 cleared on
// enable).
constexpr u32 kTargetCount = 180, kTargetFlags = 182, kTargetClearOnEnable = 0xC000;

// Effects record: +48 Owner* (vt +60, +64 getters), +64 / +68 results.
constexpr u32 kEffOwner = 48, kEffVtA = 60, kEffVtB = 64, kEffA = 64, kEffB = 68;

// Object +216 def (+772 + 196 u32 count of 44-byte entries from +772, each
// +24 f32 threshold); +1168 f32 value; +968 u8 flags (bit 1 0x02: matched).
constexpr u32 kObjDef = 216, kDefEntries = 772, kDefEntryCount = 196, kDefEntrySize = 44, kDefEntryValue = 24;
constexpr u32 kObjValue1168 = 1168, kObjFlags968 = 968, kObjMatched = 0x02;
constexpr u32 kZeroF12E5E4 = 0x0012E5E4;  // f32 0.0

// Effect slot: +0 u32, +4 Effect*, +16 u32; Effect vt +88 stop(e, 0, 1, 1,
// 1), +40 enable(e, u8); +172 bit 18 (0x40000) re-enable.
constexpr u32 kSlotA = 0, kSlotEffect = 4, kSlotB = 16;
constexpr u32 kEffVtStop = 88, kEffVtEnable = 40, kEffFlags172 = 172, kEffReenable = 0x40000;
constexpr u32 kEffBind4A621C = 0x004A621C;  // (Effect** slot, def, owner, value, u8) (name?)

// Coach: +8 Object* (+192 bit 3 0x8: has the page table at +656); 8 pages
// of 16 bytes from +2328 (+0 u32 used, +8 s32 index in the table).
constexpr u32 kCoachObj = 8, kObjFlags192 = 192, kObjHasPages = 0x8, kObjPageTable = 656;
constexpr u32 kCoachPages = 2328, kPageSize = 16, kPageIndex = 8, kNumPages = 8;
constexpr u8 kPageOn = 0x80, kPageHidden = 0x40;

// World +0x128480: 13 vectors of 20 bytes {+0 12-byte records {+8 Player*},
// +8 u16 count}.
constexpr u32 kWorldPlayerLists = 0x128480, kPlayerListStride = 20, kNumPlayerLists = 13;
constexpr u32 kPlRecSize = 12, kPlRecPlayer = 8;

// Object +248: component vector (20 bytes, built here: +0 data, +4 u16
// capacity, +6 u16 grow step, +8 u16 count, +10 u16 high-water mark).
constexpr u32 kObjComps = 248, kVecData = 0, kVecCap = 4, kVecStep = 6, kVecCount = 8, kVecHigh = 10;
constexpr u32 kTmplOwner = 8;
constexpr u32 kCompCreate2635E8 = 0x002635E8;  // (Object*, Component* template, u8) -> Component* (name?)

// Player: +164 Animator* (+40 u16 flags: bit 9 0x200 from def +1408 bit
// 0, bit 10 0x400 cleared when +188 bit 0; +60 callback OPD, +64 its
// argument); +216 def; vt +60 animator getter.
constexpr u32 kPlAnim = 164, kPlDef = 216, kDefFlag1408 = 1408, kPlFlags188 = 188;
constexpr u32 kAnimFlags = 40, kAnimFlagDef = 0x200, kAnimFlag188 = 0x400, kAnimCb = 60, kAnimCbArg = 64;
constexpr u32 kPlVtAnimator = 60;
constexpr u32 kAnimCbOpd = 0x00F31530;
constexpr u32 kPlSettle11F0E4 = 0x0011F0E4;  // (Player*, -, 0; f32) (name?)
constexpr u32 kPlGet0D9474 = 0x000D9474;     // (Player*) -> Object* or 0 (name?)
constexpr u32 kObjReset293264 = 0x00293264;  // (Object*) (name?)
constexpr u32 kZeroF120488 = 0x00120488;     // f32 0.0
}  // namespace

// void Hud_SetIcon4645A0(Hud* h, u8 on, Icon* icon) (name?): +24124 = on,
// +24120 = icon.
DECOMP(004645A0, Hud_SetIcon4645A0) {
    const u32 h = arg<u32>(ctx, 0);
    store<u8>(h + kHudIconOn, arg<u8>(ctx, 1));
    store<u32>(h + kHudIcon, arg<u32>(ctx, 2));
}

// void Query_Init465218(Query* q, Object* o) (name?): an object query (kind
// 9) seeded with the current world's tuning value.
DECOMP(00465218, Query_Init465218) {
    const u32 q = arg<u32>(ctx, 0), o = arg<u32>(ctx, 1);
    store<u32>(q + kQueryObject, o);
    const u32 tune = load<u32>(load<u32>(kCurrentWorld) + kWorldTune);
    store<u32>(q + kQueryKind, kQueryKindObject);
    store<u32>(q + 8, 0);
    store<u32>(q + kQueryValue, load<u32>(tune));  // lfs / stfs pair
    store<u32>(q + 12, 0);
    store<u32>(q + 16, 0);
    ctx.fpr[1] = hevent7::lfs_f64(tune);  // f1 left holding the value (no known caller reads it)
}

// const char* Text_Lookup34A398(u32 id) (name?): the value slot of id in the
// text map at 0x0106BBE8, dereferenced (the slot is never null-checked).
DECOMP(0034A398, Text_Lookup34A398) {
    Stack frame(ctx, 16);  // the key word (r1 +112 in the original frame)
    store<u32>(frame.addr(), arg<u32>(ctx, 0));
    const u32 slot = static_cast<u32>(call(ctx, h17fn::SortedMap_Find76DC10, {kTextMap, frame.addr()}));
    ret(ctx, load<u32>(slot));
}

// u32 World_Refresh489EF4(World* w) (name?): refreshes the world's main
// object when it is active (+164 bit 31). Returns the object, or the call's
// result.
DECOMP(00489EF4, World_Refresh489EF4) {
    const u32 o = load<u32>(arg<u32>(ctx, 0) + kWorldMain);
    if (o == 0 || (load<u32>(o + kObjFlags164) & kObjActive) == 0) return ret(ctx, o);
    ctx.gpr[4] = 1;  // rlwinm r4: the active bit as passed (the callee reads only r3)
    call(ctx, kMainRefresh42685C, {o});
}

// u32 World_Pause48A01C(World* w, u8 a) (name?): passes a to the main
// object's pause when it is active. Returns the object, or the call's result.
DECOMP(0048A01C, World_Pause48A01C) {
    const u32 o = load<u32>(arg<u32>(ctx, 0) + kWorldMain);
    const u8 a = arg<u8>(ctx, 1);
    if (o == 0 || (load<u32>(o + kObjFlags164) & kObjActive) == 0) {
        ctx.gpr[4] = a;
        return ret(ctx, o);
    }
    call(ctx, kMainPause426AD8, {o, a});
}

// u32 World_Resume48A068(World*, u8 a) (name?): the current world's first
// user object's component 36 gets 0x002549F8 (c, 1), then the scene resume
// 0x0001B2F4 (*0x00F557F0, a); returns its result. The World* argument is
// not read.
DECOMP(0048A068, World_Resume48A068) {
    const u8 a = arg<u8>(ctx, 1);
    const u32 o = load<u32>(load<u32>(kCurrentWorld) + kUserBlocks + 8);
    if (o != 0) {
        const u32 c = static_cast<u32>(call(ctx, hevent16::h16fn::Obj_FindComponent, {o, kCompType36, 0}));
        if (c != 0) call(ctx, kAnimSet2549F8, {c, 1});
    }
    call(ctx, kSceneResume1B2F4, {load<u32>(kSceneHandle), a});
}

// Layer* Anim_GetLayer2541C0(Animator* a, u32 key) (name?): the first of the
// 8 layers whose key matches, or 0.
DECOMP(002541C0, Anim_GetLayer2541C0) {
    const u32 a = arg<u32>(ctx, 0), key = arg<u32>(ctx, 1);
    for (u32 k = 0; k < kAnimNumLayers; ++k) {
        const u32 l = a + kAnimLayers + kAnimLayerSize * k;
        if (load<u32>(l + kLayerKey) == key) return ret(ctx, l);
    }
    ret(ctx, 0u);
}

// u8 Anim_Find298874(Animator* a, u32 key) (name?): whether the layer with
// key (first match) has a layer object that is playing (+20 bit 6).
DECOMP(00298874, Anim_Find298874) {
    const u32 a = arg<u32>(ctx, 0), key = arg<u32>(ctx, 1);
    for (u32 k = 0; k < kAnimNumLayers; ++k) {
        const u32 l = a + kAnimLayers + kAnimLayerSize * k;
        if (load<u32>(l + kLayerKey) != key) continue;
        if (l == 0) break;  // the original tests the (wrapped) entry address
        const u32 p = load<u32>(l + kLayerPtr);
        if (p == 0) break;
        return ret(ctx, (load<u8>(p + kLayerFlags) & kLayerPlaying) != 0 ? 1u : 0u);
    }
    ret(ctx, 0u);
}

// u8 Target_Enable10D9B0(Target* t, u8 on) (name?): on increments the
// enable count (+180) and clears flags bits 14 / 15 (+182); off decrements a
// positive count. Returns count > 0.
DECOMP(0010D9B0, Target_Enable10D9B0) {
    const u32 t = arg<u32>(ctx, 0);
    u32 c = load<u16>(t + kTargetCount);
    if (arg<u8>(ctx, 1) == 0) {
        if (static_cast<s16>(c) > 0) {
            c -= 1;
            store<u16>(t + kTargetCount, static_cast<u16>(c));
        }
    } else {
        const u16 f = load<u16>(t + kTargetFlags);
        c += 1;
        store<u16>(t + kTargetCount, static_cast<u16>(c));
        store<u16>(t + kTargetFlags, static_cast<u16>(f & ~kTargetClearOnEnable));
    }
    const s32 n = static_cast<s16>(c);
    ctx.gpr[4] = static_cast<u64>(static_cast<s64>(n >> 31));  // srawi r4 (no known caller reads it)
    ret(ctx, n > 0 ? 1u : 0u);
}

// u32 Effects_Reset2550C(Effects* e) (name?): +64 / +68 = the owner's
// (+48) vt +60 / vt +64 results; returns the second.
DECOMP(0002550C, Effects_Reset2550C) {
    const u32 e = arg<u32>(ctx, 0);
    u32 o = load<u32>(e + kEffOwner);
    store<u32>(e + kEffA, static_cast<u32>(vcall(ctx, o, kEffVtA, {o})));
    o = load<u32>(e + kEffOwner);
    store<u32>(e + kEffB, static_cast<u32>(vcall(ctx, o, kEffVtB, {o})));
}

// void Obj_Reset12E5E8(Object* o) (name?): sets +968 bit 1 when the value
// at +1168 (when >= 0, else each def entry's +24 threshold) equals 0 for
// one of the def's entries.
DECOMP(0012E5E8, Obj_Reset12E5E8) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 d = load<u32>(o + kObjDef) + kDefEntries;
    const u32 n = load<u32>(d + kDefEntryCount);
    if (n == 0) return;
    const f64 v = hevent7::lfs_f64(o + kObjValue1168);
    const u8 b = load<u8>(o + kObjFlags968);
    const f64 zero = hevent7::lfs_f64(kZeroF12E5E4);
    ctx.fpr[1] = v;  // f1 left holding the value (no known caller reads it)
    for (u32 k = 0; k < n; ++k) {
        const f64 x = v >= zero ? v : hevent7::lfs_f64(d + kDefEntrySize * k + kDefEntryValue);
        if (x == zero) {
            store<u8>(o + kObjFlags968, static_cast<u8>(b | kObjMatched));
            return;
        }
    }
}

// u32 World_SetUser47C168(World* w, u32 user, const char* name, u8 refresh)
// (name?): a non-empty name is set as the user's hashed name, then refresh
// re-targets user 0. Returns name (null / empty), 0 or the refresh's result.
DECOMP(0047C168, World_SetUser47C168) {
    const u32 w = arg<u32>(ctx, 0), user = arg<u32>(ctx, 1), name = arg<u32>(ctx, 2);
    const u32 refresh = arg<u32>(ctx, 3);
    if (name == 0 || load<u8>(name) == 0) return ret(ctx, name);
    call(ctx, h17fn::HashedName_Set, {static_cast<u8>(user), name});
    if (static_cast<u8>(refresh) == 0) return ret(ctx, 0u);
    call(ctx, h17fn::User0_RefreshTarget, {w, user});
}

// u32 Effect_Bind4A6558(Effect** slot, const char* name, const char* set,
// owner, value, u8 flag) (name?): Effect_Bind4A63FC with both names hashed.
DECOMP(004A6558, Effect_Bind4A6558) {
    const u32 slot = arg<u32>(ctx, 0), name = arg<u32>(ctx, 1), set = arg<u32>(ctx, 2);
    const u32 owner = arg<u32>(ctx, 3), value = arg<u32>(ctx, 4);
    const u8 flag = arg<u8>(ctx, 5);
    const u32 h1 = static_cast<u32>(call(ctx, fn::String_HashNoCase, {name}));
    const u32 h2 = static_cast<u32>(call(ctx, fn::String_HashNoCase, {set}));
    call(ctx, h17fn::Effect_Bind4A63FC, {slot, h1, h2, owner, value, flag});
}

// u32 Effect_Bind4A63FC(Effect** slot, u32 name, u32 set, owner, value, u8
// flag) (name?): looks the def up by name hash in the set (0 when the set
// does not exist) and binds it with 0x004A621C.
DECOMP(004A63FC, Effect_Bind4A63FC) {
    const u32 slot = arg<u32>(ctx, 0), name = arg<u32>(ctx, 1), sethash = arg<u32>(ctx, 2);
    const u32 owner = arg<u32>(ctx, 3), value = arg<u32>(ctx, 4);
    const u8 flag = arg<u8>(ctx, 5);
    const u32 set = static_cast<u32>(call(ctx, h17fn::Set_Find, {sethash}));
    if (set == 0) return ret(ctx, 0u);
    const u32 def = hevent6::set_lookup(set, name);
    call(ctx, kEffBind4A621C, {slot, def, owner, value, flag});
}

// Page* Coach_Current2A11D8(Coach* c) (name?): the last of the 8 pages that
// is in use and has a negative index or a table entry (in the object's page
// table at +656, base 0 without one) that is on (bit 7) and not hidden (bit
// 6); 0 for none.
DECOMP(002A11D8, Coach_Current2A11D8) {
    const u32 c = arg<u32>(ctx, 0);
    const u32 o = load<u32>(c + kCoachObj);
    const u32 base = (o != 0 && (load<u32>(o + kObjFlags192) & kObjHasPages) != 0) ? o : 0u;
    const u32 table = base + kObjPageTable;  // read from low memory without an object (quirk)
    u32 cur = 0;
    for (u32 k = 0; k < kNumPages; ++k) {
        if (load<u32>(c + kCoachPages + kPageSize * k) == 0) continue;
        const s32 i = static_cast<s32>(load<u32>(c + kCoachPages + kPageSize * k + kPageIndex));
        if (i >= 0) {
            const u8 b = load<u8>(table + static_cast<u32>(i));
            if ((b & kPageOn) == 0 || (b & kPageHidden) != 0) continue;
        }
        cur = c + kCoachPages + kPageSize * k;
    }
    ret(ctx, cur);
}

// u32 Anim_Reset2980AC(Animator* a, Set* s) (name?): a +36 = s; loads the
// set's layers when its vector (+92) is missing or empty, then copies the
// keys of up to 8 of its entries into the layers. Returns a (no set) or 0.
DECOMP(002980AC, Anim_Reset2980AC) {
    const u32 a = arg<u32>(ctx, 0), s = arg<u32>(ctx, 1);
    store<u32>(a + kAnimSet, s);
    const u32 x = load<u32>(a + kAnimArg28);
    if (s == 0) return ret(ctx, a);
    u32 set = s;
    const u32 v = load<u32>(s + kSetLayers);
    if (v == 0 || load<u16>(v + 8) == 0) {
        call(ctx, kSetLoad351A40, {s, x});
        set = load<u32>(a + kAnimSet);
    }
    for (u32 k = 0;; ++k) {
        const u32 vec = load<u32>(set + kSetLayers);
        const u32 n = vec != 0 ? load<u16>(vec + 8) : 0u;
        if (k >= n || k >= kAnimNumLayers) break;
        const u32 e = load<u32>(load<u32>(vec) + 4 * k);
        store<u32>(a + kAnimLayers + kAnimLayerSize * k + kLayerKey, load<u32>(e + kSetEntryKey));
    }
    ret(ctx, 0u);
}

// World* World_DropPlayer46B6CC(World* w, Player* p) (name?; was
// World_AddPlayer46B6CC): removes every record of p from the 13 player
// lists at +0x128480 (unordered: the last record moves into the hole).
// Returns w + 260 (r3 is the list cursor).
DECOMP(0046B6CC, World_DropPlayer46B6CC) {
    const u32 w = arg<u32>(ctx, 0), p = arg<u32>(ctx, 1);
    for (u32 i = 0; i < kNumPlayerLists; ++i) {
        const u32 l = w + kPlayerListStride * i + kWorldPlayerLists;
        for (s32 j = 0;;) {
            u32 n = load<u16>(l + 8);
            if (j >= static_cast<s32>(n)) break;
            const u32 data = load<u32>(l);
            const u32 e = data + kPlRecSize * static_cast<u32>(j);
            if (load<u32>(e + kPlRecPlayer) == p) {
                n = (n - 1) & 0xFFFF;
                store<u16>(l + 8, static_cast<u16>(n));
                if (static_cast<u32>(j) != n) {
                    const u32 last = data + kPlRecSize * n;
                    for (u32 b = 0; b < kPlRecSize; b += 4) store<u32>(e + b, load<u32>(last + b));
                }
                continue;  // re-check the record moved into j
            }
            ++j;
        }
    }
    ret(ctx, w + kPlayerListStride * kNumPlayerLists);
}

// u32 Effect_Release36A504(EffectSlot* s) (name?): clears +0 / +16 and, with
// an effect at +4, stops it (vt +88 (e, 0, 1, 1, 1)), disables it (vt +40
// (e, 0)), re-enables it when +172 bit 18 is set, then clears +4. Returns 0.
DECOMP(0036A504, Effect_Release36A504) {
    const u32 s = arg<u32>(ctx, 0);
    store<u32>(s + kSlotA, 0);
    store<u32>(s + kSlotB, 0);
    u32 e = load<u32>(s + kSlotEffect);
    if (e == 0) return ret(ctx, 0u);
    vcall(ctx, e, kEffVtStop, {e, 0, 1, 1, 1});
    e = load<u32>(s + kSlotEffect);
    vcall(ctx, e, kEffVtEnable, {e, 0});
    e = load<u32>(s + kSlotEffect);
    if ((load<u32>(e + kEffFlags172) & kEffReenable) != 0) vcall(ctx, e, kEffVtEnable, {e, 1});
    store<u32>(s + kSlotEffect, 0);
    ret(ctx, 0u);
}

// Component* Obj_Attach0D135C(Object* o, Component* tmpl, u8 add) (name?):
// tmpl +8 = o; creates the component (0x002635E8 (o, tmpl, add)) and, with
// add, appends it to o's component vector (+248, created on first use with
// grow step 4). An allocation failure leaves +248 null and the append reads
// through it (quirk).
DECOMP(000D135C, Obj_Attach0D135C) {
    const u32 o = arg<u32>(ctx, 0), tmpl = arg<u32>(ctx, 1), add = arg<u32>(ctx, 2);
    store<u32>(tmpl + kTmplOwner, o);
    const u32 c = static_cast<u32>(call(ctx, kCompCreate2635E8, {o, tmpl, add}));
    if (c == 0 || static_cast<u8>(add) == 0) return ret(ctx, c);
    u32 v = load<u32>(o + kObjComps);
    if (v == 0) {
        v = static_cast<u32>(call(ctx, h17fn::SmallAlloc, {20}));
        store<u32>(o + kObjComps, v);
        if (v != 0) {
            call(ctx, h17fn::PtrVector_Construct, {v});
            store<u16>(v + kVecStep, 4);
            v = load<u32>(o + kObjComps);
        }
    }
    u32 n = load<u16>(v + kVecCount);
    const u32 cap = load<u16>(v + kVecCap);
    if (n == cap) {
        call(ctx, h17fn::PtrVector_Reserve, {v, (cap << 1) & 0xFFFFFFFEu});
        n = load<u16>(v + kVecCount);
    }
    store<u32>(load<u32>(v + kVecData) + (n << 2), c);
    n = load<u16>(v + kVecCount) + 1;
    const u32 hi = load<u16>(v + kVecHigh);
    store<u16>(v + kVecCount, static_cast<u16>(n));
    store<u16>(v + kVecHigh, static_cast<u16>(static_cast<s32>(hi) > static_cast<s32>(n & 0xFFFF) ? hi : n));
    ret(ctx, c);
}

// void Player_ResetMoves120490(Player* p) (name?): with an animator (+164):
// copies def +1408 bit 0 into its flags bit 9, clears bit 10 when +188 bit
// 0, installs the animation callback (0x00F31530, p) and resets the
// animator that vt +60 returns (when non-null) on it; then 0x0011F0E4 (p, -,
// 0; 0.0) and, when 0x000D9474 (p) is non-null, 0x00293264 on its result.
DECOMP(00120490, Player_ResetMoves120490) {
    const u32 p = arg<u32>(ctx, 0);
    const u32 an = load<u32>(p + kPlAnim);
    if (an != 0) {
        const u8 b = load<u8>(load<u32>(p + kPlDef) + kDefFlag1408);
        const u16 f = load<u16>(an + kAnimFlags);
        store<u16>(an + kAnimFlags, static_cast<u16>((f & ~kAnimFlagDef) | ((b & 1u) << 9)));
        if ((load<u32>(p + kPlFlags188) & 1) != 0) {
            const u32 a2 = load<u32>(p + kPlAnim);
            store<u16>(a2 + kAnimFlags, static_cast<u16>(load<u16>(a2 + kAnimFlags) & ~kAnimFlag188));
        }
        const u32 a3 = load<u32>(p + kPlAnim);
        store<u32>(a3 + kAnimCb, kAnimCbOpd);
        store<u32>(a3 + kAnimCbArg, p);
        if (static_cast<u32>(vcall(ctx, p, kPlVtAnimator, {p})) != 0) {
            const u32 x = static_cast<u32>(vcall(ctx, p, kPlVtAnimator, {p}));
            call(ctx, h17fn::Anim_Reset2980AC, {x, load<u32>(p + kPlAnim)});
        }
    }
    // r4 holds the constant's address (the callee reads r3, r5 and f1).
    callf(ctx, kPlSettle11F0E4, {p, kZeroF120488, 0}, {hevent7::lfs_f64(kZeroF120488)});
    if (static_cast<u32>(call(ctx, kPlGet0D9474, {p})) != 0) {
        const u32 x = static_cast<u32>(call(ctx, kPlGet0D9474, {p}));
        call(ctx, kObjReset293264, {x});
    }
}
