// World objects, part 16: the sorted map instances of the object
// definition / instance maps (obj.h layout). Every instance is the same
// template code and only differs in the default comparator it tests for:
// with that comparator the keys are compared inline (unsigned, the key
// re-read each step), otherwise the map's compare is called on (entry, a
// stack copy of the key) per step.
#include "obj16.h"

using namespace sly4;
using namespace sly4::obj16;

namespace {

using obj::kMapCompare;
using obj::kMapCount;
using obj::kMapDefault;
using obj::kMapEntries;
using obj::kMapEntrySize;
using obj::kMapFlags;

// Sorts the entries (qsort with the map's compare) when flagged unsorted.
void SortIfDirty(PPUContext& ctx, u32 m) {
    const u32 flags = load<u16>(m + kMapFlags);
    if ((flags & 1) == 0) return;
    store<u16>(m + kMapFlags, static_cast<u16>(flags & 0xFFFE));  // rlwinm 0,16,30
    const u32 n = load<u16>(m + kMapCount);
    if (n != 0) call(ctx, obj::ofn::qsort, {load<u32>(m + kMapEntries), n, kMapEntrySize, load<u32>(m + kMapCompare)});
}

// The binary search shared by Find and Remove: the index of *keyp, or -1.
// `entries` is updated when the custom-compare path re-reads it on a hit;
// `last` gets what the original leaves in r3 on a miss.
s32 Search(PPUContext& ctx, u32 m, u32 keyp, u32 default_cmp, u32& entries, u64& last) {
    s32 hi = static_cast<s32>(load<u16>(m + kMapCount)) - 1, lo = 0;
    entries = load<u32>(m + kMapEntries);
    if (load<u32>(m + kMapCompare) == default_cmp) {
        while (hi >= lo) {  // cmpw
            const s32 mid = (hi + lo) >> 1;  // srawi
            const u32 key = load<u32>(keyp);
            const u32 k = load<u32>(entries + static_cast<u32>(mid) * kMapEntrySize);
            if (k > key) {  // cmplw
                hi = mid - 1;
            } else if (k < key) {
                lo = mid + 1;
            } else {
                return mid;
            }
        }
        return -1;
    }
    Stack frame(ctx, 16);  // the original's sp+112
    const u32 copy = frame.addr();
    last = load<u32>(keyp);
    store<u32>(copy, static_cast<u32>(last));
    while (hi >= lo) {
        const s32 mid = (hi + lo) >> 1;
        const u32 cmp = load<u32>(m + kMapCompare);
        // The OPD and its entry go in r5 / r6, as the original's call sequence leaves them.
        const s32 r = static_cast<s32>(
            call_ptr(ctx, cmp, {entries + static_cast<u32>(mid) * kMapEntrySize, copy, cmp, load<u32>(cmp)}));
        last = static_cast<u32>(r);
        if (r > 0) {  // cmpwi
            hi = mid - 1;
        } else if (r < 0) {
            lo = mid + 1;
        } else {
            entries = load<u32>(m + kMapEntries);  // re-read on a hit
            return mid;
        }
    }
    return -1;
}

// u32* <Map>_Find(Map* m, const u32* key): the value slot of *key, or the
// map's default slot (+16) on a miss.
void SortedMap_Find(PPUContext& ctx, u32 default_cmp) {
    const u32 m = arg<u32>(ctx, 0), keyp = arg<u32>(ctx, 1);
    SortIfDirty(ctx, m);
    u32 entries = 0;
    u64 last = 0;
    const s32 i = Search(ctx, m, keyp, default_cmp, entries, last);
    ret(ctx, i < 0 ? m + kMapDefault : entries + static_cast<u32>(i) * kMapEntrySize + 4);
}

// void <Map>_Remove(Map* m, const u32* key): removes the entry of *key
// (when there is one), shifting the later entries down one (the count
// re-read every step) and decrementing the count.
void SortedMap_Remove(PPUContext& ctx, u32 default_cmp) {
    const u32 m = arg<u32>(ctx, 0), keyp = arg<u32>(ctx, 1);
    SortIfDirty(ctx, m);
    u32 count = load<u16>(m + kMapCount);
    u64 last = count;  // r3 on a default-compare miss
    u32 entries = 0;
    const s32 i = Search(ctx, m, keyp, default_cmp, entries, last);
    if (i < 0) {
        ctx.gpr[3] = last;
        return;
    }
    count = load<u16>(m + kMapCount);  // (the default path's earlier read; the same value)
    if (static_cast<s32>(count) >= 2 && i < static_cast<s32>(count) - 1) {  // cmpwi / cmpw
        for (s32 j = i;;) {
            const u32 e = entries + static_cast<u32>(j) * kMapEntrySize;
            const u32 key = load<u32>(e + 8);
            count = load<u16>(m + kMapCount);
            ++j;
            store<u32>(e, key);
            store<u32>(e + 4, load<u32>(e + 12));
            if (j >= static_cast<s32>(count) - 1) break;
        }
    }
    store<u16>(m + kMapCount, static_cast<u16>(count - 1));
    ctx.gpr[3] = static_cast<u32>(count - 1);
}

}  // namespace

// ApePointDef** PointMap_Find(Map* m, const u32* hash) (name?)
DECOMP(0073343C, PointMap_Find) { SortedMap_Find(ctx, kCmpPointMap); }
// CameraDefMap_Find (name?)
DECOMP(0073374C, CameraDefMap_Find) { SortedMap_Find(ctx, kCmpCameraDefMap); }
// PointDefMap_Find (name?)
DECOMP(007A6A64, PointDefMap_Find) { SortedMap_Find(ctx, kCmpPointDefMap); }
// SplineDefMap_Find (name?)
DECOMP(007936FC, SplineDefMap_Find) { SortedMap_Find(ctx, kCmpSplineDefMap); }

// void HashMap_Remove2(Map* m, const u32* key): the GeomDef map's remove (name?)
DECOMP(007A7C74, HashMap_Remove2) { SortedMap_Remove(ctx, kCmpGeomDefMap); }
DECOMP(007A7E68, LightDefMap_Remove) { SortedMap_Remove(ctx, kCmpLightDefMap); }
DECOMP(007A805C, CameraDefMap_Remove) { SortedMap_Remove(ctx, kCmpCameraDefMap); }
DECOMP(007A8544, PointDefMap_Remove) { SortedMap_Remove(ctx, kCmpPointDefMap); }
DECOMP(007A8738, SplineDefMap_Remove) { SortedMap_Remove(ctx, kCmpSplineDefMap); }
DECOMP(007A8D40, CameraInstMap_Remove) { SortedMap_Remove(ctx, kCmpCameraInst); }
DECOMP(007A8F34, PointInstMap_Remove) { SortedMap_Remove(ctx, kCmpPointMap); }
DECOMP(007A92F4, SplineInstMap_Remove) { SortedMap_Remove(ctx, kCmpSplineInst); }

// More instances of the same template (0x00730000-0x007A0000), named by
// address until their maps are known: SortedMap_Find<addr> (map, const u32*
// key) -> value slot, SortedMap_Remove<addr> (map, const u32* key).
DECOMP(00732F38, SortedMap_Find732F38) { SortedMap_Find(ctx, 0x00F2E0B8u); }
DECOMP(007330C0, SortedMap_Remove7330C0) { SortedMap_Remove(ctx, 0x00F2E0C0u); }
DECOMP(007332B4, SortedMap_Find7332B4) { SortedMap_Find(ctx, 0x00F2E0C8u); }
DECOMP(00733F8C, SortedMap_Find733F8C) { SortedMap_Find(ctx, 0x00F2E978u); }
DECOMP(00734750, SortedMap_Find734750) { SortedMap_Find(ctx, 0x00F2E998u); }
DECOMP(0073B608, SortedMap_Find73B608) { SortedMap_Find(ctx, 0x00F2FD30u); }
DECOMP(00742EF4, SortedMap_Find742EF4) { SortedMap_Find(ctx, 0x00F31078u); }
DECOMP(00744748, SortedMap_Remove744748) { SortedMap_Remove(ctx, 0x00F31A10u); }
DECOMP(00744AC4, SortedMap_Find744AC4) { SortedMap_Find(ctx, 0x00F2E0C0u); }
DECOMP(00744C4C, SortedMap_Find744C4C) { SortedMap_Find(ctx, 0x00F31A68u); }
DECOMP(00744DD4, SortedMap_Find744DD4) { SortedMap_Find(ctx, 0x00F31A70u); }
DECOMP(00744F5C, SortedMap_Find744F5C) { SortedMap_Find(ctx, 0x00F31A78u); }
DECOMP(00749A14, SortedMap_Find749A14) { SortedMap_Find(ctx, 0x00F32D40u); }
DECOMP(00749B9C, SortedMap_Find749B9C) { SortedMap_Find(ctx, 0x00F32D48u); }
DECOMP(007504F0, SortedMap_Find7504F0) { SortedMap_Find(ctx, 0x00F33888u); }
DECOMP(00759654, SortedMap_Find759654) { SortedMap_Find(ctx, 0x00F348A0u); }
DECOMP(007597DC, SortedMap_Remove7597DC) { SortedMap_Remove(ctx, 0x00F348A0u); }
DECOMP(0075C054, SortedMap_Find75C054) { SortedMap_Find(ctx, 0x00F34E48u); }
DECOMP(0075DC84, SortedMap_Remove75DC84) { SortedMap_Remove(ctx, 0x00F32D40u); }
DECOMP(0075DE78, SortedMap_Remove75DE78) { SortedMap_Remove(ctx, 0x00F32D48u); }
DECOMP(0075E06C, SortedMap_Remove75E06C) { SortedMap_Remove(ctx, 0x00F35B00u); }
DECOMP(0075E260, SortedMap_Find75E260) { SortedMap_Find(ctx, 0x00F35B00u); }
DECOMP(0075E3E8, SortedMap_Find75E3E8) { SortedMap_Find(ctx, 0x00F35B08u); }
DECOMP(0075E570, SortedMap_Find75E570) { SortedMap_Find(ctx, 0x00F35B10u); }
DECOMP(0075E6F8, SortedMap_Remove75E6F8) { SortedMap_Remove(ctx, 0x00F35B18u); }
DECOMP(0075E8EC, SortedMap_Find75E8EC) { SortedMap_Find(ctx, 0x00F35B18u); }
DECOMP(0075EA74, Map_Get) { SortedMap_Find(ctx, 0x00F35B20u); }
DECOMP(0075EE10, SortedMap_Remove75EE10) { SortedMap_Remove(ctx, 0x00F35B30u); }
DECOMP(0075F004, SortedMap_Find75F004) { SortedMap_Find(ctx, 0x00F35B30u); }
DECOMP(0075F18C, SortedMap_Remove75F18C) { SortedMap_Remove(ctx, 0x00F35B38u); }
DECOMP(0075F380, SortedMap_Find75F380) { SortedMap_Find(ctx, 0x00F35B38u); }
DECOMP(0075F508, SortedMap_Remove75F508) { SortedMap_Remove(ctx, 0x00F35B40u); }
DECOMP(0075F6FC, SortedMap_Find75F6FC) { SortedMap_Find(ctx, 0x00F35B40u); }
DECOMP(0075F884, SortedMap_Remove75F884) { SortedMap_Remove(ctx, 0x00F35B48u); }
DECOMP(0075FA78, SortedMap_Find75FA78) { SortedMap_Find(ctx, 0x00F35B48u); }
DECOMP(0075FC00, SortedMap_Find75FC00) { SortedMap_Find(ctx, 0x00F35B50u); }
DECOMP(0075FD88, SortedMap_Find75FD88) { SortedMap_Find(ctx, 0x00F35B58u); }
DECOMP(00760538, SortedMap_Find760538) { SortedMap_Find(ctx, 0x00F35C30u); }
DECOMP(0076116C, SortedMap_Find76116C) { SortedMap_Find(ctx, 0x00F35DC8u); }
DECOMP(0076611C, SortedMap_Find3) { SortedMap_Find(ctx, 0x00F35F90u); }
DECOMP(0076673C, SortedMap_Find76673C) { SortedMap_Find(ctx, 0x00F35FB0u); }
DECOMP(00766DCC, SortedMap_Find766DCC) { SortedMap_Find(ctx, 0x00F36078u); }
DECOMP(00766F54, SortedMap_Find766F54) { SortedMap_Find(ctx, 0x00F36080u); }
DECOMP(00768AC0, SortedMap_Find768AC0) { SortedMap_Find(ctx, 0x00F36AB8u); }
DECOMP(0076AA2C, SortedMap_Find76AA2C) { SortedMap_Find(ctx, 0x00F37240u); }
DECOMP(0076ABB4, SortedMap_Find76ABB4) { SortedMap_Find(ctx, 0x00F37248u); }
DECOMP(0076D6A0, SortedMap_Remove76D6A0) { SortedMap_Remove(ctx, 0x00F375B8u); }
DECOMP(0076D894, SortedMap_Find76D894) { SortedMap_Find(ctx, 0x00F375B8u); }
DECOMP(0076DA1C, SortedMap_Remove76DA1C) { SortedMap_Remove(ctx, 0x00F375C0u); }
DECOMP(0076DC10, SortedMap_Find76DC10) { SortedMap_Find(ctx, 0x00F375C0u); }
DECOMP(0076DD98, SortedMap_Find76DD98) { SortedMap_Find(ctx, 0x00F375C8u); }
DECOMP(0076DF20, SortedMap_Find76DF20) { SortedMap_Find(ctx, 0x00F375D0u); }
DECOMP(0076E0A8, SortedMap_Find76E0A8) { SortedMap_Find(ctx, 0x00F375D8u); }
DECOMP(0076E230, SortedMap_Find76E230) { SortedMap_Find(ctx, 0x00F375E0u); }
DECOMP(00771DD0, SortedMap_Remove) { SortedMap_Remove(ctx, 0x00F35F90u); }
DECOMP(00774154, SortedMap_Find774154) { SortedMap_Find(ctx, 0x00F38238u); }
DECOMP(0077B1FC, SortedMap_Find77B1FC) { SortedMap_Find(ctx, 0x00F38980u); }
DECOMP(00783C8C, SortedMap_Remove783C8C) { SortedMap_Remove(ctx, 0x00F39AA8u); }
DECOMP(00783E80, SortedMap_Find783E80) { SortedMap_Find(ctx, 0x00F39AA8u); }
DECOMP(00784308, SortedMap_Find784308) { SortedMap_Find(ctx, 0x00F39AB8u); }
DECOMP(007845DC, SortedMap_Find2) { SortedMap_Find(ctx, 0x00F39AC0u); }
DECOMP(00784764, SortedMap_Find784764) { SortedMap_Find(ctx, 0x00F39AD0u); }
DECOMP(00786A18, SortedMap_Remove786A18) { SortedMap_Remove(ctx, 0x00F39E08u); }
DECOMP(00786C0C, SortedMap_Find786C0C) { SortedMap_Find(ctx, 0x00F39E08u); }
DECOMP(00788B48, SortedMap_Remove788B48) { SortedMap_Remove(ctx, 0x00F39F68u); }
DECOMP(00788D3C, SortedMap_Find788D3C) { SortedMap_Find(ctx, 0x00F39F68u); }
DECOMP(007890F8, SortedMap_Find7890F8) { SortedMap_Find(ctx, 0x00F3A018u); }
DECOMP(0079A9F8, SortedMap_Find79A9F8) { SortedMap_Find(ctx, 0x00F3B1D0u); }
DECOMP(0079AB80, SortedMap_Find79AB80) { SortedMap_Find(ctx, 0x00F3B1E0u); }
DECOMP(0079AD08, SortedMap_Remove79AD08) { SortedMap_Remove(ctx, 0x00F2E0C8u); }
DECOMP(0079AEFC, SortedMap_Remove79AEFC) { SortedMap_Remove(ctx, 0x00F3B1E0u); }
// MeshMap_Find: the mesh map (*0x010DDC80) (Mesh_FindByHash's) (name?)
DECOMP(0079C8C4, MeshMap_Find) { SortedMap_Find(ctx, 0x00F3BB88u); }
DECOMP(0079E4C8, SortedMap_Remove79E4C8) { SortedMap_Remove(ctx, 0x00F3BB80u); }
DECOMP(0079E7B0, SortedMap_Remove79E7B0) { SortedMap_Remove(ctx, 0x00F3BB88u); }
