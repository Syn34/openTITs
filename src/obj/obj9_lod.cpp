// World objects, part 9: WorldObj_UpdateChildLod, WorldObj_UpdateLod's
// update of a child object: its world matrix from the parent's (or one of
// the parent's bones), its bounding sphere, its own children, then the
// ShowAndNotify slot.
#include "obj9.h"

using namespace sly4;
using namespace sly4::obj4;
using namespace sly4::obj8;
using obj9::Fmuls;
using obj9::Lf;

namespace {

constexpr u32 kWorldObj_UpdateSphere = 0x0057E14C;   // (obj) -> &obj +240
constexpr u32 kDrawItem_GetScale = 0x00554484;       // (const f32 m[3][4], f32 out[3]): the row lengths
constexpr u32 kMatrix_NormalizeInPlace = 0x0056D374; // (f32 m[4][4]) -> m: 0x0056D268(m, m) (name?)
constexpr u32 kMatrix_Normalize = 0x0056D268;        // (f32 dst[4][4], const f32 src[4][4]) (name?)
constexpr u32 kIdentity = 0x01164BD0;                // f32[4][4]
constexpr u32 kVfnShow = 16;                         // (obj, bool moved)

// +168 bits choosing how the parent's scale reaches the child (name?).
constexpr u32 kInheritPosOnly = 0x00100000;    // only the position: the parent's rows become the identity
constexpr u32 kInheritScaleSet = 0x00040000;   // the parent's scale moved onto the child (SetScale), rows normalised
constexpr u32 kInheritNormalize = 0x00080000;  // otherwise: the offset scaled by the parent's scale, rows normalised

// The original's frame offsets.
constexpr u32 kSpScale = 112, kSpChildScale = 128, kSpMatrix = 144, kSpPos = 256, kSpSplat = 272;

}  // namespace

// f32* Matrix_NormalizeInPlace(f32 m[4][4])
// 0x0056D268(m, m); returns m.
DECOMP(0056D374, Matrix_NormalizeInPlace) {
    const u32 m = arg<u32>(ctx, 0);
    call(ctx, kMatrix_Normalize, {m, m});
    ret(ctx, m);
}

// bool WorldObj_UpdateChildLod(Obj* c, const f32 parent_scale[3])
// The parent's frame M: its world matrix (+260 +80), or with a bone tag
// (+264 != -1) that bone's world matrix of the parent's skeleton (vfunc
// +28) with its offset scaled by parent_scale, through the parent's world
// matrix. The child's local matrix through its +160 offset matrix gives
// R and the position T; +168 then picks: 0x00100000 T moved into M's
// frame and M the identity; 0x00040000 M's row lengths set as the child's
// scale (WorldObj_SetScale) and M normalised; else T scaled by the
// parent's +144 scale (M normalised with 0x00080000). The world matrix
// (+80) = M * {R, T}, the dirty bit cleared, the sphere updated
// (0x0057E14C), the children updated with scale = +144 * parent_scale,
// non-unit scale (+164 0x2000) applied to the rows, then vfunc +16 (o, 1).
// Returns 1.
// Quirk kept: without a bone tag the w lanes of M come from stack words of
// the original's frame that are never written; the locals sit at the
// original's frame offsets, and the frame base is placed so that the
// callees (including the recursion) run on the original's stack pointer.
DECOMP(0057E260, WorldObj_UpdateChildLod) {
    const u32 c = arg<u32>(ctx, 0), ps = arg<u32>(ctx, 1);
    Stack frame(ctx, 384 - 112);  // + the 112-byte linkage area call() adds = the original's 384
    const u32 sp = frame.addr() - 112;
    const u32 m = sp + kSpMatrix, t = sp + kSpPos;
    u32 p = load<u32>(c + kObjParentLink);
    const u32 pw = p + obj6::kObjWorld;
    for (u32 row = 0; row < 64; row += 16)
        for (u32 k = 0; k < 12; k += 4) store<u32>(m + row + k, load<u32>(pw + row + k));
    const s32 tag = static_cast<s16>(load<u16>(c + kObjChildTag));
    if (tag != -1) {
        const u32 sk = static_cast<u32>(Vcall0(ctx, p, kVfnGetSkeleton));
        const u32 bone = load<u32>(sk + obj6::kSkelWorldArr) + static_cast<u32>(tag) * 64;
        const f32 z = Fmuls(Lf(bone + 56), Lf(ps + 8));
        const f32 y = Fmuls(Lf(bone + 52), Lf(ps + 4));
        const f32 x = Fmuls(Lf(bone + 48), Lf(ps));
        for (u32 k = 0; k < 16; k += 4) store<f32>(sp + kSpSplat + k, x);  // the splat scratch's last value
        Vec4 s[4], pr[4], out[4];
        for (u32 i = 0; i < 3; ++i)
            s[i] = Vec4{{load<u32>(bone + i * 16), load<u32>(bone + i * 16 + 4), load<u32>(bone + i * 16 + 8), 0}};
        s[3] = Vec4{{std::bit_cast<u32>(x), std::bit_cast<u32>(y), std::bit_cast<u32>(z), 0}};
        MatLoad(pw, pr);
        MatCombine(pr, s, out);
        MatStore(m, out);
        p = load<u32>(c + kObjParentLink);
    }

    // R = the local matrix through the offset matrix; its translation row is T.
    Vec4 lr[4], om[4], r[4];
    MatLoad(c + obj6::kObjLocal, lr);
    MatLoad(load<u32>(c + obj6::kObjOffsetMtx), om);
    MatCombine(lr, om, r);
    VStore(t, r[3]);
    const u32 w168 = load<u32>(c + 168);
    if ((w168 & kInheritPosOnly) != 0) {
        Vec4 mr[4];
        MatLoad(m, mr);
        Vec4 a = VMadd(mr[0], VSplat(VLoadLeft(t), 0), VZero());
        a = VMadd(mr[1], VSplat(VLoadLeft(t + 4), 0), a);
        a = VMadd(mr[2], VSplat(VLoadLeft(t + 8), 0), a);
        VStore(t, VAdd(mr[3], a));
        for (u32 row = 0; row < 64; row += 16)
            for (u32 k = 0; k < 12; k += 4) store<u32>(m + row + k, load<u32>(kIdentity + row + k));
    } else if ((w168 & kInheritScaleSet) == 0) {
        const u32 psc = p + obj::kObjScale;
        for (u32 k = 0; k < 12; k += 4) store<f32>(t + k, Fmuls(Lf(t + k), Lf(psc + k)));
        if ((w168 & kInheritNormalize) != 0) call(ctx, kMatrix_NormalizeInPlace, {m});
    } else {
        call(ctx, kDrawItem_GetScale, {m, sp + kSpScale});
        call(ctx, kMatrix_NormalizeInPlace, {m});
        call(ctx, obj::ofn::WorldObj_SetScale, {c, sp + kSpScale, 0});
    }

    Vec4 mr[4], s[4], out[4];
    MatLoad(m, mr);
    s[0] = r[0];
    s[1] = r[1];
    s[2] = r[2];
    s[3] = VLoad(t);
    MatCombine(mr, s, out);
    MatStore(c + obj6::kObjWorld, out);
    store<u32>(c + obj::kObjFlags, load<u32>(c + obj::kObjFlags) & ~obj::kObjDirty);
    call(ctx, kWorldObj_UpdateSphere, {c});
    const u32 cs = sp + kSpChildScale;
    for (u32 ch = load<u32>(c + kObjFirst); ch != 0; ch = load<u32>(ch + kObjNext)) {
        for (u32 k = 0; k < 12; k += 4) store<f32>(cs + k, Fmuls(Lf(c + obj::kObjScale + k), Lf(ps + k)));
        call(ctx, 0x0057E260, {ch, cs});  // recursion
    }
    if ((load<u32>(c + obj::kObjFlags) & obj::kObjNonUnitScale) != 0)
        call(ctx, obj6::o6fn::Matrix_ScaleRows, {c + obj6::kObjWorld, c + obj::kObjScale});
    const u32 opd = load<u32>(load<u32>(c) + kVfnShow);
    call_ptr(ctx, opd, {c, 1, opd, load<u32>(opd)});  // r5 / r6 = OPD and entry
    ret(ctx, 1u);
}
