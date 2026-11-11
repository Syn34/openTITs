// Game code, large-function sweep part 11: object updates.
#include "big11.h"

using namespace sly4;
using namespace sly4::big11;

namespace {
constexpr u32 kMatMul = 0x005541F0u;      // Matrix_Multiply (Matrix* a, const Matrix* b): a = a x b
constexpr u32 kAxisAngle = 0x0056A410u;   // Matrix_FromAxisAngle (Matrix* out, const vec3* axis; f32 angle)
constexpr u32 kNormalize = 0x0056D374u;   // Matrix_NormalizeInPlace (Matrix*)
constexpr u32 kGetFlag = 0x002697F8u;     // GetFlag_002697F8 (o): +416 bit 31
constexpr u32 kSignedAngle = 0x00016680u; // (const vec3* a, const vec3* b) -> f1 (name?)
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
DECOMP(00016A40, Obj_UpdateMotion16A40) {
    const u32 o = arg<u32>(ctx, 0);
    const f64 dt = farg(ctx, 0);
    constexpr u32 kK = 0x00016A2Cu;   // .text f32: 0, 1, 1.1, 57.29578 (deg / rad), 0.01745329 (rad / deg)
    Stack frame(ctx, 192);
    const u32 sp = frame.addr() - 112;   // offsets below are the original frame's
    const u32 m = sp + 176;              // motion matrix (rows +0 / +16 / +32 / +48)
    for (u32 r = 0; r < 4; ++r)
        for (u32 c = 0; c < 3; ++c) CopyW(m + 16 * r + 4 * c, kIdentity + 16 * r + 4 * c);
    const u32 owner = load<u32>(o + 48);
    const u32 opd208 = load<u32>(load<u32>(owner) + 208);
    ctx.fpr[1] = dt;
    call_ptr(ctx, opd208, {owner, m, opd208, 0, load<u32>(opd208)});   // r5 = the OPD, r6 = 0
    const u32 b305 = load<u8>(o + 305);
    const u32 opd276 = load<u32>(load<u32>(o) + 276);
    if ((b305 & 0x20u) != 0 && (b305 & 0x40u) != 0)
        store<f32>(m + 52, Muls(Adds(std::fabs(F(m + 48)), std::fabs(F(m + 56))), F(o + 196)));
    call_ptr(ctx, opd276, {sp + 112, o, m + 48, load<u32>(opd276)});
    f32 x = F(sp + 112), y = F(sp + 116), z = F(sp + 120);
    CopyW(m + 48, sp + 112);
    CopyW(m + 52, sp + 116);
    CopyW(m + 56, sp + 120);
    const u32 owner2 = load<u32>(o + 48);
    const u32 opd272 = load<u32>(load<u32>(o) + 272);
    const u32 def = load<u32>(owner2 + 160);
    const u32 s = def != 0 ? def + 144 : kOnes;
    x = Muls(x, F(s));
    store<f32>(m + 48, x);
    y = Muls(y, F(s + 4));
    store<f32>(m + 52, y);
    z = Muls(z, F(s + 8));
    store<f32>(m + 56, z);
    call_ptr(ctx, opd272, {o, load<u32>(opd272)});   // r4 = the entry
    {
        const f64 k = ctx.fpr[1];
        const f32 a = Muls(F(m + 48), k), b = Muls(F(m + 52), k), c = Muls(F(m + 56), k);
        store<f32>(m + 48, a);
        store<f32>(m + 52, b);
        store<f32>(m + 56, c);
    }
    call(ctx, kMatMul, {o + 128, m});

    const f32 turn = F(o + 76);
    const f32 k0 = F(kK);
    const u32 vt = load<u32>(o);
    if (turn > k0) {
        if ((load<u8>(o + 304) & 4u) != 0) {
            const f32 dy = Subs(F(o + 100), F(o + 180));
            const f32 dx = Subs(F(o + 96), F(o + 176));
            const f32 dz = Subs(F(o + 104), F(o + 184));
            const f32 q = Fmadds(dz, dz, Fmadds(dx, dx, Muls(dy, dy)));
            if (q < F(kK + 4)) store<f32>(o + 76, Muls(turn, F(kK + 8)));
            const u32 opd80 = load<u32>(vt + 80);   // the vtable read before the +76 store
            call_ptr(ctx, opd80, {o, opd80, load<u32>(opd80)});
            const f64 lim = ctx.fpr[1];
            if (lim > k0) {
                call(ctx, kSignedAngle, {o + 192, o + 160});
                if (std::fabs(ctx.fpr[1]) > lim) VCall(ctx, o, 256, {o});
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
        store<f32>(sp + 144, Muls(h0, lh));
        store<f32>(sp + 148, Muls(k0, lh));
        store<f32>(sp + 152, Muls(h2, lh));
        store<f32>(sp + 128, Muls(g0, lg));
        store<f32>(sp + 132, Muls(k0, lg));
        store<f32>(sp + 136, Muls(g2, lg));
        call(ctx, kSignedAngle, {sp + 128, sp + 144});
        f32 a = Muls(ctx.fpr[1], F(kK + 12));
        const f32 neg = -maxa;
        a = static_cast<f64>(Subs(maxa, a)) >= 0.0 ? a : maxa;   // fsel: min(a, maxa)
        a = static_cast<f64>(Subs(a, neg)) >= 0.0 ? a : neg;     // fsel: max(a, -maxa)
        callf(ctx, kAxisAngle, {sp + 240, kAxisUp}, {Muls(a, F(kK + 16))});
        CopyW(sp + 288, kZero3);
        CopyW(sp + 292, kZero3 + 4);
        CopyW(sp + 296, kZero3 + 8);
        call(ctx, kMatMul, {o + 128, sp + 240});
    }

    const u32 tgt = load<u32>(o + 68);
    if (tgt != 0 && static_cast<u32>(call(ctx, kGetFlag, {tgt})) != 0) {
        const f32 inv = Divs(F(kK + 4), dt);
        const u32 t2 = load<u32>(o + 68);
        const f32 a0 = F(t2 + 480), a1 = F(t2 + 484), a2 = F(t2 + 488);
        const f32 p0 = F(o + 176), p1 = F(o + 180), p2 = F(o + 184);
        store<f32>(sp + 160, Muls(Subs(p0, a0), inv));
        store<f32>(sp + 164, Muls(Subs(p1, a1), inv));
        store<f32>(sp + 168, Muls(Subs(p2, a2), inv));
        if (static_cast<u32>(VCall(ctx, o, 176, {o, sp + 160})) != 0) {
            VCall(ctx, o, 296, {o});
            store<f32>(sp + 164, Adds(F(sp + 164), ctx.fpr[1]));
        }
        const u32 t3 = load<u32>(o + 68);
        const u32 body = static_cast<u32>(VCall(ctx, t3, 8, {t3}));
        const u32 w = load<u32>(body + 168);
        store<u32>(body + 152, 0);
        store<u32>(body + 128, 0);
        store<u32>(body + 168, w | 0x02000000u);
        CopyW(body + 0, sp + 160);
        CopyW(body + 4, sp + 164);
        const u32 lw = load<u32>(sp + 168);
        store<u32>(body + 8, lw);
        const u32 r = (load<u32>(load<u32>(o + 68) + 416) >> 28) & 7u;
        store<u32>(o + 300, r);
        ctx.gpr[3] = r;
        SetF1Bits(ctx, lw);
        return;
    }
    // Re-orthonormalise the frame: row0 = row1 x row2, row1 = row2 x row0.
    const u32 r0 = o + 128, r1 = o + 144, r2 = o + 160;
    store<f32>(r0 + 0, Fmsubs(F(r1 + 4), F(r2 + 8), Muls(F(r1 + 8), F(r2 + 4))));
    store<f32>(r0 + 4, Fmsubs(F(r1 + 8), F(r2 + 0), Muls(F(r1 + 0), F(r2 + 8))));
    const f32 c1 = F(r0 + 4);
    const f32 c2 = Fmsubs(F(r1 + 0), F(r2 + 4), Muls(F(r1 + 4), F(r2 + 0)));
    store<f32>(r0 + 8, c2);
    store<f32>(r1 + 0, Fmsubs(F(r2 + 4), c2, Muls(F(r2 + 8), c1)));
    store<f32>(r1 + 4, Fmsubs(F(r2 + 8), F(r0 + 0), Muls(F(r2 + 0), F(r0 + 8))));
    store<f32>(r1 + 8, Fmsubs(F(r2 + 0), F(r0 + 4), Muls(F(r2 + 4), F(r0 + 0))));
    call(ctx, kNormalize, {r0});
    if (static_cast<u32>(VCall(ctx, o, 128, {o})) == 0) {
        const u32 ow = load<u32>(o + 48);
        VCall(ctx, ow, 84, {ow, r0});
        return;
    }
    const u32 ow = load<u32>(o + 48);
    SetF1Bits(ctx, CopyMat43Last(ow + 96, r0));
    ctx.gpr[3] = ow;
}
