// Game code, part 43 (continued): the 92 and 93 instruction functions of
// the small-function sweep. Each was checked against the original in the
// PPC simulator (memory writes, calls and their arguments, r3 and f1).
#include "obj43.h"

using namespace sly4;
using namespace sly4::obj43;

// ---------------------------------------------------------------- sweep: 92 instructions
// u32 f(T* o, Actor* a): o->+308 = (a->vt[68]())->+28; AiCtrl_Bind(o, a);
// o->+60 = a when bit 3 of a->+192, else 0; c = o->+48: c->vt[96](c, 1);
// the u16 +40 of c->+164 gets bit 12 set and bit 10 cleared; o->+336 =
// 0, +340 = 20.0f, +368 = +376 = 0, bits 5-6 of the byte +416 cleared,
// +352..+360 = +176..+184; Obj_SetMode_00024048(o, 1) and o->vt[220](o,
// 2); r3 = its.
DECOMP(000240C0, Ai_Attach_000240C0) {
    const u32 o = arg<u32>(ctx, 0), a = arg<u32>(ctx, 1);
    const u32 x = static_cast<u32>(VCall(ctx, a, 68, {a}));
    store<u32>(o + 308, load<u32>(x + 28));
    call(ctx, 0x00011D08u, {o, a});
    const u32 c = load<u32>(o + 48);
    const u32 vt = load<u32>(c);
    const u32 who = (a != 0 && (load<u32>(a + 192) & 8u)) ? a : 0u;
    store<u32>(o + 60, who);
    VCallEO36(ctx, vt, 96, {c, 1});
    const u32 q = load<u32>(load<u32>(o + 48) + 164);
    if (q != 0) store<u16>(q + 40, static_cast<u16>((load<u16>(q + 40) | 0x1000u) & ~0x400u));
    const u8 b = load<u8>(o + 416);
    store<u32>(o + 336, 0);
    store<u32>(o + 340, 0x41A00000u);
    store<u32>(o + 368, 0);
    store<u32>(o + 376, 0);
    store<u8>(o + 416, static_cast<u8>(b & ~0x60u));
    store<u32>(o + 352, load<u32>(o + 176));
    store<u32>(o + 356, load<u32>(o + 180));
    store<u32>(o + 360, load<u32>(o + 184));
    ctx.fpr[1] = Lfs43(o + 184);
    call(ctx, 0x00024048u, {o, 1});
    ret(ctx, VCall(ctx, o, 220, {o, 2}));
}

// u32 f(Binding* b, T* e): the target t = the component b->+12 of e
// (Effect_FindComponent) whose vt[32]() is 5 -> its +28, when that has
// the type 0x02235C33 at +276, its +280; else Obj_FindById(b->+16) (0
// without a b->+16). b->+20 = t when bit 0 of t->+200, else 0; with it,
// byte b->+29 set: Obj_SetScale_001DB978(t, t, byte b->+28; b->+24); byte
// b->+30 set: (b->+20)->vt[44](). r3 as the original leaves it.
DECOMP(00080170, Binding_Resolve_00080170) {
    const u32 b = arg<u32>(ctx, 0), e = arg<u32>(ctx, 1);
    u32 t = 0;
    const u32 key = load<u32>(b + 12);
    if (key != 0) {
        const u32 c = static_cast<u32>(call(ctx, 0x004BB4B4u, {e, key}));
        if (c != 0 && static_cast<u32>(VCall(ctx, c, 32, {c})) == 5) {
            const u32 c2 = load<u32>(c + 28);
            if (c2 != 0 && load<u32>(c2 + 276) == 0x02235C33u) t = load<u32>(c2 + 280);
        }
    }
    if (t == 0) {
        const u32 id = load<u32>(b + 16);
        if (id != 0) t = static_cast<u32>(call(ctx, 0x000D6A28u, {id}));
    }
    const u32 s = (t != 0 && (load<u32>(t + 200) & 1u)) ? t : 0u;
    store<u32>(b + 20, s);
    if (s == 0) {
        ret(ctx, 0u);
        return;
    }
    if (load<u8>(b + 29) != 0) callf(ctx, 0x001DB978u, {s, s, load<u8>(b + 28)}, {Lfs43(b + 24)});
    const u32 on = load<u8>(b + 30);
    if (on == 0) {
        ret(ctx, on);
        return;
    }
    const u32 w = load<u32>(b + 20);
    VCall(ctx, w, 44, {w});
}

// u32 f(Ctl* c, x): t = Obj_ResolveTarget_0009A284(c, c->+12, c->+16, x)
// with bit 1 of t->+192 (else r3 = 0): with bit 31 of c->+28, t->+348 =
// max(t->+344, k); else c->+20 > k calls t->vt[316](c->+20), or c->+24 >=
// k sets t->+348 = min(c->+24, t->+344) when that is above k (else k);
// k = *0x0008AA70. Bit 30 of c->+28 then clears bits 4-11 and 17 of
// t->+376 (r3 = the word). f1 as the original leaves it.
DECOMP(0008AA78, Ctl_ApplyTarget_0008AA78) {
    const u32 c = arg<u32>(ctx, 0);
    const u32 t = static_cast<u32>(call(ctx, 0x0009A284u, {c, load<u32>(c + 12), load<u32>(c + 16), arg<u32>(ctx, 1)}));
    if (t == 0 || !(load<u32>(t + 192) & 2u)) {
        ret(ctx, 0u);
        return;
    }
    const u32 kw = load<u32>(0x0008AA70u);
    const f64 k = static_cast<f64>(std::bit_cast<f32>(kw));
    if (load<u32>(c + 28) & 0x80000000u) {
        const u32 w2 = load<u32>(t + 344);
        const f64 f2 = static_cast<f64>(std::bit_cast<f32>(w2));
        const bool gt = f2 > k;
        ctx.fpr[1] = gt ? f2 : k;
        store<u32>(t + 348, gt ? w2 : kw);
    } else {
        const f64 f2 = Lfs43(c + 20);
        ctx.fpr[1] = k;
        if (f2 > k) {
            ctx.fpr[1] = f2;
            VCall(ctx, t, 316, {t});
        } else {
            const u32 w3 = load<u32>(c + 24);
            const f64 f3 = static_cast<f64>(std::bit_cast<f32>(w3));
            if (f3 > k || f3 == k) {
                const u32 w4 = load<u32>(t + 344);
                const f64 f4 = static_cast<f64>(std::bit_cast<f32>(w4));
                const f64 m = f3 < f4 ? f3 : f4;
                if (m > k) {
                    const bool l = f3 < f4;
                    ctx.fpr[1] = l ? f3 : f4;
                    store<u32>(t + 348, l ? w3 : w4);
                } else {
                    store<u32>(t + 348, kw);
                }
            }
        }
    }
    if (!(load<u32>(c + 28) & 0x40000000u)) {
        ret(ctx, 0u);
        return;
    }
    const u32 w = load<u32>(t + 376) & ~(0x0F000000u | 0x00F00000u | 0x4000u);
    store<u32>(t + 376, w);
    ret(ctx, w);
}

// u32 f(Binding* b, T* e): the point p (sp+112) = the global vector
// 0x01164B40, or the +64 position of the target of the component b->+12
// of e (Effect_FindComponent, vt[32]() = 5, its +28); the object t = the
// +280 of the component b->+16's +28 when that has the type 0x02235C33 at
// +276 (else 0); then Obj_PokeInRange_0001B590(*0x00F557F0, t, b->+28,
// &p; b->+20, b->+24); r3 = its.
DECOMP(00092EE4, Binding_PokeInRange_00092EE4) {
    const u32 b = arg<u32>(ctx, 0), e = arg<u32>(ctx, 1);
    Stack fr(ctx, 16);  // the point at sp+112
    const u32 p = fr.addr();
    for (u32 k = 0; k < 12; k += 4) store<u32>(p + k, load<u32>(0x01164B40u + k));
    const u32 c = static_cast<u32>(call(ctx, 0x004BB4B4u, {e, load<u32>(b + 12)}));
    if (c != 0 && static_cast<u32>(VCall(ctx, c, 32, {c})) == 5) {
        const u32 c2 = load<u32>(c + 28);
        if (c2 != 0)
            for (u32 k = 0; k < 12; k += 4) store<u32>(p + k, load<u32>(c2 + 64 + k));
    }
    u32 t = 0;
    const u32 d = static_cast<u32>(call(ctx, 0x004BB4B4u, {e, load<u32>(b + 16)}));
    if (d != 0 && static_cast<u32>(VCall(ctx, d, 32, {d})) == 5) {
        const u32 d2 = load<u32>(d + 28);
        if (d2 != 0 && load<u32>(d2 + 276) == 0x02235C33u) t = load<u32>(d2 + 280);
    }
    callf(ctx, 0x0001B590u, {load<u32>(0x00F557F0u), t, load<u32>(b + 28), p}, {Lfs43(b + 20), Lfs43(b + 24)});  // r3 = its
}

// u32 f(T* o): StateMachine_Init(o); o->+204 (when 0 / -1: a new
// World_Counter_004A2560()) goes to o->vt[104](o, id); each entry of the
// vector o->+248 (u16 count +8) gets vt[20](entry, o);
// Obj_PlaceAttachments(o); then with a body o->+348 and either o->+216->
// +184 or o->vt[68](), HkBody_AddListener91A82C(body, o + 344) and bit 29
// of o->+376 cleared (r3 = the word). r3 as the original leaves it.
DECOMP(000DE56C, Obj_Activate_000DE56C) {
    const u32 o = arg<u32>(ctx, 0);
    call(ctx, 0x00464B18u, {o});
    u32 id = load<u32>(o + 204);
    if (id == 0 || id == 0xFFFFFFFFu) {
        id = static_cast<u32>(call(ctx, 0x004A2560u, {}));
        store<u32>(o + 204, id);
    }
    VCall(ctx, o, 104, {o, id});
    for (u32 i = 0;; ++i) {
        const u32 v = load<u32>(o + 248);
        const u32 n = v != 0 ? load<u16>(v + 8) : 0u;
        if (!(i < n)) break;
        const u32 x = load<u32>(load<u32>(v) + 4 * i);
        VCall(ctx, x, 20, {x, o});
    }
    call(ctx, 0x000D7A00u, {o});
    u32 body = load<u32>(o + 348);
    if (body == 0) {
        ret(ctx, 0u);
        return;
    }
    if (load<u32>(load<u32>(o + 216) + 184) == 0) {
        const u32 r = static_cast<u32>(VCall(ctx, o, 68, {o}));
        if (r == 0) {
            ret(ctx, 0u);
            return;
        }
        body = load<u32>(o + 348);
    }
    call(ctx, 0x0091A82Cu, {body, o + 344});
    const u32 w = load<u32>(o + 376) & ~0x20000000u;
    store<u32>(o + 376, w);
    ret(ctx, w);
}

// void f(T* o, Hit* h): with q = o->+216 and its +212 / +220 above k =
// *0x0016F560 and h->+48 above q->+216: s = min((h+48 - q+216) * q+212,
// q+220) goes to o->vt[352](o, s, h->+4, 0x80F43146); bit 8 of the u16
// h->+14 = (that returned 0); then o->+736 = the heading atan2(h+20 -
// o+148, h+16 - o+144) * *0x0016F564 / *0x0016F568 wrapped once into
// [k, *0x0016F56C]. r3 / f1 as the original leaves them.
DECOMP(0016F570, Obj_HitHeading_0016F570) {
    const u32 o = arg<u32>(ctx, 0), h = arg<u32>(ctx, 1);
    const f64 k = Lfs43(0x0016F560u);
    const u32 q = load<u32>(o + 216);
    const u32 vt = load<u32>(o);
    const f64 a = Lfs43(q + 212);
    if (!(a > k)) {
        ret(ctx, vt);
        return;
    }
    const f64 cap = Lfs43(q + 220);
    if (!(cap > k)) {
        ret(ctx, vt);
        return;
    }
    const f64 y = Lfs43(h + 48), base = Lfs43(q + 216);
    ctx.fpr[1] = base;
    if (!(y > base)) {
        ret(ctx, vt);
        return;
    }
    f64 s = F43(F43(y - base) * a);
    ctx.fpr[1] = s < cap ? s : cap;
    const u32 r = static_cast<u32>(VCallOpdR4_28(ctx, o, 352, load<u32>(h + 4), 0x80F43146u));
    const u32 fl = (static_cast<u32>(load<u16>(h + 14)) & ~0x100u) | (r == 0 ? 0x100u : 0u);
    const f64 hx = Lfs43(h + 16), hy = Lfs43(h + 20);
    store<u16>(h + 14, static_cast<u16>(fl));
    const f64 dx = F43(hx - Lfs43(o + 144));
    const f64 dy = F43(hy - Lfs43(o + 148));
    f64 ang = callf(ctx, 0x005E665Cu, {}, {dy, dx});
    ang = F43(F43(ang * Lfs43(0x0016F564u)) / Lfs43(0x0016F568u));
    const f64 wrap = Lfs43(0x0016F56Cu);
    if (ang < k) ang = F43(ang + wrap);
    if (ang > wrap) ang = F43(ang - wrap);
    store<u32>(o + 736, W43(ang));
    ctx.fpr[1] = ang;
    ret(ctx, 0x0016F560u);  // the constants address left in r3
}

// bool f(T* o, src, s32 kind, f32* amt): only with o->+392 = 1 (else 0):
// kind 0 is refused (0) for bit 31 of o->+216->+256 or src = o->+404,
// else *amt *= *0x0017A000; kind 4 is refused unless o->+388 has
// Comp_GetF32_922918(+388 + 224) above k = *0x0017A004, o->+216->+196 is
// not above k and bit 16 of o->+188 is clear; kinds 1-2, above 4 and
// negative ones are refused. The rest goes to Obj_CanTakeHitBase_000DF960(o, src,
// kind, amt); r3 = its. f1 as the original leaves it.
DECOMP(0017A008, Obj_CanTakeHit_0017A008) {
    const u32 o = arg<u32>(ctx, 0), src = arg<u32>(ctx, 1), amt = arg<u32>(ctx, 3);
    const s32 kind = static_cast<s32>(arg<u32>(ctx, 2));
    const u32 q = load<u32>(o + 216);
    if (load<u32>(o + 392) != 1) {
        ret(ctx, 0u);
        return;
    }
    if (kind > 2) {
        if (kind > 4) {
            ret(ctx, 0u);
            return;
        }
        if (kind == 4) {
            const u32 c = load<u32>(o + 388);
            if (c == 0) {
                ret(ctx, 0u);
                return;
            }
            const f64 v = callf(ctx, 0x00922918u, {c + 224}, {});
            const f64 k = Lfs43(0x0017A004u);
            ctx.fpr[1] = k;
            if (v < k || v == k || Lfs43(q + 196) > k || (load<u32>(o + 188) & 0x10000u)) {
                ret(ctx, 0u);
                return;
            }
        }
    } else {
        if (kind != 0 || (load<u32>(q + 256) & 0x80000000u) || (src != 0 && load<u32>(o + 404) == src)) {
            ret(ctx, 0u);
            return;
        }
        const f64 m = F43(Lfs43(amt) * Lfs43(0x0017A000u));
        store<u32>(amt, W43(m));
        ctx.fpr[1] = m;
    }
    ret(ctx, call(ctx, 0x000DF960u, {o, src, static_cast<u32>(kind), amt}));
}

// u32 f(T* o): with an o->+856 and no object 0x366FD481 (Obj_FindById),
// the effect EffectDef_Find(o->+920, 0, 1) is built (SmallAlloc(140),
// Effect_Construct); its component 0x0027469E whose vt[32]() is 5 gets
// +28 = o->+856->+160 (the old one's vt[8]() called, the new one's u16 +4
// counted up); bit 9 of the effect's +136 set and Effect_Start(effect); r3
// = its. (A failed allocation goes on with a null effect.)
DECOMP(001F0694, Obj_StartEffect_001F0694) {
    const u32 o = arg<u32>(ctx, 0);
    if (load<u32>(o + 856) == 0) {
        ret(ctx, 0u);
        return;
    }
    if (call(ctx, 0x000D6A28u, {0x366FD481u}) != 0) return;
    const u32 d = static_cast<u32>(call(ctx, 0x004BDEDCu, {load<u32>(o + 920), 0, 1}));
    if (d == 0) {
        ret(ctx, 0u);
        return;
    }
    u32 eff = static_cast<u32>(call(ctx, 0x0055238Cu, {140}));
    if (eff != 0) call(ctx, 0x004C7A68u, {eff, d});
    const u32 t = load<u32>(load<u32>(o + 856) + 160);
    const u32 c = static_cast<u32>(call(ctx, 0x004BB4B4u, {eff, 0x0027469Eu}));
    if (c != 0 && static_cast<u32>(VCall(ctx, c, 32, {c})) == 5) {
        const u32 old = load<u32>(c + 28);
        if (old != 0) VCall(ctx, old, 8, {old});
        store<u32>(c + 28, t);
        if (t != 0) store<u16>(t + 4, static_cast<u16>(load<u16>(t + 4) + 1));
    }
    store<u32>(eff + 136, load<u32>(eff + 136) | 0x200u);  // eff may be null (quirk)
    ret(ctx, call(ctx, 0x004C753Cu, {eff}));
}

// void T(T* o, const Def* d): the base Thunk_000E40B4(o, d); the vtables
// 0x00B18908 (+380 0x00B18AB4, +384 0x00B18ADC); the four 3-float rows of
// 0x01164BD0 (16 apart) into +432 / +448 / +464 / +480; from d: +504..
// +512 = +256..+264, +528..+536 = +224..+232, +544..+556 = +240..+252,
// +516 = +268, +560 = +208, +496 = +500 = 0. With the byte d->+272:
// 0x001F50E4(o) and StateMachine_SetState(o, 0, 0). Then
// Obj_SetFlagHash_0048FA54(o, 0x6927A078, 1); r3 = its.
DECOMP(001F54E8, Obj_Construct_001F54E8) {
    const u32 o = arg<u32>(ctx, 0), d = arg<u32>(ctx, 1);
    call(ctx, 0x000E40B4u, {o, d});
    store<u32>(o + 380, 0x00B18AB4u);
    store<u32>(o + 384, 0x00B18ADCu);
    for (u32 r = 0; r < 4; ++r)
        for (u32 k = 0; k < 3; ++k) store<u32>(o + 432 + 16 * r + 4 * k, load<u32>(0x01164BD0u + 16 * r + 4 * k));
    store<u32>(o, 0x00B18908u);
    store<u32>(o + 504, load<u32>(d + 256));
    store<u32>(o + 508, load<u32>(d + 260));
    store<u32>(o + 512, load<u32>(d + 264));
    store<u32>(o + 528, load<u32>(d + 224));
    store<u32>(o + 496, 0);
    store<u32>(o + 532, load<u32>(d + 228));
    const u32 w268 = load<u32>(d + 268);
    store<u32>(o + 536, load<u32>(d + 232));
    store<u32>(o + 500, 0);
    store<u32>(o + 544, load<u32>(d + 240));
    store<u32>(o + 516, w268);
    store<u32>(o + 548, load<u32>(d + 244));
    const u32 w208 = load<u32>(d + 208);
    store<u32>(o + 552, load<u32>(d + 248));
    const u32 on = load<u8>(d + 272);
    store<u32>(o + 556, load<u32>(d + 252));
    store<u32>(o + 560, w208);
    ctx.fpr[1] = Lfs43(d + 252);
    if (on != 0) {
        call(ctx, 0x001F50E4u, {o});
        call(ctx, 0x00464D1Cu, {o, 0, 0});
    }
    ret(ctx, call(ctx, 0x0048FA54u, {o, 0x6927A078u, 1}));
}

// u32 f(T* o, u8 on): switching on first drops the two attachments +396
// / +392 (vt[40](x, 0), slots cleared), removes the skeleton o->+388's
// entity +8 (Hk_RemoveEntity92FFC8(&tmp, +8, skel)), releases each bone
// (Skel_BoneAt_0091A7D8 for i < Skel_BoneCount_0091A2CC, AnimSkel_Release
// 7D4EA8), its properties 0xA734004D / 0x91C7CF5B (Hk_RemoveProperty
// 93FED8) and the skeleton itself. Then Obj_SetRagdollBase(o, on); r3 =
// its.
DECOMP(00204B74, Obj_SetRagdoll_00204B74) {
    const u32 o = arg<u32>(ctx, 0), on = arg<u32>(ctx, 1) & 0xFFu;
    if (on != 0) {
        const u32 a = load<u32>(o + 396);
        if (a != 0) VCall(ctx, a, 40, {a, 0});
        store<u32>(o + 396, 0);
        const u32 b = load<u32>(o + 392);
        if (b != 0) VCall(ctx, b, 40, {b, 0});
        store<u32>(o + 392, 0);
        Stack fr(ctx, 24);  // the results at sp+112 / sp+120 / sp+128
        const u32 sk = load<u32>(o + 388);
        const u32 ent = load<u32>(sk + 8);
        if (ent != 0) call(ctx, 0x0092FFC8u, {fr.addr() + 16, ent, sk});
        for (s32 i = 0; i < static_cast<s32>(call(ctx, 0x0091A2CCu, {load<u32>(o + 388)})); ++i) {
            const u32 bone = static_cast<u32>(call(ctx, 0x0091A7D8u, {load<u32>(o + 388), static_cast<u32>(i)}));
            call(ctx, 0x007D4EA8u, {bone});
        }
        call(ctx, 0x0093FED8u, {fr.addr(), load<u32>(o + 388), 0xA734004Du});
        call(ctx, 0x0093FED8u, {fr.addr() + 8, load<u32>(o + 388), 0x91C7CF5Bu});
        call(ctx, 0x007D4EA8u, {load<u32>(o + 388)});
    }
    ret(ctx, call(ctx, 0x000E02F8u, {o, on}));
}

// Removes o from the registry array 0x01056F1C ({key, T*} pairs, the
// later ones moved down; the count and data read once for the search).
static void RegistryRemove43(u32 o) {
    const u32 ar = 0x01056F1Cu;
    const u32 cnt = load<u16>(ar + 2);
    const u32 data = load<u32>(ar + 8);
    for (u32 i = 0; i < cnt; ++i) {
        if (load<u32>(data + 8 * i + 4) != o) continue;
        u32 c = cnt;
        if (static_cast<s32>(c) >= 2 && static_cast<s32>(i) < static_cast<s32>(c - 1)) {
            for (u32 j = i;;) {
                const u32 e = data + 8 * j;
                ++j;
                store<u32>(e, load<u32>(e + 8));
                c = load<u16>(ar + 2);
                store<u32>(e + 4, load<u32>(e + 12));
                if (!(static_cast<s32>(j) < static_cast<s32>(c - 1))) break;
            }
        }
        store<u16>(ar + 2, static_cast<u16>(c - 1));
        return;
    }
}

// The deleting ~T(): vtable 0x00B1CE78, the PtrVectors +44 / +24
// destroyed, vtable 0x00B1BE30, o removed from the registry array
// 0x01056F1C, List1013B5C_Remove(o) and SmallFree(o, 72); r3 = its (0 for
// a null o).
DECOMP(002B88F0, Registered_Delete_002B88F0) {
    const u32 o = arg<u32>(ctx, 0);
    if (o == 0) {
        ret(ctx, 0u);
        return;
    }
    store<u32>(o, 0x00B1CE78u);
    VecDtor(ctx, o + 44);
    VecDtor(ctx, o + 24);
    store<u32>(o, 0x00B1BE30u);
    RegistryRemove43(o);
    call(ctx, 0x000D6940u, {o});
    ret(ctx, call(ctx, 0x00552420u, {o, 72}));
}

// u32 f(T* a, Obj* p): plays a->+60's anim on p with
// Obj_SetAnim(p, name, 1, -, 0, 0, 0x01164CA4; *0x002E5C60) (none: r3 =
// a): the name is
// String_Format("%s" 0x00AD8B78, a->+60) without the suffix *0x010682A4,
// String_Format(0x00AD8B80, a->+60, suffix) with it unless p->+8->+152 >
// 1, in which case the plain name is played and Player_Set3632C8(p,
// String_Format(0x00AD8B78, suffix), 1) follows. r3 = the last call's.
DECOMP(002E5C68, Obj_PlayNamedAnim_002E5C68) {
    const u32 a = arg<u32>(ctx, 0), p = arg<u32>(ctx, 1);
    if (p == 0) return;  // r3 = a
    const u32 name = load<u32>(a + 60);
    const u32 suf = load<u32>(0x010682A4u);
    const f64 t = Lfs43(0x002E5C60u);
    if (suf == 0) {
        // r5 still holds 0x00AE0000 from building the format address
        const u32 s = static_cast<u32>(call(ctx, 0x005534D0u, {0x00AD8B78u, name, 0x00AE0000u}));
        callf(ctx, 0x00363A3Cu, {p, s, 1, ctx.gpr[6], 0, 0, 0x01164CA4u}, {t});
        return;
    }
    const u32 q = load<u32>(p + 8);
    if (q != 0 && load<u32>(q + 152) > 1) {
        const u32 s = static_cast<u32>(call(ctx, 0x005534D0u, {0x00AD8B78u, name, 0x00AE0000u}));
        callf(ctx, 0x00363A3Cu, {p, s, 1, ctx.gpr[6], 0, 0, 0x01164CA4u}, {t});
        const u32 s2 = static_cast<u32>(call(ctx, 0x005534D0u, {0x00AD8B78u, suf}));
        call(ctx, 0x003632C8u, {p, s2, 1});
        return;
    }
    const u32 s = static_cast<u32>(call(ctx, 0x005534D0u, {0x00AD8B80u, name, suf}));
    callf(ctx, 0x00363A3Cu, {p, s, 1, ctx.gpr[6], 0, 0, 0x01164CA4u}, {t});
}

// u32 f(T* o, Obj* x): for a non-null x (else r3 = o), steps the cursor of
// 0x002EF36C (o->+16 times from entry o->+8, entries o->+28) calling
// entry->+8->vt[24](x->+160) at each step. r3 = the last cursor (0 when
// o->+16 is 0).
DECOMP(002EF648, Cursor_StepCallsArg_002EF648) {
    const u32 o = arg<u32>(ctx, 0), x = arg<u32>(ctx, 1);
    if (x == 0) return;
    const u32 n = load<u32>(o + 16);
    const u32 a = load<u32>(x + 160);
    if (n == 0) {
        ret(ctx, 0u);
        return;
    }
    const u32 idx = load<u32>(o + 8);
    u32 c = (idx << 16) | ((load<u32>(load<u32>(o + 28) + (idx << 4) + 12) >> 15) & 0xFFFFu);
    for (u32 k = 0;;) {
        const u32 cur = c >> 16;
        const u32 obj = load<u32>(load<u32>(o + 28) + (cur << 4) + 8);
        VCall(ctx, obj, 24, {obj, a});
        if (static_cast<s32>(cur) != static_cast<s32>(load<u32>(o + 12))) {
            const u32 arr = load<u32>(o + 28);
            const u32 nx = load<u32>(arr + (cur << 4) + 4);
            c = (nx << 16) | ((load<u32>(arr + (nx << 4) + 12) >> 15) & 0xFFFFu);
        } else {
            c = 0;
        }
        if (!(++k < n)) break;
    }
    ret(ctx, c);
}

// u32 f(T* o, u32 k): switches o to the slot k (< 12, set at o->+704 +
// 4k; 255, the byte 0x01068C98 set or an empty slot leave it): the
// current slot c = o->vt[32](o, 0), when valid and set, gets
// slot->vt[44](slot, k); o->vt[28](o, 0, u8 k) and the new slot's
// vt[40](); r3 = its. Other exits leave r3.
DECOMP(0030D2B8, Obj_SwitchSlot_0030D2B8) {
    const u32 o = arg<u32>(ctx, 0), k = arg<u32>(ctx, 1);
    if (k == 255) return;  // r3 = o
    const u32 gate = load<u8>(0x01068C98u);
    if (gate != 0 || !(k < 12)) {
        ret(ctx, gate);
        return;
    }
    const u32 sl = o + 704 + 4 * k;
    const u32 s0 = load<u32>(sl);
    if (s0 == 0) {
        ret(ctx, s0);
        return;
    }
    const u32 cur = static_cast<u32>(VCall(ctx, o, 32, {o, 0}));
    if (cur < 12) {
        const u32 cs = load<u32>(o + 704 + 4 * cur);
        if (cs != 0) VCall(ctx, cs, 44, {cs, static_cast<u64>(static_cast<s64>(static_cast<s32>(k)))});
    }
    VCall(ctx, o, 28, {o, 0, k & 0xFFu});
    const u32 ns = load<u32>(sl);
    ret(ctx, VCall(ctx, ns, 40, {ns}));
}

// bool f(FileReader* r): loads a packfile from r (a local SkelHolder at
// sp+120 over r with 4096 bytes, a stream at sp+116, Hk_Load802E08(&holder,
// &out)); the stream freed and the holder destroyed (its skeleton +8
// released, vtables 0x00B1F480 / 0x00B13974); a loaded resource goes to
// loader(*g, *(g + 4), res, Iff_ChunkSize(r)) and is released. *g = 0; r3
// = 1. (0x0034B9A8 / 0x0034BC7C)
static void LoadPack43(PPUContext& ctx, u32 g, u32 loader) {
    const u32 rd = arg<u32>(ctx, 0);
    const u32 g0 = load<u32>(g), g1 = load<u32>(g + 4);
    const u32 n = static_cast<u32>(call(ctx, 0x005637D4u, {rd}));
    Stack fr(ctx, 24);  // sp+112 out, sp+116 stream, sp+120 holder (16 bytes)
    const u32 out = fr.addr(), st = out + 4, h = out + 8;
    call(ctx, 0x0034640Cu, {h, rd, 4096});
    store<u32>(out, 0);
    call(ctx, 0x007CE648u, {st});
    // r5 / r6 carry a zeroed 16-byte record (its last word unwritten);
    // Hk_Load802E08 reads only r3 / r4.
    const u32 res = static_cast<u32>(call(ctx, 0x00802E08u, {h, out, 0, 0}));
    call(ctx, 0x007CEA58u, {st});
    const u32 sk = load<u32>(h + 8);
    if (sk != 0) {
        store<u32>(h, 0x00B1F480u);
        call(ctx, 0x007D4EA8u, {sk});
        store<u32>(h + 8, 0);
    }
    store<u32>(h, 0x00B13974u);
    if (res != 0) {
        call(ctx, loader, {g0, g1, res, n});
        call(ctx, 0x007D4EA8u, {res});
    }
    store<u32>(g, 0);
    ret(ctx, 1u);
}
// The anim pack: globals 0x0106BFA4, Anim_Load34A5E4.
DECOMP(0034B9A8, Anim_LoadPack_0034B9A8) { LoadPack43(ctx, 0x0106BFA4u, 0x0034A5E4u); }

// The level pack: globals 0x0106BFB4, Level_Load34AB90.
DECOMP(0034BC7C, Level_LoadPack_0034BC7C) { LoadPack43(ctx, 0x0106BFB4u, 0x0034AB90u); }

// bool f(T* o, u32 name): for a non-null o not yet in the list 0x0106C280
// (a PtrVector of records whose +168 is the owner; found: o->+160->
// vt[12](), r3 = its), h = Hashed_Make(name, else o->+216->+52, else 0);
// with it bit 22 of o->+188 is set, h pushed on the list (grown with
// PtrVector_Reserve to twice its capacity when full; +10 high-water
// mark), Obj_AddToList36341C(h), h->+168 = o and bit 7 of the byte
// h->+172 set; r3 = 1. Otherwise r3 = 0.
DECOMP(00369910, Obj_RegisterHashed_00369910) {
    const u32 o = arg<u32>(ctx, 0), name = arg<u32>(ctx, 1);
    if (o == 0) {
        ret(ctx, 0u);
        return;
    }
    const u32 v = 0x0106C280u;
    const u32 data0 = load<u32>(v);
    const u32 n0 = load<u16>(v + 8);
    for (u32 i = 0; i < n0; ++i) {
        if (load<u32>(load<u32>(data0 + 4 * i) + 168) == o) {
            const u32 x = load<u32>(o + 160);
            ret(ctx, VCall(ctx, x, 12, {x}));
            return;
        }
    }
    u32 nm = name;
    if (nm == 0) {
        const u32 q = load<u32>(o + 216);
        if (q != 0) nm = load<u32>(q + 52);
    }
    const u32 h = static_cast<u32>(call(ctx, 0x003651F0u, {nm}));
    if (h == 0) {
        ret(ctx, 0u);
        return;
    }
    store<u64>(o + 184, load<u64>(o + 184) | 0x400000u);
    u32 cnt = load<u16>(v + 8);
    const u32 cap = load<u16>(v + 4);
    if (cnt == cap) {
        call(ctx, 0x00551718u, {v, (cap << 1) & 0x1FFFEu});
        cnt = load<u16>(v + 8);
    }
    const u32 data = load<u32>(v);
    const u32 c1 = static_cast<u32>(load<u16>(v + 8)) + 1;
    const u32 hw = load<u16>(v + 10);
    store<u32>(data + ((cnt << 2) & 0x3FFFCu), h);
    store<u16>(v + 8, static_cast<u16>(c1));
    store<u16>(v + 10, static_cast<u16>(static_cast<s32>(hw) > static_cast<s32>(c1 & 0xFFFFu) ? hw : c1));
    call(ctx, 0x0036341Cu, {h});
    const u8 b = load<u8>(h + 172);
    store<u32>(h + 168, o);
    store<u8>(h + 172, static_cast<u8>(b | 0x80u));
    ret(ctx, 1u);
}

// ~T(T* o) body: vtable 0x00B1F720, the buffer +184 freed (Mem_FreeFrom(
// p, 0)) and cleared; World_ListRemove_0046B7A4(*0x00F65CA0, 9,
// 0x00F378B8, o, 0); vtable 0x00B1F678; the effect +20 released
// (Effect_Release(e, 0x10CBB)) and bit 9 of its +136 set; the object +24
// (type 0x7DF841DC: +276 / +280 cleared first) gets vt[40](0, 1) and
// vt[8](); the object +28 vt[4]() and +28 = 0; then
// VtB21B58_DtorTrivial_004A0F70(o + 48); r3 = its.
DECOMP(0036C400, Obj_Dtor_0036C400) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 buf = load<u32>(o + 184);
    store<u32>(o, 0x00B1F720u);
    if (buf != 0) {
        call(ctx, 0x0054EE6Cu, {buf, 0});
        store<u32>(o + 184, 0);
    }
    call(ctx, 0x0046B7A4u, {load<u32>(0x00F65CA0u), 9, 0x00F378B8u, o, 0});
    const u32 e = load<u32>(o + 20);
    store<u32>(o, 0x00B1F678u);
    if (e != 0) {
        call(ctx, 0x004CF3C4u, {e, 0x10CBBu});
        const u32 e2 = load<u32>(o + 20);
        store<u32>(e2 + 136, load<u32>(e2 + 136) | 0x200u);
    }
    const u32 c = load<u32>(o + 24);
    if (c != 0) {
        if (load<u32>(c + 276) == 0x7DF841DCu) {
            store<u32>(c + 276, 0);
            store<u32>(c + 280, 0);
        }
        VCall(ctx, c, 40, {c, 0, 1});
        const u32 c2 = load<u32>(o + 24);
        VCall(ctx, c2, 8, {c2});
    }
    const u32 d = load<u32>(o + 28);
    if (d != 0) {
        VCall(ctx, d, 4, {d});
        store<u32>(o + 28, 0);
    }
    ret(ctx, call(ctx, 0x004A0F70u, {o + 48}));
}

// u32 f(T* o, u8 on): o->+24->vt[40](on, 1); Obj_SyncParts_0036D9E4(o);
// o->+208->vt[40](k, 1) with k = bit 7 of the byte o->+192 when bit 15 of
// o->+24->+164 is set (else 0); with that bit and an effect o->+20,
// Effect_SetHash4CF988(e, bit 5 of +192 ? 0xDBDDDABD : 0x009BD94D);
// switching off, Effect_SetHash4CF988(e, 0x95B0708E). r3 as the original
// leaves it.
DECOMP(0036DAFC, Obj_SetLit_0036DAFC) {
    const u32 o = arg<u32>(ctx, 0), on = arg<u32>(ctx, 1) & 0xFFu;
    const u32 a = load<u32>(o + 24);
    if (a != 0) VCall(ctx, a, 40, {a, on, 1});
    call(ctx, 0x0036D9E4u, {o});
    const u32 b = load<u32>(o + 208);
    u64 r = load<u8>(o + 192);
    if (b != 0) {
        u32 k = 0;
        const u32 a2 = load<u32>(o + 24);
        if (a2 != 0 && (load<u32>(a2 + 164) & 0x8000u) && (r & 0x80u)) k = 1;
        VCall(ctx, b, 40, {b, k, 1});
        r = load<u8>(o + 192);
    }
    const u32 a3 = load<u32>(o + 24);
    if (a3 != 0 && (load<u32>(a3 + 164) & 0x8000u)) {
        const u32 e = load<u32>(o + 20);
        if (e != 0) r = call(ctx, 0x004CF988u, {e, (r & 0x20u) ? 0xDBDDDABDu : 0x009BD94Du});
    }
    if (on == 0) {
        const u32 e = load<u32>(o + 20);
        if (e != 0) r = call(ctx, 0x004CF988u, {e, 0x95B0708Eu});
    }
    ctx.gpr[3] = r;
}

// void T(T* o): two arrays with the default allocator (Mem_DefaultAllocator):
// {u16 7, 0, 0, 16 (8-byte elements), data, alloc} at +0 and {38, 0, 0,
// 16, data, alloc} at +24, each buffer from alloc->vt[0](alloc, n * 8, 0,
// 16) and Array_Init; +20 / +44 = 0x00F2E998, +16 / +40 = 0;
// *0x00F65788 = o; then 0x0038E1D4(o); r3 = its.
DECOMP(0038E468, Tables_Construct_0038E468) {
    const u32 o = arg<u32>(ctx, 0);
    store<u16>(o, 7);
    store<u16>(o + 2, 0);
    store<u16>(o + 4, 0);
    store<u16>(o + 6, 16);
    store<u32>(o + 8, 0);
    const u32 al = static_cast<u32>(call(ctx, 0x00550EE8u, {}));
    store<u32>(o + 12, al);
    const u32 n0 = load<u16>(o);
    if (n0 != 0) {
        const u32 sz = n0 * ((static_cast<u32>(load<u16>(o + 6)) >> 1) & 0x7FFFu);
        store<u32>(o + 8, static_cast<u32>(VCallEO36(ctx, load<u32>(al), 0, {al, sz, 0, 16})));
    }
    call(ctx, 0x00558F38u, {o});
    store<u32>(o + 20, 0x00F2E998u);
    store<u16>(o + 30, 16);
    store<u16>(o + 16, 0);
    store<u16>(o + 24, 38);
    store<u16>(o + 26, 0);
    store<u16>(o + 28, 0);
    store<u32>(o + 32, 0);
    const u32 al2 = static_cast<u32>(call(ctx, 0x00550EE8u, {}));
    store<u32>(o + 36, al2);
    const u32 n1 = load<u16>(o + 24);
    if (n1 != 0) {
        const u32 sz = n1 * ((static_cast<u32>(load<u16>(o + 30)) >> 1) & 0x7FFFu);
        store<u32>(o + 32, static_cast<u32>(VCallEO36(ctx, load<u32>(al2), 0, {al2, sz, 0, 16})));
    }
    call(ctx, 0x00558F38u, {o + 24});
    store<u32>(o + 44, 0x00F2E998u);
    store<u16>(o + 40, 0);
    store<u32>(0x00F65788u, o);
    ret(ctx, call(ctx, 0x0038E1D4u, {o}));
}

// u32 f(T* o): unless *0x01089EE8 > o->+576 (signed): the first of the six
// ids at 0x00ADF900 equal to *(G + 0x128114) (G = *0x00F65CA0) sets its
// bit in the u16 mask +320 of (G + 80 + 4056 * byte G+4208); when that
// changed the mask and its bit count is *0x01089F34,
// Trophy_Award0B27B4(0x152D006A). r3 as the original leaves it.
DECOMP(003CE128, Trophy_CheckSet_003CE128) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 g = load<u32>(0x00F65CA0u);
    const u32 rec = g + static_cast<u32>(load<u8>(g + 4208)) * 4056u + 80;
    const u32 old = load<u16>(rec + 320);
    if (static_cast<s32>(load<u32>(0x01089EE8u)) > static_cast<s32>(load<u32>(o + 576))) {
        ret(ctx, g);
        return;
    }
    u32 r = g;
    for (u32 i = 0; i < 6; ++i) {
        if (load<u32>(0x00ADF900u + 4 * i) == load<u32>(g + 0x128114u)) {
            r = static_cast<u32>(load<u16>(rec + 320)) | (1u << i);
            store<u16>(rec + 320, static_cast<u16>(r));
            break;
        }
    }
    const u32 m = load<u16>(rec + 320);
    if (old == m) {
        ret(ctx, r);
        return;
    }
    const u32 need = load<u32>(0x01089F34u);
    if (need != static_cast<u32>(std::popcount(m))) {
        ret(ctx, need);
        return;
    }
    ret(ctx, call(ctx, 0x000B27B4u, {0x152D006Au}));
}

// u32 f(T* o): byte o->+85 = whether the segment {o+48, o+68} - {o+64,
// o+52} projects (View_ProjectPoint on (G + 0x128168)'s view, G =
// *0x00F65CA0) with both screen x or both y below *0x003ECBA8 or both
// above *0x003ECBAC. r3 / f1 as the original leaves them.
DECOMP(003ECBB0, Seg_OffScreen_003ECBB0) {
    const u32 o = arg<u32>(ctx, 0);
    Stack fr(ctx, 64);  // the points sp+112 / sp+128, the results sp+144 / sp+160
    const u32 p0 = fr.addr(), p1 = p0 + 16, s0 = p0 + 32, s1 = p0 + 48;
    store<u8>(o + 85, 0);
    store<u32>(p0, load<u32>(o + 48));
    store<u32>(p0 + 4, load<u32>(o + 68));
    store<u32>(p0 + 8, 0);
    store<u32>(p1, load<u32>(o + 64));
    store<u32>(p1 + 4, load<u32>(o + 52));
    store<u32>(p1 + 8, 0);
    const u32 view = load<u32>(load<u32>(0x00F65CA0u) + 0x128168u);
    call(ctx, 0x00522750u, {view, s0, p0});
    call(ctx, 0x00522750u, {view, s1, p1});
    const f64 lo = Lfs43(0x003ECBA8u), hi = Lfs43(0x003ECBACu);
    const f64 x0 = Lfs43(s0), x1 = Lfs43(s1), y0 = Lfs43(s0 + 4), y1 = Lfs43(s1 + 4);
    u32 r = 0x003ECBA8u;
    if (x0 < lo && x1 < lo) store<u8>(o + 85, 1);
    if (x0 > hi && x1 > hi) {
        store<u8>(o + 85, 1);
        r = 1;
    }
    if (y0 < lo && y1 < lo) {
        store<u8>(o + 85, 1);
        r = 1;
    }
    if (y0 > hi && y1 > hi) {
        store<u8>(o + 85, 1);
        r = 1;
    }
    ctx.fpr[1] = y1;
    ret(ctx, r);
}

// u32 f(T* o): the objects 0xB225C998 / 0x928F4335 / 0xBA681640 are
// activated (Obj_SetActive_001EB814(Obj_FindById(id), 1), when found) for
// the set bytes o->+81 / +82 / +83; unless the byte o->+53, the object
// *0x00F64CBC gets vt[20](1), vt[12]() and Obj_StartFromIdle; r3 = its.
// Other exits leave r3.
DECOMP(003F606C, Obj_ActivateTriple_003F606C) {
    const u32 o = arg<u32>(ctx, 0);
    static const u32 ids[3] = {0xB225C998u, 0x928F4335u, 0xBA681640u};
    for (u32 i = 0; i < 3; ++i) {
        if (load<u8>(o + 81 + i) == 0) continue;
        if (call(ctx, 0x000D6A28u, {ids[i]}) == 0) continue;
        const u32 x = static_cast<u32>(call(ctx, 0x000D6A28u, {ids[i]}));
        call(ctx, 0x001EB814u, {x, 1});
    }
    const u32 b = load<u8>(o + 53);
    if (b != 0) {
        ret(ctx, b);
        return;
    }
    const u32 p = load<u32>(0x00F64CBCu);
    if (p == 0) {
        ret(ctx, 0u);
        return;
    }
    VCall(ctx, p, 20, {p, 1});
    const u32 p2 = load<u32>(0x00F64CBCu);
    VCall(ctx, p2, 12, {p2});
    ret(ctx, call(ctx, 0x00364D28u, {load<u32>(0x00F64CBCu)}));
}

// u32 f(Ctl* c): t = Obj_FindByHandle42C2B0(c->+16, 0) -> c->+12; with
// bit 3 of t->+192: t->+380 = t->+392; t->vt[376]() (when set) gets
// vt[44](0); the object q = t->+380 has its +80 member's vt[28](0, 2)
// called and its +100 record set from c: +0 = c+20, +4 = c+28, +8 = c+32,
// bits 31 / 30 of +12 from c->+36. Then Flag_Set8_40000000(c); r3 = its.
DECOMP(004307CC, Ctl_ApplyRecord_004307CC) {
    const u32 c = arg<u32>(ctx, 0);
    const u32 t = static_cast<u32>(call(ctx, 0x0042C2B0u, {load<u32>(c + 16), 0}));
    store<u32>(c + 12, t);
    if (t != 0 && (load<u32>(t + 192) & 8u)) {
        store<u32>(t + 380, load<u32>(t + 392));
        if (VCall(ctx, t, 376, {t}) != 0) {
            const u32 x = static_cast<u32>(VCall(ctx, t, 376, {t}));
            VCall(ctx, x, 44, {x, 0});
        }
        const u32 q = load<u32>(t + 380);
        if (q != 0) {
            VCall(ctx, q + 80, 28, {q + 80, 0, 2});
            const u32 d = load<u32>(q + 100);
            store<u32>(d + 4, load<u32>(c + 28));
            store<u32>(d, load<u32>(c + 20));
            store<u32>(d + 8, load<u32>(c + 32));
            u32 w = (load<u32>(d + 12) & 0x7FFFFFFFu) | (load<u32>(c + 36) & 0x80000000u);
            store<u32>(d + 12, w);
            w = (w & ~0x40000000u) | (load<u32>(c + 36) & 0x40000000u);
            store<u32>(d + 12, w);
            ctx.fpr[1] = Lfs43(c + 32);
        }
    }
    ret(ctx, call(ctx, 0x0042CA60u, {c}));
}

// ---------------------------------------------------------------- sweep: 93 instructions
// u32 f(T* o): o->+308 = 3; the current player (G + 4208)->+8 (with bit 1
// of +192, else a null object) gets vt[64]() and Obj_ResetPos_0026D9D4 of
// the result; o->+352 = the spline 0x829A7FC5 of (*0x00F8F158)->+16
// (SplineMap_Find), o->+320 = whether it is set; o->+60->+348 =
// max(min(v * *0x0005DB98, v), *0x0005DB9C) of v = o->+60->+344; then
// o->vt[220](o, 4); r3 = its. f1 as the original leaves it.
DECOMP(0005DBA0, Obj_EnterSpline_0005DBA0) {
    const u32 o = arg<u32>(ctx, 0);
    store<u32>(o + 308, 3);
    const u32 p = load<u32>(load<u32>(0x00F65CA0u) + 4208 + 8);
    const u32 x = (p != 0 && (load<u32>(p + 192) & 2u)) ? p : 0u;
    const u32 y = static_cast<u32>(VCall(ctx, x, 64, {x}));
    call(ctx, 0x0026D9D4u, {y});
    store<u32>(o + 320, 0);
    Stack fr(ctx, 16);  // the key at sp+112
    store<u32>(fr.addr(), 0x829A7FC5u);
    const u32 slot = static_cast<u32>(call(ctx, 0x007335C4u, {load<u32>(load<u32>(0x00F8F158u) + 16), fr.addr()}));
    const u32 q = load<u32>(o + 60);
    const u32 sp = load<u32>(slot);
    store<u32>(o + 352, sp);
    const f64 v = Lfs43(q + 344);
    if (sp != 0) store<u32>(o + 320, 1);
    const f64 t = F43(v * Lfs43(0x0005DB98u));
    const f64 m = t < v ? t : v;
    const f64 k1 = Lfs43(0x0005DB9Cu);
    f64 f1 = v, f2 = k1;
    if (m > k1) {
        f1 = t < v ? t : v;
        f2 = f1;
    }
    store<u32>(q + 348, W43(f2));
    ctx.fpr[1] = f1;
    ret(ctx, VCall(ctx, o, 220, {o, 4}));
}

// T* f(-, XmlCursor* x): a new 24-byte record (Mem_Alloc(24, 0, 16);
// Obj_InitVt_B21C98(p, x); vtable 0x00B11118; +12 = 0; bytes +16 / +17 =
// 1, +18..+21 = 0) read from x: hash 0xB2E59948 -> +12, bools 0x7A281983
// -> +16, 0x6557DBDD -> +17 (default !+16), 0x03A6C100 -> +18, 0x04A4252F
// -> +20, 0xA92B6296 -> +19. r3 = it (0 when the allocation fails).
DECOMP(0009C614, Rec24_FromXml_0009C614) {
    const u32 x = arg<u32>(ctx, 1);
    const u32 p = static_cast<u32>(call(ctx, 0x0054ED38u, {24, 0, 16}));
    if (p == 0) {
        ret(ctx, 0u);
        return;
    }
    call(ctx, 0x004BB3B0u, {p, x});
    store<u32>(p + 12, 0);
    store<u8>(p + 18, 0);
    store<u8>(p + 16, 1);
    store<u32>(p, 0x00B11118u);
    store<u8>(p + 17, 1);
    store<u8>(p + 19, 0);
    store<u8>(p + 20, 0);
    store<u8>(p + 21, 0);
    call(ctx, 0x004FA14Cu, {x, 0xB2E59948u, p + 12, 0});
    call(ctx, 0x004F986Cu, {x, 0x7A281983u, p + 16, load<u8>(p + 16)});
    const u32 d = load<u8>(p + 16) == 0 ? 1u : 0u;
    store<u8>(p + 17, static_cast<u8>(d));
    call(ctx, 0x004F986Cu, {x, 0x6557DBDDu, p + 17, d});
    call(ctx, 0x004F986Cu, {x, 0x03A6C100u, p + 18, load<u8>(p + 18)});
    call(ctx, 0x004F986Cu, {x, 0x04A4252Fu, p + 20, load<u8>(p + 20)});
    call(ctx, 0x004F986Cu, {x, 0xA92B6296u, p + 19, load<u8>(p + 19)});
    ret(ctx, p);
}

// T* f(-, XmlCursor* x): a new 32-byte record (Mem_Alloc(32, 0, 16);
// Obj_InitVt_B21C98(p, x); vtable 0x00B11208; +12 = 0, +16 = +20 = 1.0f,
// +24 = 1, bytes +28 = 0 / +29 = 1) read from x: f32s 0x0F370611 -> +12
// (default *0x0009D308), 0x17ED3DB8 -> +16, 0xB7EA3D94 -> +20, s32
// 0x30BBD29C -> +24, bools 0x26873C74 -> +28, 0x8488565B -> +29. r3 = it
// (0 when the allocation fails).
DECOMP(0009D310, Rec32_FromXml_0009D310) {
    const u32 x = arg<u32>(ctx, 1);
    const u32 p = static_cast<u32>(call(ctx, 0x0054ED38u, {32, 0, 16}));
    if (p == 0) {
        ret(ctx, 0u);
        return;
    }
    call(ctx, 0x004BB3B0u, {p, x});
    store<u32>(p, 0x00B11208u);
    store<u32>(p + 12, 0);
    store<u32>(p + 16, 0x3F800000u);
    store<u32>(p + 20, 0x3F800000u);
    store<u32>(p + 24, 1);
    store<u8>(p + 28, 0);
    store<u8>(p + 29, 1);
    callf(ctx, 0x004F9B20u, {x, 0x0F370611u, p + 12}, {Lfs43(0x0009D308u)});
    callf(ctx, 0x004F9B20u, {x, 0x17ED3DB8u, p + 16}, {Lfs43(p + 16)});
    callf(ctx, 0x004F9B20u, {x, 0xB7EA3D94u, p + 20}, {Lfs43(p + 20)});
    call(ctx, 0x004F99BCu, {x, 0x30BBD29Cu, p + 24, static_cast<u64>(static_cast<s64>(static_cast<s32>(load<u32>(p + 24))))});
    call(ctx, 0x004F986Cu, {x, 0x26873C74u, p + 28, load<u8>(p + 28)});
    call(ctx, 0x004F986Cu, {x, 0x8488565Bu, p + 29, load<u8>(p + 29)});
    ret(ctx, p);
}

// void T(T* p, XmlCursor* x): Obj_InitVt_B21C98(p, x); vtable 0x00B112D0;
// +12 = +16 = 0, +20 = 1.0f; f32s 0x8F123DDD -> +12 (default *0x0009E520),
// 0xB7EA3D94 -> +16 (default k = *0x0009E51C); the hash 0x03D808CF picks
// the +20 default (0x8105148C: 0.5, 0x00010DFD: 2.0, else 1.0), then f32
// 0xBBD6EAAF -> +20 (default k); +16 below k is zeroed. r3 / f1 as the
// original leaves them.
DECOMP(0009E528, Rec_FromXml_0009E528) {
    const u32 p = arg<u32>(ctx, 0), x = arg<u32>(ctx, 1);
    call(ctx, 0x004BB3B0u, {p, x});
    store<u32>(p, 0x00B112D0u);
    const f64 k = Lfs43(0x0009E51Cu);
    store<u32>(p + 12, 0);
    store<u32>(p + 16, 0);
    store<u32>(p + 20, 0x3F800000u);
    callf(ctx, 0x004F9B20u, {x, 0x8F123DDDu, p + 12}, {Lfs43(0x0009E520u)});
    callf(ctx, 0x004F9B20u, {x, 0xB7EA3D94u, p + 16}, {k});
    {
        Stack fr(ctx, 16);  // the hash at sp+112
        if (call(ctx, 0x004FA14Cu, {x, 0x03D808CFu, fr.addr(), 0}) == 0) {
            const s32 h = static_cast<s32>(load<u32>(fr.addr()));
            u32 v = 0x3F800000u;
            if (h > static_cast<s32>(0x8105148Cu)) {
                if (h == 0x00010DFD) v = 0x40000000u;
            } else if (h > static_cast<s32>(0x8105148Bu)) {
                v = 0x3F000000u;
            }
            store<u32>(p + 20, v);
        }
    }
    callf(ctx, 0x004F9B20u, {x, 0xBBD6EAAFu, p + 20}, {k});
    const f64 a = Lfs43(p + 16);
    ctx.fpr[1] = a;
    if (a < k) {
        store<u32>(p + 16, 0);
        ret(ctx, 0u);
    }
}

// u32 f(T* o, T* e): a new 560-byte command (SmallAlloc) built by
// VtB210D0_Ctor_00433B2C(p, o + 12, o->+524, &{o+528, o+532}, &{o+536,
// o+540}, c, the bytes o->+556..+561, o + 548, o + 552, byte o->+562) with
// c the component o->+544 of e whose vt[32]() is 2 (else 0);
// CmdList_Push_0042C138(p) (0 when the allocation fails); r3 = its.
DECOMP(000A2098, Cmd_PushNew_000A2098) {
    const u32 o = arg<u32>(ctx, 0), e = arg<u32>(ctx, 1);
    u32 c = 0;
    const u32 key = load<u32>(o + 544);
    if (key != 0) {
        const u32 f = static_cast<u32>(call(ctx, 0x004BB4B4u, {e, key}));
        if (f != 0 && static_cast<u32>(VCall(ctx, f, 32, {f})) == 2) c = f;
    }
    const u32 p = static_cast<u32>(call(ctx, 0x0055238Cu, {560}));
    if (p != 0) {
        Stack fr(ctx, 16);  // the two pairs at sp+176 / sp+184
        const u32 v = fr.addr();
        for (u32 k = 0; k < 16; k += 4) store<u32>(v + k, load<u32>(o + 528 + k));
        ctx.fpr[1] = Lfs43(o + 536);  // left in f1 by the copy
        call(ctx, 0x00433B2Cu, {p, o + 12, static_cast<u64>(static_cast<s64>(static_cast<s32>(load<u32>(o + 524)))), v, v + 8, c, load<u8>(o + 556), load<u8>(o + 557), load<u8>(o + 558), load<u8>(o + 559), load<u8>(o + 560), load<u8>(o + 561), o + 548, o + 552, load<u8>(o + 562)});
    }
    ret(ctx, call(ctx, 0x0042C138u, {p}));
}

// bool f(T* o, src, s32 kind, f32* amt): only when o->vt[380]() is eq or
// above gt (else 0): kind 0 is refused (0) when the five floats +212 /
// +196 / +204 / +224 / +232 of o->+216 all equal *kp or src = o->+408,
// else *amt *= *(kp + 4); kinds other than 0 / 4 are refused. Then
// Obj_CanTakeHitBase_000DF960(o, src, kind, amt); r3 = its.
// (0x00185B28: 3 / 10; 0x00193270: 2 / 7)
static void CanTakeHit43(PPUContext& ctx, u32 eq, s32 gt, u32 kp) {
    const u32 o = arg<u32>(ctx, 0), src = arg<u32>(ctx, 1), amt = arg<u32>(ctx, 3);
    const s32 kind = static_cast<s32>(arg<u32>(ctx, 2));
    const u32 q = load<u32>(o + 216);
    if (static_cast<u32>(VCall(ctx, o, 380, {o})) != eq && !(static_cast<s32>(VCall(ctx, o, 380, {o})) > gt)) {
        ret(ctx, 0u);
        return;
    }
    if (kind != 0 && kind != 4) {
        ret(ctx, 0u);
        return;
    }
    if (kind == 0) {
        const f64 k = Lfs43(kp);
        ctx.fpr[1] = k;
        if (Lfs43(q + 212) == k && Lfs43(q + 196) == k && Lfs43(q + 204) == k && Lfs43(q + 224) == k && Lfs43(q + 232) == k) {
            ret(ctx, 0u);
            return;
        }
        if (src != 0 && load<u32>(o + 408) == src) {
            ret(ctx, 0u);
            return;
        }
        const f64 m = F43(Lfs43(amt) * Lfs43(kp + 4));
        store<u32>(amt, W43(m));
        ctx.fpr[1] = m;
    }
    ret(ctx, call(ctx, 0x000DF960u, {o, src, static_cast<u32>(kind), amt}));
}
// vt[380]() 3 or above 10; constants 0x00185B1C.
DECOMP(00185B28, Obj_CanTakeHit_00185B28) { CanTakeHit43(ctx, 3, 10, 0x00185B1Cu); }

// vt[380]() 2 or above 7; constants 0x00193264.
DECOMP(00193270, Obj_CanTakeHit_00193270) { CanTakeHit43(ctx, 2, 7, 0x00193264u); }

// void T(T* o, const Def* d): the base Thunk_000E27FC(o, d); vtable
// 0x00B15970; from d: +368..+376 = +288..+296, +384 / +388 / +392 =
// +304 / +308 / +312, bit 7 of the byte +396 from byte +316, the seven
// pairs +304.. = +224.. (8 bytes each); vtable 0x00B16898; +400 / +404 /
// +408 = +320 / +324 / +328, bits 31 / 30 of +412 from +332. r3 = that
// word, f1 as the original leaves it.
DECOMP(001C011C, Obj_Construct_001C011C) {
    const u32 o = arg<u32>(ctx, 0), d = arg<u32>(ctx, 1);
    call(ctx, 0x000E27FCu, {o, d});
    store<u32>(o, 0x00B15970u);
    store<u32>(o + 368, load<u32>(d + 288));
    store<u32>(o + 372, load<u32>(d + 292));
    store<u32>(o + 376, load<u32>(d + 296));
    store<u32>(o + 384, load<u32>(d + 304));
    const u8 b = load<u8>(o + 396);
    store<u32>(o + 388, load<u32>(d + 308));
    store<u32>(o + 392, load<u32>(d + 312));
    store<u8>(o + 396, static_cast<u8>((b & 0x7Fu) | (load<u8>(d + 316) & 0x80u)));
    const u32 w320 = load<u32>(d + 320), w324 = load<u32>(d + 324), w328 = load<u32>(d + 328);
    for (u32 i = 0; i < 7; ++i) {
        store<u32>(o + 304 + 8 * i, load<u32>(d + 224 + 8 * i));
        store<u32>(o + 308 + 8 * i, load<u32>(d + 228 + 8 * i));
    }
    store<u32>(o + 400, w320);
    store<u32>(o, 0x00B16898u);
    store<u32>(o + 404, w324);
    store<u32>(o + 408, w328);
    u32 w = (load<u32>(o + 412) & 0x7FFFFFFFu) | (load<u32>(d + 332) & 0x80000000u);
    store<u32>(o + 412, w);
    w = (w & ~0x40000000u) | (load<u32>(d + 332) & 0x40000000u);
    store<u32>(o + 412, w);
    ctx.fpr[1] = static_cast<f64>(std::bit_cast<f32>(w328));
    ret(ctx, w);
}

// void T(T* o, const Def* d): QuerySpawn_BaseCtor_0020C868(o, d, 32, 1);
// vtable 0x00B19E70; +428 / +430 = the u16s d+160 / +162, +432 = the kind
// d+164, bits 30-31 of +436 cleared / 29 cleared / 28 set, +440 = 0, +444
// / +448 = d+168 / +172. Kind 0x04C5F92B copies d+192..+200 -> +452..,
// d+208..+216 -> +464.., byte d+220 -> +476; kind 0x922D3BD1 looks the
// splines d+176 / d+180 up (SplineMap_Find on (*0x00F8F158)->+16) into
// +452 / +456 (a missing first one: Hash_ToString(d+176), no second
// lookup) and sets +460 = d+184. r3 / f1 as the original leaves them.
DECOMP(00214FAC, QuerySpawn_Construct_00214FAC) {
    const u32 o = arg<u32>(ctx, 0), d = arg<u32>(ctx, 1);
    call(ctx, 0x0020C868u, {o, d, 32, 1});
    const u32 w436 = ((load<u32>(o + 436) & 0x7FFFFFFFu) & ~0x40000000u & ~0x20000000u) | 0x10000000u;
    store<u16>(o + 428, load<u16>(d + 160));
    store<u16>(o + 430, load<u16>(d + 162));
    const u32 kind = load<u32>(d + 164);
    store<u32>(o, 0x00B19E70u);
    store<u32>(o + 432, kind);
    store<u32>(o + 436, w436);
    store<u32>(o + 440, 0);
    store<u32>(o + 444, load<u32>(d + 168));
    store<u32>(o + 448, load<u32>(d + 172));
    ctx.fpr[1] = Lfs43(d + 172);
    if (static_cast<s32>(kind) > static_cast<s32>(0x922D3BD1u)) {
        if (kind != 0x04C5F92Bu) {
            ret(ctx, kind);
            return;
        }
        for (u32 k = 0; k < 12; k += 4) store<u32>(o + 452 + k, load<u32>(d + 192 + k));
        for (u32 k = 0; k < 12; k += 4) store<u32>(o + 464 + k, load<u32>(d + 208 + k));
        ctx.fpr[1] = Lfs43(d + 216);
        const u32 b = load<u8>(d + 220);
        store<u8>(o + 476, static_cast<u8>(b));
        ret(ctx, b);
        return;
    }
    if (kind != 0x922D3BD1u) {
        ret(ctx, kind);
        return;
    }
    Stack fr(ctx, 16);  // the keys at sp+112 / sp+116
    store<u32>(fr.addr() + 4, load<u32>(d + 176));
    const u32 s0 = load<u32>(static_cast<u32>(call(ctx, 0x007335C4u, {load<u32>(load<u32>(0x00F8F158u) + 16), fr.addr() + 4})));
    store<u32>(o + 452, s0);
    if (s0 == 0) {
        call(ctx, 0x0056A6E0u, {load<u32>(d + 176)});
    } else {
        store<u32>(fr.addr(), load<u32>(d + 180));
        const u32 s1 = load<u32>(static_cast<u32>(call(ctx, 0x007335C4u, {load<u32>(load<u32>(0x00F8F158u) + 16), fr.addr()})));
        store<u32>(o + 456, s1);
    }
    const u32 w = load<u32>(d + 184);
    store<u32>(o + 460, w);
    ret(ctx, w);
}

// u32 f(T* a, Obj* o): walks to the first of a's four links {+328 key,
// s32 +332 index} (8 apart) whose index is valid in o's byte table +656
// (bit 7 set, bit 6 clear) and whose key maps (SortedMap_Find749A14 on
// 0x0105752C) to a node n with n->+324 <= the limit (*0x0027265C, or the
// player's (G + 0x20C80 * s8 o->+172 + 4208)->+108 with bit 0 of
// o->+188); recurses into 0x00272660(n, o) then (r3 = its), else r3 = a.
DECOMP(00272660, Link_FindReachable_00272660) {
    const u32 a = arg<u32>(ctx, 0), o = arg<u32>(ctx, 1);
    f64 lim = Lfs43(0x0027265Cu);
    if (load<u32>(o + 188) & 1u)
        lim = Lfs43(load<u32>(0x00F65CA0u) + static_cast<u32>(static_cast<s32>(static_cast<s8>(load<u8>(o + 172)))) * 0x20C80u + 4208 + 108);
    u32 best = 0;
    for (u32 i = 0; i < 4 && best == 0; ++i) {
        const s32 idx = static_cast<s32>(load<u32>(a + 8 * i + 332));
        if (idx < 0) continue;
        const u32 b = load<u8>(o + 656 + static_cast<u32>(idx));
        if (!(b & 0x80u) || (b & 0x40u)) continue;
        const u32 x = load<u32>(static_cast<u32>(call(ctx, 0x00749A14u, {0x0105752Cu, a + 8 * i + 328})));
        if (x == 0) continue;
        const f64 v = Lfs43(x + 324);
        ctx.fpr[1] = v;
        if (lim > v || lim == v) best = x;
    }
    if (best != 0) {
        ret(ctx, call(ctx, 0x00272660u, {best, o}));
        return;
    }
    ret(ctx, a);
}

// The deleting ~T(): vtable 0x00B1C578, the u16 +96 (15 bits) 192-byte
// records at +100 freed (SmallFree); World_ListRemove_0046B7A4(
// *0x00F65CA0, 0, 0x00F352C8, o, o->+8); vtable 0x00B1BE30, o removed
// from the registry array 0x01056F1C, List1013B5C_Remove(o) and
// SmallFree(o, 144); r3 = its (0 for a null o).
DECOMP(00284318, Registered_Delete_00284318) {
    const u32 o = arg<u32>(ctx, 0);
    if (o == 0) {
        ret(ctx, 0u);
        return;
    }
    const u32 buf = load<u32>(o + 100);
    store<u32>(o, 0x00B1C578u);
    if (buf != 0) call(ctx, 0x00552420u, {buf, (static_cast<u32>(load<u16>(o + 96)) & 0x7FFFu) * 192u});
    call(ctx, 0x0046B7A4u, {load<u32>(0x00F65CA0u), 0, 0x00F352C8u, o, load<u32>(o + 8)});
    store<u32>(o, 0x00B1BE30u);
    RegistryRemove43(o);
    call(ctx, 0x000D6940u, {o});
    ret(ctx, call(ctx, 0x00552420u, {o, 144}));
}

// u32 f(T* o): o->+2560 = mode; when o->+2564 is 0: o->+960 = 0 and the
// model m = o->+948 gets vt[188](m, key, 0), its colour c (m->vt[52](&c,
// m)) written back with the alpha byte cleared (m->vt[56](m, rgb0)); then
// the three effects +196 / +200 / +204 are bound with Effect_Bind4A6558(
// slot, "..." 0x00AD4B7C, 0x00AD4B7C, o->+48, o->+52, 1) (with first, +196
// with Effect_Bind4A63FC(o + 196, o->+160, 0, o->+48, o->+52, 1)); r3 =
// the last one's. (0x002A4D08: 2, 0x01059854; 0x002A4E7C: 4, 0x0105978C)
static void ResetEffects43(PPUContext& ctx, u32 mode, u32 key, bool first) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 flag = load<u32>(o + 2564);
    store<u32>(o + 2560, mode);
    if (flag == 0) {
        store<u32>(o + 960, 0);
        const u32 m = load<u32>(o + 948);
        if (m != 0) {
            VCall(ctx, m, 188, {m, key, 0});
            Stack fr(ctx, 16);  // the colour at sp+112
            const u32 c = fr.addr();
            const u32 m2 = load<u32>(o + 948);
            VCall(ctx, m2, 52, {c, m2});
            store<u8>(c + 3, 0);
            const u32 rgb = (static_cast<u32>(load<u8>(c)) << 24) | (static_cast<u32>(load<u8>(c + 1)) << 16) | (static_cast<u32>(load<u8>(c + 2)) << 8);
            const u32 m3 = load<u32>(o + 948);
            const u32 opd = load<u32>(load<u32>(m3) + 56);
            call_ptr(ctx, opd, {m3, rgb, load<u32>(opd)});  // the entry left in r5
        }
    }
    const u32 nm = 0x00AD4B7Cu;
    if (first) call(ctx, 0x004A63FCu, {o + 196, load<u32>(o + 160), 0, load<u32>(o + 48), load<u32>(o + 52), 1});
    else call(ctx, 0x004A6558u, {o + 196, nm, nm, load<u32>(o + 48), load<u32>(o + 52), 1});
    call(ctx, 0x004A6558u, {o + 200, nm, nm, load<u32>(o + 48), load<u32>(o + 52), 1});
    ret(ctx, call(ctx, 0x004A6558u, {o + 204, nm, nm, load<u32>(o + 48), load<u32>(o + 52), 1}));
}
// Mode 2 (key 0x01059854), all three by name.
DECOMP(002A4D08, Obj_ResetEffects_002A4D08) { ResetEffects43(ctx, 2, 0x01059854u, false); }

// Mode 4 (key 0x0105978C), +196 bound from o->+160.
DECOMP(002A4E7C, Obj_ResetEffects_002A4E7C) { ResetEffects43(ctx, 4, 0x0105978Cu, true); }

// u32 f(-; f32 v): v (at least k0 = *0x002AB5E8) -> *0x010592F0 and the
// running minimum *0x010592EC; when v > k0: a minimum within k1 =
// *0x002AB5EC of zero spawns the effect *0x01059F2C (Effect_Spawn(e, 0, 0,
// 1); Effect_SetTime(r; *0x002AB5F0) when one comes back), sets bit 15 of
// (*0x00F64A58)->+244 when its bits 20-27 are set and gives *0x00F64A6C
// the hash 0xCB2E45B1; then, with |min - v| < k1, *0x00F64A6C gets
// 0xF56FA63D. r3 / f1 as the original leaves them.
DECOMP(002AB5F8, Meter_Track_002AB5F8) {
    const f64 v = farg(ctx, 0);
    const f64 k0 = Lfs43(0x002AB5E8u);
    const f64 f2 = v < k0 ? k0 : v;
    f64 mn = Lfs43(0x010592ECu);
    store<u32>(0x010592F0u, W43(f2));
    if (mn > f2) {
        mn = f2;
        store<u32>(0x010592ECu, W43(f2));
    }
    ctx.fpr[1] = k0;
    if (!(f2 > k0)) {
        ret(ctx, 0x01060000u);
        return;
    }
    const f64 k1 = Lfs43(0x002AB5ECu);
    if (std::fabs(mn) < k1) {
        const u32 r = static_cast<u32>(call(ctx, 0x004A5FC8u, {load<u32>(0x01059F2Cu), 0, 0, 1}));
        if (r != 0) callf(ctx, 0x004CF710u, {r}, {Lfs43(0x002AB5F0u)});
        const u32 p = load<u32>(0x00F64A58u);
        if (p != 0) {
            const u32 w = load<u32>(p + 244);
            if ((w >> 20) & 0xFFu) store<u32>(p + 244, w | 0x8000u);
        }
        const u32 e = load<u32>(0x00F64A6Cu);
        if (e != 0) call(ctx, 0x004CF988u, {e, 0xCB2E45B1u});
    }
    const f64 d = std::fabs(F43(Lfs43(0x010592ECu) - Lfs43(0x010592F0u)));
    ctx.fpr[1] = d;
    if (!(d < k1)) {
        ret(ctx, 0x01060000u);
        return;
    }
    const u32 e = load<u32>(0x00F64A6Cu);
    if (e == 0) {
        ret(ctx, 0u);
        return;
    }
    ret(ctx, call(ctx, 0x004CF988u, {e, 0xF56FA63Du}));
}

// u32 f(T* o): with the byte o->+8->+156 set (else r3 = 0), each of the
// six parts +188 .. +208 gets vt[40](part, 0, 1). r3 as the original
// leaves it.
DECOMP(0036C898, Obj_PartsOff_0036C898) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 on = load<u8>(load<u32>(o + 8) + 156);
    if (on == 0) {
        ret(ctx, 0u);
        return;
    }
    u64 r = 0;
    for (u32 k = 0; k < 6; ++k) {
        const u32 x = load<u32>(o + 188 + 4 * k);
        if (x != 0) r = VCall(ctx, x, 40, {x, 0, 1});
        else if (k < 5) r = 0;
    }
    ctx.gpr[3] = r;
}

// u32 f(T* o): byte o->+239 = 0; Handle_Call363A3C(0x63D3E645, 0, 1, -,
// 0, 0, 0x01164CA4; k = *0x004051DC); o->+92->vt[88](1, 1, 1, 1) and, when
// it has the child 0x435538CB (Obj_FindByHash_000DB290), child->vt[88](0,
// 1, 1, 1); then when w = (o->vt[76]())->+0x1281E0 lacks the word o->+60
// (Obj_HasWord368_000FC5DC), Camera_PushTarget0FC040(w, o->+60, -, 0; k);
// r3 = its. Other exits leave r3. (r6 / r5 are not set for the two
// calls, which do not read them.)
DECOMP(004051E0, Obj_Reset_004051E0) {
    const u32 o = arg<u32>(ctx, 0);
    const f64 k = Lfs43(0x004051DCu);
    store<u8>(o + 239, 0);
    callf(ctx, 0x0036735Cu, {0x63D3E645u, 0, 1, ctx.gpr[6], 0, 0, 0x01164CA4u}, {k});
    const u32 a = load<u32>(o + 92);
    VCall(ctx, a, 88, {a, 1, 1, 1, 1});
    if (call(ctx, 0x000DB290u, {load<u32>(o + 92), 0x435538CBu}) != 0) {
        const u32 c = static_cast<u32>(call(ctx, 0x000DB290u, {load<u32>(o + 92), 0x435538CBu}));
        VCall(ctx, c, 88, {c, 0, 1, 1, 1});
    }
    const u32 x = static_cast<u32>(VCall(ctx, o, 76, {o}));
    const u32 w = load<u32>(x + 0x1281E0u);
    if (w == 0) return;  // r3 = x
    if (call(ctx, 0x000FC5DCu, {w, load<u32>(o + 60)}) != 0) return;
    callf(ctx, 0x000FC040u, {w, load<u32>(o + 60), ctx.gpr[5], 0}, {k});
}
