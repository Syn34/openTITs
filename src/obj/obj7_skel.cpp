// World objects, part 7: composing a skeleton's bone matrices.
#include <cmath>

#include "obj7.h"

using namespace sly4;
using namespace sly4::obj5;
using namespace sly4::obj6;

namespace {

constexpr u32 kSkelScaled = 0x40;  // +31 bit: the bones are scaled by their vec3s (name?)
constexpr u32 kComposeFrame = 224;  // the original's frame: the scaled matrix at sp+112
constexpr u32 kComposeLocal = 112;

struct Q {
    f32 v[4];
};

inline Q Ld(u32 ea) {  // lvx
    Q r;
    const u32 a = ea & ~15u;
    for (u32 i = 0; i < 4; ++i) r.v[i] = load<f32>(a + i * 4);
    return r;
}

inline void St(u32 ea, const Q& q) {  // stvx
    const u32 a = ea & ~15u;
    for (u32 i = 0; i < 4; ++i) store<f32>(a + i * 4, q.v[i]);
}

// out = the four rows at m times the bind rows at b: out_i = m0 * b_i.x +
// m1 * b_i.y + m2 * b_i.z (vmaddfp chains from +0, lane by lane), plus m3
// for i = 3 (vaddfp). All four lanes, as the original.
void Compose(u32 m, u32 b, u32 out) {
    const Q mm[4] = {Ld(m), Ld(m + 16), Ld(m + 32), Ld(m + 48)};
    const Q bb[4] = {Ld(b), Ld(b + 16), Ld(b + 32), Ld(b + 48)};
    Q rows[4];
    for (u32 i = 0; i < 4; ++i) {
        const Q& bi = bb[i];
        Q& r = rows[i];
        for (u32 k = 0; k < 4; ++k) {
            f32 a = std::fmaf(mm[0].v[k], bi.v[0], 0.0f);
            a = std::fmaf(mm[1].v[k], bi.v[1], a);
            a = std::fmaf(mm[2].v[k], bi.v[2], a);
            r.v[k] = i == 3 ? mm[3].v[k] + a : a;
        }
    }
    for (u32 i = 0; i < 4; ++i) St(out + 16 * i, rows[i]);
}

}  // namespace

// void Skeleton_Compose(ApeSkeleton* s, Matrix* out)
// out[i] = the bone's world matrix (+40) times its bind matrix (the
// def's bone +0, 112-byte bones) for each bone. With +31 bit 0x40 each
// world matrix is first copied (three floats per row) to the stack and its
// rows scaled by the bone's vec3 (+48, Matrix_ScaleRows); the copy's w
// lanes are whatever the stack held, as in the original (the same stack
// slots are used: the frame is the original's 224 bytes). Without the bit the
// matrices are used as they are; quirk kept: that path composes bone 0
// even when the skeleton has no bones.
DECOMP(00580EA0, Skeleton_Compose) {
    const u32 s = arg<u32>(ctx, 0), out = arg<u32>(ctx, 1);
    if ((load<u8>(s + kSkelFlags) & kSkelScaled) != 0) {
        if (static_cast<s32>(load<u16>(s + kSkelBoneCount)) <= 0) return;
        Stack frame(ctx, kComposeFrame);
        const u32 m = frame.addr() + kComposeLocal;
        u32 i = 0;
        do {
            const u32 w = load<u32>(s + kSkelWorldArr) + i * kMatrixSize;
            for (u32 row = 0; row < 64; row += 16)
                for (u32 k = 0; k < 12; k += 4) store<u32>(m + row + k, load<u32>(w + row + k));
            call(ctx, o6fn::Matrix_ScaleRows, {m, load<u32>(s + kSkelVecArr) + i * 16});
            const u32 b = load<u32>(load<u32>(s + kSkelDef) + kSkelDefBones) + i * kBoneDefSize;
            ++i;
            Compose(m, b, out + (i - 1) * kMatrixSize);
        } while (static_cast<s32>(i) < static_cast<s32>(load<u16>(s + kSkelBoneCount)));
        return;
    }
    const u32 bones = load<u32>(load<u32>(s + kSkelDef) + kSkelDefBones);
    const u32 w = load<u32>(s + kSkelWorldArr);
    Compose(w, bones, out);
    const u32 n = load<u16>(s + kSkelBoneCount);
    if (static_cast<s32>(n) <= 1) return;
    for (u32 i = 1; i < n; ++i) Compose(w + i * kMatrixSize, bones + i * kBoneDefSize, out + i * kMatrixSize);
}
