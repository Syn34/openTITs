// The render-job execution side: main's end-of-frame call runs the
// {OPD, arg} entries World_SubmitFrame pushed onto the render list
// (frame::kRenderList); the entries are Job_Swap (per view / overlay job)
// and Job_EndFrame (last).
#include "rsx.h"

namespace sly4::gfx {

namespace {

constexpr u32 kRenderList = 0x010BB854;   // RenderList (frame::kRenderList)
constexpr u32 kFrameArenas = 0x00F65C90;  // FrameArenas* (frame::kFrameArenas)
constexpr u32 sub_0042AD98 = 0x0042AD98, sub_004291C8 = 0x004291C8, sub_005AC60C = 0x005AC60C;  // ()
constexpr u32 sub_000D0298 = 0x000D0298, sub_002F481C = 0x002F481C;                              // ()

// World halfwords cleared at the end of a frame in modes 1 and 2.
constexpr u32 kWorldFrameCounters[] = {0x1284EC, 0x128528, 0x128500, 0x12853C, 0x128514, 0x128550, 0x128564};

// A job: two 12-byte halves, the one being written and the previous one.
struct Job {
    be_u32 half_a[3];    // +0
    be_u32 half_b[3];    // +12
    gp<be_u32> writing;  // +24
    gp<be_u32> previous; // +28
};
struct JobPair {
    be_u32 current[2];   // +0
    be_u32 previous[2];  // +8
};
struct JobSlot {
    gp<Job> job;       // +0
    gp<JobPair> pair;  // +4
    u8 building;       // +8
    u8 built;          // +9
};
struct EndFrame {
    u8 records[16];  // +0: two 8-byte records
    u8 which;        // +16
};
struct RenderEntry {
    be_u32 opd;
    be_u32 arg;
};
struct RenderList {
    gp<RenderEntry> entries;  // +0
    be_u16 capacity;          // +4
    pad<2> _6;
    be_u16 count;             // +8
    be_u16 high;              // +10
};
struct Arena {
    be_u32 begin;    // +0
    be_u32 a4;       // +4
    be_u32 a8;       // +8
    be_u32 top;      // +12
    be_u32 limit;    // +16
    be_u32 w20;      // +20
    be_u32 w24;      // +24
    be_u32 uses;     // +28
    u8 flags;        // +32
};
struct FrameArenas {
    gp<Arena> a;        // +0
    gp<Arena> b;        // +4
    gp<Arena> current;  // +8
    gp<Arena> previous; // +12
};

}  // namespace

// void Job_Swap(JobSlot* s)   (render list entry; code of frame::kJobRun)
// Moves "building" to "built", the pair's current values to previous,
// and points the job at its other half, cleared.
void Job_Swap(JobSlot* s) {
    const u8 building = s->building;
    JobPair* pair = s->pair;
    s->built = building;
    s->building = 0;
    const u32 p0 = pair->current[0], p1 = pair->current[1];
    pair->previous[0] = p0;
    pair->previous[1] = p1;
    pair->current[0] = 0u;
    pair->current[1] = 0u;
    Job* job = s->job;
    const u32 old = job->writing.ea;
    job->previous.ea = old;
    be_u32* half = old == guest(job) ? job->half_b : job->half_a;
    job->writing = half;
    half[0] = 0u;
    half[1] = 0u;
    half[2] = 0u;
}

// void Job_EndFrame(EndFrame* f)   (last render list entry; code of frame::kJobEndFrame)
// Flips the byte at +16 (xor 1) and clears the 8 bytes of the record it
// now selects (+0 or +8).
void Job_EndFrame(EndFrame* f) {
    const u8 b = static_cast<u8>(f->which ^ 1);
    u8* rec = at<u8>(f, (static_cast<u32>(b) << 3) & 0x7F8);  // rlwinm 3,21,28
    f->which = b;
    for (u32 i = 0; i < 8; ++i) rec[i] = 0;
}

// void World_RunRenderJobs(World* w, u32 mode)   (name?)
// Called by main at the end of a frame. Modes 0 and 2: five end-of-frame
// updates, the frame arenas swap (the other one is reset and its use
// count bumped), then every non-null {OPD, arg} entry of the render list
// is called with its arg and the list emptied. Modes 1 and 2 then clear
// seven world counters.
void World_RunRenderJobs(u8* w, s32 mode) {
    RenderList& list = global<RenderList>(kRenderList);
    if (mode == 0 || mode == 2) {
        call(sub_0042AD98);
        call(sub_004291C8);
        call(sub_005AC60C);
        call(sub_000D0298);
        call(sub_002F481C);

        FrameArenas* fa = global<gp<FrameArenas>>(kFrameArenas);
        const u32 cur = fa->current.ea;
        fa->previous.ea = cur;
        Arena* arena = cur == fa->a.ea ? fa->b.get() : fa->a.get();
        fa->current = arena;
        const u32 a4 = arena->a4, uses = arena->uses;
        const u8 flags = arena->flags;
        const u32 a8 = arena->a8;
        arena->top = a4;
        arena->limit = a8;
        arena->w24 = 0u;
        arena->w20 = 0u;
        arena->uses = uses + 1;
        arena->flags = static_cast<u8>(flags & ~0xC0);

        for (u32 i = 0, off = 0; i < list.count; ++i, off += 8) {
            const RenderEntry* e = at<RenderEntry>(list.entries.get(), off);
            const u32 opd = e->opd;
            if (opd != 0) call_ptr(opd, e->arg.get());
        }
        list.count = 0;
    }
    if (mode == 2 || mode == 1) {
        for (const u32 off : kWorldFrameCounters) *at<be_u16>(w, off) = 0;
    }
}

}  // namespace sly4::gfx

NATIVE(00737278, sly4::gfx::Job_Swap)
NATIVE(0078896C, sly4::gfx::Job_EndFrame)
NATIVE(00458BAC, sly4::gfx::World_RunRenderJobs)
