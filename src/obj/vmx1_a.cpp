// Game code, VMX sweep part 1 (see vmx1.h): matrix / vector transform users
// (attach points, look-at inversion, yaw / pitch, sphere tests, ...).
#include "../sly4.h"
#include "vmx1.h"
#include "obj20.h"

using namespace sly4;
using namespace sly4::vmx1;

namespace {
constexpr u32 kDefaultAttach = 0x01164BD0;  // +32 default direction xyz, +48 default point xyz
constexpr u32 kAttachHash = 0x001F0BAF;     // the attach entry's hash
constexpr u32 kZeroF = 0x0002B428;          // 0.0f (text constant pool)
constexpr u32 kNearDist = 0x0002B42C;       // 1.2f
}  // namespace

// bool f(Obj* o, const T* target) (a callback): the attach entry kAttachHash
// of o, when it has one, gives a point (entry matrix +48) and a direction
// (+32) transformed by o's world matrix (+96); otherwise the defaults at
// kDefaultAttach. True when target's position (+16) is in front of the point
// along the direction and closer than 1.2. Quirk: the distance only uses the
// y offset (the original squares the 0.0 constant for x and z); f1 is left
// holding the distance (or the dot product when not in front).
DECOMP(0002B430, Obj_TargetInFrontOfAttach) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 target = arg<u32>(ctx, 1);
    f32 dir[3] = {F(kDefaultAttach + 32), F(kDefaultAttach + 36), F(kDefaultAttach + 40)};
    f32 pt[3] = {F(kDefaultAttach + 48), F(kDefaultAttach + 52), F(kDefaultAttach + 56)};
    const u64 i = call(ctx, v1fn::Obj_IndexByHash, {o, kAttachHash});
    if (static_cast<s32>(i) >= 0) {
        const u32 e = static_cast<u32>(call(ctx, v1fn::Obj_Entry16, {o, i}));
        const u32 m = load<u32>(e + 8);
        const Mat4 w = LoadMat(o + 96);
        const V4 d = XformDir(w.r[0], w.r[1], w.r[2], Lvx(m + 32));
        const V4 p = XformPoint(w.r[0], w.r[1], w.r[2], w.r[3], Lvx(m + 48));
        for (u32 k = 0; k < 3; ++k) dir[k] = d.e[k], pt[k] = p.e[k];
    }
    const f32 dy = Subs(F(target + 20), pt[1]);
    const f32 dx = Subs(F(target + 16), pt[0]);
    const f32 dz = Subs(F(target + 24), pt[2]);
    const f32 zero = F(kZeroF);
    const f32 dot = Fmadds(dz, dir[2], Fmadds(dx, dir[0], Muls(dy, dir[1])));
    f32 f1 = dot;
    u32 r = 0;
    if (dot > zero) {
        f32 s = Muls(dy, dy);
        s = Fmadds(zero, zero, s);  // Quirk: x and z offsets replaced by the 0.0 constant.
        s = Fmadds(zero, zero, s);
        f1 = Sqrts(s);
        if (f1 < F(kNearDist)) r = 1;
    }
    ctx.fpr[1] = f1;
    ret(ctx, r);
}


namespace {
constexpr u32 kBlendConst = 0x0023A6B0;  // 0.0f, 1.0f; then the lane masks (y, z, w) used to pack x/y/z/0
}  // namespace

// void f(T* o, Out* out) (called by 0023C168): blends two points by the
// share of o->+612's entries (224 bytes each, at +72) whose +36 word is set:
// t = set / count (count = the s8 at (+612)->+28->+32), p = (1 - t) * a +
// t * b with a = (o->+396)+1424 +32 and b = +16. When t != 0, (+1424)+48 >=
// 0 and (+1424)+52 < |o->+116|, p becomes p.y * o's position (+112). Stores
// p - base (base = (+612)->+24->+8 +16) as x/y/z/0 at out+32 (aligned).
// Quirk: +28 / +32 / +24 are read even when +612 or its +28 is null. Leaves
// r3 = 16 and f1 = the x difference, as the original does.
DECOMP(0023A6F0, Obj_BlendOffset_0023A6F0) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 out = arg<u32>(ctx, 1);
    const f32 zero = F(kBlendConst), one = F(kBlendConst + 4);
    const u32 owner = load<u32>(o + 612);
    const u32 tab = load<u32>(o + 396) + 1424;
    const u32 info = load<u32>(owner + 28);  // Quirk: read before the null check.
    f32 set = zero;
    if (owner != 0 && info != 0) {
        const s32 n = load<s8>(info + 32);
        if (n != 0) {
            u32 e = load<u32>(owner + 72);
            for (u32 i = 0; i < static_cast<u32>(n); ++i, e += 224)  // unsigned compare, as the original
                if (load<u32>(e + 36) != 0) set = Adds(set, one);
        }
    }
    const s32 count = load<s8>(info + 32);
    const u32 base = load<u32>(load<u32>(owner + 24) + 8) + 16;
    const f32 t = Divs(set, static_cast<f32>(static_cast<f64>(static_cast<s64>(count))));
    const f32 u = Subs(one, t);
    const f32 bx = F(base), by = F(base + 4), bz = F(base + 8);
    f32 x = Fmadds(t, F(tab + 16), Fmadds(u, F(tab + 32), bx));
    f32 y = Fmadds(t, F(tab + 20), Fmadds(u, F(tab + 36), by));
    f32 z = Fmadds(t, F(tab + 24), Fmadds(u, F(tab + 40), bz));
    const f32 px = F(o + 112), py = F(o + 116), pz = F(o + 120);
    if (t != zero && F(tab + 48) >= zero && F(tab + 52) < std::fabs(py)) {
        x = Muls(y, px);
        z = Muls(y, pz);
        y = Muls(y, py);
    }
    const f32 dx = Subs(x, bx), dy = Subs(y, by), dz = Subs(z, bz);
    Stvx(out + 32, V4{{dx, dy, dz, 0.0f}});  // vsel / vandc with the lane masks: w cleared
    ctx.fpr[1] = dx;
    ctx.gpr[3] = 16;  // the original's last r3 (an lvx offset)
}

// void f(Box* out, T* o) (a callback): the component at o->+400 transforms
// the default matrix (0x01164BD0) through its +16 object's slot 20 and the
// result's bounds go to out; without it Box_FromModel(out, o). Another
// instance of the obj20 component-transform template (masks 0x000E9980);
// f1 is left holding the result's x (the original's last lfs into f1). r3
// is left as the original's stack temporary there, which nothing reads.
DECOMP(000E99B0, Comp_Xform_000E99B0) {
    const u32 out = arg<u32>(ctx, 0);
    const bool has = load<u32>(arg<u32>(ctx, 1) + 400) != 0;
    obj20::TXformVCall(ctx, 400, 0x000CB934, 0x01164BD0, 0x000E9980);
    if (has) SetF1Bits(ctx, load<u32>(out));
}

namespace {
constexpr u32 kVisConst = 0x00033558;    // 1/65535 (0x37800080), 0.25, 2.0, 0.0 (text constant pool)
constexpr u32 kVisPassFlags = 0x00F95034;  // T* : +64 the current pass index
constexpr u32 kSceneHandle = 0x00F557F0;   // T* : +68 an f32 timer
constexpr u32 kTimerReset = 0x00F9CB74;    // f32 : the value the timer is reset to
}  // namespace

// bool f(Obj* o) (called by 00034598 / 00039670): whether o may act now.
// Without the state o->+256 (s): true. When s's +188 bit 0 is set or +244
// is null, and o's pass table (o->+308->+12, one u32 per pass) has the
// current pass set: o's position (+176) goes through the two 4x4 matrices
// at world (*0x00F65CA0)->+0x128168 +368 and +432 (+2.0 on every lane);
// true only when every lane of both is >= 0 and the scene timer
// (*0x00F557F0)->+68 is below 0 (then reset from 0x00F9CB74). Otherwise:
// s's +192 bit 4 clear: true; else a random roll (the 0x01164888 step)
// true one time in four. f1 is left as the original leaves it.
DECOMP(00033568, Obj_MayAct_00033568) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 s = load<u32>(o + 256);
    if (s == 0) {
        ret(ctx, 1u);
        return;
    }
    if ((load<u32>(s + 188) & 1) != 0 || load<u32>(s + 244) == 0) {
        const u32 pass = load<u32>(load<u32>(kVisPassFlags) + 64);
        if (load<u8>(load<u32>(load<u32>(o + 308) + 12) + (pass << 2)) != 0) {
            const u32 w = load<u32>(load<u32>(0x00F65CA0) + 0x128168);
            const f32 x = Lvlx(o + 176).e[0], y = Lvlx(o + 180).e[0], z = Lvlx(o + 184).e[0];
            const f32 two = Lvlx(kVisConst + 8).e[0];
            auto inside = [&](u32 m) {
                const V4 r0 = Lvx(m), r1 = Lvx(m + 16), r2 = Lvx(m + 32), t = Lvx(m + 48);
                bool all = true;
                for (u32 k = 0; k < 4; ++k) {
                    f32 v = Vmadd(r0.e[k], x, t.e[k]);
                    v = Vmadd(r1.e[k], y, v);
                    v = Vmadd(r2.e[k], z, v);
                    all = all && (two + v >= 0.0f);  // vcmpgefp. against a zero vector
                }
                return all;
            };
            const bool a = inside(w + 368);
            if (!a || !inside(w + 432)) {
                ret(ctx, 0u);
                return;
            }
            const u32 sc = load<u32>(kSceneHandle);
            const f32 lim = F(kVisConst + 12);
            ctx.fpr[1] = lim;
            if (F(sc + 68) < lim) {
                const u32 t = load<u32>(kTimerReset);
                store<u32>(sc + 68, t);
                SetF1Bits(ctx, t);
                ret(ctx, 1u);
                return;
            }
            ret(ctx, 0u);
            return;
        }
    }
    if ((load<u32>(s + 192) & 0x10) == 0) {
        ret(ctx, 1u);
        return;
    }
    const f32 roll = Muls(static_cast<f32>(static_cast<f64>(static_cast<s64>(RandStep28()))), F(kVisConst));
    ctx.fpr[1] = roll;
    ret(ctx, roll < F(kVisConst + 4) ? 1u : 0u);
}

namespace {
constexpr u32 kHalf = 0x002462D4;           // 0.5f
constexpr u32 kIdentityRows = 0x00B2C360;   // three 16-byte identity rows (the phantom's rotation)
constexpr u32 kHkSphereShapeCtor = 0x0089FE70;  // (hkpSphereShape*; f1 radius) (Havok)
constexpr u32 kHkPhantomCtor = 0x0092A718;  // (hkpSimpleShapePhantom*, shape, const hkTransform*, u32 filterInfo) (Havok; name?)
constexpr u32 kHkRemoveReference = 0x007D4EA8;  // (hkReferencedObject* or 0)
constexpr u32 kHkWorldAddPhantom = 0x00931D10;  // (hkpWorld*, phantom) (Body_Add931D10)
}  // namespace

// T f(Obj* o) (called by 00247AB8): creates o's sphere phantom once (o->+1344
// set or no extent def o->+216: nothing, r3 = o->+1344). Radius o->+1348 =
// 0.5 * |def +800..808|; a 32-byte hkpSphereShape and a 352-byte phantom
// from the Havok router (u16 size to +4 even when null, as the original);
// the phantom gets the identity transform and filter info (o->+416 ? its
// +44 high half : 0) | (o->+188 bit 0 ? 10 : 11). o->+1344 = the phantom
// (0 when its allocation failed); the shape reference is released and the
// phantom added to the world's physics (*0x00F65CA0 +0x1281D4)->+4. r3 is
// the last call's result.
DECOMP(002462D8, Obj_CreateSpherePhantom_002462D8) {
    const u32 o = arg<u32>(ctx, 0);
    if (load<u32>(o + 1344) != 0 || load<u32>(o + 216) == 0) {
        ret(ctx, load<u32>(o + 1344));
        return;
    }
    const u32 def = load<u32>(o + 216);
    const f32 x = F(def + 800), y = F(def + 804), z = F(def + 808);
    const f32 radius = Muls(Sqrts(Fmadds(z, z, Fmadds(x, x, Muls(y, y)))), F(kHalf));
    store<f32>(o + 1348, radius);
    ctx.fpr[1] = radius;
    u32 shape = 0;
    const u32 sp = HkAlloc(ctx, 32);
    if (sp != 0) {
        callf(ctx, kHkSphereShapeCtor, {sp}, {F(o + 1348)});
        shape = sp;
    }
    Stack frame(ctx, 64);
    const u32 xf = frame.addr();  // the original's sp+112: hkTransform (3 rotation rows, zero translation)
    for (u32 k = 0; k < 48; k += 4) store<u32>(xf + k, load<u32>(kIdentityRows + k));
    for (u32 k = 48; k < 64; k += 4) store<u32>(xf + k, 0);
    const u32 owner = load<u32>(o + 416);
    const u32 hi = owner != 0 ? load<u32>(owner + 44) : 0;
    const u32 layer = (load<u32>(o + 188) & 1) != 0 ? 10 : 11;
    const u32 filter = (hi & 0xFFFF0000u) | layer;
    u32 ph = 0;
    const u32 pp = HkAlloc(ctx, 352);
    if (pp != 0) {
        call(ctx, kHkPhantomCtor, {pp, shape, xf, filter});
        ph = pp;
    }
    store<u32>(o + 1344, ph);
    call(ctx, kHkRemoveReference, {shape});
    const u32 phys = load<u32>(load<u32>(0x00F65CA0) + 0x1281D4);
    call(ctx, kHkWorldAddPhantom, {load<u32>(phys + 4), load<u32>(o + 1344)});
}

namespace {
constexpr u32 kOrigin = 0x01164B40;    // the zero point
constexpr u32 kIdentity = 0x01164BD0;  // the identity matrix
}  // namespace

// void f(Obj* o, const T* holder, f32 out[4]) (called by 003B55B8 /
// 003B5720): out = the origin; with a holder, out = holder's matrix (+96)
// applied to the point (o->+96)+240, plus that rotated by the matrix
// (o->+128 ? (o->+128)->+40 +80 : identity) applied to (o->+96)+224. The
// original copies each point to out before using it and stores the
// vectors at out & ~15 (stvx). Quirk: the w lanes of the second matrix and
// of the reloaded point come from its own uninitialised stack (sp+60 /
// +76 / +92 / +108, below the caller's r1), so out[3] is garbage-derived;
// they're read from the same guest stack words here. f1 is left holding
// the last word copied to out and r3 the vperm mask pool, as the original.
DECOMP(003B5170, Obj_OffsetPoint_003B5170) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 holder = arg<u32>(ctx, 1);
    const u32 out = arg<u32>(ctx, 2);
    for (u32 k = 0; k < 12; k += 4) store<u32>(out + k, load<u32>(kOrigin + k));
    SetF1Bits(ctx, load<u32>(kOrigin + 8));  // f1: the last word copied (lfs / stfs pairs)
    if (holder == 0) return;
    const u32 sp = static_cast<u32>(ctx.gpr[1]) - 144;  // the original's frame
    // Copy a point to out word by word, reading each lane as lvlx does.
    auto copy3 = [&](u32 p, f32 v[3]) {
        u32 w = 0;
        for (u32 k = 0; k < 3; ++k) {
            w = load<u32>(p + 4 * k);
            v[k] = Lvlx(p + 4 * k).e[0];
            store<u32>(out + 4 * k, w);
        }
        return w;
    };
    f32 a[3];
    copy3(load<u32>(o + 96) + 240, a);
    const Mat4 m = LoadMat(holder + 96);
    const V4 t = XformPoint(m.r[0], m.r[1], m.r[2], m.r[3], V4{{a[0], a[1], a[2], 0.0f}});
    const u32 ref = load<u32>(o + 128);
    const u32 base = load<u32>(o + 96);
    Stvx(out, t);
    const V4 p{{Lvlx(out).e[0], Lvlx(out + 4).e[0], Lvlx(out + 8).e[0], F(sp + 108)}};
    const u32 r = ref != 0 ? load<u32>(ref + 40) + 80 : kIdentity;
    V4 row[3];
    for (u32 i = 0; i < 3; ++i)
        row[i] = V4{{Lvlx(r + 16 * i).e[0], Lvlx(r + 16 * i + 4).e[0], Lvlx(r + 16 * i + 8).e[0], F(sp + 60 + 16 * i)}};
    f32 b[3];
    const u32 last = copy3(base + 224, b);
    Stvx(out, Add4(p, XformDir(row[0], row[1], row[2], V4{{b[0], b[1], b[2], 0.0f}})));
    SetF1Bits(ctx, last);
    ctx.gpr[3] = 0x003B5150;  // the original's last r3: its vperm mask pool
}

namespace {
constexpr u32 kObjFindComponent = 0x000D1288;  // (o, s32 kind, 0) -> component or 0
constexpr u32 kObjAttachMatrix = 0x000E3534;   // (Obj*, f32 out[4][4], const Attach* el, const f32 base[4][4])
}  // namespace

// void f(f32 out[3], Obj* o) (through Thunk_001EB078): o's attach point.
// With a kind-27 component: o->+144. Else the matrix m = the first attach
// of o->+388 (when it has any) through Obj_AttachMatrix on the base
// (o->+160 ? its +80 : identity), or the base's rows themselves; out = m's
// translation, or, when def (o->+216)->+192 has entries, m applied to the
// point at its first entry's +128. f1 is left holding out[2]; r3 as the
// original leaves it (o+144, the call result / base, 0 or 112).
DECOMP(000E7DC4, Obj_GetAttachPoint_000E7DC4) {
    const u32 out = arg<u32>(ctx, 0);
    const u32 o = arg<u32>(ctx, 1);
    if (static_cast<s32>(call(ctx, kObjFindComponent, {o, 27, 0})) != 0) {
        for (u32 k = 0; k < 12; k += 4) store<u32>(out + k, load<u32>(o + 144 + k));
        SetF1Bits(ctx, load<u32>(o + 152));
        ctx.gpr[3] = o + 144;
        return;
    }
    const u32 drv = load<u32>(o + 160);
    const u32 def = load<u32>(o + 216);
    const u32 base = drv != 0 ? drv + 80 : kIdentity;
    Stack frame(ctx, 80);
    const u32 mat = frame.addr() + 16;  // the original's sp+128 (its sp+112 holds the point)
    u32 t[3];
    const u32 list = load<u32>(o + 388);
    if (list != 0 && load<u16>(list + 8) != 0) {
        call(ctx, kObjAttachMatrix, {o, mat, load<u32>(list), base});
    } else {
        for (u32 r = 0; r < 4; ++r)
            for (u32 k = 0; k < 12; k += 4) store<u32>(mat + 16 * r + k, load<u32>(base + 16 * r + k));
        ctx.gpr[3] = base;
    }
    for (u32 k = 0; k < 3; ++k) t[k] = load<u32>(mat + 48 + 4 * k);
    const u32 pts = load<u32>(def + 192);
    if (pts != 0) {
        if (load<u16>(pts + 8) == 0) {
            ctx.gpr[3] = 0;
        } else {
            const u32 p = load<u32>(pts);
            const Mat4 m = LoadMat(mat);
            const V4 v = XformPoint(m.r[0], m.r[1], m.r[2], m.r[3], V4{{Fw(p + 128), Fw(p + 132), Fw(p + 136), 0.0f}});
            for (u32 k = 0; k < 3; ++k) t[k] = LaneW(v, k);
            ctx.gpr[3] = 112;
        }
    }
    for (u32 k = 0; k < 3; ++k) store<u32>(out + 4 * k, t[k]);
    SetF1Bits(ctx, t[2]);
}

namespace {
constexpr u32 kMatrixRotationX = 0x00553D18;  // (f32 m[4][4], f1 angle)
constexpr u32 kMatrixRotationY = 0x00553E18;  // (f32 m[4][4], f1 angle)
constexpr u32 kMatrixMultiply = 0x005541F0;   // (Matrix* a, const Matrix* b): a = a x b
constexpr u32 kMatrixNormalize = 0x0056D374;  // (f32 m[4][4]) -> m
}  // namespace

// void f(T* o) (called by 001DCBA0 / 001E3A90 / 001E4688): o's matrix
// +512 = the base matrix +448 (x/y/z words), times a rotation about y by
// o->+580 (vmaddfp rows, translation row kept), times a rotation about x
// by o->+576 (Matrix_Multiply), normalised; the normalised rows are
// copied back from the matrix Matrix_NormalizeInPlace returns. r3 is that
// result and f1 its last word copied.
DECOMP(001DC988, Obj_ApplyYawPitch_001DC988) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 m = o + 512;
    Stack frame(ctx, 64);
    const u32 rot = frame.addr();  // the original's sp+112
    CopyMat43W(m, o + 448);
    callf(ctx, kMatrixRotationY, {rot, o + 580}, {F(o + 580)});
    const Mat4 a = LoadMat(m);
    const Mat4 r = LoadMat(rot);
    Stvx(m, XformDir(r.r[0], r.r[1], r.r[2], a.r[0]));
    Stvx(m + 16, XformDir(r.r[0], r.r[1], r.r[2], a.r[1]));
    Stvx(m + 32, XformDir(r.r[0], r.r[1], r.r[2], a.r[2]));
    Stvx(m + 48, XformPoint(r.r[0], r.r[1], r.r[2], r.r[3], a.r[3]));
    callf(ctx, kMatrixRotationX, {rot, o + 576}, {F(o + 576)});
    call(ctx, kMatrixMultiply, {m, rot});
    const u32 n = static_cast<u32>(call(ctx, kMatrixNormalize, {m}));
    SetF1Bits(ctx, CopyMat43Last(m, n));
}

namespace {
constexpr u32 kMatrixNearlyEqual = 0x005545B4;  // bool (const f32 a[4][4], const f32 b[4][4], f1 tolerance)
constexpr u32 kSetMatTol = 0x00127ED4;          // 1e-5f
constexpr u32 kObjModePos = 0x0026D8F4;         // (Vec3* out, T* o): o's mode-dependent offset
constexpr u32 kAttachSetMatrix = 0x00261304;    // (T* a, const f32 m[4][4]) (Call_260D70_1)
constexpr u32 kCopyMat43To128 = 0x00012DE0;     // (T* this, const f32 m[4][4]): this->+128 = m (x/y/z words)
}  // namespace

// void f(Obj* o, const f32 m[4][4]) (a callback): sets o's matrix (+96)
// unless it is already within 1e-5 of m: copies m's x/y/z words, then t =
// o->vfn[16](o); with t, t gets m with its translation moved by m's
// rotation of t's mode offset (Obj_ModePos_0026D8F4) through
// Call_260D70_1(t, &copy). o->+244 (when set) gets m too
// (CopyMat43_00012DE0). r3 is the last call's result; f1 as the original
// leaves it.
DECOMP(00127ED8, Obj_SetMatrix_00127ED8) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 m = arg<u32>(ctx, 1);
    callf(ctx, kMatrixNearlyEqual, {m, o + 96, kSetMatTol}, {F(kSetMatTol)});
    if (static_cast<s32>(ctx.gpr[3]) != 0) return;
    SetF1Bits(ctx, CopyMat43Last(o + 96, m));
    const u32 t = static_cast<u32>(VCall(ctx, o, 64, {o}));
    if (t != 0) {
        Stack frame(ctx, 80);
        const u32 off = frame.addr(), copy = off + 16;  // the original's sp+112 / sp+128
        CopyMat43Last(copy, m);
        call(ctx, kObjModePos, {off, t});
        const V4 r = XformDir(Lvx(m), Lvx(m + 16), Lvx(m + 32), V4{{F(off), F(off + 4), F(off + 8), 0.0f}});
        Stvx(off, r);
        const f32 x = Adds(F(copy + 48), F(off)), y = Adds(F(copy + 52), F(off + 4)), z = Adds(F(copy + 56), F(off + 8));
        store<f32>(copy + 48, x);
        store<f32>(copy + 52, y);
        store<f32>(copy + 56, z);
        ctx.fpr[1] = x;
        call(ctx, kAttachSetMatrix, {t, copy});
    }
    const u32 a = load<u32>(o + 244);
    if (a != 0) call(ctx, kCopyMat43To128, {a, m});
}

namespace {
constexpr u32 kViewTable = 0x011950A0;         // +12: the view records (128 bytes each)
constexpr u32 kViewConst = 0x00289E90;         // 0.0f, 0.5f, 1.0f; +16 / +32 the transpose vperm masks
constexpr u32 kMatrixScaleDiag = 0x00553BC4;   // (f32 m[4][4], const f32 s[3])
constexpr u32 kMatrixLookAt = 0x0056A0D0;      // (f32 out[4][4], eye, target, up)
}  // namespace

// u8 f(Cam* c) (called by 0028A0E8): refreshes c's cached view values from
// its view record v (the table at kViewTable +12, index c->+240). Flag
// c->+456 bit 0x40: the look-at matrix of v (+16 eye, +32 target, +48 up)
// is inverted (transposed rotation, translation R^T * -t) into c->+432
// (minus the forward column) and c->+420 (the camera position); the
// scale matrix of (v+88 * c->+416, v+92 * c->+416, v+84 - v+80) is built
// and dropped, as in the original. Bit 0x80: with h = v+88 (times c->+52
// when > 0) * 0.5, w = v+80 and d = 1 / sqrt(h^2 + w^2): c->+444 = (d w)^2,
// +448 = (d h)^2, +452 = 1 / (d h). Each bit is cleared when done. r3 is
// the flag byte left; f1 as the original leaves it.
DECOMP(00289EC0, Cam_UpdateViewCache_00289EC0) {
    const u32 c = arg<u32>(ctx, 0);
    const u32 v = load<u32>(kViewTable + 12) + (load<u32>(c + 240) << 7);
    u32 b = load<u8>(c + 456);
    if ((b & 0x40) != 0) {
        Stack frame(ctx, 144);
        const u32 sp = frame.addr() - 112;  // the original's sp: +112 vector, +128 scale matrix, +192 look-at
        const f32 s = F(c + 416);
        store<f32>(sp + 112, Muls(F(v + 88), s));
        store<f32>(sp + 116, Muls(F(v + 92), s));
        store<f32>(sp + 120, Subs(F(v + 84), F(v + 80)));
        call(ctx, kMatrixScaleDiag, {sp + 128, sp + 112});
        call(ctx, kMatrixLookAt, {sp + 192, v + 16, v + 32, v + 48});
        const V4 r0 = Lvx(sp + 192), r1 = Lvx(sp + 208), r2 = Lvx(sp + 224);
        const V4 nt = Sub4(V4{{0.0f, 0.0f, 0.0f, 0.0f}}, Lvx(sp + 240));
        const V4 hi = Mrghw(r0, r2);
        const V4 col0 = Mrghw(hi, r1);
        const V4 col1 = Perm(hi, r1, Lvx(kViewConst + 16));
        const V4 col2 = Perm(Mrglw(r0, r2), r1, Lvx(kViewConst + 32));
        V4 inv;
        for (u32 k = 0; k < 4; ++k)
            inv.e[k] = Vmadd(col2.e[k], nt.e[2], Vmadd(col1.e[k], nt.e[1], Vmadd(col0.e[k], nt.e[0], 0.0f)));
        Stvx(sp + 224, col2);
        Stvx(sp + 240, inv);
        const f32 w = F(v + 80);
        for (u32 k = 0; k < 3; ++k) store<f32>(sp + 112 + 4 * k, Muls(F(sp + 224 + 4 * k), w));  // unused, as the original
        ctx.fpr[1] = F(sp + 120);
        b = load<u8>(c + 456) & ~0x40u;
        for (u32 k = 0; k < 3; ++k) store<u32>(c + 432 + 4 * k, load<u32>(sp + 224 + 4 * k) ^ 0x80000000u);  // fneg
        for (u32 k = 0; k < 3; ++k) store<u32>(c + 420 + 4 * k, load<u32>(sp + 240 + 4 * k));
        store<u8>(c + 456, static_cast<u8>(b));
    }
    if ((b & 0x80) == 0) {
        ctx.gpr[3] = b;
        return;
    }
    const f32 zero = F(kViewConst), half = F(kViewConst + 4), one = F(kViewConst + 8);
    f32 h = F(v + 88);
    const f32 a = F(c + 52);
    if (a > zero) h = Muls(h, a);
    const f32 w = F(v + 80);
    h = Muls(h, half);
    const f32 len = Sqrts(Fmadds(h, h, Muls(w, w)));
    b &= ~0x80u;
    store<u8>(c + 456, static_cast<u8>(b));
    const f32 d = Divs(one, len);
    h = Muls(h, d);
    const f32 dw = Muls(d, w);
    const f32 ih = Divs(one, h);
    const f32 sw = Muls(dw, dw);
    store<f32>(c + 444, sw);
    store<f32>(c + 448, Muls(h, h));
    store<f32>(c + 452, ih);
    ctx.fpr[1] = sw;
    ctx.gpr[3] = b;
}

namespace {
constexpr u32 kSplineGetMode = 0x005A8BA8;        // (SplineInstance*) -> mode
constexpr u32 kSplineFollowsParent = 0x005A8BE4;  // (SplineInstance*) -> bool
constexpr u32 kAcosf = 0x005E649C;                // f1 = acosf(f1)
constexpr u32 kConeConst = 0x00197680;            // 1.0f, (f64) 1.0, -1.0f, (f64) -1.0, 57.2958f (deg / rad), 45.0f
}  // namespace

// bool f(SplineInstance* s, const Query* q) (called by 001978F0): whether
// q (+0 point, +16 direction, +32 range, +36 cone test) sees s. Only mode
// 12 instances with +164 bit 0x8000; with a parent (+260) that isn't
// followed, the parent needs +164 bit 0x8000 and a +4 kind ((>> 10) & 63)
// other than 13. s's bounding sphere (+208 +48, radius +60) is moved by
// s's matrix (+80); visible when its surface is within range and, with the
// cone test, the angle between q's direction and the centre (cosine
// clamped to [-1, 1] with fsel) is at most 45 degrees. f1 as the original
// leaves it.
DECOMP(001976A8, Spline_VisibleFrom_001976A8) {
    const u32 s = arg<u32>(ctx, 0);
    const u32 q = arg<u32>(ctx, 1);
    auto no = [&]() { ret(ctx, 0u); };
    if (static_cast<s32>(call(ctx, kSplineGetMode, {s})) != 12 || ((load<u32>(s + 164) >> 15) & 1) == 0) return no();
    const u32 parent = load<u32>(s + 260);
    if (parent != 0 && static_cast<s32>(call(ctx, kSplineFollowsParent, {s})) == 0) {
        const u32 p = load<u32>(s + 260);
        if (((load<u32>(p + 164) >> 15) & 1) == 0 || ((load<u32>(p + 4) >> 10) & 0x3F) == 13) return no();
    }
    const u32 sph = load<u32>(s + 208) + 48;
    const Mat4 m = LoadMat(s + 80);
    const V4 c = XformPoint(m.r[0], m.r[1], m.r[2], m.r[3], V4{{Lvlx(sph).e[0], Lvlx(sph + 4).e[0], Lvlx(sph + 8).e[0], 0.0f}});
    const f32 dy = Subs(c.e[1], F(q + 4));
    const f32 dx = Subs(c.e[0], F(q));
    const f32 dz = Subs(c.e[2], F(q + 8));
    const f32 dist = Sqrts(Fmadds(dz, dz, Fmadds(dx, dx, Muls(dy, dy))));
    ctx.fpr[1] = dz;
    if (!(Subs(dist, F(sph + 12)) <= F(q + 32))) return no();
    if (load<u8>(q + 36) == 0) {
        ret(ctx, 1u);
        return;
    }
    const f32 one = F(kConeConst);
    const f32 inv = Divs(one, dist);
    const f64 dOne = Lfd(kConeConst + 8), dMinus = Lfd(kConeConst + 24);
    const f32 m1 = F(kConeConst + 16);
    const f32 cy = Muls(Muls(dy, inv), F(q + 20));
    const f32 cosv = Fmadds(Muls(dz, inv), F(q + 24), Fmadds(Muls(dx, inv), F(q + 16), cy));
    f32 t = static_cast<f32>(Fsel(Subs(one, cosv), cosv, dOne));
    t = static_cast<f32>(Fsel(Subs(t, m1), t, dMinus));
    t = static_cast<f32>(Fsel(Adds(t, one), t, dMinus));
    t = static_cast<f32>(Fsel(Subs(t, one), dOne, t));
    const f64 a = callf(ctx, kAcosf, {}, {t});
    const f32 deg = Muls(a, F(kConeConst + 32));
    ctx.fpr[1] = deg;
    ret(ctx, deg <= F(kConeConst + 36) ? 1u : 0u);
}

namespace {
constexpr u32 kTiltConst = 0x0040C838;      // 0.0f, -pi/4, 1.0f, 0.25f
constexpr u32 kAnimRestart = 0x002775A8;    // (anim, key, ...; f1..f4) (Anim_Restart2775A8)
constexpr u32 kTiltAnimKey = 0x04BE8B4E;
}  // namespace

// u32 f(T* o) (called by 00410E98): when (o->+80)->+464 is set: o->+84 =
// 4, +88 = 0; the matrix o->+208 is rotated by -pi/4 about x (rotation
// rows through the matrix, vmaddfp), its translation replaced by o->+336,
// and handed to (o->+4)->vfn[21](o->+4, &m). If (o->+4)->vfn[15]() is
// set, the animation it returns (asked again) is restarted with key
// kTiltAnimKey (f1 1, f2 0.25, f3 / f4 0). r3 as the original leaves it.
// The two stack argument words the original never writes (its sp+112 /
// +120) are passed through from the same guest stack slots.
DECOMP(0040C848, Obj_TiltRestart_0040C848) {
    const u32 o = arg<u32>(ctx, 0);
    if (load<u8>(load<u32>(o + 80) + 464) == 0) {
        ret(ctx, 0u);
        return;
    }
    const u32 osp = static_cast<u32>(ctx.gpr[1]) - 288;  // the original's frame
    const u64 junk8 = load<u64>(osp + 112), junk9 = load<u64>(osp + 120);
    Stack frame(ctx, 128);
    const u32 rot = frame.addr(), m = rot + 64;  // the original's sp+144 / sp+208
    store<u32>(o + 84, 4);
    store<u32>(o + 88, 0);
    const f32 zero = F(kTiltConst);
    callf(ctx, kMatrixRotationX, {rot, 0, 4, kTiltConst}, {F(kTiltConst + 4)});
    const Mat4 a = LoadMat(o + 208);
    const Mat4 r = LoadMat(rot);
    const u32 px = load<u32>(o + 336), py = load<u32>(o + 340), pz = load<u32>(o + 344);
    SetF1Bits(ctx, px);  // f1 = the lfs of o->+336, as the original leaves it
    Stvx(m, XformDir(a.r[0], a.r[1], a.r[2], r.r[0]));
    Stvx(m + 16, XformDir(a.r[0], a.r[1], a.r[2], r.r[1]));
    Stvx(m + 32, XformDir(a.r[0], a.r[1], a.r[2], r.r[2]));
    Stvx(m + 48, XformPoint(a.r[0], a.r[1], a.r[2], a.r[3], r.r[3]));
    store<u32>(m + 48, px);
    store<u32>(m + 52, py);
    store<u32>(m + 56, pz);
    const u32 owner = load<u32>(o + 4);
    VCall(ctx, owner, 84, {owner, m});
    const u32 o2 = load<u32>(o + 4);
    if (static_cast<s32>(VCall(ctx, o2, 60, {o2})) == 0) return;
    const u32 o3 = load<u32>(o + 4);
    const u64 an = VCall(ctx, o3, 60, {o3});
    callf(ctx, kAnimRestart, {an, kTiltAnimKey, 0, 0, ~0ull, 0, 0, 0, junk8, junk9, 0, ~0ull},
          {F(kTiltConst + 8), F(kTiltConst + 12), zero, zero});
}

namespace {
constexpr u32 kDirConst = 0x004AD300;  // lane masks (y, z, w), then 0.5f, 1e-5f, 1.0f at +48
}  // namespace

// void f(const Src* s, const f32 p[3], f32 out[4]) (a callback): s's
// direction for mode s->+20: 0: out = s->+64 (x, y, z, 0); 1 / 2: the
// offset s->+80 - p, when its squared length is above 0.5, normalised
// (unless every component is within 1e-5 of the zero vector) and stored as
// (x, y, z, 0), negated for mode 2. Other modes: nothing. out is written
// with stvx (out & ~15). r3 / f1 as the original leaves them.
DECOMP(004AD340, Src_Direction_004AD340) {
    const u32 s = arg<u32>(ctx, 0);
    const u32 p = arg<u32>(ctx, 1);
    const u32 out = arg<u32>(ctx, 2);
    const s32 mode = load<s32>(s + 20);
    if (mode <= 0) {
        if (mode < 0) return;
        Stvx(out, V4{{Lvlx(s + 64).e[0], Lvlx(s + 68).e[0], Lvlx(s + 72).e[0], 0.0f}});
        ctx.gpr[3] = 16;
        return;
    }
    if (mode > 2) return;
    f32 dy = Subs(F(s + 84), F(p + 4));
    f32 dx = Subs(F(s + 80), F(p));
    f32 dz = Subs(F(s + 88), F(p + 8));
    const f32 sq = Fmadds(dz, dz, Fmadds(dx, dx, Muls(dy, dy)));
    ctx.fpr[1] = dx;
    if (sq <= F(kDirConst + 48)) {
        ctx.gpr[3] = s + 80;
        return;
    }
    const f32 eps = F(kDirConst + 52);
    const bool zero = std::fabs(Subs(dx, F(kOrigin))) < eps && std::fabs(Subs(dy, F(kOrigin + 4))) < eps &&
                      std::fabs(Subs(dz, F(kOrigin + 8))) < eps;
    if (!zero) {
        const f32 inv = Divs(F(kDirConst + 56), Sqrts(sq));
        dx = Muls(dx, inv);
        dy = Muls(dy, inv);
        dz = Muls(dz, inv);
    }
    if (mode != 1) dx = -dx, dy = -dy, dz = -dz;  // fneg
    Stvx(out, V4{{dx, dy, dz, 0.0f}});
    ctx.fpr[1] = dx;
    ctx.gpr[3] = 16;
}

namespace {
constexpr u32 kRow3Perm = 0x003ACB80;  // vperm mask: x / y from a, z from b, w from a
}  // namespace

// void f(Obj* o, f1 -, f2 scale) (called by 003AD270 / 003AECB0): a =
// o->+96 fills an identity-initialised matrix through vfn[52](a, &m, <OPD>,
// 0); a's matrix (+96) then gets its translation moved by its rotation of
// m's translation * scale and is handed to a->vfn[21](a, &new). The new
// matrix's w lanes are the original's stack words (never written), which
// the frame here leaves alone at the same addresses. r3 is the last
// call's result; f1 the new translation's z.
DECOMP(003ACB90, Obj_OffsetByAttach_003ACB90) {
    const u32 o = arg<u32>(ctx, 0);
    const f64 scale = ctx.fpr[2];
    Stack frame(ctx, 192);
    const u32 sp = frame.addr() - 112;  // the original's sp (r1 - 304): +112 vector, +128 m, +192 new
    for (u32 r = 0; r < 4; ++r)
        for (u32 k = 0; k < 12; k += 4) store<u32>(sp + 128 + 16 * r + k, load<u32>(kIdentity + 16 * r + k));
    const u32 a = load<u32>(o + 96);
    const u32 opd = load<u32>(load<u32>(a) + 208);
    call_ptr(ctx, opd, {a, sp + 128, opd, 0, load<u32>(opd)});  // r7 = the entry
    const f32 tx = Muls(F(sp + 176), scale), ty = Muls(F(sp + 180), scale), tz = Muls(F(sp + 184), scale);
    const u32 a2 = load<u32>(o + 96);
    const u32 m = a2 + 96;
    for (u32 r = 0; r < 3; ++r)
        for (u32 k = 0; k < 12; k += 4) store<u32>(sp + 192 + 16 * r + k, load<u32>(m + 16 * r + k));
    store<u32>(sp + 240, load<u32>(m + 48));
    store<u32>(sp + 244, load<u32>(m + 52));
    const V4 row3 = Perm(Lvx(sp + 240), Lvlx(m + 56), Lvx(kRow3Perm));
    store<f32>(sp + 112, tx);
    store<f32>(sp + 116, ty);
    store<f32>(sp + 120, tz);
    Stvx(sp + 112, Add4(row3, XformDir(Lvx(sp + 192), Lvx(sp + 208), Lvx(sp + 224), V4{{tx, ty, tz, 0.0f}})));
    for (u32 k = 0; k < 12; k += 4) store<u32>(sp + 240 + k, load<u32>(sp + 112 + k));
    SetF1Bits(ctx, load<u32>(sp + 120));
    VCall(ctx, a2, 84, {a2, sp + 192});
}

namespace {
constexpr u32 kWorldObjUpdateLod = 0x0057EE00;  // (obj, -, s32)
constexpr u32 kSkeletonUpdate = 0x0057E8D0;     // (ApeSkeleton*)
constexpr u32 kObjFindBone = 0x000DA9D0;        // Bone* (T* o, u32 name)
constexpr u32 kBonePrimary = 0xAACCC4A2;        // the first pair member's mount bone
constexpr u32 kBoneFallback = 0x0408E092;
}  // namespace

// u32 f(T* o) (through ThunkArg1_003D35E8): for the two (source o->+88[i],
// target o->+96[i]) pairs: the source's skeleton is updated (LOD, vfn[7]
// skeleton, Skeleton_Update); its bone kBonePrimary (only for the first
// pair; else or when missing kBoneFallback) matrix times the source's world
// matrix (+96) goes to target->vfn[21](target, &m) when there is a target,
// then target->vfn[22](target, 1, 1, 1, 1). Quirk: that last call is made
// without a null check. r3 is the last call's result.
DECOMP(003D33AC, Obj_MountPairs_003D33AC) {
    const u32 o = arg<u32>(ctx, 0);
    Stack frame(ctx, 64);
    const u32 m = frame.addr();  // the original's sp+112
    for (u32 i = 0; i < 2; ++i) {
        const u32 src = load<u32>(o + 88 + 4 * i);
        const u32 dst = load<u32>(o + 96 + 4 * i);
        call(ctx, kWorldObjUpdateLod, {load<u32>(src + 160), 0, 0});
        const u32 wo = load<u32>(src + 160);
        const u64 skel = VCall(ctx, wo, 28, {wo});
        call(ctx, kSkeletonUpdate, {skel});
        u32 b = static_cast<u32>(call(ctx, kObjFindBone, {src, kBonePrimary}));
        if (b == 0 || i == 1) b = static_cast<u32>(call(ctx, kObjFindBone, {src, kBoneFallback}));
        const Mat4 w = LoadMat(src + 96);
        const Mat4 bm = LoadMat(load<u32>(b + 8));
        Stvx(m, XformDir(w.r[0], w.r[1], w.r[2], bm.r[0]));
        Stvx(m + 16, XformDir(w.r[0], w.r[1], w.r[2], bm.r[1]));
        Stvx(m + 32, XformDir(w.r[0], w.r[1], w.r[2], bm.r[2]));
        Stvx(m + 48, XformPoint(w.r[0], w.r[1], w.r[2], w.r[3], bm.r[3]));
        if (dst != 0) VCall(ctx, dst, 84, {dst, m});
        VCall(ctx, dst, 88, {dst, 1, 1, 1, 1});  // Quirk: no null check.
    }
}

namespace {
constexpr u32 kPushConst = 0x000992B0;          // vperm masks; +32 1e-5f, +36 1.0f
constexpr u32 kEffectFindComponent = 0x004BB4B4;  // (Effect*, u32 type hash) -> component or 0
constexpr u32 kWorldPlayerCall = 0x00488004;    // (World*, s32 player, const f32 v[3])
constexpr u32 kPlayerDispatch = 0x00488078;     // (World*, u32 player, const f32 v[3], u8 kind)
}  // namespace

// u32 f(const Push* p, Effect* fx) (a callback): for player p->+36 (none
// when negative): the direction p->+16..24, turned by the matrix of fx's
// p->+40 component's node (+28, matrix at +16) when that component is in
// state 5 (vfn[8]), normalised when its squared length is >= 1e-5 and
// scaled by p->+32, goes to World_PlayerCall_00488004(world, player, &v),
// or with p->+44 set Player_Dispatch_00488078(world, player, &v, p->+45).
// The matrix rows' w lanes come from the original's uninitialised stack
// and only reach an unused lane. r3 is the call's result (or p->+36).
DECOMP(000992D8, Player_PushFromEffect_000992D8) {
    const u32 p = arg<u32>(ctx, 0);
    const u32 fx = arg<u32>(ctx, 1);
    const s32 player = load<s32>(p + 36);
    if (player < 0) {
        ret(ctx, player);
        return;
    }
    f32 x = F(p + 16), y = F(p + 20), z = F(p + 24);
    const u32 c = static_cast<u32>(call(ctx, kEffectFindComponent, {fx, load<u32>(p + 40)}));
    if (c != 0 && static_cast<s32>(VCall(ctx, c, 32, {c})) == 5) {
        const u32 n = load<u32>(c + 28);
        if (n != 0) {
            const u32 m = n + 16;
            f32 v[3];
            for (u32 k = 0; k < 3; ++k)
                v[k] = Vmadd(Lvlx(m + 32 + 4 * k).e[0], z, Vmadd(Lvlx(m + 16 + 4 * k).e[0], y, Vmadd(Lvlx(m + 4 * k).e[0], x, 0.0f)));
            x = v[0], y = v[1], z = v[2];
        }
    }
    const f32 sq = Fmadds(z, z, Fmadds(x, x, Muls(y, y)));
    if (sq >= F(kPushConst + 32)) {
        const f32 inv = Divs(F(kPushConst + 36), Sqrts(sq));
        x = Muls(x, inv), y = Muls(y, inv), z = Muls(z, inv);
    }
    const f32 s = F(p + 32);
    x = Muls(x, s), y = Muls(y, s), z = Muls(z, s);
    Stack frame(ctx, 16);
    const u32 vec = frame.addr();  // the original's sp+112
    store<f32>(vec, x);
    store<f32>(vec + 4, y);
    store<f32>(vec + 8, z);
    ctx.fpr[1] = z;
    const u32 world = load<u32>(0x00F65CA0);
    if (load<u8>(p + 44) == 0) call(ctx, kWorldPlayerCall, {world, load<u32>(p + 36), vec});
    else call(ctx, kPlayerDispatch, {world, load<u32>(p + 36), vec, load<u8>(p + 45)});
}

namespace {
constexpr u32 kHkSetTransform = 0x0091EAB8;  // (hkpRigidBody*, const f32 m[4][4]) (Hk_SetTransform91EAB8)
}  // namespace

// void f(Obj* o, const f32 m[4][4]) (a callback): o's matrix (+96) = m
// (x/y/z words); with a physics world ((*0x00F65CA0)->+0x1281D4 ->+4) and
// a body (o->+416): the body gets m with w lanes 0 (built on the stack
// with vsel / vandc) through Hk_SetTransform91EAB8, and o->+1104 = the new
// matrix too. r3 is m or the call's result; f1 the last word loaded.
DECOMP(0024C530, Obj_SetMatrixBody_0024C530) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 m = arg<u32>(ctx, 1);
    SetF1Bits(ctx, CopyMat43Last(o + 96, m));
    ctx.gpr[3] = m;
    const u32 phys = load<u32>(load<u32>(0x00F65CA0) + 0x1281D4);
    if (load<u32>(phys + 4) == 0) return;
    const u32 body = load<u32>(o + 416);
    if (body == 0) return;
    Stack frame(ctx, 64);
    const u32 mat = frame.addr();  // the original's sp+112
    for (u32 r = 0; r < 4; ++r) {
        const u32 s = m + 16 * r;
        Stvx(mat + 16 * r, V4{{Lvlx(s).e[0], Lvlx(s + 4).e[0], Lvlx(s + 8).e[0], 0.0f}});
    }
    call(ctx, kHkSetTransform, {body, mat, mat});
    for (u32 r = 0; r < 4; ++r)
        for (u32 k = 0; k < 12; k += 4) store<u32>(o + 1104 + 16 * r + k, load<u32>(o + 96 + 16 * r + k));
    SetF1Bits(ctx, load<u32>(o + 148));
}

namespace {
constexpr u32 kCallK19 = 0x000D94F4;        // (o) -> o's kind-19 component or 0 (CallK2_000D94F4)
constexpr u32 kAimConst = 0x001DFC30;       // vperm masks; +32 200.0f
}  // namespace

// void f(f32 out[3], Obj* o) (called by Obj_ProjectToScreen_001DFEA0 /
// 001E1628): the aim point of o: without a kind-19 component the zero
// vector; else the point at component +208 + 160 * (+1264), through o's
// matrix (o->+424, else o->+160, at +80), plus 200 along the matrix's z
// row. The rows' w lanes come from the original's uninitialised stack and
// only reach unused lanes. f1 is left holding out[2], r3 as the original.
DECOMP(001DFC58, Obj_AimPoint_001DFC58) {
    const u32 out = arg<u32>(ctx, 0);
    const u32 o = arg<u32>(ctx, 1);
    const u32 c = static_cast<u32>(call(ctx, kCallK19, {o}));
    if (c == 0) {
        u32 w = 0;
        for (u32 k = 0; k < 12; k += 4) store<u32>(out + k, w = load<u32>(kOrigin + k));
        SetF1Bits(ctx, w);
        ctx.gpr[3] = 0x01160000;
        return;
    }
    const u32 p = c + load<u32>(c + 1264) * 160u + 208;
    const f32 px = Fw(p), py = Fw(p + 4), pz = Fw(p + 8);  // lvlx | lvrx: the unaligned words
    u32 mtx = load<u32>(o + 424);
    if (mtx == 0) mtx = load<u32>(o + 160);
    const u32 m = mtx + 80;
    auto el = [](u32 a) { return Lvlx(a).e[0]; };
    const f32 len = el(kAimConst + 32);
    u32 res[3];
    for (u32 k = 0; k < 3; ++k) {
        const f32 r0 = el(m + 4 * k), r1 = el(m + 16 + 4 * k), r2 = el(m + 32 + 4 * k), r3 = el(m + 48 + 4 * k);
        const f32 pt = r3 + Vmadd(r2, pz, Vmadd(r1, py, Vmadd(r0, px, 0.0f)));
        const f32 d = Vmadd(r2, len, Vmadd(r1, 0.0f, Vmadd(r0, 0.0f, 0.0f)));
        res[k] = std::bit_cast<u32>(pt + d);
    }
    for (u32 k = 0; k < 3; ++k) store<u32>(out + 4 * k, res[k]);
    SetF1Bits(ctx, res[2]);
    ctx.gpr[3] = 112;
}

namespace {
constexpr u32 kEffectSetHash = 0x004CF988;  // (Effect*, u32 hash) (Effect_SetHash4CF988)
constexpr u32 kResetFxHash = 0x707A34AB;
}  // namespace

// void f(Obj* o) (a callback): resets o's motion: +432 / +428 = 0, vfn[34]
// and vfn[96] get the zero vector, +436, +1168..+1176, +1184..+1192,
// +1200..+1208 and the bytes +1216 / +1217 = 0. When def (o->+216) +992 is
// not the zero vector, that direction turned by o's matrix (+96) times
// vfn[98](o) goes with o->vfn[94]()'s result (returned into a local) to
// o->vfn[97](o, &v, &r). Then +392 = 0.5f and o's effect (+356) is switched
// to kResetFxHash. r3 as the original leaves it.
DECOMP(0024DA10, Obj_ResetMotion_0024DA10) {
    const u32 o = arg<u32>(ctx, 0);
    store<u32>(o + 432, 0);
    store<u32>(o + 428, 0);
    VCall(ctx, o, 136, {o, kOrigin});
    VCall(ctx, o, 384, {o, kOrigin});
    const u32 d = load<u32>(o + 216);
    for (u32 off : {436u, 1176u, 1172u, 1168u, 1192u, 1188u, 1184u, 1208u, 1204u}) store<u32>(o + off, 0);
    store<u8>(o + 1217, 0);
    store<u32>(o + 1200, 0);
    store<u8>(o + 1216, 0);
    const u32 v = d + 992;
    if (load<u64>(v) != load<u64>(kOrigin) || load<u32>(v + 8) != load<u32>(kOrigin + 8)) {
        Stack frame(ctx, 80);
        const u32 sp = frame.addr() - 112;  // the original's sp (r1 - 192): +112 vector, +128 result, +144 direction
        const f32 x = Lvlx(v).e[0], y = Lvlx(v + 4).e[0], z = Lvlx(v + 8).e[0];
        for (u32 k = 0; k < 12; k += 4) store<u32>(sp + 144 + k, load<u32>(v + k));
        const Mat4 m = LoadMat(o + 96);
        Stvx(sp + 144, XformDir(m.r[0], m.r[1], m.r[2], V4{{x, y, z, 0.0f}}));
        const u32 opd = load<u32>(load<u32>(o) + 392);
        call_ptr(ctx, opd, {o, opd, load<u32>(opd)});
        const f64 s = ctx.fpr[1];
        const f32 tx = Muls(F(sp + 144), s), ty = Muls(F(sp + 148), s), tz = Muls(F(sp + 152), s);
        store<f32>(sp + 112, tx);
        store<f32>(sp + 116, ty);
        store<f32>(sp + 120, tz);
        ctx.fpr[1] = tz;
        const u32 opd2 = load<u32>(load<u32>(o) + 376);
        call_ptr(ctx, opd2, {sp + 128, o, opd2, load<u32>(opd2)});  // returns its value into sp+128
        VCall(ctx, o, 388, {o, sp + 112, sp + 128});
    }
    store<u32>(o + 392, 0x3F000000u);  // 0.5f
    ctx.gpr[3] = 0x3F000000u;
    const u32 fx = load<u32>(o + 356);
    if (fx != 0) call(ctx, kEffectSetHash, {fx, kResetFxHash});
}
