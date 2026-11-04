// World_HandleEvent callees, part 12e: the volume obstacle shape builder.
#include "hevent12.h"

using namespace sly4;
using namespace sly4::hevent12;

namespace {
// Obstacle list (as Volume_Notify112D8): +0 u32 active, +32 s32 count,
// +28708 Shape slots (u32 each), at most 512.
constexpr u32 kLstActive = 0, kLstCount = 32, kLstItems = 28708;
constexpr s32 kLstMax = 512;
constexpr u32 kShapeReset = 0x0063778C;   // (Shape*) (name?)
constexpr u32 kMatToQuat = 0x0067C024;    // (quat* out, const vec3* row2, const vec3* row0) (name?)
constexpr u32 kShapeMakeBox = 0x006382A8; // (Shape* out, const vec3* pos, const quat*, const vec3* extent zxy, const BoxParams*) (name?)
constexpr u32 kShapeAssign = 0x00637D14;  // Volume_SetShape637D14 (dst, src)
constexpr u32 kShapeRelease = 0x00638498; // Shape_Release638498 (Shape*)
constexpr u32 kStringFormat = 0x005534D0; // String_Format (fmt, ...) (symbols.txt)
constexpr u32 kOverflowFmt = 0x00AC016C;  // "Obstacle list overflow, max is (%d)"
constexpr u32 kBoxRadiusBits = 0x41700000; // 15.0f
// Stack block at r1+112 (offsets from it).
constexpr u32 kSpParams = 0, kSpTmp = 32, kSpExtent = 36, kSpPos = 48, kSpRow2 = 60, kSpRow0 = 72, kSpRow1 = 84;
constexpr u32 kSpQuat = 96, kSpQuatOut = 112, kSpSize = 128;
// Params: +0 s32 -1, +4 f32 15, +8 u32 mask, +16 u64 0, +24 u32 name.
constexpr u32 kPrId = 0, kPrRadius = 4, kPrMask = 8, kPrZero = 16, kPrName = 24;

void copy3(u32 dst, u32 src) {
    for (u32 k = 0; k < 12; k += 4) store<u32>(dst + k, load<u32>(src + k));
}
}  // namespace

// void Shape_Init110EC(Shape* out, ObstacleList* l, const Mat34* m, const vec3*
// extent, u32 name, u32 mask) (name?): resets out; for an active list with
// room builds a box shape (rotation of m as a quaternion, position m +48,
// extent as z, x, y, radius 15) into out and appends a copy to the list;
// reports "Obstacle list overflow" when it's full.
DECOMP(000110EC, Shape_Init110EC) {
    const u32 out = arg<u32>(ctx, 0), l = arg<u32>(ctx, 1), m = arg<u32>(ctx, 2);
    const u32 ext = arg<u32>(ctx, 3), name = arg<u32>(ctx, 4), mask = arg<u32>(ctx, 5);
    call(ctx, kShapeReset, {out, l, m, ext, name, mask});  // r4..r8 still hold the arguments
    if (load<u32>(l + kLstActive) == 0) return ret(ctx, 0u);
    if (static_cast<s32>(load<u32>(l + kLstCount)) >= kLstMax) {
        call(ctx, kStringFormat, {kOverflowFmt, static_cast<u32>(kLstMax)});
        return;
    }
    Stack frame(ctx, kSpSize);
    const u32 sp = frame.addr();
    copy3(sp + kSpPos, m + 48);
    copy3(sp + kSpRow0, m);
    copy3(sp + kSpRow1, m + 16);
    copy3(sp + kSpRow2, m + 32);
    ctx.fpr[1] = hevent7::lfs_f64(m + 40);  // f1 as the original leaves it
    call(ctx, kMatToQuat, {sp + kSpQuatOut, sp + kSpRow2, sp + kSpRow0, sp + kSpRow1, m + 32});
    for (u32 k = 0; k < 16; k += 4) store<u32>(sp + kSpQuat + k, load<u32>(sp + kSpQuatOut + k));
    store<u32>(sp + kPrRadius, kBoxRadiusBits);
    store<u32>(sp + kPrId, 0xFFFFFFFFu);
    store<u64>(sp + kPrZero, 0);
    store<u32>(sp + kPrName, name);
    store<u32>(sp + kPrMask, mask);
    store<u32>(sp + kSpExtent, load<u32>(ext + 8));
    store<u32>(sp + kSpExtent + 4, load<u32>(ext));
    store<u32>(sp + kSpExtent + 8, load<u32>(ext + 4));
    ctx.fpr[1] = hevent7::lfs_f64(ext + 8);  // f1..f3 as the original leaves them
    ctx.fpr[2] = hevent7::lfs_f64(ext);
    ctx.fpr[3] = hevent7::lfs_f64(ext + 4);
    call(ctx, kShapeMakeBox, {sp + kSpTmp, sp + kSpPos, sp + kSpQuat, sp + kSpExtent, sp + kSpParams});
    call(ctx, kShapeAssign, {out, sp + kSpTmp});
    call(ctx, kShapeRelease, {sp + kSpTmp});
    call(ctx, kShapeAssign, {load<u32>(l + kLstItems) + load<u32>(l + kLstCount) * 4, out});
    const u32 n = load<u32>(l + kLstCount) + 1;
    store<u32>(l + kLstCount, n);
    ret(ctx, n);  // r3 as the original leaves it
}
