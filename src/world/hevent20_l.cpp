// World_HandleEvent callees, part 20l: the level map / file op teardown and
// the overlay setup.
#include "hevent19.h"

using namespace sly4;
using namespace sly4::hevent;

namespace {
constexpr u32 kMemFreeFrom = 0x0054EE6C;     // (p, pool) (symbols.txt Mem_FreeFrom)
constexpr u32 kFsOpDestruct = 0x00570DA8;    // (FsOp*) (symbols.txt FsOp_Destruct)
constexpr u32 kMemset = 0x00726E54;          // (dst, value, size) (symbols.txt memset)
constexpr u32 kMap = 0x010686A8;             // {+2 u16 count, +8 {key, Object*}[], +24 u32 count}
constexpr u32 kBlocks = 0x0106870C;          // PtrVector of blocks
constexpr u32 kFsOp = 0x00F64C34;            // u32 FsOp*
constexpr u32 kOverlay = 0x010AA1B0;         // u8 flags, +4 u32, +88 / +92 / +94, +96 [304], +484 40-byte slots
constexpr u32 kOverlaySetup428B74 = 0x00428B74;  // (u32 32, u32 18) (name?)
constexpr u32 kWorldPartObj = 0x128164;      // World +: Object* (+5564 u8 bit 0, +5568 u32 hash)
constexpr u32 kPlainHash = 0x394E2CB4;
}  // namespace

// void Level_FreeMaps2E98CC() (name?): the map at 0x010686A8's objects get
// vt +4 (delete), its counts cleared; every block of the vector at
// 0x0106870C popped from the back and freed; the file op at 0x00F64C34
// destructed and freed.
DECOMP(002E98CC, Level_FreeMaps2E98CC) {
    for (s32 i = 0; i < static_cast<s32>(load<u16>(kMap + 2)); ++i) {
        const u32 obj = load<u32>(load<u32>(kMap + 8) + 8 * i + 4);
        if (obj != 0) vcall(ctx, obj, 4, {obj});
    }
    store<u16>(kMap + 2, 0);
    store<u32>(kMap + 24, 0);
    for (u32 n; (n = load<u16>(kBlocks + 8)) != 0;) {
        const u32 last = n - 1;
        const u32 data = load<u32>(kBlocks);
        store<u16>(kBlocks + 8, static_cast<u16>(last));
        const u32 e = load<u32>(data + 4 * last);
        // The inlined swap-remove would move the new last element into the
        // slot; the slot is the last one, so it never runs here.
        if (e != 0) call(ctx, kMemFreeFrom, {e, 0});
    }
    const u32 fs = load<u32>(kFsOp);
    if (fs == 0) {
        ret(ctx, 0);
        return;
    }
    call(ctx, kFsOpDestruct, {fs});
    call(ctx, kMemFreeFrom, {fs, 0});
    store<u32>(kFsOp, 0);
    ret(ctx, kFsOp & 0xFFFF0000u);  // lis r3, 0xF6 (left)
}

// void Overlay_Setup428F34() (name?): overlay flags: 0x20 / 0x80 / 0x10
// cleared, 0x40 set, +4 = 1; with the world's part object: 0x80 = its +5564
// bit 0 and +4 = (its +5568 != 0x394E2CB4). When 0x80 is set (and 0x20
// clear): +96 zeroed (304 bytes), +88 bits 0..16 = {0, 770}, +92 = 0x8006,
// +94 = 771, 0x00428B74(32, 18), and the 16 slots' (+484, 40 bytes) +32
// bytes cleared.
DECOMP(00428F34, Overlay_Setup428F34) {
    const u32 p = load<u32>(load<u32>(kCurrentWorld) + kWorldPartObj);
    u32 b = ((load<u8>(kOverlay) & ~0xA0u) | 0x40u) & ~0x10u;
    store<u8>(kOverlay, static_cast<u8>(b));
    store<u32>(kOverlay + 4, 1);
    if (p != 0) {
        b = (b & ~0x80u) | ((load<u8>(p + 5564) & 1u) << 7);
        store<u8>(kOverlay, static_cast<u8>(b));
        store<u32>(kOverlay + 4, load<u32>(p + 5568) == kPlainHash ? 0 : 1);
    }
    if ((b & 0x80) == 0 || (b & 0x20) != 0) {
        ctx.gpr[4] = (b >> 7) & 1;
        ret(ctx, b);
        return;
    }
    call(ctx, kMemset, {kOverlay + 96, 0, 304});
    const u32 w88 = (load<u32>(kOverlay + 88) & 0x00007FFFu) | (770u << 15);
    store<u16>(kOverlay + 92, 0x8006);
    store<u32>(kOverlay + 88, w88);
    store<u16>(kOverlay + 94, 771);
    call(ctx, kOverlaySetup428B74, {32, 18, 0x8006, 771, w88});
    for (u32 i = 0; i < 16; ++i) store<u8>(kOverlay + 484 + 40 * i + 32, 0);
    ret(ctx, 16);
}

namespace {
constexpr u32 kMemAlloc = 0x0054ED38;        // (size, 0, align) (symbols.txt Mem_Alloc)
constexpr u32 kSmallAlloc = 0x0055238C;      // (size) -> block or 0 (symbols.txt SmallAlloc)
constexpr u32 kPtrVectorConstruct = 0x005516E0;  // (vec) (symbols.txt)
constexpr u32 kPtrVectorInit = 0x00551598;   // (vec, capacity, 0, 0) (symbols.txt)
constexpr u32 kPtrVectorReserve = 0x00551718;  // (vec, capacity) (symbols.txt)
constexpr u32 kLayerBlock = 0x00F65858;      // u32 768-byte block (freed by LayerStack_Reset42B240)
constexpr u32 kLayerWord0 = 0x010AA690;      // u32 cleared
constexpr u32 kLayerStack = 0x010AA694;      // PtrVector of 36-byte layers
constexpr u32 kLayerTop = 0x00F6585C;        // u32 the bottom layer
}  // namespace

// Layer* LayerStack_Init42ADA0() (name?): a 768-byte block (0x00F65858),
// 0x010AA690 = 0, and a 36-byte base layer (SmallAlloc: +0 = 0, PtrVector
// +4 (capacity 32, step 4), +32 = 0, +24 bits 31 / 29 cleared, 30 set)
// pushed onto the layer stack at 0x010AA694 (a failed allocation pushes 0);
// the stack's first layer is stored at 0x00F6585C and gets +24 bit 31.
DECOMP(0042ADA0, LayerStack_Init42ADA0) {
    store<u32>(kLayerBlock, static_cast<u32>(call(ctx, kMemAlloc, {768, 0, 16})));
    store<u32>(kLayerWord0, 0);
    const u32 l = static_cast<u32>(call(ctx, kSmallAlloc, {36}));
    if (l != 0) {
        store<u32>(l, 0);
        call(ctx, kPtrVectorConstruct, {l + 4});
        store<u16>(l + 10, 4);
        call(ctx, kPtrVectorInit, {l + 4, 32, 0, 0});
        const u32 w = load<u32>(l + 24);
        store<u32>(l + 32, 0);
        store<u32>(l + 24, ((w & 0x7FFFFFFFu) | 0x40000000u) & ~0x20000000u);
    }
    // The inlined PtrVector push (+0 data, +4 capacity, +8 count, +10 high).
    u32 n = load<u16>(kLayerStack + 8);
    const u32 cap = load<u16>(kLayerStack + 4);
    if (n == cap) {
        call(ctx, kPtrVectorReserve, {kLayerStack, (cap << 1) & 0x1FFFEu});
        n = load<u16>(kLayerStack + 8);
    }
    const u32 data = load<u32>(kLayerStack);
    const u32 c1 = (load<u16>(kLayerStack + 8) + 1) & 0xFFFF;
    const u32 hi = load<u16>(kLayerStack + 10);
    store<u32>(data + (n << 2), l);
    const u32 data2 = load<u32>(kLayerStack);
    store<u16>(kLayerStack + 8, static_cast<u16>(c1));
    const u32 top = load<u32>(data2);
    store<u16>(kLayerStack + 10, static_cast<u16>(static_cast<s32>(hi) > static_cast<s32>(c1) ? hi : c1));
    store<u32>(kLayerTop, top);
    const u32 f = load<u32>(top + 24) | 0x80000000u;
    store<u32>(top + 24, f);
    ctx.gpr[4] = f;
    ret(ctx, top);
}
