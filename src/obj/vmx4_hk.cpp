// Game code, VMX sweep part 4 (see vmx4.h): Havok rigid-body drivers.
#include "../sly4.h"
#include "vmx4.h"

using namespace sly4;
using namespace sly4::vmx4;

namespace {
constexpr u32 kK1B5A60 = 0x001B5A60;  // f32 {0.0, 0.5, 1.0}, then (+16) the three lane masks y / z / w
}  // namespace

// void f(T* o, -, -, const f32 d[3]; f32 dt, f32 v0, f32 acc) (called by
// 001B6638): drives o's rigid body (+576) along d with a hard key frame.
// Nothing for dt <= 0. The body's speed |v| (+432, the compiler's
// rsqrte / Newton length with the 0.5 / 1.0 pool words) replaces v0 when
// > 0; the body's 4x3 rows (+240) are copied to the stack, q =
// Quat_FromMatrix(rows) normalised, the target position = the body's
// position + d * (s * dt + acc * 0.5 * dt * dt) (s the speed or v0), and
// hkpKeyFrameUtility_applyHardKeyFrame(&pos, &q, 128 (the stvx index the
// original leaves in r5), body; 1 / dt). f1 / r3 are left as the original
// leaves them.
DECOMP(001B5AA0, Body_KeyFrameAlong_001B5AA0) {
    const u32 o = arg<u32>(ctx, 0), d = arg<u32>(ctx, 3);
    const f64 dt = farg(ctx, 0), v0 = farg(ctx, 1), acc = farg(ctx, 2);
    const f32 zero = F(kK1B5A60), half = F(kK1B5A60 + 4), one = F(kK1B5A60 + 8);
    if (dt <= static_cast<f64>(zero)) return;
    Stack frame(ctx, 304);
    const u32 sp = static_cast<u32>(ctx.gpr[1]);
    const u32 pos = sp + 112, quat = sp + 128, q = sp + 144, rows = sp + 160;
    const u32 b = load<u32>(o + 576);
    for (u32 r = 0; r < 4; ++r)
        for (u32 k = 0; k < 3; ++k) CopyW(rows + 16 * r + 4 * k, b + 240 + 16 * r + 4 * k);
    const V4 v = Lvx(b + 432);
    const V4 l2 = HSum3(Madd4(v, v, Zero4()));
    const V4 sp4 = SqrtNewton4(l2, SplatW(kK1B5A60 + 4), SplatW(kK1B5A60 + 8));
    const f32 speed = sp4.e[0];
    call(ctx, v4fn::Quat_FromMatrix, {q, rows});
    f32 qx = F(q), qy = F(q + 4), qz = F(q + 8), qw = F(q + 12);
    const f32 tx = F(rows + 48), ty = F(rows + 52), tz = F(rows + 56);
    const f32 n = Divs(one, Sqrts(Fmadds(qw, qw, Fmadds(qz, qz, Fmadds(qx, qx, Muls(qy, qy))))));
    const u32 body = load<u32>(o + 576);
    qx = Muls(qx, n);
    qy = Muls(qy, n);
    qz = Muls(qz, n);
    qw = Muls(qw, n);
    const f64 s0 = speed > zero ? static_cast<f64>(speed) : v0;
    const f32 a = Muls(Muls(Muls(acc, half), dt), dt);
    const f32 s = Fmadds(s0, dt, a);
    const f32 px = Fmadds(F(d), s, tx), py = Fmadds(F(d + 4), s, ty), pz = Fmadds(F(d + 8), s, tz);
    const Masks3 mk = LoadMasks3(kK1B5A60 + 16);
    Stvx(pos, RowSel(Fill4(px), Fill4(py), Fill4(pz), mk));
    Stvx(quat, Sel(Sel(Sel(Fill4(qx), Fill4(qy), mk.m[0]), Fill4(qz), mk.m[1]), Fill4(qw), mk.m[2]));
    callf(ctx, v4fn::hkpKeyFrameUtility_applyHardKeyFrame, {pos, quat, 128, body}, {Divs(one, dt)});
}

namespace {
constexpr u32 kMasks1D1B0 = 0x0001D1B0;  // lane masks y / z / w (its first word, 0.0f, the no-hit fraction)
constexpr u32 kAllHitsVt = 0x00F1DE80;   // hkpAllRayHitCollector vtable
constexpr u32 kHkHeap = 0x011DF31C;      // the Havok heap object (vt[5] frees (heap, ptr, bytes))
}  // namespace

// bool f(const f32 from[3], const f32 to[3], f32 out[3] / 0) (called by
// Obj_SnapToGround_0001D548, 0002EE28): casts a ray through the physics
// world ((world +0x1281D4)->+4) with an all-hits collector on the stack
// (8 inline hits; input filter 16), sorts the hits and takes the first
// whose collidable (+68) is of type (+28 & 31) 1..4, or of type 0 / 7..12
// with no owner beyond a rigid body (type byte +24 == 1: owner +(s8)+16
// with byte +232 of 4 or 5, or no owner; type byte 2 with an owner
// rejects). out = from + (to - from) * fraction (0.0 without a hit). A hit
// array the collector reallocated (flag 0x80000000 of +24 clear) is freed
// through the Havok heap (vt[5], the entry before the OPD as the original
// passes them). Returns whether a hit was taken. f1 is left as the
// original leaves it.
DECOMP(0001D1E0, Hk_RayHitGround_0001D1E0) {
    const u32 from = arg<u32>(ctx, 0), to = arg<u32>(ctx, 1), out = arg<u32>(ctx, 2);
    Stack frame(ctx, 880);
    const u32 sp = static_cast<u32>(ctx.gpr[1]);
    const u32 in = sp + 112, coll = sp + 160, hits = sp + 192;
    const Masks3 mk = LoadMasks3(kMasks1D1B0);
    store<u8>(sp + 144, 0);
    store<u32>(sp + 152, 0);
    Stvx(in, RowSelW(from, from + 4, from + 8, mk));
    store<u32>(sp + 148, 16);
    Stvx(in + 16, RowSelW(to, to + 4, to + 8, mk));
    const u32 world = load<u32>(load<u32>(load<u32>(kV3WorldPtr) + 0x001281D4u) + 4);
    store<u32>(coll + 4, 0x3F800000u);
    store<u32>(coll, kAllHitsVt);
    store<u32>(coll + 16, hits);
    store<u32>(coll + 20, 0);
    store<u32>(coll + 24, 0x80000008u);
    for (u32 k = 0; k < 8; ++k) {
        const u32 h = hits + 80 * k;
        store<u32>(h + 16, 0x3F800000u);
        store<u32>(h + 20, 0xFFFFFFFFu);
        store<u32>(h + 64, 0);
        store<u32>(h + 32, 0xFFFFFFFFu);
        store<u32>(h + 68, 0);
    }
    store<u32>(coll + 20, 0);
    store<u32>(coll + 4, 0x3F800000u);
    call(ctx, v4fn::HkWorld_CastRay, {world, in, coll});
    call(ctx, v4fn::HkAllHits_Sort, {coll});
    const u32 data = load<u32>(coll + 16);
    const s32 n = static_cast<s32>(load<u32>(coll + 20));
    const u32 flags = load<u32>(coll + 24);
    f32 frac = F(kMasks1D1B0);
    u32 hit = 0;
    for (s32 i = 0; i < n; ++i) {
        const u32 p = data + 80 * static_cast<u32>(i);
        const u32 e = load<u32>(p + 68);
        const u32 t = load<u32>(e + 28) & 31u;
        bool take = false;
        if (t >= 16 || t == 6 || t == 5 || t > 12) continue;
        if (t >= 1 && t <= 4) take = true;
        else {
            const s32 ty = static_cast<s8>(load<u8>(e + 24));
            u32 own = 0;
            if (ty == 1) own = e + static_cast<u32>(static_cast<s32>(static_cast<s8>(load<u8>(e + 16))));
            if (ty == 2 && e + static_cast<u32>(static_cast<s32>(static_cast<s8>(load<u8>(e + 16)))) != 0) continue;
            if (own == 0) take = true;
            else {
                const u32 c = load<u8>(own + 232);
                take = c == 5 || c == 4;
            }
        }
        if (!take) continue;
        frac = F(p + 16);
        hit = 1;
        break;
    }
    ctx.fpr[1] = frac;
    if (out != 0) {
        const f32 fx = F(from), fy = F(from + 4), fz = F(from + 8);
        const f32 z = Fmadds(Subs(F(to + 8), fz), frac, fz);
        store<f32>(out, Fmadds(Subs(F(to), fx), frac, fx));
        store<f32>(out + 4, Fmadds(Subs(F(to + 4), fy), frac, fy));
        store<f32>(out + 8, z);
        ctx.fpr[1] = z;
    }
    if ((flags & 0x80000000u) == 0) {
        const u32 bytes = (flags & 0x3FFFFFFFu) * 80u;
        store<u32>(coll + 20, 0);
        store<u32>(coll, kAllHitsVt);
        const u32 opd = load<u32>(load<u32>(kHkHeap) + 20);
        call_ptr(ctx, opd, {kHkHeap, data, static_cast<u64>(static_cast<s64>(static_cast<s32>(bytes))), load<u32>(opd), opd});
    }
    ctx.gpr[3] = hit;
}

namespace {
constexpr u32 kK20D7B0 = 0x0020D7B0;  // lane masks z / w / y, then f32 0.5
}  // namespace

// void f(T* o, const f32 m[4][4]) (called by Obj_SetTransform_00224D50):
// o's rows +96 = m (4x3 words); then by the physics body's (o->+416) motion
// kind (vt[7]): 0 -> its AABB (+160 min / +176 max) re-centred on m's
// position (half-extents * 0.5 around it) goes to Hk_SetAabb(body, &aabb);
// 1 or 2 -> m's rows (w cleared) go to Hk_SetTransform(body, rows); else
// nothing. f1 / r3 are left as the original leaves them.
DECOMP(0020D7E8, Obj_SyncBodyTransform_0020D7E8) {
    const u32 o = arg<u32>(ctx, 0), m = arg<u32>(ctx, 1);
    for (u32 r = 0; r < 4; ++r)
        for (u32 k = 0; k < 3; ++k) CopyW(o + 96 + 16 * r + 4 * k, m + 16 * r + 4 * k);
    SetF1Bits(ctx, load<u32>(m + 56));
    const u32 b0 = load<u32>(o + 416);
    const s32 kind = static_cast<s32>(static_cast<u32>(VCall(ctx, b0, 28, {b0})));
    const Masks3 mk{{Lvx(kK20D7B0 + 32), Lvx(kK20D7B0), Lvx(kK20D7B0 + 16)}};
    ctx.gpr[3] = m + 48;  // as the original leaves it on the early returns
    if (kind > 0) {
        if (kind > 2) return;
        const u32 body = load<u32>(o + 416);
        Stack frame(ctx, 256);
        const u32 rows = static_cast<u32>(ctx.gpr[1]) + 144;
        for (u32 r = 0; r < 4; ++r) Stvx(rows + 16 * r, RowSelW(m + 16 * r, m + 16 * r + 4, m + 16 * r + 8, mk));
        call(ctx, v4fn::Hk_SetTransform, {body, rows});
        return;
    }
    if (kind != 0) return;
    const u32 body = load<u32>(o + 416);
    Stack frame(ctx, 256);
    const u32 bb = static_cast<u32>(ctx.gpr[1]) + 112;
    Stvx(bb, Lvx(body + 160));
    Stvx(bb + 16, Lvx(body + 176));
    const f32 h = F(kK20D7B0 + 48);
    const f32 ex = Subs(F(bb + 16), F(bb)), ey = Subs(F(bb + 20), F(bb + 4)), ez = Subs(F(bb + 24), F(bb + 8));
    const f32 px = F(m + 48), py = F(m + 52), pz = F(m + 56);
    const f32 hx = Fmadds(ex, h, px);
    Stvx(bb, RowSel(Fill4(Fnmsubs(ex, h, px)), Fill4(Fnmsubs(ey, h, py)), Fill4(Fnmsubs(ez, h, pz)), mk));
    Stvx(bb + 16, RowSel(Fill4(hx), Fill4(Fmadds(ey, h, py)), Fill4(Fmadds(ez, h, pz)), mk));
    ctx.fpr[1] = hx;
    call(ctx, v4fn::Hk_SetAabb, {body, bb});
}
