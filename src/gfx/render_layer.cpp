// Render layers: the 17 static draw lists the renderer fills each frame
// (Opaque, Panorama, PreShadow, Translucent0..2, ..., MotionTrail,
// ThermalVision), registered in the layer table by 0x0050D744.
#include "rsx.h"

#include <bit>
#include <cmath>

namespace sly4::gfx {

namespace {

constexpr u32 Mem_FreeFrom = 0x0054EE6C;  // (p, pool)
constexpr u32 RenderLayer_RadixSort = 0x00513394;  // (RenderLayer*) sort for > 512 entries (name?)
constexpr u32 SortEntries = 0x0056A81C;            // (Entry*, u32 count, u32 size 8) (name?)
constexpr u32 DrawItem_Draw = 0x00512C30;          // (DrawItem*) (name?)
constexpr u32 RenderLayer_End = 0x00512DA0;        // () (name?)
constexpr u32 kBaseVtable = 0x00B233E8;
constexpr u32 kAddConsts = 0x007A10F8;  // f64 0.5, f64 0x1FF8000000000000 as a value, f32 1e-5, f64 0.0, f32 2.0
constexpr u32 kCameraPos = 0x010DD8B0;  // f32[3]: eye position RenderLayer_Add measures from (name?)
constexpr u32 kStencilBytes = 0x010DDA60;  // u8[2]: Draw_SetStencilRef writes {0, ref} (name?)
constexpr u32 kStateCacheA = 0x010DD598, kStateCacheB = 0x010DD5A0;  // u64, reset to -1
constexpr u32 kCurrentSurface = 0x010DDA74;  // object whose +672 is the current view (+48 u8 id)

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
};
static_assert(offsetof(RenderLayer, pre_count) == 84 && offsetof(RenderLayer, post_count) == 152 &&
              offsetof(RenderLayer, high) == 160);

// Frees the entry buffer and drops back to the base vtable (the
// destructor body both layer classes share).
void destruct(RenderLayer* layer) {
    const u32 buf = layer->base.ea;
    layer->vtable = kBaseVtable;
    if (buf == 0) return;
    call(Mem_FreeFrom, buf, 0u);
    layer->base = nullptr;
}

// Appends {key, item} at the cursor.
void append(RenderLayer* layer, u32 key, DrawItem* item) {
    layer->cursor->key = key;
    LayerEntry* entry = layer->cursor;
    entry->item = item;
    layer->cursor = entry + 1;
}

// Runs the callbacks of a list (count byte re-read after each call) whose
// view id matches the current view's +48 byte.
void run_callbacks(RenderLayer* layer, u32 list, const u8& count_ref) {
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

}  // namespace

void Draw_SetStencilRef(u8 value);

// void RenderLayer_Reset(RenderLayer* l)   (slot 0)
// Empties the list: cursor = base.
void RenderLayer_Reset(RenderLayer* l) { l->cursor = l->base.get(); }

// void RenderLayer_AddKeyed(RenderLayer* l, void* item, u32 key)   (slot 1)
void RenderLayer_AddKeyed(RenderLayer* l, DrawItem* item, u32 key) { append(l, key, item); }

// void RenderLayerBase_Add(RenderLayer* l, void* item)   (base class slot 2)
// Appends item with key 0.
void RenderLayerBase_Add(RenderLayer* l, DrawItem* item) { append(l, 0, item); }

// void RenderLayer_Add(RenderLayer* l, DrawItem* item)   (slot 2)
// Appends item with a sort key built from the item:
//   bits 26..31  min(2 * |item +48 - camera|, g::config f32 +48), truncated
//   bits 14..25  (item +28)->+46 u16 << 14     (0x03FFC000)
//   bits  6..13  (item +28)->+48 u16 << 6      (0x00003FC0)
//   bits  0..5   (item +12)->+80 & 0x3F
// The distance is the bit-trick square root (fcfid of the double's bits *
// 0.5 + 0x1FF8000000000000, fctid back), 0 below 1e-5 squared.
void RenderLayer_Add(RenderLayer* l, DrawItem* item) {
    const u8* k = host<u8>(kAddConsts);
    const be_f32* cam = host<be_f32>(kCameraPos);
    const f32 dy = item->pos[1] - cam[1];
    const f32 dx = item->pos[0] - cam[0];
    const f32 dz = item->pos[2] - cam[2];
    f32 d2 = static_cast<f32>(std::fma(static_cast<f64>(dx), dx, static_cast<f64>(dy * dy)));
    d2 = static_cast<f32>(std::fma(static_cast<f64>(dz), dz, static_cast<f64>(d2)));
    const f32 cut = d2 - *at<be_f32>(k, 16);
    const f64 asint = rt::fcfid(std::bit_cast<u64>(static_cast<f64>(d2)));
    const f64 est = std::fma(asint, at<be_f64>(k, 0)->get(), at<be_f64>(k, 8)->get());
    const f32 max_depth = global<be_f32>(sly4::g::config + 48);
    const u8* obj = item->object;
    const u8* mat = item->material;
    const u64 root_bits = rt::fctid(est, false);
    u32 key = (static_cast<u32>(at<be_u16>(obj, 46)->get()) << 14) & 0x03FFC000u;
    key |= (static_cast<u32>(at<be_u16>(obj, 48)->get()) << 6) & 0x00003FC0u;
    key = (key & 0xFFFFFFC0u) | (at<be_u32>(mat, 80)->get() & 0x3Fu);
    const f64 sel = cut >= 0.0f ? std::bit_cast<f64>(root_bits) : at<be_f64>(k, 24)->get();  // fsel
    f32 depth = static_cast<f32>(sel) * *at<be_f32>(k, 32);
    if (!(max_depth - depth >= 0.0f)) depth = max_depth;  // fsel
    const u64 bits = rt::fctiw(depth, true);
    key = (key & 0x03FFFFFFu) | (static_cast<u32>(bits) << 26);
    append(l, key, item);
}

// void RenderLayer_Sort(RenderLayer* l)   (slot 3)
// Records the high-water mark of the entry count. When the count passes
// the capacity the list is emptied (the loop the original then runs over
// the now-empty list never executes); the old count is still sorted:
// RenderLayer_RadixSort above 512 entries, else SortEntries(base, count, 8).
void RenderLayer_Sort(RenderLayer* l) {
    const u32 base = l->base.ea;
    const s32 bytes = static_cast<s32>(l->cursor.ea - base);
    const u32 count = static_cast<u32>(bytes / 8);  // srawi + addze
    const u32 high = l->high;
    l->high = high > count ? high : count;
    if (count > l->capacity) l->cursor.ea = base;  // overflow: drop everything
    if (count == 0) return;
    if (count > 512) call(RenderLayer_RadixSort, l);
    else call(SortEntries, base, count, 8u);
}

// void RenderLayer_Draw(RenderLayer* l)   (slot 4)
// Unless disabled (+12 bit 7): the pre-draw callbacks, then, if the list
// isn't empty, one more layer counted in g::config +76, the stencil setup
// for +13, every item drawn until a null item, the layer end; finally the
// post-draw callbacks.
void RenderLayer_Draw(RenderLayer* l) {
    if ((l->flags >> 7) != 0) return;
    run_callbacks(l, 20, l->pre_count);
    if (l->base->item) {
        global<be_u32>(sly4::g::config + 76) += 1;
        Draw_SetStencilRef(l->stencil_ref);
        for (LayerEntry* e = l->base; e->item; ++e) call(DrawItem_Draw, e->item.get());
        call(RenderLayer_End);
    }
    run_callbacks(l, 88, l->post_count);
}

// void Draw_SetStencilRef(u8 ref)
// Stencil test on, writing ref where drawn: ops KEEP / KEEP / REPLACE,
// mask ref, func ALWAYS ref 0 mask 0 (cached in the render state).
void Draw_SetStencilRef(u8 value) {
    RenderState& st = rs();
    constexpr u32 kStencilOp = 0x000C033C, kStencilMask = 0x0004032C, kStencilFunc = 0x000C0330;
    constexpr u32 kKeep = 0x1E00, kReplace = 0x1E01, kAlways = 0x0207;
    const u32 r = value;
    emit({kStencilTestEnable, 1});
    st.stencil_op[0] = kKeep;
    st.stencil_op[1] = kKeep;
    st.stencil_op[2] = kReplace;
    st.stencil_func = kAlways;
    st.stencil_ref = 0u;
    st.stencil_func_mask = 0u;
    st.stencil_mask = r;
    emit({kStencilOp, kKeep, kKeep, kReplace});
    emit({kStencilMask, r});
    emit({kStencilFunc, st.stencil_func.get(), st.stencil_ref.get(), st.stencil_func_mask.get()});
    u8* b = host<u8>(kStencilBytes);
    b[0] = 0;
    b[1] = value;
}

// RenderLayer::~RenderLayer (slot 5; the base class's 0x0079ECC4 is the same)
void RenderLayer_Destruct(RenderLayer* l) { destruct(l); }
void RenderLayerBase_Destruct(RenderLayer* l) { destruct(l); }

// Deleting destructors (slot 6; base class 0x007A108C): destruct, then
// free the object itself.
void RenderLayer_Delete(RenderLayer* l) {
    if (!l) return;
    destruct(l);
    call(Mem_FreeFrom, l, 0u);
}
void RenderLayerBase_Delete(RenderLayer* l) {
    if (!l) return;
    destruct(l);
    call(Mem_FreeFrom, l, 0u);
}

}  // namespace sly4::gfx

NATIVE(007A1080, sly4::gfx::RenderLayer_Reset)
NATIVE(0051384C, sly4::gfx::RenderLayer_AddKeyed)
NATIVE(00513868, sly4::gfx::RenderLayerBase_Add)
NATIVE(007A1120, sly4::gfx::RenderLayer_Add)
NATIVE(00513894, sly4::gfx::RenderLayer_Sort)
NATIVE(00513984, sly4::gfx::RenderLayer_Draw)
NATIVE(00512EF4, sly4::gfx::Draw_SetStencilRef)
NATIVE(0079C260, sly4::gfx::RenderLayer_Destruct)
NATIVE(0079ECC4, sly4::gfx::RenderLayerBase_Destruct)
NATIVE(007A1210, sly4::gfx::RenderLayer_Delete)
NATIVE(007A108C, sly4::gfx::RenderLayerBase_Delete)
