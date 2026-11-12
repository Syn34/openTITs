// Game code, large-function sweep part 34: a contact listener's pair
// callback, which pairs up the two bodies' owners or records the lone one.
#include "../sly4.h"
#include "big34.h"

using namespace sly4;
using namespace sly4::big34;

namespace {
constexpr u32 kCompHash = 0x91C7CF5B;  // the component kind looked up in each owner's table
constexpr u32 kFlagPairable = 0x00040000;  // +172 bit: the object may be recorded
constexpr u32 PairVec_AddUnique = 0x003A95F4;  // (o, x, v)
constexpr u32 Array_Reserve = 0x00558F3C;      // (array, u16 capacity)
constexpr u32 SortedMap_Find = 0x00759654;     // (map, const u32* key) -> entry*

// The topmost body: follow +12 while it is non-zero.
u32 Root(u32 b) {
    while (load<u32>(b + 12) != 0) b = load<u32>(b + 12);
    return b;
}

// The +12 word of the owner's kCompHash entry (16-byte entries at +124,
// count +128; both reloaded per step), or 0. A root whose kind byte +24 is
// not 1 has no owner, and its table is read from address 0 (unchecked).
u32 FindComp(u32 root) {
    const u32 owner = static_cast<s8>(load<u8>(root + 24)) == 1 ? root + static_cast<s8>(load<u8>(root + 16)) : 0;
    for (s32 i = 0; i < static_cast<s32>(load<u32>(owner + 128)); ++i) {
        const u32 e = load<u32>(owner + 124) + 16 * static_cast<u32>(i);
        if (load<u32>(e) == kCompHash) return load<u32>(e + 12);
    }
    return 0;
}

struct Side {
    u32 obj;
    bool group;   // obj+216 is the listener's group (+224)
    bool focus;   // obj is the listener's +372
    bool listed;  // obj+216 is one of the listener's three groups at +160
};

Side Classify(u32 o, u32 x) {
    Side s{x, false, false, false};
    if (x == 0) return s;
    const u32 g = load<u32>(x + 216);
    s.group = g == load<u32>(o + 224);
    s.focus = x == load<u32>(o + 372);
    if (g != 0)
        for (u32 k = 0; k < 3; ++k)
            if (load<u32>(o + 160 + 4 * k) == g) { s.listed = true; break; }
    return s;
}
}  // namespace

// bool T::OnPair(T* o, Pair* p) (through AdjThunk_003A9C1C, this - 56).
// The roots of the bodies p+0 and p+4 lead to two objects (FindComp). When
// exactly one of them is in o's group (+224): if either one's group is
// listed at o+160..168, PairVec_AddUnique(o, the grouped one, the other);
// otherwise, unless either is o+372 or both exist, the grouped one (when it
// has kFlagPairable and a key +168 not yet in the sorted u16 vector at o+116:
// capacity +116, count +118, high-water +120, flags +122, data +124) is
// appended there as {key, object}, doubling the capacity when full, and
// +122 gets bit 0. Always sets p+32 = 1 and returns 1.
DECOMP(003A96F8, ContactListener_OnPair_003A96F8) {
    const u32 o = arg<u32>(ctx, 0), p = arg<u32>(ctx, 1);
    const u32 a = Root(load<u32>(p));
    const u32 b = Root(load<u32>(p + 4));
    const u32 xa = FindComp(a);
    const u32 xb = FindComp(b);
    const Side sa = Classify(o, xa), sb = Classify(o, xb);

    if (sa.group != sb.group) {
        const Side& g = sa.group ? sa : sb;
        const Side& other = sa.group ? sb : sa;
        if (sa.listed || sb.listed) {
            call(ctx, PairVec_AddUnique, {o, g.obj, other.obj});
        } else if (!sa.focus && !sb.focus && !(xa != 0 && xb != 0)) {
            const u32 x = g.obj;
            if (x != 0 && (load<u32>(x + 172) & kFlagPairable) != 0 && load<u32>(x + 168) != 0) {
                Stack frame(ctx, 16);  // the key (sp+112 or sp+116 in the original)
                store<u32>(frame.addr(), load<u32>(x + 168));
                const u32 hit = static_cast<u32>(call(ctx, SortedMap_Find, {o + 116, frame.addr()}));
                if (load<u32>(hit) == 0) {
                    const u32 key = load<u32>(x + 168);
                    if (load<u16>(o + 118) == load<u16>(o + 116))
                        call(ctx, Array_Reserve, {o + 116, (static_cast<u32>(load<u16>(o + 116)) << 1) & 0xFFFE});
                    const u32 data = load<u32>(o + 124);
                    store<u32>(data + ((static_cast<u32>(load<u16>(o + 118)) << 3) & 0x7FFF8), key);
                    store<u32>(data + (static_cast<u32>(load<u16>(o + 118)) << 3) + 4, x);
                    const u32 n = static_cast<u32>(load<u16>(o + 118)) + 1;
                    const u16 hi = load<u16>(o + 120), fl = load<u16>(o + 122);
                    store<u16>(o + 118, static_cast<u16>(n));
                    store<u16>(o + 120, static_cast<s32>(hi) > static_cast<s32>(n & 0xFFFF) ? hi : static_cast<u16>(n));
                    store<u16>(o + 122, fl | 1);
                }
            }
        }
    }
    store<u32>(p + 32, 1);
    ret(ctx, 1u);
}
