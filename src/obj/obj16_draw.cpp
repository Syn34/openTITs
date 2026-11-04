// World objects, part 16: the light volume pass.
#include "obj16.h"
#include "../gfx/fx.h"

using namespace sly4;
using namespace sly4::obj16;

namespace {

constexpr u32 View_GetCurrent = 0x004FDBA8;      // () -> View*
constexpr u32 Matrix_LookAt = 0x0056A0D0;        // (out, eye, target, up)
constexpr u32 Matrix_PerspectiveWH = 0x0056A2D8; // (out; f1 w, f2 h, f3 n, f4 f)
constexpr u32 Mat4_Ortho = 0x0056A358;           // (out; f1 l, f2 r, f3 b, f4 t, f5 n, f6 f)
constexpr u32 Mat4_Invert = 0x00554C28;          // (out, tmp, in)
constexpr u32 Vec4_Transform = 0x005539E4;       // (f32 v[4] in/out, const Matrix*)
constexpr u32 Draw_InvalidateState = 0x0051279C;
constexpr u32 LightVolume_DrawOutside = 0x00574370;  // (light, -, viewproj, proj, corners; f1) (name?)
constexpr u32 LightVolume_DrawStencil = 0x00573600;  // (light, mats, viewproj, proj, corners; f1) (name?)

// The light volumes: u16 count at 0x011957E2, the 144-byte lights at
// 0x01195120 (+4 u32 nonzero = orthographic, +16 / +32 / +48 eye / target
// / up, +80 near, +84 far, +88 width, +92 height, +128 f32 handed on in f1)
// and per light the 336-byte matrices at 0x01195990 (+0 projection, +64
// view-projection, +192 the 8 frustum corners (world), +320 u8 the
// frustum lies wholly in front of the view's plane) (name?).
constexpr u32 kLvCount = 0x011957E2, kLvLights = 0x01195120, kLvMats = 0x01195990, kLvOnce = 0x01195988;
constexpr u32 kLightSize = 144, kMatsSize = 336;
constexpr u32 kLtOrtho = 4, kLtEye = 16, kLtTarget = 32, kLtUp = 48, kLtNear = 80, kLtFar = 84, kLtWidth = 88,
              kLtHeight = 92, kLt128 = 128;
constexpr u32 kMsProj = 0, kMsViewProj = 64, kMsCorners = 192, kMsInFront = 320;
constexpr u32 kViewPlane = 336;                  // View: f32[4] (name?)
constexpr u32 kLvConsts = 0x005754D0;            // f32 {-0.5, 0.5, 1.0, 5.0}

// RSX methods (NV4097) and the render state's cached stencil words (+160
// func, +164 ref, +168, +172 mask, +176..184 ops).
constexpr u32 kStencilTestEnable = 0x00040328, kDepthMask = 0x00040A70;
constexpr u32 kStencilOp = 0x000C033C, kStencilMask = 0x0004032C, kStencilFunc = 0x000C0330;
constexpr u32 kGcmKeep = 0x1E00, kGcmAlways = 0x0207;

f32 lf(u32 ea) { return load<f32>(ea); }
f32 fmadd_s(f32 a, f32 c, f32 b) { return static_cast<f32>(std::fma(static_cast<f64>(a), static_cast<f64>(c), static_cast<f64>(b))); }

}  // namespace

// void LightVolumes_Draw()  (Render_Pass575C34's) (name?)
// With stencil testing on and depth writes off: for each light volume its
// view (Matrix_LookAt) and projection (orthographic +-width/2, +-height/2,
// or Matrix_PerspectiveWH) are built, the view-projection (view rows times
// the projection, vmaddfp lane by lane) stored, its inverse taking the NDC
// cube's corners (z 0..1) to the world (each divided by its w). A volume
// whose 8 corners all lie at least 5 in front of the view's plane is drawn
// at once (LightVolume_DrawOutside); when not all were, the others are
// drawn in a second pass (LightVolume_DrawStencil). Then stencil testing
// off, the cached stencil state reset (ops KEEP, func ALWAYS 0, mask 0)
// and sent, and the render state invalidated.
DECOMP(005754E0, LightVolumes_Draw) {
    if (load<u8>(kLvOnce) == 0) store<u8>(kLvOnce, 1);
    const u32 view = static_cast<u32>(call(ctx, View_GetCurrent));
    const u32 plane = view + kViewPlane;
    const u32 st = fx::g::state;
    fx::emit({kStencilTestEnable, 1});
    fx::emit({kDepthMask, 0});
    const u32 count = load<u16>(kLvCount);
    u32 drawn = 0;
    if (count != 0) {
        Stack frame(ctx, 320);  // the original's sp+112 .. sp+432
        const u32 inv_out = frame.addr(), look = inv_out + 64, inv = inv_out + 128, persp = inv_out + 192,
                  ortho = inv_out + 256;
        for (u32 i = 0; i < count; i = (i + 1) & 0xFFFF) {
            const u32 L = kLvLights + kLightSize * i;
            const u32 B = kLvMats + kMatsSize * i;
            const u32 f128 = load<u32>(L + kLt128);
            call(ctx, Matrix_LookAt, {look, L + kLtEye, L + kLtTarget, L + kLtUp});
            u32 src;
            if (load<u32>(L + kLtOrtho) == 0) {
                callf(ctx, Matrix_PerspectiveWH, {persp},
                      {lf(L + kLtWidth), lf(L + kLtHeight), lf(L + kLtNear), lf(L + kLtFar)});
                src = persp;
            } else {
                const f32 w = lf(L + kLtWidth), h = lf(L + kLtHeight);
                const f32 lo = lf(kLvConsts), hi = lf(kLvConsts + 4);
                callf(ctx, Mat4_Ortho, {ortho}, {w * lo, w * hi, h * lo, h * hi, lf(L + kLtNear), lf(L + kLtFar)});
                src = ortho;
            }
            for (u32 k = 0; k < 16; ++k) store<u32>(B + kMsProj + 4 * k, load<u32>(src + 4 * k));  // lfs / stfs
            // view-projection row r = P0 * V[r].x + P1 * V[r].y + P2 * V[r].z (+ P3 for row 3)
            f32 vp[4][4];
            for (u32 r = 0; r < 4; ++r) {
                const f32 x = lf(look + 16 * r), y = lf(look + 16 * r + 4), z = lf(look + 16 * r + 8);
                for (u32 k = 0; k < 4; ++k) {
                    f32 a = fmadd_s(lf(B + 4 * k), x, r == 3 ? lf(B + 48 + 4 * k) : 0.0f);
                    a = fmadd_s(lf(B + 16 + 4 * k), y, a);
                    vp[r][k] = fmadd_s(lf(B + 32 + 4 * k), z, a);
                }
            }
            for (u32 r = 0; r < 4; ++r)
                for (u32 k = 0; k < 4; ++k) store<f32>(B + kMsViewProj + 16 * r + 4 * k, vp[r][k]);
            call(ctx, Mat4_Invert, {inv_out, inv, B + kMsViewProj});
            const u32 C = B + kMsCorners;
            const f32 one = lf(kLvConsts + 8);
            static constexpr u32 kM = 0xBF800000u, kP = 0x3F800000u;  // -1.0, 1.0
            static constexpr u32 kCube[32] = {kM, kM, 0, kP, kP, kM, 0, kP, kP, kP, 0, kP, kM, kP, 0, kP,
                                              kM, kM, kP, kP, kP, kM, kP, kP, kP, kP, kP, kP, kM, kP, kP, kP};
            for (u32 k = 0; k < 32; ++k) store<u32>(C + 4 * k, kCube[k]);
            for (u32 k = 0; k < 8; ++k) {
                const u32 p = C + 16 * k;
                call(ctx, Vec4_Transform, {p, inv});
                const f32 s = one / lf(p + 12);
                const f32 x = lf(p) * s, y = lf(p + 4) * s, z = lf(p + 8) * s, w = lf(p + 12) * s;
                store<f32>(p, x);
                store<f32>(p + 4, y);
                store<f32>(p + 8, z);
                store<f32>(p + 12, w);
            }
            const f32 px = lf(plane), py = lf(plane + 4), pz = lf(plane + 8), pw = lf(plane + 12);
            const f32 margin = lf(kLvConsts + 12);
            u8 in_front = 1;
            for (u32 k = 0; k < 8; ++k) {
                const u32 q = C + 16 * k;
                f32 d = fmadd_s(px, lf(q), pw);
                d = fmadd_s(pz, lf(q + 8), d);
                d = fmadd_s(py, lf(q + 4), d);
                if (d < margin) {
                    in_front = 0;
                    break;
                }
            }
            store<u8>(B + kMsInFront, in_front);
            if (in_front != 0) {
                // r4 is the last corner pointer the test left (unused by the callee).
                callf(ctx, LightVolume_DrawOutside, {L, C + 16 * 7, B + kMsViewProj, B + kMsProj, C},
                      {static_cast<f64>(std::bit_cast<f32>(f128))});
                drawn = (drawn + 1) & 0xFFFF;
            }
        }
    }
    if (static_cast<s32>(drawn) < static_cast<s32>(count) && count != 0) {
        for (u32 i = 0; i < count; i = (i + 1) & 0xFFFF) {
            const u32 B = kLvMats + kMatsSize * i;
            if (load<u8>(B + kMsInFront) != 0) continue;
            const u32 L = kLvLights + kLightSize * i;
            callf(ctx, LightVolume_DrawStencil, {L, B, B + kMsViewProj, B + kMsProj, B + kMsCorners}, {lf(L + kLt128)});
        }
    }
    fx::emit({kStencilTestEnable, 0});
    store<u32>(st + 176, kGcmKeep);
    store<u32>(st + 180, kGcmKeep);
    store<u32>(st + 184, kGcmKeep);
    store<u32>(st + 160, kGcmAlways);
    store<u32>(st + 164, 0);
    store<u32>(st + 172, 0);
    store<u32>(st + 168, 0);
    fx::emit({kStencilOp, kGcmKeep, kGcmKeep, kGcmKeep});
    fx::emit({kStencilMask, 0});
    fx::emit({kStencilFunc, load<u32>(st + 160), load<u32>(st + 164), load<u32>(st + 172)});
    call(ctx, Draw_InvalidateState);
}
