// Render layer table set-up: RenderLayers_Construct (the static
// constructor of the 17 render layers and of the render module statics
// around them) and RenderLayers_Register (allocates each layer's entry
// buffer and appends it to render::g::layers); the derived layer classes'
// slots.
#include "layers3.h"
#include "rsx.h"

#include <atomic>

namespace sly4::gfx {

namespace {

namespace rg = sly4::render::g;
namespace l3 = sly4::render::layers3;
namespace lat = sly4::render::layers3::at;
namespace nm = sly4::render::layers3::name;
namespace g3 = sly4::render::layers3::g;

constexpr u32 kLayerVtable = 0x00B23410, kLayerBaseVtable = 0x00B233E8;
constexpr u32 kCurrentSurface = 0x010DDA74;  // object whose +672 is the current view

constexpr u32 kOne = 0x3F800000, kMinusOne = 0xBF800000;  // f32 bit patterns
constexpr u32 kTwo = 0x40000000, kMinusTwo = 0xC0000000, kHalf = 0x3F000000, kMinusHalf = 0xBF000000;

RenderLayer* layer(u32 ea) { return host<RenderLayer>(ea); }
ResolveLayer* resolve(u32 ea) { return host<ResolveLayer>(ea); }

// ---------------------------------------------------------------- RenderLayers_Register

// One registration block. The original repeats it 17 times inline; the
// order of its stores differs only in the two flagged spots.
enum : u8 {
    kSwapColour01 = 1,  // colour byte 1 is stored before byte 0
    kMaxFirst = 2,      // g::max_capacity is stored before the stencil ref
};
struct Registration {
    u32 layer;
    u32 cfg;     // offset of the s16 capacity in sly4::g::config
    u32 colour;  // u8[4] copied to +168
    u8 stencil_ref;
    u8 order;
};
// Quirks kept: PostShadow2 reads PostShadow1's capacity (+448) and
// Occluder reads Opaque's (+428), so +454 / +456 belong to the last two.
constexpr Registration kRegistrations[l3::kCount] = {
    {lat::Opaque, 428, g3::colour_cac, 255, 0},
    {lat::Panorama, 430, g3::colour_ca0, 0, 0},
    {lat::PreShadow, 432, g3::colour_cb0, 0, 0},
    {lat::Translucent0, 434, g3::colour_cb8, 32, 0},
    {lat::Translucent1, 436, g3::colour_cb8, 0, kSwapColour01},
    {lat::Translucent2, 438, g3::colour_cb8, 0, 0},
    {lat::ZTranslucent, 440, g3::colour_cc8, 0, 0},
    {lat::WaterResolve, 442, g3::colour_cb8, 0, kSwapColour01},
    {lat::ScreenWarp, 444, g3::colour_cb8, 0, 0},
    {lat::Effects1, 446, g3::colour_cb8, 0, kSwapColour01},
    {lat::PostShadow1, 448, g3::colour_cb4, 0, kMaxFirst},
    {lat::PostShadow2, 448, g3::colour_cb4, 0, 0},
    {lat::UI, 450, g3::colour_ca0, 0, kSwapColour01},
    {lat::PostPrim, 452, g3::colour_ca0, 0, 0},
    {lat::Occluder, 428, g3::colour_ca0, 0, kSwapColour01},
    {lat::MotionTrail, 454, g3::colour_ca0, 0, 0},
    {lat::ThermalVision, 456, g3::colour_ca0, 0, kMaxFirst},
};
constexpr u32 kDefaultCapacity = 2048;
constexpr u32 kEntrySize = 8, kEntryAlign = 16;

// The layer table (render::g::layers): a count and the layers.
struct LayerTable {
    be_u32 count;              // +0
    gp<RenderLayer> layers[1];  // +4 (indexed past 1)
};

// ---------------------------------------------------------------- RenderLayers_Construct

// The lwarx / stwcx. loop in the original: an atomic store of 0 (the old
// value is dropped).
void atomic_clear(u32 ea) { std::atomic_ref<u32>(*reinterpret_cast<u32*>(mem::ptr(ea))).store(0); }

void vtable(u32 l, u32 vt) { layer(l)->vtable = vt; }

// The inlined RenderLayer constructor body after the base vtable: no
// entry buffer, stencil ref 0, the name, flags bits 7 and 6 cleared (read
// before any store), and both callback lists cleared ({fn, arg, view} and
// the byte at +12 of each 16-byte record; the count bytes +84 / +152 are
// left alone). Translucent0..2 store the flags before the name.
void init(u32 ea, u32 name, bool flags_first = false) {
    RenderLayer* l = layer(ea);
    const u8 flags = static_cast<u8>(l->flags & l3::kFlagsKeep);
    l->base = nullptr;
    l->cursor = nullptr;
    l->stencil_ref = 0;
    if (flags_first) {
        l->flags = flags;
        l->name = name;
    } else {
        l->name = name;
        l->flags = flags;
    }
    for (LayerCallback* list : {l->pre, l->post}) {
        for (u32 i = 0; i < l3::kCallbacks; ++i) {
            LayerCallback& cb = list[i];
            cb.fn = 0u;
            cb.arg = 0u;
            cb.view = 0;
            *at<u8>(&cb, 12) = 0;
        }
    }
}

// Capacity, high-water count and byte size, all 0.
void zero_counts(u32 ea) {
    RenderLayer* l = layer(ea);
    l->capacity = 0u;
    l->high = 0u;
    l->bytes = 0u;
}

struct Colour {
    u8 c[4];
};
void colour_byte(u32 l, const Colour& c, u32 k) { layer(l)->colour[k] = c.c[k]; }
void colour(u32 l, const Colour& c) {
    for (u32 k = 0; k < 4; ++k) colour_byte(l, c, k);
}

// Stores the 4x4 rows {a, b, c, d}; `w` writes the fourth column too.
void matrix(u32 m, const u32 (&v)[4][4], bool w) {
    be_u32* rows = host<be_u32>(m);
    for (u32 r = 0; r < 4; ++r)
        for (u32 k = 0; k < (w ? 4u : 3u); ++k) rows[4 * r + k] = v[r][k];
}

}  // namespace

// void RenderLayers_Register()   (name?)
// For each of the 17 layers, in g::layers order: append it to
// render::g::layers, take its capacity from the s16 in g::config
// (2048 when 0; a negative value is used as is, sign-extended), allocate
// capacity * 8 bytes of entries (Mem_Alloc(size, 0, 16); +4 and +8 both
// get the result, 0 included), copy its colour to +168, set its stencil
// ref (+13; 255 for Opaque, 32 for Translucent0) and raise
// g::max_capacity (unsigned) to its capacity.
void RenderLayers_Register() {
    LayerTable& table = global<LayerTable>(rg::layers);
    be_u32& max_capacity = global<be_u32>(g3::max_capacity);
    for (const Registration& r : kRegistrations) {
        RenderLayer* l = layer(r.layer);
        const u32 n = table.count;
        table.count = n + 1;
        *at<gp<RenderLayer>>(&table, 4 + 4 * n) = l;
        const s16 cfg = static_cast<s16>(global<be_u16>(sly4::g::config + r.cfg).get());
        const u32 cap = cfg != 0 ? static_cast<u32>(static_cast<s32>(cfg)) : kDefaultCapacity;
        l->capacity = cap;
        l->bytes = cap * kEntrySize;
        LayerEntry* entries = call<LayerEntry*>(sly4::fn::Mem_Alloc, cap * kEntrySize, 0u, kEntryAlign);
        l->base = entries;
        l->cursor = entries;
        const auto copy = [&](u32 k) { l->colour[k] = global<u8>(r.colour + k); };
        if (r.order & kSwapColour01) {
            copy(1);
            copy(0);
        } else {
            copy(0);
            copy(1);
        }
        copy(2);
        copy(3);
        const u32 prev = max_capacity;  // read after the slot store above
        const u32 high = prev > cap ? prev : cap;
        if (r.order & kMaxFirst) {
            max_capacity = high;
            l->stencil_ref = r.stencil_ref;
        } else {
            l->stencil_ref = r.stencil_ref;
            max_capacity = high;
        }
    }
}

// void RenderLayers_Construct()   (static constructor; name?)
// Initialises the render module statics: a few flags and blocks, the
// start-up config (whose +24..+30 are zeroed just before the memset that
// clears them again), the 17 render layers (base vtable, the constructor
// body, then the derived vtable; their colour is the 0x01164CA0 colour
// until RenderLayers_Register sets it), the 16 slots, the pool and its
// vector, and two constant matrices. The layer constructors are inlined
// and interleaved; the stores below keep the original's order.
void RenderLayers_Construct() {
    Config& cfg = config();
    global<u8>(g3::byte_da70) = 0xFF;
    cfg.safe_x = 0;
    cfg.safe_y = 0;
    cfg.safe_w = 0;
    cfg.safe_h = 0;
    call(sly4::fn::memset, &cfg, 0u, l3::kConfigSize);
    *at<be_u32>(&cfg, 320) = kOne;
    *at<be_u32>(&cfg, 60) = kOne;
    be_u32* e061c = host<be_u32>(g3::block_e061c);
    be_u32* dd5a8 = host<be_u32>(g3::block_dd5a8);
    e061c[0] = 0u;
    dd5a8[1] = 0u;
    dd5a8[2] = 0u;
    dd5a8[3] = kOne;
    dd5a8[4] = kOne;
    dd5a8[5] = kOne;
    e061c[1] = 32u;
    atomic_clear(g3::atomic_ddc00);
    global<be_u64>(g3::block_ddb88 + 24) = 0;
    global<u8>(g3::block_ddb88 + 32) = 0;
    call(sly4::fn::memset, g3::block_dd600, 0u, l3::kBlockDd600Size);
    global<be_u32>(g3::block_dd600 + 256) = g3::block_dd600 + 128;

    constexpr u32 base = kLayerBaseVtable;
    // Opaque (its colour is read once and reused for every layer)
    vtable(lat::Opaque, base);
    init(lat::Opaque, nm::Opaque);
    const Colour c = {{global<u8>(g3::colour_ca0), global<u8>(g3::colour_ca0 + 1), global<u8>(g3::colour_ca0 + 2),
                       global<u8>(g3::colour_ca0 + 3)}};
    zero_counts(lat::Opaque);
    colour_byte(lat::Opaque, c, 0);
    vtable(lat::Opaque, kLayerVtable);
    vtable(lat::Panorama, base);
    colour_byte(lat::Opaque, c, 1);
    colour_byte(lat::Opaque, c, 2);
    colour_byte(lat::Opaque, c, 3);
    // Panorama
    init(lat::Panorama, nm::Panorama);
    layer(lat::Panorama)->capacity = 0u;
    layer(lat::Panorama)->high = 0u;
    vtable(lat::Panorama, l3::kVtableDepth);
    vtable(lat::PreShadow, base);
    layer(lat::Panorama)->bytes = 0u;
    colour(lat::Panorama, c);
    // PreShadow
    init(lat::PreShadow, nm::PreShadow);
    vtable(lat::PreShadow, l3::kVtableDepth);
    vtable(lat::Translucent0, base);
    zero_counts(lat::PreShadow);
    colour(lat::PreShadow, c);
    // Translucent0
    init(lat::Translucent0, nm::Translucent0, true);
    vtable(lat::Translucent0, l3::kVtableDepth);
    vtable(lat::Translucent1, base);
    zero_counts(lat::Translucent0);
    colour(lat::Translucent0, c);
    // Translucent1
    init(lat::Translucent1, nm::Translucent1, true);
    vtable(lat::Translucent1, l3::kVtableDepth);
    vtable(lat::Translucent2, base);
    zero_counts(lat::Translucent1);
    colour(lat::Translucent1, c);
    // Translucent2
    init(lat::Translucent2, nm::Translucent2, true);
    zero_counts(lat::Translucent2);
    colour(lat::Translucent2, c);
    vtable(lat::Translucent2, l3::kVtableDepth);
    vtable(lat::ZTranslucent, base);
    // ZTranslucent
    init(lat::ZTranslucent, nm::ZTranslucent);
    zero_counts(lat::ZTranslucent);
    colour(lat::ZTranslucent, c);
    vtable(lat::ZTranslucent, l3::kVtableZ);
    vtable(lat::WaterResolve, base);
    // WaterResolve (192 bytes)
    {
        const u32 l = lat::WaterResolve;
        init(l, nm::WaterResolve);
        resolve(l)->consts[0] = 0u;
        layer(l)->capacity = 0u;
        vtable(l, l3::kVtableResolve);
        resolve(l)->consts[1] = 0u;
        layer(l)->high = 0u;
        resolve(l)->consts[2] = 0u;
        layer(l)->bytes = 0u;
        colour_byte(l, c, 0);
        resolve(l)->consts[3] = kOne;
        colour_byte(l, c, 1);
        colour_byte(l, c, 2);
        vtable(lat::ScreenWarp, base);
        colour_byte(l, c, 3);
    }
    // ScreenWarp (192 bytes)
    {
        const u32 l = lat::ScreenWarp;
        init(l, nm::ScreenWarp);
        vtable(l, l3::kVtableResolve);
        resolve(l)->consts[0] = 0u;
        resolve(l)->consts[1] = 0u;
        resolve(l)->consts[2] = 0u;
        layer(l)->capacity = 0u;
        layer(l)->high = 0u;
        resolve(l)->consts[3] = kOne;
        layer(l)->bytes = 0u;
        colour_byte(l, c, 0);
        vtable(lat::Effects1, base);
        colour_byte(l, c, 1);
        colour_byte(l, c, 2);
        colour_byte(l, c, 3);
    }
    // Effects1
    init(lat::Effects1, nm::Effects1);
    zero_counts(lat::Effects1);
    colour(lat::Effects1, c);
    vtable(lat::Effects1, l3::kVtableDepth);
    vtable(lat::PostShadow1, base);
    // PostShadow1
    init(lat::PostShadow1, nm::PostShadow1);
    vtable(lat::PostShadow1, kLayerVtable);
    vtable(lat::PostShadow2, base);
    zero_counts(lat::PostShadow1);
    colour(lat::PostShadow1, c);
    // PostShadow2
    init(lat::PostShadow2, nm::PostShadow2);
    vtable(lat::PostShadow2, kLayerVtable);
    vtable(lat::UI, base);
    zero_counts(lat::PostShadow2);
    colour(lat::PostShadow2, c);
    // UI
    init(lat::UI, nm::UI);
    layer(lat::UI)->capacity = 0u;
    vtable(lat::UI, l3::kVtableUI);
    layer(lat::UI)->high = 0u;
    vtable(lat::PostPrim, base);
    layer(lat::UI)->bytes = 0u;
    colour(lat::UI, c);
    // PostPrim
    init(lat::PostPrim, nm::PostPrim);
    vtable(lat::PostPrim, kLayerVtable);
    vtable(lat::Occluder, base);
    zero_counts(lat::PostPrim);
    colour(lat::PostPrim, c);
    // Occluder
    init(lat::Occluder, nm::Occluder);
    vtable(lat::Occluder, kLayerVtable);
    vtable(lat::MotionTrail, base);
    zero_counts(lat::Occluder);
    colour(lat::Occluder, c);
    // MotionTrail
    init(lat::MotionTrail, nm::MotionTrail);
    vtable(lat::MotionTrail, kLayerVtable);
    vtable(lat::ThermalVision, base);
    zero_counts(lat::MotionTrail);
    colour(lat::MotionTrail, c);
    // ThermalVision
    {
        const u32 l = lat::ThermalVision;
        init(l, nm::ThermalVision);
        layer(l)->capacity = 0u;
        layer(l)->high = 0u;
        colour_byte(l, c, 0);
        layer(l)->bytes = 0u;
        colour_byte(l, c, 1);
        colour_byte(l, c, 2);
        colour_byte(l, c, 3);
        vtable(l, kLayerVtable);
    }

    atomic_clear(g3::atomic_dd400);
    global<be_u32>(g3::block_dd780 + 608) = 0u;
    global<u8>(rg::stencil_bytes) = 0;
    global<u8>(rg::stencil_bytes + 1) = 0;
    for (u32 i = 0; i < l3::kSlotCount; ++i) Slot_Construct(host<QuerySlot>(g3::slots + l3::kSlotSize * i));
    StencilShadow& pool = global<StencilShadow>(g3::pool);
    Pool_Construct(&pool);
    pool.w352 = 0u;
    pool.vtable = l3::kPoolVtable;
    *reinterpret_cast<be_u32*>(&pool.f356) = kMinusOne;
    pool.w360 = 0u;
    PtrVector* vec = host<PtrVector>(g3::pool_vector);
    call(l3::fn::PtrVector_Construct, vec);
    vec->elem_size = 4;
    container::PtrVector_Init(vec, 0, nullptr, 0);

    constexpr u32 uv_to_clip[4][4] = {
        {kTwo, 0, 0, 0}, {0, kMinusTwo, 0, 0}, {0, 0, kOne, 0}, {kMinusOne, kOne, 0, 0}};
    constexpr u32 clip_to_uv[4][4] = {
        {kHalf, 0, 0, 0}, {0, kMinusHalf, 0, 0}, {0, 0, kOne, 0}, {kHalf, kHalf, 0, kOne}};
    matrix(g3::uv_to_clip, uv_to_clip, false);
    matrix(g3::clip_to_uv, clip_to_uv, true);
}

// ---------------------------------------------------------------- derived layer slots

namespace {

void destroy(RenderLayer* l) {
    if (!l) return;
    layer_destruct(l);
    call(kMemFreeFrom, l, 0u);
}

// The view-depth Add: the item position (+48) through g::view_matrix
// (vmaddfp per lane: fmaf(row0, x, 0), fmaf(row1, y, .), fmaf(row2, z, .),
// + row3), then depth = ((s8)(extra >> 8) * c0 + view z - c1) * c2,
// truncated, and key = 0xFFFFFF - depth (far first). (The original's stack
// temps below the stack pointer are not kept.)
void depth_add(RenderLayer* l, DrawItem* item, u32 extra, u32 consts_ea) {
    const be_f32* m = host<be_f32>(l3::g::view_matrix);
    const be_f32* c = host<be_f32>(consts_ea);
    const f32 x = item->pos[0], y = item->pos[1], z = item->pos[2];
    LayerEntry* cursor = l->cursor;
    const s64 bias = static_cast<s8>(static_cast<u8>(extra >> 8));
    f32 v[4];
    for (u32 k = 0; k < 4; ++k) {
        f32 a = std::fmaf(m[k], x, 0.0f);
        a = std::fmaf(m[4 + k], y, a);
        a = std::fmaf(m[8 + k], z, a);
        v[k] = m[12 + k] + a;
    }
    f32 d = static_cast<f32>(static_cast<f64>(bias));  // fcfid + frsp
    d = fmadds(d, c[0], v[2]);
    d = d - c[1];
    d = d * c[2];
    const u32 bits = static_cast<u32>(fctiwz(d));
    or_zero_page(cursor)->key = l3::kDepthKeyMax - bits;
    LayerEntry* entry = or_zero_page(l->cursor.get());
    entry->item = item;
    l->cursor = at<LayerEntry>(entry, 8);
}

bool has_entries(RenderLayer* l) { return or_zero_page(l->base.get())->item; }

u8* view_object() { return *at<gp<u8>>(global<gp<u8>>(kCurrentSurface).get(), 672); }

}  // namespace

// void RenderLayerDepth_Add(RenderLayer* l, DrawItem* item, u32 extra)   (kVtableDepth slot 2; name?)
void RenderLayerDepth_Add(RenderLayer* l, DrawItem* item, u32 extra) { depth_add(l, item, extra, l3::kDepthAddConsts); }
// void RenderLayerZ_Add(RenderLayer* l, DrawItem* item, u32 extra)   (kVtableZ slot 2; name?)
// Same code as RenderLayerDepth_Add with its own (equal) constants.
void RenderLayerZ_Add(RenderLayer* l, DrawItem* item, u32 extra) { depth_add(l, item, extra, l3::kZAddConsts); }

// void RenderLayerZ_Draw(RenderLayer* l)   (kVtableZ slot 4; name?)
// RenderLayer_Draw with the item pass replaced by ZLayer_DrawBody(l)
// (no layer count, stencil or layer end here).
void RenderLayerZ_Draw(RenderLayer* l) {
    if ((l->flags >> 7) != 0) return;
    run_layer_callbacks(l, 20, l->pre_count);
    if (has_entries(l)) ZLayer_DrawBody(l);
    run_layer_callbacks(l, 88, l->post_count);
}

// void RenderLayerResolve_Draw(RenderLayer* l)   (kVtableResolve slot 4; name?)
// Only when the current view's +256 has bit 3 set and the layer isn't
// skipped: pre-draw callbacks, the two resolve passes if the list isn't
// empty, post-draw callbacks.
void RenderLayerResolve_Draw(RenderLayer* l) {
    if ((*at<be_u32>(view_object(), l3::kViewFlags) & l3::kViewResolve) == 0) return;
    if ((l->flags >> 7) != 0) return;
    run_layer_callbacks(l, 20, l->pre_count);
    if (has_entries(l)) {
        ResolveLayer_Pass1(l);
        ResolveLayer_Pass2(l);
    }
    run_layer_callbacks(l, 88, l->post_count);
}

// Destructors (slot 5) and deleting destructors (slot 6) of the derived
// layers: the same code as RenderLayer_Destruct / RenderLayer_Delete.
void RenderLayerDepth_Destruct(RenderLayer* l) { layer_destruct(l); }
void RenderLayerZ_Destruct(RenderLayer* l) { layer_destruct(l); }
void RenderLayerResolve_Destruct(RenderLayer* l) { layer_destruct(l); }
void RenderLayerUI_Destruct(RenderLayer* l) { layer_destruct(l); }
void RenderLayerDepth_Delete(RenderLayer* l) { destroy(l); }
void RenderLayerZ_Delete(RenderLayer* l) { destroy(l); }
void RenderLayerUI_Delete(RenderLayer* l) { destroy(l); }
void RenderLayerResolve_Delete(RenderLayer* l) { destroy(l); }

}  // namespace sly4::gfx

NATIVE(0050D744, sly4::gfx::RenderLayers_Register)
NATIVE(0052E164, sly4::gfx::RenderLayers_Construct)
NATIVE(007A1290, sly4::gfx::RenderLayerDepth_Add)
NATIVE(007A1400, sly4::gfx::RenderLayerZ_Add)
NATIVE(00514074, sly4::gfx::RenderLayerZ_Draw)
NATIVE(00514B0C, sly4::gfx::RenderLayerResolve_Draw)
NATIVE(0079C2B4, sly4::gfx::RenderLayerDepth_Destruct)
NATIVE(0079C308, sly4::gfx::RenderLayerZ_Destruct)
NATIVE(0079C35C, sly4::gfx::RenderLayerResolve_Destruct)
NATIVE(0079C3B0, sly4::gfx::RenderLayerUI_Destruct)
NATIVE(007A1380, sly4::gfx::RenderLayerDepth_Delete)
NATIVE(007A14F0, sly4::gfx::RenderLayerZ_Delete)
NATIVE(007A155C, sly4::gfx::RenderLayerUI_Delete)
NATIVE(007A15C8, sly4::gfx::RenderLayerResolve_Delete)
