// Game code, large-function sweep part 28: a ledge-grab state turning and
// placing the owner's frame on the ledge found by Ledge_Probe.
#include "../sly4.h"
#include "big28.h"

using namespace sly4;
using namespace sly4::big28;

namespace {
constexpr f32 kHalf = 0.5f;     // .text pool 0x002CEF1C +0
constexpr f32 kPi = 3.1415927f; // pool +4
constexpr f32 kOne = 1.0f;      // pool +8
constexpr f32 kGap = 0.1f;      // pool +12
constexpr u32 Ledge_Probe = 0x002CCFF0;       // bool (Obj*, const f32 pos[3], u8 high, f32 pt[3], f32 n[3], u8* grab, u8* kind)
constexpr u32 Vec_RotateToward = 0x001007D0;  // (f32 out[3], const f32 from[3], const f32 to[3]; f1 max angle)
constexpr u32 ZeroVec = 0x01164B40;           // the global zero vector
constexpr u32 UpVec = 0x01164B60;             // the global up vector
constexpr u32 AxisZ = 0x01164B70;             // the global vector after UpVec
constexpr u32 IdentityMat = 0x01164BD0;       // the global identity matrix (4 rows of 16)
inline void Cp(u32 dst, u32 src) { store<u32>(dst, load<u32>(src)); }  // lfs / stfs pair
}  // namespace

// u32 f(State* o; f32 dt) (called by f_00160A80 .. f_00164808). Needs the
// owner w = o->+8 with w->vfn[32](w, 0) == 7; m = w->vfn[64]()->vfn[8]()
// is the frame object written. Ledge_Probe(o, &w's matrix (+96) row 3, 0,
// &pt (zero vector), &n (AxisZ), o+345, o+346); 0 without a ledge (f1 =
// pt.y - o->+68 * 0.5). m's rows (+32 / +48 / +64 / +80) start as the
// identity; row 2 = Vec_RotateToward(o's row 2 (+160..), -n, dt * pi);
// row 0 = normalised UpVec x row 2, row 1 = row 2 x row 0; row 3 = w's
// position (+144..) moved along n to n . (pt - pos) + o->+72 + 0.1 (with
// pt.y lowered by half o->+68). o's rows +128 / +144 / +160 / +176 = m's.
// Returns 1 (f1 = m's +88).
DECOMP(002CEF30, Ledge_AlignToProbe_002CEF30) {
    const u32 o = arg<u32>(ctx, 0);
    const f64 dt = ctx.fpr[1];
    const u32 w = load<u32>(o + 8);
    if (w == 0) {
        ret(ctx, 0u);
        return;
    }
    if (static_cast<u32>(VCall(ctx, w, 32, {w, 0})) != 7) {
        ret(ctx, 0u);
        return;
    }
    const u32 x = static_cast<u32>(VCall(ctx, w, 64, {w}));
    const u32 m = static_cast<u32>(VCall(ctx, x, 8, {x}));
    Stack frame(ctx, 368);
    const u32 sp = frame.addr();
    const u32 PT = sp + 144, N = sp + 160, POS = sp + 224;
    const f64 hgt = Lf(o + 68), off = Lf(o + 72);  // f29 / f30
    for (u32 r = 0; r < 4; ++r)
        for (u32 k = 0; k < 3; ++k) Cp(sp + 176 + 16 * r + 4 * k, o + 128 + 16 * r + 4 * k);
    for (u32 k = 0; k < 3; ++k) Cp(POS + 4 * k, w + 144 + 4 * k);
    for (u32 k = 0; k < 3; ++k) Cp(PT + 4 * k, ZeroVec + 4 * k);
    for (u32 k = 0; k < 3; ++k) Cp(N + 4 * k, AxisZ + 4 * k);
    const u32 w2 = load<u32>(o + 8);
    VCall(ctx, w2, 64, {w2});  // result unused
    const u32 mat = w2 + 96;
    for (u32 r = 0; r < 4; ++r)
        for (u32 k = 0; k < 3; ++k) Cp(sp + 240 + 16 * r + 4 * k, mat + 16 * r + 4 * k);
    SetF1(ctx, mat + 56);
    const u32 found = static_cast<u32>(call(ctx, Ledge_Probe, {o, sp + 288, 0, PT, N, o + 345, o + 346}));
    const f64 py = S(-std::fma(hgt, static_cast<f64>(kHalf), -Lf(PT + 4)));  // fnmsubs
    ctx.fpr[1] = py;
    if (found == 0) {
        ret(ctx, 0u);
        return;
    }
    Sf(PT + 4, py);
    for (u32 r = 0; r < 4; ++r)
        for (u32 k = 0; k < 3; ++k) Cp(m + 32 + 16 * r + 4 * k, IdentityMat + 16 * r + 4 * k);
    const u32 R0 = m + 32, R1 = m + 48, R2 = m + 64, R3 = m + 80;
    Sf(sp + 128, -Lf(N)), Sf(sp + 132, -Lf(N + 4)), Sf(sp + 136, -Lf(N + 8));
    callf(ctx, Vec_RotateToward, {sp + 112, sp + 208, sp + 128}, {Muls(dt, kPi)});
    // Row 2: the turned direction; row 0: UpVec x row 2, normalised.
    const f64 d1 = Lf(sp + 116), d2 = Lf(sp + 120);
    for (u32 k = 0; k < 3; ++k) Cp(R2 + 4 * k, sp + 112 + 4 * k);
    {
        const f64 a = Fmsubs(Lf(UpVec + 4), d2, Muls(Lf(UpVec + 8), d1));
        Sf(R0, a);
        const f64 b = Fmsubs(Lf(UpVec + 8), Lf(R2), Muls(Lf(UpVec), Lf(R2 + 8)));
        Sf(R0 + 4, b);
        const f64 c = Fmsubs(Lf(UpVec), Lf(R2 + 4), Muls(Lf(UpVec + 4), Lf(R2)));
        const f64 s = Divs(kOne, Sqrts(Fmadds(c, c, Fmadds(a, a, Muls(b, b)))));
        const f64 ax = Muls(a, s), ay = Muls(b, s), az = Muls(c, s);
        Sf(R0, ax), Sf(R0 + 4, ay), Sf(R0 + 8, az);
        // Row 1: row 2 x row 0.
        Sf(R1, Fmsubs(Lf(R2 + 4), az, Muls(Lf(R2 + 8), ay)));
        Sf(R1 + 4, Fmsubs(Lf(R2 + 8), Lf(R0), Muls(Lf(R2), Lf(R0 + 8))));
        Sf(R1 + 8, Fmsubs(Lf(R2), Lf(R0 + 4), Muls(Lf(R2 + 4), Lf(R0))));
    }
    // Row 3: the position moved along n.
    {
        const f64 ex = Subs(Lf(PT), Lf(POS)), ez = Subs(Lf(PT + 8), Lf(POS + 8)), ey = Subs(Lf(PT + 4), Lf(POS + 4));
        Cp(R3, POS);
        f64 t = Fmadds(Lf(N), ex, off);
        Cp(R3 + 4, POS + 4);
        Cp(R3 + 8, POS + 8);
        t = Fmadds(Lf(N + 8), ez, t);
        t = Fmadds(Lf(N + 4), ey, t);
        t = Adds(t, kGap);
        const f64 my = Muls(t, Lf(N + 4)), mz = Muls(t, Lf(N + 8)), mx = Muls(t, Lf(N));
        const f64 qy = Adds(Lf(POS + 4), my), qz = Adds(Lf(POS + 8), mz), qx = Adds(Lf(POS), mx);
        Sf(sp + 116, my), Sf(sp + 120, mz), Sf(sp + 112, mx);
        Sf(sp + 132, qy), Sf(sp + 136, qz), Sf(sp + 128, qx);
        Sf(R3, qx);
        Cp(R3 + 4, sp + 132);
        Cp(R3 + 8, sp + 136);
    }
    for (u32 r = 0; r < 4; ++r)
        for (u32 k = 0; k < 3; ++k) Cp(o + 128 + 16 * r + 4 * k, m + 32 + 16 * r + 4 * k);
    SetF1(ctx, m + 88);
    ret(ctx, 1u);
}
