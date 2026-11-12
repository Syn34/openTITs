// Game code, medium sweep part 30: an XML-built record with lookup trees,
// a capsule-hit message handler, a state switch, an object reset and the
// volume activation toggle.
#include "mid30.h"

using namespace sly4;
using namespace sly4::mid30;

namespace {
using namespace m30fn;

// The inlined init of one 104-byte lookup tree at t (empty; its +12 points
// at itself, u8 +8 = 1). `selfFirst` picks the original's store order.
void TreeInit30(u32 t, bool selfFirst) {
    if (selfFirst) store<u32>(t + 12, t);
    store<u32>(t + 0, 0);
    store<u32>(t + 4, 0);
    if (!selfFirst) store<u32>(t + 12, t);
    store<u16>(t + 96, 0);
    store<u32>(t + 100, 0);
    store<u8>(t + 8, 1);
}

// The (u32 0x00210FAA, u32 2683) pairs of every <0x40795527> child of the
// cursor x inserted into the tree t (Avl_TreeInsert_0073B444); the two
// hashes are read into the stack words kv / kv + 4 with default 0.
// r3 = 0 at the end (the last NextSibling).
void ReadPairs30(PPUContext& ctx, u32 x, u32 t, u32 kv) {
    for (u32 n = static_cast<u32>(call(ctx, XmlFirst30, {x, 0x40795527u})); n != 0;
         n = static_cast<u32>(call(ctx, XmlNext30, {n, 0x40795527u}))) {
        call(ctx, XmlHash30, {n, 0x00210FAAu, kv, 0});
        call(ctx, XmlHash30, {n, 2683, kv + 4, 0});
        call(ctx, 0x0073B444u, {t, load<u32>(kv), load<u32>(kv + 4)});  // Avl_TreeInsert_0073B444
    }
}

// The 0..255 clamp of an XmlCursor_ReadS32 result as the original does it:
// int -> f32, fsel against 255 (the f32 *0x00070368, the f64s *0x00070370 /
// *0x00070378), truncated through fctidz. Returns the 64-bit fctidz bits.
u64 Clamp255_30(s32 v) {
    const f64 hi = Lf(0x00070368u), cap = Ld30(0x00070370u), floor0 = Ld30(0x00070378u);
    f64 x = Sgl28(static_cast<f64>(v));
    const f64 d = Sgl28(hi - x);
    x = Sgl28(Fsel28(d, x, cap));
    x = Sgl28(Fsel28(x, x, floor0));
    return rt::fctid(x, true);
}
}  // namespace

// void f(T* o, XmlCursor* x): o's constructor body. Base 0x004BB3B0(o),
// vtable 0x00B0F3B0; the tree at +12 (root +12, count +16, its iterator
// stack u16 +108 / +28.., u8 +20 = 1, +24 = &+12) is emptied and its
// leftmost node (none: the root was just cleared, so the walk never runs)
// kept at +112; four more trees at +116 + 108 * i (hash +220 + 108 * i);
// +548.. +564 = 0, u8 +568 / +569 = 255, +571 = 1. Then, unless the hash
// 0x8A93973F (+548) is missing (r3 = that non-zero result): the f32
// 0xB7EA3D94 (+564), bools 0x16C2F884 / 0xF9D05ED8 / 0xC1DF2672
// (+571..+573), hashes 0x455D877F (+552) / 0x11CCCBF1 (+560), the s32s
// 0xAAFFE3D4 / 0xE61203A3 clamped to 0..255 into u8 +568 / +569; every
// <0x40795527> pair goes into tree +12, and up to four <0xCCEB9AF9>
// children fill tree i (hash 0x0024728B -> +220 + 108 * i, pairs) with the
// count in u8 +570. r3 = the last sibling (0); f1 = the second clamp's
// fctidz bits (as the original leaves it).
DECOMP(00070390, XmlTrees_Init_00070390) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 x = arg<u32>(ctx, 1);
    call(ctx, 0x004BB3B0u, {o});  // Obj_InitVt_B21C98
    store<u32>(o + 12, 0);
    store<u32>(o + 16, 0);
    store<u32>(o + 0, 0x00B0F3B0u);
    store<u32>(o + 24, o + 12);
    store<u8>(o + 20, 1);
    store<u16>(o + 108, 0);
    u32 left = 0;
    for (u32 n = load<u32>(o + 12); n != 0;) {  // dead: the root is 0 here
        left = n;
        const u32 l = load<u32>(n + 12);
        if (l != 0) {
            const s16 k = static_cast<s16>(load<u16>(o + 108));
            store<u16>(o + 108, static_cast<u16>(k + 1));
            store<u32>(o + 28 + 4 * static_cast<u32>(static_cast<s32>(k)), n);
        }
        n = l;
    }
    store<u32>(o + 112, left);
    store<u8>(o + 20, 1);
    for (u32 k = 0; k < 2; ++k) {
        TreeInit30(o + 216 * k + 116, true);
        TreeInit30(o + 216 * k + 224, false);
    }
    store<u32>(o + 548, 0);
    store<u32>(o + 552, 0);
    store<u32>(o + 556, 0);
    store<u32>(o + 560, 0);
    store<u32>(o + 564, 0);
    store<u8>(o + 568, 255);
    store<u8>(o + 569, 255);
    store<u8>(o + 570, 0);
    store<u8>(o + 571, 1);
    store<u8>(o + 572, 0);
    store<u8>(o + 573, 0);
    if (call(ctx, XmlHash30, {x, 0x8A93973Fu, o + 548, 0}) != 0) return;
    callf(ctx, XmlF32_30, {x, 0xB7EA3D94u, o + 564}, {Lf(o + 564)});
    call(ctx, XmlBool30, {x, 0x16C2F884u, o + 571, load<u8>(o + 571)});
    call(ctx, XmlBool30, {x, 0xF9D05ED8u, o + 572, load<u8>(o + 572)});
    call(ctx, XmlBool30, {x, 0xC1DF2672u, o + 573, load<u8>(o + 573)});
    call(ctx, XmlHash30, {x, 0x455D877Fu, o + 552, load<u32>(o + 552)});
    call(ctx, XmlHash30, {x, 0x11CCCBF1u, o + 560, load<u32>(o + 560)});
    Stack frame(ctx, 16);  // the s32s at 112(r1) / 116(r1), the pair at 120(r1) / 124(r1)
    const u32 sp = frame.addr();
    call(ctx, XmlS32_30, {x, 0xAAFFE3D4u, sp, 255});
    call(ctx, XmlS32_30, {x, 0xE61203A3u, sp + 4, 255});
    store<u8>(o + 568, static_cast<u8>(Clamp255_30(static_cast<s32>(load<u32>(sp)))));
    const u64 b = Clamp255_30(static_cast<s32>(load<u32>(sp + 4)));
    ctx.fpr[1] = std::bit_cast<f64>(b);
    store<u8>(o + 569, static_cast<u8>(b));
    ReadPairs30(ctx, x, o + 12, sp + 8);
    u32 i = 0;
    for (u32 c = static_cast<u32>(call(ctx, XmlFirst30, {x, 0xCCEB9AF9u})); c != 0 && i < 4;
         c = static_cast<u32>(call(ctx, XmlNext30, {c, 0xCCEB9AF9u})), ++i) {
        call(ctx, XmlHash30, {c, 0x0024728Bu, o + 108 * i + 220, 0});
        ReadPairs30(ctx, c, o + 108 * i + 116, sp + 8);
    }
    store<u8>(o + 570, static_cast<u8>(i));
}

// void f(T* o, ?, Msg* m, f32 f1): a contact message. Unless m is kind 4
// (bits 13..15 of u16 +14) with bit 0x100 of u16 +12: when f1 > *0x000624D4
// and bit 0x1000 of +14 is set, vt[220](o, 4) and, with a target +312,
// Queue_Push_004705F8(o->+48, target, 0x59C04B13, 0, 1). The kind-4 case
// projects the point m->+16..+24 onto the bone 0x00F9BA28's capsule of
// o->+48 (Obj_BoneMatrixByName_000DAABC: the axis row +32.. scaled by
// *0x00F9BB0C * *0x000624D8 about the origin row +48..): inside the
// segment (t in [*0x000624D4, *0x000624DC]) and nearer than *0x00F9BAC0,
// o->+344 += 1, (o->+48)->vt[128](0x5640C070, 1) and a zeroed event
// (EventInit_00465534(ev, o->+48, 0, 0, 0)) is posted to World_Query482D14
// (*0x00F65CA0, 0, ev); otherwise vt[128](0xA0885876, 1). Both clear bit
// 0x80 of u8 o->+305 (r3 = that byte). r3 / f1 as the original leaves them.
DECOMP(000624E0, Msg_CapsuleHit_000624E0) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 m = arg<u32>(ctx, 2);
    const f64 f1 = ctx.fpr[1];
    const u32 c = 0x000624D4u;
    const u32 h14 = load<u16>(m + 14);
    if (((h14 >> 13) & 7) != 4 || (load<u16>(m + 12) & 0x100u) == 0) {
        if (!(f1 > Lf(c))) {
            ctx.gpr[3] = h14;
            return;
        }
        if ((h14 & 0x1000u) == 0) {
            ctx.gpr[3] = 0;
            return;
        }
        VCall(ctx, o, 220, {o, 4});
        const u32 t = load<u32>(o + 312);
        if (t == 0) {
            ctx.gpr[3] = 0;
            return;
        }
        call(ctx, 0x004705F8u, {load<u32>(o + 48), t, 0x59C04B13u, 0, 1});  // Queue_Push_004705F8
        return;
    }
    const f64 px = Lf(m + 16), py = Lf(m + 20), pz = Lf(m + 24);
    Stack frame(ctx, 96);  // the matrix at 112(r1)..175(r1), the event at 176(r1)..199(r1)
    const u32 sp = frame.addr();
    call(ctx, 0x000DAABCu, {sp, load<u32>(o + 48), load<u32>(0x00F9BA28u), 1, 1});  // Obj_BoneMatrixByName_000DAABC
    const f64 len = Lf(0x00F9BB0Cu);
    const f64 l = Sgl28(len * Lf(c + 4));
    const f64 l2 = Sgl28(len * len);
    const f64 m32 = Lf(sp + 32), m36 = Lf(sp + 36), m40 = Lf(sp + 40);
    const f64 m48 = Lf(sp + 48), m52 = Lf(sp + 52), m56 = Lf(sp + 56);
    const f64 ay = Sgl28(-std::fma(m36, l, -m52)), by = Sgl28(std::fma(m36, l, m52));
    const f64 ax = Sgl28(-std::fma(m32, l, -m48)), bx = Sgl28(std::fma(m32, l, m48));
    const f64 az = Sgl28(-std::fma(m40, l, -m56)), bz = Sgl28(std::fma(m40, l, m56));
    f64 dy = Sgl28(py - ay);
    const f64 ey = Sgl28(by - ay);
    f64 dx = Sgl28(px - ax);
    const f64 ex = Sgl28(bx - ax);
    f64 dz = Sgl28(pz - az);
    const f64 ez = Sgl28(bz - az);
    const u32 g = load<u32>(o + 48);
    f64 t = Sgl28(ey * dy);
    t = Sgl28(std::fma(ex, dx, t));
    t = Sgl28(std::fma(ez, dz, t));
    t = Sgl28(t / l2);
    ctx.fpr[1] = dz;
    bool hit = false;
    if (t >= Lf(c) && t <= Lf(c + 8)) {
        dy = Sgl28(-std::fma(t, ey, -dy));
        dx = Sgl28(-std::fma(t, ex, -dx));
        dz = Sgl28(-std::fma(t, ez, -dz));
        const f64 r = Lf(0x00F9BAC0u);
        const f64 s = Sgl28(std::fma(dz, dz, Sgl28(std::fma(dx, dx, Sgl28(dy * dy)))));
        ctx.fpr[1] = s;
        hit = s < Sgl28(r * r);
    }
    if (!hit) {
        VCall(ctx, g, 128, {g, 0xA0885876u, 1});
    } else {
        store<u32>(o + 344, load<u32>(o + 344) + 1);
        VCall(ctx, g, 128, {g, 0x5640C070u, 1});
        const u32 ev = sp + 64;
        for (u32 k = 0; k < 24; k += 4) store<u32>(ev + k, 0);
        call(ctx, 0x00465534u, {ev, load<u32>(o + 48), 0, 0, 0});  // EventInit_00465534
        call(ctx, 0x00482D14u, {load<u32>(kWorld29), 0, ev});      // World_Query482D14
    }
    const u8 v = load<u8>(o + 305) & 0x7F;
    store<u8>(o + 305, v);
    ctx.gpr[3] = v;
}

// u32 f(T* o, s32 s): switches o to state s. vt[232](o) when the state +4
// is set (not -1); o->+76 = 0; bit 0x800 of u16 (o->+48)->+176 cleared.
// With a body +60: bit 0x4000 of its u64 +184 cleared,
// Obj_Flush480_00129B58(body) and body->vt[60]()->+380 = 1.0f. With +68:
// Obj_SetPhysicsActive(+68, 1). o->+48 is swap-removed from the game
// list (*0x00F557F0: count +24, entries +3732 whose first word is the
// object). Bit 0x20 of u8 +304 cleared; an effect +72 is deleted
// (Effect_Delete) and cleared. A real change sends (o->+48)->vt[128]
// (0x1D6F9964, 0). Bit 0x10 of u8 +305 cleared; +44 = +44 + 1 when the
// state stays, else 0; +4 = s and the history slot +8 + 4 * +24 = s;
// returns vt[224](o).
DECOMP(000149E0, State_Switch_000149E0) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 s = arg<u32>(ctx, 1);
    if (load<u32>(o + 4) != 0xFFFFFFFFu) VCall(ctx, o, 232, {o});
    store<u32>(o + 76, 0);
    {
        const u32 p = load<u32>(o + 48);
        store<u16>(p + 176, load<u16>(p + 176) & ~0x0800u);
    }
    if (const u32 b = load<u32>(o + 60); b != 0) {
        store<u64>(b + 184, load<u64>(b + 184) & ~0x4000ull);
        call(ctx, 0x00129B58u, {b});  // Obj_Flush480_00129B58
        const u32 bb = load<u32>(o + 60);
        const u32 r = static_cast<u32>(VCall(ctx, bb, 60, {bb}));
        store<u32>(r + 380, 0x3F800000u);
    }
    if (const u32 ph = load<u32>(o + 68); ph != 0) call(ctx, 0x00260C14u, {ph, 1});  // Obj_SetPhysicsActive
    const u32 g = load<u32>(kGame30);
    u32 me = load<u32>(o + 48);  // r3
    for (s32 i = 0; i < static_cast<s32>(load<u32>(g + 24)); ++i) {
        const u32 arr = g + 3732;
        if (load<u32>(load<u32>(arr + 4 * static_cast<u32>(i))) != me) continue;
        store<u32>(g + 24, load<u32>(g + 24) - 1);
        const u32 n = load<u32>(g + 24);
        const u32 a = load<u32>(arr + 4 * static_cast<u32>(i));
        const u32 b = load<u32>(arr + 4 * n);
        store<u32>(arr + 4 * n, a);
        store<u32>(arr + 4 * static_cast<u32>(i), b);
        --i;
    }
    store<u8>(o + 304, load<u8>(o + 304) & ~0x20u);
    if (const u32 e = load<u32>(o + 72); e != 0) {
        call(ctx, 0x004D0280u, {e});  // Effect_Delete
        me = load<u32>(o + 48);
        store<u32>(o + 72, 0);
    }
    if (load<u32>(o + 4) != s) VCall(ctx, me, 128, {me, 0x1D6F9964u, 0});
    const bool same = load<u32>(o + 4) == s;
    store<u8>(o + 305, load<u8>(o + 305) & ~0x10u);
    const u32 slot = load<u32>(o + 24);
    store<u32>(o + 44, same ? load<u32>(o + 44) + 1 : 0);
    store<u32>(o + 4, s);
    store<u32>(o + 4 * slot + 8, s);
    VCall(ctx, o, 224, {o});
}

// u32 f(T* o): resets o. Obj_SetEffectHash_000DAEC8(o->+48, 0xB39ED94D);
// vt[8]() of the five pairs +5104.. / +5084..; vt[40](1) of each set slot
// of +1756..+2779; vt[40](0) of the +6168 entries at +5144.. (count
// cleared). The local player (*0x00F65CA0 + 4216) with bits 2 and 8 of
// +192 loses bit 0x40 of u8 +710. A handle +6216 is released
// (0x000FBB88(world->+0x1281E0, h; f1 = *0x000530B0) when that is set,
// then h->vt[4]()) and cleared; +6228 with bit 0x40000 of +172 gets
// vt[40](1); +6228, (*0x00F557F0)->+36 and +6272 are cleared and the result
// is Obj_ResetBase12928(o).
DECOMP(000530B8, Obj_ResetAll_000530B8) {
    const u32 o = arg<u32>(ctx, 0);
    call(ctx, 0x000DAEC8u, {load<u32>(o + 48), 0xB39ED94Du});  // Obj_SetEffectHash_000DAEC8
    for (u32 i = 0; i < 5; ++i) {
        const u32 a = load<u32>(o + 5104 + 4 * i);
        VCall(ctx, a, 8, {a});
        const u32 b = load<u32>(o + 5084 + 4 * i);
        VCall(ctx, b, 8, {b});
    }
    for (u32 i = 0; i < 256; ++i) {
        const u32 p = load<u32>(o + 1756 + 4 * i);
        if (p != 0) VCall(ctx, p, 40, {p, 1});
    }
    if (static_cast<s32>(load<u32>(o + 6168)) > 0) {
        s32 i = 0;
        do {
            const u32 p = load<u32>(o + 5144 + 4 * static_cast<u32>(i));
            VCall(ctx, p, 40, {p, 0});
            ++i;
        } while (i < static_cast<s32>(load<u32>(o + 6168)));
    }
    u32 w = load<u32>(kWorld29);
    store<u32>(o + 6168, 0);
    if (const u32 q = load<u32>(w + 4216); q != 0 && (load<u32>(q + 192) & 2) != 0 && (load<u32>(q + 192) & 8) != 0) {
        store<u8>(q + 710, load<u8>(q + 710) & ~0x40u);
        w = load<u32>(kWorld29);
    }
    if (u32 h = load<u32>(o + 6216); h != 0) {
        if (const u32 a = load<u32>(w + 0x001281E0u); a != 0) {
            callf(ctx, 0x000FBB88u, {a, h}, {Lf(0x000530B0u)});
            h = load<u32>(o + 6216);
        }
        if (h != 0) VCall(ctx, h, 4, {h});
        store<u32>(o + 6216, 0);
    }
    if (const u32 p = load<u32>(o + 6228); p != 0 && (load<u32>(p + 172) & 0x00040000u) != 0) VCall(ctx, p, 40, {p, 1});
    store<u32>(o + 6228, 0);
    store<u32>(load<u32>(kGame30) + 36, 0);
    store<u32>(o + 6272, 0);
    call(ctx, 0x00012928u, {o});  // Obj_ResetBase12928
}

// u32 f(Volume* v, u8 on, u8 force, u8 quiet): sets v's active bit
// 0x40000000 of +356 (clearing 0x20000000 / 0x10000000 / 0x08000000);
// nothing when unforced and unchanged (r3 = the bit). Turning off: the
// +20 / u16 +28 effects' vt[272]() components get Effect_SetHash4CF988
// 0xA86F8ED1 when the bit was already off, else 0xF13F8BB2 (unless the
// kind +316 is 1). Then: with a probe +336 and not quiet,
// Probe_SetOn39D1D4(v, 0). Otherwise kind 1: Probe_SetOn39D1D4(v, 0) and,
// turning on from off, bit 0x80000000 set, vt[40](1, 1) of each +0 / u16
// +8 entry, bit 0x08000000 set with +344 / +348 = *0x0106D5CC /
// *0x0106D618; other kinds: Probe_SetOn39D1D4(v, bit). When the bit is on,
// Probe_Prepare39D054(v). r3 / f1 as the original leaves them.
DECOMP(0039D32C, Volume_Activate39D32C) {
    const u32 v = arg<u32>(ctx, 0);
    const u32 r4 = arg<u32>(ctx, 1);
    const u32 on = r4 & 0xFF;
    const u32 force = arg<u32>(ctx, 2) & 0xFF;
    const u32 quiet = arg<u32>(ctx, 3);
    if (force == 0 && ((load<u32>(v + 356) >> 30) & 1) == on) {
        ctx.gpr[3] = on;
        return;
    }
    const u32 old = load<u32>(v + 356);
    const u32 nw = (old & ~0x78000000u) | ((r4 & 1) << 30);
    const u32 diff = ((old >> 30) & 1) - on;  // r30
    store<u32>(v + 356, nw);
    if (((nw >> 30) & 1) == 0 && (diff == 0 || load<u32>(v + 316) != 1)) {
        const u32 h = diff == 0 ? 0xA86F8ED1u : 0xF13F8BB2u;
        for (u32 i = 0; i < load<u16>(v + 28); ++i) {
            const u32 e = load<u32>(load<u32>(v + 20) + 4 * i);
            if (e == 0) continue;
            const u32 r = static_cast<u32>(VCall(ctx, e, 272, {e}));
            if (r != 0) call(ctx, 0x004CF988u, {r, h});  // Effect_SetHash4CF988
        }
    }
    if (load<u32>(v + 336) != 0 && (quiet & 0xFF) == 0) {
        call(ctx, 0x0039D1D4u, {v, 0});  // Probe_SetOn39D1D4
        return;
    }
    u32 st;
    if (load<u32>(v + 316) != 1) {
        call(ctx, 0x0039D1D4u, {v, (load<u32>(v + 356) >> 30) & 1});
        st = load<u32>(v + 356);
    } else {
        call(ctx, 0x0039D1D4u, {v, 0});
        st = load<u32>(v + 356);
        if (((st >> 30) & 1) == 0 && diff != 0) {
            store<u32>(v + 356, st | 0x80000000u);
            for (u32 i = 0; i < load<u16>(v + 8); ++i) {
                const u32 e = load<u32>(load<u32>(v + 0) + 4 * i);
                if (e != 0) VCall(ctx, e, 40, {e, 1, 1});
            }
            st = load<u32>(v + 356) | 0x08000000u;
            store<u32>(v + 344, load<u32>(0x0106D5CCu));
            store<u32>(v + 348, load<u32>(0x0106D618u));
            ctx.fpr[1] = Lf(0x0106D618u);
            store<u32>(v + 356, st);
        }
    }
    if (((st >> 30) & 1) == 0) {
        ctx.gpr[3] = 0;
        return;
    }
    call(ctx, 0x0039D054u, {v});  // Probe_Prepare39D054
}
