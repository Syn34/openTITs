// File helpers on top of the game's file system layer (archives + disc).
#include "../sly4.h"

namespace sly4::file {
using namespace opentit::native;

namespace {

// The 2-bit counter in bits 5-6 of g::file_flags wraps like the original.
void adjust_file_depth(int delta) {
    u8& flags = global<u8>(g::file_flags);
    const u8 depth = static_cast<u8>(((flags >> 5) + delta) & 3);
    flags = static_cast<u8>((flags & ~0x60) | (depth << 5));
}

constexpr u32 kModeRead = 0x00AF37C0;  // "rb"
constexpr u32 kVfnClose = 8;           // virtual slot 2: close and free

}  // namespace

u32 File_Open(u32 path, u32 mode);  // open.cpp

// bool File_Exists(const char* path)
// Opens the file for reading and closes it again. main() uses it to check
// for Sly4.psarc before mounting the game archive.
bool File_Exists(u32 path) {
    adjust_file_depth(+1);
    const u32 file = File_Open(path, kModeRead);
    if (file != 0) vcall(host<be_u32>(file), kVfnClose);
    adjust_file_depth(-1);
    return file != 0;
}

}  // namespace sly4::file

NATIVE(0055D78C, sly4::file::File_Exists)
