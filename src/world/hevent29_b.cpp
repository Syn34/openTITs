// World_HandleEvent callees, part 29b: the property table getters the
// geometry scene load (Geom_LoadScene4F05A8) reads its settings with. A
// table holds sets of {u32 key -> value} sorted by key; the getters differ
// only in the value's type.
#include "hevent29.h"

using namespace sly4;
using namespace sly4::hevent27;
using namespace sly4::hevent29;

namespace {
// The shared lookup: true and the value address for key in set idx. A
// negative index finds nothing; the keys are scanned in order and the scan
// stops at the first key above the one wanted (unsigned).
bool prop_find(u32 table, s32 idx, u32 key, u32& v) {
    if (idx < 0) return false;
    const u32 set = load<u32>(table + kPropSets + 4 * static_cast<u32>(idx));
    const s32 n = load<u16>(set + kPropCount);
    u32 keys = load<u32>(set + kPropKeys);
    for (s32 i = 0; i < n; ++i, keys += 4) {
        const u32 k = load<u32>(keys);
        if (k == key) {
            if (i <= -1) return false;  // (the original's check; never taken)
            const u32 off = load<u16>(load<u32>(set + kPropOffsets) + 2 * static_cast<u32>(i));
            v = load<u32>(set + kPropValues) + off;
            return true;
        }
        if (key < k) break;
    }
    return false;
}
}  // namespace

// bool PropTable_GetFloat799C90(PropTable* t, int idx, u32 key, f32* out)
// (name?): *out = the float for key (left unchanged when absent). Read
// through f1 (lfs / stfs), so f1 holds the value after a hit.
DECOMP(00799C90, PropTable_GetFloat799C90) {
    u32 v;
    if (!prop_find(arg<u32>(ctx, 0), arg<s32>(ctx, 1), arg<u32>(ctx, 2), v)) {
        ctx.gpr[3] = 0;
        return;
    }
    const u32 bits = load<u32>(v);
    ctx.fpr[1] = lfs_reg(bits);
    store<u32>(arg<u32>(ctx, 3), bits);
    ctx.gpr[3] = 1;
}

// bool PropTable_GetColor799D2C(PropTable* t, int idx, u32 key, u8 out[4])
// (name?): the four bytes for key (all read before any is written).
DECOMP(00799D2C, PropTable_GetColor799D2C) {
    u32 v;
    if (!prop_find(arg<u32>(ctx, 0), arg<s32>(ctx, 1), arg<u32>(ctx, 2), v)) {
        ctx.gpr[3] = 0;
        return;
    }
    const u32 out = arg<u32>(ctx, 3);
    u8 b[4];
    for (u32 k = 0; k < 4; ++k) b[k] = load<u8>(v + k);
    for (u32 k = 0; k < 4; ++k) store<u8>(out + k, b[k]);
    ctx.gpr[3] = 1;
}

// bool PropTable_GetByte799DE0(PropTable* t, int idx, u32 key, u8* out)
// (name?): the byte for key.
DECOMP(00799DE0, PropTable_GetByte799DE0) {
    u32 v;
    if (!prop_find(arg<u32>(ctx, 0), arg<s32>(ctx, 1), arg<u32>(ctx, 2), v)) {
        ctx.gpr[3] = 0;
        return;
    }
    store<u8>(arg<u32>(ctx, 3), load<u8>(v));
    ctx.gpr[3] = 1;
}

// bool PropTable_GetU32799E74(PropTable* t, int idx, u32 key, u32* out)
// (name?): the word for key.
DECOMP(00799E74, PropTable_GetU32799E74) {
    u32 v;
    if (!prop_find(arg<u32>(ctx, 0), arg<s32>(ctx, 1), arg<u32>(ctx, 2), v)) {
        ctx.gpr[3] = 0;
        return;
    }
    store<u32>(arg<u32>(ctx, 3), load<u32>(v));
    ctx.gpr[3] = 1;
}
