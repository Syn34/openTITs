// World objects, part 2: the class maps' lookups, dirty marking, the base
// constructor, class copies and the light free list.
#include "obj2.h"

using namespace sly4;
using namespace sly4::obj;
using namespace sly4::obj2;

namespace {

// u32* ClassMap_Find(Map* m, const u32* key)
// The template of the four class-map lookups (as Map_Find in map.cpp, but
// the comparator is re-read from the map on every probe): the value slot
// for *key, sorting the entries first if they changed, or the default slot.
u32 ClassMap_Find(PPUContext& ctx, u32 m, u32 keyp, u32 default_compare) {
    const u16 flags = load<u16>(m + kMapFlags);
    if ((flags & 1) != 0) {
        store<u16>(m + kMapFlags, static_cast<u16>(flags & 0xFFFE));
        const u32 n = load<u16>(m + kMapCount);
        if (static_cast<s32>(n) > 0)
            call(ctx, ofn::qsort, {load<u32>(m + kMapEntries), n, kMapEntrySize, load<u32>(m + kMapCompare)});
    }
    s32 hi = static_cast<s32>(load<u16>(m + kMapCount)) - 1, lo = 0;
    u32 entries = load<u32>(m + kMapEntries);
    s32 found = -1;
    if (load<u32>(m + kMapCompare) == default_compare) {
        while (hi >= lo) {
            const s32 mid = (hi + lo) >> 1;
            const u32 key = load<u32>(keyp);
            const u32 k = load<u32>(entries + static_cast<u32>(mid * 8));
            if (k > key) {
                hi = mid - 1;
            } else if (k < key) {
                lo = mid + 1;
            } else {
                found = mid;
                break;
            }
        }
    } else {
        Stack frame(ctx, 16);
        const u32 key = frame.addr();  // sp+112
        store<u32>(key, load<u32>(keyp));
        while (hi >= lo) {
            const s32 mid = (hi + lo) >> 1;
            const u32 cmp = load<u32>(m + kMapCompare);
            const s32 r = static_cast<s32>(
                call_ptr(ctx, cmp, {entries + static_cast<u32>(mid * 8), key, cmp, load<u32>(cmp)}));  // r5 / r6 = OPD and entry
            if (r > 0) {
                hi = mid - 1;
            } else if (r < 0) {
                lo = mid + 1;
            } else {
                entries = load<u32>(m + kMapEntries);
                found = mid;
                break;
            }
        }
    }
    if (found < 0) return m + kMapDefault;
    return entries + static_cast<u32>(found * 8) + 4;
}

}  // namespace

// u32* MapA_Find(Map* m, const u32* key)  (class map holder 0x00F8F150)
DECOMP(007A6D74, MapA_Find) { ret(ctx, ClassMap_Find(ctx, arg<u32>(ctx, 0), arg<u32>(ctx, 1), kMapCompareA)); }
// u32* MapB_Find(Map* m, const u32* key)  (class map holder 0x00F8F144)
DECOMP(007A6BEC, MapB_Find) { ret(ctx, ClassMap_Find(ctx, arg<u32>(ctx, 0), arg<u32>(ctx, 1), kMapCompareB)); }
// u32* MapC_Find(Map* m, const u32* key)  (class map holder 0x00F8F148)
DECOMP(00793884, MapC_Find) { ret(ctx, ClassMap_Find(ctx, arg<u32>(ctx, 0), arg<u32>(ctx, 1), kMapCompareC)); }
// u32* MapD_Find(Map* m, const u32* key)  (class map holder 0x00F8F14C)
DECOMP(00793A0C, MapD_Find) { ret(ctx, ClassMap_Find(ctx, arg<u32>(ctx, 0), arg<u32>(ctx, 1), kMapCompareD)); }

// void WorldObj_MarkDirty(Obj* o)
// Unless the object is already dirty (0x800): marks its parent dirty,
// first queueing the parent on the dirty list (0x400) if it is neither
// dirty nor queued, then marks the object. As WorldObj_SetScale inlines
// it; no null check on the parent. r3 is left as the original leaves it.
DECOMP(0057D000, WorldObj_MarkDirty) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 w = load<u32>(o + kObjFlags);
    if ((w & kObjDirty) != 0) return ret(ctx, w);
    const u32 p = load<u32>(o + kObjParent);
    u32 pw = load<u32>(p + kObjFlags);
    u32 ow = w;  // the parent-already-dirty path keeps the first read
    if ((pw & kObjDirty) == 0) {
        if ((pw & kObjQueuedFlag) == 0 && load<u32>(p + kObjQueued) == 0 && load<u8>(kDirtyListOff) == 0) {
            const u32 v = load<u32>(kDirtyList);
            if (load<u16>(v + 8) == load<u16>(v + 4))
                call(ctx, ofn::PtrVector_Reserve, {v, (static_cast<u32>(load<u16>(v + 4)) << 1) & ~1u});
            store<u32>(load<u32>(v) + load<u16>(v + 8) * 4u, p);
            const u32 n = load<u16>(v + 8) + 1u;
            const u32 hi = load<u16>(v + 10);
            store<u16>(v + 8, static_cast<u16>(n));
            store<u16>(v + 10, static_cast<u16>(static_cast<s32>(hi) > static_cast<s32>(n & 0xFFFF) ? hi : n));
            pw = load<u32>(p + kObjFlags) | kObjQueuedFlag;
        }
        store<u32>(p + kObjFlags, pw | kObjDirty);
        ow = load<u32>(o + kObjFlags);
    }
    store<u32>(o + kObjFlags, ow | kObjDirty);
    ret(ctx, ow | kObjDirty);
}

// void WorldObj_Construct(Obj* o, s32 kind)
// The base constructor (three base vtables in turn): +4 = {u16 1, kind in
// bits 10-15}, a new serial (0x01164CD0) in bits 11-31 of +8, the name and
// a few links cleared, then the transform and link initialisers; sets
// 0x20 in +322. r3 is left as the original leaves it.
DECOMP(00580708, WorldObj_Construct) {
    const u32 o = arg<u32>(ctx, 0), kind = arg<u32>(ctx, 1);
    u32 w4 = load<u32>(o + kObjWord4);
    store<u16>(o + kObjWord4, 1);
    w4 = (w4 & 0x0000FFFFu) | 0x00010000u;
    w4 = (w4 & ~0x0000FC00u) | ((kind << 10) & 0x0000FC00u);
    store<u32>(o + kObjWord4, w4);
    const u32 serial = load<u32>(kObjSerial) + 1;
    store<u32>(o, kVtblObjBase0);
    store<u32>(kObjSerial, serial);
    u32 w8 = load<u32>(o + kObjWord8);
    store<u8>(o + kObjName, 0);
    store<u32>(o, kVtblObjBase1);
    store<u32>(o + 276, 0);
    w8 = (serial << 11) | (w8 & 0x7FFu);
    store<u32>(o + 280, 0);
    store<u32>(o + 12, 0);
    w8 &= ~0x400u;
    w8 &= ~0x3C0u;
    store<u32>(o + kObjWord8, w8);
    call(ctx, o2fn::WorldObj_InitTransform, {o, 0});  // r4 = 0 as in the original (not read)
    store<u32>(o, kVtblObjBase2);
    store<u16>(o + kObjWord284, 0xFFFF);
    call(ctx, o2fn::WorldObj_InitLinks, {o, 0xFFFF, kVtblObjBase2});  // r4 / r5 as in the original (not read)
    const u8 b = static_cast<u8>(load<u8>(o + kObjFlags322) | 0x20);
    store<u8>(o + kObjFlags322, b);
    ret(ctx, static_cast<u32>(b));
}

// WorldClass* WorldClass_Copy(const WorldClass* src)
// A new class (WorldClass_Alloc) with src's transform rows, name and
// fields; the shared block at +108 and the resource at +164 are
// reference-counted (the old ones released), and each topology at +132 is
// copied through one copy table (on the stack), the entry at index +115
// once more unless it is the last.
DECOMP(005858D4, WorldClass_Copy) {
    const u32 src = arg<u32>(ctx, 0);
    Stack frame(ctx, 1456 - 112);
    const u32 table = frame.addr();  // sp+112, count at sp+1392
    const u32 dst = static_cast<u32>(call(ctx, o2fn::WorldClass_Alloc));
    for (u32 off : {16u, 20u, 24u, 32u, 36u, 40u, 48u, 52u, 56u, 60u}) store<u32>(dst + off, load<u32>(src + off));  // lfs / stfs
    call(ctx, o2fn::String_Assign, {dst + kClassName, src + kClassName});
    const u32 shared = load<u32>(src + kClassShared);
    store<u32>(dst + 96, load<u32>(src + 96));
    store<u32>(dst + 100, load<u32>(src + 100));
    const u32 old = load<u32>(dst + kClassShared);
    store<u32>(dst + 104, load<u32>(src + 104));
    if (old != shared) {
        store<u32>(dst + kClassShared, shared);
        if (shared != 0) store<u16>(shared, static_cast<u16>(load<u16>(shared) + 1));
        if (old != 0) {
            const u16 refs = static_cast<u16>(load<u16>(old) - 1);
            store<u16>(old, refs);
            if (static_cast<s16>(refs) <= 0) {
                const u32 data = load<u32>(old + 4);
                if (data != 0) {
                    call(ctx, o2fn::Mem_FreeFrom, {data - 16, 0});
                    store<u32>(old + 4, 0);
                }
                call(ctx, o2fn::Mem_FreeFrom, {old, 0});
            }
        }
    }
    store<u32>(table + kCopyTableCount, 0);
    store<u8>(dst + kClassTopoCount, load<u8>(src + kClassTopoCount));
    if (load<u8>(src + kClassTopoCount) != 0) {
        u32 k = 0;
        do {
            const u32 s = src + kClassTopos + k * 8, d = dst + kClassTopos + k * 8;
            store<u32>(d, static_cast<u32>(call(ctx, o2fn::Topology_Copy, {load<u32>(s), table})));
            store<u32>(d + 4, load<u32>(s + 4));  // lfs / stfs
            ++k;
        } while (k < load<u8>(src + kClassTopoCount));
    }
    store<u32>(dst + 120, load<u32>(src + 120));
    const u32 res = load<u32>(src + kClassRes);
    store<u32>(dst + 124, load<u32>(src + 124));
    const u32 old_res = load<u32>(dst + kClassRes);
    if (old_res != res) {
        store<u32>(dst + kClassRes, res);
        if (res != 0) store<u16>(res, static_cast<u16>(load<u16>(res) + 1));
        if (old_res != 0) call(ctx, o2fn::WorldRes_Release, {old_res, res});  // r4 as in the original (not read)
    }
    store<u8>(dst + 113, load<u8>(src + 113));
    store<u8>(dst + 114, load<u8>(src + 114));
    store<u8>(dst + kClassTopoIndex, load<u8>(src + kClassTopoIndex));
    store<u32>(dst + 116, load<u32>(src + 116));
    store<u32>(dst + 128, load<u32>(src + 128));
    const u32 idx = load<u8>(src + kClassTopoIndex);
    if (static_cast<s32>(idx) > 0 && idx != static_cast<u32>(load<u8>(src + kClassTopoCount)) - 1) {
        const u32 s = src + kClassTopos + ((idx << 3) & 0x7F8u), d = dst + kClassTopos + (idx << 3);
        store<u32>(d, static_cast<u32>(call(ctx, o2fn::Topology_Copy, {load<u32>(s), table})));
        store<u32>(d + 4, load<u32>(s + 4));
    }
    ret(ctx, dst);
}

// AglLight* Light_Alloc()
// A 240-byte AglLight from the free list at *0x010DDC84 (made on first
// use), not constructed: clears 0x80 in +212, takes the next serial from
// 0x011DE4BC modulo config+153 (at least 1) as +213, zeroes +214 / +215
// and sets +216 to -1. 0 when the list is empty. As in the original, a
// failed list allocation leaves a null allocator that is then called.
DECOMP(00526494, Light_Alloc) {
    const u32 list = load<u32>(kLightList);
    u32 alloc = load<u32>(list);
    if (alloc == 0) {
        const u32 obj = static_cast<u32>(call(ctx, o2fn::Mem_Alloc, {60, 0, 16}));
        if (obj != 0) {
            const u32 capacity = load<u32>(list + 4);
            for (u32 off : {8u, 4u, 12u, 20u, 16u}) store<u32>(obj + off, 0);
            const u16 h54 = load<u16>(obj + 54);
            for (u32 off : {24u, 32u, 28u, 36u, 40u, 44u, 48u}) store<u32>(obj + off, 0);
            store<u16>(obj + 52, 0);
            store<u16>(obj + 54, static_cast<u16>(h54 & 0x7FFF));
            store<u32>(obj, kLightListVtbl);
            store<u32>(obj + 56, kStrLightList);
            // r8 / r9 = the vtable and the string here in the original; FreeList_Init reads r3..r7 only.
            call(ctx, o2fn::FreeList_Init, {obj, kLightSize, capacity, 0, 0});
        }
        alloc = obj;
        store<u32>(list, alloc);
    }
    const u32 opd = load<u32>(load<u32>(alloc) + 12);
    const u32 l = static_cast<u32>(call_ptr(ctx, opd, {alloc, opd, load<u32>(opd)}));  // r4 / r5 = OPD and entry
    if (l == 0) return ret(ctx, 0u);
    store<u8>(l + 212, static_cast<u8>(load<u8>(l + 212) & 0x7F));
    const u32 serial = load<u8>(kLightSerial);
    store<u8>(kLightSerial, static_cast<u8>(serial + 1));
    store<u8>(l + 213, static_cast<u8>(serial));
    store<u8>(l + 214, 0);
    store<u8>(l + 215, 0);
    store<u32>(l + 216, 0xFFFFFFFFu);
    const u32 slots = load<u8>(kConfigLightSlots);
    const s32 div = static_cast<s32>(slots) > 1 ? static_cast<s32>(slots) : 1;
    store<u8>(l + 213, static_cast<u8>(static_cast<s32>(serial) % div));
    ret(ctx, l);
}
