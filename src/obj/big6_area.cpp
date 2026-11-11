// Game code, large-function sweep part 6: the kind-A area settings block
// (its constructor and how it is pushed onto an animator layer's list).
#include "big6.h"

using namespace sly4;
using namespace sly4::big6;

namespace {
// Layer: the settings blocks pushed onto it.
constexpr u32 kLayerCount = 536;  // s32
constexpr u32 kLayerList = 540;   // Settings* [], kLayerCount of them
// Settings block: u8 flag (also written by the push).
constexpr u32 kSetFlag = 120;

// AreaSettingsA_Construct: {source word, block offset} in store order. The
// sources are the value fields (+72, one at +80 / +4 / +8) of 76-byte
// tuning records in the 0x0103xxxx data.
struct Init {
    u32 src, off;
};
constexpr Init kInitA[] = {
    {0x01032418u, 0},  {0x010324B0u, 4},  {0x010324FCu, 8},  {0x01032550u, 16}, {0x01032554u, 20},
    {0x01032558u, 24}, {0x010325A8u, 32}, {0x010325F4u, 36}, {0x01032640u, 40}, {0x0103268Cu, 44},
    {0x010326D8u, 48}, {0x010327BCu, 52},
};
constexpr Init kInitB[] = {
    {0x01032808u, 56},
};
constexpr Init kInitC[] = {
    {0x01032724u, 60}, {0x01032770u, 64}, {0x01032854u, 68}, {0x010328A0u, 72}, {0x010328ECu, 76},
    {0x01032938u, 80}, {0x01032984u, 84}, {0x010329D0u, 88}, {0x01032A1Cu, 92}, {0x01032FE0u, 100},
    {0x0103302Cu, 104},
};
constexpr Init kInitD[] = {
    {0x01033078u, 108}, {0x010330C4u, 112}, {0x01033110u, 116},
};
}  // namespace

// void AreaSettingsA_Construct(SettingsA* b)
// The 128-byte kind-A settings block: 29 words copied (raw) from tuning
// values, +124 = 0, flag +120 = 0. Leaves f1 = the last value loaded (lfs),
// r4..r7 as the original's address loads leave them (no caller reads them).
DECOMP(000FD7AC, AreaSettingsA_Construct) {
    const u32 b = arg<u32>(ctx, 0);
    for (const Init& i : kInitA) CopyW(b + i.off, i.src);
    store<u32>(b + 124, 0);
    for (const Init& i : kInitB) CopyW(b + i.off, i.src);
    store<u8>(b + kSetFlag, 0);
    for (const Init& i : kInitC) CopyW(b + i.off, i.src);
    const u32 w96 = load<u32>(0x01035410u);  // lwz, loaded before the +108 source
    const u32 w108 = load<u32>(kInitD[0].src);
    store<u32>(b + 96, w96);
    store<u32>(b + 108, w108);
    for (u32 k = 1; k < 3; ++k) CopyW(b + kInitD[k].off, kInitD[k].src);
    SetF1FromWord(ctx, 0x01033110u);
    ctx.gpr[4] = 0x010330C8u;
    ctx.gpr[5] = 0x01033030u;
    ctx.gpr[6] = w96;
    ctx.gpr[7] = 0x0103307Cu;
}

// void AnimLayer_ApplySettingsA(Layer* l, Settings* s, u8 top)
// Stores `top` in s+120 and adds s to the layer's list (+536 count, +540
// array): appended when top is set, the list is empty or the last entry's
// flag is clear; otherwise s goes in after the last entry whose flag is
// clear, the (flagged) entries above it moved up one. Quirk (kept): when every entry has its flag set,
// they are all moved up one and s is not added (count unchanged); a
// negative count adds nothing. r3 at exit: l + 4 * count(old) + 540 after
// an append, else l.
DECOMP(000FEF54, AnimLayer_ApplySettingsA) {
    const u32 l = arg<u32>(ctx, 0), s = arg<u32>(ctx, 1);
    const u8 top = arg<u8>(ctx, 2);
    store<u8>(s + kSetFlag, top);
    const s32 n = load<s32>(l + kLayerCount);
    bool append = n == 0 || top != 0;
    if (!append) append = load<u8>(load<u32>(l + kLayerList + 4 * static_cast<u32>(n - 1)) + kSetFlag) == 0;
    if (append) {
        const u32 cnt = load<u32>(l + kLayerCount);
        store<u32>(l + kLayerCount, cnt + 1);
        const u32 slot = l + 4 * cnt + kLayerList;
        store<u32>(slot, s);
        ctx.gpr[3] = slot;
        return;
    }
    for (s32 i = n; i > 0; --i) {
        const u32 prev = load<u32>(l + 4 * static_cast<u32>(i) + kLayerCount);  // list[i - 1]
        if (load<u8>(prev + kSetFlag) == 0) {
            const u32 cnt = load<u32>(l + kLayerCount);  // loaded before the store, as in the original
            store<u32>(l + 4 * static_cast<u32>(i) + kLayerList, s);
            store<u32>(l + kLayerCount, cnt + 1);
            return;
        }
        store<u32>(l + 4 * static_cast<u32>(i) + kLayerList, prev);
    }
}
