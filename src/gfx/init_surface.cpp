// Graphics initialisers called by Gcm_Init: surface sizing, the display
// surfaces, the frame-buffer surface statics and the RSX scratch heap.
#include "rsx.h"

#include <atomic>

namespace sly4::gfx {

namespace {

constexpr u32 kAaTable = 0x00F6741C;  // 28-byte entries; +8 width factor, +12 height factor
constexpr u32 cellGcmGetTiledPitchSize = 0x00ABD584;
constexpr u32 Mem_AllocPool = 0x0056C514;

// g::mem_stats fields used here (beyond MemStats).
be_u16& stats_u16(u32 off) { return *at<be_u16>(&mem_stats(), off); }
be_u32& stats_u32(u32 off) { return *at<be_u32>(&mem_stats(), off); }

u32 config_size(u32 offset, u32 fallback) {
    const s32 v = static_cast<s32>(global<be_u32>(sly4::g::config + offset).get());
    return v == 0 ? fallback : v > 0 ? static_cast<u32>(v) : 0;
}

// The RSX scratch heap (offsets as the original uses them).
struct RsxHeap {
    pad<0x12080> _0;
    be_u32 cursor_base;    // +0x12080
    be_u32 cursor_offset;  // +0x12084
    be_u32 cursor_end;     // +0x12088
    be_u32 cursor_top;     // +0x1208C
    be_u32 cursor_loc;     // +0x12090
    be_u32 cursor_zero;    // +0x12094
    pad<0x12400 - 0x12098> _12098;
    be_u32 block;          // +0x12400
    be_u32 offset;         // +0x12404: RSX offset << 1
    be_u32 size_flags;     // +0x12408
    be_u32 kind;           // +0x1240C
    be_u32 w12410;         // +0x12410
    pad<0x6C> _12414;
    be_u32 lock;           // +0x12480
};
static_assert(offsetof(RsxHeap, block) == 0x12400 && offsetof(RsxHeap, lock) == 0x12480);

}  // namespace

// u32 Surface_GetSize(u32 width, u32 height, u32 format, u32 aa, bool tiled, u32* pitch)
// Pitch (width aligned to 256 / bytes per pixel) times the AA factors,
// tiled pitch if asked, height aligned to 32; rounded up to 64 KiB.
// AA modes 1 and 2 have no table entry (the original reads address 8).
u32 Surface_GetSize(u32 width, u32 height, s32 format, s32 aa, bool tiled, be_u32* out) {
    const FormatDesc* desc = format_desc(static_cast<u32>(format));
    u32 aa_entry;
    if (aa > 2) aa_entry = aa <= 3 ? kAaTable + 56 : aa > 4 ? 0 : kAaTable + 84;
    else if (aa != 0) aa_entry = 0;
    else aa_entry = (format == 6 || format == 4) ? kAaTable + 28 : kAaTable;
    const be_u32* factors = or_zero_page(host<be_u32>(aa_entry));

    const u32 bpp = static_cast<u32>(desc->bpp / 8);
    const u32 align = bpp != 0 ? 256u / bpp : 0;  // divwu by 0 is undefined on the PPU
    const u32 h = (height + 31) & ~31u;
    u32 pitch = ((width + align - 1) & ~(align - 1)) * bpp;
    *out = pitch;
    pitch *= factors[2];
    if (tiled) {
        *out = pitch;
        pitch = call<u32>(cellGcmGetTiledPitchSize, pitch);
    }
    const u32 total = pitch * h;
    *out = pitch;
    return (total * factors[3] + 0xFFFF) & 0xFFFF0000u;
}

// void Display_InitSurfaces()
// Creates the display render targets at the largest of the screen and
// buffer sizes, then three colour surfaces of the buffer size.
void Display_InitSurfaces() {
    const u16 buf_w = stats_u16(16), buf_h = stats_u16(18);
    const u16 scr_w = stats_u16(180), scr_h = stats_u16(182);
    const u32 max_w = scr_w > buf_w ? scr_w : buf_w;
    const u32 max_h = scr_h > buf_h ? scr_h : buf_h;
    Display_CreateTargets(static_cast<u32>(stats_u16(20)), static_cast<u32>(stats_u16(22)), 29u, max_w, max_h);
    Display* d = display();
    *at<be_u32>(d, 56) = static_cast<u32>(global<be_u16>(sly4::g::config + 4));
    *at<be_u32>(d, 60) = static_cast<u32>(global<be_u16>(sly4::g::config + 6));
    const u32 w = stats_u16(16), h = stats_u16(18);
    for (u32 i = 0; i < 3; ++i) Surface_Create(at<gp<Target>>(d, 64 + 4 * i), at<gp<Target>>(d, 16 + 4 * i), w, h, 0, 0, 1);
    Display_Finish(d);
}

// void Gfx_InitFrameSurfaces()
// Binds the frame-buffer surface statics (main target at 0x011647AC, the
// main surface, a secondary and a pair) to the display object's buffers,
// picking left/right or mono buffers from the stereo config bytes.
void Gfx_InitFrameSurfaces() {
    Display* d = display();
    const u32 w = global<be_u16>(sly4::g::config + 4), h = global<be_u16>(sly4::g::config + 6);
    gp<Target>* target = host<gp<Target>>(0x011647AC);
    gp<Target>* attached = host<gp<Target>>(0x011647B0);
    Texture* main = host<Texture>(0x01164770);
    u8& main_guard = global<u8>(0x01164768);
    Texture* second = host<Texture>(0x01164730);
    u8& second_guard = global<u8>(0x01164728);
    Texture* pair = host<Texture>(0x01164188);
    u8& pair_guard = global<u8>(0x01164720);
    // What each lazy constructor writes into a surface static.
    auto w4 = [](Texture* s) { return (s->offset_loc & 1) | 1; };
    auto w12 = [](Texture* s) { return s->kind & 0x0FFFFFFF; };

    Target_Init(target, w, h, 3, 0, 0);
    if (main_guard == 0) {
        const u32 a = w4(main), c = w12(main);
        main->address = 0u;
        main->target = nullptr;
        main_guard = 1;
        main->offset_loc = a;
        main->size_flags = 2u;
        main->kind = c;
    }
    Target_Attach(attached, target, w, h, 0, 0);
    Surface_SetBuffer(main, *attached);

    const u8 stereo = config_u8(35), stereo_on = config_u8(34);
    stats_u32(84) = guest(main);
    const u32 eye_mode = mem_stats().buffer_select;
    if (second_guard == 0) {
        const u32 a = w4(second), c = w12(second);
        second->address = 0u;
        second->offset_loc = a;
        second->target = nullptr;
        second->kind = c;
        second->size_flags = 2u;
        second_guard = 1;
    }
    gp<Target>* src;
    if (!per_eye(d)) src = &d->depth;
    else if (stereo == 0 || stereo_on == 0) src = alternate(d) ? &d->alt_depth : &d->eye_depth;
    else src = &d->stereo_depth[eye_mode == 1 ? 0 : 1];
    Surface_SetBuffer(second, *src);
    stats_u32(88) = guest(second);

    if (pair_guard == 0) {
        for (u32 i = 0; i < 2; ++i) {
            Texture* s = pair + i;
            const u32 a = w4(s), c = w12(s);
            s->address = 0u;
            s->target = nullptr;
            s->kind = c;
            s->offset_loc = a;
            s->size_flags = 2u;
        }
        pair_guard = 1;
    }
    for (u32 i = 0; i < 2; ++i) {
        Display* disp = display();
        Texture* s = pair + i;
        gp<Target>* from;
        if (!per_eye(disp)) from = at<gp<Target>>(disp, 4 * i + 16);
        else if (config_u8(35) == 0 || config_u8(34) == 0) from = alternate(disp) ? &disp->alt_colour : at<gp<Target>>(disp, 4 * i + 64);
        else from = at<gp<Target>>(disp, 4 * i + (mem_stats().buffer_select == 1 ? 100 : 108));
        Surface_SetBuffer(s, *from);
        stats_u32(92 + 4 * i) = guest(s);
    }
    // The original passes r31 = the attached-target slot (0x011647B0), not the width.
    Display_SetSize(d, attached);
}

// void RsxHeap_Init(RsxHeap* heap)
// A 2 MiB (config +460) block of RSX local memory (pool 4) with its RSX
// offset, and the heap's cursor block at +0x12080.
void RsxHeap_Init(RsxHeap* heap) {
    heap->w12410 = 0u;
    std::atomic_ref<u32>(*reinterpret_cast<u32*>(&heap->lock)).store(0);  // lwarx / stwcx.
    stats_u32(368) = 0u;
    stats_u32(372) = 0u;
    const u32 size = config_size(460, 0x200000);
    const u32 w0c = (heap->kind & 0x0FFFFFFF) | (3u << 28);
    const u32 w08 = (heap->size_flags & 0xF) | ((size << 4) & 0xFFFFFFF0u);
    heap->kind = w0c;
    heap->size_flags = w08;
    const u32 block = call<u32>(Mem_AllocPool, 0x100000u, size & 0x0FFFFFFF, 4u, 3u);
    heap->block = block;
    {
        GuestFrame f(8);
        be_u32* offset = f.as<be_u32>();
        *offset = 0u;
        call(fn::cellGcmAddressToOffset, block, offset);
        heap->offset = (*offset << 1) & ~1u;
    }
    call(sly4::fn::memset, &heap->cursor_base, 0u, 896u);
    const u32 base = heap->block;
    heap->cursor_base = base;
    heap->cursor_end = base + size;
    heap->cursor_top = base;
    const u32 offset = heap->offset;
    heap->cursor_zero = 0u;
    heap->cursor_loc = offset & 1;
    heap->cursor_offset = offset >> 1;
}

}  // namespace sly4::gfx

NATIVE(00530318, sly4::gfx::Surface_GetSize)
NATIVE(00531084, sly4::gfx::Display_InitSurfaces)
NATIVE(005333CC, sly4::gfx::Gfx_InitFrameSurfaces)
NATIVE(00537F28, sly4::gfx::RsxHeap_Init)
