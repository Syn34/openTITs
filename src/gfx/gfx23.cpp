// The renderer range's next recompiled functions (gfx23): the last undone
// ones of 0x004FB000 - 0x005E0000. Names carry the address where the role
// is unclear.
#include <bit>
#include <cmath>

#include "fx.h"
#include "gfx23.h"

using namespace sly4;

namespace {

namespace g23 = sly4::render::gfx23;
namespace quad = sly4::render::gfx23::quad;

inline f32 F(u32 a) { return std::bit_cast<f32>(load<u32>(a)); }
inline void P(u32 a, f32 v) { store<u32>(a, std::bit_cast<u32>(v)); }
inline f32 Mul(f32 a, f32 b) { return static_cast<f32>(static_cast<f64>(a) * b); }
inline f32 Add(f32 a, f32 b) { return static_cast<f32>(static_cast<f64>(a) + b); }
inline f32 Sub(f32 a, f32 b) { return static_cast<f32>(static_cast<f64>(a) - b); }
inline f32 Div(f32 a, f32 b) { return static_cast<f32>(static_cast<f64>(a) / b); }
// fmadds / fnmsubs: one rounding.
inline f32 Madd(f32 a, f32 c, f32 b) { return static_cast<f32>(std::fma(static_cast<f64>(a), c, b)); }
inline f32 Nmsub(f32 a, f32 c, f32 b) { return static_cast<f32>(-std::fma(static_cast<f64>(a), c, -static_cast<f64>(b))); }

// vcmpgefp. against zero: every lane >= 0 (cr6 bit 24) / none (bit 26).
inline bool AllGe0(u32 v) {
    for (u32 k = 0; k < 4; ++k)
        if (!(F(v + 4 * k) >= 0.0f)) return false;
    return true;
}

inline u32 rotl(u32 v, int n) { return (v << n) | (v >> (32 - n)); }

}  // namespace

// bool Quad_InFrustum(const Quad* q, const f32 near_plane[4], const Matrix*
// viewproj, const Frustum* planes)   [Quads_Prepare] (name?)
// True if any corner is inside all six planes; false if all four corners
// are outside one plane. Otherwise the quad is clipped by the near plane
// (corners with distance >= 0 kept, an intersection point added on every
// edge that crosses it unless the edge is nearly parallel, |n.d| < 1e-5),
// each point projected through viewproj (divided by w), and the polygon's
// triangle fan tested against the clip-space box (0, 0, 0.5) +- (1, 1,
// 0.5): true if any triangle overlaps it, false with fewer than 3 points.
DECOMP(00577E40, Quad_InFrustum) {
    const u32 q = arg<u32>(ctx, 0), plane = arg<u32>(ctx, 1), viewproj = arg<u32>(ctx, 2);
    const u32 planes = arg<u32>(ctx, 3);
    Stack frame(ctx, 320);  // the original's sp+112 .. sp+432
    const u32 sp = frame.addr() - 112;
    const u32 dist_a = sp + 192, dist_b = sp + 256, near_dist = sp + 400, out = sp + 320;
    for (u32 i = 0; i < 4; ++i)
        call(ctx, g23::fn::Matrix2_TransformPoint,
             {planes + quad::kPlanesMatrices, q + quad::kPointSize * i, dist_a + 16 * i, dist_b + 16 * i});
    bool any_inside = false;
    for (u32 i = 0; i < 4; ++i)
        if (AllGe0(dist_a + 16 * i) && AllGe0(dist_b + 16 * i)) any_inside = true;
    if (any_inside) return ret(ctx, 1u);
    // Planes a.x .. a.w, b.x, b.y (b.z / b.w are not tested): all corners outside one.
    for (u32 lane = 0; lane < 6; ++lane) {
        const u32 base = lane < 4 ? dist_a + 4 * lane : dist_b + 4 * (lane - 4);
        bool some = false;
        for (u32 i = 0; i < 4; ++i)
            if (F(base + 16 * i) >= 0.0f) some = true;
        if (!some) return ret(ctx, 0u);
    }
    // The corners' near-plane distances.
    const f32 p0 = F(plane), p1 = F(plane + 4), p2 = F(plane + 8), p3 = F(plane + 12);
    {
        const u32 q0 = q, q1 = q + 16, q2 = q + 32, q3 = q + 48;
        P(near_dist + 4, Madd(p1, F(q1 + 4), Madd(p2, F(q1 + 8), Madd(p0, F(q1), p3))));
        P(near_dist + 8, Madd(p1, F(q2 + 4), Madd(p2, F(q2 + 8), Madd(p0, F(q2), p3))));
        P(near_dist + 0, Add(Madd(p2, F(q0 + 8), Madd(p0, F(q0), Mul(p1, F(q0 + 4)))), p3));
        P(near_dist + 12, Madd(p1, F(q3 + 4), Madd(p2, F(q3 + 8), Madd(p0, F(q3), p3))));
    }
    const f32 kZero = F(quad::kConsts), kOne = F(quad::kConsts + 4), kEps = F(quad::kConsts + 8);
    u32 count = 0;
    // v (x, y, z, 1) through viewproj, divided by w, appended to out.
    auto project = [&](u32 v) {
        call(ctx, g23::fn::Vec4_Transform, {v, viewproj});
        const f32 w = F(v + 12);
        const f32 s = Div(kOne, w);
        const f32 x = Mul(F(v), s), y = Mul(F(v + 4), s), z = Mul(F(v + 8), s), ws = Mul(w, s);
        P(v, x); P(v + 4, y); P(v + 8, z); P(v + 12, ws);
        const u32 o = out + 16 * count;
        P(o, x); P(o + 4, y); P(o + 8, z); P(o + 12, ws);
        ++count;
    };
    for (u32 i = 0; i < 4; ++i) {
        const f32 d = F(near_dist + 4 * i);
        const bool in = d >= kZero;
        const bool next_in = F(near_dist + 4 * ((i + 1) & 3)) >= kZero;
        const u32 a = q + 16 * i;
        if (in) {
            const u32 v = sp + 112;
            store<u32>(v, load<u32>(a));
            store<u32>(v + 4, load<u32>(a + 4));
            store<u32>(v + 8, load<u32>(a + 8));
            P(v + 12, 1.0f);
            project(v);
        }
        if (in == next_in) continue;
        const u32 b = q + 16 * ((i + 1) & 3);
        const f32 ax = F(a), ay = F(a + 4), az = F(a + 8);
        const f32 dy = Sub(ay, F(b + 4)), dx = Sub(ax, F(b)), dz = Sub(az, F(b + 8));
        const f32 den = Madd(p2, dz, Madd(p0, dx, Mul(p1, dy)));
        if (std::fabs(den) < kEps) continue;
        const f32 t = Div(d, den);
        const u32 v = sp + 128;
        P(v + 12, 1.0f);
        P(v, Nmsub(t, dx, ax));
        P(v + 4, Nmsub(t, dy, ay));
        P(v + 8, Nmsub(t, dz, az));
        project(v);
    }
    if (count < 3) return ret(ctx, 0u);
    if (load<u8>(quad::kClipBoxCentreGuard) == 0) {
        store<u32>(quad::kClipBoxCentre, 0);
        store<u32>(quad::kClipBoxCentre + 4, 0);
        store<u32>(quad::kClipBoxCentre + 8, 0x3F000000u);  // 0.5
        store<u8>(quad::kClipBoxCentreGuard, 1);
    }
    if (load<u8>(quad::kClipBoxHalfGuard) == 0) {
        store<u32>(quad::kClipBoxHalf, 0x3F800000u);  // 1, 1, 0.5
        store<u32>(quad::kClipBoxHalf + 4, 0x3F800000u);
        store<u32>(quad::kClipBoxHalf + 8, 0x3F000000u);
        store<u8>(quad::kClipBoxHalfGuard, 1);
    }
    const u32 tri = sp + 144;
    for (u32 k = 2; k < count; ++k) {
        const u32 src[3] = {out, out + 16 * (k - 1), out + 16 * k};
        for (u32 j = 0; j < 3; ++j)
            for (u32 c = 0; c < 12; c += 4) store<u32>(tri + 16 * j + c, load<u32>(src[j] + c));
        if (static_cast<u32>(call(ctx, g23::fn::Tri_OverlapsBox, {quad::kClipBoxCentre, quad::kClipBoxHalf, tri})) != 0)
            return ret(ctx, 1u);
    }
    ret(ctx, 0u);
}

// bool Quad_InClip(const ClipVolume* clip, const Quad* q)   [Quads_Prepare] (name?)
// True if the quad touches the clip volume: a corner inside all six
// planes (the clip's +96 matrices through Matrix2_TransformPoint, as
// Quad_InFrustum), else (unless all corners are outside one plane) an edge
// q[i] -> q[i+1] whose ray enters the planes (Planes_ClipRay) with t0 or
// t1 in [0, 1], else one of the volume's twelve edges (its eight corners
// at clip +224) crossing the quad (Quad_IntersectSegment).
DECOMP(00578868, Quad_InClip) {
    const u32 clip = arg<u32>(ctx, 0), q = arg<u32>(ctx, 1);
    Stack frame(ctx, 208);  // the original's sp+112 .. sp+320
    const u32 sp = frame.addr() - 112;
    const u32 dist_a = sp + 176, dist_b = sp + 240, t0 = sp + 304, t1 = sp + 308, hit = sp + 312;
    for (u32 i = 0; i < 4; ++i)
        call(ctx, g23::fn::Matrix2_TransformPoint,
             {clip + quad::kPlanesMatrices, q + quad::kPointSize * i, dist_a + 16 * i, dist_b + 16 * i});
    bool any_inside = false;
    for (u32 i = 0; i < 4; ++i)
        if (AllGe0(dist_a + 16 * i) && AllGe0(dist_b + 16 * i)) any_inside = true;
    if (any_inside) return ret(ctx, 1u);
    for (u32 lane = 0; lane < 6; ++lane) {  // a.x .. a.w, b.x, b.y
        const u32 base = lane < 4 ? dist_a + 4 * lane : dist_b + 4 * (lane - 4);
        bool some = false;
        for (u32 i = 0; i < 4; ++i)
            if (F(base + 16 * i) >= 0.0f) some = true;
        if (!some) return ret(ctx, 0u);
    }
    const f32 kOne = F(quad::kClipConsts), kZero = F(quad::kClipConsts + 4);
    const u32 dirs[4] = {sp + 160, sp + 144, sp + 128, sp + 112};
    for (u32 i = 0; i < 4; ++i) {
        const u32 a = q + 16 * i, b = q + 16 * ((i + 1) & 3), d = dirs[i];
        P(d, Sub(F(b), F(a)));
        P(d + 4, Sub(F(b + 4), F(a + 4)));
        P(d + 8, Sub(F(b + 8), F(a + 8)));
        if (static_cast<u32>(call(ctx, g23::fn::Planes_ClipRay, {clip, a, d, t0, t1})) == 0) continue;
        const f32 s = F(t0), e = F(t1);
        if ((s >= kZero && s <= kOne) || (e >= kZero && e <= kOne)) return ret(ctx, 1u);
    }
    // The volume's edges: corners c0..c3 and c4..c7 (two faces), the four
    // joining edges, then the faces' edges.
    static constexpr u8 kEdges[12][2] = {{0, 4}, {1, 5}, {2, 6}, {3, 7}, {1, 0}, {5, 4},
                                         {6, 7}, {2, 3}, {4, 7}, {5, 6}, {1, 2}, {0, 3}};
    const u32 c = clip + quad::kClipCorners;
    for (const auto& e : kEdges)
        if (static_cast<u32>(call(ctx, g23::fn::Quad_IntersectSegment, {q, c + 16u * e[0], c + 16u * e[1], hit})) != 0)
            return ret(ctx, 1u);
    ret(ctx, 0u);
}

namespace {

namespace lt = sly4::render::gfx23::lightinst;
namespace in22 = sly4::render::gfx22::inst;
namespace g22 = sly4::render::gfx22;

inline f32 LK(u32 off) { return F(lt::kConsts + off); }
inline f32 Msub(f32 a, f32 c, f32 b) { return static_cast<f32>(std::fma(static_cast<f64>(a), c, -static_cast<f64>(b))); }

// ApeTListAllocator<ApeLightInstance>::GetAllocator() inlined (gfx22's
// class_list): the free list, made on first use; 0 stored when Mem_Alloc
// fails. FreeList_Init reads r3..r7 (r8 / r9 hold the vtable / name).
u32 light_list(PPUContext& ctx) {
    const in22::InstClass& c = in22::kLights;
    const u32 header = load<u32>(c.holder);
    u32 list = load<u32>(header);
    if (list != 0) return list;
    list = static_cast<u32>(call(ctx, g22::fn::Mem_Alloc, {60, 0, 16}));
    if (list != 0) {
        const u32 heap = load<u32>(header + 4);
        for (u32 off : {8u, 4u, 12u, 20u, 16u, 24u, 32u, 28u, 36u, 40u, 44u, 48u}) store<u32>(list + off, 0);
        const u16 h54 = load<u16>(list + 54);
        store<u16>(list + 52, 0);
        store<u16>(list + 54, static_cast<u16>(h54 & 0x7FFF));
        store<u32>(list, c.vtbl);
        store<u32>(list + 56, c.name);
        call(ctx, g22::fn::FreeList_Init, {list, c.size, heap, 0, 0});
    }
    store<u32>(header, list);
    return list;
}

// The inlined random step: returns the previous kRandomLast as f32.
f32 random_prev() {
    const u64 last = load<u64>(lt::kRandomLast);
    const u32 s = load<u32>(lt::kRandomState);
    const u32 mix = (rotl(s, 4) + rotl(s, 16)) ^ lt::kRandomMix;
    store<u32>(lt::kRandomState, rotl(mix, 16));
    store<u64>(lt::kRandomLast, mix & 0xFFFFu);
    return static_cast<f32>(static_cast<f64>(static_cast<s64>(last)));
}

// The record's first evaluation (the light update step with no time
// added): the phase wrapped / clamped by its type, then the intensity
// (base + ramp / sine / random blend + noise, clamped at 0, square root)
// pushed to the owner when it changed (vfunc +36). Returns the r3 the
// original leaves: the vfunc's result, else the last constant address it
// built (the constants for the sine / random intensities, else
// kRandomLast's page).
u32 light_anim_eval(PPUContext& ctx, u32 rec) {
    using namespace lt;
    const s32 type = static_cast<s32>(load<u32>(rec + kLaType));
    const f32 zero = LK(4);
    if (type == 3) {
        const f32 period = F(rec + kLaPeriod);
        if (!(period <= zero) && F(rec + kLaPhase) > period) store<u32>(rec + kLaPhase, load<u32>(rec + kLaPeriod));
    } else if (type == 0) {
        const f32 period = F(rec + kLaPeriod);
        if (!(period <= zero)) {
            f32 p = F(rec + kLaPhase);
            while (p > period) {
                p = Sub(p, period);
                P(rec + kLaPhase, p);
            }
        }
    } else if (type == 1) {
        const f32 period = F(rec + kLaPeriod);
        if (!(period <= zero)) {
            if (load<u8>(rec + kLaDir) != 0) {
                const f32 p = F(rec + kLaPhase);
                if (p < zero) {
                    store<u8>(rec + kLaDir, 0);
                    P(rec + kLaPhase, -p);
                }
            } else {
                f32 p = F(rec + kLaPhase);
                if (p > period) {
                    do {
                        p = Sub(p, period);
                        P(rec + kLaPhase, p);
                    } while (p > period);
                    store<u8>(rec + kLaDir, 1);
                    P(rec + kLaPhase, Sub(period, p));
                }
            }
        }
    } else if (type == 2) {
        const f32 period = F(rec + kLaPeriod);
        if (!(period <= zero)) {
            f32 p = F(rec + kLaPhase);
            if (p > period) {
                store<u32>(rec + kLaRandA, load<u32>(rec + kLaRandB));  // lfs / stfs: raw
                P(rec + kLaRandB, Mul(random_prev(), LK(0)));
            }
            while (p > period) {
                p = Sub(p, period);
                P(rec + kLaPhase, p);
            }
        }
    }
    // The intensity.
    u32 r3 = lt::kRandomLast & 0xFFFF0000u;
    f32 v = F(rec + kLaBase);
    f32 phase = F(rec + kLaPhase);
    const f32 rnd = random_prev();
    const f32 half = LK(8);
    f32 noise = Mul(rnd, LK(0));
    noise = Msub(noise, noise, half);
    noise = Mul(noise, LK(12));
    noise = Mul(noise, F(rec + kLaNoise));
    if (phase < zero) phase = zero;
    const f32 period = F(rec + kLaPeriod);
    if (phase > period) phase = period;
    if (period > zero) {
        if (type == 1 || type == 2) r3 = lt::kConsts;
        if (type == 1) {
            const f32 twopi = LK(16);
            const f32 x = Div(Mul(phase, twopi), period);
            const f32 pi = static_cast<f32>(x >= 0.0f ? load<f64>(lt::kConsts + 24) : load<f64>(lt::kConsts + 32));  // fsel
            f32 a = Add(pi, x);
            const f32 k = static_cast<f32>(rt::fcfid(rt::fctid(static_cast<f64>(Div(a, twopi)), true)));
            a = Nmsub(k, twopi, a);
            a = Sub(a, pi);
            a = Add(a, LK(40));
            if (a > LK(44)) a = Sub(a, twopi);
            f32 s = Mul(Mul(a, LK(52)), std::fabs(a));
            s = Madd(a, LK(48), s);
            const f32 t = Msub(s, std::fabs(s), s);
            s = Madd(t, LK(56), s);
            s = Add(s, LK(60));
            v = Madd(Mul(F(rec + kLaAmp), s), half, v);
        } else if (type == 2) {
            const f32 x = Div(phase, period);
            const f32 w = Mul(Sub(LK(60), x), F(rec + kLaRandA));
            noise = Mul(Madd(x, F(rec + kLaRandB), w), F(rec + kLaAmp));
        } else {
            v = Add(v, Div(Mul(F(rec + kLaAmp), phase), period));
        }
        v = Add(v, noise);
        if (v < zero) v = zero;
    }
    v = static_cast<f32>(std::sqrt(static_cast<f64>(v)));
    const u32 owner = load<u32>(rec + kLaOwner);
    if (F(owner + lt::kOwnerRange) == v) return r3;
    const s32 otype = static_cast<s32>(load<u32>(owner + lt::kOwnerType));
    P(owner + lt::kOwnerRange, v);
    if (otype != 2) store<u32>(lt::kConfigLights + 108, load<u32>(lt::kConfigLights + 96));
    return static_cast<u32>(sly4::fx::vcall(ctx, owner, lt::kOwnerVChanged, {owner}));
}

}  // namespace

// void LightInstances_Start_00591A30(Bank* bank)   [called by Bank_Start] (name?)
// Collects the light instances that Light_Accepts(l, 1) (up to the
// original's 1024-entry stack array, unchecked), gives the bank a record
// holder (+0; capacity count + 256) and, for each light: registers its
// object's parent in the zone vector (once, unless flagged off or the
// parent has a LOD target) and marks both registered (+164 0x800),
// updates the object's LOD, stores the record index (+382), sets up the
// next record (defaults, two random values at +52 / +56, LightAnim_Init)
// and evaluates it once.
// Quirks: a failed holder allocation stores 0 at +0 and is then
// dereferenced; the record null test is on the first record's address
// only.
DECOMP(00591A30, LightInstances_Start_00591A30) {
    using namespace lt;
    const u32 bank = arg<u32>(ctx, 0);
    Stack frame(ctx, 4320 - 128);  // the original's sp+128 .. sp+4248
    const u32 sp = frame.addr() - 128;
    const u32 count_at = sp + 144, lights = sp + 148;
    const u32 list = light_list(ctx);
    const u32 item = static_cast<u32>(call(ctx, g22::fn::Obj_GetField4, {list}));
    store<u32>(count_at, 0);
    for (u32 l = item != 0 ? item + in22::kLink : 0; l != 0;) {
        if (static_cast<u32>(call(ctx, g23::fn::Light_Accepts, {l, 1})) != 0) {
            const u32 n = load<u32>(count_at);
            store<u32>(count_at, n + 1);
            store<u32>(lights + 4 * n, l);
        }
        const u32 next = load<u32>(l - in22::kLink);
        l = next != 0 ? next + in22::kLink : 0;
    }
    u32 holder = static_cast<u32>(call(ctx, g22::fn::Mem_Alloc, {lt::kHolderSize, 0, 16}));
    u32 r3 = holder;  // the r3 left at exit (no caller reads it)
    if (holder != 0) {
        const u32 n = load<u32>(count_at);
        const u32 cap = n + lt::kSpare;
        const u32 recs = static_cast<u32>(call(ctx, g22::fn::Mem_Alloc, {cap * lt::kRecSize, 0, 16}));
        r3 = recs;
        store<u32>(holder + lt::kHolderRecs, recs);
        store<u16>(holder + lt::kHolderCap, static_cast<u16>(cap));
        store<u16>(holder + lt::kHolderCount, 0);
        store<u16>(holder + lt::kHolderSpare, static_cast<u16>(n + lt::kSpare - 1));
    }
    const u32 count = load<u32>(count_at);
    store<u32>(bank, holder);
    for (u32 i = 0; i < count; ++i) {
        const u32 l = load<u32>(lights + 4 * i);
        u32 obj = load<u32>(l + lt::kObj);
        const u32 of = load<u32>(obj + lt::kFlags);
        if ((of & lt::kRegistered) == 0) {
            const u32 parent = load<u32>(obj + lt::kParent);
            u32 pf = load<u32>(parent + lt::kFlags);
            u32 of2 = of;
            if ((pf & lt::kRegistered) == 0) {
                if ((pf & lt::kInVector) == 0 && load<u32>(parent + lt::kLodTarget) == 0 && load<u8>(lt::kZoneVectorOff) == 0) {
                    const u32 vec = load<u32>(lt::kZoneVector);
                    u32 n = load<u16>(vec + 8);
                    const u32 vcap = load<u16>(vec + 4);
                    if (n == vcap) {
                        call(ctx, g23::fn::PtrVector_Reserve, {vec, (vcap << 1) & 0xFFFFFFFEu});
                        n = load<u16>(vec + 8);
                    }
                    store<u32>(load<u32>(vec) + 4 * n, parent);
                    const u32 n1 = load<u16>(vec + 8) + 1;
                    const u32 high = load<u16>(vec + 10);
                    store<u16>(vec + 8, static_cast<u16>(n1));
                    store<u16>(vec + 10, static_cast<u16>(static_cast<s32>(high) > static_cast<s32>(n1 & 0xFFFF) ? high : n1));
                    pf = load<u32>(parent + lt::kFlags) | lt::kInVector;
                }
                store<u32>(parent + lt::kFlags, pf | lt::kRegistered);
                of2 = load<u32>(obj + lt::kFlags);
            }
            store<u32>(obj + lt::kFlags, of2 | lt::kRegistered);
            obj = load<u32>(l + lt::kObj);
        }
        call(ctx, g23::fn::WorldObj_UpdateLod, {obj, 0, 0});
        const u32 h = load<u32>(bank);
        u32 n = load<u16>(h + lt::kHolderCount);
        store<u16>(l + lt::kRecIndex, static_cast<u16>(n));
        u32 recs = load<u32>(h + lt::kHolderRecs);
        const u32 rec0 = recs + n * lt::kRecSize;
        if (rec0 != 0) {
            store<u32>(rec0 + 0, lt::kFade);
            store<u32>(rec0 + 4, lt::kCullSq);
            store<u32>(rec0 + 8, lt::kFullSq);
            store<u32>(rec0 + kLaPhase, 0);
            store<u32>(rec0 + kLaPeriod, 0);
            store<u8>(rec0 + kLaFlags, load<u8>(rec0 + kLaFlags) & 0x7F);
            store<u32>(rec0 + kLaType, 0);
            store<u8>(rec0 + kLaDir, 0);
            store<u32>(rec0 + kLaNoise, 0);
            // Two random steps at once (the middle state is never stored).
            const u64 last = load<u64>(lt::kRandomLast);
            const u32 s = load<u32>(lt::kRandomState);
            const u32 mix = (rotl(s, 4) + rotl(s, 16)) ^ lt::kRandomMix;
            const u32 mix2 = (rotl(mix, 20) + mix) ^ lt::kRandomMix;
            const f32 inv = LK(0);
            const f32 ra = static_cast<f32>(static_cast<f64>(static_cast<s64>(last)));
            const f32 rb = static_cast<f32>(static_cast<f64>(static_cast<s64>(mix & 0xFFFFu)));
            P(rec0 + kLaRandA, Mul(ra, inv));
            store<u32>(lt::kRandomState, rotl(mix2, 16));
            store<u64>(lt::kRandomLast, mix2 & 0xFFFFu);
            store<u32>(rec0 + kLaBase, 0);
            store<u32>(rec0 + kLaAmp, 0);
            store<u32>(rec0 + kLaOwner, 0);
            P(rec0 + kLaRandB, Mul(rb, inv));
            call(ctx, g23::fn::LightAnim_Init, {rec0, l});
            n = load<u16>(h + lt::kHolderCount);
            recs = load<u32>(h + lt::kHolderRecs);
        }
        const u32 n1 = n + 1;
        store<u16>(h + lt::kHolderCount, static_cast<u16>(n1));
        r3 = light_anim_eval(ctx, recs + (n1 & 0xFFFF) * lt::kRecSize - lt::kRecSize);
    }
    ctx.gpr[3] = r3;
}

namespace {

namespace cr = sly4::render::gfx23::crop;

struct V4 {
    f32 v[4];
};
V4 VLd(u32 a) { return V4{{F(a), F(a + 4), F(a + 8), F(a + 12)}}; }
void VSt(u32 a, const V4& x) {
    for (u32 k = 0; k < 4; ++k) P(a + 4 * k, x.v[k]);
}
// vmaddfp a * s + b (s a splatted scalar), lane by lane, one rounding.
V4 VMaddS(const V4& a, f32 s, const V4& b) {
    V4 r;
    for (u32 k = 0; k < 4; ++k) r.v[k] = std::fmaf(a.v[k], s, b.v[k]);
    return r;
}
V4 VAdd(const V4& a, const V4& b) {  // vaddfp
    V4 r;
    for (u32 k = 0; k < 4; ++k) r.v[k] = a.v[k] + b.v[k];
    return r;
}
const V4 kV0{{0, 0, 0, 0}};
// out_i = r0 * s_i.x + r1 * s_i.y + r2 * s_i.z (+ r3 last for i = 3).
void MatCombine(const V4 r[4], const V4 s[4], V4 out[4]) {
    for (u32 i = 0; i < 4; ++i) {
        V4 a = VMaddS(r[0], s[i].v[0], kV0);
        a = VMaddS(r[1], s[i].v[1], a);
        a = VMaddS(r[2], s[i].v[2], a);
        out[i] = i == 3 ? VAdd(r[3], a) : a;
    }
}
inline f32 Fsel(f32 a, f32 c, f32 b) { return a >= 0.0f ? c : b; }
inline f32 CK(u32 off) { return F(cr::kConsts + off); }

}  // namespace

// void ViewCrop_Fit_0052D0F0(View* v, const f32 corners[8][4], const
// Matrix* m)   [called through a pointer] (name?)
// Projects the eight corners through m x the view's +48 matrix (divided
// by w), fits a rectangle around them in 2D, aligned with the axis from
// the centre of corners 0 / 2 to the centre of corners 4 / 6 (when the
// axis is shorter than 1e-3: the box of corners 4 .. 7, as the original
// computes it), and prepends to the view matrix the transform that maps
// the rectangle onto [-1, 1]: translate its corner to the origin, rotate
// by -atan2 of its side, scale by 2 / the side lengths, translate by
// (-1, 1). The frustum (+112) is then rebuilt from the new matrix.
DECOMP(0052D0F0, ViewCrop_Fit_0052D0F0) {
    const u32 view = arg<u32>(ctx, 0), corners = arg<u32>(ctx, 1), m = arg<u32>(ctx, 2);
    Stack frame(ctx, 816 - 112);  // the original's sp+112 .. sp+720
    const u32 sp = frame.addr() - 112;
    const u32 vec = sp + 112, pt = sp + 128, xf = sp + 144, pts = sp + 656;
    const u32 vm = view + cr::kMatrix;
    {
        // xf = m's rows through the view matrix (row 3's addend first).
        const V4 r0 = VLd(vm), r1 = VLd(vm + 16), r2 = VLd(vm + 32), r3 = VLd(vm + 48);
        for (u32 i = 0; i < 4; ++i) {
            const V4 s = VLd(m + 16 * i);
            V4 a = VMaddS(r0, s.v[0], i == 3 ? r3 : kV0);
            a = VMaddS(r1, s.v[1], a);
            a = VMaddS(r2, s.v[2], a);
            VSt(xf + 16 * i, a);
        }
    }
    const f32 one = CK(0);
    for (u32 i = 0; i < 8; ++i) {
        const u32 c = corners + 16 * i;
        store<u32>(pt, load<u32>(c));
        store<u32>(pt + 4, load<u32>(c + 4));
        store<u32>(pt + 8, load<u32>(c + 8));
        store<u32>(pt + 12, cr::kOne);
        call(ctx, g23::fn::Vec4_Transform, {pt, xf});
        const f32 w = F(pt + 12), s = Div(one, w);
        const f32 x = Mul(F(pt), s), y = Mul(F(pt + 4), s), z = Mul(F(pt + 8), s);
        P(pt, x); P(pt + 4, y); P(pt + 8, z); P(pt + 12, Mul(w, s));
        P(pts + 8 * i, x);
        P(pts + 8 * i + 4, y);
    }
    auto X = [&](u32 i) { return F(pts + 8 * i); };
    auto Y = [&](u32 i) { return F(pts + 8 * i + 4); };
    const f32 half = CK(4);
    f32 cy = Mul(Add(Y(0), Y(2)), half);
    f32 cx = Mul(Add(X(0), X(2)), half);
    const f32 dy = Msub(Add(Y(4), Y(6)), half, cy);
    const f32 dx = Msub(Add(X(4), X(6)), half, cx);
    const f32 len = static_cast<f32>(std::sqrt(static_cast<f64>(Madd(dx, dx, Mul(dy, dy)))));
    f32 a1, a2, a3, a4, x0, y0;  // the f1 .. f4 / f31 / f30 the side lengths come from
    if (std::fabs(len) < CK(8)) {
        // Corners 4 .. 7's bounding box (fsel chains: NaNs pick the second).
        const f32 maxx45 = Fsel(Sub(X(4), X(5)), X(4), X(5)), maxx67 = Fsel(Sub(X(6), X(7)), X(6), X(7));
        const f32 minx45 = Fsel(Sub(X(5), X(4)), X(4), X(5)), minx67 = Fsel(Sub(X(7), X(6)), X(6), X(7));
        const f32 maxy45 = Fsel(Sub(Y(4), Y(5)), Y(4), Y(5)), maxy67 = Fsel(Sub(Y(6), Y(7)), Y(6), Y(7));
        const f32 miny45 = Fsel(Sub(Y(5), Y(4)), Y(4), Y(5)), miny67 = Fsel(Sub(Y(7), Y(6)), Y(6), Y(7));
        const f32 maxx = Fsel(Sub(maxx45, maxx67), maxx45, maxx67);
        const f32 minx = Fsel(Sub(minx67, minx45), minx45, minx67);
        const f32 maxy = Fsel(Sub(maxy45, maxy67), maxy45, maxy67);
        const f32 miny = Fsel(Sub(miny67, miny45), miny45, miny67);
        a1 = miny; a2 = maxx; a3 = maxy; a4 = minx;
        x0 = maxx; y0 = maxy;
    } else {
        const f32 inv = Div(one, len);
        const f32 uy = Mul(dy, inv), ux = Mul(dx, inv), nuy = -uy;
        f32 amin = CK(16), amax = CK(12), bmax = CK(12), bmin = CK(16);
        for (u32 i = 0; i < 8; ++i) {
            const f32 py = Sub(Y(i), cy), px = Sub(X(i), cx);
            const f32 a = Madd(px, ux, Mul(py, uy));
            if (amin > a) amin = a;
            if (amax < a) amax = a;
            const f32 b = Madd(nuy, px, Mul(ux, py));
            if (bmax < b) bmax = b;
            if (bmin > b) bmin = b;
        }
        const f32 xa = Madd(ux, amin, cx), ya = Madd(uy, amin, cy);
        const f32 xb = Madd(ux, amax, cx), yb = Madd(uy, amax, cy);
        a1 = Madd(ux, bmax, ya);
        x0 = Madd(nuy, bmin, xa);
        y0 = Madd(ux, bmin, ya);
        a4 = Madd(nuy, bmin, xb);
        a3 = Madd(ux, bmin, yb);
        a2 = Madd(nuy, bmax, xa);
    }
    const f32 ey = Sub(a1, y0), ex = Sub(a2, x0), fy = Sub(y0, a3), fx = Sub(x0, a4);
    const f32 h = static_cast<f32>(std::sqrt(static_cast<f64>(Madd(ex, ex, Mul(ey, ey)))));
    const f32 w = static_cast<f32>(std::sqrt(static_cast<f64>(Madd(fx, fx, Mul(fy, fy)))));
    const f32 angle = static_cast<f32>(callf(ctx, g23::fn::atan2f, {}, {Div(ey, h), Div(ex, h)}));
    const u32 trans = sp + 208, rot = sp + 272, scale = sp + 336, trans2 = sp + 400, crop = sp + 528, out = sp + 592;
    store<u32>(vec + 8, 0);
    P(vec, -x0);
    P(vec + 4, -y0);
    call(ctx, g23::fn::Matrix_Translation, {trans, vec});
    callf(ctx, g23::fn::Matrix_RotationZ, {rot}, {-angle});
    const f32 two = CK(20);
    store<u32>(vec + 8, cr::kOne);
    P(vec, Div(two, h));
    P(vec + 4, Div(two, w));
    call(ctx, g23::fn::Matrix_ScaleDiag, {scale, vec});
    store<u32>(vec + 4, cr::kOne);
    store<u32>(vec + 8, 0);
    store<u32>(vec, cr::kMinusOne);
    call(ctx, g23::fn::Matrix_Translation, {trans2, vec});
    // crop = trans x rot x scale x trans2 (vector rows), its w column (0, 0, 0, 1).
    V4 a[4], b[4], t[4];
    for (u32 i = 0; i < 4; ++i) a[i] = VLd(trans + 16 * i), b[i] = VLd(rot + 16 * i);
    MatCombine(b, a, t);
    for (u32 i = 0; i < 4; ++i) b[i] = VLd(scale + 16 * i);
    MatCombine(b, t, a);
    for (u32 i = 0; i < 4; ++i) b[i] = VLd(trans2 + 16 * i);
    MatCombine(b, a, t);
    for (u32 i = 0; i < 4; ++i) {
        for (u32 k = 0; k < 3; ++k) P(crop + 16 * i + 4 * k, t[i].v[k]);
        store<u32>(crop + 16 * i + 12, i == 3 ? cr::kOne : 0u);
    }
    call(ctx, g23::fn::Matrix_MultiplyTo, {out, vm, crop});
    for (u32 k = 0; k < 64; k += 4) store<u32>(vm + k, load<u32>(out + k));  // lfs / stfs: raw
    call(ctx, g23::fn::Frustum_FromMatrix, {view + cr::kFrustum, vm});
}

namespace {

namespace vw = sly4::render::gfx23::view;

// out_i = p0 * s_i.x (+ p3 first for i = 3) + p1 * s_i.y + p2 * s_i.z:
// the matrix product the view code inlines (vmaddfp chains).
void MatProduct(u32 p, u32 s, u32 out) {
    const V4 r0 = VLd(p), r1 = VLd(p + 16), r2 = VLd(p + 32), r3 = VLd(p + 48);
    V4 o[4];
    for (u32 i = 0; i < 4; ++i) {
        const V4 x = VLd(s + 16 * i);
        V4 a = VMaddS(r0, x.v[0], i == 3 ? r3 : kV0);
        a = VMaddS(r1, x.v[1], a);
        o[i] = VMaddS(r2, x.v[2], a);
    }
    for (u32 i = 0; i < 4; ++i) VSt(out + 16 * i, o[i]);
}
void Copy64(u32 dst, u32 src) {  // lfs / stfs pairs: raw
    for (u32 k = 0; k < 64; k += 4) store<u32>(dst + k, load<u32>(src + k));
}
// A 4x4 with m's xyz rows and the w column (0, 0, 0, 1).
void Affine(u32 dst, u32 m) {
    for (u32 i = 0; i < 4; ++i) {
        for (u32 k = 0; k < 12; k += 4) store<u32>(dst + 16 * i + k, load<u32>(m + 16 * i + k));
        store<u32>(dst + 16 * i + 12, i == 3 ? 0x3F800000u : 0u);
    }
}

}  // namespace

// void View_Update(View* v, bool force)   [View_BeginFrame, View_ProjectPoint, ...] (name?)
// When forced or dirty (+52 0x80, cleared): rebuilds the projection
// (+1008: ortho, perspective, or perspective with the oblique clip plane
// +496 through the view matrix), keeps a raw copy (+1264) and its product
// with the view matrix (+1328), calls vfunc +16, applies the screen
// rotation (+48 bits 13..15 quarter turns, aspect-corrected for 1 / 3
// with +50 0x10), then view x projection (+880), its transpose (+944),
// the frustum (+272), the view's transposed rotation (+1072 .. +1120)
// and the roll matrix (+1136: about kRollAxis by -atan2(view +784,
// +792)). r3 is 0 when nothing is done.
DECOMP(005115F0, View_Update) {
    const u32 v = arg<u32>(ctx, 0);
    const u8 force = static_cast<u8>(arg<u32>(ctx, 1));
    if (force == 0 && (load<u8>(v + vw::kFlags52) & vw::kDirty) == 0) return ret(ctx, 0u);
    store<u8>(v + vw::kFlags52, load<u8>(v + vw::kFlags52) & 0x7F);
    Stack frame(ctx, 688 - 112);  // the original's sp+112 .. sp+688
    const u32 sp = frame.addr() - 112;
    const u32 proj = v + vw::kProj;
    const u32 mode = load<u8>(v + vw::kProjection);
    if (mode == 0) {
        const u32 m = sp + 208;
        callf(ctx, g23::fn::Mat4_Ortho, {m},
              {F(v + vw::kOrthoL), F(v + vw::kOrthoR), F(v + vw::kOrthoB), F(v + vw::kOrthoT), F(v + vw::kNear), F(v + vw::kFar)});
        Copy64(proj, m);
    } else if (mode != 2) {
        const u32 m = sp + 144;
        callf(ctx, g23::fn::Matrix_Perspective, {m}, {F(v + vw::kFov), F(v + vw::kAspect), F(v + vw::kNear), F(v + vw::kFar)});
        Copy64(proj, m);
    } else {
        const u32 m = sp + 272, plane = sp + 112;
        callf(ctx, g23::fn::Matrix_Perspective, {m}, {F(v + vw::kFov), F(v + vw::kAspect), F(v + vw::kNear), F(v + vw::kFar)});
        Copy64(proj, m);
        for (u32 k = 0; k < 16; k += 4) store<u32>(plane + k, load<u32>(v + vw::kClipPlane + k));
        call(ctx, g23::fn::Plane_Transform, {plane, v + vw::kViewMat});
        call(ctx, g23::fn::Matrix_ObliqueClip, {proj, plane});
    }
    Copy64(v + vw::kProjRaw, proj);
    MatProduct(v + vw::kProjRaw, v + vw::kViewMat, v + vw::kProjView);
    sly4::fx::vcall(ctx, v, vw::kVUpdated, {v});
    const u32 quarters = (load<u32>(v + vw::kRotBits) >> 13) & 7;
    if (quarters != 0) {
        const u32 rot = sp + 336, rot4 = sp + 400, copy = sp + 464, scale4 = sp + 528;
        callf(ctx, g23::fn::Matrix_RotationZ, {rot}, {Mul(static_cast<f32>(quarters), F(vw::kConsts))});
        Affine(rot4, rot);
        const f32 one = F(vw::kConsts + 4);
        Copy64(copy, proj);
        call(ctx, g23::fn::Matrix_MultiplyTo, {proj, copy, rot4});
        const u32 q = (load<u32>(v + vw::kRotBits) >> 13) & 7;
        const u32 num = load<u16>(v + vw::kAspectNum), den = load<u16>(v + vw::kAspectDen);
        if ((q == 1 || q == 3) && (load<u8>(v + vw::kFlags50) & vw::kAspectFix) != 0) {
            const u32 s = sp + 128;
            const f32 a = Div(static_cast<f32>(num), static_cast<f32>(den));
            store<u32>(s + 8, 0x3F800000u);
            P(s, a);
            P(s + 4, Div(one, a));
            call(ctx, g23::fn::Matrix_ScaleDiag, {rot, s});
            Affine(scale4, rot);
            Copy64(copy, proj);
            call(ctx, g23::fn::Matrix_MultiplyTo, {proj, copy, scale4});
        }
    }
    const u32 vm = v + vw::kViewMat, vp = v + vw::kViewProj;
    MatProduct(proj, vm, vp);
    {
        const V4 r[4] = {VLd(vp), VLd(vp + 16), VLd(vp + 32), VLd(vp + 48)};
        for (u32 c = 0; c < 4; ++c) VSt(v + vw::kViewProjT + 16 * c, V4{{r[0].v[c], r[1].v[c], r[2].v[c], r[3].v[c]}});
    }
    call(ctx, g23::fn::Frustum_FromMatrix, {v + vw::kFrustum, vp});
    {
        // The view's rotation transposed (the w lanes as the vperms leave them).
        const V4 a = VLd(vm), b = VLd(vm + 16), c = VLd(vm + 32), d = VLd(vm + 48);
        VSt(v + vw::kViewRot, V4{{a.v[0], b.v[0], c.v[0], b.v[1]}});
        VSt(v + vw::kViewRot + 16, V4{{a.v[1], b.v[1], c.v[1], a.v[0]}});
        VSt(v + vw::kViewRot + 32, V4{{a.v[2], b.v[2], c.v[2], a.v[2]}});
        VSt(v + vw::kViewTrans, d);
        for (u32 k = 0; k < 12; k += 4) store<u32>(v + vw::kViewTrans + k, load<u32>(vw::kViewPos + k));
    }
    const f32 ang = static_cast<f32>(callf(ctx, g23::fn::atan2f, {}, {F(vm + 32), F(vm + 40)}));
    callf(ctx, g23::fn::Matrix_FromAxisAngle, {v + vw::kRoll, vw::kRollAxis}, {-ang});
}
