// View_SubmitSplit: sets up one split-screen view from its camera and
// starts its render job.
#include "view2.h"

#include <cmath>

#include "../game/frame.h"
#include "camera.h"

namespace sly4::view {
using namespace opentit::native;

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

}  // namespace

// Nothing without a view and a camera. When the camera has a transform
// for split `index`, the view gets its inverse (VMX: rotation rows
// transposed, translation -t through them; the w lanes carry r1.y /
// r0.x / r0.z of the source as the original's permutes leave them), its
// fov and 0.1 (View_SetF338). Then: Job_Begin(slot, view, 255), the
// split's two colours, its two optional extras, the job markers cleared,
// a marker pushed on the world's frame ring and the arena snapshot taken
// into the job. (The original leaves r3 = 0x001285E8; no caller reads it.)
void View_SubmitSplit(void* w, void* view, void* cam, u32 index, be_u32* slot) {
    if (!view || !cam) return;
    GuestFrame frame(144);  // mirrors the original's 256-byte frame
    u8* sp = frame.as<u8>(0) - 112;
    if (const u32 xf = Camera_GetSplitXform(cam, index); xf != 0) {
        const be_f32* m = &gref<be_f32>(xf + 112);
        const be_f32 *r0 = m, *r1 = m + 4, *r2 = m + 8, *t = m + 12;
        const f32 nt[3] = {0.0f - t[0], 0.0f - t[1], 0.0f - t[2]};  // vsubfp 0 - t
        const f32 a[4] = {r0[0], r1[0], r2[0], r1[1]};  // vmrghw x2
        const f32 b[4] = {r0[1], r1[1], r2[1], r0[0]};  // vperm mask 0
        const f32 c[4] = {r0[2], r1[2], r2[2], r0[2]};  // vperm mask 1
        be_f32* out = reinterpret_cast<be_f32*>(sp + 112);
        for (u32 j = 0; j < 4; ++j) {
            f32 p = std::fma(a[j], nt[0], 0.0f);
            p = std::fma(b[j], nt[1], p);
            p = std::fma(c[j], nt[2], p);
            out[j] = a[j];
            out[4 + j] = b[j];
            out[8 + j] = c[j];
            out[12 + j] = p;
        }
        call(sfn::View_SetMatrix, view, out);
        call(sfn::View_SetFov, view, gref<be_f32>(xf + 176).get());
        call(sfn::View_SetF338, view, global<be_f32>(kSplitConsts + 32).get());
    }
    call(sfn::Job_Begin, slot, view, 255u);
    Camera_GetSplitColourA(sp + 184, cam, index);
    for (u32 i = 0; i < 4; ++i) sp[176 + i] = sp[184 + i];
    Camera_GetSplitColourB(sp + 188, cam, index);
    for (u32 i = 0; i < 4; ++i) sp[180 + i] = sp[188 + i];
    View_DrawBackdrop(w, view, sp + 176, sp + 180);
    if (const u32 x = Camera_GetSplitA(cam, index)) call(sfn::SplitA_Apply, x, 1u);
    if (const u32 x = Camera_GetSplitB(cam, index)) call(sfn::SplitB_Apply, x, gref<be_u32>(guest(view) + 516).get(), 0u);
    global<be_u32>(wf::kClearedA) = 0;
    global<be_u32>(wf::kClearedB) = 0;
    const u32 world = global<be_u32>(wf::kCurrentWorld);
    wf::FrameBuffer_PushMarker(host<wf::FrameBuffer>(gref<be_u32>(gref<be_u32>(world + wf::kWorldFrameRing) + 12)), world,
                               0x00120000);  // the value the original leaves in r5
    wf::Arena_Snapshot(host<wf::Arena>(gref<be_u32>(global<be_u32>(wf::kFrameArenas) + 8)),
                       host<wf::ArenaSnapshot>(gref<be_u32>(slot->get() + 24)));
    gref<be_u32>(global<be_u32>(wf::kCurrentWorld) + wf::kWorld1285E8) = 0;
}

}  // namespace sly4::view

NATIVE(0045A6F8, sly4::view::View_SubmitSplit)
