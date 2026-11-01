// File helpers on top of the game's file system layer (archives + disc).
#include "../sly4.h"

using namespace sly4;

namespace {

// The 2-bit counter in bits 5-6 of g::file_flags wraps like the original.
void adjust_file_depth(int delta) {
    u8& flags = ref<u8>(g::file_flags);
    const u8 depth = static_cast<u8>(((flags >> 5) + delta) & 3);
    flags = static_cast<u8>((flags & ~0x60) | (depth << 5));
}

constexpr u32 kModeRead = 0x00AF37C0;  // "rb"

}  // namespace

// bool File_Exists(const char* path)
// Opens the file for reading and closes it again. main() uses it to check
// for Sly4.psarc before mounting the game archive.
DECOMP(0055D78C, File_Exists) {
    const u32 path = arg<u32>(ctx, 0);
    adjust_file_depth(+1);
    const u32 file = static_cast<u32>(call(ctx, fn::File_Open, {path, kModeRead}));
    if (file != 0) {
        const u32 vtable = load<u32>(file);
        call_ptr(ctx, load<u32>(vtable + 8), {file});  // virtual slot 2: close and free
    }
    adjust_file_depth(-1);
    ret(ctx, file != 0 ? 1u : 0u);
}
