// Zip reading: the end-of-central-directory record and stream reads.
#include "fsio.h"

using namespace sly4;
namespace io = sly4::fsio;

namespace {

constexpr u32 kZipFile = 4, kZipScratch = 8, kZipScratchSize = 12, kZipEntries = 16, kZipCommentLen = 20;
constexpr u32 kZipDirSize = 44, kZipDirOffset = 48;  // (which is size and which offset is a guess)
constexpr u32 kZipStream = 120;
constexpr u32 kStreamPos = 24, kStreamWanted = 64;
constexpr s32 kZipBadDirectory = -102;

// One byte through the File's virtual Read(buf, 1, 0) (slot 3).
bool read_byte(PPUContext& ctx, u32 zip, u32 tmp, u8& out) {
    const u32 file = field<u32>(zip, kZipFile);
    if (static_cast<s32>(call_ptr(ctx, load<u32>(load<u32>(file) + 12), {file, tmp, 1, 0})) != 1) return false;
    out = ref<u8>(tmp);
    return true;
}

// A little-endian u16 from two byte reads; 0 (and *ok = 0) if either fails.
u32 read_u16(PPUContext& ctx, u32 zip, u32 tmp, u32 ok_out) {
    u8 lo = 0, hi = 0;
    const bool ok = read_byte(ctx, zip, tmp, lo) && read_byte(ctx, zip, tmp + 1, hi);
    if (!ok) store<s32>(ok_out, 0);
    return ok ? static_cast<u32>(lo) | static_cast<u32>(hi) << 8 : 0;
}

}  // namespace

// void Zip_ReadDirectory(ZipArchive* zip, bool sca, s32* ok, u32* end)
// Finds and parses the end-of-central-directory record: entry counts
// (+16), directory size/offset (+44/+48) and comment length (+20).
// *ok becomes 0 on a read failure, -102 for a multi-disk archive, a
// count mismatch or a directory past the end record. *end is the record's
// offset. The signature is read but not checked.
DECOMP(0055CF1C, Zip_ReadDirectory) {
    const u32 zip = arg<u32>(ctx, 0);
    const u32 sca = arg<u32>(ctx, 1);
    const u32 ok_out = arg<u32>(ctx, 2), end_out = arg<u32>(ctx, 3);
    if (field<u32>(zip, kZipFile) == 0) {
        store<s32>(ok_out, 0);
        return ret(ctx, 0u);
    }

    Stack frame(ctx, 16);
    const u32 signature = frame.addr(), tmp = frame.addr() + 4;
    u32 end = static_cast<u32>(call(ctx, io::fn::Zip_FindEndRecord, {zip, sca != 0 ? 1u : 0u}));
    store<u32>(end_out, end);
    if (end == 0) {
        store<s32>(ok_out, 0);
        end = load<u32>(end_out);
    }
    const u32 file = field<u32>(zip, kZipFile);
    call_ptr(ctx, load<u32>(load<u32>(file) + 20), {file, end, 0});  // Seek(end, SEEK_SET)
    if (static_cast<s32>(call(ctx, io::fn::Zip_ReadU32, {zip, signature})) != 1) store<s32>(ok_out, 0);

    const u32 disk = read_u16(ctx, zip, tmp, ok_out);
    const u32 dir_disk = read_u16(ctx, zip, tmp, ok_out);
    field<u32>(zip, kZipEntries) = read_u16(ctx, zip, tmp, ok_out);
    const u32 total = read_u16(ctx, zip, tmp, ok_out);
    if (total != field<u32>(zip, kZipEntries) || disk != 0 || dir_disk != 0) store<s32>(ok_out, kZipBadDirectory);
    if (static_cast<s32>(call(ctx, io::fn::Zip_ReadU32, {zip, zip + kZipDirSize})) != 1) store<s32>(ok_out, 0);
    if (static_cast<s32>(call(ctx, io::fn::Zip_ReadU32, {zip, zip + kZipDirOffset})) != 1) store<s32>(ok_out, 0);
    field<u32>(zip, kZipCommentLen) = read_u16(ctx, zip, tmp, ok_out);

    const u32 dir_end = field<u32>(zip, kZipDirOffset) + field<u32>(zip, kZipDirSize);
    if (load<u32>(end_out) < dir_end && load<s32>(ok_out) == 1) store<s32>(ok_out, kZipBadDirectory);
}

// u32 Zip_StreamRead(ZipArchive* zip, void* buf, u32 size)
// Reads from the current stream at its wanted offset (+64). If that is
// ahead of the decompressed position (+24) the gap is read and discarded
// through the scratch buffer (allocated on first use, +12 bytes). The
// wanted offset then advances by the bytes read.
DECOMP(00567938, Zip_StreamRead) {
    const u32 zip = arg<u32>(ctx, 0);
    const u32 buf = arg<u32>(ctx, 1);
    const u64 size = ctx.gpr[5];
    const u32 stream = field<u32>(zip, kZipStream);
    const u32 at = field<u32>(stream, kStreamPos);
    if (field<u32>(stream, kStreamWanted) > at) {
        if (field<u32>(zip, kZipScratch) == 0) {
            // The original inlines Mem_Alloc's lazy set-up before
            // Mem_AllocPool(16, size, 0, 0); Mem_Alloc does the same.
            field<u32>(zip, kZipScratch) = static_cast<u32>(
                call(ctx, sly4::fn::Mem_Alloc, {field<u32>(zip, kZipScratchSize).get(), 0, 16}));
        }
        u32 skip = field<u32>(field<u32>(zip, kZipStream), kStreamWanted) - at;
        while (skip != 0) {
            u32 chunk = skip;
            const u32 scratch_size = field<u32>(zip, kZipScratchSize);
            if (scratch_size < chunk) chunk = scratch_size;
            call(ctx, io::fn::Zip_ReadRaw, {zip, field<u32>(zip, kZipScratch).get(), chunk});
            skip -= chunk;
        }
    }
    const u64 n = call(ctx, io::fn::Zip_ReadRaw, {zip, buf, size});
    field<u32>(field<u32>(zip, kZipStream), kStreamWanted) += static_cast<u32>(n);
    ctx.gpr[3] = n;
}
