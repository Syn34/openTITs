// Game code, VMX sweep part 6 (see vmx6.h): object updates and anim-script
// commands.
#include "../sly4.h"
#include "vmx6.h"

using namespace sly4;
using namespace sly4::vmx6;

namespace {
constexpr u32 kDegHalf20AD00 = 0x0020ACF4;  // f32 {pi / 180, 0.5}
}  // namespace

// bool f(Obj* o; f32 dt) (called through a pointer): update of an object
// carrying a spinning attachment p = o->+420. Registry_Add(o), then
// Obj_UpdateBase(o, dt); with p set: p's world position (+96) is p's local
// offset +112 through o's 4x3 rows (+96..+152, copied to a stack matrix
// whose w lanes are whatever the stack holds), p's rotation quaternion
// (+128) is o's rows (translation zeroed) times the Euler rotation of
// p's angles in degrees (+144, +148 plus the accumulated spin o->+424,
// +152), p->+172 = 1; then o->+424 += dt * (o->+216)->+172. Returns 1 (f1
// is the new spin, or what Obj_UpdateBase left without p).
DECOMP(0020AD00, SpinAttach_Update_0020AD00) {
    const u32 o = arg<u32>(ctx, 0);
    const f64 dt = ctx.fpr[1];
    Stack frame(ctx, 384);
    const u32 sp = static_cast<u32>(ctx.gpr[1]);
    const u32 def = load<u32>(o + 216);
    call(ctx, v6fn::Registry_Add_000D64F4, {o});
    callf(ctx, v6fn::Obj_UpdateBase, {o}, {dt});
    if (load<u32>(o + 420) != 0) {
        const u32 p = load<u32>(o + 420), M = sp + 160;
        for (u32 r = 0; r < 4; ++r) CopyW6(M + 16 * r, o + 96 + 16 * r, 3);
        Stvx(sp + 112, XformPoint(Lvx(M), Lvx(M + 16), Lvx(M + 32), Lvx(M + 48), W3(p + 112)));
        const f64 k = Lfs4(kDegHalf20AD00), h = Lfs4(kDegHalf20AD00 + 4);
        const f32 a = Muls(Muls(Adds(Lfs4(p + 148), Lfs4(o + 424)), k), h);
        const f32 b = Muls(Muls(Lfs4(p + 152), k), h);
        const f32 c = Muls(Muls(Lfs4(p + 144), k), h);
        const f64 ca = callf(ctx, v4fn::Math_Cosf, {}, {a});
        const f64 cb = callf(ctx, v4fn::Math_Cosf, {}, {b});
        const f64 cc = callf(ctx, v4fn::Math_Cosf, {}, {c});
        const f64 sa = callf(ctx, v5fn::Math_Sinf, {}, {a});
        const f64 sb = callf(ctx, v5fn::Math_Sinf, {}, {b});
        const f64 sc = callf(ctx, v5fn::Math_Sinf, {}, {c});
        EulerQuat6(sp + 128, ca, cb, cc, sa, sb, sc);
        call(ctx, v4fn::Quat_ToMatrix, {sp + 224, sp + 128, kV3Zero3});
        CopyW6(M + 48, kV3Zero3, 3);
        call(ctx, v4fn::Matrix_Multiply, {M, sp + 224});
        CopyW6(M + 48, sp + 112, 3);
        const u32 p2 = load<u32>(o + 420);
        store<u8>(p2 + 172, 1);
        CopyW6(p2 + 96, M + 48, 3);
        call(ctx, v4fn::Quat_FromMatrix, {sp + 144, M, 1});  // r5 = 1 left from the store above
        CopyW6(p2 + 128, sp + 144, 4);
        const f32 spin = Fmadds(dt, Lfs4(def + 172), Lfs4(o + 424));
        store<f32>(o + 424, spin);
        ctx.fpr[1] = spin;
    }
    ctx.gpr[3] = 1;
}

namespace {
constexpr u32 kPool1EF330 = 0x001EF2F0;  // f32 {0.5, 2.0, 0.0, -4.0}, then the masks {y}, {z}, {w}
}  // namespace

// u32 f(Obj* o) (called by 001F08C0): the placed-object matrix. With d =
// o->+216: Quat_ToMatrix(&m, Euler quaternion of d's angles +400..+408 in
// radians, d's position +384) into o's rows +432..+480 (xyz words), those
// rows then taken through o's world rows +96 (the translation row with
// +144); then a Havok ray (filter 13) from 2 above that point to 2 below it
// and, on a hit (the output's root collidable +68), the point moved to the
// hit fraction along the ray. Returns the root collidable word (0 on a
// miss); f1 is the new z on a hit, else what the ray cast left.
DECOMP(001EF330, Obj_PlaceOnGround_001EF330) {
    const u32 o = arg<u32>(ctx, 0);
    Stack frame(ctx, 432);
    const u32 sp = static_cast<u32>(ctx.gpr[1]);
    const u32 d = load<u32>(o + 216);
    CopyW6(sp + 128, d + 384, 3);
    const f64 k = Lfs4(kPool1EF330);
    const f32 a = Muls(Lfs4(d + 404), k), b = Muls(Lfs4(d + 408), k), c = Muls(Lfs4(d + 400), k);
    const f64 ca = callf(ctx, v4fn::Math_Cosf, {}, {a});
    const f64 cb = callf(ctx, v4fn::Math_Cosf, {}, {b});
    const f64 cc = callf(ctx, v4fn::Math_Cosf, {}, {c});
    const f64 sa = callf(ctx, v5fn::Math_Sinf, {}, {a});
    const f64 sb = callf(ctx, v5fn::Math_Sinf, {}, {b});
    const f64 sc = callf(ctx, v5fn::Math_Sinf, {}, {c});
    EulerQuat6(sp + 112, ca, cb, cc, sa, sb, sc);
    call(ctx, v4fn::Quat_ToMatrix, {sp + 192, sp + 112, sp + 128});
    for (u32 r = 0; r < 4; ++r) CopyW6(o + 432 + 16 * r, sp + 192 + 16 * r, 3);
    const Mat4 w = LoadMat(o + 96);
    const V4 t = Lvx(o + 480), l0 = Lvx(o + 432), l1 = Lvx(o + 448), l2 = Lvx(o + 464);
    Stvx(o + 432, XformDir(w.r[0], w.r[1], w.r[2], l0));
    Stvx(o + 448, XformDir(w.r[0], w.r[1], w.r[2], l1));
    Stvx(o + 464, XformDir(w.r[0], w.r[1], w.r[2], l2));
    Stvx(o + 480, XformPoint(w.r[0], w.r[1], w.r[2], w.r[3], t));
    const f64 c8 = Lfs4(kPool1EF330 + 8), c12 = Lfs4(kPool1EF330 + 12);
    const f32 y0 = Adds(Lfs4(o + 484), Lfs4(kPool1EF330 + 4));
    const f64 x0 = Lfs4(o + 480), z0 = Lfs4(o + 488);
    const Masks3 mk = LoadMasks3(kPool1EF330 + 16);
    // hkpWorldRayCastInput at sp+144: from, to, enableShapeCollectionFilter,
    // filterInfo 13, userData 0.
    Stvx(sp + 144, RowSel(SplatW(o + 480), Fill4(y0), SplatW(o + 488), mk));
    Stvx(sp + 160, RowSel(Fill4(Adds(x0, c8)), Fill4(Adds(y0, c12)), Fill4(Adds(z0, c8)), mk));
    store<u8>(sp + 176, 0);
    store<u32>(sp + 180, 13);
    store<u32>(sp + 184, 0);
    // hkpWorldRayCastOutput at sp+256: hit fraction 1.0, extra info -1,
    // shape key -1, root collidable 0.
    store<u32>(sp + 272, 0x3F800000u);
    store<u32>(sp + 276, 0xFFFFFFFFu);
    store<u32>(sp + 288, 0xFFFFFFFFu);
    store<u32>(sp + 320, 0);
    store<u32>(sp + 324, 0);
    const u32 world = load<u32>(load<u32>(load<u32>(kV3WorldPtr) + 0x1281D4u) + 4);
    call(ctx, v6fn::HkWorld_CastRay92E2E8, {world, sp + 144, sp + 256});
    const u32 hit = load<u32>(sp + 324);
    if (hit != 0) {
        const f64 f = Lfs4(sp + 272);
        store<f32>(o + 480, Fmadds(f, c8, x0));
        store<f32>(o + 484, Fmadds(f, c12, y0));
        const f32 z = Fmadds(f, c8, z0);
        store<f32>(o + 488, z);
        ctx.fpr[1] = z;
    }
    ctx.gpr[3] = hit;
}

namespace {
// The target object of the effect binding found under owner for id: the
// component Effect_FindComponent(owner, id) whose vfunc +32 (kind) is 5 and
// whose binding +28 has kind 0x02235C33 gives its +280; else 0.
u32 BoundTarget6(PPUContext& ctx, u32 owner, u32 id) {
    const u32 c = static_cast<u32>(call(ctx, v6fn::Effect_FindComponent, {owner, id}));
    if (c == 0 || static_cast<u32>(VCall(ctx, c, 32, {c})) != 5) return 0;
    const u32 e = load<u32>(c + 28);
    if (e == 0 || load<u32>(e + 276) != kEffectTargetKind6) return 0;
    return load<u32>(e + 280);
}
}  // namespace

// u32 f(Cmd* t, Effect* owner) (anim-script command, called through a
// pointer): pushes a point onto a target's Vec3 list. The target is the
// bound object of id +12 (else Obj_FindById(+16) when +16 is set); the
// frame object that of id +20, else Obj_FindById of the vfunc +28 value of
// the kind-12 component +28 (when non-zero) or of +24. The point is +48
// (or the vector +32 of the kind-3 component +32), taken through the
// frame object's rows +96 (no translation) when there is one; then
// Vec3List_Push(target, &point). Without a target nothing is pushed and r3
// is what the original leaves: the kind 0x02235C33 when the frame object
// came from a binding, else the frame object.
DECOMP(0007CBD4, AnimCmd_PushPoint_0007CBD4) {
    const u32 t = arg<u32>(ctx, 0), owner = arg<u32>(ctx, 1);
    Stack frame(ctx, 176);
    const u32 sp = static_cast<u32>(ctx.gpr[1]);
    u64 tgt = load<u32>(t + 12) != 0 ? BoundTarget6(ctx, owner, load<u32>(t + 12)) : 0;
    if (tgt == 0 && load<u32>(t + 16) != 0) tgt = call(ctx, v6fn::Obj_FindById, {load<u32>(t + 16)});
    u64 rel = load<u32>(t + 20) != 0 ? BoundTarget6(ctx, owner, load<u32>(t + 20)) : 0;
    const bool bound = rel != 0;
    if (!bound) {
        rel = load<u32>(t + 24);
        const u32 c = static_cast<u32>(call(ctx, v6fn::Effect_FindComponent, {owner, load<u32>(t + 28)}));
        if (c != 0 && static_cast<u32>(VCall(ctx, c, 32, {c})) == 12 && static_cast<u32>(VCall(ctx, c, 28, {c})) != 0)
            rel = VCall(ctx, c, 28, {c});
        rel = call(ctx, v6fn::Obj_FindById, {rel});
    }
    if (static_cast<u32>(tgt) == 0) {
        ctx.gpr[3] = bound ? kEffectTargetKind6 : rel;
        return;
    }
    CopyW6(sp + 112, t + 48, 3);
    u32 src = t + 48;
    if (load<u32>(t + 32) != 0) {
        const u32 c = static_cast<u32>(call(ctx, v6fn::Effect_FindComponent, {owner, load<u32>(t + 32)}));
        if (c != 0 && static_cast<u32>(VCall(ctx, c, 32, {c})) == 3) {
            CopyW6(sp + 112, c + 32, 3);
            src = c + 32;
        }
    }
    if (static_cast<u32>(rel) != 0) {
        const u32 M = static_cast<u32>(rel) + 96;
        Stvx(sp + 112, XformDir(Lvx(M), Lvx(M + 16), Lvx(M + 32), W3(sp + 112)));
    }
    ctx.fpr[1] = Lfs4(src);  // f1 as the original leaves it (the lfs of the point's x)
    call(ctx, v6fn::Vec3List_Push_00244810, {tgt, sp + 112});
}

namespace {
constexpr u32 kEventVt6E1BC = 0x00B0F048;  // the event record's vtable
}  // namespace

// u32 f(Cmd* t, Effect* owner) (anim-script command, called through a
// pointer): sends an event at a point. The target is the bound object of
// the binding hash 0x0027469E (see BoundTarget6), replaced by the player
// slot's object (World + 0x20C80 * t->+76 + 4208, its +8) when +76 != -1.
// The point is +16..+24, through the rows +80 of the kind-5 component +12's
// binding (+28) when there is one. The extra value is the vfunc +24 result
// of the kind-2 component 0x25431D90 (else 0). The event record (vtable
// 0x00B0F048) on the stack carries the target, +68, the u16 +72, a flags
// halfword (its other bits are whatever the stack holds: 0x4000 set,
// 0xAF00 clear, 0x1000 when the target, its +392 (with +192 & 0x1000) or
// its +432 (with +192 & 0x2000) has bit 0 of the doubleword +184), the
// point, +32..+40, +52, +48, the u8 +56, +60, +64, the up axis, 0 and the
// extra value; then Obj_SendEvent_000E1C28(&record).
DECOMP(0006E1BC, AnimCmd_SendAt_0006E1BC) {
    const u32 t = arg<u32>(ctx, 0), owner = arg<u32>(ctx, 1);
    Stack frame(ctx, 352);
    const u32 sp = static_cast<u32>(ctx.gpr[1]);
    u32 tgt = BoundTarget6(ctx, owner, 0x0027469Eu);
    const s32 slot = static_cast<s32>(load<u32>(t + 76));
    if (slot != -1) tgt = load<u32>(load<u32>(kV3WorldPtr) + static_cast<u32>(slot) * 0x20C80u + 4208 + 8);
    V4 pos = W3(t + 16);
    const u32 c = static_cast<u32>(call(ctx, v6fn::Effect_FindComponent, {owner, load<u32>(t + 12)}));
    if (c != 0 && static_cast<u32>(VCall(ctx, c, 32, {c})) == 5 && load<u32>(c + 28) != 0) {
        const u32 M = load<u32>(c + 28) + 80;
        pos = XformPoint(Lvx(M), Lvx(M + 16), Lvx(M + 32), Lvx(M + 48), pos);
        Stvx(sp + 112, pos);
    }
    u64 extra = 0;
    const u32 c2 = static_cast<u32>(call(ctx, v6fn::Effect_FindComponent, {owner, 0x25431D90u}));
    if (c2 != 0 && static_cast<u32>(VCall(ctx, c2, 32, {c2})) == 2) extra = VCall(ctx, c2, 24, {c2});
    const u32 ev = sp + 128;
    u32 h = (load<u16>(ev + 14) & ~0xEF00u) | 0x4000u;  // lhz of the stack slot
    store<u32>(ev + 4, tgt);
    store<u32>(ev + 8, load<u32>(t + 68));
    store<u16>(ev + 12, static_cast<u16>(load<u32>(t + 72)));
    u32 bit = 0;
    if (tgt != 0 && (load<u64>(tgt + 184) & 1) != 0) bit = 1;
    h = (h & ~0x1000u) | (bit << 12);
    store<u16>(ev + 14, static_cast<u16>(h));
    store<u32>(ev, kEventVt6E1BC);
    for (u32 k = 0; k < 3; ++k) store<u32>(ev + 16 + 4 * k, LaneW(pos, k));
    CopyW6(ev + 32, t + 32, 3);
    store<u32>(ev + 48, load<u32>(t + 52));
    store<u32>(ev + 52, load<u32>(t + 48));
    store<u8>(ev + 56, load<u8>(t + 56));
    store<u32>(ev + 60, load<u32>(t + 60));
    store<u32>(ev + 64, load<u32>(t + 64));
    CopyW6(ev + 80, kUpAxis5, 3);
    store<u32>(ev + 96, 0);
    store<u32>(ev + 100, static_cast<u32>(extra));
    if (tgt != 0) {
        if ((load<u32>(tgt + 192) & 0x1000u) != 0 && (load<u64>(load<u32>(tgt + 392) + 184) & 1) != 0) {
            h |= 0x1000u;
            store<u16>(ev + 14, static_cast<u16>(h));
        }
        if ((load<u32>(tgt + 192) & 0x2000u) != 0) {
            const u32 q = load<u32>(tgt + 432);
            if (q != 0 && (load<u64>(q + 184) & 1) != 0) {
                h |= 0x1000u;
                store<u16>(ev + 14, static_cast<u16>(h));
            }
        }
    }
    ctx.fpr[1] = Lfs4(kUpAxis5 + 4);  // f1 as the original leaves it (the lfs of the up axis' y)
    call(ctx, v6fn::Obj_SendEvent_000E1C28, {ev});
}
