// World_HandleEvent callees, part 21h: the model block pool teardown (the
// slot rings of Ring_Reset31E040 and their owners).
#include "hevent19.h"

using namespace sly4;
using namespace sly4::hevent;

namespace {
constexpr u32 kPtrVectorClear = 0x0054EC1C;  // (PtrVector*) (as hevent.h)
constexpr u32 kNop = 0x0054EC18;             // (x) (symbols.txt Nop)
constexpr u32 kSmallFree = 0x00552420;       // (p, size) (as hevent7.h)
constexpr u32 kRingReset31E040 = 0x0031E040; // (Ring*) (hevent21_g.cpp)
constexpr u32 kGroupReset31E890 = 0x0031E890;  // (Group*) (hevent21_h.cpp)
}  // namespace

// u32 Group_Reset31E890(Group* g) (name?): the ring (+20 begin, +28 head,
// +32 cursor, +40 count) is reset to empty (the original first walks the
// cursor over the pending slots without touching memory; that walk is
// left out). Each block of the +0 vector (u16 +8 count) releases its
// objects (vt +8), has its +20 and own vectors cleared (PtrVector_Clear,
// Nop) and is freed (SmallFree 44). Then the count, head / cursor and
// ring count are reset again. Returns the ring begin.
DECOMP(0031E890, Group_Reset31E890) {
    const u32 g = arg<u32>(ctx, 0);
    const u32 begin = load<u32>(g + 20);
    store<u32>(g + 28, begin);
    store<u32>(g + 32, begin);
    store<u32>(g + 40, 0);
    for (u32 i = 0; i < load<u16>(g + 8); ++i) {
        const u32 b = load<u32>(load<u32>(g) + 4 * i);
        if (b == 0) continue;
        for (u32 j = 0; j < load<u16>(b + 8); ++j) {
            const u32 x = load<u32>(load<u32>(b) + 4 * j);
            if (x != 0) vcall(ctx, x, 8, {x});
        }
        call(ctx, kPtrVectorClear, {b + 20});
        call(ctx, kNop, {b + 20});
        call(ctx, kPtrVectorClear, {b});
        call(ctx, kNop, {b});
        call(ctx, kSmallFree, {b, 44});
    }
    const u32 b2 = load<u32>(g + 20);
    store<u16>(g + 8, 0);
    store<u32>(g + 28, b2);
    store<u32>(g + 32, b2);
    store<u32>(g + 40, 0);
    ret(ctx, b2);
}

// u32 Model_FreeBlocks31FEE8(Pool* p) (name?): the groups of the {key,
// Vector*} table +32 (u16 +26 count): each group (144 bytes) is reset
// (Group_Reset31E890, also for a null entry, as in the original) and its
// +20 array (+36 words) and itself are freed; the vector is emptied and
// freed (24 bytes). Then the rings of the {key, Ring*} table +8 (u16 +2
// count) are reset (Ring_Reset31E040) and freed with their slot arrays
// (+0, +16 words; ring 32 bytes). Both counts end at 0. Returns the last
// count read.
DECOMP(0031FEE8, Model_FreeBlocks31FEE8) {
    const u32 p = arg<u32>(ctx, 0);
    for (u32 i = 0; static_cast<s32>(i) < static_cast<s32>(load<u16>(p + 26)); ++i) {
        const u32 v = load<u32>(load<u32>(p + 32) + 8 * i + 4);
        if (v == 0) continue;
        for (u32 j = 0; j < load<u16>(v + 8); ++j) {
            const u32 grp = load<u32>(load<u32>(v) + 4 * j);
            call(ctx, kGroupReset31E890, {grp});
            if (grp == 0) continue;
            const u32 words = load<u32>(grp + 36);
            call(ctx, kSmallFree, {load<u32>(grp + 20), words << 2});
            call(ctx, kPtrVectorClear, {grp});
            call(ctx, kNop, {grp});
            call(ctx, kSmallFree, {grp, 144});
        }
        store<u16>(v + 8, 0);
        call(ctx, kPtrVectorClear, {v});
        call(ctx, kNop, {v});
        call(ctx, kSmallFree, {v, 24});
    }
    store<u16>(p + 26, 0);
    u32 n;
    for (u32 k = 0; static_cast<s32>(k) < static_cast<s32>(n = load<u16>(p + 2)); ++k) {
        const u32 r = load<u32>(load<u32>(p + 8) + 8 * k + 4);
        if (r == 0) continue;
        call(ctx, kRingReset31E040, {r});
        const u32 words = load<u32>(r + 16);
        call(ctx, kSmallFree, {load<u32>(r), words << 2});
        call(ctx, kSmallFree, {r, 32});
    }
    store<u16>(p + 2, 0);
    ret(ctx, n);
}
