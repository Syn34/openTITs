// Game code, part 33 (continued): the small-function sweep, 64
// instructions and up. Each was checked against the original in the PPC
// simulator (memory writes, calls and their arguments, r3 and f1).
#include "obj33.h"

using namespace sly4;
using namespace sly4::obj33;

// ---------------------------------------------------------------- sweep: 64 instructions
// T f(T* o): with the name s of the current entry (index
// *0x010AAF7C->+16->+64) of o->+224's table (->+0->+12): +388 =
// String_HashNoCase(s) and o->vt[220](o, 13); then, with o->+224 set: the
// name of entry *0x010AAF94->+16->+64 hashed and (when nonzero) given to
// 0x00014004(o, h), and Msg_Post(o->+48, G->+92, G->+92, o->+224->+4->+204)
// for G = 0x00F65BF8. r3 as the last call left it (0 for no +224).
DECOMP(0003BAB8, Obj_ApplyNamedEntry_0003BAB8) {
    const u32 o = arg<u32>(ctx, 0);
    u32 a = load<u32>(o + 224);
    const u32 s = load<u32>(load<u32>(load<u32>(a) + 12) + (load<u32>(load<u32>(0x010AAF7Cu + 16) + 64) << 2));
    if (s != 0) {
        store<u32>(o + 388, static_cast<u32>(call(ctx, 0x0056A710, {s})));
        VCall(ctx, o, 220, {o, 13});
        a = load<u32>(o + 224);
    }
    ctx.gpr[3] = a;
    if (a == 0) return;
    const u32 s2 = load<u32>(load<u32>(load<u32>(a) + 12) + (load<u32>(load<u32>(0x010AAF94u + 16) + 64) << 2));
    const u32 h = static_cast<u32>(call(ctx, 0x0056A710, {s2}));
    if (h != 0) call(ctx, 0x00014004, {o, h});
    const u32 b = load<u32>(o + 224);
    const u32 g = load<u32>(0x00F65BF8u + 92);
    ctx.gpr[3] = call(ctx, 0x00470660, {load<u32>(o + 48), g, g, load<u32>(load<u32>(b + 4) + 204)});
}

// T f(T* o, u32 k1, u32 k2): the splines of k1 / k2 in the point map
// *0x00F8F154->+16 (PointMap_Find) into o->+6240 / +6244; with o's path
// +6236, the nearest parameters (SplineInstance_Nearest(path, spline+64,
// 0, 0; f1 = K[i]), K = 0x0005B9FC, or K[i] itself without a spline) into
// +6248 / +6252. r3 as the original leaves it.
DECOMP(0005BA08, Obj_FindSplines_0005BA08) {
    const u32 o = arg<u32>(ctx, 0), k1 = arg<u32>(ctx, 1), k2 = arg<u32>(ctx, 2);
    constexpr u32 K = 0x0005B9FC;
    Stack frame(ctx, 16);
    const u32 key2 = frame.addr(), key1 = key2 + 4;  // sp+112 / sp+116
    store<u32>(key1, k1);
    const u32 p1 = static_cast<u32>(call(ctx, 0x0073343C, {load<u32>(load<u32>(0x00F8F154) + 16), key1}));
    const u32 s1 = load<u32>(p1);
    store<u32>(key2, k2);
    store<u32>(o + 6240, s1);
    const u32 p2 = static_cast<u32>(call(ctx, 0x0073343C, {load<u32>(load<u32>(0x00F8F154) + 16), key2}));
    u32 path = load<u32>(o + 6236);
    u32 s2 = load<u32>(p2);
    store<u32>(o + 6244, s2);
    ctx.gpr[3] = s2;
    if (path == 0) return;
    const u32 a = load<u32>(o + 6240);
    f64 f = Lfs32(K);
    if (a != 0) {
        f = callf(ctx, 0x005927FC, {path, a + 64, 0, 0}, {f});
        s2 = load<u32>(o + 6244);
        path = load<u32>(o + 6236);
    }
    Stfs32(o + 6248, f);
    f = Lfs32(K + 4);
    ctx.fpr[1] = f;
    ctx.gpr[3] = s2;
    if (s2 != 0) f = callf(ctx, 0x005927FC, {path, s2 + 64, 0, 0}, {f});
    Stfs32(o + 6252, f);
}

// T f(Cmd* c, Owner* x): the target t (the +280 of the object (+28) of
// the kind-5 effect component Effect_FindComponent(x, c->+12) when its
// +276 is 0x02235C33, else Obj_FindById(c->+16)) gets
// 0x002E2B34(*0x00F64C20, t->+168). r3 as the original leaves it.
DECOMP(0007010C, Cmd_ShowTarget_0007010C) {
    const u32 c = arg<u32>(ctx, 0), x = arg<u32>(ctx, 1);
    u32 t = 0;
    const u32 id12 = load<u32>(c + 12);
    ctx.gpr[3] = id12;
    if (id12 != 0) {
        const u32 k = static_cast<u32>(call(ctx, 0x004BB4B4, {x, id12}));
        if (k != 0 && static_cast<u32>(VCall(ctx, k, 32, {k})) == 5) {
            const u32 k2 = load<u32>(k + 28);
            if (k2 != 0) {
                ctx.gpr[3] = 0x02235C33u;
                if (load<u32>(k2 + 276) == 0x02235C33u) t = load<u32>(k2 + 280);
            }
        }
    }
    if (t == 0) {
        const u32 id = load<u32>(c + 16);
        if (id != 0) t = static_cast<u32>(call(ctx, 0x000D6A28, {id}));
        if (t == 0) return;
    }
    ctx.gpr[3] = call(ctx, 0x002E2B34, {load<u32>(0x00F64C20), load<u32>(t + 168)});
}

// T f(Cmd* c, Owner* x): the target t (the +280 of the effect
// Effect_Bind4BB7E0(x, c->+16, 0) when its +276 is 0x02235C33, else
// Obj_FindById(c->+12)), when flag 0x1000 of its +196 is set: with u8
// c->+21, bit 0x2000 of the u16 t->+394 = u8 c->+20 & 1; with u8 c->+24,
// Obj_SwapSpline_001BD540(t); with u8 c->+23,
// Field_VCall404_10_001BD450(t, u8 c->+22). r3 as the original leaves it.
DECOMP(000902F8, Cmd_SetSplineFlags_000902F8) {
    const u32 c = arg<u32>(ctx, 0), x = arg<u32>(ctx, 1);
    u32 t = 0;
    const u32 id16 = load<u32>(c + 16);
    if (id16 != 0) {
        const u32 e = static_cast<u32>(call(ctx, 0x004BB7E0, {x, id16, 0}));
        if (e != 0 && load<u32>(e + 276) == 0x02235C33u) t = load<u32>(e + 280);
    }
    if (t == 0) {
        t = static_cast<u32>(call(ctx, 0x000D6A28, {load<u32>(c + 12)}));
        if (t == 0) return;
    }
    ctx.gpr[3] = 0;
    if ((load<u32>(t + 196) & 0x1000u) == 0) return;
    if (load<u8>(c + 21) != 0)
        store<u16>(t + 394, static_cast<u16>((load<u16>(t + 394) & ~0x2000u) | ((static_cast<u32>(load<u8>(c + 20)) << 13) & 0x2000u)));
    if (load<u8>(c + 24) != 0) call(ctx, 0x001BD540, {t});
    ctx.gpr[3] = 0;
    if (load<u8>(c + 23) != 0) call(ctx, 0x001BD450, {t, load<u8>(c + 22)});
}

// T f(T* o, u8 keep, u8 b, u8 c): without keep the buffer +372 is freed
// (Mem_FreeFrom) and cleared; u8 +364 = b and o->vt[116](o, b, 0, 0, c);
// with keep, the world object w = o->+160 moves to group 6 (its group
// ((+320 >> 18) & 63) saved in +368; r3 = 6 when already there), else it
// returns to the saved group (WorldObj_SetGroup(w, +368)). r3 as the last
// call left it.
DECOMP(000DF3CC, Obj_SetHeld_000DF3CC) {
    const u32 o = arg<u32>(ctx, 0), keep = arg<u32>(ctx, 1) & 0xFFu, b = arg<u32>(ctx, 2) & 0xFFu, c = arg<u32>(ctx, 3) & 0xFFu;
    if (keep == 0) FreeField32(ctx, o, 372);
    store<u8>(o + 364, static_cast<u8>(b));
    VCall(ctx, o, 116, {o, b, 0, 0, c});
    const u32 w = load<u32>(o + 160);
    if (keep != 0) {
        const u32 g = (load<u32>(w + 320) >> 18) & 63u;
        ctx.gpr[3] = g;
        if (g == 6) return;
        store<u32>(o + 368, g);
        ctx.gpr[3] = call(ctx, 0x00583E54, {w, 6});
        return;
    }
    ctx.gpr[3] = call(ctx, 0x00583E54, {w, Sx33(load<u32>(o + 368))});
}

// T f(T* o; f1 dt): with flag 0x00020000 of +376: the effect o->+356
// (when set) gets, for flagged (0x00080000) objects with a world object
// q (+160) whose entry in the map 0x01013B1C (key q->+204) has flag 2 of
// its +4, Effect_AddArgs49FCC0(e, entry+180, 0, 0, 0), then the hash
// 0x04B6FB5B; the flag is cleared. Then Obj_UpdateBase(o; dt). r3 as that
// left it.
DECOMP(000E0520, Obj_UpdateFx_000E0520) {
    const u32 o = arg<u32>(ctx, 0);
    const f64 dt = ctx.fpr[1];
    const u32 w = load<u32>(o + 376);
    if ((w & 0x00020000u) != 0) {
        if (load<u32>(o + 356) == 0) {
            store<u32>(o + 376, w & ~0x00020000u);
        } else {
            const u32 q = (w & 0x00080000u) != 0 ? load<u32>(o + 160) : 0u;
            if (q != 0) {
                Stack frame(ctx, 16);
                store<u32>(frame.addr(), load<u32>(q + 204));  // sp+112
                const u32 r = load<u32>(static_cast<u32>(call(ctx, 0x00732F38, {0x01013B1Cu, frame.addr()})));
                if (r != 0 && (load<u32>(r + 4) & 2u) != 0) call(ctx, 0x0049FCC0, {load<u32>(o + 356), r + 180, 0, 0, 0});
            }
            call(ctx, 0x004CF988, {load<u32>(o + 356), 0x04B6FB5Bu});
            store<u32>(o + 376, load<u32>(o + 376) & ~0x00020000u);
        }
    }
    callf(ctx, 0x000D83C4, {o}, {dt});  // r3 as it returns
}

// T f(T* o): unless o->vt[32](o, 1) is 2, o->vt[28](o, 1, 2) and
// o->vt[28](o, 2, 0); otherwise, when o->vt[324](o) is 0, the effect
// o->+356 (when set) gets the hash 0x803E1667. r3 as the last step left
// it.
DECOMP(001CEC8C, Obj_ResetPose_001CEC8C) {
    const u32 o = arg<u32>(ctx, 0);
    if (static_cast<u32>(VCall(ctx, o, 32, {o, 1})) != 2) {
        VCall(ctx, o, 28, {o, 1, 2});
        VCall(ctx, o, 28, {o, 2, 0});
        return;
    }
    if (static_cast<u32>(VCall(ctx, o, 324, {o})) != 0) return;
    const u32 e = load<u32>(o + 356);
    ctx.gpr[3] = e;
    if (e != 0) call(ctx, 0x004CF988, {e, 0x803E1667u});
}

// bool f(T* o, Def* d): unless u8 o->+504 is set (r3 = 0): the world
// enters state 7 (World_RequestState(world, 1, 7)) and its record r
// (world->+20 + u16 world->+6) gets +68 = (s8)d->+172, +72..+84 = o's
// +492 / +496 / +488 / +500, u8 +88 = o's +505, the name o+421 copied
// into r (64 chars; empty when none) and +92 = Target_GetDef0D951C(o).
// r3 = 1.
DECOMP(001EB8CC, Obj_StartCutscene_001EB8CC) {
    const u32 o = arg<u32>(ctx, 0), d = arg<u32>(ctx, 1);
    if (load<u8>(o + 504) != 0) {
        ctx.gpr[3] = 0;
        return;
    }
    call(ctx, 0x004665C0, {load<u32>(0x00F65CA0), 1, 7});
    const u32 g = load<u32>(0x00F65CA0);
    const u32 k = static_cast<u32>(static_cast<s32>(static_cast<s8>(load<u8>(d + 172))));
    const u32 r = load<u32>(g + 20) + load<u16>(g + 6);
    const u32 s = o + 421;
    store<u32>(r + 68, k);
    store<u32>(r + 72, load<u32>(o + 492));
    store<u32>(r + 76, load<u32>(o + 496));
    store<u32>(r + 80, load<u32>(o + 488));
    store<u32>(r + 84, load<u32>(o + 500));
    store<u8>(r + 88, load<u8>(o + 505));
    if (s != 0 && load<u8>(s) != 0) call(ctx, 0x00553638, {r, s, 64});
    else store<u8>(r, 0);
    store<u32>(r + 92, static_cast<u32>(call(ctx, 0x000D951C, {o})));
    ctx.gpr[3] = 1;
}

// T f(T* o): the attached object a = o->+816 (when set) released: its
// world object q (+160) handed to q->+260's vt[24](it, q) when both
// exist, then a->vt[88](a, 0, 1, 1, 1), a->vt[40](a, 0), a->vt[40](a, 1)
// and o->+816 = 0. r3 = 0.
DECOMP(001EDA8C, Obj_DropAttached_001EDA8C) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 a = load<u32>(o + 816);
    ctx.gpr[3] = 0;
    if (a == 0) return;
    const u32 q = load<u32>(a + 160);
    if (q != 0) {
        const u32 b = load<u32>(q + 260);
        if (b != 0) VCall(ctx, b, 24, {b, q});
    }
    u32 p = load<u32>(o + 816);
    VCall(ctx, p, 88, {p, 0, 1, 1, 1});
    p = load<u32>(o + 816);
    VCall(ctx, p, 40, {p, 0});
    p = load<u32>(o + 816);
    VCall(ctx, p, 40, {p, 1});
    store<u32>(o + 816, 0);
    ctx.gpr[3] = 0;
}

// u32 f(T* o, u8 on, u8 reset): when turning on from off (flag 0x00400000
// of +980 clear) or with reset: +512 (and +520 with reset) = the default
// 0x01164AFC (raw), +548 = 0, +564 / +568 = 1 / 1 (reset) or 0 / 1, and
// Motion_Set4A0C20(o+544; f1 = *0x001EF870 (reset) or *0x01053798). Then
// the flag = on. r3 = the new +980.
DECOMP(001EF878, Obj_SetMotionOn_001EF878) {
    const u32 o = arg<u32>(ctx, 0), a = arg<u32>(ctx, 1), on = a & 0xFFu, reset = arg<u32>(ctx, 2) & 0xFFu;
    const bool off = (load<u32>(o + 980) & 0x00400000u) == 0;
    if ((off && on != 0) || reset != 0) {
        constexpr u32 G = 0x01164AFC;
        if (reset != 0) {
            CopyW32(o + 520, G);
            CopyW32(o + 524, G + 4);
        }
        CopyW32(o + 512, G);
        CopyW32(o + 516, G + 4);
        const f64 k = Lfs32(0x001EF870);
        ctx.fpr[1] = k;
        if (reset != 0) {
            store<u32>(o + 564, 0x3F800000);
            store<u32>(o + 568, 0x3F800000);
            store<u32>(o + 548, 0);
            callf(ctx, 0x004A0C20, {o + 544}, {k});
        } else {
            store<u32>(o + 564, 0);
            store<u32>(o + 568, 0x3F800000);
            store<u32>(o + 548, 0);
            callf(ctx, 0x004A0C20, {o + 544}, {Lfs32(0x01053798)});
        }
    }
    const u32 w = (load<u32>(o + 980) & ~0x00400000u) | ((a << 22) & 0x00400000u);
    store<u32>(o + 980, w);
    ctx.gpr[3] = w;
}

// void f(): the static tuning variables 0x01055870 / 0x01055A00 /
// 0x01055938 / 0x010556A0 (vt 0x00B0DA38), 0x01055810 / 0x01055640 /
// 0x010555E0 (vt 0x00B0D9D0) and 0x010557B8 / 0x0105576C (vt 0x00B0D99C)
// destroyed (vt set, TuningVar_DestructBase_004FC1A0). r3 as the last
// left it.
DECOMP(0020BC08, Tuning_StaticDtor_0020BC08) {
    constexpr u32 v1 = 0x00B0DA38, v2 = 0x00B0D9D0, v3 = 0x00B0D99C;
    const u32 seq[][2] = {{0x01055870, v1}, {0x01055810, v2}, {0x01055A00, v1}, {0x01055938, v1}, {0x010556A0, v1},
                          {0x010557B8, v3}, {0x0105576C, v3}, {0x01055640, v2}, {0x010555E0, v2}};
    for (const auto& e : seq) {
        store<u32>(e[0], e[1]);
        ctx.gpr[3] = call(ctx, 0x004FC1A0, {e[0]});
    }
}

// T f(T* o; f1 dt): TeamScan33 on the team +444.
DECOMP(0021C774, Obj_TeamScan444_0021C774) { TeamScan33(ctx, 444); }

// T f(T* o; f1 dt): TeamScan33 on the team +428.
DECOMP(0021DD08, Obj_TeamScan428_0021DD08) { TeamScan33(ctx, 428); }

// f32 f(Wander* w; f1 dt): with u8 +627 set, the value +644 moves toward
// the target +648 at the rate +652 (snapping once the step undershoots K[1];
// K = 0x0028AAD4) unless within K[0]; the timer +640 += dt, and past the
// period +636 it restarts at 0 with a new random target in +628..+632
// (the inlined random step on 0x01164888, scaled by K[2]) and rate (target
// - value) / period. f1 = the rate, else the period.
DECOMP(0028AAE0, Wander_Update_0028AAE0) {
    const u32 w = arg<u32>(ctx, 0);
    const f64 dt = ctx.fpr[1];
    constexpr u32 K = 0x0028AAD4;
    if (load<u8>(w + 627) == 0) return;
    f64 cur = Lfs32(w + 644);
    const f64 tg = Lfs32(w + 648);
    if (!(std::fabs(static_cast<f32>(cur - tg)) < Lfs32(K))) {
        const f64 rate = Lfs32(w + 652);
        cur = static_cast<f32>(std::fma(rate, dt, cur));
        const f64 left = static_cast<f32>(tg - cur);
        Stfs32(w + 644, cur);
        if (static_cast<f32>(rate * left) < Lfs32(K + 4)) {
            cur = tg;
            Stfs32(w + 644, tg);
        }
    }
    const f64 t = static_cast<f32>(Lfs32(w + 640) + dt);
    const f64 period = Lfs32(w + 636);
    Stfs32(w + 640, t);
    ctx.fpr[1] = period;
    if (!(t > period)) return;
    const u64 old = load<u64>(0x01164888);
    store<u32>(w + 640, 0);
    const f64 lo = Lfs32(w + 628);
    const f64 span = static_cast<f32>(Lfs32(w + 632) - lo);
    const f64 r = static_cast<f32>(static_cast<f64>(static_cast<f32>(static_cast<f64>(static_cast<s64>(old)))) * Lfs32(K + 8));
    const f64 ntg = static_cast<f32>(std::fma(span, r, lo));
    const f64 d = static_cast<f32>(ntg - cur);
    Stfs32(w + 648, ntg);
    RandStep28();
    const f64 nr = static_cast<f32>(d / period);
    Stfs32(w + 652, nr);
    ctx.fpr[1] = nr;
}

// T f(T* o; f1 dt): unless u8 +57 is set (r3 = it) or u8 +58 clear (r3 =
// 0), each group of o's u16-counted list (+36 / +44) is ticked
// (0x002FCD4C(g; dt)); when no group has u8 +52 set with u8 +84 clear,
// u8 +57 = 1 and u8 +58 = 0 (r3 = 1); otherwise r3 = the count.
DECOMP(002FD0AC, Obj_TickGroups_002FD0AC) {
    const u32 o = arg<u32>(ctx, 0);
    const f64 dt = ctx.fpr[1];
    ctx.gpr[3] = load<u8>(o + 57);
    if (ctx.gpr[3] != 0) return;
    ctx.gpr[3] = load<u8>(o + 58);
    if (ctx.gpr[3] == 0) return;
    u32 n = 0;
    for (u32 i = 0;; ++i) {
        n = load<u16>(o + 44);
        if (!(i < n)) break;
        callf(ctx, 0x002FCD4C, {load<u32>(load<u32>(o + 36) + 4 * i)}, {dt});
    }
    ctx.gpr[3] = n;
    for (u32 j = 0; j < n; ++j) {
        const u32 g = load<u32>(load<u32>(o + 36) + 4 * j);
        if (load<u8>(g + 52) != 0 && load<u8>(g + 84) == 0) return;
    }
    store<u8>(o + 57, 1);
    store<u8>(o + 58, 0);
    ctx.gpr[3] = 1;
}

// bool f(Reader* r): the 64-byte name table 0x0106CFA1 (zeroed with its
// u8 0x0106CFA0 once, flag u8 0x0106CF98): a byte (Iff_ReadU8, ignored)
// then 64 bytes read in; *0x00F65790 = the table; with the world's
// *0x00F65CA0->+0x1281C0 = x, 0x004ECF08(x, *0x00F65794, table). r3 = 1.
DECOMP(00391498, World_ReadNameTable_00391498) {
    const u32 r = arg<u32>(ctx, 0);
    if (load<u8>(0x0106CF98) == 0) {
        call(ctx, 0x00726E54, {0x0106CFA0u, 0, 65});
        store<u8>(0x0106CF98, 1);
    }
    Stack frame(ctx, 16);
    call(ctx, 0x005643C4, {r, frame.addr(), 0});  // sp+112
    constexpr u32 table = 0x0106CFA1u;
    for (u32 i = 0; i < 64; ++i) call(ctx, 0x0056998C, {r, table + i, 0});
    const u32 g = load<u32>(0x00F65CA0);
    store<u32>(0x00F65790, table);
    const u32 x = load<u32>(g + 0x1281C0);
    if (x != 0) call(ctx, 0x004ECF08, {x, load<u32>(0x00F65794), table});
    ctx.gpr[3] = 1;
}

// T f(Rec* a, Src* b): for b's object q (+12) with q->vt[60](q) = r set:
// r->+380 = the f32 at RefEntry32(a) (raw). r3 = 0 (f1 = that value).
DECOMP(003953FC, Rec_ReadF32_003953FC) {
    const u32 a = arg<u32>(ctx, 0), b = arg<u32>(ctx, 1);
    const u32 q = load<u32>(b + 12);
    if (q != 0) {
        const u32 r = static_cast<u32>(VCall(ctx, q, 60, {q}));
        if (r != 0) {
            const u32 v = load<u32>(RefEntry32(a));
            store<u32>(r + 380, v);
            ctx.fpr[1] = static_cast<f64>(std::bit_cast<f32>(v));
        }
    }
    ctx.gpr[3] = 0;
}

// T f(Cmd* c): the child c->+16 of the object Obj_FindByHandle42C2B0(c->+12,
// 0) (List_FindAfter_000D92B0(t, id, 0)) gets vt[44](e, u8 c->+20) (entry
// in r5, OPD in r6); a missing object / child is reported (String_Format
// of 0x00AE2280 / 0x00AE2220 with the names). Then Flag_Set8_40000000(c).
// r3 as that left it.
DECOMP(0042DF40, Cmd_SetChildFlag_0042DF40) {
    const u32 c = arg<u32>(ctx, 0);
    const u32 t = static_cast<u32>(call(ctx, 0x0042C2B0, {load<u32>(c + 12), 0}));
    if (t == 0) {
        const u32 n = static_cast<u32>(call(ctx, 0x0056A6E0, {load<u32>(c + 12)}));
        call(ctx, 0x005534D0, {0x00AE2280u, n});
    } else {
        const u32 e = static_cast<u32>(call(ctx, 0x000D92B0, {t, load<u32>(c + 16), 0}));
        if (e == 0) {
            const u32 n1 = static_cast<u32>(call(ctx, 0x0056A6E0, {load<u32>(c + 12)}));
            const u32 n2 = static_cast<u32>(call(ctx, 0x0056A6E0, {load<u32>(c + 16)}));
            call(ctx, 0x005534D0, {0x00AE2220u, n1, n2});
        } else {
            const u32 opd = load<u32>(load<u32>(e) + 44);
            call_ptr(ctx, opd, {e, load<u8>(c + 20), load<u32>(opd), opd});
        }
    }
    ctx.gpr[3] = call(ctx, 0x0042CA60, {c});
}

// T* T(T* o, u32 a; f1..f7, stack: u32 w (10th), u8 b (11th), u8 init
// (12th), u8 d (13th)): Obj_InitVt_B20CC8(o, init), vt 0x00B20ED8, +12 =
// a, +16..+40 = f1..f7, +44 = w, u8 +48 = b, u8 +49 = d. r3 = the vtable.
DECOMP(0042F3BC, Cmd_Ctor_B20ED8_0042F3BC) {
    const u32 o = arg<u32>(ctx, 0), a = arg<u32>(ctx, 1);
    f64 f[7];
    for (int k = 0; k < 7; ++k) f[k] = ctx.fpr[1 + k];
    const u32 sp = static_cast<u32>(ctx.gpr[1]);
    const u32 init = load<u32>(sp + 140) & 0xFFu, b = load<u32>(sp + 132), d = load<u32>(sp + 148), w = load<u32>(sp + 124);
    call(ctx, 0x0042C9D0, {o, init});
    store<u32>(o + 12, a);
    Stfs32(o + 16, f[0]);
    Stfs32(o + 20, f[1]);
    Stfs32(o + 24, f[2]);
    store<u32>(o, 0x00B20ED8);
    for (int k = 3; k < 7; ++k) Stfs32(o + 16 + 4 * static_cast<u32>(k), f[k]);
    store<u32>(o + 44, w);
    store<u8>(o + 48, static_cast<u8>(b));
    store<u8>(o + 49, static_cast<u8>(d));
    ctx.gpr[3] = 0x00B20ED8;
}

// T f(Cmd* c): for the object t = Obj_FindByHandle42C2B0(c->+12, 0):
// 0x0042C7DC(Table_Find_0042C980(t), t), then with u8 c->+16 set and flag
// 2 of t->+192, t->vt[352](t, -, 0, 0; f1 = t->+344) (OPD in r4, entry in
// r7), else t->vt[40](t, 1). Then Flag_Set8_40000000(c). r3 as that left
// it.
DECOMP(0042FE28, Cmd_Activate_0042FE28) {
    const u32 c = arg<u32>(ctx, 0);
    const u32 t = static_cast<u32>(call(ctx, 0x0042C2B0, {load<u32>(c + 12), 0}));
    if (t != 0) {
        const u32 k = static_cast<u32>(call(ctx, 0x0042C980, {t}));
        call(ctx, 0x0042C7DC, {k, t});
        const u32 v = load<u8>(c + 16);
        if (v != 0 && (load<u32>(t + 192) & 2u) != 0) {
            const u32 opd = load<u32>(load<u32>(t) + 352);
            ctx.fpr[1] = Lfs32(t + 344);
            call_ptr(ctx, opd, {t, opd, 0, 0, load<u32>(opd)});
        } else {
            VCall(ctx, t, 40, {t, 1});
        }
    }
    ctx.gpr[3] = call(ctx, 0x0042CA60, {c});
}

// T f(T* o): the nine parameters 0x010ABDD0 + 24 * k registered
// (Obj_RegisterParam(o, p)), then o->+8's u8 +40 |= 0x80 with +44 =
// 0x00AE3798, and o->+88 = 0x00F33CC8. r3 = o->+8.
DECOMP(0043DEB8, Obj_RegisterParams9_0043DEB8) {
    const u32 o = arg<u32>(ctx, 0);
    for (u32 k = 0; k < 9; ++k) call(ctx, 0x004F7128, {o, 0x010ABDD0u + 24 * k});
    const u32 p = load<u32>(o + 8);
    const u32 t = load<u8>(p + 40);
    store<u32>(p + 44, 0x00AE3798);
    store<u8>(p + 40, static_cast<u8>(t | 0x80u));
    store<u32>(o + 88, 0x00F33CC8);
    ctx.gpr[3] = p;
}

// T f(T* o, u8 tick): without o->+36, the name of o->+40 (vt[12]) is
// reported (String_Format(0x00AE5130, name)); u8 +28 = 1 and each object
// c of the chain o->+32 (next +72) gets vt[24](c) and, with tick,
// Obj_Tick4536C0(c; f1 = *0x00454218). r3 as the last call left it (1
// for an empty chain).
DECOMP(00454220, Obj_StartChain_00454220) {
    const u32 o = arg<u32>(ctx, 0), tick = arg<u32>(ctx, 1) & 0xFFu;
    if (load<u32>(o + 36) == 0) {
        const u32 s = load<u32>(o + 40);
        const u32 n = static_cast<u32>(VCall(ctx, s, 12, {s}));
        call(ctx, 0x005534D0, {0x00AE5130u, n});
    }
    const u32 c0 = load<u32>(o + 32);
    store<u8>(o + 28, 1);
    ctx.gpr[3] = 1;
    if (c0 == 0) return;
    const f64 k = Lfs32(0x00454218);
    for (u32 c = c0; c != 0; c = load<u32>(c + 72)) {
        VCall(ctx, c, 24, {c});
        if (tick != 0) callf(ctx, 0x004536C0, {c}, {k});
    }
}

// bool f(Reader* r): NameTable33 on 0x010BE759 (-> *0x00F65D30, key
// 0x7A30D7E3).
DECOMP(0048C194, World_ReadNameTableA_0048C194) { NameTable33(ctx, 0x010BE750, 0x010BE758, 0x00F65D30, 0x7A30D7E3); }

// bool f(Reader* r): NameTable33 on 0x010BE7A9 (-> *0x00F65D34, key
// 0x1AF675EA).
DECOMP(0048C3C0, World_ReadNameTableB_0048C3C0) { NameTable33(ctx, 0x010BE7A0, 0x010BE7A8, 0x00F65D34, 0x1AF675EA); }

// T* f(?, Xml* x): a 24-byte node (SmallAlloc; vt 0x00B21D28, u16 +4 = 1,
// +20 = 0): hash 0x0024728B -> +8; then +20 = the hash 0x00014211 when
// that read returns 0, else the s32 0x00014B01 when that read returns 0
// (else +20 stays 0). r3 = p.
DECOMP(004BEC64, XmlNew_B21D28_004BEC64) {
    const u32 x = arg<u32>(ctx, 1);
    const u32 p = static_cast<u32>(call(ctx, 0x0055238C, {24}));
    ctx.gpr[3] = 0;
    if (p == 0) return;
    store<u16>(p + 4, 1);
    store<u32>(p, 0x00B21D28);
    store<u32>(p + 20, 0);
    call(ctx, 0x004FA14C, {x, 0x0024728Bu, p + 8, 0});
    Stack frame(ctx, 16);
    const u32 s = frame.addr(), h = s + 4;  // sp+112 / sp+116
    store<u32>(s, 0);
    store<u32>(h, 0);
    if (static_cast<u32>(call(ctx, 0x004FA14C, {x, 0x00014211u, h, 0})) == 0) {
        store<u32>(p + 20, load<u32>(h));
    } else if (static_cast<u32>(call(ctx, 0x004F99BC, {x, 0x00014B01u, s, Sx33(load<u32>(s))})) == 0) {
        store<u32>(p + 20, load<u32>(s));
    }
    ctx.gpr[3] = p;
}

// T* f(?, Xml* x): a 20-byte node (vt 0x00B224C0, u16 +4 = 1): hash
// 0x8A93973F -> +12, and when that read returns nonzero the s32
// 0x5C8998CD -> +12; bit 0x80 of the u8 +16 = the bool 0x71B29C0F
// (default 0). r3 = p.
DECOMP(004CC0BC, XmlNew_B224C0_004CC0BC) {
    const u32 x = arg<u32>(ctx, 1);
    const u32 p = static_cast<u32>(call(ctx, 0x0054ED38, {20, 0, 16}));
    ctx.gpr[3] = 0;
    if (p == 0) return;
    store<u16>(p + 4, 1);
    store<u32>(p, 0x00B224C0);
    if (static_cast<u32>(call(ctx, 0x004FA14C, {x, 0x8A93973Fu, p + 12, 0})) != 0)
        call(ctx, 0x004F99BC, {x, 0x5C8998CDu, p + 12, 0});
    Stack frame(ctx, 16);
    const u32 b = frame.addr();  // sp+112
    store<u8>(b, 0);
    call(ctx, 0x004F986C, {x, 0x71B29C0Fu, b, 0});
    store<u8>(p + 16, static_cast<u8>((load<u8>(p + 16) & ~0x80u) | ((static_cast<u32>(load<u8>(b)) << 7) & 0x80u)));
    ctx.gpr[3] = p;
}

// Deleting ~T() (o may be 0; vt 0x00B11E20): the pointer vectors +92 /
// +72 / +52 / +32 / +12 cleared, vt 0x00B11DC4, SmallFree(o, 152). r3 as
// that left it.
DECOMP(00743AEC, Delete_B11E20_00743AEC) {
    const u32 o = arg<u32>(ctx, 0);
    ctx.gpr[3] = 0;
    if (o == 0) return;
    store<u32>(o, 0x00B11E20);
    for (const u32 off : {92u, 72u, 52u, 32u, 12u}) PvClear32(ctx, o + off);
    store<u32>(o, 0x00B11DC4);
    ctx.gpr[3] = call(ctx, o32fn::SmallFree, {o, 152});
}

// Deleting ~T() (o may be 0; vt 0x00B16088): the pointer vector +236
// cleared, vt 0x00B12868 and the vectors +200 / +180 cleared, vt
// 0x00B12260 and the 20-byte vector object +16 (when set) cleared and
// SmallFree'd, then SmallFree(o, 272). r3 as that left it.
DECOMP(00753158, Delete_B16088_00753158) {
    const u32 o = arg<u32>(ctx, 0);
    ctx.gpr[3] = 0;
    if (o == 0) return;
    store<u32>(o, 0x00B16088);
    PvClear32(ctx, o + 236);
    store<u32>(o, 0x00B12868);
    PvClear32(ctx, o + 200);
    PvClear32(ctx, o + 180);
    const u32 q = load<u32>(o + 16);
    store<u32>(o, 0x00B12260);
    if (q != 0) {
        PvClear32(ctx, q);
        call(ctx, o32fn::SmallFree, {load<u32>(o + 16), 20});
    }
    ctx.gpr[3] = call(ctx, o32fn::SmallFree, {o, 272});
}

// ~T() (vt 0x00B1D424): each of the o->+36 8-byte slots of o->+16 with an
// object v gets v->vt[312](v, o->+4->+8, o->+8) (entry in r6, OPD in r7);
// every slot becomes {0, 0x7F7FFFFF}; then +36 = 0 and the vector +16
// cleared. r3 as that left it.
DECOMP(00765F00, Dtor_B1D424_00765F00) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 n0 = load<u32>(o + 36);
    store<u32>(o, 0x00B1D424);
    if (n0 != 0) {
        for (u32 i = 0;; ++i) {
            const u32 e = load<u32>(o + 16) + 8 * i;
            const u32 v = load<u32>(e);
            if (v != 0) {
                const u32 opd = load<u32>(load<u32>(v) + 312);
                const u32 a = load<u32>(load<u32>(o + 4) + 8), b = load<u32>(o + 8);
                call_ptr(ctx, opd, {v, a, b, load<u32>(opd), opd});
            }
            store<u32>(e, 0);
            const u32 n = load<u32>(o + 36);
            store<u32>(e + 4, 0x7F7FFFFFu);
            if (!(i + 1 < n)) break;
        }
    }
    store<u32>(o + 36, 0);
    PvClear32(ctx, o + 16);
}

// s16 f(u16 a, u8** pp, u32* link): as PushRecord33, but the record's +0
// = 6 is written before the count (re-read) is incremented, and the link
// target gets the u16 0xFFFF and the u32 0 at +4. r3 = the re-read count
// (sign-extended).
DECOMP(00773898, Block_PushRecordLink_00773898) {
    const u32 a = arg<u32>(ctx, 0), pp = arg<u32>(ctx, 1), link = arg<u32>(ctx, 2);
    const u32 base = load<u32>(pp) - 152;
    const s32 idx = static_cast<s16>(load<u16>(base + 144));
    const u32 rec = base + 152 + static_cast<u32>(idx) * 132;
    store<u32>(rec, 6);
    const u32 c = load<u16>(base + 144);
    store<u16>(base + 144, static_cast<u16>(c + 1));
    // The record is located again from the re-read count and its type
    // checked (< 10) as the original does.
    const u32 rec2 = base + 152 + static_cast<u32>(static_cast<s32>(static_cast<s16>(c))) * 132;
    const u32 r8 = (static_cast<s32>(load<u32>(rec2)) < 10 ? rec2 : 0u) + 4;
    if (r8 != 0) {
        store<u16>(r8 + 2, static_cast<u16>(load<u16>(r8 + 2) & 0x3Fu));
        store<u16>(r8, static_cast<u16>(a));
    }
    if (link != 0) {
        const u32 p = r8 + load<u16>(0x0106CF7C);
        store<u32>(link, p);
        if (p != 0) {
            store<u16>(p, 0xFFFF);
            store<u32>(p + 4, 0);
        }
    }
    ret(ctx, static_cast<s32>(static_cast<s16>(c)));
}

// Node* f(s16 id): a 24-byte condition node "CEveryXSecond" (Mem_Alloc;
// Cond_Init(p, id, name, 1)), its record's u16 +0 = 16 (at address 0 for
// index -1, quirk kept), and the binding "Frequency" (Node_Bind(p->+12,
// name, 3, 0, -1, 0); +76 = 0) whose +64 becomes the flag byte (p->+20 +
// u16 *0x0106CF7A). r3 = p (0 for a failed allocation); r4 = the
// binding's +64 as left.
DECOMP(00775AC4, CondNew_EveryXSecond_00775AC4) {
    const u64 a0 = ctx.gpr[3];
    const u32 p = static_cast<u32>(call(ctx, 0x0054ED38, {24, 0, 16}));
    if (p == 0) {
        ctx.gpr[3] = 0;
        return;
    }
    call(ctx, 0x0038E5DC, {p, static_cast<u64>(static_cast<s64>(static_cast<s16>(a0))), 0x00B1FAF7u, 1});
    const s32 idx = static_cast<s16>(load<u16>(p + 16));
    const u32 rec = idx == -1 ? 0 : load<u32>(load<u32>(0x00F65038) + 24) + (static_cast<u32>(idx) << 7);
    store<u16>(rec, 16);
    const u32 flags = load<u32>(p + 20) + load<u16>(0x0106CF7A);
    const u32 r = static_cast<u32>(call(ctx, 0x004D3088, {load<u32>(p + 12), 0x00B1FB05u, 3, 0, ~0ull, 0}));
    const u32 v = load<u32>(r + 64);
    store<u32>(r + 76, 0);
    store<u8>(flags, static_cast<u8>(v));
    ctx.gpr[3] = p;
    ctx.gpr[4] = v;
}

// ---------------------------------------------------------------- sweep: 65 instructions
// T f(Cmd* c, Ctx* x): source a = Target_Resolve_0009A190(c, c->+12, x)
// and target b = Target_Resolve(c, c->+16, x) (narrowed by
// Obj_FindByHash_000DB290(b, c->+20) when set); for a target, with c->+28
// = 0x7A30D7E3 c->+40 is resolved too, then Queue_Push_004705F8(a, b,
// c->+24, c->+40, 1); without one and u8 c->+36 clear the name is
// formatted (Hash_ToString(c->+16)). r3 as the last step left it.
DECOMP(0006E8E0, Cmd_QueueAction_0006E8E0) {
    const u32 c = arg<u32>(ctx, 0), x = arg<u32>(ctx, 1);
    const u32 a = static_cast<u32>(call(ctx, 0x0009A190, {c, load<u32>(c + 12), x}));
    u32 b = static_cast<u32>(call(ctx, 0x0009A190, {c, load<u32>(c + 16), x}));
    if (b != 0) {
        const u32 h = load<u32>(c + 20);
        if (h != 0) b = static_cast<u32>(call(ctx, 0x000DB290, {b, h}));
    }
    if (b == 0) {
        const u32 q = load<u8>(c + 36);
        ctx.gpr[3] = q;
        if (q == 0) call(ctx, 0x0056A6E0, {load<u32>(c + 16)});
        return;
    }
    if (load<u32>(c + 28) == 0x7A30D7E3u) store<u32>(c + 40, static_cast<u32>(call(ctx, 0x0009A190, {c, load<u32>(c + 40), x})));
    ctx.gpr[3] = call(ctx, 0x004705F8, {a, b, load<u32>(c + 24), load<u32>(c + 40), 1});
}

// T f(Cmd* c, Owner* x): for the kind-5 effect component
// Effect_FindComponent(x, c->+12): c->+40 = its object (+28), narrowed by
// WorldObj_FindById4A5AE8(o, c->+16) when set; for an object, c->+36 =
// the +212 of its target (+280 when +276 is 0x02235C33; raw; f1 = it) and
// ObjPool_Create(*0x00F64C38, c+32, &{0}). r3 as the last step left it.
DECOMP(00079F7C, Cmd_SpawnAt_00079F7C) {
    const u32 c = arg<u32>(ctx, 0), x = arg<u32>(ctx, 1);
    const u32 k = static_cast<u32>(call(ctx, 0x004BB4B4, {x, load<u32>(c + 12)}));
    if (k == 0) return;
    if (static_cast<u32>(VCall(ctx, k, 32, {k})) != 5) return;
    u32 o = load<u32>(k + 28);
    const u32 id = load<u32>(c + 16);
    store<u32>(c + 40, o);
    ctx.gpr[3] = id;
    if (id != 0) {
        o = static_cast<u32>(call(ctx, 0x004A5AE8, {o, id}));
        store<u32>(c + 40, o);
    }
    if (o == 0) return;
    if (load<u32>(o + 276) == 0x02235C33u) {
        const u32 t = load<u32>(o + 280);
        if (t != 0) {
            CopyW32(c + 36, t + 212);
            ctx.fpr[1] = Lfs32(t + 212);
        }
    }
    Stack frame(ctx, 16);
    const u32 z = frame.addr();  // sp+112
    store<u32>(z, 0);
    store<u16>(z + 2, 0);
    ctx.gpr[3] = call(ctx, 0x002F0E0C, {load<u32>(0x00F64C38), c + 32, z});
}

// T f(T* o, a, b): for o's object p (+32) whose hash flag
// (Obj_GetHashFlag_000AE35C(world + 80 + 4056 * u8 (world + 4208 +
// 0x20C80 * o->+36), p)) equals u8 o->+40, each member e of o's
// u16-counted list (+12 / +20) gets vt[12](e, a, b). r3 = the count (else
// the flag, 0 without p).
DECOMP(0007F4EC, Obj_BroadcastIfFlag_0007F4EC) {
    const u32 o = arg<u32>(ctx, 0), a = arg<u32>(ctx, 1), b = arg<u32>(ctx, 2);
    const u32 p = load<u32>(o + 32);
    ctx.gpr[3] = p;
    if (p == 0) return;
    const u32 w = load<u32>(0x00F65CA0);
    const u32 idx = load<u8>(w + load<u32>(o + 36) * 0x20C80u + 4208);
    const u32 f = static_cast<u32>(call(ctx, 0x000AE35C, {w + idx * 4056 + 80, p}));
    if (f != load<u8>(o + 40)) return;
    for (u32 i = 0;; ++i) {
        const u32 n = load<u16>(o + 20);
        ctx.gpr[3] = n;
        if (!(i < n)) return;
        const u32 e = load<u32>(load<u32>(o + 12) + 4 * i);
        VCall(ctx, e, 12, {e, a, b});
    }
}

// T* f(?, Xml* x): 24-byte node (vt 0x00B10328, s32 +12 = -1, +16 / +20 =
// 0): s32 0x18495581 -> +12 (default 0) and, when that read returns 0,
// the f32s 0x996B4041 -> +16 and 0xB7EA3D94 -> +20 (current defaults).
// r3 = p.
DECOMP(00088D94, XmlNew_B10328_00088D94) {
    const u32 x = arg<u32>(ctx, 1);
    const u32 p = XmlNodeAlloc32(ctx, 24, x);
    ctx.gpr[3] = 0;
    if (p == 0) return;
    store<u32>(p + 16, 0);
    store<u32>(p + 20, 0);
    store<u32>(p, 0x00B10328);
    store<u32>(p + 12, 0xFFFFFFFFu);
    if (static_cast<u32>(call(ctx, 0x004F99BC, {x, 0x18495581u, p + 12, 0})) == 0) {
        callf(ctx, 0x004F9B20, {x, 0x996B4041u, p + 16}, {Lfs32(p + 16)});
        callf(ctx, 0x004F9B20, {x, 0xB7EA3D94u, p + 20}, {Lfs32(p + 20)});
    }
    ctx.gpr[3] = p;
}

// Deleting ~T() (o may be 0; vt 0x00B12230): DtorB12230_32, then
// Mem_FreeFrom(o, 0). r3 as that left it.
DECOMP(000C2C9C, Delete_B12230_000C2C9C) {
    const u32 o = arg<u32>(ctx, 0);
    if (o == 0) return;  // (r3 = o = 0)
    DtorB12230_32(ctx);
    ctx.gpr[3] = call(ctx, o32fn::Mem_FreeFrom, {o, 0});
}

// bool f(T* o, -, Ent* e, x; f1 dmg): for dmg above K[0] (K = 0x000E875C;
// else r3 = 1): 0x000E1738(o, e, e, x; f1 = dmg * scale), the scale by
// e's kind ((+172 >> 22) & 3: tuning o->+216 +280 / +284 / +288 / +292,
// K[1] without e); then flag 0x80000000 of +420 set once o->+348 is at
// most K[0]. r3 = (o->+348 > K[0]), f1 = o->+348.
DECOMP(000E8768, Obj_TakeDamage_000E8768) {
    const u32 o = arg<u32>(ctx, 0), e = arg<u32>(ctx, 2), x = arg<u32>(ctx, 3);
    constexpr u32 K = 0x000E875C;
    const f64 dmg = ctx.fpr[1];
    const f64 k0 = Lfs32(K);
    if (dmg <= k0) {
        ctx.gpr[3] = 1;
        return;
    }
    const u32 st = load<u32>(o + 216);
    f64 sc = Lfs32(K + 4);
    if (e != 0) sc = Lfs32(st + 280 + 4 * ((load<u32>(e + 172) >> 22) & 3u));
    callf(ctx, 0x000E1738, {o, e, e, x}, {static_cast<f32>(dmg * sc)});
    const f64 h = Lfs32(o + 348);
    if (h <= k0) store<u32>(o + 420, load<u32>(o + 420) | 0x80000000u);
    ctx.gpr[3] = h > k0 ? 1u : 0u;
    ctx.fpr[1] = h;
}

// T f(a, b, u32 k, d, e, f): for a key k whose entry in the map
// 0x01032164 (SortedMap_Find744DD4) exists with u8 +125 = 2:
// 0x000F6A68(a, entry, b, d, e, f). r3 as the original leaves it.
DECOMP(000F8058, Obj_ApplyKind2_000F8058) {
    const u32 a = arg<u32>(ctx, 0), b = arg<u32>(ctx, 1), k = arg<u32>(ctx, 2);
    const u32 d = arg<u32>(ctx, 3), e = arg<u32>(ctx, 4), f = arg<u32>(ctx, 5);
    if (k == 0) return;
    constexpr u32 M = 0x01032164;
    Stack frame(ctx, 16);
    const u32 s = frame.addr();  // sp+112 / +116 / +120
    store<u32>(s + 8, k);
    ctx.gpr[3] = load<u32>(static_cast<u32>(call(ctx, 0x00744DD4, {M, s + 8})));
    if (ctx.gpr[3] == 0) return;
    store<u32>(s + 4, k);
    const u32 t = load<u8>(load<u32>(static_cast<u32>(call(ctx, 0x00744DD4, {M, s + 4}))) + 125) - 2u;
    ctx.gpr[3] = t;
    if (t != 0) return;
    store<u32>(s, k);
    const u32 v = load<u32>(static_cast<u32>(call(ctx, 0x00744DD4, {M, s})));
    ctx.gpr[3] = v;
    if (v == 0) return;
    ctx.gpr[3] = call(ctx, 0x000F6A68, {a, v, b, d, e, f});
}

// Deleting ~T() (o may be 0; vt 0x00B14D98): the 60-byte blocks of the
// u16-counted vector +296 (count +304) SmallFree'd from the back (count
// re-read and decremented after each; a count found 0 wraps as in
// DeleteOwnedVec33), the skeleton +292 released, the vector cleared, vt
// 0x00B11D98, Obj_BaseDtor_000C2DA0(o) and SmallFree(o, 320). r3 as that
// left it.
DECOMP(0018EEA0, Delete_B14D98_0018EEA0) {
    const u32 o = arg<u32>(ctx, 0);
    ctx.gpr[3] = 0;
    if (o == 0) return;
    store<u32>(o, 0x00B14D98);
    for (;;) {
        const u32 c0 = load<u16>(o + 304);
        if (c0 == 0) break;
        call(ctx, o32fn::SmallFree, {load<u32>(load<u32>(o + 296) + 4 * (c0 - 1)), 60});
        const u32 n = load<u16>(o + 304) - 1u;
        store<u16>(o + 304, static_cast<u16>(n));
        if (n != (n & 0xFFFFu)) {
            const u32 d = load<u32>(o + 296);
            store<u32>(d + 4 * n, load<u32>(d + ((n << 2) & 0x3FFFCu)));
        }
    }
    const u32 s = load<u32>(o + 292);
    if (s != 0) call(ctx, 0x007D4EA8, {s});
    PvClear32(ctx, o + 296);
    store<u32>(o, 0x00B11D98);
    call(ctx, 0x000C2DA0, {o});
    ctx.gpr[3] = call(ctx, o32fn::SmallFree, {o, 320});
}

// T f(T* o, Msg* m): for message 0xCBF1C4F3 with an effect e = o->+356:
// its kind-5 component 0x7A30D7E3 (Effect_FindComponent) drops its object
// (+28: vt[8](it), then 0) and the effect is released
// (Effect_Release(e, 0xCBF1C4F3)); then Obj_OnEvent_000E5E44(o, m). r3 as
// that left it.
DECOMP(001BE750, Obj_OnRelease_001BE750) {
    const u32 o = arg<u32>(ctx, 0), m = arg<u32>(ctx, 1);
    if (load<u32>(m + 8) == 0xCBF1C4F3u) {
        const u32 e = load<u32>(o + 356);
        if (e != 0) {
            const u32 k = static_cast<u32>(call(ctx, 0x004BB4B4, {e, 0x7A30D7E3u}));
            if (k != 0 && static_cast<u32>(VCall(ctx, k, 32, {k})) == 5) {
                const u32 q = load<u32>(k + 28);
                if (q != 0) VCall(ctx, q, 8, {q});
                store<u32>(k + 28, 0);
            }
            call(ctx, 0x004CF3C4, {load<u32>(o + 356), 0xCBF1C4F3u});
        }
    }
    ctx.gpr[3] = call(ctx, 0x000E5E44, {o, m});
}

// ~T() (vts 0x00B18480 + 8 / +436 / +476 when its attachment +500 is set,
// which then gets vt[8](it)): the 20-byte vector object +388 (when set,
// the vts first those of 0x00B12890) cleared and SmallFree'd; then vts
// 0x00B126B8 / 0x00B1200C / 0x00B12528 and VtB12550_Dtor_000DF934(o). r3
// as that left it.
DECOMP(001EB5E0, Dtor_B18480_001EB5E0) {
    const u32 o = arg<u32>(ctx, 0);
    constexpr u32 v = 0x00B18480;
    const u32 a = load<u32>(o + 500);
    if (a != 0) {
        store<u32>(o + 384, v + 476);
        store<u32>(o + 380, v + 436);
        store<u32>(o, v + 8);
        VCall(ctx, a, 8, {a});
    }
    const u32 q = load<u32>(o + 388);
    if (q != 0) {
        constexpr u32 w = 0x00B12890;
        store<u32>(o + 384, w + 476);
        store<u32>(o + 380, w + 436);
        store<u32>(o, w + 8);
        PvClear32(ctx, q);
        call(ctx, o32fn::SmallFree, {load<u32>(o + 388), 20});
    }
    store<u32>(o + 384, 0x00B12528);
    store<u32>(o + 380, 0x00B1200C);
    store<u32>(o, 0x00B126B8);
    ctx.gpr[3] = call(ctx, 0x000DF934, {o});
}

// T f(T* s, T* d): Thunk_000E44C0(s, d), then s's fields +240..+315 into
// d+208..+283 in the original's interleaved order (the f32s +240..+252 /
// +292..+304 raw; the u8s +308..+315 into d+276..+283). r3 = u8 s->+315,
// f1 = s->+300.
DECOMP(001ED4F0, Obj_CopyState_001ED4F0) {
    const u32 s = arg<u32>(ctx, 0), d = arg<u32>(ctx, 1);
    call(ctx, 0x000E44C0, {s, d});
    u32 f = load<u32>(s + 240), w = load<u32>(s + 256);
    store<u32>(d + 208, f);
    f = load<u32>(s + 244);
    store<u32>(d + 224, w);
    w = load<u32>(s + 260);
    store<u32>(d + 212, f);
    f = load<u32>(s + 248);
    store<u32>(d + 228, w);
    w = load<u32>(s + 264);
    store<u32>(d + 216, f);
    f = load<u32>(s + 252);
    store<u32>(d + 232, w);
    w = load<u32>(s + 268);
    store<u32>(d + 220, f);
    store<u32>(d + 236, w);
    u32 b = load<u8>(s + 308);
    w = load<u32>(s + 272);
    store<u8>(d + 276, static_cast<u8>(b));
    store<u32>(d + 240, w);
    w = load<u32>(s + 276);
    b = load<u8>(s + 309);
    f = load<u32>(s + 304);
    store<u32>(d + 244, w);
    store<u8>(d + 277, static_cast<u8>(b));
    store<u32>(d + 272, f);
    b = load<u8>(s + 310);
    w = load<u32>(s + 280);
    store<u8>(d + 278, static_cast<u8>(b));
    store<u32>(d + 248, w);
    b = load<u8>(s + 311);
    w = load<u32>(s + 284);
    f = load<u32>(s + 292);
    store<u8>(d + 279, static_cast<u8>(b));
    store<u32>(d + 252, w);
    b = load<u8>(s + 312);
    store<u32>(d + 260, f);
    w = load<u32>(s + 288);
    store<u8>(d + 280, static_cast<u8>(b));
    store<u32>(d + 256, w);
    b = load<u8>(s + 313);
    f = load<u32>(s + 296);
    store<u8>(d + 281, static_cast<u8>(b));
    store<u32>(d + 264, f);
    b = load<u8>(s + 314);
    f = load<u32>(s + 300);
    store<u8>(d + 282, static_cast<u8>(b));
    store<u32>(d + 268, f);
    b = load<u8>(s + 315);
    store<u8>(d + 283, static_cast<u8>(b));
    ctx.gpr[3] = b;
    ctx.fpr[1] = static_cast<f64>(std::bit_cast<f32>(f));
}

// T f(T* o, u32 k): the 8-bit field 0x07F80000 of +1268 = k; a temporary
// 140-byte effect on o->+36 (SmallAlloc, Effect_Construct) given the hash
// 0x5611CBE0 + k for k 0..2, then destroyed (Effect_Destruct, SmallFree).
// r3 as the last call left it.
DECOMP(002971BC, Obj_PlayCue_002971BC) {
    const u32 o = arg<u32>(ctx, 0), k = arg<u32>(ctx, 1);
    store<u32>(o + 1268, (load<u32>(o + 1268) & ~0x07F80000u) | ((k << 19) & 0x07F80000u));
    const u32 p = static_cast<u32>(call(ctx, 0x0055238C, {140}));
    u32 e = 0;
    if (p != 0) {
        call(ctx, 0x004C7A68, {p, load<u32>(o + 36)});
        e = p;
    }
    if (k <= 2) call(ctx, 0x004CF988, {e, 0x5611CBE0u + k});
    if (e == 0) return;
    call(ctx, 0x004C8E80, {e});
    ctx.gpr[3] = call(ctx, o32fn::SmallFree, {e, 140});
}

// T f(?, s32 i, s32 n): when n is below *0x010684BC->+72: the current
// level's (world + 4056 * u8 (world + 4208)) collectible i is marked (u8
// +728 + 44 * i = 1) and the marks of its 16 groups (bytes +728 / +772 /
// +816 / +860 of each 176-byte group) summed: one or more awards trophy
// 0x08047E80, six or more also trophy 0x2480DD71 (Trophy_Award0B27B4). r3
// as the original leaves it.
DECOMP(002E6A88, Level_MarkCollectible_002E6A88) {
    const u32 i = arg<u32>(ctx, 1), n = arg<u32>(ctx, 2);
    const u32 w = load<u32>(0x00F65CA0);
    const u32 lim = load<u32>(0x010684BCu + 72);
    u32 b = w + load<u8>(w + 4208) * 4056;
    ctx.gpr[3] = b;
    if (!(static_cast<s32>(lim) > static_cast<s32>(n))) return;
    store<u8>(b + 44 * i + 728, 1);
    u32 sum = 0;
    for (u32 k = 0; k < 16; ++k) {
        sum = sum + load<u8>(b + 728) + load<u8>(b + 772) + load<u8>(b + 816) + load<u8>(b + 860);
        b += 176;
    }
    ctx.gpr[3] = b;
    if (sum >= 1) call(ctx, 0x000B27B4, {0x08047E80u});
    if (sum < 6) return;
    ctx.gpr[3] = call(ctx, 0x000B27B4, {0x2480DD71u});
}

// u32 f(Section* o, s32 m, u8 force): Section_SetModeBase_003260F0(o, m,
// force), then each child c of the chain o->+2576 (next +2564) is told
// c->vt[104](c, m, force) unless (unforced, o->+2572 not 6) it is already
// active (flag 0x10 of +20 with u8 +18 = 5) and m is not 5; the active
// children are counted into o->+2580. r3 = the count.
DECOMP(003284C4, Section_SetModeChildren_003284C4) {
    const u32 o = arg<u32>(ctx, 0), m = arg<u32>(ctx, 1), f = arg<u32>(ctx, 2) & 0xFFu;
    call(ctx, 0x003260F0, {o, m, f});
    u32 c = load<u32>(o + 2576);
    store<u32>(o + 2580, 0);
    u32 cnt = 0;
    for (; c != 0; c = load<u32>(c + 2564)) {
        auto active = [&]() { return (load<u32>(c + 20) & 0x10u) != 0 && load<u8>(c + 18) == 5; };
        if (f == 0 && load<u32>(o + 2572) != 6 && (m == 5 || !active())) {
            VCall(ctx, c, 104, {c, m, f});
            cnt = load<u32>(o + 2580);
        }
        if (active()) {
            ++cnt;
            store<u32>(o + 2580, cnt);
        }
    }
    ctx.gpr[3] = cnt;
}

// T f(Recs* o, u8 v): with the notifier r = Obj_Notify24(o->+12->+8)
// (when set), each of the u16 o->+2580 144-byte records (o->+2572) gets
// u8 +48 = 1 (unless flag 0x10 of o->+20) and its +20 object removed from
// r (List_RemoveMatching_0029F510(r, it->+168)) and cleared; then
// Recs_Reset325A30(o, v). r3 as that left it.
DECOMP(0032C4E0, Recs_Release_0032C4E0) {
    const u32 o = arg<u32>(ctx, 0), v = arg<u32>(ctx, 1) & 0xFFu;
    u32 r = 0;
    const u32 p = load<u32>(load<u32>(o + 12) + 8);
    if (p != 0) r = static_cast<u32>(call(ctx, 0x000D949C, {p}));
    for (u32 i = 0; i < load<u16>(o + 2580); ++i) {
        const u32 e = load<u32>(o + 2572) + 144 * i;
        if ((load<u32>(o + 20) & 0x10u) == 0) store<u8>(e + 48, 1);
        if (r != 0) {
            const u32 t = load<u32>(e + 20);
            if (t != 0) call(ctx, 0x0029F510, {r, load<u32>(t + 168)});
        }
        store<u32>(e + 20, 0);
    }
    ctx.gpr[3] = call(ctx, 0x00325A30, {o, v});
}

// T* f(?, Xml* x, y): the f32s 0x00012A32 / 0x00012944 and the bool
// 0x6FE56119 read into locals (f32 defaults *0x0032CD10), then a
// 2584-byte object (SmallAlloc; Obj_Construct_00769BD8(p, y), vt
// 0x00B1E648) holding them at +2572 / +2576 / u8 +2580. r3 = p.
DECOMP(0032CD18, XmlNew_B1E648_0032CD18) {
    const u32 x = arg<u32>(ctx, 1), y = arg<u32>(ctx, 2);
    const f64 k = Lfs32(0x0032CD10);
    Stack frame(ctx, 16);
    const u32 l = frame.addr();  // sp+112 / +116 / +120
    callf(ctx, 0x004F9B20, {x, 0x00012A32u, l}, {k});
    callf(ctx, 0x004F9B20, {x, 0x00012944u, l + 4}, {k});
    call(ctx, 0x004F986C, {x, 0x6FE56119u, l + 8, 0});
    const u32 p = static_cast<u32>(call(ctx, 0x0055238C, {2584}));
    if (p != 0) {
        const u32 a = load<u32>(l), b = load<u32>(l + 4), c = load<u8>(l + 8);
        call(ctx, 0x00769BD8, {p, y});
        store<u32>(p + 2572, a);
        store<u32>(p + 2576, b);
        store<u8>(p + 2580, static_cast<u8>(c));
        store<u32>(p, 0x00B1E648);
    }
    ctx.gpr[3] = p;
}

// T f(T* o, a, Ev* b): in state 4 (u8 +18) with b->+0 = 6 and the clock
// *0x010592EC within o->+2572..+2576: o->vt[104](o, 5, 0), or (u8 +2580
// set) o->vt[104](o, 6, 0); then 0x003252D8(o, a, b). r3 as that left
// it; f1 = the clock when read.
DECOMP(0032CE1C, Section_OnTimer_0032CE1C) {
    const u32 o = arg<u32>(ctx, 0), a = arg<u32>(ctx, 1), b = arg<u32>(ctx, 2);
    if (load<u8>(o + 18) == 4 && load<u32>(b) == 6) {
        const f64 g = Lfs32(0x010592EC);
        ctx.fpr[1] = g;
        if (g >= Lfs32(o + 2572) && g <= Lfs32(o + 2576)) VCall(ctx, o, 104, {o, load<u8>(o + 2580) != 0 ? 6u : 5u, 0});
    }
    ctx.gpr[3] = call(ctx, 0x003252D8, {o, a, b});
}

// T f(Section* o, s32 m, u8 force): on a change (force, or m not the
// state u8 +18) to a mode other than 4, o->vt[28](o, 0) first; then
// Section_SetModeBase_003260F0(o, m, force); on a change to mode 4 with
// +2612 clear, o->vt[36](o, 0). r3 as the original leaves it.
DECOMP(0033380C, Section_SetModeB_0033380C) {
    const u32 o = arg<u32>(ctx, 0), m = arg<u32>(ctx, 1), f = arg<u32>(ctx, 2) & 0xFFu;
    const bool changed = f != 0 || m != load<u8>(o + 18);
    if (changed && m != 4) VCall(ctx, o, 28, {o, 0});
    ctx.gpr[3] = call(ctx, 0x003260F0, {o, m, f});
    if (!(changed && m == 4)) return;
    const u32 q = load<u32>(o + 2612);
    ctx.gpr[3] = q;
    if (q != 0) return;
    VCall(ctx, o, 36, {o, 0});
}

// T f(u32 k, a, u32 i): for the flagged (+4 bit 0) entry e of k in the map
// 0x0106C228 (SortedMap_Find3) and i below e->+8->+152: the object t of
// slot i (e->+328 + 16 * i, +12) gets t->vt[188](t, a, 0), then (re-read)
// vt[148](it). r3 as the original leaves it.
DECOMP(003675D8, Map_SlotCall_003675D8) {
    const u32 k = arg<u32>(ctx, 0), a = arg<u32>(ctx, 1), i = arg<u32>(ctx, 2);
    ctx.gpr[3] = 0;
    if (k == 0) return;
    Stack frame(ctx, 16);
    store<u32>(frame.addr(), k);  // sp+112
    const u32 e = load<u32>(static_cast<u32>(call(ctx, 0x0076611C, {0x0106C228u, frame.addr()})));
    ctx.gpr[3] = e;
    if (e == 0 || (load<u32>(e + 4) & 1u) == 0 || a == 0) return;
    if (!(i < load<u32>(load<u32>(e + 8) + 152))) return;
    const u32 s = load<u32>(e + 328) + (i << 4);
    const u32 t = load<u32>(s + 12);
    ctx.gpr[3] = t;
    if (t == 0) return;
    VCall(ctx, t, 188, {t, a, 0});
    const u32 t2 = load<u32>(s + 12);
    ctx.gpr[3] = VCall(ctx, t2, 148, {t2});
}

// T f(T* o): for a flagged (+184 bit 0x80000000) object in state (+160 >>
// 1) other than 0 and 3: o->+24 (when set) gets vt[40](it, 1, 1), the
// effect o->+20 the hash 0x02B81827 + o->+180, +160 = 6 | (+160 & 1),
// Speaker_SayId4A5C90(o->+28, o->+176, 0; f1 = *0x0036EFC0), then the
// effect (re-read) the hash 0x048129EB. r3 as the last call left it.
DECOMP(0036EFC8, Obj_EndTalk_0036EFC8) {
    const u32 o = arg<u32>(ctx, 0);
    if ((load<u32>(o + 184) & 0x80000000u) == 0) {
        ctx.gpr[3] = 0;
        return;
    }
    const u32 s = load<u32>(o + 160) >> 1;
    ctx.gpr[3] = s;
    if (s == 0 || s == 3) return;
    const u32 a = load<u32>(o + 24);
    if (a != 0) VCall(ctx, a, 40, {a, 1, 1});
    const u32 e = load<u32>(o + 20);
    const u32 x = load<u32>(o + 180);
    if (e != 0) call(ctx, 0x004CF988, {e, x + 0x02B81827u});
    const u32 w = load<u32>(o + 160);
    const u32 sp = load<u32>(o + 28);
    store<u32>(o + 160, 6u | (w & 1u));
    callf(ctx, 0x004A5C90, {sp, load<u32>(o + 176), 0}, {Lfs32(0x0036EFC0)});  // r3 stays as it returns
    const u32 e2 = load<u32>(o + 20);
    if (e2 != 0) call(ctx, 0x004CF988, {e2, 0x048129EBu});
}
