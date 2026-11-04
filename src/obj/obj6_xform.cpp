// World objects, part 6: the world-matrix update an object and its
// subtree get when they are added to the world.
#include <cmath>

#include "obj6.h"

using namespace sly4;
using namespace sly4::obj4;
using namespace sly4::obj5;
using namespace sly4::obj6;

namespace {

struct Q {
    f32 v[4];
};

inline Q Ld(u32 ea) {  // lvx
    Q r;
    const u32 a = ea & ~15u;
    for (u32 i = 0; i < 4; ++i) r.v[i] = load<f32>(a + i * 4);
    return r;
}

inline void St(u32 ea, const Q& q) {  // stvx
    const u32 a = ea & ~15u;
    for (u32 i = 0; i < 4; ++i) store<f32>(a + i * 4, q.v[i]);
}

// out_i = rows[0] * s_i.x + rows[1] * s_i.y + rows[2] * s_i.z (vmaddfp
// chains from 0, lane by lane), plus rows[3] for i = 3 (vaddfp).
void Combine(const Q rows[4], const Q s[4], Q out[4]) {
    for (u32 i = 0; i < 4; ++i)
        for (u32 k = 0; k < 4; ++k) {
            f32 a = std::fmaf(rows[0].v[k], s[i].v[0], 0.0f);
            a = std::fmaf(rows[1].v[k], s[i].v[1], a);
            a = std::fmaf(rows[2].v[k], s[i].v[2], a);
            out[i].v[k] = i == 3 ? rows[3].v[k] + a : a;
        }
}

// The object's own rows: its local matrix (+16) moved by the matrix at
// *(+160) (the default is 0x01193010).
void LocalTimesOffset(u32 o, Q out[4]) {
    const u32 m = load<u32>(o + kObjOffsetMtx);
    const Q l[4] = {Ld(o + kObjLocal), Ld(o + kObjLocal + 16), Ld(o + kObjLocal + 32), Ld(o + kObjLocal + 48)};
    const Q mm[4] = {Ld(m), Ld(m + 16), Ld(m + 32), Ld(m + 48)};
    Combine(l, mm, out);
}

void StoreWorld(u32 o, const Q w[4]) {
    for (u32 i = 0; i < 4; ++i) St(o + kObjWorld + i * 16, w[i]);
}

// The tail every updated object gets: non-unit scale (+164 0x2000) scales
// the world rows by +144, and kind-8 / kind-10 objects (the kind list's)
// get 0x00582F3C.
void Finish(PPUContext& ctx, u32 o) {
    if ((load<u32>(o + obj::kObjFlags) & obj::kObjNonUnitScale) != 0)
        call(ctx, o6fn::Matrix_ScaleRows, {o + kObjWorld, o + obj::kObjScale});
    const u32 kind = Kind(o);
    if (kind == 8 || kind == 10) call(ctx, o6fn::WorldObj_UpdateKind8, {o});
}

}  // namespace

// void WorldObj_UpdateTransforms(World* w, Obj* o)  ("World_AddObject" in symbols.txt; w unused)
// o's world matrix (+80) = its local matrix (+16) times the +160 offset
// matrix; then each descendant's, depth first, = (its local times its
// offset) times its parent's (+260) world matrix; each object then gets
// the Finish tail (o itself last). The walk uses the same 10-entry stack
// as WorldObj_ForEach (pushes past it are dropped). Quirk kept: the
// stack's slot 10 lies on the saved v31, so a push there comes back as
// v31's first word when the function returns.
DECOMP(00597814, WorldObj_UpdateTransforms) {
    const u32 o = arg<u32>(ctx, 1);
    Q w[4];
    LocalTimesOffset(o, w);
    StoreWorld(o, w);
    const u32 first = load<u32>(o + kObjFirst);
    bool clobbered = false;
    u32 slot10 = 0;
    if (first != 0) {
        const s32 cap = static_cast<s32>(kWalkStackCap);
        u32 stack[kWalkStackCap + 1] = {};
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
                below = top;
                node = stack[0];
            }
            if (node == 0) break;
            Q a[4];
            LocalTimesOffset(node, a);
            const u32 p = load<u32>(node + kObjParentLink) + kObjWorld;
            const Q pw[4] = {Ld(p), Ld(p + 16), Ld(p + 32), Ld(p + 48)};
            Combine(pw, a, w);
            StoreWorld(node, w);
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
    }
    Finish(ctx, o);
    if (clobbered) ctx.vr[31].uw(0) = slot10;
}
