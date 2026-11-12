// Game code, large-function sweep part 34: a contact listener's pair
// callback, which pairs up the two bodies' owners or records the lone one.
#include "big34.h"

namespace sly4::obj {

// A rigid body of a contact pair: +12 the parent body, +16 the s8 offset
// from the topmost body to its owner, +24 the s8 kind (1: has an owner).
struct PairBody {
    pad<12> _0;
    gp<PairBody> parent;  // +12
    s8 owner_off;         // +16
    pad<7> _17;
    s8 kind;              // +24
};
static_assert(offsetof(PairBody, owner_off) == 16 && offsetof(PairBody, kind) == 24);

// An owner's component table entry (16 bytes): +0 the kind hash, +12 the
// component.
struct CompEntry16 {
    be_u32 hash;  // +0
    pad<8> _4;
    be_u32 comp;  // +12
};
static_assert(sizeof(CompEntry16) == 16);
struct CompOwner {
    pad<124> _0;
    gp<CompEntry16> table;  // +124
    be_s32 count;           // +128
};
static_assert(offsetof(CompOwner, count) == 128);

// The object a body's owner leads to.
struct PairObj {
    pad<168> _0;
    be_u32 key;    // +168
    be_u32 flags;  // +172
    pad<40> _176;
    be_u32 group;  // +216
};
static_assert(offsetof(PairObj, flags) == 172 && offsetof(PairObj, group) == 216);

// An entry of the listener's sorted {key, object} vector.
struct KeyObj8 {
    be_u32 key;       // +0
    gp<PairObj> obj;  // +4
};

struct ContactListener {
    pad<116> _0;
    be_u16 cap;          // +116: sorted u16 vector of KeyObj8
    be_u16 count;        // +118
    be_u16 mark;         // +120: high-water mark
    be_u16 flags;        // +122
    gp<KeyObj8> data;    // +124
    pad<32> _128;
    be_u32 groups[3];    // +160
    pad<52> _172;
    be_u32 group;        // +224
    pad<144> _228;
    be_u32 focus;        // +372: a PairObj
};
static_assert(offsetof(ContactListener, data) == 124 && offsetof(ContactListener, groups) == 160);
static_assert(offsetof(ContactListener, group) == 224 && offsetof(ContactListener, focus) == 372);

struct ContactPair {
    gp<PairBody> a, b;  // +0, +4
    pad<24> _8;
    be_u32 done;        // +32
};
static_assert(offsetof(ContactPair, done) == 32);

namespace {
constexpr u32 kCompHash = 0x91C7CF5B;      // the component kind looked up in each owner's table
constexpr u32 kFlagPairable = 0x00040000;  // PairObj::flags bit: the object may be recorded
constexpr u32 PairVec_AddUnique = 0x003A95F4;  // (o, x, v)
constexpr u32 Array_Reserve = 0x00558F3C;      // (array, u16 capacity)
constexpr u32 SortedMap_Find = 0x00759654;     // (map, const u32* key) -> entry*

// The topmost body: follow +12 while it is non-zero.
PairBody* Root(PairBody* b) {
    while (b->parent) b = b->parent;
    return b;
}

// The comp word of the owner's kCompHash entry (count and table reloaded
// per step), or 0. A root whose kind is not 1 has no owner, and its table
// is read from address 0 (unchecked, as the original does).
u32 FindComp(const PairBody* root) {
    const u32 owner = root->kind == 1 ? guest(root) + static_cast<s32>(root->owner_off) : 0u;
    const CompOwner& w = gm<CompOwner>(owner);
    for (s32 i = 0; i < w.count; ++i) {
        const CompEntry16& e = gm<CompEntry16>(w.table.ea + 16 * static_cast<u32>(i));
        if (e.hash == kCompHash) return e.comp;
    }
    return 0;
}

struct Side {
    u32 obj;
    bool group;   // obj's group is the listener's group
    bool focus;   // obj is the listener's focus
    bool listed;  // obj's group is one of the listener's three groups
};

Side Classify(const ContactListener* o, u32 x) {
    Side s{x, false, false, false};
    if (x == 0) return s;
    const u32 g = gm<PairObj>(x).group;
    s.group = g == o->group;
    s.focus = x == o->focus;
    if (g != 0)
        for (u32 k = 0; k < 3; ++k)
            if (o->groups[k] == g) {
                s.listed = true;
                break;
            }
    return s;
}
}  // namespace

// bool T::OnPair(T* o, Pair* p) (through AdjThunk_003A9C1C, this - 56).
// The roots of the bodies p->a and p->b lead to two objects (FindComp).
// When exactly one of them is in o's group: if either one's group is
// listed in o->groups, PairVec_AddUnique(o, the grouped one, the other);
// otherwise, unless either is o->focus or both exist, the grouped one (when
// it has kFlagPairable and a key not yet in o's sorted vector) is appended
// there as {key, object}, doubling the capacity when full, and the flags
// get bit 0. Always sets p->done = 1 and returns 1.
u32 ContactListener_OnPair_003A96F8(ContactListener* o, ContactPair* p) {
    const PairBody* a = Root(p->a);
    const PairBody* b = Root(p->b);
    const u32 xa = FindComp(a);
    const u32 xb = FindComp(b);
    const Side sa = Classify(o, xa), sb = Classify(o, xb);

    if (sa.group != sb.group) {
        const Side& g = sa.group ? sa : sb;
        const Side& other = sa.group ? sb : sa;
        if (sa.listed || sb.listed) {
            call(PairVec_AddUnique, o, g.obj, other.obj);
        } else if (!sa.focus && !sb.focus && !(xa != 0 && xb != 0)) {
            PairObj* x = host<PairObj>(g.obj);
            if (x != nullptr && (x->flags & kFlagPairable) != 0 && x->key != 0) {
                GuestFrame frame(16);  // the key (sp+112 or sp+116 in the original)
                be_u32* key_slot = frame.as<be_u32>();
                *key_slot = x->key.get();
                const u32 hit = call<u32>(SortedMap_Find, &o->cap, key_slot);
                if (gm<be_u32>(hit) == 0) {
                    const u32 key = x->key;
                    if (o->count == o->cap) call(Array_Reserve, &o->cap, (static_cast<u32>(o->cap) << 1) & 0xFFFE);
                    KeyObj8* data = &gm<KeyObj8>(o->data.ea);  // address 0 when unset (unchecked)
                    data[o->count].key = key;
                    data[o->count].obj = x;
                    const u32 n = static_cast<u32>(o->count) + 1;
                    const u16 hi = o->mark, fl = o->flags;
                    o->count = static_cast<u16>(n);
                    o->mark = static_cast<s32>(hi) > static_cast<s32>(n & 0xFFFF) ? hi : static_cast<u16>(n);
                    o->flags = static_cast<u16>(fl | 1);
                }
            }
        }
    }
    p->done = 1u;
    return 1;
}

}  // namespace sly4::obj

NATIVE(003A96F8, sly4::obj::ContactListener_OnPair_003A96F8)
