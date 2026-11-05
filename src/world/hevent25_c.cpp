// World_HandleEvent callees, part 25c: the user camera controller switch.
#include <bit>
#include <cmath>

#include "hevent.h"

using namespace sly4;
using namespace sly4::hevent;

namespace {
constexpr u32 kWorldCamera = 0x1281E0;         // World +: Camera* (as hevent_b.cpp kWorldUserActive)
constexpr u32 kWorldController = 0x128584;     // World +: the camera controller made here
constexpr u32 kReleaseController = 0x004845F0; // (World*, UserBlock*) (name?)
constexpr u32 kSmallAlloc = 0x0055238C;
constexpr u32 kPushTarget = 0x000FC040;        // (Camera*, Target*, -, u8; f32) (symbols.txt Camera_PushTarget0FC040)
constexpr u32 kRestoreFrom544 = 0x000FCC3C;    // (Camera*) (symbols.txt Obj_RestoreFrom544)
// Controller constructors (ctrl, Camera*, Object* target, ...) (name?).
constexpr u32 kCtrlFollow = 0x000FF0D8;        // 1040 bytes: mode 1, and the default
constexpr u32 kCtrlMode0 = 0x0010DE30;         // 608 bytes
constexpr u32 kCtrlMode2 = 0x00111410;         // 272 bytes
constexpr u32 kCtrlMode3 = 0x00113608;         // 128 bytes; f1 = the profile's +688
constexpr u32 kCtrlAlt = 0x00114298;           // 176 bytes; fourth argument 0
constexpr u32 kAltSwitch = 0x010BBE68 + 72;    // u8: use kCtrlAlt for plain targets
constexpr u32 kBlendConsts = 0x0048484C;       // f32 -1.0, 0.0 (code segment; the address is also passed)
}  // namespace

// u32 World_SetCameraTarget484858(World* w, UserBlock* ub) (name?): with a
// camera (w +0x1281E0), drops the old controller (0x004845F0) and, for the
// user's +8 object, builds one: objects with +200 bit 10 by their profile's
// (+216) +240 mode (0: 608-byte, 1: 1040-byte, 2: 272-byte, 3: 128-byte
// with the profile's +688; other modes none), others with +192 bit 3 a
// 176-byte one when the u8 at 0x010BBEB0 is set, else the 1040-byte one.
// The controller (or 0) is stored at w +0x128584; a new one is pushed on
// the camera (blend -1.0), the camera gets vt +20 (0.0) and
// Obj_RestoreFrom544, and the controller vt +28. r3 as the original
// leaves it.
DECOMP(00484858, World_SetCameraTarget484858) {
    const u32 w = arg<u32>(ctx, 0), ub = arg<u32>(ctx, 1);
    if (load<u32>(w + kWorldCamera) == 0) return ret(ctx, 0u);
    call(ctx, kReleaseController, {w, ub});
    store<u32>(w + kWorldController, 0);
    const u32 t = load<u32>(ub + 8);
    if (t == 0) return ret(ctx, kWorldController);
    u32 ctrl = 0;
    const auto make = [&](u32 size, u32 ctor) {
        if (const u32 s = static_cast<u32>(call(ctx, kSmallAlloc, {size})); s != 0) {
            if (ctor == kCtrlAlt) call(ctx, ctor, {s, load<u32>(w + kWorldCamera), t, 0});
            else call(ctx, ctor, {s, load<u32>(w + kWorldCamera), t});
            ctrl = s;
        }
    };
    if ((load<u32>(t + 200) & 0x400) != 0) {
        const u32 prof = load<u32>(t + 216) + 240;
        const s32 mode = static_cast<s32>(load<u32>(prof));
        if (mode < 0 || mode > 3) return ret(ctx, static_cast<u32>(mode));
        if (mode == 3) {
            if (const u32 s = static_cast<u32>(call(ctx, kSmallAlloc, {128})); s != 0) {
                callf(ctx, kCtrlMode3, {s, load<u32>(w + kWorldCamera), t}, {static_cast<f64>(load<f32>(prof + 448))});
                ctrl = s;
            }
        } else if (mode == 2) {
            make(272, kCtrlMode2);
        } else if (mode == 1) {
            make(1040, kCtrlFollow);
        } else {
            make(608, kCtrlMode0);
        }
    } else {
        if ((load<u32>(t + 192) & 8) == 0) return ret(ctx, 0u);
        if (load<u8>(kAltSwitch) != 0) make(176, kCtrlAlt);
        else make(1040, kCtrlFollow);
    }
    store<u32>(w + kWorldController, ctrl);
    if (ctrl == 0) return ret(ctx, 0u);
    callf(ctx, kPushTarget, {load<u32>(w + kWorldCamera), ctrl, kBlendConsts, 0}, {static_cast<f64>(load<f32>(kBlendConsts))});
    const u32 cam = load<u32>(w + kWorldCamera);
    if (cam == 0) return ret(ctx, 0u);
    ctx.fpr[1] = static_cast<f64>(load<f32>(kBlendConsts + 4));
    vcall(ctx, cam, 20, {cam});
    call(ctx, kRestoreFrom544, {load<u32>(w + kWorldCamera)});
    const u32 c = load<u32>(w + kWorldController);
    if (c == 0) return ret(ctx, kWorldController);
    vcall(ctx, c, 28, {c});
}

// ---------------------------------------------------------------- 0x000F94C0
namespace {
constexpr u32 kScreen = 0x010C5A20;            // u16 +4 width, +6 height
constexpr u32 kOrigin = 0x01164AFC;            // f32 x, y of the full-screen rectangle
constexpr u32 kCustomSize = 0x010B1808;        // f32 +72 width, +76 height (used when both >= 0)
// f32 constants in the code segment: +0 0.0, +4 2.0 (gap), +32 0.5, +36
// 10.0, +40 0.125, +44 0.604, +48 0.0625, +52 3.0.
constexpr u32 kSplitConsts = 0x000F9488;
constexpr u32 kSetViewport = 0x005200AC;       // (RenderTarget*, ViewRect*) (symbols.txt RenderTarget_SetViewport)
constexpr u32 kViewFlags = 344, kViewRect = 346, kViewTarget = 364;

f32 cst(u32 off) { return load<f32>(kSplitConsts + off); }
f32 fadds(f32 a, f32 b) { return static_cast<f32>(static_cast<f64>(a) + static_cast<f64>(b)); }
f32 fmadds(f32 a, f32 c, f32 b) { return static_cast<f32>(std::fma(static_cast<f64>(a), static_cast<f64>(c), static_cast<f64>(b))); }
f32 fmsubs(f32 a, f32 c, f32 b) { return static_cast<f32>(std::fma(static_cast<f64>(a), static_cast<f64>(c), -static_cast<f64>(b))); }
// fctidz, then the low halfword of the result's low word.
u16 trunc16(f32 v) { return static_cast<u16>(rt::fctid(static_cast<f64>(v), true)); }
}  // namespace

// u32 View_SetSplit0F94C0(View* v, s32 split) (name?): the viewport
// rectangle (u16 x, y, w, h at +346) for the split-screen layout split,
// from the screen size (or the custom size at 0x010B1850 / 54, centred):
// 0 / above 10 full screen; 1 letterboxed (y + h / 10, h - h / 8 * 2); 2 /
// 3 top / bottom half; 4..9 halves and quarters with 2-pixel gaps; 10 a
// 0.604 / 0.0625 offset third-size inset. The render target (+364) is
// updated when +344 bit 31 is set; +344 bits 27..30 = split, returned.
DECOMP(000F94C0, View_SetSplit0F94C0) {
    const u32 v = arg<u32>(ctx, 0);
    const s32 split = arg<s32>(ctx, 1);
    f32 w = static_cast<f32>(load<u16>(kScreen + 4));
    f32 h = static_cast<f32>(load<u16>(kScreen + 6));
    f32 x = load<f32>(kOrigin), y = load<f32>(kOrigin + 4);
    const f32 zero = cst(0);
    if (const f32 pw = load<f32>(kCustomSize + 72); pw >= zero && load<f32>(kCustomSize + 76) >= zero) {
        const f32 ph = load<f32>(kCustomSize + 76);
        const f32 half = cst(32);
        x = fmuls(static_cast<f32>(static_cast<f64>(w) - static_cast<f64>(pw)), half);
        y = fmuls(static_cast<f32>(static_cast<f64>(h) - static_cast<f64>(ph)), half);
        w = pw;
        h = ph;
    }
    const f32 gap = cst(4);
    const u32 r = v + kViewRect;
    f32 f1;
    if (split <= 0 || split > 10) {
        store<u16>(r + 0, trunc16(x));
        store<u16>(r + 2, trunc16(y));
        store<u16>(r + 4, trunc16(w));
        store<u16>(r + 6, trunc16(h));
        f1 = y;
    } else if (split == 1) {
        store<u16>(r + 0, trunc16(x));
        store<u16>(r + 2, trunc16(fadds(y, static_cast<f32>(static_cast<f64>(h) / static_cast<f64>(cst(36))))));
        store<u16>(r + 4, trunc16(w));
        const f32 bar = fmuls(h, cst(40));
        f1 = static_cast<f32>(-std::fma(static_cast<f64>(bar), static_cast<f64>(gap), -static_cast<f64>(h)));
        store<u16>(r + 6, trunc16(f1));
    } else if (split <= 3) {
        store<u16>(r + 0, trunc16(x));
        const f32 top = split == 2 ? y : fadds(fmadds(h, cst(32), y), gap);
        store<u16>(r + 2, trunc16(top));
        store<u16>(r + 4, trunc16(w));
        f1 = fmsubs(h, cst(32), gap);
        store<u16>(r + 6, trunc16(f1));
    } else if (split <= 5) {
        const f32 left = split == 4 ? x : fadds(fmadds(w, cst(32), x), gap);
        store<u16>(r + 0, trunc16(left));
        store<u16>(r + 2, trunc16(y));
        f1 = fmsubs(w, cst(32), gap);
        store<u16>(r + 4, trunc16(f1));
        store<u16>(r + 6, trunc16(h));
    } else if (split <= 9) {
        const f32 left = ((static_cast<u32>(split) - 2) & 1) != 0 ? x : fadds(fmadds(w, cst(32), x), gap);
        store<u16>(r + 0, trunc16(left));
        const f32 top = split < 8 ? y : fadds(fmadds(h, cst(32), y), gap);
        store<u16>(r + 2, trunc16(top));
        store<u16>(r + 4, trunc16(fmsubs(w, cst(32), gap)));
        f1 = fmsubs(h, cst(32), gap);
        store<u16>(r + 6, trunc16(f1));
    } else {  // 10
        store<u16>(r + 0, trunc16(fmadds(w, cst(44), x)));
        store<u16>(r + 2, trunc16(fmadds(h, cst(48), y)));
        store<u16>(r + 4, trunc16(static_cast<f32>(static_cast<f64>(w) / static_cast<f64>(cst(52)))));
        f1 = static_cast<f32>(static_cast<f64>(h) / static_cast<f64>(cst(52)));
        store<u16>(r + 6, trunc16(f1));
    }
    // f1 as the original leaves it (also seen by the callee): the fctidz
    // of the last value converted.
    ctx.fpr[1] = std::bit_cast<f64>(rt::fctid(static_cast<f64>(f1), true));
    u32 flags = load<u32>(v + kViewFlags);
    if ((flags & 0x80000000u) != 0) {
        call(ctx, kSetViewport, {load<u32>(v + kViewTarget), v + kViewRect});
        flags = load<u32>(v + kViewFlags);
    }
    flags = (flags & ~0x78000000u) | ((static_cast<u32>(split) << 27) & 0x78000000u);
    store<u32>(v + kViewFlags, flags);
    ret(ctx, flags);
}
