// Game code, large-function sweep part 17: the constructor of a physics
// object (vtable 0x00B18B48) that takes over the rigid body of its def's
// Havok template and configures it from the def.
#include "big18.h"

namespace sly4::obj {

namespace {
constexpr u32 Base_Ctor_000E02D8 = 0x000E02D8;   // (o, def): the base constructor (Thunk_000E02D8)
constexpr u32 Shape_Reset = 0x0063778C;          // (Shape*)
constexpr u32 Hk_SetTransform = 0x0091EAB8;      // (body, const f32 m[4][4])
constexpr u32 Hk_Activate = 0x0091A4C8;          // (body)
constexpr u32 HkBody_AddListenerA82C = 0x0091A82C;  // (body, listener)
constexpr u32 HkBody_AddListenerA5F8 = 0x0091A5F8;  // (body, listener)
constexpr u32 HkBody_SetName17 = 0x007CE950;     // (body + 120, const char* name) (Havok)
constexpr u32 Hk_SetMotionFlags17 = 0x0091D470;  // (body, 1, 1, 0) for keyframed / fixed bodies (Havok; name?)
constexpr u32 Hk_SetMass17 = 0x0091D618;         // (body; f1 mass) (Havok; name?)
constexpr u32 Hk_AddProperty17 = 0x0093FE00;     // (entity, key, u64 value) (Havok)

constexpr u32 kVt = 0x00B18B40;           // +8 the vtable, +416 / +456 the two listener vtables
constexpr u32 kPool = 0x001FA1F0;         // .text f32 0, lane masks at +16 / +32 / +48, f32 FLT_MAX at +64
constexpr u32 kUnknown = 0x00B0DA98;      // "<unknown>"
constexpr u32 kKeyFilter = 0x81D42737;    // property: collision filter info
constexpr u32 kKeyA = 0xA734004D, kValA = 0x7A30D7E3;
constexpr u32 kKeyObj = 0x91C7CF5B;       // property: the game object

// The physics def: rows +48..+96 the transform, +208 / +224 the linear /
// angular velocities, +240 a word, +244 the mass, +248 a f32 limit, u8
// +252 a flag.
struct PhysDef17 {
    pad<48> _0;
    Vec4 rows[4];      // +48 .. +96
    pad<96> _112;
    Vec4 linVel;       // +208
    Vec4 angVel;       // +224
    be_u32 w240;       // +240
    be_f32 mass;       // +244
    be_f32 limit;      // +248
    u8 b252;           // +252
};
static_assert(offsetof(PhysDef17, rows) == 48 && offsetof(PhysDef17, linVel) == 208 && offsetof(PhysDef17, b252) == 252);

// The Vector4 (w0, w1, w2, 0) of the three words at src (lvlx / vspltw /
// vsel / vandc with the lane masks; src word-aligned).
inline void Row(Vec4& d, const Vec4& s) {
    CopyXYZ(d, s);
    d.w = 0u;
}
}  // namespace

// void f(Obj* o, const Def* d) (no caller reads r3 or f1): the base
// constructor (0x000E02D8), vtables (+0, listeners +380 / +384),
// +400..+408 the zero vector, +416 / +420 / +428 = 0, +424 = -1, u16 +432
// low 4 bits kept; Shape_Reset(o +436). From the template t = o +216: +424
// = t +228, bit 0x80 of +432 = bit 0x40 of u8 t +301. When t +296 is set,
// the body (t +296)->vfunc 32() -> o +416, named by (o +160)->vfunc 12()
// (or "<unknown>"), its transform from the rows d +48 / +64 / +80 / +96; d
// +244 (mass) above 0: 0x0091D618(body; f1) (for a keyframed / fixed body
// without a filter property first +44 = 4, 0x0091D470(body, 1, 1, 0), +42
// = 4); exactly 0 for a dynamic body without a filter property: +44 = 2,
// 0x0091D470(body, 5, 1, 0), +42 = 0. d +248 below FLT_MAX: u16 body +510
// = its high half. Dynamic bodies get the linear / angular velocities d
// +208 / +224 (activating first); d +252 forces +42 = 2. Then the
// properties 0xA734004D = 0x7A30D7E3 and 0x91C7CF5B = o, and the two
// listeners. Always: +428 = d +240; then PhysObj_Finish_001F9F70(o, 1).
void PhysObj_Ctor_001FA238(PhysObj18* o, const PhysDef17* d) {
    GuestFrame frame(288);
    Vec4* vec = frame.as<Vec4>(112);  // Vector4 velocity
    Vec4* mat = frame.as<Vec4>(128);  // 4x4 transform (rows (x, y, z, 0))
    call(Base_Ctor_000E02D8, o, d);

    const u32 h = o->mode & 0x000Fu;
    const Vec3& zero = global<Vec3>(kZeroVec);
    o->vec.x = zero.x;
    o->vt = kVt + 8;
    o->vtA = kVt + 416;
    o->vtB = kVt + 456;
    o->vec.y = zero.y;
    o->body = nullptr;
    o->vec.z = zero.z;
    o->w420 = 0u;
    o->w424 = 0xBF800000u;
    o->w428 = 0u;
    o->mode = static_cast<u16>(h);
    const f64 k0 = lf(global<be_f32>(kPool));
    call(Shape_Reset, &o->volume[0]);

    PhysTmpl18* t = o->tmpl;
    o->w424 = t->w228;
    o->mode = static_cast<u16>((o->mode & ~0x80u) | ((t->flags.b[1] << 1) & 0x80u));
    if (const u32 tb = t->hk; tb != 0) {
        u32 body = vcall<u32>(host<u8>(tb), 32);
        o->body.ea = body;
        const u32 no = o->nameObj;
        const u32 name = no != 0 ? vcall<u32>(host<u8>(no), 12) : kUnknown;
        call(HkBody_SetName17, body + 120, name);
        for (u32 k = 0; k < 4; ++k) Row(mat[k], d->rows[k]);
        call(Hk_SetTransform, o->body.ea.get(), mat);

        const f64 mass = lf(d->mass);
        if (mass > k0) {
            body = o->body.ea;
            const u32 mt = gm<HkBody18>(body).mtype;
            if ((mt == 5 || mt == 4) && HkPropIdx(body, kKeyFilter) < 0) {
                gm<HkBody18>(body).filter = 4u;
                call(Hk_SetMotionFlags17, body, 1u, 1u, 0u);
                body = o->body.ea;
                gm<HkBody18>(body).b42 = 4;
            }
            call(Hk_SetMass17, body, lf(d->mass));
        } else if (mass == k0) {
            body = o->body.ea;
            const u32 mt = gm<HkBody18>(body).mtype;
            if (mt != 5 && mt != 4 && HkPropIdx(body, kKeyFilter) < 0) {
                gm<HkBody18>(body).filter = 2u;
                call(Hk_SetMotionFlags17, body, 5u, 1u, 0u);
                gm<HkBody18>(o->body.ea).b42 = 0;
            }
        }
        if (lf(d->limit) < lf(global<be_f32>(kPool + 64))) gm<HkBody18>(o->body.ea).h510 = static_cast<u16>(bits(d->limit) >> 16);

        body = o->body.ea;
        const u32 mt = gm<HkBody18>(body).mtype;
        if (mt != 5 && mt != 4) {
            Row(*vec, d->linVel);
            call(Hk_Activate, body);
            vcall(&gm<HkBody18>(body).motion[0], 68, vec);
            Row(*vec, d->angVel);
            const u32 b2 = o->body.ea;
            call(Hk_Activate, b2);
            vcall(&gm<HkBody18>(b2).motion[0], 72, vec);
            body = o->body.ea;
        }
        if (d->b252 != 0 && gm<HkBody18>(body).b42 != 2) gm<HkBody18>(body).b42 = 2;
        call(Hk_AddProperty17, body, static_cast<s32>(kKeyA), kValA);  // lis: sign-extended key
        call(Hk_AddProperty17, o->body.ea.get(), kKeyObj, o);
        call(HkBody_AddListenerA82C, o->body.ea.get(), &o->vtA);
        call(HkBody_AddListenerA5F8, o->body.ea.get(), &o->vtB);
    }
    o->w428 = d->w240;
    PhysObj_Finish_001F9F70(o, 1);
}

}  // namespace sly4::obj

NATIVE(001FA238, sly4::obj::PhysObj_Ctor_001FA238)
