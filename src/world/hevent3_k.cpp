// World_HandleEvent callees, part 3: 103..104 instructions.
#include "hevent.h"

using namespace sly4;
using namespace sly4::hevent;

namespace sly4 {
namespace hevent3k {
namespace kkfn {
constexpr u32 Ui_Find = 0x004D5544;          // (path, parent)
constexpr u32 Timeline_Play = 0x004E2EF0;    // (name, Element*, u8, u8) (hevent3_e.cpp)
constexpr u32 Ui_SendEvent = 0x004D3DA4;     // (type, a, b, c)
constexpr u32 View_Set520338 = 0x00520338;   // (View*; f32) (name?)
constexpr u32 sub_004C8E80 = 0x004C8E80;     // (obj140*)
constexpr u32 SmallFree = 0x00552420;        // (ptr, size)
constexpr u32 Nop = 0x0054EC18;
constexpr u32 Mem_FreeFrom = 0x0054EE6C;     // (p, pool)
}  // namespace kkfn

constexpr u32 kStrDefaultInit = 0x00ADCBB4;  // "Default_Init"

// World +0x12820C PtrVector of u32 layers (+8 count = +0x128214), +0x128220..22 u8 (name?)
constexpr u32 kWorldLayers = 0x12820C, kWorldLayerCount = 0x128214;
constexpr u32 kWorldMainView = 0x128168;

// Job A (hevent_f.cpp kJobCF97C): the indices it lists, their count, its pending flag.
constexpr u32 kJobObjects = 0x01013B34;      // {u32 data, ..., u16 count at +8}
constexpr u32 kJobAIndices = 0x01013BB0;     // u16[]
constexpr u32 kJobACount = 0x01013BA0;       // s32
constexpr u32 kJobAPending = 0x01021D80;     // u32
}  // namespace hevent3k
}  // namespace sly4

using namespace sly4::hevent3k;

// bool Obj_ShowPanel(Object* o, const char* path): 0 in states 3, 4, 6, 7, 9;
// otherwise +336 = Ui_Find(path, 0), and unless the state is 0 or 2 that is all
// (1). In states 0 / 2 with an element: +340 = it, +336 = 0 and the timeline at
// +344 is deleted (vfunc +4) and cleared; then, when the current entry (o->+8
// +148, 16 bytes each, index +356) has +12 and +13 both 0, there is a +340 and no
// timeline, +344 = Timeline_Play("Default_Init", +340, 0, 0) and
// Ui_SendEvent(12, -1, +340, 0). Returns 1 (name?)
DECOMP(0037370C, Obj_ShowPanel) {
    const u32 o = arg<u32>(ctx, 0), path = arg<u32>(ctx, 1);
    const u32 t = load<u32>(o + 8);
    const u32 st = load<u32>(o + 160) >> 1;
    if (st == 3 || st == 9 || st == 4 || st == 6 || st == 7) return ret(ctx, 0u);
    const u32 el = static_cast<u32>(call(ctx, kkfn::Ui_Find, {path, 0}));
    const u32 idx = load<u16>(o + 356);
    store<u32>(o + 336, el);
    const u32 tab = load<u32>(t + 148);
    u32 tl = load<u32>(o + 344);
    const u32 st2 = load<u32>(o + 160) >> 1;
    bool show = false;
    if (load<u8>(tab + (idx << 4) + 12) == 0 && load<u8>(tab + ((idx << 4) & 0x000FFFF0u) + 13) == 0) show = true;
    if (st2 != 0 && st2 != 2) return ret(ctx, 1u);
    if (el != 0) {
        store<u32>(o + 340, el);
        store<u32>(o + 336, 0);
        if (tl != 0) {
            vcall(ctx, tl, 4, {tl});
            tl = 0;
            store<u32>(o + 344, 0);
        }
    }
    if (!show) return ret(ctx, 1u);
    const u32 owner = load<u32>(o + 340);
    if (owner == 0 || tl != 0) return ret(ctx, 1u);
    store<u32>(o + 344, static_cast<u32>(call(ctx, kkfn::Timeline_Play, {kStrDefaultInit, owner, 0, 0})));
    call(ctx, kkfn::Ui_SendEvent, {12, 0xFFFFFFFFu, load<u32>(o + 340), 0});
    ret(ctx, 1u);
}

// u16 World_ClearLayers(World* w): for every layer e of the vector at +0x12820C:
// GetState(1) (vfunc +32(w, 1)) == 12 marks level 1's data +60 with 0x40000000;
// the main view gets View_Set520338(view; e +36); e +28 (140 bytes) is destroyed
// and freed; e's vector is cleared and e freed. Then the count and the bytes
// +0x128220..22 = 0. Quirk: e is dereferenced before its null check (name?)
DECOMP(0047E3CC, World_ClearLayers) {
    const u32 w = arg<u32>(ctx, 0);
    u32 n = 0;
    for (u32 i = 0; i < (n = load<u16>(w + kWorldLayerCount)); ++i) {
        const u32 e = load<u32>(load<u32>(w + kWorldLayers) + i * 4);
        if (static_cast<s32>(vcall(ctx, w, world::kVfnGetState, {w, 1})) == 12) {
            const u32 d = load<u32>(w + world::kSmData) + load<u16>(w + world::kSmDataSize);
            store<u32>(d + 60, load<u32>(d + 60) | 0x40000000u);
        }
        const u32 view = load<u32>(w + kWorldMainView);
        if (view != 0) callf(ctx, kkfn::View_Set520338, {view}, {static_cast<f64>(load<f32>(e + 36))});
        const u32 x = load<u32>(e + 28);
        if (x != 0) {
            call(ctx, kkfn::sub_004C8E80, {x});
            call(ctx, kkfn::SmallFree, {x, 140});
            store<u32>(e + 28, 0);
        }
        if (e != 0) {
            call(ctx, hfn::PtrVector_Clear, {e});
            call(ctx, kkfn::Nop, {e});
            call(ctx, kkfn::Mem_FreeFrom, {e, 0});
        }
    }
    store<u16>(w + kWorldLayerCount, 0);
    store<u8>(w + 0x128220, 0);
    store<u8>(w + 0x128221, 0);
    store<u8>(w + 0x128222, 0);
    ret(ctx, n);
}

// u32 Objects_FinishJobA(): waits (300 us sleeps) for job A's SPU work, then
// for its listed indices, last first, while the object list is not empty: the
// object gets vfunc +76 when its s16 +178 is negative, +178 = 0, is deleted
// (vfunc +4) and removed from the list (swapped with the last). Clears the count.
// Quirk: +178 is read before the null check. Returns 0x01010000 as the
// original leaves r3 (name?)
DECOMP(000CFA24, Objects_FinishJobA) {
    while (load<u32>(kJobAPending) != 0) syscall(ctx, kSysTimerUsleep, {300});
    for (s32 k = load<s32>(kJobACount) - 1; k >= 0; --k) {
        if (load<u16>(kJobObjects + 8) == 0) break;
        const u32 ia = kJobAIndices + static_cast<u32>(k) * 2;
        const u32 o = load<u32>(load<u32>(kJobObjects) + (load<u16>(ia) << 2));
        if (static_cast<s16>(load<u16>(o + 178)) < 0) vcall(ctx, o, 76, {o});
        store<u16>(o + 178, 0);
        if (o != 0) vcall(ctx, o, 4, {o});
        const u32 last = load<u16>(kJobObjects + 8) - 1;
        const u32 slot = load<u16>(ia);
        store<u16>(kJobObjects + 8, static_cast<u16>(last));
        if (slot != (last & 0xFFFF)) {
            const u32 data = load<u32>(kJobObjects);
            store<u32>(data + (slot << 2), load<u32>(data + ((last << 2) & 0x3FFFCu)));
        }
    }
    store<u32>(kJobACount, 0);
    ret(ctx, 0x01010000u);
}
