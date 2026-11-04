// Stencil shadow passes (StencilShadow vtable 0x00B241E8 slots 5, 7 and 8),
// the inline-array draw the volume pass uses and the depth-only draw of
// ZTranslucent's pre-pass.
#include "pool.h"

#include "../view/view2.h"

using namespace sly4;

namespace {

namespace l4 = sly4::render::layers4;
namespace sp = sly4::render::pool;

using fx::emit;
using sp::lfs;
using sp::sgl;
using sp::st_set;

constexpr u32 kQuadConsts = 0x0054B6D8;  // f32 1/255
constexpr u32 kRef = 128, kQuadRef = 127;

// The volume triangles: Draw_InlineArray of the caster's triangle count
// (+4) * 3 vertices.
u64 draw_volume(PPUContext& ctx, u32 count, u32 buf, u32 vf) {
    return call(ctx, sp::fn::Draw_InlineArray, {sp::kTriangles, count, buf, vf});
}
u32 volume_count(u32 o) { return 3u * load<u16>(o + 4); }

// Front func NOTEQUAL with ref 128 (sent with the old ref first).
void front_notequal() {
    st_set(160, sp::kNotEqual);
    sp::front_func();
    st_set(164, kRef);
    sp::front_func();
}
void back_notequal() {
    st_set(188, sp::kNotEqual);
    sp::back_func();
    st_set(192, kRef);
    sp::back_func();
}

}  // namespace

// void StencilShadow_Begin(StencilShadow* p)   (slot 5; name?)
// Clears the stencil buffer (to 0, colour 0, depth 0xFFFFFF) once when
// g::clear_stencil is set, then stencil test on with mask 255 (front and,
// when two-sided stencil is available, back), depth test as the render
// state has it with LESS and no depth writes, blending, alpha test and
// colour writes off, and the StencilShadow programs bound.
DECOMP(0054B3D8, StencilShadow_Begin) {
    const u32 p = arg<u32>(ctx, 0);
    const u32 st = fx::g::state;
    if (ref<u8>(sp::g::clear_stencil) != 0) {
        store<u8>(sp::g::clear_stencil, 0);
        call(ctx, fx::fn::cellGcmSetClearColor, {load<u32>(st), 0});
        call(ctx, fx::fn::cellGcmSetClearDepthStencil, {load<u32>(st), 0xFFFFFF00u});
        call(ctx, fx::fn::cellGcmSetStencilMask, {load<u32>(st), 255});
        call(ctx, fx::fn::cellGcmSetClearSurface, {load<u32>(st), 2});  // stencil
        call(ctx, fx::fn::cellGcmSetStencilMask, {load<u32>(st), load<u32>(st + 168)});
    }
    emit({fx::kStencilTestEnable, 1});
    st_set(172, 255);
    sp::front_func();
    st_set(168, 255);
    emit({sp::kStencilMask, 255});
    if (sp::two_sided_stencil()) {
        st_set(200, 255);
        sp::back_func();
        st_set(196, 255);
        emit({sp::kBackStencilMask, 255});
    }
    emit({l4::kDepthTest, ref<u8>(st + 93)});
    emit({l4::kDepthFunc, sp::kLess});
    emit({fx::kDepthMask, 0});
    emit({l4::kBlendEnable, 0});
    emit({l4::kAlphaTest, 0});
    emit({l4::kColorMask, 0});
    fx::bind_vertex(ctx, p + sp::kVolumeVProgram);
    const u32 fp = load<u32>(p + sp::kVolumeFProgram);
    const bool bound = load<u32>(st + 28) == fp;
    fx::bind_fragment(p + sp::kVolumeFProgram);
    ctx.gpr[3] = bound ? fp : load<u32>(st + 92);  // left in r3
}

// void StencilShadow_DrawVolume(StencilShadow* p, Caster* o, const f32 m[4][4], const f32* vertices)   (slot 7; name?)
// Uploads transpose(m * current view's +880 matrix) as vertex constants
// 0..3, then draws the volume into the stencil buffer (func NOTEQUAL, ref
// 128). With caps (+320) it is z-fail (depth-fail increments on back
// faces, decrements on front faces), else z-pass (depth-pass increments
// on front faces, decrements on back faces); in one pass with two-sided
// stencil, else in two culled passes (wrapping vs saturating counts).
DECOMP(0054A998, StencilShadow_DrawVolume) {
    const u32 p = arg<u32>(ctx, 0), o = arg<u32>(ctx, 1), m = arg<u32>(ctx, 2) & ~15u, buf = arg<u32>(ctx, 3);
    const u32 view = static_cast<u32>(call(ctx, view::v2fn::View_GetCurrent));
    const u32 vp = (view + 880) & ~15u;
    f32 r[4][4];
    for (u32 i = 0; i < 4; ++i) {
        const u32 row = m + 16 * i;
        const f32 x = load<f32>(row), y = load<f32>(row + 4), z = load<f32>(row + 8);
        for (u32 k = 0; k < 4; ++k) {
            f32 v = std::fmaf(load<f32>(vp + 4 * k), x, i == 3 ? load<f32>(vp + 48 + 4 * k) : 0.0f);
            v = std::fmaf(load<f32>(vp + 16 + 4 * k), y, v);
            r[i][k] = std::fmaf(load<f32>(vp + 32 + 4 * k), z, v);
        }
    }
    const u32 st = fx::g::state;
    {
        Stack frame(ctx, 64);
        for (u32 k = 0; k < 4; ++k) {
            for (u32 i = 0; i < 4; ++i) store<f32>(frame.addr() + 16 * k + 4 * i, r[i][k]);
        }
        call(ctx, fx::fn::cellGcmSetVertexProgramConstants, {load<u32>(st), 0, 16, frame.addr()});
    }
    const u32 vf = p + sp::kFormat;
    if (ref<u8>(p + sp::kCaps) != 0) {
        if (sp::two_sided_stencil()) {
            emit({sp::kTwoSidedStencil, 1});
            emit({l4::kCullEnable, 0});
            st_set(204, sp::kKeep);
            st_set(212, sp::kIncrWrap);
            sp::back_op();
            back_notequal();
            st_set(208, sp::kKeep);
            sp::back_op();
            st_set(176, sp::kKeep);
            st_set(184, sp::kDecrWrap);
            sp::front_op();
            front_notequal();
            st_set(180, sp::kKeep);
            sp::front_op();
            draw_volume(ctx, volume_count(o), buf, vf);
            emit({sp::kTwoSidedStencil, 0});
            ctx.gpr[3] = load<u32>(st);
            return;
        }
        emit({l4::kCullEnable, 1});
        emit({l4::kFrontFace, l4::kCw});
        st_set(176, sp::kKeep);
        st_set(184, sp::kIncr);
        sp::front_op();
        front_notequal();
        st_set(180, sp::kKeep);
        sp::front_op();
        draw_volume(ctx, volume_count(o), buf, vf);
        emit({l4::kCullEnable, 1});
        emit({l4::kFrontFace, l4::kCcw});
        st_set(176, sp::kKeep);
        st_set(184, sp::kDecr);
        sp::front_op();
        ctx.gpr[3] = draw_volume(ctx, volume_count(o), buf, vf);
        return;
    }
    const u32 count = volume_count(o);
    if (sp::two_sided_stencil()) {
        emit({sp::kTwoSidedStencil, 1});
        emit({l4::kCullEnable, 0});
        st_set(176, sp::kKeep);
        st_set(184, sp::kKeep);
        sp::front_op();
        front_notequal();
        st_set(180, sp::kIncrWrap);
        sp::front_op();
        st_set(204, sp::kKeep);
        st_set(212, sp::kKeep);
        sp::back_op();
        back_notequal();
        st_set(208, sp::kDecrWrap);
        sp::back_op();
        draw_volume(ctx, count, buf, vf);
        emit({sp::kTwoSidedStencil, 0});
        ctx.gpr[3] = load<u32>(st);
        return;
    }
    const u32 pass_op[2] = {sp::kIncr, sp::kDecr}, face[2] = {l4::kCcw, l4::kCw};
    u64 r3 = 0;
    for (u32 k = 0; k < 2; ++k) {
        emit({l4::kCullEnable, 1});
        emit({l4::kFrontFace, face[k]});
        st_set(176, sp::kKeep);
        st_set(184, sp::kKeep);
        sp::front_op();
        front_notequal();
        st_set(180, pass_op[k]);
        sp::front_op();
        r3 = draw_volume(ctx, count, buf, vf);
    }
    ctx.gpr[3] = r3;
}

// void StencilShadow_DrawQuad(StencilShadow* p, s32 blend, const f32* colour)   (slot 8; name?)
// Shades where the stencil count is non-zero (LESS with ref 0, mask 127;
// stencil ops keep): vertex constant 0 is colour (or the pool's RGB /
// 255 * intensity, alpha 1.0), depth test off, all colour writes, no
// culling, blending by `blend` (0: src alpha / one, reverse subtract;
// 1: src alpha / one; 2: src alpha / one minus src alpha; else one /
// zero), the StencilShadow_DrawQuad programs and the pool's vertex format
// (+176, data at +16), then a 4-vertex draw (mode 6).
DECOMP(0054B6E0, StencilShadow_DrawQuad) {
    const u32 p = arg<u32>(ctx, 0);
    const s32 blend = arg<s32>(ctx, 1);
    const u32 colour = arg<u32>(ctx, 2);
    Stack frame(ctx, 16);
    const u32 rgba = frame.addr();
    {
        const u32 c = p + sp::kColour;
        const f64 k = lfs(kQuadConsts), intensity = lfs(p + sp::kIntensity);
        for (u32 i = 0; i < 3; ++i) {
            const f64 v = sgl(static_cast<f64>(ref<u8>(c + i)));
            sp::stfs(rgba + 4 * i, sgl(sgl(v * k) * intensity));
        }
        store<u32>(rgba + 12, 0x3F800000);  // 1.0f
        if (colour != 0) {
            for (u32 i = 0; i < 16; i += 4) sp::copy_word(rgba + i, colour + i);
        }
    }
    const u32 st = fx::g::state;
    st_set(172, kQuadRef);
    sp::front_func();
    if (sp::two_sided_stencil()) {
        st_set(200, kQuadRef);
        sp::back_func();
    }
    st_set(176, sp::kKeep);
    st_set(184, sp::kKeep);
    sp::front_op();
    st_set(160, sp::kLess);
    sp::front_func();
    st_set(164, 0);
    sp::front_func();
    st_set(180, sp::kKeep);
    sp::front_op();
    u32 src = 1, dst = 0, equation = 0x8006;  // ONE, ZERO, FUNC_ADD
    if (blend == 0) {
        src = 0x0302, dst = 1, equation = 0x800B;  // SRC_ALPHA, ONE, FUNC_REVERSE_SUBTRACT
    } else if (blend == 1) {
        src = 0x0302, dst = 1;
    } else if (blend == 2) {
        src = 0x0302, dst = 0x0303;  // SRC_ALPHA, ONE_MINUS_SRC_ALPHA
    }
    emit({l4::kDepthTest, 0});
    emit({l4::kColorMask, 0x01010101});
    emit({l4::kCullEnable, 0});
    emit({l4::kBlendEnable, 1});
    emit({l4::kBlendFunc2, src | (src << 16), dst | (dst << 16)});
    emit({l4::kBlendEquation, equation | (equation << 16)});
    fx::bind_vertex(ctx, p + sp::kQuadVProgram);
    fx::bind_fragment(p + sp::kQuadFProgram);
    call(ctx, fx::fn::cellGcmSetVertexProgramConstants, {load<u32>(st), 0, 4, rgba});
    sp::bind_format(p + sp::kFormat, load<u32>(p + 16), true);
    sp::upload_fragment(ctx);
    ctx.gpr[3] = call(ctx, sp::fn::cellGcmSetDrawArrays, {load<u32>(st), 6, 0, 4});
}

// void Draw_InlineArray(u8 mode, u32 count, const void* data, const VertexFormat* vf)   (name?)
// Uploads the fragment program, sends vf's attribute formats (inline
// data, offset 0) unless the render state's offset cache is already 0,
// then draws `count` vertices of vf's stride inline.
DECOMP(007A31CC, Draw_InlineArray) {
    const u64 mode = ctx.gpr[3], count = ctx.gpr[4], data = ctx.gpr[5];
    const u32 vf = arg<u32>(ctx, 3);
    const u32 st = fx::g::state;
    sp::upload_fragment(ctx);
    if (load<u32>(st + 8) != 0) {
        const u32 stride = load<u16>(vf + 120);
        store<u32>(st + 8, 0);
        sp::send_attributes(vf, stride, load<u16>(vf + 126), 0, 0x80000000u);
    }
    const u32 words = (static_cast<u32>(count) * load<u16>(vf + 120)) >> 2;  // mullw, rlwinm 30,2,31
    ctx.gpr[3] = call(ctx, draw::fn::cellGcmSetDrawInlineArray,
                      {load<u32>(st), static_cast<u32>(mode) & 0xFF, words, data});
}

// void DrawItem_DrawDepth(Mesh* mesh, DrawItem* item)   (name?)
// The ZTranslucent pre-pass draw: the item's bone palette (item +12 ->
// +84: u8 count, u8 bone[]) as 3 vec4 per bone from item +60 into vertex
// constants 64..; the skinned depth draw when the mesh has +112 and
// g::skin_depth is set; otherwise the mesh's vertex format (+68, data at
// +28), the fragment program and an indexed draw (+64 count, +44 index
// buffer, +76 mode) or, without indices, an array draw of +40 vertices.
DECOMP(00538FA0, DrawItem_DrawDepth) {
    const u32 mesh = arg<u32>(ctx, 0), item = arg<u32>(ctx, 1);
    const u32 st = fx::g::state;
    const u32 pal = load<u32>(load<u32>(item + 12) + 84);
    if (pal != 0) {
        // The original copies to its 3248-byte frame at +112 with no bound
        // on the count (more than 64 bones would run past it); the whole
        // 255-bone extent is reserved here.
        Stack frame(ctx, 48 * 255);
        const u32 bones = frame.addr();
        u32 n = ref<u8>(pal);
        if (n != 0) {
            const u32 mats = load<u32>(item + 60);
            for (u32 i = n; i-- > 0;) {
                const u32 src = mats + 48 * ref<u8>(pal + 1 + i);
                for (u32 k = 0; k < 48; k += 4) sp::copy_word(bones + 48 * i + k, src + k);
            }
            n = ref<u8>(pal);
        }
        call(ctx, fx::fn::cellGcmSetVertexProgramConstants, {load<u32>(st), 64, 12 * n, bones});
    }
    if (load<u32>(mesh + 112) != 0 && ref<u8>(sp::g::skin_depth) != 0) {
        ctx.gpr[3] = call(ctx, sp::fn::DrawItem_DrawDepthSkinned,
                          {mesh, load<u32>(sp::g::skin_table), item, load<u32>(item + 60), load<u32>(item + 28), 0x40000000u});
        return;
    }
    sp::bind_format(load<u32>(mesh + 68), load<u32>(mesh + 28));
    const u32 indices = load<u32>(mesh + 64), mode = load<u32>(mesh + 76) & 0xFF;
    if (indices != 0) {
        store<u32>(st + 4, mesh + 44);
        sp::upload_fragment(ctx);
        const u32 ib = load<u32>(st + 4);
        const u32 where = load<u32>(ib + 4);
        ctx.gpr[3] = call(ctx, sp::fn::cellGcmSetDrawIndexArray,
                          {load<u32>(st), mode, indices, load<u32>(ib + 16) & 0xFF, where & 1, where >> 1});
        return;
    }
    const u32 count = load<u32>(mesh + 40);
    sp::upload_fragment(ctx);
    ctx.gpr[3] = call(ctx, sp::fn::cellGcmSetDrawArrays, {load<u32>(st), mode, 0, count});
}
