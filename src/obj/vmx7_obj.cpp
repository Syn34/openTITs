// Game code, VMX sweep part 7 (see vmx7.h): object set-up methods whose
// only VMX use is clearing stack buffers (stvx of a zero vector).
#include "../sly4.h"
#include "vmx7.h"

using namespace sly4;
using namespace sly4::vmx7;

namespace {
constexpr u32 kObj_FindByHandle = 0x0042C2B0;  // (u32 handle, u8 unused) -> object (reads r3 only)
constexpr u32 kFlag_Set8_40000000 = 0x0042CA60;  // (Obj* o): o->+8 |= 0x40000000
constexpr u32 kStrTimeMachine = 0x00AE259C;  // "TimeMachine"
constexpr u32 kStrSly = 0x00AE25AC;  // "Sly"
constexpr u32 kStrMurray = 0x00AE25B4;  // "Murray"

// One named pair: buf (stack, 40 bytes) = {char name[32] zeroed, a, b}
// with a / b = Obj_FindByHandle(h1 / h2, 0); when both are found the name
// is assigned, each one's u16 +178 count is bumped, and the record is
// pushed onto o's 40-byte-element vector +32 (data, u16 capacity +36,
// count +40, high-water +42; PtrVector_Reserve(.., capacity * 2) when
// full).
void PushNamedPair(PPUContext& ctx, u32 o, u32 buf, u32 h1, u32 h2, u32 name) {
    store<u32>(buf + 32, 0);
    store<u32>(buf + 36, 0);
    Stvx(buf, Zero4());
    Stvx(buf + 16, Zero4());
    store<u32>(buf + 32, static_cast<u32>(call(ctx, kObj_FindByHandle, {h1, 0})));
    const u32 b = static_cast<u32>(call(ctx, kObj_FindByHandle, {h2, 0}));
    store<u32>(buf + 36, b);
    if (load<u32>(buf + 32) == 0 || b == 0) return;
    call(ctx, v5fn::String_Assign, {buf, name});
    const u32 pa = load<u32>(buf + 32), pb = load<u32>(buf + 36);
    store<u16>(pa + 178, static_cast<u16>(load<u16>(pa + 178) + 1));
    store<u16>(pb + 178, static_cast<u16>(load<u16>(pb + 178) + 1));
    const u16 cap = load<u16>(o + 36);
    if (load<u16>(o + 40) == cap) call(ctx, v5fn::PtrVector_Reserve, {o + 32, (static_cast<u32>(cap) << 1) & 0x1FFFE});
    const u32 e = load<u32>(o + 32) + 40 * load<u16>(o + 40);
    for (u32 k = 0; k < 10; ++k) store<u32>(e + 4 * k, load<u32>(buf + 4 * k));
    const u32 n = load<u16>(o + 40) + 1u;
    const u16 hw = load<u16>(o + 42);
    store<u16>(o + 40, static_cast<u16>(n));
    store<u16>(o + 42, hw > (n & 0xFFFF) ? hw : static_cast<u16>(n));
}
}  // namespace

// void f(T* o) (called through a pointer): pushes the named pairs
// "TimeMachine" (handles 0x8FA762B7 / 0x8FA762B8), "Sly" (0xAD5DF30C /
// 0xAD5DF30D) and "Murray" (0x60A2AA0C / 0x60A2AA0D) (see PushNamedPair),
// then Flag_Set8_40000000(o).
DECOMP(004365E4, Pairs_AddActors_004365E4) {
    const u32 o = arg<u32>(ctx, 0);
    Stack frame(ctx, 272);
    const u32 sp = static_cast<u32>(ctx.gpr[1]);
    PushNamedPair(ctx, o, sp + 208, 0x8FA762B7, 0x8FA762B8, kStrTimeMachine);
    PushNamedPair(ctx, o, sp + 160, 0xAD5DF30C, 0xAD5DF30D, kStrSly);
    PushNamedPair(ctx, o, sp + 112, 0x60A2AA0C, 0x60A2AA0D, kStrMurray);
    call(ctx, kFlag_Set8_40000000, {o});
}

namespace {
constexpr u32 kArray_Reserve = 0x00558F3C;  // (Array*, u32 capacity)
constexpr u32 kHk_GetBody9D19C8 = 0x009D19C8;  // (x) -> body (reads r3 only)
constexpr u32 kHk_PhantomCtor = 0x0092A718;  // (hkpPhantom*, shape, const cinfo*, u32)
constexpr u32 kAnimSkel_Release = 0x007D4EA8;  // (hkReferencedObject*): removeReference
constexpr u32 kBody_Add931D10 = 0x00931D10;  // (world, entity)
constexpr u32 kHk_Ctor9256C0 = 0x009256C0;  // (p, const T* info, u32) (name?)
constexpr u32 kPhantomCompList = 0x01056F1C;  // {u16 capacity, count, high-water, flags; {u32 key, T* comp}* data (+8)}
constexpr u32 kPhantomCompVt0 = 0x00B1BE30;  // the base class vtable
constexpr u32 kPhantomCompVt = 0x00B1D158;  // the class vtable
constexpr u32 kPhantomCompVt24 = 0x00B1D1A8;  // the secondary vtable (+24)
constexpr u32 kPhantomCinfo = 0x00B2C360;  // 48 bytes copied (lvx / stvx) into the phantom's construction info
constexpr u32 kPhantomZeroF = 0x0025297C;  // f32 0 (f1 of the body's vfn[5])
}  // namespace

// void f(PhantomComp* o, const Desc* d) (from Comp_Create2635E8): the base
// part (vtable, d's +4 / +8 / +12, +16 = 1, +20 = 0; {d+4, o} appended to
// the global list kPhantomCompList, its flags |= 1), then the class part:
// d's +20.. fields copied (+28..+48, +64..+72, +80..+88, +96..+104, +108 =
// d+96, +112 = d+92 the radius), vtables, +116 / +120 / +256 = 0, the four
// vectors +272.. = zero, +345 / +346 = 0, +348 = -1.0, +352 / +356 = 0,
// +360 = 0x80000000, +376 / +380 = 0. Then with the body of the owner's
// vfn[16]()->+32 (Hk_GetBody9D19C8, or none): a sphere shape of radius +112
// (HkAlloc(32)) and a 352-byte phantom on it (+116, the cinfo built on the
// stack) added to the body's world (+8), the shape released; the body's
// +16 object vfn[5](it, body + 240, -, &info; f1 0) fills a stack block
// that a 208-byte object (kHk_Ctor9256C0) is built from (+120), added too.
DECOMP(00252980, PhantomComp_Ctor_00252980) {
    const u32 o = arg<u32>(ctx, 0), d = arg<u32>(ctx, 1);
    Stack frame(ctx, 272);
    const u32 sp = static_cast<u32>(ctx.gpr[1]);
    const u32 G = kPhantomCompList;
    store<u32>(o, kPhantomCompVt0);
    const u32 d4 = load<u32>(d + 4);
    store<u32>(o + 4, d4);
    const u32 d8 = load<u32>(d + 8);
    u16 cnt = load<u16>(G + 2);
    store<u32>(o + 8, d8);
    const u32 d12 = load<u32>(d + 12);
    store<u8>(o + 16, 1);
    store<u32>(o + 12, d12);
    store<u32>(o + 20, 0);
    const u16 cap = load<u16>(G);
    u32 key = d4;
    if (cnt == cap) {
        call(ctx, kArray_Reserve, {G, (static_cast<u32>(cap) << 1) & 0xFFFE});
        key = load<u32>(o + 4);
        cnt = load<u16>(G + 2);
    }
    const u32 data = load<u32>(G + 8);
    store<u32>(data + ((static_cast<u32>(cnt) << 3) & 0x7FFF8), key);
    store<u32>(data + (static_cast<u32>(load<u16>(G + 2)) << 3) + 4, o);
    const u32 n = load<u16>(G + 2) + 1u;
    const u16 hw = load<u16>(G + 4), flags = load<u16>(G + 6);
    store<u16>(G + 2, static_cast<u16>(n));
    const u32 nhw = hw > (n & 0xFFFF) ? hw : n;
    store<u16>(G + 6, static_cast<u16>(flags | 1));
    CopyW6(o + 28, d + 20, 6);
    CopyW6(o + 64, d + 48, 3);
    const u32 d64 = load<u32>(d + 64);
    store<u16>(G + 4, static_cast<u16>(nhw));
    store<u32>(o + 80, d64);
    const u32 d68 = load<u32>(d + 68);
    store<u32>(o, kPhantomCompVt);
    store<u32>(o + 84, d68);
    const u32 d72 = load<u32>(d + 72);
    store<u32>(o + 24, kPhantomCompVt24);
    store<u32>(o + 88, d72);
    CopyW6(o + 96, d + 80, 3);
    CopyW6(o + 108, d + 96, 1);
    CopyW6(o + 112, d + 92, 1);
    store<u32>(o + 116, 0);
    store<u32>(o + 120, 0);
    store<u32>(o + 256, 0);
    for (u32 r = 0; r < 3; ++r) CopyW6(o + 272 + 16 * r, kV3Zero3, 3);
    CopyW6(o + 320, kV3Zero3, 1);
    store<u8>(o + 345, 0);
    CopyW6(o + 324, kV3Zero3 + 4, 1);
    store<u8>(o + 346, 0);
    CopyW6(o + 328, kV3Zero3 + 8, 1);
    store<u32>(o + 348, 0xBF800000);
    store<u32>(o + 352, 0);
    const u32 own = load<u32>(o + 8);
    store<u32>(o + 356, 0);
    store<u32>(o + 360, 0x80000000);
    store<u8>(o + 376, 0);
    store<u32>(o + 380, 0);
    const u32 x = static_cast<u32>(VCall(ctx, own, 64, {own}));
    u32 body = 0;
    if (load<u32>(x + 32) != 0) body = static_cast<u32>(call(ctx, kHk_GetBody9D19C8, {load<u32>(x + 32)}));
    const u32 world = load<u32>(body + 8), b44 = load<u32>(body + 44);  // read through a null body too
    for (u32 k = 0; k < 3; ++k) Stvx(sp + 144 + 16 * k, Lvx(kPhantomCinfo + 16 * k));
    Stvx(sp + 192, Zero4());
    const u32 sh = HkAlloc(ctx, 32);
    u32 shape = 0;
    if (sh != 0) {
        callf(ctx, v5fn::HkSphereShape_Ctor, {sh}, {Lfs4(o + 112)});
        shape = sh;
    }
    const u32 ph = HkAlloc(ctx, 352);
    u32 phantom = 0;
    if (ph != 0) {
        call(ctx, kHk_PhantomCtor, {ph, shape, sp + 144, b44});
        phantom = ph;
    }
    store<u32>(o + 116, phantom);
    call(ctx, kAnimSkel_Release, {shape});
    call(ctx, kBody_Add931D10, {world, load<u32>(o + 116)});
    const u32 x2 = load<u32>(body + 16);
    const u32 opd = load<u32>(load<u32>(x2) + 20);
    ctx.fpr[1] = Lfs4(kPhantomZeroF);
    call_ptr(ctx, opd, {x2, body + 240, opd, sp + 112, load<u32>(opd)});  // r5 = the OPD, r7 = its entry, as the original leaves them
    const u32 s = HkAlloc(ctx, 208);
    u32 obj = 0;
    if (s != 0) {
        call(ctx, kHk_Ctor9256C0, {s, sp + 112, b44});
        obj = s;
    }
    store<u32>(o + 120, obj);
    call(ctx, kBody_Add931D10, {world, obj});
}

namespace {
constexpr u32 kMotion_Set = 0x004A0C20;  // (Motion*; f32 limit) -> f32
constexpr u32 kAnim_Restart = 0x002775A8;  // (Animator*, layer, key, u8, -, -, p6, p7, [112] -, [120] -, [128], [136]; f1..f4) -> Track*
constexpr u32 kAnim_SetTarget = 0x0030DBF0;  // (T* o, u32 key, -, u8 force; f32 t)
constexpr u32 kClimb_Fn308F5C = 0x00308F5C;  // (x, 0) (name?)
constexpr u32 kClimb_Fn30D434 = 0x0030D434;  // (T* t, u32 t->+1060, f32 out[4][4]) (name?)
constexpr u32 kClimbK = 0x00312D64;  // f32 {0, 0.75, 1, 0.5}
constexpr u32 kClimbG80 = 0x01069AF0;  // +72 f32: o->+80 when it is not positive
constexpr u32 kClimbG104 = 0x01069B40;  // +72 f32: o->+104 / +108 / +112
constexpr u32 kClimbOffset = 0x01069AE0;  // f32[3]: the local offset
constexpr u32 kClimbStep = 0x01069E10;  // +72 f32: the step length
constexpr u32 kClimbAnimScale = 0x01068FC0;  // +72 f32: the animation speed
constexpr u32 kClimbAnimFlag = 0x01068F70;  // +72 f32: non-zero sets the restart's f2
constexpr u32 kClimbAnimKey = 0xE8534CE4;  // the layer key (t->+668 once started)
constexpr u32 kClimbTargetKey = 0xCB1675F3;
}  // namespace

// void f(State* o) (called through a pointer): a state's entry. o->+28
// += 1, +36 = 0, +80 = the global default when not positive, +120 = 0, +88
// = 0, +104 / +108 / +112 = the global value, Motion_Set(o + 84; 0). With
// the owner t = o->+32: its matrix (kClimb_Fn30D434 into an identity-
// initialised stack matrix S) gives P = S x kClimbOffset + S's
// translation (vmaddfp); the horizontal direction d from P to t's
// position (+448 / +456) is normalised unless it equals the zero vector
// bit for bit; o->+64.. = P (y from t->+452) + d * step (the global step,
// or 0.75 of the distance when shorter), copied to (t->+872)->+32.. after
// kClimb_Fn308F5C(t->+872, 0). Unless t->+668 is already the layer key:
// Anim_Restart(t->vfn[15](t), key, 0, 0, -1, -, 0, 0, -, -, 0, -1; f1 the
// global speed, f2 1 / 0 for the global flag (0 when t->+700 is set), 0,
// 0), t->+700 = 0, +668 = the key. Then t->+1048 = 1.0, +1044 = +1052,
// +1028 = 0, Motion_Set(t + 1024; (1 - +1052) * 0.5),
// Anim_SetTarget(t, 0xCB1675F3, -, 0; 0) and o->vfn[7](o, 0, 1).
DECOMP(00312D78, ClimbState_Enter_00312D78) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 K = kClimbK;
    Stack frame(ctx, 320);
    const u32 sp = static_cast<u32>(ctx.gpr[1]);
    const u32 c28 = load<u32>(o + 28);
    const f64 v80 = Lfs4(o + 80);
    store<u32>(o + 28, c28 + 1);
    const f64 k0 = Lfs4(K);
    store<u32>(o + 36, 0);
    if (v80 <= k0) store<u32>(o + 80, load<u32>(kClimbG80 + 72));
    store<u8>(o + 120, 0);
    const u32 w = load<u32>(kClimbG104 + 72);
    store<u32>(o + 88, 0);
    store<u32>(o + 112, w);
    store<u32>(o + 104, w);
    store<u32>(o + 108, w);
    callf(ctx, kMotion_Set, {o + 84}, {k0});
    const u32 t = load<u32>(o + 32);
    const u32 S = sp + 176;
    CopyRowsXyz7(S, kAttachIdentity, 4);
    call(ctx, kClimb_Fn30D434, {t, load<u32>(t + 1060), S});
    const V4 s0 = Lvx(S), s1 = Lvx(S + 16), s2 = Lvx(S + 32), s3 = Lvx(S + 48);
    const u32 G = kClimbOffset;
    Stvx(sp + 144, Add4(s3, Madd4(s2, big7::Splat1(F(G + 8)), Madd4(s1, big7::Splat1(F(G + 4)), Mul0(s0, big7::Splat1(F(G)))))));
    const u32 t2 = load<u32>(o + 32);
    const f64 px = Lfs4(sp + 144), pz = Lfs4(sp + 152);
    const f32 dx = Subs(Lfs4(t + 448), px);
    const f32 dz = Subs(Lfs4(t + 456), pz);
    const f64 ty = Lfs4(t + 452);
    // the 12-byte compare of {dx, 0, dz} with the zero vector (two 64-bit / 32-bit subtracts)
    const bool same = std::bit_cast<u32>(dx) == load<u32>(kV3Zero3) && load<u32>(kV3Zero3 + 4) == 0 && std::bit_cast<u32>(dz) == load<u32>(kV3Zero3 + 8);
    f64 nx = dx, ny = k0, nz = dz, len = k0;
    if (!same) {
        len = Sqrts(Fmadds(dz, dz, Fmadds(dx, dx, Muls(k0, k0))));
        const f32 s = Divs(Lfs4(K + 8), len);
        nx = Muls(dx, s);
        ny = Muls(k0, s);
        nz = Muls(dz, s);
    }
    f64 step = Lfs4(kClimbStep + 72);
    if (len < step) step = Muls(len, Lfs4(K + 4));
    store<f32>(o + 64, Fmadds(nx, step, px));
    store<f32>(o + 68, Fmadds(ny, step, ty));
    store<f32>(o + 72, Fmadds(nz, step, pz));
    call(ctx, kClimb_Fn308F5C, {load<u32>(t2 + 872), 0});
    const u32 t3 = load<u32>(o + 32);
    const u32 q = load<u32>(t3 + 872);
    CopyW6(q + 32, o + 64, 3);
    const f64 one = Lfs4(K + 8);
    const f32 speed = Muls(one, Lfs4(kClimbAnimScale + 72));
    u32 t4 = t3;
    if (load<u32>(t3 + 668) != kClimbAnimKey) {
        f64 g = k0;
        if (load<u8>(t3 + 700) == 0) g = Lfs4(kClimbAnimFlag + 72);
        const f64 flag = g != k0 ? 1.0 : 0.0;
        const u32 tr = static_cast<u32>(VCall(ctx, t3, 60, {t3}));
        // r8 and the stack slots of arguments 9 / 10 are left unset by the original
        callf(ctx, kAnim_Restart,
              {tr, 0xFFFFFFFF00000000ull | kClimbAnimKey, 0, 0, 0xFFFFFFFFFFFFFFFFull, 0, 0, 0, 0, 0, 0, 0xFFFFFFFFFFFFFFFFull},
              {speed, flag, k0, k0});
        store<u8>(t3 + 700, 0);
        store<u32>(t3 + 668, kClimbAnimKey);
        t4 = load<u32>(o + 32);
    }
    const u32 w1052 = load<u32>(t4 + 1052);
    const f64 v1052 = Lfs4(t4 + 1052);
    store<u32>(t4 + 1048, 0x3F800000);
    store<u32>(t4 + 1044, w1052);
    store<u32>(t4 + 1028, 0);
    callf(ctx, kMotion_Set, {t4 + 1024}, {Muls(Subs(one, v1052), Lfs4(K + 12))});
    callf(ctx, kAnim_SetTarget, {load<u32>(o + 32), kClimbTargetKey, 0xFFFFFFFF00000000ull | kClimbTargetKey, 0}, {k0});
    VCall(ctx, o, 28, {o, 0, 1});
}

namespace {
// f32 0 / mask {0, ~0, 0, 0} (+0), f32 1 (+16), masks (+32 lane 2, +48
// lane 3), f32 0.5 (+64), 0.05 (+68), 15 degrees (+72), -1 (+76)
constexpr u32 kCastK = 0x0024BD30;
constexpr u32 kCastAlloc = 0x011DF31C;  // the allocator object (vfn[5] frees the hit buffer)
}  // namespace

// void f(Obj* o; f32 dt) (from Obj_PhysStep_0024C230): with the definition
// d = o->+216 and the caster c = o->+412 (nothing without either): a stack
// query (+112 state 2, +128 = {0, 0, 1, 0} built with vsel / vandc from the
// masks, +144 zeroed, +160 = 0, +164 = 0, +208 = dt, +224 = 1 / dt, a hit
// buffer {data, count, 0x80000000} at +240) is cast: s = c->vfn[5](c, &q +
// 64, &buf). With s set, each 64-byte hit (point +0, normal +16) farther
// than r = 0.5 * min(d+800, d+808) + 0.05 from the probe point (o's +112
// direction * (that min * 0.5 - d+816) + o's +144) whose normal faces o's
// +112 direction closer than cos(15 degrees) and whose y is at least
// o->+148 sets o->+508 = -1 when the offset leans along o's +128
// direction, else 1; no hits clears s. A set s then gets c->vfn[6](c,
// &buf, 1, &q) with o->+480.. = q+128.. and +496 = q+160. The state s
// (1: +501 = 1; 2: +500 = 1; else, 0 too: +504 += dt) is applied, then unless the
// buffer is unowned (bit 31 of its +8) the allocator frees it. f1 / r3
// are left as the original leaves them.
DECOMP(0024BD80, PhysObj_CastUpdate_0024BD80) {
    const u32 o = arg<u32>(ctx, 0);
    const f64 dt = ctx.fpr[1];
    const u32 K = kCastK;
    Stack frame(ctx, 416);
    const u32 sp = static_cast<u32>(ctx.gpr[1]);
    const u32 d = load<u32>(o + 216);
    if (d == 0) return;
    const u32 c = load<u32>(o + 412);
    if (c == 0) {
        ctx.gpr[3] = 0;
        return;
    }
    store<u32>(sp + 112, 2);
    const V4 z = Andc(Zero4(), Lvx(K));
    const V4 m32 = Lvx(K + 32), m48 = Lvx(K + 48);
    Stvx(sp + 144, Andc(Andc(z, m32), m48));
    Stvx(sp + 128, Andc(Sel(z, SplatW(K + 16), m32), m48));
    const f64 k0 = Lfs4(K);
    store<u32>(sp + 160, 0);
    store<u8>(sp + 164, 0);
    store<f32>(sp + 208, static_cast<f32>(dt));
    const f64 k1 = Lfs4(K + 16);
    f64 F1 = Divs(k1, dt);
    store<u32>(sp + 240, 0);
    store<u32>(sp + 244, 0);
    store<u32>(sp + 248, 0x80000000);
    store<f32>(sp + 224, static_cast<f32>(F1));
    ctx.fpr[1] = F1;
    u32 s = static_cast<u32>(VCall(ctx, c, 20, {c, sp + 176, sp + 240}));
    F1 = ctx.fpr[1];
    store<u32>(sp + 112, s);
    if (s != 0) {
        const f64 d800 = Lfs4(d + 800), d808 = Lfs4(d + 808);
        const f64 h = Lfs4(K + 64), e = Lfs4(K + 68), d816 = Lfs4(d + 816);
        const f64 ox = Lfs4(o + 112), oy = Lfs4(o + 116), oz = Lfs4(o + 120);
        const f64 m = Subs(d808, d800) >= 0.0 ? d800 : d808;
        F1 = m;
        const f64 cx = Lfs4(o + 144), cy = Lfs4(o + 148), cz = Lfs4(o + 152);
        const f64 ang = Lfs4(K + 72);
        const f32 back = Fmsubs(m, h, d816);
        const f32 rr = Fmadds(m, h, e);
        const f32 px = Fmadds(ox, back, cx), py = Fmadds(oy, back, cy), pz = Fmadds(oz, back, cz);
        const f32 r2 = Muls(rr, rr);
        s32 i = 0;
        u32 off = 0;
        for (; i < static_cast<s32>(load<u32>(sp + 244)); ++i, off += 64) {
            const u32 p = load<u32>(sp + 240) + off;
            const f32 dx = Subs(Lfs4(p), px);
            const f64 hy = Lfs4(p + 4);
            const f32 dy = Subs(hy, py);
            const f32 dz = Subs(Lfs4(p + 8), pz);
            const f64 n0 = Lfs4(p + 16), n1 = Lfs4(p + 20), n2 = Lfs4(p + 24);
            F1 = n2;
            if (!(Fmadds(dz, dz, Fmadds(dx, dx, Muls(dy, dy))) > r2)) continue;
            const f32 facing = Fmadds(Lfs4(o + 120), n2, Fmadds(Lfs4(o + 112), n0, Muls(Lfs4(o + 116), n1)));
            F1 = callf(ctx, v5fn::Math_Cosf, {}, {ang});
            if (!(facing > F1)) continue;
            F1 = Lfs4(o + 148);
            if (!(hy >= F1)) continue;
            const f32 lean = Fmadds(Lfs4(o + 136), dz, Fmadds(Lfs4(o + 128), dx, Muls(Lfs4(o + 132), dy)));
            F1 = lean;
            store<u32>(o + 508, lean > k0 ? load<u32>(K + 76) : load<u32>(K + 16));
        }
        if (load<u32>(sp + 244) == 0) store<u32>(sp + 112, 0);
    }
    if (load<u32>(sp + 112) != 0) {
        const u32 c2 = load<u32>(o + 412);
        ctx.fpr[1] = F1;
        VCall(ctx, c2, 24, {c2, sp + 240, 1, sp + 112});
        F1 = Lfs4(sp + 128);
        store<u32>(o + 480, load<u32>(sp + 128));
        store<u32>(o + 484, load<u32>(sp + 132));
        store<u32>(o + 488, load<u32>(sp + 136));
        store<u32>(o + 496, load<u32>(sp + 160));
    }
    s = load<u32>(sp + 112);
    if (s == 1) {
        store<u8>(o + 501, 1);
        store<u8>(o + 500, 0);
        store<u32>(o + 504, 0);
    } else if (s == 2) {
        store<u8>(o + 501, 0);
        store<u8>(o + 500, 1);
        store<u32>(o + 504, 0);
    } else {
        const f32 v = Adds(Lfs4(o + 504), dt);
        F1 = v;
        store<u8>(o + 501, 0);
        store<u8>(o + 500, 0);
        store<f32>(o + 504, v);
    }
    ctx.fpr[1] = F1;
    const u32 cap = load<u32>(sp + 248);
    if ((cap & 0x80000000u) != 0) {
        ctx.gpr[3] = cap;
        return;
    }
    store<u32>(sp + 244, 0);
    VCall(ctx, kCastAlloc, 20, {kCastAlloc, load<u32>(sp + 240), static_cast<u64>(static_cast<s64>(static_cast<s32>(cap << 6)))});
}
