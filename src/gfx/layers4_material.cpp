// Material_Bind: the per-material render state DrawItem_Draw (and five
// other draws) sets when the bound material changes. Layout notes in
// layers4.h.
#include "layers4.h"
#include "rsx.h"

namespace sly4::gfx {

namespace {

namespace rg = sly4::render::g;
namespace l4g = sly4::render::layers4::g;

constexpr u32 kShaderBind = 8;  // ShaderPrograms vtable slot 8: Bind(shader, mat)

}  // namespace

// void Material_Bind(Material* mat, ShaderPrograms* shader)   (name?)
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
//     shader is null), then shader slot 8 Bind(shader, mat).
void Material_Bind(Material* mat, ShaderPrograms* shader) {
    RenderState& st = rs();

    if (MaterialAnim* anim = host<MaterialAnim>(mat->w128)) {
        const u32 stamp = global<be_u32>(l4g::anim_stamp);
        if (anim->stamp != stamp) {
            anim->stamp = stamp;
            const f32 step = global<be_f32>(l4g::anim_rate) * global<be_f32>(l4g::anim_scale);  // fmuls
            for (AnimTrack& t : anim->tracks) MaterialAnim_Update(&t, step);
        }
    }

    const u64 blend = *at<be_u64>(mat, 104);
    be_u64& cache_a = global<be_u64>(rg::state_cache_a);
    if (cache_a != blend) {
        cache_a = blend;
        const u32 w = mat->blend;
        if ((w >> 31) == 0) {
            emit({kBlendEnable, 0});
        } else {
            const u32 df = mat->blend_dst, eq = mat->blend_eq;
            emit({kBlendEnable, 1});
            const u32 sf = (w >> 15) & 0xFFFF;  // rlwinm 1,0,15 | rlwimi 17,16,31
            emit({kBlendFunc, (sf << 16) | sf, (df << 16) | df});
            emit({kBlendEquation, (eq << 16) | eq});
        }
    }

    const u64 depth = *at<be_u64>(mat, 112);
    be_u64& cache_b = global<be_u64>(rg::state_cache_b);
    if (cache_b != depth) {
        cache_b = depth;
        const u32 w = mat->blend2;
        emit({kDepthTestEnable, st.dirty_b[1]});
        emit({kDepthFunc, (w >> 15) & 0xFFFF});
        emit({kDepthMask, ((mat->blend2 >> 31) & st.dirty_b[1]) != 0 ? 1u : 0u});
        const u32 cull = mat->state;
        if (cull == 0) {
            emit({kCullFaceEnable, 0});
        } else {
            emit({kCullFaceEnable, 1});
            emit({kFrontFace, cull == kGlCw ? kGlCcw : kGlCw});
        }
    }

    for (u32 i = mat->b44; i-- > 0;) {
        MatTexture* t = host<MatTexture>(*at<be_u32>(mat, 20 + 4 * i));
        if (!t) continue;
        const u32 unit = i & 0xFF;
        Texture* tex = &t->unit;
        gp<Texture>* slot = at<gp<Texture>>(&st, 52 + 4 * i);
        if (slot->get() != tex) {
            *slot = tex;
            call(fn::cellGcmSetTexture, gcm(), unit, &tex->gcm);
            const u32 levels = tex->gcm.mipmap;
            call(fn::cellGcmSetTextureControl, gcm(), unit, 1u, 0u, ((levels << 8) - 256) & 0xFFFF, 0u);
        }
        const u32 filter = t->filter, mip = t->mip;  // table indexes (name?)
        call(fn::cellGcmSetTextureFilter, gcm(), unit, 0u, static_cast<u32>(*at<u8>(&st, 225 + 3 * mip + filter)),
             static_cast<u32>(*at<u8>(&st, 222 + filter)), 1u);
        const u32 wrap_s = t->wrap_s, wrap_t = t->wrap_t;
        emit({kTextureAddress0 + ((i << 5) & 0x1FE0), 0x00030000u | ((wrap_t & 0xFF) << 8) | (wrap_s & 0xFF)});
    }

    if (!shader) shader = host<ShaderPrograms>(*at<be_u32>(host<u8>(*at<be_u32>(host<u8>(mat->w16), 12)), 0));
    shader = or_zero_page(shader);  // (unchecked by the original)
    bind_vertex(shader->vprogram);
    bind_fragment(shader->fprogram);
    vcall(shader, kShaderBind, mat);
}

}  // namespace sly4::gfx

NATIVE(0079CD0C, sly4::gfx::Material_Bind)
