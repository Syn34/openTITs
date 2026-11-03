// Zip name index: one ZipEntry per central-directory entry, hashed into
// 128 buckets for Archive_Find / Archive_Contains.
#include "fsio.h"

using namespace sly4;
namespace io = sly4::fsio;

namespace {

constexpr u32 kZipEntries = 16, kZipEntryIndex = 28, kZipEntryOffset = 32, kZipEntryValid = 36;
constexpr u32 kZipNameLen = 84, kZipExtraLen = 88, kZipCommentLen = 92;  // of the current entry
constexpr u32 kZipDirStart = 48;   // where the first central-directory header is
constexpr u32 kZipName = 124, kZipBuckets = 416, kZipIndexCount = 412, kZipPrefix = 928;
constexpr u32 kZipBucketCount = 128, kBucketMask = 0x1FC;
constexpr u32 kCentralHeaderSize = 46;
// ZipEntry (264 bytes): +0 directory offset, +4 char name[256], +260 next.
constexpr u32 kEntrySize = 264, kEntryName = 4, kEntryNext = 260;
constexpr u32 kStrPathFmt = 0x00AF3784;  // "%s%s"
constexpr u32 kCtypeTable = 0x00F94594;  // u32 -> u16 C library character class table
constexpr u16 kCtypeUpper = 0x0002;

namespace callee {
constexpr u32 Zip_LocateEntry = 0x0055A8E4;  // (zip) -> 1 on success; reads the header at +32 (696 instr)
constexpr u32 String_Printf = 0x00551540;    // sprintf
}  // namespace callee

}  // namespace

// s8 Zip_BuildIndex(ZipArchive* zip)
// Rebuilds the bucket index from the central directory, naming each entry
// "<prefix><name>". Stores the entry count at +412 and returns it (as s8);
// 0 if the archive has no usable entry.
DECOMP(0055C01C, Zip_BuildIndex) {
    const u32 zip = arg<u32>(ctx, 0);
    if (field<u32>(zip, kZipEntryValid) == 0) return ret(ctx, 0u);

    for (u32 b = 0; b < kZipBucketCount; ++b) store<u32>(zip + kZipBuckets + 4 * b, 0);
    u32 count = 0;
    field<u32>(zip, kZipEntryIndex) = 0;
    field<u32>(zip, kZipEntryOffset) = field<u32>(zip, kZipDirStart);
    call(ctx, callee::Zip_LocateEntry, {zip});
    if (field<u32>(zip, kZipEntryValid) == 0) ref<u8>(zip + kZipName) = 0;

    for (;;) {
        if (ref<u8>(zip + kZipName) != 0) {
            // The original inlines Mem_Alloc's lazy set-up before
            // Mem_AllocPool(16, 264, 0, 0); Mem_Alloc does the same.
            const u32 entry = static_cast<u32>(call(ctx, sly4::fn::Mem_Alloc, {kEntrySize, 0, 16}));
            call(ctx, callee::String_Printf, {entry + kEntryName, kStrPathFmt, zip + kZipPrefix, zip + kZipName});
            const u32 ctype = load<u32>(kCtypeTable);
            u32 h = 0;
            for (u32 p = entry + kEntryName; ref<u8>(p) != 0; ++p) {
                s32 c = static_cast<s8>(ref<u8>(p));
                if (load<u16>(ctype + 2 * static_cast<u32>(c)) & kCtypeUpper) c += 32;
                h = h * 37 + static_cast<u32>(c);
            }
            const u32 bucket = zip + kZipBuckets + ((h << 2) & kBucketMask);
            ++count;
            store<u32>(entry, field<u32>(zip, kZipEntryOffset));  // unchecked allocation
            store<u32>(entry + kEntryNext, load<u32>(bucket));
            store<u32>(bucket, entry);
        }
        if (field<u32>(zip, kZipEntryValid) == 0) break;
        const u32 next = field<u32>(zip, kZipEntryIndex) + 1;
        if (static_cast<s32>(next) == static_cast<s32>(field<u32>(zip, kZipEntries).get())) break;
        field<u32>(zip, kZipEntryIndex) = next;
        field<u32>(zip, kZipEntryOffset) = field<u32>(zip, kZipEntryOffset) + field<u32>(zip, kZipNameLen) +
                                           field<u32>(zip, kZipExtraLen) + field<u32>(zip, kZipCommentLen) +
                                           kCentralHeaderSize;
        const s8 more = static_cast<s8>(call(ctx, callee::Zip_LocateEntry, {zip}));
        if (field<u32>(zip, kZipEntryValid) == 0) ref<u8>(zip + kZipName) = 0;
        if (more != 1) break;
    }
    field<u32>(zip, kZipIndexCount) = count;
    ret(ctx, static_cast<s8>(count));
}
