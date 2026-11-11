// Game code, large-function sweep part 20: a two-ray probe for a ledge in
// front of an object (Havok linear casts from two side points).
#include "big20.h"

using namespace sly4;
using namespace sly4::big20;

namespace {
namespace ledge {
constexpr u32 kK = 0x002CCFB0u;          // .text f32 0, 0.04, 1, 0.3; then the lane masks
constexpr u32 kUp = 0x01164B60u;         // f32[3] the up axis
constexpr u32 kSegInit = 0x01164AFCu;    // f32[2] the intersection's initial value
constexpr u32 kHkAlloc = 0x011DF31Cu;    // the Havok allocator object (vt 20 frees)
constexpr u32 kCollectorVt = 0x00F1DE58u;   // all-hits collector (48-byte hits)
constexpr u32 kCollectorBaseVt = 0x00B13C84u;
constexpr u32 kKeyLedge = 0x418F096Fu;   // world object property keys
constexpr u32 kKeyGrab = 0x15E9DC74u;
constexpr u32 kKeyKind = 0x2CC3F147u;
constexpr u32 HkAllHits_Sort = 0x008871E8;  // (collector*) (reads r3)
constexpr u32 Seg2_Intersect = 0x002CCE90;  // (-, a, b, c, d, Vec2* out) -> bool

inline f64 K(u32 off) { return Lf(kK + off); }
inline f64 Fnmsubs(f64 a, f64 c, f64 b) { return S(-std::fma(a, c, -b)); }

// Havok world object property {+124 {u32 key, pad, u64 value}[], s32 +128
// count}: the value of key, 0 when absent.
u64 Prop(u32 wo, u32 key) {
    const s32 n = static_cast<s32>(load<u32>(wo + 128));
    for (s32 k = 0; k < n; ++k) {
        const u32 e = load<u32>(wo + 124) + 16u * static_cast<u32>(k);
        if (load<u32>(e) == key) return load<u64>(e + 8);
    }
    return 0;
}

}  // namespace ledge
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
// x/z) with the edge: outPoint = (x, base y, z) and outNormal = the edge normal. Returns whether it found one.
DECOMP(002CCFF0, Ledge_Probe2CCFF0) {
    using namespace ledge;
    const u32 o = arg<u32>(ctx, 0);
    const u32 pos = arg<u32>(ctx, 1);
    const bool high = arg<u8>(ctx, 2) != 0;
    const u32 outPt = arg<u32>(ctx, 3), outN = arg<u32>(ctx, 4);
    u32 grab = arg<u32>(ctx, 5);
    const u32 kind = arg<u32>(ctx, 6);
    Stack frame(ctx, 1008);
    const u32 sp = frame.addr();
    const u32 own = load<u32>(o + 8);
    VCall(ctx, own, 64, {own});
    const u32 m = own + 96;
    const f64 r0x = Lf(m), r0y = Lf(m + 4), r0z = Lf(m + 8);
    const f64 r1x = Lf(m + 16), r1y = Lf(m + 20), r1z = Lf(m + 24);
    const f64 r2x = Lf(m + 32), r2y = Lf(m + 36), r2z = Lf(m + 40);
    if (grab == 0) grab = sp + 680;
    store<u8>(grab, 0);
    if (kind != 0) store<u8>(kind, 0);
    const f64 reach = high ? Lf(o + 32) : Lf(o + 44);
    const u32 angleW = high ? load<u32>(o + 28) : load<u32>(o + 40);
    const f64 k0 = K(0), kMin = K(4);
    const f64 z2 = S(k0 * k0);
    const f64 h = Lf(o + 68);
    const f64 bx = Fmadds(r1x, h, Lf(pos)), by = Fmadds(r1y, h, Lf(pos + 4)), bz = Fmadds(r1z, h, Lf(pos + 8));
    f64 l = Fmadds(r2x, r2x, z2);
    l = Fmadds(r2z, r2z, l);
    if (std::fabs(l) < kMin) return ret(ctx, 0u);
    const f64 one = K(8);
    f64 inv = S(one / S(std::sqrt(l)));
    const f64 fx = S(r2x * inv), fy = S(k0 * inv), fz = S(r2z * inv);
    if (load<u32>(o + 116) == 0) return ret(ctx, 0u);
    {
        const f64 back = Lf(o + 112), side = K(12);
        const f64 px = Fnmsubs(r2x, back, bx), py = Fnmsubs(r2y, back, by), pz = Fnmsubs(r2z, back, bz);
        store<u32>(sp + 720, angleW);
        store<u8>(sp + 688, 0);
        store<u8>(sp + 689, 0);
        for (u32 c = 0; c < 3; ++c) {
            Cw(sp + 160 + 4 * c, kZeroVec + 4 * c);
            Cw(sp + 176 + 4 * c, kZeroVec + 4 * c);
        }
        store<u32>(sp + 128, Fb(Fnmsubs(r0x, side, px)));
        store<u32>(sp + 132, Fb(Fnmsubs(r0y, side, py)));
        store<u32>(sp + 136, Fb(Fnmsubs(r0z, side, pz)));
        store<u32>(sp + 144, Fb(Fmadds(r0x, side, px)));
        store<u32>(sp + 148, Fb(Fmadds(r0y, side, py)));
        store<u32>(sp + 152, Fb(Fmadds(r0z, side, pz)));
    }
    for (u32 k = 0; k < 2; ++k) {
        const u32 pt = sp + 128 + 16 * k;
        const f64 d = S(Lf(o + 112) + reach);
        const f64 qx = Fmadds(r2x, d, Lf(pt)), qy = Fmadds(r2y, d, Lf(pt + 4)), qz = Fmadds(r2z, d, Lf(pt + 8));
        // Cast input: from (sp +112), to (sp +192), zeroed 16 bytes after;
        // the collector at sp +224 with an inline buffer of 8 hits at +256.
        for (u32 c = 0; c < 16; c += 4) store<u32>(sp + 208 + c, 0);
        store<u32>(sp + 224, kCollectorVt);
        store<u32>(sp + 228, 0x7F7FFFEEu);
        store<u32>(sp + 240, sp + 256);
        store<u32>(sp + 244, 0);
        store<u32>(sp + 248, 0x80000008u);
        Cw(sp + 112, pt);
        Cw(sp + 116, pt + 4);
        Cw(sp + 120, pt + 8);
        store<u32>(sp + 124, 0);
        store<u32>(sp + 192, Fb(qx));
        store<u32>(sp + 196, Fb(qy));
        store<u32>(sp + 200, Fb(qz));
        store<u32>(sp + 204, 0);
        const u32 world = load<u32>(o + 116);
        VCall(ctx, world, 64, {world, sp + 112, sp + 192, sp + 224, 0});
        if (static_cast<s32>(load<u32>(sp + 244)) > 0) {
            call(ctx, HkAllHits_Sort, {sp + 224});
            for (s32 i = 0; i < static_cast<s32>(load<u32>(sp + 244)); ++i) {
                const u32 hit = load<u32>(sp + 240) + 48u * static_cast<u32>(i);
                const u32 c = load<u32>(hit + 40);
                if (static_cast<s8>(load<u8>(c + 24)) != 1) continue;
                const u32 wo = c + static_cast<u32>(static_cast<s32>(static_cast<s8>(load<u8>(c + 16))));
                if (wo == 0) continue;
                if (static_cast<u32>(Prop(wo, kKeyLedge)) == 0 && static_cast<u32>(Prop(wo, kKeyGrab)) == 0) {
                    if (!high) continue;
                    break;
                }
                if (static_cast<u32>(Prop(wo, kKeyGrab)) != 0) store<u8>(grab, 1);
                if (kind != 0) store<u8>(kind, static_cast<u8>(Prop(wo, kKeyKind)));
                const f64 nx = Lf(hit + 16), ny = Lf(hit + 20);
                const u32 nzW = load<u32>(hit + 24);
                const u32 hx = load<u32>(hit), hy = load<u32>(hit + 4), hz = load<u32>(hit + 8);
                const f64 s = callf(ctx, b20fn::Math_Sinf, {}, {Lf(o + 36)});
                if (std::fabs(ny) > s) continue;
                const f64 nz = mid13::LfsF(nzW);
                f64 nl = Fmadds(nx, nx, z2);
                nl = Fmadds(nz, nz, nl);
                const f64 ni = S(one / S(std::sqrt(nl)));
                f64 dot = S(fy * S(k0 * ni));
                dot = Fmadds(fx, S(nx * ni), dot);
                dot = Fmadds(fz, S(nz * ni), dot);
                if (dot > k0) continue;
                store<u8>(sp + 688 + k, 1);
                store<u32>(sp + 160 + 16 * k, hx);
                store<u32>(sp + 164 + 16 * k, hy);
                store<u32>(sp + 168 + 16 * k, hz);
                break;
            }
        }
        // Collector destructor: the hit buffer freed unless it is not owned.
        const u32 cap = load<u32>(sp + 248);
        store<u32>(sp + 244, 0);
        if ((cap & 0x80000000u) == 0) {
            store<u32>(sp + 224, kCollectorVt);
            const u32 size = (cap & 0x3FFFFFFFu) * 48u;
            VCall(ctx, kHkAlloc, 20, {kHkAlloc, load<u32>(sp + 240), static_cast<u64>(static_cast<s64>(static_cast<s32>(size)))});
        }
    }
    if (load<u8>(sp + 688) == 0 || load<u8>(sp + 689) == 0) return ret(ctx, 0u);
    // The edge between the two hits.
    const u32 w160 = load<u32>(sp + 160), w168 = load<u32>(sp + 168), w176 = load<u32>(sp + 176);
    const f64 ey = S(Lf(sp + 164) - Lf(sp + 180));
    const f64 ex = S(mid13::LfsF(w160) - mid13::LfsF(w176));
    const f64 ez = S(mid13::LfsF(w168) - Lf(sp + 184));
    l = S(ey * ey);
    l = Fmadds(ex, ex, l);
    l = Fmadds(ez, ez, l);
    if (l < kMin) return ret(ctx, 0u);
    const f64 el = S(std::sqrt(l));
    const f64 ux = Lf(kUp), uz = Lf(kUp + 8), uy = Lf(kUp + 4);
    store<u32>(sp + 224, kCollectorBaseVt);
    store<u32>(sp + 240, 0);
    store<u32>(sp + 248, 0x80000000u);
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
    const f64 c = callf(ctx, b20fn::Math_Cosf, {}, {mid13::LfsF(angleW)});
    if (face > -c) return ret(ctx, 0u);
    // Segments a-b (the edge, x/z) and c-d (the base forward).
    const f64 d = S(Lf(o + 112) + reach);
    f64 fl = Fmadds(r2x, r2x, S(r2z * r2z));
    store<u32>(sp + 640, w160);
    store<u32>(sp + 644, w168);
    store<u32>(sp + 648, w176);
    Cw(sp + 652, sp + 184);
    store<u32>(sp + 656, Fb(bx));
    store<u32>(sp + 660, Fb(bz));
    Cw(sp + 672, kSegInit);
    Cw(sp + 676, kSegInit + 4);
    fl = S(one / S(std::sqrt(fl)));
    store<u32>(sp + 664, Fb(Fmadds(S(r2x * fl), d, bx)));
    store<u32>(sp + 668, Fb(Fmadds(S(r2z * fl), d, bz)));
    if (static_cast<u32>(call(ctx, Seg2_Intersect, {o, sp + 640, sp + 648, sp + 656, sp + 664, sp + 672})) == 0)
        return ret(ctx, 0u);
    if (outPt != 0) {
        Cw(outPt, sp + 672);
        store<u32>(outPt + 4, Fb(by));  // the base y (kept as a double at sp +728)
        Cw(outPt + 8, sp + 676);
    }
    if (outN != 0) {
        store<u32>(outN, Fb(n28));
        store<u32>(outN + 4, Fb(n27));
        store<u32>(outN + 8, Fb(n26));
    }
    ret(ctx, 1u);
}
