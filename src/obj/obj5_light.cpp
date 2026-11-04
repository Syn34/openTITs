// ApeLightInstance, part 2: the light slot lists (*0x011945A0) and light
// creation / registration.
#include <cmath>

#include "obj5.h"

using namespace sly4;
using namespace sly4::obj;
using namespace sly4::obj4;
using namespace sly4::obj5;

// u32 LightSlot_Move(u16 to, u16 from)
// Light slots are 64-byte records ({f32[4] pos, ..., +44, +48 owner, +52,
// +56, +60 / +61 bytes, +62 bit 0x80}). The owner of slot `to` loses its
// slot (+284 = 0xFFFF); then slot `from` is copied into `to` and its owner
// takes `to`, or, when to == from, the slot is reset (the default sphere,
// +44 = 0, owner 0xFEEDF00D). r3 is left as the original leaves it.
DECOMP(005A2024, LightSlot_Move) {
    const u32 to = arg<u32>(ctx, 0), from = arg<u32>(ctx, 1);
    const u32 to_off = (to << 6) & 0x003FFFC0u;
    const u32 d = load<u32>(cls::kLightSlotLists) + to_off;
    store<u16>(load<u32>(d + kLsOwner) + obj2::kObjWord284, 0xFFFF);
    if (((to ^ from) & 0xFFFF) == 0) {
        for (u32 k = 0; k < 16; k += 4) store<u32>(d + k, load<u32>(cls::kDefaultSphere + k));
        store<u32>(d + 44, 0);
        store<u32>(d + kLsOwner, kLsNoOwner);
        return ret(ctx, static_cast<s32>(kLsNoOwner));  // lis / ori: sign-extended
    }
    const u32 s = load<u32>(cls::kLightSlotLists) + ((from << 6) & 0x003FFFC0u);
    for (u32 k = 0; k < 60; k += 4) store<u32>(d + k, load<u32>(s + k));
    const u8 d62 = load<u8>(d + 62);
    store<u8>(d + 60, load<u8>(s + 60));
    store<u8>(d + 61, load<u8>(s + 61));
    store<u8>(d + 62, static_cast<u8>((d62 & 0x7F) | (load<u8>(s + 62) & 0x80)));
    const u32 owner = load<u32>(load<u32>(cls::kLightSlotLists) + to_off + kLsOwner);
    store<u16>(owner + obj2::kObjWord284, static_cast<u16>(to));
    ret(ctx, owner);
}

namespace {

// A fresh light slot for o: the default sphere, +44 = 0, owner o.
inline void InitLightSlot(u32 o, u32 slot) {
    store<u16>(o + obj2::kObjWord284, static_cast<u16>(slot));
    const u32 rec = load<u32>(cls::kLightSlotLists) + ((slot << 6) & 0xFFFFFFC0u);
    for (u32 k = 0; k < 16; k += 4) store<u32>(rec + k, load<u32>(cls::kDefaultSphere + k));
    store<u32>(rec + 44, 0);
    store<u32>(rec + kLsOwner, o);
}

}  // namespace

// void LightInstance_Update(LightInstance* o)
// A visible light without a slot takes one while the lists have room (+4 <
// +6): type 2 from the end of the first list (+4 grows), types 0 / 1 from
// the start of the second (+6 shrinks). A light with a slot then publishes
// itself: +240 position, +292 (0 when hidden), +380, the visible bit
// (+62 0x80, cleared again when the light is dead), the type, the colour,
// the +384 parameters (with +372: {+388, +376 / (+388 - +384), 0, +376}),
// and from the definition (+160 squared, 1 / ((+160 * 2 - +156) * +156)).
DECOMP(005A31C8, LightInstance_Update) {
    const u32 o = arg<u32>(ctx, 0);
    if (load<u16>(o + obj2::kObjWord284) == 0xFFFF && (load<u32>(o + kObjFlags) & kObjFlagVisible) != 0) {
        const u32 end = load<u16>(cls::kLightSlotLists + 4), start = load<u16>(cls::kLightSlotLists + 6);
        if (static_cast<s32>(end) < static_cast<s32>(start)) {
            const s32 type = load<s32>(o + cls::kLightType);
            if (type == 2) {
                InitLightSlot(o, end);
                store<u16>(cls::kLightSlotLists + 4, static_cast<u16>(end + 1));
            } else if (type == 0 || type == 1) {
                InitLightSlot(o, start);
                store<u16>(cls::kLightSlotLists + 6, static_cast<u16>(start - 1));
            }
        }
    }
    const u32 slot = load<u16>(o + obj2::kObjWord284);
    if (slot == 0xFFFF) return;
    const u32 rec = load<u32>(cls::kLightSlotLists) + ((slot << 6) & 0xFFFFFFC0u);
    for (u32 k = 0; k < 16; k += 4) store<u32>(rec + k, load<u32>(o + kObjAnchor + k));
    const u32 v292 = load<u32>(o + kObj292);
    store<u32>(rec + 44, v292);
    const bool vis = (load<u32>(o + kObjFlags) & kObjFlagVisible) != 0;
    store<u32>(rec + 44, vis ? v292 : 0);
    store<u8>(rec + 60, load<u8>(o + cls::kLight380));
    const bool vis2 = (load<u32>(o + kObjFlags) & kObjFlagVisible) != 0;
    const u8 b62 = static_cast<u8>((load<u8>(rec + 62) & 0x7F) | (vis2 ? 0x80 : 0));
    store<u8>(rec + 62, b62);
    const s32 refs = static_cast<s16>(load<u16>(o + kObjRefs));
    store<u8>(rec + 62, static_cast<u8>(refs > 0 ? b62 : (b62 & 0x7F)));
    store<u8>(rec + 61, static_cast<u8>(load<u32>(o + cls::kLightType)));
    for (u32 k = 0; k < 12; k += 4) store<u32>(rec + 32 + k, load<u32>(o + cls::kLightColor + k));
    if (load<u32>(o + cls::kLight372) == 0) {
        store<u32>(rec + 16, load<u32>(o + cls::kLight384));
        store<u32>(rec + 20, load<u32>(o + cls::kLight384 + 4));
        store<u32>(rec + 24, load<u32>(o + cls::kLight384 + 8));
        store<u32>(rec + 28, load<u32>(o + cls::kLightRangeSq));
    } else {
        const f32 a = load<f32>(o + cls::kLight384 + 4), b = load<f32>(o + cls::kLight384);
        const f32 r = load<f32>(o + cls::kLightRangeSq);
        store<u32>(rec + 16, load<u32>(o + cls::kLight384 + 4));  // raw lfs / stfs
        store<f32>(rec + 20, r / (a - b));
        store<u32>(rec + 24, load<u32>(kLightConsts + 8));
        store<u32>(rec + 28, load<u32>(o + cls::kLightRangeSq));
    }
    const u32 def = load<u32>(o + kObjClassPtr);
    const f32 outer = load<f32>(def + 160), inner = load<f32>(def + 156);
    store<f32>(rec + 52, outer * outer);
    const f64 two = load<f32>(kLightConsts), one = load<f32>(kLightConsts + 4);
    const f32 t = static_cast<f32>(std::fma(static_cast<f64>(outer), two, -static_cast<f64>(inner)));  // fmsubs
    const f32 u = t * inner;
    store<f32>(rec + 56, static_cast<f32>(one / static_cast<f64>(u)));
}
