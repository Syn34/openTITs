// Game code, part 32 (continued): the 60-instruction functions of the
// small-function sweep; helpers in obj32.h. Each was checked against the
// original in the PPC simulator (memory writes, calls and their
// arguments, r3 and f1).
#include "obj32.h"

using namespace sly4;
using namespace sly4::obj32;

// ---------------------------------------------------------------- sweep: 60 instructions
// void f(T* o): with a target t = o->+256 flagged 0x40000 (+172): the unit
// direction and distance from o->+48's position to t's (both via
// vt[80](out, it)) into +272..+280 / +288; then flag 0x40 in u8 +306.
DECOMP(00016124, Obj_AimAtTarget_00016124) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 t = load<u32>(o + 256);
    if (t != 0 && (load<u32>(t + 172) & 0x40000u) != 0) {
        Stack frame(ctx, 32);
        const u32 b = frame.addr(), a = b + 16;  // sp+112 / +128
        VCall(ctx, t, 80, {a, t});
        const u32 q = load<u32>(o + 48);
        VCall(ctx, q, 80, {b, q});
        const f64 dy = static_cast<f32>(Lfs32(a + 4) - Lfs32(b + 4));
        const f64 dx = static_cast<f32>(Lfs32(a) - Lfs32(b));
        const f64 dz = static_cast<f32>(Lfs32(a + 8) - Lfs32(b + 8));
        f64 l = static_cast<f32>(dy * dy);
        l = static_cast<f32>(std::fma(dx, dx, l));
        l = static_cast<f32>(std::fma(dz, dz, l));
        l = static_cast<f32>(std::sqrt(l));
        const f64 nx = static_cast<f32>(dx / l);
        Stfs32(o + 288, l);
        const f64 ny = static_cast<f32>(dy / l), nz = static_cast<f32>(dz / l);
        Stfs32(o + 272, nx);
        Stfs32(o + 276, ny);
        Stfs32(o + 280, nz);
        ctx.fpr[1] = ny;
        ctx.fpr[2] = nx;
        ctx.fpr[3] = nz;
        ctx.fpr[4] = l;
    }
    const u32 v = load<u8>(o + 306) | 0x40u;
    store<u8>(o + 306, static_cast<u8>(v));
    ctx.gpr[3] = v;
}

DECOMP(000745C0, XmlH3_B0F608_New_000745C0) {
    XmlGen32(ctx, 24, 0x00B0F608, {{12, 'w'}, {16, 'w'}, {20, 'w'}, {0, 'v'}},
             {{'H', 0x7A30D7E3, 16, false}, {'H', 0x03D1FCFC, 20, false}, {'H', 0x8A93973F, 12, false}});
}

DECOMP(0007B4F8, XmlH3_B0F9A0_New_0007B4F8) {
    XmlGen32(ctx, 24, 0x00B0F9A0, {{12, 'w'}, {16, 'w'}, {20, 'w'}, {0, 'v'}},
             {{'H', 0x00011F9D, 12, false}, {'H', 0x7D385CF6, 16, false}, {'H', 0x82BF2A4A, 20, false}});
}

DECOMP(00081310, XmlH3_B0FD88_New_00081310) {
    XmlGen32(ctx, 24, 0x00B0FD88, {{12, 'w'}, {16, 'w'}, {0, 'v'}, {20, 'w'}},
             {{'H', 0xE79D9080, 12, false}, {'H', 0x4D4EF768, 16, false}, {'H', 0x40795527, 20, false}});
}

DECOMP(00085660, XmlHB2_B10058_New_00085660) {
    XmlGen32(ctx, 20, 0x00B10058, {{12, 'w'}, {16, 'B'}, {0, 'v'}, {17, 'B'}},
             {{'H', 0x7A30D7E3, 12, false}, {'B', 0x25F79944, 16, true}, {'B', 0x399AA9F7, 17, true}});
}

DECOMP(00086E28, XmlH3_B10170_New_00086E28) {
    XmlGen32(ctx, 24, 0x00B10170, {{12, 'w'}, {16, 'w'}, {0, 'v'}, {20, 'w'}},
             {{'H', 0x8A93973F, 12, false}, {'H', 0x7A30D7E3, 16, false}, {'H', 0x19AD5C28, 20, false}});
}

DECOMP(0008AF74, XmlH2B_B10558_New_0008AF74) {
    XmlGen32(ctx, 24, 0x00B10558, {{12, 'w'}, {16, 'w'}, {0, 'v'}, {20, 'b'}},
             {{'H', 0x8A93973F, 12, false}, {'H', 0x7A30D7E3, 16, true}, {'B', 0x469D7C52, 20, true}});
}

DECOMP(00096730, XmlHB2_B10BC0_New_00096730) {
    XmlGen32(ctx, 20, 0x00B10BC0, {{12, 'w'}, {16, 'b'}, {17, 'B'}, {0, 'v'}},
             {{'H', 0x7FAAD4A0, 12, false}, {'B', 0x2A1E8C78, 16, true}, {'B', 0x013A354D, 17, true}});
}

DECOMP(00096E50, XmlH3_B10C60_New_00096E50) {
    XmlGen32(ctx, 24, 0x00B10C60, {{12, 'w'}, {16, 'w'}, {0, 'v'}, {20, 'w'}},
             {{'H', 0x8A93973F, 12, false}, {'H', 0x7A30D7E3, 16, true}, {'H', 0x000107E5, 20, true}});
}

// The three-hash XML node of vtable 0x00B10CD8 (see XmlHash3_32).
DECOMP(00097618, XmlH3_B10CD8_New_00097618) { XmlHash3_32(ctx, 0x00B10CD8, 0x82EFC443); }

DECOMP(0009BE98, XmlH2B_B11078_New_0009BE98) {
    XmlGen32(ctx, 24, 0x00B11078, {{12, 'w'}, {16, 'w'}, {20, 'B'}, {0, 'v'}},
             {{'H', 0x7A30D7E3, 12, false}, {'H', 0x52A72CDD, 16, true}, {'B', 0x7A281983, 20, true}});
}

DECOMP(0009C2A4, XmlHB2_B110C8_New_0009C2A4) {
    XmlGen32(ctx, 20, 0x00B110C8, {{12, 'w'}, {17, 'b'}, {16, 'B'}, {0, 'v'}},
             {{'H', 0x7A30D7E3, 12, false}, {'B', 0x7A281983, 16, true}, {'B', 0x53B32BA4, 17, true}});
}

// AnimCmd node (AnimCmd_ConstructBase).
DECOMP(000A3518, XmlAnimCmd_B116C0_New_000A3518) {
    XmlGen32(ctx, 20, 0x00B116C0, {{12, 'w'}, {0, 'v'}, {16, 'B'}, {17, 'B'}},
             {{'H', 0x14929F25, 12, false}, {'B', 0x2EC28132, 16, true}, {'B', 0xBCDD3C19, 17, true}}, 0x004B9CB8);
}

// void f(T* o): by r = o->vt[32](o, 2): 0 -> the effect +356 (when set)
// gets the hash 0x9AA65B78 and o->vt[28](o, 2, 1); 1 -> 0x000B6A58(o) when
// the record (o->+20 + u16 +6) has f32 +0 <= o->+12->+8 or u8 +4 set.
DECOMP(000BDF40, Obj_Step_000BDF40) {
    const u32 o = arg<u32>(ctx, 0);
    const s32 r = static_cast<s32>(VCall(ctx, o, 32, {o, 2}));
    if (r > 0) {
        if (r > 1) return;
        const u32 rec = load<u32>(o + 20) + load<u16>(o + 6);
        const f64 f1 = Lfs32(load<u32>(o + 12) + 8), f2 = Lfs32(rec);
        ctx.fpr[1] = f1;
        ctx.fpr[2] = f2;
        if (!(f1 >= f2)) {
            ctx.gpr[3] = load<u8>(rec + 4);
            if (ctx.gpr[3] == 0) return;
        }
        ctx.gpr[3] = call(ctx, 0x000B6A58, {o});
        return;
    }
    if (r <= -1) return;
    if (load<u32>(o + 356) != 0) call(ctx, 0x004CF988, {load<u32>(o + 356), 0x9AA65B78u});
    ctx.gpr[3] = VCall(ctx, o, 28, {o, 2, 1});
}

// void f(T* o, Hit* m): the scaled hit reaction (HitScaled32; tuning
// o->+216 +256..+264, K 0x000E8308).
DECOMP(000E8310, Obj_OnHitScaled_000E8310) { HitScaled32(ctx, 0x000E8308, 256); }

// T* f(T* o): the type-73 object (Obj_CollectByType(73, &vec)) whose +168
// is o, else 0 (also for a null o).
DECOMP(000F6118, Obj_FindOwned73_000F6118) {
    const u32 o = arg<u32>(ctx, 0);
    ctx.gpr[3] = 0;
    if (o == 0) return;
    Stack frame(ctx, 16);
    const u32 v = frame.addr();  // sp+112
    call(ctx, 0x005516E0, {v});
    store<u16>(v + 6, 4);
    call(ctx, 0x000DD258, {73, v, 4});
    const u32 n = load<u16>(v + 8), data = load<u32>(v);
    u32 found = 0;
    for (u32 i = 0; i < n; ++i) {
        const u32 e = load<u32>(data + 4 * i);
        if (load<u32>(e + 168) == o) {
            found = e;
            break;
        }
    }
    PvClear32(ctx, v);
    ctx.gpr[3] = found;
}

// char* f(T* o): String_Format(0x00AC6D74, name(o->+16), name(o->+440),
// (double)o->+644) with name(p) = p->vt[76](p), or 0x00AC6DA4 for null.
DECOMP(0010660C, Obj_Describe_0010660C) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 a = load<u32>(o + 16);
    const u32 s1 = a != 0 ? static_cast<u32>(VCall(ctx, a, 76, {a})) : 0x00AC6DA4u;
    const u32 b = load<u32>(o + 440);
    const u32 s2 = b != 0 ? static_cast<u32>(VCall(ctx, b, 76, {b})) : 0x00AC6DA4u;
    const f64 f = Lfs32(o + 644);
    callf(ctx, 0x005534D0, {0x00AC6D74u, s1, s2, std::bit_cast<u64>(f)}, {f});  // the vararg double in f1 and r6
}

// T* f(Stack* s, T* x): removes x from the u32 stack s+540 (count +536):
// the top when it is x (and the count > 1), else the topmost match below
// it (entries 1..top; entry 0 never matches), shifting the rest down and
// clearing the old top. r3 = x, or 0 when not found.
DECOMP(001071CC, Stack_Remove_001071CC) {
    const u32 s = arg<u32>(ctx, 0), x = arg<u32>(ctx, 1);
    const u32 n = load<u32>(s + 536);
    if (n > 1) {
        const u32 e = s + 540 + 4 * (n - 1);
        if (load<u32>(e) == x) {
            store<u32>(e, 0);
            store<u32>(s + 536, load<u32>(s + 536) - 1);
            ctx.gpr[3] = x;
            return;
        }
    }
    const u32 top = n - 1;
    ctx.gpr[3] = 0;
    if (static_cast<s32>(top) <= 0) return;
    u32 i = top;
    while (load<u32>(s + 540 + 4 * i) != x) {
        --i;
        if (static_cast<s32>(i) <= 0) return;
    }
    store<u32>(s + 536, top);
    u32 last = top;
    if (static_cast<s32>(i) < static_cast<s32>(top)) {
        do {
            store<u32>(s + 540 + 4 * i, load<u32>(s + 544 + 4 * i));
            ++i;
            last = load<u32>(s + 536);
        } while (static_cast<s32>(i) < static_cast<s32>(last));
    }
    store<u32>(s + 540 + (last << 2), 0);
    ctx.gpr[3] = x;
}

// void f(T* o, u8 off): Flag_Set172_80_000DA0CC(o, off); the physics body b
// = o->+388 is added to the world (*0x00F65CA0)->+0x1281D4->+4
// (Hk_AddEntity92F2A8(w, b, 1) when o->+396 is 1, plus 0x0092E9D8(w,
// o->+392) when set) when off is 0 and b is not in a world (b->+8 null),
// and removed (Hk_RemoveEntity92FFC8(&tmp, b->+8, b)) when off and it is.
DECOMP(001684EC, Obj_SetPhysOff_001684EC) {
    const u32 o = arg<u32>(ctx, 0), off = arg<u32>(ctx, 1) & 0xFFu;
    call(ctx, 0x000DA0CC, {o, off});
    const u32 b = load<u32>(o + 388);
    ctx.gpr[3] = b;
    if (b == 0) return;
    const u32 w = load<u32>(b + 8);
    if ((w != 0) == (off == 0)) return;
    if (off != 0) {
        Stack frame(ctx, 16);
        ctx.gpr[3] = call(ctx, 0x0092FFC8, {frame.addr(), w, b});
        return;
    }
    const u32 world = load<u32>(load<u32>(load<u32>(0x00F65CA0) + 0x1281D4) + 4);
    if (load<u32>(o + 396) != 1) return;
    call(ctx, 0x0092F2A8, {world, b, 1});
    const u32 c = load<u32>(o + 392);
    if (c == 0) return;
    ctx.gpr[3] = call(ctx, 0x0092E9D8, {world, c});
}

// T* f(?, Xml* x, a, b): a new 240-byte object (SmallAlloc,
// 0x00750680(p, a, b, 0, 0x01164BD0, 0x01164B30, 1, 0; stack 0, 0,
// 0x01164B40, 0x01164B40, -1, 0)) and the s32 0x803BA106 into p+228 (also
// for a failed allocation: the original then reads/writes address 228).
DECOMP(001BB830, Obj_NewFromXml_001BB830) {
    const u32 x = arg<u32>(ctx, 1), a = arg<u32>(ctx, 2), b = arg<u32>(ctx, 3);
    u32 p = static_cast<u32>(call(ctx, 0x0055238C, {240}));
    if (p != 0) call(ctx, 0x00750680, {p, a, b, 0, 0x01164BD0u, 0x01164B30u, 1, 0, 0, 0, 0x01164B40u, 0x01164B40u, ~0ull, 0});
    const u64 d = static_cast<u64>(static_cast<s64>(static_cast<s32>(load<u32>(p + 228))));
    call(ctx, 0x004F99BC, {x, 0x803BA106u, p + 228, d});
    ctx.gpr[3] = p;
}

// bool f(T* o, T* b, f32 t, f32 lim): 1 when allowed (+556/+564 clear and
// the f32 +576 > K (0x001C7E34) with flag 0x20000000 of +580 and not
// 0x10000000 of b->+444, or flag 0x20000000 of +552, or lim < K or t <
// lim); else o->vt[420](o, 5, 0) and 0.
DECOMP(001C7E38, Obj_CheckLimit_001C7E38) {
    const u32 o = arg<u32>(ctx, 0), b = arg<u32>(ctx, 1);
    const f64 t = farg(ctx, 0), lim = farg(ctx, 1);
    const f64 k = Lfs32(0x001C7E34);
    bool test = true;
    if (load<u32>(o + 556) == 0 && load<u32>(o + 564) == 0) {
        ctx.fpr[3] = Lfs32(o + 576);
        ctx.fpr[4] = k;
        if (ctx.fpr[3] <= k) test = false;
    }
    if (test && (load<u32>(o + 580) & 0x20000000u) != 0 && (load<u32>(b + 444) & 0x10000000u) == 0) {
        ctx.gpr[3] = 1;
        return;
    }
    ctx.gpr[3] = 1;
    if ((load<u32>(o + 552) & 0x20000000u) != 0) return;
    ctx.fpr[3] = k;
    if (!(lim >= k) || !(t >= lim)) return;
    VCall(ctx, o, 420, {o, 5, 0});
    ctx.gpr[3] = 0;
}

// bool f(T* o, f32 t, f32 lim): 1 when o->vt[32](o, 1), u8 +853 / +854 or
// flag 0x20000000 of +552; else when lim < K (0x001CFB34) or t >= lim,
// o->vt[420](o, 1, 0; f1 = K) and 0; 1 otherwise.
DECOMP(001CFB38, Obj_CheckLimit2_001CFB38) {
    const u32 o = arg<u32>(ctx, 0);
    const f64 t = farg(ctx, 0), lim = farg(ctx, 1);
    const u32 r = static_cast<u32>(VCall(ctx, o, 32, {o, 1}));
    const u32 vt = load<u32>(o);
    ctx.gpr[3] = 1;
    if (r != 0 || load<u8>(o + 853) != 0 || load<u8>(o + 854) != 0 || (load<u32>(o + 552) & 0x20000000u) != 0) return;
    const f64 k = Lfs32(0x001CFB34);
    ctx.fpr[1] = k;
    if (!(lim < k) && !(t >= lim)) return;
    VCallVt(ctx, vt, 420, {o, 1, 0});
    ctx.gpr[3] = 0;
}

// s32 f(T* o, T* a, s32 k, d): 0x000E7FF0(o, a, k, d) unless k is 3 with a
// target definition (Target_GetDef0D951C, +68 <= 1), o's u8 +436 without
// 0x80 and a flagged 8 (+192) whose +424 differs from o->+432 (then 0).
DECOMP(001E411C, Obj_Forward_001E411C) {
    const u32 o = arg<u32>(ctx, 0), a = arg<u32>(ctx, 1), k = arg<u32>(ctx, 2), d = arg<u32>(ctx, 3);
    if (k == 3 && static_cast<u32>(call(ctx, 0x000D951C, {o})) != 0) {
        const u32 def = static_cast<u32>(call(ctx, 0x000D951C, {o}));
        if (static_cast<s32>(load<u32>(def + 68)) <= 1 && (load<u8>(o + 436) & 0x80u) == 0 && a != 0 && (load<u32>(a + 192) & 8u) != 0 &&
            load<u32>(a + 424) != load<u32>(o + 432)) {
            ctx.gpr[3] = 0;
            return;
        }
    }
    ctx.gpr[3] = call(ctx, 0x000E7FF0, {o, a, k, d});
}

// bool f(T* o): o->vt[28](o, 0, 1), Msg_Post(o, 0x55B67D7C, 0x57C66C13, 0),
// then with the definition's +64 target t: 0x001EDD84(o, t) and +968 = the
// f32 +104 of the world's player slot (s8)t->+172 (0x20C80-byte slots at
// +4208); +508 = 0 and flag 0x00200000 of +980 cleared. r3 = 1.
DECOMP(001EE3E4, Obj_Reengage_001EE3E4) {
    const u32 o = arg<u32>(ctx, 0);
    VCall(ctx, o, 28, {o, 0, 1});
    call(ctx, 0x00470660, {o, 0x55B67D7Cu, 0x57C66C13u, 0});
    const u32 def = static_cast<u32>(call(ctx, 0x000D951C, {o}));
    const u32 t = load<u32>(def + 64);
    if (t != 0) call(ctx, 0x001EDD84, {o, t});
    const u32 w = load<u32>(o + 980);
    store<u32>(o + 508, 0);
    store<u32>(o + 980, w & ~0x00200000u);
    if (t != 0) {
        const s32 slot = static_cast<s8>(load<u8>(t + 172));
        const u32 e = load<u32>(0x00F65CA0) + static_cast<u32>(slot) * 0x20C80u + 4208;
        ctx.fpr[1] = Lfs32(e + 104);
        CopyW32(o + 968, e + 104);
    }
    ctx.gpr[3] = 1;
}

// Material* f(): a material for the shader 0xBDEA8CED (Shader_Find /
// Material_Create) with its blend state at +104 set (+0 / +8 flag
// 0x80000000 and fields 770 / 515 << 15, u16 +4 = 0x8006, +6 = 771, +12 =
// 2304) and the default texture added four times. r3 = it.
DECOMP(00258D44, Material_NewBlend_00258D44) {
    const u32 s = static_cast<u32>(call(ctx, 0x004FEBD0, {0xBDEA8CEDu}));
    const u32 m = static_cast<u32>(call(ctx, 0x004FF43C, {s}));
    const u32 p = m + 104;
    const u32 w8 = (load<u32>(p + 8) & ~0x7FFF8000u) | ((515u << 15) & 0x7FFF8000u);
    const u32 w0 = ((load<u32>(p) | 0x80000000u) & ~0x7FFF8000u) | ((770u << 15) & 0x7FFF8000u);
    store<u16>(p + 6, 771);
    store<u32>(p, w0);
    store<u32>(p + 8, w8 | 0x80000000u);
    store<u16>(p + 4, 0x8006);
    store<u16>(p + 12, 2304);
    const u32 t = static_cast<u32>(call(ctx, 0x004FEF30, {0}));
    for (u32 k = 0; k < 4; ++k) call(ctx, 0x004FF008, {m, t});
    ctx.gpr[3] = m;
}

// void f(T* o, Contact* c): the contact side that is not o's body
// (Hk_GetBody9D19C8(o->+32); c->+8 unless its owner is that body, then
// c->+12; owner = side + 16 + (s8)side->+32 when (s8)side->+40 is 1) has
// its owner o2 (same rule, else 0); for u8 o2->+232 of 4 or 5,
// o->+8->vt[384](it, o2). r3 = o2 otherwise.
DECOMP(0026EEB4, Contact_Notify_0026EEB4) {
    const u32 o = arg<u32>(ctx, 0), c = arg<u32>(ctx, 1);
    const u32 t = load<u32>(o + 8);
    const u32 h = static_cast<u32>(call(ctx, 0x009D19C8, {load<u32>(o + 32)}));
    u32 x = load<u32>(c + 8);
    u32 own = 0;
    if (static_cast<s8>(load<u8>(x + 40)) == 1) own = x + static_cast<u32>(static_cast<s32>(static_cast<s8>(load<u8>(x + 32)))) + 16;
    if (own == h) x = load<u32>(c + 12);
    const u32 y = x + 16;
    u32 o2 = 0;
    if (static_cast<s8>(load<u8>(y + 24)) == 1) o2 = y + static_cast<u32>(static_cast<s32>(static_cast<s8>(load<u8>(y + 16))));
    const u32 b = load<u8>(o2 + 232);
    ctx.gpr[3] = o2;
    if (b != 5 && b != 4) return;
    ctx.gpr[3] = VCall(ctx, t, 384, {t, o2});
}

// void f(T* o, u8 on): the anim object a = o->+436: off ->
// Obj_Dispatch362(a, 1, 1, 0); on -> Obj_SetAnim(a, 0, 1, -, 0, 0,
// 0x01164CA4; f1 = *0x002C6E28), +428 = -1, and when a has flag 0x40000000
// (+396): +428 = 0 and its effect +20 gets the hash of String_Format(
// 0x00AD55D8, 0x00AD55E8, 0).
DECOMP(002C6E30, Obj_SetAnimOn_002C6E30) {
    const u32 o = arg<u32>(ctx, 0), on = arg<u32>(ctx, 1) & 0xFFu;
    const u32 a = load<u32>(o + 436);
    ctx.gpr[3] = a;
    if (a == 0) return;
    if (on == 0) {
        ctx.gpr[3] = call(ctx, 0x0036325C, {a, 1, 1, 0});
        return;
    }
    callf(ctx, 0x00363A3C, {a, 0, 1, ctx.gpr[6], 0, 0, 0x01164CA4u}, {Lfs32(0x002C6E28)});  // r6 as the caller left it (unread)
    const u32 a2 = load<u32>(o + 436);
    store<u32>(o + 428, 0xFFFFFFFFu);
    ctx.gpr[3] = a2;
    if (a2 == 0 || (load<u32>(a2 + 396) & 0x40000000u) == 0) return;
    store<u32>(o + 428, 0);
    const u32 e = load<u32>(a2 + 20);
    if (e == 0) return;
    const u32 s = static_cast<u32>(call(ctx, 0x005534D0, {0x00AD55D8u, 0x00AD55E8u, 0}));
    const u32 h = static_cast<u32>(call(ctx, 0x0056A710, {s}));
    ctx.gpr[3] = call(ctx, 0x004CF988, {e, h});
}

// bool f(T* o, u32 idx): the list o->+8->+340 (created on first use: a
// 20-byte PtrVector with u16 +6 = 240); r3 = 0 when one of o's 256-byte
// records (o->+68, u16 count +76) has +208 equal to the list's record idx
// (240-byte entries) +192, else 1.
DECOMP(002D375C, Obj_HasRecord_002D375C) {
    const u32 o = arg<u32>(ctx, 0), idx = arg<u32>(ctx, 1);
    const u32 slot = load<u32>(o + 8) + 340;
    u32 l = load<u32>(slot);
    if (l == 0) {
        l = static_cast<u32>(call(ctx, 0x0055238C, {20}));
        store<u32>(slot, l);
        if (l != 0) {
            call(ctx, 0x005516E0, {l});
            store<u16>(l + 6, 240);
            l = load<u32>(slot);
        }
    }
    ctx.gpr[3] = 0;
    for (u32 i = 0; i < load<u16>(o + 76); ++i)
        if (load<u32>(load<u32>(o + 68) + 256 * i + 208) == load<u32>(load<u32>(l) + idx * 240 + 192)) return;
    ctx.gpr[3] = 1;
}

// Deleting ~T() (vt 0x00B1D650, 80 bytes): each element e of the vector
// +24 (u16 count +32) gets e->vt[40](e, 0); count 0, the vector cleared,
// vt 0x00B1D600, SmallFree(o, 80). Nothing for a null o.
DECOMP(002FB5B4, Dtor_B1D650_002FB5B4) {
    const u32 o = arg<u32>(ctx, 0);
    if (o == 0) return;
    store<u32>(o, 0x00B1D650);
    for (u32 i = 0; i < load<u16>(o + 32); ++i) {
        const u32 e = load<u32>(load<u32>(o + 24) + 4 * i);
        VCall(ctx, e, 40, {e, 0});
    }
    store<u16>(o + 32, 0);
    PvClear32(ctx, o + 24);
    store<u32>(o, 0x00B1D600);
    ctx.gpr[3] = call(ctx, o32fn::SmallFree, {o, 80});
}

// void f(T* o): the current +68 (its +48..+60 zeroed) and +36 / +68
// cleared, the 12-byte elements of the vectors +72 (u16 count +80) and +92
// (count +100) SmallFree'd and both counts 0, u16 +140 / +120 = 0. r3 = 0.
DECOMP(0033C03C, Obj_ClearLists_0033C03C) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 p = load<u32>(o + 68);
    if (p != 0) {
        store<u32>(p + 56, 0);
        store<u32>(p + 60, 0);
        store<u32>(p + 48, 0);
        store<u32>(p + 52, 0);
    }
    store<u32>(o + 36, 0);
    store<u32>(o + 68, 0);
    for (u32 i = 0; i < load<u16>(o + 80); ++i) call(ctx, o32fn::SmallFree, {load<u32>(load<u32>(o + 72) + 4 * i), 12});
    store<u16>(o + 80, 0);
    for (u32 i = 0; i < load<u16>(o + 100); ++i) call(ctx, o32fn::SmallFree, {load<u32>(load<u32>(o + 92) + 4 * i), 12});
    store<u16>(o + 100, 0);
    store<u16>(o + 140, 0);
    store<u16>(o + 120, 0);
    ctx.gpr[3] = 0;
}

// void f(T* o, u8 on): OnEach32 over o->+8's 16-byte entries (+148, count
// +152, object +4).
DECOMP(00370130, Obj_SetOnAll16_00370130) { OnEach32(ctx, 152, 148, 16, 4); }

// void f(T* o): o->vt[20](o, 0), Obj_Place3626E8(o, 0x01164B40, 1), +168 /
// +336 / +340 = 0, +160 &= 1, the +344 object released (vt[4]) and
// cleared, flags 0x37000000 of +360 cleared (and 0xC0000000 too when
// 0x40000000 was set). r3 = the flags.
DECOMP(003733BC, Obj_Reset_003733BC) {
    const u32 o = arg<u32>(ctx, 0);
    VCall(ctx, o, 20, {o, 0});
    call(ctx, 0x003626E8, {o, 0x01164B40u, 1});
    const u32 w = load<u32>(o + 160) & 1u;
    const u32 p = load<u32>(o + 344);
    store<u32>(o + 168, 0);
    store<u32>(o + 336, 0);
    store<u32>(o + 340, 0);
    store<u32>(o + 160, w);
    if (p != 0) {
        VCall(ctx, p, 4, {p});
        store<u32>(o + 344, 0);
    }
    u32 f = load<u32>(o + 360) & ~0x37000000u;
    store<u32>(o + 360, f);
    if ((f & 0x40000000u) != 0) {
        f &= 0x3FFFFFFFu;
        store<u32>(o + 360, f);
    }
    ctx.gpr[3] = f;
}

// void f(T* o, u8 on): OnEach32 over o->+8's 28-byte entries (+196, count
// +200, object +12).
DECOMP(00374704, Obj_SetOnAll28_00374704) { OnEach32(ctx, 200, 196, 28, 12); }

// bool f(Ref* a, T* b): the object b->+12->vt[68]() (when set) gets its
// +24's f32 +76 = the table value for a (RefEntry32). r3 = 0.
DECOMP(003954FC, Ref_SetValue_003954FC) {
    const u32 a = arg<u32>(ctx, 0), b = arg<u32>(ctx, 1);
    const u32 c = load<u32>(b + 12);
    const u32 r = static_cast<u32>(VCall(ctx, c, 68, {c}));
    if (r != 0) {
        const u32 d = load<u32>(r + 24);
        const u32 e = RefEntry32(a);
        ctx.fpr[1] = Lfs32(e);
        CopyW32(d + 76, e);
    }
    ctx.gpr[3] = 0;
}

// u32 f(T* o): a random variant for the mode o->+408: 2 -> seed & 1, 3 / 4
// -> seed % 3 (the seed 0x01164890 before the inlined random step), else 0.
DECOMP(003A7264, Obj_RandVariant_003A7264) {
    const u32 o = arg<u32>(ctx, 0);
    const s32 k = static_cast<s32>(load<u32>(o + 408));
    ctx.gpr[3] = 0;
    if (k != 2 && k != 3 && k != 4) return;
    const u32 s = load<u32>(0x01164890);
    RandStep28();
    ctx.gpr[3] = k == 2 ? (s & 1u) : (s % 3u);
}

// void f(Msg* m): the object of id m->+12 (map *0x00F65BE8 + 28;
// Hash_ToString when missing) gets 0x004563A0(it, u8 +18) when u8 +17,
// then with u8 +16: 0x0045488C(it) when u8 +19 and 0x00454220(it, 0);
// without: Obj_ForEachSet_004562F4(it, 0). Then Flag_Set8(m).
DECOMP(0042E8F8, Msg_Apply_0042E8F8) {
    const u32 m = arg<u32>(ctx, 0);
    const u32 id = load<u32>(m + 12);
    if (id != 0) {
        Stack frame(ctx, 16);
        const u32 key = frame.addr();  // sp+112
        store<u32>(key, id);
        const u32 o = load<u32>(static_cast<u32>(call(ctx, 0x0073B608, {load<u32>(0x00F65BE8) + 28, key})));
        if (o == 0) call(ctx, 0x0056A6E0, {load<u32>(m + 12)});
        if (o != 0) {
            if (load<u8>(m + 17) != 0) call(ctx, 0x004563A0, {o, load<u8>(m + 18)});
            if (load<u8>(m + 16) != 0) {
                if (load<u8>(m + 19) != 0) call(ctx, 0x0045488C, {o});
                call(ctx, 0x00454220, {o, 0});
            } else {
                call(ctx, 0x004562F4, {o, 0});
            }
        }
    }
    ctx.gpr[3] = call(ctx, 0x0042CA60, {m});
}

// T::T(T* o, a; f32 x, y, z): Obj_InitVt_B20CC8(o, 1), vt 0x00B21148, the
// defaults of 0x01164BD0 (+16..+72 as 0x0042D72C lays them out), +80 = a,
// +84/+88/+92 = x/y/z, +96 = 0. r3 = 0.
DECOMP(00434E0C, Obj_Ctor_B21148_00434E0C) {
    const u32 o = arg<u32>(ctx, 0), a = arg<u32>(ctx, 1);
    const f64 x = farg(ctx, 0), y = farg(ctx, 1), z = farg(ctx, 2);
    constexpr u32 D = 0x01164BD0;
    call(ctx, 0x0042C9D0, {o, 1});
    CopyW32(o + 16, D);
    CopyW32(o + 20, D + 4);
    CopyW32(o + 24, D + 8);
    CopyW32(o + 32, D + 16);
    CopyW32(o + 36, D + 20);
    CopyW32(o + 40, D + 24);
    const u32 w32 = load<u32>(D + 32);
    store<u32>(o, 0x00B21148);
    store<u32>(o + 48, w32);
    CopyW32(o + 52, D + 36);
    CopyW32(o + 56, D + 40);
    CopyW32(o + 64, D + 48);
    const u32 w52 = load<u32>(D + 52);
    store<u32>(o + 80, a);
    store<u32>(o + 68, w52);
    const u32 w56 = load<u32>(D + 56);
    Stfs32(o + 84, x);
    store<u32>(o + 72, w56);
    Stfs32(o + 88, y);
    Stfs32(o + 92, z);
    store<u32>(o + 96, 0);
    ctx.fpr[1] = std::bit_cast<f32>(w56);
    ctx.gpr[3] = 0;
}

// void Voice_SetF32(u32 h, f32 v): VoiceCmd32 kind 2 with the f32 at +32.
DECOMP(0044CB18, Voice_SetF32_0044CB18) {
    VoiceCmd32(ctx, 2, 32, std::bit_cast<u32>(static_cast<f32>(farg(ctx, 0))));
}

// void Voice_SetU32(u32 h, u32 v): VoiceCmd32 kind 1 with v at +28.
DECOMP(0044CC08, Voice_SetU32_0044CC08) { VoiceCmd32(ctx, 1, 28, arg<u32>(ctx, 1)); }

// void f(?, u8 a, f32 x, y, z): the global objects 0x010B9EF0
// (vt[12](it, a)) and 0x010B9F3C / 0x010B9F88 / 0x010B9FD4 (vt[20](it;
// f1 = x / y / z)).
DECOMP(0048C6EC, Globals_Set4_0048C6EC) {
    const u32 a = arg<u32>(ctx, 1) & 0xFFu;
    const f64 x = farg(ctx, 0), y = farg(ctx, 1), z = farg(ctx, 2);
    VCall(ctx, 0x010B9EF0, 12, {0x010B9EF0u, a});
    ctx.fpr[1] = x;
    VCall(ctx, 0x010B9F3C, 20, {0x010B9F3Cu});
    ctx.fpr[1] = y;
    VCall(ctx, 0x010B9F88, 20, {0x010B9F88u});
    ctx.fpr[1] = z;
    ctx.gpr[3] = VCall(ctx, 0x010B9FD4, 20, {0x010B9FD4u});
}

// T* f(?, Xml* x): a 36-byte node (Mem_Alloc(36, 0, 16); u16 +4 = 1, vt
// 0x00B21FD8, the vector +8 constructed (u16 +14 = 4) and
// PtrVector_Init(+8, 0, 0, 0), +28 = 0, vt 0x00B22050, +32 = 0x03F47A7A)
// and the hash 0x0024728B -> +32 (also for a failed allocation: address
// 32). r3 = p.
DECOMP(004C17E8, XmlVecNode_New_004C17E8) {
    const u32 x = arg<u32>(ctx, 1);
    const u32 p = static_cast<u32>(call(ctx, 0x0054ED38, {36, 0, 16}));
    if (p != 0) {
        store<u16>(p + 4, 1);
        store<u32>(p, 0x00B21FD8);
        call(ctx, 0x005516E0, {p + 8});
        store<u16>(p + 14, 4);
        call(ctx, 0x00551598, {p + 8, 0, 0, 0});
        store<u32>(p + 28, 0);
        store<u32>(p, 0x00B22050);
        store<u32>(p + 32, 0x03F47A7A);
    }
    call(ctx, 0x004FA14C, {x, 0x0024728Bu, p + 32, load<u32>(p + 32)});
    ctx.gpr[3] = p;
}

// s32 f(u16 v, Pool** pp, u8** out): takes the next 132-byte slot of the
// pool b = *pp - 152 (s16 index +144, post-incremented): slot word = 7,
// then at slot + 4 the u16s {v, +2 with bits 0xC000 cleared} and, with out,
// *out = slot + 4 + u16 0x0106CF84 (its byte zeroed when non-null). r3 =
// the index.
DECOMP(00774D2C, Pool_Take_00774D2C) {
    const u32 v = arg<u32>(ctx, 0), pp = arg<u32>(ctx, 1), out = arg<u32>(ctx, 2);
    const u32 b = load<u32>(pp) - 152;
    const u32 u = load<u16>(b + 144);
    const s32 c = static_cast<s16>(u);
    const u32 n1 = u + 1;
    store<u16>(b + 144, static_cast<u16>(n1));
    const u32 e = b + 152 + static_cast<u32>(c) * 132u;
    store<u32>(e, 7);
    u32 s = 0;
    if (c < static_cast<s32>(n1 & 0xFFFFu)) s = static_cast<s32>(load<u32>(e)) < 10 ? e : 0u;
    s += 4;
    if (s != 0) store<u16>(s + 2, static_cast<u16>(load<u16>(s + 2) & ~0xC000u));
    store<u16>(s, static_cast<u16>(v));
    if (out != 0) {
        const u32 q = s + load<u16>(0x0106CF84);
        store<u32>(out, q);
        if (q != 0) store<u8>(q, 0);
    }
    ctx.gpr[3] = static_cast<u64>(static_cast<s64>(c));
}

// Deleting ~T() (vt 0x00B20A48, 496 bytes): the targets +332 / +336 with
// flag 0x40000 in their +172 get 0x1000 in their u16 +176 and are cleared;
// the vector +164 cleared, vt 0x00B207A8, Dtor_B219D0_00490B34(o),
// SmallFree(o, 496). Nothing for a null o.
DECOMP(0077F248, Dtor_B20A48_0077F248) {
    const u32 o = arg<u32>(ctx, 0);
    if (o == 0) return;
    const u32 p = load<u32>(o + 332);
    store<u32>(o, 0x00B20A48);
    if (p != 0 && (load<u32>(p + 172) & 0x40000u) != 0) store<u16>(p + 176, static_cast<u16>(load<u16>(p + 176) | 0x1000u));
    const u32 q = load<u32>(o + 336);
    store<u32>(o + 332, 0);
    if (q != 0 && (load<u32>(q + 172) & 0x40000u) != 0) store<u16>(q + 176, static_cast<u16>(load<u16>(q + 176) | 0x1000u));
    store<u32>(o + 336, 0);
    PvClear32(ctx, o + 164);
    store<u32>(o, 0x00B207A8);
    call(ctx, 0x00490B34, {o});
    ctx.gpr[3] = call(ctx, o32fn::SmallFree, {o, 496});
}
