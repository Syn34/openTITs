// Per-frame post-process effect draws (fx_*.cpp) and the render-state
// helpers they inline.
//
// Render state (post::g::gcm_state, 0x010F10A0), as used here beyond
// effect_draw.h / draw_state.h: +84 Target* depth target, +136 u8 depth
// surface format (2 when none), +137 u8 depth location, +140 depth offset,
// +144 depth pitch (64 when none), +156 u32 frame stamp, +168 u32 stencil
// mask to restore.
//
// Fragment constants: FProgram +592 + 16 * k is parameter record k
// {CgParameter (0 = unused), resource, binding entry, constant index}.
// Writing record k's vec4 marks its constant dirty (FProgram +16) when the
// value changed or the program's stamp (+24) isn't this frame's.
#pragma once

#include <bit>
#include <initializer_list>

#include "draw_state.h"
#include "effect_draw.h"

namespace sly4::fx {

namespace fn {
constexpr u32 Draw_BindDisplay = 0x0053585C;     // decompiled: src/gfx/fx_draw.cpp
constexpr u32 Draw_BindTargets = 0x00532090;     // decompiled: src/gfx/fx_draw.cpp
constexpr u32 FProgram_Upload = 0x0052F4F0;      // decompiled: src/gfx/fx_draw.cpp
constexpr u32 Draw_SetTexture0Call = 0x0053C7EC; // decompiled: src/gfx/fx_draw.cpp
constexpr u32 Draw_ResetStateCall = 0x0053C80C;  // decompiled: src/gfx/fx_draw.cpp
constexpr u32 PostProc_ResolveMsaa = 0x00534F80; // decompiled: src/gfx/fx_draw.cpp
// Still recompiled: game code.
constexpr u32 Display_GetTargets = 0x005356F4;   // (Target*** colour, Target*** depth) leaf (name?)
constexpr u32 memcpy = 0x00732BCC;               // (dst, src, n) (name?)
// Still recompiled: libgcm (inlined command writers).
constexpr u32 cellGcmSetVertexProgramConstants = 0x005E5CB0;  // (context, first, count, f32*) method 0x1EFC (name?)
// draw_state.h calls 0x005E5CB0 cellGcmSetColorMask; it writes vertex
// constants (NV4097_SET_TRANSFORM_CONSTANT_LOAD).
constexpr u32 cellGcmSetFragmentProgramOffset = 0x005E073C;  // (context, offset, location) method 0x8E4 (name?)
constexpr u32 cellGcmSetInvalidateTextureCache = 0x005E3E64;  // (context, 1) method 0x1FD8 (name?)
constexpr u32 cellGcmSetClearColor = 0x005E3CB4;         // (context, colour) method 0x1D90 (name?)
constexpr u32 cellGcmSetClearDepthStencil = 0x005E3CD8;  // (context, value) method 0x1D8C (name?)
constexpr u32 cellGcmSetStencilMask = 0x005E3D8C;        // (context, mask) method 0x32C (name?)
constexpr u32 cellGcmSetClearSurface = 0x005E3C74;       // (context, mask) method 0x1D94 (name?)
}  // namespace fn

namespace imp {
constexpr u32 cellGcmAddressToOffset = 0x00ABD5E4;
}  // namespace imp

namespace g {
constexpr u32 state = 0x010F10A0;           // render state (post::g::gcm_state)
constexpr u32 local_pool = 0x010F1338;      // +4 u32 buffer index, +8.. u32 base per buffer, +16 u32 offset (grows down)
constexpr u32 zero_vconsts = 0x01164BC0;    // f32[4] for vertex constant 0
// Screen-sized work targets (Target*), 0x01164040 +4 * i.
constexpr u32 work0 = 0x01164040, work1 = 0x01164044, work2 = 0x01164048, work3 = 0x0116404C;
// Program globals (VProgram* / FProgram*), named after the shader.
constexpr u32 vp_copy = 0x01164080, fp_copy = 0x01164084;              // PostProc_Copy
constexpr u32 vp_render_txr = 0x01164090, fp_render_txr = 0x01164094;  // PostProc_RenderTxr
constexpr u32 vp_down4x = 0x011640A0, fp_down4x = 0x011640A4;          // PostProc_DownSample4x
constexpr u32 vp_blur11h = 0x011640B0, fp_blur11h = 0x011640B4;        // PostProc_GuassianBlur11H
constexpr u32 vp_blur11v = 0x011640B8, fp_blur11v = 0x011640BC;        // PostProc_GuassianBlur11V
constexpr u32 vp_down_depth = 0x011640DC, fp_down_depth = 0x011640E0;  // PostProc_DownSampleDepth
constexpr u32 vp_copy_scale = 0x01164110, fp_copy_scale = 0x01164114;  // PostProc_CopyScale
constexpr u32 vp_bloom_apply = 0x01164120, fp_bloom_apply = 0x01164124;  // PostProc_BloomApply
constexpr u32 vp_god_rays = 0x010F0B40, fp_god_rays = 0x010F0B44;      // PostProc_GodRaysScatter
}  // namespace g

// RSX methods written inline (one-word NV4097 commands, count 1 << 18).
enum : u32 {
    kAlphaTestEnable = 0x00040304,
    kBlendEnable = 0x00040310,
    kBlendFunc = 0x00080314,  // two words: sfactor rgb|a, dfactor rgb|a
    kBlendEquation = 0x00040320,
    kColorMask = 0x00040324,
    kStencilTestEnable = 0x00040328,
    kDepthFunc = 0x00040A6C,
    kDepthMask = 0x00040A70,
    kDepthTestEnable = 0x00040A74,
    kCullFaceEnable = 0x0004183C,
};
constexpr u32 kSrcAlpha2 = 0x03020302, kOneMinusSrcAlpha2 = 0x03030303, kOne2 = 0x00010001;
constexpr u32 kFuncAdd2 = 0x80068006;

inline f32 load_f32(u32 a) { return std::bit_cast<f32>(load<u32>(a)); }
inline u32 bits(f32 v) { return std::bit_cast<u32>(v); }

using effect::bind_fragment;
using effect::bind_vertex;
using effect::emit;

// Writes {a, b} (16 bytes, big endian) into fragment parameter record k of
// the current fragment program, if the program uses it.
inline void set_fconst(u32 k, u64 a, u64 b) {
    const u32 st = g::state;
    const u32 fp = load<u32>(st + 28);
    const u32 rec = fp + 592 + 16 * k;
    if (load<u32>(rec) == 0) return;
    const u32 index = load<u32>(rec + 12);
    const u32 dst = fp + 48 + (index << 4);
    const u64 old_a = load<u64>(dst), old_b = load<u64>(dst + 8);
    store<u64>(dst, a);
    store<u64>(dst + 8, b);
    const bool changed = ((old_a ^ a) | (old_b ^ b)) != 0 || load<u32>(fp + 24) != load<u32>(st + 156);
    const u32 shift = index & 0x7F;  // sld
    const u64 bit = shift < 64 ? u64{1} << shift : 0;
    store<u64>(fp + 16, load<u64>(fp + 16) | (changed ? bit : 0));
}
inline void set_fconst(u32 k, u32 w0, u32 w1, u32 w2, u32 w3) {
    set_fconst(k, (u64{w0} << 32) | w1, (u64{w2} << 32) | w3);
}
inline void set_fconst_from(u32 k, u32 src) { set_fconst(k, load<u64>(src), load<u64>(src + 8)); }

// Blending on with the given factor pair, FUNC_ADD.
inline void blend_on(u32 sfactor, u32 dfactor) {
    emit({kBlendEnable, 1});
    emit({kBlendFunc, sfactor, dfactor});
    emit({kBlendEquation, kFuncAdd2});
}

// Makes d the depth target (0 = none): the depth half of the surface at
// render state +96, cellGcmSetSurface, then depth test / write on iff d.
inline void set_depth_target(PPUContext& ctx, u32 d) {
    const u32 st = g::state;
    store<u32>(st + 84, d);
    u32 offset = 0, pitch = 64;
    if (d != 0) {
        ref<u8>(st + 136) = static_cast<u8>(load<u32>(d + 28));
        ref<u8>(st + 137) = static_cast<u8>(load<u32>(d + 4) & 1);
        offset = load<u32>(d + 4) >> 1;
        pitch = load<u32>(d + 32);
    } else {
        ref<u8>(st + 137) = 0;
        ref<u8>(st + 136) = 2;
    }
    store<u32>(st + 140, offset);
    store<u32>(st + 144, pitch);
    call(ctx, draw::fn::cellGcmSetSurface, {load<u32>(st), st + 96});
    const u32 on = d != 0 ? 1u : 0u;
    ref<u8>(st + 93) = static_cast<u8>(on);
    emit({kDepthTestEnable, on});
    emit({kDepthMask, on});
}

inline void invalidate_textures(PPUContext& ctx) {
    call(ctx, fn::cellGcmSetInvalidateTextureCache, {load<u32>(g::state), 1});
}

// Draw_PrepareQuad + Draw_RestoreDepth; returns the r3 left behind.
inline u64 draw_quad(PPUContext& ctx) {
    call(ctx, draw::fn::Draw_PrepareQuad);
    return call(ctx, draw::fn::Draw_RestoreDepth);
}

// Binds texture `tex` (Target-like: +16 CellGcmTexture, +17 mipmap
// count) to `unit` as Draw_SetTexture0 does, with TEXTURE_ADDRESS
// `address` and the filter pair `filter` from the render state tables.
inline void bind_texture(PPUContext& ctx, u32 unit, u32 tex, u32 address, u32 filter) {
    const u32 st = g::state;
    const u32 slot = st + 52 + 4 * unit;
    if (load<u32>(slot) != tex) {
        store<u32>(slot, tex);
        call(ctx, draw::fn::cellGcmSetTexture, {load<u32>(st), unit, tex + 16});
        const u32 levels = ref<u8>(tex + 17);
        call(ctx, draw::fn::cellGcmSetTextureControl, {load<u32>(st), unit, 1, 0, ((levels << 8) - 256) & 0xFFFF, 0});
    }
    emit({0x00041A08 + (unit << 5), address});  // TEXTURE_ADDRESS
    call(ctx, draw::fn::cellGcmSetTextureFilter,
         {load<u32>(st), unit, 0, ref<u8>(st + 225 + filter), ref<u8>(st + 222 + filter), 1});
}

// Copies colour target 0 (from its first visible row, +42) into texture
// `tex` (+32 pitch, +36 RSX offset, +44 format index) with
// cellGcmSetTransferImage (local to local).
inline void copy_colour_to(PPUContext& ctx, u32 tex) {
    constexpr u32 kFormatTable = 0x00F67254;  // 24-byte entries, +8 bits per pixel (tgt::g::format_table)
    constexpr u32 cellGcmSetTransferImage = 0x005E47AC;  // (context, mode, dst offset, dst pitch, dst x, y,
                                                         //  src offset, src pitch, src x, y, w, h, bytes per pixel)
    const u32 st = g::state;
    const u32 t = load<u32>(st + 36);
    const u32 src_offset = (load<u32>(t + 4) >> 1) + load<u32>(t + 32) * load<u16>(t + 42);
    const s32 bpp = static_cast<s32>(load<u32>(kFormatTable + 24 * load<u32>(tex + 44) + 8));
    const u32 bytes = static_cast<u32>(bpp / 8) & 0xFF;  // srawi + addze
    call(ctx, cellGcmSetTransferImage,
         {load<u32>(st), 0, load<u32>(tex + 36), load<u32>(tex + 32), 0, 0, src_offset, load<u32>(t + 32), 0, 0,
          load<u16>(t + 36), load<u16>(t + 38), bytes});
}

// The scene colour texture for the current eye / buffer set (from the
// display, g::mem_stats +92.. / +112.. / +120.., or +128 for the
// alternate set). Its +40 is the matching render target.
inline u32 scene_colour() {
    constexpr u32 kStats = sly4::g::mem_stats, kConfig = sly4::g::config;
    const u32 disp = load<u32>(draw::g::display);
    if ((ref<u8>(disp + 148) >> 7) != 0) return load<u32>(kStats + 128);
    const u32 k = ((load<u32>(disp + 52) >> 28) & 3) << 2;
    if (field<u8>(kConfig, 34) != 0) return load<u32>(kStats + k + (load<u32>(kStats + 184) == 1 ? 112 : 120));
    return load<u32>(kStats + k + 92);
}

// The display's depth target for the current eye / buffer set (the
// choice Draw_BindScreenTargets makes).
inline u32 display_depth() {
    constexpr u32 kStats = sly4::g::mem_stats, kConfig = sly4::g::config;
    const u32 disp = load<u32>(draw::g::display);
    if (((load<u32>(disp + 52) >> 25) & 1) == 0) return load<u32>(disp + 24);
    if (field<u8>(kConfig, 35) == 0 || field<u8>(kConfig, 34) == 0)
        return load<u32>(disp + ((ref<u8>(disp + 148) >> 7) != 0 ? 88 : 72));
    return load<u32>(disp + (load<u32>(kStats + 184) == 1 ? 120 : 124));
}

// Virtual call through slot `slot` of obj's vtable; like the original
// sequence, the OPD and its entry address are left in the two registers
// after the arguments.
inline u64 vcall(PPUContext& ctx, u32 obj, u32 slot, std::initializer_list<u64> args) {
    const u32 opd = load<u32>(load<u32>(obj) + slot);
    u64 regs[8] = {};
    u32 n = 0;
    for (u64 a : args) regs[n++] = a;
    regs[n++] = opd;
    regs[n++] = load<u32>(opd);
    switch (n) {
    case 3: return call_ptr(ctx, opd, {regs[0], regs[1], regs[2]});
    case 4: return call_ptr(ctx, opd, {regs[0], regs[1], regs[2], regs[3]});
    case 5: return call_ptr(ctx, opd, {regs[0], regs[1], regs[2], regs[3], regs[4]});
    default: return call_ptr(ctx, opd, {regs[0], regs[1], regs[2], regs[3], regs[4], regs[5]});
    }
}

}  // namespace sly4::fx
