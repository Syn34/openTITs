// Game code, large-function sweep part 34: a path follower's event handler,
// which moves its current node along the path graph.
#include "../sly4.h"
#include "big34.h"

using namespace sly4;
using namespace sly4::big34;

namespace {
constexpr u32 kPathGraph = 0x00F557F4;    // u32: the graph (+4 -> u16 +2 node count, +12 the 20-byte nodes)
constexpr u32 kRandomLast = 0x01164888;   // u64: low 16 bits of the last random number
constexpr u32 kRandomState = 0x01164890;  // u32
constexpr u32 kRandomMix = 0x87654321;
constexpr u32 Obj_GetFocusPos = 0x000137C4;  // (Vec3* out, Obj* o)

// The event hashes (o+1800).
constexpr u32 kEvNextA = 0x13234CFF, kEvNextB = 0x0A211E20, kEvNextC = 0x75DCAF1F;
constexpr u32 kEvNextUt2 = 0x75742F32, kEvNextUt6 = 0x75742F36;  // "ut/2".."ut/6"
constexpr u32 kEvStepA = 0xDD291809, kEvStepB = 0x194665B8;
constexpr u32 kEvRandom = 0x8FA5D8C3;
constexpr u32 kEvClosest = 0x5E71DA39;
constexpr u32 kEvClear = 0x78348FE9;

// Node i of the graph, or 0 when i is negative or past the count.
u32 Node(s32 i) {
    const u32 g = load<u32>(kPathGraph);
    if (i < 0 || i >= static_cast<s32>(load<u16>(load<u32>(g + 4) + 2))) return 0;
    return load<u32>(g + 12) + 20 * static_cast<u32>(i);
}
s32 NextIdx(u32 n) { return static_cast<s16>(load<u16>(n + 10)); }
s32 EndIdx(u32 n) { return static_cast<s16>(load<u16>(n + 14)); }

// The distance from a node's position (its +4 object's +128) to p.
f64 Dist(u32 n, const f64 p[3]) {
    const u32 v = load<u32>(n + 4) + 128;
    const f64 dy = Subs(Lfs(v + 4), p[1]);
    const f64 dx = Subs(Lfs(v + 0), p[0]);
    const f64 dz = Subs(Lfs(v + 8), p[2]);
    return Sqrts(Fmadds(dz, dz, Fmadds(dx, dx, Muls(dy, dy))));
}
}  // namespace

// u32 T::OnPathEvent(T* o) (called through a pointer). The follower state
// s = o + 412 + 20 * o->1796 (+4 the current node, +8 the start node) reacts
// to the event hash o->1800:
//  - kEvNextA/B/C, "ut/2".."ut/6": unless the skip byte o->1811 is set
//    (then just cleared), s->4 = the node at the current node's +10 index;
//  - kEvStepA/B: the same, but 0 when there is no current node or it is an
//    end (its +10 index equals +14);
//  - kEvRandom: from the start node, a random number of steps (the game's
//    generator, modulo start->16 + 1) along the +10 links;
//  - kEvClosest: from the start node to the end of its chain, the node
//    nearest Obj_GetFocusPos(o) (strictly nearer wins; f1 is left as that
//    distance);
//  - kEvClear: s->4 = 0.
// The original's r3 at each exit is kept (see the returns).
DECOMP(00051B5C, PathFollow_OnEvent_00051B5C) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 s = o + 412 + 20 * load<u32>(o + 1796);
    if (s == 0) return;  // r3 = o
    const s32 ev = static_cast<s32>(load<u32>(o + 1800));
    auto is = [ev](u32 h) { return ev == static_cast<s32>(h); };

    if (is(kEvNextA) || is(kEvNextB) || is(kEvNextC) || (ev > static_cast<s32>(kEvNextUt2) - 1 && ev <= static_cast<s32>(kEvNextUt6))) {
        if (load<u8>(o + 1811) != 0) {
            store<u8>(o + 1811, 0);
            return;  // r3 = o
        }
        const u32 nx = Node(NextIdx(load<u32>(s + 4)));
        store<u32>(s + 4, nx);
        return ret(ctx, nx);
    }
    if (is(kEvStepA) || is(kEvStepB)) {
        const u32 n = load<u32>(s + 4);
        if (n == 0 || NextIdx(n) == EndIdx(n)) {
            store<u32>(s + 4, 0);
            return ret(ctx, 0u);
        }
        const s32 a = NextIdx(n);
        const u32 nx = Node(a);
        store<u32>(s + 4, nx);
        // r3: the index scaled to a node offset, or the bare index when out of range
        return ret(ctx, nx != 0 ? static_cast<s32>(static_cast<u32>(a) * 20) : a);
    }
    if (is(kEvRandom)) {
        u32 cur = load<u32>(s + 8);
        const u32 x = load<u32>(kRandomState);
        const u32 m = load<u32>(cur + 16) + 1;
        const u32 rem = x - (m != 0 ? x / m : 0) * m;
        store<u32>(s + 4, cur);
        const u32 mix = (std::rotl(x, 4) + std::rotl(x, 16)) ^ kRandomMix;
        store<u32>(kRandomState, std::rotl(mix, 16));
        store<u64>(kRandomLast, mix & 0xFFFF);
        for (s32 k = 0; k < static_cast<s32>(rem); ++k) {
            cur = Node(NextIdx(cur));
            store<u32>(s + 4, cur);
        }
        return ret(ctx, cur);
    }
    if (is(kEvClosest)) {
        f64 p[3];
        {
            Stack frame(ctx, 16);
            call(ctx, Obj_GetFocusPos, {frame.addr(), o});
            for (u32 k = 0; k < 3; ++k) p[k] = Lfs(frame.addr() + 4 * k);
        }
        u32 cur = load<u32>(s + 8);
        store<u32>(s + 4, cur);
        f64 best = Dist(cur, p);
        while (NextIdx(cur) != EndIdx(cur)) {
            const u32 nx = Node(NextIdx(cur));
            const f64 d = Dist(nx, p);
            if (best > d) {
                best = d;
                store<u32>(s + 4, nx);
            }
            cur = nx;
        }
        ctx.fpr[1] = best;
        return ret(ctx, cur);
    }
    if (is(kEvClear)) {
        store<u32>(s + 4, 0);
        return ret(ctx, 0u);
    }
    // no case: r3 as the original's compare chain leaves it
    if (ev > 0x194665B7) {
        if (ev > static_cast<s32>(kEvNextUt6)) {
            if (ev > static_cast<s32>(kEvNextC)) return ret(ctx, kEvClear);
            return;  // r3 = o
        }
        if (ev > static_cast<s32>(kEvClosest - 1)) return;  // r3 = o
        return ret(ctx, kEvStepB);
    }
    ret(ctx, kEvRandom);
}
