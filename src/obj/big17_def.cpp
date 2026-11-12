// Game code, large-function sweep part 17: the damage def constructor that
// turns its record's body list into Havok rigid bodies (built by the inline
// damage def branch of XmlDef_NewFromNodeCA038).
#include "big18.h"

namespace sly4::obj {

namespace {
constexpr u32 Obj_FindName17 = 0x0034654C;            // (o, const char* s) -> entity or 0
constexpr u32 Kind_FromHash17 = 0x003557B8;           // (hash) -> collision layer 0..28
constexpr u32 PtrVector_Construct17 = 0x005516E0;     // (PtrVector*)
constexpr u32 HkBody_SetName17d = 0x007CE950;         // (body + 120, const char* name) (Havok)
constexpr u32 Hk_RemoveReference17 = 0x007D4EA8;      // (hkReferencedObject* or 0) (AnimSkel_Release7D4EA8)
constexpr u32 Hk_TranslateShapeCtor17 = 0x00898B30;   // (shape, 10, f1 child radius, child, 1) (Havok; r5 not read; name?)
constexpr u32 Hk_BoxShapeCtor17 = 0x00899720;         // (BoxShape*, const Vector4* halfExtents; f1 radius) (Havok)
constexpr u32 Hk_CinfoFromBody17 = 0x0091D208;        // (body, hkpRigidBodyCinfo*) (Havok)
constexpr u32 Hk_RigidBodyCtor17 = 0x0091E100;        // (body, const hkpRigidBodyCinfo*) (Havok)
constexpr u32 Hk_SetMotionFlags17d = 0x0091D470;      // (body, 1, 1, 0) for keyframed / fixed bodies (Havok; name?)
constexpr u32 Hk_SetMass17d = 0x0091D618;             // (body; f1 mass) (Havok; name?)
constexpr u32 Hk_RigidBodyCinfoCtor17 = 0x0091FB50;   // (hkpRigidBodyCinfo*) (Havok)
constexpr u32 Hk_RemoveEntity17 = 0x0092FFC8;         // (u8* out, world, body) (Havok)
constexpr u32 Hk_AddProperty17d = 0x0093FE00;         // (entity, key, u64 value) (Havok)

constexpr u32 kVtDamageDef = 0x00B12838;      // the 304-byte def's vtable
constexpr u32 kVtTranslateShape = 0x00F1E7F8; // vtable stored over the translate shape after its constructor
constexpr u32 kZero79 = 0x00C7980;            // .text f32 0 (the compare constant)
constexpr u32 kBoxRadius = 0x00F9203C;        // f32 0.05
constexpr u32 kCopyKeys = 0x00AC6838;         // u32[10] property keys copied from a named entity
constexpr u32 kKeyFilter17 = 0x81D42737;      // property: collision filter info
constexpr u32 kKeyFlag = 0x67E6342B;          // property added (value 1) for the two def kinds below
constexpr u32 kKindA = 0x29846DDA, kKindB = 0x4438A28F;
constexpr u32 kVtLeafDef = 0x00B12C88;        // the 416-byte leaf def's vtable
constexpr u32 kPool30 = 0x000C30A0;           // .text f32 0, lane masks at +16 / +32 / +48, f32 1 at +64
constexpr u32 kWorldPhys17 = 0x001281D4;      // World +0x1281D4: the physics holder (+4 hkpWorld*)

// hkpRigidBodyCinfo as a word view: +0 filter info, +4 shape, +144 mass,
// +148 / +152 / +156 / +172 / +176 tuning, u8 +180 motion, u8 +200
// quality, u8 +204 a flag.
struct RbCinfo17 {
    WordBytes w[52];            // +0 .. +204 (index = offset / 4)
};
static_assert(sizeof(RbCinfo17) == 208);

// stvx of four words to the quadword containing p.
inline void StQ(const void* p, u32 a, u32 b, u32 c, u32 d) {
    Quad& q = gm<Quad>(guest(p) & ~15u);
    q.w[0] = a;
    q.w[1] = b;
    q.w[2] = c;
    q.w[3] = d;
}
inline f64 LfAt(u32 ea) { return LfsW(gm<be_u32>(ea)); }

// A box shape (half extents x0 / x1 / x2, radius 0.05) wrapped in a
// translate shape by (0, x1, 0); the shape (0 when its allocation failed)
// goes to cinfo +4, the box is released. vec is the Vector4 scratch.
inline void BoxShape(RbCinfo17* cinfo, Quad* vec, u32 x0, u32 x1, u32 x2) {
    const u32 box = HkAlloc(48);
    if (box != 0) {
        // splat / vsel / vandc with the .text lane masks: (x0, x1, x2, 0).
        StQ(vec, x0, x1, x2, 0u);
        call(Hk_BoxShapeCtor17, box, vec, lf(global<be_f32>(kBoxRadius)));
    }
    const u32 s = HkAlloc(48);
    u32 shape = 0;
    if (s != 0) {
        StQ(vec, 0u, x1, 0u, 0u);  // (0, x1, 0, 0)
        call(Hk_TranslateShapeCtor17, s, 10u, LfAt(box + 16), box, 1u);
        gm<be_u32>(s) = kVtTranslateShape;
        StQ(host<u8>(s + 32), vec->w[0], vec->w[1], vec->w[2], 0u);
        gm<be_u32>(s + 28) = 0u;
        shape = s;
    }
    cinfo->w[1].w = shape;
    call(Hk_RemoveReference17, box);
}

// A new 544-byte rigid body from the cinfo (0 when the allocation failed).
inline u32 NewBody(RbCinfo17* cinfo) {
    const u32 nb = HkAlloc(544);
    if (nb == 0) return 0;
    call(Hk_RigidBodyCtor17, nb, cinfo);
    return nb;
}
}  // namespace

// void f(Def* p (304 bytes), const Record* rec) (no caller reads r3 or
// f1): the def base (0x000C2E8C), then the record's tuning words
// (+136..+216 -> p +204..+296, +140 -> p +208), the top 10 flag bits of
// +220 into p +300 (one store per bit), the 6 bytes at +128 -> p +196 and p
// +264 = rec +76 when below 0. Then for each 160-byte entry e of the
// PtrVector at rec +124: a rigid body cinfo on the stack, either from the
// physics world's entity named by e +32 (removed from the world first when
// it is in one) or from a new box shape (half extents e +0..+8, radius
// 0.05) wrapped in a translate shape by (0, e +4, 0) (a zero first 12
// bytes skips the entry); the filter info (the entity's 0x81D42737
// property, else Kind_FromHash(e +120), else from the flags / mass), the
// motion / quality types, a new 544-byte rigid body (named as the entity),
// and the 160-byte body record (from e) pushed onto p +192 (a new 20-byte
// PtrVector of 160-byte elements when unset). An entity's ten properties
// at 0x00AC6838 are copied to the new body, and kinds 0x29846DDA /
// 0x4438A28F (p +208) add property 0x67E6342B = 1.
void DamageDef_Ctor_000C79C0(DamageDef17* p, const DamageRec17* rec) {
    GuestFrame frame(624);
    Quad* vec = frame.as<Quad>(112);              // Vector4: box half extents / translation
    BodyRec160* br = frame.as<BodyRec160>(128);   // the body record pushed to p +192
    RbCinfo17* cinfo = frame.as<RbCinfo17>(288);  // hkpRigidBodyCinfo
    u8* out = frame.as<u8>(496);                  // Hk_RemoveEntity out byte
    DefBase_Ctor_000C2E8C(&p->base, &rec->base);

    auto pw = [&](u32 off) -> be_u32& { return p->w[(off - 204) / 4]; };
    auto rw = [&](u32 off) -> const be_u32& { return rec->w[(off - 136) / 4]; };
    pw(204) = rw(136);
    pw(224) = rw(144);
    pw(228) = rw(148);
    pw(232) = rw(152);
    for (u32 k = 0; k < 7; ++k) pw(240 + 4 * k) = rw(160 + 4 * k);  // +240..+264
    p->base.vt = kVtDamageDef;
    pw(268) = rw(188);
    p->vec = nullptr;
    pw(272) = rw(192);
    pw(208) = rw(140);
    for (u32 k = 0; k < 6; ++k) pw(276 + 4 * k) = rw(196 + 4 * k);  // +276..+296
    // The top 10 flag bits of rec +220, one bit (and one store) at a time.
    u32 w = (p->flags & 0x7FFFFFFFu) | (rec->flags & 0x80000000u);
    p->flags = w;
    for (u32 b = 1; b < 10; ++b) {
        const u32 m = 0x80000000u >> b;
        w = (w & ~m) | (rec->flags & m);
        p->flags = w;
    }
    const u32 w76 = rec->base.w76;
    for (u32 k = 0; k < 6; ++k) p->b196[k] = rec->b128[k];
    const f64 k0 = lf(global<be_f32>(kZero79));
    if (LfsW(pw(264)) < k0) pw(264) = w76;

    const u32 phys = gm<be_u32>(global<be_u32>(kWorld) + kWorldPhys17);
    for (u32 i = 0, off = 0;; ++i, off += 160) {
        BodyVec160* v = rec->list;
        const u32 n = v != nullptr ? v->count.get() : 0u;
        if (i >= n) return;
        const BodyRec160& e = gm<BodyRec160>(v->data.ea + off);
        call(Hk_RigidBodyCinfoCtor17, cinfo);
        u32 ent = 0;     // the named entity (r25)
        u32 isNew = 0;   // a new shape went into the cinfo (r26)
        const u32 name = e.w[8];
        if (name != 0 && gm<u8>(name) != 0) {
            if (phys == 0) continue;
            ent = call<u32>(Obj_FindName17, phys, name);
            if (ent == 0) continue;
            const u32 world = gm<HkBody18>(ent).world;
            if (world != 0) call(Hk_RemoveEntity17, out, world, ent);
            call(Hk_CinfoFromBody17, ent, cinfo);
        } else {
            // A first 12 bytes equal to the zero vector: no body.
            const Vec3& z = global<Vec3>(kZeroVec);
            if (e.w[0] == bits(z.x) && e.w[1] == bits(z.y) && e.w[2] == bits(z.z)) continue;
            BoxShape(cinfo, vec, e.w[0], e.w[1], e.w[2]);
            isNew = 1;
        }
        const u32 fl = p->flags;
        if ((fl & 0x08000000u) == 0 && (fl & 0x30000000u) != 0) cinfo->w[51].b[0] = 1;

        // The collision filter info.
        u32 filt;
        const s32 j = ent != 0 ? HkPropIdx(ent, kKeyFilter17) : -1;
        if (j >= 0) {
            filt = static_cast<u32>(HkPropVal64(ent, j));  // the low word of the u64 value
        } else if (e.w[30] != 0u) {
            filt = call<u32>(Kind_FromHash17, e.w[30].get());
        } else {
            const u32 f4 = e.w[31];
            if (f4 & 0x80000000u) {
                filt = 3;
            } else if (f4 & 0x20000000u) {
                filt = 5;
            } else if (!(LfsW(cinfo->w[36].w) > k0)) {
                filt = 2;
            } else {
                const f64 m = LfsW(e.w[28]);
                if (m > k0) filt = (f4 & 0x10000000u) ? 26 : 4;
                else if (m == k0) filt = 2;
                else if (m < k0) filt = 3;
                else filt = 4;  // NaN
            }
        }
        // Motion / quality types.
        u32 motion, quality;
        if (e.w[31] & 0x80000000u) {
            motion = 4;
            quality = (e.w[31] & 0x40000000u) ? 9 : 1;
        } else if (e.w[31] & 0x20000000u) {
            motion = 5, quality = 0;
        } else if (!(LfsW(cinfo->w[36].w) > k0)) {
            motion = 5, quality = 0;
        } else {
            // Tuning values at or above 0 override the cinfo's.
            static constexpr u32 kTune[5][2] = {{96, 172}, {100, 148}, {104, 176}, {108, 152}, {116, 156}};
            for (const auto& t : kTune)
                if (LfsW(e.w[t[0] / 4]) >= k0) cinfo->w[t[1] / 4].w = e.w[t[0] / 4];
            const f64 m = LfsW(e.w[28]);
            if (m > k0) {
                motion = 1, quality = 4;
                cinfo->w[36].w = e.w[28];
            } else if (m == k0) {
                motion = 5, quality = 0;
            } else if (m < k0) {
                motion = 4;
                quality = (e.w[31] & 0x40000000u) ? 9 : 1;
            } else {
                motion = 1, quality = 4;  // NaN
            }
        }
        cinfo->w[45].b[0] = static_cast<u8>(motion);
        cinfo->w[50].b[0] = static_cast<u8>(quality);

        // The body record's defaults (word k of the record at sp +128 + 4k).
        const Vec3& z = global<Vec3>(kZeroVec);
        const u32 z0 = bits(z.x), z1 = bits(z.y), z2 = bits(z.z);
        constexpr u32 one = 0x3F800000u, m1 = 0xBF800000u;
        cinfo->w[0].w = filt;
        for (u32 k : {0u, 4u, 12u, 32u}) {  // sp +128 / +144 / +176 / +256
            br->w[k] = z0;
            br->w[k + 1] = z1;
            br->w[k + 2] = z2;
        }
        br->w[8] = 0u;
        br->w[9] = 0u;
        static constexpr u32 kDef[] = {one, 0, m1, m1, m1, one, m1};  // +192..+216
        for (u32 k = 0; k < 7; ++k) br->w[16 + k] = kDef[k];
        for (u32 k = 0; k < 4; ++k) br->w[24 + k] = m1;
        br->w[28] = one;
        br->w[29] = one;
        br->w[30] = 0u;
        br->w[31] = br->w[31] & 0x0FFFFFFFu;
        br->w[36] = 0u;

        const u32 body = NewBody(cinfo);
        br->w[8] = body;
        if (ent != 0) call(HkBody_SetName17d, body + 120, gm<be_u32>(ent + 120) & ~1u);
        br->w[9] = e.w[9];
        for (u32 k = 0; k < 3; ++k) br->w[4 + k] = e.w[4 + k];
        for (u32 k = 0; k < 6; ++k) br->w[24 + k] = e.w[24 + k];
        br->w[30] = e.w[30];
        br->w[31] = (br->w[31] & 0x0FFFFFFFu) | (e.w[31] & 0xF0000000u);
        for (u32 k = 0; k < 3; ++k) br->w[12 + k] = e.w[12 + k];
        for (u32 k = 0; k < 7; ++k) br->w[16 + k] = e.w[16 + k];
        for (u32 k = 0; k < 3; ++k) br->w[32 + k] = e.w[32 + k];
        br->w[36] = e.w[36];

        BodyVec160* bv = p->vec;
        if (bv == nullptr) {
            bv = call<BodyVec160*>(fn::SmallAlloc, 20u);
            p->vec = bv;
            if (bv != nullptr) {
                call(PtrVector_Construct17, bv);
                bv->elem = 160;
                bv = p->vec;
            }
        }
        BodyVector_Add_00745720(&gm<BodyVec160>(guest(bv)), br);  // unchecked: a failed allocation passes 0
        if (isNew != 0 && cinfo->w[1].w != 0u) call(Hk_RemoveReference17, cinfo->w[1].w.get());
        if (ent != 0) {
            for (u32 k = 0; k < 10; ++k) {
                const u32 key = gm<be_u32>(kCopyKeys + 4 * k);
                const s32 jj = HkPropIdx(ent, key);
                if (jj >= 0) call(Hk_AddProperty17d, br->w[8].get(), key, HkPropVal64(ent, jj));
            }
        }
        const u32 kind = pw(208);
        if (kind == kKindA || kind == kKindB) call(Hk_AddProperty17d, br->w[8].get(), kKeyFlag, 1u);
    }
}

// void f(Def* p (416 bytes), const Record* rec) (no caller reads r3 or
// f1): the leaf def constructor. The def base (0x000C2E8C), the record's
// words +168..+336 into p +192..+368 (as the original orders the stores),
// the low 23 flag bits of +340 into p +372 one store per bit (p +400 = 0
// before the last), the vector +128 -> p +384. Then one rigid body: its
// cinfo from the physics world's entity named by rec +144 (removed from
// the world first when in one; no entity: return) or from a new box shape
// (half extents p +384..+392, radius 0.05) wrapped in a translate shape by
// (0, p +388, 0) (a zero vector at p +384: return); a new 544-byte body ->
// p +400 (named as the entity). Keyframed / fixed bodies (motion byte +232
// of 4 or 5) get 0x0091D470(body, 1, 1, 0) and 0x0091D618(body; f1 = rec
// +148 when above 0, else 1). Flag bit 0x800 of p +372: body +42 = 6. rec
// +152 / +156 at or above 0 -> body +140 / +144; the high halves of rec
// +160 / +164 (when those as f32 are at or above 0) -> body +404 / +406;
// body +44 = 6; a new shape is released from the cinfo. The word loads of
// p +384..+392 are lvlx (p word-aligned, as allocated).
void DefLeaf_Construct_000C30E8(LeafDef17* p, const LeafRec17* rec) {
    GuestFrame frame(416);
    Quad* vec = frame.as<Quad>(112);              // Vector4: box half extents / translation
    RbCinfo17* cinfo = frame.as<RbCinfo17>(128);  // hkpRigidBodyCinfo (+4 shape)
    u8* out = frame.as<u8>(336);                  // Hk_RemoveEntity out byte
    DefBase_Ctor_000C2E8C(&p->base, &rec->base);

    auto pw = [&](u32 off) -> be_u32& { return p->w[(off - 192) / 4].w; };
    auto pb = [&](u32 off) -> u8& { return p->w[(off - 192) / 4].b[off & 3]; };
    auto rw = [&](u32 off) -> const be_u32& { return rec->w[(off - 124) / 4]; };
    auto rb = [&](u32 off) -> u8 { return gm<u8>(guest(rec) + off); };
    for (u32 k = 0; k < 9; ++k) pw(192 + 4 * k) = rw(168 + 4 * k);  // +192..+224
    for (u32 k = 0; k < 3; ++k) pw(240 + 4 * k) = rw(208 + 4 * k);  // +240..+248
    for (u32 k = 0; k < 10; ++k) pw(256 + 4 * k) = rw(224 + 4 * k); // +256..+292
    p->base.vt = kVtLeafDef;
    for (u32 k = 0; k < 7; ++k) pw(296 + 4 * k) = rw(264 + 4 * k);  // +296..+320
    pw(336) = rw(304);
    pw(324) = rw(292);
    pw(340) = rw(308);
    pw(328) = rw(296);
    pw(344) = rw(312);
    pw(332) = rw(300);
    pw(348) = rw(316);
    pb(352) = rb(320);
    pw(356) = rw(324);
    pb(353) = rb(321);
    pw(360) = rw(328);
    pw(364) = rw(332);
    pw(368) = rw(336);
    // The top 23 flag bits of rec +340, one bit (and one store) at a time.
    u32 w = (pw(372) & 0x7FFFFFFFu) | (rw(340) & 0x80000000u);
    pw(372) = w;
    for (u32 b = 1; b < 23; ++b) {
        const u32 m = 0x80000000u >> b;
        w = (w & ~m) | (rw(340) & m);
        if (b == 22) p->body = 0u;
        pw(372) = w;
    }
    for (u32 k = 0; k < 3; ++k) pw(384 + 4 * k) = rw(128 + 4 * k);
    const u32 phys = gm<be_u32>(global<be_u32>(kWorld) + kWorldPhys17);
    call(Hk_RigidBodyCinfoCtor17, cinfo);

    u32 ent = 0;    // r28
    u32 isNew = 0;  // r29
    const u32 name = rw(144);
    if (name != 0 && gm<u8>(name) != 0) {
        ent = call<u32>(Obj_FindName17, phys, name);
        if (ent == 0) return;
        const u32 world = gm<HkBody18>(ent).world;
        if (world != 0) call(Hk_RemoveEntity17, out, world, ent);
        call(Hk_CinfoFromBody17, ent, cinfo);
    } else {
        const Vec3& z = global<Vec3>(kZeroVec);
        if (pw(384) == bits(z.x) && pw(388) == bits(z.y) && pw(392) == bits(z.z)) return;
        BoxShape(cinfo, vec, pw(384), pw(388), pw(392));
        isNew = 1;
    }

    p->body = NewBody(cinfo);
    if (ent != 0) call(HkBody_SetName17d, p->body + 120, gm<be_u32>(ent + 120) & ~1u);
    u32 body = p->body;
    const u32 motion = gm<HkBody18>(body).mtype;
    if (motion == 5 || motion == 4) {
        call(Hk_SetMotionFlags17d, body, 1u, 1u, 0u);
        const f64 m = LfsW(rw(148));
        const f64 k0 = lf(global<be_f32>(kPool30));
        const f64 f = m > k0 ? m : lf(global<be_f32>(kPool30 + 64));
        call(Hk_SetMass17d, p->body.get(), f);
        body = p->body;
    }
    if (pw(372) & 0x00000800u) {
        gm<HkBody18>(body).b42 = 6;
        body = p->body;
    }
    const f64 k0 = lf(global<be_f32>(kPool30));
    if (LfsW(rw(152)) >= k0) {
        gm<HkBody18>(body).w140 = rw(152);
        body = p->body;
    }
    if (LfsW(rw(156)) >= k0) {
        gm<HkBody18>(body).w144 = rw(156);
        body = p->body;
    }
    if (LfsW(rw(160)) >= k0) {
        gm<HkBody18>(body).h404 = static_cast<u16>(rw(160) >> 16);
        body = p->body;
    }
    if (LfsW(rw(164)) >= k0) {
        gm<HkBody18>(body).h406 = static_cast<u16>(rw(164) >> 16);
        body = p->body;
    }
    gm<HkBody18>(body).filter = 6u;
    if (isNew == 0) return;
    const u32 sh = cinfo->w[1].w;
    if (sh == 0) return;
    call(Hk_RemoveReference17, sh);
}

}  // namespace sly4::obj

NATIVE(000C79C0, sly4::obj::DamageDef_Ctor_000C79C0)
NATIVE(000C30E8, sly4::obj::DefLeaf_Construct_000C30E8)
