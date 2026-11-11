// Game code, large-function sweep part 20: a burst of delayed spawns queued
// around a point near the player (g_SpawnQueue, run by
// SpawnQueue_Update4AE498).
#include "big20.h"

using namespace sly4;
using namespace sly4::big20;

namespace {
namespace sq {
constexpr u32 kK = 0x004AED00u;             // .text f32 constants (0, 1e-5, 1, 2, -3, 3, 1/65535, pi, 2pi, -pi, ...)
constexpr u32 kSpawnQueue = 0x010C2480u;    // vector of 128-byte records (g_SpawnQueue)
constexpr u32 kAimB = 0x01164C90u;          // Obj_AimMatrix's quat argument
constexpr u32 kProfA = 0x00F65F74u;         // cached Save_Profile(0xF214F06E)
constexpr u32 kProfB = 0x00F65F78u;         // cached Save_Profile(0x42361175) (Profile_Cached_F65F78)
constexpr u32 kProfC = 0x00F65F7Cu;         // cached Save_Profile(0x02D45643) (Profile_Cached_F65F7C)
constexpr u32 kCooldown = 0x00F65F80u;      // f32 cooldown of the alternate profile
constexpr u32 kWorldGrav = 0x001281D4u;     // world +: the physics settings (f32 gravity +84; +4 -> +116 id counter +48)
// Tweak variable records (value at +72).
constexpr u32 tDist = 0x010C2340u, tRange = 0x010C1F10u, tScaleLocal = 0x010C2430u, tScaleBits = 0x010C23E0u;
constexpr u32 tMax = 0x010C2230u, tRadius = 0x010C1F60u, tLifeA = 0x010C20A0u, tLifeB = 0x010C20F0u;
constexpr u32 tVelA = 0x010C22E0u, tVelB = 0x010C2280u, tLiftA = 0x010C2140u, tLiftB = 0x010C2390u;
constexpr u32 tAngStep = 0x010C21E0u, tRadStep = 0x010C2050u, tRadMax = 0x010C2000u, tRadMin = 0x010C1FB0u;
constexpr u32 tRatio = 0x010C1E08u, tChanceLo = 0x010C1D68u, tChanceHi = 0x010C1DB8u, tCooldown = 0x010C1E58u;

inline f64 K(u32 off) { return Lf(kK + off); }

// The 64-bit truncation (fctidz) of max(x, lo) (fsel on lo - x), low word.
inline u32 TruncMax(f64 x, f64 lo) {
    const f64 v = mid15::Fsel15(S(lo - x), lo, x);
    return static_cast<u32>(rt::fctid(v, true));
}

// The player record of slot s: *(world +4208 + s * 0x20C80) is its entry.
inline u32 SlotEntry(u32 s) { return load<u32>(kWorld) + s * kPlayerStride + 4208u; }

// Cached profile `cache`, made from Save_Profile(hash) when 0.
inline u32 CachedProfile(PPUContext& ctx, u32 cache, u32 hash) {
    u32 v = load<u32>(cache);
    if (v == 0) {
        v = static_cast<u32>(call(ctx, b20fn::Save_Profile, {hash}));
        store<u32>(cache, v);
    }
    return v;
}
}  // namespace sq
}  // namespace

// void f(SpawnReq* p): p +16 the source object (none: nothing), p +20 the
// player slot (its object needs bit 0x40000 of +172), p +24 an attachment
// hash, p +28 / +32 / +36 the count range (Map_GetPair when +28 is set),
// f32 p +40 a count scale, u8 p +44 / +45 / +46 / +47 flags. The aim matrix
// of the source (Obj_AimMatrix when it has bit 0x40000) is normalized; the
// target is tDist past the source position toward the player (y kept),
// probed on the navigation mesh (snapped when the probe fails); nothing
// when it lies beyond tRange. The profile (a cached one; the local player's
// own, else scaled by p +40 or tScaleBits) and an id (source vfunc 100, a
// fresh one from the world counter via vfunc 104 when 0 / -1) are picked,
// the count drawn from the range (scaled when the scale is above 1), and up
// to min(count, tMax) 128-byte records pushed onto g_SpawnQueue: matrix,
// launch offset, velocity (ballistic toward the target when profile +20
// has bit 0x20), profile, id, source, a random delay and a flag. The points
// walk a circle (angles f28 / f29, radius f30) snapped to the mesh unless
// p +46; with p +44 / +45 one record per count uses the alternate profile,
// gated by a random cooldown when p +45 is clear.
DECOMP(004AED50, SpawnQueue_Burst4AED50) {
    using namespace sq;
    using namespace b20fn;
    const u32 p = arg<u32>(ctx, 0);
    const u32 src = load<u32>(p + 16);
    if (src == 0) return;
    const u32 slot = SlotEntry(load<u32>(p + 20));
    {
        const u32 pl = load<u32>(slot + 8);
        if (pl == 0 || (load<u32>(pl + 172) & 0x40000u) == 0) return;
    }
    Stack frame(ctx, 896);
    const u32 sp = frame.addr();
    // Aim matrix: identity rows (3 floats each; the fourth words are left
    // as the stack had them), then Obj_AimMatrix.
    for (u32 r = 0; r < 4; ++r)
        for (u32 c = 0; c < 3; ++c) Cw(sp + 208 + 16 * r + 4 * c, kIdentity + 16 * r + 4 * c);
    const u32 hash = load<u32>(p + 24);
    if (load<u32>(src + 172) & 0x40000u) {
        u64 rec = 0, bone = ~0ull, wo = 0;
        if (hash != 0) {
            const u32 c = load<u32>(src + 160);
            const u32 d = c != 0 ? load<u32>(c + 208) : 0;
            const u32 e = d != 0 ? load<u32>(d + 108) : 0;
            if (e != 0) rec = call(ctx, Rec80_Find, {e, hash});
            if (static_cast<u32>(rec) == 0) {
                bone = call(ctx, Obj_IndexByHash, {src, hash});
                if (static_cast<u32>(bone) == 0xFFFFFFFFu) wo = call(ctx, Obj_FindByKey, {src, hash});
            }
        }
        call(ctx, Obj_AimMatrix, {src, sp + 208, rec, bone, wo, p, kZeroVec, kAimB});
    }
    call(ctx, Matrix_NormalizeInPlace, {sp + 208});
    // Source position (matrix row 3) and the target toward the player.
    Cw(sp + 112, sp + 256);
    Cw(sp + 116, sp + 260);
    Cw(sp + 120, sp + 264);
    const f64 k0 = K(0);
    {
        const u32 pl = load<u32>(slot + 8);
        const f64 px = Lf(pl + 144), py = Lf(pl + 148), pz = Lf(pl + 152);
        f64 dx = S(px - Lf(sp + 256)), dy = k0, dz = S(pz - Lf(sp + 264));
        f64 l2 = S(dy * dy);
        l2 = Fmadds(dx, dx, l2);
        l2 = Fmadds(dz, dz, l2);
        if (l2 > K(4)) {
            const f64 inv = S(K(8) / S(std::sqrt(l2)));
            dx = S(dx * inv);
            dy = S(dy * inv);
            dz = S(dz * inv);
        }
        const f64 d = Tw(tDist);
        store<u32>(sp + 128, Fb(Fmadds(dx, d, px)));
        store<u32>(sp + 132, Fb(Fmadds(dy, d, py)));
        store<u32>(sp + 136, Fb(Fmadds(dz, d, pz)));
    }
    Cw(sp + 144, kZeroVec);
    Cw(sp + 148, kZeroVec + 4);
    Cw(sp + 152, kZeroVec + 8);
    {
        const u32 nav = load<u32>(kNav);
        // r5 = nav and r6 / r7 as the previous call left them (not read).
        callf(ctx, Nav_Probe10980, {nav, sp + 112, nav, ctx.gpr[6], ctx.gpr[7], sp + 144}, {K(12), K(16), K(20)});
        if (static_cast<u32>(ctx.gpr[3]) == 0) {
            call(ctx, Nav_SnapPoint, {sp + 160, load<u32>(kNav), sp + 128});
            Cw(sp + 144, sp + 160);
            Cw(sp + 148, sp + 164);
            Cw(sp + 152, sp + 168);
        }
    }
    {
        const f64 dx = S(Lf(sp + 112) - Lf(sp + 144));
        const f64 dy = S(Lf(sp + 116) - Lf(sp + 148));
        const f64 dz = S(Lf(sp + 120) - Lf(sp + 152));
        f64 r = Tw(tRange);
        r = S(r * r);
        f64 l2 = Fmadds(dx, dx, S(dy * dy));
        l2 = Fmadds(dz, dz, l2);
        if (l2 > r) return;
    }
    const f64 one = K(8);  // f24
    f64 scale = one;       // f31
    u32 prof = CachedProfile(ctx, kProfA, 0xF214F06Eu);
    if (static_cast<u32>(call(ctx, Obj_IsLocalPlayerSlot, {load<u32>(slot + 8)})) != 0) {
        prof = CachedProfile(ctx, kProfB, 0x42361175u);
        scale = Tw(tScaleLocal);
    } else {
        const f64 s = Lf(p + 40);
        if (s > scale) scale = s;
        else if (static_cast<u32>(call(ctx, SaveSlot_Bits2Is1, {slot, 1})) != 0) scale = Tw(tScaleBits);
    }
    // The id: source vfunc 100; 0 / -1 -> the next world counter value (when
    // there is a counter) handed to vfunc 104.
    u32 id;
    {
        const u32 o = load<u32>(p + 16);
        id = static_cast<u32>(VCall(ctx, o, 100, {o}));
        const u32 w = load<u32>(kWorld);
        const u32 o2 = load<u32>(p + 16);
        if (id == 0 || id == 0xFFFFFFFFu) {
            const u32 t = load<u32>(load<u32>(w + kWorldGrav) + 4);
            if (t != 0) {
                const u32 q = load<u32>(t + 116);
                id = load<u32>(q + 48) + 1;
                store<u32>(q + 48, id);
            }
            VCall(ctx, o2, 104, {o2, id});
        }
    }
    const u32 profAlt = CachedProfile(ctx, kProfC, 0x02D45643u);
    // Count range.
    store<u32>(sp + 456, load<u32>(p + 32));
    store<u32>(sp + 460, load<u32>(p + 36));
    if (load<u32>(p + 28) != 0) call(ctx, Map_GetPair, {load<u32>(p + 28), sp + 456, sp + 460});
    u32 lo = load<u32>(sp + 456), hi = load<u32>(sp + 460);
    if (hi > 1u && scale > one) {
        const u32 a = lo + TruncMax(S(S(static_cast<f64>(static_cast<s32>(lo))) * scale), one);
        store<u32>(sp + 456, a);
        const u32 b = hi + TruncMax(S(S(static_cast<f64>(static_cast<s32>(hi))) * scale), one);
        store<u32>(sp + 460, b);
        lo = a;
        hi = b;
    }
    u32 count = 0;  // r20
    if (prof != 0) {
        // lo + seed % (hi - lo + 1) (divwu by 0 gives 0 here), then a step.
        const u32 s = load<u32>(kRand + 8);
        const u32 n = hi - lo, d = n + 1;
        const u32 q = d != 0 ? s / d : 0;
        count = lo + (s - (q * n + q));
        Rng g;
        g.seed = s;
        g.Step();
        g.Commit();
    }
    const bool alt = load<u8>(p + 44) != 0 || load<u8>(p + 45) != 0;  // r19
    // Loop state.
    Rng g;
    g.Load();
    f64 ang0 = S(S(g.Next() * K(24)) * K(28));             // f28
    f64 ang1 = Fmadds(S(g.Next() * K(24)), K(32), K(36));  // f29
    g.Commit();
    const f64 pi = K(28), twoPi = K(32);
    const f64 c40 = K(40), c44 = K(44), c48 = K(48), c52 = K(52);
    const f64 two = K(12);  // f18
    f64 rad = Tw(tRadius);  // f30
    const s32 limit = static_cast<s32>(count + (alt ? 1u : 0u));
    u32 made = 0;  // r24
    for (s32 i = 0;; ++i) {
        if (!(i < limit) || !(i < static_cast<s32>(load<u32>(tMax + 72)))) return;
        const bool useAlt = alt && !(static_cast<s32>(made) < static_cast<s32>(count));  // cror eq | cr1.lt
        const u32 pr = useAlt ? profAlt : prof;
        if (pr == 0) continue;
        if (useAlt && load<u8>(p + 45) == 0) {
            // Random cooldown for the alternate profile (the player's
            // health-like ratio +348 / +344 picks the chance).
            if (Lf(kCooldown) > k0) return;
            const u32 pl = load<u32>(SlotEntry(load<u32>(p + 20)) + 8);
            if (pl == 0 || (load<u32>(pl + 192) & 2u) == 0) return;
            const f64 m = Lf(pl + 344);
            f64 ratio = one;
            if (m > k0) ratio = S(Lf(pl + 348) / m);
            if (ratio > one || ratio == one) return;
            const f64 a = Tw(tRatio);
            const f64 thr = (ratio < a || ratio == a) ? Tw(tChanceLo) : Tw(tChanceHi);
            const f64 r = S(g.Next() * K(24));
            g.Commit();
            if (!(r < thr || r == thr)) return;
            Cw(kCooldown, tCooldown + 72);
        }
        // The point: on the circle around the target, snapped to the mesh,
        // or the unsnapped target with p +46.
        u32 w10, w11, w12;
        if (load<u8>(p + 46) == 0) {
            Cw(sp + 176, kZeroVec);
            Cw(sp + 180, kZeroVec + 4);
            Cw(sp + 184, kZeroVec + 8);
            const f64 cx = S(rad * callf(ctx, Math_Cosf, {}, {ang0}));
            const f64 cz = S(rad * callf(ctx, Math_Sinf, {}, {ang1}));
            const f64 tx = S(Lf(sp + 144) + cx), ty = S(Lf(sp + 148) + k0), tz = S(Lf(sp + 152) + cz);
            store<u32>(sp + 192, Fb(cx));
            store<u32>(sp + 196, 0);
            store<u32>(sp + 200, Fb(cz));
            store<u32>(sp + 176, Fb(tx));
            store<u32>(sp + 180, Fb(ty));
            store<u32>(sp + 184, Fb(tz));
            call(ctx, Nav_SnapPoint, {sp + 192, load<u32>(kNav), sp + 176});
            w10 = load<u32>(sp + 192);
            w11 = load<u32>(sp + 196);
            w12 = load<u32>(sp + 200);
            g.Load();
        } else {
            w10 = load<u32>(sp + 128);
            w11 = load<u32>(sp + 132);
            w12 = load<u32>(sp + 136);
        }
        f64 f10 = mid13::LfsF(w10), f11 = mid13::LfsF(w11), f12 = mid13::LfsF(w12);
        // The record at sp +272.
        const u32 rc = sp + 272;
        for (u32 r = 0; r < 4; ++r)
            for (u32 c = 0; c < 3; ++c) Cw(rc + 16 * r + 4 * c, sp + 208 + 16 * r + 4 * c);
        for (u32 c = 0; c < 3; ++c) {
            Cw(rc + 64 + 4 * c, kZeroVec + 4 * c);
            Cw(rc + 80 + 4 * c, kZeroVec + 4 * c);
        }
        {
            const f64 r = S(g.Next() * K(24));
            const f64 la = Tw(tLifeA), lb = Tw(tLifeB);
            store<u32>(rc + 96, pr);
            store<u32>(rc + 100, id);
            store<u32>(rc + 104, load<u32>(p + 16));
            g.Commit();
            store<u32>(rc + 108, Fb(Fmadds(S(lb - la), r, la)));
        }
        const f64 va0 = Lf(tVelA + 80), vb0 = Lf(tVelB + 80);
        const f64 va1 = Lf(tVelA + 84), vb1 = Lf(tVelB + 84);
        const f64 va2 = Lf(tVelA + 88), vb2 = Lf(tVelB + 88);
        store<u8>(rc + 112, (load<u8>(p + 46) == 0 ? load<u8>(p + 47) : 1u) != 0 ? 1 : 0);
        if (load<u32>(pr + 20) & 0x20u) {
            // Ballistic: a random launch offset in [-va, va] + vb, and the
            // velocity reaching the point under the world gravity.
            const f64 ra = g.Next(), rb = g.Next(), rcz = g.Next();
            const f64 n0 = -va0, n1 = -va1, n2 = -va2;
            const f64 s0 = S(va0 - n0), s1 = S(va1 - n1), s2 = S(va2 - n2);
            const f64 ox = S(vb0 + Fmadds(s0, S(ra * K(24)), n0));
            const f64 oy = S(vb1 + Fmadds(s1, S(rb * K(24)), n1));
            const f64 dy = S(f11 - Lf(sp + 116));
            g.Commit();
            const u32 phys = load<u32>(load<u32>(kWorld) + kWorldGrav);
            const f64 oz = S(vb2 + Fmadds(s2, S(rcz * K(24)), n2));
            store<u32>(rc + 64, Fb(ox));
            store<u32>(rc + 68, Fb(oy));
            store<u32>(rc + 72, Fb(oz));
            f64 h = S(dy + Tw(tLiftA));
            const f64 gk = Tw(tLiftB);
            const f64 grav = Lf(phys + 84);
            h = S(h * c40);
            f64 gg = S(grav * gk);
            h = S(h * gg);
            gg = S(gg * c44);
            const f64 v2 = mid15::Fsel15(S(k0 - h), k0, h);
            const f64 ndy = -dy;
            const f64 g2 = S(gg * c48);
            f10 = S(f10 - Lf(sp + 112));
            const f64 vy = S(std::sqrt(v2));
            const f64 t = S(g2 * ndy);
            f12 = S(f12 - Lf(sp + 120));
            const f64 disc = Fmsubs(vy, vy, t);
            store<u32>(rc + 84, Fb(vy));
            if (!(disc < k0)) {
                const f64 sd = S(std::sqrt(disc));
                const f64 den = S(gg * two);
                const f64 a = S(vy / den);
                const f64 b = S(sd / den);
                u32 idx;
                if (std::fabs(b) < c52) {
                    store<u32>(sp + 464, Fb(-a));
                    idx = 1;
                } else {
                    const f64 t1 = S(b - a), t2 = S(-a - b);
                    store<u32>(sp + 464, Fb(t1));
                    store<u32>(sp + 468, Fb(t2));
                    if (t1 > t2) {
                        store<u32>(sp + 464, Fb(t2));
                        store<u32>(sp + 468, Fb(t1));
                    }
                    idx = 2;
                }
                const f64 tt = Lf(sp + 460 + 4 * idx);  // one root: +464, two: the larger (+468)
                store<u32>(rc + 80, Fb(S(f10 / tt)));
                store<u32>(rc + 88, Fb(S(f12 / tt)));
            }
        }
        call(ctx, Vec128_Push, {kSpawnQueue, rc});
        const f64 dAng = Tw(tAngStep), dRad = Tw(tRadStep), rMax = Tw(tRadMax), rMin = Tw(tRadMin);
        g.Load();
        if (!useAlt) ++made;
        ang0 = S(ang0 + dAng);
        if (ang0 > pi) ang0 = S(ang0 - pi);
        ang1 = S(ang1 + dAng);
        if (ang1 > pi) ang1 = S(ang1 - twoPi);  // compared with pi, reduced by 2pi (as compiled)
        rad = S(rad + dRad);
        if (rad > rMax) rad = S(rad - S(rMax - rMin));
    }
}
