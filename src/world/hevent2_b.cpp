// World_HandleEvent callees, part 2: Material_CopyFrom (used by Material_Clone).
#include "hevent.h"

using namespace sly4;
using namespace sly4::hevent;

namespace sly4 {
namespace hevent2 {
namespace bfn {
constexpr u32 Material_CopyAnim = 0x00508FBC;  // (AglMaterialAnim* dst, const AglMaterialAnim* src) (name?)
constexpr u32 SmallAlloc = 0x0055238C;         // (size)
constexpr u32 SmallFree = 0x00552420;          // (p, size)
constexpr u32 FreeList_Init = 0x00551A58;      // (FreeList*, elem size, count, heap or 0, bool)
constexpr u32 memmove = 0x0071C3F0;
}  // namespace bfn

// Material fields copied here (name?).
constexpr u32 kMatSlots = 20;        // RefObj* slots[], refcount u16 at +4, vfunc +8 release
constexpr u32 kMatSlotCount = 44;    // u8
constexpr u32 kMatSlot0Value = 48;   // u16: low half of slot 0's +112
constexpr u32 kMatValues = 124;      // f32*: kMatValueCap (u8 +121) entries
constexpr u32 kMatValueCap = 121;    // u8, inside the word at +120 (top 4 bits are flags)
constexpr u32 kMatAnim = 128;        // AglMaterialAnim*
constexpr u32 kMatMinValues = 8;

// AglMaterialAnim (152 bytes): +0 u32, +4 u8, three 48-byte tracks at +8.
constexpr u32 kAnimSize = 152, kAnimTracks = 3, kAnimTrackSize = 48;
constexpr u32 kOne = 0x3F800000, kHalf = 0x3F000000;  // f32 1.0, 0.5

// static ApeTFreeList<AglMaterialAnim>& ApeTMemClassFL<AglMaterialAnim, 256>::GetAllocator()
constexpr u32 kAnimFreeList = 0x011DE4B8;      // u32 ApeTFreeList* (built on first use)
constexpr u32 kFreeListVtable = 0x00B0EF88;
constexpr u32 kAnimFreeListName = 0x00AF10D9;  // the GetAllocator() signature
constexpr u32 kFreeListObjSize = 60, kFreeListAlign = 16, kAnimPerBlock = 256;
}  // namespace hevent2
}  // namespace sly4

using namespace sly4::hevent2;

// void Material_CopyFrom(Material* dst, const Material* src): appends src's
// texture slots to dst's (taking references; slot 0 also sets +48), copies the
// scalar fields, the f32 value array (growing dst's as needed) and the
// animation block (allocating dst's from the AglMaterialAnim free list) (name?)
// Quirks kept: a failed free-list allocation still calls through the null
// list, and a null animation is passed on to Material_CopyAnim.
DECOMP(00509090, Material_CopyFrom) {
    const u32 dst = arg<u32>(ctx, 0), src = arg<u32>(ctx, 1);

    for (u32 i = 0; i < ref<u8>(src + kMatSlotCount); ++i) {
        const u32 p = load<u32>(src + kMatSlots + i * 4);
        const u8 cnt = static_cast<u8>(ref<u8>(dst + kMatSlotCount) + 1);
        const s32 k = static_cast<s32>(cnt) - 1;  // -1 when the count wraps (unchecked)
        ref<u8>(dst + kMatSlotCount) = cnt;
        const u32 slot = dst + kMatSlots + static_cast<u32>(k) * 4;
        const u32 old = load<u32>(slot);
        if (old != p) {
            store<u32>(slot, p);
            if (p != 0) store<u16>(p + 4, static_cast<u16>(load<u16>(p + 4) + 1));
            if (old != 0) vcall(ctx, old, 8, {old});
        }
        if (k == 0) store<u16>(dst + kMatSlot0Value, static_cast<u16>(p != 0 ? load<u32>(p + 112) : 0u));
    }

    store<u32>(dst + 40, load<u32>(src + 40));
    ref<u8>(dst + 45) = ref<u8>(src + 45);
    for (u32 off = 64; off <= 96; off += 4) store<u32>(dst + off, load<u32>(src + off));  // 9 floats, raw
    ref<u8>(dst + 100) = static_cast<u8>((ref<u8>(dst + 100) & 0x07) | (ref<u8>(src + 100) & 0xF8));
    store<u64>(dst + 104, load<u64>(src + 104));
    store<u64>(dst + 112, load<u64>(src + 112));
    const u32 w120 = (load<u32>(dst + 120) & 0x0FFFFFFFu) | (load<u32>(src + 120) & 0xF0000000u);
    store<u32>(dst + 120, w120);

    if (load<u32>(src + kMatValues) != 0) {
        u32 d = load<u32>(dst + kMatValues);
        const u32 n = ref<u8>(src + kMatValueCap);
        if (d == 0) {
            const u32 cap = n < kMatMinValues ? kMatMinValues : n;
            ref<u8>(dst + kMatValueCap) = static_cast<u8>(cap);
            d = static_cast<u32>(call(ctx, bfn::SmallAlloc, {(cap << 2) & 0x3FC}));
            store<u32>(dst + kMatValues, d);
        } else {
            const u32 cap = (w120 >> 16) & 0xFF;
            if (n > cap) {
                const u32 step = cap == 16 ? 32u : 16u;
                const u32 now = n < step ? step : n;
                const u32 old_bytes = (w120 >> 14) & 0x3FC;
                d = static_cast<u32>(call(ctx, bfn::SmallAlloc, {now << 2}));
                call(ctx, bfn::memmove, {d, load<u32>(dst + kMatValues), old_bytes});
                call(ctx, bfn::SmallFree, {load<u32>(dst + kMatValues), old_bytes});
                store<u32>(dst + kMatValues, d);
                ref<u8>(dst + kMatValueCap) = static_cast<u8>(now);
            }
        }
        const u32 m = ref<u8>(src + kMatValueCap);
        for (u32 k = 0; k < m; ++k) store<u32>(d + k * 4, load<u32>(load<u32>(src + kMatValues) + k * 4));
    }

    const u32 sa = load<u32>(src + kMatAnim);
    if (sa == 0) return;
    u32 da = load<u32>(dst + kMatAnim);
    if (da == 0) {
        u32 fl = load<u32>(kAnimFreeList);
        if (fl == 0) {
            const u32 mem = static_cast<u32>(call(ctx, fn::Mem_Alloc, {kFreeListObjSize, 0, kFreeListAlign}));
            if (mem != 0) {
                // ApeTFreeList<AglMaterialAnim> constructor (inlined)
                const u16 flags = load<u16>(mem + 54);
                for (u32 off = 4; off <= 48; off += 4) store<u32>(mem + off, 0);
                store<u16>(mem + 52, 0);
                store<u16>(mem + 54, static_cast<u16>(flags & ~0x8000u));
                store<u32>(mem + 0, kFreeListVtable);
                store<u32>(mem + 56, kAnimFreeListName);
                ctx.gpr[8] = kFreeListVtable;  // left over from the stores above
                ctx.gpr[9] = kAnimFreeListName;
                call(ctx, bfn::FreeList_Init, {mem, kAnimSize, kAnimPerBlock, 0, 0});
            }
            fl = mem;
            store<u32>(kAnimFreeList, fl);  // 0 when Mem_Alloc failed
        }
        const u32 obj = static_cast<u32>(vcall(ctx, fl, 12, {fl}));  // Alloc()
        if (obj != 0) {
            store<u32>(obj + 0, 0);
            ref<u8>(obj + 4) = 0;
            for (u32 t = 0; t < kAnimTracks; ++t) {
                const u32 tr = obj + 8 + t * kAnimTrackSize;
                const u32 w = load<u32>(tr + 32);
                store<u32>(tr + 0, 0);
                store<u32>(tr + 4, 0);
                store<u32>(tr + 8, kOne);
                store<u32>(tr + 12, 0);
                store<u32>(tr + 16, 0);
                store<u32>(tr + 20, 0);
                store<u32>(tr + 24, kHalf);
                store<u32>(tr + 28, kHalf);
                store<u32>(tr + 32, (w & 0x00000FFFu) | 0x1000u);
                store<u32>(tr + 36, 0);
                store<u32>(tr + 40, 0);
                store<u32>(tr + 44, 0);
            }
        }
        da = obj;
        store<u32>(dst + kMatAnim, da);
        call(ctx, bfn::Material_CopyAnim, {da, load<u32>(src + kMatAnim)});
        return;
    }
    call(ctx, bfn::Material_CopyAnim, {da, sa});
}
