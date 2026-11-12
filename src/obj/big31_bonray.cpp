// Game code, large-function sweep part 31: a controller's aim ray (Havok
// ray cast along row 2 of its target matrix).
#include "big31.h"

namespace sly4::obj {

struct Mat44b {
    Vec4 r[4];  // rows 0..2, translation
};
// BoneCtl_Target's output: the matrix, +64 its source buffer, +68, +72.
struct BoneTargetOut {
    Mat44b m;        // +0
    be_u32 src;      // +64
    be_u32 w68;      // +68
    u8 b72;          // +72
};
struct AimCtl {
    pad<8> _0;
    be_u32 self;     // +8: the owning object (hits owned by it count)
    pad<1252> _12;
    be_u32 target;   // +1264
};
static_assert(offsetof(AimCtl, target) == 1264);

namespace {
// .text: f32 0 (hit threshold), 40.0 (ray length); lane masks y / z / w at
// +16 / +32 / +48; f32 -1.0 (no hit) at +64
constexpr u32 kAimK = 0x002947B0;
constexpr u32 kIdent = 0x01164BD0;      // the global identity matrix
constexpr u32 BoneCtl_Target = 0x00250F40;  // (Ctl*, Out* m, u32 k, const f32* pos) -> m

f64 KF(u32 off) { return lf(gm<be_f32>(kAimK + off)); }
// Rows 0..n-1 of the identity's x / y / z words to d.
void IdentRows(Mat44b& d, u32 n) {
    const Mat44b& id = global<Mat44b>(kIdent);
    for (u32 r = 0; r < n; ++r) {
        set_bits(d.r[r].x, bits(id.r[r].x));
        set_bits(d.r[r].y, bits(id.r[r].y));
        set_bits(d.r[r].z, bits(id.r[r].z));
    }
}
}  // namespace

// bool f(Ctl* c, f32 out[16]) (called by f_00295268 and f_00296328). The
// controller's target matrix M (on the stack, set to the identity's rows
// first; its source pointer is a 64-byte stack buffer) comes from
// BoneCtl_Target(c, &M, c->target, 0). A ray from M's row 3 to row 3 + 40
// * row 2 is cast (an all-hits collector with eight inline hits) and the
// hits sorted. The first hit whose collidable type is below 16 and not 5
// or 6, that is not a phantom (kind 2), and whose owner (a kind-1
// collidable with kPropKind kKindOwned: its kPropOwner) is none or c->self,
// gives the fraction. out's rows 0..2 = the identity's (x, y, z words); out
// row 3 = the hit point for a fraction above 0 (true), else the ray's end
// (false). A heap hit array is freed through kHkHeap.
bool BoneCtl_RayTarget_002947F8(AimCtl* c, Mat44b* out) {
    GuestFrame frame(896);  // the original's sp+112..
    RayInput* ray = frame.as<RayInput>(0);
    BoneTargetOut* M = frame.as<BoneTargetOut>(112);
    RayCollector* coll = frame.as<RayCollector>(192);
    IdentRows(M->m, 4);
    const f64 thr = KF(0);
    M->w68 = 0u;
    M->b72 = 0;
    M->src = frame.addr() + 48;
    call(BoneCtl_Target, c, M, c->target.get(), 0u);
    const Vec4& p = M->m.r[3];
    const Vec4& d = M->m.r[2];
    const f64 len = KF(4);
    const f64 sx = lf(p.x), sy = lf(p.y), sz = lf(p.z);
    const f64 ex = Fmadds(len, lf(d.x), sx), ey = Fmadds(len, lf(d.y), sy), ez = Fmadds(len, lf(d.z), sz);
    ray->b32 = 0;
    ray->w40 = 0u;
    const Masks3 mk = LoadMasks3(kAimK + 16);
    Stvx(&ray->from, RowSelW(&p.x, &p.y, &p.z, mk));
    Stvx(&ray->to, RowSel(Fill4(static_cast<f32>(ex)), Fill4(static_cast<f32>(ey)), Fill4(static_cast<f32>(ez)), mk));
    ray->w36 = 0u;
    const u32 world = PhysWorld();
    RayCollector_Init(coll);
    call(b31fn::HkWorld_CastRay92E510, world, ray, coll);
    call(b31fn::HkAllHits_Sort887E28, coll);
    const s32 n = coll->count;
    const u32 hp = coll->hits;
    const u32 self = c->self;
    const u32 cw = coll->capflags;
    f64 t = KF(64);
    for (s32 i = 0; i < n; ++i) {
        const RayHit& hit = gm<RayHit>(hp + 80 * static_cast<u32>(i));
        const u32 sh = hit.shape;
        const HkCollidable& col = gm<HkCollidable>(sh);
        const u32 ty = col.type & 0x1F;
        if (ty >= 16 || ty == 6 || ty == 5) continue;
        const s8 kind = col.kind;
        const u32 rel = sh + static_cast<u32>(static_cast<s32>(col.owner_off));
        const u32 props = kind == 1 ? rel : 0;
        const u32 owned = kind == 2 ? rel : 0;
        u32 owner = 0;
        if (props == 0) {
            if (owned != 0) continue;
        } else if (HkPropLo(props, kPropKind) == kKindOwned) {
            owner = gm<HkProps>(props).count > 0 ? HkPropLo(props, kPropOwner) : 0;
        }
        if (owner != 0 && owner != self) continue;
        t = lf(hit.fraction);
        break;
    }
    IdentRows(*out, 3);
    const bool got = t > thr;
    Vec4& o3 = out->r[3];
    if (got) {
        o3.x = static_cast<f32>(Fmadds(Subs(ex, sx), t, sx));
        o3.y = static_cast<f32>(Fmadds(Subs(ey, sy), t, sy));
        o3.z = static_cast<f32>(Fmadds(Subs(ez, sz), t, sz));
    } else {
        o3.x = static_cast<f32>(ex);
        o3.y = static_cast<f32>(ey);
        o3.z = static_cast<f32>(ez);
    }
    RayCollector_Free(coll, hp, cw);
    return got;
}

}  // namespace sly4::obj

NATIVE(002947F8, sly4::obj::BoneCtl_RayTarget_002947F8)
