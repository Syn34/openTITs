// Game code, large-function sweep part 35: the spawn group's constructor
// (the object Obj_SlotApply_000B6208 and Group_Reset0B7384 work on).
#include "../sly4.h"
#include "big35.h"

using namespace sly4;
using namespace sly4::big35;

namespace {
constexpr u32 kGroupVtable = 0x00B11E20;
constexpr u32 kItems = 0x00F657FC;           // u32: item manager (+36 header, u16 count at +2; +44 368-byte items)
constexpr u32 kMapHolderPoint = 0x00F8F154;  // header*: +16 the point map
constexpr u32 PtrVector_Construct = 0x005516E0;  // (vec)
constexpr u32 PtrVector_Init = 0x00551598;       // (vec, capacity, a, b)
constexpr u32 PtrVector_Reserve = 0x00551718;    // (vec, capacity)
constexpr u32 Save_Profile = 0x000C9FC8;         // (u32 hash) -> Profile*
constexpr u32 Hash_ToString = 0x0056A6E0;        // (u32 hash) -> const char* (the missing-name messages; result unused)
constexpr u32 Rec80_Find = 0x005848CC;           // (list, u32 hash) -> rec or 0
constexpr u32 Obj_IndexByHash = 0x000D14DC;      // (Obj*, u32 hash) -> s32 or -1
constexpr u32 Obj_FindByKey = 0x000D1454;        // (Obj*, u32 hash) -> WorldObj*
constexpr u32 PointMap_Find = 0x0073343C;        // (map, const u32* key) -> value slot*
constexpr u32 Item_Hash = 0x0039FEBC;            // (Item*) -> u32
constexpr u32 Obj_Call4A71D0 = 0x0039FEE0;       // (Item*, 0)
constexpr u32 Volume_Activate = 0x0039D32C;      // (Item*, 0, 1, 0)

// Appends x to the u16-counted vector at v (+0 data, +4 capacity, +8
// count, +10 high-water mark), doubling the capacity when full.
void Push(PPUContext& ctx, u32 v, u32 x) {
    const u32 cap = load<u16>(v + 4);
    if (load<u16>(v + 8) == cap) call(ctx, PtrVector_Reserve, {v, (cap << 1) & 0x1FFFEu});
    store<u32>(load<u32>(v) + ((static_cast<u32>(load<u16>(v + 8)) << 2) & 0x3FFFCu), x);
    const u32 n = load<u16>(v + 8) + 1u;
    const u32 hi = load<u16>(v + 10);
    store<u16>(v + 8, static_cast<u16>(n));
    store<u16>(v + 10, static_cast<u16>(static_cast<s32>(hi) > static_cast<s32>(n & 0xFFFF) ? hi : n));
}
}  // namespace

// u32 SpawnGroup_Construct0B5D14(Group* o, World* w, const Def* d) (from
// 0x000B9020, on a 152-byte SmallAlloc block): +0 the vtable, +4 w, +8 its
// top bit cleared; four u16 vectors (+12 capacity 4 / step 16, +32 4 / 16,
// +52 4 / 8, then the marker list +72 and its index list +92, both with
// d->8 entries / step 4, flag 1); +112 d, +116..+148 0 except +128 -1.
// Then, each missing name reported through Hash_ToString:
//   +120 = the template profile Save_Profile(d->60) if its +20 has bit
//          0x00200000 (left 0 when there is no profile);
//   if d->52: +124 = Rec80_Find(w->160->208->108, d->52) (0 without a
//          table); when that is 0, +128 = Obj_IndexByHash(w, d->52) and,
//          for -1, +132 = Obj_FindByKey(w, d->52);
//   +136 = the item whose Item_Hash is d->56 (first match among the item
//          manager's 368-byte items; 0 for none or d->56 == 0); a found item
//          gets Obj_Call4A71D0(item, 0) and Volume_Activate(+136, 0, 1, 0);
//   +116 = *PointMap_Find(points, &d->48);
//   for each non-zero point hash i of d->0 (d->8 entries) that the point map
//          resolves: the point onto +72 and i onto +92.
// Returns d->8 (the loop bound last read).
DECOMP(000B5D14, SpawnGroup_Construct0B5D14) {
    const u32 o = arg<u32>(ctx, 0), w = arg<u32>(ctx, 1), d = arg<u32>(ctx, 2);
    store<u32>(o + 4, w);
    store<u32>(o + 8, load<u32>(o + 8) & 0x7FFFFFFFu);
    store<u32>(o + 0, kGroupVtable);
    call(ctx, PtrVector_Construct, {o + 12});
    store<u16>(o + 18, 16);
    call(ctx, PtrVector_Init, {o + 12, 4, 0, 0});
    call(ctx, PtrVector_Construct, {o + 32});
    store<u16>(o + 38, 16);
    call(ctx, PtrVector_Init, {o + 32, 4, 0, 0});
    call(ctx, PtrVector_Construct, {o + 52});
    store<u16>(o + 58, 8);
    call(ctx, PtrVector_Init, {o + 52, 4, 0, 0});
    const u32 markers = o + 72, indices = o + 92;
    u32 n = load<u16>(d + 8);
    call(ctx, PtrVector_Construct, {markers});
    store<u16>(o + 78, 4);
    call(ctx, PtrVector_Init, {markers, n, 0, 1});
    n = load<u16>(d + 8);
    call(ctx, PtrVector_Construct, {indices});
    store<u16>(o + 98, 4);
    call(ctx, PtrVector_Init, {indices, n, 0, 1});
    store<u32>(o + 112, d);
    store<u32>(o + 116, 0);
    store<u32>(o + 120, 0);
    store<u32>(o + 124, 0);
    store<u32>(o + 128, 0xFFFFFFFFu);
    for (u32 k = 132; k <= 148; k += 4) store<u32>(o + k, 0);

    // the template profile
    const u32 prof = static_cast<u32>(call(ctx, Save_Profile, {load<u32>(d + 60)}));
    if (prof == 0) {
        if (const u32 h = load<u32>(load<u32>(o + 112) + 60); h != 0) call(ctx, Hash_ToString, {h});
    } else {
        const u32 dd = load<u32>(o + 112);
        const u32 t = (load<u32>(prof + 20) & 0x00200000u) != 0 ? prof : 0u;
        store<u32>(o + 120, t);
        if (t == 0) call(ctx, Hash_ToString, {load<u32>(dd + 60)});
    }

    // the owner object
    u32 dd = load<u32>(o + 112);
    if (const u32 key = load<u32>(dd + 52); key != 0) {
        u32 rec = 0;
        bool table = false;
        if (const u32 a = load<u32>(w + 160); a != 0) {
            if (const u32 b = load<u32>(a + 208); b != 0) {
                table = true;
                if (const u32 list = load<u32>(b + 108); list != 0) {
                    rec = static_cast<u32>(call(ctx, Rec80_Find, {list, key}));
                    dd = load<u32>(o + 112);
                }
                store<u32>(o + 124, rec);
            }
        }
        if (!table || rec == 0) {
            const u32 idx = static_cast<u32>(call(ctx, Obj_IndexByHash, {w, load<u32>(dd + 52)}));
            store<u32>(o + 128, idx);
            if (idx == 0xFFFFFFFFu)
                store<u32>(o + 132, static_cast<u32>(call(ctx, Obj_FindByKey, {w, load<u32>(load<u32>(o + 112) + 52)})));
        }
    }

    // the item
    const u32 items = load<u32>(kItems);
    const u32 itemHash = load<u32>(load<u32>(o + 112) + 56);
    u32 item = 0;
    if (itemHash != 0 && load<u16>(load<u32>(items + 36) + 2) != 0) {
        for (u32 i = 0, off = 0;; ++i, off += 368) {
            const u32 p = load<u32>(items + 44) + off;
            if (p != 0 && static_cast<u32>(call(ctx, Item_Hash, {p})) == itemHash) {
                item = p;
                break;
            }
            if (i + 1 >= load<u16>(load<u32>(items + 36) + 2)) break;
        }
    }
    store<u32>(o + 136, item);
    if (item != 0) {
        call(ctx, Obj_Call4A71D0, {item, 0});
        call(ctx, Volume_Activate, {load<u32>(o + 136), 0, 1, 0});
    } else if (const u32 h = load<u32>(load<u32>(o + 112) + 56); h != 0) {
        call(ctx, Hash_ToString, {h});
    }

    // the points
    Stack frame(ctx, 8);  // sp+112 (the list keys) and sp+116 (d->48)
    const u32 key = frame.addr();
    const u32 pointMap = load<u32>(load<u32>(kMapHolderPoint) + 16);
    store<u32>(key + 4, load<u32>(d + 48));
    const u32 pt = load<u32>(static_cast<u32>(call(ctx, PointMap_Find, {pointMap, key + 4})));
    store<u32>(o + 116, pt);
    if (pt == 0) call(ctx, Hash_ToString, {load<u32>(d + 48)});
    u32 cnt;
    for (u32 i = 0;; ++i) {
        cnt = load<u16>(d + 8);
        if (i >= cnt) break;
        const u32 h = load<u32>(load<u32>(d) + 4 * i);
        if (h == 0) continue;
        store<u32>(key, h);
        const u32 p = load<u32>(static_cast<u32>(call(ctx, PointMap_Find, {load<u32>(load<u32>(kMapHolderPoint) + 16), key})));
        if (p == 0) {
            call(ctx, Hash_ToString, {h});
            continue;
        }
        Push(ctx, markers, p);
        Push(ctx, indices, i);
    }
    ret(ctx, cnt);
}
