// Game code, part 38 (continued): the small-function sweep, 79
// instructions and up. Each was checked against the original in the PPC
// simulator (memory writes, calls and their arguments, r3 and f1).
#include "obj38.h"

using namespace sly4;
using namespace sly4::obj38;

// ---------------------------------------------------------------- sweep: 79 instructions
// u32 f(T* o) (no direct caller: a vtable slot): the direction +192 = the
// f32 vector +112 - +176; its y is cleared when bit 5 of the u8 +305 is
// clear and either bit 6 is set or o->vt[33](o) is 0 (OPD / entry after
// the arguments); then normalized (1.0 / |d|, single precision); the
// counter +24 grows up to 4; o->vt[55](o, 6) (r3 = its; f1 = the y).
DECOMP(00027588, Obj_AimDir_00027588) {
    const u32 o = arg<u32>(ctx, 0);
    const f64 x = F38(Lfs32(o + 112) - Lfs32(o + 176));
    const f64 y = F38(Lfs32(o + 116) - Lfs32(o + 180));
    const f64 z = F38(Lfs32(o + 120) - Lfs32(o + 184));
    const u32 b = load<u8>(o + 305);
    Stfs32(o + 192, x);
    Stfs32(o + 196, y);
    Stfs32(o + 200, z);
    if (((b >> 5) & 1u) == 0 && (((b >> 6) & 1u) != 0 || static_cast<u32>(VCall(ctx, o, 132, {o})) == 0))
        store<u32>(o + 196, 0);
    const f64 dy = Lfs32(o + 196), dx = Lfs32(o + 192), dz = Lfs32(o + 200);
    f64 l = F38(dy * dy);
    l = F38(std::fma(dx, dx, l));
    l = F38(std::fma(dz, dz, l));
    l = F38(std::sqrt(l));
    const f64 s = F38(Lfs32(0x00027580u) / l);  // 1.0f
    const u32 n = load<u32>(o + 24) + 1;
    const f64 ny = F38(dy * s);
    Stfs32(o + 192, F38(dx * s));
    Stfs32(o + 196, ny);
    Stfs32(o + 200, F38(dz * s));
    if (n < 4) store<u32>(o + 24, n);
    ctx.fpr[1] = ny;
    VCall(ctx, o, 220, {o, 6});
}

// u32 f(T* o, s32 k) (no direct caller): the animation set x of the
// 116-byte move entry e = +612 + 116 * +716 + 92 (its +80 with bit 30 of
// +516, else +60 with bit 31, else +20) advanced by 44 * +596; when +600
// is below x->+40, the animation of slot k of its 12-byte row +600 (at
// x + 4) is restarted with Anim_Restart2775A8(o->+64, anim, 0, 0, 0, -1,
// 0, 0, -, -, 0, -1; the floats 0x00044690 [0], [1], [2], [2]) and, when
// that started, +32 = anim. r3 as the original leaves it.
DECOMP(000446A0, Obj_PlayMoveAnim_000446A0) {
    const u32 o = arg<u32>(ctx, 0);
    const s32 k = arg<s32>(ctx, 1);
    const u32 e = load<u32>(o + 612) + load<u32>(o + 716) * 116u + 92u;
    const u32 w = load<u32>(o + 516);
    const u32 set = (w & 0x40000000u) != 0 ? load<u32>(e + 80) : (w & 0x80000000u) != 0 ? load<u32>(e + 60) : load<u32>(e + 20);
    const u32 x = set + load<u32>(o + 596) * 44u;
    const u32 row = load<u32>(o + 600);
    if (!(row < load<u32>(x + 40))) {
        ret(ctx, row);
        return;
    }
    const u32 anim = load<u32>(x + row * 12u + 4u + static_cast<u32>(k) * 4u);
    constexpr u32 K = 0x00044690u;  // {1.0f, 0.25f, 0.0f}
    const f64 c = Lfs32(K + 8);
    // Arguments 9 / 10 (r1 +112 / +120) are never written (float slots).
    callf(ctx, 0x002775A8u, {load<u32>(o + 64), anim, 0, 0, 0, ~0ull, 0, 0, 0, 0, 0, ~0ull}, {Lfs32(K), Lfs32(K + 4), c, c});
    if (static_cast<u32>(ctx.gpr[3]) != 0) store<u32>(o + 32, anim);
}

// bool f(T* o) (no direct caller): with the world's player p =
// (*0x00F65CA0 + 4208)->+8 (bit 1 of +192 set, p->vt[80](p) 0; OPD /
// entry after the arguments), o's state +4 not 16 and its current 20-byte
// slot (+424 + 20 * +1796) holding +1756: 1 when the request +1800 is
// 0x93D8EE68 / 69 / 6A and the matching u8 +1804 / +1805 / +1806 is set.
// 0 otherwise.
DECOMP(0004B200, Obj_CanAccept_0004B200) {
    const u32 o = arg<u32>(ctx, 0);
    auto no = [&] { ret(ctx, 0u); };
    const u32 p = load<u32>(load<u32>(0x00F65CA0u) + 4208 + 8);
    if (p == 0 || (load<u32>(p + 192) & 2u) == 0) return no();
    if (static_cast<u32>(VCall(ctx, p, 320, {p})) != 0) return no();
    if (load<u32>(o + 4) == 16) return no();
    if (load<u32>(o + load<u32>(o + 1796) * 20u + 424) != load<u32>(o + 1756)) return no();
    const u32 r = load<u32>(o + 1800);
    for (u32 i = 0; i < 3; ++i)
        if (r == 0x93D8EE68u + i && load<u8>(o + 1804 + i) != 0) {
            ret(ctx, 1u);
            return;
        }
    no();
}

// void f(u32* out, u32 handle, u32* src) (no direct caller): the handle
// (index << 16 | serial) names the 16-byte entry e of the table
// *0x00F65798 (count +24, data +28; e->+12 holds the serial in bits
// 15..30, valid while its top bit is clear) and its def d = e->+8; *out =
// 0 unless d's class (u16 (d->vt - 152)->+6) has bit 7 ("The source
// knowledge list is not a def, cannot clone it." otherwise); then *out =
// the first word of a stack clone request {0, u16 0, u8s 0, 0, d, u8 0, 0,
// *src, u16 64, u16 0, 0} handed to 0x00390DC8(*0x00F65798, &req + 4,
// &req). r3 = *out.
DECOMP(00065290, Knowledge_Clone_00065290) {
    const u32 out = arg<u32>(ctx, 0), h = arg<u32>(ctx, 1), src = arg<u32>(ctx, 2);
    const u32 serial = h & 0xFFFFu;
    const u32 g = load<u32>(0x00F65798u);
    auto none = [&] {
        store<u32>(out, 0);
        ret(ctx, 0u);
    };
    if (serial == 0 || static_cast<s32>(h >> 16) >= load<s32>(g + 24)) return none();
    const u32 e = load<u32>(g + 28) + ((h >> 12) & 0xFFFF0u);
    if (((load<u32>(e + 12) >> 15) & 0xFFFFu) != serial || (load<u8>(e + 12) & 0x80u) != 0) return none();
    const u32 d = load<u32>(e + 8);
    if (d == 0) return none();
    if (((load<u16>(load<u32>(d) - 152 + 6) >> 7) & 1u) == 0) {
        call(ctx, o38fn::String_Format, {0x00AC0554u});
        return none();
    }
    Stack fr(ctx, 32);  // the original's 112..143
    const u32 q = fr.addr();
    store<u8>(q + 4, 0);
    store<u8>(q + 5, 0);
    store<u32>(q + 16, 0);
    store<u16>(q + 24, 64);
    store<u16>(q + 26, 0);
    store<u32>(q + 28, 0);
    store<u32>(q + 8, d);
    store<u8>(q + 12, 0);
    store<u32>(q, 0);
    store<u32>(q + 20, load<u32>(src));
    store<u16>(q + 2, 0);
    call(ctx, 0x00390DC8u, {load<u32>(0x00F65798u), q + 4, q});
    const u32 r = load<u32>(q);
    store<u32>(out, r);
    ret(ctx, r);
}

// T* f(-, XmlCursor* x) (no direct caller: a factory): p = Mem_Alloc(24, 0,
// 16); when not 0: Obj_InitVt_B21C98(p, x), +12 / +16 and the u8s
// +20..+22 = 0, vtable 0x00B0F658; from x the hashes 0x00014B07 -> +12
// (default 0) and 0x7A30D7E3 -> +16 and the bools 0xCADB1721 -> +20,
// 0x5543F7DF -> +21 and 0xF5897C08 -> +22 (defaulting to the current
// values). r3 = p.
DECOMP(00074D34, XmlNode_New_00074D34) {
    const u32 x = arg<u32>(ctx, 1);
    const u32 p = static_cast<u32>(call(ctx, 0x0054ED38u, {24, 0, 16}));
    if (p == 0) {
        ret(ctx, 0u);
        return;
    }
    call(ctx, 0x004BB3B0u, {p, x});
    store<u32>(p + 12, 0);
    store<u32>(p + 16, 0);
    store<u8>(p + 20, 0);
    store<u8>(p + 21, 0);
    store<u32>(p, 0x00B0F658u);
    store<u8>(p + 22, 0);
    call(ctx, 0x004FA14Cu, {x, 0x00014B07u, p + 12, 0});
    call(ctx, 0x004FA14Cu, {x, 0x7A30D7E3u, p + 16, load<u32>(p + 16)});
    call(ctx, 0x004F986Cu, {x, 0xCADB1721u, p + 20, load<u8>(p + 20)});
    call(ctx, 0x004F986Cu, {x, 0x5543F7DFu, p + 21, load<u8>(p + 21)});
    call(ctx, 0x004F986Cu, {x, 0xF5897C08u, p + 22, load<u8>(p + 22)});
    ret(ctx, p);
}

// u32 f(Cmd* c, Effect* e) (no direct caller): the target t: the +280 of
// the +28 object (its +276 = 0x02235C33) of the component
// Effect_FindComponent(e, c->+12) whose vt[8]() is 5 (OPD / entry after
// the arguments), replaced by Obj_FindById(c->+16) when that is set; for
// t with bit 19 of +196: Obj_CallByKey420_001AE9C8(t, c->+20) when set,
// and when bit 31 of c->+24 is set, bit 29 of t->+552 = bit 30 of c->+24.
// r3 = 0, or that bit.
DECOMP(0007A240, SeqCmd_SetTargetFlag_0007A240) {
    const u32 c = arg<u32>(ctx, 0), e = arg<u32>(ctx, 1);
    u32 t = 0;
    if (const u32 k = load<u32>(c + 12); k != 0) {
        const u32 m = static_cast<u32>(call(ctx, 0x004BB4B4u, {e, k}));
        if (m != 0 && static_cast<s32>(VCall(ctx, m, 32, {m})) == 5) {
            const u32 q = load<u32>(m + 28);
            if (q != 0 && load<u32>(q + 276) == 0x02235C33u) t = load<u32>(q + 280);
        }
    }
    if (const u32 id = load<u32>(c + 16); id != 0) t = static_cast<u32>(call(ctx, 0x000D6A28u, {id}));
    if (t == 0 || (load<u32>(t + 196) & 0x00080000u) == 0) {
        ret(ctx, 0u);
        return;
    }
    if (const u32 k = load<u32>(c + 20); k != 0) call(ctx, 0x001AE9C8u, {t, k});
    const u32 w = load<u32>(c + 24);
    if ((w & 0x80000000u) == 0) {
        ret(ctx, 0u);
        return;
    }
    const u32 b = (w >> 30) & 1u;
    store<u32>(t + 552, (load<u32>(t + 552) & ~0x20000000u) | (b << 29));
    ret(ctx, b);
}

// T* f(-, XmlCursor* x) (no direct caller: a factory): p = Mem_Alloc(160,
// 0, 16); when not 0: Obj_InitVt_B21C98(p, x), vtable 0x00B0FEF0, the
// vec2s +144 / +152 = 0x01164AF4 / 0x01164AFC, the text +12 (u8 0, then up
// to 128 bytes of the attribute "name" when present and not empty); from
// x the vec2s 0x04B1112A -> +144 and 0x8ACA5893 -> +152 (defaulting to
// the current values). r3 = p (f1 as the original leaves it).
DECOMP(00083424, XmlNode_New_00083424) {
    const u32 x = arg<u32>(ctx, 1);
    const u32 p = static_cast<u32>(call(ctx, 0x0054ED38u, {160, 0, 16}));
    if (p == 0) {
        ret(ctx, 0u);
        return;
    }
    call(ctx, 0x004BB3B0u, {p, x});
    store<u32>(p + 144, load<u32>(0x01164AF4u));
    store<u32>(p, 0x00B0FEF0u);
    store<u32>(p + 148, load<u32>(0x01164AF8u));
    store<u32>(p + 152, load<u32>(0x01164AFCu));
    store<u8>(p + 12, 0);
    store<u32>(p + 156, load<u32>(0x01164B00u));
    ctx.fpr[1] = Lfs32(0x01164B00u);  // f1 as the original leaves it
    const u32 s = static_cast<u32>(call(ctx, 0x004F9628u, {x, 0x00AC4F18u}));
    if (s != 0 && load<u8>(s) != 0) call(ctx, 0x00553638u, {p + 12, s, 128});
    call(ctx, 0x004F9C9Cu, {x, 0x04B1112Au, p + 144, p + 144});
    call(ctx, 0x004F9C9Cu, {x, 0x8ACA5893u, p + 152, p + 152});
    ret(ctx, p);
}

// T* f(-, XmlCursor* x) (no direct caller: a factory): p = Mem_Alloc(28, 0,
// 16); when not 0: Obj_InitVt_B21C98(p, x), vtable 0x00B0FF18, the colors
// +12 = 0 and +16 = {0, 0, 0, 255}, f32 +20 = 1.0, u8 +24 = 0; from x the
// colors 0xBF5BC441 -> +12 and 0x95B5E768 -> +16, the f32 0xB7EA3D94 ->
// +20 and the bool 0x0AA56D8B -> +24 (defaulting to the current values).
// r3 = p.
DECOMP(00083620, XmlNode_New_00083620) {
    const u32 x = arg<u32>(ctx, 1);
    const u32 p = static_cast<u32>(call(ctx, 0x0054ED38u, {28, 0, 16}));
    if (p == 0) {
        ret(ctx, 0u);
        return;
    }
    call(ctx, 0x004BB3B0u, {p, x});
    store<u8>(p + 12, 0);
    store<u8>(p + 13, 0);
    store<u8>(p + 14, 0);
    store<u32>(p, 0x00B0FF18u);
    for (u32 k = 15; k <= 18; ++k) store<u8>(p + k, 0);
    store<u8>(p + 19, 255);
    store<u32>(p + 20, 0x3F800000u);
    store<u8>(p + 24, 0);
    call(ctx, 0x004F9F9Cu, {x, 0xBF5BC441u, p + 12, p + 12});
    call(ctx, 0x004F9F9Cu, {x, 0x95B5E768u, p + 16, p + 16});
    callf(ctx, 0x004F9B20u, {x, 0xB7EA3D94u, p + 20}, {Lfs32(p + 20)});
    call(ctx, 0x004F986Cu, {x, 0x0AA56D8Bu, p + 24, load<u8>(p + 24)});
    ret(ctx, p);
}

// u32 f(T* o; f32 v) (no direct caller: a vtable slot): when the world's
// physics system ((*0x00F65CA0 + 0x1281D4)->+4) exists, the u16 +510 of
// each body +28 of the 48-byte entries of o->+388 (data +0, u16 count +8)
// = the high half of v's f32 bits; then each 240-byte entry of o->+340
// (re-read) whose +192 x is set gets x->vt[28](x; f1 = v) (OPD / entry
// after the arguments). r3 = o->+340.
DECOMP(000E65F0, Obj_SetBodyParam_000E65F0) {
    const u32 o = arg<u32>(ctx, 0);
    const f64 v = farg(ctx, 0);
    if (load<u32>(load<u32>(load<u32>(0x00F65CA0u) + 0x001281D4u) + 4) != 0) {
        const u32 a = load<u32>(o + 388);
        for (u32 i = 0; i < (a != 0 ? static_cast<u32>(load<u16>(a + 8)) : 0u); ++i) {
            const u32 b = load<u32>(load<u32>(a) + 48 * i + 28);
            if (b != 0) store<u16>(b + 510, static_cast<u16>(std::bit_cast<u32>(static_cast<f32>(v)) >> 16));
        }
    }
    u32 a;
    for (u32 i = 0;; ++i) {
        a = load<u32>(o + 340);
        if (i >= (a != 0 ? static_cast<u32>(load<u16>(a + 8)) : 0u)) break;
        if (const u32 x = load<u32>(load<u32>(a) + 240 * i + 192); x != 0) {
            ctx.fpr[1] = v;
            VCall(ctx, x, 112, {x});
        }
    }
    ret(ctx, a);
}

// void f(Vec3* out, Box* o) (from several callers): a random point of the
// box: three 16-bit values from the generator state 0x01164888 (the u64
// +0 = the previous value, the word +8 stepped by s = rotl(s, 4) +
// rotl(s, 16) ^ 0x87654321, then twice by t = rotl(t, 20) + t ^
// 0x87654321; +8 = rotl(last, 16), +0 = last & 0xFFFF), each scaled by
// k = the f32 0x000F64C4 (1 / 65535) into out = o->+128 + (2 r k - 1) *
// o->+144 per axis (single precision, fused). f1 = out.z.
DECOMP(000F64C8, Rand_PointInBox_000F64C8) {
    const u32 out = arg<u32>(ctx, 0), o = arg<u32>(ctx, 1);
    constexpr u32 S = 0x01164888u, M = 0x87654321u;
    auto rotl = [](u32 v, u32 n) { return (v << n) | (v >> (32 - n)); };
    const s64 prev = static_cast<s64>(load<u64>(S));
    const u32 s = load<u32>(S + 8);
    const u32 t1 = (rotl(s, 4) + rotl(s, 16)) ^ M;
    const u32 t2 = (rotl(t1, 20) + t1) ^ M;
    const u32 t3 = (rotl(t2, 20) + t2) ^ M;
    const f64 k = Lfs32(0x000F64C4u);
    auto axis = [&](f64 r, u32 c, u32 e) {
        const f64 a = Lfs32(e);
        const f64 sc = F38(r * k);
        const f64 w = F38(a - (-a));
        return F38(Lfs32(c) + F38(std::fma(w, sc, -a)));
    };
    const f64 x = axis(F38(static_cast<f64>(prev)), o + 128, o + 144);
    const f64 y = axis(static_cast<f64>(t1 & 0xFFFFu), o + 132, o + 148);
    const f64 z = axis(static_cast<f64>(t2 & 0xFFFFu), o + 136, o + 152);
    store<u32>(S + 8, rotl(t3, 16));
    store<u64>(S, t3 & 0xFFFFu);
    Stfs32(out, x);
    Stfs32(out + 4, y);
    Stfs32(out + 8, z);
    ctx.fpr[1] = z;
}

// u32 f(T* o, s32 s) (from 0x001676F8): the physics state +396 = s when
// it changes (2 is stored as 4); 2 / 4: the body h = +388 leaves its
// world (Hk_RemoveEntity92FFC8(stack result, h->+8, h) when in one), the
// u8s +448 / +449 and +452 = 0; 1: a body not in a world, with bit 7 of
// +172 clear, gets the collision layer of h->+44 (bits 16..31 = +412, bits 0..4
// kept) and Hk_AddEntity92F2A8(sys, h, 1) (sys = the world's
// (*0x00F65CA0 + 0x1281D4)->+4), then 0x0092E9D8(sys, +392) when set;
// +400 / +404 = 0. r3 as the original leaves it.
DECOMP(00167570, Phys_SetState_00167570) {
    const u32 o = arg<u32>(ctx, 0);
    s32 s = arg<s32>(ctx, 1);
    const u32 h = load<u32>(o + 388);
    if (load<s32>(o + 396) == s) {
        ret(ctx, h);
        return;
    }
    u32 r3 = h;
    if (s == 2 || s == 4) {
        s = 4;
        if (h != 0 && load<u32>(h + 8) != 0) {
            Stack fr(ctx, 16);
            call(ctx, 0x0092FFC8u, {fr.addr(), load<u32>(h + 8), h});
        }
        store<u8>(o + 448, 0);
        store<u8>(o + 449, 0);
        store<u32>(o + 452, 0);
        r3 = 0;
    } else if (s == 1) {
        if (h != 0 && load<u32>(h + 8) == 0 && ((load<u32>(o + 172) >> 7) & 1u) == 0) {
            const u32 w = load<u32>(0x00F65CA0u);
            const u32 l = (load<u32>(h + 44) & 0x1Fu) | (load<u32>(o + 412) << 16);
            const u32 sys = load<u32>(load<u32>(w + 0x001281D4u) + 4);
            store<u32>(h + 44, l);
            call(ctx, 0x0092F2A8u, {sys, h, 1});
            if (const u32 c = load<u32>(o + 392); c != 0) call(ctx, 0x0092E9D8u, {sys, c});
        }
        store<u32>(o + 400, 0);
        store<u32>(o + 404, 0);
        r3 = 0;
    }
    store<u32>(o + 396, static_cast<u32>(s));
    ret(ctx, r3);
}

// u32 f(T* o, a, b, c, Src* src) (no direct caller): 0x009FD904(o, a, b,
// c, src) (the registers passed on); +28 / +32 = src's f32s +0 / +4; the
// f32 array +36 (capacity bits 0..29 of +44, bit 31 = not owned) grows to
// the count n = src->+12 when smaller (the old buffer freed through the
// allocator object 0x011DF31C's vt[5](a, buf, cap * 4) when owned; the
// new one from its vt[4](a, &bytes), +44 = bytes / 4), +40 = n and the n
// values of src->+8 are copied. r3 = the buffer (f1 as the original
// leaves it).
DECOMP(0023DF90, FloatArray_Assign_0023DF90) {
    const u32 o = arg<u32>(ctx, 0), src = arg<u32>(ctx, 4);
    call(ctx, 0x009FD904u, {o, ctx.gpr[4], ctx.gpr[5], ctx.gpr[6], src});
    constexpr u32 A = 0x011DF31Cu;
    store<u32>(o + 28, load<u32>(src));
    ctx.fpr[1] = Lfs32(src + 4);
    store<u32>(o + 32, load<u32>(src + 4));
    const u32 w = load<u32>(o + 44);
    u32 buf = load<u32>(o + 36);
    if (static_cast<s32>(w & 0x3FFFFFFFu) < load<s32>(src + 12)) {
        if ((w & 0x80000000u) == 0) VCall(ctx, A, 20, {A, buf, static_cast<u64>(static_cast<s64>(static_cast<s32>(w << 2)))});
        Stack fr(ctx, 16);
        store<u32>(fr.addr(), load<u32>(src + 12) << 2);
        buf = static_cast<u32>(VCall(ctx, A, 16, {A, fr.addr()}));
        const s32 bytes = load<s32>(fr.addr());
        store<u32>(o + 36, buf);
        store<u32>(o + 44, static_cast<u32>(bytes / 4));
    }
    const s32 n = load<s32>(src + 12);
    store<u32>(o + 40, static_cast<u32>(n));
    const u32 d = load<u32>(src + 8);
    for (s32 i = 0; i < n; ++i) {
        const u32 v = load<u32>(d + 4 * i);
        store<u32>(buf + 4 * i, v);
        ctx.fpr[1] = static_cast<f64>(std::bit_cast<f32>(v));
    }
    ret(ctx, buf);
}

// u32 f(T* o, a, u8 b) (no direct caller: a vtable slot): for each child c
// of the pointer array +28 (u16 count +36): its controller +52 (when set)
// and each of the 8-byte entries +8 (u16 count +16) x get vt[32](x, a, b)
// (OPD / entry after the arguments). r3 = the count.
DECOMP(00267FE0, Children_Broadcast_00267FE0) {
    const u32 o = arg<u32>(ctx, 0), a = arg<u32>(ctx, 1), b = arg<u32>(ctx, 2) & 0xFFu;
    u32 n;
    for (u32 i = 0; i < (n = load<u16>(o + 36)); ++i) {
        const u32 c = load<u32>(load<u32>(o + 28) + 4 * i);
        if (const u32 x = load<u32>(c + 52); x != 0) VCall(ctx, x, 128, {x, a, b});
        for (u32 j = 0; j < load<u16>(c + 16); ++j)
            if (const u32 x = load<u32>(load<u32>(c + 8) + 8 * j); x != 0) VCall(ctx, x, 128, {x, a, b});
    }
    ret(ctx, n);
}

// u32 f(T* o, u32 k) (from several callers): for k != 0, k is removed
// from the u16-counted array +920 (count +928; the last entry moved into
// its slot); when the world's player p = (*0x00F65CA0 + 4208)->+8 has bit
// 3 of +192 and Obj_Notify24(p) is not 0: Records_RemoveKey_0029F7A0(
// Obj_Notify24(the player again, or 0 when it lost the bit), k) (r3 =
// its). r3 as the original leaves it.
DECOMP(0030C2B4, Records_Forget_0030C2B4) {
    const u32 o = arg<u32>(ctx, 0), k = arg<u32>(ctx, 1);
    if (k == 0) return;
    const u32 n = load<u16>(o + 928);
    const u32 d = load<u32>(o + 920);
    for (u32 i = 0; i < n; ++i) {
        if (load<u32>(d + 4 * i) != k) continue;
        const u32 m = n - 1;
        store<u16>(o + 928, static_cast<u16>(m));
        if (static_cast<s32>(i) != static_cast<s32>(m & 0xFFFFu)) store<u32>(d + 4 * i, load<u32>(d + 4 * (m & 0xFFFFu)));
        break;
    }
    auto player = [] {
        const u32 p = load<u32>(load<u32>(0x00F65CA0u) + 4208 + 8);
        return p != 0 && (load<u32>(p + 192) & 8u) != 0 ? p : 0u;
    };
    const u32 p = load<u32>(load<u32>(0x00F65CA0u) + 4208 + 8);
    if (p == 0) {
        ret(ctx, 0u);
        return;
    }
    if ((load<u32>(p + 192) & 8u) == 0) {
        ret(ctx, p);
        return;
    }
    if (static_cast<u32>(call(ctx, 0x000D949Cu, {p})) == 0) {
        ret(ctx, 0u);
        return;
    }
    const u32 r = static_cast<u32>(call(ctx, 0x000D949Cu, {player()}));
    call(ctx, 0x0029F7A0u, {r, k});
}

// u32 f(Section* s, u8 on) (no direct caller: a vtable slot):
// 0x0032549C(s, on, r5 as passed in); then for each child c of the list
// +2576 (next +2564), with the u8 mode m = s->+18: in state 6 (+2572) and
// mode 4, a child not in mode 5 (u8 c->+18), or with the u8 +2557 set and
// on, gets c->vt[10](c, c->+20 | 1) and c->vt[6](c, 0, 0xFFFFFFFF) (OPD /
// entry after the arguments); any other gets c->vt[6](c, on, m). r3 as
// the original leaves it.
DECOMP(003277E4, Section_SetChildren_003277E4) {
    const u32 s = arg<u32>(ctx, 0), on = arg<u32>(ctx, 1) & 0xFFu;
    // r5 is not set by the original: 0x0032549C reads the r5 this function
    // was called with.
    call(ctx, 0x0032549Cu, {s, on, ctx.gpr[5]});
    const u32 m = load<u8>(s + 18);
    for (u32 c = load<u32>(s + 2576); c != 0; c = load<u32>(c + 2564)) {
        bool reset = false;
        if (load<u32>(s + 2572) == 6 && m == 4) reset = load<u8>(c + 18) != 5 || (load<u8>(s + 2557) != 0 && on != 0);
        if (reset) {
            VCall(ctx, c, 40, {c, load<u32>(c + 20) | 1u});
            VCall(ctx, c, 24, {c, 0, 0xFFFFFFFFu});
        } else {
            VCall(ctx, c, 24, {c, on, m});
        }
    }
}

// T* f(-, XmlCursor* x, a) (from 0x00322D88): from x (into stack
// defaults) the f32 0x0451539B (default 0.0), the bools 0x418A9FF0
// (false) and 0xDAC2D715 (true) and the f32 0xD4A5405E (default 10.0,
// the constants 0x00340438); p = SmallAlloc(2588) built by
// Obj_Construct_00769BD8(p, a) when not 0: +2572 = 0, +2576 = the first
// f32, u8s +2580 / +2581 = the first bool, +2582 = the second, vtable
// 0x00B1F0C0, +2584 = the second f32. r3 = p.
DECOMP(00340440, Obj_New_00340440) {
    const u32 x = arg<u32>(ctx, 1), a = arg<u32>(ctx, 2);
    constexpr u32 K = 0x00340438u;
    Stack fr(ctx, 16);  // f32 +0, f32 +4, u8 +8, u8 +9 (the original's 112..121)
    const u32 q = fr.addr();
    callf(ctx, 0x004F9B20u, {x, 0x0451539Bu, q}, {Lfs32(K)});
    call(ctx, 0x004F986Cu, {x, 0x418A9FF0u, q + 8, 0});
    call(ctx, 0x004F986Cu, {x, 0xDAC2D715u, q + 9, 1});
    callf(ctx, 0x004F9B20u, {x, 0xD4A5405Eu, q + 4}, {Lfs32(K + 4)});
    const u32 p = static_cast<u32>(call(ctx, 0x0055238Cu, {2588}));
    if (p == 0) {
        ret(ctx, 0u);
        return;
    }
    const u32 f0 = load<u32>(q), f1 = load<u32>(q + 4);
    const u8 b0 = load<u8>(q + 8), b1 = load<u8>(q + 9);
    call(ctx, 0x00769BD8u, {p, a});
    store<u32>(p + 2572, 0);
    store<u32>(p + 2576, f0);
    store<u8>(p + 2580, b0);
    store<u8>(p + 2581, b0);
    store<u8>(p + 2582, b1);
    store<u32>(p, 0x00B1F0C0u);
    store<u32>(p + 2584, f1);
    ret(ctx, p);
}

// T* f(Owner* a, u32 b) (from 0x0005BB20, Map_LookupThen_00367900): for a
// and b set and a's parent +8: p = SmallAlloc(400) built by
// UiObj_Construct35A868(p, parent), vtable 0x00B1F6E0, bit 0 of +4 set;
// the pair {b, p} is appended to the 8-byte entry array 0x0106C228 (u16
// capacity +0 doubled by Array_Reserve when full, u16 count +2, u16
// high-water +4, u16 flags +6 |= 1, data +8); +12 = b. r3 = p (0 when
// anything is missing).
DECOMP(003677C4, UiObj_Register_003677C4) {
    const u32 a = arg<u32>(ctx, 0), b = arg<u32>(ctx, 1);
    if (a == 0 || b == 0 || load<u32>(a + 8) == 0) {
        ret(ctx, 0u);
        return;
    }
    const u32 par = load<u32>(a + 8);
    const u32 p = static_cast<u32>(call(ctx, 0x0055238Cu, {400}));
    if (p == 0) {
        ret(ctx, 0u);
        return;
    }
    call(ctx, 0x0035A868u, {p, par});
    constexpr u32 V = 0x0106C228u;
    const u32 w4 = load<u32>(p + 4);
    store<u32>(p, 0x00B1F6E0u);
    const u32 cap = load<u16>(V);
    u32 cnt = load<u16>(V + 2);
    store<u32>(p + 4, w4 | 1u);
    if (cnt == cap) {
        call(ctx, 0x00558F3Cu, {V, (cap << 1) & 0xFFFEu});
        cnt = load<u16>(V + 2);
    }
    const u32 d = load<u32>(V + 8);
    const u32 c2 = load<u16>(V + 2);
    store<u32>(d + ((cnt << 3) & 0x7FFF8u), b);
    const u32 n1 = c2 + 1u;
    const u32 hw = load<u16>(V + 4);
    store<u16>(V + 2, static_cast<u16>(n1));
    store<u32>(d + (c2 << 3) + 4, p);
    const u32 fl = load<u16>(V + 6);
    store<u16>(V + 4, static_cast<u16>(static_cast<s32>(hw) > static_cast<s32>(n1 & 0xFFFFu) ? hw : n1));
    store<u16>(V + 6, static_cast<u16>(fl | 1u));
    store<u32>(p + 12, b);
    ret(ctx, p);
}

// u32 f(Cmd* c, Anim* o) (no direct caller: a vtable slot): the word form
// of the animation variable copy (AnimVarCopy38).
DECOMP(00391FF4, AnimVar_CopyU32_00391FF4) { AnimVarCopy38(ctx, 4); }

// u32 f(Cmd* c, Anim* o) (no direct caller): the f32 form of the
// animation variable copy (AnimVarCopy38; f1 = the value).
DECOMP(003925A4, AnimVar_CopyF32_003925A4) { AnimVarCopy38(ctx, 5); }

// u32 f(Cmd* c, Anim* o) (no direct caller): the byte form of the
// animation variable copy (AnimVarCopy38).
DECOMP(00392B54, AnimVar_CopyU8_00392B54) { AnimVarCopy38(ctx, 1); }

// u32 f(Cmd* c, Anim* o) (no direct caller): a second word form of the
// animation variable copy (AnimVarCopy38; another template instance).
DECOMP(00393104, AnimVar_CopyU32_00393104) { AnimVarCopy38(ctx, 4); }

// u32 f(T* o) (no direct caller: a vtable slot): with the owner +64 and the
// attached object +72: Obj_Detach0D72C0(+64, +72) and +72->vt[53](it, 0)
// (OPD / entry after the arguments); Obj_Flag13Off_0048F298(o + 56, 0);
// a layer +76 is popped (Obj_PopLayer(+72, +76, +72; f1 = 1.0)) and +76 /
// +72 cleared; the owner gets Obj_SetEffectHash_000DAEC8(it, 0xBC1BA9E7),
// vt[44](it, 0) and vt[10](it, 0) and +64 = 0; +68 = 0. r3 = 0.
DECOMP(003DAA70, Attach_Release_003DAA70) {
    const u32 o = arg<u32>(ctx, 0);
    if (load<u32>(o + 64) != 0 && load<u32>(o + 72) != 0) {
        call(ctx, 0x000D72C0u, {load<u32>(o + 64), load<u32>(o + 72)});
        const u32 x = load<u32>(o + 72);
        VCall(ctx, x, 212, {x, 0});
    }
    call(ctx, 0x0048F298u, {o + 56, 0});
    if (const u32 l = load<u32>(o + 76); l != 0) {
        const u32 x = load<u32>(o + 72);
        callf(ctx, 0x000FBB88u, {x, l, x}, {Lfs32(0x003DAA6Cu)});
        store<u32>(o + 76, 0);
        store<u32>(o + 72, 0);
    }
    if (const u32 w = load<u32>(o + 64); w != 0) {
        call(ctx, 0x000DAEC8u, {w, 0xBC1BA9E7u});
        u32 y = load<u32>(o + 64);
        VCall(ctx, y, 176, {y, 0});
        y = load<u32>(o + 64);
        VCall(ctx, y, 40, {y, 0});
        store<u32>(o + 64, 0);
    }
    store<u32>(o + 68, 0);
    ret(ctx, 0u);
}

// void f(Vec3* out, T* o, const Vec3* p) (from 0x00195440): the average
// push of p away from the points of o->+44 (16-byte entries, data +0, u16
// count +8) within (k1, k0) of it, k = 0x003DC9E4 {5.0, 1e-4, 1.0, 3.0}:
// each adds (p - e) / |p - e| * ((k0 - |p - e|) / k0) * k3 (single
// precision, fused), the sum divided by the count; the zero vector
// 0x01164B40 without any. f1 as the original leaves it.
DECOMP(003DC9F8, Avoid_Push_003DC9F8) {
    const u32 out = arg<u32>(ctx, 0), o = arg<u32>(ctx, 1), p = arg<u32>(ctx, 2);
    constexpr u32 K = 0x003DC9E4u, V = 0x01164B40u;
    f64 ax = Lfs32(V), ay = Lfs32(V + 4), az = Lfs32(V + 8);
    const f64 k0 = Lfs32(K), k1 = Lfs32(K + 4), k2 = Lfs32(K + 8), k3 = Lfs32(K + 12);
    const u32 a = load<u32>(o + 44);
    const u32 n = load<u16>(a + 8);
    u32 cnt = 0;
    for (u32 i = 0; i < n; ++i) {
        const u32 e = load<u32>(a) + 16 * i;
        f64 dy = F38(Lfs32(p + 4) - Lfs32(e + 4));
        f64 dx = F38(Lfs32(p) - Lfs32(e));
        f64 dz = F38(Lfs32(p + 8) - Lfs32(e + 8));
        f64 l = F38(dy * dy);
        l = F38(std::fma(dx, dx, l));
        l = F38(std::fma(dz, dz, l));
        l = F38(std::sqrt(l));
        if (!(l < k0) || !(l > k1)) continue;
        ++cnt;
        f64 w = F38(k0 - l);
        const f64 s = F38(k2 / l);
        w = F38(w / k0);
        dx = F38(w * F38(dx * s));
        dy = F38(w * F38(dy * s));
        dz = F38(w * F38(dz * s));
        ax = F38(std::fma(dx, k3, ax));
        ay = F38(std::fma(dy, k3, ay));
        az = F38(std::fma(dz, k3, az));
    }
    if (cnt == 0) {
        for (u32 k = 0; k < 12; k += 4) store<u32>(out + k, load<u32>(V + k));  // the raw words
        ctx.fpr[1] = k3;
        return;
    }
    const f64 r = F38(k2 / F38(static_cast<f64>(cnt)));
    const f64 z = F38(az * r);
    Stfs32(out, F38(ax * r));
    Stfs32(out + 4, F38(ay * r));
    Stfs32(out + 8, z);
    ctx.fpr[1] = z;
}

// T* f(T* o) (from Obj_AddHelper_0040C1E4): field defaults: vtable
// 0x00B20A38, f32 +16 / +20 = -1 / 1, the vectors +96 / +112 / +272 /
// +288 = 0x01164B40, the quaternions +128 / +144 = 0x01164C90, +72 = 3,
// the other words +4..+92 / +352..+360 and u8 +368 = 0. r3 = o (f1 as the
// original leaves it).
DECOMP(0040AB98, Helper_Init_0040AB98) {
    const u32 o = arg<u32>(ctx, 0);
    constexpr u32 V = 0x01164B40u, W = 0x01164C90u;
    auto v = [](u32 a) { return load<u32>(a); };
    store<u32>(o + 16, 0xBF800000u);
    store<u32>(o + 20, 0x3F800000u);
    store<u32>(o + 60, 0);
    store<u32>(o + 76, 0);
    store<u32>(o + 88, 0);
    store<u32>(o + 92, 0);
    store<u32>(o + 96, v(V));
    store<u32>(o + 100, v(V + 4));
    store<u32>(o + 104, v(V + 8));
    store<u32>(o + 112, v(V));
    store<u32>(o + 116, v(V + 4));
    store<u32>(o + 120, v(V + 8));
    store<u32>(o + 128, v(W));
    store<u32>(o, 0x00B20A38u);
    store<u32>(o + 132, v(W + 4));
    store<u32>(o + 4, 0);
    store<u32>(o + 136, v(W + 8));
    store<u32>(o + 8, 0);
    store<u32>(o + 12, 0);
    store<u32>(o + 140, v(W + 12));
    for (u32 k = 24; k <= 36; k += 4) store<u32>(o + k, 0);
    store<u32>(o + 144, v(W));
    for (u32 k = 40; k <= 56; k += 4) store<u32>(o + k, 0);
    store<u32>(o + 64, 0);
    store<u32>(o + 148, v(W + 4));
    store<u32>(o + 68, 0);
    store<u32>(o + 72, 3);
    store<u32>(o + 80, 0);
    store<u32>(o + 84, 0);
    store<u32>(o + 152, v(W + 8));
    store<u32>(o + 156, v(W + 12));
    store<u32>(o + 272, v(V));
    store<u32>(o + 276, v(V + 4));
    store<u32>(o + 280, v(V + 8));
    store<u32>(o + 288, v(V));
    store<u32>(o + 292, v(V + 4));
    store<u32>(o + 352, 0);
    store<u32>(o + 296, v(V + 8));
    store<u32>(o + 356, 0);
    store<u32>(o + 360, 0);
    store<u8>(o + 368, 0);
    ctx.fpr[1] = Lfs32(V + 8);  // f1 as the original leaves it
}

// bool f(Server* o) (from 0x000A9768): with fewer than 3 clients (+20):
// the slot +4 + 4 * count = sys_net::accept(+0, &addr, &len = 16) (stack
// sockaddr); for a client (> 0): when the u8s +24..+27 are set and differ
// (as s8) from the address bytes +4..+7 of the peer, or are not set yet
// (then +24..+27 = those bytes, u8 +28 = the last one again, +4 = the new
// slot's socket), +20 = 1 and r3 = 1. r3 = 0 otherwise.
DECOMP(00427120, Server_Accept_00427120) {
    const u32 o = arg<u32>(ctx, 0);
    if (!(load<u32>(o + 20) < 3)) {
        ret(ctx, 0u);
        return;
    }
    Stack fr(ctx, 32);  // len +0, sockaddr +4 (the original's 112..131)
    store<u32>(fr.addr(), 16);
    const u32 s = static_cast<u32>(call(ctx, o38imp::sys_net_accept, {static_cast<u64>(static_cast<s64>(load<s32>(o))), fr.addr() + 4, fr.addr()}));
    store<u32>(o + 4 + 4 * load<u32>(o + 20), s);
    if (load<s32>(o + 4 + 4 * load<u32>(o + 20)) <= 0) {
        ret(ctx, 0u);
        return;
    }
    u8 ip[4];
    for (u32 k = 0; k < 4; ++k) ip[k] = load<u8>(fr.addr() + 8 + k);
    bool same = load<u8>(o + 24) != 0;
    for (u32 k = 0; same && k < 4; ++k) same = static_cast<s8>(load<u8>(o + 24 + k)) == static_cast<s8>(ip[k]);
    if (load<u8>(o + 24) == 0 || same) {
        for (u32 k = 0; k < 4; ++k) store<u8>(o + 24 + k, ip[k]);
        store<u8>(o + 28, ip[3]);
        store<u32>(o + 4, load<u32>(o + 4 + 4 * load<u32>(o + 20)));
    }
    store<u32>(o + 20, 1);
    ret(ctx, 1u);
}

// f32 f(Body* o; f32 r) (from 0x00211E40, 0x00214640): resizes the sphere
// shape s = o->+16 of a body in a world (+8) when s is not locked (u8
// +8) and |r| >= 1e-5 (k = 0x0044DD48 {0, 1e-5}): Body_Remove931F00(world,
// o); a new 32-byte shape from the pool (Sda -28332)->+44 (vt[2](pool,
// 32), OPD / entry after the arguments; its u16 +4 = 32 written even when
// null: a quirk) built by Hk_SphereShapeCtor89FE70(p; f1 = r + the old
// radius s->+16) replaces s (AnimSkel_Release7D4EA8(s), 0x0085F1F0(o +
// 16)); Body_Add931D10(world, o). f1 = the old radius (0 without a
// world / when locked); r3 as the original leaves it.
DECOMP(0044DD50, Body_SetRadius_0044DD50) {
    const u32 o = arg<u32>(ctx, 0);
    const f64 r = farg(ctx, 0);
    constexpr u32 K = 0x0044DD48u;
    if (o == 0) {
        ctx.fpr[1] = Lfs32(K);
        return;
    }
    const u32 w = load<u32>(o + 8);
    if (w == 0) {
        ctx.fpr[1] = Lfs32(K);
        return;
    }
    const u32 s = load<u32>(o + 16);
    if (load<u8>(s + 8) != 0) {
        ctx.fpr[1] = Lfs32(K);
        ret(ctx, s);
        return;
    }
    const f64 old = Lfs32(s + 16);
    if (std::fabs(r) < Lfs32(K + 4)) {
        ctx.fpr[1] = old;
        ret(ctx, s);
        return;
    }
    call(ctx, 0x00931F00u, {w, o});
    const u32 pool = Pool28332(ctx);
    const u32 p = static_cast<u32>(VCall(ctx, pool, 8, {pool, 32}));
    store<u16>(p + 4, 32);
    if (p != 0) callf(ctx, 0x0089FE70u, {p}, {F38(r + old)});
    call(ctx, 0x007D4EA8u, {load<u32>(o + 16)});
    store<u32>(o + 16, p);
    call(ctx, 0x0085F1F0u, {o + 16});
    call(ctx, 0x00931D10u, {w, o});
    ctx.fpr[1] = old;
}

// u32 f(T* o, u8 on) (from several callers): when the u8 field
// (*0x010AB1B8)->+64 of o->+48's table (+12) is set: the pending count +24
// is taken (cleared) and each of its objects x (+20 array) with bit 18 of
// +172 gets, when on, x->vt[22](x, 0, 1, 1, 1), then the counter +32
// decreased and x->vt[10](x, 1) (OPD / entry after the arguments). r3 as
// the original leaves it.
DECOMP(00453578, Pending_Flush_00453578) {
    const u32 o = arg<u32>(ctx, 0), on = arg<u32>(ctx, 1) & 0xFFu;
    const u32 b = load<u8>(load<u32>(load<u32>(o + 48) + 12) + (load<u32>(load<u32>(0x010AB1A8u + 16) + 64) << 2));
    if (b == 0) {
        ret(ctx, 0u);
        return;
    }
    const s32 n = load<s32>(o + 24);
    store<u32>(o + 24, 0);
    u32 r3 = 0;
    for (s32 i = 0; i < n; ++i) {
        const u32 x = load<u32>(load<u32>(o + 20) + 4 * static_cast<u32>(i));
        r3 = (load<u32>(x + 172) >> 18) & 1u;
        if (r3 == 0) continue;
        if (on != 0) VCall(ctx, x, 88, {x, 0, 1, 1, 1});
        store<u32>(o + 32, load<u32>(o + 32) - 1);
        r3 = static_cast<u32>(VCall(ctx, x, 40, {x, 1}));
    }
    ret(ctx, r3);
}

// u32 f(AnimCmd* o) (from Instance_Base4B71BC): the key k = o->+20 when
// bit 7 of the u8 +24 is set (else 0) is looked up in the tree of the
// table T = 0x010C28F0 (root (+12)->+0, nodes {-, key +4, value +8, left
// +12 for larger keys, right +16}), the visited path pushed on T's
// s16-counted stack (+16, count +96, reset first); found: T->+100 = the
// node, *0x011DE2AC = its value v and, when v is set, r3 = v->vt[7](v, o)
// (OPD / entry after the arguments) followed by
// AnimCmdList_Read_004B6E74(r3, o); not found: T->+100 = the last node
// visited; an empty tree clears T->+100 and *0x011DE2AC. r3 = 0 unless
// found with a value.
DECOMP(004B707C, AnimCmd_Dispatch_004B707C) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 key = (load<u8>(o + 24) & 0x80u) != 0 ? load<u32>(o + 20) : 0u;
    constexpr u32 T = 0x010C28F0u;
    u32 n = load<u32>(load<u32>(T + 12));
    if (n == 0) {
        store<u32>(0x011DE2ACu, 0);
        store<u32>(T + 100, 0);
        ret(ctx, 0u);
        return;
    }
    store<u16>(T + 96, 0);
    for (;;) {
        const u32 k = load<u32>(n + 4);
        if (k == key) break;
        const u32 c = k > key ? load<u32>(n + 12) : load<u32>(n + 16);
        if (c == 0) {
            store<u32>(T + 100, n);  // (the original's null-node branch is unreachable)
            ret(ctx, 0u);
            return;
        }
        const s32 d = static_cast<s16>(load<u16>(T + 96));
        store<u16>(T + 96, static_cast<u16>(d + 1));
        store<u32>(T + 16 + 4 * static_cast<u32>(d), n);
        n = c;
    }
    store<u32>(T + 100, n);
    store<u32>(0x011DE2ACu, load<u32>(n + 8));
    const u32 v = load<u32>(n + 8);
    if (v == 0) {
        ret(ctx, 0u);
        return;
    }
    const u32 r = static_cast<u32>(VCall(ctx, v, 28, {v, o}));
    call(ctx, 0x004B6E74u, {r, o});
    ret(ctx, r);
}

// bool f(XmlCursor* x) (no direct caller): with the settings g =
// *0x010C2C28 (r3 = 0 without them): 1 when the u8 g->+585 is set;
// otherwise the attribute push PushAttrStr37 of "name" onto the vector g +
// 316.
DECOMP(004EE320, Settings_PushName_004EE320) {
    const u32 g = load<u32>(0x010C2C28u);
    if (g == 0) {
        ret(ctx, 0u);
        return;
    }
    if (load<u8>(g + 585) != 0) {
        ret(ctx, 1u);
        return;
    }
    PushAttrStr37(ctx, 0x00AF0120u, 316);
}
