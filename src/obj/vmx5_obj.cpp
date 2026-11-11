// Game code, VMX sweep part 5 (see vmx5.h): game-object functions.
#include "../sly4.h"
#include "vmx5.h"

using namespace sly4;
using namespace sly4::vmx5;

// u32 f(List* l, Obj* o, u32 a, -, -, -, u32 hash, const f32 pos[3], u32
// a9, u32 a10; f32 f1, f32 f2, f32 f3) (called by 0039724C): adds an
// attachment record. With room (l->+24 + 1 < 16) the record e = l->+3732
// [l->+24] gets {+0 o, +4 a, +8 hash, +60 f1, +64 f2, +48 f3, +16..+24 pos};
// when o->+160 ->+208 ->+108 has an 80-byte record for hash (Rec80_Find),
// pos goes through that record's rows (e+16, four lanes) and its s16 +68 is
// the index, else Obj_IndexByHash0D14DC(o, hash); e->+12 = index, +52 a9,
// +56 a10; e+32 = pos through o's world rows (+96; or, for an index >= 0,
// Obj_BoneMatrix(&m, o, index, 1, 1)), the w lanes from the stack; the count
// incremented. Returns l->+24 + 1 when full; else r3 / f1 as the original
// leaves them.
DECOMP(0001B8C0, AttachList_Add_0001B8C0) {
    const u32 l = arg<u32>(ctx, 0), o = arg<u32>(ctx, 1), a = arg<u32>(ctx, 2);
    const u32 hash = arg<u32>(ctx, 6), pos = arg<u32>(ctx, 7);
    const f64 g1 = ctx.fpr[1], g2 = ctx.fpr[2], g3 = ctx.fpr[3];
    const u32 a9 = StkW(ctx, 116), a10 = StkW(ctx, 124);
    const u32 n = load<u32>(l + 24) + 1;
    if (!(static_cast<s32>(n) < 16)) {
        ctx.gpr[3] = n;
        return;
    }
    Stack frame(ctx, 304);
    const u32 sp = static_cast<u32>(ctx.gpr[1]);
    const u32 e = load<u32>(l + load<u32>(l + 24) * 4 + 3732);
    store<u32>(e, o);
    store<u32>(e + 4, a);
    store<f32>(e + 60, static_cast<f32>(g1));
    store<f32>(e + 64, static_cast<f32>(g2));
    store<f32>(e + 48, static_cast<f32>(g3));
    const u32 t = load<u32>(load<u32>(o + 160) + 208);
    store<u32>(e + 16, load<u32>(pos));
    store<u32>(e + 20, load<u32>(pos + 4));
    store<u32>(e + 8, hash);
    store<u32>(e + 24, load<u32>(pos + 8));
    const u32 set = load<u32>(t + 108);
    u32 rec = 0;
    if (set != 0) rec = static_cast<u32>(call(ctx, v5fn::Rec80_Find, {set, hash}));
    s32 idx;
    if (rec != 0) {
        const V4 v = Add4(Lvx(rec + 48), Row3(Lvx(rec), Lvx(rec + 16), Lvx(rec + 32), Splat(Lvlx(e + 16), 0).e[0], Splat(Lvlx(e + 20), 0).e[0],
                                              Splat(Lvlx(e + 24), 0).e[0]));
        store<u32>(e + 12, static_cast<u32>(static_cast<s32>(static_cast<s16>(load<u16>(rec + 68)))));
        Stvx(e + 16, v);
        idx = static_cast<s32>(load<u32>(e + 12));
    } else {
        idx = static_cast<s32>(call(ctx, v1fn::Obj_IndexByHash, {o, hash}));
        store<u32>(e + 12, static_cast<u32>(idx));
    }
    store<u32>(e + 52, a9);
    store<u32>(e + 56, a10);
    // The rows as words at sp+112.. (the w lanes are the stack's).
    const u32 R = sp + 112;
    if (idx < 0) {
        CopyMat43W(R, load<u32>(e) + 96);
    } else {
        call(ctx, v4fn::Obj_BoneMatrix, {sp + 176, load<u32>(e), static_cast<u64>(static_cast<s64>(idx)), 1, 1});
        CopyMat43W(R, sp + 176);
    }
    const f32 px = Splat(Lvlx(e + 16), 0).e[0], py = Splat(Lvlx(e + 20), 0).e[0], pz = Splat(Lvlx(e + 24), 0).e[0];
    const V4 v = Add4(Lvx(R + 48), Row3(Lvx(R), Lvx(R + 16), Lvx(R + 32), px, py, pz));
    store<u32>(l + 24, load<u32>(l + 24) + 1);
    Stvx(e + 32, v);
    SetF1Bits(ctx, load<u32>(R));
    ctx.gpr[3] = l + 24;
}

namespace {
constexpr u32 kZero33E2C0 = 0x0033E290;   // f32 0.0
constexpr u32 kPlus33E2C0 = 0x00ADBAE4;   // "+"
constexpr u32 kMinus33E2C0 = 0x00ADBAEC;  // "-"
constexpr u32 kFmt33E2C0 = 0x00ADBAF4;    // "%s%i"
}  // namespace

// u32 f(Popups* t, Obj* o, s32 n, const f32 pos[3]) (called by
// Obj_OnMsgRecs_0033E6EC): spawns a "+n" / "-n" score popup. Only when
// t->+2812 is set, the count t->+2780 (u16) is below the capacity +2776 and
// o is set: the widget Ui_Find(n > 0 ? t+2636 : t+2700, 0), the colour bytes
// t->+2764 / +2768 and a 144-byte record p = SmallAlloc(144) (when it
// succeeds) {+0 widget, +16..+56 o's world rows (+96, 3x3 words), +64..+72
// pos through those rows (+ row 3), +80 / +84 t->+2624, +88 +2628, +92
// +2632, +96 +2620, +100 colour, +104 String_Assign(String_Format("%s%i",
// sign, |n|))}; p (or 0) appended to the PtrVector t+2772 (grown to twice
// the capacity when full) and the high-water mark +2782 raised. Returns the
// gate value that failed (byte / count), else the new high-water mark; f1
// as the original leaves it.
DECOMP(0033E2C0, Popup_SpawnScore_0033E2C0) {
    const u32 t = arg<u32>(ctx, 0), o = arg<u32>(ctx, 1), pos = arg<u32>(ctx, 3);
    const s32 n = arg<s32>(ctx, 2);
    const u8 on = load<u8>(t + 2812);
    if (on == 0) {
        ctx.gpr[3] = 0;
        return;
    }
    const u32 c0 = load<u16>(t + 2780);
    if (!(c0 < load<u16>(t + 2776)) || o == 0) {
        ctx.gpr[3] = c0;
        return;
    }
    Stack frame(ctx, 416);
    const u32 sp = static_cast<u32>(ctx.gpr[1]);
    const f64 zero = Lfs4(kZero33E2C0);
    const f64 fn = Frsp(static_cast<f64>(static_cast<s64>(n)));
    const bool pos_n = fn > zero;
    const u32 ui = static_cast<u32>(call(ctx, v5fn::Ui_Find, {pos_n ? t + 2636 : t + 2700, 0}));
    const f32 px = Splat(Lvlx(pos), 0).e[0], py = Splat(Lvlx(pos + 4), 0).e[0], pz = Splat(Lvlx(pos + 8), 0).e[0];
    SetF1Bits(ctx, load<u32>(pos + 4));
    const u32 sign = pos_n ? kPlus33E2C0 : kMinus33E2C0;
    const u32 col = pos_n ? t + 2764 : t + 2768;
    const u8 cb0 = load<u8>(col), cb1 = load<u8>(col + 1), cb2 = load<u8>(col + 2), cb3 = load<u8>(col + 3);
    const u32 mag = n < 0 ? static_cast<u32>(-static_cast<s64>(n)) : static_cast<u32>(n);
    const u32 m = o + 96;
    u32 w[12];
    for (u32 r = 0; r < 3; ++r)
        for (u32 k = 0; k < 3; ++k) w[3 * r + k] = load<u32>(m + 16 * r + 4 * k);
    // The rows' x/y/z words at sp+128.. (the w lanes are the stack's).
    CopyMat43W(sp + 128, m);
    const V4 v = Add4(Lvx(sp + 176), Row3(Lvx(sp + 128), Lvx(sp + 144), Lvx(sp + 160), px, py, pz));
    Stvx(sp + 112, v);
    const u32 p = static_cast<u32>(call(ctx, v5fn::SmallAlloc, {144}));
    u32 rec = 0;
    if (p != 0) {
        const u32 g80 = load<u32>(t + 2624), g88 = load<u32>(t + 2628), g92 = load<u32>(t + 2632), g96 = load<u32>(t + 2620);
        const u32 s = static_cast<u32>(call(ctx, v1fn::String_Format, {kFmt33E2C0, sign, mag}));
        store<u32>(p, ui);
        for (u32 r = 0; r < 3; ++r)
            for (u32 k = 0; k < 3; ++k) store<u32>(p + 16 + 16 * r + 4 * k, w[3 * r + k]);
        for (u32 k = 0; k < 3; ++k) store<u32>(p + 64 + 4 * k, LaneW(v, k));
        store<u32>(p + 80, g80);
        store<u32>(p + 84, g80);
        store<u32>(p + 88, g88);
        store<u32>(p + 92, g92);
        store<u32>(p + 96, g96);
        store<u8>(p + 100, cb0);
        store<u8>(p + 101, cb1);
        store<u8>(p + 102, cb2);
        store<u8>(p + 103, cb3);
        call(ctx, v5fn::String_Assign, {p + 104, s});
        rec = p;
    }
    u32 c = load<u16>(t + 2780);
    const u32 cap = load<u16>(t + 2776);
    if (c == cap) {
        call(ctx, v5fn::PtrVector_Reserve, {t + 2772, (cap << 1) & 0x1FFFEu});
        c = load<u16>(t + 2780);
    }
    store<u32>(load<u32>(t + 2772) + ((c << 2) & 0x3FFFCu), rec);
    u32 r3 = load<u16>(t + 2780) + 1;
    const u32 hi = load<u16>(t + 2782);
    store<u16>(t + 2780, static_cast<u16>(r3));
    if (static_cast<s32>(hi) > static_cast<s32>(r3 & 0xFFFFu)) r3 = hi;
    store<u16>(t + 2782, static_cast<u16>(r3));
    ctx.gpr[3] = r3;
}

namespace {
constexpr u32 kK1E420 = 0x0001E414;  // f32 0.0, 1.0, 0.4
// Property descriptors: the index of a per-type value is desc->+16 ->+64.
constexpr u32 kPropDist1E420 = 0x00F949AC;    // f32 flee distance
constexpr u32 kPropUseNav1E420 = 0x00F94844;  // u8 use the nav graph
constexpr u32 kPropNavTol1E420 = 0x00F94994;  // f32 nav tolerance
constexpr u32 kPropRange1E420 = 0x00F94784;   // f32 leash range
constexpr u32 kPropNoBox1E420 = 0x00F947CC;   // u8 skip the box test
inline u32 PropOfs(u32 desc) { return (load<u32>(load<u32>(desc + 16) + 64) << 2) & 0xFFFFFFFCu; }
}  // namespace

// void f(f32 out[3], Obj* o, const f32 dir[3]) (called by 0001E7F0,
// 0001F408, 00020310, 00023420): a point away from dir. With the type
// values v = (o->+308)->+12 (indexed through property descriptors): out =
// o's position (+176) + normalize(-dir) * v.dist (y zeroed unless bit 0x20
// of o->+305). With v.use_nav: n = Nav_ClosestPoint147B8(&n, o, out); out =
// a random spawn point when n is within (v.nav_tol + 0.4) of position + dir,
// else n. Otherwise, with a home object h = o->+48 ->+220: when
// Obj_Pick(h) and !v.no_box, out is kept if Box_ContainsPoint(pick->+208
// +16, out through the pick's rows +80) and replaced by a random spawn point
// otherwise; else out is replaced by a random spawn point when farther than
// v.range from Obj_SubAddr_00452024(h)+48 (y ignored unless the 0x20 bit).
// r3 / f1 are left as the original leaves them.
DECOMP(0001E420, Obj_AwayPoint_0001E420) {
    const u32 out = arg<u32>(ctx, 0), o = arg<u32>(ctx, 1), dir = arg<u32>(ctx, 2);
    Stack frame(ctx, 224);
    const u32 sp = static_cast<u32>(ctx.gpr[1]);
    const bool fly = ((load<u8>(o + 305) >> 5) & 1u) != 0;
    f32 dx = -F(dir), dy = -F(dir + 4), dz = -F(dir + 8);
    const u32 vals = load<u32>(load<u32>(o + 308) + 12);
    if (!fly) dy = F(kK1E420);
    const f32 inv = Divs(F(kK1E420 + 4), Sqrts(Fmadds(dz, dz, Fmadds(dx, dx, Muls(dy, dy)))));
    const u32 P = o + 176;
    const f32 r = F(vals + PropOfs(kPropDist1E420));
    dx = Muls(dx, inv);
    dy = Muls(dy, inv);
    dz = Muls(dz, inv);
    const f32 ox = Fmadds(dx, r, F(P)), oy = Fmadds(dy, r, F(P + 4)), oz = Fmadds(dz, r, F(P + 8));
    store<f32>(out, ox);
    store<f32>(out + 4, oy);
    store<f32>(out + 8, oz);
    auto spawn = [&](u32 tmp) {
        call(ctx, v5fn::Obj_RandomSpawnPoint, {tmp, o});
        for (u32 k = 0; k < 3; ++k) store<u32>(out + 4 * k, load<u32>(tmp + 4 * k));
        SetF1Bits(ctx, load<u32>(tmp + 8));
    };
    const u32 ou = PropOfs(kPropUseNav1E420);
    const u8 use = load<u8>(load<u32>(load<u32>(o + 308) + 12) + ou);
    if (use != 0) {
        const u32 n = sp + 112;
        call(ctx, v5fn::Nav_ClosestPoint147B8, {n, o, out});
        const f32 ey = Subs(F(n + 4), Adds(F(P + 4), F(dir + 4)));
        const f32 ex = Subs(F(n), Adds(F(P), F(dir)));
        const f32 ez = Subs(F(n + 8), Adds(F(P + 8), F(dir + 8)));
        const u32 ot = PropOfs(kPropNavTol1E420);
        const f32 tol = Adds(F(load<u32>(load<u32>(o + 308) + 12) + ot), F(kK1E420 + 8));
        const f32 d2 = Fmadds(ez, ez, Fmadds(ex, ex, Muls(ey, ey)));
        if (d2 < Muls(tol, tol)) {
            spawn(sp + 128);
            return;
        }
        for (u32 k = 0; k < 3; ++k) store<u32>(out + 4 * k, load<u32>(n + 4 * k));
        SetF1Bits(ctx, load<u32>(n + 8));
        ctx.gpr[3] = ot;
        return;
    }
    ctx.fpr[1] = oz;
    ctx.gpr[3] = use;
    const u32 h = load<u32>(load<u32>(o + 48) + 220);
    if (h == 0) return;
    const u32 pick = static_cast<u32>(call(ctx, v4fn::Obj_Pick, {h}));
    if (pick != 0) {
        const u32 ob = PropOfs(kPropNoBox1E420);
        if (load<u8>(load<u32>(load<u32>(o + 308) + 12) + ob) == 0) {
            const u32 m = pick + 80, q = sp + 144;
            for (u32 k = 0; k < 3; ++k) store<u32>(q + 4 * k, load<u32>(out + 4 * k));
            SetF1Bits(ctx, load<u32>(out + 8));
            const f32 px = Splat(Lvlx(out), 0).e[0], py = Splat(Lvlx(out + 4), 0).e[0], pz = Splat(Lvlx(out + 8), 0).e[0];
            Stvx(q, Add4(Lvx(m + 48), Row3(Lvx(m), Lvx(m + 16), Lvx(m + 32), px, py, pz)));
            if (call(ctx, v4fn::Box_ContainsPoint, {load<u32>(pick + 208) + 16, q}) != 0) return;
            spawn(sp + 160);
            return;
        }
    }
    const u32 q = static_cast<u32>(call(ctx, v5fn::Obj_SubAddr_00452024, {h})) + 48;
    const bool fly2 = ((load<u8>(o + 305) >> 5) & 1u) != 0;
    f32 ey = Subs(F(q + 4), F(out + 4));
    const f32 ex = Subs(F(q), F(out)), ez = Subs(F(q + 8), F(out + 8));
    const u32 vals2 = load<u32>(load<u32>(o + 308) + 12);
    if (!fly2) ey = F(kK1E420);
    const u32 orr = PropOfs(kPropRange1E420);
    const f32 rr = F(vals2 + orr);
    const f32 d2 = Fmadds(ez, ez, Fmadds(ex, ex, Muls(ey, ey)));
    if (d2 > Muls(rr, rr)) {
        spawn(sp + 176);
        return;
    }
    ctx.fpr[1] = d2;
    ctx.gpr[3] = orr;
}

namespace {
constexpr u32 kK23420 = 0x00023400;  // f32 2.0, 0.0, 1.0, pi/4; vperm mask {a0 a1 b0 a3} (+16)
constexpr u32 kPropReach23420 = 0x00F947E4;  // f32 reach distance (property descriptor)
constexpr u32 kPropNavTol23420 = 0x00F94994;  // f32 nav tolerance
constexpr u32 kPropFlee23420 = 0x00F94A6C;  // u32 flee instead of chasing
constexpr u32 kPathCtx23420 = 0x011AD1D4;  // the path query context
}  // namespace

// void f(Obj* o) (called through a pointer): one chase step toward the
// target t = o->+52, with r = 2 * the type's reach value and d2 = o->+56
// (squared distance). Without t nothing changes. When t's vt[128](t) is 0
// and d2 < nav_tol^2: with the flee value o->vt[220](o, 14); else, unless
// o->+4 is 8 or 9, o->vt[220](o, 8) and the point away from (t.pos -
// o.pos) (Obj_AwayPoint_0001E420) stored at o+96.., replaced by the path's
// point (Path_Query6C0AA4(o->+212, &p, ctx), Path_Point6C1B94) with bit
// 0x10 of o->+304 and a path. Otherwise, when d2 < r^2: o's position (+176)
// is set to t's minus the direction to t times r and, when the flat
// direction (x, 0, z; z = 1 when d2 is 0) is within pi/4 of o's facing
// (+192) by Vec_SignedAngle16680, the facing is turned by (pi/4 - angle)
// about the up axis (Matrix_FromAxisAngle). The position words are always
// stored back. r3 / f1 are left as the original leaves them.
DECOMP(00023420, Ai_ChaseStep_00023420) {
    const u32 o = arg<u32>(ctx, 0);
    Stack frame(ctx, 320);
    const u32 sp = static_cast<u32>(ctx.gpr[1]);
    const u32 P = o + 176;
    const u32 vals = load<u32>(load<u32>(o + 308) + 12);
    u32 px = load<u32>(P), py = load<u32>(P + 4), pz = load<u32>(P + 8);
    const u32 ofs = (load<u32>(load<u32>(kPropReach23420 + 16) + 64) << 2) & 0xFFFFFFFCu;
    const u32 t = load<u32>(o + 52);
    const f32 reach = F(vals + ofs);
    const f32 r = Muls(reach, F(kK23420));
    auto finish = [&]() {
        store<u32>(P, px);
        store<u32>(P + 4, py);
        store<u32>(P + 8, pz);
    };
    if (t == 0) {
        ctx.fpr[1] = reach;
        ctx.gpr[3] = 0;
        finish();
        return;
    }
    const u32 ok = static_cast<u32>(VCall(ctx, t, 128, {t}));
    const f32 d2 = F(o + 56);
    ctx.fpr[1] = d2;
    ctx.gpr[3] = load<u32>(o + 52);
    if (ok == 0) {
        const u32 v2 = load<u32>(load<u32>(o + 308) + 12);
        const f32 tol = F(v2 + ((load<u32>(load<u32>(kPropNavTol23420 + 16) + 64) << 2) & 0xFFFFFFFCu));
        if (d2 < Muls(tol, tol)) {
            if (load<u32>(v2 + ((load<u32>(load<u32>(kPropFlee23420 + 16) + 64) << 2) & 0xFFFFFFFCu)) != 0) {
                VCall(ctx, o, 220, {o, 14});
                finish();
                return;
            }
            const s32 st = static_cast<s32>(load<u32>(o + 4));
            if (st == 9 || st == 8) {
                finish();
                return;
            }
            const u32 q = ctx.gpr[3] + 176;
            const f32 ex = Subs(F(q), F(P)), ey = Subs(F(q + 4), F(P + 4));
            const f32 ez = Subs(F(q + 8), F(P + 8));  // (re-read after the vcall)
            store<f32>(sp + 128, ex);
            store<f32>(sp + 132, ey);
            store<f32>(sp + 136, ez);
            VCall(ctx, o, 220, {o, 8});
            call(ctx, v5fn::AwayPoint1E420, {sp + 112, o, sp + 128});
            const u32 a0 = load<u32>(sp + 112), a1 = load<u32>(sp + 116), a2 = load<u32>(sp + 120);
            store<u32>(o + 96, a0);
            store<u32>(o + 100, a1);
            store<u32>(o + 104, a2);
            SetF1Bits(ctx, a2);
            const u32 fl = (load<u8>(o + 304) >> 4) & 1u;
            ctx.gpr[3] = fl;
            if (fl != 0) {
                const u32 path = load<u32>(o + 212);
                ctx.gpr[3] = path;
                if (path != 0) {
                    store<u32>(sp + 224, a0);
                    store<u32>(sp + 228, a1);
                    store<u32>(sp + 232, a2);
                    call(ctx, v5fn::Path_Query6C0AA4, {path, sp + 224, load<u32>(kPathCtx23420)});
                    call(ctx, v5fn::Path_Point6C1B94, {sp + 236, load<u32>(o + 212)});
                    const u32 b0 = load<u32>(sp + 236), b1 = load<u32>(sp + 240), b2 = load<u32>(sp + 244);
                    SetF1Bits(ctx, b0);
                    store<u32>(o + 96, b0);
                    store<u32>(o + 100, b1);
                    store<u32>(o + 104, b2);
                }
            }
            finish();
            return;
        }
    }
    if (!(d2 < Muls(r, r))) {
        finish();
        return;
    }
    const u32 q = ctx.gpr[3] + 176;
    const f32 z0 = F(kK23420 + 4), one = F(kK23420 + 8);
    const f32 qx = F(q), qz = F(q + 8), qy = F(q + 4);
    const f32 dx = Subs(qx, std::bit_cast<f32>(px));
    f32 dz = Subs(qz, std::bit_cast<f32>(pz));
    if (d2 == z0) dz = one;
    const f32 dy = Subs(qy, std::bit_cast<f32>(py));
    const f32 i1 = Divs(one, Sqrts(Fmadds(dz, dz, Fmadds(dx, dx, Muls(z0, z0)))));
    const f32 i2 = Divs(one, Sqrts(Fmadds(dz, dz, Fmadds(dx, dx, Muls(dy, dy)))));
    store<f32>(sp + 144, Muls(dx, i1));
    store<f32>(sp + 148, Muls(z0, i1));
    store<f32>(sp + 152, Muls(dz, i1));
    px = std::bit_cast<u32>(Fnmsubs(Muls(dx, i2), r, qx));
    pz = std::bit_cast<u32>(Fnmsubs(Muls(dz, i2), r, qz));
    py = std::bit_cast<u32>(Fnmsubs(Muls(dy, i2), r, qy));
    const f32 ang = static_cast<f32>(callf(ctx, v5fn::Vec_SignedAngle16680, {sp + 144, o + 192}, {}));
    const f32 lim = F(kK23420 + 12);
    if (std::fabs(ang) < lim) {
        callf(ctx, v4fn::Matrix_FromAxisAngle, {sp + 160, kUpAxis5}, {static_cast<f64>(Subs(lim, ang))});
        store<u32>(sp + 208, load<u32>(kV3Zero3));
        store<u32>(sp + 212, load<u32>(kV3Zero3 + 4));
        const V4 tz = Perm(Lvx(sp + 208), Lvlx(kV3Zero3 + 8), Lvx(kK23420 + 16));
        const V4 v = Add4(tz, Row3(Lvx(sp + 160), Lvx(sp + 176), Lvx(sp + 192), Splat(Lvlx(o + 192), 0).e[0], Splat(Lvlx(o + 196), 0).e[0],
                                    Splat(Lvlx(o + 200), 0).e[0]));
        Stvx(o + 192, v);
        SetF1Bits(ctx, load<u32>(kV3Zero3));
        ctx.gpr[3] = kK23420;
    }
    finish();
}

namespace {
constexpr u32 kK3CC068 = 0x003CC054;  // f32 0.3, 4.0, 0.1, 0.2, 0.7
constexpr u32 kBone3CC068 = 0x7E95647Cu;  // the carry bone's name hash
}  // namespace

// void f(Carry* c) (called by 003CF648): keeps a carried item t = c->+112
// on its carrier x = c->+4. While c->+96 < 0.3: the item's frame = the
// carrier's bone 0x7E95647C (Obj_FindBone_000DA9D0, its +8 rows) through
// x's world rows (+96): t's +32..+40 = the translation, then t->+108's
// vt[84](body, Mat34_FromRotPos553B4C(&m, the body's rotation +96.., &p))
// and vt[88](body, 1, 1, 1, 1). Else the item is put down: t's +48..+56 =
// x's position (+128) * 0.3 + up * 4, adjusted along x's forward (+96) by
// mode c->+88 (1: back 0.2 when c->+104 > 0.7; 0: forward 0.2 and toward
// the position by 0.1 when c->+104 < 0.1) (vt[88](body, 1, 1, 1, 1)), then
// t->+0 = 3, t->+104 = 1.0, c->+76 = 3, c->+96 = 0. r3 / f1 are left as
// the original leaves them.
DECOMP(003CC068, Carry_Update_003CC068) {
    const u32 c = arg<u32>(ctx, 0);
    const u32 K = kK3CC068;
    Stack frame(ctx, 320);
    const u32 sp = static_cast<u32>(ctx.gpr[1]);
    const f32 k0 = F(K);
    if (F(c + 96) < k0) {
        const u32 bone = static_cast<u32>(call(ctx, v5fn::Obj_FindBone_000DA9D0, {load<u32>(c + 4), kBone3CC068}));
        const u32 b = load<u32>(bone + 8);
        const u32 m = load<u32>(c + 4) + 96;
        const V4 m0 = Lvx(m), m1 = Lvx(m + 16), m2 = Lvx(m + 32), m3 = Lvx(m + 48);
        const V4 b0 = Lvx(b), b1 = Lvx(b + 16), b2 = Lvx(b + 32), b3 = Lvx(b + 48);
        Stvx(sp + 160, XformDir(m0, m1, m2, b0));
        Stvx(sp + 176, XformDir(m0, m1, m2, b1));
        Stvx(sp + 192, XformDir(m0, m1, m2, b2));
        Stvx(sp + 208, Add4(m3, XformDir(m0, m1, m2, b3)));
        const u32 t = load<u32>(c + 112);
        store<u32>(t + 32, load<u32>(sp + 208));
        const u32 body = load<u32>(t + 108);
        store<u32>(t + 36, load<u32>(sp + 212));
        store<u32>(t + 40, load<u32>(sp + 216));
        for (u32 r = 0; r < 3; ++r)
            for (u32 k = 0; k < 3; ++k) store<u32>(sp + 112 + 16 * r + 4 * k, load<u32>(body + 96 + 16 * r + 4 * k));
        SetF1Bits(ctx, load<u32>(body + 136));
        call(ctx, v5fn::Mat34_FromRotPos553B4C, {sp + 224, sp + 112, sp + 208});
        VCall(ctx, body, 84, {body, sp + 224});
        const u32 b2b = load<u32>(load<u32>(c + 112) + 108);
        VCall(ctx, b2b, 88, {b2b, 1, 1, 1, 1});
        return;
    }
    const u32 A = kUpAxis5;
    const f32 up = F(K + 4);
    f32 x = Muls(F(A), up), y = Muls(F(A + 4), up), z = Muls(F(A + 8), up);
    const s32 mode = static_cast<s32>(load<u32>(c + 88));
    const u32 t = load<u32>(c + 112);
    const f32 g104 = F(c + 104);
    const u32 X = load<u32>(c + 4);
    if (mode == 1) {
        x = Fmadds(F(X + 128), k0, x);
        y = Fmadds(F(X + 132), k0, y);
        z = Fmadds(F(X + 136), k0, z);
        if (g104 > F(K + 16)) {
            const f32 k3 = F(K + 12);
            x = Fnmsubs(F(X + 96), k3, x);
            y = Fnmsubs(F(X + 100), k3, y);
            z = Fnmsubs(F(X + 104), k3, z);
        }
    } else if (mode == 0) {
        const f32 k2 = F(K + 8);
        if (g104 < k2) {
            const f32 k3 = F(K + 12);
            x = Fmadds(F(X + 96), k3, x);
            y = Fmadds(F(X + 100), k3, y);
            z = Fmadds(F(X + 104), k3, z);
            x = Fnmsubs(F(X + 128), k2, x);
            y = Fnmsubs(F(X + 132), k2, y);
            z = Fnmsubs(F(X + 136), k2, z);
        }
    }
    const f32 nx = Fmadds(F(X + 128), k0, x), ny = Fmadds(F(X + 132), k0, y), nz = Fmadds(F(X + 136), k0, z);
    const u32 body = load<u32>(t + 108);
    store<f32>(t + 48, nx);
    store<f32>(t + 52, ny);
    store<f32>(t + 56, nz);
    ctx.fpr[1] = nx;
    VCall(ctx, body, 88, {body, 1, 1, 1, 1});
    const u32 t2 = load<u32>(c + 112);
    store<u32>(t2, 3);
    store<u32>(t2 + 104, 0x3F800000u);
    store<u32>(c + 76, 3);
    store<u32>(c + 96, 0);
    ctx.gpr[3] = 3;
}

namespace {
constexpr u32 kKEE310 = 0x000EE308;  // f32 0.0, 1.0
constexpr u32 kWorldPhysEE310 = 0x001281D4;  // World: the physics holder (+84 gravity scale)
// The AimQuery block Aim_SolveLaunch reads (on the stack at sp+288): from
// +0, from velocity +16, target +32, target velocity +48, speed +64,
// gravity +68, iterations +72 (7), high arc +76.
}  // namespace

// bool f(f32 dir[3], Launcher* o) (called by 000EE710, 00296910, 00296D48):
// aims o at its target. p = the target point (o->+80's vt[284](&p, ...));
// with an aim offset o+96 other than the zero vector (word compare) it is
// added to p in the frame looking from p to o's origin (o+64; rows: up x d,
// up, d through Mat34_FromRotPos553B4C). Gravity g = shooter (o->+0)'s
// +192 * the physics holder's +84 when the shooter's +400 is set (else no
// aim: 0). With |g| > 0: the velocities of o->+4 / o->+80 (their vt[140],
// when o's byte +116 is set; else zero) and Aim_SolveLaunch(&query, dir,
// &t) with speed o->+120, 7 iterations and the high-arc byte +124. Returns
// whether a launch direction was found; f1 as the original leaves it.
DECOMP(000EE310, Launcher_Aim_000EE310) {
    const u32 dir = arg<u32>(ctx, 0), o = arg<u32>(ctx, 1);
    Stack frame(ctx, 512);
    const u32 sp = static_cast<u32>(ctx.gpr[1]);
    const u32 p = sp + 144;
    {
        const u32 tg = load<u32>(o + 80);
        VCall(ctx, tg, 284, {p, tg});
    }
    const u32 ofs = o + 96, org = o + 64;
    const u64 o01 = (static_cast<u64>(load<u32>(ofs)) << 32) | load<u32>(ofs + 4);
    const u64 z01 = load<u64>(kV3Zero3);
    if (((o01 - z01) | (load<u32>(ofs + 8) - load<u32>(kV3Zero3 + 8))) != 0) {
        f32 dy = Subs(F(org + 4), F(p + 4)), dz = Subs(F(org + 8), F(p + 8)), dx = Subs(F(org), F(p));
        const f32 inv = Divs(F(kKEE310 + 4), Sqrts(Fmadds(dz, dz, Fmadds(dx, dx, Muls(dy, dy)))));
        dx = Muls(dx, inv);
        dy = Muls(dy, inv);
        dz = Muls(dz, inv);
        store<f32>(sp + 208, dx);
        store<f32>(sp + 212, dy);
        store<f32>(sp + 216, dz);
        const u32 A = kUpAxis5;
        const f32 a0 = F(A), a1 = F(A + 4), a2 = F(A + 8);
        store<u32>(sp + 192, load<u32>(A));
        store<u32>(sp + 196, load<u32>(A + 4));
        store<u32>(sp + 200, load<u32>(A + 8));
        store<f32>(sp + 180, Fmsubs(a2, dx, Muls(a0, dz)));
        store<f32>(sp + 184, Fmsubs(a0, dy, Muls(a1, dx)));
        store<f32>(sp + 176, Fmsubs(a1, dz, Muls(a2, dy)));
        const f32 ox = Splat(Lvlx(ofs), 0).e[0], oy = Splat(Lvlx(ofs + 4), 0).e[0], oz = Splat(Lvlx(ofs + 8), 0).e[0];
        for (u32 k = 0; k < 3; ++k) store<u32>(sp + 160 + 4 * k, load<u32>(ofs + 4 * k));
        call(ctx, v5fn::Mat34_FromRotPos553B4C, {sp + 224, sp + 176, kV3Zero3});
        const V4 v = Row3(Lvx(sp + 224), Lvx(sp + 240), Lvx(sp + 256), ox, oy, oz);
        Stvx(sp + 160, v);
        const f32 nx = Adds(F(p), F(sp + 160)), ny = Adds(F(p + 4), F(sp + 164)), nz = Adds(F(p + 8), F(sp + 168));
        store<f32>(p, nx);
        store<f32>(p + 4, ny);
        store<f32>(p + 8, nz);
    }
    const f32 k0 = F(kKEE310);
    f32 g = k0;
    ctx.fpr[1] = k0;
    const u32 sh = load<u32>(o);
    if (load<u32>(sh + 400) == 0) {
        ctx.gpr[3] = 0;
        return;
    }
    g = Muls(F(load<u32>(load<u32>(kV3WorldPtr) + kWorldPhysEE310) + 84), F(sh + 192));
    if (std::fabs(g) <= k0) {
        ctx.gpr[3] = 0;
        return;
    }
    u32 fv0 = load<u32>(kV3Zero3), fv1 = load<u32>(kV3Zero3 + 4), fv2 = load<u32>(kV3Zero3 + 8);
    if (load<u8>(o + 116) != 0) {
        const u32 a = load<u32>(o + 4);
        VCall(ctx, a, 140, {sp + 112, a});
        fv0 = load<u32>(sp + 112);
        fv1 = load<u32>(sp + 116);
        fv2 = load<u32>(sp + 120);
    }
    u32 tv0 = load<u32>(kV3Zero3), tv1 = load<u32>(kV3Zero3 + 4), tv2 = load<u32>(kV3Zero3 + 8);
    if (load<u8>(o + 116) != 0) {
        const u32 b = load<u32>(o + 80);
        VCall(ctx, b, 140, {sp + 128, b});
        tv0 = load<u32>(sp + 128);
        tv1 = load<u32>(sp + 132);
        tv2 = load<u32>(sp + 136);
    }
    const u32 q = sp + 288;
    for (u32 k = 0; k < 3; ++k) store<u32>(q + 4 * k, load<u32>(org + 4 * k));
    store<u32>(q + 16, fv0);
    store<u32>(q + 20, fv1);
    store<u32>(q + 24, fv2);
    for (u32 k = 0; k < 3; ++k) store<u32>(q + 32 + 4 * k, load<u32>(p + 4 * k));
    store<u32>(q + 48, tv0);
    store<u32>(q + 52, tv1);
    store<u32>(q + 56, tv2);
    store<u32>(q + 64, load<u32>(o + 120));
    store<f32>(q + 68, g);
    store<u32>(q + 72, 7);
    store<u8>(q + 76, load<u8>(o + 124));
    SetF1Bits(ctx, tv2);
    const u32 r = static_cast<u32>(call(ctx, v5fn::Aim_SolveLaunch, {q, dir, sp + 368, 7, load<u8>(o + 124)}));
    ctx.gpr[3] = r != 0 ? 1 : 0;
}

namespace {
constexpr u32 kKEE710 = 0x000EE700;  // f32 0.0, 2.0 (lead factor), 1.0
constexpr u32 kDefDirEE710 = 0x01164B70;  // f32[3] the default launch direction
constexpr u32 kIdentEE710 = 0x01164BD0;   // f32[4][4] the identity
}  // namespace

// Obj* f(Launcher* o) (called by 00295268, 00398040): fires o's projectile.
// Without a shooter o->+4, target o->+80 or def o->+0: Obj_SpawnDef0EDE30(
// def, shooter, o+16, default dir, 1, o->+112, 0, 0, 1; 0.0). Else, when
// Launcher_Aim_000EE310 finds a direction d: p = Obj_SpawnDef0EDE30(def,
// shooter, o+16, &d, 1, o->+112, 0, 0, 1; def->+220) and, when it spawned, its
// aim frame: rotation +448.. the identity rows, aim point +496.. = the
// target point (vt[284], plus the offset o+96 in the look frame as in
// Launcher_Aim) or, with o's byte +116, o's origin + d * 2|target - origin|;
// its body (+440) vt[84](body, p+448); +512.. = o's origin (+64). Returns
// the spawned object (or 0); f1 as the original leaves it.
DECOMP(000EE710, Launcher_Fire_000EE710) {
    const u32 o = arg<u32>(ctx, 0);
    Stack frame(ctx, 368);
    const u32 sp = static_cast<u32>(ctx.gpr[1]);
    const u32 a = load<u32>(o + 4);
    if (a == 0 || load<u32>(o + 80) == 0 || load<u32>(o) == 0) {
        callf(ctx, v5fn::Obj_SpawnDef0EDE30, {load<u32>(o), a, o + 16, kDefDirEE710, 1, load<u32>(o + 112), 0, 0, 1}, {Lfs4(kKEE710)});
        return;
    }
    const u32 d = sp + 128;
    if (call(ctx, v5fn::LauncherAimEE310, {d, o}) == 0) {
        ctx.gpr[3] = 0;
        return;
    }
    const u32 def = load<u32>(o);
    callf(ctx, v5fn::Obj_SpawnDef0EDE30, {def, load<u32>(o + 4), o + 16, d, 1, load<u32>(o + 112), 0, 0, 1}, {Lfs4(def + 220)});
    const u32 p = static_cast<u32>(ctx.gpr[3]);
    if (p == 0) return;
    const u32 tp = sp + 144;
    {
        const u32 tg = load<u32>(o + 80);
        VCall(ctx, tg, 284, {tp, tg});
    }
    const u32 ofs = o + 96, org = o + 64;
    const u64 o01 = (static_cast<u64>(load<u32>(ofs)) << 32) | load<u32>(ofs + 4);
    f32 px = F(tp), py = F(tp + 4), pz = F(tp + 8);
    const f32 ox = Splat(Lvlx(ofs), 0).e[0], oy = Splat(Lvlx(ofs + 4), 0).e[0], oz = Splat(Lvlx(ofs + 8), 0).e[0];
    if (((o01 - load<u64>(kV3Zero3)) | (load<u32>(ofs + 8) - load<u32>(kV3Zero3 + 8))) != 0) {
        f32 dy = Subs(F(org + 4), py), dz = Subs(F(org + 8), pz), dx = Subs(F(org), px);
        for (u32 k = 0; k < 3; ++k) store<u32>(sp + 160 + 4 * k, load<u32>(ofs + 4 * k));
        const f32 inv = Divs(F(kKEE710 + 8), Sqrts(Fmadds(dz, dz, Fmadds(dx, dx, Muls(dy, dy)))));
        dx = Muls(dx, inv);
        dy = Muls(dy, inv);
        dz = Muls(dz, inv);
        store<f32>(sp + 208, dx);
        store<f32>(sp + 212, dy);
        store<f32>(sp + 216, dz);
        const u32 A = kUpAxis5;
        const f32 a0 = F(A), a1 = F(A + 4), a2 = F(A + 8);
        for (u32 k = 0; k < 3; ++k) store<u32>(sp + 192 + 4 * k, load<u32>(A + 4 * k));
        store<f32>(sp + 180, Fmsubs(a2, dx, Muls(a0, dz)));
        store<f32>(sp + 184, Fmsubs(a0, dy, Muls(a1, dx)));
        store<f32>(sp + 176, Fmsubs(a1, dz, Muls(a2, dy)));
        call(ctx, v5fn::Mat34_FromRotPos553B4C, {sp + 224, sp + 176, kV3Zero3});
        const V4 v = Row3(Lvx(sp + 224), Lvx(sp + 240), Lvx(sp + 256), ox, oy, oz);
        Stvx(sp + 160, v);
        px = Adds(F(tp), F(sp + 160));
        py = Adds(F(tp + 4), F(sp + 164));
        pz = Adds(F(tp + 8), F(sp + 168));
        store<f32>(tp, px);
        store<f32>(tp + 4, py);
        store<f32>(tp + 8, pz);
    }
    if (load<u8>(o + 116) != 0) {
        const f32 gx = F(org), gy = F(org + 4), gz = F(org + 8);
        const f32 ey = Subs(py, gy), ex = Subs(px, gx), ez = Subs(pz, gz);
        const f32 l = Muls(Sqrts(Fmadds(ez, ez, Fmadds(ex, ex, Muls(ey, ey)))), F(kKEE710 + 4));
        px = Fmadds(F(d), l, gx);
        py = Fmadds(F(d + 4), l, gy);
        pz = Fmadds(F(d + 8), l, gz);
    }
    for (u32 r = 0; r < 3; ++r)
        for (u32 k = 0; k < 3; ++k) store<u32>(p + 448 + 16 * r + 4 * k, load<u32>(kIdentEE710 + 16 * r + 4 * k));
    store<f32>(p + 496, px);
    store<f32>(p + 500, py);
    store<f32>(p + 504, pz);
    const u32 body = load<u32>(p + 440);
    if (body != 0) VCall(ctx, body, 84, {body, p + 448});
    const u32 w0 = load<u32>(org), w1 = load<u32>(org + 4), w2 = load<u32>(org + 8);
    store<u32>(p + 512, w0);
    store<u32>(p + 516, w1);
    store<u32>(p + 520, w2);
    SetF1Bits(ctx, w2);
    ctx.gpr[3] = p;
}

namespace {
constexpr u32 kKEEB08 = 0x000EEB00;  // f32 0.0, 1.0
// The target point p of a launcher (its +80's vt[284](&p, t)) plus the aim
// offset o+96 (unless it equals the zero vector word for word) in the
// frame looking from p to the origin o+64 (rows at rows: up x d, up, d;
// Mat34_FromRotPos553B4C into m; the offset through it at v), as
// Launcher_Aim_000EE310 inlines it. one: the f32 1.0 used to normalize;
// swap: the vt[284] call has r5 the entry and r6 the OPD (else the reverse).
inline void AimPoint(PPUContext& ctx, u32 o, u32 p, u32 rows, u32 m, u32 v, u32 one, bool swap) {
    {
        const u32 tg = load<u32>(o + 80);
        const u32 opd = load<u32>(load<u32>(tg) + 284);
        if (swap) call_ptr(ctx, opd, {p, tg, load<u32>(opd), opd});
        else call_ptr(ctx, opd, {p, tg, opd, load<u32>(opd)});
    }
    const u32 ofs = o + 96, org = o + 64;
    const u64 o01 = (static_cast<u64>(load<u32>(ofs)) << 32) | load<u32>(ofs + 4);
    if (((o01 - load<u64>(kV3Zero3)) | (load<u32>(ofs + 8) - load<u32>(kV3Zero3 + 8))) == 0) return;
    f32 dy = Subs(F(org + 4), F(p + 4)), dz = Subs(F(org + 8), F(p + 8)), dx = Subs(F(org), F(p));
    const f32 inv = Divs(F(one), Sqrts(Fmadds(dz, dz, Fmadds(dx, dx, Muls(dy, dy)))));
    dx = Muls(dx, inv);
    dy = Muls(dy, inv);
    dz = Muls(dz, inv);
    store<f32>(rows + 32, dx);
    store<f32>(rows + 36, dy);
    store<f32>(rows + 40, dz);
    const u32 A = kUpAxis5;
    const f32 a0 = F(A), a1 = F(A + 4), a2 = F(A + 8);
    for (u32 k = 0; k < 3; ++k) store<u32>(rows + 16 + 4 * k, load<u32>(A + 4 * k));
    store<f32>(rows + 4, Fmsubs(a2, dx, Muls(a0, dz)));
    store<f32>(rows + 8, Fmsubs(a0, dy, Muls(a1, dx)));
    store<f32>(rows, Fmsubs(a1, dz, Muls(a2, dy)));
    const f32 ox = Splat(Lvlx(ofs), 0).e[0], oy = Splat(Lvlx(ofs + 4), 0).e[0], oz = Splat(Lvlx(ofs + 8), 0).e[0];
    for (u32 k = 0; k < 3; ++k) store<u32>(v + 4 * k, load<u32>(ofs + 4 * k));
    call(ctx, v5fn::Mat34_FromRotPos553B4C, {m, rows, kV3Zero3});
    const V4 w = Row3(Lvx(m), Lvx(m + 16), Lvx(m + 32), ox, oy, oz);
    Stvx(v, w);
    const f32 nx = Adds(F(p), F(v)), ny = Adds(F(p + 4), F(v + 4)), nz = Adds(F(p + 8), F(v + 8));
    store<f32>(p, nx);
    store<f32>(p + 4, ny);
    store<f32>(p + 8, nz);
}
}  // namespace

// bool f(f32 vel[4 + speed], Launcher* o) (called by 000EEE80, 00296910,
// 00296D48): as Launcher_Aim_000EE310 for a ballistic shot: with the
// target point p (target vt[284] plus the offset in the look frame), the
// target velocity (its vt[140] with o's byte +116, else zero) and gravity
// g (shooter +192 * physics holder +84; none without the shooter's +400,
// or when |g| <= 0), Ballistic_Solve4A2D78(&shot, vel, vel+16, &p) with shot
// {+0 origin o+64, +16 p, +32 target velocity, +48 o->+120, +52 o->+124,
// +56 g, +60 7}. Returns whether it found a solution; f1 as the original
// leaves it.
DECOMP(000EEB08, Launcher_AimBallistic_000EEB08) {
    const u32 vel = arg<u32>(ctx, 0), o = arg<u32>(ctx, 1);
    Stack frame(ctx, 432);
    const u32 sp = static_cast<u32>(ctx.gpr[1]);
    const u32 p = sp + 128, org = o + 64;
    AimPoint(ctx, o, p, sp + 160, sp + 272, sp + 144, kKEEB08 + 4, false);
    u32 tv0 = load<u32>(kV3Zero3), tv1 = load<u32>(kV3Zero3 + 4), tv2 = load<u32>(kV3Zero3 + 8);
    if (load<u8>(o + 116) != 0) {
        const u32 b = load<u32>(o + 80);
        VCall(ctx, b, 140, {sp + 112, b});
        tv0 = load<u32>(sp + 112);
        tv1 = load<u32>(sp + 116);
        tv2 = load<u32>(sp + 120);
    }
    SetF1Bits(ctx, load<u32>(p + 8));
    const f32 k0 = F(kKEEB08);
    const u32 sh = load<u32>(o);
    if (load<u32>(sh + 400) == 0) {
        ctx.gpr[3] = 0;
        return;
    }
    const f32 g = Muls(F(load<u32>(load<u32>(kV3WorldPtr) + kWorldPhysEE310) + 84), F(sh + 192));
    if (std::fabs(g) <= k0) {
        ctx.gpr[3] = 0;
        return;
    }
    const u32 q = sp + 208;
    for (u32 k = 0; k < 3; ++k) store<u32>(q + 4 * k, load<u32>(org + 4 * k));
    for (u32 k = 0; k < 3; ++k) store<u32>(q + 16 + 4 * k, load<u32>(p + 4 * k));
    store<u32>(q + 32, tv0);
    store<u32>(q + 36, tv1);
    store<u32>(q + 40, tv2);
    store<u32>(q + 48, load<u32>(o + 120));
    store<u32>(q + 52, load<u32>(o + 124));
    store<f32>(q + 56, g);
    store<u32>(q + 60, 7);
    const u32 r = static_cast<u32>(call(ctx, v5fn::Ballistic_Solve4A2D78, {q, vel, vel + 16, p}));
    ctx.gpr[3] = r != 0 ? 1 : 0;
}

namespace {
constexpr u32 kKEEE80 = 0x000EEE78;  // f32 0.0, 1.0
}  // namespace

// Obj* f(Launcher* o) (called by 00295268, 00398040): the ballistic
// launcher, as Launcher_Fire_000EE710 with Launcher_AimBallistic_000EEB08.
// Without a target o->+80 or def o->+0: Obj_SpawnDef0EDE30(def, shooter,
// o+16, default dir, shooter, o->+112, 0, 0, 1; 0.0). Else, with a
// solution v: p = Obj_SpawnDef0EDE30(def, shooter, o+16, &v, 1, o->+112, 0,
// 0, 1; v's speed +16) and, when it spawned: rotation +448.. the identity
// rows (read before the target query), aim point +496.. = the target point
// plus the offset in the look frame, its body (+440) vt[84](body, p+448),
// +512.. = o's origin (+64). Returns the spawned object (or 0); f1 as the
// original leaves it.
DECOMP(000EEE80, Launcher_FireBallistic_000EEE80) {
    const u32 o = arg<u32>(ctx, 0);
    Stack frame(ctx, 448);
    const u32 sp = static_cast<u32>(ctx.gpr[1]);
    const u32 t = load<u32>(o + 80), def = load<u32>(o), a = load<u32>(o + 4);
    if (t == 0 || def == 0) {
        callf(ctx, v5fn::Obj_SpawnDef0EDE30, {def, a, o + 16, kDefDirEE710, a, load<u32>(o + 112), 0, 0, 1}, {Lfs4(kKEEE80)});
        return;
    }
    const u32 v = sp + 160;
    if (call(ctx, v5fn::LauncherAimBallisticEEB08, {v, o}) == 0) {
        ctx.gpr[3] = 0;
        return;
    }
    callf(ctx, v5fn::Obj_SpawnDef0EDE30, {load<u32>(o), load<u32>(o + 4), o + 16, v, 1, load<u32>(o + 112), 0, 0, 1}, {Lfs4(sp + 176)});
    const u32 p = static_cast<u32>(ctx.gpr[3]);
    if (p == 0) return;
    u32 id[9];
    for (u32 r = 0; r < 3; ++r)
        for (u32 k = 0; k < 3; ++k) id[3 * r + k] = load<u32>(kIdentEE710 + 16 * r + 4 * k);
    const u32 tp = sp + 128;
    AimPoint(ctx, o, tp, sp + 192, sp + 240, sp + 144, kKEEE80 + 4, true);
    const u32 w0 = load<u32>(tp), w1 = load<u32>(tp + 4), w2 = load<u32>(tp + 8);
    for (u32 r = 0; r < 3; ++r)
        for (u32 k = 0; k < 3; ++k) store<u32>(p + 448 + 16 * r + 4 * k, id[3 * r + k]);
    store<u32>(p + 496, w0);
    store<u32>(p + 500, w1);
    store<u32>(p + 504, w2);
    const u32 body = load<u32>(p + 440);
    if (body != 0) VCall(ctx, body, 84, {body, p + 448});
    const u32 org = o + 64;
    for (u32 k = 0; k < 3; ++k) store<u32>(p + 512 + 4 * k, load<u32>(org + 4 * k));
    SetF1Bits(ctx, load<u32>(org + 8));
    ctx.gpr[3] = p;
}
