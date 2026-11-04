// World_HandleEvent callees, part 5: 149..152 instructions.
#include "hevent.h"

using namespace sly4;
using namespace sly4::hevent;

namespace sly4 {
namespace hevent5d {
namespace dfn {
constexpr u32 Overlay_Clear2AA9C8 = 0x002AA9C8;   // (0) (name?)
constexpr u32 RenderTarget_Create = 0x0051E33C;   // (u32 w, u32 h, 1, 0) -> target, +54 u16 height (name?)
constexpr u32 UiRes_ConstructThunk = 0x004D54D4;  // (mem, a, b, 1, 0)
}  // namespace dfn

constexpr u32 kOverlayWords = 0x010592E8;  // u32 [4] cleared together (+0, +8, +4, +12) (name?)
constexpr u32 kOverlayTarget = 0x00F64A60; // u32 render target, made once (name?)
constexpr u32 kOverlayTex = 0x00F64A5C;    // u32 UI texture resource wrapping the target (name?)
constexpr u32 kOverlayScale = 0x00F64A64;  // f32 1 - (target height - h) / target height (name?)
constexpr u32 kOne2AAC44 = 0x002AAC44;     // f32 1.0
constexpr u32 kMinTargetW = 720, kMinTargetH = 480;
constexpr u32 kTextureVtbl = 0x00B22618;   // UI texture resource, 112 bytes (font/tex.h)
constexpr u32 kTexSize = 112;
}  // namespace hevent5d
}  // namespace sly4

using namespace sly4::hevent5d;

// UiTexture* Overlay_ResetTexture2AAC48() (name?): clears the four words at
// 0x010592E8, Overlay_Clear2AA9C8(0); creates the render target once (half the
// display size, at least 720 x 480) with the scale 0x00F64A64; deletes the old
// UI texture (vfunc +8) and wraps the target in a new one (Mem_Alloc(112, 0, 16)
// + UiRes_ConstructThunk(mem, vfunc +12, vfunc +12, 1, 0), vtable 0x00B22618,
// +92 bits 30..31 cleared, +96 = target (its +4 u16 refcount + 1), +100.. zeroed,
// +82 = 2). Stores and returns it (0 when the allocation failed).
DECOMP(002AAC48, Overlay_ResetTexture2AAC48) {
    store<u32>(kOverlayWords + 0, 0);
    store<u32>(kOverlayWords + 8, 0);
    store<u32>(kOverlayWords + 4, 0);
    store<u32>(kOverlayWords + 12, 0);
    call(ctx, dfn::Overlay_Clear2AA9C8, {0, 0});
    if (load<u32>(kOverlayTarget) == 0) {
        u32 w = static_cast<u32>(load<u16>(kDisplayConfig + 4)) >> 1;
        u32 h = static_cast<u32>(load<u16>(kDisplayConfig + 6)) >> 1;
        if (w < kMinTargetW) w = kMinTargetW;
        if (h < kMinTargetH) h = kMinTargetH;
        const u32 t = static_cast<u32>(call(ctx, dfn::RenderTarget_Create, {w, h, 1, 0}));
        const f32 th = static_cast<f32>(static_cast<f64>(load<u16>(t + 54)));
        store<u32>(kOverlayTarget, t);
        const f32 fh = static_cast<f32>(static_cast<f64>(h));
        const f32 d = static_cast<f32>(static_cast<f64>(th) - fh);
        const f32 q = static_cast<f32>(static_cast<f64>(d) / th);
        store<f32>(kOverlayScale, static_cast<f32>(static_cast<f64>(load<f32>(kOne2AAC44)) - q));
    }
    const u32 old = load<u32>(kOverlayTex);
    if (old != 0) vcall(ctx, old, 8, {old});
    const u32 p = static_cast<u32>(call(ctx, fn::Mem_Alloc, {kTexSize, 0, 16}));
    if (p != 0) {
        const u32 t = load<u32>(kOverlayTarget);
        const u32 a = static_cast<u32>(vcall(ctx, t, 12, {t}));
        const u32 b = static_cast<u32>(vcall(ctx, t, 12, {t}));
        call(ctx, dfn::UiRes_ConstructThunk, {p, a, b, 1, 0});
        const u32 f = load<u32>(p + 92);
        store<u32>(p + 96, t);
        store<u32>(p + 0, kTextureVtbl);
        store<u32>(p + 92, f & 0x3FFFFFFFu);
        for (u32 k = 0; k < 12; ++k) store<u8>(p + 100 + k, 0);
        store<u16>(p + 82, 2);
        store<u16>(t + 4, static_cast<u16>(load<u16>(t + 4) + 1));
    }
    store<u32>(kOverlayTex, p);
    ret(ctx, p);
}

namespace sly4 {
namespace hevent5d {
namespace dfn2 {
constexpr u32 Obj_Resolve453A68 = 0x00453A68;  // (Object*, 0) -> Object* or 0 (name?)
constexpr u32 Obj_Ready453350 = 0x00453350;    // (Object*) -> bool (name?)
constexpr u32 Obj_Activate452058 = 0x00452058; // (Object*) (name?)
constexpr u32 Obj_Tick4536C0 = 0x004536C0;     // (Object*; f32 dt) -> 0 to remove (name?)
constexpr u32 Pool_Remove784DE4 = 0x00784DE4;  // (out iterator*, Pool*, u32 iterator) (name?)
}  // namespace dfn2
constexpr u32 kLodSource = 0x010ABD58;   // +16 -> +64: the LOD index (hevent3_d.cpp)
constexpr u32 kLodSource2 = 0x010ABD70;  // +16 -> +64: a second index of the same kind (name?)
// Group +52: a Pool (see Pool_DeleteAll): +60 head, +64 tail, +68 used count,
// +76 capacity, +80 entries {prev, next, Object*, flags} of 16 bytes.
constexpr u32 kGroupPool = 52;
}  // namespace hevent5d
}  // namespace sly4

// void Group_Update453FC4(Group* g, f32 dt) (name?): every object e of the
// second list (+36, +30 count) whose +16 is below (e->+0 -> +12)[index of
// 0x010ABD70] and whose +8 is below the entry at the index of 0x010ABD58 (or
// that entry is 0) is resolved (Obj_Resolve453A68(e, 0)) and, when ready,
// activated. Then the pool at +52 is walked from its head by {u16 index, u16
// generation} handles, stopping at a stale or free entry: each object gets
// Obj_Tick4536C0(o; dt), and is removed (Pool_Remove784DE4) when that is 0.
DECOMP(00453FC4, Group_Update453FC4) {
    const u32 g = arg<u32>(ctx, 0);
    const f64 dt = ctx.fpr[1];
    for (u32 i = 0; i < load<u16>(g + 30); ++i) {
        const u32 e = load<u32>(load<u32>(g + 36) + i * 8 + 4);
        if (e == 0) continue;
        const u32 arr = load<u32>(load<u32>(e) + 12);
        const s32 v = load<s32>(arr + (load<u32>(load<u32>(kLodSource2 + 16) + 64) << 2));
        if (!(load<s32>(e + 16) < v)) continue;
        const s32 v2 = load<s32>(arr + (load<u32>(load<u32>(kLodSource + 16) + 64) << 2));
        if (v2 != 0 && !(load<s32>(e + 8) < v2)) continue;
        const u32 r = static_cast<u32>(call(ctx, dfn2::Obj_Resolve453A68, {e, 0}));
        if (r == 0) continue;
        if (static_cast<u32>(call(ctx, dfn2::Obj_Ready453350, {r})) == 0) continue;
        call(ctx, dfn2::Obj_Activate452058, {r});
    }
    u32 it = 0;
    u64 r3 = 0;  // the original's r3 at each exit
    if (load<u32>(g + 68) != 0) {
        const u32 h = load<u32>(g + 60);
        r3 = (load<u32>(load<u32>(g + 80) + h * 16 + 12) >> 15) & 0xFFFF;
        it = ((h & 0xFFFF) << 16) | static_cast<u32>(r3);
    }
    const u32 pool = g + kGroupPool;
    for (;;) {
        const u32 gen = it & 0xFFFF;
        if (gen == 0) break;
        const u32 idx = it >> 16;
        r3 = idx;
        if (!(static_cast<s32>(idx) < load<s32>(g + 76))) break;
        const u32 ents = load<u32>(g + 80);
        const u32 ent = ents + ((idx << 4) & 0x000FFFF0u);
        if (((load<u32>(ent + 12) >> 15) & 0xFFFF) != gen) break;
        if (load<u8>(ent + 12) & 0x80) break;
        u32 next = 0;
        const u32 obj = load<u32>(ent + 8);
        if (static_cast<s32>(idx) != load<s32>(g + 64)) {
            const u32 n = load<u32>(ent + 4);
            next = (n << 16) | ((load<u32>(ents + (n << 4) + 12) >> 15) & 0xFFFF);
        }
        ctx.fpr[1] = dt;
        r3 = call(ctx, dfn2::Obj_Tick4536C0, {obj});
        if (static_cast<u32>(r3) == 0) {
            Stack frame(ctx, 16);  // out iterator at r1+112
            r3 = call(ctx, dfn2::Pool_Remove784DE4, {frame.addr(), pool, it});
        }
        it = next;
    }
    ctx.gpr[3] = r3;
}
