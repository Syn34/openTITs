// Job_Begin: starts building a render job in a job slot.
#include "world.h"

using namespace sly4;
using namespace sly4::world;

// void Job_Begin(JobSlot* slot, View* view, u8 layer)  (name?)
//
// Creates the slot's job (32 bytes, +24 pointing at itself) and pair
// (16 bytes) the first time, neither allocation checked afterwards. Then
// marks the frame arenas as in use by the current world, restores the
// current arena to the job's mark, opens a layer on the current frame
// buffer, and records frame buffer and result in the pair.
DECOMP(00459198, Job_Begin) {
    const u32 slot = arg<u32>(ctx, 0), view = arg<u32>(ctx, 1);
    const u32 layer = arg<u32>(ctx, 2) & 0xFF;
    if (load<u32>(slot) == 0) {
        u32 job = static_cast<u32>(call(ctx, fn::Mem_Alloc, {32, 0, 16}));
        if (job != 0) {
            for (const u32 off : {0u, 4u, 8u, 12u, 16u, 20u, 28u}) store<u32>(job + off, 0);
            store<u32>(job + 24, job);
        }
        store<u32>(slot, job);
        u32 pair = static_cast<u32>(call(ctx, fn::Mem_Alloc, {16, 0, 16}));
        if (pair != 0)
            for (const u32 off : {8u, 0u, 12u, 4u}) store<u32>(pair + off, 0);
        store<u32>(slot + 4, pair);
    }

    field<u32>(load<u32>(kCurrentWorld), kWorldArenas) = load<u32>(kFrameArenas);
    call(ctx, wfn::Arena_Restore, {field<u32>(load<u32>(kFrameArenas), 8), field<u32>(load<u32>(slot), 24)});
    const u32 fb = field<u32>(field<u32>(load<u32>(kCurrentWorld), kWorldFrameRing), 12);
    const u32 result = static_cast<u32>(call(ctx, wfn::FrameBuffer_BeginLayer, {fb, view, layer}));
    const u32 pair = load<u32>(slot + 4);
    store<u32>(pair, fb);
    store<u32>(pair + 4, result);
    store<u32>(kJobFrameBuffer, fb);
    store<u32>(kJobView, view);
    ref<u8>(slot + 8) = 1;
}
