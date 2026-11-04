// World_HandleEvent callees, part 11g: the level release.
#include "hevent11.h"

using namespace sly4;
using namespace sly4::hevent;
using namespace sly4::hevent11;

namespace {
// Level (as hevent11_c.cpp): +508 state, +512 main request, +516 owner of
// the +520 / +524 pair, +528 PtrVector of 16-byte part holders, +532
// PtrVector of extra objects.
constexpr u32 kLvState = 508, kLvMain = 512, kLvOwner = 516, kLvKey = 520, kLvKeyArg = 524;
constexpr u32 kLvRequests = 528, kLvExtras = 532;
// Holder: +0 request, +4 owner, +8 / +12 pair.
constexpr u32 kHoldReq = 0, kHoldOwner = 4, kHoldKey = 8, kHoldKeyArg = 12;
constexpr u32 kVtCancel = 16, kVtDelete = 4, kVtRelease = 8;  // vtable slots
constexpr u32 kMemFreeFrom = 0x0054EE6C;  // Mem_FreeFrom (p, 0)
constexpr u32 kVecData = 0, kVecCount = 8;

// Cancels and deletes the request in *slot (if any), then clears it.
void drop_request(PPUContext& ctx, u32 slot) {
    u32 r = load<u32>(slot);
    if (r == 0) return;
    vcall(ctx, r, kVtCancel, {r});
    r = load<u32>(slot);
    if (r != 0) vcall(ctx, r, kVtDelete, {r});
    store<u32>(slot, 0);
}

// Releases the (key, arg) pair at pair through owner's vtable +8.
void drop_pair(PPUContext& ctx, u32 owner_slot, u32 pair) {
    const u32 key = load<u32>(pair);
    if (key == 0) return;
    const u32 o = load<u32>(owner_slot);
    vcall(ctx, o, kVtRelease, {o, key, load<u32>(pair + 4)});
    store<u32>(pair, 0);
    store<u32>(pair + 4, 0);
}
}  // namespace

// void Level_Release2D9A14(Level* l) (name?): drops the main request and
// the level's pair, frees every part holder (its request and pair first),
// deletes the extra objects, empties both vectors and resets the state. r3
// ends as the extras vector, as in the original.
DECOMP(002D9A14, Level_Release2D9A14) {
    const u32 l = arg<u32>(ctx, 0);
    drop_request(ctx, l + kLvMain);
    drop_pair(ctx, l + kLvOwner, l + kLvKey);
    u32 v;
    for (u32 i = 0, off = 0;; ++i, off += 4) {
        v = load<u32>(l + kLvRequests);
        if (!(i < (v != 0 ? load<u16>(v + kVecCount) : 0u))) break;
        const u32 h = load<u32>(load<u32>(v + kVecData) + off);
        if (h == 0) continue;
        drop_request(ctx, h + kHoldReq);
        drop_pair(ctx, h + kHoldOwner, h + kHoldKey);
        call(ctx, kMemFreeFrom, {h, 0});
        store<u32>(load<u32>(load<u32>(l + kLvRequests) + kVecData) + off, 0);
    }
    if (v != 0) store<u16>(v + kVecCount, 0);
    u32 w;
    for (u32 i = 0, off = 0;; ++i, off += 4) {
        w = load<u32>(l + kLvExtras);
        if (!(i < (w != 0 ? load<u16>(w + kVecCount) : 0u))) break;
        const u32 q = load<u32>(load<u32>(w + kVecData) + off);
        if (q == 0) continue;
        vcall(ctx, q, kVtDelete, {q});
        store<u32>(load<u32>(load<u32>(l + kLvExtras) + kVecData) + off, 0);
    }
    if (w != 0) store<u16>(w + kVecCount, 0);
    store<u32>(l + kLvState, 0);
    ret(ctx, w);
}
