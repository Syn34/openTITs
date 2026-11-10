// The direct (non-job) slot culling loops (gfx24): Slots_UpdateDirect and
// Slots_UpdateDirectB, the PPU fallbacks of the slot culling job when it
// isn't worth starting (Slots_UpdateA / Slots_UpdateB).
#include <bit>
#include <cmath>

#include "opentit/vmx.h"
#include "gfx24.h"

using namespace sly4;

namespace {

namespace sl = sly4::render::gfx24::slots;
namespace fn24 = sly4::render::gfx24::fn;

inline f32 F(u32 a) { return std::bit_cast<f32>(load<u32>(a)); }
inline f32 Mul(f32 a, f32 b) { return static_cast<f32>(static_cast<f64>(a) * b); }
inline f32 Sub(f32 a, f32 b) { return static_cast<f32>(static_cast<f64>(a) - b); }
inline f32 Div(f32 a, f32 b) { return static_cast<f32>(static_cast<f64>(a) / b); }
inline f32 Madd(f32 a, f32 c, f32 b) { return static_cast<f32>(std::fma(static_cast<f64>(a), c, b)); }  // fmadds
inline f32 Sqrt(f32 x) { return static_cast<f32>(std::sqrt(static_cast<f64>(x))); }                     // fsqrts
inline f32 FromInt(s32 v) { return static_cast<f32>(static_cast<f64>(v)); }                              // fcfid + frsp

// ---- VMX helpers (guest semantics)
v128 Lvx(u32 ea) { return vmx::load(mem::ptr(ea & ~15u)); }
// lvlx: the bytes from ea to the end of its 16-byte block, left-aligned, zero filled.
v128 Lvlx(u32 ea) {
    u8 b[16] = {};
    for (u32 k = 0; k < 16 - (ea & 15); ++k) b[k] = load<u8>(ea + k);
    return vmx::load(b);
}
v128 SplatW(u32 ea) { return vmx::vspltw(Lvlx(ea), 0); }  // lvlx + vspltw 0
v128 Splat(u32 bits) {
    v128 r;
    for (auto& e : r.u32_) e = bits;
    return r;
}
// vmaddfp: a * c + b per lane, one rounding.
v128 VMadd(const v128& a, const v128& c, const v128& b) {
    v128 r;
    for (int i = 0; i < 4; ++i) r.f32_[i] = std::fma(a.f32_[i], c.f32_[i], b.f32_[i]);
    return r;
}
// vandc with a 0x80000000 mask (vspltisw -1, vslw): the sign cleared.
v128 Abs(const v128& v) { return vmx::vandc(v, Splat(0x80000000u)); }
// vcmpgtfp. / vcmpgefp. against zero, read back through mfcr.
bool AllGt0(const v128& v) {
    for (int i = 0; i < 4; ++i)
        if (!(v.f32_[i] > 0.0f)) return false;
    return true;
}
bool AnyGt0(const v128& v) {
    for (int i = 0; i < 4; ++i)
        if (v.f32_[i] > 0.0f) return true;
    return false;
}

u32 AtomicStore(u32 ea, u32 v) {  // lwarx / stwcx. loop: returns the old word
    for (;;) {
        const u32 old = load<u32>(ea);
        if (mem::cas32(ea, old, v)) return old;
    }
}

// The sphere at s (centre, radius) against 8 planes SoA at P + 96:
// 0 outside some plane, 1 inside all, 2 crossing (inside but closer than
// the radius to at least one plane). Distances are x * nx + d, + y * ny,
// + z * nz (vmaddfp chains).
u32 SphereClass8(u32 P, u32 s) {
    const v128 x = SplatW(s), y = SplatW(s + 4), z = SplatW(s + 8), r = SplatW(s + 12);
    const u32 p = P + sl::kPlanes;
    v128 a = VMadd(Lvx(p + 0), x, Lvx(p + 48));
    v128 b = VMadd(Lvx(p + 64), x, Lvx(p + 112));
    a = VMadd(Lvx(p + 16), y, a);
    b = VMadd(Lvx(p + 80), y, b);
    a = VMadd(Lvx(p + 32), z, a);
    b = VMadd(Lvx(p + 96), z, b);
    const v128 mn = vmx::vminfp(a, b);
    bool cross = false;  // vcmpgtfp. r, min: not all false
    for (int i = 0; i < 4; ++i)
        if (r.f32_[i] > mn.f32_[i]) cross = true;
    const v128 m = vmx::vminfp(vmx::vaddfp(r, a), vmx::vaddfp(r, b));
    bool in = true;  // vcmpgefp. m, 0: all true
    for (int i = 0; i < 4; ++i)
        if (!(m.f32_[i] >= 0.0f)) in = false;
    return static_cast<u32>(in) + static_cast<u32>(in && cross);
}

// The 20 values of a group (5 vectors: planes 0-3, 4-7, ... of 4
// occluders, 5 planes each) regrouped per occluder by the constant masks
// (padding lanes from pad, the lvlx of the radius), true if some occluder
// has all 5 values > 0.
bool AnyAllPositive(const v128 e[5], const v128& pad, u32 C) {
    const v128 a = vmx::vminfp(e[0], vmx::vperm(pad, e[1], Lvx(C + 0)));
    const v128 b = vmx::vminfp(vmx::vsldoi(e[1], pad, 4), vmx::vperm(pad, e[2], Lvx(C + 16)));
    const v128 c = vmx::vminfp(vmx::vperm(e[2], pad, Lvx(C + 32)), vmx::vperm(pad, e[3], Lvx(C + 48)));
    const v128 d = vmx::vminfp(vmx::vperm(e[3], pad, Lvx(C + 64)), e[4]);
    return AllGt0(a) || AllGt0(b) || AllGt0(c) || AllGt0(d);
}

// The occlusion test of record rec against the occluder planes from list
// to end (one group of 4 occluders per pass, at least one pass): the
// sphere is occluded when it is on the inner side of all 5 planes of an
// occluder by more than its radius. With box, a sphere touching some
// plane gets a second test with the box (rec's matrix, half extents at
// ext): distance minus the box's projected extent.
bool Occluded(u32 rec, u32 ext, u32 list, u32 end, u32 C, bool box) {
    const u32 s = rec + sl::kRecSphere;
    const v128 cx = SplatW(s), cy = SplatW(s + 4), cz = SplatW(s + 8);
    const v128 pad = Lvlx(s + 12), r = vmx::vspltw(pad, 0);
    const v128 zero = v128::zero();
    u32 P = list;
    for (;;) {
        v128 X[5], Y[5], Z[5], D[5], E[5];
        for (u32 g = 0; g < 5; ++g) {
            const u32 q = P + 64 * g;
            X[g] = Lvx(q);
            Y[g] = Lvx(q + 16);
            Z[g] = Lvx(q + 32);
            D[g] = VMadd(Z[g], cz, VMadd(Y[g], cy, VMadd(X[g], cx, Lvx(q + 48))));
            E[g] = vmx::vsubfp(D[g], r);
        }
        bool occ = AnyAllPositive(E, pad, C);
        if (!occ && box) {
            bool touch = false;
            for (u32 g = 0; g < 5; ++g) touch = touch || AnyGt0(vmx::vaddfp(D[g], r));
            if (touch) {
                const v128 ex = Splat(load<u32>(ext)), ey = Splat(load<u32>(ext + 4)), ez = Splat(load<u32>(ext + 8));
                v128 m[3][3];
                for (u32 i = 0; i < 3; ++i)
                    for (u32 j = 0; j < 3; ++j) m[i][j] = SplatW(rec + sl::kRecMatrix + 16 * i + 4 * j);
                for (u32 g = 0; g < 5; ++g) {
                    v128 row[3];
                    for (u32 i = 0; i < 3; ++i)
                        row[i] = VMadd(Z[g], m[i][2], VMadd(Y[g], m[i][1], VMadd(X[g], m[i][0], zero)));
                    const v128 R = VMadd(Abs(row[2]), ez, VMadd(Abs(row[1]), ey, VMadd(Abs(row[0]), ex, zero)));
                    E[g] = vmx::vsubfp(D[g], R);
                }
                occ = AnyAllPositive(E, pad, C);
            }
        }
        P += sl::kOccluderGroup;
        if (occ) return true;
        if (!(P < end)) return false;
    }
}

// The cheap tests both loops start with: not skipped (flags 4), the mask
// (0 = any) shares a bit with the block's, flags 1 and 2 set, (distance to
// the view point * scale)^2 below the record's limit; then the sphere
// against the block's 8 planes. 0 = rejected, else SphereClass8's 1 / 2.
u32 Precheck(u32 G, u32 rec, f32 scale2) {
    const u32 flags = load<u16>(rec + sl::kRecFlags);
    if (flags & sl::kFlagSkip) return 0;
    const u32 mask = load<u32>(rec + sl::kRecMask);
    const bool on = mask == 0 || (mask & load<u32>(G + sl::kMaskBits)) != 0;
    const u32 p = G + sl::kViewPos, q = rec + sl::kRecPos;
    const f32 dy = Sub(F(p + 4), F(q + 4)), dx = Sub(F(p), F(q)), dz = Sub(F(p + 8), F(q + 8));
    f32 d = Mul(dy, dy);
    d = Madd(dx, dx, d);
    d = Madd(dz, dz, d);
    d = Mul(d, scale2);
    const bool near = d < F(rec + sl::kRecMaxDistSq);
    if (!on || (flags & sl::kFlagNeed) != sl::kFlagNeed || !near) return 0;
    return SphereClass8(G, rec + sl::kRecSphere);
}

// The LOD pick and fade: the first LOD threshold beyond the (distance -
// radius) * scale (none: hidden); then, when +108 * 100 / 65535 > 1e-5,
// the near fade (+104 > 0, +110 / 65535 floor) and the far fade (the last
// threshold) scale the alpha byte; hidden when a fade reaches 1/255 or
// the alpha is 0.
struct Lod {
    u32 index, alpha;
    bool vis;
};
Lod PickLod(u32 rec, u32 G, u32 C) {
    const u32 p = G + sl::kViewPos, q = rec + sl::kRecPos;
    const f32 dy = Sub(F(q + 4), F(p + 4)), dx = Sub(F(q), F(p)), dz = Sub(F(q + 8), F(p + 8));
    f32 d = Mul(dy, dy);
    d = Madd(dx, dx, d);
    d = Madd(dz, dz, d);
    d = Sqrt(d);
    d = Sub(d, F(rec + sl::kRecRadius));
    d = Mul(d, F(G + sl::kDistScale));
    u32 alpha = load<u8>(rec + sl::kRecAlpha);
    const s32 n = load<u8>(rec + sl::kRecLodCount);
    u32 i = 0;
    do {  // at least one threshold is read
        if (F(rec + sl::kRecLodDist + 4 * i) > d) break;
        i = (i + 1) & 0xFF;
    } while (static_cast<s32>(i) < n);
    if (!(static_cast<s32>(i) < n)) return {i, alpha, false};
    const f32 c65535 = F(C + sl::kC65535);
    const f32 range = Div(Mul(FromInt(load<u16>(rec + sl::kRecFadeA)), F(C + sl::kC100)), c65535);
    if (range > F(C + sl::kCEps)) {
        const f32 zero = F(C + sl::kCZero);
        const f32 nearf = F(rec + sl::kRecFadeNear);
        if (nearf > zero && d < nearf) {
            const f32 one = F(C + sl::kCOne);
            const f32 t = Sub(one, Div(Sub(nearf, d), range));
            const f32 floorv = Div(FromInt(load<u16>(rec + sl::kRecFadeB)), c65535);
            f32 k = floorv;
            f32 lim = one;
            if (t < lim) lim = t;
            if (k < lim) {  // max(floor, min(t, 1))
                k = one;
                if (t < k) k = t;
            }
            if (k < F(C + sl::kC255th) || k == F(C + sl::kC255th)) return {i, alpha, false};
            alpha = static_cast<u32>(rt::fctiw(Mul(FromInt(static_cast<s32>(alpha)), k), true)) & 0xFF;
        }
        // The far fade over the last threshold's range (index n - 1; with n
        // = 0 this reads the u16 pair at +108 as a float, as the original).
        const f32 last = F(rec + 108 + ((static_cast<u32>(n) << 2) & 0x3FC));
        if (d > Sub(last, range)) {
            const f32 t = Div(Sub(last, d), range);
            const f32 c255 = F(C + sl::kC255th);
            if (t < c255 || t == c255) return {i, alpha, false};
            const f32 one = F(C + sl::kCOne);
            f32 k = zero, m = one;
            if (t < m) m = t;
            if (m > k) {
                k = one;
                if (t < k) k = t;
            }
            alpha = static_cast<u32>(rt::fctiw(Mul(FromInt(static_cast<s32>(alpha)), k), true)) & 0xFF;
        }
    }
    return {i, alpha, alpha != 0};
}

}  // namespace

// void Slots_UpdateDirect(Obj* o)   [Slots_UpdateA's non-job path] (name?)
// o's virtual +8 first. Then every slot record of the table (count 0xFFFF:
// the end is address 0, as the original) that passes Precheck, the fine
// box test when crossing (Sphere_FrustumFine), the occluders (sphere,
// then box) and the LOD gets a draw entry at the block's entry cursor
// (0x0116E110 on); hidden ones are written but not counted. Visible
// records without flag 8 are also tested against each extra plane set
// (block +224 * (k + 1)): the current entry pointer is stored at the
// set's cursor, which advances only when the record is inside. The
// cursors are loaded at the start (the block's +3156 pointers, into a
// stack array at sp+128 that holds 12) and stored back atomically after
// the entry cursor.
DECOMP(00589BC8, Slots_UpdateDirect) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 opd = load<u32>(load<u32>(o) + 8);
    call_ptr(ctx, opd, {o, opd, load<u32>(opd)});
    Stack frame(ctx, 928);
    const u32 ext = frame.addr() + 112, cursors = frame.addr() + 128;
    const u32 base = load<u32>(sl::kSlotTable);
    const u32 count = load<u16>(sl::kSlotTable + 4);
    const u32 end = count != 0xFFFF ? base + (count << 7) : 0;
    const u32 G = sl::kBlockA, C = sl::kConstsA;
    u32 out = sl::kEntriesA;
    if (const u32 n = load<u32>(G + sl::kExtraCount); n != 0) {
        s32 left = static_cast<s32>(n);  // do-while on the signed count
        u32 k = 0;
        do {
            store<u32>(cursors + 4 * k, load<u32>(load<u32>(G + sl::kExtraCursors + 4 * k)));
            ++k;
        } while (--left > 0);
    }
    const f32 scale = F(G + sl::kDistScale), scale2 = Mul(scale, scale);
    for (u32 rec = base; rec != end; rec += sl::kRecSize) {
        const u32 cls = Precheck(G, rec, scale2);
        if (cls == 0) continue;
        const u32 s = rec + sl::kRecSphere;
        for (u32 k = 0; k < 3; ++k) store<u32>(ext + 4 * k, load<u32>(rec + sl::kRecExtents + 4 * k));
        u32 vis = 1;
        if (cls == 2) vis = static_cast<u32>(call(ctx, fn24::Sphere_FrustumFine, {G, s, ext, rec}));
        if (vis != 0) {
            const u32 nocc = load<u16>(sl::kOccluders + 4);
            if (nocc != 0) {
                const u32 list = load<u32>(sl::kOccluders);
                vis = Occluded(rec, ext, list, list + nocc * sl::kOccluderSize, C, true) ? 0 : 1;
            }
        }
        u32 alpha = load<u8>(rec + sl::kRecAlpha), lod = 255;
        if (vis != 0) {
            const Lod l = PickLod(rec, G, C);
            lod = l.index;
            alpha = l.alpha;
            vis = l.vis;
        }
        if (vis != 0 && !(load<u16>(rec + sl::kRecFlags) & sl::kFlagNoExtra) && load<u32>(G + sl::kExtraCount) != 0) {
            u32 k = 0;
            do {
                const u32 P = G + sl::kExtraSetStride * (k + 1);
                const u32 c = SphereClass8(P, s);
                u32 in = c != 0;
                if (c == 2) in = static_cast<u32>(call(ctx, fn24::Sphere_FrustumFine, {P, s, ext, rec}));
                const u32 cur = load<u32>(cursors + 4 * k);
                store<u32>(cur, out);
                store<u32>(cursors + 4 * k, cur + (in << 2));
                ++k;
            } while (k < load<u32>(G + sl::kExtraCount));
        }
        const u32 word = load<u32>(rec + sl::kRecDrawWord);
        store<u32>(out + sl::kEntryRec, rec);
        store<u32>(out + sl::kEntryWord, word);
        store<u8>(out + sl::kEntryLod, static_cast<u8>(lod));
        store<u8>(out + sl::kEntryAlpha, static_cast<u8>(alpha));
        u32 f = load<u32>(rec + sl::kRecHasExtra) != 0 ? 16u : 0u;
        store<u8>(out + sl::kEntryFlags, static_cast<u8>(f));
        f |= (load<u16>(rec + sl::kRecFlags) & sl::kFlagLit) ? 4u : 0u;
        store<u8>(out + sl::kEntryFlags, static_cast<u8>(f));
        f |= (load<u16>(rec + sl::kRecFlags) & sl::kFlagNoExtra) ? 8u : 0u;
        store<u8>(out + sl::kEntryFlags, static_cast<u8>(f));
        out += vis << 4;
    }
    AtomicStore(load<u32>(G + sl::kOutCursor), out);
    u32 r3 = load<u32>(G + sl::kExtraCount);
    if (r3 != 0) {
        u32 k = 0;
        do {
            AtomicStore(load<u32>(G + sl::kExtraCursors + 4 * k), load<u32>(cursors + 4 * k));
            ++k;
        } while (k < load<u32>(G + sl::kExtraCount));
        r3 = k;
    }
    ret(ctx, r3);  // r3 as the original leaves it (callers ignore it)
}

// void Slots_UpdateDirectB()   [Slots_UpdateB's non-job path] (name?)
// Slots_PrepareB, then the records from the table's u16 +6 + 1 to its
// u16 +8 (0xFFFF: address 0) as Slots_UpdateDirect, with these
// differences: the occluder count is rounded up to 4, the box occlusion
// test only with flag 0x20, no extra plane sets, and the entry gets no
// flags byte. Entries go to 0x01183100 on; r3 is the entry cursor's old
// word.
DECOMP(0058B4B8, Slots_UpdateDirectB) {
    call(ctx, fn24::Slots_PrepareB);
    Stack frame(ctx, 864);
    const u32 ext = frame.addr() + 112;
    const u32 first = load<u16>(sl::kSlotTable + 6) + 1, last = load<u16>(sl::kSlotTable + 8);
    const u32 base = load<u32>(sl::kSlotTable);
    const u32 start = (first & 0xFFFF) != 0xFFFF ? base + ((first << 7) & 0x007FFF80) : 0;
    const u32 end = last != 0xFFFF ? base + (last << 7) : 0;
    const u32 G = sl::kBlockB, C = sl::kConstsB;
    u32 out = sl::kEntriesB;
    const f32 scale = F(G + sl::kDistScale), scale2 = Mul(scale, scale);
    for (u32 rec = start; rec != end; rec += sl::kRecSize) {
        const u32 cls = Precheck(G, rec, scale2);
        if (cls == 0) continue;
        for (u32 k = 0; k < 3; ++k) store<u32>(ext + 4 * k, load<u32>(rec + sl::kRecExtents + 4 * k));
        u32 vis = 1;
        if (cls == 2) vis = static_cast<u32>(call(ctx, fn24::Sphere_FrustumFine, {G, rec + sl::kRecSphere, ext, rec}));
        if (vis != 0) {
            const u32 nocc = load<u16>(sl::kOccluders + 4);
            if (nocc != 0) {
                const bool box = (load<u16>(rec + sl::kRecFlags) & sl::kFlagBoxOcclude) != 0;
                const u32 groups = (nocc + 3) & 0xFFFC;
                const u32 list = load<u32>(sl::kOccluders);
                vis = Occluded(rec, ext, list, list + groups * sl::kOccluderSize, C, box) ? 0 : 1;
            }
        }
        u32 alpha = load<u8>(rec + sl::kRecAlpha), lod = 255;
        if (vis != 0) {
            const Lod l = PickLod(rec, G, C);
            lod = l.index;
            alpha = l.alpha;
            vis = l.vis;
        }
        const u32 word = load<u32>(rec + sl::kRecDrawWord);
        store<u32>(out + sl::kEntryRec, rec);
        store<u32>(out + sl::kEntryWord, word);
        store<u8>(out + sl::kEntryLod, static_cast<u8>(lod));
        store<u8>(out + sl::kEntryAlpha, static_cast<u8>(alpha));
        out += vis << 4;
    }
    ret(ctx, AtomicStore(load<u32>(G + sl::kOutCursor), out));
}
