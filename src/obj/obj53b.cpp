// Game code, part 53 (b): small functions the size sweeps skipped because
// they use VMX, smallest first. Built on the vmx7.h lane helpers (the
// runtime's evaluation: fused vmaddfp, 1 / sqrtf for vrsqrtefp). Each was
// checked against the original in the PPC simulator (memory writes, calls
// and their arguments, r3 and f1).
#include "vmx7.h"

using namespace sly4;
using namespace sly4::vmx7;

// The f32 at ea (lvlx + vspltw 0, lane 0).
static f32 S53(u32 ea) { return Lvlx(ea).e[0]; }

// v * m for the row vector of the three f32s at v and the three rows m,
// m + 16, m + 32 (lvx): m2 * z + (m1 * y + (m0 * x + 0)), fused.
static V4 MulRows3_53(u32 v, u32 m) { return Row3(Lvx(m), Lvx(m + 16), Lvx(m + 32), S53(v), S53(v + 4), S53(v + 8)); }

// The aligned quadword at src copied to the one at dst as four raw words.
static void CopyQ53(u32 dst, u32 src) {
    for (u32 k = 0; k < 4; ++k) store<u32>((dst & ~15u) + 4 * k, load<u32>((src & ~15u) + 4 * k));
}

// The body's vector setter: the {x, y, z, 0} row of the three f32s at v
// (RowSelW with the pool masks at k) on the stack (sp+112),
// Hk_Activate91A4C8(rb), then (rb + 224)->vt[slot](rb + 224, &row); r3 =
// its result.
static void HkSetVec53(PPUContext& ctx, u32 rb, u32 v, u32 k, u32 slot) {
    Stack fr(ctx, 16);
    Stvx(fr.addr(), RowSelW(v, v + 4, v + 8, LoadMasks3(k)));
    call(ctx, 0x0091A4C8u, {rb});
    const u32 b = rb + 224;
    ctx.gpr[3] = VCall(ctx, b, slot, {b, fr.addr()});
}

// out = a * b for the 4x4 matrices a, b (rows of 16 bytes): each row of a
// weights the rows b0..b2 by its x / y / z (Row3), row 3 adds b3 (vaddfp
// b3 + the sum); all rows loaded before the four stores (so out may be a).
static void MulMat4To53(u32 out, u32 a, u32 b) {
    const V4 a0 = Lvx(a), a1 = Lvx(a + 16), a2 = Lvx(a + 32), a3 = Lvx(a + 48);
    const V4 b0 = Lvx(b), b1 = Lvx(b + 16), b2 = Lvx(b + 32), b3 = Lvx(b + 48);
    const V4 r3 = Add4(b3, Row3(b0, b1, b2, a3.e[0], a3.e[1], a3.e[2]));
    Stvx(out, Row3(b0, b1, b2, a0.e[0], a0.e[1], a0.e[2]));
    Stvx(out + 16, Row3(b0, b1, b2, a1.e[0], a1.e[1], a1.e[2]));
    Stvx(out + 32, Row3(b0, b1, b2, a2.e[0], a2.e[1], a2.e[2]));
    Stvx(out + 48, r3);
}

// The point v (three f32s, lvlx each) through the 4x4 matrix m: m3 + v *
// m (vaddfp m3 + the Row3 sum).
static V4 XformPt53(u32 v, u32 m) { return Add4(Lvx(m + 48), Row3(Lvx(m), Lvx(m + 16), Lvx(m + 32), S53(v), S53(v + 4), S53(v + 8))); }

// Three raw words from src to dst (lfs / stfs pairs); f1 = the last one.
static void Copy3F53(PPUContext& ctx, u32 dst, u32 src) {
    for (u32 k = 0; k < 3; ++k) store<u32>(dst + 4 * k, load<u32>(src + 4 * k));
    ctx.fpr[1] = static_cast<f64>(load<f32>(src + 8));
}

// The rigid inverse of the 4x4 matrix at m (InvRigid4 with the two vperm
// masks at p) stored as four rows at out.
static void InvRigidTo53(u32 out, u32 m, u32 p) {
    const InvRows4 r = InvRigid4(m, Lvx(p), Lvx(p + 16));
    Stvx(out, r.c[0]);
    Stvx(out + 16, r.c[1]);
    Stvx(out + 32, r.c[2]);
    Stvx(out + 48, r.t);
}

// ---------------------------------------------------------------- VMX: 21 to 28 instructions
// void f(V4* out, const f32 v[3], const f32 m[3][4]): out (stvx) = v * m
// (MulRows3_53: the three rows weighted by x / y / z).
DECOMP(0074BC04, Vec3_MulRows3_0074BC04) { Stvx(arg<u32>(ctx, 0), MulRows3_53(arg<u32>(ctx, 1), arg<u32>(ctx, 2))); }

// void f(V4* v, const f32 m[3][4]): v = v * m in place (MulRows3_53).
DECOMP(0074C3D8, Vec3_MulRows3InPlace_0074C3D8) { Stvx(arg<u32>(ctx, 0), MulRows3_53(arg<u32>(ctx, 0), arg<u32>(ctx, 1))); }

// void f(V4* v, const f32 m[4][4]): v = m3 + v * m (the point transform,
// in place; vaddfp m3 + the MulRows3_53 sum).
DECOMP(00748B54, Vec3_XformPoint_00748B54) {
    const u32 v = arg<u32>(ctx, 0), m = arg<u32>(ctx, 1);
    Stvx(v, Add4(Lvx(m + 48), MulRows3_53(v, m)));
}

// void f(V4* out, const f32 v[3], const f32 m[4][4]): out = m3 + v * m.
DECOMP(0074C49C, Vec3_XformPointTo_0074C49C) {
    const u32 v = arg<u32>(ctx, 1), m = arg<u32>(ctx, 2);
    Stvx(arg<u32>(ctx, 0), Add4(Lvx(m + 48), MulRows3_53(v, m)));
}

// void f(V4* v, const f32 m[4][4]): Vec3_XformPoint_00748B54 (the same code).
DECOMP(00760D58, Vec3_XformPoint_00760D58) {
    const u32 v = arg<u32>(ctx, 0), m = arg<u32>(ctx, 1);
    Stvx(v, Add4(Lvx(m + 48), MulRows3_53(v, m)));
}

// void f(V4* out, const f32 s[4]): out (stvx) = the four f32s of s merged
// into one vector with the three lane masks at 0x002BCCE0 (splat s0, vsel
// s1 / s2 / s3).
DECOMP(002BCD10, Vec4_FromFloats_002BCD10) {
    const u32 s = arg<u32>(ctx, 1);
    constexpr u32 K = 0x002BCCE0u;
    const V4 v = Sel(Sel(Sel(SplatW(s), SplatW(s + 4), Lvx(K)), SplatW(s + 8), Lvx(K + 16)), SplatW(s + 12), Lvx(K + 32));
    Stvx(arg<u32>(ctx, 0), v);
}

// void f(T* o) (init): +0 = 2, +48 = 0, byte +52 = 0, the quadword at +32
// = 0 and the one at +16 = the f32 at 0x00760330 splatted, masked with the
// pool masks at 0x00760340 (kept) and 0x00760350 (cleared) (the zero
// vector built from 0 & ~0x00760320 first).
DECOMP(00760360, Obj_InitVec_00760360) {
    const u32 o = arg<u32>(ctx, 0);
    constexpr u32 K = 0x00760320u;
    const V4 z = Andc(Zero4(), Lvx(K));
    store<u32>(o, 2);
    const V4 m1 = Lvx(K + 32), m2 = Lvx(K + 48);
    const V4 a = Andc(Andc(z, m1), m2);
    const V4 b = Andc(Sel(z, SplatW(K + 16), m1), m2);
    store<u32>(o + 48, 0);
    store<u8>(o + 52, 0);
    Stvx(o + 32, a);
    Stvx(o + 16, b);
}

// void f(V4* out, const V4* v): out (all four lanes) = |v.xyz| by the
// compiler's rsqrte + Newton step (SqrtNewton4 with the pool f32s
// 0x0074D820 (0.5) / 0x0074D824 (1.0)).
DECOMP(0074D828, Vec3_Length4_0074D828) {
    const V4 v = Lvx(arg<u32>(ctx, 1));
    constexpr u32 K = 0x0074D820u;
    Stvx(arg<u32>(ctx, 0), SqrtNewton4(HSum3(Madd4(v, v, Zero4())), SplatW(K), SplatW(K + 4)));
}

// ---------------------------------------------------------------- VMX: 33 to 54 instructions
// void f(f32 m[4][4]): m = its rigid inverse in place (InvRigidTo53: the
// transposed 3x3 from vmrghw / vmrglw / vperm with the masks at 0x007341C0,
// translation -t through it).
DECOMP(007341E0, Mat4_InvertRigid_007341E0) { InvRigidTo53(arg<u32>(ctx, 0), arg<u32>(ctx, 0), 0x007341C0u); }

// void f(f32 out[4][4], const f32 m[4][4]): out = m's rigid inverse
// (InvRigidTo53, masks at 0x0074D270).
DECOMP(0074D290, Mat4_InvertRigidTo_0074D290) { InvRigidTo53(arg<u32>(ctx, 0), arg<u32>(ctx, 1), 0x0074D270u); }

// u32 f(Collector* a, Collidable* b, const Hit* c) (ray hit callback):
// when f32 c->+16 < a->+40: the root of b (followed along +12) goes to
// HkFilter_TestCollidable_00884DC8(&ok (sp+112), a->+32, a->+36,
// root->+28); when ok: byte a->+44 = 1, the quadword of c copied to a +16,
// a->+40 = c->+16 (r3 = 16, f1 = c->+16). Otherwise r3 = a (fraction not
// lower) or ok's 0.
DECOMP(00771BF0, RayCollector_AddHit_00771BF0) {
    const u32 a = arg<u32>(ctx, 0), b = arg<u32>(ctx, 1), c = arg<u32>(ctx, 2);
    ctx.fpr[1] = static_cast<f64>(load<f32>(c + 16));
    if (!(static_cast<f64>(load<f32>(c + 16)) < static_cast<f64>(load<f32>(a + 40)))) return;
    u32 root = b;
    while (load<u32>(root + 12) != 0) root = load<u32>(root + 12);
    Stack fr(ctx, 16);
    call(ctx, 0x00884DC8u, {fr.addr(), load<u32>(a + 32), load<u32>(a + 36), load<u32>(root + 28), root});
    const u32 ok = load<u8>(fr.addr());
    if (ok == 0) {
        ret(ctx, 0u);
        return;
    }
    store<u8>(a + 44, 1);
    CopyQ53(a + 16, c);
    store<u32>(a + 40, load<u32>(c + 16));
    ctx.fpr[1] = static_cast<f64>(load<f32>(c + 16));
    ret(ctx, 16u);
}

// u32 f(T* o, const f32 v[3]): with the rigid body o->+388: HkSetVec53
// (masks 0x000ED650, vt[68]: the linear velocity?). None: r3 = v.
DECOMP(000ED680, Body_SetVec68_000ED680) {
    const u32 rb = load<u32>(arg<u32>(ctx, 0) + 388), v = arg<u32>(ctx, 1);
    if (rb == 0) {
        ret(ctx, v);
        return;
    }
    HkSetVec53(ctx, rb, v, 0x000ED650u, 68);
}

// u32 f(T* o, const f32 v[3]): with the rigid body o->+396: HkSetVec53
// (masks 0x00243FC0, vt[68]). None: r3 = v.
DECOMP(00243FF0, Body_SetVec68_00243FF0) {
    const u32 rb = load<u32>(arg<u32>(ctx, 0) + 396), v = arg<u32>(ctx, 1);
    if (rb == 0) {
        ret(ctx, v);
        return;
    }
    HkSetVec53(ctx, rb, v, 0x00243FC0u, 68);
}

// u32 f(T* o, const f32 v[3]): with the rigid body o->+396: HkSetVec53
// (masks 0x002441B0, vt[72]: the angular velocity?). None: r3 = v.
DECOMP(002441E0, Body_SetVec72_002441E0) {
    const u32 rb = load<u32>(arg<u32>(ctx, 0) + 396), v = arg<u32>(ctx, 1);
    if (rb == 0) {
        ret(ctx, v);
        return;
    }
    HkSetVec53(ctx, rb, v, 0x002441B0u, 72);
}

// u32 f(T* o, const f32 v[3]): with the rigid body (o->+612)->+24:
// HkSetVec53 (masks 0x0023C6E0, vt[68]). None: r3 = v.
DECOMP(0023C710, Body_SetVec68_0023C710) {
    const u32 h = load<u32>(arg<u32>(ctx, 0) + 612), v = arg<u32>(ctx, 1);
    const u32 rb = h != 0 ? load<u32>(h + 24) : 0;
    if (rb == 0) {
        ret(ctx, v);
        return;
    }
    HkSetVec53(ctx, rb, v, 0x0023C6E0u, 68);
}

// u32 f(T* o, const f32 v[3]): with the rigid body (o->+612)->+24:
// HkSetVec53 (masks 0x0023C820, vt[72]). None: r3 = v.
DECOMP(0023C850, Body_SetVec72_0023C850) {
    const u32 h = load<u32>(arg<u32>(ctx, 0) + 612), v = arg<u32>(ctx, 1);
    const u32 rb = h != 0 ? load<u32>(h + 24) : 0;
    if (rb == 0) {
        ret(ctx, v);
        return;
    }
    HkSetVec53(ctx, rb, v, 0x0023C820u, 72);
}

// void f(f32 a[4][4], const f32 b[4][4]): a = a * b (MulMat4To53).
DECOMP(007557C4, Mat4_MulInPlace_007557C4) { MulMat4To53(arg<u32>(ctx, 0), arg<u32>(ctx, 0), arg<u32>(ctx, 1)); }

// void f(f32 a[4][4], const f32 b[4][4]): a = a * b (MulMat4To53).
DECOMP(0075D154, Mat4_MulInPlace_0075D154) { MulMat4To53(arg<u32>(ctx, 0), arg<u32>(ctx, 0), arg<u32>(ctx, 1)); }

// ---------------------------------------------------------------- VMX: 56 to 61 instructions
// u32 f(Obj* o, const f32 v[3]; f32 t): with the rigid body o->+388 and t
// not <= the f32 0x00175BF8: o->vt[136](o, &d) with d (sp+128) = (v - the
// body's position (the quadword +288)) * (0x00175BFC / t), single steps
// (f1 = d.z). Otherwise r3 = v.
DECOMP(00175C00, Obj_SetVelocityTo_00175C00) {
    const u32 o = arg<u32>(ctx, 0), v = arg<u32>(ctx, 1);
    const u32 rb = load<u32>(o + 388);
    const f64 t = ctx.fpr[1];
    constexpr u32 K = 0x00175BF8u;
    if (rb == 0 || t <= static_cast<f64>(load<f32>(K))) {
        ret(ctx, v);
        return;
    }
    const V4 q = Lvx(rb + 288);
    const f64 inv = static_cast<f32>(static_cast<f64>(load<f32>(K + 4)) / t);
    Stack fr(ctx, 32);
    const u32 d = fr.addr() + 16;  // sp+128
    f32 r[3];
    for (u32 k = 0; k < 3; ++k) {
        r[k] = static_cast<f32>(static_cast<f64>(static_cast<f32>(static_cast<f64>(load<f32>(v + 4 * k)) - static_cast<f64>(q.e[k]))) * inv);
        store<f32>(d + 4 * k, r[k]);
    }
    ctx.fpr[1] = static_cast<f64>(r[2]);
    ctx.gpr[3] = VCall(ctx, o, 136, {o, d});
}

// void f(f32 out[3], Obj* a, const T* b): out = b's position (+48) when b
// is set, else a's foot point ((a->+216)->+704) through a's matrix (+96)
// (XformPt53; the vector goes through sp+48). f1 = out.z; r3 = out.
DECOMP(00228FA0, Obj_GetFootPos_00228FA0) {
    const u32 out = arg<u32>(ctx, 0), a = arg<u32>(ctx, 1), b = arg<u32>(ctx, 2);
    if (b != 0) {
        Copy3F53(ctx, out, b + 48);
        return;
    }
    const V4 p = XformPt53(load<u32>(a + 216) + 240 + 464, a + 96);
    for (u32 k = 0; k < 3; ++k) store<u32>(out + 4 * k, LaneW(p, k));
    ctx.fpr[1] = static_cast<f64>(p.e[2]);
}

// void f(f32 out[4][4], const f32 a[4][4], const f32 b[4][4]): out = a * b
// (MulMat4To53).
DECOMP(00761308, Mat4_Mul_00761308) { MulMat4To53(arg<u32>(ctx, 0), arg<u32>(ctx, 1), arg<u32>(ctx, 2)); }

// void f(Obj* o, f32 out[3]): out = the zero vector 0x01164B40 first;
// with the attachment o->+524: its node's (+8) position (+48) copied to
// out, then out (stvx) = that point through o's matrix ((o->+160) + 80,
// XformPt53; r3 = the matrix + 48, f1 = the point's z); without: out =
// (o->+516) + 128's three words (r3 = that address, f1 = its z).
DECOMP(001B18A0, Obj_GetAttachPos_001B18A0) {
    const u32 o = arg<u32>(ctx, 0), out = arg<u32>(ctx, 1);
    for (u32 k = 0; k < 3; ++k) store<u32>(out + 4 * k, load<u32>(0x01164B40u + 4 * k));
    const u32 at = load<u32>(o + 524);
    if (at == 0) {
        const u32 src = load<u32>(o + 516) + 128;
        Copy3F53(ctx, out, src);
        ret(ctx, src);
        return;
    }
    const u32 s = load<u32>(at + 8) + 48, m = load<u32>(o + 160) + 80;
    Copy3F53(ctx, out, s);
    Stvx(out, XformPt53(s, m));
    ret(ctx, m + 48);
}

// u32 f(Obj* o): Obj_Refresh_000E158C(o); with the rigid body o->+576 (its
// +8 set): HkSetVec53 of the three f32s at o + 560 (masks 0x001B8EF0,
// vt[68]). Otherwise r3 as left (the refresh result or +8's 0).
DECOMP(001B8F20, Obj_PushVelocity_001B8F20) {
    const u32 o = arg<u32>(ctx, 0);
    ctx.gpr[3] = call(ctx, 0x000E158Cu, {o});
    const u32 rb = load<u32>(o + 576);
    if (rb == 0) return;
    if (load<u32>(rb + 8) == 0) {
        ret(ctx, 0u);
        return;
    }
    HkSetVec53(ctx, rb, o + 560, 0x001B8EF0u, 68);
}

// u32 f(T* o, const f32 v[3]): the rigid body o->+388 (not checked for 0):
// unless its +8 is 0 or its u8 +232 (motion type?) is 4 or 5, HkSetVec53
// (masks 0x00206E50, vt[68]). Otherwise r3 = v.
DECOMP(00206E80, Body_SetVec68_00206E80) {
    const u32 rb = load<u32>(arg<u32>(ctx, 0) + 388), v = arg<u32>(ctx, 1);
    const u32 mt = load<u8>(rb + 232);
    if (load<u32>(rb + 8) == 0 || mt == 5 || mt == 4) {
        ret(ctx, v);
        return;
    }
    HkSetVec53(ctx, rb, v, 0x00206E50u, 68);
}

// u32 f(T* o, const f32 v[3]): Body_SetVec68_00206E80 with the masks
// 0x00206FC0 and vt[72].
DECOMP(00206FF0, Body_SetVec72_00206FF0) {
    const u32 rb = load<u32>(arg<u32>(ctx, 0) + 388), v = arg<u32>(ctx, 1);
    const u32 mt = load<u8>(rb + 232);
    if (load<u32>(rb + 8) == 0 || mt == 5 || mt == 4) {
        ret(ctx, v);
        return;
    }
    HkSetVec53(ctx, rb, v, 0x00206FC0u, 72);
}

// u32 f(T* o, const f32 v[3]): with the rigid body o->+416, its u8 +232
// not 4 / 5 and its +8 set: HkSetVec53 (masks 0x001FB4E0, vt[72]).
// Otherwise r3 = v.
DECOMP(001FB510, Body_SetVec72_001FB510) {
    const u32 rb = load<u32>(arg<u32>(ctx, 0) + 416), v = arg<u32>(ctx, 1);
    if (rb == 0) {
        ret(ctx, v);
        return;
    }
    const u32 mt = load<u8>(rb + 232);
    if (mt == 5 || mt == 4 || load<u32>(rb + 8) == 0) {
        ret(ctx, v);
        return;
    }
    HkSetVec53(ctx, rb, v, 0x001FB4E0u, 72);
}

// u32 f(Link* a, const Arg* b): with a's rigid body a->+24 (its +8 set):
// 0x000EF750(&p (sp+128), a->+16) gives a point, its {x, y, z, 0} row
// (masks 0x00748850) at sp+112; Hk_Activate91A4C8(rb), then (rb +
// 224)->vt[96](rb + 224, r4 = the OPD as loaded, &row) with f1 = f32
// b->+32. Otherwise r3 = a->+16.
DECOMP(00748880, Body_ApplyAt_00748880) {
    const u32 a = arg<u32>(ctx, 0), b = arg<u32>(ctx, 1);
    const u32 rb = load<u32>(a + 24), x = load<u32>(a + 16);
    if (load<u32>(rb + 8) == 0) {
        ret(ctx, x);
        return;
    }
    Stack fr(ctx, 32);
    const u32 row = fr.addr(), p = fr.addr() + 16;  // sp+112 / sp+128
    call(ctx, 0x000EF750u, {p, x});
    Stvx(row, RowSelW(p, p + 4, p + 8, LoadMasks3(0x00748850u)));
    const f64 f = static_cast<f64>(load<f32>(b + 32));
    call(ctx, 0x0091A4C8u, {rb});
    const u32 h = rb + 224;
    const u32 opd = load<u32>(load<u32>(h) + 96);
    ctx.fpr[1] = f;
    ctx.gpr[3] = call_ptr(ctx, opd, {h, opd, row, load<u32>(opd)});
}

// u32 f(T* o): with the owner o->+8 and its matrix holder m = owner->+160:
// unless bit 0x80000000 of o->+248 is set, o->+224..+232 = the position
// (+112) of o->+240 (none: of m); with the bit, o->+224.. = the three
// words at (o->+244)->+8 + 32, then the quadword at o + 224 (stvx) = that
// direction through m's rows +80 / +96 / +112 (Row3, no translation; r3 =
// o + 224). f1 = the last word copied.
DECOMP(00290084, Obj_UpdateAimDir_00290084) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 ow = load<u32>(o + 8);
    if (ow == 0) return;
    const u32 m = load<u32>(ow + 160);
    if (m == 0) return;
    if ((load<u32>(o + 248) & 0x80000000u) == 0) {
        const u32 t = load<u32>(o + 240);
        Copy3F53(ctx, o + 224, (t != 0 ? t : m) + 112);
        return;
    }
    const u32 s = load<u32>(load<u32>(o + 244) + 8) + 32;
    Copy3F53(ctx, o + 224, s);
    Stvx(o + 224, Row3(Lvx(m + 80), Lvx(m + 96), Lvx(m + 112), S53(s), S53(s + 4), S53(s + 8)));
    ret(ctx, o + 224);
}

// ---------------------------------------------------------------- VMX: 70 to 77 instructions
// u32 f(T* o, const f32 v[3]): +1168..+1176 = v and +1184..+1192 = those
// (raw words; f1 = the last); with the rigid body o->+416 (its +8 set):
// HkSetVec53 of o + 1184 (masks 0x0024D870, vt[68]). Otherwise r3 = o.
DECOMP(0024D8A0, Obj_SetVelocity_0024D8A0) {
    const u32 o = arg<u32>(ctx, 0), v = arg<u32>(ctx, 1);
    Copy3F53(ctx, o + 1168, v);
    Copy3F53(ctx, o + 1184, o + 1168);
    const u32 rb = load<u32>(o + 416);
    if (rb == 0 || load<u32>(rb + 8) == 0) return;
    HkSetVec53(ctx, rb, o + 1184, 0x0024D870u, 68);
}

// u32 f(T* o, const f32 v[3]; f32 t): +592..+600 and +688..+696 = v (raw
// words); Hk_GetBody9D19C8(o->+32) (f1 = v.z); when its +8 is set and t >=
// the f32 0x002696A0: 0x009D2460(o->+32, &row) with the {x, y, z, 0} row
// of v (RowSelW, masks 0x002696B0) at sp+112 and f1 = t. Otherwise r3 =
// the body's +8 (0) or 0x002696A0 (f1 = that f32).
DECOMP(002696E0, Obj_SetVelocityT_002696E0) {
    const u32 o = arg<u32>(ctx, 0), v = arg<u32>(ctx, 1);
    const f64 t = ctx.fpr[1];
    constexpr u32 K = 0x002696A0u;
    Copy3F53(ctx, o + 592, v);
    Copy3F53(ctx, o + 688, v);
    const u32 b = static_cast<u32>(call(ctx, 0x009D19C8u, {load<u32>(o + 32)}));
    if (load<u32>(b + 8) == 0) {
        ret(ctx, 0u);
        return;
    }
    ctx.fpr[1] = static_cast<f64>(load<f32>(K));
    if (!(t >= static_cast<f64>(load<f32>(K)))) {
        ret(ctx, K);
        return;
    }
    Stack fr(ctx, 16);
    Stvx(fr.addr(), RowSelW(v, v + 4, v + 8, LoadMasks3(K + 16)));
    ctx.fpr[1] = t;
    ctx.gpr[3] = call(ctx, 0x009D2460u, {load<u32>(o + 32), fr.addr()});
}

// u32 f(V4* out, T* b): b->vt[160](b, &u (sp+176), &w (sp+184), &m
// (sp+112)) gives a 4x4 matrix and two 2-vectors; out->+8 = 0, f32 +0 = s
// = (u.x + w.x) * k and +4 = t = (u.y + w.y) * k (k the f32 0x004E03EC,
// single steps); then out (stvx) = m3 + (m2 * 0 + (m1 * t + (m0 * s +
// 0))). r3 = 144, f1 = s.
DECOMP(004E03F0, Obj_GetMidPoint_004E03F0) {
    const u32 out = arg<u32>(ctx, 0), b = arg<u32>(ctx, 1);
    Stack fr(ctx, 96);
    const u32 m = fr.addr(), u = m + 64, w = m + 72;
    VCall(ctx, b, 160, {b, u, w, m});
    const f64 k = static_cast<f64>(load<f32>(0x004E03ECu));
    const f32 s = static_cast<f32>(static_cast<f64>(static_cast<f32>(static_cast<f64>(load<f32>(u)) + static_cast<f64>(load<f32>(w)))) * k);
    const f32 t = static_cast<f32>(static_cast<f64>(static_cast<f32>(static_cast<f64>(load<f32>(u + 4)) + static_cast<f64>(load<f32>(w + 4)))) * k);
    store<u32>(out + 8, 0);
    store<f32>(out, s);
    store<f32>(out + 4, t);
    const V4 m0 = Lvx(m), m1 = Lvx(m + 16), m2 = Lvx(m + 32), m3 = Lvx(m + 48);
    const V4 r = Add4(m3, Madd4(m2, Zero4(), Madd4(m1, Fill4(t), Madd4(m0, Fill4(s), Zero4()))));
    Stvx(out, r);
    ctx.fpr[1] = static_cast<f64>(s);
    ret(ctx, 144u);
}

// Two {x, y, z, 0} rows (RowSelW, masks at k) of a and b on the stack
// (sp+112 / sp+128), Hk_Activate91A4C8(rb), (rb + 224)->vt[88](rb + 224,
// &ra, &rb); r3 = its result.
static void HkSetVec2_53(PPUContext& ctx, u32 rb, u32 a, u32 b, u32 k) {
    Stack fr(ctx, 32);
    const Masks3 mk = LoadMasks3(k);
    Stvx(fr.addr(), RowSelW(a, a + 4, a + 8, mk));
    Stvx(fr.addr() + 16, RowSelW(b, b + 4, b + 8, mk));
    call(ctx, 0x0091A4C8u, {rb});
    const u32 h = rb + 224;
    ctx.gpr[3] = VCall(ctx, h, 88, {h, fr.addr(), fr.addr() + 16});
}

// u32 f(T* o, const f32 a[3], const f32 b[3]): with the rigid body
// o->+416 and its u8 +232 not 4 / 5: HkSetVec2_53 (masks 0x001FB610).
// Otherwise r3 = b.
DECOMP(001FB640, Body_SetVecPair_001FB640) {
    const u32 rb = load<u32>(arg<u32>(ctx, 0) + 416), a = arg<u32>(ctx, 1), b = arg<u32>(ctx, 2);
    if (rb == 0 || load<u8>(rb + 232) == 5 || load<u8>(rb + 232) == 4) {
        ret(ctx, b);
        return;
    }
    HkSetVec2_53(ctx, rb, a, b, 0x001FB610u);
}

// u32 f(T* o, const f32 a[3], const f32 b[3]): with the rigid body
// o->+396, its u8 +232 not 4 / 5 and its +8 set: HkSetVec2_53 (masks
// 0x002442B0). Otherwise r3 = b.
DECOMP(002442E0, Body_SetVecPair_002442E0) {
    const u32 rb = load<u32>(arg<u32>(ctx, 0) + 396), a = arg<u32>(ctx, 1), b = arg<u32>(ctx, 2);
    if (rb == 0 || load<u8>(rb + 232) == 5 || load<u8>(rb + 232) == 4 || load<u32>(rb + 8) == 0) {
        ret(ctx, b);
        return;
    }
    HkSetVec2_53(ctx, rb, a, b, 0x002442B0u);
}

// u32 f(const Def* a, Desc* d): Obj_CopyQuery_000CDB28(a, d); d->+144 =
// a->+180; bytes d->+148..+153 = bits 0x80 / 0x40 / 0x20 / 0x10 / 0x08 /
// 0x02 of u8 a->+184 (as 0 / 1); +156 = +188, u16 +160 / +162 = +192 /
// +194, +164 = +196, f32 +168 / +172 = +200 / +204; the three quadwords at
// a + 208 copied to d + 176 (lvx / stvx). r3 = a + 208, f1 = a->+204.
DECOMP(00215ACC, Def_CopyDesc_00215ACC) {
    const u32 a = arg<u32>(ctx, 0), d = arg<u32>(ctx, 1);
    call(ctx, 0x000CDB28u, {a, d});
    auto bit = [&](u32 m) { return static_cast<u8>((load<u8>(a + 184) & m) != 0 ? 1 : 0); };
    store<u32>(d + 144, load<u32>(a + 180));
    store<u8>(d + 148, bit(0x80));
    store<u8>(d + 149, bit(0x40));
    store<u8>(d + 150, bit(0x20));
    store<u8>(d + 151, bit(0x10));
    store<u8>(d + 152, bit(0x08));
    store<u32>(d + 156, load<u32>(a + 188));
    store<u16>(d + 160, load<u16>(a + 192));
    store<u32>(d + 168, load<u32>(a + 200));
    store<u16>(d + 162, load<u16>(a + 194));
    store<u8>(d + 153, bit(0x02));
    store<u32>(d + 164, load<u32>(a + 196));
    store<u32>(d + 172, load<u32>(a + 204));
    for (u32 k = 0; k < 3; ++k) CopyQ53(d + 176 + 16 * k, a + 208 + 16 * k);
    ctx.fpr[1] = static_cast<f64>(load<f32>(a + 204));
    ret(ctx, a + 208);
}
