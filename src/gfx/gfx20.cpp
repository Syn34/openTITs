// The renderer range's next recompiled functions (gfx20). Names carry the
// address where the role is unclear.
#include <bit>
#include <cmath>

#include "fx.h"
#include "gfx20.h"
#include "pool.h"

using namespace sly4;

namespace {

namespace g11 = sly4::render::gfx11;
namespace g20 = sly4::render::gfx20;
using g11::lfs;
using g11::madds;
using g11::nmsubs;
using g11::sgl;
using g11::stfs_bits;

inline f32 f32_at(u32 a) { return std::bit_cast<f32>(load<u32>(a)); }
inline void put_f32(u32 a, f32 v) { store<u32>(a, std::bit_cast<u32>(v)); }
inline void put_fpr(u32 a, f64 v) { store<u32>(a, stfs_bits(v)); }  // stfs

}  // namespace

// void Lights_Cull_005AC840(View* view)   [called by f_0028D7B0] (name?)
// With the light table on and view->+256 bit 1 set: rebuilds the list of
// lights this view draws. Every light with bit 0 set gets bit 2 (not
// listed) first; its bounding sphere comes from its look-at frame
// (Matrix_LookAt, inverted as a rigid transform): the centre half way
// between near and far along the axis, the radius out to a far corner
// (perspective: the near half extents scaled by far / near / 2;
// orthographic: the half extents). It is dropped when outside one of the
// view's six planes; a sphere crossing one goes to Sphere_FrustumFine.
// Its fade is 1 - clamp((-view z - start) / length, 0, 1); a light with
// fade > 0, u8 +103 set and fewer than 12 listed is listed with bit 2
// cleared: in the light list, or (bit 1 set) on a stack list appended
// after the others at the end. The list's own count leaves out those
// appended ones (the original's quirk); the total counts all.
DECOMP(005AC840, Lights_Cull_005AC840) {
    namespace L = g20;
    const u32 view = arg<u32>(ctx, 0);
    const u8 on = load<u8>(L::g::lights_on);
    if (on == 0) return ret(ctx, 0u);
    if (view == 0) return ret(ctx, static_cast<u32>(on));
    if ((load<u32>(view + L::kViewFlags) & 2) == 0) return ret(ctx, 0u);  // rlwinm 0,30,30

    const u32 counts = L::g::light_counts, list = L::g::light_list;
    store<u16>(counts + 4, 0);
    store<u16>(counts + 2, 0);
    store<u16>(counts + 0, 0);

    // Stack: the look-at matrix and then its inverse (+0, 64 bytes), the
    // sphere centre (+64), the extents (+80), the appended list (+96).
    Stack frame(ctx, 192);
    const u32 inv = frame.addr(), centre = inv + 64, ext = inv + 80, after = inv + 96;
    const u32 planes = view + L::kViewPlanes;
    const f64 half = lfs(L::kCullConsts + 32), zero = lfs(L::kCullConsts + 36), one = lfs(L::kCullConsts + 40);
    const f64 one_d = g11::lfd(L::kCullConsts + 48), zero_d = g11::lfd(L::kCullConsts + 56);

    f64 f1 = ctx.fpr[1];  // f1 as the last light leaves it
    u32 n = load<u16>(load<u32>(L::g::lights + 4) + 2);
    for (u32 i = 0, off = 0; static_cast<s32>(i) < static_cast<s32>(n);) {
        const u32 e = load<u32>(L::g::lights + 12) + off;
        const u8 b = static_cast<u8>(load<u8>(e) | L::kLightNotListed);
        store<u8>(e, b);
        if (b & L::kLightOn) {
            call(ctx, L::fn::Matrix_LookAt, {inv, e + L::kLightEye, e + L::kLightDir, e + L::kLightUp});
            // The rigid inverse: the 3x3 transposed (lane 3 of the rows as
            // the vperm masks leave it), row 3 = -t times it.
            f32 m[4][4];
            for (u32 r = 0; r < 4; ++r)
                for (u32 k = 0; k < 4; ++k) m[r][k] = f32_at(inv + 16 * r + 4 * k);
            const f32 c[3][4] = {{m[0][0], m[1][0], m[2][0], m[1][1]},
                                 {m[0][1], m[1][1], m[2][1], m[0][0]},
                                 {m[0][2], m[1][2], m[2][2], m[0][2]}};
            const f32 t[3] = {0.0f - m[3][0], 0.0f - m[3][1], 0.0f - m[3][2]};  // vsubfp
            for (u32 k = 0; k < 4; ++k) {
                f32 v = std::fmaf(c[0][k], t[0], 0.0f);
                v = std::fmaf(c[1][k], t[1], v);
                v = std::fmaf(c[2][k], t[2], v);
                for (u32 r = 0; r < 3; ++r) put_f32(inv + 16 * r + 4 * k, c[r][k]);
                put_f32(inv + 48 + 4 * k, v);
            }
            auto M = [&](u32 off2) { return lfs(inv + off2); };

            const f64 nearp = lfs(e + L::kLightNear), farp = lfs(e + L::kLightFar);
            const f64 mid = sgl(sgl(nearp + farp) * half);
            const f64 cx = nmsubs(mid, M(32), M(48)), cy = nmsubs(mid, M(36), M(52)), cz = nmsubs(mid, M(40), M(56));
            put_fpr(centre, cx);
            put_fpr(centre + 4, cy);
            put_fpr(centre + 8, cz);
            f64 dx, dy, dz;
            if (load<u32>(e + L::kLightOrtho) == 0) {
                const f64 fr = lfs(e + L::kLightFar);
                const f64 scale = sgl(sgl(fr * half) / lfs(e + L::kLightNear));
                f64 py = nmsubs(fr, M(36), M(52)), px = nmsubs(fr, M(32), M(48)), pz = nmsubs(fr, M(40), M(56));
                const f64 ex = sgl(lfs(e + L::kLightHalfW) * scale), ey = sgl(lfs(e + L::kLightHalfH) * scale);
                px = nmsubs(ex, M(0), px);
                py = nmsubs(ex, M(4), py);
                pz = nmsubs(ex, M(8), pz);
                put_fpr(ext, ex);
                put_fpr(ext + 4, ey);
                const f64 depth = sgl(lfs(e + L::kLightFar) - lfs(e + L::kLightNear));
                px = nmsubs(ey, M(16), px);
                py = nmsubs(ey, M(20), py);
                pz = nmsubs(ey, M(24), pz);
                put_fpr(ext + 8, sgl(depth * half));
                dx = sgl(px - cx);
                dy = sgl(py - cy);
                dz = sgl(pz - cz);
            } else {
                const f64 depth = sgl(lfs(e + L::kLightFar) - lfs(e + L::kLightNear));
                dx = sgl(lfs(e + L::kLightHalfW) * half);
                dy = sgl(lfs(e + L::kLightHalfH) * half);
                dz = sgl(depth * half);
                put_fpr(ext, dx);
                put_fpr(ext + 4, dy);
                put_fpr(ext + 8, dz);
            }
            const f64 radius = sgl(std::sqrt(madds(dz, dz, madds(dx, dx, sgl(dy * dy)))));  // fsqrts

            f64 x = cx, y = cy, z = cz;
            f1 = x;
            bool visible = true;
            for (u32 k = 0; k < 6; ++k) {  // cmplwi
                const u32 p = planes + 16 * k;
                const f64 d = sgl(madds(lfs(p + 8), z, madds(lfs(p), x, sgl(lfs(p + 4) * y))) + lfs(p + 12));
                if (sgl(d + radius) < zero) {
                    visible = false;
                    break;
                }
                if (sgl(d - radius) < zero) {
                    const u32 inside = static_cast<u32>(call(ctx, L::fn::Sphere_FrustumFine, {planes, centre, ext, inv}));
                    x = lfs(centre);
                    y = lfs(centre + 4);
                    z = lfs(centre + 8);
                    f1 = x;
                    if (inside == 0) {
                        visible = false;
                        break;
                    }
                }
            }
            if (visible) {
                // The centre in view space (vmaddfp per lane, rows read with lvx).
                const u32 vm = (view + L::kViewMatrix) & ~15u;
                f32 vz = std::fmaf(f32_at(vm + 8), static_cast<f32>(cx), 0.0f);
                vz = std::fmaf(f32_at(vm + 24), static_cast<f32>(cy), vz);
                vz = std::fmaf(f32_at(vm + 40), static_cast<f32>(cz), vz);
                vz = f32_at(vm + 56) + vz;  // vaddfp
                f64 t = sgl(-static_cast<f64>(vz) - lfs(e + L::kLightFadeStart));
                t = sgl(t / lfs(e + L::kLightFadeLength));
                t = sgl(sgl(one - t) >= 0.0 ? t : one_d);  // fsel, frsp
                t = sgl(t >= 0.0 ? t : zero_d);
                f1 = t;
                const f64 fade = sgl(one - t);
                if (fade > zero && load<u8>(e + L::kLightListed) != 0 &&
                    static_cast<s32>(load<u16>(counts)) < static_cast<s32>(L::kMaxListed)) {
                    if ((load<u8>(e) & L::kLightAfter) == 0) {
                        const u32 k = load<u16>(counts + 2);
                        store<u16>(counts + 2, static_cast<u16>(k + 1));
                        store<u32>(list + 8 * k, e);
                        put_fpr(list + 8 * k + 4, fade);
                    } else {
                        const u32 k = load<u16>(counts + 4);
                        store<u16>(counts + 4, static_cast<u16>(k + 1));
                        store<u32>(after + 8 * k, e);
                        put_fpr(after + 8 * k + 4, fade);
                    }
                    store<u8>(e, static_cast<u8>(load<u8>(e) & ~L::kLightNotListed));  // rlwimi from r26 = 0
                    store<u16>(counts, static_cast<u16>(load<u16>(counts) + 1));
                }
            }
        }
        ++i;
        off += L::kLightSize;
        n = load<u16>(load<u32>(L::g::lights + 4) + 2);
    }
    ctx.fpr[1] = f1;

    const u32 n_after = load<u16>(counts + 4);
    if (static_cast<s32>(n_after) <= 0) return ret(ctx, n);  // r3: the light count last read
    for (u32 i = 0; static_cast<s32>(i) < static_cast<s32>(n_after); i = (i + 1) & 0xFFFF) {
        const u32 d = list + 8 * (load<u16>(counts + 2) + i);
        store<u32>(d, load<u32>(after + 8 * i));
        store<u32>(d + 4, load<u32>(after + 8 * i + 4));  // lfs / stfs pair
        ctx.fpr[1] = lfs(after + 8 * i + 4);              // f1 left as the last fade copied
    }
    ret(ctx, list);  // r3 as left
}

namespace {

namespace g13 = sly4::render::gfx13;
namespace g18 = sly4::render::gfx18;

inline f64 cvt(s64 v) { return sgl(static_cast<f64>(v)); }                // fcfid, frsp
inline f64 trunc_s(f64 x) { return sgl(rt::fcfid(rt::fctid(x, true))); }  // fctidz, fcfid, frsp
inline u32 fctidz_lo(f64 x) { return static_cast<u32>(rt::fctid(x, true)); }
inline s32 fctiwz(f64 x) { return static_cast<s32>(static_cast<u32>(rt::fctiw(x, true))); }

// The four lanes of an lvx (address rounded down to 16).
struct V4 {
    f32 v[4];
};
inline V4 lvx(u32 a) {
    V4 r;
    for (u32 k = 0; k < 4; ++k) r.v[k] = f32_at((a & ~15u) + 4 * k);
    return r;
}
// m0 * x + 0, + m1 * y, + m2 * z (vmaddfp), per lane.
inline V4 madd3(const V4& m0, const V4& m1, const V4& m2, f32 x, f32 y, f32 z) {
    V4 r;
    for (u32 k = 0; k < 4; ++k) {
        f32 v = std::fmaf(m0.v[k], x, 0.0f);
        v = std::fmaf(m1.v[k], y, v);
        r.v[k] = std::fmaf(m2.v[k], z, v);
    }
    return r;
}
// The same, then + m3 (vaddfp).
inline V4 transform(const V4& m0, const V4& m1, const V4& m2, const V4& m3, f32 x, f32 y, f32 z) {
    V4 r = madd3(m0, m1, m2, x, y, z);
    for (u32 k = 0; k < 4; ++k) r.v[k] = m3.v[k] + r.v[k];
    return r;
}

}  // namespace

// void Emitter_BuildParticles_00506598(Emitter* e, Record* rec, const f32
// view[4][4], const Occluder* occ, u16 occ_count)   [Emitter_Draw_00507B18]
// Walks the emitter's particle ring from the oldest (with an empty ring
// it still reads one particle at address 0, the original's quirk) and
// writes, per particle drawn, the per-index arrays the sprite draws read:
// colour (key lerp, fade in / out, random darkening, record tint), world
// position (velocity and half acceleration over the age past the delay,
// or the props' spline +136), its view-space position (by rec's matrix
// times view, or view alone when e->+24 is set; then lane 3 is the
// stack's), size, UV rect, spin matrix, layer byte and direction.
// Particles past their life, fully transparent, outside the record's
// depth range or inside one of the occluder boxes are skipped. Stops
// after writing index 512 (one past the arrays' 512, the original's
// quirk); rec->+124 = the count written.
DECOMP(00506598, Emitter_BuildParticles_00506598) {
    namespace L = g20;
    const u32 e = arg<u32>(ctx, 0), rec = arg<u32>(ctx, 1), view = arg<u32>(ctx, 2), occ = arg<u32>(ctx, 3);
    const u32 occ_count = arg<u32>(ctx, 4) & 0xFFFF;
    const u32 colour = load<u32>(rec + L::kRecColour), motion = load<u32>(rec + L::kRecMotion),
              size = load<u32>(rec + L::kRecSize), uv = load<u32>(rec + L::kRecUv), spin = load<u32>(rec + L::kRecSpin);
    const u32 pool = load<u32>(e + 72);
    const bool empty = load<u32>(pool + L::kPoolCount) == 0;

    // The original's frame layout: the matrix at +128 (its lanes 3 stay as
    // the stack holds them when only view's rows are copied), the motion
    // offset at +112.
    Stack frame(ctx, 480);
    const u32 mat = frame.addr() + 128, ofs = frame.addr() + 112;
    if (load<u32>(e + 24) != 0) {
        for (u32 r = 0; r < 4; ++r)
            for (u32 k = 0; k < 3; ++k) store<u32>(mat + 16 * r + 4 * k, load<u32>(view + 16 * r + 4 * k));
    } else {
        // rec's rows times view (vmaddfp per lane; row 3 adds view's row 3).
        const V4 v0 = lvx(view), v1 = lvx(view + 16), v2 = lvx(view + 32), v3 = lvx(view + 48);
        for (u32 r = 0; r < 4; ++r) {
            const V4 m = lvx(rec + 16 * r);
            const V4 o = madd3(v0, v1, v2, m.v[0], m.v[1], m.v[2]);
            for (u32 k = 0; k < 4; ++k) put_f32(mat + 16 * r + 4 * k, r == 3 ? v3.v[k] + o.v[k] : o.v[k]);
        }
    }

    const u32 C = L::kParticleConsts;
    const f64 k127 = lfs(C), half = lfs(C + 36), two_pi = lfs(C + 40), k255 = lfs(C + 44), eps = lfs(C + 48),
              two = lfs(C + 52), zero_f = lfs(C + 20), zero_d = g11::lfd(C + 24), k256 = lfs(C + 32);
    u32 p = empty ? 0 : load<u32>(pool + L::kPoolOldest);
    u32 n = 0;
    do {
        const u32 cur = p;
        u32 nx = cur + L::kParticleSize;
        const u32 end = load<u32>(pool + L::kPoolEnd), first = load<u32>(pool + L::kPoolFirst),
                  last = load<u32>(pool + L::kPoolLast);
        if (static_cast<s32>(nx - end) >= 0) nx = first;
        p = nx == last ? 0 : nx;

        const f64 one = lfs(C + 4);
        const f64 age = sgl(lfs(e + 8) - lfs(cur + L::kPartBirth));
        const f64 rate_age = sgl(lfs(rec + L::kRecRate) * age);
        const f64 t = sgl(rate_age * sgl(cvt(load<u8>(cur + L::kPartLife)) * k127));
        if (t >= one) continue;

        // Colour.
        const u32 col = g13::g::particle_colour + 4 * n;
        if (colour == 0) {
            for (u32 c = 0; c < 4; ++c) store<u8>(col + c, 255);
        } else {
            const s32 keys = static_cast<s32>((static_cast<u32>(load<u8>(colour + 6)) << 8) - 256u);
            const u32 ix = fctidz_lo(sgl(t * cvt(keys)));
            const u32 rnd = load<u32>(cur + L::kPartRandom) & 0xFF;
            const u32 key = (ix >> 6) & 0x03FFFFFCu, frac = ix & 0xFF;  // rlwinm 26,6,29 / 0,24,31
            for (u32 c = 0; c < 4; ++c) {
                const s32 a = load<u8>(colour + key + 20 + c), b = load<u8>(colour + key + 24 + c);
                store<u8>(col + c, static_cast<u8>(a + ((b - a) * static_cast<s32>(frac) >> 8)));
            }
            f64 in = sgl(t * lfs(colour + 12));
            in = in < one ? in : one;
            f64 out = sgl(sgl(one - t) * lfs(colour + 16));
            out = out < one ? out : one;
            const s32 alpha = fctiwz(sgl(sgl(in * out) * k256));
            store<u8>(col + 3, static_cast<u8>(static_cast<s32>(static_cast<u32>(load<u8>(col + 3)) * static_cast<u32>(alpha)) >> 8));
            const u32 dark = load<u16>(colour + 8);
            if (static_cast<s32>(dark) > 0) {
                const u32 d = (dark * rnd) >> 8;  // rlwinm 24,8,31
                u32 v[4];
                for (u32 c = 0; c < 4; ++c) v[c] = load<u8>(col + c);
                for (u32 c : {0u, 1u, 3u, 2u}) store<u8>(col + c, static_cast<u8>(v[c] - (((v[c] * d) >> 8) & 0x00FFFFFFu)));
            }
        }
        u32 a8 = 0;
        for (u32 c = 0; c < 4; ++c) {
            a8 = ((load<u8>(col + c) * static_cast<u32>(load<u8>(rec + L::kRecTint + c))) >> 8) & 0xFF;
            store<u8>(col + c, static_cast<u8>(a8));
        }
        if (a8 == 0) continue;

        // Position: the motion offset (spline, or v t + a t^2 / 2 past the delay).
        const f64 px = lfs(cur), py = lfs(cur + 4), pz = lfs(cur + 8);
        const f64 vx = lfs(cur + L::kPartVelocity), vy = lfs(cur + L::kPartVelocity + 4), vz = lfs(cur + L::kPartVelocity + 8);
        for (u32 k = 0; k < 12; k += 4) store<u32>(ofs + k, load<u32>(g18::kZeroVec + k));
        f64 ox;
        {
            const u32 spline = load<u32>(load<u32>(e + 4) + 136);
            if (spline != 0) {
                callf(ctx, L::fn::Spline_PointAt, {spline + 16, 0x01160000u /* r4 as left */, ofs, 0},
                      {sgl(lfs(rec + L::kRecRate) * age)});
                ox = lfs(ofs);
            } else {
                const f64 d = sgl(age - lfs(motion + 32));
                const f64 tt = sgl(d >= 0.0 ? d : zero_d);  // fsel, frsp
                const f64 h = sgl(sgl(tt * tt) * half), vzt = sgl(vz * tt);
                const f64 ax = sgl(lfs(motion + 16) * h);
                ox = madds(vx, tt, ax);
                const f64 ay = sgl(lfs(motion + 20) * h);
                const f64 oy = madds(vy, tt, ay);
                const f64 oz = madds(lfs(motion + 24), h, vzt);
                put_fpr(ofs, ox);
                put_fpr(ofs + 4, oy);
                put_fpr(ofs + 8, oz);
            }
        }
        const u32 pos = g13::g::particle_pos + 16 * n;
        const f64 wx = sgl(px + ox);
        put_fpr(pos, wx);
        const f64 wy = sgl(py + lfs(ofs + 4));
        put_fpr(pos + 4, wy);
        const f64 wz = sgl(pz + lfs(ofs + 8));
        put_fpr(pos + 8, wz);
        const u32 pos2 = g18::g::particle_pos2 + 16 * n;
        {
            const V4 r = transform(lvx(mat), lvx(mat + 16), lvx(mat + 32), lvx(mat + 48), static_cast<f32>(wx),
                                   static_cast<f32>(wy), static_cast<f32>(wz));
            for (u32 k = 0; k < 4; ++k) put_f32(pos2 + 4 * k, r.v[k]);  // stvx
        }
        const f64 depth = lfs(pos2 + 8);
        if (depth > lfs(rec + L::kRecFar)) continue;
        if (depth < lfs(rec + L::kRecNear)) continue;

        // Occluders: hidden inside any of the boxes (|transformed| <= half extents).
        if (static_cast<s32>(occ_count) > 0) {
            const f32 qx = f32_at(pos2), qy = f32_at(pos2 + 4), qz = f32_at(pos2 + 8);  // lvlx, vspltw 0
            bool inside = false;
            u32 j = 0;
            do {
                const u32 b = occ + L::kOccluderSize * j;
                const V4 q = transform(lvx(b + 16), lvx(b + 32), lvx(b + 48), lvx(b + 64), qx, qy, qz);
                const V4 ext = lvx(b);
                inside = true;  // vsldoi'd lane 0 compares 0 >= 0
                for (u32 k = 0; k < 3; ++k)
                    if (!(ext.v[k] >= std::fabs(q.v[k]))) inside = false;  // vandc sign, vcmpgefp.
                j = (j + 1) & 0xFFFF;
            } while (static_cast<s32>(j) < static_cast<s32>(occ_count) && !inside);
            if (inside) continue;
        }

        // Size.
        f64 sz = one;
        if (size != 0) {
            const f64 b = sgl(cvt(load<u8>(cur + L::kPartSize)) * k255);
            const f64 at = sgl(rate_age * cvt(static_cast<u32>(load<u32>(size + 8) - 1)));
            const u32 k = static_cast<u32>(fctiwz(at));
            const f64 a0 = lfs(size + 16 + 4 * k), a1 = lfs(size + 16 + 4 * (k + 1));
            sz = sgl(b * madds(sgl(a1 - a0), sgl(at - trunc_s(at)), a0));
        }
        put_fpr(g13::g::particle_scale + 4 * n, sz);  // stfsx

        // UV rect {u0, u1, 0, 1}.
        const u32 rect = g18::g::particle_uv + 16 * n;
        const u32 mode = uv != 0 ? load<u8>(uv + 20) : 0;
        if (uv == 0 || mode == 0 || mode > 4) {
            store<u32>(rect, 0);
            store<u32>(rect + 4, 0x3F800000u);
            store<u32>(rect + 8, 0);
            store<u32>(rect + 12, 0x3F800000u);
        } else {
            const u32 rnd = load<u32>(cur + L::kPartRandom);
            f64 frame_u;
            if (mode == 1) {
                frame_u = trunc_s(sgl(rate_age * lfs(uv + 16)));
            } else if (mode == 2) {
                f64 f = sgl(age * lfs(uv + 28));
                f = sgl(sgl(f - trunc_s(f)) * lfs(uv + 16));
                frame_u = trunc_s(f);
            } else if (mode == 3) {  // ping-pong
                f64 f = sgl(age * lfs(uv + 28));
                f = sgl(sgl(f - trunc_s(f)) * lfs(uv + 32));
                const f64 fr = trunc_s(f);
                const f64 back = sgl(sgl(sgl(fr - lfs(uv + 16)) + one) * two);
                frame_u = sgl(fr - (back < zero_f ? zero_f : back));
            } else {  // 4: floorf(random / 255 * frames), inlined
                const f64 f = sgl(sgl(cvt(rnd & 0xFF) * k255) * lfs(uv + 16));
                const s32 w = fctiwz(f);
                const f64 tr = trunc_s(f);
                frame_u = f;
                if (stfs_bits(sgl(tr - f)) != 0) {
                    const u32 bits = stfs_bits(f);
                    if (static_cast<s32>(bits & 0x7F800000u) < 0x4B800000) {  // rlwinm 0,1,8; below 2^24
                        if ((bits & 0x80000000u) == 0) frame_u = tr;
                        else frame_u = cvt(static_cast<s32>(static_cast<u32>(w) - 1u));
                    }
                }
            }
            const f64 u0 = sgl(frame_u * lfs(uv + 24));
            store<u32>(rect + 8, 0);
            store<u32>(rect + 12, 0x3F800000u);
            put_fpr(rect, u0);
            put_fpr(rect + 4, sgl(sgl(u0 + lfs(uv + 12)) - eps));
        }

        // Spin matrix.
        const u32 mtx = g13::g::particle_mtx + 64 * n;
        u32 r4 = 0;  // r4 as the original leaves it for Spline_TangentAt (unread there)
        if (spin == 0) {
            for (u32 r = 0; r < 4; ++r)
                for (u32 k = 0; k < 12; k += 4) store<u32>(mtx + 16 * r + k, load<u32>(g11::g::identity + 16 * r + k));
            r4 = 0x01160000u;
        } else {
            const f64 rate = cvt(static_cast<s8>(load<u8>(cur + L::kPartSpinRate)));
            const f64 phase = cvt(static_cast<s8>(load<u8>(cur + L::kPartSpinPhase)));
            const f64 at = sgl(rate_age * cvt(static_cast<u32>(load<u32>(spin + 36) - 1)));
            const u32 k = static_cast<u32>(fctiwz(at));
            const f64 spread = sgl(lfs(spin + 172) * half);
            const f64 a0 = lfs(spin + 40 + 4 * k), a1 = lfs(spin + 40 + 4 * (k + 1));
            const f64 ph = sgl(g11::msubs(phase, k127, spread) * two_pi);
            f64 ang = madds(sgl(a1 - a0), sgl(at - trunc_s(at)), a0);
            ang = madds(ang, sgl(rate * k127), ph);
            const u32 axis = load<u8>(spin + 176);
            r4 = mtx;
            if (axis <= 3) {
                if (axis == 0) callf(ctx, L::fn::Matrix_RotationX, {mtx}, {ang});
                else if (axis == 1) callf(ctx, L::fn::Matrix_RotationY, {mtx}, {ang});
                else if (axis == 2) callf(ctx, L::fn::Matrix_RotationZ, {mtx}, {ang});
                else callf(ctx, L::fn::Matrix_FromAxisAngle, {mtx, spin + 16}, {ang});
                r4 = static_cast<u32>(ctx.gpr[4]);
            }
        }
        if (load<u8>(rec + L::kRecLayered) == 1) {
            r4 = load<u32>(cur + L::kPartRandom);
            store<u8>(g13::g::particle_layer + n, static_cast<u8>(r4));
        }

        // Direction: the spline's tangent, or v + a t.
        const u32 dir = g18::g::particle_dir + 16 * n;
        const u32 spline = load<u32>(load<u32>(e + 4) + 136);
        if (spline != 0) {
            callf(ctx, L::fn::Spline_TangentAt, {spline + 16, r4, dir, 0}, {sgl(lfs(rec + L::kRecRate) * age)});
        } else {
            const f64 d = sgl(age - lfs(motion + 32));
            const f64 tt = sgl(d >= 0.0 ? d : zero_d);
            put_fpr(dir, madds(lfs(motion + 16), tt, vx));
            put_fpr(dir + 4, madds(lfs(motion + 20), tt, vy));
            put_fpr(dir + 8, madds(lfs(motion + 24), tt, vz));
        }
        if (++n > L::kMaxParticleIndex) break;  // cmplwi
    } while (p != 0);
    store<u32>(rec + L::kRecCount, n);
}


// void Obj23D50_FitBox_0052BC9C(Obj23D50* o, const f32 box[8] {min, -,
// max, -}, const f32 m[4][4])   [Obj23D50_FitBoxes_0052C6B8] (name?)
// Marks the box's screen area in the stencil buffer: the box as a
// 14-vertex triangle strip (inline data) with m times the current
// view's +880 matrix (transposed) as vertex constants 0..3, the shader
// group 2 programs, stencil on with ALWAYS / 255 and front INCR_WRAP /
// back DECR_WRAP on depth pass, two-sided, depth test as render state
// +93 says with LESS and no depth write, colour writes off, culling off.
// Then the stencil state back to off / KEEP, depth func LEQUAL with the
// depth mask as +93 bit 0, colour RGBA, culling on, front face CW. o is
// not read. Returns r3 as left (the last method word).
DECOMP(0052BC9C, Obj23D50_FitBox_0052BC9C) {
    namespace L = g20;
    using sly4::fx::emit;
    const u32 box = arg<u32>(ctx, 1), m = arg<u32>(ctx, 2);
    const u32 st = sly4::fx::g::state;

    // Stack: +0 the transposed matrix (64 bytes), +64 the strip (14 x 12).
    Stack frame(ctx, 240);
    const u32 consts = frame.addr(), strip = frame.addr() + 64;
    for (u32 i = 0; i < 14; ++i)
        for (u32 a = 0; a < 3; ++a)  // lfs / stfs pairs: min at box +0.., max at +16..
            store<u32>(strip + 12 * i + 4 * a, load<u32>(box + ((L::kBoxStrip[i] >> (2 - a)) & 1) * 16 + 4 * a));
    {
        const u32 vp = load<u32>(load<u32>(sly4::render::gfx15::g::current_surface) + 672) + L::kViewViewProj;
        const V4 p0 = lvx(vp), p1 = lvx(vp + 16), p2 = lvx(vp + 32), p3 = lvx(vp + 48);
        V4 r[4];
        for (u32 i = 0; i < 4; ++i) {
            const V4 mi = lvx(m + 16 * i);
            for (u32 k = 0; k < 4; ++k) {  // row 3 starts from p3 (vmaddfp's addend)
                f32 v = std::fmaf(p0.v[k], mi.v[0], i == 3 ? p3.v[k] : 0.0f);
                v = std::fmaf(p1.v[k], mi.v[1], v);
                r[i].v[k] = std::fmaf(p2.v[k], mi.v[2], v);
            }
        }
        for (u32 k = 0; k < 4; ++k)
            for (u32 i = 0; i < 4; ++i) put_f32(consts + 16 * k + 4 * i, r[i].v[k]);  // vmrghw / vmrglw transpose
    }

    const u32 mat = static_cast<u32>(call(ctx, L::fn::Shader_LoadGroup, {L::kBoxShaderGroup}));
    const u32 progs = load<u32>(load<u32>(load<u32>(mat + 16) + 12));
    auto emit_func = [&](u32 method, u32 o1, u32 o2, u32 o3) {
        emit({method, load<u32>(st + o1), load<u32>(st + o2), load<u32>(st + o3)});
    };
    emit({sly4::fx::kStencilTestEnable, 1});
    store<u32>(st + 176, L::kGcmKeep);  // front op: fail, z-fail, z-pass
    store<u32>(st + 180, L::kGcmIncrWrap);
    store<u32>(st + 184, L::kGcmKeep);
    store<u32>(st + 160, L::kGcmAlways);  // front func, ref, mask
    store<u32>(st + 164, 0);
    store<u32>(st + 172, 0);
    store<u32>(st + 168, 255);  // front write mask
    emit({sly4::render::pool::kStencilOp, L::kGcmKeep, L::kGcmIncrWrap, L::kGcmKeep});
    emit({sly4::render::pool::kStencilMask, 255});
    emit_func(sly4::render::pool::kStencilFunc, 160, 164, 172);
    emit({sly4::render::pool::kTwoSidedStencil, 1});
    store<u32>(st + 196, 255);
    store<u32>(st + 204, L::kGcmKeep);  // back op
    store<u32>(st + 208, L::kGcmDecrWrap);
    store<u32>(st + 212, L::kGcmKeep);
    store<u32>(st + 188, L::kGcmAlways);  // back func
    store<u32>(st + 192, 0);
    store<u32>(st + 200, 0);
    emit({sly4::render::pool::kBackStencilOp, L::kGcmKeep, L::kGcmDecrWrap, L::kGcmKeep});
    emit({sly4::render::pool::kBackStencilMask, load<u32>(st + 196) & 0xFF});
    emit_func(sly4::render::pool::kBackStencilFunc, 188, 192, 200);
    emit({sly4::fx::kDepthTestEnable, load<u8>(st + 93)});
    emit({sly4::fx::kDepthFunc, L::kGcmLess});
    emit({sly4::fx::kDepthMask, 0});
    emit({sly4::fx::kBlendEnable, 0});
    emit({sly4::fx::kColorMask, 0});
    emit({sly4::fx::kCullFaceEnable, 0});

    // The programs (render state +20 / +24 vertex program and ucode, +28 /
    // +92 bit 31 fragment program), as the draws bind them.
    const u32 vprog = load<u32>(progs + 4);
    if (load<u32>(st + 20) != vprog) {
        store<u32>(st + 20, vprog);
        store<u32>(st + 8, 0xFFFFFFFFu);
        const u32 ucode = load<u32>(vprog + 4);
        if (load<u32>(st + 24) != ucode) {
            store<u32>(st + 24, ucode);
            call(ctx, L::fn::Gcm_SetVertexProgramAt, {load<u32>(st), load<u32>(vprog + 20)});
        }
    }
    const u32 fprog = load<u32>(progs + 8);
    if (load<u32>(st + 28) != fprog) {
        const u32 w92 = load<u32>(st + 92);
        store<u32>(st + 28, fprog);
        store<u32>(st + 92, w92 | 0x80000000u);
    }
    call(ctx, sly4::fx::fn::cellGcmSetVertexProgramConstants, {load<u32>(st), 0, 16, consts});
    sly4::render::pool::upload_fragment(ctx);

    // The vertex format, sent when the cached vertex data (+8) is set; the
    // strip goes inline (offsets | 0x80000000).
    const u32 vf = L::kBoxFormat;
    const u32 stride = load<u16>(vf + 120);
    if (load<u32>(st + 8) != 0) {
        store<u32>(st + 8, 0);
        sly4::render::pool::send_attributes(vf, stride, load<u16>(vf + 126), 0, 0x80000000u);
    }
    const u32 stride2 = load<u16>(vf + 120);
    call(ctx, L::fn::cellGcmSetDrawInlineArray, {load<u32>(st), L::kPrimTriangleStrip, (14 * stride2) >> 2, strip});

    emit({sly4::fx::kStencilTestEnable, 0});
    store<u32>(st + 176, L::kGcmKeep);
    store<u32>(st + 180, L::kGcmKeep);
    store<u32>(st + 184, L::kGcmKeep);
    store<u32>(st + 160, L::kGcmAlways);
    store<u32>(st + 164, 0);
    store<u32>(st + 172, 0);
    store<u32>(st + 168, 0);
    emit({sly4::render::pool::kStencilOp, L::kGcmKeep, L::kGcmKeep, L::kGcmKeep});
    emit({sly4::render::pool::kStencilMask, 0});
    emit_func(sly4::render::pool::kStencilFunc, 160, 164, 172);
    emit({sly4::render::pool::kTwoSidedStencil, 0});
    store<u32>(st + 196, 0);
    store<u32>(st + 204, L::kGcmKeep);
    store<u32>(st + 208, L::kGcmKeep);
    store<u32>(st + 212, L::kGcmKeep);
    store<u32>(st + 188, L::kGcmAlways);
    store<u32>(st + 192, 0);
    store<u32>(st + 200, 0);
    emit({sly4::render::pool::kBackStencilOp, L::kGcmKeep, L::kGcmKeep, L::kGcmKeep});
    emit({sly4::render::pool::kBackStencilMask, load<u32>(st + 196) & 0xFF});
    emit_func(sly4::render::pool::kBackStencilFunc, 188, 192, 200);
    emit({sly4::fx::kDepthTestEnable, load<u8>(st + 93)});
    emit({sly4::fx::kDepthFunc, L::kGcmLequal});
    emit({sly4::fx::kDepthMask, static_cast<u32>(load<u8>(st + 93) & 1)});
    emit({sly4::fx::kBlendEnable, 1});
    emit({sly4::fx::kColorMask, 0x01010101u});  // R, G, B, A
    emit({sly4::fx::kCullFaceEnable, 1});
    emit({sly4::render::layers4::kFrontFace, L::kGcmCw});
    ctx.fpr[1] = lfs(box);  // f1 as left: the box's min x
    ret(ctx, sly4::render::layers4::kFrontFace);
}

// void DebugFont_Draw_00565550(DebugFont* font, FrameBuffer* fb, const
// f32 pos[3], const char* text, const u8 colour[4]; f32 size)
// [Debug_Draw_0056AEFC's text prims] (name?)
// The text as 6-vertex quads (Draw_BeginPrims type 5 for 6 * (length -
// tabs) vertices; the r7 it reads is the one strchr leaves): each glyph
// (code past the font's last or with index 0: glyph 0) spans x .. x +
// size * width, y .. y + size * line height, its texel rect over the
// texture size; x advances by size * advance; a tab moves x to the next
// multiple of 30 above it. Draw_EndPrims after; nothing without visible
// characters (r3 0 then).
DECOMP(00565550, DebugFont_Draw_00565550) {
    namespace L = g20;
    const u32 font = arg<u32>(ctx, 0), fb = arg<u32>(ctx, 1), pos = arg<u32>(ctx, 2), colour = arg<u32>(ctx, 4);
    u32 text = arg<u32>(ctx, 3);
    const f64 size = farg(ctx, 0);
    f64 x = lfs(pos);
    const f64 y = lfs(pos + 4);
    u32 n = static_cast<u32>(call(ctx, L::fn::strlen, {text}));
    for (u32 t = static_cast<u32>(call(ctx, L::fn::strchr, {text, 9})); t != 0;
         t = static_cast<u32>(call(ctx, L::fn::strchr, {t + 1, 9})))
        --n;  // tabs draw nothing
    if (n == 0) return ret(ctx, 0u);  // r3: strchr's 0

    Stack frame(ctx, 16);
    const u32 prims = frame.addr();  // {+0 vertex cursor, +4 u16 count, +6 u16 written}
    call(ctx, L::fn::Draw_BeginPrims, {prims, fb, L::kPrimQuadsAsTriangles, static_cast<u64>(static_cast<s64>(static_cast<s32>(6 * n)))});
    if (load<u8>(text) != 0) {
        u32 cur = load<u32>(prims);
        u32 written = load<u16>(prims + 6);
        f64 f1 = 0.0;
        do {
            const s32 c = static_cast<s8>(load<u8>(text));
            ++text;
            if (c == 9) {
                const f64 w = lfs(L::kFontConsts + 4);
                f64 stop = w;
                f1 = w;
                while (x >= stop) stop = sgl(stop + w);
                x = stop;
                continue;
            }
            u32 gl = load<u32>(font + L::kFontGlyphs);
            if (!(static_cast<u32>(c) > load<u16>(font + L::kFontLastCode))) {  // cmplw
                const u32 ix = load<u16>(load<u32>(font + L::kFontIndex) + 2 * static_cast<u32>(c));
                if (ix != 0) gl = load<u32>(font + L::kFontGlyphs) + L::kGlyphSize * static_cast<u32>(static_cast<s16>(ix));
            }
            auto s16_at = [](u32 a) { return cvt(static_cast<s16>(load<u16>(a))); };
            const u32 tex = load<u32>(font + L::kFontTexture);
            const f64 one = lfs(L::kFontConsts);
            const f64 inv_w = sgl(one / cvt(load<u16>(tex + 52))), inv_h = sgl(one / cvt(load<u16>(tex + 54)));
            const f64 x0 = x, x1 = madds(size, s16_at(gl + 10), x);
            const f64 y1 = madds(size, lfs(font + L::kFontLineHeight), y);
            const f64 adv = s16_at(gl + 12);
            x = madds(size, adv, x);
            f1 = adv;
            const f64 u0 = sgl(s16_at(gl) * inv_w), u1 = sgl(s16_at(gl + 4) * inv_w);
            const f64 v0 = sgl(s16_at(gl + 2) * inv_h), v1 = sgl(s16_at(gl + 6) * inv_h);
            const f64 quad[6][4] = {{x1, y, u1, v0}, {x0, y, u0, v0}, {x0, y1, u0, v1},
                                    {x1, y, u1, v0}, {x0, y1, u0, v1}, {x1, y1, u1, v1}};
            for (u32 k = 0; k < 6; ++k) {
                const u32 v = cur + L::kDebugVertexSize * k;
                put_fpr(v, quad[k][0]);
                put_fpr(v + 4, quad[k][1]);
                store<u32>(v + 8, 0xBF800000u);  // z -1
                put_fpr(v + 16, quad[k][2]);
                put_fpr(v + 20, quad[k][3]);
                for (u32 b = 0; b < 4; ++b) store<u8>(v + 12 + b, load<u8>(colour + b));
            }
            cur += 6 * L::kDebugVertexSize;
            written = (written + 6) & 0xFFFF;
            store<u32>(prims, cur);
            store<u16>(prims + 6, static_cast<u16>(written));
        } while (load<u8>(text) != 0);
        ctx.fpr[1] = f1;
    }
    call(ctx, L::fn::Draw_EndPrims, {fb});
}

// f32* Plane_TransformInverse_0055EA6C(f32 plane[4], const f32 m[4][4])
// [LightCam_Setup] (name?)
// The plane through m: plane = plane * inverse(transpose(m)) (m's rows
// read with lvx, transposed, Mat4_Invert, Vec4_TransformByMatrix of a
// copy). Returns plane; f1 = its d as passed to the transform.
DECOMP(0055EA6C, Plane_TransformInverse_0055EA6C) {
    namespace L = g20;
    const u32 plane = arg<u32>(ctx, 0), m = arg<u32>(ctx, 1);
    Stack frame(ctx, 208);  // +0 the plane's copy, +16 Mat4_Invert's scratch, +80 transpose(m), +144 its inverse
    const u32 copy = frame.addr(), tmp = copy + 16, tr = copy + 80, inv = copy + 144;
    const V4 r[4] = {lvx(m), lvx(m + 16), lvx(m + 32), lvx(m + 48)};
    for (u32 k = 0; k < 4; ++k)
        for (u32 i = 0; i < 4; ++i) put_f32(tr + 16 * k + 4 * i, r[i].v[k]);  // vmrghw / vmrglw
    call(ctx, L::fn::Mat4_Invert, {tmp, inv, tr});
    for (u32 k = 0; k < 16; k += 4) store<u32>(copy + k, load<u32>(plane + k));  // lfs / stfs pairs
    ctx.fpr[1] = lfs(plane + 12);
    call(ctx, L::fn::Vec4_TransformByMatrix, {plane, copy, inv});
    ret(ctx, plane);
}

// u32 LoadProgress_Update_004FB5E0(?, s32 stage, s32 done, s32 total)
// [a callback; first argument unread] (name?)
// The loading bar: stages 5 and 7 weigh 10, stage 6 80 (others 0: no
// change); stage <= 0 or > 9 marks the loading screen (+94 = 1). The
// progress shown is base + weight * done / total; the base takes it
// when done == total, and the value shown only moves up by whole steps.
// Returns 0; f1 as left.
DECOMP(004FB5E0, LoadProgress_Update_004FB5E0) {
    namespace L = g20;
    const s32 stage = arg<s32>(ctx, 1), done = arg<s32>(ctx, 2), total = arg<s32>(ctx, 3);
    const u32 C = L::kProgressConsts;
    const f64 none = lfs(C);
    f64 w = none;
    if (stage == 5 || stage == 7) w = lfs(C + 28);
    else if (stage == 6) w = lfs(C + 24);
    else if (stage <= 0 || stage > 9) store<u8>(load<u32>(L::g::load_screen) + L::kLoadScreenUnknownStage, 1);
    ctx.fpr[1] = w;
    if (w == none) return ret(ctx, 0u);
    const f64 p = madds(sgl(cvt(done) / cvt(total)), w, lfs(L::g::load_progress));
    if (done == total) put_fpr(L::g::load_progress, p);
    u32 step = 0;
    const f64 shown = lfs(L::g::load_shown);
    if (shown < p) step = fctidz_lo(sgl(p - shown));
    if (step != 0) put_fpr(L::g::load_shown, p);
    ctx.fpr[1] = p;
    ret(ctx, 0u);
}

// void Records_AddList_0058CD44(Obj* o, Item* const* items, u32 count)
// [a vtable slot] (name?)
// The view set from o's virtual slot 13 (+52)(o, record set id, 0), then
// Records_AddToViews(o, the item's resource entry (handle table base +
// 128 * item->+0 ->+284 u16; 0 for 0xFFFF), set) per item. The loop runs
// once even for a count of 0 (the original's do-while).
DECOMP(0058CD44, Records_AddList_0058CD44) {
    namespace L = g20;
    const u32 o = arg<u32>(ctx, 0), count = arg<u32>(ctx, 2);
    u32 items = arg<u32>(ctx, 1);
    const u32 set = static_cast<u32>(sly4::fx::vcall(ctx, o, 52, {o, load<u16>(L::kRecordSet + 12), 0}));
    const u32 end = items + 4 * count;
    do {
        const u32 h = load<u16>(load<u32>(items) + 284);
        const u32 res = h == L::kNoHandle ? 0u : load<u32>(L::kHandleTable) + (h << 7);
        call(ctx, L::fn::Records_AddToViews, {o, res, set});
        items += 4;
    } while (items < end);  // cmplw
}

// bool Quads_ReadCorners_0059B388(IffReader* r)   [a chunk handler] (name?)
// Each bank quad's four corners (FileReader_ReadVec3Swapped, xyz at +16
// * k). Returns 1.
DECOMP(0059B388, Quads_ReadCorners_0059B388) {
    namespace L = g20;
    const u32 r = arg<u32>(ctx, 0);
    u32 q = load<u32>(L::kBankQuads);
    call(ctx, L::fn::Iff_ChunkSize, {r});
    if (load<u16>(L::kBankQuadCount) != 0) {
        Stack frame(ctx, 16);
        const u32 v = frame.addr();
        u32 i = 0;
        do {
            for (u32 k = 0; k < 4; ++k) {
                call(ctx, L::fn::FileReader_ReadVec3Swapped, {r, v, 0});
                for (u32 a = 0; a < 12; a += 4) store<u32>(q + 16 * k + a, load<u32>(v + a));  // lfs / stfs
                ctx.fpr[1] = lfs(v);  // f1 as left: the x just read
            }
            ++i;
            q += L::kQuadSize;
        } while (i < load<u16>(L::kBankQuadCount));
    }
    ret(ctx, 1u);
}

// bool Quads_ReadNames_0059B648(IffReader* r)   [a chunk handler] (name?)
// Each bank quad's name: a 64-byte record, its first min(length, 31)
// characters copied to quad +70. Returns 1.
DECOMP(0059B648, Quads_ReadNames_0059B648) {
    namespace L = g20;
    const u32 r = arg<u32>(ctx, 0);
    u32 q = load<u32>(L::kBankQuads);
    call(ctx, L::fn::Iff_ChunkSize, {r});
    if (load<u16>(L::kBankQuadCount) != 0) {
        Stack frame(ctx, L::kQuadNameRecord);
        const u32 buf = frame.addr();
        u32 i = 0;
        do {
            call(ctx, L::fn::FileReader_Read, {r, buf, L::kQuadNameRecord});
            const u32 len = static_cast<u32>(call(ctx, L::fn::String_Length, {buf}));
            const u32 n = static_cast<s32>(len) < static_cast<s32>(L::kQuadNameMax) ? len : L::kQuadNameMax;  // cmpwi
            call(ctx, L::fn::String_CopyN, {q + L::kQuadName, buf, n});
            ++i;
            q += L::kQuadSize;
        } while (i < load<u16>(L::kBankQuadCount));
    }
    ret(ctx, 1u);
}

// Collector* Tree_Collect_0058E990(Collector* c, Node* node) (name?)
// Appends node and its siblings (+272), each followed by its children
// (+268, recursively), to c's PtrVector (doubling it when full): each
// node's u16 +4 is counted up and its flags bits 6..9 set to c's level
// (u8 +56); c's u16 +46 keeps the largest count reached. Returns c.
DECOMP(0058E990, Tree_Collect_0058E990) {
    namespace L = g20;
    const u32 c = arg<u32>(ctx, 0);
    u32 r3 = c;  // what the last recursive call returns (c)
    for (u32 node = arg<u32>(ctx, 1); node != 0; node = load<u32>(node + L::kNodeNext)) {
        store<u16>(node + L::kNodeVisits, static_cast<u16>(load<u16>(node + L::kNodeVisits) + 1));
        u32 n = load<u16>(c + L::kCollCount);
        const u32 cap = load<u16>(c + L::kCollCapacity);
        if (n == cap) {
            call(ctx, L::fn::PtrVector_Reserve, {c + L::kCollVector, cap << 1});
            n = load<u16>(c + L::kCollCount);
        }
        const u32 data = load<u32>(c + L::kCollVector);
        const u32 n1 = load<u16>(c + L::kCollCount) + 1;
        const u32 peak = load<u16>(c + L::kCollPeak);
        store<u32>(data + 4 * n, node);
        const u32 level = load<u8>(c + L::kCollLevel);
        store<u16>(c + L::kCollCount, static_cast<u16>(n1));
        const u32 child = load<u32>(node + L::kNodeChild);
        store<u16>(c + L::kCollPeak, static_cast<u16>(static_cast<s32>(peak) > static_cast<s32>(n1 & 0xFFFF) ? peak : n1));
        store<u32>(node + L::kNodeFlags, (load<u32>(node + L::kNodeFlags) & ~0x3C0u) | ((level << 6) & 0x3C0u));  // rlwimi 6,22,25
        r3 = static_cast<u32>(call(ctx, L::fn::Tree_Collect_0058E990, {c, child}));
    }
    ret(ctx, r3);
}

// bool GeomDef_ReadChunks_0059B788(IffReader* r)   [a chunk handler] (name?)
// A new GeomDef read from the chunk's children, between the geometry
// class's load hook calls (0 / 1). With LODs given: each LOD object
// marked (SetFlags54_0050A214(obj, 1, 0, 0)), the count dropped by one
// when the last LOD is the final one, the last LOD defaulting to count -
// 1. Without LODs it is deleted (virtual slot 2); otherwise it goes in
// the class's named map under its +96 name (doubling the map when
// full) and gets flag 0x400. Returns 1.
DECOMP(0059B788, GeomDef_ReadChunks_0059B788) {
    namespace L = g20;
    const u32 r = arg<u32>(ctx, 0);
    const u32 g = static_cast<u32>(call(ctx, L::fn::GeomDef_New, {r}));
    auto hook = [&](u32 phase) {
        const u32 cls = load<u32>(L::kGeomClass);
        const u32 opd = load<u32>(cls + 8);
        if (opd != 0) call_ptr(ctx, opd, {g, load<u32>(cls + 12), phase, opd, load<u32>(opd)});
    };
    hook(0);
    call(ctx, L::fn::Iff_ReadChunks, {r, g});
    if (load<u8>(g + 113) != 0) {
        u32 n = load<u8>(g + 112);
        if (n != 0) {
            for (u32 i = 0; i < n; i = (i + 1) & 0xFF)  // cmplw
                call(ctx, L::fn::SetFlags54_0050A214, {load<u32>(g + 132 + 8 * i), 1, 0, 0});
            n = load<u8>(g + 112);
        }
        if (static_cast<s32>(n) > 1 && static_cast<s32>(load<u8>(g + 115)) == static_cast<s32>(n - 1)) {
            --n;
            store<u8>(g + 112, static_cast<u8>(n));
        }
        if (load<u8>(g + 115) == 0 && (n & 0xFF) != 0) store<u8>(g + 115, static_cast<u8>(n - 1));
    }
    hook(1);
    if (load<u8>(g + 112) == 0) {
        sly4::fx::vcall(ctx, g, 8, {g});  // delete
        return ret(ctx, 1u);
    }
    const u32 map = load<u32>(load<u32>(L::kGeomClass) + 16);
    const u32 name = load<u32>(g + 96);
    if (load<u16>(map + 2) == load<u16>(map)) call(ctx, L::fn::Array_Reserve, {map, (load<u16>(map) << 1) & 0xFFFEu});
    const u32 data = load<u32>(map + 8);
    store<u32>(data + 8 * load<u16>(map + 2), name);
    store<u32>(data + 8 * load<u16>(map + 2) + 4, g);
    const u32 n1 = load<u16>(map + 2) + 1, peak = load<u16>(map + 4), flags = load<u16>(map + 6);
    store<u16>(map + 2, static_cast<u16>(n1));
    store<u16>(map + 4, static_cast<u16>(static_cast<s32>(peak) > static_cast<s32>(n1 & 0xFFFF) ? peak : n1));
    store<u16>(map + 6, static_cast<u16>(flags | 1));
    store<u32>(g + 8, load<u32>(g + 8) | L::kGeomRegistered);
    ret(ctx, 1u);
}

// void Quads_ClipSetup_00578F64(Clip* out, const f32 p0[4], p1[4], p2[4],
// p3[4], p4[4], p5[4])   [Quads_Prepare] (name?)
// out +0: the six planes; +96: the same with the frustum pad plane
// twice, as two 4x4 blocks transposed in place (stvx, 16-byte aligned);
// +224: the eight corners (Plane_Intersect3 of each plane triple),
// negated. f1 = the last corner's x as stored; r3 as Plane_Intersect3
// leaves it.
DECOMP(00578F64, Quads_ClipSetup_00578F64) {
    namespace L = g20;
    const u32 out = arg<u32>(ctx, 0);
    u32 planes[6];
    for (u32 k = 0; k < 6; ++k) planes[k] = arg<u32>(ctx, 1 + k);
    for (u32 rep = 0; rep < 2; ++rep)
        for (u32 k = 0; k < 6; ++k)
            for (u32 a = 0; a < 16; a += 4) store<u32>(out + 96 * rep + 16 * k + a, load<u32>(planes[k] + a));  // lfs / stfs
    for (u32 rep = 0; rep < 2; ++rep)
        for (u32 a = 0; a < 16; a += 4) store<u32>(out + 192 + 16 * rep + a, load<u32>(L::kFrustumPad + a));
    for (u32 blk = 96; blk <= 160; blk += 64) {  // vmrghw / vmrglw transpose
        u32 m[4][4];
        for (u32 r = 0; r < 4; ++r)
            for (u32 c = 0; c < 4; ++c) m[r][c] = load<u32>(((out + blk + 16 * r) & ~15u) + 4 * c);
        for (u32 r = 0; r < 4; ++r)
            for (u32 c = 0; c < 4; ++c) store<u32>(((out + blk + 16 * r) & ~15u) + 4 * c, m[c][r]);
    }
    for (u32 k = 0; k < 8; ++k) {
        const u32 t = L::kCornerPlanes + 12 * k, pt = out + 224 + 16 * k;
        call(ctx, L::fn::Plane_Intersect3,
             {pt, out + (load<u32>(t) << 4), out + (load<u32>(t + 4) << 4), out + (load<u32>(t + 8) << 4)});
        u32 w[3];
        for (u32 a = 0; a < 3; ++a) w[a] = load<u32>(pt + 4 * a) ^ 0x80000000u;  // fneg
        for (u32 a = 0; a < 3; ++a) store<u32>(pt + 4 * a, w[a]);
        ctx.fpr[1] = lfs(pt);
    }
}

// u32 Node_SyncTree_0058E680(Scene* s, Node* n) (name?)
// An emitter node whose def asks for it gets a fresh copy of the def
// (EmitterDef_Copy(def, 0) handed to n's virtual slot 11, then the
// copy's slot 2). Recurses into the first child and the next sibling.
// A node following an attached transform (flags bits 6..7, +260) takes
// its rotation rows transposed from the transform's 3x3 (+80) and its
// position as is (rewritten in place); when not yet dirty, its parent
// is queued on the dirty list (unless already queued or dirty, attached
// itself or the list is off) and both are marked dirty. Returns r3 = n's
// flags as left.
DECOMP(0058E680, Node_SyncTree_0058E680) {
    namespace L = g20;
    const u32 s = arg<u32>(ctx, 0), n = arg<u32>(ctx, 1);
    if (((load<u32>(n + 4) >> 10) & 0x3F) == L::kNodeKindEmitter) {  // rlwinm 22,26,31
        const u32 def = load<u32>(n + L::kNodeDef);
        if ((load<u8>(load<u32>(def + 112) + 42) & 0x80) != 0) {
            const u32 copy = static_cast<u32>(call(ctx, L::fn::EmitterDef_Copy, {def, 0}));
            sly4::fx::vcall(ctx, n, 44, {n, copy});
            sly4::fx::vcall(ctx, copy, 8, {copy});
        }
    }
    if (const u32 c = load<u32>(n + L::kNodeChild); c != 0) call(ctx, L::fn::Node_SyncTree_0058E680, {s, c});
    if (const u32 c = load<u32>(n + L::kNodeNext); c != 0) call(ctx, L::fn::Node_SyncTree_0058E680, {s, c});
    const u32 f = load<u32>(n + L::kNodeState);
    const u32 at = load<u32>(n + L::kNodeAttach);
    if (((f >> 6) & 3) != 0 && at != 0) {
        const u32 m = at + 80;
        u32 w[12];
        for (u32 r = 0; r < 3; ++r)
            for (u32 c = 0; c < 3; ++c) w[3 * r + c] = load<u32>(m + 16 * c + 4 * r);  // lvlx / vmrghw / vperm transpose
        for (u32 a = 0; a < 3; ++a) w[9 + a] = load<u32>(n + L::kNodePos + 4 * a);
        for (u32 r = 0; r < 3; ++r)
            for (u32 c = 0; c < 3; ++c) store<u32>(n + L::kNodeRot + 16 * r + 4 * c, w[3 * r + c]);
        for (u32 a = 0; a < 3; ++a) store<u32>(n + L::kNodePos + 4 * a, w[9 + a]);  // lfs / stfs in place
        ctx.fpr[1] = lfs(m);  // f1 as left: the first element
        if (((f >> 11) & 1) == 0) {
            const u32 q = load<u32>(n + L::kNodeParent);
            u32 qf = load<u32>(q + L::kNodeState);
            if (((qf >> 11) & 1) == 0) {
                if (((qf >> 10) & 1) == 0 && load<u32>(q + L::kNodeAttach) == 0 && load<u8>(L::kDirtyListOff) == 0) {
                    const u32 list = load<u32>(L::kDirtyList);
                    u32 cnt = load<u16>(list + 8);
                    const u32 cap = load<u16>(list + 4);
                    if (cnt == cap) {
                        call(ctx, L::fn::PtrVector_Reserve, {list, (cap << 1) & ~1u});
                        cnt = load<u16>(list + 8);
                    }
                    const u32 data = load<u32>(list);
                    const u32 c1 = load<u16>(list + 8) + 1, peak = load<u16>(list + 10);
                    store<u32>(data + 4 * cnt, q);
                    store<u16>(list + 8, static_cast<u16>(c1));
                    store<u16>(list + 10, static_cast<u16>(static_cast<s32>(peak) > static_cast<s32>(c1 & 0xFFFF) ? peak : c1));
                    qf = load<u32>(q + L::kNodeState) | L::kStateQueued;
                }
                store<u32>(q + L::kNodeState, qf | L::kStateDirty);
                store<u32>(n + L::kNodeState, load<u32>(n + L::kNodeState) | L::kStateDirty);
            } else {
                store<u32>(n + L::kNodeState, f | L::kStateDirty);
            }
        }
    }
    ret(ctx, load<u32>(n + L::kNodeState));
}
