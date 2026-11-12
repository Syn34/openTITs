// Game code, large-function sweep part 11: object updates.
#include "big11.h"
#include "big12.h"

namespace sly4::obj {

namespace {
inline be_u32& W(u32 base, u32 off) { return gm<be_u32>(base + off); }
// lfs as the verified code reads it: the f32 word (promoted when used).
inline f32 F(u32 ea) { return std::bit_cast<f32>(gm<be_u32>(ea).get()); }
inline void St(u32 ea, f64 v) { gm<be_u32>(ea) = StfsBits(v); }
inline void CopyW(u32 dst, u32 src) { gm<be_u32>(dst) = gm<be_u32>(src).get(); }
}  // namespace

namespace {
constexpr u32 kMatMul = 0x005541F0u;      // Matrix_Multiply (Matrix* a, const Matrix* b): a = a x b
constexpr u32 kAxisAngle = 0x0056A410u;   // Matrix_FromAxisAngle (Matrix* out, const vec3* axis; f32 angle)
constexpr u32 kNormalize = 0x0056D374u;   // Matrix_NormalizeInPlace (Matrix*)
constexpr u32 kGetFlag = 0x002697F8u;     // GetFlag_002697F8 (o): +416 bit 31
constexpr u32 kIdentity = 0x01164BD0u;    // identity matrix rows
constexpr u32 kOnes = 0x01164B30u;        // default scale vector
constexpr u32 kZero3 = 0x01164B40u;       // zero vector
constexpr u32 kAxisUp = 0x01164B60u;      // the turn axis
}  // namespace

// void Obj_UpdateMotion16A40(Obj* o; f32 dt) (name?): the owner (+48) fills
// a motion matrix (vfunc +208; with +305 bits 0x20 | 0x40 its y gets
// (|x| + |z|) * +196), the step (vfunc +276) scaled by the owner's def
// (+160 +144, or ones) and vfunc +272 is applied to the frame +128; while
// +76 > 0 (shrunk by K8 when within K4 of +96 under +304 bit 4; vfunc +256
// when the angle between +192 and +160 exceeds vfunc +80) it turns towards
// +192 by at most +76 * dt degrees; then with a live +68 the frame's
// position goes to it as a velocity (vfunc +176, +296 added to y) and +300 =
// its +416 >> 28 & 7, else the frame is re-orthonormalised and handed to
// the owner (vfunc +84, or copied to its +96 rows when vfunc +128 says so).
void Obj_UpdateMotion16A40(u32 o, f64 dt) {
    constexpr u32 kK = 0x00016A2Cu;   // .text f32: 0, 1, 1.1, 57.29578 (deg / rad), 0.01745329 (rad / deg)
    GuestFrame frame(192);
    const u32 sp = frame.addr() - 112;   // offsets below are the original frame's
    const u32 m = sp + 176;              // motion matrix (rows +0 / +16 / +32 / +48)
    for (u32 r = 0; r < 4; ++r)
        for (u32 c = 0; c < 3; ++c) CopyW(m + 16 * r + 4 * c, kIdentity + 16 * r + 4 * c);
    const u32 owner = gm<be_u32>(o + 48);
    const u32 opd208 = gm<be_u32>(gm<be_u32>(owner) + 208);
    call_ptr(opd208, owner, m, dt);   // f1 = dt (the float in the r5 slot)
    const u32 b305 = gm<u8>(o + 305);
    const u32 opd276 = gm<be_u32>(gm<be_u32>(o) + 276);
    if ((b305 & 0x20u) != 0 && (b305 & 0x40u) != 0)
        gm<be_f32>(m + 52) = Muls(Adds(std::fabs(F(m + 48)), std::fabs(F(m + 56))), F(o + 196));
    call_ptr(opd276, sp + 112, o, m + 48);
    f32 x = F(sp + 112), y = F(sp + 116), z = F(sp + 120);
    CopyW(m + 48, sp + 112);
    CopyW(m + 52, sp + 116);
    CopyW(m + 56, sp + 120);
    const u32 owner2 = gm<be_u32>(o + 48);
    const u32 opd272 = gm<be_u32>(gm<be_u32>(o) + 272);
    const u32 def = gm<be_u32>(owner2 + 160);
    const u32 s = def != 0 ? def + 144 : kOnes;
    x = Muls(x, F(s));
    gm<be_f32>(m + 48) = x;
    y = Muls(y, F(s + 4));
    gm<be_f32>(m + 52) = y;
    z = Muls(z, F(s + 8));
    gm<be_f32>(m + 56) = z;
    {
        const f64 k = call_ptr<f64>(opd272, o);
        const f32 a = Muls(F(m + 48), k), b = Muls(F(m + 52), k), c = Muls(F(m + 56), k);
        gm<be_f32>(m + 48) = a;
        gm<be_f32>(m + 52) = b;
        gm<be_f32>(m + 56) = c;
    }
    call(kMatMul, o + 128, m);

    const f32 turn = F(o + 76);
    const f32 k0 = F(kK);
    const u32 vt = gm<be_u32>(o);
    if (turn > k0) {
        if ((gm<u8>(o + 304) & 4u) != 0) {
            const f32 dy = Subs(F(o + 100), F(o + 180));
            const f32 dx = Subs(F(o + 96), F(o + 176));
            const f32 dz = Subs(F(o + 104), F(o + 184));
            const f32 q = Fmadds(dz, dz, Fmadds(dx, dx, Muls(dy, dy)));
            if (q < F(kK + 4)) gm<be_f32>(o + 76) = Muls(turn, F(kK + 8));
            const u32 opd80 = gm<be_u32>(vt + 80);   // the vtable read before the +76 store
            const f64 lim = call_ptr<f64>(opd80, o);
            if (lim > k0) {
                if (std::fabs(Vec_SignedAngle16680(o + 192, o + 160)) > lim) vcall(host<u8>(o), 256);
            }
        }
        const f32 sq = Muls(k0, k0);
        const f32 g0 = F(o + 192), h0 = F(o + 160), g2 = F(o + 200);
        f32 lh = Fmadds(h0, h0, sq);
        const f32 h2 = F(o + 168);
        f32 lg = Fmadds(g0, g0, sq);
        lh = Fmadds(h2, h2, lh);
        lg = Fmadds(g2, g2, lg);
        const f32 one = F(kK + 4);
        const f32 rate = F(o + 76);
        lh = static_cast<f32>(std::sqrt(static_cast<f64>(lh)));
        lg = static_cast<f32>(std::sqrt(static_cast<f64>(lg)));
        const f32 maxa = Muls(rate, dt);
        lh = Divs(one, lh);
        lg = Divs(one, lg);
        gm<be_f32>(sp + 144) = Muls(h0, lh);
        gm<be_f32>(sp + 148) = Muls(k0, lh);
        gm<be_f32>(sp + 152) = Muls(h2, lh);
        gm<be_f32>(sp + 128) = Muls(g0, lg);
        gm<be_f32>(sp + 132) = Muls(k0, lg);
        gm<be_f32>(sp + 136) = Muls(g2, lg);
        f32 a = Muls(Vec_SignedAngle16680(sp + 128, sp + 144), F(kK + 12));
        const f32 neg = -maxa;
        a = static_cast<f64>(Subs(maxa, a)) >= 0.0 ? a : maxa;   // fsel: min(a, maxa)
        a = static_cast<f64>(Subs(a, neg)) >= 0.0 ? a : neg;     // fsel: max(a, -maxa)
        call(kAxisAngle, sp + 240, kAxisUp, Muls(a, F(kK + 16)));
        CopyW(sp + 288, kZero3);
        CopyW(sp + 292, kZero3 + 4);
        CopyW(sp + 296, kZero3 + 8);
        call(kMatMul, o + 128, sp + 240);
    }

    const u32 tgt = gm<be_u32>(o + 68);
    if (tgt != 0 && call<u32>(kGetFlag, tgt) != 0) {
        const f32 inv = Divs(F(kK + 4), dt);
        const u32 t2 = gm<be_u32>(o + 68);
        const f32 a0 = F(t2 + 480), a1 = F(t2 + 484), a2 = F(t2 + 488);
        const f32 p0 = F(o + 176), p1 = F(o + 180), p2 = F(o + 184);
        gm<be_f32>(sp + 160) = Muls(Subs(p0, a0), inv);
        gm<be_f32>(sp + 164) = Muls(Subs(p1, a1), inv);
        gm<be_f32>(sp + 168) = Muls(Subs(p2, a2), inv);
        if (vcall<u32>(host<u8>(o), 176, sp + 160) != 0) {
            gm<be_f32>(sp + 164) = Adds(F(sp + 164), vcall<f64>(host<u8>(o), 296));
        }
        const u32 t3 = gm<be_u32>(o + 68);
        const u32 body = vcall<u32>(host<u8>(t3), 8);
        const u32 w = gm<be_u32>(body + 168);
        gm<be_u32>(body + 152) = 0;
        gm<be_u32>(body + 128) = 0;
        gm<be_u32>(body + 168) = w | 0x02000000u;
        CopyW(body + 0, sp + 160);
        CopyW(body + 4, sp + 164);
        const u32 lw = gm<be_u32>(sp + 168);
        gm<be_u32>(body + 8) = lw;
        const u32 r = (gm<be_u32>(gm<be_u32>(o + 68) + 416) >> 28) & 7u;
        gm<be_u32>(o + 300) = r;
        return;
    }
    // Re-orthonormalise the frame: row0 = row1 x row2, row1 = row2 x row0.
    const u32 r0 = o + 128, r1 = o + 144, r2 = o + 160;
    gm<be_f32>(r0 + 0) = Fmsubs(F(r1 + 4), F(r2 + 8), Muls(F(r1 + 8), F(r2 + 4)));
    gm<be_f32>(r0 + 4) = Fmsubs(F(r1 + 8), F(r2 + 0), Muls(F(r1 + 0), F(r2 + 8)));
    const f32 c1 = F(r0 + 4);
    const f32 c2 = Fmsubs(F(r1 + 0), F(r2 + 4), Muls(F(r1 + 4), F(r2 + 0)));
    gm<be_f32>(r0 + 8) = c2;
    gm<be_f32>(r1 + 0) = Fmsubs(F(r2 + 4), c2, Muls(F(r2 + 8), c1));
    gm<be_f32>(r1 + 4) = Fmsubs(F(r2 + 8), F(r0 + 0), Muls(F(r2 + 0), F(r0 + 8)));
    gm<be_f32>(r1 + 8) = Fmsubs(F(r2 + 0), F(r0 + 4), Muls(F(r2 + 4), F(r0 + 0)));
    call(kNormalize, r0);
    if (vcall<u32>(host<u8>(o), 128) == 0) {
        const u32 ow = gm<be_u32>(o + 48);
        vcall(host<u8>(ow), 84, r0);
        return;
    }
    const u32 ow = gm<be_u32>(o + 48);
    CopyMtxXYZ(gm<Mtx44>(ow + 96), gm<Mtx44>(r0));
}

}  // namespace sly4::obj

NATIVE(00016A40, sly4::obj::Obj_UpdateMotion16A40)
