// Game code, large-function sweep part 29: the line-of-sight ray between
// two objects (Havok ray cast; hits on the objects themselves and on shape
// types 5 / 6 / >= 16 ignored, a hit's owning object asked through an
// optional callback).
#include "../sly4.h"
#include "big29.h"

using namespace sly4;
using namespace sly4::big29;

namespace {
constexpr u32 kMasksLos = 0x004A4750;  // .text: the y / z / w lane masks
constexpr u32 kGame = 0x00F65CA0;      // Game* (+0x1281D4 the physics holder, +4 its world)
constexpr u32 kVtRayCol = 0x00F1DE80;  // ray hit collector vtable (80-byte hits, 8 inline)
constexpr u32 kHkHeap = 0x011DF31C;    // Havok heap allocator (vfn[20] (p, bytes) frees)
constexpr u32 kPropKind = 0xA734004D;   // shape property: the shape's kind
constexpr u32 kKindOwned = 0x7A30D7E3;  // that kind: the shape belongs to the object in kPropOwner
constexpr u32 kPropOwner = 0x91C7CF5B;  // shape property: the owning object
constexpr u32 HkWorld_CastRay_92E510 = 0x0092E510;  // (hkpWorld*, const RayInput*, Collector*)

// The u64 value of property `key` in the shape's list (+124 the 16-byte
// entries {key, -, u64 value}, +128 the count), 0 when absent.
u64 Prop(u32 props, u32 key) {
    for (s32 j = 0; j < load<s32>(props + 128); ++j) {
        const u32 e = load<u32>(props + 124) + 16 * static_cast<u32>(j);
        if (load<u32>(e) == key) return load<u64>(e + 8);
    }
    return 0;
}
}  // namespace

// u32 f(Obj* a, Obj* b, Opd* cb) (called by f_0010C680,
// Obj_PickNearer_0017A1E0, f_00280AD0). Casts a ray from a->+144 to b->+144
// (RayInput at sp + 112: from, to, +32 / +36 / +40 = 0; the collector at
// sp + 160 with eight inline 80-byte hits at sp + 192). Hits whose shape
// (hit +68) type (+28 & 0x1F) is 5, 6 or >= 16 are ignored; for a shape
// with byte +24 == 1 whose kPropKind property is kKindOwned the owner is
// its kPropOwner property (zero-extended), else there is none. Hits owned
// by a or b are ignored. The first other hit decides: no owner -> 0; an
// owner -> cb(owner) (r4 the entry) or 0 without cb. No deciding hit ->
// 1. A heap hit array is freed first through kHkHeap->vfn[20] (OPD in r6,
// entry r7).
DECOMP(004A4780, Obj_LineOfSight_004A4780) {
    const u32 a = arg<u32>(ctx, 0), b = arg<u32>(ctx, 1), cb = arg<u32>(ctx, 2);
    Stack frame(ctx, 896);
    const u32 sp = frame.addr();  // the original's frame (offsets below are its sp offsets)
    const Masks3 mk = LoadMasks3(kMasksLos);
    store<u8>(sp + 144, 0);
    store<u32>(sp + 152, 0);
    store<u32>(sp + 148, 0);
    Stvx(sp + 112, RowSel(SplatW(a + 144), SplatW(a + 148), SplatW(a + 152), mk));
    Stvx(sp + 128, RowSel(SplatW(b + 144), SplatW(b + 148), SplatW(b + 152), mk));
    const u32 world = load<u32>(load<u32>(load<u32>(kGame) + 0x1281D4) + 4);
    store<u32>(sp + 164, 0x3F800000u);
    store<u32>(sp + 160, kVtRayCol);
    store<u32>(sp + 176, sp + 192);
    store<u32>(sp + 180, 0);
    store<u32>(sp + 184, 0x80000008u);
    for (u32 k = 0; k < 8; ++k) {
        const u32 h = sp + 192 + 80 * k;
        store<u32>(h + 16, 0x3F800000u);
        store<u32>(h + 20, 0xFFFFFFFFu);
        store<u32>(h + 64, 0);
        store<u32>(h + 32, 0xFFFFFFFFu);
        store<u32>(h + 68, 0);
    }
    store<u32>(sp + 180, 0);
    store<u32>(sp + 164, 0x3F800000u);
    call(ctx, HkWorld_CastRay_92E510, {world, sp + 112, sp + 160});
    const s32 n = load<s32>(sp + 180);
    const u32 hits = load<u32>(sp + 176), cap = load<u32>(sp + 184);
    // Frees a heap hit array (the collector's count / vtable reset first).
    auto release = [&](u32 c, u32 p) {
        if ((c & 0x80000000u) != 0) return;
        store<u32>(sp + 180, 0);
        store<u32>(sp + 160, kVtRayCol);
        VCall(ctx, kHkHeap, 20, {kHkHeap, p, static_cast<u64>(static_cast<s64>(static_cast<s32>(80 * (c & 0x3FFFFFFFu))))});
    };
    u32 hit = hits;
    for (s32 i = 0; i < n; ++i, hit += 80) {
        const u32 sh = load<u32>(hit + 68);
        const u32 ty = load<u32>(sh + 28) & 0x1F;
        if (!(ty < 16) || ty == 6 || ty == 5) continue;
        u32 owner = 0;
        if (static_cast<s8>(load<u8>(sh + 24)) == 1) {
            const u32 props = sh + static_cast<u32>(static_cast<s32>(static_cast<s8>(load<u8>(sh + 16))));
            if (props != 0) {
                // Only a kPropKind of kKindOwned has an owner (kPropOwner).
                if (static_cast<u32>(Prop(props, kPropKind)) == kKindOwned) {
                    owner = load<s32>(props + 128) > 0 ? static_cast<u32>(Prop(props, kPropOwner)) : 0;
                    if (static_cast<u32>(owner) == b || static_cast<u32>(owner) == a) continue;
                }
            }
        }
        if (static_cast<u32>(owner) == 0) {
            release(cap, hits);
            ctx.gpr[3] = 0;
            return;
        }
        if (static_cast<u32>(owner) == a || static_cast<u32>(owner) == b) continue;
        if (cb == 0) {
            release(cap, hits);
            ctx.gpr[3] = 0;
            return;
        }
        const u64 r = call_ptr(ctx, cb, {owner, load<u32>(cb)});
        release(load<u32>(sp + 184), load<u32>(sp + 176));
        ctx.gpr[3] = r;
        return;
    }
    release(cap, hits);
    ctx.gpr[3] = 1;
}
