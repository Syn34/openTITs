// Game code, medium sweep part 25: the one-shot "activate" handlers that
// post a world event record (Rec_Set_00470560 + an inlined record of the
// world's 28-byte event queue), start the 0x2508CAD1 handle and collect
// the active helper objects (keys 0xC0E4A7C9 / 0xA9A7866C of the 0x0106C228
// map, and the global 0x00F64CBC) into the object's vector.
#include "mid25.h"

using namespace sly4;
using namespace sly4::mid25;

namespace {
constexpr u32 kRecSet = 0x00470560u;          // Rec_Set_00470560(r, ..., w = r7, b = r8; f1, f2, f3)
constexpr u32 kHandleCall = 0x0036735Cu;      // Handle_Call363A3C (hash, r4, r5, -, r7, r8, r9; f1)
constexpr u32 kSortedMapFind3 = 0x0076611Cu;  // SortedMap_Find3(map, &key) -> slot
constexpr u32 kPtrVectorReserve = 0x00551718u;
constexpr u32 kHelperMap = 0x0106C228u;       // {key, Object*} map of named helpers
constexpr u32 kGlobalHelper = 0x00F64CBCu;    // Object* (a third helper)
constexpr u32 kEvRecord = 0x001283A8u;        // world + : the Rec the event is set into
constexpr u32 kEvCount = 0x001283A4u;         // world + : u32 count of the queue below
constexpr u32 kEvQueue = 0x00128334u;         // world + : 28-byte event records
constexpr u32 kHandleHash = 0x2508CAD1u;

// Rec_Set_00470560(world + 0x1283A8, r4, r5, r6, (s32)*0x010B9BF4, 0;
// f1 = *k0, f2 = *0x010B9BA8, f3 = *0x010B9C40) with the leftover r4-r6 the
// caller passes (not read by the callee).
void SetRecord(PPUContext& ctx, u32 r4, u32 r5, u32 r6, u32 k0) {
    const u32 w = load<u32>(kWorld25);
    const u64 r7 = static_cast<u64>(static_cast<s64>(static_cast<s32>(load<u32>(0x010B9BF4u))));
    callf(ctx, kRecSet, {w + kEvRecord, r4, r5, r6, r7, 0}, {Lf(k0), Lf(0x010B9BA8u), Lf(0x010B9C40u)});
}

// Appends the 28-byte record E to the world's event queue (count + 1
// first): kind nibble 2 (bits 0x0F800000 cleared), +2..+5 = the bytes at
// 0x01164CA0, +6..+9 = the bytes at 0x010B9B5C, +12 = 0, +16 = 1.0,
// +20 = the word at 0x010B9C40, +24 = 0. Returns the record's index.
u32 PushEvent() {
    const u32 w = load<u32>(kWorld25);
    const u32 idx = load<u32>(w + kEvCount);
    const u32 e = w + 28 * idx + kEvQueue;
    store<u32>(w + kEvCount, idx + 1);
    store<u32>(e, (load<u32>(e) & 0x007FFFFFu) | 0x20000000u);
    for (u32 k = 0; k < 4; ++k) store<u8>(e + 2 + k, load<u8>(0x01164CA0u + k));
    for (u32 k = 0; k < 4; ++k) store<u8>(e + 6 + k, load<u8>(0x010B9B5Cu + k));
    store<u32>(e + 12, 0);
    store<u32>(e + 16, 0x3F800000u);
    store<u32>(e + 20, load<u32>(0x010B9C40u));
    store<u32>(e + 24, 0);
    return idx;
}

// Handle_Call363A3C(0x2508CAD1, 0, 1, r6 (leftover), 0, 0, 0x01164CA4; f1 = *k1).
void StartHandle(PPUContext& ctx, u32 r6, u32 k1) {
    callf(ctx, kHandleCall, {kHandleHash, 0, 1, r6, 0, 0, 0x01164CA4u}, {Lf(k1)});
}

// x->vt[20](x, 0), then x is appended to the u16-counted vector at v
// (data +0, capacity +4, count +8, high-water +10; grown to twice the
// capacity by PtrVector_Reserve when full).
void PushHelper(PPUContext& ctx, u32 v, u32 x) {
    VCall(ctx, x, 20, {x, 0});
    const u32 cap = load<u16>(v + 4);
    if (load<u16>(v + 8) == cap) call(ctx, kPtrVectorReserve, {v, (cap << 1) & 0x1FFFEu});
    store<u32>(load<u32>(v) + ((load<u16>(v + 8) << 2) & 0x3FFFCu), x);
    const u32 n = load<u16>(v + 8) + 1;
    const u32 hi = load<u16>(v + 10);
    store<u16>(v + 8, static_cast<u16>(n));
    store<u16>(v + 10, static_cast<u16>(static_cast<s32>(hi) > static_cast<s32>(n & 0xFFFF) ? hi : n));
}

// The helper x is active: x->+24 set and bit 0x8000 of its +164. quirk:
// the second form tests the bit also for a null x->+24 (reading 0 + 164).
bool Active(u32 x, bool quirk) {
    const u32 c = load<u32>(x + 24);
    if (c == 0 && !quirk) return false;
    return (load<u32>(c + 164) & 0x8000u) != 0;
}

// The helper named key in the 0x0106C228 map (the key in a stack slot).
u32 FindHelper(PPUContext& ctx, u32 slot, u32 key) {
    store<u32>(slot, key);
    return load<u32>(static_cast<u32>(call(ctx, kSortedMapFind3, {kHelperMap, slot})));
}

// The shared body of 0x003DE790 / 0x003E5970: once (byte flag f): the
// event record, the handle, the helpers collected into the vector v.
void Activate(PPUContext& ctx, u32 o, u32 f, u32 v, u32 idxoff, u32 rec_r5, u32 k0, bool flag2581) {
    if (const u32 b = load<u8>(o + f); b != 0) {
        ctx.gpr[3] = b;
        return;
    }
    const u32 w = load<u32>(kWorld25);
    SetRecord(ctx, w + kEvRecord, rec_r5, w, k0);
    const u32 idx = PushEvent();
    store<u8>(o + idxoff, static_cast<u8>(idx));
    u32 r6 = 0x25080000u;  // lis r6 / ori r3: the hash's high half
    if (flag2581) {
        if (const u32 g = load<u32>(load<u32>(0x00F64C20u) + 80); g != 0) store<u8>(g + 2581, 1);
        r6 = load<u32>(kWorld25) + 28 * idx + kEvQueue + 16;  // the record's +16 address
    }
    StartHandle(ctx, r6, k0 + 4);
    Stack frame(ctx, 16);
    const u32 a = FindHelper(ctx, frame.addr() + 4, 0xC0E4A7C9u);
    if (a != 0 && Active(a, false)) PushHelper(ctx, o + v, a);
    const u32 b = FindHelper(ctx, frame.addr(), 0xA9A7866Cu);
    if (b != 0 && Active(b, false)) PushHelper(ctx, o + v, b);
    const u32 c = load<u32>(kGlobalHelper);
    if (c != 0 && Active(c, false)) PushHelper(ctx, o + v, c);
    store<u8>(o + f, 1);
    ctx.gpr[3] = 1;
}
}  // namespace

// u32 f(T* o) (called by 0x003E1020): unless the byte +117 is set (r3 =
// it): the world event record (f1 = *0x003DE788), the event index in the
// byte +252, Handle_Call363A3C(0x2508CAD1, 0, 1, ..; f1 = *0x003DE78C), the
// active helpers appended to the vector o + 120; +117 = 1, r3 = 1.
DECOMP(003DE790, Obj_Activate_003DE790) {
    Activate(ctx, arg<u32>(ctx, 0), 117, 120, 252, 0x003E0000u, 0x003DE788u, false);
}

// u32 f(T* o) (called by 0x003E7D78): as 0x003DE790 with the byte +2325,
// the index byte +2368, the vector o + 2340, the constants 0x003E5968 /
// 0x003E596C, and the byte +2581 of *(*0x00F64C20 + 80) set (when that
// is set) before the handle starts.
DECOMP(003E5970, Obj_Activate_003E5970) {
    Activate(ctx, arg<u32>(ctx, 0), 2325, 2340, 2368, kEvRecord, 0x003E5968u, true);
}

// u32 f(T* o) (called by 0x003F4198): unless the byte +181 is set: the
// words 0x01085D50 / +4 cleared, the world event record (f1 = *0x003F22F0),
// the index byte +260, the handle (f1 = *0x003F22F4), the helpers
// 0xA9A7866C and 0x00F64CBC appended to the vector o + 232 when active (the
// bit tested also for a null +24: quirk), +181 = 1. Then always: the helper
// 0xC0E4A7C9 gets vt[20](x, 0) when found. r3 = that call's, else 0.
DECOMP(003F22F8, Obj_Activate_003F22F8) {
    const u32 o = arg<u32>(ctx, 0);
    Stack frame(ctx, 16);
    if (load<u8>(o + 181) == 0) {
        store<u32>(0x01085D50u, 0);
        store<u32>(0x01085D54u, 0);
        SetRecord(ctx, 0x010B9BACu, load<u32>(kWorld25), 0x003F0000u, 0x003F22F0u);
        const u32 idx = PushEvent();
        store<u8>(o + 260, static_cast<u8>(idx));
        StartHandle(ctx, idx, 0x003F22F4u);
        const u32 b = FindHelper(ctx, frame.addr() + 4, 0xA9A7866Cu);
        if (b != 0 && Active(b, true)) PushHelper(ctx, o + 232, b);
        const u32 c = load<u32>(kGlobalHelper);
        if (c != 0 && Active(c, true)) PushHelper(ctx, o + 232, c);
        store<u8>(o + 181, 1);
    }
    const u32 a = FindHelper(ctx, frame.addr(), 0xC0E4A7C9u);
    if (a == 0) {
        ctx.gpr[3] = 0;
        return;
    }
    VCall(ctx, a, 20, {a, 0});
}
