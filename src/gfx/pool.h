// The stencil shadow renderer (layers3::g::pool, 0x010DF8B0; vtable
// 0x00B241E8, base 0x00B236E8) and the small draw helpers next to it.
//
// StencilShadow, beyond layers4.cpp's fields:
//   +4 f32* vertex cursor (volume triangles, 12 bytes per vertex)
//   +160 VProgram* / +164 FProgram* StencilShadow (the volume pass)
//   +168 VProgram* / +172 FProgram* StencilShadow_DrawQuad (the shade pass)
//   +176 vertex format of the volume vertices (as below; +296 stride, +302 mask)
//   +304 s8* per-face light term (127 * N.L; negative = facing away)
//   +308 u8[4] shade colour (RGB used, alpha ignored), +312 f32 shade intensity
//   +316 f32 extrusion distance, +320 u8 caps (z-fail with caps when set)
//   +324 f32* cursor at the start of the current Build
// Slots: 0 AddCaps, 1 AddCapsSkinned, 2 ~, 3 delete, 4 nop, 5 Begin,
// 6 Build, 7 DrawVolume, 8 DrawQuad, 9 ResetState, 10 Refresh, 11 nop.
//
// Shadow caster (the object slots 0 / 1 / 6 take): +0 ShadowMesh*, +4 u16
// volume triangle count (set by Build).
// ShadowMesh: +0 header (+72 nonzero: skinned, use slot 1), +4 u32 edges[]
// (bits 18..31 index-list position of the face, 4..17 the neighbour's,
// 2..3 / 0..1 the edge's two corners), +0x8004 f32[4] face normals,
// +0x8008 u16 edge count, +0x800C f32[4] rest positions, +0x8010 f32[4]
// skinned positions, +0x8014 u16 index list (3 per face), +0x8018 u16
// index count.
//
// Vertex format (as Draw_PrepareQuad reads it): 12-byte attributes {+0 u32
// register base, +4 u32 type / size (0 ends the list), +8 u8 offset, +9 u8
// register, +10 u8 skip}, +120 u16 stride, +126 u16 attribute mask.
#pragma once

#include <cmath>

#include "layers4.h"

namespace sly4::render {
namespace pool {

namespace fn {
// Decompiled in pool*.cpp.
constexpr u32 StencilShadow_AddCaps = 0x00526650;  // native (rsx.h) slot 0 (name?)
constexpr u32 StencilShadow_AddCapsSkinned = 0x00526A84;  // native (rsx.h) slot 1 (name?)
constexpr u32 StencilShadow_FaceSign = 0x00526960;  // native (rsx.h) (StencilShadow*, index, u16* indices, f32* positions, f32* light) (name?)
constexpr u32 StencilShadow_Begin = 0x0054B3D8;  // native (rsx.h) slot 5 (name?)
constexpr u32 StencilShadow_Build = 0x00526D98;  // native (rsx.h) slot 6 (name?)
constexpr u32 StencilShadow_DrawVolume = 0x0054A998;  // native (rsx.h) slot 7 (name?)
constexpr u32 StencilShadow_DrawQuad = 0x0054B6E0;  // native (rsx.h) slot 8 (name?)
constexpr u32 Draw_InlineArray = 0x007A31CC;  // native (rsx.h) (mode, vertex count, data, VertexFormat*) (name?)
constexpr u32 MaterialAnim_SetState = 0x00501410;  // native (rsx.h) (track*, state) (name?)
// layers4.h's Draw_Clear, DrawItem_DrawDepth, DrawItem_GetScale,
// View_ProjectSphere and MaterialAnim_Update are decompiled in pool.cpp /
// pool_draw.cpp.
// Still recompiled.
constexpr u32 DrawItem_DrawDepthSkinned = 0x005382C0;  // native (rsx.h) (mesh, g::skin_table, DrawItem*, item+60, item+28, 0x40000000) (name?)
constexpr u32 cellGcmSetDrawIndexArray = 0x005E4F64;   // (context, mode, count, type, location, offset)
constexpr u32 cellGcmSetDrawArrays = 0x005E4D98;       // (context, mode, first, count) (name?)
}  // namespace fn

namespace g {
constexpr u32 clear_stencil = 0x010DF8A2;  // u8: StencilShadow_Begin clears the stencil buffer once when set (name?)
constexpr u32 skin_depth = 0x00F674FC;     // u8: DrawItem_DrawDepth's skinned path enabled (name?)
constexpr u32 skin_table = 0x01163A80;     // u32 passed to DrawItem_DrawDepthSkinned (name?)
constexpr u32 anim_random = 0x01164888;    // u64 last random 0..0xFFFF, +8 u32 seed (MaterialAnim_SetState) (name?)
}  // namespace g

// Stencil shadow fields.
constexpr u32 kCursor = 4, kVolumeVProgram = 160, kVolumeFProgram = 164, kQuadVProgram = 168,
              kQuadFProgram = 172, kFormat = 176, kFaceSign = 304, kColour = 308, kIntensity = 312,
              kExtrude = 316, kCaps = 320, kStart = 324;
// Mesh fields.
constexpr u32 kEdges = 4, kNormals = 0x8004, kEdgeCount = 0x8008, kPositions = 0x800C,
              kSkinnedPositions = 0x8010, kIndices = 0x8014, kIndexCount = 0x8018;

// RSX methods and values (beyond layers4.h / fx.h).
constexpr u32 kStencilFunc = 0x000C0330, kStencilMask = 0x0004032C, kStencilOp = 0x000C033C;
constexpr u32 kBackStencilFunc = 0x000C0350, kBackStencilMask = 0x0004034C, kBackStencilOp = 0x000C035C;
constexpr u32 kTwoSidedStencil = 0x00040348, kVertexFormat0 = 0x00041740, kVertexOffset0 = 0x00041680;
constexpr u32 kKeep = 0x1E00, kIncr = 0x1E02, kDecr = 0x1E03, kIncrWrap = 0x8507, kDecrWrap = 0x8508;
constexpr u32 kLess = 0x0201, kNotEqual = 0x0205;
constexpr u32 kTriangles = 5;

// The 16 slots (layers3::g::slots; vtable layers4::kSlotVtable 0x00B240F8,
// base 0x00B23720: 0 ~, 1 delete, 2 Issue, 3 GetResult): occlusion
// queries. +80 entry[4] of 20 bytes {+0 / +4 u32 result pair, +8 / +12
// u32 report index (depth test off / on pass), +16 u8 state: 1 issued, 7
// done}, +160 u16 current entry. Each pass allocates a report from the
// ring at g::report_next (128 entries, marked at g::report_base +30636
// + 16 * i; index i + 1914).
// Two more small classes sit in the same vtable block: 0x00B24110 (a base
// with nothing to destroy) and 0x00B24120 (derived: +8 buffer from
// Mem_FreePool pool 4, tag 1).
namespace query {
constexpr u32 kVtable = 0x00B240F8, kBaseVtable = 0x00B23720;
constexpr u32 kBufferBaseVtable = 0x00B24110, kBufferVtable = 0x00B24120;
constexpr u32 kEntries = 80, kEntrySize = 20, kCurrent = 160;
constexpr u32 kReportMark = 30636, kReportIndex = 1914, kReportRing = 0x7F;
namespace fn {
constexpr u32 SlotBase_Destruct = 0x004FCC10;  // native (rsx.h) base vtable slot 0 (name?)
constexpr u32 SlotBase_DestructInline = 0x004FCC20;  // native (rsx.h) the same, called by Slot_Delete (name?)
constexpr u32 SlotBase_Delete = 0x005276C4;  // native (rsx.h) base vtable slot 1 (name?)
constexpr u32 Slot_Destruct = 0x0079C404;  // native (rsx.h) slot 0 (name?)
constexpr u32 Slot_Delete = 0x007A395C;  // native (rsx.h) slot 1 (name?)
constexpr u32 Slot_Issue = 0x00548694;  // native (rsx.h) slot 2 (name?)
constexpr u32 Slot_GetResult = 0x00548848;  // native (rsx.h) slot 3 (Slot*, u32 out[2], u32 frames back) -> done (name?)
constexpr u32 BufferBase_Destruct = 0x007A39B4;  // native (rsx.h) vtable 0x00B24110 slot 0 (name?)
constexpr u32 BufferBase_Delete = 0x007A39C4;  // native (rsx.h) slot 1 (name?)
constexpr u32 Buffer_Destruct = 0x007A3A08;  // native (rsx.h) vtable 0x00B24120 slot 0 (name?)
constexpr u32 Buffer_Delete = 0x007A3A60;  // native (rsx.h) slot 1 (name?)
// Still recompiled.
constexpr u32 Slot_DrawProxy = 0x00527708;  // native (rsx.h) (Slot*) the geometry each query pass draws (name?)
constexpr u32 cellGcmSetClearReport = 0x005E3248;    // (context, type) (name?)
constexpr u32 cellGcmSetReport = 0x005E32D8;         // (context, type, index) (name?)
}  // namespace fn
namespace g {
constexpr u32 report_base = 0x011640E8;  // u32 (name?)
constexpr u32 report_next = 0x011640EC;  // u32 ring position 0..127 (name?)
}  // namespace g
}  // namespace query

// Scalar FPU operations as the PPU does them on f64 registers (the
// s-forms round to single).
inline f64 sgl(f64 x) { return rt::to_single(x); }
inline f64 lfs(u32 a) { return static_cast<f64>(load<f32>(a)); }
inline void stfs(u32 a, f64 v) { store<f32>(a, static_cast<f32>(v)); }
inline f64 fmadds(f64 a, f64 c, f64 b) { return sgl(std::fma(a, c, b)); }
inline f64 fmsubs(f64 a, f64 c, f64 b) { return sgl(std::fma(a, c, -b)); }
inline f64 fnmsubs(f64 a, f64 c, f64 b) { return sgl(-std::fma(a, c, -b)); }
// fsel: a >= 0 ? ge : lt (NaN takes lt).
inline f64 sel_ge(f64 a, f64 ge, f64 lt) { return a >= 0.0 ? ge : lt; }
// lfs / stfs pairs copy raw words.
inline void copy_word(u32 dst, u32 src) { store<u32>(dst, load<u32>(src)); }
inline void copy3(u32 dst, u32 src) {
    for (u32 k = 0; k < 12; k += 4) copy_word(dst + k, src + k);
}

// The stencil state the volume and quad passes write: the cached word
// triples at render state +160 (front func: func, ref, +172 mask), +176
// (front ops), +188 (back func, +200 mask) and +204 (back ops), each
// re-sent after its fields are set.
inline u32 st_word(u32 off) { return load<u32>(fx::g::state + off); }
inline void st_set(u32 off, u32 v) { store<u32>(fx::g::state + off, v); }
inline void front_func() { fx::emit({kStencilFunc, st_word(160), st_word(164), st_word(172)}); }
inline void front_op() { fx::emit({kStencilOp, st_word(176), st_word(180), st_word(184)}); }
inline void back_func() { fx::emit({kBackStencilFunc, st_word(188), st_word(192), st_word(200)}); }
inline void back_op() { fx::emit({kBackStencilOp, st_word(204), st_word(208), st_word(212)}); }

// g::config +152 bit 5: two-sided stencil available.
inline bool two_sided_stencil() { return ((ref<u8>(sly4::g::config + 152) >> 5) & 1) != 0; }  // rlwinm 27,31,31

// Sends the attribute formats of vertex format `vf` that are in `mask`
// (offset (attribute +8 + base) | loc), then switches off the attributes
// still enabled (render state +32) but not in `mask`, highest first, and
// records `mask`. `offset_first` sends each offset word before its
// method word (StencilShadow_DrawQuad's store order).
inline void send_attributes(u32 vf, u32 stride, u32 mask, u32 base, u32 loc, bool offset_first = false) {
    const u32 st = fx::g::state;
    if (load<u32>(vf + 4) != 0) {
        const u32 stride_bits = (stride << 8) & 0xFF00;  // rlwinm 8,16,23
        u32 e = vf;
        do {
            const u32 index = load<u32>(e) + ref<u8>(e + 9);
            const u32 shift = index & 63;  // slw
            const u32 bit = shift < 32 ? 1u << shift : 0;
            if (ref<u8>(e + 10) == 0 && (bit & mask) != 0) {
                const u32 type = load<u32>(e + 4);
                const u32 reg = (index << 2) & 0x3FC;  // rlwinm 2,22,29
                const u32 format = (((type >> 8) & 0xFF) << 4) | stride_bits | (type & 0xFF);
                const u32 offset = (ref<u8>(e + 8) + base) | loc;
                const u32 c = load<u32>(st);
                const u32 cur = load<u32>(c + 8);
                store<u32>(cur, kVertexFormat0 + reg);
                store<u32>(cur + 4, format);
                if (offset_first) {
                    store<u32>(cur + 12, offset);
                    store<u32>(cur + 8, kVertexOffset0 + reg);
                } else {
                    store<u32>(cur + 8, kVertexOffset0 + reg);
                    store<u32>(cur + 12, offset);
                }
                store<u32>(c + 8, cur + 16);
            }
            e += 12;
        } while (load<u32>(e + 4) != 0);
    }
    u32 stale = (load<u16>(st + 32) | mask) ^ mask;
    if (stale != 0) {
        for (s32 i = 15; i >= 0; --i) {
            const u32 bit = (1u << i) & 0xFFFF;
            if (stale & bit) {
                const u16 enabled = load<u16>(st + 32);
                stale = (stale & ~bit) & 0xFFFF;
                store<u16>(st + 32, static_cast<u16>(enabled & ~bit));
                fx::emit({kVertexFormat0 + ((static_cast<u32>(i) << 2) & 0x3FC), 2});  // attribute off
            }
            if (stale == 0) break;
        }
    }
    store<u16>(st + 32, static_cast<u16>(mask));
}

// Makes `vf` the current vertex format with the vertex data at `word`
// (bit 0 location, bits 1..31 offset; render state +16 / +8 cache them),
// enabling the attributes the current vertex program (+20, its +10 mask)
// also uses.
inline void bind_format(u32 vf, u32 word, bool offset_first = false) {
    const u32 st = fx::g::state;
    if (load<u32>(st + 16) != vf) store<s32>(st + 8, -1);
    const u32 cache = load<u32>(st + 8);
    const u32 stride = load<u16>(vf + 120);
    store<u32>(st + 16, vf);
    const u32 base = word >> 1;  // rlwinm 31,1,31
    if (cache == base) return;
    store<u32>(st + 8, base);
    const u32 mask = load<u16>(load<u32>(st + 20) + 10) & load<u16>(vf + 126);
    send_attributes(vf, stride, mask, base, (word & 1) << 31, offset_first);
}

// Uploads the current fragment program: re-binds it first (clearing the
// dirty bit, render state +92 bit 31) when it changed, otherwise only when
// its constants are dirty (FProgram +16).
inline void upload_fragment(PPUContext& ctx) {
    const u32 st = fx::g::state;
    const u32 dirty = load<u32>(st + 92);
    if ((dirty >> 31) != 0) {
        const u32 fp = load<u32>(st + 28);
        store<u32>(st + 92, dirty & 0x7FFFFFFF);
        call(ctx, draw::fn::cellGcmSetFragmentProgram, {load<u32>(st), fp + 1136, 1});
        call(ctx, fx::fn::FProgram_Upload, {st + 28});
        return;
    }
    const u32 fp = load<u32>(st + 28);
    if (fp != 0 && load<u64>(fp + 16) != 0) call(ctx, fx::fn::FProgram_Upload, {st + 28});
}

}  // namespace pool
}  // namespace sly4::render
