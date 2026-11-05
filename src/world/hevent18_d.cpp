// World_HandleEvent callees, part 18d: component look tables and the effect
// slot binder.
#include "hevent18.h"

using namespace sly4;
using namespace sly4::hevent;
using namespace sly4::hevent18;

namespace {
// Look component: +8 Object* (+160 WorldObj, +4 bits 10..15 kind, 8: mesh),
// +24 220-byte entries, +28 u32 count. Entry: +4 u32 per-look source[],
// +12 Material*, +76..+88 f32 params 28..31, +92 f32[32] copied from the
// material's +124 array (or 0.0).
constexpr u32 kLookObj = 8, kLookEntries = 24, kLookCount = 28, kLookEntrySize = 220;
constexpr u32 kEntSources = 4, kEntMaterial = 12, kEntParams = 76, kEntValues = 92, kNumValues = 32;
constexpr u32 kMatValues = 124, kFirstParam = 28, kNumParams = 4;
constexpr u32 kObjKindWord = 4, kKindMesh = 8;
constexpr u32 kZeroF287314 = 0x00287314;  // f32 0.0
constexpr u32 kMatParamSlot = 0x0050C018;     // (Material*, u32 index) -> u32* (symbols.txt)
constexpr u32 kWobjMaterial4A59B4 = 0x004A59B4;  // (WorldObj*, source) -> Material* (name?)

// Effect: +136 u32 flags (bit 9 0x200: detached). Its component of type
// 0x0027469E: vt +32 kind (5: bound), +28 Owner* (vt +8 release; +4 u16
// reference count), +36 u32 value.
constexpr u32 kEffSize = 140, kEffFlags = 136, kEffDetached = 0x200;
constexpr u32 kCompBound = 0x0027469E, kCompVtKind = 32, kKindBound = 5, kCompOwner = 28, kCompValue = 36;
constexpr u32 kOwnerVtRelease = 8, kOwnerRefs = 4;
constexpr u32 kEffectRelease = 0x004CF3C4, kReleaseArg = 0x10CBB;
}  // namespace

// void Comp_SetLook287318(Component* c, u8 look) (name?): for every entry:
// on a mesh WorldObj, binds the look's source material (0x004A59B4) to the
// entry; then, with a material, copies the entry's four params into
// material params 28..31 and the material's 32 values (0.0 without a value
// array) back into the entry.
DECOMP(00287318, Comp_SetLook287318) {
    const u32 c = arg<u32>(ctx, 0), look = arg<u8>(ctx, 1);
    const u32 o = load<u32>(c + kLookObj);
    const u32 wobj = o != 0 ? load<u32>(o + hevent10::kObjWorldObj) : 0u;
    if (load<u32>(c + kLookCount) == 0) {
        ret(ctx, 0u);  // r3 left as the original leaves it
        return;
    }
    u32 last = 0;  // the single f1 is left holding
    for (u32 i = 0; i < load<u32>(c + kLookCount); ++i) {
        const u32 e = load<u32>(c + kLookEntries) + kLookEntrySize * i;
        const u32 src = load<u32>(e + look * 4 + kEntSources);
        if (src != 0 && wobj != 0 && ((load<u32>(wobj + kObjKindWord) >> 10) & 0x3F) == kKindMesh)
            store<u32>(e + kEntMaterial, static_cast<u32>(call(ctx, kWobjMaterial4A59B4, {wobj, src})));
        if (load<u32>(e + kEntMaterial) == 0) continue;
        for (u32 k = 0; k < kNumParams; ++k) {
            const u32 v = load<u32>(e + kEntParams + 4 * k);  // lfs / stfs pair
            const u32 slot = static_cast<u32>(call(ctx, kMatParamSlot, {load<u32>(e + kEntMaterial), kFirstParam + k}));
            store<u32>(slot, v);
        }
        for (u32 j = 0; j < kNumValues; ++j) {
            const u32 vals = load<u32>(load<u32>(e + kEntMaterial) + kMatValues);
            last = vals != 0 ? vals + 4 * j : kZeroF287314;
            store<u32>(e + kEntValues + 4 * j, load<u32>(last));
        }
        ctx.fpr[1] = hevent7::lfs_f64(last);
    }
    ctx.gpr[3] = load<u32>(c + kLookCount);  // r3 left as the original leaves it
}

// Effect* Effect_Bind4A621C(Effect** slot, Def* def, Owner* owner, u32
// value, u8 force) (name?): a bound effect is released (marked detached)
// when a def is given or force is set. With a def: creates and starts a new
// effect for it, binding owner (reference counted) and value to its bound
// component, and stores it in the slot. Returns the slot's effect. A failed
// allocation still flags and starts address 0 (as the original).
DECOMP(004A621C, Effect_Bind4A621C) {
    const u32 slot = arg<u32>(ctx, 0), def = arg<u32>(ctx, 1), owner = arg<u32>(ctx, 2), value = arg<u32>(ctx, 3);
    u32 r3 = load<u32>(slot);
    if (r3 != 0 && (def != 0 || arg<u8>(ctx, 4) != 0)) {
        call(ctx, kEffectRelease, {r3, kReleaseArg});
        const u32 cur = load<u32>(slot);
        store<u32>(cur + kEffFlags, load<u32>(cur + kEffFlags) | kEffDetached);
        store<u32>(slot, 0);
        r3 = 0;
    }
    if (def == 0) {
        ret(ctx, r3);
        return;
    }
    u32 e = static_cast<u32>(call(ctx, hevent7::h7fn::SmallAlloc, {kEffSize}));
    if (e != 0) call(ctx, hevent7::h7fn::Effect_Construct, {e, def});
    if (owner != 0) {
        const u32 comp = static_cast<u32>(call(ctx, hevent7::h7fn::Effect_FindComponent, {e, kCompBound}));
        if (comp != 0 && static_cast<s32>(vcall(ctx, comp, kCompVtKind, {comp})) == static_cast<s32>(kKindBound)) {
            const u32 old = load<u32>(comp + kCompOwner);
            if (old != 0) vcall(ctx, old, kOwnerVtRelease, {old});
            store<u32>(comp + kCompOwner, owner);
            store<u16>(owner + kOwnerRefs, static_cast<u16>(load<u16>(owner + kOwnerRefs) + 1));
            store<u32>(comp + kCompValue, value);
        }
    }
    store<u32>(e + kEffFlags, load<u32>(e + kEffFlags) | kEffDetached);
    call(ctx, hevent7::h7fn::Effect_Start, {e});
    store<u32>(slot, e);
    if (e == 0) {
        ret(ctx, 0u);
        return;
    }
    store<u32>(e + kEffFlags, load<u32>(e + kEffFlags) & ~kEffDetached);
    ret(ctx, load<u32>(slot));
}
