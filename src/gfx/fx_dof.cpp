// Depth of field: blends the blurred scene over the screen by depth
// with PostProc_DofApply.
#include "fx.h"

#include <cmath>

using namespace sly4;

namespace {

namespace dfn = sly4::draw::fn;
namespace fg = sly4::fx::g;
using namespace sly4::fx;  // helpers and method names (fx::g is spelt out)

constexpr u32 kStats = sly4::g::mem_stats;
constexpr u32 kConfig = sly4::g::config;
constexpr u32 kDofConsts = 0x0053F898;  // f32 { 1.0, 1e-5, 1.0, 1.0 }
constexpr u32 vp_dof = 0x01164148;      // PostProc_DofApply
constexpr u32 fp_dof = 0x01164150;      // FProgram*[4]: bit 0 e +20 set, bit 1 camera moved
constexpr u32 kLastEye = 0x01164B40;    // f32[3] camera position last compared with (never written here)
constexpr u32 kLastMatrix = 0x01164C10; // f32[16] (never written here)
constexpr u32 kToScreen = 0x01164800, kToScreenGuard = 0x011647F8;  // f32[4][4], set up on first use

namespace dofn {
constexpr u32 View_GetCurrent = 0x004FDBA8;  // () -> view; +752 f32[3][4] rotation rows, +1008 matrix (name?)
constexpr u32 Mat4_Sub554C28 = 0x00554C28;   // (f32[16]*, f32[16]* out, const f32[16]*) (unknown)
}  // namespace dofn

f32 fma4(f32 a, f32 c, f32 b) { return std::fmaf(a, c, b); }  // vmaddfp lane

}  // namespace

// void Dof_Draw(Dof* e, Target* blurred)   (vtable slot 20)
// e: +20 u8 variant bit, +24 / +28 f32 focus parameters, +32 f32
// amount, +48 f32[3] camera position, +68 / +72 f32 near / far.
// When the camera moved (more than 1e-5 on an axis from kLastEye) the
// view matrix is turned into a screen-to-world transform (constants
// 2..5) and the rotated camera position (constant 6); otherwise the
// near / far terms {1 / near, (near - far) / (far * near)} go into
// constant 1. Alpha blends `blurred` (unit 0) over the screen using the
// scene depth (unit 1).
DECOMP(0053F8A8, Dof_Draw) {
    const u32 e = arg<u32>(ctx, 0), blurred = arg<u32>(ctx, 1);
    const u32 k = kDofConsts;
    Stack frame(ctx, 272);  // mirrors the original's r1 +112..+383
    const u32 sp = frame.addr() - 112;
    store<u32>(sp + 120, 0x3F800000);
    store<u32>(sp + 124, 0x3F800000);
    store<u32>(sp + 112, load<u32>(e + 24));
    store<u32>(sp + 116, load<u32>(e + 28));
    for (u32 i = 0; i < 64; i += 4) store<u32>(sp + 176 + i, load<u32>(kLastMatrix + i));
    const u64 focus = load<u64>(sp + 112);

    f32 ex = load_f32(kLastEye), ey = load_f32(kLastEye + 4), ez = load_f32(kLastEye + 8);
    const f32 eps = load_f32(k + 4);
    bool moved = true;
    if (std::fabs(load_f32(e + 48) - ex) < eps && std::fabs(load_f32(e + 52) - ey) < eps &&
        std::fabs(load_f32(e + 56) - ez) < eps) {
        moved = false;
        const f32 n = load_f32(e + 68), f = load_f32(e + 72);
        store<u32>(sp + 120, bits(load_f32(k) / n));
        store<u32>(sp + 124, bits((n - f) / (f * n)));
    }
    if (moved) {
        if (ref<u8>(kToScreenGuard) == 0) {
            const u32 t = kToScreen;
            store<u32>(t, 0x40000000);  // 2
            store<u32>(t + 4, 0);
            store<u32>(t + 8, 0);
            store<u32>(t + 16, 0);
            store<u32>(t + 20, 0xC0000000);  // -2
            store<u32>(t + 24, 0);
            store<u32>(t + 32, 0);
            store<u32>(t + 36, 0);
            store<u32>(t + 40, 0x3F800000);  // 1
            store<u32>(t + 48, 0xBF800000);  // -1
            store<u32>(t + 52, 0x3F800000);  // 1
            store<u32>(t + 56, 0);
            ref<u8>(kToScreenGuard) = 1;
        }
        const u32 view = static_cast<u32>(call(ctx, dofn::View_GetCurrent));
        const u32 a = sp + 304;
        call(ctx, dofn::Mat4_Sub554C28, {sp + 240, a, view + 1008});
        // R[i] = T[i][0] * A0 + T[i][1] * A1 + T[i][2] * A2 (+ A3 for row 3),
        // stored transposed at sp +176.
        f32 r[4][4];
        for (u32 i = 0; i < 4; ++i) {
            const u32 t = kToScreen + 16 * i;
            for (u32 j = 0; j < 4; ++j) {
                f32 v = fma4(load_f32(a + 4 * j), load_f32(t), i == 3 ? load_f32(a + 48 + 4 * j) : 0.0f);
                v = fma4(load_f32(a + 16 + 4 * j), load_f32(t + 4), v);
                r[i][j] = fma4(load_f32(a + 32 + 4 * j), load_f32(t + 8), v);
            }
        }
        for (u32 i = 0; i < 4; ++i)
            for (u32 j = 0; j < 4; ++j) store<u32>(sp + 176 + 16 * j + 4 * i, bits(r[i][j]));

        const f32 px = -load_f32(e + 48), py = -load_f32(e + 52), pz = -load_f32(e + 56);
        store<u32>(sp + 128, bits(px));
        store<u32>(sp + 132, bits(py));
        store<u32>(sp + 136, bits(pz));
        const u32 rot = static_cast<u32>(call(ctx, dofn::View_GetCurrent)) + 752;
        for (u32 j = 0; j < 4; ++j) {
            f32 v = fma4(load_f32(rot + 4 * j), px, 0.0f);
            v = fma4(load_f32(rot + 16 + 4 * j), py, v);
            v = fma4(load_f32(rot + 32 + 4 * j), pz, v);
            store<u32>(sp + 128 + 4 * j, bits(v));
        }
        ex = load_f32(sp + 128);
        ey = load_f32(sp + 132);
        ez = load_f32(sp + 136);
    }

    store<u32>(sp + 152, 0x3F800000);
    store<u32>(sp + 156, load<u32>(e + 32));
    const u64 amount = load<u64>(sp + 152);
    const u32 variant = (ref<u8>(e + 20) != 0 ? 1u : 0u) | (moved ? 2u : 0u);
    bind_vertex(ctx, vp_dof);
    bind_fragment(fp_dof + 4 * variant);
    set_fconst(0, load<u64>(k + 8), amount);
    set_fconst(1, focus, load<u64>(sp + 120));
    if (moved) {
        for (u32 i = 0; i < 4; ++i) set_fconst(2 + i, load<u64>(sp + 176 + 16 * i), load<u64>(sp + 184 + 16 * i));
        set_fconst(6, bits(ex), bits(ey), bits(ez), 0x3F800000);
    }
    blend_on(kSrcAlpha2, kOneMinusSrcAlpha2);
    call(ctx, dfn::Draw_ResetState, {0, 0});
    call(ctx, dfn::Draw_SetTexture0, {blurred, 0, 1});
    const u32 disp = load<u32>(sly4::draw::g::display);
    u32 depth;
    if ((ref<u8>(disp + 148) >> 7) != 0) depth = load<u32>(kStats + 132);
    else if (field<u8>(kConfig, 34) != 0) depth = load<u32>(kStats + (load<u32>(kStats + 184) == 1 ? 104 : 108));
    else depth = load<u32>(kStats + 88);
    call(ctx, dfn::Draw_SetTexture0, {depth, 1, 0});
    ctx.gpr[3] = draw_quad(ctx);
}
