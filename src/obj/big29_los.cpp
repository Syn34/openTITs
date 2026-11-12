// Game code, large-function sweep part 29: the line-of-sight ray between
// two objects (Havok ray cast; hits on the objects themselves and on shape
// types 5 / 6 / >= 16 ignored, a hit's owning object asked through an
// optional callback).
#include "big29.h"
#include "big31.h"

namespace sly4::obj {

namespace {
constexpr u32 kMasksLos = 0x004A4750;  // .text: the y / z / w lane masks

// An object's position (+144).
struct LosObj {
    pad<144> _0;
    Vec3 pos;  // +144
};
}  // namespace

// u32 f(Obj* a, Obj* b, Opd* cb) (called by f_0010C680,
// Obj_PickNearer_0017A1E0, f_00280AD0). Casts a ray from a->+144 to b->+144
// (RayInput at sp + 112: from, to, +32 / +36 / +40 = 0; the collector at
// sp + 160 with eight inline 80-byte hits at sp + 192). Hits whose shape
// (hit +68) type (+28 & 0x1F) is 5, 6 or >= 16 are ignored; for a shape
// with byte +24 == 1 whose kPropKind property is kKindOwned the owner is
// its kPropOwner property (zero-extended), else there is none. Hits owned
// by a or b are ignored. The first other hit decides: no owner -> 0; an
// owner -> cb(owner) or 0 without cb. No deciding hit ->
// 1. A heap hit array is freed first through kHkHeap->vfn[20].
u32 Obj_LineOfSight_004A4780(LosObj* a, LosObj* b, u32 cb) {
    GuestFrame frame(896);  // the original's frame (offsets below are its sp offsets)
    RayInput* in = frame.as<RayInput>(112);
    RayCollector* col = frame.as<RayCollector>(160);
    const Masks3 mk = LoadMasks3(kMasksLos);
    in->b32 = 0;
    in->w40 = 0u;
    in->w36 = 0u;
    Stvx(&in->from, RowSelW(&a->pos.x, &a->pos.y, &a->pos.z, mk));
    Stvx(&in->to, RowSelW(&b->pos.x, &b->pos.y, &b->pos.z, mk));
    const u32 world = PhysWorld();
    RayCollector_Init(col);
    call(b31fn::HkWorld_CastRay92E510, world, in, col);
    const s32 n = col->count;
    const u32 hits = col->hits, cap = col->capflags;
    const u32 ea = guest(a), eb = guest(b);
    u32 hit = hits;
    for (s32 i = 0; i < n; ++i, hit += 80) {
        const u32 shea = gm<RayHit>(hit).shape;
        const HkCollidable& sh = gm<HkCollidable>(shea);
        const u32 ty = sh.type & 0x1F;
        if (!(ty < 16) || ty == 6 || ty == 5) continue;
        u32 owner = 0;
        if (sh.kind == 1) {
            const u32 props = shea + static_cast<u32>(static_cast<s32>(sh.owner_off));
            // Only a kPropKind of kKindOwned has an owner (kPropOwner).
            if (props != 0 && HkPropLo(props, kPropKind) == kKindOwned) {
                owner = HkPropLo(props, kPropOwner);
                if (owner == eb || owner == ea) continue;
            }
        }
        if (owner == 0 || cb == 0) {
            RayCollector_Free(col, hits, cap);
            return 0;
        }
        const u32 r = call_ptr<u32>(cb, owner);
        RayCollector_Free(col, col->hits, col->capflags);
        return r;
    }
    RayCollector_Free(col, hits, cap);
    return 1;
}

}  // namespace sly4::obj

NATIVE(004A4780, sly4::obj::Obj_LineOfSight_004A4780)
