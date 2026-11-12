// The bloom-noise post effect: three rotated, scaled and scrolled layers
// of noise (with an optional bloom of the scene) drawn into a work
// target and added back onto the screen.
#include "rsx.h"

#include <cmath>

namespace sly4::gfx {

namespace {

constexpr u32 kConsts = 0x00540DC4;  // f32 { 0.0, 6.0, 0.25, 1/255, 255.0 }
constexpr u32 vp_down = 0x01164160, fp_down = 0x01164164;  // PostProc_BloomDownSample4x (BloomNoise_Init)
constexpr u32 vp_noise = 0x01164168;  // PostProc_BloomNoise
constexpr u32 fp_noise = 0x01164170;  // FProgram*[2]: PostProc_BloomNoise_NoBloom, PostProc_BloomNoise
constexpr u32 vp_blur11h = 0x011640B0, fp_blur11h = 0x011640B4;
constexpr u32 vp_blur11v = 0x011640B8, fp_blur11v = 0x011640BC;
constexpr u32 vp_copy = 0x01164080, fp_copy = 0x01164084;
constexpr u32 kAxis = 0x01164B70;     // f32[3]: axis the noise layers rotate about (bss)
constexpr u32 kNoiseAddress = 0x00030101;  // TEXTURE_ADDRESS word for the noise textures
constexpr u32 kNoiseDst = 0x00000302;      // blend dfactor word of the noise pass
constexpr u32 Matrix_FromAxisAngle = 0x0056A410;  // (f32[16]* out, const f32[3]* axis, f32 angle)
constexpr u32 Matrix_ScaleRows = 0x00554394;      // (f32[16]*, const f32[3]* scale)

// Fragment parameter records of the noise programs (bindings at 0x00F67790).
enum : u32 {
    kIndirectMtx1 = 0,  // gps_vIndirectMtx1[0..2]; layer i uses 3 * i ..
    kWeights = 9,       // gps_vWeights
    kNoiseColorTint = 10,
    kMaskParamsX = 11,
    kMaskParamsY = 12,
};

struct NoiseLayer {
    be_u32 scale[2];   // +0: f32 x, y
    be_f32 scroll[2];  // +8
    be_f32 angle;      // +16
    be_u32 weight;     // +20: f32
};
struct TextureHolder {
    pad<56> _0;
    Texture tex;  // +56
};
struct BloomNoise {
    pad<20> _0;
    NoiseLayer layers[3];      // +20
    u8 tint[4];                // +92
    be_f32 tint_scale;         // +96
    be_f32 tint_alpha;         // +100
    gp<TextureHolder> noise0;  // +104
    be_f32 mask[6];            // +108..+128
    gp<TextureHolder> noise1;  // +132
    pad<8> _136;
    be_f32 threshold[3];       // +144: 0..255
    pad<4> _156;
    be_f32 bloom;              // +160
    be_f32 mask_scale;         // +164
};
static_assert(offsetof(BloomNoise, tint) == 92 && offsetof(BloomNoise, noise1) == 132 && offsetof(BloomNoise, mask_scale) == 164);

bool has_fconst(u32 k) { return at<FParam>(rs().fprogram.get(), 592 + 16 * k)->param != 0; }
u64 pack(u32 hi, u32 lo) { return (u64{hi} << 32) | lo; }

void bind(u32 vertex, u32 fragment) {
    bind_vertex(vprog(vertex));
    bind_fragment(fprog(fragment));
}

}  // namespace

// void BloomNoise_Draw(BloomNoise* e)   (virtual)
// With bloom (e +160 > 0) the scene is down-sampled past the threshold
// into work 3 and blurred once (work 1, work 3) as Bloom_Chain does; the
// blurred result (work 2) is unit 2 of the noise pass. The noise pass
// draws into work 1 with blending (ONE / src alpha); each layer's
// rotation (Matrix_FromAxisAngle about kAxis), scale and the fractional
// part of its rotated scroll go into gps_vIndirectMtx<i>. Work 0 is then
// added onto the scene target (PostProc_Copy), and the display depth
// and scene targets are bound again.
// The xyz-only layer rows leave the w of three constant records
// (stack r1 +236 / +252 / +268) unwritten: the original sends whatever
// the stack held there, so the guest frame keeps the original's layout
// (r1 +112..+527; sp(off) below is r1 + off).
void BloomNoise_Draw(BloomNoise* e) {
    GuestFrame frame(416);
    auto sp = [&](u32 off) { return frame.as<be_u32>(off - 112); };
    auto spf = [&](u32 off) { return frame.as<be_f32>(off - 112); };
    const be_f32* k = host<be_f32>(kConsts);

    Draw_SetDepthTarget(nullptr);
    Texture* tex = Display_SelectBuffer(&mem_stats());  // the scene colour
    bool bloom = false;
    if (e->bloom > k[0]) {
        const f32 c255 = k[4];
        const u32 t0 = bits(e->threshold[0] / c255), t1 = bits(e->threshold[1] / c255), t2 = bits(e->threshold[2] / c255);
        Texture* half = work_texture(2);
        set_target(work_target(3));
        bind(vp_down, fp_down);
        set_fconst(0, pack(t0, t1), pack(t2, 0));
        Draw_ResetState(true, 0);
        Draw_SetTexture0(tex, 0, 1);
        draw_quad();
        for (u32 i = 0; i < 1; ++i) {
            Texture* h = work_texture(0);
            set_target(work_target(1));
            bind(vp_blur11h, fp_blur11h);
            Blur11H_SetOffsets();
            Draw_ResetState(true, 0);
            Draw_SetTexture0(half, 0, 0);
            draw_quad();
            half = work_texture(2);
            set_target(work_target(3));
            bind(vp_blur11v, fp_blur11v);
            Blur11V_SetOffsets();
            Draw_ResetState(true, 0);
            Draw_SetTexture0(h, 0, 0);
            draw_quad();
        }
        bloom = true;
        tex = half;
    }

    // The noise pass.
    const u8* ss = system_state();
    const f32 weight = e->bloom / k[1] * k[2];
    const f32 s = *at<be_f32>(ss, 16);
    const u32 fp = fp_noise + (bloom ? 4u : 0u);
    Texture* screen = work_texture(0);
    set_target(work_target(1));
    bind(vp_noise, fp);
    Draw_ResetState(true, 0);
    emit({kBlendEnable, 1});
    emit({kBlendFunc, kOne2, kNoiseDst});
    emit({kBlendEquation, kFuncAdd2});
    if (TextureHolder* t0 = e->noise0) bind_texture(0, &t0->tex, kNoiseAddress, 1);
    if (TextureHolder* t1 = e->noise1) bind_texture(1, &t1->tex, kNoiseAddress, 1);
    if (bloom) Draw_SetTexture0(tex, 2, 1);

    const f32 inv255 = k[3];
    const f32 tint = e->tint_scale;
    f32 rgba[4];
    for (u32 i = 0; i < 4; ++i) rgba[i] = static_cast<f32>(e->tint[i]) * inv255;
    const f32 m = e->mask_scale;
    const u64 tint_a = pack(bits(rgba[0] * tint), bits(rgba[1] * tint));
    const u64 tint_b = pack(bits(rgba[2] * tint), bits(e->tint_alpha * m * rgba[3]));
    const u64 mask_x = pack(bits(e->mask[4] * m), bits(e->mask[0] * m));
    const u64 mask_xb = pack(bits(e->mask[2] * s), 0);
    const u64 mask_y = pack(bits(e->mask[5] * m), bits(e->mask[1] * m));
    const u64 mask_yb = pack(bits(e->mask[3] * s), 0);
    if (has_fconst(kWeights)) {
        set_fconst(kWeights, pack(e->layers[0].weight, e->layers[1].weight), pack(e->layers[2].weight, bits(weight)));
    }
    set_fconst(kNoiseColorTint, tint_a, tint_b);
    set_fconst(kMaskParamsX, mask_x, mask_xb);
    set_fconst(kMaskParamsY, mask_y, mask_yb);

    be_f32* mtx = spf(272);  // f32[4][4] written by Matrix_FromAxisAngle
    be_u32* scale = sp(192);
    for (u32 i = 0; i < 3; ++i) {
        const NoiseLayer& layer = e->layers[i];
        scale[0] = layer.scale[0].get();  // {x, y, 0}
        scale[1] = layer.scale[1].get();
        scale[2] = 0u;
        const f32 ox = layer.scroll[0] * s, oy = layer.scroll[1] * s;
        call(Matrix_FromAxisAngle, mtx, kAxis, layer.angle.get());

        // The scroll through the rotation (VMX, per lane):
        // row0 * ox + row1 * oy + row2 * 0 + row3.
        f32 p[4];
        for (u32 j = 0; j < 4; ++j) p[j] = std::fma(mtx[j].get(), ox, 0.0f);
        for (u32 j = 0; j < 4; ++j) {
            p[j] = std::fma(mtx[4 + j].get(), oy, p[j]);
            p[j] = std::fma(mtx[8 + j].get(), 0.0f, p[j]);
            p[j] = mtx[12 + j] + p[j];
        }
        const f32 px = p[0], py = p[1];
        const f32 fx = px - static_cast<f32>(rt::fcfid(rt::fctid(px, true)));  // fractional parts
        const f32 fy = py - static_cast<f32>(rt::fcfid(rt::fctid(py, true)));
        call(Matrix_ScaleRows, mtx, scale);
        const u32 pz = bits(p[2]);
        *spf(304) = fx;  // row 2 becomes {fx, fy, pz}
        *spf(308) = fy;
        *sp(312) = pz;

        const u32 first = 3 * i;
        // Row 1 xyz and {fx, fy, pz}; the w words are the stack's (r1 +252 / +268).
        const u64 r1a = pack(*sp(288), *sp(292)), r1b = pack(*sp(296), *sp(252));
        const u64 r2a = pack(bits(fx), bits(fy)), r2b = pack(pz, *sp(268));
        if (has_fconst(first)) set_fconst(first, pack(*sp(272), *sp(276)), pack(*sp(280), *sp(236)));
        set_fconst(first + 1, r1a, r1b);
        set_fconst(first + 2, r2a, r2b);
    }
    draw_quad();

    // Add work 0 onto the scene target.
    set_target(scene_target(scene_colour()));
    bind(vp_copy, fp_copy);
    Draw_ResetState(false, 0);
    emit({kBlendEnable, 1});
    emit({kBlendEnable, 1});  // twice in the original
    emit({kBlendFunc, kOne2, kOneMinusSrcAlpha2});
    emit({kBlendEquation, kFuncAdd2});
    Draw_SetTexture0(screen, 0, 1);
    draw_quad();

    Draw_SetDepthTarget(display_depth());
    Draw_SetTarget(scene_target(scene_colour()), 0);
}

}  // namespace sly4::gfx

NATIVE(00540DD8, sly4::gfx::BloomNoise_Draw)
