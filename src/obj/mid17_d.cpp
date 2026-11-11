// Game code, medium sweep part 17: visibility and misc.
#include "mid17.h"

using namespace sly4;
using namespace sly4::mid17;

namespace {
// The screen box test of Obj_IsHidden3EFC28: p = the projected {x, y}.
// f1 is left as the compiled tests leave it: the first test (first) loads
// hi into f1 once x >= lo, the loop tests load y into f1 instead.
inline bool OffBox(u32 p, f64 lo, f64 hi, bool first, f64& f1) {
    const f64 x = Lf(p);
    f1 = x;
    if (x < lo) return true;
    if (first) f1 = hi;
    if (x > hi) return true;
    const f64 y = Lf(p + 4);
    if (!first) f1 = y;
    return y < lo || y > hi;
}
}  // namespace

// bool Obj_IsHidden3EFC28(Obj* o)
// (called by 003F4198) 1 when o is out of view: o->188 set, o's point +96
// projects (View_ProjectPoint with the view *(game + 0x128168)) outside
// [K0, K1]^2 (K = 0x003EFC20), so does the +144 point of every part in
// o->112[o->120] (each needs its +16 flag and +8 object, else 0), no entry
// of o->144[o->152] has a non-zero u16 +40, and Vec_AnyOnScreen_003EDB08
// returns non-zero for every entry of o->164[o->172]. Else 0.
DECOMP(003EFC28, Obj_IsHidden3EFC28) {
    const u32 o = arg<u32>(ctx, 0);
    Stack frame(ctx, 192);
    const u32 sp = frame.addr();
    auto view = []() { return load<u32>(load<u32>(0x00F65CA0u) + 0x128168u); };
    f64 f1 = ctx.fpr[1];
    auto no = [&]() { ctx.fpr[1] = f1; ret(ctx, 0); };
    if (load<u8>(o + 188) == 0) return no();
    call(ctx, 0x00522750u, {view(), sp + 128, o + 96});  // View_ProjectPoint
    const f64 lo = Lf(0x003EFC20u), hi = Lf(0x003EFC24u);
    if (!OffBox(sp + 128, lo, hi, true, f1)) return no();
    for (u32 i = 0; i < load<u16>(o + 120); ++i) {
        const u32 e = load<u32>(load<u32>(o + 112) + 4 * i);
        if (load<u8>(e + 16) == 0) return no();
        const u32 t = load<u32>(e + 8);
        if (t == 0) return no();
        call(ctx, 0x00522750u, {view(), sp + 112, t + 144});
        if (!OffBox(sp + 112, lo, hi, false, f1)) return no();
    }
    for (u32 i = 0; i < load<u16>(o + 152); ++i)
        if (load<u16>(load<u32>(load<u32>(o + 144) + 4 * i) + 40) != 0) return no();
    for (u32 i = 0; i < load<u16>(o + 172); ++i) {
        const u32 r = static_cast<u32>(call(ctx, 0x003EDB08u, {load<u32>(load<u32>(o + 164) + 4 * i)}));
        f1 = ctx.fpr[1];  // as the callee leaves it
        if (r == 0) return no();
    }
    ctx.fpr[1] = f1;
    ret(ctx, 1);
}

// u32 Obj_LaunchSlots43784(Obj* o, u32 k)
// (called by 00043BC0) For each of the o->316 (u16) 112-byte slots at
// o->308 that has an object (+68) and no +100 flag, with T(a) = the f32 at
// a + 72 and j counting the slots launched so far: +84 = T(0x00F95E20),
// +88 = T(0x00F95EB8) * (u32)(o->316 - k - j - 1), +0/+48 = the origin
// (o->664 + 128, or 0x01164B40 when o->664 is null), +0.x += T(0x00F95F50),
// +48.x += (T(0x00F959AC) + slot->64->4) * T(0x00F95A90); the object's
// +64..72 = +0..8, WorldObj_MarkDirty(obj), WorldObj_UpdateLod(obj, 0,
// 0), and Effect_SetHash4CF988(+80, 0x9020A4D6) when +80 is set. r3 is left
// as the last test or call leaves it (o when there are no slots).
DECOMP(00043784, Obj_LaunchSlots43784) {
    const u32 o = arg<u32>(ctx, 0), k = arg<u32>(ctx, 1);
    const u32 n = load<u16>(o + 316);
    const u32 left = n - k;
    u32 r3 = o, j = 0;
    for (u32 i = 0; i < n; ++i) {
        const u32 e = load<u32>(o + 308) + 112 * i;
        r3 = load<u32>(e + 68);
        if (r3 == 0) continue;
        r3 = load<u8>(e + 100);
        if (r3 != 0) continue;
        Cw(e + 84, 0x00F95E20u + 72);
        Sf(e + 88, S(Lf(0x00F95EB8u + 72) * S(static_cast<f64>(left - j - 1))));
        const u32 p = load<u32>(o + 664);
        const u32 src = p != 0 ? p + 128 : 0x01164B40u;
        const u32 x = load<u32>(src), y = load<u32>(src + 4), z = load<u32>(src + 8);
        store<u32>(e + 48, x);
        store<u32>(e + 52, y);
        store<u32>(e, x);
        store<u32>(e + 56, z);
        store<u32>(e + 4, y);
        store<u32>(e + 8, z);
        Sf(e, S(LfsF(x) + Lf(0x00F95F50u + 72)));
        const f64 b = S(Lf(0x00F959ACu + 72) + Lf(load<u32>(e + 64) + 4));
        Sf(e + 48, Fmadds(b, Lf(0x00F95A90u + 72), Lf(e + 48)));
        const u32 w = load<u32>(e + 68);
        CopyVec3(w + 64, e);
        ctx.fpr[1] = Lf(e + 8);
        call(ctx, 0x0057D000u, {w});                        // WorldObj_MarkDirty
        r3 = static_cast<u32>(call(ctx, 0x0057EE00u, {load<u32>(e + 68), 0, 0}));  // WorldObj_UpdateLod
        const u32 fx = load<u32>(e + 80);
        if (fx != 0) r3 = static_cast<u32>(call(ctx, 0x004CF988u, {fx, 0x9020A4D6u}));  // Effect_SetHash4CF988
        ++j;
    }
    ret(ctx, r3);
}

// u32 Obj_OnHit126E04(Obj* o, HitEvent* ev)
// (no direct callers: a handler) When Obj_TestE0BCC(o, ev) accepts the
// hit and its data d = ev->52 has (s8) +24 == 1, with b = d + (s8) d->16
// non-zero: the attacker a is ev->4 (if flagged 0x8 in +192) or, for a
// source flagged 0x20 / 0x1000 / 0x4 in +192, the flagged (0x8) owner at
// +428 / +392 / +400 (else null). Unless a is o itself, the damage is
// ev->48 * o->500 when b->232 (u8) is 4, else ev->48 *
// Comp_GetF32_922918(b + 224) * o->496; times T(0x01038940) (the f32 at +72)
// when a is set, in o's team ((+172 >> 22) & 3), o's +188 lacks 0x20 and
// a's +188 lacks 0x08000000; then 00125A80(o, 0x80F43146, ev, 0x80F43146;
// f1 = damage). r3 is left as the compiled code leaves it.
DECOMP(00126E04, Obj_OnHit126E04) {
    const u32 o = arg<u32>(ctx, 0), ev = arg<u32>(ctx, 1);
    u32 r3 = static_cast<u32>(call(ctx, 0x000E0BCCu, {o, ev}));  // Obj_TestE0BCC
    if (r3 == 0) return ret(ctx, 0);
    const u32 d = load<u32>(ev + 52);
    if (static_cast<s8>(load<u8>(d + 24)) != 1) return ret(ctx, d);
    const u32 bsrc = d + static_cast<u32>(static_cast<s32>(static_cast<s8>(load<u8>(d + 16))));
    if (bsrc == 0) return ret(ctx, 0);
    auto flagged = [](u32 q) { return q != 0 && (load<u32>(q + 192) & 0x8u) != 0; };
    const u32 src = load<u32>(ev + 4);
    u32 a = flagged(src) ? src : 0;
    if (src != 0) {
        const u32 f = load<u32>(src + 192);
        u32 link = 0;
        if (f & 0x20u) link = 428;
        else if (f & 0x1000u) link = 392;
        else if (load<u32>(src + 192) & 0x4u) link = 400;
        if (link != 0) {
            const u32 q = load<u32>(src + link);
            a = flagged(q) ? q : 0;
        }
    }
    if (a == o) return ret(ctx, bsrc);
    const u32 x = bsrc + 224;
    f64 dmg;
    if (load<u8>(x + 8) == 4) {
        dmg = S(Lf(ev + 48) * Lf(o + 500));
    } else {
        call(ctx, 0x00922918u, {x});  // Comp_GetF32_922918
        const f64 c = ctx.fpr[1];
        dmg = S(S(Lf(ev + 48) * c) * Lf(o + 496));
    }
    if (a != 0 && ((load<u32>(o + 172) >> 22) & 3) == ((load<u32>(a + 172) >> 22) & 3) && (load<u32>(o + 188) & 0x20u) == 0 &&
        (load<u32>(a + 188) & 0x08000000u) == 0)
        dmg = S(dmg * Lf(0x01038940u + 72));
    callf(ctx, 0x00125A80u, {o, 0x80F43146u, ev, 0x80F43146u}, {dmg});
}

// u32 Area_SpawnGrid2228C8(Area* o, T* a)
// (called by 00222B18, 00223890, 00223A7C) o->656..664 = the view position
// (*(game + 0x128168) + 816 + 48); then for the 3x3x3 cells (i, j, k) in
// -1..1 (slot o + 428 + 36 (i + 1) + 12 (j + 1) + 4 (k + 1)) whose slot is
// empty: slot = 000DCECC(a, d->176, &m, 1) with d = o->216 and m = the
// 0x01164BD0 rotation with the translation o->656 + {i, j, k} * d->180;
// a new effect gets +136 bit 0x200 cleared, Effect_SetTime(e;
// *(0x002228C0)) and 00222654(o, e). r3 / f1 are left as the compiled loop
// leaves them (f1 = the raw 64-bit integer j when the last cell is full).
DECOMP(002228C8, Area_SpawnGrid2228C8) {
    const u32 o = arg<u32>(ctx, 0), a = arg<u32>(ctx, 1);
    Stack frame(ctx, 304);
    const u32 sp = frame.addr();
    const u32 d = load<u32>(o + 216);
    CopyVec3(o + 656, load<u32>(load<u32>(0x00F65CA0u) + 0x128168u) + 816 + 48);
    for (u32 k = 0; k < 9; ++k) Cw(sp + 112 + MOff16(k), 0x01164BD0u + MOff16(k));
    const f64 t = Lf(0x002228C0u);
    auto ibits = [](s32 v) { return std::bit_cast<f64>(static_cast<u64>(static_cast<s64>(v))); };
    u32 r3 = static_cast<u32>(ctx.gpr[3]);
    for (s32 i = -1; i < 2; ++i) {
        ctx.fpr[1] = ibits(i);
        const f64 fi = S(static_cast<f64>(i));
        for (s32 j = -1; j < 2; ++j) {
            ctx.fpr[1] = ibits(j);
            const f64 fj = S(static_cast<f64>(j));
            for (s32 k = -1; k < 2; ++k) {
                const u32 slot = o + 428 + 36 * (i + 1) + 12 * (j + 1) + 4 * (k + 1);
                r3 = load<u32>(slot);
                if (r3 != 0) continue;
                const f64 sz = Lf(d + 180);
                const f64 x = Fmadds(fi, sz, Lf(o + 656)), y = Fmadds(fj, sz, Lf(o + 660));
                const f64 z = Fmadds(S(static_cast<f64>(k)), sz, Lf(o + 664));
                Sf(sp + 160, x);
                Sf(sp + 164, y);
                Sf(sp + 168, z);
                ctx.fpr[1] = z;
                r3 = static_cast<u32>(call(ctx, 0x000DCECCu, {a, load<u32>(d + 176), sp + 112, 1}));
                store<u32>(slot, r3);
                if (r3 == 0) continue;
                store<u32>(r3 + 136, load<u32>(r3 + 136) & ~0x200u);
                callf(ctx, 0x004CF710u, {load<u32>(slot)}, {t});  // Effect_SetTime
                r3 = static_cast<u32>(call(ctx, 0x00222654u, {o, load<u32>(slot)}));
            }
        }
    }
    ret(ctx, r3);
}

// u32 Wave_Apply4288C8(Wave* o; f32 dt)
// (called by 00429950) While o->32 is set: c = 004286D8(&c, o, o->0) (the
// centre), ramp = Math_Ramp_004A6BF0(o->4, o->8, o->12, o->36), dir =
// 0x01164B60 (or 0x01164B70 when the grid 0x010AA1B0 has +4 set), the
// phase o->24 * K0 * o->36 + o->28 wrapped to
// [-pi, pi) (K = 0x004288A0: +0 2 pi, +4 0, +8/+16 the doubles pi / -pi,
// +24 4/pi, +28 -4/pi^2, +32 0.225) through the parabolic sine; amp =
// sine * o->20 * ramp. Every point of the grid's list +480 (stride 24,
// (+84 + 1) x (+82 + 1) points, u16 counters) moves by amp *
// Falloff_004A6B98(0, o->16, |point - c|) along dir. Then o->36 += dt, and
// o->32 = 0 once o->36 > o->4 + o->8 + o->12. r3 / f1 are left as the
// compiled code leaves them.
DECOMP(004288C8, Wave_Apply4288C8) {
    const u32 o = arg<u32>(ctx, 0);
    const f64 dt = ctx.fpr[1];
    if (load<u8>(o + 32) == 0) return ret(ctx, 0);
    Stack frame(ctx, 256);
    const u32 sp = frame.addr();
    constexpr u32 W = 0x010AA1B0u, K = 0x004288A0u;
    u32 p = load<u32>(W + 480);
    const u32 nb = load<u16>(W + 84), na = load<u16>(W + 82);
    call(ctx, 0x004286D8u, {sp + 112, o, load<u32>(o)});
    const u32 dir = load<u32>(W + 4) == 0 ? 0x01164B60u : 0x01164B70u;
    const f64 dx = Lf(dir), dy = Lf(dir + 4), dz = Lf(dir + 8);
    const f64 ramp = callf(ctx, 0x004A6BF0u, {}, {Lf(o + 4), Lf(o + 8), Lf(o + 12), Lf(o + 36)});  // Math_Ramp_004A6BF0
    const f64 k0 = Lf(K);
    const f64 ph = Fmadds(S(Lf(o + 24) * k0), Lf(o + 36), Lf(o + 28));
    const f64 h = S(mid15::Fsel15(ph, Ld(K + 8), Ld(K + 16)));
    f64 x = S(h + ph);
    x = Fnmsubs(mid15::Trunc15(Divs(x, k0)), k0, x);
    x = S(x - h);
    f64 sn = Fmadds(x, Lf(K + 24), S(S(x * Lf(K + 28)) * std::fabs(x)));
    sn = Fmadds(Fmsubs(sn, std::fabs(sn), sn), Lf(K + 32), sn);
    const f64 amp = S(S(sn * Lf(o + 20)) * ramp);
    u32 r3 = K;
    const u32 cb = nb + 1, ca = na + 1;  // not masked (the u16 counters are)
    if (static_cast<s32>(cb) > 0) {
        const f64 zero = Lf(K + 4);
        for (u32 b = 0; static_cast<s32>(b) < static_cast<s32>(cb); b = (b + 1) & 0xFFFF) {
            for (u32 a = 0; static_cast<s32>(a) < static_cast<s32>(ca); a = (a + 1) & 0xFFFF) {
                const f64 py = Lf(p + 4), px = Lf(p), pz = Lf(p + 8);
                const f64 ey = S(py - Lf(sp + 116)), ex = S(px - Lf(sp + 112)), ez = S(pz - Lf(sp + 120));
                const f64 dist = Sqrts(Fmadds(ez, ez, Fmadds(ex, ex, S(ey * ey))));
                const f64 f = S(callf(ctx, 0x004A6B98u, {}, {zero, Lf(o + 16), dist}) * amp);  // Falloff_004A6B98
                Sf(p, Fmadds(f, dx, px));
                Sf(p + 4, Fmadds(f, dy, py));
                Sf(p + 8, Fmadds(f, dz, pz));
                r3 = p + 24;
                p += 24;
            }
        }
    }
    const f64 t = S(Lf(o + 36) + dt);
    const f64 total = S(S(Lf(o + 4) + Lf(o + 8)) + Lf(o + 12));
    Sf(o + 36, t);
    if (t > total) store<u8>(o + 32, 0);
    ctx.fpr[1] = total;
    ret(ctx, r3);
}

// u32 Obj_PushNearest327A58(Ctx* a, Vector* out, List* l)
// (called by 00327CB8) Pushes up to m = min(l->8 (u16 count n), a->2588)
// of the 128-byte records l->0[] into out with Vector128_Push_0075EBFC:
// the first m when n > 16, all n when n <= a->2588, else the m records
// nearest to the player (game *(0x00F65CA0) +4208 +8, position +144) by
// their +8 object's +144 position (a record without one scores
// *(0x00327A54)), kept in an insertion-sorted stack table of -dist^2
// (seeded with -FLT_MAX) and pushed farthest first. r3 / f1 are left as
// the compiled code leaves them (r3 = 0x00327A54 when nothing is pushed).
DECOMP(00327A58, Obj_PushNearest327A58) {
    const u32 a = arg<u32>(ctx, 0), out = arg<u32>(ctx, 1), l = arg<u32>(ctx, 2);
    Stack frame(ctx, 288);
    const u32 sp = frame.addr();
    constexpr u32 K = 0x00327A54u;
    const u32 n = load<u16>(l + 8);
    const u32 lim = load<u32>(a + 2588);
    const u32 m = lim < n ? lim : n;
    const u32 data = load<u32>(l);
    u32 r3 = K;
    auto push = [&](u32 r) { r3 = static_cast<u32>(call(ctx, 0x0075EBFCu, {out, r})); };  // Vector128_Push_0075EBFC
    if (n > 16) {
        for (s32 i = 0; i < static_cast<s32>(m); ++i) push(load<u32>(l) + 128 * i);
        return ret(ctx, r3);
    }
    if (n <= lim) {
        for (u32 i = 0; i < n; ++i) push(load<u32>(l) + 128 * i);
        return ret(ctx, r3);
    }
    const u32 D = sp + 112, P = sp + 176;
    for (s32 k = 0; k < static_cast<s32>(m); ++k) {
        store<u32>(D + 4 * k, 0xFF7FFFFFu);
        store<u32>(P + 4 * k, 0);
    }
    for (u32 i = 0; i < n; ++i) {
        const u32 e = data + 128 * i;
        f64 f1 = Lf(K);
        const u32 t = load<u32>(e + 8);
        if (t != 0) {
            const u32 q = t + 144;
            const u32 pl = load<u32>(load<u32>(0x00F65CA0u) + 4208 + 8) + 144;
            const f64 dy = S(Lf(pl + 4) - Lf(q + 4)), dx = S(Lf(pl) - Lf(q)), dz = S(Lf(pl + 8) - Lf(q + 8));
            f1 = Fnmadds(dz, dz, Fmadds(dx, dx, S(dy * dy)));
        }
        ctx.fpr[1] = f1;
        if (!(f1 > Lf(D))) continue;  // (with m = 0 this reads stale stack, as the original)
        s32 j = 1;
        while (j < static_cast<s32>(m)) {
            const u32 w = load<u32>(D + 4 * j);
            if (!(f1 >= LfsF(w))) break;
            const u32 pv = load<u32>(P + 4 * j);
            ++j;
            store<u32>(D + 4 * (j - 2), w);
            store<u32>(P + 4 * (j - 2), pv);
        }
        Sf(D + 4 * (j - 1), f1);
        store<u32>(P + 4 * (j - 1), e);
    }
    for (s32 k = 0; k < static_cast<s32>(m); ++k) push(load<u32>(P + 4 * k));
    ret(ctx, r3);
}

// u32 Obj_PickTurnAnim463A8(Obj* o)
// (called by 00046718) Unless Anim_TrackSet36_0029879C(o->64, 0, 1)
// returns 0 while o->32 is not 0x04B4B51A: with c = 0x00046394 (0, 1,
// 57.29, 0.25), the cosine of the angle between the facing o->160 and the
// move o->192 (both with y = c[0], normalized) is compared with
// cosf(T(0x00F954EC) * c[2]) (T(a) = the f32 at a + 72). Turned further: the
// animation is 0xDAB0BC0C when dot(o->128, move) > 0, else 0x7BBD2697;
// otherwise the state's (o->4 20 / 18 -> 0x50292BA3, 3 -> 0x82E16876,
// else 0x04B4B51A), skipped when already o->32. Started with
// Anim_Restart2775A8(o->64, anim, 0, 0, 0, -1, 0, 0, -, -, 0, -1; f1..f4 =
// 1, 0.25, 0, 0); o->32 = anim when that returns non-zero.
DECOMP(000463A8, Obj_PickTurnAnim463A8) {
    const u32 o = arg<u32>(ctx, 0);
    Stack frame(ctx, 208);
    const u32 r = static_cast<u32>(call(ctx, 0x0029879Cu, {load<u32>(o + 64), 0, 1}));  // Anim_TrackSet36_0029879C
    const u32 st = load<u32>(o + 4);
    if (r == 0 && load<u32>(o + 32) != 0x04B4B51Au) return ret(ctx, st);
    constexpr u32 C = 0x00046394u;
    const f64 vx = Lf(o + 160), vz = Lf(o + 168), mx = Lf(o + 192), mz = Lf(o + 200);
    u32 h = 0x04B4B51Au;
    if (st == 20 || st == 18) h = 0x50292BA3u;
    else if (st == 3) h = 0x82E16876u;
    const f64 c0 = Lf(C), c1 = Lf(C + 4);
    const f64 z2 = S(c0 * c0);
    const f64 la = Sqrts(Fmadds(vz, vz, Fmadds(vx, vx, z2)));
    const f64 lb = Sqrts(Fmadds(mz, mz, Fmadds(mx, mx, z2)));
    const f64 ia = Divs(c1, la), ib = Divs(c1, lb);
    const f64 ay = S(c0 * ia), ax = S(vx * ia), az = S(vz * ia);
    const f64 by = S(c0 * ib), bx = S(mx * ib), bz = S(mz * ib);
    const f64 na = Sqrts(Fmadds(az, az, Fmadds(ax, ax, S(ay * ay))));
    const f64 dot = Fmadds(az, bz, Fmadds(ax, bx, S(ay * by)));
    const f64 nb = Sqrts(Fmadds(bz, bz, Fmadds(bx, bx, S(by * by))));
    const f64 cs = Divs(dot, S(na * nb));
    const f64 lim = callf(ctx, 0x007344E0u, {}, {S(Lf(0x00F954ECu + 72) * Lf(C + 8))});  // Math_Cosf
    const u32 an = load<u32>(o + 64);
    if (cs < lim) {
        const u32 w = o + 128;
        const f64 d = Fmadds(Lf(w + 8), bz, Fmadds(Lf(w), bx, S(Lf(w + 4) * by)));
        h = d > c0 ? 0xDAB0BC0Cu : 0x7BBD2697u;
    } else if (load<u32>(o + 32) == h) {
        return ret(ctx, an);
    }
    // Arguments 8 / 9 (r1 +112 / +120) are never written; the callee reads only 10 / 11.
    callf(ctx, 0x002775A8u, {an, h, 0, 0, 0, ~0ull, 0, 0, 0, 0, 0, ~0ull}, {c1, Lf(C + 12), c0, c0});  // Anim_Restart2775A8
    if (static_cast<u32>(ctx.gpr[3]) != 0) store<u32>(o + 32, h);
}
