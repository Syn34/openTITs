// Zip archives: opening one and finding an entry by path.
#include "zip.h"

using namespace sly4;
namespace fs = sly4::files;
namespace zp = sly4::files::zip;

namespace {
constexpr u32 kStrSca = 0x00AF37CC;  // ".sca"
}  // namespace

// s32 Archive_Find(ZipArchive* archive, const char* path, int mode)
// Makes `path` the archive's current entry. Returns 1 if found (or already
// current), 0 if the path is 256 characters or longer, -100 if not found
// or the archive has no usable entry. mode 1 compares exactly, anything
// else ignores ASCII case.
DECOMP(0055B3C8, Archive_Find) {
    const u32 archive = arg<u32>(ctx, 0);
    const u32 path = arg<u32>(ctx, 1);
    const s32 mode = arg<s32>(ctx, 2);

    if (static_cast<s32>(call(ctx, fs::libc::strlen, {path})) >= zp::kMaxPath) return ret(ctx, 0);
    if (field<u32>(archive, zp::kEntryValid) == 0) return ret(ctx, zp::kNotFound);
    if (ref<u8>(archive + zp::kCurrentName) != 0 &&
        fs::name_compare(ctx, archive + zp::kCurrentName, path, mode) == 0)
        return ret(ctx, 1);

    const u32 name = fs::normalise_path(ctx, path);
    const u32 saved_index = field<u32>(archive, zp::kEntryIndex);
    const u32 saved_offset = field<u32>(archive, zp::kEntryOffset);
    const u32 hash = fs::name_hash(name);
    for (u32 entry = load<u32>(archive + zp::kBuckets + ((hash << 2) & zp::kBucketMask)); entry != 0;
         entry = load<u32>(entry + zp::kEntryNext)) {
        if (fs::name_compare(ctx, name, entry + zp::kEntryName, mode) != 0) continue;
        field<u32>(archive, zp::kEntryOffset) = load<u32>(entry);
        call(ctx, fs::fn::Zip_LocateEntry, {archive});
        if (field<u32>(archive, zp::kEntryValid) != 0) return ret(ctx, 1);
        ref<u8>(archive + zp::kCurrentName) = 0;
        break;
    }
    field<u32>(archive, zp::kEntryIndex) = saved_index;
    field<u32>(archive, zp::kEntryOffset) = saved_offset;
    ret(ctx, zp::kNotFound);
}

// bool ZipArchive_Open(ZipArchive* zip, const char* path, const char* prefix)
// Opens the .zip, reads its central directory (".sca" in the name selects
// a variant), optionally sets a path prefix, selects the first entry and
// builds the name index. On failure the file is closed (but its pointer
// is left in the object).
DECOMP(0055D494, ZipArchive_Open) {
    const u32 zip = arg<u32>(ctx, 0);
    const u32 path = arg<u32>(ctx, 1);
    const u32 prefix = arg<u32>(ctx, 2);

    Stack frame(ctx, 8);
    const u32 end_out = frame.addr(), ok_out = frame.addr() + 4;
    store<u32>(end_out, 0);
    store<s32>(ok_out, 1);
    const u32 sca = static_cast<u32>(call(ctx, fs::libc::strstr, {path, kStrSca}));
    field<u32>(zip, zp::kFile) = static_cast<u32>(call(ctx, sly4::fn::File_Open, {path, fs::kStrModeRead}));
    ref<u8>(zip + zp::kFlag1184) = 0;
    call(ctx, fs::fn::Zip_ReadDirectory, {zip, sca, ok_out, end_out});
    if (load<s32>(ok_out) != 1) {
        const u32 file = field<u32>(zip, zp::kFile);
        if (file != 0) fs::file_vcall(ctx, file, fs::kFileClose, {file});
        return ret(ctx, 0u);
    }

    const u32 end = load<u32>(end_out);
    field<u32>(zip, zp::kEnd) = end;
    field<u32>(zip, zp::kStream) = 0;
    field<u32>(zip, zp::kDataBase) =
        end - (field<u32>(zip, zp::kDirSize) + field<u32>(zip, zp::kDirOffset));
    if (prefix == 0) {
        ref<u8>(zip + zp::kPrefix) = 0;
    } else {
        call(ctx, fs::libc::strcpy, {zip + zp::kPrefix, fs::normalise_path(ctx, prefix)});
    }
    field<u32>(zip, zp::kEntryOffset) = field<u32>(zip, zp::kDirSize);
    field<u32>(zip, zp::kEntryIndex) = 0;
    call(ctx, fs::fn::Zip_LocateEntry, {zip});
    if (field<u32>(zip, zp::kEntryValid) == 0) ref<u8>(zip + zp::kCurrentName) = 0;
    call(ctx, fs::fn::Zip_BuildIndex, {zip});
    ret(ctx, 1u);
}
