// Game code, large-function sweep part 35: the spawn group's constructor
// (the object Obj_SlotApply_000B6208 and Group_Reset0B7384 work on).
#include "native/objtypes.h"

namespace sly4::obj {

// A u16-counted vector {+0 data, +4 capacity, +6 element size, +8 count,
// +10 high-water mark} in a 20-byte slot.
struct UVec20 {
    be_u32 data;    // +0
    be_u16 cap;     // +4
    be_u16 step;    // +6
    be_u16 count;   // +8
    be_u16 mark;    // +10
    pad<8> _12;
};
static_assert(sizeof(UVec20) == 20);
struct SpawnGroupDef {
    gp<be_u32> hashes;   // +0: point hashes
    pad<4> _4;
    be_u16 n;            // +8
    pad<38> _10;
    be_u32 point;        // +48
    be_u32 owner;        // +52
    be_u32 item;         // +56
    be_u32 profile;      // +60
};
static_assert(offsetof(SpawnGroupDef, point) == 48 && offsetof(SpawnGroupDef, profile) == 60);
struct SpawnGroup {
    be_u32 vtable;       // +0
    be_u32 world;        // +4
    be_u32 flags;        // +8
    UVec20 v12, v32, v52;  // +12 / +32 / +52
    UVec20 markers;      // +72
    UVec20 indices;      // +92
    gp<SpawnGroupDef> def;  // +112
    be_u32 point;        // +116
    be_u32 profile;      // +120
    be_u32 rec;          // +124
    be_u32 index;        // +128
    be_u32 owner;        // +132
    be_u32 item;         // +136
    be_u32 w140[3];      // +140 .. +148
};
static_assert(sizeof(SpawnGroup) == 152 && offsetof(SpawnGroup, def) == 112);
struct SgProfile {
    pad<20> _0;
    be_u32 flags;  // +20: bit 0x00200000 = template
};
struct SgTable {
    pad<108> _0;
    be_u32 list;   // +108
};
struct SgHolder {
    pad<208> _0;
    gp<SgTable> table;  // +208
};
struct SgWorld {
    pad<160> _0;
    gp<SgHolder> holder;  // +160
};
struct SgItemHeader {
    pad<2> _0;
    be_u16 count;  // +2
};
struct SgItems {
    pad<36> _0;
    gp<SgItemHeader> header;  // +36
    pad<4> _40;
    be_u32 items;             // +44: 368-byte items
};
struct SgPointHolder {
    pad<16> _0;
    be_u32 map;  // +16
};

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

// Appends x to the u16-counted vector v, doubling the capacity when full.
void Push(UVec20* v, u32 x) {
    const u32 cap = v->cap;
    if (v->count == cap) call(PtrVector_Reserve, v, (cap << 1) & 0x1FFFEu);
    gm<be_u32>(v->data + ((static_cast<u32>(v->count) << 2) & 0x3FFFCu)) = x;
    const u32 n = v->count + 1u;
    const u32 hi = v->mark;
    v->count = static_cast<u16>(n);
    v->mark = static_cast<u16>(static_cast<s32>(hi) > static_cast<s32>(n & 0xFFFF) ? hi : n);
}
}  // namespace

// void SpawnGroup_Construct0B5D14(Group* o, World* w, const Def* d) (from
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
void SpawnGroup_Construct0B5D14(SpawnGroup* o, SgWorld* w, SpawnGroupDef* d) {
    o->world = guest(w);
    o->flags = o->flags & 0x7FFFFFFFu;
    o->vtable = kGroupVtable;
    call(PtrVector_Construct, &o->v12);
    o->v12.step = 16;
    call(PtrVector_Init, &o->v12, 4u, 0u, 0u);
    call(PtrVector_Construct, &o->v32);
    o->v32.step = 16;
    call(PtrVector_Init, &o->v32, 4u, 0u, 0u);
    call(PtrVector_Construct, &o->v52);
    o->v52.step = 8;
    call(PtrVector_Init, &o->v52, 4u, 0u, 0u);
    u32 n = d->n;
    call(PtrVector_Construct, &o->markers);
    o->markers.step = 4;
    call(PtrVector_Init, &o->markers, n, 0u, 1u);
    n = d->n;
    call(PtrVector_Construct, &o->indices);
    o->indices.step = 4;
    call(PtrVector_Init, &o->indices, n, 0u, 1u);
    o->def = d;
    o->point = 0u;
    o->profile = 0u;
    o->rec = 0u;
    o->index = 0xFFFFFFFFu;
    o->owner = 0u;
    o->item = 0u;
    for (u32 k = 0; k < 3; ++k) o->w140[k] = 0u;

    // the template profile
    SgProfile* prof = call<SgProfile*>(Save_Profile, d->profile.get());
    if (prof == nullptr) {
        if (const u32 h = o->def->profile; h != 0) call(Hash_ToString, h);
    } else {
        SpawnGroupDef* dd = o->def;
        const u32 t = (prof->flags & 0x00200000u) != 0 ? guest(prof) : 0u;
        o->profile = t;
        if (t == 0) call(Hash_ToString, dd->profile.get());
    }

    // the owner object
    SpawnGroupDef* dd = o->def;
    if (const u32 key = dd->owner; key != 0) {
        u32 rec = 0;
        bool table = false;
        if (SgHolder* a = w->holder) {
            if (SgTable* b = a->table) {
                table = true;
                if (const u32 list = b->list; list != 0) {
                    rec = call<u32>(Rec80_Find, list, key);
                    dd = o->def;
                }
                o->rec = rec;
            }
        }
        if (!table || rec == 0) {
            const u32 idx = call<u32>(Obj_IndexByHash, w, dd->owner.get());
            o->index = idx;
            if (idx == 0xFFFFFFFFu) o->owner = call<u32>(Obj_FindByKey, w, o->def->owner.get());
        }
    }

    // the item
    SgItems* items = global<gp<SgItems>>(kItems);
    const u32 itemHash = o->def->item;
    u32 item = 0;
    if (itemHash != 0 && items->header->count != 0) {
        for (u32 i = 0, off = 0;; ++i, off += 368) {
            const u32 p = items->items + off;
            if (p != 0 && call<u32>(Item_Hash, p) == itemHash) {
                item = p;
                break;
            }
            if (i + 1 >= items->header->count) break;
        }
    }
    o->item = item;
    if (item != 0) {
        call(Obj_Call4A71D0, item, 0u);
        call(Volume_Activate, o->item.get(), 0u, 1u, 0u);
    } else if (const u32 h = o->def->item; h != 0) {
        call(Hash_ToString, h);
    }

    // the points
    GuestFrame frame(8);  // the list keys (+0) and d->48 (+4)
    be_u32* key = frame.as<be_u32>(0);
    be_u32* pkey = frame.as<be_u32>(4);
    const u32 pointMap = global<gp<SgPointHolder>>(kMapHolderPoint)->map;
    *pkey = d->point.get();
    const u32 pt = *call<be_u32*>(PointMap_Find, pointMap, pkey);
    o->point = pt;
    if (pt == 0) call(Hash_ToString, d->point.get());
    for (u32 i = 0;; ++i) {
        const u32 cnt = d->n;
        if (i >= cnt) break;
        const u32 h = d->hashes.get()[i];
        if (h == 0) continue;
        *key = h;
        const u32 p = *call<be_u32*>(PointMap_Find, global<gp<SgPointHolder>>(kMapHolderPoint)->map.get(), key);
        if (p == 0) {
            call(Hash_ToString, h);
            continue;
        }
        Push(&o->markers, p);
        Push(&o->indices, i);
    }
}

}  // namespace sly4::obj

NATIVE(000B5D14, sly4::obj::SpawnGroup_Construct0B5D14)
