// Game code, large-function sweep part 13: further game functions.
#include "big13.h"

using namespace sly4;
using namespace sly4::big13;

namespace {
constexpr u32 kAnim_Restart = 0x002775A8;   // Anim_Restart2775A8
constexpr u32 kTarget_GetDef = 0x000D951C;  // Target_GetDef0D951C

// Anim_Restart2775A8(o's animator (vfunc +60), key, 0, 0, -, -, 0, 0, -, -,
// 0, -1; k, k + 4, k + 8, k + 8). r7 / r8 and arguments 9 / 10 are not
// written by the original (the callee does not read them): r7 / r8 pass
// what the virtual call left.
void RestartOn(PPUContext& ctx, u32 o, u32 key, u32 k) {
    const u32 an = static_cast<u32>(VCall(ctx, o, 60, {o}));
    const f64 c = LfsAt(k + 8);
    callf(ctx, kAnim_Restart, {an, key, 0, 0, ctx.gpr[7], ctx.gpr[8], 0, 0, 0, 0, 0, ~0ull}, {LfsAt(k), LfsAt(k + 4), c, c});
}
}  // namespace

// u32 Obj_SetPairAnimState1DEE48(Obj* p, s32 state) (name?): p->704 =
// state for -1..5 and, for 0..5, restarts a pair of animations, one on the
// target def's +64 object and one on p, picked by the state and (states
// 1..4) the variant p->740 (0..2; other variants play nothing). State 4
// first copies bit 0 of the +68 child's +16 byte to bit 5 of p->744 (clear
// without a child) and calls the child's vfunc +44 (0). Returns the last
// Anim_Restart's result, or what the original leaves in r3 (the state, the
// variant for state 4, -1).
DECOMP(001DEE48, Obj_SetPairAnimState1DEE48) {
    constexpr u32 kK = 0x001DEE38u;   // .text f32 x3
    struct Pair {
        u32 a, b;
    };
    static constexpr Pair kVariant[4][3] = {
        {{0xF020F7B8u, 0x5960DC3Du}, {0x13FDFF94u, 0xD2BAABAFu}, {0x13FDFF9Au, 0xD2BAABB5u}},   // state 1
        {{0xF01FAE1Au, 0x595F929Fu}, {0x13D61572u, 0xD292C18Du}, {0x13D61578u, 0xD292C193u}},   // state 2
        {{0x9F4C9EA8u, 0xE54CB8ADu}, {0x4A4736A4u, 0xC44A5D3Fu}, {0x4A4736AAu, 0xC44A5D45u}},   // state 3
        {{0xEEF8C057u, 0xD997B732u}, {0xF01F4AD5u, 0x595F2F5Au}, {0xF01F4ADBu, 0x595F2F60u}},   // state 4
    };
    const u32 p = arg<u32>(ctx, 0);
    const u64 r4 = ctx.gpr[4];
    const s32 state = static_cast<s32>(r4);
    const u32 o = load<u32>(static_cast<u32>(call(ctx, kTarget_GetDef, {p, r4})) + 64);
    auto play = [&](Pair pr) {
        RestartOn(ctx, o, pr.a, kK);
        RestartOn(ctx, p, pr.b, kK);
    };
    switch (state) {
    case -1:
        store<s32>(p + 704, -1);
        ctx.gpr[3] = ~0ull;
        return;
    case 0:
        store<u32>(p + 704, 0);
        play({0x149208DEu, 0xD34EB4F9u});
        return;
    case 5:
        store<u32>(p + 704, 5);
        play({0x63451417u, 0x7C1DEB5Cu});
        return;
    case 1:
    case 2:
    case 3:
    case 4: {
        store<u32>(p + 704, static_cast<u32>(state));
        if (state == 4) {
            const u32 child = static_cast<u32>(VCall(ctx, o, 68, {o}));
            const u8 f = load<u8>(p + 744);
            if (child == 0) {
                store<u8>(p + 744, static_cast<u8>(f & ~0x20u));
            } else {
                store<u8>(p + 744, static_cast<u8>((f & ~0x20u) | ((load<u8>(child + 16) << 5) & 0x20u)));
                VCall(ctx, child, 44, {child, 0});
            }
        }
        const u32 var = load<u32>(p + 740);
        if (var > 2) {   // (signed: negative variants too)
            ctx.gpr[3] = state == 4 ? var : static_cast<u32>(state);
            return;
        }
        play(kVariant[state - 1][var]);
        return;
    }
    default:
        ctx.gpr[3] = r4;
        return;
    }
}

namespace {
// The maps of 0x004B7DF0's object: {+0 root, +4 u32 count, +12 Node**
// (the root slot), +16 s32 path[20], +96 s16 depth, +100 current}; nodes
// {+4 key, +8 value, +12 left, +16 right}.
void PathPush(u32 m, u32 node, bool depthFirst) {
    const s32 d = static_cast<s16>(load<u16>(m + 96));
    const u32 slot = m + 16 + 4 * static_cast<u32>(d);
    if (depthFirst) {
        store<u16>(m + 96, static_cast<u16>(d + 1));
        store<u32>(slot, node);
    } else {
        store<u32>(slot, node);
        store<u16>(m + 96, static_cast<u16>(d + 1));
    }
}
// The leftmost node under the root slot (pushing the path), or 0.
u32 MapFirst(u32 m) {
    u32 x = load<u32>(load<u32>(m + 12));
    if (x == 0) return 0;
    while (load<u32>(x + 12) != 0) {
        PathPush(m, x, true);
        x = load<u32>(x + 12);
    }
    return x;
}
// The inlined in-order walk over the map at p + off (re-read after every
// release) calling each value's vfunc +8. Returns the last map pointer read.
u32 MapReleaseValues(PPUContext& ctx, u32 p, u32 off) {
    u32 m = load<u32>(p + off);
    if (m != 0) {
        store<u16>(m + 96, 0);
        store<u32>(m + 100, MapFirst(m));
    }
    while (m != 0 && load<u32>(m + 100) != 0) {
        const u32 v = load<u32>(load<u32>(m + 100) + 8);
        VCall(ctx, v, 8, {v});
        m = load<u32>(p + off);
        if (m == 0) continue;
        u32 cur = load<u32>(m + 100);
        if (cur == 0) continue;
        u32 r = load<u32>(cur + 16);
        if (r == 0) {   // climb until coming up from a left child
            u32 d = load<u16>(m + 96);
            for (;;) {
                if ((d & 0xFFFFu) == 0) {
                    store<u32>(m + 100, 0);
                    break;
                }
                d -= 1;
                store<u16>(m + 96, static_cast<u16>(d));
                const u32 par = load<u32>(m + 16 + 4 * static_cast<u32>(static_cast<s32>(static_cast<s16>(d))));
                store<u32>(m + 100, par);
                if (cur == load<u32>(par + 12)) break;
                cur = par;
            }
        } else {   // the right subtree's leftmost
            PathPush(m, cur, true);
            while (load<u32>(r + 12) != 0) {
                PathPush(m, r, false);
                r = load<u32>(r + 12);
            }
            store<u32>(m + 100, r);
        }
    }
    return m;
}
// Erases the leftmost node until the map is empty: find(m, key, out,
// root) and, when found, erase(m, &root, key, changed byte) and --count.
// kind 3 is the f32-keyed map (Tree_FindF32 / 0x0078FD34 with the key in
// f1; r4 = 0 for the find, r5 left as the find leaves it for the erase).
void MapEraseAll(PPUContext& ctx, u32 m, u32 kind, u32 out, u32 changed) {
    static constexpr u32 kFind[3] = {0x0078E260u, 0x0079187Cu, 0x0078D300u};
    static constexpr u32 kErase[3] = {0x0078D670u, 0x00790C8Cu, 0x0078C710u};
    for (;;) {
        const u32 slot = load<u32>(m + 12);
        store<u16>(m + 96, 0);
        u32 x = load<u32>(slot);
        if (x == 0) {
            store<u32>(m + 100, 0);
            return;
        }
        while (load<u32>(x + 12) != 0) {
            PathPush(m, x, true);
            x = load<u32>(x + 12);
        }
        store<u32>(m + 100, x);
        store<u8>(changed, 0);
        if (kind == 3) {
            const f64 key = LfsX(x + 4);
            callf(ctx, 0x00790920u, {m, 0, out, load<u32>(m)}, {key});
            if (static_cast<u32>(ctx.gpr[3]) == 0) continue;
            callf(ctx, 0x0078FD34u, {m, m, ctx.gpr[5], changed}, {key});
        } else {
            const u32 key = load<u32>(x + 4);
            if (static_cast<u32>(call(ctx, kFind[kind], {m, key, out, load<u32>(m)})) == 0) continue;
            call(ctx, kErase[kind], {m, m, key, changed});
        }
        store<u32>(m + 4, load<u32>(m + 4) - 1);
    }
}
}  // namespace

// void MapSet4_Destruct4B7DF0(T* p) (vtable 0x00B21C68 -> 0x00B11B20;
// name?): releases (vfunc +8) every value of its four maps (+12, +24,
// +16, +20; the last f32-keyed) and empties each, decrements the u16 +4,
// clears and frees (20 bytes) the PtrVector +28, then empties again and
// frees (104 bytes) the maps +24, +20, +16, +12. Returns the base vtable.
DECOMP(004B7DF0, MapSet4_Destruct4B7DF0) {
    const u32 p = arg<u32>(ctx, 0);
    Stack frame(ctx, 224);
    const u32 sp = frame.addr();   // the original's frame offsets (outs +112.., bytes +144..)
    store<u32>(p, 0x00B21C68u);
    struct M {
        u32 off, kind, out, changed;
    };
    static constexpr M kFirst[4] = {{12, 0, 140, 151}, {24, 1, 136, 150}, {16, 2, 132, 149}, {20, 3, 128, 148}};
    for (const M& e : kFirst) {
        const u32 m = MapReleaseValues(ctx, p, e.off);
        if (m != 0) MapEraseAll(ctx, m, e.kind, sp + e.out, sp + e.changed);
    }
    store<u16>(p + 4, static_cast<u16>(load<u16>(p + 4) - 1));
    const u32 v = load<u32>(p + 28);
    if (v != 0) {
        call(ctx, 0x0054EC1Cu, {v});   // PtrVector_Clear
        call(ctx, 0x0054EC18u, {v});   // Nop
        call(ctx, 0x00552420u, {load<u32>(p + 28), 20});   // SmallFree
    }
    static constexpr M kSecond[4] = {{24, 1, 124, 147}, {20, 3, 120, 146}, {16, 2, 116, 145}, {12, 0, 112, 144}};
    for (const M& e : kSecond) {
        const u32 m = load<u32>(p + e.off);
        if (m == 0) continue;
        MapEraseAll(ctx, m, e.kind, sp + e.out, sp + e.changed);
        call(ctx, 0x00552420u, {load<u32>(p + e.off), 104});
    }
    store<u32>(p, 0x00B11B20u);
    ctx.gpr[3] = 0x00B11B20u;
}
