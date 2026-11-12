// Texture_ConstructBase and Texture_InitLayout.
#include "image2.h"

#include "../memory/mem.h"

namespace sly4::image2 {
using namespace opentit::native;

u32 Texture_ConstructBase(void* t) { return call<u32>(i2fn::Texture_ConstructRoot, t); }

// As a render target (Target_Init into a temporary, then
// Surface_SetBuffer; *out gets the target when given), or as a plain
// texture: format-table fields, pitch (linear formats only), the size of
// `mips` levels (only the height halves per level, as in the original),
// and memory from pool 5 (pool5 != 0) or 4 with its RSX offset. `w` / `h`
// are u16 and `mips` a u8 in the original; `target` is tested by its low byte.
u32 Texture_InitLayout(TexLayout* l, u32 w, u32 h, u32 mips, u32 fmt, u32 target, be_u32* out, u32 pool5) {
    GuestFrame frame(16);
    be_u32* tmp = frame.as<be_u32>();  // sp+112
    if (static_cast<u8>(target) != 0) {
        call(i2fn::Target_Init, tmp, w, h, fmt, 0u, pool5);
        const u32 r = call<u32>(i2fn::Surface_SetBuffer, l, tmp[0].get());
        if (!out) return r;
        *out = tmp[0].get();
        return tmp[0];
    }
    const u32 e = kFormatTable + fmt * 24;
    const u32 gcm = gref<be_u32>(e + 4), bpp = gref<be_u32>(e + 8), remap = gref<be_u32>(e + 16);
    l->format_index = fmt;
    l->format = static_cast<u8>(gcm);
    l->mips = static_cast<u8>(mips);
    l->dim = 2;
    l->cube = 0;
    l->remap = remap;
    l->height = static_cast<u16>(h);
    l->width = static_cast<u16>(w);
    l->depth = 1;
    l->b31 = 0;
    l->pitch = (gcm & 0x20) != 0 ? (w * bpp) >> 3 : 0u;
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
    l->size = (l->size & 0xFu) | (size << 4);
    l->word12 = (l->word12 & 0x0FFFFFFFu) | 0x10000000u;
    const u32 bit = pool5 != 0 ? 1 : 0;
    const u32 mem = guest(memory::Mem_AllocPool(128, size & 0x0FFFFFFFu, bit != 0 ? 5 : 4, 1));
    l->data = mem;
    tmp[1] = 0u;
    call(i2fn::cellGcmAddressToOffset, mem, tmp + 1);
    const u32 off = tmp[1];
    const u32 w4 = (off << 1) | bit;
    l->offset = w4;
    l->gcm_offset = off & 0x7FFFFFFFu;
    l->location = static_cast<u8>(w4 & 1);
    return off & 0x7FFFFFFFu;
}

}  // namespace sly4::image2

NATIVE(0050A3E4, sly4::image2::Texture_ConstructBase)
NATIVE(00536E3C, sly4::image2::Texture_InitLayout)
