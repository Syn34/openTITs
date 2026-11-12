// Game code, large-function sweep part 34: a path follower's event handler,
// which moves its current node along the path graph.
#include "big34.h"

namespace sly4::obj {

struct PathPoint {
    pad<128> _0;
    Vec3 pos;  // +128
};
// A path graph node (20 bytes): +4 the point object, +10 the next node's
// index, +14 the chain end's index, +16 the random-walk range.
struct PathNode {
    pad<4> _0;
    gp<PathPoint> point;  // +4
    pad<2> _8;
    be_s16 next;          // +10
    pad<2> _12;
    be_s16 end;           // +14
    be_u32 range;         // +16
};
static_assert(sizeof(PathNode) == 20 && offsetof(PathNode, range) == 16);
struct PathGraphHead {
    pad<2> _0;
    be_u16 count;  // +2
};
struct PathGraph {
    pad<4> _0;
    gp<PathGraphHead> head;  // +4
    pad<4> _8;
    be_u32 nodes;            // +12: PathNode[count]
};
// A follower state (20 bytes at the object's +412): +4 the current node,
// +8 the start node.
struct PathState {
    pad<4> _0;
    be_u32 cur;    // +4: a PathNode
    be_u32 start;  // +8
};
struct PathFollower {
    pad<1796> _0;
    be_u32 state;  // +1796: index of the active PathState at +412
    be_u32 event;  // +1800: the event hash
    pad<7> _1804;
    u8 skip;       // +1811
};
static_assert(offsetof(PathFollower, event) == 1800 && offsetof(PathFollower, skip) == 1811);

namespace {
constexpr u32 kPathGraph = 0x00F557F4;    // PathGraph*
constexpr u32 kRandomLast = 0x01164888;   // u64: low 16 bits of the last random number
constexpr u32 kRandomState = 0x01164890;  // u32
constexpr u32 kRandomMix = 0x87654321;
constexpr u32 Obj_GetFocusPos = 0x000137C4;  // (Vec3* out, Obj* o)

// The event hashes.
constexpr u32 kEvNextA = 0x13234CFF, kEvNextB = 0x0A211E20, kEvNextC = 0x75DCAF1F;
constexpr u32 kEvNextUt2 = 0x75742F32, kEvNextUt6 = 0x75742F36;  // "ut/2".."ut/6"
constexpr u32 kEvStepA = 0xDD291809, kEvStepB = 0x194665B8;
constexpr u32 kEvRandom = 0x8FA5D8C3;
constexpr u32 kEvClosest = 0x5E71DA39;
constexpr u32 kEvClear = 0x78348FE9;

// Nodes are handled by guest address: the original reads a null node's
// fields at address 0 + offset without a check.
const PathNode& N(u32 n) { return gm<PathNode>(n); }

// Node i of the graph, or 0 when i is negative or past the count.
u32 Node(s32 i) {
    const PathGraph& g = gm<PathGraph>(global<be_u32>(kPathGraph));  // unchecked
    if (i < 0 || i >= static_cast<s32>(gm<PathGraphHead>(g.head.ea).count)) return 0;
    return g.nodes + 20 * static_cast<u32>(i);
}

// The distance from a node's point to p.
f64 Dist(u32 n, const f64 p[3]) {
    const Vec3& v = gm<PathPoint>(N(n).point.ea).pos;
    const f64 dy = Subs(lf(v.y), p[1]);
    const f64 dx = Subs(lf(v.x), p[0]);
    const f64 dz = Subs(lf(v.z), p[2]);
    return Sqrts(Fmadds(dz, dz, Fmadds(dx, dx, Muls(dy, dy))));
}
}  // namespace

// u32 T::OnPathEvent(T* o) (called through a pointer). The follower state
// s = o + 412 + 20 * o->state reacts to the event hash o->event:
//  - kEvNextA/B/C, "ut/2".."ut/6": unless o->skip is set (then just
//    cleared), s->cur = the node at the current node's next index;
//  - kEvStepA/B: the same, but 0 when there is no current node or it is an
//    end (next == end);
//  - kEvRandom: from the start node, a random number of steps (the game's
//    generator, modulo start->range + 1) along the next links;
//  - kEvClosest: from the start node to the end of its chain, the node
//    nearest Obj_GetFocusPos(o) (strictly nearer wins);
//  - kEvClear: s->cur = 0.
// The result is the original's r3 at each exit (its callers are unknown).
u32 PathFollow_OnEvent_00051B5C(PathFollower* o) {
    const u32 s_ea = guest(o) + 412 + 20 * o->state;
    if (s_ea == 0) return guest(o);
    PathState& s = gm<PathState>(s_ea);
    const s32 ev = static_cast<s32>(o->event.get());
    auto is = [ev](u32 h) { return ev == static_cast<s32>(h); };

    if (is(kEvNextA) || is(kEvNextB) || is(kEvNextC) || (ev > static_cast<s32>(kEvNextUt2) - 1 && ev <= static_cast<s32>(kEvNextUt6))) {
        if (o->skip != 0) {
            o->skip = 0;
            return guest(o);
        }
        const u32 nx = Node(N(s.cur).next);
        s.cur = nx;
        return nx;
    }
    if (is(kEvStepA) || is(kEvStepB)) {
        const u32 n = s.cur;
        if (n == 0 || N(n).next == N(n).end) {
            s.cur = 0u;
            return 0;
        }
        const s32 a = N(n).next;
        const u32 nx = Node(a);
        s.cur = nx;
        // the index scaled to a node offset, or the bare index when out of range
        return nx != 0 ? static_cast<u32>(a) * 20 : static_cast<u32>(a);
    }
    if (is(kEvRandom)) {
        u32 cur = s.start;
        be_u32& state = global<be_u32>(kRandomState);
        const u32 x = state;
        const u32 m = N(cur).range + 1;
        const u32 rem = x - (m != 0 ? x / m : 0) * m;
        s.cur = cur;
        const u32 mix = (std::rotl(x, 4) + std::rotl(x, 16)) ^ kRandomMix;
        state = std::rotl(mix, 16);
        global<be_u64>(kRandomLast) = static_cast<u64>(mix & 0xFFFF);
        for (s32 k = 0; k < static_cast<s32>(rem); ++k) {
            cur = Node(N(cur).next);
            s.cur = cur;
        }
        return cur;
    }
    if (is(kEvClosest)) {
        f64 p[3];
        {
            GuestFrame frame(16);
            Vec3* fp = frame.as<Vec3>();
            call(Obj_GetFocusPos, fp, o);
            p[0] = lf(fp->x);
            p[1] = lf(fp->y);
            p[2] = lf(fp->z);
        }
        u32 cur = s.start;
        s.cur = cur;
        f64 best = Dist(cur, p);
        while (N(cur).next != N(cur).end) {
            const u32 nx = Node(N(cur).next);
            const f64 d = Dist(nx, p);
            if (best > d) {
                best = d;
                s.cur = nx;
            }
            cur = nx;
        }
        return cur;
    }
    if (is(kEvClear)) {
        s.cur = 0u;
        return 0;
    }
    // no case: r3 as the original's compare chain leaves it
    if (ev > 0x194665B7) {
        if (ev > static_cast<s32>(kEvNextUt6)) {
            if (ev > static_cast<s32>(kEvNextC)) return kEvClear;
            return guest(o);
        }
        if (ev > static_cast<s32>(kEvClosest - 1)) return guest(o);
        return kEvStepB;
    }
    return kEvRandom;
}

}  // namespace sly4::obj

NATIVE(00051B5C, sly4::obj::PathFollow_OnEvent_00051B5C)
