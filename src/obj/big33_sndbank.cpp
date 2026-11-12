// Game code, large-function sweep part 33: a sound bank loaded from a zip
// archive in memory (IFF chunks read into a 316-byte bank object), then
// either made the global bank or registered under its key.
#include "../sly4.h"
#include "big33.h"

using namespace sly4;
using namespace sly4::big33;

namespace {
constexpr u32 kLoadKey = 0x010AC8E0;     // u32: the key of the bank being loaded
constexpr u32 kBankMap = 0x010AFEB0;     // {key, bank} array: u16 capacity +0, count +2, high-water +4, flags +6, data +8
constexpr u32 kGlobalKey = 0x010B00D8;   // u32: the global bank's key
constexpr u32 kLoadName = 0x00F65B34;    // const char*: the name of the bank being loaded
constexpr u32 kLoadBank = 0x00F65B38;    // Bank*: the bank being loaded (the IFF chunk handlers fill it)
constexpr u32 kGlobalBank = 0x00F65B60;  // Bank*: the global bank
constexpr u32 kVtPoolA = 0x00F39AD0;     // member vtables stored at bank +28 / +52
constexpr u32 kVtPoolB = 0x00F39AD8;

constexpr u32 Array_Destruct = 0x0054EC94;        // (array)
constexpr u32 Mem_FreeFrom = 0x0054EE6C;          // (p, pool)
constexpr u32 Mem_DefaultAllocator = 0x00550EE8;  // () -> allocator
constexpr u32 Array_Init = 0x00558F38;            // (array)
constexpr u32 Array_Reserve = 0x00558F3C;         // (array, u32 n)
constexpr u32 Archive_OpenZip = 0x00567034;       // (data, size, name) -> archive or 0
constexpr u32 Bank_AllocHeader = 0x00569280;      // (reader)
constexpr u32 Mem_FreeAndClear = 0x005692E8;      // (reader)
constexpr u32 Iff_ReadFile = 0x005695C4;          // (reader, archive, 0)
constexpr u32 SortedMap_Find783E80 = 0x00783E80;  // (map, const u32* key) -> u32*

// One pool array of the bank at o + a (u16 capacity +0, u16 element size * 2
// at +6, data +8, allocator +12) is given its storage: the allocator's slot 0
// (allocator, n * size, 0, 16) with the entry / OPD in r7 / r8 (as compiled).
void PoolAlloc(PPUContext& ctx, u32 o, u32 a, u32 alloc, u32 n) {
    const u32 sz = n * ((load<u16>(o + a + 6) >> 1) & 0x7FFF);
    const u32 opd = load<u32>(load<u32>(alloc));
    store<u32>(o + a + 8, static_cast<u32>(call_ptr(ctx, opd, {alloc, sz, 0u, 16u, load<u32>(opd), opd})));
}

// The pool array at o + a freed: its data through the allocator's slot 8
// (allocator, data, capacity * size) when set, then Array_Destruct.
void PoolFree(PPUContext& ctx, u32 o, u32 a) {
    const u32 n = load<u16>(o + a) * ((load<u16>(o + a + 6) >> 1) & 0x7FFF);
    const u32 d = load<u32>(o + a + 8);
    if (d != 0) {
        const u32 alloc = load<u32>(o + a + 12);
        VCall(ctx, alloc, 8, {alloc, d, n});
    }
    call(ctx, Array_Destruct, {o + a});
}

void ClearLoad() {
    store<u32>(kLoadKey, 0);
    store<u32>(kLoadBank, 0);
    store<u32>(kLoadName, 0);
}
}  // namespace

// bool f(u32 key, const char* name, const void* data, u32 size, bool global)
// (called by f_00448C10 and Call_446EE4_0044BD4C). Nothing is loaded (0)
// when key is already the global bank's or is in kBankMap. Otherwise a bank
// (Mem_Alloc(316, 0, 16); a failed allocation is still written at 0 + 56
// .. quirk) gets two pool arrays (+8: capacity 64, +32: capacity 8, 8-byte
// elements), +56 = 1 and the name at +57 (String_CopyN 256, +313 = 0); the
// IFF file of Archive_OpenZip(data, size, name) is read into it and the
// archive released (vt slot 8). A bank whose +0 is still 0 is destroyed
// (0); otherwise it becomes kGlobalBank (global) or is appended to kBankMap
// under key, and 1 returns. kLoadKey / kLoadName / kLoadBank are always
// cleared at the end.
DECOMP(00446EE4, SoundBank_Load_00446EE4) {
    const u32 key = arg<u32>(ctx, 0), name = arg<u32>(ctx, 1), data = arg<u32>(ctx, 2), size = arg<u32>(ctx, 3);
    const u8 global = arg<u8>(ctx, 4);
    store<u32>(kLoadKey, key);
    const u32 gb = load<u32>(kGlobalBank);
    store<u32>(kLoadBank, 0);
    store<u32>(kLoadName, name);
    u32 have;
    if (gb != 0 && key == load<u32>(kGlobalKey)) have = gb;
    else have = load<u32>(static_cast<u32>(call(ctx, SortedMap_Find783E80, {kBankMap, kLoadKey})));
    if (have != 0) {
        ClearLoad();
        return ret(ctx, 0u);
    }

    const u32 o = static_cast<u32>(call(ctx, fn::Mem_Alloc, {316u, 0u, 16u}));
    u32 bank = 0;
    if (o != 0) {
        store<u32>(o + 0, 0);
        store<u32>(o + 4, 0);
        store<u16>(o + 8, 64);
        store<u16>(o + 10, 0);
        store<u16>(o + 12, 0);
        store<u16>(o + 14, 16);
        store<u32>(o + 16, 0);
        u32 alloc = static_cast<u32>(call(ctx, Mem_DefaultAllocator));
        u32 cap = load<u16>(o + 8);
        store<u32>(o + 20, alloc);
        if (cap != 0) PoolAlloc(ctx, o, 8, alloc, cap);
        call(ctx, Array_Init, {o + 8});
        store<u32>(o + 28, kVtPoolA);
        store<u16>(o + 38, 16);
        store<u32>(o + 24, 0);
        store<u16>(o + 32, 8);
        store<u16>(o + 34, 0);
        store<u16>(o + 36, 0);
        store<u32>(o + 40, 0);
        alloc = static_cast<u32>(call(ctx, Mem_DefaultAllocator));
        cap = load<u16>(o + 32);
        store<u32>(o + 44, alloc);
        if (cap != 0) PoolAlloc(ctx, o, 32, alloc, cap);
        call(ctx, Array_Init, {o + 32});
        store<u32>(o + 52, kVtPoolB);
        store<u32>(o + 48, 0);
        store<u8>(o + 56, 0);
        store<u8>(o + 57, 0);
        bank = o;
    }
    store<u8>(bank + 56, 1);
    store<u32>(kLoadBank, bank);
    call(ctx, b33fn::String_CopyN, {bank + 57, load<u32>(kLoadName), 256u});
    store<u8>(bank + 313, 0);
    const u32 zip = static_cast<u32>(call(ctx, Archive_OpenZip, {data, size, name}));
    if (zip != 0) {
        Stack frame(ctx, 64);  // sp+112: the IFF reader
        const u32 rd = frame.addr();
        call(ctx, Bank_AllocHeader, {rd});
        call(ctx, Iff_ReadFile, {rd, zip, 0u});
        VCall(ctx, zip, 8, {zip});
        call(ctx, Mem_FreeAndClear, {rd});
    }

    const u32 b = load<u32>(kLoadBank);
    if (load<u32>(b) != 0) {  // (read even when b is 0)
        if (global != 0) {
            store<u32>(kGlobalBank, b);
        } else {
            const u32 cap = load<u16>(kBankMap);
            u32 cnt = load<u16>(kBankMap + 2);
            if (cnt == cap) {
                call(ctx, Array_Reserve, {kBankMap, (cap << 1) & 0xFFFEu});
                cnt = load<u16>(kBankMap + 2);
            }
            const u32 arr = load<u32>(kBankMap + 8);
            const u32 n = load<u16>(kBankMap + 2);
            store<u32>(arr + ((cnt << 3) & 0x7FFF8u), load<u32>(kLoadKey));
            const u32 mx = load<u16>(kBankMap + 4);
            store<u32>(arr + n * 8 + 4, b);
            const u32 fl = load<u16>(kBankMap + 6);
            store<u16>(kBankMap + 2, static_cast<u16>(n + 1));
            store<u16>(kBankMap + 4, static_cast<u16>(static_cast<s32>(mx) > static_cast<s32>((n + 1) & 0xFFFF) ? mx : n + 1));
            store<u16>(kBankMap + 6, static_cast<u16>(fl | 1));
        }
        ClearLoad();
        return ret(ctx, 1u);
    }
    if (b != 0) {
        PoolFree(ctx, b, 32);
        PoolFree(ctx, b, 8);
        call(ctx, Mem_FreeFrom, {b, 0u});
    }
    ClearLoad();
    ret(ctx, 0u);
}
