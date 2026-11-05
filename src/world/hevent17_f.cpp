// World_HandleEvent callees, part 17f: the camera target stack push.
#include "hevent17.h"

using namespace sly4;
using namespace sly4::hevent;
using namespace sly4::hevent17;

namespace {
// Camera: +96 f32[4][4] matrix, +356 / +360 f32 fov / near, +345 u8 target
// count, +368 Target*[] stack; blend state: +704.. / +736.. positions (+12
// zero vec), +800.. / +832.. quaternions (+16 zero vec), +884 / +892 f32
// fov (+4 u32 0), +768 / +864 / +900 f32 blend.
constexpr u32 kCamMatrix = 96, kCamFov = 356, kCamNear = 360, kCamCount = 345, kCamStack = 368;
constexpr u32 kCamPosFrom = 704, kCamPosTo = 736, kCamPosBlend = 768;
constexpr u32 kCamRotFrom = 800, kCamRotTo = 832, kCamRotBlend = 864;
constexpr u32 kCamFovFrom = 884, kCamFovTo = 892, kCamFovBlend = 900;
// Target: +8 u8 priority; vt +36 deactivate(t), +28 snap(t), +32
// blend_from(t, const View*), +48 view(t, -, View*; f32).
constexpr u32 kTgtPriority = 8, kTgtVtDeactivate = 36, kTgtVtSnap = 28, kTgtVtBlendFrom = 32, kTgtVtView = 48;
// View (112 bytes): +0 f32[4][3] (rows of 16), +64 f32 fov, +68 f32 near,
// +72 u8, +76 / +80 / +84 u32, +88 u8, +92 u32, +96 u8[4] colour.
constexpr u32 kViewSize = 112, kViewPos = 48, kViewFov = 64, kViewNear = 68, kViewColour = 96;
constexpr u32 kDefaultFov = 0x425C0000, kDefaultNear = 0x40F00000;  // 55.0, 7.5
constexpr u32 kIdentity = 0x01164BD0, kZeroVec = 0x01164B40, kViewColourSrc = 0x01164CA8;
constexpr u32 kZeroF = 0x000FC030;  // f32 0.0 (f64 0.0 at +8)
constexpr u32 kQuatFromMatrix = 0x00555478;  // Quat_FromMatrix (f32 q[4] out, const Mat*) (symbols.txt)

using hevent7::lfs_f64;

void copy3x4(u32 dst, u32 src) {
    for (u32 r = 0; r < 64; r += 16)
        for (u32 c = 0; c < 12; c += 4) store<u32>(dst + r + c, load<u32>(src + r + c));
}

// Builds the camera's current view in v (the identity copy is overwritten).
void make_view(u32 v, u32 cam) {
    copy3x4(v, kIdentity);
    store<u32>(v + kViewFov, kDefaultFov);
    store<u32>(v + kViewNear, kDefaultNear);
    store<u8>(v + 72, 0);
    store<u32>(v + 76, 0);
    store<u32>(v + 80, 0);
    store<u32>(v + 84, 0);
    store<u8>(v + 88, 0);
    store<u32>(v + 92, 0);
    for (u32 j = 0; j < 4; ++j) store<u8>(v + kViewColour + j, load<u8>(kViewColourSrc + j));
    copy3x4(v, cam + kCamMatrix);
    store<u32>(v + kViewFov, load<u32>(cam + kCamFov));
    store<u32>(v + kViewNear, load<u32>(cam + kCamNear));
}

void copy_zero3(u32 dst) {
    for (u32 k = 0; k < 12; k += 4) store<u32>(dst + k, load<u32>(kZeroVec + k));
}
}  // namespace

// u32 Camera_PushTarget0FC040(Camera* cam, Target* t, -, u8 priority; f32
// blend) (name?): pushes t on the camera's target stack (a non-priority
// target goes below the priority ones on top; when every entry has
// priority the entries are shifted up and t is dropped, quirk), else
// deactivates the old top, snaps t or blends it from the previous view,
// then records the start / end of the position, rotation and fov blends.
DECOMP(000FC040, Camera_PushTarget0FC040) {
    const u32 cam = arg<u32>(ctx, 0), t = arg<u32>(ctx, 1);
    const u8 prio = arg<u8>(ctx, 3);
    const f64 blend = farg(ctx, 0);
    store<u8>(t + kTgtPriority, prio);
    u32 n = load<u8>(cam + kCamCount);
    if (n != 0) {
        if (prio == 0 && load<u8>(load<u32>(cam + kCamStack + 4 * (n - 1)) + kTgtPriority) != 0) {
            for (u32 i = n; static_cast<s32>(i) > 0; --i) {
                const u32 e = load<u32>(cam + kCamStack + 4 * (i - 1));
                if (load<u8>(e + kTgtPriority) == 0) {
                    store<u32>(cam + kCamStack + 4 * i, t);
                    store<u8>(cam + kCamCount, static_cast<u8>(load<u8>(cam + kCamCount) + 1));
                    return ret(ctx, cam + kCamStack + 4 * i);
                }
                store<u32>(cam + kCamStack + 4 * i, e);
            }
            return ret(ctx, 0u);
        }
        const u32 old = load<u32>(cam + kCamStack - 4 + ((4 * n) & 0x3FC));
        vcall(ctx, old, kTgtVtDeactivate, {old});
    }
    n = load<u8>(cam + kCamCount);
    store<u8>(cam + kCamCount, static_cast<u8>(n + 1));
    store<u32>(cam + kCamStack + 4 * n, t);
    const f64 zero = lfs_f64(kZeroF);
    Stack frame(ctx, 240);  // quat (r1 +112), view (r1 +128), previous view (r1 +240)
    const u32 quat = frame.addr(), view = frame.addr() + 16, prev = frame.addr() + 128;
    if (blend > zero && load<u8>(cam + kCamCount) > 1) {
        make_view(prev, cam);
        vcall(ctx, t, kTgtVtBlendFrom, {t, prev});
    } else {
        vcall(ctx, t, kTgtVtSnap, {t});
    }
    make_view(view, cam);
    {
        // r4 holds the OPD (the original loads it over r4), the entry in r6.
        const u32 opd = load<u32>(load<u32>(t) + kTgtVtView);
        ctx.gpr[6] = load<u32>(opd);
        ctx.fpr[1] = zero;
        call_ptr(ctx, opd, {t, opd, view});
    }
    // fsel(-blend, 0.0, blend), rounded to single.
    const f64 b = static_cast<f32>(-blend >= 0.0 ? std::bit_cast<f64>(load<u64>(kZeroF + 8)) : blend);
    const bool snap = b <= zero;  // cror lt | eq (false for NaN)
    store<f32>(cam + kCamPosBlend, static_cast<f32>(b));
    for (u32 k = 0; k < 12; k += 4) store<u32>(cam + kCamPosTo + k, load<u32>(view + kViewPos + k));
    copy_zero3(cam + kCamPosTo + 16);
    if (snap) {
        for (u32 k = 0; k < 12; k += 4) store<u32>(cam + kCamPosFrom + k, load<u32>(view + kViewPos + k));
        copy_zero3(cam + kCamPosFrom + 16);
    }
    call(ctx, kQuatFromMatrix, {quat, view});
    store<f32>(cam + kCamRotBlend, static_cast<f32>(b));
    for (u32 k = 0; k < 16; k += 4) store<u32>(cam + kCamRotTo + k, load<u32>(quat + k));
    copy_zero3(cam + kCamRotTo + 16);
    const u32 fov = load<u32>(view + kViewFov);
    if (snap) {
        for (u32 k = 0; k < 16; k += 4) store<u32>(cam + kCamRotFrom + k, load<u32>(quat + k));
        copy_zero3(cam + kCamRotFrom + 16);
    }
    store<f32>(cam + kCamFovBlend, static_cast<f32>(b));
    store<u32>(cam + kCamFovTo, fov);
    store<u32>(cam + kCamFovTo + 4, 0);
    if (snap) {
        store<u32>(cam + kCamFovFrom, fov);
        store<u32>(cam + kCamFovFrom + 4, 0);
    }
    ctx.fpr[1] = lfs_f64(view + kViewFov);  // f1 left holding the fov (no known caller reads it)
    ret(ctx, 0u);
}
