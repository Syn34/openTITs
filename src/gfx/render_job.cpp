// The render-job execution side: main's end-of-frame call runs the
// {OPD, arg} entries World_SubmitFrame pushed onto the render list
// (frame::kRenderList); the entries are Job_Swap (per view / overlay job)
// and Job_EndFrame (last).
#include "render.h"

#include "../game/frame.h"

using namespace sly4;

namespace {

namespace wf = sly4::frame;

namespace jfn {
constexpr u32 sub_0042AD98 = 0x0042AD98, sub_004291C8 = 0x004291C8, sub_005AC60C = 0x005AC60C;  // ()
constexpr u32 sub_000D0298 = 0x000D0298, sub_002F481C = 0x002F481C;                              // ()
}  // namespace jfn

// World halfwords cleared at the end of a frame in modes 1 and 2.
constexpr u32 kWorldFrameCounters[] = {0x1284EC, 0x128528, 0x128500, 0x12853C, 0x128514, 0x128550, 0x128564};

}  // namespace

// void Job_Swap(JobSlot* s)   (render list entry; code of frame::kJobRun)
// s: +0 Job* (two 12-byte halves at +0 / +12, +24 the half being
// written, +28 the previous one), +4 pair {+0, +4 current; +8, +12
// previous}, +8 u8 building, +9 u8 built.
// Moves "building" to "built", the pair's current values to previous,
// and points the job at its other half, cleared.
DECOMP(00737278, Job_Swap) {
    const u32 s = arg<u32>(ctx, 0);
    const u8 building = ref<u8>(s + 8);
    const u32 pair = load<u32>(s + 4);
    ref<u8>(s + 9) = building;
    ref<u8>(s + 8) = 0;
    const u32 p0 = load<u32>(pair), p1 = load<u32>(pair + 4);
    store<u32>(pair + 8, p0);
    store<u32>(pair + 12, p1);
    store<u32>(pair, 0);
    store<u32>(pair + 4, 0);
    const u32 job = load<u32>(s);
    const u32 old = load<u32>(job + 24);
    store<u32>(job + 28, old);
    const u32 half = old == job ? job + 12 : job;
    store<u32>(job + 24, half);
    store<u32>(half, 0);
    store<u32>(half + 4, 0);
    store<u32>(half + 8, 0);
    ctx.gpr[3] = half;  // left in r3
}

// void Job_EndFrame(EndFrame* f)   (last render list entry; code of frame::kJobEndFrame)
// Flips the byte at +16 (xor 1) and clears the 8 bytes of the record it
// now selects (+0 or +8).
DECOMP(0078896C, Job_EndFrame) {
    const u32 f = arg<u32>(ctx, 0);
    const u8 b = static_cast<u8>(ref<u8>(f + 16) ^ 1);
    const u32 rec = f + ((static_cast<u32>(b) << 3) & 0x7F8);  // rlwinm 3,21,28
    ref<u8>(f + 16) = b;
    for (u32 i = 0; i < 8; ++i) ref<u8>(rec + i) = 0;
    ctx.gpr[3] = rec;
}

// void World_RunRenderJobs(World* w, u32 mode)   (name?)
// Called by main at the end of a frame. Modes 0 and 2: five end-of-frame
// updates, the frame arenas swap (the other one is reset and its use
// count bumped), then every non-null {OPD, arg} entry of the render list
// is called (arg in r3, entry address in r4, OPD in r5, as the original
// leaves them) and the list emptied. Modes 1 and 2 then clear seven world
// counters.
DECOMP(00458BAC, World_RunRenderJobs) {
    const u32 w = arg<u32>(ctx, 0);
    const s32 mode = arg<s32>(ctx, 1);
    const u32 list = wf::kRenderList;
    if (mode == 0 || mode == 2) {
        call(ctx, jfn::sub_0042AD98);
        call(ctx, jfn::sub_004291C8);
        call(ctx, jfn::sub_005AC60C);
        call(ctx, jfn::sub_000D0298);
        call(ctx, jfn::sub_002F481C);

        const u32 fa = load<u32>(wf::kFrameArenas);
        const u32 cur = load<u32>(fa + 8);
        store<u32>(fa + 12, cur);
        const u32 arena = cur == load<u32>(fa) ? load<u32>(fa + 4) : load<u32>(fa);
        store<u32>(fa + 8, arena);
        const u32 a4 = load<u32>(arena + 4), uses = load<u32>(arena + 28);
        const u8 flags = ref<u8>(arena + 32);
        const u32 a8 = load<u32>(arena + 8);
        store<u32>(arena + 12, a4);
        store<u32>(arena + 16, a8);
        store<u32>(arena + 24, 0);
        store<u32>(arena + 20, 0);
        store<u32>(arena + 28, uses + 1);
        ref<u8>(arena + 32) = static_cast<u8>(flags & ~0xC0);

        for (u32 i = 0, off = 0; i < load<u16>(list + 8); ++i, off += 8) {
            const u32 e = load<u32>(list) + off;
            const u32 opd = load<u32>(e);
            if (opd != 0) call_ptr(ctx, opd, {load<u32>(e + 4), load<u32>(opd), opd});
        }
        store<u16>(list + 8, 0);
        ctx.gpr[3] = 0;
    }
    if (mode == 2 || mode == 1) {
        for (const u32 off : kWorldFrameCounters) store<u16>(w + off, 0);
        ctx.gpr[3] = 0x128514;  // left in r3 by the original
    }
}
