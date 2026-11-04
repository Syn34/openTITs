// Per-frame post-effect draws and the program binding they inline.
//
// Render state at post::g::gcm_state (0x010F10A0): +0 CellGcmContextData*
// (+8 current command pointer), +8 s32 vertex constant cache (-1 = stale),
// +20 current VProgram*, +24 its CgProgram*, +28 current FProgram*,
// +92 dirty bits (0x80000000: fragment program changed).
#pragma once

#include <initializer_list>

#include "post.h"

namespace sly4::effect {

namespace fn {
constexpr u32 PostProc_DrawCopyBlend = 0x005433AC;  // decompiled: src/gfx/effect_draw.cpp
constexpr u32 Blur03_Draw = 0x00542D2C;             // decompiled: src/gfx/effect_draw.cpp
// Still recompiled: render helpers.
constexpr u32 Gcm_Sub5E3E64 = 0x005E3E64;       // (context, 1) libgcm? (unknown)
constexpr u32 Gcm_SetVertexProgramAt = 0x005E3C00;  // (context, ucode offset) libgcm? (name?)
constexpr u32 Draw_Sub513038 = 0x00513038;      // (effect) (unknown)
constexpr u32 Draw_SetDepthMode = 0x005355D8;   // (u32, u32) (name?)
constexpr u32 Draw_SetTexture0 = 0x00536AC0;    // (Target*, u32, u32) (name?)
constexpr u32 Draw_SetTarget = 0x005373A8;      // (Target*, u32) (name?)
constexpr u32 Draw_FullscreenQuad = 0x0053539C; // () (name?)
constexpr u32 Draw_EndQuad = 0x00535294;        // () (name?)
}  // namespace fn

// Makes *vertex the current vertex program (uploading it when its Cg
// program changes), as each draw inlines it.
inline void bind_vertex(PPUContext& ctx, u32 vertex) {
    const u32 st = post::g::gcm_state;
    const u32 vp = load<u32>(vertex);
    if (load<u32>(st + 20) == vp) return;
    store<u32>(st + 20, vp);
    store<s32>(st + 8, -1);
    const u32 cg = load<u32>(vp + 4);
    if (load<u32>(st + 24) == cg) return;
    store<u32>(st + 24, cg);
    ctx.gpr[3] = call(ctx, fn::Gcm_SetVertexProgramAt, {load<u32>(st), load<u32>(vp + 20)});
}

// Makes *fragment the current fragment program (flagging the change).
inline void bind_fragment(u32 fragment) {
    const u32 st = post::g::gcm_state;
    const u32 fp = load<u32>(fragment);
    if (load<u32>(st + 28) == fp) return;
    const u32 dirty = load<u32>(st + 92);
    store<u32>(st + 28, fp);
    store<u32>(st + 92, dirty | 0x80000000u);
}

// Appends words to the RSX command buffer of the render state's context.
inline void emit(std::initializer_list<u32> words) {
    const u32 c = load<u32>(post::g::gcm_state);
    const u32 cur = load<u32>(c + 8);
    u32 i = 0;
    for (u32 w : words) store<u32>(cur + 4 * i++, w);
    store<u32>(c + 8, cur + 4 * i);
}

}  // namespace sly4::effect
