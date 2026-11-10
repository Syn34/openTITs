// Game code, large-function sweep part 4: the component factory
// Comp_Create2635E8 (called by Obj_Attach0D135C and 000D3578 / 000D9648 /
// 000D99D8 / 000D9B30 / 000D9C88). 46 component kinds: most allocate and
// call their constructor; twelve have the constructor inlined, each with
// the entity registry insertion.
#include "big4.h"

using namespace sly4;
using namespace sly4::big4;

namespace b4cc {
constexpr u32 SmallAlloc = 0x0055238Cu;
constexpr u32 Mem_Alloc = 0x0054ED38u;
constexpr u32 Array_Reserve = 0x00558F3Cu;                // (Array*, u16 capacity)
constexpr u32 StateMachine_ConstructThunk = 0x0045CBE8u;  // (sm, a, b, c)
constexpr u32 kMemset = 0x00726E54u;
constexpr u32 CopyMat43 = 0x00748BB8u;                    // (dst, src): 4x3 floats
constexpr u32 SortedMap_Find = 0x0075E8ECu;               // (map, const u32* key) -> value slot
constexpr u32 MapGet_003482B0 = 0x003482B0u, MapGet_00348650 = 0x00348650u;
constexpr u32 Thunk_00017F64 = 0x00017F64u;
// The entity registry: u16 +0 capacity, +2 count, +4 high-water mark, +6
// flags, +8 {u32 key, Component*} entries.
constexpr u32 kEntities = 0x01056F1Cu;
constexpr u32 kMap14 = 0x0105AE30u;  // the sorted map kind 14 looks its +20 up in
// Vtables.
constexpr u32 kVtBase = 0x00B1BE30u;   // Component (the inlined base constructor)
constexpr u32 kVtBase2 = 0x00B1C118u;  // the base of kinds 5..12 (+24 colour, +32 48 bytes)
constexpr u32 kVt13 = 0x00B1D2C8u;
constexpr u32 kVt39 = 0x00B1CBB8u, kVt40 = 0x00B1CC80u, kVt25 = 0x00B1C930u, kVt14 = 0x00B1C628u;
constexpr u32 kVt5 = 0x00B1C168u, kVt6 = 0x00B1C1B8u, kVt7 = 0x00B1C208u, kVt8 = 0x00B1C258u, kVt8Sm = 0x00B1C2A8u;
constexpr u32 kVt9 = 0x00B1C2D8u, kVt10 = 0x00B1C328u, kVt10Sm = 0x00B1C378u, kVt11 = 0x00B1C3A8u;
constexpr u32 kVt12 = 0x00B1C3F8u, kVt12Sm = 0x00B1C444u;

// The kinds made by allocate + constructor(obj, tmpl).
struct Simple {
    s32 kind;
    u32 size, ctor;
    bool mem_alloc;  // Mem_Alloc(size, 0, 16) instead of SmallAlloc(size)
};
constexpr Simple kSimple[] = {
    {1, 400, 0x002607C0, false},  {2, 452, 0x00260464, false},  {3, 188, 0x0025FE94, false},
    {4, 128, 0x0025FACC, false},  {15, 144, 0x0025F150, false}, {16, 36, 0x0025EBF8, false},
    {17, 104, 0x0025E960, false}, {18, 56, 0x0025E7E4, false},  {19, 1312, 0x0025DE10, false},
    {20, 656, 0x0025D370, false}, {21, 2948, 0x0025CC9C, false}, {22, 272, 0x0025C090, false},
    {23, 368, 0x0025B3E8, false}, {24, 2352, 0x0025AAE0, false}, {26, 2912, 0x00258E38, false},
    {27, 144, 0x002587C0, false}, {28, 72, 0x0025843C, false},  {29, 192, 0x00258008, false},
    {30, 80, 0x00257A40, false},  {31, 96, 0x00256FD0, true},   {32, 352, 0x00256B60, false},
    {33, 144, 0x00256684, false}, {34, 272, 0x00255CA8, false}, {35, 192, 0x00255AD8, true},
    {36, 400, 0x00254CA0, false}, {37, 720, 0x00253390, false}, {38, 56, 0x00252FF0, false},
    {41, 44, 0x002562C8, false},  {42, 144, 0x002560F0, false}, {43, 144, 0x00255F18, false},
    {44, 384, 0x00252980, false}, {45, 80, 0x00252258, false},  {46, 68, 0x00251F64, true},
    {47, 96, 0x002506A8, false},  {48, 456, 0x00063E24, false},
};
}  // namespace b4cc

namespace {
using namespace b4cc;

u32 Alloc(PPUContext& ctx, u32 size) { return static_cast<u32>(call(ctx, SmallAlloc, {size})); }

// The inlined Component constructor: +0 vtable, +4 / +8 / +12 the
// template's id / owner / kind, +16 (u8) 1, +20 0. Returns the id read.
u32 CompBase(u32 c, u32 tmpl) {
    store<u32>(c + 0, kVtBase);
    const u32 id = load<u32>(tmpl + 4);
    store<u32>(c + 4, id);
    store<u32>(c + 8, load<u32>(tmpl + 8));
    store<u32>(c + 12, load<u32>(tmpl + 12));
    store<u8>(c + 16, 1);
    store<u32>(c + 20, 0);
    return id;
}

// The entity registry insertion: {id, c} appended (the array doubled when
// full, the id then read again from c), the count, mark and flag bit 0
// updated.
void CompRegister(PPUContext& ctx, u32 c, u32 id) {
    const u32 r = kEntities;
    const u32 cap = load<u16>(r + 0);
    u32 n = load<u16>(r + 2);
    if (n == cap) {
        call(ctx, Array_Reserve, {r, (cap << 1) & 0xFFFEu});
        id = load<u32>(c + 4);
        n = load<u16>(r + 2);
    }
    const u32 data = load<u32>(r + 8);
    store<u32>(data + ((n << 3) & 0x0007FFF8u), id);
    store<u32>(data + (static_cast<u32>(load<u16>(r + 2)) << 3) + 4, c);
    const u32 n1 = load<u16>(r + 2) + 1;
    const u32 hw = load<u16>(r + 4);
    const u32 fl = load<u16>(r + 6);
    store<u16>(r + 2, static_cast<u16>(n1));
    store<u16>(r + 4, static_cast<u16>(static_cast<s32>(hw) > static_cast<s32>(n1 & 0xFFFFu) ? hw : n1));
    store<u16>(r + 6, static_cast<u16>(fl | 1u));
}

// Base + registry for the inlined kinds.
void CompInit(PPUContext& ctx, u32 c, u32 tmpl) { CompRegister(ctx, c, CompBase(c, tmpl)); }

// The base of kinds 5..12: vtable 0x00B1C118, +24..+27 0, +32 48 bytes 0.
void CompBase2(PPUContext& ctx, u32 c) {
    store<u32>(c + 0, kVtBase2);
    for (u32 k = 0; k < 4; ++k) store<u8>(c + 24 + k, 0);
    call(ctx, kMemset, {c + 32, 0, 48});
}

// The inlined kinds; returns the component (0 when the allocation fails).
u32 CreateInline(PPUContext& ctx, s32 kind, u32 tmpl) {
    static constexpr struct {
        s32 kind;
        u32 size;
    } kSizes[] = {{5, 96}, {6, 96}, {7, 80}, {8, 112}, {9, 96}, {10, 112}, {11, 96}, {12, 112}, {14, 40}, {25, 28}, {39, 32}, {40, 64}};
    u32 size = 0;
    for (const auto& k : kSizes)
        if (k.kind == kind) size = k.size;
    const u32 c = Alloc(ctx, size);
    if (c == 0) return 0;
    CompInit(ctx, c, tmpl);
    switch (kind) {
    case 39:
        store<u32>(c + 0, kVt39);
        CopyWords(c + 24, tmpl + 20, 2);
        ctx.fpr[1] = std::bit_cast<f64>(LfsBits(load<u32>(tmpl + 24)));  // f1 as the lfs / stfs copy leaves it
        break;
    case 40:
        store<u32>(c + 0, kVt40);
        CopyW(c + 24, tmpl + 20);
        CopyWords(c + 32, tmpl + 32, 3);
        CopyWords(c + 48, tmpl + 48, 3);
        store<u8>(c + 60, 0);
        ctx.fpr[1] = std::bit_cast<f64>(LfsBits(load<u32>(tmpl + 56)));  // f1 as the copy leaves it
        break;
    case 25:
        store<u32>(c + 0, kVt25);
        store<u32>(c + 24, load<u32>(tmpl + 20));
        break;
    case 14: {
        store<u32>(c + 0, kVt14);
        Stack frame(ctx, 16);
        store<u32>(frame.addr(), load<u32>(tmpl + 20));
        store<u32>(c + 24, load<u32>(static_cast<u32>(call(ctx, SortedMap_Find, {kMap14, frame.addr()}))));
        store<u32>(c + 28, load<u32>(tmpl + 32));
        store<u32>(c + 32, static_cast<u32>(call(ctx, MapGet_003482B0, {load<u32>(tmpl + 24)})));
        store<u32>(c + 36, static_cast<u32>(call(ctx, MapGet_00348650, {load<u32>(tmpl + 28)})));
        break;
    }
    default:  // 5..12
        CompBase2(ctx, c);
        switch (kind) {
        case 7: store<u32>(c + 0, kVt7); break;
        case 10:
            call(ctx, StateMachine_ConstructThunk, {c + 80, 2, 32, 0});
            store<u32>(c + 0, kVt10);
            store<u32>(c + 80, kVt10Sm);
            break;
        case 12:
            call(ctx, StateMachine_ConstructThunk, {c + 80, 0, 0, 0});
            store<u32>(c + 0, kVt12);
            store<u32>(c + 80, kVt12Sm);
            break;
        case 8:
            call(ctx, StateMachine_ConstructThunk, {c + 80, 2, 48, 0});
            store<u32>(c + 0, kVt8);
            store<u32>(c + 80, kVt8Sm);
            break;
        case 11:
        case 9:
            store<u32>(c + 0, kind == 11 ? kVt11 : kVt9);
            store<u32>(c + 80, load<u32>(tmpl + 20));
            break;
        case 6:
            store<u32>(c + 0, kVt6);
            store<u32>(c + 84, 0);
            store<u32>(c + 80, load<u32>(tmpl + 20));
            break;
        case 5:
            store<u32>(c + 0, kVt5);
            store<u8>(c + 88, 0);
            store<u32>(c + 80, load<u32>(tmpl + 20));
            for (u32 k = 0; k < 4; ++k) store<u8>(c + 84 + k, 0);
            break;
        }
        break;
    }
    return c;
}

}  // namespace

// Component* Comp_Create2635E8(Object* owner, Component* tmpl, u8): sets
// tmpl->+8 = owner, then makes the component of kind tmpl->+12 (0..48;
// 0 for any other) from the template. Kind 0 first copies the 4x3 matrix
// owner->+96 into tmpl->+32 and sets tmpl->+96 = !(bit 0 of the
// doubleword owner->+184) (even when the allocation then fails). Kinds 5..
// 12, 14, 25, 39 and 40 have the Component base constructor and the entity
// registry insertion (0x01056F1C) inlined; kind 13 sets its final vtable
// after Thunk_00017F64. Returns the component, or 0 (allocation failure or
// unknown kind).
DECOMP(002635E8, Comp_Create2635E8) {
    const u32 owner = arg<u32>(ctx, 0), tmpl = arg<u32>(ctx, 1);
    const s32 kind = load<s32>(tmpl + 12);
    store<u32>(tmpl + 8, owner);
    u32 c = 0;
    if (kind == 0) {
        call(ctx, CopyMat43, {tmpl + 32, owner + 96});
        store<u32>(tmpl + 96, (load<u32>(owner + 188) & 1u) == 0 ? 1u : 0u);
        c = Alloc(ctx, 720);
        if (c != 0) call(ctx, 0x00261FE0u, {c, tmpl});
    } else if (kind == 13) {
        c = Alloc(ctx, 32);
        if (c != 0) {
            call(ctx, Thunk_00017F64, {c, tmpl});
            store<u32>(c + 0, kVt13);
        }
    } else if ((kind >= 5 && kind <= 12) || kind == 14 || kind == 25 || kind == 39 || kind == 40) {
        c = CreateInline(ctx, kind, tmpl);
    } else {
        for (const Simple& s : kSimple) {
            if (s.kind != kind) continue;
            c = s.mem_alloc ? static_cast<u32>(call(ctx, Mem_Alloc, {s.size, 0, 16})) : Alloc(ctx, s.size);
            if (c != 0) call(ctx, s.ctor, {c, tmpl});
            break;
        }
    }
    ret(ctx, c);
}
