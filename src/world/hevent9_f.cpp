// World_HandleEvent callees, part 9f: the layer stack update.
#include "hevent9.h"

using namespace sly4;
using namespace sly4::hevent;
using namespace sly4::hevent7;
using namespace sly4::hevent8;
using namespace sly4::hevent9;

namespace sly4 {
namespace hevent9f {
namespace ffn {
constexpr u32 Entry_Release42AEE8 = 0x0042AEE8;  // (Entry16*) (as hevent3_i.cpp sub_0042AEE8)
constexpr u32 Layer_Update42B5B0 = 0x0042B5B0;   // (Layer*; f32 dt) (name?)
constexpr u32 Layer_Removed42B420 = 0x0042B420;  // (Layer*) after it left the stack (name?)
constexpr u32 Nop = 0x0054EC18;
constexpr u32 PtrVector_Clear = 0x0054EC1C;
}  // namespace ffn

// The layer stack (as hevent3_i.cpp): 16-byte entries {+0 Layer*, ...} at
// *0x00F65858 (u32 count 0x010AA690), the PtrVector of 36-byte layers at
// 0x010AA694 and a second one at 0x010AA6A8 (layers added during the
// update).
constexpr u32 kLayerEntries = 0x00F65858, kLayerEntryCount = 0x010AA690;
constexpr u32 kLayers = 0x010AA694, kNewLayers = 0x010AA6A8;
constexpr u32 kVecCount = 8;
constexpr u32 kEntrySize = 16, kEntryCopy = 13;
// Layer: +4 PtrVector of objects (+12 u16 count), +24 flags (bit 31
// active, bit 30 running, bit 29 closing).
constexpr u32 kLayerObjects = 4, kLayerObjectCount = 12, kLayerFlags = 24, kLayerSize = 36;
constexpr u32 kFlagActive = 0x80000000u, kFlagRunning = 0x40000000u, kFlagClosing = 0x20000000u;
}  // namespace hevent9f
}  // namespace sly4

using namespace sly4::hevent9f;

namespace {
// One layer's step: an active running layer is updated with dt; a closing
// one has its entries released and swap-removed (13 of the 16 bytes copied
// from the last entry) from the entry block.
void layer_step(PPUContext& ctx, u32 l, f64 dt) {
    u32 f = load<u32>(l + kLayerFlags);
    if ((f & kFlagActive) != 0 && (f & kFlagRunning) != 0) {
        callf(ctx, ffn::Layer_Update42B5B0, {l}, {dt});
        f = load<u32>(l + kLayerFlags);
    }
    if ((f & kFlagClosing) == 0 || load<u32>(kLayerEntryCount) == 0) return;
    for (u32 j = 0, off = 0;; ++j, off += kEntrySize) {
        const u32 e = load<u32>(kLayerEntries) + off;
        if (load<u32>(e) == l) {
            call(ctx, ffn::Entry_Release42AEE8, {e});
            const u32 n = load<u32>(kLayerEntryCount);
            const u32 a = load<u32>(kLayerEntries);
            call(ctx, h8fn::Memmove, {a + off, a + (n - 1) * kEntrySize, kEntryCopy});
            store<u32>(kLayerEntryCount, load<u32>(kLayerEntryCount) - 1);
            j -= 1;
            off -= kEntrySize;
        }
        if (!(load<u32>(kLayerEntryCount) > j + 1)) break;
    }
}
}  // namespace

// u16 LayerStack_Update42B720(f32 dt) (name?): with more than one layer,
// clears the new-layer vector, steps every layer from the top down, then
// the layers added meanwhile; finally every closing layer leaves the stack
// (swap-remove; its objects released by vtable +4, its vector cleared, the
// 36-byte block freed, then 0x0042B420 with the layer). A null entry is read
// through (+24 of 0) as in the original. r3 as the original leaves it.
DECOMP(0042B720, LayerStack_Update42B720) {
    const f64 dt = farg(ctx, 0);
    const u32 n = load<u16>(kLayers + kVecCount);
    if (n <= 1) return;
    store<u16>(kNewLayers + kVecCount, 0);
    for (s32 i = static_cast<s32>(n) - 1; i >= 0; --i)
        layer_step(ctx, load<u32>(load<u32>(kLayers) + static_cast<u32>(i) * 4), dt);
    for (u32 i = 0; i < load<u16>(kNewLayers + kVecCount); ++i)
        layer_step(ctx, load<u32>(load<u32>(kNewLayers) + i * 4), dt);

    for (u32 i = 0, off = 0;; ++i, off += 4) {
        const u32 cnt = load<u16>(kLayers + kVecCount);
        if (!(i < cnt)) {
            ctx.gpr[3] = cnt;
            return;
        }
        const u32 l = load<u32>(load<u32>(kLayers) + off);
        if ((load<u32>(l + kLayerFlags) & kFlagClosing) == 0) continue;
        if (l != 0) {
            for (u32 k = 0; k < load<u16>(l + kLayerObjectCount); ++k) {
                const u32 o = load<u32>(load<u32>(l + kLayerObjects) + k * 4);
                if (o != 0) vcall(ctx, o, 4, {o});
            }
            store<u16>(l + kLayerObjectCount, 0);
            call(ctx, ffn::PtrVector_Clear, {l + kLayerObjects});
            call(ctx, ffn::Nop, {l + kLayerObjects});
            call(ctx, h7fn::SmallFree, {l, kLayerSize});
        }
        const u32 c = load<u16>(kLayers + kVecCount) - 1;
        const u32 data = load<u32>(kLayers);
        store<u16>(kLayers + kVecCount, static_cast<u16>(c));
        if (i != (c & 0xFFFF)) store<u32>(data + off, load<u32>(data + (c & 0xFFFF) * 4));
        call(ctx, ffn::Layer_Removed42B420, {l});
        i -= 1;
        off -= 4;
    }
}
