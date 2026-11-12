// Game code, large-function sweep part 26: a character's Havok rigid body
// (hkpCharacterRigidBody on a sphere + box list shape) and its four bone
// frame pairs.
#include "../sly4.h"
#include "big26.h"

using namespace sly4;
using namespace sly4::big26;

namespace {
constexpr u32 kTab247 = 0x00247020;       // .text pool: f32 0.5; masks {y} (+16), {z} (+32), {w} (+48); f32 0.1 (+64), -0.5 (+68)
constexpr u32 kVtTranslate = 0x00F1E7F8;  // stored over the translate shape after its constructor
constexpr u32 kBoxRadius = 0x00F9203C;    // f32 0.05 (the box shape's convex radius)
constexpr u32 kVtCrbCinfo = 0x00B13CCC;   // hkpCharacterRigidBodyCinfo vtable (name?)
constexpr u32 kUpVec = 0x01164B60;        // the global up vector
constexpr u32 kPropKeyTag = 0xA734004Du;  // body property: the constant 0x7A30D7E3
constexpr u32 kPropTagValue = 0x7A30D7E3u;
constexpr u32 kPropKeyOwner = 0x91C7CF5Bu;  // body property: the owning object

// A translate shape (48 bytes from the Havok allocator, its u16 +4 set even
// when the allocation failed) around child, offset by the vector v; r5 is
// what the original's indexing leaves there (the constructor does not read
// it). f1 is child->+16, read even when child is 0 (quirk).
u32 TranslateShape(PPUContext& ctx, u32 child, const V4& v, u32 r5, u32 spill) {
    const u32 t = HkAlloc(ctx, 48);
    if (t == 0) return 0;
    if (spill != 0) Stvx(spill, v);
    callf(ctx, b26fn::Hk_TranslateShapeCtor, {t, 10, r5, child, 0}, {Lf(child + 16)});
    store<u32>(t, kVtTranslate);
    Stvx(t + 32, Andc(spill != 0 ? Lvx(spill) : v, Lvx(kTab247 + 48)));
    store<u32>(t + 28, 0);
    return t;
}
}  // namespace

// T* f(Char* o) (called by Obj_Ctor_00247AB8, 0024B1E8). o->+216 the
// def (+780 a body to take the cinfo from, its byte +232 the kind; +784
// copied to o+420; +800/+804/+808 the size x/y/z; +816 the sphere drop;
// +820 the character body's +96 word; +824[4] words to o+528 + 144 i;
// +840[4] / +856[4] bone hashes). Without a def returns o at once.
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
//    corner points (+-(x/2 - 0.1), 0.1, +-(z/2 - 0.1)) are built on the
//    stack but only their 0.1 is used.
// Returns o+1040 when the last bone was found, else 0; f1 is 0.1 when
// either of the last pair was found, else def->+836 (what the last lfs
// leaves).
DECOMP(00247068, Char_CreateRigidBody_00247068) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 d = load<u32>(o + 216);
    if (d == 0) return;  // r3 = o
    Stack frame(ctx, 784);
    const u32 sp = frame.addr();
    const u32 ci = sp + 368;   // hkpRigidBodyCinfo
    const u32 arr = sp + 576;  // {sphere part, box part} for the list shape
    const u32 cc = sp + 256;   // the character rigid body cinfo
    const u32 world = load<u32>(load<u32>(load<u32>(b26fn::Game) + b26fn::GamePhys) + 4);
    store<u32>(o + 420, load<u32>(d + 784));
    call(ctx, b26fn::Hk_RigidBodyCinfoCtor, {ci});
    call(ctx, b26fn::Hk_CinfoFromBody, {load<u32>(d + 780), ci});
    const u8 kind = load<u8>(load<u32>(d + 780) + 232);
    if (kind == 5 || kind == 4) {
        VCall(ctx, o, 76, {o});
        store<u8>(ci + 180, 1);
        store<u32>(ci + 144, 0x3F800000u);
    }
    store<u8>(ci + 204, 1);
    store<u32>(arr, 0);
    store<u32>(arr + 4, 0);
    const Masks3 mk = LoadMasks3(kTab247 + 16);
    const f64 half = Lf(kTab247);

    // 2. Shapes.
    const f64 x0 = Lf(d + 800), z0 = Lf(d + 808);
    const f64 rad = Muls(Fsel(Subs(z0, x0), x0, z0), half);
    u32 sphere = 0;
    if (const u32 p = HkAlloc(ctx, 32); p != 0) {
        callf(ctx, b26fn::Hk_SphereShapeCtor, {p}, {rad});
        sphere = p;
    }
    const f64 lift = Subs(rad, Lf(d + 816));
    store<u32>(arr, TranslateShape(ctx, sphere, Andc(Andc(And26(Fill4(static_cast<f32>(lift)), mk.m[0]), mk.m[1]), mk.m[2]), kTab247, 0));
    const f64 hx = Muls(Lf(d + 800), half), hy = Muls(Lf(d + 804), half), hz = Muls(Lf(d + 808), half);
    u32 box = 0;
    if (const u32 p = HkAlloc(ctx, 48); p != 0) {
        Stvx(sp + 112, Andc(Sel(Sel(Fill4(static_cast<f32>(hx)), Fill4(static_cast<f32>(hy)), mk.m[0]), Fill4(static_cast<f32>(hz)), mk.m[1]), mk.m[2]));
        callf(ctx, b26fn::Hk_BoxShapeCtor, {p, sp + 112}, {Lf(kBoxRadius)});
        box = p;
    }
    store<u32>(arr + 4, TranslateShape(ctx, box, Andc(Andc(And26(Fill4(static_cast<f32>(hy)), mk.m[0]), mk.m[1]), mk.m[2]), 32, sp + 112));
    u32 list = 0;
    if (const u32 p = HkAlloc(ctx, 112); p != 0) {
        call(ctx, b26fn::Hk_ListShapeCtor_008947A0, {p, arr, 2, 0});
        list = p;
    }

    // 3. The character rigid body.
    store<u16>(cc + 6, 1);
    store<u32>(cc + 0, kVtCrbCinfo);
    store<u32>(cc + 88, 0x3F000000u);   // 0.5
    store<u32>(cc + 92, 0x41200000u);   // 10.0
    store<u32>(cc + 104, 0xA0FF0000u);  // vdb color
    store<u32>(cc + 12, list);
    store<u32>(cc + 8, (load<u32>(o + 424) << 16) | 8);
    Stvx(cc + 16, Lvx(ci + 16));
    store<u32>(cc + 80, 0x3F060A92u);  // 0.525
    Stvx(cc + 32, Lvx(ci + 32));
    store<u32>(cc + 56, 0x42C80000u);  // 100.0
    store<u32>(cc + 48, load<u32>(ci + 144));
    store<u32>(cc + 52, 0);
    store<u32>(cc + 84, 0);
    Stvx(cc + 64, RowSelW(kUpVec, kUpVec + 4, kUpVec + 8, mk));
    store<u32>(cc + 100, 0);
    store<u32>(cc + 96, load<u32>(d + 820));
    store<u32>(cc + 60, 0x3D4CCCCDu);  // 0.05
    u32 crb = 0;
    if (const u32 p = HkAlloc(ctx, 112); p != 0) {
        call(ctx, b26fn::Hk_CharRigidBodyCtor_009D32E0, {p, cc});
        crb = p;
    }
    store<u32>(o + 412, crb);
    if (crb != 0) {
        const u32 body = static_cast<u32>(call(ctx, b26fn::Hk_CharRigidBodyGetBody, {crb}));
        store<u32>(o + 416, body);
        if (body != 0) {
            const u32 name = static_cast<u32>(VCall(ctx, o, 76, {o}));
            call(ctx, b26fn::HkBody_SetName, {body + 120, name});
            for (u32 k = 0; k < 4; ++k) Stvx(sp + 192 + 16 * k, RowSelW(o + 96 + 16 * k, o + 100 + 16 * k, o + 104 + 16 * k, mk));
            call(ctx, b26fn::Hk_SetTransform, {load<u32>(o + 416), sp + 192});
            call(ctx, b26fn::Hk_AddProperty, {load<u32>(o + 416), kPropKeyTag, kPropTagValue});
            call(ctx, b26fn::Hk_AddProperty, {load<u32>(o + 416), kPropKeyOwner, o});
            call(ctx, b26fn::Hk_AddEntity, {world, load<u32>(o + 416), 1});
            call(ctx, b26fn::HkBody_AddListener, {load<u32>(o + 416), o + 388});
            store<u16>(load<u32>(o + 416) + 166, 0);
        }
    }
    call(ctx, b26fn::Hk_RemoveReference, {load<u32>(cc + 12)});

    // 4. Bone frames.
    const f64 x1 = Lf(d + 800), z1 = Lf(d + 808);
    const f64 m = Lf(kTab247 + 64), nh = Lf(kTab247 + 68);
    const f64 xa = Fmsubs(x1, half, m), za = Fmsubs(z1, half, m);
    const f64 xb = Fmadds(x1, nh, m), zb = Fmadds(z1, nh, m);
    const u32 pts = sp + 128;
    const f64 cx[4] = {xa, xb, xa, xb}, cz[4] = {za, za, zb, zb};
    for (u32 k = 0; k < 4; ++k) {
        Sf(pts + 16 * k, cx[k]);
        store<u32>(pts + 16 * k + 4, 0x3DCCCCCDu);  // 0.1
        Sf(pts + 16 * k + 8, cz[k]);
    }
    u32 r3 = 0;
    f64 f1 = 0;
    for (u32 i = 0; i < 4; ++i) {
        const u32 w = o + 144 * i;
        store<u32>(w + 528, load<u32>(d + 824 + 4 * i));
        f1 = Lf(d + 824 + 4 * i);
        const u32 b0 = static_cast<u32>(call(ctx, b26fn::Obj_FindBone, {o, load<u32>(d + 840 + 4 * i)}));
        const u32 h1 = load<u32>(d + 856 + 4 * i);
        if (b0 != 0) {
            CopyMat43W(w + 544, load<u32>(b0 + 4));
            store<u32>(w + 596, load<u32>(pts + 16 * i + 4));
            f1 = Lf(pts + 16 * i + 4);
        }
        const u32 b1 = static_cast<u32>(call(ctx, b26fn::Obj_FindBone, {o, h1}));
        r3 = b1;
        if (b1 != 0) {
            CopyMat43W(w + 608, load<u32>(b1 + 4));
            store<u32>(w + 660, load<u32>(pts + 16 * i + 4));
            f1 = Lf(pts + 16 * i + 4);
            r3 = w + 608;
        }
    }
    ctx.gpr[3] = r3;
    ctx.fpr[1] = f1;
}
