// World_SubmitFrame: queues the frame's render jobs after the world update.
#include "frame.h"

using namespace sly4;
using namespace sly4::frame;

namespace {

// Appends {fn, arg} to kRenderList, growing it (to twice the capacity) when
// full, and keeps the high-water mark at +10. Inlined five times in the original.
void RenderList_Push(PPUContext& ctx, u32 fn, u32 arg) {
    const u32 list = kRenderList;
    u32 count = load<u16>(list + 8);
    const u32 capacity = load<u16>(list + 4);
    if (count == capacity) {
        call(ctx, wfn::RenderList_Grow, {list, (capacity << 1) & 0x1FFFE});
        count = load<u16>(list + 8);
    }
    const u32 entry = load<u32>(list) + ((count << 3) & 0x7FFF8);
    store<u32>(entry, fn);
    store<u32>(entry + 4, arg);
    const u16 next = static_cast<u16>(load<u16>(list + 8) + 1);
    const u16 high = load<u16>(list + 10);
    store<u16>(list + 8, next);
    store<u16>(list + 10, static_cast<s32>(high) > static_cast<s32>(next) ? high : next);
}

// A 36-byte arena with a 40 KB buffer (0 if the arena can't be allocated;
// the buffer allocation isn't checked).
u32 Frame_NewArena(PPUContext& ctx) {
    const u32 arena = static_cast<u32>(call(ctx, fn::Mem_Alloc, {36, 0, 16}));
    if (arena == 0) return 0;
    field<u32>(arena, 0) = kArenaSize;
    const u32 buffer = static_cast<u32>(call(ctx, fn::Mem_Alloc, {kArenaSize, 0, 16}));
    const u32 end = buffer + field<u32>(arena, 0);
    const u8 flags = field<u8>(arena, 32);
    field<u32>(arena, 4) = buffer;
    field<u32>(arena, 12) = buffer;
    field<u32>(arena, 24) = 0;
    field<u32>(arena, 20) = 0;
    field<u32>(arena, 8) = end;
    field<u32>(arena, 16) = end;
    field<u32>(arena, 28) = 1;
    field<u8>(arena, 32) = static_cast<u8>(flags & ~0xC0);
    return arena;
}

// The double-buffered frame arenas, built the first time.
void Frame_CreateArenas(PPUContext& ctx) {
    u32 pair = 0;
    const u32 mem = static_cast<u32>(call(ctx, fn::Mem_Alloc, {16, 0, 16}));
    if (mem != 0) {
        field<u32>(mem, 0) = Frame_NewArena(ctx);
        const u32 second = Frame_NewArena(ctx);
        const u32 first = field<u32>(mem, 0);
        field<u32>(mem, 4) = second;
        field<u32>(mem, 12) = 0;
        field<u32>(mem, 8) = first;
        pair = mem;
    }
    store<u32>(kFrameArenas, pair);
}

// Runs a callback list of 12-byte {opd, arg, -} entries with f1 = 0.0. The
// original loads r3 with the code address from the descriptor (not an
// object), so that is what the callbacks get in r3; arg goes in r4.
void World_RunCallbacks(PPUContext& ctx, u32 list) {
    for (u32 i = 0, offset = 0; i < load<u16>(list + 8); ++i, offset += 12) {
        const u32 entry = load<u32>(list) + offset;
        const u32 opd = load<u32>(entry);
        const u32 arg = load<u32>(entry + 4);
        ctx.fpr[1] = 0.0;  // f32 at 0x0045A9B0
        call_ptr(ctx, opd, {load<u32>(opd), arg});
    }
}

}  // namespace

// void World_SubmitFrame(World* world)  (name?)
//
// After the world update: queues the view jobs, runs the per-frame
// callback lists and the 0x00459xxx submit steps, and always ends with the
// end-of-frame job. With Game +0x5E40 bit 0x80 only the end job is queued.
DECOMP(0045A9B8, World_SubmitFrame) {
    const u32 world = arg<u32>(ctx, 0);
    const u32 game = g::game;
    if (field<u8>(game, kGameFlags5E40) & 0x80) return RenderList_Push(ctx, kJobEndFrame, kJobEndFrameArg);

    if (load<u32>(kFrameArenas) == 0) Frame_CreateArenas(ctx);

    if (const u32 camera = load<u32>(kCamera)) {
        if (static_cast<u32>(call(ctx, wfn::Camera_IsSplit, {camera})) != 0 &&
            (field<u32>(camera, kCameraSplitBits) >> 28) != 0) {
            ref<u8>(kJobEndFrameArg + 4 + (static_cast<u32>(ref<u8>(kJobSplitIndex)) << 3)) |= 0x80;
            call(ctx, wfn::View_SubmitSplit, {world, field<u32>(world, kWorldSplitViews), camera, 0, kJobView0});
            call(ctx, wfn::View_SubmitSplit, {world, field<u32>(world, kWorldSplitViews + 4), camera, 1, kJobView1});
        }
        if (load<u32>(kJobView0) != 0) RenderList_Push(ctx, kJobRun, kJobView0);
        if (load<u32>(kJobView1) != 0) RenderList_Push(ctx, kJobRun, kJobView1);
    }

    call(ctx, wfn::View_Notify, {load<u32>(kSub1BEE8Arg), field<u32>(world, kWorldMainView)});
    World_RunCallbacks(ctx, world + kWorldCallbacksA);
    store<u16>(world + kWorldCallbacksA + 8, 0);

    const u8 w47 = field<u8>(world, kWorld47);
    if ((field<u8>(kTuneSkipA, kTuningValue) != 0 || field<u8>(kTuneSkipB, kTuningValue) != 0) && w47 == 0 &&
        static_cast<s32>(call_ptr(ctx, vfunc(world, kWorldVfnState), {world, 0})) == static_cast<s32>(w47) &&
        !(static_cast<s32>(call_ptr(ctx, vfunc(world, kWorldVfnState), {world, 1})) > 5)) {
        call(ctx, wfn::sub_004596E8, {world, 1});
        return;
    }

    const u32 r = static_cast<u32>(call(ctx, wfn::sub_00457654, {world}));
    if (r == 0) call(ctx, wfn::sub_005AC280, {field<u32>(world, kWorldMainView)});
    if (r != 0 || field<u32>(world, kWorld1281A4) == 0) call(ctx, wfn::sub_0045A208, {world});

    if (const u32 overlay = field<u32>(world, kWorldOverlay)) {
        const u32 v = static_cast<u32>(call(ctx, wfn::Overlay_GetValue, {overlay}));
        call(ctx, wfn::sub_00459198, {kJobOverlay, v, 255});
        call(ctx, wfn::sub_000AB7B0, {field<u32>(world, kWorldOverlay)});
        const u32 current = load<u32>(kCurrentWorld);
        store<u32>(kClearedA, 0);
        store<u32>(kClearedB, 0);
        // FrameBuffer_PushMarker reads r5, which the original leaves holding
        // the frame-ring offset 0x1285E4 (and r4 = 0x00F60000, unused).
        call(ctx, wfn::FrameBuffer_PushMarker,
             {field<u32>(field<u32>(current, kWorldFrameRing), 12), 0x00F60000, kWorldFrameRing});
        call(ctx, wfn::Arena_Snapshot, {field<u32>(load<u32>(kFrameArenas), 8), field<u32>(load<u32>(kJobOverlay), 24)});
        field<u32>(load<u32>(kCurrentWorld), kWorld1285E8) = 0;
        if (load<u32>(kJobOverlay) != 0) RenderList_Push(ctx, kJobRun, kJobOverlay);
    }

    call(ctx, wfn::sub_00459E10, {world});
    call(ctx, wfn::sub_00459B60, {world});
    call(ctx, wfn::sub_004598D0, {world});
    const u32 flag = (field<u32>(game, kGame5E38) == 0 && field<u8>(kTuneNoFlag, kTuningValue) == 0) ? 1u : 0u;
    call(ctx, wfn::sub_004596E8, {world, flag});
    call(ctx, wfn::sub_00459500, {world});
    call(ctx, wfn::sub_00459308, {world});
    World_RunCallbacks(ctx, world + kWorldCallbacksB);
    RenderList_Push(ctx, kJobEndFrame, kJobEndFrameArg);
}
