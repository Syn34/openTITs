// Render surfaces and default targets: the 60-byte Target defaults
// (Target_Construct), depth targets with tile + zcull (Target_InitDepth)
// and AglRenderSurface allocation from its free list.
#include "surface_alloc.h"

#include "tile.h"

using namespace sly4;

namespace {

namespace sfn = sly4::surf::fn;
namespace sg = sly4::surf::g;

u64 sx(s32 v) { return static_cast<u64>(static_cast<s64>(v)); }

u32 new_target(PPUContext& ctx) {
    const u32 t = static_cast<u32>(call(ctx, fn::Mem_Alloc, {60, 0, 16}));
    if (t == 0) return 0;
    call(ctx, sfn::Target_Construct, {t});
    return t;
}

}  // namespace

// Target* Target_Construct(Target* t)
// The default target: no pixels, format 1, AA mode 0, pitch 64, no tile,
// zcull or compression tags.
DECOMP(0053044C, Target_Construct) {
    const u32 t = arg<u32>(ctx, 0);
    const u32 w4 = (load<u32>(t + 4) & 1) | 1;
    const u32 w12 = load<u32>(t + 12) & 0x0FFFFFFF;
    store<u32>(t, 0);
    ref<u8>(t + 16) = 1;
    store<u32>(t + 4, w4);
    store<u32>(t + 8, 2);
    store<u32>(t + 12, w12);
    store<u32>(t + 20, 0);
    store<u32>(t + 24, 1);
    store<u32>(t + 28, 5);
    store<u32>(t + 32, 64);
    for (u32 off = 36; off <= 42; off += 2) store<u16>(t + off, 0);
    ref<u8>(t + 44) = 0;
    ref<u8>(t + 45) = 0xFF;
    ref<u8>(t + 46) = 0xFF;
    ref<u8>(t + 47) = 0xFF;
    store<u32>(t + 48, 0);
    store<s32>(t + 52, -1);
    ref<u8>(t + 56) = 0;
    ctx.gpr[3] = t;
}

// void Target_InitDepth(Target** out, u32 w, u32 h, s32 format, s32 aa)
// A depth target in RSX local memory (pool 4): a tile slot, compression
// tags when AA table +24 asks, bank 1, Target_BindTile(out, 1), then a
// zcull region (4 KiB-rounded, while the 3 MiB zcull space lasts) and
// Target_BindZcull.
DECOMP(00530A9C, Target_InitDepth) {
    const u32 out = arg<u32>(ctx, 0);
    const u32 w = arg<u32>(ctx, 1), h = arg<u32>(ctx, 2);
    const s32 format = arg<s32>(ctx, 3), aa = arg<s32>(ctx, 4);
    store<u32>(out, new_target(ctx));
    const u32 desc = sg::format_table + static_cast<u32>(format) * 24;
    const u32 entry = tile::aa_table_entry(format, aa);

    Stack frame(ctx, 8);  // +0 pitch, +4 RSX offset
    store<u32>(frame.addr(), 0);
    const u32 size = static_cast<u32>(call(ctx, sfn::Surface_GetSize, {w, h, sx(format), sx(aa), 1, frame.addr()}));
    const u32 t = load<u32>(out);
    const u32 w8 = (load<u32>(t + 8) & 0xF) | (size << 4);
    store<u32>(t + 12, (load<u32>(t + 12) & 0x0FFFFFFF) | 0x40000000);
    store<u32>(t + 8, w8);
    store<u32>(t, static_cast<u32>(call(ctx, sfn::Mem_AllocPool, {0x10000, size & 0x0FFFFFFF, 4, 4})));
    store<u32>(frame.addr() + 4, 0);
    call(ctx, surf::imp::cellGcmAddressToOffset, {load<u32>(t), frame.addr() + 4});
    store<u32>(t + 4, load<u32>(frame.addr() + 4) << 1);

    ref<u8>(load<u32>(out) + 16) = 1;
    const u32 t2 = load<u32>(out);
    const u16 taken = load<u16>(sg::tile_slots);
    store<u32>(t2 + 20, load<u32>(entry));  // entry 0 reads guest address 0, as the original does
    store<s32>(t2 + 24, format);
    store<u32>(t2 + 28, load<u32>(desc + 20));
    store<u16>(t2 + 36, static_cast<u16>(w));
    store<u32>(t2 + 32, load<u32>(frame.addr()));
    store<u16>(t2 + 40, 0);
    store<u16>(t2 + 38, static_cast<u16>(h));
    store<u16>(t2 + 42, 0);
    ref<u8>(t2 + 44) = static_cast<u8>(load<u32>(desc + 8));

    const u32 t3 = load<u32>(out);
    s32 slot = -1;
    for (u32 i = 0; i < 14; ++i) {
        const u16 bit = static_cast<u16>(1u << i);
        if ((taken & bit) == 0) {
            store<u16>(sg::tile_slots, static_cast<u16>(taken | bit));
            slot = static_cast<s32>(i);
            break;
        }
    }
    ref<u8>(t3 + 45) = static_cast<u8>(slot);

    const u32 compressed = load<u32>(entry + 24);
    const u32 t4 = load<u32>(out);
    s32 tag = -1;
    if (compressed != 0) {
        tag = load<s32>(sg::tag_cursor);
        store<u32>(sg::tag_cursor, static_cast<u32>(tag) + ((size + 0xFFFF) >> 16));
    }
    const u32 t5 = load<u32>(out);
    store<s32>(t4 + 52, tag);
    ref<u8>(t5 + 46) = 1;
    call(ctx, sfn::Target_BindTile, {out, 1});

    const u32 zw = (w * load<u32>(entry + 8) + 63) & ~63u;
    const u32 zh = (h * load<u32>(entry + 12) + 63) & ~63u;
    const u32 zsize = (zh * zw + 4095) & ~4095u;
    const u32 start = load<u32>(sg::zcull_cursor);
    store<u32>(load<u32>(out) + 48, start);
    const u32 end = start + zsize;
    const u32 t6 = load<u32>(out);
    s32 zcull = -1;
    if (static_cast<s32>(end) <= 0x2FFFFF) {
        zcull = load<s32>(sg::zcull_count);
        store<u32>(sg::zcull_cursor, end);
        store<s32>(sg::zcull_count, zcull + 1);
    }
    ref<u8>(t6 + 47) = static_cast<u8>(zcull);
    ctx.gpr[3] = call(ctx, sfn::Target_BindZcull, {out});
}

// AglRenderSurface* RenderSurface_Alloc()
// Takes a 156-byte AglRenderSurface from the free list at 0x00F8F110,
// making the list's allocator on first use; 0 when the list header is
// missing or the list is empty. Clears the surface's target slots.
DECOMP(0051AF28, RenderSurface_Alloc) {
    const u32 list = load<u32>(sg::surface_list);
    if (list == 0) {
        ctx.gpr[3] = 0;
        return;
    }
    u32 alloc = load<u32>(list);
    if (alloc == 0) {
        const u32 obj = static_cast<u32>(call(ctx, fn::Mem_Alloc, {60, 0, 16}));
        if (obj != 0) {
            const u32 capacity = load<u32>(list + 4);
            for (u32 off : {8u, 4u, 12u, 20u, 16u}) store<u32>(obj + off, 0);
            const u16 h54 = load<u16>(obj + 54);
            for (u32 off : {24u, 32u, 28u, 36u, 40u, 44u, 48u}) store<u32>(obj + off, 0);
            store<u16>(obj + 52, 0);
            store<u16>(obj + 54, static_cast<u16>(h54 & 0x7FFF));
            store<u32>(obj, sg::vt_surface_list);
            store<u32>(obj + 56, sg::str_surface_list);
            call(ctx, sfn::FreeList_Init, {obj, 156, capacity, 0, 0});
        }
        alloc = obj;  // 0 if Mem_Alloc failed: the original then reads guest address 0
        store<u32>(list, alloc);
    }
    const u32 s = static_cast<u32>(call_ptr(ctx, load<u32>(load<u32>(alloc) + 12), {alloc}));
    if (s == 0) {
        ctx.gpr[3] = 0;
        return;
    }
    call(ctx, sfn::RenderSurface_Construct, {s});
    const u8 b148 = ref<u8>(s + 148);
    ref<u8>(s + 136) = 0;
    ref<u8>(s + 138) = 0;
    store<u32>(s + 140, 0);
    store<u32>(s + 144, 0);
    ref<u8>(s + 137) = 1;
    store<u32>(s, sg::vt_render_surface);
    store<u32>(s + 152, 0);
    ref<u8>(s + 148) = static_cast<u8>(b148 & 0x7F);
    for (u32 off = 64; off <= 112; off += 4) store<u32>(s + off, 0);
    for (u32 off = 120; off <= 132; off += 4) store<u32>(s + off, 0);  // +116 is left as it was
    ctx.gpr[3] = s;
}

// AglRenderSurface* RenderSurface_Construct(AglRenderSurface* s)
// Base part of the render surface: a new serial (<< 11 in +8), clear
// colour bytes (64, 64, 128, 0), depth 1.0f, +52 eye-pair count 1.
DECOMP(0051AB74, RenderSurface_Construct) {
    const u32 s = arg<u32>(ctx, 0);
    const u32 w4 = (load<u32>(s + 4) & 0x03FF) | 0x00011800;
    store<u16>(s + 4, 1);
    store<u32>(s + 4, w4);
    store<u32>(s, sg::vt_surface_base0);
    const u32 serial = load<u32>(sg::surface_serial) + 1;
    store<u32>(sg::surface_serial, serial);
    const u32 w8 = load<u32>(s + 8);
    store<u32>(s + 16, 0);
    store<u32>(s + 20, 0);
    store<u32>(s + 24, 0);
    store<u32>(s, sg::vt_surface_base);
    store<u32>(s + 28, 0);
    store<u32>(s + 12, 0);
    store<u32>(s + 8, (serial << 11) | (w8 & 0x3F));
    ref<u8>(s + 32) = 64;
    ref<u8>(s + 33) = 64;
    ref<u8>(s + 34) = 128;
    ref<u8>(s + 35) = 0;
    const u32 w52 = load<u32>(s + 52);
    ref<u8>(s + 40) = 0;
    store<u32>(s + 36, 0x3F800000);  // 1.0f
    store<u32>(s + 44, 0);
    store<u32>(s + 48, 0x01010101);
    store<u32>(s + 56, 0);
    store<u32>(s + 60, 0);
    store<u32>(s + 52, (w52 & 0x003FFFFF) | 0x00400000);
    ctx.gpr[3] = s;
}
