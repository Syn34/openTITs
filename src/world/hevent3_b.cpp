// World_HandleEvent callees, part 3: 66..69 instructions.
#include "hevent.h"

using namespace sly4;
using namespace sly4::hevent;

namespace sly4 {
namespace hevent3b {
namespace bfn {
constexpr u32 sub_0048981C = 0x0048981C;    // (World*, 0) -> bool
constexpr u32 Obj_SelectEntry = 0x003734B0; // (Object*, s32 id) (hevent3_a.cpp)
constexpr u32 sub_00470750 = 0x00470750;    // (World*, u32, u32)
constexpr u32 sub_000CDFB0 = 0x000CDFB0;    // 22 arguments (14 on the stack), f32 x2 (name?)
constexpr u32 SortedMap_Remove = 0x00771DD0; // (map, const u32* key) (name?)
constexpr u32 sub_003651F0 = 0x003651F0;    // (u32 hash) -> Object* (name?)
constexpr u32 sub_009D19C8 = 0x009D19C8;    // (body) -> h (physics)
constexpr u32 sub_00931D10 = 0x00931D10;    // (world, x)
constexpr u32 sub_0092F2A8 = 0x0092F2A8;    // (world, h, 1): add?
constexpr u32 sub_0092FFC8 = 0x0092FFC8;    // (out*, world, h): remove?
constexpr u32 sub_00931F00 = 0x00931F00;    // (world, x)
}  // namespace bfn

constexpr u32 kGamePlayersOfs = 0xB0, kPlayerSize = 0x2EC0;  // Game + (player/player.h)
constexpr u32 kPlayerHeld = 8, kPlayerPressed = 12;
constexpr u32 kButton4000 = 0x00004000;     // a button bit held and pressed (name?)
constexpr u32 kEntryId23FDB9 = 0x0023FDB9;  // id given to Obj_SelectEntry (name?)

constexpr u32 kMinusOne788070 = 0x00788070; // f32 -1.0
constexpr u32 kEmptyStrAE65E4 = 0x00AE65E4; // ""
constexpr u32 kOneVec3 = 0x01164B30;
constexpr u32 kIdentity = 0x01164BD0;

constexpr u32 kCurrentF64CBC = 0x00F64CBC;  // u32 Object* made by sub_003651F0 for kHashedName (name?)

constexpr u32 kMsgQueueCount = 0x010BA134;  // s32
constexpr u32 kMsgQueue = 0x010BA138;       // entries of 20 bytes: +4 Object*, +16 u8
}  // namespace hevent3b
}  // namespace sly4

using namespace sly4::hevent3b;

// bool World_PromptButton(World* w): when sub_0048981C(w, 0), the user filter
// +30 is 0 or 255, user 0's player has button 0x4000 held and pressed, there
// is a Player_Get() (its +24 object without +164 bit 0x8000) and +31 is clear:
// +51 = +30, +30 = 0, Obj_SelectEntry(Player_Get(), 0x0023FDB9) and
// sub_00470750(w, 4, 0). Always returns 1 (name?)
DECOMP(0048EE44, World_PromptButton) {
    const u32 w = arg<u32>(ctx, 0);
    if (static_cast<u32>(call(ctx, bfn::sub_0048981C, {w, 0})) != 0) {
        const u32 filter = load<u8>(w + 30);
        if (filter == 0 || filter == 255) {
            const u32 pl = g::game + kGamePlayersOfs + load<u8>(w + kUserBlocks + 1) * kPlayerSize;
            if ((load<u32>(pl + kPlayerHeld) & load<u32>(pl + kPlayerPressed) & kButton4000) != 0 &&
                static_cast<u32>(call(ctx, hfn::Player_Get)) != 0) {
                const u32 o = load<u32>(static_cast<u32>(call(ctx, hfn::Player_Get)) + 24);
                const bool blocked = o != 0 && (load<u32>(o + 164) & 0x00008000u) != 0;
                if (!blocked && load<u8>(w + 31) == 0) {
                    store<u8>(w + 51, load<u8>(w + 30));
                    store<u8>(w + 30, 0);
                    call(ctx, bfn::Obj_SelectEntry, {call(ctx, hfn::Player_Get), kEntryId23FDB9});
                    call(ctx, bfn::sub_00470750, {w, 4, 0});
                }
            }
        }
    }
    ret(ctx, 1u);
}

// Iter* TreeIter_Next(Iter* it): advances the walk at +100 (current node) with
// the stack at +16 (u32[]) and s16 depth at +96. A node with +16 is pushed and the
// walk descends through +12 links to the last one; otherwise nodes are popped
// until one whose +12 is the node just left (that one becomes current), or the
// stack runs out (current = 0) (name?)
DECOMP(00787A64, TreeIter_Next) {
    const u32 it = arg<u32>(ctx, 0);
    const auto slot = [it](u16 d) { return it + 16 + static_cast<u32>(static_cast<s32>(static_cast<s16>(d)) * 4); };
    const u32 cur = load<u32>(it + 100);
    if (cur == 0) return;
    u32 n = load<u32>(cur + 16);
    if (n != 0) {
        u16 d = load<u16>(it + 96);
        store<u32>(slot(d), cur);
        store<u16>(it + 96, static_cast<u16>(d + 1));
        while (load<u32>(n + 12) != 0) {
            d = load<u16>(it + 96);
            store<u32>(slot(d), n);
            n = load<u32>(n + 12);
            store<u16>(it + 96, static_cast<u16>(d + 1));
        }
        store<u32>(it + 100, n);
        return;
    }
    u32 child = cur;
    u32 d = load<u16>(it + 96);
    while (true) {
        if ((d & 0xFFFF) == 0) {
            store<u32>(it + 100, 0);
            return;
        }
        d -= 1;
        store<u16>(it + 96, static_cast<u16>(d));
        const u32 p = load<u32>(slot(static_cast<u16>(d)));
        store<u32>(it + 100, p);
        if (child == load<u32>(p + 12)) return;
        child = p;
    }
}

// u32 Obj_InitDefault(Object* o, a, b, c, d): sub_000CDFB0(o, 0, 0, a, 0, 0, "",
// identity, unit vec3, 1, 0, 0, 1, 0 x5, -1, -, -, 0; -1.0, -1.0), then +144 = b,
// +148 = c, +152 = d, +132 = 0, +20 |= 0x10300 with 0x2000 cleared, +12 |= 0x100;
// returns the new +20 (name?)
DECOMP(00788078, Obj_InitDefault) {
    const u32 o = arg<u32>(ctx, 0), a = arg<u32>(ctx, 1), b = arg<u32>(ctx, 2);
    const u32 c = arg<u32>(ctx, 3), d = arg<u32>(ctx, 4);
    const f64 m1 = static_cast<f64>(load<f32>(kMinusOne788070));
    // Stack arguments at r1+112..+216. The slots at +200 and +208 belong to the
    // two float arguments and are left unwritten by the original (written as 0 here).
    callf(ctx, bfn::sub_000CDFB0,
          {o, 0, 0, a, 0, 0, kEmptyStrAE65E4, kIdentity,
           kOneVec3, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0xFFFFFFFFull, 0, 0, 0},
          {m1, m1});
    const u32 f20 = ((load<u32>(o + 20) | 0x00010000u) & ~0x00002000u) | 0x300u;
    const u32 f12 = load<u32>(o + 12) | 0x100u;
    store<u32>(o + 144, b);
    store<u32>(o + 148, c);
    store<u32>(o + 152, d);
    store<u32>(o + 132, 0);
    store<u32>(o + 20, f20);
    store<u32>(o + 12, f12);
    ctx.gpr[4] = f12;  // as the original leaves it
    ret(ctx, f20);
}

// Object* Hashed_Refresh(): keeps the object at 0x00F64CBC in step with the
// name hash at 0x0106C278: if its key (+12, else +8 -> +68) differs, the key is
// removed from the handle map, the object deleted (vfunc +4) and the slot
// cleared; a missing object is (re)made with sub_003651F0(hash) (name?)
DECOMP(003654C0, Hashed_Refresh) {
    u32 cur = load<u32>(kCurrentF64CBC);
    const auto keyOf = [](u32 o) -> u32 {
        const u32 k = load<u32>(o + 12);
        if (k != 0) return k;
        const u32 p = load<u32>(o + 8);
        return p != 0 ? load<u32>(p + 68) : 0;
    };
    if (cur != 0) {
        const u32 want = load<u32>(kHashedName);
        if (keyOf(cur) == want) return ret(ctx, cur);
        Stack frame(ctx, 16);  // the key at r1+112
        const u32 k = keyOf(cur);
        store<u32>(frame.addr(), k);
        ctx.gpr[5] = k;
        call(ctx, bfn::SortedMap_Remove, {kHandleMap, frame.addr()});
        cur = load<u32>(kCurrentF64CBC);
        if (cur == 0) goto make;
        vcall(ctx, cur, 4, {cur});
        store<u32>(kCurrentF64CBC, 0);
    }
make:
    store<u32>(kCurrentF64CBC, static_cast<u32>(call(ctx, bfn::sub_003651F0, {load<u32>(kHashedName)})));
}

// s32 MsgQueue_Dispatch(): for each queued entry (20 bytes at 0x010BA138): with
// +16 clear, vfunc +8(obj, entry); with +16 set and obj +172 bit 0x00040000,
// vfunc +172(obj, entry). Then the count is cleared; returns its last value (name?)
DECOMP(0048F800, MsgQueue_Dispatch) {
    s32 n = load<s32>(kMsgQueueCount);
    for (s32 i = 0; i < n; ++i, n = load<s32>(kMsgQueueCount)) {
        const u32 e = kMsgQueue + static_cast<u32>(i) * 20;
        const u32 o = load<u32>(e + 4);
        if (load<u8>(e + 16) == 0) vcall(ctx, o, 8, {o, e});
        else if (load<u32>(o + 172) & 0x00040000u) vcall(ctx, o, 172, {o, e});
    }
    store<u32>(kMsgQueueCount, 0);
    ret(ctx, n);
}

// u32 Obj_SetPhysicsActive(Object* o, u8 on): when o has a body (+32) and the
// state bit (+416 bit 31) differs from `on`: on -> sub_00931D10(world, +48) when
// set and sub_0092F2A8(world, h, 1); off -> sub_0092FFC8(&tmp, world, h) when
// h->+8, and sub_00931F00(world, +48) when +48 and its +8 are set. h =
// sub_009D19C8(body), world = (+28)->+4. Then bit 31 of +416 = on & 1;
// returns the new +416 (name?)
DECOMP(00260C14, Obj_SetPhysicsActive) {
    const u32 o = arg<u32>(ctx, 0), on = arg<u32>(ctx, 1);
    const u32 onb = on & 0xFF;
    const u32 body = load<u32>(o + 32);
    if (body != 0 && (onb ^ (load<u32>(o + 416) >> 31)) != 0) {
        const u32 h = static_cast<u32>(call(ctx, bfn::sub_009D19C8, {body}));
        const u32 world = load<u32>(load<u32>(o + 28) + 4);
        if (onb != 0) {
            const u32 x = load<u32>(o + 48);
            if (x != 0) call(ctx, bfn::sub_00931D10, {world, x});
            call(ctx, bfn::sub_0092F2A8, {world, h, 1});
        } else {
            if (load<u32>(h + 8) != 0) {
                Stack frame(ctx, 16);  // r1+112
                call(ctx, bfn::sub_0092FFC8, {frame.addr(), world, h});
            }
            const u32 x = load<u32>(o + 48);
            if (x != 0 && load<u32>(x + 8) != 0) call(ctx, bfn::sub_00931F00, {world, x});
        }
    }
    const u32 v = (load<u32>(o + 416) & 0x7FFFFFFFu) | ((on & 1u) << 31);
    store<u32>(o + 416, v);
    ret(ctx, v);
}
