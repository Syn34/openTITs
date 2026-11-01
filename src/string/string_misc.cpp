// Thin wrappers around the C library used by the XML reader and the tuning
// variables.
#include "str.h"

using namespace sly4;

namespace {
constexpr u32 kStrcpy = 0x00723884;    // strcpy(dst, src)
constexpr u32 kMemcmp = 0x00723AC4;    // memcmp(a, b, n)
constexpr u32 kVsprintf = 0x00724588;  // vsprintf(dst, fmt, va) (src/game/load_util.cpp)
}  // namespace

// char* String_Assign(char* dst, const char* src)
// strcpy (the String member Tuning_Load assigns names with).
DECOMP(00555D94, String_Assign) {
    ret(ctx, call(ctx, kStrcpy, {arg<u32>(ctx, 0), arg<u32>(ctx, 1)}));
}

// s32 String_Length(const char* s)
DECOMP(00555FA4, String_Length) {
    ret(ctx, static_cast<s32>(static_cast<u32>(call(ctx, libc::strlen, {arg<u32>(ctx, 0)}))));
}

// int String_Printf(char* dst, const char* fmt, ...)
// sprintf: spills the variadic arguments to the caller's parameter save
// area (fmt's slot at +56, the rest from +64), as the original does.
DECOMP(00551540, String_Printf) {
    const u32 sp = static_cast<u32>(ctx.gpr[1]);
    store<u32>(sp + 60, static_cast<u32>(ctx.gpr[4]));
    for (u32 k = 0; k < 6; ++k) store<u64>(sp + 64 + k * 8, ctx.gpr[5 + k]);
    ret(ctx, call(ctx, kVsprintf, {arg<u32>(ctx, 0), arg<u32>(ctx, 1), sp + 64}));
}

// int Mem_Compare(const void* a, const void* b, u32 n)
// memcmp.
DECOMP(00555DB8, Mem_Compare) {
    ret(ctx, call(ctx, kMemcmp, {arg<u32>(ctx, 0), arg<u32>(ctx, 1), arg<u32>(ctx, 2)}));
}
