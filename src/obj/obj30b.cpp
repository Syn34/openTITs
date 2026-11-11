// Game code, part 30 (continued): the small-function sweep from 52
// instructions (see obj30.cpp for the notes). Each was checked against the
// original in the PPC simulator.
#include "obj30.h"

using namespace sly4;
using namespace sly4::obj30;

// ---------------------------------------------------------------- sweep: 52 instructions
// void f(T* o, a, b, c; f32 v): each of the s32 o->+4 children o->+1616[k]
// gets vt[164](child, a, b, c; v) (r3 = the count)
DECOMP(0001B3BC, Obj_BroadcastChildren_0001B3BC) {
    const u32 o = arg<u32>(ctx, 0), a = arg<u32>(ctx, 1), b = arg<u32>(ctx, 2), c = arg<u32>(ctx, 3);
    const f64 v = farg(ctx, 0);
    ctx.gpr[3] = load<u32>(o + 4);
    for (s32 k = 0; k < static_cast<s32>(load<u32>(o + 4)); ++k) {
        const u32 ch = load<u32>(o + 1616 + 4 * static_cast<u32>(k));
        ctx.fpr[1] = v;
        VCall(ctx, ch, 164, {ch, a, b, c});
        ctx.gpr[3] = load<u32>(o + 4);
    }
}

// f32 f(T* o, const Vec3* p, const Vec3* q): the weighted score of the
// point p against the object's position o->+176 and facing o->+160, with
// the weights at 0x00F95580 / 0x00F955CC / 0x00F95618 and the constants
// k0 = *0x0004430C, k1 = *0x00044310: (len * w0 + k0) + (k1 - dot(B,
// n)) * w1 + (k1 - dot(q, B)) * w2 for the unit direction n = (p - A) *
// (k1 / len)
DECOMP(00044318, Obj_AimScore_00044318) {
    const u32 o = arg<u32>(ctx, 0), p = arg<u32>(ctx, 1), q = arg<u32>(ctx, 2);
    const u32 A = o + 176, B = o + 160;
    const f64 dy = Sub28(LfsAt(p + 4), LfsAt(A + 4));
    const f64 dx = Sub28(LfsAt(p), LfsAt(A));
    const f64 dz = Sub28(LfsAt(p + 8), LfsAt(A + 8));
    const f64 k1 = LfsAt(0x00044310), k0 = LfsAt(0x0004430C);
    f64 l = Mul28(dy, dy);
    l = Madd28(dx, dx, l);
    const f64 by = LfsAt(B + 4), bx = LfsAt(B), bz = LfsAt(B + 8);
    f64 qb = Mul28(LfsAt(q + 4), by);
    l = Madd28(dz, dz, l);
    qb = Madd28(LfsAt(q), bx, qb);
    l = static_cast<f32>(std::sqrt(l));
    qb = Madd28(LfsAt(q + 8), bz, qb);
    const f64 inv = static_cast<f32>(k1 / l);
    const f64 base = Madd28(l, LfsAt(0x00F95580), k0);
    const f64 t2 = Sub28(k1, qb);
    const f64 ny = Mul28(inv, dy), nx = Mul28(inv, dx), nz = Mul28(inv, dz);
    f64 d = Mul28(by, ny);
    d = Madd28(bx, nx, d);
    d = Madd28(bz, nz, d);
    f64 r = Sub28(k1, d);
    r = Madd28(r, LfsAt(0x00F955CC), base);
    ctx.fpr[1] = Madd28(t2, LfsAt(0x00F95618), r);
    ctx.gpr[3] = 0x00F955D0;  // r3 = the last weight's base, as the original leaves it
}

// void f(): the objects 0x09E8933B and 0xA8F58862 (Obj_FindById), when
// found with bit 0x00400000 of +192, get vt[304](obj, 0)
DECOMP(000496CC, Obj_ResetPair_000496CC) {
    for (u32 id : {0x09E8933Bu, 0xA8F58862u}) {
        const u32 r = static_cast<u32>(call(ctx, 0x000D6A28, {id}));
        if (r != 0 && (load<u32>(r + 192) & 0x00400000u) != 0) VCall(ctx, r, 304, {r, 0});
    }
}

// void f(T* o): +340..+356 = 0, the f32 +328 = a + (b - a) * (rand *
// *0x00061DC0) with a = *0x00F9B860, b = *0x00F9B8AC and rand the last
// value of the generator 0x01164888 (stepped here, inlined), then
// 0x0003C090(o)
DECOMP(00061DC8, Obj_ResetTimer_00061DC8) {
    const u32 o = arg<u32>(ctx, 0);
    const u64 old = load<u64>(0x01164888);
    const f64 k = LfsAt(0x00061DC0);
    const f64 a = LfsAt(0x00F9B860), b = LfsAt(0x00F9B8AC);
    const u32 s = load<u32>(0x01164890);
    const f64 span = Sub28(b, a);
    const u32 x = (std::rotl(s, 4) + std::rotl(s, 16)) ^ 0x87654321u;
    store<u32>(o + 340, 0);
    store<u32>(o + 348, 0);
    store<u32>(o + 344, 0);
    const f64 rnd = static_cast<f32>(static_cast<f64>(static_cast<s64>(old)));
    store<u64>(0x01164888, x & 0xFFFFu);
    store<u32>(o + 352, 0);
    store<u32>(o + 356, 0);
    store<u32>(0x01164890, std::rotl(x, 16));
    const f32 v = Madd28(span, Mul28(rnd, k), a);
    store<f32>(o + 328, v);
    ctx.fpr[1] = v;
    ctx.gpr[3] = call(ctx, 0x0003C090, {o, arg<u32>(ctx, 1)});
}

// T* f(?, XmlCursor* x, u8 flag): when x has the attribute 0x00AC019C,
// p = SmallAlloc(452) built by 0x00062824(p, attr, 0, 0), its u8 +16 =
// flag (written even when the alloc failed, as the original does) and
// 0x000632A8(x, p); returns p. Without it, XmlCursor_DocPath(arg0) and 0.
DECOMP(00063D54, Obj_NewFromXml_00063D54) {
    const u32 a0 = arg<u32>(ctx, 0), x = arg<u32>(ctx, 1), flag = arg<u32>(ctx, 2);
    const u32 at = static_cast<u32>(call(ctx, 0x004F9628, {x, 0x00AC019C}));
    if (at == 0) {
        call(ctx, 0x004F8944, {a0});
        ctx.gpr[3] = 0;
        return;
    }
    const u32 p = static_cast<u32>(call(ctx, o19fn::SmallAlloc, {452}));
    if (p != 0) call(ctx, 0x00062824, {p, at, 0, 0});
    store<u8>(p + 16, static_cast<u8>(flag));
    call(ctx, 0x000632A8, {x, p});
    ctx.gpr[3] = p;
}

// T* f(?, XmlCursor* x): the 0x00B103F0 record with the hash 0x8A93973F
// (+12) and the f32 under the same hash (+16, default 1.0f)
DECOMP(00089C1C, VtB103F0_NewXml_00089C1C) {
    NewXml30(ctx, 20, o28fn::Obj_InitVt_B21C98, true, [&](u32 p, u32 x) {
        store<u32>(p + 12, 0);
        store<u32>(p, 0x00B103F0);
        store<u32>(p + 16, 0x3F800000u);
        call(ctx, o28fn::XmlCursor_ReadHash, {x, 0x8A93973Fu, p + 12, 0});
        callf(ctx, o28fn::XmlCursor_ReadF32, {x, 0x8A93973Fu, p + 16}, {LfsAt(p + 16)});
    });
}

// T* f(?, XmlCursor* x): the 0x00B10418 record with the hashes 0x8A93973F
// (+12) and 0x922D3BD1 (+16, default current)
DECOMP(00089DE0, VtB10418_NewXml_00089DE0) {
    NewXml30(ctx, 20, o28fn::Obj_InitVt_B21C98, true, [&](u32 p, u32 x) {
        store<u32>(p + 12, 0);
        store<u32>(p + 16, 0);
        store<u32>(p, 0x00B10418);
        call(ctx, o28fn::XmlCursor_ReadHash, {x, 0x8A93973Fu, p + 12, 0});
        call(ctx, o28fn::XmlCursor_ReadHash, {x, 0x922D3BD1u, p + 16, load<u32>(p + 16)});
    });
}

// void f(Cmd* o, Owner* x): with the u64 mask o->+24: the target (the +280
// of Effect_Bind4BB7E0(x, o->+12, 0) when its +276 is 0x02235C33, else
// Obj_FindById(o->+16) when that is set) gets the mask set (u8 o->+32) or
// cleared in its u64 +184
DECOMP(0008A500, Cmd_SetTargetBits_0008A500) {
    const u32 o = arg<u32>(ctx, 0), x = arg<u32>(ctx, 1);
    ctx.gpr[3] = x;
    if (load<u64>(o + 24) == 0) return;
    u32 t = 0;
    const u32 h = load<u32>(o + 12);
    if (h != 0) {
        const u32 e = static_cast<u32>(call(ctx, 0x004BB7E0, {x, h, 0}));
        if (e != 0 && load<u32>(e + 276) == 0x02235C33u) t = load<u32>(e + 280);
    }
    const u32 id = load<u32>(o + 16);
    ctx.gpr[3] = id;
    if (id != 0 && t == 0) t = static_cast<u32>(call(ctx, 0x000D6A28, {id}));
    if (t == 0) return;
    const u32 on = load<u8>(o + 32);
    const u64 m = load<u64>(o + 24);
    const u64 v = on != 0 ? (load<u64>(t + 184) | m) : (load<u64>(t + 184) & ~m);
    store<u64>(t + 184, v);
    ctx.gpr[3] = v;
}

// T* f(?, XmlCursor* x): the 0x00B105D0 record with the hashes 0x8CFF50C1
// (+12) and 0x8CFF50C2 (+16)
DECOMP(0008B838, VtB105D0_NewXml_0008B838) {
    NewXml30(ctx, 20, o28fn::Obj_InitVt_B21C98, true, [&](u32 p, u32 x) {
        store<u32>(p + 16, 0);
        store<u32>(p + 12, 0);
        store<u32>(p, 0x00B105D0);
        call(ctx, o28fn::XmlCursor_ReadHash, {x, 0x8CFF50C1u, p + 12, 0});
        call(ctx, o28fn::XmlCursor_ReadHash, {x, 0x8CFF50C2u, p + 16, 0});
    });
}

// T* f(?, XmlCursor* x): the 0x00B10A08 record with the hashes 0x8A93973F
// (+12) and 0x9178C4CB (+16, default current)
DECOMP(00092464, VtB10A08_NewXml_00092464) {
    NewXml30(ctx, 20, o28fn::Obj_InitVt_B21C98, true, [&](u32 p, u32 x) {
        store<u32>(p + 12, 0);
        store<u32>(p + 16, 0);
        store<u32>(p, 0x00B10A08);
        call(ctx, o28fn::XmlCursor_ReadHash, {x, 0x8A93973Fu, p + 12, 0});
        call(ctx, o28fn::XmlCursor_ReadHash, {x, 0x9178C4CBu, p + 16, load<u32>(p + 16)});
    });
}

// void f(Cmd* o): with a name o->+16, the entry for it in the point map
// (*0x00F8F154)->+16 (warning Hash_ToString(name) when missing); when
// found, CmdList_Push(VtB20D70_Ctor(SmallAlloc(24), o->+12, entry, u8
// o->+20)) (CmdList_Push(0) when the alloc fails)
DECOMP(0009B8D4, Cmd_PushPoint_0009B8D4) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 name = load<u32>(o + 16);
    if (name == 0) return;
    u32 e;
    {
        Stack frame(ctx, 16);
        store<u32>(frame.addr(), name);
        e = load<u32>(static_cast<u32>(call(ctx, 0x0073343C, {load<u32>(load<u32>(0x00F8F154) + 16), frame.addr()})));
    }
    if (e == 0) {
        call(ctx, o27fn::Hash_ToString, {load<u32>(o + 16)});
        return;
    }
    const u32 p = static_cast<u32>(call(ctx, o19fn::SmallAlloc, {24}));
    if (p != 0) call(ctx, 0x0042D8BC, {p, load<u32>(o + 12), e, load<u8>(o + 20)});
    ctx.gpr[3] = call(ctx, o30fn::CmdList_Push, {p});
}

// T* f(?, XmlCursor* x): the 0x00B11878 record with the hash 0x7A30D7E3
// (+12) and the s32 0x39908DCA (+16, default 1)
DECOMP(000A4E04, VtB11878_NewXml_000A4E04) {
    NewXml30(ctx, 20, o28fn::Obj_InitVt_B21C98, true, [&](u32 p, u32 x) {
        store<u32>(p + 12, 0);
        store<u32>(p, 0x00B11878);
        store<u32>(p + 16, 1);
        call(ctx, o28fn::XmlCursor_ReadHash, {x, 0x7A30D7E3, p + 12, 0});
        const s64 d = static_cast<s32>(load<u32>(p + 16));
        call(ctx, 0x004F99BC, {x, 0x39908DCA, p + 16, static_cast<u64>(d)});
    });
}

// T* f(?, XmlCursor* x): the 16-byte 0x00B118A0 record with the bools
// 0x469D7C52 (u8 +12) and 0x8488565B (u8 +13, default 1)
DECOMP(000A4F9C, VtB118A0_NewXml_000A4F9C) {
    NewXml30(ctx, 16, o28fn::Obj_InitVt_B21C98, true, [&](u32 p, u32 x) {
        store<u8>(p + 12, 0);
        store<u32>(p, 0x00B118A0);
        store<u8>(p + 13, 1);
        call(ctx, o28fn::XmlCursor_ReadBool, {x, 0x469D7C52, p + 12, 0});
        call(ctx, o28fn::XmlCursor_ReadBool, {x, 0x8488565Bu, p + 13, load<u8>(p + 13)});
    });
}

// void f(Cmd* o): when *o->+20 is set, CmdList_Push(VtB212C8_Ctor(
// SmallAlloc(20), o->+20, u8 o->+24, u8 o->+25)) (0 when the alloc
// fails); else the warning String_Format(0x00AC59D4, Hash_ToString(o->+12),
// Hash_ToString(o->+16))
DECOMP(000A571C, Cmd_PushRef_000A571C) {
    const u32 o = arg<u32>(ctx, 0);
    if (static_cast<u32>(call(ctx, 0x00446960, {load<u32>(o + 20)})) == 0) {
        const u32 a = static_cast<u32>(call(ctx, o27fn::Hash_ToString, {load<u32>(o + 12)}));
        const u32 b = static_cast<u32>(call(ctx, o27fn::Hash_ToString, {load<u32>(o + 16)}));
        ctx.gpr[3] = call(ctx, o27fn::String_Format, {0x00AC59D4, a, b});
        return;
    }
    const u32 p = static_cast<u32>(call(ctx, o19fn::SmallAlloc, {20}));
    if (p != 0) call(ctx, 0x00438B84, {p, load<u32>(o + 20), load<u8>(o + 24), load<u8>(o + 25)});
    ctx.gpr[3] = call(ctx, o30fn::CmdList_Push, {p});
}

// void f(T* o, u32 key, u8 b): every element e of the u16-counted vector
// o->+248 with e->+12 == key gets e->vt[44](e, b) (r3 = the vector)
DECOMP(000D90EC, Obj_NotifyKeyed_000D90EC) {
    const u32 o = arg<u32>(ctx, 0), key = arg<u32>(ctx, 1), b = arg<u32>(ctx, 2) & 0xFFu;
    for (u32 i = 0;; ++i) {
        const u32 v = load<u32>(o + 248);
        ctx.gpr[3] = v;
        const u32 n = v != 0 ? load<u16>(v + 8) : 0u;
        if (!(i < n)) return;
        const u32 e = load<u32>(load<u32>(v) + i * 4);
        if (load<u32>(e + 12) == key) VCall(ctx, e, 44, {e, b});
    }
}

// void f(u32* out, T* o, u32 h): *out = the first of o's 12-byte slots
// +256 (count bits 0x00380000 of +172) whose handle resolves
// (Handle_Get_0044CE10) to h, else 0
DECOMP(000DBC28, Obj_FindSlotByHandle_000DBC28) {
    const u32 out = arg<u32>(ctx, 0), o = arg<u32>(ctx, 1), h = arg<u32>(ctx, 2);
    u32 n = (load<u32>(o + 172) >> 19) & 7u;
    ctx.gpr[3] = n;
    for (u32 i = 0; i < n; n = (load<u32>(o + 172) >> 19) & 7u, ctx.gpr[3] = n) {
        const u32 slot = o + 256 + 12 * i;
        if (static_cast<u32>(call(ctx, 0x0044CE10, {load<u32>(slot)})) == h) {
            const u32 w = load<u32>(slot);
            store<u32>(out, w);
            ctx.gpr[3] = w;
            return;
        }
        ++i;
    }
    store<u32>(out, 0);
}

// u32 f(T* o, u32 h): the choice v picked by Choice_Pick_00288974(c, &v,
// h) from c = CallK2_000D9424(o, h, 0), else Call_34A398_Arg1(o, h); when
// c is set but nothing was found, the warning Hash_ToString(h) and
// o->vt[76](o). Returns v.
DECOMP(001287F8, Obj_PickChoice_001287F8) {
    const u32 o = arg<u32>(ctx, 0), h = arg<u32>(ctx, 1);
    Stack frame(ctx, 16);
    const u32 vp = frame.addr();
    store<u32>(vp, 0);
    const u32 c = static_cast<u32>(call(ctx, 0x000D9424, {o, h, 0}));
    if (c != 0) call(ctx, 0x00288974, {c, vp, h});
    u32 v = load<u32>(vp);
    if (v == 0) {
        v = static_cast<u32>(call(ctx, 0x000DBCF8, {o, h}));
        store<u32>(vp, v);
    }
    if (c != 0 && v == 0) {
        call(ctx, o27fn::Hash_ToString, {h});
        VCall(ctx, o, 76, {o});
        v = load<u32>(vp);
    }
    ctx.gpr[3] = v;
}

// bool f(T* o, ?, a, b; f32 x): 1 when x <= k (k = *0x001690F0); with the
// health o->+348 above k, 0x000E1738(o, a, a, b; x) and, once it is
// exactly k, o->vt[396](o, 7, 0); returns whether it is still above k
DECOMP(001690F8, Obj_TakeDamage_001690F8) {
    const u32 o = arg<u32>(ctx, 0), a = arg<u32>(ctx, 2), b = arg<u32>(ctx, 3);
    const f64 x = farg(ctx, 0);
    const f64 k = LfsAt(0x001690F0);
    if (x < k || x == k) {
        ctx.gpr[3] = 1;
        return;
    }
    f64 h = LfsAt(o + 348);
    if (h > k) {
        call(ctx, 0x000E1738, {o, a, a, b});
        h = LfsAt(o + 348);
        if (h == k) {
            VCall(ctx, o, 396, {o, 7, 0});
            h = LfsAt(o + 348);
        }
    }
    ctx.gpr[3] = h > k ? 1u : 0u;
}

// void f(T* o, u8 a, u8 b, u8 c, u8 d): 0x000D9F1C(o, a, b, c, d), then
// each 96-byte record's object (+80 of o->+388, u16 count +392) gets
// vt[40](obj, a, b) (r3 = the count)
DECOMP(001BD1E0, Obj_EnableParts_001BD1E0) {
    const u32 o = arg<u32>(ctx, 0), a = arg<u32>(ctx, 1) & 0xFFu, b = arg<u32>(ctx, 2) & 0xFFu;
    call(ctx, 0x000D9F1C, {o, a, b, arg<u32>(ctx, 3) & 0xFFu, arg<u32>(ctx, 4) & 0xFFu});
    ctx.gpr[3] = load<u16>(o + 392);
    if (ctx.gpr[3] == 0) return;
    for (u32 i = 0; i < load<u16>(o + 392); ++i) {
        const u32 e = load<u32>(load<u32>(o + 388) + 96 * i + 80);
        VCall(ctx, e, 40, {e, a, b});
        ctx.gpr[3] = load<u16>(o + 392);
    }
}

// void f(T* o, const Plane* x): the 96-byte record o->+388[i] (i =
// trunc(x->+36 * count + *0x001BD368), capped at the u16 count +392):
// its point +64 moves by the component of x->+16 off its axis +16 (p +=
// v - axis * dot(axis, v))
DECOMP(001BD370, Obj_ProjectPart_001BD370) {
    const u32 o = arg<u32>(ctx, 0), x = arg<u32>(ctx, 1);
    const u32 n = load<u16>(o + 392);
    const f64 vx = LfsAt(x + 16), vy = LfsAt(x + 20), vz = LfsAt(x + 24);
    const f64 fn = static_cast<f32>(static_cast<f64>(n));
    const s32 i = static_cast<s32>(rt::fctiw(Madd28(LfsAt(x + 36), fn, LfsAt(0x001BD368)), true));
    const u32 idx = i < static_cast<s32>(n) ? static_cast<u32>(i) : n;
    const u32 rec = load<u32>(o + 388) + idx * 96u;
    const u32 A = rec + 16, B = rec + 64;
    const f64 ay = LfsAt(A + 4), ax = LfsAt(A), az = LfsAt(A + 8);
    f64 d = Mul28(ay, vy);
    d = Madd28(ax, vx, d);
    d = Madd28(az, vz, d);
    const f32 rx = Add28(LfsAt(B), Nmsub29(ax, d, vx));
    const f32 ry = Add28(LfsAt(B + 4), Nmsub29(ay, d, vy));
    const f32 rz = Add28(LfsAt(B + 8), Nmsub29(az, d, vz));
    store<f32>(B, rx);
    store<f32>(B + 4, ry);
    store<f32>(B + 8, rz);
    ctx.fpr[1] = rz;
    ctx.gpr[3] = B;
}

// void f(T* o, u8 b): every element e of the u16-counted list o->+928
// gets vt[40](e, b, 0); for b, the +44 and +92 of (o->+948)->+128 (when
// both are set) = *0x01054268
DECOMP(001EFA5C, Obj_EnableList_001EFA5C) {
    const u32 o = arg<u32>(ctx, 0), b = arg<u32>(ctx, 1) & 0xFFu;
    for (u32 i = 0;; ++i) {
        const u32 n = load<u16>(o + 936);
        ctx.gpr[3] = n;
        if (!(i < n)) break;
        const u32 e = load<u32>(load<u32>(o + 928) + 4 * i);
        VCall(ctx, e, 40, {e, b, 0});
    }
    if (b == 0) return;
    const u32 p = load<u32>(o + 948);
    if (p == 0) return;
    const u32 q = load<u32>(p + 128);
    if (q == 0) return;
    const u32 w = load<u32>(0x01054268);
    store<u32>(q + 44, w);
    store<u32>(q + 92, w);
    ctx.gpr[3] = 0x01054220;
    ctx.fpr[1] = F32Bits28(w);
}

// void f(T* o, const Mat34* m): 0x0020D7E8(o, m); with a body o->+436, a
// stack copy of m goes through Volume_GetTransform_00224170(o, &copy) and
// Obj_SetMatrix_004ACCD0(body, &copy)
DECOMP(00224D50, Obj_SetTransform_00224D50) {
    const u32 o = arg<u32>(ctx, 0), m = arg<u32>(ctx, 1);
    call(ctx, 0x0020D7E8, {o, m});
    ctx.gpr[3] = load<u32>(o + 436);
    if (ctx.gpr[3] == 0) return;
    Stack frame(ctx, 96);
    const u32 buf = frame.addr();
    for (u32 r = 0; r < 4; ++r)
        for (u32 k = 0; k < 3; ++k) Cp(buf + 16 * r + 4 * k, m + 16 * r + 4 * k);
    ctx.fpr[1] = LfsAt(m + 52);  // f1 = the last row's y, as the copy leaves it
    call(ctx, 0x00224170, {o, buf});
    ctx.gpr[3] = call(ctx, 0x004ACCD0, {load<u32>(o + 436), buf});
}

// void f(T* o, u32 b): for b == 0, the children reset of 0x00316548
// without the busy flag
DECOMP(0031ADD4, Obj_ResetChildren_0031ADD4) {
    const u32 o = arg<u32>(ctx, 0), b = arg<u32>(ctx, 1);
    if (b != 0) return;
    const u32 p = load<u32>(o + 32);
    for (u32 k = 0; k < 3; ++k) {
        const u32 slot = p + 776 + 4 * k;
        if (load<u32>(slot) == 0) continue;
        call(ctx, 0x001F4460, {load<u32>(slot)});
        const u32 c = load<u32>(slot);
        VCall(ctx, c, 88, {c, 1, 0, 0, 0});
    }
    ctx.gpr[3] = call(ctx, 0x004A090C, {p + 640, 0});
}

// bool f(T* o, Out* out): in state 4 (u8 +18), 1 when some state-4
// element p of the chain o->+2576 (next +2564) answers p->vt[64](p, out);
// else with o->+2204 set, out->+0 = it, the u8 out->+4 = o->+2558 and 1;
// otherwise 0
DECOMP(00327E0C, Obj_QueryChain_00327E0C) {
    const u32 o = arg<u32>(ctx, 0), out = arg<u32>(ctx, 1);
    ctx.gpr[3] = 0;
    if (load<u8>(o + 18) != 4) return;
    for (u32 p = load<u32>(o + 2576); p != 0; p = load<u32>(p + 2564)) {
        if (load<u8>(p + 18) != 4) continue;
        if (static_cast<u32>(VCall(ctx, p, 64, {p, out})) != 0) {
            ctx.gpr[3] = 1;
            return;
        }
    }
    const u32 v = load<u32>(o + 2204);
    if (v != 0) {
        const u32 b = load<u8>(o + 2558);
        store<u32>(out, v);
        store<u8>(out + 4, static_cast<u8>(b));
    }
    ctx.gpr[3] = v != 0 ? 1u : 0u;
}

// void f(T* o): 0x00325CE4(o); in state 4 (u8 +18), o->vt[28](o, 0) and
// every 24-byte link of o->+2588 (u16 count +2596) with an id +0 but no
// object +4 gets +4 = Obj_FindById(id) (r3 = the count)
DECOMP(0032D688, Obj_ResolveLinks_0032D688) {
    const u32 o = arg<u32>(ctx, 0);
    call(ctx, 0x00325CE4, {o, arg<u32>(ctx, 1)});
    ctx.gpr[3] = load<u8>(o + 18);
    if (ctx.gpr[3] != 4) return;
    VCall(ctx, o, 28, {o, 0});
    for (u32 i = 0;; ++i) {
        const u32 n = load<u16>(o + 2596);
        ctx.gpr[3] = n;
        if (!(i < n)) return;
        const u32 e = load<u32>(o + 2588) + 24 * i;
        if (load<u32>(e + 4) != 0 || load<u32>(e) == 0) continue;
        store<u32>(e + 4, static_cast<u32>(call(ctx, 0x000D6A28, {load<u32>(e)})));
    }
}

// void f(bool* out, T* p, a, b): q = p - 8; *out = 0 when q's filter
// q->+256 (an OPD, called (a, b)) rejects, else the bool 0x00885750(&r, q,
// a, b) leaves in r
DECOMP(00342920, Obj_FilterTest_00342920) {
    const u32 out = arg<u32>(ctx, 0), q = arg<u32>(ctx, 1) - 8, a = arg<u32>(ctx, 2), b = arg<u32>(ctx, 3);
    const u32 fn = load<u32>(q + 256);
    u32 r = 0;
    if (fn != 0) {
        if (static_cast<u32>(call_ptr(ctx, fn, {a, b, fn, load<u32>(fn)})) == 0) {
            store<u8>(out, 0);
            return;
        }
    }
    {
        Stack frame(ctx, 16);
        call(ctx, 0x00885750, {frame.addr(), q, a, b});
        r = load<u8>(frame.addr());
    }
    store<u8>(out, static_cast<u8>(r));
}

// s32 f(T* o, a, s32 n): when the u8 o->vt[16]() returns (in a stack
// buffer) is set, n >= 1 and o->+8 is set, r = (o->+8)->vt[12](c, a, n, 0);
// a negative r clears the u8 o->+24 and gives 0
DECOMP(00347B2C, Obj_Feed_00347B2C) {
    const u32 o = arg<u32>(ctx, 0), a = arg<u32>(ctx, 1);
    const s32 n = arg<s32>(ctx, 2);
    u32 ok;
    {
        Stack frame(ctx, 16);
        VCall(ctx, o, 16, {frame.addr(), o});
        ok = load<u8>(frame.addr());
    }
    ctx.gpr[3] = 0;
    if (ok == 0 || n < 1) return;
    const u32 c = load<u32>(o + 8);
    if (c == 0) return;
    const s32 r = static_cast<s32>(VCall(ctx, c, 12, {c, a, static_cast<u32>(n), 0}));
    if (r < 0) {
        store<u8>(o + 24, 0);
        ctx.gpr[3] = 0;
    }
}

// s32 f(FiosFile* h): frees the buffer h->+4 (fios_Free on the
// scheduler) and closes the file h->+0 (fios_CloseFile with a cleared
// 40-byte op record, callback word 0x00F39B20); -5 when the close returns
// 0, else 0 (and 0 without a file)
DECOMP(00450670, Fios_Close_00450670) {
    const u32 h = arg<u32>(ctx, 0);
    const u32 buf = load<u32>(h + 4);
    if (buf != 0) {
        const u32 s = static_cast<u32>(call(ctx, 0x005FC90C, {buf}));
        call(ctx, 0x005FE784, {s, load<u32>(h + 4)});
        store<u32>(h + 4, 0);
    }
    if (load<u32>(h) == 0) {
        ctx.gpr[3] = 0;
        return;
    }
    s32 rc = 0;
    {
        Stack frame(ctx, 48);
        const u32 op = frame.addr();
        store<u64>(op, 0);
        store<u8>(op + 8, 0);
        for (u32 k = 16; k <= 36; k += 4) store<u32>(op + k, 0);
        store<u32>(op + 12, 0x00F39B20);
        const u32 s = static_cast<u32>(call(ctx, 0x005FC90C, {0}));
        if (static_cast<u32>(call(ctx, 0x006001D4, {s, op, load<u32>(h)})) == 0) rc = -5;
    }
    store<u32>(h, 0);
    ret(ctx, rc);
}

// void f(World* w, u32 i, ?, u32 b, u32 a): removes every 12-byte entry
// {-, b, a} from the u16-counted list w + 20 * i + 0x128480 by moving the
// last entry into its place (r3 = w)
DECOMP(0046B7A4, World_ListRemove_0046B7A4) {
    const u32 w = arg<u32>(ctx, 0), i = arg<u32>(ctx, 1), b = arg<u32>(ctx, 3), a = arg<u32>(ctx, 4);
    const u32 list = w + 20 * i + 0x128480;
    for (s32 k = 0, off = 0;; ++k, off += 12) {
        const u32 n = load<u16>(list + 8);
        if (!(k < static_cast<s32>(n))) return;
        const u32 data = load<u32>(list);
        const u32 e = data + static_cast<u32>(off);
        if (load<u32>(e + 8) != a || load<u32>(e + 4) != b) continue;
        const u32 m = (n - 1) & 0xFFFFu;
        store<u16>(list + 8, static_cast<u16>(n - 1));
        const s32 kk = k;
        const u32 dst = data + static_cast<u32>(off);
        --k;
        off -= 12;
        if (kk == static_cast<s32>(m)) continue;
        const u32 src = data + 12 * m;
        Cp(dst, src);
        Cp(dst + 4, src + 4);
        Cp(dst + 8, src + 8);
    }
}

// void f(FiosStream* o): unless the handle o->+8 is -1, closes it
// (0x005F98B8(sched->+352 or 0, h)); a non-null result goes to
// 0x004911C0, else the handle becomes -1 and the owner o->+12 gets vt[4]()
// and is cleared; then the u8 o->+4 = 0 (r3 = 0; -1 for no handle)
DECOMP(004915A4, FiosStream_Close_004915A4) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 s = static_cast<u32>(call(ctx, 0x005FC90C, {o}));
    const u32 fd = load<u32>(o + 8);
    const u32 q = s != 0 ? load<u32>(s + 352) : 0u;
    ctx.gpr[3] = fd;
    if (fd == 0xFFFFFFFFu) return;
    const s64 sfd = static_cast<s32>(fd);
    const u32 r = static_cast<u32>(call(ctx, 0x005F98B8, {q, static_cast<u64>(sfd)}));
    if (r != 0) {
        call(ctx, 0x004911C0, {r});
    } else {
        const u32 c = load<u32>(o + 12);
        store<u32>(o + 8, 0xFFFFFFFFu);
        if (c != 0) {
            VCall(ctx, c, 4, {c});
            store<u32>(o + 12, 0);
        }
    }
    store<u8>(o + 4, 0);
    ctx.gpr[3] = 0;
}

// bool f(const Mat34* a, const Mat34* b): the four rows' first 12 bytes
// match bit for bit
DECOMP(007343A0, Mat34_Equal_007343A0) {
    const u32 a = arg<u32>(ctx, 0), b = arg<u32>(ctx, 1);
    bool eq = true;
    for (u32 r = 0; r < 4; ++r)
        if (load<u64>(a + 16 * r) != load<u64>(b + 16 * r) || load<u32>(a + 16 * r + 8) != load<u32>(b + 16 * r + 8)) eq = false;
    ctx.gpr[3] = eq ? 1u : 0u;
}

// ---------------------------------------------------------------- sweep: 53 instructions
// void f(T* o, u32 id; f32 v): for 0xCC65890A, 0x0005E990(o, 0x6021F400 +
// (state o->+308 == 1 ? 0 : == 2 ? 1 : 2); *0x00F9C5C0) first; then the
// u8 +513 = 0, +432 = id, +436 = v and the u8 +440 = 1 (r3 = 0)
DECOMP(0005FA08, Obj_SetPending_0005FA08) {
    const u32 o = arg<u32>(ctx, 0), id = arg<u32>(ctx, 1);
    const f64 v = farg(ctx, 0);
    if (id == 0xCC65890Au) {
        const u32 s = load<u32>(o + 308);
        const u32 h = s == 1 ? 0x6021F400u : s == 2 ? 0x6021F401u : 0x6021F402u;
        callf(ctx, 0x0005E990, {o, h}, {LfsAt(0x00F9C5C0)});
    }
    store<u8>(o + 513, 0);
    store<u32>(o + 432, id);
    store<f32>(o + 436, static_cast<f32>(v));
    store<u8>(o + 440, 1);
    ctx.gpr[3] = 0;
}

// void f(Cmd* o, Owner* x): the id o->+12, or (with a component name
// o->+16) the vt[24]() of Effect_FindComponent(x, name) when its vt[32]()
// is 2; when set, Loader_StartById_002E56F4(*0x00F64C20, id, 1, 1)
DECOMP(0006FF1C, Cmd_StartLoader_0006FF1C) {
    const u32 o = arg<u32>(ctx, 0), x = arg<u32>(ctx, 1);
    u32 id = load<u32>(o + 12);
    const u32 name = load<u32>(o + 16);
    ctx.gpr[3] = name;
    if (name != 0) {
        const u32 c = static_cast<u32>(call(ctx, 0x004BB4B4, {x, name}));
        if (c != 0 && static_cast<u32>(VCall(ctx, c, 32, {c})) == 2) id = static_cast<u32>(VCall(ctx, c, 24, {c}));
    }
    if (id == 0) return;
    ctx.gpr[3] = call(ctx, 0x002E56F4, {load<u32>(0x00F64C20), id, 1, 1});
}

// T* f(?, XmlCursor* x): the 0x00B0FA90 record with the s32 0xCDF92809
// (+12, default -1) and the hash 0xF12B8E48 (+16, default current)
DECOMP(0007C7F4, VtB0FA90_NewXml_0007C7F4) {
    NewXml30(ctx, 20, o28fn::Obj_InitVt_B21C98, true, [&](u32 p, u32 x) {
        store<u32>(p + 12, 0xFFFFFFFFu);
        store<u32>(p, 0x00B0FA90);
        store<u32>(p + 16, 0);
        call(ctx, 0x004F99BC, {x, 0xCDF92809u, p + 12, ~0ull});
        call(ctx, o28fn::XmlCursor_ReadHash, {x, 0xF12B8E48u, p + 16, load<u32>(p + 16)});
    });
}

// T* f(?, XmlCursor* x): the 0x00B0FFE0 record with the hashes 0x7A30D7E3
// (+12, default 0x0027469E) and 0x040EFE5F (+16, default current)
DECOMP(00084D10, VtB0FFE0_NewXml_00084D10) {
    NewXml30(ctx, 20, o28fn::Obj_InitVt_B21C98, true, [&](u32 p, u32 x) {
        store<u32>(p + 16, 0);
        store<u32>(p, 0x00B0FFE0);
        store<u32>(p + 12, 0x0027469E);
        call(ctx, o28fn::XmlCursor_ReadHash, {x, 0x7A30D7E3, p + 12, 0x0027469E});
        call(ctx, o28fn::XmlCursor_ReadHash, {x, 0x040EFE5F, p + 16, load<u32>(p + 16)});
    });
}

// T* f(?, XmlCursor* x): the 0x00B107B0 record with the s32 0xA212C671
// (+12, default -1) and the hash 0xB2E59A9F (+16)
DECOMP(0008D490, VtB107B0_NewXml_0008D490) {
    NewXml30(ctx, 20, o28fn::Obj_InitVt_B21C98, true, [&](u32 p, u32 x) {
        store<u32>(p + 12, 0xFFFFFFFFu);
        store<u32>(p, 0x00B107B0);
        store<u32>(p + 16, 0);
        call(ctx, 0x004F99BC, {x, 0xA212C671u, p + 12, ~0ull});
        call(ctx, o28fn::XmlCursor_ReadHash, {x, 0xB2E59A9Fu, p + 16, 0});
    });
}

// T* f(?, XmlCursor* x): the 0x00B10FB0 record with the f32 0x230A334D
// (+12, default *0x0009B1C8) and the bool 0x8488565B (u8 +16, default
// current)
DECOMP(0009B1D0, VtB10FB0_NewXml_0009B1D0) {
    NewXml30(ctx, 20, o28fn::Obj_InitVt_B21C98, true, [&](u32 p, u32 x) {
        store<u32>(p, 0x00B10FB0);
        const f64 d = LfsAt(0x0009B1C8);
        store<u32>(p + 12, 0);
        store<u8>(p + 16, 0);
        callf(ctx, o28fn::XmlCursor_ReadF32, {x, 0x230A334D, p + 12}, {d});
        call(ctx, o28fn::XmlCursor_ReadBool, {x, 0x8488565Bu, p + 16, load<u8>(p + 16)});
    });
}

// T* f(?, XmlCursor* x): the 0x00B113E8 record with the f32 0x00274ACD
// (+12, default *0x0009F7A8) and the hash 0xDC7AF11A (+16, default
// current)
DECOMP(0009F7B0, VtB113E8_NewXml_0009F7B0) {
    NewXml30(ctx, 20, o28fn::Obj_InitVt_B21C98, true, [&](u32 p, u32 x) {
        store<u32>(p, 0x00B113E8);
        const f64 d = LfsAt(0x0009F7A8);
        store<u32>(p + 12, 0);
        store<u32>(p + 16, 0);
        callf(ctx, o28fn::XmlCursor_ReadF32, {x, 0x00274ACD, p + 12}, {d});
        call(ctx, o28fn::XmlCursor_ReadHash, {x, 0xDC7AF11Au, p + 16, load<u32>(p + 16)});
    });
}

// void f(Vec3* out, T* o): out = o->+144, plus the half-height offset
// Vec3_SetHalfY84_002BC1E0(&t, component 31) when o has component 31
DECOMP(000D9E48, Obj_CenterPoint_000D9E48) {
    const u32 out = arg<u32>(ctx, 0), o = arg<u32>(ctx, 1);
    const u32 p = o + 144;
    if (static_cast<u32>(call(ctx, 0x000D1288, {o, 31, 0})) == 0) {
        Cp(out, p);
        Cp(out + 4, p + 4);
        ctx.fpr[1] = F32Bits28(Cp(out + 8, p + 8));
        return;
    }
    Stack frame(ctx, 16);
    const u32 t = frame.addr();
    const u32 c = static_cast<u32>(call(ctx, 0x000D1288, {o, 31, 0}));
    call(ctx, 0x002BC1E0, {t, c});
    const f32 x = Add28(LfsAt(p), LfsAt(t));
    const f32 y = Add28(LfsAt(p + 4), LfsAt(t + 4));
    const f32 z = Add28(LfsAt(p + 8), LfsAt(t + 8));
    store<f32>(out, x);
    store<f32>(out + 4, y);
    store<f32>(out + 8, z);
    ctx.fpr[1] = x;
}

// void f(T* o, ?, u8 b; f32 v): the f32 o->+212 = v,
// 0x002EF7B8(*0x00F64C38, o->+160); for b, itself on every element
// rec->+192 of the 240-byte records of the vector o->+340 (r3 = the vector)
DECOMP(000DAB90, Obj_SetTimeTree_000DAB90) {
    const u32 o = arg<u32>(ctx, 0), b = arg<u32>(ctx, 2) & 0xFFu;
    const f64 v = farg(ctx, 0);
    const u32 w = load<u32>(o + 160), g = load<u32>(0x00F64C38);
    store<f32>(o + 212, static_cast<f32>(v));
    ctx.gpr[3] = call(ctx, 0x002EF7B8, {g, w});
    if (b == 0) return;
    for (u32 i = 0, off = 0;; ++i, off += 240) {
        const u32 vec = load<u32>(o + 340);
        ctx.gpr[3] = vec;
        const u32 n = vec != 0 ? load<u16>(vec + 8) : 0u;
        if (!(i < n)) return;
        const u32 e = load<u32>(load<u32>(vec) + off + 192);
        if (e != 0) callf(ctx, 0x000DAB90, {e, n, b}, {v});
    }
}

// void f(T* o, a; f32 x, f32 y): lo / hi = min / max(x, y), each kept
// when at most *0x00101930 (else the double *0x00101938) and at least
// *0x00101940 (else the double *0x00101948); then 0x001007D0(&t,
// 0x01164B60, a; k - lo) and 0x001007D0(o, 0x01164B90, &t; hi + k) with
// k = *0x00101950
DECOMP(00101958, Obj_SetRange_00101958) {
    const u32 o = arg<u32>(ctx, 0), a = arg<u32>(ctx, 1);
    const f64 x = farg(ctx, 0), y = farg(ctx, 1);
    auto fsel = [](f64 c, f64 p, f64 q) { return c >= 0.0 ? p : q; };
    const f64 kmax = LfsAt(0x00101930), dmax = load<f64>(0x00101938);
    const f64 kmin = LfsAt(0x00101940), dmin = load<f64>(0x00101948);
    const f64 k = LfsAt(0x00101950);
    f64 lo = fsel(static_cast<f64>(Sub28(y, x)), x, y);
    f64 hi = fsel(static_cast<f64>(Sub28(x, y)), x, y);
    lo = static_cast<f32>(fsel(static_cast<f64>(Sub28(kmax, lo)), lo, dmax));
    hi = static_cast<f32>(fsel(static_cast<f64>(Sub28(kmax, hi)), hi, dmax));
    const f64 hi2 = static_cast<f32>(fsel(static_cast<f64>(Sub28(hi, kmin)), hi, dmin));
    const f64 lo2 = static_cast<f32>(fsel(static_cast<f64>(Sub28(lo, kmin)), lo, dmin));
    Stack frame(ctx, 32);
    const u32 t = frame.addr();
    callf(ctx, 0x001007D0, {t, 0x01164B60, a}, {static_cast<f64>(Sub28(k, lo2))});
    callf(ctx, 0x001007D0, {o, 0x01164B90, t}, {static_cast<f64>(Add28(hi2, k))});
}

// void f(Box* out, List* x): with the entry c = List_FindAfter_000CB888(x,
// 0, 0), out's corners from its center +128..+136 and half sizes
// +112..+120 (y: +132 - *0x0011F008 and +132 + +116 * *0x0011F00C);
// else 0x000CB934(out, x)
DECOMP(0011F010, Box_FromEntry_0011F010) {
    const u32 out = arg<u32>(ctx, 0), x = arg<u32>(ctx, 1);
    const u32 c = static_cast<u32>(call(ctx, 0x000CB888, {x, 0, 0}));
    if (c == 0) {
        ctx.gpr[3] = call(ctx, 0x000CB934, {out, x});
        return;
    }
    const f64 k0 = LfsAt(0x0011F008), k1 = LfsAt(0x0011F00C);
    const f64 hx = LfsAt(c + 112), cx = LfsAt(c + 128), hz = LfsAt(c + 120), cy = LfsAt(c + 132), cz = LfsAt(c + 136);
    const f64 hy = LfsAt(c + 116);
    store<f32>(out, Sub28(cx, hx));
    store<f32>(out + 16, Add28(cx, hx));
    store<f32>(out + 8, Sub28(cz, hz));
    store<f32>(out + 24, Add28(cz, hz));
    const f32 y0 = Sub28(cy, k0);
    store<f32>(out + 4, y0);
    store<f32>(out + 20, Madd28(hy, k1, cy));
    ctx.fpr[1] = y0;
}

// void f(T* o): o->vt[120](o, o + 560, -, 1; *0x001243C8) (the OPD in r5,
// the entry in r7); with component 23 of type 2..5 (bits 0x3E000000 of
// its +24), 0x00276558(c, 0), 0x001241A0(o, 255) and bit 0x08 of the u8
// o->+968; then +440 = 0 and bit 0x40 of the u64 +184 cleared (r3 = 0)
DECOMP(001243D0, Obj_Detach_001243D0) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 opd = load<u32>(load<u32>(o) + 120);
    ctx.fpr[1] = LfsAt(0x001243C8);
    call_ptr(ctx, opd, {o, o + 560, opd, 1, load<u32>(opd)});
    const u32 c = static_cast<u32>(call(ctx, 0x000D1288, {o, 23, 0}));
    if (c != 0) {
        const u32 t = (load<u32>(c + 24) >> 25) & 0x1Fu;
        if (t > 1 && t <= 5) {
            call(ctx, 0x00276558, {c, 0});
            call(ctx, 0x001241A0, {o, 255});
            store<u8>(o + 968, static_cast<u8>(load<u8>(o + 968) | 0x8u));
        }
    }
    const u64 d = load<u64>(o + 184) & ~0x40ull;
    store<u32>(o + 440, 0);
    store<u64>(o + 184, d);
    ctx.gpr[3] = 0;
}

// f32 f(T* o): the spawner's +224 (Spawner_Find0D155C(o->+336, o)) when
// o->+336 is set; else the high half of the u16 body->+510 as f32 bits
// for the body Obj_GetBody_00252940(o->vt[64]()) when both are set, or
// *0x00124BBC
DECOMP(00124BC0, Obj_GetSpeed_00124BC0) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 sp = load<u32>(o + 336);
    if (sp != 0) {
        const u32 s = static_cast<u32>(call(ctx, 0x000D155C, {sp, o}));
        ctx.fpr[1] = LfsAt(s + 224);
        return;
    }
    f64 r = LfsAt(0x00124BBC);
    if (static_cast<u32>(VCall(ctx, o, 64, {o})) != 0) {
        const u32 c = static_cast<u32>(VCall(ctx, o, 64, {o}));
        const u32 b = static_cast<u32>(call(ctx, 0x00252940, {c}));
        if (b != 0) {
            ctx.gpr[3] = static_cast<u32>(load<u16>(b + 510)) << 16;
            r = F32Bits28(static_cast<u32>(ctx.gpr[3]));
        }
    }
    ctx.fpr[1] = r;
}

// void f(T* o): o->+404 = (old p)->vt[56](p, 5) (p is not checked), the
// old p gets vt[8]() when set; the new one (when set) gets
// SplineInstance_SetMode(n, Obj_TypeIndex4A1244(n)), +280 = o and +276 =
// 0x02235C33
DECOMP(001BD540, Obj_SwapSpline_001BD540) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 p = load<u32>(o + 404);
    u32 n = static_cast<u32>(VCall(ctx, p, 56, {p, 5}));
    store<u32>(o + 404, n);
    if (p != 0) {
        VCall(ctx, p, 8, {p});
        n = load<u32>(o + 404);
    }
    ctx.gpr[3] = n;
    if (n == 0) return;
    const u32 t = static_cast<u32>(call(ctx, 0x004A1244, {n}));
    call(ctx, 0x005A7B38, {n, t & 0xFFu});
    const u32 q = load<u32>(o + 404);
    store<u32>(q + 280, o);
    store<u32>(q + 276, 0x02235C33u);
    ctx.gpr[3] = q;
}

// void f(T* a, U* b): Obj_CopyParams_001D4620's copy plus the bytes
// b->+248 / +249 = a->+280 / +281 (r3 = the last byte, f1 = a->+276)
DECOMP(001BF6C0, Obj_CopyParams_001BF6C0) {
    const u32 a = arg<u32>(ctx, 0), b = arg<u32>(ctx, 1);
    call(ctx, o30fn::Thunk_E44C0, {a, b});
    Cp(b + 236, a + 268);
    for (u32 k = 0; k < 7; ++k) Cp(b + 208 + 4 * k, a + 240 + 4 * k);
    store<u32>(b + 20, (load<u32>(b + 20) & 0x1FFFFu) | (4u << 17));
    Cp(b + 240, a + 272);
    ctx.fpr[1] = F32Bits28(Cp(b + 244, a + 276));
    store<u8>(b + 248, load<u8>(a + 280));
    const u32 c = load<u8>(a + 281);
    store<u8>(b + 249, static_cast<u8>(c));
    ctx.gpr[3] = c;
}

// bool f(T* o; f32 a, f32 b): 1 (no change) unless the gates below send it
// on to o->vt[420](o, 2, 0) and 0: with no +556 / +564 and +576 at most
// k (*0x001C7D58), or bit 0x20000000 of +580; then bit 0x20000000 of
// +552 clear and (b < k or a >= b)
DECOMP(001C7D60, Obj_RangeGate_001C7D60) {
    const u32 o = arg<u32>(ctx, 0);
    const f64 a = farg(ctx, 0), b = farg(ctx, 1);
    const f64 k = LfsAt(0x001C7D58);
    ctx.gpr[3] = 1;
    bool next = false;
    if (load<u32>(o + 556) == 0 && load<u32>(o + 564) == 0) {
        const f64 t = LfsAt(o + 576);
        if (t < k || t == k) next = true;
    }
    if (!next && (load<u32>(o + 580) & 0x20000000u) == 0) return;
    if ((load<u32>(o + 552) & 0x20000000u) != 0) return;
    if (!(b < k || a > b || a == b)) return;
    VCall(ctx, o, 420, {o, 2, 0});
    ctx.gpr[3] = 0;
}

// void f(T* o): resolves the links named by the definition q = o->+216:
// +848 = 0x000DA9D0(o, q->+400) (then +844 = Obj_Entry16_000DA954(o, the
// s16 (*e)->+100), or +832 = Obj_FindByKey_000D1454(o, q->+400) when
// that fails), +836 / +840 = Obj_FindByKey(o, q->+404 / q->+408)
DECOMP(001CAF1C, Obj_ResolveDefLinks_001CAF1C) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 q = load<u32>(o + 216);
    ctx.gpr[3] = load<u32>(q + 400);
    if (ctx.gpr[3] != 0) {
        const u32 e = static_cast<u32>(call(ctx, 0x000DA9D0, {o, load<u32>(q + 400)}));
        store<u32>(o + 848, e);
        if (e == 0) {
            store<u32>(o + 832, static_cast<u32>(call(ctx, 0x000D1454, {o, load<u32>(q + 400)})));
        } else {
            const s64 k = static_cast<s16>(load<u16>(load<u32>(e) + 100));
            store<u32>(o + 844, static_cast<u32>(call(ctx, 0x000DA954, {o, static_cast<u64>(k)})));
        }
    }
    ctx.gpr[3] = load<u32>(q + 404);
    if (ctx.gpr[3] != 0) store<u32>(o + 836, static_cast<u32>(call(ctx, 0x000D1454, {o, load<u32>(q + 404)})));
    const u32 k2 = load<u32>(q + 408);
    if (k2 != 0) store<u32>(o + 840, static_cast<u32>(call(ctx, 0x000D1454, {o, k2})));
}

// void f(T* o, Query* out): Obj_CopyQuery_000CDB28(o, out), then out->+144
// = o->+180 and the u8 flags out->+148..+153 = bits 0x80 / 0x40 / 0x20 /
// 0x10 / 0x08 / 0x02 of the u8 o->+184
DECOMP(0020D264, Obj_CopyQuery_0020D264) {
    const u32 o = arg<u32>(ctx, 0), out = arg<u32>(ctx, 1);
    call(ctx, 0x000CDB28, {o, out});
    store<u32>(out + 144, load<u32>(o + 180));
    u32 v = 0;
    const u32 bits[6] = {0x80u, 0x40u, 0x20u, 0x10u, 0x08u, 0x02u};
    for (u32 k = 0; k < 6; ++k) {
        v = (load<u8>(o + 184) & bits[k]) != 0 ? 1u : 0u;
        store<u8>(out + 148 + k, static_cast<u8>(v));
    }
    ctx.gpr[3] = v;
}

// u32 f(const Vec3* a, -, -, const Mat* m, bool* ok; f32 x, f32 y): *ok =
// x >= y; then the dominant of d0 = dot(a, m row 0) and d2 = dot(a, m row
// 2) picks the face: (d0 < k) * 2 + 1 or (d2 < k) * 2 (k = *0x0027C344;
// 0 when not ok)
DECOMP(0027C348, Box_PickFace_0027C348) {
    const u32 a = arg<u32>(ctx, 0), m = arg<u32>(ctx, 3), ok = arg<u32>(ctx, 4);
    const f64 x = farg(ctx, 0), y = farg(ctx, 1);
    const u32 okv = (x > y || x == y) ? 1u : 0u;
    store<u8>(ok, static_cast<u8>(okv));
    ctx.gpr[3] = 0;
    if (okv == 0) return;
    const u32 m2 = m + 32;
    const f64 ay = LfsAt(a + 4), ax = LfsAt(a), az = LfsAt(a + 8);
    f64 d0 = Mul28(ay, LfsAt(m + 4));
    f64 d2 = Mul28(ay, LfsAt(m2 + 4));
    d0 = Madd28(ax, LfsAt(m), d0);
    d2 = Madd28(ax, LfsAt(m2), d2);
    d0 = Madd28(az, LfsAt(m + 8), d0);
    d2 = Madd28(az, LfsAt(m2 + 8), d2);
    const f64 k = LfsAt(0x0027C344);
    if (std::fabs(d0) > std::fabs(d2)) {
        ctx.gpr[3] = ((d0 < k ? 1u : 0u) << 1) | 1u;
        ctx.fpr[1] = k;
        ctx.fpr[2] = d0;
    } else {
        ctx.gpr[3] = (d2 < k ? 1u : 0u) << 1;
        ctx.fpr[1] = d2;
        ctx.fpr[2] = k;
    }
    ctx.gpr[5] = m2;  // r5 = row 2, as the original leaves it
}

// bool f(const Filter* a, const Item* b): b matches a by id (b->+8 ==
// a->+24, b->+4 == a->+20 or b->+32 == a->+48, each when set), owner (when
// a->+52 and b->+40 are set, a->+52->+216->+52 == b->+40) and the four
// flag bits 0x08000000.. of b->+120 equal to 0x20000000.. of a->+72
DECOMP(0029998C, Filter_Match_0029998C) {
    const u32 a = arg<u32>(ctx, 0), b = arg<u32>(ctx, 1);
    const u32 i8 = load<u32>(b + 8), i4 = load<u32>(b + 4);
    ctx.gpr[3] = 0;
    bool hit = (i8 != 0 && i8 == load<u32>(a + 24)) || (i4 != 0 && i4 == load<u32>(a + 20));
    if (!hit) {
        const u32 i32 = load<u32>(b + 32);
        hit = i32 != 0 && i32 == load<u32>(a + 48);
    }
    if (!hit) return;
    const u32 ow = load<u32>(a + 52);
    if (ow != 0) {
        const u32 want = load<u32>(b + 40);
        if (want != 0) {
            const u32 d = load<u32>(ow + 216);
            if ((d != 0 ? load<u32>(d + 52) : 0u) != want) return;
        }
    }
    const u32 f = load<u32>(b + 120), g = load<u32>(a + 72);
    for (u32 k = 0; k < 4; ++k)
        if (((f >> (27 - k)) & 1u) != ((g >> (29 - k)) & 1u)) return;
    ctx.gpr[3] = 1;
}

// void f(T* o): counts the o->+24 860-byte players at o->+20 whose slot
// (u8 +822, not 255) has bit 0x40 in the u16 table of the current world
// record (+496); exactly one awards Trophy 0xC2900B80, six or more
// 0x368BAA71
DECOMP(002E619C, Players_CountTrophy_002E619C) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 w = load<u32>(kWorldPtr);
    const u32 rec = w + load<u8>(w + 4208) * 4056u;
    const u32 n = load<u32>(o + 24);
    if (n == 0) return;
    const u32 base = rec + 496;
    u32 p = load<u32>(o + 20), cnt = 0;
    for (u32 i = 0; i < n; ++i, p += 860) {
        if (p == 0) continue;
        const u32 b = load<u8>(p + 822);
        if (b == 255) continue;
        if ((load<u16>(base + b * 4) & 0x40u) != 0) ++cnt;
    }
    ctx.gpr[3] = p;
    if (cnt == 1) ctx.gpr[3] = call(ctx, 0x000B27B4, {0xC2900B80u});
    else if (cnt >= 6) ctx.gpr[3] = call(ctx, 0x000B27B4, {0x368BAA71u});
}

// void f(T* o): bits 0x80 / 0x40 of the u8 o->+28 cleared, then each of
// the o->+24 elements o->+16[i] and the owner o->+20 (when set) get
// vt[16]()
DECOMP(002E8CA0, Group_Reset_002E8CA0) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 b = load<u8>(o + 28) & ~0xC0u;
    const u32 n = load<u32>(o + 24);
    store<u8>(o + 28, static_cast<u8>(b));
    if (n != 0) {
        for (u32 i = 0; i < load<u32>(o + 24); ++i) {
            const u32 e = load<u32>(load<u32>(o + 16) + 4 * i);
            VCall(ctx, e, 16, {e});
            ctx.gpr[3] = load<u32>(o + 24);
        }
    }
    const u32 p = load<u32>(o + 20);
    if (p != 0) VCall(ctx, p, 16, {p});
}

// T::~T() (vtable 0x00B1D650, base 0x00B1D600): every element e of the
// u16-counted PtrVector +24 gets vt[40](e, 0), the count is cleared and
// the vector destroyed (r3 = the base vtable)
DECOMP(002FB4E0, VtB1D650_Dtor_002FB4E0) {
    const u32 o = arg<u32>(ctx, 0);
    store<u32>(o, 0x00B1D650);
    for (u32 i = 0; i < load<u16>(o + 32); ++i) {
        const u32 e = load<u32>(load<u32>(o + 24) + 4 * i);
        VCall(ctx, e, 40, {e, 0});
    }
    store<u16>(o + 32, 0);
    VecDtor(ctx, o + 24);
    store<u32>(o, 0x00B1D600);
    ctx.gpr[3] = 0x00B1D600;
}

// void f(T* o): 0x00325CE4(o); in state 4 (u8 +18), o->vt[28](o, 0) and,
// without a target +2600, the object Obj_FindById(o->+2592) becomes it
// when it has bit 0x2 of +192: +2604 = its +348 and its u16 +178 counts
// up
DECOMP(00332C84, Obj_AcquireTarget_00332C84) {
    const u32 o = arg<u32>(ctx, 0);
    call(ctx, 0x00325CE4, {o, arg<u32>(ctx, 1)});
    ctx.gpr[3] = load<u8>(o + 18);
    if (ctx.gpr[3] != 4) return;
    VCall(ctx, o, 28, {o, 0});
    ctx.gpr[3] = load<u32>(o + 2600);
    if (ctx.gpr[3] != 0) return;
    const u32 id = load<u32>(o + 2592);
    ctx.gpr[3] = id;
    if (id == 0) return;
    const u32 t = static_cast<u32>(call(ctx, 0x000D6A28, {id}));
    if (t == 0) return;
    const u32 r = (load<u32>(t + 192) & 2u) != 0 ? t : 0u;
    store<u32>(o + 2600, r);
    if (r == 0) return;
    const u32 w = load<u32>(r + 348);
    const u32 c = load<u16>(r + 178);
    store<u32>(o + 2604, w);
    store<u16>(r + 178, static_cast<u16>(c + 1));
    ctx.gpr[3] = r + 178;
    ctx.fpr[1] = F32Bits28(w);
}
