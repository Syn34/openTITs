// The motion trail: a ping-pong pair of screen copies, offset by the
// camera motion each frame and smeared onto the screen targets.
#include "rsx.h"

#include <cmath>

namespace sly4::gfx {

namespace {

constexpr u32 vp_smear = 0x010F0B48, fp_smear = 0x010F0B4C;  // PostProc_MotionTrailSmear
constexpr u32 vp_copy_scale = 0x01164110, fp_copy_scale = 0x01164114;  // PostProc_CopyScale
constexpr u32 kAddressMode = 0x00030404;  // u / v wrap mode 4 (border?), w clamp
constexpr u32 kRenderer = 0x010DDACC;     // object*: vtable slot 16 is called after the trail copy (name?)

struct MotionTrail {
    pad<96> _0;
    be_u32 colour[4];       // +96: f32
    pad<16> _112;
    be_f32 offset_scale;    // +128
    be_f32 smear_scale;     // +132
    be_f32 threshold;       // +136
    gp<Target> depth;       // +140
    gp<Target> targets[2];  // +144
    gp<Texture> textures[2];  // +152
    be_u32 index;           // +160: current
};
static_assert(offsetof(MotionTrail, depth) == 140 && offsetof(MotionTrail, index) == 160);

// Clears the bound colour target to 0 (depth / stencil 0xFFFFFF00).
void clear() {
    RenderState& st = rs();
    call(fn::cellGcmSetClearColor, gcm(), 0u);
    call(fn::cellGcmSetClearDepthStencil, gcm(), 0xFFFFFF00u);
    call(fn::cellGcmSetStencilMask, gcm(), 255u);
    call(fn::cellGcmSetClearSurface, gcm(), 240u);  // R | G | B | A
    call(fn::cellGcmSetStencilMask, gcm(), st.stencil_mask.get());
}

// Blending, alpha / depth / stencil test and culling off.
void plain_state() {
    emit({kBlendEnable, 0});
    emit({kAlphaTestEnable, 0});
    emit({kDepthMask, 0});
    emit({kDepthTestEnable, 0});
    emit({kStencilTestEnable, 0});
    emit({kCullFaceEnable, 0});
}

}  // namespace

// void MotionTrail_Draw(MotionTrail* e, const f32* motion, bool notify)   (vtable slot 32)
// Clears the other trail target and copies the current one into it
// (PostProc_CopyScale, shifted by -motion[0..1] * e +128 in vertex
// constant 0). If |motion.xy|^2 > e +136: vtable slot 36 (when notify),
// Draw_InvalidateState, rebind the other target with depth e +140 and
// call the renderer's slot 16. Finally smears the other texture into the
// current target with PostProc_MotionTrailSmear, colour motion * e +132.
void MotionTrail_Draw(MotionTrail* e, const be_f32* motion, bool notify) {
    GuestFrame f(16);  // vertex constant 0: {0, 0, -dx, -dy}
    be_u32* vc = f.as<be_u32>();
    vc[0] = 0u;
    vc[1] = 0u;
    const f32 k = e->offset_scale;
    const u32 index = e->index;
    const u32 other = index ^ 1, current = index;
    vc[2] = bits(-(motion[0] * k));
    vc[3] = bits(-(motion[1] * k));

    Draw_BindTargets(nullptr, *at<gp<Target>>(e, 144 + (other << 2)), 0);
    clear();
    bind_vertex(vprog(vp_copy_scale));
    bind_fragment(fprog(fp_copy_scale));
    call(fn::cellGcmSetVertexProgramConstants, gcm(), 0u, 4u, vc);
    set_fconst_from(0, e->colour);
    plain_state();
    Draw_SetTexture0(*at<gp<Texture>>(e, 152 + (current << 2)), 0, 1);
    emit({kTextureAddress0, kAddressMode});
    draw_quad();

    const f32 vx = motion[0], vy = motion[1];
    const f32 len2 = static_cast<f32>(std::fma(static_cast<f64>(vx), static_cast<f64>(vx), static_cast<f64>(vy * vy)));
    if (len2 > e->threshold) {
        if (notify) vcall(e, 36);
        Draw_InvalidateState();
        Draw_BindTargets(e->depth, *at<gp<Target>>(e, 144 + (other << 2)), 0);
        invalidate_textures();
        u8* r = global<gp<u8>>(kRenderer);
        vcall(r, 16);
    }

    const f32 s = e->smear_scale;
    const u32 c0 = bits(motion[0] * s), c1 = bits(motion[1] * s);
    const u32 c2 = bits(motion[2] * s), c3 = bits(motion[3] * s);
    Draw_BindTargets(nullptr, *at<gp<Target>>(e, 144 + (current << 2)), 0);
    bind_vertex(vprog(vp_smear));
    bind_fragment(fprog(fp_smear));
    set_fconst(0, c0, c1, c2, c3);
    plain_state();
    Draw_SetTexture0(*at<gp<Texture>>(e, 152 + (other << 2)), 0, 1);
    emit({kTextureAddress0, kAddressMode});
    draw_quad();
}

}  // namespace sly4::gfx

NATIVE(00544120, sly4::gfx::MotionTrail_Draw)
