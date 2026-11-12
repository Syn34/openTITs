// Game code, medium sweep part 31: three hit-message handlers of the
// mover family (message kind 2 carries a damage time in its f32 +4) and
// two damage/kill handlers.
#include "mid31.h"

using namespace sly4;
using namespace sly4::mid31;

namespace {
using namespace m31fn;

constexpr u32 CompF32_31 = 0x002CEF30u;   // (comp; f1 t) -> bool: the hit-timer update
constexpr u32 BodySync31 = 0x0015F104u;   // Body_SyncRotation_0015F104(o)

// The shared head of the handlers: o's vslot 64 result c gets vslot 8 and
// vslot 12, the object +380 its vslot 12; returns the component
// 0x000D1288(o, 44, 0) and (in *own) the owner Obj_Owner0D93FC(o).
u32 MsgHead31(PPUContext& ctx, u32 o, u32* own) {
    const u32 c = static_cast<u32>(VCall(ctx, o, 64, {o}));
    VCall(ctx, c, 8, {c});
    VCall(ctx, c, 12, {c});
    const u32 p = load<u32>(o + 380);
    VCall(ctx, p, 12, {p});
    const u32 r = static_cast<u32>(call(ctx, 0x000D1288u, {o, 44, 0}));
    *own = static_cast<u32>(call(ctx, 0x000D93FCu, {o}));  // Obj_Owner0D93FC
    return r;
}

// The "give up" tail: o's vslot 28(o, 1, 1); r3 = 0.
void GiveUp31(PPUContext& ctx, u32 o) {
    VCall(ctx, o, 28, {o, 1, 1});
    ret(ctx, 0u);
}
}  // namespace

// bool f(T* o, Msg* m): not kind 2 -> 1. The hit timer (0x002CEF30(comp,
// m f32 +4)) expired -> Body_SyncRotation_0015F104(o), 0. With an owner,
// u8 +671 bit 0x80 set and 0x40 clear: comp f32 +348 = -1; then with
// (+16)->+4 set the owner's vslot 8 record gets word +4 = 0 and its state
// bits 0xFC000000 cleared (1 when owner +40 has 0x80000000, else give up);
// otherwise the def (+216) +56 key resolves (0x005534D0 under 0x00ACB310,
// 0x0056A710, the map 0x0105752C) to a value that, when non-zero, goes to
// the record's +4 with state bits 0x24000000; r3 = 1. Everything else gives
// up (vslot 28(o, 1, 1), 0).
DECOMP(00164210, HitMsg_Grab_00164210) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 m = arg<u32>(ctx, 1);
    u32 own;
    const u32 comp = MsgHead31(ctx, o, &own);
    if (load<u32>(m) != 2) {
        ret(ctx, 1u);
        return;
    }
    callf(ctx, CompF32_31, {comp}, {Lf(m + 4)});
    if (static_cast<u32>(ctx.gpr[3]) == 0) {
        call(ctx, BodySync31, {o});
        ret(ctx, 0u);
        return;
    }
    const u8 b = load<u8>(o + 671);
    if (own == 0 || (b & 0x80) == 0 || (b & 0x40) != 0) {
        GiveUp31(ctx, o);
        return;
    }
    store<u32>(comp + 348, 0xBF800000u);  // -1.0f
    if (load<u32>(load<u32>(o + 16) + 4) != 0) {
        const u32 q = static_cast<u32>(VCall(ctx, own, 8, {own}));
        const u32 w = load<u32>(q) & 0x03FFFFFFu;
        store<u32>(q + 4, 0);
        store<u32>(q, w);
        if ((load<u32>(own + 40) & 0x80000000u) != 0) {
            ret(ctx, 1u);
            return;
        }
        GiveUp31(ctx, o);
        return;
    }
    const u32 a = static_cast<u32>(call(ctx, 0x005534D0u, {0x00ACB310u, load<u32>(o + 216) + 56}));
    const u32 h = static_cast<u32>(call(ctx, 0x0056A710u, {a}));
    Stack frame(ctx, 16);
    store<u32>(frame.addr(), h);
    const u32 e = static_cast<u32>(call(ctx, 0x00749A14u, {0x0105752Cu, frame.addr()}));  // SortedMap_Find749A14
    const u32 v = load<u32>(e);
    if (v != 0) {
        const u32 q = static_cast<u32>(VCall(ctx, own, 8, {own}));
        const u32 w = (load<u32>(q) & 0x03FFFFFFu) | 0x24000000u;
        store<u32>(q + 4, v);
        store<u32>(q, w);
    }
    ret(ctx, 1u);
}

// bool f(T* o, Msg* m): not kind 2 -> 1; hit timer expired ->
// Body_SyncRotation_0015F104(o), 0; no owner -> give up. With (+16)->+4
// clear the owner's +136 = 0 and +40 loses bits 0x80000000 / 0x01FE0000;
// with it set, an owner without +40 bit 0x40000000 gives up. Then, when
// the owner's +140 target exists, o's 64-bit +184 has bit 0, the word at
// +20 + u16 +6 has 0x80000000 and (+12) f32 +4 is below *0x01038BE8: the
// colour (4 bytes, by the target's +72 kind: 0x01038B04 for 0x00011A6B,
// 0x01038AB8 for 0x8CFF6308, else 0x01038B50) goes to o's vslot 120(o,
// colour, -, 1; f1 = *0x01038B9C). r3 = 1.
DECOMP(001644C8, HitMsg_Flash_001644C8) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 m = arg<u32>(ctx, 1);
    u32 own;
    const u32 comp = MsgHead31(ctx, o, &own);
    if (load<u32>(m) != 2) {
        ret(ctx, 1u);
        return;
    }
    const u32 flags = load<u32>(o + 20) + load<u16>(o + 6);
    callf(ctx, CompF32_31, {comp}, {Lf(m + 4)});
    if (static_cast<u32>(ctx.gpr[3]) == 0) {
        call(ctx, BodySync31, {o});
        ret(ctx, 0u);
        return;
    }
    if (own == 0) {
        GiveUp31(ctx, o);
        return;
    }
    if (load<u32>(load<u32>(o + 16) + 4) == 0) {
        const u32 x = load<u32>(own + 40) & 0x7FFFFFFFu & ~0x01FE0000u;
        store<u32>(own + 136, 0);
        store<u32>(own + 40, x);
    } else if ((load<u32>(own + 40) & 0x40000000u) == 0) {
        GiveUp31(ctx, o);
        return;
    }
    const u32 n = load<u32>(own + 140);
    if (n == 0 || (load<u64>(o + 184) & 1) == 0 || (load<u32>(flags) & 0x80000000u) == 0) {
        ret(ctx, 1u);
        return;
    }
    const f64 lim = Lf(0x01038BE8u), v = Lf(load<u32>(o + 12) + 4);
    ctx.fpr[1] = lim;
    ctx.fpr[2] = v;
    if (!(v < lim)) {
        ret(ctx, 1u);
        return;
    }
    Stack frame(ctx, 16);
    const u32 col = frame.addr();
    u32 src = 0x01038B50u;
    const s32 t = static_cast<s32>(load<u32>(n + 72));
    if (t > static_cast<s32>(0x8CFF6308u)) {
        if (t == 0x00011A6B) src = 0x01038B04u;
    } else if (t > static_cast<s32>(0x8CFF6307u)) {
        src = 0x01038AB8u;
    }
    for (u32 k = 0; k < 4; ++k) store<u8>(col + k, load<u8>(src + k));  // the default bytes are stored first, then overwritten
    ctx.fpr[1] = Lf(0x01038B9Cu);
    const u32 opd = load<u32>(load<u32>(o) + 120);
    call_ptr(ctx, opd, {o, col, opd, 1, load<u32>(opd)});
    ret(ctx, 1u);
}

// bool f(T* o, Msg* m): not kind 2 -> 1; hit timer expired ->
// Body_SyncRotation_0015F104(o), 0. The comp's 4 status bytes
// (0x002CE360(comp, b)) choose the reaction: with (+16)->+8 clear, b[3]
// clear and 0x002CDD68(comp, 3, .., 0, 0; 2, 10) -> anim 0xB8E15655; else
// b[1] clear and 0x002CDD68(comp, 1, ..; 2, 10) -> anim 0xB8E1565B; else
// anim 0xC0424C88 (Anim_Restart2775A8 on o's vslot 60 animator, f32s 1,
// 0.25, 0, 0); then 0x0015FCF8(o, b; t), r3 = 1. With (+16)->+8 set:
// 0x00160000(o, b) -> 0; else Anim_TrackSet36_0029879C(animator, 0, 0)
// non-zero gives up, zero does the 0x0015FCF8 tail.
DECOMP(00160A80, HitMsg_React_00160A80) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 m = arg<u32>(ctx, 1);
    u32 own;
    const u32 comp = MsgHead31(ctx, o, &own);
    if (load<u32>(m) != 2) {
        ret(ctx, 1u);
        return;
    }
    const f64 t = Lf(m + 4);
    callf(ctx, CompF32_31, {comp}, {t});
    if (static_cast<u32>(ctx.gpr[3]) == 0) {
        call(ctx, BodySync31, {o});
        ret(ctx, 0u);
        return;
    }
    Stack frame(ctx, 16);
    const u32 b = frame.addr();  // 144(r1) in the original
    for (u32 k = 0; k < 4; ++k) store<u8>(b + k, 0);
    call(ctx, 0x002CE360u, {comp, b});
    constexpr u32 K = 0x00160A68u;  // f32 {0, 1, 0.25, 2, 10}
    if (load<u32>(load<u32>(o + 16) + 8) == 0) {
        u32 key = 0xC0424C88u;
        u64 a5 = 0, a6 = ~0ull;  // li -1: all 64 bits
        bool picked = false;
        if (load<u8>(b + 3) == 0) {
            callf(ctx, 0x002CDD68u, {comp, 3, K, 0 /* r6: not read */, 0, 0}, {Lf(K + 12), Lf(K + 16)});
            if (static_cast<u32>(ctx.gpr[3]) != 0) {
                key = 0xB8E15655u;
                picked = true;
            }
        }
        if (!picked && load<u8>(b + 1) == 0) {
            callf(ctx, 0x002CDD68u, {comp, 1, K, 0 /* r6: not read */, 0, 0}, {Lf(K + 12), Lf(K + 16)});
            if (static_cast<u32>(ctx.gpr[3]) != 0) {
                key = 0xB8E1565Bu;
                a5 = ~0ull;
                a6 = 0;
            }
        }
        const u32 an = static_cast<u32>(VCall(ctx, o, 60, {o}));
        // args 9 / 10 are stack words the original leaves unwritten (passed as 0)
        callf(ctx, 0x002775A8u, {an, key, 0, 0, a5, a6, 0, 0, 0, 0, 0, ~0ull}, {Lf(K + 4), Lf(K + 8), Lf(K), Lf(K)});  // Anim_Restart2775A8
    } else {
        if (static_cast<u32>(call(ctx, 0x00160000u, {o, b})) != 0) {
            ret(ctx, 0u);
            return;
        }
        const u32 an = static_cast<u32>(VCall(ctx, o, 60, {o}));
        if (static_cast<u32>(call(ctx, 0x0029879Cu, {an, 0, 0})) != 0) {  // Anim_TrackSet36_0029879C
            GiveUp31(ctx, o);
            return;
        }
    }
    callf(ctx, 0x0015FCF8u, {o, b}, {t});
    ret(ctx, 1u);
}

namespace {
// The health clamp of the damage handler: h = min(a, b) (b when b < a)
// when that is above z, else z; returns the f32 word stored.
u32 ClampMin31(u32 aw, u32 bw, u32 zw) {
    const f64 a = LfsF(aw), b = LfsF(bw), z = LfsF(zw);
    const f64 m = b < a ? b : a;
    if (!(m > z)) return zw;
    return b < a ? bw : aw;
}
}  // namespace

// bool f(T* o, -, Obj* src, u32 x; f32 dmg): o's vslot 380 state 11 -> 0;
// dmg <= 0, state 2, a src with +192 bit 0x100, or a src with +192 bit
// 0x2000 whose rider +432 has bit 0x100 -> 1. The 0xC493B59D component of
// the effect +356 in state 2 (vslot 32) takes a src with +172 bit 0x40000
// as its +20 value (src +216's +52, 0 without it). Unless o is the player
// ((*0x00F65CA0)->+4216), health +348 = min(+344, player +348) (0 when not
// positive); when then positive, Obj_ApplyDamage(o, -, src, x; dmg), +1648
// = 0 and, health now 0, vslot 376(o, 11). The player's +348 = min(o's
// health, player +344) likewise. r3 = health > 0.
DECOMP(001857A0, Obj_TakeHit_001857A0) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 src = arg<u32>(ctx, 2);
    const u32 x = arg<u32>(ctx, 3);
    const f64 dmg = farg(ctx, 0);
    if (static_cast<s32>(VCall(ctx, o, 380, {o})) == 11) {
        ret(ctx, 0u);
        return;
    }
    constexpr u32 kZ = 0x0018579Cu;  // 0.0f
    const u32 zw = load<u32>(kZ);
    const f64 z = LfsF(zw);
    if (dmg < z || dmg == z) {
        ret(ctx, 1u);
        return;
    }
    if (static_cast<s32>(VCall(ctx, o, 380, {o})) == 2) {
        ret(ctx, 1u);
        return;
    }
    if (src != 0 && (load<u32>(src + 192) & 0x100u) != 0) {
        ret(ctx, 1u);
        return;
    }
    if (src != 0 && (load<u32>(src + 192) & 0x2000u) != 0) {
        const u32 p = load<u32>(src + 432);
        if (p != 0 && (load<u32>(p + 192) & 0x100u) != 0) {
            ret(ctx, 1u);
            return;
        }
    }
    const u32 e = static_cast<u32>(call(ctx, FindComp31, {load<u32>(o + 356), 0xC493B59Du}));
    if (e != 0 && static_cast<s32>(VCall(ctx, e, 32, {e})) == 2 && src != 0 && (load<u32>(src + 172) & 0x40000u) != 0) {
        const u32 v = load<u32>(src + 216);
        store<u32>(e + 20, v != 0 ? load<u32>(v + 52) : 0u);
    }
    const u32 kPlayer = load<u32>(0x00F65CA0u) + 4216;
    u32 g = load<u32>(kPlayer);
    if (o != g) store<u32>(o + 348, ClampMin31(load<u32>(o + 344), load<u32>(g + 348), zw));
    if (Lf(o + 348) > z) {
        callf(ctx, 0x000E1738u, {o, 0 /* r4: not read */, src, x}, {dmg});  // Obj_ApplyDamage
        store<u32>(o + 1648, 0);
        if (Lf(o + 348) == z) VCall(ctx, o, 376, {o, 11});
    }
    ctx.fpr[1] = Lf(o + 348);
    g = load<u32>(kPlayer);
    if (o != g) {
        ctx.fpr[2] = Lf(g + 344);
        store<u32>(g + 348, ClampMin31(load<u32>(g + 344), load<u32>(o + 348), zw));
    }
    ctx.fpr[2] = Lf(o + 348);
    ret(ctx, Lf(o + 348) > z ? 1u : 0u);
}

// bool f(T* o, -, Obj* src, u32 x; f32 dmg): dmg <= 0 -> 1. While any of
// the u16 +604 parts (+596) has a shield (+620) with u8 +53 set, r3 =
// health +348 > 0 and nothing happens. Otherwise, when alive,
// Obj_ApplyDamage(o, src, src, x; dmg); if that kills it: the body +388 (if
// any) leaves its world (Hk_RemoveEntity92FFC8), loses the properties
// 0xA734004D / 0x91C7CF5B (Hk_RemoveProperty93FED8) and is released
// (AnimSkel_Release7D4EA8, +388 = 0); flag 0x1000 of +172 is set, the
// world matrix ((+160) +16, rows of three) goes through vslot 84(o, m) and
// back into o's +32 rows, then vslot 376(o, 8). r3 = health > 0.
DECOMP(00192F00, Obj_TakeHitShielded_00192F00) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 src = arg<u32>(ctx, 2);
    const u32 x = arg<u32>(ctx, 3);
    const f64 dmg = farg(ctx, 0);
    const f64 z = Lf(0x00192EF8u);  // 0.0f
    if (dmg < z || dmg == z) {
        ret(ctx, 1u);
        return;
    }
    const u32 n = load<u16>(o + 604);
    const u32 parts = load<u32>(o + 596);
    f64 h = Lf(o + 348);
    ctx.fpr[2] = h;
    for (u32 i = 0; i < n; ++i) {
        const u32 s = load<u32>(load<u32>(parts + 4 * i) + 620);
        if (s != 0 && load<u8>(s + 53) != 0) {
            ret(ctx, h > z ? 1u : 0u);
            return;
        }
    }
    if (h > z) {
        callf(ctx, 0x000E1738u, {o, src, src, x}, {dmg});  // Obj_ApplyDamage
        h = Lf(o + 348);
        ctx.fpr[2] = h;
        if (h < z || h == z) {
            Stack frame(ctx, 96);
            const u32 m = frame.addr();  // 112(r1); the Havok results at 176 / 184 / 192(r1)
            for (u32 r = 0; r < 4; ++r)
                for (u32 c = 0; c < 3; ++c) store<u32>(m + 16 * r + 4 * c, load<u32>(load<u32>(o + 160) + 16 + 16 * r + 4 * c));
            u32 b = load<u32>(o + 388);
            if (b != 0) {
                const u32 w = load<u32>(b + 8);
                if (w != 0) {
                    call(ctx, 0x0092FFC8u, {m + 80, w, b});  // Hk_RemoveEntity92FFC8
                    b = load<u32>(o + 388);
                }
                call(ctx, 0x0093FED8u, {m + 64, b, 0xA734004Du});  // Hk_RemoveProperty93FED8
                call(ctx, 0x0093FED8u, {m + 72, load<u32>(o + 388), 0x91C7CF5Bu});
                call(ctx, 0x007D4EA8u, {load<u32>(o + 388)});  // AnimSkel_Release7D4EA8
                store<u32>(o + 388, 0);
            }
            store<u32>(o + 172, load<u32>(o + 172) | 0x1000u);
            VCall(ctx, o, 84, {o, m});
            for (u32 r = 0; r < 3; ++r)
                for (u32 c = 0; c < 3; ++c) store<u32>(o + 32 + 16 * r + 4 * c, load<u32>(m + 16 * r + 4 * c));
            for (u32 c = 0; c < 3; ++c) store<u32>(o + 80 + 4 * c, load<u32>(m + 48 + 4 * c));
            ctx.fpr[1] = Lf(m + 56);  // the copy runs through f1
            VCall(ctx, o, 376, {o, 8});
            h = Lf(o + 348);
            ctx.fpr[2] = h;
        }
    }
    ret(ctx, h > z ? 1u : 0u);
}

// bool f(T* o, -, Obj* src, u32 x; f32 dmg): 1 (nothing done) for self
// damage, with the global hit lock u8 *0x010388F0, o's 64-bit flags +184
// bits 0x20000 / 0x40000, an owner with +40 bit 0x1000, a hit cooldown
// +380 above 0, dmg <= 0 or o's vslot 32(o, 0) state 1. Then (state
// re-read, not 1) with health +348 above 0: dmg >= health kills: vslot
// 28(o, 0, 1), vslot 28(o, 1, 0), the killer x into *(+20) and (+364) +20,
// Obj_ApplyDamage(o, .., src, x; health - min(health * 0.01, 1)); otherwise
// Obj_ApplyDamage(o, .., src, x; dmg), flag 0x40000000 of +384 and the
// cooldown +380 = def (+216) +724. Either way a zeroed 24-byte event
// (EventInit_00465494(ev, o, 0, 0, 0)) goes to World_Query482D14(game,
// (s8) u8 +172, ev). r3 = health > 0.
DECOMP(00229738, Obj_TakeHitTimed_00229738) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 src = arg<u32>(ctx, 2);
    const u32 x = arg<u32>(ctx, 3);
    const f64 dmg = farg(ctx, 0);
    if (o == src || load<u8>(0x010388F0u) != 0 || (load<u32>(o + 188) & 0x60000u) != 0) {
        ret(ctx, 1u);
        return;
    }
    if (static_cast<u32>(call(ctx, 0x000D93FCu, {o})) != 0) {  // Obj_Owner0D93FC (asked twice)
        const u32 ow = static_cast<u32>(call(ctx, 0x000D93FCu, {o}));
        if ((load<u32>(ow + 40) & 0x1000u) != 0) {
            ret(ctx, 1u);
            return;
        }
    }
    constexpr u32 K = 0x00229720u;  // f32 {0, 0.01, 1}, f64 1 at +16
    const f64 z = Lf(K);
    ctx.fpr[1] = Lf(o + 380);
    if (Lf(o + 380) > z || dmg < z || dmg == z || static_cast<s32>(VCall(ctx, o, 32, {o, 0})) == 1) {
        ret(ctx, 1u);
        return;
    }
    const u32 d = load<u32>(o + 216);
    const f64 h = Lf(o + 348);
    if (static_cast<s32>(VCall(ctx, o, 32, {o, 0})) != 1 && h > z) {
        if (dmg > h || dmg == h) {
            VCall(ctx, o, 28, {o, 0, 1});
            VCall(ctx, o, 28, {o, 1, 0});
            const u32 p = load<u32>(o + 20);
            const u32 q = load<u32>(o + 364);
            store<u32>(p, x);
            if (q != 0) store<u32>(q + 20, x);
            const f64 a = Fmuls31(h, Lf(K + 4));
            const f64 c = Sgl28(a - Lf(K + 8));
            const f64 s = Sgl28(Fsel28(c, Ld30(K + 16), a));
            callf(ctx, 0x000E1738u, {o, p, src, x}, {Sgl28(h - s)});  // Obj_ApplyDamage
        } else {
            callf(ctx, 0x000E1738u, {o, 0 /* r4: not read */, src, x}, {dmg});
            const u32 fl = load<u32>(o + 384) | 0x40000000u;
            store<u32>(o + 380, load<u32>(d + 724));
            store<u32>(o + 384, fl);
        }
        Stack frame(ctx, 32);
        const u32 ev = frame.addr();
        for (u32 k = 0; k < 6; ++k) store<u32>(ev + 4 * k, 0);
        call(ctx, 0x00465494u, {ev, o, 0, 0, 0});  // EventInit_00465494
        call(ctx, 0x00482D14u, {load<u32>(0x00F65CA0u), static_cast<u64>(static_cast<s64>(static_cast<s8>(load<u8>(o + 172)))), ev});  // World_Query482D14
    }
    ctx.fpr[1] = Lf(o + 348);
    ret(ctx, Lf(o + 348) > z ? 1u : 0u);
}
