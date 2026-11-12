// Display buffer selection, the depth target, the render-state cache
// reset and the vertex format table.
#include "rsx.h"

namespace sly4::gfx {

namespace {

constexpr u32 kStateCache = 0x010DDBB4;    // StateCache*: object whose cached state Draw_InvalidateState resets (name?)
constexpr u32 kStateCacheA = 0x010DD598;   // u64, reset to -1 (name?)
constexpr u32 kStateCacheB = 0x010DD5A0;   // u64, reset to -1 (name?)
constexpr u32 kVertexFormats = 0x010DC740;  // VertexFormat[2]

// The state another renderer object caches (fields named by their reset value only).
struct StateCache {
    pad<80> _0;
    be_u32 w80;    // +80
    be_u32 w84;    // +84
    be_u32 w88;    // +88
    pad<4> _92;
    be_u32 w96;    // +96: 100000.0f
    pad<60> _100;
    be_u32 w160;   // +160
    be_u16 h164;   // +164
    pad<10> _166;
    be_u64 d176;   // +176
    be_u64 d184;   // +184
    be_u32 w192;   // +192
    be_u32 w196;   // +196: -1.0f
    u8 b200;       // +200
};
static_assert(offsetof(StateCache, w160) == 160 && offsetof(StateCache, d176) == 176 && offsetof(StateCache, b200) == 200);

}  // namespace

// void Display_GetTargets(Target** colour[1], Target** depth[1])
// Stores the address of the display's colour target slot and depth
// target slot for the current eye / buffer set (the choice
// display_depth makes for the depth one). Returns the depth slot.
gp<Target>* Display_GetTargets(gp<gp<Target>>* colour_out, gp<gp<Target>>* depth_out) {
    Display* disp = display();
    const u32 k = eye_offset(disp);
    gp<Target>* colour;
    if (!per_eye(disp)) {
        colour = at<gp<Target>>(disp, k + 16);
    } else if (config_u8(35) == 0 || config_u8(34) == 0) {
        colour = alternate(disp) ? &disp->alt_colour : at<gp<Target>>(disp, k + 64);
    } else {
        colour = at<gp<Target>>(disp, k + (mem_stats().buffer_select == 1 ? 100 : 108));
    }
    *colour_out = colour;

    gp<Target>* depth;
    if (!per_eye(disp)) {
        depth = &disp->depth;
    } else if (config_u8(35) == 0 || config_u8(34) == 0) {
        depth = alternate(disp) ? &disp->alt_depth : &disp->eye_depth;
    } else {
        depth = &disp->stereo_depth[mem_stats().buffer_select == 1 ? 0 : 1];
    }
    *depth_out = depth;
    return depth;
}

// Target* Display_SelectBuffer(Target* const* table)
// The entry of a per-buffer table laid out like g::mem_stats (+128 for
// the alternate set, else +92 / +112 / +120 per eye), as scene_colour
// picks it.
Texture* Display_SelectBuffer(const void* table) {
    const Display* disp = display();
    u32 off;
    if (alternate(disp)) off = 128;
    else if (config_u8(34) != 0) off = eye_offset(disp) + (mem_stats().buffer_select == 1 ? 112 : 120);
    else off = eye_offset(disp) + 92;
    return *at<gp<Texture>>(table, off);
}

// void Draw_SetDepthTarget(Target* d)
// Makes d the depth target (0 = none) unless it already is.
void Draw_SetDepthTarget(Target* d) {
    if (rs().depth.get() == d) return;
    set_depth_target(d);
}

// void Draw_InvalidateState()
// Forgets everything the render state caches (programs, textures, vertex
// attributes, blend / depth / stencil words) so the next draws resend it,
// and resets the state cached in the object at kStateCache.
void Draw_InvalidateState() {
    RenderState& st = rs();
    constexpr u32 kAll = 0xFFFFFFFFu;
    constexpr u32 kFunc = 519;     // 0x207
    constexpr u32 kKeep = 0x1E00;  // 7680
    st.index_buffer = 0u;
    st.vbase_cache = -1;
    st.vformat = nullptr;
    st.vprogram = nullptr;
    st.vcg = 0u;
    st.fprogram = nullptr;
    st.attr_mask = 0xFFFF;
    st.unk88 = 0u;
    for (u32 i = 0; i < 8; ++i) st.texture[i] = nullptr;
    st.unk216 = kAll;
    st.unk220 = 0;
    st.unk221 = 0;
    st.stencil_func = kFunc;
    st.stencil_ref = 0u;
    st.stencil_mask = kAll;
    st.stencil_func_mask = kAll;
    const u32 dirty = st.dirty & 0x7FFFFFFFu;
    st.stencil_op[0] = kKeep;
    st.stencil_op[1] = kKeep;
    st.stencil_op[2] = kKeep;
    st.back_func = kFunc;
    st.back_ref = 0u;
    st.back_mask = kAll;
    StateCache* obj = global<gp<StateCache>>(kStateCache);
    st.back_func_mask = kAll;
    st.back_op[0] = kKeep;
    st.back_op[1] = kKeep;
    st.back_op[2] = kKeep;
    global<be_u64>(kStateCacheA) = ~u64{0};
    global<be_u64>(kStateCacheB) = ~u64{0};
    st.dirty = dirty;  // fragment-program-changed bit cleared
    obj->w80 = kAll;
    obj->w84 = kAll;
    obj->w88 = kAll;
    obj->w96 = 0x47C35000u;  // 100000.0f
    obj->w160 = kAll;
    obj->d176 = ~u64{0};
    obj->h164 = 0xFFFF;
    obj->d184 = ~u64{0};
    obj->w192 = kAll;
    obj->w196 = 0xBF800000u;  // -1.0f
    obj->b200 = 0;
}

// const VertexFormat* VertexFormat_Get(bool second)
// One of the two 128-byte vertex formats at kVertexFormats.
const VertexFormat* VertexFormat_Get(bool second) { return host<VertexFormat>(kVertexFormats) + (second ? 1 : 0); }

}  // namespace sly4::gfx

NATIVE(005356F4, sly4::gfx::Display_GetTargets)
NATIVE(00537C18, sly4::gfx::Display_SelectBuffer)
NATIVE(0053755C, sly4::gfx::Draw_SetDepthTarget)
NATIVE(0051279C, sly4::gfx::Draw_InvalidateState)
NATIVE(00515750, sly4::gfx::VertexFormat_Get)
