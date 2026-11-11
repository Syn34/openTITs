// Game code, part 42 (continued): more of the small-function sweep, from
// the 90 instruction functions up. Each was checked against the original
// in the PPC simulator (memory writes, calls and their arguments, r3 and
// f1).
#include "obj42.h"

using namespace sly4;
using namespace sly4::obj42;

// ---------------------------------------------------------------- sweep: 90 instructions
// void SetLit(T* o, u8 on): on: bits 3 / 4 of u8 o->+2336 cleared and bit
// 7 set; an effect o->+2276 whose component (+4)->+8 is 0x9F6559A5 gets
// Effect_SetHash4CF988(e, 0xD3380BFC) and is kept, any other is deleted
// and replaced by Spawn_Effect25A8A8(o, o->+2264, 0); a kept / new effect
// with a 0x04B1112A component of kind 3 (vt[32]) gets +36 = 1.0 and +32 /
// +40 = o->+2272. off: the effect gets Effect_SetHash4CF988(e,
// 0xDC2411A6), bit 4 of +2336 set and bit 3 cleared. r3 / f1 as the
// original leaves them.
DECOMP(0029A420, Obj_SetLit_0029A420) {
    const u32 o = arg<u32>(ctx, 0), on = arg<u32>(ctx, 1) & 0xFFu;
    if (on == 0) {
        const u32 e = load<u32>(o + 2276);
        if (e != 0) call(ctx, 0x004CF988u, {e, 0xDC2411A6u});
        store<u8>(o + 2336, static_cast<u8>((load<u8>(o + 2336) | 0x10u) & ~0x08u));
        ret(ctx, 0u);
        return;
    }
    store<u8>(o + 2336, static_cast<u8>((load<u8>(o + 2336) & ~0x18u) | 0x80u));
    u32 e = load<u32>(o + 2276);
    bool spawn = true;
    if (e != 0) {
        const u32 c = load<u32>(e + 4);
        const u32 k = c != 0 ? load<u32>(c + 8) : 0u;
        if (k == 0x9F6559A5u) {
            call(ctx, 0x004CF988u, {e, 0xD3380BFCu});
            e = load<u32>(o + 2276);
            spawn = false;
        } else {
            call(ctx, 0x004D0280u, {e});
            store<u32>(o + 2276, 0);
        }
    }
    if (spawn) {
        e = static_cast<u32>(call(ctx, 0x0025A8A8u, {o, load<u32>(o + 2264), 0}));
        store<u32>(o + 2276, e);
    }
    if (e == 0) return;
    const u32 c = static_cast<u32>(call(ctx, 0x004BB4B4u, {e, 0x04B1112Au}));
    if (c == 0) return;
    if (static_cast<s32>(VCall(ctx, c, 32, {c})) != 3) return;
    ctx.fpr[1] = Lfs42(o + 2272);  // f1 as the original leaves it
    store<u32>(c + 36, 0x3F800000u);
    store<u32>(c + 32, load<u32>(o + 2272));
    store<u32>(c + 40, load<u32>(o + 2272));
    ret(ctx, 0x3F800000u);
}

// ~T(): vtables 0x00B1CC10 / 0x00B1CC5C (+24), Registry_Unlink(o + 24,
// 0), an effect +40 gets bit 9 of +136 and is cleared,
// Registry_Unlink2(o + 24); then vtable 0x00B1BE30, o leaves the
// 0x01056F1C list and List1013B5C_Remove(o) (r3 = its).
DECOMP(002B17EC, Dtor_002B17EC) {
    const u32 o = arg<u32>(ctx, 0);
    store<u32>(o, 0x00B1CC10u);
    store<u32>(o + 24, 0x00B1CC5Cu);
    call(ctx, 0x0048F298u, {o + 24, 0});
    const u32 e = load<u32>(o + 40);
    if (e != 0) {
        store<u32>(e + 136, load<u32>(e + 136) | 0x200u);
        store<u32>(o + 40, 0);
    }
    call(ctx, 0x0048F6B0u, {o + 24});
    store<u32>(o, 0x00B1BE30u);
    Unlist1056F1C39(o);
    ret(ctx, call(ctx, 0x000D6940u, {o}));
}

// void f(T* o, a): walks the chain of 16-byte nodes o->+28 (u32 count
// o->+16 steps from node o->+8; next = u16 node +4, and node o->+12 ends
// at index 0): each node's +8 object gets obj->vt[24](obj, a). r3 as the
// original leaves it (the packed {index, (next node +12) >> 15} cursor).
DECOMP(002EF4E0, Chain_Visit_002EF4E0) {
    const u32 o = arg<u32>(ctx, 0), a = arg<u32>(ctx, 1);
    const u32 n = load<u32>(o + 16);
    u32 cur = 0;
    if (n != 0) {
        const u32 i = load<u32>(o + 8);
        const u32 w = load<u32>(load<u32>(o + 28) + (i << 4) + 12);
        cur = ((i & 0xFFFFu) << 16) | ((w >> 15) & 0xFFFFu);
    }
    ret(ctx, cur);
    if (n == 0) return;
    for (u32 j = 0; j < n;) {
        const u32 idx = cur >> 16;
        const u32 x = load<u32>(load<u32>(o + 28) + (idx << 4) + 8);
        VCall(ctx, x, 24, {x, a});
        if (idx == load<u32>(o + 12)) {
            cur = 0;
        } else {
            const u32 t = load<u32>(o + 28);
            const u32 nx = load<u32>(t + (idx << 4) + 4);
            const u32 w = load<u32>(t + (nx << 4) + 12);
            cur = (nx << 16) | ((w >> 15) & 0xFFFFu);
        }
        ++j;
        ret(ctx, cur);
    }
}

// void Set(T* o, u8 a, u8 second): a = 0 is View_SetState_0030C018(o, 0);
// otherwise the view o->+0 (second: o->+4) whose u8 +8 (+9) differs from a
// takes it (byte updated) and, when set, gets v->vt[88](v, a, 1, 1, 1)
// and v->vt[92](v, 0, -1). r3 as the original leaves it.
DECOMP(0030C14C, View_Set_0030C14C) {
    const u32 o = arg<u32>(ctx, 0), a = arg<u32>(ctx, 1) & 0xFFu, second = arg<u32>(ctx, 2) & 0xFFu;
    if (a == 0) {
        call(ctx, 0x0030C018u, {o, 0});
        return;
    }
    const u32 boff = second != 0 ? 8u : 9u, voff = second != 0 ? 0u : 4u;
    ret(ctx, a);
    if (a == load<u8>(o + boff)) return;
    const u32 v = load<u32>(o + voff);
    store<u8>(o + boff, static_cast<u8>(a));
    if (v == 0) return;
    VCall(ctx, v, 88, {v, a, 1, 1, 1});
    const u32 w = load<u32>(o + voff);
    VCall(ctx, w, 92, {w, 0, ~0ull});
}

// u8 f(T* o, Desc* d) of the loader states (u8 o->+18): 4 ticks
// (Loader_Tick3250A0(o, 0)) and acts on the message kind d->+0: 9 calls
// o->vt[88](o); 10 with a schedule o->+2544 ({?, f32 delay} pairs, count
// o->+2536, cursor +2540) runs the fade: the delay +2548 above
// *0x0106B2AC or not above *0x003252CC ramps +2552 by d->+24 / that
// constant up to 1.0 (when below *0x003252D0), else +2552 = delay /
// constant; a positive delay counts down by d->+24 and on running out
// moves to the next entry (wrapping), +2552 = 0 and +2548 = its delay. 5
// / 6 tick with 2 / 1. r3 = u8 o->+18; f1 as the original leaves it.
DECOMP(003252D8, Loader_Step_003252D8) {
    const u32 o = arg<u32>(ctx, 0), d = arg<u32>(ctx, 2);
    const s32 st = static_cast<s32>(load<u8>(o + 18));
    if (st > 4) {
        if (st == 5) call(ctx, 0x003250A0u, {o, 2});
        else if (st == 6) call(ctx, 0x003250A0u, {o, 1});
    } else if (st == 4) {
        call(ctx, 0x003250A0u, {o, 0});
        const s32 k = static_cast<s32>(load<u32>(d));
        if (k == 9) {
            VCall(ctx, o, 88, {o});
        } else if (k == 10) {
            const u32 arr = load<u32>(o + 2544);
            if (arr != 0) {
                f64 f1 = Lfs42(o + 2548);
                ctx.fpr[1] = f1;
                const f64 f5 = Lfs42(0x0106B2ACu), f2 = Lfs42(d + 24);
                if (f1 > f5 || f1 < Lfs42(0x003252CCu) || f1 == Lfs42(0x003252CCu)) {
                    f64 f4 = Lfs42(o + 2552);
                    const f64 f3 = Lfs42(0x003252D0u);
                    if (f4 < f3) {
                        f4 = F42(f4 + F42(f2 / f5));
                        store<u32>(o + 2552, W42(f4));
                        if (f4 > f3) store<u32>(o + 2552, 0x3F800000u);
                    }
                } else {
                    store<u32>(o + 2552, W42(f1 / f5));
                }
                const f64 f3 = Lfs42(0x003252CCu);
                if (f1 > f3) {
                    f1 = F42(f1 - f2);
                    ctx.fpr[1] = f1;
                    store<u32>(o + 2548, W42(f1));
                    if (f1 < f3 || f1 == f3) {
                        u32 n = load<u32>(o + 2540) + 1;
                        const u32 m = load<u32>(o + 2536);
                        store<u32>(o + 2540, n);
                        if (!(static_cast<s32>(n) < static_cast<s32>(m))) {
                            store<u32>(o + 2540, 0);
                            n = 0;
                        }
                        const u32 src = arr + (n << 3) + 4;
                        ctx.fpr[1] = Lfs42(src);
                        const u32 v = load<u32>(src);
                        store<u32>(o + 2552, 0);
                        store<u32>(o + 2548, v);
                    }
                }
            }
        }
    }
    ret(ctx, load<u8>(o + 18));
}

// u8 f(T* o, s32 i, Desc* d): player slot i's object p
// ((*0x00F65CA0)->+4216 + 0x20C80 * i, with bit 3 of +192): with the flag
// f = (no o->+2572 index s, or byte p->+656[s] with bit 7 and without bit
// 6), state 4 with f calls o->vt[104](o, 5, 0) and state 5 without f
// o->vt[104](o, 4, 0). Then Loader_Step_003252D8(o, i, d); r3 = its.
DECOMP(0032EE20, Loader_Player_0032EE20) {
    const u32 o = arg<u32>(ctx, 0), i = arg<u32>(ctx, 1), d = arg<u32>(ctx, 2);
    const u32 p = load<u32>(load<u32>(0x00F65CA0u) + i * 0x00020C80u + 4216);
    if (p != 0 && (load<u32>(p + 192) & 8u)) {
        const u32 s = load<u32>(o + 2572);
        u32 f = 1;
        if (s != 0) {
            const u32 b = load<u8>(p + 656 + static_cast<u32>(static_cast<s32>(s)));
            f = ((b & 0x80u) && !(b & 0x40u)) ? 1u : 0u;
        }
        const s32 st = static_cast<s32>(load<u8>(o + 18));
        if (st > 4) {
            if (st == 5 && f == 0) VCall(ctx, o, 104, {o, 4, 0});
        } else if (st > 3 && f != 0) {
            VCall(ctx, o, 104, {o, 5, 0});
        }
    }
    ret(ctx, call(ctx, 0x003252D8u, {o, i, d}));
}

// void f(T* o, f32 x): the band k of x in o->+96's thresholds +300..+312
// (first with x <= t[k], from k = -1; 3 above all); a new band 0 / 1 / 2
// (kept in o->+136) applies the hash 0x623497B4 / 0x37E2E82C / 0x68D46DA2
// (Effect_SetHash4CF988) to o->+104's vt[272]() effect, to (o->+132)->
// +32's vt[272]() effect and to the effect o->+32, where set. r3 as the
// original leaves it.
DECOMP(003B4D24, Obj_SetBand_003B4D24) {
    const u32 o = arg<u32>(ctx, 0);
    const f64 x = farg(ctx, 0);
    const u32 t = load<u32>(o + 96) + 304;
    s32 k = -1;
    for (;;) {
        const f64 v = Lfs42(t + static_cast<u32>(k * 4));
        ctx.fpr[2] = v;
        if (x < v || x == v) break;
        ++k;
        if (!(k < 3)) break;
    }
    if (k == -1) {
        ret(ctx, t);
        return;
    }
    const u32 cur = load<u32>(o + 136);
    ret(ctx, cur);
    if (k == static_cast<s32>(cur)) return;
    store<u32>(o + 136, static_cast<u32>(k));
    u32 h;
    if (k == 0) h = 0x623497B4u;
    else if (k == 1) h = 0x37E2E82Cu;
    else if (k == 2) h = 0x68D46DA2u;
    else return;
    const u32 e = load<u32>(o + 104);
    ret(ctx, e);
    if (e != 0) {
        const u32 r = static_cast<u32>(VCall(ctx, e, 272, {e}));
        if (r != 0) call(ctx, 0x004CF988u, {r, h});
    }
    const u32 q = load<u32>(o + 132);
    ret(ctx, q);
    if (q != 0) {
        const u32 z = load<u32>(q + 32);
        ret(ctx, z);
        if (z != 0) {
            const u32 r = static_cast<u32>(VCall(ctx, z, 272, {z}));
            if (r != 0) call(ctx, 0x004CF988u, {r, h});
        }
    }
    const u32 w = load<u32>(o + 32);
    if (w != 0) call(ctx, 0x004CF988u, {w, h});
}

// T* T(T* o, a, b, c, const Vec3* p, const Vec3* q, d, const Vec3* r,
// const Vec3* s, u8 f10, u8 f11, u8 f12, u8 f13): Obj_InitVt_B20CC8(o, 1),
// vtable 0x00B20CF8; vec3s p / q / r / s into +32 / +48 / +80 / +96, +16 =
// a, +24 = b, +28 = c, +64 = d, +12 = +20 = 0, u8 +112..+115 = f10..f13
// (the 9th..13th arguments come from the stack). r3 = the vtable; f1 as
// the original leaves it.
DECOMP(0042CCD4, Obj_Ctor_0042CCD4) {
    const u32 o = arg<u32>(ctx, 0), a = arg<u32>(ctx, 1), b = arg<u32>(ctx, 2), c = arg<u32>(ctx, 3), p = arg<u32>(ctx, 4),
              q = arg<u32>(ctx, 5), d = arg<u32>(ctx, 6), r = arg<u32>(ctx, 7);
    const u32 sp = static_cast<u32>(ctx.gpr[1]);
    const u32 s = load<u32>(sp + 116), f10 = load<u32>(sp + 124), f11 = load<u32>(sp + 132), f12 = load<u32>(sp + 140),
              f13 = load<u32>(sp + 148);
    // r5..r10 keep the arguments (r7 = the caller's sp + 224 in the original)
    call(ctx, 0x0042C9D0u, {o, 1});
    store<u32>(o + 32, load<u32>(p));
    store<u32>(o + 36, load<u32>(p + 4));
    store<u32>(o + 40, load<u32>(p + 8));
    store<u32>(o + 48, load<u32>(q));
    store<u32>(o + 52, load<u32>(q + 4));
    u32 t = load<u32>(q + 8);
    store<u32>(o, 0x00B20CF8u);
    store<u32>(o + 56, t);
    t = load<u32>(r);
    store<u32>(o + 12, 0);
    store<u32>(o + 80, t);
    t = load<u32>(r + 4);
    store<u32>(o + 16, a);
    store<u32>(o + 20, 0);
    store<u32>(o + 24, b);
    store<u32>(o + 84, t);
    store<u32>(o + 28, c);
    t = load<u32>(r + 8);
    store<u32>(o + 64, d);
    store<u32>(o + 88, t);
    store<u32>(o + 96, load<u32>(s));
    t = load<u32>(s + 4);
    store<u8>(o + 112, static_cast<u8>(f10));
    store<u32>(o + 100, t);
    t = load<u32>(s + 8);
    store<u8>(o + 113, static_cast<u8>(f11));
    store<u32>(o + 104, t);
    store<u8>(o + 114, static_cast<u8>(f12));
    store<u8>(o + 115, static_cast<u8>(f13));
    ctx.fpr[1] = static_cast<f64>(std::bit_cast<f32>(t));
    ret(ctx, 0x00B20CF8u);
}

// void Delete(T* o): unless null, RegistryUnlink40 (vtable 0x00B21A30; o
// leaves the 0x010BB738 node chains when u8 +4 is set) and
// Mem_FreeFrom(o, 0); r3 = its.
DECOMP(0048F548, RegNode_Delete_0048F548) {
    const u32 o = arg<u32>(ctx, 0);
    if (o == 0) return;
    RegistryUnlink40(ctx);
    ret(ctx, call(ctx, 0x0054EE6Cu, {o, 0}));
}

// void Remove(T* o, Obj* x): x is taken out of the first of the two u32
// arrays that holds it ({data +128, u16 count +136}, then {+148, +156};
// the last element moves into its place and the count drops; quirk: a
// count that reads 0 by then copies data[0xFFFF] to data[-1]); found, the
// body of (x->+24)->+8 goes to 0x00933120. r3 as the original leaves it.
DECOMP(004A95FC, Obj_RemoveRef_004A95FC) {
    const u32 o = arg<u32>(ctx, 0), x = arg<u32>(ctx, 1);
    u32 doff = 0, coff = 0, i = 0, n = 0, arr = 0;
    const u32 n1 = load<u16>(o + 136), a1 = load<u32>(o + 128);
    for (u32 k = 0; k < n1; ++k) {
        if (load<u32>(a1 + 4 * k) == x) {
            doff = 128, coff = 136, i = k, n = n1, arr = a1;
            break;
        }
    }
    if (doff == 0) {
        for (u32 k = 0; k < load<u16>(o + 156); ++k) {
            const u32 a2 = load<u32>(o + 148), n2 = load<u16>(o + 156);
            if (load<u32>(a2 + 4 * k) == x) {
                doff = 148, coff = 156, i = k, n = n2, arr = a2;
                break;
            }
        }
        if (doff == 0) return;
    }
    store<u32>(arr + 4 * i, load<u32>(arr + 4 * (n - 1)));
    const u32 c = load<u16>(o + coff);
    store<u16>(o + coff, static_cast<u16>(c - 1));
    if (c == 0) {
        const u32 d = load<u32>(o + doff);
        store<u32>(d - 4, load<u32>(d + 0x3FFFCu));
    }
    ret(ctx, call(ctx, 0x00933120u, {load<u32>(load<u32>(x + 24) + 8), x}));  // r4 = x, read by the callee
}

// void Pick(T* o, Table* t, Out* out, x, s32 idx): *out = 1; the entry is
// the table row t->+12 + 76 * j (j = (s16)(u16 o->+68 + idx), within u16
// (t->+4)->+2) for 0 <= idx < o->+60, else Table_PickWeighted(o, t)
// (quirk: a j out of range uses the address 4); its words +4..+16 (zeros
// for a null entry) go to out->+20..+32; u8 out->+41 / +42 = bits 6 / 2
// of u8 o->+20, out->+44 / +52 = o->+28 / +32, u8 out->+40 = u8 o->+21;
// unless out->+24 is 0x0024A738, 0x004B36A0(o, t, out, x) (r3 = its; else
// u8 o->+21). f1 as the original leaves it.
DECOMP(004B453C, Table_Pick_004B453C) {
    const u32 o = arg<u32>(ctx, 0), t = arg<u32>(ctx, 1), out = arg<u32>(ctx, 2), x = arg<u32>(ctx, 3);
    const s32 idx = static_cast<s32>(arg<u32>(ctx, 4));
    store<u32>(out, 1);
    u32 e;
    if (idx < 0 || !(idx < static_cast<s32>(load<u32>(o + 60)))) {
        e = static_cast<u32>(call(ctx, 0x004B43E8u, {o, t}));
    } else {
        const s32 j = static_cast<s16>(static_cast<u16>(load<u16>(o + 68) + static_cast<u32>(idx)));
        u32 base = 0;
        if (j >= 0 && j < static_cast<s32>(load<u16>(load<u32>(t + 4) + 2))) base = load<u32>(t + 12) + 76u * static_cast<u32>(j);
        e = base + 4;
    }
    u32 w[4] = {0, 0, 0, 0};
    if (e != 0)
        for (u32 k = 0; k < 4; ++k) w[k] = load<u32>(e + 4 + 4 * k);
    for (u32 k = 0; k < 4; ++k) store<u32>(out + 20 + 4 * k, w[k]);
    store<u8>(out + 41, (load<u8>(o + 20) & 0x40u) ? 1 : 0);
    const u32 b = load<u8>(o + 20);
    store<u32>(out + 44, load<u32>(o + 28));
    ctx.fpr[1] = Lfs42(o + 32);  // f1 as the original leaves it
    store<u32>(out + 52, load<u32>(o + 32));
    store<u8>(out + 42, (b & 0x04u) ? 1 : 0);
    const u32 b21 = load<u8>(o + 21);
    store<u8>(out + 40, static_cast<u8>(b21));
    if (w[1] == 0x0024A738u) {
        ret(ctx, b21);
        return;
    }
    ret(ctx, call(ctx, 0x004B36A0u, {o, t, out, x}));
}

// ~T() body: vtable 0x00B23020; the object +8 gets vt[4]() and is
// cleared; the o->+72 nodes of o->+76 (16 bytes: prev = i - 1, next = i +
// 1, bit 7 of u8 +12 set, bits 15..30 of +12 cleared) are relinked, the
// first prev and the last next = -1 (quirk: written even for a null
// array), +56 = +60 = -1, +48 = 0, +52 = count - 1 (-1 for fewer than
// two), +64 = 0, +68 = count; then the arrays +76 and +40 are freed
// (Mem_FreeFrom(p, 0)) and cleared. r3 as the original leaves it.
DECOMP(004F6F54, Pool_Reset_004F6F54) {
    const u32 o = arg<u32>(ctx, 0);
    store<u32>(o, 0x00B23020u);
    const u32 x = load<u32>(o + 8);
    if (x != 0) {
        VCall(ctx, x, 4, {x});
        store<u32>(o + 8, 0);
    }
    s32 n = static_cast<s32>(load<u32>(o + 72));
    if (n > 0) {
        for (s32 k = 0; k < n; n = static_cast<s32>(load<u32>(o + 72))) {
            const u32 off = 16u * static_cast<u32>(k);
            const u32 arr = load<u32>(o + 76);
            store<u32>(arr + off, static_cast<u32>(k - 1));
            store<u32>(arr + off + 4, static_cast<u32>(k + 1));
            store<u8>(arr + off + 12, static_cast<u8>(load<u8>(arr + off + 12) | 0x80u));
            const u32 a2 = load<u32>(o + 76) + off + 12;
            store<u32>(a2, load<u32>(a2) & ~0x7FFF8000u);
            ++k;
        }
    }
    const u32 a = load<u32>(o + 76);
    store<u32>(a, ~0u);
    store<u32>(a + (static_cast<u32>(n) << 4) - 12, ~0u);
    store<u32>(o + 56, ~0u);
    store<u32>(o + 60, ~0u);
    store<u32>(o + 48, 0);
    store<u32>(o + 52, n > 1 ? static_cast<u32>(n - 1) : ~0u);
    store<u32>(o + 64, 0);
    store<u32>(o + 68, static_cast<u32>(n));
    ret(ctx, static_cast<u32>(n));
    if (a != 0) {
        call(ctx, 0x0054EE6Cu, {a, 0});
        store<u32>(o + 76, 0);
    }
    const u32 p = load<u32>(o + 40);
    ret(ctx, p);
    if (p != 0) {
        call(ctx, 0x0054EE6Cu, {p, 0});
        store<u32>(o + 40, 0);
    }
}

// u32 Push(Vector* v, const Rec* r): the 128-byte record r (words +0,
// +80..+100, +108, +112, +120, f32 +16..+56, +64..+72, +116, bit 7 of u8
// +104) is appended to v (data +0, u16 capacity +4, count +8, high-water
// +10; doubled with PtrVector_Reserve when full). r3 = the new count - 1;
// f1 as the original leaves it.
DECOMP(0077AF70, Vec_PushRec128_0077AF70) {
    const u32 v = arg<u32>(ctx, 0), r = arg<u32>(ctx, 1);
    u32 n = load<u16>(v + 8);
    const u32 cap = load<u16>(v + 4);
    if (n == cap) {
        call(ctx, 0x00551718u, {v, (cap << 1) & 0x1FFFEu});
        n = load<u16>(v + 8);
    }
    const u32 e = load<u32>(v) + ((n << 7) & 0x007FFF80u);
    store<u32>(e, load<u32>(r));
    for (u32 k = 16; k <= 56; k += 4) store<u32>(e + k, load<u32>(r + k));
    for (u32 k = 64; k <= 72; k += 4) store<u32>(e + k, load<u32>(r + k));
    for (u32 k = 80; k <= 92; k += 4) store<u32>(e + k, load<u32>(r + k));
    const u32 w96 = load<u32>(r + 96);
    const u32 b = load<u8>(e + 104);
    store<u32>(e + 96, w96);
    store<u32>(e + 100, load<u32>(r + 100));
    store<u8>(e + 104, static_cast<u8>((b & ~0x80u) | (load<u8>(r + 104) & 0x80u)));
    store<u32>(e + 108, load<u32>(r + 108));
    store<u32>(e + 112, load<u32>(r + 112));
    ctx.fpr[1] = Lfs42(r + 116);  // f1 as the original leaves it
    store<u32>(e + 116, load<u32>(r + 116));
    store<u32>(e + 120, load<u32>(r + 120));
    const u32 n1 = load<u16>(v + 8) + 1;
    const u32 hw = load<u16>(v + 10);
    store<u16>(v + 8, static_cast<u16>(n1));
    store<u16>(v + 10, static_cast<u16>(static_cast<s32>(hw) > static_cast<s32>(n1 & 0xFFFFu) ? hw : n1));
    ret(ctx, (n1 & 0xFFFFu) - 1);
}

// bool Lookup(Node* n, u16 a, u32 b, Out* out): from n up through the
// parents (+0) until one has a table t = +16: the key (a << 16) | (b >>
// 16) is looked up (0x0078987C(t, &key) -> i; t's default entry t + 16
// for i < 0, else the 28-byte row t->+8 + 28 * i, from its +4) and its
// 24 bytes copied to out (words +0 / +4 / +16 / +20, f32 +8, u8 +12); an
// entry whose out->+8 is *0x00789DF8 (the 'unset' value) continues at the
// parent. r3 = 1 when found set (0 without a table or at the root); f1 as
// the original leaves it.
DECOMP(00789E00, Node_LookupKey_00789E00) {
    const u32 n = arg<u32>(ctx, 0), a = arg<u32>(ctx, 1), b = arg<u32>(ctx, 2), out = arg<u32>(ctx, 3);
    const u32 t = load<u32>(n + 16);
    if (t == 0) {
        const u32 up = load<u32>(n);
        if (up == 0) {
            ret(ctx, 0u);
            return;
        }
        ret(ctx, call(ctx, 0x00789E00u, {up, a, b, out}));
        return;
    }
    Stack fr(ctx, 16);  // the key at sp+112
    store<u32>(fr.addr(), ((b >> 16) & 0xFFFFu) | (a << 16));
    const s32 i = static_cast<s32>(call(ctx, 0x0078987Cu, {t, fr.addr()}));
    const u32 parent = load<u32>(n);
    const u32 p = i < 0 ? t + 16 : load<u32>(t + 8) + 28u * static_cast<u32>(i) + 4;
    const u32 w0 = load<u32>(p), f8 = load<u32>(p + 8);
    store<u32>(out, w0);
    const u32 w4 = load<u32>(p + 4);
    store<u32>(out + 8, f8);
    store<u32>(out + 4, w4);
    store<u8>(out + 12, load<u8>(p + 12));
    const u32 w16 = load<u32>(p + 16), w20 = load<u32>(p + 20);
    store<u32>(out + 16, w16);
    store<u32>(out + 20, w20);
    const f64 k = Lfs42(0x00789DF8u);
    ctx.fpr[1] = k;
    const bool unset = Lfs42(out + 8) == k;
    if (unset && parent != 0) {
        ret(ctx, call(ctx, 0x00789E00u, {parent, a, b, out}));
        return;
    }
    ret(ctx, unset ? 0u : 1u);
}

// T* New(?, XmlCursor* x): a 20-byte object (Mem_Alloc(20, 0, 16); r3 = 0
// on failure) made by Obj_InitVt_B21C98, vtable 0x00B11738, u8 +12..+17
// = 0; read: bools 0xF19B0C5A (default 0), 0xFA4A62D8, 0xCE13953D,
// 0x00814A44, 0xFE2AAC3C and 0x6C06B3AF (defaults the current values)
// into u8 +12..+17. r3 = p.
DECOMP(000A3BA8, Act_NewXml_000A3BA8) {
    const u32 x = arg<u32>(ctx, 1);
    const u32 p = static_cast<u32>(call(ctx, 0x0054ED38u, {20, 0, 16}));
    if (p == 0) {
        ret(ctx, 0u);
        return;
    }
    call(ctx, 0x004BB3B0u, {p, x});
    store<u8>(p + 12, 0);
    store<u8>(p + 13, 0);
    store<u8>(p + 14, 0);
    store<u32>(p, 0x00B11738u);
    store<u8>(p + 15, 0);
    store<u8>(p + 16, 0);
    store<u8>(p + 17, 0);
    call(ctx, 0x004F986Cu, {x, 0xF19B0C5Au, p + 12, 0});
    static constexpr u32 kKeys[] = {0xFA4A62D8u, 0xCE13953Du, 0x00814A44u, 0xFE2AAC3Cu, 0x6C06B3AFu};
    for (u32 k = 0; k < 5; ++k) call(ctx, 0x004F986Cu, {x, kKeys[k], p + 13 + k, load<u8>(p + 13 + k)});
    ret(ctx, p);
}

// ---------------------------------------------------------------- sweep: 91 instructions
// Node* Pick(Node* o): from o's +14 link (16-bit index into the 20-byte
// nodes of *0x00F557F4, +12 data, u16 count (+4)->+2; out of range = 0)
// it steps r times along the +10 links (stopping at o itself), r =
// state % (node +16 + 1) of the random state 0x01164890, which is stepped
// (0x01164888 = the low 16 bits of the mix). A leaf; quirk: a null node
// is read at address 0.
DECOMP(000155E0, Node_PickRandom_000155E0) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 g = load<u32>(0x00F557F4u);
    auto node = [&](s32 j) -> u32 {
        if (j < 0 || !(j < static_cast<s32>(load<u16>(load<u32>(g + 4) + 2)))) return 0u;
        return load<u32>(g + 12) + 20u * static_cast<u32>(j);
    };
    u32 cur = node(static_cast<s16>(load<u16>(o + 14)));
    const u32 c = load<u32>(cur + 16);
    const u32 s = load<u32>(0x01164890u);
    const u32 d = c + 1;
    const u32 q = d != 0 ? s / d : 0u;
    const s32 r = static_cast<s32>(s - (q * c + q));
    const u32 mix = ((s << 4 | s >> 28) + (s << 16 | s >> 16)) ^ 0x87654321u;
    store<u32>(0x01164890u, mix << 16 | mix >> 16);
    store<u64>(0x01164888u, mix & 0xFFFFu);
    for (s32 k = 0; k < r; ++k) {
        if (cur == o) break;
        cur = node(static_cast<s16>(load<u16>(cur + 10)));
    }
    ret(ctx, cur);
}

// bool f(T* o): with bit 6 of u8 o->+306, the line of sight from o's
// position (o->+48's vt[80] position, moved by the offset of the second
// sample from o->+176) to the target o->+256's vt[80] position
// (Follow_LineOfSight138B0(o, &from, &to, t)) sets bit 7 (cleared without
// a target) and bit 6 is cleared. r3 = bit 7 of +306.
DECOMP(000164D4, Obj_SightCheck_000164D4) {
    const u32 o = arg<u32>(ctx, 0);
    u32 b = load<u8>(o + 306);
    if (b & 0x40u) {
        Stack fr(ctx, 64);  // sp+112 / +128 / +144 / +160
        const u32 v1 = fr.addr(), v2 = v1 + 16, s = v1 + 32, v3 = v1 + 48;
        u32 a = load<u32>(o + 48);
        VCall(ctx, a, 80, {v1, a});
        const u32 t = load<u32>(o + 256);
        u32 r = 0;
        if (t != 0) {
            a = load<u32>(o + 48);
            VCall(ctx, a, 80, {v2, a});
            const f64 dx = F42(Lfs42(v2) - Lfs42(o + 176)), dy = F42(Lfs42(v2 + 4) - Lfs42(o + 180)),
                      dz = F42(Lfs42(v2 + 8) - Lfs42(o + 184));
            const f64 sx = F42(Lfs42(v1) + dx), sy = F42(Lfs42(v1 + 4) + dy), sz = F42(Lfs42(v1 + 8) + dz);
            store<u32>(s, W42(sx));
            store<u32>(s + 4, W42(sy));
            store<u32>(s + 8, W42(sz));
            ctx.fpr[1] = sx;  // f1 as the original leaves it
            VCall(ctx, t, 80, {v3, t});
            r = static_cast<u32>(call(ctx, 0x000138B0u, {o, s, v3, t}));
        }
        b = (load<u8>(o + 306) & ~0x80u) | ((r & 1u) << 7);
        b &= ~0x40u;
        store<u8>(o + 306, static_cast<u8>(b));
    }
    ret(ctx, (b & 0x80u) ? 1u : 0u);
}

// void Apply(T* o, Ctx* x): with *0x00F64C20's +40 player record g, the
// component o->+20 of x of kind 12 (vt[32]) gets +20 = g->+468; with
// o->+12 and o->+16 set, the component o->+16 of kind 12 gets +20 = the
// table word 0x00AC4D74[i], i = SlotRec_Byte464(Name_Resolve(*0x00F64C20,
// o->+12)) (0 when unresolved). r3 as the original leaves it.
DECOMP(0007B5E8, Action_SetSlot_0007B5E8) {
    const u32 o = arg<u32>(ctx, 0), x = arg<u32>(ctx, 1);
    if (load<u32>(o + 16) == 0 && load<u32>(o + 20) == 0) {
        ret(ctx, 0u);
        return;
    }
    const u32 c20 = load<u32>(o + 20);
    if (c20 != 0) {
        const u32 g = load<u32>(load<u32>(0x00F64C20u) + 40);
        if (g != 0) {
            const u32 comp = static_cast<u32>(call(ctx, 0x004BB4B4u, {x, c20}));
            if (comp != 0 && static_cast<s32>(VCall(ctx, comp, 32, {comp})) == 12) store<u32>(comp + 20, load<u32>(g + 468));
        }
    }
    const u32 n = load<u32>(o + 12);
    ret(ctx, n);
    if (n == 0 || load<u32>(o + 16) == 0) return;
    u32 i = 0;
    const u32 r = static_cast<u32>(call(ctx, 0x002E1220u, {load<u32>(0x00F64C20u), n}));
    if (r != 0) i = static_cast<u32>(call(ctx, 0x002D98A4u, {r}));
    const u32 comp = static_cast<u32>(call(ctx, 0x004BB4B4u, {x, load<u32>(o + 16)}));
    if (comp == 0) return;
    if (static_cast<s32>(VCall(ctx, comp, 32, {comp})) != 12) return;
    const u32 v = load<u32>(0x00AC4D74u + (i << 2));
    store<u32>(comp + 20, v);
    ret(ctx, v);
}

// T* New(?, XmlCursor* x): a 24-byte object (Mem_Alloc(24, 0, 16); r3 = 0
// on failure) made by Obj_InitVt_B21C98, vtable 0x00B10878, +12 = +16 =
// -1.0, bits 30 / 31 of +20 cleared; read (float default *0x0008FF18):
// 0xB7EA3D94 into a temporary (-1.0) whose 0 result copies it to +12 /
// +16 and sets bits 30 / 31; 0x80244A9A into +12 (a 0 result sets bit
// 31); 0x1867714F into +16 (a 0 result makes it 1.0). r3 = p.
DECOMP(0008FF20, Act_NewXml_0008FF20) {
    const u32 x = arg<u32>(ctx, 1);
    const u32 p = static_cast<u32>(call(ctx, 0x0054ED38u, {24, 0, 16}));
    if (p == 0) {
        ret(ctx, 0u);
        return;
    }
    call(ctx, 0x004BB3B0u, {p, x});
    const u32 w = load<u32>(p + 20) & 0x3FFFFFFFu;
    store<u32>(p, 0x00B10878u);
    store<u32>(p + 12, 0xBF800000u);
    store<u32>(p + 16, 0xBF800000u);
    store<u32>(p + 20, w);
    Stack fr(ctx, 16);  // the temporary at sp+112
    const u32 tmp = fr.addr();
    store<u32>(tmp, 0xBF800000u);
    const f64 k = Lfs42(0x0008FF18u);
    callf(ctx, 0x004F9B20u, {x, 0xB7EA3D94u, tmp}, {k});
    if (static_cast<u32>(ctx.gpr[3]) == 0) {
        store<u32>(p + 20, load<u32>(p + 20) | 0xC0000000u);
        const u32 t = load<u32>(tmp);
        store<u32>(p + 16, t);
        store<u32>(p + 12, t);
    }
    callf(ctx, 0x004F9B20u, {x, 0x80244A9Au, p + 12}, {k});
    if (static_cast<u32>(ctx.gpr[3]) == 0) store<u32>(p + 20, load<u32>(p + 20) | 0x80000000u);
    callf(ctx, 0x004F9B20u, {x, 0x1867714Fu, p + 16}, {k});
    if (static_cast<u32>(ctx.gpr[3]) == 0) store<u32>(p + 16, 0x3F800000u);
    ret(ctx, p);
}

// T* New(?, XmlCursor* x): a 28-byte object (Mem_Alloc(28, 0, 16); r3 = 0
// on failure) made by Obj_InitVt_B21C98, vtable 0x00B108A0, +12 = +16 =
// 0, u8 +20..+24 = 0; read: hashes 0x7A30D7E3 (default 0) / 0x7F247172
// into +12 / +16, bools 0x76DAA94D / 0x903B1E30 / 0xBE647980 into u8 +20
// / +22 / +24 (a result other than 1 sets u8 +21 / +23). r3 = p.
DECOMP(0009018C, Act_NewXml_0009018C) {
    const u32 x = arg<u32>(ctx, 1);
    const u32 p = static_cast<u32>(call(ctx, 0x0054ED38u, {28, 0, 16}));
    if (p == 0) {
        ret(ctx, 0u);
        return;
    }
    call(ctx, 0x004BB3B0u, {p, x});
    store<u32>(p + 12, 0);
    store<u32>(p + 16, 0);
    store<u8>(p + 20, 0);
    store<u8>(p + 21, 0);
    store<u32>(p, 0x00B108A0u);
    store<u8>(p + 22, 0);
    store<u8>(p + 23, 0);
    store<u8>(p + 24, 0);
    call(ctx, 0x004FA14Cu, {x, 0x7A30D7E3u, p + 12, 0});
    call(ctx, 0x004FA14Cu, {x, 0x7F247172u, p + 16, load<u32>(p + 16)});
    s32 r = static_cast<s32>(call(ctx, 0x004F986Cu, {x, 0x76DAA94Du, p + 20, load<u8>(p + 20)}));
    u32 d = load<u8>(p + 22);
    if (r != 1) store<u8>(p + 21, 1);
    r = static_cast<s32>(call(ctx, 0x004F986Cu, {x, 0x903B1E30u, p + 22, d}));
    d = load<u8>(p + 24);
    if (r != 1) store<u8>(p + 23, 1);
    call(ctx, 0x004F986Cu, {x, 0xBE647980u, p + 24, d});
    ret(ctx, p);
}

// T* New(?, XmlCursor* x): a 28-byte object (Mem_Alloc(28, 0, 16); r3 = 0
// on failure) made by Obj_InitVt_B21C98, vtable 0x00B115D0, colours +12 =
// 0 / +16 = 0x000000FF, +20 = 1.0, u8 +24 = 1, u8 +25 = 0; read: colours
// 0xBF5BC441 / 0x95B5E768 into +12 / +16, f32 0xB7EA3D94 into +20, bools
// 0x8488565B / 0x0AA56D8B into u8 +24 / +25 (defaults the current values).
// r3 = p.
DECOMP(000A2284, Act_NewXml_000A2284) {
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
    store<u32>(p, 0x00B115D0u);
    for (u32 k = 15; k < 19; ++k) store<u8>(p + k, 0);
    store<u8>(p + 19, 255);
    store<u32>(p + 20, 0x3F800000u);
    store<u8>(p + 24, 1);
    store<u8>(p + 25, 0);
    // r7 = 1 and r8 = the key are left over from the stores in the original
    call(ctx, 0x004F9F9Cu, {x, 0xBF5BC441u, p + 12, p + 12, 1, 0xBF5BC441u});
    call(ctx, 0x004F9F9Cu, {x, 0x95B5E768u, p + 16, p + 16});
    callf(ctx, 0x004F9B20u, {x, 0xB7EA3D94u, p + 20}, {Lfs42(p + 20)});
    call(ctx, 0x004F986Cu, {x, 0x8488565Bu, p + 24, load<u8>(p + 24)});
    call(ctx, 0x004F986Cu, {x, 0x0AA56D8Bu, p + 25, load<u8>(p + 25)});
    ret(ctx, p);
}

// void f(T* o): the o->vt[68]() and o->vt[64]() objects get vt[44](it, 0);
// a peer of the world object o->+160 with +136 set gets bit 5 of u8 +28
// of its Peer_GetVec record; the effect o->+356 gets
// Effect_SetHash4CF988(e, 0xE69B8987) and bit 8 of +136 cleared; bit 14
// of u16 (o->+164)->+40 is cleared. r3 as the original leaves it.
DECOMP(000E1420, Obj_Release_000E1420) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 a = static_cast<u32>(VCall(ctx, o, 68, {o}));
    if (a != 0) VCall(ctx, a, 44, {a, 0});
    const u32 b = static_cast<u32>(VCall(ctx, o, 64, {o}));
    if (b != 0) VCall(ctx, b, 44, {b, 0});
    if (load<u32>(o + 160) != 0 && call(ctx, 0x0057A428u, {load<u32>(o + 160)}) != 0 &&
        load<u32>(static_cast<u32>(call(ctx, 0x0057A428u, {load<u32>(o + 160)})) + 136) != 0) {
        const u32 peer = static_cast<u32>(call(ctx, 0x0057A428u, {load<u32>(o + 160)}));
        const u32 v = static_cast<u32>(call(ctx, 0x0057BE98u, {peer}));
        store<u8>(v + 28, static_cast<u8>(load<u8>(v + 28) | 0x20u));
    }
    u32 r = 0;
    if (load<u32>(o + 356) != 0) {
        call(ctx, 0x004CF988u, {load<u32>(o + 356), 0xE69B8987u});
        r = load<u32>(o + 356);
        store<u32>(r + 136, load<u32>(r + 136) & ~0x100u);
    }
    const u32 q = load<u32>(o + 164);
    if (q != 0) {
        store<u16>(q + 40, static_cast<u16>(load<u16>(q + 40) & ~0x4000u));
        r = 0;
    }
    ret(ctx, r);
}

// T* New(a, b, Src* s, c): the 168-byte definition on the stack (+4 =
// 0x1000, +12..+20 = s->+12..+20, u8 +25 = 1, -1.0 at +36 / +40 / +72 /
// +128..+148, the vec3 *0x01164B30 at +112, +160 = -1, u8 +164 = 6, zeros
// elsewhere; +108 / +124 unwritten) is read by 0x001BAFFC(a, b, s, c,
// &def) and handed to a 272-byte object (SmallAlloc(272); 0x001B9B70(p,
// &def)). r3 = p or 0.
DECOMP(001BB6C4, Def_New_001BB6C4) {
    const u32 a = arg<u32>(ctx, 0), b = arg<u32>(ctx, 1), s = arg<u32>(ctx, 2), c = arg<u32>(ctx, 3);
    Stack fr(ctx, 176);  // the definition at sp+112 .. sp+279
    const u32 t = fr.addr();
    store<u32>(t + 12, load<u32>(s + 12));
    store<u32>(t + 16, load<u32>(s + 16));
    store<u32>(t + 20, load<u32>(s + 20));
    for (u32 k = 24; k < 32; ++k) store<u8>(t + k, 0);
    store<u32>(t + 32, 0);
    store<u32>(t + 36, 0xBF800000u);
    store<u32>(t + 40, 0xBF800000u);
    store<u8>(t + 44, 0);
    for (u32 k = 48; k < 72; k += 4) store<u32>(t + k, 0);
    store<u32>(t, 0);
    store<u32>(t + 4, 0);
    store<u32>(t + 8, 0);
    store<u32>(t + 72, 0xBF800000u);
    for (u32 k = 0; k < 12; k += 4) store<u32>(t + 112 + k, load<u32>(0x01164B30u + k));
    ctx.fpr[1] = Lfs42(0x01164B38u);  // f1 / f2 as the original leaves them
    ctx.fpr[2] = Lfs42(0x01164B34u);
    for (u32 k = 128; k < 152; k += 4) store<u32>(t + k, 0xBF800000u);
    store<u32>(t + 152, 0);
    store<u32>(t + 156, 0);
    store<u32>(t + 160, ~0u);
    store<u8>(t + 164, 6);
    store<u8>(t + 165, 0);
    store<u8>(t + 166, 0);
    store<u32>(t + 4, 0x1000u);
    for (u32 k = 76; k < 108; k += 4) store<u32>(t + k, 0);
    store<u8>(t + 25, 1);
    // r8..r11 = -1, 1, 6, 0x1000 are left over from the stores in the original
    call(ctx, 0x001BAFFCu, {a, b, s, c, t, ~0ull, 1, 6});
    const u32 p = static_cast<u32>(call(ctx, 0x0055238Cu, {272}));
    if (p == 0) {
        ret(ctx, 0u);
        return;
    }
    call(ctx, 0x001B9B70u, {p, t});
    ret(ctx, p);
}

// bool SetCamTarget(T* o, Obj* c): o->+428 = c; with c, a 224-byte
// follower (SmallAlloc(224); 0x00111FE0(p, cam, c, o); 0 on failure)
// becomes o->+424 and is pushed on the camera cam = *(*0x00F65CA0 +
// 0x1281E0) (Camera_PushTarget0FC040(cam, p, -, 0; f1 = *0x001EAD14)), c
// gets c->vt[88](c, 0, 1, 1, 1), and o->+440 w gets w->vt[40](w, 1, 0)
// and, when its kind (bits 10..15 of +4) is 8, w->vt[52](w, 0, 0). r3 = 1.
DECOMP(001EAD18, Obj_SetCamTarget_001EAD18) {
    const u32 o = arg<u32>(ctx, 0), c = arg<u32>(ctx, 1);
    store<u32>(o + 428, c);
    if (c != 0) {
        const u32 cam = load<u32>(load<u32>(0x00F65CA0u) + 0x001281E0u);
        u32 p = static_cast<u32>(call(ctx, 0x0055238Cu, {224}));
        if (p != 0) {
            call(ctx, 0x00111FE0u, {p, cam, load<u32>(o + 428), o});
        }
        store<u32>(o + 424, p);
        // the f32 takes r5's slot, which holds the constant's address
        callf(ctx, 0x000FC040u, {cam, p, 0x001EAD14u, 0}, {Lfs42(0x001EAD14u)});
        const u32 cc = load<u32>(o + 428);
        VCall(ctx, cc, 88, {cc, 0, 1, 1, 1});
        u32 w = load<u32>(o + 440);
        if (w != 0) {
            VCall(ctx, w, 40, {w, 1, 0});
            w = load<u32>(o + 440);
            if (((load<u32>(w + 4) >> 10) & 0x3Fu) == 8) VCall(ctx, w, 52, {w, 0, 0});
        }
    }
    ret(ctx, 1u);
}

// T* New(a, b, Src* s, c): the 85-byte definition on the stack (+0 =
// 0x00400000, +4 = 8, +12 = s->+12, +16 / +20 = 0x00AD366C, -1.0 at +36 /
// +40 / +72, +76 = name 0x00AD36D0, u8 +80 = 1, zeros elsewhere) is read
// by TriggerAlertDef_Read2124D0(a, b, s, c, &def) and copied into a
// 192-byte object (SmallAlloc(192); r3 = 0 on failure): Thunk_000C2B90(p,
// &def), u8 +172..+175 = def +80..+83, vtable 0x00B19510,
// String_CopyN(p + 140, def +76, 31), u8 +176 = def +84, vtable
// 0x00B19B28. r3 = p.
DECOMP(002126B4, TriggerAlert_New_002126B4) {
    const u32 a = arg<u32>(ctx, 0), b = arg<u32>(ctx, 1), s = arg<u32>(ctx, 2), c = arg<u32>(ctx, 3);
    Stack fr(ctx, 96);  // the definition at sp+112 .. sp+196
    const u32 t = fr.addr();
    store<u32>(t + 16, 0x00AD366Cu);
    store<u32>(t + 12, load<u32>(s + 12));
    store<u32>(t + 20, 0x00AD366Cu);
    for (u32 k = 24; k < 32; ++k) store<u8>(t + k, 0);
    store<u32>(t + 32, 0);
    store<u32>(t + 36, 0xBF800000u);
    store<u32>(t + 40, 0xBF800000u);
    store<u8>(t + 44, 0);
    for (u32 k = 48; k < 72; k += 4) store<u32>(t + k, 0);
    store<u32>(t + 72, 0xBF800000u);
    store<u32>(t + 8, 0);
    store<u32>(t + 76, 0x00AD36D0u);
    store<u8>(t + 80, 1);
    store<u8>(t + 81, 0);
    store<u8>(t + 82, 0);
    store<u8>(t + 83, 0);
    store<u32>(t, 0x00400000u);
    store<u8>(t + 84, 0);
    store<u32>(t + 4, 8);
    call(ctx, 0x002124D0u, {a, b, s, c, t});
    const u32 p = static_cast<u32>(call(ctx, 0x0055238Cu, {192}));
    if (p == 0) {
        ret(ctx, 0u);
        return;
    }
    call(ctx, 0x000C2B90u, {p, t});
    store<u8>(p + 172, load<u8>(t + 80));
    store<u8>(p + 173, load<u8>(t + 81));
    store<u32>(p, 0x00B19510u);
    const u8 b82 = load<u8>(t + 82);
    const u32 str = load<u32>(t + 76);
    store<u8>(p + 174, b82);
    store<u8>(p + 175, load<u8>(t + 83));
    call(ctx, 0x00553638u, {p + 140, str, 31});
    store<u8>(p + 176, load<u8>(t + 84));
    store<u32>(p, 0x00B19B28u);
    ret(ctx, p);
}

// ~T(): vtable 0x00B1B180 and the PtrVector +432 destroyed; then the
// volume-shape body (vtable 0x00B19558, Volume_Notify(*0x00F557EC, o +
// 424), Shape_Release(o + 424), the arrays +388 / +364 and the PtrVector
// +344, the base Thunk_000D729C; r3 = its).
DECOMP(00225600, VolumeShape_Dtor_00225600) {
    const u32 o = arg<u32>(ctx, 0);
    store<u32>(o, 0x00B1B180u);
    VecDtor(ctx, o + 432);
    store<u32>(o, 0x00B19558u);
    call(ctx, 0x000112D8u, {load<u32>(0x00F557ECu), o + 424});
    call(ctx, 0x00638498u, {o + 424});
    ArrayDtor42(ctx, o + 388);
    ArrayDtor42(ctx, o + 364);
    VecDtor(ctx, o + 344);
    call(ctx, 0x000D729Cu, {o});
}
