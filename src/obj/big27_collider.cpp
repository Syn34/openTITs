// Game code, large-function sweep part 27: an object's Havok hit colliders
// (512-byte bodies on a bone, up to four per object).
#include "../sly4.h"
#include "big27.h"

using namespace sly4;
using namespace sly4::big27;

namespace {
constexpr u32 kPool278 = 0x002787F0;     // .text pool: f32 0.0; masks {y} (+16), {z} (+32), {w} (+48)
constexpr u32 kColFlags = 0x0105B8C0;    // +72: bit 1 colliders on, bit 2 / 4 filter options
constexpr u32 kVtCollider = 0x00B1D370;  // the 512-byte collider's vtable
constexpr u32 kMask278 = 0x00277FD0;      // .text pool: masks {y}, {z} (+16), {w} (+32)
constexpr u32 kVtCastCol = 0x00B1D3C8;    // the stack hit collector's vtable during the cast
constexpr u32 kVtCastColEnd = 0x00B13C84; // ... and after it (its base class)
}  // namespace

// u32 f(Obj* o, const ShapeDesc* desc, const HitRec* rec, const Mat44* m,
// u32 bone, u32 a5, u32 a6) (called by Ev_ApplyHit_0004BCF0, 002794F0).
// Nothing unless kColFlags+72 bit 1 (r3 0), o has fewer than four
// colliders (o->+40 bits 7..10; r3 that count, f1 m->+52) and desc's sizes
// (+4/+8/+12) are not all 0 (r3 desc+4, f1 0.0). The frame m (rows 0..3,
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
// cleared. Returns 4 * that u16.
DECOMP(00278830, Obj_AddCollider_00278830) {
    const u32 o = arg<u32>(ctx, 0), desc = arg<u32>(ctx, 1), rec = arg<u32>(ctx, 2), m = arg<u32>(ctx, 3);
    const u32 bone = arg<u32>(ctx, 4), a5 = arg<u32>(ctx, 5), a6 = arg<u32>(ctx, 6);
    if ((load<u32>(kColFlags + 72) & 1) == 0) {
        ret(ctx, 0u);
        return;
    }
    Stack frame(ctx, 640);
    const u32 sp = frame.addr();
    const u32 M = sp + 128;   // the frame, rows of 16
    const u32 B = sp + 192;   // the bone matrix
    const u32 R = sp + 256;   // the frame's 3x3 (translation 0)
    const u32 T = sp + 320;   // B as four x/y/z rows for the collider
    static constexpr u32 kRow[12] = {0, 4, 8, 16, 20, 24, 32, 36, 40, 48, 52, 56};
    for (u32 k = 0; k < 12; ++k) store<u32>(M + kRow[k], load<u32>(m + kRow[k]));
    const u32 n0 = (load<u32>(o + 40) >> 7) & 0xF;
    if (static_cast<s32>(n0) >= 4) {
        SetF1(ctx, m + 52);
        ret(ctx, n0);
        return;
    }
    const f64 zero = Lf(kPool278);
    SetF1(ctx, kPool278);
    if (Lf(desc + 4) == zero && Lf(desc + 8) == zero && Lf(desc + 12) == zero) {
        ret(ctx, desc + 4);
        return;
    }
    const u32 w = load<u32>(o + 8);
    s32 idx = 0;
    bool viaRec = false;
    if (const u32 a = load<u32>(w + 160); a != 0) {
        if (const u32 t = load<u32>(load<u32>(a + 208) + 108); t != 0) {
            if (const u32 r = static_cast<u32>(call(ctx, b27fn::Rec80_Find, {t, bone})); r != 0) {
                const V4 r0 = Lvx(r), r1 = Lvx(r + 16), r2 = Lvx(r + 32), r3 = Lvx(r + 48);
                const V4 m0 = Lvx(M), m1 = Lvx(M + 16), m2 = Lvx(M + 32), m3 = Lvx(M + 48);
                idx = static_cast<s16>(load<u16>(r + 68));
                Stvx(M, Row3N26(r0, r1, r2, m0.e[0], m0.e[1], m0.e[2]));
                Stvx(M + 16, Row3N26(r0, r1, r2, m1.e[0], m1.e[1], m1.e[2]));
                Stvx(M + 32, Row3N26(r0, r1, r2, m2.e[0], m2.e[1], m2.e[2]));
                Stvx(M + 48, Add4N26(r3, Row3N26(r0, r1, r2, m3.e[0], m3.e[1], m3.e[2])));
                viaRec = true;
            }
        }
    }
    if (!viaRec) idx = static_cast<s32>(call(ctx, b27fn::Obj_IndexByHash, {w, bone}));
    call(ctx, b27fn::Obj_BoneMatrix, {B, w, static_cast<u32>(idx), 1, 1});

    // B's translation = the frame's origin through B; B *= the frame's 3x3.
    {
        const V4 b0 = Lvx(B), b1 = Lvx(B + 16), b2 = Lvx(B + 32), b3 = Lvx(B + 48);
        const V4 p = Add4N26(b3, Row3N26(b0, b1, b2, Fw27(M + 48), Fw27(M + 52), Fw27(M + 56)));
        for (u32 k = 0; k < 9; ++k) store<u32>(R + kRow[k], load<u32>(M + kRow[k]));
        store<u32>(R + 48, 0);
        store<u32>(R + 52, 0);
        store<u32>(R + 56, 0);
        Stvx(sp + 112, p);
        for (u32 k = 0; k < 3; ++k) store<u32>(B + 48 + 4 * k, load<u32>(sp + 112 + 4 * k));
        SetF1(ctx, sp + 120);  // leftover f1 (the last lfs)
        call(ctx, b27fn::Matrix_Multiply, {B, R});
    }

    const u32 world = load<u32>(load<u32>(load<u32>(b27fn::Game) + b27fn::GamePhys) + 4);
    u32 filt;
    if (VCallEO27(ctx, w, 64, {w}) == 0) {
        filt = (load<u64>(w + 184) & 1) != 0 ? 8 : 9;
    } else {
        filt = load<u32>(static_cast<u32>(VCall(ctx, w, 64, {w})) + 428);  // OPD / entry order this time
    }
    const bool bit = (load<u64>(w + 184) & 1) != 0;
    u32 p20 = load<u32>(rec + 20);
    u32 lo = filt & 0x1F;
    if (p20 == 0 || (load<u32>(p20 + 68) & 0x2000) == 0) lo = bit ? 10 : 11;
    if (const u32 q = load<u32>(w + 244); q != 0 && !bit) {
        lo = static_cast<u32>(VCall(ctx, q, 180, {q}));
        p20 = load<u32>(rec + 20);
    }
    filt = (filt & 0xFFFF0000u) | lo;
    u32 fA = 0, fB = 0, fK = 0;  // r20 / r19 / r18: the flag bits for u16 +398
    if (p20 != 0) {
        const u32 f = load<u32>(p20 + 68);
        fA = (f & 0x100) != 0;
        if ((f & 0x20) == 0) {
            if ((load<u64>(w + 184) & 1) != 0) {
                if ((load<u32>(kColFlags + 72) & 2) != 0) fB = 1;
            } else {
                if ((load<u32>(kColFlags + 72) & 4) != 0) fB = 1;
            }
        }
        fK = (f >> 26) & 0xF;
    }

    u32 c = 0;
    if (const u32 p = HkAlloc(ctx, 512); p != 0) {
        const u32 shape = static_cast<u32>(call(ctx, b27fn::HkShape_FromDesc, {desc}));
        const Masks3 mk = LoadMasks3(kPool278 + 16);
        for (u32 k = 0; k < 4; ++k) {
            const u32 s = B + 16 * k;
            Stvx(T + 16 * k, Andc(Sel(Sel(SplatW(s), SplatW(s + 4), mk.m[0]), SplatW(s + 8), mk.m[1]), mk.m[2]));
        }
        call(ctx, b27fn::Hk_ColliderCtor_009265D4, {p, shape, T, filt});
        store<u32>(p + 352, o);
        store<u32>(p + 0, kVtCollider);
        for (u32 k = 0; k < 16; k += 4) store<u32>(p + 356 + k, load<u32>(desc + k));
        u32 h = load<u16>(p + 398);
        store<u32>(p + 372, 0);
        h = (h & ~0x8000u) | ((fA & 1) << 15);
        store<u32>(p + 376, 0);
        h = (h & ~0x4000u) | ((fB & 1) << 14);
        store<u32>(p + 380, 0x80000000u);
        h &= ~0x3E00u;
        store<u32>(p + 384, 0);
        store<u32>(p + 388, 0);
        h = (h & ~0x1E00u) | ((fK << 9) & 0x1E00u);
        store<u32>(p + 392, 0);
        store<u16>(p + 396, 0xFFFF);
        store<u32>(p + 400, 0);
        store<u16>(p + 398, static_cast<u16>(h));
        for (u32 k = 0; k < 10; ++k) store<u32>(p + 416 + kRow[k], load<u32>(b27fn::IdentityMat + kRow[k]));
        const u32 sh = load<u32>(p + 16);
        store<u32>(p + 468, load<u32>(b27fn::IdentityMat + 52));
        store<u32>(p + 472, load<u32>(b27fn::IdentityMat + 56));
        SetF1(ctx, b27fn::IdentityMat + 56);
        call(ctx, b27fn::Hk_RemoveReference, {sh});
        c = p;
    }
    // Written even when the allocation failed (c = 0; quirk).
    store<u32>(c + 384, a5);
    store<u32>(c + 388, a6);
    store<u32>(c + 392, bone);
    store<u16>(c + 396, static_cast<u16>(idx));
    for (u32 k = 0; k < 12; ++k) store<u32>(c + 416 + kRow[k], load<u32>(M + kRow[k]));
    store<u32>(c + 400, w);
    for (u32 k = 0; k < 28; k += 4) store<u32>(c + 480 + k, load<u32>(rec + k));
    {
        u32 v = load<u32>(o + 40);
        const u32 n = (v >> 7) & 0xF;
        v = (v & ~0x780u) | (((n + 1) << 7) & 0x780u);
        store<u32>(o + 40, v);
        store<u32>(o + 144 + 4 * n, c);
    }
    call(ctx, b27fn::Body_Add, {world, c});
    u32 i = 0;
    for (; i < load<u16>(o + 168); ++i) store<u32>(load<u32>(o + 160) + 4 * i, 0);
    ret(ctx, 4 * i);
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
// Returns 1 when any cast hit, else 0.
DECOMP(00278000, Obj_UpdateColliders_00278000) {
    const u32 o = arg<u32>(ctx, 0), frm = arg<u32>(ctx, 2), desc = arg<u32>(ctx, 3);
    const f64 t = ctx.fpr[1];
    static constexpr u32 kRow[12] = {0, 4, 8, 16, 20, 24, 32, 36, 40, 48, 52, 56};
    auto count = [](u32 v) { return (v >> 7) & 0xF; };
    u32 v40 = load<u32>(o + 40);
    const u32 w = load<u32>(o + 8);
    u32 hit = 0;
    if (count(v40) == 0) {
        ret(ctx, 0u);
        return;
    }
    Stack frame(ctx, 1248);
    const u32 sp = frame.addr();
    const u32 B = sp + 192, ROT = sp + 256, F3 = sp + 320, NT = sp + 384, COL = sp + 448;
    for (u32 i = 0;;) {
        const u32 slot = o + 144 + 4 * i;
        const u32 c = load<u32>(slot);
        // 1. Shapes from desc.
        if (desc != 0) {
            for (u32 j = 0; j < count(v40); ++j) {
                const u32 cj = load<u32>(o + 144 + 4 * j);
                if (load<u32>(desc) == load<u32>(cj + 356) && Lf(desc + 4) == Lf(cj + 360) && Lf(desc + 8) == Lf(cj + 364) &&
                    Lf(desc + 12) == Lf(cj + 368))
                    continue;
                const u32 shape = static_cast<u32>(call(ctx, b27fn::HkShape_FromDesc, {desc}));
                VCall(ctx, cj, 16, {cj, shape});
                call(ctx, b27fn::Hk_RemoveReference, {load<u32>(cj + 16)});
                for (u32 k = 0; k < 16; k += 4) store<u32>(cj + 356 + k, load<u32>(desc + k));
                SetF1(ctx, desc + 12);
                v40 = load<u32>(o + 40);
            }
        }
        // 2. The frame.
        if (frm != 0) {
            for (u32 k = 0; k < 12; ++k) store<u32>(c + 416 + kRow[k], load<u32>(frm + kRow[k]));
            SetF1(ctx, frm + 56);
        }
        // 3. The new transform.
        call(ctx, b27fn::Obj_BoneMatrix, {B, load<u32>(c + 400), static_cast<u32>(static_cast<s32>(static_cast<s16>(load<u16>(c + 396)))), 1, 0});
        {
            const V4 b0 = Lvx(B), b1 = Lvx(B + 16), b2 = Lvx(B + 32), b3 = Lvx(B + 48);
            Stvx(sp + 112, Add4N26(b3, Row3N26(b0, b1, b2, Fw27(c + 464), Fw27(c + 468), Fw27(c + 472))));
        }
        for (u32 k = 0; k < 3; ++k) store<u32>(B + 48 + 4 * k, load<u32>(sp + 112 + 4 * k));
        SetF1(ctx, sp + 120);
        u32 r25 = ROT;  // what the original stores to the collector's +0 before the free
        call(ctx, b27fn::Matrix_SplitScale, {B, ROT, sp + 128});
        for (u32 k = 0; k < 9; ++k) store<u32>(F3 + kRow[k], load<u32>(c + 416 + kRow[k]));
        store<u32>(F3 + 48, 0);
        store<u32>(F3 + 52, 0);
        store<u32>(F3 + 56, 0);
        SetF1(ctx, c + 456);
        call(ctx, b27fn::Matrix_Multiply, {ROT, F3});
        store<u32>(sp + 176, 0x34000000u);
        store<u32>(sp + 180, 0x34000000u);
        const Masks3 mk = LoadMasks3(kMask278);
        for (u32 k = 0; k < 4; ++k) Stvx(NT + 16 * k, RowSelW(ROT + 16 * k, ROT + 16 * k + 4, ROT + 16 * k + 8, mk));

        // 4. The cast.
        u32 px, py, pz;  // the target point (f30 / f29 / f28)
        if (const u32 r = static_cast<u32>(VCall(ctx, w, 64, {w})); r == 0) {
            px = load<u32>(w + 144), py = load<u32>(w + 148), pz = load<u32>(w + 152);
        } else {
            px = load<u32>(r + 480), py = load<u32>(r + 484), pz = load<u32>(r + 488);
            if (load<u32>(r + 32) != 0) {
                r25 = r + 528;
                call(ctx, b27fn::Obj_RefPos, {sp + 144, r});
                const f64 s = Lf(sp + 148);
                SetF1(ctx, sp + 148);
                px = Fb(Fmadds(Lf(r + 528), s, LfsF(px)));
                py = Fb(Fmadds(Lf(r + 532), s, LfsF(py)));
                pz = Fb(Fmadds(Lf(r + 536), s, LfsF(pz)));
            }
        }
        const u32 base = (load<u64>(w + 184) & 1) != 0 ? 8 : 9;
        const u32 c2 = load<u32>(slot);
        const u32 w44 = load<u32>(c + 44);
        store<u32>(COL + 16, COL + 32);  // the inline array's data
        store<u32>(COL + 24, 0x80000008u);
        store<u32>(COL + 20, 0);
        store<u8>(COL + 416, 0);
        store<u32>(COL + 4, 0x7F7FFFEEu);
        store<u32>(COL + 0, kVtCastCol);
        store<u32>(COL + 420, c2);
        for (u32 k = 0; k < 4; ++k) Stvx(COL + 432 + 16 * k, Lvx(c2 + 160 + 16 * k));
        for (u32 k = 0; k < 4; ++k) Stvx(COL + 496 + 16 * k, Lvx(NT + 16 * k));
        store<u32>(COL + 576, (w44 & 0xFFFF0000u) | base);
        Sf(COL + 580, t);
        store<u32>(sp + 176, 0);
        store<u32>(sp + 180, 0);
        Stvx(sp + 160, Lvx(NT + 48));
        const auto fill = [](u32 x) { return Fill4(std::bit_cast<f32>(x)); };
        for (u32 k = 0; k < 4; ++k) store<u32>(sp + 1040 + 4 * k, pz);  // the last of the three splats it builds there
        Stvx(COL + 560, Andc(Sel(Sel(fill(px), fill(py), mk.m[0]), fill(pz), mk.m[1]), mk.m[2]));
        VCall(ctx, c, 64, {c, c2 + 208, sp + 160, COL, COL});
        call(ctx, b27fn::Hk_SetBodyTransform, {c, NT});
        const u32 size = load<u32>(COL + 20), cap = load<u32>(COL + 24), data = load<u32>(COL + 16);
        hit = (hit | (size != 0 ? 1u : 0u)) != 0 ? 1 : 0;
        store<u32>(COL + 20, 0);
        if ((cap & 0x80000000u) == 0) {
            store<u32>(COL + 0, r25);
            VCallEO27(ctx, b27fn::HkHeap, 20, {b27fn::HkHeap, data, (cap & 0x3FFFFFFFu) * 48u});
        }
        store<u32>(COL + 16, 0);
        store<u32>(COL + 24, 0x80000000u);
        store<u32>(COL + 0, kVtCastColEnd);
        ++i;
        v40 = load<u32>(o + 40);
        if (i >= count(v40)) break;
    }
    ret(ctx, hit);
}
