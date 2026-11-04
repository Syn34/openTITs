// Opening the current zip entry for reading (minizip's unzOpenCurrentFile).
#include "../sly4.h"

using namespace sly4;

namespace {

// ZipArchive fields.
constexpr u32 kZipFile = 4, kZipBufferSize = 12, kZipDataBase = 24, kZipMethod = 64, kZipCrc = 72;
constexpr u32 kZipCompressed = 76, kZipUncompressed = 80, kZipLocalOffset = 116, kZipStream = 120;
// ZipStream (112 bytes); a z_stream at +4.
constexpr u32 kStreamSize = 112;
constexpr u32 sReadBuffer = 0, sAvailIn = 8, sTotalOut = 24, sZalloc = 36, sZfree = 40, sOpaque = 44;
constexpr u32 sPosInZip = 60, sWanted = 64, sInflating = 68, sExtraOffset = 72, sExtraSize = 76, sExtraPos = 80;
constexpr u32 sCrc = 84, sCrcWanted = 88, sRestCompressed = 92, sRestUncompressed = 96, sFile = 100;
constexpr u32 sMethod = 104, sByteBefore = 108;
constexpr u32 kLocalHeaderSize = 30;

constexpr u32 kZalloc = 0x00F3C020, kZfree = 0x00F3C028;  // OPDs
constexpr u32 kStrZlibVersion = 0x00AF3778;              // "1.2.3"
constexpr s64 kWindowBits = -15;                          // raw deflate
constexpr u32 kZStreamSize = 56;
constexpr s32 kErrBadZip = -102, kErrNoMemory = -103;
constexpr u32 kMem_FreePool = 0x0056C730;

namespace callee {
constexpr u32 Zip_CheckLocalHeader = 0x005596F0;  // (zip, u32* header_extra, u32* extra_offset, u32* extra_size) -> 1 (name?)
constexpr u32 inflateInit2_ = 0x005DC638;         // zlib
}  // namespace callee

u32 alloc(PPUContext& ctx, u32 size) {
    // The original inlines Mem_Alloc's lazy set-up before
    // Mem_AllocPool(16, size, 0, 0); Mem_Alloc does the same.
    return static_cast<u32>(call(ctx, sly4::fn::Mem_Alloc, {size, 0, 16}));
}

}  // namespace

// s32 Archive_Prepare(ZipArchive* zip)
// Checks the current entry's local header and creates its read stream
// (+120): a read buffer of the archive's buffer size and, for deflated
// entries, a raw-inflate z_stream. Returns 1, -102 for a bad local header
// or -103 if out of memory.
DECOMP(00559D5C, Archive_Prepare) {
    const u32 zip = arg<u32>(ctx, 0);
    Stack frame(ctx, 16);
    const u32 header_extra = frame.addr(), extra_offset = frame.addr() + 4, extra_size = frame.addr() + 8;
    if (static_cast<s32>(call(ctx, callee::Zip_CheckLocalHeader, {zip, header_extra, extra_offset, extra_size})) != 1)
        return ret(ctx, kErrBadZip);

    const u32 s = alloc(ctx, kStreamSize);
    if (s == 0) {
        call(ctx, kMem_FreePool, {0, 0, 0});  // frees the null stream, as in the original
        return ret(ctx, kErrNoMemory);
    }
    const u32 buffer_size = field<u32>(zip, kZipBufferSize);
    const u32 buffer = alloc(ctx, buffer_size);
    field<u32>(s, sReadBuffer) = buffer;
    field<u32>(s, sExtraOffset) = load<u32>(extra_offset);
    field<u32>(s, sExtraPos) = 0;
    field<u32>(s, sExtraSize) = load<u32>(extra_size);
    if (buffer == 0) {
        call(ctx, kMem_FreePool, {s, 0, 0});
        return ret(ctx, kErrNoMemory);
    }
    field<u32>(s, sInflating) = 0;
    field<u32>(s, sCrcWanted) = field<u32>(zip, kZipCrc);
    field<u32>(s, sCrc) = 0;
    const u32 method = field<u32>(zip, kZipMethod);
    field<u32>(s, sMethod) = field<u32>(zip, kZipMethod);
    field<u32>(s, sFile) = field<u32>(zip, kZipFile);
    field<u32>(s, sByteBefore) = field<u32>(zip, kZipDataBase);
    field<u32>(s, sTotalOut) = 0;
    if (method != 0) {
        field<u32>(s, sZalloc) = kZalloc;
        field<u32>(s, sOpaque) = 0;
        field<u32>(s, sZfree) = kZfree;
        if (static_cast<s32>(call(ctx, callee::inflateInit2_,
                                  {s + 4, static_cast<u64>(kWindowBits), kStrZlibVersion, kZStreamSize})) == 0)
            field<u32>(s, sInflating) = 1;
    }
    field<u32>(s, sRestCompressed) = field<u32>(zip, kZipCompressed);
    field<u32>(s, sRestUncompressed) = field<u32>(zip, kZipUncompressed);
    const u32 pos = field<u32>(zip, kZipLocalOffset) + load<u32>(header_extra);
    field<u32>(s, sAvailIn) = 0;
    field<u32>(s, sWanted) = 0;
    field<u32>(zip, kZipStream) = s;
    field<u32>(s, sPosInZip) = pos + kLocalHeaderSize;
    ret(ctx, 1);
}
