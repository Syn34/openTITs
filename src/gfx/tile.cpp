// Binding render targets to RSX tile and zcull regions, and dropping the
// display targets' bindings.
#include "rsx.h"

namespace sly4::gfx {

namespace {

constexpr u32 cellGcmBindTile = 0x00ABD624;     // imports
constexpr u32 cellGcmBindZcull = 0x00ABD744;
constexpr u32 cellGcmUnbindZcull = 0x00ABD7C4;
constexpr u32 cellGcmSetTileInfo = 0x00ABD824;
constexpr u32 cellGcmUnbindTile = 0x00ABD884;
constexpr u32 kAaTable = 0x00F6741C;     // 28-byte entries: +8/+12 width/height factor, +20/+24 compression modes
constexpr u32 kTileRecords = 0x010F1190; // 28-byte records per tile slot (g::mem_stats +880)

struct AaEntry {
    be_u32 mode;         // +0
    pad<4> _4;
    be_u32 width_factor;   // +8
    be_u32 height_factor;  // +12
    pad<4> _16;
    be_u32 compression[2];  // +20 / +24
};
struct TileRecord {
    be_u32 location;     // +0
    be_u32 offset;       // +4
    be_u32 size;         // +8
    be_u32 pitch;        // +12
    be_u32 compression;  // +16
    be_s32 base;         // +20
    be_s32 bank;         // +24
};
static_assert(sizeof(TileRecord) == 28);

// The AA table entry for (format, aa), as Surface_GetSize picks it; the
// zero page for AA modes with no entry (the callers then read low guest
// memory, as the original does).
const AaEntry* aa_entry(s32 format, s32 aa) {
    u32 ea;
    if (aa > 2) ea = aa <= 3 ? kAaTable + 56 : aa > 4 ? 0 : kAaTable + 84;
    else if (aa != 0) ea = 0;
    else ea = (format == 6 || format == 4) ? kAaTable + 28 : kAaTable;
    return or_zero_page(host<AaEntry>(ea));
}

}  // namespace

// s32 Target_BindTile(Target** slot, bool alt_compression)
// Sets and binds the tile region of *slot (when it has a tile slot): size
// = target +8 >> 4, compression mode from AA table +20 (or +24), base tag
// max(+52, 0), bank +46. Mirrors the settings in the tile record table.
// Returns the bank (or, with no tile slot, the target pointer).
s32 Target_BindTile(gp<Target>* slot, bool alt) {
    Target* t = or_zero_page(slot->get());
    const u8 index = static_cast<u8>(t->tile);
    if (static_cast<s8>(index) == -1) return static_cast<s32>(slot->ea.get());
    const u32 size = t->size_flags >> 4;
    const AaEntry* entry = aa_entry(t->format, static_cast<s32>(t->aa_mode.get()));
    const u32 comp = entry->compression[alt ? 1 : 0];
    const u32 w4 = t->offset_loc;
    const s32 tag = t->tag_base;
    call(cellGcmSetTileInfo, static_cast<u32>(index), w4 & 1, w4 >> 1, size, t->pitch.get(), comp & 0xFF,
         static_cast<u32>(tag < 0 ? 0 : tag) & 0xFFFF, static_cast<u32>(t->bank));
    call(cellGcmBindTile, static_cast<u32>(static_cast<u8>(or_zero_page(slot->get())->tile)));

    Target* b = or_zero_page(slot->get());
    TileRecord* rec = at<TileRecord>(host<u8>(kTileRecords), static_cast<u32>(static_cast<s32>(b->tile) * 28));
    rec->location = b->offset_loc & 1;
    rec->size = size;
    rec->offset = b->offset_loc >> 1;
    rec->compression = comp;
    rec->pitch = b->pitch.get();
    const s32 base = b->tag_base;
    rec->base = base < 0 ? 0 : base;
    const s32 bank = static_cast<s8>(b->bank);
    rec->bank = bank;
    return bank;
}

// void Target_BindZcull(Target** slot)
// Binds zcull region +47 of *slot (when it has one) over the target's
// AA-scaled size rounded up to 64, cull start +48, depth format 2, the
// AA mode, and stencil test settings (1, 1, 0, 0xFF).
void Target_BindZcull(gp<Target>* slot) {
    Target* t = or_zero_page(slot->get());
    const u8 index = static_cast<u8>(t->zcull);
    if (static_cast<s8>(index) == -1) return;
    const u32 aa = t->aa_mode;
    const u32 w = t->width, h = t->height;
    const AaEntry* entry = aa_entry(t->format, static_cast<s32>(aa));
    const u32 zw = (w * entry->width_factor + 63) & ~63u;
    const u32 zh = (h * entry->height_factor + 63) & ~63u;
    // index, offset, width, height, cull start, zFormat, aaFormat, zcullDir,
    // zcullFormat, sFunc, sRef, sMask (arguments 9-12 go on the stack).
    call(cellGcmBindZcull, static_cast<u32>(index), t->offset_loc >> 1, zw, zh, t->cull_start.get(), 2u, aa, 0u, 1u, 1u, 0u,
         255u);
}

// void Tiles_Unbind()
// Unbinds the tile regions of the display's colour targets (+16, +20) and
// the tile and zcull regions of its depth target (+24), and of the extra
// eye pair (+76, +80) when display +52 has one.
void Tiles_Unbind() {
    Display* d = display();
    auto target = [&](u32 off) { return or_zero_page(at<gp<Target>>(d, off)->get()); };
    auto unbind_tile = [&](u32 off) {
        const s8 index = target(off)->tile;
        if (index != -1) call(cellGcmUnbindTile, static_cast<u32>(static_cast<u8>(index)));
    };
    auto unbind_zcull = [&](u32 off) {
        const s8 index = target(off)->zcull;
        if (index != -1) call(cellGcmUnbindZcull, static_cast<u32>(static_cast<u8>(index)));
    };
    unbind_tile(16);
    unbind_tile(20);
    unbind_tile(24);
    unbind_zcull(24);
    const s32 pairs = static_cast<s32>((d->flags >> 22) & 7);
    if (pairs <= 1) return;
    unbind_tile(76);
    unbind_tile(80);
    unbind_zcull(80);
}

}  // namespace sly4::gfx

NATIVE(0052F32C, sly4::gfx::Target_BindTile)
NATIVE(00530998, sly4::gfx::Target_BindZcull)
NATIVE(005311D8, sly4::gfx::Tiles_Unbind)
