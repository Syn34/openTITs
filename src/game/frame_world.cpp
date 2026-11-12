// World_SubmitFrame: queues the frame's render jobs after the world update.
#include "frame.h"

#include "../container/container.h"
#include "../memory/mem.h"

namespace sly4::frame {
using namespace opentit::native;

namespace {

constexpr u32 kArenaObjSize = 36;
constexpr u32 kArenaPairSize = 16;
constexpr u32 kTuningSkipValue = kTuningValue;  // u8 at +72 of the tuning objects
constexpr f32 kCallbackTime = 0.0f;              // f32 at 0x0045A9B0

struct RenderJob {
    be_u32 fn;   // OPD
    be_u32 arg;
};

// Appends {fn, arg} to kRenderList, growing it (to twice the capacity) when
// full, and keeps the high-water mark at +10. Inlined five times in the original.
void RenderList_Push(u32 fn, u32 arg) {
    container::PtrVector* list = host<container::PtrVector>(kRenderList);
    u32 count = list->count;
    const u32 capacity = list->capacity;
    if (count == capacity) {
        container::PtrVector_Reserve(list, (capacity << 1) & 0x1FFFE);
        count = list->count;
    }
    RenderJob& job = gref<RenderJob>(list->data.ea + ((count << 3) & 0x7FFF8));
    job.fn = fn;
    job.arg = arg;
    const u16 next = static_cast<u16>(list->count + 1);
    const u16 high = list->high_water;
    list->count = next;
    list->high_water = static_cast<s32>(high) > static_cast<s32>(next) ? high : next;
}

// A 36-byte arena with a 40 KB buffer (0 if the arena can't be allocated;
// the buffer allocation isn't checked).
FrameArenaObj* Frame_NewArena() {
    auto* arena = static_cast<FrameArenaObj*>(memory::Mem_Alloc(kArenaObjSize, 0, 16));
    if (!arena) return nullptr;
    arena->size = kArenaSize;
    const u32 buffer = guest(memory::Mem_Alloc(kArenaSize, 0, 16));
    const u32 end = buffer + arena->size;
    const u8 flags = arena->flags;
    arena->base = buffer;
    arena->cursor = buffer;
    arena->w24 = 0u;
    arena->w20 = 0u;
    arena->end = end;
    arena->end2 = end;
    arena->w28 = 1u;
    arena->flags = static_cast<u8>(flags & ~0xC0);
    return arena;
}

// The double-buffered frame arenas, built the first time.
void Frame_CreateArenas() {
    FrameArenas* pair = static_cast<FrameArenas*>(memory::Mem_Alloc(kArenaPairSize, 0, 16));
    if (pair) {
        pair->a = Frame_NewArena();
        FrameArenaObj* second = Frame_NewArena();
        FrameArenaObj* first = pair->a;
        pair->b = second;
        pair->zero = 0u;
        pair->current = first;
    }
    global<gp<FrameArenas>>(kFrameArenas) = pair;
}

// Runs a callback list of 12-byte {opd, arg, -} entries with f1 = 0.0. The
// original loads r3 with the code address from the descriptor (not an
// object), so that is what the callbacks get in r3; arg goes in r4.
void World_RunCallbacks(FrameCallbackList* list) {
    for (u32 i = 0; i < list->count; ++i) {
        const FrameCallback& entry = list->entries.get()[i];
        const u32 opd = entry.opd;
        const u32 arg = entry.arg;
        call_ptr(opd, *host<be_u32>(opd), arg, kCallbackTime);
    }
}

s32 World_State(World* world, u32 depth) { return vcall<s32>(world, kWorldVfnState, depth); }

}  // namespace

// After the world update: queues the view jobs, runs the per-frame
// callback lists and the 0x00459xxx submit steps, and always ends with the
// end-of-frame job. With Game +0x5E40 bit 0x80 only the end job is queued.  (name?)
void World_SubmitFrame(World* world) {
    const u32 game = g::game;
    if (gref<u8>(game + kGameFlags5E40) & 0x80) return RenderList_Push(kJobEndFrame, kJobEndFrameArg);

    if (!global<gp<FrameArenas>>(kFrameArenas)) Frame_CreateArenas();

    if (Camera* camera = global<gp<Camera>>(kCamera)) {
        if (Camera_IsSplit(camera) && (camera->split_bits >> 28) != 0) {
            global<u8>(kJobEndFrameArg + 4 + (static_cast<u32>(global<u8>(kJobSplitIndex)) << 3)) |= 0x80;
            call(wfn::View_SubmitSplit, world, world->split_views[0].get(), camera, 0u, kJobView0);
            call(wfn::View_SubmitSplit, world, world->split_views[1].get(), camera, 1u, kJobView1);
        }
        if (global<be_u32>(kJobView0) != 0) RenderList_Push(kJobRun, kJobView0);
        if (global<be_u32>(kJobView1) != 0) RenderList_Push(kJobRun, kJobView1);
    }

    View_Notify(host<ViewOwner>(global<be_u32>(kSub1BEE8Arg)), world->views[0]);
    World_RunCallbacks(&world->callbacks_a);
    world->callbacks_a.count = 0;

    const u8 w47 = world->b47;
    if ((gref<u8>(kTuneSkipA + kTuningSkipValue) != 0 || gref<u8>(kTuneSkipB + kTuningSkipValue) != 0) && w47 == 0 &&
        World_State(world, 0) == static_cast<s32>(w47) && !(World_State(world, 1) > 5)) {
        call(wfn::sub_004596E8, world, 1u);
        return;
    }

    const u32 r = call<u32>(wfn::sub_00457654, world);
    if (r == 0) call(wfn::sub_005AC280, world->views[0].get());
    if (r != 0 || world->w1281a4 == 0) call(wfn::sub_0045A208, world);

    if (world->overlay != 0) {
        const u32 v = Overlay_GetValue();
        call(wfn::sub_00459198, kJobOverlay, v, 255u);
        call(wfn::sub_000AB7B0, world->overlay.get());
        World& current = gref<World>(global<be_u32>(kCurrentWorld));
        global<be_u32>(kClearedA) = 0u;
        global<be_u32>(kClearedB) = 0u;
        // FrameBuffer_PushMarker reads r5, which the original leaves holding
        // the frame-ring offset 0x1285E4. (No null checks below, as in the original.)
        FrameBuffer_PushMarker(gref(current.frame_ring).current, 0, kWorldFrameRing);
        Arena_Snapshot(host<Arena>(gref<FrameArenas>(global<be_u32>(kFrameArenas)).current.ea),
                       gref<OverlayJob>(global<be_u32>(kJobOverlay)).snapshot);
        gref<World>(global<be_u32>(kCurrentWorld)).w1285e8 = 0u;
        if (global<be_u32>(kJobOverlay) != 0) RenderList_Push(kJobRun, kJobOverlay);
    }

    call(wfn::sub_00459E10, world);
    call(wfn::sub_00459B60, world);
    call(wfn::sub_004598D0, world);
    const u32 flag = (gref<be_u32>(game + kGame5E38) == 0 && gref<u8>(kTuneNoFlag + kTuningValue) == 0) ? 1u : 0u;
    call(wfn::sub_004596E8, world, flag);
    call(wfn::sub_00459500, world);
    call(wfn::sub_00459308, world);
    World_RunCallbacks(&world->callbacks_b);
    RenderList_Push(kJobEndFrame, kJobEndFrameArg);
}

}  // namespace sly4::frame

NATIVE(0045A9B8, sly4::frame::World_SubmitFrame)
