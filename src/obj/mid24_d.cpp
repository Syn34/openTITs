// Game code, medium sweep part 24: slot pools keyed through sorted maps
// (proxy points, laser groups) and a melee-result bone hash.
#include "mid24.h"

using namespace sly4;
using namespace sly4::mid24;

namespace {
constexpr u32 kStringFormat = 0x005534D0u;     // String_Format(fmt, ...)
constexpr u32 kArrayReserve = 0x00558F3Cu;     // Array_Reserve(Array*, u16 capacity)
constexpr u32 kSortedMapFind = 0x00734750u;    // SortedMap_Find734750(map, &key) -> s16* index
constexpr u32 kHashNoCase = 0x0056A710u;       // String_HashNoCase(s)

// Appends {key, s16 idx} to the sorted index map m (u16 capacity +0, count
// +2, high-water +4, flags +6 (bit 0: unsorted), 8-byte entries at +8),
// growing it with Array_Reserve(m, 2 * capacity) when full.
void MapPush(PPUContext& ctx, u32 m, u32 key, u32 idx) {
    if (load<u16>(m + 2) == load<u16>(m)) call(ctx, kArrayReserve, {m, (static_cast<u32>(load<u16>(m)) << 1) & 0xFFFEu});
    const u32 base = load<u32>(m + 8);
    store<u32>(base + ((static_cast<u32>(load<u16>(m + 2)) << 3) & 0x7FFF8u), key);
    store<u16>(base + (static_cast<u32>(load<u16>(m + 2)) << 3) + 4, static_cast<u16>(idx));
}

// The count / high-water / flags update after MapPush.
void MapCount(u32 m) {
    const u32 c1 = static_cast<u32>(load<u16>(m + 2)) + 1;
    const u32 hi = load<u16>(m + 4);
    const u32 fl = load<u16>(m + 6);
    store<u16>(m + 2, static_cast<u16>(c1));
    store<u16>(m + 4, static_cast<u16>(static_cast<s32>(hi) > static_cast<s32>(c1 & 0xFFFFu) ? hi : c1));
    store<u16>(m + 6, static_cast<u16>(fl | 1u));
}

// Takes the next slot of the pool s->4 (u16 capacity +0, used +2), clearing
// s->8[slot] when s->8 is set. Returns the slot.
u32 TakeSlot(u32 s) {
    u32 pool = load<u32>(s + 4);
    const u32 n = load<u16>(pool + 2);
    if (const u32 a = load<u32>(s + 8); a != 0) {
        store<u32>(a + ((n << 2) & 0x3FFFCu), 0);
        pool = load<u32>(s + 4);
    }
    store<u16>(pool + 2, static_cast<u16>(load<u16>(pool + 2) + 1));
    return n;
}

// The pool s->4 has a free slot (signed compare of the two u16s).
bool HasSlot(u32 s) {
    const u32 pool = load<u32>(s + 4);
    return static_cast<s32>(load<u16>(pool + 2)) < static_cast<s32>(load<u16>(pool));
}
}  // namespace

// Rec* ProxyPoint_Add64D48(Sys* s, Obj* o, u32 x): 0 for a null o; the
// 20-byte record s->12[i] of o (i from the map s+16 keyed by o->204) when
// it has one; else, with the slot pool s->4 full, String_Format("Run out of
// proxy point slot, proxy pt (%s) and any point after will not be added to
// the system", o->vfn[12](o)) and 0; otherwise a new slot i: record +8 /
// +10 / +14 / +12 = i, +0 = x, +4 = o, {o->204, i} added to the map. r3 =
// the record.
DECOMP(00064D48, ProxyPoint_Add64D48) {
    const u32 s = arg<u32>(ctx, 0), o = arg<u32>(ctx, 1), x = arg<u32>(ctx, 2);
    if (o == 0) {
        ctx.gpr[3] = 0;
        return;
    }
    Stack frame(ctx, 16);
    const u32 key = frame.addr();
    store<u32>(key, load<u32>(o + 204));
    const u32 e = static_cast<u32>(call(ctx, kSortedMapFind, {s + 16, key}));
    const s32 f = static_cast<s16>(load<u16>(e));
    if (f != -1) {
        ctx.gpr[3] = load<u32>(s + 12) + 20u * static_cast<u32>(f);
        return;
    }
    if (!HasSlot(s)) {
        const u32 nm = static_cast<u32>(VCall(ctx, o, 12, {o}));
        call(ctx, kStringFormat, {0x00AC041Cu, nm});
        ctx.gpr[3] = 0;
        return;
    }
    // (The original's not-free fallback, index -1, is unreachable here.)
    const u32 i = TakeSlot(s);
    const u32 r = load<u32>(s + 12) + 20u * static_cast<u32>(static_cast<s32>(static_cast<s16>(i)));
    const u32 k = load<u32>(o + 204);
    store<u16>(r + 8, static_cast<u16>(i));
    store<u16>(r + 10, static_cast<u16>(i));
    store<u16>(r + 14, static_cast<u16>(i));
    store<u16>(r + 12, static_cast<u16>(i));
    store<u32>(r, x);
    store<u32>(r + 4, o);
    store<u32>(key, k);  // only spilled before Array_Reserve in the original (stack)
    MapPush(ctx, s + 16, load<u32>(key), i);
    MapCount(s + 16);
    ctx.gpr[3] = r;
}

// Rec* LaserGroup_Add3A187C(Sys* s, u32 key, Obj* o): the 112-byte record
// s->12[i] for key (i from the map s+68; a 0 key never matches) when it
// has one; else, with the slot pool s->4 full, String_Format("Ran out of
// laser group slots (Max %i), laser group (%s) and any laser group added
// after will not be added to the system!\n", 32, o->vfn[12](o)) and 0;
// otherwise a new slot i, {key, i} added to the map and
// Bounds_Init_00398ED0(record) (when non-null). r3 = the record.
DECOMP(003A187C, LaserGroup_Add3A187C) {
    const u32 s = arg<u32>(ctx, 0), key = arg<u32>(ctx, 1), o = arg<u32>(ctx, 2);
    Stack frame(ctx, 16);
    const u32 kp = frame.addr();
    store<u32>(kp, key);
    u32 r = 0;
    if (key != 0) {
        const u32 e = static_cast<u32>(call(ctx, kSortedMapFind, {s + 68, kp}));
        if (const s32 f = static_cast<s16>(load<u16>(e)); f != -1) r = load<u32>(s + 12) + 112u * static_cast<u32>(f);
    }
    if (r != 0) {
        ctx.gpr[3] = r;
        return;
    }
    if (!HasSlot(s)) {
        const u32 nm = static_cast<u32>(VCall(ctx, o, 12, {o}));
        call(ctx, kStringFormat, {0x00ADDF50u, 32, nm});
        ctx.gpr[3] = 0;
        return;
    }
    // (The original's not-free fallback, index -1, is unreachable here.)
    const u32 i = TakeSlot(s);
    const u32 idx = static_cast<u32>(static_cast<s32>(static_cast<s16>(i)));
    MapPush(ctx, s + 68, key, i);
    const u32 tab = load<u32>(s + 12);
    MapCount(s + 68);
    r = tab + 112u * idx;
    if (r != 0) call(ctx, 0x00398ED0u, {r});
    ctx.gpr[3] = r;
}

// u32 Result_SetMeleeBone396D80(Cmd* c, Obj* o): slot = o + 152 + 132 *
// (s16)c->6 when that is below the u16 o->144 and the slot's first word is
// below 10 (else 0). The bone name comes from the table *0x00F65038 (+24,
// 128-byte rows): row (s16)c->4 (0 for -1) + 4 * the byte at row (s16)c->2
// (0 for -1) + its u16 at *0x0106CF7E when its byte +2 is non-zero, offset
// by the u16 *0x0106CF70. A name: (slot + u16 *0x0106CF80)->4 =
// String_HashNoCase(name), r3 = the hash. None: String_Format(
// "ResultSetMeleeAttackRaidus: No attach bone name associate with!"),
// String_Format(0x00ADD2EC, the name of entry (s16)slot->4 of the 44-byte
// table 0x00F65100 (+40 -> name, "NULL" when null), that message), u16
// slot->6 |= 0x8000, r3 = that value.
DECOMP(00396D80, Result_SetMeleeBone396D80) {
    const u32 c = arg<u32>(ctx, 0), o = arg<u32>(ctx, 1);
    const s32 i6 = static_cast<s16>(load<u16>(c + 6));
    const bool inr = i6 < static_cast<s32>(load<u16>(o + 144));
    auto slotOf = [&]() -> u32 {
        if (!inr) return 0;
        const u32 sl = o + 152 + 132u * static_cast<u32>(i6);
        return static_cast<s32>(load<u32>(sl)) < 10 ? sl : 0u;
    };
    const u32 slot = slotOf();
    const u32 dst = slot + load<u16>(0x0106CF80u) + 4;
    const u32 tb = load<u32>(0x00F65038u);
    const s32 i4 = static_cast<s16>(load<u16>(c + 4));
    const s32 i2 = static_cast<s16>(load<u16>(c + 2));
    const u32 rowA = i4 != -1 ? load<u32>(tb + 24) + (static_cast<u32>(i4) << 7) : 0u;
    const u32 rowB = i2 != -1 ? load<u32>(tb + 24) + (static_cast<u32>(i2) << 7) : 0u;
    const u32 pb = load<u8>(rowB + 2) != 0 ? rowB + load<u16>(0x0106CF7Eu) : 0u;
    const u32 ea = rowA + load<u16>(0x0106CF70u) + (static_cast<u32>(load<u8>(pb)) << 2);
    if (load<u32>(ea) != 0) {
        const u32 h = static_cast<u32>(call(ctx, kHashNoCase, {load<u32>(ea)}));
        store<u32>(dst, h);
        ctx.gpr[3] = h;
        return;
    }
    const u32 d2 = slotOf() + 4;
    const u32 msg = static_cast<u32>(call(ctx, kStringFormat, {0x00ADDC80u}));
    const u32 ent = 0x00F65100u + 44u * static_cast<u32>(static_cast<s32>(static_cast<s16>(load<u16>(d2))));
    u32 nm = load<u32>(load<u32>(ent + 40));
    if (nm == 0) nm = 0x00B1FBACu;  // "NULL"
    call(ctx, kStringFormat, {0x00ADD2ECu, nm, msg});
    const u32 v = static_cast<u32>(load<u16>(d2 + 2)) | 0x8000u;
    store<u16>(d2 + 2, static_cast<u16>(v));
    ctx.gpr[3] = v;
}
