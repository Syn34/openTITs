// Game code, medium sweep part 9: contact callbacks that turn a hit into a
// Havok impulse (three identical copies each, in different object classes;
// called through pointers).
#include "mid9.h"

using namespace sly4;
using namespace sly4::mid9;

namespace {

constexpr u32 Obj_TestE0BCC = 0x000E0BCC;       // (o, e) -> 0 to ignore the event
constexpr u32 Hk_Activate91A4C8 = 0x0091A4C8;   // (hkpEntity*) (reads r3 only)

// The copy's parameters: lit the literal pool before it (+0 the f32 k, +16
// / +32 / +48 the VMX masks of the vector build), offA / offB the f32
// fields of the class data o->216 tested against k, impulse = 1 for the
// first kind (two vectors, slot 88) or 2 (one vector, slot 84).
struct HitCfg {
    u32 lit, offA, offB;
    int kind;
};

f32 Muls(f32 a, f32 b) { return static_cast<f32>(static_cast<f64>(a) * static_cast<f64>(b)); }

// void Obj_OnHit(Obj* o, const HitEvent* e): unless Obj_TestE0BCC(o, e)
// says no: when o has a rigid body o->388 (not of motion type 4 / 5), the
// event's strength (kind 1: e->68, kind 2: e->36) is not k and the class
// data's f32 offA is not k, the body is activated and pushed (kind 1: the
// vector -e->32..40 * strength * offA through vfn 88 of the body's +224 with
// the point e->16..24; kind 2: e->16..24 * strength * offA through vfn 84).
// Then, when the class data's f32 offB > k, o's vfn 352(o, e->4, hash) with
// f1 = (kind 1: e->64, kind 2: e->32) * offB, and the bit 0x100 of the u16
// e->14 = whether that returned 0.
// r3 at exit: 0 when the test failed, the vfn 352 result's cntlzw, else
// lit (the literal pool address the original leaves in r3).
void ObjOnHit(PPUContext& ctx, const HitCfg& k, u32 hash) {
    const u32 o = arg<u32>(ctx, 0), e = arg<u32>(ctx, 1);
    const u32 d = load<u32>(o + 216);
    if (call(ctx, Obj_TestE0BCC, {o, e}) == 0) return ret(ctx, 0);
    const u32 kw = load<u32>(k.lit);
    const f32 kf = F32(kw);
    const u32 body = load<u32>(o + 388);
    if (body != 0) {
        const u32 mt = load<u8>(body + 232);
        const f32 s = F32(load<u32>(e + (k.kind == 1 ? 68 : 36)));
        if (mt != 5 && mt != 4 && !(s == kf)) {
            const f32 a = F32(load<u32>(d + k.offA));
            if (!(a == kf)) {
                Stack frame(ctx, 32);
                const u32 v1 = frame.addr(), v2 = v1 + 16;  // the original's r1 + 112 / + 128
                if (k.kind == 1) {
                    const f32 x = Muls(Muls(-F32(load<u32>(e + 32)), s), a);
                    const f32 y = Muls(Muls(-F32(load<u32>(e + 36)), s), a);
                    const f32 z = Muls(Muls(-F32(load<u32>(e + 40)), s), a);
                    Vec3Sel(v1, k.lit + 16, W32(x), W32(y), W32(z));
                    Vec3Sel(v2, k.lit + 16, load<u32>(e + 16), load<u32>(e + 20), load<u32>(e + 24));
                    call(ctx, Hk_Activate91A4C8, {body});
                    obj19::VCall(ctx, body + 224, 88, {body + 224, v1, v2});
                } else {
                    const f32 x = Muls(Muls(F32(load<u32>(e + 16)), s), a);
                    const f32 y = Muls(Muls(F32(load<u32>(e + 20)), s), a);
                    const f32 z = Muls(Muls(F32(load<u32>(e + 24)), s), a);
                    Vec3Sel(v1, k.lit + 16, W32(x), W32(y), W32(z));
                    call(ctx, Hk_Activate91A4C8, {body});
                    obj19::VCall(ctx, body + 224, 84, {body + 224, v1});
                }
            }
        }
    }
    const u32 bw = load<u32>(d + k.offB);
    ctx.fpr[1] = std::bit_cast<f64>(LfsBits(bw));
    ctx.fpr[2] = std::bit_cast<f64>(LfsBits(kw));
    if (!(F32(bw) > kf)) return ret(ctx, k.lit);
    const u32 ew = load<u32>(e + (k.kind == 1 ? 64 : 32));
    const f32 m = Muls(F32(ew), F32(bw));
    const u32 opd = load<u32>(load<u32>(o) + 352);
    ctx.fpr[1] = static_cast<f64>(m);
    ctx.fpr[2] = std::bit_cast<f64>(LfsBits(ew));
    const u32 r = static_cast<u32>(call_ptr(ctx, opd, {o, opd, load<u32>(e + 4), hash, load<u32>(opd)}));
    const u32 z = static_cast<u32>(std::countl_zero(r));
    store<u16>(e + 14, static_cast<u16>((load<u16>(e + 14) & ~0x100u) | ((z << 3) & 0x100u)));
    ret(ctx, z);
}

}  // namespace

// void Obj_OnHit168950(Obj* o, const HitEvent* e): ObjOnHit, kind 1
// (class data +200 / +196, the literal pool 0x00168910, hash 0x045D9554).
DECOMP(00168950, Obj_OnHit168950) { ObjOnHit(ctx, {0x00168910, 200, 196, 1}, 0x045D9554); }

// void Obj_OnHit178CA0(Obj* o, const HitEvent* e): as Obj_OnHit168950 with
// the class data +204 / +200 (literal pool 0x00178C60).
DECOMP(00178CA0, Obj_OnHit178CA0) { ObjOnHit(ctx, {0x00178C60, 204, 200, 1}, 0x045D9554); }

// void Obj_OnHit192750(Obj* o, const HitEvent* e): as Obj_OnHit168950
// (literal pool 0x00192710).
DECOMP(00192750, Obj_OnHit192750) { ObjOnHit(ctx, {0x00192710, 200, 196, 1}, 0x045D9554); }

// void Obj_OnHit168E50(Obj* o, const HitEvent* e): ObjOnHit, kind 2
// (class data +228 / +224, the literal pool 0x00168E10, hash 0x811B1D5E).
DECOMP(00168E50, Obj_OnHit168E50) { ObjOnHit(ctx, {0x00168E10, 228, 224, 2}, 0x811B1D5E); }

// void Obj_OnHit1791A0(Obj* o, const HitEvent* e): as Obj_OnHit168E50 with
// the class data +232 / +228 (literal pool 0x00179160).
DECOMP(001791A0, Obj_OnHit1791A0) { ObjOnHit(ctx, {0x00179160, 232, 228, 2}, 0x811B1D5E); }

// void Obj_OnHit192C50(Obj* o, const HitEvent* e): as Obj_OnHit168E50
// (literal pool 0x00192C10).
DECOMP(00192C50, Obj_OnHit192C50) { ObjOnHit(ctx, {0x00192C10, 228, 224, 2}, 0x811B1D5E); }

namespace {

constexpr u32 Hk_SetTransform91EAB8 = 0x0091EAB8;  // (hkpEntity*, const Vec4 m[4]) (regflow: reads r3)
constexpr u32 Body_Moved3556AC = 0x003556AC;      // (body) (reads r3 only)

// void Obj_SetMatrix(Obj* o, const Matrix4* m): the rows 0..3 (xyz) copied
// to o+96 / +112 / +128 / +144; when o has a rigid body o->388, the matrix
// is handed to Hk_SetTransform91EAB8 (as four {x, y, z, 0} vectors built
// with the masks at lit / lit + 16 / lit + 32), and Body_Moved3556AC(body)
// follows when the translation differs from the body's old position
// (+288..+296) by other than the f32 at lit (fsubs / fcmpu, NaN = moved).
// tol: the body is also skipped when its +8 is 0, and "moved" is
// |(t - old) - the zero vector 0x01164B40| not below the f32 at lit + 48.
void ObjSetMatrix(PPUContext& ctx, u32 lit, u32 bodyOff = 388, bool tol = false) {
    const u32 o = arg<u32>(ctx, 0), m = arg<u32>(ctx, 1);
    constexpr u32 kZero = 0x01164B40;
    for (u32 r = 0; r < 4; ++r)
        for (u32 k = 0; k < 3; ++k) CopyW(o + 96 + 16 * r + 4 * k, m + 16 * r + 4 * k);
    ctx.fpr[1] = std::bit_cast<f64>(LfsBits(load<u32>(m + 56)));
    const u32 body = load<u32>(o + bodyOff);
    if (body == 0 || (tol && load<u32>(body + 8) == 0)) return ret(ctx, m);
    const f32 ox = F32(load<u32>(body + 288)), oy = F32(load<u32>(body + 292)), oz = F32(load<u32>(body + 296));
    const f32 tx = F32(load<u32>(m + 48)), ty = F32(load<u32>(m + 52)), tz = F32(load<u32>(m + 56));
    Stack frame(ctx, 64);
    const u32 v = frame.addr();  // the original's r1 + 112
    for (u32 r = 0; r < 4; ++r) Vec3Sel(v + 16 * r, lit, load<u32>(m + 16 * r), load<u32>(m + 16 * r + 4), load<u32>(m + 16 * r + 8));
    call(ctx, Hk_SetTransform91EAB8, {body, v});
    if (tol) {
        const u32 ew = load<u32>(lit + 48);
        const f32 eps = F32(ew);
        ctx.fpr[1] = std::bit_cast<f64>(LfsBits(ew));
        const u32 b2 = load<u32>(o + bodyOff);
        const f32 t[3] = {tx, ty, tz}, old[3] = {ox, oy, oz};
        bool moved = false;
        for (u32 i = 0; i < 3; ++i) {
            const f32 z = F32(load<u32>(kZero + 4 * i));
            ctx.fpr[3] = static_cast<f64>(z);
            const f32 dd = std::fabs(static_cast<f32>(static_cast<f64>(static_cast<f32>(static_cast<f64>(t[i]) - old[i])) - z));
            ctx.fpr[2] = static_cast<f64>(dd);
            if (!(dd < eps)) {
                moved = true;
                break;
            }
        }
        if (moved)
            call(ctx, Body_Moved3556AC, {b2});
        else
            ret(ctx, kZero);  // the address the original leaves in r3
        return;
    }
    const u32 kw = load<u32>(lit);
    const f32 k = F32(kw);
    ctx.fpr[1] = std::bit_cast<f64>(LfsBits(kw));
    const u32 b2 = load<u32>(o + bodyOff);
    f32 dd = static_cast<f32>(static_cast<f64>(tx) - ox);
    bool moved = !(dd == k);
    if (!moved) {
        dd = static_cast<f32>(static_cast<f64>(ty) - oy);
        moved = !(dd == k);
        if (!moved) {
            dd = static_cast<f32>(static_cast<f64>(tz) - oz);
            moved = !(dd == k);
        }
    }
    ctx.fpr[2] = static_cast<f64>(dd);
    if (moved) call(ctx, Body_Moved3556AC, {b2});
}

}  // namespace

// void Obj_SetMatrix169AB0(Obj* o, const Matrix4* m): ObjSetMatrix with the
// literal pool 0x00169A80.
DECOMP(00169AB0, Obj_SetMatrix169AB0) { ObjSetMatrix(ctx, 0x00169A80); }

// void Obj_SetMatrix188040(Obj* o, const Matrix4* m): ObjSetMatrix with the
// literal pool 0x00188010.
DECOMP(00188040, Obj_SetMatrix188040) { ObjSetMatrix(ctx, 0x00188010); }

// void Obj_SetMatrix17BD28(Obj* o, const Matrix4* m): ObjSetMatrix with the
// literal pool 0x0017BCF0, the tolerance test (the body +8 checked).
DECOMP(0017BD28, Obj_SetMatrix17BD28) { ObjSetMatrix(ctx, 0x0017BCF0, 388, true); }

// void Obj_SetMatrix1FD2C8(Obj* o, const Matrix4* m): as
// Obj_SetMatrix17BD28 with the body at +416 (literal pool 0x001FD290).
DECOMP(001FD2C8, Obj_SetMatrix1FD2C8) { ObjSetMatrix(ctx, 0x001FD290, 416, true); }

namespace {

// u32 Obj_WakeBodies(Owner* o): for each entry e of the PtrVector o+64
// (u16 count +72, re-read each pass; null entries skipped): an entry with
// the flag 2 in +192 is passed over when its vfn 320(e) says so (non-zero);
// one with the flag 0x2000 in +192 gets: vfn 144(e)->+510 (u16) = 16668,
// then its body b = vfn 144(e) activated (Hk_Activate91A4C8) and given the
// vector v (vfn 68 of b+224, (b+224, &v)), and the flag 0x80000 in +624.
// v = splat(lit[0]) & mask lit+16 & ~mask lit+32 & ~mask lit+48 (VMX).
// Then the u16 counts +72 / +92 / +112 / +132 = 0. Returns the count last
// read.
void ObjWakeBodies(PPUContext& ctx, u32 lit) {
    const u32 o = arg<u32>(ctx, 0);
    u32 vw[4];
    for (u32 i = 0; i < 4; ++i)
        vw[i] = load<u32>(lit) & load<u32>(lit + 16 + 4 * i) & ~load<u32>(lit + 32 + 4 * i) & ~load<u32>(lit + 48 + 4 * i);
    Stack frame(ctx, 16);
    const u32 v = frame.addr();  // the original's r1 + 112
    u32 n;
    for (u32 i = 0; i < (n = load<u16>(o + 72)); ++i) {
        const u32 e = load<u32>(load<u32>(o + 64) + 4 * i);
        if (e == 0) continue;
        if ((load<u32>(e + 192) & 2u) != 0 && obj19::VCall(ctx, e, 320, {e}) != 0) continue;
        if ((load<u32>(e + 192) & 0x2000u) == 0) continue;
        const u32 r = static_cast<u32>(obj19::VCall(ctx, e, 144, {e}));
        store<u16>(r + 510, 16668);
        const u32 b = static_cast<u32>(obj19::VCall(ctx, e, 144, {e}));
        for (u32 k = 0; k < 4; ++k) store<u32>(v + 4 * k, vw[k]);
        call(ctx, Hk_Activate91A4C8, {b});
        obj19::VCall(ctx, b + 224, 68, {b + 224, v});
        store<u32>(e + 624, load<u32>(e + 624) | 0x00080000u);
    }
    store<u16>(o + 72, 0);
    store<u16>(o + 92, 0);
    store<u16>(o + 112, 0);
    store<u16>(o + 132, 0);
    ret(ctx, n);
}

}  // namespace

// u32 Obj_WakeBodies2B2FC0(Owner* o): ObjWakeBodies with the literal pool
// 0x002B2F80.
DECOMP(002B2FC0, Obj_WakeBodies2B2FC0) { ObjWakeBodies(ctx, 0x002B2F80); }

// u32 Obj_WakeBodies2B5620(Owner* o): ObjWakeBodies with the literal pool
// 0x002B55E0.
DECOMP(002B5620, Obj_WakeBodies2B5620) { ObjWakeBodies(ctx, 0x002B55E0); }
