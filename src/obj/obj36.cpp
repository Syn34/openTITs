// Game code, part 36: the small-function sweep continued from the last 70
// instruction functions (size order; obj35.cpp did 69-70). Each was
// checked against the original in the PPC simulator (memory writes, calls
// and their arguments, r3 and f1).
#include "obj36.h"

using namespace sly4;
using namespace sly4::obj36;

// ---------------------------------------------------------------- sweep: 70 instructions
// void f(T* o): when bit 0x80000000 of o->+136 is set, vt[156](p) for each
// set word p of o->+204..+236 (9); then, when that bit is (still) set, the
// same call for each object of the chain from o->+152 linked by +156. r3:
// 0 (flag clear), 1 (empty chain) or the last call's.
DECOMP(004E81C0, Obj_VCall156Chain_004E81C0) {
    const u32 o = arg<u32>(ctx, 0);
    if ((load<u32>(o + 136) >> 31) != 0) {
        for (u32 i = 0; i < 9; ++i) {
            const u32 p = load<u32>(o + 204 + 4 * i);
            if (p != 0) VCall(ctx, p, 156, {p});
        }
    }
    if ((load<u32>(o + 136) >> 31) == 0) {
        ret(ctx, 0u);
        return;
    }
    u32 q = load<u32>(o + 152);
    if (q == 0) {
        ret(ctx, 1u);
        return;
    }
    do {
        VCall(ctx, q, 156, {q});
        q = load<u32>(q + 156);
    } while (q != 0);
}

// void f(T* o): resets the two buffers of o: +40 cleared; when bit 31 of
// +44 is clear, vtable 0x00B1B940 and the allocator g (0x011DF31C)
// vt[20](g, +36, +44 << 2) frees the first; +36 = 0, +44 = 0x80000000,
// +20 = 0; when bit 31 of +24 is clear, vtable 0x00F29900 and
// vt[20](g, +16, +24 & 0x3FFFFFFF) frees the second; +16 = 0, +24 =
// 0x80000000, vtable 0x00B13974. (Both calls leave the entry address in
// the register after the arguments and the OPD after it.) r3 = 0.
DECOMP(0075D220, Buf2_Reset_0075D220) {
    const u32 o = arg<u32>(ctx, 0);
    constexpr u32 g = 0x011DF31Cu;
    const u32 w = load<u32>(o + 44);
    store<u32>(o + 40, 0);
    if ((w & 0x80000000u) == 0) {
        store<u32>(o, 0x00B1B940u);
        VCallEO36(ctx, load<u32>(g), 20, {g, load<u32>(o + 36), w << 2});
    }
    const u32 w2 = load<u32>(o + 24);
    store<u32>(o + 36, 0);
    store<u32>(o + 44, 0x80000000u);
    store<u32>(o + 20, 0);
    if ((w2 & 0x80000000u) == 0) {
        store<u32>(o, 0x00F29900u);
        VCallEO36(ctx, load<u32>(g), 20, {g, load<u32>(o + 16), w2 & 0x3FFFFFFFu});
    }
    store<u32>(o + 16, 0);
    store<u32>(o + 24, 0x80000000u);
    store<u32>(o, 0x00B13974u);
    ret(ctx, 0u);
}

// u32 f(Vec32* v, const E* e): push_back of a 32-byte element (data +0,
// u16 capacity +4, u16 count +8, u16 high-water +10; the capacity is
// doubled by PtrVector_Reserve when full): words +0/+4/+8 (floats, copied
// raw), +16, +20 copied, and bits 31..25 of +24 copied one at a time (the
// rest of the destination word kept), each store done as in the original;
// count + 1, high-water = max. r3 = new count - 1; f1 = the float +8.
DECOMP(0076A914, Vec32_Push_0076A914) {
    const u32 v = arg<u32>(ctx, 0), e = arg<u32>(ctx, 1);
    u32 cnt = load<u16>(v + 8);
    const u32 cap = load<u16>(v + 4);
    if (cnt == cap) {
        call(ctx, 0x00551718, {v, (cap << 1) & 0x1FFFEu});
        cnt = load<u16>(v + 8);
    }
    const u32 d = load<u32>(v) + ((cnt << 5) & 0x1FFFE0u);
    store<u32>(d, load<u32>(e));
    store<u32>(d + 4, load<u32>(e + 4));
    const u32 f8 = load<u32>(e + 8);
    store<u32>(d + 8, f8);
    const u32 w16 = load<u32>(e + 16);
    u32 x = load<u32>(d + 24) & 0x7FFFFFFFu;
    store<u32>(d + 16, w16);
    store<u32>(d + 20, load<u32>(e + 20));
    x |= load<u32>(e + 24) & 0x80000000u;
    store<u32>(d + 24, x);
    for (u32 b = 0x40000000u; b >= 0x02000000u; b >>= 1) {
        x = (x & ~b) | (load<u32>(e + 24) & b);
        store<u32>(d + 24, x);
    }
    const u32 n = (load<u16>(v + 8) + 1u) & 0xFFFFu;
    const u32 hw = load<u16>(v + 10);
    store<u16>(v + 8, static_cast<u16>(n));
    store<u16>(v + 10, static_cast<u16>(hw > n ? hw : n));
    ctx.fpr[1] = static_cast<f64>(std::bit_cast<f32>(f8));
    ret(ctx, n - 1u);
}

// ---------------------------------------------------------------- sweep: 71 instructions
// void f(T* o, X* x): nothing when o->+20 is 0. The target: when o->+12
// names a component of x (Effect_FindComponent) whose vt[32]() is 5 and
// whose +28 owner has the class hash 0x02235C33 at +276, that owner's
// +280; otherwise Obj_FindById(o->+16) when +16 is set. A target goes
// through Obj_Notify26 (0x000D94CC) and, when that gives one,
// 0x002ABD08(it, o->+20). r3 = 0 or that call's.
DECOMP(00086F18, Effect_ApplyToTarget_00086F18) {
    const u32 o = arg<u32>(ctx, 0), x = arg<u32>(ctx, 1);
    if (load<u32>(o + 20) == 0) {
        ret(ctx, 0u);
        return;
    }
    u32 t = 0;
    const u32 id = load<u32>(o + 12);
    if (id != 0) {
        const u32 c = static_cast<u32>(call(ctx, 0x004BB4B4, {x, id}));
        if (c != 0 && static_cast<s32>(VCall(ctx, c, 32, {c})) == 5) {
            const u32 p = load<u32>(c + 28);
            if (p != 0 && load<u32>(p + 276) == 0x02235C33u) t = load<u32>(p + 280);
        }
    }
    if (t == 0) {
        const u32 k = load<u32>(o + 16);
        if (k != 0) t = static_cast<u32>(call(ctx, 0x000D6A28, {k}));
    }
    if (t == 0) {
        ret(ctx, 0u);
        return;
    }
    t = static_cast<u32>(call(ctx, 0x000D94CC, {t}));
    if (t == 0) {
        ret(ctx, 0u);
        return;
    }
    call(ctx, 0x002ABD08, {t, load<u32>(o + 20)});
}

// T* New(-, Xml* x): NewXml34 (28 bytes): +12/+16/+20 cleared, vtable
// 0x00B10940, +24 cleared; the hash 0x8A93973F to +12 (default 0), the
// hashes 0x7A30D7E3 / 0x9178C4CB to +16 / +20 and the s32 0x3ACE549F to
// +24 (current defaults).
DECOMP(00090EC4, VtB10940_NewXml_00090EC4) {
    NewXml34(ctx, 28, [&](u32 m, u32 x) {
        store<u32>(m + 12, 0);
        store<u32>(m + 16, 0);
        store<u32>(m + 20, 0);
        store<u32>(m, 0x00B10940u);
        store<u32>(m + 24, 0);
        RdHash34(ctx, x, 0x8A93973Fu, m + 12, 0);
        RdHashCur34(ctx, x, 0x7A30D7E3u, m + 16);
        RdHashCur34(ctx, x, 0x9178C4CBu, m + 20);
        RdS32Cur36(ctx, x, 0x3ACE549Fu, m + 24);
    });
}

// T* New(-, Xml* x): NewXml34 (20 bytes): +12 cleared, u8 +16/+17
// cleared, vtable 0x00B10E18; the hash 0x8A93973F to +12 (default 0; when
// that read finds nothing, the s32 0x5C8998CD to +12, default 0), then the
// bools 0x7A281983 / 0xF5897C08 to +16 / +17 (default 0).
DECOMP(00099924, VtB10E18_NewXml_00099924) {
    NewXml34(ctx, 20, [&](u32 m, u32 x) {
        store<u32>(m + 12, 0);
        store<u8>(m + 16, 0);
        store<u8>(m + 17, 0);
        store<u32>(m, 0x00B10E18u);
        if (static_cast<u32>(call(ctx, 0x004FA14C, {x, 0x8A93973Fu, m + 12, 0})) != 0)
            call(ctx, 0x004F99BC, {x, 0x5C8998CDu, m + 12, 0});
        call(ctx, 0x004F986C, {x, 0x7A281983u, m + 16, 0});
        call(ctx, 0x004F986C, {x, 0xF5897C08u, m + 17, 0});
    });
}

// void f(T* o, u8 on): bit 0x20000 of o->+172 = !on; when the global byte
// 0x01022014 is set and o->vt[68]() gives r, r->vt[44](r, on); then,
// unless o has the component 32 (Obj_FindComponent(o, 32, 0); r3 = it),
// o's world peer vector flag 0x20 = !on (PeerVecFlag36).
DECOMP(000DD0A4, Obj_SetHidden_000DD0A4) {
    const u32 o = arg<u32>(ctx, 0), on = arg<u32>(ctx, 1) & 0xFFu;
    const bool off = on == 0;
    store<u32>(o + 172, (load<u32>(o + 172) & ~0x20000u) | (off ? 0x20000u : 0u));
    if (load<u8>(0x01022014u) != 0) {
        const u32 r = static_cast<u32>(VCall(ctx, o, 68, {o}));
        if (r != 0) VCall(ctx, r, 44, {r, on});
    }
    if (static_cast<u32>(call(ctx, 0x000D1288, {o, 32, 0})) != 0) return;
    PeerVecFlag36(ctx, o, off);
}

// void f(T* o): r->vt[44](r, 0) for r = o->vt[68]() and for r =
// o->vt[64]() (when set), then o's world peer vector flag 0x20 set
// (PeerVecFlag36).
DECOMP(000DD504, Obj_HideParts_000DD504) {
    const u32 o = arg<u32>(ctx, 0);
    u32 r = static_cast<u32>(VCall(ctx, o, 68, {o}));
    if (r != 0) VCall(ctx, r, 44, {r, 0});
    r = static_cast<u32>(VCall(ctx, o, 64, {o}));
    if (r != 0) VCall(ctx, r, 44, {r, 0});
    PeerVecFlag36(ctx, o, true);
}

// u32 f(T* o, u32 a, s32 kind, f32* amount): 0 unless o->+396 is 1 or
// above 6 (unsigned) and kind is 0 or 4. Kind 0: 0 when the five floats
// +212/+196/+204/+224/+232 of o->+216 all equal the constant at
// 0x00169278, or when a is set and equals o->+408; else *amount is scaled
// by the constant at 0x0016927C. Then 0x000DF960(o, a, kind, amount)
// (r3 = its). f1 as the original leaves it.
DECOMP(00169280, Obj_FilterHit_00169280) {
    const u32 o = arg<u32>(ctx, 0), a = arg<u32>(ctx, 1), p = arg<u32>(ctx, 3);
    const s32 k = arg<s32>(ctx, 2);
    const u32 s = load<u32>(o + 396);
    const u32 t = load<u32>(o + 216);
    if (!(s == 1 || s > 6)) {
        ret(ctx, 0u);
        return;
    }
    if (k == 0) {
        const f64 c = Lfs32(0x00169278u);
        ctx.fpr[1] = c;
        if (Lfs32(t + 212) == c && Lfs32(t + 196) == c && Lfs32(t + 204) == c && Lfs32(t + 224) == c &&
            Lfs32(t + 232) == c) {
            ret(ctx, 0u);
            return;
        }
        if (a != 0 && load<u32>(o + 408) == a) {
            ret(ctx, 0u);
            return;
        }
        const f64 v = static_cast<f32>(Lfs32(p) * Lfs32(0x0016927Cu));
        Stfs32(p, v);
        ctx.fpr[1] = v;
    } else if (k != 4) {
        ret(ctx, 0u);
        return;
    }
    call(ctx, 0x000DF960, {o, a, static_cast<u32>(k), p});
}

// void f(T* o): after Nop_000D8FC0(o), when o->+348 is above the constant
// at 0x0017C7E8 (K), u8 +433 is set, bit 0x100 of +172 is set and
// Math_Cosf(+400 * three constants at 0x0017C7EC..) is above K:
// Obj_ApplyTree_000D8F08(o, o + 434, 1), u8 +432 = bit 0x8000 of
// o->+160->+164, and o->vt[88](o, 0, 1, 1, 0). r3 / f1 as left.
DECOMP(0017C7F8, Obj_CheckCone_0017C7F8) {
    const u32 o = arg<u32>(ctx, 0);
    call(ctx, 0x000D8FC0, {o});
    const f64 K = Lfs32(0x0017C7E8u);
    const f64 v = Lfs32(o + 348);
    ctx.fpr[1] = v;
    if (!(v > K)) return;
    const u32 b = load<u8>(o + 433);
    ctx.gpr[3] = b;
    if (b == 0) return;
    const u32 fl = (load<u32>(o + 172) >> 8) & 1u;
    ctx.gpr[3] = fl;
    if (fl == 0) return;
    f64 x = static_cast<f32>(Lfs32(o + 400) * Lfs32(0x0017C7ECu));
    x = static_cast<f32>(x * Lfs32(0x0017C7F0u));
    x = static_cast<f32>(x * Lfs32(0x0017C7F4u));
    if (!(callf(ctx, 0x007344E0, {}, {x}) > K)) return;
    call(ctx, 0x000D8F08, {o, o + 434, 1});
    const u32 bit = (load<u32>(load<u32>(o + 160) + 164) >> 15) & 1u;
    const u32 vt = load<u32>(o);
    store<u8>(o + 432, static_cast<u8>(bit));
    VCallVt(ctx, vt, 88, {o, 0, 1, 1, 0});
}

// void f(T* o) (destructor body): vtable 0x00B151A0; when o->+1896 is set,
// +1900->vt[8]() and +1896->vt[8]() (each word cleared after its call),
// then for i < u16 +1888, +432 + 96i ->vt[8]() and that word cleared;
// finally Thunk_000D729C(o) (r3 = its).
DECOMP(001A7D8C, VtB151A0_Dtor_001A7D8C) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 a = load<u32>(o + 1896);
    store<u32>(o, 0x00B151A0u);
    if (a != 0) {
        u32 p = load<u32>(o + 1900);
        VCall(ctx, p, 8, {p});
        store<u32>(o + 1900, 0);
        p = load<u32>(o + 1896);
        VCall(ctx, p, 8, {p});
        const u32 n = load<u16>(o + 1888);
        store<u32>(o + 1896, 0);
        if (n != 0) {
            u32 i = 0;
            do {
                const u32 e = o + 432 + 96 * i;
                p = load<u32>(e);
                VCall(ctx, p, 8, {p});
                ++i;
                store<u32>(e, 0);
            } while (i < load<u16>(o + 1888));
        }
    }
    call(ctx, 0x000D729C, {o});
}

// void f(T* o, u8 on): on: unless o->+496 is set (r3 = it),
// 0x001F50E4(o) and o->vt[28](o, 0, 0). Off: when +496 (h) is set,
// Obj_Detach0D72C0(o, h), h->vt[40](h, 0), h->vt[40](h, 1), +496 = 0 and
// o->vt[28](o, 0, 3) (the vtable read before that store; entry / OPD
// left in r6 / r7). r3 as left.
DECOMP(001F7168, Obj_SetHeld_001F7168) {
    const u32 o = arg<u32>(ctx, 0), on = arg<u32>(ctx, 1) & 0xFFu;
    if (on != 0) {
        const u32 h = load<u32>(o + 496);
        ctx.gpr[3] = h;
        if (h != 0) return;
        call(ctx, 0x001F50E4, {o});
        VCall(ctx, o, 28, {o, 0, 0});
        return;
    }
    const u32 h = load<u32>(o + 496);
    ctx.gpr[3] = h;
    if (h == 0) return;
    call(ctx, 0x000D72C0, {o, h});
    u32 p = load<u32>(o + 496);
    VCall(ctx, p, 40, {p, 0});
    p = load<u32>(o + 496);
    VCall(ctx, p, 40, {p, 1});
    const u32 vt = load<u32>(o);
    store<u32>(o + 496, 0);
    VCallEO36(ctx, vt, 28, {o, 0, 3});
}

// bool f(T* o; f1 = dt): the timer o->+1000 against the settings d =
// o->+396 + 1528 (K = the constant at 0x00238C9C): false without
// settings or when d +8 (r) is below K. A running timer (>= K) advances
// by dt; otherwise, with u8 d +20 set, unless +972 and +980 both equal K,
// it is reset to K (word 0) when r^2 < |o +576..+584|^2; with d +20 clear,
// true when d +12 <= o->+988 and r^2 >= that length. Result: d +12 <=
// timer. f1 as left.
DECOMP(00238CA0, Obj_TimerReady_00238CA0) {
    const u32 o = arg<u32>(ctx, 0);
    const f64 dt = farg(ctx, 0);
    const u32 q = load<u32>(o + 396);
    if (q == 0) {
        ret(ctx, 0u);
        return;
    }
    const u32 d = q + 1528;
    const f64 K = Lfs32(0x00238C9Cu);
    const f64 r = Lfs32(d + 8);
    if (!(r >= K)) {
        ret(ctx, 0u);
        return;
    }
    f64 tm = Lfs32(o + 1000);
    auto len2 = [&]() {
        const f64 y = Lfs32(o + 580), x = Lfs32(o + 576), z = Lfs32(o + 584);
        f64 s = static_cast<f32>(y * y);
        s = static_cast<f32>(std::fma(x, x, s));
        return static_cast<f64>(static_cast<f32>(std::fma(z, z, s)));
    };
    if (tm >= K) {
        tm = static_cast<f32>(tm + dt);
        Stfs32(o + 1000, tm);
    } else {
        const f64 sq = static_cast<f32>(r * r);
        if (load<u8>(d + 20) != 0) {
            if (!(Lfs32(o + 972) == K && Lfs32(o + 980) == K) && sq < len2()) {
                store<u32>(o + 1000, 0);
                tm = K;
            }
        } else if (Lfs32(d + 12) <= Lfs32(o + 988) && sq >= len2()) {
            ctx.fpr[1] = sq;
            ret(ctx, 1u);
            return;
        }
    }
    const f64 e = Lfs32(d + 12);
    ctx.fpr[1] = e;
    ret(ctx, e <= tm ? 1u : 0u);
}

// u16 f(T* o, Filters* f): removes from o's word array (+64, u16 count
// +72) each entry e that matches none of f's 128-byte filters (+0, u16
// count +8; Filter_Match(e, filter)): 0x00299F74(o, e), count - 1 and the
// last word moved into its slot (unless it was the last); the scan then
// goes on with the next slot (so a moved-in entry is skipped, as in the
// original). r3 = the count read at the end.
DECOMP(0029E684, FilterList_RemoveUnmatched_0029E684) {
    const u32 o = arg<u32>(ctx, 0), f = arg<u32>(ctx, 1);
    u32 i = 0, off = 0;
    for (;;) {
        const u32 n = load<u16>(o + 72);
        if (!(i < n)) {
            ret(ctx, n);
            return;
        }
        const u32 e = load<u32>(load<u32>(o + 64) + off);
        u32 j = 0, joff = 0;
        while (j < load<u16>(f + 8)) {
            if (static_cast<u32>(call(ctx, 0x0029998C, {e, load<u32>(f) + joff})) != 0) break;
            joff += 128;
            ++j;
        }
        if (!(j < load<u16>(f + 8))) {
            call(ctx, 0x00299F74, {o, e});
            const u32 m = load<u16>(o + 72) - 1u;
            store<u16>(o + 72, static_cast<u16>(m));
            if (i != (m & 0xFFFFu)) {
                const u32 a = load<u32>(o + 64);
                store<u32>(a + off, load<u32>(a + ((m << 2) & 0x3FFFCu)));
            }
        }
        off += 4;
        ++i;
    }
}

// void f(T* o): when u8 o->+36 is set and the callback OPD o->+20 is set:
// with d = o->+32, the value v = the +4 word of d's 16-byte entry (table
// d->+76) number d->+104 when d->+32 is 5, else of the first entry whose
// key word equals o->+4 (d->+72 entries; 0 if none); then the callback
// (o->+4, o->+12, d->+40 when d->+32 is 5 else 0, v, o->+24; the OPD and
// its entry in r8 / r9). Then u8 +36 and +28 cleared. r3 = 0.
DECOMP(002EA344, Callback_Fire_002EA344) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 k = load<u32>(o + 4), a12 = load<u32>(o + 12), a24 = load<u32>(o + 24);
    if (load<u8>(o + 36) != 0) {
        const u32 fn = load<u32>(o + 20);
        if (fn != 0) {
            const u32 d = load<u32>(o + 32);
            u32 b = 0, v = 0;
            if (load<u32>(d + 32) == 5) {
                b = load<u32>(d + 40);
                v = load<u32>(load<u32>(d + 76) + (load<u32>(d + 104) << 4) + 4);
            } else {
                const u32 n = load<u32>(d + 72);
                for (u32 i = 0; i < n; ++i) {
                    const u32 t = load<u32>(d + 76);
                    if (load<u32>(t + 16 * i) == k) {
                        v = load<u32>(t + (i << 4) + 4);
                        break;
                    }
                }
            }
            call_ptr(ctx, fn, {k, a12, b, v, a24, fn, load<u32>(fn)});
        }
    }
    store<u8>(o + 36, 0);
    store<u8>(o + 28, 0);
    ret(ctx, 0u);
}

// void f(T* o, s32 i): for the list p = o->+2232 + 8i (nothing for i = -1
// or an empty list; the list's array object *p: data +0, u16 count +8):
// each entry e with a set +20 and bit 0x08000000 of +28: when
// Entry_IsNew2E9B78(e->+20), bit 2 of byte +45 of *0x00F65CA0 set; then
// that +28 bit cleared. r3 = the list word as last read.
DECOMP(00325FCC, EntryList_Flush_00325FCC) {
    const u32 o = arg<u32>(ctx, 0);
    const s32 i = arg<s32>(ctx, 1);
    if (i == -1) {
        ret(ctx, o);
        return;
    }
    const u32 slot = o + static_cast<u32>(i) * 8u + 2232u;
    u32 p = load<u32>(slot);
    ctx.gpr[3] = p;
    if (p == 0 || load<u32>(p) == 0 || load<u16>(load<u32>(p) + 8) == 0) return;
    u32 off = 0;
    for (u32 k = 0;; ++k, off += 4) {
        const u32 a = load<u32>(p);
        const u32 n = a != 0 ? load<u16>(a + 8) : 0u;
        if (!(k < n)) break;
        const u32 e = load<u32>(load<u32>(a) + off);
        const u32 h = load<u32>(e + 20);
        if (h == 0 || (load<u32>(e + 28) & 0x08000000u) == 0) continue;
        if (static_cast<u32>(call(ctx, 0x002E9B78, {h})) != 0) {
            const u32 g = load<u32>(0x00F65CA0u);
            store<u8>(g + 45, static_cast<u8>(load<u8>(g + 45) | 2u));
        }
        store<u32>(e + 28, load<u32>(e + 28) & ~0x08000000u);
        p = load<u32>(slot);
    }
    ctx.gpr[3] = p;
}

// void f(T* o, T* other): Obj_ShowEntries_00325CE4(o, other); then, when
// o's u8 +18 is 4 (or 5 with other's u8 +18 4): for each 144-byte entry e
// of o (+2572, u16 count +2580), e->+20 = Obj_FindById(e->+16) for a set
// +16; an entry without +16, or whose object has bit 0x00040000 of +172,
// gets u8 +48 = (o's kind is 4); at the first other one,
// o->vt[104](o, 6, 0) and the scan stops. r3 as left.
DECOMP(0032C5E4, Obj_LinkEntries_0032C5E4) {
    const u32 o = arg<u32>(ctx, 0), other = arg<u32>(ctx, 1);
    call(ctx, 0x00325CE4, {o, other});
    const u32 t = load<u8>(o + 18);
    if (t != 4) {
        if (t != 5 || other == 0) return;
        const u32 u = load<u8>(other + 18);
        ctx.gpr[3] = u;
        if (u != 4) return;
    }
    const u32 is4 = t == 4 ? 1u : 0u;
    u32 off = 0;
    for (u32 i = 0;; ++i, off += 144) {
        const u32 n = load<u16>(o + 2580);
        if (!(i < n)) {
            ret(ctx, n);
            return;
        }
        const u32 e = load<u32>(o + 2572) + off;
        const u32 id = load<u32>(e + 16);
        if (id != 0) {
            const u32 x = static_cast<u32>(call(ctx, 0x000D6A28, {id}));
            store<u32>(e + 20, x);
            if (x == 0 || ((load<u32>(x + 172) >> 18) & 1u) == 0) {
                VCall(ctx, o, 104, {o, 6, 0});
                return;
            }
        }
        store<u8>(e + 48, static_cast<u8>(is4));
    }
}

// void f(T* o, a, u8 b, u8 c; f1 = dt): 0x003537A8(o, a, b, c; dt); then,
// when o->+24 and its +20 are set: unless the reference value (o->+32's
// +40 ->+12 ->+12 float, else 0) and o->+32's +8 / +64 (0 without +32)
// are all 0 and u8 o->+104 is clear: +100 = 0 and +20 = +16. Otherwise x
// = +100 clamped to [0, 0.25] (fsel), +20 = 1 - 4x and +100 = x + dt.
// r3 / f1 as left (r3 = the constant table 0x003546B0 on the last path).
DECOMP(003546D0, Obj_FadeStep_003546D0) {
    const u32 o = arg<u32>(ctx, 0);
    const f64 dt = farg(ctx, 0);
    callf(ctx, 0x003537A8, {o, arg<u32>(ctx, 1), arg<u32>(ctx, 2) & 0xFFu, arg<u32>(ctx, 3) & 0xFFu}, {dt});
    const u32 p = load<u32>(o + 24);
    ctx.gpr[3] = p;
    if (p == 0) return;
    const u32 p20 = load<u32>(p + 20);
    ctx.gpr[3] = p20;
    if (p20 == 0) return;
    constexpr u32 K = 0x003546B0u;
    const u32 q = load<u32>(o + 32);
    ctx.gpr[3] = q;
    f64 ref;
    if (q != 0 && load<u32>(q + 40) != 0) ref = Lfs32(load<u32>(load<u32>(q + 40) + 12) + 12);
    else ref = Lfs32(K);
    const f64 z = Lfs32(K);
    ctx.fpr[1] = z;
    bool tail = true;
    if (ref == (q != 0 ? Lfs32(q + 8) : z) && (q != 0 ? Lfs32(q + 64) : z) == z) {
        const u32 b = load<u8>(o + 104);
        if (b == 0) tail = false;
        else ctx.gpr[3] = b;
    }
    if (tail) {
        const u32 w16 = load<u32>(o + 16);
        Stfs32(o + 100, z);
        store<u32>(o + 20, w16);
        return;
    }
    const f64 cur = Lfs32(o + 100);
    const f64 d1 = std::bit_cast<f64>(load<u64>(K + 8)), d2 = std::bit_cast<f64>(load<u64>(K + 16));
    const f64 s = static_cast<f32>(Lfs32(K + 4) - cur);
    f64 x = static_cast<f32>(s >= 0.0 ? cur : d1);
    x = static_cast<f32>(x >= 0.0 ? x : d2);
    const f64 y = static_cast<f32>(-std::fma(x, Lfs32(K + 24), -Lfs32(K + 28)));
    const f64 nx = static_cast<f32>(x + dt);
    Stfs32(o + 20, y);
    Stfs32(o + 100, nx);
    ctx.fpr[1] = nx;
    ret(ctx, K);
}

// void f(T* o; f1 = dt): TimerTick36 with the timer +340 (K at
// 0x0036BA9C) and Obj_NotifyA362F1C(o, bit 0x00400000 / 0x00200000 of
// +396).
DECOMP(0036BAA0, Obj_TimerTick_0036BAA0) {
    TimerTick36(ctx, 340, 0x0036BA9Cu, 396, 0x00400000u, 0x00200000u, 0x00362F1Cu);
}

// void f(T* o; f1 = dt): TimerTick36 with the timer +200 (K at
// 0x00384084) and Obj_ModeOff368AC8(o, bit 0x00200000 / 0x00100000 of
// +532).
DECOMP(00384088, Obj_TimerTick_00384088) {
    TimerTick36(ctx, 200, 0x00384084u, 532, 0x00200000u, 0x00100000u, 0x00368AC8u);
}

// T* ctor(T* o, a, b, c, d; f1, f2, f3): Obj_InitVt_B20CC8(o, 1); +48 /
// +52 = f1 / f2, +44 = 0, +64..+76 = the four words at 0x01164C90, vtable
// 0x00B20F68, +12..+40 = 0, a, 0, b, 0, c, 0, d (in the original's store
// order), +80..+88 = the three words at 0x01164B40, +96 = f3, bit 31 of
// +100 cleared. r3 = that word; f1 = the float at 0x01164B48.
DECOMP(0042FF28, VtB20F68_Ctor_0042FF28) {
    const u32 o = arg<u32>(ctx, 0), a = arg<u32>(ctx, 1), b = arg<u32>(ctx, 2), c = arg<u32>(ctx, 3),
              d = arg<u32>(ctx, 4);
    const f64 f1 = farg(ctx, 0), f2 = farg(ctx, 1), f3 = farg(ctx, 2);
    call(ctx, 0x0042C9D0, {o, 1, b, c, d});
    constexpr u32 g1 = 0x01164C90u, g2 = 0x01164B40u;
    Stfs32(o + 48, f1);
    Stfs32(o + 52, f2);
    store<u32>(o + 44, 0);
    store<u32>(o + 64, load<u32>(g1));
    store<u32>(o + 68, load<u32>(g1 + 4));
    store<u32>(o, 0x00B20F68u);
    store<u32>(o + 12, 0);
    store<u32>(o + 72, load<u32>(g1 + 8));
    store<u32>(o + 16, a);
    store<u32>(o + 20, 0);
    const u32 w76 = load<u32>(g1 + 12);
    store<u32>(o + 24, b);
    store<u32>(o + 28, 0);
    store<u32>(o + 32, c);
    store<u32>(o + 36, 0);
    store<u32>(o + 76, w76);
    store<u32>(o + 40, d);
    const u32 w80 = load<u32>(g2);
    const u32 w100 = load<u32>(o + 100) & 0x7FFFFFFFu;
    store<u32>(o + 80, w80);
    store<u32>(o + 84, load<u32>(g2 + 4));
    const u32 w88 = load<u32>(g2 + 8);
    Stfs32(o + 96, f3);
    store<u32>(o + 88, w88);
    store<u32>(o + 100, w100);
    ctx.fpr[1] = static_cast<f64>(std::bit_cast<f32>(w88));
    ret(ctx, w100);
}

// void f(Msg* m): applies m's flag word +8 to the current target x =
// *(*0x00F65CA0 + 0x1281F4) (nothing without one, r3 = m; nothing unless
// bit 0x40 of x->+28, r3 = 0): bits 0x00800000 / 0x00400000 / 0x00200000
// / 0x00100000 / 0x00080000 / 0x00040000 call 0x003A4010 / 0x003AAAE0 /
// 0x003AAAF0 / 0x003AAB08 / 0x003AAB54 / 0x003A4000 on x (the flag word
// re-read after each call). r3 as left.
DECOMP(00437E08, Msg_ApplyTargetFlags_00437E08) {
    const u32 m = arg<u32>(ctx, 0);
    const u32 x = load<u32>(load<u32>(0x00F65CA0u) + 0x001281F4u);
    if (x == 0) return;
    if ((load<u32>(x + 28) & 0x40u) == 0) {
        ret(ctx, 0u);
        return;
    }
    u32 f = load<u32>(m + 8);
    if ((f & 0x00800000u) != 0) {
        call(ctx, 0x003A4010, {x, 1});
        f = load<u32>(m + 8);
    }
    if ((f & 0x00400000u) != 0) {
        call(ctx, 0x003AAAE0, {x, f, 1});
        f = load<u32>(m + 8);
    }
    if ((f & 0x00200000u) != 0) {
        call(ctx, 0x003AAAF0, {x, f, 1});
        f = load<u32>(m + 8);
    }
    if ((f & 0x00100000u) != 0) {
        call(ctx, 0x003AAB08, {x, 1});
        f = load<u32>(m + 8);
    }
    if ((f & 0x00080000u) != 0) {
        call(ctx, 0x003AAB54, {x, 1});
        f = load<u32>(m + 8);
    }
    if ((f & 0x00040000u) == 0) {
        ret(ctx, 0u);
        return;
    }
    call(ctx, 0x003A4000, {x});
}

// T* ctor(T* o, a, b, c, d, const Vec3* p, const Vec3* q, u8 e, u8 g
// [stack], u8 h [stack]): Obj_InitVt_B20CC8(o, 1); +16..+24 = *p,
// vtable 0x00B21328, +32..+40 = *q, +48 = 0, +52..+68 = a, b, 0, c, d
// (in the original's order), bits 31 / 30 / 29 of +72 = bit 0 of e / g
// / h. r3 = the vtable; f1 = q's z.
DECOMP(00439224, VtB21328_Ctor_00439224) {
    const u32 o = arg<u32>(ctx, 0), a = arg<u32>(ctx, 1), b = arg<u32>(ctx, 2), c = arg<u32>(ctx, 3),
              d = arg<u32>(ctx, 4), p = arg<u32>(ctx, 5), q = arg<u32>(ctx, 6), e = arg<u32>(ctx, 7);
    const u32 g = StkArg34(ctx, 116), h = StkArg34(ctx, 124);
    call(ctx, 0x0042C9D0, {o, 1});
    store<u32>(o + 16, load<u32>(p));
    store<u32>(o + 20, load<u32>(p + 4));
    const u32 pz = load<u32>(p + 8);
    u32 w = load<u32>(o + 72);
    store<u32>(o + 24, pz);
    const u32 qx = load<u32>(q);
    w &= 0x7FFFFFFFu;
    store<u32>(o, 0x00B21328u);
    store<u32>(o + 32, qx);
    const u32 qy = load<u32>(q + 4);
    w = (w & ~0x80000000u) | ((e & 1u) << 31);
    store<u32>(o + 48, 0);
    store<u32>(o + 36, qy);
    const u32 qz = load<u32>(q + 8);
    w = (w & ~0x40000000u) | ((g & 1u) << 30);
    store<u32>(o + 52, a);
    store<u32>(o + 40, qz);
    w = (w & ~0x20000000u) | ((h & 1u) << 29);
    store<u32>(o + 56, b);
    store<u32>(o + 60, 0);
    store<u32>(o + 64, c);
    store<u32>(o + 68, d);
    store<u32>(o + 72, w);
    ctx.fpr[1] = static_cast<f64>(std::bit_cast<f32>(qz));
    ret(ctx, 0x00B21328u);
}

// bool f(u32 key): false unless the global byte 0x010AEC60 is set; the
// key 0x00014065 / 0x0464F605 / 0x782CF148 picks slot 0 / 1 / 2 of the
// word table 0x00F65AB0 (other keys: false). Slots 0 / 1: 0x006D56E8(that
// word) != 0. Slot 2: true when 0x006D56E8 gives non-zero for the table's
// +8 or (then) +120 word.
DECOMP(0044D3AC, Key_Check_0044D3AC) {
    const u32 key = arg<u32>(ctx, 0);
    constexpr u32 tbl = 0x00F65AB0u;
    if (load<u8>(0x010AEC60u) == 0) {
        ret(ctx, 0u);
        return;
    }
    s32 i;
    if (key == 0x782CF148u) i = 2;
    else if (key == 0x0464F605u) i = 1;
    else if (key == 0x00014065u) i = 0;
    else i = -1;
    if (i == -1) {
        ret(ctx, 0u);
        return;
    }
    auto chk = [&](u32 a) {
        return static_cast<u32>(call(ctx, 0x006D56E8, {static_cast<u64>(static_cast<s64>(static_cast<s32>(load<u32>(a))))})) != 0;
    };
    if (i != 2) {
        ret(ctx, chk(tbl + 4 * static_cast<u32>(i)) ? 1u : 0u);
        return;
    }
    ret(ctx, (chk(tbl + 8) || chk(tbl + 120)) ? 1u : 0u);
}

// bool f(T* o): true when o is set, has bit 0x00040000 of +172, its s8
// slot (byte +172) is not -1, and in the world state g = *0x00F65CA0 the
// slot's record e = g + slot * 0x20C80 + 4208 belongs to a player g + 80
// + 4056 * u8 e[0] whose +292 is 0xCBEBC6C2, and e->+8->+216 (r) is set
// with bit 8 of r->+20 and r->+192 == 0x00014120. The profile word
// 0x00F65F74 is first set from Save_Profile0C9FC8(0xF214F06E) when 0.
DECOMP(004AEBE0, Obj_IsLocalPlayerSlot_004AEBE0) {
    const u32 o = arg<u32>(ctx, 0);
    if (o == 0 || (load<u32>(o + 172) & 0x00040000u) == 0 || static_cast<s8>(load<u8>(o + 172)) == -1) {
        ret(ctx, 0u);
        return;
    }
    u32 b = load<u8>(o + 172);
    if (load<u32>(0x00F65F74u) == 0) {
        store<u32>(0x00F65F74u, static_cast<u32>(call(ctx, 0x000C9FC8, {0xF214F06Eu})));
        b = load<u8>(o + 172);
    }
    const u32 g = load<u32>(0x00F65CA0u);
    const u32 e = g + static_cast<u32>(static_cast<s32>(static_cast<s8>(b)) * 0x20C80) + 4208;
    const u32 pl = g + load<u8>(e) * 4056u + 80;
    if (load<u32>(pl + 292) != 0xCBEBC6C2u) {
        ret(ctx, 0u);
        return;
    }
    const u32 r = load<u32>(load<u32>(e + 8) + 216);
    if (r == 0 || (load<u32>(r + 20) & 8u) == 0 || load<u32>(r + 192) != 0x00014120u) {
        ret(ctx, 0u);
        return;
    }
    ret(ctx, 1u);
}

// void f(T* o): the index of the name 0x00AEE310 (String_HashNoCase) in
// the five 20-byte records at 0x00F65F88 (-1 with a String_Format warning
// 0x00AEE4FC when missing) to u16 +0; the rest reset: +4..+16, +32..+48,
// +60 cleared, +24 = 0.25, +28 = 1.0, u16 +52/+56/+58/+64 = -1, u16 +54
// = 0, u8 +21/+66 = 0, byte +20: bits 0x80/0x40/0x20/0x04 cleared, 0x10
// set (in the original's store order). r3 = the index.
DECOMP(004B5D78, Rec_Reset_004B5D78) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 h = static_cast<u32>(call(ctx, 0x0056A710, {0x00AEE310u}));
    s32 idx = -1;
    for (u32 i = 0; i < 5; ++i) {
        if (load<u32>(0x00F65F88u + 20 * i) == h) {
            idx = static_cast<s32>(i);
            break;
        }
    }
    if (idx < 0) call(ctx, 0x005534D0, {0x00AEE4FCu, 0x00AEE310u});
    const u32 b = (load<u8>(o + 20) & ~0xE4u) | 0x10u;
    store<u32>(o + 4, 0);
    store<u32>(o + 8, 0);
    store<u32>(o + 12, 0);
    store<u32>(o + 16, 0);
    store<u32>(o + 24, 0x3E800000u);
    store<u32>(o + 28, 0x3F800000u);
    store<u16>(o + 54, 0);
    store<u32>(o + 44, 0);
    store<u32>(o + 48, 0);
    store<u32>(o + 36, 0);
    store<u32>(o + 40, 0);
    store<u32>(o + 32, 0);
    store<u8>(o + 21, 0);
    store<u16>(o + 52, 0xFFFF);
    store<u16>(o, static_cast<u16>(idx));
    store<u16>(o + 56, 0xFFFF);
    store<u16>(o + 58, 0xFFFF);
    store<u16>(o + 64, 0xFFFF);
    store<u8>(o + 20, static_cast<u8>(b));
    store<u8>(o + 66, 0);
    store<u32>(o + 60, 0);
    ret(ctx, idx);
}

// void f(T* o, X* x): when o->+16 names a component c of x
// (Effect_FindComponent) whose vt[32]() is 10: the node n = c->+40, or,
// when that is 0, the result of Node_FindByKey_004D54FC(key, previous)
// over c's keys (+20, u16 count +28), each stored to c->+40 (the last
// one wins); a node then gets Obj_Set4E3168(0x00AEEAC4, n) and
// Obj_Attach4E2DB0(o->+12, n, 1, 0). r3 as left.
DECOMP(004CC3F4, Effect_AttachNode_004CC3F4) {
    const u32 o = arg<u32>(ctx, 0), x = arg<u32>(ctx, 1);
    const u32 id = load<u32>(o + 16);
    ctx.gpr[3] = id;
    if (id == 0) return;
    const u32 c = static_cast<u32>(call(ctx, 0x004BB4B4, {x, id}));
    if (c == 0) return;
    if (static_cast<s32>(VCall(ctx, c, 32, {c})) != 10) return;
    u32 n = load<u32>(c + 40);
    if (n == 0) {
        for (u32 i = 0, off = 0; i < load<u16>(c + 28); ++i, off += 4) {
            n = static_cast<u32>(call(ctx, 0x004D54FC, {load<u32>(load<u32>(c + 20) + off), n}));
            store<u32>(c + 40, n);
        }
    }
    ctx.gpr[3] = n;
    if (n == 0) return;
    call(ctx, 0x004E3168, {0x00AEEAC4u, n});
    call(ctx, 0x004E2DB0, {load<u32>(o + 12), n, 1, 0});
}

// T* New(-, Xml* x): a 20-byte record (Mem_Alloc(20, 0, 16); 0 on
// failure): +12 cleared, u16 +4 = 1, u8 +16/+17 cleared, vtable
// 0x00B22240; the hash 0x8A93973F to +12 (default 0; when that read finds
// nothing, the s32 0x5C8998CD to +12, default 0), then the bools
// 0xCADB1721 / 0xF5897C08 to +16 / +17 (default 0). r3 = m.
DECOMP(004CC564, VtB22240_NewXml_004CC564) {
    const u32 x = arg<u32>(ctx, 1);
    const u32 m = static_cast<u32>(call(ctx, 0x0054ED38, {20, 0, 16}));
    if (m != 0) {
        store<u32>(m + 12, 0);
        store<u16>(m + 4, 1);
        store<u8>(m + 16, 0);
        store<u8>(m + 17, 0);
        store<u32>(m, 0x00B22240u);
        if (static_cast<u32>(call(ctx, 0x004FA14C, {x, 0x8A93973Fu, m + 12, 0})) != 0)
            call(ctx, 0x004F99BC, {x, 0x5C8998CDu, m + 12, 0});
        call(ctx, 0x004F986C, {x, 0xCADB1721u, m + 16, 0});
        call(ctx, 0x004F986C, {x, 0xF5897C08u, m + 17, 0});
    }
    ret(ctx, m);
}

// void ctor(T* o, a, b, const Vec3* p, const Vec2* q, -, u8 e; f1 = s):
// +4 = a, u8 +8 = 0, vtable 0x00B13FC8, +16..+72 = the 4x3 matrix at
// 0x01164BD0 (16-byte rows), +80..+88 and +96..+104 = *p, +112/+116 and
// +120/+124 = *q, +128/+132 = s, +136/+140/+144 = 0, bit 31 of +148 = bit
// 0 of e (in the original's order), then SetRef_001138D8(o, b) (r3 = its).
DECOMP(0074D09C, VtB13FC8_Ctor_0074D09C) {
    const u32 o = arg<u32>(ctx, 0), a = arg<u32>(ctx, 1), b = arg<u32>(ctx, 2), p = arg<u32>(ctx, 3),
              q = arg<u32>(ctx, 4), e = arg<u32>(ctx, 6);
    const f64 s = farg(ctx, 0);
    constexpr u32 g = 0x01164BD0u;
    store<u32>(o + 4, a);
    store<u8>(o + 8, 0);
    store<u32>(o + 16, load<u32>(g));
    store<u32>(o + 20, load<u32>(g + 4));
    store<u32>(o + 24, load<u32>(g + 8));
    store<u32>(o + 32, load<u32>(g + 16));
    store<u32>(o + 36, load<u32>(g + 20));
    store<u32>(o + 40, load<u32>(g + 24));
    const u32 g32 = load<u32>(g + 32);
    store<u32>(o, 0x00B13FC8u);
    store<u32>(o + 48, g32);
    store<u32>(o + 52, load<u32>(g + 36));
    store<u32>(o + 56, load<u32>(g + 40));
    store<u32>(o + 64, load<u32>(g + 48));
    store<u32>(o + 68, load<u32>(g + 52));
    store<u32>(o + 72, load<u32>(g + 56));
    for (u32 k = 0; k < 2; ++k) {
        store<u32>(o + 80 + 16 * k, load<u32>(p));
        store<u32>(o + 84 + 16 * k, load<u32>(p + 4));
        store<u32>(o + 88 + 16 * k, load<u32>(p + 8));
    }
    store<u32>(o + 112, load<u32>(q));
    const u32 q4 = load<u32>(q + 4);
    const u32 w = (load<u32>(o + 148) & 0x7FFFFFFFu) | ((e & 1u) << 31);
    store<u32>(o + 116, q4);
    const u32 q0 = load<u32>(q);
    store<u32>(o + 136, 0);
    store<u32>(o + 120, q0);
    const u32 q4b = load<u32>(q + 4);
    Stfs32(o + 128, s);
    store<u32>(o + 124, q4b);
    Stfs32(o + 132, s);
    store<u32>(o + 140, 0);
    store<u32>(o + 144, 0);
    store<u32>(o + 148, w);
    call(ctx, 0x001138D8, {o, b});
}

// void ctor(T* o, r4..r8, u8 c, u8 d, [stack] k, const Vec3* p, const
// Vec3* q, h, i, u8 x, u8 y, u8 z): SpawnQuery_Init(o, r4..r8, c, d,
// stack k / 0; f1 = the float at 0x00761588), vtable 0x00B19178, +240..
// +248 = *p, +256..+264 = *q, +272 / +276 = h / i, u8 +280/+281/+282 =
// x / y / z, bit 0x1000 of +4 set (in the original's order). r3 = that
// word; f1 = q's z.
DECOMP(00761590, VtB19178_Ctor_00761590) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 k = StkArg34(ctx, 116), p = StkArg34(ctx, 124), q = StkArg34(ctx, 132), h = StkArg34(ctx, 140),
              i = StkArg34(ctx, 148), x = StkArg34(ctx, 156), y = StkArg34(ctx, 164), z = StkArg34(ctx, 172);
    // stack argument 10 is not written by the original (left as is in its frame)
    callf(ctx, 0x000CC4D4,
          {o, arg<u32>(ctx, 1), arg<u32>(ctx, 2), arg<u32>(ctx, 3), arg<u32>(ctx, 4), arg<u32>(ctx, 5),
           arg<u32>(ctx, 6) & 0xFFu, arg<u32>(ctx, 7) & 0xFFu, k, 0, 0},
          {Lfs32(0x00761588u)});
    store<u32>(o + 240, load<u32>(p));
    store<u32>(o + 244, load<u32>(p + 4));
    const u32 p8 = load<u32>(p + 8);
    store<u32>(o, 0x00B19178u);
    store<u32>(o + 248, p8);
    store<u32>(o + 256, load<u32>(q));
    const u32 q4 = load<u32>(q + 4);
    const u32 w = load<u32>(o + 4) | 0x1000u;
    store<u32>(o + 260, q4);
    const u32 q8 = load<u32>(q + 8);
    store<u32>(o + 272, h);
    store<u32>(o + 264, q8);
    store<u32>(o + 276, i);
    store<u8>(o + 280, static_cast<u8>(x));
    store<u8>(o + 281, static_cast<u8>(y));
    store<u8>(o + 282, static_cast<u8>(z));
    store<u32>(o + 4, w);
    ctx.fpr[1] = static_cast<f64>(std::bit_cast<f32>(q8));
    ret(ctx, w);
}

// void Delete(T* o): nothing for o = 0; else vtable 0x00B1D424, for each
// 8-byte slot of the array +16 (count +36): a set object p gets
// p->vt[312](p, o->+4->+8, o->+8) (entry / OPD left in r6 / r7), then the
// slot = {0, FLT_MAX}; +36 = 0, the vector +16 cleared (PtrVector_Clear,
// Nop) and Mem_FreeFrom(o, 0). r3 as left.
DECOMP(00766000, VtB1D424_Delete_00766000) {
    const u32 o = arg<u32>(ctx, 0);
    if (o == 0) return;
    store<u32>(o, 0x00B1D424u);
    if (load<u32>(o + 36) != 0) {
        u32 i = 0;
        do {
            const u32 e = load<u32>(o + 16) + 8 * i;
            const u32 p = load<u32>(e);
            if (p != 0) VCallEO36(ctx, load<u32>(p), 312, {p, load<u32>(load<u32>(o + 4) + 8), load<u32>(o + 8)});
            store<u32>(e, 0);
            ++i;
            const u32 n = load<u32>(o + 36);
            store<u32>(e + 4, 0x7F7FFFFFu);
            if (!(i < n)) break;
        } while (true);
    }
    store<u32>(o + 36, 0);
    call(ctx, 0x0054EC1C, {o + 16});
    call(ctx, 0x0054EC18, {o + 16});
    call(ctx, 0x0054EE6C, {o, 0});
}

// ---------------------------------------------------------------- sweep: 72 instructions
// bool f(T* o, -, X* x, u8 out[4]): out = o's colour +20; with x set,
// o->+24 >= K (the constant at 0x00077D70) and x->+32 < o->+24, the colour
// is faded towards the base colour +16 by a = min(255, (u32)(x->+32 /
// o->+24 * the constant at 0x00077D74)) (fctidz): out[k] = base[k] +
// ((colour[k] - base[k]) * a >> 8), and the result is 0; otherwise 1.
// f1 as the original leaves it (the fctidz bits on the fade path).
DECOMP(00077D90, Colour_Fade_00077D90) {
    const u32 o = arg<u32>(ctx, 0), x = arg<u32>(ctx, 2), out = arg<u32>(ctx, 3);
    const u32 src = o + 20, base = o + 16;
    for (u32 k = 0; k < 4; ++k) store<u8>(out + k, load<u8>(src + k));
    if (x == 0) {
        ret(ctx, 1u);
        return;
    }
    const f64 K = Lfs32(0x00077D70u);
    const f64 d = Lfs32(o + 24);
    ctx.fpr[1] = K;
    if (!(d >= K)) {
        ret(ctx, 1u);
        return;
    }
    const f64 v = Lfs32(x + 32);
    ctx.fpr[1] = v;
    if (!(v < d)) {
        ret(ctx, 1u);
        return;
    }
    const f64 t = static_cast<f32>(static_cast<f32>(v / d) * Lfs32(0x00077D74u));
    const u64 iv = rt::fctid(t, true);
    ctx.fpr[1] = std::bit_cast<f64>(iv);
    const u32 lo = static_cast<u32>(iv);
    const s32 a = lo < 0xFFu ? static_cast<s32>(lo) : 0xFF;
    for (u32 k = 0; k < 4; ++k) {
        const s32 b = load<u8>(base + k), c = load<u8>(src + k);
        store<u8>(out + k, static_cast<u8>(b + (((c - b) * a) >> 8)));
    }
    ret(ctx, 0u);
}

// T* New(-, Xml* x): NewXml34 (28 bytes): +12 / +20 cleared, +16 = 90,
// vtable 0x00B10468, u8 +24 cleared; the hash 0x8A93973F to +12 (default
// 0), the bool 0x7DED99DC to a stack byte (default 0), the hashes
// 0x001EDFA1 / 0x00255BF3 to +16 / +20 (current defaults), then u8 +24 =
// that byte.
DECOMP(0008A140, VtB10468_NewXml_0008A140) {
    NewXml34(ctx, 28, [&](u32 m, u32 x) {
        Stack fr(ctx, 16);
        const u32 t = fr.addr();
        store<u32>(m + 12, 0);
        store<u32>(m + 20, 0);
        store<u32>(m + 16, 90);
        store<u32>(m, 0x00B10468u);
        store<u8>(m + 24, 0);
        store<u8>(t, 0);
        RdHash34(ctx, x, 0x8A93973Fu, m + 12, 0);
        RdBoolCur34(ctx, x, 0x7DED99DCu, t);
        RdHashCur34(ctx, x, 0x001EDFA1u, m + 16);
        RdHashCur34(ctx, x, 0x00255BF3u, m + 20);
        store<u8>(m + 24, load<u8>(t));
    });
}

// T* New(-, Xml* x): NewXml34 (28 bytes): u8 +12 / +16 / +20 cleared,
// vtable 0x00B105F8, +24 cleared; the bool 0xCADB1721 to +12 (default 0),
// the hash 0xB2E59A9F to +16 and the f32s 0x8FA16092 / 0xB7EA3D94 to +20
// / +24 (current defaults).
DECOMP(0008B9C0, VtB105F8_NewXml_0008B9C0) {
    NewXml34(ctx, 28, [&](u32 m, u32 x) {
        store<u8>(m + 12, 0);
        store<u32>(m + 16, 0);
        store<u32>(m + 20, 0);
        store<u32>(m, 0x00B105F8u);
        store<u32>(m + 24, 0);
        call(ctx, 0x004F986C, {x, 0xCADB1721u, m + 12, 0});
        RdHashCur34(ctx, x, 0xB2E59A9Fu, m + 16);
        RdF32Cur34(ctx, x, 0x8FA16092u, m + 20);
        RdF32Cur34(ctx, x, 0xB7EA3D94u, m + 24);
    });
}

// T* New(-, Xml* x): NewXml34 (24 bytes): +12 cleared, vtable 0x00B10760,
// the colour at 0x01164CA0 copied (bytewise) to +16, +20 = 7.25; the hash
// 0x8A93973F to +12, the colour 0x03D53743 to +16 (XmlCursor_ReadColor;
// default +16 itself) and the f32 0x6A0A0754 to +20 (current defaults).
DECOMP(0008CF50, VtB10760_NewXml_0008CF50) {
    NewXml34(ctx, 24, [&](u32 m, u32 x) {
        store<u32>(m + 12, 0);
        store<u32>(m, 0x00B10760u);
        for (u32 k = 0; k < 4; ++k) store<u8>(m + 16 + k, load<u8>(0x01164CA0u + k));
        store<u32>(m + 20, 0x40E80000u);
        RdHashCur34(ctx, x, 0x8A93973Fu, m + 12);
        call(ctx, 0x004F9F9C, {x, 0x03D53743u, m + 16, m + 16});
        RdF32Cur34(ctx, x, 0x6A0A0754u, m + 20);
    });
}

// void f(T* o): adds o to the registry vector 0x01013B48 (data +0, u16
// capacity +4 doubled by PtrVector_Reserve when full, u16 count +8, u16
// high-water +10; r3 = the old high-water). When o is already in it,
// String_Format(0x00AC6A50, o->vt[76]() (o's name), or 0x00AC6AC8 for a
// null o) instead (r3 = its).
DECOMP(000D64F4, Registry_Add_000D64F4) {
    const u32 o = arg<u32>(ctx, 0);
    constexpr u32 V = 0x01013B48u;
    const u32 n = load<u16>(V + 8), cap = load<u16>(V + 4);
    const u32 d = load<u32>(V);
    for (u32 i = 0; i < n; ++i) {
        if (load<u32>(d + 4 * i) == o) {
            const u32 name = o == 0 ? 0x00AC6AC8u : static_cast<u32>(VCall(ctx, o, 76, {o}));
            call(ctx, 0x005534D0, {0x00AC6A50u, name});
            return;
        }
    }
    u32 c = n;
    u32 data = d;
    if (n == cap) {
        call(ctx, 0x00551718, {V, (cap << 1) & 0x1FFFEu});
        c = load<u16>(V + 8);
        data = load<u32>(V);
    }
    const u32 c2 = load<u16>(V + 8);
    store<u32>(data + ((c << 2) & 0x3FFFCu), o);
    const u32 nc = (c2 + 1u) & 0xFFFFu;
    const u32 hw = load<u16>(V + 10);
    store<u16>(V + 8, static_cast<u16>(nc));
    store<u16>(V + 10, static_cast<u16>(hw > nc ? hw : nc));
    ret(ctx, hw);
}

// void f(T* o, u8 deep): when o's world object w = o->+160 has kind 8
// (bits 10..15 of w->+4): w->vt[40](w, 1, 0), 0x004A6DF0(w, 1),
// w->vt[40](w, 0, 0). With deep, the same for the +192 object of each
// 240-byte child record of o's list +340 (data +0, u16 count +8),
// recursively. r3: 0 (not deep) or the list word.
DECOMP(000D8FC4, Obj_RefreshKind8_000D8FC4) {
    const u32 o = arg<u32>(ctx, 0), deep = arg<u32>(ctx, 1);
    const u32 w = load<u32>(o + 160);
    if (w != 0 && ((load<u32>(w + 4) >> 10) & 0x3Fu) == 8) {
        VCall(ctx, w, 40, {w, 1, 0});
        call(ctx, 0x004A6DF0, {w, 1});
        VCall(ctx, w, 40, {w, 0, 0});
    }
    if ((deep & 0xFFu) == 0) {
        ret(ctx, 0u);
        return;
    }
    for (u32 i = 0, off = 0;; ++i, off += 240) {
        const u32 l = load<u32>(o + 340);
        const u32 n = l != 0 ? load<u16>(l + 8) : 0u;
        if (!(i < n)) {
            ret(ctx, l);
            return;
        }
        call(ctx, 0x000D8FC4, {load<u32>(load<u32>(l) + off + 192), 1});
    }
}

// void f(-, Msg* m): OwnerMsg36 with 0x000E8E48 / Obj_AddSample_001A97D8
// / Obj_ProjectPart_001BD370 and the message 0x2FBD3D56.
DECOMP(0012CEA8, OwnerMsg_0012CEA8) {
    OwnerMsg36(ctx, 0x000E8E48u, 0x001A97D8u, 0x001BD370u, 0x2FBD3D56u);
}

// void f(-, Msg* m): OwnerMsg36 with Nop_000E969C / 0x001A98B4 /
// Obj_SetFlag16_001BD440 and the message 0x340866A4.
DECOMP(0012CFCC, OwnerMsg_0012CFCC) {
    OwnerMsg36(ctx, 0x000E969Cu, 0x001A98B4u, 0x001BD440u, 0x340866A4u);
}

// void f(const T* o, T* d): Thunk_000E44C0(o, d); then o's floats +240..
// +248 / +256..+264 / +272..+284 to d +208.. / +224.. / +240.. and the 17
// words o +288..+352 to d +256..+320 (copied raw, in pairs as the
// original); bits 31..30 of d->+324 from o->+356. r3 = that word; f1 =
// o's float +284.
DECOMP(001D80C4, Obj_CopyState_001D80C4) {
    const u32 o = arg<u32>(ctx, 0), d = arg<u32>(ctx, 1);
    call(ctx, 0x000E44C0, {o, d});
    static constexpr u32 kSrc[] = {240, 244, 248, 256, 260, 264, 272, 276, 280, 284};
    static constexpr u32 kDst[] = {208, 212, 216, 224, 228, 232, 240, 244, 248, 252};
    for (u32 k = 0; k < 10; ++k) store<u32>(d + kDst[k], load<u32>(o + kSrc[k]));
    const u32 s = o + 288, t = d + 256;
    for (u32 k = 0; k < 64; k += 8) {
        const u32 a = load<u32>(s + k), b = load<u32>(s + k + 4);
        store<u32>(t + k, a);
        store<u32>(t + k + 4, b);
    }
    store<u32>(t + 64, load<u32>(s + 64));
    const u32 w = (load<u32>(d + 324) & 0x3FFFFFFFu) | (load<u32>(o + 356) & 0xC0000000u);
    store<u32>(d + 324, w);
    ctx.fpr[1] = Lfs32(o + 284);
    ret(ctx, w);
}

// void f(T* o, u8 on): the three parts o->+716 / +720 / +724. Off:
// Obj_Dispatch362(part, 1, 1, 0) for each. On: Obj_SetAnim(part, anim, 1,
// -, 0, 0, 0x01164CA4; f1) with anim 0 (r6 = the part, later as left; f1 = the float at
// 0x001DED10), 0x01052F0C and 0x01052FD4 (f1 = the float at 0x01052EC0).
// r3 as left.
DECOMP(001DED18, Obj_ShowParts_001DED18) {
    const u32 o = arg<u32>(ctx, 0), on = arg<u32>(ctx, 1) & 0xFFu;
    if (on == 0) {
        for (u32 k = 0; k < 3; ++k) call(ctx, 0x0036325C, {load<u32>(o + 716 + 4 * k), 1, 1, 0});
        return;
    }
    constexpr u32 g = 0x01164CA4u;
    const u32 p = load<u32>(o + 716);
    callf(ctx, 0x00363A3C, {p, 0, 1, p, 0, 0, g}, {Lfs32(0x001DED10u)});
    // r6 is not set for the next two calls (the callee does not read it): passed as left
    callf(ctx, 0x00363A3C, {load<u32>(o + 720), 0x01052F0Cu, 1, ctx.gpr[6], 0, 0, g}, {Lfs32(0x01052EC0u)});
    callf(ctx, 0x00363A3C, {load<u32>(o + 724), 0x01052FD4u, 1, ctx.gpr[6], 0, 0, g}, {Lfs32(0x01052EC0u)});
}

// T* f(a, b, c): a 256-byte object m (SmallAlloc): 0x00746148(m, c, 0, 0,
// 1, 0, 1, 0, 0, 0), +188 = 0, vtable 0x00B19E58, u16 +192 = 0xFFFF, u16
// +194 = 0, +196 = 0x922D3BD1, +200 / +204 = -1.0, +208..+255 cleared
// (memset), bit 0x00800000 of +4 and 0x10 of u8 +184 set; then (also on
// failure, with m = 0) QueryDef_Read215518(a, b, m). r3 = m.
DECOMP(002159AC, QueryDef_New_002159AC) {
    const u32 a = arg<u32>(ctx, 0), b = arg<u32>(ctx, 1), c = arg<u32>(ctx, 2);
    const u32 m = static_cast<u32>(call(ctx, 0x0055238C, {256}));
    if (m != 0) {
        call(ctx, 0x00746148, {m, c, 0, 0, 1, 0, 1, 0, 0, 0});
        store<u32>(m + 188, 0);
        store<u32>(m, 0x00B19E58u);
        store<u16>(m + 192, 0xFFFF);
        store<u16>(m + 194, 0);
        store<u32>(m + 196, 0x922D3BD1u);
        store<u32>(m + 200, 0xBF800000u);
        store<u32>(m + 204, 0xBF800000u);
        call(ctx, 0x00726E54, {m + 208, 0, 48});
        const u32 w = load<u32>(m + 4) | 0x00800000u;
        const u32 f = load<u8>(m + 184) | 0x10u;
        store<u32>(m + 4, w);
        store<u8>(m + 184, static_cast<u8>(f));
    }
    call(ctx, 0x00215518, {a, b, m});
    ret(ctx, m);
}

// void f(T* o, const Ev* e): nothing for the id u16 e->+0 = 0xFFFF (r3 =
// it). Else o->+628 is decremented and r = e->+20->vt[48](e->+20, id);
// with r, its vector +16 (negated unless e->+8 equals o->+612->+24) is
// subtracted from o's +640..+648, and when the count is then 0 that
// vector is reset to the one at 0x01164B40 (r3 = 0x01160000). r3 = the
// count; f1 as left.
DECOMP(0023D690, Obj_DropContact_0023D690) {
    const u32 o = arg<u32>(ctx, 0), e = arg<u32>(ctx, 1);
    const u32 id = load<u16>(e);
    if (id == 0xFFFFu) {
        ret(ctx, id);
        return;
    }
    const u32 n = load<u32>(o + 628) - 1u;
    const u32 c = load<u32>(e + 20);
    store<u32>(o + 628, n);
    const u32 r = static_cast<u32>(VCall(ctx, c, 48, {c, id}));
    const u32 cnt = load<u32>(o + 628);
    ret(ctx, cnt);
    if (r == 0) return;
    f64 x = Lfs32(r + 16), y = Lfs32(r + 20), z = Lfs32(r + 24);
    if (load<u32>(e + 8) != load<u32>(load<u32>(o + 612) + 24)) {
        x = -x;
        y = -y;
        z = -z;
    }
    const f64 nx = static_cast<f32>(Lfs32(o + 640) - x), ny = static_cast<f32>(Lfs32(o + 644) - y),
              nz = static_cast<f32>(Lfs32(o + 648) - z);
    Stfs32(o + 640, nx);
    Stfs32(o + 644, ny);
    Stfs32(o + 648, nz);
    ctx.fpr[1] = nx;
    if (cnt != 0) return;
    constexpr u32 g = 0x01164B40u;
    store<u32>(o + 640, load<u32>(g));
    store<u32>(o + 644, load<u32>(g + 4));
    const u32 gz = load<u32>(g + 8);
    store<u32>(o + 648, gz);
    ctx.fpr[1] = static_cast<f64>(std::bit_cast<f32>(gz));
    ret(ctx, 0x01160000u);  // r3 = the high half of the address, as the original leaves it
}

// void f(T* o, Spline* sp, Vec3* out): without sp, out = the vector at
// 0x01164B40 (r3 = 0x01160000). Else the parameter t of o->+8's position
// +144 on sp (SplineInstance_Nearest(sp, &pos, 0, 0), a stack copy),
// advanced by the float at 0x01057AE4 / sp->+208->+120 and clamped to
// [the float at 0x0029C0E0 (0), the one at 0x0029C0DC (1)] (the
// original's compare order), then WorldObj_SplinePointAt(sp, 0x01057A9C
// (left in r4), out, 0; f1 = t). r3 / f1 as left.
DECOMP(0029C0E8, Spline_AdvancePoint_0029C0E8) {
    const u32 o = arg<u32>(ctx, 0), sp = arg<u32>(ctx, 1), out = arg<u32>(ctx, 2);
    if (sp == 0) {
        constexpr u32 g = 0x01164B40u;
        store<u32>(out, load<u32>(g));
        store<u32>(out + 4, load<u32>(g + 4));
        const u32 z = load<u32>(g + 8);
        store<u32>(out + 8, z);
        ctx.fpr[1] = static_cast<f64>(std::bit_cast<f32>(z));
        ret(ctx, 0x01160000u);
        return;
    }
    Stack fr(ctx, 16);
    const u32 tmp = fr.addr();
    const u32 v = load<u32>(o + 8) + 144;
    store<u32>(tmp, load<u32>(v));
    store<u32>(tmp + 4, load<u32>(v + 4));
    const u32 vz = load<u32>(v + 8);
    store<u32>(tmp + 8, vz);
    const f64 t0 = callf(ctx, 0x005927FC, {sp, tmp, 0, 0}, {static_cast<f64>(std::bit_cast<f32>(vz))});
    const f64 step = static_cast<f32>(Lfs32(0x01057AE4u) / Lfs32(load<u32>(sp + 208) + 120));
    const f64 A = Lfs32(0x0029C0DCu), B = Lfs32(0x0029C0E0u);
    const f64 t = static_cast<f32>(t0 + step);
    const f64 m = t < A ? t : A;
    const f64 p = m > B ? m : B;
    callf(ctx, 0x00592748, {sp, 0x01057A9Cu, out, 0}, {p});
}

// bool f(-, const Vec2* a, const Vec2* b, const Vec2* c, const Vec2* d,
// Vec2* out): intersection of the segments a-b and c-d: false when the
// determinant is below 1e-5 (the constant at 0x002CCE80) in magnitude or
// either parameter is outside [0, 1] (0x002CCE88 / 0x002CCE84); else
// true, with out (when set) = a + t (b - a). f1 as the original leaves it.
DECOMP(002CCE90, Seg2_Intersect_002CCE90) {
    const u32 a = arg<u32>(ctx, 1), b = arg<u32>(ctx, 2), c = arg<u32>(ctx, 3), d = arg<u32>(ctx, 4),
              out = arg<u32>(ctx, 5);
    auto f = [](f64 x) { return static_cast<f64>(static_cast<f32>(x)); };
    const f64 ax = Lfs32(a), ay = Lfs32(a + 4), bx = Lfs32(b), by = Lfs32(b + 4);
    const f64 cx = Lfs32(c), cy = Lfs32(c + 4), dx = Lfs32(d), dy = Lfs32(d + 4);
    const f64 dxc = f(dx - cx), bya = f(by - ay), axc = f(ax - cx), dyc = f(dy - cy), ayc = f(ay - cy),
              bxa = f(bx - ax);
    const f64 p7 = f(dxc * bya), p8 = f(dyc * axc), p5 = f(bya * axc);
    const f64 den = f(std::fma(dyc, bxa, -p7));
    const f64 n1 = f(std::fma(dxc, ayc, -p8));
    const f64 n2 = f(std::fma(bxa, ayc, -p5));
    ret(ctx, 0u);
    ctx.fpr[1] = n2;
    if (std::fabs(den) < Lfs32(0x002CCE80u)) return;
    const f64 t = f(n1 / den), u = f(n2 / den);
    const f64 hi = Lfs32(0x002CCE84u), lo = Lfs32(0x002CCE88u);
    ctx.fpr[1] = hi;
    if (t > hi || t < lo || u > hi || u < lo) return;
    if (out != 0) {
        const f64 x0 = Lfs32(a);
        Stfs32(out, f(std::fma(t, f(Lfs32(b) - x0), x0)));
        const f64 y0 = Lfs32(a + 4);
        const f64 y = f(std::fma(t, f(Lfs32(b + 4) - y0), y0));
        Stfs32(out + 4, y);
        ctx.fpr[1] = y;
    }
    ret(ctx, 1u);
}

// u32 f(T* o): over o's o->+24 860-byte records (+20): for each with u8
// +828 set, its index k among the game list's records (*0x00F64C20: +20,
// count +24) with the same +724 key (-1 if none), then the number of the
// entries of the table 0x0106819C (u16 count +2, 8-byte entries +8) whose
// +4 object's first byte equals k; r3 = the sum.
DECOMP(002E57A8, Rec_CountRefs_002E57A8) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 n = load<u32>(o + 24);
    u32 sum = 0;
    if (n != 0) {
        constexpr u32 V = 0x0106819Cu;
        const u32 g = load<u32>(0x00F64C20u);
        const u32 cnt2 = load<u16>(V + 2), ents = load<u32>(V + 8);
        u32 r = load<u32>(o + 20);
        for (u32 i = 0; i < n; ++i, r += 860) {
            if (load<u8>(r + 828) == 0) continue;
            const u32 key = load<u32>(r + 724);
            const u32 m = load<u32>(g + 24);
            s32 k = -1;
            for (u32 j = 0; j < m; ++j) {
                if (load<u32>(load<u32>(g + 20) + 860 * j + 724) == key) {
                    k = static_cast<s32>(j);
                    break;
                }
            }
            u32 c = 0;
            for (s32 j = 0; j < static_cast<s32>(cnt2); ++j)
                if (static_cast<s32>(load<u8>(load<u32>(ents + 8 * static_cast<u32>(j) + 4))) == k) ++c;
            sum += c;
        }
    }
    ret(ctx, sum);
}

// bool f(T* o, Counts* out): with o->+2212 set: out = {it, o->+2216,
// o->+2216}, and for each part c of the chain o->+2576 (next +2564) with
// bit 0x100 of +20: out[2]++ and, when c->vt[56]() is set, out[1]++;
// true. Without: true when some part's vt[52](c, out) is set, else false.
DECOMP(00327928, Obj_CountParts_00327928) {
    const u32 o = arg<u32>(ctx, 0), out = arg<u32>(ctx, 1);
    const u32 w = load<u32>(o + 2212);
    u32 c = load<u32>(o + 2576);
    if (w != 0) {
        store<u32>(out, w);
        const u32 x = load<u32>(o + 2216);
        store<u32>(out + 4, x);
        store<u32>(out + 8, x);
        for (; c != 0; c = load<u32>(c + 2564)) {
            if ((load<u32>(c + 20) & 0x100u) == 0) continue;
            const u32 r = static_cast<u32>(VCall(ctx, c, 56, {c}));
            const u32 y = load<u32>(out + 8);
            if (r != 0) store<u32>(out + 4, load<u32>(out + 4) + 1u);
            store<u32>(out + 8, y + 1u);
        }
        ret(ctx, 1u);
        return;
    }
    for (; c != 0; c = load<u32>(c + 2564)) {
        if (static_cast<u32>(VCall(ctx, c, 52, {c, out})) != 0) {
            ret(ctx, 1u);
            return;
        }
    }
    ret(ctx, 0u);
}

// void f(Section* o, s32 mode, u8 b): changed = b or u8 o->+18 != mode.
// Changed into a mode other than 4: o->vt[28](o, 0) first; then
// Section_SetModeBase_003260F0(o, mode, b); changed into mode 4:
// o->vt[28](o, 0) and o->vt[36](o, 0) after it. r3 as left.
DECOMP(0032E918, Section_SetMode_0032E918) {
    const u32 o = arg<u32>(ctx, 0), b = arg<u32>(ctx, 2) & 0xFFu;
    const s32 mode = arg<s32>(ctx, 1);
    const bool ch = b != 0 || mode != static_cast<s32>(load<u8>(o + 18));
    const bool late = ch && mode == 4;
    if (ch && mode != 4) VCall(ctx, o, 28, {o, 0});
    call(ctx, 0x003260F0, {o, static_cast<u32>(mode), b});
    if (late) {
        VCall(ctx, o, 28, {o, 0});
        VCall(ctx, o, 36, {o, 0});
    }
}

// void f() (module init): under the lock 0x0106BFC8 (0x00ABF0E4 /
// 0x00ABF104): the heap size word 0x0106BD68 = 0x01600000, its block
// *0x00F64CA0 = Mem_Alloc(size, 0, 16), and a 24-byte pool (cleared;
// Pool_Create(pool, block, size, 0); 0 when the allocation fails) to
// 0x0106BFE0; the byte 0x0106BFE4 and the counters 0x0106BFE8..0x0106BFF4
// cleared. r3 = the unlock's.
DECOMP(00354F60, Heap_Init_00354F60) {
    constexpr u32 lk = 0x0106BFC8u;
    call(ctx, 0x00ABF0E4, {lk, 100000});
    store<u32>(0x0106BD68u, 0x01600000u);
    store<u32>(0x00F64CA0u, static_cast<u32>(call(ctx, 0x0054ED38, {0x01600000u, 0, 16})));
    const u32 m = static_cast<u32>(call(ctx, 0x0054ED38, {24, 0, 16}));
    if (m != 0) {
        const u32 blk = load<u32>(0x00F64CA0u), sz = load<u32>(0x0106BD68u);
        for (u32 k = 0; k < 24; k += 4) store<u32>(m + k, 0);
        call(ctx, 0x00565AA8, {m, blk, sz, 0});
    }
    store<u32>(0x0106BFE0u, m);
    store<u8>(0x0106BFE4u, 0);
    store<u32>(0x0106BFF4u, 0);
    store<u32>(0x0106BFE8u, 0);
    store<u32>(0x0106BFECu, 0);
    store<u32>(0x0106BFF0u, 0);
    call(ctx, 0x00ABF104, {lk});
}

// void f(T* o) (destructor body): DtorB1F678_36.
DECOMP(0036A098, VtB1F678_Dtor_0036A098) {
    DtorB1F678_36(ctx);
}

// void f(T* o) (destructor body, a second copy): DtorB1F678_36.
DECOMP(0036D118, VtB1F678_Dtor_0036D118) {
    DtorB1F678_36(ctx);
}

// void f(T* o): SayStep36 with the flags 0x00800000 / 0x00400000 of
// +532, the floats +196 / +192 -> +200 (K at 0x003841A4), the line +176
// and the hash *0x00F64FBC.
DECOMP(003841B0, Obj_SayStep_003841B0) {
    SayStep36(ctx, 532, 0x00800000u, 0x00400000u, 196, 192, 200, 0x003841A4u, 176, 0x00F64FBCu);
}

// void f(T* o): SayStep36 with the flags 0x01000000 / 0x00800000 of
// +960, the floats +244 / +240 -> +248 (K at 0x0038773C), the line +212
// and the hash *0x00F65024.
DECOMP(00387748, Obj_SayStep_00387748) {
    SayStep36(ctx, 960, 0x01000000u, 0x00800000u, 244, 240, 248, 0x0038773Cu, 212, 0x00F65024u);
}

// bool f(T* o): for the target t = o->+12 with bit 8 of +192 and a set
// t->+380 (c): false when (c + 80)->vt[32](c + 80, 0) is set; else, when
// t->vt[376]() is set, t->vt[376]()->vt[44](it, 1), and t->+380 = t->+384.
// True otherwise.
DECOMP(0042FCA4, Obj_ReleaseHold_0042FCA4) {
    ReleaseHold36(ctx);
}

// bool f(T* o): ReleaseHold36 (a second copy of 0x0042FCA4).
DECOMP(00430940, Obj_ReleaseHold_00430940) {
    ReleaseHold36(ctx);
}

// void f(Scope* o, u32 key, Res* out, a, b): without a map o->+8, bit
// 0x80 of u8 out->+4 set (r3 = that byte). Else the entry e of key
// (SortedMap_Find75E570(map, &key), a stack copy): none: the parent scope
// o->+0 is searched the same way (recursively, b = -1), or without one
// the bit is set; found: the handler of e's type (the OPD at +16 of the
// 20-byte record (s16) e->+0 at 0x00F65F88) is called (e, o->+36, out, a,
// b; the OPD and its entry in r8 / r9) and out->+8 = e. r3 as left.
DECOMP(004B55E0, Scope_Resolve_004B55E0) {
    const u32 o = arg<u32>(ctx, 0), key = arg<u32>(ctx, 1), out = arg<u32>(ctx, 2), a = arg<u32>(ctx, 3),
              b = arg<u32>(ctx, 4);
    const u32 map = load<u32>(o + 8);
    auto miss = [&]() {
        const u32 v = load<u8>(out + 4) | 0x80u;
        store<u8>(out + 4, static_cast<u8>(v));
        ret(ctx, v);
    };
    if (map == 0) {
        miss();
        return;
    }
    Stack fr(ctx, 16);
    const u32 k = fr.addr();
    store<u32>(k, key);
    const u32 e = load<u32>(static_cast<u32>(call(ctx, 0x0075E570, {map, k})));
    if (e == 0) {
        const u32 par = load<u32>(o);
        if (par == 0) {
            miss();
            return;
        }
        call(ctx, 0x004B55E0, {par, load<u32>(k), out, a, 0xFFFFFFFFu});
        return;
    }
    const s32 idx = static_cast<s16>(load<u16>(e));
    const u32 fn = load<u32>(0x00F65F88u + static_cast<u32>(idx * 20) + 16);
    call_ptr(ctx, fn, {e, load<u32>(o + 36), out, a, b, fn, load<u32>(fn)});
    store<u32>(out + 8, e);
}

// void f(Mat43* out, T* o): out = the bone target of o:
// BoneCtl_Target(o, &m, o->+1264, 0) on a stack block whose matrix m
// starts as the one at 0x01164BD0 (16-byte rows; the w column not set)
// and is followed by {a 64-byte stack buffer, 0, u8 0} (the buffer also
// left in r7), then the matrix copied out (raw). r3 as left; f1 = the
// last float.
DECOMP(007558E0, Bone_GetTarget_007558E0) {
    const u32 out = arg<u32>(ctx, 0), o = arg<u32>(ctx, 1);
    Stack fr(ctx, 160);
    const u32 buf = fr.addr(), m = buf + 64;
    constexpr u32 g = 0x01164BD0u;
    static constexpr u32 kOff[] = {0, 4, 8, 16, 20, 24, 32, 36, 40, 48, 52, 56};
    for (u32 k : kOff) store<u32>(m + k, load<u32>(g + k));
    store<u32>(m + 68, 0);
    store<u8>(m + 72, 0);
    store<u32>(m + 64, buf);
    call(ctx, 0x00250F40, {o, m, load<u32>(o + 1264), 0, buf});
    for (u32 k : kOff) store<u32>(out + k, load<u32>(m + k));
    ctx.fpr[1] = Lfs32(m + 56);
}
