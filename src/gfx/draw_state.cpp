// Low-level draw helpers for the post effects. RSX commands are written
// straight into the context's command buffer (no space check), as the
// original does.
#include "rsx.h"

namespace sly4::gfx {

namespace {
constexpr u32 kViewportConsts = 0x00537398;  // f32 { 0.5, -0.5, 1.0, 0.0 }
constexpr u32 kCurrentSurface = 0x010DDA74;  // object whose +512 is the surface being drawn (name?)
}  // namespace

// void Draw_SetTexture0(Target* t, u32 unit, u32 filter)
// Binds t to texture unit `unit` (re-sending it only when it changed),
// clamps all three coordinates and sets the filter from the render
// state's tables at +225 / +222 (index `filter`). t == 0 disables the unit.
void Draw_SetTexture0(Texture* t, u32 unit, u32 filter) {
    RenderState& st = rs();
    gp<Texture>* slot = at<gp<Texture>>(&st, 52 + static_cast<u32>(static_cast<s32>(unit) * 4));
    if (!t) {
        GcmContext* c = st.context;
        *slot = nullptr;
        call(fn::cellGcmSetTextureControl, c, unit & 0xFF, 0u, 0u, 0u, 0u);
        return;
    }
    if (slot->get() != t) {
        GcmContext* c = st.context;
        *slot = t;
        call(fn::cellGcmSetTexture, c, unit & 0xFF, &t->gcm);
        const u32 levels = t->gcm.mipmap;
        call(fn::cellGcmSetTextureControl, gcm(), unit & 0xFF, 1u, 0u, ((levels << 8) - 256) & 0xFFFF, 0u);
    }
    emit({kTextureAddress0 + ((unit << 5) & 0x1FE0), 0x00030303});  // clamp
    const u8 min = *at<u8>(&st, 225 + filter), mag = *at<u8>(&st, 222 + filter);
    call(fn::cellGcmSetTextureFilter, gcm(), unit & 0xFF, 0u, static_cast<u32>(min), static_cast<u32>(mag), 1u);
}

// void Draw_SetTarget(Target* t, u32 slot)
// Makes t colour target `slot` (nothing if unchanged or t == 0): fills the
// render state's CellGcmSurface, sets it, a full-target viewport
// (scale/offset w/2, -h/2) and the scissor.
void Draw_SetTarget(Target* t, u32 slot) {
    RenderState& st = rs();
    gp<Target>* colour = at<gp<Target>>(&st, 36 + (slot << 2));
    if (colour->get() == t) return;
    *colour = t;
    if (!t) return;
    GcmSurface& s = st.surface;
    s.type = t->surface_type;
    s.antialias = static_cast<u8>(t->aa_mode);
    const u32 format = t->surface_format;
    s.colour_target = 1;
    s.colour_format = static_cast<u8>(format);
    *at<u8>(&st, 100 + slot) = static_cast<u8>(t->offset_loc & 1);
    s.width = t->width.get();
    *at<be_u32>(&st, 104 + (slot << 2)) = t->offset_loc >> 1;
    s.height = t->height.get();
    *at<be_u32>(&st, 120 + (slot << 2)) = t->pitch.get();
    s.x = t->x.get();
    s.y = t->y.get();
    call(fn::cellGcmSetSurface, gcm(), &s);

    const u32 w = t->width, h = t->height;
    const be_f32* vc = host<be_f32>(kViewportConsts);
    const f32 c0 = vc[0], c1 = vc[1], c2 = vc[2], c3 = vc[3];
    const f32 fw = int_to_f32(w), fh = int_to_f32(h);
    GuestFrame f(32);
    be_f32* scale = f.as<be_f32>(0);
    be_f32* offset = f.as<be_f32>(16);
    scale[0] = fw * c0;
    scale[1] = fh * c1;
    scale[2] = 1.0f;
    scale[3] = 0.0f;
    offset[0] = fw * c0;
    offset[1] = fh * c0;
    offset[2] = 0.0f;
    offset[3] = 0.0f;
    call(fn::cellGcmSetViewport, gcm(), 0u, 0u, w, h, c3, c2, scale, offset);
    emit({kSetWindowOffset, static_cast<u32>(t->width) << 16, static_cast<u32>(t->height) << 16});
}

// void Draw_ResetState(bool no_blend, u32 vconst)
// Optionally blending off, then depth test, depth write, alpha test and
// culling off, and zeroes vertex program constant `vconst`.
void Draw_ResetState(bool no_blend, u32 vconst) {
    if (no_blend) emit({kBlendEnable, 0});
    emit({kDepthTestEnable, 0});
    emit({kDepthMask, 0});
    emit({kAlphaTestEnable, 0});
    emit({kCullFaceEnable, 0});
    GuestFrame f(16);
    be_u32* zero = f.as<be_u32>();
    for (u32 i = 0; i < 4; ++i) zero[i] = 0u;
    call(fn::cellGcmSetVertexProgramConstants, gcm(), vconst, 4u, zero);
}

// void Draw_PrepareQuad()
// Uploads a changed fragment program (or its constants), re-sends the
// quad's vertex attribute formats when the vertex program changed
// (disabling attributes no longer used), then draws the quad (inline
// array, mode 9).
void Draw_PrepareQuad() {
    RenderState& st = rs();
    const u32 dirty = st.dirty;
    if (dirty & 0x80000000u) {
        st.dirty = dirty & 0x7FFFFFFF;
        call(fn::cellGcmSetFragmentProgram, gcm(), &or_zero_page(st.fprogram.get())->program, 1u);
        FProgram_Upload(&st.fprogram);
    } else {
        FProgram* fp = st.fprogram;
        if (fp && fp->dirty != 0) FProgram_Upload(&st.fprogram);
    }
    VertexFormat& q = global<VertexFormat>(g::quad);
    const u32 count = q.count;
    if (st.vbase_cache != 0) {
        st.vbase_cache = 0;
        const u32 want = q.mask;
        if (q.attrs[0].format != 0) {
            const u32 count_bits = (count << 8) & 0xFF00;
            const VertexAttr* e = q.attrs;
            do {
                const u32 index = e->index + e->index_add;
                const u32 shift = index & 63;
                const u32 bit = shift < 32 ? 1u << shift : 0;
                if (e->disabled == 0 && (bit & want) != 0) {
                    const u32 format = e->format;
                    const u32 reg = (index << 2) & 0x3FC;
                    emit({0x00041740 + reg, (((format >> 8) & 0xFF) << 4) | count_bits | (format & 0xFF),
                          0x00041680 + reg, e->offset | 0x80000000u});
                }
                ++e;
            } while (e->format != 0);
        }
        u32 stale = (st.attr_mask | want) ^ want;
        if (stale != 0) {
            for (s32 i = 15; i >= 0; --i) {
                const u32 bit = (1u << i) & 0xFFFF;
                if (stale & bit) {
                    const u16 enabled = st.attr_mask;
                    stale = (stale & ~bit) & 0xFFFF;
                    st.attr_mask = static_cast<u16>(enabled & ~bit);
                    emit({0x00041740 + ((static_cast<u32>(i) << 2) & 0x3FC), 2});  // attribute off
                }
                if (stale == 0) break;
            }
        }
        st.attr_mask = static_cast<u16>(want);
    }
    call(fn::cellGcmSetDrawInlineArray, gcm(), 9u, static_cast<u32>(q.count), g::quad_vertices);
}

// void Draw_RestoreDepth()
// Depth test back to render state +93 bit 0 (also sent raw), depth func
// 0x203 (LEQUAL), depth write from bit 0, culling on, front face 0x901.
void Draw_RestoreDepth() {
    RenderState& st = rs();
    emit({kDepthTestEnable, (st.dirty_b[1] & 1) != 0 ? 1u : 0u});
    emit({kDepthTestEnable, st.dirty_b[1]});
    emit({kDepthFunc, 515});
    emit({kDepthMask, (st.dirty_b[1] & 1) != 0 ? 1u : 0u});
    emit({kCullFaceEnable, 1});
    emit({kFrontFace, 2305});
}

// void Draw_BindScreenTargets(Effect* unused)
// Binds the surface being drawn (current_surface +512): the display via
// Draw_BindDisplay, else its depth and colour targets for the current
// eye / buffer set (the same choice Gfx_InitFrameSurfaces makes).
void Draw_BindScreenTargets(void*) {
    Display* s = *at<gp<Display>>(global<gp<u8>>(kCurrentSurface).get(), 512);
    if (s == display()) {
        Draw_BindDisplay();
        return;
    }
    const u32 k = eye_offset(s);
    auto stereo = [] { return config_u8(35) != 0 && config_u8(34) != 0; };
    gp<Target>* colour;
    gp<Target>* depth;
    if (!per_eye(s)) {
        colour = at<gp<Target>>(s, k + 16);
        depth = &s->depth;
    } else {
        if (!stereo()) colour = alternate(s) ? &s->alt_colour : at<gp<Target>>(s, k + 64);
        else colour = at<gp<Target>>(s, k + (mem_stats().buffer_select == 1 ? 100 : 108));
        if (!stereo()) depth = alternate(s) ? &s->alt_depth : &s->eye_depth;
        else depth = &s->stereo_depth[mem_stats().buffer_select == 1 ? 0 : 1];
    }
    Draw_BindTargets(*depth, *colour, 0);
}

}  // namespace sly4::gfx

NATIVE(00536AC0, sly4::gfx::Draw_SetTexture0)
NATIVE(005373A8, sly4::gfx::Draw_SetTarget)
NATIVE(005355D8, sly4::gfx::Draw_ResetState)
NATIVE(0053539C, sly4::gfx::Draw_PrepareQuad)
NATIVE(00535294, sly4::gfx::Draw_RestoreDepth)
NATIVE(00513038, sly4::gfx::Draw_BindScreenTargets)
