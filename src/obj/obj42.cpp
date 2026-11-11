// Game code, part 42: the small-function sweep continued from the remaining
// 89 instruction functions (size order; obj41*.cpp did 86-89). Each was
// checked against the original in the PPC simulator (memory writes, calls
// and their arguments, r3 and f1).
#include "obj42.h"

using namespace sly4;
using namespace sly4::obj42;

// ---------------------------------------------------------------- sweep: 89 instructions
// void f(T* o, u8 all): with all set, each of the six parts {+556 skeleton,
// +560 body} (16 bytes apart) is dropped: a body with bit 18 of +172 gets
// body->vt[40](body, 1), the body slot is cleared, the skeleton's +8 goes
// to Body_Remove931F00 and the skeleton to AnimSkel_Release7D4EA8, then
// cleared; +536 = 0 and each of the seven emitters {+428, +432} is stopped
// (Emitter_Stop(e, 1)) and both words cleared. Then Obj_SoundOffBase(o,
// all); r3 = its.
DECOMP(001C98C4, Obj_DropParts_001C98C4) {
    const u32 o = arg<u32>(ctx, 0), all = arg<u32>(ctx, 1) & 0xFFu;
    if (all != 0) {
        for (u32 i = 0; i < 6; ++i) {
            const u32 q = o + 16 * i;
            const u32 b = load<u32>(q + 560);
            if (b != 0) {
                if (load<u32>(b + 172) & 0x40000u) VCall(ctx, b, 40, {b, 1});
                store<u32>(q + 560, 0);
                const u32 sk = load<u32>(q + 556);
                call(ctx, 0x00931F00u, {load<u32>(sk + 8), sk});
                call(ctx, 0x007D4EA8u, {load<u32>(q + 556)});
                store<u32>(q + 556, 0);
            }
        }
        store<u32>(o + 536, 0);
        for (u32 i = 0; i < 7; ++i) {
            const u32 q = o + 16 * i;
            call(ctx, 0x00443424u, {load<u32>(q + 428), 1});
            store<u32>(q + 428, 0);
            store<u32>(q + 432, 0);
        }
    }
    ret(ctx, call(ctx, 0x000E4544u, {o, all}));
}

// void f(T* o, a, b): with a path o->+820 and bit 28 of o->+980, walks
// its segments (Field_At388x96(path, i) +48 to Field_At(path, i + 1) +48,
// i < u16 path->+392 - 1, the path re-read each step) until
// Seg_DistSq4A44C8(a, b, &p0, &p1) <= *0x0105309C; then, if
// Field_404Bit15(path) and bit 8 of path->+172, 0x001F46D4(o, i). r3 as
// the original leaves it.
DECOMP(001F4F38, Obj_PathNear_001F4F38) {
    const u32 o = arg<u32>(ctx, 0), a = arg<u32>(ctx, 1), b = arg<u32>(ctx, 2);
    const f64 lim = Lfs42(0x0105309Cu);
    u32 path = load<u32>(o + 820);
    if (path == 0) {
        ret(ctx, o);
        return;
    }
    if (!(load<u32>(o + 980) & 0x10000000u)) {
        ret(ctx, 0u);
        return;
    }
    Stack fr(ctx, 32);  // the two points at sp+112 / sp+128
    const u32 p0 = fr.addr(), p1 = p0 + 16;
    for (u32 i = 0;; ++i) {
        const u32 n = static_cast<u32>(load<u16>(path + 392)) - 1u;
        if (!(i < n)) {
            ret(ctx, n);
            return;
        }
        const u32 s0 = static_cast<u32>(call(ctx, 0x001BD51Cu, {path, i})) + 48;
        for (u32 k = 0; k < 12; k += 4) store<u32>(p0 + k, load<u32>(s0 + k));
        const u32 s1 = static_cast<u32>(call(ctx, 0x001BD51Cu, {load<u32>(o + 820), i + 1})) + 48;
        for (u32 k = 0; k < 12; k += 4) store<u32>(p1 + k, load<u32>(s1 + k));
        const f64 d = callf(ctx, 0x004A44C8u, {a, b, p0, p1}, {});
        if (lim > d || lim == d) {
            if (call(ctx, 0x001BD4A0u, {load<u32>(o + 820)}) == 0) {
                ret(ctx, 0u);
                return;
            }
            if (!(load<u32>(load<u32>(o + 820) + 172) & 0x100u)) {
                ret(ctx, 0u);
                return;
            }
            ret(ctx, call(ctx, 0x001F46D4u, {o, i}));
            return;
        }
        path = load<u32>(o + 820);
    }
}

// void Update(T* o, f32 dt): outside the states 0 / 3 of o->+428 it is
// Obj_UpdateTeamBase(o, dt). Otherwise, with bit 31 of o->+412, the
// position (*0x00F65CA0 + 0x128260) goes to o->vt[316](o, &pos) (a hit
// with u8 +480 clear calls o->vt[284](o, 0) and keeps +480 = 1); else a
// set u8 +480 calls o->vt[288](o, 0) and +480 is cleared. Then
// Obj_UpdateBase(o, dt); r3 = its.
DECOMP(0020FB9C, Obj_UpdateTeam_0020FB9C) {
    const u32 o = arg<u32>(ctx, 0);
    const f64 dt = farg(ctx, 0);
    const s32 st = static_cast<s32>(load<u32>(o + 428));
    if (!(st == 3 || st == 0)) {
        callf(ctx, 0x0020DFD8u, {o}, {dt});
        return;
    }
    u8 flag = 0;
    bool hit = false;
    if (load<u32>(o + 412) & 0x80000000u) {
        Stack fr(ctx, 16);  // the position at sp+112
        const u32 v = fr.addr();
        const u32 src = load<u32>(0x00F65CA0u) + 0x00128260u;
        for (u32 k = 0; k < 12; k += 4) store<u32>(v + k, load<u32>(src + k));
        hit = static_cast<u32>(VCall(ctx, o, 316, {o, v})) != 0;
    }
    if (hit) {
        flag = 1;
        if (load<u8>(o + 480) == 0) VCall(ctx, o, 284, {o, 0});
    } else if (load<u8>(o + 480) != 0) {
        VCall(ctx, o, 288, {o, 0});
    }
    store<u8>(o + 480, flag);
    callf(ctx, 0x000D83C4u, {o}, {dt});
}

// T* New(a, b, Src* s, c) of the 176-byte definition 0x0020BFE8 reads
// (DefNewSmall41: +0 = 0x00400000, +4 = 4, +32 = 1, name 0x00AD376C;
// vtable 0x00B1A120). r3 = the object or 0.
DECOMP(002198A8, Def_New_002198A8) {
    DefNewSmall41(ctx, 0x0020BFE8u, 0x00400000u, 4, 1, true, 0x00AD376Cu, 0x00B1A120u);
}

// ~T() (vtable 0x00B1A770 first when a body is at +428).
DECOMP(0021F088, VolumeShape_Dtor_0021F088) { DtorBodyShape42(ctx, 0x00B1A770u, 428); }

// ~T() (vtable 0x00B1B010 first when a body is at +432).
DECOMP(0022499C, VolumeShape_Dtor_0022499C) { DtorBodyShape42(ctx, 0x00B1B010u, 432); }

// void f(T* o, Msg* m): unless *0x010388F0, bits 17 / 18 of o->+188,
// bit 12 of Obj_Owner(o)->+40, o->+380 > k (k = *0x00229084), a failed
// Obj_TestE0BCC(o, m), or the reach of q = o->+216 (q->+200 > k, q->+204 >
// k, m->+48 > q->+208), it calls o->vt[352](o, m->+4, 0x80F43146, f1 =
// min(q->+204, (m->+48 - q->+208) * q->+200)) and bit 8 of u16 m->+14 is
// set when that returns 0 (cleared otherwise). r3 as the original leaves
// it.
DECOMP(00229088, Obj_OnMsg_00229088) {
    const u32 o = arg<u32>(ctx, 0), m = arg<u32>(ctx, 1);
    const u32 g = load<u8>(0x010388F0u);
    if (g != 0) {
        ret(ctx, g);
        return;
    }
    const u32 w = load<u32>(o + 188);
    if (w & 0x20000u) {
        ret(ctx, w);
        return;
    }
    if (w & 0x40000u) {
        ret(ctx, 0x40000u);
        return;
    }
    if (call(ctx, 0x000D93FCu, {o}) != 0) {
        if (load<u32>(static_cast<u32>(call(ctx, 0x000D93FCu, {o})) + 40) & 0x1000u) {
            ret(ctx, 1u);
            return;
        }
    }
    const f64 k = Lfs42(0x00229084u);
    ctx.fpr[1] = Lfs42(o + 380);  // f1 as the original leaves it
    if (ctx.fpr[1] > k) {
        ret(ctx, 0x00229084u);
        return;
    }
    const u32 t = static_cast<u32>(call(ctx, 0x000E0BCCu, {o, m}));
    const u32 vt = load<u32>(o);
    if (t == 0) {
        ret(ctx, vt);
        return;
    }
    const u32 q = load<u32>(o + 216);
    const f64 f2 = Lfs42(q + 200);
    f64 f1 = Lfs42(q + 204);
    const f64 f4 = Lfs42(m + 48), f3 = Lfs42(q + 208);
    if (f2 > k) ctx.fpr[1] = f1;
    if (!(f2 > k) || !(f1 > k) || !(f4 > f3)) {
        ret(ctx, vt);
        return;
    }
    const f64 r = F42(F42(f4 - f3) * f2);
    if (r < f1) f1 = r;
    // o->vt[352](o, f32 f1, m->+4, 0x80F43146): the float takes r4's slot,
    // which holds the entry address; r7 = the OPD
    const u32 opd = load<u32>(load<u32>(o) + 352);
    ctx.fpr[1] = f1;
    const u32 res = static_cast<u32>(call_ptr(ctx, opd, {o, load<u32>(opd), load<u32>(m + 4), 0x80F43146u, opd}));
    const u32 z = static_cast<u32>(std::countl_zero(res));
    store<u16>(m + 14, static_cast<u16>((load<u16>(m + 14) & ~0x100u) | ((z << 3) & 0x100u)));
    ret(ctx, z);
}

// void f(T* o): with an owner w = Obj_Owner(o) and the trigger record
// t = o->+396 + 1596: when t->+4 >= *0x00238DC0, |o->vt[140](o)|^2 > t->+4,
// t->+0 <= o->+1004 and (u8 t->+8 or u8 (o->+612)->+176 clear), the top
// three bits of *w->vt[8](w) become 001; otherwise w->+136 = 0, bit 31 and
// bits 17..24 of w->+40 are cleared and Obj_PopBodies(w). r3 as the
// original leaves it.
DECOMP(00238DC8, Obj_TriggerOwner_00238DC8) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 w = static_cast<u32>(call(ctx, 0x000D93FCu, {o}));
    if (w == 0) {
        ret(ctx, 0u);
        return;
    }
    const u32 t = load<u32>(o + 396) + 1596;
    const f64 r = Lfs42(t + 4), k = Lfs42(0x00238DC0u);
    bool ok = false;
    ctx.fpr[1] = k;  // f1 as the original leaves it for the calls below
    if (r > k || r == k) {
        Stack fr(ctx, 16);  // the vector at sp+112
        const u32 v = fr.addr();
        VCall(ctx, o, 140, {v, o});
        const f64 y = Lfs42(v + 4), x = Lfs42(v), z = Lfs42(v + 8);
        f64 d = F42(y * y);
        d = F42(std::fma(x, x, d));
        d = F42(std::fma(z, z, d));
        ctx.fpr[1] = d;
        if (r < d) {
            const f64 a = Lfs42(t), b = Lfs42(o + 1004);
            ctx.fpr[1] = a;
            if (a < b || a == b) ok = load<u8>(t + 8) != 0 || load<u8>(load<u32>(o + 612) + 176) == 0;
        }
    }
    if (ok) {
        const u32 p = static_cast<u32>(VCall(ctx, w, 8, {w}));
        store<u32>(p, (load<u32>(p) & 0x1FFFFFFFu) | 0x20000000u);
        ret(ctx, p);
        return;
    }
    const u32 v40 = load<u32>(w + 40) & 0x7FFFFFFFu & ~0x01FE0000u;
    store<u32>(w + 136, 0);
    store<u32>(w + 40, v40);
    ret(ctx, call(ctx, 0x00275030u, {w}));
}

// void SetOn(T* o, u8 on): when on differs from u8 o->+16: turning on
// calls 0x00251B10(e, 1) for each e of the u16 counted array o->+24
// (count +32, re-read); turning off visits each e: its +36 body b gets
// b->vt[40](b, 0), and if still there with bit 1 of b->+192,
// b->vt[352](b, f1 = b->+344, 0, 0); +36 is cleared. u8 o->+16 = on. r3
// as the original leaves it.
DECOMP(002B4D14, Group_SetOn_002B4D14) {
    const u32 o = arg<u32>(ctx, 0), on = arg<u32>(ctx, 1) & 0xFFu;
    const u32 cur = load<u8>(o + 16);
    if (on == cur) {
        store<u8>(o + 16, static_cast<u8>(on));
        ret(ctx, cur);
        return;
    }
    u32 i = 0;
    if (on != 0) {
        for (;; ++i) {
            const u32 n = load<u16>(o + 32);
            if (!(i < n)) {
                ret(ctx, n);
                break;
            }
            call(ctx, 0x00251B10u, {load<u32>(load<u32>(o + 24) + 4 * i), 1});
        }
    } else {
        for (;; ++i) {
            const u32 n = load<u16>(o + 32);
            if (!(i < n)) {
                ret(ctx, n);
                break;
            }
            const u32 e = load<u32>(load<u32>(o + 24) + 4 * i);
            u32 b = load<u32>(e + 36);
            if (b == 0) continue;
            VCall(ctx, b, 40, {b, 0});
            b = load<u32>(e + 36);
            if (b != 0 && (load<u32>(b + 192) & 2u)) {
                const u32 opd = load<u32>(load<u32>(b) + 352);
                ctx.fpr[1] = Lfs42(b + 344);
                call_ptr(ctx, opd, {b, opd, 0, 0, load<u32>(opd)});
            }
            store<u32>(e + 36, 0);
        }
    }
    store<u8>(o + 16, static_cast<u8>(on));
}

// bool f(?, Obj* a, Obj* b): a (with bit 18 of +172, a body +216 whose
// +52 kind k is set) and b (with its +492 kind j set) are compatible when
// j == k, or for j = 0x1D8C2817 also k = 0x9AA76568, j = 0x00014120 also
// k = 0x1D5B6571 or bit 5 of a->+188, j = 0xDF0611F9 also k = 0xB4D78147,
// j = 0x88397820 also k = 0xC92661E2.
DECOMP(002E4ED0, Obj_KindMatch_002E4ED0) {
    const u32 a = arg<u32>(ctx, 1), b = arg<u32>(ctx, 2);
    if (a == 0 || !(load<u32>(a + 172) & 0x40000u)) {
        ret(ctx, 0u);
        return;
    }
    const u32 body = load<u32>(a + 216);
    const u32 k = body != 0 ? load<u32>(body + 52) : 0u;
    const bool f5 = (load<u32>(a + 188) & 0x20u) != 0;
    if (k == 0 || b == 0) {
        ret(ctx, 0u);
        return;
    }
    const u32 j = load<u32>(b + 492);
    if (j == 0) {
        ret(ctx, 0u);
        return;
    }
    bool r;
    if (j == 0x1D8C2817u) r = k == j || k == 0x9AA76568u;
    else if (j == 0x00014120u) r = k == j || f5 || k == 0x1D5B6571u;
    else if (j == 0xDF0611F9u) r = k == j || k == 0xB4D78147u;
    else if (j == 0x88397820u) r = k == j || k == 0xC92661E2u;
    else r = j == k;
    ret(ctx, r ? 1u : 0u);
}

// Static destructor of a block of tuning variables: each gets its vtable
// back (0x00B0D99C / 0x00B0DA04 / 0x00B0D934 / 0x00B0D968) and
// TuningVar_DestructBase; the PtrVectors 0x0106882C / 0x01068818 are
// destroyed in between. r3 as the last call leaves it.
DECOMP(002F5C08, TuningVars_StaticDtor_002F5C08) {
    struct E { u32 var, vt; };
    static constexpr E kA[] = {{0x01068B18u, 0x00B0D99Cu}, {0x01068AC8u, 0x00B0D99Cu}, {0x01068A78u, 0x00B0DA04u},
                               {0x01068A28u, 0x00B0D934u}, {0x010689D8u, 0x00B0D968u}, {0x01068988u, 0x00B0D99Cu}};
    static constexpr E kB[] = {{0x01068938u, 0x00B0D934u}, {0x010688E8u, 0x00B0D99Cu}, {0x01068898u, 0x00B0D99Cu},
                               {0x01068848u, 0x00B0D934u}};
    for (const E& e : kA) {
        store<u32>(e.var, e.vt);
        call(ctx, 0x004FC1A0u, {e.var});
    }
    VecDtor(ctx, 0x0106882Cu);
    for (const E& e : kB) {
        store<u32>(e.var, e.vt);
        call(ctx, 0x004FC1A0u, {e.var});
    }
    VecDtor(ctx, 0x01068818u);
}

// void Tick(T* o, f32 dt): for a non-empty group list (u16 count +60,
// array +52): with no current group +48 it takes entry +44 as current and
// Obj_ResetGroups_002FD1B0(it); every group gets Obj_TickGroups(g, dt).
// Then a current group with u8 +57 or u8 o->+72 set and (u8 +56 clear or
// Obj_EffectsDone of every group) advances: +48 = 0, u8 +72 = 0, +44 + 1
// (wrapping to 0 at the count). r3 as the original leaves it.
DECOMP(002FD2A8, Groups_Tick_002FD2A8) {
    const u32 o = arg<u32>(ctx, 0);
    const f64 dt = farg(ctx, 0);
    if (load<u16>(o + 60) == 0) {
        ret(ctx, 0u);
        return;
    }
    if (load<u32>(o + 48) == 0) {
        const u32 g = load<u32>(load<u32>(o + 52) + (load<u32>(o + 44) << 2));
        store<u32>(o + 48, g);
        call(ctx, 0x002FD1B0u, {g});
    }
    u32 n;
    for (u32 i = 0;; ++i) {
        n = load<u16>(o + 60);
        if (!(i < n)) break;
        callf(ctx, 0x002FD0ACu, {load<u32>(load<u32>(o + 52) + 4 * i)}, {dt});
    }
    ret(ctx, n);
    const u32 cur = load<u32>(o + 48);
    if (cur == 0) return;
    if (load<u8>(cur + 57) == 0 && load<u8>(o + 72) == 0) return;
    if (load<u8>(cur + 56) != 0 && n != 0) {
        for (u32 j = 0;;) {
            if (call(ctx, 0x002FCC8Cu, {load<u32>(load<u32>(o + 52) + 4 * j)}) == 0) {
                ret(ctx, 0u);
                return;
            }
            ++j;
            n = load<u16>(o + 60);
            if (!(j < n)) break;
        }
        ret(ctx, n);
    }
    const u32 k = load<u32>(o + 44) + 1;
    store<u32>(o + 48, 0);
    store<u8>(o + 72, 0);
    store<u32>(o + 44, k);
    if (!(k < n)) store<u32>(o + 44, 0);
}

// void Read(T* o, XmlCursor* x): the shape kind from the element's hash
// (x->+20 when bit 7 of u8 x->+24): 0x04862DD6 -> +0 = 3; 0x03B880B3 ->
// floats 'A' / 'R' into +4 / +8, +0 = 2; 0x8ACA5893 -> 'X' / 'Y', +0 = 1;
// others are reported (String_Format(0x00AD9F14, hash)). Then f32
// 0x03DEC163 into +12 and bool 0x042B7A7B into u8 +16 (defaults the
// current values). r3 = the last read's.
DECOMP(00302DA8, Shape_ReadXml_00302DA8) {
    const u32 o = arg<u32>(ctx, 0), x = arg<u32>(ctx, 1);
    const u32 h = (load<u8>(x + 24) & 0x80u) ? load<u32>(x + 20) : 0u;
    if (h == 0x04862DD6u) {
        store<u32>(o, 3);
    } else if (h == 0x03B880B3u) {
        callf(ctx, 0x004F9B20u, {x, 65, o + 4}, {Lfs42(o + 4)});
        callf(ctx, 0x004F9B20u, {x, 82, o + 8}, {Lfs42(o + 8)});
        store<u32>(o, 2);
    } else if (h == 0x8ACA5893u) {
        callf(ctx, 0x004F9B20u, {x, 88, o + 4}, {Lfs42(o + 4)});
        callf(ctx, 0x004F9B20u, {x, 89, o + 8}, {Lfs42(o + 8)});
        store<u32>(o, 1);
    } else {
        call(ctx, 0x005534D0u, {0x00AD9F14u, h});
    }
    callf(ctx, 0x004F9B20u, {x, 0x03DEC163u, o + 12}, {Lfs42(o + 12)});
    ret(ctx, call(ctx, 0x004F986Cu, {x, 0x042B7A7Bu, o + 16, load<u8>(o + 16)}));
}

// void f(T* o, Desc* d): with a rig (o->+8)->+28, d->+12 / +16 = the bones
// d->+4 / d->+8 (Rig_FindBoneA59468(rig, name, 0)); then d's 64-byte
// record (+0..+16, f32 +32..+40, u8 +48) is appended to the PtrVector-like
// list o->+104 (SmallAlloc(20) + PtrVector_Construct, u16 +6 = 64, made on
// first use; doubled with PtrVector_Reserve when full; quirk: a failed
// allocation goes on with the list at 0). r3 = the old high-water mark.
DECOMP(00352B08, Rig_AddBonePair_00352B08) {
    const u32 o = arg<u32>(ctx, 0), d = arg<u32>(ctx, 1);
    u32 rig = load<u32>(o + 8);
    if (rig == 0 || (rig = load<u32>(rig + 28)) == 0) {
        ret(ctx, o);
        return;
    }
    store<u32>(d + 12, static_cast<u32>(call(ctx, 0x00A59468u, {rig, load<u32>(d + 4), 0})));
    store<u32>(d + 16, static_cast<u32>(call(ctx, 0x00A59468u, {rig, load<u32>(d + 8), 0})));
    u32 v = load<u32>(o + 104);
    if (v == 0) {
        v = static_cast<u32>(call(ctx, 0x0055238Cu, {20}));
        store<u32>(o + 104, v);
        if (v != 0) {
            call(ctx, 0x005516E0u, {v});
            store<u16>(v + 6, 64);
            v = load<u32>(o + 104);
        }
    }
    u32 n = load<u16>(v + 8);
    const u32 cap = load<u16>(v + 4);
    if (n == cap) {
        call(ctx, 0x00551718u, {v, cap << 1});
        n = load<u16>(v + 8);
    }
    const u32 e = load<u32>(v) + (n << 6);
    for (u32 k = 0; k < 20; k += 4) store<u32>(e + k, load<u32>(d + k));
    for (u32 k = 32; k < 44; k += 4) store<u32>(e + k, load<u32>(d + k));
    store<u8>(e + 48, load<u8>(d + 48));
    ctx.fpr[1] = Lfs42(d + 40);  // f1 as the original leaves it (the last lfs)
    const u32 n1 = (load<u16>(v + 8) + 1u) & 0xFFFFu;
    const u32 hw = load<u16>(v + 10);
    store<u16>(v + 8, static_cast<u16>(n1));
    store<u16>(v + 10, static_cast<u16>(hw > n1 ? hw : n1));
    ret(ctx, hw);
}

// ~T(): with an object p at +24, vtable 0x00B1F7C8 first and
// p->vt[40](p, 0, 1); then the shared body (DtorB1F678_36).
DECOMP(0036EB74, Dtor_0036EB74) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 p = load<u32>(o + 24);
    if (p != 0) {
        store<u32>(o, 0x00B1F7C8u);
        VCall(ctx, p, 40, {p, 0, 1});
    }
    ctx.gpr[3] = o;  // the shared body reads o from r3
    DtorB1F678_36(ctx);
}

// void f(Vec3* out, Vec3* a, Vec3* b, f32 t, ?, x, Vec3* dir): the step
// from a toward b. Within *0x004A8944 of each other out = b (dir = the
// zero vector 0x01164B40 when given); else 0x004A8778(0x004A8944,
// 0x004B0000, b, x, &s; f1 = *0x004A8948, f2 = |b - a|, f3 = t) gives the
// distance d along (b - a) (dir = (b - a) * s / |b - a|) and out = a + (b
// - a) * d / |b - a|. r3 / f1 as the original leaves them.
DECOMP(004A8950, Vec_StepToward_004A8950) {
    const u32 out = arg<u32>(ctx, 0), a = arg<u32>(ctx, 1), b = arg<u32>(ctx, 2), x = arg<u32>(ctx, 4),
              dir = arg<u32>(ctx, 5);
    const f64 t = farg(ctx, 0);
    const f64 dy = F42(Lfs42(b + 4) - Lfs42(a + 4));
    const f64 dx = F42(Lfs42(b) - Lfs42(a));
    const f64 dz = F42(Lfs42(b + 8) - Lfs42(a + 8));
    f64 len = F42(dy * dy);
    len = F42(std::fma(dx, dx, len));
    len = F42(std::fma(dz, dz, len));
    len = F42(std::sqrt(len));
    if (!(len > Lfs42(0x004A8944u))) {
        ctx.gpr[3] = x;
        if (dir != 0) {
            for (u32 k = 0; k < 12; k += 4) store<u32>(dir + k, load<u32>(0x01164B40u + k));
            ctx.gpr[3] = 0x01160000u;
        }
        store<u32>(out, load<u32>(b));
        store<u32>(out + 4, load<u32>(b + 4));
        store<u32>(out + 8, load<u32>(b + 8));
        ctx.fpr[1] = Lfs42(b + 8);
        return;
    }
    Stack fr(ctx, 16);  // s at sp+112
    const u32 s = fr.addr();
    const f64 r = callf(ctx, 0x004A8778u, {0x004A8944u, 0x004B0000u, b, x, s, dir}, {Lfs42(0x004A8948u), len, t});
    if (dir != 0) {
        const f64 q = F42(Lfs42(s) / len);
        store<u32>(dir, W42(q * dx));
        store<u32>(dir + 4, W42(q * dy));
        store<u32>(dir + 8, W42(q * dz));
    }
    const f64 f = F42(r / len);
    const f64 ox = F42(std::fma(dx, f, Lfs42(a))), oy = F42(std::fma(dy, f, Lfs42(a + 4))),
              oz = F42(std::fma(dz, f, Lfs42(a + 8)));
    store<u32>(out, W42(ox));
    store<u32>(out + 4, W42(oy));
    store<u32>(out + 8, W42(oz));
    ctx.fpr[1] = oz;
}

// T* New(?, XmlCursor* x): a 40-byte record (SmallAlloc(40); r3 = 0 on
// failure): vtable 0x00B21DC8, u16 +4 = 1, +20 / +24 / +28 / +36 = 0, u8
// +32 = 1, u8 +33 = 0; read: hash 0x0024728B into +8; hash 0x00014B01 into
// +20, then as an s32 0x38939B8F when found; likewise 0x000107E5 /
// 0x86E24673 into +24; bool 0x80B914A7 into u8 +32 (default 1). r3 = p.
DECOMP(004BF2F0, Rec_NewXml_004BF2F0) {
    const u32 x = arg<u32>(ctx, 1);
    const u32 p = static_cast<u32>(call(ctx, 0x0055238Cu, {40}));
    if (p == 0) {
        ret(ctx, 0u);
        return;
    }
    store<u16>(p + 4, 1);
    store<u32>(p, 0x00B21DC8u);
    store<u32>(p + 20, 0);
    store<u32>(p + 24, 0);
    store<u32>(p + 28, 0);
    store<u32>(p + 36, 0);
    store<u8>(p + 32, 1);
    store<u8>(p + 33, 0);
    call(ctx, 0x004FA14Cu, {x, 0x0024728Bu, p + 8, 0});
    if (call(ctx, 0x004FA14Cu, {x, 0x00014B01u, p + 20, 0}) != 0) call(ctx, 0x004F99BCu, {x, 0x38939B8Fu, p + 20, 0});
    if (call(ctx, 0x004FA14Cu, {x, 0x000107E5u, p + 24, 0}) != 0) call(ctx, 0x004F99BCu, {x, 0x86E24673u, p + 24, 0});
    call(ctx, 0x004F986Cu, {x, 0x80B914A7u, p + 32, 1});
    ret(ctx, p);
}

// bool f(XmlCursor* x): with the settings block g = *0x010C2C28: the slot
// i (s32 by name 0x00AF01DC, default 0) gets bool 0x00AF0144 into u8 g +
// 721 + i, f32 0x00AF0170 / 0x00AF0178 into g + 724 / 732 + 4i (default
// *0x004EDAF8), a colour 0x00AF0180 into g + 740 + 4i (default
// 0x01164CA0) and s32 0x00AF01E4 into g + 748 + 4i; u8 g->+720 = 1. r3 =
// 1 (0 without g).
DECOMP(004EDB00, Settings_ReadSlot_004EDB00) {
    const u32 x = arg<u32>(ctx, 0);
    const u32 g = load<u32>(0x010C2C28u);
    if (g == 0) {
        ret(ctx, 0u);
        return;
    }
    Stack fr(ctx, 16);  // i at sp+112
    const u32 pi = fr.addr();
    store<u32>(pi, 0);
    call(ctx, 0x004FA564u, {x, 0x00AF01DCu, pi, 0});
    call(ctx, 0x004FA504u, {x, 0x00AF0144u, g + load<u32>(pi) + 721, 0});
    const f64 k = Lfs42(0x004EDAF8u);
    callf(ctx, 0x004FA5C4u, {x, 0x00AF0170u, g + (load<u32>(pi) << 2) + 724}, {k});
    callf(ctx, 0x004FA5C4u, {x, 0x00AF0178u, g + (load<u32>(pi) << 2) + 732}, {k});
    call(ctx, 0x004FA684u, {x, 0x00AF0180u, g + (load<u32>(pi) << 2) + 740, 0x01164CA0u});
    call(ctx, 0x004FA564u, {x, 0x00AF01E4u, g + (load<u32>(pi) << 2) + 748, 0});
    store<u8>(g + 720, 1);
    ret(ctx, 1u);
}

// bool f(XmlCursor* x): the settings block g = *0x010C2C28 reads bool
// 0x00AF0144 into u8 +825, f32 0x00AF0314 / 0x00AF0324 into +828 / +832,
// s32 0x00AF0334 / 0x00AF0344 into +844 / +848, f32 0x00AF0354 /
// 0x00AF0364 into +836 / +840 (float default *0x004EEB3C) and a colour
// 0x00AF0278 into +852 (default 0x01164CA0). r3 = 1 (0 without g).
DECOMP(004EEB40, Settings_Read_004EEB40) {
    const u32 x = arg<u32>(ctx, 0);
    const u32 g = load<u32>(0x010C2C28u);
    if (g == 0) {
        ret(ctx, 0u);
        return;
    }
    call(ctx, 0x004FA504u, {x, 0x00AF0144u, g + 825, 0});
    const f64 k = Lfs42(0x004EEB3Cu);
    callf(ctx, 0x004FA5C4u, {x, 0x00AF0314u, g + 828}, {k});
    callf(ctx, 0x004FA5C4u, {x, 0x00AF0324u, g + 832}, {k});
    call(ctx, 0x004FA564u, {x, 0x00AF0334u, g + 844, 0});
    call(ctx, 0x004FA564u, {x, 0x00AF0344u, g + 848, 0});
    callf(ctx, 0x004FA5C4u, {x, 0x00AF0354u, g + 836}, {k});
    callf(ctx, 0x004FA5C4u, {x, 0x00AF0364u, g + 840}, {k});
    call(ctx, 0x004FA684u, {x, 0x00AF0278u, g + 852, 0x01164CA0u});
    ret(ctx, 1u);
}

// u32 f(FileReader* r) of 0x004F505C / 0x004F52C4: the chunk (Iff_ChunkSize
// bytes; r3 = 0 when empty) is read into a MemStack_Alloc block of the
// default stack, opened as a zip (Archive_OpenZip(buf, n, *0x00F668D4)),
// loaded as an xmlb document on the stack (XmlDoc_Init / LoadXmlb(doc, z,
// 0)) and handed to apply(*0x00F668D8, &doc, 0), or reported
// (String_Format(msg)); then the document, the zip (z->vt[8](z)) and the
// block (stack->vt[8](stack, buf)) are released and *0x00F668D4 = 0. r3 = 0.
static void LoadXmlbChunk42(PPUContext& ctx, u32 apply, u32 msg) {
    const u32 r = arg<u32>(ctx, 0);
    const u32 n = static_cast<u32>(call(ctx, 0x005637D4u, {r}));
    if (n == 0) {
        ret(ctx, 0u);
        return;
    }
    const u32 st = static_cast<u32>(call(ctx, 0x005525FCu, {}));
    const u32 buf = static_cast<u32>(call(ctx, 0x0055260Cu, {st, n}));
    call(ctx, 0x00563768u, {r, buf, n});
    const u32 z = static_cast<u32>(call(ctx, 0x00567034u, {buf, n, load<u32>(0x00F668D4u)}));
    const u32 dst = load<u32>(0x00F668D8u);
    Stack fr(ctx, 2240);  // the document at sp+112 .. sp+2351
    const u32 doc = fr.addr();
    call(ctx, 0x004FA7A4u, {doc});
    if (call(ctx, 0x004FACCCu, {doc, z, 0}) != 0) call(ctx, apply, {dst, doc, 0});
    else call(ctx, 0x005534D0u, {msg});
    call(ctx, 0x004FA9C0u, {doc});
    VCall(ctx, z, 8, {z});
    const u32 st2 = static_cast<u32>(call(ctx, 0x005525FCu, {}));
    VCall(ctx, st2, 8, {st2, buf});
    store<u32>(0x00F668D4u, 0);
    ret(ctx, 0u);
}

DECOMP(004F505C, Xmlb_LoadChunk_004F505C) { LoadXmlbChunk42(ctx, 0x004F457Cu, 0x00AF05B0u); }

DECOMP(004F52C4, Xmlb_LoadChunk_004F52C4) { LoadXmlbChunk42(ctx, 0x004F4B28u, 0x00AF0794u); }

// ---------------------------------------------------------------- sweep: 90 instructions
// void f(T* o): the animation name for the current mode (the string at
// index (*0x010AAF8C)->+64 of ((o->+224)->+0)->+12), when not empty:
// o->vt[220](o, 0), then Anim_Restart2775A8(o->+64, hash, 0, 0, 0, -1, 0,
// 0, -, -, 0, -1; f1..f4 = *0x0003DECC / +4 / +8 / +8) and o->+32 = the
// hash when it started. Then with o->+224 the second name (index
// (*0x010AAFA4)->+64) is applied with 0x00014004(o, hash) when it hashes
// non-zero, and Msg_Post(o->+48, m, m, ((o->+224)->+4)->+204) with m =
// *0x00F65C54. r3 = Msg_Post's (0 without o->+224).
DECOMP(0003DED8, Obj_ApplyModeAnim_0003DED8) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 s = load<u32>(load<u32>(load<u32>(load<u32>(o + 224)) + 12) + (load<u32>(load<u32>(0x010AAF8Cu) + 64) << 2));
    if (s != 0 && load<u8>(s) != 0) {
        VCall(ctx, o, 220, {o, 0});
        const u32 h = static_cast<u32>(call(ctx, 0x0056A710u, {s}));
        // stack arguments 9 / 10 (sp+112 / sp+120) are not written by the original
        const u32 r = static_cast<u32>(callf(ctx, 0x002775A8u, {load<u32>(o + 64), h, 0, 0, 0, ~0ull, 0, 0, 0, 0, 0, ~0ull},
                                             {Lfs42(0x0003DECCu), Lfs42(0x0003DED0u), Lfs42(0x0003DED4u), Lfs42(0x0003DED4u)}));
        if (r != 0) store<u32>(o + 32, h);
    }
    const u32 x = load<u32>(o + 224);
    if (x == 0) {
        ret(ctx, 0u);
        return;
    }
    const u32 s2 = load<u32>(load<u32>(load<u32>(x) + 12) + (load<u32>(load<u32>(0x010AAFA4u) + 64) << 2));
    const u32 h2 = static_cast<u32>(call(ctx, 0x0056A710u, {s2}));
    if (h2 != 0) call(ctx, 0x00014004u, {o, h2});
    const u32 info = load<u32>(load<u32>(o + 224) + 4);
    const u32 m = load<u32>(0x00F65C54u);
    ret(ctx, call(ctx, 0x00470660u, {load<u32>(o + 48), m, m, load<u32>(info + 204)}));
}

// void f(T* o, Ctx* x): the target t = Target_Resolve(o, o->+12, x); for
// kind o->+28 0x7A30D7E3 o->+40 is resolved the same way, for 0x0021B90E
// it becomes the component o->+32 of x (Effect_FindComponent) when its
// vt[32]() is 2 (its vt[24]()), else 0. Then with o->+52 > *0x0006EB4C
// Obj_CollectNear(t, o->+44, o->+24, o->+40), else Msg_Post(t, ...)
// (f1 = o->+52 for both). r3 = theirs.
DECOMP(0006EB50, Action_Post_0006EB50) {
    const u32 o = arg<u32>(ctx, 0), x = arg<u32>(ctx, 1);
    const u32 t = static_cast<u32>(call(ctx, 0x0009A190u, {o, load<u32>(o + 12), x}));
    const u32 kind = load<u32>(o + 28);
    if (kind == 0x7A30D7E3u) {
        store<u32>(o + 40, static_cast<u32>(call(ctx, 0x0009A190u, {o, load<u32>(o + 40), x})));
    } else if (kind == 0x0021B90Eu) {
        const u32 c = load<u32>(o + 32);
        if (c != 0) {
            const u32 comp = static_cast<u32>(call(ctx, 0x004BB4B4u, {x, c}));
            u32 v = 0;
            if (comp != 0 && static_cast<s32>(VCall(ctx, comp, 32, {comp})) == 2) v = static_cast<u32>(VCall(ctx, comp, 24, {comp}));
            store<u32>(o + 40, v);
        }
    }
    const f64 r = Lfs42(o + 52);
    ctx.fpr[1] = r;
    if (r > Lfs42(0x0006EB4Cu)) call(ctx, 0x0048F90Cu, {t, load<u32>(o + 44), load<u32>(o + 24), load<u32>(o + 40)});
    else call(ctx, 0x00470660u, {t, load<u32>(o + 44), load<u32>(o + 24), load<u32>(o + 40)});
}

// void f(T* o, Ctx* x): the path object: the component o->+12 of x when
// its vt[32]() is 5 and its +28 object carries the path (+276 ==
// 0x02235C33: +280), else Obj_FindById(o->+16). Found with bit 27 of
// +196: u8 o->+21 calls Path_SetActive(p, u8 o->+20), u8 o->+23 copies bit
// 0 of u8 o->+22 into bit 26 of p->+972. Otherwise the names are formatted
// (Hash_ToString of o->+12 / o->+16, when set). r3 as the original leaves
// it.
DECOMP(00073748, Action_PathSet_00073748) {
    const u32 o = arg<u32>(ctx, 0), x = arg<u32>(ctx, 1);
    u32 p = 0;
    bool viaComp = false;
    const u32 c = load<u32>(o + 12);
    if (c != 0) {
        const u32 comp = static_cast<u32>(call(ctx, 0x004BB4B4u, {x, c}));
        if (comp != 0 && static_cast<s32>(VCall(ctx, comp, 32, {comp})) == 5) {
            const u32 q = load<u32>(comp + 28);
            if (q != 0) {
                if (load<u32>(q + 276) == 0x02235C33u) p = load<u32>(q + 280);
                viaComp = p != 0;
            }
        }
    }
    if (!viaComp) {
        const u32 id = load<u32>(o + 16);
        if (id != 0) p = static_cast<u32>(call(ctx, 0x000D6A28u, {id}));
    }
    if (p == 0 || !(load<u32>(p + 196) & 0x08000000u)) {
        const u32 a = load<u32>(o + 12);
        ret(ctx, a);
        if (a != 0) call(ctx, 0x0056A6E0u, {a});
        const u32 b = load<u32>(o + 16);
        if (b != 0) call(ctx, 0x0056A6E0u, {b});
        return;
    }
    const u32 b21 = load<u8>(o + 21);
    ret(ctx, b21);
    if (b21 != 0) call(ctx, 0x001D218Cu, {p, load<u8>(o + 20)});
    const u32 b23 = load<u8>(o + 23);
    ret(ctx, b23);
    if (b23 != 0) {
        const u32 b22 = load<u8>(o + 22);
        store<u32>(p + 972, (load<u32>(p + 972) & ~0x04000000u) | ((b22 << 26) & 0x04000000u));
        ret(ctx, b22);
    }
}

// T* New(?, XmlCursor* x): a 28-byte object (Mem_Alloc(28, 0, 16); r3 = 0
// on failure) made by Obj_InitVt_B21C98, vtable 0x00B10260, +12..+20 = 0,
// u8 +24..+27 = 0; read: hashes 0x8A93973F / 0x7A30D7E3 / 0xE33D410A into
// +12 / +16 / +20, bools 0x03C5CC6D / 0x7A281983 into u8 +24 / +26 (a
// missing one sets u8 +25 / +27). r3 = p.
DECOMP(000878D8, Act_NewXml_000878D8) {
    const u32 x = arg<u32>(ctx, 1);
    const u32 p = static_cast<u32>(call(ctx, 0x0054ED38u, {28, 0, 16}));
    if (p == 0) {
        ret(ctx, 0u);
        return;
    }
    call(ctx, 0x004BB3B0u, {p, x});
    store<u32>(p + 12, 0);
    store<u32>(p + 16, 0);
    store<u32>(p + 20, 0);
    store<u32>(p, 0x00B10260u);
    for (u32 k = 24; k < 28; ++k) store<u8>(p + k, 0);
    call(ctx, 0x004FA14Cu, {x, 0x8A93973Fu, p + 12, 0});
    call(ctx, 0x004FA14Cu, {x, 0x7A30D7E3u, p + 16, 0});
    call(ctx, 0x004FA14Cu, {x, 0xE33D410Au, p + 20, 0});
    if (call(ctx, 0x004F986Cu, {x, 0x03C5CC6Du, p + 24, 0}) == 0) store<u8>(p + 25, 1);
    if (call(ctx, 0x004F986Cu, {x, 0x7A281983u, p + 26, 0}) == 0) store<u8>(p + 27, 1);
    ret(ctx, p);
}

// T* New(?, XmlCursor* x): a 28-byte object (Mem_Alloc(28, 0, 16); r3 = 0
// on failure) made by Obj_InitVt_B21C98, vtable 0x00B10620, +12..+20 = 0,
// u8 +24..+26 = 0; read: f32 0x17A758DC into +12 (default *0x0008BCA0),
// hashes 0x9B5EF4AB / 0xA3370170 into +16 / +20 (a missing +12 / +16 sets
// u8 +25, a missing +20 u8 +26), bool 0x25F79944 into u8 +24 (defaults the
// current values). r3 = p.
DECOMP(0008BCA8, Act_NewXml_0008BCA8) {
    const u32 x = arg<u32>(ctx, 1);
    const u32 p = static_cast<u32>(call(ctx, 0x0054ED38u, {28, 0, 16}));
    if (p == 0) {
        ret(ctx, 0u);
        return;
    }
    call(ctx, 0x004BB3B0u, {p, x});
    store<u32>(p, 0x00B10620u);
    store<u32>(p + 12, 0);
    store<u32>(p + 16, 0);
    store<u32>(p + 20, 0);
    store<u8>(p + 24, 0);
    store<u8>(p + 25, 0);
    store<u8>(p + 26, 0);
    callf(ctx, 0x004F9B20u, {x, 0x17A758DCu, p + 12}, {Lfs42(0x0008BCA0u)});
    if (static_cast<u32>(ctx.gpr[3]) == 0) store<u8>(p + 25, 1);
    if (call(ctx, 0x004FA14Cu, {x, 0x9B5EF4ABu, p + 16, load<u32>(p + 16)}) == 0) store<u8>(p + 25, 1);
    if (call(ctx, 0x004FA14Cu, {x, 0xA3370170u, p + 20, load<u32>(p + 20)}) == 0) store<u8>(p + 26, 1);
    call(ctx, 0x004F986Cu, {x, 0x25F79944u, p + 24, load<u8>(p + 24)});
    ret(ctx, p);
}

// T* New(?, XmlCursor* x): a 160-byte object (Mem_Alloc(160, 0, 16); r3 =
// 0 on failure) made by Obj_InitVt_B21C98, vtable 0x00B11580, the vec2s
// +140 / +148 = *0x01164AF4 / *0x01164AFC, u8 +156 = 1, name +12 empty;
// read: the name attribute 0x00AC59C0 (String_CopyN(p + 12, s, 127) when
// not empty), vec2s 0x04B1112A / 0x8ACA5893 into +140 / +148 and bool
// 0x8488565B into u8 +156 (defaults the current values). r3 = p.
DECOMP(000A1A30, Act_NewXml_000A1A30) {
    const u32 x = arg<u32>(ctx, 1);
    const u32 p = static_cast<u32>(call(ctx, 0x0054ED38u, {160, 0, 16}));
    if (p == 0) {
        ret(ctx, 0u);
        return;
    }
    call(ctx, 0x004BB3B0u, {p, x});
    store<u32>(p + 140, load<u32>(0x01164AF4u));
    store<u32>(p, 0x00B11580u);
    store<u32>(p + 144, load<u32>(0x01164AF8u));
    store<u8>(p + 156, 1);
    store<u32>(p + 148, load<u32>(0x01164AFCu));
    store<u8>(p + 12, 0);
    store<u32>(p + 152, load<u32>(0x01164B00u));
    ctx.fpr[1] = Lfs42(0x01164B00u);  // f1 as the original leaves it (the last lfs)
    const u32 s = static_cast<u32>(call(ctx, 0x004F9628u, {x, 0x00AC59C0u}));
    if (s != 0 && load<u8>(s) != 0) call(ctx, 0x00553638u, {p + 12, s, 127});
    call(ctx, 0x004F9C9Cu, {x, 0x04B1112Au, p + 140, p + 140});
    call(ctx, 0x004F9C9Cu, {x, 0x8ACA5893u, p + 148, p + 148});
    call(ctx, 0x004F986Cu, {x, 0x8488565Bu, p + 156, load<u8>(p + 156)});
    ret(ctx, p);
}

// Static constructor of a block of tuning variables: the words 0x01013778
// .. 0x01013780 cleared; TuningVar_ConstructBase(var, name) for 0x00F9D090
// (vtable 0x00B0DA38, String_CopyN(0x00F9D0D8, 0x00AC62C4, 127)),
// 0x00F9D158 (+72 = 0, vtable 0x00B0D968), 0x00F9CFAC (u8 +72 = 1,
// vtable 0x00B0D934), 0x00F9CFF8 (vtable 0x00B0D99C, +72 = 0.85f) and
// 0x00F9D044 (vtable 0x00B0D99C, +72 = 0.6f); u8 0x00F9D1A4 = 0. r3 as the
// original leaves it (0x3F19999A).
DECOMP(000B2114, TuningVars_StaticInit_000B2114) {
    store<u32>(0x01013778u, 0);
    store<u32>(0x0101377Cu, 0);
    store<u32>(0x01013780u, 0);
    call(ctx, 0x004FBFA8u, {0x00F9D090u, 0x00AC62A4u});
    store<u32>(0x00F9D090u, 0x00B0DA38u);
    call(ctx, 0x00553638u, {0x00F9D0D8u, 0x00AC62C4u, 127});
    call(ctx, 0x004FBFA8u, {0x00F9D158u, 0x00AC62DCu});
    store<u32>(0x00F9D158u + 72, 0);
    store<u32>(0x00F9D158u, 0x00B0D968u);
    call(ctx, 0x004FBFA8u, {0x00F9CFACu, 0x00AC62FCu});
    store<u8>(0x00F9CFACu + 72, 1);
    store<u32>(0x00F9CFACu, 0x00B0D934u);
    call(ctx, 0x004FBFA8u, {0x00F9CFF8u, 0x00AC631Cu});
    store<u32>(0x00F9CFF8u, 0x00B0D99Cu);
    store<u32>(0x00F9CFF8u + 72, 0x3F59999Au);
    call(ctx, 0x004FBFA8u, {0x00F9D044u, 0x00AC633Cu});
    store<u32>(0x00F9D044u, 0x00B0D99Cu);
    store<u32>(0x00F9D044u + 72, 0x3F19999Au);
    store<u8>(0x00F9D1A4u, 0);
    ret(ctx, 0x3F19999Au);
}

// void f(T* o): s = o->vt[32](o, 1) (signed): 1 with u16 (*0x00F65CA0)
// +0x128214 clear calls o->vt[28](o, 1, 2) and Msg_Post(o, 0xAFEE1340,
// table 0x00F646F8[o->+724], 0); 0 starts the scene
// World_StartScene46C3B8(*0x00F65CA0, o->+216 entry o->+724 (8 bytes,
// from +232) word, 0, 1, 0, 0, 0, 0, 0, 0) and calls o->vt[28](o, 1, 1).
// r3 as the original leaves it.
DECOMP(000BDB70, Obj_SceneStep_000BDB70) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 ent = load<u32>(o + 216) + (load<u32>(o + 724) << 3) + 232;
    const s32 s = static_cast<s32>(VCall(ctx, o, 32, {o, 1}));
    if (s > 0) {
        if (s > 1) return;
        const u32 busy = load<u16>(load<u32>(0x00F65CA0u) + 0x00128214u);
        if (busy != 0) {
            ret(ctx, busy);
            return;
        }
        VCall(ctx, o, 28, {o, 1, 2});
        const u32 v = load<u32>(0x00F646F8u + (load<u32>(o + 724) << 2));
        call(ctx, 0x00470660u, {o, 0xAFEE1340u, v, 0, 0x00F646F8u});
        return;
    }
    if (s > -1) {
        call(ctx, 0x0046C3B8u, {load<u32>(0x00F65CA0u), load<u32>(ent), 0, 1, 0, 0, 0, 0, 0, 0});
        VCall(ctx, o, 28, {o, 1, 1});
    }
}

// void SetRagdoll(T* o, u8 on): turning on calls o->vt[396](o, 4, 0) and
// drops the skeleton o->+388 (each of its 0x0091A2CC() bones
// 0x0091A7D8(skel, i) released with AnimSkel_Release7D4EA8, the properties
// 0xA734004D / 0x91C7CF5B removed (Hk_RemoveProperty93FED8(&tmp, skel,
// hash)) and the skeleton released) and the effects +420 / +416 (bit 9 of
// +136 set, Effect_Delete; quirk: +420 is the word cleared both times).
// Then Obj_SetRagdollBase(o, on); r3 = its.
DECOMP(001783E0, Obj_SetRagdoll_001783E0) {
    const u32 o = arg<u32>(ctx, 0), on = arg<u32>(ctx, 1) & 0xFFu;
    if (on != 0) {
        VCall(ctx, o, 396, {o, 4, 0});
        if (load<u32>(o + 388) != 0) {
            for (s32 i = 0;; ++i) {
                const s32 n = static_cast<s32>(call(ctx, 0x0091A2CCu, {load<u32>(o + 388)}));
                if (!(i < n)) break;
                const u32 b = static_cast<u32>(call(ctx, 0x0091A7D8u, {load<u32>(o + 388), static_cast<u64>(static_cast<s64>(i))}));
                call(ctx, 0x007D4EA8u, {b});
            }
            Stack fr(ctx, 16);  // the two results at sp+112 / sp+120
            call(ctx, 0x0093FED8u, {fr.addr(), load<u32>(o + 388), 0xA734004Du});
            call(ctx, 0x0093FED8u, {fr.addr() + 8, load<u32>(o + 388), 0x91C7CF5Bu});
            call(ctx, 0x007D4EA8u, {load<u32>(o + 388)});
        }
        u32 e = load<u32>(o + 420);
        if (e != 0) {
            store<u32>(e + 136, load<u32>(e + 136) | 0x200u);
            call(ctx, 0x004D0280u, {load<u32>(o + 420)});
            store<u32>(o + 420, 0);
        }
        e = load<u32>(o + 416);
        if (e != 0) {
            store<u32>(e + 136, load<u32>(e + 136) | 0x200u);
            call(ctx, 0x004D0280u, {load<u32>(o + 416)});
            store<u32>(o + 420, 0);  // sic: +416 itself is left set
        }
    }
    ret(ctx, call(ctx, 0x000E02F8u, {o, on}));
}

// void OnEvent(T* o, Event* ev): for kind 0x69988172 the first entry
// {obj, fx} of o->+424 (u16 count +432, 8 bytes each) whose obj is ev->+0
// and that is enabled (u8 o->+464, or obj->vt[32](obj, 0) == 0) gets: r =
// obj->vt[272](obj); with r, fx->+20 = o->+452 (when fx) and
// Effect_SetHash4CF988(r, 0x8FA5922D); then obj->vt[28](obj, 0, 1). Then
// Obj_OnEvent_000E5E44(o, ev); r3 = its.
DECOMP(001AAA70, Obj_OnEvent_001AAA70) {
    const u32 o = arg<u32>(ctx, 0), ev = arg<u32>(ctx, 1);
    if (load<u32>(ev + 8) == 0x69988172u) {
        for (u32 i = 0; i < load<u16>(o + 432); ++i) {
            const u32 e = load<u32>(o + 424) + 8 * i;
            u32 x = load<u32>(e);
            if (load<u32>(ev) != x) continue;
            if (load<u8>(o + 464) == 0) {
                if (VCall(ctx, x, 32, {x, 0}) != 0) continue;
                x = load<u32>(e);
            }
            const u32 r = static_cast<u32>(VCall(ctx, x, 272, {x}));
            if (r != 0) {
                const u32 fx = load<u32>(e + 4);
                if (fx != 0) {
                    ctx.fpr[1] = Lfs42(o + 452);  // f1 as the original leaves it
                    store<u32>(fx + 20, load<u32>(o + 452));
                }
                call(ctx, 0x004CF988u, {r, 0x8FA5922Du});
            }
            x = load<u32>(e);
            VCall(ctx, x, 28, {x, 0, 1});
            break;
        }
    }
    ret(ctx, call(ctx, 0x000E5E44u, {o, ev}));
}

// void f(T* o, u8* out): with a body o->+216: an attached o->+556 gives
// *out = (u16 +352 != 0); otherwise *out = (o->+564 <= *0x001D48B0), then
// the first live player slot q (u8 count (*0x00F65CA0)->+29, words at
// +4216 + 0x20C80 * k) within o->+564 squared of o's position +144 sets
// *out = 1 and, when the body's +408 target t and q's body +216 (bit 3 of
// +20) are set, *out = (body +192 == t || body +980 != 0). A leaf; r3 /
// f1 as the original leaves them.
DECOMP(001D48B8, Obj_PlayerNear_001D48B8) {
    const u32 o = arg<u32>(ctx, 0), out = arg<u32>(ctx, 1);
    const u32 body = load<u32>(o + 216);
    if (body == 0) return;
    const u32 att = load<u32>(o + 556);
    if (att != 0) {
        const u32 v = load<u16>(att + 352) != 0 ? 1u : 0u;
        store<u8>(out, static_cast<u8>(v));
        ret(ctx, v);
        return;
    }
    const f64 rr = Lfs42(o + 564);
    store<u8>(out, (rr < Lfs42(0x001D48B0u) || rr == Lfs42(0x001D48B0u)) ? 1 : 0);
    const f64 px = Lfs42(o + 144), py = Lfs42(o + 148), pz = Lfs42(o + 152);
    ctx.fpr[1] = rr;
    u32 off = 0;
    u32 q = 0;
    for (u32 k = 0;; ++k, off += 0x00020C80u) {
        const u32 g = load<u32>(0x00F65CA0u);
        if (!(k < load<u8>(g + 29))) {
            ret(ctx, off);
            return;
        }
        q = load<u32>(g + off + 4216);
        if (q == 0) continue;
        const f64 dy = F42(Lfs42(q + 148) - py), dx = F42(Lfs42(q + 144) - px), dz = F42(Lfs42(q + 152) - pz);
        f64 d = F42(dy * dy);
        d = F42(std::fma(dx, dx, d));
        d = F42(std::fma(dz, dz, d));
        if (d < rr || d == rr) break;
    }
    ret(ctx, 1u);
    const u32 t = load<u32>(body + 408);
    store<u8>(out, 1);
    if (t == 0) return;
    const u32 qb = load<u32>(q + 216);
    if (qb == 0) return;
    if (!(load<u32>(qb + 20) & 8u)) {
        ret(ctx, 0u);
        return;
    }
    const u32 a = load<u32>(qb + 192);
    if (a == t) {
        ret(ctx, a);
        store<u8>(out, 1);
        return;
    }
    const u32 b = load<u32>(qb + 980);
    ret(ctx, b);
    store<u8>(out, b != 0 ? 1 : 0);
}

// bool Tick(T* o): false when Obj_TickBase(o) fails; with
// o->vt[32](o, 0) and o->vt[32](o, 1) both 1 and Anim_TrackSet164(o, 0,
// 0) set, +348 = k (*0x00228E30; the clamp of +344 against k leaves k in
// f1 either way) and o->vt[28](o, 1, 3; f1 = k). r3 = (o->+348 > k).
DECOMP(00228E38, Obj_Tick_00228E38) {
    const u32 o = arg<u32>(ctx, 0);
    if (call(ctx, 0x000E0734u, {o}) == 0) {
        ret(ctx, 0u);
        return;
    }
    if (static_cast<s32>(VCall(ctx, o, 32, {o, 0})) == 1 && static_cast<s32>(VCall(ctx, o, 32, {o, 1})) == 1 &&
        call(ctx, 0x000D8760u, {o, 0, 0}) != 0) {
        const f64 k = Lfs42(0x00228E30u);
        store<u32>(o + 348, load<u32>(0x00228E30u));
        ctx.fpr[1] = k;
        VCall(ctx, o, 28, {o, 1, 3});
    }
    const f64 v = Lfs42(o + 348);
    ctx.fpr[1] = v;
    const f64 k = Lfs42(0x00228E30u);
    ret(ctx, (v < k || v == k) ? 0u : 1u);
}
