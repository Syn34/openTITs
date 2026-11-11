// Game code, large-function sweep part 6: a bone controller's target
// matrix (00250F40).
#include "big6.h"

using namespace sly4;
using namespace sly4::big6;

namespace {
// Controller.
constexpr u32 kCtlObj = 8;          // the object (vfn[140]: (vec3* out, obj) its velocity (name?); +96 matrix)
constexpr u32 kCtlLinks = 32;       // 28-byte links: +0 node (f32 at +220), +12 obj (+80 matrix), +16 s32 bone,
                                    // +20 body (+240 transform), +24 f32 weight
constexpr u32 kCtlParams = 208;     // 160-byte params: +0 pos[3], +16 euler[3], +32 euler at full speed[3],
                                    // +48 f32 full speed, +96 f32 weight, +144 flags
constexpr u32 kCtlJitter = 1184;    // f32[3]: random euler amplitude
constexpr u32 kCtlBase = 1216;      // an object whose matrix (+80) is the base, or 0
// Output: f32 [4][4], +64 a source matrix, +68 f32 weight, +72 u8 source initialised.
constexpr u32 kOutSrc = 64;
constexpr u32 kOutWeight = 68;
constexpr u32 kOutInit = 72;

constexpr u32 kC = 0x00250F18u;       // .text: f32 0, f32 1, f64 1, f64 0, f32 0.5, f32 0.99, f32 1/65535
constexpr u32 kUp = 0x01164B60u;      // f32[3]: the up vector
constexpr u32 kZeroPos = 0x01164B40u;
constexpr u32 kRandLast = 0x01164888u;  // u64: the last 16-bit random value
constexpr u32 kRandSeed = 0x01164890u;  // u32

constexpr u32 kBoneMatrix = 0x000CE688u;   // (f32 out[4][4], Object*, s32 bone, u8 world, u8 raw)
constexpr u32 kInitSource = 0x00250AF0u;   // (ctl, f32 m[4][4], u32 index, 0) (name?)
constexpr u32 kQuatToMatrix = 0x00553A90u; // (f32 out[4][4], const f32 q[4], const f32 pos[3])
constexpr u32 kMatMultiply = 0x005541F0u;  // (Matrix* a, const Matrix* b): a = a x b
constexpr u32 kMatNormalize = 0x0056D374u; // (f32 m[4][4]) -> m
constexpr u32 kCosf = 0x007344E0u;
constexpr u32 kSinf = 0x007345F0u;
constexpr u32 kVfnVelocity = 140;

f32 F(u32 a) { return load<f32>(a); }
f32 Fb(u32 w) { return std::bit_cast<f32>(w); }
u32 Rotl(u32 x, u32 n) { return (x << n) | (x >> (32 - n)); }
f32 Fsel(f64 a, f64 b, f64 c) { return static_cast<f32>(a >= 0.0 ? b : c); }

// The quaternion of (half) euler angles from their cosines / sines (f1 as
// the math calls return it, used unrounded as fmuls / fmadds operands),
// stored at q (+0 x, +4 y, +8 z, +12 w) in the original's store order.
f32 Mul(f64 a, f64 b) { return static_cast<f32>(a * b); }
f32 Fma(f64 a, f64 c, f64 b) { return static_cast<f32>(std::fma(a, c, b)); }
void EulerQuat(u32 q, f64 cy, f64 cz, f64 cx, f64 sy, f64 sz, f64 sx) {
    const f32 a = Mul(sy, sz), b = Mul(cy, cz), c = Mul(cz, sy), d = Mul(cy, sz);
    const f32 a1 = Mul(a, sx), b1 = Mul(b, sx), c1 = Mul(c, sx), d1 = Mul(d, sx);
    const f32 w = Fma(b, cx, -static_cast<f64>(a1)), x = Fma(a, cx, b1), z = Fma(d, cx, -static_cast<f64>(c1)), y = Fma(c, cx, d1);
    store<f32>(q + 12, w);
    store<f32>(q, x);
    store<f32>(q + 8, z);
    store<f32>(q + 4, y);
}
// cos / sin of the three angles in the original's call order (cos y, z, x,
// then sin y, z, x), then the quaternion.
void AnglesQuat(PPUContext& ctx, u32 q, f32 ay, f32 az, f32 ax) {
    const f64 cy = callf(ctx, kCosf, {}, {ay});
    const f64 cz = callf(ctx, kCosf, {}, {az});
    const f64 cx = callf(ctx, kCosf, {}, {ax});
    const f64 sy = callf(ctx, kSinf, {}, {ay});
    const f64 sz = callf(ctx, kSinf, {}, {az});
    const f64 sx = callf(ctx, kSinf, {}, {ax});
    EulerQuat(q, cy, cz, cx, sy, sz, sx);
}
}  // namespace

// Matrix* BoneCtl_Target(Ctl* ctl, Out* out, u32 k, const f32* pos)
// The base matrix M (sp+224) of controller entry k: the out source (+64,
// initialised once through 0x00250AF0) for flag 0x80000000, else link k's
// object matrix, bone matrix, body transform, the controller's base object
// or its object's matrix; flag 0x10000000 rebuilds rows 0..2 around the up
// vector (row 1 = up); pos replaces row 3. Then out = Q x M with Q the
// matrix of params euler + pos (blended towards the full-speed euler by the
// object's speed / +48), times a random euler jitter when +1184 is not
// zero; out+68 = the link / params weight (or the link node's +220); out
// normalised and copied onto itself from the pointer the normaliser
// returns, which is left in r3 (f1: its last word loaded). The 624-byte
// frame is the original's: M's w lanes (never written) reach out's w
// lanes through the VMX product.
DECOMP(00250F40, BoneCtl_Target) {
    const u32 ctl = arg<u32>(ctx, 0), out = arg<u32>(ctx, 1), k = arg<u32>(ctx, 2), pos = arg<u32>(ctx, 3);
    Stack frame(ctx, 624);
    const u32 sp = frame.addr();
    const u32 prm = ctl + kCtlParams + 160 * k, lnk = ctl + kCtlLinks + 28 * k;
    const u32 src0 = load<u32>(out + kOutSrc);
    const u32 m = sp + 224;
    // The base matrix (x, y, z words of its rows).
    u32 w[12];
    auto take = [&](u32 s) {
        for (u32 i = 0; i < 12; ++i) w[i] = load<u32>(s + 16 * (i / 3) + 4 * (i % 3));
        for (u32 i = 0; i < 12; ++i) store<u32>(m + 16 * (i / 3) + 4 * (i % 3), w[i]);
    };
    if ((load<u32>(prm + 144) & 0x80000000u) != 0) {
        take(src0);
        if (load<u8>(out + kOutInit) == 0) {
            call(ctx, kInitSource, {ctl, src0, k, 0});
            take(src0);
            store<u8>(out + kOutInit, 1);
        }
    } else if (const u32 o12 = load<u32>(lnk + 12); o12 != 0) {
        take(o12 + 80);
    } else if (const s32 bone = load<s32>(lnk + 16); bone != -1) {
        const u32 b = sp + 288;
        call(ctx, kBoneMatrix, {b, load<u32>(ctl + kCtlObj), static_cast<u64>(static_cast<s64>(bone)), 1, 1});
        take(b);
    } else if (const u32 body = load<u32>(lnk + 20); body != 0) {
        const u32 t = sp + 352;
        for (u32 i = 0; i < 12; ++i) store<u32>(t + 16 * (i / 3) + 4 * (i % 3), load<u32>(body + 240 + 16 * (i / 3) + 4 * (i % 3)));
        take(t);
    } else if (const u32 base = load<u32>(ctl + kCtlBase); base != 0) {
        take(base + 80);
    } else {
        take(load<u32>(ctl + kCtlObj) + 96);
    }
    if ((load<u32>(prm + 144) & 0x10000000u) != 0) {
        // Rows 0..2 rebuilt around up U: W = row 2 (row 1 when |row2 . U| > 0.99),
        // row 0 = normalise(U x W), row 1 = U, row 2 = normalise(row0 x U).
        const f32 ux = F(kUp), uy = F(kUp + 4), uz = F(kUp + 8);
        f32 wx = Fb(w[6]), wy = Fb(w[7]), wz = Fb(w[8]);
        f32 d = wy * uy;
        d = std::fma(wx, ux, d);
        d = std::fma(wz, uz, d);
        if (std::fabs(d) > F(kC + 28)) {
            wx = Fb(w[3]);
            wy = Fb(w[4]);
            wz = Fb(w[5]);
        }
        const f32 one = F(kC + 4);
        f32 a1 = std::fma(uz, wx, -(ux * wz));
        f32 a2 = std::fma(uy, wz, -(uz * wy));
        f32 a3 = std::fma(ux, wy, -(uy * wx));
        for (u32 i = 0; i < 3; ++i) CopyW(m + 16 + 4 * i, kUp + 4 * i);
        store<u32>(m + 48, w[9]);
        store<u32>(m + 52, w[10]);
        store<u32>(m + 56, w[11]);
        f32 l = a1 * a1;
        l = std::fma(a2, a2, l);
        l = std::fma(a3, a3, l);
        l = static_cast<f32>(std::sqrt(static_cast<f64>(l)));
        l = static_cast<f32>(static_cast<f64>(one) / static_cast<f64>(l));
        a2 = a2 * l;
        a3 = a3 * l;
        a1 = a1 * l;
        store<f32>(m, a2);
        store<f32>(m + 4, a1);
        const f32 b4 = std::fma(a3, ux, -(a2 * uz));
        const f32 b5 = std::fma(a1, uz, -(a3 * uy));
        const f32 b6 = std::fma(a2, uy, -(a1 * ux));
        store<f32>(m + 8, a3);
        f32 l2 = b4 * b4;
        l2 = std::fma(b5, b5, l2);
        l2 = std::fma(b6, b6, l2);
        l2 = static_cast<f32>(std::sqrt(static_cast<f64>(l2)));
        const f32 r = static_cast<f32>(static_cast<f64>(one) / static_cast<f64>(l2));
        store<f32>(m + 32, b5 * r);
        store<f32>(m + 36, b4 * r);
        store<f32>(m + 40, b6 * r);
    }
    if (pos != 0)
        for (u32 i = 0; i < 3; ++i) CopyW(m + 48 + 4 * i, pos + 4 * i);
    // Euler, blended by speed.
    const f32 zero = F(kC);
    f32 ex = F(prm + 16), ey = F(prm + 20), ez = F(prm + 24);
    if (F(prm + 48) > zero) {
        const u32 obj = load<u32>(ctl + kCtlObj);
        const u32 v = sp + 128;
        VCall(ctx, obj, kVfnVelocity, {v, obj});
        f32 s = F(v + 4) * F(v + 4);
        s = std::fma(F(v), F(v), s);
        s = std::fma(F(v + 8), F(v + 8), s);
        s = static_cast<f32>(std::sqrt(static_cast<f64>(s)));
        const f32 one = F(kC + 4);
        f32 t = static_cast<f32>(static_cast<f64>(s) / static_cast<f64>(F(prm + 48)));
        t = Fsel(static_cast<f64>(static_cast<f32>(one - t)), t, load<f64>(kC + 8));
        t = Fsel(t, t, load<f64>(kC + 16));
        const f32 u = one - t;
        ex = std::fma(u, ex, t * F(prm + 32));
        ey = std::fma(u, ey, t * F(prm + 36));
        ez = std::fma(u, ez, t * F(prm + 40));
        store<f32>(v, ex);
        store<f32>(v + 4, ey);
        store<f32>(v + 8, ez);
    }
    const f32 half = F(kC + 24);
    const u32 q = sp + 112, qm = sp + 160;
    AnglesQuat(ctx, q, ey * half, ez * half, ex * half);
    call(ctx, kQuatToMatrix, {qm, q, prm});
    // out = Q x M: Q's rows copied to out first (row 0 then re-read from the
    // stack with lvlx, rows 1..3 from out with lvx).
    for (u32 i = 0; i < 12; ++i) CopyW(out + 16 * (i / 3) + 4 * (i % 3), qm + 16 * (i / 3) + 4 * (i % 3));
    {
        const V4 q0x = Lvlx(qm), q0y = Lvlx(qm + 4), q0z = Lvlx(qm + 8);
        const V4 q1 = Lvx(out + 16), q2 = Lvx(out + 32), q3 = Lvx(out + 48);
        const V4 m0 = Lvx(m), m1 = Lvx(m + 16), m2 = Lvx(m + 32), m3 = Lvx(m + 48);
        const V4 o0 = Row3(m0, m1, m2, q0x.e[0], q0y.e[0], q0z.e[0]);
        const V4 o1 = Row3(m0, m1, m2, q1.e[0], q1.e[1], q1.e[2]);
        const V4 o2 = Row3(m0, m1, m2, q2.e[0], q2.e[1], q2.e[2]);
        const V4 o3 = Add4(m3, Row3(m0, m1, m2, q3.e[0], q3.e[1], q3.e[2]));
        Stvx(out, o0);
        Stvx(out + 16, o1);
        Stvx(out + 32, o2);
        Stvx(out + 48, o3);
    }
    // Random jitter.
    const f32 jx = F(ctl + kCtlJitter), jy = F(ctl + kCtlJitter + 4), jz = F(ctl + kCtlJitter + 8);
    f32 j2 = jy * jy;
    j2 = std::fma(jx, jx, j2);
    j2 = std::fma(jz, jz, j2);
    if (j2 != zero) {
        constexpr u64 kK = 0xFFFFFFFF87654321ull;
        const u64 old = load<u64>(kRandLast);
        const u32 seed = load<u32>(kRandSeed);
        const u64 b = (static_cast<u64>(Rotl(seed, 4)) + Rotl(seed, 16)) ^ kK;
        const u64 v0 = b & 0xFFFF;
        const u64 d = (static_cast<u64>(Rotl(static_cast<u32>(b), 20)) + b) ^ kK;
        const u64 v1 = d & 0xFFFF;
        const u64 f = (static_cast<u64>(Rotl(static_cast<u32>(d), 20)) + d) ^ kK;
        store<u32>(kRandSeed, Rotl(static_cast<u32>(f), 16));
        store<u64>(kRandLast, f & 0xFFFF);
        const f32 sc = F(kC + 32);
        const f32 r0 = static_cast<f32>(static_cast<f64>(static_cast<s64>(old))) * sc;
        const f32 r1 = static_cast<f32>(static_cast<f64>(static_cast<s64>(v0))) * sc;
        const f32 r2 = static_cast<f32>(static_cast<f64>(static_cast<s64>(v1))) * sc;
        const f32 twox = static_cast<f32>(static_cast<f64>(jx) - static_cast<f64>(-jx));
        const f32 twoy = static_cast<f32>(static_cast<f64>(jy) - static_cast<f64>(-jy));
        const f32 twoz = static_cast<f32>(static_cast<f64>(jz) - static_cast<f64>(-jz));
        const f32 ax = std::fma(twox, r0, -jx);
        const f32 ay = std::fma(twoy, r1, -jy);
        const f32 az = std::fma(twoz, r2, -jz);
        const u32 jq = sp + 144, jm = sp + 416;
        AnglesQuat(ctx, jq, ay * half, az * half, ax * half);
        call(ctx, kQuatToMatrix, {jm, jq, kZeroPos});
        call(ctx, kMatMultiply, {out, jm});
    }
    // Weight.
    store<u32>(out + kOutWeight, 0);
    u32 wt = load<u32>(lnk + 24);
    if (!(static_cast<f64>(Fb(wt)) > static_cast<f64>(zero))) wt = load<u32>(prm + 96);
    if (static_cast<f64>(Fb(wt)) >= static_cast<f64>(zero)) {
        store<u32>(out + kOutWeight, wt);
    } else if (const u32 node = load<u32>(lnk); node != 0) {
        store<u32>(out + kOutWeight, load<u32>(node + 220));
    }
    const u32 n = static_cast<u32>(call(ctx, kMatNormalize, {out}));
    for (u32 i = 0; i < 12; ++i) CopyW(out + 16 * (i / 3) + 4 * (i % 3), n + 16 * (i / 3) + 4 * (i % 3));
    SetF1FromWord(ctx, n + 56);
    ctx.gpr[3] = n;
}
