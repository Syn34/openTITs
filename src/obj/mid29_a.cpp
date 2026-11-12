// Game code, medium sweep part 29: AnimScript commands (target / component
// resolution, track sign, pending flushes, material binding).
#include "mid29.h"

using namespace sly4;
using namespace sly4::mid29;

namespace {
using namespace m29fn;
}  // namespace

// u32 f(Cmd* c, Effect* x): plays the track c->+20 of the animation
// behind the target backwards. The animation: the target bound as c->+12
// (Effect_Bind4BB7E0(x, key, 0), else with create = 1) gives vt[60]()->+36;
// with no key Obj_FindById(c->+16) does the same (its +160 kept as the
// fallback record); else Effect_GetComp36_004CF2BC(x, c->+12, record).
// Its track (Obj_FindComp34CC34) gets the speed s = Field_Get32_64f, or
// with a kind-1 component c->+24 of duration d (vt[20]) length / d (1
// when d <= 0) signed like s; Obj_SetSign_003541E0(track, -s). A negative
// result on a track without bit 0x08000000 of +12 is pushed to the world
// list (*0x00F64C9C + 12) with HkArray_Push132_003540BC. r3 / f1 as the
// original leaves them.
DECOMP(00085B10, AnimCmd_PlayTrackReverse_00085B10) {
    const u32 c = arg<u32>(ctx, 0);
    const u32 x = arg<u32>(ctx, 1);
    u32 rec = 0;  // r28
    u32 t = 0;    // r3 at 0x00085BB0
    bool have = true;
    if (load<u32>(c + 12) != 0) {
        rec = static_cast<u32>(call(ctx, Bind29, {x, load<u32>(c + 12), 0}));
        if (rec == 0) rec = static_cast<u32>(call(ctx, Bind29, {x, load<u32>(c + 12), 1}));
        if (rec == 0) return;
        t = load<u32>(rec + 276) == kTarget29 ? load<u32>(rec + 280) : 0;
    } else if (const u32 id = load<u32>(c + 16); id != 0) {
        t = static_cast<u32>(call(ctx, FindById29, {id}));
        rec = load<u32>(t + 160);  // read before the null test (as the original)
    } else {
        have = false;
    }
    u32 a = 0;
    if (have && t != 0) {
        a = static_cast<u32>(VCall(ctx, t, 60, {t}));
        if (a != 0) a = load<u32>(a + 36);
    }
    if (a == 0) {
        a = static_cast<u32>(call(ctx, 0x004CF2BCu, {x, load<u32>(c + 12), rec}));  // Effect_GetComp36_004CF2BC
        if (a == 0) return;
    }
    const u32 tr = static_cast<u32>(call(ctx, 0x0034CC34u, {a, load<u32>(c + 20)}));  // Obj_FindComp34CC34
    if (tr == 0) return;
    call(ctx, 0x003541B0u, {tr});  // Field_Get32_64f_003541B0
    const f64 s = ctx.fpr[1];
    f64 v = s;
    if (const u32 k = load<u32>(c + 24); k != 0) {
        const u32 e = static_cast<u32>(call(ctx, FindComp29, {x, k}));
        if (e != 0 && static_cast<u32>(VCall(ctx, e, 32, {e})) == 1) {
            VCall(ctx, e, 20, {e});
            const f64 d = ctx.fpr[1];
            const f64 zero = Lf(0x00085B04u);
            f64 r = Lf(0x00085B08u);
            if (d > zero) {
                call(ctx, 0x00353748u, {tr});  // Track_Length353748
                r = Sgl28(ctx.fpr[1] / d);
            }
            v = r;
            if (s < zero) v = -v;
        }
    }
    v = -v;
    callf(ctx, 0x003541E0u, {tr}, {v});  // Obj_SetSign_003541E0
    ctx.fpr[1] = Lf(0x00085B04u);
    if (!(v < Lf(0x00085B04u))) {
        ctx.gpr[3] = 0x00085B04u;
        return;
    }
    if ((load<u32>(tr + 12) & 0x08000000u) != 0) {
        ctx.gpr[3] = 1;
        return;
    }
    const u32 w = load<u32>(0x00F64C9Cu);
    call(ctx, 0x003540BCu, {tr, w != 0 ? w + 12 : 0});  // HkArray_Push132_003540BC
}

// u32 f(Cmd* c, Effect* x): resets the target a (CompTarget29 with key
// c->+52, else Obj_FindById(c->+56); used only with bit 8 of +192) onto
// the target b (key c->+60, else id c->+64). A missing side formats its
// two keys with Hash_ToString. With bit 0x80 of u8 a->+704 set and 0x40
// clear: Obj_Reset121FA8(a, b, u8 c->+68 ? c + 48 : 0, u8 c->+69 ? c + 16
// : 0, u8 c->+70 ? c + 32 : 0, u8 c->+71), b->+432 bits 0x300 = 0x100 when
// bit 0x20 of b->+192 is set, then Player_Settle11F0E4(a, 0x0006D074, 1,
// f1 = *0x0006D074); else Hash_ToString of both +168 names. r3 as the
// original leaves it.
DECOMP(0006D078, AnimCmd_ResetToTarget_0006D078) {
    const u32 c = arg<u32>(ctx, 0);
    const u32 x = arg<u32>(ctx, 1);
    const u32 a = TargetOrId29(ctx, c, x, 52, 56);
    const u32 b = TargetOrId29(ctx, c, x, 60, 64);
    u32 t = 0;
    if (a != 0 && (load<u32>(a + 192) & 8u) != 0) t = a;
    if (t == 0) {
        DbgHash29(ctx, load<u32>(c + 52));
        DbgHash29(ctx, load<u32>(c + 56));
    }
    if (b == 0) {
        DbgHash29(ctx, load<u32>(c + 60));
        DbgHash29(ctx, load<u32>(c + 64));
    }
    if (b == 0 || t == 0) return;
    const u32 fl = load<u8>(t + 704);
    if ((fl & 0x80u) == 0 || (fl & 0x40u) != 0) {
        call(ctx, HashStr29, {load<u32>(t + 168)});
        call(ctx, HashStr29, {load<u32>(b + 168)});
        return;
    }
    const u32 p48 = load<u8>(c + 68) != 0 ? c + 48 : 0;
    const u32 p16 = load<u8>(c + 69) != 0 ? c + 16 : 0;
    const u32 p32 = load<u8>(c + 70) != 0 ? c + 32 : 0;
    call(ctx, 0x00121FA8u, {t, b, p48, p16, p32, load<u8>(c + 71)});  // Obj_Reset121FA8
    if ((load<u32>(b + 192) & 0x20u) != 0) store<u16>(b + 432, static_cast<u16>((load<u16>(b + 432) & ~0x300u) | 0x100u));
    callf(ctx, 0x0011F0E4u, {t, 0x0006D074u, 1}, {Lf(0x0006D074u)});  // Player_Settle11F0E4
}

// u32 f(Cmd* c, Effect* x): flushes pending work. Mode c->+12 == 0xFD81:
// the global list *0x00F65BE8 (List_FlushPending_00454DBC when u8 c->+20,
// else f_00454E88). Else the object: the kind-17 component c->+16's +20
// (Hash_ToString(c->+16) when that fails), else with a non-zero mode the
// list's map entry (SortedMap_Find733F8C(list + 4, &mode), Hash_ToString
// of the mode when empty). It gets Pending_Flush_00453578(o, u8 c->+22)
// when u8 c->+21; with u8 c->+20 Obj_FlushPending_0045477C (u8 c->+24),
// vt[24](), and with u8 c->+23 Obj_Tick4536C0(o, 0x00073AA4, list, f1 =
// 0) keeping the list's first byte; else vt[28](o, 0). r3 as the original
// leaves it.
DECOMP(00073AA8, AnimCmd_FlushPending_00073AA8) {
    constexpr u32 kList = 0x00F65BE8u;
    const u32 c = arg<u32>(ctx, 0);
    const u32 x = arg<u32>(ctx, 1);
    u32 o = 0;
    if (const u32 k = load<u32>(c + 16); k != 0) {
        const u32 e = static_cast<u32>(call(ctx, FindComp29, {x, k}));
        if (e != 0 && static_cast<u32>(VCall(ctx, e, 32, {e})) == 17) o = load<u32>(e + 20);
        else call(ctx, HashStr29, {load<u32>(c + 16)});
    }
    const u32 mode = load<u32>(c + 12);
    if (mode == 0xFD81u) {
        call(ctx, load<u8>(c + 20) != 0 ? 0x00454DBCu : 0x00454E88u, {load<u32>(kList)});
        return;
    }
    if (mode != 0 && o == 0) {
        Stack frame(ctx, 16);  // the key at 112(r1)
        const u32 key = frame.addr();
        store<u32>(key, mode);
        const u32 p = static_cast<u32>(call(ctx, 0x00733F8Cu, {load<u32>(kList) + 4, key}));  // SortedMap_Find733F8C
        o = load<u32>(p);
        if (o == 0) call(ctx, HashStr29, {load<u32>(c + 12)});
    }
    if (o == 0) {
        if (mode == 0) ctx.gpr[3] = 0;
        return;
    }
    if (load<u8>(c + 21) != 0) call(ctx, 0x00453578u, {o, load<u8>(c + 22)});  // Pending_Flush_00453578
    if (load<u8>(c + 20) == 0) {
        VCall(ctx, o, 28, {o, 0});
        return;
    }
    if (load<u8>(c + 24) != 0) call(ctx, 0x0045477Cu, {o});  // Obj_FlushPending_0045477C
    VCall(ctx, o, 24, {o});
    if (load<u8>(c + 23) == 0) {
        ctx.gpr[3] = 0;
        return;
    }
    const u32 l = load<u32>(kList);
    const u8 keep = load<u8>(l);
    callf(ctx, 0x004536C0u, {o, 0x00073AA4u, l}, {Lf(0x00073AA4u)});  // Obj_Tick4536C0
    const u32 l2 = load<u32>(kList);
    store<u8>(l2, keep);
    ctx.gpr[3] = l2;
}

// u32 f(Cmd* c, Effect* x): copies the target's state into components.
// Nothing unless one of c->+24..+40 is set. The target: the player slot
// c->+20 (!= -1), else the one bound as c->+12 (Effect_Bind4BB7E0 create
// 0, then 1), else Obj_FindById(c->+16); used with bit 8 of +192. Its
// 3x4 matrix at +576 goes to the kind-4 component c->+24 (+32..+88, rows
// of 3), its four bytes at +640 to the +20 words of the kind-2
// components c->+28 / +32 / +36 / +40. r3 as the original leaves it.
DECOMP(0007BF3C, AnimCmd_CopyTargetState_0007BF3C) {
    const u32 c = arg<u32>(ctx, 0);
    const u32 x = arg<u32>(ctx, 1);
    const u32 k12 = load<u32>(c + 12);
    const u32 id = load<u32>(c + 16);
    const s32 slot = static_cast<s32>(load<u32>(c + 20));
    if (load<u32>(c + 24) == 0 && load<u32>(c + 28) == 0 && load<u32>(c + 32) == 0 && load<u32>(c + 36) == 0 &&
        load<u32>(c + 40) == 0) {
        ctx.gpr[3] = id;
        return;
    }
    u32 t;
    if (slot != -1) {
        const u32 p = load<u32>(kWorld29) + static_cast<u32>(slot) * 0x00020C80u + 4208;
        t = load<u32>(p + 8);
        ctx.gpr[3] = p;
    } else if (k12 == 0) {
        if (id == 0) {
            ctx.gpr[3] = 0;
            return;
        }
        t = static_cast<u32>(call(ctx, FindById29, {id}));
    } else {
        u32 b = static_cast<u32>(call(ctx, Bind29, {x, k12, 0}));
        if (b == 0) b = static_cast<u32>(call(ctx, Bind29, {x, load<u32>(c + 12), 1}));
        if (b == 0) return;
        t = RecTarget29(ctx, b);
    }
    if (t == 0) return;
    if ((load<u32>(t + 192) & 8u) == 0) {
        ctx.gpr[3] = 0;
        return;
    }
    const u32 m = t + 576;
    const u32 e = static_cast<u32>(call(ctx, FindComp29, {x, load<u32>(c + 24)}));
    if (e != 0 && static_cast<u32>(VCall(ctx, e, 32, {e})) == 4) {
        for (u32 row = 0; row < 4; ++row)
            for (u32 k = 0; k < 3; ++k) store<u32>(e + 32 + 16 * row + 4 * k, load<u32>(m + 16 * row + 4 * k));
        ctx.fpr[1] = Lf(m + 56);  // f1 = the last word copied (lfs / stfs)
    }
    for (u32 k = 0; k < 4; ++k) {
        const u32 e2 = static_cast<u32>(call(ctx, FindComp29, {x, load<u32>(c + 28 + 4 * k)}));
        if (e2 != 0 && static_cast<u32>(VCall(ctx, e2, 32, {e2})) == 2) {
            const u32 v = load<u8>(m + 64 + k);
            store<u32>(e2 + 20, v);
            ctx.gpr[3] = v;
        }
    }
}

// u32 f(Cmd* c, Effect* x): binds the material named c->+20 to the
// kind-7 component c->+12 (+24 the material, +20 the name). With a
// kind-5 component c->+16 whose record is a type-8 object (bits 10..15 of
// +4) every entry of each of its u8 +208->+112 lists (+208 + 8i + 132:
// {+48 data, +52 u16 count}, 8-byte items, +4 the material) named so is
// stored; without c->+16 the live AglMaterial (the free list made on
// first use, nodes walked from Obj_GetField4) named so and with a
// positive s16 +4 use count, else 0. r3 as the original leaves it.
DECOMP(00081400, AnimCmd_BindMaterial_00081400) {
    const u32 c = arg<u32>(ctx, 0);
    const u32 x = arg<u32>(ctx, 1);
    const u32 k = load<u32>(c + 12);
    if (k == 0) return;
    if (load<u32>(c + 20) == 0) {
        ctx.gpr[3] = 0;
        return;
    }
    const u32 e = static_cast<u32>(call(ctx, FindComp29, {x, k}));
    if (e == 0) return;
    if (static_cast<u32>(VCall(ctx, e, 32, {e})) != 7) return;
    const u32 k2 = load<u32>(c + 16);
    if (k2 == 0) {
        const u32 h = load<u32>(c + 20);
        u32 mat = 0;
        if (h != 0) mat = MatFind29(ctx, h);
        store<u32>(e + 24, mat);
        store<u32>(e + 20, load<u32>(c + 20));
        ctx.gpr[3] = mat;
        return;
    }
    const u32 e2 = static_cast<u32>(call(ctx, FindComp29, {x, k2}));
    if (e2 == 0) return;
    if (static_cast<u32>(VCall(ctx, e2, 32, {e2})) != 5) return;
    const u32 q = load<u32>(e2 + 28);
    if (q == 0) return;  // r3 = the kind (5)
    const u32 ty = (load<u32>(q + 4) >> 10) & 0x3Fu;
    if (ty != 8) {
        ctx.gpr[3] = ty;
        return;
    }
    const u32 d = load<u32>(q + 208);
    const u32 n = load<u8>(d + 112);
    for (u32 i = 0; i < n; ++i) {
        const u32 slot = d + 8 * i + 132;
        for (u32 j = 0;; ++j) {
            const u32 arr = load<u32>(slot);
            if (j >= load<u16>(arr + 52)) break;
            const u32 p = load<u32>(load<u32>(arr + 48) + 8 * j + 4);
            if (load<u32>(p + 40) == load<u32>(c + 20)) {
                const u32 h = load<u32>(c + 20);
                store<u32>(e + 24, p);
                store<u32>(e + 20, h);
                break;
            }
        }
    }
    ctx.gpr[3] = n;
}

// u32 f(Cmd* c, Effect* x): sends a message to the target (the kind-5
// component c->+12's record, GeomInstance_FindLiveByHash(+20) when it has
// none, else Obj_FindById(c->+12)) when bit 0x40000 of its +172 is set.
// Without bit 8 of +192 but with bit 2: vt[352](t, opd, 0, c->+24, f1 =
// c->+16). With bit 8: a 40-byte message (vtable 0x00B0DADC, +8 the mode
// c->+24, +16..+24 the direction *0x01164B40.., +32 / +36 = c->+16 / +20,
// u16 +14 bits 0xFF00 = 0x6000) goes to vt[344](t, &msg); for modes 0..2
// with the bound object 0x0027469E the direction is the horizontal one
// towards it, scaled by *0x00071920 / its length and by cos / sin of
// *0x00071924. r3 / f1 as the original leaves them.
DECOMP(00071928, AnimCmd_SendTargetMsg_00071928) {
    const u32 c = arg<u32>(ctx, 0);
    const u32 x = arg<u32>(ctx, 1);
    u32 t = 0;
    const u32 e = static_cast<u32>(call(ctx, FindComp29, {x, load<u32>(c + 12)}));
    if (e != 0 && static_cast<u32>(VCall(ctx, e, 32, {e})) == 5) {
        u32 q = load<u32>(e + 28);
        if (q == 0) q = static_cast<u32>(call(ctx, 0x005A1EBCu, {load<u32>(e + 20)}));  // GeomInstance_FindLiveByHash
        if (q != 0 && load<u32>(q + 276) == kTarget29) t = load<u32>(q + 280);
    }
    if (t == 0) {
        t = static_cast<u32>(call(ctx, FindById29, {load<u32>(c + 12)}));
        if (t == 0) return;
    }
    if ((load<u32>(t + 172) & 0x40000u) == 0) {
        ctx.gpr[3] = 0;
        return;
    }
    const u32 fl = load<u32>(t + 192);
    if ((fl & 8u) == 0) {
        if ((fl & 2u) == 0) {
            ctx.gpr[3] = 0;
            return;
        }
        const u32 opd = load<u32>(load<u32>(t) + 352);
        ctx.fpr[1] = Lf(c + 16);
        // r4 = the OPD, r7 = the entry (the original's call sequence).
        call_ptr(ctx, opd, {t, opd, 0, load<u32>(c + 24), load<u32>(opd)});
        return;
    }
    const u32 b = static_cast<u32>(call(ctx, Bind29, {x, 0x0027469Eu, 0}));
    u32 o = 0;
    if (b != 0 && load<u32>(b + 276) == kTarget29) o = load<u32>(b + 280);
    u32 w31 = load<u32>(0x01164B40u), w30 = load<u32>(0x01164B48u);
    f64 f1 = Lf(0x01164B44u);
    u32 w1 = load<u32>(0x01164B44u);
    const u32 mode = load<u32>(c + 24);
    if (o != 0 && mode <= 2) {
        const f64 c0 = Lf(0x0007191Cu);
        const f64 dx = Sgl28(Lf(o + 144) - Lf(t + 144));
        const f64 dz = Sgl28(Lf(o + 152) - Lf(t + 152));
        f64 s = Sgl28(c0 * c0);
        s = Sgl28(std::fma(dx, dx, s));
        s = Sgl28(std::fma(dz, dz, s));
        const f64 inv = Sgl28(Lf(0x00071920u) / Sgl28(std::sqrt(s)));
        const f64 ang = Lf(0x00071924u);
        callf(ctx, 0x007344E0u, {}, {ang});  // Math_Cosf
        const f64 cs = ctx.fpr[1];
        const f64 vx = Sgl28(Sgl28(dx * inv) * cs);
        const f64 vz = Sgl28(Sgl28(dz * inv) * cs);
        callf(ctx, 0x007345F0u, {}, {ang});  // Math_Sinf
        f1 = ctx.fpr[1];
        w31 = Bits28(vx);
        w30 = Bits28(vz);
        w1 = Bits28(f1);
    }
    Stack frame(ctx, 48);  // the message at 112(r1)..151(r1)
    const u32 m = frame.addr();
    const u32 bits = load<u16>(m + 14);
    store<u32>(m + 4, 0);
    store<u32>(m + 8, mode);
    store<u16>(m + 12, 0);
    store<u32>(m + 16, w31);
    store<u32>(m + 20, w1);
    store<u32>(m + 0, 0x00B0DADCu);
    store<u32>(m + 24, w30);
    store<u32>(m + 32, load<u32>(c + 16));
    store<u16>(m + 14, static_cast<u16>((bits & 0x00FFu) | 0x6000u));
    store<u32>(m + 36, load<u32>(c + 20));
    // The original calls through r3 = 0 here when bit 8 has meanwhile
    // been cleared (it is re-read); with it set the target.
    const u32 r = (load<u32>(t + 192) & 8u) != 0 ? t : 0;
    const u32 opd = load<u32>(load<u32>(r) + 344);
    call_ptr(ctx, opd, {r, m, opd, load<u32>(opd)});
    ctx.fpr[1] = f1;
}

// u32 f(Cmd* c, Effect* x) (AnimScriptCommandMapEntityToVar): fills an
// argument (Obj_Clear68_0049E860; +68 = the variable c->+12, u8 +72 the
// kind) and adds it to x (Effect_AddArg49F080(x, &arg, kind == 1 ? x : 0,
// 0, 0)) when the kind is set. Without c->+52 the first set word of
// c->+16..+48 gives kind 1..9 (stored at +0, at +4 for kinds 2 and 3);
// with it, the object named c->+52 (Obj_FindByHash_000DB290) under the
// target (CompTarget29 with key c->+16, else Obj_FindById(c->+20); failures
// String_Format the "cannot find" messages) gives kind 2 at +0.
// r3 as the original leaves it.
DECOMP(00080FC4, AnimCmd_MapEntityToVar_00080FC4) {
    const u32 c = arg<u32>(ctx, 0);
    const u32 x = arg<u32>(ctx, 1);
    Stack frame(ctx, 80);  // the argument at 112(r1)..191(r1)
    const u32 v = frame.addr();
    call(ctx, 0x0049E860u, {v});  // Obj_Clear68_0049E860
    const u32 var = load<u32>(c + 12);
    store<u32>(v + 68, var);
    if (var == 0) {
        ctx.gpr[3] = 0;
        return;
    }
    if (load<u32>(c + 52) == 0) {
        static constexpr u32 kSrc[9][3] = {{16, 1, 0}, {20, 2, 4}, {24, 3, 4}, {28, 4, 0}, {32, 5, 0},
                                           {36, 6, 0}, {40, 7, 0}, {44, 8, 0}, {48, 9, 0}};
        for (const auto& s : kSrc) {
            if (const u32 w = load<u32>(c + s[0]); w != 0) {
                store<u8>(v + 72, static_cast<u8>(s[1]));
                store<u32>(v + s[2], w);
                break;
            }
        }
    } else {
        u32 o = 0;
        if (load<u32>(c + 16) != 0) {
            o = CompTarget29(ctx, c, x, 16);
            if (o == 0) {
                const u32 hs = static_cast<u32>(call(ctx, HashStr29, {load<u32>(c + 16)}));
                call(ctx, Format29, {0x00AC4E4Cu, hs});
            }
        } else if (const u32 id = load<u32>(c + 20); id != 0) {
            o = static_cast<u32>(call(ctx, FindById29, {id}));
            if (o == 0) {
                const u32 hs = static_cast<u32>(call(ctx, HashStr29, {load<u32>(c + 20)}));
                call(ctx, Format29, {0x00AC4E7Cu, hs});
            }
        }
        if (o != 0) {
            const u32 r = static_cast<u32>(call(ctx, FindByHash29, {o, load<u32>(c + 52)}));
            if (r == 0) {
                call(ctx, HashStr29, {load<u32>(c + 52)});
                VCall(ctx, o, 76, {o});
            } else {
                store<u8>(v + 72, 2);
                store<u32>(v, r);
            }
        }
    }
    const u32 kind = load<u8>(v + 72);
    if (kind == 0) {
        ctx.gpr[3] = 0;
        return;
    }
    call(ctx, 0x0049F080u, {x, v, kind == 1 ? x : 0, 0, 0});  // Effect_AddArg49F080
}
