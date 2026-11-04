// World_HandleEvent callees, part 12j: the effect def command count used by
// Obj_Register35BD08 ("has no commands").
#include "hevent12.h"

using namespace sly4;
using namespace sly4::hevent6;
using namespace sly4::hevent12;

namespace {
// EffectDef: +16 / +20 command maps, +24 child def map, +28 vector (+8 u16
// count). Each map object holds the engine's map at +12 (hevent6.h layout:
// path +16, depth +96, current +100); command values have a u16 count at +16.
constexpr u32 kDefMapA = 16, kDefMapB = 20, kDefChildren = 24, kDefList = 28, kListCount = 8;
constexpr u32 kMapObjMap = 12, kCmdCount = 16;
constexpr u32 kHasCommands = 0x004B95F0;  // this function (recursive call)

// Sums the u16 at value +16 over the map object's entries, in key order.
s32 sum_counts(u32 mo, s32 total) {
    if (mo == 0) return total;
    const u32 m = mo + kMapObjMap;
    map_first(m);
    for (u32 cur; (cur = load<u32>(m + kMapNode)) != 0;) {
        total = static_cast<s32>(static_cast<u32>(total) + load<u16>(load<u32>(cur + kNodeValue) + kCmdCount));
        map_next(m, cur);
    }
    return total;
}
}  // namespace

// s32 EffectDef_HasCommands4B95F0(EffectDef* d) (name?): the def's command
// count: the +28 vector's count, the +16 / +20 maps' per-entry counts and,
// recursively, the child defs' counts (+24).
DECOMP(004B95F0, EffectDef_HasCommands4B95F0) {
    const u32 d = arg<u32>(ctx, 0);
    const u32 list = load<u32>(d + kDefList);
    const u32 a = load<u32>(d + kDefMapA), b = load<u32>(d + kDefMapB);
    u32 c = load<u32>(d + kDefChildren);
    s32 total = list != 0 ? static_cast<s32>(load<u16>(list + kListCount)) : 0;
    total = sum_counts(a, total);
    total = sum_counts(b, total);
    if (c != 0) map_first(c + kMapObjMap);
    for (;;) {
        if (c == 0) break;
        const u32 cur = load<u32>(c + kMapObjMap + kMapNode);
        if (cur == 0) break;
        total = static_cast<s32>(static_cast<u32>(total) + static_cast<u32>(call(ctx, kHasCommands, {load<u32>(cur + kNodeValue)})));
        c = load<u32>(d + kDefChildren);
        if (c == 0) continue;
        const u32 cur2 = load<u32>(c + kMapObjMap + kMapNode);
        if (cur2 == 0) continue;
        map_next(c + kMapObjMap, cur2);
    }
    ret(ctx, total);
}
