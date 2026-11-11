// Game code, large-function sweep part 17: the damage def constructor that
// turns its record's body list into Havok rigid bodies (built by the inline
// damage def branch of XmlDef_NewFromNodeCA038).
#include "big17.h"

using namespace sly4;
using namespace sly4::big17;

namespace {
namespace hc79 {
using big4::CopyW;

constexpr u32 kVtDamageDef = 0x00B12838;      // the 304-byte def's vtable
constexpr u32 kVtTranslateShape = 0x00F1E7F8; // vtable stored over the translate shape after its constructor
constexpr u32 kZero = 0x00C7980;              // .text f32 0 (the compare constant)
constexpr u32 kVecZero = 0x01164B40;          // f32[3] 0
constexpr u32 kBoxRadius = 0x00F9203C;        // f32 0.05
constexpr u32 kCopyKeys = 0x00AC6838;         // u32[10] property keys copied from a named entity
constexpr u32 kKeyFilter = 0x81D42737;        // property: collision filter info
constexpr u32 kKeyFlag = 0x67E6342B;          // property added (value 1) for the two def kinds below
constexpr u32 kKindA = 0x29846DDA, kKindB = 0x4438A28F;

// Frame layout (offsets from the 624-byte frame's base, as the original).
constexpr u32 kFrame = 624;
constexpr u32 kVec = 112;    // Vector4: box half extents / translation
constexpr u32 kRec = 128;    // the 160-byte body record pushed to p +192
constexpr u32 kCinfo = 288;  // hkpRigidBodyCinfo: +0 filter info, +4 shape, +144 mass,
                             // +148 / +152 / +172 / +176 / +156 tuning, +180 motion, +200 quality, +204 flag
constexpr u32 kOut = 496;    // Hk_RemoveEntity out byte

// The four words of a Vector4 at the (16-aligned) frame offset, as stvx.
inline void StW4(u32 ea, u32 a, u32 b, u32 c, u32 d) {
    ea &= ~15u;
    store<u32>(ea, a);
    store<u32>(ea + 4, b);
    store<u32>(ea + 8, c);
    store<u32>(ea + 12, d);
}
}  // namespace hc79
}  // namespace

// void f(Def* p (304 bytes), const Record* rec): the def base (0x000C2E8C),
// then the record's tuning words (+136..+216 -> p +204..+296, +140 -> p
// +208), the top 10 flag bits of +220 into p +300 (one store per bit), the 6
// bytes at +128 -> p +196 and p +264 = rec +76 when below 0. Then for each
// 160-byte entry e of the PtrVector at rec +124: a rigid body cinfo on the
// stack, either from the physics world's entity named by e +32 (removed from
// the world first when it is in one) or from a new box shape (half extents
// e +0..+8, radius 0.05) wrapped in a translate shape by (0, e +4, 0) (a
// zero first 12 bytes skips the entry); the filter info (the entity's
// 0x81D42737 property, else Kind_FromHash(e +120), else from the flags /
// mass), the motion / quality types, a new 544-byte rigid body (named as
// the entity), and the 160-byte body record (from e) pushed onto p +192 (a
// new 20-byte PtrVector of 160-byte elements when unset). An entity's ten
// properties at 0x00AC6838 are copied to the new body, and kinds 0x29846DDA /
// 0x4438A28F (p +208) add property 0x67E6342B = 1. Returns the entry count
// (r3; 0 without a vector). f1 is left as the original leaves it (the lfs of
// rec +76, or of the last entry's +136).
DECOMP(000C79C0, DamageDef_Ctor_000C79C0) {
    using namespace b17fn;
    using namespace hc79;
    const u32 p = arg<u32>(ctx, 0), rec = arg<u32>(ctx, 1);
    Stack frame(ctx, kFrame);
    const u32 sp = frame.addr();
    call(ctx, DefBase_Ctor_000C2E8C, {p, rec});

    CopyW(p + 204, rec + 136);
    CopyW(p + 224, rec + 144);
    CopyW(p + 228, rec + 148);
    CopyW(p + 232, rec + 152);
    for (u32 k = 0; k < 7; ++k) CopyW(p + 240 + 4 * k, rec + 160 + 4 * k);  // +240..+264
    store<u32>(p + 0, kVtDamageDef);
    CopyW(p + 268, rec + 188);
    store<u32>(p + 192, 0);
    CopyW(p + 272, rec + 192);
    CopyW(p + 208, rec + 140);
    for (u32 k = 0; k < 6; ++k) CopyW(p + 276 + 4 * k, rec + 196 + 4 * k);  // +276..+296
    // The top 10 flag bits of rec +220, one bit (and one store) at a time.
    u32 w = (load<u32>(p + 300) & 0x7FFFFFFFu) | (load<u32>(rec + 220) & 0x80000000u);
    store<u32>(p + 300, w);
    for (u32 b = 1; b < 10; ++b) {
        const u32 m = 0x80000000u >> b;
        w = (w & ~m) | (load<u32>(rec + 220) & m);
        store<u32>(p + 300, w);
    }
    const u32 w76 = load<u32>(rec + 76);
    ctx.fpr[1] = std::bit_cast<f64>(LfsBits(w76));
    for (u32 k = 0; k < 6; ++k) store<u8>(p + 196 + k, load<u8>(rec + 128 + k));
    const f64 k0 = LfsX(kZero);
    if (LfsX(p + 264) < k0) store<u32>(p + 264, w76);

    const u32 phys = load<u32>(load<u32>(kWorldPtr) + kWorldPhys);
    for (u32 i = 0, off = 0;; ++i, off += 160) {
        const u32 v = load<u32>(rec + 124);
        const u32 n = v != 0 ? load<u16>(v + 8) : 0;
        if (i >= n) {
            ret(ctx, n);
            return;
        }
        const u32 e = load<u32>(v) + off;
        call(ctx, Hk_RigidBodyCinfoCtor, {sp + kCinfo});
        u32 ent = 0;     // the named entity (r25)
        u32 isNew = 0;   // a new shape went into the cinfo (r26)
        const u32 name = load<u32>(e + 32);
        if (name != 0 && load<u8>(name) != 0) {
            if (phys == 0) continue;
            ent = static_cast<u32>(call(ctx, Obj_FindName, {phys, name}));
            if (ent == 0) continue;
            const u32 world = load<u32>(ent + 8);
            if (world != 0) call(ctx, Hk_RemoveEntity, {sp + kOut, world, ent});
            call(ctx, Hk_CinfoFromBody, {ent, sp + kCinfo});
        } else {
            // A first 12 bytes equal to the zero vector: no body.
            if (load<u64>(e) == load<u64>(kVecZero) && load<u32>(e + 8) == load<u32>(kVecZero + 8)) continue;
            const u32 e1 = load<u32>(e + 4);  // lane 0 of the (unaligned) vector at e +4, kept in v31
            u32 box = HkAlloc(ctx, 48);
            if (box != 0) {
                // splat / vsel / vandc with the .text lane masks at 0x000C7990: (e0, e1, e2, 0).
                StW4(sp + kVec, load<u32>(e), load<u32>(e + 4), load<u32>(e + 8), 0);
                callf(ctx, Hk_BoxShapeCtor, {box, sp + kVec}, {LfsX(kBoxRadius)});
            }
            const u32 s = HkAlloc(ctx, 48);
            u32 shape = 0;
            if (s != 0) {
                StW4(sp + kVec, 0, e1, 0, 0);  // (0, e1, 0, 0)
                callf(ctx, Hk_TranslateShapeCtor_00898B30, {s, 10, 48, box, 1}, {LfsX(box + 16)});
                store<u32>(s, kVtTranslateShape);
                StW4(s + 32, load<u32>(sp + kVec), load<u32>(sp + kVec + 4), load<u32>(sp + kVec + 8), 0);
                store<u32>(s + 28, 0);
                shape = s;
            }
            store<u32>(sp + kCinfo + 4, shape);
            call(ctx, Hk_RemoveReference, {box});
            isNew = 1;
        }
        const u32 fl = load<u32>(p + 300);
        if ((fl & 0x08000000u) == 0 && (fl & 0x30000000u) != 0) store<u8>(sp + kCinfo + 204, 1);

        // The collision filter info.
        u32 filt;
        const s32 j = ent != 0 ? HkPropIndex(ent, kKeyFilter) : -1;
        if (j >= 0) {
            filt = static_cast<u32>(HkPropValue(ent, j));  // the low word of the u64 value
        } else if (load<u32>(e + 120) != 0) {
            filt = static_cast<u32>(call(ctx, Kind_FromHash, {load<u32>(e + 120)}));
        } else {
            const u32 f4 = load<u32>(e + 124);
            if (f4 & 0x80000000u) {
                filt = 3;
            } else if (f4 & 0x20000000u) {
                filt = 5;
            } else if (!(LfsX(sp + kCinfo + 144) > k0)) {
                filt = 2;
            } else {
                const f64 m = LfsX(e + 112);
                if (m > k0) filt = (f4 & 0x10000000u) ? 26 : 4;
                else if (m == k0) filt = 2;
                else if (m < k0) filt = 3;
                else filt = 4;  // NaN
            }
        }
        // Motion / quality types.
        u32 motion, quality;
        if (load<u32>(e + 124) & 0x80000000u) {
            motion = 4;
            quality = (load<u32>(e + 124) & 0x40000000u) ? 9 : 1;
        } else if (load<u32>(e + 124) & 0x20000000u) {
            motion = 5, quality = 0;
        } else if (!(LfsX(sp + kCinfo + 144) > k0)) {
            motion = 5, quality = 0;
        } else {
            // Tuning values at or above 0 override the cinfo's.
            static constexpr u32 kTune[5][2] = {{96, 172}, {100, 148}, {104, 176}, {108, 152}, {116, 156}};
            for (const auto& t : kTune)
                if (LfsX(e + t[0]) >= k0) CopyW(sp + kCinfo + t[1], e + t[0]);
            const f64 m = LfsX(e + 112);
            if (m > k0) {
                motion = 1, quality = 4;
                CopyW(sp + kCinfo + 144, e + 112);
            } else if (m == k0) {
                motion = 5, quality = 0;
            } else if (m < k0) {
                motion = 4;
                quality = (load<u32>(e + 124) & 0x40000000u) ? 9 : 1;
            } else {
                motion = 1, quality = 4;  // NaN
            }
        }
        store<u8>(sp + kCinfo + 180, static_cast<u8>(motion));
        store<u8>(sp + kCinfo + 200, static_cast<u8>(quality));

        // The body record's defaults.
        const u32 z0 = load<u32>(kVecZero), z1 = load<u32>(kVecZero + 4), z2 = load<u32>(kVecZero + 8);
        constexpr u32 one = 0x3F800000u, m1 = 0xBF800000u;
        store<u32>(sp + kCinfo, filt);
        for (u32 o : {128u, 144u, 176u, 256u}) {
            store<u32>(sp + o, z0);
            store<u32>(sp + o + 4, z1);
            store<u32>(sp + o + 8, z2);
        }
        store<u32>(sp + 160, 0);
        store<u32>(sp + 164, 0);
        static constexpr u32 kDef[] = {one, 0, m1, m1, m1, one, m1};  // +192..+216
        for (u32 k = 0; k < 7; ++k) store<u32>(sp + 192 + 4 * k, kDef[k]);
        for (u32 k = 0; k < 4; ++k) store<u32>(sp + 224 + 4 * k, m1);
        store<u32>(sp + 240, one);
        store<u32>(sp + 244, one);
        store<u32>(sp + 248, 0);
        store<u32>(sp + 252, load<u32>(sp + 252) & 0x0FFFFFFFu);
        store<u32>(sp + 272, 0);

        const u32 nb = HkAlloc(ctx, 544);
        u32 body = 0;
        if (nb != 0) {
            call(ctx, Hk_RigidBodyCtor_0091E100, {nb, sp + kCinfo});
            body = nb;
        }
        store<u32>(sp + 160, body);
        if (ent != 0) call(ctx, HkBody_SetName, {body + 120, load<u32>(ent + 120) & ~1u});
        CopyW(sp + 164, e + 36);
        for (u32 k = 0; k < 3; ++k) CopyW(sp + 144 + 4 * k, e + 16 + 4 * k);
        for (u32 k = 0; k < 6; ++k) CopyW(sp + 224 + 4 * k, e + 96 + 4 * k);
        CopyW(sp + 248, e + 120);
        store<u32>(sp + 252, (load<u32>(sp + 252) & 0x0FFFFFFFu) | (load<u32>(e + 124) & 0xF0000000u));
        for (u32 k = 0; k < 3; ++k) CopyW(sp + 176 + 4 * k, e + 48 + 4 * k);
        for (u32 k = 0; k < 7; ++k) CopyW(sp + 192 + 4 * k, e + 64 + 4 * k);
        for (u32 k = 0; k < 3; ++k) CopyW(sp + 256 + 4 * k, e + 128 + 4 * k);
        SetF1(ctx, e + 136);  // the last lfs into f1
        CopyW(sp + 272, e + 144);

        u32 vec = load<u32>(p + 192);
        if (vec == 0) {
            vec = static_cast<u32>(call(ctx, SmallAlloc, {20}));
            store<u32>(p + 192, vec);
            if (vec != 0) {
                call(ctx, PtrVector_Construct, {vec});
                store<u16>(vec + 6, 160);
                vec = load<u32>(p + 192);
            }
        }
        call(ctx, BodyVector_Add_00745720, {vec, sp + kRec});
        if (isNew != 0 && load<u32>(sp + kCinfo + 4) != 0) call(ctx, Hk_RemoveReference, {load<u32>(sp + kCinfo + 4)});
        if (ent != 0) {
            for (u32 k = 0; k < 10; ++k) {
                const u32 key = load<u32>(kCopyKeys + 4 * k);
                const s32 jj = HkPropIndex(ent, key);
                if (jj >= 0) call(ctx, Hk_AddProperty, {load<u32>(sp + 160), key, HkPropValue(ent, jj)});
            }
        }
        const u32 kind = load<u32>(p + 208);
        if (kind == kKindA || kind == kKindB) call(ctx, Hk_AddProperty, {load<u32>(sp + 160), kKeyFlag, 1});
    }
}

namespace {
namespace hc30 {
constexpr u32 kVtLeafDef = 0x00B12C88;        // the 416-byte leaf def's vtable
constexpr u32 kPool = 0x000C30A0;             // .text f32 0, lane masks at +16 / +32 / +48, f32 1 at +64

// Frame layout (offsets from the 416-byte frame's base, as the original).
constexpr u32 kFrame = 416;
constexpr u32 kVec = 112;    // Vector4: box half extents / translation
constexpr u32 kCinfo = 128;  // hkpRigidBodyCinfo (+4 shape)
constexpr u32 kOut = 336;    // Hk_RemoveEntity out byte
}  // namespace hc30
}  // namespace

// u32 f(Def* p (416 bytes), const Record* rec): the leaf def constructor.
// The def base (0x000C2E8C), the record's words +168..+336 into p
// +192..+368 (as the original orders the stores), the low 23 flag bits of
// +340 into p +372 one store per bit (p +400 = 0 before the last), the
// vector +128 -> p +384. Then one rigid body: its cinfo from the physics
// world's entity named by rec +144 (removed from the world first when in
// one; no entity: return 0) or from a new box shape (half extents p
// +384..+392, radius 0.05) wrapped in a translate shape by (0, p +388, 0)
// (a zero vector at p +384: return 0); a new 544-byte body -> p +400 (named
// as the entity). Keyframed / fixed bodies (motion byte +232 of 4 or 5) get
// 0x0091D470(body, 1, 1, 0) and 0x0091D618(body; f1 = rec +148 when above
// 0, else 1). Flag bit 0x800 of p +372: body +42 = 6. rec +152 / +156 at or
// above 0 -> body +140 / +144; the high halves of rec +160 / +164 (when
// those as f32 are at or above 0) -> body +404 / +406; body +44 = 6; a new
// shape is released from the cinfo. r3: 0 for the early returns, 6 for an
// entity's body, else the shape release's result (or 0 without a shape).
// The word loads of p +384..+392 are lvlx (p word-aligned, as allocated).
DECOMP(000C30E8, DefLeaf_Construct_000C30E8) {
    using namespace b17fn;
    using namespace hc30;
    using big4::CopyW;
    const u32 p = arg<u32>(ctx, 0), rec = arg<u32>(ctx, 1);
    Stack frame(ctx, kFrame);
    const u32 sp = frame.addr();
    call(ctx, DefBase_Ctor_000C2E8C, {p, rec});

    for (u32 k = 0; k < 9; ++k) CopyW(p + 192 + 4 * k, rec + 168 + 4 * k);  // +192..+224
    for (u32 k = 0; k < 3; ++k) CopyW(p + 240 + 4 * k, rec + 208 + 4 * k);  // +240..+248
    for (u32 k = 0; k < 10; ++k) CopyW(p + 256 + 4 * k, rec + 224 + 4 * k); // +256..+292
    store<u32>(p + 0, kVtLeafDef);
    for (u32 k = 0; k < 7; ++k) CopyW(p + 296 + 4 * k, rec + 264 + 4 * k);  // +296..+320
    CopyW(p + 336, rec + 304);
    CopyW(p + 324, rec + 292);
    CopyW(p + 340, rec + 308);
    CopyW(p + 328, rec + 296);
    CopyW(p + 344, rec + 312);
    CopyW(p + 332, rec + 300);
    CopyW(p + 348, rec + 316);
    store<u8>(p + 352, load<u8>(rec + 320));
    CopyW(p + 356, rec + 324);
    store<u8>(p + 353, load<u8>(rec + 321));
    CopyW(p + 360, rec + 328);
    CopyW(p + 364, rec + 332);
    CopyW(p + 368, rec + 336);
    // The top 23 flag bits of rec +340, one bit (and one store) at a time.
    u32 w = (load<u32>(p + 372) & 0x7FFFFFFFu) | (load<u32>(rec + 340) & 0x80000000u);
    store<u32>(p + 372, w);
    for (u32 b = 1; b < 23; ++b) {
        const u32 m = 0x80000000u >> b;
        w = (w & ~m) | (load<u32>(rec + 340) & m);
        if (b == 22) store<u32>(p + 400, 0);
        store<u32>(p + 372, w);
    }
    for (u32 k = 0; k < 3; ++k) CopyW(p + 384 + 4 * k, rec + 128 + 4 * k);
    SetF1(ctx, rec + 136);  // f1 as the original leaves it
    const u32 phys = load<u32>(load<u32>(kWorldPtr) + kWorldPhys);
    call(ctx, Hk_RigidBodyCinfoCtor, {sp + kCinfo});

    u32 ent = 0;    // r28
    u32 isNew = 0;  // r29
    const u32 name = load<u32>(rec + 144);
    if (name != 0 && load<u8>(name) != 0) {
        ent = static_cast<u32>(call(ctx, Obj_FindName, {phys, name}));
        if (ent == 0) {
            ret(ctx, 0);
            return;
        }
        const u32 world = load<u32>(ent + 8);
        if (world != 0) call(ctx, Hk_RemoveEntity, {sp + kOut, world, ent});
        call(ctx, Hk_CinfoFromBody, {ent, sp + kCinfo});
    } else {
        if (load<u64>(p + 384) == load<u64>(hc79::kVecZero) && load<u32>(p + 392) == load<u32>(hc79::kVecZero + 8)) {
            ret(ctx, 0);
            return;
        }
        const u32 e1 = load<u32>(p + 388);  // lane 0 of the lvlx at p +388, kept in v31
        u32 box = HkAlloc(ctx, 48);
        if (box != 0) {
            // splat / vsel / vandc with the .text lane masks: (p384, p388, p392, 0).
            hc79::StW4(sp + kVec, load<u32>(p + 384), load<u32>(p + 388), load<u32>(p + 392), 0);
            callf(ctx, Hk_BoxShapeCtor, {box, sp + kVec}, {LfsX(hc79::kBoxRadius)});
        }
        const u32 s = HkAlloc(ctx, 48);
        u32 shape = 0;
        if (s != 0) {
            hc79::StW4(sp + kVec, 0, e1, 0, 0);
            callf(ctx, Hk_TranslateShapeCtor_00898B30, {s, 10, 32, box, 1}, {LfsX(box + 16)});
            store<u32>(s, hc79::kVtTranslateShape);
            hc79::StW4(s + 32, load<u32>(sp + kVec), load<u32>(sp + kVec + 4), load<u32>(sp + kVec + 8), 0);
            store<u32>(s + 28, 0);
            shape = s;
        }
        store<u32>(sp + kCinfo + 4, shape);
        call(ctx, Hk_RemoveReference, {box});
        isNew = 1;
    }

    const u32 nb = HkAlloc(ctx, 544);
    u32 body = 0;
    if (nb != 0) {
        call(ctx, Hk_RigidBodyCtor_0091E100, {nb, sp + kCinfo});
        body = nb;
    }
    store<u32>(p + 400, body);
    if (ent != 0) call(ctx, HkBody_SetName, {body + 120, load<u32>(ent + 120) & ~1u});
    body = load<u32>(p + 400);
    const u32 motion = load<u8>(body + 232);
    if (motion == 5 || motion == 4) {
        call(ctx, Hk_SetMotionFlags_0091D470, {body, 1, 1, 0});
        const f64 m = LfsX(rec + 148);
        const f64 k0 = LfsX(kPool);
        const f64 f = m > k0 ? m : LfsX(kPool + 64);
        callf(ctx, Hk_SetMass_0091D618, {load<u32>(p + 400)}, {f});
        body = load<u32>(p + 400);
    }
    if (load<u32>(p + 372) & 0x00000800u) {
        store<u8>(body + 42, 6);
        body = load<u32>(p + 400);
    }
    const f64 k0 = LfsX(kPool);
    if (LfsX(rec + 152) >= k0) {
        CopyW(body + 140, rec + 152);
        body = load<u32>(p + 400);
    }
    if (LfsX(rec + 156) >= k0) {
        CopyW(body + 144, rec + 156);
        body = load<u32>(p + 400);
    }
    if (LfsX(rec + 160) >= k0) {
        store<u16>(body + 404, static_cast<u16>(load<u32>(rec + 160) >> 16));
        body = load<u32>(p + 400);
    }
    SetF1(ctx, rec + 164);
    if (LfsX(rec + 164) >= k0) {
        store<u16>(body + 406, static_cast<u16>(load<u32>(rec + 164) >> 16));
        body = load<u32>(p + 400);
    }
    store<u32>(body + 44, 6);
    if (isNew == 0) {
        ret(ctx, 6);
        return;
    }
    const u32 sh = load<u32>(sp + kCinfo + 4);
    if (sh == 0) {
        ret(ctx, 0);
        return;
    }
    ret(ctx, call(ctx, Hk_RemoveReference, {sh}));
}
