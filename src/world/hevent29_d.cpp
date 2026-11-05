// World_HandleEvent callees, part 29d: the session's level start (users
// placed at their entries, the current part and its episode's masks,
// spawner checks, the loader reset and the level's start objects).
#include "hevent29.h"

using namespace sly4;
using namespace sly4::hevent;
using namespace sly4::hevent29;

namespace {
namespace fn29d {
constexpr u32 World_SpawnUsers = 0x00487670;  // (World*, Entry*) (World_SpawnUsers487670)
constexpr u32 Loader_Enter = 0x002DE41C;      // (Session*, UserBlock*, Entry*) (Loader_Enter2DE41C)
constexpr u32 Loader_Leave = 0x002DE794;      // (Session*, Entry*, UserBlock*) (Loader_Leave2DE794)
constexpr u32 SortedMap_Find = 0x0073B608;    // (map, const u32* key) -> value slot (SortedMap_Find73B608)
constexpr u32 String_Format = 0x005534D0;     // (fmt, ...)
constexpr u32 List_Notify = 0x00456500;       // (Owner*, s32 key) (List_Notify456500)
constexpr u32 Mask_CheckSpawner = 0x002DF010;  // (Session*, Spawner*, const u16* masks) (Mask_CheckSpawner2DF010)
constexpr u32 Loader_Reset = 0x002DEDB4;      // (Session*, u8 restart, u8 restore) (Loader_Reset2DEDB4)
constexpr u32 Entry_Index = 0x00325EBC;       // (Entry*, Object*) -> s32 (Entry_Index325EBC)
constexpr u32 Obj_Release = 0x0012BF70;       // (Object*) (Obj_Release12BF70)
constexpr u32 Obj_Snap = 0x001662F4;          // (Object*) (Obj_Snap1662F4)
constexpr u32 Handle_ResolveBit1 = 0x003568E0;  // (handle) -> Object*
constexpr u32 Save_Value = 0x00465D5C;        // (const u8* user) -> s32 (Save_Value465D5C)
constexpr u32 Obj_SetAnim = 0x00363A3C;       // (o, name, now, -, b, c, d; f32)
constexpr u32 Obj_FindById = 0x000D6A28;      // (u32 id) -> Object*
constexpr u32 Target_GetDef = 0x000D951C;     // (Object*) -> TargetDef* (Target_GetDef0D951C)
constexpr u32 Effect_SetHash = 0x004CF988;    // (Effect*, u32 hash) -> bool (Effect_SetHash4CF988)
constexpr u32 Obj_SetStateFx = 0x00266E6C;    // (Object*, s32 state) (Obj_SetStateFx266E6C)
constexpr u32 Mask_ShowCount = 0x002DE258;    // (Session*, u8 complete) (Mask_ShowCount2DE258)
constexpr u32 FindFlagged = 0x00368A48;       // (u32 key) -> Object* (FindFlagged_00368A48)
constexpr u32 Obj_EnterMode = 0x003689D8;     // (Obj*) (Obj_EnterMode3689D8)
constexpr u32 Hash_ToString = 0x0056A6E0;     // (u32) -> "0x%x"
}  // namespace fn29d

// Session fields.
constexpr u32 kSessParts = 20, kSessPartCount = 24;  // Part[860]
constexpr u32 kSessPart = 36;                        // Part*: the current part, or 0
constexpr u32 kSessEffectFlag = 40;                  // nonzero: the 0xE07DD018 object's effect is set
constexpr u32 kSessTargetDef = 64, kSessTarget = 68;
constexpr u32 kSessEntry = 72;                       // Entry*: +20 bit 0x20 = spawn users; +2500 / +2524 per-user bit sets
constexpr u32 kPartSize = 860;
// Part fields.
constexpr u32 kPartLevels = 748;     // u32[+820 count]: level ids
constexpr u32 kPartLevelCount = 820;
constexpr u32 kPartEpisodeSlot = 822;  // u8: index of its u16 state (save +496) and mask byte (save +228)
constexpr u32 kPartEpisode = 823;      // u8: episode index, 255 for none
constexpr u32 kPartSkip = 826;         // u8
constexpr u32 kPartLevel = 740;        // u32: level id
constexpr u32 kEpisodes = 12;
constexpr u32 kWorldLevel = 0x00128114;  // World +: u32 current level id
// World / user block / save slot fields.
constexpr u32 kWUserCount = 29;       // u8
constexpr u32 kUserEntries = 80;      // Entry* list, u16 count at +88; entry +18 == 5 is skipped
constexpr u32 kUserObj = 8;           // Object* of user block 0
constexpr u32 kUserIndex = 0, kUserAltIndex = 2;  // u8: save slot / fallback index
constexpr u32 kWFlags = 4144;         // bit 0x02000000: enter the flagged object's mode
constexpr u32 kSaveEpisodeMasks = 132;  // u32[12] bit sets per episode
constexpr u32 kSaveSpawnerMasks = 180;  // u16 masks for Mask_CheckSpawner
constexpr u32 kSaveStartMasks = 228;    // u8 per episode slot
constexpr u32 kSaveEpisodeState = 496;  // u16 per episode slot, bits 0..2
// Globals.
constexpr u32 kLevelInfo = 0x00F65BE8;   // +28: map hash -> object
constexpr u32 kEpisodeKey = 0xFE71E1BB;  // map key: episode list owner (name?)
constexpr u32 kSpawnerKey = 0xCE33CC21;  // map key: mask spawner list (+32, next +72) (name?)
constexpr u32 kEpisodeCb = 0x00F35F68;   // OPD stored in the owner +56 (name?)
constexpr u32 kTargetCbA = 0x00F35F70, kTargetCbB = 0x00F35F78;  // OPDs stored in the target def +72 / +80 (name?)
constexpr u32 kMaskTotal = 0x00F64C24;   // u32
constexpr u32 kHandleB = 0x00F64CDC;     // u32 handle of the save value display
constexpr u32 kFlaggedKey = 0x00F64D3C;  // u32
constexpr u32 kHighlightBusy = 0x0106C330 + 132;  // u8
constexpr u32 kStartObjs = 0x010681CC;   // {+2 u16 count, +8 {u32 id, const u8* bit}[]} (name?)
constexpr u32 kStartBusy = 0x010683D8 + 72;  // u8 (name?)
constexpr u32 kTargetMode = 0x010682A8 + 72;  // s32 (name?)
constexpr u32 kFmtEpisode = 0x00B1D308;  // "Episode Index is out of bounds '%i'! Range is [0 - %i]! ..."
constexpr u32 kFmtValue = 0x00AD8B78;    // format of the save value text
constexpr u32 kAnimNow = 0x002DF190;     // f32 (also passed in r6, unused)
constexpr u32 kAnimArg = 0x01164CA4;
constexpr u32 kTargetId = 0x8624AF0B, kEffectObjId = 0xE07DD018;
constexpr u32 kEffectHash = 0xA772696E, kStartHash = 0x3032DD97;

// slw: shifts of 32..63 give 0.
u32 slw(u32 v, u32 n) { return (n & 0x20) != 0 ? 0 : v << (n & 31); }

u32 cur_world() { return load<u32>(kCurrentWorld); }
u32 save_slot(u32 w) { return w + load<u8>(w + kUserBlocks) * kSaveSlotSize; }
}  // namespace

// u32 Session_StartLevel2DF198(Session* s) (name?):
//  - every user block: users spawned when the entry (+72) has bit 0x20,
//    then entered / left at its first entry not of kind 5 (or 0);
//  - the part: the current one (+36), else the first part not skipped
//    whose level list holds the world's level;
//  - its episode (+823, when < 12, else an error string is formatted):
//    the owner (map key 0xFE71E1BB) gets s / 0x00F35F68 at +60 / +56 and
//    is notified for every mask (up to *0x00F64C24) not set in the save's
//    episode bits;
//  - the mask spawners checked (map key 0xCE33CC21), and when the part is
//    the current level, the level's start objects whose save bit is set
//    (and 0x01068420 clear) get effect 0x3032DD97 (missing ones logged
//    with Hash_ToString);
//  - the loader reset, user 0's object released / snapped / vt +28 reset
//    by the entry's per-user bit sets, the save value shown, the target
//    (0x8624AF0B) def hooked and its state (from 0x010682F0 or the save)
//    set, the 0xE07DD018 object's effect set, the mask count shown and
//    the flagged object's mode entered.
// r3: Obj_EnterMode3689D8's result, or 0.
DECOMP(002DF198, Session_StartLevel2DF198) {
    const u32 s = arg<u32>(ctx, 0);
    Stack frame(ctx, 16);  // +0 / +4 map keys

    // Users.
    u32 w;
    for (u32 i = 0, off = 0; i < load<u8>((w = cur_world()) + kWUserCount); ++i, off += kUserBlockSize) {
        const u32 ub = w + off + kUserBlocks;
        const u32 e = load<u32>(s + kSessEntry);
        if (e != 0) {
            const u32 bit = load<u32>(e + 20) & 0x20u;
            if (bit != 0) call(ctx, fn29d::World_SpawnUsers, {w, e, bit});
        }
        u32 entry = 0;
        for (u32 j = 0; j < load<u16>(ub + kUserEntries + 8); ++j) {
            const u32 en = load<u32>(load<u32>(ub + kUserEntries) + 4 * j);
            if (load<u8>(en + 18) != 5) {
                entry = en;
                break;
            }
        }
        call(ctx, fn29d::Loader_Enter, {s, ub, entry});
        call(ctx, fn29d::Loader_Leave, {s, entry, ub});
    }
    const u32 ub0 = w + kUserBlocks;
    u32 save = save_slot(w);

    // The part.
    u32 part = load<u32>(s + kSessPart);
    if (part == 0) {
        const u32 n = load<u32>(s + kSessPartCount);
        const u32 level = load<u32>(w + kWorldLevel);
        for (u32 i = 0; i < n && part == 0; ++i) {
            const u32 p = load<u32>(s + kSessParts) + i * kPartSize;
            if (load<u8>(p + kPartSkip) != 0) continue;
            const u32 m = load<u8>(p + kPartLevelCount);
            for (u32 k = 0; k < m; ++k)
                if (load<u32>(p + kPartLevels + 4 * k) == level) {
                    part = p;
                    break;
                }
        }
    }

    // The episode's masks.
    const u32 key1 = frame.addr() + 4, key2 = frame.addr();
    store<u32>(key1, kEpisodeKey);
    const u32 owner = load<u32>(static_cast<u32>(call(ctx, fn29d::SortedMap_Find, {load<u32>(kLevelInfo) + 28, key1})));
    if (owner != 0 && part != 0) {
        const u32 ep = load<u8>(part + kPartEpisode);
        if (ep != 255) {
            store<u32>(owner + 60, s);
            store<u32>(owner + 56, kEpisodeCb);
            if (ep < kEpisodes) {
                const u32 bits = save + kSaveEpisodeMasks + 4 * ep;
                u32 total = load<u32>(kMaskTotal);
                if (total != 0) {
                    u32 k = 0;
                    do {
                        if (k >= load<u32>(owner + 36)) break;
                        if ((load<u32>(bits) & slw(1, k)) == 0) {
                            call(ctx, fn29d::List_Notify, {owner, static_cast<u64>(static_cast<s64>(static_cast<s32>(k)))});
                            total = load<u32>(kMaskTotal);
                        }
                        ++k;
                    } while (total > k);
                }
            } else {
                call(ctx, fn29d::String_Format, {kFmtEpisode, ep, kEpisodes});
            }
        }
    }

    // Mask spawners, then the level's start objects.
    store<u32>(key2, kSpawnerKey);
    u32 list = load<u32>(static_cast<u32>(call(ctx, fn29d::SortedMap_Find, {load<u32>(kLevelInfo) + 28, key2})));
    if (list == 0 || part != 0) {
        if (list != 0)
            for (u32 sp = load<u32>(list + 32); sp != 0; sp = load<u32>(sp + 72))
                call(ctx, fn29d::Mask_CheckSpawner, {s, sp, save + kSaveSpawnerMasks});
        u32 lv;
        if (part != 0 && (lv = load<u32>(part + kPartLevel)) != 0 && lv == load<u32>(cur_world() + kWorldLevel)) {
            const u32 mask = save + load<u8>(part + kPartEpisodeSlot) + kSaveStartMasks;
            for (s32 i = 0; i < static_cast<s32>(load<u16>(kStartObjs + 2)); ++i) {
                const u32 tab = load<u32>(kStartObjs + 8);
                const u8 b = load<u8>(mask);
                bool on = false;
                if ((b & slw(1, load<u8>(load<u32>(tab + 8 * i + 4)))) != 0 && load<u8>(kStartBusy) == 0) on = true;
                const u32 id = load<u32>(tab + 8 * i);
                const u32 o = static_cast<u32>(call(ctx, fn29d::Obj_FindById, {id}));
                if (o == 0) {
                    call(ctx, fn29d::Hash_ToString, {id});
                } else if (on) {
                    const u32 fx = static_cast<u32>(vcall(ctx, o, 272, {o}));
                    if (fx != 0) call(ctx, fn29d::Effect_SetHash, {fx, kStartHash});
                }
            }
        }
    }

    // The loader, then user 0's object by the entry's bit sets.
    call(ctx, fn29d::Loader_Reset, {s, 1, 0});
    const u32 obj = load<u32>(ub0 + kUserObj);
    u32 entry;
    if (obj != 0 && (load<u32>(obj + 192) & 8u) != 0 && (entry = load<u32>(s + kSessEntry)) != 0) {
        const s32 idx = static_cast<s32>(call(ctx, fn29d::Entry_Index, {entry, obj}));
        const u32 e = load<u32>(s + kSessEntry);
        const u32 i1 = idx == -1 ? load<u8>(ub0 + kUserAltIndex) : static_cast<u32>(idx);
        if ((load<u8>(load<u32>(e + 2500) + (i1 >> 3)) & slw(1, i1 & 7)) != 0) {
            call(ctx, fn29d::Obj_Release, {obj});
        } else {
            const u32 i2 = idx == -1 ? load<u8>(ub0 + kUserAltIndex) : static_cast<u32>(idx);
            if ((load<u8>(load<u32>(e + 2524) + (i2 >> 3)) & slw(1, i2 & 7)) != 0) {
                call(ctx, fn29d::Obj_Snap, {obj});
            } else if (static_cast<u32>(vcall(ctx, obj, 32, {obj, 0})) != 0 || static_cast<s32>(vcall(ctx, obj, 32, {obj, 1})) == 10) {
                vcall(ctx, obj, 28, {obj, 0, 0});
                vcall(ctx, obj, 28, {obj, 1, 0});
            }
        }
    }

    // The save value display.
    if (load<u8>(kHighlightBusy) == 0) {
        const u32 o = static_cast<u32>(call(ctx, fn29d::Handle_ResolveBit1, {load<u32>(kHandleB)}));
        if (o != 0) {
            const u32 v = static_cast<u32>(call(ctx, fn29d::Save_Value, {cur_world() + kUserBlocks}));
            const u32 txt = static_cast<u32>(call(ctx, fn29d::String_Format, {kFmtValue, v}));
            callf(ctx, fn29d::Obj_SetAnim, {o, txt, 1, kAnimNow, 0, 0, kAnimArg}, {lfs_at(kAnimNow)});
        }
    }

    // The target's def and its state.
    store<u32>(s + kSessTargetDef, 0);
    u32 t = static_cast<u32>(call(ctx, fn29d::Obj_FindById, {kTargetId}));
    if (t != 0) {
        const u32 def = static_cast<u32>(call(ctx, fn29d::Target_GetDef, {t}));
        store<u32>(s + kSessTargetDef, def);
        if (def != 0) {
            store<u32>(def + 76, s);
            store<u32>(def + 72, kTargetCbA);
            store<u32>(def + 80, kTargetCbB);
            store<u32>(def + 84, s);
            const s32 mode = load<s32>(kTargetMode);
            bool set = true;
            u32 state = 0;
            if (mode <= 0) {
                if (part == 0) {
                    set = false;
                } else {
                    w = cur_world();
                    const u32 st = w + load<u8>(w + kUserBlocks) * kSaveSlotSize + 4 * load<u8>(part + kPartEpisodeSlot) + kSaveEpisodeState;
                    if ((load<u16>(st) & 7u) == 3) state = 3;
                }
            } else if (mode != 1) {
                state = 3;
            }
            if (set) {
                if (state == 3) {
                    store<u32>(def + 124, load<u32>(def + 124) | 0x00200000u);
                    call(ctx, fn29d::Obj_SetStateFx, {def, 3, def});
                } else {
                    call(ctx, fn29d::Obj_SetStateFx, {def, 0});
                }
                if (part != 0) {
                    w = cur_world();
                    const u32 st = w + load<u8>(w + kUserBlocks) * kSaveSlotSize + 4 * load<u8>(part + kPartEpisodeSlot) + kSaveEpisodeState;
                    const u32 v = load<u32>(load<u32>(s + kSessTargetDef) + 68);
                    store<u16>(st, static_cast<u16>((load<u16>(st) & ~7u) | (v & 7u)));
                }
            }
        }
    }

    // The effect object.
    store<u32>(s + kSessTarget, 0);
    t = static_cast<u32>(call(ctx, fn29d::Obj_FindById, {kEffectObjId}));
    const u32 want = load<u32>(s + kSessEffectFlag);
    if (t != 0) {
        const u32 o = (load<u32>(t + 192 + 8) & 2u) != 0 ? t : 0;
        store<u32>(s + kSessTarget, o);
        if (o != 0 && want != 0) {
            const u32 fx = static_cast<u32>(vcall(ctx, o, 272, {o}));
            if (fx != 0) call(ctx, fn29d::Effect_SetHash, {fx, kEffectHash});
        }
    }

    call(ctx, fn29d::Mask_ShowCount, {s, 0});
    const u32 f = static_cast<u32>(call(ctx, fn29d::FindFlagged, {load<u32>(kFlaggedKey)}));
    if (f == 0) return;  // r3 = 0
    store<u32>(f + 532, load<u32>(f + 532) & ~0x02000000u);
    ctx.gpr[3] = 0;
    if ((load<u32>(cur_world() + kWFlags) & 0x02000000u) != 0) call(ctx, fn29d::Obj_EnterMode, {f});
}
