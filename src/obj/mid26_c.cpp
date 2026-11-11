// Game code, medium sweep part 26: two bank-table loaders (no direct
// callers: table entries). Each collects (key, object) pairs into a
// temporary pool array on the stack (Mem_DefaultAllocator; 32 x 8 bytes,
// type 0x00F39AD0 / 0x00F39AD8) with a filler, then appends them to an
// 8-byte-element vector of the global at 0x00F65B38.
#include "mid26.h"

using namespace sly4;
using namespace sly4::mid26;

namespace {
constexpr u32 kDefaultAllocator = 0x00550EE8u;  // Mem_DefaultAllocator()
constexpr u32 kArrayInit = 0x00558F38u;         // Array_Init(arr)
constexpr u32 kArrayReserve = 0x00558F3Cu;      // Array_Reserve(arr, u16 cap)
constexpr u32 kArrayDestruct = 0x0054EC94u;     // Array_Destruct(arr)
constexpr u32 kBankGlobal = 0x00F65B38u;        // the bank set whose vectors are filled
constexpr u32 kBankPending = 0x00F65B3Cu;

// Call through slot `slot` of o's vtable with the entry address in the
// register after the arguments and the OPD in the one after that.
u64 VCallEO26(PPUContext& ctx, u32 o, u32 slot, std::initializer_list<u64> args) {
    const u32 opd = load<u32>(load<u32>(o) + slot);
    u64 r[8] = {};
    u32 n = 0;
    for (u64 a : args) r[n++] = a;
    r[n++] = load<u32>(opd);
    r[n++] = opd;
    return call_ptr(ctx, opd, {r[0], r[1], r[2], r[3], r[4], r[5]});  // four arguments here
}

// The temporary array (24 bytes): u16 cap +0, u16 count +2, u16 +4, u16
// element size * 2 +6, data +8, allocator +12, +16, type +20.
void TempArrayInit(PPUContext& ctx, u32 a, u32 type) {
    store<u16>(a, 32);
    store<u16>(a + 2, 0);
    store<u16>(a + 4, 0);
    store<u32>(a + 8, 0);
    store<u16>(a + 6, 16);
    const u32 al = static_cast<u32>(call(ctx, kDefaultAllocator, {}));
    store<u32>(a + 12, al);
    if (const u32 cap = load<u16>(a); cap != 0) {
        const u32 sz = cap * ((static_cast<u32>(load<u16>(a + 6)) >> 1) & 0x7FFFu);
        store<u32>(a + 8, static_cast<u32>(VCallEO26(ctx, al, 0, {al, sz, 0, 16})));
    }
    call(ctx, kArrayInit, {a});
    store<u32>(a + 20, type);
    store<u32>(a + 16, 0);
}

// The data goes back to the allocator (vt[8](al, data, size)) and
// Array_Destruct(a); returns 1 (r3).
void TempArrayFree(PPUContext& ctx, u32 a) {
    const u32 sz = load<u16>(a) * ((static_cast<u32>(load<u16>(a + 6)) >> 1) & 0x7FFFu);
    if (const u32 d = load<u32>(a + 8); d != 0) {
        const u32 al = load<u32>(a + 12);
        VCall(ctx, al, 8, {al, d, sz});
    }
    call(ctx, kArrayDestruct, {a});
    ctx.gpr[3] = 1;
}

// Room for one more element of the 8-byte vector v (u16 cap +0, count +2,
// peak +4, flags +6, data +8): Array_Reserve(v, cap * 2) when full.
// Returns the count.
u32 VecRoom(PPUContext& ctx, u32 v) {
    const u32 cap = load<u16>(v);
    if (load<u16>(v + 2) == cap) call(ctx, kArrayReserve, {v, (cap << 1) & 0xFFFEu});
    return load<u16>(v + 2);
}

// The new peak: the old one when it is above (signed) the new count.
inline u32 Peak(u32 old, u32 cnt) { return static_cast<s32>(old) > static_cast<s32>(cnt & 0xFFFFu) ? old : cnt; }
}  // namespace

// u32 f(T* o): pairs from the filler 0x00441178(arr, o) are appended to the
// vector G + 8 (G = *0x00F65B38, re-read per pair); each pair's object
// (second word) gets its +4 = G. Without G the pairs are dropped; else
// *0x00F65B3C = 0 afterwards. Returns 1.
DECOMP(0044F0C0, Bank_LoadPairs_0044F0C0) {
    const u32 o = arg<u32>(ctx, 0);
    Stack frame(ctx, 32);
    const u32 a = frame.addr();
    TempArrayInit(ctx, a, 0x00F39AD0u);
    call(ctx, 0x00441178u, {a, o});
    u32 g = load<u32>(kBankGlobal);
    if (g == 0) {
        TempArrayFree(ctx, a);
        return;
    }
    const s32 n = static_cast<s32>(load<u16>(a + 2));
    for (s32 k = 0; k < n; ++k) {
        const u32 v = g + 8;
        const u32 d = load<u32>(a + 8);
        const u32 key = load<u32>(d + 8 * k);
        const u32 obj = load<u32>(d + 8 * k + 4);
        const u32 c = VecRoom(ctx, v);
        const u32 vd = load<u32>(v + 8);
        store<u32>(vd + 8 * c, key);
        const u32 old = load<u16>(v + 4);
        store<u16>(v + 2, static_cast<u16>(c + 1));
        store<u32>(vd + 8 * c + 4, obj);
        store<u16>(v + 6, static_cast<u16>(load<u16>(v + 6) | 1u));
        store<u16>(v + 4, static_cast<u16>(Peak(old, c + 1)));
        store<u32>(obj + 4, load<u32>(kBankGlobal));
        g = load<u32>(kBankGlobal);
    }
    store<u32>(kBankPending, 0);
    TempArrayFree(ctx, a);
}

// u32 f(T* o): as Bank_LoadPairs_0044F0C0 with the filler 0x004413A4 and
// the vector G + 32; the objects are not back-linked and 0x00F65B3C is
// left alone. Returns 1.
DECOMP(0044F3BC, Bank_LoadPairs_0044F3BC) {
    const u32 o = arg<u32>(ctx, 0);
    Stack frame(ctx, 32);
    const u32 a = frame.addr();
    TempArrayInit(ctx, a, 0x00F39AD8u);
    call(ctx, 0x004413A4u, {a, o});
    u32 g = load<u32>(kBankGlobal);
    if (g != 0) {
        const s32 n = static_cast<s32>(load<u16>(a + 2));
        for (s32 k = 0; k < n; ++k) {
            const u32 v = g + 32;
            const u32 d = load<u32>(a + 8);
            const u32 key = load<u32>(d + 8 * k);
            const u32 obj = load<u32>(d + 8 * k + 4);
            const u32 c = VecRoom(ctx, v);
            const u32 vd = load<u32>(v + 8);
            store<u32>(vd + 8 * c, key);
            const u32 old = load<u16>(v + 4);
            store<u32>(vd + 8 * c + 4, obj);
            store<u16>(v + 2, static_cast<u16>(c + 1));
            store<u16>(v + 4, static_cast<u16>(Peak(old, c + 1)));
            store<u16>(v + 6, static_cast<u16>(load<u16>(v + 6) | 1u));
            g = load<u32>(kBankGlobal);
        }
    }
    TempArrayFree(ctx, a);
}
