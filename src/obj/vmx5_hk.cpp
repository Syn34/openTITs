// Game code, VMX sweep part 5 (see vmx5.h): Havok-body game code.
#include "../sly4.h"
#include "vmx5.h"

using namespace sly4;
using namespace sly4::vmx5;

namespace {
// f32 0, 1, 0.1, 0.001, 3.0 (+16); lane masks z (+32), y (+48), w (+64);
// f32 0.01 (+80), 0.6 (+84), 0.4 (+88), 2.0 (+92).
constexpr u32 kK189B20 = 0x00189AC0;
// call_ptr through o's vt[96] as the original sets it up: r3 = o, r4 the
// OPD, r5 = v, r6 the entry; f1 = dt.
inline void PushVcall96(PPUContext& ctx, u32 o, u32 v, f64 dt) {
    const u32 opd = load<u32>(load<u32>(o) + 96);
    ctx.fpr[1] = dt;
    call_ptr(ctx, opd, {o, opd, v, load<u32>(opd)});
}
}  // namespace

// void f(Obj* o; f32 dt) (called by 0018BC90): pushes o's Havok body by its
// stick input. Nothing when dt < 0. With the input (x, z) = o->+1584 /
// +1592 and its length l (capped at 1): when l >= 0.1 the direction is
// normalized and negated. With a body (o->+388) and o's byte +1424 set:
// the body's velocity (lanes 0 / 2 of its +432 when its
// +8 is set, else 0) and the scaled input (o->+1448 * 0.001) give an impulse
// {x, 0, z} (vsel rows) applied through Hk_Activate(body) and the body's
// +224 vt[96](+224, &v; dt): (input * l - vel * o->+1452 * 3) * 0.01 when
// l >= 0.1, else -vel * o->+1452 * 3 * 2 with the direction then zeroed.
// Finally o->+1504.. = 0.6 * o->+1504.. + 0.4 * (x, 0-ish, z) (exponential
// smoothing of the direction). r3 / f1 are left as the original leaves them.
DECOMP(00189B20, Obj_HkPushUpdate_00189B20) {
    const u32 o = arg<u32>(ctx, 0);
    const f64 dt = ctx.fpr[1];
    const u32 K = kK189B20;
    const f32 k0 = F(K);
    if (dt < k0) {
        ctx.gpr[3] = o;
        return;
    }
    Stack frame(ctx, 240);
    const u32 sp = static_cast<u32>(ctx.gpr[1]);
    f32 x = F(o + 1584), z = F(o + 1592);
    const f32 one = F(K + 4);
    const f32 g1448 = F(o + 1448), g1452 = F(o + 1452);
    const u32 body = load<u32>(o + 388);
    const u8 on = load<u8>(o + 1424);
    const f32 len = Sqrts(Fmadds(z, z, Fmadds(x, x, Muls(k0, k0))));
    f32 l = len;
    if (len > one) l = one;
    const f32 k2 = F(K + 8);
    if (l >= k2) {
        const f32 s = Divs(one, len);
        x = -Muls(x, s);
        z = -Muls(z, s);
    }
    const f32 sc = Muls(g1448, F(K + 12));
    const f32 ix = Muls(x, sc), iz = Muls(z, sc), iy = Muls(k0, sc);
    f32 vx = k0, vz = k0, y = k0;
    if (body != 0) {
        if (load<u32>(body + 8) != 0) {
            const V4 v = Lvx(body + 432);
            Stvx(sp + 112, v);
            vx = F(sp + 112);
            vz = F(sp + 120);
        }
        if (on != 0) {
            const u32 hb = body + 224;
            const f32 k4 = F(K + 16);
            const Masks3 m{{Lvx(K + 48), Lvx(K + 32), Lvx(K + 64)}};
            if (l >= k2) {
                const f32 a = Muls(Muls(vx, g1452), k4), b = Muls(Muls(k0, g1452), k4), c = Muls(Muls(vz, g1452), k4);
                const f32 k80 = F(K + 80);
                const f32 px = Muls(Fmsubs(ix, l, a), k80), py = Muls(Fmsubs(iy, l, b), k80), pz = Muls(Fmsubs(iz, l, c), k80);
                Stvx(sp + 128, RowSel(Fill4(px), Fill4(py), Fill4(pz), m));
                call(ctx, v4fn::Hk_Activate, {body});
                PushVcall96(ctx, hb, sp + 128, dt);
            } else {
                const f32 k92 = F(K + 92);
                const f32 px = Muls(Muls(Muls(-vx, g1452), k4), k92), py = Muls(Muls(Muls(-k0, g1452), k4), k92);
                const f32 pz = Muls(Muls(Muls(-vz, g1452), k4), k92);
                Stvx(sp + 144, RowSel(Fill4(px), Fill4(py), Fill4(pz), m));
                call(ctx, v4fn::Hk_Activate, {body});
                PushVcall96(ctx, hb, sp + 144, dt);
                y = F(kV3Zero3 + 4);
                z = F(kV3Zero3 + 8);
                x = F(kV3Zero3);
            }
        }
    }
    const f32 a = F(K + 88), b = F(K + 84);
    const f32 n0 = Fmadds(b, F(o + 1504), Muls(a, x)), n1 = Fmadds(b, F(o + 1508), Muls(a, y)), n2 = Fmadds(b, F(o + 1512), Muls(a, z));
    store<f32>(o + 1504, n0);
    store<f32>(o + 1508, n1);
    store<f32>(o + 1512, n2);
    ctx.fpr[1] = n2;
    ctx.gpr[3] = K;
}

namespace {
constexpr u32 kK277C90 = 0x00277C50;  // f32 0.5, 0.001; lane masks y (+16), z (+32), w (+48)
constexpr u32 kBoxRadius277C90 = 0x00F9203C;  // f32 the box shapes' convex radius
}  // namespace

// hkpShape* f(const ShapeDesc* d) (called by 00278000, 00278830): builds the
// Havok shape d describes (d->+0 type; d+4.. its sizes). 0: hkpSphereShape
// (32 bytes) of radius d->+4; 1: a capsule (64 bytes, 00899B24) along the
// up axis from +h to -h, h = max((d->+8 - d->+4) / 2, 0.001), radius
// d->+4; 2: hkpBoxShape (48 bytes) of half extents {d+4, d+8, d+12, 0};
// else 0. Allocated through the thread's Havok allocator (HkAlloc: the size
// goes to +4 even when the allocation failed). Returns the shape or 0; f1
// as the original leaves it.
DECOMP(00277C90, HkShape_FromDesc_00277C90) {
    const u32 d = arg<u32>(ctx, 0);
    const s32 type = static_cast<s32>(load<u32>(d));
    const u32 K = kK277C90;
    Stack frame(ctx, 208);
    const u32 sp = static_cast<u32>(ctx.gpr[1]);
    if (type > 0) {
        if (type > 1) {
            if (type > 2) {
                ctx.gpr[3] = 0;
                return;
            }
            const u32 p = HkAlloc(ctx, 48);
            if (p == 0) {
                ctx.gpr[3] = 0;
                return;
            }
            Stvx(sp + 112, RowSelW(d + 4, d + 8, d + 12, LoadMasks3(K + 16)));
            callf(ctx, v5fn::HkBoxShape_Ctor, {p, sp + 112}, {Lfs4(kBoxRadius277C90)});
            ctx.gpr[3] = p;
            return;
        }
        f32 h = F(K + 4);
        const f32 len = Muls(Subs(F(d + 8), F(d + 4)), F(K));
        if (!(len < h)) h = len;
        ctx.fpr[1] = len;
        const u32 p = HkAlloc(ctx, 64);
        if (p == 0) {
            ctx.gpr[3] = 0;
            return;
        }
        const u32 A = kUpAxis5;
        const f32 ax = F(A), ay = F(A + 4), az = F(A + 8);
        const Masks3 m = LoadMasks3(K + 16);
        Stvx(sp + 128, RowSel(Fill4(Muls(ax, h)), Fill4(Muls(ay, h)), Fill4(Muls(az, h)), m));
        const f32 nh = -h;
        Stvx(sp + 144, RowSel(Fill4(Muls(ax, nh)), Fill4(Muls(ay, nh)), Fill4(Muls(az, nh)), m));
        callf(ctx, v5fn::HkCapsuleShape_Ctor, {p, sp + 128, sp + 144}, {Lfs4(d + 4)});
        ctx.gpr[3] = p;
        return;
    }
    if (!(type > -1)) {
        ctx.gpr[3] = 0;
        return;
    }
    const u32 p = HkAlloc(ctx, 32);
    if (p != 0) callf(ctx, v5fn::HkSphereShape_Ctor, {p}, {Lfs4(d + 4)});
    ctx.gpr[3] = p;
}

namespace {
constexpr u32 kK185370 = 0x00185330;  // f32 0.0; lane masks y (+16), z (+32), w (+48)
// The two vt[352] reaction calls: the registers as the original leaves them
// (with the hit's count: r4 the OPD, r7 the entry; without: r4 the entry,
// r7 the OPD), f1 = scale. Bit 0x100 of the hit's u16 +14 = (result == 0).
inline void HitReact352(PPUContext& ctx, u32 o, u32 hit, u32 key, bool opd_in_r4, f64 scale) {
    const u32 opd = load<u32>(load<u32>(o) + 352);
    const u32 a = load<u32>(hit + 4);
    ctx.fpr[1] = scale;
    const u32 r = static_cast<u32>(opd_in_r4 ? call_ptr(ctx, opd, {o, opd, a, key, load<u32>(opd)}) : call_ptr(ctx, opd, {o, load<u32>(opd), a, key, opd}));
    const u32 clz = static_cast<u32>(std::countl_zero(r));
    store<u16>(hit + 14, static_cast<u16>((load<u16>(hit + 14) & ~0x100u) | ((clz << 3) & 0x100u)));
    ctx.gpr[3] = clz;
}
}  // namespace

// void f(Obj* o, HitEvent* hit) (called through a pointer): o's hit
// reaction, at most every 0.25 s (o->+1616 is the cool-down: nothing while
// it is > 0, else reset to 0.25). With Obj_TestE0BCC(o, hit): a counted hit
// (u16 +12) sets the hit's force +36 to 20; with a body (o->+388) whose type
// byte +232 is neither 4 nor 5, a force and o->+216's +228 scale both
// non-zero: the body's velocity is zeroed (+224 vt[68] with {0, 0, 0} after
// Hk_Activate) and an impulse hit dir (+16..+24) * force * scale applied
// (+224 vt[84]). Then, when o->+216's +224 damage scale is > 0, o's
// vt[352](o, hit->+4, key; hit->+32 * scale) with key 0x044DC6AB (counted)
// or 0x811B1D5E, recording a zero result in bit 0x100 of the hit's +14.
// r3 / f1 are left as the original leaves them.
DECOMP(00185370, Obj_OnHitImpulse_00185370) {
    const u32 o = arg<u32>(ctx, 0), hit = arg<u32>(ctx, 1);
    const u32 K = kK185370;
    Stack frame(ctx, 256);
    const u32 sp = static_cast<u32>(ctx.gpr[1]);
    const f32 cd = F(o + 1616);
    const f32 z = F(K);
    const u32 t = load<u32>(o + 216);
    ctx.fpr[1] = cd;
    if (cd > z) return;
    store<u32>(o + 1616, 0x3E800000u);
    const u32 ok = static_cast<u32>(call(ctx, v5fn::Obj_TestE0BCC, {o, hit}));
    const u32 body = load<u32>(o + 388);
    if (ok == 0) return;
    u32 c = load<u16>(hit + 12);
    if (c != 0) store<u32>(hit + 36, 0x41A00000u);
    if (body != 0) {
        const u8 bt = load<u8>(body + 232);
        if (bt != 5 && bt != 4 && F(hit + 36) != z && F(t + 228) != z) {
            const Masks3 m = LoadMasks3(K + 16);
            Stvx(sp + 112, RowSelW(kV3Zero3, kV3Zero3 + 4, kV3Zero3 + 8, m));
            call(ctx, v4fn::Hk_Activate, {body});
            VCall(ctx, body + 224, 68, {body + 224, sp + 112});
            const f32 f = F(hit + 36), s = F(t + 228);
            const f32 ix = Muls(Muls(F(hit + 16), f), s), iy = Muls(Muls(F(hit + 20), f), s), iz = Muls(Muls(F(hit + 24), f), s);
            const u32 b2 = load<u32>(o + 388);
            Stvx(sp + 112, RowSel(Fill4(ix), Fill4(iy), Fill4(iz), m));
            call(ctx, v4fn::Hk_Activate, {b2});
            VCall(ctx, b2 + 224, 84, {b2 + 224, sp + 112});
            c = load<u16>(hit + 12);
        }
    }
    const f32 ds = F(t + 224);
    ctx.fpr[1] = ds;
    ctx.gpr[3] = c;
    if (!(ds > z)) return;
    const f32 sc = Muls(F(hit + 32), ds);
    if (c != 0) HitReact352(ctx, o, hit, 0x044DC6ABu, true, sc);
    else HitReact352(ctx, o, hit, 0x811B1D5Eu, false, sc);
}

namespace {
constexpr u32 kK40AE8 = 0x00040AA0;  // f32 1.0; lane masks y (+16), z (+32), w (+48); f32 20.0 (+64)
constexpr u32 kDownGuard40AE8 = 0x00F9CD08, kDown40AE8 = 0x00F9CD10;  // static local: the down vector {0, -1, 0}
constexpr u32 kCollectorVt5 = 0x00F1DE80;  // hkpAllRayHitCollector vtable (8 inline hits)
constexpr u32 kHeapAlloc5 = 0x011DF31C;    // allocator object: vt[20] free(p, size)
constexpr u32 kWorldPhys5 = 0x001281D4;    // World: the physics holder (+4 hkpWorld*)

// The inlined hkpAllRayHitCollector constructor (8 inline 80-byte hits at
// hits, each with fraction +16 1.0, +20 / +32 -1, +64 / +68 0), as the
// original stores it on the stack.
inline void AllHitsInit(u32 col, u32 hits) {
    store<u32>(col + 4, 0x3F800000u);
    store<u32>(col, kCollectorVt5);
    store<u32>(col + 16, hits);
    store<u32>(col + 20, 0);
    store<u32>(col + 24, 0x80000008u);
    for (u32 k = 0; k < 8; ++k) {
        const u32 h = hits + 80 * k;
        store<u32>(h + 16, 0x3F800000u);
        store<u32>(h + 20, 0xFFFFFFFFu);
        store<u32>(h + 64, 0);
        store<u32>(h + 32, 0xFFFFFFFFu);
        store<u32>(h + 68, 0);
    }
    store<u32>(col + 20, 0);
    store<u32>(col + 4, 0x3F800000u);
}
// The inlined destructor: a heap hit array (capacity word without the top
// bit) is freed through the allocator's vt[20](a, data, capacity * 80)
// (r6 / r7 the OPD and entry, or swapped when swap, as the caller leaves
// them).
inline void AllHitsFree(PPUContext& ctx, u32 col, u32 data, u32 cap, bool swap) {
    if ((cap & 0x80000000u) != 0) return;
    const u32 c = cap & 0x3FFFFFFFu;
    store<u32>(col + 20, 0);
    store<u32>(col, kCollectorVt5);
    const u64 n = static_cast<u64>(static_cast<s64>(static_cast<s32>(c * 80u)));
    const u32 opd = load<u32>(load<u32>(kHeapAlloc5) + 20);
    if (swap) call_ptr(ctx, opd, {kHeapAlloc5, data, n, load<u32>(opd), opd});
    else call_ptr(ctx, opd, {kHeapAlloc5, data, n, opd, load<u32>(opd)});
}
}  // namespace

// void f(-, const f32 p[3], f32 out[3]) (called by 00041B78): drops p onto
// the ground: casts a ray from p + (0, 1, 0) along the static down vector
// times 20 (hkpWorldRayCastInput on the stack, filter info 5) into an
// hkpAllRayHitCollector with 8 inline 80-byte hits (each initialised: +16
// fraction 1.0, +20 / +32 -1, +64 / +68 0), sorts the hits and sets out to
// the first hit point (from + (to - from) * fraction) or, without a hit,
// to the ray start. A heap hit array (capacity word without its top bit) is
// freed through the allocator's vt[20](a, data, capacity * 80). r3 / f1 are
// left as the original leaves them.
DECOMP(00040AE8, Ground_DropPoint_00040AE8) {
    const u32 p = arg<u32>(ctx, 1), out = arg<u32>(ctx, 2);
    const u32 K = kK40AE8;
    Stack frame(ctx, 928);
    const u32 sp = static_cast<u32>(ctx.gpr[1]);
    store<u8>(sp + 144, 0);
    store<u32>(sp + 148, 0);
    store<u32>(sp + 152, 0);
    const f32 fx = F(p), fz = F(p + 8);
    const f32 fy = Adds(F(p + 4), F(K));
    const Masks3 m = LoadMasks3(K + 16);
    Stvx(sp + 112, RowSel(SplatW(p), Fill4(fy), SplatW(p + 8), m));  // m_from
    if (load<u8>(kDownGuard40AE8) == 0) {
        store<u32>(kDown40AE8, 0);
        store<u32>(kDown40AE8 + 4, 0xBF800000u);
        store<u32>(kDown40AE8 + 8, 0);
        store<u8>(kDownGuard40AE8, 1);
    }
    const f32 len = F(K + 64);
    const f32 tx = Fmadds(F(kDown40AE8), len, fx), ty = Fmadds(F(kDown40AE8 + 4), len, fy);
    const f32 dz = F(kDown40AE8 + 8);
    const f32 tz = Fmadds(dz, len, fz);
    store<u32>(sp + 148, 5);
    Stvx(sp + 128, RowSel(Fill4(tx), Fill4(ty), Fill4(tz), m));  // m_to
    const u32 world = load<u32>(load<u32>(load<u32>(kV3WorldPtr) + kWorldPhys5) + 4);
    const u32 col = sp + 160;
    AllHitsInit(col, sp + 192);
    call(ctx, v4fn::HkWorld_CastRay, {world, sp + 112, col});
    call(ctx, v4fn::HkAllHits_Sort, {col});
    const s32 n = static_cast<s32>(load<u32>(col + 20));
    const u32 cap = load<u32>(col + 24), data = load<u32>(col + 16);
    ctx.gpr[3] = data;
    if (n > 0) {
        const f32 t = F(data + 16);
        const f32 ox = Fmadds(Subs(tx, fx), t, fx), oy = Fmadds(Subs(ty, fy), t, fy), oz = Fmadds(Subs(tz, fz), t, fz);
        store<f32>(out, ox);
        store<f32>(out + 4, oy);
        store<f32>(out + 8, oz);
        ctx.fpr[1] = ox;
    } else {
        store<f32>(out, fx);
        store<f32>(out + 4, fy);
        store<f32>(out + 8, fz);
        ctx.fpr[1] = dz;
    }
    AllHitsFree(ctx, col, data, cap, false);
}

namespace {
constexpr u32 kKB33D8 = 0x000B3390;  // f32 1.0; lane masks y (+16), z (+32), w (+48); f32 ray length (+64)
constexpr u32 kDownGuardB33D8 = 0x01013A80, kDownB33D8 = 0x01013A90;  // static local: the down vector {0, -1, 0}
}  // namespace

// void f(const f32 p[3], f32 out[3]) (called by 000B3A60, 000BEAF8): as
// Ground_DropPoint_00040AE8 with filter info 4 and its own down vector /
// length: out = the first sorted hit whose collidable (+68) has a +28 type
// (low 5 bits) other than 8, else p itself.
DECOMP(000B33D8, Ground_DropPointB33D8) {
    const u32 p = arg<u32>(ctx, 0), out = arg<u32>(ctx, 1);
    const u32 K = kKB33D8;
    Stack frame(ctx, 944);
    const u32 sp = static_cast<u32>(ctx.gpr[1]);
    store<u8>(sp + 144, 0);
    store<u32>(sp + 148, 0);
    store<u32>(sp + 152, 0);
    const f32 fy = Adds(F(p + 4), F(K));
    const f32 fx = F(p), fz = F(p + 8);
    const Masks3 m = LoadMasks3(K + 16);
    Stvx(sp + 112, RowSel(SplatW(p), Fill4(fy), SplatW(p + 8), m));  // m_from
    if (load<u8>(kDownGuardB33D8) == 0) {
        store<u32>(kDownB33D8, 0);
        store<u32>(kDownB33D8 + 4, 0xBF800000u);
        store<u32>(kDownB33D8 + 8, 0);
        store<u8>(kDownGuardB33D8, 1);
    }
    const f32 len = F(K + 64);
    const f32 tx = Fmadds(F(kDownB33D8), len, fx), ty = Fmadds(F(kDownB33D8 + 4), len, fy), tz = Fmadds(F(kDownB33D8 + 8), len, fz);
    store<u32>(sp + 148, 4);
    Stvx(sp + 128, RowSel(Fill4(tx), Fill4(ty), Fill4(tz), m));  // m_to
    const u32 world = load<u32>(load<u32>(load<u32>(kV3WorldPtr) + kWorldPhys5) + 4);
    const u32 col = sp + 160;
    AllHitsInit(col, sp + 192);
    call(ctx, v4fn::HkWorld_CastRay, {world, sp + 112, col});
    call(ctx, v4fn::HkAllHits_Sort, {col});
    const s32 n = static_cast<s32>(load<u32>(col + 20));
    const u32 cap = load<u32>(col + 24), data = load<u32>(col + 16);
    ctx.gpr[3] = data;
    u32 hit = 0;
    bool found = false;
    for (s32 i = 0; i < n; ++i) {
        const u32 h = data + 80 * static_cast<u32>(i);
        if ((load<u32>(load<u32>(h + 68) + 28) & 0x1Fu) != 8) {
            hit = h;
            found = true;
            break;
        }
    }
    if (found) {
        const f32 t = F(hit + 16);
        const f32 ox = Fmadds(Subs(tx, fx), t, fx), oy = Fmadds(Subs(ty, fy), t, fy), oz = Fmadds(Subs(tz, fz), t, fz);
        store<f32>(out, ox);
        store<f32>(out + 4, oy);
        store<f32>(out + 8, oz);
        ctx.fpr[1] = ox;
    } else {
        for (u32 k = 0; k < 3; ++k) store<u32>(out + 4 * k, load<u32>(p + 4 * k));
        SetF1Bits(ctx, load<u32>(p + 8));
    }
    AllHitsFree(ctx, col, data, cap, true);
}

namespace {
constexpr u32 kHitsVt5 = 0x00F1DE58;   // all-hits collector vtable (8 inline 48-byte hits)
constexpr u32 kObjProp5 = 0x91C7CF5Bu;  // entity property key: the game object
constexpr u32 kMasks1EEF20 = 0x001EEEF0;  // lane masks y, z, w
constexpr u32 kLen1EEF20 = 0x01053928;    // f32 probe length
// The game object of a hit's collidable (+40): its entity (type +24 == 1,
// owner at +(s8)+16) property kObjProp5's low word, else 0.
inline u32 HitGameObj(u32 hit) {
    const u32 col = load<u32>(hit + 40);
    if (static_cast<s8>(load<u8>(col + 24)) != 1) return 0;
    const u32 ent = col + static_cast<u32>(static_cast<s32>(static_cast<s8>(load<u8>(col + 16))));
    if (ent == 0) return 0;
    const s32 i = big17::HkPropIndex(ent, kObjProp5);
    return i < 0 ? 0 : static_cast<u32>(big17::HkPropValue(ent, i));
}
}  // namespace

// bool f(Obj* o, const f32 from[3], const f32 dir[3], f32* dist, u8* seen)
// (called by 001F08C0): probes o's phantom (o->+852, with its +8 set) along
// dir * length (the phantom's vt[64](ph, &from, &to, &collector, 0) into a
// stack all-hits collector of 48-byte hits, sorted with HkHits_Sort8871E8).
// For the sorted hits: the first one whose game object is not o gives *dist
// = |hit point - from| (and the result 1); a hit whose object's +160 ->+204
// is o's target o->+840 sets *seen = 1 and stops. A heap hit array is freed
// (allocator vt[20], capacity * 48). Returns whether a distance was taken;
// f1 as the original leaves it.
DECOMP(001EEF20, Obj_ProbeRay_001EEF20) {
    const u32 o = arg<u32>(ctx, 0), from = arg<u32>(ctx, 1), dir = arg<u32>(ctx, 2);
    const u32 dist = arg<u32>(ctx, 3), seen = arg<u32>(ctx, 4);
    const u32 ph = load<u32>(o + 852);
    if (ph == 0 || load<u32>(ph + 8) == 0) {
        ctx.gpr[3] = 0;
        return;
    }
    Stack frame(ctx, 656);
    const u32 sp = static_cast<u32>(ctx.gpr[1]);
    const u32 col = sp + 160;
    store<u32>(sp + 144, 0x34000000u);
    store<u32>(sp + 148, 0x34000000u);
    store<u32>(col, kHitsVt5);
    store<u32>(col + 16, sp + 192);
    store<u32>(col + 24, 0x80000008u);
    store<u32>(col + 20, 0);
    store<u32>(col + 4, 0x7F7FFFEEu);
    Stvx(sp + 144, Zero4());
    const Masks3 m = LoadMasks3(kMasks1EEF20);
    Stvx(sp + 112, RowSelW(from, from + 4, from + 8, m));
    const f32 len = F(kLen1EEF20);
    const f32 tx = Fmadds(F(dir), len, F(from)), ty = Fmadds(F(dir + 4), len, F(from + 4)), tz = Fmadds(F(dir + 8), len, F(from + 8));
    Stvx(sp + 128, RowSel(Fill4(tx), Fill4(ty), Fill4(tz), m));
    ctx.fpr[1] = tz;
    VCall(ctx, ph, 64, {ph, sp + 112, sp + 128, col, 0});
    u32 got = 0;
    u32 data = load<u32>(col + 16);
    if (static_cast<s32>(load<u32>(col + 20)) > 0) {
        call(ctx, v5fn::HkHits_Sort8871E8, {col});
        data = load<u32>(col + 16);
        const s32 n = static_cast<s32>(load<u32>(col + 20));
        for (s32 i = 0; i < n; ++i) {
            const u32 h = data + 48 * static_cast<u32>(i);
            const u32 g = HitGameObj(h);
            if (got == 0 && g != o) {
                const f32 dy = Subs(F(h + 4), F(from + 4)), dx = Subs(F(h), F(from)), dz = Subs(F(h + 8), F(from + 8));
                const f32 d = Sqrts(Fmadds(dz, dz, Fmadds(dx, dx, Muls(dy, dy))));
                store<f32>(dist, d);
                ctx.fpr[1] = d;
                got = 1;
            }
            if (g != 0) {
                const u32 tgt = load<u32>(o + 840);
                if (tgt != 0) {
                    const u32 q = load<u32>(g + 160);
                    if (q != 0 && load<u32>(q + 204) == tgt) {
                        store<u8>(seen, 1);
                        break;
                    }
                }
            }
        }
    }
    const u32 cap = load<u32>(col + 24);
    ctx.gpr[3] = got;
    if ((cap & 0x80000000u) != 0) return;
    store<u32>(col + 20, 0);
    store<u32>(col, kHitsVt5);
    VCall(ctx, kHeapAlloc5, 20, {kHeapAlloc5, data, static_cast<u64>(static_cast<s64>(static_cast<s32>((cap & 0x3FFFFFFFu) * 48u)))});
    ctx.gpr[3] = got;
}

namespace {
constexpr u32 kMasks436A90 = 0x00436A60;  // lane masks y, z, w
}  // namespace

// u32 f(Sweep* s) (called by 00436E98): with entries (u16 s->+40, 40-byte
// records at s->+32 whose +32 is a game object) and a phantom s->+28, and
// neither end of the segment s+96 -> s+112 equal to the zero vector (word
// compare): casts it through the phantom's vt[64](ph, &from, &to,
// &collector, 0) into a stack all-hits collector (48-byte hits, sorted with
// HkHits_Sort8871E8). Each hit's game object g that is not yet listed, has
// bit 0x2 of +192 and not bit 0x2 of +172 gets g->vt[352](g, 0, 0; g->+344).
// A heap hit array is freed (allocator vt[20], capacity * 48). Returns 1
// when some object was hit that way, else 0; f1 as the original leaves it.
DECOMP(00436A90, Sweep_HitObjects_00436A90) {
    const u32 s = arg<u32>(ctx, 0);
    const u32 ph = load<u32>(s + 28);
    if (load<u16>(s + 40) == 0 || ph == 0) {
        ctx.gpr[3] = 0;
        return;
    }
    Stack frame(ctx, 656);
    const u32 sp = static_cast<u32>(ctx.gpr[1]);
    const u32 A = s + 96, B = s + 112;
    SetF1Bits(ctx, load<u32>(B + 4));
    const u64 z01 = load<u64>(kV3Zero3);
    const u32 z2 = load<u32>(kV3Zero3 + 8);
    const u64 a01 = (static_cast<u64>(load<u32>(A)) << 32) | load<u32>(A + 4);
    const u64 b01 = (static_cast<u64>(load<u32>(B)) << 32) | load<u32>(B + 4);
    if (((a01 - z01) | (load<u32>(A + 8) - z2)) == 0 || ((b01 - z01) | (load<u32>(B + 8) - z2)) == 0) {
        ctx.gpr[3] = 0;
        return;
    }
    const u32 col = sp + 192;
    store<u32>(sp + 176, 0x34000000u);
    store<u32>(sp + 180, 0x34000000u);
    store<u32>(col, kHitsVt5);
    store<u32>(col + 16, sp + 224);
    store<u32>(col + 24, 0x80000008u);
    store<u32>(col + 20, 0);
    store<u32>(col + 4, 0x7F7FFFEEu);
    Stvx(sp + 176, Zero4());
    const Masks3 m = LoadMasks3(kMasks436A90);
    Stvx(sp + 144, RowSelW(A, A + 4, A + 8, m));
    Stvx(sp + 160, RowSelW(B, B + 4, B + 8, m));
    {
        // r8 the entry, r9 160 and r10 the OPD, as the original leaves them.
        const u32 opd = load<u32>(load<u32>(ph) + 64);
        call_ptr(ctx, opd, {ph, sp + 144, sp + 160, col, 0, load<u32>(opd), 160, opd});
    }
    u32 got = 0;
    if (static_cast<s32>(load<u32>(col + 20)) > 0) {
        call(ctx, v5fn::HkHits_Sort8871E8, {col});
        for (s32 i = 0; i < static_cast<s32>(load<u32>(col + 20)); ++i) {
            const u32 g = HitGameObj(load<u32>(col + 16) + 48 * static_cast<u32>(i));
            if (g == 0) continue;
            bool listed = false;
            for (u32 j = 0; j < load<u16>(s + 40); ++j)
                if (load<u32>(load<u32>(s + 32) + 40 * j + 32) == g) {
                    listed = true;
                    break;
                }
            if (listed || (load<u32>(g + 192) & 2u) == 0 || ((load<u32>(g + 172) >> 1) & 1u) != 0) continue;
            const u32 opd = load<u32>(load<u32>(g) + 352);
            ctx.fpr[1] = Lfs4(g + 344);
            call_ptr(ctx, opd, {g, opd, 0, 0, load<u32>(opd)});
            got = 1;
        }
    }
    const u32 cap = load<u32>(col + 24);
    if ((cap & 0x80000000u) == 0) {
        const u64 n = static_cast<u64>(static_cast<s64>(static_cast<s32>((cap & 0x3FFFFFFFu) * 48u)));
        store<u32>(col + 20, 0);
        const u32 data = load<u32>(col + 16);
        store<u32>(col, kHitsVt5);
        VCall(ctx, kHeapAlloc5, 20, {kHeapAlloc5, data, n});
    }
    ctx.gpr[3] = got;
}
