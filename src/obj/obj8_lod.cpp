// World objects, part 8: WorldObj_UpdateLod, the update a dirty object gets:
// its world matrix, its skeleton and bounding sphere, its children, then
// the ShowAndNotify slot.
#include <bit>
#include <cmath>

#include "obj8.h"

using namespace sly4;
using namespace sly4::obj;
using namespace sly4::obj4;
using namespace sly4::obj8;

namespace {

constexpr u32 kWorldObj_Refresh = 0x0057E14C;    // (obj) (name?)
constexpr u32 kSkeleton_Update = 0x0057E8D0;     // (ApeSkeleton*) (name?)
constexpr u32 kChild_UpdateLod = 0x0057E260;     // (child, const f32 parent_scale[3]) (name?)
constexpr u32 kBox_AddPoint = 0x00556FE8;        // (f32 box[8], const f32 p[3]) (name?)
constexpr u32 kSphere_FromBox = 0x005577E0;      // (f32 sphere[4], const f32 box[8]) (name?)
constexpr u32 kSphere_Transform = 0x00557900;    // (f32 sphere[4], const Matrix*, 0) (name?)
constexpr u32 kVec3_Add = 0x00557A20;            // (f32 dst[3], const f32 src[3]) (name?)

constexpr u32 kFlagMirror = 0x4000;              // +164: the local matrix's first row negated (name?)
constexpr u32 kFlagSkelBounds = 0x100;           // +164: the sphere comes from the bone positions (name?)
constexpr u32 kVfnShow = 16;                     // (obj, bool moved)
constexpr u32 kObjSphereC = 240, kObjSphereR = 252;  // f32[3] centre, f32 radius
constexpr u32 kPermTable = 0x0057EDA0;           // the mirror path's six vperm masks
constexpr u32 kSkelSphere = 0;                   // ApeSkeleton +0: f32[4] cached sphere
constexpr u32 kFrame = 528;                      // the original's frame
constexpr u32 kSpSphere = 112, kSpDir = 128, kSpBox = 144;

// world row i = r0 * m_i.x + r1 * m_i.y + r2 * m_i.z (+ r3 for i = 3), with
// m the +160 offset matrix's rows; stored at +80..+128.
void StoreWorld(u32 o, const Vec4 r[4]) {
    Vec4 m[4], out[4];
    MatLoad(load<u32>(o + obj6::kObjOffsetMtx), m);
    MatCombine(r, m, out);
    MatStore(o + obj6::kObjWorld, out);
}

// The integer (truncated) translation of the world matrix.
void IntPos(u32 o, s32 out[3]) {
    for (u32 k = 0; k < 3; ++k)
        out[k] = static_cast<s32>(static_cast<u32>(rt::fctiw(static_cast<f64>(load<f32>(o + obj6::kObjWorld + 48 + k * 4)), true)));
}

}  // namespace

// void WorldObj_UpdateLod(Obj* o, u32 unused, bool keep_skeleton_sphere)
// Only for a dirty object (+164 0x800): its world matrix = its local
// matrix (+16; with +164 0x4000 the first row negated) times the +160
// offset matrix; the dirty bit cleared; 0x0057E14C; with a skeleton
// (vfunc +28) 0x0057E8D0 and its bounding sphere: with +164 0x100
// the sphere of the bone positions (cached in the skeleton +0 and only
// recomputed every config +154 updates, the counter at +30, unless
// keep_skeleton_sphere) moved into world space (radius kept at the
// larger), else the first bone's offset turned by the world matrix added
// to +240; the children get 0x0057E260 with +144; non-unit scale scales
// the world rows; then vfunc +16 (moved = the truncated position
// changed) and the dirty bit cleared again.
// Quirk kept: with 0x4000 the w lanes of the rows come from uninitialised
// stack words of the original's frame (sp+188 / +204 / +220 / +236); they
// are read from the same guest addresses here.
DECOMP(0057EE00, WorldObj_UpdateLod) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 keep = arg<u32>(ctx, 2) & 0xFF;
    const u32 w164 = load<u32>(o + kObjFlags);
    if ((w164 & kObjDirty) == 0) {
        ctx.gpr[4] = 0;
        ret(ctx, w164);
        return;
    }
    Stack frame(ctx, kFrame);
    const u32 sp = frame.addr();
    s32 old_pos[3];
    IntPos(o, old_pos);
    const u32 l = o + obj6::kObjLocal;
    Vec4 rows[4];
    if ((w164 & kFlagMirror) == 0) {
        for (u32 i = 0; i < 4; ++i) rows[i] = VLoad(l + i * 16);
    } else {
        // The original's vperm sequence, step by step (lane 3 of each row
        // from the stack).
        const Vec4 p0 = VLoad(kPermTable), p1 = VLoad(kPermTable + 16), p2 = VLoad(kPermTable + 32);
        const Vec4 p3 = VLoad(kPermTable + 48), p4 = VLoad(kPermTable + 64), p5 = VLoad(kPermTable + 80);
        const Vec4 n2 = VFill(load<u32>(l + 8) ^ 0x80000000u), n1 = VFill(load<u32>(l + 4) ^ 0x80000000u);
        const Vec4 n0 = VFill(load<u32>(l) ^ 0x80000000u);
        Vec4 v3 = VPerm(VLoadLeft(l + 8), VLoadLeft(l + 4), p1);
        v3 = VPerm(v3, VSplat(VLoadLeft(l), 0), p2);
        v3 = VPerm(v3, VLoad(sp + 176), p3);
        Vec4 v2 = VPerm(VPerm(n2, n1, p0), n0, p2);
        rows[0] = VPerm(v2, v3, p3);
        rows[1] = VPerm(VPerm(VLoadLeft(l + 16), VLoadLeft(l + 20), p4), VPerm(VLoadLeft(l + 24), VLoad(sp + 192), p4), p5);
        rows[2] = VPerm(VPerm(VLoadLeft(l + 32), VLoadLeft(l + 36), p4), VPerm(VLoadLeft(l + 40), VLoad(sp + 208), p4), p5);
        rows[3] = VPerm(VPerm(VLoadLeft(l + 48), VLoadLeft(l + 52), p4), VPerm(VLoadLeft(l + 56), VLoad(sp + 224), p4), p5);
    }
    StoreWorld(o, rows);
    store<u32>(o + kObjFlags, load<u32>(o + kObjFlags) & ~kObjDirty);
    s32 pos[3];
    IntPos(o, pos);
    const u32 moved = (pos[0] != old_pos[0] || pos[1] != old_pos[1] || pos[2] != old_pos[2]) ? 1 : 0;

    call(ctx, kWorldObj_Refresh, {o});
    const u32 sk = static_cast<u32>(Vcall0(ctx, o, kVfnGetSkeleton));
    if (sk != 0) {
        call(ctx, kSkeleton_Update, {sk});
        if ((load<u32>(o + kObjFlags) & kFlagSkelBounds) != 0) {
            const u32 mod = load<u8>(obj6::kConfigSkelMod);
            bool rebuild = true;
            if (mod != 0) {
                const s32 n = static_cast<s32>(load<u8>(sk + obj6::kSkelSerial)) - 1;
                if (n < 0) {
                    store<u8>(sk + obj6::kSkelSerial, static_cast<u8>(n + static_cast<s32>(mod)));
                } else {
                    store<u8>(sk + obj6::kSkelSerial, static_cast<u8>(n));
                    if (keep == 0) {
                        for (u32 k = 0; k < 16; k += 4) store<u32>(sp + kSpSphere + k, load<u32>(sk + kSkelSphere + k));
                        rebuild = false;
                    }
                }
            }
            if (rebuild) {
                call(ctx, kSkeleton_Update, {sk});
                const u32 box = sp + kSpBox;
                store<u32>(box + 16, kFltMinNeg);
                for (u32 k = 0; k < 12; k += 4) store<u32>(box + k, kFltMax);
                store<u32>(box + 20, kFltMinNeg);
                store<u32>(box + 24, kFltMinNeg);
                call(ctx, kSkeleton_Update, {sk});
                call(ctx, cls::cfn::Box_SetPoint, {box, load<u32>(sk + obj6::kSkelWorldArr) + 48});
                if (load<u16>(sk + obj5::kSkelBoneCount) > 1) {
                    u32 off = obj6::kMatrixSize;
                    for (u32 i = 1; i < load<u16>(sk + obj5::kSkelBoneCount); ++i, off += obj6::kMatrixSize)
                        call(ctx, kBox_AddPoint, {box, load<u32>(sk + obj6::kSkelWorldArr) + off + 48});
                }
                call(ctx, kSphere_FromBox, {sp + kSpSphere, box});
                for (u32 k = 0; k < 16; k += 4) store<u32>(sk + kSkelSphere + k, load<u32>(sp + kSpSphere + k));
            }
            call(ctx, kSphere_Transform, {sp + kSpSphere, o + obj6::kObjWorld, 0});
            const u32 r = load<u32>(sp + kSpSphere + 12), old = load<u32>(o + kObjSphereR);
            const f32 d = static_cast<f32>(static_cast<f64>(std::bit_cast<f32>(r)) - static_cast<f64>(std::bit_cast<f32>(old)));
            store<u32>(o + kObjSphereR, d >= 0.0f ? r : old);
            for (u32 k = 0; k < 12; k += 4) store<u32>(o + kObjSphereC + k, load<u32>(sp + kSpSphere + k));
        } else {
            // The first bone's local offset turned by the world matrix.
            const u32 t = load<u32>(load<u32>(sk + obj5::kSkelBones) + obj5::kSkelLocal) + 48;
            const u32 w = o + obj6::kObjWorld;
            Vec4 a = VMadd(VLoad(w), VSplat(VLoadLeft(t), 0), VZero());
            a = VMadd(VLoad(w + 16), VSplat(VLoadLeft(t + 4), 0), a);
            a = VMadd(VLoad(w + 32), VSplat(VLoadLeft(t + 8), 0), a);
            VStore(sp + kSpDir, a);
            call(ctx, kVec3_Add, {o + kObjSphereC, sp + kSpDir});
        }
    }
    for (u32 c = load<u32>(o + kObjFirst); c != 0; c = load<u32>(c + kObjNext))
        call(ctx, kChild_UpdateLod, {c, o + kObjScale});
    if ((load<u32>(o + kObjFlags) & kObjNonUnitScale) != 0)
        call(ctx, obj6::o6fn::Matrix_ScaleRows, {o + obj6::kObjWorld, o + kObjScale});
    const u32 opd = load<u32>(load<u32>(o) + kVfnShow);
    call_ptr(ctx, opd, {o, moved, opd, load<u32>(opd)});  // r5 / r6 = OPD and entry
    const u32 w = load<u32>(o + kObjFlags) & ~kObjDirty;
    store<u32>(o + kObjFlags, w);
    ctx.gpr[4] = w;
    ret(ctx, 0u);
}
