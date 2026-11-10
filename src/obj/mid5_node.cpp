// Game code, medium sweep part 5: action-script node parameter binders.
#include "mid5.h"

using namespace sly4;
using namespace sly4::mid5;

namespace {

constexpr u32 kStrAnimEvent = 0x00AEE320;  // "AnimEvent"

}  // namespace

// bool f(Owner* o): the parameters of an AnimClip play node bound on o
// (Name, Class, ChannelMap, Loop, BlendIn, ScaleExtMotion, Speed,
// PlayNode, ForceSyncAnimPos, StartTime, TranAnimClipName; BlendType with
// the names Smooth / Linear / Slow; SyncType with the game's sync-type
// names), then Owner_AddName(o, "AnimEvent", 3); returns its result.
// The sync-type name of an index outside [0, Game->+268) is read from
// address 0, as the original.
DECOMP(004B2DA0, AnimClipNode_BindParams_004B2DA0) {
    const u32 o = arg<u32>(ctx, 0);
    Bind(ctx, o, 0x00AEE480, 5, 0);  // "Name"
    Bind(ctx, o, 0x00AEE53C, 2, 1);  // "Class"
    Bind(ctx, o, 0x00AEE544, 2, 1);  // "ChannelMap"
    Bind(ctx, o, 0x00AEE554, 4, 1);  // "Loop"
    Bind(ctx, o, 0x00AEE55C, 3, 1);  // "BlendIn"
    Bind(ctx, o, 0x00AEE564, 4, 1);  // "ScaleExtMotion"
    Bind(ctx, o, 0x00AEE574, 3, 1);  // "Speed"
    Bind(ctx, o, 0x00AEE57C, 5, 1);  // "PlayNode"
    Bind(ctx, o, 0x00AEE58C, 4, 1);  // "ForceSyncAnimPos"
    Bind(ctx, o, 0x00AEE5A4, 3, 1);  // "StartTime"
    Bind(ctx, o, 0x00AEE5B4, 5, 1);  // "TranAnimClipName"
    const u32 bt = Bind(ctx, o, 0x00AEE5CC, 2, 1);  // "BlendType"
    call(ctx, m5fn::Binding_Reserve, {bt, 3});
    NodeBindingAddName(ctx, bt, 0x00AEE5DC);  // "Smooth"
    NodeBindingAddName(ctx, bt, 0x00AEE5E4);  // "Linear"
    NodeBindingAddName(ctx, bt, 0x00AEE5EC);  // "Slow"
    const u32 st = Bind(ctx, o, 0x00AEE5F4, 2, 1);  // "SyncType"
    call(ctx, m5fn::Binding_Reserve, {st, load<u32>(load<u32>(kGame) + 276)});
    for (u32 i = 0;; i = (i + 1) & 0xFFFFu) {
        const u32 g = load<u32>(kGame);
        if (!(i < load<u32>(g + 276))) break;
        const s32 si = static_cast<s16>(i);
        const u32 e = (si >= 0 && si < static_cast<s32>(load<u32>(g + 268))) ? load<u32>(g + 272) + (static_cast<u32>(si) << 3) : 0u;
        NodeBindingAddName(ctx, st, load<u32>(e));
    }
    ret(ctx, call(ctx, m5fn::Owner_AddName, {o, kStrAnimEvent, 3}));
}
