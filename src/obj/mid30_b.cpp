// Game code, medium sweep part 30: effect spawning from the effect-set
// registry, AI controller binding, mode records, timers and resets.
#include "mid30.h"

using namespace sly4;
using namespace sly4::mid30;

namespace {
using namespace m30fn;

constexpr u32 SetFind30 = 0x004BCE90u;     // Set_Find(name)
constexpr u32 SmallAlloc30 = 0x0055238Cu;  // SmallAlloc(n)
constexpr u32 EffNew30 = 0x004C7A68u;      // Effect_Construct(e, def)
constexpr u32 EffStart30 = 0x004C753Cu;    // Effect_Start(e)
constexpr u32 SetHash30 = 0x004CF988u;     // Effect_SetHash4CF988(e, hash)
constexpr u32 kSetIter30 = 0x011DE288u;    // the registry's last-found value

// The inlined find of key in the effect set `name` (Set_Find): the AVL
// root *(set->+16) is walked by the u32 keys +4 (larger -> +12, else +16)
// with the path pushed on the set's stack (u16 +100, nodes +20..); the
// found node -> set->+104 and its value +8 -> *0x011DE288 (returned), a
// miss leaves the last node in +104 (0), an empty set clears both (0).
// r3 = the set (the found node on a hit) as the original leaves it.
u32 SetLookup30(PPUContext& ctx, u32 name, u32 key) {
    const u32 s = static_cast<u32>(call(ctx, SetFind30, {name}));
    if (s == 0) return 0;
    u32 n = load<u32>(load<u32>(s + 16));
    if (n == 0) {
        store<u32>(s + 104, 0);
        store<u32>(kSetIter30, 0);
        return 0;
    }
    store<u16>(s + 100, 0);
    for (;;) {
        const u32 k = load<u32>(n + 4);
        if (k == key) {
            store<u32>(s + 104, n);
            store<u32>(kSetIter30, load<u32>(n + 8));
            const u32 f = load<u32>(s + 104);
            ctx.gpr[3] = f;  // the node, as the original leaves it
            return f != 0 ? load<u32>(f + 8) : 0;
        }
        const u32 nx = k > key ? load<u32>(n + 12) : load<u32>(n + 16);
        if (nx == 0) {
            store<u32>(s + 104, n);
            return 0;
        }
        const s16 c = static_cast<s16>(load<u16>(s + 100));
        store<u16>(s + 100, static_cast<u16>(c + 1));
        store<u32>(s + 20 + 4 * static_cast<u32>(static_cast<s32>(c)), n);
        n = nx;
    }
}

// The effect definition of hash h: set 0, else set 0x7719A548.
u32 EffDef30(PPUContext& ctx, u32 h) {
    const u32 d = SetLookup30(ctx, 0, h);
    return d != 0 ? d : SetLookup30(ctx, 0x7719A548u, h);
}

// A new 140-byte effect of definition d (SmallAlloc + Effect_Construct);
// 0 when the allocation fails.
u32 EffMake30(PPUContext& ctx, u32 d) {
    const u32 p = static_cast<u32>(call(ctx, SmallAlloc30, {140}));
    if (p == 0) return 0;
    call(ctx, EffNew30, {p, d});
    return p;
}

// The kind-5 component 0x0027469E of e gets the ref-counted object s
// (u16 +4 += 1) in place of its +28 (released with vt[8]).
void EffAttach30(PPUContext& ctx, u32 e, u32 s) {
    const u32 c = static_cast<u32>(call(ctx, FindComp29, {e, 0x0027469Eu}));
    if (c == 0 || static_cast<u32>(VCall(ctx, c, 32, {c})) != 5) return;
    if (const u32 q = load<u32>(c + 28); q != 0) VCall(ctx, q, 8, {q});
    store<u32>(c + 28, s);
    if (s != 0) store<u16>(s + 4, static_cast<u16>(load<u16>(s + 4) + 1));
}

// The kind-4 component 0x001E8ECC of e gets the 4x3 rows of the matrix m
// (rows 16 bytes apart into +32 / +48 / +64 / +80); f1 = the last float.
void EffPlace30(PPUContext& ctx, u32 e, u32 m) {
    const u32 c = static_cast<u32>(call(ctx, FindComp29, {e, 0x001E8ECCu}));
    if (c == 0 || static_cast<u32>(VCall(ctx, c, 32, {c})) != 4) return;
    for (u32 r = 0; r < 4; ++r)
        for (u32 k = 0; k < 3; ++k) store<u32>(c + 32 + 16 * r + 4 * k, load<u32>(m + 16 * r + 4 * k));
    ctx.fpr[1] = Lf(m + 56);
}

// The player when its +192 has bit 2, else 0.
u32 Player30() {
    const u32 p = load<u32>(load<u32>(kWorld29) + 4216);
    return p != 0 && (load<u32>(p + 192) & 2) != 0 ? p : 0;
}

// A virtual call whose OPD sits in r4 (and its entry in r7): o->vt[slot]
// (o, opd, a, b), as the original's register reuse leaves it.
u64 VCallR4_30(PPUContext& ctx, u32 o, u32 slot, u64 a, u64 b) {
    const u32 opd = load<u32>(load<u32>(o) + slot);
    return call_ptr(ctx, opd, {o, opd, a, b, load<u32>(opd)});
}

constexpr u32 AnimRestart30 = 0x002775A8u;  // Anim_Restart2775A8
}  // namespace

// u32 f(T* o, u32 h): plays the effect h on o's body. The definition (set
// 0, else 0x7719A548; none: r3 = the set as left) builds a new effect
// whose sound component gets (o->+48)->+160 and whose placement component
// the body matrix (o->+48)+96; its +136 gets bit 0x200 and it is started
// (Effect_Start, r3). A failed allocation still writes +136 of address 0
// (as the original).
DECOMP(00014004, Obj_ApplyAnimHash_00014004) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 h = arg<u32>(ctx, 1);
    const u32 d = EffDef30(ctx, h);
    if (d == 0) return;
    const u32 e = EffMake30(ctx, d);
    EffAttach30(ctx, e, load<u32>(load<u32>(o + 48) + 160));
    EffPlace30(ctx, e, load<u32>(o + 48) + 96);
    store<u32>(e + 136, load<u32>(e + 136) | 0x200u);
    call(ctx, EffStart30, {e});
}

// u32 f(T* o, u32 h): as 0x00014004, but the effect replaces o->+72 (the
// old one deleted with Effect_Delete first, not cleared) and its bit 0x200
// of +136 is cleared before Effect_Start(o->+72).
DECOMP(000143C8, Obj_ReplaceEffect_000143C8) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 h = arg<u32>(ctx, 1);
    const u32 d = EffDef30(ctx, h);
    if (d == 0) return;
    if (const u32 old = load<u32>(o + 72); old != 0) call(ctx, 0x004D0280u, {old});  // Effect_Delete
    const u32 e = EffMake30(ctx, d);
    const u32 s = load<u32>(load<u32>(o + 48) + 160);
    store<u32>(o + 72, e);
    EffAttach30(ctx, e, s);
    EffPlace30(ctx, load<u32>(o + 72), load<u32>(o + 48) + 96);
    const u32 x = load<u32>(o + 72);
    store<u32>(x + 136, load<u32>(x + 136) & ~0x200u);
    call(ctx, EffStart30, {load<u32>(o + 72)});
}

// u32 f(T* o, u32 a): AiCtrl_Bind_00011D08(o, a), then resets o's state:
// +372 = 0, (o->+48)->vt[96](1), the body's +164 flags (u16 +40: bit
// 0x1000 set, 0x400 cleared), +208 = 3.0f, +4 / +28 / +352 = 0, bit 0x80
// of u8 +305 set, u8 +307 = 0, *0x00F55830 = a, +320.. = the body's
// position +144... The scale +336 = *0x000279F8, divided by the y extent
// (+20 of WorldObj_WorldBounds) of the world object 0xF857B41A when that
// exists; +340 = 1.0f, +344 = 0.2f, +348 = 40.0f. A body with bit 4 of
// +192 joins group 9 (Obj_SetGroupBits_000E9728), gets bit 0x80000000 of
// +396, +416 = 0x00F2DAA0 and +348 = max(+344, *0x000279FC); then the
// global 0x00F64CB8 (when set) gets 0x00364C90(g; f1 = the scale
// constant). +356 = 0, u8 +376 = 0; returns vt[224](o).
DECOMP(00027A00, AiCtrl_Reset_00027A00) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 a = arg<u32>(ctx, 1);
    call(ctx, 0x00011D08u, {o, a});  // AiCtrl_Bind_00011D08
    store<u32>(o + 372, 0);
    {
        const u32 p = load<u32>(o + 48);
        VCall(ctx, p, 96, {p, 1});
    }
    if (const u32 q = load<u32>(load<u32>(o + 48) + 164); q != 0)
        store<u16>(q + 40, static_cast<u16>((load<u16>(q + 40) | 0x1000u) & ~0x0400u));
    const u8 b305 = load<u8>(o + 305);
    const u32 body = load<u32>(o + 48);
    store<u32>(o + 208, 0x40400000u);
    store<u32>(o + 4, 0);
    store<u32>(o + 28, 0);
    store<u8>(o + 305, b305 | 0x80);
    store<u8>(o + 307, 0);
    store<u32>(o + 352, 0);
    store<u32>(0x00F55830u, a);
    for (u32 k = 0; k < 12; k += 4) store<u32>(o + 320 + k, load<u32>(body + 144 + k));
    const u32 wo = static_cast<u32>(call(ctx, 0x005801DCu, {0xF857B41Au, 0}));  // WorldObj_FindByHash
    const u32 c31 = load<u32>(0x000279F8u);
    const f64 f31 = LfsF(c31);
    ctx.fpr[1] = f31;
    u32 scale = c31;
    if (wo != 0) {
        Stack frame(ctx, 32);  // the bounds at 112(r1)..139(r1)
        const u32 sp = frame.addr();
        for (u32 k = 0; k < 12; k += 4) store<u32>(sp + k, 0x7F7FFFFFu);
        for (u32 k = 16; k < 28; k += 4) store<u32>(sp + k, 0xFF7FFFFFu);
        call(ctx, 0x005772E4u, {wo, sp});  // WorldObj_WorldBounds
        const f64 s = Sgl28(f31 / Lf(sp + 20));
        ctx.fpr[1] = s;
        scale = Bits28(s);
    }
    store<u32>(o + 336, scale);
    store<u32>(o + 348, 0x42200000u);
    store<u32>(o + 340, 0x3F800000u);
    store<u32>(o + 344, 0x3E4CCCCDu);
    if (const u32 b = load<u32>(o + 48); b != 0 && (load<u32>(b + 192) & 4) != 0) {
        call(ctx, 0x000E9728u, {b, 9});  // Obj_SetGroupBits_000E9728
        const u32 f396 = load<u32>(b + 396) | 0x80000000u;
        store<u32>(b + 416, 0x00F2DAA0u);
        store<u32>(b + 396, f396);
        const u32 v344 = load<u32>(b + 344), lim = load<u32>(0x000279FCu);
        const u32 v = LfsF(v344) > LfsF(lim) ? v344 : lim;
        ctx.fpr[1] = LfsF(v);
        store<u32>(b + 348, v);
        if (const u32 g = load<u32>(0x00F64CB8u); g != 0) callf(ctx, 0x00364C90u, {g}, {f31});
    }
    store<u32>(o + 356, 0);
    store<u8>(o + 376, 0);
    VCall(ctx, o, 224, {o});
}

// u32 f(T* o): applies the mode +716 (records of 116 bytes at +612 + 92).
// A body +60 gets +348 = max(min(G, body->+344), *0x0003E278) with G =
// *0x00F954E8, scaled by the previous record's +112 for a non-zero mode,
// then Global_F64CB8_Copy216(). +764 = G * record->+108; a record with
// u16 +28 looks its +104 up in the sound map ((*0x00F8F158)->+16,
// SortedMap 0x007335C4) into +724 (a miss names it with Hash_ToString).
// The effect +624 gets 0xB454D2CA, then 0x1C75088D / 0x1E29E12C /
// 0x1FDEB9CB for modes 0 / 1 / 2. r3 / f1 as the original leaves them.
DECOMP(0003E280, Obj_ApplyMode_0003E280) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 cG = 0x00F954E8u, cMin = 0x0003E278u;
    if (const u32 b = load<u32>(o + 60); b != 0) {
        const u32 k = load<u32>(o + 716);
        const u32 r2 = load<u32>(b + 344);
        u32 r3;
        if (k == 0) {
            r3 = load<u32>(cG);
        } else {
            const u32 prev = load<u32>(o + 612) + 116 * k - 24;
            r3 = Bits28(Sgl28(Lf(cG) * Lf(prev + 112)));
        }
        const u32 mn = LfsF(r3) < LfsF(r2) ? r3 : r2;
        const u32 v = LfsF(mn) > Lf(cMin) ? mn : load<u32>(cMin);
        ctx.fpr[1] = LfsF(v);
        store<u32>(b + 348, v);
        call(ctx, 0x00369D58u, {});  // Global_F64CB8_Copy216
    }
    const u32 rec = load<u32>(o + 612) + 116 * load<u32>(o + 716) + 92;
    const f64 s = Sgl28(Lf(cG) * Lf(rec + 108));
    store<u32>(o + 764, Bits28(s));
    ctx.fpr[1] = s;
    if (load<u16>(rec + 28) != 0) {
        Stack frame(ctx, 16);  // the key at 112(r1)
        const u32 key = frame.addr();
        store<u32>(key, load<u32>(rec + 104));
        const u32 p = static_cast<u32>(call(ctx, 0x007335C4u, {load<u32>(load<u32>(0x00F8F158u) + 16), key}));
        const u32 v = load<u32>(p);
        store<u32>(o + 724, v);
        if (v == 0) call(ctx, HashStr29, {load<u32>(rec + 104)});
    }
    const u32 e = load<u32>(o + 624);
    if (e == 0) {
        ctx.gpr[3] = 0;
        return;
    }
    call(ctx, SetHash30, {e, 0xB454D2CAu});
    const s32 k = static_cast<s32>(load<u32>(o + 716));
    u32 h;
    if (k > 0) {
        if (k > 2) {
            ctx.gpr[3] = static_cast<u32>(k);
            return;
        }
        h = k == 1 ? 0x1E29E12Cu : 0x1FDEB9CBu;
    } else {
        if (k <= -1) {
            ctx.gpr[3] = static_cast<u32>(k);
            return;
        }
        h = 0x1C75088Du;
    }
    call(ctx, SetHash30, {load<u32>(o + 624), h});
}

// u32 f(T* o, f32 dt, u8 force): counts the timer +756 down by dt. Unforced,
// it runs out only while the input mask (*0x010B1A98 & *0x010B1A9C) has
// bit 0x10 or 0x20 and the timer is <= *0x0004608C. Then the animation
// 0x24A8169A (bit 0x40000000 of +776 set) / 0x70B11DC9 restarts on o->+64
// (+32 = it when it plays), the player's +244 gets vt[164](0x9EA78345,
// 0x202272F9 / 0xE4824D4A, 0; f1 = *0x0004608C), the bit is toggled and
// +756 = *0x00F96FD8. Unforced with bit 0x20000000 clear, that bit is set
// and a spawn request (Obj_Reset_004AE360; position *0x00F97030..
// +0x00F97038, +16 = o->+60, +20 = (s8) player->+172, u8 +45 / +46 = 1)
// is burst (SpawnQueue_Burst4AED50). +768 = the player, +772 =
// *0x00F95794 (unforced) / *0x00046098, bit 0x80000000 of +776 set and the
// effect +624 gets 0x8178FFDC. r3 / f1 as the original leaves them.
DECOMP(000460A0, Obj_TimerToggle_000460A0) {
    const u32 o = arg<u32>(ctx, 0);
    const f64 dt = ctx.fpr[1];
    const u32 force = arg<u32>(ctx, 2) & 0xFF;
    const u32 C = 0x0004608Cu;
    const f64 t0 = Lf(o + 756);
    const u32 pl = Player30();
    const u32 m = load<u32>(0x010B1A98u) & load<u32>(0x010B1A9Cu);
    const bool busy = (m & 0x10u) != 0 || (m & 0x20u) != 0;
    const f64 t = Sgl28(t0 - dt);
    store<u32>(o + 756, Bits28(t));
    const u32 an = load<u32>(o + 64);
    if (force == 0) {
        if (!busy) {
            ctx.gpr[3] = an;
            return;
        }
        ctx.fpr[1] = Lf(C);
        if (!(t <= Lf(C))) {
            ctx.gpr[3] = an;
            return;
        }
    }
    const u32 h = (load<u32>(o + 776) & 0x40000000u) != 0 ? 0x24A8169Au : 0x70B11DC9u;
    const f64 f31 = Lf(C);
    // arguments 9 / 10 (sp+112 / sp+120) are whatever the frame holds
    callf(ctx, AnimRestart30, {an, h, 0, 0, 0, ~0ull, 0, 0, 0, 0, 0, ~0ull}, {Lf(C + 4), Lf(C + 8), f31, f31});
    const u32 r = static_cast<u32>(ctx.gpr[3]);
    u32 st = load<u32>(o + 776);
    const u32 x = load<u32>(pl + 244);  // read even without a player (as the original)
    if (r != 0) store<u32>(o + 32, h);
    if (x != 0) {
        const u32 h4 = (st & 0x40000000u) != 0 ? 0x202272F9u : 0xE4824D4Au;
        ctx.fpr[1] = f31;
        VCall(ctx, x, 164, {x, 0x9EA78345u, h4, 0});
        st = load<u32>(o + 776);
    }
    st ^= 0x40000000u;
    store<u32>(o + 776, st);
    ctx.fpr[1] = Lf(0x00F96FD8u);
    store<u32>(o + 756, load<u32>(0x00F96FD8u));
    if ((st & 0x20000000u) == 0 && force == 0) {
        st |= 0x20000000u;
        store<u32>(o + 776, st);
        Stack frame(ctx, 48);  // the request at 144(r1)..191(r1)
        const u32 q = frame.addr();
        call(ctx, 0x004AE360u, {q});  // Obj_Reset_004AE360
        store<u32>(q + 0, load<u32>(0x00F97030u));
        store<u32>(q + 4, load<u32>(0x00F97034u));
        store<u32>(q + 8, load<u32>(0x00F97038u));
        store<u32>(q + 16, load<u32>(o + 60));
        store<u8>(q + 45, 1);
        store<u8>(q + 46, 1);
        const s32 sb = static_cast<s8>(load<u8>(pl + 172));
        store<u32>(q + 20, static_cast<u32>(sb));
        ctx.fpr[1] = Lf(0x00F97038u);
        call(ctx, 0x004AED50u, {q, static_cast<u64>(static_cast<s64>(sb)), 1});  // SpawnQueue_Burst4AED50
        st = load<u32>(o + 776);
    }
    store<u32>(o + 768, pl);
    const u32 lim = force != 0 ? C + 12 : 0x00F95794u;
    ctx.fpr[1] = Lf(lim);
    store<u32>(o + 772, load<u32>(lim));
    st |= 0x80000000u;
    store<u32>(o + 776, st);
    ctx.gpr[3] = st;
    if (const u32 e = load<u32>(o + 624); e != 0) call(ctx, SetHash30, {e, 0x8178FFDCu});
}

// u32 f(T* o, ?, Msg* m, f32 f1): a hit message. Outside state 16, while
// the attack +1800 is 0x93D8EE68..0x93D8EE6A and m comes from o's body +60
// as kind 4: +1764 += 1 and bit 0x80 of u8 +305 set (r3 = the count). In
// state 18 with f1 > *0x0004E71C: (o->+60)->vt[352](opd, m->+4, 0),
// Global_F64CB8_Copy216(), and the reaction 0x27C431B3 / 0x27C431AD /
// 0x27C431A4 (by the player owner's +136 ->+36: 0xC78FB623 / 0x2A2FA0CE /
// other or none) restarts on o->+64 (+32 = it when it plays); bit 0x80 of
// u8 +305 set (r3 = the byte). r3 / f1 as the original leaves them.
DECOMP(0004E728, Obj_HitReact_0004E728) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 m = arg<u32>(ctx, 2);
    const f64 f1 = ctx.fpr[1];
    const u32 k = load<u32>(o + 4);
    u32 r3 = k;
    if (k != 16) {
        const u32 a = load<u32>(o + 1800);
        r3 = a;
        if (a == 0x93D8EE68u || a == 0x93D8EE69u || a == 0x93D8EE6Au) {
            r3 = load<u32>(m + 4);
            if (r3 == load<u32>(o + 60)) {
                r3 = (load<u16>(m + 14) >> 13) & 7;
                if (r3 == 4) {
                    const u32 n = load<u32>(o + 1764) + 1;
                    const u8 b = load<u8>(o + 305) | 0x80;
                    store<u32>(o + 1764, n);
                    store<u8>(o + 305, b);
                    ctx.gpr[3] = n;
                    return;
                }
            }
        }
    }
    const u32 C = 0x0004E71Cu;
    if (k != 18) {
        ctx.gpr[3] = r3;
        return;
    }
    if (!(f1 > Lf(C))) {
        ctx.gpr[3] = C;  // the constant's address, as the original leaves it
        return;
    }
    const f64 f31 = Lf(C);
    const u32 b = load<u32>(o + 60);
    VCallR4_30(ctx, b, 352, load<u32>(m + 4), 0);
    call(ctx, 0x00369D58u, {});  // Global_F64CB8_Copy216
    const u32 ow = static_cast<u32>(call(ctx, 0x000D93FCu, {Player30()}));  // Obj_Owner0D93FC
    u32 h = 0x27C431A4u;
    if (const u32 q = load<u32>(ow + 136); q != 0) {
        const u32 t = load<u32>(q + 36);
        if (t == 0xC78FB623u) h = 0x27C431B3u;
        else if (t == 0x2A2FA0CEu) h = 0x27C431ADu;
    }
    // r8 is left by Obj_Owner0D93FC; arguments 9 / 10 (sp+112 / sp+120) are whatever the frame holds
    const u64 r8 = ctx.gpr[8];
    callf(ctx, AnimRestart30, {load<u32>(o + 64), h, 0, 0, ~0ull, r8, 0, 0, 0, 0, 0, ~0ull}, {Lf(C + 4), Lf(C + 8), f31, f31});
    if (static_cast<u32>(ctx.gpr[3]) != 0) store<u32>(o + 32, h);
    const u8 v = load<u8>(o + 305) | 0x80;
    store<u8>(o + 305, v);
    ctx.gpr[3] = v;
}

// u32 f(T* o): finishes a pending scene switch (u8 +512). A pending +511
// re-enters slot +468 (0x0036325C(o->+492[slot], 1, 1, 0)). With u8 +515
// the camera object +416 is detached from the player (Obj_Detach0D72C0),
// gets vt[212](0) and loses bit 0x8000 of +172. A player with bits 2 / 8
// of +192 loses bit 0x01 of u8 +968, gets vt[176](0), the effect hash
// 0xBC1BA9E7 and Obj_ResetPos_0026D9D4(vt[64]()). u8 +509 = 0; the group
// 0x1840AF90 (when *0x00F9C660) or the +308th id of the table 0x00AC034C
// (copied to the stack) is released (Group_ReleaseById_0042C00C) and u8
// world->+0x1285A5 = 0 (r3 = the world).
DECOMP(0005E6A4, Scene_FinishSwitch_0005E6A4) {
    const u32 o = arg<u32>(ctx, 0);
    if (load<u8>(o + 512) == 0) {
        ctx.gpr[3] = 0;
        return;
    }
    store<u8>(o + 512, 0);
    if (load<u8>(o + 511) != 0) {
        store<u8>(o + 511, 0);
        call(ctx, 0x0036325Cu, {load<u32>(o + 492 + 4 * load<u32>(o + 468)), 1, 1, 0});
    }
    const u32 pl = Player30();
    const u32 who = pl != 0 && (load<u32>(pl + 192) & 8) != 0 ? pl : 0;
    if (load<u8>(o + 515) != 0) {
        store<u8>(o + 515, 0);
        call(ctx, 0x000D72C0u, {Player30(), load<u32>(o + 416)});  // Obj_Detach0D72C0
        const u32 c = load<u32>(o + 416);
        VCall(ctx, c, 212, {c, 0});
        const u32 c2 = load<u32>(o + 416);
        store<u32>(c2 + 172, load<u32>(c2 + 172) & ~0x8000u);
    }
    if (who != 0) {
        store<u8>(who + 968, load<u8>(who + 968) & 0xFE);
        VCall(ctx, who, 176, {who, 0});
        call(ctx, 0x000DAEC8u, {who, 0xBC1BA9E7u});  // Obj_SetEffectHash_000DAEC8
        const u32 r = static_cast<u32>(VCall(ctx, who, 64, {who}));
        call(ctx, 0x0026D9D4u, {r});  // Obj_ResetPos_0026D9D4
    }
    Stack frame(ctx, 64);  // the id table at 112(r1)..175(r1)
    const u32 sp = frame.addr();
    store<u8>(o + 509, 0);
    call(ctx, 0x0071C3F0u, {sp, 0x00AC034Cu, 64});  // memcpy
    const u32 id = load<u8>(0x00F9C660u) != 0 ? 0x1840AF90u : load<u32>(sp - 4 + 4 * load<u32>(o + 308));
    call(ctx, 0x0042C00Cu, {id});  // Group_ReleaseById_0042C00C
    const u32 w = load<u32>(kWorld29);
    store<u8>(w + 0x001285A5u, 0);
    ctx.gpr[3] = w;
}
