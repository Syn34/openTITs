// Depth of field: blends the blurred scene over the screen by depth
// with PostProc_DofApply.
#include "rsx.h"

#include <cmath>

namespace sly4::gfx {

namespace {

constexpr u32 kDofConsts = 0x0053F898;  // f32 { 1.0, 1e-5, 1.0, 1.0 }
constexpr u32 vp_dof = 0x01164148;      // PostProc_DofApply
constexpr u32 fp_dof = 0x01164150;      // FProgram*[4]: bit 0 e +20 set, bit 1 camera moved
constexpr u32 kLastEye = 0x01164B40;    // f32[3] camera position last compared with (never written here)
constexpr u32 kToScreen = 0x01164800, kToScreenGuard = 0x011647F8;  // f32[4][4], set up on first use
constexpr u32 Mat4_Sub554C28 = 0x00554C28;  // (f32[16]*, f32[16]* out, const f32[16]*) (unknown)

struct Dof {
    pad<20> _0;
    u8 variant;           // +20
    pad<3> _21;
    be_u32 focus[2];      // +24: f32
    be_u32 amount;        // +32: f32
    pad<12> _36;
    be_f32 eye[3];        // +48: camera position
    pad<8> _60;
    be_f32 near_z;        // +68
    be_f32 far_z;         // +72
};
static_assert(offsetof(Dof, eye) == 48 && offsetof(Dof, far_z) == 72);

f32 fma4(f32 a, f32 c, f32 b) { return std::fmaf(a, c, b); }  // vmaddfp lane
u64 pack(u32 hi, u32 lo) { return (u64{hi} << 32) | lo; }

}  // namespace

// void Dof_Draw(Dof* e, Target* blurred)   (vtable slot 20)
// When the camera moved (more than 1e-5 on an axis from kLastEye) the
// view matrix is turned into a screen-to-world transform (constants
// 2..5) and the rotated camera position (constant 6); otherwise the
// near / far terms {1 / near, (near - far) / (far * near)} go into
// constant 1. Alpha blends `blurred` (unit 0) over the screen using the
// scene depth (unit 1).
void Dof_Draw(Dof* e, Texture* blurred) {
    const be_f32* k = host<be_f32>(kDofConsts);
    // The original's frame r1 +112..+383: Mat4_Sub554C28 reads r1 +240
    // (never written here) and writes r1 +304, so the guest frame keeps
    // that layout (offsets below are r1 offsets - 112).
    GuestFrame frame(272);
    const be_f32* input = frame.as<be_f32>(240 - 112);
    const be_f32* a = frame.as<be_f32>(304 - 112);
    const u64 focus = pack(e->focus[0], e->focus[1]);
    u32 nf[2] = {0x3F800000u, 0x3F800000u};

    const be_f32* last = host<be_f32>(kLastEye);
    f32 ex = last[0], ey = last[1], ez = last[2];
    const f32 eps = k[1];
    bool moved = true;
    if (std::fabs(e->eye[0] - ex) < eps && std::fabs(e->eye[1] - ey) < eps && std::fabs(e->eye[2] - ez) < eps) {
        moved = false;
        const f32 n = e->near_z, f = e->far_z;
        nf[0] = bits(k[0] / n);
        nf[1] = bits((n - f) / (f * n));
    }
    f32 r[4][4] = {};
    if (moved) {
        u8& guard = global<u8>(kToScreenGuard);
        be_u32* t = host<be_u32>(kToScreen);
        if (guard == 0) {
            t[0] = 0x40000000u;  // 2
            t[1] = 0u;
            t[2] = 0u;
            t[4] = 0u;
            t[5] = 0xC0000000u;  // -2
            t[6] = 0u;
            t[8] = 0u;
            t[9] = 0u;
            t[10] = 0x3F800000u;  // 1
            t[12] = 0xBF800000u;  // -1
            t[13] = 0x3F800000u;  // 1
            t[14] = 0u;
            guard = 1;
        }
        View* view = or_zero_page(call<View*>(fn::View_GetCurrent));
        call(Mat4_Sub554C28, input, a, view->matrix);
        // R[i] = T[i][0] * A0 + T[i][1] * A1 + T[i][2] * A2 (+ A3 for row 3).
        const be_f32* ts = host<be_f32>(kToScreen);
        for (u32 i = 0; i < 4; ++i) {
            const be_f32* ti = ts + 4 * i;
            for (u32 j = 0; j < 4; ++j) {
                f32 v = fma4(a[j], ti[0], i == 3 ? a[12 + j].get() : 0.0f);
                v = fma4(a[4 + j], ti[1], v);
                r[i][j] = fma4(a[8 + j], ti[2], v);
            }
        }

        const f32 px = -e->eye[0], py = -e->eye[1], pz = -e->eye[2];
        const View* v2 = or_zero_page(call<View*>(fn::View_GetCurrent));
        f32 p[4];
        for (u32 j = 0; j < 4; ++j) {
            f32 v = fma4(v2->rotation[0][j], px, 0.0f);
            v = fma4(v2->rotation[1][j], py, v);
            p[j] = fma4(v2->rotation[2][j], pz, v);
        }
        ex = p[0];
        ey = p[1];
        ez = p[2];
    }

    const u64 amount = pack(0x3F800000u, e->amount);
    const u32 variant = (e->variant != 0 ? 1u : 0u) | (moved ? 2u : 0u);
    bind_vertex(vprog(vp_dof));
    bind_fragment(fprog(fp_dof + 4 * variant));
    set_fconst(0, global<be_u64>(kDofConsts + 8), amount);
    set_fconst(1, focus, pack(nf[0], nf[1]));
    if (moved) {
        for (u32 i = 0; i < 4; ++i)  // transposed: constant 2 + i is column i of R
            set_fconst(2 + i, pack(bits(r[0][i]), bits(r[1][i])), pack(bits(r[2][i]), bits(r[3][i])));
        set_fconst(6, bits(ex), bits(ey), bits(ez), 0x3F800000u);
    }
    blend_on(kSrcAlpha2, kOneMinusSrcAlpha2);
    Draw_ResetState(false, 0);
    Draw_SetTexture0(blurred, 0, 1);
    const Display* disp = display();
    MemStats& ms = mem_stats();
    u32 off;
    if (alternate(disp)) off = 132;
    else if (config_u8(34) != 0) off = ms.buffer_select == 1 ? 104 : 108;
    else off = 88;
    Draw_SetTexture0(*at<gp<Texture>>(&ms, off), 1, 0);
    draw_quad();
}

}  // namespace sly4::gfx

NATIVE(0053F8A8, sly4::gfx::Dof_Draw)
