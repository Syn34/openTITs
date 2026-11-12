// Game code, large-function sweep part 31: the steepest ground slope under
// a point, from five Havok ray casts (the point and four neighbours).
#include "../sly4.h"
#include "big31.h"

using namespace sly4;
using namespace sly4::big31;

namespace {
constexpr u32 kSlopeK = 0x000F0230;        // .text: f32 2.0 (offset), 5.0 (ray length); lane masks y / z / w at +16 / +32 / +48;
                                      // f32 1.0 at +64, f64 1.0 at +72, f32 -1.0 at +80, f64 -1.0 at +88, f32 57.29578 at +96
constexpr u32 kOffsReady = 0x01032200;  // u8: kOffs built
constexpr u32 kOffs = 0x01032210;       // f32[5][4]: the ray offsets (copies of the global vectors below)
constexpr u32 kDirReady = 0x01032260;   // u8: kDir built
constexpr u32 kDir = 0x01032270;        // f32[3]: the ray direction (a copy of 0x01164B90)
constexpr u32 kOffsSrc[5] = {0x01164B40, 0x01164B50, 0x01164B70, 0x01164B80, 0x01164BA0};
constexpr u32 kDirSrc = 0x01164B90;
constexpr u32 kGame = 0x00F65CA0;       // Game* (+0x1281D4 the physics holder, +4 its world)
constexpr u32 kVtRayCol = 0x00F1DE80;   // all-hits ray collector vtable (80-byte hits, 8 inline)
constexpr u32 kVtRayColBase = 0x00B0EF40;  // the collector's base vtable (its state after use)
constexpr u32 kHkHeap = 0x011DF31C;     // Havok heap allocator (vfn[20] (p, bytes) frees)
constexpr u32 kPropKind = 0xA734004D;   // shape property: the shape's kind
constexpr u32 kKindOwned = 0x7A30D7E3;  // that kind: the shape belongs to the object in kPropOwner
constexpr u32 kPropOwner = 0x91C7CF5B;  // shape property: the owning object
constexpr u32 HkWorld_CastRay92E510 = 0x0092E510;  // (hkpWorld*, const RayInput*, Collector*)
constexpr u32 HkAllHits_Sort887E28 = 0x00887E28;   // (Collector*)
constexpr u32 PtrVector_Construct = 0x005516E0;    // (vec)
constexpr u32 PtrVector_Init = 0x00551598;         // (vec, u32 n, 0, 0)
constexpr u32 PtrVector_Reserve = 0x00551718;      // (vec, u32 n)
constexpr u32 PtrVector_Clear = 0x0054EC1C;        // (vec)
constexpr u32 Nop = 0x0054EC18;                    // (vec)
constexpr u32 Acosf = 0x005E649C;                  // f1 -> f1

// Three words from src to dst (lfs / stfs pairs).
void Copy3(u32 dst, u32 src) {
    for (u32 k = 0; k < 12; k += 4) store<u32>(dst + k, load<u32>(src + k));
}

// The low word of property `key` in the shape's list (+124 the 16-byte
// entries {key, -, u64 value}, +128 the s32 count), 0 when absent.
u32 Prop(u32 props, u32 key) {
    for (s32 j = 0; j < load<s32>(props + 128); ++j) {
        const u32 e = load<u32>(props + 124) + 16 * static_cast<u32>(j);
        if (load<u32>(e) == key) return load<u32>(e + 12);
    }
    return 0;
}

// The four fsel / frsp steps clamping x to [-1, 1] (as the original
// repeats them, NaN -> 1.0).
f64 Clamp1(f64 x, f64 one, f64 d_one, f64 m_one, f64 d_m_one) {
    x = S(Fsel31(Subs(one, x), x, d_one));
    x = S(Fsel31(Subs(x, m_one), x, d_m_one));
    x = S(Fsel31(Adds(x, one), x, d_m_one));
    return S(Fsel31(Subs(x, one), d_one, x));
}
}  // namespace

// void f(-, f32* out, const f32 p[3], const f32 up[3]) (called by
// Obj_SyncComps_000F0B48; r3 is not read). For each of the five offsets
// (built once: 0x01164B40 / B50 / B70 / B80 / BA0) a ray from
// p + 2 * offset along kDir (built once from 0x01164B90) * 5 is cast
// (RayInput at sp + 112: from, to, +32 / +36 / +40 = 0; the collector at
// sp + 160 with eight inline 80-byte hits at sp + 192) and the hits sorted.
// The first hit whose shape (+68) type (+28 & 0x1F) is 1..4 or 28 and that
// is not excluded is kept: a shape with byte +24 == 2 (an owned shape) is
// excluded, as is one with +24 == 1 whose kPropKind is kKindOwned and whose
// kPropOwner is non-zero. Its point (from + (to - from) * fraction) is
// pushed into a 16-byte PtrVector at sp + 832. A heap hit array is freed
// through kHkHeap->vfn[20] (entry in r6, OPD in r7). With all five points,
// for the four triangles (centre, q[j - 1], q[j]) (j = 1..4, q[0] taken
// as q[4]) the dot of the unit normal (-(a x b)) with up is computed and
// clamped to [-1, 1] (acosf called on each, the result unused); *out =
// acosf(the clamped minimum) * 57.29578 (degrees). Otherwise *out is left
// alone. The vector is then cleared (PtrVector_Clear, Nop).
DECOMP(000F0298, Obj_GroundSlope_000F0298) {
    const u32 out = arg<u32>(ctx, 1), p = arg<u32>(ctx, 2), up = arg<u32>(ctx, 3);
    Stack frame(ctx, 768);
    const u32 sp = frame.addr() - 112;  // the original's frame (offsets below are its sp offsets)
    const u32 ray = sp + 112, coll = sp + 160, hits = sp + 192, vec = sp + 832;
    store<u8>(sp + 144, 0);
    store<u32>(sp + 148, 0);
    store<u32>(sp + 152, 0);
    if (load<u8>(kOffsReady) == 0) {
        for (u32 k = 0; k < 5; ++k) Copy3(kOffs + 16 * k, kOffsSrc[k]);
        store<u8>(kOffsReady, 1);
    }
    call(ctx, PtrVector_Construct, {vec});
    store<u16>(vec + 6, 16);
    call(ctx, PtrVector_Init, {vec, 5, 0, 0});
    u32 found = 0;
    for (u32 k = 0; k < 5; ++k) {
        const f64 two = Lf(kSlopeK);
        const u32 o = kOffs + 16 * k;
        f64 x = Fmadds(Lf(o), two, Lf(p)), y = Fmadds(Lf(o + 4), two, Lf(p + 4)), z = Fmadds(Lf(o + 8), two, Lf(p + 8));
        if (load<u8>(kDirReady) == 0) {
            Copy3(kDir, kDirSrc);
            store<u8>(kDirReady, 1);
        }
        const f64 len = Lf(kSlopeK + 4);
        const f64 ex = Fmadds(len, Lf(kDir), x), ey = Fmadds(len, Lf(kDir + 4), y), ez = Fmadds(len, Lf(kDir + 8), z);
        const auto mk = LoadMasks3(kSlopeK + 16);
        Stvx(ray, RowSel(Fill4(static_cast<f32>(x)), Fill4(static_cast<f32>(y)), Fill4(static_cast<f32>(z)), mk));
        Stvx(ray + 16, RowSel(Fill4(static_cast<f32>(ex)), Fill4(static_cast<f32>(ey)), Fill4(static_cast<f32>(ez)), mk));
        store<u32>(sp + 148, 0);
        const u32 world = load<u32>(load<u32>(load<u32>(kGame) + 0x1281D4) + 4);
        store<u32>(coll + 4, 0x3F800000u);
        store<u32>(coll, kVtRayCol);
        store<u32>(coll + 16, hits);
        store<u32>(coll + 20, 0);
        store<u32>(coll + 24, 0x80000008u);
        for (u32 j = 0; j < 8; ++j) {
            const u32 h = hits + 80 * j;
            store<u32>(h + 16, 0x3F800000u);
            store<u32>(h + 20, 0xFFFFFFFFu);
            store<u32>(h + 64, 0);
            store<u32>(h + 32, 0xFFFFFFFFu);
            store<u32>(h + 68, 0);
        }
        store<u32>(coll + 20, 0);
        store<u32>(coll + 4, 0x3F800000u);
        call(ctx, HkWorld_CastRay92E510, {world, ray, coll});
        call(ctx, HkAllHits_Sort887E28, {coll});
        const u32 hp = load<u32>(coll + 16);
        u32 cnt = load<u16>(vec + 8);
        const s32 n = load<s32>(coll + 20);
        const u32 cap = load<u16>(vec + 4);
        for (s32 i = 0; i < n; ++i) {
            const u32 hit = hp + 80 * static_cast<u32>(i);
            const u32 sh = load<u32>(hit + 68);
            const u32 ty = load<u32>(sh + 28) & 0x1F;
            if (!((ty >= 1 && ty <= 4) || ty == 28)) continue;
            const s8 kind = static_cast<s8>(load<u8>(sh + 24));
            const u32 rel = sh + static_cast<u32>(static_cast<s32>(static_cast<s8>(load<u8>(sh + 16))));
            const u32 props = kind == 1 ? rel : 0;
            const u32 owned = kind == 2 ? rel : 0;
            if (props != 0 && Prop(props, kPropKind) == kKindOwned) {
                const u32 owner = load<s32>(props + 128) > 0 ? Prop(props, kPropOwner) : 0;
                if (owner != 0) continue;
            }
            if (owned != 0) continue;
            const f64 t = Lf(hit + 16);
            x = Fmadds(Subs(ex, x), t, x);
            y = Fmadds(Subs(ey, y), t, y);
            z = Fmadds(Subs(ez, z), t, z);
            if (cnt == cap) {
                call(ctx, PtrVector_Reserve, {vec, (cap << 1) & 0x1FFFEu});
                cnt = load<u16>(vec + 8);
            }
            const u32 e = load<u32>(vec) + ((cnt << 4) & 0xFFFF0u);
            store<u32>(e, Fb(x));
            store<u32>(e + 4, Fb(y));
            store<u32>(e + 8, Fb(z));
            const u32 c1 = load<u16>(vec + 8) + 1u;
            const u32 mx = load<u16>(vec + 10);
            store<u16>(vec + 8, static_cast<u16>(c1));
            store<u16>(vec + 10, static_cast<u16>(static_cast<s32>(mx) > static_cast<s32>(c1 & 0xFFFF) ? mx : c1));
            ++found;
            break;
        }
        const u32 cw = load<u32>(coll + 24);
        store<u32>(coll + 20, 0);
        if ((cw & 0x80000000u) == 0) {
            const u32 hpn = load<u32>(coll + 16);
            store<u32>(coll, kVtRayCol);
            const u32 opd = load<u32>(load<u32>(kHkHeap) + 20);
            const u64 size = static_cast<u64>(static_cast<s64>(static_cast<s32>(80u * (cw & 0x3FFFFFFFu))));
            call_ptr(ctx, opd, {kHkHeap, hpn, size, load<u32>(opd), opd});  // entry in r6, OPD in r7
        }
        store<u32>(coll + 16, 0);
        store<u32>(coll + 24, 0x80000000u);
        store<u32>(coll, kVtRayColBase);
    }
    if (found == 5) {
        const f64 one = Lf(kSlopeK + 64), d_one = Lfd31(kSlopeK + 72), m_one = Lf(kSlopeK + 80), d_m_one = Lfd31(kSlopeK + 88);
        f64 mn = one;
        u32 prev = 4;
        for (u32 i = 1; i < 5; ++i) {
            const u32 d = load<u32>(vec);
            const u32 a = d + (prev << 4), b = d + 16 * i;
            f64 ay = Subs(Lf(a + 4), Lf(d + 4)), by = Subs(Lf(b + 4), Lf(d + 4));
            f64 ax = Subs(Lf(a), Lf(d)), bx = Subs(Lf(b), Lf(d));
            f64 az = Subs(Lf(a + 8), Lf(d + 8)), bz = Subs(Lf(b + 8), Lf(d + 8));
            f64 la = Muls(ay, ay), lb = Muls(by, by);
            la = Fmadds(ax, ax, la);
            lb = Fmadds(bx, bx, lb);
            la = Fmadds(az, az, la);
            lb = Fmadds(bz, bz, lb);
            const f64 ia = Divs(one, Sqrts(la)), ib = Divs(one, Sqrts(lb));
            az = Muls(az, ia);
            ax = Muls(ax, ia);
            ay = Muls(ay, ia);
            bx = Muls(bx, ib);
            bz = Muls(bz, ib);
            by = Muls(by, ib);
            const f64 t7 = Muls(az, bx), t8 = Muls(ay, bz), t12 = Muls(ax, by);
            const f64 ny = Fmsubs(ax, bz, t7), nx = Fmsubs(az, by, t8), nz = Fmsubs(ay, bx, t12);
            f64 dot = Muls(ny, Lf(up + 4));
            dot = Fmadds(nx, Lf(up), dot);
            dot = Fmadds(nz, Lf(up + 8), dot);
            f64 c = S(Fsel31(Subs(one, dot), dot, d_one));
            c = S(Fsel31(Subs(c, m_one), c, d_m_one));
            mn = Fsel31(Subs(dot, mn), mn, dot);
            c = S(Fsel31(Adds(c, one), c, d_m_one));
            c = S(Fsel31(Subs(c, one), d_one, c));
            callf(ctx, Acosf, {}, {c});  // the result is not used
            prev = i;
        }
        const f64 r = callf(ctx, Acosf, {}, {Clamp1(mn, one, d_one, m_one, d_m_one)});
        store<u32>(out, Fb(Muls(r, Lf(kSlopeK + 96))));
    }
    call(ctx, PtrVector_Clear, {vec});
    call(ctx, Nop, {vec});
}
