// Render surfaces and default targets: the 60-byte Target defaults
// (Target_Construct), depth targets with tile + zcull (Target_InitDepth)
// and AglRenderSurface allocation from its free list.
#include "rsx.h"

namespace sly4::gfx {

namespace {

constexpr u32 Mem_AllocPool = 0x0056C514;  // (align, size, pool, flags)
constexpr u32 FreeList_Init = 0x00551A58;  // (list, elem_size, count, heap, bool)
constexpr u32 kAaTable = 0x00F6741C;
constexpr u32 kTileSlots = 0x00F67244;    // u16: bit i set when tile slot i (0..13) is taken
constexpr u32 kTagCursor = 0x00F67248;    // u32: next compression tag, in 64 KiB units
constexpr u32 kZcullCount = 0x00F6724C;   // u32: zcull regions handed out
constexpr u32 kZcullCursor = 0x00F67250;  // u32: next free zcull memory (limit 0x300000)
constexpr u32 kSurfaceList = 0x00F8F110;  // SurfaceList*: AglRenderSurface free-list header
constexpr u32 kSurfaceSerial = 0x01164CD0;  // u32: last serial given to a render surface
constexpr u32 kVtSurfaceList = 0x00B23D80;   // ApeTFreeList<AglRenderSurface>
constexpr u32 kVtSurfaceBase0 = 0x00B242B8;  // written first by RenderSurface_Construct
constexpr u32 kVtSurfaceBase = 0x00B234B0;
constexpr u32 kVtRenderSurface = 0x00B23F20;
constexpr u32 kStrSurfaceList = 0x00AF1141;  // "ApeTFreeList<T> &ApeTListAllocator<T>::GetAllocator() [with T = AglRenderSurface]"

struct SurfaceList {
    gp<u8> allocator;  // +0: the free list (vtable slot 12 takes one)
    be_u32 capacity;   // +4
};

// The AA table entry for (format, aa); the zero page for modes with none.
const be_u32* aa_entry(s32 format, s32 aa) {
    u32 ea;
    if (aa > 2) ea = aa <= 3 ? kAaTable + 56 : aa > 4 ? 0 : kAaTable + 84;
    else if (aa != 0) ea = 0;
    else ea = (format == 6 || format == 4) ? kAaTable + 28 : kAaTable;
    return or_zero_page(host<be_u32>(ea));
}

be_u32& word(void* base, u32 off) { return *at<be_u32>(base, off); }
be_u16& half(void* base, u32 off) { return *at<be_u16>(base, off); }
u8& byte(void* base, u32 off) { return *at<u8>(base, off); }

}  // namespace

Display* RenderSurface_Construct(Display* s);

// Target* Target_Construct(Target* t)  (declared in rsx.h)
// The default target: no pixels, format 1, AA mode 0, pitch 64, no tile,
// zcull or compression tags.
Target* Target_Construct(Target* t) {
    const u32 w4 = (t->offset_loc & 1) | 1;
    const u32 w12 = t->kind & 0x0FFFFFFF;
    t->address = 0u;
    t->surface_type = 1;
    t->offset_loc = w4;
    t->size_flags = 2u;
    t->kind = w12;
    t->aa_mode = 0u;
    t->format = 1;
    t->surface_format = 5u;
    t->pitch = 64u;
    t->width = 0;
    t->height = 0;
    t->x = 0;
    t->y = 0;
    t->bpp = 0;
    t->tile = -1;
    t->bank = 0xFF;
    t->zcull = -1;
    t->cull_start = 0u;
    t->tag_base = -1;
    byte(t, 56) = 0;
    return t;
}

// void Target_InitDepth(Target** out, u32 w, u32 h, s32 format, s32 aa)
// A depth target in RSX local memory (pool 4): a tile slot, compression
// tags when AA table +24 asks, bank 1, Target_BindTile(out, 1), then a
// zcull region (4 KiB-rounded, while the 3 MiB zcull space lasts) and
// Target_BindZcull.
void Target_InitDepth(gp<Target>* out, u32 w, u32 h, s32 format, s32 aa) {
    Target* nt = call<Target*>(sly4::fn::Mem_Alloc, 60u, 0u, 16u);
    if (nt) Target_Construct(nt);
    *out = nt;
    auto t_ = [&] { return or_zero_page(out->get()); };
    const FormatDesc* desc = format_desc(static_cast<u32>(format));
    const be_u32* entry = aa_entry(format, aa);
    GuestFrame f(8);  // +0 pitch, +4 RSX offset
    be_u32* words = f.as<be_u32>();
    words[0] = 0u;
    const u32 size = Surface_GetSize(w, h, format, aa, true, &words[0]);
    Target* t = t_();
    const u32 w8 = (t->size_flags & 0xF) | (size << 4);
    t->kind = (t->kind & 0x0FFFFFFF) | 0x40000000;
    t->size_flags = w8;
    t->address = call<u32>(Mem_AllocPool, 0x10000u, size & 0x0FFFFFFF, 4u, 4u);
    words[1] = 0u;
    call(fn::cellGcmAddressToOffset, t->address.get(), &words[1]);
    t->offset_loc = words[1] << 1;
    t_()->surface_type = 1;
    Target* t2 = t_();
    be_u16& slots = global<be_u16>(kTileSlots);
    const u16 taken = slots;
    t2->aa_mode = entry[0].get();
    t2->format = format;
    t2->surface_format = desc->surface.get();
    t2->width = static_cast<u16>(w);
    t2->pitch = words[0].get();
    t2->x = 0;
    t2->height = static_cast<u16>(h);
    t2->y = 0;
    t2->bpp = static_cast<u8>(desc->bpp);
    Target* t3 = t_();
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
    const u32 compressed = entry[6];
    Target* t4 = t_();
    s32 tag = -1;
    if (compressed != 0) {
        be_s32& cursor = global<be_s32>(kTagCursor);
        tag = cursor;
        cursor = static_cast<s32>(static_cast<u32>(tag) + ((size + 0xFFFF) >> 16));
    }
    Target* t5 = t_();
    t4->tag_base = tag;
    t5->bank = 1;
    Target_BindTile(out, true);
    const u32 zw = (w * entry[2] + 63) & ~63u;
    const u32 zh = (h * entry[3] + 63) & ~63u;
    const u32 zsize = (zh * zw + 4095) & ~4095u;
    be_u32& zcursor = global<be_u32>(kZcullCursor);
    const u32 start = zcursor;
    t_()->cull_start = start;
    const u32 end = start + zsize;
    Target* t6 = t_();
    s32 zcull = -1;
    if (static_cast<s32>(end) <= 0x2FFFFF) {
        be_s32& count = global<be_s32>(kZcullCount);
        zcull = count;
        zcursor = end;
        count = zcull + 1;
    }
    t6->zcull = static_cast<s8>(zcull);
    Target_BindZcull(out);
}

// AglRenderSurface* RenderSurface_Alloc()
// Takes a 156-byte AglRenderSurface from the free list, making the
// list's allocator on first use; 0 when the list header is missing or the
// list is empty. Clears the surface's target slots.
Display* RenderSurface_Alloc() {
    SurfaceList* list = global<gp<SurfaceList>>(kSurfaceList);
    if (!list) return nullptr;
    u8* alloc = list->allocator;
    if (!alloc) {
        u8* obj = call<u8*>(sly4::fn::Mem_Alloc, 60u, 0u, 16u);
        if (obj) {
            const u32 capacity = list->capacity;
            for (u32 off : {8u, 4u, 12u, 20u, 16u}) word(obj, off) = 0u;
            const u16 h54 = half(obj, 54);
            for (u32 off : {24u, 32u, 28u, 36u, 40u, 44u, 48u}) word(obj, off) = 0u;
            half(obj, 52) = 0;
            half(obj, 54) = static_cast<u16>(h54 & 0x7FFF);
            word(obj, 0) = kVtSurfaceList;
            word(obj, 56) = kStrSurfaceList;
            call(FreeList_Init, obj, 156u, capacity, 0u, 0u);
        }
        alloc = obj;  // 0 if Mem_Alloc failed: the original then reads guest address 0
        list->allocator = alloc;
    }
    Display* s = vcall<Display*>(or_zero_page(alloc), 12);
    if (!s) return nullptr;
    RenderSurface_Construct(s);
    const u8 b148 = s->flags148;
    byte(s, 136) = 0;
    byte(s, 138) = 0;
    word(s, 140) = 0u;
    word(s, 144) = 0u;
    byte(s, 137) = 1;
    word(s, 0) = kVtRenderSurface;
    word(s, 152) = 0u;
    s->flags148 = static_cast<u8>(b148 & 0x7F);
    for (u32 off = 64; off <= 112; off += 4) word(s, off) = 0u;
    for (u32 off = 120; off <= 132; off += 4) word(s, off) = 0u;  // +116 is left as it was
    return s;
}

// AglRenderSurface* RenderSurface_Construct(AglRenderSurface* s)
// Base part of the render surface: a new serial (<< 11 in +8), clear
// colour bytes (64, 64, 128, 0), depth 1.0f, +52 eye-pair count 1.
Display* RenderSurface_Construct(Display* s) {
    const u32 w4 = (word(s, 4) & 0x03FF) | 0x00011800;
    half(s, 4) = 1;
    word(s, 4) = w4;
    word(s, 0) = kVtSurfaceBase0;
    be_u32& serial_ref = global<be_u32>(kSurfaceSerial);
    const u32 serial = serial_ref + 1;
    serial_ref = serial;
    const u32 w8 = word(s, 8);
    word(s, 16) = 0u;
    word(s, 20) = 0u;
    word(s, 24) = 0u;
    word(s, 0) = kVtSurfaceBase;
    word(s, 28) = 0u;
    word(s, 12) = 0u;
    word(s, 8) = (serial << 11) | (w8 & 0x3F);
    byte(s, 32) = 64;
    byte(s, 33) = 64;
    byte(s, 34) = 128;
    byte(s, 35) = 0;
    const u32 w52 = s->flags;
    byte(s, 40) = 0;
    word(s, 36) = 0x3F800000u;  // 1.0f
    word(s, 44) = 0u;
    word(s, 48) = 0x01010101u;
    word(s, 56) = 0u;
    word(s, 60) = 0u;
    s->flags = (w52 & 0x003FFFFF) | 0x00400000;
    return s;
}

}  // namespace sly4::gfx

NATIVE(0053044C, sly4::gfx::Target_Construct)
NATIVE(00530A9C, sly4::gfx::Target_InitDepth)
NATIVE(0051AF28, sly4::gfx::RenderSurface_Alloc)
NATIVE(0051AB74, sly4::gfx::RenderSurface_Construct)
