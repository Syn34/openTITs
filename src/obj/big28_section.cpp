// Game code, large-function sweep part 28: the section objects' base SetMode
// (mode bookkeeping, the next user started, the section effect).
#include "../sly4.h"
#include "big28.h"

using namespace sly4;
using namespace sly4::big28;

namespace {
constexpr u32 kWorld = 0x00F64C20;         // Obj** (the world object)
constexpr u32 kFxTime = 0x003260E8;        // .text pool: f32 0.0167 (the effect's start time)
constexpr u32 kFxSetCache = 0x011DE288;    // the effect set's last found record +8
constexpr u32 kModeTable = 0x00020074;     // +o->12: the u32 mode words (top nibble the mode, 0x04000000 changed)
constexpr u32 World_Notify = 0x002E5DD8;   // (world, section, s32 group) (f_002E5DD8)
constexpr u32 EntryList_Flush = 0x00325FCC;  // (section, u8)
constexpr u32 User_Start = 0x00325B58;     // (UserBlock*, s32 slot)
constexpr u32 Obj_CanShow = 0x002E2104;    // (-, section) -> bool (reads r4 only)
constexpr u32 Section_Link = 0x003218B0;   // (const Link20* l, section, u8 on) (f_003218B0)
constexpr u32 Set_Find = 0x004BCE90;       // (u32 hash) -> Set*
constexpr u32 SmallAlloc = 0x0055238C;     // (u32 size) -> p or 0
constexpr u32 Effect_Construct = 0x004C7A68;  // (Effect*, def)
constexpr u32 Effect_Start = 0x004C753C;   // (Effect*)
constexpr u32 Effect_SetTime = 0x004CF710; // (Effect*; f1 t)
constexpr u32 Effect_SetHash = 0x004CF988; // (Effect*, u32 hash)
constexpr u32 kFxSet = 0x7719A548u;        // the section effects' set
constexpr u32 kHashOpen = 0xDCCA6EB0u;     // effect hash: mode 4
constexpr u32 kHashDone = 0x264D6485u;     // effect hash: mode 5
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
// r3 is left as the original leaves it.
DECOMP(003260F0, Section_SetModeBase_003260F0) {
    const u32 o = arg<u32>(ctx, 0);
    const s32 mode = arg<s32>(ctx, 1);
    const u32 force = arg<u32>(ctx, 2) & 0xFF;
    if (const u32 b = load<u32>(o + 20) & 0x80u; b != 0) {
        ret(ctx, b);
        return;
    }
    if (mode == 6) {
        const u32 w = load<u32>(kWorld);
        const u32 x = load<u32>(load<u32>(o + 12) + 8);
        const u32 grp = x != 0 ? static_cast<u32>(static_cast<s32>(load<s8>(x + 172))) : 0u;
        call(ctx, World_Notify, {w, o, grp});
    }
    u32 r3;
    r3 = force;
    bool set = force != 0;
    if (!set) {
        r3 = load<u8>(o + 18);
        set = static_cast<s32>(r3) != mode;
    }
    if (set) {
        u32 idx = load<u32>(o + 2452), tab = load<u32>(o + 12);
        store<u8>(o + 18, static_cast<u8>(mode));
        u32 a = tab + 4 * idx + kModeTable;
        store<u32>(a, (load<u32>(a) & 0x0FFFFFFFu) | (static_cast<u32>(mode) << 28));
        idx = load<u32>(o + 2452), tab = load<u32>(o + 12);
        a = tab + 4 * idx + kModeTable;
        store<u32>(a, load<u32>(a) | 0x04000000u);
        r3 = a;
    }
    // The links of o->+2272 (u16 count +8, 20-byte entries), counted afresh each step.
    const auto links = [&](u32 on) -> u32 {
        for (u32 i = 0;; ++i) {
            const u32 l = load<u32>(o + 2272);
            const u32 n = l != 0 ? load<u16>(l + 8) : 0u;
            if (!(i < n)) return n;
            call(ctx, Section_Link, {load<u32>(l) + 20 * i, o, on});
        }
    };
    if (mode == 4) {
        store<u8>(o + 17, 0);
        for (u32 i = 0;; ++i) {
            const u32 lst = load<u32>(o + 12) + 80;
            const u32 n = load<u16>(lst + 8);
            if (!(i < n)) break;
            const u32 arr = load<u32>(lst);
            if (load<u32>(arr + 4 * i) != o || !(i + 1 < n)) continue;
            const u32 nx = load<u32>(arr + 4 * (i + 1));
            call(ctx, User_Start, {nx, 0});
            if (static_cast<u32>(VCall(ctx, nx, 100, {nx})) == 0) {
                call(ctx, User_Start, {nx, 2});
                call(ctx, User_Start, {nx, 1});
            }
            break;
        }
        if (const u32 u = load<u32>(o + 2564); u != 0) call(ctx, User_Start, {u, 0});
        call(ctx, User_Start, {o, 2});
        call(ctx, User_Start, {o, 1});
        if (const u32 e = load<u32>(o + 2224); e != 0) {
            call(ctx, Effect_SetHash, {e, kHashOpen});
        } else if (const u32 s = static_cast<u32>(call(ctx, Set_Find, {kFxSet})); s != 0) {
            u32 node = load<u32>(load<u32>(s + 16));
            const u32 key = load<u32>(o + 2220);
            if (node == 0) {
                store<u32>(s + 104, 0);
                store<u32>(kFxSetCache, 0);
            } else {
                store<u16>(s + 100, 0);
                for (;;) {
                    const u32 prev = node;
                    const u32 k = load<u32>(node + 4);
                    if (k == key) {
                        store<u32>(s + 104, node);
                        store<u32>(kFxSetCache, load<u32>(node + 8));
                        const u32 rec = load<u32>(s + 104);
                        if (rec == 0) break;
                        const u32 def = load<u32>(rec + 8);
                        if (def == 0) break;
                        u32 p = static_cast<u32>(call(ctx, SmallAlloc, {140}));
                        if (p != 0) {
                            call(ctx, Effect_Construct, {p, def});
                        }
                        store<u32>(o + 2224, p);
                        store<u32>(p + 136, load<u32>(p + 136) & ~0x200u);  // quirk: written at 136 for a failed allocation too
                        call(ctx, Effect_Start, {load<u32>(o + 2224)});
                        callf(ctx, Effect_SetTime, {load<u32>(o + 2224)}, {Lf(kFxTime)});
                        break;
                    }
                    node = load<u32>(node + (k > key ? 12 : 16));
                    if (node == 0) {
                        // prev is never null here; the original still tests it
                        if (prev == 0) store<u16>(s + 100, 0);
                        store<u32>(s + 104, prev);
                        break;
                    }
                    const s32 d = load<s16>(s + 100);
                    store<u16>(s + 100, static_cast<u16>(d + 1));
                    store<u32>(s + 20 + 4 * static_cast<u32>(d), prev);
                }
            }
        }
        const u32 l = load<u32>(o + 2272);
        if (l == 0 || load<u16>(l + 8) == 0) {
            ret(ctx, 0u);
            return;
        }
        if (static_cast<u32>(call(ctx, Obj_CanShow, {load<u32>(kWorld), o})) == 0) {
            ret(ctx, 0u);
            return;
        }
        ret(ctx, links(1));
        return;
    }
    if (mode == 5) {
        if (const u32 l = load<u32>(o + 2272); l != 0) {
            r3 = load<u16>(l + 8);
            if (r3 != 0) {
                r3 = static_cast<u32>(call(ctx, Obj_CanShow, {load<u32>(kWorld), o}));
                if (r3 != 0) links(0);
            }
        }
        call(ctx, EntryList_Flush, {o, 1});
        if (const u32 e = load<u32>(o + 2224); e != 0) call(ctx, Effect_SetHash, {e, kHashDone});
        return;
    }
    ret(ctx, r3);
}
