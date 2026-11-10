// Game code, medium sweep part 6: QuerySpawn_BaseCtor_0020C868, the base
// constructor of the 432-byte volume objects the query-spawn methods make
// (vtable 0x00B19558; ~T is TDtorVolumeShape in obj20_tmpl.cpp).
#include "mid6.h"

using namespace sly4;
using namespace sly4::mid6;

namespace {

namespace qfn {
constexpr u32 View_BaseInit = 0x000D3B94;          // (o, q) (Thunk_000D3B94)
constexpr u32 PtrVector_Construct = 0x005516E0;    // (v)
constexpr u32 PtrVector_Init = 0x00551598;         // (v, n, 0, 1)
constexpr u32 Mem_DefaultAllocator = 0x00550EE8;   // () -> allocator
constexpr u32 Array_Init = 0x00558F38;             // (a)
constexpr u32 Shape_Reset63778C = 0x0063778C;      // (shape)
constexpr u32 Volume_Notify112D8 = 0x000112D8;     // (listener, shape)
constexpr u32 Volume_Activate20C2BC = 0x0020C2BC;  // (o) (name?)
constexpr u32 Volume_Prepare20C670 = 0x0020C670;   // (o) (name?)
constexpr u32 Hk_AddRef7D48C4 = 0x007D48C4;        // (hkReferencedObject*)
constexpr u32 Hk_SetQualityLayer = 0x009315F0;     // (world, entity, 0) (name?)
constexpr u32 Hk_RemoveListener = 0x00928828;      // (entity, listener) (name?)
constexpr u32 Hk_AddListener = 0x009288D8;         // (entity, listener) (name?)
constexpr u32 Hk_AddProperty = 0x0093FE00;         // (entity, key, u64 value)
constexpr u32 Hk_RemoveProperty = 0x0093FED8;      // (hkpPropertyValue* out, entity, key)
}  // namespace qfn

constexpr u32 kVtVolume = 0x00B19558;
constexpr u32 kVtListener = 0x00B19AF0;       // the 12-byte entity listener: +0 vtable, +4 u16 size, +6 u16 refs, +8 second vtable
constexpr u32 kVtListener2 = 0x00B19B10;
constexpr u32 kArrayTag = 0x00F348A0;         // stored at +384 / +408
constexpr u32 kVolumeListener = 0x00F557EC;   // the global passed to Volume_Notify112D8
constexpr u32 kHalf = 0x0020C864;             // f32 0.5 just before the function
constexpr u32 kLayerByQuality = 0x00AD364C;   // u8[]: 0x11..0x16
constexpr s32 kTlsRouter = -28332;            // r13 +: hkMemoryRouter* (+44 the allocator, vt +8 alloc(a, size))
constexpr u32 kPropQuality = 0x08A94D07;
constexpr u32 kPropA734 = 0xA734004D;
constexpr u32 kPropA734Value = 0x7A30D7E3;
constexpr u32 kPropOwner = 0x91C7CF5B;        // value: the object

// A {+0 u16 count, +2, +4, +6 u16 elem size 16, +8 data, +12 allocator}
// array's data, allocated when count != 0.
void ArrayAlloc(PPUContext& ctx, u32 arr, u32 a) {
    const u32 n = load<u16>(arr);
    if (n != 0) store<u32>(arr + 8, static_cast<u32>(VCall(ctx, a, 0, {a, n * ((static_cast<u32>(load<u16>(arr + 6)) >> 1) & 0x7FFF), 0, 16})));
}

}  // namespace

// void QuerySpawn_BaseCtor_0020C868(Volume* o, Query* q, u32 n, bool props)
// The base init, the vtable, a PtrVector at +344 (n) and two arrays at +364
// and +388 (n each), the flags word +412 from q's bytes +148..+153, +416 the
// Havok entity q->+144 and the shape at +424. When q->+150 differs from the
// top flag bit it is copied there and the volume is (de)activated. With
// props, the entity's quality-type property (0x08A94D07, below 6) picks its
// collision layer. Then a 12-byte entity listener (Havok thread allocator)
// replaces the entity's listeners, the two properties are set (owner = o)
// and, when the entity is in a world, +144 = the centre of its AABB.
DECOMP(0020C868, QuerySpawn_BaseCtor_0020C868) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 q = arg<u32>(ctx, 1);
    const u64 n = ctx.gpr[5];
    const u32 props = arg<u32>(ctx, 3);
    Stack frame(ctx, 48);  // +0 hkAabb {min, max}, +32 / +40 the removed properties' values
    const u32 aabb = frame.addr();

    call(ctx, qfn::View_BaseInit, {o, q});
    store<u32>(o, kVtVolume);
    call(ctx, qfn::PtrVector_Construct, {o + 344});
    store<u16>(o + 350, 4);
    call(ctx, qfn::PtrVector_Init, {o + 344, n, 0, 1});

    store<u16>(o + 364, static_cast<u16>(n));
    store<u16>(o + 366, 0);
    store<u16>(o + 368, 0);
    store<u16>(o + 370, 16);
    store<u32>(o + 372, 0);
    const u32 a1 = static_cast<u32>(call(ctx, qfn::Mem_DefaultAllocator, {}));
    store<u32>(o + 376, a1);
    ArrayAlloc(ctx, o + 364, a1);
    call(ctx, qfn::Array_Init, {o + 364});

    store<u16>(o + 388, static_cast<u16>(n));
    store<u32>(o + 384, kArrayTag);
    store<u16>(o + 394, 16);
    store<u32>(o + 380, 0);
    store<u16>(o + 390, 0);
    store<u16>(o + 392, 0);
    store<u32>(o + 396, 0);
    const u32 a2 = static_cast<u32>(call(ctx, qfn::Mem_DefaultAllocator, {}));
    store<u32>(o + 400, a2);
    ArrayAlloc(ctx, o + 388, a2);
    call(ctx, qfn::Array_Init, {o + 388});

    // Flags +412: bit 31 set, bits 30..26 = q's bytes +151 / +152 / +153 /
    // +148 / +149 (bit 0 each), bit 25 clear; stored after each step.
    u32 f = load<u32>(o + 412) | 0x80000000u;
    store<u32>(o + 412, f);
    f = (f & ~0x40000000u) | ((load<u8>(q + 151) & 1u) << 30);
    store<u32>(o + 412, f);
    f = (f & ~0x20000000u) | ((load<u8>(q + 152) & 1u) << 29);
    store<u32>(o + 404, 0);
    store<u32>(o + 408, kArrayTag);
    store<u32>(o + 412, f);
    f = (f & ~0x10000000u) | ((load<u8>(q + 153) & 1u) << 28);
    store<u32>(o + 412, f);
    f = (f & ~0x08000000u) | ((load<u8>(q + 148) & 1u) << 27);
    store<u32>(o + 412, f);
    f = ((f & ~0x04000000u) | ((load<u8>(q + 149) & 1u) << 26)) & ~0x02000000u;
    store<u32>(o + 412, f);
    const u32 ent0 = load<u32>(q + 144);
    store<u32>(o + 420, 0);
    store<u32>(o + 416, ent0);
    call(ctx, qfn::Shape_Reset63778C, {o + 424});
    store<u32>(o + 208, 0);

    f = load<u32>(o + 412);
    const u32 active = load<u8>(q + 150);
    if ((f >> 31) != active) {  // cmpw of the bit against the whole byte
        store<u32>(o + 412, (f & 0x7FFFFFFFu) | ((active & 1u) << 31));
        if (active == 0) call(ctx, qfn::Volume_Notify112D8, {load<u32>(kVolumeListener), o + 424});
        else call(ctx, qfn::Volume_Activate20C2BC, {o});
        if (active == 0) {
            if ((load<u32>(o + 412) & 0x02000000u) == 0) call(ctx, qfn::Volume_Prepare20C670, {o});
            VCall(ctx, o, 96, {o, 0});
        }
    }
    if ((load<u32>(o + 412) & 0x04000000u) == 0) call(ctx, qfn::Hk_AddRef7D48C4, {load<u32>(o + 416)});

    if ((props & 0xFF) != 0) {
        const u32 ent = load<u32>(o + 416);
        const s32 i = HkPropFind(ent, kPropQuality);
        // ld of the u64 value, compared as its low word (0 when not found)
        const u32 v = i < 0 ? 0 : load<u32>(load<u32>(ent + 124) + 16 * static_cast<u32>(i) + 12);
        if (v < 6) {
            const u32 world = load<u32>(ent + 8);
            store<u32>(ent + 44, load<u8>(kLayerByQuality + (v & 15)));
            if (world != 0) call(ctx, qfn::Hk_SetQualityLayer, {world, ent, 0});
        }
    }

    const u32 al = load<u32>(load<u32>(static_cast<u32>(ctx.gpr[13]) + static_cast<u32>(kTlsRouter)) + 44);
    const u32 lis = static_cast<u32>(VCall(ctx, al, 8, {al, 12}));
    store<u16>(lis + 4, 12);  // quirk: written before the null check
    if (lis != 0) {
        store<u16>(lis + 6, 1);
        store<u32>(lis, kVtListener);
        store<u32>(lis + 8, kVtListener2);
    }
    store<u32>(o + 420, lis);

    u32 ent;
    for (;;) {  // remove every listener in the entity's +136 array
        ent = load<u32>(o + 416);
        if (static_cast<s32>(load<u32>(ent + 140)) <= 0) break;
        call(ctx, qfn::Hk_RemoveListener, {ent, load<u32>(load<u32>(ent + 136))});
    }
    const u32 l = load<u32>(o + 420);
    call(ctx, qfn::Hk_AddListener, {ent, l != 0 ? l + 8 : 0});

    ent = load<u32>(o + 416);
    if (static_cast<s32>(load<u32>(ent + 128)) > 0 && HkPropFind(ent, kPropA734) >= 0) {
        call(ctx, qfn::Hk_RemoveProperty, {aabb + 32, ent, kPropA734});
        ent = load<u32>(o + 416);
    }
    call(ctx, qfn::Hk_AddProperty, {ent, kPropA734, kPropA734Value});
    ent = load<u32>(o + 416);
    if (static_cast<s32>(load<u32>(ent + 128)) > 0 && HkPropFind(ent, kPropOwner) >= 0) {
        call(ctx, qfn::Hk_RemoveProperty, {aabb + 40, ent, kPropOwner});
        ent = load<u32>(o + 416);
    }
    call(ctx, qfn::Hk_AddProperty, {ent, kPropOwner, o});

    ent = load<u32>(o + 416);
    if (load<u32>(ent + 8) != 0) {
        VCall(ctx, ent, 32, {ent, aabb});  // getAabb
        const f32 h = F32(load<u32>(kHalf));
        for (u32 k = 0; k < 3; ++k) {
            const f32 s = F32(load<u32>(aabb + 4 * k)) + F32(load<u32>(aabb + 16 + 4 * k));
            store<u32>(o + 144 + 4 * k, W32(s * h));
        }
    }
    call(ctx, qfn::Volume_Activate20C2BC, {o});
}
