// World_HandleEvent callees, part 10g: the track item update.
#include <cmath>

#include "hevent10.h"

using namespace sly4;
using namespace sly4::hevent;
using namespace sly4::hevent7;
using namespace sly4::hevent10;

namespace sly4 {
namespace hevent10g {
namespace gfn10 {
constexpr u32 Child_Place31D4A4 = 0x0031D4A4;  // (child, const Mat34* m) (name?)
}  // namespace gfn10

// Track item: +0 Group*[] (+8 u16 count; each group {+0 Child*[], +8 u16
// count}), +20 source (+48 3x4 matrix rows at 48 / 64 / 80 / 96, +112 vec3
// velocity, +128 f32 duration), +24 -> +24 -> +4 f32 speed, +28 f32 time,
// +32 f32 fade time, +36 f32 fade left, +40 s32 state (0 fading in, 1 done,
// 2 fading out, 3 finished). Child vtable +52: (child, u8 alpha, 1).
constexpr u32 kItGroups = 0, kItGroupCount = 8, kItSrc = 20, kItSpeedRef = 24, kItTime = 28;
constexpr u32 kItFade = 32, kItFadeLeft = 36, kItState = 40;
constexpr u32 kSrcMatrix = 48, kSrcVel = 112, kSrcDuration = 128;
constexpr u32 kVAlpha = 52;
constexpr u32 kConsts = 0x0031DCB0;  // f32 {0, 1, 255} (code segment)
}  // namespace hevent10g
}  // namespace sly4

using namespace sly4::hevent10g;

namespace {
bool le(f64 a, f64 b) { return a < b || a == b; }
}  // namespace

// u8 Item_Update31DCC0(Item* it, f32 dt) (name?): advances the time by dt
// times the speed, places every group at the source matrix moved along
// the velocity (0x0031D4A4), then runs the fade: fading in (0) or out (2)
// counts the fade down and sets every child's alpha (255 * progress, in
// reverse when fading in); a finished fade goes to 3 (out) or 1 (in, fade
// left 0). Outside a fade, passing the duration starts the fade out (or
// finishes without one). Returns state != 3. f1 as the original leaves it.
DECOMP(0031DCC0, Item_Update31DCC0) {
    const u32 it = arg<u32>(ctx, 0);
    const f64 dt = farg(ctx, 0);
    const u32 src = load<u32>(it + kItSrc);
    const f64 speed = lfs_f64(load<u32>(load<u32>(it + kItSpeedRef) + 24) + 4);
    const f32 t = static_cast<f32>(std::fma(speed, dt, lfs_f64(it + kItTime)));
    store<f32>(it + kItTime, t);
    Stack frame(ctx, 64);
    const u32 m = frame.addr();
    for (const u32 k : {0u, 4u, 8u, 16u, 20u, 24u, 32u, 36u, 40u, 48u, 52u, 56u})
        store<u32>(m + k, load<u32>(src + kSrcMatrix + k));
    for (u32 j = 0; j < 3; ++j)
        store<f32>(m + 48 + 4 * j, static_cast<f32>(std::fma(lfs_f64(src + kSrcVel + 4 * j), static_cast<f64>(t),
                                                             lfs_f64(src + kSrcMatrix + 48 + 4 * j))));
    for (u32 i = 0; i < load<u16>(it + kItGroupCount); ++i) {
        const u32 g = load<u32>(load<u32>(it + kItGroups) + i * 4);
        if (g != 0) call(ctx, gfn10::Child_Place31D4A4, {g, m});
    }
    const s32 st = static_cast<s32>(load<u32>(it + kItState));
    if (st == 3) {
        ret(ctx, 0u);
        return;
    }
    const f64 now = lfs_f64(it + kItTime);
    const bool past = now > lfs_f64(load<u32>(it + kItSrc) + kSrcDuration);
    ctx.fpr[1] = now;
    const bool fading = st > 0 ? st == 2 : st == 0;
    if (fading) {
        const f64 zero = lfs_f64(kConsts);
        f64 left = static_cast<f32>(lfs_f64(it + kItFadeLeft) - dt);
        if (left < zero) left = zero;
        store<f32>(it + kItFadeLeft, static_cast<f32>(left));
        f64 p = zero;
        const f64 fade = lfs_f64(it + kItFade);
        if (fade > p) p = static_cast<f32>(left / fade);
        if (st == 0) p = static_cast<f32>(lfs_f64(kConsts + 4) - p);
        const u32 alpha = static_cast<u32>(rt::fctiw(fmuls(p, lfs_f64(kConsts + 8)), true)) & 0xFF;
        for (u32 i = 0; i < load<u16>(it + kItGroupCount); ++i) {
            const u32 g = load<u32>(load<u32>(it + kItGroups) + i * 4);
            if (g == 0) continue;
            for (u32 j = 0; j < load<u16>(g + kItGroupCount); ++j) {
                const u32 c = load<u32>(load<u32>(g + kItGroups) + j * 4);
                if (c != 0) vcall(ctx, c, kVAlpha, {c, alpha, 1u});
            }
        }
        const f64 f1 = lfs_f64(it + kItFadeLeft);
        ctx.fpr[1] = f1;
        if (le(f1, zero)) {
            if (static_cast<s32>(load<u32>(it + kItState)) == 2) {
                store<u32>(it + kItState, 3);
            } else {
                store<u32>(it + kItFadeLeft, 0);
                store<u32>(it + kItState, 1);
            }
        }
    } else if (past) {
        const f64 fade = lfs_f64(it + kItFade);
        ctx.fpr[1] = fade;
        if (fade > lfs_f64(kConsts)) {
            store<u32>(it + kItFadeLeft, load<u32>(it + kItFade));
            store<u32>(it + kItState, 2);
        } else {
            store<u32>(it + kItState, 3);
        }
    }
    ret(ctx, load<u32>(it + kItState) != 3 ? 1u : 0u);
}
