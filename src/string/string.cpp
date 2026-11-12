// Engine string helpers. Both are called from all over the game, so they
// work on guest memory directly instead of calling back into the C library
// where that is behaviour-identical.
#include "opentit/native.h"
#include "str.h"

namespace sly4::str {
using namespace opentit::native;

// strncpy(dst, src, size - 1), then dst[size - 1] = 0: copies at most
// size - 1 characters, zero-pads the rest and always terminates.
char* String_Copy(char* dst, const char* src, u32 size) {
    const u32 n = size - 1;
    if (n != 0) {
        // strncpy copies 8 bytes at a time when it can, so overlapping
        // buffers and size 0 (n = 0xFFFFFFFF) are left to the original.
        const u64 d = guest(dst), s = guest(src);
        if (d + n <= s || s + n <= d) {
            u32 k = 0;
            for (; k < n && src[k] != 0; ++k) dst[k] = src[k];
            for (; k < n; ++k) dst[k] = 0;
        } else {
            call(libc::strncpy, dst, src, n);
        }
    }
    *host<char>(guest(dst) + size - 1) = 0;  // 32-bit address arithmetic
    return dst;
}

// h = h * 31 + c over the string with a-z upper-cased (chars are signed);
// 0 for a null pointer.
u32 String_HashNoCase(const char* s) {
    if (!s) return 0;
    u32 h = 0;
    for (const char* p = s; *p != 0; ++p) {  // the original loops to strlen(s)
        s32 c = static_cast<s8>(*p);
        if (c >= 'a' && c <= 'z') c -= 'a' - 'A';
        h = h * 31 + static_cast<u32>(c);
    }
    return h;
}

}  // namespace sly4::str

NATIVE(00553684, sly4::str::String_Copy)
NATIVE(0056A710, sly4::str::String_HashNoCase)
