// Game code, part 37: the small-function sweep continued from the last 73
// instruction functions (size order; obj36*.cpp did 70-73). Each was
// checked against the original in the PPC simulator (memory writes, calls
// and their arguments, r3 and f1).
#include "obj37.h"

using namespace sly4;
using namespace sly4::obj37;

// ---------------------------------------------------------------- sweep: 73 instructions
// void f(T* o): Shape_Reset63778C(o), then the bounds +16..+24 = FLT_MAX
// and +32..+40 = -FLT_MAX, +64 / +68 = 0, bit 31 of +72 set and bit 30
// cleared; the array +76 (u16 capacity 256, +78 / +80 = 0, element size
// +82 = 16, data +84 = 0) gets the default allocator a (+88) and, when
// its capacity (re-read) is not 0, data = a->vt[0](a, cap * (size >> 1),
// 0, 16) (entry / OPD after the arguments); Array_Init(o + 76); +96 =
// the OPD 0x00F2E998, u16 +92 = 0xFFFF. r3 = -1.
DECOMP(00398ED0, Bounds_Init_00398ED0) {
    const u32 o = arg<u32>(ctx, 0);
    call(ctx, 0x0063778Cu, {o});
    constexpr u32 kMax = 0x7F7FFFFFu, kMin = 0xFF7FFFFFu;
    const u32 w72 = load<u32>(o + 72);
    store<u32>(o + 32, kMin);
    store<u32>(o + 16, kMax);
    store<u32>(o + 20, kMax);
    store<u32>(o + 24, kMax);
    store<u32>(o + 36, kMin);
    store<u32>(o + 40, kMin);
    store<u32>(o + 64, 0);
    store<u32>(o + 68, 0);
    store<u32>(o + 72, (w72 | 0x80000000u) & ~0x40000000u);
    store<u16>(o + 76, 256);
    store<u16>(o + 78, 0);
    store<u16>(o + 80, 0);
    store<u16>(o + 82, 16);
    store<u32>(o + 84, 0);
    const u32 a = static_cast<u32>(call(ctx, 0x00550EE8u));
    const u32 cap = load<u16>(o + 76);
    store<u32>(o + 88, a);
    if (cap != 0) {
        const u32 n = cap * ((static_cast<u32>(load<u16>(o + 82)) >> 1) & 0x7FFFu);
        store<u32>(o + 84, static_cast<u32>(VCallEO36(ctx, load<u32>(a), 0, {a, n, 0, 16})));
    }
    call(ctx, 0x00558F38u, {o + 76});
    store<u32>(o + 96, 0x00F2E998u);
    store<u16>(o + 92, 0xFFFF);
    ret(ctx, -1);
}

// u32 f(T* o, u32 key): the object e of the registry (*0x00F8F154)->+16
// for key (PointMap_Find on a stack copy of key, the value read through
// the returned slot); when missing, String_Format("Fail to find the
// associate object (%s) ...", Hash_ToString(key)) (r3 = its). Otherwise
// the link l = 0x003A187C(*0x00F657FC, e->+204 or, when that is 0,
// String_HashNoCase(e->vt[12](e)), e); r3 = 0 without one, else r3 =
// *0x00AE2CA4 - o->+16->vt[12](o->+16), and when that is 0, l->+64 = o.
DECOMP(003A1A84, Registry_LinkObject_003A1A84) {
    const u32 o = arg<u32>(ctx, 0), key = arg<u32>(ctx, 1);
    u32 e;
    {
        Stack fr(ctx, 16);
        store<u32>(fr.addr(), key);
        const u32 slot = static_cast<u32>(call(ctx, 0x0073343Cu, {load<u32>(load<u32>(0x00F8F154u) + 16), fr.addr()}));
        e = load<u32>(slot);
    }
    if (e == 0) {
        const u32 s = static_cast<u32>(call(ctx, 0x0056A6E0u, {key}));
        call(ctx, 0x005534D0u, {0x00ADDFCCu, s});
        return;
    }
    const u32 reg = load<u32>(0x00F657FCu);
    u32 h = load<u32>(e + 204);
    if (h == 0) {
        const u32 nm = static_cast<u32>(VCall(ctx, e, 12, {e}));
        h = static_cast<u32>(call(ctx, 0x0056A710u, {nm}));
    }
    const u32 l = static_cast<u32>(call(ctx, 0x003A187Cu, {reg, h, e}));
    if (l == 0) {
        ret(ctx, 0u);
        return;
    }
    const u32 k = load<u32>(0x00AE2CA4u);
    const u32 p = load<u32>(o + 16);
    const u32 d = k - static_cast<u32>(VCall(ctx, p, 12, {p}));
    if (d == 0) store<u32>(l + 64, o);
    ret(ctx, d);
}

// void f(T* o; f1 = v): for the owner p = o->+104 with bit 0x40000 of
// +172: q = p when bit 2 of p->+192 is set, else 0. When u8 o->+163 is
// set and q->+348 - v < m = q->+344 * 0.05: o->+162 = 1 and q->+348 =
// min(m, q->+344) when that is above 0, else 0 (raw words; r3 =
// 0x003AAF80, f1 = m). Otherwise, with q: q->vt[352](q, 0, 0; f1 = v)
// (OPD in r4, entry in r7). q may be 0 on the compare path (quirk: the
// floats are then read from / written to address 344 / 348). r3 as left.
DECOMP(003AAF88, Obj_ClampRate_003AAF88) {
    const u32 o = arg<u32>(ctx, 0);
    const f64 v = farg(ctx, 0);
    const u32 p = load<u32>(o + 104);
    if (p == 0 || (load<u32>(p + 172) & 0x40000u) == 0) return;
    const u32 q = (load<u32>(p + 192) & 2u) != 0 ? p : 0;
    if (load<u8>(o + 163) != 0) {
        constexpr u32 K = 0x003AAF80u;  // 0.05f, 0.0f
        const f64 d = static_cast<f32>(Lfs32(q + 348) - v);
        const f64 m = static_cast<f32>(Lfs32(q + 344) * Lfs32(K));
        ctx.fpr[1] = m;
        if (d < m) {
            store<u8>(o + 162, 1);
            const f64 a = Lfs32(q + 344);
            const bool lt = m < a;
            const f64 mn = lt ? m : a;
            u32 w;
            if (mn > Lfs32(K + 4)) w = lt ? std::bit_cast<u32>(static_cast<f32>(m)) : load<u32>(q + 344);
            else w = load<u32>(K + 4);
            store<u32>(q + 348, w);
            ctx.gpr[3] = K;
            return;
        }
    }
    if (q == 0) return;
    ctx.fpr[1] = v;
    const u32 opd = load<u32>(load<u32>(q) + 352);
    call_ptr(ctx, opd, {q, opd, 0, 0, load<u32>(opd)});
}

// void f(T* o): when u8 o->+68 is set: World_ListRemove_0046B7A4(
// *0x00F65CA0, 0, 0x00F38708, o, 0), Handle_Notify36742C(0xAEF40D0E, 0,
// 1, 0), SetSelect_491000(o, 0x2A1D0807), o->+220->vt[88](it, 0, 1, 1,
// 1); then with o->+60 set and a set word +0x1281E0 of o->vt[76](o):
// Obj_PopLayer(that, o->+60; f1 = -1). r3 = 0 or the last call's.
DECOMP(0040AA70, Obj_Close_0040AA70) {
    const u32 o = arg<u32>(ctx, 0);
    if (load<u8>(o + 68) == 0) {
        ret(ctx, 0u);
        return;
    }
    call(ctx, 0x0046B7A4u, {load<u32>(0x00F65CA0u), 0, 0x00F38708u, o, 0});
    call(ctx, 0x0036742Cu, {0xAEF40D0Eu, 0, 1, 0});
    call(ctx, 0x00491000u, {o, 0x2A1D0807u});
    const u32 p = load<u32>(o + 220);
    VCall(ctx, p, 88, {p, 0, 1, 1, 1});
    if (load<u32>(o + 60) == 0) {
        ret(ctx, 0u);
        return;
    }
    const u32 g = static_cast<u32>(VCall(ctx, o, 76, {o}));
    const u32 s = load<u32>(g + 0x1281E0u);
    if (s == 0) {
        ret(ctx, 0u);
        return;
    }
    const u32 l = load<u32>(o + 60);
    callf(ctx, 0x000FBB88u, {s, l, l}, {Lfs32(0x0040AA68u)});
}

// void f(T* o, const char* path): loads the xmlb document path (a stack
// XmlDoc); for each top-level node with bit 0x80 of u8 +24 and the hash
// 0x1AF675EA at +20, while bits 7..6 of u8 o->+28 are 0 and the word
// o->+0x128164 is 0: o->+0x128164 = 0x00481A88(&doc, node) when that is
// not 0. A document without nodes gets XmlCursor_DocPath(&doc). Then
// XmlDoc_Destruct(&doc) (r3 = its).
DECOMP(004829F0, World_LoadXmlNodes_004829F0) {
    const u32 o = arg<u32>(ctx, 0), path = arg<u32>(ctx, 1);
    Stack fr(ctx, 2256);
    const u32 doc = fr.addr();
    call(ctx, 0x004FA7A4u, {doc});
    if (static_cast<u32>(call(ctx, 0x004FACCCu, {doc, path, 0})) != 0) {
        u32 c = static_cast<u32>(call(ctx, 0x004F9140u, {doc}));
        if (c == 0) call(ctx, 0x004F8944u, {doc});
        while (c != 0) {
            if ((load<u8>(c + 24) & 0x80u) != 0 && load<u32>(c + 20) == 0x1AF675EAu &&
                ((load<u8>(o + 28) >> 6) & 3u) == 0 && load<u32>(o + 0x128164u) == 0) {
                const u32 r = static_cast<u32>(call(ctx, 0x00481A88u, {doc, c}));
                if (r != 0) store<u32>(o + 0x128164u, r);
            }
            c = static_cast<u32>(call(ctx, 0x004F8D74u, {c}));
        }
    }
    call(ctx, 0x004FA9C0u, {doc});
}

// void f(T* o): the deleting ~T() of 0x00753258: nothing for o = 0 (r3 =
// 0); else DtorB160A8_34(o) and SmallFree(o, 608) (r3 = its).
DECOMP(00753360, VtB160A8_Delete_00753360) {
    const u32 o = arg<u32>(ctx, 0);
    if (o == 0) return;
    DtorB160A8_34(ctx, o);
    call(ctx, 0x00552420u, {o, 608});
}

// ---------------------------------------------------------------- sweep: 74 instructions
// void f(Vec3* out, -, const Vec3* a, const Vec3* b, const Vec3* p): the
// point of the segment a-b closest to p (SegClosest37, constants
// 0x00013E34).
DECOMP(00013E40, Seg_ClosestPoint_00013E40) {
    SegClosest37(ctx, arg<u32>(ctx, 0), arg<u32>(ctx, 2), arg<u32>(ctx, 3), arg<u32>(ctx, 4), 0x00013E34u);
}

// void f(T* o): o->+400 (when set) gets vt[304](it, 0); the object
// 0x8BD1C3E8 (Obj_FindById), when found, gets its +160->vt[64](; f1 =
// 40) and vt[88](it, 1, 1, 1, 1); then the object 0x19F541C3 the same
// vt[88]. r3 = 0 (not found) or that call's.
DECOMP(000495A0, Obj_ShowPair_000495A0) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 p = load<u32>(o + 400);
    if (p != 0) VCall(ctx, p, 304, {p, 0});
    const u32 q = static_cast<u32>(call(ctx, 0x000D6A28u, {0x8BD1C3E8u}));
    if (q != 0) {
        const u32 w = load<u32>(q + 160);
        ctx.fpr[1] = Lfs32(0x00049598u);
        VCall(ctx, w, 64, {w});
        VCall(ctx, q, 88, {q, 1, 1, 1, 1});
    }
    const u32 s = static_cast<u32>(call(ctx, 0x000D6A28u, {0x19F541C3u}));
    if (s == 0) {
        ret(ctx, 0u);
        return;
    }
    VCall(ctx, s, 88, {s, 1, 1, 1, 1});
}

// void f(Def* o, X* x): the target t = o->+24, replaced by c->vt[24](c)
// when that is not 0 for the component c of x named o->+28 (when set)
// whose vt[32]() is 2; a new 40-byte command (SmallAlloc; 0 on failure)
// VtB211A8_Ctor_00437A1C(m, o->+12, o->+16, o->+20, t, -, u8 o->+37,
// u8 o->+36; f1 = o->+32) goes to CmdList_Push_0042C138 (r3 = its). The
// float's register slot r8 is passed as 0 (the original leaves garbage).
DECOMP(000A38C4, Cmd_NewB211A8_000A38C4) {
    const u32 o = arg<u32>(ctx, 0), x = arg<u32>(ctx, 1);
    u32 t = load<u32>(o + 24);
    const u32 k = load<u32>(o + 28);
    if (k != 0) {
        const u32 c = static_cast<u32>(call(ctx, 0x004BB4B4u, {x, k}));
        if (c != 0 && static_cast<s32>(VCall(ctx, c, 32, {c})) == 2) {
            const u32 r = static_cast<u32>(VCall(ctx, c, 24, {c}));
            if (r != 0) t = r;
        }
    }
    const u32 m = static_cast<u32>(call(ctx, 0x0055238Cu, {40}));
    u32 cmd = 0;
    if (m != 0) {
        const u32 a12 = load<u32>(o + 12), a16 = load<u32>(o + 16), a20 = load<u32>(o + 20);
        callf(ctx, 0x00437A1Cu, {m, a12, a16, a20, t, 0, load<u8>(o + 37), load<u8>(o + 36)}, {Lfs32(o + 32)});
        cmd = m;
    }
    call(ctx, 0x0042C138u, {cmd});
}

// T* f(T* o, a): constructor: 0x000E9DD8(o, a), vtables 0x00B13528 / +380
// 0x00B136C8 / +384 0x00B136F0, +628 / +632 / +636 = 0, u8 +640 = 1,
// Ctor_B21B88_004A0DB0 of +644 and +680, the vector 0x01164B40 copied
// (raw) to +720 and +736 (f1 = its z). With the spline key k = o->+216->
// +404: +632 = the spline of (*0x00F8F158)->+16 for k (SplineMap_Find on
// a stack copy) and Obj_SetMode_000F498C(o, 2); a missing one gets
// String_Format("Can't find spline path '%u'for EntityProjectile...",
// k). r3 = 0 (no key) or the last call's.
DECOMP(000F4A08, Projectile_Ctor_000F4A08) {
    const u32 o = arg<u32>(ctx, 0);
    call(ctx, 0x000E9DD8u, {o, arg<u32>(ctx, 1)});  // the base constructor reads r4 too (passed through)
    constexpr u32 vt = 0x00B13520u;
    store<u32>(o, vt + 8);
    store<u32>(o + 628, 0);
    store<u32>(o + 380, vt + 424);
    store<u32>(o + 384, vt + 464);
    store<u32>(o + 632, 0);
    store<u32>(o + 636, 0);
    store<u8>(o + 640, 1);
    call(ctx, 0x004A0DB0u, {o + 644});
    call(ctx, 0x004A0DB0u, {o + 680});
    constexpr u32 g = 0x01164B40u;
    store<u32>(o + 720, load<u32>(g));
    store<u32>(o + 724, load<u32>(g + 4));
    const u32 d = load<u32>(o + 216);
    store<u32>(o + 728, load<u32>(g + 8));
    store<u32>(o + 736, load<u32>(g));
    store<u32>(o + 740, load<u32>(g + 4));
    const u32 key = load<u32>(d + 404);
    store<u32>(o + 744, load<u32>(g + 8));
    ctx.fpr[1] = Lfs32(g + 8);
    if (key == 0) {
        ret(ctx, 0u);
        return;
    }
    u32 s;
    {
        Stack fr(ctx, 16);
        store<u32>(fr.addr(), key);
        const u32 slot = static_cast<u32>(call(ctx, 0x007335C4u, {load<u32>(load<u32>(0x00F8F158u) + 16), fr.addr()}));
        s = load<u32>(slot);
    }
    store<u32>(o + 632, s);
    if (s == 0) {
        call(ctx, 0x005534D0u, {0x00AC6C04u, load<u32>(d + 404)});
        return;
    }
    call(ctx, 0x000F498Cu, {o, 2});
}

// void f(Vec3* out, const Vec3* a, const Vec3* b, const Vec3* p): the
// point of the segment a-b closest to p (SegClosest37, constants
// 0x000FD194).
DECOMP(000FD1A0, Seg_ClosestPoint_000FD1A0) {
    SegClosest37(ctx, arg<u32>(ctx, 0), arg<u32>(ctx, 1), arg<u32>(ctx, 2), arg<u32>(ctx, 3), 0x000FD194u);
}

// void f(T* o): the debug text "Target: %s\nTgt Spline: %s\nPos Spline:
// %s\nT: ..." (String_Format) with o->+12->vt[76](), o->+24->vt[12]()
// (each "<none>" when the object is 0), o->+20->vt[12]() and the floats
// +96, +144, +148 (as doubles in f1-f3 and r7-r9). r3 = its.
DECOMP(0010B574, Spline_DebugText_0010B574) {
    const u32 o = arg<u32>(ctx, 0);
    constexpr u32 kNone = 0x00AC6DA4u;
    u32 a = load<u32>(o + 12);
    a = a != 0 ? static_cast<u32>(VCall(ctx, a, 76, {a})) : kNone;
    u32 b = load<u32>(o + 24);
    b = b != 0 ? static_cast<u32>(VCall(ctx, b, 12, {b})) : kNone;
    const u32 c = load<u32>(o + 20);
    const u32 s = static_cast<u32>(VCall(ctx, c, 12, {c}));
    const f64 f1 = Lfs32(o + 96), f2 = Lfs32(o + 144), f3 = Lfs32(o + 148);
    const u64 d1 = std::bit_cast<u64>(f1), d2 = std::bit_cast<u64>(f2), d3 = std::bit_cast<u64>(f3);
    callf(ctx, 0x005534D0u, {0x00AC6DACu, a, b, s, d1, d2, d3}, {f1, f2, f3});
}

// void f(Timer* o, T* p): counts o->+8 down. At 0: +8 = 0, bit 0x02000000
// of p->+656 cleared, and an effect e = o->+4 gets bit 0x200 of +136 set,
// 0x400 cleared, Effect_SetHash4CF988(e, 0x879F2BDA) and o->+4 = 0.
// Otherwise +8 = n, p->+668 + 1, bit 0x02000000 of p->+656 set, p->+688 =
// 1 / n (f1), and e gets 0x200 cleared, 0x400 set and
// Effect_SetHash4CF988(e, 0x00011773). r3 = 0 (no effect) or its.
DECOMP(0016EB20, Timer_Step_0016EB20) {
    const u32 o = arg<u32>(ctx, 0), p = arg<u32>(ctx, 1);
    const u32 n = load<u32>(o + 8) - 1u;
    if (n == 0) {
        store<u32>(o + 8, 0);
        store<u32>(p + 656, load<u32>(p + 656) & ~0x02000000u);
        u32 e = load<u32>(o + 4);
        if (e == 0) {
            ret(ctx, 0u);
            return;
        }
        store<u32>(e + 136, load<u32>(e + 136) | 0x200u);
        e = load<u32>(o + 4);
        store<u32>(e + 136, load<u32>(e + 136) & ~0x400u);
        call(ctx, 0x004CF988u, {load<u32>(o + 4), 0x879F2BDAu});
        store<u32>(o + 4, 0);
        return;
    }
    const f64 k = Lfs32(0x0016EB18u);
    const f64 fn = static_cast<f32>(static_cast<f64>(n));
    store<u32>(o + 8, n);
    const u32 w = load<u32>(p + 656);
    const u32 c = load<u32>(p + 668);
    store<u32>(p + 668, c + 1u);
    store<u32>(p + 656, w | 0x02000000u);
    const f64 r = static_cast<f32>(k / fn);
    Stfs32(p + 688, r);
    ctx.fpr[1] = r;
    u32 e = load<u32>(o + 4);
    if (e == 0) {
        ret(ctx, 0u);
        return;
    }
    store<u32>(e + 136, load<u32>(e + 136) & ~0x200u);
    e = load<u32>(o + 4);
    store<u32>(e + 136, load<u32>(e + 136) | 0x400u);
    call(ctx, 0x004CF988u, {load<u32>(o + 4), 0x00011773u});
}

// void f(T* o, bool on): with on: o->vt[396](o, 4, 0); a Havok object h
// = o->+416 has each of its 0x0091A2CC(h) parts 0x0091A7D8(h, i)
// released (AnimSkel_Release7D4EA8), its properties 0xA734004D and
// 0x91C7CF5B removed (Hk_RemoveProperty93FED8 with stack results), is
// released and +416 = 0; +428 = 0. Then Obj_SetRagdollBase(o, on) (r3 =
// its).
DECOMP(001FAE1C, Obj_SetRagdoll_001FAE1C) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 on = arg<u32>(ctx, 1) & 0xFFu;
    if (on != 0) {
        VCall(ctx, o, 396, {o, 4, 0});
        if (load<u32>(o + 416) != 0) {
            for (s32 i = 0;; ++i) {
                const s32 n = static_cast<s32>(call(ctx, 0x0091A2CCu, {load<u32>(o + 416)}));
                if (!(i < n)) break;
                const u32 x = static_cast<u32>(call(ctx, 0x0091A7D8u, {load<u32>(o + 416), Sx33(static_cast<u32>(i))}));
                call(ctx, 0x007D4EA8u, {x});
            }
            {
                Stack fr(ctx, 16);
                call(ctx, 0x0093FED8u, {fr.addr(), load<u32>(o + 416), 0xA734004Du});
                call(ctx, 0x0093FED8u, {fr.addr() + 8, load<u32>(o + 416), 0x91C7CF5Bu});
            }
            call(ctx, 0x007D4EA8u, {load<u32>(o + 416)});
            store<u32>(o + 416, 0);
        }
        store<u32>(o + 428, 0);
    }
    call(ctx, 0x000E02F8u, {o, on});
}

// bool f(Area* o, T* m): false unless Area_Enter(o, m). When u8 o->+444
// is 0 or bit 0x00100000 of m->+188 is set: with o->+436 set and
// 0x004ADD68(it, m), the callback o->+440 (when set) gets (o, m) (OPD in
// r5, entry in r6). Then the effect e = o->+432 (when set): its ref
// 0x1ABD6576 cleared when Effect_GetParam_004A66D0 gives m, the ref
// 0x2C73353E = m->+160 and Effect_SetHash4CF988(e, 0xDC175839). True.
DECOMP(00224E20, Area_OnEnter_00224E20) {
    const u32 o = arg<u32>(ctx, 0), m = arg<u32>(ctx, 1);
    if (static_cast<u32>(call(ctx, 0x0020E7A4u, {o, m})) == 0) {
        ret(ctx, 0u);
        return;
    }
    if (load<u8>(o + 444) == 0 || (load<u32>(m + 188) & 0x00100000u) != 0) {
        const u32 q = load<u32>(o + 436);
        if (q != 0 && static_cast<u32>(call(ctx, 0x004ADD68u, {q, m})) != 0) {
            const u32 cb = load<u32>(o + 440);
            if (cb != 0) call_ptr(ctx, cb, {o, m, cb, load<u32>(cb)});
        }
    }
    const u32 e = load<u32>(o + 432);
    if (e != 0) {
        if (static_cast<u32>(call(ctx, 0x004A66D0u, {e, 0x1ABD6576u})) == m)
            call(ctx, 0x004A65F0u, {load<u32>(o + 432), 0x1ABD6576u, 0});
        call(ctx, 0x004A65F0u, {load<u32>(o + 432), 0x2C73353Eu, load<u32>(m + 160)});
        call(ctx, 0x004CF988u, {load<u32>(o + 432), 0xDC175839u});
    }
    ret(ctx, 1u);
}

// void f(T* o, Hit* h): unless the flag byte 0x010388F0 is set, bit
// 0x20000 or 0x40000 of o->+188 is set, or o's owner (Obj_Owner0D93FC)
// has bit 0x1000 of +40: when o->+380 <= 0 (the constant 0x002291EC),
// Obj_TestE0BCC(o, h) and o->+216->+212 > 0: o->vt[352](o, h->+4,
// 0x045D9554; f1 = h->+64 * that) (OPD in r4, entry in r7), and bit 0x100
// of u16 h->+14 = (its result is 0). r3 / f1 as left.
DECOMP(002291F0, Obj_HitReact_002291F0) {
    const u32 o = arg<u32>(ctx, 0), h = arg<u32>(ctx, 1);
    const u32 fl = load<u8>(0x010388F0u);
    if (fl != 0) {
        ret(ctx, fl);
        return;
    }
    const u32 w = load<u32>(o + 188);
    if ((w & 0x20000u) != 0) {
        ret(ctx, w);
        return;
    }
    if ((w & 0x40000u) != 0) {
        ret(ctx, 0x40000u);
        return;
    }
    if (static_cast<u32>(call(ctx, 0x000D93FCu, {o})) != 0) {
        const u32 ow = static_cast<u32>(call(ctx, 0x000D93FCu, {o}));
        const u32 b = (load<u32>(ow + 40) >> 12) & 1u;
        if (b != 0) {
            ret(ctx, b);
            return;
        }
    }
    constexpr u32 K = 0x002291ECu;
    const f64 z = Lfs32(K);
    ctx.fpr[1] = Lfs32(o + 380);
    if (ctx.fpr[1] > z) {
        ret(ctx, K);
        return;
    }
    if (static_cast<u32>(call(ctx, 0x000E0BCCu, {o, h})) == 0) return;
    const u32 d = load<u32>(o + 216);
    const f64 v = Lfs32(d + 212);
    ctx.fpr[1] = v;
    if (!(v > z)) {
        ret(ctx, d);
        return;
    }
    ctx.fpr[1] = static_cast<f32>(Lfs32(h + 64) * v);
    const u32 opd = load<u32>(load<u32>(o) + 352);
    const u32 r = static_cast<u32>(call_ptr(ctx, opd, {o, opd, load<u32>(h + 4), 0x045D9554u, load<u32>(opd)}));
    const u32 lz = static_cast<u32>(std::countl_zero(r));
    store<u16>(h + 14, static_cast<u16>((load<u16>(h + 14) & ~0x100u) | ((lz << 3) & 0x100u)));
    ret(ctx, lz);
}

// void f(T* a, Node* n, Slot* s, bool on): with on, for a node n->+24 of
// kind 8 (bits 15..10 of its +4) and an empty s->+0: x =
// Node_FindFlagged_00257828(a, it); a new material (Material_Create(
// Shader_Find(0x0FD7FF3E)), copied from x) to s->+4 when that is 0, else
// x->+128's 152 bytes copied over s->+4->+128 (when both are set); s->+0
// = n. Without on, a set s->+0 is cleared and Node_FindFlagged_00257828(
// a, n->+24)->+128 gets the 152 bytes of s->+4->+128 back. r3 as left.
DECOMP(00257910, Node_SwapMaterial_00257910) {
    const u32 a = arg<u32>(ctx, 0), n = arg<u32>(ctx, 1), s = arg<u32>(ctx, 2);
    if ((arg<u32>(ctx, 3) & 0xFFu) == 0) {
        if (load<u32>(s) == 0) {
            ret(ctx, 0u);
            return;
        }
        store<u32>(s, 0);
        const u32 x = static_cast<u32>(call(ctx, 0x00257828u, {a, load<u32>(n + 24)}));
        call(ctx, 0x0071C3F0u, {load<u32>(x + 128), load<u32>(load<u32>(s + 4) + 128), 152});
        return;
    }
    const u32 c = load<u32>(n + 24);
    ctx.gpr[3] = c;
    if (((load<u32>(c + 4) >> 10) & 0x3Fu) != 8 || load<u32>(s) != 0) return;
    const u32 x = static_cast<u32>(call(ctx, 0x00257828u, {a, c}));
    const u32 cur = load<u32>(s + 4);
    if (cur == 0) {
        const u32 sh = static_cast<u32>(call(ctx, 0x004FEBD0u, {0x0FD7FF3Eu}));
        const u32 m = static_cast<u32>(call(ctx, 0x004FF43Cu, {sh}));
        call(ctx, 0x00509090u, {m, x});
        store<u32>(s + 4, m);
    } else {
        const u32 src = load<u32>(x + 128);
        ctx.gpr[3] = cur;
        if (src != 0) {
            const u32 dst = load<u32>(cur + 128);
            ctx.gpr[3] = dst;
            if (dst != 0) call(ctx, 0x0071C3F0u, {dst, src, 152});
        }
    }
    store<u32>(s, n);
}

// f32 f(T* o, const Vec3* v): the angle (Acosf) between o's direction
// +208..+216 and the unit vector from the world's player position (the
// object +0x1281E0 of *0x00F65CA0, its +144) to v, the dot clamped to
// [-1, 1] through fsel; 0 without a player or when the squared distance
// is <= 0.001 (r3 = 0x002C71C8 then).
DECOMP(002C71F0, Obj_AngleTo_002C71F0) {
    const u32 o = arg<u32>(ctx, 0), v = arg<u32>(ctx, 1);
    constexpr u32 K = 0x002C71C8u;  // 0, 0.001, 1, -, (f64) 1, (f32) -1, -, (f64) -1
    auto f = [](f64 x) { return static_cast<f64>(static_cast<f32>(x)); };
    const u32 e = load<u32>(load<u32>(0x00F65CA0u) + 0x1281E0u);
    if (e == 0) {
        ctx.fpr[1] = Lfs32(K);
        ret(ctx, K);
        return;
    }
    const u32 c = e + 144;
    const f64 dy = f(Lfs32(v + 4) - Lfs32(c + 4));
    const f64 dx = f(Lfs32(v) - Lfs32(c));
    const f64 dz = f(Lfs32(v + 8) - Lfs32(c + 8));
    f64 d2 = f(dy * dy);
    d2 = f(std::fma(dx, dx, d2));
    d2 = f(std::fma(dz, dz, d2));
    if (d2 < Lfs32(K + 4) || d2 == Lfs32(K + 4)) {
        ctx.fpr[1] = Lfs32(K);
        ret(ctx, K);
        return;
    }
    const f64 one = Lfs32(K + 8);
    const f64 dp1 = std::bit_cast<f64>(static_cast<u64>(load<u32>(K + 16)) << 32 | load<u32>(K + 20));
    const f64 dm1 = std::bit_cast<f64>(static_cast<u64>(load<u32>(K + 32)) << 32 | load<u32>(K + 36));
    const f64 inv = f(one / f(std::sqrt(d2)));
    const f64 uy = f(inv * dy), ux = f(inv * dx), uz = f(inv * dz);
    f64 t = f(uy * Lfs32(o + 212));
    t = f(std::fma(ux, Lfs32(o + 208), t));
    t = f(std::fma(uz, Lfs32(o + 216), t));
    t = f(Fsel33(f(one - t), t, dp1));
    t = f(Fsel33(f(t - Lfs32(K + 24)), t, dm1));
    t = f(Fsel33(f(t + one), t, dm1));
    t = f(Fsel33(f(t - one), dp1, t));
    callf(ctx, 0x005E649Cu, {}, {t});
}

// void f(T* o): resets o's state block: u8 +720 = 0, +724..+744 = 0,
// +812 = 0, u8 +820 = 0, +816 = -1.0f, u8 +822 = 0, u8 +821 = the low
// byte of *0x00F64C24, u8 +823..+825 = 255, +716 / +832 = 0, the +72
// floats of the five 80-byte records from 0x01068508 to +836..+852
// (raw; f1 = the last), u8 +826 / +827 / +829 = 0, u8 +828 = 1, the 16
// words +748..+808 = 0 and the bytes +0, +65, +130, +195, +324, +453,
// +582, +711 = 0 (each in the original's order). r3 = o.
DECOMP(002DA734, State_Reset_002DA734) {
    const u32 o = arg<u32>(ctx, 0);
    constexpr u32 R = 0x01068508u + 72u;  // record i's float at R + 80 * i
    store<u8>(o + 720, 0);
    for (u32 k = 724; k <= 744; k += 4) store<u32>(o + k, 0);
    store<u32>(o + 812, 0);
    store<u8>(o + 820, 0);
    store<u32>(o + 816, 0xBF800000u);
    const u32 b = load<u32>(0x00F64C24u);
    store<u8>(o + 822, 0);
    store<u8>(o + 821, static_cast<u8>(b));
    store<u8>(o + 823, 255);
    store<u8>(o + 824, 255);
    store<u8>(o + 825, 255);
    store<u32>(o + 716, 0);
    store<u32>(o + 832, 0);
    store<u8>(o + 826, 0);
    store<u32>(o + 836, load<u32>(R));
    store<u8>(o + 827, 0);
    store<u32>(o + 840, load<u32>(R + 80));
    store<u8>(o + 828, 1);
    store<u8>(o + 829, 0);
    store<u32>(o + 844, load<u32>(R + 160));
    store<u32>(o + 848, load<u32>(R + 240));
    const u32 last = load<u32>(R + 320);
    store<u32>(o + 852, last);
    for (u32 k = 0; k < 16; ++k) store<u32>(o + 748 + 4 * k, 0);
    for (u32 off : {0u, 65u, 130u, 195u, 324u, 453u, 582u, 711u}) store<u8>(o + off, 0);
    ctx.fpr[1] = static_cast<f64>(std::bit_cast<f32>(last));
}

// void f(T* o, bool release): for each 20-byte record e of the vector
// o->+2272 (data +0, u16 count +8): the top two bits of e->+8 cleared,
// then (unless its low byte is 0 while u8 o->+18 is not 5) with u8
// e->+10 set they become 2 (u8 e->+9 set) or 1; u8 +11 / +12 and the
// bytes +16..+19 = 0. With release and an effect o->+2224:
// Effect_Release(it, 0x00010CBB), its +136 gets bit 0x200 and +2224 = 0.
// r3 = 0 or that effect.
DECOMP(00325A30, Recs_Reset_00325A30) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 rel = arg<u32>(ctx, 1) & 0xFFu;
    for (u32 i = 0, off = 0;; ++i, off += 20) {
        const u32 v = load<u32>(o + 2272);
        const u32 n = v != 0 ? load<u16>(v + 8) : 0u;
        if (!(i < n)) break;
        const u32 e = load<u32>(v) + off;
        const bool five = load<u8>(o + 18) == 5;
        const u32 w = load<u32>(e + 8);
        store<u32>(e + 8, w & 0x3FFFFFFFu);
        if (!((w & 0xFFu) == 0 && !five) && load<u8>(e + 10) != 0) {
            const u32 st = load<u8>(e + 9) != 0 ? 2u : 1u;  // the 2-bit state in bits 31..30
            store<u32>(e + 8, (load<u32>(e + 8) & 0x3FFFFFFFu) | (st << 30));
        }
        store<u8>(e + 11, 0);
        store<u8>(e + 12, 0);
        for (u32 k = 0; k < 4; ++k) store<u8>(e + 16 + k, 0);
    }
    if (rel == 0 || load<u32>(o + 2224) == 0) {
        ret(ctx, 0u);
        return;
    }
    call(ctx, 0x004CF3C4u, {load<u32>(o + 2224), 0x00010CBBu});
    const u32 e = load<u32>(o + 2224);
    store<u32>(e + 136, load<u32>(e + 136) | 0x200u);
    store<u32>(o + 2224, 0);
    ret(ctx, e);
}

// bool f(T* o, Pad* x): the direction o->+2580 (-1: false) as a channel
// test: 7 / 5 / 3 / 1 mean channel 0 / 1 / 2 / 3 (Obj_GetChannel_0045B160,
// f1) above o->+2584, 6 / 4 / 2 / 0 the same channel below -o->+2584;
// other values test the constant 0 (0x00340E9C) against +2584.
DECOMP(00340EA0, Pad_DirActive_00340EA0) {
    const u32 o = arg<u32>(ctx, 0), x = arg<u32>(ctx, 1);
    const s32 k = load<s32>(o + 2580);
    if (k == -1) {
        ret(ctx, 0u);
        return;
    }
    ctx.fpr[1] = Lfs32(0x00340E9Cu);
    bool pos = true;
    if (k >= 0 && k <= 7) {
        pos = (k & 1) != 0;
        call(ctx, 0x0045B160u, {x, static_cast<u32>(3 - (k >> 1))});
    }
    const f64 v = ctx.fpr[1];
    if (pos) ret(ctx, v > Lfs32(o + 2584) ? 1u : 0u);
    else ret(ctx, v < -Lfs32(o + 2584) ? 1u : 0u);
}

// void f(T* o, bool paused): Obj_SetPausedBit_00490F88(o, paused); the
// animation o->+244 gets Obj_SetAnim(it, 0, 1, it, 0, 0, 0x01164CA4; f1
// = -1) when paused, else Obj_Dispatch362(it, 1, 1, 0); then the child
// c = o->+248 (when set) gets bit 31 of +184 and vt[24](c, 1, 0) when
// paused, else vt[24](c, 0, 0). r3 as left.
DECOMP(003A9160, Obj_SetPaused_003A9160) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 on = arg<u32>(ctx, 1) & 0xFFu;
    call(ctx, 0x00490F88u, {o, on});
    if (on != 0) {
        const u32 a = load<u32>(o + 244);
        callf(ctx, 0x00363A3Cu, {a, 0, 1, a, 0, 0, 0x01164CA4u}, {Lfs32(0x003A915Cu)});
        const u32 c = load<u32>(o + 248);
        if (c == 0) {
            ret(ctx, 0u);
            return;
        }
        store<u32>(c + 184, load<u32>(c + 184) | 0x80000000u);
        const u32 c2 = load<u32>(o + 248);
        VCall(ctx, c2, 24, {c2, 1, 0});
        return;
    }
    call(ctx, 0x0036325Cu, {load<u32>(o + 244), 1, 1, 0});
    const u32 c = load<u32>(o + 248);
    if (c == 0) return;
    VCall(ctx, c, 24, {c, 0, 0});
}

// Static init: the words 0x0108A100..+8 cleared and the five tuning
// floats "Microgame.ReturnTreasure.{PosInterpTime, ScaleInterpTime,
// TickerInfoTime, PosMoveYOffset, CountDownStartTime}" at 0x01089F70 +
// 80 * i (TuningVar_ConstructBase_004FBFA8, vtable 0x00B0D99C, value +72
// = 1.5, 1.5, 10, 2, 10). r3 = the last call's.
DECOMP(00426BB8, ReturnTreasure_InitTuning_00426BB8) {
    constexpr u32 W = 0x0108A100u, T = 0x01089F70u, vt = 0x00B0D99Cu;
    store<u32>(W, 0);
    store<u32>(W + 4, 0);
    store<u32>(W + 8, 0);
    call(ctx, 0x004FBFA8u, {T, 0x00AE2058u});
    store<u32>(T + 72, 0x3FC00000u);
    store<u32>(T, vt);
    call(ctx, 0x004FBFA8u, {T + 80, 0x00AE2080u});
    store<u32>(T + 80, vt);
    store<u32>(T + 80 + 72, 0x3FC00000u);
    call(ctx, 0x004FBFA8u, {T + 160, 0x00AE20B0u});
    store<u32>(T + 160, vt);
    store<u32>(T + 160 + 72, 0x41200000u);
    call(ctx, 0x004FBFA8u, {T + 240, 0x00AE20D8u});
    store<u32>(T + 240, vt);
    store<u32>(T + 240 + 72, 0x40000000u);
    call(ctx, 0x004FBFA8u, {T + 320, 0x00AE2100u});
    store<u32>(T + 320, vt);
    store<u32>(T + 320 + 72, 0x41200000u);
}

// bool f(u32 id, const Vec3* v, -, w; f1 = s): with the system enabled
// (u8 0x010AEC60) and the handle id (low 16 bits a serial, high 16 an
// index below *0x010B0218) naming a live 272-byte slot of *0x010B021C
// (its word +256: serial in bits 30..15, bit 31 clear): q = slot + 16
// gets the position v (raw, and u8 +192 = 1) unless every axis is within
// 1e-5 of q->+176..+184; then u8 +132 = 1, +80 = s, +128 = w and +76 =
// s * q->+84 * q->+88 (f1). r3 = 0 when disabled, else 1.
DECOMP(0044C758, Voice_SetPos_0044C758) {
    const u32 id = arg<u32>(ctx, 0), v = arg<u32>(ctx, 1), w = arg<u32>(ctx, 3);
    const f64 s = farg(ctx, 0);
    if (load<u8>(0x010AEC60u) == 0) {
        ret(ctx, 0u);
        return;
    }
    const u32 lo = id & 0xFFFFu, hi = id >> 16;
    ret(ctx, 1u);
    if (lo == 0) return;
    constexpr u32 T = 0x010B0200u;
    if (!(static_cast<s32>(hi) < load<s32>(T + 24))) return;
    const u32 e = load<u32>(T + 28) + (hi << 8) + (hi << 4);
    if (((load<u32>(e + 256) >> 15) & 0xFFFFu) != lo) return;
    if ((load<u8>(e + 256) & 0x80u) != 0) return;
    const u32 q = e + 16;
    const f64 K = Lfs32(0x0044C754u);
    auto near = [&](u32 off, u32 voff) {
        return std::fabs(static_cast<f64>(static_cast<f32>(Lfs32(q + off) - Lfs32(v + voff)))) < K;
    };
    if (!(near(176, 0) && near(180, 4) && near(184, 8))) {
        store<u32>(q + 176, load<u32>(v));
        store<u32>(q + 180, load<u32>(v + 4));
        const u32 z = load<u32>(v + 8);
        store<u8>(q + 192, 1);
        store<u32>(q + 184, z);
    }
    const f64 a = Lfs32(q + 84), b = Lfs32(q + 88);
    const f64 m = static_cast<f32>(s * a);
    store<u8>(q + 132, 1);
    Stfs32(q + 80, s);
    store<u32>(q + 128, w);
    const f64 r = static_cast<f32>(m * b);
    Stfs32(q + 76, r);
    ctx.fpr[1] = r;
}

// void f(): starts the current world's level: the stage record s =
// W->+20 + u16 W->+6 (W = *0x00F65CA0); when s->+72 is the current
// session part's (Session_CurrentPart2DDD24(*0x00F64C20))->+744 and the
// session's +40 is set, Session_ResetPart2E5060(session) first;
// World_StartLevel(W, s->+72, 0), W->+0x128160 = s->+76, and with u8
// s->+88 an empty name W->+0x12811C gets 64 bytes of W->+0x1280F0->+216
// + 56 (String_Copy). r3 as left.
DECOMP(0048A144, World_StartStage_0048A144) {
    constexpr u32 kW = 0x00F65CA0u, kS = 0x00F64C20u;
    u32 W = load<u32>(kW);
    const u32 s = load<u32>(W + 20) + load<u16>(W + 6);
    u32 lv = load<u32>(s + 72);
    if (lv != 0) {
        const u32 p = static_cast<u32>(call(ctx, 0x002DDD24u, {load<u32>(kS)}));
        lv = load<u32>(s + 72);
        const u32 ses = load<u32>(kS);
        const u32 cur = p != 0 ? load<u32>(p + 744) : 0u;
        if (cur == lv && load<u32>(ses + 40) != 0) {
            call(ctx, 0x002E5060u, {ses});
            lv = load<u32>(s + 72);
        }
    }
    call(ctx, 0x004674C4u, {load<u32>(kW), lv, 0});
    W = load<u32>(kW);
    store<u32>(W + 0x128160u, load<u32>(s + 76));
    if (load<u8>(s + 88) == 0) {
        ret(ctx, 0x128160u);
        return;
    }
    W = load<u32>(kW);
    ctx.gpr[3] = W;
    if (load<u8>(W + 0x12811Cu) != 0) return;
    call(ctx, 0x00553684u, {W + 0x12811Cu, load<u32>(load<u32>(W + 0x1280F0u) + 216) + 56, 64});
}

// void f(u32 owner, u32 key): for each entry e of the list *0x00F668A0
// (linked by +28) whose (e->+4)->+36 is owner (when owner is set) and
// whose +8 is key (when key is set): each object of its chain +24
// (linked by +16) gets vt[4](), +24 / +16 / +20 = 0, and when u8 e->+12
// is set e->vt[4](e). r3 as the original leaves it.
DECOMP(004E3040, Timers_RemoveMatching_004E3040) {
    const u32 a = arg<u32>(ctx, 0), b = arg<u32>(ctx, 1);
    u64 r3 = ctx.gpr[3];
    for (u32 e = load<u32>(0x00F668A0u); e != 0;) {
        const u32 nx = load<u32>(e + 28);
        bool hit = true;
        if (a != 0) {
            r3 = load<u32>(load<u32>(e + 4) + 36);
            hit = static_cast<u32>(r3) == a;
        }
        if (hit && b != 0) {
            r3 = load<u32>(e + 8);
            hit = static_cast<u32>(r3) == b;
        }
        if (hit) {
            u32 p = load<u32>(e + 24);
            while (p != 0) {
                const u32 n = load<u32>(p + 16);
                VCall(ctx, p, 4, {p});
                p = n;
            }
            store<u32>(e + 24, 0);
            store<u32>(e + 16, 0);
            store<u32>(e + 20, 0);
            r3 = 0;
            if (load<u8>(e + 12) != 0) r3 = VCall(ctx, e, 4, {e});
        }
        e = nx;
    }
    ctx.gpr[3] = r3;
}

// bool f(XmlCursor* x): PushAttrStr37 of the attribute "name" onto the
// settings' vector +456.
DECOMP(004EE45C, Settings_PushName_004EE45C) { PushAttrStr37(ctx, 0x00AF0120u, 456); }

// bool f(XmlCursor* x): PushAttrStr37 of the attribute "include" onto the
// settings' vector +276.
DECOMP(004EEE68, Settings_PushInclude_004EEE68) { PushAttrStr37(ctx, 0x00AF0128u, 276); }

// ~T() body: vtable 0x00B11E40, then the pointer vectors +496, +348,
// +300, +252, +232, +212, +192, +148 and +128 cleared (PtrVector_Clear,
// Nop each). r3 = the last call's.
DECOMP(00743BEC, VtB11E40_Clear_00743BEC) {
    const u32 o = arg<u32>(ctx, 0);
    store<u32>(o, 0x00B11E40u);
    for (u32 off : {496u, 348u, 300u, 252u, 232u, 212u, 192u, 148u, 128u}) PvClear32(ctx, o + off);
}

// void f(T* p): the deleting ~T() thunk for the +384 base: for o = p -
// 384 (nothing when that is 0; r3 = p), DtorB160A8_34(o) and SmallFree(o,
// 608) (r3 = its).
DECOMP(00753484, VtB160A8_DeleteThunk384_00753484) {
    const u32 o = arg<u32>(ctx, 0) - 384u;
    if (o == 0) return;
    DtorB160A8_34(ctx, o);
    call(ctx, 0x00552420u, {o, 608});
}

// void f(T* p): the deleting ~T() thunk for the +380 base: for o = p -
// 380 (nothing when that is 0; r3 = p), DtorB160A8_34(o) and SmallFree(o,
// 608) (r3 = its).
DECOMP(007535AC, VtB160A8_DeleteThunk380_007535AC) {
    const u32 o = arg<u32>(ctx, 0) - 380u;
    if (o == 0) return;
    DtorB160A8_34(ctx, o);
    call(ctx, 0x00552420u, {o, 608});
}

// ---------------------------------------------------------------- sweep: 75 instructions
// void f(Ai* o, T* a): o->+308 = a->vt[68](a)->+28; AiCtrl_Bind_00011D08(o,
// a); bit 2 of u8 +304 cleared, u8 +317 = 0, +318 = 1; 0x0048FA54(o->+48,
// h, 1) for 0xCB186A06 and 0x4908BDCF; AiCtrl_RebuildProxy(o); +312 = 0,
// u8 +316 = 0, +324 = 0x002889C9, +320 = (o->+308->+12)[(*0x00F952D4)->
// +64], +28 = 0; o->vt[220](o, 0) (r3 = its).
DECOMP(0003C090, AiCtrl_Reset_0003C090) {
    const u32 o = arg<u32>(ctx, 0), a = arg<u32>(ctx, 1);
    const u32 r = static_cast<u32>(VCall(ctx, a, 68, {a}));
    store<u32>(o + 308, load<u32>(r + 28));
    call(ctx, 0x00011D08u, {o, a});
    const u32 b = load<u8>(o + 304);
    const u32 m = load<u32>(o + 48);
    store<u8>(o + 317, 0);
    store<u8>(o + 304, static_cast<u8>(b & ~2u));
    store<u8>(o + 318, 1);
    call(ctx, 0x0048FA54u, {m, 0xCB186A06u, 1});
    call(ctx, 0x0048FA54u, {load<u32>(o + 48), 0x4908BDCFu, 1});
    call(ctx, 0x0001199Cu, {o});
    store<u32>(o + 312, 0);
    const u32 s = load<u32>(o + 308);
    store<u8>(o + 316, 0);
    const u32 idx = load<u32>(load<u32>(0x00F952D4u) + 64);
    const u32 v = load<u32>(load<u32>(s + 12) + (idx << 2));
    store<u32>(o + 324, 0x002889C9u);
    store<u32>(o + 320, v);
    store<u32>(o + 28, 0);
    VCall(ctx, o, 220, {o, 0});
}

// T* New(-, Xml* x) of 0x0007A7C8 / 0x000859D8: a 28-byte object
// (Mem_Alloc(28, 0, 16); 0 on failure): Obj_InitVt_B21C98(m, x), +12..+24
// = 0, vtable vt; the hashes 0x8A93973F (+12) and 0x7A30D7E3 (+16) read
// (XmlCursor_ReadHash, default 0) and, when either is missing,
// 0x044DDD31 (+20) and h4 (+24). r3 = m.
static void NewHash4_37(PPUContext& ctx, u32 vt, u32 h4) {
    const u32 x = arg<u32>(ctx, 1);
    const u32 m = static_cast<u32>(call(ctx, 0x0054ED38u, {28, 0, 16}));
    if (m == 0) {
        ret(ctx, 0u);
        return;
    }
    call(ctx, 0x004BB3B0u, {m, x});
    store<u32>(m + 12, 0);
    store<u32>(m + 16, 0);
    store<u32>(m + 20, 0);
    store<u32>(m, vt);
    store<u32>(m + 24, 0);
    if (static_cast<u32>(call(ctx, 0x004FA14Cu, {x, 0x8A93973Fu, m + 12, 0})) == 0 ||
        static_cast<u32>(call(ctx, 0x004FA14Cu, {x, 0x7A30D7E3u, m + 16, 0})) == 0) {
        call(ctx, 0x004FA14Cu, {x, 0x044DDD31u, m + 20, 0});
        call(ctx, 0x004FA14Cu, {x, h4, m + 24, 0});
    }
    ret(ctx, m);
}

// T* New(-, Xml* x): NewHash4_37 with vtable 0x00B0F8D8 and 0xAF9EFD80.
DECOMP(0007A7C8, NewB0F8D8_0007A7C8) { NewHash4_37(ctx, 0x00B0F8D8u, 0xAF9EFD80u); }

// T* New(-, Xml* x): NewHash4_37 with vtable 0x00B100A8 and 0xA4871DCB.
DECOMP(000859D8, NewB100A8_000859D8) { NewHash4_37(ctx, 0x00B100A8u, 0xA4871DCBu); }

// T* New(-, Xml* x): a 28-byte object (Mem_Alloc(28, 0, 16); 0 on
// failure): Obj_InitVt_B21C98(m, x), +12..+20 = 0, u8 +24 = 0, vtable
// 0x00B107D8; the hashes 0x7A30D7E3 (+16) and 0x7F247172 (+12) read
// (default 0) and, when either is missing, the s32 0x3683141C (+20) and
// the bool 0xD7718094 (u8 +24) with their current values. r3 = m.
DECOMP(0008D798, NewB107D8_0008D798) {
    const u32 x = arg<u32>(ctx, 1);
    const u32 m = static_cast<u32>(call(ctx, 0x0054ED38u, {28, 0, 16}));
    if (m == 0) {
        ret(ctx, 0u);
        return;
    }
    call(ctx, 0x004BB3B0u, {m, x});
    store<u32>(m + 12, 0);
    store<u32>(m + 16, 0);
    store<u32>(m + 20, 0);
    store<u8>(m + 24, 0);
    store<u32>(m, 0x00B107D8u);
    if (static_cast<u32>(call(ctx, 0x004FA14Cu, {x, 0x7A30D7E3u, m + 16, 0})) == 0 ||
        static_cast<u32>(call(ctx, 0x004FA14Cu, {x, 0x7F247172u, m + 12, 0})) == 0) {
        RdS32Cur36(ctx, x, 0x3683141Cu, m + 20);
        call(ctx, 0x004F986Cu, {x, 0xD7718094u, m + 24, load<u8>(m + 24)});
    }
    ret(ctx, m);
}

// T* New(-, Xml* x): a 28-byte object (Mem_Alloc(28, 0, 16); 0 on
// failure): Obj_InitVt_B21C98(m, x), +12..+20 = 0, u8 +24 / +25 = 0,
// vtable 0x00B108C8; the hashes 0x8A93973F (+12, default 0), 0x7A30D7E3
// (+16) and 0x922D3BD1 (+20) and the bool 0x6C59DEC2 (u8 +25), each with
// its current value; u8 +24 = 1 unless that bool read gives 1. r3 = m.
DECOMP(00090450, NewB108C8_00090450) {
    const u32 x = arg<u32>(ctx, 1);
    const u32 m = static_cast<u32>(call(ctx, 0x0054ED38u, {28, 0, 16}));
    if (m == 0) {
        ret(ctx, 0u);
        return;
    }
    call(ctx, 0x004BB3B0u, {m, x});
    store<u32>(m + 12, 0);
    store<u32>(m + 16, 0);
    store<u32>(m + 20, 0);
    store<u8>(m + 24, 0);
    store<u8>(m + 25, 0);
    store<u32>(m, 0x00B108C8u);
    call(ctx, 0x004FA14Cu, {x, 0x8A93973Fu, m + 12, 0});
    call(ctx, 0x004FA14Cu, {x, 0x7A30D7E3u, m + 16, load<u32>(m + 16)});
    call(ctx, 0x004FA14Cu, {x, 0x922D3BD1u, m + 20, load<u32>(m + 20)});
    if (static_cast<s32>(call(ctx, 0x004F986Cu, {x, 0x6C59DEC2u, m + 25, load<u8>(m + 25)})) != 1) store<u8>(m + 24, 1);
    ret(ctx, m);
}

// void f(Def* o, X* x): the target t = CompTarget34(x, o->+12) or else
// Obj_FindById(o->+16); with one: t->vt[72](t, o->+20, -, -, -, -, u8
// o->+40; f1-f4 = o->+24..+36) (OPD / entry in r5 / r6, r7 / r8 as left)
// and, when bit 0x02000000 of t->+196 is set, Obj_Scale500_001CAA88(t).
// r3 = 0 or the last call's / that bit.
DECOMP(00097B24, Obj_ApplyDef_00097B24) {
    const u32 o = arg<u32>(ctx, 0), x = arg<u32>(ctx, 1);
    u32 t = CompTarget34(ctx, x, load<u32>(o + 12));
    if (t == 0) t = static_cast<u32>(call(ctx, 0x000D6A28u, {load<u32>(o + 16)}));
    if (t == 0) {
        ret(ctx, 0u);
        return;
    }
    const u32 opd = load<u32>(load<u32>(t) + 72);
    ctx.fpr[1] = Lfs32(o + 24);
    ctx.fpr[2] = Lfs32(o + 28);
    ctx.fpr[3] = Lfs32(o + 32);
    ctx.fpr[4] = Lfs32(o + 36);
    call_ptr(ctx, opd, {t, load<u32>(o + 20), opd, load<u32>(opd), ctx.gpr[7], ctx.gpr[8], load<u8>(o + 40)});
    const u32 b = load<u32>(t + 196) & 0x02000000u;
    if (b == 0) {
        ret(ctx, 0u);
        return;
    }
    call(ctx, 0x001CAA88u, {t});
}

// void f(Def* o, X* x): the target t = o->+32, replaced by c->vt[24](c)
// for the component c of x named o->+36 (when set) whose vt[32]() is 2;
// with t: a new 52-byte command (SmallAlloc; 0 on failure)
// 0x00433120(m, o + 12, t, o->+40, u8 o->+44, x, u8 o->+45) goes to
// CmdList_Push_0042C138 (r3 = its). r3 as left without t.
DECOMP(000A0E80, Cmd_New433120_000A0E80) {
    const u32 o = arg<u32>(ctx, 0), x = arg<u32>(ctx, 1);
    u32 t = load<u32>(o + 32);
    const u32 k = load<u32>(o + 36);
    if (k != 0) {
        const u32 c = static_cast<u32>(call(ctx, 0x004BB4B4u, {x, k}));
        if (c != 0 && static_cast<s32>(VCall(ctx, c, 32, {c})) == 2) t = static_cast<u32>(VCall(ctx, c, 24, {c}));
    }
    if (t == 0) return;
    const u32 m = static_cast<u32>(call(ctx, 0x0055238Cu, {52}));
    u32 cmd = 0;
    if (m != 0) {
        call(ctx, 0x00433120u, {m, o + 12, t, load<u32>(o + 40), load<u8>(o + 44), x, load<u8>(o + 45)});
        cmd = m;
    }
    call(ctx, 0x0042C138u, {cmd});
}

// void f(T* o): each object of the pointer vector 0x01013B48 (u16 count
// +8, re-read per step) gets vt[56](), each of 0x01013B5C vt[32](); then
// 0x0028D7B0(o) (r3 = its).
DECOMP(000D016C, World_SubmitLists_000D016C) {
    const u32 o = arg<u32>(ctx, 0);
    constexpr u32 A = 0x01013B48u, B = 0x01013B5Cu;
    for (u32 i = 0; i < load<u16>(A + 8); ++i) {
        const u32 p = load<u32>(load<u32>(A) + 4 * i);
        VCall(ctx, p, 56, {p});
    }
    for (u32 i = 0; i < load<u16>(B + 8); ++i) {
        const u32 p = load<u32>(load<u32>(B) + 4 * i);
        VCall(ctx, p, 32, {p});
    }
    call(ctx, 0x0028D7B0u, {o});
}

// void f(T* o): unless bit 0x80 of o->+172 is set (r3 = 1): for each
// 48-byte record e of the vector v = o->+388 (u16 count +8; re-read after
// each notify) with e->+28 set, while o->+216's +208 is 0xA970EC1F and the
// matching 160-byte record d of (o->+216->+192)->+0 has +144 0 or
// 0xA970EC1F: Volume_Notify112D8(*0x00F557EC, e + 32). r3 = 0 or v.
DECOMP(000E704C, Obj_NotifyVolumes_000E704C) {
    const u32 o = arg<u32>(ctx, 0);
    if ((load<u32>(o + 172) & 0x80u) != 0) {
        ret(ctx, 1u);
        return;
    }
    u32 v = load<u32>(o + 388);
    const u32 g = load<u32>(o + 216);
    ctx.gpr[3] = v;
    if (v == 0 || load<u16>(v + 8) == 0) return;
    constexpr u32 kMagic = 0xA970EC1Fu;
    for (u32 i = 0;; ++i) {
        const u32 n = v != 0 ? load<u16>(v + 8) : 0u;
        if (!(i < n)) break;
        const u32 e = load<u32>(v) + 48 * i;
        const u32 d = load<u32>(load<u32>(g + 192)) + 160 * i;
        if (load<u32>(e + 28) == 0 || load<u32>(g + 208) != kMagic) continue;
        const u32 k = load<u32>(d + 144);
        if (k != 0 && k != kMagic) continue;
        call(ctx, 0x000112D8u, {load<u32>(0x00F557ECu), e + 32});
        v = load<u32>(o + 388);
    }
    ctx.gpr[3] = v;
}

// bool f(T* o, u32 key, a, u8 b, -, -, c, -, d; f1-f3): the speaker
// o->+164 (when set) gets Speaker_SetLineByKey_00350764(it, key, 1, 1);
// then o->vt[196](o, o->vt[188](o, key), a, b, -, -, c, -, d; f1-f3)
// (OPD / entry in r7 / r8, r10 as left, d on the stack); when that is
// set o->+600 = key and true.
DECOMP(000FC9B8, Obj_SayLine_000FC9B8) {
    const u32 o = arg<u32>(ctx, 0), key = arg<u32>(ctx, 1), a = arg<u32>(ctx, 2), c = arg<u32>(ctx, 6);
    const u32 b = arg<u32>(ctx, 3) & 0xFFu;
    const f64 f1 = farg(ctx, 0), f2 = farg(ctx, 1), f3 = farg(ctx, 2);
    const u32 d = StkArg34(ctx, 116);
    const u32 sp = load<u32>(o + 164);
    if (sp != 0) call(ctx, 0x00350764u, {sp, key, 1, 1});
    const u32 r = static_cast<u32>(VCall(ctx, o, 188, {o, key}));
    const u32 opd = load<u32>(load<u32>(o) + 196);
    ctx.fpr[1] = f1;
    ctx.fpr[2] = f2;
    ctx.fpr[3] = f3;
    const u32 s = static_cast<u32>(call_ptr(ctx, opd, {o, r, a, b, opd, load<u32>(opd), c, ctx.gpr[10], d}));
    if (s == 0) {
        ret(ctx, 0u);
        return;
    }
    store<u32>(o + 600, key);
    ret(ctx, 1u);
}

// void f(T* o, -, Mtx* out): for the world object w = o->+4->+160 of kind
// 11 (bits 15..10 of +4; not when bit 31 of o->+4->+468 is set):
// WorldObj_UpdateLod(w, 0, 0), then out gets w's matrix +80 with the
// rows +0 and +32 negated (sign flips of the raw words) and +16 / +48
// copied; with u8 o->+9 set, out->+64 = w->+340 when that is above 0
// (0x000FD2C8). r3 / f1 as left.
DECOMP(000FD2D0, Obj_GetMirrorMtx_000FD2D0) {
    const u32 o = arg<u32>(ctx, 0), out = arg<u32>(ctx, 2);
    const u32 p = load<u32>(o + 4);
    if ((load<u32>(p + 468) >> 31) != 0) {
        ret(ctx, 1u);
        return;
    }
    const u32 w = load<u32>(p + 160);
    if (w == 0) {
        ret(ctx, 0u);
        return;
    }
    const u32 kind = (load<u32>(w + 4) >> 10) & 0x3Fu;
    if (kind != 11) {
        ret(ctx, kind);
        return;
    }
    call(ctx, 0x0057EE00u, {w, 0, 0});
    const u32 m = w + 80;
    constexpr u32 S = 0x80000000u;
    const u32 n0 = load<u32>(m) ^ S, n4 = load<u32>(m + 4) ^ S, n8 = load<u32>(m + 8) ^ S;
    const u32 n32 = load<u32>(m + 32) ^ S, n36 = load<u32>(m + 36) ^ S, n40 = load<u32>(m + 40) ^ S;
    const u32 a16 = load<u32>(m + 16), a20 = load<u32>(m + 20), a24 = load<u32>(m + 24);
    const u32 a48 = load<u32>(m + 48), a52 = load<u32>(m + 52), a56 = load<u32>(m + 56);
    store<u32>(out + 16, a16);
    store<u32>(out + 20, a20);
    store<u32>(out, n0);
    store<u32>(out + 24, a24);
    store<u32>(out + 4, n4);
    store<u32>(out + 8, n8);
    store<u32>(out + 32, n32);
    store<u32>(out + 36, n36);
    const u32 fl = load<u8>(o + 9);
    store<u32>(out + 40, n40);
    store<u32>(out + 48, a48);
    store<u32>(out + 52, a52);
    store<u32>(out + 56, a56);
    ctx.fpr[1] = static_cast<f64>(std::bit_cast<f32>(n0));
    if (fl == 0) {
        ret(ctx, 0u);
        return;
    }
    constexpr u32 K = 0x000FD2C8u;
    ctx.fpr[1] = Lfs32(w + 340);
    if (ctx.fpr[1] > Lfs32(K)) store<u32>(out + 64, load<u32>(w + 340));
    ret(ctx, K);
}

// ~T(): vtable 0x00B13E00; o->+592 (when set) gets vt[40](it, 0) and is
// cleared; the skeleton q = o->+596 is removed from the world's physics
// ((*0x00F65CA0)->+0x1281D4->+4, Body_Remove931F00) when its +8 is set,
// released (AnimSkel_Release7D4EA8) and cleared; a Havok object h =
// o->+600 has its body entity removed (Hk_RemoveEntity92FFC8 with a stack
// result, when Hk_GetBody9D19C8(h)->+8 is set) and is released; vtable
// 0x00B0DAEC. r3 = 0x00B0DAEC.
DECOMP(0010E9A4, VtB13E00_Dtor_0010E9A4) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 p = load<u32>(o + 592);
    store<u32>(o, 0x00B13E00u);
    if (p != 0) VCall(ctx, p, 40, {p, 0});
    u32 q = load<u32>(o + 596);
    store<u32>(o + 592, 0);
    if (load<u32>(q + 8) != 0) {
        call(ctx, 0x00931F00u, {load<u32>(load<u32>(load<u32>(0x00F65CA0u) + 0x1281D4u) + 4), q});
        q = load<u32>(o + 596);
    }
    call(ctx, 0x007D4EA8u, {q});
    const u32 h = load<u32>(o + 600);
    store<u32>(o + 596, 0);
    if (h != 0) {
        if (load<u32>(static_cast<u32>(call(ctx, 0x009D19C8u, {h})) + 8) != 0) {
            const u32 e = load<u32>(static_cast<u32>(call(ctx, 0x009D19C8u, {load<u32>(o + 600)})) + 8);
            const u32 b = static_cast<u32>(call(ctx, 0x009D19C8u, {load<u32>(o + 600)}));
            Stack fr(ctx, 16);
            call(ctx, 0x0092FFC8u, {fr.addr(), e, b});
        }
        call(ctx, 0x007D4EA8u, {load<u32>(o + 600)});
    }
    store<u32>(o, 0x00B0DAECu);
    ret(ctx, 0x00B0DAECu);
}

// void f(T* o, -, u8 a, u8 b, c, d, const u32* arr, s32 n; f1 = v): unless
// bit 8 of u8 o->+968 is set (r3 = o): with arr, the word array +976 is
// freed (Mem_FreeFrom) and reallocated (Mem_Alloc(n * 4, 0, 16), not
// checked) and n words copied (at least one for a negative n); then bit
// 0x10 of u8 +968 set, +980 = v, +984 = 0, u8 +988 / +996 = a, +989 = b,
// bits 0x80 / 0x40 of u8 +997 = bit 0 of c / d. r3 = 0.
DECOMP(00129488, Obj_SetRoute_00129488) {
    const u32 o = arg<u32>(ctx, 0), a = arg<u32>(ctx, 2), b = arg<u32>(ctx, 3), c = arg<u32>(ctx, 4);
    const u32 d = arg<u32>(ctx, 5), arr = arg<u32>(ctx, 6), n = arg<u32>(ctx, 7);
    const f64 v = farg(ctx, 0);
    if ((load<u8>(o + 968) & 0x08u) != 0) return;
    if (arr != 0) {
        const u32 old = load<u32>(o + 976);
        if (old != 0) {
            call(ctx, 0x0054EE6Cu, {old, 0});
            store<u32>(o + 976, 0);
        }
        store<u32>(o + 976, static_cast<u32>(call(ctx, 0x0054ED38u, {n << 2, 0, 16})));
        if (n != 0) {
            s32 left = static_cast<s32>(n);
            u32 k = 0;
            do {
                const u32 m = load<u32>(o + 976);
                --left;
                store<u32>(m + k, load<u32>(arr + k));
                k += 4;
            } while (left > 0);
        }
    }
    const u32 b968 = load<u8>(o + 968) | 0x10u;
    u32 b997 = load<u8>(o + 997);
    b997 = (b997 & ~0x80u) | ((c & 1u) << 7);
    Stfs32(o + 980, v);
    store<u32>(o + 984, 0);
    b997 = (b997 & ~0x40u) | ((d & 1u) << 6);
    store<u8>(o + 968, static_cast<u8>(b968));
    store<u8>(o + 988, static_cast<u8>(a));
    store<u8>(o + 989, static_cast<u8>(b));
    store<u8>(o + 996, static_cast<u8>(a));
    store<u8>(o + 997, static_cast<u8>(b997));
    ret(ctx, 0u);
}

// void f(T* o; f1 = dt): the timer +584, while above 0, decreases by dt *
// +504 (fnmsubs); once at or below 0 it is 0 and, when the state +592 is
// 0x04A4252F, it becomes 0xD473DFE9 (the effect +356, when set, gets
// Effect_SetHash4CF988 0x7FB3B232 then 0xD473DFE9). The ratio timer /
// +496 (0 unless +496 > 0) goes to (o->+576)->+20 (when set), and the
// effect then gets Effect_SetTime(; f1 = 1) (constants 0x001D85B8). r3 =
// the effect or that call's.
DECOMP(001D85C0, Obj_TickTimer_001D85C0) {
    const u32 o = arg<u32>(ctx, 0);
    const f64 dt = farg(ctx, 0);
    constexpr u32 K = 0x001D85B8u;
    const f64 k0 = Lfs32(K);
    f64 v = Lfs32(o + 584);
    u32 e = load<u32>(o + 356);
    if (v > k0) {
        v = static_cast<f32>(-std::fma(dt, Lfs32(o + 504), -v));
        Stfs32(o + 584, v);
    }
    f64 f1 = v;
    if (v < k0 || v == k0) {
        if (load<u32>(o + 592) == 0x04A4252Fu) {
            if (e != 0) {
                call(ctx, 0x004CF988u, {e, 0x7FB3B232u});
                call(ctx, 0x004CF988u, {load<u32>(o + 356), 0xD473DFE9u});
                e = load<u32>(o + 356);
            }
            store<u32>(o + 592, 0xD473DFE9u);
        }
        f1 = k0;
        store<u32>(o + 584, 0);
    }
    f64 r = k0;
    const f64 dv = Lfs32(o + 496);
    if (dv > k0) r = static_cast<f32>(f1 / dv);
    const u32 t = load<u32>(o + 576);
    ctx.gpr[3] = e;
    ctx.fpr[1] = f1;
    if (t == 0) return;
    Stfs32(t + 20, r);
    if (e == 0) return;
    callf(ctx, 0x004CF710u, {e}, {Lfs32(K + 4)});
}

// void f(Area* o, T* m): for c = m when bit 8 of m->+192 is set, else 0
// (quirk: c's vtable is then read from address 0): when c->vt[32](c, 0)
// is 0, c->vt[32](c, 1) is 10 and c->vt[32](c, 2) is 2, c->vt[28](c, 2,
// 3). Then Area_Leave(o, m) (r3 = its).
DECOMP(00219E84, Area_OnLeave_00219E84) {
    const u32 o = arg<u32>(ctx, 0), m = arg<u32>(ctx, 1);
    const u32 c = (m != 0 && (load<u32>(m + 192) & 8u) != 0) ? m : 0u;
    if (static_cast<s32>(VCall(ctx, c, 32, {c, 0})) == 0 && static_cast<s32>(VCall(ctx, c, 32, {c, 1})) == 10 &&
        static_cast<s32>(VCall(ctx, c, 32, {c, 2})) == 2)
        VCall(ctx, c, 28, {c, 2, 3});
    call(ctx, 0x0020EBC8u, {o, m});
}

// void f(T* o, Query* q): Obj_CopyQuery_000CDB28(o, q), then q->+144 =
// +180, the bools q->+148..+153 = bits 0x80, 0x40, 0x20, 0x10, 0x08,
// 0x02 of u8 o->+184, the floats / words +192..+200 -> +160..+168,
// +208..+220 -> +176..+188 and the bytes +224..+227 -> +192, +193, +195,
// +196 (raw, in the original's order). r3 = u8 +227, f1 = +220.
DECOMP(0021AE1C, Query_CopyFrom_0021AE1C) {
    const u32 o = arg<u32>(ctx, 0), q = arg<u32>(ctx, 1);
    call(ctx, 0x000CDB28u, {o, q});
    store<u32>(q + 144, load<u32>(o + 180));
    auto bit = [&](u32 m) { return static_cast<u8>((load<u8>(o + 184) & m) != 0 ? 1 : 0); };
    store<u8>(q + 148, bit(0x80));
    store<u8>(q + 149, bit(0x40));
    store<u8>(q + 150, bit(0x20));
    store<u8>(q + 151, bit(0x10));
    store<u8>(q + 152, bit(0x08));
    const u8 b153 = bit(0x02);
    store<u32>(q + 160, load<u32>(o + 192));
    store<u32>(q + 164, load<u32>(o + 196));
    const u32 f200 = load<u32>(o + 200);
    store<u8>(q + 153, b153);
    store<u32>(q + 168, f200);
    const u32 f212 = load<u32>(o + 212);
    const u8 c224 = load<u8>(o + 224);
    store<u32>(q + 180, f212);
    store<u8>(q + 192, c224);
    const u32 f216 = load<u32>(o + 216), w208 = load<u32>(o + 208);
    const u8 c225 = load<u8>(o + 225);
    store<u32>(q + 184, f216);
    store<u32>(q + 176, w208);
    store<u8>(q + 193, c225);
    const u32 f220 = load<u32>(o + 220);
    const u8 c226 = load<u8>(o + 226);
    store<u32>(q + 188, f220);
    store<u8>(q + 195, c226);
    const u8 c227 = load<u8>(o + 227);
    store<u8>(q + 196, c227);
    ctx.fpr[1] = static_cast<f64>(std::bit_cast<f32>(f220));
    ret(ctx, static_cast<u32>(c227));
}

// T* f(a, b, c): a new 240-byte query (Mem_Alloc(240, 0, 16); 0 on
// failure): 0x00746148(m, c, 0, 0, 1, 0, 1, 0, 0, 0), vtable 0x00B1A758,
// the pointer vector +188 (PtrVector_Construct, element size u16 +194 =
// 76, PtrVector_Init(v, 16, 0, 1)), +208..+232 = 0, bit 31 of +4 set;
// then QueryDef_Read21E978(a, b, m). r3 = m.
DECOMP(0021ED78, Query_New_0021ED78) {
    const u32 a = arg<u32>(ctx, 0), b = arg<u32>(ctx, 1), c = arg<u32>(ctx, 2);
    const u32 m = static_cast<u32>(call(ctx, 0x0054ED38u, {240, 0, 16}));
    u32 q = 0;
    if (m != 0) {
        call(ctx, 0x00746148u, {m, c, 0, 0, 1, 0, 1, 0, 0, 0});
        store<u32>(m, 0x00B1A758u);
        call(ctx, 0x005516E0u, {m + 188});
        store<u16>(m + 194, 76);
        call(ctx, 0x00551598u, {m + 188, 16, 0, 1});
        const u32 w = load<u32>(m + 4);
        for (u32 k = 208; k <= 232; k += 4) store<u32>(m + k, 0);
        q = m;
        store<u32>(m + 4, w | 0x80000000u);
    }
    call(ctx, 0x0021E978u, {a, b, q});
    ret(ctx, q);
}

// void f(Volume* o, bool on): when bit 31 of +412 changes: the effect
// +428 (when set) gets Effect_SetHash4CF988 0xA6CB0022 (on) / 0xF6239C09
// (off); the bit is set to on; on: Volume_Activate20C2BC(o); off:
// Volume_Notify112D8(*0x00F557EC, o + 424), Volume_Prepare20C670(o)
// unless bit 0x02000000 of +412 is set, and o->vt[96](o, 0). r3 = +412
// when unchanged, else the last call's.
DECOMP(00220190, Volume_SetActive_00220190) {
    const u32 o = arg<u32>(ctx, 0), v = arg<u32>(ctx, 1);
    const u32 b = v & 0xFFu;
    if ((load<u32>(o + 412) >> 31) != b) {
        const u32 e = load<u32>(o + 428);
        if (e != 0) call(ctx, 0x004CF988u, {e, b != 0 ? 0xA6CB0022u : 0xF6239C09u});
    }
    u32 w = load<u32>(o + 412);
    if ((w >> 31) == b) {
        ret(ctx, w);
        return;
    }
    w = (w & 0x7FFFFFFFu) | ((v & 1u) << 31);
    store<u32>(o + 412, w);
    if (b != 0) {
        call(ctx, 0x0020C2BCu, {o});
        return;
    }
    call(ctx, 0x000112D8u, {load<u32>(0x00F557ECu), o + 424});
    if ((load<u32>(o + 412) & 0x02000000u) == 0) call(ctx, 0x0020C670u, {o});
    VCall(ctx, o, 96, {o, 0});
}

// void f(T* o): with an animation a = o->+216->+720: r = o->vt[60](o);
// with r, Anim_Restart2775A8(r, a, 0, 0, -1, 0, 0, 0, -, -, 0, -1; f1 =
// 1, f2 = 0, f3 = f4 = ...) (constants 0x00229B24; the float slots 9 / 10
// passed as 0), else o->vt[200](o, a, 0, 0, entry, a, 0, OPD, 0; the same
// floats). Then the effect +356 (when set) gets Effect_SetHash4CF988(it,
// 0xD2C2058D). r3 as left.
DECOMP(00229B30, Obj_RestartAnim_00229B30) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 g = load<u32>(o + 216);
    const u32 a0 = load<u32>(g + 720);
    ctx.gpr[3] = a0;
    if (a0 != 0) {
        const u32 r = static_cast<u32>(VCall(ctx, o, 60, {o}));
        constexpr u32 K = 0x00229B24u;
        const f64 k0 = Lfs32(K), k1 = Lfs32(K + 4), k2 = Lfs32(K + 8);
        const u32 a = load<u32>(g + 720);
        if (r != 0) {
            callf(ctx, 0x002775A8u, {r, a, 0, 0, ~0ull, 0, 0, 0, 0, 0, 0, ~0ull}, {k1, k0, k2, k2});
        } else {
            const u32 opd = load<u32>(load<u32>(o) + 200);
            ctx.fpr[1] = k1;
            ctx.fpr[2] = k0;
            ctx.fpr[3] = k2;
            ctx.fpr[4] = k2;
            call_ptr(ctx, opd, {o, a, 0, 0, load<u32>(opd), a, 0, opd, 0});
        }
    }
    const u32 e = load<u32>(o + 356);
    if (e != 0) call(ctx, 0x004CF988u, {e, 0xD2C2058Du});
}

// ~T() body: DtorUnlist37 with vtables 0x00B1C258 / 0x00B1C2A8.
DECOMP(0027CE34, VtB1C258_Dtor_0027CE34) { DtorUnlist37(ctx, 0x00B1C258u, 0x00B1C2A8u); }

// ~T() body: DtorUnlist37 with vtables 0x00B1C328 / 0x00B1C378.
DECOMP(0027E968, VtB1C328_Dtor_0027E968) { DtorUnlist37(ctx, 0x00B1C328u, 0x00B1C378u); }

// ~T() body: DtorUnlist37 with vtables 0x00B1C3F8 / 0x00B1C444.
DECOMP(00280118, VtB1C3F8_Dtor_00280118) { DtorUnlist37(ctx, 0x00B1C3F8u, 0x00B1C444u); }

// bool f(Req* o): once (u8 +36 set to 1): with the definition d = o->+32,
// a = d->+40 when d->+32 is 5 (else 0) and the value v = the second word
// of d->+76's 16-byte entry d->+104 (kind 5) or of the entry whose first
// word is o->+4 (d->+72 entries; 0 when none); the callback o->+16 (when
// set) gets (o->+4, o->+12, a, v, o->+24, entry, OPD). r3 = 1.
DECOMP(002EA218, Req_Fire_002EA218) {
    const u32 o = arg<u32>(ctx, 0);
    if (load<u8>(o + 36) != 0) {
        ret(ctx, 1u);
        return;
    }
    const u32 cb = load<u32>(o + 16), k = load<u32>(o + 4), b = load<u32>(o + 12), c = load<u32>(o + 24);
    const u32 d = load<u32>(o + 32);
    const bool five = load<s32>(d + 32) == 5;
    const u32 a = five ? load<u32>(d + 40) : 0u;
    u32 v = 0;
    if (five) {
        v = load<u32>(load<u32>(d + 76) + (load<u32>(d + 104) << 4) + 4);
    } else {
        const u32 n = load<u32>(d + 72);
        for (u32 j = 0; j < n; ++j) {
            const u32 e = load<u32>(d + 76) + 16 * j;
            if (load<u32>(e) == k) {
                v = load<u32>(e + 4);
                break;
            }
        }
    }
    store<u8>(o + 36, 1);
    if (cb != 0) call_ptr(ctx, cb, {k, b, a, v, c, load<u32>(cb), cb});
    ret(ctx, 1u);
}

// void f(T* o): for the owner p = o->+8 and key k = o->+16: the effect
// p->+52 (when set) gets Effect_Release(it, k); when p is one of the
// three words +900 of the slot owner s = p->+56, the matching pair
// +916 (only when s->+432 is 1) / +920 get vt[128](it, k, 1). +20 = 1;
// r3 = 1.
DECOMP(002FB6A8, Slot_Release_002FB6A8) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 p = load<u32>(o + 8), k = load<u32>(o + 16);
    const u32 e = load<u32>(p + 52);
    if (e != 0) call(ctx, 0x004CF3C4u, {e, k});
    const u32 s = load<u32>(p + 56);
    if (s != 0) {
        for (u32 i = 0; i < 3; ++i) {
            if (load<u32>(s + 900 + 4 * i) != p) continue;
            const u32 q = s + 8 * i;
            if (load<s32>(s + 432) == 1) {
                const u32 a = load<u32>(q + 916);
                if (a != 0) VCall(ctx, a, 128, {a, k, 1});
            }
            const u32 b = load<u32>(q + 920);
            if (b != 0) VCall(ctx, b, 128, {b, k, 1});
            break;
        }
    }
    store<u32>(o + 20, 1);
    ret(ctx, 1u);
}

// u16 f(T* o, a, b): for each child x of the vector +52 (u16 count +60)
// with u8 +58 set and +57 clear, each child y of x's vector +36 (u16
// count +44) with u8 +52 set and a handler z = y->+60 gets z->vt[24](z,
// a, b). r3 = o's count.
DECOMP(002FD414, Tree_Broadcast_002FD414) {
    const u32 o = arg<u32>(ctx, 0), a = arg<u32>(ctx, 1), b = arg<u32>(ctx, 2);
    for (u32 i = 0; i < load<u16>(o + 60); ++i) {
        const u32 x = load<u32>(load<u32>(o + 52) + 4 * i);
        if (load<u8>(x + 58) == 0 || load<u8>(x + 57) != 0) continue;
        for (u32 j = 0; j < load<u16>(x + 44); ++j) {
            const u32 y = load<u32>(load<u32>(x + 36) + 4 * j);
            if (load<u8>(y + 52) == 0) continue;
            const u32 z = load<u32>(y + 60);
            if (z != 0) VCall(ctx, z, 24, {z, a, b});
        }
    }
    ret(ctx, static_cast<u32>(load<u16>(o + 60)));
}

// void f(T* o, bool on): o->+24 (when set) gets vt[40](it, on, 1); the
// second word of each 16-byte record of o->+328 (o->+8->+152 of them;
// re-read per step) that is set gets vt[88](it, on); the effect o->+20
// (when set) gets Effect_SetHash4CF988 0x152FB9B1 (on) / 0x90C77BBD
// (off). r3 = that call's or the record count.
DECOMP(0036AACC, Obj_SetOn_0036AACC) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 on = arg<u32>(ctx, 1) & 0xFFu;
    const u32 p = load<u32>(o + 24);
    if (p != 0) VCall(ctx, p, 40, {p, on, 1});
    const u32 q = load<u32>(o + 8);
    for (u32 i = 0; i < load<u32>(q + 152); ++i) {
        const u32 e = load<u32>(load<u32>(o + 328) + 16 * i + 4);
        if (e != 0) VCall(ctx, e, 88, {e, on});
    }
    ctx.gpr[3] = load<u32>(q + 152);
    const u32 f = load<u32>(o + 20);
    if (f != 0) call(ctx, 0x004CF988u, {f, on != 0 ? 0x152FB9B1u : 0x90C77BBDu});
}

// void f(const Ref* o, T* t): the name of o's entry: the 128-byte records
// of (*0x00F65038)->+24 indexed by s16 o->+4 (base) and s16 o->+2 (r; 0
// for -1, quirk: then read at address 2), the word at base + u16
// *0x0106CF70 + 4 * the byte r + u16 *0x0106CF7E (r itself when u8 r->+2
// is 0); when set, its String_HashNoCase goes to t's 132-byte entry
// s16 o->+6 (+152; when below u16 t->+144 and its first word is below
// 10, else the base 0 - quirk) + u16 *0x0106CF80 + 4. r3 = o or the hash.
DECOMP(00391878, Ref_HashName_00391878) {
    const u32 o = arg<u32>(ctx, 0), t = arg<u32>(ctx, 1);
    const u32 g = load<u32>(0x00F65038u);
    const s32 i1 = static_cast<s16>(load<u16>(o + 4)), i2 = static_cast<s16>(load<u16>(o + 2));
    const u32 base = i1 != -1 ? load<u32>(g + 24) + (static_cast<u32>(i1) << 7) : 0u;
    const u32 r = i2 != -1 ? load<u32>(g + 24) + (static_cast<u32>(i2) << 7) : 0u;
    const u32 rb = load<u8>(r + 2) != 0 ? r + load<u16>(0x0106CF7Eu) : 0u;
    const u32 name = load<u32>(base + load<u16>(0x0106CF70u) + (static_cast<u32>(load<u8>(rb)) << 2));
    if (name == 0) return;
    const s32 k = static_cast<s16>(load<u16>(o + 6));
    u32 e = 0;
    if (k < static_cast<s32>(load<u16>(t + 144))) {
        const u32 c = t + 152 + static_cast<u32>(k) * 132u;
        if (load<s32>(c) < 10) e = c;
    }
    const u32 dst = e + load<u16>(0x0106CF80u) + 4;
    store<u32>(dst, static_cast<u32>(call(ctx, 0x0056A710u, {name})));
}

// void f(Box* o): the bounds +48..+56 / +64..+72 of the objects of the
// vector o (data +0, u16 count +8; each one's +0->+144 position): the
// first sets min = p - (h, h, hz) and max = p + (h, h, hz); later ones
// only widen x / y (constants 0x003EE558: h = 3, hz = 0). r3 = o.
DECOMP(003EE560, Box_FitPoints_003EE560) {
    const u32 o = arg<u32>(ctx, 0);
    constexpr u32 K = 0x003EE558u;
    auto f = [](f64 x) { return static_cast<f64>(static_cast<f32>(x)); };
    const f64 h = Lfs32(K), hz = Lfs32(K + 4);
    for (u32 i = 0; i < load<u16>(o + 8); ++i) {
        const u32 p = load<u32>(load<u32>(load<u32>(o) + 4 * i)) + 144;
        if (i == 0) {
            const f64 x = Lfs32(p), y = Lfs32(p + 4), z = Lfs32(p + 8);
            Stfs32(o + 48, f(x - h));
            Stfs32(o + 52, f(y - h));
            Stfs32(o + 56, f(z - hz));
            Stfs32(o + 64, f(x + h));
            Stfs32(o + 68, f(y + h));
            ctx.fpr[1] = f(x + h);
            Stfs32(o + 72, f(z + hz));
            continue;
        }
        const f64 x = Lfs32(p), y = Lfs32(p + 4);
        const f64 x0 = f(x - h), y0 = f(y - h), x1 = f(x + h), y1 = f(y + h);
        if (x0 < Lfs32(o + 48)) Stfs32(o + 48, x0);
        if (y0 < Lfs32(o + 52)) Stfs32(o + 52, y0);
        if (x1 > Lfs32(o + 64)) Stfs32(o + 64, x1);
        ctx.fpr[1] = Lfs32(o + 68);
        if (y1 > ctx.fpr[1]) Stfs32(o + 68, y1);
    }
}

// void f(T* o): unless one of the objects of the vector +164 (u16 count
// +172) has a +84 kind of 0..5 (r3 = its slot address): a new 384-byte
// helper (SmallAlloc; 0x0040AB98(m); 0 on failure) set up by
// 0x0040AD98(m, o->+88, o) and pushed on the vector (capacity +168,
// doubled by PtrVector_Reserve when full; high-water +174). r3 = the old
// high-water.
DECOMP(0040C1E4, Obj_AddHelper_0040C1E4) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 n = load<u16>(o + 172);
    const u32 d = load<u32>(o + 164);
    for (u32 i = 0; i < n; ++i) {
        if (load<u32>(load<u32>(d + 4 * i) + 84) <= 5u) {
            ret(ctx, d + 4 * i);
            return;
        }
    }
    const u32 m = static_cast<u32>(call(ctx, 0x0055238Cu, {384}));
    u32 h = 0;
    if (m != 0) {
        call(ctx, 0x0040AB98u, {m});
        h = m;
    }
    call(ctx, 0x0040AD98u, {h, load<u32>(o + 88), o});
    const u32 v = o + 164;
    u32 cnt = load<u16>(v + 8);
    const u32 cap = load<u16>(v + 4);
    if (cnt == cap) {
        call(ctx, 0x00551718u, {v, cap << 1});
        cnt = load<u16>(v + 8);
    }
    const u32 data = load<u32>(v);
    const u32 c2 = load<u16>(v + 8);
    const u32 hw = load<u16>(v + 10);
    store<u32>(data + (cnt << 2), h);
    const u32 n1 = c2 + 1u;
    store<u16>(v + 8, static_cast<u16>(n1));
    store<u16>(v + 10, static_cast<u16>(static_cast<s32>(hw) > static_cast<s32>(n1 & 0xFFFFu) ? hw : n1));
    ret(ctx, hw);
}
