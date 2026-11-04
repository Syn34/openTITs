// Zip reading helpers: little-endian reads through the File vtable, the
// end-record scan, opening a zip from an already open File, and the
// minizip-style stream read (stored or deflated, with CRC).
#include "fsop.h"

using namespace sly4;
namespace op = sly4::fsop;

namespace {

constexpr u32 kZipKey = 0, kZipFile = 4, kZipBufferSize = 12, kZipStream = 120;
constexpr u32 kZipEnd = 40, kZipDirSize = 44, kZipDirStart = 48, kZipDataBase = 24;
constexpr u32 kZipEntryIndex = 28, kZipEntryOffset = 32, kZipEntryValid = 36, kZipName = 124;
constexpr u32 kZipPrefix = 928, kZipNoClose = 1184;
constexpr u32 kPathBuffer = 0x0116B050;
constexpr u32 kStrDotSlash = 0x00AF3780;  // "./"

namespace callee {
constexpr u32 Zip_ReadDirectory = 0x0055CF1C;
constexpr u32 Zip_LocateEntry = 0x0055A8E4;
constexpr u32 Zip_BuildIndex = 0x0055C01C;
constexpr u32 crc32 = 0x005DBCB0;    // zlib crc32(crc, buf, len)
constexpr u32 inflate = 0x005DCAC8;  // zlib inflate(strm, flush)
constexpr u32 strlen = 0x0071ED08, strstr = 0x0071EEF8, strcpy = 0x007239D8;
}  // namespace callee
constexpr u32 Z_SYNC_FLUSH = 2;
constexpr s32 Z_STREAM_END = 1;

// File virtuals (slot 3 Read(buf, n, 0), slot 5 Seek(offset, whence)).
s32 file_read(PPUContext& ctx, u32 file, u32 buf, u32 n) {
    return static_cast<s32>(call_ptr(ctx, load<u32>(load<u32>(file) + 12), {file, buf, n, 0}));
}
u64 file_seek(PPUContext& ctx, u32 file, u32 offset, u32 whence) {
    return call_ptr(ctx, load<u32>(load<u32>(file) + 20), {file, offset, whence});
}

// Zip stream (ZipArchive+120) fields: a z_stream at +4.
namespace zs {
constexpr u32 read_buffer = 0, next_in = 4, avail_in = 8, next_out = 16, avail_out = 20, total_out = 24;
constexpr u32 pos_in_zip = 60, crc = 84, rest_compressed = 92, rest_uncompressed = 96;
constexpr u32 file = 100, method = 104, byte_before = 108;
}  // namespace zs

u32 stream(u32 zip) { return field<u32>(zip, kZipStream); }

// Refill the input buffer: up to min(rest_compressed, buffer size) bytes at
// pos_in_zip + byte_before. Returns false if nothing to read or a short read.
bool refill(PPUContext& ctx, u32 zip, u32 want) {
    if (want == 0) return false;
    u32 s = stream(zip);
    file_seek(ctx, field<u32>(s, zs::file), field<u32>(s, zs::pos_in_zip) + field<u32>(s, zs::byte_before), 0);
    s = stream(zip);
    if (file_read(ctx, field<u32>(s, zs::file), field<u32>(s, zs::read_buffer), want) != static_cast<s32>(want))
        return false;
    return true;
}

}  // namespace

// bool Zip_ReadU32(ZipArchive* zip, u32* out)
// Four bytes, little-endian; *out = 0 if any read fails.
DECOMP(00559534, Zip_ReadU32) {
    const u32 zip = arg<u32>(ctx, 0), out = arg<u32>(ctx, 1);
    Stack frame(ctx, 8);
    u32 value = 0;
    bool ok = true;
    for (u32 i = 0; i < 4 && ok; ++i) {
        ok = file_read(ctx, field<u32>(zip, kZipFile), frame.addr() + 3 - i, 1) == 1;
        if (ok) value |= static_cast<u32>(ref<u8>(frame.addr() + 3 - i)) << (8 * i);
    }
    store<u32>(out, ok ? value : 0);
    ret(ctx, ok ? 1 : 0);
}

// bool ZipArchive_OpenMemory(ZipArchive* zip, File* file, const char* prefix)
// ZipArchive_Open for a File that is already open (it is never closed by
// the archive: +1184 = 1).
DECOMP(005677A4, ZipArchive_OpenMemory) {
    const u32 zip = arg<u32>(ctx, 0), file = arg<u32>(ctx, 1), prefix = arg<u32>(ctx, 2);
    Stack frame(ctx, 8);
    const u32 end_out = frame.addr(), ok_out = frame.addr() + 4;
    store<u32>(end_out, 0);
    field<u32>(zip, kZipFile) = file;
    store<s32>(ok_out, 1);
    ref<u8>(zip + kZipNoClose) = 1;
    call(ctx, callee::Zip_ReadDirectory, {zip, 0, ok_out, end_out});
    if (load<s32>(ok_out) != 1) return ret(ctx, 0u);

    const u32 end = load<u32>(end_out);
    field<u32>(zip, kZipEnd) = end;
    field<u32>(zip, kZipStream) = 0;
    field<u32>(zip, kZipDataBase) = end - (field<u32>(zip, kZipDirStart) + field<u32>(zip, kZipDirSize));
    if (prefix == 0) {
        ref<u8>(zip + kZipPrefix) = 0;
    } else {
        for (s32 i = 0; i < static_cast<s32>(call(ctx, callee::strlen, {prefix})); ++i) {
            const u8 c = ref<u8>(prefix + i);
            ref<u8>(kPathBuffer + i) = c == '\\' ? '/' : c;
        }
        ref<u8>(kPathBuffer + static_cast<s32>(call(ctx, callee::strlen, {prefix}))) = 0;
        const u32 dot = static_cast<u32>(call(ctx, callee::strstr, {kPathBuffer, kStrDotSlash}));
        call(ctx, callee::strcpy, {zip + kZipPrefix, dot != 0 ? dot + 2 : kPathBuffer});
    }
    field<u32>(zip, kZipEntryOffset) = field<u32>(zip, kZipDirStart);
    field<u32>(zip, kZipEntryIndex) = 0;
    call(ctx, callee::Zip_LocateEntry, {zip});
    if (field<u32>(zip, kZipEntryValid) == 0) ref<u8>(zip + kZipName) = 0;
    call(ctx, callee::Zip_BuildIndex, {zip});
    ret(ctx, 1u);
}

// u32 Zip_FindEndRecord(ZipArchive* zip, bool sca)
// Scans backwards (1 KiB windows, at most 64 KiB) for "PK\5\6" and returns
// its offset, or 0. For .sca archives the last 4 bytes are an XOR key
// (read into zip+0) applied to each candidate word before the test.
DECOMP(0055C780, Zip_FindEndRecord) {
    const u32 zip = arg<u32>(ctx, 0);
    const bool sca = arg<u8>(ctx, 1) != 0;
    u32 size = static_cast<u32>(file_seek(ctx, field<u32>(zip, kZipFile), 0, 2));  // SEEK_END
    if (sca) {
        size -= 4;
        file_seek(ctx, field<u32>(zip, kZipFile), size, 0);
        file_read(ctx, field<u32>(zip, kZipFile), zip + kZipKey, 4);
    }
    const u32 max_back = size < 0xFFFF ? size : 0xFFFF;
    // The original inlines Mem_Alloc's lazy set-up before
    // Mem_AllocPool(16, 1028, 0, 0); Mem_Alloc does the same.
    const u32 buf = static_cast<u32>(call(ctx, sly4::fn::Mem_Alloc, {1028, 0, 16}));
    u32 found = 0;
    u32 back = 4;
    if (max_back > 4) {
        do {
            back = back + 1024 > max_back ? max_back : back + 1024;
            const u32 read_pos = size - back;
            u32 chunk = 1028;
            if (size - read_pos <= 0x404) chunk = size - read_pos;
            file_seek(ctx, field<u32>(zip, kZipFile), read_pos, 0);
            if (file_read(ctx, field<u32>(zip, kZipFile), buf, chunk) != static_cast<s32>(chunk)) break;
            for (s32 i = static_cast<s32>(chunk) - 4; static_cast<s32>(chunk) - 3 > 0;) {
                const u32 p = buf + static_cast<u32>(i);
                if (sca) store<u32>(p, load<u32>(p) ^ field<u32>(zip, kZipKey));
                if (ref<u8>(p) == 'P' && ref<u8>(p + 1) == 'K' && ref<u8>(p + 2) == 5 && ref<u8>(p + 3) == 6) {
                    found = read_pos + static_cast<u32>(i);
                    break;  // (the matched word stays XORed)
                }
                if (sca) store<u32>(p, load<u32>(p) ^ field<u32>(zip, kZipKey));
                if (i-- <= 0) break;
            }
        } while (found == 0 && back < max_back);
    }
    call(ctx, op::fn::Mem_FreePool, {buf, 0, 0});
    ret(ctx, found);
}

// s32 Zip_ReadRaw(ZipArchive* zip, void* buf, u32 size)
// minizip's unzReadCurrentFile on the current stream. Deflated entries
// return the bytes produced (or a zlib error); stored entries return 1.
// -100 if the stream has no read buffer.
DECOMP(0056736C, Zip_ReadRaw) {
    const u32 zip = arg<u32>(ctx, 0);
    const u32 buf = arg<u32>(ctx, 1);
    const u32 size = arg<u32>(ctx, 2);
    u32 s = stream(zip);
    if (field<u32>(s, zs::read_buffer) == 0) return ret(ctx, -100);
    if (size == 0) return ret(ctx, 0);
    field<u32>(s, zs::next_out) = buf;
    field<u32>(s, zs::avail_out) = size;
    if (size > field<u32>(s, zs::rest_uncompressed)) field<u32>(s, zs::avail_out) = field<u32>(s, zs::rest_uncompressed);
    u32 total = 0;

    if (field<u32>(s, zs::method) == 0) {
        // Stored: copy straight from the input buffer.
        if (field<u32>(s, zs::avail_out) == 0) return ret(ctx, 1);
        do {
            if (field<u32>(s, zs::avail_in) == 0 && field<u32>(s, zs::rest_compressed) != 0) {
                const u32 rest = field<u32>(s, zs::rest_compressed), bufsize = field<u32>(zip, kZipBufferSize);
                const u32 want = rest < bufsize ? rest : bufsize;
                if (!refill(ctx, zip, want)) return ret(ctx, 0);
                s = stream(zip);
                field<u32>(s, zs::avail_in) = want;
                field<u32>(s, zs::pos_in_zip) += want;
                field<u32>(s, zs::rest_compressed) -= want;
                field<u32>(s, zs::next_in) = field<u32>(s, zs::read_buffer);
            }
            const u32 out = field<u32>(s, zs::avail_out), in = field<u32>(s, zs::avail_in);
            const u32 n = out < in ? out : in;
            for (u32 i = 0; i < n; ++i) {  // byte copy, pointers re-read each byte in the original
                s = stream(zip);
                ref<u8>(field<u32>(s, zs::next_out) + i) = ref<u8>(field<u32>(s, zs::next_in) + i);
            }
            s = stream(zip);
            const u32 crc = static_cast<u32>(call(ctx, callee::crc32, {field<u32>(s, zs::crc).get(), field<u32>(s, zs::next_out).get(), n}));
            s = stream(zip);
            total += n;
            field<u32>(s, zs::crc) = crc;
            field<u32>(s, zs::rest_uncompressed) -= n;
            field<u32>(s, zs::avail_in) -= n;
            field<u32>(s, zs::avail_out) -= n;
            field<u32>(s, zs::next_out) += n;
            field<u32>(s, zs::next_in) += n;
            field<u32>(s, zs::total_out) += n;
        } while (field<u32>(s, zs::avail_out) != 0);
        return ret(ctx, 1);  // err stays 1 on this path in the original
    }

    // Deflated.
    if (field<u32>(s, zs::avail_out) == 0) return ret(ctx, 1);
    for (;;) {
        if (field<u32>(s, zs::avail_in) == 0 && field<u32>(s, zs::rest_compressed) != 0) {
            const u32 rest = field<u32>(s, zs::rest_compressed), bufsize = field<u32>(zip, kZipBufferSize);
            const u32 want = bufsize < rest ? bufsize : rest;
            if (!refill(ctx, zip, want)) return ret(ctx, 0);
            s = stream(zip);
            field<u32>(s, zs::pos_in_zip) += want;
            field<u32>(s, zs::rest_compressed) -= want;
            field<u32>(s, zs::next_in) = field<u32>(s, zs::read_buffer);
            field<u32>(s, zs::avail_in) = want;
        }
        const u32 before = field<u32>(s, zs::total_out);
        const u32 out_before = field<u32>(s, zs::next_out);
        const s32 err = static_cast<s32>(call(ctx, callee::inflate, {s + 4, Z_SYNC_FLUSH}));
        s = stream(zip);
        const u32 produced = field<u32>(s, zs::total_out) - before;
        const u32 crc = static_cast<u32>(call(ctx, callee::crc32, {field<u32>(s, zs::crc).get(), out_before, produced}));
        s = stream(zip);
        total += produced;
        field<u32>(s, zs::crc) = crc;
        field<u32>(s, zs::rest_uncompressed) -= produced;
        if (err == Z_STREAM_END) return ret(ctx, static_cast<s32>(total));
        if (err != 0) return ret(ctx, err);
        if (field<u32>(s, zs::avail_out) == 0) return ret(ctx, static_cast<s32>(total));
    }
}
