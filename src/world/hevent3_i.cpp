// World_HandleEvent callees, part 3: 90..97 instructions.
#include "hevent.h"

using namespace sly4;
using namespace sly4::hevent;

namespace sly4 {
namespace hevent3i {
namespace ifn {
constexpr u32 sub_002D9A14 = 0x002D9A14;          // (u32)
constexpr u32 sub_002DDAB0 = 0x002DDAB0;          // (Session*)
constexpr u32 Set_Find = 0x004BCE90;              // (u32 name) -> Set*
constexpr u32 sub_004BD1CC = 0x004BD1CC;          // (Set*)
constexpr u32 UserBlock_Reset45C7A8 = 0x0045C7A8; // (UserBlock*) (name?)
constexpr u32 sub_0042AEE8 = 0x0042AEE8;          // (Entry16*)
constexpr u32 Nop = 0x0054EC18;
constexpr u32 SmallAlloc = 0x0055238C;
constexpr u32 SmallFree = 0x00552420;             // (ptr, size)
constexpr u32 Mem_FreeFrom = 0x0054EE6C;          // (p, pool)
constexpr u32 Mem_Alloc = 0x0054ED38;             // (size, pool, align)
constexpr u32 PtrVector_Construct = 0x005516E0;
constexpr u32 PtrVector_Init = 0x00551598;        // (PtrVector*, capacity, 0, 0)
constexpr u32 PtrVector_Reserve = 0x00551718;     // (PtrVector*, capacity)
constexpr u32 LayerStack_Clear = 0x0042B0C8;      // (hevent3_i.cpp)
}  // namespace ifn

constexpr u32 kSetName7719A548 = 0x7719A548;

// The layer stack (name?): 0x00F65858 a block of 16-byte entries (count at
// 0x010AA690), 0x010AA694 a PtrVector of 36-byte layers {+0, +4 PtrVector of
// objects (+12 u16 count), +24 flags, +32}, 0x00F6585C the first layer.
constexpr u32 kLayerEntries = 0x00F65858, kLayerTop = 0x00F6585C;
constexpr u32 kLayerEntryCount = 0x010AA690;
constexpr u32 kLayers = 0x010AA694;               // +0 data, +4 u16 capacity, +8 u16 count, +10 u16 high water
constexpr u32 kLayerSize = 36;
constexpr u32 kLayerEntriesBytes = 768;
}  // namespace hevent3i
}  // namespace sly4

using namespace sly4::hevent3i;

// u32 Session_Reset(Session* s): with +16 bit 0x20000000: sub_002D9A14(+8)
// when set, +0..+16 cleared, sub_002DDAB0(s) twice, sub_004BD1CC(Set
// 0x7719A548), +40 = +72 = +48 = 0, UserBlock_Reset45C7A8 on each user block,
// +36 = 0; returns 0. Otherwise, with +40 set, vfunc +28(obj, 1) on every object
// of user block 0's list (+80, +88 u16 count); returns the count (name?)
DECOMP(002E4D64, Session_Reset) {
    const u32 s = arg<u32>(ctx, 0);
    if ((load<u32>(s + 16) & 0x20000000u) != 0) {
        const u32 a = load<u32>(s + 8);
        if (a != 0) call(ctx, ifn::sub_002D9A14, {a});
        for (u32 k = 0; k < 20; k += 4) store<u32>(s + k, 0);
        call(ctx, ifn::sub_002DDAB0, {s});
        call(ctx, ifn::sub_002DDAB0, {s});
        const u32 set = static_cast<u32>(call(ctx, ifn::Set_Find, {kSetName7719A548}));
        if (set != 0) call(ctx, ifn::sub_004BD1CC, {set});
        store<u32>(s + 40, 0);
        store<u32>(s + 72, 0);
        store<u32>(s + 48, 0);
        for (u32 u = 0; u < load<u8>(load<u32>(kCurrentWorld) + 29); ++u)
            call(ctx, ifn::UserBlock_Reset45C7A8, {load<u32>(kCurrentWorld) + u * kUserBlockSize + kUserBlocks});
        store<u32>(s + 36, 0);
        return ret(ctx, 0u);
    }
    if (load<u32>(s + 40) == 0) return ret(ctx, 0u);
    const u32 blk = load<u32>(kCurrentWorld) + kUserBlocks;
    u32 i = 0;
    for (; i < load<u16>(blk + 88); ++i) {
        const u32 e = load<u32>(load<u32>(blk + 80) + i * 4);
        vcall(ctx, e, 28, {e, 1});
    }
    ret(ctx, load<u16>(blk + 88));
}

// u16 LayerStack_Clear(): sub_0042AEE8 on each 16-byte entry at *0x00F65858
// (count 0x010AA690), then every layer of 0x010AA694 has its objects deleted
// (vfunc +4), its vector cleared and is freed (36 bytes); the layer count = 0 (name?)
DECOMP(0042B0C8, LayerStack_Clear) {
    if (load<u32>(kLayerEntryCount) != 0)
        for (u32 i = 0; i < load<u32>(kLayerEntryCount); ++i)
            call(ctx, ifn::sub_0042AEE8, {load<u32>(kLayerEntries) + i * 16});
    u32 n = 0;
    for (u32 j = 0; j < (n = load<u16>(kLayers + 8)); ++j) {
        const u32 l = load<u32>(load<u32>(kLayers) + j * 4);
        if (l == 0) continue;
        for (u32 k = 0; k < load<u16>(l + 12); ++k) {
            const u32 o = load<u32>(load<u32>(l + 4) + k * 4);
            if (o != 0) vcall(ctx, o, 4, {o});
        }
        store<u16>(l + 12, 0);
        call(ctx, hfn::PtrVector_Clear, {l + 4});
        call(ctx, ifn::Nop, {l + 4});
        call(ctx, ifn::SmallFree, {l, kLayerSize});
    }
    store<u16>(kLayers + 8, 0);
    ret(ctx, n);
}

// Layer* LayerStack_Reset(): LayerStack_Clear(), frees and reallocates the
// entry block (768 bytes, align 16), and pushes one new layer (vector of 32,
// +24 = 0x40000000 with bits 31 / 29 clear, +32 = 0) which becomes the top
// (0x00F6585C) with +24 bit 31 set. Quirk: a failed SmallAlloc pushes 0 and the
// original still sets bit 31 of whatever the first layer is (name?)
DECOMP(0042B29C, LayerStack_Reset) {
    call(ctx, ifn::LayerStack_Clear);
    const u32 old = load<u32>(kLayerEntries);
    if (old != 0) {
        call(ctx, ifn::Mem_FreeFrom, {old, 0});
        store<u32>(kLayerEntries, 0);
    }
    store<u32>(kLayerEntryCount, 0);
    store<u32>(kLayerTop, 0);
    const u32 buf = static_cast<u32>(call(ctx, ifn::Mem_Alloc, {kLayerEntriesBytes, 0, 16}));
    store<u32>(kLayerEntryCount, 0);
    store<u32>(kLayerEntries, buf);
    u32 l = static_cast<u32>(call(ctx, ifn::SmallAlloc, {kLayerSize}));
    if (l != 0) {
        store<u32>(l + 0, 0);
        call(ctx, ifn::PtrVector_Construct, {l + 4});
        store<u16>(l + 10, 4);
        call(ctx, ifn::PtrVector_Init, {l + 4, 32, 0, 0});
        const u32 f = (((load<u32>(l + 24) & 0x7FFFFFFFu) | 0x40000000u) & ~0x20000000u);
        store<u32>(l + 32, 0);
        store<u32>(l + 24, f);
    }
    u32 n = load<u16>(kLayers + 8);
    const u32 cap = load<u16>(kLayers + 4);
    if (n == cap) {
        call(ctx, ifn::PtrVector_Reserve, {kLayers, (cap << 1) & 0x1FFFEu});
        n = load<u16>(kLayers + 8);
    }
    const u32 data = load<u32>(kLayers);
    const u32 cnt = load<u16>(kLayers + 8);
    const u32 high = load<u16>(kLayers + 10);
    store<u32>(data + ((n << 2) & 0x3FFFCu), l);
    u32 c1 = cnt + 1;
    const u32 data2 = load<u32>(kLayers);
    store<u16>(kLayers + 8, static_cast<u16>(c1));
    if (static_cast<s32>(high) > static_cast<s32>(c1 & 0xFFFF)) c1 = high;
    const u32 top = load<u32>(data2);
    store<u16>(kLayers + 10, static_cast<u16>(c1));
    store<u32>(kLayerTop, top);
    store<u32>(top + 24, load<u32>(top + 24) | 0x80000000u);
    ret(ctx, top);
}
