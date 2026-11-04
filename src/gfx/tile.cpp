// Binding render targets to RSX tile and zcull regions, and dropping the
// display targets' bindings.
#include "tile.h"

using namespace sly4;

namespace {

namespace timp = sly4::tile::imp;
namespace tg = sly4::tile::g;

constexpr u32 kDisplay = 0x00F66FBC;  // AglRenderSurface*: the display (see display.h)

u64 sx(s32 v) { return static_cast<u64>(static_cast<s64>(v)); }

}  // namespace

// s32 Target_BindTile(Target** slot, bool alt_compression)
// Sets and binds the tile region of *slot (when it has a tile slot): size
// = target +8 >> 4, compression mode from AA table +20 (or +24), base tag
// max(+52, 0), bank +46. Mirrors the settings in the tile record table.
DECOMP(0052F32C, Target_BindTile) {
    const u32 slot = arg<u32>(ctx, 0);
    const bool alt = arg<u8>(ctx, 1) != 0;
    const u32 t = load<u32>(slot);
    const u8 index = ref<u8>(t + 45);
    if (static_cast<s8>(index) == -1) {
        ctx.gpr[3] = t;
        return;
    }
    const u32 size = load<u32>(t + 8) >> 4;
    const u32 entry = tile::aa_table_entry(load<s32>(t + 24), load<s32>(t + 20));
    const u32 comp = load<u32>(entry + (alt ? 24 : 20));  // entry 0 reads low guest memory, as the original does
    const u32 w4 = load<u32>(t + 4);
    const s32 tag = load<s32>(t + 52);
    call(ctx, timp::cellGcmSetTileInfo,
         {index, w4 & 1, w4 >> 1, size, load<u32>(t + 32), comp & 0xFF, static_cast<u32>(tag < 0 ? 0 : tag) & 0xFFFF,
          ref<u8>(t + 46)});
    call(ctx, timp::cellGcmBindTile, {ref<u8>(load<u32>(slot) + 45)});

    const u32 b = load<u32>(slot);
    const u32 rec = tg::tile_records + static_cast<u32>(static_cast<s32>(static_cast<s8>(ref<u8>(b + 45))) * 28);
    store<u32>(rec, load<u32>(b + 4) & 1);
    store<u32>(rec + 8, size);
    store<u32>(rec + 4, load<u32>(b + 4) >> 1);
    store<u32>(rec + 16, comp);
    store<u32>(rec + 12, load<u32>(b + 32));
    const s32 base = load<s32>(b + 52);
    store<s32>(rec + 20, base < 0 ? 0 : base);
    const s32 bank = static_cast<s8>(ref<u8>(b + 46));
    store<s32>(rec + 24, bank);
    ctx.gpr[3] = sx(bank);
}

// void Target_BindZcull(Target** slot)
// Binds zcull region +47 of *slot (when it has one) over the target's
// AA-scaled size rounded up to 64, cull start +48, depth format 2, the
// AA mode, and stencil test settings (1, 1, 0, 0xFF).
DECOMP(00530998, Target_BindZcull) {
    const u32 t = load<u32>(arg<u32>(ctx, 0));
    const u8 index = ref<u8>(t + 47);
    if (static_cast<s8>(index) == -1) {
        ctx.gpr[3] = index;
        return;
    }
    const u32 aa = load<u32>(t + 20);
    const u32 w = load<u16>(t + 36), h = load<u16>(t + 38);
    const u32 entry = tile::aa_table_entry(load<s32>(t + 24), static_cast<s32>(aa));
    const u32 zw = (w * load<u32>(entry + 8) + 63) & ~63u;
    const u32 zh = (h * load<u32>(entry + 12) + 63) & ~63u;
    // index, offset, width, height, cull start, zFormat, aaFormat, zcullDir,
    // zcullFormat, sFunc, sRef, sMask (arguments 9-12 go on the stack).
    ctx.gpr[3] = call(ctx, timp::cellGcmBindZcull,
                      {index, load<u32>(t + 4) >> 1, zw, zh, load<u32>(t + 48), 2, aa, 0, 1, 1, 0, 255});
}

// void Tiles_Unbind()
// Unbinds the tile regions of the display's colour targets (+16, +20) and
// the tile and zcull regions of its depth target (+24), and of the extra
// eye pair (+76, +80) when display +52 has one.
DECOMP(005311D8, Tiles_Unbind) {
    const u32 d = load<u32>(kDisplay);
    auto unbind_tile = [&](u32 off) {
        const u8 index = ref<u8>(load<u32>(d + off) + 45);
        if (static_cast<s8>(index) != -1) ctx.gpr[3] = call(ctx, timp::cellGcmUnbindTile, {index});
    };
    auto unbind_zcull = [&](u32 off) {
        const u8 index = ref<u8>(load<u32>(d + off) + 47);
        if (static_cast<s8>(index) != -1) ctx.gpr[3] = call(ctx, timp::cellGcmUnbindZcull, {index});
    };
    unbind_tile(16);
    unbind_tile(20);
    unbind_tile(24);
    unbind_zcull(24);
    const s32 pairs = static_cast<s32>((load<u32>(d + 52) >> 22) & 7);
    if (pairs <= 1) {
        ctx.gpr[3] = static_cast<u32>(pairs);
        return;
    }
    unbind_tile(76);
    unbind_tile(80);
    unbind_zcull(80);
}
