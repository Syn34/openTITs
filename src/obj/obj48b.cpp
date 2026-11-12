// Game code, part 48 (continued): the small-function sweep from the 105
// instruction functions up. Each was checked against the original in the PPC
// simulator (memory writes, calls and their arguments, r3 and f1).
#include "obj48.h"

using namespace sly4;
using namespace sly4::obj48;

// ---------------------------------------------------------------- sweep: 105 instructions
// u32 f(Game* g): the phase a level asks for. With the current level w =
// (*0x00F64C20)->+72 (r3 = 0 without one), its hash +2560 maps to a phase
// 0..9 (0x28BD7B54, 0x61EE7D6E, 0x9A087C42, 0x85800157, 0x9A087C43,
// 0x3ECD0DB6, 0xDBE13084, 0x9A087C44, 0x2A8029BB, 0x9FBBC0E8); 0xE7A66BC9
// instead switches g to state 25 (g->vt[220](g, 25), unless g->+4 is 25
// already). Game_SetPhase(g, phase) when that is past g->+308, else r3 =
// g->+308.
DECOMP(00054110, Game_PhaseFromLevel_00054110) {
    const u32 g = arg<u32>(ctx, 0);
    const u32 w = load<u32>(load<u32>(0x00F64C20u) + 72);
    if (w == 0) {
        ret(ctx, 0u);
        return;
    }
    const u32 h = load<u32>(w + 2560);
    u32 cur = load<u32>(g + 308);
    s32 tgt = static_cast<s32>(cur);
    switch (h) {
    case 0x28BD7B54u: tgt = 0; break;
    case 0x61EE7D6Eu: tgt = 1; break;
    case 0x9A087C42u: tgt = 2; break;
    case 0x85800157u: tgt = 3; break;
    case 0x9A087C43u: tgt = 4; break;
    case 0x3ECD0DB6u: tgt = 5; break;
    case 0xDBE13084u: tgt = 6; break;
    case 0x9A087C44u: tgt = 7; break;
    case 0x2A8029BBu: tgt = 8; break;
    case 0x9FBBC0E8u: tgt = 9; break;
    case 0xE7A66BC9u:
        if (load<u32>(g + 4) != 25) {
            VcOE48(ctx, g, 220, {g, 25});
            cur = load<u32>(g + 308);
        }
        break;
    default: break;
    }
    if (tgt > static_cast<s32>(cur)) {
        ret(ctx, static_cast<u32>(call(ctx, 0x00053A80u, {g, Sx48(static_cast<u32>(tgt))})));
        return;
    }
    ret(ctx, cur);
}

// T* f(?, XmlCursor* x): new 28-byte record (base Obj_InitVt_B21C98,
// vtable 0x00B0F7E8): +12 = hash 0x8A93973F; when that is absent the
// colours +16 / +20 = 0x00210FAA / 0x00000A7B (defaults 0x01164CA0 /
// 0x01164CA8, also the initial values) and f32 +24 = 0xB7EA3D94 (default
// *0x00077BC4).
DECOMP(00077BC8, XmlNew_B0F7E8_00077BC8) {
    const u32 x = arg<u32>(ctx, 1);
    const u32 m = NewXmlObj47(ctx, 28, x);
    if (m == 0) {
        ret(ctx, 0u);
        return;
    }
    store<u32>(m + 12, 0);
    store<u32>(m, 0x00B0F7E8u);
    for (u32 k = 0; k < 4; ++k) store<u8>(m + 16 + k, load<u8>(0x01164CA0u + k));
    for (u32 k = 0; k < 4; ++k) store<u8>(m + 20 + k, load<u8>(0x01164CA8u + k));
    store<u32>(m + 24, 0);
    if (static_cast<u32>(call(ctx, kReadHash47, {x, 0x8A93973Fu, m + 12, 0})) == 0) {
        call(ctx, 0x004F9F9Cu, {x, 0x00210FAAu, m + 16, 0x01164CA0u});
        call(ctx, 0x004F9F9Cu, {x, 2683, m + 20, 0x01164CA8u});
        callf(ctx, kReadF32_47, {x, 0xB7EA3D94u, m + 24}, {Lfs48(0x00077BC4u)});
    }
    ret(ctx, m);
}

// u32 f(): loading screen. Loading_Init_000ABBE0(0); with the screen
// object s = *0x00F558E8 (r3 = 0 without): s->vt[88](s, 1), the old render
// target *0x00F558E4 released (vt[8]) and replaced by
// AglRenderTarget_New_0052317C() (after EnsureTextures_00542804) named
// 0x00AC5E68, RenderTarget_SetByte49(rt, 0), viewport {0, 0, display
// w/h (u16 0x010C5A24 / 26)}, RenderTarget_Sync_005109B0; then
// s->vt[40](s, {aspect, 1, 1}) with aspect = w / *0x000AC694 when u8
// 0x010C5A42 is set, else 1 (*0x000AC690).
DECOMP(000AC698, Loading_ShowScreen_000AC698) {
    call(ctx, 0x000ABBE0u, {0});
    constexpr u32 kScr = 0x00F558E8u, kRt = 0x00F558E4u, kDisp = 0x010C5A20u;
    const u32 s = load<u32>(kScr);
    if (s == 0) {
        ret(ctx, 0u);
        return;
    }
    VcOE48(ctx, s, 88, {s, 1});
    const u32 old = load<u32>(kRt);
    if (old != 0) {
        VcOE48(ctx, old, 8, {old});
        store<u32>(kRt, 0);
    }
    const u32 et = static_cast<u32>(call(ctx, 0x00542804u));
    const u32 rt = static_cast<u32>(call(ctx, 0x0052317Cu, {et}));  // r3 = EnsureTextures' result, read by the callee
    store<u32>(kRt, rt);
    call(ctx, 0x00553684u, {rt + 16, 0x00AC5E68u, 32});
    call(ctx, 0x0052038Cu, {load<u32>(kRt), 0});
    Stack fr(ctx, 32);  // sp+112: the vec3, sp+128: the viewport rect
    const u32 v = fr.addr(), rc = v + 16;
    store<u16>(rc, 0);
    store<u16>(rc + 2, 0);
    store<u16>(rc + 4, load<u16>(kDisp + 4));
    store<u16>(rc + 6, load<u16>(kDisp + 6));
    call(ctx, 0x005200ACu, {load<u32>(kRt), rc});
    call(ctx, 0x005109B0u, {load<u32>(kRt)});
    const f64 one = Lfs48(0x000AC690u);
    f64 asp = one;
    if (load<u8>(kDisp + 34) != 0) asp = F48(F48(static_cast<f64>(load<u16>(kDisp + 4))) / Lfs48(0x000AC694u));
    store<u32>(v, W48(asp));
    store<u32>(v + 4, W48(one));
    store<u32>(v + 8, W48(one));
    const u32 s2 = load<u32>(kScr);
    ctx.fpr[1] = one;
    ctx.fpr[2] = asp;
    ret(ctx, static_cast<u32>(VcOE48(ctx, s2, 40, {s2, v})));
}

// u32 f(u32* out, Obj* o, a, Body* b, u32 kind): *out (and r3) = a handle
// from Handle_Acquire443EC0(&h, a, b), 0 when bit 4 of u16 o->+176 is set,
// a is 0 or kind is 0x411D6991. Except for kind 0xF59E7E78 the body first
// gets o's position (+144..152 -> b+0..8) and bit 1 of b->+56. Kind
// 0x7641A763 only works with fewer than 4 handles (bits 19..21 of o->+172)
// and records {h, -1, 0} in o's 12-byte slot (+256) before counting it.
DECOMP(000DBA84, Obj_AcquireHandle_000DBA84) {
    const u32 out = arg<u32>(ctx, 0), o = arg<u32>(ctx, 1), a = arg<u32>(ctx, 2), b = arg<u32>(ctx, 3);
    const u32 kind = arg<u32>(ctx, 4);
    Stack fr(ctx, 16);  // sp+112: the result; sp+116 / +120 / +124: the handle outs
    const u32 sp = fr.addr();
    store<u32>(sp, 0);
    store<u16>(sp + 2, 0);
    auto place = [&]() {
        const u32 f = load<u32>(b + 56) | 2u;
        Cp48(b, o + 144);
        Cp48(b + 4, o + 148);
        store<u32>(b + 56, f);
        Cp48(b + 8, o + 152);
        ctx.fpr[1] = Lfs48(o + 152);  // f1 = the last lfs
    };
    if (((load<u16>(o + 176) >> 4) & 1u) == 0 && a != 0 && kind != 0x411D6991u) {
        if (kind == 0x7641A763u) {
            if (static_cast<s32>((load<u32>(o + 172) >> 19) & 7u) < 4) {
                place();
                call(ctx, 0x00443EC0u, {sp + 4, a, b});
                const u32 n = (load<u32>(o + 172) >> 19) & 7u;
                const u32 h = load<u32>(sp + 4);
                store<u32>(sp, h);
                const u32 e = o + n * 12u + 256;
                store<u32>(e + 4, 0xFFFFFFFFu);
                store<u32>(e + 8, 0);
                store<u32>(e, h);
                const u32 w = load<u32>(o + 172);
                store<u32>(o + 172, (w & ~0x00380000u) | (((((w >> 19) & 7u) + 1) << 19) & 0x00380000u));
            }
        } else if (kind == 0xF59E7E78u) {
            call(ctx, 0x00443EC0u, {sp + 8, a, b});
            store<u32>(sp, load<u32>(sp + 8));
        } else {
            place();
            call(ctx, 0x00443EC0u, {sp + 12, a, b});
            store<u32>(sp, load<u32>(sp + 12));
        }
    }
    const u32 r = load<u32>(sp);
    store<u32>(out, r);
    ret(ctx, r);
}

// u32 f(Obj* o, x): constructor body (vtable 0x00B15778, interfaces +380 /
// +384) after Thunk_000E40B4(o, x): +424 = Save_Profile0C9FC8(o->+216->+308)
// (StateMachine_SetState(o, 0, 0) when none); Obj_SetFlagHash_0048FA54(o,
// 0x600AE700, 1) when bit 31 of o->+216->+316 is set, (o, 0xABB250AE, 1);
// +432 = the component 0xBBBC16FF of o->+356 when its vt[32] answers 1;
// +428 = 0, o->vt[28](o, 0, 1), o->vt[28](o, 1, 0).
DECOMP(001AC1E8, Obj_ConstructB15778_001AC1E8) {
    const u32 o = arg<u32>(ctx, 0), x = arg<u32>(ctx, 1);
    call(ctx, 0x000E40B4u, {o, x});
    const u32 d = load<u32>(o + 216);
    constexpr u32 VT = 0x00B15770u;
    store<u32>(o + 424, 0);
    store<u32>(o + 380, VT + 436);
    store<u32>(o + 384, VT + 476);
    store<u32>(o, VT + 8);
    store<u32>(o + 428, 0);
    const u32 key = load<u32>(d + 308);
    store<u32>(o + 432, 0);
    const u32 p = static_cast<u32>(call(ctx, 0x000C9FC8u, {key}));
    store<u32>(o + 424, p);
    if (p == 0) {
        ret(ctx, static_cast<u32>(call(ctx, 0x00464D1Cu, {o, 0, 0})));
        return;
    }
    if ((load<u32>(d + 316) & 0x80000000u) != 0) call(ctx, 0x0048FA54u, {o, 0x600AE700u, 1});
    call(ctx, 0x0048FA54u, {o, 0xABB250AEu, 1});
    const u32 e = load<u32>(o + 356);
    if (e != 0) {
        const u32 c = static_cast<u32>(call(ctx, kFindComp48, {e, 0xBBBC16FFu}));
        if (c != 0 && static_cast<u32>(VcOE48(ctx, c, 32, {c})) == 1) store<u32>(o + 432, c);
    }
    store<u32>(o + 428, 0);
    VcOE48(ctx, o, 28, {o, 0, 1});
    ret(ctx, static_cast<u32>(VcOE48(ctx, o, 28, {o, 1, 0})));
}

// u32 f(Obj* o, u8 on): switching the ragdoll on first drops the volume
// registration (Volume_Notify112D8(*0x00F557EC, o + 588) when u8 o->+592)
// and the animation skeleton o->+576: its Havok properties 0xA734004D /
// 0x91C7CF5B (Hk_RemoveProperty93FED8 when present in the +124/+128
// table), the entity from its world +8 (Hk_RemoveEntity92FFC8), then
// AnimSkel_Release7D4EA8 and +576 = 0. r3 = Obj_SetRagdollBase(o, on).
DECOMP(001B58BC, Obj_SetRagdoll_001B58BC) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 on = arg<u32>(ctx, 1) & 0xFFu;
    if (on != 0) {
        if (load<u8>(o + 592) != 0) {
            call(ctx, 0x000112D8u, {load<u32>(0x00F557ECu), o + 588});
            store<u8>(o + 592, 0);
        }
        u32 sk = load<u32>(o + 576);
        if (sk != 0) {
            const u32 world = load<u32>(sk + 8);
            if (world != 0) {
                Stack fr(ctx, 64);  // sp+112 / +120 / +128: the Havok result records
                const u32 sp = fr.addr();
                u32 s = sk;
                for (s32 i = 0; i < static_cast<s32>(load<u32>(sk + 128)); ++i) {
                    if (load<u32>(load<u32>(sk + 124) + 16 * static_cast<u32>(i)) == 0xA734004Du) {
                        call(ctx, 0x0093FED8u, {sp, sk, 0xA734004Du});
                        s = load<u32>(o + 576);
                        break;
                    }
                }
                u32 ent = s;
                for (s32 i = 0; i < static_cast<s32>(load<u32>(s + 128)); ++i) {
                    if (load<u32>(load<u32>(s + 124) + 16 * static_cast<u32>(i)) == 0x91C7CF5Bu) {
                        call(ctx, 0x0093FED8u, {sp + 8, s, 0x91C7CF5Bu});
                        ent = load<u32>(o + 576);
                        break;
                    }
                }
                call(ctx, 0x0092FFC8u, {sp + 16, world, ent});
                sk = load<u32>(o + 576);
            }
            call(ctx, 0x007D4EA8u, {sk});
            store<u32>(o + 576, 0);
        }
    }
    ret(ctx, static_cast<u32>(call(ctx, 0x000E02F8u, {o, on})));
}

// void f(Obj* o): steering direction. o->+656..664 = the vec3 o->+640
// normalised when its squared length exceeds *0x00239A94 (1 / sqrt, 1 =
// *0x00239A98); then the avoidance vec (0x01164B40 minus +128..136 of each
// active (+36) 224-byte entry from o->+612->+72, s8 count at
// o->+612->+28->+32), when long enough, is added in normalised and the sum
// normalised again; finally the result is normalised once more if long
// enough. r3 = o, f1 = the z component.
DECOMP(00239AA0, Obj_SteerDir_00239AA0) {
    const u32 o = arg<u32>(ctx, 0);
    constexpr u32 V = 0x01164B40u, K = 0x00239A94u;
    f64 ax = Lfs48(V), ay = Lfs48(V + 4), az = Lfs48(V + 8);
    const u32 s = load<u32>(o + 612);
    if (s != 0 && load<u32>(s + 28) != 0) {
        const u32 n = static_cast<u32>(static_cast<s32>(static_cast<s8>(load<u8>(load<u32>(s + 28) + 32))));
        u32 p = load<u32>(s + 72);
        for (u32 i = 0; i < n; ++i, p += 224) {
            if (load<u32>(p + 36) != 0) {
                ax = F48(ax - Lfs48(p + 128));
                ay = F48(ay - Lfs48(p + 132));
                az = F48(az - Lfs48(p + 136));
            }
        }
    }
    auto fma_s = [](f64 a, f64 c, f64 b) { return F48(std::fma(a, c, b)); };
    f64 y = Lfs48(o + 644), x = Lfs48(o + 640);
    f64 t = F48(y * y);
    f64 z = Lfs48(o + 648);
    Cp48(o + 656, o + 640);
    Cp48(o + 660, o + 644);
    Cp48(o + 664, o + 648);
    const f64 c0 = Lfs48(K);
    auto put = [&]() {
        store<u32>(o + 656, W48(x));
        store<u32>(o + 660, W48(y));
        store<u32>(o + 664, W48(z));
    };
    auto scale = [&](f64 len2) {
        const f64 k = F48(Lfs48(K + 4) / F48(std::sqrt(len2)));
        x = F48(x * k);
        y = F48(y * k);
        z = F48(z * k);
        put();
    };
    t = fma_s(z, z, fma_s(x, x, t));
    if (t > c0) scale(t);
    const f64 a2 = fma_s(az, az, fma_s(ax, ax, F48(ay * ay)));
    if (a2 > c0) {
        const f64 k = F48(Lfs48(K + 4) / F48(std::sqrt(a2)));
        y = fma_s(k, ay, y);
        z = fma_s(k, az, z);
        x = fma_s(k, ax, x);
        scale(fma_s(z, z, fma_s(x, x, F48(y * y))));
    }
    const f64 l2 = fma_s(z, z, fma_s(x, x, F48(y * y)));
    if (l2 > c0) scale(l2);
    ctx.fpr[1] = z;
}

// u32 f(): resets the blend state when the renderer (*0x00F64A50)->+24 is
// live (r3 = 0 otherwise): register/value pairs into the command buffer
// *0x010F10A0 (0x40310 = 1; 0x80314 = 0x03020302, 0x03030303; 0x40320 =
// 0x80068006; 0x40A70 = 0; 0x40A74 = bit 0 / all of u8 0x010F10FD; 0x40A6C
// = 515; 0x4183C = 0), then PairBuffer_Flush(*0x00F64A50) and r3 =
// Draw_InvalidateState().
DECOMP(0028CF58, Gfx_ResetBlend_0028CF58) {
    constexpr u32 G = 0x00F64A50u, B = 0x010F10A0u;
    if (load<u32>(load<u32>(G) + 24) == 0) {
        ret(ctx, 0u);
        return;
    }
    auto put = [&](u32 reg, u32 val) {
        const u32 p = load<u32>(B), w = load<u32>(p + 8);
        store<u32>(w, reg);
        store<u32>(w + 4, val);
        store<u32>(p + 8, w + 8);
    };
    auto put_late = [&](u32 reg, u32 val) {  // the value word written after the pointer bump
        const u32 p = load<u32>(B), w = load<u32>(p + 8);
        store<u32>(w, reg);
        store<u32>(p + 8, w + 8);
        store<u32>(w + 4, val);
    };
    put(0x00040310u, 1);
    {
        const u32 p = load<u32>(B), w = load<u32>(p + 8);
        store<u32>(w, 0x00080314u);
        store<u32>(w + 4, 0x03020302u);
        store<u32>(w + 8, 0x03030303u);
        store<u32>(p + 8, w + 12);
    }
    put(0x00040320u, 0x80068006u);
    const u32 b93 = load<u8>(B + 93);
    put(0x00040A70u, 0);
    put(0x00040A74u, b93 & 1u);
    put(0x00040A74u, b93);
    put_late(0x00040A6Cu, 515);
    put_late(0x0004183Cu, 0);
    call(ctx, 0x00565228u, {load<u32>(G), 0});
    ret(ctx, static_cast<u32>(call(ctx, 0x0051279Cu)));
}

// u32 f(Obj* o, Target* t): picks up t (r3 = o when null): bit 22 of
// o->+124, Obj_SetStateFx266E6C(o, 2), the old carried object (+64) gets
// vt[40](old, 0), +64 = t, ++u16 t->+178. With bit 30 of +124: state +68 ==
// 2 sets bit 21, Obj_SetStateFx(o, bit 31 ? 3 : 0), o->+8->vt[252],
// releases +64 again (vt[40], +64 = 0) and calls the callback +80(o, +84);
// state 0 only sets bit 21 and the state fx. r3 = the last result.
DECOMP(002B7560, Obj_PickUp_002B7560) {
    const u32 o = arg<u32>(ctx, 0), t = arg<u32>(ctx, 1);
    if (t == 0) {
        ret(ctx, o);
        return;
    }
    store<u32>(o + 124, load<u32>(o + 124) | 0x00400000u);
    call(ctx, 0x00266E6Cu, {o, 2});
    u32 r3 = load<u32>(o + 64);
    if (r3 != 0) VcOE48(ctx, r3, 40, {r3, 0});
    store<u32>(o + 64, t);
    store<u16>(t + 178, static_cast<u16>(load<u16>(t + 178) + 1));
    const u32 f = load<u32>(o + 124);
    if ((f & 0x40000000u) == 0) {
        ret(ctx, 0u);
        return;
    }
    const u32 st = load<u32>(o + 68);
    if (st == 2) {
        const u32 nf = f | 0x00200000u;
        store<u32>(o + 124, nf);
        call(ctx, 0x00266E6Cu, {o, (nf & 0x80000000u) != 0 ? 3u : 0u});
        r3 = load<u32>(o + 8);
        if (r3 != 0) VcOE48(ctx, r3, 252, {r3});
        r3 = load<u32>(o + 64);
        if (r3 != 0) r3 = static_cast<u32>(VcOE48(ctx, r3, 40, {r3, 0}));
        store<u32>(o + 64, 0);
        const u32 cb = load<u32>(o + 80);
        if (cb != 0) r3 = static_cast<u32>(call_ptr(ctx, cb, {o, load<u32>(o + 84), load<u32>(cb)}));
        ret(ctx, r3);
        return;
    }
    if (st != 0) {
        ret(ctx, st);
        return;
    }
    const u32 nf = f | 0x00200000u;
    store<u32>(o + 124, nf);
    ret(ctx, static_cast<u32>(call(ctx, 0x00266E6Cu, {o, (nf & 0x80000000u) != 0 ? 3u : 0u})));
}

// u32 f(State* s): climb restart. ++s->+28, +36 = 0, the hashes +76 / +68
// / +72 = 0x7883E3B1 / 0x785F53FE / 0x67DC7D75. Unless the actor a =
// s->+32 already plays 0x785F53FE (+668): Anim_Restart2775A8(a->vt[60](a),
// 0x785F53FE, 0, 0, -1, -, 0, 0, -, -, 0, -1; speed *0x01069A38 *
// *0x01069008, blend (f32)(the rate != 1), 1, 1) where the rate is 1
// (*0x00316300) or *0x01068FB8 when u8 a->+700 is clear; then a->+700 = 0,
// +668 = 0x785F53FE. Then Climb_Fn308F5C(a->+872, 0) and r3 =
// s->vt[28](s, 0, 1).
DECOMP(00316308, Climb_Restart_00316308) {
    const u32 s = arg<u32>(ctx, 0);
    const u32 n = load<u32>(s + 28) + 1;
    const u32 a = load<u32>(s + 32);
    store<u32>(s + 28, n);
    store<u32>(s + 36, 0);
    store<u32>(s + 76, 0x7883E3B1u);
    store<u32>(s + 68, 0x785F53FEu);
    store<u32>(s + 72, 0x67DC7D75u);
    const f64 one = Lfs48(0x00316300u);
    const f64 sp = F48(Lfs48(0x010699F0u + 72) * Lfs48(0x01068FC0u + 72));
    ctx.fpr[1] = Lfs48(0x01068FC0u + 72);
    u32 c = a;
    if (load<u32>(a + 668) != 0x785F53FEu) {
        f64 rate = one;
        if (load<u8>(a + 700) == 0) rate = Lfs48(0x01068F70u + 72);
        const u32 blend = rate == one ? 0u : 1u;
        const u32 an = static_cast<u32>(VcOE48(ctx, a, 60, {a}));
        // r8 and the stack words sp+112 / sp+120 (arguments 9 and 10) are
        // whatever is there in the original; the callee reads neither.
        callf(ctx, 0x002775A8u, {an, 0x785F53FEu, 0, 0, ~0ull, 0, 0, 0, 0, 0, 0, ~0ull}, {sp, static_cast<f64>(blend), one, one});
        store<u8>(a + 700, 0);
        store<u32>(a + 668, 0x785F53FEu);
        c = load<u32>(s + 32);
    }
    call(ctx, 0x00308F5Cu, {load<u32>(c + 872), 0});
    ret(ctx, static_cast<u32>(VcOE48(ctx, s, 28, {s, 0, 1})));
}

// u32 f(Obj* o; f32 dt): ticks the two state blocks o->+104 and o->+108:
// each, when +4, +64, +68, +72 and +108 are all set, runs
// Obj_StateTick3D1AD8 (u8 +92) or Obj_StateTick3CF648 (blk; dt) and, when
// its state +76 did not change, f32 +96 += dt. When the first block
// started in state 7 or 8 and has left both, o->+80 = o->+84 = 0. r3 / f1
// are what the last step left.
DECOMP(003D2514, Obj_TickStates_003D2514) {
    const u32 o = arg<u32>(ctx, 0);
    const f64 dt = farg(ctx, 0);
    u32 r3 = 0;
    auto tick = [&](u32 b, u32 k0, bool k_late) {
        static constexpr u32 kOff[5] = {4, 64, 68, 72, 108};
        for (u32 off : kOff) {
            r3 = load<u32>(b + off);
            if (r3 == 0) return;
        }
        const u32 k = k_late ? load<u32>(b + 76) : k0;
        callf(ctx, load<u8>(b + 92) != 0 ? 0x003D1AD8u : 0x003CF648u, {b}, {dt});
        r3 = load<u32>(b + 76);
        if (k == r3) {
            const f64 v = F48(Lfs48(b + 96) + dt);
            store<u32>(b + 96, W48(v));
            ctx.fpr[1] = v;
        }
    };
    const u32 a = load<u32>(o + 104);
    const u32 k = load<u32>(a + 76);
    const bool was = k == 7 || k == 8;
    tick(a, k, false);
    tick(load<u32>(o + 108), 0, true);
    if (was) {
        r3 = load<u32>(load<u32>(o + 104) + 76);
        if (r3 != 7 && r3 != 8) {
            store<u32>(o + 80, 0);
            store<u32>(o + 84, 0);
        }
    }
    ret(ctx, r3);
}

// u32 f(a, Obj* b): drops the {a, b} entry from the 16-byte table
// *0x00F65858 (count 0x010AA690): b->vt[180](b, 0), b->vt[96](b, bit 26 of
// the entry's +12); when b->+164 is set its u16 +40 bits 12 / 10 take
// bits 27 / 25 of +12; b->vt[40](b, 0); then memmove(entry, last entry,
// 13) (13 bytes, as the original) and --count (r3 = the new count). r3 = a
// when not found.
DECOMP(0042C7DC, PairTable_Remove_0042C7DC) {
    const u32 a = arg<u32>(ctx, 0), b = arg<u32>(ctx, 1);
    constexpr u32 kCnt = 0x010AA690u, kTab = 0x00F65858u;
    const u32 n = load<u32>(kCnt);
    if (n == 0) {
        ret(ctx, a);
        return;
    }
    u32 tab = load<u32>(kTab), i = 0, e = 0;
    for (;;) {
        e = tab + 16 * i;
        if (load<u32>(e + 4) == b && load<u32>(e) == a) break;
        ++i;
        if (!(n > i)) {
            ret(ctx, a);
            return;
        }
    }
    u32 ob = load<u32>(e + 4);
    VcOE48(ctx, ob, 180, {ob, 0});
    ob = load<u32>(e + 4);
    VcOE48(ctx, ob, 96, {ob, (load<u32>(e + 12) & 0x04000000u) != 0 ? 1u : 0u});
    ob = load<u32>(e + 4);
    u32 g = load<u32>(ob + 164);
    if (g != 0) {
        const u32 f1 = (load<u32>(e + 12) & 0x08000000u) != 0 ? 1u : 0u;
        store<u16>(g + 40, static_cast<u16>((load<u16>(g + 40) & ~0x1000u) | (f1 << 12)));
        const u32 f2 = (load<u32>(e + 12) & 0x02000000u) != 0 ? 1u : 0u;
        g = load<u32>(load<u32>(e + 4) + 164);
        store<u16>(g + 40, static_cast<u16>((load<u16>(g + 40) & ~0x400u) | (f2 << 10)));
        ob = load<u32>(e + 4);
    }
    VcOE48(ctx, ob, 40, {ob, 0});
    tab = load<u32>(kTab);
    call(ctx, 0x0071C3F0u, {tab + (i << 4), tab + ((load<u32>(kCnt) - 1) << 4), 13});
    const u32 c = load<u32>(kCnt) - 1;
    store<u32>(kCnt, c);
    ret(ctx, c);
}

// u32 f(Track* t, Anim* a, Pick* out, d): fills a pick record from the
// 76-byte clip entry a->+12[s16 t->+60] (null when out of range of the u16
// a->+4->+2; its fields are then read from low memory, unchecked): out->+0
// = 1, +20..32 = entry +8..+20, u8 +41 / +42 = bits 6 / 2 of u8 t->+20, f32
// +44 / +52 = t->+28 / +32; r3 = Table_PickFinish_004B36A0(t, a, out, d).
DECOMP(004B3D84, Pick_Fill_004B3D84) {
    const u32 t = arg<u32>(ctx, 0), a = arg<u32>(ctx, 1), out = arg<u32>(ctx, 2), d = arg<u32>(ctx, 3);
    store<u32>(out, 1);
    const s32 i = static_cast<s16>(load<u16>(t + 60));
    auto entry = [&]() -> u32 {
        if (i < 0 || !(i < static_cast<s32>(load<u16>(load<u32>(a + 4) + 2)))) return 0u;
        return load<u32>(a + 12) + static_cast<u32>(i) * 76u;
    };
    const u32 f2 = load<u32>(t + 28), f1 = load<u32>(t + 32);
    store<u32>(out + 20, load<u32>(entry() + 8));
    store<u32>(out + 24, load<u32>(entry() + 12));
    store<u32>(out + 28, load<u32>(entry() + 16));
    store<u32>(out + 32, load<u32>(entry() + 20));
    store<u8>(out + 41, (load<u8>(t + 20) & 0x40u) != 0 ? 1 : 0);
    const u8 b20 = load<u8>(t + 20);
    store<u32>(out + 44, f2);
    store<u32>(out + 52, f1);
    store<u8>(out + 42, (b20 & 0x04u) != 0 ? 1 : 0);
    ctx.fpr[1] = Lfs48(t + 32);  // f1 / f2 as loaded (the callee does not read them)
    ctx.fpr[2] = Lfs48(t + 28);
    ret(ctx, static_cast<u32>(call(ctx, 0x004B36A0u, {t, a, out, d})));
}

// u32 f(Track* t, XmlCursor* x, Anim* a, d): AnimClip_ReadBase4B31D8(t, x,
// a, d), then from the child 0x00AEE340 of x the clip entry a->+12[s16
// t->+60] (null out of range: the writes then go to low memory, unchecked)
// gets +8 = the attribute 0x00AEE734 interned in the string buffer
// *0x00F65F84 (StrBuf_Append) and +12 = its String_HashNoCase, likewise
// +16 / +20 from 0x00AEE784 when present. A missing 0x00AEE734 is reported
// with String_Format(0x00AEE744, Xml_Where_004F61C4(child)).
DECOMP(004B3F28, AnimClip_ReadNames_004B3F28) {
    const u32 t = arg<u32>(ctx, 0), x = arg<u32>(ctx, 1), a = arg<u32>(ctx, 2), d = arg<u32>(ctx, 3);
    call(ctx, 0x004B31D8u, {t, x, a, d});
    const u32 c = static_cast<u32>(call(ctx, 0x004F936Cu, {x, 0x00AEE340u}));
    const s32 i = static_cast<s16>(load<u16>(t + 60));
    const bool in = i >= 0 && i < static_cast<s32>(load<u16>(load<u32>(a + 4) + 2));
    const u32 e0 = in ? load<u32>(a + 12) + static_cast<u32>(i) * 76u : 0u;
    if (e0 + 4 == 0 || c == 0) {  // the first test is never true (kept)
        ret(ctx, 0u);
        return;
    }
    const bool in2 = i >= 0 && i < static_cast<s32>(load<u16>(load<u32>(a + 4) + 2));
    const u32 e = in2 ? load<u32>(a + 12) + static_cast<u32>(i) * 76u : 0u;
    constexpr u32 kBuf = 0x00F65F84u;
    const u32 s = static_cast<u32>(call(ctx, 0x004F9628u, {c, 0x00AEE734u}));
    store<u32>(e + 8, s);
    if (s == 0) {
        const u32 w = static_cast<u32>(call(ctx, 0x004F61C4u, {c}));
        ret(ctx, static_cast<u32>(call(ctx, 0x005534D0u, {0x00AEE744u, w})));
        return;
    }
    u32 v = static_cast<u32>(call(ctx, 0x00566F1Cu, {load<u32>(kBuf), s}));
    store<u32>(e + 8, v);
    store<u32>(e + 12, static_cast<u32>(call(ctx, 0x0056A710u, {v})));
    const u32 s2 = static_cast<u32>(call(ctx, 0x004F9628u, {c, 0x00AEE784u}));
    store<u32>(e + 16, s2);
    if (s2 == 0) {
        ret(ctx, 0u);
        return;
    }
    v = static_cast<u32>(call(ctx, 0x00566F1Cu, {load<u32>(kBuf), s2}));
    store<u32>(e + 16, v);
    const u32 h = static_cast<u32>(call(ctx, 0x0056A710u, {v}));
    store<u32>(e + 20, h);
    ret(ctx, h);
}

// T* f(Obj* o): deleting destructor (null o: r3 = 0). vtable 0x00B0E888,
// PtrVector_Clear + Nop_54EC18 on the vectors +492, +472, +452,
// SmallFree(o->+428, 16 * o->+444), the vectors +408, +388, +368, +348,
// +328, +308, base vtable 0x00B0DC70, NavQuery_Free62D08C(o + 216); r3 =
// Mem_FreeFrom(o, 0).
DECOMP(00739464, Obj_DeleteB0E888_00739464) {
    const u32 o = arg<u32>(ctx, 0);
    if (o == 0) {
        ret(ctx, 0u);
        return;
    }
    auto clear = [&](u32 off) {
        call(ctx, 0x0054EC1Cu, {o + off});
        call(ctx, 0x0054EC18u, {o + off});
    };
    store<u32>(o, 0x00B0E888u);
    clear(492);
    clear(472);
    clear(452);
    call(ctx, 0x00552420u, {load<u32>(o + 428), load<u32>(o + 444) << 4});
    for (u32 off : {408u, 388u, 368u, 348u, 328u, 308u}) clear(off);
    store<u32>(o, 0x00B0DC70u);
    call(ctx, 0x0062D08Cu, {o + 216});
    ret(ctx, static_cast<u32>(call(ctx, 0x0054EE6Cu, {o, 0})));
}

// ---------------------------------------------------------------- sweep: 106 instructions
// T* f(Obj* o): picks one of the u16 o->+396 8-byte spawn entries of
// o->+388 (r3 = 0 without any): the entry whose point (+0 -> +128, else
// 0x01164B40) gives the largest score above *0x000447DC among those in
// front (dot with the direction o->+160 / +168 offset by o->+164 * c >= c,
// c = *0x000447E0), the score being the squared x/z distances from the
// player ((*0x00F65CA0)->+4216, +144 when bit 1 of +192, else 0x01164B40)
// and from o->+176 / +184 plus 2c^2; failing that a random entry (the
// generator at 0x01164888: seed +8, the low half stored at +4).
DECOMP(000447E8, Spawn_Pick_000447E8) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 n = load<u16>(o + 396);
    const u32 arr = load<u32>(o + 388);
    if (n == 0) {
        ret(ctx, 0u);
        return;
    }
    constexpr u32 kDef = 0x01164B40u, kC = 0x000447DCu;
    u32 pl = load<u32>(load<u32>(kGameRoot48) + 4208 + 8);
    pl = (pl != 0 && (load<u32>(pl + 192) & 2u) != 0) ? pl + 144 : kDef;
    auto fma_s = [](f64 a, f64 c, f64 b) { return F48(std::fma(a, c, b)); };
    f64 best = Lfs48(kC);
    const f64 px = Lfs48(pl), c = Lfs48(kC + 4), pz = Lfs48(pl + 8);
    const f64 ox = Lfs48(o + 176), oz = Lfs48(o + 184);
    const f64 c2 = F48(c * c), dx = Lfs48(o + 160), dz = Lfs48(o + 168);
    const f64 off = F48(Lfs48(o + 164) * c);
    s32 pick = -1;
    for (u32 i = 0; i < n; ++i) {
        const u32 e = load<u32>(arr + 8 * i);
        const u32 pt = e != 0 ? e + 128 : kDef;
        const f64 x = Lfs48(pt), z = Lfs48(pt + 8);
        const f64 a0 = F48(x - px), a1 = F48(z - pz), b0 = F48(x - ox), b1 = F48(z - oz);
        f64 s = fma_s(a0, a0, c2);
        s = fma_s(a1, a1, s);
        s = fma_s(b0, b0, s);
        s = fma_s(b1, b1, s);
        s = fma_s(c, c, s);
        if (s > best) {
            const f64 d = fma_s(dz, b1, fma_s(dx, b0, off));
            if (d >= c) {
                pick = static_cast<s32>(i);
                best = s;
            }
        }
    }
    ctx.fpr[1] = off;
    if (pick == -1) {
        constexpr u32 R = 0x01164888u;
        const u32 sd = load<u32>(R + 8);
        pick = static_cast<s32>(sd - (sd / n) * n);
        const u32 m = (((sd << 4) | (sd >> 28)) + ((sd << 16) | (sd >> 16))) ^ 0x87654321u;  // rotl 4 + rotl 16
        store<u32>(R + 8, (m << 16) | (m >> 16));
        store<u32>(R, 0);
        store<u32>(R + 4, m & 0xFFFFu);
    }
    ret(ctx, arr + (static_cast<u32>(pick) << 3));
}

// u32 f(Cmd* c, Owner* x): animation-speed command. With c->+24 set, the
// actor is Effect_Bind4BB7E0(x, c->+12, 0 / 1) (its +280 when +276 is
// 0x02235C33), or the object Obj_FindById(c->+16) (r29 = its +160,
// unchecked); its ->vt[60]()->+36 is the anim set, else
// Effect_GetComp36_004CF2BC(x, c->+12, r29). Then with an = Anim_Find34CC34(
// set, c->+20) and the component comp = Effect_FindComponent(x, c->+24)
// answering 1 to vt[32]: f32 comp->+20 = Field_Get32_64f_003541B0(an).
DECOMP(0007A8F4, Cmd_AnimSpeed_0007A8F4) {
    const u32 c = arg<u32>(ctx, 0), x = arg<u32>(ctx, 1);
    if (load<u32>(c + 24) == 0) {
        ret(ctx, 0u);
        return;
    }
    u32 r29 = 0, r3 = load<u32>(c + 12);
    if (r3 != 0) {
        r29 = static_cast<u32>(call(ctx, 0x004BB7E0u, {x, r3, 0}));
        if (r29 == 0) r29 = static_cast<u32>(call(ctx, 0x004BB7E0u, {x, load<u32>(c + 12), 1}));
        if (r29 == 0) {
            ret(ctx, 0u);
            return;
        }
        r3 = load<u32>(r29 + 276) == 0x02235C33u ? load<u32>(r29 + 280) : 0u;
    } else {
        r3 = load<u32>(c + 16);
        if (r3 != 0) {
            r3 = static_cast<u32>(call(ctx, 0x000D6A28u, {r3}));
            r29 = load<u32>(r3 + 160);
        }
    }
    u32 set = 0;
    if (r3 != 0) {
        const u32 q = static_cast<u32>(VcOE48(ctx, r3, 60, {r3}));
        if (q != 0) set = load<u32>(q + 36);
    }
    if (set == 0) {
        set = static_cast<u32>(call(ctx, 0x004CF2BCu, {x, load<u32>(c + 12), r29}));
        if (set == 0) {
            ret(ctx, 0u);
            return;
        }
    }
    const u32 an = static_cast<u32>(call(ctx, 0x0034CC34u, {set, load<u32>(c + 20)}));
    if (an == 0) {
        ret(ctx, 0u);
        return;
    }
    const u32 comp = static_cast<u32>(call(ctx, kFindComp48, {x, load<u32>(c + 24)}));
    if (comp == 0) {
        ret(ctx, 0u);
        return;
    }
    const u32 k = static_cast<u32>(VcOE48(ctx, comp, 32, {comp}));
    if (k != 1) {
        ret(ctx, k);
        return;
    }
    const u64 r = call(ctx, 0x003541B0u, {an});
    store<u32>(comp + 20, W48(ctx.fpr[1]));
    ctx.gpr[3] = r;
}

// u32 f(Cmd* c, Owner* x): visibility command. The target t is the
// component Effect_FindComponent(x, c->+12) answering 5 (its +28, then +280
// when +276 is 0x02235C33), else Target_Resolve_0009A190(c, c->+16, x);
// refined by Obj_FindByHash_000DB290(t, c->+20) when +20 is set. Without
// t: Hash_ToString(c->+16), Hash_ToString(c->+12). With an owner
// Obj_Owner0D93FC(t) bit 12 of its +40 = bit 0 of u8 c->+24; else, when bit
// 1 of t->+192 is set, Node_SetFlags64_007348D8(t, 0x40000, u8 c->+24,
// u8 c->+25), otherwise Hash_ToString(c->+16).
DECOMP(0008B360, Cmd_SetVisible_0008B360) {
    const u32 c = arg<u32>(ctx, 0), x = arg<u32>(ctx, 1);
    u32 t = 0;
    const u32 k = load<u32>(c + 12);
    if (k != 0) {
        const u32 p = static_cast<u32>(call(ctx, kFindComp48, {x, k}));
        if (p != 0 && static_cast<u32>(VcOE48(ctx, p, 32, {p})) == 5) {
            const u32 q = load<u32>(p + 28);
            if (q != 0 && load<u32>(q + 276) == 0x02235C33u) t = load<u32>(q + 280);
        }
    }
    if (t == 0) {
        const u32 h = load<u32>(c + 16);
        if (h != 0) t = static_cast<u32>(call(ctx, 0x0009A190u, {c, h, x}));
    }
    if (t != 0 && load<u32>(c + 20) != 0) t = static_cast<u32>(call(ctx, 0x000DB290u, {t, load<u32>(c + 20)}));
    if (t == 0) {
        call(ctx, kHashStr48, {load<u32>(c + 16)});
        ret(ctx, static_cast<u32>(call(ctx, kHashStr48, {load<u32>(c + 12)})));
        return;
    }
    const u32 ow = static_cast<u32>(call(ctx, 0x000D93FCu, {t}));
    if (ow != 0) {
        store<u32>(ow + 40, (load<u32>(ow + 40) & ~0x1000u) | ((load<u8>(c + 24) & 1u) << 12));
        ret(ctx, ow);
        return;
    }
    if ((load<u32>(t + 192) & 2u) != 0) {
        ret(ctx, static_cast<u32>(call(ctx, 0x007348D8u, {t, 0x00040000u, load<u8>(c + 24), load<u8>(c + 25)})));
        return;
    }
    ret(ctx, static_cast<u32>(call(ctx, kHashStr48, {load<u32>(c + 16)})));
}

// u32 f(Obj* o): grab release check. A held object h = o->+872 is dropped
// right away when bit 18 of h->+172 is clear (bit 29 of o->+980 cleared,
// h->vt[40](h, 0), +872 = 0, Handle_Notify36742C(*0x00F64D0C, 0, 1, 0)).
// Otherwise, with bit 29 of +980 set and the grab range r =
// Field_Get372At_12_00271C90(CallK2_000D93D4(h), 3) (both non-null), the
// same drop happens once the squared distance between o->+144 and h->+144
// exceeds r->+4 squared (f1 = that distance). r3 = what the last step left.
DECOMP(001EE990, Grab_CheckRelease_001EE990) {
    const u32 o = arg<u32>(ctx, 0);
    constexpr u32 kN = 0x00F64D0Cu;
    u32 h = load<u32>(o + 872);
    if (h != 0 && (load<u32>(h + 172) & 0x00040000u) == 0) {
        store<u32>(o + 980, load<u32>(o + 980) & ~0x20000000u);
        VcOE48(ctx, h, 40, {h, 0});
        store<u32>(o + 872, 0);
        call(ctx, 0x0036742Cu, {load<u32>(kN), 0, 1, 0});
        h = load<u32>(o + 872);
    }
    if (h == 0 || (load<u32>(o + 980) & 0x20000000u) == 0) {
        ret(ctx, h);
        return;
    }
    if (static_cast<u32>(call(ctx, 0x000D93D4u, {h})) == 0) {
        ret(ctx, 0u);
        return;
    }
    const u32 q = static_cast<u32>(call(ctx, 0x000D93D4u, {load<u32>(o + 872)}));
    const u32 r = static_cast<u32>(call(ctx, 0x00271C90u, {q, 3}));
    if (r == 0) {
        ret(ctx, 0u);
        return;
    }
    const u32 a = o + 144, b = load<u32>(o + 872) + 144;
    const f64 dy = F48(Lfs48(b + 4) - Lfs48(a + 4)), dx = F48(Lfs48(b) - Lfs48(a));
    const f64 dz = F48(Lfs48(b + 8) - Lfs48(a + 8));
    const f64 rr = F48(Lfs48(r + 4) * Lfs48(r + 4));
    const f64 d2 = F48(std::fma(dz, dz, F48(std::fma(dx, dx, F48(dy * dy)))));
    ctx.fpr[1] = d2;
    if (!(rr < d2)) {
        ret(ctx, r);
        return;
    }
    call(ctx, 0x0036742Cu, {load<u32>(kN), 0, 1, 0});
    h = load<u32>(o + 872);
    if (h != 0) VcOE48(ctx, h, 40, {h, 0});
    const u32 f = load<u32>(o + 980) & ~0x20000000u;
    store<u32>(o + 872, 0);
    store<u32>(o + 980, f);
    ret(ctx, 0u);
}

// u32 f(Obj* o, u8 want): effect refresh for the target o->+48 when bit 15
// of its +164 is set (r3 = o->+8 otherwise). want is forced on when the
// first non-null entry of CallK2_000D93D4(o->+8)->+372's list (+16, count
// +36) has bit 3 of +192, (+172 >> 22) & 3 == 1 and a +244. want with no
// effect in o->+188 and a clear top nibble of o->+244:
// Effect_Bind4A63FC(o + 188, o->+156, 0, o->+48, o->+52, 1); no want with
// an effect: Effect_Bind4A6558(o + 188, 0x00AD4B7C, 0x00AD4B7C, o->+48,
// o->+52, 1). r3 = what the last step left.
DECOMP(002ABB60, Obj_RefreshTargetFx_002ABB60) {
    const u32 o = arg<u32>(ctx, 0);
    u32 want = arg<u32>(ctx, 1) & 0xFFu;
    const u32 tg = load<u32>(o + 48);
    u32 r3 = load<u32>(o + 8);
    if (tg == 0 || (load<u32>(tg + 164) & 0x8000u) == 0) {
        ret(ctx, r3);
        return;
    }
    if (r3 != 0) {
        r3 = static_cast<u32>(call(ctx, 0x000D93D4u, {r3}));
        if (r3 != 0) {
            r3 = static_cast<u32>(call(ctx, 0x000D93D4u, {load<u32>(o + 8)}));
            if (r3 != 0) {
                const u32 l = load<u32>(r3 + 372);
                r3 = 0;
                const u32 n = load<u32>(l + 36);
                if (n != 0) {
                    const u32 arr = load<u32>(l + 16);
                    while (load<u32>(arr + 8 * r3) == 0) {
                        ++r3;
                        if (!(r3 < n)) break;
                    }
                    if (r3 < n) {
                        const u32 e = load<u32>(arr + ((r3 << 3) & ~7u));
                        r3 = e;
                        if (e != 0 && (load<u32>(e + 192) & 8u) != 0 && ((load<u32>(e + 172) >> 22) & 3u) == 1) {
                            r3 = load<u32>(e + 244);
                            if (r3 != 0) want = 1;
                        }
                    }
                }
            }
        }
    }
    const u32 fx = o + 188;
    if (want != 0) {
        r3 = load<u32>(fx);
        if (r3 == 0) {
            r3 = load<u32>(o + 244) >> 28;
            if (r3 == 0) {
                const u32 a = load<u32>(o + 156), b = load<u32>(o + 48), c = load<u32>(o + 52);
                r3 = static_cast<u32>(call(ctx, 0x004A63FCu, {fx, a, 0, b, c, 1}));
            }
        }
        ret(ctx, r3);
        return;
    }
    if (load<u32>(fx) == 0) {
        ret(ctx, 0u);
        return;
    }
    const u32 b = load<u32>(o + 48), c = load<u32>(o + 52);
    ret(ctx, static_cast<u32>(call(ctx, 0x004A6558u, {fx, 0x00AD4B7Cu, 0x00AD4B7Cu, b, c, 1})));
}

// u32 f(Pair* p, Ent* m): posts Msg_Post(m->+12, c1, c0, c2) with the cells
// k = 1, 0, 2 of the 128-byte table rows *0x00F65038 (row s16 p->+4,
// sub-block of row s16 p->+2, TblCell46) unless the first one is 0; r3 = 0.
DECOMP(0039379C, Pair_PostCells_0039379C) {
    const u32 p = arg<u32>(ctx, 0), m = arg<u32>(ctx, 1);
    const s32 a = static_cast<s16>(load<u16>(p + 4)), b = static_cast<s16>(load<u16>(p + 2));
    const u32 tgt = load<u32>(m + 12);
    const u32 T = load<u32>(0x00F65038u);
    const u32 c1 = load<u32>(TblCell46(T, a, b, 1));
    if (c1 != 0) {
        const u32 c0 = load<u32>(TblCell46(T, a, b, 0));
        const u32 c2 = load<u32>(TblCell46(T, a, b, 2));
        call(ctx, 0x00470660u, {tgt, c1, c0, c2});
    }
    ret(ctx, 0u);
}

// u32 f(Obj* o): stop. Effect_Release(o->+192->vt[272](), 0x002237D4);
// with u8 o->+68: World_ListRemove_0046B7A4(*0x00F65CA0, 0, 0x00F38760, o,
// 0), Handle_Notify36742C(0xBB9F16F3, 0, 1, 0), SetSelect_491000(o,
// 0xB3000DC2), o->+92->vt[88](0, 1, 1, 1), the layer o->+60 popped
// (Obj_PopLayer(l, o->+60, o->+60; *0x00412EDC)) from l = o->vt[76](o)->
// +0x1281E0 when Obj_HasWord368_000FC5DC(l, o->+60), and r3 =
// Obj_Child28_Vfn32(0x010B19E0 + 11968 * u8 (*0x00F65CA0)->+4209 + 176).
DECOMP(00412EE0, Obj_Stop_00412EE0) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 s = load<u32>(o + 192);
    const u32 q = static_cast<u32>(VcOE48(ctx, s, 272, {s}));
    call(ctx, 0x004CF3C4u, {q, 0x002237D4u});
    if (load<u8>(o + 68) == 0) {
        ret(ctx, 0u);  // r3 = the u8 just tested
        return;
    }
    call(ctx, 0x0046B7A4u, {load<u32>(kGameRoot48), 0, 0x00F38760u, o, 0});
    call(ctx, 0x0036742Cu, {0xBB9F16F3u, 0, 1, 0});
    call(ctx, 0x00491000u, {o, 0xB3000DC2u});
    const u32 v = load<u32>(o + 92);
    VcOE48(ctx, v, 88, {v, 0, 1, 1, 1});
    const u32 g = static_cast<u32>(VcOE48(ctx, o, 76, {o}));
    const u32 l = load<u32>(g + 0x001281E0u);
    if (l != 0 && static_cast<u32>(call(ctx, 0x000FC5DCu, {l, load<u32>(o + 60)})) != 0) {
        const u32 k = load<u32>(o + 60);
        callf(ctx, 0x000FBB88u, {l, k, k}, {Lfs48(0x00412EDCu)});
    }
    const u32 i = load<u8>(load<u32>(kGameRoot48) + 4208 + 1);
    ctx.gpr[3] = call(ctx, 0x004644E0u, {0x010B19E0u + i * 11968u + 176});
}

// u32 f(Cmd* c): applies a layer command to the layer stack L =
// (*0x00F65CA0)->+0x1281E0 from its top (+364[u8 +345]) down
// (Obj_LayerBelow): when f32 c->+12 > *0x0042D4C4, L->+592 = c->+12,
// L->vt[212](L, 0) and +592 = -1; otherwise the first layer whose vt[8]
// answers 11 gets Target_Reset0FEACC and vt[28]. r3 =
// Flag_Set8_40000000(c); f1 = c->+12 as last loaded.
DECOMP(0042D4C8, Cmd_LayerApply_0042D4C8) {
    const u32 c = arg<u32>(ctx, 0);
    const u32 L = load<u32>(load<u32>(kGameRoot48) + 0x001281E0u);
    if (L != 0) {
        const u32 b = load<u8>(L + 345);
        u32 l = b != 0 ? load<u32>(L + b * 4 + 364) : 0u;
        const f64 lim = Lfs48(0x0042D4C4u);
        while (l != 0) {
            const u32 k = static_cast<u32>(VcOE48(ctx, l, 8, {l}));
            ctx.fpr[1] = Lfs48(c + 12);
            if (ctx.fpr[1] > lim) {
                Cp48(L + 592, c + 12);
                VcOE48(ctx, L, 212, {L, 0});
                store<u32>(L + 592, 0xBF800000u);
                break;
            }
            if (k == 11) {
                call(ctx, 0x000FEACCu, {l});
                VcOE48(ctx, l, 28, {l});
                break;
            }
            l = static_cast<u32>(call(ctx, 0x000FCBD0u, {L, l}));
        }
    }
    ret(ctx, static_cast<u32>(call(ctx, 0x0042CA60u, {c})));
}

// u32 f(Gfx* g, View* v): end-of-frame sync. m = v->+672 is locked
// (Call_00548508). With the pending buffer *(0x010BBE0C + 8 * (u8
// 0x010BBE1C ^ 1)) it is flushed (0x004D28F8); otherwise, for the two
// stream descriptors 0x010BBE2C / 0x010BBE38 (u8 +9 set): FrameBuffer_Reset_00523584(
// *0x010DDBB4, q->+8, q->+12) for q = +4 (when q->+12) and
// CallbackList_Invoke(*0x00F65C90->+12, (+0)->+28) when set, with
// Obj_MemberVCall_004F2FF4(g->+0x1281C4, m) in between. r3 =
// Sync_00548550(m, 0). FrameBuffer_Reset reads r7 as left by the call
// before it, which is passed on explicitly.
DECOMP(0045717C, Gfx_FrameSync_0045717C) {
    const u32 g = arg<u32>(ctx, 0), v = arg<u32>(ctx, 1);
    const u32 m = load<u32>(v + 672);
    constexpr u32 P = 0x010BBE0Cu;
    const u32 pend = load<u32>(P + ((load<u8>(P + 16) ^ 1u) << 3));
    call(ctx, 0x00548508u, {m, v});
    if (pend != 0) {
        call(ctx, 0x004D28F8u, {pend});
    } else {
        auto stream = [&](u32 S, bool r6_is_p) {
            if (load<u8>(S + 9) == 0) return;
            const u32 q = load<u32>(S + 4);
            const u32 fb = load<u32>(0x010DDBB4u);
            const u32 b = load<u32>(q + 12);
            if (b != 0) call(ctx, 0x00523584u, {fb, load<u32>(q + 8), b, r6_is_p ? q : b, ctx.gpr[7]});
            const u32 cb = load<u32>(load<u32>(S) + 28);
            const u32 l = load<u32>(load<u32>(0x00F65C90u) + 12);
            if (cb != 0) call(ctx, 0x005651B0u, {l, cb, cb});
        };
        stream(0x010BBE2Cu, true);
        const u32 h = load<u32>(g + 0x001281C4u);
        if (h != 0) call(ctx, 0x004F2FF4u, {h, m});
        stream(0x010BBE38u, false);
    }
    ret(ctx, static_cast<u32>(call(ctx, 0x00548550u, {m, 0})));
}

// u32 f(Mgr* m, Obj* o): frees o's effects (u16 o->+120 pointers at
// o->+112; Effect_Destruct + SmallFree(e, 140)): all of them (count reset
// to 0) when u8 m->+16 is set, else only those whose +0->+8 is the owner
// m->+12, swap-removing each from the list. r3 = what the last step left.
DECOMP(004C94C8, Effects_FreeOwned_004C94C8) {
    const u32 m = arg<u32>(ctx, 0), o = arg<u32>(ctx, 1);
    u32 r3 = m;
    if (o == 0) {
        ret(ctx, r3);
        return;
    }
    auto destroy = [&](u32 e) {
        call(ctx, 0x004C8E80u, {e});
        call(ctx, 0x00552420u, {e, 140});
    };
    if (load<u8>(m + 16) != 0) {
        r3 = load<u16>(o + 120);
        if (r3 == 0) {
            ret(ctx, 0u);
            return;
        }
        for (u32 i = 0; i < r3; ++i) {
            const u32 e = load<u32>(load<u32>(o + 112) + 4 * i);
            if (e != 0) {
                destroy(e);
                r3 = load<u16>(o + 120);
            }
        }
        store<u16>(o + 120, 0);
        ret(ctx, r3);
        return;
    }
    const u32 k = load<u32>(m + 12);
    if (k == 0) {
        ret(ctx, r3);
        return;
    }
    r3 = load<u16>(o + 120);
    if (r3 == 0) {
        ret(ctx, 0u);
        return;
    }
    for (s32 i = 0; i < static_cast<s32>(r3); ++i) {
        const u32 e = load<u32>(load<u32>(o + 112) + 4 * static_cast<u32>(i));
        if (e == 0) continue;
        const u32 h = load<u32>(e);
        if ((h != 0 ? load<u32>(h + 8) : 0u) != k) continue;
        destroy(e);
        const u32 n = load<u16>(o + 120) - 1;
        const u32 arr = load<u32>(o + 112);
        store<u16>(o + 120, static_cast<u16>(n));
        r3 = n & 0xFFFFu;
        if (static_cast<u32>(i) != r3) {
            const u32 last = load<u32>(arr + ((n << 2) & 0x3FFFCu));
            r3 = load<u16>(o + 120);
            store<u32>(arr + 4 * static_cast<u32>(i), last);
        }
        --i;  // the slot is examined again
    }
    ret(ctx, r3);
}

// The shared body of the two UI widget destructors below: with a child
// object at w->+off, vtable vt and child->vt[8](child); base vtable
// 0x00B22710; every child in the list w->+152 released (vt[4], which
// unlinks it); unless bit 24 of +136 is set, w unlinked from its parent's
// child list (+148->+152, or the root list *0x00F66868 without a parent;
// siblings chained by +156; no end check, as the original) and +156 = 0;
// w cleared from the 4 focus slots 0x010C2AD8 (all of them for a null w).
// r3 = 0x010C2AD8.
static void WidgetDtorBody48(PPUContext& ctx, u32 vt, u32 off) {
    const u32 w = arg<u32>(ctx, 0);
    u32 c = load<u32>(w + off);
    if (c != 0) {
        store<u32>(w, vt);
        VcOE48(ctx, c, 8, {c});
    }
    store<u32>(w, 0x00B22710u);
    for (c = load<u32>(w + 152); c != 0; c = load<u32>(w + 152)) VcOE48(ctx, c, 4, {c});
    if ((load<u32>(w + 136) & 0x01000000u) == 0) {
        constexpr u32 kRoot = 0x00F66868u;
        const u32 p = load<u32>(w + 148);
        u32 n = p != 0 ? load<u32>(p + 152) : load<u32>(kRoot);
        u32 prev = 0;
        if (n != w) {
            do {
                prev = n;
                n = load<u32>(prev + 156);
            } while (n != w);
        }
        if (prev != 0) store<u32>(prev + 156, load<u32>(w + 156));
        else if (p != 0) store<u32>(p + 152, load<u32>(w + 156));
        else store<u32>(kRoot, load<u32>(w + 156));
        store<u32>(w + 156, 0);
    }
    constexpr u32 kFocus = 0x010C2AD8u;
    for (u32 k = 0; k < 4; ++k)
        if (w == 0 || load<u32>(kFocus + 4 * k) == w) store<u32>(kFocus + 4 * k, 0);
    ret(ctx, kFocus);
}

// void f(Widget* w): destructor body (vtable 0x00B22AB0, child +192), see
// WidgetDtorBody48.
DECOMP(004E6224, Widget_DtorB22AB0_004E6224) { WidgetDtorBody48(ctx, 0x00B22AB0u, 192); }

// void f(Widget* w): destructor body (vtable 0x00B22DD0, child +168), see
// WidgetDtorBody48.
DECOMP(004E8F54, Widget_DtorB22DD0_004E8F54) { WidgetDtorBody48(ctx, 0x00B22DD0u, 168); }

// bool f(XmlCursor* x): loads the level list named by the attribute
// 0x00AF010C of x (r3 = 1 right away when *0x010C2C28 is set with u8 +584,
// 0 without the attribute). Saves the path string 0x010C2C2C, *0x00F668D0
// and *0x010C2C28; parses the file into a stack XmlDoc (XmlDoc_Construct /
// XmlDoc_Parse with *0x00F668D0 = 0): on failure String_Format(0x00AEFC84,
// name) and r3 = 1, else the path = name and r3 = Levels_Parse(child
// 0x00AEFC6C) (0 without it); then XmlDoc_Destruct and everything restored.
DECOMP(004ED1C0, Levels_LoadFile_004ED1C0) {
    const u32 x = arg<u32>(ctx, 0);
    constexpr u32 kCur = 0x010C2C28u, kPath = 0x010C2C2Cu, kFlag = 0x00F668D0u;
    const u32 cur = load<u32>(kCur);
    if (cur != 0 && load<u8>(cur + 584) != 0) {
        ret(ctx, 1u);
        return;
    }
    const u32 a = static_cast<u32>(call(ctx, 0x004F9628u, {x, 0x00AF010Cu}));
    if (a == 0) {
        ret(ctx, 0u);
        return;
    }
    Stack fr(ctx, 2496);  // sp+112: the XmlDoc, sp+2352 / sp+2480: two 128-byte strings
    const u32 doc = fr.addr(), saved = doc + 2240, name = doc + 2368;
    const u32 oldFlag = load<u32>(kFlag), oldCur = load<u32>(kCur);
    call(ctx, 0x00555F80u, {saved, kPath});
    call(ctx, 0x00555F80u, {name, a});
    u32 r = 0;
    call(ctx, 0x004FA888u, {doc, name});
    store<u32>(kFlag, 0);
    if (static_cast<u32>(call(ctx, 0x004FAF10u, {doc, 0})) == 0) {
        call(ctx, 0x005534D0u, {0x00AEFC84u, name});
        r = 1;
    } else {
        call(ctx, 0x00555F80u, {kPath, name});
        const u32 c = static_cast<u32>(call(ctx, 0x004F936Cu, {doc, 0x00AEFC6Cu}));
        if (c != 0) r = static_cast<u32>(call(ctx, 0x004EB904u, {c}));
    }
    call(ctx, 0x004FA9C0u, {doc});
    store<u32>(kFlag, oldFlag);
    store<u32>(kCur, oldCur);
    call(ctx, 0x00555F80u, {kPath, saved});
    ret(ctx, r);
}

// u32 XmlCursor_ReadString_004FA2AC(XmlCursor* c, u32 hash, const char**
// out, const char* def): looks hash up in the attribute keys of c's node
// (u16 c->+26 indexes (c->+28)->+16, or (c->+16)->+44 + 20 without +28;
// keys {hash, value} 8 bytes at node +20, u16 +14 of them, sorted, 4-bit
// types packed 8 per word from node +24). Not found or no attributes
// (bit 7 of u8 c->+24 clear): *out = def, r3 = 1. Type 0: *out = the
// inline string (value + 4); 1: String_Format(0x00AF0EB4, (s32) *value);
// 6: String_Format(0x00AF0EB4, *value); r3 = 0. Other types: *out = def,
// r3 = 2.
DECOMP(004FA2AC, XmlCursor_ReadString_004FA2AC) {
    const u32 c = arg<u32>(ctx, 0), hash = arg<u32>(ctx, 1), out = arg<u32>(ctx, 2), def = arg<u32>(ctx, 3);
    auto dflt = [&](u32 r) {
        store<u32>(out, def);
        ret(ctx, r);
    };
    if ((load<u8>(c + 24) & 0x80u) == 0) return dflt(1);
    const u32 doc = load<u32>(c + 28);
    const u32 idx4 = load<u16>(c + 26) << 2;
    const u32 node = doc != 0 ? load<u32>(load<u32>(doc + 16) + idx4) : load<u32>(load<u32>(load<u32>(c + 16) + 44) + idx4 + 20);
    s32 hi = static_cast<s32>(load<u16>(node + 14)) - 1, lo = 0;
    while (hi >= lo) {
        const s32 mid = (hi + lo) >> 1;
        const u32 e = load<u32>(node + 20) + static_cast<u32>(mid) * 8u;
        const u32 k = load<u32>(e);
        if (k > hash) {
            hi = mid - 1;
        } else if (k < hash) {
            lo = mid + 1;
        } else {
            if (e == 0) return dflt(1);
            const u32 m = static_cast<u32>(mid);
            const u32 w = load<u32>(node + 24 + ((m >> 1) & 0x7FFFFFFCu));
            const u32 type = (w >> ((m << 2) & 0x1Cu)) & 0xFu;
            const u32 v = type == 0 ? load<u32>(e + 4) + 4 : load<u32>(e + 4);
            if (v == 0) return dflt(1);
            if (type == 0) {
                store<u32>(out, v);
                ret(ctx, 0u);
                return;
            }
            if (type == 1 || type == 6) {
                const u32 n = load<u32>(v);
                const u32 s = static_cast<u32>(call(ctx, 0x005534D0u, {0x00AF0EB4u, type == 1 ? Sx48(n) : n}));
                store<u32>(out, s);
                ret(ctx, 0u);
                return;
            }
            return dflt(2);
        }
    }
    dflt(1);
}

// T* f(Obj* o): deleting destructor (null o: r3 = 0). vtable 0x00B20558,
// PtrVector_Clear + Nop_54EC18 on +392, the array +368 (storage freed with
// allocator +380 ->vt[8](allocator, data +376, count * stride / 2),
// Array_Destruct), the vector +112, the arrays +88 and +64 likewise, then
// Dtor_B219D0_00490B34(o); r3 = SmallFree(o, 512).
DECOMP(0077E9C4, Obj_DeleteB20558_0077E9C4) {
    const u32 o = arg<u32>(ctx, 0);
    if (o == 0) {
        ret(ctx, 0u);
        return;
    }
    auto vec = [&](u32 off) {
        call(ctx, 0x0054EC1Cu, {o + off});
        call(ctx, 0x0054EC18u, {o + off});
    };
    auto arr = [&](u32 a) {
        const u32 sz = load<u16>(a) * ((load<u16>(a + 6) >> 1) & 0x7FFFu);
        const u32 data = load<u32>(a + 8);
        if (data != 0) {
            const u32 al = load<u32>(a + 12);
            VcOE48(ctx, al, 8, {al, data, sz});
        }
        call(ctx, 0x0054EC94u, {a});
    };
    store<u32>(o, 0x00B20558u);
    vec(392);
    arr(o + 368);
    vec(112);
    arr(o + 88);
    arr(o + 64);
    call(ctx, 0x00490B34u, {o});
    ret(ctx, static_cast<u32>(call(ctx, 0x00552420u, {o, 512})));
}

// ---------------------------------------------------------------- sweep: 107 instructions
// u32 f(Obj* o): plays the animation state for the current move. With the
// move table o->+612 (r3 = o without one), the record o->+612 + 116 *
// o->+716 + 92 gives the move list (+80 when bit 30 of o->+516 is set, else
// +60 with bit 31, else +20) whose 44-byte entry o->+596 has the kind
// (0..12 -> states 2, 5, 8, 12, 15, 0, 16, 17, 32, 33, 34, 35, 36, else 0);
// r3 = o->vt[220](o, state).
DECOMP(0003E0CC, Obj_PlayMoveState_0003E0CC) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 t = load<u32>(o + 612);
    if (t == 0) {
        ret(ctx, o);
        return;
    }
    const u32 rec = t + load<u32>(o + 716) * 116u + 92;
    const u32 f = load<u32>(o + 516);
    u32 list;
    if ((f & 0x40000000u) != 0) list = load<u32>(rec + 80);
    else list = (f & 0x80000000u) != 0 ? load<u32>(rec + 60) : load<u32>(rec + 20);
    const s32 kind = static_cast<s32>(load<u32>(list + load<u32>(o + 596) * 44u));
    static constexpr u32 kState[13] = {2, 5, 8, 12, 15, 0, 16, 17, 32, 33, 34, 35, 36};
    const u32 st = (kind >= 0 && kind <= 12) ? kState[kind] : 0u;
    ret(ctx, static_cast<u32>(VcOE48(ctx, o, 220, {o, st})));
}

// u32 f(Cmd* c, Owner* x): like Cmd_RunOnTarget_0042E6C0 with a component
// target: o = the +20 of Effect_FindComponent(x, c->+16) when its vt[32]
// answers 18 (Hash_ToString(c->+16) otherwise), else, for a non-zero
// c->+12, the map entry (*0x00F65BE8)->+28[c->+12] (Hash_ToString(c->+12)
// on a miss). 0xFD81 in +12 flushes the list *0x00F65BE8 instead
// (List_FlushPending_00454DBC with u8 c->+20, else 0x00454E88). With o:
// Group_Release_004563A0(o, u8 +22) when u8 +21; u8 +20 ?
// (Obj_ResetList_0045488C(o) when u8 +23, Obj_StartChain_00454220(o, u8
// +24)) : Obj_ForEachSet_004562F4(o, 0). r3 = the last result.
DECOMP(00073EAC, Cmd_RunOnComp_00073EAC) {
    const u32 c = arg<u32>(ctx, 0), x = arg<u32>(ctx, 1);
    constexpr u32 L = 0x00F65BE8u;
    u32 o = 0;
    const u32 k = load<u32>(c + 16);
    if (k != 0) {
        const u32 p = static_cast<u32>(call(ctx, kFindComp48, {x, k}));
        if (p != 0 && static_cast<u32>(VcOE48(ctx, p, 32, {p})) == 18) o = load<u32>(p + 20);
        else call(ctx, kHashStr48, {load<u32>(c + 16)});
    }
    const u32 h = load<u32>(c + 12);
    if (h == 0xFD81u) {
        ret(ctx, static_cast<u32>(call(ctx, load<u8>(c + 20) != 0 ? 0x00454DBCu : 0x00454E88u, {load<u32>(L)})));
        return;
    }
    u32 r3 = h;
    if (h != 0 && o == 0) {
        Stack fr(ctx, 16);  // sp+112: the key
        store<u32>(fr.addr(), h);
        o = load<u32>(static_cast<u32>(call(ctx, 0x0073B608u, {load<u32>(L) + 28, fr.addr()})));
        if (o == 0) r3 = static_cast<u32>(call(ctx, kHashStr48, {load<u32>(c + 12)}));
    }
    if (o == 0) {
        ret(ctx, r3);
        return;
    }
    if (load<u8>(c + 21) != 0) call(ctx, 0x004563A0u, {o, load<u8>(c + 22)});
    if (load<u8>(c + 20) != 0) {
        if (load<u8>(c + 23) != 0) call(ctx, 0x0045488Cu, {o});
        ret(ctx, static_cast<u32>(call(ctx, 0x00454220u, {o, load<u8>(c + 24)})));
        return;
    }
    ret(ctx, static_cast<u32>(call(ctx, 0x004562F4u, {o, 0})));
}

// T* f(?, XmlCursor* x): new 44-byte record (base Obj_InitVt_B21C98,
// vtable 0x00B100F8): hashes +12..+32 = 0x7A30D7E3, 0x8A93973F, 0x04B8CC71,
// 0x001F9B50, 0x93116591, 0x757BAFD6; f32 +36 = 0x04DA9951; u8 +40 = bool
// 0x9C3FA861 (all defaulting to 0).
DECOMP(00086528, XmlNew_B100F8_00086528) {
    const u32 x = arg<u32>(ctx, 1);
    const u32 m = NewXmlObj47(ctx, 44, x);
    if (m == 0) {
        ret(ctx, 0u);
        return;
    }
    store<u32>(m + 12, 0);
    store<u32>(m + 16, 0);
    store<u32>(m + 20, 0);
    store<u32>(m + 24, 0);
    store<u32>(m, 0x00B100F8u);
    store<u32>(m + 28, 0);
    store<u32>(m + 32, 0);
    store<u32>(m + 36, 0);
    store<u8>(m + 40, 0);
    call(ctx, kReadHash47, {x, 0x7A30D7E3u, m + 12, 0});
    RdHash47(ctx, x, 0x8A93973Fu, m + 16);
    RdHash47(ctx, x, 0x04B8CC71u, m + 20);
    RdHash47(ctx, x, 0x001F9B50u, m + 24);
    RdHash47(ctx, x, 0x93116591u, m + 28);
    RdHash47(ctx, x, 0x757BAFD6u, m + 32);
    RdF32_47(ctx, x, 0x04DA9951u, m + 36);
    RdBool47(ctx, x, 0x9C3FA861u, m + 40);
    ret(ctx, m);
}
