// Game code, part 31 (continued): the small-function sweep from 56
// instructions (see obj31.cpp for the notes). Each was checked against the
// original in the PPC simulator.
#include "obj31.h"

using namespace sly4;
using namespace sly4::obj31;

// ---------------------------------------------------------------- sweep: 56 instructions
// void f(T* o, const Vec3* d): +192 = d normalised (k / |d|, k =
// *0x00011C08), its y zeroed first for bit 0x40 of u8 +305 or a false
// o->vt[132]() (bit 0x20 skips both). r3 = 0x00011C08, f1 = the new y.
DECOMP(00011C10, Obj_SetDir_00011C10) {
    const u32 o = arg<u32>(ctx, 0), d = arg<u32>(ctx, 1);
    CopyW31(o + 192, d);
    const u32 b = load<u8>(o + 305);
    CopyW31(o + 196, d + 4);
    CopyW31(o + 200, d + 8);
    bool zero = false;
    if ((b & 0x20u) == 0) zero = (b & 0x40u) != 0 || static_cast<u32>(VCall(ctx, o, 132, {o})) == 0;
    if (zero) store<u32>(o + 196, 0);
    const f64 y = LfsAt(o + 196), x = LfsAt(o + 192), z = LfsAt(o + 200);
    f64 l = Mul28(y, y);
    l = Madd28(x, x, l);
    l = Madd28(z, z, l);
    l = static_cast<f32>(std::sqrt(l));
    const f64 s = static_cast<f32>(LfsAt(0x00011C08) / l);
    const f32 nx = Mul28(x, s), ny = Mul28(y, s), nz = Mul28(z, s);
    store<f32>(o + 192, nx);
    store<f32>(o + 196, ny);
    store<f32>(o + 200, nz);
    ctx.fpr[1] = ny;
    ctx.gpr[3] = 0x00011C08;
}

// T::~T(): see DtorPopAll31 (vtable 0x00B0FBA8, vector +24, count +32)
DECOMP(0007DC44, Dtor_B0FBA8_0007DC44) { DtorPopAll31(ctx, 0x00B0FBA8, 32, 24); }

// T::~T(): see DtorPopAll31 (vtable 0x00B0FBD0, vector +28, count +36)
DECOMP(0007E388, Dtor_B0FBD0_0007E388) { DtorPopAll31(ctx, 0x00B0FBD0, 36, 28); }

// T::~T(): see DtorPopAll31 (vtable 0x00B0FC20, vector +12, count +20)
DECOMP(0007F218, Dtor_B0FC20_0007F218) { DtorPopAll31(ctx, 0x00B0FC20, 20, 12); }

// void f(T* o, x): the target t (as TargetCall31, no Hash_ToString) gets
// r = CallK2_000D94F4(t); bit 0x10000000 of r->+1268 = bit 0 of u8 o->+24.
// r3 = r or 0.
DECOMP(00092790, Obj_SetTargetFlag_00092790) {
    const u32 o = arg<u32>(ctx, 0), x = arg<u32>(ctx, 1);
    const u32 c = static_cast<u32>(call(ctx, 0x004BB4B4, {x, load<u32>(o + 12)}));
    u32 t = 0;
    if (c != 0 && static_cast<u32>(VCall(ctx, c, 32, {c})) == 5) {
        const u32 c2 = load<u32>(c + 28);
        if (c2 != 0 && load<u32>(c2 + 276) == 0x02235C33u) t = load<u32>(c2 + 280);
    }
    if (t == 0) {
        t = static_cast<u32>(call(ctx, 0x000D6A28, {load<u32>(o + 16)}));
        ctx.gpr[3] = 0;
        if (t == 0) return;
    }
    const u32 r = static_cast<u32>(call(ctx, 0x000D94F4, {t}));
    ctx.gpr[3] = r;
    if (r == 0) return;
    store<u32>(r + 1268, (load<u32>(r + 1268) & ~0x10000000u) | ((load<u8>(o + 24) & 1u) << 28));
}

// bool f(T* o; f32 dt): unless StateMachine_Tick(o; dt) fails (r3 = 0),
// each enabled (u8 +16) child of the list o->+248 (u16 count +8) gets
// vt[28](c; dt). r3 = 1.
DECOMP(000D84C4, Obj_TickChildren_000D84C4) {
    const u32 o = arg<u32>(ctx, 0);
    const f64 dt = farg(ctx, 0);
    callf(ctx, 0x00464CBC, {o}, {dt});
    if (static_cast<u32>(ctx.gpr[3]) == 0) {
        ctx.gpr[3] = 0;
        return;
    }
    for (u32 i = 0;; ++i) {
        const u32 l = load<u32>(o + 248);
        const u32 n = l != 0 ? load<u16>(l + 8) : 0u;
        if (i >= n) break;
        const u32 c = load<u32>(load<u32>(l) + 4 * i);
        if (load<u8>(c + 16) == 0) continue;
        ctx.fpr[1] = dt;
        VCall(ctx, c, 28, {c});
    }
    ctx.gpr[3] = 1;
}

// void f(T* o, bool on): bit 0x100 of o->+356->+136 = !on, bit 0x2 of
// +172 = on (bit 0 of r4), o->vt[212](o, on) and bits 0x8000 = on, 0x4000
// = 0 of +172 (r3 = the new +172)
DECOMP(000E7730, Obj_SetEnabled_000E7730) {
    const u32 o = arg<u32>(ctx, 0), r4 = arg<u32>(ctx, 1), on = r4 & 0xFFu;
    const u32 c = load<u32>(o + 356);
    if (c != 0) store<u32>(c + 136, (load<u32>(c + 136) & ~0x100u) | (on == 0 ? 0x100u : 0u));
    store<u32>(o + 172, (load<u32>(o + 172) & ~2u) | ((r4 & 1u) << 1));
    VCall(ctx, o, 212, {o, on != 0 ? 1u : 0u});
    u32 w = load<u32>(o + 172);
    w = on != 0 ? (w | 0x8000u) : (w & ~0x8000u);
    w &= ~0x4000u;
    store<u32>(o + 172, w);
    ctx.gpr[3] = w;
}

// u32 f(u32* out, T* o): Emitter_Stop(o->+348, 1), then *out = o->+348 =
// Sound_Play44C474(&h, o->+356, pos, link, 0...) with pos = o + 144 and
// link = 0, or 0 and o + 352 for bit 31 of +364 (r3 = h)
DECOMP(000F5FFC, Obj_RestartSound_000F5FFC) {
    const u32 out = arg<u32>(ctx, 0), o = arg<u32>(ctx, 1);
    // the original passes +348 through a stack doubleword (stw / ld / rldicl)
    call(ctx, 0x00443424, {load<u32>(o + 348), 1});
    const u32 bit = load<u32>(o + 364) >> 31;
    const u32 snd = load<u32>(o + 356);
    Stack frame(ctx, 16);
    const u32 h = frame.addr();
    call(ctx, 0x0044C474, {h, snd, bit == 0 ? o + 144 : 0u, bit != 0 ? o + 352 : 0u, 0, 0, 0, 0, 0, 0, 0});
    const u32 v = load<u32>(h);
    store<u32>(o + 348, v);
    store<u32>(out, v);
    ctx.gpr[3] = v;
}

// void f(T* o, T* d): when Obj_TestE0BCC(o, d): 0x00125A80(o, 0xA77EB5E9,
// d, 0xA77EB5E9; d->+52 * o->+528), the scale also times the tuning var
// 0x01038988 for a same-team (+172 >> 22 & 3) d->+4 with bit 8 of +192,
// neither o's u64 +184 bit 0x20 nor its bit 0x08000000
DECOMP(001274D4, Obj_HitScaled_001274D4) {
    const u32 o = arg<u32>(ctx, 0), d = arg<u32>(ctx, 1);
    if (static_cast<u32>(call(ctx, 0x000E0BCC, {o, d})) == 0) {
        ctx.gpr[3] = 0;
        return;
    }
    f64 s = Mul28(LfsAt(d + 52), LfsAt(o + 528));
    const u32 t = load<u32>(d + 4);
    if (t != 0 && (load<u32>(t + 192) & 8u) != 0 && ((load<u32>(o + 172) >> 22) & 3u) == ((load<u32>(t + 172) >> 22) & 3u) &&
        (load<u64>(o + 184) & 0x20u) == 0 && (load<u64>(t + 184) & 0x08000000u) == 0)
        s = Mul28(s, LfsAt(0x01038988));
    // r4 keeps the lis sign extension, r6 the zero-extended copy
    callf(ctx, 0x00125A80, {o, 0xFFFFFFFFA77EB5E9ull, d, 0xA77EB5E9u}, {s});
}

// void f(u32 a, const Msg* m): for the messages 0xCCF83851 / 0xB94AE51B
// (+8) whose sender p = +0 has bit 2 of +192 and p->+372 == a: n = (s32)
// the f32 +24 of p's kind-40 component c: 0x002B21D8(c, n, 1) and the
// level counter (*0x00F65CA0)->+4216->+464 += n
DECOMP(0016A358, Msg_AddScore_0016A358) {
    const u32 a = arg<u32>(ctx, 0), m = arg<u32>(ctx, 1);
    const u32 h = load<u32>(m + 8), p = load<u32>(m);
    if (h != 0xCCF83851u && h != 0xB94AE51Bu) return;
    if ((load<u32>(p + 192) & 2u) == 0 || p == 0) return;
    if (load<u32>(p + 372) != a) return;
    const u32 c = static_cast<u32>(call(ctx, o31fn::Obj_FindComponent, {p, 40, 0}));
    ctx.gpr[3] = c;
    if (c == 0) return;
    const u64 nb = rt::fctid(LfsAt(c + 24), true);
    const u32 n = static_cast<u32>(nb);
    ctx.fpr[1] = std::bit_cast<f64>(nb);  // f1 keeps the fctidz bits
    call(ctx, 0x002B21D8, {c, n, 1});
    const u32 q = load<u32>(load<u32>(0x00F65CA0) + 4208 + 8);
    store<u32>(q + 464, load<u32>(q + 464) + n);
    ctx.gpr[3] = q;
}

// void f(T* o, u32 anim): unless the f32 +348 is <= k0 and +692 is already
// 0x03DE9A54: Anim_Restart2775A8(o->vt[60](o), anim, 0, 0, 0, -1, 0, 0, -,
// -, 0, -1; k1, k2, k0, k0) (k = *0x0016CDD8..) and +692 = anim
DECOMP(0016CDE8, Obj_PlayAnim_0016CDE8) {
    const u32 o = arg<u32>(ctx, 0), an = arg<u32>(ctx, 1);
    constexpr u32 K = 0x0016CDD8;
    const f64 k0 = LfsAt(K);
    const u32 vt = load<u32>(o);
    ctx.fpr[1] = LfsAt(o + 348);
    if (ctx.fpr[1] <= k0 && load<u32>(o + 692) == 0x03DE9A54u) {
        ctx.gpr[3] = vt;
        return;
    }
    const u32 r = static_cast<u32>(VCall(ctx, o, 60, {o}));
    // args 9 / 10 are stack slots the original never writes (the callee
    // reads only 11 and 12)
    callf(ctx, 0x002775A8, {r, an, 0, 0, 0, ~0ull, 0, 0, 0, 0, 0, ~0ull}, {LfsAt(K + 4), LfsAt(K + 8), k0, k0});
    store<u32>(o + 692, an);
}

// void f(const T* s, T* o): o->+684 = s->+8; for the effect r =
// o->vt[272]() its component c (Effect_FindComponent(r, 0x04B70827)) of
// kind 2 (vt[32]) gives v = c->vt[24](): s->+4 = c->+20 = v
DECOMP(0016D314, Obj_LinkEffectTime_0016D314) {
    const u32 s = arg<u32>(ctx, 0), o = arg<u32>(ctx, 1);
    ctx.fpr[1] = LfsAt(s + 8);
    CopyW31(o + 684, s + 8);
    const u32 r = static_cast<u32>(VCall(ctx, o, 272, {o}));
    ctx.gpr[3] = r;
    if (r == 0) return;
    const u32 c = static_cast<u32>(call(ctx, 0x004BB4B4, {r, 0x04B70827u}));
    ctx.gpr[3] = c;
    if (c == 0) return;
    ctx.gpr[3] = VCall(ctx, c, 32, {c});
    if (static_cast<u32>(ctx.gpr[3]) != 2) return;
    const u32 v = static_cast<u32>(VCall(ctx, c, 24, {c}));
    store<u32>(s + 4, v);
    store<u32>(c + 20, v);
    ctx.gpr[3] = v;
}

// void f(const T* a, T* b): Thunk_000E44C0(a, b), b->+236 = a->+268, the 7
// words b+208.. = a+240.., the top 15 bits of b->+20 = 4, b->+240 =
// a->+272 and bits 31 / 30 of b->+244 from a->+276 (r3 = the new +244,
// f1 = a->+272)
DECOMP(001C103C, Obj_CopyState_001C103C) {
    const u32 a = arg<u32>(ctx, 0), b = arg<u32>(ctx, 1);
    call(ctx, o30fn::Thunk_E44C0, {a, b});
    CopyW31(b + 236, a + 268);
    for (u32 k = 0; k < 7; ++k) CopyW31(b + 208 + 4 * k, a + 240 + 4 * k);
    store<u32>(b + 20, (load<u32>(b + 20) & 0x1FFFFu) | 0x80000u);
    const u32 w = load<u32>(b + 244);
    ctx.fpr[1] = LfsAt(a + 272);
    CopyW31(b + 240, a + 272);
    u32 v = (w & 0x7FFFFFFFu) | (load<u32>(a + 276) & 0x80000000u);
    store<u32>(b + 244, v);
    v = (v & ~0x40000000u) | (load<u32>(a + 276) & 0x40000000u);
    store<u32>(b + 244, v);
    ctx.gpr[3] = v;
}

// T::~T(T* o): vtables 0x00B1B9E8 / +388 0x00B1BBA0 / +392 0x00B1BBC8; the
// skeleton +396 (its entity +8 removed via Hk_RemoveEntity92FFC8 first)
// released and cleared, vector +528 cleared, base vtables +392 0x00B12528,
// +388 0x00B1200C, 0x00B126B8 and VtB12550_Dtor(o)
DECOMP(00240E8C, Dtor_B1B9E8_00240E8C) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 s = load<u32>(o + 396);
    store<u32>(o, 0x00B1B9E8);
    store<u32>(o + 388, 0x00B1BBA0);
    store<u32>(o + 392, 0x00B1BBC8);
    if (s != 0) {
        const u32 e = load<u32>(s + 8);
        if (e != 0) {
            Stack frame(ctx, 32);
            call(ctx, 0x0092FFC8, {frame.addr(), e, s});
        }
        call(ctx, 0x007D4EA8, {load<u32>(o + 396)});
        store<u32>(o + 396, 0);
    }
    call(ctx, o31fn::PtrVector_Clear, {o + 528});
    call(ctx, o31fn::Nop_54EC18, {o + 528});
    store<u32>(o + 392, 0x00B12528);
    store<u32>(o + 388, 0x00B1200C);
    store<u32>(o, 0x00B126B8);
    ctx.gpr[3] = call(ctx, 0x000DF934, {o});
}

// void f(Vec3* out, T* o): by the 3-bit mode (+416 >> 28): 4 = the Vec3 at
// 0x0105C3C0, 6 = the +80 of o->+8's kind-44 component (when found), else
// o->+448 (f1 = the last value moved)
DECOMP(0026D8F4, Obj_ModePos_0026D8F4) {
    const u32 out = arg<u32>(ctx, 0), o = arg<u32>(ctx, 1);
    const u32 k = (load<u32>(o + 416) >> 28) & 7u;
    u32 src = o + 448;
    ctx.gpr[3] = k;
    if (k == 4) {
        src = 0x0105C3C0;
        ctx.gpr[3] = 0x0105C370;
    } else if (k == 6) {
        const u32 t = load<u32>(o + 8);
        ctx.gpr[3] = t;
        if (t != 0) {
            const u32 c = static_cast<u32>(call(ctx, o31fn::Obj_FindComponent, {t, 44, 0}));
            ctx.gpr[3] = c;
            if (c != 0) {
                src = c + 80;
                ctx.gpr[3] = src;
            }
        }
    }
    for (u32 j = 0; j < 3; ++j) CopyW31(out + 4 * j, src + 4 * j);
    ctx.fpr[1] = LfsAt(src + 8);
}

// void f(T* o, u32 key): +80 = the spline SplineMap_Find(*0x00F8F158->+16,
// &key) maps to, +136 = 0; when found, +144 = 0 and +96 = the offset of
// o->+8's position +144 from WorldObj_SplinePointAt(s, .., &p, 0;
// *0x00282400) (f1 = its x)
DECOMP(00282408, Obj_AttachSpline_00282408) {
    const u32 o = arg<u32>(ctx, 0), key = arg<u32>(ctx, 1);
    const f64 k = LfsAt(0x00282400);
    Stack frame(ctx, 32);
    const u32 pt = frame.addr(), kp = pt + 16;
    store<u32>(kp, key);
    const u32 r = static_cast<u32>(call(ctx, 0x007335C4, {load<u32>(load<u32>(0x00F8F158) + 16), kp}));
    const u32 s = load<u32>(r);
    store<u32>(o + 80, s);
    ctx.fpr[1] = k;
    store<u32>(o + 136, 0);
    ctx.gpr[3] = s;
    if (s == 0) return;
    const u32 p = load<u32>(o + 8) + 144;
    const f64 px = LfsAt(p), py = LfsAt(p + 4), pz = LfsAt(p + 8);
    callf(ctx, 0x00592748, {s, p, pt, 0}, {k});
    const f32 dx = Sub28(px, LfsAt(pt)), dy = Sub28(py, LfsAt(pt + 4)), dz = Sub28(pz, LfsAt(pt + 8));
    store<u32>(o + 144, 0);
    store<f32>(o + 96, dx);
    store<f32>(o + 100, dy);
    store<f32>(o + 104, dz);
    ctx.fpr[1] = dx;
    ctx.gpr[3] = 0;
}

// void f(T* o): the first time (bit 0x00800000 of +124 then set),
// Obj_SetStateFx266E6C(o, (s32)+68); then state 1 when +68 == 0 with +88
// set, state 0 when +68 == 1 with +88 clear; o->+112->vt[40](c, 0, 1) when
// set with bit 0x80000 of +124
DECOMP(002B70BC, Obj_SyncState_002B70BC) {
    const u32 o = arg<u32>(ctx, 0);
    if ((load<u32>(o + 124) & 0x00800000u) == 0) {
        call(ctx, 0x00266E6C, {o, static_cast<u64>(static_cast<s64>(static_cast<s32>(load<u32>(o + 68))))});
        store<u32>(o + 124, load<u32>(o + 124) | 0x00800000u);
    }
    const u32 s = load<u32>(o + 68);
    if (s == 0) {
        if (load<u32>(o + 88) != 0) call(ctx, 0x00266E6C, {o, 1});
    } else if (s == 1) {
        if (load<u32>(o + 88) == 0) call(ctx, 0x00266E6C, {o, 0});
    }
    const u32 c = load<u32>(o + 112);
    ctx.gpr[3] = c;
    if (c == 0) return;
    if ((load<u32>(o + 124) & 0x00080000u) == 0) return;
    ctx.gpr[3] = VCall(ctx, c, 40, {c, 0, 1});
}

// void f(T* o): f32 +348 = -1; when active (u8 +376) the body of
// o->+8->vt[64]()->+32 gets 0x0091A228(b, o + 24) and +376 = 0; then u8
// +345 = 0 and o->+380->vt[40](c, 0) with +380 = 0 when set. r3 = 0.
DECOMP(002CCD94, Phys_Reset_002CCD94) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 b = load<u8>(o + 376);
    store<u32>(o + 348, 0xBF800000u);
    if (b != 0) {
        const u32 c = load<u32>(o + 8);
        const u32 h = load<u32>(static_cast<u32>(VCall(ctx, c, 64, {c})) + 32);
        if (h != 0) {
            const u32 bd = static_cast<u32>(call(ctx, 0x009D19C8, {h}));
            if (bd != 0) {
                call(ctx, 0x0091A228, {bd, o + 24});
                store<u8>(o + 376, 0);
            }
        }
    }
    const u32 e = load<u32>(o + 380);
    store<u8>(o + 345, 0);
    ctx.gpr[3] = 0;
    if (e == 0) return;
    VCall(ctx, e, 40, {e, 0});
    store<u32>(o + 380, 0);
    ctx.gpr[3] = 0;
}

// void f(Section* o, r4): 0x00325CE4(o, r4 as passed); in mode 4 (u8 +18)
// o->vt[28](o, 0) and each record's id o->+2592[48k] (k < +3360) resolved
// by Obj_FindById into +2596 (u16 +178 counted up)
DECOMP(0032E838, Recs_Resolve_0032E838) {
    const u32 o = arg<u32>(ctx, 0);
    call(ctx, 0x00325CE4, {o, arg<u32>(ctx, 1)});  // the callee reads the r4 our caller left
    ctx.gpr[3] = load<u8>(o + 18);
    if (load<u8>(o + 18) != 4) return;
    VCall(ctx, o, 28, {o, 0});
    s32 n = static_cast<s32>(load<u32>(o + 3360));
    ctx.gpr[3] = static_cast<u32>(n);
    for (s32 i = 0; i < n;) {
        const u32 r = o + 48 * static_cast<u32>(i) + 2592;
        const u32 t = static_cast<u32>(call(ctx, 0x000D6A28, {load<u32>(r)}));
        if (t != 0) {
            store<u32>(r + 4, t);
            store<u16>(t + 178, static_cast<u16>(load<u16>(t + 178) + 1));
        }
        ++i;
        n = static_cast<s32>(load<u32>(o + 3360));
        ctx.gpr[3] = static_cast<u32>(n);
    }
}

// void f(T* o, const Msg* m): for message 0x36FB367F (+8) from s = +0,
// each 32-byte record e (o->+2792, u16 count +2800) whose +0 is s's
// +216->+52 (0 without one) gets 0x0033E2C0(o, s, (s32)m->+12, e + 16).
// r3 = m->+0.
DECOMP(0033E6EC, Obj_OnMsgRecs_0033E6EC) {
    const u32 o = arg<u32>(ctx, 0), m = arg<u32>(ctx, 1);
    u32 s = load<u32>(m);
    ctx.gpr[3] = s;
    if (s == 0 || load<u32>(m + 8) != 0x36FB367Fu) return;
    for (u32 i = 0; i < load<u16>(o + 2800); ++i) {
        const u32 e = load<u32>(o + 2792) + 32 * i;
        const u32 q = load<u32>(s + 216);
        const u32 v = q != 0 ? load<u32>(q + 52) : 0u;
        if (load<u32>(e) != v) continue;
        call(ctx, 0x0033E2C0, {o, s, static_cast<u64>(static_cast<s64>(static_cast<s32>(load<u32>(m + 12)))), e + 16});
        s = load<u32>(m);
    }
    ctx.gpr[3] = s;
}

// void f(T* o, u8 on): on: Obj_StartFromIdle and Obj_AddToList36341C;
// off (unless state +160 >> 1 is 0 or 3): o->vt[20](o, 1), state = 3,
// Speaker_SayId4A5C90(o->+28, o->+188, 0; *0x0036D8FC) and
// Effect_SetHash4CF988(o->+20, 0x048129EB) when set
DECOMP(0036D900, Talk_Toggle_0036D900) {
    const u32 o = arg<u32>(ctx, 0), on = arg<u32>(ctx, 1) & 0xFFu;
    if (on != 0) {
        call(ctx, 0x00364D28, {o});
        ctx.gpr[3] = call(ctx, 0x0036341C, {o});
        return;
    }
    const u32 s = load<u32>(o + 160) >> 1;
    ctx.gpr[3] = s;
    if (s == 3 || s == 0) return;
    VCall(ctx, o, 20, {o, 1});
    const u32 w = (load<u32>(o + 160) & 1u) | 6u;
    const u32 sp = load<u32>(o + 28);
    store<u32>(o + 160, w);
    callf(ctx, 0x004A5C90, {sp, load<u32>(o + 188), 0}, {LfsAt(0x0036D8FC)});
    const u32 t = load<u32>(o + 20);
    if (t != 0) ctx.gpr[3] = call(ctx, 0x004CF988, {t, 0x048129EBu});
}

// void f(T* o): the pad mask +448 from the player pad 0x010B1A90 (held a =
// +8, both c = a & +12): bits 0x80 / 0x40 / 0x10 / 0x20 of c give 2 / 4 /
// 1 / 8, else of a 0x20 / 0x40 / 0x10 / 0x80 (stored as each is added)
DECOMP(003D7310, Pad_Mask_003D7310) {
    const u32 o = arg<u32>(ctx, 0);
    constexpr u32 P = 0x010B19E0 + 176;
    store<u32>(o + 448, 0);
    const u32 a = load<u32>(P + 8), c = a & load<u32>(P + 12);
    static constexpr u8 tab[4][3] = {{0x80, 2, 0x20}, {0x40, 4, 0x40}, {0x10, 1, 0x10}, {0x20, 8, 0x80}};
    u32 v = 0;
    for (auto& t : tab) {
        if ((c & t[0]) != 0) v |= t[1];
        else if ((a & t[0]) != 0) v |= t[2];
        else continue;
        store<u32>(o + 448, v);
    }
}

// void f(): resets the table 0x010AA1B0: three 16-byte slots from +8 and
// the one at +56 get {0, 1, 2, +12 & 0x0FFFFFFF} (+72 = 1 after the last),
// and the 16 40-byte records from +484 {0, 0, 0, 0, 2.0, -1.0, 0,
// 1.5707964, u8 0, 0}. r3 = 0xBF800000 (sign-extended).
DECOMP(0042A37C, Table_Reset_0042A37C) {
    constexpr u32 B = 0x010AA1B0;
    auto slot = [](u32 p) {
        const u32 w12 = load<u32>(p + 12);
        store<u32>(p, 0);
        store<u32>(p + 8, 2);
        store<u32>(p + 4, 1);
        store<u32>(p + 12, w12 & 0x0FFFFFFFu);
    };
    for (u32 k = 0; k < 3; ++k) slot(B + 8 + 16 * k);
    const u32 w12 = load<u32>(B + 68);  // the fourth slot, stored in field order
    store<u32>(B + 56, 0);
    store<u32>(B + 60, 1);
    store<u32>(B + 64, 2);
    store<u32>(B + 68, w12 & 0x0FFFFFFFu);
    store<u32>(B + 72, 1);
    for (u32 k = 0; k < 16; ++k) {
        const u32 p = B + 484 + 40 * k;
        for (u32 off : {0u, 4u, 8u, 12u}) store<u32>(p + off, 0);
        store<u32>(p + 16, 0x40000000);
        store<u32>(p + 20, 0xBF800000u);
        store<u32>(p + 24, 0);
        store<u32>(p + 28, 0x3FC90FDB);
        store<u8>(p + 32, 0);
        store<u32>(p + 36, 0);
    }
    ctx.gpr[3] = 0xFFFFFFFFBF800000ull;
}

// void f(T* o): t = the object WorldObj_ForEach(o->+12, 0x00F390A8, {h =
// o->+16, 0}, 0) reports for a set h (else o->+12) gets vt[40](t, bit 31,
// bit 30 of o->+20); then Flag_Set8(o)
DECOMP(00439144, Cmd_SetFlags_00439144) {
    const u32 o = arg<u32>(ctx, 0);
    u32 t = load<u32>(o + 12);
    const u32 h = load<u32>(o + 16);
    if (h != 0) {
        Stack frame(ctx, 16);
        const u32 q = frame.addr();
        store<u32>(q, h);
        store<u32>(q + 4, 0);
        call(ctx, 0x0057F5D8, {t, 0x00F390A8u, q, 0});
        const u32 r = load<u32>(q + 4);
        if (r != 0) t = r;
    }
    if (t != 0) {
        const u32 w = load<u32>(o + 20);
        const u32 opd = load<u32>(load<u32>(t) + 40);
        call_ptr(ctx, opd, {t, w >> 31, (w >> 30) & 1u, load<u32>(opd), opd});  // entry in r6, OPD in r7
    }
    ctx.gpr[3] = call(ctx, o31fn::Flag_Set8, {o});
}

// bool f(T* o, ?, u32 key): the tree search of 0x0074BA80 over t =
// o->+16 (root t->+12->+0, path stack t->+16[] with count u16 +96, node
// +100, value 0x011DE2AC); true when found with a non-null value
DECOMP(004BCBB0, Tree_Has_004BCBB0) {
    const u32 o = arg<u32>(ctx, 0), key = arg<u32>(ctx, 2);
    ctx.gpr[3] = 0;
    const u32 t = load<u32>(o + 16);
    if (t == 0) return;
    u32 node = load<u32>(load<u32>(t + 12));
    if (node == 0) {
        store<u32>(t + 100, 0);
        store<u32>(0x011DE2AC, 0);
        return;
    }
    store<u16>(t + 96, 0);
    u32 cur;
    for (;;) {
        cur = node;
        const u32 k = load<u32>(cur + 4);
        if (k == key) {
            store<u32>(t + 100, cur);
            store<u32>(0x011DE2AC, load<u32>(cur + 8));
            const u32 n = load<u32>(t + 100);
            if (n != 0 && load<u32>(n + 8) != 0) ctx.gpr[3] = 1;
            return;
        }
        node = k > key ? load<u32>(cur + 12) : load<u32>(cur + 16);
        if (node == 0) break;
        const s32 n = static_cast<s16>(load<u16>(t + 96));
        store<u16>(t + 96, static_cast<u16>(n + 1));
        store<u32>(t + 16 + 4 * static_cast<u32>(n), cur);
    }
    if (cur == 0) store<u16>(t + 96, 0);  // dead: cur is a visited node
    store<u32>(t + 100, cur);
}

// T::~T(): see DtorPopAll31 (vtable 0x00B22308, vector +40, count +48,
// base inlined)
DECOMP(004C6210, Dtor_B22308_004C6210) { DtorPopAll31(ctx, 0x00B22308, 48, 40, true); }

// T::~T(): see DtorPopAll31 (vtable 0x00B22330, vector +28, count +36,
// base inlined)
DECOMP(004C6B2C, Dtor_B22330_004C6B2C) { DtorPopAll31(ctx, 0x00B22330, 36, 28, true); }

// void f(T* o, x): for the kind-10 effect e = Effect_Bind4BB7E0(x,
// o->+12, 0) with a target r = e->+208->+112->+132: r = {+8 = 1, +12 =
// o->+20, +16..+24 = (0, 1, 0), +28 = plane distance k1 * y' + k0 * x' -
// k0 * z' of e's position +64 moved by o->+16 (k = *0x004CCC24..)}
DECOMP(004CCC30, Effect_SetPlane_004CCC30) {
    const u32 o = arg<u32>(ctx, 0), x = arg<u32>(ctx, 1);
    const u32 e = static_cast<u32>(call(ctx, 0x004BB7E0, {x, load<u32>(o + 12), 0}));
    ctx.gpr[3] = e;
    if (e == 0 || ((load<u32>(e + 4) >> 10) & 0x3Fu) != 10) return;
    u32 r = load<u32>(e + 208);
    if (r == 0 || (r = load<u32>(r + 112)) == 0 || (r = load<u32>(r + 132)) == 0) return;
    const f64 k1 = LfsAt(0x004CCC28), k0 = LfsAt(0x004CCC24);
    const u32 v = e + 64;
    const f64 d = LfsAt(o + 16);
    const f64 y = Madd28(k1, d, LfsAt(v + 4));
    const f64 xx = Madd28(k0, d, LfsAt(v));
    const f64 z = Madd28(k0, d, LfsAt(v + 8));
    store<u32>(r + 16, 0);
    store<u32>(r + 20, 0x3F800000);
    store<u32>(r + 24, 0);
    f64 f = Mul28(k1, y);
    store<u32>(r + 8, 1);
    f = Madd28(k0, xx, f);
    f = static_cast<f32>(-std::fma(k0, z, f));
    store<f32>(r + 28, static_cast<f32>(f));
    ctx.fpr[1] = LfsAt(o + 20);
    CopyW31(r + 12, o + 20);
    ctx.gpr[3] = 0;
}

// void f(T* o, u32 v): the bytes +132..+135 = v (big-endian, one at a
// time) and each set o->+204[k] (k < 9) gets vt[56](c, v) (the original
// passes v through a stack doubleword and calls with r5 = the entry only)
DECOMP(004E82D8, Obj_BroadcastId_004E82D8) {
    const u32 o = arg<u32>(ctx, 0), v = arg<u32>(ctx, 1);
    for (u32 k = 0; k < 4; ++k) store<u8>(o + 132 + k, static_cast<u8>(v >> (24 - 8 * k)));
    for (u32 i = 0; i < 9; ++i) {
        const u32 c = load<u32>(o + 204 + 4 * i);
        ctx.gpr[3] = c;
        if (c == 0) continue;
        const u32 opd = load<u32>(load<u32>(c) + 56);
        call_ptr(ctx, opd, {c, v, load<u32>(opd)});
    }
}

// ---------------------------------------------------------------- sweep: 57 instructions
// void f(T* o): with a held o->+256: c->vt[40](c, 0) unless o->vt[124]()
// is set; +256 = 0, o->+48->vt[96](m, 0) and, for m's +164, u16 +40 bits
// 0x1000 cleared / 0x400 set
DECOMP(00012460, Obj_ReleaseHeld_00012460) {
    const u32 o = arg<u32>(ctx, 0);
    ctx.gpr[3] = 0;
    if (load<u32>(o + 256) == 0) return;
    if (static_cast<u32>(VCall(ctx, o, 124, {o})) == 0) {
        const u32 c = load<u32>(o + 256);
        VCall(ctx, c, 40, {c, 0});
    }
    const u32 m = load<u32>(o + 48);
    store<u32>(o + 256, 0);
    VCall(ctx, m, 96, {m, 0});
    const u32 q = load<u32>(load<u32>(o + 48) + 164);
    if (q == 0) return;
    store<u16>(q + 40, static_cast<u16>((load<u16>(q + 40) & ~0x1000u) | 0x400u));
    ctx.gpr[3] = 0;
}

// void f(T* o): the attachments +656 / +660 (with the sources +664 / +668)
// take the source's position +128 at +64, then WorldObj_MarkDirty and
// WorldObj_UpdateLod(a, 0, 0) (f1 = the last value moved)
DECOMP(0004399C, Obj_SyncAttach_0004399C) {
    const u32 o = arg<u32>(ctx, 0);
    for (u32 k = 0; k < 2; ++k) {
        const u32 a = load<u32>(o + 656 + 4 * k);
        if (k == 0) ctx.gpr[3] = a;  // the second check leaves r3 alone
        if (a == 0) continue;
        const u32 s = load<u32>(o + 664 + 4 * k);
        ctx.gpr[3] = k == 0 ? a : s;
        if (s == 0) continue;
        for (u32 j = 0; j < 3; ++j) CopyW31(a + 64 + 4 * j, s + 128 + 4 * j);
        ctx.fpr[1] = LfsAt(s + 136);
        call(ctx, 0x0057D000, {a});
        ctx.gpr[3] = call(ctx, 0x0057EE00, {load<u32>(o + 656 + 4 * k), 0, 0});
    }
}

// void f(T* o, x): the kind-2 component c = Effect_FindComponent(x,
// o->+12) gets +20 = whether the level player (*0x00F65CA0)->+4216 passes
// Obj_Notify26 and Camera_IsSplit
DECOMP(0007AC24, Cam_SetSplitFlag_0007AC24) {
    const u32 o = arg<u32>(ctx, 0), x = arg<u32>(ctx, 1);
    ctx.gpr[3] = 0;
    if (load<u32>(o + 12) == 0) return;
    u32 sp = 0, r = 0;
    const u32 q = load<u32>(load<u32>(0x00F65CA0) + 4208 + 8);
    if (q != 0) r = static_cast<u32>(call(ctx, 0x000D94CC, {q}));
    if (r != 0) sp = static_cast<u32>(call(ctx, 0x002A6044, {r}));
    const u32 c = static_cast<u32>(call(ctx, 0x004BB4B4, {x, load<u32>(o + 12)}));
    ctx.gpr[3] = c;
    if (c == 0) return;
    ctx.gpr[3] = VCall(ctx, c, 32, {c});
    if (static_cast<u32>(ctx.gpr[3]) != 2) return;
    const u32 f = sp != 0 ? 1u : 0u;
    store<u32>(c + 20, f);
    ctx.gpr[3] = f;
}

// void f(T* o, x): the kind-12 component c = Effect_FindComponent(x,
// o->+12) gets +20 = the hash for the current session part's slot s
// (SlotRec_Bits416, 0..4: 0x00AC4DB0[s]), else 0x19D1382A
DECOMP(0007C40C, Ui_SetSlotHash_0007C40C) {
    const u32 o = arg<u32>(ctx, 0), x = arg<u32>(ctx, 1);
    ctx.gpr[3] = 0;
    if (load<u32>(o + 12) == 0) return;
    u32 h = 0x19D1382A;
    const u32 p = static_cast<u32>(call(ctx, 0x002DDD24, {load<u32>(0x00F64C20)}));
    if (p != 0) {
        const u32 s = static_cast<u32>(call(ctx, 0x002DA990, {p}));
        if (static_cast<s32>(s) >= 0 && s < 5) h = load<u32>(0x00AC4DB0 + (s << 2));
    }
    const u32 c = static_cast<u32>(call(ctx, 0x004BB4B4, {x, load<u32>(o + 12)}));
    ctx.gpr[3] = c;
    if (c == 0) return;
    ctx.gpr[3] = VCall(ctx, c, 32, {c});
    if (static_cast<u32>(ctx.gpr[3]) != 12) return;
    store<u32>(c + 20, h);
}

// void f(T* o, x): the target t (as TargetCall31, no Hash_ToString) gets
// Obj_SetTarget_0012E534(t, o->+20) when it has bit 8 of +192
DECOMP(00096D18, Obj_TargetSet_00096D18) {
    const u32 o = arg<u32>(ctx, 0), x = arg<u32>(ctx, 1);
    const u32 c = static_cast<u32>(call(ctx, 0x004BB4B4, {x, load<u32>(o + 12)}));
    u32 t = 0;
    if (c != 0 && static_cast<u32>(VCall(ctx, c, 32, {c})) == 5) {
        const u32 c2 = load<u32>(c + 28);
        if (c2 != 0 && load<u32>(c2 + 276) == 0x02235C33u) t = load<u32>(c2 + 280);
    }
    ctx.gpr[3] = 0;
    if (t == 0) {
        t = static_cast<u32>(call(ctx, 0x000D6A28, {load<u32>(o + 16)}));
        ctx.gpr[3] = 0;
        if (t == 0) return;
    }
    ctx.gpr[3] = t;
    if ((load<u32>(t + 192) & 8u) == 0) return;
    ctx.gpr[3] = call(ctx, 0x0012E534, {t, load<u32>(o + 20)});
}

// void f(T* o): for the live handle o->+20 (Ptr_DerefNonZero, Ptr_Test88)
// CmdList_Push(p) with p = SmallAlloc(32) built by 0x0042ECB8(p, +20,
// +24, (s32)+28, u8 +32, +33, +34) (or null); else Hash_ToString of the
// names +12 (and +16 when the handle is dead)
DECOMP(0009D1C0, Cmd_PushHandle_0009D1C0) {
    const u32 o = arg<u32>(ctx, 0);
    if (static_cast<u32>(call(ctx, 0x00446960, {load<u32>(o + 20)})) == 0) {
        call(ctx, 0x0056A6E0, {load<u32>(o + 12)});
        ctx.gpr[3] = call(ctx, 0x0056A6E0, {load<u32>(o + 16)});
        return;
    }
    if (static_cast<u32>(call(ctx, 0x00443168, {load<u32>(o + 20)})) == 0) {
        ctx.gpr[3] = call(ctx, 0x0056A6E0, {load<u32>(o + 12)});
        return;
    }
    const u32 p = static_cast<u32>(call(ctx, o31fn::SmallAlloc, {32}));
    u32 q = 0;
    if (p != 0) {
        call(ctx, 0x0042ECB8, {p, load<u32>(o + 20), load<u32>(o + 24), static_cast<u64>(static_cast<s64>(static_cast<s32>(load<u32>(o + 28)))),
                               load<u8>(o + 32), load<u8>(o + 33), load<u8>(o + 34)});
        q = p;
    }
    ctx.gpr[3] = call(ctx, o30fn::CmdList_Push, {q});
}

// void f(T* o): for the point named o->+16 (PointMap_Find(*0x00F8F154->+16,
// &name); Hash_ToString when missing) and a set +12: CmdList_Push(p) with
// p = SmallAlloc(32) built by VtB20F38_Ctor(p, +12, pt, +20, u8 +24, +25)
// (or null)
DECOMP(0009E928, Cmd_PushPoint_0009E928) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 h = load<u32>(o + 16);
    if (h == 0) return;
    ctx.gpr[3] = 0;
    if (load<u32>(o + 12) == 0) return;
    Stack frame(ctx, 16);
    store<u32>(frame.addr(), h);
    const u32 pt = load<u32>(static_cast<u32>(call(ctx, 0x0073343C, {load<u32>(load<u32>(0x00F8F154) + 16), frame.addr()})));
    if (pt == 0) {
        ctx.gpr[3] = call(ctx, 0x0056A6E0, {load<u32>(o + 16)});
        return;
    }
    const u32 p = static_cast<u32>(call(ctx, o31fn::SmallAlloc, {32}));
    u32 q = 0;
    if (p != 0) {
        call(ctx, 0x0042FACC, {p, load<u32>(o + 12), pt, load<u32>(o + 20), load<u8>(o + 24), load<u8>(o + 25)});
        q = p;
    }
    ctx.gpr[3] = call(ctx, o30fn::CmdList_Push, {q});
}

// void f(T* o): for the spline named o->+16 (SplineMap_Find(*0x00F8F158->
// +16, &name)) and a set +12: CmdList_Push(p) with p = SmallAlloc(40)
// built by VtB20F80_Init(p, +12, s, +20, u8 +34, -, u8 +32, u8 +33, u8 +34;
// f32 +24, +28) (or null)
DECOMP(0009F070, Cmd_PushSpline_0009F070) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 h = load<u32>(o + 16);
    if (h == 0) return;
    ctx.gpr[3] = 0;
    if (load<u32>(o + 12) == 0) return;
    Stack frame(ctx, 16);
    store<u32>(frame.addr(), h);
    const u32 r = static_cast<u32>(call(ctx, 0x007335C4, {load<u32>(load<u32>(0x00F8F158) + 16), frame.addr()}));
    const u32 sp = load<u32>(r);
    ctx.gpr[3] = r;
    if (sp == 0) return;
    const u32 p = static_cast<u32>(call(ctx, o31fn::SmallAlloc, {40}));
    u32 q = 0;
    if (p != 0) {
        const u32 b34 = load<u8>(o + 34);
        // r8 is not set by the original (as SmallAlloc left it; unread)
        callf(ctx, 0x004306F4, {p, load<u32>(o + 12), sp, load<u32>(o + 20), b34, ctx.gpr[8], load<u8>(o + 32), load<u8>(o + 33), b34},
              {LfsAt(o + 24), LfsAt(o + 28)});
        q = p;
    }
    ctx.gpr[3] = call(ctx, o30fn::CmdList_Push, {q});
}

// void f(T* o, ...): see VPair196_31 (vt[188], the caller's 9th argument)
DECOMP(000DC010, Obj_VPair188_000DC010) { VPair196_31(ctx, 188, 116); }

// void f(T* o, bool on): bit 0x80 of +172 = on; when o->+348's entity +8
// presence matches on: on: Hk_RemoveEntity92FFC8(&tmp, e->+8, e); off:
// Hk_AddEntity92F2A8(world->+4, e, 1) and o->vt[84](o, o + 96)
DECOMP(000DEE68, Obj_SetPhysEntity_000DEE68) {
    const u32 o = arg<u32>(ctx, 0), r4 = arg<u32>(ctx, 1), on = r4 & 0xFFu;
    const u32 e = load<u32>(o + 348);
    store<u32>(o + 172, (load<u32>(o + 172) & ~0x80u) | ((r4 & 1u) << 7));
    ctx.gpr[3] = e;
    if (e == 0) return;
    const u32 ent = load<u32>(e + 8);
    if (((ent != 0 ? 1u : 0u) ^ (on == 0 ? 1u : 0u)) == 0) return;
    if (on != 0) {
        Stack frame(ctx, 32);
        ctx.gpr[3] = call(ctx, 0x0092FFC8, {frame.addr(), ent, e});
        return;
    }
    call(ctx, 0x0092F2A8, {load<u32>(load<u32>(load<u32>(0x00F65CA0) + 0x1281D4u) + 4), e, 1});
    ctx.gpr[3] = VCall(ctx, o, 84, {o, o + 96});
}

// u32 f(T* o, T* b, u32* out): for b in mode 1 (s8 +24), the entry of
// o->+388 (48 bytes, u16 count +8) whose +28 is b + (s8)b->+16: its +16
// (with *out = -1), else *out = its +20 and r3 = o->+160; otherwise
// 0x000DA85C(o, b, out)
DECOMP(000E764C, Obj_LookupBone_000E764C) {
    const u32 o = arg<u32>(ctx, 0), b = arg<u32>(ctx, 1), out = arg<u32>(ctx, 2);
    if (static_cast<s8>(load<u8>(b + 24)) == 1) {
        const u32 key = b + static_cast<u32>(static_cast<s32>(static_cast<s8>(load<u8>(b + 16))));
        if (key != 0) {
            const u32 l = load<u32>(o + 388);
            for (u32 i = 0; i < (l != 0 ? load<u16>(l + 8) : 0u); ++i) {
                const u32 e = load<u32>(l) + 48 * i;
                if (load<u32>(e + 28) != key) continue;
                const u32 v = load<u32>(e + 16);
                if (v != 0) {
                    store<u32>(out, 0xFFFFFFFFu);
                    ctx.gpr[3] = v;
                    return;
                }
                const u32 w = load<u32>(e + 20);
                const u32 r = load<u32>(o + 160);
                store<u32>(out, w);
                ctx.gpr[3] = r;
                return;
            }
        }
    }
    ctx.gpr[3] = call(ctx, 0x000DA85C, {o, b, out});
}

// bool f(T* o, Hit* d): when Obj_TestE0BCC(o, d) and the filter OPD
// o->+416 (when set) passes: with the factor f = o->+216->+276 above
// *0x000E8674, bit 0x100 of u16 d->+14 = !o->vt[352](o, opd, d->+4, d->+8;
// d->+52 * f) (r3 = countl_zero of it)
DECOMP(000E8678, Obj_ApplyHit_000E8678) {
    const u32 o = arg<u32>(ctx, 0), d = arg<u32>(ctx, 1);
    const u32 m = load<u32>(o + 216);
    ctx.gpr[3] = 0;
    if (static_cast<u32>(call(ctx, 0x000E0BCC, {o, d})) == 0) {
        ctx.gpr[3] = 0;
        return;
    }
    const u32 fp = load<u32>(o + 416);
    if (fp != 0 && static_cast<u32>(call_ptr(ctx, fp, {o, d, fp, load<u32>(fp)})) == 0) {
        ctx.gpr[3] = 0;
        return;
    }
    const f64 f = LfsAt(m + 276);
    ctx.fpr[1] = f;
    ctx.gpr[3] = 0x000E8674;
    if (!(f > LfsAt(0x000E8674))) return;
    ctx.fpr[1] = Mul28(LfsAt(d + 52), f);
    const u32 r = static_cast<u32>(VCallOpdR4_28(ctx, o, 352, load<u32>(d + 4), load<u32>(d + 8)));
    const u32 z = static_cast<u32>(std::countl_zero(r));
    store<u16>(d + 14, static_cast<u16>((load<u16>(d + 14) & ~0x100u) | ((z << 3) & 0x100u)));
    ctx.gpr[3] = z;
}

// T::~T() deleting (size 912): vtable 0x00B139B8, the parts +608
// (vt[4]) and +612 (vt[8]) released and cleared, the member vtables +880
// 0x00B139A8, +784 0x00B13998, +688 0x00B13988, Thunk_000D729C(o),
// SmallFree(o, 912)
DECOMP(000FA2E4, Dtor_B139B8_000FA2E4) {
    const u32 o = arg<u32>(ctx, 0);
    if (o == 0) return;
    const u32 c = load<u32>(o + 608);
    store<u32>(o, 0x00B139B8);
    if (c != 0) {
        VCall(ctx, c, 4, {c});
        store<u32>(o + 608, 0);
    }
    const u32 c2 = load<u32>(o + 612);
    if (c2 != 0) {
        VCall(ctx, c2, 8, {c2});
        store<u32>(o + 612, 0);
    }
    store<u32>(o + 880, 0x00B139A8);
    store<u32>(o + 784, 0x00B13998);
    store<u32>(o + 688, 0x00B13988);
    call(ctx, 0x000D729C, {o});
    ctx.gpr[3] = call(ctx, o19fn::SmallFree, {o, 912});
}

// void f(T* o, ...): see VPair196_31 (vt[192], the caller's 10th argument)
DECOMP(000FCAE4, Obj_VPair192_000FCAE4) { VPair196_31(ctx, 192, 124); }
