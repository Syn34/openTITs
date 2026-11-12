// Game code, medium sweep part 34: sorted 8-byte {key, value} array
// removals.
#include "mid34.h"

namespace sly4::obj {

// A sorted array of 8-byte {u32 key, value} entries.
struct SortedKV {
    be_u32 key;    // +0
    be_u32 value;  // +4
};
struct SortedArray8 {
    pad<2> _0;
    be_u16 count;         // +2
    pad<2> _4;
    be_u16 flags;         // +6: bit 0 = needs sorting
    gp<SortedKV> data;    // +8
    pad<8> _12;
    be_u32 cmp;           // +20: compare OPD (const SortedKV*, const u32* key) -> int
};
static_assert(offsetof(SortedArray8, data) == 8 && offsetof(SortedArray8, cmp) == 20);

namespace {
constexpr u32 Qsort34 = 0x0071EC28u;          // qsort(base, n, size, compare OPD)
constexpr u32 kCmpS32F34 = 0x00F377A8u;       // the default (inlined) compare OPD of the s32 -> f32 arrays
constexpr u32 kCmpU32U34 = 0x00F39E10u;       // the default (inlined) compare OPD of the u32 -> u32 arrays

// Remove(array, const u32* key): sorts the array first when its bit 0 is
// set (cleared; qsort(entries, count, 8, cmp) when count > 0), then a
// binary search for *key: inline (signed / unsigned) when the compare is
// the default OPD, else cmp(&entry, &copy) with the key copied to a stack
// word. Found: the later entries moved down one by one (the count re-read
// each step; `fval`: the count read before the step's stores, else after
// them; the value is copied as a raw word either way) and the count
// decremented.
void SortedRemove34(SortedArray8* a, const be_u32* kp, u32 defcmp, bool sgn, bool fval) {
    const u16 fl = a->flags;
    if (fl & 1u) {
        a->flags = static_cast<u16>(fl & 0xFFFEu);
        const u16 c = a->count;
        if (static_cast<s32>(c) > 0) call(Qsort34, a->data.get(), u32(c), 8u, a->cmp.get());
    }
    u32 n = a->count;
    SortedKV* data = a->data;
    const u32 cmp = a->cmp;
    s32 lo = 0, hi = static_cast<s32>(n) - 1, mid = 0;
    bool found = false;
    if (cmp == defcmp) {
        if (hi < 0) return;
        do {
            mid = (hi + lo) >> 1;
            const u32 key = *kp;
            const u32 e = data[mid].key;
            const bool gt = sgn ? static_cast<s32>(e) > static_cast<s32>(key) : e > key;
            const bool lt = sgn ? static_cast<s32>(e) < static_cast<s32>(key) : e < key;
            if (gt) hi = mid - 1;
            else if (lt) lo = mid + 1;
            else { found = true; break; }
        } while (hi >= lo);
        if (!found) return;
    } else {
        GuestFrame f(16);
        be_u32* kcopy = f.as<be_u32>();
        *kcopy = kp->get();
        if (hi < 0) return;
        do {
            mid = (hi + lo) >> 1;
            const s32 r = call_ptr<s32>(cmp, &data[mid], kcopy);
            if (r > 0) hi = mid - 1;
            else if (r < 0) lo = mid + 1;
            else { found = true; break; }
        } while (hi >= lo);
        if (!found) return;
        n = a->count;
        data = a->data;
    }
    if (mid < 0) return;
    if (static_cast<s32>(n) >= 2 && mid < static_cast<s32>(n) - 1) {
        s32 i = mid;
        do {
            SortedKV* p = &data[i];
            ++i;
            if (fval) {
                const u32 k = p[1].key;
                n = a->count;
                p->key = k;
                p->value = p[1].value;  // lfs / stfs: the raw word
            } else {
                p->key = p[1].key.get();
                p->value = p[1].value.get();
                n = a->count;
            }
        } while (i < static_cast<s32>(n) - 1);
    }
    a->count = static_cast<u16>(n - 1);
}
}  // namespace

// SortedArray_Remove(array, const s32* key) of an s32 -> f32 array
// (callee of f_003547EC).
void SortedS32F_Remove77016C(SortedArray8* a, const be_u32* key) { SortedRemove34(a, key, kCmpS32F34, true, true); }

// SortedArray_Remove(array, const u32* key) of a u32 -> u32 array (callee
// of Obj_Register7_00490518).
void SortedU32_Remove786D94(SortedArray8* a, const be_u32* key) { SortedRemove34(a, key, kCmpU32U34, false, false); }

}  // namespace sly4::obj

NATIVE(0077016C, sly4::obj::SortedS32F_Remove77016C)
NATIVE(00786D94, sly4::obj::SortedU32_Remove786D94)
