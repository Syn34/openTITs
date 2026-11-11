// Game code, VMX sweep part 3 (see vmx3.h): Havok-side queries.
#include "../sly4.h"
#include "vmx3.h"

using namespace sly4;
using namespace sly4::vmx3;

namespace {
constexpr u32 kHkAlloc3 = 0x011DF31C;      // the Havok allocator object: vt[20] free(p, bytes)
constexpr u32 kHitCollectorVt = 0x00F1DE58;  // hit collector vtable (48-byte hits, the array's dtor resets it)
constexpr u32 kPool4A41C0 = 0x004A4180;    // f32 1e-5, then three lane masks

// The inlined hkArray<48-byte hit> destructor of a stack collector at c:
// count (+20) = 0; unless "don't free" (bit 0x80000000 of +24), the vtable
// is reset and the allocator frees +16 (capacity * 48 bytes, extsw). The
// two original call sites leave different registers after the arguments.
inline void FreeHits3(PPUContext& ctx, u32 c, bool r7size) {
    const u32 f = load<u32>(c + 24);
    if ((f & 0x80000000u) != 0) return;
    const u64 n = static_cast<u64>(static_cast<s64>(static_cast<s32>(((f & 0x3FFFFFFFu) << 4) * 3)));
    store<u32>(c + 20, 0);
    store<u32>(c, kHitCollectorVt);
    const u32 data = load<u32>(c + 16);
    const u32 opd = load<u32>(load<u32>(kHkAlloc3) + 20);
    if (r7size) call_ptr(ctx, opd, {kHkAlloc3, data, n, load<u32>(opd), n, opd});
    else call_ptr(ctx, opd, {kHkAlloc3, data, n, load<u32>(opd), opd});
}
}  // namespace

// bool f(T* o, const f32 m[4][4], const Vec3* p, f32 out[4][4]) (called by
// 000EB420): out = m's rotation rows with p as its translation; when that
// differs from m (Matrix_NearlyEqual, 1e-5), o's world (+8) is queried
// along it (0x0092E3D8(world, o + 16, {p, 0}, collector, 0)) with a stack
// hit collector (inline room for 8 48-byte hits, early-out 0x7F7FFFEE).
// With a hit, 0x008871E8(collector) then out's translation = the first
// hit's first three words and returns 1; else 0. The collector's array is
// freed either way (the inlined destructor). Returns 0 when unchanged.
DECOMP(004A41C0, Hk_SweepToPoint_004A41C0) {
    const u32 o = arg<u32>(ctx, 0), m = arg<u32>(ctx, 1), p = arg<u32>(ctx, 2), out = arg<u32>(ctx, 3);
    const u32 world = load<u32>(o + 8);
    for (u32 r = 0; r < 4; ++r)
        for (u32 k = 0; k < 3; ++k) CopyW(out + 16 * r + 4 * k, m + 16 * r + 4 * k);
    CopyW(out + 48, p);
    CopyW(out + 52, p + 4);
    CopyW(out + 56, p + 8);
    callf(ctx, v3fn::Matrix_NearlyEqual, {m, out}, {F(kPool4A41C0)});
    if (static_cast<u32>(ctx.gpr[3]) != 0) {
        ret(ctx, 0u);
        return;
    }
    Stack frame(ctx, 592);
    const u32 sp = frame.addr();
    const u32 c = sp + 144;  // the collector
    store<u32>(sp + 128, 0x34000000u);  // dead: overwritten by the zero quadword below
    store<u32>(sp + 132, 0x34000000u);
    store<u32>(c + 16, sp + 176);
    store<u32>(c, kHitCollectorVt);
    store<u32>(c + 24, 0x80000008u);
    store<u32>(c + 20, 0);
    store<u32>(c + 4, 0x7F7FFFEEu);
    Stvx(sp + 128, Zero4());
    Stvx(sp + 112, RowSelW(p, p + 4, p + 8, LoadMasks3(kPool4A41C0 + 16)));
    call(ctx, 0x0092E3D8, {world, o + 16, sp + 112, c, 0});
    if (load<s32>(c + 20) <= 0) {
        FreeHits3(ctx, c, true);
        ret(ctx, 0u);
        return;
    }
    call(ctx, 0x008871E8, {c});
    const u32 h = load<u32>(c + 16);
    CopyW(out + 48, h);
    CopyW(out + 52, h + 4);
    CopyW(out + 56, h + 8);
    SetF1Bits(ctx, load<u32>(h));  // f1 as the original leaves it (the hit's x; no caller reads it)
    FreeHits3(ctx, c, false);
    ret(ctx, 1u);
}

namespace {
constexpr u32 kPool4AB940 = 0x004AB900;  // f32 0; +16 / +32 the yzx / zxy vperm masks; +48 f32 0.5, -0.5, 2.0944
constexpr u32 kStrTtSphere = 0x00AEDCC0;  // "TtSphere"
constexpr u32 kStrEt = 0x00B21B48;        // "Et" (profiler end marker)
}  // namespace

// void f(T* o, const Sphere* s, const hkTransform* t) (called by 004ABC28):
// drag on a sphere ("TtSphere" profiler markers). p = t's translation and
// the medium's velocity v = o+60's vfn[4](p, &v) (it may move p), else 0.
// With a drag factor k (+64) > 0 the body's (+48) point velocity at p
// (linear +432 + angular +448 x (p - centre +320)) is taken off: v -= k *
// that. When |v|^2 > 0 (rsqrte refined once by Newton), the force v *
// 2.0944 * r^2 (r = s+16) is applied at p - 0.5 * r * v / |v|: added to the
// force (+16) and its torque about the centre to +32. f1 / r3 are left as
// the original leaves them (0.0 or the force scale; the marker cursor).
DECOMP(004AB940, HkBody_SphereDrag_004AB940) {
    const u32 o = arg<u32>(ctx, 0), s = arg<u32>(ctx, 1), t = arg<u32>(ctx, 2);
    const u32 K = kPool4AB940;
    Stack frame(ctx, 176);
    const u32 sp = frame.addr();
    ProfMark(ctx, kStrTtSphere);
    const f32 r = F(s + 16);
    V4 p = Lvx(t + 48);
    Stvx(sp + 112, p);
    const u32 med = load<u32>(o + 60);
    if (med != 0) {
        VCall(ctx, med, 16, {med, sp + 112, sp + 128});
        p = Lvx(sp + 112);
    } else {
        Stvx(sp + 128, Zero4());
    }
    const f32 zero = F(K);
    f64 f1 = zero;
    const f32 k = F(o + 64);
    const V4 yzx = Lvx(K + 16), zxy = Lvx(K + 32);
    if (k > zero) {
        const u32 b = load<u32>(o + 48) + 224;
        const V4 rel = Sub4(p, Lvx(b + 96));
        const V4 pv = Add4(Cross4(Lvx(b + 224), rel, yzx, zxy), Lvx(b + 208));
        Stvx(sp + 128, Madd4(pv, Fill4(-k), Lvx(sp + 128)));
    }
    const V4 v = Lvx(sp + 128);
    const V4 l2 = HSum3(Madd4(v, v, Zero4()));
    if (l2.e[0] > zero) {
        const V4 e = Rsqrte4(l2);
        const V4 half = SplatW(K + 48);
        const V4 nr = Nmsub4(Madd4(l2, half, Zero4()), Madd4(e, e, Zero4()), half);
        const V4 inv = Andc(Madd4(e, nr, e), Cmpge4(Zero4(), l2));
        const f32 off = Muls(r, F(K + 52));
        const f32 sc = Muls(Muls(r, F(K + 56)), r);
        const u32 c = load<u32>(o + 48) + 320;
        const V4 at = Add4(p, Madd4(Madd4(v, inv, Zero4()), Fill4(off), Zero4()));
        const V4 force = Madd4(v, Fill4(sc), Zero4());
        const V4 arm = Sub4(at, Lvx(c));
        Stvx(o + 16, Add4(Lvx(o + 16), force));
        Stvx(o + 32, Add4(Lvx(o + 32), Cross4(arm, force, yzx, zxy)));
        f1 = sc;
    }
    ctx.fpr[1] = f1;
    ctx.gpr[3] = ProfMark(ctx, kStrEt);
}

namespace {
constexpr u32 kMasks1B590 = 0x0001B560;  // three lane masks
constexpr u32 kCastRay3 = 0x0092E2E8;    // HkWorld_CastRay92E2E8 (world, input, output)
}  // namespace

// void f(List* l, -, u32 noRay, const Vec3* p; f32 r, f32 v) (called by
// 00092EE4): pokes the objects in range. For each of the s32 l+0 objects
// (pointers at l+592..), those with vfn[30]() set and vfn[39]() clear whose
// position (+48 ->+144) is within r of p (<=), and, unless noRay, visible
// from p (HkWorld_CastRay92E2E8 on the world's +0x1281D4 ->+4, filter 13,
// no hit +68), get vfn[43](). Then l+40 = v. The count is re-read each
// pass. f1 / r3 are left as the original leaves them (the last distance^2
// or r; the count).
DECOMP(0001B590, Obj_PokeInRange_0001B590) {
    const u32 l = arg<u32>(ctx, 0), noRay = arg<u32>(ctx, 2), p = arg<u32>(ctx, 3);
    const f64 r = ctx.fpr[1], v = ctx.fpr[2];
    Stack frame(ctx, 448);
    const u32 sp = frame.addr();
    const f32 r2 = Muls(r, r);
    const u32 world = load<u32>(load<u32>(load<u32>(kV3WorldPtr) + 0x001281D4) + 4);
    store<u8>(sp + 144, 0);
    store<u32>(sp + 152, 0);
    store<u32>(sp + 148, 13);
    const Masks3 mk = LoadMasks3(kMasks1B590);
    Stvx(sp + 112, RowSelW(p, p + 4, p + 8, mk));
    f64 f1 = r;
    s32 n = load<s32>(l);
    for (s32 k = 0; k < n; n = load<s32>(l)) {
        const u32 slot = l + 4 * static_cast<u32>(k) + 592;
        do {
            const u32 obj = load<u32>(slot);
            const u32 pos = load<u32>(obj + 48) + 144;
            const f32 x = F(pos), y = F(pos + 4), z = F(pos + 8);
            const u32 opd = load<u32>(load<u32>(obj) + 120);
            if (static_cast<u32>(call_ptr(ctx, opd, {obj, load<u32>(opd), opd, 8, pos})) == 0) break;
            const u32 o2 = load<u32>(slot);
            if (static_cast<u32>(VCall(ctx, o2, 156, {o2})) != 0) break;
            const f32 dy = Subs(y, F(p + 4)), dx = Subs(x, F(p)), dz = Subs(z, F(p + 8));
            const f32 d2 = Fmadds(dz, dz, Fmadds(dx, dx, Muls(dy, dy)));
            f1 = d2;
            if (!(d2 <= r2)) break;
            store<u32>(sp + 176, 0x3F800000u);
            store<u32>(sp + 224, 0);
            store<u32>(sp + 180, 0xFFFFFFFFu);
            store<u32>(sp + 228, 0);
            Stvx(sp + 128, RowSel(Fill4(x), Fill4(y), Fill4(z), mk));
            store<u32>(sp + 192, 0xFFFFFFFFu);
            if (noRay == 0) {
                call(ctx, kCastRay3, {world, sp + 112, sp + 160});
                if (load<u32>(sp + 228) != 0) break;
            }
            const u32 o3 = load<u32>(slot);
            VCall(ctx, o3, 172, {o3});
        } while (false);
        ++k;
    }
    store<f32>(l + 40, static_cast<f32>(v));
    ctx.fpr[1] = f1;
    ctx.gpr[3] = static_cast<u32>(n);
}

namespace {
constexpr u32 kVtProbe107320 = 0x00B13C18;  // the probe's vtable
constexpr u32 kGetBody3 = 0x00252940;       // Obj_GetBody_00252940 (x) -> body
constexpr u32 kSphereCtor3 = 0x0089FE70;    // Hk_SphereShapeCtor89FE70 (shape*; f1 radius)
constexpr u32 kPhantomCtor3 = 0x0092A718;   // Hk_PhantomCtor_0092A718 (phantom*, shape, const hkTransform*, filter)
constexpr u32 kHkRelease3 = 0x007D4EA8;     // AnimSkel_Release7D4EA8 (hkReferencedObject*)
constexpr u32 kBodyAdd3 = 0x00931D10;       // Body_Add931D10 (world, body)
constexpr u32 kIdXform3 = 0x00B2C360;       // hkTransform rotation rows (3 quadwords)
}  // namespace

// void f(Probe* o, u32 a, Obj* b) (called by 002C5780): probe constructor.
// vtable 0x00B13C18, +4 = a, +64 = b, the def vectors 0x01033250 /
// 0x010332B0 at +16 / +32 and the word 0x010333EC at +48, bytes +8 / +68 /
// +80 and words +72 / +76 cleared. The collision group is b's body's
// (vfn[16] then Obj_GetBody_00252940, asked three times; bit 3 of b+192
// needed) +44 high half, else 0. A sphere shape (HkAlloc(32), radius
// 0x01033438) in a phantom (HkAlloc(352), identity transform, filter group
// << 16 | 0x10) becomes +76; the shape is released and the phantom added to
// the world (+0x1281D4 ->+4). HkAlloc's size store hits a failed (null)
// allocation too, as in the original. f1 is left as the original leaves it.
DECOMP(00107320, Probe_Construct_00107320) {
    const u32 o = arg<u32>(ctx, 0), a = arg<u32>(ctx, 1), b = arg<u32>(ctx, 2);
    store<u32>(o + 4, a);
    store<u8>(o + 8, 0);
    CopyW(o + 16, 0x01033250);
    CopyW(o + 20, 0x01033254);
    CopyW(o + 24, 0x01033258);
    store<u32>(o, kVtProbe107320);
    CopyW(o + 32, 0x010332B0);
    CopyW(o + 36, 0x010332B4);
    store<u32>(o + 64, b);
    CopyW(o + 40, 0x010332B8);
    store<u8>(o + 68, 0);
    store<u32>(o + 76, 0);
    CopyW(o + 48, 0x010333EC);
    store<u32>(o + 72, 0);
    store<u8>(o + 80, 0);
    const u32 world = load<u32>(load<u32>(load<u32>(kV3WorldPtr) + 0x001281D4) + 4);
    u32 grp = 0;
    if (b != 0 && (load<u32>(b + 192) & 0x8u) != 0 && static_cast<u32>(VCall(ctx, b, 64, {b})) != 0) {
        const u32 x = static_cast<u32>(VCall(ctx, b, 64, {b}));
        if (static_cast<u32>(call(ctx, kGetBody3, {x})) != 0) {
            const u32 y = static_cast<u32>(VCall(ctx, b, 64, {b}));
            grp = load<u32>(static_cast<u32>(call(ctx, kGetBody3, {y})) + 44) >> 16;
        }
    }
    Stack frame(ctx, 224);
    const u32 sp = frame.addr();
    for (u32 k = 0; k < 3; ++k) Stvx(sp + 112 + 16 * k, Lvx(kIdXform3 + 16 * k));
    const u32 filt = (grp << 16) | 0x10;
    Stvx(sp + 160, Zero4());
    const u32 sh = HkAlloc(ctx, 32);
    u32 shape = 0;
    u32 f1w = load<u32>(0x010333EC);
    if (sh != 0) {
        f1w = load<u32>(0x01033438);
        callf(ctx, kSphereCtor3, {sh}, {std::bit_cast<f64>(LfsBits(f1w))});
        shape = sh;
    }
    const u32 ph = HkAlloc(ctx, 352);
    u32 r = 0;
    if (ph != 0) {
        call(ctx, kPhantomCtor3, {ph, shape, sp + 112, filt});
        r = ph;
    }
    store<u32>(o + 76, r);
    call(ctx, kHkRelease3, {shape});
    call(ctx, kBodyAdd3, {world, load<u32>(o + 76)});
    SetF1Bits(ctx, f1w);
}

namespace {
constexpr u32 kPool2FFD68 = 0x002FFD20;  // f32 2.0, 0.0, -20.0; +16 three lane masks; +64 f32 1e-5
}  // namespace

// void f(T* o) (called by 00300078): tracks the player's ground point. The
// world's player (+4208 +8) with bit 3 of +192: its position (+144); when
// vfn[16]() is set, cast from 2 above it to 18 below (HkWorld_CastRay92E2E8,
// filter 13) and take the hit point. Unless o's point (+480) is the zero
// vector (bitwise), its own y is kept. The point is stored only when it
// moved by 1e-5 or more on some axis. Without a player: the zero vector.
// f1 / r3 are left as the original leaves them.
DECOMP(002FFD68, Obj_TrackGroundPoint_002FFD68) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 K = kPool2FFD68;
    const u32 p = load<u32>(load<u32>(kV3WorldPtr) + 4208 + 8);
    if (p == 0 || (load<u32>(p + 192) & 0x8u) == 0) {
        CopyW(o + 480, kV3Zero3);
        CopyW(o + 484, kV3Zero3 + 4);
        CopyW(o + 488, kV3Zero3 + 8);
        SetF1Bits(ctx, load<u32>(kV3Zero3 + 8));
        ctx.gpr[3] = 0x01160000u;
        return;
    }
    Stack frame(ctx, 320);
    const u32 sp = frame.addr();
    f32 x = F(p + 144), y = F(p + 148), z = F(p + 152);
    if (static_cast<u32>(VCall(ctx, p, 64, {p})) != 0) {
        y = Adds(y, F(K));
        const f32 k1 = F(K + 4), k2 = F(K + 8);
        const f32 x1 = Adds(x, k1), y1 = Adds(y, k2), z1 = Adds(z, k1);
        store<u8>(sp + 144, 0);
        store<u32>(sp + 148, 0);
        store<u32>(sp + 152, 0);
        store<u32>(sp + 176, 0x3F800000u);
        store<u32>(sp + 180, 0xFFFFFFFFu);
        store<u32>(sp + 224, 0);
        store<u32>(sp + 192, 0xFFFFFFFFu);
        store<u32>(sp + 228, 0);
        const u32 world = load<u32>(load<u32>(load<u32>(kV3WorldPtr) + 0x001281D4) + 4);
        const Masks3 mk = LoadMasks3(K + 16);
        Stvx(sp + 112, RowSel(Fill4(x), Fill4(y), Fill4(z), mk));
        store<u32>(sp + 148, 13);
        Stvx(sp + 128, RowSel(Fill4(x1), Fill4(y1), Fill4(z1), mk));
        call(ctx, kCastRay3, {world, sp + 112, sp + 160});
        if (load<u32>(sp + 228) != 0) {
            const f32 t = F(sp + 176);
            x = Fmadds(t, k1, x);
            y = Fmadds(t, k2, y);
            z = Fmadds(t, k1, z);
        }
    }
    if (load<u64>(o + 480) != load<u64>(kV3Zero3) || load<u32>(o + 488) != load<u32>(kV3Zero3 + 8)) y = F(o + 484);
    const f32 eps = F(K + 64);
    if (!(std::fabs(Subs(x, F(o + 480))) < eps && std::fabs(Subs(y, F(o + 484))) < eps && std::fabs(Subs(z, F(o + 488))) < eps)) {
        store<f32>(o + 480, x);
        store<f32>(o + 484, y);
        store<f32>(o + 488, z);
    }
    ctx.fpr[1] = eps;
    ctx.gpr[3] = K;
}

namespace {
constexpr u32 kPool2BBE20 = 0x002BBDE0;  // f32 1e-5; +16 three lane masks
}  // namespace

// void f(T* o; f32 dt) (a callback; no direct callers): moves o's Havok
// proxy with its owner. With bit 0x80000000 of +32, dt >= 1e-5 and (+24)
// +4 set: o+48 = the owner's (+8) matrix (+96) translation, the proxy
// (+28) gets it as a position (0x009CEF60(proxy, {t, 0})), then the object
// 0x009CEEAC(proxy) returns gets the full transform (0x00929E98(it, rows
// 0..3 as {x, y, z, 0})). f1 / r3 are left as the original leaves them.
DECOMP(002BBE20, HkProxy_FollowOwner_002BBE20) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 K = kPool2BBE20;
    if ((load<u32>(o + 32) & 0x80000000u) == 0) {
        ctx.gpr[3] = 0;
        return;
    }
    if (!(ctx.fpr[1] >= F(K))) {
        ctx.gpr[3] = K;
        return;
    }
    if (load<u32>(load<u32>(o + 24) + 4) == 0) {
        ctx.gpr[3] = 0;
        return;
    }
    Stack frame(ctx, 416);
    const u32 sp = frame.addr();
    const u32 s = load<u32>(o + 8) + 96;
    for (u32 r = 0; r < 4; ++r)
        for (u32 k = 0; k < 3; ++k) CopyW(sp + 192 + 16 * r + 4 * k, s + 16 * r + 4 * k);
    CopyW(o + 48, s + 48);
    CopyW(o + 52, s + 52);
    CopyW(o + 56, s + 56);
    const Masks3 mk = LoadMasks3(K + 16);
    // the rows' x lanes (and all of row 0) are loaded before the calls
    const V4 x0 = SplatW(s), y0 = SplatW(s + 4), z0 = SplatW(s + 8);
    const V4 xr[3] = {SplatW(s + 16), SplatW(s + 32), SplatW(s + 48)};
    const u32 f1w = load<u32>(s + 48);
    Stvx(sp + 112, RowSelW(o + 48, o + 52, o + 56, mk));
    call(ctx, 0x009CEF60, {load<u32>(o + 28), sp + 112});
    const u32 x = static_cast<u32>(call(ctx, 0x009CEEAC, {load<u32>(o + 28)}));
    Stvx(sp + 128, RowSel(x0, y0, z0, mk));
    for (u32 r = 1; r < 4; ++r) Stvx(sp + 128 + 16 * r, RowSel(xr[r - 1], SplatW(sp + 196 + 16 * r), SplatW(sp + 200 + 16 * r), mk));
    call(ctx, 0x00929E98, {x, sp + 128});
    SetF1Bits(ctx, f1w);
}

namespace {
constexpr u32 kMasks4A4CA0 = 0x004A4C70;    // three lane masks
constexpr u32 kAllHitsVt = 0x00F1DE80;      // all-hits ray collector vtable (80-byte hits)
constexpr u32 kCastRayAll = 0x0092E510;     // HkWorld_CastRay92E510 (world, input, collector)
constexpr u32 kAllHitsSort = 0x00887E28;    // HkAllHits_Sort887E28 (collector)
}  // namespace

// bool f(World* w /* or 0: the world's +0x1281D4 ->+4 */, const Vec3* dir,
// const Vec3* from, -, u32 filter, Vec3* hit /* or 0 */; f32 len) (called
// by 00179460): casts from `from` to from + dir * len with a stack
// all-hits collector (inline room for 8 80-byte hits, each initialised:
// fraction 1.0, ids -1, +64 / +68 0), sorts the hits and, with any, stores
// from + dir * (the first hit's fraction +16) to hit and returns 1; else 0.
// The collector's array is freed (the inlined destructor; capacity * 80
// bytes) unless marked "don't free". f1 is left as the original leaves it.
DECOMP(004A4CA0, Hk_RayHitPoint_004A4CA0) {
    u32 w = arg<u32>(ctx, 0);
    const u32 dir = arg<u32>(ctx, 1), from = arg<u32>(ctx, 2), filt = arg<u32>(ctx, 4), out = arg<u32>(ctx, 5);
    const f64 len = ctx.fpr[1];
    Stack frame(ctx, 896);
    const u32 sp = frame.addr();
    const V4 fx = SplatW(from), fy = SplatW(from + 4), fz = SplatW(from + 8);
    const f32 px = F(from), py = F(from + 4), pz = F(from + 8);
    const f32 dx = F(dir), dy = F(dir + 4), dz = F(dir + 8);
    if (w == 0) w = load<u32>(load<u32>(load<u32>(kV3WorldPtr) + 0x001281D4) + 4);
    const f32 tx = Fmadds(dx, len, px), ty = Fmadds(dy, len, py), tz = Fmadds(dz, len, pz);
    store<u8>(sp + 144, 0);
    store<u32>(sp + 152, 0);
    const Masks3 mk = LoadMasks3(kMasks4A4CA0);
    Stvx(sp + 112, RowSel(fx, fy, fz, mk));
    const u32 c = sp + 160;  // the collector
    store<u32>(sp + 148, filt);
    store<u32>(c + 4, 0x3F800000u);
    store<u32>(c + 16, sp + 192);
    store<u32>(c, kAllHitsVt);
    store<u32>(c + 20, 0);
    store<u32>(c + 24, 0x80000008u);
    Stvx(sp + 128, RowSel(Fill4(tx), Fill4(ty), Fill4(tz), mk));
    for (u32 k = 0; k < 8; ++k) {
        const u32 e = sp + 192 + 80 * k;
        store<u32>(e + 16, 0x3F800000u);
        store<u32>(e + 20, 0xFFFFFFFFu);
        store<u32>(e + 64, 0);
        store<u32>(e + 32, 0xFFFFFFFFu);
        store<u32>(e + 68, 0);
    }
    store<u32>(c + 20, 0);
    store<u32>(c + 4, 0x3F800000u);
    call(ctx, kCastRayAll, {w, sp + 112, c});
    call(ctx, kAllHitsSort, {c});
    const u32 n = load<u32>(c + 20), fl = load<u32>(c + 24), data = load<u32>(c + 16);
    f32 f1 = tz;
    if (n != 0 && out != 0) {
        const f32 t = F(data + 16);
        const f32 hx = Fmadds(F(dir), t, F(from)), hy = Fmadds(F(dir + 4), t, F(from + 4)), hz = Fmadds(F(dir + 8), t, F(from + 8));
        store<f32>(out, hx);
        store<f32>(out + 4, hy);
        store<f32>(out + 8, hz);
        f1 = hz;
    }
    if ((fl & 0x80000000u) == 0) {
        const u64 sz = static_cast<u64>(static_cast<s64>(static_cast<s32>((fl & 0x3FFFFFFFu) * 80u)));
        store<u32>(c + 20, 0);
        store<u32>(c, kAllHitsVt);
        const u32 opd = load<u32>(load<u32>(kHkAlloc3) + 20);
        if (n != 0) call_ptr(ctx, opd, {kHkAlloc3, data, sz, load<u32>(opd), opd, sz});
        else call_ptr(ctx, opd, {kHkAlloc3, data, sz, load<u32>(opd), opd});
    }
    ctx.fpr[1] = f1;
    ret(ctx, n != 0 ? 1u : 0u);
}
