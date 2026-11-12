// Game code, large-function sweep part 35: the object registry's
// constructor (the singleton at 0x00F668D8; destructor Obj_Dtor_B22FC8_004F3CB4).
#include "native/objtypes.h"

namespace sly4::obj {

// A pool array (24 bytes): +0 u16 count, +6 u16 element size (16), +8
// data, +12 allocator, +20 the element descriptor.
struct PoolArray24 {
    be_u16 count;   // +0
    be_u16 h2;      // +2
    be_u16 h4;      // +4
    be_u16 elem;    // +6
    be_u32 data;    // +8
    be_u32 alloc;   // +12
    be_u32 w16;     // +16
    be_u32 desc;    // +20
};
static_assert(sizeof(PoolArray24) == 24);
struct ObjRegistry {
    be_u32 vtable;       // +0
    be_u32 buf;          // +4: 24-byte buffer
    PoolArray24 a8;      // +8
    PoolArray24 a32;     // +32
    PoolArray24 a56;     // +56
    PoolArray24 a80;     // +80
    pad<36> slots;       // +104: the slot list
    be_u32 w140;         // +140
    u8 b144;             // +144
    pad<3> _145;
    be_u32 block;        // +148: 32 x 48 bytes when hashed
    be_u32 w152;         // +152
};
static_assert(offsetof(ObjRegistry, a80) == 80 && offsetof(ObjRegistry, w140) == 140 && offsetof(ObjRegistry, block) == 148);

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

// A pool array: the default allocator stored at +12; for a non-zero
// count the data = alloc->vfn[0](alloc, count * ((size >> 1) & 0x7FFF), 0,
// 16); then Array_Init. The caller stores the fields (and the descriptor
// words before them).
void PoolArray(PoolArray24* a) {
    const u32 alloc = call<u32>(Mem_DefaultAllocator);
    const u32 n = a->count;
    a->alloc = alloc;
    if (n != 0) {
        const u32 size = n * ((a->elem >> 1) & 0x7FFFu);
        a->data = vcall<u32>(host<u8>(alloc), 0, size, 0u, 16u);
    }
    call(Array_Init, a);
}
}  // namespace

// void Registry_Construct4F37B0(Registry* o, Factory* const* fa, u32 na,
// Factory* const* fb, u32 nb, x, u8 hashed, u8 b) (Call_4F37B0_004F3C88):
// +0 the vtable; +4 a 24-byte buffer (Mem_Alloc(24, 0, 16), then
// Buffer_Init565CC8(buf, x, 0, 0); 0 when the allocation fails); pool
// arrays at +8 (na entries), +32 (600), +56 (nb) and +80 (100), each after
// its descriptor (+28 / +52 / +76 / +100) and a zero word; the slot list
// +104 (1500 slots); +144 = b, +140 0; +148 a zeroed 1536-byte block
// (32 x 48) when hashed (0 without, or when the allocation fails); +152 0.
// Then *0x00F668D8 = o, the factories fa / fb are added when na / nb are
// non-zero, and the six IFF chunk handlers FTES, HTES, DTES, FTMG, HTMG,
// DTMG registered.
void Registry_Construct4F37B0(ObjRegistry* o, u32 fa, u32 na, u32 fb, u32 nb, u32 x, u8 hashed, u8 b) {
    o->vtable = kRegistryVtable;
    u32 buf = 0;
    if (const u32 p = call<u32>(Mem_Alloc, 24u, 0u, 16u); p != 0) {
        call(Buffer_Init565CC8, p, x, 0u, 0u);
        buf = p;
    }
    o->buf = buf;
    o->a8.count = static_cast<u16>(na);
    o->a8.h2 = 0;
    o->a8.h4 = 0;
    o->a8.elem = 16;
    o->a8.data = 0u;
    PoolArray(&o->a8);

    o->a8.desc = kPool8Desc;
    o->a8.w16 = 0u;
    o->a32.h2 = 0;
    o->a32.count = 600;
    o->a32.h4 = 0;
    o->a32.elem = 16;
    o->a32.data = 0u;
    PoolArray(&o->a32);

    o->a32.desc = kPool32Desc;
    o->a32.w16 = 0u;
    o->a56.count = static_cast<u16>(nb);
    o->a56.h2 = 0;
    o->a56.h4 = 0;
    o->a56.elem = 16;
    o->a56.data = 0u;
    PoolArray(&o->a56);

    o->a56.desc = kPool56Desc;
    o->a56.w16 = 0u;
    o->a80.count = 100;
    o->a80.h2 = 0;
    o->a80.elem = 16;
    o->a80.h4 = 0;
    o->a80.data = 0u;
    PoolArray(&o->a80);
    o->a80.w16 = 0u;
    o->a80.desc = kPool80Desc;
    call(SlotList_Init, &o->slots, 1500);

    o->b144 = b;
    o->w140 = 0u;
    u32 block = 0;
    if (hashed != 0) {
        block = call<u32>(Mem_Alloc, 1536u, 0u, 16u);
        if (block != 0) {
            be_u32* w = host<be_u32>(block);
            for (u32 k = 0; k < 32 * 12; ++k) w[k] = 0u;
        }
    }
    o->block = block;
    o->w152 = 0u;
    global<be_u32>(kRegistry) = guest(o);
    if (na != 0) call(Registry_AddFactories4F3554, o, fa, na);
    if (nb != 0) call(Registry_AddFactories4F32F4, o, fb, nb);
    call(Iff_RegisterChunk, 0x46544553u, 0x00F3B168u, 0u, 0u, 0u);  // 'FTES'
    call(Iff_RegisterChunk, 0x48544553u, 0x00F3B170u, 0u, 0u, 0u);  // 'HTES'
    call(Iff_RegisterChunk, 0x44544553u, 0x00F3B178u, 0u, 0u, 0u);  // 'DTES'
    call(Iff_RegisterChunk, 0x46544D47u, 0x00F3B180u, 0u, 0u, 0u);  // 'FTMG'
    call(Iff_RegisterChunk, 0x48544D47u, 0x00F3B188u, 0u, 0u, 0u);  // 'HTMG'
    call(Iff_RegisterChunk, 0x44544D47u, 0x00F3B190u, 0u, 0u, 0u);  // 'DTMG'
}

}  // namespace sly4::obj

NATIVE(004F37B0, sly4::obj::Registry_Construct4F37B0)
