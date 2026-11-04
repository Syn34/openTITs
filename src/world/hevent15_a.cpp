// World_HandleEvent callees, part 15a: small leaves under the hevent14
// functions (kind-13 component, camera target stack, object by id, cutscene
// stop).
#include "hevent15.h"

using namespace sly4;
using namespace sly4::hevent;
using namespace sly4::hevent15;

namespace {
// Object +272: component list head; component +268 next, +4 bits 10..15 kind.
constexpr u32 kObjComponents = 272, kCompNext = 268, kCompBits = 4;
constexpr u32 kCompKindShift = 10, kCompKindMask = 0x3F, kCompKind13 = 13;

// Camera +345: u8 target count, +368 Target*[] (bottom first).
constexpr u32 kCamTargetCount = 345, kCamTargets = 368;

// Object +168: id, +172 bit 18: has an id.
constexpr u32 kObjId = 168, kObjIdFlags = 172, kObjHasId = 0x00040000;

// Cutscene: +164 flags (bit 30: kept), +152 Object* (+396 bit 30: dispatch on
// stop), +136 Player* (vt +96 stop(Player*, u8)).
constexpr u32 kCutFlags = 164, kCutKept = 0x40000000;
constexpr u32 kCutObject = 152, kCutPlayer = 136, kPlayerVtStop = 96;
constexpr u32 kObjFlags396 = 396, kObj396Bit30 = 0x40000000;
}  // namespace

// Component* Obj_FindComponent13(Object* o) (name?): the first component in o
// +272 (next at +268) whose kind (+4 bits 10..15) is 13, or 0.
DECOMP(004A22A4, Obj_FindComponent13) {
    u32 c = load<u32>(arg<u32>(ctx, 0) + kObjComponents);
    while (c != 0) {
        if (((load<u32>(c + kCompBits) >> kCompKindShift) & kCompKindMask) == kCompKind13) return ret(ctx, c);
        c = load<u32>(c + kCompNext);
    }
    ret(ctx, 0u);
}

// Target* Obj_LayerBelow(Camera* cam, Target* t) (name?): searches the
// target stack (+368, +345 entries) from the top for t and returns the entry
// below it; 0 when t is the bottom one or not there.
DECOMP(000FCBD0, Obj_LayerBelow) {
    const u32 cam = arg<u32>(ctx, 0), t = arg<u32>(ctx, 1);
    const u32 base = cam + kCamTargets;
    for (u32 n = load<u8>(cam + kCamTargetCount); n != 0; --n) {
        if (load<u32>(base + (n - 1) * 4) == t) return ret(ctx, n > 1 ? load<u32>(base + (n - 2) * 4) : 0u);
    }
    ret(ctx, 0u);
}

// Object* Obj_FindById(u32 id) (name?): the object in the all-objects
// list (0x01013B34) that has an id (+172 bit 18) equal to id (+168); 0 for
// id 0 or none.
DECOMP(000D6A28, Obj_FindById) {
    const u32 id = arg<u32>(ctx, 0);
    if (id == 0) return ret(ctx, 0u);
    const u32 data = load<u32>(kAllObjects + kAllData);
    const u32 n = load<u16>(kAllObjects + kAllCount);
    for (u32 i = 0; i < n; ++i) {
        const u32 o = load<u32>(data + 4 * i);
        if ((load<u32>(o + kObjIdFlags) & kObjHasId) != 0 && load<u32>(o + kObjId) == id) return ret(ctx, o);
    }
    ret(ctx, 0u);
}

// void Cutscene_Stop4269E4(Cutscene* c, u8 keep) (name?): records keep in +164
// bit 30 (from the full argument's bit 0); with keep, dispatches the
// cutscene's object (+152) when its +396 bit 30 is set; then tells the
// player (+136) to stop (vt +96, keep).
DECOMP(004269E4, Cutscene_Stop4269E4) {
    const u32 c = arg<u32>(ctx, 0), a = arg<u32>(ctx, 1);
    const u8 keep = static_cast<u8>(a);
    store<u32>(c + kCutFlags, (load<u32>(c + kCutFlags) & ~kCutKept) | ((a & 1) << 30));
    u64 r3 = c;
    if (keep != 0) {
        const u32 o = load<u32>(c + kCutObject);
        r3 = o;
        if (o != 0 && (load<u32>(o + kObjFlags396) & kObj396Bit30) != 0) r3 = call(ctx, h15fn::Obj_Dispatch362, {o, 1, 1, 0});
    }
    const u32 p = load<u32>(c + kCutPlayer);
    if (p == 0) {
        ctx.gpr[3] = r3;  // r3 as the original leaves it
        return;
    }
    vcall(ctx, p, kPlayerVtStop, {p, keep});
}
