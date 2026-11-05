// World_HandleEvent callees, part 23f: the physics world teardown, an
// object's child broadcast and the camera rig's defaults capture.
#include "hevent22.h"

#include <bit>
#include <cmath>

using namespace sly4;
using namespace sly4::hevent;

namespace {
// Physics_Shutdown341D5C: the Havok world (0x00F64C94), freed through the
// thread's memory router (r13 - 28332, +44 allocator, vtable +12 (a, p,
// size)) after 0x007E2B38; the +0x00F64C90 object released (vtable +4).
constexpr u32 kHkWorld = 0x00F64C94, kHkObject = 0x00F64C90, kHkFlag = 0x00F64C8C;
constexpr u32 kHkWorldRemoveAll7E2B38 = 0x007E2B38;  // (hkpWorld*) (name?)
constexpr u32 kTlsMemoryRouter = 28332;               // r13 - 28332: hkMemoryRouter* (as hevent22_d.cpp)
constexpr u32 kRouterHeap = 44, kHeapFree = 12, kHkWorldSize = 1984;
constexpr u32 kSlotRelease = 4;

// Obj_BroadcastChildren385F00: child object slots of the owner.
constexpr u32 kSlotNotify = 40;  // child vtable: (child, u8, 1)

// CamRig_SaveDefaults42A480: constants at 0x0042A464 {75.0, 1e-5, 50.0,
// 0.5, 0.002, 1.0, 0.0}; the rig (0x00F66FD0, vtable +28 () -> f32 field of
// view, +24 (f32) set it) and its defaults block (0x010AA620, as
// hevent3_g.cpp CamRig_Reset); world part object +5548.. overrides.
constexpr u32 kCamRig = 0x00F66FD0, kCamRigDefaults = 0x010AA620;
constexpr u32 kCamK = 0x0042A464;
constexpr u32 kSlotGetFov = 28, kSlotSetFov = 24;
constexpr u32 kWorldPart = 0x128164;
constexpr u32 kZeroVec = 0x01164B40;
constexpr u32 kRigFlags = 96, kRigCaptured = 0x20;

f64 lfs(u32 ea) { return static_cast<f64>(std::bit_cast<f32>(load<u32>(ea))); }
f32 fsubs(f64 a, f64 b) { return static_cast<f32>(a - b); }
bool near(u32 ea, u32 k, u32 tol) { return std::fabs(static_cast<f64>(fsubs(lfs(ea), lfs(k)))) < lfs(tol); }
}  // namespace

// void Physics_Shutdown341D5C() (name?): with a Havok world at
// 0x00F64C94: 0x007E2B38(world), then the TLS memory router's heap frees
// its 1984 bytes and the pointer is cleared; the object at 0x00F64C90 is
// released (vtable +4) and cleared; 0x00F64C8C = 0. Leaves r3 =
// 0x00F60000, r4 = 0.
DECOMP(00341D5C, Physics_Shutdown341D5C) {
    const u32 w = load<u32>(kHkWorld);
    if (w != 0) {
        call(ctx, kHkWorldRemoveAll7E2B38, {w});
        const u32 heap = load<u32>(load<u32>(static_cast<u32>(ctx.gpr[13]) - kTlsMemoryRouter) + kRouterHeap);
        vcall(ctx, heap, kHeapFree, {heap, w, kHkWorldSize});
        store<u32>(kHkWorld, 0);
    }
    const u32 o = load<u32>(kHkObject);
    if (o != 0) {
        vcall(ctx, o, kSlotRelease, {o});
        store<u32>(kHkObject, 0);
    }
    store<u32>(kHkFlag, 0);
    ctx.gpr[3] = 0x00F60000u;
    ctx.gpr[4] = 0;
}

// void Obj_BroadcastChildren385F00(Object* o, u8 arg) (name?): every
// non-null child in +588[9], +624[32], +752[8] / +784[8] (interleaved per
// index), +816[32] and +944 gets vtable +40 (child, arg, 1). r3 as the
// original leaves it (the last call's result or the last slot read).
DECOMP(00385F00, Obj_BroadcastChildren385F00) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 a = arg<u8>(ctx, 1);
    u64 r3 = 0;
    auto notify = [&](u32 slot) {
        const u32 c = load<u32>(slot);
        r3 = c;
        if (c != 0) r3 = vcall(ctx, c, kSlotNotify, {c, a, 1});
    };
    for (u32 i = 0; i < 9; ++i) notify(o + 588 + 4 * i);
    for (u32 i = 0; i < 32; ++i) notify(o + 624 + 4 * i);
    for (u32 i = 0; i < 8; ++i) {
        notify(o + 752 + 4 * i);
        notify(o + 784 + 4 * i);
    }
    for (u32 i = 0; i < 32; ++i) notify(o + 816 + 4 * i);
    const u32 last = load<u32>(o + 944);
    if (last != 0) r3 = vcall(ctx, last, kSlotNotify, {last, a, 1});
    ctx.gpr[3] = r3;
}

// void CamRig_SaveDefaults42A480() (name?): with a camera rig at
// 0x00F66FD0: a field of view within 1e-5 of 75 is set to 50 (vtable
// +24), a +48 vector within 0.002 of {0.5, 0.5, 0.5, 1.0} becomes {0, 0,
// 0.05, 0.5}; then the defaults block 0x010AA620 is filled: +0..+8 = rig
// +32..+40, +16 / +20 = +64 / +68, +24 = the fov, +28 = +88, +32 / +36 /
// +40 (u8) / +44 from the world part object's +5548 / +5552 / +5556 /
// +5560 (0 without one), +48 and +64 = the zero vector, +80 / +84 = 0,
// +88 / +92 = +20, +96 = 255; the rig's +96 gets flag 0x20.
DECOMP(0042A480, CamRig_SaveDefaults42A480) {
    const u32 c = load<u32>(kCamRig);
    if (c == 0) return;
    vcall(ctx, c, kSlotGetFov, {c});
    // f1 is left as the original's register sequence leaves it at each call.
    ctx.fpr[1] = std::fabs(static_cast<f64>(fsubs(ctx.fpr[1], lfs(kCamK))));
    if (ctx.fpr[1] < lfs(kCamK + 4)) {
        ctx.fpr[1] = lfs(kCamK + 8);
        vcall(ctx, c, kSlotSetFov, {c});
    }
    const u32 v = c + 48;
    ctx.fpr[1] = lfs(kCamK + 16);
    if (near(v + 0, kCamK + 12, kCamK + 16) && near(v + 4, kCamK + 12, kCamK + 16) && near(v + 8, kCamK + 12, kCamK + 16) &&
        near(v + 12, kCamK + 20, kCamK + 16)) {
        store<u32>(c + 48, 0);
        store<u32>(c + 52, 0);
        store<u32>(c + 60, 0x3F000000u);  // 0.5
        store<u32>(c + 56, 0x3D4CCCCDu);  // 0.05
    }
    const u32 part = load<u32>(load<u32>(kCurrentWorld) + kWorldPart);
    const u32 g = kCamRigDefaults;
    store<u32>(g + 0, load<u32>(c + 32));
    store<u32>(g + 4, load<u32>(c + 36));
    store<u32>(g + 8, load<u32>(c + 40));
    store<u32>(g + 16, load<u32>(c + 64));
    store<u32>(g + 20, load<u32>(c + 68));
    ctx.fpr[1] = lfs(c + 68);
    vcall(ctx, c, kSlotGetFov, {c});
    store<f32>(g + 24, static_cast<f32>(ctx.fpr[1]));
    store<u32>(g + 28, load<u32>(c + 88));
    const u32 f2 = load<u32>(g + 20);
    const u32 zero = load<u32>(kCamK + 24);
    const u32 rig = load<u32>(kCamRig);
    store<u32>(g + 32, part != 0 ? load<u32>(part + 5548) : zero);
    store<u32>(g + 36, part != 0 ? load<u32>(part + 5552) : zero);
    store<u8>(g + 40, part != 0 ? load<u8>(part + 5556) : 0);
    const u32 f44 = part != 0 ? load<u32>(part + 5560) : zero;
    const u32 z0 = load<u32>(kZeroVec + 0), z1 = load<u32>(kZeroVec + 4), z2 = load<u32>(kZeroVec + 8);
    store<u32>(g + 44, f44);
    store<u32>(g + 48, z0);
    store<u32>(g + 52, z1);
    store<u32>(g + 56, z2);
    store<u32>(g + 64, z0);
    store<u32>(g + 68, z1);
    store<u32>(g + 72, z2);
    store<u32>(g + 80, zero);
    store<u32>(g + 84, zero);
    store<u32>(g + 88, f2);
    store<u32>(g + 92, f2);
    store<u8>(g + 96, 255);
    ctx.fpr[1] = static_cast<f64>(std::bit_cast<f32>(zero));
    ctx.gpr[3] = rig;
    if (rig != 0) store<u8>(rig + kRigFlags, static_cast<u8>(load<u8>(rig + kRigFlags) | kRigCaptured));
}
