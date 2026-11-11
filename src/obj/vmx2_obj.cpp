// Game code, VMX sweep part 2 (see vmx2.h): object callbacks that run a
// vector through an object's rows or the VMX row-building pattern.
#include "../sly4.h"
#include "vmx2.h"

using namespace sly4;
using namespace sly4::vmx2;

namespace {
constexpr u32 kEffectSetHash = 0x004CF988;  // Effect_SetHash4CF988 (Effect*, u32 hash) -> bool
constexpr u32 kVec3Zero = 0x01164B40;       // the global default Vec3 (zero)
constexpr u32 kResetEffectHash = 0x707A34AB;
constexpr u32 kHkActivate = 0x0091A4C8;     // Hk_Activate91A4C8 (hkpRigidBody*)
constexpr u32 kBodySetVec91E8C8 = 0x0091E8C8;  // (body, const Vec4*) (name?)
constexpr u32 kPartUpdate103AC0 = 0x00103AC0;  // (T* o, a, u8 flag)
constexpr u32 kWorldObjMarkDirty = 0x0057D000;  // WorldObj_MarkDirty (obj)
constexpr u32 kWorldObjUpdateLod = 0x0057EE00;  // WorldObj_UpdateLod (obj, -, s32)
constexpr u32 kHkSetMotion91A228 = 0x0091A228;   // (hkpEntity*, const T* motion) (name?)
constexpr u32 kHkSetPosRot91ECA8 = 0x0091ECA8;   // (hkpRigidBody*, const Vec4* pos, const Vec4* rot) (name?)
constexpr u32 kMatrixMultiply = 0x005541F0;  // Matrix_Multiply (Matrix* a, const Matrix* b): a = a x b
constexpr u32 kHkSetTransform = 0x0091EAB8;  // Hk_SetTransform91EAB8 (body, const f32 m[4][4])
constexpr u32 kObjAttach = 0x000D15C0;      // Obj_Attach0D15C0 (Object*, AttachDesc*) -> u32
constexpr u32 kPtrVectorClear = 0x0054EC1C;  // PtrVector_Clear (PtrVector*)
constexpr u32 kNop = 0x0054EC18;             // Nop (shared empty function)
constexpr u32 kSmallFree = 0x00552420;       // SmallFree (p, size)
constexpr u32 kQuatIdentity = 0x01164C80;    // the global identity quaternion (4 words)
constexpr u32 kInvXformPerm = 0x003B4EA0;    // two vperm masks for the transpose
constexpr u32 kCopyMat43 = 0x00012DE0;      // CopyMat43_00012DE0 (T* dst, const f32 m[4x4]): dst->+128 rows = m
constexpr u32 kBodyMoved = 0x003556AC;      // Body_Moved_003556AC (Body*)
constexpr u32 kMatrixRotationY = 0x00553E18;  // Matrix_RotationY (f32 m[4][4]; f32 angle)
constexpr u32 kObjModePos2 = 0x0026D8F4;       // Obj_ModePos_0026D8F4 (Vec3* out, Obj*)
constexpr u32 kPivotVec = 0x0105B3B0;          // Vec3: the pivot the rotation is about
constexpr u32 kUpVec = 0x01164B60;             // Vec3: the global up axis
constexpr u32 kObjPrep0D85A4 = 0x000D85A4;   // (Obj*) (name?)
constexpr u32 kWorldObjFindByHash = 0x005801DC;  // WorldObj_FindByHash (u32 hash, 0) -> WorldObj*
constexpr u32 kQuatToMatrix = 0x00553A90;    // Quat_ToMatrix (f32 out[4][4], const f32 q[4], const f32 pos[3])
constexpr u32 kAttachedHash = 0x45A6A51A;    // the world object the rotation is applied to
constexpr u32 kSharedMatrix = 0x01193010;    // the shared identity matrix (left alone)
constexpr u32 kRowPerm = 0x00169D30;         // two vperm masks building {x, y, z, pad} rows
constexpr u32 kRec80Find = 0x005848CC;       // Rec80_Find (table, u32 key) -> 80-byte record or 0
constexpr u32 kBoxCenter = 0x00556F98;       // Box_Center (box, out) -> out
constexpr u32 kAnchorKey = 0x2AF3A6AD;       // the record whose +48 point is the anchor
constexpr u32 kAabbTolerance = 0x00025EFC;   // f32 0.0: passed to the shape's aabb query
constexpr u32 kHalf4 = 0x00B2C2C0;           // {0.5, 0.5, 0.5, 0.5}
constexpr u32 kWorldPtr2 = 0x00F65CA0;       // World* ; +0x1281D4 the physics holder (+4 the hkpWorld)
constexpr u32 kObjRefPos2 = 0x0026A42C;      // Obj_RefPos26A42C (f32 out[3], Obj*)
constexpr u32 kFocusConst = 0x001277E8;      // f32 x, y default offset; the "unset" marker; the extent's y factor
constexpr u32 kObjGetChannel = 0x0045B160;   // Obj_GetChannel_0045B160 (const T* o, s32 i) -> f1
constexpr u32 kAtan2f = 0x005E665C;          // atan2f (f1 y, f2 x) -> f1
constexpr u32 kMathCosf = 0x007344E0;        // Math_Cosf (f1) -> f1
constexpr u32 kMathSinf = 0x007345F0;        // Math_Sinf (f1) -> f1
constexpr u32 kPadBase = 0x010B19E0;         // per-pad input records (11968 bytes; channels at +176)
constexpr u32 kWorldPtr3 = 0x00F65CA0;       // World* : +0x128224 a blocking mode, +0x128168 the camera (its matrix at +752)

// The stick-to-camera-direction update shared by two input handlers (they
// differ only in their constant pool and button tables): see below.
void TStickDir(PPUContext& ctx, u32 kpool, u32 tab_idx, u32 tab_mask, u32 n) {
    const u32 o = arg<u32>(ctx, 0);
    VCall(ctx, o, 16, {o});
    const u32 blk = load<u32>(load<u32>(kWorldPtr3) + 0x128224u);
    ctx.gpr[3] = blk;
    if (blk != 0) return;
    ctx.gpr[3] = load<u8>(o + 16);
    if (ctx.gpr[3] == 0) return;
    const u32 pad = kPadBase + load<u32>(o + 80) * 11968u + 176;
    call(ctx, kObjGetChannel, {pad, 0});
    const f64 sx = ctx.fpr[1];
    call(ctx, kObjGetChannel, {pad, 1});
    store<f32>(o + 32, static_cast<f32>(sx));
    store<f32>(o + 40, static_cast<f32>(ctx.fpr[1]));
    store<u32>(o + 36, 0);
    callf(ctx, kAtan2f, {}, {std::bit_cast<f64>(LfsBits(load<u32>(o + 40))), std::bit_cast<f64>(LfsBits(load<u32>(o + 32)))});
    const f64 a = ctx.fpr[1];
    callf(ctx, kMathCosf, {}, {a});
    const f64 c = std::fabs(ctx.fpr[1]);
    callf(ctx, kMathSinf, {}, {a});
    const bool use_cos = c > std::fabs(ctx.fpr[1]);
    callf(ctx, use_cos ? kMathCosf : kMathSinf, {}, {a});
    // scale by |cos| or |sin| (the larger): square stick to circle
    const f64 k = std::fabs(ctx.fpr[1]);
    const f32 y = Muls(F(o + 36), k), x = Muls(F(o + 32), k), z = Muls(F(o + 40), k);
    store<f32>(o + 32, x);
    store<f32>(o + 36, y);
    store<f32>(o + 40, z);
    const f32 one = F(kpool);
    const f32 len = Sqrts(Fmadds(z, z, Fmadds(x, x, Muls(y, y))));
    store<f32>(o + 64, len < one ? len : one);
    // the stick vector in camera space: R^T * v (+ the translation term's
    // lanes the perm keeps), R / T the camera matrix
    const u32 m = load<u32>(load<u32>(kWorldPtr3) + 0x128168u) + 752;
    const V4 r0 = Lvx(m), r1 = Lvx(m + 16), r2 = Lvx(m + 32);
    const V4 zero = Zero4();
    const V4 nt = Sub4(zero, Lvx(m + 48));
    const V4 lo = Mrghw(r0, r2);
    const V4 c0 = Mrghw(lo, r1);
    const V4 c1 = Perm(lo, r1, Lvx(kpool + 16));
    const V4 c2 = Perm(Mrglw(r0, r2), r1, Lvx(kpool + 32));
    store<f32>(o + 48, x);
    store<f32>(o + 52, y);
    store<f32>(o + 56, z);
    Stack frame(ctx, 176);
    Stvx(frame.addr() + 112, Fill4(z));
    const V4 t = Madd4(c2, Splat(nt, 2), Madd4(c1, Splat(nt, 1), Madd4(c0, Splat(nt, 0), zero)));
    const V4 v = Madd4(c2, Fill4(z), Madd4(c1, Fill4(y), Madd4(c0, Fill4(x), zero)));
    const V4 p2 = Lvx(kpool + 48), p3 = Lvx(kpool + 64);
    const V4 base = Perm(Perm(Lvlx(kVec3Zero), Lvlx(kVec3Zero + 4), p2), Perm(Lvlx(kVec3Zero + 8), t, p2), p3);
    Stvx(o + 48, Add4(base, v));
    store<u32>(o + 52, 0);
    SetF1Bits(ctx, load<u32>(kpool + 80));
    if (F(o + 64) >= F(kpool + 80)) {
        const f32 ny = F(o + 52), nx = F(o + 48), nz = F(o + 56);
        const f32 inv = Divs(one, Sqrts(Fmadds(nz, nz, Fmadds(nx, nx, Muls(ny, ny)))));
        const f32 rx = Muls(nx, inv), ry = Muls(ny, inv), rz = Muls(nz, inv);
        store<f32>(o + 48, rx);
        store<f32>(o + 52, ry);
        store<f32>(o + 56, rz);
        ctx.fpr[1] = ry;
    }
    // per-button 2-bit states at +68 (16 per word): bit 0 = held in the
    // pad's +8 mask, bit 1 = also in its +12 mask
    const u32 held = load<u32>(pad + 8), edge = load<u32>(pad + 12);
    for (u32 i = 0; i < n; ++i) {
        const u32 mask = load<u32>(tab_mask + (load<u32>(tab_idx + 12 * i) << 4));
        const u32 b0 = (mask & held) != 0 ? 1u : 0u;
        const u32 b1 = b0 != 0 && (mask & edge) != 0 ? 1u : 0u;
        const u32 w = o + 68 + ((i >> 2) & 0x3FFFFFFCu);
        const u32 sh = (i << 1) & 0x1Eu;
        store<u32>(w, (load<u32>(w) & ~(3u << sh)) | ((b0 | (b1 << 1)) << sh));
    }
    ctx.gpr[3] = o + 68;
}
}  // namespace

// void f(Obj* o) (a callback): resets o's motion. The owner's (+612) +156
// record gets +8 / +12 = 0 and +16 / +17 = 0 (bytes); the +948 object's
// +1268 flag 0x40000000 is cleared; +932 / +936 = 0; then o->vfn[34](o,
// zero) and o->vfn[96](o, zero). When the def's (+216) +1040 vector is not
// the zero vector (12-byte compare), it goes through o's rows (+96 / +112
// / +128, vmaddfp), is scaled by o->vfn[98](o) (f1) and handed to
// o->vfn[94](&tmp, o) and o->vfn[97](o, &scaled, &tmp). Last, o->+356
// (an effect) gets Effect_SetHash4CF988(e, kResetEffectHash). Leftovers as
// the original: f1 into vfn[94] is the scaled z; r3 when there is no
// effect is o's vtable (vector zero) or vfn[97]'s result.
DECOMP(0023CE5C, Obj_ResetMotion_0023CE5C) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 own = load<u32>(o + 612), x948 = load<u32>(o + 948);
    if (own != 0) {
        const u32 r = load<u32>(own + 156);
        if (r != 0) {
            store<u32>(r + 8, 0);
            store<u32>(r + 12, 0);
            store<u8>(r + 16, 0);
            store<u8>(r + 17, 0);
        }
    }
    if (x948 != 0) store<u32>(x948 + 1268, load<u32>(x948 + 1268) & ~0x40000000u);
    store<u32>(o + 932, 0);
    store<u32>(o + 936, 0);
    VCall(ctx, o, 136, {o, kVec3Zero});
    VCall(ctx, o, 384, {o, kVec3Zero});
    const u32 v = load<u32>(o + 216) + 1040;
    if (load<u64>(v) == load<u64>(kVec3Zero) && load<u32>(v + 8) == load<u32>(kVec3Zero + 8)) {
        ctx.gpr[3] = load<u32>(o);
    } else {
        Stack frame(ctx, 192);
        const u32 sp = frame.addr(), scaled = sp + 112, tmp = sp + 128, dir = sp + 144;
        V4 d = Madd4(Lvx(o + 96), SplatW(v), Zero4());
        d = Madd4(Lvx(o + 112), SplatW(v + 4), d);
        d = Madd4(Lvx(o + 128), SplatW(v + 8), d);
        Stvx(dir, d);
        VCall(ctx, o, 392, {o});
        const f64 s = ctx.fpr[1];
        store<f32>(scaled, Muls(F(dir), s));
        store<f32>(scaled + 4, Muls(F(dir + 4), s));
        const f32 z = Muls(F(dir + 8), s);
        store<f32>(scaled + 8, z);
        ctx.fpr[1] = z;
        VCall(ctx, o, 376, {tmp, o});
        VCall(ctx, o, 388, {o, scaled, tmp});
    }
    const u32 e = load<u32>(o + 356);
    if (e != 0) call(ctx, kEffectSetHash, {e, kResetEffectHash});
}

// void f(T* o, a, u8 flag) (called by 00105F90): with o->+16 (a pose
// source), +20 (a rigid body) and +24 all set: p = (+16)->vfn[39](&p,
// +16, 0) (a position out-parameter); when the body's +8 is set, the body
// is moved there (0x0091E8C8(body, &{p, 0})) and its two velocities are
// zeroed (Hk_Activate91A4C8(body), then (body+224)->vfn[17] / vfn[18]
// with the global zero vector, the body reloaded from o->+20 each time).
// Then 0x00103AC0(o, a, flag). Vectors are built with the vsel / vandc
// lane masks at 0x00105330 (w cleared). r3: 0 / o->+16 on the early exits
// (the original's last load), else 0x00103AC0's result.
DECOMP(00105360, Part_SyncBody_00105360) {
    const u32 o = arg<u32>(ctx, 0), a = arg<u32>(ctx, 1), flag = arg<u32>(ctx, 2) & 0xFFu;
    const u32 src = load<u32>(o + 16);
    ctx.gpr[3] = src;
    if (src == 0 || load<u32>(o + 20) == 0 || load<u32>(o + 24) == 0) return;
    Stack frame(ctx, 272);
    const u32 pos = frame.addr() + 112, vec = frame.addr() + 128;
    VCall(ctx, src, 156, {pos, src, 0});
    const u32 body = load<u32>(o + 20);
    if (load<u32>(body + 8) != 0) {
        const Masks3 k = LoadMasks3(0x00105330);
        Stvx(vec, RowSelW(pos, pos + 4, pos + 8, k));
        call(ctx, kBodySetVec91E8C8, {body, vec});
        for (u32 slot : {68u, 72u}) {
            Stvx(vec, RowSelW(kVec3Zero, kVec3Zero + 4, kVec3Zero + 8, k));
            const u32 b = load<u32>(o + 20);
            call(ctx, kHkActivate, {b});
            VCall(ctx, b + 224, slot, {b + 224, vec});
        }
    }
    call(ctx, kPartUpdate103AC0, {o, a, flag});
}

// void f(Obj* o) (called by ThunkArg1_001E514C): pushes o's transform out.
// With a parent (+424) the 4x3 matrix at o+512 is copied to its +16 rows
// (raw words) and WorldObj_MarkDirty(parent); else, with a world object
// (+160), o->vfn[21](o, o+512). Then WorldObj_UpdateLod(+160, 0, 0) when
// set. With an attached object (+620): the matrix (parent +80, else o+96)
// is copied to the attachment's +16 / +32 / +48 rows and its +64 row is
// the def's (+216) +320 offset (the global zero vector without a def)
// through that matrix (vmaddfp + translation). The matrix goes through a
// stack copy whose w lanes the original never writes (stale stack); only
// x / y / z of the result are used, so they don't matter. Store order and
// the f1 / r3 leftovers are the original's (f1: the +40 word; r3: +620).
DECOMP(001E4EDC, Obj_PushXform_001E4EDC) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 par = load<u32>(o + 424);
    if (par != 0) {
        for (u32 r = 0; r < 4; ++r)
            for (u32 k = 0; k < 3; ++k) CopyW(par + 16 + 16 * r + 4 * k, o + 512 + 16 * r + 4 * k);
        SetF1Bits(ctx, load<u32>(o + 568));
        call(ctx, kWorldObjMarkDirty, {par, par});
    } else if (load<u32>(o + 160) != 0) {
        VCall(ctx, o, 84, {o, o + 512});
    }
    const u32 wo = load<u32>(o + 160);
    if (wo != 0) call(ctx, kWorldObjUpdateLod, {wo, 0, 0});
    const u32 t = load<u32>(o + 620);
    ctx.gpr[3] = t;
    if (t == 0) return;
    const u32 p2 = load<u32>(o + 424);
    const u32 src = p2 != 0 ? p2 + 80 : o + 96;
    Stack frame(ctx, 208);
    const u32 v = frame.addr() + 112, m = frame.addr() + 128;
    for (u32 r = 0; r < 4; ++r)
        for (u32 k = 0; k < 3; ++k) CopyW(m + 16 * r + 4 * k, src + 16 * r + 4 * k);
    const u32 d = load<u32>(o + 216);
    const u32 q = d != 0 ? d + 320 : kVec3Zero;
    for (u32 k = 0; k < 3; ++k) CopyW(v + 4 * k, q + 4 * k);
    V4 x = Madd4(Lvx(m), SplatW(q), Zero4());
    x = Madd4(Lvx(m + 16), SplatW(q + 4), x);
    x = Madd4(Lvx(m + 32), SplatW(q + 8), x);
    Stvx(v, Add4(Lvx(m + 48), x));
    for (u32 off : {0u, 4u, 8u, 16u, 20u, 24u, 32u, 36u}) CopyW(t + 16 + off, m + off);
    CopyW(t + 64, v);
    CopyW(t + 56, m + 40);
    CopyW(t + 68, v + 4);
    CopyW(t + 72, v + 8);
    SetF1Bits(ctx, load<u32>(m + 40));
}

// u32 f(T* o) (called by 002AD2A4): re-seats o's rigid body. Clears the
// u16 counters +248 / +288 / +268 and the byte +300; then, when the
// physics wrapper (+8) yields a body (vfn[36] non-null with its +8 set),
// the position {+112..+120, w 0} and the rotation {+128..+140} are built
// (vsel lane masks at 0x002AD000), the four velocity vectors +192 / +176
// / +160 / +144 and +224 are set to the zero vector, a pending motion
// (byte +312) is applied (0x0091A228(body, o+24), then cleared), and
// 0x0091ECA8(body, &pos, &rot). The body is fetched through vfn[36] again
// before each use, as the original does. r3: 0 on the early exits, else
// 0x0091ECA8's result; f1 the zero vector's z once it is copied.
DECOMP(002AD030, Obj_ResetBody_002AD030) {
    const u32 o = arg<u32>(ctx, 0);
    store<u16>(o + 248, 0);
    store<u16>(o + 288, 0);
    store<u16>(o + 268, 0);
    store<u8>(o + 300, 0);
    auto body = [&]() { const u32 w = load<u32>(o + 8); return static_cast<u32>(VCall(ctx, w, 144, {w})); };
    ctx.gpr[3] = load<u32>(o + 8);
    if (ctx.gpr[3] == 0 || body() == 0) return;
    if (load<u32>(body() + 8) == 0) {
        ctx.gpr[3] = 0;
        return;
    }
    body();
    Stack frame(ctx, 160);
    const u32 pos = frame.addr() + 112, rot = frame.addr() + 128;
    const Masks3 k = LoadMasks3(0x002AD000);
    Stvx(pos, RowSelW(o + 112, o + 116, o + 120, k));
    Stvx(rot, Sel(Sel(Sel(SplatW(o + 128), SplatW(o + 132), k.m[0]), SplatW(o + 136), k.m[1]), SplatW(o + 140), k.m[2]));
    for (u32 off : {192u, 176u, 160u, 144u, 224u})
        for (u32 j = 0; j < 3; ++j) CopyW(o + off + 4 * j, kVec3Zero + 4 * j);
    SetF1Bits(ctx, load<u32>(kVec3Zero + 8));
    if (load<u8>(o + 312) != 0) {
        call(ctx, kHkSetMotion91A228, {body(), o + 24});
        store<u8>(o + 312, 0);
    }
    call(ctx, kHkSetPosRot91ECA8, {body(), pos, rot});
}

// void f(Obj* o) (called by 000DDCA8): with a rigid body (+348), the
// def's (+216) +164 model's +240 local matrix (4x3, copied to the stack
// as raw words) is multiplied by o's world matrix (+96) and handed to
// Hk_SetTransform91EAB8(body, m) as four {x, y, z, 0} rows (vsel lane
// masks at 0x000DDA00). r3: 0 without a body, else Hk_SetTransform's;
// f1 as the original leaves it (the last matrix word copied).
DECOMP(000DDA30, Obj_SyncBodyXform_000DDA30) {
    const u32 o = arg<u32>(ctx, 0);
    ctx.gpr[3] = load<u32>(o + 348);
    if (ctx.gpr[3] == 0) return;
    Stack frame(ctx, 304);
    const u32 rows = frame.addr() + 112, m = frame.addr() + 176;
    const u32 src = load<u32>(load<u32>(o + 216) + 164) + 240;
    for (u32 r = 0; r < 4; ++r)
        for (u32 k = 0; k < 3; ++k) CopyW(m + 16 * r + 4 * k, src + 16 * r + 4 * k);
    SetF1Bits(ctx, load<u32>(src + 56));  // the original's last lfs (f1 into Matrix_Multiply)
    call(ctx, kMatrixMultiply, {m, o + 96});
    const Masks3 km = LoadMasks3(0x000DDA00);
    for (u32 r = 0; r < 4; ++r) Stvx(rows + 16 * r, RowSelW(m + 16 * r, m + 16 * r + 4, m + 16 * r + 8, km));
    call(ctx, kHkSetTransform, {load<u32>(o + 348), rows});
}

// void f(T* o, Obj* b, Vec3* out) (called by 003B5720): attaches b to the
// object o->+104 at b's current place. out (when set) is first the zero
// vector. With both objects: an AttachDesc is built on the stack (the
// original's frame: desc at +176, b at +368, flags word +352 = its stale
// value with the top byte cleared, then | 0x20000000; -1 at +376 / +388,
// zeros, rotation the identity quaternion) whose position (+304) is b's
// translation (+144) in o->+104's frame (its rows +96 / +112 / +128 and
// translation +144: transposed with vmrghw / vmrglw / vperm, then
// R^T * p - R^T * t). Obj_Attach0D15C0(target, &desc); out gets the
// desc's position afterwards; a link the attach left in desc +356 is
// cleared, Nop'd and SmallFree'd (20 bytes). Leftovers as the original:
// f1 the last word copied (out's z or the position's z), r3 the target
// (early exits) or the last call's result.
DECOMP(003B4EC0, Obj_AttachInPlace_003B4EC0) {
    const u32 o = arg<u32>(ctx, 0), b = arg<u32>(ctx, 1), out = arg<u32>(ctx, 2);
    Stack frame(ctx, 432);
    const u32 sp = frame.addr();
    const u32 stale = load<u32>(sp + 352);
    if (out != 0) {
        for (u32 k = 0; k < 3; ++k) CopyW(out + 4 * k, kVec3Zero + 4 * k);
        SetF1Bits(ctx, load<u32>(kVec3Zero + 8));
    }
    const u32 t = load<u32>(o + 104);
    ctx.gpr[3] = t;
    if (t == 0 || b == 0) return;
    const u32 flags = stale & 0x00FFFFFFu;
    for (u32 k = 0; k < 3; ++k) CopyW(sp + 304 + 4 * k, kVec3Zero + 4 * k);
    for (u32 k = 0; k < 4; ++k) CopyW(sp + 320 + 4 * k, kQuatIdentity + 4 * k);
    store<u8>(sp + 176, 0);
    store<u8>(sp + 240, 0);
    store<u32>(sp + 352, flags);
    store<u32>(sp + 400, 0);
    for (u32 k = 0; k < 3; ++k) CopyW(sp + 160 + 4 * k, b + 144 + 4 * k);
    for (u32 off : {336u, 340u, 344u, 348u, 356u, 372u, 380u, 384u, 392u, 396u}) store<u32>(sp + off, 0);
    store<u32>(sp + 376, 0xFFFFFFFFu);
    store<u32>(sp + 388, 0xFFFFFFFFu);
    store<u32>(sp + 368, b);
    store<u32>(sp + 352, flags | 0x20000000u);
    // b's translation in t's frame
    const V4 r0 = Lvx(t + 96), r1 = Lvx(t + 112), r2 = Lvx(t + 128);
    const V4 nt = Sub4(Zero4(), Lvx(t + 144));
    const V4 lo = Mrghw(r0, r2);
    const V4 c0 = Mrghw(lo, r1);
    const V4 c1 = Perm(lo, r1, Lvx(kInvXformPerm));
    const V4 c2 = Perm(Mrglw(r0, r2), r1, Lvx(kInvXformPerm + 16));
    V4 p = Madd4(c0, SplatW(b + 144), Zero4());
    V4 q = Madd4(c0, Splat(nt, 0), Zero4());
    p = Madd4(c1, SplatW(b + 148), p);
    q = Madd4(c1, Splat(nt, 1), q);
    p = Madd4(c2, SplatW(b + 152), p);
    q = Madd4(c2, Splat(nt, 2), q);
    Stvx(sp + 160, Add4(q, p));
    for (u32 k = 0; k < 3; ++k) CopyW(sp + 304 + 4 * k, sp + 160 + 4 * k);
    ctx.fpr[2] = std::bit_cast<f64>(LfsBits(load<u32>(sp + 164)));
    SetF1Bits(ctx, load<u32>(sp + 168));
    call(ctx, kObjAttach, {t, sp + 176});
    const u32 link = load<u32>(sp + 356);
    if (out != 0) {
        for (u32 k = 0; k < 3; ++k) CopyW(out + 4 * k, sp + 304 + 4 * k);
        SetF1Bits(ctx, load<u32>(sp + 312));
    }
    if (link == 0) return;
    call(ctx, kPtrVectorClear, {link});
    call(ctx, kNop, {link});
    call(ctx, kSmallFree, {load<u32>(sp + 356), 20});
}

// void f(Obj* o, const f32 m[4][4]) (a callback): sets o's world matrix.
// The 4x3 part of m is copied to o+96 (raw words). With a rigid body
// (+388): Hk_SetTransform91EAB8(body, m as {x, y, z, 0} rows; vsel masks
// at 0x00194130, whose first word is the 0.0 the moves are compared with),
// then Body_Moved_003556AC(body) when the translation differs from the
// body's previous +288 position (read before the call; any component's
// difference != 0.0, unordered included). Then CopyMat43_00012DE0(+244,
// m) when +244 is set. f1 / r3 as the original leaves them (f1: m's last
// word, or 0.0 with a body; r3 the last call's result or the last load).
DECOMP(00194160, Obj_SetWorldMatrix_00194160) {
    const u32 o = arg<u32>(ctx, 0), m = arg<u32>(ctx, 1);
    for (u32 r = 0; r < 4; ++r)
        for (u32 k = 0; k < 3; ++k) CopyW(o + 96 + 16 * r + 4 * k, m + 16 * r + 4 * k);
    SetF1Bits(ctx, load<u32>(m + 56));
    const u32 body = load<u32>(o + 388);
    ctx.gpr[3] = body;
    if (body != 0) {
        const f32 ox = F(body + 288), oy = F(body + 292), oz = F(body + 296);
        const f32 nx = F(m + 48), ny = F(m + 52), nz = F(m + 56);
        Stack frame(ctx, 256);
        const u32 rows = frame.addr() + 112;
        const Masks3 k = LoadMasks3(0x00194130);
        for (u32 r = 0; r < 4; ++r) Stvx(rows + 16 * r, RowSelW(m + 16 * r, m + 16 * r + 4, m + 16 * r + 8, k));
        call(ctx, kHkSetTransform, {body, rows});
        const f32 zero = F(0x00194130);
        const u32 b2 = load<u32>(o + 388);
        ctx.gpr[3] = b2;
        f32 d = Subs(nx, ox);
        bool moved = !(d == zero);
        if (!moved) {
            d = Subs(ny, oy);
            moved = !(d == zero);
            if (!moved) {
                d = Subs(nz, oz);
                moved = !(d == zero);
            }
        }
        ctx.fpr[2] = d;
        SetF1Bits(ctx, load<u32>(0x00194130));
        if (moved) call(ctx, kBodyMoved, {b2});
    }
    const u32 c = load<u32>(o + 244);
    if (c != 0) call(ctx, kCopyMat43, {c, m});
}

// void f(f32 m[4][4], const T* a) (called by 0013A538): m = a rotation by
// a->+52 about y (Matrix_RotationY), translated so that the shape position
// p (Obj_ModePos_0026D8F4 of a->+8's vfn[16] shape) lands at the pivot
// (0x0105B3B0) of the rotated frame plus up (0x01164B60) * a->+40 plus the
// translation row of the a->+32 model's +240 matrix: m+48 = (up * s + t)
// (fmadds) + m * pivot - m * p (rows of m, vmaddfp). The model matrix is
// copied to the stack as the original does (only its translation is
// used). r3 / f1 as the original leaves them (8; the stored z).
DECOMP(002D2660, Matrix_PlaceRotY_002D2660) {
    const u32 m = arg<u32>(ctx, 0), a = arg<u32>(ctx, 1);
    callf(ctx, kMatrixRotationY, {m}, {std::bit_cast<f64>(LfsBits(load<u32>(a + 52)))});
    Stack frame(ctx, 256);
    const u32 sp = frame.addr(), p = sp + 112, mp = sp + 128, mq = sp + 144, cm = sp + 160;
    const u32 src = load<u32>(a + 32) + 240;
    for (u32 r = 0; r < 4; ++r)
        for (u32 k = 0; k < 3; ++k) CopyW(cm + 16 * r + 4 * k, src + 16 * r + 4 * k);
    SetF1Bits(ctx, load<u32>(src + 56));
    const u32 w = load<u32>(a + 8);
    call(ctx, kObjModePos2, {p, static_cast<u32>(VCall(ctx, w, 64, {w}))});
    const V4 m0 = Lvx(m), m1 = Lvx(m + 16), m2 = Lvx(m + 32), m3 = Lvx(m + 48);
    V4 v = Madd4(m0, SplatW(p), Zero4());
    v = Madd4(m1, SplatW(p + 4), v);
    v = Madd4(m2, SplatW(p + 8), v);
    Stvx(mp, Add4(m3, v));
    V4 u = Madd4(m0, SplatW(kPivotVec), Zero4());
    u = Madd4(m1, SplatW(kPivotVec + 4), u);
    u = Madd4(m2, SplatW(kPivotVec + 8), u);
    Stvx(mq, Add4(m3, u));
    const f64 s = F(a + 40);
    f32 r[3];
    for (u32 k = 0; k < 3; ++k) r[k] = Subs(Adds(Fmadds(F(kUpVec + 4 * k), s, F(cm + 48 + 4 * k)), F(mq + 4 * k)), F(mp + 4 * k));
    for (u32 k = 0; k < 3; ++k) store<f32>(m + 48 + 4 * k, r[k]);
    ctx.fpr[1] = r[2];
    ctx.gpr[3] = 8;
}

// void f(Obj* o) (a callback): applies o's rotation (quaternion +432) to
// the world object kAttachedHash. After 0x000D85A4(o), w =
// WorldObj_FindByHash(0x45A6A51A, 0); unless missing or still on the
// shared matrix (w->+160 == 0x01193010), its matrix m (w->+160) becomes
// Quat_ToMatrix(q, zero position) x m: m's rows are rebuilt with vperm
// (the pad lanes come from the original's uninitialised frame, which only
// reach the stack copy's w lanes; read from the same place here), each
// quaternion row goes through them (the last plus m's translation) and
// the 4x3 result is copied back as raw words. Then WorldObj_MarkDirty on
// w twice and on each object of its +260 chain. f1 is left as the last
// word copied; r3 as the last call's (0 / the shared matrix's address on
// the early exits).
DECOMP(00169D50, Obj_ApplyRotation_00169D50) {
    const u32 o = arg<u32>(ctx, 0);
    call(ctx, kObjPrep0D85A4, {o});
    const u32 w = static_cast<u32>(call(ctx, kWorldObjFindByHash, {kAttachedHash, 0}));
    if (w == 0) return;
    if (load<u32>(w + 160) == kSharedMatrix) {
        ctx.gpr[3] = kSharedMatrix;
        return;
    }
    Stack frame(ctx, 288);
    const u32 sp = frame.addr(), q = sp + 112;
    call(ctx, kQuatToMatrix, {q, o + 432, kVec3Zero});
    const u32 m = load<u32>(w + 160);
    const V4 P0 = Lvx(kRowPerm), P1 = Lvx(kRowPerm + 16);
    V4 row[4];
    for (u32 r = 0; r < 4; ++r)
        row[r] = Perm(Perm(Lvlx(m + 16 * r), Lvlx(m + 16 * r + 4), P0), Perm(Lvlx(m + 16 * r + 8), Lvx(sp + 176 + 16 * r), P0), P1);
    const Mat4 qm = LoadMat(q);
    const V4 r0 = XformDir(row[0], row[1], row[2], qm.r[0]);
    const V4 r1 = XformDir(row[0], row[1], row[2], qm.r[1]);
    const V4 r2 = XformDir(row[0], row[1], row[2], qm.r[2]);
    const V4 r3 = XformPoint(row[0], row[1], row[2], row[3], qm.r[3]);
    Stvx(q, r0);
    Stvx(q + 16, r1);
    Stvx(q + 32, r2);
    Stvx(q + 48, r3);
    for (u32 r = 0; r < 4; ++r)
        for (u32 k = 0; k < 3; ++k) CopyW(m + 16 * r + 4 * k, q + 16 * r + 4 * k);
    SetF1Bits(ctx, load<u32>(q + 56));
    call(ctx, kWorldObjMarkDirty, {w});
    for (u32 c = w; c != 0; c = load<u32>(c + 260)) call(ctx, kWorldObjMarkDirty, {c});
}

// void f(Vec3* out, -, Obj* o) (called by Spline_PointAt_000261BC /
// 000262C8): o's anchor point. With the model's (+160 -> +208 -> +108)
// record kAnchorKey: its +48 point (unaligned loads) through o's matrix
// (+96 rows, +144 translation). Else, with a rigid body (o->vfn[36](o))
// in a live hkpWorld: the body's shape (+16) aabb (shape->vfn[5](shape,
// body+240, &aabb; f1 0.0), entered with r5 = the OPD and r7 = the entry
// as the original leaves them) gives out = its centre with y raised by the
// half height (0.5 * (max - min), vmaddfp). Else out = Box_Center of
// o->vfn[40](&box, o, 1) with y = the box's max y. Leftovers as the
// original: f1 the last float stored; r3 272, 128 or the stack address of
// the box's max y.
DECOMP(00025F00, Obj_AnchorPoint_00025F00) {
    const u32 out = arg<u32>(ctx, 0), o = arg<u32>(ctx, 2);
    Stack frame(ctx, 320);
    const u32 sp = frame.addr();
    const u32 tab = load<u32>(load<u32>(load<u32>(o + 160) + 208) + 108);
    const u32 rec = tab != 0 ? static_cast<u32>(call(ctx, kRec80Find, {tab, kAnchorKey})) : 0u;
    if (rec != 0) {
        const u32 v = sp + 272;
        for (u32 k = 0; k < 3; ++k) CopyW(v + 4 * k, rec + 48 + 4 * k);
        V4 p = Madd4(Lvx(o + 96), Fill4(F(rec + 48)), Zero4());
        p = Madd4(Lvx(o + 112), Fill4(F(rec + 52)), p);
        p = Madd4(Lvx(o + 128), Fill4(F(rec + 56)), p);
        Stvx(v, Add4(Lvx(o + 144), p));
        for (u32 k = 0; k < 3; ++k) CopyW(out + 4 * k, v + 4 * k);
        SetF1Bits(ctx, load<u32>(v + 8));
        ctx.gpr[3] = 272;
        return;
    }
    const u32 body = static_cast<u32>(VCall(ctx, o, 144, {o}));
    const u32 world = load<u32>(load<u32>(load<u32>(kWorldPtr2) + 0x1281D4u) + 4);
    if (body != 0 && world != 0) {
        const u32 shape = load<u32>(body + 16);
        const u32 opd = load<u32>(load<u32>(shape) + 20);
        const u32 aabb = sp + 160;
        ctx.fpr[1] = std::bit_cast<f64>(LfsBits(load<u32>(kAabbTolerance)));
        call_ptr(ctx, opd, {shape, body + 240, opd, aabb, load<u32>(opd)});
        const V4 mn = Lvx(aabb), mx = Lvx(aabb + 16), h = Lvx(kHalf4);
        Stvx(sp + 128, Madd4(h, Add4(mn, mx), Zero4()));
        Stvx(sp + 112, Madd4(h, Sub4(mx, mn), Zero4()));
        for (u32 k = 0; k < 3; ++k) CopyW(out + 4 * k, sp + 128 + 4 * k);
        const f32 y = Adds(F(out + 4), F(sp + 116));
        store<f32>(out + 4, y);
        ctx.fpr[1] = y;
        ctx.gpr[3] = 128;
        return;
    }
    const u32 box = sp + 192;
    VCall(ctx, o, 160, {box, o, 1});
    const u32 c = static_cast<u32>(call(ctx, kBoxCenter, {box, sp + 144}));
    for (u32 k = 0; k < 3; ++k) CopyW(out + 4 * k, c + 4 * k);
    CopyW(out + 4, box + 20);
    SetF1Bits(ctx, load<u32>(box + 20));
    ctx.gpr[3] = box + 20;
}

// void f(Vec3* out, Obj* o) (a callback): o's focus point. When o's state
// (vfn[8](o, 0) == 0, vfn[8](o, 1) == 6) says it rides an object (the
// +20 table's entry at the u16 +6 offset, with its +192 bit 1 set), that
// object's vfn[71](out, it) answers. Else a local point goes through o's
// matrix (+96 rows, +144 translation; each coordinate splatted through
// the stack): the def's (+216) +1440 offset when it isn't the "unset"
// marker (kFocusConst +8, compared != so a NaN counts as set), else the
// shape's (vfn[16]) mode position with y raised by its extent's y times
// kFocusConst +12 (fmadds), else {k0, k1, k0}. f1 is left as out's z
// and r3 as o+144 (the original's last address) on that path.
DECOMP(001277F8, Obj_FocusPoint_001277F8) {
    const u32 out = arg<u32>(ctx, 0), o = arg<u32>(ctx, 1);
    if (static_cast<u32>(VCall(ctx, o, 32, {o, 0})) == 0 && static_cast<u32>(VCall(ctx, o, 32, {o, 1})) == 6) {
        const u32 c = load<u32>(load<u32>(o + 20) + load<u16>(o + 6));
        if (c != 0 && (load<u32>(c + 192) & 1u) != 0) {
            VCall(ctx, c, 284, {out, c});
            return;
        }
    }
    Stack frame(ctx, 224);
    const u32 sp = frame.addr(), p = sp + 112, q = sp + 128;
    f32 x = F(kFocusConst), y = F(kFocusConst + 4), z = x;
    store<u32>(p, 0);
    store<u32>(p + 4, 0x3F800000u);
    store<u32>(p + 8, 0);
    const u32 d = load<u32>(o + 216);
    if (d != 0 && !(F(d + 1440) == F(kFocusConst + 8))) {
        x = F(d + 1440);
        y = F(d + 1444);
        z = F(d + 1448);
        for (u32 k = 0; k < 3; ++k) CopyW(p + 4 * k, d + 1440 + 4 * k);
    } else {
        const u32 m = static_cast<u32>(VCall(ctx, o, 64, {o}));
        if (m != 0) {
            call(ctx, kObjModePos2, {q, m});
            x = F(q);
            z = F(q + 8);
            y = F(q + 4);
            CopyW(p, q);
            CopyW(p + 8, q + 8);
            call(ctx, kObjRefPos2, {q, m});
            y = Fmadds(F(q + 4), F(kFocusConst + 12), y);
            store<f32>(p + 4, y);
        }
    }
    V4 v = Madd4(Lvx(o + 96), Fill4(x), Zero4());
    v = Madd4(Lvx(o + 112), Fill4(y), v);
    v = Madd4(Lvx(o + 128), Fill4(z), v);
    Stvx(sp + 144, Fill4(z));
    Stvx(p, Add4(Lvx(o + 144), v));
    for (u32 k = 0; k < 3; ++k) CopyW(out + 4 * k, p + 4 * k);
    SetF1Bits(ctx, load<u32>(p + 8));
    ctx.gpr[3] = o + 144;
}

// void f(T* o) (a callback): an input handler's update. After o->vfn[4](o),
// unless the world blocks input (+0x128224) or o is inactive (u8 +16): the
// pad's (+80) stick channels 0 / 1 give the stick vector {x, 0, y} at +32,
// scaled by the larger of |cos| / |sin| of its angle (atan2f, Math_Cosf /
// Math_Sinf), +64 = min(length, 1.0); +48 = the vector in camera space
// (the camera matrix transposed with vmrghw / vmrglw / vperm; the base
// vector from the zero vector's lanes), y cleared, normalised when +64 >=
// 1e-5. Then the 2-bit button states at +68 for the pad buttons of the
// 11-entry table 0x00AD46AC (masks 0x00AD4528). f1 / r3 as the original
// leaves them (the normalised y or 1e-5; o+68).
DECOMP(0027E508, Input_StickDir_0027E508) { TStickDir(ctx, 0x0027E4B0, 0x00AD46AC, 0x00AD4528, 11); }

// The same with the constant pool 0x0027FC60 and the one-entry button
// table 0x00AD4820 (masks 0x00AD472C).
DECOMP(0027FCB8, Input_StickDir_0027FCB8) { TStickDir(ctx, 0x0027FC60, 0x00AD4820, 0x00AD472C, 1); }
