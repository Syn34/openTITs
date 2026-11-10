// Game code, medium sweep part 6: a static initializer (no direct callers)
// setting the vtable word of 281 global 24-byte objects.
#include "mid6.h"

using namespace sly4;
using namespace sly4::mid6;

namespace {

constexpr u32 kVtStaticObj = 0x00B0DBF8;  // 0x00B0DBF0 + 8

// The objects as runs {first, count} of 24-byte stride.
struct Run {
    u32 first, count;
};
constexpr Run kRuns[] = {
    {0x010AADB4, 24}, {0x010AAFF8, 44}, {0x010AB420, 1}, {0x010AB460, 1}, {0x010AB4A0, 1},
    {0x010AB4E0, 1},  {0x010AB520, 2},  {0x010AB558, 1}, {0x010AB578, 206},
};

}  // namespace

// void Statics_InitVt_0043E514(): *obj = 0x00B0DBF8 for each of the
// objects (the original stores them in a different, unrolled order).
DECOMP(0043E514, Statics_InitVt_0043E514) {
    for (const Run& r : kRuns)
        for (u32 k = 0; k < r.count; ++k) store<u32>(r.first + 24 * k, kVtStaticObj);
    ctx.gpr[3] = 0x010B0000;    // leftover: the last lis
    ctx.gpr[4] = kVtStaticObj;  // leftover
}
