// Native VMX helpers for src/obj: a vector register as four f32 lanes in
// guest order (x, y, z, w), loaded from / stored to guest memory with the
// AltiVec addressing rules (lvx / stvx ignore the low four address bits,
// lvlx loads the rest of the 16-byte block left-justified). Arithmetic is
// single precision per lane, vmaddfp fused (fmaf), as the verified code
// does it.
#pragma once

#include <bit>
#include <cmath>

#include "objtypes.h"

namespace sly4::obj {

struct V4 {
    f32 e[4];
};

inline u32 LaneW(const V4& v, u32 k) { return std::bit_cast<u32>(v.e[k]); }
inline V4 FromWords(u32 a, u32 b, u32 c, u32 d) {
    return V4{{std::bit_cast<f32>(a), std::bit_cast<f32>(b), std::bit_cast<f32>(c), std::bit_cast<f32>(d)}};
}

// lvx: the aligned quadword containing p.
inline V4 Lvx(const void* p) {
    const Quad& q = gm<Quad>(guest(p) & ~15u);
    return FromWords(q.w[0], q.w[1], q.w[2], q.w[3]);
}
// stvx: the four lanes to the aligned quadword containing p (one 32-bit
// store per lane, in order).
inline void Stvx(void* p, const V4& v) {
    Quad& q = gm<Quad>(guest(p) & ~15u);
    for (u32 k = 0; k < 4; ++k) q.w[k] = LaneW(v, k);
}
// lvlx: the bytes from p to the end of its 16-byte block, left-justified,
// the rest zero.
inline V4 Lvlx(const void* p) {
    const u32 ea = guest(p);
    u8 b[16] = {};
    const u32 n = 16 - (ea & 15);
    for (u32 k = 0; k < n; ++k) b[k] = gm<u8>(ea + k);
    V4 v;
    for (u32 k = 0; k < 4; ++k)
        v.e[k] = std::bit_cast<f32>((u32(b[4 * k]) << 24) | (u32(b[4 * k + 1]) << 16) | (u32(b[4 * k + 2]) << 8) | b[4 * k + 3]);
    return v;
}

inline V4 Zero4() { return V4{{0.0f, 0.0f, 0.0f, 0.0f}}; }
inline V4 Splat(const V4& v, u32 k) { return V4{{v.e[k], v.e[k], v.e[k], v.e[k]}}; }
// The f32 word at p in every lane (lvlx + vspltw 0).
inline V4 SplatW(const void* p) { return Splat(Lvlx(p), 0); }
inline V4 Mrghw(const V4& a, const V4& b) { return V4{{a.e[0], b.e[0], a.e[1], b.e[1]}}; }
inline V4 Mrglw(const V4& a, const V4& b) { return V4{{a.e[2], b.e[2], a.e[3], b.e[3]}}; }
// vperm: byte c & 31 of the 32 bytes a:b, for each byte c of the mask.
inline V4 Perm(const V4& a, const V4& b, const V4& m) {
    u8 ab[32], mb[16];
    for (u32 k = 0; k < 4; ++k)
        for (u32 j = 0; j < 4; ++j) {
            ab[4 * k + j] = static_cast<u8>(LaneW(a, k) >> (24 - 8 * j));
            ab[16 + 4 * k + j] = static_cast<u8>(LaneW(b, k) >> (24 - 8 * j));
            mb[4 * k + j] = static_cast<u8>(LaneW(m, k) >> (24 - 8 * j));
        }
    V4 o;
    for (u32 k = 0; k < 4; ++k) {
        u32 w = 0;
        for (u32 j = 0; j < 4; ++j) w = (w << 8) | ab[mb[4 * k + j] & 31];
        o.e[k] = std::bit_cast<f32>(w);
    }
    return o;
}
// vsel: b's bits where m is set, a's elsewhere.
inline V4 Sel(const V4& a, const V4& b, const V4& m) {
    V4 o;
    for (u32 k = 0; k < 4; ++k) o.e[k] = std::bit_cast<f32>((LaneW(a, k) & ~LaneW(m, k)) | (LaneW(b, k) & LaneW(m, k)));
    return o;
}
// vandc a & ~m.
inline V4 Andc(const V4& a, const V4& m) {
    V4 o;
    for (u32 k = 0; k < 4; ++k) o.e[k] = std::bit_cast<f32>(LaneW(a, k) & ~LaneW(m, k));
    return o;
}
inline V4 Add4(const V4& a, const V4& b) {
    V4 o;
    for (u32 k = 0; k < 4; ++k) o.e[k] = a.e[k] + b.e[k];
    return o;
}
inline V4 Sub4(const V4& a, const V4& b) {
    V4 o;
    for (u32 k = 0; k < 4; ++k) o.e[k] = a.e[k] - b.e[k];
    return o;
}
// vxor.
inline V4 Xor4(const V4& a, const V4& b) {
    V4 o;
    for (u32 k = 0; k < 4; ++k) o.e[k] = std::bit_cast<f32>(LaneW(a, k) ^ LaneW(b, k));
    return o;
}
// vminfp / vmaxfp as the runtime's vmx.h evaluates them: a NaN in a, else
// a NaN in b, else the smaller / larger (a on ties, so +0 / -0 keep a).
inline V4 Min4(const V4& a, const V4& b) {
    V4 o;
    for (u32 k = 0; k < 4; ++k) {
        const f32 x = a.e[k], y = b.e[k];
        o.e[k] = std::isnan(x) ? x : std::isnan(y) ? y : (y < x ? y : x);
    }
    return o;
}
inline V4 Max4(const V4& a, const V4& b) {
    V4 o;
    for (u32 k = 0; k < 4; ++k) {
        const f32 x = a.e[k], y = b.e[k];
        o.e[k] = std::isnan(x) ? x : std::isnan(y) ? y : (x < y ? y : x);
    }
    return o;
}
// vmaddfp chain for one row: c2 * s2 + (c1 * s1 + (c0 * s0 + 0)), lane by
// lane (fused, single precision).
inline V4 Row3(const V4& c0, const V4& c1, const V4& c2, f32 s0, f32 s1, f32 s2) {
    V4 o;
    for (u32 k = 0; k < 4; ++k) {
        f32 x = std::fma(c0.e[k], s0, 0.0f);
        x = std::fma(c1.e[k], s1, x);
        o.e[k] = std::fma(c2.e[k], s2, x);
    }
    return o;
}

// The three lane masks the compiler keeps for building {x, y, z, 0} rows.
struct Masks3 {
    V4 m[3];
};
inline Masks3 LoadMasks3(u32 ea) { return Masks3{{Lvx(host<u8>(ea)), Lvx(host<u8>(ea + 16)), Lvx(host<u8>(ea + 32))}}; }
// {x, y, z, 0}: the words at px / py / pz merged with the masks.
inline V4 RowSelW(const void* px, const void* py, const void* pz, const Masks3& k) {
    return Andc(Sel(Sel(SplatW(px), SplatW(py), k.m[0]), SplatW(pz), k.m[1]), k.m[2]);
}

// x in every lane.
inline V4 Fill4(f32 x) { return V4{{x, x, x, x}}; }
// {x, y, z, 0} from three splats and the masks (vsel y, vsel z, vandc).
inline V4 RowSel(const V4& x, const V4& y, const V4& z, const Masks3& k) {
    return Andc(Sel(Sel(x, y, k.m[0]), z, k.m[1]), k.m[2]);
}

// The rigid inverse the compiler builds of the 4x4 rows at m: the
// transposed rotation (c0 from vmrghw, c1 / c2 through the two vperm masks
// p0 / p1) and t = -row3 through it.
struct InvRows4 {
    V4 c[3];
    V4 t;
};
inline InvRows4 InvRigid4(const void* m, const V4& p0, const V4& p1) {
    const u8* b = static_cast<const u8*>(m);
    const V4 r0 = Lvx(b), r1 = Lvx(b + 16), r2 = Lvx(b + 32), r3 = Lvx(b + 48);
    const V4 nt = Sub4(Zero4(), r3);
    const V4 h = Mrghw(r0, r2), l = Mrglw(r0, r2);
    InvRows4 o;
    o.c[0] = Mrghw(h, r1);
    o.c[1] = Perm(h, r1, p0);
    o.c[2] = Perm(l, r1, p1);
    o.t = Row3(o.c[0], o.c[1], o.c[2], nt.e[0], nt.e[1], nt.e[2]);
    return o;
}

}  // namespace sly4::obj
