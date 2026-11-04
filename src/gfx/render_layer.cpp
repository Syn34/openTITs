// Render layers: the 17 static draw lists the renderer fills each frame
// (Opaque, Panorama, PreShadow, Translucent0..2, ..., MotionTrail,
// ThermalVision), registered in render::g::layers by 0x0050D744.
#include "render.h"

#include <bit>
#include <cmath>

using namespace sly4;

namespace {

namespace rg = sly4::render::g;
namespace rl = sly4::render::layer;

constexpr u32 Mem_FreeFrom = 0x0054EE6C;  // (p, pool)

// Frees the entry buffer and drops back to the base vtable (the
// destructor body both layer classes share). Returns the r3 it leaves.
u64 destruct(PPUContext& ctx, u32 layer) {
    const u32 buf = load<u32>(layer + rl::kBase);
    store<u32>(layer, rl::kBaseVtable);
    if (buf == 0) return 0;
    call(ctx, Mem_FreeFrom, {buf, 0});
    store<u32>(layer + rl::kBase, 0);
    return 0;
}

// Appends {key, item} at the cursor. Leaves r5 / r6 = the entry and the
// new cursor, as the original sequence does.
void append(PPUContext& ctx, u32 layer, u32 key, u32 item) {
    store<u32>(load<u32>(layer + rl::kCursor), key);
    const u32 entry = load<u32>(layer + rl::kCursor);
    store<u32>(entry + 4, item);
    store<u32>(layer + rl::kCursor, entry + 8);
    ctx.gpr[5] = entry;
    ctx.gpr[6] = entry + 8;
}

// Runs the callbacks of a list (+20 / +88: 16-byte records {OPD fn, u32
// arg, u32 view id, -}, count byte +84 / +152) whose view id matches the
// current view's +48 byte. The count is re-read after each call.
void run_callbacks(PPUContext& ctx, u32 layer, u32 list, u32 count_at) {
    u32 count = ref<u8>(layer + count_at);
    if (static_cast<s32>(count) <= 0) return;
    for (u32 i = 0; i < count; i = (i + 1) & 0xFF) {
        const u32 cb = layer + list + 16 * i;
        const u32 fn = load<u32>(cb);
        if (fn == 0) continue;
        const u32 view = load<u32>(load<u32>(draw::g::current_surface) + 672);
        if (static_cast<s32>(ref<u8>(view + 48)) != load<s32>(cb + 8)) continue;
        call_ptr(ctx, fn, {load<u32>(cb + 4), fn, load<u32>(fn)});  // OPD / entry left in r4 / r5
        store<u64>(rg::state_cache_a, ~u64{0});
        store<u64>(rg::state_cache_b, ~u64{0});
        count = ref<u8>(layer + count_at);
    }
}

}  // namespace

// void RenderLayer_Reset(RenderLayer* l)   (slot 0)
// Empties the list: cursor = base.
DECOMP(007A1080, RenderLayer_Reset) {
    const u32 l = arg<u32>(ctx, 0);
    const u32 base = load<u32>(l + rl::kBase);
    store<u32>(l + rl::kCursor, base);
    ctx.gpr[4] = base;  // left in r4
}

// void RenderLayer_AddKeyed(RenderLayer* l, void* item, u32 key)   (slot 1)
DECOMP(0051384C, RenderLayer_AddKeyed) {
    append(ctx, arg<u32>(ctx, 0), arg<u32>(ctx, 2), arg<u32>(ctx, 1));
}

// void RenderLayerBase_Add(RenderLayer* l, void* item)   (base class slot 2)
// Appends item with key 0.
DECOMP(00513868, RenderLayerBase_Add) {
    append(ctx, arg<u32>(ctx, 0), 0, arg<u32>(ctx, 1));
}

// void RenderLayer_Add(RenderLayer* l, DrawItem* item)   (slot 2)
// Appends item with a sort key built from the item:
//   bits 26..31  min(2 * |item +48 - camera|, g::config f32 +48), truncated
//   bits 14..25  (item +28)->+46 u16 << 14     (0x03FFC000)
//   bits  6..13  (item +28)->+48 u16 << 6      (0x00003FC0)
//   bits  0..5   (item +12)->+80 & 0x3F
// The distance is the bit-trick square root (fcfid of the double's bits *
// 0.5 + 0x1FF8000000000000, fctid back), 0 below 1e-5 squared.
DECOMP(007A1120, RenderLayer_Add) {
    const u32 l = arg<u32>(ctx, 0), item = arg<u32>(ctx, 1);
    constexpr u32 k = rl::kAddConsts;
    const u32 pos = item + 48, cam = rg::camera_pos;
    const f32 dy = load<f32>(pos + 4) - load<f32>(cam + 4);
    const f32 dx = load<f32>(pos) - load<f32>(cam);
    const f32 dz = load<f32>(pos + 8) - load<f32>(cam + 8);
    f32 d2 = static_cast<f32>(std::fma(static_cast<f64>(dx), dx, static_cast<f64>(dy * dy)));
    d2 = static_cast<f32>(std::fma(static_cast<f64>(dz), dz, static_cast<f64>(d2)));
    const f32 cut = d2 - load<f32>(k + 16);
    const f64 asint = rt::fcfid(std::bit_cast<u64>(static_cast<f64>(d2)));
    const f64 est = std::fma(asint, load<f64>(k), load<f64>(k + 8));
    const f32 max_depth = load<f32>(sly4::g::config + 48);
    const u32 obj = load<u32>(item + 28), mat = load<u32>(item + 12);
    const u64 root_bits = rt::fctid(est, false);
    u32 key = (static_cast<u32>(load<u16>(obj + 46)) << 14) & 0x03FFC000u;
    key |= (static_cast<u32>(load<u16>(obj + 48)) << 6) & 0x00003FC0u;
    key = (key & 0xFFFFFFC0u) | (load<u32>(mat + 80) & 0x3Fu);
    const f64 sel = cut >= 0.0f ? std::bit_cast<f64>(root_bits) : load<f64>(k + 24);  // fsel
    f32 depth = static_cast<f32>(sel) * load<f32>(k + 32);
    if (!(max_depth - depth >= 0.0f)) depth = max_depth;  // fsel
    const u64 bits = rt::fctiw(depth, true);
    store<u64>(static_cast<u32>(ctx.gpr[1]) - 16, bits);  // the original's stfd / lwz temp
    key = (key & 0x03FFFFFFu) | (static_cast<u32>(bits) << 26);
    append(ctx, l, key, item);
}

// void RenderLayer_Sort(RenderLayer* l)   (slot 3)
// Records the high-water mark (+160) of the entry count. When the count
// passes the capacity (+156) the list is emptied (the loop the original
// then runs over the now-empty list never executes); the old count is
// still sorted: 0x00513394 above 512 entries, else 0x0056A81C(base,
// count, 8).
DECOMP(00513894, RenderLayer_Sort) {
    const u32 l = arg<u32>(ctx, 0);
    const u32 base = load<u32>(l + rl::kBase);
    const s32 bytes = static_cast<s32>(load<u32>(l + rl::kCursor) - base);
    const u32 count = static_cast<u32>(bytes / 8);  // srawi + addze
    const u32 high = load<u32>(l + 160);
    store<u32>(l + 160, high > count ? high : count);
    if (count > load<u32>(l + 156)) store<u32>(l + rl::kCursor, base);  // overflow: drop everything
    if (count == 0) return;
    if (count > 512) {
        call(ctx, rl::RenderLayer_RadixSort, {l});
    } else {
        call(ctx, rl::SortEntries, {base, count, 8});
    }
}

// void RenderLayer_Draw(RenderLayer* l)   (slot 4)
// Unless disabled (+12 bit 7): the pre-draw callbacks (+20), then, if
// the list isn't empty, one more layer counted in g::config +76, the
// stencil setup for +13, every item drawn until a null item, the layer
// end; finally the post-draw callbacks (+88).
DECOMP(00513984, RenderLayer_Draw) {
    const u32 l = arg<u32>(ctx, 0);
    if ((ref<u8>(l + 12) >> 7) != 0) return;
    run_callbacks(ctx, l, 20, 84);
    if (load<u32>(load<u32>(l + rl::kBase) + 4) != 0) {
        field<u32>(sly4::g::config, 76) += 1;
        call(ctx, rl::Draw_SetStencilRef, {ref<u8>(l + 13)});
        for (u32 e = load<u32>(l + rl::kBase); load<u32>(e + 4) != 0; e += 8) {
            call(ctx, rl::DrawItem_Draw, {load<u32>(e + 4)});
        }
        call(ctx, rl::RenderLayer_End);
    }
    run_callbacks(ctx, l, 88, 152);
}

// void Draw_SetStencilRef(u8 ref)
// Stencil test on, writing ref where drawn: ops KEEP / KEEP / REPLACE,
// mask ref, func ALWAYS ref 0 mask 0 (cached at render state +160..+184).
DECOMP(00512EF4, Draw_SetStencilRef) {
    const u32 value = arg<u32>(ctx, 0);
    const u32 st = fx::g::state;
    constexpr u32 kStencilOp = 0x000C033C, kStencilMask = 0x0004032C, kStencilFunc = 0x000C0330;
    constexpr u32 kKeep = 0x1E00, kReplace = 0x1E01, kAlways = 0x0207;
    const u32 r = value & 0xFF;
    fx::emit({fx::kStencilTestEnable, 1});
    store<u32>(st + 176, kKeep);
    store<u32>(st + 180, kKeep);
    store<u32>(st + 184, kReplace);
    store<u32>(st + 160, kAlways);
    store<u32>(st + 164, 0);
    store<u32>(st + 172, 0);
    store<u32>(st + 168, r);
    fx::emit({kStencilOp, kKeep, kKeep, kReplace});
    fx::emit({kStencilMask, r});
    fx::emit({kStencilFunc, load<u32>(st + 160), load<u32>(st + 164), load<u32>(st + 172)});
    ref<u8>(rg::stencil_bytes) = 0;
    ref<u8>(rg::stencil_bytes + 1) = static_cast<u8>(value);
}

// RenderLayer::~RenderLayer (slot 5; the base class's 0x0079ECC4 is the same)
DECOMP(0079C260, RenderLayer_Destruct) {
    ctx.gpr[3] = destruct(ctx, arg<u32>(ctx, 0));
}
DECOMP(0079ECC4, RenderLayerBase_Destruct) {
    ctx.gpr[3] = destruct(ctx, arg<u32>(ctx, 0));
}

// Deleting destructors (slot 6; base class 0x007A108C): destruct, then
// free the object itself.
DECOMP(007A1210, RenderLayer_Delete) {
    const u32 l = arg<u32>(ctx, 0);
    if (l == 0) return;
    destruct(ctx, l);
    ctx.gpr[3] = call(ctx, Mem_FreeFrom, {l, 0});
}
DECOMP(007A108C, RenderLayerBase_Delete) {
    const u32 l = arg<u32>(ctx, 0);
    if (l == 0) return;
    destruct(ctx, l);
    ctx.gpr[3] = call(ctx, Mem_FreeFrom, {l, 0});
}
