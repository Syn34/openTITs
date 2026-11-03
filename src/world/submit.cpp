// The render-job steps World_SubmitFrame runs (0x00459xxx).
#include "world.h"

using namespace sly4;
using namespace sly4::world;

namespace {

// Begins the job, runs the callback list, ends the job.
void World_BuildJob(PPUContext& ctx, u32 world, u32 slot, u32 view, u32 list) {
    call(ctx, wfn::Job_Begin, {slot, view, kLayerAll});
    World_RunCallbacks(ctx, world + list);
    Job_End(ctx, slot);
}

// The 3D view setup shared by 004598D0 and 00459E10: near/far and fov
// from tuning, then the matrix built from World +0x1281B0.
void World_Setup3DView(PPUContext& ctx, u32 world, u32 view) {
    const u8 flags = field<u8>(view, kViewFlags);
    copy_word(view + kViewNear, kTuneNear + kTuningValue);
    copy_word(view + kViewFar, kTuneFar + kTuningValue);
    field<u8>(view, kViewFlags) = static_cast<u8>(flags | 0x20);
    // r4/r5 hold the tuning addresses here in the original.
    ctx.fpr[1] = field<f32>(kTuneFov, kTuningValue);
    call(ctx, wfn::View_SetFov, {view, kTuneFar, kTuneFov});
    Stack frame(ctx, 64);
    call(ctx, wfn::Matrix_Build, {frame.addr(), world + kWorld1281B0, kMatrixArgA, kMatrixArgB});
    call(ctx, wfn::View_SetMatrix, {view, frame.addr()});
    call(ctx, wfn::View_Set5115F0, {view, 0});
}

}  // namespace

// void World_SubmitList308(World*)  (name?)
DECOMP(00459308, World_SubmitList308) {
    const u32 world = arg<u32>(ctx, 0);
    const u32 view = field<u32>(world, kWorldViews + 4);
    if (load<u16>(world + kList308 + 8) != 0) World_BuildJob(ctx, world, kJobSlot308, view, kList308);
    Job_Queue(ctx, kJobSlot308);
}

// void World_SubmitList500(World*)  (name?)
DECOMP(00459500, World_SubmitList500) {
    const u32 world = arg<u32>(ctx, 0);
    const u32 view = field<u32>(world, kWorldViews + 4);
    if (load<u16>(world + kList500 + 8) != 0) World_BuildJob(ctx, world, kJobSlot500, view, kList500);
    Job_Queue(ctx, kJobSlot500);
}

// void World_SubmitOverlay(World*, bool flag)  (name?)
// Always builds the job: on the main view, with Ui_Render(flag, view) first.
DECOMP(004596E8, World_SubmitOverlay) {
    const u32 world = arg<u32>(ctx, 0);
    const u32 flag = arg<u32>(ctx, 1) & 0xFF;
    const u32 view = load<u32>(kMainView);
    call(ctx, wfn::Job_Begin, {kJobSlotOverlay, view, kLayerAll});
    call(ctx, wfn::Ui_Render, {flag, view});
    World_RunCallbacks(ctx, world + kListOverlay);
    Job_End(ctx, kJobSlotOverlay);
    Job_Queue(ctx, kJobSlotOverlay);
}

// void World_Submit3D8D0(World*)  (name?)  view = World views[3]
DECOMP(004598D0, World_Submit3D8D0) {
    const u32 world = arg<u32>(ctx, 0);
    const u32 view = field<u32>(world, kWorldViews + 12);
    if (load<u16>(world + kList8D0 + 8) != 0) {
        World_Setup3DView(ctx, world, view);
        World_BuildJob(ctx, world, kJobSlot8D0, view, kList8D0);
    }
    Job_Queue(ctx, kJobSlot8D0);
}

// void World_Submit3DE10(World*)  (name?)  view = World views[1]; runs
// when World +0x1281C4 points at a nonzero word.
DECOMP(00459E10, World_Submit3DE10) {
    const u32 world = arg<u32>(ctx, 0);
    const u32 obj = field<u32>(world, kWorld1281C4);
    const u32 view = field<u32>(world, kWorldViews + 4);
    if (obj != 0 && load<u32>(obj) != 0) {
        World_Setup3DView(ctx, world, view);
        World_BuildJob(ctx, world, kJobSlotE10, view, kListE10);
    }
    Job_Queue(ctx, kJobSlotE10);
}

// void World_SubmitCameraB60(World*)  (name?)  view = World views[2]
// Copies the main view's camera settings and lens values to this view first.
DECOMP(00459B60, World_SubmitCameraB60) {
    const u32 world = arg<u32>(ctx, 0);
    const u32 view = field<u32>(world, kWorldViews + 8);
    if (load<u16>(world + kListB60 + 8) != 0) {
        const u32 main = field<u32>(world, kWorldViews);
        const u32 dst = field<u32>(field<u32>(view, kViewCamera) + 4, 4);
        const u32 src = field<u32>(field<u32>(main, kViewCamera) + 4, 4);
        ref<u8>(dst + 4) = ref<u8>(src + 4);
        for (const u32 off : {32u, 36u, 40u, 48u}) copy_word(dst + off, src + off);
        ref<u8>(dst + 52) = ref<u8>(src + 52);

        // r4 holds the main view at each of these calls.
        ctx.fpr[1] = field<f32>(main, 64);
        call(ctx, wfn::View_SetF2C8, {view, main});
        const u32 m1 = field<u32>(world, kWorldViews);
        ctx.fpr[1] = field<f32>(m1, 60);
        call(ctx, wfn::View_SetF338, {view, m1});
        const u32 m2 = field<u32>(world, kWorldViews);
        ctx.fpr[1] = field<f32>(m2, 112);
        call(ctx, wfn::View_SetF094, {view, m2});
        const u32 m3 = field<u32>(world, kWorldViews);
        ctx.fpr[1] = fmuls(field<f32>(m3, 56), kRadToDeg);
        call(ctx, wfn::View_SetFov, {view, m3});
        const u32 m4 = field<u32>(world, kWorldViews);
        call(ctx, wfn::View_SetMatrix, {view, m4 + kViewMatrix});
        World_BuildJob(ctx, world, kJobSlotB60, view, kListB60);
    }
    Job_Queue(ctx, kJobSlotB60);
}
