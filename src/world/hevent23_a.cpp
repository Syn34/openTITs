// World_HandleEvent callees, part 23a: small callees named by the
// hevent22_*.cpp functions (item ownership, the float-keyed AVL insert, the
// track sub-record release, the effect component bind) and a reset helper.
#include "hevent22.h"

using namespace sly4;
using namespace sly4::hevent;

namespace {
// Item_Owned49E628: the session (0x00F64C20 +36), the item table (u32 count
// at 0x010C1A9C, 152-byte entries from 0x010BE8C0 {+0 key, +4 kind}) and
// the world's per-item byte flags (World +4028 + index).
constexpr u32 kSessionHolder = 0x00F64C20, kSessionPtr = 36;
constexpr u32 kItemCount = 0x010C1A9C, kItemKeys = 0x010BE8C0, kItemKinds = 0x010BE8C4, kItemSize = 152;
constexpr u32 kWorldItemFlags = 4028;
constexpr u32 kItemFlagOwned = 0x01, kItemFlagSeen = 0x02;

// AvlF_Insert78F898: AVL<float, ApeAnimScriptEvent*> {+0 root, +4 count};
// 20-byte nodes {+0 s16 balance, +2 s16 height, +4 f32 key, +8 value, +12
// left, +16 right} from the tree's node allocator, created on first use (a
// 60-byte ApeTFreeList of 64 x 20-byte nodes).
constexpr u32 kAvlFInsertBelow = 0x0078F410;  // (tree, -, value, tree; f32 key): insert below the root (name?)
constexpr u32 kAvlFAllocator = 0x00F8F0FC;    // ApeTFreeList*
constexpr u32 kAvlFListVtable = 0x00B22530;
constexpr u32 kAvlFAllocatorName = 0x00AEEC21;  // "static ApeTFreeList<AVL<...>::AVLNode<...>> &...GetAllocator() [with KeyType = float, ItemType = ApeAnimScriptEvent *]"
constexpr u32 kMemAlloc = 0x0054ED38, kFreeListInit = 0x00551A58;
constexpr u32 kFlSize = 60, kFlAlign = 16, kFlElem = 20, kFlCount = 64;
constexpr u32 kFlAllocSlot = 12;  // allocator vtable slot: () -> node

// Track_SubRelease38B174: sub-record {+0 u16 count, +8 Rec*, +12 s32 kind,
// +18 u16 first record index}.
constexpr u32 kSubCount = 0, kSubRec = 8, kSubKind = 12, kSubFirst = 18;

// Effect_Bind4BB7E0: the global map of bindable objects by hash (hevent6
// map layout from +12), its last value (0x011DE490), component type 5's
// bound object (+28, u16 refcount at +4).
constexpr u32 kEffectFindComponent = 0x004BB4B4;  // (Effect*, u32 hash) -> component or 0
constexpr u32 kWorldObjFindByHash = 0x005801DC;   // (u32 hash, 0) -> WorldObj* or 0
constexpr u32 kSplineMapFind = 0x007335C4;        // (map, const u32* key) -> value slot
constexpr u32 kSplineInstList = 0x00F8F158, kSplineMap = 16;
constexpr u32 kBindMap = 0x010C2750 + 12;  // hevent6 map: +0 Node** root, +4 path, +84 depth, +88 node
constexpr u32 kBindLast = 0x011DE490;
constexpr u32 kSlotType = 32, kSlotRelease = 8;  // vtable slots: () -> s32 type; release
constexpr u32 kTypeBindable = 5, kBound = 28, kRefCount = 4;

// 0028BF1C's word (name?).
constexpr u32 kWord1057870 = 0x01057870;
}  // namespace

// void Reset_28BF1C() (name?): clears the word at 0x01057870 (called by
// WorldObjs_Reset0CE110). Leaves r3 = 0x01050000 and r4 = 0 like the
// original (no caller reads them).
DECOMP(0028BF1C, Reset_28BF1C) {
    store<u32>(kWord1057870, 0);
    ctx.gpr[3] = 0x01050000u;
    ctx.gpr[4] = 0;
}

// bool Item_Owned49E628(s32 kind, s32 key) (name?): true without a session
// (0x00F64C20 +36 null). Otherwise false for a zero kind or key; else the
// item table (0x010BE8C0, 152-byte {key, kind} entries, u32 count at
// 0x010C1A9C) is searched and a match is owned when the world's byte at
// +4028 + index has both bit 0 and bit 1 set.
DECOMP(0049E628, Item_Owned49E628) {
    const u32 kind = arg<u32>(ctx, 0), key = arg<u32>(ctx, 1);
    if (load<u32>(load<u32>(kSessionHolder) + kSessionPtr) == 0) return ret(ctx, 1u);
    if (kind == 0 || key == 0) return ret(ctx, 0u);
    const u32 n = load<u32>(kItemCount);
    const u32 world = load<u32>(kCurrentWorld);
    if (n == 0) return ret(ctx, 0u);
    for (u32 i = 0; i < n; ++i) {
        const u32 off = i * kItemSize;
        if (load<u32>(kItemKinds + off) != kind || load<u32>(kItemKeys + off) != key) continue;
        const u32 f = load<u8>(world + i + kWorldItemFlags);
        return ret(ctx, ((f & kItemFlagOwned) != 0 && (f & kItemFlagSeen) != 0) ? 1u : 0u);
    }
    ret(ctx, 0u);
}

// void AvlF_Insert78F898(Tree* t, -, ApeAnimScriptEvent* value, f32 key)
// (name?): count (+4) incremented; a non-empty tree inserts through
// 0x0078F410(t, count, value, t; key). An empty one takes a node from the
// node allocator (vtable +12, the allocator created on first use; a failed
// allocation stores 0 and the original then calls through it anyway) and
// fills {0, 1, key, value, 0, 0} into the root (a null node is stored too).
// r3: the insert's result, or 0.
DECOMP(0078F898, AvlF_Insert78F898) {
    const u32 t = arg<u32>(ctx, 0), value = arg<u32>(ctx, 2);
    const f64 key = farg(ctx, 0);  // passed on unrounded; stored with stfs
    const u32 count = load<u32>(t + 4) + 1;
    store<u32>(t + 4, count);
    if (load<u32>(t) != 0) {
        callf(ctx, kAvlFInsertBelow, {t, count, value, t}, {key});
        return;
    }
    u32 fl = load<u32>(kAvlFAllocator);
    if (fl == 0) {
        const u32 a = static_cast<u32>(call(ctx, kMemAlloc, {kFlSize, 0, kFlAlign}));
        if (a != 0) {
            for (u32 o : {8u, 4u, 12u, 20u, 16u, 24u}) store<u32>(a + o, 0);
            const u32 f54 = load<u16>(a + 54);
            for (u32 o : {32u, 28u, 36u, 40u, 44u, 48u}) store<u32>(a + o, 0);
            store<u16>(a + 52, 0);
            store<u16>(a + 54, static_cast<u16>(f54 & ~0x8000u));
            store<u32>(a + 0, kAvlFListVtable);
            store<u32>(a + 56, kAvlFAllocatorName);
            ctx.gpr[8] = kAvlFAllocatorName;  // left in r8 by the original
            call(ctx, kFreeListInit, {a, kFlElem, kFlCount, 0, 0});
            fl = a;
        }
        store<u32>(kAvlFAllocator, fl);
    }
    const u32 node = static_cast<u32>(vcall(ctx, fl, kFlAllocSlot, {fl}));
    if (node != 0) {
        store<u16>(node + 0, 0);
        store<u16>(node + 2, 1);
        store<f32>(node + 4, static_cast<f32>(key));
        store<u32>(node + 8, value);
        store<u32>(node + 12, 0);
        store<u32>(node + 16, 0);
    }
    store<u32>(t, node);
    ctx.gpr[3] = 0;  // the original returns 0 here (the node or the zero stored)
}

// u32 Track_SubRelease38B174(Sub* sub, Base* base) (name?): by sub +12:
// kinds 5 / 2 and 4 / 1 call the +8 record's type +36 OPD (rec, base) when
// set, then clear the record's +8 (r3: the record, or 0 for kinds 4 / 1).
// Kind 3 walks its u16 +0 records from base index +18 + 1 (each read as
// the null record when out of range or of kind 10+): the record +4 slot's
// type +36 OPD (slot, base) when set, then slot +8 = 0 (r3: the count as
// last read). Other kinds do nothing (r3: the +8 record).
DECOMP(0038B174, Track_SubRelease38B174) {
    using namespace sly4::hevent22;
    const u32 sub = arg<u32>(ctx, 0), base = arg<u32>(ctx, 1);
    const u32 rec = load<u32>(sub + kSubRec);
    const s32 kind = static_cast<s32>(load<u32>(sub + kSubKind));
    auto type_fn = [](u32 p) { return load<u32>(type_entry(static_cast<s16>(load<u16>(p))) + 36); };
    if (kind == 5 || kind == 2 || kind == 4 || kind == 1) {
        const u32 fn = type_fn(rec);
        u32 r = rec;
        if (fn != 0) {
            ctx.gpr[5] = fn;
            ctx.gpr[6] = load<u32>(fn);
            call_ptr(ctx, fn, {rec, base});
            r = load<u32>(sub + kSubRec);
        }
        store<u32>(r + 8, 0);
        ctx.gpr[3] = (kind == 5 || kind == 2) ? r : 0;
        return;
    }
    if (kind != 3) {
        ctx.gpr[3] = rec;
        return;
    }
    u32 n = load<u16>(sub + kSubCount);
    ctx.gpr[3] = n;
    if (static_cast<s16>(n) <= 0) return;
    for (u32 i = 0;;) {
        const s32 k = static_cast<s16>(load<u16>(sub + kSubFirst) + i + 1);
        const u32 slot = rec_checked(base, k) + 4;
        const u32 fn = type_fn(slot);
        if (fn != 0) {
            ctx.gpr[5] = fn;
            ctx.gpr[6] = load<u32>(fn);
            call_ptr(ctx, fn, {slot, base});
            n = load<u16>(sub + kSubCount);
        }
        i = (i + 1) & 0xFFFF;
        store<u32>(slot + 8, 0);
        if (!(static_cast<s32>(i) < static_cast<s16>(n))) break;
    }
    ctx.gpr[3] = n;
}

// Bindable* Effect_Bind4BB7E0(Effect* e, u32 hash, u8 world) (name?): with
// world set, WorldObj_FindByHash(hash, 0), else the spline instance map's
// entry for hash. Otherwise the effect's component hash (when of type 5 and
// already bound, its +28 is returned) and the global map at 0x010C2750
// (hit: 0x011DE490 = value, the lookup repeated and the value's vtable
// +32 type checked; a type 5 value's +28 is the result; the original calls
// through a null value when the repeat misses). A result for a type 5
// component replaces its +28 (old one released via vtable +8) and gets
// its u16 +4 refcount incremented.
DECOMP(004BB7E0, Effect_Bind4BB7E0) {
    using namespace sly4::hevent6;
    const u32 e = arg<u32>(ctx, 0), hash = arg<u32>(ctx, 1);
    const u8 world = arg<u8>(ctx, 2);
    if (world != 0) {
        const u32 o = static_cast<u32>(call(ctx, kWorldObjFindByHash, {hash, 0}));
        if (o != 0) return ret(ctx, o);
        Stack frame(ctx, 16);
        store<u32>(frame.addr(), hash);
        const u32 slot = static_cast<u32>(call(ctx, kSplineMapFind, {load<u32>(load<u32>(kSplineInstList) + kSplineMap), frame.addr()}));
        return ret(ctx, load<u32>(slot));
    }
    const u32 c = static_cast<u32>(call(ctx, kEffectFindComponent, {e, hash}));
    u32 comp = 0;
    if (c != 0 && static_cast<s32>(vcall(ctx, c, kSlotType, {c})) == static_cast<s32>(kTypeBindable)) {
        comp = c;
        const u32 b = load<u32>(c + kBound);
        if (b != 0) return ret(ctx, b);
    }
    u32 result = 0;
    const u32 m = kBindMap;
    const u32 root = load<u32>(load<u32>(m));
    if (root == 0) {
        store<u32>(m + kMapNode, 0);
        store<u32>(kBindLast, 0);
    } else {
        const u32 n = map_walk(m, root, hash);
        if (n != 0) {
            store<u32>(m + kMapNode, n);
            store<u32>(kBindLast, load<u32>(n + kNodeValue));
            const u32 root2 = load<u32>(load<u32>(m));
            u32 obj = 0;
            if (root2 == 0) {
                store<u32>(kBindLast, 0);
                store<u32>(m + kMapNode, 0);
            } else {
                const u32 n2 = map_walk(m, root2, hash);
                if (n2 != 0) {
                    store<u32>(m + kMapNode, n2);
                    obj = load<u32>(n2 + kNodeValue);
                    store<u32>(kBindLast, obj);
                }
            }
            // Called through even when obj is 0 (original quirk).
            if (static_cast<s32>(vcall(ctx, obj, kSlotType, {obj})) == static_cast<s32>(kTypeBindable)) result = load<u32>(obj + kBound);
        }
    }
    if (result == 0 || comp == 0) return ret(ctx, result);
    const u32 old = load<u32>(comp + kBound);
    if (old != 0) vcall(ctx, old, kSlotRelease, {old});
    store<u32>(comp + kBound, result);
    store<u16>(result + kRefCount, static_cast<u16>(load<u16>(result + kRefCount) + 1));
    ret(ctx, result);
}
