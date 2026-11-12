// The screen warp: copies the colour target into the scene texture and
// draws the effect's mesh over it with PostProc_Warp.
#include "rsx.h"

namespace sly4::gfx {

namespace {

constexpr u32 vp_warp = 0x01151A04, fp_warp = 0x01151A08;  // PostProc_Warp
constexpr u32 Mat4_Ortho = 0x0056A358;                     // (f32[16]*, left, right, bottom, top, near, far) (name?)
constexpr u32 cellGcmSetDrawIndexArray = 0x005E4F64;       // (context, mode, count, type, location, offset) (name?)
constexpr u32 kTriangleStrip = 6;

struct GpuBuffer {
    be_u32 address;     // +0
    be_u32 offset_loc;  // +4: RSX offset << 1 | location
    pad<8> _8;
    be_u32 type;        // +16: index type (index buffer)
};

struct Warp {
    pad<5780> _0;
    be_u32 current;          // +5780: current vertex buffer
    pad<4> _5784;
    be_u32 index_count;      // +5788
    pad<4> _5792;
    u8 vertex_buffers[32];   // +5796: 16-byte records (+4 offset << 1 | location), indexed by current
    GpuBuffer index_buffer;  // +5828
};
static_assert(offsetof(Warp, current) == 5780 && offsetof(Warp, vertex_buffers) == 5796 && offsetof(Warp, index_buffer) == 5828);

}  // namespace

// void Warp_Draw(Warp* e)
// Binds PostProc_Warp with an orthographic projection of the current
// view (transposed into vertex constants 0..3), turns blending, depth,
// alpha test and culling off, copies colour target 0 into the scene
// texture and binds it to unit 0 (inlined Draw_SetTexture0, filter 1),
// then sets the vertex attributes of the current buffer (as
// Draw_PrepareQuad does for the quad), uploads the fragment program if
// needed and draws the indexed triangle strip.
void Warp_Draw(Warp* e) {
    RenderState& st = rs();
    bind_vertex(vprog(vp_warp));
    bind_fragment(fprog(fp_warp));

    View* view = or_zero_page(call<View*>(fn::View_GetCurrent));
    const f32 x = int_to_f32(view->rect[0]), y = int_to_f32(view->rect[1]);
    const f32 w = int_to_f32(view->rect[2]), h = int_to_f32(view->rect[3]);
    GuestFrame f(128);  // +0 the matrix, +64 its transpose
    be_u32* m = f.as<be_u32>(0);
    be_u32* mt = f.as<be_u32>(64);
    call(Mat4_Ortho, m, x, w + x, h + y, y, view->near_z.get(), view->far_z.get());
    for (u32 i = 0; i < 4; ++i)
        for (u32 j = 0; j < 4; ++j) mt[4 * i + j] = m[4 * j + i].get();
    call(fn::cellGcmSetVertexProgramConstants, gcm(), 0u, 16u, mt);

    emit({kBlendEnable, 0});
    emit({kDepthTestEnable, 0});
    emit({kDepthMask, 0});
    emit({kAlphaTestEnable, 0});
    emit({kCullFaceEnable, 0});

    // Copy colour target 0 into the scene texture and bind it (inlined
    // Draw_SetTexture0(tex, 0, 1)).
    Texture* tex = stats_texture(config_u8(34) != 0 ? 100 : 84);
    copy_colour_to(tex);
    bind_texture(0, tex, 0x00030303 /* clamp */, 1);

    // Vertex attributes of the current buffer.
    const GpuBuffer* buffer = at<GpuBuffer>(e, 5796 + (e->current << 4));
    const VertexFormat* fmt = VertexFormat_Get(true);
    u32 cached = static_cast<u32>(st.vbase_cache.get());
    const VProgram* vp = st.vprogram;
    const u32 count = fmt->count;
    if (st.vformat.get() != fmt) {
        cached = 0xFFFFFFFFu;
        st.vbase_cache = static_cast<s32>(cached);
    }
    st.vformat = const_cast<VertexFormat*>(fmt);
    const u32 raw = buffer->offset_loc;
    const u32 base = raw >> 1;
    if (cached != base) {
        st.vbase_cache = static_cast<s32>(base);
        const u32 want = fmt->mask & or_zero_page(vp)->attr_mask;
        if (fmt->attrs[0].format != 0) {
            const u32 count_bits = (count << 8) & 0xFF00;
            const u32 location = (raw & 1) << 31;
            const VertexAttr* a = fmt->attrs;
            do {
                const u32 index = a->index + a->index_add;
                const u32 shift = index & 63;  // slw
                const u32 bit = shift < 32 ? 1u << shift : 0;
                if (a->disabled == 0 && (bit & want) != 0) {
                    const u32 format = a->format;
                    const u32 reg = (index << 2) & 0x3FC;
                    emit({kVertexFormat0 + reg,
                          ((static_cast<u32>(static_cast<s32>(format) >> 8) << 4) & 0xFF0) | count_bits | (format & 0xFF),
                          kVertexOffset0 + reg, (a->offset + base) | location});
                }
                ++a;
            } while (a->format != 0);
        }
        u32 stale = (st.attr_mask | want) ^ want;
        if (stale != 0) {
            for (s32 i = 15; i >= 0; --i) {
                const u32 bit = (1u << i) & 0xFFFF;
                if ((stale & bit) != 0) {
                    const u16 enabled = st.attr_mask;
                    stale = (stale & ~bit) & 0xFFFF;
                    st.attr_mask = static_cast<u16>(enabled & ~bit);
                    emit({kVertexFormat0 + ((static_cast<u32>(i) << 2) & 0x3FC), 2});  // attribute off
                }
                if (stale == 0) break;
            }
        }
        st.attr_mask = static_cast<u16>(want);
    }

    st.index_buffer = guest(&e->index_buffer);
    const u32 indices = e->index_count;
    const u32 dirty = st.dirty;
    if ((dirty & 0x80000000u) != 0) {
        st.dirty = dirty & 0x7FFFFFFF;
        call(fn::cellGcmSetFragmentProgram, gcm(), &or_zero_page(st.fprogram.get())->program, 1u);
        FProgram_Upload(&st.fprogram);
    } else {
        FProgram* fp = st.fprogram;
        if (fp && fp->dirty != 0) FProgram_Upload(&st.fprogram);
    }
    const GpuBuffer* ibo = host<GpuBuffer>(st.index_buffer);
    const u32 w4 = ibo->offset_loc;
    call(cellGcmSetDrawIndexArray, gcm(), kTriangleStrip, indices, ibo->type & 0xFF, w4 & 1, w4 >> 1);
}

}  // namespace sly4::gfx

NATIVE(0054522C, sly4::gfx::Warp_Draw)
