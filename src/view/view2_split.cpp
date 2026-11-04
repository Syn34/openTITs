// View_SubmitSplit: sets up one split-screen view from its camera and
// starts its render job.
#include "view2.h"

#include <cmath>

#include "../game/frame.h"

using namespace sly4;

namespace {

namespace wf = sly4::frame;

constexpr u32 kSplitConsts = 0x0045A6D0;  // two vperm masks, then f32 0.1 at +32
namespace sfn {
constexpr u32 Job_Begin = 0x00459198;
constexpr u32 Camera_GetSplitXform = 0x002AB9F0;  // (Camera*, u32 index) -> object: +112 f32[4][4] world matrix, +176 f32 fov (name?)
constexpr u32 Camera_GetSplitColourA = 0x002ABA10;  // (u8 out[4], Camera*, u32 index) (name?)
constexpr u32 Camera_GetSplitColourB = 0x002ABAB8;  // (u8 out[4], Camera*, u32 index) (name?)
constexpr u32 World_SetViewColours = 0x004583F8;    // (World*, View*, const u8[4], const u8[4]) (name?)
constexpr u32 Camera_GetSplitA = 0x002AB9B0;        // (Camera*, u32 index) -> object or 0 (name?)
constexpr u32 SplitA_Apply = 0x000D8FC4;            // (object, 1) (name?)
constexpr u32 Camera_GetSplitB = 0x002AB9D0;        // (Camera*, u32 index) -> object or 0 (name?)
constexpr u32 SplitB_Apply = 0x004FEF48;            // (object, u32 view +516, 0) (name?)
constexpr u32 View_SetMatrix = 0x005201B0;          // (View*, const f32[16])
constexpr u32 View_SetFov = 0x00520368;             // (View*, f32)
constexpr u32 View_SetF338 = 0x00520338;            // (View*, f32) (name?)
}  // namespace sfn

f32 lane(u32 v, u32 i) { return load<f32>(v + 4 * i); }

}  // namespace

// void View_SubmitSplit(World* w, View* view, Camera* cam, u32 index, JobSlot* slot)
// Nothing without a view and a camera. When the camera has a transform
// for split `index`, the view gets its inverse (VMX: rotation rows
// transposed, translation -t through them; the w lanes carry r1.y /
// r0.x / r0.z of the source as the original's permutes leave them), its
// fov and 0.1 (View_SetF338). Then: Job_Begin(slot, view, 255), the
// split's two colours, its two optional extras, the job markers cleared,
// a marker pushed on the world's frame ring and the arena snapshot taken
// into the job.
// Returns 0x001285E8 in r3, as the original's last address computation
// leaves it.
DECOMP(0045A6F8, View_SubmitSplit) {
    const u32 w = arg<u32>(ctx, 0), view = arg<u32>(ctx, 1), cam = arg<u32>(ctx, 2);
    const u32 index = arg<u32>(ctx, 3), slot = arg<u32>(ctx, 4);
    if (view == 0 || cam == 0) return;  // r3 = w
    Stack frame(ctx, 144);  // mirrors the original's 256-byte frame
    const u32 sp = frame.addr() - 112;

    if (const u32 xf = static_cast<u32>(call(ctx, sfn::Camera_GetSplitXform, {cam, index})); xf != 0) {
        const u32 m = xf + 112;
        const u32 r0 = m, r1 = m + 16, r2 = m + 32, t = m + 48;
        const f32 nt[3] = {0.0f - lane(t, 0), 0.0f - lane(t, 1), 0.0f - lane(t, 2)};  // vsubfp 0 - t
        const f32 a[4] = {lane(r0, 0), lane(r1, 0), lane(r2, 0), lane(r1, 1)};  // vmrghw x2
        const f32 b[4] = {lane(r0, 1), lane(r1, 1), lane(r2, 1), lane(r0, 0)};  // vperm mask 0
        const f32 c[4] = {lane(r0, 2), lane(r1, 2), lane(r2, 2), lane(r0, 2)};  // vperm mask 1
        const u32 out = sp + 112;
        for (u32 j = 0; j < 4; ++j) {
            f32 p = std::fma(a[j], nt[0], 0.0f);
            p = std::fma(b[j], nt[1], p);
            p = std::fma(c[j], nt[2], p);
            store<f32>(out + 4 * j, a[j]);
            store<f32>(out + 16 + 4 * j, b[j]);
            store<f32>(out + 32 + 4 * j, c[j]);
            store<f32>(out + 48 + 4 * j, p);
        }
        call(ctx, sfn::View_SetMatrix, {view, out});
        callf(ctx, sfn::View_SetFov, {view}, {static_cast<f64>(load<f32>(xf + 176))});
        callf(ctx, sfn::View_SetF338, {view}, {static_cast<f64>(load<f32>(kSplitConsts + 32))});
    }

    call(ctx, sfn::Job_Begin, {slot, view, 255});
    call(ctx, sfn::Camera_GetSplitColourA, {sp + 184, cam, index});
    for (u32 i = 0; i < 4; ++i) ref<u8>(sp + 176 + i) = ref<u8>(sp + 184 + i);
    call(ctx, sfn::Camera_GetSplitColourB, {sp + 188, cam, index});
    for (u32 i = 0; i < 4; ++i) ref<u8>(sp + 180 + i) = ref<u8>(sp + 188 + i);
    call(ctx, sfn::World_SetViewColours, {w, view, sp + 176, sp + 180});
    if (const u32 x = static_cast<u32>(call(ctx, sfn::Camera_GetSplitA, {cam, index})); x != 0)
        call(ctx, sfn::SplitA_Apply, {x, 1});
    if (const u32 x = static_cast<u32>(call(ctx, sfn::Camera_GetSplitB, {cam, index})); x != 0)
        call(ctx, sfn::SplitB_Apply, {x, load<u32>(view + 516), 0});

    store<u32>(wf::kClearedA, 0);
    store<u32>(wf::kClearedB, 0);
    const u32 world = load<u32>(wf::kCurrentWorld);
    // FrameBuffer_PushMarker reads r5 (its value): the original leaves
    // 0x00120000 there (the high half of the frame-ring offset), and the
    // current world in r4.
    call(ctx, wf::wfn::FrameBuffer_PushMarker, {load<u32>(load<u32>(world + wf::kWorldFrameRing) + 12), world, 0x00120000});
    call(ctx, wf::wfn::Arena_Snapshot,
         {load<u32>(load<u32>(wf::kFrameArenas) + 8), load<u32>(load<u32>(slot) + 24)});
    store<u32>(load<u32>(wf::kCurrentWorld) + wf::kWorld1285E8, 0);
    ctx.gpr[3] = wf::kWorld1285E8;
}
