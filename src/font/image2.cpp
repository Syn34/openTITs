// Texture_ConstructBase and Texture_InitLayout.
#include "image2.h"

using namespace sly4;
using namespace sly4::image2;

// void Texture_ConstructBase(AglTexture* t)
DECOMP(0050A3E4, Texture_ConstructBase) { ret(ctx, call(ctx, i2fn::Texture_ConstructRoot, {arg<u32>(ctx, 0)})); }

// u32 Texture_InitLayout(Layout* l, u16 w, u16 h, u8 mips, u32 format, bool target, u32* out, u32 pool5)
// As a render target (Target_Init into a temporary, then
// Surface_SetBuffer; *out gets the target when given), or as a plain
// texture: format-table fields, pitch (linear formats only), the size of
// `mips` levels (only the height halves per level, as in the original),
// and memory from pool 5 (pool5 != 0) or 4 with its RSX offset.
DECOMP(00536E3C, Texture_InitLayout) {
    const u32 l = arg<u32>(ctx, 0), w = arg<u32>(ctx, 1), h = arg<u32>(ctx, 2), mips = arg<u32>(ctx, 3);
    const u32 fmt = arg<u32>(ctx, 4), out = arg<u32>(ctx, 6), pool5 = arg<u32>(ctx, 7);
    const u8 target = static_cast<u8>(arg<u32>(ctx, 5));
    Stack frame(ctx, 16);
    const u32 tmp = frame.addr();  // sp+112
    if (target != 0) {
        call(ctx, i2fn::Target_Init, {tmp, w, h, fmt, 0, pool5});
        const u64 r = call(ctx, i2fn::Surface_SetBuffer, {l, load<u32>(tmp)});
        if (out == 0) return ret(ctx, r);
        store<u32>(out, load<u32>(tmp));
        return ret(ctx, load<u32>(tmp));
    }
    const u32 e = kFormatTable + fmt * 24;
    const u32 gcm = load<u32>(e + 4), bpp = load<u32>(e + 8), remap = load<u32>(e + 16);
    store<u32>(l + kLayFormatIndex, fmt);
    store<u8>(l + kLayFormat, static_cast<u8>(gcm));
    store<u8>(l + kLayMips, static_cast<u8>(mips));
    store<u8>(l + kLayDim, 2);
    store<u8>(l + kLayCube, 0);
    store<u32>(l + kLayRemap, remap);
    store<u16>(l + kLayHeight, static_cast<u16>(h));
    store<u16>(l + kLayWidth, static_cast<u16>(w));
    store<u16>(l + kLayDepth, 1);
    store<u8>(l + 31, 0);
    store<u32>(l + kLayPitch, (gcm & 0x20) != 0 ? (w * bpp) >> 3 : 0);
    u32 size = 0;
    if (static_cast<s32>(mips) != 0) {
        const u32 row = w * bpp;
        s32 left = static_cast<s32>(mips);
        for (u32 m = 0; left > 0; ++m) {
            --left;
            const s32 hh = static_cast<s32>(h & 0xFFFF);
            const s32 shifted = (m & 63) >= 32 ? (hh < 0 ? -1 : 0) : (hh >> (m & 31));
            size += (static_cast<u32>(shifted) * row) >> 3;
        }
    }
    store<u32>(l + kLaySize, (load<u32>(l + kLaySize) & 0xFu) | (size << 4));
    store<u32>(l + kLayWord12, (load<u32>(l + kLayWord12) & 0x0FFFFFFFu) | 0x10000000u);
    const u32 bit = pool5 != 0 ? 1 : 0;
    const u32 mem = static_cast<u32>(call(ctx, i2fn::Mem_AllocPool, {128, size & 0x0FFFFFFFu, bit != 0 ? 5u : 4u, 1}));
    store<u32>(l + kLayData, mem);
    store<u32>(tmp + 4, 0);
    call(ctx, i2fn::cellGcmAddressToOffset, {mem, tmp + 4});
    const u32 off = load<u32>(tmp + 4);
    const u32 w4 = (off << 1) | bit;
    store<u32>(l + kLayOffset, w4);
    store<u32>(l + kLayGcmOffset, off & 0x7FFFFFFFu);
    store<u8>(l + kLayLocation, static_cast<u8>(w4 & 1));
    ret(ctx, off & 0x7FFFFFFFu);
}
