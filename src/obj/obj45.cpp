// Game code, part 45: the small-function sweep continued from the remaining
// 97 instruction functions (size order; obj44*.cpp did 93-97). Each was
// checked against the original in the PPC simulator (memory writes, calls
// and their arguments, r3 and f1).
#include "obj45.h"

using namespace sly4;
using namespace sly4::obj45;

// ---------------------------------------------------------------- sweep: 97 instructions
// bool f(T* o, f32 dt): StateMachine_Tick(o, dt) == 0 -> 0. Then every entry
// e of the array +248 (u16 count +8) with the byte e->+16 set gets
// e->vt[28](e, dt); when o->+348 has +8 set and its byte +232 is neither 4
// nor 5, o->vt[284](o). With t = o->+216->+188: t >= 0 -> +356 += dt, and
// r3 = 0 once +356 >= t; else 1. f1 = t.
DECOMP(000DEAE0, Obj_TickParts_000DEAE0) {
    const u32 o = arg<u32>(ctx, 0);
    const f64 dt = farg(ctx, 0);
    callf(ctx, 0x00464CBCu, {o}, {dt});
    if (static_cast<u32>(ctx.gpr[3]) == 0) {
        ret(ctx, 0);
        return;
    }
    for (u32 i = 0, off = 0;; ++i, off += 4) {
        const u32 arr = load<u32>(o + 248);
        const u32 n = arr != 0 ? load<u16>(arr + 8) : 0;
        if (!(i < n)) break;
        const u32 e = load<u32>(load<u32>(arr) + off);
        if (load<u8>(e + 16) == 0) continue;
        ctx.fpr[1] = dt;
        VCall(ctx, e, 28, {e});
    }
    const u32 s = load<u32>(o + 348);
    const u32 st = load<u32>(o + 216);
    if (s != 0 && load<u32>(s + 8) != 0) {
        const u8 b = load<u8>(s + 232);
        if (b != 5 && b != 4) VCall(ctx, o, 284, {o});
    }
    const f64 t = Lfs45(st + 188);
    ctx.fpr[1] = t;
    if (t >= Lfs45(0x000DEADCu)) {
        const f64 acc = F45(Lfs45(o + 356) + dt);
        store<u32>(o + 356, W45(acc));
        if (acc >= t) {
            ret(ctx, 0);
            return;
        }
    }
    ret(ctx, 1);
}

// f32 f(T* o, f32 a, f32 b, f32 t): one integration step of the point A
// (+16) with velocity B (+32) towards the target C (o->+64 when the byte
// +84 is set, else the vector 0x01164B40) and the point D (+48), weighted
// by the constants 0x000F8C64 (k0, k1, k2): ka = a * a * k0, kb = b * a *
// k1, s = k2 / (k2 + t * kb + t * t * ka); B += t * ((kb + t * ka) * s *
// (C - B) + ka * s * (D - A)); A += t * B. r3 = o, f1 = the new B.z.
DECOMP(000F8C70, Spring_Step_000F8C70) {
    const u32 o = arg<u32>(ctx, 0);
    const f64 a = farg(ctx, 0), b = farg(ctx, 1), t = farg(ctx, 2);
    const u32 c = load<u8>(o + 84) != 0 ? o + 64 : 0x01164B40u;
    const f64 ax = Lfs45(o + 16), ay = Lfs45(o + 20), az = Lfs45(o + 24);
    const f64 bx = Lfs45(o + 32), by = Lfs45(o + 36), bz = Lfs45(o + 40);
    const f64 k0 = Lfs45(0x000F8C64u), k1 = Lfs45(0x000F8C68u), k2 = Lfs45(0x000F8C6Cu);
    f64 ka = F45(a * a);
    f64 kb = F45(b * a);
    ka = F45(ka * k0);
    kb = F45(kb * k1);
    const f64 tka = F45(t * ka);
    f64 d = Fma45(t, kb, k2);
    d = Fma45(tka, t, d);
    f64 w = F45(kb + tka);
    const f64 cbx = F45(Lfs45(c) - bx);
    const f64 s = F45(k2 / d);
    const f64 cby = F45(Lfs45(c + 4) - by);
    const f64 dax = F45(Lfs45(o + 48) - ax);
    const f64 day = F45(Lfs45(o + 52) - ay);
    const f64 cbz = F45(Lfs45(c + 8) - bz);
    const f64 daz = F45(Lfs45(o + 56) - az);
    w = F45(w * s);
    ka = F45(ka * s);
    f64 vx = F45(w * cbx), vy = F45(w * cby), vz = F45(w * cbz);
    vx = Fma45(ka, dax, vx);
    vy = Fma45(ka, day, vy);
    vz = Fma45(ka, daz, vz);
    const f64 nbx = Fma45(vx, t, bx), nby = Fma45(vy, t, by), nbz = Fma45(vz, t, bz);
    const f64 nax = Fma45(t, nbx, ax), nay = Fma45(t, nby, ay), naz = Fma45(t, nbz, az);
    store<u32>(o + 32, W45(nbx));
    store<u32>(o + 36, W45(nby));
    store<u32>(o + 40, W45(nbz));
    store<u32>(o + 16, W45(nax));
    store<u32>(o + 20, W45(nay));
    store<u32>(o + 24, W45(naz));
    ctx.fpr[1] = nbz;
    ret(ctx, o);
}

// u32 f(T* o, Msg* m): Obj_TestE0BCC(o, m) == 0 -> 0. Damage x = m->+64 *
// o->+504, scale f = 1.0, or o->+516 / o->+520 for the message kinds
// 0x0020EE36 / 0x2DA0A51D (m->+8). When the source m->+4 has bit 0x8 of
// +192: same team ((+172 >> 22) & 3) and (the u16 m->+14 bit 0x1000 clear
// or team 0) and neither o->+188 bit 0x20 nor src->+188 bit 0x08000000 ->
// f = min(f, *0x01038988); in every one of those cases x *= f. f == 0 ->
// r3 = 0x0012703C (the address left in r3; f1 = 0); else
// Obj_Damage_00125A80(o, -, m, 0x045D9554, x) and r3 =
// Trophy_OnMsg_000B2AE0(o, m).
DECOMP(00127048, Obj_OnDamageMsg_00127048) {
    const u32 o = arg<u32>(ctx, 0), m = arg<u32>(ctx, 1);
    const u32 t = static_cast<u32>(call(ctx, 0x000E0BCCu, {o, m}));
    if (t == 0) {
        ret(ctx, t);
        return;
    }
    f64 x = F45(Lfs45(m + 64) * Lfs45(o + 504));
    f64 f = Lfs45(0x0012703Cu);
    const s32 kind = static_cast<s32>(load<u32>(m + 8));
    if (kind > 0x0020EE36) {
        if (kind == 0x2DA0A51D) f = Lfs45(o + 520);
    } else if (kind > 0x0020EE35) {
        f = Lfs45(o + 516);
    }
    const u32 src = load<u32>(m + 4);
    if (src != 0 && (load<u32>(src + 192) & 0x8u) != 0) {
        const u32 ta = (load<u32>(o + 172) >> 22) & 3u, tb = (load<u32>(src + 172) >> 22) & 3u;
        bool clamp = false;
        if (ta == tb) {
            if (((load<u16>(m + 14) >> 12) & 1u) == 0 || ta == 0) {
                if ((load<u32>(o + 188) & 0x20u) == 0 && (load<u32>(src + 188) & 0x08000000u) == 0) clamp = true;
            }
        }
        if (clamp) {
            const f64 k = Lfs45(0x01038988u);
            f = (f < k) ? f : k;
        }
        x = F45(x * f);
    }
    const f64 z = Lfs45(0x00127040u);
    if (f == z) {
        ctx.fpr[1] = z;
        ret(ctx, 0x0012703Cu);  // quirk: the constant pool address left in r3
        return;
    }
    // r4 (unused by the callee) holds a leftover value in the original
    callf(ctx, 0x00125A80u, {o, 0, m, 0x045D9554u}, {x});
    ret(ctx, call(ctx, 0x000B2AE0u, {o, m}));
}

// T* f(Xml* x, u32 name, u32 c, u32 d): a new 336-byte object (SmallAlloc;
// Wrapper_0CC4D4(p, c, d, 0, 0x01164BD0, 0x01164B30, 1, 0, 0, -, 0) with
// f1 = *0x00190E98, vtable 0x00B14DC8, +240 / +256 = the vector
// 0x01164B40, +272 / +280 / +304 = 1.0, +276 = 10.0, +284 = 40.0, +320 = 1,
// +4 |= 0x200, the rest zeroed), then XmlDef_Read_00190C28(x, name, p).
// r3 = p (0 when the allocation fails; the read still runs with 0).
DECOMP(00190EA0, Obj_New_B14DC8_00190EA0) {
    const u32 x = arg<u32>(ctx, 0), name = arg<u32>(ctx, 1), c = arg<u32>(ctx, 2), d = arg<u32>(ctx, 3);
    const u32 p = static_cast<u32>(call(ctx, 0x0055238Cu, {336}));
    if (p != 0) {
        // the 10th argument (sp + 120) is not written by the original; the callee skips it
        callf(ctx, 0x000CC4D4u, {p, c, d, 0, 0x01164BD0u, 0x01164B30u, 1, 0, 0, 0, 0}, {Lfs45(0x00190E98u)});
        store<u32>(p, 0x00B14DC8u);
        Cp45(p + 240, 0x01164B40u);
        Cp45(p + 244, 0x01164B44u);
        Cp45(p + 248, 0x01164B48u);
        Cp45(p + 256, 0x01164B40u);
        Cp45(p + 260, 0x01164B44u);
        const u32 v8 = load<u32>(0x01164B48u);
        store<u32>(p + 272, 0x3F800000u);
        store<u32>(p + 264, v8);
        store<u32>(p + 276, 0x41200000u);
        store<u32>(p + 280, 0x3F800000u);
        store<u8>(p + 288, 0);
        store<u32>(p + 284, 0x42200000u);
        store<u32>(p + 292, 0);
        store<u32>(p + 296, 0);
        store<u32>(p + 300, 0);
        const u32 fl = load<u32>(p + 4);
        store<u32>(p + 308, 0);
        store<u32>(p + 304, 0x3F800000u);
        store<u32>(p + 312, 0);
        store<u32>(p + 316, 0);
        store<u32>(p + 324, 0);
        store<u8>(p + 320, 1);
        store<u32>(p + 328, 0);
        store<u32>(p + 4, fl | 0x200u);
        ctx.fpr[1] = static_cast<f64>(std::bit_cast<f32>(v8));
    }
    call(ctx, 0x00190C28u, {x, name, p});
    ret(ctx, p);
}

// u32 f(T* o, const Def* d): Thunk_000E40B4(o, d); vtable 0x00B15C20 (the
// secondary ones 0x00B15DCC / 0x00B15DF4 at +380 / +384), +444 = FLT_MAX,
// +452..+464 = d->+208..+216, the flag word +468 takes bits 0x80000000 and
// 0x40000000 from d->+220 and clears 0x20000000; with i = the byte
// o->+216->+372: +424 = entry[i].+28 of the array +388 (48-byte entries)
// when i < its count, else 0 (stored again); +444 = (*(o->+216->+192))[i].+104 (160-byte
// entries). Then r3 = StateMachine_SetState(o, 0, 0).
DECOMP(001AF6A0, Obj_Init_B15C20_001AF6A0) {
    const u32 o = arg<u32>(ctx, 0), d = arg<u32>(ctx, 1);
    call(ctx, 0x000E40B4u, {o, d});
    store<u32>(o + 380, 0x00B15DCCu);
    store<u32>(o + 436, 0);
    store<u32>(o + 384, 0x00B15DF4u);
    store<u32>(o + 440, 0);
    store<u32>(o + 444, 0x7F7FFFFFu);
    store<u32>(o, 0x00B15C20u);
    store<u32>(o + 448, 0);
    const u32 w208 = load<u32>(d + 208);
    store<u32>(o + 424, 0);
    store<u32>(o + 428, 0);
    store<u32>(o + 452, w208);
    store<u32>(o + 432, 0);
    const u32 w212 = load<u32>(d + 212);
    store<u32>(o + 460, 0);
    store<u32>(o + 456, w212);
    Cp45(o + 464, d + 216);
    u32 f = (load<u32>(o + 468) & 0x7FFFFFFFu) | (load<u32>(d + 220) & 0x80000000u);
    const u32 st = load<u32>(o + 216);
    const u32 arr = load<u32>(o + 388);
    store<u32>(o + 468, f);
    f = (f & ~0x40000000u) | (load<u32>(d + 220) & 0x40000000u);
    f &= ~0x20000000u;
    store<u32>(o + 468, f);
    const u32 tab = load<u32>(load<u32>(st + 192));
    const u32 i = load<u8>(st + 372);
    const u32 n = arr != 0 ? load<u16>(arr + 8) : 0;
    store<u32>(o + 424, i < n ? load<u32>(load<u32>(arr) + 48 * i + 28) : 0);
    const u32 e = tab + 160 * i;
    Cp45(o + 444, e + 104);
    ctx.fpr[1] = Lfs45(e + 104);
    ret(ctx, call(ctx, 0x00464D1Cu, {o, 0, 0}));
}

// u32 f(T* o): clears bits 0xC0000000 of +436; with the layer p = +440
// set: Obj_PopLayer(p->+4, p, t) where t = +448, or (when +448 < the
// constant c = *0x002163A4) c, or p->+72 when p->vt[8](p) == 2; then
// p->vt[4](p) and +440 = 0. When +432 is 0x922D3BD1, every layer of q =
// p->+4 from q->+364[q->+345] down (Obj_LayerBelow) that answers 2 to
// vt[8] gets bit 0x40000000 of +176. r3 = 0, or the slot address
// q + 364 + 4 * i when that slot (i != 0) is empty.
DECOMP(002163A8, Obj_ClearLayer_002163A8) {
    const u32 o = arg<u32>(ctx, 0);
    store<u32>(o + 436, load<u32>(o + 436) & 0x3FFFFFFFu);
    const u32 p = load<u32>(o + 440);
    if (p == 0) {
        ret(ctx, 0);
        return;
    }
    const f64 t = Lfs45(o + 448);
    f64 c = Lfs45(0x002163A4u);
    const u32 q = load<u32>(p + 4);
    if (t < c) {
        if (static_cast<u32>(VCall(ctx, p, 8, {p})) == 2) c = Lfs45(load<u32>(o + 440) + 72);
        callf(ctx, 0x000FBB88u, {q, load<u32>(o + 440)}, {c});
    } else {
        callf(ctx, 0x000FBB88u, {q, p}, {t});
    }
    const u32 p2 = load<u32>(o + 440);
    if (p2 != 0) VCall(ctx, p2, 4, {p2});
    const u32 kind = load<u32>(o + 432);
    store<u32>(o + 440, 0);
    if (kind != 0x922D3BD1u) {
        ret(ctx, 0);
        return;
    }
    const u32 b = load<u8>(q + 345);
    u32 r3 = 0, l = 0;
    if (b != 0) {
        r3 = q + 364 + 4 * b;
        l = load<u32>(r3);
    }
    if (l == 0) {
        ret(ctx, r3);  // the slot address left in r3
        return;
    }
    while (l != 0) {
        if (static_cast<u32>(VCall(ctx, l, 8, {l})) == 2) store<u32>(l + 176, load<u32>(l + 176) | 0x40000000u);
        l = static_cast<u32>(call(ctx, 0x000FCBD0u, {q, l}));
    }
    ret(ctx, 0);
}

// u32 f(u32* out, T* o, Sound* snd, Pos* pos, u32 tag): plays snd at pos
// on o (up to 8 voices). snd == 0 -> handle 0. With 8 voices (+448 >= 8):
// String_Format(0x00AD5070, the name o->+8->vt[76]() (or 0x00AD50B0 when
// o->+8 is 0), ReturnK_004440E8(snd)) and handle 0. Else pos gets the
// position o->+8->+144 and +56 |= 2, Audio_Start_0044A374(&h, snd, pos,
// pos, o->+8 + 144, flags); a nonzero handle h goes into the voice slot
// {+384, +388} = {h, tag} and ++o->+448. *out = r3 = the handle.
DECOMP(00298C10, Obj_PlaySound_00298C10) {
    const u32 out = arg<u32>(ctx, 0), o = arg<u32>(ctx, 1), snd = arg<u32>(ctx, 2), pos = arg<u32>(ctx, 3), tag = arg<u32>(ctx, 4);
    Stack frame(ctx, 16);
    const u32 s = frame.addr();
    store<u32>(s, 0);
    store<u16>(s + 2, 0);
    if (snd != 0) {
        if (load<u32>(o + 448) < 8) {
            const u32 src = load<u32>(o + 8) + 144;
            const u32 fl = load<u32>(pos + 56) | 2u;
            Cp45(pos, src);
            Cp45(pos + 4, src + 4);
            const u32 z = load<u32>(src + 8);
            store<u32>(pos + 56, fl);
            store<u32>(pos + 8, z);
            ctx.fpr[1] = static_cast<f64>(std::bit_cast<f32>(z));
            call(ctx, 0x0044A374u, {s + 4, snd, pos, pos, src, fl});
            const u32 h = load<u32>(s + 4);
            store<u32>(s, h);
            const u32 n = load<u32>(o + 448);
            if ((h >> 16) != 0 || (h & 0xFFFFu) != 0) {
                store<u32>(o + 384 + 8 * n, h);
                const u32 n2 = load<u32>(o + 448);
                store<u32>(o + 448, n2 + 1);
                store<u32>(o + 388 + 8 * n2, tag);
            }
        } else {
            const u32 c = load<u32>(o + 8);
            const u32 name = c != 0 ? static_cast<u32>(VCall(ctx, c, 76, {c})) : 0x00AD50B0u;
            const u32 k = static_cast<u32>(call(ctx, 0x004440E8u, {snd}));
            call(ctx, 0x005534D0u, {0x00AD5070u, name, k});
        }
    }
    const u32 h = load<u32>(s);
    store<u32>(out, h);
    ret(ctx, h);
}

// u32 f(T* o): pops the head of the queue +356 (12-byte entries {a, b,
// u8 flag}, u16 count +364; 0 when empty -> r3 = 0): when a is set and
// CallK2_000D9614(a) gives an object x and flag is set, x->vt[44](x, 1);
// the entries move down one, --count; then a->vt[40](a, 0) and
// b->vt[8](b) for those set. r3 = 0.
DECOMP(002C69A0, Queue_PopHead_002C69A0) {
    const u32 o = arg<u32>(ctx, 0);
    if (load<u16>(o + 364) == 0) {
        ret(ctx, 0);
        return;
    }
    const u32 q = load<u32>(o + 356);
    const u32 a = load<u32>(q), b = load<u32>(q + 4);
    const u32 flag = load<u8>(q + 8);
    if (a != 0) {
        const u32 x = static_cast<u32>(call(ctx, 0x000D9614u, {a}));
        if (x != 0 && flag != 0) VCall(ctx, x, 44, {x, 1});
    }
    u32 n = load<u16>(o + 364);
    if (static_cast<s32>(n - 1) > 0) {
        u32 i = 0;
        do {
            const u32 d = load<u32>(o + 356) + 12 * i;
            const u32 sr = d + 12;
            const u8 fb = load<u8>(sr + 8);
            const u32 w0 = load<u32>(sr);
            store<u8>(d + 8, fb);
            store<u32>(d, w0);
            const u32 w1 = load<u32>(sr + 4);
            n = load<u16>(o + 364);
            store<u32>(d + 4, w1);
            i = (i + 1) & 0xFFFFu;
        } while (static_cast<s32>(i) < static_cast<s32>(n - 1));
    }
    store<u16>(o + 364, static_cast<u16>(n - 1));
    if (a != 0) VCall(ctx, a, 40, {a, 0});
    if (b != 0) VCall(ctx, b, 8, {b});
    ret(ctx, 0);
}

// u32 f(T* o) (destructor): vtables 0x00B1D268 (+0) / 0x00B1D2B4 (+24);
// the owner +32 (when set) gets bit 0x200 of +136 and +32 = 0;
// PtrVector_Clear(o + 68), Nop(o + 68), Registry_Unlink2_0048F6B0(o + 24),
// vtable 0x00B1BE30, o taken out of the entry array of 0x01056F1C;
// r3 = List1013B5C_Remove(o).
DECOMP(002D31B8, Obj_Dtor_B1D268_002D31B8) {
    const u32 o = arg<u32>(ctx, 0);
    store<u32>(o, 0x00B1D268u);
    store<u32>(o + 24, 0x00B1D2B4u);
    const u32 w = load<u32>(o + 32);
    if (w != 0) {
        store<u32>(w + 136, load<u32>(w + 136) | 0x200u);
        store<u32>(o + 32, 0);
    }
    call(ctx, 0x0054EC1Cu, {o + 68});
    call(ctx, 0x0054EC18u, {o + 68});
    call(ctx, 0x0048F6B0u, {o + 24});
    store<u32>(o, 0x00B1BE30u);
    sly4::obj39::Unlist1056F1C39(o);
    ret(ctx, call(ctx, 0x000D6940u, {o}));
}

// u32 f(T* o, -, -, bool set, Obj* tgt, const u8 key[4], f32 t):
// SetSpeech45 with the constants 0x0037CB08 (0, 1.0), the hash
// 0x00F64FE8 and Obj_37AC70 / Obj_37AE00.
DECOMP(0037CB10, Obj_SetSpeech_0037CB10) { SetSpeech45(ctx, 0x0037CB08u, 0x00F64FE8u, 0x0037AC70u, 0x0037AE00u); }

// The same as 0x0037CB10 with the constants 0x0037F910 (0, 1.0), the hash
// 0x00F65004 and Obj_37F1A0 / Obj_37F32C.
DECOMP(0037F918, Obj_SetSpeech_0037F918) { SetSpeech45(ctx, 0x0037F910u, 0x00F65004u, 0x0037F1A0u, 0x0037F32Cu); }

// u32 f(T* o, bool now): when +460 is 1 and not now, r3 = 1. Else the
// nine motion rates +156 + 36 k = the defaults of the table 0x01087A70..
// (+72 of each entry), and each motion m = o + 132 + 36 k gets u16 +16 =
// 1, +20 = +28, +4 = 0 and Motion_Set4A0C20(m; *0x01087BA8) (and
// Obj_Blend_004A0E60(m; same) when now); +460 = 1. r3 = the last call's.
DECOMP(003ACE58, Obj_ResetMotions_003ACE58) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 now = arg<u32>(ctx, 1) & 0xFFu;
    if (static_cast<s32>(load<u32>(o + 460)) == 1 && now == 0) {
        ret(ctx, 1);
        return;
    }
    const f64 r = Lfs45(0x01087BA8u);
    static const u32 tv[9] = {0x01087C98u, 0x01087CE8u, 0x01087BF8u, 0x01087C48u, 0x01087DD8u,
                              0x01087E28u, 0x01087D38u, 0x01087D88u, 0x01087AB8u};
    for (u32 k = 0; k < 9; ++k) Cp45(o + 156 + 36 * k, tv[k]);
    u64 r3 = 0;
    for (u32 i = 0; i < 9; ++i) {
        const u32 m = o + 132 + 36 * i;
        const u32 v = load<u32>(m + 28);
        store<u16>(m + 16, 1);
        store<u32>(m + 20, v);
        store<u32>(m + 4, 0);
        // r4 is a leftover (the last table address, then a callee's); Motion_Set reads r3 and f1 only
        r3 = (callf(ctx, 0x004A0C20u, {m}, {r}), ctx.gpr[3]);
        if (now != 0) r3 = (callf(ctx, 0x004A0E60u, {m}, {r}), ctx.gpr[3]);
    }
    store<u32>(o + 460, 1);
    ret(ctx, r3);
}

// u32 f(T* o): with the state s = +432 (the hashes 0x342DE5A9 /
// 0x56F7EC21 / 0x590FD1DB for 0 / 1 / 2, else none) and its animation
// key k = o->+132[2 s]: unless bit 0x04000000 of +456 is set,
// Hash_ToString(k) when the debug byte 0x010853C4 is set, and when k and
// the animator +412 are set and t = animator->vt[60]() is nonzero,
// Anim_Restart2775A8(t, k, 0, 0, -1, -, 0, 0, -, -, 0, -1; 1.0, 0.25,
// 0, 0 from 0x003D73F8). Then Emitter_StopById44CEE8(3, 1); r3 =
// Effect_SetHash4CF988(o->+32, hash) when both are set, else the stop's.
DECOMP(003D7408, Obj_PlayStateAnim_003D7408) {
    const u32 o = arg<u32>(ctx, 0);
    const s32 st = static_cast<s32>(load<u32>(o + 432));
    const u32 fl = load<u32>(o + 456);
    u32 hash = 0;
    if (st == 0) hash = 0x342DE5A9u;
    else if (st == 1) hash = 0x56F7EC21u;
    else if (st == 2) hash = 0x590FD1DBu;
    const u32 k = load<u32>(o + 132 + 8 * static_cast<u32>(st));
    if ((fl & 0x04000000u) == 0) {
        if (load<u8>(0x010853C4u) != 0) call(ctx, 0x0056A6E0u, {k});
        if (k != 0) {
            const u32 an = load<u32>(o + 412);
            if (an != 0) {
                const u32 t = static_cast<u32>(VCall(ctx, an, 60, {an}));
                if (t != 0) {
                    const f64 c = Lfs45(0x003D7400u);
                    // r8 and the 9th / 10th stack slots are leftovers the callee does not read
                    const f64 a = Lfs45(0x003D73F8u), b = Lfs45(0x003D73FCu);
                    callf(ctx, 0x002775A8u, {t, k, 0, 0, ~0ull, ctx.gpr[8], 0, 0, 0, 0, 0, ~0ull}, {a, b, c, c});
                }
            }
        }
    }
    u64 r3 = call(ctx, 0x0044CEE8u, {3, 1});
    const u32 fx = load<u32>(o + 32);
    if (fx != 0 && hash != 0) r3 = call(ctx, 0x004CF988u, {fx, hash});
    ret(ctx, r3);
}

// u32 f(T* o): the stick direction bits of the input channels 0 and 1 of
// 0x010B1A90 (Obj_GetChannel_0045B160(in, ch), dead zone d =
// *0x01085C1C), starting from in->+8 & in->+12: a channel inside the dead
// zone (d > 0 and |v| < d) sets its latch byte (+2208 / +2209); one
// outside it, with the latch set or d <= 0, clears the latch and adds
// 4 / 1 for v < 0, else 8 / 2. r3 = the bits; f1 = d.
DECOMP(003E73A8, Input_StickBits_003E73A8) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 in = 0x010B1A90u;
    u32 bits = load<u32>(in + 8) & load<u32>(in + 12);
    const f64 z = Lfs45(0x003E73A0u);
    const u32 dz = 0x01085C1Cu;
    for (u32 ch = 0; ch < 2; ++ch) {
        const u32 latch = o + 2208 + ch;
        const f64 v = callf(ctx, 0x0045B160u, {in, ch}, {});
        const f64 n = -v;
        const f64 d = Lfs45(dz);
        if (d > z && std::fabs(n) < d) {
            store<u8>(latch, 1);
        } else if (load<u8>(latch) != 0 || d <= z) {
            const u32 hi = ch == 0 ? 8u : 2u, lo = ch == 0 ? 4u : 1u;
            bits = n > z ? (bits | lo) : (bits | hi);
            store<u8>(latch, 0);
        }
        ctx.fpr[1] = Lfs45(dz);
    }
    ret(ctx, bits);
}

// u32 f(T* o, u32 a, const Def* d, u32 b, u32 e): SubMachine_Construct(o,
// a, b, d->+48, e) (e passes through in r7), vtable 0x00B209A0, +88 = d, +72 = 1.0, +84..+86 = 1, the top
// bit of +92 cleared, the rest of +53..+96 zeroed; PtrVector_Construct(o
// + 100), u16 +106 = 4, the 4x3 rows +128.. = 0x01164BD0.. (w omitted),
// +208 = 2.0, +192..+241 zeroed; +28 |= 0x400. r3 = the new +28; f1 = the
// last row word.
DECOMP(0040773C, Obj_Construct_B209A0_0040773C) {
    const u32 o = arg<u32>(ctx, 0), a = arg<u32>(ctx, 1), d = arg<u32>(ctx, 2), b = arg<u32>(ctx, 3), e = arg<u32>(ctx, 4);
    call(ctx, 0x0047C5D8u, {o, a, b, load<u32>(d + 48), e});
    const u32 w92 = load<u32>(o + 92) & 0x7FFFFFFFu;
    store<u8>(o + 53, 0);
    store<u8>(o + 54, 0);
    store<u32>(o + 60, 0);
    store<u8>(o + 68, 0);
    store<u32>(o + 72, 0x3F800000u);
    store<u32>(o + 76, 0);
    store<u8>(o + 80, 0);
    store<u8>(o + 81, 0);
    store<u8>(o + 82, 0);
    store<u8>(o + 83, 0);
    store<u8>(o + 84, 1);
    store<u8>(o + 85, 1);
    store<u8>(o + 86, 1);
    store<u8>(o + 87, 0);
    store<u32>(o, 0x00B209A0u);
    store<u32>(o + 88, d);
    store<u32>(o + 92, w92);
    store<u8>(o + 93, 0);
    store<u32>(o + 96, 0);
    call(ctx, 0x005516E0u, {o + 100});
    const u32 k = 0x01164BD0u;
    Cp45(o + 128, k);
    Cp45(o + 132, k + 4);
    Cp45(o + 136, k + 8);
    Cp45(o + 144, k + 16);
    Cp45(o + 148, k + 20);
    Cp45(o + 152, k + 24);
    store<u16>(o + 106, 4);
    Cp45(o + 160, k + 32);
    Cp45(o + 164, k + 36);
    Cp45(o + 168, k + 40);
    Cp45(o + 176, k + 48);
    store<u32>(o + 192, 0);
    Cp45(o + 180, k + 52);
    store<u32>(o + 196, 0);
    Cp45(o + 184, k + 56);
    store<u32>(o + 200, 0);
    store<u32>(o + 208, 0x40000000u);
    store<u32>(o + 212, 0);
    store<u32>(o + 216, 0);
    store<u32>(o + 220, 0);
    for (u32 i = 228; i <= 232; ++i) store<u8>(o + i, 0);
    store<u32>(o + 236, 0);
    store<u8>(o + 241, 0);
    const u32 w28 = load<u32>(o + 28) | 0x400u;
    store<u32>(o + 28, w28);
    ctx.fpr[1] = Lfs45(k + 56);
    ret(ctx, w28);
}

// bool f(T* o, -, Vec4* out, u32* n, f32 t): copies the last
// m = min(max(trunc(t / *0x011DE098), 1), 360) samples of the 360-entry
// ring o->+5760 (16-byte entries, newest first from index +11524 - 1,
// wrapping to 359), limited to +11524 - +11520 when that is positive,
// as xyz into out; *n = the count copied; r3 = (count == m). The first
// call sets up the record 0x011DE094 (+20 / +24 = 0, bits 0xC0 of +32
// cleared, 400 bytes at +92 zeroed, *0x011972E8 = it, the guard byte
// 0x011DE280 = 1).
DECOMP(0046426C, Ring_CopyRecent_0046426C) {
    const u32 o = arg<u32>(ctx, 0), out0 = arg<u32>(ctx, 2), np = arg<u32>(ctx, 3);
    const f64 t = farg(ctx, 0);
    if (load<u8>(0x011DE280u) == 0) {
        const u32 r = 0x011DE094u;
        u8 b = load<u8>(r + 32);
        store<u32>(r + 20, 0);
        b &= static_cast<u8>(~0x80u);
        store<u32>(r + 24, 0);
        b &= static_cast<u8>(~0x40u);
        store<u8>(r + 32, b);
        call(ctx, 0x00726E54u, {r + 92, 0, 400});
        store<u32>(0x011972E8u, r);
        store<u8>(0x011DE280u, 1);
    }
    const f64 q = F45(t / Lfs45(0x011DE098u));
    const u64 nb = rt::fctid(q, true);
    const u32 n = static_cast<u32>(nb);
    const u32 n1 = n > 1 ? n : 1;
    const u32 m = n1 < 360 ? n1 : 360;
    const u32 hi = load<u32>(o + 11524);
    const u32 lo = load<u32>(o + 11520);
    const u32 avail = hi > lo ? hi - lo : 360;
    const u32 cnt = avail < m ? avail : m;
    u32 i = 0;
    ctx.fpr[1] = std::bit_cast<f64>(nb);  // f1 keeps the fctidz bits unless a sample is copied
    if (cnt != 0) {
        u32 idx = hi - 1, out = out0;
        const u32 ring = o + 5760;
        do {
            if (static_cast<s32>(idx) < 0) idx = 359;
            const u32 src = ring + 16 * idx;
            ++i;
            --idx;
            Cp45(out, src);
            Cp45(out + 4, src + 4);
            Cp45(out + 8, src + 8);
            ctx.fpr[1] = Lfs45(src + 8);
            out += 16;
        } while (i < cnt);
    }
    store<u32>(np, i);
    ret(ctx, m == i ? 1u : 0u);
}

// u32 f(Node* o): Owner_AddName(o, 0x00AEE368, 1),
// AnimClipNode_BindParams_004B2DA0(o); the string list b =
// Node_Bind(o, 0x00AEE910, 5, 0, -1, 0) gets Binding_Reserve(b, n) and a
// 100-byte copy (Mem_Alloc(100, 0, 16) into b->+68[count], String_CopyN
// from the name of entry i of the n = (*0x00F65F84)->+268 names at
// +264, 8-byte entries; index 0 is read through a null entry when i is
// out of range) for each i while count (b->+0) < capacity (+72); then
// r3 = Node_Bind(o, 0x00AEEAB4, 4, 1, -1, 0).
DECOMP(004B64D4, AnimNode_BindNames_004B64D4) {
    const u32 o = arg<u32>(ctx, 0);
    call(ctx, 0x004D3378u, {o, 0x00AEE368u, 1});
    call(ctx, 0x004B2DA0u, {o});
    const u32 b = static_cast<u32>(call(ctx, 0x004D3088u, {o, 0x00AEE910u, 5, 0, ~0ull, 0}));
    const u32 n = load<u32>(load<u32>(0x00F65F84u) + 268);
    call(ctx, 0x004D2BB8u, {b, n});
    if (n != 0) {
        u32 cnt = load<u32>(b);
        for (u32 i = 0; i < n; ++i) {
            const u32 g = load<u32>(0x00F65F84u);
            const s32 j = static_cast<s16>(i);
            const u32 e = (j < 0 || !(j < static_cast<s32>(load<u32>(g + 268)))) ? 0u : load<u32>(g + 264) + 8 * static_cast<u32>(j);
            const u32 cap = load<u32>(b + 72);
            const u32 name = load<u32>(e);
            if (!(cnt < cap)) continue;
            const u32 p = static_cast<u32>(call(ctx, 0x0054ED38u, {100, 0, 16}));
            const u32 c = load<u32>(b);
            store<u32>(load<u32>(b + 68) + 4 * c, p);
            call(ctx, 0x00553638u, {load<u32>(load<u32>(b + 68) + 4 * c), name, 100});
            cnt = load<u32>(b) + 1;
            store<u32>(b, cnt);
        }
    }
    ret(ctx, call(ctx, 0x004D3088u, {o, 0x00AEEAB4u, 4, 1, ~0ull, 0}));
}

// V* f(SortedMap* m, const s32* key): binary search of the 8-byte {key,
// value} entries +8 (u16 count +2), sorted first with qsort(+8, count, 8,
// cmp +20) when bit 1 of the u16 +6 is set (cleared); keys compare as
// s32 when cmp is the default 0x00F377A8, else through cmp(entry, &k).
// r3 = &entry.value, or m + 16 (the default) when absent.
DECOMP(0076ECC4, SortedMap_Find_0076ECC4) {
    const u32 m = arg<u32>(ctx, 0), key = arg<u32>(ctx, 1);
    const u32 f = load<u16>(m + 6);
    if ((f & 1u) != 0) {
        const u32 cnt = load<u16>(m + 2);
        store<u16>(m + 6, static_cast<u16>(f & 0xFFFEu));
        if (static_cast<s32>(cnt) > 0) call(ctx, 0x0071EC28u, {load<u32>(m + 8), cnt, 8, load<u32>(m + 20)});
    }
    s32 hi = static_cast<s32>(load<u16>(m + 2)) - 1, lo = 0;
    u32 base = load<u32>(m + 8);
    const u32 cmp = load<u32>(m + 20);
    if (cmp == 0x00F377A8u) {
        while (hi >= lo) {
            const s32 mid = static_cast<s32>(static_cast<u32>(hi) + static_cast<u32>(lo)) >> 1;
            const s32 k = static_cast<s32>(load<u32>(key));
            const s32 e = static_cast<s32>(load<u32>(base + 8 * static_cast<u32>(mid)));
            if (e > k) hi = mid - 1;
            else if (e < k) lo = mid + 1;
            else {
                if (mid < 0) break;
                ret(ctx, base + 8 * static_cast<u32>(mid) + 4);
                return;
            }
        }
    } else {
        Stack frame(ctx, 16);
        const u32 s = frame.addr();
        store<u32>(s, load<u32>(key));
        while (hi >= lo) {
            const s32 mid = static_cast<s32>(static_cast<u32>(hi) + static_cast<u32>(lo)) >> 1;
            const s32 r = static_cast<s32>(call_ptr(ctx, cmp, {base + 8 * static_cast<u32>(mid), s, cmp, load<u32>(cmp)}));
            if (r > 0) hi = mid - 1;
            else if (r < 0) lo = mid + 1;
            else {
                base = load<u32>(m + 8);
                if (mid < 0) break;
                ret(ctx, base + 8 * static_cast<u32>(mid) + 4);
                return;
            }
        }
    }
    ret(ctx, m + 16);
}
