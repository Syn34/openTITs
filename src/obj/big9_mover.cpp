// Game code, large-function sweep part 9: AI mover helpers used by
// Ai_ChooseMoveState (002CDA40, 002CDD68, 002CEB80): the nearest of eight
// move directions, a probe along an axis of the owner, and a ray test
// along a fixed direction.
#include "big9.h"

using namespace sly4;
using namespace sly4::big9;
using namespace sly4::big9::b9fn;

namespace {
using big7::Sel;
using big7::Splat1;
using big7::Fnmsubs;

constexpr u32 kDirGuards = 0x01068130u;  // u8 guards (+0 / +8 / +16 / +24) of the four static directions
constexpr u32 kDirs = 0x01068150u;       // static f32[4][2]: (0, -1), (0.7071, -0.7071), (1, 0), (0.7071, 0.7071)
constexpr u32 kMinStick = 0x0105AD48u;   // f32: shorter input vectors give -1 (tuning, name?)
constexpr u32 kOne = 0x002CDA38u;        // .text: f32 1.0
constexpr u32 kProbeK = 0x002CDD30u;     // .text: lane masks y / z / w at +0 / +16 / +32 (+0 also the f32 0); f32 0.5 (step) at +48, 1.5 (hits needed) at +52
constexpr u32 kRayK = 0x002CEB40u;       // .text: f32 0 (min distance), 0.5 (raise); lane masks y / z / w at +16 / +32 / +48
constexpr u32 kUp = 0x01164B60u;         // f32[3] the up axis
constexpr u32 kRayDir = 0x01164B90u;     // f32[3] fixed ray direction (down?)
constexpr u32 kCollectorBaseVt = 0x00B0EF40u;  // base ray collector vtable (the collector's state after use)
constexpr u32 kHkGetBody = 0x009D19C8u;  // Hk_GetBody9D19C8 (entity) -> rigid body
constexpr u32 kObjTransform = 0x002CCFF0u;     // Obj_Transform2CCFF0 (Mover*, const f32 pos[3], 1, f32 out[3], a, 0, 0) -> bool
constexpr u32 kHitSize = 80;
constexpr u32 kHitInline = 8;
constexpr u32 kOwner = 8;                // Mover: owner object (vfunc +64 -> its physics object; +96 matrix)
constexpr u32 kHeight = 68;              // Mover: f32 ray height along the owner's up row
constexpr u32 kVtPhys = 64;              // owner vfunc (o) -> physics object (+32 entity, +428 filter info)

struct P3 {
    f32 x, y, z;
};
P3 Ld3(u32 a) { return P3{F(a), F(a + 4), F(a + 8)}; }

// The x / y / z lanes of p as a quadword (w cleared) through the masks.
V4 Vec3(const P3& p, u32 my, u32 mz, u32 mw) {
    return Andc(Sel(Sel(Splat1(p.x), Splat1(p.y), Lvx(my)), Splat1(p.z), Lvx(mz)), Lvx(mw));
}

// hkpAllRayHitCollector at coll (+0 vtable, +4 early-out 1.0, +16 hits,
// +20 count, +24 capacity | 0x80000000 while inline) with 8 inline records
// at recs (+16 fraction 1.0, +20 / +32 -1, +64 / +68 0).
void InitCollector(u32 coll, u32 recs) {
    store<u32>(coll + 4, 0x3F800000u);
    store<u32>(coll, big8::kCollectorVt);
    store<u32>(coll + 16, recs);
    store<u32>(coll + 20, 0);
    store<u32>(coll + 24, 0x80000000u | kHitInline);
    for (u32 k = 0; k < kHitInline; ++k) {
        const u32 rec = recs + kHitSize * k;
        store<u32>(rec + 16, 0x3F800000u);
        store<u32>(rec + 20, 0xFFFFFFFFu);
        store<u32>(rec + 64, 0);
        store<u32>(rec + 32, 0xFFFFFFFFu);
        store<u32>(rec + 68, 0);
    }
    store<u32>(coll + 20, 0);
    store<u32>(coll + 4, 0x3F800000u);
}

// The allocator's vfunc +20 frees the heap hit array (capacity without the
// flag bits) of size cap * 80.
void FreeHits(PPUContext& ctx, u32 hits, u32 cap, bool opdFirst) {
    const u32 opd = load<u32>(load<u32>(big8::kHeapAlloc) + 20);
    const u64 size = Sx((cap & 0x3FFFFFFFu) * kHitSize);
    if (opdFirst) call_ptr(ctx, opd, {big8::kHeapAlloc, hits, size, opd, load<u32>(opd)});  // OPD in r6, entry in r7
    else call_ptr(ctx, opd, {big8::kHeapAlloc, hits, size, load<u32>(opd), opd});           // entry in r6, OPD in r7
}
}  // namespace

// s32 Mover_NearestDir(Mover* c, const f32 v[2])
// The index (0..7) of the static direction (0, -1), (0.7071, -0.7071),
// (1, 0), (0.7071, 0.7071) or their negations (4..7) with the largest dot
// product with v normalised (the first on ties); -1 when |v| is below the
// tuning value at 0x0105AD48. Builds the static table on first use.
DECOMP(002CDA40, Mover_NearestDir) {
    const u32 v = arg<u32>(ctx, 1);
    const u32 d0 = kDirs, d1 = kDirs + 8, d2 = kDirs + 16, d3 = kDirs + 24;
    if (load<u8>(kDirGuards) == 0) {
        store<u32>(d0, 0);
        store<u32>(d0 + 4, 0xBF800000u);
        store<u8>(kDirGuards, 1);
    }
    if (load<u8>(kDirGuards + 8) == 0) {
        store<u32>(d1, 0x3F3504F3u);
        store<u8>(kDirGuards + 8, 1);
        store<u32>(d1 + 4, 0xBF3504F3u);
    }
    if (load<u8>(kDirGuards + 16) == 0) {
        store<u32>(d2, 0x3F800000u);
        store<u32>(d2 + 4, 0);
        store<u8>(kDirGuards + 16, 1);
    }
    if (load<u8>(kDirGuards + 24) == 0) {
        store<u32>(d3, 0x3F3504F3u);
        store<u32>(d3 + 4, 0x3F3504F3u);
        store<u8>(kDirGuards + 24, 1);
    }
    const f32 y = F(v + 4), x = F(v);
    const f32 th = F(kMinStick);
    const f32 len2 = Fmadds(x, x, y * y);
    if (len2 < th * th) {
        ctx.fpr[1] = y;
        ret(ctx, static_cast<s32>(-1));
        return;
    }
    const f32 inv = F(kOne) / std::sqrt(len2);
    const f32 ny = inv * y, nx = inv * x;
    f32 s[8];
    for (u32 k = 0; k < 4; ++k) {
        const u32 d = kDirs + 8 * k;
        const f32 t = ny * F(d + 4);
        s[k] = Fmadds(nx, F(d), t);
        s[4 + k] = static_cast<f32>(-std::fma(static_cast<f64>(nx), static_cast<f64>(F(d)), static_cast<f64>(t)));  // fnmadds
    }
    u32 best = 0;
    for (u32 i = 1; i < 8; ++i)
        if (s[i] > s[best]) best = i;
    ctx.fpr[1] = s[7];  // the original's last load
    ret(ctx, best);
}

// bool Mover_ProbeDir(Mover* c, u8 axis, -, -, f32 out[3] or 0, a; f32 minDist, f32 maxDist)
// Along the owner's axis (0: +row 1, 1: -row 0, 2: -row 1, 3: +row 0, else
// none) from its position raised by +68 along row 1: a ray (all hits,
// sorted, filter info = physics object +428) shortens maxDist to the
// nearest entity hit; less than minDist gives false. Then from minDist in
// steps of 0.5 below that reach, Obj_Transform2CCFF0(c, point, 1, out, a,
// 0, 0) is tried at each point; the third success gives true, with out
// (when given) lowered by the raise. False when the reach runs out.
DECOMP(002CDD68, Mover_ProbeDir) {
    const u32 c = arg<u32>(ctx, 0);
    const u32 axis = arg<u32>(ctx, 1) & 0xFFu;
    const u32 out = arg<u32>(ctx, 4);
    const u32 a8 = arg<u32>(ctx, 5);
    const f64 minD = ctx.fpr[1];
    f64 reach = ctx.fpr[2];
    const u32 obj = load<u32>(c + kOwner);
    const u32 q = static_cast<u32>(VCall(ctx, obj, kVtPhys, {obj}));
    const u32 m = obj + 96;
    const P3 r0 = Ld3(m), r1 = Ld3(m + 16), pos = Ld3(m + 48);
    u32 body = 0;
    const u32 ent = load<u32>(q + 32);
    if (ent != 0) body = static_cast<u32>(call(ctx, kHkGetBody, {ent}));
    P3 dir = Ld3(kZero3);
    const u32 world = load<u32>(body + 8);  // quirk: address 8 when there is no body
    if (axis == 0) dir = r1;
    else if (axis == 1) dir = P3{-r0.x, -r0.y, -r0.z};
    else if (axis == 2) dir = P3{-r1.x, -r1.y, -r1.z};
    else if (axis == 3) dir = r0;
    const f32 h = F(c + kHeight);
    const P3 s{Fmadds(r1.x, h, pos.x), Fmadds(r1.y, h, pos.y), Fmadds(r1.z, h, pos.z)};
    const P3 e{Fmadds(reach, dir.x, s.x), Fmadds(reach, dir.y, s.y), Fmadds(reach, dir.z, s.z)};
    f64 f1 = s.z;  // what the original leaves in f1 along the way

    Stack frame(ctx, 736);  // the original's sp+112..+847
    const u32 sp = frame.addr() - 112;
    // hkpWorldRayCastInput at sp+128; collector at sp+176, records at sp+208.
    const u32 in = sp + 128, coll = sp + 176, recs = sp + 208;
    store<u8>(in + 32, 0);
    store<u32>(in + 40, 0);
    Stvx(in, Vec3(s, kProbeK, kProbeK + 16, kProbeK + 32));
    Stvx(in + 16, Vec3(e, kProbeK, kProbeK + 16, kProbeK + 32));
    store<u32>(in + 36, load<u32>(q + 428));
    InitCollector(coll, recs);
    call(ctx, kHkCastRay, {world, in, coll});
    call(ctx, kSortHits, {coll});
    const s32 count = static_cast<s32>(load<u32>(coll + 20));
    const u32 hits = load<u32>(coll + 16);
    const u32 cap = load<u32>(coll + 24);
    for (s32 j = 0; j < count; ++j) {
        const u32 rec = hits + kHitSize * static_cast<u32>(j);
        const u32 col = load<u32>(rec + 68);
        if (col == 0 || big8::HitEntity(col) == 0) continue;
        const f32 p = static_cast<f32>(reach * static_cast<f64>(F(rec + 16)));
        const f32 diff = static_cast<f32>(static_cast<f64>(p) - reach);
        f1 = p;
        if (!(diff >= 0.0f)) reach = p;  // fsel: the smaller of the two
    }
    if (reach < minD) {
        if ((cap & 0x80000000u) == 0) {
            store<u32>(coll + 20, 0);
            store<u32>(coll, big8::kCollectorVt);
            FreeHits(ctx, hits, cap, true);
        }
        ctx.fpr[1] = f1;
        ret(ctx, 0u);
        return;
    }
    store<u32>(coll + 20, 0);
    if ((cap & 0x80000000u) == 0) {
        store<u32>(coll, big8::kCollectorVt);
        FreeHits(ctx, hits, cap, false);
    }
    store<u32>(coll + 16, 0);
    store<u32>(coll + 24, 0x80000000u);
    store<u32>(coll, kCollectorBaseVt);
    f64 d = minD;
    f32 acc = F(kProbeK);
    bool ok = false;
    if (d < reach) {
        const f32 step = F(kProbeK + 48), need = F(kProbeK + 52);
        for (;;) {
            const f32 px = Fmadds(d, dir.x, pos.x), py = Fmadds(d, dir.y, pos.y), pz = Fmadds(d, dir.z, pos.z);
            store<f32>(sp + 112, px);
            store<f32>(sp + 116, py);
            store<f32>(sp + 120, pz);
            f1 = px;
            if (static_cast<u32>(call(ctx, kObjTransform, {c, sp + 112, 1, out, a8, 0, 0})) != 0) acc = static_cast<f32>(static_cast<f64>(acc) + step);
            if (acc > need || acc == need) {
                ok = true;
                break;
            }
            d = static_cast<f32>(d + static_cast<f64>(step));
            if (!(d < reach)) break;
        }
    }
    if (ok && out != 0) {
        const f32 hh = F(c + kHeight);
        const f32 ox = F(out), oy = F(out + 4), oz = F(out + 8);
        const f32 nx = Fnmsubs(r1.x, hh, ox), ny = Fnmsubs(r1.y, hh, oy), nz = Fnmsubs(r1.z, hh, oz);
        store<f32>(out, nx);
        store<f32>(out + 4, ny);
        store<f32>(out + 8, nz);
        f1 = nz;
    }
    ctx.fpr[1] = f1;
    ret(ctx, ok ? 1u : 0u);
}

// bool Mover_RayHit(Mover* c; f32 dist)
// False when dist <= 0 or the owner's physics object has no body in a
// world (quirk: without a body its fields are read from address 8 / 44).
// Else one ray (all hits, sorted, filter info = body +44, shape filter on)
// from the owner's position raised 0.5 along the up axis to its position
// plus dist along the fixed direction at 0x01164B90: true when it hits an
// entity.
DECOMP(002CEB80, Mover_RayHit) {
    const u32 c = arg<u32>(ctx, 0);
    const f64 dist = ctx.fpr[1];
    const u32 obj = load<u32>(c + kOwner);
    const u32 q = static_cast<u32>(VCall(ctx, obj, kVtPhys, {obj}));
    const u32 c0 = load<u32>(kRayK);
    SetF1Bits(ctx, c0);
    const P3 pos = Ld3(obj + 144);
    const f64 lim = std::bit_cast<f32>(c0);
    if (dist < lim || dist == lim) {
        ret(ctx, 0u);
        return;
    }
    u32 body = 0;
    const u32 ent = load<u32>(q + 32);
    if (ent != 0) body = static_cast<u32>(call(ctx, kHkGetBody, {ent}));
    const u32 filt = load<u32>(body + 44);  // quirk: addresses 44 / 8 when there is no body
    const u32 world = load<u32>(body + 8);
    if (world == 0) {
        ret(ctx, 0u);
        return;
    }
    const f32 raise = F(kRayK + 4);
    const P3 up = Ld3(kUp), rd = Ld3(kRayDir);
    const P3 s{Fmadds(up.x, raise, pos.x), Fmadds(up.y, raise, pos.y), Fmadds(up.z, raise, pos.z)};
    const P3 e{Fmadds(rd.x, dist, pos.x), Fmadds(rd.y, dist, pos.y), Fmadds(rd.z, dist, pos.z)};
    ctx.fpr[1] = s.z;

    Stack frame(ctx, 736);  // the original's sp+112..+847
    const u32 sp = frame.addr() - 112;
    // hkpWorldRayCastInput at sp+112; collector at sp+160, records at sp+192.
    const u32 in = sp + 112, coll = sp + 160, recs = sp + 192;
    store<u32>(in + 40, 0);
    Stvx(in, Vec3(s, kRayK + 16, kRayK + 32, kRayK + 48));
    store<u32>(in + 36, filt);
    store<u8>(in + 32, 1);
    Stvx(in + 16, Vec3(e, kRayK + 16, kRayK + 32, kRayK + 48));
    InitCollector(coll, recs);
    call(ctx, kHkCastRay, {world, in, coll});
    call(ctx, kSortHits, {coll});
    const s32 count = static_cast<s32>(load<u32>(coll + 20));
    const u32 hits = load<u32>(coll + 16);
    const u32 cap = load<u32>(coll + 24);
    u32 hit = 0;
    for (s32 j = 0; j < count; ++j) {
        const u32 col = load<u32>(hits + kHitSize * static_cast<u32>(j) + 68);
        if (col != 0 && big8::HitEntity(col) != 0) {
            hit = 1;
            break;
        }
    }
    if ((cap & 0x80000000u) == 0) {
        store<u32>(coll + 20, 0);
        store<u32>(coll, big8::kCollectorVt);
        FreeHits(ctx, hits, cap, true);
    }
    ret(ctx, hit);
}
