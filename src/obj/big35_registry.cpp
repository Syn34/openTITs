// Game code, large-function sweep part 35: the object registry's
// constructor (the singleton at 0x00F668D8; destructor Obj_Dtor_B22FC8_004F3CB4).
#include "../sly4.h"
#include "big35.h"

using namespace sly4;
using namespace sly4::big35;

namespace {
constexpr u32 kRegistryVtable = 0x00B22FC8;
constexpr u32 kRegistry = 0x00F668D8;  // u32: the live registry
// the pool arrays' element descriptors (+20 of each 24-byte array block)
constexpr u32 kPool8Desc = 0x00F3B1D0, kPool32Desc = 0x00F2E0C8, kPool56Desc = 0x00F3B1D8, kPool80Desc = 0x00F3B1E0;
constexpr u32 Mem_Alloc = 0x0054ED38;             // (size, 0, align)
constexpr u32 Mem_DefaultAllocator = 0x00550EE8;  // () -> Allocator*
constexpr u32 Array_Init = 0x00558F38;            // (Array*): empty in this build
constexpr u32 Buffer_Init565CC8 = 0x00565CC8;     // (buf, x, 0, 0) (f_00565CC8)
constexpr u32 SlotList_Init = 0x0079A1C8;         // (list, s32 n)
constexpr u32 Registry_AddFactories4F3554 = 0x004F3554;  // (Registry*, Factory* const*, n): array +8
constexpr u32 Registry_AddFactories4F32F4 = 0x004F32F4;  // (Registry*, Factory* const*, n): array +56
constexpr u32 Iff_RegisterChunk = 0x0055902C;     // (fourcc, opd, arg, name, flags)

// A pool array at a (+0 u16 count, +2 / +4 0, +6 u16 element size 16, +8
// data, +12 allocator): count from n; the data alloc->vfn[0](alloc, count *
// ((size >> 1) & 0x7FFF), 0, 16) for a non-zero count; then Array_Init.
// The caller stores the fields (and the descriptor words before them).
void PoolArray(PPUContext& ctx, u32 a) {
    const u32 alloc = static_cast<u32>(call(ctx, Mem_DefaultAllocator));
    const u32 n = load<u16>(a);
    store<u32>(a + 12, alloc);
    if (n != 0) {
        const u32 size = n * ((load<u16>(a + 6) >> 1) & 0x7FFFu);
        store<u32>(a + 8, static_cast<u32>(VCall(ctx, alloc, 0, {alloc, size, 0, 16})));
    }
    call(ctx, Array_Init, {a});
}
}  // namespace

// u64 Registry_Construct4F37B0(Registry* o, Factory* const* fa, u32 na,
// Factory* const* fb, u32 nb, x, u8 hashed, u8 b) (Call_4F37B0_004F3C88):
// +0 the vtable; +4 a 24-byte buffer (Mem_Alloc(24, 0, 16), then
// Buffer_Init565CC8(buf, x, 0, 0); 0 when the allocation fails); pool
// arrays at +8 (na entries), +32 (600), +56 (nb) and +80 (100), each after
// its descriptor (+28 / +52 / +76 / +100) and a zero word; the slot list
// +104 (1500 slots); +144 = b, +140 0; +148 a zeroed 1536-byte block
// (32 x 48) when hashed (0 without, or when the allocation fails); +152 0.
// Then *0x00F668D8 = o, the factories fa / fb are added when na / nb are
// non-zero (as 32-bit values), and the six IFF chunk handlers FTES, HTES,
// DTES, FTMG, HTMG, DTMG registered. Returns the last registration's r3.
DECOMP(004F37B0, Registry_Construct4F37B0) {
    const u32 o = arg<u32>(ctx, 0);
    // passed on as the full registers, as in the original
    const u64 fa = ctx.gpr[4], na = ctx.gpr[5], fb = ctx.gpr[6], nb = ctx.gpr[7], x = ctx.gpr[8];
    const u32 hashed = arg<u32>(ctx, 6) & 0xFF;
    const u8 b = arg<u8>(ctx, 7);
    store<u32>(o + 0, kRegistryVtable);
    u32 buf = 0;
    if (const u32 p = static_cast<u32>(call(ctx, Mem_Alloc, {24, 0, 16})); p != 0) {
        call(ctx, Buffer_Init565CC8, {p, x, 0, 0});
        buf = p;
    }
    store<u32>(o + 4, buf);
    store<u16>(o + 8, static_cast<u16>(na));
    store<u16>(o + 10, 0);
    store<u16>(o + 12, 0);
    store<u16>(o + 14, 16);
    store<u32>(o + 16, 0);
    PoolArray(ctx, o + 8);

    store<u32>(o + 28, kPool8Desc);
    store<u32>(o + 24, 0);
    store<u16>(o + 34, 0);
    store<u16>(o + 32, 600);
    store<u16>(o + 36, 0);
    store<u16>(o + 38, 16);
    store<u32>(o + 40, 0);
    PoolArray(ctx, o + 32);

    store<u32>(o + 52, kPool32Desc);
    store<u32>(o + 48, 0);
    store<u16>(o + 56, static_cast<u16>(nb));
    store<u16>(o + 58, 0);
    store<u16>(o + 60, 0);
    store<u16>(o + 62, 16);
    store<u32>(o + 64, 0);
    PoolArray(ctx, o + 56);

    store<u32>(o + 76, kPool56Desc);
    store<u32>(o + 72, 0);
    store<u16>(o + 80, 100);
    store<u16>(o + 82, 0);
    store<u16>(o + 86, 16);
    store<u16>(o + 84, 0);
    store<u32>(o + 88, 0);
    PoolArray(ctx, o + 80);
    store<u32>(o + 96, 0);
    store<u32>(o + 100, kPool80Desc);
    call(ctx, SlotList_Init, {o + 104, 1500});

    store<u8>(o + 144, b);
    store<u32>(o + 140, 0);
    u32 block = 0;
    if (hashed != 0) {
        block = static_cast<u32>(call(ctx, Mem_Alloc, {1536, 0, 16}));
        if (block != 0) {
            for (u32 p = block, k = 0; k < 32; ++k, p += 48) {
                for (u32 j = 0; j < 24; j += 4) store<u32>(p + j, 0);
                for (u32 j = 0; j < 24; j += 4) store<u32>(p + 24 + j, 0);
            }
        }
    }
    store<u32>(o + 148, block);
    store<u32>(o + 152, 0);
    store<u32>(kRegistry, o);
    if (static_cast<u32>(na) != 0) call(ctx, Registry_AddFactories4F3554, {o, fa, na});
    if (static_cast<u32>(nb) != 0) call(ctx, Registry_AddFactories4F32F4, {o, fb, nb});
    call(ctx, Iff_RegisterChunk, {0x46544553u, 0x00F3B168u, 0, 0, 0});  // 'FTES'
    call(ctx, Iff_RegisterChunk, {0x48544553u, 0x00F3B170u, 0, 0, 0});  // 'HTES'
    call(ctx, Iff_RegisterChunk, {0x44544553u, 0x00F3B178u, 0, 0, 0});  // 'DTES'
    call(ctx, Iff_RegisterChunk, {0x46544D47u, 0x00F3B180u, 0, 0, 0});  // 'FTMG'
    call(ctx, Iff_RegisterChunk, {0x48544D47u, 0x00F3B188u, 0, 0, 0});  // 'HTMG'
    ret(ctx, call(ctx, Iff_RegisterChunk, {0x44544D47u, 0x00F3B190u, 0, 0, 0}));  // 'DTMG'
}
