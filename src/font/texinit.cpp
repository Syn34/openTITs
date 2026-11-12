// Texture_Alloc and Texture_Init: making an AglTexture from a file's bytes.
#include "image.h"

#include "../memory/mem.h"

namespace sly4::image {
using namespace opentit::native;

namespace {

namespace xfn {
constexpr u32 FreeList_Init = 0x00551A58;  // (list, elem_size, count, heap, bool) (src/game/load_util.cpp)
}  // namespace xfn

constexpr u32 kTextureList = 0x00F8F118;     // AglTexture free-list header: +0 allocator, +4 capacity
constexpr u32 kTextureListVtbl = 0x00B23DA0; // ApeTFreeList<AglTexture>
constexpr u32 kStrTextureList = 0x00AF11E0;  // "ApeTFreeList<T> &ApeTListAllocator<T>::GetAllocator() [with T = AglTexture]"
constexpr u32 kTextureSize = 132;
constexpr u32 kExtBmp = 0x00AF3494, kExtTga = 0x00AF349C, kExtGtf = 0x00AF34A4;  // "BMP", "TGA", "GTF"
constexpr u32 kStackVfnFree = 8;

struct ListHeader {
    be_u32 alloc;     // +0 the ApeTFreeList (made on first use)
    be_u32 capacity;  // +4
};

void Image_Free(Image* img) {
    if ((img->flags & 2) != 0) return;
    auto free = [](u32 p) { vcall(memory::Mem_GetDefaultStack(), kStackVfnFree, p); };
    if (img->pixels) {
        free(img->pixels.ea);
        img->pixels = nullptr;
    }
    if (img->palette) free(img->palette.ea);
}

}  // namespace

// A 132-byte AglTexture from the free list at 0x00F8F118 (made on first
// use, as Material_Alloc does for materials), constructed; 0 when the
// list header is missing or the list is empty.
AglTexture* Texture_Alloc() {
    ListHeader* list = host<ListHeader>(global<be_u32>(kTextureList));
    if (!list) return nullptr;
    u32 alloc = list->alloc;
    if (alloc == 0) {
        const u32 obj = guest(memory::Mem_Alloc(60, 0, 16));
        if (obj != 0) {
            const u32 capacity = list->capacity;
            be_u32* w = host<be_u32>(obj);
            for (u32 off : {8u, 4u, 12u, 20u, 16u}) w[off / 4] = 0u;
            be_u16* h = host<be_u16>(obj);
            const u16 h54 = h[27];
            for (u32 off : {24u, 32u, 28u, 36u, 40u, 44u, 48u}) w[off / 4] = 0u;
            h[26] = 0;
            h[27] = static_cast<u16>(h54 & 0x7FFF);
            w[0] = kTextureListVtbl;
            w[14] = kStrTextureList;
            call(xfn::FreeList_Init, obj, kTextureSize, capacity, 0u, 0u);
        }
        alloc = obj;  // 0 if Mem_Alloc failed: the original then calls through guest address 0
        list->alloc = alloc;
    }
    AglTexture* t = vcall<AglTexture*>(host<void>(alloc), 12);
    if (!t) return nullptr;
    Texture_Construct(t);
    return t;
}

// By extension: BMP and TGA are decoded into an image that is then
// uploaded (Texture_FromImage, only when decoding returned 1); GTF goes
// straight to the texture. The image buffers are freed. Other extensions
// give 0.
u32 Texture_Init(AglTexture* t, const u8* data, u32 size, const char* ext, const char* path) {
    GuestFrame frame(192 - 112);
    Image* img = frame.as<Image>();  // sp+112
    img->pixels = nullptr;
    img->width = 0;
    img->height = 0;
    img->bpp = 0;
    img->palette = nullptr;
    img->pal_count = 0;
    img->flags = 0;

    u32 result;  // the decoder's result
    bool upload = true;
    if (String_CompareNoCase2(ext, host<const char>(kExtBmp)) == 0) {
        result = Image_FromBmp(data, size, img, path);
    } else if (String_CompareNoCase2(ext, host<const char>(kExtTga)) == 0) {
        result = Image_FromTga(data, size, img, path);
    } else if (String_CompareNoCase2(ext, host<const char>(kExtGtf)) == 0) {
        result = Texture_FromGtf(t, const_cast<u8*>(data), size, 0);
        upload = false;
    } else {
        Image_Free(img);  // nothing to free here: the image is still empty
        return 0;
    }
    if (upload && static_cast<s32>(result) == 1) Texture_FromImage(t, img, path);
    Image_Free(img);
    return result;
}

}  // namespace sly4::image

NATIVE(0051D368, sly4::image::Texture_Alloc)
NATIVE(0053A640, sly4::image::Texture_Init)
