// Material_Bind: the per-material render state DrawItem_Draw (and five
// other draws) sets when the bound material changes. Layout notes in
// layers4.h.
#include "layers4.h"

using namespace sly4;

namespace {

namespace rg = sly4::render::g;
namespace l4 = sly4::render::layers4;

using fx::emit;

constexpr u32 kAnim = 128, kTextures = 20, kTextureCount = 44, kBlend = 104, kDepth = 112;
constexpr u32 kShaderBind = 8;  // Shader vtable slot 8: Bind(shader, mat)

}  // namespace

// u32 Material_Bind(AglMaterial* mat, Shader* shader)   (name?)
//  1. Once per l4::g::anim_stamp: steps the three animation tracks of the
//     block at +128 (if any) by anim_rate * anim_scale.
//  2. When the blend pair (+104) differs from render::g::state_cache_a:
//     blending off, or on with factors / equation from it.
//  3. When the depth / cull pair (+112) differs from state_cache_b: depth
//     test as the render state has it, the depth func, depth write iff
//     both the pair's bit and the state's depth bit, culling.
//  4. Textures n-1 .. 0 (null ones skipped): bound to unit i when the unit
//     holds another, then filter and wrap modes.
//  5. The shader's programs (default: the picker's +12 table [0] when
//     shader is null), then shader slot 8 Bind(shader, mat); returns its r3.
DECOMP(0079CD0C, Material_Bind) {
    const u32 mat = arg<u32>(ctx, 0);
    u32 shader = arg<u32>(ctx, 1);
    const u32 st = fx::g::state;

    if (const u32 anim = load<u32>(mat + kAnim); anim != 0) {
        const u32 stamp = load<u32>(l4::g::anim_stamp);
        if (load<u32>(anim) != stamp) {
            store<u32>(anim, stamp);
            const f32 step = static_cast<f32>(static_cast<f64>(load<f32>(l4::g::anim_rate)) *
                                              static_cast<f64>(load<f32>(l4::g::anim_scale)));  // fmuls
            for (const u32 track : {8u, 56u, 104u}) callf(ctx, l4::fn::MaterialAnim_Update, {anim + track}, {step});
        }
    }

    const u32 blend = mat + kBlend;
    if (const u64 pair = load<u64>(blend); load<u64>(rg::state_cache_a) != pair) {
        store<u64>(rg::state_cache_a, pair);
        const u32 w = load<u32>(blend);
        if ((w >> 31) == 0) {
            emit({l4::kBlendEnable, 0});
        } else {
            const u32 df = load<u16>(blend + 6), eq = load<u16>(blend + 4);
            emit({l4::kBlendEnable, 1});
            const u32 sf = (w >> 15) & 0xFFFF;  // rlwinm 1,0,15 | rlwimi 17,16,31
            emit({l4::kBlendFunc2, (sf << 16) | sf, (df << 16) | df});
            emit({l4::kBlendEquation, (eq << 16) | eq});
        }
    }

    const u32 depth = mat + kDepth;
    if (const u64 pair = load<u64>(depth); load<u64>(rg::state_cache_b) != pair) {
        store<u64>(rg::state_cache_b, pair);
        const u32 w = load<u32>(depth);
        emit({l4::kDepthTest, ref<u8>(st + 93)});
        emit({l4::kDepthFunc, (w >> 15) & 0xFFFF});
        emit({l4::kDepthMask, ((load<u32>(depth) >> 31) & ref<u8>(st + 93)) != 0 ? 1u : 0u});
        const u32 cull = load<u16>(depth + 4);
        if (cull == 0) {
            emit({l4::kCullEnable, 0});
        } else {
            emit({l4::kCullEnable, 1});
            emit({l4::kFrontFace, cull == l4::kCw ? l4::kCcw : l4::kCw});
        }
    }

    for (u32 i = ref<u8>(mat + kTextureCount); i-- > 0;) {
        const u32 t = load<u32>(mat + kTextures + 4 * i);
        if (t == 0) continue;
        const u32 unit = i & 0xFF;
        const u32 tex = t + 56, slot = st + 52 + 4 * i;
        if (load<u32>(slot) != tex) {
            store<u32>(slot, tex);
            call(ctx, draw::fn::cellGcmSetTexture, {load<u32>(st), unit, t + 72});
            const u32 levels = ref<u8>(t + 73);
            call(ctx, draw::fn::cellGcmSetTextureControl,
                 {load<u32>(st), unit, 1, 0, ((levels << 8) - 256) & 0xFFFF, 0});
        }
        const u32 filter = load<u16>(t + 118), mip = load<u16>(t + 120);  // table indexes (name?)
        call(ctx, draw::fn::cellGcmSetTextureFilter,
             {load<u32>(st), unit, 0, ref<u8>(st + 225 + 3 * mip + filter), ref<u8>(st + 222 + filter), 1});
        const u32 wrap_s = load<u16>(t + 122), wrap_t = load<u16>(t + 124);
        emit({l4::kTexAddress + ((i << 5) & 0x1FE0), 0x00030000u | ((wrap_t & 0xFF) << 8) | (wrap_s & 0xFF)});
    }

    if (shader == 0) shader = load<u32>(load<u32>(load<u32>(mat + 16) + 12));
    fx::bind_vertex(ctx, shader + 4);
    fx::bind_fragment(shader + 8);
    ctx.gpr[3] = fx::vcall(ctx, shader, kShaderBind, {shader, mat});
}
