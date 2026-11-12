// Game code, large-function sweep part 28: a ledge-grab state turning and
// placing the owner's frame on the ledge found by Ledge_Probe.
#include "big28.h"

namespace sly4::obj {

namespace {
constexpr f32 kHalf = 0.5f;     // .text pool 0x002CEF1C +0
constexpr f32 kPi = 3.1415927f; // pool +4
constexpr f32 kOne = 1.0f;      // pool +8
constexpr f32 kGap = 0.1f;      // pool +12
constexpr u32 Ledge_Probe = 0x002CCFF0;       // bool (Obj*, const f32 pos[3], u8 high, f32 pt[3], f32 n[3], u8* grab, u8* kind)
constexpr u32 Vec_RotateToward = 0x001007D0;  // (f32 out[3], const f32 from[3], const f32 to[3]; f1 max angle)
constexpr u32 UpVec = 0x01164B60;             // the global up vector
constexpr u32 AxisZ = 0x01164B70;             // the global vector after UpVec ({0, 0, 1})
constexpr u32 IdentityMat = 0x01164BD0;       // the global identity matrix

// The owner: +96 its matrix (row 3 the position).
struct LedgeOwner {
    pad<96> _0;
    Mtx44 m;  // +96
};
// The frame object written: +32 its matrix.
struct LedgeFrame {
    pad<32> _0;
    Mtx44 m;  // +32
};
struct LedgeState {
    pad<8> _0;
    be_u32 owner;   // +8: LedgeOwner*
    pad<56> _12;
    be_f32 height;  // +68
    be_f32 off;     // +72
    pad<52> _76;
    Mtx44 m;        // +128
    pad<153> _192;
    u8 grab;        // +345
    u8 kind;        // +346
};
static_assert(offsetof(LedgeState, m) == 128 && offsetof(LedgeState, grab) == 345);
// The original's locals (sp offsets - 112): the turned direction, -n, the
// probe point and normal, a copy of o's rows (row 3 replaced by the
// owner's position), a copy of the owner's matrix (row 3 the probe's
// position).
struct LedgeLocals {
    Vec4 dir;      // sp+112
    Vec4 negn;     // sp+128
    Vec4 pt;       // sp+144
    Vec4 n;        // sp+160
    Mtx44 rows;    // sp+176 (rows.r[3] = the position, sp+224)
    Mtx44 wm;      // sp+240 (wm.r[3] = sp+288)
};
static_assert(offsetof(LedgeLocals, rows) == 64 && offsetof(LedgeLocals, wm) == 128);

// lfs / stfs copies of the x / y / z words.
void Cp3(Vec4& d, const Vec4& s) {
    set_bits(d.x, bits(s.x));
    set_bits(d.y, bits(s.y));
    set_bits(d.z, bits(s.z));
}
void Cp3(Vec4& d, const Vec3& s) {
    set_bits(d.x, bits(s.x));
    set_bits(d.y, bits(s.y));
    set_bits(d.z, bits(s.z));
}
void Cp4x3(Mtx44& d, const Mtx44& s) {
    for (u32 r = 0; r < 4; ++r) Cp3(d.r[r], s.r[r]);
}
}  // namespace

// u32 f(State* o; f32 dt) (called by f_00160A80 .. f_00164808). Needs the
// owner w = o->+8 with w->vfn[32](w, 0) == 7; m = w->vfn[64]()->vfn[8]()
// is the frame object written. Ledge_Probe(o, &w's matrix (+96) row 3, 0,
// &pt (zero vector), &n (AxisZ), o+345, o+346); 0 without a ledge. m's rows (+32 / +48 / +64 / +80) start as the
// identity; row 2 = Vec_RotateToward(o's row 2 (+160..), -n, dt * pi);
// row 0 = normalised UpVec x row 2, row 1 = row 2 x row 0; row 3 = w's
// position (+144..) moved along n to n . (pt - pos) + o->+72 + 0.1 (with
// pt.y lowered by half o->+68). o's rows +128 / +144 / +160 / +176 = m's.
// Returns 1. Callers read r3, not f1 (r3.sh).
u32 Ledge_AlignToProbe_002CEF30(LedgeState* o, f32 dt) {
    const u32 w = o->owner;
    if (w == 0) return 0;
    if (vcall<u32>(host<u8>(w), 32, 0u) != 7) return 0;
    const u32 x = vcall<u32>(host<u8>(w), 64);
    LedgeFrame& fr = gm<LedgeFrame>(vcall<u32>(host<u8>(x), 8));
    GuestFrame frame(368);
    LedgeLocals& L = *frame.as<LedgeLocals>(112);
    const f64 hgt = lf(o->height), off = lf(o->off);  // f29 / f30
    Cp4x3(L.rows, o->m);
    Vec4& pos = L.rows.r[3];
    Cp3(pos, gm<LedgeOwner>(w).m.r[3]);
    Cp3(L.pt, global<Vec3>(kZeroVec));
    Cp3(L.n, global<Vec3>(AxisZ));
    const u32 w2 = o->owner;
    vcall(host<u8>(w2), 64);  // result unused
    Cp4x3(L.wm, gm<LedgeOwner>(w2).m);
    const u32 found = call<u32>(Ledge_Probe, o, &L.wm.r[3], 0u, &L.pt, &L.n, &o->grab, &o->kind);
    const f64 py = Fnmsubs(hgt, static_cast<f64>(kHalf), lf(L.pt.y));
    if (found == 0) return 0;
    L.pt.y = static_cast<f32>(py);
    Cp4x3(fr.m, global<Mtx44>(IdentityMat));
    Vec4 &R0 = fr.m.r[0], &R1 = fr.m.r[1], &R2 = fr.m.r[2], &R3 = fr.m.r[3];
    L.negn.x = static_cast<f32>(-lf(L.n.x)), L.negn.y = static_cast<f32>(-lf(L.n.y)), L.negn.z = static_cast<f32>(-lf(L.n.z));
    call(Vec_RotateToward, &L.dir, &L.rows.r[2], &L.negn, Muls(dt, kPi));
    // Row 2: the turned direction; row 0: UpVec x row 2, normalised.
    const Vec3& up = global<Vec3>(UpVec);
    const f64 d1 = lf(L.dir.y), d2 = lf(L.dir.z);
    Cp3(R2, L.dir);
    {
        const f64 a = Fmsubs(lf(up.y), d2, Muls(lf(up.z), d1));
        const f64 b = Fmsubs(lf(up.z), lf(R2.x), Muls(lf(up.x), lf(R2.z)));
        const f64 c = Fmsubs(lf(up.x), lf(R2.y), Muls(lf(up.y), lf(R2.x)));
        const f64 s = Divs(kOne, Sqrts(Fmadds(c, c, Fmadds(a, a, Muls(b, b)))));
        const f64 ax = Muls(a, s), ay = Muls(b, s), az = Muls(c, s);
        R0.x = static_cast<f32>(ax), R0.y = static_cast<f32>(ay), R0.z = static_cast<f32>(az);
        // Row 1: row 2 x row 0.
        R1.x = static_cast<f32>(Fmsubs(lf(R2.y), az, Muls(lf(R2.z), ay)));
        R1.y = static_cast<f32>(Fmsubs(lf(R2.z), lf(R0.x), Muls(lf(R2.x), lf(R0.z))));
        R1.z = static_cast<f32>(Fmsubs(lf(R2.x), lf(R0.y), Muls(lf(R2.y), lf(R0.x))));
    }
    // Row 3: the position moved along n.
    {
        const f64 ex = Subs(lf(L.pt.x), lf(pos.x)), ez = Subs(lf(L.pt.z), lf(pos.z)), ey = Subs(lf(L.pt.y), lf(pos.y));
        f64 t = Fmadds(lf(L.n.x), ex, off);
        t = Fmadds(lf(L.n.z), ez, t);
        t = Fmadds(lf(L.n.y), ey, t);
        t = Adds(t, kGap);
        const f64 my = Muls(t, lf(L.n.y)), mz = Muls(t, lf(L.n.z)), mx = Muls(t, lf(L.n.x));
        const f64 qy = Adds(lf(pos.y), my), qz = Adds(lf(pos.z), mz), qx = Adds(lf(pos.x), mx);
        R3.x = static_cast<f32>(qx);
        R3.y = static_cast<f32>(qy);
        R3.z = static_cast<f32>(qz);
    }
    Cp4x3(o->m, fr.m);
    return 1;
}

}  // namespace sly4::obj

NATIVE(002CEF30, sly4::obj::Ledge_AlignToProbe_002CEF30)
