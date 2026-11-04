// World_HandleEvent callees, part 9l: the light / volume set update.
#include <cmath>

#include "hevent9.h"

using namespace sly4;
using namespace sly4::hevent;
using namespace sly4::hevent7;
using namespace sly4::hevent9;

namespace sly4 {
namespace hevent9l {
namespace lfn {
constexpr u32 Volume_Refresh399758 = 0x00399758;  // (Volume*, u8, 0, 1) (name?)
constexpr u32 Item_Update39C1C8 = 0x0039C1C8;     // (Item*; f32 dt) -> active (name?)
constexpr u32 Items_Submit39C8F0 = 0x0039C8F0;    // ({Item** data, u32 count}*) (name?)
constexpr u32 Probe_Prepare39D054 = 0x0039D054;   // (Probe*) (name?)
constexpr u32 Probe_SetOn39D1D4 = 0x0039D1D4;     // (Probe*, u8) (name?)
constexpr u32 Probe_Update39FBD8 = 0x0039FBD8;    // (Probe*; f32 dt) (name?)
constexpr u32 PtrVector_Reserve = 0x00551718;
}  // namespace lfn

// The set: +4 / +36 / +20 tables (u16 count at +2) of the +12 volumes (112
// bytes), +44 probes (368 bytes) and +28 items (336 bytes); +48 PtrVector
// of the active items {+0 data, +4 u16 capacity, +8 u16 count, +10 u16
// high water}.
constexpr u32 kSetVolTab = 4, kSetVols = 12, kSetItemTab = 20, kSetItems = 28, kSetProbeTab = 36, kSetProbes = 44;
constexpr u32 kSetActive = 48;
constexpr u32 kVolSize = 112, kProbeSize = 368, kItemSize = 336;
// Volume: +48 vec4 sphere (unaligned), +68 Model*, +72 flags (bit 31
// visible, bit 30 always test), +78 u16 count, +84 {+4 s16 item index}[8].
constexpr u32 kVolSphere = 48, kVolModel = 68, kVolFlags = 72, kVolLinkCount = 78, kVolLinks = 84;
constexpr u32 kModelBusy = 352;   // u16
// Probe: +316 s32 kind (1: no on/off), +336 Model*, +356 flags (bit 30
// enabled, bit 29 on, bit 28 forced).
constexpr u32 kProbeKind = 316, kProbeModel = 336, kProbeFlags = 356;
// Linked items (*0x00F657FC +28, 336 bytes each): +276 s32 kind (1: skip),
// +328 flags (bit 28: visible).
constexpr u32 kItemTable = 0x00F657FC, kItemKind = 276, kItemFlags = 328;
constexpr u32 kForceAll = 0x0106D084;      // u8 (0x0106D03C + 72) (name?)
constexpr u32 kSubmit = 0x01085240;        // {u32 data, u32 count}
constexpr u32 kZero = 0x003A0468;          // f32 0.0
constexpr u32 kWorldRenderTarget = 0x128168;
constexpr u32 kRtPlanes = 368;             // RenderTarget +368: 8 planes as 2 x {x, y, z, w} rows of 4 lanes
}  // namespace hevent9l
}  // namespace sly4

using namespace sly4::hevent9l;

namespace {
// The VMX sphere test against the render target's 8 planes: for each set of
// 4 lanes, ((n.x * X + d) + n.y * Y) + n.z * Z (fused, as vmaddfp) plus the
// radius, all >= 0.
bool sphere_visible(u32 rt, u32 p) {
    const f32 x = load<f32>(p), y = load<f32>(p + 4), z = load<f32>(p + 8), w = load<f32>(p + 12);
    const u32 base = rt + kRtPlanes;
    for (u32 set = 0; set < 2; ++set) {
        // set 0: rows +0 / +16 / +32, bias +48; set 1: rows +64 / +80 / +96, bias +112.
        const u32 r = base + 64 * set;
        for (u32 k = 0; k < 4; ++k) {
            f32 v = std::fma(load<f32>(r + 4 * k), x, load<f32>(r + 48 + 4 * k));
            v = std::fma(load<f32>(r + 16 + 4 * k), y, v);
            v = std::fma(load<f32>(r + 32 + 4 * k), z, v);
            if (!(w + v >= 0.0f)) return false;
        }
    }
    return true;
}
}  // namespace

// void Set_Update3A0470(Set* s, f32 dt) (name?): refreshes the volumes
// with a model (0x00399758 with "forced" = 0x0106D084 or the model busy),
// sphere-tests the forced / always-test ones against the render target's
// planes (visible: +72 bit 31, copied to bit 28 of their linked items not of
// kind 1); keeps each enabled probe's on bit (+356 bit 29) on its model's
// busy count (preparing it first when neither on nor forced), telling it
// unless of kind 1, and updates every probe with dt; with dt > 0, collects
// the items 0x0039C1C8 reports active into +48 and submits them
// (0x01085240 {data, count}). r3 / f1 as the original leaves them.
DECOMP(003A0470, Set_Update3A0470) {
    const u32 s = arg<u32>(ctx, 0);
    const f64 dt = farg(ctx, 0);
    if (load<u16>(load<u32>(s + kSetVolTab) + 2) != 0) {
        for (u32 i = 0, off = 0;;) {
            const u32 v = load<u32>(s + kSetVols) + off;
            const u32 model = load<u32>(v + kVolModel);
            if (model != 0) {
                const u32 forced = (load<u8>(kForceAll) != 0 || load<u16>(model + kModelBusy) != 0) ? 1 : 0;
                call(ctx, lfn::Volume_Refresh399758, {v, forced, 0, 1});
            }
            if (load<u8>(kForceAll) != 0 || (load<u32>(v + kVolFlags) & 0x40000000u) != 0) {
                const u32 rt = load<u32>(load<u32>(kCurrentWorld) + kWorldRenderTarget);
                const u32 vis = sphere_visible(rt, v + kVolSphere) ? 1 : 0;
                store<u32>(v + kVolFlags, (load<u32>(v + kVolFlags) & 0x7FFFFFFFu) | (vis << 31));
                for (u32 j = 0; j < load<u16>(v + kVolLinkCount); ++j) {
                    const s32 idx = static_cast<s16>(load<u16>(load<u32>(v + kVolLinks) + j * 8 + 4));
                    if (idx == -1) continue;
                    const u32 it = load<u32>(load<u32>(kItemTable) + 28) + static_cast<u32>(idx) * kItemSize;
                    if (it == 0 || load<u32>(it + kItemKind) == 1) continue;
                    store<u32>(it + kItemFlags, (load<u32>(it + kItemFlags) & ~0x10000000u) | (vis << 28));
                }
            }
            ++i;
            off += kVolSize;
            if (!(i < load<u16>(load<u32>(s + kSetVolTab) + 2))) break;
        }
    }

    if (load<u16>(load<u32>(s + kSetProbeTab) + 2) != 0) {
        for (u32 i = 0, off = 0;;) {
            const u32 p = load<u32>(s + kSetProbes) + off;
            if (p != 0) {
                u32 f = load<u32>(p + kProbeFlags);
                u32 m = load<u32>(p + kProbeModel);
                if ((f & 0x40000000u) != 0 && m != 0) {
                    if ((f & 0x10000000u) == 0 && (f & 0x20000000u) == 0 && load<u16>(m + kModelBusy) != 0) {
                        call(ctx, lfn::Probe_Prepare39D054, {p});
                        m = load<u32>(p + kProbeModel);
                        f = load<u32>(p + kProbeFlags);
                    }
                    const u32 on = load<u16>(m + kModelBusy) != 0 ? 1 : 0;
                    const u32 kind = load<u32>(p + kProbeKind);
                    store<u32>(p + kProbeFlags, (f & ~0x20000000u) | (on << 29));
                    if (kind != 1) call(ctx, lfn::Probe_SetOn39D1D4, {p, on});
                }
                callf(ctx, lfn::Probe_Update39FBD8, {p}, {dt});
            }
            ++i;
            off += kProbeSize;
            if (!(i < load<u16>(load<u32>(s + kSetProbeTab) + 2))) break;
        }
    }

    const u32 n = load<u16>(load<u32>(s + kSetItemTab) + 2);
    ctx.gpr[3] = n;
    if (n == 0) return;
    const f64 zero = lfs_f64(kZero);
    ctx.fpr[1] = zero;  // f1 as the original leaves it
    if (!(dt > zero)) return;
    store<u16>(s + kSetActive + 8, 0);
    for (u32 i = 0, off = 0;;) {
        const u32 it = load<u32>(s + kSetItems) + off;
        callf(ctx, lfn::Item_Update39C1C8, {it}, {dt});
        if (static_cast<u32>(ctx.gpr[3]) != 0) {
            const u32 vec = s + kSetActive;
            u32 c = load<u16>(vec + 8);
            const u32 cap = load<u16>(vec + 4);
            if (c == cap) {
                call(ctx, lfn::PtrVector_Reserve, {vec, (cap << 1) & 0x1FFFEu});
                c = load<u16>(vec + 8);
            }
            store<u32>(load<u32>(vec) + ((c << 2) & 0x3FFFCu), it);
            const u32 c1 = load<u16>(vec + 8) + 1;
            const u32 high = load<u16>(vec + 10);
            store<u16>(vec + 8, static_cast<u16>(c1));
            store<u16>(vec + 10, static_cast<u16>(static_cast<s32>(high) > static_cast<s32>(c1 & 0xFFFF) ? high : c1));
        }
        ++i;
        off += kItemSize;
        const u32 cnt = load<u16>(load<u32>(s + kSetItemTab) + 2);
        ctx.gpr[3] = cnt;
        if (!(i < cnt)) break;
    }
    const u32 m = load<u16>(s + kSetActive + 8);
    if (m == 0) return;
    store<u32>(kSubmit, load<u32>(s + kSetActive));
    store<u32>(kSubmit + 4, m);
    call(ctx, lfn::Items_Submit39C8F0, {kSubmit});
}
