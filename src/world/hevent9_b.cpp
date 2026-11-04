// World_HandleEvent callees, part 9b: the handle-object teardown.
#include "hevent9.h"

using namespace sly4;
using namespace sly4::hevent;
using namespace sly4::hevent7;
using namespace sly4::hevent9;

namespace sly4 {
namespace hevent9b {
namespace bfn {
constexpr u32 SortedMap_Remove = 0x00771DD0;     // (map, const u32* key)
constexpr u32 SortedMap_Find3 = 0x0076611C;      // (map, const u32* key) -> value*
constexpr u32 Objects_Reset35B470 = 0x0035B470;  // (Highlight*) (name?)
}  // namespace bfn

// kHandleMap (0x0106C228): +2 u16 count, +8 {key, Object*}[].
constexpr u32 kMapCount = 2, kMapEntries = 8;
// Five more object lists ahead of / at kObjLists (20 bytes each, u16 count at +8).
constexpr u32 kLists = 0x0106C2C0, kListCount = 5;
// Owned objects: a PtrVector {+0 Object**, +8 u16 count} and a map of the
// same shape as kHandleMap; then one more list cleared with them.
constexpr u32 kOwned = 0x0106C280, kOwnedMap = 0x0106C240, kList294 = 0x0106C294;
constexpr u32 kHighlight = 0x0106C330;
constexpr u32 kFlag106C3C0 = 0x0106C3C0;  // u8 (name?)
constexpr u32 kWord_F64CC8 = 0x00F64CC8;  // u32 (name?)
constexpr u32 kPlayerHandle = 0x00F64D48; // u32 handle of the player object (name?)
constexpr u32 kResetHash = 0x25F79944;    // given to 0x004CF988 (name?)
// Object: +4 flags (bit 1: mark +212, bit 16: player-ready), +8 def (+68
// handle), +12 u32 handle, +20 sub-object, +212 u8, +960 flags.
constexpr u32 kObjFlags4 = 4, kObjDef = 8, kObjHandle = 12, kObjSub = 20, kObjMark = 212, kObjFlags960 = 960;
constexpr u32 kDefHandle = 68;
constexpr u32 kVRelease = 4, kVReset = 12;  // vtable slots
}  // namespace hevent9b
}  // namespace sly4

using namespace sly4::hevent9b;

namespace {
// An object's handle: its own (+12), else its def's (+68), else 0.
u32 handle_of(u32 o) {
    const u32 h = load<u32>(o + kObjHandle);
    if (h != 0) return h;
    const u32 def = load<u32>(o + kObjDef);
    return def != 0 ? load<u32>(def + kDefHandle) : 0;
}

void map_remove(PPUContext& ctx, Stack& frame, u32 off, u32 key) {
    store<u32>(frame.addr() + off, key);
    call(ctx, bfn::SortedMap_Remove, {kHandleMap, frame.addr() + off});
}
}  // namespace

// void Objects_Release35B984(u8 keep_player) (name?): every object in the
// handle map is reset (bit 1 of +4 sets +212 first; +20's 0x004CF988 with
// 0x25F79944; vtable +12); the five object lists are emptied; every owned
// object (vector 0x0106C280, map 0x0106C240) leaves the handle map and is
// released (vtable +4), the vector by swap-remove (the moved one is skipped,
// as in the original); then Objects_Reset35B470(0x0106C330) and two flags
// cleared. With keep_player, the player object (handle 0x00F64D48) gets
// +960 bit 20 when its +4 bit 16 is set. Null objects are read through as in
// the original (+20 / +12 of 0).
DECOMP(0035B984, Objects_Release35B984) {
    const u32 keep_player = arg<u32>(ctx, 0) & 0xFF;
    Stack frame(ctx, 16);  // the keys at r1+112, +116, +120
    for (s32 i = 0; i < static_cast<s32>(load<u16>(kHandleMap + kMapCount)); ++i) {
        const u32 o = load<u32>(load<u32>(kHandleMap + kMapEntries) + static_cast<u32>(i) * 8 + 4);
        if (o != 0 && (load<u32>(o + kObjFlags4) & 0x2u) != 0) store<u8>(o + kObjMark, 1);
        const u32 sub = load<u32>(o + kObjSub);
        if (sub != 0) call(ctx, hfn::sub_004CF988, {sub, kResetHash});
        vcall(ctx, o, kVReset, {o});
    }
    for (u32 k = 0; k < kListCount; ++k) store<u16>(kLists + 20 * k + 8, 0);

    for (u32 i = 0, off = 0; i < load<u16>(kOwned + 8); ++i, off += 4) {
        map_remove(ctx, frame, 0, handle_of(load<u32>(load<u32>(kOwned) + off)));
        u32 data = load<u32>(kOwned);
        const u32 o = load<u32>(data + off);
        if (o != 0) {
            vcall(ctx, o, kVRelease, {o});
            store<u32>(load<u32>(kOwned) + off, 0);
            data = load<u32>(kOwned);
        }
        const u32 n = (load<u16>(kOwned + 8) - 1) & 0xFFFF;
        store<u16>(kOwned + 8, static_cast<u16>(n));
        if (i != n) store<u32>(data + off, load<u32>(data + n * 4));
    }
    store<u16>(kOwned + 8, 0);
    store<u16>(kList294 + 8, 0);

    for (s32 i = 0; i < static_cast<s32>(load<u16>(kOwnedMap + kMapCount)); ++i) {
        const u32 o = load<u32>(load<u32>(kOwnedMap + kMapEntries) + static_cast<u32>(i) * 8 + 4);
        map_remove(ctx, frame, 4, handle_of(o));
        if (o != 0) vcall(ctx, o, kVRelease, {o});
    }
    store<u16>(kOwnedMap + kMapCount, 0);
    call(ctx, bfn::Objects_Reset35B470, {kHighlight});
    store<u8>(kFlag106C3C0, 0);
    store<u32>(kWord_F64CC8, 0);
    if (keep_player == 0) {
        ctx.gpr[3] = 0x01070000;  // r3 as the original leaves it (lis for the stores)
        return;
    }
    const u32 h = load<u32>(kPlayerHandle);
    if (h == 0) {
        ctx.gpr[3] = 0;
        return;
    }
    store<u32>(frame.addr() + 8, h);
    const u32 o = load<u32>(static_cast<u32>(call(ctx, bfn::SortedMap_Find3, {kHandleMap, frame.addr() + 8})));
    if (o != 0 && (load<u32>(o + kObjFlags4) & 0x00010000u) != 0)
        store<u32>(o + kObjFlags960, load<u32>(o + kObjFlags960) | 0x00100000u);
    ctx.gpr[3] = o;
}
