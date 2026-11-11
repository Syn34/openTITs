// Game code, VMX sweep part 3 (see vmx3.h): object code.
#include "../sly4.h"
#include "vmx3.h"

using namespace sly4;
using namespace sly4::vmx3;

namespace {
constexpr u32 kPool146870 = 0x00146830;  // f32 0.2, 0.5, -0.5, pad, then three lane masks
}  // namespace

// bool f(Obj* o, u8 front) (called by 00148130): ground probe. With r =
// o->vfn[16](o): a = Obj_RefPos26A42C(r).x, b = .z, t = Obj_ModePos_0026D8F4
// (r).z; without, a = 0, b = t = 0 (the zero vector). From o's matrix (+96):
// P = row3 + row2 * t -/+ row2 * (b + 0.2) (+ when front), and the points
// P - row0 * (a + 0.2), P, P + row0 * (a + 0.2) are each cast down from
// y + 0.5 to y - 0.5 (HkWorld_CastRay92E2E8 on the world's +0x1281D4 ->+4,
// filter 8). Returns 1 when all three rays hit (output +68 set), else 0.
// f1 is left as the original leaves it (the last ray's y - 0.5).
DECOMP(00146870, Obj_GroundProbe3_00146870) {
    const u32 o = arg<u32>(ctx, 0);
    const u8 front = static_cast<u8>(arg<u32>(ctx, 1));
    const u32 K = kPool146870;
    Stack frame(ctx, 432);
    const u32 sp = frame.addr();
    const u32 r = static_cast<u32>(VCall(ctx, o, 64, {o}));
    f32 a = F(kV3Zero3), b = F(kV3Zero3 + 8), t = b;
    if (r != 0) {
        call(ctx, 0x0026A42C, {sp + 112, r});
        a = F(sp + 112);
        b = F(sp + 120);
        call(ctx, 0x0026D8F4, {sp + 112, r});
        t = F(sp + 120);
    }
    const u32 m = o + 96;
    const f32 k0 = F(K);
    const f32 c0 = F(m + 32), c1 = F(m + 36), c2 = F(m + 40);
    const f32 bx = Fmadds(c0, t, F(m + 48)), by = Fmadds(c1, t, F(m + 52)), bz = Fmadds(c2, t, F(m + 56));
    const f32 db = Adds(b, k0);
    f32 px = Fnmsubs(c0, db, bx), py = Fnmsubs(c1, db, by), pz = Fnmsubs(c2, db, bz);
    if (front != 0) {
        px = Fmadds(c0, db, bx);
        py = Fmadds(c1, db, by);
        pz = Fmadds(c2, db, bz);
    }
    const f32 r0 = F(m), r1 = F(m + 4), r2 = F(m + 8);
    const f32 da = Adds(a, k0);
    store<f32>(sp + 144, px);
    store<f32>(sp + 148, py);
    store<f32>(sp + 152, pz);
    store<f32>(sp + 128, Fnmsubs(r0, da, px));
    store<f32>(sp + 132, Fnmsubs(r1, da, py));
    store<f32>(sp + 136, Fnmsubs(r2, da, pz));
    store<f32>(sp + 160, Fmadds(r0, da, px));
    store<f32>(sp + 164, Fmadds(r1, da, py));
    store<f32>(sp + 168, Fmadds(r2, da, pz));
    const u32 world = load<u32>(load<u32>(load<u32>(kV3WorldPtr) + 0x001281D4) + 4);
    const f32 up = F(K + 4), down = F(K + 8);
    const Masks3 mk = LoadMasks3(K + 16);
    u32 all = 1;
    f32 last = 0.0f;
    for (u32 i = 0; i < 3; ++i) {
        const u32 pt = sp + 128 + 16 * i;
        const f32 y = F(pt + 4);
        const V4 x = SplatW(pt), z = SplatW(pt + 8);
        const f32 y0 = Adds(y, up), y1 = Adds(y, down);
        store<u32>(sp + 216, 0);
        store<u8>(sp + 208, 1);
        store<u32>(sp + 212, 8);
        store<u32>(sp + 288, 0);
        store<u32>(sp + 292, 0);
        store<u32>(sp + 240, 0x3F800000u);  // hit fraction 1.0
        store<u32>(sp + 244, 0xFFFFFFFFu);
        store<u32>(sp + 256, 0xFFFFFFFFu);
        Stvx(sp + 176, RowSel(x, Fill4(y0), z, mk));
        Stvx(sp + 192, RowSel(x, Fill4(y1), z, mk));
        call(ctx, 0x0092E2E8, {world, sp + 176, sp + 224});
        if (load<u32>(sp + 292) == 0) all = 0;
        last = y1;
    }
    ctx.fpr[1] = last;
    ret(ctx, all);
}

namespace {
constexpr u32 kEps3B6420 = 0x003B641C;    // f32 1e-5
constexpr u32 kIdentity4 = 0x01164BD0;    // f32[4][4] identity (the default parent matrix)
constexpr u32 kCurveStep = 0x004A83D8;    // f32 (f1 cur, f2 target, f3 rate, f4 dt)
constexpr u32 kCurveLerp3 = 0x004A8598;   // Curve_Lerp3_004A8598 (out, a, b; f1, f2)
}  // namespace

// void f(T* o; f32 dt) (called by 003B6C30): eases the follower c (+100)
// toward its def's (+96) anchor. c+80 steps toward def +128 +56 (+132 set)
// or +48 (0x004A83D8 with rate +52, unless within 1e-5). The def's point
// +128 goes through the parent matrix (+104 +96, or the identity), then
// along c's row 2 to c's translation height; unless c's translation is
// within 1e-5 of that on every axis, it moves there by Curve_Lerp3_004A8598
// (def +172, dt). c's 4x3 matrix words (+16..+72) are written back either
// way (through a stack copy). f1 is left as the original leaves it (the
// last word copied).
DECOMP(003B6420, Follow_EaseToAnchor_003B6420) {
    const u32 o = arg<u32>(ctx, 0);
    const f64 dt = ctx.fpr[1];
    u32 c = load<u32>(o + 100);
    if (c == 0) return;
    const f32 eps = F(kEps3B6420);
    u32 d = load<u32>(o + 96);
    const u32 r30 = d + 128;
    f32 tgt = load<u32>(o + 132) != 0 ? F(r30 + 56) : F(r30 + 48);
    const f32 cur = F(c + 80);
    if (!(std::fabs(Subs(cur, tgt)) < eps)) {
        tgt = static_cast<f32>(callf(ctx, kCurveStep, {}, {cur, tgt, F(r30 + 52), dt}));
        d = load<u32>(o + 96);
        c = load<u32>(o + 100);
    }
    store<f32>(c + 80, tgt);
    Stack frame(ctx, 256);
    const u32 sp = frame.addr();
    for (u32 r = 0; r < 4; ++r)
        for (u32 k = 0; k < 3; ++k) CopyW(sp + 160 + 16 * r + 4 * k, c + 16 + 16 * r + 4 * k);
    const f32 ax = F(c + 48), ay = F(c + 52), az = F(c + 56);
    const f32 tx = F(c + 64), ty = F(c + 68), tz = F(c + 72);
    CopyW(sp + 112, kV3Zero3);
    CopyW(sp + 116, kV3Zero3 + 4);
    CopyW(sp + 120, kV3Zero3 + 8);
    const u32 p = d + 128;
    CopyW(sp + 128, p);
    CopyW(sp + 132, p + 4);
    CopyW(sp + 136, p + 8);
    const u32 par = load<u32>(o + 104);
    const Mat4 m = LoadMat(par != 0 ? par + 96 : kIdentity4);
    const V4 v = XformPoint(m.r[0], m.r[1], m.r[2], m.r[3], V4{{F(p), F(p + 4), F(p + 8), 0.0f}});
    Stvx(sp + 128, v);
    const f32 s = Divs(Subs(ty, v.e[1]), ay);
    const f32 gx = Fmadds(ax, s, v.e[0]), gz = Fmadds(az, s, v.e[2]), gy = Fmadds(ay, s, v.e[1]);
    store<f32>(sp + 112, gx);
    store<f32>(sp + 116, gy);
    store<f32>(sp + 120, gz);
    if (!(std::fabs(Subs(tx, gx)) < eps && std::fabs(Subs(ty, gy)) < eps && std::fabs(Subs(tz, gz)) < eps)) {
        callf(ctx, kCurveLerp3, {sp + 144, sp + 208, sp + 112}, {F(r30 + 44), dt});
        CopyW(sp + 208, sp + 144);
        CopyW(sp + 212, sp + 148);
        CopyW(sp + 216, sp + 152);
        c = load<u32>(o + 100);
    }
    for (u32 r = 0; r < 4; ++r)
        for (u32 k = 0; k < 3; ++k) CopyW(c + 16 + 16 * r + 4 * k, sp + 160 + 16 * r + 4 * k);
    SetF1Bits(ctx, load<u32>(sp + 216));
}

namespace {
constexpr u32 kPool3CFA18 = 0x003CFA10;  // f32 0.3, 4.0
constexpr u32 kV3Up = 0x01164B60;        // f32[3] the global up vector
constexpr u32 kHashBone3CFA18 = 0x7E95647C;
}  // namespace

// void f(T* o) (called by 003D1AD8): carried-object attach. While the timer
// +96 < 0.3: the holder's (+4) bone 0x7E95647C (Obj_FindBone_000DA9D0) ->
// +8 matrix goes through the holder's matrix (+96); the carried link c
// (+112) gets the result's translation at +32 and its object (c+108) the
// matrix of its own rotation rows (+96..) with that translation
// (Mat34_FromRotPos553B4C) through vfn[21], then vfn[22](1, 1, 1, 1).
// Otherwise it is released: c+48 = holder row 2 * 0.3 + up * 4, vfn[22](1,
// 1, 1, 1), c+0 = 3, c+104 = 1.0, o+76 = 3, o+96 = 0. f1 / r3 are left as
// the original leaves them.
DECOMP(003CFA18, Carry_FollowBone_003CFA18) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 K = kPool3CFA18;
    const f32 k0 = F(K);
    if (!(F(o + 96) < k0)) {
        const f32 k1 = F(K + 4);
        const f32 ux = Muls(F(kV3Up), k1), uy = Muls(F(kV3Up + 4), k1), uz = Muls(F(kV3Up + 8), k1);
        const u32 r2 = load<u32>(o + 4) + 128;
        const f32 px = Fmadds(F(r2), k0, ux), py = Fmadds(F(r2 + 4), k0, uy), pz = Fmadds(F(r2 + 8), k0, uz);
        u32 c = load<u32>(o + 112);
        const u32 t = load<u32>(c + 108);
        store<f32>(c + 48, px);
        store<f32>(c + 52, py);
        store<f32>(c + 56, pz);
        VCall(ctx, t, 88, {t, 1, 1, 1, 1});
        c = load<u32>(o + 112);
        store<u32>(c, 3);
        store<u32>(c + 104, 0x3F800000u);
        store<u32>(o + 76, 3);
        store<u32>(o + 96, 0);
        ctx.fpr[1] = pz;
        ctx.gpr[3] = c;
        return;
    }
    Stack frame(ctx, 320);
    const u32 sp = frame.addr();
    const u32 bone = static_cast<u32>(call(ctx, 0x000DA9D0, {load<u32>(o + 4), kHashBone3CFA18}));
    const u32 b = load<u32>(bone + 8);
    const Mat4 A = LoadMat(load<u32>(o + 4) + 96);
    const Mat4 B = LoadMat(b);
    Stvx(sp + 160, XformDir(A.r[0], A.r[1], A.r[2], B.r[0]));
    Stvx(sp + 176, XformDir(A.r[0], A.r[1], A.r[2], B.r[1]));
    Stvx(sp + 192, XformDir(A.r[0], A.r[1], A.r[2], B.r[2]));
    Stvx(sp + 208, XformPoint(A.r[0], A.r[1], A.r[2], A.r[3], B.r[3]));
    const u32 c = load<u32>(o + 112);
    CopyW(c + 32, sp + 208);
    const u32 t = load<u32>(c + 108);
    CopyW(c + 36, sp + 212);
    CopyW(c + 40, sp + 216);
    for (u32 r = 0; r < 3; ++r)
        for (u32 k = 0; k < 3; ++k) CopyW(sp + 112 + 16 * r + 4 * k, t + 96 + 16 * r + 4 * k);
    const u32 last = load<u32>(sp + 152);
    call(ctx, 0x00553B4C, {sp + 224, sp + 112, sp + 208});
    VCall(ctx, t, 84, {t, sp + 224});
    const u32 t2 = load<u32>(load<u32>(o + 112) + 108);
    VCall(ctx, t2, 88, {t2, 1, 1, 1, 1});
    SetF1Bits(ctx, last);
}

namespace {
constexpr u32 kPool1AED20 = 0x001AED18;   // f32 1.0, 0.0
constexpr u32 kFindComponent = 0x000D1288;  // Obj_FindComponent (o, type, after)
constexpr u32 kEmitterStop = 0x00443424;    // Emitter_Stop (handle, fade)
constexpr u32 kSoundPlay = 0x0044C474;      // Sound_Play44C474 (11 arguments)
}  // namespace

// void f(Obj* o, u32 i, u8 force) (called by Obj_Set_001C4280): switches o
// to sound state i. Needs a def (+216). Bit 0x40000000 of +552 = whether
// the world's (+4208 +8) object has a type-23 component whose +24 bits
// 25..29 are 7. A playing loop (+540 != -1) is stopped (Emitter_Stop of
// +428 + 16 * it, fade). The state's volume +436 + 16i (times def +392 with
// that bit) is v. With force or v > 0: def +304 + 8i set -> the sound
// (+424 + 16i) is played at def +368 through o's matrix (+96) and its
// handle kept at +428 + 16i; then, with +356 set, +536 ->+20 = v (when
// +536 is set) and o->vfn[113](o, i, force). Always o->vfn[7](o, 0, (u8)i)
// and +540 = i. f1 is left as the original leaves it.
DECOMP(001AED20, Obj_SetSoundState_001AED20) {
    const u32 o = arg<u32>(ctx, 0), i = arg<u32>(ctx, 1);
    const u32 force = arg<u32>(ctx, 2) & 0xFF;
    const u32 K = kPool1AED20;
    const u32 def = load<u32>(o + 216);
    if (def == 0) return;
    Stack frame(ctx, 224);
    const u32 sp = frame.addr();
    store<u32>(o + 552, load<u32>(o + 552) & ~0x40000000u);
    const u32 w = load<u32>(load<u32>(kV3WorldPtr) + 4208 + 8);
    if (w != 0) {
        u32 c = static_cast<u32>(call(ctx, kFindComponent, {w, 23, 0}));
        while (c != 0) {
            if (((load<u32>(c + 24) >> 25) & 0x1F) == 7) {
                store<u32>(o + 552, load<u32>(o + 552) | 0x40000000u);
                break;
            }
            c = static_cast<u32>(call(ctx, kFindComponent, {w, 23, c}));
        }
    }
    const u32 cur = load<u32>(o + 540);
    if (cur != 0xFFFFFFFFu) call(ctx, kEmitterStop, {load<u32>(o + 428 + (cur << 4)), 1});
    const u32 e = o + (i << 4);
    f32 sc = F(K);
    f64 f1 = F(e + 436);
    if ((load<u32>(o + 552) & 0x40000000u) != 0) sc = F(def + 392);
    const f32 v = Muls(f1, sc);
    bool play = force != 0;
    if (!play) {
        f1 = F(K + 4);
        play = v > F(K + 4);
    }
    if (play) {
        if (load<u32>(def + 304 + static_cast<u32>(static_cast<s32>(i << 3))) != 0) {
            CopyW(sp + 144, def + 368);
            CopyW(sp + 148, def + 372);
            CopyW(sp + 152, def + 376);
            const Mat4 m = LoadMat(o + 96);
            Stvx(sp + 144, XformPoint(m.r[0], m.r[1], m.r[2], m.r[3], V4{{F(def + 368), F(def + 372), F(def + 376), 0.0f}}));
            f1 = F(def + 376);
            call(ctx, kSoundPlay, {sp + 160, load<u32>(e + 424), sp + 144, 0, 0, 0, 0, 0, 0, 0, 0});
            CopyW(e + 428, sp + 160);
        }
        if (load<u32>(o + 356) != 0) {
            const u32 q = load<u32>(o + 536);
            if (q != 0) store<f32>(q + 20, v);
            VCall(ctx, o, 452, {o, static_cast<u64>(static_cast<s64>(static_cast<s32>(i))), force});
        }
    }
    VCall(ctx, o, 28, {o, 0, i & 0xFF});
    store<u32>(o + 540, i);
    ctx.fpr[1] = f1;
}

namespace {
constexpr u32 kPool23E0F0 = 0x0023E0D0;  // f32 0, 1, 0.05, 0.5, f64 1.0, f64 0.0
}  // namespace

// void f(T* o, x, T* a, T* b, T* c; f32 dt) (a callback; no direct
// callers): first the base update 0x009F9ADC(o, x, a, b, c; dt). Needs the
// owner's (o+24) +396 record with byte +872+55 set, and b. All of the
// owner's +612 group's (+28 ->, s8 count +32) 224-byte members (+72 ->)
// must be active (+36): none active resets the timer (+28 = 0), and so does
// an active share below 1 (the group pointers are read even when null, as
// the original does) or |b+12| >= 0.05. Otherwise c+4 = max(c+4, the record
// curve (+872 +56 .. +60 by (timer / +64)^4, clamped to [0, 1]; 1 without a
// duration)), c+12 = 1 when a's object (+24) moves slower than 0.5 (+432,
// the rsqrte length), and the timer advances by dt. r3 is not set; f1 is
// left as the original leaves it.
DECOMP(0023E0F0, Group_ChargeTimer_0023E0F0) {
    const u32 o = arg<u32>(ctx, 0), x = arg<u32>(ctx, 1), a = arg<u32>(ctx, 2), b = arg<u32>(ctx, 3), c = arg<u32>(ctx, 4);
    const f64 dt = ctx.fpr[1];
    const u32 K = kPool23E0F0;
    Stack frame(ctx, 192);
    const u32 sp = frame.addr();
    callf(ctx, 0x009F9ADC, {o, x, a, b, c}, {dt});
    ctx.fpr[1] = dt;
    const u32 own = load<u32>(o + 24);
    const u32 rec = load<u32>(own + 396);
    if (rec == 0) return;
    const u32 s = rec + 872;
    if (load<u8>(s + 55) == 0 || b == 0) return;
    const u32 g = load<u32>(own + 612);
    if (g != 0 && load<u32>(g + 28) != 0) {
        const u32 n = static_cast<u32>(static_cast<s32>(static_cast<s8>(load<u8>(load<u32>(g + 28) + 32))));
        const u32 arr = load<u32>(g + 72);
        bool any = false;
        for (u32 k = 0; n > k; ++k)
            if (load<u32>(arr + 224 * k + 36) != 0) { any = true; break; }
        if (!any) {
            store<u32>(o + 28, 0);
            return;
        }
    }
    f32 cnt = F(K);
    ctx.fpr[1] = cnt;
    if (g != 0 && load<u32>(g + 28) != 0) {
        const u32 n = static_cast<u32>(static_cast<s32>(static_cast<s8>(load<u8>(load<u32>(g + 28) + 32))));
        const u32 arr = load<u32>(g + 72);
        const f32 one = F(K + 4);
        for (u32 k = 0; k < n; ++k)
            if (load<u32>(arr + 224 * k + 36) != 0) cnt = Adds(cnt, one);
    }
    // the group record is re-read here, null or not (as the original does)
    const s32 n = static_cast<s8>(load<u8>(load<u32>(g + 28) + 32));
    const f32 one = F(K + 4);
    if (Divs(cnt, static_cast<f32>(static_cast<f64>(n))) < one || !(std::fabs(F(b + 12)) < F(K + 8))) {
        store<u32>(o + 28, 0);
        return;
    }
    f32 lvl = one;
    const f32 dur = F(s + 64);
    if (dur > F(K)) {
        f32 t = Divs(F(o + 28), dur);
        t = Fsels(Subs(one, t), t, Lfd(K + 16));
        t = Fsels(t, t, Lfd(K + 24));
        t = Muls(t, t);
        t = Muls(t, t);
        const f32 lo = F(s + 56);
        const f32 l = Fmadds(Subs(F(s + 60), lo), t, lo);
        const f32 u = Fsels(Subs(one, l), l, Lfd(K + 16));
        lvl = Fsels(u, u, Lfd(K + 24));
    }
    const f32 prev = F(c + 4);
    store<f32>(c + 4, Subs(lvl, prev) >= 0.0f ? lvl : prev);
    const V4 v = Lvx(load<u32>(a + 24) + 432);
    const V4 len = SqrtNewton4(HSum3(Madd4(v, v, Zero4())), SplatW(K + 12), SplatW(K + 4));
    Stvx(sp + 112, len);
    if (F(sp + 112) < F(K + 12)) store<u8>(c + 12, 1);
    const f32 tm = Adds(F(o + 28), dt);
    store<f32>(o + 28, tm);
    ctx.fpr[1] = tm;
}

namespace {
constexpr u32 kQuatToMatrix = 0x00553A90;   // Quat_ToMatrix (out, q, pos)
constexpr u32 kBoneMatrix = 0x000CE688;     // Obj_BoneMatrixCE688 (out, o, bone, world, raw)
constexpr u32 kMatrixMultiply = 0x005541F0;  // Matrix_Multiply (io, b)
constexpr u32 kMatrixNormalize = 0x0056D374;  // Matrix_NormalizeInPlace (m)

// The shared body of the two slot-matrix functions below: the def's slot
// record e holds an offset (+32) and a rotation quaternion (+48); the base
// is the slot's attachment (*att) matrix (+80), else o's bone (att + 4; -1:
// o's own matrix +96; Obj_BoneMatrixCE688 world, raw). Its 4x3 words go to
// out, out's translation becomes the offset through it, then out = out x
// Quat_ToMatrix(quat, 0) and is normalized. f1 is left as the original
// leaves it (the new translation's z).
void SlotMatrix3(PPUContext& ctx, u32 out, u32 o, u32 e, u32 att) {
    Stack frame(ctx, 336);
    const u32 sp = frame.addr();
    const V4 ox = SplatW(e + 32), oy = SplatW(e + 36), oz = SplatW(e + 40);
    CopyW(sp + 112, e + 32);
    CopyW(sp + 116, e + 36);
    CopyW(sp + 120, e + 40);
    call(ctx, kQuatToMatrix, {sp + 128, e + 48, kV3Zero3});
    const u32 a = load<u32>(att);
    u32 src;
    if (a != 0) {
        src = a + 80;
    } else {
        const u32 bone = load<u32>(att + 4);
        if (bone == 0xFFFFFFFFu) {
            src = o + 96;
        } else {
            call(ctx, kBoneMatrix, {sp + 192, o, static_cast<u64>(static_cast<s64>(static_cast<s32>(bone))), 1, 1});
            src = sp + 192;
        }
    }
    for (u32 r = 0; r < 4; ++r)
        for (u32 k = 0; k < 3; ++k) CopyW(out + 16 * r + 4 * k, src + 16 * r + 4 * k);
    const Mat4 m = LoadMat(out);
    Stvx(sp + 112, Add4(m.r[3], Madd4(m.r[2], oz, Madd4(m.r[1], oy, Madd4(m.r[0], ox, Zero4())))));
    CopyW(out + 48, sp + 112);
    CopyW(out + 52, sp + 116);
    CopyW(out + 56, sp + 120);
    const u32 last = load<u32>(sp + 120);
    call(ctx, kMatrixMultiply, {out, sp + 128});
    call(ctx, kMatrixNormalize, {out});
    SetF1Bits(ctx, last);
}
}  // namespace

// void f(f32 out[4][4], Obj* o, u32 i) (called by 001C8DB8 /
// Obj_SlotsNotify_001C9A28 / Obj_UpdateSlots_001C9C18): slot i's world
// matrix (SlotMatrix3): the def's (+216) 96-byte record +416 + 96i, the
// attachment / bone pair o +564 / +568 + 16i.
DECOMP(001C8A74, Obj_SlotMatrix_001C8A74) {
    const u32 out = arg<u32>(ctx, 0), o = arg<u32>(ctx, 1), i = arg<u32>(ctx, 2);
    SlotMatrix3(ctx, out, o, load<u32>(o + 216) + 96 * i + 416, o + (i << 4) + 564);
}

// void f(f32 out[4][4], Obj* o, u32 i) (called by 001C5A80 / 001C78C8 /
// Obj_RefreshSlots_001C7C5C): the same for the second slot kind: the
// def's 80-byte record +448 + 80i, the attachment / bone pair o +592 /
// +596 + 20i.
DECOMP(001C5738, Obj_SlotMatrixB_001C5738) {
    const u32 out = arg<u32>(ctx, 0), o = arg<u32>(ctx, 1), i = arg<u32>(ctx, 2);
    SlotMatrix3(ctx, out, o, load<u32>(o + 216) + 80 * i + 448, o + 20 * i + 592);
}

namespace {
constexpr u32 kPerm128230 = 0x00128210;  // two vperm masks: {a0 b0 a0 b3}, {a0 a1 b2 b3}
constexpr u32 kOfs128230 = 0x010395F0;   // f32[3] the added offset
}  // namespace

// void f(Vec3* out, Obj* o, const f32 m[4][4] /* or 0 */) (a callback; no
// direct callers): o's aim point. With r = o->vfn[16](o): p = r's mode
// position (Obj_ModePos_0026D8F4) with y + its ref position's y
// (Obj_RefPos26A42C) and the offset 0x010395F0 added, taken through m (o's
// matrix +96 when m is 0); when o->vfn[8](o, 0) is 2 and vfn[8](o, 1) is 3,
// y = o+20 ->+72. Without r: m's translation, or o's position +144. The
// original builds m's rows with vperm from lvlx loads and pads their w
// lanes with its uninitialised frame (only the unused w lane of the result
// sees them); the same stack words are read here. f1 is left as the
// original leaves it (out's z); r3 is not set.
DECOMP(00128230, Obj_AimPoint_00128230) {
    const u32 out = arg<u32>(ctx, 0), o = arg<u32>(ctx, 1), m = arg<u32>(ctx, 2);
    Stack frame(ctx, 288);
    const u32 sp = frame.addr();
    const u32 r = static_cast<u32>(VCall(ctx, o, 64, {o}));
    if (r == 0) {
        const u32 src = m != 0 ? m + 48 : o + 144;
        CopyW(out, src);
        CopyW(out + 4, src + 4);
        CopyW(out + 8, src + 8);
        SetF1Bits(ctx, load<u32>(src + 8));
        return;
    }
    call(ctx, 0x0026D8F4, {sp + 128, r});
    const f32 my = F(sp + 132);
    call(ctx, 0x0026A42C, {sp + 112, r});
    f32 y = Adds(my, F(sp + 116));
    f32 x = F(sp + 128);
    f32 z = F(sp + 136);
    store<f32>(sp + 132, y);
    const u32 opd8 = load<u32>(load<u32>(o) + 32);
    x = Adds(x, F(kOfs128230));
    store<f32>(sp + 128, x);
    y = Adds(y, F(kOfs128230 + 4));
    store<f32>(sp + 132, y);
    z = Adds(z, F(kOfs128230 + 8));
    store<f32>(sp + 136, z);
    const u32 mm = m != 0 ? m : o + 96;
    const V4 P0 = Lvx(kPerm128230), P1 = Lvx(kPerm128230 + 16);
    V4 row[4];
    for (u32 k = 0; k < 4; ++k) {
        const u32 b = mm + 16 * k;
        row[k] = Perm(Perm(Lvlx(b), Lvlx(b + 4), P0), Perm(Lvlx(b + 8), Lvx(sp + 144 + 16 * k), P0), P1);
    }
    const V4 v = Add4(row[3], Madd4(row[2], Fill4(z), Madd4(row[1], Fill4(y), Madd4(row[0], Fill4(x), Zero4()))));
    Stvx(sp + 208, Fill4(z));
    Stvx(sp + 128, v);
    if (static_cast<u32>(call_ptr(ctx, opd8, {o, 0, load<u32>(opd8)})) == 2 && static_cast<u32>(VCall(ctx, o, 32, {o, 1})) == 3)
        CopyW(sp + 132, load<u32>(o + 20) + 72);
    CopyW(out, sp + 128);
    CopyW(out + 4, sp + 132);
    CopyW(out + 8, sp + 136);
    SetF1Bits(ctx, load<u32>(sp + 136));
}

namespace {
constexpr u32 kQsIdentT = 0x00B2C390;    // quadword {0, 0, 0, 1} (identity rotation quaternion)
constexpr u32 kQsIdentS = 0x00B2C1E0;    // quadword {1, 1, 1, 1} (unit scale)
constexpr u32 kQsIdentity = 0x00F8F670;  // hkQsTransform identity
constexpr u32 kEps352F70 = 0x00352F6C;   // f32 0.001
constexpr u32 kQsApproxEq = 0x007CFA2C;  // bool (const hkQsTransform* a, const hkQsTransform* b; f1 eps)
constexpr u32 kQsToMatrix = 0x007CF654;  // (const hkQsTransform*, hkTransform* inout: starts as the identity)
constexpr u32 kIdXform3v = 0x00B2C360;  // hkTransform rotation rows (3 quadwords)
}  // namespace

// void f(T* o, -, f32 out[4][4]) (called by 000DC730 / Anim_Apply_00298AC8):
// samples the animated transform. out = the identity (4x3 words); with a
// sampler c (+32) whose clip (+40 ->+12 ->+24) is set, an identity
// hkQsTransform is filled by c->vfn[8](c, -, &qs) (with bit 0x00400000 of +12:
// c+64 = 1.0 and the time step c+8 - o+40 in f1, then o+40 = c+8, c+64 =
// 0; otherwise f1 is passed through from the caller, as the original
// leaves it) and, unless it is within 0.001 of the identity, converted
// (0x007CF654 into an identity hkTransform) to out's 4x3 words. f1 / r3
// are left as the original leaves them.
DECOMP(00352F70, Anim_SampleXform_00352F70) {
    const u32 o = arg<u32>(ctx, 0), out = arg<u32>(ctx, 2);
    const u32 c = load<u32>(o + 32);
    if (c == 0) {
        ctx.gpr[3] = 0;
        return;
    }
    for (u32 r = 0; r < 4; ++r)
        for (u32 k = 0; k < 3; ++k) CopyW(out + 16 * r + 4 * k, kIdentity4 + 16 * r + 4 * k);
    const u32 fl = load<u32>(o + 12);
    if (load<u32>(load<u32>(load<u32>(c + 40) + 12) + 24) == 0) {
        ctx.gpr[3] = c;  // (the identity was copied through f2: f1 is untouched)
        return;
    }
    Stack frame(ctx, 272);
    const u32 sp = frame.addr();
    Stvx(sp + 112, Zero4());
    Stvx(sp + 128, Lvx(kQsIdentT));
    Stvx(sp + 144, Lvx(kQsIdentS));
    if ((fl & 0x00400000u) == 0) {
        const u32 opd = load<u32>(load<u32>(c) + 32);
        call_ptr(ctx, opd, {c, opd, sp + 112, load<u32>(opd)});
    } else {
        store<u32>(c + 64, 0x3F800000u);
        const f32 t = F(c + 8);
        ctx.fpr[1] = Subs(t, F(o + 40));
        const u32 opd = load<u32>(load<u32>(c) + 32);
        call_ptr(ctx, opd, {c, opd, sp + 112, load<u32>(opd)});
        store<f32>(o + 40, t);
        store<u32>(load<u32>(o + 32) + 64, 0);
    }
    callf(ctx, kQsApproxEq, {sp + 112, kQsIdentity}, {std::bit_cast<f64>(LfsBits(load<u32>(kEps352F70)))});
    if (static_cast<u32>(ctx.gpr[3]) != 0) return;
    for (u32 k = 0; k < 3; ++k) Stvx(sp + 160 + 16 * k, Lvx(kIdXform3v + 16 * k));
    Stvx(sp + 208, Zero4());
    call(ctx, kQsToMatrix, {sp + 112, sp + 160});
    for (u32 r = 0; r < 4; ++r)
        for (u32 k = 0; k < 3; ++k) CopyW(out + 16 * r + 4 * k, sp + 160 + 16 * r + 4 * k);
    SetF1Bits(ctx, load<u32>(sp + 216));
    ctx.gpr[3] = out + 56;
}

namespace {
constexpr u32 kObjPick = 0x00451E08;      // Obj_Pick_00451E08 (o) -> picked object or 0
constexpr u32 kParamA0 = 0x00F94784;      // ParamsA descriptor (+16 ->+64 the per-object slot index): a radius
constexpr u32 kParamA3 = 0x00F947CC;      // ParamsA descriptor 3: a byte flag
}  // namespace

// void f(Vec3* out, Obj* o, const Vec3* p) (called by 000215D8): keeps a
// point inside o's region. out = p. When o's (+48) +220 object has a pick
// (Obj_Pick_00451E08) and o's flag param (0x00F947CC slot of the +308 ->+12
// table) is clear, out is clamped to the pick's box (+208: +16 min, +32 max,
// each shrunk by o+324) through its matrix (+80): min(out, max'), then
// max(that, min'), per axis. Otherwise, past the radius param (0x00F94784
// slot, an f32) minus o+324 from o's centre (+48 ->+80), out += (distance
// - that) * (centre - p) (the difference is not normalised, as in the
// original). f1 is left as the original leaves it; r3 is not set.
DECOMP(000212B8, Obj_ClampToRegion_000212B8) {
    const u32 out = arg<u32>(ctx, 0), o = arg<u32>(ctx, 1), p = arg<u32>(ctx, 2);
    CopyW(out, p);
    CopyW(out + 4, p + 4);
    CopyW(out + 8, p + 8);
    const u32 t = load<u32>(load<u32>(o + 48) + 220);
    if (t != 0 && static_cast<u32>(call(ctx, kObjPick, {t})) != 0 &&
        load<u8>(load<u32>(load<u32>(o + 308) + 12) + (load<u32>(load<u32>(kParamA3 + 16) + 64) << 2)) == 0) {
        Stack frame(ctx, 192);
        const u32 sp = frame.addr();
        const u32 b = static_cast<u32>(call(ctx, kObjPick, {t}));
        const u32 bb = load<u32>(b + 208) + 16;
        const f32 mg = F(o + 324);
        const f32 ax = Subs(F(bb + 16), mg), ay = Subs(F(bb + 20), mg), az = Subs(F(bb + 24), mg);
        const f32 bx = Adds(F(bb), mg), by = Adds(F(bb + 4), mg), bz = Adds(F(bb + 8), mg);
        const Mat4 m = LoadMat(b + 80);
        Stvx(sp + 112, XformPoint(m.r[0], m.r[1], m.r[2], m.r[3], V4{{ax, ay, az, 0.0f}}));
        Stvx(sp + 128, XformPoint(m.r[0], m.r[1], m.r[2], m.r[3], V4{{bx, by, bz, 0.0f}}));
        f32 v = 0.0f;
        for (u32 k = 0; k < 3; ++k) {
            const f32 c = F(out + 4 * k), hi = F(sp + 112 + 4 * k), lo = F(sp + 128 + 4 * k);
            const f32 mn = Subs(hi, c) >= 0.0f ? c : hi;
            v = Subs(mn, lo) >= 0.0f ? mn : lo;
            store<f32>(out + 4 * k, v);
        }
        ctx.fpr[1] = v;
        return;
    }
    const u32 c = load<u32>(o + 48);
    const f32 dy = Subs(F(c + 84), F(p + 4)), dx = Subs(F(c + 80), F(p)), dz = Subs(F(c + 88), F(p + 8));
    const f32 lim = Subs(F(load<u32>(load<u32>(o + 308) + 12) + (load<u32>(load<u32>(kParamA0 + 16) + 64) << 2)), F(o + 324));
    const f32 d = Len3s(dx, dy, dz);
    ctx.fpr[1] = d;
    if (d > lim) {
        const f32 s = Subs(d, lim);
        const f32 nx = Fmadds(s, dx, F(out)), ny = Fmadds(s, dy, F(out + 4)), nz = Fmadds(s, dz, F(out + 8));
        store<f32>(out, nx);
        store<f32>(out + 4, ny);
        store<f32>(out + 8, nz);
        ctx.fpr[1] = nz;
    }
}

namespace {
constexpr u32 kPerm74B770 = 0x0074B740;  // three vperm masks: {a0 b0 a0 b3}, {a0 a1 b2 b3}, {a0 a1 b0 b1}
constexpr u32 kMarkDirty3 = 0x0057D000;  // WorldObj_MarkDirty (o)
}  // namespace

// void f(WorldObj* o, Obj* a, const f32 m[4][4], u32 rgba) (called by
// 0012EB80): sets o's local matrix from world matrix m and its tint. The
// parent is o+260, or a (then a->vfn[5](a, o, -1) first); m goes through
// the parent's (+80) rigid inverse (the transposed rotation built with
// vperm from lvlx loads; -t's w lane comes from the uninitialised frame,
// read here at the same place) to o's 4x3 rows +16..+72, then
// WorldObj_MarkDirty. For kind 10 (bits 10..15 of +4) the colour goes
// big-endian to the +208 ->+112 ->+112 record's +20 (byte +6 >= 1) and +24
// (>= 2). f1 is left as the original leaves it (o+72).
DECOMP(0074B770, WorldObj_SetWorldMatrix_0074B770) {
    const u32 o = arg<u32>(ctx, 0), a = arg<u32>(ctx, 1), m = arg<u32>(ctx, 2), col = arg<u32>(ctx, 3);
    Stack frame(ctx, 304);
    const u32 sp = frame.addr();
    u32 p = load<u32>(o + 260);
    if (p == 0) {
        p = a;
        VCall(ctx, a, 20, {a, o, 0xFFFFFFFFu});
    }
    const u32 b = p + 80;
    const V4 P0 = Lvx(kPerm74B770), P1 = Lvx(kPerm74B770 + 16), P2 = Lvx(kPerm74B770 + 32);
    const V4 nt = Sub4(Zero4(), Perm(Perm(Lvlx(b + 48), Lvlx(b + 52), P0), Perm(Lvlx(b + 56), Lvx(sp + 160), P0), P1));
    const V4 m0 = Lvlx(b), m1 = Lvlx(b + 16), m2 = Lvlx(b + 32);
    const V4 c0 = Perm(Mrghw(m0, m1), Mrghw(m2, Lvlx(b + 20)), P2);
    const V4 c1 = Perm(Mrghw(Lvlx(b + 4), Lvlx(b + 20)), Mrghw(Lvlx(b + 36), m0), P2);
    const V4 c2 = Perm(Mrghw(Lvlx(b + 8), Lvlx(b + 24)), Mrghw(Lvlx(b + 40), Lvlx(b + 8)), P2);
    const Mat4 w = LoadMat(m);
    const V4 t = XformDir(c0, c1, c2, nt);
    Stvx(sp + 176, XformDir(c0, c1, c2, w.r[0]));
    Stvx(sp + 192, XformDir(c0, c1, c2, w.r[1]));
    Stvx(sp + 208, XformDir(c0, c1, c2, w.r[2]));
    Stvx(sp + 224, Add4(t, XformDir(c0, c1, c2, w.r[3])));
    for (u32 r = 0; r < 4; ++r)
        for (u32 k = 0; k < 3; ++k) CopyW(o + 16 + 16 * r + 4 * k, sp + 176 + 16 * r + 4 * k);
    const u32 last = load<u32>(sp + 232);
    call(ctx, kMarkDirty3, {o});
    SetF1Bits(ctx, last);
    if (((load<u32>(o + 4) >> 10) & 0x3F) != 10) return;
    const u32 q = load<u32>(load<u32>(load<u32>(o + 208) + 112) + 112);
    const u8 c3 = static_cast<u8>(col >> 24), c2b = static_cast<u8>(col >> 16), c1b = static_cast<u8>(col >> 8), c0b = static_cast<u8>(col);
    u32 n = load<u8>(q + 6);
    if (n != 0) {
        store<u8>(q + 20, c3);
        store<u8>(q + 21, c2b);
        store<u8>(q + 22, c1b);
        store<u8>(q + 23, c0b);
        n = load<u8>(q + 6);
    }
    if (n > 1) {
        store<u8>(q + 24, c3);
        store<u8>(q + 25, c2b);
        store<u8>(q + 26, c1b);
        store<u8>(q + 27, c0b);
    }
}

namespace {
constexpr u32 kSetMatrixD87CC = 0x000D87CC;  // Obj_SetMatrixD87CC (o, const Mat44*)
constexpr u32 kAddTimedCb = 0x0046B5F8;      // World_AddTimedCallback (World*, kind, opd, arg, c)
constexpr u32 kCbFAABC = 0x00F31548;         // the callback OPD queued
constexpr u32 kAnimAdvance = 0x0034C918;     // (anim; f1 dt)
constexpr u32 kFindComp34CC34 = 0x0034CC34;  // Obj_FindComp34CC34 (owner, key)
constexpr u32 kAnimDone = 0x00353EB0;        // (comp, 0) -> bool
}  // namespace

// u32 f(Obj* o; f32 dt) (a callback; no direct callers): per-frame
// transform update. The 112-byte block +480 is zeroed (vector stores),
// gets the identity, the bytes 0x01164CA8 at +576, then o's matrix (+96,
// 4x3 words) at +480 and +356 / +360 at +544 / +548. Without +336, the
// controller at +364 + 4 * byte +345 (when the byte and it are set) gets
// vfn[12](it, -, +480; dt). o's world object (+160) takes +16 via
// Obj_SetMatrixD87CC; World_AddTimedCallback(world, 0, 0x00F31548, o, o).
// With byte +604: the animation +608 advances by dt * +212, +624 = the
// +612 matrix (+80, 4x3 words) normalised, and +604 is cleared when its
// component (Obj_FindComp34CC34(+608, 0)) without bit 0x08000000 of +12
// reports done (0x00353EB0). Returns 1. f1 is left as the original leaves
// it.
DECOMP(000FAABC, Obj_UpdateXform_000FAABC) {
    const u32 o = arg<u32>(ctx, 0);
    const f64 dt = ctx.fpr[1];
    for (u32 k = 0; k < 7; ++k) Stvx(o + 480 + 16 * k, Zero4());
    for (u32 r = 0; r < 4; ++r)
        for (u32 k = 0; k < 3; ++k) CopyW(o + 480 + 16 * r + 4 * k, kIdentity4 + 16 * r + 4 * k);
    for (u32 k = 0; k < 4; ++k) store<u8>(o + 576 + k, load<u8>(0x01164CA8 + k));
    for (u32 r = 0; r < 4; ++r)
        for (u32 k = 0; k < 3; ++k) CopyW(o + 480 + 16 * r + 4 * k, o + 96 + 16 * r + 4 * k);
    CopyW(o + 544, o + 356);
    CopyW(o + 548, o + 360);
    u32 f1w = load<u32>(o + 152);
    bool f1dt = false;
    if (load<u32>(o + 336) == 0) {
        const u32 n = load<u8>(o + 345);
        if (n != 0) {
            const u32 x = load<u32>(o + 364 + ((n << 2) & 0x3FC));
            if (x != 0) {
                const u32 opd = load<u32>(load<u32>(x) + 48);
                ctx.fpr[1] = dt;
                call_ptr(ctx, opd, {x, opd, o + 480, load<u32>(opd)});
                f1dt = true;
            }
        }
    }
    const u32 wo = load<u32>(o + 160);
    if (wo != 0) call(ctx, kSetMatrixD87CC, {o, wo + 16});
    call(ctx, kAddTimedCb, {load<u32>(kV3WorldPtr), 0, kCbFAABC, o, o});
    if (load<u8>(o + 604) != 0) {
        callf(ctx, kAnimAdvance, {load<u32>(o + 608)}, {Muls(dt, F(o + 212))});
        const u32 s = load<u32>(o + 612) + 80;
        for (u32 r = 0; r < 4; ++r)
            for (u32 k = 0; k < 3; ++k) CopyW(o + 624 + 16 * r + 4 * k, s + 16 * r + 4 * k);
        f1w = load<u32>(s + 56);
        f1dt = false;
        call(ctx, kMatrixNormalize, {o + 624});
        const u32 c = static_cast<u32>(call(ctx, kFindComp34CC34, {load<u32>(o + 608), 0}));
        if (c != 0 && (load<u32>(c + 12) & 0x08000000u) == 0 && static_cast<u32>(call(ctx, kAnimDone, {c, 0})) != 0)
            store<u8>(o + 604, 0);
    }
    if (f1dt) ctx.fpr[1] = dt;
    else SetF1Bits(ctx, f1w);
    ret(ctx, 1u);
}

namespace {
constexpr u32 kPool30CE0 = 0x00030CB0;   // f32 1, 1/65535, 2, 0, pi/2; +32 vperm mask {a0 a1 b0 a3}
constexpr u32 kParamB0 = 0x00F94B74;     // param descriptor (+16 ->+64 slot index): a byte flag (stay put)
constexpr u32 kParamB1 = 0x00F94F64;     // param descriptor: the wander distance (f32)
constexpr u32 kNavClosest = 0x000147B8;  // Nav_ClosestPoint147B8 (out, o, target)
constexpr u32 kAxisAngle = 0x0056A410;   // Matrix_FromAxisAngle (out, axis; f1 angle)
constexpr u32 kGlob11AD1D4 = 0x011AD1D4;
}  // namespace

// void f(T* o) (called by 00031008 / 00034598 / 00039670): picks o's next
// wander goal. From the position +240: with the stay flag param
// (0x00F94B74 slot of +308 ->+12) set, +208 = 5.0 and the goal is the
// position; else +208 = 1.0 and by mode +456: <= 0 the position, 1 a new
// random xz direction (two inlined random steps, each mapped to [-1, 1],
// normalised with y 0) at +352, >= 2 the direction +352 turned a quarter
// turn about up (Matrix_FromAxisAngle; the vector store writes +364 too,
// from a stack word the original never sets, read here at the same place);
// the goal is the position plus the direction times the distance param
// (0x00F94F64 slot), then Nav_ClosestPoint147B8. +96 = the goal; with bit 4
// of byte +304 and a path +212 it is replaced by 0x006C0AA4(+212, &goal,
// *0x011AD1D4) / 0x006C1B94(&out, +212). f1 is left as the original leaves
// it.
DECOMP(00030CE0, Wander_PickGoal_00030CE0) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 K = kPool30CE0;
    Stack frame(ctx, 272);
    const u32 sp = frame.addr();
    CopyW(sp + 112, o + 240);
    CopyW(sp + 116, o + 244);
    CopyW(sp + 120, o + 248);
    u32 gx = load<u32>(o + 240), gy = load<u32>(o + 244), gz = load<u32>(o + 248);
    u32 f1w = gz;
    const u32 tab = load<u32>(load<u32>(o + 308) + 12);
    if (load<u8>(tab + (load<u32>(load<u32>(kParamB0 + 16) + 64) << 2)) != 0) {
        store<u32>(o + 208, 0x40A00000u);
    } else {
        store<u32>(o + 208, 0x3F800000u);
        const s32 mode = load<s32>(o + 456);
        if (mode > 0) {
            f32 dx, dy, dz;
            if (mode == 1) {
                const f32 one = F(K), sc = F(K + 4), two = F(K + 8), k0 = F(K + 12);
                // the inlined random pair on 0x01164888 (+0 u64 last value, +8 u32 seed):
                // a = the old u64; x = (rotl4(s) + rotl16(s)) ^ K, b = x & 0xFFFF; y =
                // (rotl20(x) + x) ^ K; seed = rotl16(y), u64 = y & 0xFFFF.
                const u64 a = load<u64>(0x01164888);
                const u32 sd = load<u32>(0x01164890);
                const u32 x = (std::rotl(sd, 4) + std::rotl(sd, 16)) ^ 0x87654321u;
                const u64 b = x & 0xFFFFu;
                const u32 y = (std::rotl(x, 20) + x) ^ 0x87654321u;
                store<u32>(0x01164890, std::rotl(y, 16));
                store<u64>(0x01164888, y & 0xFFFFu);
                const f32 ra = Fmsubs(Muls(static_cast<f32>(static_cast<f64>(static_cast<s64>(a))), sc), two, one);
                const f32 rb = Fmsubs(Muls(static_cast<f32>(static_cast<f64>(static_cast<s64>(b))), sc), two, one);
                const f32 inv = Divs(one, Sqrts(Fmadds(rb, rb, Fmadds(ra, ra, Muls(k0, k0)))));
                dx = Muls(ra, inv);
                dy = Muls(k0, inv);
                dz = Muls(rb, inv);
                store<f32>(o + 352, dx);
                store<f32>(o + 356, dy);
                store<f32>(o + 360, dz);
            } else {
                callf(ctx, kAxisAngle, {sp + 144, kV3Up}, {std::bit_cast<f64>(LfsBits(load<u32>(K + 16)))});
                CopyW(sp + 192, kV3Zero3);
                CopyW(sp + 196, kV3Zero3 + 4);
                CopyW(sp + 200, kV3Zero3 + 8);
                const V4 t = Perm(Lvx(sp + 192), Lvlx(kV3Zero3 + 8), Lvx(K + 32));
                const V4 v = Madd4(Lvx(sp + 176), SplatW(o + 360), Madd4(Lvx(sp + 160), SplatW(o + 356), Madd4(Lvx(sp + 144), SplatW(o + 352), Zero4())));
                Stvx(o + 352, Add4(t, v));
                dx = F(o + 352);
                dy = F(o + 356);
                dz = F(o + 360);
            }
            const f32 d = F(tab + (load<u32>(load<u32>(kParamB1 + 16) + 64) << 2));
            const f32 px = Fmadds(dx, d, std::bit_cast<f32>(gx)), py = Fmadds(dy, d, std::bit_cast<f32>(gy)), pz = Fmadds(dz, d, std::bit_cast<f32>(gz));
            store<f32>(sp + 112, px);
            store<f32>(sp + 116, py);
            store<f32>(sp + 120, pz);
            f1w = std::bit_cast<u32>(pz);
        }
        call(ctx, kNavClosest, {sp + 128, o, sp + 112});
        gx = load<u32>(sp + 128);
        gy = load<u32>(sp + 132);
        gz = load<u32>(sp + 136);
        store<u32>(sp + 112, gx);
        store<u32>(sp + 116, gy);
        store<u32>(sp + 120, gz);
    }
    store<u32>(o + 96, gx);
    store<u32>(o + 100, gy);
    store<u32>(o + 104, gz);
    if ((load<u8>(o + 304) & 0x10u) != 0 && load<u32>(o + 212) != 0) {
        store<u32>(sp + 224, gx);
        store<u32>(sp + 228, gy);
        store<u32>(sp + 232, gz);
        call(ctx, 0x006C0AA4, {load<u32>(o + 212), sp + 224, load<u32>(kGlob11AD1D4)});
        call(ctx, 0x006C1B94, {sp + 236, load<u32>(o + 212)});
        CopyW(o + 96, sp + 236);
        CopyW(o + 100, sp + 240);
        CopyW(o + 104, sp + 244);
        f1w = load<u32>(sp + 236);
    }
    SetF1Bits(ctx, f1w);
}

namespace {
constexpr u32 kPerm2008C0 = 0x00200890;  // three vperm masks: {a0 b0 a0 a3}, {b0 a1 a2 a3}, {a0 a1 a2 b3}
}  // namespace

// void f(Obj* o, Vec4* out) (called by 00202810): o's held-item point.
// Without +564: out = the zero vector (3 words). Else the def's (+216)
// point +352 (first copied to out) goes through the attachment's (+444)
// matrix (+80), or the held object's (+564) bone o+440 (Obj_BoneMatrixCE688
// world, raw; its 4x3
// words also copied to the frame at +112); the rows are built with vperm
// from splats and their w lanes come from those frame quadwords, which the
// attachment path never writes (read here at the same place). The vector
// store writes out+12 too. f1 / r3 are left as the original leaves them.
DECOMP(002008C0, Obj_HeldPoint_002008C0) {
    const u32 o = arg<u32>(ctx, 0), out = arg<u32>(ctx, 1);
    if (load<u32>(o + 564) == 0) {
        CopyW(out, kV3Zero3);
        CopyW(out + 4, kV3Zero3 + 4);
        CopyW(out + 8, kV3Zero3 + 8);
        SetF1Bits(ctx, load<u32>(kV3Zero3 + 8));
        ctx.gpr[3] = 0x01160000u;
        return;
    }
    const u32 held = load<u32>(o + 564);
    Stack frame(ctx, 272);
    const u32 sp = frame.addr();
    const u32 att = load<u32>(o + 444);
    const u32 def = load<u32>(o + 216);
    u32 src;
    if (att != 0) {
        src = att + 80;
    } else {
        call(ctx, kBoneMatrix, {sp + 176, held, static_cast<u64>(static_cast<s64>(static_cast<s32>(load<u32>(o + 440)))), 1, 1});
        for (u32 r = 0; r < 4; ++r)
            for (u32 k = 0; k < 3; ++k) CopyW(sp + 112 + 16 * r + 4 * k, sp + 176 + 16 * r + 4 * k);
        src = sp + 176;
    }
    f32 w[12];
    for (u32 r = 0; r < 4; ++r)
        for (u32 k = 0; k < 3; ++k) w[3 * r + k] = F(src + 16 * r + 4 * k);
    const V4 pad[4] = {Lvx(sp + 112), Lvx(sp + 128), Lvx(sp + 144), Lvx(sp + 160)};
    CopyW(out, def + 352);
    CopyW(out + 4, def + 356);
    CopyW(out + 8, def + 360);
    const V4 px = SplatW(def + 352), py = SplatW(def + 356), pz = SplatW(def + 360);
    const V4 K0 = Lvx(kPerm2008C0), K1 = Lvx(kPerm2008C0 + 16), K2 = Lvx(kPerm2008C0 + 32);
    V4 row[4];
    for (u32 r = 0; r < 4; ++r)
        row[r] = Perm(Perm(Perm(Fill4(w[3 * r + 2]), Fill4(w[3 * r + 1]), K0), Fill4(w[3 * r]), K1), pad[r], K2);
    Stvx(out, Add4(row[3], Madd4(row[2], pz, Madd4(row[1], py, Madd4(row[0], px, Zero4())))));
    SetF1Bits(ctx, std::bit_cast<u32>(w[0]));
    ctx.gpr[3] = kPerm2008C0;
}

namespace {
constexpr u32 kPermDF010 = 0x000DEFF0;  // two vperm masks (the transpose)
}  // namespace

// void f(Obj* o) (a callback; no direct callers): o's matrix from its body.
// With a body (+348): o+96 (4x3 words) = the rigid inverse of the def's
// (+216 ->+164) model matrix (+240) times the body's matrix (+240)
// (Matrix_Multiply), both first copied (4x3 words) to the frame; the
// inverse is built in place with vmrghw / vmrglw / vperm, whose w lanes
// come from frame words the copies never set (read here at the same
// place). f1 / r3 are left as the original leaves them.
DECOMP(000DF010, Obj_MatrixFromBody_000DF010) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 b = load<u32>(o + 348);
    if (b == 0) {
        ctx.gpr[3] = 0;
        return;
    }
    Stack frame(ctx, 256);
    const u32 sp = frame.addr();
    const u32 m = load<u32>(load<u32>(o + 216) + 164) + 240;
    for (u32 r = 0; r < 4; ++r)
        for (u32 k = 0; k < 3; ++k) CopyW(sp + 112 + 16 * r + 4 * k, m + 16 * r + 4 * k);
    for (u32 r = 0; r < 4; ++r)
        for (u32 k = 0; k < 3; ++k) CopyW(sp + 176 + 16 * r + 4 * k, b + 240 + 16 * r + 4 * k);
    const V4 nt = Sub4(Zero4(), Lvx(sp + 160));
    const V4 r0 = Lvx(sp + 112), r1 = Lvx(sp + 128), r2 = Lvx(sp + 144);
    const V4 h = Mrghw(r0, r2), l = Mrglw(r0, r2);
    const V4 c0 = Mrghw(h, r1), c1 = Perm(h, r1, Lvx(kPermDF010)), c2 = Perm(l, r1, Lvx(kPermDF010 + 16));
    Stvx(sp + 112, c0);
    Stvx(sp + 128, c1);
    Stvx(sp + 144, c2);
    Stvx(sp + 160, XformDir(c0, c1, c2, nt));
    call(ctx, kMatrixMultiply, {sp + 112, sp + 176});
    for (u32 r = 0; r < 4; ++r)
        for (u32 k = 0; k < 3; ++k) CopyW(o + 96 + 16 * r + 4 * k, sp + 112 + 16 * r + 4 * k);
    SetF1Bits(ctx, load<u32>(sp + 168));
}

namespace {
constexpr u32 kMasks1725C0 = 0x00172590;  // f32 0 / three lane masks
constexpr u32 kBodyOfs1725C0 = 0x010419A0;  // f32[3] added to the body's translation
constexpr u32 kHkSetXform3 = 0x0091EAB8;  // Hk_SetTransform91EAB8 (body, const f32 rows[4][4])
constexpr u32 kBodyMoved3 = 0x003556AC;   // Body_Moved_003556AC (body)
}  // namespace

// void f(Obj* o, const f32 m[4][4]) (a callback; no direct callers): sets
// o's matrix (+96, 4x3 words) and, with a body (+388) whose +8 is set,
// moves it: Hk_SetTransform91EAB8 with m's rows as {x, y, z, 0} and the
// translation plus 0x010419A0; then body +44 = +500 << 16 | 8, bit 0 of the
// u64 +184 is set, and Body_Moved_003556AC when the translation differs
// from the body's old one (+288) on any axis. The twin of
// Obj_SetWorldMatrix_00194160 (vmx2). f1 / r3 are left as the original
// leaves them.
DECOMP(001725C0, Obj_SetMatrixBody_001725C0) {
    const u32 o = arg<u32>(ctx, 0), m = arg<u32>(ctx, 1);
    for (u32 r = 0; r < 4; ++r)
        for (u32 k = 0; k < 3; ++k) CopyW(o + 96 + 16 * r + 4 * k, m + 16 * r + 4 * k);
    const u32 body = load<u32>(o + 388);
    if (body == 0 || load<u32>(body + 8) == 0) {
        SetF1Bits(ctx, load<u32>(m + 56));
        ctx.gpr[3] = m;
        return;
    }
    Stack frame(ctx, 320);
    const u32 sp = frame.addr();
    const f32 ox = F(body + 288), oy = F(body + 292), oz = F(body + 296);
    const f32 nx = F(m + 48), ny = F(m + 52), nz = F(m + 56);
    for (u32 r = 0; r < 3; ++r)
        for (u32 k = 0; k < 3; ++k) CopyW(sp + 176 + 16 * r + 4 * k, m + 16 * r + 4 * k);
    const f32 tx = Adds(nx, F(kBodyOfs1725C0)), ty = Adds(ny, F(kBodyOfs1725C0 + 4)), tz = Adds(nz, F(kBodyOfs1725C0 + 8));
    store<f32>(sp + 224, tx);
    store<f32>(sp + 228, ty);
    store<f32>(sp + 232, tz);
    const Masks3 mk = LoadMasks3(kMasks1725C0);
    Stvx(sp + 112, RowSelW(m, m + 4, m + 8, mk));
    Stvx(sp + 128, RowSelW(m + 16, sp + 196, sp + 200, mk));
    Stvx(sp + 144, RowSelW(m + 32, sp + 212, sp + 216, mk));
    Stvx(sp + 240, Fill4(tx));
    Stvx(sp + 160, RowSel(Fill4(tx), SplatW(sp + 228), SplatW(sp + 232), mk));
    call(ctx, kHkSetXform3, {body, sp + 112});
    const u32 b2 = load<u32>(o + 388);
    store<u32>(b2 + 44, (load<u32>(o + 500) << 16) | 0x8u);
    store<u64>(o + 184, load<u64>(o + 184) | 1u);
    const f32 zero = F(kMasks1725C0);
    SetF1Bits(ctx, load<u32>(kMasks1725C0));
    ctx.gpr[3] = b2;
    if (Subs(nx, ox) != zero || Subs(ny, oy) != zero || Subs(nz, oz) != zero) call(ctx, kBodyMoved3, {b2});
}
