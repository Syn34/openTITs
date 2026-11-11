// Game code, part 34: helpers shared by obj34*.cpp (the continued sweep of
// small still-recompiled game functions, in size order, from 65
// instructions up).
#pragma once

#include <bit>
#include <cmath>

#include "obj33.h"

namespace sly4 {
namespace obj34 {

using namespace opentit;
using namespace opentit::decomp;
using namespace sly4::obj33;
using namespace sly4::obj32;
using namespace sly4::obj31;
using namespace sly4::obj30;
using namespace sly4::obj29;
using namespace sly4::obj28;
using namespace sly4::obj27;
using namespace sly4::obj26;
using namespace sly4::obj25;
using namespace sly4::obj24;
using namespace sly4::obj23;
using namespace sly4::obj19;

// The caller's stack argument word at sp + off (sp = r1 at entry; the
// ninth integer argument's slot is +116 for a u32).
inline u32 StkArg34(PPUContext& ctx, u32 off) { return load<u32>(static_cast<u32>(ctx.gpr[1]) + off); }

// The u16-counted pointer vector v (data +0, capacity +4, count +8): the
// allocator a = v->+16 frees the data (a->vt[8](a, data, n * (cap >> 1)))
// when set, then Array_Destruct(v). The pattern of a destructor body that
// tears down two of these.
inline void FreeAllocVec34(PPUContext& ctx, u32 v) {
    const u32 n = static_cast<u32>(load<u16>(v)) * ((static_cast<u32>(load<u16>(v + 6)) >> 1) & 0x7FFFu);
    const u32 d = load<u32>(v + 8);
    if (d != 0) {
        const u32 a = load<u32>(v + 12);
        VCall(ctx, a, 8, {a, d, n});
    }
    call(ctx, 0x0054EC94, {v});
}

// The layer close of 0x0010B240 / 0x00113350 (void f(Layer* o)): in the
// owner p = o->+4's layer stack (u8 count +345, entries +368) the layer
// below o (0 when o is the bottom one or not on the stack); then
// Obj_PopLayer(p, o; f1 = the constant at k), the layer below gets
// vt[40](it) and o (when not 0) vt[4](o). r3 as left.
inline void CloseLayer34(PPUContext& ctx, u32 k) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 p = load<u32>(o + 4);
    u32 below = 0;
    for (u32 n = load<u8>(p + 345); n != 0; --n) {
        if (load<u32>(p + 368 + 4 * (n - 1)) == o) {
            if (n > 1) below = load<u32>(p + 368 + 4 * (n - 2));
            break;
        }
    }
    callf(ctx, 0x000FBB88, {p, o}, {Lfs32(k)});
    if (below != 0) VCall(ctx, below, 40, {below});
    if (o != 0) VCall(ctx, o, 4, {o});
}

// The arm checks of 0x001BFB00 / 0x001BFD28 (void f(T* o, Msg* m)):
// after Obj_TestE0BCC(o, m) passes, with o's u8 +572 lacking 0x20 and
// either having 0x10, or lacking 0x08 with (+564 <= k or o->vt[32](o, 0)
// == 1), the u8 gets 0x40 and +560 = (s8)(m->+4)->+172 when the sender's
// flag 1 of +188 is set and (o->+216)->+doff > k (k = the float at kaddr;
// late: the sender test comes before the float one). r3 as left (the u8
// or the test's / vt's result when stopped, +560 when armed). range:
// the variant of 0x001BFC10 (see there).
inline void ArmCheck34(PPUContext& ctx, u32 kaddr, u32 doff, bool late, bool range = false) {
    const u32 o = arg<u32>(ctx, 0), m = arg<u32>(ctx, 1);
    if (static_cast<u32>(call(ctx, 0x000E0BCC, {o, m})) == 0) return;
    u32 b = load<u8>(o + 572);
    ctx.gpr[3] = b;
    if ((b & 0x20u) != 0) return;
    if ((b & 0x10u) == 0) {
        if ((b & 0x08u) != 0) return;
        ctx.fpr[1] = Lfs32(o + 564);
        ctx.fpr[2] = Lfs32(kaddr);
        if (Lfs32(o + 564) > Lfs32(kaddr)) {
            if (static_cast<s32>(VCall(ctx, o, 32, {o, 0})) != 1) return;
            b = load<u8>(o + 572);
            ctx.gpr[3] = b;
        }
    }
    const f64 k = Lfs32(kaddr);
    const u32 d = load<u32>(o + 216);
    if (late) {
        if ((load<u32>(load<u32>(m + 4) + 188) & 1u) == 0) return;
        ctx.fpr[1] = Lfs32(d + doff);
        ctx.fpr[2] = k;
        if (!(Lfs32(d + doff) > k)) return;
    } else {
        ctx.fpr[1] = k;
        ctx.fpr[2] = Lfs32(d + doff);
        if (!(Lfs32(d + doff) > k)) return;
        if (range) {
            // The range variant: also +260 > k and the message's +48 > +260;
            // armed with +560 = 0.
            const f64 hi = Lfs32(d + 260);
            ctx.fpr[1] = hi;
            ctx.fpr[2] = k;
            if (!(hi > k)) return;
            ctx.fpr[2] = Lfs32(m + 48);
            if (!(Lfs32(m + 48) > hi)) return;
            store<u8>(o + 572, static_cast<u8>(b | 0x40u));
            store<u32>(o + 560, 0);
            ctx.gpr[3] = b | 0x40u;
            return;
        }
        if ((load<u32>(load<u32>(m + 4) + 188) & 1u) == 0) return;
    }
    store<u8>(o + 572, static_cast<u8>(b | 0x40u));
    const u32 v = static_cast<u32>(static_cast<s32>(static_cast<s8>(load<u8>(load<u32>(m + 4) + 172))));
    store<u32>(o + 560, v);
    ctx.gpr[3] = static_cast<u64>(static_cast<s64>(static_cast<s32>(v)));
}

// ~T() body of 0x000DD8A4 / 0x000E9820: with a skeleton o->+sk, vtable
// vt and AnimSkel_Release7D4EA8(it); then vtable 0x00B12230, the block
// +128 freed (Mem_FreeFrom(b, 0)) and cleared, Obj_Register7_00490518(o),
// the id +52 removed from the map 0x01013B04 (SortedMap_Remove744748 on
// a stack copy), and the pointer vectors +136 / +16 (when set) cleared
// and SmallFree'd (20 bytes). r3 as left.
inline void DtorSkel34(PPUContext& ctx, u32 sk, u32 vt) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 s = load<u32>(o + sk);
    if (s != 0) {
        store<u32>(o, vt);
        call(ctx, 0x007D4EA8, {s});
    }
    const u32 b = load<u32>(o + 128);
    store<u32>(o, 0x00B12230u);
    if (b != 0) {
        call(ctx, 0x0054EE6C, {b, 0});
        store<u32>(o + 128, 0);
    }
    call(ctx, 0x00490518, {o});
    {
        Stack frame(ctx, 16);
        const u32 k = frame.addr();  // sp+112
        store<u32>(k, load<u32>(o + 52));
        call(ctx, 0x00744748, {0x01013B04u, k});
    }
    for (const u32 off : {136u, 16u}) {
        const u32 v = load<u32>(o + off);
        if (v == 0) continue;
        call(ctx, 0x0054EC1C, {v});
        call(ctx, 0x0054EC18, {v});
        call(ctx, 0x00552420, {load<u32>(o + off), 20});
    }
}

// The tail of the ~T() of 0x001AE468 (as obj33's Dtor3Vt33): the
// 20-byte pointer vector +388 (when set, the vtables first becoming those
// of 0x00B12890) cleared and SmallFree'd; then vtables 0x00B126B8 /
// +380 0x00B1200C / +384 0x00B12528 and VtB12550_Dtor_000DF934(o). r3 as
// that left it.
inline void DtorTail388_34(PPUContext& ctx, u32 o) {
    const u32 q = load<u32>(o + 388);
    if (q != 0) {
        constexpr u32 w = 0x00B12890;
        store<u32>(o + 384, w + 476);
        store<u32>(o + 380, w + 436);
        store<u32>(o, w + 8);
        PvClear32(ctx, q);
        call(ctx, 0x00552420, {load<u32>(o + 388), 20});
    }
    store<u32>(o + 384, 0x00B12528);
    store<u32>(o + 380, 0x00B1200C);
    store<u32>(o, 0x00B126B8);
    call(ctx, 0x000DF934, {o});
}

// The timed blend of 0x00787070 / 0x00787190 (void f(World* o; f1 dt)) on
// the record b = o + off (start +0, end +4, value +8, done-time +12,
// duration +16, time +20, flags +24): while flag 0x80000000 is set,
// either it finishes (time >= done-time >= k[0]: flag cleared, value =
// end) or the time advances by dt (when |duration| >= k[2]) and value =
// start + (end - start) * min(time / duration, k[1]) (k[1] when the
// duration is tiny). r3 = o; f1 = the value stored (dt when idle).
inline void TimedLerp34(PPUContext& ctx, u32 off, u32 K) {
    const u32 b = arg<u32>(ctx, 0) + off;
    const u32 w = load<u32>(b + 24);
    if ((w >> 31) == 0) return;
    const f64 t = Lfs32(b + 20), lim = Lfs32(b + 12);
    if (t >= lim && lim >= Lfs32(K)) {
        CopyW32(b + 8, b + 4);
        store<u32>(b + 24, w & 0x7FFFFFFFu);
        ctx.fpr[1] = Lfs32(b + 4);
        return;
    }
    f64 f = Lfs32(K + 4);
    const f64 d = Lfs32(b + 16);
    if (!(std::fabs(d) < Lfs32(K + 8))) {
        const f64 t2 = static_cast<f32>(t + ctx.fpr[1]);
        const f64 u = static_cast<f32>(t2 / d);
        Stfs32(b + 20, t2);
        if (u < f) f = u;
    }
    const f64 a = Lfs32(b);
    const f64 v = static_cast<f32>(std::fma(f, static_cast<f32>(Lfs32(b + 4) - a), a));
    Stfs32(b + 8, v);
    ctx.fpr[1] = v;
}

// ~T() of 0x00753258 and its thunks 0x007536D4 / 0x007537E0: vtables
// 0x00B160A8 / +380 0x00B16230 / +384 0x00B16258, the shape +588
// released, the pointer vector +412 cleared, the allocator-backed u16
// vector +388 freed and destructed, then vtables 0x00B126B8 / 0x00B1200C
// / 0x00B12528 and VtB12550_Dtor_000DF934(o). r3 as that left it.
inline void DtorB160A8_34(PPUContext& ctx, u32 o) {
    constexpr u32 vt = 0x00B160A0u;
    store<u32>(o, vt + 8);
    store<u32>(o + 380, vt + 400);
    store<u32>(o + 384, vt + 440);
    call(ctx, 0x00638498, {o + 588});
    PvClear32(ctx, o + 412);
    FreeAllocVec34(ctx, o + 388);
    store<u32>(o + 384, 0x00B12528u);
    store<u32>(o + 380, 0x00B1200Cu);
    store<u32>(o, 0x00B126B8u);
    call(ctx, 0x000DF934, {o});
}

// The kind-5 component target of x for key (as obj31's TargetCall31):
// c = Effect_FindComponent(x, key) with c->vt[32](c) == 5, its +28 whose
// +276 is 0x02235C33 gives t = +280. Returns t (0 when any step fails).
inline u32 CompTarget34(PPUContext& ctx, u32 x, u32 key) {
    const u32 c = static_cast<u32>(call(ctx, 0x004BB4B4, {x, key}));
    if (c == 0 || static_cast<s32>(VCall(ctx, c, 32, {c})) != 5) return 0;
    const u32 c2 = load<u32>(c + 28);
    if (c2 == 0 || load<u32>(c2 + 276) != 0x02235C33u) return 0;
    return load<u32>(c2 + 280);
}

// T* New(-, Xml* x) of 0x0007AD68 / 0x00088F38: a 92-byte object
// (Mem_Alloc(92, 0, 16); 0 on failure): Obj_InitVt_B21C98(m, x), vtable
// vt, Obj_Clear68_0049E860(m + 12), u8 +88 = 0; the first child of x that
// 0x0049EC08(c, m + 12) accepts, then the bool key to +88. r3 = m.
inline void NewXmlList34(PPUContext& ctx, u32 vt, u32 key) {
    const u32 x = arg<u32>(ctx, 1);
    const u32 m = static_cast<u32>(call(ctx, 0x0054ED38, {92, 0, 16}));
    if (m != 0) {
        call(ctx, 0x004BB3B0, {m, x});
        store<u32>(m, vt);
        call(ctx, 0x0049E860, {m + 12});
        store<u8>(m + 88, 0);
        for (u32 c = static_cast<u32>(call(ctx, 0x004F9140, {x})); c != 0; c = static_cast<u32>(call(ctx, 0x004F8D74, {c}))) {
            if (static_cast<u32>(call(ctx, 0x0049EC08, {c, m + 12})) != 0) break;
        }
        call(ctx, 0x004F986C, {x, key, m + 88, load<u8>(m + 88)});
    }
    ctx.gpr[3] = m;
}

// The physics toggle of 0x00184934 / 0x00192220 (void f(T* o, u8 off)):
// Flag_Set172_80_000DA0CC(o, off); the body b = o->+388 (when set) is
// taken out of its world (Hk_RemoveEntity92FFC8 into a stack result)
// when off and in one, or, when on and in none and o->vt[380](o) (r4 =
// the world's group set, entry in r5, OPD in r6) is state, added to the
// group set's world (Hk_AddEntity92F2A8(w, b, 1)) with its skeleton +392
// (when set) attached (0x0092E9D8(w, s)). r3 as left.
inline void PhysToggle34(PPUContext& ctx, s32 state) {
    const u32 o = arg<u32>(ctx, 0), off = arg<u32>(ctx, 1) & 0xFFu;
    call(ctx, 0x000DA0CC, {o, off});
    const u32 b = load<u32>(o + 388);
    ctx.gpr[3] = load<u32>(o);  // the vtable, loaded for the call below
    if (b == 0) return;
    const u32 w = load<u32>(b + 8);
    if ((w != 0) != (off != 0)) return;
    if (off != 0) {
        Stack frame(ctx, 32);
        call(ctx, 0x0092FFC8, {frame.addr(), w, b});  // sp+112
        return;
    }
    const u32 tbl = load<u32>(load<u32>(0x00F65CA0) + 0x1281D4u);
    const u32 g = load<u32>(tbl + 4);
    const u32 opd = load<u32>(load<u32>(o) + 380);
    if (static_cast<s32>(call_ptr(ctx, opd, {o, tbl, load<u32>(opd), opd})) != state) return;
    call(ctx, 0x0092F2A8, {g, load<u32>(o + 388), 1});
    const u32 s = load<u32>(o + 392);
    if (s != 0) call(ctx, 0x0092E9D8, {g, s});
}

// bool f(T* o) of 0x0017C6D8 / 0x002824E8: any of the state flags
// 0xFFC06800 of +192 or 0xFD of +196 set.
inline void AnyStateFlag34(PPUContext& ctx) {
    const u32 p = arg<u32>(ctx, 0) + 192u;
    constexpr u32 m0 = 0x800u | 0x2000u | 0x4000u | 0x400000u | 0x800000u | 0x01000000u | 0x02000000u | 0x04000000u | 0x08000000u |
                       0x10000000u | 0x20000000u | 0x40000000u | 0x80000000u;
    ctx.gpr[3] = (load<u32>(p) & m0) != 0 || (load<u32>(p + 4) & 0xFDu) != 0;
}

// The reset of 0x00427BF0 / 0x00427E2C (identical bodies; void f(T* o)):
// vtable 0x00B20CB0; a 304-byte request on the stack ({0, u16 0, 8} then
// 292 bytes memset to 0); for each 12-byte entry of o (+24, count +16)
// whose s32 +8 is at least 1, the request's head = {o->+4, u16 o->+8}
// and, with an id table o->+28, Import_ABDDC4((s32)ids[i], &req, 304, 0)
// (the count re-read after each). r3 = the count.
inline void SendRequests34(PPUContext& ctx) {
    const u32 o = arg<u32>(ctx, 0);
    Stack frame(ctx, 304);
    const u32 rq = frame.addr();  // sp+112
    store<u32>(o, 0x00B20CB0u);
    store<u32>(rq, 0);
    store<u16>(rq + 4, 0);
    store<u32>(rq + 8, 8);
    call(ctx, 0x00726E54, {rq + 12, 0, 292});
    u32 n = load<u32>(o + 16);
    for (u32 i = 0; i < n; ++i) {
        if (static_cast<s32>(load<u32>(load<u32>(o + 24) + 12 * i + 8)) < 1) continue;
        const u32 ids = load<u32>(o + 28);
        store<u32>(rq, load<u32>(o + 4));
        store<u16>(rq + 4, load<u16>(o + 8));
        if (ids == 0) continue;
        call(ctx, 0x00ABDDC4, {Sx33(load<u32>(ids + ((i << 2) & 0x3FFFCu) + 4)), rq, 304, 0});
        n = load<u32>(o + 16);
    }
    ctx.gpr[3] = n;
}

// The reset of 0x004B3C70 / 0x004B54D0 (void f(T* o)): the slot of the
// name (String_HashNoCase(name)) in the five 20-byte entries of
// 0x00F65F88 (-1 and a "..." (0x00AEE4FC) report when missing) to u16
// +0; the words +4..+16 / +32..+48 cleared, +24 = 0.25, +28 = 1.0, the
// flags 0xF4 of u8 +20 cleared, u8 +21 = 0, u16 +54 = 0 and the u16s
// +52 / +56 / +58 / +last = -1. r3 = the slot.
inline void AnimSlotReset34(PPUContext& ctx, u32 name, u32 last) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 h = static_cast<u32>(call(ctx, 0x0056A710, {name}));
    u32 i = 0;
    while (i < 5 && load<u32>(0x00F65F88u + 20 * i) != h) ++i;
    u64 idx = i;
    if (i >= 5) {
        call(ctx, 0x005534D0, {0x00AEE4FCu, name});
        idx = ~0ull;
    }
    u32 b = load<u8>(o + 20);
    store<u32>(o + 4, 0);
    b &= ~0x80u;
    store<u32>(o + 8, 0);
    store<u32>(o + 12, 0);
    b &= ~0x40u;
    store<u32>(o + 16, 0);
    store<u32>(o + 24, 0x3E800000u);
    b &= ~0x04u;
    store<u32>(o + 28, 0x3F800000u);
    b &= ~0x20u;
    store<u16>(o + 54, 0);
    store<u32>(o + 44, 0);
    b &= ~0x10u;
    store<u32>(o + 48, 0);
    store<u8>(o + 20, static_cast<u8>(b));
    store<u32>(o + 36, 0);
    store<u32>(o + 40, 0);
    store<u32>(o + 32, 0);
    store<u8>(o + 21, 0);
    store<u16>(o + 52, 0xFFFF);
    store<u16>(o, static_cast<u16>(idx));
    store<u16>(o + 56, 0xFFFF);
    store<u16>(o + 58, 0xFFFF);
    store<u16>(o + last, 0xFFFF);
    ctx.gpr[3] = idx;
}

// T* New(-, Xml* x) of the small XML-read records (0x0006BE14 and
// others): an object of size bytes (Mem_Alloc(size, 0, 16); 0 on
// failure), Obj_InitVt_B21C98(m, x), then body(m, x) for the fields. r3 =
// m.
template <class F>
inline void NewXml34(PPUContext& ctx, u32 size, F body) {
    const u32 x = arg<u32>(ctx, 1);
    const u32 m = static_cast<u32>(call(ctx, 0x0054ED38, {size, 0, 16}));
    if (m != 0) {
        call(ctx, 0x004BB3B0, {m, x});
        body(m, x);
    }
    ctx.gpr[3] = m;
}
// XmlCursor_ReadHash(x, key, m + off, dflt) and friends, for NewXml34
// bodies (the default "current" ones pass the field's own value).
inline void RdHash34(PPUContext& ctx, u32 x, u32 key, u32 a, u32 dflt) { call(ctx, 0x004FA14C, {x, key, a, dflt}); }
inline void RdHashCur34(PPUContext& ctx, u32 x, u32 key, u32 a) { call(ctx, 0x004FA14C, {x, key, a, load<u32>(a)}); }
inline void RdBoolCur34(PPUContext& ctx, u32 x, u32 key, u32 a) { call(ctx, 0x004F986C, {x, key, a, load<u8>(a)}); }
inline void RdF32Cur34(PPUContext& ctx, u32 x, u32 key, u32 a) { callf(ctx, 0x004F9B20, {x, key, a}, {Lfs32(a)}); }

}  // namespace obj34
}  // namespace sly4
