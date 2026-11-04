// Texture_Alloc and Texture_Init: making an AglTexture from a file's bytes.
#include "../sly4.h"

using namespace sly4;

namespace {

namespace xfn {
constexpr u32 Mem_Alloc = 0x0054ED38;
constexpr u32 FreeList_Init = 0x00551A58;        // (list, elem_size, count, heap, bool)
constexpr u32 Texture_Construct = 0x0053A284;    // (AglTexture*) (name?)
constexpr u32 Mem_GetDefaultStack = 0x005525FC;
constexpr u32 String_CompareNoCase2 = 0x00555D70; // strcasecmp wrapper
constexpr u32 Image_FromBmp = 0x0050A6C8;        // (data, size, Image*, path) -> 1 on success (name?)
constexpr u32 Image_FromTga = 0x0050ACAC;        // (data, size, Image*, path) -> 1 on success (name?)
constexpr u32 Texture_FromGtf = 0x0053A448;      // (tex, data, size, 0) -> result (name?)
constexpr u32 Texture_FromImage = 0x0053A34C;    // (tex, const Image*, path) (name?)
}  // namespace xfn

constexpr u32 kTextureList = 0x00F8F118;     // AglTexture free-list header: +0 allocator, +4 capacity
constexpr u32 kTextureListVtbl = 0x00B23DA0; // ApeTFreeList<AglTexture>
constexpr u32 kStrTextureList = 0x00AF11E0;  // "ApeTFreeList<T> &ApeTListAllocator<T>::GetAllocator() [with T = AglTexture]"
constexpr u32 kTextureSize = 132;
constexpr u32 kExtBmp = 0x00AF3494, kExtTga = 0x00AF349C, kExtGtf = 0x00AF34A4;  // "BMP", "TGA", "GTF"

// Image (20 bytes on the stack): +0 pixels, +12 palette (both from the
// default memory stack), +18 u16 flags (0x2: the buffers aren't owned).
constexpr u32 kImagePixels = 0, kImagePalette = 12, kImageFlags = 18;
constexpr u32 kStackVfnFree = 8;

void Image_Free(PPUContext& ctx, u32 img) {
    if ((load<u16>(img + kImageFlags) & 2) != 0) return;
    auto free = [&](u32 p) {
        const u32 st = static_cast<u32>(call(ctx, xfn::Mem_GetDefaultStack));
        const u32 opd = load<u32>(load<u32>(st) + kStackVfnFree);
        call_ptr(ctx, opd, {st, p, opd, load<u32>(opd)});  // r5 / r6 = OPD and entry
    };
    if (load<u32>(img + kImagePixels) != 0) {
        free(load<u32>(img + kImagePixels));
        store<u32>(img + kImagePixels, 0);
    }
    if (load<u32>(img + kImagePalette) != 0) free(load<u32>(img + kImagePalette));
}

}  // namespace

// AglTexture* Texture_Alloc()
// A 132-byte AglTexture from the free list at 0x00F8F118 (made on first
// use, as Material_Alloc does for materials), constructed; 0 when the
// list header is missing or the list is empty.
DECOMP(0051D368, Texture_Alloc) {
    const u32 list = load<u32>(kTextureList);
    if (list == 0) return ret(ctx, 0u);
    u32 alloc = load<u32>(list);
    if (alloc == 0) {
        const u32 obj = static_cast<u32>(call(ctx, xfn::Mem_Alloc, {60, 0, 16}));
        if (obj != 0) {
            const u32 capacity = load<u32>(list + 4);
            for (u32 off : {8u, 4u, 12u, 20u, 16u}) store<u32>(obj + off, 0);
            const u16 h54 = load<u16>(obj + 54);
            for (u32 off : {24u, 32u, 28u, 36u, 40u, 44u, 48u}) store<u32>(obj + off, 0);
            store<u16>(obj + 52, 0);
            store<u16>(obj + 54, static_cast<u16>(h54 & 0x7FFF));
            store<u32>(obj, kTextureListVtbl);
            store<u32>(obj + 56, kStrTextureList);
            // r8 / r9 = the vtable and the string here in the original; FreeList_Init reads r3..r7 only.
            call(ctx, xfn::FreeList_Init, {obj, kTextureSize, capacity, 0, 0});
        }
        alloc = obj;  // 0 if Mem_Alloc failed: the original then calls through guest address 0
        store<u32>(list, alloc);
    }
    const u32 opd = load<u32>(load<u32>(alloc) + 12);
    const u32 t = static_cast<u32>(call_ptr(ctx, opd, {alloc, opd, load<u32>(opd)}));  // r4 / r5 = OPD and entry
    if (t == 0) return ret(ctx, 0u);
    call(ctx, xfn::Texture_Construct, {t});
    ret(ctx, t);
}

// bool Texture_Init(AglTexture* t, const void* data, u32 size, const char* ext, const char* path)
// By extension: BMP and TGA are decoded into an image that is then
// uploaded (Texture_FromImage, only when decoding returned 1); GTF goes
// straight to the texture. The image buffers are freed. Other extensions
// give 0.
DECOMP(0053A640, Texture_Init) {
    const u32 t = arg<u32>(ctx, 0), data = arg<u32>(ctx, 1), size = arg<u32>(ctx, 2);
    const u32 ext = arg<u32>(ctx, 3), path = arg<u32>(ctx, 4);
    Stack frame(ctx, 192 - 112);
    const u32 img = frame.addr();  // sp+112
    store<u32>(img + 0, 0);
    store<u16>(img + 4, 0);
    store<u16>(img + 6, 0);
    store<u16>(img + 8, 0);
    store<u32>(img + 12, 0);
    store<u16>(img + 16, 0);
    store<u16>(img + 18, 0);

    u64 result;  // r3 as the decoder returned it
    bool upload = true;
    if (static_cast<u32>(call(ctx, xfn::String_CompareNoCase2, {ext, kExtBmp})) == 0) {
        result = call(ctx, xfn::Image_FromBmp, {data, size, img, path});
    } else if (static_cast<u32>(call(ctx, xfn::String_CompareNoCase2, {ext, kExtTga})) == 0) {
        result = call(ctx, xfn::Image_FromTga, {data, size, img, path});
    } else if (static_cast<u32>(call(ctx, xfn::String_CompareNoCase2, {ext, kExtGtf})) == 0) {
        result = call(ctx, xfn::Texture_FromGtf, {t, data, size, 0});
        upload = false;
    } else {
        Image_Free(ctx, img);  // nothing to free here: the image is still empty
        return ret(ctx, 0u);
    }
    if (upload && static_cast<s32>(static_cast<u32>(result)) == 1) call(ctx, xfn::Texture_FromImage, {t, img, path});
    Image_Free(ctx, img);
    ret(ctx, result);
}
