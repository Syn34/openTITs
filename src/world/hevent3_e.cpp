// World_HandleEvent callees, part 3: 81 instructions.
#include "hevent.h"

using namespace sly4;
using namespace sly4::hevent;

namespace sly4 {
namespace hevent3e {
namespace efn {
constexpr u32 sub_000DD02C = 0x000DD02C;     // (Object*, 0, 0)
constexpr u32 SmallAlloc = 0x0055238C;
}  // namespace efn

constexpr u32 kWorldPostFx = 0x1281E0;       // u32 object (wupdate.h)
constexpr u32 kWorldLevel = 0x128118;        // u32: the level hash given to World_StartLevel (name?)

// Timelines (ui/ui.h, uielem/text_slots.h).
constexpr u32 kTimelineDefs = 0x00F6689C;    // u32 TimelineDef*: +36 name hash, +48 first key (+4 f32 delay), +52 next
constexpr u32 kUiTimelines = 0x00F668A0;     // u32 Timeline*, linked by +28
constexpr u32 kTimelineVtbl = 0x00B228A0;
constexpr u32 kTimelineSize = 32;
constexpr u32 kTlDef = 4, kTlOwned = 8, kTl12 = 12, kTlOwns = 13, kTlKey = 16, kTlDelay = 20, kTlTracks = 24, kTlNext = 28;
constexpr u32 kZero4E2EEC = 0x004E2EEC;      // f32 0.0
}  // namespace hevent3e
}  // namespace sly4

using namespace sly4::hevent3e;

// u8 World_StartLevel(World* w, u32 level, u8 flag): nothing for level 0.
// Otherwise sub_000DD02C(+0x1281E0, 0, 0) when set, +0x128118 = level, and the
// state machine is set up: with no level-0 state yet (ids[0] == 255) +48 = 1 and
// SetState(0, 1); otherwise +48 = 1, +49 = 0, +50 = 1, +56 = 0. Then SetState(1, 0)
// unless the phase +50 is 1..11 (then +49 = 0), and SetState(2, 0); +47 mirrors
// ids[0] along the way. Finally +44 bit 0x2 = flag, +45 |= 0x20, +46 |= 0xC0;
// returns the new +44 (name?)
DECOMP(004674C4, World_StartLevel) {
    const u32 w = arg<u32>(ctx, 0), level = arg<u32>(ctx, 1);
    const u64 flag = ctx.gpr[5];
    if (level == 0) return;  // r3 = w
    const u32 fx = load<u32>(w + kWorldPostFx);
    if (fx != 0) call(ctx, efn::sub_000DD02C, {fx, 0, 0});
    u32 ids = load<u32>(w + world::kSmIds);
    store<u32>(w + kWorldLevel, level);
    const u32 id0 = load<u8>(ids);
    store<u8>(w + 47, static_cast<u8>(id0));
    u32 phase;
    if (id0 == world::kSmNone) {
        store<u8>(w + 48, 1);
        store<u8>(w + 47, 1);
        call(ctx, world::vfn::StateMachine_SetState, {w, 0, 1});
        ids = load<u32>(w + world::kSmIds);
        phase = load<u8>(w + 50);
    } else {
        store<u8>(w + 48, 1);
        store<u8>(w + 49, 0);
        store<u8>(w + 50, 1);
        store<u32>(w + 56, 0);
        phase = 1;
    }
    store<u8>(w + 47, load<u8>(ids));
    if (static_cast<s32>(phase) < 12 && phase != 0) {
        store<u8>(w + 49, 0);
    } else {
        call(ctx, world::vfn::StateMachine_SetState, {w, 1, 0});
        ids = load<u32>(w + world::kSmIds);
    }
    store<u8>(w + 47, load<u8>(ids));
    call(ctx, world::vfn::StateMachine_SetState, {w, 2, 0});
    const u32 b44 = (load<u8>(w + 44) & ~0x02u) | ((static_cast<u32>(flag) & 1u) << 1);
    const u32 b45 = load<u8>(w + 45) | 0x20u;
    const u32 b46 = load<u8>(w + 46) | 0xC0u;
    store<u8>(w + 44, static_cast<u8>(b44));
    store<u8>(w + 45, static_cast<u8>(b45));
    store<u8>(w + 46, static_cast<u8>(b46));
    ctx.gpr[4] = b45;
    ctx.gpr[5] = b46;
    ret(ctx, b44);
}

// Timeline* Timeline_Play(const char* name, Element* target, u8 a, u8 owns):
// finds the TimelineDef whose hash (+36) is String_HashNoCase(name) and
// appends a new Timeline {vtable, def, target, a, owns, key = def +48, delay =
// key +4 (0.0 without a key)} to the list at 0x00F668A0. 0 when there is no
// such def or the allocation fails (name?)
DECOMP(004E2EF0, Timeline_Play) {
    const u32 target = arg<u32>(ctx, 1);
    const u32 a = arg<u32>(ctx, 2), owns = arg<u32>(ctx, 3);
    const u32 h = static_cast<u32>(call(ctx, hfn::String_HashNoCase, {arg<u32>(ctx, 0)}));
    u32 def = load<u32>(kTimelineDefs);
    while (def != 0 && load<u32>(def + 36) != h) def = load<u32>(def + 52);
    if (def == 0) return ret(ctx, 0u);
    const u32 t = static_cast<u32>(call(ctx, efn::SmallAlloc, {kTimelineSize}));
    if (t == 0) return ret(ctx, 0u);
    store<u32>(t + kTlOwned, target);
    store<u32>(t + 0, kTimelineVtbl);
    store<u32>(t + kTlKey, 0);
    store<u32>(t + kTlTracks, 0);
    store<u32>(t + kTlDef, def);
    store<u32>(t + kTlNext, 0);
    u32 last = load<u32>(kUiTimelines);
    store<u8>(t + kTl12, static_cast<u8>(a));
    store<u8>(t + kTlOwns, static_cast<u8>(owns));
    store<u32>(t + kTlDelay, 0);
    if (last != 0) {
        while (load<u32>(last + kTlNext) != 0) last = load<u32>(last + kTlNext);
        store<u32>(last + kTlNext, t);
        def = load<u32>(t + kTlDef);
    } else {
        store<u32>(kUiTimelines, t);
    }
    const u32 key = load<u32>(def + 48);
    store<u32>(t + kTlKey, key);
    const u32 src = key != 0 ? key + 4 : kZero4E2EEC;
    ctx.fpr[1] = static_cast<f64>(load<f32>(src));  // f1 as the original leaves it
    store<u32>(t + kTlDelay, load<u32>(src));
    ret(ctx, t);
}
