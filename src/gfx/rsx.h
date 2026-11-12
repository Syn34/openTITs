// The renderer's core structures, native: the RSX command context, render
// targets, Cg program objects, the render state cache (0x010F10A0), the
// display, and the small command-writing helpers the draws inline.
//
// Native functions of src/gfx live in sly4::gfx and are declared here when
// other files call them.
#pragma once

#include <bit>
#include <cmath>
#include <cstddef>
#include <initializer_list>

#include "opentit/native.h"
#include "../sly4.h"
#include "../container/container.h"

namespace sly4::gfx {
using namespace opentit::native;

// ---------------------------------------------------------------- small helpers
// The object at a byte offset computed at run time (the original indexes
// some tables by eye / buffer / constant number; guest addresses wrap at 32 bits).
// Never null: an address that wraps to 0 is guest page 0, as for the original.
template <typename T>
T* at(const void* base, u32 off) {
    return reinterpret_cast<T*>(mem::ptr(guest(base) + off));
}
// p, or the zero page when p is null: for the few reads the original makes
// through a pointer it hasn't checked (it reads guest address 0 + offset).
template <typename T>
T* or_zero_page(T* p) {
    return p ? p : reinterpret_cast<T*>(mem::ptr(0));
}
// The 16-byte aligned block at or below p (lvx); never null.
inline const be_f32* aligned16(const void* p) { return reinterpret_cast<const be_f32*>(mem::ptr(guest(p) & ~15u)); }
inline f32 as_f32(u32 w) { return std::bit_cast<f32>(w); }
inline u32 bits(f32 v) { return std::bit_cast<u32>(v); }
// Single-precision fused multiply-adds as the PPU does them (fma in double,
// then rounded to single): fmadds a*c+b, fmsubs a*c-b, fnmsubs -(a*c-b),
// fnmadds -(a*c+b).
inline f32 fmadds(f32 a, f32 c, f32 b) { return static_cast<f32>(std::fma(f64{a}, f64{c}, f64{b})); }
inline f32 fmsubs(f32 a, f32 c, f32 b) { return static_cast<f32>(std::fma(f64{a}, f64{c}, -f64{b})); }
inline f32 fnmsubs(f32 a, f32 c, f32 b) { return static_cast<f32>(-std::fma(f64{a}, f64{c}, -f64{b})); }
inline f32 fnmadds(f32 a, f32 c, f32 b) { return static_cast<f32>(-std::fma(f64{a}, f64{c}, f64{b})); }
// fsel: a >= 0 ? ge : lt (NaN takes lt).
inline f64 fsel(f64 a, f64 ge, f64 lt) { return a >= 0.0 ? ge : lt; }
// fctidz: f64 -> s64 toward zero, saturating as the PPU does.
inline s64 fctidz(f64 x) { return static_cast<s64>(rt::fctid(x, true)); }
// fctiwz: f64 -> s32 toward zero, saturating as the PPU does.
inline s32 fctiwz(f64 x) { return static_cast<s32>(rt::fctiw(x, true)); }
// lfs / stfs pairs copy raw words.
inline void copy_word(void* dst, const void* src) { *static_cast<be_u32*>(dst) = static_cast<const be_u32*>(src)->get(); }
inline void copy3(void* dst, const void* src) {
    for (u32 k = 0; k < 3; ++k) copy_word(static_cast<be_u32*>(dst) + k, static_cast<const be_u32*>(src) + k);
}
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
    pad<16> _28;
    be_u32 clear_mask;       // +44: 240 colour, 243 with depth, 3 depth only
    pad<4> _48;
    be_u32 flags;            // +52: 0x02000000 per-eye buffers; bits 30..31 eye
    be_u32 width;            // +56
    be_u32 height;           // +60
    gp<Target> eye_colour[2];  // +64
    gp<Target> eye_depth;    // +72
    gp<Target> extra[2];     // +76
    gp<Target> alt_colour;   // +84
    gp<Target> alt_depth;    // +88
    gp<Target> copy[2];      // +92: 3D copies of the colour targets at the buffer size
    gp<Target> eye_view[4];  // +100: 720-row eye views of copy[] (left pair, right pair)
    gp<Target> packed;       // +116: both eyes frame-packed
    gp<Target> stereo_depth[2];  // +120: the eye views of packed
    gp<Target> extra_view[2];    // +128
    pad<12> _136;
    u8 flags148;             // +148: bit 7 alternate buffer set
};
static_assert(offsetof(Display, flags) == 52 && offsetof(Display, alt_colour) == 84 &&
              offsetof(Display, stereo_depth) == 120 && offsetof(Display, extra_view) == 128 &&
              offsetof(Display, flags148) == 148);

// The current view (View_GetCurrent, 0x004FDBA8), as the renderer reads it.
struct View {
    pad<60> _0;
    be_f32 near_z;           // +60
    be_f32 far_z;            // +64
    pad<28> _68;
    be_u16 rect[4];          // +96: x, y, w, h
    pad<648> _104;
    be_f32 rotation[3][4];   // +752: rows
    pad<80> _800;
    be_f32 view_proj[16];    // +880 (read 16-byte aligned)
    pad<64> _944;
    be_f32 matrix[16];       // +1008
};
static_assert(offsetof(View, rect) == 96 && offsetof(View, rotation) == 752 && offsetof(View, view_proj) == 880 &&
              offsetof(View, matrix) == 1008);

// A draw item as the render layers see it (fields named where known).
struct DrawMesh;
struct Material;
struct DrawItem {
    pad<12> _0;
    gp<DrawMesh> mesh;  // +12: +80 u32 low 6 bits sort key
    pad<12> _16;
    gp<Material> mat;   // +28: +46 / +48 u16 sort key parts
    pad<12> _32;
    gp<be_u32> layers;  // +44: layer list (+0 layer index)
    be_f32 pos[3];      // +48
    gp<u8> bones;       // +60: 48-byte bone matrices
    be_u32 w64;         // +64
    u8 flags68;         // +68
    u8 variant;         // +69: the shader variant the picker writes back
};
static_assert(offsetof(DrawItem, pos) == 48 && offsetof(DrawItem, bones) == 60 && offsetof(DrawItem, variant) == 69);

// An index buffer (render state +4 points at the current one).
struct IndexBuffer {
    pad<4> _0;
    be_u32 where;   // +4: offset << 1 | location
    pad<8> _8;
    be_u32 type;    // +16: low byte
};
// A mesh as the depth pre-pass draws it.
struct DrawMesh {
    pad<28> _0;
    be_u32 vertex_word;       // +28: vertex data, offset << 1 | location
    pad<8> _32;
    be_u32 vertex_count;      // +40
    IndexBuffer index_buffer;  // +44
    be_u32 index_count;       // +64
    gp<VertexFormat> format;  // +68
    pad<4> _72;
    be_u32 mode;              // +76: low byte primitive
    pad<4> _80;
    gp<u8> palette;           // +84: bone palette (u8 count, u8 bone[])
    be_u32 job_param;         // +88: f32 copied into each skin job record (+188)
    pad<20> _92;
    gp<struct MeshSkin> skin;  // +112
};
// A mesh's skin (DrawMesh +112).
struct VertexBufferRef {
    pad<4> _0;
    be_u32 where;  // +4: offset << 1 | location
};
struct MeshSkin {
    gp<u8> parts;                     // +0: 128-byte part records
    gp<gp<VertexBufferRef>> buffers;  // +4: per part
    be_u32 count;                     // +8: parts
};
static_assert(offsetof(DrawMesh, index_buffer) == 44 && offsetof(DrawMesh, format) == 68 && offsetof(DrawMesh, skin) == 112);

// g::mem_stats (0x010F0E20) as the renderer uses it: scene textures per
// buffer set (+84.. read at computed offsets) and the buffer selector.
struct MemStats {
    pad<8> _0;
    u8 resolution;         // +8: video-out resolution id (cellVideoOutGetState writes +0..+15)
    pad<2> _9;
    u8 aspect;             // +11
    pad<2> _12;
    be_u16 refresh_rates;  // +14
    be_u16 res_w;          // +16: CellVideoOutResolution, 2D
    be_u16 res_h;          // +18
    be_u16 buffer_w;       // +20: CellVideoOutResolution, 3D (the buffer size)
    be_u16 buffer_h;       // +22
    be_u32 res3d_id;       // +24
    be_u32 res_id;         // +28
    pad<24> _32;
    be_f32 one;            // +56: 1.0f
    pad<24> _60;
    gp<Texture> resolve;   // +84: the resolve texture (2D)
    pad<12> _88;
    gp<Texture> wide;      // +100: buffer-width 720-row frame record
    gp<Texture> eye[2];    // +104
    gp<Texture> pair_a[2];  // +112
    gp<Texture> pair_b[2];  // +120
    pad<32> _128;
    be_f32 screen_size;    // +160: inches (cellVideoOutGetScreenSize)
    pad<16> _164;
    be_u16 screen_w;       // +180
    be_u16 screen_h;       // +182
    be_u32 buffer_select;  // +184: eye (1 = left?)
};
static_assert(offsetof(MemStats, res3d_id) == 24 && offsetof(MemStats, one) == 56 && offsetof(MemStats, wide) == 100 &&
              offsetof(MemStats, screen_size) == 160 && offsetof(MemStats, buffer_select) == 184);

// The start-up configuration (sly4::g::config), as the renderer uses it.
struct Config {
    pad<4> _0;
    be_u16 width;        // +4
    be_u16 height;       // +6
    be_u16 unk8;         // +8
    be_u16 unk10;        // +10
    pad<8> _12;
    be_f32 refresh;      // +20: Hz
    be_u16 safe_x;       // +24: 5% margins
    be_u16 safe_y;       // +26
    be_u16 safe_w;       // +28: the size inside them
    be_u16 safe_h;       // +30
    pad<1> _32;
    u8 wide;             // +33: aspect is not 4:3
    u8 stereo;           // +34: 3D in use
    u8 stereo_ok;        // +35: 3D available
    pad<40> _36;
    be_u32 layer_count;  // +76: resolve layers drawn (a frame stamp for the scene copy)
    pad<244> _80;
    be_u32 frame_bytes;  // +324: render frame buffer size (0 = default, negative = 0)
    be_u32 frame_bytes2;  // +328: second pair (0 = none)
    pad<22> _332;
    be_u16 surface_count;  // +354: render surface free-list size (0 = 1024)
    pad<68> _356;
    be_u32 heap_bytes;   // +424: render LinearHeap size
};
static_assert(offsetof(Config, refresh) == 20 && offsetof(Config, wide) == 33 && offsetof(Config, frame_bytes) == 324 &&
              offsetof(Config, surface_count) == 354 && offsetof(Config, heap_bytes) == 424);

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
inline Config& config() { return global<Config>(sly4::g::config); }
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
// Stencil and vertex attribute methods; GL values.
enum : u32 {
    kStencilFunc = 0x000C0330,  // three words: func, ref, mask
    kStencilMask = 0x0004032C,
    kStencilOp = 0x000C033C,    // three words: fail, zfail, zpass
    kBackStencilFunc = 0x000C0350,
    kBackStencilMask = 0x0004034C,
    kBackStencilOp = 0x000C035C,
    kTwoSidedStencil = 0x00040348,
    kVertexFormat0 = 0x00041740,  // + index * 4
    kVertexOffset0 = 0x00041680,  // + index * 4
};
enum : u32 {
    kGlLess = 0x0201, kGlLequal = 0x0203, kGlNotEqual = 0x0205,
    kGlKeep = 0x1E00, kGlIncr = 0x1E02, kGlDecr = 0x1E03, kGlIncrWrap = 0x8507, kGlDecrWrap = 0x8508,
    kGlCw = 0x0900, kGlCcw = 0x0901,
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

inline View* current_view() { return call<View*>(fn::View_GetCurrent); }

// Appends words to the RSX command buffer (no space check, as the original).
inline void emit(std::initializer_list<u32> words) {
    GcmContext* c = or_zero_page(gcm());  // as the original, which reads through it unchecked
    const u32 cur = c->current;
    be_u32* p = reinterpret_cast<be_u32*>(mem::ptr(cur));
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
    const u32 cg = or_zero_page(vp)->cg;  // (unchecked by the original)
    if (st.vcg == cg) return;
    st.vcg = cg;
    call(fn::cellGcmSetVertexProgramAt, gcm(), or_zero_page(vp)->offset.get());
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

// The cached stencil state (render state +160..+212), each triple re-sent
// after its fields are set.
inline void front_func() { RenderState& st = rs(); emit({kStencilFunc, st.stencil_func, st.stencil_ref, st.stencil_func_mask}); }
inline void front_op() { RenderState& st = rs(); emit({kStencilOp, st.stencil_op[0], st.stencil_op[1], st.stencil_op[2]}); }
inline void back_func() { RenderState& st = rs(); emit({kBackStencilFunc, st.back_func, st.back_ref, st.back_func_mask}); }
inline void back_op() { RenderState& st = rs(); emit({kBackStencilOp, st.back_op[0], st.back_op[1], st.back_op[2]}); }
// g::config +152 bit 5: two-sided stencil available.
inline bool two_sided_stencil() { return ((config_u8(152) >> 5) & 1) != 0; }  // rlwinm 27,31,31

// Sends the attribute formats of `vf` that are in `mask` (offset
// (attribute offset + base) | loc), then switches off the attributes
// still enabled (render state +32) but not in `mask`, highest first, and
// records `mask`. `offset_first` stores each offset word before its
// method word (StencilShadow_DrawQuad's store order).
inline void send_attributes(const VertexFormat* vf, u32 stride, u32 mask, u32 base, u32 loc, bool offset_first = false) {
    RenderState& st = rs();
    if (vf->attrs[0].format != 0) {
        const u32 stride_bits = (stride << 8) & 0xFF00;  // rlwinm 8,16,23
        u32 i = 0;
        const VertexAttr* e;
        do {
            e = at<VertexAttr>(vf, 12 * i);
            const u32 index = e->index + e->index_add;
            const u32 shift = index & 63;  // slw
            const u32 bit = shift < 32 ? 1u << shift : 0;
            if (e->disabled == 0 && (bit & mask) != 0) {
                const u32 type = e->format;
                const u32 reg = (index << 2) & 0x3FC;  // rlwinm 2,22,29
                const u32 format = (((type >> 8) & 0xFF) << 4) | stride_bits | (type & 0xFF);
                const u32 offset = (e->offset + base) | loc;
                GcmContext* c = or_zero_page(gcm());
                const u32 cur = c->current;
                be_u32* w = reinterpret_cast<be_u32*>(mem::ptr(cur));
                w[0] = kVertexFormat0 + reg;
                w[1] = format;
                if (offset_first) {
                    w[3] = offset;
                    w[2] = kVertexOffset0 + reg;
                } else {
                    w[2] = kVertexOffset0 + reg;
                    w[3] = offset;
                }
                c->current = cur + 16;
            }
            ++i;
        } while (at<VertexAttr>(vf, 12 * i)->format != 0);
    }
    u32 stale = (st.attr_mask | mask) ^ mask;
    if (stale != 0) {
        for (s32 i = 15; i >= 0; --i) {
            const u32 bit = (1u << i) & 0xFFFF;
            if (stale & bit) {
                const u16 enabled = st.attr_mask;
                stale = (stale & ~bit) & 0xFFFF;
                st.attr_mask = static_cast<u16>(enabled & ~bit);
                emit({kVertexFormat0 + ((static_cast<u32>(i) << 2) & 0x3FC), 2});  // attribute off
            }
            if (stale == 0) break;
        }
    }
    st.attr_mask = static_cast<u16>(mask);
}

// Makes `vf` the current vertex format with the vertex data at `word`
// (bit 0 location, bits 1..31 offset; render state +16 / +8 cache them),
// enabling the attributes the current vertex program (its +10 mask) also uses.
inline void bind_format(VertexFormat* vf, u32 word, bool offset_first = false) {
    RenderState& st = rs();
    if (st.vformat.get() != vf) st.vbase_cache = -1;
    const u32 cache = static_cast<u32>(st.vbase_cache.get());
    const u32 stride = or_zero_page(vf)->count;
    st.vformat = vf;
    const u32 base = word >> 1;  // rlwinm 31,1,31
    if (cache == base) return;
    st.vbase_cache = static_cast<s32>(base);
    const u32 mask = or_zero_page(st.vprogram.get())->attr_mask & or_zero_page(vf)->mask;
    send_attributes(or_zero_page(vf), stride, mask, base, (word & 1) << 31, offset_first);
}

// Uploads the current fragment program: re-binds it first (clearing the
// dirty bit, render state +92 bit 31) when it changed, otherwise only when
// its constants are dirty (FProgram +16).
void FProgram_Upload(gp<FProgram>* current);
inline void upload_fragment() {
    RenderState& st = rs();
    const u32 dirty = st.dirty;
    if ((dirty >> 31) != 0) {
        FProgram* fp = st.fprogram;
        st.dirty = dirty & 0x7FFFFFFF;
        call(fn::cellGcmSetFragmentProgram, gcm(), at<u8>(fp, 1136), 1u);
        FProgram_Upload(&st.fprogram);
        return;
    }
    FProgram* fp = st.fprogram;
    if (fp && fp->dirty != 0) FProgram_Upload(&st.fprogram);
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
    tex = or_zero_page(tex);  // (both unchecked by the original)
    const Target* t = or_zero_page(st.colour[0].get());
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

// ---------------------------------------------------------------- materials and shaders (material.cpp, shadergroup.cpp)
using container::PtrVector;
struct Material {
    be_u32 vtable;       // +0
    be_u32 w4;           // +4 (u16 +4 written first)
    be_u32 serial;       // +8: serial << 11 | flags
    be_u32 w12;          // +12
    be_u32 w16;          // +16
    gp<u8> textures[5];  // +20: released through their vtable slot 2
    pad<4> _40;
    u8 b44;              // +44
    pad<1> _45;
    be_u16 id;           // +46
    pad<16> _48;
    be_u32 colour[4];    // +64
    be_u32 colour2[4];   // +80
    be_u32 alpha;        // +96: f32
    u8 b100;             // +100
    pad<3> _101;
    be_u32 blend;        // +104
    be_u16 blend_eq;     // +108
    be_u16 blend_dst;    // +110
    be_u32 blend2;       // +112
    be_u16 state;        // +116
    pad<2> _118;
    be_u32 w120;         // +120 (byte +121 cleared after)
    be_u32 w124;         // +124
    be_u32 w128;         // +128
};
static_assert(offsetof(Material, id) == 46 && offsetof(Material, blend) == 104 && offsetof(Material, w128) == 128);

// A shader (registry entry).
struct Shader {
    be_u32 vtable;   // +0
    pad<4> _4;
    be_u32 hash;     // +8: name hash
    pad<12> _12;
    be_u32 flags;    // +24: 2 = registered
};


// ---------------------------------------------------------------- the stencil shadow renderer (pool*.cpp)
// The shadow mesh of a caster.
struct ShadowMesh {
    gp<u8> header;           // +0: +72 nonzero = skinned
    be_u32 edges[0x2000];    // +4: bits 18..31 index-list position of the face, 4..17 the
                             // neighbour's, 2..3 / 0..1 the edge's two corners
    gp<be_f32> normals;      // +0x8004: f32[4] per face
    be_u16 edge_count;       // +0x8008
    pad<2> _800A;
    gp<be_f32> positions;    // +0x800C: f32[4] rest positions
    gp<be_f32> skinned;      // +0x8010: f32[4] skinned positions
    gp<be_u16> indices;      // +0x8014: 3 per face
    be_u16 index_count;      // +0x8018
};
static_assert(offsetof(ShadowMesh, normals) == 0x8004 && offsetof(ShadowMesh, index_count) == 0x8018);
inline bool skinned(const ShadowMesh* m) { return *at<be_u32>(m->header.get(), 72) != 0; }

// A shadow caster (the object StencilShadow slots 0 / 1 / 6 take).
struct ShadowCaster {
    gp<ShadowMesh> mesh;  // +0
    be_u16 tris;          // +4: volume triangle count (set by Build)
};

// A block of vertex memory: freed with Mem_FreePool(data, where bit 0 ? 5 : 4, tag >> 28)
// unless flags bit 0 (not owned).
struct VertexBlock {
    gp<u8> data;     // +0
    be_u32 where;    // +4: offset << 1 | location
    be_u32 flags;    // +8: bit 0 not owned
    be_u32 tag;      // +12: bits 28..31
};

// StencilShadow (layers3::g::pool, 0x010DF8B0; vtable 0x00B241E8). Slots:
// 0 AddCaps, 1 AddCapsSkinned, 2 ~, 3 delete, 4 nop, 5 Begin, 6 Build,
// 7 DrawVolume, 8 DrawQuad, 9 ResetState, 10 Refresh, 11 nop.
struct StencilShadow {
    be_u32 vtable;              // +0
    gp<be_f32> cursor;          // +4: volume triangles, 12 bytes per vertex
    be_u32 w8;                  // +8
    VertexBlock quad;           // +12: the shade quad's vertices
    pad<132> _28;
    gp<VProgram> volume_v;      // +160: StencilShadow (the volume pass)
    gp<FProgram> volume_f;      // +164
    gp<VProgram> quad_v;        // +168: StencilShadow_DrawQuad (the shade pass)
    gp<FProgram> quad_f;        // +172
    VertexFormat format;        // +176: of the volume vertices
    gp<u8> face_sign;           // +304: s8 per face, 127 * N.L (negative = facing away)
    u8 colour[4];               // +308: shade colour (RGB used)
    be_f32 intensity;           // +312
    be_f32 extrude;             // +316: extrusion distance
    u8 caps;                    // +320: z-fail with caps when set
    pad<3> _321;
    gp<be_f32> start;           // +324: cursor at the start of the current Build
    pad<24> _328;
    be_u32 w352;                // +352
    be_f32 f356;                // +356: -1.0 at construction
    be_u32 w360;                // +360
};
static_assert(offsetof(StencilShadow, format) == 176 && offsetof(StencilShadow, face_sign) == 304 &&
              offsetof(StencilShadow, start) == 324 && offsetof(StencilShadow, w360) == 360);

// An engine texture as a material holds it: the 52-byte texture bound to a
// unit sits at +56.
struct MatTexture {
    pad<56> _0;
    Texture unit;     // +56 (+72 its CellGcmTexture)
    pad<10> _108;
    be_u16 filter;    // +118: filter table indexes
    be_u16 mip;       // +120
    be_u16 wrap_s;    // +122
    be_u16 wrap_t;    // +124
};
static_assert(offsetof(MatTexture, filter) == 118 && offsetof(MatTexture, wrap_t) == 124);

// The shader object Material_Bind binds: its programs, vtable slot 8 Bind(shader, mat).
struct ShaderPrograms {
    be_u32 vtable;             // +0
    gp<VProgram> vprogram;     // +4
    gp<FProgram> fprogram;     // +8
};

// MaterialAnim track.
struct AnimTrack {
    be_f32 rate;       // +0: per second (its sign flips when ping-ponging)
    pad<4> _4;
    be_f32 period;     // +8
    be_f32 delay_lo;   // +12: delay range
    be_f32 delay_hi;   // +16
    be_f32 hold;       // +20: hold time
    pad<8> _24;
    be_u32 flags;      // +32: bits 12..19 state (0 delay, 1 run, 2 stopped); 20..27 mode (0 loop,
                       // 1 oscillate, 3 frozen); bit 28 sine oscillation
    be_f32 value;      // +36
    be_f32 countdown;  // +40
    be_f32 phase;      // +44
};
static_assert(sizeof(AnimTrack) == 48 && offsetof(AnimTrack, flags) == 32 && offsetof(AnimTrack, phase) == 44);

// A shader type (57 statics of 32 bytes from 0x01196A80). Slots: 0 Create,
// 8 ~, 12 delete, 16 PickShader(type, material, item) -> ShaderPrograms*.
struct ShaderType {
    be_u32 vtable;                      // +0
    gp<const char> name;                // +4
    be_u32 hash;                        // +8
    gp<gp<ShaderPrograms>> shaders;     // +12: PtrVector data
    be_u16 capacity;                    // +16
    pad<2> _18;
    be_u16 count;                       // +20
};

// A material's animation block (Material +128): three tracks stepped once per frame stamp.
struct MaterialAnim {
    be_u32 stamp;          // +0
    pad<4> _4;
    AnimTrack tracks[3];   // +8, +56, +104
};
// The render module's 16 occlusion query slots (vtable 0x00B240F8).
struct QueryEntry {
    be_u32 result[2];   // +0
    be_u32 report_on;   // +8: report index of the depth-test-off pass
    be_u32 report_off;  // +12: report index of the pass with the render state's depth test
    u8 state;           // +16: 1 issued, 7 done
    pad<3> _17;
};
static_assert(sizeof(QueryEntry) == 20);
struct QuerySlot {
    be_u32 vtable;          // +0
    u8 b4;                  // +4
    pad<3> _5;
    gp<u8> mesh;            // +8: the proxy mesh (+16 its material; vtable slot 16 draws)
    pad<4> _12;
    be_f32 rows[4][4];      // +16: 4 x 3 matrix, 16-byte rows
    QueryEntry entries[4];  // +80
    be_u16 current;         // +160
};
static_assert(offsetof(QuerySlot, rows) == 16 && offsetof(QuerySlot, entries) == 80 && offsetof(QuerySlot, current) == 160);

// ---------------------------------------------------------------- render layers (render_layer.cpp, layer2.cpp)
struct LayerEntry {
    be_u32 key;
    gp<DrawItem> item;
};
// {OPD fn, u32 arg, u32 view id, -}
struct LayerCallback {
    be_u32 fn;
    be_u32 arg;
    be_s32 view;
    pad<4> _12;
};

// RenderLayer (vtable 0x00B23410; base class 0x00B233E8).
// Slots: 0 Reset, 1 AddKeyed, 2 Add, 3 Sort, 4 Draw, 5 ~, 6 delete.
struct RenderLayer {
    be_u32 vtable;              // +0
    gp<LayerEntry> base;        // +4
    gp<LayerEntry> cursor;      // +8
    u8 flags;                   // +12: bit 7 skip
    u8 stencil_ref;             // +13
    pad<2> _14;
    be_u32 name;                // +16
    LayerCallback pre[4];       // +20 (indexed by a u8 count, at computed offsets)
    u8 pre_count;               // +84
    pad<3> _85;
    LayerCallback post[4];      // +88
    u8 post_count;              // +152
    pad<3> _153;
    be_u32 capacity;            // +156
    be_u32 high;                // +160: high-water count
    be_u32 bytes;               // +164: capacity * 8
    u8 colour[4];               // +168
};
static_assert(sizeof(RenderLayer) == 172);
// The 192-byte resolve layers (WaterResolve, ScreenWarp): four floats
// ResolveLayer_Pass1 sends as vertex constant 4.
struct ResolveLayer {
    RenderLayer layer;   // +0
    pad<4> _172;
    be_u32 consts[4];    // +176: 0, 0, 0, 1.0f at construction
};
static_assert(sizeof(ResolveLayer) == 192);
static_assert(offsetof(RenderLayer, pre_count) == 84 && offsetof(RenderLayer, post_count) == 152 &&
              offsetof(RenderLayer, high) == 160);

constexpr u32 kMemFreeFrom = 0x0054EE6C;  // Mem_FreeFrom(p, pool)

// The destructor body every layer class shares: entry buffer freed and
// cleared, base vtable 0x00B233E8.
inline void layer_destruct(RenderLayer* layer) {
    const u32 buf = layer->base.ea;
    layer->vtable = 0x00B233E8u;
    if (buf == 0) return;
    call(kMemFreeFrom, buf, 0u);
    layer->base = nullptr;
}

// Runs the callbacks of a list (at byte offset `list`; count byte re-read
// after each call) whose view id matches the current view's +48 byte; the
// state caches are reset after each. (The original leaves the OPD / entry
// words in r4 / r5; not read.)
inline void run_layer_callbacks(RenderLayer* layer, u32 list, const u8& count_ref) {
    constexpr u32 kCurrentSurface = 0x010DDA74;  // object whose +672 is the current view (+48 u8 id)
    constexpr u32 kStateCacheA = 0x010DD598, kStateCacheB = 0x010DD5A0;  // u64, reset to -1
    u32 count = count_ref;
    if (static_cast<s32>(count) <= 0) return;
    for (u32 i = 0; i < count; i = (i + 1) & 0xFF) {
        const LayerCallback* cb = at<LayerCallback>(layer, list + 16 * i);
        const u32 fn = cb->fn;
        if (fn == 0) continue;
        const u8* view = *at<gp<u8>>(global<gp<u8>>(kCurrentSurface).get(), 672);
        if (static_cast<s32>(*at<u8>(view, 48)) != cb->view) continue;
        call_ptr(fn, cb->arg.get());
        global<be_u64>(kStateCacheA) = ~u64{0};
        global<be_u64>(kStateCacheB) = ~u64{0};
        count = count_ref;
    }
}

// The draw binding (0x010DC840).
struct DrawBinding {
    gp<Material> material;        // +0
    gp<ShaderPrograms> shader;    // +4
    be_u32 w8;                    // +8 (cleared with them)
};

// Uploads transpose(item rows * view_proj) as vertex constants 0..3 (one
// SET_TRANSFORM_CONSTANT_LOAD of 17 words): row i of the product is
// item[i].x * M0 + item[i].y * M1 + item[i].z * M2 (+ M3 for row 3),
// fused per lane in vmaddfp order; the item's rows are read with lvx
// (address rounded down to 16).
inline void upload_item_matrix(const void* item) {
    constexpr u32 kViewProj = 0x010DD7C0;       // f32[4][4] rows (layers4::g::view_proj)
    constexpr u32 kVertexConst17 = 0x00441EFC;  // SET_TRANSFORM_CONSTANT_LOAD, 17 words
    const be_f32* m = host<be_f32>(kViewProj);
    const be_f32* rows = aligned16(item);
    f32 r[4][4];
    for (u32 i = 0; i < 4; ++i) {
        const be_f32* row = rows + 4 * i;
        const f32 x = row[0], y = row[1], z = row[2];
        for (u32 k = 0; k < 4; ++k) {
            f32 v = std::fmaf(m[k], x, i == 3 ? static_cast<f32>(m[12 + k]) : 0.0f);
            v = std::fmaf(m[4 + k], y, v);
            r[i][k] = std::fmaf(m[8 + k], z, v);
        }
    }
    GcmContext* c = or_zero_page(gcm());
    const u32 cur = c->current;
    c->current = cur + 72;
    be_u32* w = reinterpret_cast<be_u32*>(mem::ptr(cur));
    w[0] = kVertexConst17;
    w[1] = 0u;  // first constant
    for (u32 k = 0; k < 4; ++k)
        for (u32 i = 0; i < 4; ++i) reinterpret_cast<be_f32*>(w + 2)[4 * k + i] = r[i][k];  // stvlx / stvrx pairs
}

// The lwarx / stwcx. loop of the original: an atomic ++; returns the old value.
inline u32 fetch_inc(be_u32& v) {
    const u32 ea = guest(&v);
    for (;;) {
        const u32 old = mem::r32(ea);
        if (mem::cas32(ea, old, old + 1)) return old;
    }
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
void Tiles_Unbind();
u32 Surface_GetSize(u32 width, u32 height, s32 format, s32 aa, bool tiled, be_u32* pitch_out);
void Surface_Create(gp<Target>* out, gp<Target>* src, u32 w, u32 h, u32 unused, u32 skip, u32 unused2);
void Target_Init(gp<Target>* out, u32 w, u32 h, s32 format, s32 aa, u32 where);
void Target_Attach(gp<Target>* out, gp<Target>* src, u32 w, u32 h, u32 unused, s32 row);
void Surface_SetBuffer(Texture* s, Target* t);
Target* Target_Construct(Target* t);  // surface_alloc.cpp
void Target_InitDepth(gp<Target>* out, u32 w, u32 h, s32 format, s32 aa);
Display* RenderSurface_Alloc();
// display.cpp
void Display_CreateTargets(u32 w, u32 h, u32 flags, u32 max_w, u32 max_h);
void Display_Finish(Display* d);
void Display_SetSize(Display* d, gp<Target>* frame);
// material.cpp, shadergroup.cpp
void Material_Construct(Material* m);
bool Shader_Register(Shader* s);
Material* Shader_LoadGroup(s32 group);
Shader* Shader_FindByHash(u32 hash);
Material* Material_Alloc();
// effect_init.cpp
void ThermalVision_Init();
void Bloom_Init();
void Dof_Init();
void Effect_InitNone();
void Monochrome_Init();
void Warp_Init();
void GodRays_Init();
void MotionTrail_Init();
void BloomNoise_Init();
void GodRays_Load();
void MotionTrail_Load();
// pool.cpp
void Draw_Clear(u32 mask, u32 colour, f32 depth, u32 stencil, u32 stencil_mask);
void DrawItem_GetScale(const be_f32* m, be_f32* out);
void View_ProjectSphere(const be_f32* sphere, const be_f32* m, const be_f32* proj, be_f32* x0, be_f32* y0, be_f32* x1,
                        be_f32* y1);
void MaterialAnim_SetState(AnimTrack* t, s32 state);
void MaterialAnim_Update(AnimTrack* t, f32 dt);
// pool_shadow.cpp
void StencilShadow_AddCaps(StencilShadow* p, ShadowCaster* o, const be_f32* light);
void StencilShadow_FaceSign(StencilShadow* p, u32 index, const be_u16* idx, const be_f32* pos, const be_f32* light);
void StencilShadow_AddCapsSkinned(StencilShadow* p, ShadowCaster* o, const be_f32* light);
void StencilShadow_Build(StencilShadow* p, ShadowCaster* o, const be_f32* light, u8 caps);
// pool_draw.cpp
void StencilShadow_Begin(StencilShadow* p);
void StencilShadow_DrawVolume(StencilShadow* p, ShadowCaster* o, const be_f32* matrix, const be_f32* buf);
void StencilShadow_DrawQuad(StencilShadow* p, s32 blend, const be_f32* colour);
void Draw_InlineArray(u8 mode, u32 count, const void* data, const VertexFormat* vf);
void DrawItem_DrawDepth(DrawMesh* mesh, DrawItem* item);
// layers4_material.cpp
void Material_Bind(Material* mat, ShaderPrograms* shader);
// draw3.cpp
void DrawItem_DrawDepthSkinned(DrawMesh* mesh, u32 skin_table, DrawItem* item, const be_f32* bones, void* unused, u32 flags);
void Slot_DrawProxy(QuerySlot* s);
void Render_Init();
// layer2.cpp
void DrawItem_Draw(DrawItem* item);
void RenderLayer_End();
void RenderLayer_RadixSort(RenderLayer* l);
void SortEntries(LayerEntry* e, s32 count, u32 size);
void RenderLayers_Destroy();
// layers4.cpp
void SlotBase_Construct(QuerySlot* s);
void Slot_Construct(QuerySlot* s);
void Pool_Construct(StencilShadow* p);
void Pool_DestructPart(VertexBlock* part);
void Pool_Destruct(StencilShadow* p);
void Pool_Delete(StencilShadow* p);
void Pool_Nop4();
void Pool_Nop11();
void Pool_Refresh(StencilShadow* p);
void Draw_ApplyView();
void Pool_ResetState(StencilShadow* p);
// layers4_draw.cpp
void ZLayer_DrawBody(RenderLayer* l);
void ResolveLayer_Pass1(RenderLayer* l);
void ResolveLayer_Pass2(RenderLayer* l);
Texture* Stats_ResolveTexture(MemStats* s);
Target* Display_SelectTarget(const void* table);
void Draw_CopyRect(Texture* dst, Target* src, u32 sx, u32 sy, u32 dx, u32 dy, u32 w, u32 h);
Texture* Draw_CopyScreen();
void ResolveLayer_ItemRect(RenderLayer* l, DrawItem* item, be_f32* x0, be_f32* y0, be_f32* x1, be_f32* y1);
// layers3.cpp
void RenderLayers_Register();
void RenderLayers_Construct();
void RenderLayerDepth_Add(RenderLayer* l, DrawItem* item, u32 extra);
void RenderLayerZ_Add(RenderLayer* l, DrawItem* item, u32 extra);
void RenderLayerZ_Draw(RenderLayer* l);
void RenderLayerResolve_Draw(RenderLayer* l);
// shader2_bind.cpp
void SlyCharacter2_Bind(ShaderPrograms* s, Material* mat);
// shader3.cpp
void Draw_SetParticleState(u32 fog, u8 enable, u32 fconst);
void DrawItem_SetParticleState(DrawItem* item, u32 fconst);
void MaterialAnim_UploadUV(u32 anim, u32 first, f32 su, f32 sv);
void MaterialAnim_UploadUVAlt(u32 anim, u32 first, f32 su, f32 sv);
void FProgram_SetConstant(u32 index, const be_u32* v);
Texture* Draw_GetScreenCopy();
Texture* Display_GetDepthTexture(MemStats* s);
void TextureWarp_BuildMatrices(Material* mat, be_f32* m1, be_f32* m2, be_u32* v);
void SlyTextureWarp_BuildMatrices(Material* mat, be_f32* m1, be_f32* m2, be_u32* v);
void SlyHeightFog_UploadFog(Material* mat);
void SlyWater2_UploadWaves(Material* mat);
void SlyWater3_UploadWaves(Material* mat);
DrawItem* DrawItem_Create(DrawMesh* mesh, Material* mat, const u8* d);
void Skin_BuildJob(u8* job, const be_u32* desc, const u8* part, u32 bones, u32 flags);
// setup.cpp
void Video_SelectMode();
bool Video_Configure(u8 mode, bool unbind, bool rebind);
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

// The UV animation block as MaterialAnim_GetUVAlt reads it.
struct UVAnim {
    pad<4> _0;
    u8 wrap;           // +4
    pad<7> _5;
    be_f32 u_steps;    // +12
    pad<28> _16;
    be_f32 u_offset;   // +44
    pad<12> _48;
    be_f32 v_steps;    // +60
    pad<28> _64;
    be_f32 v_offset;   // +92
};
static_assert(offsetof(UVAnim, u_steps) == 12 && offsetof(UVAnim, u_offset) == 44 && offsetof(UVAnim, v_steps) == 60 &&
              offsetof(UVAnim, v_offset) == 92);

// shader4.cpp
void Shader_Construct(ShaderPrograms* s);
void Shader_Load(ShaderPrograms* s, const char* name, const char* fname, const Binding* bindings);
void Shader_SetFpControl(ShaderPrograms* s, u32 bits);
be_f32* MaterialAnim_GetUV(u8* anim, f32 su, f32 sv);
void MaterialAnim_GetUVAlt(UVAnim* anim, be_f32* out);
void Vec3_ToItemSpace(be_f32* v, const be_f32* m);
void Vec3_RotateTo(be_f32* out, const be_f32* v, const be_f32* m);
// shader4_draw.cpp, shader4_frame.cpp, shader4_setup.cpp
void Matrix_RotateRows(be_f32* out, const be_f32* rows, const be_f32* m);
void View_BeginFrame(u8* scene);
void SlyCharacter2_SetupDraw(ShaderPrograms* s, DrawItem* item);
// shader5_util.cpp
struct MatrixAnim;
be_f32* MaterialAnim_GetMatrix(MatrixAnim* a);
void Matrix_Perspective(be_f32* out, f32 fov, f32 aspect, f32 near, f32 far);
void ProjTxr_BuildProjection(be_f32* pos_out, be_f32* m_out, const be_f32* eye, const be_f32* target, const be_f32* up,
                             const be_f32* proj, const be_f32* item, const be_f32* block);
void SelfLight_GetLight(ShaderPrograms* s, DrawItem* item, be_u32* a, be_u32* dir, be_u32* b);
void View_SetConstants(u8* view);
// shader6_util.cpp
struct SortedMap;
be_u32* Material_ParamSlot(Material* mat, u32 index);
be_u32* Map_Find3(SortedMap* map, const be_u32* key);
be_f32* Matrix_MultiplyTo(be_f32* out, const be_f32* a, const be_f32* b);
void SlyWater3_UploadFragment(Material* mat);
void SlyHeightFog_BuildMatrices(Material* mat, be_f32* a, be_f32* b);
// gfx7_math.cpp
void Matrix_SplitScale(const be_f32* m, be_f32* rot, be_f32* scale);
void Matrix_RotationZ(be_f32* out, f32 a);
u32 Fur_ShellCount(ShaderPrograms* s, DrawItem* item, be_f32* fade_out);

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
