// Render targets: allocation (Target_Init), copies at another size
// (Surface_Create, Target_Attach) and binding to a surface record
// (Surface_SetBuffer). See target.h for the 60-byte target layout.
#include "target.h"

using namespace sly4;

namespace {

namespace tfn = sly4::tgt::fn;
namespace tg = sly4::tgt::g;

u64 sx(s32 v) { return static_cast<u64>(static_cast<s64>(v)); }

// The AA table entry for (format, aa), as Surface_GetSize picks it.
// 0 for modes 1, 2 and above 4: callers then read guest address 0.
u32 aa_entry(s32 format, s32 aa) {
    if (aa > 2) return aa <= 3 ? tg::aa_table + 56 : aa > 4 ? 0 : tg::aa_table + 84;
    if (aa != 0) return 0;
    return (format == 6 || format == 4) ? tg::aa_table + 28 : tg::aa_table;
}

// A new default target (0 if Mem_Alloc fails).
u32 new_target(PPUContext& ctx) {
    const u32 t = static_cast<u32>(call(ctx, fn::Mem_Alloc, {60, 0, 16}));
    if (t == 0) return 0;
    call(ctx, tfn::Target_Construct, {t});
    return t;
}

// Copies target `src` into `t`, in the original's load/store order.
// Words 8 and 12 keep some of t's bits; +56 becomes 1.
void copy_target(u32 t, u32 src) {
    store<u32>(t, load<u32>(src));
    store<u32>(t + 4, load<u32>(src + 4));
    u32 w8 = (load<u32>(t + 8) & 0xF) | (load<u32>(src + 8) & ~0xFu);
    const u32 w12 = load<u32>(t + 12) & 0x0FFFFFFF;
    store<u32>(t + 8, w8);
    w8 = (w8 & ~0xEu) | (load<u32>(src + 8) & 0xE);
    store<u32>(t + 8, w8);
    const u32 kind = load<u32>(src + 12) & 0xF0000000u;
    store<u32>(t + 8, w8 | 1);
    store<u32>(t + 12, w12 | kind);
    ref<u8>(t + 16) = ref<u8>(src + 16);
    for (u32 off = 20; off <= 32; off += 4) store<u32>(t + off, load<u32>(src + off));
    for (u32 off = 36; off <= 42; off += 2) store<u16>(t + off, load<u16>(src + off));
    for (u32 off = 44; off <= 47; ++off) ref<u8>(t + off) = ref<u8>(src + off);
    store<u32>(t + 48, load<u32>(src + 48));
    const u32 w52 = load<u32>(src + 52);
    ref<u8>(t + 56) = 1;
    store<u32>(t + 52, w52);
}

}  // namespace

// void Surface_Create(Target** out, Target** src, u32 w, u32 h, u32 unused, u32 skip, u32 unused)
// A copy of *src at w x h with the pitch for that size. A non-zero `skip`
// moves the start down 744 rows and sets +42 to 6 (the right eye of a
// frame-packed 3D frame). r7 and r9 are passed but never read.
DECOMP(005304D4, Surface_Create) {
    const u32 out = arg<u32>(ctx, 0), src = arg<u32>(ctx, 1);
    const u32 w = arg<u32>(ctx, 2), h = arg<u32>(ctx, 3);
    const u32 skip = arg<u32>(ctx, 5);
    const u32 first = load<u32>(src);
    const s32 aa = load<s32>(first + 20), format = load<s32>(first + 24);
    store<u32>(out, new_target(ctx));
    copy_target(load<u32>(out), load<u32>(src));
    const u32 t = load<u32>(out), s = load<u32>(src);
    const bool tiled = static_cast<s8>(ref<u8>(t + 45)) != -1;
    Stack frame(ctx, 8);
    store<u32>(frame.addr(), 0);
    // The size of the source, as the original works it out; nothing uses it.
    call(ctx, tfn::Surface_GetSize,
         {load<u16>(s + 36), load<u16>(s + 38), sx(load<s32>(s + 24)), sx(aa), tiled ? 1u : 0u, frame.addr()});
    store<u32>(frame.addr() + 4, 0);
    call(ctx, tfn::Surface_GetSize, {w, h, sx(format), sx(aa), tiled ? 1u : 0u, frame.addr() + 4});
    store<u32>(load<u32>(out) + 32, load<u32>(frame.addr() + 4));
    store<u16>(load<u32>(out) + 36, static_cast<u16>(w));
    const u32 t2 = load<u32>(out);
    store<u16>(t2 + 38, static_cast<u16>(h));
    if (skip == 0) {
        ctx.gpr[3] = t2;
        return;
    }
    const u32 t3 = load<u32>(out);
    const u32 w4 = load<u32>(t3 + 4);
    store<u32>(t3 + 4, (((w4 >> 1) + load<u32>(t3 + 32) * 744) << 1) | (w4 & 1));
    store<u16>(load<u32>(out) + 42, 6);
    ctx.gpr[3] = 6;
}

// void Target_Init(Target** out, u32 w, u32 h, s32 format, s32 aa, u32 where)
// A new target with its own pixels: Surface_GetSize (tiled) bytes from
// pool 5 (where == 1, main memory) or pool 4 (RSX local), 64 KiB aligned,
// the first free tile slot and, for compressed AA modes, a run of
// compression tags. Then Target_BindTile(out, 0).
DECOMP(00530DB4, Target_Init) {
    const u32 out = arg<u32>(ctx, 0);
    const u32 w = arg<u32>(ctx, 1), h = arg<u32>(ctx, 2);
    const s32 format = arg<s32>(ctx, 3), aa = arg<s32>(ctx, 4);
    const u32 where = arg<u32>(ctx, 5);
    store<u32>(out, new_target(ctx));
    const u32 desc = tg::format_table + static_cast<u32>(format) * 24;
    const u32 entry = aa_entry(format, aa);

    Stack frame(ctx, 8);  // +0 pitch, +4 RSX offset
    store<u32>(frame.addr(), 0);
    const u32 size = static_cast<u32>(call(ctx, tfn::Surface_GetSize, {w, h, sx(format), sx(aa), 1, frame.addr()}));
    const u32 t = load<u32>(out);
    const u32 w8 = (load<u32>(t + 8) & 0xF) | (size << 4);
    store<u32>(t + 12, (load<u32>(t + 12) & 0x0FFFFFFF) | 0x40000000);
    store<u32>(t + 8, w8);
    const bool main_memory = where == 1;
    store<u32>(t, static_cast<u32>(call(ctx, tfn::Mem_AllocPool,
                                        {0x10000, size & 0x0FFFFFFF, main_memory ? 5u : 4u, 4})));
    store<u32>(frame.addr() + 4, 0);
    call(ctx, tgt::imp::cellGcmAddressToOffset, {load<u32>(t), frame.addr() + 4});
    store<u32>(t + 4, (load<u32>(frame.addr() + 4) << 1) | (main_memory ? 1u : 0u));

    ref<u8>(load<u32>(out) + 16) = 1;
    const u32 t2 = load<u32>(out);
    const u16 taken = load<u16>(tg::tile_slots);
    store<u32>(t2 + 20, load<u32>(entry));  // entry 0 reads guest address 0, as the original does
    store<s32>(t2 + 24, format);
    store<u32>(t2 + 28, load<u32>(desc + 20));
    store<u16>(t2 + 36, static_cast<u16>(w));
    store<u32>(t2 + 32, load<u32>(frame.addr()));
    store<u16>(t2 + 38, static_cast<u16>(h));
    store<u16>(t2 + 40, 0);
    store<u16>(t2 + 42, 0);
    ref<u8>(t2 + 44) = static_cast<u8>(load<u32>(desc + 8));

    const u32 t3 = load<u32>(out);
    s32 slot = -1;
    for (u32 i = 0; i < 14; ++i) {
        const u16 bit = static_cast<u16>(1u << i);
        if ((taken & bit) == 0) {
            store<u16>(tg::tile_slots, static_cast<u16>(taken | bit));
            slot = static_cast<s32>(i);
            break;
        }
    }
    ref<u8>(t3 + 45) = static_cast<u8>(slot);

    const u32 compressed = load<u32>(entry + 20);
    const u32 t4 = load<u32>(out);
    s32 tag = -1;
    if (compressed != 0) {
        tag = load<s32>(tg::tag_cursor);
        store<u32>(tg::tag_cursor, static_cast<u32>(tag) + ((size + 0xFFFF) >> 16));
    }
    store<s32>(t4 + 52, tag);
    ref<u8>(load<u32>(out) + 46) = 0;
    ctx.gpr[3] = call(ctx, tfn::Target_BindTile, {out, 0});
}

// void Target_Attach(Target** out, Target** src, u32 w, u32 h, u32 unused, u32 row)
// A copy of *src sharing its pixels, resized to w x h; a non-zero `row`
// starts it align8(row) rows further down (rows rounded up to 8).
// r7 is passed but never read.
DECOMP(00532E70, Target_Attach) {
    const u32 out = arg<u32>(ctx, 0), src = arg<u32>(ctx, 1);
    const u32 w = arg<u32>(ctx, 2), h = arg<u32>(ctx, 3);
    const s32 row = arg<s32>(ctx, 5);
    const u32 t = new_target(ctx);
    store<u32>(out, t);
    copy_target(t, load<u32>(src));
    store<u16>(load<u32>(out) + 36, static_cast<u16>(w));
    store<u16>(load<u32>(out) + 38, static_cast<u16>(h));
    const u32 dst = load<u32>(out);
    if (row == 0) {
        ctx.gpr[3] = load<u32>(out);
        return;
    }
    const u32 pitch = load<u32>(load<u32>(src) + 32);
    const u32 rows = ((static_cast<u32>(row) - 1) & ~7u) + 8;
    const u32 w4 = load<u32>(dst + 4);
    const u32 moved = (w4 >> 1) + rows * pitch;
    store<u32>(dst + 4, (moved << 1) | (w4 & 1));
    ctx.gpr[3] = moved;
}

// void Surface_SetBuffer(Surface* s, Target* t)
// Points the 52-byte surface record at target t: copies its address,
// offset and flag words, keeps t at +40 and fills the RSX surface
// descriptor at +16 (type, format words, size, pitch, offset of the
// first row skipped, location).
DECOMP(00532D5C, Surface_SetBuffer) {
    const u32 s = arg<u32>(ctx, 0), t = arg<u32>(ctx, 1);
    const u32 desc = tg::format_table + load<u32>(t + 24) * 24;
    store<u32>(s, load<u32>(t));
    store<u32>(s + 4, load<u32>(t + 4));
    u32 w8 = (load<u32>(s + 8) & 0xF) | (load<u32>(t + 8) & ~0xFu);
    store<u32>(s + 8, w8);
    w8 = (w8 & ~0xEu) | (load<u32>(t + 8) & 0xE);
    const u32 w12 = load<u32>(s + 12) & 0x0FFFFFFF;
    store<u32>(s + 8, w8);
    const u32 kind = load<u32>(t + 12) & 0xF0000000u;
    store<u32>(s + 40, t);
    store<u32>(s + 8, w8 | 1);
    store<u32>(s + 12, w12 | kind);
    const u32 format = load<u32>(t + 24);
    ref<u8>(s + 48) = 1;
    store<u32>(s + 44, format);
    const u32 offset = (load<u32>(t + 4) >> 1) + load<u32>(t + 32) * load<u16>(t + 42);
    const u32 d = s + 16;
    const u32 type = load<u32>(desc + 4);
    ref<u8>(d + 1) = 1;
    ref<u8>(d + 0) = static_cast<u8>(type);
    ref<u8>(d + 2) = 2;
    ref<u8>(d + 3) = 0;
    const u32 bound = load<u32>(s + 40);
    store<u32>(d + 4, load<u32>(desc + 16));
    store<u16>(d + 8, load<u16>(bound + 36));
    const u16 height = load<u16>(bound + 38);
    store<u16>(d + 12, 1);
    store<u16>(d + 10, height);
    store<u32>(d + 16, load<u32>(bound + 32));
    const u32 location = load<u32>(bound + 4) & 1;
    store<u32>(d + 20, offset);
    ref<u8>(d + 15) = 0;
    ref<u8>(d + 14) = static_cast<u8>(location);
    ctx.gpr[3] = location;
}
