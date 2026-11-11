// Game code, part 34: the small-function sweep continued from the last 65
// instruction functions (size order; obj33*.cpp did 61-65). Each was
// checked against the original in the PPC simulator (memory writes, calls
// and their arguments, r3 and f1).
#include "obj34.h"

using namespace sly4;
using namespace sly4::obj34;

// ---------------------------------------------------------------- sweep: 65 instructions
// T f(T* o, X* x, u32 v): unless x is 0, lacks flag 0x40000 in +172 or has
// no +168, the pair {x, v} appended to o's u16-counted 8-byte vector +96
// (capacity +100 doubled when full, count +104, high-water +106) when x is
// not in it yet. r3 = o for no x, 0 for a missing flag / +168, the count
// when x is already there, else the old high-water.
DECOMP(003A95F4, PairVec_AddUnique_003A95F4) {
    const u32 o = arg<u32>(ctx, 0), x = arg<u32>(ctx, 1), v = arg<u32>(ctx, 2);
    if (x == 0) return;  // r3 = o
    if (((load<u32>(x + 172) >> 18) & 1u) == 0 || load<u32>(x + 168) == 0) {
        ctx.gpr[3] = 0;
        return;
    }
    const u32 n = load<u16>(o + 104);
    for (u32 i = 0, p = load<u32>(o + 96); i < n; ++i, p += 8) {
        if (load<u32>(p) == x) {
            ctx.gpr[3] = n;
            return;
        }
    }
    const u32 cap = load<u16>(o + 100);
    if (n == cap) call(ctx, 0x00551718, {o + 96, (cap << 1) & 0x1FFFEu});
    const u32 c = load<u16>(o + 104);
    const u32 e = load<u32>(o + 96) + ((c << 3) & 0x7FFF8u);
    const u32 hw = load<u16>(o + 106);
    store<u32>(e, x);
    const u32 n2 = c + 1u;
    store<u32>(e + 4, v);
    store<u16>(o + 104, static_cast<u16>(n2));
    store<u16>(o + 106, static_cast<u16>(static_cast<s32>(hw) > static_cast<s32>(n2 & 0xFFFFu) ? hw : n2));
    ctx.gpr[3] = hw;
}

// bool f(T* o, const s32* ev): 1 (nothing done) when o's +56 state has
// type (u8 +18) 6, o's u8 +466 is set or *ev is outside 0..2; else by
// o->vt[32](o, 0): 1 -> 0x003B2718(o, ev), 2 -> 0x003B1E88(o, ev) (r3 =
// their result), 0 -> 0x003B0BA8(o) when *ev is 2 (r3 = 1).
DECOMP(003B3014, State_HandleEvent_003B3014) {
    const u32 o = arg<u32>(ctx, 0), ev = arg<u32>(ctx, 1);
    u32 r = 1;
    const u32 st = load<u32>(o + 56);
    if (!(st != 0 && load<u8>(st + 18) == 6) && load<u8>(o + 466) == 0) {
        const s32 e = static_cast<s32>(load<u32>(ev));
        if (e >= 0 && e <= 2) {
            const s32 k = static_cast<s32>(VCall(ctx, o, 32, {o, 0}));
            if (k == 1) r = static_cast<u32>(call(ctx, 0x003B2718, {o, ev}));
            else if (k == 2) r = static_cast<u32>(call(ctx, 0x003B1E88, {o, ev}));
            else if (k == 0 && load<u32>(ev) == 2) call(ctx, 0x003B0BA8, {o});
        }
    }
    ctx.gpr[3] = r;
}

// void f(u32 i, bool on, bool down): entry i of the u16-counted pointer
// table 0x010AA694 (data +0, count +8; i = -1 or out of range does
// nothing) gets bit 0x40000000 of +24 = on; with down, the same for every
// later entry whose +28 parent is entry i, recursively. r3 = i, or the
// table's count after a scan.
DECOMP(0042BD94, Node_SetFlagTree_0042BD94) {
    const u32 i = arg<u32>(ctx, 0), on = arg<u32>(ctx, 1), down = arg<u32>(ctx, 2) & 0xFFu;
    constexpr u32 tab = 0x010AA694u;
    if (i == 0xFFFFFFFFu || i >= load<u16>(tab + 8)) return;
    const u32 e = load<u32>(load<u32>(tab) + (i << 2));
    if (e == 0) return;
    store<u32>(e + 24, (load<u32>(e + 24) & ~0x40000000u) | ((on & 1u) << 30));
    if (down == 0) return;
    for (u32 j = i + 1u;; ++j) {
        const u32 n = load<u16>(tab + 8);
        if (j >= n) {
            ctx.gpr[3] = n;
            return;
        }
        if (load<u32>(load<u32>(load<u32>(tab) + (j << 2)) + 28) == e) call(ctx, 0x0042BD94, {j, on & 0xFFu, down});
    }
}

// Static initialiser: the pointer tables 0x010AA694 / 0x010AA6A8
// (PtrVector_Construct, u16 +6 = 4, PtrVector_Init(t, 352, 0, 0)) and the
// tuning variables 0x010AA6C0 "..." (0x00AE21D8) / 0x010AA710 (0x00AE21F0)
// (TuningVar base, vtable 0x00B0D934, u8 +72 = 0). r3 as the last left it.
DECOMP(0042CB0C, Node_StaticInit_0042CB0C) {
    for (const u32 t : {0x010AA694u, 0x010AA6A8u}) {
        call(ctx, 0x005516E0, {t});
        store<u16>(t + 6, 4);
        call(ctx, 0x00551598, {t, 352, 0, 0});
    }
    call(ctx, 0x004FBFA8, {0x010AA6C0u, 0x00AE21D8u});
    store<u32>(0x010AA6C0, 0x00B0D934u);
    store<u8>(0x010AA6C0 + 72, 0);
    call(ctx, 0x004FBFA8, {0x010AA710u, 0x00AE21F0u});
    store<u32>(0x010AA710, 0x00B0D934u);
    store<u8>(0x010AA710 + 72, 0);
}

// T::T(T* o, a, b, c; f1 x, f2 y; d, u32 e, stack: u8 k (+124), bool f0
// (+116), bool f3 (+132), bool f4 (+140)): 0x0042C9D0(o, k), vtable
// 0x00B21040, +12 = 0, +16 = a, +20 = b, +24 = c, +28 = x, +32 = y, +36 =
// d, and the flag bits of +40: 0x80000000 = e, 0x40000000 = f0,
// 0x20000000 cleared, 0x10000000 = f3, 0x08000000 = f4. r3 = that word.
DECOMP(004321A4, VtB21040_Ctor_004321A4) {
    const u32 o = arg<u32>(ctx, 0), a = arg<u32>(ctx, 1), b = arg<u32>(ctx, 2), c = arg<u32>(ctx, 3);
    const u32 d = arg<u32>(ctx, 6), e = arg<u32>(ctx, 7);
    const f64 x = ctx.fpr[1], y = ctx.fpr[2];
    const u32 f0 = StkArg34(ctx, 116), k = StkArg34(ctx, 124), f3 = StkArg34(ctx, 132), f4 = StkArg34(ctx, 140);
    call(ctx, 0x0042C9D0, {o, k & 0xFFu});
    u32 w = load<u32>(o + 40) & 0x7FFFFFFFu;
    w |= (e & 1u) << 31;
    w = (w & ~0x40000000u) | ((f0 & 1u) << 30);
    w &= ~0x20000000u;
    store<u32>(o + 12, 0);
    store<u32>(o, 0x00B21040u);
    store<u32>(o + 16, a);
    w = (w & ~0x10000000u) | ((f3 & 1u) << 28);
    store<u32>(o + 20, b);
    store<u32>(o + 24, c);
    w = (w & ~0x08000000u) | ((f4 & 1u) << 27);
    Stfs32(o + 28, x);
    Stfs32(o + 32, y);
    store<u32>(o + 36, d);
    store<u32>(o + 40, w);
    ctx.gpr[3] = w;
}

// ~T() body: *0x00F65BE8 = 0; the block +80 freed (Mem_FreeFrom(b, 0))
// and cleared; then the allocator-backed u16 vectors +28 and +4 freed and
// destructed. r3 as Array_Destruct left it.
DECOMP(00451A6C, Obj_Dtor_00451A6C) {
    const u32 o = arg<u32>(ctx, 0);
    store<u32>(0x00F65BE8, 0);
    const u32 b = load<u32>(o + 80);
    if (b != 0) {
        call(ctx, 0x0054EE6C, {b, 0});
        store<u32>(o + 80, 0);
    }
    FreeAllocVec34(ctx, o + 28);
    FreeAllocVec34(ctx, o + 4);
}

// void f(T* o): with no entry for the current mode in o's table
// (o->vt->+12)[*(*0x010ABD80)->+64], each child c of o (+32, chained by
// +72) gets c->vt[24](c) and Obj_Tick4536C0(c; f1 = *0x00456564); else
// for i below that entry (re-read each pass), r = Obj_Resolve453A68(o, 1)
// and, when set, Obj_Activate452058(r). r3 as left (0 / the entry when no
// work was done).
DECOMP(00456568, Obj_TickOrActivate_00456568) {
    const u32 o = arg<u32>(ctx, 0);
    constexpr u32 g = 0x010ABD70u;
    const auto entry = [&]() { return load<u32>(load<u32>(load<u32>(o) + 12) + (load<u32>(load<u32>(g + 16) + 64) << 2)); };
    const u32 v = entry();
    if (v == 0) {
        ctx.gpr[3] = 0;
        for (u32 c = load<u32>(o + 32); c != 0; c = load<u32>(c + 72)) {
            const f64 dt = Lfs32(0x00456564);
            VCall(ctx, c, 24, {c});
            callf(ctx, 0x004536C0, {c}, {dt});
        }
        return;
    }
    for (s32 i = 0;; ++i) {
        const u32 n = entry();
        ctx.gpr[3] = n;
        if (!(i < static_cast<s32>(n))) return;
        const u32 r = static_cast<u32>(call(ctx, 0x00453A68, {o, 1}));
        if (r != 0) call(ctx, 0x00452058, {r});
    }
}

// void f(): Session_Reset2E0860(*0x00F64C20, 0); the "..." (0x00AE8AF8)
// UI element (Ui_Find) gets vt[88](u, 0); then the world w = *0x00F65CA0
// takes its u8 +47 from *w->+8: 255 -> +48 / +47 cleared and
// StateMachine_SetState(w, 0, 0), else +48 / +49 = 0, +50 = 1, +56 = 0.
// After a re-read of +47, a u8 +50 of 0 or >= 12 sets state (w, 1, 7),
// any other sets +49 = 7. r3 as left (w or SetState's).
DECOMP(0048A57C, Game_ResetToMenu_0048A57C) {
    call(ctx, 0x002E0860, {load<u32>(0x00F64C20), 0});
    const u32 u = static_cast<u32>(call(ctx, 0x004D5544, {0x00AE8AF8u, 0}));
    if (u != 0) VCall(ctx, u, 88, {u, 0});
    u32 w = load<u32>(0x00F65CA0);
    const u32 b = load<u8>(load<u32>(w + 8));
    store<u8>(w + 47, static_cast<u8>(b));
    if (b == 255) {
        store<u8>(w + 48, 0);
        store<u8>(w + 47, 0);
        // r6 = w, r7 = 0 are left over from the stores.
        call(ctx, 0x00464D1C, {w, 0, 0});
    } else {
        store<u8>(w + 48, 0);
        store<u8>(w + 49, 0);
        store<u8>(w + 50, 1);
        store<u32>(w + 56, 0);
    }
    w = load<u32>(0x00F65CA0);
    const u32 st = load<u8>(w + 50);
    store<u8>(w + 47, load<u8>(load<u32>(w + 8)));
    ctx.gpr[3] = w;
    if (st == 0 || static_cast<s32>(st) >= 12) call(ctx, 0x00464D1C, {w, 1, 7});
    else store<u8>(w + 49, 7);
}

// T* f(R* o, u32 key, u32* kind): *kind = 7; unless flag 0x80 of u8 +24
// is clear, the table h (o->+28 ? o->+28->+16[u16 +26] : (o->+16->+44 +
// 4 * u16 +26)->+20) is binary-searched by key in its 8-byte entries
// (+20, count u16 +14); on a hit, *kind = nibble i of the packed words
// +24 and r3 = entry->+4 (+ 4 for a kind of 0). r3 = 0 otherwise.
DECOMP(004F9768, Res_FindByKey_004F9768) {
    const u32 o = arg<u32>(ctx, 0), key = arg<u32>(ctx, 1), out = arg<u32>(ctx, 2);
    store<u32>(out, 7);
    ctx.gpr[3] = 0;
    if (((load<u8>(o + 24) >> 7) & 1u) == 0) return;
    const u32 t = load<u32>(o + 28);
    const u32 i4 = static_cast<u32>(load<u16>(o + 26)) << 2;
    const u32 h = t != 0 ? load<u32>(load<u32>(t + 16) + i4) : load<u32>(load<u32>(load<u32>(o + 16) + 44) + i4 + 20);
    s32 hi = static_cast<s32>(load<u16>(h + 14)) - 1, lo = 0;
    while (hi >= lo) {
        const s32 m = (hi + lo) >> 1;
        const u32 e = load<u32>(h + 20) + static_cast<u32>(m) * 8u;
        const u32 k = load<u32>(e);
        if (k > key) hi = m - 1;
        else if (k < key) lo = m + 1;
        else {
            if (e == 0) return;
            const u32 w = (load<u32>(h + 24 + ((static_cast<u32>(m) >> 1) & 0x7FFFFFFCu)) >> ((static_cast<u32>(m) << 2) & 0x1Cu)) & 0xFu;
            store<u32>(out, w);
            ctx.gpr[3] = w != 0 ? load<u32>(e + 4) : load<u32>(e + 4) + 4u;
            return;
        }
    }
}

// void f(Vec* v, u16 i): the 112-byte element i of the u16-counted vector
// v (data +0, count +8) removed by moving the last one over it (the
// count decremented first; nothing moved when i was the last). The copy
// takes the float triples +0/+16/+32/+48, the words +64..+80, the floats
// +76..+96 and the byte +100. r3 = v, or the byte and f1 the float +96
// after a move.
DECOMP(00735D10, Vec112_RemoveSwap_00735D10) {
    const u32 v = arg<u32>(ctx, 0);
    const u64 i = ctx.gpr[4];
    const u32 c = static_cast<u32>(load<u16>(v + 8)) - 1u;
    const u32 last = c & 0xFFFFu;
    store<u16>(v + 8, static_cast<u16>(c));
    if (static_cast<u32>(i) == last) return;
    const u32 d = load<u32>(v) + static_cast<u32>(i * 112u), s = load<u32>(v) + last * 112u;
    for (u32 k = 0; k < 64; k += 16)
        for (u32 j = 0; j < 12; j += 4) CopyW32(d + k + j, s + k + j);
    for (const u32 off : {64u, 76u, 68u, 84u, 72u, 88u, 80u, 92u, 96u}) CopyW32(d + off, s + off);
    store<u8>(d + 100, load<u8>(s + 100));
    ctx.gpr[3] = load<u8>(s + 100);
    ctx.fpr[1] = Lfs32(s + 96);
}

// T f(Tree** pp, u32 key): the search state n = (*pp)->+24 walks the
// binary tree n->+12->+0 (key +4, children +12 / +16 for smaller / not
// smaller keys), pushing each node passed on its s16-counted stack (+96
// count, +16 entries) and leaving the last node visited in +100. On a hit
// *0x011DE2A8 = node->+8 and r3 = r4 = ((*pp)->+24->+100)->+8; for no
// tree / no hit r3 = r4 = 0 (an empty tree also clears +100 and the
// global).
DECOMP(007609DC, Tree_Find_007609DC) {
    const u32 pp = arg<u32>(ctx, 0), key = arg<u32>(ctx, 1);
    const u32 t = load<u32>(pp);
    ctx.gpr[3] = 0;
    if (t == 0) return;
    const u32 n = load<u32>(t + 24);
    ctx.gpr[4] = 0;
    if (n == 0) return;
    u32 node = load<u32>(load<u32>(n + 12));
    if (node == 0) {
        store<u32>(n + 100, 0);
        store<u32>(0x011DE2A8, 0);
        return;
    }
    store<u16>(n + 96, 0);
    for (;;) {
        const u32 k = load<u32>(node + 4);
        if (k == key) {
            store<u32>(n + 100, node);
            store<u32>(0x011DE2A8, load<u32>(node + 8));
            u32 r = 0;
            const u32 m = load<u32>(t + 24);
            if (m != 0) {
                const u32 c = load<u32>(m + 100);
                if (c != 0) r = load<u32>(c + 8);
            }
            ctx.gpr[3] = ctx.gpr[4] = r;
            return;
        }
        const u32 nx = k <= key ? load<u32>(node + 16) : load<u32>(node + 12);
        if (nx == 0) {
            // The original's branch for a null last node (count and +100
            // cleared) cannot be reached.
            store<u32>(n + 100, node);
            return;
        }
        const s32 c = static_cast<s16>(load<u16>(n + 96));
        store<u16>(n + 96, static_cast<u16>(c + 1));
        store<u32>(n + 16 + static_cast<u32>(c) * 4u, node);
        node = nx;
    }
}

// ---------------------------------------------------------------- sweep: 66 instructions
// T f(T* o): one of o's candidates +120 / +124 / +128 picked with the
// game's random generator (state *0x01164890; each roll stores the next
// state and its low half as a u64 at 0x01164888): +128 when set and a
// roll is divisible by 3, else +124 when set and a roll is odd, else
// +120. r3 = r4 = pick->+12[mode] (mode = (*0x00F94B24)->+64), 0 for no
// pick.
DECOMP(00061C94, Obj_PickRandomVariant_00061C94) {
    const u32 o = arg<u32>(ctx, 0);
    u32 r = load<u32>(o + 120);
    const u32 c = load<u32>(o + 128), b = load<u32>(o + 124);
    constexpr u32 G = 0x01164888u;
    const auto roll = [&]() {
        const u32 s = load<u32>(G + 8);
        const u32 x = (std::rotl(s, 4) + std::rotl(s, 16)) ^ 0x87654321u;
        store<u32>(G + 8, std::rotl(x, 16));
        store<u64>(G, x & 0xFFFFu);
        return s;
    };
    bool picked = false;
    if (c != 0 && roll() % 3u == 0) {
        r = c;
        picked = true;
    }
    if (!picked && b != 0 && (roll() & 1u) != 0) r = b;
    u32 v = 0;
    if (r != 0) v = load<u32>(load<u32>(r + 12) + (load<u32>(load<u32>(0x00F94B24) + 64) << 2));
    ctx.gpr[3] = ctx.gpr[4] = v;
}

// T::T(T* o, Xml* x): Obj_InitVt_B21C98(o), vtable 0x00B106C0, +12 = -1,
// +16 = +20 = 0, then the s32s 0x05821EA3 (+12) / 0x04DA9951 (+16) and
// the f32 0x21CA4030 (+20) read with their current values as defaults;
// +12 decremented and +16 clamped to -128..127. r3 = +16, f1 as
// XmlCursor_ReadF32 left it.
DECOMP(0008C72C, VtB106C0_CtorXml_0008C72C) {
    const u32 o = arg<u32>(ctx, 0), x = arg<u32>(ctx, 1);
    call(ctx, 0x004BB3B0, {o, x});
    store<u32>(o + 12, 0xFFFFFFFFu);
    store<u32>(o, 0x00B106C0u);
    store<u32>(o + 16, 0);
    store<u32>(o + 20, 0);
    call(ctx, 0x004F99BC, {x, 0x05821EA3u, o + 12, ~0ull});
    call(ctx, 0x004F99BC, {x, 0x04DA9951u, o + 16, Sx33(load<u32>(o + 16))});
    callf(ctx, 0x004F9B20, {x, 0x21CA4030u, o + 20}, {Lfs32(o + 20)});
    const u32 a = load<u32>(o + 12);
    s32 v = static_cast<s32>(load<u32>(o + 16));
    store<u32>(o + 12, a - 1u);
    if (v > 127 || v < -128) {
        v = v < -128 ? -128 : v;
        v = v > 127 ? 127 : v;
        store<u32>(o + 16, static_cast<u32>(v));
    }
    ctx.gpr[3] = static_cast<u64>(static_cast<s64>(v));
}

// void f(u32* done, Tab* t): starting from *0x00F645B8 + 8 * *0x00F645BC,
// each of t's 48-byte entries (count +0, data +8) subtracts *0x00F645B8
// when its name equals "..." (0x00AC6220), else *0x00F645BC, while the
// total stays positive; *0x00F645B0 = the total (0 when negative) and
// *done = 1. r3 = that total.
DECOMP(000B1E6C, Budget_Compute_000B1E6C) {
    const u32 done = arg<u32>(ctx, 0), t = arg<u32>(ctx, 1);
    s32 left = static_cast<s32>(load<u32>(0x00F645B8) + (load<u32>(0x00F645BC) << 3));
    for (u32 i = 0, off = 0; left > 0 && i < load<u32>(t); ++i, off += 48) {
        const u32 eq = static_cast<u32>(call(ctx, 0x00555D94, {load<u32>(t + 8) + off, 0x00AC6220u}));
        left -= static_cast<s32>(load<u32>(eq == 0 ? 0x00F645B8u : 0x00F645BCu));
    }
    const u32 r = left < 0 ? 0u : static_cast<u32>(left);
    store<u32>(0x00F645B0, r);
    store<u32>(done, 1);
    ctx.gpr[3] = r;
}

// void f(T* o): by o->vt[32](o, 2): 0 -> the effect +356 (when set) gets
// hash 0x8EEFCC97 and o->vt[28](o, 2, 1); 1 -> when (o->+12)->+8 >=
// (o->+216)->+700, o->vt[28](o, 1, 1). r3 as left (o->+12 when the 1
// path makes no call); f1 = the threshold on the 1 path.
DECOMP(000BE9DC, Obj_StateStep_000BE9DC) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 h = load<u32>(o + 216);
    const s32 k = static_cast<s32>(VCall(ctx, o, 32, {o, 2}));
    if (k == 0) {
        const u32 e = load<u32>(o + 356);
        if (e != 0) call(ctx, 0x004CF988, {e, 0x8EEFCC97u});
        VCall(ctx, o, 28, {o, 2, 1});
    } else if (k == 1) {
        const f64 lim = Lfs32(h + 700), v = Lfs32(load<u32>(o + 12) + 8);
        ctx.fpr[1] = lim;
        ctx.fpr[2] = v;
        ctx.gpr[3] = load<u32>(o + 12);
        if (v >= lim) VCall(ctx, o, 28, {o, 1, 1});
    }
}

// void f(T* o): StateMachine_Init(o); an id +204 of 0 / -1 replaced by
// World_Counter_004A2560(); o->vt[104](o, id); each entry e of the u16
// pointer vector +248 gets e->vt[20](e, o); then
// Obj_PlaceAttachments(o). r3 as that left it.
DECOMP(000D82BC, Obj_Spawn_000D82BC) {
    const u32 o = arg<u32>(ctx, 0);
    call(ctx, 0x00464B18, {o});
    u32 id = load<u32>(o + 204);
    if (id == 0 || id == 0xFFFFFFFFu) {
        id = static_cast<u32>(call(ctx, 0x004A2560, {}));
        store<u32>(o + 204, id);
    }
    VCall(ctx, o, 104, {o, id});
    for (u32 i = 0;; ++i) {
        const u32 v = load<u32>(o + 248);
        if (!(i < (v != 0 ? load<u16>(v + 8) : 0u))) break;
        const u32 e = load<u32>(load<u32>(v) + 4 * i);
        VCall(ctx, e, 20, {e, o});
    }
    call(ctx, 0x000D7A00, {o});
}

// void f(T* o, u16 g): with the world's physics group set p =
// (*0x00F65CA0->+0x1281D4)->+4, each body b = entry->+28 of o's 48-byte
// u16 vector +388 gets its group (high half of +44, low 5 bits kept) =
// g, and Hk_UpdateEntityGroup(p, b, 0, 1) when it has a world (+8); then
// +412 = g and 0x000DA138(o, g). r3 as that left it.
DECOMP(000E6448, Obj_SetPhysGroup_000E6448) {
    const u32 o = arg<u32>(ctx, 0), g = arg<u32>(ctx, 1);
    const u32 p = load<u32>(load<u32>(load<u32>(0x00F65CA0) + 0x1281D4u) + 4);
    if (p != 0) {
        for (u32 i = 0, off = 0;; ++i, off += 48) {
            const u32 v = load<u32>(o + 388);
            if (!(i < (v != 0 ? load<u16>(v + 8) : 0u))) break;
            const u32 b = load<u32>(load<u32>(v) + off + 28);
            if (b == 0) continue;
            const u32 w = load<u32>(b + 8);
            store<u32>(b + 44, (load<u32>(b + 44) & 0x1Fu) | (g << 16));
            if (w != 0) call(ctx, 0x00930B70, {p, b, 0, 1});
        }
    }
    store<u32>(o + 412, g);
    call(ctx, 0x000DA138, {o, g});
}

// void f(Layer* o): CloseLayer34 with the delay *0x0010B238.
DECOMP(0010B240, Layer_Close_0010B240) { CloseLayer34(ctx, 0x0010B238); }

// void f(Layer* o): CloseLayer34 with the delay *0x00113348.
DECOMP(00113350, Layer_Close_00113350) { CloseLayer34(ctx, 0x00113348); }

// bool f(T* o): 1 when flag 0x01000000 of o's +188 is set or o->vt[32]
// reports 1 for phase 0 and 7 for phase 1; else 1 only when flag 0x20 of
// +188 is set, (+172 >> 22) & 3 is 1 and the session's current part p
// (Session_CurrentPart2DDD24(*0x00F64C20)) exists with p->+724 ==
// (o->+216)->+980.
DECOMP(00129894, Obj_IsPartActive_00129894) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 d = load<u32>(o + 216);
    if ((load<u32>(o + 188) & 0x01000000u) != 0) {
        ctx.gpr[3] = 1;
        return;
    }
    if (static_cast<s32>(VCall(ctx, o, 32, {o, 0})) == 1 && static_cast<s32>(VCall(ctx, o, 32, {o, 1})) == 7) {
        ctx.gpr[3] = 1;
        return;
    }
    const u32 p = static_cast<u32>(call(ctx, 0x002DDD24, {load<u32>(0x00F64C20)}));
    ctx.gpr[3] = (load<u32>(o + 188) & 0x20u) != 0 && ((load<u32>(o + 172) >> 22) & 3u) == 1 && p != 0 &&
                 load<u32>(d + 980) == load<u32>(p + 724);
}

// void f(T* o; f1 dt): unless the timer +696 is at *0x001731F0 (k[0]),
// it counts down by dt (stopping at k[0], stored as 0); u = 1 - t / +700
// shapes a damped wobble: +708 = (1 - u)^2 * sin(u * k[2] * k[3] * +716)^2
// * +712 + (+704 * (1 - u) + u), +704 = +704 * k[4] + k[5], and
// Obj_MoveTo0D7904(o, {+708 x3}, 0). r3 / f1 as that left them (o / dt
// when idle).
DECOMP(00173208, Obj_WobbleScale_00173208) {
    const u32 o = arg<u32>(ctx, 0);
    constexpr u32 K = 0x001731F0u;
    const f64 k0 = Lfs32(K);
    const f64 t = Lfs32(o + 696);
    if (t == k0) return;
    f64 a = static_cast<f32>(t - ctx.fpr[1]);
    Stfs32(o + 696, a);
    if (a < k0) {
        a = k0;
        store<u32>(o + 696, 0);
    }
    const f64 u = static_cast<f32>(a / Lfs32(o + 700));
    const f64 k1 = Lfs32(K + 4), k2 = Lfs32(K + 8), k3 = Lfs32(K + 12);
    const f64 v = static_cast<f32>(k1 - u);
    const f64 w = static_cast<f32>(k1 - v);
    const f64 base = static_cast<f32>(std::fma(Lfs32(o + 704), w, v));
    const f64 arg_ = static_cast<f32>(static_cast<f32>(static_cast<f32>(v * k2) * k3) * Lfs32(o + 716));
    const f64 s = callf(ctx, 0x007345F0, {}, {arg_});
    const f64 amp = Lfs32(o + 712), cur = Lfs32(o + 704);
    const f64 y = static_cast<f32>(std::fma(static_cast<f32>(static_cast<f32>(static_cast<f32>(w * w) * s) * s), amp, base));
    Stfs32(o + 708, y);
    Stack frame(ctx, 16);
    const u32 sc = frame.addr();  // sp+112
    Stfs32(sc, y);
    Stfs32(sc + 4, y);
    Stfs32(sc + 8, y);
    const f64 n = static_cast<f32>(std::fma(cur, Lfs32(K + 16), Lfs32(K + 20)));
    Stfs32(o + 704, n);
    ctx.fpr[1] = n;
    call(ctx, 0x000D7904, {o, sc, 0});
}

// void f(T* o, U* u): Obj_CopyQuery_000CDB28(o, u), then o's u16-counted
// 12-byte points (+180, count +188) appended to u's vector +144
// (capacity +148 doubled when full, count +152, high-water +154). r3 =
// o's count, f1 the last float copied.
DECOMP(001A5FF4, Obj_CopyPoints_001A5FF4) {
    const u32 o = arg<u32>(ctx, 0), u = arg<u32>(ctx, 1);
    call(ctx, 0x000CDB28, {o, u});
    for (u32 i = 0, off = 0;; ++i, off += 12) {
        const u32 n = load<u16>(o + 188);
        ctx.gpr[3] = n;
        if (i >= n) return;
        const u32 src = load<u32>(o + 180) + off;
        const u32 cap = load<u16>(u + 148);
        if (load<u16>(u + 152) == cap) call(ctx, 0x00551718, {u + 144, (cap << 1) & 0x1FFFEu});
        const u32 c = load<u16>(u + 152);
        const u32 dst = load<u32>(u + 144) + c * 12u;
        CopyW32(dst, src);
        CopyW32(dst + 4, src + 4);
        CopyW32(dst + 8, src + 8);
        ctx.fpr[1] = Lfs32(src + 8);
        const u32 c2 = load<u16>(u + 152) + 1u;
        const u32 hw = load<u16>(u + 154);
        store<u16>(u + 152, static_cast<u16>(c2));
        store<u16>(u + 154, static_cast<u16>(static_cast<s32>(hw) > static_cast<s32>(c2 & 0xFFFFu) ? hw : c2));
    }
}

// T::T(T* o, const S* s): Thunk_000E27FC(o, s), vtable 0x00B15BD8, the
// floats s+224..+232 to o+304.. and s+240..+288 to o+320..; the u8 +372
// and the flags 0x00800000..0x00100000 of the word +372 taken from s's
// +292 (stored after each bit). r3 = that word, f1 = s->+288.
DECOMP(001AF598, VtB15BD8_CtorCopy_001AF598) {
    const u32 o = arg<u32>(ctx, 0), s = arg<u32>(ctx, 1);
    call(ctx, 0x000E27FC, {o, s});
    for (u32 k = 0; k < 12; k += 4) CopyW32(o + 304 + k, s + 224 + k);
    for (u32 k = 0; k < 24; k += 4) CopyW32(o + 320 + k, s + 240 + k);
    store<u32>(o, 0x00B15BD8u);
    for (u32 k = 24; k < 52; k += 4) CopyW32(o + 320 + k, s + 240 + k);
    u32 w = load<u32>(o + 372);
    const u32 b = load<u8>(s + 292);
    store<u8>(o + 372, static_cast<u8>(b));
    w = (w & ~0xFF000000u) | (b << 24);
    for (const u32 m : {0x00800000u, 0x00400000u, 0x00200000u, 0x00100000u}) {
        w = (w & ~m) | (load<u32>(s + 292) & m);
        store<u32>(o + 372, w);
    }
    ctx.gpr[3] = w;
    ctx.fpr[1] = Lfs32(s + 288);
}

// void f(T* o, Msg* m): ArmCheck34 with k = *0x001BFAFC and the +240
// float.
DECOMP(001BFB00, Obj_ArmOnMsg_001BFB00) { ArmCheck34(ctx, 0x001BFAFC, 240, false); }

// void f(T* o, Msg* m): ArmCheck34 with k = *0x001BFD20 and the +276
// float (sender flag tested first).
DECOMP(001BFD28, Obj_ArmOnMsg_001BFD28) { ArmCheck34(ctx, 0x001BFD20, 276, true); }

// T::T(T* o): Thunk_000E40B4(o), the vtables 0x00B18260 / +380
// 0x00B1840C / +384 0x00B18434, +424..+448 cleared, the parts +432..+444
// looked up by the keys (o->+216)->+352..+364 (Obj_FindByKey_000D1454),
// then the part +440 (when found) gets vt[40](p, 0, 0). r3 as left.
DECOMP(001E9C90, VtB18260_Ctor_001E9C90) {
    const u32 o = arg<u32>(ctx, 0);
    call(ctx, 0x000E40B4, {o, ctx.gpr[4]});  // r4 passed through as the caller left it
    const u32 d = load<u32>(o + 216);
    constexpr u32 vt = 0x00B18258u;
    store<u32>(o + 380, vt + 436);
    store<u32>(o + 384, vt + 476);
    store<u32>(o, vt + 8);
    for (u32 k = 424; k <= 448; k += 4) store<u32>(o + k, 0);
    for (u32 k = 0; k < 16; k += 4) {
        const u32 key = load<u32>(d + 352 + k);
        // The first lookup leaves r5 = the key (unused by the callee).
        const u32 r = static_cast<u32>(k == 0 ? call(ctx, 0x000D1454, {o, key, key}) : call(ctx, 0x000D1454, {o, key}));
        store<u32>(o + 432 + k, r);
    }
    const u32 p = load<u32>(o + 440);
    ctx.gpr[3] = p;
    if (p != 0) VCall(ctx, p, 40, {p, 0, 0});
}

// void f(T* o, U* u): for u (not 0): when flag 0x00200000 of
// (u->+216)->+372 is set, the effect e = (u->+160)->+260 (both set) gets
// e->vt[24](e, u->+160); u->vt[40](u, 0), and u->vt[40](u, 1) for flag
// 0x40000 of +172; o's +812 cleared when it is u. r3 as left (o for no u;
// 0 / +812).
DECOMP(001ED85C, Obj_ReleaseChild_001ED85C) {
    const u32 o = arg<u32>(ctx, 0), u = arg<u32>(ctx, 1);
    if (u == 0) return;
    if ((load<u32>(load<u32>(u + 216) + 372) & 0x00200000u) != 0) {
        const u32 c = load<u32>(u + 160);
        if (c != 0) {
            const u32 e = load<u32>(c + 260);
            if (e != 0) VCall(ctx, e, 24, {e, c});
        }
    }
    VCall(ctx, u, 40, {u, 0});
    if ((load<u32>(u + 172) & 0x40000u) != 0) VCall(ctx, u, 40, {u, 1});
    const u32 cur = load<u32>(o + 812);
    ctx.gpr[3] = cur;
    if (cur == u) {
        store<u32>(o + 812, 0);
        ctx.gpr[3] = 0;
    }
}

// void f(T* o): each of the six lists s = o->+372.. (count +36, 8-byte
// slots +16) is emptied: a slot's object e gets e->vt[312](e,
// (s->+4)->+8, s->+8) (entry in r6, OPD in r7), then the slot = {0,
// FLT_MAX}; count = 0. r3 as left.
DECOMP(00271AF8, Obj_FlushLists_00271AF8) {
    const u32 o = arg<u32>(ctx, 0);
    for (u32 j = 0; j < 6; ++j) {
        const u32 s = load<u32>(o + 372 + 4 * j);
        ctx.gpr[3] = load<u32>(s + 36);
        if (load<u32>(s + 36) != 0) {
            for (u32 i = 0, off = 0;;) {
                const u32 p = load<u32>(s + 16) + off;
                const u32 e = load<u32>(p);
                if (e != 0) {
                    const u32 opd = load<u32>(load<u32>(e) + 312);
                    call_ptr(ctx, opd, {e, load<u32>(load<u32>(s + 4) + 8), load<u32>(s + 8), load<u32>(opd), opd});
                }
                store<u32>(p, 0);
                ++i;
                off += 8;
                store<u32>(p + 4, 0x7F7FFFFFu);
                ctx.gpr[3] = 0x7F7FFFFFu;
                if (!(i < load<u32>(s + 36))) break;
            }
        }
        store<u32>(s + 36, 0);
    }
}

// void f(T* o): the world w = *0x00F65CA0 reset for a cut: o's +392 and
// w's +0x128588 = 1.0, +0x12858C = -1.0, the u8 +0x128590 = 1,
// Rec_Set_0048EF50(w + 0x1282C0; k[0], k[0], k[1], k[2]) with k at
// 0x002C6734, flag 0x80000000 of +0x1282D8 cleared, and
// Obj_SetTimeTree_000DAB90 with o's +392 for o->+276 (r4 left as
// 0x1282D8) and, when its flag 1 of +188 is set, the world's +0x1281E0
// (r4 left as 0x1281E0). r3 as left.
DECOMP(002C6740, Cut_ResetTime_002C6740) {
    const u32 o = arg<u32>(ctx, 0);
    constexpr u32 K = 0x002C6734u;
    u32 w = load<u32>(0x00F65CA0);
    const f64 k0 = Lfs32(K);
    store<u32>(o + 392, 0x3F800000u);
    store<u32>(w + 0x128588u, 0x3F800000u);
    store<u32>(w + 0x12858Cu, 0xBF800000u);
    store<u8>(w + 0x128590u, 1);
    callf(ctx, 0x0048EF50, {w + 0x1282C0u}, {k0, k0, Lfs32(K + 4), Lfs32(K + 8)});
    w = load<u32>(0x00F65CA0);
    store<u32>(w + 0x1282D8u, load<u32>(w + 0x1282D8u) & 0x7FFFFFFFu);
    callf(ctx, 0x000DAB90, {load<u32>(o + 276), 0x1282D8u, 0}, {Lfs32(o + 392)});
    ctx.gpr[3] = 0;
    if ((load<u32>(load<u32>(o + 276) + 188) & 1u) == 0) return;
    const u32 t = load<u32>(load<u32>(0x00F65CA0) + 0x1281E0u);
    ctx.gpr[3] = t;
    if (t != 0) callf(ctx, 0x000DAB90, {t, 0x1281E0u, 0}, {Lfs32(o + 392)});
}

// T::T(T* o): the words +0..+96 cleared (the flags 0xE0000000 of +16 and
// 0xF0000000 of +100 cleared, 0x08000000 of +100 set), *0x00F64C20 = o
// (the session), the blocks +20 (10320 bytes) and +28 (0x12D80 bytes)
// allocated (Mem_Alloc(n, 0, 16)), then 0x002DCDD0(o,
// "Scripts/Episodes.xml"). r3
// as that left it.
DECOMP(002DD4FC, Session_Ctor_002DD4FC) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 w16 = load<u32>(o + 16) & 0x7FFFFFFFu & ~0x40000000u & ~0x20000000u;
    store<u32>(o + 4, 0);
    store<u32>(o, 0);
    store<u32>(o + 8, 0);
    store<u32>(o + 12, 0);
    const u32 w100 = (load<u32>(o + 100) & 0x7FFFFFFFu & ~0x40000000u & ~0x20000000u & ~0x10000000u) | 0x08000000u;
    store<u32>(o + 16, w16);
    for (const u32 k : {20u, 28u, 24u, 36u, 32u, 40u, 44u, 48u, 52u, 56u, 64u, 60u, 68u, 72u, 76u, 80u, 84u, 88u, 92u, 96u}) store<u32>(o + k, 0);
    store<u32>(o + 100, w100);
    store<u32>(0x00F64C20, o);
    store<u32>(o + 20, static_cast<u32>(call(ctx, 0x0054ED38, {10320, 0, 16})));
    store<u32>(o + 28, static_cast<u32>(call(ctx, 0x0054ED38, {0x12D80u, 0, 16})));
    call(ctx, 0x002DCDD0, {o, 0x00AD8B60u});
}

// bool f(T* o): 1 unless o->vt[32](o, 0) is 1; then, when the animator
// c = o->+32 has a c->vt[60](c) whose Anim_TrackSet36_0029879C(it, 0, 0)
// is 1, r3 = 1; else o->vt[28](o, 0, 2) and r3 = 0.
DECOMP(0031504C, Obj_CheckAnimDone_0031504C) {
    const u32 o = arg<u32>(ctx, 0);
    if (static_cast<s32>(VCall(ctx, o, 32, {o, 0})) != 1) {
        ctx.gpr[3] = 1;
        return;
    }
    const u32 c = load<u32>(o + 32);
    if (static_cast<u32>(VCall(ctx, c, 60, {c})) != 0) {
        const u32 x = static_cast<u32>(VCall(ctx, c, 60, {c}));
        if (static_cast<u32>(call(ctx, 0x0029879C, {x, 0, 0})) == 0) {
            ctx.gpr[3] = 1;
            return;
        }
    }
    VCall(ctx, o, 28, {o, 0, 2});
    ctx.gpr[3] = 0;
}

// Deleting ~T() (o may be 0): vtable 0x00B1F570, the allocator-backed u16
// vectors +76 and +52 freed and destructed, 0x00353334(o), then
// SmallFree(o, 108). r3 as that left it.
DECOMP(00354598, VtB1F570_Delete_00354598) {
    const u32 o = arg<u32>(ctx, 0);
    if (o == 0) return;
    store<u32>(o, 0x00B1F570u);
    FreeAllocVec34(ctx, o + 76);
    FreeAllocVec34(ctx, o + 52);
    call(ctx, 0x00353334, {o});
    call(ctx, 0x00552420, {o, 108});
}

// void f(T* o, X* x, s32 key): in o->+8's 32-byte table (+148, count
// +152) the first entry with +28 == key (x and key not 0); its object e
// = +16 (when set) gets e->vt[188](e, x, 0), then the entry's object
// (re-read) vt[148](it). r3 as left (x when nothing was called).
DECOMP(0036CCA0, Obj_ApplyToKeyed_0036CCA0) {
    const u32 x = arg<u32>(ctx, 1), key = arg<u32>(ctx, 2);
    const u32 t = load<u32>(arg<u32>(ctx, 0) + 8);
    ctx.gpr[3] = x;
    if (x == 0 || key == 0) return;
    const u32 n = load<u32>(t + 152);
    for (u32 i = 0; i < n; ++i) {
        if (load<u32>(load<u32>(t + 148) + 32 * i + 28) != key) continue;
        const u32 e = load<u32>(load<u32>(t + 148) + (i << 5) + 16);
        if (e == 0) return;
        VCall(ctx, e, 188, {e, x, 0});
        const u32 e2 = load<u32>(load<u32>(t + 148) + (i << 5) + 16);
        VCall(ctx, e2, 148, {e2});
        return;
    }
}

// void f(T* o): o->vt[20](o, 0), Obj_Place3626E8(o, 0x01164B40, 1), the
// parts +176 / +180 / +184 (when set) get vt[88](p, 0); then +204 = 0 and
// the word +160 = (its bit 1) | 1. r3 = that word.
DECOMP(0036F8C4, Obj_Reset_0036F8C4) {
    const u32 o = arg<u32>(ctx, 0);
    VCall(ctx, o, 20, {o, 0});
    call(ctx, 0x003626E8, {o, 0x01164B40u, 1});
    for (const u32 k : {176u, 180u, 184u}) {
        const u32 p = load<u32>(o + k);
        if (p != 0) VCall(ctx, p, 88, {p, 0});
    }
    const u32 w = (load<u32>(o + 160) & 1u) | 1u;
    store<u32>(o + 204, 0);
    store<u32>(o + 160, w);
    ctx.gpr[3] = w;
}

// void f(A* a, const Req* q): a knowledge-list request: q->+0 set ->
// Block_Alloc_0038D91C(a, q->+24, u16 +20, u16 +22, 0); else q->+1 set
// -> 0x0038D498(a, q->+12), on failure "Fail to load knowledge list (%s)
// from file (%s)" with the file's name (Xml_FindResolved_004F9434(x) +
// 48) and its "Name" attribute; else for the list h = *q->+4 a header
// flag 0x10 (u16 h - 146) gives "Ignore cloning operation of the
// knowledge list (...", otherwise 0x0038BAE0(a, h, q + 16, u8 q->+8). r3
// as left.
DECOMP(0038DB04, Knowledge_Load_0038DB04) {
    const u32 a = arg<u32>(ctx, 0), q = arg<u32>(ctx, 1);
    if (load<u8>(q) != 0) {
        call(ctx, 0x0038D91C, {a, load<u32>(q + 24), load<u16>(q + 20), load<u16>(q + 22), 0});
    } else if (load<u8>(q + 1) != 0) {
        if (static_cast<u32>(call(ctx, 0x0038D498, {a, load<u32>(q + 12)})) == 0) {
            const u32 name = static_cast<u32>(call(ctx, 0x004F9434, {load<u32>(q + 12)})) + 48u;
            const u32 attr = static_cast<u32>(call(ctx, 0x004F9628, {load<u32>(q + 12), 0x00ADD4CCu}));
            call(ctx, 0x005534D0, {0x00ADD67Cu, name, attr});
        }
    } else {
        const u32 h = load<u32>(load<u32>(q + 4));
        const u32 f = (load<u16>(h - 152 + 6) >> 4) & 1u;
        if (f != 0) call(ctx, 0x005534D0, {0x00ADD6ACu, 0x00B1FAC7u, f});
        else call(ctx, 0x0038BAE0, {a, h, q + 16, load<u8>(q + 8)});
    }
}

// bool f(T* o): o (with u8 +16 set) is visible: when u8 +32 is set and
// Obj_OffScreen_000DADA8(o->+8; f1 = k[0]) reports it, or when the
// screen point of o->+8's position (+144, View_ProjectPoint on the
// world's view) falls outside k[1]..k[2] on either axis (k at
// 0x003EBD24). r3 = 1 then, else 0; f1 as the last compare left it.
DECOMP(003EBD30, Obj_IsOnScreenEdge_003EBD30) {
    const u32 o = arg<u32>(ctx, 0);
    constexpr u32 K = 0x003EBD24u;
    u32 a = load<u8>(o + 16);
    if (a != 0 && load<u8>(o + 32) != 0 && load<u32>(o + 8) != 0) {
        callf(ctx, 0x000DADA8, {load<u32>(o + 8)}, {Lfs32(K)});
        const u32 r = static_cast<u32>(ctx.gpr[3]);
        a = load<u8>(o + 16);
        if (r != 0) {
            ctx.gpr[3] = 1;
            return;
        }
    }
    ctx.gpr[3] = 0;
    if (a == 0) return;
    const u32 p = load<u32>(o + 8);
    if (p == 0) return;
    Stack frame(ctx, 16);
    const u32 sp = frame.addr();  // sp+112
    call(ctx, 0x00522750, {load<u32>(load<u32>(0x00F65CA0) + 0x128168u), sp, p + 144});
    const f64 x = Lfs32(sp), lo = Lfs32(K + 4), hi = Lfs32(K + 8);
    ctx.gpr[3] = 1;
    if (x < lo) {
        ctx.fpr[1] = x;
        return;
    }
    ctx.fpr[1] = hi;
    if (x > hi) return;
    const f64 y = Lfs32(sp + 4);
    if (y < lo || y > hi) return;
    ctx.gpr[3] = 0;
}

// void f(Ref* o): for the object e = o->+0, its bone 0x0026D2FA (when
// found, b) gets the offset {-o->+20, -(b->+0)->+52, 0.64} at (b->+4)->
// +48..+56; with a world object w = e->+160, WorldObj_MarkDirty(w) and,
// when w->vt[28](w) is set, its u8 +31 |= 0x80 (vt[28] called again).
// Always ends with o->+0 = 0; r3 = 0.
DECOMP(0040DA8C, Ref_PlaceBone_0040DA8C) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 e = load<u32>(o);
    if (e != 0) {
        const u32 b = static_cast<u32>(call(ctx, 0x000DA9D0, {e, 0x0026D2FAu}));
        if (b != 0) {
            const u32 t = load<u32>(b + 4);
            // fneg on the raw float: the sign bit flipped.
            const u32 nx = load<u32>(o + 20) ^ (1u << 31), ny = load<u32>(load<u32>(b) + 52) ^ (1u << 31);
            store<u32>(t + 56, 0x3F23D70Au);
            store<u32>(t + 48, nx);
            store<u32>(t + 52, ny);
            ctx.fpr[1] = static_cast<f64>(std::bit_cast<f32>(nx));
            ctx.fpr[2] = static_cast<f64>(std::bit_cast<f32>(ny));
            const u32 w = load<u32>(load<u32>(o) + 160);
            if (w != 0) {
                call(ctx, 0x0057D000, {w});
                const u32 w1 = load<u32>(load<u32>(o) + 160);
                if (static_cast<u32>(VCall(ctx, w1, 28, {w1})) != 0) {
                    const u32 w2 = load<u32>(load<u32>(o) + 160);
                    const u32 r = static_cast<u32>(VCall(ctx, w2, 28, {w2}));
                    store<u8>(r + 31, static_cast<u8>(load<u8>(r + 31) | 0x80u));
                }
            }
        }
    }
    store<u32>(o, 0);
    ctx.gpr[3] = 0;
}

// void f(Swap* o; f1 t): with the world's view v, +12 = k[0] (k at
// 0x0048F14C) for a phase (s8 +8) outside 0..1, else (v->+656)->+40->
// +164; PostFx_UpdateSwap(o; k[1]) until the phase leaves 0..1; then +0
// = 1, +4 = 2, +16 = 0, +20 = t, +24 = 0, phase = 0 and
// PostFx_UpdateSwap(o; k[2]). r3 / f1 as that left them (0 / t without
// a view).
DECOMP(0048F158, PostFx_StartSwap_0048F158) {
    const u32 o = arg<u32>(ctx, 0);
    const f64 t = ctx.fpr[1];
    constexpr u32 K = 0x0048F14Cu;
    const u32 v = load<u32>(load<u32>(0x00F65CA0) + 0x128168u);
    ctx.gpr[3] = 0;
    if (v == 0) return;
    const auto out = [](u32 b) {
        const s32 m = static_cast<s8>(b);
        return m < 0 || m >= 2;
    };
    const u32 q = load<u32>(load<u32>(v + 656) + 40);
    u32 b = load<u8>(o + 8);
    CopyW32(o + 12, out(b) ? K : q + 164);
    while (!out(b)) {
        callf(ctx, 0x004695A8, {o}, {Lfs32(K + 4)});
        b = load<u8>(o + 8);
    }
    store<u32>(o, 1);
    store<u32>(o + 4, 2);
    store<u32>(o + 16, 0);
    Stfs32(o + 20, t);
    store<u32>(o + 24, 0);
    store<u8>(o + 8, 0);
    callf(ctx, 0x004695A8, {o}, {Lfs32(K + 8)});
}

// bool f(Xml* x): with the settings object s (*0x010C2C28; r3 = 0
// without), the bool "..." (0x00AF0144) to s+769, the floats 0x00AF0248 /
// 0x00AF0260 / 0x00AF022C (default *0x004EE64C) to s+772 / +776 / +780,
// the s32 0x00AF0268 (default 0) to the u8 s+784; u8 s+768 = 1. r3 = 1.
DECOMP(004EE650, Settings_ReadTuning_004EE650) {
    const u32 x = arg<u32>(ctx, 0);
    const u32 s = load<u32>(kGlobals10C2C28);
    if (s == 0) {
        ctx.gpr[3] = 0;
        return;
    }
    call(ctx, o33fn::XmlReadBoolByName, {x, 0x00AF0144u, s + 769, 0});
    const f64 k = Lfs32(0x004EE64C);
    callf(ctx, o33fn::XmlReadF32ByName, {x, 0x00AF0248u, s + 772}, {k});
    callf(ctx, o33fn::XmlReadF32ByName, {x, 0x00AF0260u, s + 776}, {k});
    callf(ctx, o33fn::XmlReadF32ByName, {x, 0x00AF022Cu, s + 780}, {k});
    Stack frame(ctx, 16);
    const u32 v = frame.addr();  // sp+112
    call(ctx, 0x004FA564, {x, 0x00AF0268u, v, 0});
    store<u8>(s + 784, static_cast<u8>(load<u32>(v)));
    store<u8>(s + 768, 1);
    ctx.gpr[3] = 1;
}

// Deleting ~T() (o may be 0): vtable 0x00B230D0, the buffer +44 (when
// set) returned to the default memory stack (st->vt[8](st, p), entry in
// r5, OPD in r6), the 64 32-byte members from +192 destructed from the
// back (vtable 0x00B23080 each), vtable 0x00B23080 for o, then
// SmallFree(o, 2240). r3 as that left it.
DECOMP(004FAAAC, VtB230D0_Delete_004FAAAC) {
    const u32 o = arg<u32>(ctx, 0);
    if (o == 0) return;
    store<u32>(o, 0x00B230D0u);
    const u32 p = load<u32>(o + 44);
    if (p != 0) {
        const u32 st = static_cast<u32>(call(ctx, o33fn::Mem_GetDefaultStack, {}));
        const u32 opd = load<u32>(load<u32>(st) + 8);
        call_ptr(ctx, opd, {st, load<u32>(o + 44), load<u32>(opd), opd});
    }
    for (u32 k = 64; k-- > 0;) store<u32>(o + 192 + 32 * k, 0x00B23080u);
    store<u32>(o, 0x00B23080u);
    call(ctx, 0x00552420, {o, 2240});
}

// ~T(): DtorB160A8_34.
DECOMP(00753258, VtB160A8_Dtor_00753258) { DtorB160A8_34(ctx, arg<u32>(ctx, 0)); }

// ---------------------------------------------------------------- sweep: 67 instructions
// void f(T* o, Vec3* p, -, u32 n; f1 dt): p pulled toward o's target w =
// o->+604 (its +128 point; read even without w, quirk kept): d = the
// x/z distance (y = k[0]; normalised to length k[3] past k[1], k at
// 0x00040E7C); when *0x00F96D2C - dt < |d|, p = (w's velocity +128 (k[0]
// without w) + d * (*0x00F96D2C - dt)) - d * (n * k[2] * dt) (n via a
// u64 stack slot, as the original converts it). r3 = &k, f1 = p->z (dt
// when unchanged).
DECOMP(00040E90, Obj_PullToward_00040E90) {
    const u32 o = arg<u32>(ctx, 0), p = arg<u32>(ctx, 1);
    const u32 n = static_cast<u32>(ctx.gpr[6]);
    const f64 dt = ctx.fpr[1];
    constexpr u32 K = 0x00040E7Cu;
    const f64 k0 = Lfs32(K);
    const u32 w = load<u32>(o + 604);
    f64 vx = k0, vy = k0, vz = k0;
    if (w != 0) {
        vx = Lfs32(w + 128);
        vy = Lfs32(w + 132);
        vz = Lfs32(w + 136);
    }
    const u32 c = w + 128u;
    ctx.gpr[3] = K;
    ctx.gpr[5] = c;
    f64 dx = static_cast<f32>(Lfs32(p) - Lfs32(c));
    f64 dy = k0;
    f64 dz = static_cast<f32>(Lfs32(p + 8) - Lfs32(c + 8));
    f64 d = static_cast<f32>(std::fma(dz, dz, static_cast<f32>(std::fma(dx, dx, static_cast<f32>(k0 * k0)))));
    if (d > Lfs32(K + 4)) {
        d = static_cast<f32>(std::sqrt(d));
        const f64 s = static_cast<f32>(Lfs32(K + 12) / d);
        dx = static_cast<f32>(dx * s);
        dy = static_cast<f32>(dy * s);
        dz = static_cast<f32>(dz * s);
    }
    const f64 lim = static_cast<f32>(Lfs32(0x00F96D2C) - dt);
    if (!(lim < d)) return;
    const f64 ax = static_cast<f32>(std::fma(dx, lim, vx));
    const f64 ay = static_cast<f32>(std::fma(dy, lim, vy));
    const f64 az = static_cast<f32>(std::fma(dz, lim, vz));
    {
        Stack frame(ctx, 16);
        store<u64>(frame.addr(), n);  // sp+48 of the original's frame
    }
    ctx.gpr[5] = n;
    const f64 m = static_cast<f32>(static_cast<f32>(static_cast<f32>(static_cast<f64>(n)) * Lfs32(K + 8)) * dt);
    const f64 x = static_cast<f32>(std::fma(-dx, m, ax));
    const f64 y = static_cast<f32>(std::fma(-dy, m, ay));
    const f64 z = static_cast<f32>(std::fma(-dz, m, az));
    Stfs32(p, x);
    Stfs32(p + 4, y);
    Stfs32(p + 8, z);
    ctx.fpr[1] = z;
}

// T* New(-, Xml* x): NewXmlList34 with vtable 0x00B0F928, bool 0x7416F03B.
DECOMP(0007AD68, VtB0F928_NewXml_0007AD68) { NewXmlList34(ctx, 0x00B0F928u, 0x7416F03Bu); }

// void f(E* e, X* x): v = e->+12, or the component c (Effect_FindComponent
// (x, e->+16)) value c->vt[20](c) when c->vt[32](c) is 1; when v >
// *0x0009F884, a 20-byte VtB20FC8 command (v) pushed with
// CmdList_Push_0042C138 (0 for a failed allocation). r3 / f1 as left
// (&k / k when nothing was pushed).
DECOMP(0009F888, Effect_PushValueCmd_0009F888) {
    const u32 e = arg<u32>(ctx, 0), x = arg<u32>(ctx, 1);
    f64 v = Lfs32(e + 12);
    const u32 k = load<u32>(e + 16);
    if (k != 0) {
        const u32 c = static_cast<u32>(call(ctx, 0x004BB4B4, {x, k}));
        if (c != 0 && static_cast<s32>(VCall(ctx, c, 32, {c})) == 1) {
            VCall(ctx, c, 20, {c});
            v = ctx.fpr[1];
        }
    }
    const f64 lim = Lfs32(0x0009F884);
    ctx.gpr[3] = 0x0009F884u;
    ctx.fpr[1] = lim;
    if (!(v > lim)) return;
    const u32 p = static_cast<u32>(call(ctx, 0x0055238C, {20}));
    if (p != 0) callf(ctx, 0x00431EEC, {p}, {v});
    call(ctx, 0x0042C138, {p});
}

// void f(Box* b, T* o): b emptied (min +0 = FLT_MAX, max +16 = -FLT_MAX);
// without a model m = o->+8, Box_SetPoint(b, origin 0x01164B40); else b
// = m's bounds ((m->+208) + 16, min +0 / max +16) scaled per axis by m's
// +144 scale. r3 = b (or as Box_SetPoint left it), f1 = max.x.
DECOMP(000CB934, Box_FromModel_000CB934) {
    const u32 b = arg<u32>(ctx, 0), o = arg<u32>(ctx, 1);
    store<u32>(b + 16, 0xFF7FFFFFu);
    store<u32>(b, 0x7F7FFFFFu);
    store<u32>(b + 4, 0x7F7FFFFFu);
    store<u32>(b + 8, 0x7F7FFFFFu);
    store<u32>(b + 20, 0xFF7FFFFFu);
    store<u32>(b + 24, 0xFF7FFFFFu);
    const u32 m = load<u32>(o + 8);
    if (m == 0) {
        call(ctx, 0x00556F60, {b, 0x01164B40u});
        return;
    }
    const u32 s = load<u32>(m + 208) + 16u, sc = m + 144;
    for (const u32 k : {0u, 4u, 8u, 16u, 20u, 24u}) CopyW32(b + k, s + k);
    f64 r[3];
    for (u32 i = 0; i < 3; ++i) r[i] = static_cast<f32>(Lfs32(b + 4 * i) * Lfs32(sc + 4 * i));
    for (u32 i = 0; i < 3; ++i) Stfs32(b + 4 * i, r[i]);
    for (u32 i = 0; i < 3; ++i) r[i] = static_cast<f32>(Lfs32(b + 16 + 4 * i) * Lfs32(sc + 4 * i));
    for (u32 i = 0; i < 3; ++i) Stfs32(b + 16 + 4 * i, r[i]);
    ctx.gpr[3] = b;
    ctx.fpr[1] = r[0];
}

// ~T(): DtorSkel34 with the skeleton +164 and vtable 0x00B123A0.
DECOMP(000DD8A4, VtB123A0_Dtor_000DD8A4) { DtorSkel34(ctx, 164, 0x00B123A0u); }

// ~T(): DtorSkel34 with the skeleton +400 and vtable 0x00B12C88.
DECOMP(000E9820, VtB12C88_Dtor_000E9820) { DtorSkel34(ctx, 400, 0x00B12C88u); }

// void f(T* o, Obj* u): u becomes o's +436 (the old one, when set, gets
// vt[40](it, 0); u's u16 +178 incremented); then o's holder s = o->+620
// (when set) takes u as its +48 the same way (old one vt[40](it, 0)),
// u's +178 incremented again when its flag 0x40000 of +172 is set.
// Nothing happens when u already is +436. r3 as left.
DECOMP(000EB2F0, Obj_SetFocus_000EB2F0) {
    const u32 o = arg<u32>(ctx, 0), u = arg<u32>(ctx, 1);
    const u32 cur = load<u32>(o + 436);
    ctx.gpr[3] = cur;
    if (u == cur) return;
    if (cur != 0) VCall(ctx, cur, 40, {cur, 0});
    store<u32>(o + 436, u);
    u32 f = u;
    if (u != 0) {
        store<u16>(u + 178, static_cast<u16>(load<u16>(u + 178) + 1));
        ctx.gpr[3] = u + 178u;
        f = load<u32>(o + 436);
    }
    const u32 s = load<u32>(o + 620);
    if (s == 0) return;
    const u32 e = load<u32>(s + 48);
    ctx.gpr[3] = e;
    if (e != 0) VCall(ctx, e, 40, {e, 0});
    store<u32>(s + 48, f);
    if (f == 0) return;
    ctx.gpr[3] = 0;
    if ((load<u32>(f + 172) & 0x40000u) == 0) return;
    store<u16>(f + 178, static_cast<u16>(load<u16>(f + 178) + 1));
    ctx.gpr[3] = f + 178u;
}

// void f(T* o): o->vt[28](o, 0, 0); each entry e of o's 8-byte u16 list
// (+424, count +432): its effect e->vt[272](e) (when set) gets hash
// 0xCE7C29A3, then e->vt[28](e, 0, 0). r3 = the count.
DECOMP(001AABD8, Obj_ResetParts_001AABD8) {
    const u32 o = arg<u32>(ctx, 0);
    VCall(ctx, o, 28, {o, 0, 0});
    for (u32 i = 0, off = 0;; ++i, off += 8) {
        const u32 n = load<u16>(o + 432);
        ctx.gpr[3] = n;
        if (i >= n) return;
        const u32 p = load<u32>(o + 424) + off;
        const u32 e = load<u32>(p);
        const u32 fx = static_cast<u32>(VCall(ctx, e, 272, {e}));
        if (fx != 0) call(ctx, 0x004CF988, {fx, 0xCE7C29A3u});
        const u32 e2 = load<u32>(p);
        VCall(ctx, e2, 28, {e2, 0, 0});
    }
}

// void f(T* o, Ev* e): for events 0xCCF83851 / 0x35007D87 (e->+8) whose
// +12 matches o's tracked object t = o->+428 (flag 0x40000 of +172, id
// +168), t is dropped (+428 = 0) and o->vt[28](o, 0, 1) / (o, 1, 0);
// then Obj_OnEvent_000E5E44(o, e). r3 as that left it.
DECOMP(001AD044, Obj_OnEventDrop_001AD044) {
    const u32 o = arg<u32>(ctx, 0), e = arg<u32>(ctx, 1);
    const u32 k = load<u32>(e + 8);
    if (k == 0xCCF83851u || k == 0x35007D87u) {
        const u32 t = load<u32>(o + 428);
        if (t != 0 && (load<u32>(t + 172) & 0x40000u) != 0 && load<u32>(t + 168) == load<u32>(e + 12)) {
            store<u32>(o + 428, 0);
            VCall(ctx, o, 28, {o, 0, 1});
            VCall(ctx, o, 28, {o, 1, 0});
        }
    }
    call(ctx, 0x000E5E44, {o, e});
}

// ~T(): vtables 0x00B159B8 / +380 0x00B15B8C / +384 0x00B15BB4, the seven
// refs +424 + 16 * i released (RefNode_Release), then DtorTail388_34. r3
// as that left it.
DECOMP(001AE468, VtB159B8_Dtor_001AE468) {
    const u32 o = arg<u32>(ctx, 0);
    constexpr u32 vt = 0x00B159B0u;
    store<u32>(o, vt + 8);
    store<u32>(o + 380, vt + 476);
    store<u32>(o + 384, vt + 516);
    for (u32 i = 0; i < 7; ++i) call(ctx, 0x004432DC, {load<u32>(o + 424 + 16 * i)});
    DtorTail388_34(ctx, o);
}

// void f(T* o, U* u): Obj_CopyQueryEx_000DFBFC(o, u), u's +220 = o's
// +256, then o's u16-counted 8-byte pairs (+236, count +244) appended to
// u's vector +200 (capacity +204 doubled when full, count +208 (cached as
// the original does), high-water +210). r3 = the count, f1 the last
// float copied.
DECOMP(001B57A4, Obj_CopyPairs_001B57A4) {
    const u32 o = arg<u32>(ctx, 0), u = arg<u32>(ctx, 1);
    call(ctx, 0x000DFBFC, {o, u});
    u32 cnt = load<u16>(u + 208);
    CopyW32(u + 220, o + 256);
    ctx.fpr[1] = Lfs32(o + 256);
    for (u32 i = 0, off = 0;; ++i, off += 8) {
        ctx.gpr[3] = cnt;
        if (i >= load<u16>(o + 244)) return;
        const u32 src = load<u32>(o + 236) + off;
        const u32 cap = load<u16>(u + 204);
        u32 idx = cnt;
        if ((cnt & 0xFFFFu) == cap) {
            call(ctx, 0x00551718, {u + 200, (cap << 1) & 0x1FFFEu});
            idx = load<u16>(u + 208);
        }
        const u32 dst = load<u32>(u + 200) + ((idx << 3) & 0x7FFF8u);
        CopyW32(dst, src);
        CopyW32(dst + 4, src + 4);
        ctx.fpr[1] = Lfs32(src + 4);
        const u32 c = load<u16>(u + 208) + 1u;
        const u32 hw = load<u16>(u + 210);
        store<u16>(u + 208, static_cast<u16>(c));
        u32 nhw;
        if (static_cast<s32>(hw) > static_cast<s32>(c & 0xFFFFu)) {
            nhw = hw;
            cnt = c & 0xFFFFu;
        } else {
            nhw = c;
            cnt = c;
        }
        store<u16>(u + 210, static_cast<u16>(nhw));
    }
}

// void f(T* o, s32 k, bool on): o's effect +356 updated for slot k: k 1
// / 4 use the hash tables 0x00F64A20 / 0x00F64A18 indexed by +556
// (Effect_SetHash4CF988 when on, else Effect_Release); any other k uses
// o's +432 + 16 * k entry (Effect_SetDef_004CF8D8 when on, else
// Effect_SetInstance_004CF330). r3 as left.
DECOMP(001C0010, Obj_SetSlotEffect_001C0010) {
    const u32 o = arg<u32>(ctx, 0), on = arg<u32>(ctx, 2) & 0xFFu;
    const u64 k64 = ctx.gpr[4];
    const s32 k = static_cast<s32>(k64);
    if (k == 1 || k == 4) {
        const u32 tab = k == 1 ? 0x00F64A20u : 0x00F64A18u;
        const u64 i4 = Sx33(load<u32>(o + 556) << 2);
        const u32 h = load<u32>(tab + static_cast<u32>(i4));
        // r5 is left as the sign-extended index * 4.
        call(ctx, on != 0 ? 0x004CF988u : 0x004CF3C4u, {load<u32>(o + 356), h, i4});
        return;
    }
    const u32 ent = o + static_cast<u32>(k64 << 4) + 432u;
    call(ctx, on != 0 ? 0x004CF8D8u : 0x004CF330u, {load<u32>(o + 356), load<u32>(ent), on});
}

// void f(const T* o, T* d): Thunk_000E44C0(o, d), then d's +236 = o+268,
// +208..+232 = o+240..+264, the field 0xFFFE0000 of +20 = 4 << 17,
// +240..+272 = o+272..+304 and the bytes +276 / +277 = o+308 / +309. r3
// = that last byte, f1 = o->+300.
DECOMP(001CBD04, Obj_CtorCopy_001CBD04) {
    const u32 o = arg<u32>(ctx, 0), s = arg<u32>(ctx, 1);
    call(ctx, 0x000E44C0, {o, s});
    CopyW32(s + 236, o + 268);
    for (u32 k = 0; k < 28; k += 4) CopyW32(s + 208 + k, o + 240 + k);
    store<u32>(s + 20, (load<u32>(s + 20) & ~0xFFFE0000u) | (4u << 17));
    for (u32 k = 0; k < 36; k += 4) CopyW32(s + 240 + k, o + 272 + k);
    store<u8>(s + 276, load<u8>(o + 308));
    store<u8>(s + 277, load<u8>(o + 309));
    ctx.gpr[3] = load<u8>(o + 309);
    ctx.fpr[1] = Lfs32(o + 300);
}

// void f(T* o, bool on): flag 0x80000000 of +384 = on; when it changes,
// the link (o->+612)->+156 (both set) gets +8 / +12 / u8 +16 / +17
// cleared, the helper o->+948 (when set) loses flag 0x40000000 of +1268,
// +932 / +936 = 0 and o->vt[136] / o->vt[384](o, origin 0x01164B40). r3 =
// the new word.
DECOMP(0023CB04, Obj_SetActiveFlag_0023CB04) {
    const u32 o = arg<u32>(ctx, 0);
    const u64 on = ctx.gpr[4];
    if ((on & 0xFFu) != (load<u32>(o + 384) >> 31)) {
        const u32 p = load<u32>(o + 612);
        if (p != 0) {
            const u32 q = load<u32>(p + 156);
            if (q != 0) {
                store<u32>(q + 8, 0);
                store<u32>(q + 12, 0);
                store<u8>(q + 16, 0);
                store<u8>(q + 17, 0);
            }
        }
        const u32 h = load<u32>(o + 948);
        if (h != 0) store<u32>(h + 1268, load<u32>(h + 1268) & ~0x40000000u);
        store<u32>(o + 932, 0);
        store<u32>(o + 936, 0);
        VCall(ctx, o, 136, {o, 0x01164B40u});
        VCall(ctx, o, 384, {o, 0x01164B40u});
    }
    const u32 w = (load<u32>(o + 384) & 0x7FFFFFFFu) | (static_cast<u32>(on & 1u) << 31);
    store<u32>(o + 384, w);
    ctx.gpr[3] = w;
}

// void f(T* o): every material slot e (8-byte entries +48 of each of the
// u8-counted (+112) parts +132 + 8 * i of the model (o->+160)->+208, u16
// count +52, the slot's object at +4) whose shader ((e->+16)->+8) is
// 0xA359CDE9 gets *Material_ParamSlot(e, 7) = 1. r3 as left.
DECOMP(00245F70, Model_FlagShaderParam_00245F70) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 m = load<u32>(o + 160);
    if (m == 0) return;
    const u32 md = load<u32>(m + 208);
    if (md == 0) return;
    for (u32 i = 0;; ++i) {
        const u32 n = load<u8>(md + 112);
        ctx.gpr[3] = n;
        if (i >= n) return;
        const u32 s = load<u32>(md + 132 + 8 * i);
        if (s == 0) continue;
        for (u32 j = 0; j < load<u16>(s + 52); ++j) {
            const u32 e = load<u32>(load<u32>(s + 48) + 8 * j + 4);
            if (e == 0 || load<u32>(load<u32>(e + 16) + 8) != 0xA359CDE9u) continue;
            const u32 slot = static_cast<u32>(call(ctx, 0x0050C018, {e, 7}));
            store<u32>(slot, 1);
        }
    }
}

// void f(T* o, Body* b): the entry of o's list +144 (count (+40 >> 7) &
// 15) whose body (+392) is b is torn down (Body_Remove931F00(e->+8, e),
// AnimSkel_Release7D4EA8(e), again while its s16 +6 stays positive),
// replaced by the last entry, the count decremented and the old last slot
// cleared. r3 as left (the count when not found).
DECOMP(0027B530, Obj_RemoveBody_0027B530) {
    const u32 o = arg<u32>(ctx, 0), b = arg<u32>(ctx, 1);
    const u32 n = (load<u32>(o + 40) >> 7) & 0xFu;
    ctx.gpr[3] = n;
    u32 i = 0;
    for (; i < n; ++i)
        if (load<u32>(load<u32>(o + 144 + 4 * i) + 392) == b) break;
    if (i >= n) return;
    const u32 slot = o + (i << 2) + 144;
    const u32 e = load<u32>(slot);
    call(ctx, 0x00931F00, {load<u32>(e + 8), e});
    call(ctx, 0x007D4EA8, {load<u32>(slot)});
    const u32 s = load<u32>(slot);
    if (static_cast<s16>(load<u16>(s + 6)) > 0) call(ctx, 0x007D4EA8, {s});
    const u32 w = load<u32>(o + 40);
    const u32 m = ((w >> 7) & 0xFu) - 1u;
    store<u32>(o + 40, (w & ~0x780u) | ((m << 7) & 0x780u));
    store<u32>(slot, load<u32>(o + ((m << 2) & 0x3Cu) + 144));
    const u32 last = o + ((load<u32>(o + 40) >> 5) & 0x3Cu) + 144;
    store<u32>(last, 0);
    ctx.gpr[3] = last;
}

// void f(T* o, U* u): each entry of o's u16 pointer vector +532 pushed
// onto u's vector +80 (PushU16Vec33), then o's count cleared. r3 = the
// count (0 for no vector).
DECOMP(002DA04C, Vec_MoveAll_002DA04C) {
    const u32 o = arg<u32>(ctx, 0), u = arg<u32>(ctx, 1);
    for (u32 i = 0;; ++i) {
        const u32 v = load<u32>(o + 532);
        const u32 n = v != 0 ? load<u16>(v + 8) : 0u;
        ctx.gpr[3] = n;
        if (i >= n) {
            if (v != 0) store<u16>(v + 8, 0);
            return;
        }
        PushU16Vec33(ctx, u + 80, load<u32>(load<u32>(v) + 4 * i));
    }
}

// void f(A* a, B* b): the record c = b->+36, or else the first of b's
// 860-byte records (+20, count +24) with u8 +826 clear whose u8-counted
// (+820) key list +748 holds the world's *(w + 0x128114), gets the low 3
// bits of the u16 at w + 496 + 4056 * u8 w->+4208 + 4 * u8 c->+822 set to
// a->+68 (w = *0x00F65CA0). Nothing for a or b of 0. r3 = a, or a->+68
// after a store.
DECOMP(002E7588, Obj_SetLodBits_002E7588) {
    const u32 a = arg<u32>(ctx, 0), b = arg<u32>(ctx, 1);
    if (a == 0 || b == 0) return;
    u32 c = load<u32>(b + 36);
    if (c == 0) {
        const u32 w = load<u32>(0x00F65CA0);
        const u32 n = load<u32>(b + 24);
        const u32 key = load<u32>(w + 0x128114u);
        u32 rec = load<u32>(b + 20);
        for (u32 j = 0; j < n && c == 0; ++j, rec += 860) {
            if (load<u8>(rec + 826) != 0) continue;
            const u32 m = load<u8>(rec + 820);
            for (u32 i = 0; i < m; ++i) {
                if (load<u32>(rec + 748 + 4 * i) == key) {
                    c = rec;
                    break;
                }
            }
        }
        if (c == 0) return;
    }
    const u32 w = load<u32>(0x00F65CA0);
    const u32 v = load<u32>(a + 68);
    const u32 p = w + static_cast<u32>(load<u8>(w + 4208)) * 4056u + (static_cast<u32>(load<u8>(c + 822)) << 2) + 496u;
    store<u16>(p, static_cast<u16>((load<u16>(p) & ~7u) | (v & 7u)));
    ctx.gpr[3] = v;
}

// ~T(): vtable 0x00B1D550; unless flag 0x1000 of u16 +236 is set, the
// owned objects +228 (when +232 is set), +232 (when +228 is set; read
// as 0 without +232, quirk kept) and +212 get vt[8](it); +216 gets
// vt[8](it); the block +248 SmallFree'd (u16 +238 * 32 bytes); vtable
// 0x00B1D4D8. r3 = that vtable.
DECOMP(002F2294, VtB1D550_Dtor_002F2294) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 h = load<u16>(o + 236);
    store<u32>(o, 0x00B1D550u);
    if ((h & 0x1000u) == 0) {
        const u32 a = load<u32>(o + 232);
        u32 y = 0;
        if (a != 0) {
            const u32 x = load<u32>(o + 228);
            VCall(ctx, x, 8, {x});
            y = load<u32>(o + 232);
        }
        if (load<u32>(o + 228) != 0) VCall(ctx, y, 8, {y});
        const u32 z = load<u32>(o + 212);
        VCall(ctx, z, 8, {z});
    }
    const u32 w = load<u32>(o + 216);
    VCall(ctx, w, 8, {w});
    call(ctx, 0x00552420, {load<u32>(o + 248), (static_cast<u32>(load<u16>(o + 238)) << 5) & 0xFFFFFFE0u});
    store<u32>(o, 0x00B1D4D8u);
    ctx.gpr[3] = 0x00B1D4D8u;
}

// void f(T* o): for the state k = o->vt[32](o, 0) with a handler
// o->+704[k] (k < 12), that handler (looked up again with a fresh
// vt[32], 0 when out of range) gets vt[48](it); then
// Effect_Spawn(0xEB4F48A0, 0, 0, 1). r3 as that left it.
DECOMP(0030932C, Obj_StateFx_0030932C) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 k = static_cast<u32>(VCall(ctx, o, 32, {o, 0}));
    if (k < 12 && load<u32>(o + 704 + 4 * k) != 0) {
        const u32 k2 = static_cast<u32>(VCall(ctx, o, 32, {o, 0}));
        const u32 h = k2 < 12 ? load<u32>(o + 704 + 4 * k2) : 0u;
        VCall(ctx, h, 48, {h});
    }
    call(ctx, 0x004A5FC8, {0xEB4F48A0u, 0, 0, 1});
}

// void f(T* o, a, b): in state 4 (u8 +18), when every entry of o's
// 32-byte list (+2572, count u16 +2580) with a +20 resolves
// (Obj_FindByHash_000DB290(+20, +24)) and at least one does,
// o->vt[104](o, 5, 0); then 0x003252D8(o, a, b). r3 as that left it.
DECOMP(0032F8CC, Obj_CheckAllFound_0032F8CC) {
    const u32 o = arg<u32>(ctx, 0), a = arg<u32>(ctx, 1), b = arg<u32>(ctx, 2);
    if (load<u8>(o + 18) == 4) {
        u32 found = 0, i = 0;
        for (; i < load<u16>(o + 2580); ++i) {
            const u32 e = load<u32>(o + 2572) + 32 * i;
            if (load<u32>(e + 20) != 0 && static_cast<u32>(call(ctx, 0x000DB290, {load<u32>(e + 20), load<u32>(e + 24)})) != 0) ++found;
        }
        if (found != 0 && found == load<u16>(o + 2580)) VCall(ctx, o, 104, {o, 5, 0});
    }
    call(ctx, 0x003252D8, {o, a, b});
}

// void f(T* o): Obj_ShowEntries_00325CE4(o); in state 4 (u8 +18),
// o->vt[28](o, 0) and each 32-byte entry of +2576 (count u16 +2584) gets
// +20 = the target definition of the object with id +16 (0 when none);
// with no mode bits (+2572 >> 26), Obj_SetStateFx266E6C(def, 0) for
// each found one. r3 as left.
DECOMP(00330394, Obj_ResolveTargets_00330394) {
    const u32 o = arg<u32>(ctx, 0);
    call(ctx, 0x00325CE4, {o, ctx.gpr[4]});  // r4 passed through as the caller left it
    const u32 st = load<u8>(o + 18);
    ctx.gpr[3] = st;
    if (st != 4) return;
    VCall(ctx, o, 28, {o, 0});
    for (u32 i = 0;; ++i) {
        const u32 n = load<u16>(o + 2584);
        ctx.gpr[3] = n;
        if (i >= n) return;
        const u32 e = load<u32>(o + 2576) + 32 * i;
        const u32 r = static_cast<u32>(call(ctx, 0x000D6A28, {load<u32>(e + 16)}));
        const u32 d = r != 0 ? static_cast<u32>(call(ctx, 0x000D951C, {r})) : 0u;
        store<u32>(e + 20, d);
        if (d != 0 && ((load<u32>(o + 2572) >> 26) & 0x3Fu) == 0) call(ctx, 0x00266E6C, {d, 0});
    }
}

// void Body_Moved(Body* h): activates h (Hk_Activate91A4C8) and walks its
// constraints c (0x0091A2CC count / 0x0091A7D8 get): the other body p of
// each (c->+20, or +24 when that is h) is activated and the walk ends
// when p also lists c; otherwise p is walked recursively. r3 as left.
DECOMP(003556AC, Body_Moved_003556AC) {
    const u32 h = arg<u32>(ctx, 0);
    if (h == 0) return;
    call(ctx, 0x0091A4C8, {h});
    for (s32 i = 0; i < static_cast<s32>(call(ctx, 0x0091A2CC, {h})); ++i) {
        const u32 c = static_cast<u32>(call(ctx, 0x0091A7D8, {h, static_cast<u64>(static_cast<s64>(i))}));
        u32 p = load<u32>(c + 20);
        if (p == h) p = load<u32>(c + 24);
        for (s32 j = 0; j < static_cast<s32>(call(ctx, 0x0091A2CC, {p})); ++j) {
            if (static_cast<u32>(call(ctx, 0x0091A7D8, {p, static_cast<u64>(static_cast<s64>(j))})) == c) {
                call(ctx, 0x0091A4C8, {p});
                return;
            }
        }
        call(ctx, 0x003556AC, {p});
    }
}

// T::T(T* o, Obj* a, b, u8 c, d, e, f, g, stack: u8 h (+116), i (+124),
// j (+132)): 0x0042C9D0(o, 1), vtable 0x00B21280, +12 = a (its u16
// reference count +178 incremented when set), +16 = b and the bytes
// +20..+27 = c..j. r3 = a + 178 (the vtable for no a).
DECOMP(004386C4, VtB21280_Ctor_004386C4) {
    const u32 o = arg<u32>(ctx, 0), a = arg<u32>(ctx, 1), b = arg<u32>(ctx, 2);
    const u32 c = arg<u32>(ctx, 3), d = arg<u32>(ctx, 4), e = arg<u32>(ctx, 5), f = arg<u32>(ctx, 6), g = arg<u32>(ctx, 7);
    const u32 h = StkArg34(ctx, 116), i = StkArg34(ctx, 124), j = StkArg34(ctx, 132);
    call(ctx, 0x0042C9D0, {o, 1});
    store<u32>(o + 12, a);
    store<u32>(o + 16, b);
    store<u8>(o + 20, static_cast<u8>(c));
    store<u8>(o + 21, static_cast<u8>(d));
    store<u32>(o, 0x00B21280u);
    store<u8>(o + 22, static_cast<u8>(e));
    store<u8>(o + 23, static_cast<u8>(f));
    store<u8>(o + 24, static_cast<u8>(g));
    store<u8>(o + 25, static_cast<u8>(h));
    store<u8>(o + 26, static_cast<u8>(i));
    store<u8>(o + 27, static_cast<u8>(j));
    ctx.gpr[3] = 0x00B21280u;
    if (a != 0) {
        store<u16>(a + 178, static_cast<u16>(load<u16>(a + 178) + 1));
        ctx.gpr[3] = a + 178u;
    }
}

// T::T(T* o, const Vec* src): 0x0042C9D0(o, 1), vtable 0x00B21400, the
// pointer vector +12 constructed (u16 +18 = 4) and filled with src's
// entries (u16-counted, data +0) via PushU16Vec33. r3 = src's count.
DECOMP(0043ABA0, VtB21400_CtorCopy_0043ABA0) {
    const u32 o = arg<u32>(ctx, 0), src = arg<u32>(ctx, 1);
    call(ctx, 0x0042C9D0, {o, 1});
    store<u32>(o, 0x00B21400u);
    call(ctx, 0x005516E0, {o + 12});
    store<u16>(o + 18, 4);
    for (u32 i = 0;; ++i) {
        const u32 n = load<u16>(src + 8);
        ctx.gpr[3] = n;
        if (i >= n) return;
        PushU16Vec33(ctx, o + 12, load<u32>(load<u32>(src) + 4 * i));
    }
}

// void f(T* o): when o's mode flag ((o->+48)->+12)[mode] (mode =
// (*0x010AB1B8)->+64) is set, the pending list (+20, count +24, then
// cleared) is flushed: each entry with flag 0x40000 of +172 decrements
// +32 and gets vt[40](e, 1); then o->vt[12](o). r3 as that left it.
DECOMP(0045477C, Obj_FlushPending_0045477C) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 mode = load<u32>(load<u32>(0x010AB1A8u + 16) + 64);
    if (load<u8>(load<u32>(load<u32>(o + 48) + 12) + (mode << 2)) != 0) {
        const s32 n = static_cast<s32>(load<u32>(o + 24));
        store<u32>(o + 24, 0);
        for (s32 i = 0; i < n; ++i) {
            const u32 e = load<u32>(load<u32>(o + 20) + static_cast<u32>(i) * 4u);
            if ((load<u32>(e + 172) & 0x40000u) == 0) continue;
            store<u32>(o + 32, load<u32>(o + 32) - 1u);
            VCall(ctx, e, 40, {e, 1});
        }
    }
    VCall(ctx, o, 12, {o});
}

// void f(Vec* v): the indices i (below *0x010C1A9C) of the 152-byte
// table 0x010BE8BC whose entry is set and whose world flag byte (bit 1
// of *0x00F65CA0 + 4028 + i) is set are pushed onto v (PushU16Vec33). r3
// = the count used last.
DECOMP(0049E330, Level_ListActive_0049E330) {
    const u32 v = arg<u32>(ctx, 0);
    u32 n = load<u32>(0x010C1A9C);
    const u32 w = load<u32>(0x00F65CA0);
    for (u32 i = 0; i < n; ++i) {
        if (load<u32>(0x010BE8BCu + 152 * i) == 0 || (load<u8>(w + 80 + 3948 + i) & 1u) == 0) continue;
        PushU16Vec33(ctx, v, i);
        n = load<u32>(0x010C1A9C);
    }
    ctx.gpr[3] = n;
}

// T* New(-, Xml* x): a 20-byte object (Mem_Alloc(20, 0, 16); 0 on
// failure): vtable 0x00B221F0, u16 +4 = 1, +12 = the hash of the
// attribute 0x00AEF030 (String_HashNoCase) or else the s32 0x5C8998CD
// (default 0), +16 = the f32 0x000110AD (default *0x004C43A8). r3 = the
// object.
DECOMP(004C43B0, VtB221F0_NewXml_004C43B0) {
    const u32 x = arg<u32>(ctx, 1);
    const u32 m = static_cast<u32>(call(ctx, 0x0054ED38, {20, 0, 16}));
    if (m != 0) {
        const f64 k = Lfs32(0x004C43A8);
        store<u16>(m + 4, 1);
        store<u32>(m, 0x00B221F0u);
        store<u32>(m + 12, 0);
        store<u32>(m + 16, 0);
        const u32 a = static_cast<u32>(call(ctx, 0x004F9628, {x, 0x00AEF030u}));
        if (a != 0) store<u32>(m + 12, static_cast<u32>(call(ctx, 0x0056A710, {a})));
        else call(ctx, 0x004F99BC, {x, 0x5C8998CDu, m + 12, 0});
        callf(ctx, 0x004F9B20, {x, 0x000110ADu, m + 16}, {k});
    }
    ctx.gpr[3] = m;
}

// Deleting ~T() (o may be 0): vtable 0x00B11E88, the block +680
// SmallFree'd (+696 * 16 bytes), the pointer vectors +620 / +600 / +580 /
// +556 / +536 cleared, vtable 0x00B126B8, VtB12550_Dtor_000DF934(o) and
// SmallFree(o, 736). r3 as that left it.
DECOMP(007441E0, VtB11E88_Delete_007441E0) {
    const u32 o = arg<u32>(ctx, 0);
    if (o == 0) return;
    const u32 n = load<u32>(o + 696);
    store<u32>(o, 0x00B11E88u);
    call(ctx, 0x00552420, {load<u32>(o + 680), (n << 4) & 0xFFFFFFF0u});
    for (const u32 k : {620u, 600u, 580u, 556u, 536u}) PvClear32(ctx, o + k);
    store<u32>(o, 0x00B126B8u);
    call(ctx, 0x000DF934, {o});
    call(ctx, 0x00552420, {o, 736});
}

// T::T(T* o, Obj* u, a, u16 id, bool f9, const Vec3* p, const Vec3* q,
// const Vec3* r; f1 x, f2 y; stack: bool f31 (+132), bool f30 (+140), w
// (+148)): vtable 0x00B0DACC, +4 = u, +8 = a, u16 +12 = id, the u16
// flags +14 (field 0xE000 = 1, 0x0800 / 0x0400 / 0x0100 cleared, 0x0200
// = f9, 0x1000 = u's flag 1 of +188), +16 / +32 / +48 = p / q / r, +64 =
// x, +68 = y, +72 = (+72 & 0x3FFFFFFF) | f31 << 31 | f30 << 30, +76 = w.
// r3 = o, r4 = the new +72.
DECOMP(0074DAC4, VtB0DACC_Ctor_0074DAC4) {
    const u32 o = arg<u32>(ctx, 0), u = arg<u32>(ctx, 1), a = arg<u32>(ctx, 2), id = arg<u32>(ctx, 3);
    const u32 f9 = arg<u32>(ctx, 4), p = arg<u32>(ctx, 5), q = arg<u32>(ctx, 6), r = arg<u32>(ctx, 7);
    const f64 x = ctx.fpr[1], y = ctx.fpr[2];
    const u32 b31 = StkArg34(ctx, 132), b30 = StkArg34(ctx, 140), w = StkArg34(ctx, 148);
    u32 h = load<u16>(o + 14);
    store<u32>(o + 4, u);
    h = (h & ~0xE000u) | (1u << 13);
    store<u32>(o + 8, a);
    h &= ~0x800u;
    h &= ~0x400u;
    h = (h & ~0x200u) | ((f9 & 1u) << 9);
    store<u16>(o + 12, static_cast<u16>(id));
    const u32 w72 = load<u32>(o + 72);
    h &= ~0x100u;
    u32 f = 0;
    if (u != 0) {
        store<u16>(o + 14, static_cast<u16>(h));
        if ((load<u32>(u + 188) & 1u) != 0) f = 1;
    }
    h = (h & ~0x1000u) | (f << 12);
    store<u16>(o + 14, static_cast<u16>(h));
    for (u32 k = 0; k < 12; k += 4) CopyW32(o + 16 + k, p + k);
    CopyW32(o + 32, q);
    store<u32>(o, 0x00B0DACCu);
    CopyW32(o + 36, q + 4);
    CopyW32(o + 40, q + 8);
    CopyW32(o + 48, r);
    CopyW32(o + 52, r + 4);
    Stfs32(o + 64, x);
    CopyW32(o + 56, r + 8);
    Stfs32(o + 68, y);
    const u32 n72 = (w72 & 0x7FFFFFFFu & ~0x40000000u) | ((b31 & 1u) << 31) | ((b30 & 1u) << 30);
    store<u32>(o + 72, n72);
    store<u32>(o + 76, w);
    ctx.gpr[3] = o;
    ctx.gpr[4] = n72;
}

// ~T() thunk for the +384 base: DtorB160A8_34(this - 384).
DECOMP(007536D4, VtB160A8_DtorThunk384_007536D4) { DtorB160A8_34(ctx, arg<u32>(ctx, 0) - 384u); }

// ~T() thunk for the +380 base: DtorB160A8_34(this - 380).
DECOMP(007537E0, VtB160A8_DtorThunk380_007537E0) { DtorB160A8_34(ctx, arg<u32>(ctx, 0) - 380u); }

// u16 f(Vec* v, const E* e): the 32-byte element e appended to the
// u16-counted vector v (capacity +4 doubled when full, count +8,
// high-water +10): the floats +0..+8, words +16 / +20 and the flags
// 0xFC000000 of +24 (stored after each bit). r3 = the new element's
// index, f1 = e->+8.
DECOMP(0076AF44, Vec32_Push_0076AF44) {
    const u32 v = arg<u32>(ctx, 0), e = arg<u32>(ctx, 1);
    const u32 cap = load<u16>(v + 4);
    if (load<u16>(v + 8) == cap) call(ctx, 0x00551718, {v, (cap << 1) & 0x1FFFEu});
    const u32 d = load<u32>(v) + ((static_cast<u32>(load<u16>(v + 8)) << 5) & 0x1FFFE0u);
    for (u32 k = 0; k < 12; k += 4) CopyW32(d + k, e + k);
    const u32 old = load<u32>(d + 24);
    CopyW32(d + 16, e + 16);
    CopyW32(d + 20, e + 20);
    u32 w = (old & 0x7FFFFFFFu) | (load<u32>(e + 24) & 0x80000000u);
    store<u32>(d + 24, w);
    for (const u32 m : {0x40000000u, 0x20000000u, 0x10000000u, 0x08000000u, 0x04000000u}) {
        w = (w & ~m) | (load<u32>(e + 24) & m);
        store<u32>(d + 24, w);
    }
    const u32 n = load<u16>(v + 8) + 1u;
    const u32 hw = load<u16>(v + 10);
    store<u16>(v + 8, static_cast<u16>(n));
    store<u16>(v + 10, static_cast<u16>(static_cast<s32>(hw) > static_cast<s32>(n & 0xFFFFu) ? hw : n));
    ctx.gpr[3] = static_cast<u64>(static_cast<s64>(n & 0xFFFFu) - 1);
    ctx.fpr[1] = Lfs32(e + 8);
}

// u16 f(Set* s, u32 i): slot i of the set (header h = s->+4 with u16
// count +2 and a move callback OPD +8; 8-byte keys +12, objects +8)
// overwritten by the last one; the object now at i (when the object array
// and the callback exist and it is set) is handed to the callback (obj,
// i). The count is decremented (cleared when it was 1 or less). r3 = the
// new count (0 then).
DECOMP(00770060, Set_RemoveAt_00770060) {
    const u32 s = arg<u32>(ctx, 0);
    const u64 i = ctx.gpr[4];
    const u32 h = load<u32>(s + 4);
    const u32 n = load<u16>(h + 2);
    if (static_cast<s32>(n) <= 1) {
        store<u16>(h + 2, 0);
        ctx.gpr[3] = 0;
        return;
    }
    const u32 last = n - 1u;
    const u32 keys = load<u32>(s + 12);
    CopyW32(keys + static_cast<u32>(i << 3), keys + (last << 3));
    CopyW32(keys + static_cast<u32>(i << 3) + 4, keys + (last << 3) + 4);
    u32 hh = h, p = load<u32>(s + 8);
    if (p != 0) {
        CopyW32(p + (static_cast<u32>(i) << 2), p + (last << 2));
        hh = load<u32>(s + 4);
        p = load<u32>(s + 8);
    }
    const u32 cb = load<u32>(hh + 8);
    if (cb != 0 && p != 0) {
        const u64 i4 = Sx33(static_cast<u32>(i << 2));
        const u32 e = load<u32>(p + static_cast<u32>(i4));
        if (e != 0) {
            call_ptr(ctx, cb, {e, i, load<u32>(cb), hh, cb, i4});
            hh = load<u32>(s + 4);
        }
    }
    const u32 c = static_cast<u32>(load<u16>(hh + 2)) - 1u;
    store<u16>(hh + 2, static_cast<u16>(c));
    ctx.gpr[3] = c;
}

// void f(World* o; f1 dt): TimedLerp34 on o + 0x1282A4 (k at
// 0x00787060).
DECOMP(00787070, World_BlendA_00787070) { TimedLerp34(ctx, 0x1282A4u, 0x00787060u); }

// void f(World* o; f1 dt): TimedLerp34 on o + 0x1282C0 (k at
// 0x00787180).
DECOMP(00787190, World_BlendB_00787190) { TimedLerp34(ctx, 0x1282C0u, 0x00787180u); }
