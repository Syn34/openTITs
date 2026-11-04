// Zip local file header check (minizip's
// unzlocal_CheckCurrentFileCoherencyHeader), read through the File vtable.
#include "../sly4.h"

using namespace sly4;

namespace {

constexpr u32 kZipFile = 4, kZipDataBase = 24, kZipMethod = 64, kZipCrc = 72;
constexpr u32 kZipCompressed = 76, kZipUncompressed = 80, kZipNameLen = 84, kZipLocalOffset = 116;
constexpr u32 kLocalSignature = 0x04034B50;
constexpr u32 kLocalHeaderSize = 30;
constexpr u32 kFlagDataDescriptor = 8;  // sizes/CRC follow the data
constexpr u32 Z_DEFLATED = 8;
constexpr s32 kBadZip = -102;
constexpr u32 Zip_ReadU32 = 0x00559534;

s32 file_read(PPUContext& ctx, u32 zip, u32 buf, u32 n) {
    const u32 file = field<u32>(zip, kZipFile);
    return static_cast<s32>(call_ptr(ctx, load<u32>(load<u32>(file) + 12), {file, buf, n, 0}));
}

// Little-endian u16 from two byte reads; 0 if either fails.
bool read_u16(PPUContext& ctx, u32 zip, u32 tmp, u32& value) {
    bool ok = file_read(ctx, zip, tmp, 1) == 1;
    const u8 lo = ref<u8>(tmp);
    if (ok) ok = file_read(ctx, zip, tmp + 1, 1) == 1;
    value = ok ? static_cast<u32>(lo) | static_cast<u32>(ref<u8>(tmp + 1)) << 8 : 0;
    return ok;
}

}  // namespace

// s8 Zip_CheckLocalHeader(ZipArchive* zip, u32* header_extra, u32* extra_offset, u32* extra_size)
// Checks the current entry's local header against its central-directory
// info. Outputs the name + extra field length (after the 30-byte header),
// where the local extra field starts and its size. 1 / 0 (read error) / -102.
DECOMP(005596F0, Zip_CheckLocalHeader) {
    const u32 zip = arg<u32>(ctx, 0);
    const u32 header_extra = arg<u32>(ctx, 1), extra_offset = arg<u32>(ctx, 2), extra_size = arg<u32>(ctx, 3);
    s32 err = 1;
    store<u32>(header_extra, 0);
    store<u32>(extra_offset, 0);
    store<u32>(extra_size, 0);
    {
        const u32 file = field<u32>(zip, kZipFile);
        const u32 at = field<u32>(zip, kZipLocalOffset) + field<u32>(zip, kZipDataBase);
        call_ptr(ctx, load<u32>(load<u32>(file) + 20), {file, at, 0});  // Seek(SEEK_SET)
    }
    Stack frame(ctx, 16);
    const u32 magic = frame.addr(), data = frame.addr() + 4, tmp = frame.addr() + 8;
    u32 value;

    if (static_cast<s32>(call(ctx, Zip_ReadU32, {zip, magic})) != 1) err = 0;
    else if (load<u32>(magic) != kLocalSignature) err = kBadZip;

    if (!read_u16(ctx, zip, tmp, value)) err = 0;  // version needed
    store<u32>(data, value);
    u32 flags;
    if (!read_u16(ctx, zip, tmp, flags)) err = 0;
    {
        const bool was_ok = err == 1;
        const bool ok = read_u16(ctx, zip, tmp, value);  // method
        store<u32>(data, value);
        if (!ok) err = 0;
        else if (was_ok) {
            if (value != field<u32>(zip, kZipMethod)) err = kBadZip;
            else if (field<u32>(zip, kZipMethod) != 0 && field<u32>(zip, kZipMethod) != Z_DEFLATED) err = kBadZip;
        }
    }
    if (static_cast<s32>(call(ctx, Zip_ReadU32, {zip, data})) != 1) err = 0;  // date
    for (u32 off : {kZipCrc, kZipCompressed, kZipUncompressed}) {
        if (static_cast<s32>(call(ctx, Zip_ReadU32, {zip, data})) != 1) err = 0;
        else if (err == 1 && load<u32>(data) != field<u32>(zip, off) && !(flags & kFlagDataDescriptor)) err = kBadZip;
    }
    u32 name_len;
    {
        const bool was_ok = err == 1;
        if (!read_u16(ctx, zip, tmp, name_len)) err = 0;
        else if (was_ok && name_len != field<u32>(zip, kZipNameLen)) err = kBadZip;
    }
    store<u32>(header_extra, load<u32>(header_extra) + name_len);
    u32 extra_len;
    if (!read_u16(ctx, zip, tmp, extra_len)) err = 0;
    store<u32>(extra_offset, field<u32>(zip, kZipLocalOffset) + name_len + kLocalHeaderSize);
    store<u32>(extra_size, extra_len);
    store<u32>(header_extra, load<u32>(header_extra) + extra_len);
    ret(ctx, static_cast<s8>(err));
}
