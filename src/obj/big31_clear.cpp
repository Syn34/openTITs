// Game code, large-function sweep part 31: whether the segment between two
// points is free of geometry (Havok ray cast, two objects' shapes ignored).
#include "big31.h"

namespace sly4::obj {

namespace {
constexpr u32 kClearMasks = 0x00235100;  // .text: lane masks y / z / w
}  // namespace

// bool f(const f32 a[3], const f32 b[3], Obj* x, Obj* y) (called by
// f_000EBEF8 and f_00235688). Casts a ray from a to b (an all-hits
// collector with eight inline hits; the hits are not sorted). Hits whose
// collidable type is 5, 6 or >= 16 are ignored. A kind 1 or 2 collidable
// has a property block; when its kPropKind is kKindOwned its owner is the
// kPropOwner property. Hits owned by x or y are ignored; any other hit
// (with no owner or another one) blocks: false. No blocking hit: true. A
// heap hit array is freed first through kHkHeap.
bool Ray_ClearBetween_00235130(const Vec3* a, const Vec3* b, u32 x, u32 y) {
    GuestFrame frame(720);  // the original's sp+112..
    RayInput* ray = frame.as<RayInput>(0);
    RayCollector* coll = frame.as<RayCollector>(48);
    ray->b32 = 0;
    ray->w40 = 0u;
    const Masks3 mk = LoadMasks3(kClearMasks);
    Stvx(&ray->from, RowSelW(&a->x, &a->y, &a->z, mk));
    Stvx(&ray->to, RowSelW(&b->x, &b->y, &b->z, mk));
    ray->w36 = 0u;
    const u32 world = PhysWorld();
    RayCollector_Init(coll);
    call(b31fn::HkWorld_CastRay92E510, world, ray, coll);
    const s32 n = coll->count;
    const u32 hp = coll->hits;
    const u32 cw = coll->capflags;
    bool clear = true;
    for (s32 i = 0; i < n; ++i) {
        const u32 sh = gm<RayHit>(hp + 80 * static_cast<u32>(i)).shape;
        const HkCollidable& col = gm<HkCollidable>(sh);
        const u32 ty = col.type & 0x1F;
        if (ty >= 16 || ty == 6 || ty == 5) continue;
        const s8 kind = col.kind;
        const u32 rel = sh + static_cast<u32>(static_cast<s32>(col.owner_off));
        const u32 props = kind == 1 ? rel : (kind == 2 ? rel : 0);
        u32 owner = 0;
        if (props != 0 && HkPropLo(props, kPropKind) == kKindOwned)
            owner = gm<HkProps>(props).count > 0 ? HkPropLo(props, kPropOwner) : 0;
        if (owner != 0 && (owner == x || owner == y)) continue;
        clear = false;
        break;
    }
    RayCollector_Free(coll, hp, cw);
    return clear;
}

}  // namespace sly4::obj

NATIVE(00235130, sly4::obj::Ray_ClearBetween_00235130)
