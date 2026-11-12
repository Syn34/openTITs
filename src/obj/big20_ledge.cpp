// Game code, large-function sweep part 20: a two-ray probe for a ledge in
// front of an object (Havok linear casts from two side points).
#include "big20.h"

namespace sly4::obj {

namespace {
constexpr u32 kK = 0x002CCFB0u;          // .text f32 0, 0.04, 1, 0.3; then the lane masks
constexpr u32 kUp = 0x01164B60u;         // f32[3] the up axis
constexpr u32 kSegInit = 0x01164AFCu;    // f32[2] the intersection's initial value
constexpr u32 kHkAllocObj = 0x011DF31Cu; // the Havok allocator object (vt 20 frees)
constexpr u32 kCollectorVt = 0x00F1DE58u;   // all-hits collector (48-byte hits)
constexpr u32 kCollectorBaseVt = 0x00B13C84u;
constexpr u32 kKeyLedge = 0x418F096Fu;   // world object property keys
constexpr u32 kKeyGrab = 0x15E9DC74u;
constexpr u32 kKeyKind = 0x2CC3F147u;
constexpr u32 HkAllHits_Sort = 0x008871E8;  // (collector*) (reads r3)
constexpr u32 Seg2_Intersect = 0x002CCE90;  // (-, a, b, c, d, Vec2* out) -> bool
constexpr u32 Math_Cosf = 0x007344E0;
constexpr u32 Math_Sinf = 0x007345F0;

// The prober: +8 its owner (vfn[64] refreshes the matrix +96), the high /
// low reach and angle, +68 the height, +112 the pull-back, +116 the world.
struct LedgeObj {
    pad<8> _0;
    be_u32 owner;       // +8
    pad<16> _12;
    be_u32 angle_hi;    // +28 (f32 bits)
    be_f32 reach_hi;    // +32
    be_f32 slope;       // +36
    be_u32 angle_lo;    // +40 (f32 bits)
    be_f32 reach_lo;    // +44
    pad<20> _48;
    be_f32 height;      // +68
    pad<40> _72;
    be_f32 back;        // +112
    be_u32 world;       // +116
};
static_assert(offsetof(LedgeObj, angle_hi) == 28 && offsetof(LedgeObj, height) == 68 && offsetof(LedgeObj, world) == 116);
// One 48-byte hit: +0 the point, +16 the normal, +40 the collidable.
struct LedgeHit {
    Vec3 p;             // +0
    pad<4> _12;
    Vec3 n;             // +16
    pad<12> _28;
    be_u32 coll;        // +40
    pad<4> _44;
};
static_assert(sizeof(LedgeHit) == 48);
// The all-hits collector with eight inline 48-byte hits.
struct LedgeCollector {
    be_u32 vt;          // +0
    be_u32 early_out;   // +4
    pad<8> _8;
    be_u32 hits;        // +16
    be_s32 count;       // +20
    be_u32 capflags;    // +24
    pad<4> _28;
    LedgeHit inline_hits[8];  // +32
};
static_assert(sizeof(LedgeCollector) == 416);
// The original's locals (sp offsets - 112).
struct LedgeLocals {
    Vec4 from;          // sp+112
    Vec4 pts[2];        // sp+128 / sp+144
    Vec4 hits[2];       // sp+160 / sp+176
    Vec4 to;            // sp+192
    Quad z208;          // sp+208
    LedgeCollector col; // sp+224
    be_u32 seg[10];     // sp+640: a, b, c, d (x, z each), out
    u8 grab;            // sp+680
    pad<7> _681;
    u8 hit[2];          // sp+688 / sp+689
    pad<30> _690;
    be_u32 angle;       // sp+720
};
static_assert(offsetof(LedgeLocals, col) == 112 && offsetof(LedgeLocals, seg) == 528 && offsetof(LedgeLocals, grab) == 568);
static_assert(offsetof(LedgeLocals, hit) == 576 && offsetof(LedgeLocals, angle) == 608);

f64 K(u32 off) { return lf(global<be_f32>(kK + off)); }
}  // namespace

// bool f(Obj* o, const f32 pos[3], u8 high, f32 outPoint[3], f32
// outNormal[3], u8* grab, u8* kind): o +8's matrix (refreshed by its vfunc
// 64) gives the forward row (row 2, flattened and normalized: none -> 0);
// the base is pos + row 1 * o +68, pulled back by row 2 * o +112, and two
// points 0.3 apart along row 0 cast forward (row 2 * (o +112 + reach), the
// reach o +32 / +44 by `high`, the angle o +28 / +40) through the Havok
// world o +116 (vfunc 64 with an all-hits collector, sorted). Hits on
// world objects (type 1) with property kKeyLedge or kKeyGrab (else: skipped,
// or the cast ends when `high`) set *grab when they have kKeyGrab, *kind =
// kKeyKind's value; a hit counts when its normal is no steeper than
// sin(o +36) and faces the cast. With both points hit, the edge between
// them must be longer than 0.2 and its normal (edge x up) face back within
// cos(angle); then the forward segment from the base is intersected (2D,
// x/z) with the edge: outPoint = (x, base y, z) and outNormal = the edge
// normal. Returns whether it found one (callers read r3).
bool Ledge_Probe2CCFF0(LedgeObj* o, const Vec3* pos, u8 high_, Vec3* outPt, Vec3* outN, u8* grab, u8* kind) {
    const bool high = high_ != 0;
    GuestFrame frame(1008);
    LedgeLocals& L = *frame.as<LedgeLocals>(112);
    const u32 own = o->owner;
    vcall(&gm<u8>(own), 64);
    const Mtx44& m = gm<Mtx44>(own + 96);
    const f64 r0x = lf(m.r[0].x), r0y = lf(m.r[0].y), r0z = lf(m.r[0].z);
    const f64 r1x = lf(m.r[1].x), r1y = lf(m.r[1].y), r1z = lf(m.r[1].z);
    const f64 r2x = lf(m.r[2].x), r2y = lf(m.r[2].y), r2z = lf(m.r[2].z);
    if (grab == nullptr) grab = &L.grab;
    *grab = 0;
    if (kind != nullptr) *kind = 0;
    const f64 reach = high ? lf(o->reach_hi) : lf(o->reach_lo);
    const u32 angleW = high ? o->angle_hi : o->angle_lo;
    const f64 k0 = K(0), kMin = K(4);
    const f64 z2 = S(k0 * k0);
    const f64 h = lf(o->height);
    const f64 bx = Fmadds(r1x, h, lf(pos->x)), by = Fmadds(r1y, h, lf(pos->y)), bz = Fmadds(r1z, h, lf(pos->z));
    f64 l = Fmadds(r2x, r2x, z2);
    l = Fmadds(r2z, r2z, l);
    if (std::fabs(l) < kMin) return false;
    const f64 one = K(8);
    f64 inv = S(one / S(std::sqrt(l)));
    const f64 fx = S(r2x * inv), fy = S(k0 * inv), fz = S(r2z * inv);
    if (o->world == 0u) return false;
    {
        const f64 back = lf(o->back), side = K(12);
        const f64 px = Fnmsubs(r2x, back, bx), py = Fnmsubs(r2y, back, by), pz = Fnmsubs(r2z, back, bz);
        L.angle = angleW;
        L.hit[0] = 0;
        L.hit[1] = 0;
        CopyXYZ(L.hits[0], global<Vec3>(kZeroVec));
        CopyXYZ(L.hits[1], global<Vec3>(kZeroVec));
        L.pts[0].x = static_cast<f32>(Fnmsubs(r0x, side, px));
        L.pts[0].y = static_cast<f32>(Fnmsubs(r0y, side, py));
        L.pts[0].z = static_cast<f32>(Fnmsubs(r0z, side, pz));
        L.pts[1].x = static_cast<f32>(Fmadds(r0x, side, px));
        L.pts[1].y = static_cast<f32>(Fmadds(r0y, side, py));
        L.pts[1].z = static_cast<f32>(Fmadds(r0z, side, pz));
    }
    LedgeCollector& col = L.col;
    for (u32 k = 0; k < 2; ++k) {
        const Vec4& pt = L.pts[k];
        const f64 d = S(lf(o->back) + reach);
        const f64 qx = Fmadds(r2x, d, lf(pt.x)), qy = Fmadds(r2y, d, lf(pt.y)), qz = Fmadds(r2z, d, lf(pt.z));
        // Cast input: from, to, zeroed 16 bytes after; the collector with an
        // inline buffer of 8 hits.
        L.z208 = Quad{};
        col.vt = kCollectorVt;
        col.early_out = 0x7F7FFFEEu;
        col.hits = guest(&col.inline_hits[0]);
        col.count = 0;
        col.capflags = 0x80000008u;
        CopyXYZ(L.from, pt);
        set_bits(L.from.w, 0u);
        L.to.x = static_cast<f32>(qx);
        L.to.y = static_cast<f32>(qy);
        L.to.z = static_cast<f32>(qz);
        set_bits(L.to.w, 0u);
        vcall(&gm<u8>(o->world), 64, &L.from, &L.to, &col, 0u);
        if (col.count > 0) {
            call(HkAllHits_Sort, &col);
            for (s32 i = 0; i < col.count; ++i) {
                const LedgeHit& hit = gm<LedgeHit>(col.hits + 48u * static_cast<u32>(i));
                const HkCollidable& c = gm<HkCollidable>(hit.coll);
                if (c.kind != 1) continue;
                const u32 wo = hit.coll + static_cast<u32>(static_cast<s32>(c.owner_off));
                if (wo == 0) continue;
                if (HkPropLo(wo, kKeyLedge) == 0 && HkPropLo(wo, kKeyGrab) == 0) {
                    if (!high) continue;
                    break;
                }
                if (HkPropLo(wo, kKeyGrab) != 0) *grab = 1;
                if (kind != nullptr) *kind = static_cast<u8>(HkPropLo(wo, kKeyKind));
                const f64 nx = lf(hit.n.x), ny = lf(hit.n.y);
                const u32 nzW = bits(hit.n.z);
                const u32 hx = bits(hit.p.x), hy = bits(hit.p.y), hz = bits(hit.p.z);
                const f64 s = call<f64>(Math_Sinf, lf(o->slope));
                if (std::fabs(ny) > s) continue;
                const f64 nz = LfsW(nzW);
                f64 nl = Fmadds(nx, nx, z2);
                nl = Fmadds(nz, nz, nl);
                const f64 ni = S(one / S(std::sqrt(nl)));
                f64 dot = S(fy * S(k0 * ni));
                dot = Fmadds(fx, S(nx * ni), dot);
                dot = Fmadds(fz, S(nz * ni), dot);
                if (dot > k0) continue;
                L.hit[k] = 1;
                set_bits(L.hits[k].x, hx);
                set_bits(L.hits[k].y, hy);
                set_bits(L.hits[k].z, hz);
                break;
            }
        }
        // Collector destructor: the hit buffer freed unless it is not owned.
        const u32 cap = col.capflags;
        col.count = 0;
        if ((cap & 0x80000000u) == 0) {
            col.vt = kCollectorVt;
            const u32 size = (cap & 0x3FFFFFFFu) * 48u;
            vcall(&gm<u8>(kHkAllocObj), 20, col.hits.get(), static_cast<s32>(size));
        }
    }
    if (L.hit[0] == 0 || L.hit[1] == 0) return false;
    // The edge between the two hits.
    const u32 w160 = bits(L.hits[0].x), w168 = bits(L.hits[0].z), w176 = bits(L.hits[1].x);
    const f64 ey = S(lf(L.hits[0].y) - lf(L.hits[1].y));
    const f64 ex = S(LfsW(w160) - LfsW(w176));
    const f64 ez = S(LfsW(w168) - lf(L.hits[1].z));
    l = S(ey * ey);
    l = Fmadds(ex, ex, l);
    l = Fmadds(ez, ez, l);
    if (l < kMin) return false;
    const f64 el = S(std::sqrt(l));
    const Vec3& up = global<Vec3>(kUp);
    const f64 ux = lf(up.x), uz = lf(up.z), uy = lf(up.y);
    col.vt = kCollectorBaseVt;
    col.hits = 0u;
    col.capflags = 0x80000000u;
    inv = S(one / el);
    const f64 dx = S(inv * ex), dz = S(inv * ez), dy = S(inv * ey);
    f64 n27 = Fmsubs(dz, ux, S(dx * uz));
    f64 n28 = Fmsubs(dy, uz, S(dz * uy));
    f64 n26 = Fmsubs(dx, uy, S(dy * ux));
    l = S(n27 * n27);
    l = Fmadds(n28, n28, l);
    l = Fmadds(n26, n26, l);
    inv = S(one / S(std::sqrt(l)));
    n27 = S(inv * n27);
    n28 = S(inv * n28);
    n26 = S(inv * n26);
    f64 face = S(fy * n27);
    face = Fmadds(fx, n28, face);
    face = Fmadds(fz, n26, face);
    const f64 c = call<f64>(Math_Cosf, LfsW(angleW));
    if (face > -c) return false;
    // Segments a-b (the edge, x/z) and c-d (the base forward).
    const f64 d = S(lf(o->back) + reach);
    f64 fl = Fmadds(r2x, r2x, S(r2z * r2z));
    be_u32* seg = L.seg;
    seg[0] = w160;
    seg[1] = w168;
    seg[2] = w176;
    seg[3] = bits(L.hits[1].z);
    seg[4] = std::bit_cast<u32>(static_cast<f32>(bx));
    seg[5] = std::bit_cast<u32>(static_cast<f32>(bz));
    seg[8] = global<be_u32>(kSegInit).get();
    seg[9] = global<be_u32>(kSegInit + 4).get();
    fl = S(one / S(std::sqrt(fl)));
    seg[6] = std::bit_cast<u32>(static_cast<f32>(Fmadds(S(r2x * fl), d, bx)));
    seg[7] = std::bit_cast<u32>(static_cast<f32>(Fmadds(S(r2z * fl), d, bz)));
    if (call<u32>(Seg2_Intersect, o, &seg[0], &seg[2], &seg[4], &seg[6], &seg[8]) == 0) return false;
    if (outPt != nullptr) {
        set_bits(outPt->x, seg[8]);
        outPt->y = static_cast<f32>(by);  // the base y (kept as a double at sp +728)
        set_bits(outPt->z, seg[9]);
    }
    if (outN != nullptr) {
        outN->x = static_cast<f32>(n28);
        outN->y = static_cast<f32>(n27);
        outN->z = static_cast<f32>(n26);
    }
    return true;
}

}  // namespace sly4::obj

NATIVE(002CCFF0, sly4::obj::Ledge_Probe2CCFF0)
