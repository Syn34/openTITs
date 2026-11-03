// Mounting and unmounting zip archives in the archive list.
#include "fsio.h"

using namespace sly4;
namespace io = sly4::fsio;

namespace {

constexpr u32 kZipArchiveSize = 1188;
constexpr u32 kZipBufferSize = 0x4000;

// The archive list (PtrVector): data +0, capacity +4, count +8, high-water +10.
constexpr u32 kArchiveList = 0x01164E14;

// Zip fields used by Archive_Unmount.
constexpr u32 kZipFile = 4, kZipScratch = 8, kZipStream = 120, kZipBuckets = 416, kZipNoClose = 1184;
constexpr u32 kZipBucketCount = 128, kEntryNext = 260;
constexpr u32 kStreamBuffer = 0, kStreamZ = 4, kStreamInflate = 68;

u32 new_zip(PPUContext& ctx) {
    const u32 zip = static_cast<u32>(call(ctx, sly4::fn::Mem_New, {kZipArchiveSize}));
    if (zip != 0) {
        field<u32>(zip, 4) = 0;
        field<u32>(zip, 8) = 0;
        field<u32>(zip, 12) = kZipBufferSize;
    }
    return zip;
}

// Appends to the archive list; returns the new count (the 1-based index).
u32 push_archive(PPUContext& ctx, u32 zip) {
    if (load<u16>(kArchiveList + 8) == load<u16>(kArchiveList + 4))
        call(ctx, io::fn::PtrVector_Reserve, {kArchiveList, (static_cast<u32>(load<u16>(kArchiveList + 4)) << 1) & 0x1FFFE});
    const u16 index = load<u16>(kArchiveList + 8);
    store<u32>(load<u32>(kArchiveList) + ((static_cast<u32>(index) << 2) & 0x3FFFC), zip);
    const u32 count = static_cast<u32>(load<u16>(kArchiveList + 8)) + 1;
    const u16 high = load<u16>(kArchiveList + 10);
    store<u16>(kArchiveList + 8, static_cast<u16>(count));
    store<u16>(kArchiveList + 10, static_cast<s32>(high) > static_cast<s32>(count & 0xFFFF) ? high : static_cast<u16>(count));
    return count & 0xFFFF;
}

}  // namespace

// u16 Archive_MountMemory(const void* data, const char* prefix)
// Mounts a zip image already in memory. Returns its 1-based index, or 0.
// (Callers pass a third argument, 0x4000, which is unused.)
DECOMP(00567F2C, Archive_MountMemory) {
    const u32 data = arg<u32>(ctx, 0), prefix = arg<u32>(ctx, 1);
    const u32 zip = new_zip(ctx);
    if (static_cast<s32>(call(ctx, io::fn::ZipArchive_OpenMemory, {zip, data, prefix})) != 1) {
        call(ctx, io::fn::Mem_FreePool, {zip, 0, 0});
        return ret(ctx, 0u);
    }
    ret(ctx, push_archive(ctx, zip));
}

// u16 Archive_MountFile(const char* path, const char* prefix)
// Same for a .zip file.
DECOMP(0055D684, Archive_MountFile) {
    const u32 path = arg<u32>(ctx, 0), prefix = arg<u32>(ctx, 1);
    const u32 zip = new_zip(ctx);
    if (static_cast<s32>(call(ctx, io::fn::ZipArchive_Open, {zip, path, prefix})) != 1) {
        call(ctx, io::fn::Mem_FreePool, {zip, 0, 0});
        return ret(ctx, 0u);
    }
    ret(ctx, push_archive(ctx, zip));
}

// bool Archive_Unmount(u16 index)
// Frees the archive mounted at 1-based `index`: its stream, file (closed
// unless +1184 is set), directory entries and scratch buffer. The last
// archive moves into the freed slot. Returns false if the slot is empty.
DECOMP(0055E350, Archive_Unmount) {
    const s32 slot = static_cast<s32>(arg<u32>(ctx, 0) & 0xFFFF) - 1;
    const u32 slot_ea = load<u32>(kArchiveList) + static_cast<u32>(slot) * 4;
    const u32 zip = load<u32>(slot_ea);
    if (zip == 0) return ret(ctx, 0u);

    if (field<u32>(zip, kZipStream) != 0) {
        call(ctx, io::fn::Mem_FreePool, {load<u32>(field<u32>(zip, kZipStream) + kStreamBuffer), 0, 0});
        u32 s = field<u32>(zip, kZipStream);
        const u32 inflating = field<u32>(s, kStreamInflate);
        field<u32>(s, kStreamBuffer) = 0;
        if (inflating != 0) {
            call(ctx, io::fn::Zip_InflateEnd, {s + kStreamZ});
            s = field<u32>(zip, kZipStream);
        }
        field<u32>(s, kStreamInflate) = 0;
        call(ctx, io::fn::Mem_FreePool, {s, 0, 0});
        field<u32>(zip, kZipStream) = 0;
    }
    if (ref<u8>(zip + kZipNoClose) == 0) {
        const u32 file = field<u32>(zip, kZipFile);  // not null-checked
        call_ptr(ctx, load<u32>(load<u32>(file) + 8), {file});
    }
    for (u32 b = 0; b < kZipBucketCount; ++b) {
        for (u32 e = load<u32>(zip + kZipBuckets + 4 * b); e != 0;) {
            const u32 next = load<u32>(e + kEntryNext);
            store<u32>(e + kEntryNext, 0);
            call(ctx, io::fn::Mem_FreePool, {e, 0, 0});
            e = next;
        }
    }
    call(ctx, io::fn::Mem_FreePool, {field<u32>(zip, kZipScratch).get(), 0, 0});
    field<u32>(zip, kZipScratch) = 0;

    const u32 count = static_cast<u32>(load<u16>(kArchiveList + 8)) - 1;
    store<u16>(kArchiveList + 8, static_cast<u16>(count));
    if (slot != static_cast<s32>(count & 0xFFFF)) {
        const u32 data = load<u32>(kArchiveList);
        store<u32>(data + static_cast<u32>(slot) * 4, load<u32>(data + ((count << 2) & 0x3FFFC)));
    }
    call(ctx, io::fn::Mem_FreePool, {zip, 0, 0});
    ret(ctx, 1u);
}
