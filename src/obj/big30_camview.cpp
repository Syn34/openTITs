// Game code, large-function sweep part 30: pushing a camera object's
// transform, field of view and lens settings to its render target.
#include "big30.h"
#include "native/vec.h"

namespace sly4::obj {

namespace {
constexpr u32 Obj_SetMatrixD87CC = 0x000D87CC;      // (Obj*, Mtx* io)
constexpr u32 RenderTarget_SetMatrix = 0x005201B0;  // (RenderTarget*, const Mtx*)
constexpr u32 View_SetFov = 0x00520368;             // (RenderTarget*, f1 fov)
constexpr u32 Matrix_Multiply = 0x005541F0;         // (Mtx* io, const Mtx*)
constexpr u32 kK = 0x000FADD0;  // .rodata: two vperm masks, then 0.01, 0.0, 1.0, 999.0

// The render target's lens settings holder: +12 the depth-of-field
// object, +24 the color filter (both with vfn[2] = enable(x, u8)).
struct CamLens {
    pad<12> _0;
    be_u32 dof;     // +12
    pad<8> _16;
    be_u32 filter;  // +24
};
struct CamTarget {
    pad<656> _0;
    gp<CamLens> lens;  // +656
};
// The depth-of-field object: +24 focus, +28 range, +32 blur, u8 +64 the
// quality (1..4).
struct CamDof {
    pad<24> _0;
    be_f32 focus;   // +24
    be_u32 range;   // +28 (the clamped word)
    be_f32 blur;    // +32
    pad<28> _36;
    u8 quality;     // +64
};
static_assert(offsetof(CamDof, quality) == 64);
// The color filter: +20 the strength (clamped 0..1), +32..+40 the color.
struct CamFilter {
    pad<20> _0;
    be_u32 strength;  // +20 (the clamped word)
    pad<8> _24;
    be_f32 rgb[3];    // +32
};
static_assert(offsetof(CamFilter, rgb) == 32);
struct CamObj {
    pad<96> _0;
    Mtx44 m;              // +96
    pad<12> _160;
    be_u32 flags;        // +172: bits 6 / 5 skip the matrix update
    pad<180> _176;
    be_f32 fov;          // +356
    be_f32 focus;        // +360
    gp<CamTarget> rt;    // +364
    pad<72> _368;
    u8 dof_on;           // +440
    pad<3> _441;
    be_u32 range;        // +444
    be_f32 blur;         // +448
    pad<3> _452;
    u8 quality;          // +455
    u8 filter_on;        // +456
    pad<3> _457;
    be_u32 strength;     // +460
    u8 rgb[3];           // +464
    pad<1> _467;
    be_u32 lens_flags;   // +468: bit 30 lens settings, bit 29 depth of field
    pad<132> _472;
    u8 parent_on;        // +604
    u8 parent_basis;     // +605
    pad<18> _606;
    Mtx44 parent;         // +624
};
static_assert(offsetof(CamObj, flags) == 172 && offsetof(CamObj, fov) == 356 && offsetof(CamObj, rt) == 364);
static_assert(offsetof(CamObj, dof_on) == 440 && offsetof(CamObj, quality) == 455 && offsetof(CamObj, rgb) == 464);
static_assert(offsetof(CamObj, lens_flags) == 468 && offsetof(CamObj, parent_on) == 604 && offsetof(CamObj, parent) == 624);

// A row as four raw words.
Quad& Q(Vec4& v) { return reinterpret_cast<Quad&>(v); }
const Quad& Q(const Vec4& v) { return reinterpret_cast<const Quad&>(v); }

// if x < lo: lo, else if x > hi: hi (the stored word, raw).
u32 Clamp(const be_u32& x, const be_f32& lo, const be_f32& hi) {
    const u32 xw = x;
    const f64 v = LfsW(xw);
    if (v < lf(lo)) return bits(lo);
    if (v > lf(hi)) return bits(hi);
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
void CamObj_PushView_000FAE00(CamObj* o, u8 force) {
    GuestFrame frame(128);
    Mtx44* L = frame.as<Mtx44>(0);
    Mtx44* B = frame.as<Mtx44>(64);
    const Mtx44& m = o->m;
    for (u32 j = 0; j < 3; ++j) {  // rows -r0, r1, -r2, r3 (sign flips on the raw words)
        Q(L->r[0]).w[j] = Q(m.r[0]).w[j] ^ 0x80000000u;
        Q(L->r[1]).w[j] = Q(m.r[1]).w[j].get();
        Q(L->r[2]).w[j] = Q(m.r[2]).w[j] ^ 0x80000000u;
        Q(L->r[3]).w[j] = Q(m.r[3]).w[j].get();
    }
    f64 t0 = lf(m.r[3].x), t1 = lf(m.r[3].y), t2 = lf(m.r[3].z);
    if (force != 0 || !(o->flags & 0x60u)) {
        call(Obj_SetMatrixD87CC, o, L);
        t0 = lf(L->r[3].x);
        t1 = lf(L->r[3].y);
        t2 = lf(L->r[3].z);
    }
    if (o->parent_on != 0) {
        if (o->parent_basis != 0) {
            set_bits(L->r[3].z, 0u);
            set_bits(L->r[3].y, 0u);
            set_bits(L->r[3].x, 0u);
            const Mtx44& P = o->parent;
            const V4 a0 = Lvx(&P.r[0]), a1 = Lvx(&P.r[1]), a2 = Lvx(&P.r[2]), a3 = Lvx(&P.r[3]);
            const V4 r0 = Lvx(&L->r[0]), r1 = Lvx(&L->r[1]), r2 = Lvx(&L->r[2]);
            const V4 z0 = Row3(a0, a1, a2, 0.0f, 0.0f, 0.0f);
            Stvx(&L->r[0], Row3(a0, a1, a2, r0.e[0], r0.e[1], r0.e[2]));
            Stvx(&L->r[1], Row3(a0, a1, a2, r1.e[0], r1.e[1], r1.e[2]));
            Stvx(&L->r[2], Row3(a0, a1, a2, r2.e[0], r2.e[1], r2.e[2]));
            Stvx(&L->r[3], Add4(a3, z0));
            const f64 x = lf(L->r[3].x), y = lf(L->r[3].y), z = lf(L->r[3].z);
            L->r[3].x = static_cast<f32>(x + t0);
            L->r[3].y = static_cast<f32>(y + t1);
            L->r[3].z = static_cast<f32>(z + t2);
        } else {
            call(Matrix_Multiply, L, &o->parent);
        }
    }
    {
        const V4 r0 = Lvx(&L->r[0]), r1 = Lvx(&L->r[1]), r2 = Lvx(&L->r[2]), r3 = Lvx(&L->r[3]);
        const f32 nt0 = 0.0f - r3.e[0], nt1 = 0.0f - r3.e[1], nt2 = 0.0f - r3.e[2];
        const V4 c0{{r0.e[0], r1.e[0], r2.e[0], r1.e[1]}};
        const V4 c1{{r0.e[1], r1.e[1], r2.e[1], r0.e[0]}};
        const V4 c2{{r0.e[2], r1.e[2], r2.e[2], r0.e[2]}};
        Stvx(&B->r[0], c0);
        Stvx(&B->r[1], c1);
        Stvx(&B->r[2], c2);
        Stvx(&B->r[3], Row3(c0, c1, c2, nt0, nt1, nt2));
    }
    call(RenderTarget_SetMatrix, o->rt.ea, B);
    call(View_SetFov, o->rt.ea, lf(o->fov));
    const u32 f468 = o->lens_flags;
    if (!(f468 & 0x40000000u)) return;
    if (f468 & 0x20000000u) {
        const u32 q = gm<CamLens>(gm<CamTarget>(o->rt.ea).lens.ea).dof;
        if (q != 0) {
            CamDof& d = gm<CamDof>(q);
            const u32 b440 = o->dof_on;
            if (b440 != 0) {
                d.focus = o->focus;
                d.range = Clamp(o->range, global<be_f32>(kK + 32), global<be_f32>(kK + 44));
                d.blur = o->blur;
                const u32 b = o->quality;
                d.quality = static_cast<u8>(b < 1 ? 1 : (b > 4 ? 4 : b));
            }
            const u32 g = global<be_u32>(kWorld);
            const u32 s = gm<be_u32>(g + 0x1281C0);
            const u32 r7 = s != 0 ? gm<u8>(s + 769) : 0u;
            const u32 r8 = (b440 | r7) != 0 ? 1u : 0u;
            const u32 on = (r8 | ((gm<be_u32>(g + 0x128304) >> 29) & 1)) != 0 ? 1u : 0u;
            vcall(host<u8>(q), 8, on);
        }
    }
    const u32 p = gm<CamLens>(gm<CamTarget>(o->rt.ea).lens.ea).filter;
    if (p == 0) return;
    CamFilter& f = gm<CamFilter>(p);
    const u32 b456 = o->filter_on;
    if (b456 != 0) {
        vcall(host<u8>(p), 8, b456);
        f.strength = Clamp(o->strength, global<be_f32>(kK + 36), global<be_f32>(kK + 40));
        for (u32 j = 0; j < 3; ++j) f.rgb[j] = static_cast<f32>(o->rgb[j]);
    }
    vcall(host<u8>(p), 8, static_cast<u32>(o->filter_on));
}

}  // namespace sly4::obj

NATIVE(000FAE00, sly4::obj::CamObj_PushView_000FAE00)
