// Game code, part 44: the small-function sweep continued from the remaining
// 93 instruction functions (size order; obj43*.cpp did 91-93). Each was
// checked against the original in the PPC simulator (memory writes, calls
// and their arguments, r3 and f1).
#include "obj44.h"

using namespace sly4;
using namespace sly4::obj44;

// ---------------------------------------------------------------- sweep: 93 instructions
// void f(T* o, u32 k1, u32 k2, f32 a, f32 b, f32 c, f32 d, f32 e, (r10) u32
// k3, stack u8 m): Obj_InitVt_B20CC8(o, m, k2, 332), vtable 0x00B20F98,
// +40/+48/+56/+64 = a/b/c/d with zeroed words between, +80..+92 = the
// vector 0x01164C90, +120 = e; then +12 / +32 / +112 = *PointMap_Find(
// (*0x00F8F154)->+16, &k1 / &k2 / &k3). r3 = the last of those; f1 = the
// w of 0x01164C90.
DECOMP(00430A64, Obj_InitPoints_00430A64) {
    const u32 o = arg<u32>(ctx, 0), k1 = arg<u32>(ctx, 1), k2 = arg<u32>(ctx, 2), k3 = arg<u32>(ctx, 7);
    const f64 a = farg(ctx, 0), b = farg(ctx, 1), c = farg(ctx, 2), d = farg(ctx, 3), e = farg(ctx, 4);
    // the 10th argument: the low byte of the word at the caller's sp + 124
    const u32 m = load<u32>(static_cast<u32>(ctx.gpr[1]) + 124) & 0xFFu;
    Stack frame(ctx, 16);
    const u32 s = frame.addr();
    call(ctx, 0x0042C9D0u, {o, m, k2, 332});
    store<u32>(o + 40, W44(a));
    store<u32>(o + 36, 0);
    store<u32>(o + 44, 0);
    store<u32>(o + 48, W44(b));
    store<u32>(o + 52, 0);
    store<u32>(o + 56, W44(c));
    store<u32>(o + 60, 0);
    store<u32>(o, 0x00B20F98u);
    store<u32>(o + 64, W44(d));
    store<u32>(o + 12, 0);
    store<u32>(o + 32, 0);
    Cp44(o + 80, 0x01164C90u);
    store<u32>(o + 112, 0);
    Cp44(o + 84, 0x01164C94u);
    Cp44(o + 88, 0x01164C98u);
    store<u32>(o + 116, 0);
    Cp44(o + 92, 0x01164C9Cu);
    store<u32>(o + 120, W44(e));
    store<u32>(s + 8, k1);
    u32 v = load<u32>(static_cast<u32>(call(ctx, 0x0073343Cu, {load<u32>(load<u32>(0x00F8F154u) + 16), s + 8})));
    store<u32>(s + 4, k2);
    store<u32>(o + 12, v);
    v = load<u32>(static_cast<u32>(call(ctx, 0x0073343Cu, {load<u32>(load<u32>(0x00F8F154u) + 16), s + 4})));
    store<u32>(s, k3);
    store<u32>(o + 32, v);
    v = load<u32>(static_cast<u32>(call(ctx, 0x0073343Cu, {load<u32>(load<u32>(0x00F8F154u) + 16), s})));
    store<u32>(o + 112, v);
    ctx.fpr[1] = Lfs44(0x01164C9Cu);
    ret(ctx, v);
}

// T* f(T* o) (deleting destructor): vtable 0x00B22358; while the tree of
// +24 has a root (*(o->+36); the u16 +120 = 0 each round, +124 = 0 at the
// end): walk down the +12 links from it, pushing every node that has one
// onto the u32 stack at +40 (u16 count +120), +124 = the last node; when
// TreeFind_0073AF5C(o + 24, key (+4), &out, o->+24) finds it,
// Avl_Remove(o + 24, o + 24, key, &changed) and --o->+28. Then vtable
// 0x00B11B20 and Mem_FreeFrom(o, 0). r3 = 0 for null, else Mem_FreeFrom's.
DECOMP(004C7350, Tree_DeleteAll_004C7350) {
    const u32 o = arg<u32>(ctx, 0);
    if (o == 0) {
        ret(ctx, 0);
        return;
    }
    Stack frame(ctx, 16);
    const u32 s = frame.addr();
    store<u32>(o, 0x00B22358u);
    for (;;) {
        const u32 root = load<u32>(o + 36);
        store<u16>(o + 120, 0);
        u32 n = load<u32>(root);
        if (n == 0) {
            store<u32>(o + 124, 0);
            break;
        }
        u32 leaf = n;
        for (;;) {
            leaf = n;
            const u32 nx = load<u32>(leaf + 12);
            if (nx != 0) {
                const s32 i = static_cast<s16>(load<u16>(o + 120));
                store<u16>(o + 120, static_cast<u16>(i + 1));
                store<u32>(o + 40 + 4 * static_cast<u32>(i), n);
            }
            n = nx;
            if (n == 0) break;
        }
        const u32 key = load<u32>(leaf + 4);
        store<u32>(o + 124, leaf);
        store<u8>(s + 4, 0);
        if (call(ctx, 0x0073AF5Cu, {o + 24, key, s, load<u32>(o + 24)}) == 0) continue;
        call(ctx, 0x0073A36Cu, {o + 24, o + 24, key, s + 4});
        store<u32>(o + 28, load<u32>(o + 28) - 1);
    }
    store<u32>(o, 0x00B11B20u);
    ret(ctx, call(ctx, 0x0054EE6Cu, {o, 0}));
}

// T* f(?, XmlCursor* cur): a new 28-byte record (Mem_Alloc(28, 0, 16);
// vtable 0x00B22268, u16 +4 = 1, the rest zeroed) read from cur: hash
// 0x8A93973F into +12 (when absent, the s32 0x5C8998CD), hash 0x00014B07
// into +20, f32 0x002644E0 into +16 (default *0x004CC7A8), bools
// 0xF5897C08 / 0x6334AE6F into +24 / +25. r3 = the record (0 when the
// allocation fails).
DECOMP(004CC7B0, XmlRec_New_B22268_004CC7B0) {
    const u32 cur = arg<u32>(ctx, 1);
    const u32 p = static_cast<u32>(call(ctx, 0x0054ED38u, {28, 0, 16}));
    if (p == 0) {
        ret(ctx, 0);
        return;
    }
    store<u16>(p + 4, 1);
    store<u32>(p, 0x00B22268u);
    store<u32>(p + 12, 0);
    const f64 k = Lfs44(0x004CC7A8u);
    store<u32>(p + 16, 0);
    store<u32>(p + 20, 0);
    store<u8>(p + 24, 0);
    store<u8>(p + 25, 0);
    if (call(ctx, 0x004FA14Cu, {cur, 0x8A93973Fu, p + 12, 0}) != 0) call(ctx, 0x004F99BCu, {cur, 0x5C8998CDu, p + 12, 0});
    call(ctx, 0x004FA14Cu, {cur, 0x00014B07u, p + 20, 0});
    callf(ctx, 0x004F9B20u, {cur, 0x002644E0u, p + 16}, {k});
    call(ctx, 0x004F986Cu, {cur, 0xF5897C08u, p + 24, 0});
    call(ctx, 0x004F986Cu, {cur, 0x6334AE6Fu, p + 25, 0});
    ret(ctx, p);
}

// ---------------------------------------------------------------- sweep: 94 instructions
// u32 f(T* o, Ctx* x): resolves the target object t: from the component
// Effect_FindComponent(x, o->+12) when its vt[32] kind is 5 and its +28
// object is of class 0x02235C33 (t = +280), else Target_Resolve_0009A190(
// o, o->+16, x) when +16 is set; then Obj_FindByHash_000DB290(t, o->+20)
// when +20 is set. With t and bit 1 of t->+192: bit 28 of t->+376 =
// bit 0 of the u8 o->+24 (r3 = that byte); without t / the bit:
// Hash_ToString of +16 (and of +12 when there is no t).
DECOMP(0008CAE8, Target_ApplyFlag_0008CAE8) {
    const u32 o = arg<u32>(ctx, 0), x = arg<u32>(ctx, 1);
    u32 t = 0;
    bool viaComp = false;
    u32 c = load<u32>(o + 12);
    if (c != 0) {
        c = static_cast<u32>(call(ctx, 0x004BB4B4u, {x, c}));
        if (c != 0 && static_cast<s32>(VCall(ctx, c, 32, {c})) == 5) {
            const u32 q = load<u32>(c + 28);
            if (q != 0) {
                if (load<u32>(q + 276) == 0x02235C33u) t = load<u32>(q + 280);
                viaComp = t != 0;
            }
        }
    }
    if (!viaComp) {
        const u32 h = load<u32>(o + 16);
        if (h != 0) t = static_cast<u32>(call(ctx, 0x0009A190u, {o, h, x}));
    }
    if (t != 0) {
        const u32 h = load<u32>(o + 20);
        if (h != 0) t = static_cast<u32>(call(ctx, 0x000DB290u, {t, h}));
    }
    if (t == 0) {
        call(ctx, 0x0056A6E0u, {load<u32>(o + 16)});
        call(ctx, 0x0056A6E0u, {load<u32>(o + 12)});
        return;
    }
    if (load<u32>(t + 192) & 0x2u) {
        const u32 b = load<u8>(o + 24);
        store<u32>(t + 376, (load<u32>(t + 376) & ~0x10000000u) | ((b & 1u) << 28));
        ret(ctx, b);
        return;
    }
    call(ctx, 0x0056A6E0u, {load<u32>(o + 16)});
}

// u32 f(T* o): o->vt[32](o, &rec) with a 100-byte stack record: the 4x4
// rows 0..3 (x, y, z of each) of the matrix at o->+4->+96 over the
// defaults of 0x01164BD0, +64 / +68 = o->+4->+356 / +360 (defaults
// 0x425C0000 / 0x40F00000), zeroed words / bytes and the 4 bytes of
// 0x01164CA8 at +96. r3 = the call's; f1 = o->+4->+360.
DECOMP(001082F0, Obj_SendXform_001082F0) {
    const u32 o = arg<u32>(ctx, 0);
    Stack frame(ctx, 112);
    const u32 s = frame.addr();
    const u32 K = 0x01164BD0u;
    for (u32 r = 0; r < 4; ++r)
        for (u32 j = 0; j < 3; ++j) Cp44(s + 16 * r + 4 * j, K + 16 * r + 4 * j);
    store<u32>(s + 64, 0x425C0000u);
    store<u32>(s + 68, 0x40F00000u);
    store<u8>(s + 72, 0);
    store<u32>(s + 76, 0);
    store<u32>(s + 80, 0);
    store<u32>(s + 84, 0);
    store<u8>(s + 88, 0);
    const u8 b0 = load<u8>(0x01164CA8u);
    store<u32>(s + 92, 0);
    const u8 b1 = load<u8>(0x01164CA9u);
    store<u8>(s + 96, b0);
    const u8 b2 = load<u8>(0x01164CAAu);
    store<u8>(s + 97, b1);
    const u8 b3 = load<u8>(0x01164CABu);
    store<u8>(s + 98, b2);
    store<u8>(s + 99, b3);
    const u32 g = load<u32>(o + 4);
    const u32 mt = g + 96;
    for (u32 r = 0; r < 4; ++r)
        for (u32 j = 0; j < 3; ++j) Cp44(s + 16 * r + 4 * j, mt + 16 * r + 4 * j);
    Cp44(s + 64, g + 356);
    Cp44(s + 68, g + 360);
    ctx.fpr[1] = Lfs44(g + 360);
    ret(ctx, VCall(ctx, o, 32, {o, s}));
}

// u32 f(T* o, Hit* h): when Obj_TestE0BCC(o, h): the strength s = h->+60 *
// t * o->+508 (t = (h+48 - h+96) / (h+48 - h+52) when h+96 > h+52 and
// h+48 > h+52, else *0x001271CC); a = h->+4 when bit 3 of its +192 is set;
// s = *0x001271D0 when h->+4 has bit 5 of +192, (u16 +432 >> 8) & 3 == 3
// and +428 == o. With a of o's team (bits 22..23 of +172) where bit 5 of
// o->+188 and bit 27 of a->+188 are clear, s *= the tuning var
// 0x01038988. Then 0x00125A80(o, a, h, 0x03C599D8; s) and bit 8 of the
// u16 h->+14 is set (r3 = that u16). r3 = 0 when the test fails.
DECOMP(001271D8, Obj_HitScaled_001271D8) {
    const u32 o = arg<u32>(ctx, 0), h = arg<u32>(ctx, 1);
    if (call(ctx, 0x000E0BCCu, {o, h}) == 0) {
        ret(ctx, 0);
        return;
    }
    const f64 p96 = Lfs44(h + 96), p52 = Lfs44(h + 52);
    f64 t = Lfs44(0x001271CCu);
    if (p96 > p52) {
        const f64 p48 = Lfs44(h + 48);
        if (p48 > p52) t = F44(F44(p48 - p96) / F44(p48 - p52));
    }
    f64 s = F44(Lfs44(h + 60) * t);
    const f64 m = Lfs44(o + 508);
    const u32 q = load<u32>(h + 4);
    u32 a = 0;
    s = F44(s * m);
    if (q != 0) {
        if (load<u32>(q + 192) & 0x8u) a = q;
        if ((load<u32>(q + 192) & 0x20u) && ((load<u16>(q + 432) >> 8) & 3u) == 3u && load<u32>(q + 428) == o)
            s = Lfs44(0x001271D0u);
    }
    if (a != 0 && ((load<u32>(o + 172) >> 22) & 3u) == ((load<u32>(a + 172) >> 22) & 3u) &&
        (static_cast<u32>(load<u64>(o + 184)) & 0x20u) == 0 &&
        (static_cast<u32>(load<u64>(a + 184)) & 0x08000000u) == 0)
        s = F44(s * Lfs44(0x01038988u));
    callf(ctx, 0x00125A80u, {o, a, h, 0x03C599D8u}, {s});
    const u32 f = static_cast<u32>(load<u16>(h + 14)) | 0x100u;
    store<u16>(h + 14, static_cast<u16>(f));
    ret(ctx, f);
}

// T* f(a, b, name, tmpl): a 336-byte spawn definition (SmallAlloc)
// initialised by SpawnQuery_Init(p, name, tmpl, 0, 0x01164BD0, 0x01164B30,
// 1, 0, stack 0 / - / 0; f1 = *0x0017F198), vtable 0x00B14B80, +240 /
// +256 = the vector 0x01164B40, +272 = 1.0, +280 / +284 = 0.5, the rest up
// to +324 zeroed and flag 0x100 in +4; then XmlDef_Read_0017EF48(a, b, p).
// r3 = p (0 for a failed allocation, still passed).
DECOMP(0017F1A0, XmlDef_New_B14B80_0017F1A0) {
    const u32 a = arg<u32>(ctx, 0), b = arg<u32>(ctx, 1), name = arg<u32>(ctx, 2), tmpl = arg<u32>(ctx, 3);
    const u32 p = static_cast<u32>(call(ctx, 0x0055238Cu, {336}));
    if (p != 0) {
        // stack argument 10 is not written by the original (left as is in its frame)
        callf(ctx, 0x000CC4D4u, {p, name, tmpl, 0, 0x01164BD0u, 0x01164B30u, 1, 0, 0, 0, 0}, {Lfs44(0x0017F198u)});
        store<u32>(p, 0x00B14B80u);
        Cp44(p + 240, 0x01164B40u);
        Cp44(p + 244, 0x01164B44u);
        Cp44(p + 248, 0x01164B48u);
        Cp44(p + 256, 0x01164B40u);
        Cp44(p + 260, 0x01164B44u);
        const u32 z = load<u32>(0x01164B48u);
        store<u32>(p + 276, 0);
        store<u32>(p + 264, z);
        store<u32>(p + 272, 0x3F800000u);
        store<u32>(p + 280, 0x3F000000u);
        store<u32>(p + 288, 0);
        store<u32>(p + 284, 0x3F000000u);
        store<u32>(p + 292, 0);
        store<u32>(p + 296, 0);
        const u32 f = load<u32>(p + 4);
        store<u32>(p + 300, 0);
        store<u8>(p + 304, 0);
        store<u32>(p + 308, 0);
        store<u32>(p + 312, 0);
        store<u8>(p + 316, 0);
        store<u32>(p + 320, 0);
        store<u32>(p + 324, 0);
        store<u32>(p + 4, f | 0x100u);
        ctx.fpr[1] = static_cast<f64>(std::bit_cast<f32>(z));
    }
    call(ctx, 0x0017EF48u, {a, b, p});
    ret(ctx, p);
}

// u32 f(?, T* a, Basis* out, T* b): an orthonormal basis: Z = out->+32 =
// normalize(a->+32.x, b->+36, a->+40) when its squared length > *0x001D09AC
// (unit length *0x001D09B0), else b->+32..+40; Y = out->+16 = b->+16..+24;
// X = out->+0 = normalize(Y x Z); out->+48 = b->+48..+56 (the position).
// r3 = b + 48; f1 = b->+56.
DECOMP(001D09B8, Basis_FromDir_001D09B8) {
    const u32 a = arg<u32>(ctx, 1), out = arg<u32>(ctx, 2), b = arg<u32>(ctx, 3);
    f64 zx, zy, zz;
    {
        const f64 y = Lfs44(b + 36), x = Lfs44(a + 32);
        f64 l = F44(y * y);
        const f64 z = Lfs44(a + 40);
        const f64 eps = Lfs44(0x001D09ACu);
        l = Fma44(x, x, l);
        l = Fma44(z, z, l);
        if (l > eps) {
            l = F44(std::sqrt(l));
            const f64 k = F44(Lfs44(0x001D09B0u) / l);
            zx = F44(x * k);
            zy = F44(y * k);
            zz = F44(z * k);
        } else {
            zx = Lfs44(b + 32);
            zy = Lfs44(b + 36);
            zz = Lfs44(b + 40);
        }
    }
    const u32 Z = out + 32, Y = out + 16;
    store<u32>(Z + 4, W44(zy));
    store<u32>(Z, W44(zx));
    store<u32>(Z + 8, W44(zz));
    f64 f1 = Lfs44(b + 16);
    Cp44(Y, b + 16);
    f1 = Lfs44(b + 20);
    Cp44(Y + 4, b + 20);
    f64 f2 = Lfs44(b + 24);
    Cp44(Y + 8, b + 24);
    f2 = F44(f2 * Lfs44(Z + 4));
    f1 = Fms44(f1, Lfs44(Z + 8), f2);
    store<u32>(out, W44(f1));
    f1 = F44(Lfs44(Y) * Lfs44(Z + 8));
    f1 = Fms44(Lfs44(Y + 8), Lfs44(Z), f1);
    store<u32>(out + 4, W44(f1));
    f1 = F44(Lfs44(Y + 4) * Lfs44(Z));
    f1 = Fms44(Lfs44(Y), Lfs44(Z + 4), f1);
    f64 x0 = Lfs44(out);
    const f64 x1 = Lfs44(out + 4);
    store<u32>(out + 8, W44(f1));
    f64 l = Fma44(x0, x0, F44(x1 * x1));
    const f64 x2 = Lfs44(out + 8);
    l = Fma44(x2, x2, l);
    const f64 k = F44(Lfs44(0x001D09B0u) / F44(std::sqrt(l)));
    store<u32>(out, W44(F44(x0 * k)));
    store<u32>(out + 4, W44(F44(x1 * k)));
    store<u32>(out + 8, W44(F44(x2 * k)));
    Cp44(out + 48, b + 48);
    Cp44(out + 52, b + 52);
    Cp44(out + 56, b + 56);
    ctx.fpr[1] = Lfs44(b + 56);
    ret(ctx, b + 48);
}

// void f(T* o, const T* src) (copy constructor): Thunk_000E27FC(o, src),
// vtable 0x00B17FF0, then the words / floats +224..+396 of src into
// o->+304..+476 (in the original's order). r3 = src->+392; f1 =
// src->+396.
DECOMP(001E5170, Obj_CopyCtor_B17FF0_001E5170) {
    const u32 o = arg<u32>(ctx, 0), src = arg<u32>(ctx, 1);
    call(ctx, 0x000E27FCu, {o, src});
    auto L = [&](u32 off) { return load<u32>(src + off); };
    auto S = [&](u32 off, u32 v) { store<u32>(o + off, v); };
    for (u32 k = 0; k < 7; ++k) S(304 + 4 * k, L(224 + 4 * k));
    u32 w = L(252), f = L(264);
    S(332, w); w = L(256); S(344, f); S(336, w);
    w = L(260); f = L(272); S(340, w); S(352, f);
    f = L(276); S(356, f);
    f = L(280); w = L(316); S(360, f);
    f = L(288); S(396, w); S(368, f);
    f = L(292); w = L(320); S(372, f);
    f = L(296); S(400, w); S(376, f);
    w = L(324); S(0, 0x00B17FF0u);
    f = L(300); S(404, w); S(380, f);
    w = L(328); f = L(304); S(408, w); S(384, f);
    w = L(332); f = L(308); S(412, w); S(388, f);
    w = L(336); f = L(312); S(416, w); S(392, f);
    w = L(340); S(420, w);
    w = L(344); S(424, w);
    w = L(348); f = L(368); S(428, w);
    w = L(352); S(448, f); S(432, w);
    f = L(372); w = L(384); S(452, f);
    f = L(376); S(464, w); S(456, f);
    w = L(388); f = L(396); S(468, w);
    w = L(392); S(476, f); S(472, w);
    ctx.fpr[1] = static_cast<f64>(std::bit_cast<f32>(f));
    ret(ctx, w);
}

// u32 f(T* o, f32 mass): with the body b = o->+416: when the fixed state
// (mass != *0x001FB1C0) differs from b's (Comp_GetF32_922918(b + 224) !=
// it): Hk_SetMotion_0091D470(b, 1 / 5, 1, 0) (dynamic / fixed), the u8
// b->+42 = 4 / 0 (dynamic also sets bit 0 of the u8 +33), the low half of
// b->+44 = 4 / 2, and Hk_UpdateEntityGroup(b->+8, b, 0, 1) when b->+8 is
// set. Then Hk_SetMass_0091D618(o->+416, mass). r3 = 0 without a body,
// else Hk_SetMass's; f1 = mass.
DECOMP(001FB1C8, Body_SetMass_001FB1C8) {
    const u32 o = arg<u32>(ctx, 0);
    const f64 mass = farg(ctx, 0);
    u32 bd = load<u32>(o + 416);
    if (bd == 0) {
        ret(ctx, 0);
        return;
    }
    const f64 zero = Lfs44(0x001FB1C0u);
    const u32 grp = load<u32>(bd + 8);
    const bool dyn = !(mass == zero);
    call(ctx, 0x00922918u, {bd + 224});
    const bool cur = !(ctx.fpr[1] == zero);
    bd = load<u32>(o + 416);
    if (dyn != cur) {
        if (dyn) {
            call(ctx, 0x0091D470u, {bd, 1, 1, 0});
            bd = load<u32>(o + 416);
            store<u8>(bd + 42, 4);
            store<u8>(bd + 33, static_cast<u8>(load<u8>(bd + 33) | 1u));
        } else {
            call(ctx, 0x0091D470u, {bd, 5, 1, 0});
            bd = load<u32>(o + 416);
            store<u8>(bd + 42, 0);
        }
        store<u32>(bd + 44, (load<u32>(bd + 44) & 0xFFFF0000u) | (dyn ? 4u : 2u));
        if (grp != 0) {
            call(ctx, 0x00930B70u, {grp, bd, 0, 1});
            bd = load<u32>(o + 416);
        }
    }
    callf(ctx, 0x0091D618u, {bd}, {mass});
}
