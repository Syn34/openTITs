// Game code, large-function sweep part 33: a sound bank loaded from a zip
// archive in memory (IFF chunks read into a 316-byte bank object), then
// either made the global bank or registered under its key.
#include "big33.h"

namespace sly4::obj {

// An allocator: vt slot 0 allocates (allocator, size, 0, align), slot 8
// frees (allocator, p, size).
struct BankAllocator;
// One pool array of the bank (24 bytes at +8 and +32).
struct BankPool {
    be_u16 cap;               // +0
    be_u16 h2, h4;            // +2, +4
    be_u16 elem2;             // +6: element size * 2
    be_u32 data;              // +8
    gp<BankAllocator> alloc;  // +12
    be_u32 w16;               // +16
    be_u32 vtable;            // +20
};
static_assert(sizeof(BankPool) == 24);
struct SoundBank {
    be_u32 w0, w4;     // +0 (non-zero once loaded), +4
    BankPool pool8;    // +8
    BankPool pool32;   // +32
    u8 loaded;         // +56
    char name[256];    // +57
    u8 name_end;       // +313
    pad<2> _314;
};
static_assert(offsetof(SoundBank, pool32) == 32 && offsetof(SoundBank, name) == 57 && sizeof(SoundBank) == 316);
// The {key, bank} array of registered banks.
struct BankEntry {
    be_u32 key;            // +0
    gp<SoundBank> bank;    // +4
};
struct BankMap {
    be_u16 cap;             // +0
    be_u16 count;           // +2
    be_u16 mark;            // +4
    be_u16 flags;           // +6
    gp<BankEntry> data;     // +8
};

namespace {
constexpr u32 kLoadKey = 0x010AC8E0;     // u32: the key of the bank being loaded
constexpr u32 kBankMap = 0x010AFEB0;     // BankMap
constexpr u32 kGlobalKey = 0x010B00D8;   // u32: the global bank's key
constexpr u32 kLoadName = 0x00F65B34;    // const char*: the name of the bank being loaded
constexpr u32 kLoadBank = 0x00F65B38;    // SoundBank*: the bank being loaded (the IFF chunk handlers fill it)
constexpr u32 kGlobalBank = 0x00F65B60;  // SoundBank*: the global bank
constexpr u32 kVtPoolA = 0x00F39AD0;     // pool vtables
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

u32 ElemSize(const BankPool& p) { return (p.elem2 >> 1) & 0x7FFF; }

// The pool's storage from the allocator's slot 0 (allocator, n * size, 0, 16).
void PoolAlloc(BankPool& p, BankAllocator* alloc, u32 n) {
    const u32 sz = n * ElemSize(p);
    p.data = call_ptr<u32>(vfn(alloc, 0), alloc, sz, 0u, 16u);
}

// The pool freed: its data through the allocator's slot 8 (allocator, data,
// capacity * size) when set, then Array_Destruct.
void PoolFree(BankPool& p) {
    const u32 n = p.cap * ElemSize(p);
    const u32 d = p.data;
    if (d != 0) {
        BankAllocator* alloc = p.alloc;
        vcall(alloc, 8, d, n);
    }
    call(Array_Destruct, &p);
}

void ClearLoad() {
    global<be_u32>(kLoadKey) = 0u;
    global<be_u32>(kLoadBank) = 0u;
    global<be_u32>(kLoadName) = 0u;
}
}  // namespace

// bool f(u32 key, const char* name, const void* data, u32 size, bool global)
// (called by f_00448C10 and Call_446EE4_0044BD4C). Nothing is loaded (0)
// when key is already the global bank's or is in kBankMap. Otherwise a bank
// (Mem_Alloc(316, 0, 16); a failed allocation is still written at 0 + 56
// .. quirk) gets two pool arrays (+8: capacity 64, +32: capacity 8, 8-byte
// elements), loaded = 1 and the name (String_CopyN 256, +313 = 0); the IFF
// file of Archive_OpenZip(data, size, name) is read into it and the archive
// released (vt slot 8). A bank whose +0 is still 0 is destroyed (0);
// otherwise it becomes kGlobalBank (global) or is appended to kBankMap
// under key, and 1 returns. kLoadKey / kLoadName / kLoadBank are always
// cleared at the end.
bool SoundBank_Load_00446EE4(u32 key, const char* name, const void* data, u32 size, bool make_global) {
    be_u32& load_key = global<be_u32>(kLoadKey);
    be_u32& load_bank = global<be_u32>(kLoadBank);
    BankMap& map = global<BankMap>(kBankMap);
    load_key = key;
    const u32 gb = global<be_u32>(kGlobalBank);
    load_bank = 0u;
    global<be_u32>(kLoadName) = guest(name);
    u32 have;
    if (gb != 0 && key == global<be_u32>(kGlobalKey)) have = gb;
    else have = gm<be_u32>(call<u32>(SortedMap_Find783E80, &map, &load_key));
    if (have != 0) {
        ClearLoad();
        return false;
    }

    SoundBank* o = call<SoundBank*>(fn::Mem_Alloc, 316u, 0u, 16u);
    u32 bank = 0;
    if (o != nullptr) {
        o->w0 = 0u;
        o->w4 = 0u;
        o->pool8.cap = 64;
        o->pool8.h2 = 0;
        o->pool8.h4 = 0;
        o->pool8.elem2 = 16;
        o->pool8.data = 0u;
        BankAllocator* alloc = call<BankAllocator*>(Mem_DefaultAllocator);
        u32 cap = o->pool8.cap;
        o->pool8.alloc = alloc;
        if (cap != 0) PoolAlloc(o->pool8, alloc, cap);
        call(Array_Init, &o->pool8);
        o->pool8.vtable = kVtPoolA;
        o->pool32.elem2 = 16;
        o->pool8.w16 = 0u;
        o->pool32.cap = 8;
        o->pool32.h2 = 0;
        o->pool32.h4 = 0;
        o->pool32.data = 0u;
        alloc = call<BankAllocator*>(Mem_DefaultAllocator);
        cap = o->pool32.cap;
        o->pool32.alloc = alloc;
        if (cap != 0) PoolAlloc(o->pool32, alloc, cap);
        call(Array_Init, &o->pool32);
        o->pool32.vtable = kVtPoolB;
        o->pool32.w16 = 0u;
        o->loaded = 0;
        o->name[0] = 0;
        bank = guest(o);
    }
    SoundBank& B = gm<SoundBank>(bank);
    B.loaded = 1;
    load_bank = bank;
    call(b33fn::String_CopyN, B.name, global<be_u32>(kLoadName).get(), 256u);
    B.name_end = 0;
    const u32 zip = call<u32>(Archive_OpenZip, data, size, name);
    if (zip != 0) {
        GuestFrame frame(64);  // sp+112: the IFF reader
        u8* rd = frame.as<u8>();
        call(Bank_AllocHeader, rd);
        call(Iff_ReadFile, rd, zip, 0u);
        vcall(host<u8>(zip), 8);
        call(Mem_FreeAndClear, rd);
    }

    const u32 b = load_bank;
    if (gm<SoundBank>(b).w0 != 0) {  // (read even when b is 0)
        if (make_global) {
            global<be_u32>(kGlobalBank) = b;
        } else {
            const u32 cap = map.cap;
            u32 cnt = map.count;
            if (cnt == cap) {
                call(Array_Reserve, &map, (cap << 1) & 0xFFFEu);
                cnt = map.count;
            }
            const u32 arr = map.data.ea;
            const u32 n = map.count;
            gm<be_u32>(arr + ((cnt << 3) & 0x7FFF8u)) = load_key.get();
            const u32 mx = map.mark;
            gm<BankEntry>(arr + n * 8).bank.ea = b;
            const u32 fl = map.flags;
            map.count = static_cast<u16>(n + 1);
            map.mark = static_cast<u16>(static_cast<s32>(mx) > static_cast<s32>((n + 1) & 0xFFFF) ? mx : n + 1);
            map.flags = static_cast<u16>(fl | 1);
        }
        ClearLoad();
        return true;
    }
    if (b != 0) {
        SoundBank& f = gm<SoundBank>(b);
        PoolFree(f.pool32);
        PoolFree(f.pool8);
        call(Mem_FreeFrom, b, 0u);
    }
    ClearLoad();
    return false;
}

}  // namespace sly4::obj

NATIVE(00446EE4, sly4::obj::SoundBank_Load_00446EE4)
