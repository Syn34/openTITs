// Render targets: allocation (Target_Init), copies at another size
// (Surface_Create, Target_Attach) and binding to a surface record
// (Surface_SetBuffer). See target.h for the 60-byte target layout.
#include "rsx.h"

namespace sly4::gfx {

namespace {

constexpr u32 Mem_AllocPool = 0x0056C514;     // (align, size, pool, flags)
constexpr u32 kAaTable = 0x00F6741C;          // 28-byte entries: +0 mode, +20 compressed
constexpr u32 kTileSlots = 0x00F67244;        // u16: bit i set when tile slot i (0..13) is taken
constexpr u32 kTagCursor = 0x00F67248;        // u32: next compression tag, in 64 KiB units

// The AA table entry for (format, aa), as Surface_GetSize picks it; the
// zero page for modes 1, 2 and above 4 (the original reads guest address 0).
const be_u32* aa_entry(s32 format, s32 aa) {
    u32 ea;
    if (aa > 2) ea = aa <= 3 ? kAaTable + 56 : aa > 4 ? 0 : kAaTable + 84;
    else if (aa != 0) ea = 0;
    else ea = (format == 6 || format == 4) ? kAaTable + 28 : kAaTable;
    return or_zero_page(host<be_u32>(ea));
}

// A new default target (0 if Mem_Alloc fails).
Target* new_target() {
    Target* t = call<Target*>(sly4::fn::Mem_Alloc, 60u, 0u, 16u);
    if (!t) return nullptr;
    Target_Construct(t);
    return t;
}

// Copies target `src` into `t`, in the original's load/store order.
// Words 8 and 12 keep some of t's bits; +56 becomes 1.
void copy_target(Target* t, const Target* src) {
    t = or_zero_page(t);
    src = or_zero_page(src);
    t->address = src->address.get();
    t->offset_loc = src->offset_loc.get();
    u32 w8 = (t->size_flags & 0xF) | (src->size_flags & ~0xFu);
    const u32 w12 = t->kind & 0x0FFFFFFF;
    t->size_flags = w8;
    w8 = (w8 & ~0xEu) | (src->size_flags & 0xE);
    t->size_flags = w8;
    const u32 kind = src->kind & 0xF0000000u;
    t->size_flags = w8 | 1;
    t->kind = w12 | kind;
    t->surface_type = src->surface_type;
    t->aa_mode = src->aa_mode.get();
    t->format = src->format.get();
    t->surface_format = src->surface_format.get();
    t->pitch = src->pitch.get();
    t->width = src->width.get();
    t->height = src->height.get();
    t->x = src->x.get();
    t->y = src->y.get();
    t->bpp = src->bpp;
    t->tile = src->tile;
    t->bank = src->bank;
    t->zcull = src->zcull;
    t->cull_start = src->cull_start.get();
    const s32 w52 = src->tag_base;
    *at<u8>(t, 56) = 1;
    t->tag_base = w52;
}

}  // namespace

// void Surface_Create(Target** out, Target** src, u32 w, u32 h, u32 unused, u32 skip, u32 unused)
// A copy of *src at w x h with the pitch for that size. A non-zero `skip`
// moves the start down 744 rows and sets +42 to 6 (the right eye of a
// frame-packed 3D frame).
void Surface_Create(gp<Target>* out, gp<Target>* src, u32 w, u32 h, u32, u32 skip, u32) {
    const Target* first = or_zero_page(src->get());
    const s32 aa = static_cast<s32>(first->aa_mode.get()), format = first->format;
    *out = new_target();
    copy_target(*out, *src);
    const Target* t = or_zero_page(out->get());
    const Target* s = or_zero_page(src->get());
    const bool tiled = t->tile != -1;
    GuestFrame f(8);
    be_u32* sizes = f.as<be_u32>();
    sizes[0] = 0u;
    // The size of the source, as the original works it out; nothing uses it.
    Surface_GetSize(s->width, s->height, s->format, aa, tiled, &sizes[0]);
    sizes[1] = 0u;
    Surface_GetSize(w, h, format, aa, tiled, &sizes[1]);
    or_zero_page(out->get())->pitch = sizes[1].get();
    or_zero_page(out->get())->width = static_cast<u16>(w);
    Target* t2 = or_zero_page(out->get());
    t2->height = static_cast<u16>(h);
    if (skip == 0) return;
    Target* t3 = or_zero_page(out->get());
    const u32 w4 = t3->offset_loc;
    t3->offset_loc = (((w4 >> 1) + t3->pitch * 744) << 1) | (w4 & 1);
    or_zero_page(out->get())->y = 6;
}

// void Target_Init(Target** out, u32 w, u32 h, s32 format, s32 aa, u32 where)
// A new target with its own pixels: Surface_GetSize (tiled) bytes from
// pool 5 (where == 1, main memory) or pool 4 (RSX local), 64 KiB aligned,
// the first free tile slot and, for compressed AA modes, a run of
// compression tags. Then Target_BindTile(out, 0).
void Target_Init(gp<Target>* out, u32 w, u32 h, s32 format, s32 aa, u32 where) {
    *out = new_target();
    const FormatDesc* desc = format_desc(static_cast<u32>(format));
    const be_u32* entry = aa_entry(format, aa);

    GuestFrame f(8);  // +0 pitch, +4 RSX offset
    be_u32* words = f.as<be_u32>();
    words[0] = 0u;
    const u32 size = Surface_GetSize(w, h, format, aa, true, &words[0]);
    Target* t = or_zero_page(out->get());
    const u32 w8 = (t->size_flags & 0xF) | (size << 4);
    t->kind = (t->kind & 0x0FFFFFFF) | 0x40000000;
    t->size_flags = w8;
    const bool main_memory = where == 1;
    t->address = call<u32>(Mem_AllocPool, 0x10000u, size & 0x0FFFFFFF, main_memory ? 5u : 4u, 4u);
    words[1] = 0u;
    call(fn::cellGcmAddressToOffset, t->address.get(), &words[1]);
    t->offset_loc = (words[1] << 1) | (main_memory ? 1u : 0u);

    or_zero_page(out->get())->surface_type = 1;
    Target* t2 = or_zero_page(out->get());
    be_u16& slots = global<be_u16>(kTileSlots);
    const u16 taken = slots;
    t2->aa_mode = entry[0].get();
    t2->format = format;
    t2->surface_format = desc->surface.get();
    t2->width = static_cast<u16>(w);
    t2->pitch = words[0].get();
    t2->height = static_cast<u16>(h);
    t2->x = 0;
    t2->y = 0;
    t2->bpp = static_cast<u8>(desc->bpp);

    Target* t3 = or_zero_page(out->get());
    s32 slot = -1;
    for (u32 i = 0; i < 14; ++i) {
        const u16 bit = static_cast<u16>(1u << i);
        if ((taken & bit) == 0) {
            slots = static_cast<u16>(taken | bit);
            slot = static_cast<s32>(i);
            break;
        }
    }
    t3->tile = static_cast<s8>(slot);

    const u32 compressed = entry[5];
    Target* t4 = or_zero_page(out->get());
    s32 tag = -1;
    if (compressed != 0) {
        be_s32& cursor = global<be_s32>(kTagCursor);
        tag = cursor;
        cursor = static_cast<s32>(static_cast<u32>(tag) + ((size + 0xFFFF) >> 16));
    }
    t4->tag_base = tag;
    or_zero_page(out->get())->bank = 0;
    Target_BindTile(out, false);
}

// void Target_Attach(Target** out, Target** src, u32 w, u32 h, u32 unused, u32 row)
// A copy of *src sharing its pixels, resized to w x h; a non-zero `row`
// starts it align8(row) rows further down (rows rounded up to 8).
void Target_Attach(gp<Target>* out, gp<Target>* src, u32 w, u32 h, u32, s32 row) {
    Target* t = new_target();
    *out = t;
    copy_target(t, *src);
    or_zero_page(out->get())->width = static_cast<u16>(w);
    or_zero_page(out->get())->height = static_cast<u16>(h);
    Target* dst = or_zero_page(out->get());
    if (row == 0) return;
    const u32 pitch = or_zero_page(src->get())->pitch;
    const u32 rows = ((static_cast<u32>(row) - 1) & ~7u) + 8;
    const u32 w4 = dst->offset_loc;
    const u32 moved = (w4 >> 1) + rows * pitch;
    dst->offset_loc = (moved << 1) | (w4 & 1);
}

// void Surface_SetBuffer(Texture* s, Target* t)
// Points the 52-byte surface record at target t: copies its address,
// offset and flag words, keeps t at +40 and fills the CellGcmTexture at
// +16 (type, remap, size, pitch, offset of the first row skipped, location).
void Surface_SetBuffer(Texture* s, Target* t) {
    const FormatDesc* desc = format_desc(static_cast<u32>(t->format));
    s->address = t->address.get();
    s->offset_loc = t->offset_loc.get();
    u32 w8 = (s->size_flags & 0xF) | (t->size_flags & ~0xFu);
    s->size_flags = w8;
    w8 = (w8 & ~0xEu) | (t->size_flags & 0xE);
    const u32 w12 = s->kind & 0x0FFFFFFF;
    s->size_flags = w8;
    const u32 kind = t->kind & 0xF0000000u;
    s->target = t;
    s->size_flags = w8 | 1;
    s->kind = w12 | kind;
    const u32 format = static_cast<u32>(t->format);
    s->bound = 1;
    s->format_index = format;
    const u32 offset = (t->offset_loc >> 1) + t->pitch * t->y;
    GcmTexture& d = s->gcm;
    const u32 type = desc->type;
    d.mipmap = 1;
    d.format = static_cast<u8>(type);
    d.dimension = 2;
    d.cubemap = 0;
    const Target* bound = s->target;
    d.remap = desc->remap.get();
    d.width = bound->width.get();
    const u16 height = bound->height;
    d.depth = 1;
    d.height = height;
    d.pitch = bound->pitch.get();
    const u32 location = bound->offset_loc & 1;
    d.offset = offset;
    *at<u8>(&d, 15) = 0;
    d.location = static_cast<u8>(location);
}

}  // namespace sly4::gfx

NATIVE(005304D4, sly4::gfx::Surface_Create)
NATIVE(00530DB4, sly4::gfx::Target_Init)
NATIVE(00532E70, sly4::gfx::Target_Attach)
NATIVE(00532D5C, sly4::gfx::Surface_SetBuffer)
