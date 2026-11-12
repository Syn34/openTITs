// The renderer's core structures, native: the RSX command context, render
// targets, Cg program objects, the render state cache (0x010F10A0), the
// display, and the small command-writing helpers the draws inline.
//
// Native functions of src/gfx live in sly4::gfx and are declared here when
// other files call them.
#pragma once

#include <bit>
#include <cstddef>
#include <initializer_list>

#include "opentit/native.h"
#include "../sly4.h"

namespace sly4::gfx {
using namespace opentit::native;

// ---------------------------------------------------------------- small helpers
// The object at a byte offset computed at run time (the original indexes
// some tables by eye / buffer / constant number; guest addresses wrap at 32 bits).
template <typename T>
T* at(const void* base, u32 off) {
    return host<T>(guest(base) + off);
}
// p, or the zero page when p is null: for the few reads the original makes
// through a pointer it hasn't checked (it reads guest address 0 + offset).
template <typename T>
T* or_zero_page(T* p) {
    return p ? p : reinterpret_cast<T*>(mem::ptr(0));
}
inline f32 as_f32(u32 w) { return std::bit_cast<f32>(w); }
inline u32 bits(f32 v) { return std::bit_cast<u32>(v); }
inline f32 int_to_f32(u32 v) { return static_cast<f32>(static_cast<f64>(static_cast<s64>(v))); }  // fcfid + frsp

// ---------------------------------------------------------------- RSX context and surfaces
// CellGcmContextData.
struct GcmContext {
    be_u32 begin;     // +0
    be_u32 end;       // +4
    be_u32 current;   // +8: next command word
    be_u32 callback;  // +12
};

// CellGcmSurface (60 bytes).
struct GcmSurface {
    u8 type;                 // +0
    u8 antialias;            // +1
    u8 colour_format;        // +2
    u8 colour_target;        // +3
    u8 colour_location[4];   // +4
    be_u32 colour_offset[4];  // +8
    be_u32 colour_pitch[4];   // +24
    u8 depth_format;         // +40
    u8 depth_location;       // +41
    pad<2> _42;
    be_u32 depth_offset;     // +44
    be_u32 depth_pitch;      // +48
    be_u16 width;            // +52
    be_u16 height;           // +54
    be_u16 x;                // +56
    be_u16 y;                // +58
};
static_assert(sizeof(GcmSurface) == 60);

// Render target (60 bytes; see target.h for how it is made). Targets are
// also bound as textures: Texture below is the same memory seen as one.
struct Target {
    be_u32 address;       // +0: pixels
    be_u32 offset_loc;    // +4: RSX offset << 1 | location (1 = main memory)
    be_u32 size_flags;    // +8: size in bytes << 4 | flags
    be_u32 kind;          // +12: top nibble
    u8 surface_type;      // +16
    pad<3> _17;
    be_u32 aa_mode;       // +20
    be_s32 format;        // +24
    be_u32 surface_format;  // +28
    be_u32 pitch;         // +32
    be_u16 width;         // +36
    be_u16 height;        // +38
    be_u16 x;             // +40
    be_u16 y;             // +42: first visible row
    u8 bpp;               // +44
    s8 tile;              // +45: tile slot or -1
    u8 bank;              // +46
    s8 zcull;             // +47: zcull slot or -1
    be_u32 cull_start;    // +48
    be_s32 tag_base;      // +52: compression tag base or -1
    pad<4> _56;
};
static_assert(sizeof(Target) == 60);
static_assert(offsetof(Target, surface_format) == 28 && offsetof(Target, tile) == 45 && offsetof(Target, tag_base) == 52);

// CellGcmTexture (24 bytes).
struct GcmTexture {
    u8 format;        // +0
    u8 mipmap;        // +1
    u8 dimension;     // +2
    u8 cubemap;       // +3
    be_u32 remap;     // +4
    be_u16 width;     // +8
    be_u16 height;    // +10
    be_u16 depth;     // +12
    u8 location;      // +14
    pad<1> _15;
    be_u32 pitch;     // +16
    be_u32 offset;    // +20
};
static_assert(sizeof(GcmTexture) == 24);

// A texture: the CellGcmTexture at +16, and for the scene textures the
// matching render target at +40.
// (The 52-byte surface records Surface_SetBuffer fills are these.)
struct Texture {
    be_u32 address;        // +0
    be_u32 offset_loc;     // +4
    be_u32 size_flags;     // +8
    be_u32 kind;           // +12: top nibble
    GcmTexture gcm;        // +16
    gp<Target> target;     // +40
    be_u32 format_index;   // +44: into the format table (24-byte entries, +8 bits per pixel)
    u8 bound;              // +48
    pad<3> _49;
};
static_assert(sizeof(Texture) == 52 && offsetof(Texture, target) == 40 && offsetof(Texture, format_index) == 44);

// The surface format table (24-byte entries).
struct FormatDesc {
    pad<4> _0;
    be_u32 type;      // +4: low byte surface type
    be_s32 bpp;       // +8: bits per pixel
    pad<4> _12;
    be_u32 remap;     // +16
    be_u32 surface;   // +20
};
static_assert(sizeof(FormatDesc) == 24);
inline const FormatDesc* format_desc(u32 index) { return at<FormatDesc>(host<u8>(0x00F67254), index * 24); }

// ---------------------------------------------------------------- Cg programs (see post.h)
struct VProgram {
    be_u32 ucode;       // +0
    be_u32 cg;          // +4: CgProgram*
    be_u16 ucode_size;  // +8
    be_u16 attr_mask;   // +10
    be_u32 flags;       // +12
    be_u32 ucode_copy;  // +16
    be_u32 offset;      // +20: ucode offset for the RSX
};
static_assert(sizeof(VProgram) == 24);

// Fragment parameter record: {CgParameter (0 = unused), resource, binding entry, constant index}.
struct FParam {
    be_u32 param;
    be_u32 resource;
    be_u32 entry;
    be_u32 index;
};

struct FProgram {
    be_u32 ucode;        // +0: ucode copy
    be_u32 cg;           // +4: CgProgram*
    be_u16 ucode_size;   // +8
    pad<2> _10;
    be_u32 offset_loc;   // +12
    be_u64 dirty;        // +16: bit i = constant i changed
    be_u32 stamp;        // +24: frame the constants were last sent
    be_u32 used;         // +28
    pad<4> _32;
    be_u32 patch_table;  // +36: per constant {count, ucode offsets[count]}
    pad<8> _40;
    be_u64 consts[34][2];  // +48: vec4 constants (written as two doublewords)
    FParam params[34];     // +592
    pad<20> program;       // +1136: the CGprogram-like block given to cellGcmSetFragmentProgram
    pad<12> _1156;
};
static_assert(offsetof(FProgram, consts) == 48 && offsetof(FProgram, params) == 592 && offsetof(FProgram, program) == 1136);

// {constant name, constant index}: a fragment program binding list entry (ends at name 0).
struct Binding {
    gp<const char> name;
    be_u32 index;
};

// ProgramMap: a sorted array map {u32 key, u32 value} (two instantiations
// that differ only in their default comparator OPD).
struct MapEntry {
    be_u32 key;
    be_u32 value;
};
struct ProgramMap {
    be_u16 capacity;     // +0
    be_u16 count;        // +2
    be_u16 high;         // +4: high-water count
    be_u16 flags;        // +6: bit 0 unsorted
    gp<MapEntry> data;   // +8
    pad<4> _12;
    be_u32 not_found;    // +16: what a failed find points at
    be_u32 compare;      // +20: comparator OPD
};
static_assert(sizeof(ProgramMap) == 24);

// The vertex attribute list of a quad / vertex format (128 bytes).
struct VertexAttr {
    be_u32 index;     // +0
    be_u32 format;    // +4: size << 8 | type
    u8 offset;        // +8
    u8 index_add;     // +9
    u8 disabled;      // +10
    pad<1> _11;
};
struct VertexFormat {
    VertexAttr attrs[10];  // +0: ends at format 0
    be_u16 count;          // +120: vertex count (quad)
    pad<4> _122;
    be_u16 mask;           // +126: attribute mask
};
static_assert(sizeof(VertexFormat) == 128);

// ---------------------------------------------------------------- the render state cache (0x010F10A0)
struct RenderState {
    gp<GcmContext> context;  // +0
    be_u32 index_buffer;     // +4: current IndexBuffer*
    be_s32 vbase_cache;      // +8: vertex base the attribute formats were sent for (-1 stale)
    pad<4> _12;
    gp<VertexFormat> vformat;  // +16: current vertex format
    gp<VProgram> vprogram;   // +20
    be_u32 vcg;              // +24: its CgProgram*
    gp<FProgram> fprogram;   // +28
    be_u16 attr_mask;        // +32: enabled vertex attributes
    pad<2> _34;
    gp<Target> colour[4];    // +36
    gp<Texture> texture[8];  // +52: per texture unit
    gp<Target> depth;        // +84
    be_u32 unk88;            // +88
    union {
        be_u32 dirty;        // +92: 0x80000000 = fragment program changed
        u8 dirty_b[4];       // +93 (dirty_b[1]): depth test on
    };
    GcmSurface surface;      // +96
    be_u32 stamp;            // +156: frame stamp
    be_u32 stencil_func;     // +160
    be_u32 stencil_ref;      // +164
    be_u32 stencil_mask;     // +168: write mask to restore
    be_u32 stencil_func_mask;  // +172
    be_u32 stencil_op[3];    // +176
    be_u32 back_func;        // +188
    be_u32 back_ref;         // +192
    be_u32 back_mask;        // +196
    be_u32 back_func_mask;   // +200
    be_u32 back_op[3];       // +204
    be_u32 unk216;           // +216
    u8 unk220;               // +220
    u8 unk221;               // +221
    u8 mag_filter[3];        // +222
    u8 min_filter[3];        // +225
};
static_assert(offsetof(RenderState, depth) == 84 && offsetof(RenderState, surface) == 96 &&
              offsetof(RenderState, stamp) == 156 && offsetof(RenderState, unk216) == 216 &&
              offsetof(RenderState, min_filter) == 225);

// ---------------------------------------------------------------- display and buffer tables
// The display (draw::g::display). Eye-indexed slots are read at computed
// offsets with at<gp<Target>>(): +16 + eye*4 colour, +64 + eye*4 per-eye
// colour, +100 / +108 + eye*4 stereo colour.
struct Display {
    pad<16> _0;
    gp<Target> colour16[2];  // +16
    gp<Target> depth;        // +24
    pad<24> _28;
    be_u32 flags;            // +52: 0x02000000 per-eye buffers; bits 30..31 eye
    pad<16> _56;
    gp<Target> eye_depth;    // +72
    pad<8> _76;
    gp<Target> alt_colour;   // +84
    gp<Target> alt_depth;    // +88
    pad<28> _92;
    gp<Target> stereo_depth[2];  // +120
    pad<20> _128;
    u8 flags148;             // +148: bit 7 alternate buffer set
};
static_assert(offsetof(Display, flags) == 52 && offsetof(Display, alt_colour) == 84 &&
              offsetof(Display, stereo_depth) == 120 && offsetof(Display, flags148) == 148);

// The current view (View_GetCurrent, 0x004FDBA8), as the renderer reads it.
struct View {
    pad<60> _0;
    be_f32 near_z;           // +60
    be_f32 far_z;            // +64
    pad<28> _68;
    be_u16 rect[4];          // +96: x, y, w, h
    pad<648> _104;
    be_f32 rotation[3][4];   // +752: rows
    pad<208> _800;
    be_f32 matrix[16];       // +1008
};
static_assert(offsetof(View, rect) == 96 && offsetof(View, rotation) == 752 && offsetof(View, matrix) == 1008);

// A draw item as the render layers see it (fields named where known).
struct DrawItem {
    pad<12> _0;
    gp<u8> material;  // +12: +80 u32 low 6 bits sort key
    pad<12> _16;
    gp<u8> object;    // +28: +46 / +48 u16 sort key parts
    pad<16> _32;
    be_f32 pos[3];    // +48
};
static_assert(offsetof(DrawItem, pos) == 48);

// g::mem_stats (0x010F0E20) as the renderer uses it: scene textures per
// buffer set (+84.. read at computed offsets) and the buffer selector.
struct MemStats {
    pad<184> _0;
    be_u32 buffer_select;  // +184
};

// The local-memory pool fragment programs are copied into (g::local_pool).
struct LocalPool {
    pad<4> _0;
    be_u32 index;    // +4: current buffer
    be_u32 base[2];  // +8: base per buffer (indexed at +8 + index * 4)
    be_u32 offset;   // +16: grows down
};

namespace g {
constexpr u32 render_state = 0x010F10A0;
constexpr u32 display = 0x00F66FBC;          // Display*
constexpr u32 local_pool = 0x010F1338;       // LocalPool
constexpr u32 quad = 0x010F1020;             // VertexFormat of the full-screen quad
constexpr u32 quad_vertices = 0x00F671E4;
constexpr u32 zero_vconsts = 0x01164BC0;     // f32[4]
constexpr u32 work_targets = 0x01164040;     // Target*[4]: screen-sized work targets
}  // namespace g

inline RenderState& rs() { return global<RenderState>(g::render_state); }
inline GcmContext* gcm() { return rs().context; }
inline Display* display() { return global<gp<Display>>(g::display); }
inline MemStats& mem_stats() { return global<MemStats>(sly4::g::mem_stats); }
inline u8 config_u8(u32 off) { return global<u8>(sly4::g::config + off); }
inline Target* work_target(u32 i) { return global<gp<Target>>(g::work_targets + 4 * i); }
inline Texture* work_texture(u32 i) { return global<gp<Texture>>(g::work_targets + 4 * i); }  // the same object, as a texture
inline VProgram* vprog(u32 ea) { return global<gp<VProgram>>(ea); }  // a program global
inline FProgram* fprog(u32 ea) { return global<gp<FProgram>>(ea); }

// The SystemState function-local static (see sly4.h system_state), made on
// first use; +16 is the frame time scale the effects read.
inline u8* system_state() {
    using namespace sly4::system_state;
    u8& made = global<u8>(guard);
    if (made == 0) {
        global<be_u32>(object + 20) = 0u;
        global<be_u32>(object + 24) = 0u;
        global<u8>(object + 32) &= static_cast<u8>(~0xC0);
        call(sly4::fn::memset, object + 92, 0u, 400u);
        global<be_u32>(registry_slot) = object;
        made = 1;
    }
    return host<u8>(object);
}

// ---------------------------------------------------------------- RSX methods written inline
enum : u32 {
    kAlphaTestEnable = 0x00040304,
    kBlendEnable = 0x00040310,
    kBlendFunc = 0x00080314,  // two words: sfactor rgb|a, dfactor rgb|a
    kBlendEquation = 0x00040320,
    kColorMask = 0x00040324,
    kStencilTestEnable = 0x00040328,
    kSetWindowOffset = 0x000808C0,  // scissor: two words
    kDepthFunc = 0x00040A6C,
    kDepthMask = 0x00040A70,
    kDepthTestEnable = 0x00040A74,
    kFrontFace = 0x00041834,
    kCullFaceEnable = 0x0004183C,
    kTextureAddress0 = 0x00041A08,  // + unit * 32
};
constexpr u32 kSrcAlpha2 = 0x03020302, kOneMinusSrcAlpha2 = 0x03030303, kOne2 = 0x00010001;
constexpr u32 kFuncAdd2 = 0x80068006;

namespace fn {
// libgcm (still recompiled).
constexpr u32 cellGcmSetTexture = 0x005E4514;         // (context, index, CellGcmTexture*)
constexpr u32 cellGcmSetTextureControl = 0x005E3F18;  // (context, index, enable, minlod, maxlod, aniso)
constexpr u32 cellGcmSetTextureFilter = 0x005E3E88;   // (context, index, bias, min, mag, conv)
constexpr u32 cellGcmSetSurface = 0x005E47A0;         // (context, CellGcmSurface*)
constexpr u32 cellGcmSetViewport = 0x005E4068;        // (context, x, y, w, h, min, max, scale, offset)
constexpr u32 cellGcmSetFragmentProgram = 0x005E42A0; // (context, program, 1)
constexpr u32 cellGcmSetDrawInlineArray = 0x005E51AC; // (context, mode 9, count, data)
constexpr u32 cellGcmSetVertexProgramConstants = 0x005E5CB0;  // (context, first, count, f32*)
constexpr u32 cellGcmSetVertexProgramAt = 0x005E3C00;         // (context, ucode offset)
constexpr u32 cellGcmSetFragmentProgramOffset = 0x005E073C;   // (context, offset, location)
constexpr u32 cellGcmSetInvalidateTextureCache = 0x005E3E64;  // (context, 1)
constexpr u32 cellGcmSetTransferImage = 0x005E47AC;  // (context, mode, dst offset, dst pitch, dst x, y,
                                                     //  src offset, src pitch, src x, y, w, h, bytes per pixel)
constexpr u32 cellGcmSetClearColor = 0x005E3CB4;         // (context, colour)
constexpr u32 cellGcmSetClearDepthStencil = 0x005E3CD8;  // (context, value)
constexpr u32 cellGcmSetStencilMask = 0x005E3D8C;        // (context, mask)
constexpr u32 cellGcmSetClearSurface = 0x005E3C74;       // (context, mask)
constexpr u32 cellGcmAddressToOffset = 0x00ABD5E4;       // import
constexpr u32 memcpy = 0x00732BCC;
constexpr u32 View_GetCurrent = 0x004FDBA8;  // () -> View* (name?)
}  // namespace fn

// Appends words to the RSX command buffer (no space check, as the original).
inline void emit(std::initializer_list<u32> words) {
    GcmContext* c = gcm();
    const u32 cur = c->current;
    be_u32* p = host<be_u32>(cur);
    u32 i = 0;
    for (u32 w : words) p[i++] = w;
    c->current = cur + 4 * i;
}

// Makes vp the current vertex program (uploading it when its Cg program changes).
inline void bind_vertex(VProgram* vp) {
    RenderState& st = rs();
    if (st.vprogram.get() == vp) return;
    st.vprogram = vp;
    st.vbase_cache = -1;
    const u32 cg = vp->cg;
    if (st.vcg == cg) return;
    st.vcg = cg;
    call(fn::cellGcmSetVertexProgramAt, gcm(), vp->offset.get());
}

// Makes fp the current fragment program (flagging the change).
inline void bind_fragment(FProgram* fp) {
    RenderState& st = rs();
    if (st.fprogram.get() == fp) return;
    const u32 dirty = st.dirty;
    st.fprogram = fp;
    st.dirty = dirty | 0x80000000u;
}

// Fragment constant `index` of fp (16 bytes; index from the program's records).
inline be_u64* fconst(FProgram* fp, u32 index) { return at<be_u64>(fp, 48 + (index << 4)); }

// Writes {a, b} into fragment parameter record k of the current fragment
// program, if the program uses it; marks the constant dirty when the value
// changed or the program's stamp isn't this frame's.
inline void set_fconst(u32 k, u64 a, u64 b) {
    RenderState& st = rs();
    FProgram* fp = st.fprogram;
    const FParam* rec = at<FParam>(fp, 592 + 16 * k);
    if (rec->param == 0) return;
    const u32 index = rec->index;
    be_u64* dst = fconst(fp, index);
    const u64 old_a = dst[0], old_b = dst[1];
    dst[0] = a;
    dst[1] = b;
    const bool changed = ((old_a ^ a) | (old_b ^ b)) != 0 || fp->stamp != st.stamp;
    const u32 shift = index & 0x7F;  // sld
    const u64 bit = shift < 64 ? u64{1} << shift : 0;
    fp->dirty = fp->dirty | (changed ? bit : 0);
}
inline void set_fconst(u32 k, u32 w0, u32 w1, u32 w2, u32 w3) {
    set_fconst(k, (u64{w0} << 32) | w1, (u64{w2} << 32) | w3);
}
inline void set_fconst_from(u32 k, const void* src) {
    const be_u64* s = static_cast<const be_u64*>(src);
    set_fconst(k, s[0], s[1]);
}

// Blending on with the given factor pair, FUNC_ADD.
inline void blend_on(u32 sfactor, u32 dfactor) {
    emit({kBlendEnable, 1});
    emit({kBlendFunc, sfactor, dfactor});
    emit({kBlendEquation, kFuncAdd2});
}

inline void invalidate_textures() { call(fn::cellGcmSetInvalidateTextureCache, gcm(), 1u); }

// Makes d the depth target (0 = none): the depth half of the render
// state's surface, cellGcmSetSurface, then depth test / write on iff d.
inline void set_depth_target(Target* d) {
    RenderState& st = rs();
    st.depth = d;
    u32 offset = 0, pitch = 64;
    if (d) {
        st.surface.depth_format = static_cast<u8>(d->surface_format);
        st.surface.depth_location = static_cast<u8>(d->offset_loc & 1);
        offset = d->offset_loc >> 1;
        pitch = d->pitch;
    } else {
        st.surface.depth_location = 0;
        st.surface.depth_format = 2;
    }
    st.surface.depth_offset = offset;
    st.surface.depth_pitch = pitch;
    call(fn::cellGcmSetSurface, gcm(), &st.surface);
    const u32 on = d ? 1u : 0u;
    st.dirty_b[1] = static_cast<u8>(on);
    emit({kDepthTestEnable, on});
    emit({kDepthMask, on});
}

// Binds texture `tex` to `unit` as Draw_SetTexture0 does, with
// TEXTURE_ADDRESS `address` and filter pair `filter`.
inline void bind_texture(u32 unit, Texture* tex, u32 address, u32 filter) {
    RenderState& st = rs();
    gp<Texture>* slot = at<gp<Texture>>(&st, 52 + 4 * unit);
    if (slot->get() != tex) {
        *slot = tex;
        call(fn::cellGcmSetTexture, gcm(), unit, &tex->gcm);
        const u32 levels = tex->gcm.mipmap;
        call(fn::cellGcmSetTextureControl, gcm(), unit, 1u, 0u, ((levels << 8) - 256) & 0xFFFF, 0u);
    }
    emit({kTextureAddress0 + (unit << 5), address});
    call(fn::cellGcmSetTextureFilter, gcm(), unit, 0u, static_cast<u32>(*at<u8>(&st, 225 + filter)),
         static_cast<u32>(*at<u8>(&st, 222 + filter)), 1u);
}

// Copies colour target 0 (from its first visible row) into texture `tex`
// with cellGcmSetTransferImage (local to local).
inline void copy_colour_to(Texture* tex) {
    RenderState& st = rs();
    Target* t = st.colour[0];
    const u32 src_offset = (t->offset_loc >> 1) + t->pitch * t->y;
    const s32 bpp = format_desc(tex->format_index)->bpp;
    const u32 bytes = static_cast<u32>(bpp / 8) & 0xFF;  // srawi + addze
    call(fn::cellGcmSetTransferImage, gcm(), 0u, tex->gcm.offset.get(), tex->gcm.pitch.get(), 0u, 0u, src_offset,
         t->pitch.get(), 0u, 0u, static_cast<u32>(t->width), static_cast<u32>(t->height), bytes);
}

// Eye / buffer selection, as the display draws make it.
inline u32 eye_offset(const Display* d) { return (d->flags >> 28) & 0xC; }  // rlwinm 4,28,29
inline bool per_eye(const Display* d) { return ((d->flags >> 25) & 1) != 0; }
inline bool alternate(const Display* d) { return (d->flags148 >> 7) != 0; }

// The scene colour texture for the current eye / buffer set (its +40 word
// is the matching render target).
inline Texture* scene_colour() {
    const Display* disp = display();
    MemStats& ms = mem_stats();
    if (alternate(disp)) return *at<gp<Texture>>(&ms, 128);
    const u32 k = eye_offset(disp);
    if (config_u8(34) != 0) return *at<gp<Texture>>(&ms, k + (ms.buffer_select == 1 ? 112 : 120));
    return *at<gp<Texture>>(&ms, k + 92);
}
// The render target of a scene texture (+40).
inline Target* scene_target(Texture* tex) { return *at<gp<Target>>(tex, 40); }
// A scene texture of g::mem_stats at a computed offset.
inline Texture* stats_texture(u32 off) { return *at<gp<Texture>>(&mem_stats(), off); }

// The display's depth target for the current eye / buffer set.
inline Target* display_depth() {
    Display* disp = display();
    if (!per_eye(disp)) return disp->depth;
    if (config_u8(35) == 0 || config_u8(34) == 0) return alternate(disp) ? disp->alt_depth : disp->eye_depth;
    return disp->stereo_depth[mem_stats().buffer_select == 1 ? 0 : 1];
}

// ---------------------------------------------------------------- native functions (draw_state.cpp, fx_draw.cpp, render_state.cpp)
void Draw_SetTexture0(Texture* t, u32 unit, u32 filter);
void Draw_SetTarget(Target* t, u32 slot);
void Draw_ResetState(bool no_blend, u32 vconst);
void Draw_PrepareQuad();
void Draw_RestoreDepth();
void Draw_BindScreenTargets(void* unused);
void Draw_BindDisplay();
void Draw_BindTargets(Target* depth, Target* colour, u32 slot);
void FProgram_Upload(gp<FProgram>* current);
gp<Target>* Display_GetTargets(gp<gp<Target>>* colour_out, gp<gp<Target>>* depth_out);
Texture* Display_SelectBuffer(const void* table);
void Draw_SetDepthTarget(Target* d);
void Draw_InvalidateState();
const VertexFormat* VertexFormat_Get(bool second);
void Blur11V_SetOffsets();
// tile.cpp, target.cpp, init_surface.cpp
s32 Target_BindTile(gp<Target>* slot, bool alt);
void Target_BindZcull(gp<Target>* slot);
u32 Surface_GetSize(u32 width, u32 height, s32 format, s32 aa, bool tiled, be_u32* pitch_out);
void Surface_Create(gp<Target>* out, gp<Target>* src, u32 w, u32 h, u32 unused, u32 skip, u32 unused2);
void Target_Init(gp<Target>* out, u32 w, u32 h, s32 format, s32 aa, u32 where);
void Target_Attach(gp<Target>* out, gp<Target>* src, u32 w, u32 h, u32 unused, s32 row);
void Surface_SetBuffer(Texture* s, Target* t);
Target* Target_Construct(Target* t);  // surface_alloc.cpp
// post_map.cpp, post_programs.cpp, prog_pair.cpp
void FMap_Insert(ProgramMap* map, const be_u32* key, FProgram* value);
gp<FProgram>* FMap_Find(ProgramMap* map, const be_u32* key);
void VMap_Insert(ProgramMap* map, const be_u32* key, VProgram* value);
gp<VProgram>* VMap_Find(ProgramMap* map, const be_u32* key);
void VProgram_Create(gp<VProgram>* out, u32 cg, u32 unused);
FProgram* FProgram_Create(gp<FProgram>* out, u32 cg, u32 unused);
void FProgram_Setup(gp<FProgram>* out, const Binding* extra);
void FProgram_Finish(gp<FProgram>* out);
void VProgram_Load(gp<VProgram>* v, const char* name, u32 unused);
void FProgram_Load(gp<FProgram>* f, const char* name, const Binding* bindings);
void PostProc_Init();
void PostProc_InitCopy();
void StencilShadow_Init();
void PostProc_InitEffects();
void ProgramPair_Load(gp<VProgram>* v, gp<FProgram>* f, const char* name, const Binding* bindings, u32 unused);
void Blur11H_SetOffsets();

// At least `min` registers for the fragment program *f.
inline void min_registers(gp<FProgram>* f, u8 min) {
    constexpr u32 Cg_GetRegisterCount = 0x005E5F38;  // (CgProgram*) -> u32 (name?)
    constexpr u32 Cg_SetRegisterCount = 0x005E5F9C;  // (CgProgram*, u32) (name?)
    u8& have = *at<u8>(f->get(), 1156);
    have = have > min ? have : min;
    const u32 cg = or_zero_page(f->get())->cg;
    if (call<u32>(Cg_GetRegisterCount, cg) < min) call(Cg_SetRegisterCount, or_zero_page(f->get())->cg.get(), static_cast<u32>(min));
}

// Draw_PrepareQuad + Draw_RestoreDepth.
inline void draw_quad() {
    Draw_PrepareQuad();
    Draw_RestoreDepth();
}

// Draw_SetTarget(t, 0) and a texture cache invalidate.
inline void set_target(Target* t) {
    Draw_SetTarget(t, 0);
    invalidate_textures();
}

}  // namespace sly4::gfx
