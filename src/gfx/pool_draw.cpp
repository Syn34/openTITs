// Stencil shadow passes (StencilShadow vtable 0x00B241E8 slots 5, 7 and 8),
// the inline-array draw the volume pass uses and the depth-only draw of
// ZTranslucent's pre-pass.
#include "pool.h"
#include "rsx.h"

namespace sly4::gfx {

namespace {

namespace spg = sly4::render::pool::g;
constexpr u32 cellGcmSetDrawIndexArray = 0x005E4F64;   // (context, mode, count, type, location, offset)
constexpr u32 cellGcmSetDrawArrays = 0x005E4D98;       // (context, mode, first, count) (name?)

constexpr u32 kQuadConsts = 0x0054B6D8;  // f32 1/255
constexpr u32 kRef = 128, kQuadRef = 127;
constexpr u8 kTriangles = 5;

// The volume triangles: Draw_InlineArray of the caster's triangle count
// (+4) * 3 vertices.
void draw_volume(u32 count, const be_f32* buf, const VertexFormat* vf) { Draw_InlineArray(kTriangles, count, buf, vf); }
u32 volume_count(const ShadowCaster* o) { return 3u * o->tris; }

// Front func NOTEQUAL with ref 128 (sent with the old ref first).
void front_notequal() {
    rs().stencil_func = kGlNotEqual;
    front_func();
    rs().stencil_ref = kRef;
    front_func();
}
void back_notequal() {
    rs().back_func = kGlNotEqual;
    back_func();
    rs().back_ref = kRef;
    back_func();
}

}  // namespace

// void StencilShadow_Begin(StencilShadow* p)   (slot 5; name?)
// Clears the stencil buffer (to 0, colour 0, depth 0xFFFFFF) once when
// g::clear_stencil is set, then stencil test on with mask 255 (front and,
// when two-sided stencil is available, back), depth test as the render
// state has it with LESS and no depth writes, blending, alpha test and
// colour writes off, and the StencilShadow programs bound.
void StencilShadow_Begin(StencilShadow* p) {
    RenderState& st = rs();
    u8& clear = global<u8>(spg::clear_stencil);
    if (clear != 0) {
        clear = 0;
        call(fn::cellGcmSetClearColor, gcm(), 0u);
        call(fn::cellGcmSetClearDepthStencil, gcm(), 0xFFFFFF00u);
        call(fn::cellGcmSetStencilMask, gcm(), 255u);
        call(fn::cellGcmSetClearSurface, gcm(), 2u);  // stencil
        call(fn::cellGcmSetStencilMask, gcm(), st.stencil_mask.get());
    }
    emit({kStencilTestEnable, 1});
    st.stencil_func_mask = 255u;
    front_func();
    st.stencil_mask = 255u;
    emit({kStencilMask, 255});
    if (two_sided_stencil()) {
        st.back_func_mask = 255u;
        back_func();
        st.back_mask = 255u;
        emit({kBackStencilMask, 255});
    }
    emit({kDepthTestEnable, st.dirty_b[1]});
    emit({kDepthFunc, kGlLess});
    emit({kDepthMask, 0});
    emit({kBlendEnable, 0});
    emit({kAlphaTestEnable, 0});
    emit({kColorMask, 0});
    bind_vertex(p->volume_v);
    bind_fragment(p->volume_f);
}

// void StencilShadow_DrawVolume(StencilShadow* p, ShadowCaster* o, const f32 m[4][4], const f32* vertices)   (slot 7; name?)
// Uploads transpose(m * current view's +880 matrix) as vertex constants
// 0..3, then draws the volume into the stencil buffer (func NOTEQUAL, ref
// 128). With caps (+320) it is z-fail (depth-fail increments on back
// faces, decrements on front faces), else z-pass (depth-pass increments
// on front faces, decrements on back faces); in one pass with two-sided
// stencil, else in two culled passes (wrapping vs saturating counts).
void StencilShadow_DrawVolume(StencilShadow* p, ShadowCaster* o, const be_f32* matrix, const be_f32* buf) {
    const be_f32* m = aligned16(matrix);  // lvx
    View* view = current_view();
    const be_f32* vp = aligned16(at<u8>(view, 880));
    f32 r[4][4];
    for (u32 i = 0; i < 4; ++i) {
        const be_f32* row = m + 4 * i;
        const f32 x = row[0], y = row[1], z = row[2];
        for (u32 k = 0; k < 4; ++k) {
            f32 v = std::fmaf(vp[k], x, i == 3 ? static_cast<f32>(vp[12 + k]) : 0.0f);
            v = std::fmaf(vp[4 + k], y, v);
            r[i][k] = std::fmaf(vp[8 + k], z, v);
        }
    }
    RenderState& st = rs();
    {
        GuestFrame f(64);
        be_f32* t = f.as<be_f32>();
        for (u32 k = 0; k < 4; ++k)
            for (u32 i = 0; i < 4; ++i) t[4 * k + i] = r[i][k];
        call(fn::cellGcmSetVertexProgramConstants, gcm(), 0u, 16u, t);
    }
    const VertexFormat* vf = &p->format;
    if (p->caps != 0) {
        if (two_sided_stencil()) {
            emit({kTwoSidedStencil, 1});
            emit({kCullFaceEnable, 0});
            st.back_op[0] = kGlKeep;
            st.back_op[2] = kGlIncrWrap;
            back_op();
            back_notequal();
            st.back_op[1] = kGlKeep;
            back_op();
            st.stencil_op[0] = kGlKeep;
            st.stencil_op[2] = kGlDecrWrap;
            front_op();
            front_notequal();
            st.stencil_op[1] = kGlKeep;
            front_op();
            draw_volume(volume_count(o), buf, vf);
            emit({kTwoSidedStencil, 0});
            return;
        }
        emit({kCullFaceEnable, 1});
        emit({kFrontFace, kGlCw});
        st.stencil_op[0] = kGlKeep;
        st.stencil_op[2] = kGlIncr;
        front_op();
        front_notequal();
        st.stencil_op[1] = kGlKeep;
        front_op();
        draw_volume(volume_count(o), buf, vf);
        emit({kCullFaceEnable, 1});
        emit({kFrontFace, kGlCcw});
        st.stencil_op[0] = kGlKeep;
        st.stencil_op[2] = kGlDecr;
        front_op();
        draw_volume(volume_count(o), buf, vf);
        return;
    }
    const u32 count = volume_count(o);
    if (two_sided_stencil()) {
        emit({kTwoSidedStencil, 1});
        emit({kCullFaceEnable, 0});
        st.stencil_op[0] = kGlKeep;
        st.stencil_op[2] = kGlKeep;
        front_op();
        front_notequal();
        st.stencil_op[1] = kGlIncrWrap;
        front_op();
        st.back_op[0] = kGlKeep;
        st.back_op[2] = kGlKeep;
        back_op();
        back_notequal();
        st.back_op[1] = kGlDecrWrap;
        back_op();
        draw_volume(count, buf, vf);
        emit({kTwoSidedStencil, 0});
        return;
    }
    const u32 pass_op[2] = {kGlIncr, kGlDecr}, face[2] = {kGlCcw, kGlCw};
    for (u32 k = 0; k < 2; ++k) {
        emit({kCullFaceEnable, 1});
        emit({kFrontFace, face[k]});
        st.stencil_op[0] = kGlKeep;
        st.stencil_op[2] = kGlKeep;
        front_op();
        front_notequal();
        st.stencil_op[1] = pass_op[k];
        front_op();
        draw_volume(count, buf, vf);
    }
}

// void StencilShadow_DrawQuad(StencilShadow* p, s32 blend, const f32* colour)   (slot 8; name?)
// Shades where the stencil count is non-zero (LESS with ref 0, mask 127;
// stencil ops keep): vertex constant 0 is colour (or the pool's RGB /
// 255 * intensity, alpha 1.0), depth test off, all colour writes, no
// culling, blending by `blend` (0: src alpha / one, reverse subtract;
// 1: src alpha / one; 2: src alpha / one minus src alpha; else one /
// zero), the StencilShadow_DrawQuad programs and the pool's vertex format
// (+176, data at +16), then a 4-vertex draw (mode 6).
void StencilShadow_DrawQuad(StencilShadow* p, s32 blend, const be_f32* colour) {
    GuestFrame f(16);
    be_f32* rgba = f.as<be_f32>();
    {
        const f32 k = global<be_f32>(kQuadConsts), intensity = p->intensity;
        for (u32 i = 0; i < 3; ++i) rgba[i] = (static_cast<f32>(p->colour[i]) * k) * intensity;
        *reinterpret_cast<be_u32*>(rgba + 3) = 0x3F800000u;  // 1.0f
        if (colour)
            for (u32 i = 0; i < 4; ++i) copy_word(rgba + i, colour + i);
    }
    RenderState& st = rs();
    st.stencil_func_mask = kQuadRef;
    front_func();
    if (two_sided_stencil()) {
        st.back_func_mask = kQuadRef;
        back_func();
    }
    st.stencil_op[0] = kGlKeep;
    st.stencil_op[2] = kGlKeep;
    front_op();
    st.stencil_func = kGlLess;
    front_func();
    st.stencil_ref = 0u;
    front_func();
    st.stencil_op[1] = kGlKeep;
    front_op();
    u32 src = 1, dst = 0, equation = 0x8006;  // ONE, ZERO, FUNC_ADD
    if (blend == 0) {
        src = 0x0302, dst = 1, equation = 0x800B;  // SRC_ALPHA, ONE, FUNC_REVERSE_SUBTRACT
    } else if (blend == 1) {
        src = 0x0302, dst = 1;
    } else if (blend == 2) {
        src = 0x0302, dst = 0x0303;  // SRC_ALPHA, ONE_MINUS_SRC_ALPHA
    }
    emit({kDepthTestEnable, 0});
    emit({kColorMask, 0x01010101});
    emit({kCullFaceEnable, 0});
    emit({kBlendEnable, 1});
    emit({kBlendFunc, src | (src << 16), dst | (dst << 16)});
    emit({kBlendEquation, equation | (equation << 16)});
    bind_vertex(p->quad_v);
    bind_fragment(p->quad_f);
    call(fn::cellGcmSetVertexProgramConstants, gcm(), 0u, 4u, rgba);
    bind_format(&p->format, p->quad.where, true);
    upload_fragment();
    call(cellGcmSetDrawArrays, gcm(), 6u, 0u, 4u);
}

// void Draw_InlineArray(u8 mode, u32 count, const void* data, const VertexFormat* vf)   (name?)
// Uploads the fragment program, sends vf's attribute formats (inline
// data, offset 0) unless the render state's offset cache is already 0,
// then draws `count` vertices of vf's stride inline.
void Draw_InlineArray(u8 mode, u32 count, const void* data, const VertexFormat* vf) {
    RenderState& st = rs();
    upload_fragment();
    if (st.vbase_cache != 0) {
        const u32 stride = vf->count;
        st.vbase_cache = 0;
        send_attributes(vf, stride, vf->mask, 0, 0x80000000u);
    }
    const u32 words = (count * vf->count) >> 2;  // mullw, rlwinm 30,2,31
    call(fn::cellGcmSetDrawInlineArray, gcm(), static_cast<u32>(mode), words, data);
}

// void DrawItem_DrawDepth(DrawMesh* mesh, DrawItem* item)   (name?)
// The ZTranslucent pre-pass draw: the item's bone palette (item +12 ->
// +84: u8 count, u8 bone[]) as 3 vec4 per bone from item +60 into vertex
// constants 64..; the skinned depth draw when the mesh has +112 and
// g::skin_depth is set; otherwise the mesh's vertex format (+68, data at
// +28), the fragment program and an indexed draw (+64 count, +44 index
// buffer, +76 mode) or, without indices, an array draw of +40 vertices.
void DrawItem_DrawDepth(DrawMesh* mesh, DrawItem* item) {
    RenderState& st = rs();
    const u8* pal = or_zero_page(item->mesh.get())->palette;
    if (pal) {
        // The original copies to its 3248-byte frame at +112 with no bound
        // on the count (more than 64 bones would run past it); the whole
        // 255-bone extent is reserved here.
        GuestFrame f(48 * 255);
        u8* bones = f.as<u8>();
        u32 n = pal[0];
        if (n != 0) {
            u8* mats = item->bones;
            for (u32 i = n; i-- > 0;) {
                const u8* src = at<u8>(mats, 48 * pal[1 + i]);
                for (u32 k = 0; k < 48; k += 4) copy_word(bones + 48 * i + k, src + k);
            }
            n = pal[0];
        }
        call(fn::cellGcmSetVertexProgramConstants, gcm(), 64u, 12 * n, bones);
    }
    if (mesh->skin != 0 && global<u8>(spg::skin_depth) != 0) {
        DrawItem_DrawDepthSkinned(mesh, global<be_u32>(spg::skin_table), item, host<be_f32>(item->bones.ea),
                                  item->mat, 0x40000000u);
        return;
    }
    bind_format(mesh->format, mesh->vertex_word);
    const u32 indices = mesh->index_count, mode = mesh->mode & 0xFF;
    if (indices != 0) {
        st.index_buffer = guest(&mesh->index_buffer);
        upload_fragment();
        const IndexBuffer* ib = or_zero_page(host<IndexBuffer>(st.index_buffer));
        const u32 where = ib->where;
        call(cellGcmSetDrawIndexArray, gcm(), mode, indices, ib->type & 0xFF, where & 1, where >> 1);
        return;
    }
    const u32 count = mesh->vertex_count;
    upload_fragment();
    call(cellGcmSetDrawArrays, gcm(), mode, 0u, count);
}

}  // namespace sly4::gfx

NATIVE(0054B3D8, sly4::gfx::StencilShadow_Begin)
NATIVE(0054A998, sly4::gfx::StencilShadow_DrawVolume)
NATIVE(0054B6E0, sly4::gfx::StencilShadow_DrawQuad)
NATIVE(007A31CC, sly4::gfx::Draw_InlineArray)
NATIVE(00538FA0, sly4::gfx::DrawItem_DrawDepth)
