// Game code, part 27: helpers shared by obj27*.cpp (the continued sweep of
// small still-recompiled game functions, in size order, from 36
// instructions up).
#pragma once

#include <bit>
#include <cmath>

#include "../game/update.h"
#include "obj26.h"

namespace sly4 {
namespace obj27 {

using namespace opentit;
using namespace opentit::decomp;
using namespace sly4::obj26;
using namespace sly4::obj25;
using namespace sly4::obj24;
using namespace sly4::obj23;
using namespace sly4::obj19;

namespace o27fn {
constexpr u32 Obj_InitVt_B20CC8 = 0x0042C9D0;   // (o, bool on): the 0x00B20CC8 base constructor
constexpr u32 Flag_Set8_40000000 = 0x0042CA60;  // (o): o->+8 |= 0x40000000
constexpr u32 Hash_ToString = 0x0056A6E0;       // (hash) -> char*
constexpr u32 String_Format = 0x005534D0;       // (fmt, ...)
constexpr u32 PtrVector_Reserve = 0x00551718;   // (vec, capacity)
constexpr u32 Effect_SetHash = 0x004CF988;      // (effect, hash)
constexpr u32 SortedMap_Find = 0x00749A14;      // (map, const u32* key) -> value slot
}  // namespace o27fn

// A script command record (vtable vt) whose base is 0x0042C9D0(o, on).
inline void CmdBase27(PPUContext& ctx, u32 o, u32 on) { call(ctx, o27fn::Obj_InitVt_B20CC8, {o, on & 0xFFu}); }

// Reports a missing hash: String_Format(fmt, Hash_ToString(h)).
inline void WarnHash27(PPUContext& ctx, u32 fmt, u32 h) {
    const u32 s = static_cast<u32>(call(ctx, o27fn::Hash_ToString, {h}));
    call(ctx, o27fn::String_Format, {fmt, s});
}

}  // namespace obj27
}  // namespace sly4
