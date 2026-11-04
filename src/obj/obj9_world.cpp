// World objects, part 9: World_AddGroup, the transform pass World_AddObjects
// runs over a group's tree as it adds it.
#include "obj9.h"

using namespace sly4;
using namespace sly4::obj4;
using namespace sly4::obj8;

namespace {

constexpr u32 kWorldObj_UpdateSphere = 0x0057E14C;  // (obj) -> &obj +240

// The local matrix (+16) through the +160 offset matrix.
void LocalTimesOffset(u32 o, Vec4 out[4]) {
    Vec4 l[4], m[4];
    MatLoad(o + obj6::kObjLocal, l);
    MatLoad(load<u32>(o + obj6::kObjOffsetMtx), m);
    MatCombine(l, m, out);
}

// After an object's world matrix: the dirty bit cleared, the bounding
// sphere, the Changed slot (+36), and for kinds 8 / 10 0x00582F3C.
// Returns what the original leaves in r3.
u64 Finish(PPUContext& ctx, u32 o) {
    store<u32>(o + obj::kObjFlags, load<u32>(o + obj::kObjFlags) & ~obj::kObjDirty);
    call(ctx, kWorldObj_UpdateSphere, {o});
    Vcall0(ctx, o, kVfnChanged);
    const u32 kind = Kind(o);
    if (kind == 8 || kind == 10) return call(ctx, obj6::o6fn::WorldObj_UpdateKind8, {o});
    return kind;
}

}  // namespace

// void World_AddGroup(World* w, Obj* g)  (w unused)
// g's world matrix (+80) = its local matrix times its +160 offset matrix;
// then each descendant's, depth first, = its parent's (+260) world matrix
// times (its local times its offset). Each object gets the Finish tail
// right after its matrix (g first). The walk uses WorldObj_ForEach's
// 10-entry stack (pushes past it are dropped).
// Quirk kept: the stack's slot 10 lies on the saved v31, so a push there
// comes back as v31's first word when the function returns.
DECOMP(005922A8, World_AddGroup) {
    const u32 g = arg<u32>(ctx, 1);
    Vec4 w[4];
    LocalTimesOffset(g, w);
    MatStore(g + obj6::kObjWorld, w);
    const u64 r3 = Finish(ctx, g);
    const u32 first = load<u32>(g + kObjFirst);
    if (first == 0) {
        ctx.gpr[3] = r3;  // r3 as the original leaves it
        return;
    }
    const s32 cap = static_cast<s32>(obj5::kWalkStackCap);
    u32 stack[obj5::kWalkStackCap + 1] = {};  // sp+120: a 0 sentinel, then the pushes
    bool clobbered = false;
    u32 slot10 = 0;
    s32 top = 0;
    auto push = [&](s32 at, u32 v) {
        top = at;
        stack[at] = v;
        if (at == cap) {
            clobbered = true;
            slot10 = v;
        }
    };
    push(1, first);  // top 0 < cap
    for (;;) {
        s32 below;
        u32 node;
        if (top > -1) {
            below = top - 1;
            node = stack[top];
            top = below;
        } else {
            below = top;  // unreachable: popping the sentinel ends the walk
            node = stack[0];
        }
        if (node == 0) break;
        Vec4 a[4], pw[4];
        LocalTimesOffset(node, a);
        MatLoad(load<u32>(node + kObjParentLink) + obj6::kObjWorld, pw);
        MatCombine(pw, a, w);
        MatStore(node + obj6::kObjWorld, w);
        Finish(ctx, node);
        s32 at = below;
        const u32 next = load<u32>(node + kObjNext);
        if (next != 0 && below < cap) {
            push(below + 1, next);
            at = top;
        }
        const u32 child = load<u32>(node + kObjFirst);
        if (child != 0 && at < cap) push(at + 1, child);
    }
    if (clobbered) ctx.vr[31].uw(0) = slot10;
}
