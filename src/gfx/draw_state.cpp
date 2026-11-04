// Low-level draw helpers for the post effects. RSX commands are written
// straight into the context's command buffer (no space check), as the
// original does.
#include "draw_state.h"

#include <bit>
#include <initializer_list>

using namespace sly4;

namespace {

namespace dfn = sly4::draw::fn;
namespace dg = sly4::draw::g;

constexpr u32 kStats = g::mem_stats;
constexpr u32 kConfig = g::config;

void emit(std::initializer_list<u32> words) {
    const u32 c = load<u32>(dg::render_state);
    const u32 cur = load<u32>(c + 8);
    u32 i = 0;
    for (u32 w : words) store<u32>(cur + 4 * i++, w);
    store<u32>(c + 8, cur + 4 * i);
}

f32 load_f32(u32 a) { return std::bit_cast<f32>(load<u32>(a)); }

}  // namespace

// void Draw_SetTexture0(Target* t, u32 unit, u32 filter)
// Binds t to texture unit `unit` (re-sending it only when it changed),
// clamps all three coordinates and sets the filter from the render
// state's tables at +225 / +222 (index `filter`). t == 0 disables the unit.
DECOMP(00536AC0, Draw_SetTexture0) {
    const u32 t = arg<u32>(ctx, 0);
    const u32 unit = arg<u32>(ctx, 1), filter = arg<u32>(ctx, 2);
    const u32 st = dg::render_state;
    const u32 slot = st + 52 + static_cast<u32>(static_cast<s32>(unit) * 4);
    if (t == 0) {
        const u32 c = load<u32>(st);
        store<u32>(slot, 0);
        ctx.gpr[3] = call(ctx, dfn::cellGcmSetTextureControl, {c, unit & 0xFF, 0, 0, 0, 0});
        return;
    }
    if (load<u32>(slot) != t) {
        const u32 c = load<u32>(st);
        store<u32>(slot, t);
        call(ctx, dfn::cellGcmSetTexture, {c, unit & 0xFF, t + 16});
        const u32 levels = ref<u8>(t + 17);
        call(ctx, dfn::cellGcmSetTextureControl,
             {load<u32>(st), unit & 0xFF, 1, 0, ((levels << 8) - 256) & 0xFFFF, 0});
    }
    emit({0x00041A08 + ((unit << 5) & 0x1FE0), 0x00030303});  // TEXTURE_ADDRESS: clamp
    const u8 min = ref<u8>(st + 225 + filter), mag = ref<u8>(st + 222 + filter);
    ctx.gpr[3] = call(ctx, dfn::cellGcmSetTextureFilter, {load<u32>(st), unit & 0xFF, 0, min, mag, 1});
}

// void Draw_SetTarget(Target* t, u32 slot)
// Makes t colour target `slot` (nothing if unchanged or t == 0): fills the
// CellGcmSurface at render state +96, sets it, a full-target viewport
// (scale/offset w/2, -h/2) and the scissor.
DECOMP(005373A8, Draw_SetTarget) {
    const u32 t = arg<u32>(ctx, 0), slot = arg<u32>(ctx, 1);
    const u32 st = dg::render_state;
    const u32 sp = st + (slot << 2);
    ctx.gpr[3] = slot;
    if (load<u32>(sp + 36) == t) return;
    store<u32>(sp + 36, t);
    if (t == 0) return;
    ref<u8>(st + 96) = ref<u8>(t + 16);
    ref<u8>(st + 97) = static_cast<u8>(load<u32>(t + 20));
    const u32 w28 = load<u32>(t + 28);
    ref<u8>(st + 99) = 1;
    ref<u8>(st + 98) = static_cast<u8>(w28);
    ref<u8>(st + slot + 100) = static_cast<u8>(load<u32>(t + 4) & 1);
    store<u16>(st + 148, load<u16>(t + 36));
    store<u32>(sp + 104, load<u32>(t + 4) >> 1);
    store<u16>(st + 150, load<u16>(t + 38));
    store<u32>(sp + 120, load<u32>(t + 32));
    store<u16>(st + 152, load<u16>(t + 40));
    store<u16>(st + 154, load<u16>(t + 42));
    call(ctx, dfn::cellGcmSetSurface, {load<u32>(st), st + 96});

    const u32 w = load<u16>(t + 36), h = load<u16>(t + 38);
    const u32 vc = dg::viewport_consts;
    const f32 c0 = load_f32(vc), c1 = load_f32(vc + 4), c2 = load_f32(vc + 8), c3 = load_f32(vc + 12);
    const f32 fw = static_cast<f32>(static_cast<f64>(static_cast<s64>(w)));
    const f32 fh = static_cast<f32>(static_cast<f64>(static_cast<s64>(h)));
    Stack frame(ctx, 32);  // +0 scale[4], +16 offset[4]
    const u32 scale = frame.addr(), offset = frame.addr() + 16;
    store<u32>(scale, std::bit_cast<u32>(fw * c0));
    store<u32>(scale + 4, std::bit_cast<u32>(fh * c1));
    store<u32>(scale + 8, 0x3F800000);
    store<u32>(scale + 12, 0);
    store<u32>(offset, std::bit_cast<u32>(fw * c0));
    store<u32>(offset + 4, std::bit_cast<u32>(fh * c0));
    store<u32>(offset + 8, 0);
    store<u32>(offset + 12, 0);
    // r8 / r9 are the shadow slots of the two float arguments; the original
    // leaves the constant table address and 1.0f there.
    callf(ctx, dfn::cellGcmSetViewport, {load<u32>(st), 0, 0, w, h, vc, 0x3F800000, scale, offset},
          {static_cast<f64>(c3), static_cast<f64>(c2)});
    emit({0x000808C0, static_cast<u32>(load<u16>(t + 36)) << 16, static_cast<u32>(load<u16>(t + 38)) << 16});
    ctx.gpr[3] = load<u32>(st);
}

// void Draw_ResetState(bool no_blend, u32 vconst)
// Optionally blending off, then depth test, depth write, alpha test and
// culling off, and zeroes vertex program constant `vconst` (cellGcmSetVertexProgramConstants, 1 vec4).
DECOMP(005355D8, Draw_ResetState) {
    const bool no_blend = arg<u8>(ctx, 0) != 0;
    const u32 mask = arg<u32>(ctx, 1);
    if (no_blend) emit({0x00040310, 0});  // BLEND_ENABLE
    emit({0x00040A74, 0});                 // DEPTH_TEST_ENABLE
    emit({0x00040A70, 0});                 // DEPTH_MASK
    emit({0x00040304, 0});                 // ALPHA_TEST_ENABLE
    emit({0x0004183C, 0});                 // CULL_FACE_ENABLE
    Stack frame(ctx, 16);
    for (u32 i = 0; i < 16; i += 4) store<u32>(frame.addr() + i, 0);
    ctx.gpr[3] = call(ctx, dfn::cellGcmSetVertexProgramConstants, {load<u32>(dg::render_state), mask, 4, frame.addr()});
}

// void Draw_PrepareQuad()
// Uploads a changed fragment program (or its constants), re-sends the
// quad's vertex attribute formats when the vertex program changed
// (disabling attributes no longer used), then draws the quad (inline
// array, mode 9).
DECOMP(0053539C, Draw_PrepareQuad) {
    const u32 st = dg::render_state;
    const u32 w92 = load<u32>(st + 92);
    if (w92 & 0x80000000u) {
        store<u32>(st + 92, w92 & 0x7FFFFFFF);
        call(ctx, dfn::cellGcmSetFragmentProgram, {load<u32>(st), load<u32>(st + 28) + 1136, 1});
        call(ctx, dfn::FProgram_Upload, {st + 28});
    } else {
        const u32 fp = load<u32>(st + 28);
        if (fp != 0 && load<u64>(fp + 16) != 0) call(ctx, dfn::FProgram_Upload, {st + 28});
    }
    const u32 q = dg::quad;
    const u32 count = load<u16>(q + 120);
    if (load<u32>(st + 8) != 0) {
        store<u32>(st + 8, 0);
        const u32 want = load<u16>(q + 126);
        if (load<u32>(q + 4) != 0) {
            const u32 count_bits = (count << 8) & 0xFF00;
            u32 e = q;
            do {
                const u32 index = load<u32>(e) + ref<u8>(e + 9);
                const u32 shift = index & 63;
                const u32 bit = shift < 32 ? 1u << shift : 0;
                if (ref<u8>(e + 10) == 0 && (bit & want) != 0) {
                    const u32 format = load<u32>(e + 4);
                    const u32 reg = (index << 2) & 0x3FC;
                    const u32 offset = ref<u8>(e + 8);
                    emit({0x00041740 + reg, (((format >> 8) & 0xFF) << 4) | count_bits | (format & 0xFF),
                          0x00041680 + reg, offset | 0x80000000u});
                }
                e += 12;
            } while (load<u32>(e + 4) != 0);
        }
        u32 stale = (load<u16>(st + 32) | want) ^ want;
        if (stale != 0) {
            for (s32 i = 15; i >= 0; --i) {
                const u32 bit = (1u << i) & 0xFFFF;
                if (stale & bit) {
                    const u16 enabled = load<u16>(st + 32);
                    stale = (stale & ~bit) & 0xFFFF;
                    store<u16>(st + 32, static_cast<u16>(enabled & ~bit));
                    emit({0x00041740 + ((static_cast<u32>(i) << 2) & 0x3FC), 2});  // attribute off
                }
                if (stale == 0) break;
            }
        }
        store<u16>(st + 32, static_cast<u16>(want));
    }
    ctx.gpr[3] = call(ctx, dfn::cellGcmSetDrawInlineArray, {load<u32>(st), 9, load<u16>(q + 120), dg::quad_vertices});
}

// void Draw_RestoreDepth()
// Depth test back to render state +93 bit 0 (also sent raw), depth func
// 0x203 (LEQUAL), depth write from bit 0, culling on, front face 0x901.
DECOMP(00535294, Draw_RestoreDepth) {
    const u32 st = dg::render_state;
    emit({0x00040A74, (ref<u8>(st + 93) & 1) != 0 ? 1u : 0u});  // DEPTH_TEST_ENABLE
    emit({0x00040A74, ref<u8>(st + 93)});
    emit({0x00040A6C, 515});                                     // DEPTH_FUNC
    emit({0x00040A70, (ref<u8>(st + 93) & 1) != 0 ? 1u : 0u});  // DEPTH_MASK
    emit({0x0004183C, 1});                                       // CULL_FACE_ENABLE
    emit({0x00041834, 2305});                                    // FRONT_FACE
    ctx.gpr[3] = load<u32>(st);
}

// void Draw_BindScreenTargets(Effect* unused)
// Binds the surface being drawn (current_surface +512): the display via
// Draw_BindDisplay, else its depth and colour targets for the current
// eye / buffer set (the same choice Gfx_InitFrameSurfaces makes).
DECOMP(00513038, Draw_BindScreenTargets) {
    const u32 s = load<u32>(load<u32>(dg::current_surface) + 512);
    if (s == load<u32>(dg::display)) {
        ctx.gpr[3] = call(ctx, dfn::Draw_BindDisplay);
        return;
    }
    const u32 w52 = load<u32>(s + 52);
    const u32 k = (w52 >> 28) & 0xC;  // rlwinm 4,28,29: bits 30..31 * 4
    auto alt = [&] { return (ref<u8>(s + 148) >> 7) != 0; };
    auto stereo = [] { return field<u8>(kConfig, 35) != 0 && field<u8>(kConfig, 34) != 0; };
    u32 colour, depth;
    if (((w52 >> 25) & 1) == 0) {
        colour = s + k + 16;
        depth = s + 24;
    } else {
        if (!stereo()) colour = alt() ? s + 84 : s + k + 64;
        else colour = load<u32>(kStats + 184) == 1 ? s + k + 100 : s + k + 108;
        if (!stereo()) depth = alt() ? s + 88 : s + 72;
        else depth = load<u32>(kStats + 184) == 1 ? s + 120 : s + 124;
    }
    ctx.gpr[3] = call(ctx, dfn::Draw_BindTargets, {load<u32>(depth), load<u32>(colour), 0});
}
