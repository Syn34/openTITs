// RSX tile and zcull bindings for render targets (see target.h for the
// 60-byte Target: +45 tile slot, +46 bank, +47 zcull slot, +48 cull start,
// +52 compression tag base).
#pragma once

#include "../sly4.h"

namespace sly4::tile {

namespace imp {
constexpr u32 cellGcmBindTile = 0x00ABD624;
constexpr u32 cellGcmBindZcull = 0x00ABD744;
constexpr u32 cellGcmUnbindZcull = 0x00ABD7C4;
constexpr u32 cellGcmSetTileInfo = 0x00ABD824;
constexpr u32 cellGcmUnbindTile = 0x00ABD884;
}  // namespace imp

namespace fn {
constexpr u32 Target_BindTile = 0x0052F32C;   // decompiled: src/gfx/tile.cpp
constexpr u32 Target_BindZcull = 0x00530998;  // decompiled: src/gfx/tile.cpp
constexpr u32 Tiles_Unbind = 0x005311D8;      // decompiled: src/gfx/tile.cpp
}  // namespace fn

namespace g {
constexpr u32 aa_table = 0x00F6741C;      // 28-byte entries: +8/+12 width/height factor, +20/+24 compression modes
constexpr u32 tile_records = 0x010F1190;  // 28-byte records per tile slot (g::mem_stats +880)
}  // namespace g

// The AA table entry for (format, aa), as Surface_GetSize picks it; 0 for
// AA modes with no entry (the callers then read low guest memory).
inline u32 aa_table_entry(s32 format, s32 aa) {
    if (aa > 2) return aa <= 3 ? g::aa_table + 56 : aa > 4 ? 0 : g::aa_table + 84;
    if (aa != 0) return 0;
    return (format == 6 || format == 4) ? g::aa_table + 28 : g::aa_table;
}

}  // namespace sly4::tile
