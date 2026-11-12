// Game code, large-function sweep part 12: helpers shared by big12_*.cpp
// (callees part 11 named: the set-entry selectors, the player's setup and
// rate helpers, and further still-recompiled game functions).
#pragma once

#include <bit>
#include <cmath>

#include "big11.h"

namespace sly4 {
namespace big12 {

using namespace opentit;
using namespace opentit::decomp;
using mid1::LfsBits;
using obj19::VCall;
using obj23::LfsAt;
using obj25::Sx;
using big4::CopyW;
using big7::F;
using big7::Fmadds;
using big7::Fmsubs;
using big7::Fnmsubs;
using big7::SetF1Bits;
using big10::StfsW;
using big10::Stfs;
using big11::Adds;
using big11::Subs;
using big11::Muls;
using big11::Divs;

namespace b12fn {
constexpr u32 Set_Find = 0x004BCE90;             // (u32 name) -> effect set
constexpr u32 SmallAlloc = 0x0055238C;           // (size) -> p or 0
constexpr u32 Effect_Construct = 0x004C7A68;     // (Effect* 140 bytes, Def*)
constexpr u32 Effect_Start = 0x004C753C;         // (Effect*)
constexpr u32 Effect_SetTime = 0x004CF710;       // (Effect*; f32)
constexpr u32 Effect_New = 0x004CFB24;           // (u32 hash) -> Effect* or 0
constexpr u32 Effect_FindComponent = 0x004BB4B4; // (Effect*, u32 hash) -> component or 0
constexpr u32 Obj_EnableFirstN = 0x0036E9BC;     // (hud, s32 n)
constexpr u32 acosf = 0x005E649C;                // (f1) -> f1
}  // namespace b12fn

constexpr u32 kSetSelected = 0x011DE288u;   // the selected set entry's def (node +8)
constexpr u32 kHud = 0x00F64CC0u;           // the HUD object
constexpr u32 kWorld = 0x00F65CA0u;         // the world
constexpr u32 kTierEffect = 0x00F6495Cu;    // u32 effect hash per tier

// fsel: b when a >= 0 (a NaN selects c).
inline f64 Fsel(f64 a, f64 b, f64 c) { return a >= 0.0 ? b : c; }
// lfd of a .text / data double.
inline f64 Lfd(u32 ea) { return std::bit_cast<f64>(load<u64>(ea)); }

// The inlined BST select of an effect set (or a sub-set): the tree root
// is *(set + rootOff); equality by cmpw, the left child +12 when the
// node's key is unsigned-greater than hash, else the right +16. Each node
// passed on the way is pushed to the set's path (+pathOff + 4 * the s16
// count at +cntOff, unchecked). Found: +selOff = node and *global = node
// +8; a walk off the tree leaves +selOff = the last node (*global
// untouched; the original tests that node against 0 to zero the count
// first, never true); an empty tree zeroes +selOff and *global. Returns
// the found node or 0.
inline u32 TreeSelect(u32 set, u32 rootOff, u32 cntOff, u32 pathOff, u32 selOff, u32 global, u32 hash) {
    u32 node = load<u32>(load<u32>(set + rootOff));
    if (node == 0) {
        store<u32>(set + selOff, 0);
        store<u32>(global, 0);
        return 0;
    }
    store<u16>(set + cntOff, 0);
    for (;;) {
        const u32 key = load<u32>(node + 4);
        if (key == hash) break;
        const u32 next = load<u32>(node + (key > hash ? 12 : 16));
        if (next == 0) {
            store<u32>(set + selOff, node);
            return 0;
        }
        const s32 n = static_cast<s16>(load<u16>(set + cntOff));
        store<u16>(set + cntOff, static_cast<u16>(n + 1));
        store<u32>(set + pathOff + 4u * static_cast<u32>(n), node);
        node = next;
    }
    store<u32>(set + selOff, node);
    store<u32>(global, load<u32>(node + 8));
    return node;
}
// The global effect set's select (root +16, path +20, count +100,
// selected +104, global kSetSelected).
inline u32 SetSelect(u32 set, u32 hash) { return TreeSelect(set, 16, 100, 20, 104, kSetSelected, hash); }

// Set_Find(0), then SetSelect for hash. Returns the found node (re-read
// from +104) or 0; r3 is what the original's r3 holds at that point (the
// set, or the re-read node).
inline u32 SetSelectEntry(PPUContext& ctx, u32 hash, u32& r3) {
    const u32 set = static_cast<u32>(call(ctx, b12fn::Set_Find, {0}));
    r3 = set;
    if (set == 0) return 0;
    if (SetSelect(set, hash) == 0) return 0;
    r3 = load<u32>(set + 104);
    return r3;
}

// new Effect(def) in 140 bytes of SmallAlloc (0 when the allocation fails).
inline u32 NewEffect(PPUContext& ctx, u32 def) {
    const u32 p = static_cast<u32>(call(ctx, b12fn::SmallAlloc, {140}));
    if (p == 0) return 0;
    call(ctx, b12fn::Effect_Construct, {p, def});
    return p;
}

// Effect_SetTime(e; the .text f32 at k; r4 = k as the original's lfs base
// leaves it), then e->+136 |= 0x200 (unchecked e: a failed allocation
// writes near 0). Returns the new flags (the r3 the original leaves).
inline u32 EffectTimeFlag(PPUContext& ctx, u32 e, u32 k) {
    callf(ctx, b12fn::Effect_SetTime, {e, k}, {LfsAt(k)});
    const u32 f = load<u32>(e + 136) | 0x200u;
    store<u32>(e + 136, f);
    return f;
}

}  // namespace big12
}  // namespace sly4

// ---------------------------------------------------------------- native
#include "native/objtypes.h"

namespace sly4::obj {
void SetSelect_491000(u32, u32 hash);  // big12_game.cpp
}  // namespace sly4::obj

namespace sly4::obj {
// big12_game.cpp / big12_player.cpp
void Hud_ShowBanner36E568(u32, u32 hash, u32 tier);
void Player_SetTierKey16BA60(u32 p);
f64 Vec_SignedAngle16680(u32 a, u32 b);
void Player_Setup232F00(u32 p, u32 q);
void Player_DropLinks2349E0(u32 p);
void Player_Setup231470(u32 p);
void Player_Setup230CA0(u32 p);
void Player_SetRate2_236868(u32 p, u32, f64 speed, f64 spin);
void Player_Setup231928(u32 p);
}  // namespace sly4::obj
