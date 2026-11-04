// World_HandleEvent callees, part 9c: the pose blend start.
#include "hevent9.h"

using namespace sly4;
using namespace sly4::hevent;
using namespace sly4::hevent7;
using namespace sly4::hevent9;

namespace sly4 {
namespace hevent9c {
namespace cfn {
constexpr u32 Quat_FromMatrix = 0x00555478;  // (f32 out[4], const f32 m[4][4])
}  // namespace cfn

// Object: +96 4x3 matrix (rows of 3 at stride 16), +345 u8 layer, +356 /
// +360 f32 pair for the pose query, +364 Animator*[] by layer. The three
// blends: +768 / +864 / +900 f32 time; +704 / +736 translation, +720 /
// +752 zero, +800 / +832 quat, +816 / +848 zero, +884 / +892 f32 and u32
// pairs.
constexpr u32 kObjMatrix = 96, kObjLayer = 345, kObjQuery = 356, kObjAnimators = 364;
constexpr u32 kBlendA = 768, kBlendB = 864, kBlendC = 900;
constexpr u32 kZeroConsts = 0x000FA650;     // f32 0.0 (and f64 0.0 at +0 / +8)
constexpr u32 kQueryTail = 0x01164CA8;      // 4 bytes copied to the query's +96 (name?)
constexpr u32 kVPose = 48;                  // Animator vtable: (this, f32, Query*)
// The query (104 bytes on the stack): +0 4x3 matrix (stride 16, the w
// slots left unset), +64 / +68 f32, +72 u8, +76..+84 0, +88 u8, +92 0, +96
// 4 bytes. The identity matrix and 55.0 / 7.5 the original writes first are
// overwritten before the call (dead stores, left out).
constexpr u32 kQSize = 104, kQPair = 64;

// fsel: a >= 0 ? c : b (NaN a picks b).
inline f64 sel_ge(f64 a, f64 c, f64 b) { return a >= 0.0 ? c : b; }
}  // namespace hevent9c
}  // namespace sly4

using namespace sly4::hevent9c;

// void Obj_BlendPose0FA660(Object* o, f32 t) (name?): the animator of o's
// current layer (none: nothing). While a blend is running (the largest of
// the three times above 0) and t > 0, only raises all three to t when t is
// larger. Otherwise asks the animator for the pose (vtable +48 with 0.0 and
// a query holding o's matrix) and restarts all three blends at max(t, 0)
// from it: translation, quat and the query's +64 word, the "from" halves
// (+704, +800, +884) too when that time is 0. r3 / f1 as the original
// leaves them.
DECOMP(000FA660, Obj_BlendPose0FA660) {
    const u32 o = arg<u32>(ctx, 0);
    const f64 t = farg(ctx, 0);
    const u32 layer = load<u8>(o + kObjLayer);
    if (layer == 0) return;  // r3 = o, f1 = t
    const u32 slot = o + kObjAnimators + ((layer << 2) & 0x3FC);
    const u32 anim = load<u32>(slot);
    if (anim == 0) {
        ctx.gpr[3] = slot;
        return;
    }
    const f64 zero = lfs_f64(kZeroConsts);
    {
        const f64 a = lfs_f64(o + kBlendA), b = lfs_f64(o + kBlendB), c = lfs_f64(o + kBlendC);
        f64 m = sel_ge(static_cast<f32>(a - b), a, b);
        m = sel_ge(static_cast<f32>(m - c), m, c);
        if (m > zero && t > zero) {
            ctx.gpr[3] = kZeroConsts;
            if (!(t > m)) {
                ctx.fpr[1] = m;
                return;
            }
            const f32 v = static_cast<f32>(sel_ge(-t, std::bit_cast<f64>(load<u64>(kZeroConsts + 8)), t));
            store<f32>(o + kBlendA, v);
            store<f32>(o + kBlendB, v);
            store<f32>(o + kBlendC, v);
            ctx.fpr[1] = v;
            return;
        }
    }

    Stack frame(ctx, 16 + kQSize);  // quat out at r1+112, query at r1+128
    const u32 quat = frame.addr(), q = frame.addr() + 16;
    for (u32 row = 0; row < 4; ++row)
        for (u32 k = 0; k < 12; k += 4) store<u32>(q + 16 * row + k, load<u32>(o + kObjMatrix + 16 * row + k));
    store<u32>(q + kQPair + 0, load<u32>(o + kObjQuery + 0));
    store<u32>(q + kQPair + 4, load<u32>(o + kObjQuery + 4));
    store<u8>(q + 72, 0);
    store<u32>(q + 76, 0);
    store<u32>(q + 80, 0);
    store<u32>(q + 84, 0);
    store<u8>(q + 88, 0);
    store<u32>(q + 92, 0);
    for (u32 k = 0; k < 4; ++k) store<u8>(q + 96 + k, load<u8>(kQueryTail + k));
    {
        // (this, f32 0.0, Query*): the float takes r4's slot, which holds
        // the OPD as in the original; r6 its entry.
        const u32 opd = load<u32>(load<u32>(anim) + kVPose);
        ctx.fpr[1] = zero;
        ctx.gpr[6] = load<u32>(opd);
        call_ptr(ctx, opd, {anim, opd, q});
    }
    const f32 w = static_cast<f32>(sel_ge(-t, std::bit_cast<f64>(load<u64>(kZeroConsts)), t));
    const bool restart = static_cast<f64>(w) <= zero;
    store<f32>(o + kBlendA, w);
    const u32 tx = load<u32>(q + 48), ty = load<u32>(q + 52), tz = load<u32>(q + 56);
    store<u32>(o + 736, tx);
    store<u32>(o + 740, ty);
    store<u32>(o + 744, tz);
    for (u32 k = 0; k < 12; k += 4) store<u32>(o + 752 + k, load<u32>(kZeroVec + k));
    if (restart) {
        store<u32>(o + 704, tx);
        store<u32>(o + 708, ty);
        store<u32>(o + 712, tz);
        for (u32 k = 0; k < 12; k += 4) store<u32>(o + 720 + k, load<u32>(kZeroVec + k));
    }
    call(ctx, cfn::Quat_FromMatrix, {quat, q});
    store<f32>(o + kBlendB, w);
    u32 qv[4];
    for (u32 k = 0; k < 4; ++k) {
        qv[k] = load<u32>(quat + 4 * k);
        store<u32>(o + 832 + 4 * k, qv[k]);
    }
    store<u32>(o + 848, load<u32>(kZeroVec + 0));
    const u32 pair = load<u32>(q + kQPair);
    store<u32>(o + 852, load<u32>(kZeroVec + 4));
    store<u32>(o + 856, load<u32>(kZeroVec + 8));
    if (restart) {
        for (u32 k = 0; k < 4; ++k) store<u32>(o + 800 + 4 * k, qv[k]);
        for (u32 k = 0; k < 12; k += 4) store<u32>(o + 816 + k, load<u32>(kZeroVec + k));
    }
    store<f32>(o + kBlendC, w);
    store<u32>(o + 892, pair);
    store<u32>(o + 896, 0);
    if (restart) {
        store<u32>(o + 884, pair);
        store<u32>(o + 888, 0);
    }
    ctx.gpr[3] = 0;
    ctx.fpr[1] = lfs_f64(q + kQPair);  // f1 as the original leaves it
}
