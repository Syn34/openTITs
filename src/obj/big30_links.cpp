// Game code, large-function sweep part 30: a section's link mode, which
// registers or forgets its keys in the 128-byte records of its holder.
#include "native/objtypes.h"

namespace sly4::obj {

struct LinkRec {
    pad<8> _0;
    be_u32 key;    // +8
    pad<4> _12;
    Vec3 pos;      // +16
    pad<8> _28;
    be_u32 w36;    // +36
    be_u32 tag;    // +40
    pad<76> _44;
    be_u32 flags;  // +120
};
static_assert(offsetof(LinkRec, tag) == 40 && offsetof(LinkRec, flags) == 120);
struct LinkHolderRef {
    pad<8> _0;
    be_u32 holder;  // +8
};
struct LinkSection {
    pad<12> _0;
    gp<LinkHolderRef> ref;  // +12
    pad<2616> _16;
    be_u32 b_keys;          // +2632: u32[]
    pad<4> _2636;
    be_u16 b_count;         // +2640
    pad<10> _2642;
    be_u32 a_keys;          // +2652: u32[]
    pad<4> _2656;
    be_u16 a_count;         // +2660
    pad<10> _2662;
    be_u32 single;          // +2672
    be_u32 mode;            // +2676
};
static_assert(offsetof(LinkSection, b_keys) == 2632 && offsetof(LinkSection, a_count) == 2660);
static_assert(offsetof(LinkSection, single) == 2672 && offsetof(LinkSection, mode) == 2676);

namespace {
constexpr u32 Obj_Notify24 = 0x000D949C;       // (holder) -> record owner or 0
constexpr u32 Rec128_Alloc = 0x0029F760;       // (owner) -> Rec* or 0
constexpr u32 Records_RemoveKey = 0x0029F7A0;  // (owner, key)
constexpr u32 kRecTag = 0x427BF52D;

u32 Owner(const LinkSection* o) { return call<u32>(Obj_Notify24, gm<LinkHolderRef>(o->ref.ea).holder.get()); }

// A fresh record for key: the zero vector (raw), the tag, w36 = 1, flags
// bits 28 / 27 / 25 / 23 cleared and bit 26 set.
void AddKey(u32 owner, u32 key) {
    LinkRec* r = call<LinkRec*>(Rec128_Alloc, owner);
    if (r == nullptr) return;
    const Vec3& z = global<Vec3>(kZeroVec);
    const u32 w = r->flags;
    r->key = key;
    r->pos.x = z.x;
    r->tag = kRecTag;
    r->w36 = 1u;
    r->pos.y = z.y;
    r->pos.z = z.z;
    r->flags = (w & ~0x1A800000u) | 0x04000000u;
}
// Each key of a list (count re-read): with an owner, added or removed.
void ForList(const LinkSection* o, const be_u32& keys, const be_u16& count, bool add) {
    for (u32 i = 0, off = 0; i < count; ++i, off += 4) {
        const u32 key = gm<be_u32>(keys + off);
        const u32 w = Owner(o);
        if (w == 0) continue;
        if (add) AddKey(w, key);
        else call(Records_RemoveKey, w, key);
    }
}
}  // namespace

// void f(Section* o, s32 mode, u8 force) (called by
// Section_ReleaseAll334114, Section_AcquireAll334B74, f_00334D88; none
// reads r3). Nothing when mode is the current one and force is clear. The
// keys are the list A, the list B and the single key; the record owner is
// Obj_Notify24(o->ref->holder), asked per key. Mode 0 removes A, B and the
// single key; 1 adds A and removes B and the single key; 2 removes A, adds
// B, removes the single key; 3 (only with a single key, read on entry)
// removes A and B and adds the single key. Then o->mode = mode (also for
// other modes).
void Section_SetLinkMode_00333BC8(LinkSection* o, s32 mode, u8 force) {
    const u32 single = o->single;
    if (static_cast<u32>(mode) == o->mode && force == 0) return;
    auto single_key = [&](bool add) {
        const u32 k = o->single;
        if (!add && k == 0) return;
        const u32 w = Owner(o);
        if (w == 0) return;
        if (add) AddKey(w, k);
        else call(Records_RemoveKey, w, k);
    };
    switch (mode) {
    case 0:
        ForList(o, o->a_keys, o->a_count, false);
        ForList(o, o->b_keys, o->b_count, false);
        single_key(false);
        break;
    case 1:
        ForList(o, o->a_keys, o->a_count, true);
        ForList(o, o->b_keys, o->b_count, false);
        single_key(false);
        break;
    case 2:
        ForList(o, o->a_keys, o->a_count, false);
        ForList(o, o->b_keys, o->b_count, true);
        single_key(false);
        break;
    case 3:
        if (single == 0) break;
        ForList(o, o->a_keys, o->a_count, false);
        ForList(o, o->b_keys, o->b_count, false);
        single_key(true);
        break;
    default:
        break;
    }
    o->mode = static_cast<u32>(mode);
}

}  // namespace sly4::obj

NATIVE(00333BC8, sly4::obj::Section_SetLinkMode_00333BC8)
