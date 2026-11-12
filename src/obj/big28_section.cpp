// Game code, large-function sweep part 28: the section objects' base SetMode
// (mode bookkeeping, the next user started, the section effect).
#include "big28.h"

namespace sly4::obj {

namespace {
constexpr u32 kShowWorld = 0x00F64C20;     // Obj** (the world object)
constexpr u32 kFxTime = 0x003260E8;        // .text pool: f32 0.0167 (the effect's start time)
constexpr u32 kFxSetCache = 0x011DE288;    // the effect set's last found record +8
constexpr u32 World_Notify = 0x002E5DD8;   // (world, section, s32 group) (f_002E5DD8)
constexpr u32 EntryList_Flush = 0x00325FCC;  // (section, u8)
constexpr u32 User_Start = 0x00325B58;     // (UserBlock*, s32 slot)
constexpr u32 Obj_CanShow = 0x002E2104;    // (-, section) -> bool (reads r4 only)
constexpr u32 Section_Link = 0x003218B0;   // (const Link20* l, section, u8 on) (f_003218B0)
constexpr u32 Set_Find = 0x004BCE90;       // (u32 hash) -> Set*
constexpr u32 Effect_Construct = 0x004C7A68;  // (Effect*, def)
constexpr u32 Effect_Start = 0x004C753C;   // (Effect*)
constexpr u32 Effect_SetTime = 0x004CF710; // (Effect*; f1 t)
constexpr u32 kFxSet = 0x7719A548u;        // the section effects' set
constexpr u32 kHashOpen = 0xDCCA6EB0u;     // effect hash: mode 4
constexpr u32 kHashDone = 0x264D6485u;     // effect hash: mode 5

// The section's owner block: +8 an object (s8 +172 its group), +80 the
// user list (data +0, u16 count +8), +0x20074 the mode words.
struct SecOwner {
    pad<8> _0;
    be_u32 x;               // +8
    pad<68> _12;
    PtrVec<u8> users;       // +80
};
static_assert(offsetof(SecOwner, users) == 80);
struct SecSection {
    pad<12> _0;
    be_u32 owner;           // +12: SecOwner*
    pad<1> _16;
    u8 b17;                 // +17
    u8 mode;                // +18
    pad<1> _19;
    be_u32 flags;           // +20
    pad<2196> _24;
    be_u32 fx_key;          // +2220
    be_u32 fx;              // +2224: Effect*
    pad<44> _2228;
    be_u32 links;           // +2272: Link vector (20-byte entries, u16 count +8)
    pad<176> _2276;
    be_u32 idx;             // +2452
    pad<108> _2456;
    be_u32 user;            // +2564
};
static_assert(offsetof(SecSection, flags) == 20 && offsetof(SecSection, fx_key) == 2220 && offsetof(SecSection, links) == 2272);
static_assert(offsetof(SecSection, idx) == 2452 && offsetof(SecSection, user) == 2564);
// The effect set: +16 -> its tree's root, +20 the path (s16 depth +100),
// +104 the node found.
struct FxSet {
    pad<16> _0;
    be_u32 root;     // +16: Node**
    be_u32 path[20]; // +20
    be_s16 depth;    // +100
    pad<2> _102;
    be_u32 found;    // +104
};
static_assert(offsetof(FxSet, depth) == 100 && offsetof(FxSet, found) == 104);
struct FxNode {
    pad<4> _0;
    be_u32 key;    // +4
    be_u32 value;  // +8
    be_u32 left;   // +12
    be_u32 right;  // +16
};
struct FxEffect {
    pad<136> _0;
    be_u32 flags;  // +136
};

be_u32& ModeWord(const SecSection* o) { return gm<be_u32>(o->owner + 4 * o->idx + 0x00020074u); }
}  // namespace

// u32 f(Section* o, s32 mode, u8 force) (the section base SetMode: called by
// the Section_SetMode_* family, Obj_SetKind_*, the section destructors).
// Nothing with o->+20 bit 0x80 (returns it). Mode 6 first notifies the
// world (World_Notify(world, o, s8 (o->+12)->+8 ->+172, 0 without it)).
// With force or a new mode: o->+18 = mode and o's word in the mode table
// (o->+12 + 0x20074 + 4 * o->+2452) gets the mode in its top nibble, then
// bit 0x04000000. Mode 4: o->+17 = 0; the user after o in (o->+12)->+80's
// list (u16 count +8) is started (slot 0) and, unless its vfn[100] says
// so, slots 2 and 1; o->+2564 slot 0; o slots 2 and 1; the effect +2224
// gets kHashOpen or is created (the kFxSet record for key o->+2220 found
// in its tree with the path cached at set+20.. / +100 / +104, kFxSetCache
// = record +8; a 140-byte Effect from it, bit 0x200 of +136 cleared (also
// through a null allocation), started at 0.0167); then, when the world
// can show o, each 20-byte link of o->+2272 (u16 count +8) Section_Link(
// .., 1). Mode 5: with links and the world showing o they get Section_Link(
// .., 0); then EntryList_Flush(o, 1) and the effect kHashDone.
// The result is the r3 the original leaves: several Section_SetMode_*
// callers return straight after the call, passing it on to callers that
// are not all known, so it is kept.
u32 Section_SetModeBase_003260F0(SecSection* o, s32 mode, u32 force_) {
    const u32 force = force_ & 0xFF;
    if (const u32 b = o->flags & 0x80u; b != 0) return b;
    if (mode == 6) {
        const u32 w = global<be_u32>(kShowWorld);
        const u32 x = gm<SecOwner>(o->owner).x;
        const s32 grp = x != 0 ? static_cast<s32>(gm<s8>(x + 172)) : 0;
        call(World_Notify, w, o, grp);
    }
    u32 r3 = force;
    bool set = force != 0;
    if (!set) {
        r3 = o->mode;
        set = static_cast<s32>(r3) != mode;
    }
    if (set) {
        be_u32& a = ModeWord(o);
        o->mode = static_cast<u8>(mode);
        a = (a & 0x0FFFFFFFu) | (static_cast<u32>(mode) << 28);
        be_u32& a2 = ModeWord(o);
        a2 = a2 | 0x04000000u;
        r3 = guest(&a2);
    }
    // The links of o->+2272 (u16 count +8, 20-byte entries), counted afresh each step.
    const auto links = [&](u32 on) -> u32 {
        for (u32 i = 0;; ++i) {
            const u32 l = o->links;
            const u32 n = l != 0 ? gm<PtrVec<u8>>(l).count : 0u;
            if (!(i < n)) return n;
            call(Section_Link, gm<be_u32>(l) + 20 * i, o, on);
        }
    };
    if (mode == 4) {
        o->b17 = 0;
        for (u32 i = 0;; ++i) {
            const PtrVec<u8>& lst = gm<SecOwner>(o->owner).users;
            const u32 n = lst.count;
            if (!(i < n)) break;
            const u32 arr = lst.data.ea;
            if (gm<be_u32>(arr + 4 * i) != guest(o) || !(i + 1 < n)) continue;
            const u32 nx = gm<be_u32>(arr + 4 * (i + 1));
            call(User_Start, nx, 0);
            if (vcall<u32>(host<u8>(nx), 100) == 0) {
                call(User_Start, nx, 2);
                call(User_Start, nx, 1);
            }
            break;
        }
        if (const u32 u = o->user; u != 0) call(User_Start, u, 0);
        call(User_Start, o, 2);
        call(User_Start, o, 1);
        if (const u32 e = o->fx; e != 0) {
            call(fn::Effect_SetHash, e, kHashOpen);
        } else if (const u32 sa = call<u32>(Set_Find, kFxSet); sa != 0) {
            FxSet& st = gm<FxSet>(sa);
            u32 node = gm<be_u32>(st.root);
            const u32 key = o->fx_key;
            if (node == 0) {
                st.found = 0u;
                global<be_u32>(kFxSetCache) = 0u;
            } else {
                st.depth = 0;
                for (;;) {
                    const u32 prev = node;
                    const u32 k = gm<FxNode>(node).key;
                    if (k == key) {
                        st.found = node;
                        global<be_u32>(kFxSetCache) = gm<FxNode>(node).value.get();
                        const u32 rec = st.found;
                        if (rec == 0) break;
                        const u32 def = gm<FxNode>(rec).value;
                        if (def == 0) break;
                        const u32 p = call<u32>(fn::SmallAlloc, 140u);
                        if (p != 0) call(Effect_Construct, p, def);
                        o->fx = p;
                        FxEffect& fx = gm<FxEffect>(p);
                        fx.flags = fx.flags & ~0x200u;  // quirk: written at 136 for a failed allocation too
                        call(Effect_Start, o->fx.get());
                        call(Effect_SetTime, o->fx.get(), lf(global<be_f32>(kFxTime)));
                        break;
                    }
                    node = k > key ? gm<FxNode>(node).left : gm<FxNode>(node).right;
                    if (node == 0) {
                        // prev is never null here; the original still tests it
                        if (prev == 0) st.depth = 0;
                        st.found = prev;
                        break;
                    }
                    const s32 d = st.depth;
                    st.depth = static_cast<s16>(d + 1);
                    gm<be_u32>(sa + 20 + 4 * static_cast<u32>(d)) = prev;
                }
            }
        }
        const u32 l = o->links;
        if (l == 0 || gm<PtrVec<u8>>(l).count == 0) return 0;
        if (call<u32>(Obj_CanShow, global<be_u32>(kShowWorld), o) == 0) return 0;
        return links(1);
    }
    if (mode == 5) {
        if (const u32 l = o->links; l != 0 && gm<PtrVec<u8>>(l).count != 0 && call<u32>(Obj_CanShow, global<be_u32>(kShowWorld), o) != 0)
            links(0);
        r3 = call<u32>(EntryList_Flush, o, 1u);
        if (const u32 e = o->fx; e != 0) r3 = call<u32>(fn::Effect_SetHash, e, kHashDone);
        return r3;  // the last callee's r3, as the original leaves it
    }
    return r3;
}

}  // namespace sly4::obj

NATIVE(003260F0, sly4::obj::Section_SetModeBase_003260F0)
