// The five end-of-frame updates World_RunRenderJobs (0x00458BAC) makes
// before it swaps the frame arenas and runs the render list.
#include "../gfx/render.h"

using namespace sly4;

namespace {

// 0x010AA1B0: u8 flags (0x10: arena swap requested), +1124 ArenaPair*
// (the same {Arena* a, Arena* b, Arena* current, Arena* previous}
// layout as frame::kFrameArenas).
constexpr u32 kSwapState = 0x010AA1B0;
constexpr u32 kSwapPair = kSwapState + 1124;
constexpr u8 kSwapRequested = 0x10;

// 0x01195090: two u32 slots and, at +8, the index of the current one.
constexpr u32 kFlipSlots = 0x01195090;
constexpr u32 kFlipIndex = kFlipSlots + 8;

// 0x01013B70: {Obj** data; ...; +8 u16 count}; each object's vtable slot
// 36 is its end-of-frame update.
constexpr u32 kEndFrameList = 0x01013B70;
constexpr u32 kEndFrameSlot = 36;
constexpr u32 sub_0028E320 = 0x0028E320;  // () (name?)

// 0x01068841 u8: notify pending; 0x01068840 u8: the value passed along.
constexpr u32 kViewNotifyPending = 0x01068841;
constexpr u32 kViewNotifyValue = 0x01068840;
constexpr u32 kCurrentWorld = 0x00F65CA0;  // u32 World*
constexpr u32 kWorldMainView = 0x128168;   // View* (world + 0x128168)

}  // namespace

// void FrameUpdate_Nop()   (name?)
// The first end-of-frame update: an empty function.
DECOMP(0042AD98, FrameUpdate_Nop) {}

// void ArenaPair_SwapIfRequested()   (name?)
// When flag 0x10 of 0x010AA1B0 is set and its arena pair exists, swaps
// the pair (the other arena becomes current and is reset, its use count
// bumped, flag bits 0xC0 cleared), as World_RunRenderJobs does for the
// frame arenas. The flag is always cleared.
DECOMP(004291C8, ArenaPair_SwapIfRequested) {
    if ((ref<u8>(kSwapState) & kSwapRequested) != 0) {
        if (const u32 pair = load<u32>(kSwapPair); pair != 0) {
            const u32 cur = load<u32>(pair + 8);
            store<u32>(pair + 12, cur);
            const u32 arena = cur == load<u32>(pair) ? load<u32>(pair + 4) : load<u32>(pair);
            store<u32>(pair + 8, arena);
            const u32 a4 = load<u32>(arena + 4), uses = load<u32>(arena + 28);
            const u8 flags = ref<u8>(arena + 32);
            const u32 a8 = load<u32>(arena + 8);
            store<u32>(arena + 12, a4);
            store<u32>(arena + 16, a8);
            store<u32>(arena + 24, 0);
            store<u32>(arena + 20, 0);
            store<u32>(arena + 28, uses + 1);
            ref<u8>(arena + 32) = static_cast<u8>(flags & ~0xC0);
        }
    }
    const u8 state = static_cast<u8>(ref<u8>(kSwapState) & ~kSwapRequested);
    ref<u8>(kSwapState) = state;
    ctx.gpr[3] = state;  // left in r3 / r4 / r5
    ctx.gpr[4] = 0;
    ctx.gpr[5] = 0x010B0000;
}

// void DoubleSlot_Flip()   (name?)
// Toggles the index at 0x01195098 and clears the slot it now selects.
DECOMP(005AC60C, DoubleSlot_Flip) {
    const u32 index = load<u32>(kFlipIndex) ^ 1;
    const u32 off = index << 2;  // rlwinm 2,0,29
    store<u32>(kFlipIndex, index);
    store<u32>(kFlipSlots + off, 0);
    ctx.gpr[3] = 0x01190000;  // left by the original
    ctx.gpr[4] = kFlipSlots;
    ctx.gpr[5] = 0;
    ctx.gpr[6] = index;
    ctx.gpr[7] = off;
}

// void Objects_EndFrame()   (name?)
// sub_0028E320, then the slot-36 virtual of every object in the list at
// 0x01013B70 (count re-read each time; OPD / entry left in r4 / r5).
DECOMP(000D0298, Objects_EndFrame) {
    call(ctx, sub_0028E320);
    for (u32 i = 0, off = 0; i < load<u16>(kEndFrameList + 8); ++i, off += 4) {
        const u32 obj = load<u32>(load<u32>(kEndFrameList) + off);
        fx::vcall(ctx, obj, kEndFrameSlot, {obj});
    }
    ctx.gpr[3] = load<u16>(kEndFrameList + 8);  // the count the loop test last read, left in r3
}

// void MainView_NotifyIfPending()   (name?)
// When 0x01068841 is set: calls slot 8 of the object at
// ((main view +656) + 4) +32 with the byte at 0x01068840 (OPD / entry
// left in r5 / r6), then clears 0x01068841.
DECOMP(002F481C, MainView_NotifyIfPending) {
    if (ref<u8>(kViewNotifyPending) == 0) {
        ctx.gpr[3] = 0;  // the byte, left in r3
        return;
    }
    const u32 view = load<u32>(load<u32>(kCurrentWorld) + kWorldMainView);
    const u32 obj = load<u32>(load<u32>(view + 656) + 4 + 32);
    fx::vcall(ctx, obj, 8, {obj, ref<u8>(kViewNotifyValue)});
    ref<u8>(kViewNotifyPending) = 0;
    ctx.gpr[3] = 0x01070000;  // left by the original
    ctx.gpr[4] = 0;
}
