// World_HandleEvent callees, part 5: 146 instructions.
#include "hevent.h"

using namespace sly4;
using namespace sly4::hevent;

namespace sly4 {
namespace hevent5c {
namespace cfn {
constexpr u32 memmove = 0x0071C3F0;
constexpr u32 String_CopyN = 0x00553638;        // (dst, src, n)
constexpr u32 StringTable_Lookup = 0x004F87C4;  // (strings, id, fallback, quiet) -> const char*
constexpr u32 sys_ppu_thread_create = 0x00ABF144;  // import stub
constexpr u32 Set_Find = 0x004BCE90;            // (u32 name) -> Set*
constexpr u32 SmallAlloc = 0x0055238C;          // (size) -> block or 0
constexpr u32 Effect_Construct = 0x004C7A68;    // (mem 140 bytes, def)
constexpr u32 Effect_Start = 0x004C753C;        // (Effect*)
constexpr u32 Effect_FindComponent = 0x004BB4B4;  // (Effect*, u32 type hash) -> component or 0 (name?)
}  // namespace cfn

// The save threads (see hevent3_f.cpp).
constexpr u32 kSaveTaskState = 0x00F9D1A8;   // s32: 0 idle, 1 running, 2 ok, 3 failed
constexpr u32 kSaveTaskWord = 0x00F9D1B8;    // u32 cleared on start (name?)
constexpr u32 kSaveTaskFlag = 0x00F9D1C4;    // u8 (name?)
constexpr u32 kSaveBusy = 0x00F64538;        // u8: a save thread is running (name?)
constexpr u32 kSaveTaskBuffer = 0x010127A0;
constexpr u32 kSaveSettingsSize = 36;
constexpr u32 kSaveTitle = 0x00F9D1C5, kSaveSubtitle = 0x00F9D245;  // char[128] each
constexpr u32 kSaveDetails = 0x00F9D2C5;     // char[1024]
constexpr u32 kSaveTaskPrio = 1001, kSaveTaskStack = 16384;
constexpr u32 kSaveSettingsEntry = 0x00F30FC8, kSaveSettingsName = 0x00AC6088;  // "SaveSettings"
constexpr u32 kStrings = 0x00F6687C;         // u32 StringTable* (0 = none)
constexpr u32 kMissingTitle = 0x00AC5EBC;    // "[Missing Title String]"
constexpr u32 kMissingSubtitle = 0x00AC5ED4; // "[Missing Subtitle String]"
constexpr u32 kMissingDetails = 0x00AC5EF4;  // "[Missing Details String]"
constexpr u32 kIdGameName = 0x00AC5F2C, kDefGameName = 0x00AC6008;  // "SYSTEM_GAMENAME", "Sly Cooper Thieves in Time"
constexpr u32 kIdSettings = 0x00AC6028, kDefSettings = 0x00AC6038;  // "SYSTEM_SETTINGS", "Game Settings"
constexpr u32 kIdSettingsInfo = 0x00AC6048, kDefSettingsInfo = 0x00AC6060;  // "SYSTEM_SETTINGSINFO", "Player configuration and preferences."

// Effect sets: Set +16 -> root node {+4 u32 key, +8 def, +12 left (keys below
// the node's), +16 right}; +20 s32[] search path, +100 s16 path length, +104 last node.
constexpr u32 kLastFound = 0x011DE288;       // u32 def of the last lookup
constexpr u32 kComponentOwnerRef = 0x0027469E;  // component type given to Effect_FindComponent (name?)
constexpr u32 kEffectFlags = 136, kEffectAutoStart = 0x200;
}  // namespace hevent5c
}  // namespace sly4

using namespace sly4::hevent5c;

// s32 Save_SaveSettings(const void* settings) (name?): the "SaveSettings"
// thread starter / poll. State 0: state 1, +0x00F9D1B8 = 0, the flags
// 0x00F9D1C4 / kSaveBusy = 1, the 36 bytes copied to the task buffer, the
// title / subtitle / details texts filled from the string table
// (SYSTEM_GAMENAME, SYSTEM_SETTINGS, SYSTEM_SETTINGSINFO, or "[Missing ...]"
// without one), the thread started; returns the state then. 2 and 3 are
// returned once and reset (state 0, flags 0); others are returned as they are.
DECOMP(000AF2E8, Save_SaveSettings) {
    const u32 data = arg<u32>(ctx, 0);
    const s32 s = load<s32>(kSaveTaskState);
    if (s == 2 || s == 3) {
        store<u32>(kSaveTaskState, 0);
        store<u8>(kSaveTaskFlag, 0);
        store<u8>(kSaveBusy, 0);
        ret(ctx, s);
        return;
    }
    if (s != 0) {
        ret(ctx, s);
        return;
    }
    store<u32>(kSaveTaskState, 1);
    store<u32>(kSaveTaskWord, 0);
    store<u8>(kSaveTaskFlag, 1);
    store<u8>(kSaveBusy, 1);
    call(ctx, cfn::memmove, {kSaveTaskBuffer, data, kSaveSettingsSize});
    const u32 strings = load<u32>(kStrings);
    if (strings == 0) {
        call(ctx, cfn::String_CopyN, {kSaveTitle, kMissingTitle, 128});
        call(ctx, cfn::String_CopyN, {kSaveSubtitle, kMissingSubtitle, 128});
        call(ctx, cfn::String_CopyN, {kSaveDetails, kMissingDetails, 1024});
    } else {
        u32 t = static_cast<u32>(call(ctx, cfn::StringTable_Lookup, {strings, kIdGameName, kDefGameName, 0}));
        call(ctx, cfn::String_CopyN, {kSaveTitle, t, 128});
        t = static_cast<u32>(call(ctx, cfn::StringTable_Lookup, {strings, kIdSettings, kDefSettings, 0}));
        call(ctx, cfn::String_CopyN, {kSaveSubtitle, t, 128});
        t = static_cast<u32>(call(ctx, cfn::StringTable_Lookup, {strings, kIdSettingsInfo, kDefSettingsInfo, 0}));
        call(ctx, cfn::String_CopyN, {kSaveDetails, t, 1024});
    }
    Stack frame(ctx, 16);  // sys_ppu_thread_t out at r1+112
    call(ctx, cfn::sys_ppu_thread_create,
         {frame.addr(), kSaveSettingsEntry, 0, kSaveTaskPrio, kSaveTaskStack, 0, kSaveSettingsName});
    ret(ctx, load<s32>(kSaveTaskState));
}

// Effect* Effect_Spawn(u32 name, RefObj* owner, u32 value, u8 start) (name?):
// looks name up in the effect set (Set_Find(0)), recording the search path,
// the node (+104) and its def (0x011DE288); not found -> +104 = the last node
// visited, 0. With a def, builds an Effect (SmallAlloc(140) + Effect_Construct;
// 0 when the allocation fails). With an owner, the effect's 0x0027469E
// component in state 5 (vfunc +32) swaps its +28 reference (vfunc +8 on the
// old one) for owner (owner +4 u16 refcount + 1) and takes +36 = value. start:
// flags +136 |= 0x200 and Effect_Start (on the effect even when it is 0, as
// the original does).
DECOMP(004A5FC8, Effect_Spawn) {
    const u32 name = arg<u32>(ctx, 0);
    const u32 owner = arg<u32>(ctx, 1);
    const u32 value = arg<u32>(ctx, 2);
    const u32 start = arg<u32>(ctx, 3) & 0xFF;
    if (name == 0) {
        ret(ctx, 0);
        return;
    }
    const u32 s = static_cast<u32>(call(ctx, cfn::Set_Find, {0}));
    if (s == 0) {
        ret(ctx, 0);
        return;
    }
    u32 node = load<u32>(load<u32>(s + 16));
    if (node == 0) {
        store<u32>(s + 104, 0);
        store<u32>(kLastFound, 0);
        ret(ctx, 0);
        return;
    }
    store<u16>(s + 100, 0);
    for (;;) {
        const u32 key = load<u32>(node + 4);
        if (key == name) break;
        const u32 next = key > name ? load<u32>(node + 12) : load<u32>(node + 16);
        if (next == 0) {
            store<u32>(s + 104, node);
            ret(ctx, 0);
            return;
        }
        const s32 n = static_cast<s16>(load<u16>(s + 100));
        store<u16>(s + 100, static_cast<u16>(n + 1));
        store<u32>(s + 20 + static_cast<u32>(n) * 4, node);
        node = next;
    }
    store<u32>(s + 104, node);
    store<u32>(kLastFound, load<u32>(node + 8));
    const u32 found = load<u32>(s + 104);
    const u32 def = found != 0 ? load<u32>(found + 8) : 0;
    if (def == 0) {
        ret(ctx, 0);
        return;
    }
    u32 e = 0;
    const u32 mem = static_cast<u32>(call(ctx, cfn::SmallAlloc, {140}));
    if (mem != 0) {
        call(ctx, cfn::Effect_Construct, {mem, def});
        e = mem;
    }
    if (owner != 0) {
        const u32 c = static_cast<u32>(call(ctx, cfn::Effect_FindComponent, {e, kComponentOwnerRef}));
        if (c != 0 && static_cast<s32>(vcall(ctx, c, 32, {c})) == 5) {
            const u32 old = load<u32>(c + 28);
            if (old != 0) vcall(ctx, old, 8, {old});
            store<u32>(c + 28, owner);
            store<u16>(owner + 4, static_cast<u16>(load<u16>(owner + 4) + 1));
            store<u32>(c + 36, value);
        }
    }
    if (start != 0) {
        store<u32>(e + kEffectFlags, load<u32>(e + kEffectFlags) | kEffectAutoStart);
        call(ctx, cfn::Effect_Start, {e});
    }
    ret(ctx, e);
}
