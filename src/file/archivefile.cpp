// ArchiveFile (vtable 0x00B243D0): a file read from a zip archive. It
// points at its archive (+280) and its decompression stream (+284), which
// it makes the archive's current stream before every operation.
#include "zip.h"

using namespace sly4;
namespace fs = sly4::files;
namespace zp = sly4::files::zip;

namespace {

constexpr u32 kPosition = 264;  // u64
constexpr u32 kArchive = 280;   // ZipArchive*
constexpr u32 kStream = 284;    // ZipStream*

// ZipStream fields touched here.
constexpr u32 kStreamBuffer = 0;     // u8* input buffer
constexpr u32 kStreamZ = 4;          // z_stream (inflate state at +68 when active)
constexpr u32 kStreamInflate = 68;   // non-zero while inflate is set up
constexpr u32 kStreamOffset = 64;    // read offset inside the entry
constexpr u32 kStreamCrc = 84;       // CRC computed so far
constexpr u32 kStreamCrcWanted = 88;
constexpr u32 kStreamPending = 96;   // non-zero if not read to the end (CRC not checked)
constexpr u32 kArchiveEntrySize = 80;  // ZipArchive+80: uncompressed size of the current entry

constexpr u32 kStrWriteFmt = 0x00AF3980;  // "%s - Write %d bytes to %x\n"

enum SeekWhence : s32 { kSeekSet = 0, kSeekCur = 1, kSeekEnd = 2 };

u32 select_stream(u32 file) {
    const u32 archive = field<u32>(file, kArchive);
    field<u32>(archive, zp::kStream) = field<u32>(file, kStream);
    return archive;
}

}  // namespace

// ArchiveFile::~ArchiveFile() (non-deleting): only resets the vtable.
DECOMP(007A4DF8, ArchiveFile_Destruct) {
    store<u32>(arg<u32>(ctx, 0), fs::kFileBaseVtable);
}

// ArchiveFile deleting destructor.
DECOMP(007A4E08, ArchiveFile_Delete) {
    const u32 file = arg<u32>(ctx, 0);
    if (file == 0) return;
    store<u32>(file, fs::kFileBaseVtable);
    ctx.gpr[3] = call(ctx, fs::fn::Mem_FreePool, {ctx.gpr[3], 0, 0});
}

// bool ArchiveFile::Close()
// Frees the stream (and its inflate state). Returns false if the entry was
// read to the end and its CRC didn't match. The object itself stays alive.
DECOMP(00568034, ArchiveFile_Close) {
    const u32 file = arg<u32>(ctx, 0);
    const u32 archive = field<u32>(file, kArchive);
    const u32 stream = field<u32>(file, kStream);
    field<u32>(archive, zp::kStream) = stream;
    if (stream == 0) return ret(ctx, 0u);

    bool ok = true;
    if (field<u32>(stream, kStreamPending) == 0 && field<u32>(stream, kStreamCrc) != field<u32>(stream, kStreamCrcWanted))
        ok = false;
    call(ctx, fs::fn::Mem_FreePool, {field<u32>(stream, kStreamBuffer), 0, 0});
    u32 s = field<u32>(archive, zp::kStream);
    field<u32>(s, kStreamBuffer) = 0;
    if (field<u32>(s, kStreamInflate) != 0) {
        call(ctx, fs::fn::Zip_InflateEnd, {s + kStreamZ});
        s = field<u32>(archive, zp::kStream);
    }
    field<u32>(s, kStreamInflate) = 0;
    call(ctx, fs::fn::Mem_FreePool, {s, 0, 0});
    field<u32>(archive, zp::kStream) = 0;
    ret(ctx, ok ? 1u : 0u);
}

// s32 ArchiveFile::Read(void* buf, u32 size)
DECOMP(00568118, ArchiveFile_Read) {
    const u32 file = arg<u32>(ctx, 0);
    const u32 archive = select_stream(file);
    const u32 n = static_cast<u32>(call(ctx, fs::fn::Zip_StreamRead, {archive, ctx.gpr[4], ctx.gpr[5]}));
    field<u64>(file, kPosition) += static_cast<u64>(n);
    ret(ctx, static_cast<s32>(n));
}

// s32 ArchiveFile::Write(const void* buf, u32 size): archives are read-only;
// formats a log line and returns 0.
DECOMP(00568168, ArchiveFile_Write) {
    const u32 file = arg<u32>(ctx, 0);
    call(ctx, fs::fn::String_Format, {kStrWriteFmt, file + 4, ctx.gpr[5], ctx.gpr[4]});
    ret(ctx, 0);
}

// s32 ArchiveFile::Seek(s64 offset, u8 whence)
// Moves both the stream offset (32-bit, whence taken as a byte) and the
// file position (64-bit, whence taken as an int). Returns 0.
DECOMP(005681A4, ArchiveFile_Seek) {
    const u32 file = arg<u32>(ctx, 0);
    const u64 offset = ctx.gpr[4];
    const s32 whence = arg<s32>(ctx, 2);
    const u32 archive = field<u32>(file, kArchive);
    const u32 stream = field<u32>(file, kStream);
    const u64 position = field<u64>(file, kPosition);
    field<u32>(archive, zp::kStream) = stream;

    switch (static_cast<u8>(whence)) {
    case kSeekSet: field<u32>(stream, kStreamOffset) = static_cast<u32>(offset); break;
    case kSeekCur: field<u32>(stream, kStreamOffset) += static_cast<u32>(offset); break;
    case kSeekEnd:
        field<u32>(stream, kStreamOffset) = field<u32>(archive, kArchiveEntrySize) - static_cast<u32>(offset);
        break;
    default: break;
    }
    switch (whence) {
    case kSeekSet: field<u64>(file, kPosition) = offset; break;
    case kSeekCur: field<u64>(file, kPosition) = position + offset; break;
    case kSeekEnd: field<u64>(file, kPosition) = fs::file_vcall(ctx, file, fs::kFileSize, {file}) - offset; break;
    default: break;
    }
    ret(ctx, 0);
}

// u32 ArchiveFile::Size(): the current entry's uncompressed size.
DECOMP(0056829C, ArchiveFile_Size) {
    const u32 archive = select_stream(arg<u32>(ctx, 0));
    ret(ctx, field<u32>(archive, kArchiveEntrySize).get());
}
