// Game code, large-function sweep part 27: the 304-byte collision object
// constructor (a rigid body from a named physics object or a box shape).
#include "big27.h"
#include "native/vec.h"

namespace sly4::obj {

namespace {
constexpr u32 kPool1F8 = 0x001F85B0;     // .text pool: f32 0.0; masks {y} (+16), {z} (+32), {w} (+48)
constexpr u32 kVtColObj = 0x00B18B00;    // the collision object's vtable
constexpr u32 kPropKeys = 0x00AD2B40;    // u32[10]: property keys copied from the named object
constexpr u32 kPropMotion = 0x81D42737u; // property whose value goes to body+44 (else from the flags / kind)
constexpr u32 kPropFlag = 0x67E6342Bu;   // property set to 1 when o+284 is one of the two hashes below
constexpr u32 kTypeHashA = 0x29846DDAu;
constexpr u32 kTypeHashB = 0x4438A28Fu;
constexpr u32 Def_BaseCtor = 0x000C3078;            // (Def*, rec) (Thunk_000C3078)
constexpr u32 Obj_FindName = 0x0034654C;            // (table, name) -> object or 0
constexpr u32 Hk_RigidBodyCinfoCtor = 0x0091FB50;   // (hkpRigidBodyCinfo*)
constexpr u32 Hk_CinfoFromBody = 0x0091D208;        // (body, hkpRigidBodyCinfo*)
constexpr u32 Hk_RigidBodyCtor_0091E100 = 0x0091E100;  // (hkpRigidBody*, const Cinfo*) (name?)
constexpr u32 Hk_RemoveEntity = 0x0092FFC8;         // (out*, world, entity)
constexpr u32 Hk_BoxShapeCtor = 0x00899720;         // (hkpBoxShape*, const hkVector4* halfExtents; f1 radius)
constexpr u32 Hk_TranslateShapeCtor = 0x00898B30;   // (shape, 10; f1 child radius, child, u8)
constexpr u32 HkBody_SetName = 0x007CE950;          // (body + 120, const char*)
constexpr u32 Hk_RemoveReference = 0x007D4EA8;      // (hkReferencedObject*)
constexpr u32 Hk_SetMotion = 0x0091D470;            // (body, type, 1, 0)
constexpr u32 Hk_SetMass = 0x0091D618;              // (body; f1 mass)
constexpr u32 Hk_AddProperty = 0x0093FE00;          // (entity, u32 key, u64 value)
constexpr u32 VtTranslate = 0x00F1E7F8;             // stored over a translate shape after its constructor
constexpr u32 BoxRadius = 0x00F9203C;               // f32 0.05 (box convex radius)

// The 256-byte description record.
struct ColObjDef {
    pad<76> _0;
    be_u32 w76;        // +76
    pad<48> _80;
    Vec3 box;          // +128: the box half extents
    pad<4> _140;
    Vec3 v144;         // +144
    pad<4> _156;
    be_u32 name;       // +160: const char* of the named physics object
    be_f32 mass;       // +164
    be_f32 lim[4];     // +168 .. +180: cinfo values (used when >= 0)
    be_u32 w184[16];   // +184 .. +244 (copied to the object's +224..)
    u8 b248[4];        // +248 (bytes 4 / 5 are +252 / +253)
    WordBytes f252;    // +252: flag bits
};
static_assert(offsetof(ColObjDef, w184) == 184 && offsetof(ColObjDef, f252) == 252);
// The 304-byte collision object.
struct ColObj304 {
    be_u32 vtable;     // +0
    pad<188> _4;
    Vec3 box;          // +192
    pad<4> _204;
    Vec3 v208;         // +208
    pad<4> _220;
    be_u32 w224[16];   // +224 .. +284 (w224[8] = +256 a float, w224[15] = +284 the type hash)
    u8 b288[6];        // +288
    pad<2> _294;
    be_u32 body;       // +296
    WordBytes flags;   // +300
};
static_assert(sizeof(ColObj304) == 304 && offsetof(ColObj304, w224) == 224 && offsetof(ColObj304, body) == 296);
// The 48-byte translate shape: +28, +32 the translation.
struct HkTranslate {
    be_u32 vtable;   // +0
    pad<24> _4;
    be_u32 w28;      // +28
    Vec4 t;          // +32
};
// The 544-byte rigid body: u8 +42 the quality, +44 the motion kind, +120
// its name, u8 +232 its motion type.
struct RbBody {
    pad<42> _0;
    u8 quality;      // +42
    pad<1> _43;
    be_u32 motion;   // +44
    pad<184> _48;
    u8 kind;         // +232
};
static_assert(offsetof(RbBody, motion) == 44 && offsetof(RbBody, kind) == 232);
}  // namespace

// void f(ColObj* o, const Rec256* d) (called by ColObj_New_001F9BA0,
// 001FDF7C). Def base constructor (0x000C3078), then copies from d:
// +128..+136 -> o+192..200 (box size), +144..+152 -> o+208..216,
// +184..+240 -> o+224..280, +244 -> o+284 (a type hash), the flag bits
// 0x8000 >> b of d+252 into o+300 bits 0x80000000 >> b (b < 8), d+252
// bits 0x80..0x08 into the byte o+301, d+248[6] -> o+288[6]; vtable
// 0x00B18B00, o+296 (the body) = 0. o+256 below 0 becomes d+76.
// Without the physics table (Game->+0x1281D4) returns. A rigid body cinfo
// on the stack comes either from the object named d+160 in that table
// (none: return 0; it leaves its world first, its shape is kept) or, with
// no name, from a box of half extents o+192 (all zero: return 0) inside a
// translate shape offset by (0, o+196, 0). Cinfo fields: d+168/+172/+176/
// +180 when >= 0, 1.0, o+224, the identity inertia; byte +204 = 1 when any
// of o+300 bits 0x7E000000. The 544-byte rigid body goes to o+296 (named
// after the object). Named: the ten properties of kPropKeys the object has
// are copied, and kPropFlag = 1 for the two type hashes. Mass d+164: < 0
// (or NaN) keyed motion 5 (byte +42 = 0); for a body of kind 4 / 5
// nothing; == 0 motion 5 (byte +42 = 0); > 0 motion 1 (byte +42 = 4) and
// that mass. body+44 = the object's kPropMotion value, else 5 for flag
// 0x80000000, else 2 / 3 / 4 for kind 5 / 4 / other. No caller reads r3
// or f1 (r3.sh).
void ColObj_Ctor1F85F0(ColObj304* o, const ColObjDef* d) {
    GuestFrame frame(448);
    Vec4* tmp = frame.as<Vec4>(112);
    HkRbCinfo& ci = *frame.as<HkRbCinfo>(128);
    call(Def_BaseCtor, o, d);
    CopyXYZ(o->box, d->box);
    CopyXYZ(o->v208, d->v144);
    for (u32 k = 0; k < 7; ++k) o->w224[k] = d->w184[k].get();  // +224 .. +248
    {
        const u32 w = d->w184[7];
        o->vtable = kVtColObj;
        o->w224[7] = w;  // +252
    }
    for (u32 k = 8; k < 13; ++k) o->w224[k] = d->w184[k].get();  // +256 .. +272
    {
        // lfs d+236 / lwz d+244 / stfs o+276 / lfs d+240 / stw o+284 / stfs o+280
        const u32 w236 = d->w184[13], w244 = d->w184[15];
        o->w224[13] = w236;
        const u32 w240 = d->w184[14];
        o->w224[15] = w244;
        o->w224[14] = w240;
    }
    u32 fl = o->flags.w;
    o->body = 0u;
    fl &= 0x7FFFFFFFu;
    for (u32 b = 0; b < 8; ++b) {
        const u32 m = 0x80000000u >> b;
        fl = (fl & ~m) | (std::rotl(d->f252.w.get(), 16) & m);
        o->flags.w = fl;
    }
    u32 by = (fl >> 16) & 0xFF;
    for (u32 k = 7; k >= 3; --k) {
        by = (by & ~(1u << k)) | (((d->f252.w >> k) & 1) << k);
        o->flags.b[1] = static_cast<u8>(by);
    }
    const u32 w76 = d->w76;
    for (u32 k = 0; k < 6; ++k) o->b288[k] = k < 4 ? d->b248[k] : d->f252.b[k - 4];
    const f64 zero = lf(global<be_f32>(kPool1F8));
    if (LfsW(o->w224[8]) < zero) o->w224[8] = w76;

    u32 named = gm<be_u32>(global<be_u32>(kWorld) + 0x001281D4);
    if (named == 0) return;
    call(Hk_RigidBodyCinfoCtor, &ci);
    const Masks3 mk = LoadMasks3(kPool1F8 + 16);
    bool release = false;  // r28: release the cinfo's shape after the body is built
    const u32 name = d->name;
    if (name != 0 && gm<u8>(name) != 0) {
        named = call<u32>(Obj_FindName, named, name);
        if (named == 0) return;
        if (const u32 w = gm<be_u32>(named + 8); w != 0) call(Hk_RemoveEntity, frame.as<u8>(336), w, named);
        call(Hk_CinfoFromBody, named, &ci);
    } else {
        // No name: a box of half extents o+192..200 unless that is the zero vector.
        const Vec3& zv = global<Vec3>(kZeroVec);
        if (bits(o->box.x) == bits(zv.x) && bits(o->box.y) == bits(zv.y) && bits(o->box.z) == bits(zv.z)) return;
        const V4 y0 = SplatW(&o->box.y);  // read before the allocation
        u32 box = 0;
        if (const u32 p = HkAlloc(48); p != 0) {
            Stvx(tmp, RowSelW(&o->box.x, &o->box.y, &o->box.z, mk));
            call(Hk_BoxShapeCtor, p, tmp, lf(global<be_f32>(BoxRadius)));
            box = p;
        }
        u32 tr = 0;
        if (const u32 t = HkAlloc(48); t != 0) {
            Stvx(tmp, Andc(Andc(And4(y0, mk.m[0]), mk.m[1]), mk.m[2]));
            // f1 = box->+16, read even when the box allocation failed (quirk).
            call(Hk_TranslateShapeCtor, t, 10u, lf(gm<be_f32>(box + 16)), box, 1u);
            HkTranslate& ts = gm<HkTranslate>(t);
            ts.vtable = VtTranslate;
            Stvx(&ts.t, Andc(Lvx(tmp), mk.m[2]));
            ts.w28 = 0u;
            tr = t;
        }
        ci.shape = tr;
        call(Hk_RemoveReference, box);  // even a failed (0) box (quirk)
        release = true;
        named = 0;
    }

    // Cinfo fields.
    {
        be_u32* dst[4] = {&ci.w160, &ci.w168, &ci.w148, &ci.w152};
        for (u32 k = 0; k < 4; ++k)
            if (lf(d->lim[k]) >= zero) *dst[k] = bits(d->lim[k]);
    }
    ci.w144 = 0x3F800000u;
    ci.w156 = o->w224[0].get();
    ci.b180 = 1;
    for (Vec4& r : ci.inertia) Stvx(&r, Zero4());
    set_bits(ci.inertia[0].x, 0x3F800000u);
    set_bits(ci.inertia[1].y, 0x3F800000u);
    set_bits(ci.inertia[2].z, 0x3F800000u);
    {
        const u32 f = o->flags.w;
        bool any = false;  // flag bits 0x40000000 >> 0..5
        for (u32 b = 1; b <= 6; ++b) any = any || (f & (0x80000000u >> b)) != 0;
        if (any) ci.b204 = 1;
    }

    u32 body = 0;
    if (const u32 p = HkAlloc(544); p != 0) {
        call(Hk_RigidBodyCtor_0091E100, p, &ci);
        body = p;
    }
    o->body = body;
    if (named != 0) call(HkBody_SetName, body + 120, gm<be_u32>(named + 120) & ~1u);
    if (release) {
        if (const u32 sh = ci.shape; sh != 0) call(Hk_RemoveReference, sh);
    }
    if (named != 0) {
        for (u32 i = 0; i < 10; ++i) {
            const u32 key = global<be_u32>(kPropKeys + 4 * i);
            const s32 j = HkPropIdx(named, key);
            if (j < 0) continue;
            call(Hk_AddProperty, o->body.get(), key, HkPropVal64(named, j));
        }
        const u32 th = o->w224[15];
        if (th == kTypeHashA || th == kTypeHashB) call(Hk_AddProperty, o->body.get(), kPropFlag, u64{1});
    }

    // Motion and mass.
    const f64 mass = lf(d->mass);
    if (!(mass >= zero)) {
        call(Hk_SetMotion, o->body.get(), 5u, 1u, 0u);
        gm<RbBody>(o->body).quality = 0;
    } else {
        const u32 b = o->body;
        const u8 kind = gm<RbBody>(b).kind;
        if (kind != 5 && kind != 4) {
            if (mass != zero) {
                call(Hk_SetMotion, b, 1u, 1u, 0u);
                gm<RbBody>(o->body).quality = 4;
                call(Hk_SetMass, o->body.get(), mass);
            } else {
                call(Hk_SetMotion, b, 5u, 1u, 0u);
                gm<RbBody>(o->body).quality = 0;
            }
        }
    }
    body = o->body;
    u32 q = 4;
    s32 j = -1;
    if (named != 0) j = HkPropIdx(named, kPropMotion);
    if (j >= 0) {
        q = static_cast<u32>(HkPropVal64(named, j));  // low word of the u64 value
    } else if ((o->flags.w & 0x80000000u) != 0) {
        q = 5;
    } else {
        const u8 kind = gm<RbBody>(body).kind;
        q = kind == 5 ? 2 : kind == 4 ? 3 : 4;
    }
    gm<RbBody>(body).motion = q;
}

}  // namespace sly4::obj

NATIVE(001F85F0, sly4::obj::ColObj_Ctor1F85F0)
