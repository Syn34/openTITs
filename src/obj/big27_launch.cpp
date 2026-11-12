// Game code, large-function sweep part 27: an object launching the piece it
// holds (detached, aimed, given linear and random angular velocity).
#include "../sly4.h"
#include "big27.h"

using namespace sly4;
using namespace sly4::big27;

namespace {
constexpr u32 kPool3A9 = 0x003A9EB0;  // .text pool: f32 1.0; masks {y} (+16), {z} (+32), {w} (+48); -1.0 (+64), 1/65535 (+68)
constexpr u32 kFxHash = 0xC01710E7u;  // effect hash set on the piece's effect
constexpr u32 kPropIndex = 0x0026E1FEu;  // body property: o->+400 (sign-extended)
constexpr u32 kPropHeldFlag = 0x516CC502u;   // body property: o->+384 bit 0x08000000
constexpr u32 kRow[12] = {0, 4, 8, 16, 20, 24, 32, 36, 40, 48, 52, 56};
}  // namespace

// u32 f(T* o) (called by Obj_OnEvent_003AA9F0, Obj_OnEventAdj64_003AAA64).
// Without the held piece x (o->+368) or its holder h (o->+372) returns o.
// x leaves h (+368 = 0, Obj_Detach). The launch point is the def's
// (o->+212) +528.. taken through the matrix (+80..) of the object at
// o->+140[o->+400] (80-byte entries) +4 when set; x gets h's matrix
// (+96..) re-aimed along the unit direction from h's origin to that point
// (Matrix_FromDir), with def+512.. through it as origin (x->vfn[84]); its
// effect (x->vfn[272]) gets kFxHash. With x's body b (x->vfn[144], its +8
// set): linear velocity = the aim row * def+584, angular velocity = one
// random sign times lerp(def+544.., def+560.., 16-bit randoms / 65535) per
// axis (the global generator advanced four times), both after
// Hk_Activate(b) through b+224's vfn[68] / vfn[72]; o+56 / o+60 listen on
// b; {b, x} is pushed on o's array (+72 u16 capacity, +74 size, +76 high
// mark, +78 |= 1, +80 data); properties kPropIndex / kPropHeldFlag; the entry's
// +76 count is incremented and returned (0 without the body).
DECOMP(003A9EF8, Obj_LaunchHeld_003A9EF8) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 x = load<u32>(o + 368);
    if (x == 0) return;  // r3 = o
    if (load<u32>(o + 372) == 0) return;
    store<u32>(o + 368, 0);
    call(ctx, b27fn::Obj_Detach, {load<u32>(o + 372), x});
    Stack frame(ctx, 416);
    const u32 sp = frame.addr();
    const u32 M = sp + 176, DIR = sp + 128, Q = sp + 144, V = sp + 160;
    const u32 hm = load<u32>(o + 372) + 96;
    const u32 ent = load<u32>(o + 140) + load<u32>(o + 400) * 80;
    const u32 def = load<u32>(o + 212);
    u32 m[12];
    for (u32 k = 0; k < 12; ++k) m[k] = load<u32>(hm + kRow[k]);
    u32 px = load<u32>(def + 528), py = load<u32>(def + 532), pz = load<u32>(def + 536);
    if (const u32 t = load<u32>(ent + 4); t != 0) {
        const V4 r0 = Lvx(t + 80), r1 = Lvx(t + 96), r2 = Lvx(t + 112), r3 = Lvx(t + 128);
        const V4 p = Add4N26(r3, Row3N26(r0, r1, r2, std::bit_cast<f32>(px), std::bit_cast<f32>(py), std::bit_cast<f32>(pz)));
        px = std::bit_cast<u32>(p.e[0]), py = std::bit_cast<u32>(p.e[1]), pz = std::bit_cast<u32>(p.e[2]);
    }
    const f64 dy = Subs(LfsF(py), LfsF(m[10])), dx = Subs(LfsF(px), LfsF(m[9])), dz = Subs(LfsF(pz), LfsF(m[11]));
    const f64 len = Sqrts(Fmadds(dz, dz, Fmadds(dx, dx, Muls(dy, dy))));
    const f64 one = Lf(kPool3A9);
    for (u32 k = 0; k < 12; ++k) store<u32>(M + kRow[k], m[k]);
    const f64 inv = Divs(one, len);
    Sf(DIR, Muls(dx, inv));
    Sf(DIR + 4, Muls(dy, inv));
    Sf(DIR + 8, Muls(dz, inv));
    call(ctx, b27fn::Matrix_FromDir, {DIR, M});
    {
        const u32 q = load<u32>(o + 212) + 512;
        for (u32 k = 0; k < 3; ++k) store<u32>(Q + 4 * k, load<u32>(q + 4 * k));
        const V4 m0 = Lvx(M), m1 = Lvx(M + 16), m2 = Lvx(M + 32), m3 = Lvx(M + 48);
        Stvx(Q, Add4N26(m3, Row3N26(m0, m1, m2, Fw27(q), Fw27(q + 4), Fw27(q + 8))));
        for (u32 k = 0; k < 3; ++k) store<u32>(M + 48 + 4 * k, load<u32>(Q + 4 * k));
        SetF1(ctx, Q + 8);  // leftover f1 (the last lfs)
    }
    VCall(ctx, x, 84, {x, M});
    if (const u32 e = static_cast<u32>(VCall(ctx, x, 272, {x})); e != 0) call(ctx, b27fn::Effect_SetHash, {e, kFxHash});
    const u32 b = static_cast<u32>(VCall(ctx, x, 144, {x}));
    if (b == 0 || load<u32>(b + 8) == 0) {
        ret(ctx, 0u);
        return;
    }
    const Masks3 mk = LoadMasks3(kPool3A9 + 16);
    auto row = [&](f64 a, f64 c, f64 d) {
        return Andc(Sel(Sel(Fill4(static_cast<f32>(a)), Fill4(static_cast<f32>(c)), mk.m[0]), Fill4(static_cast<f32>(d)), mk.m[1]), mk.m[2]);
    };
    {
        const f64 s = Lf(load<u32>(o + 212) + 584);
        Stvx(V, row(Muls(Lf(M + 32), s), Muls(Lf(M + 36), s), Muls(Lf(M + 40), s)));
    }
    call(ctx, b27fn::Hk_Activate, {b});
    const u32 bi = b + 224;
    VCall(ctx, bi, 68, {bi, V});

    // Angular velocity from four steps of the global generator.
    const u32 s0 = load<u32>(b27fn::Rng + 8);
    const f64 sign = (s0 & 1) == 1 ? one : Lf(kPool3A9 + 64);
    const u32 d2 = load<u32>(o + 212);
    const u32 x1 = RngMix27(s0);
    const u32 x2 = RngMix27(std::rotl(x1, 16));
    const u32 x3 = RngMix27(std::rotl(x2, 16));
    const u32 x4 = RngMix27(std::rotl(x3, 16));
    const f64 sc = Lf(kPool3A9 + 68);
    auto rnd = [](u32 v) { return S(static_cast<f64>(v & 0xFFFF)); };
    const f64 lo0 = Lf(d2 + 544), lo1 = Lf(d2 + 548), lo2 = Lf(d2 + 552);
    const f64 w0 = Muls(Fmadds(Subs(Lf(d2 + 560), lo0), Muls(rnd(x1), sc), lo0), sign);
    const f64 w1 = Muls(Fmadds(Subs(Lf(d2 + 564), lo1), Muls(rnd(x2), sc), lo1), sign);
    store<u32>(b27fn::Rng + 8, std::rotl(x4, 16));
    store<u64>(b27fn::Rng, x4 & 0xFFFF);
    const f64 w2 = Muls(Fmadds(Subs(Lf(d2 + 568), lo2), Muls(rnd(x3), sc), lo2), sign);
    ctx.fpr[1] = w2;  // leftover f1
    Stvx(V, row(w0, w1, w2));
    call(ctx, b27fn::Hk_Activate, {b});
    VCall(ctx, bi, 72, {bi, V});
    call(ctx, b27fn::HkBody_AddListener_0091A82C, {b, o + 56});
    call(ctx, b27fn::HkBody_AddListener_0091A5F8, {b, o + 60});

    // Push {b, x}.
    u32 n = load<u16>(o + 74);
    if (const u32 cap = load<u16>(o + 72); n == cap) {
        call(ctx, b27fn::Array_Reserve, {o + 72, (cap << 1) & 0xFFFE});
        n = load<u16>(o + 74);
    }
    const u32 data = load<u32>(o + 80);
    store<u32>(data + ((n << 3) & 0x7FFF8u), b);
    store<u32>(data + ((static_cast<u32>(load<u16>(o + 74)) << 3) & 0xFFFFFFF8u) + 4, x);
    const u32 n1 = load<u16>(o + 74) + 1;
    u32 hi = load<u16>(o + 76);
    const u32 fl = load<u16>(o + 78);
    store<u16>(o + 74, static_cast<u16>(n1));
    const u32 idx = load<u32>(o + 400), w384 = load<u32>(o + 384);
    if (!(static_cast<s32>(hi) > static_cast<s32>(n1 & 0xFFFF))) hi = n1;
    store<u16>(o + 76, static_cast<u16>(hi));
    store<u16>(o + 78, static_cast<u16>(fl | 1));
    call(ctx, b27fn::Hk_AddProperty, {b, kPropIndex, static_cast<u64>(static_cast<s64>(static_cast<s32>(idx)))});
    call(ctx, b27fn::Hk_AddProperty, {b, kPropHeldFlag, (w384 >> 27) & 1});
    const u32 c = load<u32>(ent + 76) + 1;
    store<u32>(ent + 76, c);
    ret(ctx, c);
}
