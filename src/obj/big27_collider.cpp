// Game code, large-function sweep part 27: an object's Havok hit colliders
// (512-byte bodies on a bone, up to four per object).
#include "big27.h"
#include "native/vec.h"

namespace sly4::obj {

namespace {
constexpr u32 kPool278 = 0x002787F0;     // .text pool: f32 0.0; masks {y} (+16), {z} (+32), {w} (+48)
constexpr u32 kColFlags = 0x0105B8C0;    // +72: bit 1 colliders on, bit 2 / 4 filter options
constexpr u32 kVtCollider = 0x00B1D370;  // the 512-byte collider's vtable
constexpr u32 kMask278 = 0x00277FD0;      // .text pool: masks {y}, {z} (+16), {w} (+32)
constexpr u32 kVtCastCol = 0x00B1D3C8;    // the stack hit collector's vtable during the cast
constexpr u32 kVtCastColEnd = 0x00B13C84; // ... and after it (its base class)
constexpr u32 IdentityMat = 0x01164BD0;
constexpr u32 Obj_IndexByHash = 0x000D14DC;         // (owner, u32 hash) -> bone index
constexpr u32 Obj_BoneMatrix = 0x000CE688;          // (f32 out[4][4], owner, s32 bone, u8 world, u8 raw)
constexpr u32 Rec80_Find = 0x005848CC;              // (table, key) -> 80-byte record or 0
constexpr u32 Matrix_Multiply = 0x005541F0;         // (Mat34* io, const Mat34*)
constexpr u32 HkShape_FromDesc = 0x00277C90;        // (const ShapeDesc*) -> hkpShape* or 0
constexpr u32 Hk_ColliderCtor_009265D4 = 0x009265D4;  // (body, shape, const hkTransform*, u32 filter) (name?)
constexpr u32 Body_Add = 0x00931D10;                // (world, body)
constexpr u32 Obj_RefPos = 0x0026A42C;              // (f32 out[3], Obj*)
constexpr u32 Matrix_SplitScale = 0x00554858;       // (const f32 m[16], f32 rot[16], f32 scale[3])
constexpr u32 Hk_SetBodyTransform = 0x00929E98;     // (hkpRigidBody*, const f32 rows[4][4])
constexpr u32 Hk_RemoveReference = 0x007D4EA8;      // (hkReferencedObject*)
constexpr u32 HkHeap = 0x011DF31C;                  // Havok heap allocator object (vfn[20] (p, bytes) frees)

// The shape description: a type word and three sizes.
struct ShapeDesc {
    be_u32 type;      // +0
    be_f32 size[3];   // +4
};
// The hit record copied to the collider (+20: its definition, +68 flags).
struct HitRec {
    be_u32 w[7];  // +0 .. +24 (w[5] = +20)
};
// The 512-byte collider (an hkpRigidBody underneath).
struct Collider {
    be_u32 vtable;       // +0
    pad<12> _4;
    be_u32 shape;        // +16
    pad<24> _20;
    be_u32 filter;       // +44
    pad<112> _48;
    Mtx44 xf;            // +160: the current transform
    pad<128> _224;
    be_u32 owner;        // +352: the object
    ShapeDesc desc;      // +356
    be_u32 w372;         // +372
    be_u32 w376;         // +376
    be_u32 w380;         // +380
    be_u32 a5;           // +384
    be_u32 a6;           // +388
    be_u32 bone;         // +392
    be_u16 idx;          // +396
    be_u16 flags;        // +398
    be_u32 w;            // +400: the owner's object
    pad<12> _404;
    Mtx44 frame;         // +416
    HitRec rec;          // +480
    pad<4> _508;
};
static_assert(sizeof(Collider) == 512 && offsetof(Collider, owner) == 352 && offsetof(Collider, idx) == 396);
static_assert(offsetof(Collider, frame) == 416 && offsetof(Collider, rec) == 480);
// The collider owner object: +144 its position, +160 a skeleton holder,
// +184 a u64 whose bit 0 picks the filter, +244 a filter source.
struct ColOwner {
    pad<144> _0;
    Vec3 pos;        // +144
    pad<4> _156;
    be_u32 a160;     // +160: its +208 -> +108 the bone record table
    pad<20> _164;
    be_u32 f184;     // +184
    be_u32 f188;     // +188: bit 0
    pad<52> _192;
    be_u32 q244;     // +244
};
static_assert(offsetof(ColOwner, f188) == 188 && offsetof(ColOwner, q244) == 244);
struct ColObj {
    pad<8> _0;
    be_u32 w;            // +8: ColOwner*
    pad<28> _12;
    be_u32 flags;        // +40: bits 10..7 the collider count
    pad<100> _44;
    be_u32 cols[4];      // +144: Collider*
    be_u32 list;         // +160: u32[]
    pad<4> _164;
    be_u16 nlist;        // +168
};
static_assert(offsetof(ColObj, cols) == 144 && offsetof(ColObj, nlist) == 168);
// The cast hit collector on the stack.
struct CastCol {
    be_u32 vtable;     // +0
    be_u32 early_out;  // +4 (raw)
    pad<8> _8;
    be_u32 data;       // +16
    be_u32 size;       // +20
    be_u32 capflags;   // +24
    pad<4> _28;
    pad<384> inl;      // +32: eight 48-byte hits
    u8 b416;           // +416
    pad<3> _417;
    be_u32 body;       // +420
    pad<8> _424;
    Mtx44 cur;         // +432
    Mtx44 next;        // +496
    Vec4 target;       // +560
    be_u32 filter;     // +576
    be_f32 t;          // +580
};
static_assert(offsetof(CastCol, cur) == 432 && offsetof(CastCol, target) == 560 && offsetof(CastCol, t) == 580);

bool OwnerBit(u32 w) { return (gm<ColOwner>(w).f188 & 1) != 0; }  // bit 0 of the u64 at +184
}  // namespace

// void f(Obj* o, const ShapeDesc* desc, const HitRec* rec, const Mat44* m,
// u32 bone, u32 a5, u32 a6) (called by Ev_ApplyHit_0004BCF0, 002794F0).
// Nothing unless kColFlags+72 bit 1, o has fewer than four
// colliders (o->+40 bits 7..10) and desc's sizes (+4/+8/+12) are not all
// 0. The frame m (rows 0..3,
// x/y/z) is copied to the stack; when o's owner (o->+8 = w) has a +160
// whose +208->+108 table holds a record for bone (Rec80_Find), the frame
// is taken through that record's matrix and the bone index is its s16
// +68, else Obj_IndexByHash(w, bone). With w's bone matrix B
// (Obj_BoneMatrix(.., 1, 1)): B's translation becomes the frame's origin
// in B and B *= the frame's 3x3 (Matrix_Multiply). The collision filter
// is w's vfn[64]()->+428 (8 / 9 without that object, by w->+184 bit 0)
// with its low half replaced by: 10 / 11 (by that bit) unless rec->+20 has
// flag 0x2000 (then the filter's low 5 bits), or w->+244's vfn[180]()
// when w->+244 is set and the bit clear. A 512-byte collider (Havok
// allocator) on HkShape_FromDesc(desc) and B (009265D4), vtable
// 0x00B1D370, +352 o, +356 desc[0..3], u16 +398 flags from rec->+20->+68
// (0x8000: bit 0x100; 0x4000: bit 0x20 clear and kColFlags bit 4 (bit 2
// when w->+184 bit 0); 0x1E00: bits 26..29), identity +416.. (the shape
// reference released), then (even without the allocation, quirk: the
// writes go to 0 + offset) +384 a5, +388 a6, +392 bone, +396 index, the
// frame at +416.., +400 w, rec[0..27] at +480. It is o->+144[count]
// (count + 1) and added to the world; o->+160[0 .. u16 +168) are
// cleared. No caller reads r3 or f1 (r3.sh).
void Obj_AddCollider_00278830(ColObj* o, const ShapeDesc* desc, const HitRec* rec, const Mtx44* m, u32 bone, u32 a5, u32 a6) {
    if ((global<be_u32>(kColFlags + 72) & 1) == 0) return;
    GuestFrame frame(640);
    Vec4* tmp = frame.as<Vec4>(112);
    Mtx44* M = frame.as<Mtx44>(128);  // the frame
    Mtx44* B = frame.as<Mtx44>(192);  // the bone matrix
    Mtx44* R = frame.as<Mtx44>(256);  // the frame's 3x3 (translation 0)
    Mtx44* T = frame.as<Mtx44>(320);  // B as four x/y/z rows for the collider
    CopyMtxXYZ(*M, *m);
    const u32 n0 = (o->flags >> 7) & 0xF;
    if (static_cast<s32>(n0) >= 4) return;
    const f64 zero = lf(global<be_f32>(kPool278));
    if (lf(desc->size[0]) == zero && lf(desc->size[1]) == zero && lf(desc->size[2]) == zero) return;
    const u32 w = o->w;
    s32 idx = 0;
    bool viaRec = false;
    if (const u32 a = gm<ColOwner>(w).a160; a != 0) {
        if (const u32 t = gm<be_u32>(gm<be_u32>(a + 208) + 108); t != 0) {
            if (const u32 r = call<u32>(Rec80_Find, t, bone); r != 0) {
                const Mtx44& rm = gm<Mtx44>(r);
                const V4 r0 = Lvx(&rm.r[0]), r1 = Lvx(&rm.r[1]), r2 = Lvx(&rm.r[2]), r3 = Lvx(&rm.r[3]);
                const V4 m0 = Lvx(&M->r[0]), m1 = Lvx(&M->r[1]), m2 = Lvx(&M->r[2]), m3 = Lvx(&M->r[3]);
                idx = gm<be_s16>(r + 68);
                Stvx(&M->r[0], Row3N(r0, r1, r2, m0.e[0], m0.e[1], m0.e[2]));
                Stvx(&M->r[1], Row3N(r0, r1, r2, m1.e[0], m1.e[1], m1.e[2]));
                Stvx(&M->r[2], Row3N(r0, r1, r2, m2.e[0], m2.e[1], m2.e[2]));
                Stvx(&M->r[3], Add4N(r3, Row3N(r0, r1, r2, m3.e[0], m3.e[1], m3.e[2])));
                viaRec = true;
            }
        }
    }
    if (!viaRec) idx = call<s32>(Obj_IndexByHash, w, bone);
    call(Obj_BoneMatrix, B, w, static_cast<u32>(idx), 1u, 1u);

    // B's translation = the frame's origin through B; B *= the frame's 3x3.
    {
        const V4 b0 = Lvx(&B->r[0]), b1 = Lvx(&B->r[1]), b2 = Lvx(&B->r[2]), b3 = Lvx(&B->r[3]);
        auto Fw = [](const be_f32& v) { return std::bit_cast<f32>(bits(v)); };
        const V4 p = Add4N(b3, Row3N(b0, b1, b2, Fw(M->r[3].x), Fw(M->r[3].y), Fw(M->r[3].z)));
        for (u32 k = 0; k < 3; ++k) CopyXYZ(R->r[k], M->r[k]);
        R->r[3].x = 0.0f;
        R->r[3].y = 0.0f;
        R->r[3].z = 0.0f;
        Stvx(tmp, p);
        CopyXYZ(B->r[3], *tmp);
        call(Matrix_Multiply, B, R);
    }

    const u32 world = gm<be_u32>(gm<be_u32>(global<be_u32>(kWorld) + 0x001281D4) + 4);
    u32 filt;
    if (vcall<u32>(host<u8>(w), 64) == 0) filt = OwnerBit(w) ? 8u : 9u;
    else filt = gm<be_u32>(vcall<u32>(host<u8>(w), 64) + 428);
    const bool bit = OwnerBit(w);
    u32 p20 = rec->w[5];
    u32 lo = filt & 0x1F;
    if (p20 == 0 || (gm<be_u32>(p20 + 68) & 0x2000) == 0) lo = bit ? 10 : 11;
    if (const u32 q = gm<ColOwner>(w).q244; q != 0 && !bit) {
        lo = vcall<u32>(host<u8>(q), 180);
        p20 = rec->w[5];
    }
    filt = (filt & 0xFFFF0000u) | lo;
    u32 fA = 0, fB = 0, fK = 0;  // r20 / r19 / r18: the flag bits for u16 +398
    if (p20 != 0) {
        const u32 f = gm<be_u32>(p20 + 68);
        fA = (f & 0x100) != 0;
        if ((f & 0x20) == 0) {
            if (OwnerBit(w)) {
                if ((global<be_u32>(kColFlags + 72) & 2) != 0) fB = 1;
            } else {
                if ((global<be_u32>(kColFlags + 72) & 4) != 0) fB = 1;
            }
        }
        fK = (f >> 26) & 0xF;
    }

    u32 c = 0;
    if (const u32 pa = HkAlloc(512); pa != 0) {
        Collider& p = gm<Collider>(pa);
        const u32 shape = call<u32>(HkShape_FromDesc, desc);
        const Masks3 mk = LoadMasks3(kPool278 + 16);
        for (u32 k = 0; k < 4; ++k) Stvx(&T->r[k], RowSelW(&B->r[k].x, &B->r[k].y, &B->r[k].z, mk));
        call(Hk_ColliderCtor_009265D4, pa, shape, T, filt);
        p.owner = guest(o);
        p.vtable = kVtCollider;
        p.desc.type = desc->type.get();
        for (u32 k = 0; k < 3; ++k) set_bits(p.desc.size[k], bits(desc->size[k]));
        u32 h = p.flags;
        h = (h & ~0x8000u) | ((fA & 1) << 15);
        h = (h & ~0x4000u) | ((fB & 1) << 14);
        h &= ~0x3E00u;
        h = (h & ~0x1E00u) | ((fK << 9) & 0x1E00u);
        p.w372 = 0u;
        p.w376 = 0u;
        p.w380 = 0x80000000u;
        p.a5 = 0u;
        p.a6 = 0u;
        p.bone = 0u;
        p.idx = 0xFFFF;
        p.w = 0u;
        p.flags = static_cast<u16>(h);
        CopyMtxXYZ(p.frame, global<Mtx44>(IdentityMat));
        call(Hk_RemoveReference, p.shape.get());
        c = pa;
    }
    // Written even when the allocation failed (c = 0; quirk).
    Collider& cc = gm<Collider>(c);
    cc.a5 = a5;
    cc.a6 = a6;
    cc.bone = bone;
    cc.idx = static_cast<u16>(idx);
    CopyMtxXYZ(cc.frame, *M);
    cc.w = w;
    for (u32 k = 0; k < 7; ++k) cc.rec.w[k] = rec->w[k].get();
    {
        u32 v = o->flags;
        const u32 n = (v >> 7) & 0xF;
        v = (v & ~0x780u) | (((n + 1) << 7) & 0x780u);
        o->flags = v;
        o->cols[n] = c;
    }
    call(Body_Add, world, c);
    for (u32 i = 0; i < o->nlist; ++i) gm<be_u32>(o->list + 4 * i) = 0u;
}

// u32 f(Obj* o, -, const Mat44* frm, const ShapeDesc* desc; f32 t) (called
// by 0004BE50, 002794F0). For each of o's colliders c (o->+144[i], i <
// o->+40 bits 7..10, the count re-read every step):
// 1. With desc: every collider whose desc copy (+356: type word, sizes
//    +360..+368, compared as floats) differs gets HkShape_FromDesc(desc)
//    through its vfn[16] (the old shape +16 released) and the copy (the
//    whole pass repeats for every i, quirk of the original).
// 2. With frm: frm's x/y/z rows to c+416.. (the frame).
// 3. The bone matrix of c->+400 / s16 +396 (Obj_BoneMatrix(.., 1, 0)) with
//    the frame origin (+464..) through it as translation, split into
//    rotation and scale (Matrix_SplitScale) and times the frame's 3x3; its
//    rows go to the stack (the new transform).
// 4. A hit collector on the stack (vtable 0x00B1D3C8, +4 0x7F7FFFEE, an
//    inline array {+16 data, +20 size, +24 0x80000008}, +420 c, +432 c's
//    current transform (+160..), +496 the new one, +560 the target point:
//    o's owner w->vfn[64]() +480.. (+ its +528.. scaled by Obj_RefPos +4
//    when its +32 is set), else w+144..; +576 the filter (c->+44 high half
//    | 8 / 9 by w->+184 bit 0), +580 t) is passed to c->vfn[64](c, c+208,
//    &row3, col, col); then Hk_SetBodyTransform(c, new transform). Any hit
//    (size != 0) sets the result; a heap array (flag 0x80000000 clear) is
//    freed through HkHeap->vfn[20](data, capacity * 48).
// Returns 1 when any cast hit, else 0 (f1 is not read).
u32 Obj_UpdateColliders_00278000(ColObj* o, u32, const Mtx44* frm, const ShapeDesc* desc, f32 t) {
    auto count = [](u32 v) { return (v >> 7) & 0xF; };
    u32 v40 = o->flags;
    const u32 w = o->w;
    u32 hit = 0;
    if (count(v40) == 0) return 0;
    GuestFrame frame(1248);
    Vec4* tmp = frame.as<Vec4>(112);
    Vec4* scale = frame.as<Vec4>(128);
    Vec4* refpos = frame.as<Vec4>(144);
    Vec4* row3 = frame.as<Vec4>(160);
    Vec4* zero = frame.as<Vec4>(176);
    Mtx44* B = frame.as<Mtx44>(192);
    Mtx44* ROT = frame.as<Mtx44>(256);
    Mtx44* F3 = frame.as<Mtx44>(320);
    Mtx44* NT = frame.as<Mtx44>(384);
    CastCol* col = frame.as<CastCol>(448);
    for (u32 i = 0;;) {
        Collider& c = gm<Collider>(o->cols[i]);
        // 1. Shapes from desc.
        if (desc != nullptr) {
            for (u32 j = 0; j < count(v40); ++j) {
                Collider& cj = gm<Collider>(o->cols[j]);
                if (desc->type == cj.desc.type && lf(desc->size[0]) == lf(cj.desc.size[0]) && lf(desc->size[1]) == lf(cj.desc.size[1]) &&
                    lf(desc->size[2]) == lf(cj.desc.size[2]))
                    continue;
                const u32 shape = call<u32>(HkShape_FromDesc, desc);
                vcall(&cj, 16, shape);
                call(Hk_RemoveReference, cj.shape.get());
                cj.desc.type = desc->type.get();
                for (u32 k = 0; k < 3; ++k) set_bits(cj.desc.size[k], bits(desc->size[k]));
                v40 = o->flags;
            }
        }
        // 2. The frame.
        if (frm != nullptr) CopyMtxXYZ(c.frame, *frm);
        // 3. The new transform.
        call(Obj_BoneMatrix, B, c.w.get(), static_cast<u32>(static_cast<s32>(static_cast<s16>(c.idx.get()))), 1u, 0u);
        {
            auto Fw = [](const be_f32& v) { return std::bit_cast<f32>(bits(v)); };
            const V4 b0 = Lvx(&B->r[0]), b1 = Lvx(&B->r[1]), b2 = Lvx(&B->r[2]), b3 = Lvx(&B->r[3]);
            Stvx(tmp, Add4N(b3, Row3N(b0, b1, b2, Fw(c.frame.r[3].x), Fw(c.frame.r[3].y), Fw(c.frame.r[3].z))));
        }
        CopyXYZ(B->r[3], *tmp);
        u32 r25 = guest(ROT);  // what the original stores to the collector's +0 before the free
        call(Matrix_SplitScale, B, ROT, scale);
        for (u32 k = 0; k < 3; ++k) CopyXYZ(F3->r[k], c.frame.r[k]);
        F3->r[3].x = 0.0f;
        F3->r[3].y = 0.0f;
        F3->r[3].z = 0.0f;
        call(Matrix_Multiply, ROT, F3);
        const Masks3 mk = LoadMasks3(kMask278);
        for (u32 k = 0; k < 4; ++k) Stvx(&NT->r[k], RowSelW(&ROT->r[k].x, &ROT->r[k].y, &ROT->r[k].z, mk));

        // 4. The cast.
        u32 px, py, pz;  // the target point (f30 / f29 / f28), raw
        if (const u32 r = vcall<u32>(host<u8>(w), 64); r == 0) {
            const Vec3& wp = gm<ColOwner>(w).pos;
            px = bits(wp.x), py = bits(wp.y), pz = bits(wp.z);
        } else {
            const Vec3& rp = gm<Vec3>(r + 480);
            px = bits(rp.x), py = bits(rp.y), pz = bits(rp.z);
            if (gm<be_u32>(r + 32) != 0u) {
                r25 = r + 528;
                call(Obj_RefPos, refpos, r);
                const f64 sc = lf(refpos->y);
                const Vec3& d = gm<Vec3>(r + 528);
                px = std::bit_cast<u32>(static_cast<f32>(Fmadds(lf(d.x), sc, LfsW(px))));
                py = std::bit_cast<u32>(static_cast<f32>(Fmadds(lf(d.y), sc, LfsW(py))));
                pz = std::bit_cast<u32>(static_cast<f32>(Fmadds(lf(d.z), sc, LfsW(pz))));
            }
        }
        const u32 base = OwnerBit(w) ? 8u : 9u;
        const u32 c2 = o->cols[i];
        const u32 w44 = c.filter;
        col->data = guest(&col->inl);
        col->capflags = 0x80000008u;
        col->size = 0u;
        col->b416 = 0;
        col->early_out = 0x7F7FFFEEu;
        col->vtable = kVtCastCol;
        col->body = c2;
        for (u32 k = 0; k < 4; ++k) Stvx(&col->cur.r[k], Lvx(&gm<Collider>(c2).xf.r[k]));
        for (u32 k = 0; k < 4; ++k) Stvx(&col->next.r[k], Lvx(&NT->r[k]));
        col->filter = (w44 & 0xFFFF0000u) | base;
        col->t = t;
        Stvx(zero, Zero4());
        Stvx(row3, Lvx(&NT->r[3]));
        auto fill = [](u32 x) { return Fill4(std::bit_cast<f32>(x)); };
        Stvx(&col->target, RowSel(fill(px), fill(py), fill(pz), mk));
        vcall(&c, 64, &gm<Collider>(c2).xf.r[3], row3, col, col);  // (c, c2 + 208, &row 3, col, col)
        call(Hk_SetBodyTransform, &c, NT);
        const u32 size = col->size, cap = col->capflags, data = col->data;
        hit = (hit | (size != 0 ? 1u : 0u)) != 0 ? 1 : 0;
        col->size = 0u;
        if ((cap & 0x80000000u) == 0) {
            col->vtable = r25;
            vcall(host<u8>(HkHeap), 20, data, (cap & 0x3FFFFFFFu) * 48u);
        }
        col->data = 0u;
        col->capflags = 0x80000000u;
        col->vtable = kVtCastColEnd;
        ++i;
        v40 = o->flags;
        if (i >= count(v40)) break;
    }
    return hit;
}

}  // namespace sly4::obj

NATIVE(00278830, sly4::obj::Obj_AddCollider_00278830)
NATIVE(00278000, sly4::obj::Obj_UpdateColliders_00278000)
