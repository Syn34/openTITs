// World_HandleEvent callees, part 11b: three more instantiations of the
// sorted pair table lookup and the attached-effect starter.
#include "hevent11.h"

using namespace sly4;
using namespace sly4::hevent7;
using namespace sly4::hevent9;
using namespace sly4::hevent11;

namespace {
// Default compare OPDs of the three instantiations: with one of these the
// search compares the keys as plain u32s instead of calling it.
constexpr u32 kCompareU32_766 = 0x00F35FA0;
constexpr u32 kCompareU32_771 = 0x00F37C10;
constexpr u32 kCompareU32_768 = 0x00F36AC0;

// The sorted pair table lookup (as Map_Find7665B4): sorts first when
// flagged, then a binary search, with plain unsigned keys for the default
// compare and the compare OPD on (entry, &key copy) otherwise. Returns
// &entry.value, or &t->miss (+16).
void table_find(PPUContext& ctx, u32 plain_compare) {
    const u32 t = arg<u32>(ctx, 0), key = arg<u32>(ctx, 1);
    const u32 flags = load<u16>(t + kTabFlags);
    if (flags & kTabUnsorted) {
        store<u16>(t + kTabFlags, static_cast<u16>(flags & 0xFFFE));
        const u32 n = load<u16>(t + kTabCount);
        if (n != 0) call(ctx, h7fn::qsort, {load<u32>(t + kTabEntries), n, 8, load<u32>(t + kTabCompare)});
    }
    s32 hi = static_cast<s32>(load<u16>(t + kTabCount)) - 1, lo = 0;
    u32 base = load<u32>(t + kTabEntries);
    s32 found = -1;
    if (load<u32>(t + kTabCompare) == plain_compare) {
        while (hi >= lo) {
            const s32 mid = (hi + lo) >> 1;
            const u32 want = load<u32>(key), k = load<u32>(base + static_cast<u32>(mid) * 8);
            if (k > want) hi = mid - 1;
            else if (k < want) lo = mid + 1;
            else { found = mid; break; }
        }
    } else {
        Stack frame(ctx, 16);  // the key copy at r1+112
        store<u32>(frame.addr(), load<u32>(key));
        while (hi >= lo) {
            const s32 mid = (hi + lo) >> 1;
            const s32 c = static_cast<s32>(opdcall(ctx, load<u32>(t + kTabCompare), {base + static_cast<u32>(mid) * 8, frame.addr()}));
            if (c > 0) hi = mid - 1;
            else if (c < 0) lo = mid + 1;
            else {
                base = load<u32>(t + kTabEntries);
                found = mid;
                break;
            }
        }
    }
    ret(ctx, found >= 0 ? base + static_cast<u32>(found) * 8 + 4 : t + kTabMiss);
}

constexpr u32 kSetMapNode = hevent6::kSetDefs + hevent6::kMapNode;  // Set +104
constexpr u32 kEffectSize = 140;
constexpr u32 kEffectFlags = 136, kEffectFlagAttached = 0x200;
constexpr u32 kAttachStartTime = 0x00364400;  // f32 start time (lfs)
constexpr u32 kSmallAlloc = h7fn::SmallAlloc;
constexpr u32 kEffectSetTime = 0x004CF710;    // (Effect*; f32) (symbols.txt Effect_SetTime)
constexpr u32 kSetFind = 0x004BCE90;          // (u32 name) -> Set* (symbols.txt Set_Find)
}  // namespace

// void* Map_Find76642C(Table* t, const u32* key) (name?): the sorted pair
// table lookup with default compare 0x00F35FA0.
DECOMP(0076642C, Map_Find76642C) { table_find(ctx, kCompareU32_766); }

// void* SortedMap_Find771FC4(Table* t, const u32* key) (name?): the same
// with default compare 0x00F37C10.
DECOMP(00771FC4, SortedMap_Find771FC4) { table_find(ctx, kCompareU32_771); }

// void* Keys_Find768C48(Table* t, const u32* key) (name?): the same with
// default compare 0x00F36AC0.
DECOMP(00768C48, Keys_Find768C48) { table_find(ctx, kCompareU32_768); }

// void Obj_Attach364408(?, u32 hash) (name?): looks hash up in the default
// effect set (Set_Find(0)) and, for a def, starts a new 140-byte effect from
// it at the time in 0x00364400 and marks it attached (+136 bit 9). The first
// argument is unused. A failed allocation still starts and flags effect 0,
// as in the original. r3 is left as the original leaves it.
DECOMP(00364408, Obj_Attach364408) {
    const u32 hash = arg<u32>(ctx, 1);
    const u32 set = static_cast<u32>(call(ctx, kSetFind, {0}));
    if (set == 0) return ret(ctx, 0u);
    // hevent6::set_lookup, inlined to keep the r3 values of its exits.
    const u32 m = set + hevent6::kSetDefs;
    const u32 root = load<u32>(load<u32>(m));
    if (root == 0) {
        store<u32>(set + kSetMapNode, 0);
        store<u32>(hevent6::kLastDef, 0);
        return ret(ctx, set);
    }
    const u32 n = hevent6::map_walk(m, root, hash);
    if (n == 0) return ret(ctx, set);
    store<u32>(set + kSetMapNode, n);
    store<u32>(hevent6::kLastDef, load<u32>(n + hevent6::kNodeValue));
    const u32 cur = load<u32>(set + kSetMapNode);
    if (cur == 0) return ret(ctx, 0u);
    const u32 def = load<u32>(cur + hevent6::kNodeValue);
    if (def == 0) return ret(ctx, cur);
    const u32 mem = static_cast<u32>(call(ctx, kSmallAlloc, {kEffectSize}));
    u32 e = 0;
    if (mem != 0) {
        call(ctx, h7fn::Effect_Construct, {mem, def});
        e = mem;
    }
    call(ctx, h7fn::Effect_Start, {e});
    callf(ctx, kEffectSetTime, {e}, {lfs_f64(kAttachStartTime)});
    const u32 flags = load<u32>(e + kEffectFlags) | kEffectFlagAttached;
    store<u32>(e + kEffectFlags, flags);
    ret(ctx, flags);
}
