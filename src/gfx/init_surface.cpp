// Graphics initialisers called by Gcm_Init: surface sizing, the display
// surfaces, the frame-buffer surface statics and the RSX scratch heap.
#include "../sly4.h"

#include <atomic>

using namespace sly4;

namespace {

constexpr u32 kStats = g::mem_stats;
constexpr u32 kConfig = g::config;
constexpr u32 kDisplay = 0x00F66FBC;  // u32: the display object (buffers at +16.., flags at +52/+148)

constexpr u32 kFormatTable = 0x00F67254;  // 24-byte entries; +8 bits per pixel
constexpr u32 kAaTable = 0x00F6741C;      // 28-byte entries; +8 width factor, +12 height factor
constexpr u32 cellGcmGetTiledPitchSize = 0x00ABD584;
constexpr u32 cellGcmAddressToOffset = 0x00ABD5E4;

namespace callee {
constexpr u32 Display_CreateTargets = 0x0051B228;  // (w, h, 29, max w, max h) (name?)
constexpr u32 Surface_Create = 0x005304D4;         // (desc*, buffer*, w, h, 0, 0, 1) (name?)
constexpr u32 Display_Finish = 0x0053074C;         // (display) (name?)
constexpr u32 Target_Init = 0x00530DB4;            // (target, w, h, 3, 0, 0) (name?)
constexpr u32 Target_Attach = 0x00532E70;          // (out, target, w, h, 0, 0) (name?)
constexpr u32 Surface_SetBuffer = 0x00532D5C;      // (surface, buffer) (name?)
constexpr u32 Display_SetSize = 0x00533054;        // (display, Target** frame)
constexpr u32 Mem_AllocPool = 0x0056C514;
constexpr u32 memset = 0x00726E54;
}  // namespace callee

u32 config_size(u32 offset, u32 fallback) {
    const s32 v = static_cast<s32>(field<u32>(kConfig, offset).get());
    return v == 0 ? fallback : v > 0 ? static_cast<u32>(v) : 0;
}

// The 52-byte surface statics: what each lazy constructor writes.
struct SurfaceInit {
    u32 w4, w8, w12;
};
SurfaceInit surface_words(u32 s) {
    return {(load<u32>(s + 4) & 1) | 1, 2, load<u32>(s + 12) & 0x0FFFFFFF};
}

}  // namespace

// u32 Surface_GetSize(u32 width, u32 height, u32 format, u32 aa, bool tiled, u32* pitch)
// Pitch (width aligned to 256 / bytes per pixel) times the AA factors,
// tiled pitch if asked, height aligned to 32; rounded up to 64 KiB.
// AA modes 1 and 2 have no table entry (the original reads address 8).
DECOMP(00530318, Surface_GetSize) {
    const u32 width = arg<u32>(ctx, 0), height = arg<u32>(ctx, 1);
    const s32 format = arg<s32>(ctx, 2), aa = arg<s32>(ctx, 3);
    const bool tiled = arg<u8>(ctx, 4) != 0;
    const u32 out = arg<u32>(ctx, 5);
    const u32 desc = kFormatTable + static_cast<u32>(format) * 24;
    u32 aa_entry;
    if (aa > 2) aa_entry = aa <= 3 ? kAaTable + 56 : aa > 4 ? 0 : kAaTable + 84;
    else if (aa != 0) aa_entry = 0;
    else aa_entry = (format == 6 || format == 4) ? kAaTable + 28 : kAaTable;

    const u32 bpp = static_cast<u32>(load<s32>(desc + 8) / 8);
    const u32 align = bpp != 0 ? 256u / bpp : 0;  // divwu by 0 is undefined on the PPU
    const u32 h = (height + 31) & ~31u;
    u32 pitch = ((width + align - 1) & ~(align - 1)) * bpp;
    store<u32>(out, pitch);
    pitch *= load<u32>(aa_entry + 8);
    if (tiled) {
        store<u32>(out, pitch);
        pitch = static_cast<u32>(call(ctx, cellGcmGetTiledPitchSize, {pitch}));
    }
    const u32 total = pitch * h;
    store<u32>(out, pitch);
    ret(ctx, (total * load<u32>(aa_entry + 12) + 0xFFFF) & 0xFFFF0000u);
}

// void Display_InitSurfaces()
// Creates the display render targets at the largest of the screen and
// buffer sizes, then three colour surfaces of the buffer size.
DECOMP(00531084, Display_InitSurfaces) {
    const u16 buf_w = load<u16>(kStats + 16), buf_h = load<u16>(kStats + 18);
    const u16 scr_w = load<u16>(kStats + 180), scr_h = load<u16>(kStats + 182);
    const u32 max_w = scr_w > buf_w ? scr_w : buf_w;
    const u32 max_h = scr_h > buf_h ? scr_h : buf_h;
    call(ctx, callee::Display_CreateTargets, {load<u16>(kStats + 20), load<u16>(kStats + 22), 29, max_w, max_h});
    const u32 d = load<u32>(kDisplay);
    field<u32>(d, 56) = field<u16>(kConfig, 4);
    field<u32>(d, 60) = field<u16>(kConfig, 6);
    const u32 w = load<u16>(kStats + 16), h = load<u16>(kStats + 18);
    for (u32 i = 0; i < 3; ++i) call(ctx, callee::Surface_Create, {d + 64 + 4 * i, d + 16 + 4 * i, w, h, 0, 0, 1});
    ctx.gpr[3] = call(ctx, callee::Display_Finish, {d});
}

// void Gfx_InitFrameSurfaces()
// Binds the frame-buffer surface statics (main target at 0x011647AC, the
// main surface, a secondary and a pair) to the display object's buffers,
// picking left/right or mono buffers from the stereo config bytes.
DECOMP(005333CC, Gfx_InitFrameSurfaces) {
    const u32 d = load<u32>(kDisplay);
    const u32 w = field<u16>(kConfig, 4), h = field<u16>(kConfig, 6);
    constexpr u32 kTarget = 0x011647AC, kAttached = 0x011647B0;
    constexpr u32 kMain = 0x01164770, kMainGuard = 0x01164768;
    constexpr u32 kSecond = 0x01164730, kSecondGuard = 0x01164728;
    constexpr u32 kPair = 0x01164188, kPairGuard = 0x01164720;
    call(ctx, callee::Target_Init, {kTarget, w, h, 3, 0, 0});
    if (ref<u8>(kMainGuard) == 0) {
        const SurfaceInit v = surface_words(kMain);
        store<u32>(kMain, 0);
        store<u32>(kMain + 40, 0);
        ref<u8>(kMainGuard) = 1;
        store<u32>(kMain + 4, v.w4);
        store<u32>(kMain + 8, v.w8);
        store<u32>(kMain + 12, v.w12);
    }
    call(ctx, callee::Target_Attach, {kAttached, kTarget, w, h, 0, 0});
    call(ctx, callee::Surface_SetBuffer, {kMain, load<u32>(kAttached)});

    const u8 stereo = field<u8>(kConfig, 35), stereo_on = field<u8>(kConfig, 34);
    store<u32>(kStats + 84, kMain);
    const u32 eye_mode = load<u32>(kStats + 184);
    if (ref<u8>(kSecondGuard) == 0) {
        const SurfaceInit v = surface_words(kSecond);
        store<u32>(kSecond, 0);
        store<u32>(kSecond + 4, v.w4);
        store<u32>(kSecond + 40, 0);
        store<u32>(kSecond + 12, v.w12);
        store<u32>(kSecond + 8, v.w8);
        ref<u8>(kSecondGuard) = 1;
    }
    auto has_flag52 = [](u32 disp) { return ((load<u32>(disp + 52) >> 25) & 1) != 0; };
    auto flag148 = [](u32 disp) { return ((ref<u8>(disp + 148) >> 7) & 1) != 0; };
    u32 src;
    if (!has_flag52(d)) src = d + 24;
    else if (stereo == 0 || stereo_on == 0) src = flag148(d) ? d + 88 : d + 72;
    else src = eye_mode == 1 ? d + 120 : d + 124;
    call(ctx, callee::Surface_SetBuffer, {kSecond, load<u32>(src)});
    store<u32>(kStats + 88, kSecond);

    if (ref<u8>(kPairGuard) == 0) {
        for (u32 i = 0; i < 2; ++i) {
            const u32 s = kPair + 52 * i;
            const SurfaceInit v = surface_words(s);
            store<u32>(s, 0);
            store<u32>(s + 40, 0);
            store<u32>(s + 12, v.w12);
            store<u32>(s + 4, v.w4);
            store<u32>(s + 8, v.w8);
        }
        ref<u8>(kPairGuard) = 1;
    }
    for (u32 i = 0; i < 2; ++i) {
        const u32 disp = load<u32>(kDisplay);
        const u32 s = kPair + 52 * i;
        u32 from;
        if (!has_flag52(disp)) from = disp + 4 * i + 16;
        else if (field<u8>(kConfig, 35) == 0 || field<u8>(kConfig, 34) == 0) from = flag148(disp) ? disp + 84 : disp + 4 * i + 64;
        else from = load<u32>(kStats + 184) == 1 ? disp + 4 * i + 100 : disp + 4 * i + 108;
        call(ctx, callee::Surface_SetBuffer, {s, load<u32>(from)});
        store<u32>(kStats + 92 + 4 * i, s);
    }
    // The original passes r31 = the attached-target slot (0x011647B0), not the width.
    ctx.gpr[3] = call(ctx, callee::Display_SetSize, {d, kAttached});
}

// void RsxHeap_Init(RsxHeap* heap)
// A 2 MiB (config +460) block of RSX local memory (pool 4) with its RSX
// offset, and the heap's cursor block at +0x12080.
DECOMP(00537F28, RsxHeap_Init) {
    const u32 heap = arg<u32>(ctx, 0);
    store<u32>(heap + 0x12410, 0);
    std::atomic_ref<u32>(*reinterpret_cast<u32*>(mem::ptr(heap + 0x12480))).store(0);  // lwarx / stwcx.
    store<u32>(kStats + 368, 0);
    store<u32>(kStats + 372, 0);
    const u32 size = config_size(460, 0x200000);
    const u32 w0c = (load<u32>(heap + 0x1240C) & 0x0FFFFFFF) | (3u << 28);
    const u32 w08 = (load<u32>(heap + 0x12408) & 0xF) | ((size << 4) & 0xFFFFFFF0u);
    store<u32>(heap + 0x1240C, w0c);
    store<u32>(heap + 0x12408, w08);
    const u32 block = static_cast<u32>(call(ctx, callee::Mem_AllocPool, {0x100000, size & 0x0FFFFFFF, 4, 3}));
    store<u32>(heap + 0x12400, block);
    {
        Stack frame(ctx, 8);
        store<u32>(frame.addr(), 0);
        call(ctx, cellGcmAddressToOffset, {block, frame.addr()});
        store<u32>(heap + 0x12404, (load<u32>(frame.addr()) << 1) & ~1u);
    }
    call(ctx, callee::memset, {heap + 0x12080, 0, 896});
    const u32 base = load<u32>(heap + 0x12400);
    store<u32>(heap + 0x12080, base);
    store<u32>(heap + 0x12088, base + size);
    store<u32>(heap + 0x1208C, base);
    const u32 offset = load<u32>(heap + 0x12404);
    store<u32>(heap + 0x12094, 0);
    store<u32>(heap + 0x12090, offset & 1);
    store<u32>(heap + 0x12084, offset >> 1);
}
