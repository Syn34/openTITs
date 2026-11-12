// Thin wrappers around the C library used by the XML reader and the tuning
// variables.
#include "opentit/native.h"
#include "str.h"

namespace sly4::str {
using namespace opentit::native;

namespace {
constexpr u32 kStrcmp = 0x00723884;    // strcmp(a, b)
constexpr u32 kMemcmp = 0x00723AC4;    // memcmp(a, b, n)
constexpr u32 kVsprintf = 0x00724588;  // vsprintf(dst, fmt, va) (src/game/load_util.cpp)
}  // namespace

// strcmp.
s32 String_Compare(const char* a, const char* b) { return call<s32>(kStrcmp, a, b); }

s32 String_Length(const char* s) { return static_cast<s32>(call<u32>(libc::strlen, s)); }

// sprintf: spills the variadic arguments to the caller's parameter save
// area (fmt's slot at +56, the rest from +64), as the original does, and
// passes that area to vsprintf as the va_list. Arguments past the eighth are
// already in the caller's save area, so this needs the guest caller's r1:
// the function must be reached through the PPU ABI (native callers use
// call<s32>(0x00551540, ...)).
s32 String_Printf(char* dst, const char* fmt, u64 a2, u64 a3, u64 a4, u64 a5, u64 a6, u64 a7) {
    const u32 sp = static_cast<u32>(ctx().gpr[1]);  // the caller's stack pointer
    *host<be_u32>(sp + 60) = guest(fmt);
    be_u64* va = host<be_u64>(sp + 64);
    const u64 rest[6] = {a2, a3, a4, a5, a6, a7};
    for (u32 k = 0; k < 6; ++k) va[k] = rest[k];
    return call<s32>(kVsprintf, dst, fmt, va);
}

// memcmp.
s32 Mem_Compare(const void* a, const void* b, u32 n) { return call<s32>(kMemcmp, a, b, n); }

}  // namespace sly4::str

NATIVE(00555D94, sly4::str::String_Compare)
NATIVE(00555FA4, sly4::str::String_Length)
NATIVE(00551540, sly4::str::String_Printf)
NATIVE(00555DB8, sly4::str::Mem_Compare)
