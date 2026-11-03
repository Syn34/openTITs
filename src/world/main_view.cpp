// World_SubmitMain: the main view's render jobs.
#include "world.h"

using namespace sly4;
using namespace sly4::world;

namespace {

namespace mfn {
constexpr u32 FlowState_GetMovieOwner = 0x00458D6C;
constexpr u32 sub_0045A09C = 0x0045A09C;  // (World*, View*) -> bool
constexpr u32 sub_000D016C = 0x000D016C;  // (View*)
constexpr u32 sub_003A13C8 = 0x003A13C8;  // (load(0x00F657FC))
constexpr u32 sub_005AC470 = 0x005AC470;  // (0, 0x00F39F10, 1)
constexpr u32 sub_002EF36C = 0x002EF36C;  // (load(0x00F64C38))
constexpr u32 sub_00365A50 = 0x00365A50;  // ()
constexpr u32 sub_002E11D8 = 0x002E11D8;  // (load(0x00F64C20))
constexpr u32 sub_003DD2A0 = 0x003DD2A0;  // (Obj*, View*)
constexpr u32 sub_002AC808 = 0x002AC808;  // (Camera*)
constexpr u32 sub_00429950 = 0x00429950;  // (View*, 0, 1)
}  // namespace mfn

constexpr u32 kJobSlotMainA = 0x010BBE2C;  // { job, pair, begun }
constexpr u32 kJobSlotMainB = 0x010BBE38;
constexpr u32 kLayerMain = 254;
constexpr u32 kViewFlags256 = 256;  // u32: 0x10 = no movie owner (name?)
constexpr u32 kViewFlags52 = 52;    // u8: 0x10 from sub_0045A09C
constexpr u32 kArgF657FC = 0x00F657FC, kArgF64C38 = 0x00F64C38, kArgF64C20 = 0x00F64C20;
constexpr u32 kArgF39F10 = 0x00F39F10;
constexpr u32 kCamera = 0x00F64A58;
constexpr u32 kWorld128224 = 0x128224;  // u32 obj: vfunc +44 every frame
constexpr u32 kWorld1281F4 = 0x1281F4;  // u32 obj: flags at +52 (0x20000000) and at +28 (0x100)
constexpr u32 kJobObj = 0x00F384F8;     // OPD pushed with the World +0x1281F4 object
constexpr u32 kViewOwners = 0x010BBE0C;  // {u32 owner, ...}[] by the u8 index at +16
constexpr u32 kViewOwnerIndex = 0x010BBE1C;

bool Obj_Flag100(u32 obj) { return obj != 0 && (load<u32>(obj + 28) & 0x100) != 0; }

}  // namespace

// void World_SubmitMain(World* world)  (name?)
//
// Builds the main view's two jobs (layer 254), unless the flow state has
// a movie owner, in which case only last frame's jobs are queued again.
// Records the owner for the current view slot.
DECOMP(0045A208, World_SubmitMain) {
    const u32 world = arg<u32>(ctx, 0);
    const u32 owner = static_cast<u32>(call(ctx, mfn::FlowState_GetMovieOwner, {world}));
    const u32 view = field<u32>(world, kWorldViews);
    const u32 flags = field<u32>(view, kViewFlags256);
    field<u32>(view, kViewFlags256) = owner != 0 ? (flags & ~0x10u) : (flags | 0x10u);

    if (owner == 0) {
        const u32 v = static_cast<u32>(call(ctx, mfn::sub_0045A09C, {world, view}));
        field<u8>(view, kViewFlags52) = static_cast<u8>((field<u8>(view, kViewFlags52) & ~0x10) | ((v << 4) & 0x10));
        call(ctx, wfn::Job_Begin, {kJobSlotMainA, view, kLayerMain});
        call(ctx, mfn::sub_000D016C, {view});
        call(ctx, mfn::sub_003A13C8, {load<u32>(kArgF657FC)});
        Job_End(ctx, kJobSlotMainA);

        call(ctx, wfn::Job_Begin, {kJobSlotMainB, view, kLayerMain});
        call(ctx, mfn::sub_005AC470, {0, kArgF39F10, 1});
        call(ctx, mfn::sub_002EF36C, {load<u32>(kArgF64C38)});
        call(ctx, mfn::sub_00365A50);  // r3 is left from the call above
        call(ctx, mfn::sub_002E11D8, {load<u32>(kArgF64C20)});
        if (const u32 obj = field<u32>(world, kWorld128224)) call_ptr(ctx, vfunc(obj, 44), {obj});

        u32 obj = field<u32>(world, kWorld1281F4);
        if (obj != 0 && (load<u32>(obj + 52) & 0x20000000)) {
            call_ptr(ctx, vfunc(obj, 48), {obj});
            obj = field<u32>(world, kWorld1281F4);
        }
        if (Obj_Flag100(obj)) {
            call(ctx, mfn::sub_003DD2A0, {obj, view});
            RenderList_Push(ctx, kJobObj, obj);
        }
        if (const u32 camera = load<u32>(kCamera)) call(ctx, mfn::sub_002AC808, {camera});
        if (!Obj_Flag100(field<u32>(world, kWorld1281F4)))
            call(ctx, mfn::sub_00429950, {field<u32>(world, kWorldViews), 0, 1});
        Job_End(ctx, kJobSlotMainB);
    }

    Job_Queue(ctx, kJobSlotMainA);
    Job_Queue(ctx, kJobSlotMainB);
    store<u32>(kViewOwners + (static_cast<u32>(ref<u8>(kViewOwnerIndex)) << 3), owner);
}
