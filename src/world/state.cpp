// World_HasMovieOwner: FlowState_GetMovieOwner (0x00458D6C) as a bool.
#include "world.h"

using namespace sly4;
using namespace sly4::world;

namespace {
constexpr u32 kFlowVfnState = 32;               // (u32 depth) -> state id
constexpr u32 kMovieOwnerFallback = 0x010BBF58;  // u32
}  // namespace

// bool World_HasMovieOwner(World* world)  (name?)
// The same state checks and calls as FlowState_GetMovieOwner, in the same
// order; true when that would return a nonzero object.
DECOMP(00457654, World_HasMovieOwner) {
    const u32 world = arg<u32>(ctx, 0);
    auto state = [&](u32 depth) {
        return static_cast<s32>(call_ptr(ctx, vfunc(world, kFlowVfnState), {world, depth}));
    };
    auto top = [&] { return load<u32>(field<u32>(world, 20) + field<u16>(world, 6)) != 0; };
    auto bottom = [&] { return load<u32>(field<u32>(world, 20)) != 0; };
    auto result = [&](bool v) { ret(ctx, v ? 1u : 0u); };

    if (state(1) == 12) return result(top());
    if (state(0) == 2) return result(bottom());
    if (state(0) == 3) return result(bottom());
    if (state(0) == 0 && state(1) == 6) return result(top());
    if (state(0) == 0) {
        bool check_top = state(1) == 25;
        if (!check_top) check_top = state(1) == 24;
        if (check_top && state(2) > 0) return result(top());
    }
    const u8 sub = field<u8>(world, 31);
    if (sub != 14 && sub != 15) return result(false);
    if (field<u8>(world, 32) == 0) return result(false);
    result(load<u32>(kMovieOwnerFallback) != 0);
}
