// Game code, large-function sweep part 31: a marker's state change (its
// shown object swapped, or a ground-snapped debris object spawned).
#include "big31.h"

namespace sly4::obj {

struct MarkObj {
    pad<144> _0;
    Vec3 pos;       // +144
    pad<16> _156;
    be_u32 flags;   // +172: 0x00040000 hidden twice
    pad<2> _176;
    be_u16 refs;    // +178
};
static_assert(offsetof(MarkObj, flags) == 172 && offsetof(MarkObj, refs) == 178);
struct MarkOwner {
    pad<876> _0;
    PtrVec<MarkObj> spawned;  // +876
};
static_assert(offsetof(MarkOwner, spawned) == 876);
struct Marker {
    be_u32 owner;     // +0: a MarkOwner
    pad<12> _4;
    Vec3 a;           // +16
    pad<4> _28;
    Vec3 b;           // +32
    pad<8> _44;
    be_f32 lift;      // +52
    pad<24> _56;
    be_u32 def;       // +80
    pad<4> _84;
    be_u32 state;     // +88
    pad<4> _92;
    be_u32 shown;     // +96: a MarkObj
    pad<32> _100;
    be_u32 k;         // +132
    u8 snap;          // +136
};
static_assert(offsetof(Marker, lift) == 52 && offsetof(Marker, def) == 80 && offsetof(Marker, shown) == 96);
static_assert(offsetof(Marker, k) == 132 && offsetof(Marker, snap) == 136);

namespace {
// .text: f32 25.0 (ray top y), 0 (x / z step), -50.0 (y step); lane masks
// y / z / w at +16 / +32 / +48; f32 0.1 (lift) at +64
constexpr u32 kMarkK = 0x00308180;
constexpr u32 kUp = 0x01164B60;       // Vec3: the up axis
constexpr u32 kIdentRot = 0x01164C50; // the global identity rotation (Mat33)
constexpr u32 kDebrisHash = 0x5F9739CF;
constexpr u32 Obj_SpawnToward307F70 = 0x00307F70;  // Obj* (Obj* a, Def* b, const Vec3* from, const Vec3* to)
constexpr u32 Obj_SpawnHashMtx307CC8 = 0x00307CC8; // Obj* (Owner*, u32 hash, const Mat34*)
constexpr u32 Mat34_FromRotPos553B4C = 0x00553B4C; // (Mat34* out, const Mat33*, const f32 p[3])
constexpr u32 HkWorld_CastRay92E2E8 = 0x0092E2E8;  // (hkpWorld*, const hkpWorldRayCastInput*, hkpWorldRayCastOutput*)

f64 KF(u32 off) { return lf(gm<be_f32>(kMarkK + off)); }

// x hidden: vt +40 (x, 0), again with 1 when (re-read) its flags have bit
// 0x00040000.
void Hide(Marker* m, MarkObj* x) {
    vcall(x, 40, 0u);
    const MarkObj& x2 = gm<MarkObj>(m->shown);  // re-read (unchecked)
    if (x2.flags & 0x00040000u) vcall(&x2, 40, 1u);
}

// The shown object is hidden and cleared.
void Release(Marker* m) {
    MarkObj* x = host<MarkObj>(m->shown);
    if (x == nullptr) return;
    Hide(m, x);
    m->shown = 0u;
}

// Shows y = Obj_SpawnToward307F70(m->owner, m->def, from, to) instead of
// the shown object (hidden as in Release); y gets refs += 1 and vt +104 (y,
// m->k) with a non-zero k. Nothing happens when y is already shown.
void Swap(Marker* m, const Vec3* from, const Vec3* to) {
    MarkObj* y = call<MarkObj*>(Obj_SpawnToward307F70, m->owner.get(), m->def.get(), from, to);
    MarkObj* x = host<MarkObj>(m->shown);
    if (x == y) return;
    if (x != nullptr) Hide(m, x);
    if (y != nullptr) {
        y->refs = static_cast<u16>(y->refs + 1);
        const u32 k = m->k;
        if (k != 0) vcall(y, 104, k);
    }
    m->shown = guest(y);
}
}  // namespace

// void Marker_SetState(Marker* m, u32 state) (called by f_00308C98 and
// Obj_DropMarker_0030A900): m->state = state, then
//  1: Swap from a to a + up;
//  3: Swap from b + up * lift to b;
//  2: Release;
//  4: with snap and a shown object x: a ray from (x.x, 25, x.z) to (x.x,
//     -25, x.z) (filter 13) and, on a hit, when the owner has room in its
//     spawned PtrVector (count below capacity): y = 0x00307CC8(owner,
//     0x5F9739CF, identity rotation at the hit point lifted by 0.1) gets
//     refs += 1 and is pushed (Reserve when full, high-water mark).
//     Release in every case.
// Other states do nothing more.
void Marker_ChangeState_003081C8(Marker* m, u32 state_) {
    const s32 state = static_cast<s32>(state_);
    m->state = state_;
    GuestFrame frame(256);  // the original's sp+112..
    const Vec3& up = global<Vec3>(kUp);
    if (state == 3) {
        Vec3* v = frame.as<Vec3>(16);
        const f64 s = lf(m->lift);
        v->x = static_cast<f32>(Fmadds(lf(up.x), s, lf(m->b.x)));
        v->y = static_cast<f32>(Fmadds(lf(up.y), s, lf(m->b.y)));
        v->z = static_cast<f32>(Fmadds(lf(up.z), s, lf(m->b.z)));
        Swap(m, v, &m->b);
    } else if (state == 1) {
        Vec3* v = frame.as<Vec3>(0);
        v->x = static_cast<f32>(Adds(lf(m->a.x), lf(up.x)));
        v->y = static_cast<f32>(Adds(lf(m->a.y), lf(up.y)));
        v->z = static_cast<f32>(Adds(lf(m->a.z), lf(up.z)));
        Swap(m, &m->a, v);
    } else if (state == 2) {
        Release(m);
    } else if (state == 4) {
        if (m->snap != 0 && m->shown != 0) {
            const Vec3& pos = gm<MarkObj>(m->shown).pos;
            const f64 dxz = KF(4), top = KF(0), dy = KF(8);
            const f64 px = lf(pos.x), pz = lf(pos.z);
            RayInput* in = frame.as<RayInput>(48);
            RayHit* out = frame.as<RayHit>(160);
            in->b32 = 0;
            in->w36 = 0u;
            in->w40 = 0u;
            set_bits(out->fraction, 0x3F800000u);
            out->w20 = 0xFFFFFFFFu;
            out->w64 = 0u;
            out->w32 = 0xFFFFFFFFu;
            out->shape = 0u;
            const Masks3 mk = LoadMasks3(kMarkK + 16);
            Stvx(&in->from, RowSelW(&pos.x, host<u8>(kMarkK), &pos.z, mk));
            in->w36 = 13u;
            Stvx(&in->to, RowSel(Fill4(static_cast<f32>(Adds(px, dxz))), Fill4(static_cast<f32>(Adds(top, dy))),
                                 Fill4(static_cast<f32>(Adds(pz, dxz))), mk));
            const u32 world = PhysWorld();
            call(HkWorld_CastRay92E2E8, world, in, out);
            if (out->shape != 0) {
                const f64 t = lf(out->fraction);
                Vec3* p = frame.as<Vec3>(32);
                p->x = static_cast<f32>(Fmadds(t, dxz, px));
                p->z = static_cast<f32>(Fmadds(t, dxz, pz));
                p->y = static_cast<f32>(Adds(Fmadds(t, dy, top), KF(64)));
                MarkOwner& own = gm<MarkOwner>(m->owner);  // unchecked
                PtrVec<MarkObj>& v = own.spawned;
                if (v.count < v.cap) {
                    u8* mtx = frame.as<u8>(96);
                    call(Mat34_FromRotPos553B4C, mtx, kIdentRot, p);
                    MarkObj* y = call<MarkObj*>(Obj_SpawnHashMtx307CC8, &own, kDebrisHash, mtx);
                    if (y != nullptr) {
                        y->refs = static_cast<u16>(y->refs + 1);
                        u32 n = v.count;
                        const u32 cap = v.cap;
                        if (n == cap) {
                            call(fn::PtrVector_Reserve, &v, cap << 1);
                            n = v.count;
                        }
                        gm<gp<MarkObj>>(v.data.ea + (n << 2)) = y;
                        const u32 c1 = v.count + 1u;
                        const u32 mx = v.mark;
                        v.count = static_cast<u16>(c1);
                        v.mark = static_cast<u16>(static_cast<s32>(mx) > static_cast<s32>(c1 & 0xFFFF) ? mx : c1);
                    }
                }
            }
        }
        Release(m);
    }
}

}  // namespace sly4::obj

NATIVE(003081C8, sly4::obj::Marker_ChangeState_003081C8)
