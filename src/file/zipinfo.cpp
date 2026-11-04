// Zip central-directory entry parsing (minizip's
// unzlocal_GetCurrentFileInfoInternal, read byte by byte through the File).
#include "../sly4.h"

using namespace sly4;

namespace {

constexpr u32 kZipFile = 4, kZipDataBase = 24, kZipEntryOffset = 32, kZipEntryValid = 36;
// Entry info at ZipArchive+52.. (all u32 unless noted).
constexpr u32 kVersion = 52, kVersionNeeded = 56, kFlag = 60, kMethod = 64, kDosDate = 68;
constexpr u32 kCrc = 72, kCompressed = 76, kUncompressed = 80, kNameLen = 84, kExtraLen = 88;
constexpr u32 kCommentLen = 92, kDiskStart = 96, kInternalAttr = 100, kExternalAttr = 104;
constexpr u32 kSec = 108, kMin = 109, kHour = 110, kDay = 111, kMonth = 112, kYear = 114;  // u8 / u16 year
constexpr u32 kLocalOffset = 116, kName = 124, kExtra = 380, kComment = 396;
constexpr u32 kMaxName = 255, kMaxExtra = 16;
constexpr u32 kCentralSignature = 0x02014B50;
constexpr s32 kBadZip = -102;
constexpr u32 Zip_ReadU32 = 0x00559534;

s32 file_read(PPUContext& ctx, u32 zip, u32 buf, u32 n) {
    const u32 file = field<u32>(zip, kZipFile);
    return static_cast<s32>(call_ptr(ctx, load<u32>(load<u32>(file) + 12), {file, buf, n, 0}));
}
void file_seek(PPUContext& ctx, u32 zip, u64 offset, u32 whence) {
    const u32 file = field<u32>(zip, kZipFile);
    call_ptr(ctx, load<u32>(load<u32>(file) + 20), {file, offset, whence});
}

// A little-endian u16 stored at zip+off; 0 (and err = 0) if a byte read fails.
void read_u16(PPUContext& ctx, u32 zip, u32 tmp, u32 off, s32& err) {
    bool ok = file_read(ctx, zip, tmp, 1) == 1;
    const u8 lo = ok ? ref<u8>(tmp) : 0;
    if (ok) ok = file_read(ctx, zip, tmp + 1, 1) == 1;
    const u8 hi = ref<u8>(tmp + 1);
    field<u32>(zip, off) = ok ? static_cast<u32>(lo) | static_cast<u32>(hi) << 8 : 0;
    if (!ok) err = 0;
}

void read_u32(PPUContext& ctx, u32 zip, u32 out, s32& err) {
    if (static_cast<s32>(call(ctx, Zip_ReadU32, {zip, out})) != 1) err = 0;
}

}  // namespace

// s8 Zip_LocateEntry(ZipArchive* zip)
// Reads the central-directory header at data base + entry offset (+32)
// into the entry fields, the name (at most 255 bytes) into +124, up to 16
// bytes of the extra field into +380 and of the comment into +396. Returns
// 1 on success, 0 on a read failure, -102 for a bad signature; +36 = (result == 1).
DECOMP(0055A8E4, Zip_LocateEntry) {
    const u32 zip = arg<u32>(ctx, 0);
    s32 err = 1;
    Stack frame(ctx, 16);
    const u32 sig = frame.addr(), tmp = frame.addr() + 8;

    file_seek(ctx, zip, static_cast<u32>(field<u32>(zip, kZipEntryOffset) + field<u32>(zip, kZipDataBase)), 0);
    if (static_cast<s32>(call(ctx, Zip_ReadU32, {zip, sig})) != 1) err = 0;
    else if (load<u32>(sig) != kCentralSignature) err = kBadZip;

    read_u16(ctx, zip, tmp, kVersion, err);
    read_u16(ctx, zip, tmp, kVersionNeeded, err);
    read_u16(ctx, zip, tmp, kFlag, err);
    read_u16(ctx, zip, tmp, kMethod, err);
    read_u32(ctx, zip, zip + kDosDate, err);
    {
        const u32 d = field<u32>(zip, kDosDate);
        ref<u8>(zip + kDay) = static_cast<u8>((d >> 16) & 0x1F);
        ref<u8>(zip + kHour) = static_cast<u8>((d >> 11) & 0x1F);
        ref<u8>(zip + kMonth) = static_cast<u8>(((d >> 21) & 0xF) - 1);
        store<u16>(zip + kYear, static_cast<u16>(((d >> 25) & 0x7F) + 1980));
        ref<u8>(zip + kMin) = static_cast<u8>((d >> 5) & 0x3F);
        ref<u8>(zip + kSec) = static_cast<u8>((d << 1) & 0x3E);
    }
    read_u32(ctx, zip, zip + kCrc, err);
    read_u32(ctx, zip, zip + kCompressed, err);
    read_u32(ctx, zip, zip + kUncompressed, err);
    read_u16(ctx, zip, tmp, kNameLen, err);
    read_u16(ctx, zip, tmp, kExtraLen, err);
    read_u16(ctx, zip, tmp, kCommentLen, err);
    read_u16(ctx, zip, tmp, kDiskStart, err);
    read_u16(ctx, zip, tmp, kInternalAttr, err);
    read_u32(ctx, zip, zip + kExternalAttr, err);
    read_u32(ctx, zip, zip + kLocalOffset, err);

    // Name.
    const u32 name_len = field<u32>(zip, kNameLen);
    s64 skip = static_cast<s32>(name_len);
    if (err == 1 && name_len != 0) {
        const u32 n = name_len < kMaxName ? name_len : kMaxName;
        if (file_read(ctx, zip, zip + kName, n) != static_cast<s32>(n)) err = 0;
        ref<u8>(zip + kName + n) = 0;
        skip = static_cast<s32>(field<u32>(zip, kNameLen) - n);
    }
    // Extra field.
    if (err != 1) {
        skip = static_cast<s32>(static_cast<u32>(skip) + field<u32>(zip, kExtraLen));
    } else {
        const u32 extra_len = field<u32>(zip, kExtraLen);
        const u32 n = extra_len < kMaxExtra ? extra_len : kMaxExtra;
        if (skip != 0) file_seek(ctx, zip, static_cast<u64>(skip), 1);  // SEEK_CUR past the rest of the name
        if (field<u32>(zip, kExtraLen) != 0 && file_read(ctx, zip, zip + kExtra, n) != static_cast<s32>(n)) err = 0;
        skip = static_cast<s32>(field<u32>(zip, kExtraLen) - n);
    }
    // Comment. Its read length is min(extra length, 16), as in the original.
    if (err == 1) {
        const u32 extra_len = field<u32>(zip, kExtraLen);
        const u32 n = extra_len < kMaxExtra ? extra_len : kMaxExtra;
        if (skip != 0) file_seek(ctx, zip, static_cast<u64>(skip), 1);
        if (field<u32>(zip, kCommentLen) != 0 && file_read(ctx, zip, zip + kComment, n) != static_cast<s32>(n)) err = 0;
    }
    field<u32>(zip, kZipEntryValid) = err == 1 ? 1 : 0;
    ret(ctx, static_cast<s8>(err));
}
