// World_HandleEvent callees, part 3: 93..101 instructions.
#include "hevent.h"

using namespace sly4;
using namespace sly4::hevent;

namespace sly4 {
namespace hevent3j {
namespace jfn {
constexpr u32 SaveSlot_MarkSeen = 0x002E2874;    // (?, u32 hash)
constexpr u32 Ui_OpenScreen4D245C = 0x004D245C;  // (name, a, b) -> Element* (name?)
constexpr u32 UiElem_SetX4D21B4 = 0x004D21B4;    // (Element*, s32 x, 0) (name?)
constexpr u32 sub_0047BE90 = 0x0047BE90;         // (World*, u32 v) -> u32 hash (reads only r4)
constexpr u32 WorldObj_FindByHash = 0x005801DC;  // (u32 hash, 0) -> object or 0
constexpr u32 Hash_ToString = 0x0056A6E0;        // (u32 hash) -> "0x%x"
constexpr u32 sub_002B7EA0 = 0x002B7EA0;         // (Entity*, Object*)
}  // namespace jfn

constexpr u32 kUiBusy1163F81 = 0x01163F81, kUiBusy1163F82 = 0x01163F82;  // u8 (hevent_c.cpp Flags1163F81_Idle)
constexpr u32 kSessionPtr = 0x00F64C20;
constexpr u32 kHalf466004 = 0x00466004;          // f32 0.5
constexpr u32 kElemWidth = 288, kElemHeight = 292;  // u32 (name?)
constexpr u32 kElemScaleX = 280, kElemScaleY = 284; // f32 (name?)

// World +0x128610: u8 count of 36-byte groups at +0x128628, each with up to 4 objects (name?)
constexpr u32 kWorldGroupCount = 0x128610, kWorldGroups = 0x128628, kWorldGroupSize = 36;

constexpr u32 kEntities = 0x01056F1C;            // SortedMap: +2 u16 count, +8 {key, Entity*}; type at entity +12
constexpr u32 kEntityType27 = 27;
}  // namespace hevent3j
}  // namespace sly4

using namespace sly4::hevent3j;

// Element* Ui_OpenScaledScreen(?, const char* name, a, b): unless the UI is busy
// (0x01163F81 / 0x01163F82), marks the name seen in the save slot (when given),
// opens it (Ui_OpenScreen4D245C(name, a, b)) and scales it to the display height:
// scale = height / +292 into +280 and +284, x = (width - scale * +288) * 0.5 (name?)
DECOMP(00466008, Ui_OpenScaledScreen) {
    const u32 name = arg<u32>(ctx, 1), a = arg<u32>(ctx, 2), b = arg<u32>(ctx, 3);
    if (load<u8>(kUiBusy1163F81) != 0 || load<u8>(kUiBusy1163F82) != 0) return ret(ctx, 0u);
    if (name != 0) {
        const u32 sess = load<u32>(kSessionPtr);
        const u32 h = static_cast<u32>(call(ctx, hfn::String_HashNoCase, {name}));
        call(ctx, jfn::SaveSlot_MarkSeen, {sess, h});
    }
    const u32 e = static_cast<u32>(call(ctx, jfn::Ui_OpenScreen4D245C, {name, a, b}));
    if (e == 0) return ret(ctx, 0u);
    const f32 hgt = static_cast<f32>(static_cast<f64>(load<u16>(kDisplayConfig + 6)));
    const f32 eh = static_cast<f32>(static_cast<f64>(load<u32>(e + kElemHeight)));
    const f32 scale = static_cast<f32>(static_cast<f64>(hgt) / static_cast<f64>(eh));
    const f32 wid = static_cast<f32>(static_cast<f64>(load<u16>(kDisplayConfig + 4)));
    const f32 ew = static_cast<f32>(static_cast<f64>(load<u32>(e + kElemWidth)));
    const f32 half = load<f32>(kHalf466004);
    const f32 d = static_cast<f32>(-std::fma(static_cast<f64>(scale), static_cast<f64>(ew), -static_cast<f64>(wid)));  // fnmsubs
    const f32 x = fmuls(d, half);
    const u64 bits = rt::fctiw(x, true);
    ctx.set_fpr_bits(1, bits);
    ctx.fpr[2] = half;
    ctx.fpr[3] = ew;
    call(ctx, jfn::UiElem_SetX4D21B4, {e, static_cast<u64>(static_cast<s64>(static_cast<s32>(static_cast<u32>(bits)))), 0});
    store<f32>(e + kElemScaleX, scale);
    store<f32>(e + kElemScaleY, scale);
    ret(ctx, e);
}

// u32 World_ReleaseGroups(World* w): for each of the u8 +0x128610 groups, each
// object of its 4 slots up to the first empty one gets vfunc +88(o, 0, 1, 1, 1)
// and +40(o, 0) (+40(o, 1) too with +172 bit 0x00040000) and the slot is
// cleared; then the count = 0 (name?)
DECOMP(0046B874, World_ReleaseGroups) {
    const u32 w = arg<u32>(ctx, 0);
    u32 n = load<u8>(w + kWorldGroupCount);
    for (u32 g = 0; g < n; ++g, n = load<u8>(w + kWorldGroupCount)) {
        for (u32 k = 0; k < 4; ++k) {
            const u32 slot = w + g * kWorldGroupSize + kWorldGroups + k * 4;
            const u32 o = load<u32>(slot);
            if (o == 0) break;
            vcall(ctx, o, 88, {o, 0, 1, 1, 1});
            vcall(ctx, o, 40, {o, 0});
            if (load<u32>(o + 172) & 0x00040000u) vcall(ctx, o, 40, {o, 1});
            store<u32>(slot, 0);
        }
    }
    store<u8>(w + kWorldGroupCount, 0);
    ret(ctx, n);
}

// u32 User0_RefreshTarget(World* w, u32 user): only for user 0: v = vfunc +20
// (or +16 when that gives 0) of its object's +216; h = sub_0047BE90(world, v);
// with h, WorldObj_FindByHash(h, 0) gets vfunc +40(obj, 0, 1), otherwise
// Hash_ToString(v) (result unused). Every type-27 entity of 0x01056F1C then
// gets sub_002B7EA0(entity, obj). Returns the entity count (name?)
DECOMP(0047BFD4, User0_RefreshTarget) {
    const u32 w = arg<u32>(ctx, 0), user = arg<u32>(ctx, 1);
    if (user != 0) return;  // r3 = w
    u32 v = 0;
    const u32 o = load<u32>(w + user * kUserBlockSize + kUserBlocks + 8);
    if (o != 0) {
        const u32 c = load<u32>(o + 216);
        v = static_cast<u32>(vcall(ctx, c, 20, {c}));
        if (v == 0) {
            const u32 c2 = load<u32>(o + 216);
            v = static_cast<u32>(vcall(ctx, c2, 16, {c2}));
        }
    }
    const u32 h = static_cast<u32>(call(ctx, jfn::sub_0047BE90, {load<u32>(kCurrentWorld), v}));
    u32 obj = 0;
    if (h != 0) {
        obj = static_cast<u32>(call(ctx, jfn::WorldObj_FindByHash, {h, 0}));
        if (obj != 0) vcall(ctx, obj, 40, {obj, 0, 1});
    } else {
        call(ctx, jfn::Hash_ToString, {v});
    }
    s32 i = 0;
    for (; i < static_cast<s32>(load<u16>(kEntities + 2)); ++i) {
        const u32 e = load<u32>(load<u32>(kEntities + 8) + static_cast<u32>(i) * 8 + 4);
        if (e != 0 && load<s32>(e + 12) == static_cast<s32>(kEntityType27)) call(ctx, jfn::sub_002B7EA0, {e, obj});
    }
    ret(ctx, load<u16>(kEntities + 2));
}
