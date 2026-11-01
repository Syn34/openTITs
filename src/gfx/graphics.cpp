// Graphics start-up.
#include "gfx.h"

#include "../platform/platform.h"

using namespace sly4;
namespace gx = sly4::gfx;
namespace pf = sly4::platform;

namespace {

// Aspect ratios stored as f32 just before Graphics_Init's code.
constexpr u32 kAspect4x3 = 0x0050F518;   // 1.3333334f
constexpr u32 kAspect16x9 = 0x0050F51C;  // 1.7777778f
constexpr u32 kDefaultUnk44 = 20;

// Small pool descriptors: { u32 list (0); u32 count; u32 ? (0) } and similar.
constexpr u32 kPoolOnce = 0x00F670D9;    // u8: kPoolA is set up only once
constexpr u32 kPoolA = 0x00F8F110;       // 16 bytes { 0, count (config +354, 1024), 0 }
constexpr u32 kPoolB = 0x00F8F120;       // 16 bytes { 0, count (config +352, 1024), 0 }
constexpr u32 kPoolC = 0x010DDC84;       // 8 bytes  { 0, count (config +398, 128) }
constexpr u32 kPoolD = 0x00F8F11C;       // 16 bytes { 0, 4, 0 }
constexpr u32 kDeferredPool = 0x010DDC88;  // { PoolDesc* (8 bytes { FreeList*, 1024 }); u32 = 1; u8 +8 = 1 }
constexpr u32 kDefaultPoolA = 1024, kDefaultPoolB = 1024, kDefaultPoolC = 128, kPoolDCount = 4;
constexpr u32 kDeferredCount = 1024;  // the original reuses its 1024 default here

// ApeTListAllocator<AglDeferredDestructionObj> free list.
constexpr u32 kFreeListSize = 60;
constexpr u32 kDeferredFreeListVtable = 0x00B23EA0;
constexpr u32 kStrDeferredAllocator = 0x00AF14B5;  // "void ApeTListAllocator<T>::CreateAllocator() [with T = AglDeferredDestructionObj]"
constexpr u32 kDeferredObjSize = 12;

constexpr u32 kGfxUnk010C5A08 = 0x010C5A08;  // two u32s cleared at the end

// Chunk handler OPDs.
constexpr u32 kIffSmsh = 0x00F3B5C0, kIffMvtx = 0x00F3B5D0, kIffMidx = 0x00F3B5D8, kIffVtxd = 0x00F3B5E0,
              kIffMpal = 0x00F3B5F0, kIffVinf = 0x00F3B5E8, kIffMhdr = 0x00F3B5C8;

// A u16 count from g::config read as s16: 0 -> default, negative -> 0.
u32 config_count(u32 offset, u32 fallback) {
    const u16 raw = field<u16>(g::config, offset);
    if (raw == 0) return fallback;
    const s16 v = static_cast<s16>(raw);
    return v > 0 ? static_cast<u32>(v) : 0;
}

u32 alloc16(PPUContext& ctx, u32 size) { return static_cast<u32>(call(ctx, sly4::fn::Mem_Alloc, {size, 0, 16})); }

void register_chunk(PPUContext& ctx, u32 code, u32 handler) {
    call(ctx, pf::fn::Iff_RegisterChunk, {code, handler, 0, 0, 0});
}

}  // namespace

// bool Graphics_Init()
// Starts libgcm, sets the aspect ratio, creates the Agl pools and shaders,
// and registers the mesh chunk types. Fails if libgcm or the shaders fail.
DECOMP(0050F520, Graphics_Init) {
    if (static_cast<s32>(call(ctx, gx::fn::Gcm_Init)) == 0) return ret(ctx, 0u);

    // lfs/stfs in the original: copied as bits.
    const u32 aspect = load<u32>(field<u8>(g::config, gx::kConfigWidescreen) != 0 ? kAspect16x9 : kAspect4x3);
    const u32 unk44 = field<u32>(g::config, gx::kConfigUnk44);
    field<u32>(g::config, gx::kConfigAspect) = aspect;
    field<u32>(g::config, gx::kConfigUnk44) = unk44 != 0 ? unk44 : kDefaultUnk44;

    if (ref<u8>(kPoolOnce) == 0) {
        const u32 count = config_count(gx::kConfigPoolA, kDefaultPoolA);
        ref<u8>(kPoolOnce) = 1;
        u32 pool = 0;
        const u32 p = alloc16(ctx, 16);
        if (p != 0) {
            store<u32>(p, 0);
            store<u32>(p + 8, 0);
            store<u32>(p + 4, count);
            pool = p;
        }
        store<u32>(kPoolA, pool);
    }

    call(ctx, gx::fn::Agl_Init542704);
    if (static_cast<s32>(call(ctx, gx::fn::Shaders_Init)) == 0) return ret(ctx, 0u);
    call(ctx, gx::fn::Agl_Init548BC0);

    {
        const u32 count = config_count(gx::kConfigPoolB, kDefaultPoolB);
        u32 pool = 0;
        const u32 p = alloc16(ctx, 16);
        if (p != 0) {
            store<u32>(p, 0);
            store<u32>(p + 8, 0);
            store<u32>(p + 4, count);
            pool = p;
        }
        store<u32>(kPoolB, pool);
    }

    using pf::fourcc;
    register_chunk(ctx, fourcc("SMSH"), kIffSmsh);
    register_chunk(ctx, fourcc("MVTX"), kIffMvtx);
    register_chunk(ctx, fourcc("MIDX"), kIffMidx);
    register_chunk(ctx, fourcc("VTXD"), kIffVtxd);
    register_chunk(ctx, fourcc("MPAL"), kIffMpal);
    register_chunk(ctx, fourcc("VINF"), kIffVinf);
    register_chunk(ctx, fourcc("MHDR"), kIffMhdr);
    call(ctx, gx::fn::Agl_Init50EF04);
    call(ctx, gx::fn::Agl_Init50ECD4);

    {
        const u32 count = config_count(gx::kConfigPoolC, kDefaultPoolC);
        u32 pool = 0;
        const u32 p = alloc16(ctx, 8);
        if (p != 0) {
            store<u32>(p, 0);
            store<u32>(p + 4, count);
            pool = p;
        }
        store<u32>(kPoolC, pool);
    }
    {
        u32 pool = 0;
        const u32 p = alloc16(ctx, 16);
        if (p != 0) {
            store<u32>(p, 0);
            store<u32>(p + 4, kPoolDCount);
            store<u32>(p + 8, 0);
            pool = p;
        }
        store<u32>(kPoolD, pool);
    }
    {
        u32 desc = 0;
        const u32 p = alloc16(ctx, 8);
        if (p != 0) {
            store<u32>(p, 0);
            store<u32>(p + 4, kDeferredCount);
            desc = p;
        }
        store<u32>(kDeferredPool, desc);
        const bool empty = load<u32>(desc) == 0;  // unchecked: reads address 0 if the allocation failed
        ref<u8>(kDeferredPool + 8) = 1;
        if (empty) {
            u32 list = 0;
            const u32 f = alloc16(ctx, kFreeListSize);
            if (f != 0) {
                const u32 count = load<u32>(desc + 4);
                for (u32 off = 4; off <= 48; off += 4) field<u32>(f, off) = 0;
                field<u16>(f, 52) = 0;
                field<u16>(f, 54) &= static_cast<u16>(~0x8000);
                field<u32>(f, 0) = kDeferredFreeListVtable;
                field<u32>(f, 56) = kStrDeferredAllocator;
                call(ctx, pf::fn::FreeList_Init, {f, kDeferredObjSize, count, 0, 0});
                list = f;
            }
            store<u32>(desc, list);
        }
        store<u32>(kDeferredPool + 4, 1);
    }

    call(ctx, gx::fn::Agl_Init50E614);
    call(ctx, gx::fn::Agl_Init50E410);
    call(ctx, gx::fn::Agl_Init54BF24);
    call(ctx, gx::fn::Agl_Init50D2E0);
    call(ctx, gx::fn::Agl_Init548974);
    field<u8>(g::config, gx::kConfigGraphicsUp) = 1;
    store<u32>(kGfxUnk010C5A08, 0);
    store<u32>(kGfxUnk010C5A08 + 4, 0);
    ret(ctx, 1u);
}
