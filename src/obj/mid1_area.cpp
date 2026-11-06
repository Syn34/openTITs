// Game code, medium sweep part 1: an area object (the classes around
// 0x0020Fxxx..0x00225xxx) taking an actor in and letting it go: the actor
// list +344 (a PtrVector: +344 data, +348 capacity, +352 count, +354 mark),
// two sorted maps keyed by the actor's +168 (+364 with the count +366, +388
// with the count +390), the occupancy counter fed through vfn[8] / vfn[7],
// and the world events of kinds 4 / 5 / 31.
#include "mid1.h"

using namespace sly4;
using namespace sly4::mid1;

namespace m1a {
constexpr u32 kAreaDebug = 0x01056A08;          // u8: both objects' vfn[19] run first
constexpr u32 SortedMap_Find = 0x00759654;      // (map, const u32* key) -> Node** slot
constexpr u32 SortedMap_Remove = 0x007597DC;    // (map, const u32* key)
constexpr u32 Obj_Detach_0012E8B0 = 0x0012E8B0; // (node, area) when node->+192 has 0x8 (name?)
constexpr u32 EventInit_Enter = 0x0046502C;     // (ev, src, 0, 0): kind 4
constexpr u32 EventInit_Leave = 0x004650A8;     // (ev, src, 0, 0): kind 5
constexpr u32 EventInit_Full = 0x00465124;      // (ev, src, 0, 0): kind 31
constexpr u32 World_Query482D14 = 0x00482D14;   // (world, user, const Record*)
}  // namespace m1a

namespace {
// vfn[7](o, 0, v) as the original calls it: the entry in r6 and the OPD in
// r7 (the reverse of the usual order).
void VCall28(PPUContext& ctx, u32 o, u32 v) {
    const u32 opd = load<u32>(load<u32>(o) + 28);
    call_ptr(ctx, opd, {o, 0, v, load<u32>(opd), opd});
}

// The common start of Area_Enter / Area_Leave: (debug) vfn[19] on both;
// the actor's entries under its key +168 dropped from the maps +364 (the
// node's 0x0012E8B0(node, area) when its +192 has 0x8, then its vfn[10](0))
// and +388 (its vfn[10](0)). sp + 112 / + 116 are the original's key slots.
void AreaDropKeys(PPUContext& ctx, u32 area, u32 actor, u32 sp) {
    if (load<u8>(m1a::kAreaDebug) != 0) {
        VCall(ctx, area, 76, {area});
        VCall(ctx, actor, 76, {actor});
    }
    u32 key = load<u32>(actor + 168);
    store<u32>(sp + 116, key);
    if (load<u16>(area + 366) != 0) {
        const u32 node = load<u32>(static_cast<u32>(call(ctx, m1a::SortedMap_Find, {area + 364, sp + 116})));
        if (node != 0) {
            if (load<u32>(node + 192) & 0x8u) call(ctx, m1a::Obj_Detach_0012E8B0, {node, area});
            VCall(ctx, node, 40, {node, 0});
        }
        call(ctx, m1a::SortedMap_Remove, {area + 364, sp + 116});
        key = load<u32>(actor + 168);
    }
    store<u32>(sp + 112, key);
    if (load<u16>(area + 390) != 0) {
        const u32 x = load<u32>(static_cast<u32>(call(ctx, m1a::SortedMap_Find, {area + 388, sp + 112})));
        if (x != 0) VCall(ctx, x, 40, {x, 0});
        call(ctx, m1a::SortedMap_Remove, {area + 388, sp + 112});
    }
}

// A zeroed 24-byte event at ev, built by init(ev, area, 0, 0).
void AreaEvent(PPUContext& ctx, u32 ev, u32 init, u32 area) {
    for (u32 k = 0; k < 24; k += 4) store<u32>(ev + k, 0);
    call(ctx, init, {ev, area, 0, 0});
}

// The world user an area event goes to: 0 when the area's +412 has
// 0x20000000, else the actor's s8 +172.
u32 AreaUser(u32 area, u32 actor) {
    return (load<u32>(area + 412) & 0x20000000u) ? 0u : static_cast<u32>(static_cast<s32>(static_cast<s8>(load<u8>(actor + 172))));
}
}  // namespace

// void Area_Enter(Area* area, Actor* actor): AreaDropKeys, the actor pushed
// onto +344, vfn[24](1), the actor's u16 +178 (its area count) incremented,
// vfn[7](0, vfn[8](0) + 1). When the area's +412 has 0x20000000 or the
// actor's +184 has bit 0 (unless such an actor with +192 bit 0x2 declines
// through its vfn[80]), the world gets an enter event (kind 4) and, when the
// area now holds as many actors as the world's player count (+29), a kind
// 31 event. Then the actor's vfn[57](area).
DECOMP(0020E7A4, Area_Enter) {
    const u32 area = arg<u32>(ctx, 0), actor = arg<u32>(ctx, 1);
    Stack frame(ctx, 112);
    const u32 sp = frame.addr() - 112;
    AreaDropKeys(ctx, area, actor, sp);
    {
        const u32 v = area + 344;
        u32 c = load<u16>(v + 8);
        const u32 cap = load<u16>(v + 4);
        if (c == cap) {
            call(ctx, m1fn::PtrVector_Reserve, {v, (cap << 1) & 0x1FFFEu});
            c = load<u16>(v + 8);
        }
        store<u32>(load<u32>(v) + ((c << 2) & 0x3FFFCu), actor);
        const u32 n = load<u16>(v + 8) + 1u;
        const u32 m = load<u16>(v + 10);
        const u32 vt = load<u32>(area);  // read before the count is stored
        store<u16>(v + 8, static_cast<u16>(n));
        store<u16>(v + 10, static_cast<u16>(static_cast<s32>(m) > static_cast<s32>(n & 0xFFFFu) ? m : n));
        obj25::VCallVt(ctx, vt, 96, {area, 1});
    }
    store<u16>(actor + 178, static_cast<u16>(load<u16>(actor + 178) + 1));
    const u32 r = static_cast<u32>(VCall(ctx, area, 32, {area, 0}));
    VCall28(ctx, area, (r + 1) & 0xFFu);
    const bool quiet = (load<u32>(area + 412) & 0x20000000u) != 0;
    const u32 avt = load<u32>(actor);
    bool post = quiet || (load<u32>(actor + 188) & 1u);
    if (post && (load<u32>(actor + 188) & 1u) && actor != 0 && (load<u32>(actor + 192) & 0x2u)) {
        const u32 opd = load<u32>(avt + 320);
        post = call_ptr(ctx, opd, {actor, opd, load<u32>(opd)}) == 0;
    }
    if (post) {
        AreaEvent(ctx, sp + 120, m1a::EventInit_Enter, area);
        const u32 user = AreaUser(area, actor);
        call(ctx, m1a::World_Query482D14, {load<u32>(kWorldPtr), user, sp + 120});
        if (load<u16>(area + 344 + 8) == load<u8>(load<u32>(kWorldPtr) + 29)) {
            AreaEvent(ctx, sp + 152, m1a::EventInit_Full, area);
            call(ctx, m1a::World_Query482D14,
                 {load<u32>(kWorldPtr), static_cast<u32>(static_cast<s32>(static_cast<s8>(load<u8>(actor + 172)))), sp + 152});
        }
    }
    VCall(ctx, actor, 228, {actor, area});
}

// void Area_Leave(Area* area, Actor* actor): AreaDropKeys; when the actor
// is on +344: its vfn[58](area) and vfn[10](0), swap-removed, vfn[24](any
// left), vfn[7](0, vfn[8](0) - 1), and, when the area's +412 has
// 0x20000000 or the actor's +184 has bit 0, a leave event (kind 5) to the
// world. r3 = 4 * the count when the actor is not on the list.
DECOMP(0020EBC8, Area_Leave) {
    const u32 area = arg<u32>(ctx, 0), actor = arg<u32>(ctx, 1);
    Stack frame(ctx, 112);
    const u32 sp = frame.addr() - 112;
    AreaDropKeys(ctx, area, actor, sp);
    s32 i = 0;
    for (;; ++i) {
        if (!(i < static_cast<s32>(load<u16>(area + 352)))) {
            ret(ctx, 4u * static_cast<u32>(i));
            return;
        }
        if (load<u32>(load<u32>(area + 344) + 4 * static_cast<u32>(i)) == actor) break;
    }
    VCall(ctx, actor, 232, {actor, area});
    VCall(ctx, actor, 40, {actor, 0});
    u32 c = load<u16>(area + 352) - 1u;
    store<u16>(area + 352, static_cast<u16>(c));
    if (static_cast<u32>(i) != (c & 0xFFFFu)) {
        const u32 data = load<u32>(area + 344);
        store<u32>(data + (static_cast<u32>(i) << 2), load<u32>(data + ((c << 2) & 0x3FFFCu)));
        c = load<u16>(area + 352);
    }
    VCall(ctx, area, 96, {area, (c & 0xFFFFu) != 0 ? 1u : 0u});
    const u32 r = static_cast<u32>(VCall(ctx, area, 32, {area, 0}));
    VCall28(ctx, area, (r - 1) & 0xFFu);
    const bool quiet = (load<u32>(area + 412) & 0x20000000u) != 0;
    if (!quiet && !(load<u32>(actor + 188) & 1u)) {
        ret(ctx, 0u);
        return;
    }
    AreaEvent(ctx, sp + 120, m1a::EventInit_Leave, area);
    const u32 user = AreaUser(area, actor);
    call(ctx, m1a::World_Query482D14, {load<u32>(kWorldPtr), user, sp + 120});
}
