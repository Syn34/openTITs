// World_HandleEvent callees, part 8e: the view query job posted from the
// handler (a 176-byte job on the frame stack: radius, the main view's
// matrix and the first user's matrix).
#include "hevent8.h"

using namespace sly4;
using namespace sly4::hevent;
using namespace sly4::hevent7;
using namespace sly4::hevent8;

namespace sly4 {
namespace hevent8e {
namespace efn8 {
constexpr u32 MemStack_Alloc = 0x0055260C;  // (MemStack*, size)
constexpr u32 Job_Post456918 = 0x00456918;  // (Job*) (name?)
}  // namespace efn8

constexpr u32 kQueryList = 0x01013B34;      // {+0 u32 data, +8 u16 count} (name?)
constexpr u32 kQueryRadius = 0x00F64734;    // f32, -1.0 initially: < 0 uses kQueryRadiusAlt (name?)
constexpr u32 kQueryRadiusAlt = 0x01021C84; // f32 (name?)
constexpr u32 kZeroF = 0x000CFBC8;          // f32 0.0
constexpr u32 kFrameStack = 0x010B1A80;     // u32 MemStack* (0x010B19E0 +160)
constexpr u32 kViewMatrix = 816;            // View +816: 4x4 matrix (rows of 4 floats)
constexpr u32 kUserMatrix = 96;             // first user block's +8 object, +96 matrix
// Job words +0/+4/+8/+12/+160.
constexpr u32 kJobW0 = 0x01015BC0, kJobW4 = 0x01015BB0, kJobW8 = 0x01017BD0, kJobW12 = 0x01017BC0, kJobW160 = 0x01021E00;
constexpr u32 kJobSize = 176;
}  // namespace hevent8e
}  // namespace sly4

using namespace sly4::hevent8e;

// void Ev_PostViewQuery0CFBD0() (name?): when the query list at 0x01013B34
// is non-empty, a job from the frame stack gets {+16 list data, +20 count,
// +24 radius^2 (0x00F64734 when >= 0, else 0x01021C84), +32 the main view's
// matrix (3x3 rows and the position), +96 the same of the first user's
// object (+96, or the view's again without one)} and is posted. Floats are
// copied raw (lfs/stfs). A failed alloc is written at 0 + offset, as in the
// original.
DECOMP(000CFBD0, Ev_PostViewQuery0CFBD0) {
    if (load<u16>(kQueryList + 8) == 0) return;
    static constexpr u32 kRow[12] = {0, 4, 8, 16, 20, 24, 32, 36, 40, 48, 52, 56};
    const u32 w = load<u32>(kCurrentWorld);
    const u32 view = load<u32>(w + 0x00128168) + kViewMatrix;
    const u32 blk = w + kUserBlocks;
    u32 m[12], s[12];
    for (u32 k = 0; k < 12; ++k) m[k] = load<u32>(view + kRow[k]);
    const u32 obj = load<u32>(blk + 8);
    for (u32 k = 0; k < 12; ++k) s[k] = obj != 0 ? load<u32>(obj + kUserMatrix + kRow[k]) : m[k];
    f64 r = lfs_f64(kQueryRadius);
    if (!(r >= lfs_f64(kZeroF))) r = lfs_f64(kQueryRadiusAlt);
    const f32 r2 = fmuls(r, r);
    const u32 e = static_cast<u32>(call(ctx, efn8::MemStack_Alloc, {load<u32>(kFrameStack), kJobSize}));
    const u32 data = load<u16>(kQueryList + 8) != 0 ? load<u32>(kQueryList) : 0;
    store<f32>(e + 24, r2);
    store<u32>(e + 32, m[0]);
    store<u32>(e + 36, m[1]);
    store<u32>(e + 40, m[2]);
    const u32 count = load<u16>(kQueryList + 8);
    store<u32>(e + 48, m[3]);
    store<u32>(e + 52, m[4]);
    store<u32>(e + 16, data);
    store<u32>(e + 56, m[5]);
    store<u32>(e + 64, m[6]);
    store<u32>(e + 20, count);
    store<u32>(e + 68, m[7]);
    store<u32>(e + 72, m[8]);
    store<u32>(e + 80, m[9]);
    store<u32>(e + 84, m[10]);
    store<u32>(e + 88, m[11]);
    for (u32 k = 0; k < 12; ++k) store<u32>(e + 96 + kRow[k], s[k]);
    store<u32>(e + 0, kJobW0);
    store<u32>(e + 4, kJobW4);
    store<u32>(e + 8, kJobW8);
    store<u32>(e + 12, kJobW12);
    store<u32>(e + 160, kJobW160);
    ctx.fpr[1] = lfs_f64(e + 104);  // f1 is left as the last value copied, as in the original
    call(ctx, efn8::Job_Post456918, {e});
}
