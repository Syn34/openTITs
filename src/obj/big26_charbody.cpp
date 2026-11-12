// Game code, large-function sweep part 26: a character's Havok rigid body
// (hkpCharacterRigidBody on a sphere + box list shape) and its four bone
// frame pairs.
#include "big26.h"
#include "native/vec.h"

namespace sly4::obj {

namespace {
constexpr u32 kTab247 = 0x00247020;       // .text pool: f32 0.5; masks {y} (+16), {z} (+32), {w} (+48); f32 0.1 (+64), -0.5 (+68)
constexpr u32 kVtTranslate = 0x00F1E7F8;  // stored over the translate shape after its constructor
constexpr u32 kBoxRadius = 0x00F9203C;    // f32 0.05 (the box shape's convex radius)
constexpr u32 kVtCrbCinfo = 0x00B13CCC;   // hkpCharacterRigidBodyCinfo vtable (name?)
constexpr u32 kUpVec = 0x01164B60;        // the global up vector
constexpr u32 kPropKeyTag = 0xA734004Du;  // body property: the constant 0x7A30D7E3
constexpr u32 kPropTagValue = 0x7A30D7E3u;
constexpr u32 kPropKeyOwner = 0x91C7CF5Bu;  // body property: the owning object
constexpr u32 kTenth = 0x3DCCCCCDu;       // 0.1 (raw)

// The character rigid body cinfo (112 bytes, on the stack).
struct CrbCinfo {
    be_u32 vtable;   // +0
    pad<2> _4;
    be_u16 h6;       // +6
    be_u32 filter;   // +8
    be_u32 shape;    // +12
    Vec4 pos;        // +16
    Vec4 rot;        // +32
    be_u32 mass;     // +48
    be_u32 w52;      // +52
    be_u32 w56;      // +56: 100.0
    be_u32 w60;      // +60: 0.05
    Vec4 up;         // +64
    be_u32 w80;      // +80: 0.525
    be_u32 w84;      // +84
    be_u32 w88;      // +88: 0.5
    be_u32 w92;      // +92: 10.0
    be_u32 w96;      // +96
    be_u32 w100;     // +100
    be_u32 color;    // +104
    pad<4> _108;
};
static_assert(sizeof(CrbCinfo) == 112 && offsetof(CrbCinfo, up) == 64 && offsetof(CrbCinfo, color) == 104);
// The 48-byte translate shape: +28, +32 the translation.
struct CbTranslate {
    be_u32 vtable;   // +0
    pad<24> _4;
    be_u32 w28;      // +28
    Vec4 t;          // +32
};
struct CharDef {
    pad<780> _0;
    be_u32 body;        // +780: a body to take the cinfo from (u8 +232 its kind)
    be_u32 w784;        // +784
    pad<12> _788;
    Vec3 size;          // +800
    pad<4> _812;
    be_f32 drop;        // +816
    be_u32 w820;        // +820
    be_u32 w824[4];     // +824
    be_u32 bone_a[4];   // +840
    be_u32 bone_b[4];   // +856
};
static_assert(offsetof(CharDef, size) == 800 && offsetof(CharDef, bone_a) == 840 && offsetof(CharDef, bone_b) == 856);
}  // namespace

// One of the four 144-byte frame slots at o+528.
struct CharSlot {
    be_u32 w;       // +0
    pad<12> _4;
    Mtx44 a;        // +16
    Mtx44 b;        // +80
};
static_assert(sizeof(CharSlot) == 144);
struct CharObj {
    pad<96> _0;
    Mtx44 m;            // +96
    pad<56> _160;
    be_u32 def;         // +216: CharDef*
    pad<168> _220;
    be_u32 listener;    // +388
    pad<20> _392;
    be_u32 crb;         // +412
    be_u32 body;        // +416
    be_u32 w420;        // +420
    be_u32 w424;        // +424
    pad<100> _428;
    CharSlot slots[4];  // +528
};
static_assert(offsetof(CharObj, def) == 216 && offsetof(CharObj, crb) == 412 && offsetof(CharObj, slots) == 528);
namespace {
// The original's locals (sp offsets - 112).
struct CharLocals {
    Vec4 half;       // sp+112
    pad<64> _128;    // sp+128: the corner points (only their 0.1 is used)
    Mtx44 xf;        // sp+192
    CrbCinfo cc;     // sp+256
    HkRbCinfo ci;    // sp+368
    be_u32 arr[2];   // sp+576: {sphere part, box part}
};
static_assert(offsetof(CharLocals, cc) == 144 && offsetof(CharLocals, ci) == 256 && offsetof(CharLocals, arr) == 464);

// A translate shape (48 bytes from the Havok allocator, its u16 +4 set even
// when the allocation failed) around child, offset by the vector v. f1 is
// child->+16, read even when child is 0 (quirk).
u32 TranslateShape(u32 child, const V4& v, const V4& wmask) {
    const u32 t = HkAlloc(48);
    if (t == 0) return 0;
    call(b26fn::Hk_TranslateShapeCtor, t, 10u, lf(gm<be_f32>(child + 16)), child, 0u);
    CbTranslate& ts = gm<CbTranslate>(t);
    ts.vtable = kVtTranslate;
    Stvx(&ts.t, Andc(v, wmask));
    ts.w28 = 0u;
    return t;
}
}  // namespace

// void f(Char* o) (called by Obj_Ctor_00247AB8, 0024B1E8). o->+216 the
// def (+780 a body to take the cinfo from, its byte +232 the kind; +784
// copied to o+420; +800/+804/+808 the size x/y/z; +816 the sphere drop;
// +820 the character body's +96 word; +824[4] words to o+528 + 144 i;
// +840[4] / +856[4] bone hashes). Nothing without a def.
// 1. A rigid body cinfo from def->+780 (kind 4 / 5: o->vfn[76](), then
//    cinfo byte +180 = 1 and +144 = 1.0); cinfo byte +204 = 1.
// 2. Shapes: a sphere of radius min(x, z) / 2 translated by (0, radius -
//    drop, 0) and a box of half extents size / 2 translated by (0, y / 2,
//    0), both in a two-entry list shape (008947A0).
// 3. The character rigid body (112 bytes, 009D32E0) from a stack cinfo
//    (vtable 0x00B13CCC; filter (o->+424 << 16) | 8, the list shape, the
//    rigid cinfo's position / rotation and +144, 100.0, 0.05, up vector,
//    0.525, 0.5, 10.0, def->+820, vdb color 0xA0FF0000) into o+412; its
//    rigid body into o+416. With one: named o->vfn[76](), transform from
//    o's rows +96..+144, properties 0xA734004D = 0x7A30D7E3 and 0x91C7CF5B
//    = o, added to the world (Game->+0x1281D4->+4), listener o+388, u16
//    +166 = 0. The list shape is released.
// 4. For i < 4: the bones def->+840[i] / +856[i] give o's frames at +544 /
//    +608 + 144 i (x/y/z of the four rows) plus 0.1 into +596 / +660; the
//    corner points (+-(x/2 - 0.1), 0.1, +-(z/2 - 0.1)) the original builds
//    on the stack are only read for their 0.1, so only that is kept.
// No caller reads r3 or f1 (r3.sh).
void Char_CreateRigidBody_00247068(CharObj* o) {
    const u32 dea = o->def;
    if (dea == 0) return;
    const CharDef& d = gm<CharDef>(dea);
    GuestFrame frame(784);
    CharLocals& L = *frame.as<CharLocals>(112);
    HkRbCinfo& ci = L.ci;
    CrbCinfo& cc = L.cc;
    const u32 world = gm<be_u32>(gm<be_u32>(global<be_u32>(kWorld) + 0x001281D4) + 4);
    o->w420 = d.w784.get();
    call(b26fn::Hk_RigidBodyCinfoCtor, &ci);
    call(b26fn::Hk_CinfoFromBody, d.body.get(), &ci);
    const u8 kind = gm<u8>(d.body + 232);
    if (kind == 5 || kind == 4) {
        vcall(o, 76);
        ci.b180 = 1;
        ci.w144 = 0x3F800000u;
    }
    ci.b204 = 1;
    L.arr[0] = 0u;
    L.arr[1] = 0u;
    const Masks3 mk = LoadMasks3(kTab247 + 16);
    const f64 half = lf(global<be_f32>(kTab247));

    // 2. Shapes.
    const f64 x0 = lf(d.size.x), z0 = lf(d.size.z);
    const f64 rad = Muls(Fsel(Subs(z0, x0), x0, z0), half);
    u32 sphere = 0;
    if (const u32 p = HkAlloc(32); p != 0) {
        call(b26fn::Hk_SphereShapeCtor, p, rad);
        sphere = p;
    }
    const f64 lift = Subs(rad, lf(d.drop));
    auto yonly = [&](f64 v) { return Andc(Andc(And4(Fill4(static_cast<f32>(v)), mk.m[0]), mk.m[1]), mk.m[2]); };
    L.arr[0] = TranslateShape(sphere, yonly(lift), mk.m[2]);
    const f64 hx = Muls(lf(d.size.x), half), hy = Muls(lf(d.size.y), half), hz = Muls(lf(d.size.z), half);
    u32 box = 0;
    if (const u32 p = HkAlloc(48); p != 0) {
        Stvx(&L.half, RowSel(Fill4(static_cast<f32>(hx)), Fill4(static_cast<f32>(hy)), Fill4(static_cast<f32>(hz)), mk));
        call(b26fn::Hk_BoxShapeCtor, p, &L.half, lf(global<be_f32>(kBoxRadius)));
        box = p;
    }
    L.arr[1] = TranslateShape(box, yonly(hy), mk.m[2]);
    u32 list = 0;
    if (const u32 p = HkAlloc(112); p != 0) {
        call(b26fn::Hk_ListShapeCtor_008947A0, p, L.arr, 2u, 0u);
        list = p;
    }

    // 3. The character rigid body.
    cc.h6 = 1;
    cc.vtable = kVtCrbCinfo;
    cc.w88 = 0x3F000000u;    // 0.5
    cc.w92 = 0x41200000u;    // 10.0
    cc.color = 0xA0FF0000u;  // vdb color
    cc.shape = list;
    cc.filter = (o->w424 << 16) | 8u;
    Stvx(&cc.pos, Lvx(&ci.pos));
    cc.w80 = 0x3F060A92u;    // 0.525
    Stvx(&cc.rot, Lvx(&ci.rot));
    cc.w56 = 0x42C80000u;    // 100.0
    cc.mass = ci.w144.get();
    cc.w52 = 0u;
    cc.w84 = 0u;
    const Vec3& up = global<Vec3>(kUpVec);
    Stvx(&cc.up, RowSelW(&up.x, &up.y, &up.z, mk));
    cc.w100 = 0u;
    cc.w96 = d.w820.get();
    cc.w60 = 0x3D4CCCCDu;    // 0.05
    u32 crb = 0;
    if (const u32 p = HkAlloc(112); p != 0) {
        call(b26fn::Hk_CharRigidBodyCtor_009D32E0, p, &cc);
        crb = p;
    }
    o->crb = crb;
    if (crb != 0) {
        const u32 body = call<u32>(b26fn::Hk_CharRigidBodyGetBody, crb);
        o->body = body;
        if (body != 0) {
            const u32 name = vcall<u32>(o, 76);
            call(b26fn::HkBody_SetName, body + 120, name);
            for (u32 k = 0; k < 4; ++k) Stvx(&L.xf.r[k], RowSelW(&o->m.r[k].x, &o->m.r[k].y, &o->m.r[k].z, mk));
            call(b26fn::Hk_SetTransform, o->body.get(), &L.xf);
            call(b26fn::Hk_AddProperty, o->body.get(), kPropKeyTag, kPropTagValue);
            call(b26fn::Hk_AddProperty, o->body.get(), kPropKeyOwner, o);
            call(b26fn::Hk_AddEntity, world, o->body.get(), 1u);
            call(b26fn::HkBody_AddListener, o->body.get(), &o->listener);
            gm<be_u16>(o->body + 166) = 0;
        }
    }
    call(b26fn::Hk_RemoveReference, cc.shape.get());

    // 4. Bone frames.
    for (u32 i = 0; i < 4; ++i) {
        CharSlot& w = o->slots[i];
        w.w = d.w824[i].get();
        const u32 b0 = call<u32>(b26fn::Obj_FindBone, o, d.bone_a[i].get());
        const u32 h1 = d.bone_b[i];
        if (b0 != 0) {
            CopyMtxXYZ(w.a, gm<Mtx44>(gm<be_u32>(b0 + 4)));
            set_bits(w.a.r[3].y, kTenth);
        }
        const u32 b1 = call<u32>(b26fn::Obj_FindBone, o, h1);
        if (b1 != 0) {
            CopyMtxXYZ(w.b, gm<Mtx44>(gm<be_u32>(b1 + 4)));
            set_bits(w.b.r[3].y, kTenth);
        }
    }
}

}  // namespace sly4::obj

NATIVE(00247068, sly4::obj::Char_CreateRigidBody_00247068)
