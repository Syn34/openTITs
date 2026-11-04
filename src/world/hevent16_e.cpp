// World_HandleEvent callees, part 16e: the player's (re)set onto a new
// definition (11 callers, among them Player_Respawn12C250 and 0x0048CF08).
#include "hevent16.h"

using namespace sly4;
using namespace sly4::hevent;
using namespace sly4::hevent16;
using hevent7::lfs_f64;

namespace {
namespace efn {
constexpr u32 Effects_Reset2550C = 0x0002550C;      // (effects) (name?)
constexpr u32 Query_Init0CDD28 = 0x000CDD28;        // 22 arguments: (Query* out, 4, 128, name, def, 0, "", Mat*, ...; f32, f32) (name?)
constexpr u32 Obj_Attach0D135C = 0x000D135C;        // (Object*, Component*, 1) -> Component* (name?)
constexpr u32 Obj_SetAnimSet0D1B10 = 0x000D1B10;    // (Object*, def +136) (name?)
constexpr u32 Obj_ApplyQuery0D2F04 = 0x000D2F04;    // (Object*, Query*) (name?)
constexpr u32 Obj_SetShadow0E0000 = 0x000E0000;     // (Object*, u32) (name?)
constexpr u32 Player_Settle11F7B8 = 0x0011F7B8;     // (Player*, -, 1; f32 0) (name?)
constexpr u32 Player_ResetState11FCC0 = 0x0011FCC0; // (Player*) (name?)
constexpr u32 Player_Blend11FFF0 = 0x0011FFF0;      // (Player*, -, 1; f32 0) (name?)
constexpr u32 Player_ResetMoves120490 = 0x00120490; // (Player*) (name?)
constexpr u32 Player_ResetInput1205FC = 0x001205FC; // (Player*) (name?)
constexpr u32 Obj_PlayAttachedAnims = 0x001231B8;   // (o, u8 now) (symbols.txt)
constexpr u32 Anim_GetLayer2541C0 = 0x002541C0;     // (Animator*, 0) -> layer or 0 (name?)
constexpr u32 Anim_Stop254814 = 0x00254814;         // (Animator*, 0) (name?)
constexpr u32 Anim_Restart2775A8 = 0x002775A8;      // 12 arguments + f1..f4 (name?)
constexpr u32 Anim_Reset2980AC = 0x002980AC;        // (Animator*, 0) (name?)
constexpr u32 Anim_Find298874 = 0x00298874;         // (Animator*, 0, layer) -> u32 (name?)
constexpr u32 Registry_Notify = 0x002E9BE4;         // (symbols.txt)
constexpr u32 Entry_IsNew2E9B78 = 0x002E9B78;       // (symbols.txt)
constexpr u32 World_AddPlayer46B6CC = 0x0046B6CC;   // (World*, Player*) (name?)
constexpr u32 World_SetUser47C168 = 0x0047C168;     // (World*, s32 user, u32, 1) (name?)
constexpr u32 World_Refresh489EF4 = 0x00489EF4;     // (World*) (name?)
constexpr u32 Nop = 0x0054EC18;
constexpr u32 PtrVector_Clear = 0x0054EC1C;
constexpr u32 PtrVector_Construct = 0x005516E0;
constexpr u32 SmallAlloc = 0x0055238C;
constexpr u32 SmallFree = 0x00552420;
constexpr u32 String_Format = 0x005534D0;           // (fmt, ...) -> rotating static buffer
constexpr u32 Vector_RemoveAt74534C = 0x0074534C;   // (Vector*, s32 i): swap-remove of 240-byte elements
constexpr u32 Singleton_011DF320_Get = 0x007D3B80;
}  // namespace efn

// Player (Object) fields.
constexpr u32 kPMotion = 20, kPDef = 216, kPFlags172 = 172, kPFlags184 = 184, kPUser = 172;
constexpr u32 kPRelease164 = 164, kPWidget160 = 160, kPRelease484 = 484, kPRelease488 = 488;
constexpr u32 kPComponents = 248, kPSlots = 340, kPMarker = 356, kPEntry = 1180;
constexpr u32 kPGrab = 400, kPHang = 404, kPListener388 = 388, kPEffects = 244, kPHooks = 252;
constexpr u32 kPTuning = 496, kPBits726 = 726;
// Motion: +48 def, +52 / +56 picked values, +64 flags (bits 22..29 the anim
// layer, bit 31 the def's +980 set).
constexpr u32 kMDef = 48, kMA = 52, kMB = 56, kMFlags = 64;
// Def: +16 component templates (PtrVector), +52 registry entry, +136 anim
// set, +144 shadow (+8), +228 bits 0..1, +724.. tuning, +776 / +780 /
// +784 / +788 picks, +980 alt.
constexpr u32 kDComponents = 16, kDRegistry = 52, kDAnims = 136, kDShadow = 144, kDBits228 = 228;
constexpr u32 kDTuning = 724, kDPickA = 776, kDPickB = 780, kDAltPickA = 784, kDAltPickB = 788, kDAlt = 980;
constexpr u32 kKindGrab = 2, kKindHang = 1;
// Slot vector (+340): 240-byte entries, +176 bit 27 kept, +192 Object*
// (+200 bit 8 kept).
constexpr u32 kSlotSize = 240, kSlotFlags = 176, kSlotKept = 0x08000000, kSlotObj = 192, kObjFlags200 = 200;

constexpr u32 kFmtControllable = 0x00ACA8DC;  // "Player%02d_Controllable"
constexpr u32 kEmpty = 0x00AC8C70;            // ""
constexpr u32 kZeroVec4 = 0x01164B30;         // (name?)
constexpr u32 kConst124D48 = 0x00124D48;      // f32 -1.0, 0.0, 1.0, 0.25
constexpr u32 kWidgetTag = 0x02235C33;

// Stack (offsets as the original's r1): +112.. outgoing stack arguments,
// +224 the Query (its first word a PtrVector* freed at the end), to +376.
constexpr u32 kFrame = 376, kSpQuery = 224;

// Component kinds the player owns (released / attached by kind).
bool owned_kind(s32 k) {
    switch (k) {
    case 1: case 2: case 3: case 13: case 15: case 16: case 17: case 23: case 36: case 37: case 45: return true;
    default: return false;
    }
}
void copy32(u32 dst, u32 src) { store<u32>(dst, load<u32>(src)); }
}  // namespace

// u32 Player_SetSave124D58(Player* o, Def* d, u8 quick) (name?): moves the
// player onto definition d. quick: only re-picks the motion's values (+48..
// +64) from d and the old def. Otherwise tears down the animator, widgets,
// owned components and unkept slot objects, re-registers with the world,
// rebuilds through a "Player%02d_Controllable" query, re-attaches d's
// component templates (remembering the grab / hang ones at +400 / +404),
// copies d's tuning and restarts the anims.
DECOMP(00124D58, Player_SetSave124D58) {
    const u32 o = arg<u32>(ctx, 0), d = arg<u32>(ctx, 1);
    const u8 quick = arg<u8>(ctx, 2);
    const u32 old = load<u32>(o + kPDef);
    Stack frame(ctx, kFrame);
    const u32 sp = frame.addr();
    call(ctx, efn::Obj_PlayAttachedAnims, {o, 0});

    if (quick != 0) {
        const u32 layer = static_cast<u32>(vcall(ctx, o, 32, {o, 0}));
        vcall(ctx, o, 28, {o, 0, 5});
        const u32 m = load<u32>(o + kPMotion);
        u32 f = load<u32>(m + kMFlags);
        store<u32>(m + kMDef, d);
        f = (f & ~0x3FC00000u) | ((layer << 22) & 0x3FC00000u);  // rlwimi r3, r28, 22, 2, 9
        store<u32>(m + kMFlags, f);
        const u32 alt = load<u32>(d + kDAlt) != 0 ? 1u : 0u;
        store<u32>(m + kMFlags, (f & 0x7FFFFFFFu) | (alt << 31));
        const u32 a = load<u32>(old + kDAlt) != 0 ? load<u32>(old + kDAltPickA) : load<u32>(d + kDPickA);
        store<u32>(m + kMA, a);
        const u32 dalt = load<u32>(d + kDAlt);
        store<u32>(m + kMB, dalt != 0 ? load<u32>(d + kDPickB) : load<u32>(old + kDAltPickB));
        return ret(ctx, dalt);  // r3 is left with d +980
    }

    u32 layer_obj = 0, found = 0;  // r29, r28
    const u32 an = static_cast<u32>(vcall(ctx, o, 60, {o}));
    if (an != 0) {
        const u32 l = static_cast<u32>(call(ctx, efn::Anim_GetLayer2541C0, {an, 0}));
        if (l != 0) {
            const u32 x = load<u32>(l + 4);
            if (x != 0) {
                layer_obj = load<u32>(x + 8);
                found = static_cast<u32>(call(ctx, efn::Anim_Find298874, {an, 0, x}));
            }
        }
        call(ctx, efn::Anim_Stop254814, {an, 0});
        call(ctx, efn::Anim_Reset2980AC, {an, 0});
    }
    if (const u32 x = load<u32>(o + kPRelease164); x != 0) {
        vcall(ctx, x, 4, {x});
        store<u32>(o + kPRelease164, 0);
    }
    if (const u32 x = load<u32>(o + kPWidget160); x != 0) {
        vcall(ctx, x, 40, {x, 0, 1});
        const u32 w = load<u32>(o + kPWidget160);
        const u32 vt = load<u32>(w);
        store<u32>(w + 276, kWidgetTag);
        store<u32>(w + 280, 0);
        const u32 opd = load<u32>(vt + 8);
        hevent7::opdcall(ctx, opd, {w});
        store<u32>(o + kPWidget160, 0);
    }
    for (const u32 off : {kPRelease484, kPRelease488}) {
        const u32 x = load<u32>(o + off);
        if (x == 0) continue;
        vcall(ctx, x, 8, {x});
        store<u32>(o + off, 0);
    }
    // Release the owned components, swap-removing each (the slot is looked at
    // again after a move).
    for (s32 i = 0;; ++i) {
        const u32 lst = load<u32>(o + kPComponents);
        const s32 n = lst != 0 ? static_cast<s32>(load<u16>(lst + 8)) : 0;
        if (!(i < n)) break;
        const u32 e = load<u32>(load<u32>(lst) + 4 * static_cast<u32>(i));
        if (e == 0 || !owned_kind(load<s32>(e + 12))) continue;
        vcall(ctx, e, 4, {e});
        const u32 l2 = load<u32>(o + kPComponents);
        const u32 c = static_cast<u32>(load<u16>(l2 + 8)) - 1;
        store<u16>(l2 + 8, static_cast<u16>(c));
        if (i != static_cast<s32>(c & 0xFFFF)) {
            const u32 arr = load<u32>(l2);
            store<u32>(arr + 4 * static_cast<u32>(i), load<u32>(arr + ((c << 2) & 0x3FFFC)));
        }
        --i;
    }

    u32 slots = load<u32>(o + kPSlots);
    store<u32>(o + kPGrab, 0);
    store<u32>(o + kPHang, 0);
    if (slots != 0) {
        // Unkept slot objects (from the back): hide, disable, remove.
        for (s32 i = static_cast<s32>(load<u16>(slots + 8)) - 1; i >= 0; --i) {
            const u32 ent = load<u32>(slots) + static_cast<u32>(i) * kSlotSize;
            const u32 so = load<u32>(ent + kSlotObj);
            if (so == 0 || (load<u32>(ent + kSlotFlags) & kSlotKept) != 0 || (load<u32>(so + kObjFlags200) & 0x100) != 0) continue;
            vcall(ctx, so, 184, {so, 0});
            vcall(ctx, so, 40, {so, 1});
            call(ctx, efn::Vector_RemoveAt74534C, {load<u32>(o + kPSlots), static_cast<u64>(static_cast<s64>(i))});
            slots = load<u32>(o + kPSlots);
        }
    }

    call(ctx, efn::World_AddPlayer46B6CC, {load<u32>(kCurrentWorld), o});
    if (const u32 mk = load<u32>(o + kPMarker); mk != 0) {
        store<u32>(mk + 136, load<u32>(mk + 136) | 0x200);
        store<u32>(o + kPMarker, 0);
    }
    call(ctx, efn::Registry_Notify, {load<u32>(d + kDRegistry)});
    if (const u32 en = load<u32>(o + kPEntry); en != 0) {
        if (static_cast<u32>(call(ctx, efn::Entry_IsNew2E9B78, {en})) != 0) {
            const u32 s = static_cast<u32>(call(ctx, efn::Singleton_011DF320_Get));
            vcall(ctx, s, 64, {s});
        }
        store<u32>(o + kPEntry, 0);
    }

    // The "Player%02d_Controllable" query.
    const s32 user = static_cast<s8>(load<u8>(o + kPUser));
    store<u32>(o + kPDef, d);
    const u32 ub = load<u32>(kCurrentWorld) + static_cast<u32>(user) * kUserBlockSize + kUserBlocks;
    const u32 name = static_cast<u32>(call(ctx, efn::String_Format, {kFmtControllable, load<u8>(ub + 1) + 1u, ub}));
    const f64 m1 = lfs_f64(kConst124D48);
    // Arguments 20 and 21 (r1 +200 / +208) are never stored by the original
    // and never read by the callee.
    callf(ctx, efn::Query_Init0CDD28,
          {sp + kSpQuery, 4, 128, name, d, 0, kEmpty, hevent13::kIdentity, kZeroVec4, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0xFFFFFFFFu, 0, 0, 0},
          {m1, m1});
    call(ctx, efn::Obj_ApplyQuery0D2F04, {o, sp + kSpQuery});
    {
        const u32 bits = load<u32>(d + kDBits228);
        const u32 f172 = load<u32>(o + kPFlags172);
        const u64 f184 = load<u64>(o + kPFlags184);
        store<u32>(o + kPFlags172, (f172 & ~0x00C00000u) | ((bits << 22) & 0x00C00000u));  // rlwimi r4, r3, 22, 8, 9
        store<u64>(o + kPFlags184, f184 | 1);
    }
    // Attach the def's component templates.
    for (u32 i = 0;; ++i) {
        const u32 lst = load<u32>(load<u32>(o + kPDef) + kDComponents);
        const u32 n = lst != 0 ? load<u16>(lst + 8) : 0u;
        if (i >= n) break;
        const u32 e = load<u32>(load<u32>(lst) + 4 * i);
        if (e == 0 || !owned_kind(load<s32>(e + 12))) continue;
        store<u32>(e + 8, o);
        const u32 c = static_cast<u32>(call(ctx, efn::Obj_Attach0D135C, {o, e, 1}));
        store<u32>(e + 8, 0);
        const s32 k = load<s32>(e + 12);
        if (k == static_cast<s32>(kKindGrab))
            store<u32>(o + kPGrab, c != 0 && load<s32>(c + 12) == static_cast<s32>(kKindGrab) ? c : 0u);
        else if (k == static_cast<s32>(kKindHang))
            store<u32>(o + kPHang, c != 0 && load<s32>(c + 12) == static_cast<s32>(kKindHang) ? c : 0u);
    }
    // Tell every component (no null check) and the +388 listener.
    for (u32 i = 0;; ++i) {
        const u32 lst = load<u32>(o + kPComponents);
        const u32 n = lst != 0 ? load<u16>(lst + 8) : 0u;
        if (i >= n) break;
        const u32 e = load<u32>(load<u32>(lst) + 4 * i);
        vcall(ctx, e, 20, {e, o});
    }
    if (const u32 x = load<u32>(o + kPListener388); x != 0) vcall(ctx, x, 20, {x, o});

    call(ctx, efn::Player_ResetMoves120490, {o});
    if (layer_obj != 0) {
        const u32 an2 = static_cast<u32>(vcall(ctx, o, 60, {o}));
        if (an2 != 0) {
            const u64 r8 = ctx.gpr[8];  // never set by the original: what the vcall left
            const f64 f0 = lfs_f64(kConst124D48 + 4);
            // Arguments 9 / 10 (r1 +112 / +120) still hold the query call's
            // values; the callee doesn't read them.
            callf(ctx, efn::Anim_Restart2775A8,
                  {an2, layer_obj, 0, found, ~0ull, r8, 0, 0, kZeroVec4, 1, 0, ~0ull},
                  {lfs_f64(kConst124D48 + 8), lfs_f64(kConst124D48 + 12), f0, f0});
        }
    }
    call(ctx, efn::Player_ResetInput1205FC, {o});
    call(ctx, efn::Player_ResetState11FCC0, {o});
    // Tuning: d +724.. -> o +496.. (8 words, 2 bytes, 3 words), in the
    // original's store order.
    for (u32 k = 0; k < 32; k += 4) copy32(o + kPTuning + k, d + kDTuning + k);
    store<u8>(o + kPTuning + 36, load<u8>(d + kDTuning + 36));
    copy32(o + kPTuning + 32, d + kDTuning + 32);
    copy32(o + kPTuning + 40, d + kDTuning + 40);
    store<u8>(o + kPTuning + 37, load<u8>(d + kDTuning + 37));
    copy32(o + kPTuning + 44, d + kDTuning + 44);
    ctx.fpr[1] = lfs_f64(d + kDTuning + 44);
    call(ctx, efn::Obj_SetAnimSet0D1B10, {o, load<u32>(o + kPDef) + kDAnims});
    call(ctx, efn::World_Refresh489EF4, {load<u32>(kCurrentWorld)});
    if (const u32 sh = load<u32>(d + kDShadow); sh != 0) call(ctx, efn::Obj_SetShadow0E0000, {o, load<u32>(sh + 8)});
    if (const u32 fx = load<u32>(o + kPEffects); fx != 0) call(ctx, efn::Effects_Reset2550C, {fx});

    {
        const u32 def = load<u32>(o + kPDef);
        const u32 w = load<u32>(kCurrentWorld);
        const u32 u = static_cast<u32>(static_cast<s32>(static_cast<s8>(load<u8>(o + kPUser))));
        const u32 v = static_cast<u32>(vcall(ctx, def, 8, {def}));
        call(ctx, efn::World_SetUser47C168, {w, u, v, 1});
    }
    // The hook vector (+252), created on first use (no check of a failed
    // allocation: the count is then read from address 8).
    u32 hooks = load<u32>(o + kPHooks);
    if (hooks == 0) {
        hooks = static_cast<u32>(call(ctx, efn::SmallAlloc, {20}));
        store<u32>(o + kPHooks, hooks);
        if (hooks != 0) {
            call(ctx, efn::PtrVector_Construct, {hooks});
            store<u16>(hooks + 6, 4);
            hooks = load<u32>(o + kPHooks);
        }
    }
    for (u32 i = 0; i < load<u16>(hooks + 8); ++i) {
        const u32 h = load<u32>(load<u32>(hooks) + 4 * i);
        if (h != 0) vcall(ctx, h, 292, {h, o});
    }

    if (load<u32>(d + kDAlt) != 0) store<u64>(o + kPFlags184, load<u64>(o + kPFlags184) | 0x20);
    else store<u64>(o + kPFlags184, load<u64>(o + kPFlags184) & ~0x20ull);
    const f64 zero = lfs_f64(kConst124D48 + 4);
    callf(ctx, efn::Player_Blend11FFF0, {o, kConst124D48, 1}, {zero});  // r4 still the constant's address
    callf(ctx, efn::Player_Settle11F7B8, {o, ctx.gpr[4], 1}, {zero});   // r4 as the previous call left it
    call(ctx, efn::Obj_PlayAttachedAnims, {o, 1});
    u64 r3 = vcall(ctx, o, 32, {o, 0});
    if (static_cast<u32>(r3) == 0) {
        r3 = vcall(ctx, o, 32, {o, 1});
        if (static_cast<u32>(r3) == 15) {
            const u8 b = load<u8>(o + kPBits726);
            if ((b & 0x80) == 0 || (b & 0x40) != 0) r3 = vcall(ctx, o, 28, {o, 1, 0});
        }
    }
    if (const u32 q = load<u32>(sp + kSpQuery); q != 0) {
        call(ctx, efn::PtrVector_Clear, {q});
        call(ctx, efn::Nop, {q});
        r3 = call(ctx, efn::SmallFree, {load<u32>(sp + kSpQuery), 20});
    }
    ctx.gpr[3] = r3;
}
