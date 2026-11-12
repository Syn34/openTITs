// Game code, large-function sweep part 30: pushing a camera object's
// transform, field of view and lens settings to its render target.
#include "../sly4.h"
#include "big30.h"

using namespace sly4;
using namespace sly4::big30;
using big5::Lvx;
using big5::Stvx;
using big5::V4;
using mid15::Fb;

namespace {
constexpr u32 Obj_SetMatrixD87CC = 0x000D87CC;      // (Obj*, Mtx* io)
constexpr u32 RenderTarget_SetMatrix = 0x005201B0;  // (RenderTarget*, const Mtx*)
constexpr u32 View_SetFov = 0x00520368;             // (RenderTarget*, f1 fov)
constexpr u32 Matrix_Multiply = 0x005541F0;         // (Mtx* io, const Mtx*)
constexpr u32 kK = 0x000FADD0;  // .rodata: two vperm masks, then 0.01, 0.0, 1.0, 999.0
constexpr u32 kGame = 0x00F65CA0;

inline f64 LfsD(u32 ea) { return std::bit_cast<f64>(mid1::LfsBits(load<u32>(ea))); }
inline f32 Madd(f32 a, f32 c, f32 b) { return std::fma(a, c, b); }
// lfs x; if x < lo: lo, else if x > hi: hi (the stored word, raw).
inline u32 Clamp(u32 xw, u32 low, u32 hiw) {
    const f64 x = std::bit_cast<f64>(mid1::LfsBits(xw));
    if (x < std::bit_cast<f64>(mid1::LfsBits(low))) return low;
    if (x > std::bit_cast<f64>(mid1::LfsBits(hiw))) return hiw;
    return xw;
}
}  // namespace

// void f(CamObj* o, u8 force) (called by Obj_ApplyDesc_000FB31C,
// Obj_StepParts_000FBA18, Obj_RestoreFrom544). From the 4x4 matrix at
// o+96 a local matrix L (sp+112) of rows -r0, r1, -r2, r3 (x/y/z only;
// the w lanes are left as stack garbage, as in the original) is built.
// With force, or neither bit 6 nor bit 5 of o->+172, Obj_SetMatrixD87CC(o,
// L) updates it first. With u8 o->+604: either (u8 +605) L's rows go
// through o's matrix at +624 (rows x/y/z * the 3 basis rows, translation =
// +672 row + the basis rows * 0, plus the old translation) or
// Matrix_Multiply(L, o+624). The inverse of the rigid L (transposed 3x3,
// translation -t * R^T) goes to RenderTarget_SetMatrix(o->+364, it) and
// View_SetFov(o->+364, o->+356). With bit 30 of o->+468: (bit 29 and a
// lens q = rt->+656->+12) with u8 +440 q gets +24 = o->+360, +28 =
// clamp(o->+444, 0.01, 999), +32 = o->+448, u8 +64 = clamp(u8 o->+455,
// 1, 4); then q->vfn[2](q, +440 | game's +0x1281C0->+769 | bit 29 of
// game's +0x128304 != 0). Then with p = rt->+656->+24: when u8 o->+456 is
// set p->vfn[2](p, it), p->+20 = clamp(o->+460, 0, 1) and p->+32..+40 =
// the u8s o->+464..466 as floats; always p->vfn[2](p, o->+456). No caller
// reads r3, f1 or other leftovers.
DECOMP(000FAE00, CamObj_PushView_000FAE00) {
    const u32 o = arg<u32>(ctx, 0);
    const bool force = static_cast<u8>(ctx.gpr[4]) != 0;
    Stack frame(ctx, 128);
    const u32 L = frame.addr(), B = L + 64;
    const u32 m = o + 96;
    for (u32 j = 0; j < 3; ++j) {
        store<u32>(L + j * 4, load<u32>(m + j * 4) ^ 0x80000000u);
        store<u32>(L + 16 + j * 4, load<u32>(m + 16 + j * 4));
        store<u32>(L + 32 + j * 4, load<u32>(m + 32 + j * 4) ^ 0x80000000u);
        store<u32>(L + 48 + j * 4, load<u32>(m + 48 + j * 4));
    }
    f64 t0 = LfsD(m + 48), t1 = LfsD(m + 52), t2 = LfsD(m + 56);
    const u32 f172 = load<u32>(o + 172);
    if (force || !(f172 & 0x60)) {
        call(ctx, Obj_SetMatrixD87CC, {o, L});
        t0 = LfsD(L + 48);
        t1 = LfsD(L + 52);
        t2 = LfsD(L + 56);
    }
    if (load<u8>(o + 604) != 0) {
        if (load<u8>(o + 605) != 0) {
            store<u32>(L + 56, 0);
            store<u32>(L + 52, 0);
            store<u32>(L + 48, 0);
            const V4 a0 = Lvx(o + 624), a1 = Lvx(o + 640), a2 = Lvx(o + 656), a3 = Lvx(o + 672);
            const V4 r0 = Lvx(L), r1 = Lvx(L + 16), r2 = Lvx(L + 32);
            V4 n0, n1, n2, n3;
            for (u32 k = 0; k < 4; ++k) {
                const f32 z0 = Madd(a2.e[k], 0.0f, Madd(a1.e[k], 0.0f, Madd(a0.e[k], 0.0f, 0.0f)));
                n0.e[k] = Madd(a2.e[k], r0.e[2], Madd(a1.e[k], r0.e[1], Madd(a0.e[k], r0.e[0], 0.0f)));
                n1.e[k] = Madd(a2.e[k], r1.e[2], Madd(a1.e[k], r1.e[1], Madd(a0.e[k], r1.e[0], 0.0f)));
                n2.e[k] = Madd(a2.e[k], r2.e[2], Madd(a1.e[k], r2.e[1], Madd(a0.e[k], r2.e[0], 0.0f)));
                n3.e[k] = a3.e[k] + z0;
            }
            Stvx(L, n0);
            Stvx(L + 16, n1);
            Stvx(L + 32, n2);
            Stvx(L + 48, n3);
            const f64 x = LfsD(L + 48), y = LfsD(L + 52), z = LfsD(L + 56);
            store<u32>(L + 48, Fb(x + t0));
            store<u32>(L + 52, Fb(y + t1));
            store<u32>(L + 56, Fb(z + t2));
        } else {
            call(ctx, Matrix_Multiply, {L, o + 624});
        }
    }
    {
        const V4 r0 = Lvx(L), r1 = Lvx(L + 16), r2 = Lvx(L + 32), r3 = Lvx(L + 48);
        const f32 nt0 = 0.0f - r3.e[0], nt1 = 0.0f - r3.e[1], nt2 = 0.0f - r3.e[2];
        const V4 c0{{r0.e[0], r1.e[0], r2.e[0], r1.e[1]}};
        const V4 c1{{r0.e[1], r1.e[1], r2.e[1], r0.e[0]}};
        const V4 c2{{r0.e[2], r1.e[2], r2.e[2], r0.e[2]}};
        V4 tr;
        for (u32 k = 0; k < 4; ++k) tr.e[k] = Madd(c2.e[k], nt2, Madd(c1.e[k], nt1, Madd(c0.e[k], nt0, 0.0f)));
        Stvx(B, c0);
        Stvx(B + 16, c1);
        Stvx(B + 32, c2);
        Stvx(B + 48, tr);
    }
    call(ctx, RenderTarget_SetMatrix, {load<u32>(o + 364), B});
    callf(ctx, View_SetFov, {load<u32>(o + 364)}, {LfsD(o + 356)});
    const u32 f468 = load<u32>(o + 468);
    if (!(f468 & 0x40000000u)) return;
    if (f468 & 0x20000000u) {
        const u32 q = load<u32>(load<u32>(load<u32>(o + 364) + 656) + 12);
        if (q != 0) {
            const u32 b440 = load<u8>(o + 440);
            if (b440 != 0) {
                store<u32>(q + 24, load<u32>(o + 360));
                store<u32>(q + 28, Clamp(load<u32>(o + 444), load<u32>(kK + 32), load<u32>(kK + 44)));
                store<u32>(q + 32, load<u32>(o + 448));
                const u32 b = load<u32>(o + 452) & 0xFF;
                store<u8>(q + 64, static_cast<u8>(b < 1 ? 1 : (b > 4 ? 4 : b)));
            }
            const u32 g = load<u32>(kGame);
            const u32 s = load<u32>(g + 0x1281C0);
            const u32 r7 = s != 0 ? load<u8>(s + 769) : 0;
            const u32 r8 = (b440 | r7) != 0 ? 1 : 0;
            const u32 on = (r8 | ((load<u32>(g + 0x128304) >> 29) & 1)) != 0 ? 1 : 0;
            const u32 opd = load<u32>(load<u32>(q) + 8);
            // r5 = !on and r6 = the entry, as the original's call sequence leaves them.
            call_ptr(ctx, opd, {q, on, on ^ 1u, load<u32>(opd), r7, r8});
        }
    }
    const u32 p = load<u32>(load<u32>(load<u32>(o + 364) + 656) + 24);
    if (p == 0) return;
    const u32 b456 = load<u8>(o + 456);
    if (b456 != 0) {
        VCall(ctx, p, 8, {p, b456});
        store<u32>(p + 20, Clamp(load<u32>(o + 460), load<u32>(kK + 36), load<u32>(kK + 40)));
        for (u32 j = 0; j < 3; ++j) store<u32>(p + 32 + 4 * j, Fb(static_cast<f64>(load<u8>(o + 464 + j))));
    }
    VCall(ctx, p, 8, {p, static_cast<u32>(load<u8>(o + 456))});
}
