// Game code, large-function sweep part 27: the 304-byte collision object
// constructor (a rigid body from a named physics object or a box shape).
#include "../sly4.h"
#include "big27.h"

using namespace sly4;
using namespace sly4::big27;

namespace {
constexpr u32 kPool1F8 = 0x001F85B0;     // .text pool: f32 0.0; masks {y} (+16), {z} (+32), {w} (+48)
constexpr u32 kVtColObj = 0x00B18B00;    // the collision object's vtable
constexpr u32 kPropKeys = 0x00AD2B40;    // u32[10]: property keys copied from the named object
constexpr u32 kPropMotion = 0x81D42737u; // property whose value goes to body+44 (else from the flags / kind)
constexpr u32 kPropFlag = 0x67E6342Bu;   // property set to 1 when o+284 is one of the two hashes below
constexpr u32 kTypeHashA = 0x29846DDAu;
constexpr u32 kTypeHashB = 0x4438A28Fu;
}  // namespace

// T* f(ColObj* o, const Rec256* d) (called by ColObj_New_001F9BA0,
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
// 0x80000000, else 2 / 3 / 4 for kind 5 / 4 / other. Returns the body.
DECOMP(001F85F0, ColObj_Ctor1F85F0) {
    const u32 o = arg<u32>(ctx, 0), d = arg<u32>(ctx, 1);
    Stack frame(ctx, 448);
    const u32 sp = frame.addr();
    const u32 ci = sp + 128;  // hkpRigidBodyCinfo (shape at +4)
    call(ctx, b27fn::Def_BaseCtor, {o, d});
    for (u32 k = 0; k < 12; k += 4) store<u32>(o + 192 + k, load<u32>(d + 128 + k));
    for (u32 k = 0; k < 12; k += 4) store<u32>(o + 208 + k, load<u32>(d + 144 + k));
    for (u32 k = 0; k < 8; k += 4) store<u32>(o + 224 + k, load<u32>(d + 184 + k));
    store<u32>(o + 232, load<u32>(d + 192));
    store<u32>(o + 236, load<u32>(d + 196));
    store<u32>(o + 240, load<u32>(d + 200));
    store<u32>(o + 244, load<u32>(d + 204));
    store<u32>(o + 248, load<u32>(d + 208));
    {
        const u32 w = load<u32>(d + 212);
        store<u32>(o + 0, kVtColObj);
        store<u32>(o + 252, w);
    }
    for (u32 k = 0; k < 20; k += 4) store<u32>(o + 256 + k, load<u32>(d + 216 + k));
    {
        // lfs d+236 / lwz d+244 / stfs o+276 / lfs d+240 / stw o+284 / stfs o+280
        const u32 w236 = load<u32>(d + 236);
        const u32 w244 = load<u32>(d + 244);
        store<u32>(o + 276, w236);
        const u32 w240 = load<u32>(d + 240);
        store<u32>(o + 284, w244);
        store<u32>(o + 280, w240);
    }
    u32 fl = load<u32>(o + 300);
    store<u32>(o + 296, 0);
    fl &= 0x7FFFFFFFu;
    for (u32 b = 0; b < 8; ++b) {
        const u32 m = 0x80000000u >> b;
        fl = (fl & ~m) | (std::rotl(load<u32>(d + 252), 16) & m);
        store<u32>(o + 300, fl);
    }
    u32 by = (fl >> 16) & 0xFF;
    for (u32 k = 7; k >= 3; --k) {
        by = (by & ~(1u << k)) | (((load<u32>(d + 252) >> k) & 1) << k);
        store<u8>(o + 301, static_cast<u8>(by));
    }
    const u32 w76 = load<u32>(d + 76);
    for (u32 k = 0; k < 6; ++k) store<u8>(o + 288 + k, load<u8>(d + 248 + k));
    const f64 zero = Lf(kPool1F8);
    if (Lf(o + 256) < zero) store<u32>(o + 256, w76);
    SetF1Bits(ctx, w76);  // leftover f1 on the early returns

    const u32 game = load<u32>(b27fn::Game);
    u32 named = load<u32>(game + b27fn::GamePhys);
    if (named == 0) {
        ret(ctx, game);  // r3 = the Game pointer the lookup loaded
        return;
    }
    call(ctx, b27fn::Hk_RigidBodyCinfoCtor, {ci});
    const Masks3 mk = LoadMasks3(kPool1F8 + 16);
    bool release = false;  // r28: release the cinfo's shape after the body is built
    const u32 name = load<u32>(d + 160);
    if (name != 0 && load<u8>(name) != 0) {
        named = static_cast<u32>(call(ctx, b27fn::Obj_FindName, {named, name}));
        if (named == 0) {
            ret(ctx, 0u);
            return;
        }
        if (const u32 w = load<u32>(named + 8); w != 0) call(ctx, b27fn::Hk_RemoveEntity, {sp + 336, w, named});
        call(ctx, b27fn::Hk_CinfoFromBody, {named, ci});
    } else {
        // No name: a box of half extents o+192..200 unless that is the zero vector.
        if (load<u64>(o + 192) == load<u64>(b27fn::ZeroVec) && load<u32>(o + 200) == load<u32>(b27fn::ZeroVec + 8)) {
            ret(ctx, 0u);
            return;
        }
        const V4 y0 = SplatW(o + 196);  // read before the allocation
        u32 box = 0;
        if (const u32 p = HkAlloc(ctx, 48); p != 0) {
            Stvx(sp + 112, Andc(Sel(Sel(SplatW(o + 192), SplatW(o + 196), mk.m[0]), SplatW(o + 200), mk.m[1]), mk.m[2]));
            callf(ctx, b27fn::Hk_BoxShapeCtor, {p, sp + 112}, {Lf(b27fn::BoxRadius)});
            box = p;
        }
        u32 tr = 0;
        if (const u32 t = HkAlloc(ctx, 48); t != 0) {
            Stvx(sp + 112, Andc(Andc(And26(y0, mk.m[0]), mk.m[1]), mk.m[2]));
            // f1 = box->+16, read even when the box allocation failed (quirk);
            // r5 = 48 is the original's leftover vector index (unread).
            callf(ctx, b27fn::Hk_TranslateShapeCtor, {t, 10, 48, box, 1}, {Lf(box + 16)});
            store<u32>(t, b27fn::VtTranslate);
            Stvx(t + 32, Andc(Lvx(sp + 112), mk.m[2]));
            store<u32>(t + 28, 0);
            tr = t;
        }
        store<u32>(ci + 4, tr);
        call(ctx, b27fn::Hk_RemoveReference, {box});  // even a failed (0) box (quirk)
        release = true;
        named = 0;
    }

    // Cinfo fields.
    for (u32 k = 0; k < 4; ++k) {
        static constexpr u32 kSrc[4] = {168, 172, 176, 180}, kDst[4] = {160, 168, 148, 152};
        const u32 w = load<u32>(d + kSrc[k]);
        if (Lf(d + kSrc[k]) >= zero) store<u32>(ci + kDst[k], w);
    }
    store<u32>(ci + 144, 0x3F800000u);
    store<u32>(ci + 156, load<u32>(o + 224));
    store<u8>(ci + 180, 1);
    Stvx(ci + 80, Zero4());
    Stvx(ci + 96, Zero4());
    Stvx(ci + 112, Zero4());
    store<u32>(ci + 80, 0x3F800000u);
    store<u32>(ci + 100, 0x3F800000u);
    store<u32>(ci + 120, 0x3F800000u);
    {
        const u32 f = load<u32>(o + 300);
        bool any = false;  // flag bits 0x40000000 >> 0..5
        for (u32 b = 1; b <= 6; ++b) any = any || (f & (0x80000000u >> b)) != 0;
        if (any) store<u8>(ci + 204, 1);
    }

    u32 body = 0;
    if (const u32 p = HkAlloc(ctx, 544); p != 0) {
        call(ctx, b27fn::Hk_RigidBodyCtor_0091E100, {p, ci});
        body = p;
    }
    store<u32>(o + 296, body);
    if (named != 0) call(ctx, b27fn::HkBody_SetName, {body + 120, load<u32>(named + 120) & ~1u});
    if (release) {
        if (const u32 s = load<u32>(ci + 4); s != 0) call(ctx, b27fn::Hk_RemoveReference, {s});
    }
    if (named != 0) {
        for (u32 i = 0; i < 10; ++i) {
            const u32 key = load<u32>(kPropKeys + 4 * i);
            const s32 j = HkPropIndex(named, key);
            if (j < 0) continue;
            call(ctx, b27fn::Hk_AddProperty, {load<u32>(o + 296), key, HkPropValue(named, j)});
        }
        const u32 th = load<u32>(o + 284);
        if (th == kTypeHashA || th == kTypeHashB) call(ctx, b27fn::Hk_AddProperty, {load<u32>(o + 296), kPropFlag, 1});
    }

    // Motion and mass.
    const u32 wm = load<u32>(d + 164);
    const f64 mass = Lf(d + 164);
    SetF1Bits(ctx, wm);
    if (!(mass >= zero)) {
        call(ctx, b27fn::Hk_SetMotion, {load<u32>(o + 296), 5, 1, 0});
        store<u8>(load<u32>(o + 296) + 42, 0);
    } else {
        const u32 b = load<u32>(o + 296);
        const u8 kind = load<u8>(b + 232);
        if (kind != 5 && kind != 4) {
            if (mass != zero) {
                call(ctx, b27fn::Hk_SetMotion, {b, 1, 1, 0});
                store<u8>(load<u32>(o + 296) + 42, 4);
                callf(ctx, b27fn::Hk_SetMass, {load<u32>(o + 296)}, {mass});
            } else {
                call(ctx, b27fn::Hk_SetMotion, {b, 5, 1, 0});
                store<u8>(load<u32>(o + 296) + 42, 0);
            }
        }
    }
    body = load<u32>(o + 296);
    u32 q = 4;
    s32 j = -1;
    if (named != 0) j = HkPropIndex(named, kPropMotion);
    if (j >= 0) {
        q = static_cast<u32>(HkPropValue(named, j));  // low word of the u64 value
    } else if ((load<u32>(o + 300) & 0x80000000u) != 0) {
        q = 5;
    } else {
        const u8 kind = load<u8>(body + 232);
        q = kind == 5 ? 2 : kind == 4 ? 3 : 4;
    }
    store<u32>(body + 44, q);
    ret(ctx, body);
}
