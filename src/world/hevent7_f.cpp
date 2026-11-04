// World_HandleEvent callees, part 7f: the sorted insert into the per-kind
// object list (the inverse of Obj_Prepare3629D8) and the player / group
// refresh case.
#include "hevent7.h"

using namespace sly4;
using namespace sly4::hevent;
using namespace sly4::hevent7;

namespace sly4 {
namespace hevent7f {
namespace ffn {
constexpr u32 PtrVector_Reserve = 0x00551718;    // (Vector*, u32 capacity)
constexpr u32 Obj_Pos3628B4 = 0x003628B4;        // (vec3* out, Object*)
constexpr u32 Obj_Place3626E8 = 0x003626E8;      // (Object*, const vec3*, u8 snap)
constexpr u32 SortedMap_Find73B608 = 0x0073B608; // (Map*, const u32* key) -> value*
constexpr u32 Map_Find7665B4 = 0x007665B4;       // (Map*, const u32* key) -> value* (name?)
constexpr u32 Ev_ApplyGroup2DFAA0 = 0x002DFAA0;  // (Event*, Group*, SaveSlot* +204, u8 is_main) (name?)
}  // namespace ffn

// Object list (20 bytes): +0 Object** data, +4 u16 capacity, +8 u16 count,
// +10 u16 high-water mark. Kind descriptor (+8 of the object): +84 kind,
// +112 s32 order.
constexpr u32 kListData = 0, kListCap = 4, kListCount = 8, kListHigh = 10;
constexpr u32 kDescKind = 84, kDescOrder = 112;

constexpr u32 kGroups = 0x00F65BE8;        // u32: +28 SortedMap of groups by name hash (name?)
constexpr u32 kHashPlayerGroups = 0xFB673D64, kHashNpcGroups = 0x5E69B571;
constexpr u32 kNpcMap = 0x010681B4;        // map of NPC ids -> {u8 player index} (name?)
constexpr u32 kWorldMainPlayer = 0x128114; // u32 World +0x128114: main player id (name?)
// Event: +20 Player[] (860 bytes: +748 u32 ids[], +820 u8 id count, +822 u8
// slot, +826 u8 gone), +24 u32 count, +36 Player* (or 0), +744 its id.
constexpr u32 kPlayerSize = 860;
constexpr u32 kSlotState = 496;            // SaveSlot +496 + 4 * slot: u16, low 3 bits
}  // namespace hevent7f
}  // namespace sly4

using namespace sly4::hevent7f;

// void Obj_AddToList36341C(Object* o) (name?): inserts o (unless it has no
// descriptor, is kind 5 or is already listed) into its kind's object list,
// sorted by the descriptor's order (+112; o goes before the first larger
// one), growing the list (x2) when full and keeping the high-water mark.
// Appended: o is placed at its position (Obj_Pos3628B4, snapped). Inserted:
// o and every entry after it are re-placed, o snapped.
// Quirk: the insert reads the last entry through the data pointer from
// before PtrVector_Reserve.
DECOMP(0036341C, Obj_AddToList36341C) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 desc = load<u32>(o + 8);
    if (desc == 0) return;
    const u32 kind = load<u32>(desc + kDescKind);
    if (kind == kKindNone) return;
    const u32 list = kObjLists + kind * kObjListStride;
    const u32 n = load<u16>(list + kListCount);
    for (u32 i = 0; i < n; ++i)
        if (load<u32>(load<u32>(list + kListData) + 4 * i) == o) {
            ret(ctx, n);  // r3 as the original leaves it (two virtual callers return it)
            return;
        }
    const s32 order = load<s32>(desc + kDescOrder);
    u32 pos = 0;
    if (n != 0) {
        for (; pos < n; ++pos) {
            const u32 d = load<u32>(load<u32>(load<u32>(list + kListData) + 4 * pos) + 8);
            if ((d != 0 ? load<s32>(d + kDescOrder) : 0) > order) break;
        }
    }
    auto bump = [&](u32 count) {  // count + 1 and the high-water mark
        const u32 c = count + 1;
        const u32 high = load<u16>(list + kListHigh);
        store<u16>(list + kListCount, static_cast<u16>(c));
        store<u16>(list + kListHigh, static_cast<u16>(static_cast<s32>(high) > static_cast<s32>(c & 0xFFFF) ? high : c));
        return c & 0xFFFF;
    };
    Stack frame(ctx, 32);  // vec3 at r1+112 (append) / r1+128 (insert)
    if (pos == n) {
        u32 count = n;
        const u32 cap = load<u16>(list + kListCap);
        if (n == cap) {
            call(ctx, ffn::PtrVector_Reserve, {list, (cap << 1) & 0x1FFFE});
            count = load<u16>(list + kListCount);
        }
        const u32 data = load<u32>(list + kListData);
        const u32 c = load<u16>(list + kListCount);
        store<u32>(data + ((count << 2) & 0x3FFFC), o);
        bump(c);
        call(ctx, ffn::Obj_Pos3628B4, {frame.addr(), o});
        call(ctx, ffn::Obj_Place3626E8, {o, frame.addr(), 1});
        return;
    }
    const u32 last = load<u32>(list + kListData) + (n - 1) * 4;  // before any reserve
    u32 data = load<u32>(list + kListData);
    u32 count = n;
    const u32 cap = load<u16>(list + kListCap);
    if (n == cap) {
        call(ctx, ffn::PtrVector_Reserve, {list, (cap << 1) & 0x1FFFE});
        data = load<u32>(list + kListData);
        count = load<u16>(list + kListCount);
    }
    const u32 v = load<u32>(last);
    const u32 c = load<u16>(list + kListCount);
    store<u32>(((count << 2) & 0x3FFFC) + data, v);
    const u32 now = bump(c);
    for (u32 k = now - 1; k > pos; --k) {
        const u32 d = load<u32>(list + kListData);
        store<u32>(d + 4 * k, load<u32>(d + 4 * k - 4));
    }
    store<u32>(load<u32>(list + kListData) + 4 * pos, o);
    const u32 vec = frame.addr() + 16;
    u32 j = pos, total;
    for (; j < (total = load<u16>(list + kListCount)); ++j) {
        call(ctx, ffn::Obj_Pos3628B4, {vec, load<u32>(load<u32>(list + kListData) + 4 * j)});
        call(ctx, ffn::Obj_Place3626E8, {load<u32>(load<u32>(list + kListData) + 4 * j), vec, j == pos ? 1u : 0u});
    }
    ret(ctx, total);  // r3 as the original leaves it
}

// void Ev_RefreshGroups2DFC2C(Event* ev) (name?): the acting player (+36,
// else the one holding the world's main player id) applies every player
// group (0xFB673D64) to its save slot (+204), flagged when it is the main
// player; then each NPC group (0x5E69B571) whose owner (+60 -> +204) maps
// to a present player in slot state 3 gets virtual +24.
DECOMP(002DFC2C, Ev_RefreshGroups2DFC2C) {
    const u32 ev = arg<u32>(ctx, 0);
    const u32 w = load<u32>(kCurrentWorld);
    const u32 save = w + load<u8>(w + kUserBlocks) * kSaveSlotSize;
    u32 player = load<u32>(ev + 36);
    if (player == 0) {
        const u32 id = load<u32>(w + kWorldMainPlayer);
        const u32 count = load<u32>(ev + 24);
        for (u32 i = 0; i < count && player == 0; ++i) {
            const u32 p = load<u32>(ev + 20) + i * kPlayerSize;
            if (load<u8>(p + 826) != 0) continue;
            const u32 m = load<u8>(p + 820);
            for (u32 k = 0; k < m; ++k)
                if (load<u32>(p + 748 + 4 * k) == id) {
                    player = p;
                    break;
                }
        }
    }
    Stack frame(ctx, 16);  // keys at r1+120 / +116 / +112
    store<u32>(frame.addr() + 8, kHashPlayerGroups);
    u32 g = load<u32>(static_cast<u32>(call(ctx, ffn::SortedMap_Find73B608, {load<u32>(kGroups) + 28, frame.addr() + 8})));
    if (g != 0 && player != 0) {
        const u32 slot = save + 204;
        const u32 pid = load<u32>(player + 744);
        const u32 main = pid != 0 && pid == load<u32>(load<u32>(kCurrentWorld) + kWorldMainPlayer) ? 1 : 0;
        for (g = load<u32>(g + 32); g != 0; g = load<u32>(g + 72)) call(ctx, ffn::Ev_ApplyGroup2DFAA0, {ev, g, slot, main});
    }
    store<u32>(frame.addr() + 4, kHashNpcGroups);
    g = load<u32>(static_cast<u32>(call(ctx, ffn::SortedMap_Find73B608, {load<u32>(kGroups) + 28, frame.addr() + 4})));
    if (g == 0) return;
    for (g = load<u32>(g + 32); g != 0; g = load<u32>(g + 72)) {
        const u32 owner = load<u32>(load<u32>(g + 60) + 204);
        if (owner == 0) continue;
        store<u32>(frame.addr(), owner);
        const u32 y = load<u32>(static_cast<u32>(call(ctx, ffn::Map_Find7665B4, {kNpcMap, frame.addr()})));
        if (y == 0) continue;
        const u32 p = load<u32>(ev + 20) + load<u8>(y) * kPlayerSize;
        if (p == 0 || load<u8>(p + 826) != 0) continue;
        const u32 w2 = load<u32>(kCurrentWorld);
        const u32 st = load<u16>(w2 + load<u8>(w2 + kUserBlocks) * kSaveSlotSize + load<u8>(p + 822) * 4 + kSlotState) & 7;
        if (st == 3) hevent::vcall(ctx, g, 24, {g});
    }
}

namespace sly4 {
namespace hevent7f {
constexpr u32 Audio_StartFocusSound = 0x00444610;  // (u32* out, SoundDesc*, PlayDesc*)
// PlayDesc (80 bytes, as audio/focus_start.h): +0 position (flag 2), +16
// velocity, +32 f32 gain (flag 4), +36 u32, +40 u32, +44 u8[4] (flags
// 0x10000 << i), +48 key, +52 priority, +56 flags (bit 0 from the u8
// argument), +60 bank, +64 shape (both ref-counted at +178, released
// through virtual +40).
constexpr u32 kOne = 0x3F800000;
constexpr u32 kRefs = 178;
}  // namespace hevent7f
}  // namespace sly4

// void Sound_Play44C474(u32* out, SoundDesc** desc, const vec3* pos,
// const f32* gain, u32 a, u8 b, u32 c, Bank* bank, Shape* shape,
// const u8* p0, const u8* p3) (name?): fills a PlayDesc on the stack
// (inlined constructor: zero vectors, gain 1.0) from the optional
// arguments, takes a reference on bank / shape, starts *desc
// (Audio_StartFocusSound) when there is one and stores the handle (or 0)
// in *out after dropping the references. The frame has the original's
// layout, so the fields the constructor leaves unset (+12, +28, +52, +68..)
// hold the same stack bytes. Arguments 9-11 come on the caller's stack.
DECOMP(0044C474, Sound_Play44C474) {
    const u32 out = arg<u32>(ctx, 0), descp = arg<u32>(ctx, 1), pos = arg<u32>(ctx, 2), gain = arg<u32>(ctx, 3);
    const u32 a = arg<u32>(ctx, 4), b = arg<u32>(ctx, 5) & 0xFF, c = arg<u32>(ctx, 6), bank = arg<u32>(ctx, 7);
    const u32 sp = static_cast<u32>(ctx.gpr[1]);
    const u32 shape = load<u32>(sp + 116), p0 = load<u32>(sp + 124), p3 = load<u32>(sp + 132);
    Stack frame(ctx, 272);
    const u32 d = frame.addr() + 112, res = frame.addr() + 192;
    for (u32 off : {0u, 16u}) {
        for (u32 k = 0; k < 12; k += 4) store<u32>(d + off + k, load<u32>(kZeroVec + k));
    }
    store<u32>(d + 32, kOne);
    store<u32>(d + 36, 0);
    store<u32>(d + 40, 0);
    store<u32>(d + 44, 0);  // the four bytes +44..+47
    store<u32>(d + 48, 0);
    store<u32>(d + 60, 0);
    store<u32>(d + 64, 0);
    u32 flags = 0;
    if (pos != 0) {
        for (u32 k = 0; k < 12; k += 4) store<u32>(d + k, load<u32>(pos + k));
        flags |= 2;
    }
    if (gain != 0) {
        store<u32>(d + 32, load<u32>(gain));
        flags |= 4;
    }
    store<u32>(d + 36, a);
    flags = (flags & ~1u) | (b != 0 ? 1u : 0u);
    store<u32>(d + 40, c);
    // (The inlined assignments release the previous bank / shape, which are
    // always 0 here.)
    if (bank != 0) store<u16>(bank + kRefs, static_cast<u16>(load<u16>(bank + kRefs) + 1));
    store<u32>(d + 60, bank);
    if (shape != 0) store<u16>(shape + kRefs, static_cast<u16>(load<u16>(shape + kRefs) + 1));
    store<u32>(d + 64, shape);
    if (p0 != 0) {
        store<u8>(d + 44, load<u8>(p0));
        flags |= 0x10000;
    }
    if (p3 != 0) {
        store<u8>(d + 47, load<u8>(p3));
        flags |= 0x80000;
    }
    store<u32>(d + 56, flags);
    // f1 as the original leaves it (the last lfs) unless a callee changes it.
    ctx.fpr[1] = lfs_f64(gain != 0 ? gain : pos != 0 ? pos + 8 : kZeroVec);
    u32 handle = 0;
    if (descp != 0) {
        if (const u32 desc = load<u32>(descp); desc != 0) {
            call(ctx, Audio_StartFocusSound, {res, desc, d});
            handle = load<u32>(res);
        }
    }
    if (const u32 x = load<u32>(d + 60); x != 0) hevent::vcall(ctx, x, 40, {x, 0});
    if (const u32 x = load<u32>(d + 64); x != 0) {
        store<u32>(d + 60, 0);
        hevent::vcall(ctx, x, 40, {x, 0});
    } else {
        ctx.gpr[3] = 0;  // r3 as the original leaves it
    }
    store<u32>(out, handle);
}
