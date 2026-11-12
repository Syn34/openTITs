// Game code, large-function sweep part 20: a burst of delayed spawns queued
// around a point near the player (g_SpawnQueue, run by
// SpawnQueue_Update4AE498).
#include "big20.h"

namespace sly4::obj {

namespace {
constexpr u32 kK = 0x004AED00u;             // .text f32 constants (0, 1e-5, 1, 2, -3, 3, 1/65535, pi, 2pi, -pi, ...)
constexpr u32 kSpawnQueue = 0x010C2480u;    // vector of 128-byte records (g_SpawnQueue)
constexpr u32 kAimB = 0x01164C90u;          // Obj_AimMatrix's quat argument
constexpr u32 kProfA = 0x00F65F74u;         // cached Save_Profile(0xF214F06E)
constexpr u32 kProfB = 0x00F65F78u;         // cached Save_Profile(0x42361175) (Profile_Cached_F65F78)
constexpr u32 kProfC = 0x00F65F7Cu;         // cached Save_Profile(0x02D45643) (Profile_Cached_F65F7C)
constexpr u32 kCooldown = 0x00F65F80u;      // f32 cooldown of the alternate profile
constexpr u32 kWorldGrav = 0x001281D4u;     // world +: the physics settings (f32 gravity +84; +4 -> +116 id counter +48)
constexpr u32 kNav = 0x00F557ECu;           // u32 the navigation mesh handle
constexpr u32 kIdentity = 0x01164BD0u;      // f32[4][4] identity (rows of 3 copied)
constexpr u32 kPlayerStride = 0x00020C80u;
// Tweak variable records (value at +72).
constexpr u32 tDist = 0x010C2340u, tRange = 0x010C1F10u, tScaleLocal = 0x010C2430u, tScaleBits = 0x010C23E0u;
constexpr u32 tMax = 0x010C2230u, tRadius = 0x010C1F60u, tLifeA = 0x010C20A0u, tLifeB = 0x010C20F0u;
constexpr u32 tVelA = 0x010C22E0u, tVelB = 0x010C2280u, tLiftA = 0x010C2140u, tLiftB = 0x010C2390u;
constexpr u32 tAngStep = 0x010C21E0u, tRadStep = 0x010C2050u, tRadMax = 0x010C2000u, tRadMin = 0x010C1FB0u;
constexpr u32 tRatio = 0x010C1E08u, tChanceLo = 0x010C1D68u, tChanceHi = 0x010C1DB8u, tCooldown = 0x010C1E58u;
constexpr u32 Nav_SnapPoint = 0x0001087C;          // (Vec3* out, Nav* nav, const Vec3* p)
constexpr u32 Nav_Probe10980 = 0x00010980;         // (Nav*, const Vec3* p, -, -, -, Vec3* out; f1, f2, f3) -> bool (reads r3 r4 r8 f1-f3)
constexpr u32 Save_Profile = 0x000C9FC8;           // (u32 hash) -> Profile*
constexpr u32 Obj_FindByKey = 0x000D1454;          // (Obj*, u32 hash) -> WorldObj*
constexpr u32 Obj_IndexByHash = 0x000D14DC;        // (Obj*, u32 hash) -> s32 or -1
constexpr u32 Map_GetPair = 0x000F816C;            // (map, u32* a, u32* b)
constexpr u32 SaveSlot_Bits2Is1 = 0x00465E60;      // (slot, u32 k) -> bool
constexpr u32 Obj_AimMatrix = 0x0049FFE0;          // (Obj*, f32 out[4][4], att, s32 bone, wo, pos, aim, quat)
constexpr u32 Obj_IsLocalPlayerSlot = 0x004AEBE0;  // (Obj*) -> bool
constexpr u32 Matrix_NormalizeInPlace = 0x0056D374;// (Mat34*)
constexpr u32 Rec80_Find = 0x005848CC;             // (list, u32 hash) -> rec or 0
constexpr u32 Math_Cosf = 0x007344E0;              // f1 -> f1
constexpr u32 Math_Sinf = 0x007345F0;              // f1 -> f1
constexpr u32 Vec128_Push = 0x00788A00;            // (Vec*, const Rec128*)

struct SpawnReq {
    pad<16> _0;
    be_u32 src;         // +16: the source object
    be_u32 slot;        // +20: the player slot
    be_u32 hash;        // +24: an attachment hash
    be_u32 map;         // +28
    be_u32 lo;          // +32
    be_u32 hi;          // +36
    be_f32 scale;       // +40
    u8 alt44, alt45, unsnapped, flag47;  // +44 .. +47
};
static_assert(offsetof(SpawnReq, src) == 16 && offsetof(SpawnReq, scale) == 40 && offsetof(SpawnReq, flag47) == 47);
// The player object: +144 its position, +172 / +192 flags, +344 / +348.
struct BurstPlayer {
    pad<144> _0;
    Vec3 pos;           // +144
    pad<16> _156;
    be_u32 f172;        // +172
    pad<16> _176;
    be_u32 f192;        // +192
    pad<148> _196;
    be_f32 f344;        // +344
    be_f32 f348;        // +348
};
static_assert(offsetof(BurstPlayer, f172) == 172 && offsetof(BurstPlayer, f192) == 192 && offsetof(BurstPlayer, f344) == 344);
// One 128-byte queue record.
struct BurstRec {
    Mtx44 m;            // +0
    Vec4 off;           // +64
    Vec4 vel;           // +80
    be_u32 prof;        // +96
    be_u32 id;          // +100
    be_u32 src;         // +104
    be_f32 delay;       // +108
    u8 flag;            // +112
    pad<15> _113;
};
static_assert(sizeof(BurstRec) == 128);
// The original's locals (sp offsets - 112).
struct BurstLocals {
    Vec4 from;          // sp+112: the source position
    Vec4 target;        // sp+128
    Vec4 probe;         // sp+144
    Vec4 snap;          // sp+160
    Vec4 pt;            // sp+176
    Vec4 pt2;           // sp+192
    Mtx44 aim;          // sp+208
    BurstRec rec;       // sp+272
    pad<56> _400;
    be_u32 range[2];    // sp+456 / sp+460
    be_f32 roots[2];    // sp+464 / sp+468
};
static_assert(offsetof(BurstLocals, aim) == 96 && offsetof(BurstLocals, rec) == 160 && offsetof(BurstLocals, range) == 344);

f64 K(u32 off) { return lf(global<be_f32>(kK + off)); }
f64 Tw(u32 rec) { return lf(gm<be_f32>(rec + 72)); }
u32 Fw(f64 x) { return std::bit_cast<u32>(static_cast<f32>(x)); }

// The 64-bit truncation (fctidz) of max(x, lo) (fsel on lo - x), low word.
u32 TruncMax(f64 x, f64 lo) { return static_cast<u32>(Fctidz(Fsel(S(lo - x), lo, x))); }

// The player record of slot s: *(world +4208 + s * 0x20C80) is its entry.
u32 SlotEntry(u32 s) { return global<be_u32>(kWorld) + s * kPlayerStride + 4208u; }

// Cached profile `cache`, made from Save_Profile(hash) when 0.
u32 CachedProfile(u32 cache, u32 hash) {
    u32 v = global<be_u32>(cache);
    if (v == 0) {
        v = call<u32>(Save_Profile, hash);
        global<be_u32>(cache) = v;
    }
    return v;
}
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
// gated by a random cooldown when p +45 is clear. No caller reads r3.
void SpawnQueue_Burst4AED50(SpawnReq* p) {
    const u32 src = p->src;
    if (src == 0) return;
    const u32 slot = SlotEntry(p->slot);
    {
        const u32 pl = gm<be_u32>(slot + 8);
        if (pl == 0 || (gm<BurstPlayer>(pl).f172 & 0x40000u) == 0) return;
    }
    GuestFrame frame(896);
    BurstLocals& L = *frame.as<BurstLocals>(112);
    // Aim matrix: identity rows (3 floats each; the fourth words are left
    // as the stack had them), then Obj_AimMatrix.
    CopyMtxXYZ(L.aim, global<Mtx44>(kIdentity));
    const u32 hash = p->hash;
    if (gm<be_u32>(src + 172) & 0x40000u) {
        u64 rec = 0, bone = ~0ull, wo = 0;
        if (hash != 0) {
            const u32 c = gm<be_u32>(src + 160);
            const u32 d = c != 0 ? gm<be_u32>(c + 208).get() : 0u;
            const u32 e = d != 0 ? gm<be_u32>(d + 108).get() : 0u;
            if (e != 0) rec = call<u64>(Rec80_Find, e, hash);
            if (static_cast<u32>(rec) == 0) {
                bone = call<u64>(Obj_IndexByHash, src, hash);
                if (static_cast<u32>(bone) == 0xFFFFFFFFu) wo = call<u64>(Obj_FindByKey, src, hash);
            }
        }
        call(Obj_AimMatrix, src, &L.aim, rec, bone, wo, p, kZeroVec, kAimB);
    }
    call(Matrix_NormalizeInPlace, &L.aim);
    // Source position (matrix row 3) and the target toward the player.
    CopyXYZ(L.from, L.aim.r[3]);
    const f64 k0 = K(0);
    {
        const BurstPlayer& pl = gm<BurstPlayer>(gm<be_u32>(slot + 8));
        const f64 px = lf(pl.pos.x), py = lf(pl.pos.y), pz = lf(pl.pos.z);
        f64 dx = S(px - lf(L.aim.r[3].x)), dy = k0, dz = S(pz - lf(L.aim.r[3].z));
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
        L.target.x = static_cast<f32>(Fmadds(dx, d, px));
        L.target.y = static_cast<f32>(Fmadds(dy, d, py));
        L.target.z = static_cast<f32>(Fmadds(dz, d, pz));
    }
    CopyXYZ(L.probe, global<Vec3>(kZeroVec));
    {
        const u32 nav = global<be_u32>(kNav);
        // r5 = nav, r6 / r7 not read (the first two floats' slots).
        if (call<u32>(Nav_Probe10980, nav, &L.from, nav, K(12), K(16), &L.probe, K(20)) == 0) {
            call(Nav_SnapPoint, &L.snap, global<be_u32>(kNav).get(), &L.target);
            CopyXYZ(L.probe, L.snap);
        }
    }
    {
        const f64 dx = S(lf(L.from.x) - lf(L.probe.x));
        const f64 dy = S(lf(L.from.y) - lf(L.probe.y));
        const f64 dz = S(lf(L.from.z) - lf(L.probe.z));
        f64 r = Tw(tRange);
        r = S(r * r);
        f64 l2 = Fmadds(dx, dx, S(dy * dy));
        l2 = Fmadds(dz, dz, l2);
        if (l2 > r) return;
    }
    const f64 one = K(8);  // f24
    f64 scale = one;       // f31
    u32 prof = CachedProfile(kProfA, 0xF214F06Eu);
    if (call<u32>(Obj_IsLocalPlayerSlot, gm<be_u32>(slot + 8).get()) != 0) {
        prof = CachedProfile(kProfB, 0x42361175u);
        scale = Tw(tScaleLocal);
    } else {
        const f64 s = lf(p->scale);
        if (s > scale) scale = s;
        else if (call<u32>(SaveSlot_Bits2Is1, slot, 1u) != 0) scale = Tw(tScaleBits);
    }
    // The id: source vfunc 100; 0 / -1 -> the next world counter value (when
    // there is a counter) handed to vfunc 104.
    u32 id;
    {
        id = vcall<u32>(&gm<u8>(p->src), 100);
        const u32 w = global<be_u32>(kWorld);
        const u32 o2 = p->src;
        if (id == 0 || id == 0xFFFFFFFFu) {
            const u32 t = gm<be_u32>(gm<be_u32>(w + kWorldGrav) + 4);
            if (t != 0) {
                const u32 q = gm<be_u32>(t + 116);
                id = gm<be_u32>(q + 48) + 1;
                gm<be_u32>(q + 48) = id;
            }
            vcall(&gm<u8>(o2), 104, id);
        }
    }
    const u32 profAlt = CachedProfile(kProfC, 0x02D45643u);
    // Count range.
    L.range[0] = p->lo.get();
    L.range[1] = p->hi.get();
    if (p->map != 0u) call(Map_GetPair, p->map.get(), &L.range[0], &L.range[1]);
    u32 lo = L.range[0], hi = L.range[1];
    if (hi > 1u && scale > one) {
        const u32 a = lo + TruncMax(S(S(static_cast<f64>(static_cast<s32>(lo))) * scale), one);
        L.range[0] = a;
        const u32 b = hi + TruncMax(S(S(static_cast<f64>(static_cast<s32>(hi))) * scale), one);
        L.range[1] = b;
        lo = a;
        hi = b;
    }
    u32 count = 0;  // r20
    if (prof != 0) {
        // lo + seed % (hi - lo + 1) (divwu by 0 gives 0 here), then a step.
        const u32 s = global<be_u32>(kGameRandState);
        const u32 n = hi - lo, d = n + 1;
        const u32 q = d != 0 ? s / d : 0;
        count = lo + (s - (q * n + q));
        GameRng g;
        g.seed = s;
        g.Step();
        g.Commit();
    }
    const bool alt = p->alt44 != 0 || p->alt45 != 0;  // r19
    // Loop state.
    GameRng g;
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
    BurstRec& rc = L.rec;
    for (s32 i = 0;; ++i) {
        if (!(i < limit) || !(i < gm<be_s32>(tMax + 72))) return;
        const bool useAlt = alt && !(static_cast<s32>(made) < static_cast<s32>(count));  // cror eq | cr1.lt
        const u32 pr = useAlt ? profAlt : prof;
        if (pr == 0) continue;
        if (useAlt && p->alt45 == 0) {
            // Random cooldown for the alternate profile (the player's
            // health-like ratio +348 / +344 picks the chance).
            if (lf(global<be_f32>(kCooldown)) > k0) return;
            const u32 pl = gm<be_u32>(SlotEntry(p->slot) + 8);
            if (pl == 0 || (gm<BurstPlayer>(pl).f192 & 2u) == 0) return;
            const f64 m = lf(gm<BurstPlayer>(pl).f344);
            f64 ratio = one;
            if (m > k0) ratio = S(lf(gm<BurstPlayer>(pl).f348) / m);
            if (ratio > one || ratio == one) return;
            const f64 a = Tw(tRatio);
            const f64 thr = (ratio < a || ratio == a) ? Tw(tChanceLo) : Tw(tChanceHi);
            const f64 r = S(g.Next() * K(24));
            g.Commit();
            if (!(r < thr || r == thr)) return;
            global<be_u32>(kCooldown) = gm<be_u32>(tCooldown + 72).get();
        }
        // The point: on the circle around the target, snapped to the mesh,
        // or the unsnapped target with p +46.
        u32 w10, w11, w12;
        if (p->unsnapped == 0) {
            CopyXYZ(L.pt, global<Vec3>(kZeroVec));
            const f64 cx = S(rad * call<f64>(Math_Cosf, ang0));
            const f64 cz = S(rad * call<f64>(Math_Sinf, ang1));
            const f64 tx = S(lf(L.probe.x) + cx), ty = S(lf(L.probe.y) + k0), tz = S(lf(L.probe.z) + cz);
            L.pt2.x = static_cast<f32>(cx);
            set_bits(L.pt2.y, 0u);
            L.pt2.z = static_cast<f32>(cz);
            L.pt.x = static_cast<f32>(tx);
            L.pt.y = static_cast<f32>(ty);
            L.pt.z = static_cast<f32>(tz);
            call(Nav_SnapPoint, &L.pt2, global<be_u32>(kNav).get(), &L.pt);
            w10 = bits(L.pt2.x);
            w11 = bits(L.pt2.y);
            w12 = bits(L.pt2.z);
            g.Load();
        } else {
            w10 = bits(L.target.x);
            w11 = bits(L.target.y);
            w12 = bits(L.target.z);
        }
        f64 f10 = LfsW(w10), f11 = LfsW(w11), f12 = LfsW(w12);
        // The record.
        CopyMtxXYZ(rc.m, L.aim);
        CopyXYZ(rc.off, global<Vec3>(kZeroVec));
        CopyXYZ(rc.vel, global<Vec3>(kZeroVec));
        {
            const f64 r = S(g.Next() * K(24));
            const f64 la = Tw(tLifeA), lb = Tw(tLifeB);
            rc.prof = pr;
            rc.id = id;
            rc.src = p->src.get();
            g.Commit();
            rc.delay = static_cast<f32>(Fmadds(S(lb - la), r, la));
        }
        const f64 va0 = lf(gm<be_f32>(tVelA + 80)), vb0 = lf(gm<be_f32>(tVelB + 80));
        const f64 va1 = lf(gm<be_f32>(tVelA + 84)), vb1 = lf(gm<be_f32>(tVelB + 84));
        const f64 va2 = lf(gm<be_f32>(tVelA + 88)), vb2 = lf(gm<be_f32>(tVelB + 88));
        rc.flag = (p->unsnapped == 0 ? p->flag47 : 1u) != 0 ? 1 : 0;
        if (gm<be_u32>(pr + 20) & 0x20u) {
            // Ballistic: a random launch offset in [-va, va] + vb, and the
            // velocity reaching the point under the world gravity.
            const f64 ra = g.Next(), rb = g.Next(), rcz = g.Next();
            const f64 n0 = -va0, n1 = -va1, n2 = -va2;
            const f64 s0 = S(va0 - n0), s1 = S(va1 - n1), s2 = S(va2 - n2);
            const f64 ox = S(vb0 + Fmadds(s0, S(ra * K(24)), n0));
            const f64 oy = S(vb1 + Fmadds(s1, S(rb * K(24)), n1));
            const f64 dy = S(f11 - lf(L.from.y));
            g.Commit();
            const u32 phys = gm<be_u32>(global<be_u32>(kWorld) + kWorldGrav);
            const f64 oz = S(vb2 + Fmadds(s2, S(rcz * K(24)), n2));
            rc.off.x = static_cast<f32>(ox);
            rc.off.y = static_cast<f32>(oy);
            rc.off.z = static_cast<f32>(oz);
            f64 h = S(dy + Tw(tLiftA));
            const f64 gk = Tw(tLiftB);
            const f64 grav = lf(gm<be_f32>(phys + 84));
            h = S(h * c40);
            f64 gg = S(grav * gk);
            h = S(h * gg);
            gg = S(gg * c44);
            const f64 v2 = Fsel(S(k0 - h), k0, h);
            const f64 ndy = -dy;
            const f64 g2 = S(gg * c48);
            f10 = S(f10 - lf(L.from.x));
            const f64 vy = S(std::sqrt(v2));
            const f64 t = S(g2 * ndy);
            f12 = S(f12 - lf(L.from.z));
            const f64 disc = Fmsubs(vy, vy, t);
            rc.vel.y = static_cast<f32>(vy);
            if (!(disc < k0)) {
                const f64 sd = S(std::sqrt(disc));
                const f64 den = S(gg * two);
                const f64 a = S(vy / den);
                const f64 b = S(sd / den);
                u32 idx;
                if (std::fabs(b) < c52) {
                    L.roots[0] = static_cast<f32>(-a);
                    idx = 0;
                } else {
                    const f64 t1 = S(b - a), t2 = S(-a - b);
                    L.roots[0] = static_cast<f32>(t1);
                    L.roots[1] = static_cast<f32>(t2);
                    if (t1 > t2) {
                        L.roots[0] = static_cast<f32>(t2);
                        L.roots[1] = static_cast<f32>(t1);
                    }
                    idx = 1;
                }
                const f64 tt = lf(L.roots[idx]);  // one root: +464, two: the larger (+468)
                rc.vel.x = static_cast<f32>(S(f10 / tt));
                rc.vel.z = static_cast<f32>(S(f12 / tt));
            }
        }
        call(Vec128_Push, kSpawnQueue, &rc);
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

}  // namespace sly4::obj

NATIVE(004AED50, sly4::obj::SpawnQueue_Burst4AED50)
