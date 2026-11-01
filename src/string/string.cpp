// Engine string helpers. Both are called from all over the game, so they
// work on guest memory directly instead of calling back into the C library
// where that is behaviour-identical.
#include "str.h"

using namespace sly4;

// char* String_Copy(char* dst, const char* src, u32 size)
// strncpy(dst, src, size - 1), then dst[size - 1] = 0: copies at most
// size - 1 characters, zero-pads the rest and always terminates.
DECOMP(00553684, String_Copy) {
    const u32 dst = arg<u32>(ctx, 0);
    const u32 src = arg<u32>(ctx, 1);
    const u32 size = arg<u32>(ctx, 2);
    const u32 n = size - 1;
    if (n != 0) {
        // strncpy copies 8 bytes at a time when it can, so overlapping
        // buffers and size 0 (n = 0xFFFFFFFF) are left to the original.
        const u64 d = dst, s = src;
        if (d + n <= s || s + n <= d) {
            u8* out = mem::ptr(dst);
            const u8* in = mem::ptr(src);
            u32 k = 0;
            for (; k < n && in[k] != 0; ++k) out[k] = in[k];
            for (; k < n; ++k) out[k] = 0;
        } else {
            call(ctx, libc::strncpy, {dst, src, n});
        }
    }
    ref<u8>(dst + size - 1) = 0;
    ret(ctx, dst);
}

// u32 String_HashNoCase(const char* s)
// h = h * 31 + c over the string with a-z upper-cased (chars are signed);
// 0 for a null pointer.
DECOMP(0056A710, String_HashNoCase) {
    const u32 s = arg<u32>(ctx, 0);
    if (s == 0) return ret(ctx, 0u);
    const u8* p = mem::ptr(s);
    u32 h = 0;
    for (; *p != 0; ++p) {  // the original loops to strlen(s)
        s32 c = static_cast<s8>(*p);
        if (c >= 'a' && c <= 'z') c -= 'a' - 'A';
        h = h * 31 + static_cast<u32>(c);
    }
    ret(ctx, h);
}
