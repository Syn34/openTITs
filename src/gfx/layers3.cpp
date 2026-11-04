// Render layer table set-up: RenderLayers_Construct (the static
// constructor of the 17 render layers and of the render module statics
// around them) and RenderLayers_Register (allocates each layer's entry
// buffer and appends it to render::g::layers).
#include "layers3.h"

#include <atomic>
#include <cmath>

using namespace sly4;

namespace {

namespace rl = sly4::render::layer;
namespace rg = sly4::render::g;
namespace l3 = sly4::render::layers3;
namespace at = sly4::render::layers3::at;
namespace nm = sly4::render::layers3::name;
namespace g3 = sly4::render::layers3::g;

constexpr u32 kOne = 0x3F800000, kMinusOne = 0xBF800000;  // f32 bit patterns
constexpr u32 kTwo = 0x40000000, kMinusTwo = 0xC0000000, kHalf = 0x3F000000, kMinusHalf = 0xBF000000;

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
    {at::Opaque, 428, g3::colour_cac, 255, 0},
    {at::Panorama, 430, g3::colour_ca0, 0, 0},
    {at::PreShadow, 432, g3::colour_cb0, 0, 0},
    {at::Translucent0, 434, g3::colour_cb8, 32, 0},
    {at::Translucent1, 436, g3::colour_cb8, 0, kSwapColour01},
    {at::Translucent2, 438, g3::colour_cb8, 0, 0},
    {at::ZTranslucent, 440, g3::colour_cc8, 0, 0},
    {at::WaterResolve, 442, g3::colour_cb8, 0, kSwapColour01},
    {at::ScreenWarp, 444, g3::colour_cb8, 0, 0},
    {at::Effects1, 446, g3::colour_cb8, 0, kSwapColour01},
    {at::PostShadow1, 448, g3::colour_cb4, 0, kMaxFirst},
    {at::PostShadow2, 448, g3::colour_cb4, 0, 0},
    {at::UI, 450, g3::colour_ca0, 0, kSwapColour01},
    {at::PostPrim, 452, g3::colour_ca0, 0, 0},
    {at::Occluder, 428, g3::colour_ca0, 0, kSwapColour01},
    {at::MotionTrail, 454, g3::colour_ca0, 0, 0},
    {at::ThermalVision, 456, g3::colour_ca0, 0, kMaxFirst},
};
constexpr u32 kDefaultCapacity = 2048;
constexpr u32 kEntrySize = 8, kEntryAlign = 16;

// ---------------------------------------------------------------- RenderLayers_Construct

// The lwarx / stwcx. loop in the original: an atomic store of 0 (the old
// value is dropped). Same as gcm.cpp / init_surface.cpp.
void atomic_clear(u32 ea) { std::atomic_ref<u32>(*reinterpret_cast<u32*>(mem::ptr(ea))).store(0); }

void vtable(u32 l, u32 vt) { store<u32>(l, vt); }

// The inlined RenderLayer constructor body after the base vtable: no
// entry buffer, stencil ref 0, the name, flags bits 7 and 6 cleared (read
// before any store), and both callback lists cleared ({fn, arg, view} and
// the byte at +12 of each 16-byte record; the count bytes +84 / +152 are
// left alone). Translucent0..2 store the flags before the name.
void init(u32 l, u32 name, bool flags_first = false) {
    const u8 flags = static_cast<u8>(ref<u8>(l + l3::kFlags) & l3::kFlagsKeep);
    store<u32>(l + rl::kBase, 0);
    store<u32>(l + rl::kCursor, 0);
    store<u8>(l + l3::kStencilRef, 0);
    if (flags_first) {
        store<u8>(l + l3::kFlags, flags);
        store<u32>(l + l3::kName, name);
    } else {
        store<u32>(l + l3::kName, name);
        store<u8>(l + l3::kFlags, flags);
    }
    for (const u32 list : {l3::kPreDraw, l3::kPostDraw}) {
        for (u32 i = 0; i < l3::kCallbacks; ++i) {
            const u32 cb = l + list + l3::kCallbackSize * i;
            store<u32>(cb, 0);
            store<u32>(cb + 4, 0);
            store<u32>(cb + 8, 0);
            store<u8>(cb + 12, 0);
        }
    }
}

// Capacity, high-water count and byte size, all 0.
void zero_counts(u32 l) {
    store<u32>(l + l3::kCapacity, 0);
    store<u32>(l + l3::kHighWater, 0);
    store<u32>(l + l3::kBytes, 0);
}

struct Colour {
    u8 c[4];
};
void colour_byte(u32 l, const Colour& c, u32 k) { store<u8>(l + l3::kColour + k, c.c[k]); }
void colour(u32 l, const Colour& c) {
    for (u32 k = 0; k < 4; ++k) colour_byte(l, c, k);
}

// The 192-byte layers' extra fields.
constexpr u32 kResolveA = 176, kResolveB = 180, kResolveC = 184, kResolveScale = 188;

// Stores the 4x4 rows {a, b, c, d}; `w` writes the fourth column too.
void matrix(u32 m, const u32 (&v)[4][4], bool w) {
    for (u32 r = 0; r < 4; ++r) {
        for (u32 k = 0; k < (w ? 4u : 3u); ++k) store<u32>(m + 16 * r + 4 * k, v[r][k]);
    }
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
DECOMP(0050D744, RenderLayers_Register) {
    for (const Registration& r : kRegistrations) {
        const u32 l = r.layer;
        const u32 n = load<u32>(rg::layers);
        store<u32>(rg::layers, n + 1);
        store<u32>(rg::layers + 4 + 4 * n, l);
        const s16 cfg = load<s16>(sly4::g::config + r.cfg);
        const u32 cap = cfg != 0 ? static_cast<u32>(static_cast<s32>(cfg)) : kDefaultCapacity;
        store<u32>(l + l3::kCapacity, cap);
        store<u32>(l + l3::kBytes, cap * kEntrySize);
        const u32 entries = static_cast<u32>(call(ctx, sly4::fn::Mem_Alloc, {cap * kEntrySize, 0, kEntryAlign}));
        store<u32>(l + rl::kBase, entries);
        store<u32>(l + rl::kCursor, entries);
        const auto copy = [&](u32 k) { store<u8>(l + l3::kColour + k, ref<u8>(r.colour + k)); };
        if (r.order & kSwapColour01) {
            copy(1);
            copy(0);
        } else {
            copy(0);
            copy(1);
        }
        copy(2);
        copy(3);
        const u32 prev = load<u32>(g3::max_capacity);  // read after the slot store above
        const u32 high = prev > cap ? prev : cap;
        if (r.order & kMaxFirst) {
            store<u32>(g3::max_capacity, high);
            store<u8>(l + l3::kStencilRef, r.stencil_ref);
        } else {
            store<u8>(l + l3::kStencilRef, r.stencil_ref);
            store<u32>(g3::max_capacity, high);
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
DECOMP(0052E164, RenderLayers_Construct) {
    const u32 cfg = sly4::g::config;
    store<u8>(g3::byte_da70, 0xFF);
    store<u16>(cfg + 24, 0);
    store<u16>(cfg + 26, 0);
    store<u16>(cfg + 28, 0);
    store<u16>(cfg + 30, 0);
    call(ctx, sly4::fn::memset, {cfg, 0, l3::kConfigSize});
    store<u32>(cfg + 320, kOne);
    store<u32>(cfg + 60, kOne);
    store<u32>(g3::block_e061c, 0);
    store<u32>(g3::block_dd5a8 + 4, 0);
    store<u32>(g3::block_dd5a8 + 8, 0);
    store<u32>(g3::block_dd5a8 + 12, kOne);
    store<u32>(g3::block_dd5a8 + 16, kOne);
    store<u32>(g3::block_dd5a8 + 20, kOne);
    store<u32>(g3::block_e061c + 4, 32);
    atomic_clear(g3::atomic_ddc00);
    store<u64>(g3::block_ddb88 + 24, 0);
    store<u8>(g3::block_ddb88 + 32, 0);
    call(ctx, sly4::fn::memset, {g3::block_dd600, 0, l3::kBlockDd600Size});
    store<u32>(g3::block_dd600 + 256, g3::block_dd600 + 128);

    constexpr u32 base = rl::kBaseVtable;
    // Opaque (its colour is read once and reused for every layer)
    vtable(at::Opaque, base);
    init(at::Opaque, nm::Opaque);
    const Colour c = {{ref<u8>(g3::colour_ca0), ref<u8>(g3::colour_ca0 + 1), ref<u8>(g3::colour_ca0 + 2),
                       ref<u8>(g3::colour_ca0 + 3)}};
    zero_counts(at::Opaque);
    colour_byte(at::Opaque, c, 0);
    vtable(at::Opaque, rl::kVtable);
    vtable(at::Panorama, base);
    colour_byte(at::Opaque, c, 1);
    colour_byte(at::Opaque, c, 2);
    colour_byte(at::Opaque, c, 3);
    // Panorama
    init(at::Panorama, nm::Panorama);
    store<u32>(at::Panorama + l3::kCapacity, 0);
    store<u32>(at::Panorama + l3::kHighWater, 0);
    vtable(at::Panorama, l3::kVtableDepth);
    vtable(at::PreShadow, base);
    store<u32>(at::Panorama + l3::kBytes, 0);
    colour(at::Panorama, c);
    // PreShadow
    init(at::PreShadow, nm::PreShadow);
    vtable(at::PreShadow, l3::kVtableDepth);
    vtable(at::Translucent0, base);
    zero_counts(at::PreShadow);
    colour(at::PreShadow, c);
    // Translucent0
    init(at::Translucent0, nm::Translucent0, true);
    vtable(at::Translucent0, l3::kVtableDepth);
    vtable(at::Translucent1, base);
    zero_counts(at::Translucent0);
    colour(at::Translucent0, c);
    // Translucent1
    init(at::Translucent1, nm::Translucent1, true);
    vtable(at::Translucent1, l3::kVtableDepth);
    vtable(at::Translucent2, base);
    zero_counts(at::Translucent1);
    colour(at::Translucent1, c);
    // Translucent2
    init(at::Translucent2, nm::Translucent2, true);
    zero_counts(at::Translucent2);
    colour(at::Translucent2, c);
    vtable(at::Translucent2, l3::kVtableDepth);
    vtable(at::ZTranslucent, base);
    // ZTranslucent
    init(at::ZTranslucent, nm::ZTranslucent);
    zero_counts(at::ZTranslucent);
    colour(at::ZTranslucent, c);
    vtable(at::ZTranslucent, l3::kVtableZ);
    vtable(at::WaterResolve, base);
    // WaterResolve (192 bytes)
    {
        const u32 l = at::WaterResolve;
        init(l, nm::WaterResolve);
        store<u32>(l + kResolveA, 0);
        store<u32>(l + l3::kCapacity, 0);
        vtable(l, l3::kVtableResolve);
        store<u32>(l + kResolveB, 0);
        store<u32>(l + l3::kHighWater, 0);
        store<u32>(l + kResolveC, 0);
        store<u32>(l + l3::kBytes, 0);
        colour_byte(l, c, 0);
        store<u32>(l + kResolveScale, kOne);
        colour_byte(l, c, 1);
        colour_byte(l, c, 2);
        vtable(at::ScreenWarp, base);
        colour_byte(l, c, 3);
    }
    // ScreenWarp (192 bytes)
    {
        const u32 l = at::ScreenWarp;
        init(l, nm::ScreenWarp);
        vtable(l, l3::kVtableResolve);
        store<u32>(l + kResolveA, 0);
        store<u32>(l + kResolveB, 0);
        store<u32>(l + kResolveC, 0);
        store<u32>(l + l3::kCapacity, 0);
        store<u32>(l + l3::kHighWater, 0);
        store<u32>(l + kResolveScale, kOne);
        store<u32>(l + l3::kBytes, 0);
        colour_byte(l, c, 0);
        vtable(at::Effects1, base);
        colour_byte(l, c, 1);
        colour_byte(l, c, 2);
        colour_byte(l, c, 3);
    }
    // Effects1
    init(at::Effects1, nm::Effects1);
    zero_counts(at::Effects1);
    colour(at::Effects1, c);
    vtable(at::Effects1, l3::kVtableDepth);
    vtable(at::PostShadow1, base);
    // PostShadow1
    init(at::PostShadow1, nm::PostShadow1);
    vtable(at::PostShadow1, rl::kVtable);
    vtable(at::PostShadow2, base);
    zero_counts(at::PostShadow1);
    colour(at::PostShadow1, c);
    // PostShadow2
    init(at::PostShadow2, nm::PostShadow2);
    vtable(at::PostShadow2, rl::kVtable);
    vtable(at::UI, base);
    zero_counts(at::PostShadow2);
    colour(at::PostShadow2, c);
    // UI
    init(at::UI, nm::UI);
    store<u32>(at::UI + l3::kCapacity, 0);
    vtable(at::UI, l3::kVtableUI);
    store<u32>(at::UI + l3::kHighWater, 0);
    vtable(at::PostPrim, base);
    store<u32>(at::UI + l3::kBytes, 0);
    colour(at::UI, c);
    // PostPrim
    init(at::PostPrim, nm::PostPrim);
    vtable(at::PostPrim, rl::kVtable);
    vtable(at::Occluder, base);
    zero_counts(at::PostPrim);
    colour(at::PostPrim, c);
    // Occluder
    init(at::Occluder, nm::Occluder);
    vtable(at::Occluder, rl::kVtable);
    vtable(at::MotionTrail, base);
    zero_counts(at::Occluder);
    colour(at::Occluder, c);
    // MotionTrail
    init(at::MotionTrail, nm::MotionTrail);
    vtable(at::MotionTrail, rl::kVtable);
    vtable(at::ThermalVision, base);
    zero_counts(at::MotionTrail);
    colour(at::MotionTrail, c);
    // ThermalVision
    {
        const u32 l = at::ThermalVision;
        init(l, nm::ThermalVision);
        store<u32>(l + l3::kCapacity, 0);
        store<u32>(l + l3::kHighWater, 0);
        colour_byte(l, c, 0);
        store<u32>(l + l3::kBytes, 0);
        colour_byte(l, c, 1);
        colour_byte(l, c, 2);
        colour_byte(l, c, 3);
        vtable(l, rl::kVtable);
    }

    atomic_clear(g3::atomic_dd400);
    store<u32>(g3::block_dd780 + 608, 0);
    store<u8>(rg::stencil_bytes, 0);
    store<u8>(rg::stencil_bytes + 1, 0);
    for (u32 i = 0; i < l3::kSlotCount; ++i) call(ctx, l3::fn::Slot_Construct, {g3::slots + l3::kSlotSize * i});
    call(ctx, l3::fn::Pool_Construct, {g3::pool});
    store<u32>(g3::pool + 352, 0);
    store<u32>(g3::pool, l3::kPoolVtable);
    store<u32>(g3::pool + 356, kMinusOne);
    store<u32>(g3::pool + 360, 0);
    call(ctx, l3::fn::PtrVector_Construct, {g3::pool_vector});
    store<u16>(g3::pool_vector + 6, 4);  // element size
    call(ctx, l3::fn::PtrVector_Init, {g3::pool_vector, 0, 0, 0});

    constexpr u32 uv_to_clip[4][4] = {
        {kTwo, 0, 0, 0}, {0, kMinusTwo, 0, 0}, {0, 0, kOne, 0}, {kMinusOne, kOne, 0, 0}};
    constexpr u32 clip_to_uv[4][4] = {
        {kHalf, 0, 0, 0}, {0, kMinusHalf, 0, 0}, {0, 0, kOne, 0}, {kHalf, kHalf, 0, kOne}};
    matrix(g3::uv_to_clip, uv_to_clip, false);
    matrix(g3::clip_to_uv, clip_to_uv, true);
}

// ---------------------------------------------------------------- derived layer slots

namespace {

// The destructor body every layer class shares: base vtable, entry buffer
// freed and cleared. r3 = 0 on return; r4 = the base vtable when there was
// no buffer (otherwise what Mem_FreeFrom leaves).
void destruct(PPUContext& ctx, u32 l) {
    const u32 buf = load<u32>(l + rl::kBase);
    store<u32>(l, rl::kBaseVtable);
    if (buf == 0) {
        ctx.gpr[4] = rl::kBaseVtable;
    } else {
        call(ctx, l3::fn::Mem_FreeFrom, {buf, 0});
        store<u32>(l + rl::kBase, 0);
    }
    ctx.gpr[3] = 0;
}

void destroy(PPUContext& ctx) {
    const u32 l = arg<u32>(ctx, 0);
    if (l == 0) return;  // r3 stays 0
    destruct(ctx, l);
    ctx.gpr[3] = call(ctx, l3::fn::Mem_FreeFrom, {l, 0});
}

// The view-depth Add: the item position (+48) through g::view_matrix
// (vmaddfp per lane: fmaf(row0, x, 0), fmaf(row1, y, .), fmaf(row2, z, .),
// + row3), then depth = ((s8)(extra >> 8) * c0 + view z - c1) * c2,
// truncated, and key = 0xFFFFFF - depth (far first). The stack temps the
// original writes (the vector at sp-32, the double at sp-16) are kept.
// lvlx loads: the position is assumed 4-byte aligned (lane 0 is whole).
void depth_add(PPUContext& ctx, u32 consts) {
    const u32 l = arg<u32>(ctx, 0), item = arg<u32>(ctx, 1), extra = arg<u32>(ctx, 2);
    const u32 sp = static_cast<u32>(ctx.gpr[1]);
    const u32 pos = item + 48, m = g3::view_matrix;
    const f32 x = load<f32>(pos), y = load<f32>(pos + 4), z = load<f32>(pos + 8);
    const u32 cursor = load<u32>(l + rl::kCursor);
    const s64 bias = static_cast<s8>(static_cast<u8>(extra >> 8));
    store<u64>(sp - 16, static_cast<u64>(bias));  // std r5, 64(r1)
    f32 v[4];
    for (u32 k = 0; k < 4; ++k) {
        f32 a = std::fmaf(load<f32>(m + 4 * k), x, 0.0f);
        a = std::fmaf(load<f32>(m + 16 + 4 * k), y, a);
        a = std::fmaf(load<f32>(m + 32 + 4 * k), z, a);
        v[k] = load<f32>(m + 48 + 4 * k) + a;
    }
    for (u32 k = 0; k < 4; ++k) store<f32>(sp - 32 + 4 * k, v[k]);  // stvx v2, sp+48
    f32 d = static_cast<f32>(rt::fcfid(static_cast<u64>(bias)));
    d = static_cast<f32>(std::fma(static_cast<f64>(d), static_cast<f64>(load<f32>(consts)), static_cast<f64>(v[2])));
    d = static_cast<f32>(static_cast<f64>(d) - static_cast<f64>(load<f32>(consts + 4)));
    d = static_cast<f32>(static_cast<f64>(d) * static_cast<f64>(load<f32>(consts + 8)));
    const u64 bits = rt::fctiw(d, true);
    store<u64>(sp - 16, bits);
    store<u32>(cursor, l3::kDepthKeyMax - static_cast<u32>(bits));
    const u32 entry = load<u32>(l + rl::kCursor);
    store<u32>(entry + 4, item);
    store<u32>(l + rl::kCursor, entry + 8);
    ctx.gpr[5] = entry;  // left as the original leaves them
    ctx.gpr[6] = entry + 8;
    ctx.gpr[7] = 0x00FF0000;
    ctx.gpr[8] = cursor;
}

// The callback lists, as RenderLayer_Draw runs them (render_layer.cpp):
// records whose view id (+8) matches the current view's +48 byte, count
// re-read after each call, the state caches reset after each. The call
// leaves the OPD / entry in r4 / r5, or r5 / r4 when `swapped` (the
// resolve layers' pre-draw loop).
void run_callbacks(PPUContext& ctx, u32 l, u32 list, u32 count_at, bool swapped = false) {
    u32 count = ref<u8>(l + count_at);
    if (static_cast<s32>(count) <= 0) return;
    for (u32 i = 0; i < count; i = (i + 1) & 0xFF) {
        const u32 cb = l + list + l3::kCallbackSize * i;
        const u32 fn = load<u32>(cb);
        if (fn == 0) continue;
        const u32 view = load<u32>(load<u32>(draw::g::current_surface) + 672);
        if (static_cast<s32>(ref<u8>(view + 48)) != load<s32>(cb + 8)) continue;
        if (swapped) call_ptr(ctx, fn, {load<u32>(cb + 4), load<u32>(fn), fn});
        else call_ptr(ctx, fn, {load<u32>(cb + 4), fn, load<u32>(fn)});
        store<u64>(rg::state_cache_a, ~u64{0});
        store<u64>(rg::state_cache_b, ~u64{0});
        count = ref<u8>(l + count_at);
    }
}

bool has_entries(u32 l) { return load<u32>(load<u32>(l + rl::kBase) + 4) != 0; }

}  // namespace

// void RenderLayerDepth_Add(RenderLayer* l, DrawItem* item, u32 extra)   (kVtableDepth slot 2; name?)
DECOMP(007A1290, RenderLayerDepth_Add) { depth_add(ctx, l3::kDepthAddConsts); }
// void RenderLayerZ_Add(RenderLayer* l, DrawItem* item, u32 extra)   (kVtableZ slot 2; name?)
// Same code as RenderLayerDepth_Add with its own (equal) constants.
DECOMP(007A1400, RenderLayerZ_Add) { depth_add(ctx, l3::kZAddConsts); }

// void RenderLayerZ_Draw(RenderLayer* l)   (kVtableZ slot 4; name?)
// RenderLayer_Draw with the item pass replaced by ZLayer_DrawBody(l)
// (no layer count, stencil or layer end here).
DECOMP(00514074, RenderLayerZ_Draw) {
    const u32 l = arg<u32>(ctx, 0);
    if ((ref<u8>(l + l3::kFlags) >> 7) != 0) return;
    run_callbacks(ctx, l, l3::kPreDraw, 84);
    if (has_entries(l)) call(ctx, l3::fn::ZLayer_DrawBody, {l});
    run_callbacks(ctx, l, l3::kPostDraw, 152);
}

// void RenderLayerResolve_Draw(RenderLayer* l)   (kVtableResolve slot 4; name?)
// Only when the current view's +256 has bit 3 set and the layer isn't
// skipped: pre-draw callbacks, the two resolve passes if the list isn't
// empty, post-draw callbacks.
DECOMP(00514B0C, RenderLayerResolve_Draw) {
    const u32 l = arg<u32>(ctx, 0);
    const u32 view = load<u32>(load<u32>(draw::g::current_surface) + 672);
    if ((load<u32>(view + l3::kViewFlags) & l3::kViewResolve) == 0) return;
    if ((ref<u8>(l + l3::kFlags) >> 7) != 0) return;
    run_callbacks(ctx, l, l3::kPreDraw, 84, true);
    if (has_entries(l)) {
        call(ctx, l3::fn::ResolveLayer_Pass1, {l});
        call(ctx, l3::fn::ResolveLayer_Pass2, {l});
    }
    run_callbacks(ctx, l, l3::kPostDraw, 152);
}

// Destructors (slot 5) and deleting destructors (slot 6) of the derived
// layers: the same code as RenderLayer_Destruct / RenderLayer_Delete.
DECOMP(0079C2B4, RenderLayerDepth_Destruct) { destruct(ctx, arg<u32>(ctx, 0)); }
DECOMP(0079C308, RenderLayerZ_Destruct) { destruct(ctx, arg<u32>(ctx, 0)); }
DECOMP(0079C35C, RenderLayerResolve_Destruct) { destruct(ctx, arg<u32>(ctx, 0)); }
DECOMP(0079C3B0, RenderLayerUI_Destruct) { destruct(ctx, arg<u32>(ctx, 0)); }
DECOMP(007A1380, RenderLayerDepth_Delete) { destroy(ctx); }
DECOMP(007A14F0, RenderLayerZ_Delete) { destroy(ctx); }
DECOMP(007A155C, RenderLayerUI_Delete) { destroy(ctx); }
DECOMP(007A15C8, RenderLayerResolve_Delete) { destroy(ctx); }
