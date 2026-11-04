// World_HandleEvent callees, part 15b: the player's spline release
// (Obj_Release12BF70) and the scenes-finish pass of World_ScenesEnd48B674.
#include "hevent15.h"

using namespace sly4;
using namespace sly4::hevent;
using namespace sly4::hevent15;

namespace {
namespace bfn {
constexpr u32 Body_SetMode = 0x00261328;        // (Body*, u32 mode) (hevent15_c.cpp)
constexpr u32 Obj_SetPhysicsActive = 0x00260C14;    // (Body*, u8 on) (symbols.txt)
constexpr u32 Spline_FindNear5A8CF8 = 0x005A8CF8;   // (vec3*, f32* t, f32* dist, filter OPD, Query*, vec3*, 0) -> SplineInstance* (name?)
constexpr u32 SplineInstance_GetMode = 0x005A8BA8;  // (inst) -> u8 (symbols.txt)
constexpr u32 Obj_Notify24 = 0x000D949C;            // (Object*) -> u32 (symbols.txt)
constexpr u32 Obj_Notify26 = 0x000D94CC;            // (Object*) -> u32 (symbols.txt)
constexpr u32 sub_002A8AE0 = 0x002A8AE0;            // (x, 0, 1, 0) (hevent15_c.cpp)
constexpr u32 Obj_UpdateFlags2336 = 0x0029D838;     // (x, 0) (symbols.txt)
constexpr u32 sub_0029F26C = 0x0029F26C;            // (x)
constexpr u32 sub_002549F8 = 0x002549F8;            // (component 36, 0)
constexpr u32 sub_00276558 = 0x00276558;            // (component 23, 0)
constexpr u32 Scene_Keep1AC68 = 0x0001AC68;         // (*0x00F557F0) (name?)
constexpr u32 Scene_Drop1AC74 = 0x0001AC74;         // (*0x00F557F0) (name?)
constexpr u32 Obj_SceneEnd0DD02C = 0x000DD02C;      // (Object*, u8 drop, 0) (name?)
constexpr u32 sub_004A9084 = 0x004A9084;            // ()
}  // namespace bfn

// Obj_Release12BF70. Object: vt +64 body(Object*) -> Body*, vt +28
// set(Object*, u32 slot, u32 value); +144 vec3 position; +20 motion (+48
// flags), +6 u16 offset of the spline record from the motion; +698 u8 flags
// (bit 7: on a spline, bit 6: keep it).
constexpr u32 kObjVtBody = 64, kObjVtSet = 28;
constexpr u32 kObjPos = 144, kObjMotion = 20, kObjRecOffset = 6, kObjSplineFlags = 656 + 42;
constexpr u8 kOnSpline = 0x80, kKeepSpline = 0x40;
constexpr u32 kMotionFlags = 48, kMotionFree1 = 0x00200000, kMotionFree2 = 0x00400000;
// Spline record: +24 SplineInstance*, +28 f32 t, +36 u8 mode (the word's bit
// 23 cleared).
constexpr u32 kRecInst = 24, kRecT = 28, kRecMode = 36, kRecModeBit23 = 0x00800000;
constexpr u32 kBodyModeFree = 2, kBodyModeSpline = 4;
constexpr u32 kSplineFilter = 0x00F32FC8;  // OPD of 0x0019FB9C
constexpr u32 kSplineMaxDist = 0x0103CC18; // f32 (0x0103CBD0 +72)

// World +0x128588 .. +0x12859C: two {f32 1, f32 -1, u8 0} blend records.
constexpr u32 kWBlendA = 0x128588, kWBlendB = 0x128594;
constexpr u32 kOne = 0x3F800000, kMinusOne = 0xBF800000;
// Player +184 (u64) bit 8; +192 bit 3.
constexpr u32 kObjFlags184 = 184;
constexpr u64 kObjScene = 0x100;
constexpr u32 kSceneHandle = 0x00F557F0;
constexpr u32 kCompType36 = 36, kCompType23 = 23, kComp23Kind = 24;
constexpr u32 kWCamera = 0x1281E0, kCamFlags468 = 468, kCamSceneDrop = 0x20000000;
}  // namespace

// u32 Obj_Release12BF70(Object* o) (name?): lets go of the spline o rides.
// Without a body, nothing. Unless o is on a spline (+698 bit 7) without the
// keep flag (bit 6), or when no spline lies within 0x0103CC18 of o's
// position: the body goes free (mode 2), set(0, 1), set(1, 0) and the motion
// gets +48 bits 21 and 22. Otherwise o is put on the spline found: set(0, 0),
// set(1, 10), set(2, 0), the body (again) gets physics and mode 4, and the
// record at the motion +o[6] takes the instance, t and the instance's mode.
DECOMP(0012BF70, Obj_Release12BF70) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 body = static_cast<u32>(vcall(ctx, o, kObjVtBody, {o}));
    if (body == 0) return ret(ctx, 0u);
    const u8 f = load<u8>(o + kObjSplineFlags);
    if ((f & kOnSpline) != 0 && (f & kKeepSpline) == 0) {
        // r1+112 pos, +128 query {pos, +16 zero vec, +32 f32 max dist, +36 u8
        // 0}, +176 f32 t, +180 f32 dist.
        Stack frame(ctx, 80);
        const u32 s = frame.addr();
        const u32 pos = s, query = s + 16, t = s + 64, dist = s + 68;
        for (u32 k = 0; k < 12; k += 4) store<u32>(pos + k, load<u32>(o + kObjPos + k));
        for (u32 k = 0; k < 12; k += 4) store<u32>(query + k, load<u32>(o + kObjPos + k));
        for (u32 k = 0; k < 12; k += 4) store<u32>(query + 16 + k, load<u32>(hevent7::kZeroVec + k));
        store<u32>(query + 32, load<u32>(kSplineMaxDist));
        store<u8>(query + 36, 0);
        // f1..f4 as the original's copies leave them.
        ctx.fpr[1] = hevent7::lfs_f64(hevent7::kZeroVec + 8);
        ctx.fpr[2] = hevent7::lfs_f64(hevent7::kZeroVec + 4);
        ctx.fpr[3] = hevent7::lfs_f64(o + kObjPos + 8);
        ctx.fpr[4] = hevent7::lfs_f64(kSplineMaxDist);
        const u32 inst = static_cast<u32>(call(ctx, bfn::Spline_FindNear5A8CF8,
                                               {pos, t, dist, kSplineFilter, query, hevent7::kZeroVec, 0}));
        bool near = false;
        if (inst != 0) {
            ctx.fpr[1] = hevent7::lfs_f64(dist);  // f1 / f2 as the compare leaves them
            ctx.fpr[2] = hevent7::lfs_f64(kSplineMaxDist);
            near = ctx.fpr[1] <= ctx.fpr[2];
        }
        if (near) {
            vcall(ctx, o, kObjVtSet, {o, 0, 0});
            vcall(ctx, o, kObjVtSet, {o, 1, 10});
            vcall(ctx, o, kObjVtSet, {o, 2, 0});
            const u32 b2 = static_cast<u32>(vcall(ctx, o, kObjVtBody, {o}));
            call(ctx, bfn::Obj_SetPhysicsActive, {b2, 1});
            call(ctx, bfn::Body_SetMode, {b2, kBodyModeSpline});
            const u32 rec = load<u32>(o + kObjMotion) + load<u16>(o + kObjRecOffset);
            store<u32>(rec + kRecInst, inst);
            store<u32>(rec + kRecT, load<u32>(t));
            ctx.fpr[1] = hevent7::lfs_f64(t);
            const u32 mode = static_cast<u32>(call(ctx, bfn::SplineInstance_GetMode, {inst}));
            const u32 old = load<u32>(rec + kRecMode);
            store<u8>(rec + kRecMode, static_cast<u8>(mode));
            store<u32>(rec + kRecMode, (old & 0x00FFFFFFu & ~kRecModeBit23) | (mode << 24));
            return;  // r3 = the mode
        }
    }
    call(ctx, bfn::Body_SetMode, {body, kBodyModeFree});
    vcall(ctx, o, kObjVtSet, {o, 0, 1});
    vcall(ctx, o, kObjVtSet, {o, 1, 0});
    const u32 m = load<u32>(o + kObjMotion) + kMotionFlags;
    store<u32>(m, load<u32>(m) | kMotionFree1);
    store<u32>(m, load<u32>(m) | kMotionFree2);
    ret(ctx, m);
}

// void World_ScenesFinish48B330(World* w, u8 keep) (name?): with keep, resets
// the two blend records (+0x128588 / +0x128594) and passes a +192 bit 3
// player's Obj_Notify26 result to 0x002A8AE0. Then for the player (user 0):
// +184 bit 8 = keep, its Obj_Notify24 object gets vt +44 (!keep) and
// 0x0029F26C (keep) or Obj_UpdateFlags2336(0); component 36 gets
// 0x002549F8(0); with keep, every component 23 of kind 7 gets 0x00276558(0).
// Then the scene handle (0x00F557F0) is kept or dropped, every +192 bit 3
// object gets 0x000DD02C(!keep, 0), 0x004A9084 runs and the camera's +468
// bit 29 becomes !keep.
DECOMP(0048B330, World_ScenesFinish48B330) {
    const u32 w = arg<u32>(ctx, 0);
    const u8 keep = arg<u8>(ctx, 1);
    const u32 ub = w + kUserBlocks;
    if (keep != 0) {
        for (u32 b : {kWBlendA, kWBlendB}) {
            store<u32>(w + b, kOne);
            store<u32>(w + b + 4, kMinusOne);
            store<u8>(w + b + 8, 0);
        }
        const u32 p = load<u32>(ub + 8);
        if (p != 0 && (load<u32>(p + hevent14::kObjFlags192) & hevent14::kObjFlag192Bit3) != 0) {
            if (static_cast<u32>(call(ctx, bfn::Obj_Notify26, {p})) != 0) {
                const u32 x = static_cast<u32>(call(ctx, bfn::Obj_Notify26, {p}));
                call(ctx, bfn::sub_002A8AE0, {x, 0, 1, 0});
            }
        }
    }
    const u32 drop = keep == 0 ? 1u : 0u;
    const u32 p = load<u32>(ub + 8);
    if (p != 0) {
        const u64 fl = load<u64>(p + kObjFlags184);
        store<u64>(p + kObjFlags184, keep != 0 ? (fl | kObjScene) : (fl & ~kObjScene));
        const u32 n = static_cast<u32>(call(ctx, bfn::Obj_Notify24, {load<u32>(ub + 8)}));
        if (n != 0) {
            vcall(ctx, n, 44, {n, drop});
            if (keep != 0) call(ctx, bfn::sub_0029F26C, {n});
            else call(ctx, bfn::Obj_UpdateFlags2336, {n, 0});
        }
        const u32 c36 = static_cast<u32>(call(ctx, hevent12::h12fn::Obj_FindComponent, {load<u32>(ub + 8), kCompType36, 0}));
        if (c36 != 0) call(ctx, bfn::sub_002549F8, {c36, 0});
        if (keep != 0) {
            u32 c = static_cast<u32>(call(ctx, hevent12::h12fn::Obj_FindComponent, {load<u32>(ub + 8), kCompType23, 0}));
            while (c != 0) {
                if (((load<u32>(c + kComp23Kind) >> 25) & 0x1F) == 7) call(ctx, bfn::sub_00276558, {c, 0});
                c = static_cast<u32>(call(ctx, hevent12::h12fn::Obj_FindComponent, {load<u32>(ub + 8), kCompType23, c}));
            }
        }
    }
    call(ctx, keep != 0 ? bfn::Scene_Keep1AC68 : bfn::Scene_Drop1AC74, {load<u32>(kSceneHandle)});
    for (u32 i = 0; i < load<u16>(kAllObjects + kAllCount); ++i) {
        const u32 o = load<u32>(load<u32>(kAllObjects + kAllData) + 4 * i);
        if (o != 0 && (load<u32>(o + hevent14::kObjFlags192) & hevent14::kObjFlag192Bit3) != 0)
            call(ctx, bfn::Obj_SceneEnd0DD02C, {o, drop, 0});
    }
    call(ctx, bfn::sub_004A9084, {load<u16>(kAllObjects + kAllCount)});  // r3 = the count (unread)
    const u32 cam = load<u32>(w + kWCamera);
    if (cam == 0) return ret(ctx, kWCamera);
    const u32 v = (load<u32>(cam + kCamFlags468) & ~kCamSceneDrop) | (drop << 29);
    store<u32>(cam + kCamFlags468, v);
    ret(ctx, v);
}
