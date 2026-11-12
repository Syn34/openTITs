// Game code, large-function sweep part 34: the B8 linear texture set a UI
// screen draws into, carved out of the surface heap.
#include "../sly4.h"
#include "big34.h"

using namespace sly4;
using namespace sly4::big34;

namespace {
constexpr u32 kSurfaceHeap = 0x01163FC8;  // the cursor allocator the buffers come from
constexpr u8 kTexFormatB8Linear = 0xA1;   // CELL_GCM_TEXTURE_B8 | CELL_GCM_TEXTURE_LN
constexpr u32 kTexRemap = 0xAAE4;         // the default (identity) remap
constexpr u8 kLocationMain = 1;           // CELL_GCM_LOCATION_MAIN

constexpr u32 Align128(u32 x) { return (x + 127) & ~127u; }
// A buffer's size: its pixels rounded up to 128 bytes, plus 128 for the
// CellGcmTexture placed right after them (0 stays 0).
constexpr u32 BufSize(u32 bytes) {
    const u32 s = Align128(bytes);
    return s != 0 ? s + 128 : 0;
}
}  // namespace

// bool T::AllocTextures(T* o) (called by Ui_OpenScreen4D245C).
// o+0 is the layer count; o+4/+8 the width/height of the full-size planes,
// o+12/+16 those of the half planes. Each layer i owns four planes, 12 bytes
// each at o + 24 + 48 i + 12 k: {enabled, data, pitch}, and four texture
// pointers at o + 120 + 20 i + 4 k (+16 is zeroed). Layer 0's enable flags
// size every layer: planes 0 and 3 have pitch (o+24 ? w : 0) | (o+60 ? w : 0),
// planes 1 and 2 pitch (o+36 ? align16(o+12) : 0); the full set is
// Cursor_Alloc(kSurfaceHeap, 64, n * stride) into o+160, rejected when null or
// in 0xC0000000..0xCFFFFFFF. Each enabled plane gets its data pointer and pitch
// and a CellGcmTexture (B8, linear, main memory) right after its pixels, whose
// offset cellGcmAddressToOffset fills in; a zero offset fails. Returns 1 with
// o+164 = the layer stride, or 0 (o+160 cleared when it was set).
DECOMP(004D10C8, UiScreen_AllocB8Textures) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 w = load<u32>(o + 4);
    const u32 pa = load<u32>(o + 24) != 0 ? w : 0;
    const u32 pb = load<u32>(o + 60) != 0 ? w : 0;
    const u32 pitchHalf = load<u32>(o + 36) != 0 ? (load<u32>(o + 12) + 15) & ~15u : 0;
    const u32 h = load<u32>(o + 8);
    const u32 s0 = BufSize(h * pa);
    const u32 s3 = BufSize(pb * h);
    const u32 s12 = BufSize(load<u32>(o + 16) * pitchHalf);
    const u32 off3 = s0;               // plane 3
    const u32 off1 = s3 + s0;          // plane 1
    const u32 off2 = off1 + s12;       // plane 2
    const u32 stride = off1 + 2 * s12;
    const u32 pitchFull = pa | pb;

    u32 cur = static_cast<u32>(call(ctx, b34fn::Cursor_Alloc, {kSurfaceHeap, 64u, static_cast<u32>(stride * load<u32>(o))}));
    store<u32>(o + 160, cur);
    bool ok = cur != 0 && !(cur >= 0xC0000000u && cur < 0xD0000000u);
    if (ok) {
        cur = Align128(cur);
        // One plane: data/pitch into its slot, the texture after the pixels,
        // its pointer into *texSlot. False when the offset comes back 0.
        auto plane = [&](u32 slot, u32 texSlot, u32 data, u32 pitch, u32 wOff, u32 hOff) {
            store<u32>(slot + 4, data);
            store<u32>(slot + 8, pitch);
            const u32 hh = load<u32>(o + hOff), ww = load<u32>(o + wOff);
            const u32 tex = data + Align128(hh * pitch);
            store<u8>(tex + 0, kTexFormatB8Linear);
            store<u8>(tex + 1, 1);  // mipmap
            store<u8>(tex + 2, 2);  // dimension: 2D
            store<u8>(tex + 3, 0);  // cubemap
            store<u32>(tex + 4, kTexRemap);
            store<u16>(tex + 8, static_cast<u16>(ww));
            store<u16>(tex + 10, static_cast<u16>(hh));
            store<u16>(tex + 12, 1);  // depth
            store<u8>(tex + 14, kLocationMain);
            store<u32>(tex + 16, pitch);
            call(ctx, b34fn::cellGcmAddressToOffset, {data, tex + 20});
            if (load<u32>(tex + 20) == 0) return false;
            store<u32>(texSlot, tex);
            return true;
        };
        for (u32 i = 0; static_cast<s32>(i) < static_cast<s32>(load<u32>(o)); ++i) {
            const u32 l = o + 24 + 48 * i, t = o + 120 + 20 * i;
            if (load<u32>(l) != 0 && !plane(l, t, cur, pitchFull, 4, 8)) { ok = false; break; }
            if (load<u32>(l + 12) != 0 && !plane(l + 12, t + 4, cur + off1, pitchHalf, 12, 16)) { ok = false; break; }
            if (load<u32>(l + 24) != 0 && !plane(l + 24, t + 8, cur + off2, pitchHalf, 12, 16)) { ok = false; break; }
            if (load<u32>(l + 36) != 0 && !plane(l + 36, t + 12, cur + off3, pitchFull, 4, 8)) { ok = false; break; }
            store<u32>(t + 16, 0);
            cur += stride;
        }
    }
    if (!ok) {
        if (load<u32>(o + 160) != 0) store<u32>(o + 160, 0);
        return ret(ctx, 0u);
    }
    store<u32>(o + 164, stride);
    ret(ctx, 1u);
}
