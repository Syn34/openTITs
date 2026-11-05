// World_HandleEvent callees, part 25a: an effect-slot reset, the effect set
// registry's find-or-create, a level table loader and the spawn query
// constructor.
#include "hevent22.h"

using namespace sly4;
using namespace sly4::hevent;

namespace {
constexpr u32 kEffectRelease = 0x004CF3C4;     // (Effect*, u32 hash) -> bool (symbols.txt Effect_Release)
constexpr u32 kEffectStart = 0x004C753C;       // (Effect*) (symbols.txt Effect_Start)
constexpr u32 kShapeRelease = 0x00638498;      // (Shape*) (symbols.txt Shape_Release638498)
constexpr u32 kObjResetEffects = 0x0039CD68;   // (Object*) (symbols.txt Obj_ResetEffects39CD68)
constexpr u32 kNop = 0x0054EC18;               // (symbols.txt Nop)
constexpr u32 kArrayDestruct = 0x0054EC94;     // (Array*) (symbols.txt Array_Destruct)
constexpr u32 kMemAlloc = 0x0054ED38;          // (size, pool, align)
constexpr u32 kAvlTreeInsert = 0x00791D64;     // (tree, u32 key, value) (symbols.txt Avl_TreeInsert_00791D64)
constexpr u32 kObjConstruct2E8DFC = 0x002E8DFC;  // (Obj*) (symbols.txt)
constexpr u32 kHolderNew = 0x00550E70;         // () -> u32 (symbols.txt Holder_New550E70)
constexpr u32 kStringDup = 0x00556038;         // (const char*) -> pooled copy
constexpr u32 kSortedMapFind = 0x00766DCC;     // (map, const u32* key) -> value slot (symbols.txt SortedMap_Find766DCC)
constexpr u32 kStringCopy = 0x00553684;        // (dst, src, size)
constexpr u32 kPtrVectorConstruct = 0x005516E0, kPtrVectorInit = 0x00551598;
constexpr u32 kSmallAlloc = 0x0055238C;        // (size)
constexpr u32 kVectorAdd = 0x00733964;         // (Vector*, const Rec* 192 bytes) (symbols.txt Vector_Add733964)

void copy32(u32 dst, u32 src) { store<u32>(dst, load<u32>(src)); }
}  // namespace

// ---------------------------------------------------------------- 0x003A1500
namespace {
// Effect-slot owner: +4 / +20 / +36 count headers (u16 at +2) of the arrays
// at +12 (112-byte buffers), +28 (336-byte effect slots), +44 (368-byte
// effect objects); +70 u16.
constexpr u32 kBufHdr = 4, kBufs = 12, kSlotHdr = 20, kSlots = 28, kFxHdr = 36, kFxs = 44, kExtra = 70;
constexpr u32 kBufSize = 112, kSlotSize = 336, kFxSize = 368;
constexpr u32 kSlotSwitchHash = 0x00010CBB;    // def hash given to Effect_Release
constexpr u32 kTimerHash = 0x044DC6AB;         // written to a slot +280 owner's +276
}  // namespace

// void Obj_ResetFx3A1500(Owner* o) (name?): for every effect slot, detaches
// the +280 owner (its +280 = 0, +276 = 0x044DC6AB), releases the +288
// object (vt +8), switches both +76 effects (96 apart) to def 0x10CBB, sets
// their +136 bit 9 and restarts them, then stops the +308 emitter and
// releases the +312 shape. Each effect object is reset and its four
// vectors (+60, +40, +20, +0) cleared; each buffer's data (+84, count +76
// times stride +82 / 2) goes back to its +88 allocator (vt +8), then its
// +76 array and shape are released. All three counts and +70 end at 0.
DECOMP(003A1500, Obj_ResetFx3A1500) {
    const u32 o = arg<u32>(ctx, 0);
    if (static_cast<s32>(load<u16>(load<u32>(o + kSlotHdr) + 2)) != 0) {
        for (u32 i = 0; i < load<u16>(load<u32>(o + kSlotHdr) + 2); ++i) {
            const u32 e = load<u32>(o + kSlots) + i * kSlotSize;
            if (const u32 p = load<u32>(e + 280); p != 0) {
                store<u32>(p + 280, 0);
                store<u32>(p + 276, kTimerHash);
            }
            if (const u32 q = load<u32>(e + 288); q != 0) {
                vcall(ctx, q, 8, {q});
                store<u32>(e + 288, 0);
            }
            for (u32 j = 0; j < 2; ++j) {
                const u32 s = e + 96 * j;
                const u32 fx = load<u32>(s + 76);
                if (fx == 0) continue;
                call(ctx, kEffectRelease, {fx, kSlotSwitchHash});
                store<u32>(fx + 136, load<u32>(fx + 136) | 0x200);
                call(ctx, kEffectStart, {fx});
                store<u32>(s + 76, 0);
                store<u32>(s + 80, 0);
            }
            call(ctx, hfn::Emitter_Stop, {load<u32>(e + 308), 1});
            store<u32>(e + 308, 0);
            call(ctx, kShapeRelease, {e + 312});
        }
    }
    if (static_cast<s32>(load<u16>(load<u32>(o + kFxHdr) + 2)) != 0) {
        for (u32 i = 0; i < load<u16>(load<u32>(o + kFxHdr) + 2); ++i) {
            const u32 e = load<u32>(o + kFxs) + i * kFxSize;
            call(ctx, kObjResetEffects, {e});
            for (u32 off : {60u, 40u, 20u, 0u}) {
                call(ctx, hfn::PtrVector_Clear, {e + off});
                call(ctx, kNop, {e + off});
            }
        }
    }
    if (static_cast<s32>(load<u16>(load<u32>(o + kBufHdr) + 2)) != 0) {
        for (u32 i = 0; i < load<u16>(load<u32>(o + kBufHdr) + 2); ++i) {
            const u32 e = load<u32>(o + kBufs) + i * kBufSize;
            const u32 size = static_cast<u32>(load<u16>(e + 76)) * (static_cast<u32>(load<u16>(e + 82)) >> 1);
            if (const u32 data = load<u32>(e + 84); data != 0) {
                const u32 alloc = load<u32>(e + 88);
                vcall(ctx, alloc, 8, {alloc, data, size});
            }
            call(ctx, kArrayDestruct, {e + 76});
            call(ctx, kShapeRelease, {e});
        }
    }
    const u32 bufs = load<u32>(o + kBufHdr);
    const u32 fxs = load<u32>(o + kFxHdr);
    store<u16>(bufs + 2, 0);
    store<u16>(load<u32>(o + kSlotHdr) + 2, 0);
    store<u16>(fxs + 2, 0);
    store<u16>(o + kExtra, 0);
    ret(ctx, fxs);  // r3 as the original leaves it
}

// ---------------------------------------------------------------- 0x004BCE90
namespace {
// The effect set registry: an AVL tree at 0x010C2888 whose map (hevent6.h
// layout) is at +12 (root slot +12, path +16, length +96, current +100).
constexpr u32 kRegistry = 0x010C2888;
constexpr u32 kRegistryMap = kRegistry + 12;
constexpr u32 kRegistryLast = 0x011DE4A4;      // u32 value of the last hit (or 0 for an empty tree)
constexpr u32 kSetSize = 108;

// One inlined lookup: returns the value of the node keyed name, or 0, and
// sets hit. An empty tree stores 0 to the last-value global and then the
// current node; a hit stores the node, then its value; a miss stores the
// last node visited.
u32 registry_lookup(u32 name, bool& hit) {
    using namespace hevent6;
    const u32 m = kRegistryMap;
    hit = false;
    const u32 root = load<u32>(load<u32>(m));
    if (root == 0) {
        store<u32>(kRegistryLast, 0);
        store<u32>(m + kMapNode, 0);
        return 0;
    }
    store<u16>(m + kMapDepth, 0);
    u32 n = root;
    for (;;) {
        const u32 k = load<u32>(n + kNodeKey);
        if (k == name) {
            store<u32>(m + kMapNode, n);
            store<u32>(kRegistryLast, load<u32>(n + kNodeValue));
            hit = true;
            return load<u32>(n + kNodeValue);
        }
        const u32 next = k > name ? load<u32>(n + kNodeLeft) : load<u32>(n + kNodeRight);
        if (next == 0) {
            store<u32>(m + kMapNode, n);
            return 0;
        }
        map_push(m, n);
        n = next;
    }
}
}  // namespace

// Set* Set_Find(u32 name): the effect set named name, created when missing.
// A new set is a 108-byte block (Mem_Alloc(108, 0, 16); +4 / +8 = 0) whose map at
// +16 has its root slot at +4 (+12 = 1, +100 / +104 = 0), inserted into the
// registry (a null set is inserted when the allocation fails); the set
// found again then gets +0 = name (quirk: written to guest address 0 when
// that lookup misses). Returns the final lookup's result.
DECOMP(004BCE90, Set_Find) {
    const u32 name = arg<u32>(ctx, 0);
    bool hit;
    registry_lookup(name, hit);  // only decides whether to create
    if (!hit) {
        u32 set = static_cast<u32>(call(ctx, kMemAlloc, {kSetSize, 0, 16}));
        if (set != 0) {
            store<u32>(set + 4, 0);
            store<u8>(set + 12, 1);
            store<u32>(set + 16, set + 4);
            store<u16>(set + 100, 0);
            store<u32>(set + 8, 0);
            // Inlined map_first on the new (empty) map: the root read back is 0.
            (void)load<u32>(set + 4);
            store<u32>(set + 104, 0);
            store<u8>(set + 12, 1);
        }
        call(ctx, kAvlTreeInsert, {kRegistry, name, set});
        const u32 v = registry_lookup(name, hit);
        store<u32>(v, name);  // quirk: v == 0 writes guest address 0
    }
    ret(ctx, registry_lookup(name, hit));
}

// ---------------------------------------------------------------- 0x002E9338
namespace {
constexpr u32 kTableCount = 0x010686C0;        // u32 entries loaded (0 = not yet)
constexpr u32 kTableMap = 0x010686A8;          // {u32 key, Obj*} map (hevent22.h map_append layout)
constexpr u32 kNameMapA = 0x010686C4, kNameMapB = 0x010686DC;  // SortedMaps searched by the item keys
constexpr u32 kItemVtable = 0x00B1D450;
constexpr u32 kObjSize = 112, kItemSize = 72, kRecSize = 12;
// Level def: +536 Entry*[] with the count u16 at +544. Entry: +0 key, +4,
// +8 12-byte records {key, u32, const char*}, +12 record count.
constexpr u32 kDefEntries = 536, kDefCount = 544;
}  // namespace

// u32 Level_LoadTable2E9338(LevelDef* def) (name?): once (while the count
// at 0x010686C0 is 0), the def's +544 entries become Obj_Construct2E8DFC
// objects (112 bytes; +8 / +12 the entry's key and +4, +16 an array of the
// entry's +12 items, +24 the count). Each item is a 72-byte block (vtable
// 0x00B1D450, zeroed, +60 Holder_New550E70()) holding the record's string
// copy (+12), +4, and +16 / +20 the values the record key maps to in
// 0x010686C4 / 0x010686DC. Objects are appended to the map 0x010686A8 by
// key. Failed allocations leave null pointers that are written through.
DECOMP(002E9338, Level_LoadTable2E9338) {
    const u32 def = arg<u32>(ctx, 0);
    if (def == 0) return ret(ctx, 0u);
    if (const u32 c = load<u32>(kTableCount); c != 0) return ret(ctx, c);
    const u32 count = load<u16>(def + kDefCount);
    store<u32>(kTableCount, count);
    if (count == 0) return ret(ctx, 0u);
    Stack frame(ctx, 16);
    const u32 keyB = frame.addr(), keyA = frame.addr() + 4;  // the original's r1 +112 / +116
    u32 r3 = 0;
    for (u32 i = 0; i < load<u32>(kTableCount); ++i) {
        const u32 entry = load<u32>(load<u32>(def + kDefEntries) + 4 * i);
        const u32 p = static_cast<u32>(call(ctx, kMemAlloc, {kObjSize, 0, 16}));
        u32 obj = 0;
        if (p != 0) {
            call(ctx, kObjConstruct2E8DFC, {p});
            obj = p;
        }
        copy32(obj + 8, entry + 0);
        copy32(obj + 12, entry + 4);
        const u32 items = static_cast<u32>(call(ctx, kMemAlloc, {load<u32>(entry + 12) << 2, 0, 16}));
        const u32 n = load<u32>(entry + 12);
        store<u32>(obj + 16, items);
        store<u32>(obj + 24, n);
        if (n != 0) {
            u32 j = 0;
            for (;;) {
                const u32 q = static_cast<u32>(call(ctx, kMemAlloc, {kItemSize, 0, 16}));
                u32 item = 0;
                if (q != 0) {
                    for (u32 off = 4; off <= 24; off += 4) store<u32>(q + off, 0);
                    store<u8>(q + 28, 0);
                    store<u32>(q + 0, kItemVtable);
                    store<u32>(q + 32, 0);
                    store<u32>(q + 36, 0);
                    store<u32>(q + 40, 0);
                    store<u64>(q + 48, 0);
                    store<u32>(q + 56, 0);
                    store<u32>(q + 60, 0);
                    store<u32>(q + 64, 0);
                    const u32 h = static_cast<u32>(call(ctx, kHolderNew, {}));
                    item = q;
                    store<u32>(q + 60, h);
                }
                const u32 slot = 4 * j, roff = kRecSize * j;
                store<u32>(load<u32>(obj + 16) + slot, item);
                const u32 s = static_cast<u32>(call(ctx, kStringDup, {load<u32>(load<u32>(entry + 8) + roff + 8)}));
                store<u32>(load<u32>(load<u32>(obj + 16) + slot) + 12, s);
                const u32 recs = load<u32>(entry + 8);
                const u32 it = load<u32>(load<u32>(obj + 16) + slot);
                store<u32>(it + 4, load<u32>(recs + roff + 4));
                store<u32>(keyA, load<u32>(recs + roff));
                const u32 va = static_cast<u32>(call(ctx, kSortedMapFind, {kNameMapA, keyA}));
                const u32 itemsA = load<u32>(obj + 16);
                const u32 xa = load<u32>(va);
                store<u32>(load<u32>(itemsA + slot) + 16, xa);
                store<u32>(keyB, load<u32>(load<u32>(entry + 8) + roff));
                const u32 vb = static_cast<u32>(call(ctx, kSortedMapFind, {kNameMapB, keyB}));
                const u32 xb = load<u32>(vb);
                ++j;
                const u32 cnt = load<u32>(obj + 24);
                store<u32>(load<u32>(load<u32>(obj + 16) + slot) + 20, xb);
                if (!(j < cnt)) break;
            }
        }
        // The inlined map append, with the count stored before the value.
        const u32 m = kTableMap;
        u32 n0 = load<u16>(m + 2);
        if (n0 == load<u16>(m + 0)) {
            call(ctx, hevent22::h22fn::Array_Reserve, {m, (static_cast<u32>(load<u16>(m + 0)) << 1) & 0xFFFEu});
            n0 = load<u16>(m + 2);
        }
        const u32 data = load<u32>(m + 8);
        const u32 n1 = load<u16>(m + 2);
        store<u32>(data + (n0 << 3), load<u32>(obj + 8));
        store<u16>(m + 2, static_cast<u16>(n1 + 1));
        store<u32>(data + (n1 << 3) + 4, obj);
        const u32 hi = load<u16>(m + 4);
        const u32 fl = load<u16>(m + 6);
        store<u16>(m + 4, static_cast<u16>(static_cast<s32>(hi) > static_cast<s32>((n1 + 1) & 0xFFFF) ? hi : n1 + 1));
        r3 = fl | 1;
        store<u16>(m + 6, static_cast<u16>(r3));
    }
    ret(ctx, r3);
}

// ---------------------------------------------------------------- 0x000CC188
namespace {
constexpr u32 kQueryVtable = 0x00B12260, kSpawnQueryVtable = 0x00B12868;
constexpr u32 kDefaultRadius = 0x000CC184;     // f32 -1.0 (read from the code segment)
constexpr u32 kRecStep = 192;
}  // namespace

// u8 SpawnQuery_Construct0CC188(SpawnQuery* q, const char* name, Profile*
// prof, u32 a3, const f32 m[12 of 16], const vec3* pos, u8 flag31, u8
// flag30, u32 a8 (stack); f32 f) (name?): the base query (vtable
// 0x00B12260; +32 3x4 matrix, +96 pos, +112 a3, +116 a8, +120 prof, +124 /
// +128 prof +36 / +40 or -1.0, +132 flags bit 31 / 30 from the two bytes,
// bit 29 prof +32 bit 2, bits 21..28 set, +140 name[40]) with the
// profile's +136 192-byte records added to a PtrVector created at +16 (20
// bytes, step 192). Then vtable 0x00B12868, two PtrVectors (+180, +200;
// step 76, Init(16, 0, 1)), +220 prof +140 or f, +224 / +228 prof +148 /
// +152, +4 |= 2, +232 = (prof ? prof +188 bit 15 : 1). Returns +232.
DECOMP(000CC188, SpawnQuery_Construct0CC188) {
    const u32 q = arg<u32>(ctx, 0), name = arg<u32>(ctx, 1), prof = arg<u32>(ctx, 2), a3 = arg<u32>(ctx, 3);
    const u32 m = arg<u32>(ctx, 4), pos = arg<u32>(ctx, 5);
    const u32 flag31 = arg<u32>(ctx, 6), flag30 = arg<u32>(ctx, 7);
    const u32 a8 = load<u32>(static_cast<u32>(ctx.gpr[1]) + 116);  // 9th argument
    const f32 f = static_cast<f32>(farg(ctx, 0));
    store<u32>(q + 16, 0);
    store<u32>(q + 0, kQueryVtable);
    for (u32 row = 0; row < 3; ++row)
        for (u32 c = 0; c < 3; ++c) copy32(q + 32 + 16 * row + 4 * c, m + 16 * row + 4 * c);
    copy32(q + 80, m + 48);
    copy32(q + 84, m + 52);
    const u32 old = load<u32>(q + 132);
    copy32(q + 88, m + 56);
    copy32(q + 96, pos + 0);
    copy32(q + 100, pos + 4);
    const u32 pz = load<u32>(pos + 8);
    store<u32>(q + 112, a3);
    store<u32>(q + 104, pz);
    store<u32>(q + 116, a8);
    store<u32>(q + 120, prof);
    store<u32>(q + 124, load<u32>(prof != 0 ? prof + 36 : kDefaultRadius));
    store<u32>(q + 128, load<u32>(prof != 0 ? prof + 40 : kDefaultRadius));
    u32 flags = (old & 0x7FFFFFFFu) | ((flag31 & 1) << 31);
    flags = (flags & ~0x40000000u) | ((flag30 & 1) << 30);
    u32 bit29 = 0;
    if (prof != 0) {
        store<u32>(q + 132, flags);
        bit29 = (load<u8>(prof + 32) >> 2) & 1;
    }
    flags = (flags & ~0x20000000u) | (bit29 << 29);
    store<u32>(q + 136, 0);
    flags |= 0x1FE00000u;  // bits 21..28
    store<u32>(q + 132, flags);
    store<u32>(q + 4, 0);
    store<u32>(q + 8, 0);
    store<u32>(q + 12, 0);
    store<u8>(q + 140, 0);
    if (name != 0) call(ctx, kStringCopy, {q + 140, name, 40});
    if (prof != 0) {
        u32 recs = load<u32>(prof + 136);
        if (recs != 0 && load<u16>(recs + 8) != 0) {
            for (u32 i = 0, off = 0; i < (recs != 0 ? static_cast<u32>(load<u16>(recs + 8)) : 0u); ++i, off += kRecStep) {
                const u32 rec = load<u32>(recs + 0) + off;
                u32 vec = load<u32>(q + 16);
                if (vec == 0) {
                    vec = static_cast<u32>(call(ctx, kSmallAlloc, {20}));
                    store<u32>(q + 16, vec);
                    if (vec != 0) {
                        call(ctx, kPtrVectorConstruct, {vec});
                        store<u16>(vec + 6, kRecStep);
                        vec = load<u32>(q + 16);
                    }
                }
                call(ctx, kVectorAdd, {vec, rec});
                recs = load<u32>(prof + 136);
            }
        }
    }
    store<u32>(q + 0, kSpawnQueryVtable);
    call(ctx, kPtrVectorConstruct, {q + 180});
    store<u16>(q + 186, 76);
    call(ctx, kPtrVectorInit, {q + 180, 16, 0, 1});
    call(ctx, kPtrVectorConstruct, {q + 200});
    store<u16>(q + 206, 76);
    call(ctx, kPtrVectorInit, {q + 200, 16, 0, 1});
    const u32 w4 = load<u32>(q + 4);
    if (prof != 0) copy32(q + 220, prof + 140);
    else store<f32>(q + 220, f);
    store<u32>(q + 224, prof != 0 ? load<u32>(prof + 148) : 0u);
    store<u32>(q + 228, prof != 0 ? load<u32>(prof + 152) : 0u);
    const u32 on = prof != 0 ? (static_cast<u32>(load<u16>(prof + 188)) >> 15) & 1 : 1u;
    store<u32>(q + 4, w4 | 2);
    store<u8>(q + 232, static_cast<u8>(on != 0 ? 1 : 0));
    ret(ctx, on != 0 ? 1u : 0u);
}
