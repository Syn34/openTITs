// The screen warp: copies the colour target into the scene texture and
// draws the effect's mesh over it with PostProc_Warp.
#include "fx.h"

using namespace sly4;

namespace {

namespace dfn = sly4::draw::fn;
namespace ffn = sly4::fx::fn;
namespace fg = sly4::fx::g;
using namespace sly4::fx;  // helpers and method names (fx::g / fx::fn are spelt out)

constexpr u32 kStats = sly4::g::mem_stats;
constexpr u32 kConfig = sly4::g::config;
constexpr u32 vp_warp = 0x01151A04, fp_warp = 0x01151A08;  // PostProc_Warp

namespace wfn {
// Still recompiled.
constexpr u32 View_GetCurrent = 0x004FDBA8;      // () -> view; +60 f32 near, +64 far, +96 u16 x, y, w, h (name?)
constexpr u32 Mat4_Ortho = 0x0056A358;           // (f32[16]*, left, right, bottom, top, near, far) (name?)
constexpr u32 VertexFormat_Get = 0x00515750;     // (u32 kind) -> vertex format like draw::g::quad (name?)
constexpr u32 cellGcmSetDrawIndexArray = 0x005E4F64;  // (context, mode, count, type, location, offset) (name?)
}  // namespace wfn

constexpr u32 kVertexFormat0 = 0x00041740, kVertexOffset0 = 0x00041680;
constexpr u32 kTriangleStrip = 6;

}  // namespace

// void Warp_Draw(Warp* e)
// e: +5780 u32 current buffer, +5788 u32 index count, +5796 + 16 * i the
// vertex buffers (+4 RSX offset << 1 | location), +5828 index buffer
// (+4 offset << 1 | location, +16 index type).
// Binds PostProc_Warp with an orthographic projection of the current
// view (transposed into vertex constants 0..3), turns blending, depth,
// alpha test and culling off, copies colour target 0 into the scene
// texture and binds it to unit 0 (inlined Draw_SetTexture0, filter 1),
// then sets the vertex attributes of the current buffer (as
// Draw_PrepareQuad does for the quad), uploads the fragment program if
// needed and draws the indexed triangle strip.
DECOMP(0054522C, Warp_Draw) {
    const u32 e = arg<u32>(ctx, 0);
    const u32 st = fg::state;
    bind_vertex(ctx, vp_warp);
    bind_fragment(fp_warp);

    const u32 view = static_cast<u32>(call(ctx, wfn::View_GetCurrent));
    const u32 rect = view + 96;
    auto fl = [](u32 v) { return static_cast<f32>(static_cast<f64>(static_cast<s64>(v))); };
    const f32 x = fl(load<u16>(rect)), y = fl(load<u16>(rect + 2));
    const f32 w = fl(load<u16>(rect + 4)), h = fl(load<u16>(rect + 6));
    Stack frame(ctx, 128);  // +0 the matrix, +64 its transpose
    const u32 m = frame.addr(), mt = frame.addr() + 64;
    callf(ctx, wfn::Mat4_Ortho, {m},
          {static_cast<f64>(x), static_cast<f64>(w + x), static_cast<f64>(h + y), static_cast<f64>(y),
           static_cast<f64>(load_f32(view + 60)), static_cast<f64>(load_f32(view + 64))});
    for (u32 i = 0; i < 4; ++i)
        for (u32 j = 0; j < 4; ++j) store<u32>(mt + 16 * i + 4 * j, load<u32>(m + 16 * j + 4 * i));
    call(ctx, ffn::cellGcmSetVertexProgramConstants, {load<u32>(st), 0, 16, mt});

    emit({kBlendEnable, 0});
    emit({kDepthTestEnable, 0});
    emit({kDepthMask, 0});
    emit({kAlphaTestEnable, 0});
    emit({kCullFaceEnable, 0});

    // Copy colour target 0 into the scene texture and bind it (inlined
    // Draw_SetTexture0(tex, 0, 1)).
    const u32 tex = load<u32>(kStats + (field<u8>(kConfig, 34) != 0 ? 100 : 84));
    copy_colour_to(ctx, tex);
    bind_texture(ctx, 0, tex, 0x00030303 /* clamp */, 1);

    // Vertex attributes of the current buffer.
    const u32 buffer = e + 5796 + (load<u32>(e + 5780) << 4);
    const u32 fmt = static_cast<u32>(call(ctx, wfn::VertexFormat_Get, {1}));
    u32 cached = load<u32>(st + 8);
    const u32 vp = load<u32>(st + 20);
    const u32 count = load<u16>(fmt + 120);
    if (load<u32>(st + 16) != fmt) {
        cached = 0xFFFFFFFFu;
        store<u32>(st + 8, cached);
    }
    store<u32>(st + 16, fmt);
    const u32 raw = load<u32>(buffer + 4);
    const u32 base = raw >> 1;
    if (cached != base) {
        store<u32>(st + 8, base);
        const u32 want = load<u16>(fmt + 126) & load<u16>(vp + 10);
        if (load<u32>(fmt + 4) != 0) {
            const u32 count_bits = (count << 8) & 0xFF00;
            const u32 location = (raw & 1) << 31;
            u32 a = fmt;
            do {
                const u32 index = load<u32>(a) + ref<u8>(a + 9);
                const u32 shift = index & 63;  // slw
                const u32 bit = shift < 32 ? 1u << shift : 0;
                if (ref<u8>(a + 10) == 0 && (bit & want) != 0) {
                    const u32 format = load<u32>(a + 4);
                    const u32 reg = (index << 2) & 0x3FC;
                    emit({kVertexFormat0 + reg,
                          ((static_cast<u32>(static_cast<s32>(format) >> 8) << 4) & 0xFF0) | count_bits | (format & 0xFF),
                          kVertexOffset0 + reg, (ref<u8>(a + 8) + base) | location});
                }
                a += 12;
            } while (load<u32>(a + 4) != 0);
        }
        u32 stale = (load<u16>(st + 32) | want) ^ want;
        if (stale != 0) {
            for (s32 i = 15; i >= 0; --i) {
                const u32 bit = (1u << i) & 0xFFFF;
                if ((stale & bit) != 0) {
                    const u16 enabled = load<u16>(st + 32);
                    stale = (stale & ~bit) & 0xFFFF;
                    store<u16>(st + 32, static_cast<u16>(enabled & ~bit));
                    emit({kVertexFormat0 + ((static_cast<u32>(i) << 2) & 0x3FC), 2});  // attribute off
                }
                if (stale == 0) break;
            }
        }
        store<u16>(st + 32, static_cast<u16>(want));
    }

    const u32 ib = e + 5828;
    store<u32>(st + 4, ib);
    const u32 indices = load<u32>(e + 5788);
    const u32 dirty = load<u32>(st + 92);
    if ((dirty & 0x80000000u) != 0) {
        store<u32>(st + 92, dirty & 0x7FFFFFFF);
        call(ctx, dfn::cellGcmSetFragmentProgram, {load<u32>(st), load<u32>(st + 28) + 1136, 1});
        call(ctx, ffn::FProgram_Upload, {st + 28});
    } else {
        const u32 fp = load<u32>(st + 28);
        if (fp != 0 && load<u64>(fp + 16) != 0) call(ctx, ffn::FProgram_Upload, {st + 28});
    }
    const u32 ibo = load<u32>(st + 4);
    const u32 w4 = load<u32>(ibo + 4);
    ctx.gpr[3] = call(ctx, wfn::cellGcmSetDrawIndexArray,
                      {load<u32>(st), kTriangleStrip, indices, load<u32>(ibo + 16) & 0xFF, w4 & 1, w4 >> 1});
}
