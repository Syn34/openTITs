// Names for the World_SubmitFrame steps (0x00459xxx, 0x00457654, 0x0045A208).
// Addresses are guest addresses in the BCES01284 v01.02 EBOOT; "?" marks a guess.
#pragma once

#include <bit>

#include "../sly4.h"

namespace sly4 {
namespace world {

inline u32 vfunc(u32 obj, u32 offset) { return load<u32>(load<u32>(obj) + offset); }
inline f32 fmuls(f64 a, f64 c) { return static_cast<f32>(a * c); }
inline void copy_word(u32 dst, u32 src) { store<u32>(dst, load<u32>(src)); }  // lfs/stfs pairs

namespace wfn {
constexpr u32 Job_Begin = 0x00459198;          // (Job** slot, View* view, u8 layer); world/job.cpp
constexpr u32 PtrVector_Reserve = 0x00551718;  // (vector, new capacity)
constexpr u32 FrameBuffer_PushMarker = 0x005220E8;  // (fb, -, value): game/frame_misc.cpp
constexpr u32 Arena_Snapshot = 0x00565178;     // (Arena*, out): game/frame_misc.cpp
constexpr u32 Arena_Restore = 0x00565154;      // (Arena*, u32) (name?)
constexpr u32 FrameBuffer_BeginLayer = 0x00521A5C;  // (fb, View*, u8 layer) -> u32 (name?)
constexpr u32 View_SetMatrix = 0x005201B0;     // (View*, const f32[16]) VMX
constexpr u32 View_SetFov = 0x00520368;        // (View*, f32) (name?)
constexpr u32 View_SetF2C8 = 0x005202C8;       // (View*, f32) (name?)
constexpr u32 View_SetF338 = 0x00520338;       // (View*, f32) (name?)
constexpr u32 View_SetF094 = 0x00520094;       // (View*, f32) (name?)
constexpr u32 View_Set5115F0 = 0x005115F0;     // (View*, 0) (name?)
constexpr u32 Matrix_LookAt = 0x0056A0D0;       // (f32 out[16], const ?, const ?, const ?) (name?)
constexpr u32 Ui_Render = 0x004D4E28;          // (u8 flag, View*) (name?)
}  // namespace wfn

// Render job list (see game/frame_world.cpp).
constexpr u32 kRenderList = 0x010BB854;
constexpr u32 kJobRun = 0x00F2E248;  // OPD pushed with a job slot

// Globals shared by the steps.
constexpr u32 kCurrentWorld = 0x00F65CA0;  // u32 World*
constexpr u32 kFrameArenas = 0x00F65C90;   // u32 { a, b, current Arena*, 0 }
constexpr u32 kJobFrameBuffer = 0x00F66890;  // u32: frame buffer of the job being built
constexpr u32 kJobView = 0x00F6688C;         // u32: view of the job being built
constexpr u32 kMainView = 0x00F6684C;      // u32 View*
constexpr u32 kWorldFrameRing = 0x1285E4;  // World +: FrameRing*, current FrameBuffer at +12
constexpr u32 kWorldArenas = 0x1285E8;     // World +: u32 frame arenas while a job is built, else 0

// Job slots: { Job* job, Pair* pair, u8 begun }.
constexpr u32 kJobSlotOverlay = 0x010BBE20;  // 004596E8
constexpr u32 kJobSlotE10 = 0x010BBFA8;      // 00459E10
constexpr u32 kJobSlotB60 = 0x010BBFB4;      // 00459B60
constexpr u32 kJobSlot308 = 0x010BBFC0;      // 00459308
constexpr u32 kJobSlot8D0 = 0x010BBFCC;      // 004598D0
constexpr u32 kJobSlot500 = 0x010BBFD8;      // 00459500
constexpr u32 kLayerAll = 255;

// World fields.
constexpr u32 kWorldViews = 0x128168;      // View*[4]: main, +4, +8, +12
constexpr u32 kWorld1281B0 = 0x1281B0;     // given to Matrix_LookAt
constexpr u32 kWorld1281C4 = 0x1281C4;     // u32 obj*: 00459E10 runs when *obj != 0
constexpr u32 kViewCamera = 656;           // View +: u32 obj, its +8 holds camera settings (name?)
constexpr u32 kViewFlags = 1776;           // u8: 0x20 set by the 3D steps (name?)
constexpr u32 kViewNear = 1824, kViewFar = 1828;  // f32 (name?)
constexpr u32 kViewMatrix = 752;           // f32[16]

// Callback lists: World + offset, 12-byte entries {opd, arg, -}, u16 count at +8.
constexpr u32 kListE10 = 0x1284F8, kListB60 = 0x12850C, kList8D0 = 0x128520, kList308 = 0x128534;
constexpr u32 kListOverlay = 0x128548, kList500 = 0x12855C;

// Tuning objects (value at +72).
constexpr u32 kTuningValue = 72;
constexpr u32 kTuneNear = 0x010B97D0, kTuneFar = 0x010B981C, kTuneFov = 0x010B96C4;
constexpr u32 kMatrixArgA = 0x010B97C0, kMatrixArgB = 0x01164B60;
constexpr f32 kRadToDeg = std::bit_cast<f32>(0x42652EE0u);  // at 0x00459B54

// ---------------------------------------------------------------- shared inlined code
// The render-list push the original inlines everywhere.
inline void RenderList_Push(PPUContext& ctx, u32 fn, u32 arg) {
    const u32 list = kRenderList;
    u32 count = load<u16>(list + 8);
    const u32 capacity = load<u16>(list + 4);
    if (count == capacity) {
        call(ctx, wfn::PtrVector_Reserve, {list, (capacity << 1) & 0x1FFFE});
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

// Runs a callback list with f1 = 0.0; r3 gets the code address read from
// each descriptor (as in the original), r4 the entry's argument.
inline void World_RunCallbacks(PPUContext& ctx, u32 list) {
    for (u32 i = 0, offset = 0; i < load<u16>(list + 8); ++i, offset += 12) {
        const u32 entry = load<u32>(list) + offset;
        const u32 opd = load<u32>(entry);
        const u32 arg = load<u32>(entry + 4);
        ctx.fpr[1] = 0.0;
        call_ptr(ctx, opd, {load<u32>(opd), arg});
    }
}

// Closes the job being built in `slot`.
inline void Job_End(PPUContext& ctx, u32 slot) {
    const u32 current = load<u32>(kCurrentWorld);
    store<u32>(kJobView, 0);
    store<u32>(kJobFrameBuffer, 0);
    // FrameBuffer_PushMarker reads r5: the original has the ring offset there.
    call(ctx, wfn::FrameBuffer_PushMarker,
         {field<u32>(field<u32>(current, kWorldFrameRing), 12), 0x00F60000, kWorldFrameRing});
    call(ctx, wfn::Arena_Snapshot, {field<u32>(load<u32>(kFrameArenas), 8), field<u32>(load<u32>(slot), 24)});
    field<u32>(load<u32>(kCurrentWorld), kWorldArenas) = 0;
}

// Queues the slot's job if it has one.
inline void Job_Queue(PPUContext& ctx, u32 slot) {
    if (load<u32>(slot) != 0) RenderList_Push(ctx, kJobRun, slot);
}

}  // namespace world
}  // namespace sly4
