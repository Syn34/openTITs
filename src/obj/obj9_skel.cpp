// World objects, part 9: Skeleton_Update, which turns a skeleton's local
// bone matrices into world matrices.
#include "obj9.h"

using namespace sly4;
using namespace sly4::obj8;

namespace {

constexpr u32 kSkelDirty = 0x80;        // +31: the world matrices are stale
constexpr u32 kSkelWatched = 0x20;      // +31: on the watch list (obj4::kSkeletonWatch)
constexpr u32 kSkelScaledPos = 0x40;    // +31: bone offsets scaled by the parent's scale vector
constexpr u32 kWatchCount = 4, kWatchCap = 8;

// The root bone: rows 0..3, x / y / z only (the w lanes are kept).
void CopyRoot(u32 world, u32 local) {
    for (u32 row = 0; row < 64; row += 16)
        for (u32 k = 0; k < 12; k += 4) store<u32>(world + row + k, load<u32>(local + row + k));
}

}  // namespace

// void Skeleton_Update(ApeSkeleton* sk)
// Only for a dirty skeleton (+31 0x80, cleared): queues it on the watch
// list once (+31 0x20, when the list has room), then the world matrices:
// the root bone's is its local matrix, every other bone's is its parent's
// world matrix (+52 parent indices, parents first) times its local
// matrix. With +31 0x40 the bone's translation row is then replaced by its
// local offset scaled by the parent's +48 scale vector, through the
// parent's world matrix.
DECOMP(0057E8D0, Skeleton_Update) {
    const u32 sk = arg<u32>(ctx, 0);
    u32 b = load<u8>(sk + obj6::kSkelFlags);
    if ((b & kSkelDirty) == 0) return;
    b &= ~kSkelDirty;
    store<u8>(sk + obj6::kSkelFlags, static_cast<u8>(b));
    if ((b & kSkelWatched) == 0) {
        const u32 w = load<u32>(obj4::kSkeletonWatch);
        const u32 n = load<u32>(w + kWatchCount);
        if (n < load<u32>(w + kWatchCap)) {
            const u32 data = load<u32>(w);
            store<u32>(w + kWatchCount, n + 1);
            store<u32>(data + (n << 2), sk);
            b = load<u8>(sk + obj6::kSkelFlags) | kSkelWatched;
            store<u8>(sk + obj6::kSkelFlags, static_cast<u8>(b));
        }
    }
    const u32 local = load<u32>(sk + obj6::kSkelLocalArr), world = load<u32>(sk + obj6::kSkelWorldArr);
    CopyRoot(world, local);
    ctx.fpr[1] = static_cast<f64>(obj9::Lf(local + 56));  // f1 as the original leaves it
    const s32 count = load<u16>(sk + obj5::kSkelBoneCount);
    for (s32 i = 1; i < count; ++i) {
        const u32 l = local + static_cast<u32>(i) * 64, w = world + static_cast<u32>(i) * 64;
        const s32 parent = static_cast<s16>(load<u16>(load<u32>(sk + obj6::kSkelParents) + static_cast<u32>(i) * 2));
        const u32 wp = world + static_cast<u32>(parent) * 64;
        Vec4 lr[4], pr[4], out[4];
        MatLoad(l, lr);
        MatLoad(wp, pr);
        MatCombine(pr, lr, out);
        MatStore(w, out);
        if ((b & kSkelScaledPos) == 0) continue;
        const u32 scale = load<u32>(sk + obj6::kSkelVecArr) + static_cast<u32>(parent) * 16;
        const f32 x = obj9::Fmuls(obj9::Lf(l + 48), obj9::Lf(scale));
        const f32 y = obj9::Fmuls(obj9::Lf(l + 52), obj9::Lf(scale + 4));
        const f32 z = obj9::Fmuls(obj9::Lf(l + 56), obj9::Lf(scale + 8));
        ctx.fpr[1] = static_cast<f64>(x);
        MatLoad(wp, pr);  // reloaded after the stores
        Vec4 a = VMadd(pr[0], VFill(std::bit_cast<u32>(x)), VZero());
        a = VMadd(pr[1], VFill(std::bit_cast<u32>(y)), a);
        a = VMadd(pr[2], VFill(std::bit_cast<u32>(z)), a);
        VStore(w + 48, VAdd(pr[3], a));
    }
}
