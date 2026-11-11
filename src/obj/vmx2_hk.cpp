// Game code, VMX sweep part 2 (see vmx2.h): Havok-side shape helpers
// (profiler-marked with the inlined timebase marker).
#include "../sly4.h"
#include "vmx2.h"

using namespace sly4;
using namespace sly4::vmx2;

namespace {
constexpr u32 kHkInvPos = 0x007D170C;      // HkTransform_InvPos7D170C (out, const hkTransform*, const Vec4* p)
constexpr u32 kHkShapeRadius = 0x0089ECA8;  // f1 (const Shape*) (name?)
constexpr u32 kHkCylDisplace = 0x004AA740;  // (body, a, b; f1 radius; out n (r7), out d (r8)) (name?)
constexpr u32 kVec3Zero2 = 0x01164B40;     // the global default Vec3 (zero)
constexpr u32 kTtCylinder = 0x00AEDCB4;     // "TtCylinder"
constexpr u32 kTtEnd = 0x00B21B48;          // the closing marker's name
constexpr u32 kCylConst = 0x004AB660;       // pi, 0.0, then the vperm masks {z x y w} / {y z x w}
constexpr u32 kHkWorldSetPos929FDC = 0x00929FDC;  // (phantom, const Vec4* pos; f1) (name?)
constexpr u32 kHkCollectorVt = 0x00F1DE30;  // the stack collector's vtable
constexpr u32 kHkAllocator = 0x011DF31C;    // the static Havok allocator object (vfn[5] frees (p, bytes))
constexpr u32 kOverlapConst = 0x00101660;   // 0.0f, then the lane masks
constexpr u32 kObjBaseInit = 0x000C3078;     // Thunk_000C3078 (T*, def)
constexpr u32 kHkShapeCinfoCtor = 0x0091FB50;  // Hk_ShapeCinfoCtor91FB50 (cinfo*)
constexpr u32 kHkSphereShapeCtor = 0x0089FE70; // Hk_SphereShapeCtor89FE70 (shape*; f1 radius)
constexpr u32 kHkBodyCtor = 0x0091E100;      // Hk_ShapeCtor91E100 (body* 544 bytes, const cinfo*)
constexpr u32 kHkRefRelease = 0x007D4EA8;    // AnimSkel_Release7D4EA8 (hkReferencedObject*)
constexpr u32 kNodePush = 0x00446984;        // Node_PushF65B7C (u32, 0) -> handle
constexpr u32 kVtSphereBody = 0x00B19148;
constexpr u32 kSphereInertia = 0x01055010;   // f32[3]: the solid-sphere inertia factors
constexpr u32 kSphereMasks = 0x00203350;     // the lane masks
constexpr u32 kCtorBase42C9D0 = 0x0042C9D0;  // Ctor_Base42C9D0 (T*, 1)
constexpr u32 kObjBaseConstruct = 0x0048F27C;  // ObjBase_Construct (Obj*)
constexpr u32 kPtrVectorConstruct = 0x005516E0;  // PtrVector_Construct (PtrVector*)
constexpr u32 kPtrVectorInit = 0x00551598;   // PtrVector_Init (PtrVector*, 8, 0, 1)
constexpr u32 kCtorB21B88 = 0x004A0DB0;      // Ctor_B21B88_004A0DB0 (T*)
constexpr u32 kObjSetFlagHash = 0x0048FA54;  // Obj_SetFlagHash_0048FA54 (obj, hash, 0)
constexpr u32 kHkPhantomCtor = 0x0092A718;   // Hk_PhantomCtor_0092A718 (phantom* 352 bytes, shape, const hkTransform*, u32 filter)
constexpr u32 kBodyAdd = 0x00931D10;         // Body_Add931D10 (world, body)
constexpr u32 kWorldPtr = 0x00F65CA0;        // World* ; +0x1281D4 the physics holder (+4 the hkpWorld)
constexpr u32 kTriggerRadius = 0x010AA8E8;   // f32: the trigger sphere's radius
constexpr u32 kIdentityXform = 0x00B2C360;   // hkTransform rotation rows (3 quadwords; translation zero)
constexpr u32 kVtTrigger = 0x00B21178;
constexpr u32 kVtTriggerBase = 0x00B21194;
constexpr u32 kTriggerFlagHash = 0x08F26945;
constexpr u32 kObjCanStep228E38 = 0x00228E38;  // bool (Obj*; f1 dt) (name?)
constexpr u32 kObjPhysPre24B064 = 0x0024B064;  // (Obj*; f1 dt) (name?)
constexpr u32 kObjPhysPost24BD80 = 0x0024BD80; // (Obj*; f1 dt) (name?)
constexpr u32 kHkActivate = 0x0091A4C8;     // Hk_Activate91A4C8 (hkpRigidBody*)
constexpr u32 kStepConst = 0x0024C1F0;      // 1e-5, 5.0 (max speed), 1.0, then the lane masks
constexpr u32 kWorldPtr4 = 0x00F65CA0;      // World* ; +0x1281D4 the physics holder (+4 the hkpWorld)
}  // namespace

// void f(Body* b, const Cylinder* c, const hkTransform* x) (called by
// 004ABC28): the cylinder's end points (c+32 / c+48) are brought into the
// transform's frame (HkTransform_InvPos7D170C), r = 0x0089ECA8(c) and
// 0x004AA740(b, &p0, &p1; f1 r; &n, &d) gives a direction n and a depth
// vector d. The impulse n * (pi * r * r * dot(d, n)) is added to b's +16
// linear vector and its moment (p - b's (+48) +320 centre) x impulse to
// the +32 angular one, p being p0 when the dot is below 0, else p1.
// Profiler markers "TtCylinder" / the closing one around it. r6 into
// 0x004AA740 is the one 0x0089ECA8 leaves (the original never sets it).
// f1 is left as the 0.0 constant and r3 as the marker cursor.
DECOMP(004AB690, HkBody_CylinderImpulse_004AB690) {
    const u32 b = arg<u32>(ctx, 0), c = arg<u32>(ctx, 1), x = arg<u32>(ctx, 2);
    ProfMark(ctx, kTtCylinder);
    Stack frame(ctx, 240);
    const u32 sp = frame.addr(), p0 = sp + 112, p1 = sp + 128, d = sp + 144, n = sp + 160, t = sp + 176;
    call(ctx, kHkInvPos, {p0, x, c + 32});
    call(ctx, kHkInvPos, {p1, x, c + 48});
    call(ctx, kHkShapeRadius, {c});
    const f64 r = ctx.fpr[1];
    call(ctx, kHkCylDisplace, {b, p0, p1, ctx.gpr[6], n, d});
    const V4 nv = Lvx(n);
    const V4 dn = Madd4(Lvx(d), nv, Zero4());
    Stvx(t, Add4(Add4(Splat(dn, 0), Splat(dn, 1)), Splat(dn, 2)));
    const f32 dot = F(t);
    const f32 s = Muls(Muls(Muls(r, F(kCylConst)), r), dot);
    Stvx(t, Fill4(s));
    const V4 zxy = Lvx(kCylConst + 16), yzx = Lvx(kCylConst + 32);
    const V4 imp = Madd4(nv, Lvx(t), Zero4());
    const V4 ia = Perm(imp, imp, zxy), ib = Perm(imp, imp, yzx);
    const u32 p = dot < F(kCylConst + 4) ? p0 : p1;
    const V4 arm = Sub4(Lvx(p), Lvx(load<u32>(b + 48) + 320));
    const V4 m = Nmsub4(Perm(arm, arm, zxy), ib, Madd4(Perm(arm, arm, yzx), ia, Zero4()));
    Stvx(b + 16, Add4(Lvx(b + 16), imp));
    Stvx(b + 32, Add4(Lvx(b + 32), m));
    SetF1Bits(ctx, load<u32>(kCylConst + 4));
    ctx.gpr[3] = ProfMark(ctx, kTtEnd);
}

// bool f(T* o; f32 r) (called by 00101CC0): whether anything other than
// o's own two bodies (+20 / +24) overlaps the phantom o->+1028. False when
// r <= 0.0 (a NaN r goes on), without the +24 body, or when the phantom's +8 is clear. Else the
// phantom is moved to the body's +288 position ({x, y, z, 0}, vsel masks
// after the 0.0 constant; 0x00929FDC is entered with f1 = 0.0 and f2 = r,
// as the original leaves them), its overlaps are collected into a stack
// array (collector {vt, u8 0, data -> inline storage, count 0, capacity
// 0x80000010}) by phantom->vfn[19](phantom, &collector, 0), and each pair
// whose two entries' owners (entry +24 type 1: entry + s8 +16) are not
// both null and are neither of o's bodies is counted. A heap-grown array
// (capacity top bit clear) is freed through the static Havok allocator's
// (0x011DF31C) vfn[5]
// (array, capacity * 16). r3 = (count != 0); f1 is left as 0.0.
DECOMP(001016A0, Hk_PhantomOverlapsOther_001016A0) {
    const u32 o = arg<u32>(ctx, 0);
    const f64 r = ctx.fpr[1];
    const f64 zero = std::bit_cast<f64>(LfsBits(load<u32>(kOverlapConst)));
    ctx.fpr[2] = r;
    ctx.fpr[1] = zero;
    ctx.gpr[3] = 0;
    if (r <= zero) return;
    const u32 body = load<u32>(o + 24);
    if (body == 0) return;
    const u32 ph = load<u32>(o + 1028);
    if (load<u32>(ph + 8) == 0) return;
    Stack frame(ctx, 448);
    const u32 pos = frame.addr() + 112, col = frame.addr() + 128;
    const Masks3 k = LoadMasks3(kOverlapConst + 16);
    Stvx(pos, RowSelW(body + 288, body + 292, body + 296, k));
    callf(ctx, kHkWorldSetPos929FDC, {ph, pos}, {zero, r});
    store<u8>(col + 4, 0);
    store<u32>(col + 8, col + 20);
    store<u32>(col, kHkCollectorVt);
    store<u32>(col + 16, 0x80000010u);
    store<u32>(col + 12, 0);
    const u32 ph2 = load<u32>(o + 1028);
    VCall(ctx, ph2, 76, {ph2, col, 0});
    const s32 n = static_cast<s32>(load<u32>(col + 12));
    const u32 arr = load<u32>(col + 8);
    const u32 a = load<u32>(o + 20), b = load<u32>(o + 24);
    const u32 cap = load<u32>(col + 16);
    auto owner = [](u32 e) { return static_cast<s8>(load<u8>(e + 24)) == 1 ? e + static_cast<u32>(static_cast<s32>(static_cast<s8>(load<u8>(e + 16)))) : 0u; };
    u32 count = 0;
    for (s32 i = 0; i < n; ++i) {
        const u32 e = arr + 16 * static_cast<u32>(i);
        const u32 p = owner(load<u32>(e)), q = owner(load<u32>(e + 8));
        if ((q == 0 && p == 0) || q == a || p == a || q == b || p == b) continue;
        ++count;
    }
    if ((cap & 0x80000000u) == 0) {
        store<u32>(col + 12, 0);
        store<u32>(col, kHkCollectorVt);
        VCall(ctx, kHkAllocator, 20, {kHkAllocator, arr, static_cast<u64>(static_cast<s64>(static_cast<s32>(cap << 4)))});
    }
    ctx.gpr[3] = count != 0 ? 1u : 0u;
}

// void f(T* o, const XmlDef* d) (called by XmlDef_New203BE4): constructs
// a sphere rigid-body object. Base init (Thunk_000C3078(o, d)), vtable
// 0x00B19148, +192 (u8) / +194 (u16) / +196 / +208 from d +124 / +128 /
// +132 / +168. A Havok cinfo is built on the stack (Hk_ShapeCinfoCtor
// first): the sphere shape (HkAlloc(32) + Hk_SphereShapeCtor89FE70(s; f1
// radius d+136), null when the allocation failed) at +4, motion type 6
// (+0 and the byte +200), the identity 3x3 at +80 (stvx zero, then the
// 1.0 diagonal), the inertia diagonal {k0 r, k1 r, k2 r, 0} at +128 (the
// three factors at 0x01055010 each splatted through the stack, vsel /
// vandc), mass d+140 at +144, d+156 / +160 / +152 / +148 / +144 at +148 /
// +152 / +156 / +160 / +168, bytes +180 = 1 and +204 = 1. The body
// (HkAlloc(544) + Hk_ShapeCtor91E100(b, &cinfo), else null) goes to +200,
// the shape reference is released, and +204 = Node_PushF65B7C(d+164, 0).
// f1 is left as k2 * r (the original's last float result); r3 the push's.
DECOMP(00203380, SphereBody_Construct_00203380) {
    const u32 o = arg<u32>(ctx, 0), d = arg<u32>(ctx, 1);
    call(ctx, kObjBaseInit, {o, d});
    store<u8>(o + 192, static_cast<u8>(load<u32>(d + 124)));
    store<u32>(o, kVtSphereBody);
    store<u16>(o + 194, static_cast<u16>(load<u32>(d + 128)));
    CopyW(o + 196, d + 132);
    store<u32>(o + 208, load<u32>(d + 168));
    Stack frame(ctx, 384);
    const u32 sp = frame.addr(), ci = sp + 112, tmp = sp + 320;
    call(ctx, kHkShapeCinfoCtor, {ci});
    const u32 shape = HkAlloc(ctx, 32);
    if (shape != 0) callf(ctx, kHkSphereShapeCtor, {shape}, {std::bit_cast<f64>(LfsBits(load<u32>(d + 136)))});
    store<u32>(ci + 4, shape);
    CopyW(ci + 144, d + 140);
    const f64 r = F(d + 136);
    const f32 i0 = Muls(F(kSphereInertia), r), i1 = Muls(F(kSphereInertia + 4), r), i2 = Muls(F(kSphereInertia + 8), r);
    const Masks3 k = LoadMasks3(kSphereMasks);
    Stvx(tmp, Fill4(i0));
    const V4 a = Lvx(tmp);
    Stvx(tmp, Fill4(i1));
    const V4 b = Lvx(tmp);
    Stvx(tmp, Fill4(i2));
    const V4 c = Lvx(tmp);
    CopyW(ci + 168, d + 144);
    CopyW(ci + 160, d + 148);
    CopyW(ci + 156, d + 152);
    CopyW(ci + 148, d + 156);
    CopyW(ci + 152, d + 160);
    store<u8>(ci + 180, 1);
    for (u32 r3 = 0; r3 < 3; ++r3) Stvx(ci + 80 + 16 * r3, Zero4());
    Stvx(ci + 128, RowSel(a, b, c, k));
    store<u32>(ci + 80, 0x3F800000u);
    store<u32>(ci + 100, 0x3F800000u);
    store<u32>(ci + 120, 0x3F800000u);
    store<u32>(ci, 6);
    store<u8>(ci + 204, 1);
    store<u8>(ci + 200, 6);
    ctx.fpr[1] = i2;
    const u32 body = HkAlloc(ctx, 544);
    if (body != 0) call(ctx, kHkBodyCtor, {body, ci});
    store<u32>(o + 200, body);
    call(ctx, kHkRefRelease, {load<u32>(ci + 4)});
    store<u32>(o + 204, static_cast<u32>(call(ctx, kNodePush, {load<u32>(d + 164), 0})));
}

// void f(T* o, u32 a, u8 b, u8 c) (called by Cmd_Push4360CC_000A3608):
// constructs a sphere trigger phantom (160 bytes). Bases: Ctor_Base42C9D0
// (o, 1), ObjBase_Construct(o+12), vtables 0x00B21178 / 0x00B21194, +20 =
// a, +24 / +28 = 0, a PtrVector at +32 (u16 +38 = 40, Init(8, 0, 1)),
// Ctor_B21B88 at +52, +96 / +112 the zero vector, +128..+140 = 0, bytes
// +144 / +145 = 0, +146 = b, +147 = c, Obj_SetFlagHash(o+12, 0x08F26945,
// 0). Then a sphere shape (HkAlloc(32) + Hk_SphereShapeCtor89FE70, radius
// 0x010AA8E8) and a phantom (HkAlloc(352) + Hk_PhantomCtor_0092A718(p,
// shape, &identity transform on the stack, 4); null when an allocation
// failed) at +28, the shape reference released and the phantom added to
// the world (Body_Add931D10(world->+0x1281D4->+4, phantom)). f1 is left
// as the radius (or the zero vector's z without a shape); r3 as Body_Add's.
DECOMP(004360CC, Trigger_Construct_004360CC) {
    const u32 o = arg<u32>(ctx, 0), a = arg<u32>(ctx, 1), b = arg<u32>(ctx, 2), c = arg<u32>(ctx, 3);
    call(ctx, kCtorBase42C9D0, {o, 1});
    call(ctx, kObjBaseConstruct, {o + 12});
    store<u32>(o, kVtTrigger);
    store<u32>(o + 12, kVtTriggerBase);
    store<u32>(o + 20, a);
    store<u32>(o + 24, 0);
    store<u32>(o + 28, 0);
    call(ctx, kPtrVectorConstruct, {o + 32});
    store<u16>(o + 38, 40);
    call(ctx, kPtrVectorInit, {o + 32, 8, 0, 1});
    call(ctx, kCtorB21B88, {o + 52});
    for (u32 k = 0; k < 3; ++k) CopyW(o + 96 + 4 * k, kVec3Zero2 + 4 * k);
    CopyW(o + 112, kVec3Zero2);
    CopyW(o + 116, kVec3Zero2 + 4);
    store<u32>(o + 128, 0);
    CopyW(o + 120, kVec3Zero2 + 8);
    for (u32 off : {132u, 136u, 140u}) store<u32>(o + off, 0);
    store<u8>(o + 144, 0);
    store<u8>(o + 145, 0);
    store<u8>(o + 146, static_cast<u8>(b));
    store<u8>(o + 147, static_cast<u8>(c));
    SetF1Bits(ctx, load<u32>(kVec3Zero2 + 8));
    call(ctx, kObjSetFlagHash, {o + 12, kTriggerFlagHash, 0});
    const u32 shape = HkAlloc(ctx, 32);
    if (shape != 0) callf(ctx, kHkSphereShapeCtor, {shape}, {std::bit_cast<f64>(LfsBits(load<u32>(kTriggerRadius)))});
    Stack frame(ctx, 240);
    const u32 xf = frame.addr() + 112;
    for (u32 k = 0; k < 3; ++k) Stvx(xf + 16 * k, Lvx(kIdentityXform + 16 * k));
    Stvx(xf + 48, Zero4());
    const u32 ph = HkAlloc(ctx, 352);
    if (ph != 0) call(ctx, kHkPhantomCtor, {ph, shape, xf, 4});
    store<u32>(o + 28, ph);
    call(ctx, kHkRefRelease, {shape});
    call(ctx, kBodyAdd, {load<u32>(load<u32>(load<u32>(kWorldPtr) + 0x1281D4u) + 4), load<u32>(o + 28)});
}

// bool f(Obj* o; f32 dt) (a callback): o's physics step. False when
// 0x00228E38(o; dt) refuses. With a live hkpWorld: 0x0024B064(o; dt);
// with a body (+416) the tracked position +1184 (previous kept at +1168)
// follows the body's +432 position, but a move faster than 5.0 / s
// (|d| / dt) is clamped: pos = prev + d * 5 * (1 / speed) (fmadds), and
// the body's velocity is set to that {x, y, z, 0} (Hk_Activate91A4C8,
// then (body+224)->vfn[17](body+224, &v)); then 0x0024BD80(o; dt). Last:
// o's 4x3 matrix (+96) is copied to +1104, the input vector +1232..+1240
// goes to +1248 (normalised unless its squared length's magnitude is below
// 1e-5) and is cleared. r3 1; f1 as the original leaves it.
DECOMP(0024C230, Obj_PhysStep_0024C230) {
    const u32 o = arg<u32>(ctx, 0);
    const f64 dt = ctx.fpr[1];
    if (static_cast<u32>(call(ctx, kObjCanStep228E38, {o})) == 0) {
        ctx.gpr[3] = 0;
        return;
    }
    if (load<u32>(load<u32>(load<u32>(kWorldPtr4) + 0x1281D4u) + 4) != 0) {
        ctx.fpr[1] = dt;
        call(ctx, kObjPhysPre24B064, {o});
        const u32 body = load<u32>(o + 416);
        if (body != 0) {
            const f32 px = F(o + 1184), py = F(o + 1188), pz = F(o + 1192);
            for (u32 k = 0; k < 3; ++k) CopyW(o + 1168 + 4 * k, o + 1184 + 4 * k);
            const f32 cy = F(body + 436), cx = F(body + 432);
            f32 dy = Subs(cy, py), dx = Subs(cx, px);
            const f32 cz = F(body + 440);
            const f32 dz = Subs(cz, pz);
            for (u32 k = 0; k < 3; ++k) CopyW(o + 1184 + 4 * k, body + 432 + 4 * k);
            const f32 vmax = F(kStepConst + 4);
            const f32 speed = Divs(Sqrts(Fmadds(dz, dz, Fmadds(dx, dx, Muls(dy, dy)))), dt);
            if (speed > vmax) {
                dx = Muls(dx, vmax);
                dy = Muls(dy, vmax);
                const f32 inv = Divs(F(kStepConst + 8), speed);
                const f32 nz = Muls(dz, vmax);
                store<f32>(o + 1184, Fmadds(dx, inv, px));
                store<f32>(o + 1188, Fmadds(dy, inv, py));
                store<f32>(o + 1192, Fmadds(nz, inv, pz));
                Stack frame(ctx, 160);
                const u32 v = frame.addr() + 112;
                Stvx(v, RowSelW(o + 1184, o + 1188, o + 1192, LoadMasks3(kStepConst + 16)));
                call(ctx, kHkActivate, {body});
                VCall(ctx, body + 224, 68, {body + 224, v});
            }
        }
        ctx.fpr[1] = dt;
        call(ctx, kObjPhysPost24BD80, {o});
    }
    const f32 iy = F(o + 1236), ix = F(o + 1232), iz = F(o + 1240);
    for (u32 r = 0; r < 4; ++r)
        for (u32 k = 0; k < 3; ++k) CopyW(o + 1104 + 16 * r + 4 * k, o + 96 + 16 * r + 4 * k);
    const f32 l2 = Fmadds(iz, iz, Fmadds(ix, ix, Muls(iy, iy)));
    store<f32>(o + 1248, ix);
    store<f32>(o + 1252, iy);
    store<f32>(o + 1256, iz);
    ctx.fpr[1] = l2;
    if (!(std::fabs(l2) < F(kStepConst))) {
        const f32 inv = Divs(F(kStepConst + 8), Sqrts(l2));
        const f32 rz = Muls(iz, inv);
        store<f32>(o + 1248, Muls(ix, inv));
        store<f32>(o + 1252, Muls(iy, inv));
        store<f32>(o + 1256, rz);
        ctx.fpr[1] = rz;
    }
    store<u32>(o + 1240, 0);
    store<u32>(o + 1236, 0);
    store<u32>(o + 1232, 0);
    ctx.gpr[3] = 1;
}
