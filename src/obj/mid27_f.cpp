// Game code, medium sweep part 27: resets and handlers of assorted game
// objects (animated prop reset, grab begin, effect binding lookup, the
// 0x00B1D070 object's reset, the world's map registration, attachment
// release).
#include "mid27.h"

using namespace sly4;
using namespace sly4::mid27;

namespace {
using namespace m27fn;
constexpr u32 kSetStateFx = 0x00266E6Cu;    // Obj_SetStateFx266E6C(o, state)
constexpr u32 kSetHash = 0x004CF988u;       // Effect_SetHash4CF988(e, hash)
constexpr u32 kFxBindGlobal = 0x011DE2A8u;  // the bound node's value (+8) of the last lookup
constexpr u32 kFxOff = 0x2C9E20u;           // f32 passed to f_002C96A8 by the reset

// The binding lookup of a {.., +12 tree root*, +16 path[], u16 depth +96,
// +100 node} record e: walks the tree (key +4, value +8, children +12 /
// +16 by unsigned key) from *(e->+12) for k, pushing the nodes passed into
// the path; e->+100 = the node found (and *0x011DE2A8 = its value) or the
// last node visited; an empty tree clears both. Returns true when found.
bool BindLookup(u32 e, u32 k) {
    u32 cur = load<u32>(load<u32>(e + 12));
    if (cur == 0) {
        store<u32>(e + 100, 0);
        store<u32>(kFxBindGlobal, 0);
        return false;
    }
    store<u16>(e + 96, 0);
    for (;;) {
        const u32 ck = load<u32>(cur + 4);
        if (ck == k) {
            store<u32>(e + 100, cur);
            store<u32>(kFxBindGlobal, load<u32>(cur + 8));
            return true;
        }
        const u32 next = load<u32>(cur + (ck > k ? 12 : 16));
        if (next == 0) {
            store<u32>(e + 100, cur);
            return false;
        }
        const s32 d = static_cast<s16>(load<u16>(e + 96));
        store<u16>(e + 96, static_cast<u16>(d + 1));
        store<u32>(e + 16 + 4 * static_cast<u32>(d), cur);
        cur = next;
    }
}

// Appends {key, val} to the sorted map array m (u16 cap +0, count +2, peak
// +4, flags +6, data +8) unless find(m, &key) reports a slot (*slot != 0);
// the peak is stored before the flags. Returns r3 as the original leaves it
// (the slot word, or the new flags).
u32 MapAddNew(PPUContext& ctx, u32 m, u32 find, u32 keyp, u32 val) {
    const u32 w = load<u32>(static_cast<u32>(call(ctx, find, {m, keyp})));
    if (w != 0) return w;
    const u32 cap = load<u16>(m);
    if (load<u16>(m + 2) == cap) call(ctx, ArrReserve, {m, (cap << 1) & 0xFFFEu});
    const u32 c = load<u16>(m + 2);
    const u32 d = load<u32>(m + 8);
    store<u32>(d + ((c << 3) & 0x7FFF8u), load<u32>(keyp));
    store<u32>(d + 8 * load<u16>(m + 2) + 4, val);
    const u32 n = load<u16>(m + 2) + 1;
    const u32 peak = load<u16>(m + 4);
    const u32 fl = load<u16>(m + 6) | 1u;
    store<u16>(m + 2, static_cast<u16>(n));
    store<u16>(m + 4, static_cast<u16>(static_cast<s32>(peak) > static_cast<s32>(n & 0xFFFFu) ? peak : n));
    store<u16>(m + 6, static_cast<u16>(fl));
    return fl;
}
}  // namespace

// u32 f(T* o) (a vtable method): when o->+40 has bit 0x40000000 and the
// def o->+140 is of kind 0x2DA0A51D / 0x0020EE36 / 0x00011A6B / 0x8CFF6308
// (+72), the component 23 of o->+8 (Obj_FindComponent) gets
// Obj_SetField20_00276558(c, 0). Obj_PopBodies_00275030(o); when o->+8's
// vt[60] gives an animator: with bit 0x80000000 of +40, Anim_Stop254814(it,
// o->+136->+60); its emitter pairs (+384 / +388, count +448) are stopped
// from the first (Emitter_Stop(h, 1), the last pair moved into the first
// and cleared). Obj_SwapEffect_00274EB0(o, 0, 1); +40 keeps bits 0x79F and
// gains 0x10000, +24 keeps 0x001FFFFF, +44..+56, +136, +140, +28..+36 = 0.
// r3 = the new +40.
DECOMP(002766A8, Obj_Reset_002766A8) {
    const u32 o = arg<u32>(ctx, 0);
    if ((load<u32>(o + 40) & 0x40000000u) != 0 && load<u32>(o + 140) != 0) {
        if (const u32 c = static_cast<u32>(call(ctx, 0x000D1288u, {load<u32>(o + 8), 23, 0})); c != 0) {
            const u32 k = load<u32>(load<u32>(o + 140) + 72);
            if (k == 0x2DA0A51Du || k == 0x0020EE36u || k == 0x00011A6Bu || k == 0x8CFF6308u) call(ctx, 0x00276558u, {c, 0});
        }
    }
    call(ctx, 0x00275030u, {o});
    const u32 d = load<u32>(o + 8);
    if (VCall(ctx, d, 60, {d}) != 0) {
        if ((load<u32>(o + 40) & 0x80000000u) != 0) {
            const u32 d2 = load<u32>(o + 8);
            const u32 an = static_cast<u32>(VCall(ctx, d2, 60, {d2}));
            call(ctx, 0x00254814u, {an, load<u32>(load<u32>(o + 136) + 60)});
        }
        const u32 d3 = load<u32>(o + 8);
        const u32 a = static_cast<u32>(VCall(ctx, d3, 60, {d3}));
        const u32 h = a + 384, v = a + 388;
        while (static_cast<s32>(load<u32>(a + 448)) > 0) {
            call(ctx, 0x00443424u, {load<u32>(h), 1});
            const u32 n = load<u32>(a + 448) - 1;
            store<u32>(a + 448, n);
            store<u32>(h, load<u32>(h + 8 * n));
            store<u32>(v, load<u32>(v + 8 * load<u32>(a + 448)));
            store<u32>(h + 8 * load<u32>(a + 448), 0);
            store<u32>(v + 8 * load<u32>(a + 448), 0);
        }
    }
    call(ctx, 0x00274EB0u, {o, 0, 1});
    const u32 f = (load<u32>(o + 40) & 0x0000079Fu) | 0x00010000u;
    const u32 g = load<u32>(o + 24) & 0x001FFFFFu;
    store<u32>(o + 44, 0);
    store<u32>(o + 48, 0);
    store<u32>(o + 52, 0);
    store<u32>(o + 40, f);
    store<u32>(o + 56, 0);
    store<u32>(o + 136, 0);
    store<u32>(o + 140, 0);
    store<u32>(o + 24, g);
    store<u32>(o + 28, 0);
    store<u32>(o + 32, 0);
    store<u32>(o + 36, 0);
    ctx.gpr[3] = f;
}

// u32 f(T* o, Grabbable* x): the begin-grab handler. The filter callback
// o->+72 (OPD; called with o, o->+76) may refuse (r3 = 0); states above 1
// (+68, signed) refuse too. Else +124 gains 0x00400000, Obj_SetStateFx266E6C(o, 2),
// the previous target +64 gets vt[40](p, 0) and x becomes the target (its
// u16 +178 counted up); o->+8's vt[248](d, x) may refuse (r3 = its 0).
// With bit 0x40000000 of +124: a flagged (+192 bit 8) target gets
// Obj_Reset6_0012D290; in state 2: +124 gains 0x00200000, state 3 or 0
// (bit 0x80000000 of +124), o->+8 vt[252], the target vt[40](t, 0) and
// cleared, the done callback +80 (OPD; o, o->+84) called; in state 0 only
// the flag and state. r3 = 1 or o->+8's vt[248] result.
DECOMP(002B72DC, Grab_Begin_002B72DC) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 x = arg<u32>(ctx, 1);
    if (const u32 cb = load<u32>(o + 72); cb != 0) {
        if (call_ptr(ctx, cb, {o, load<u32>(o + 76), cb, load<u32>(cb)}) == 0) {
            ctx.gpr[3] = 0;
            return;
        }
    }
    if (const u32 s = load<u32>(o + 68); s == 3 || static_cast<s32>(s) > 1) {  // cror eq, cr1.gt
        ctx.gpr[3] = 0;
        return;
    }
    store<u32>(o + 124, load<u32>(o + 124) | 0x00400000u);
    call(ctx, kSetStateFx, {o, 2});
    if (const u32 p = load<u32>(o + 64); p != 0) VCall(ctx, p, 40, {p, 0});
    store<u32>(o + 64, x);
    u32 r = 1, t = x, d = load<u32>(o + 8);
    store<u16>(x + 178, static_cast<u16>(load<u16>(x + 178) + 1));
    if (d != 0) {
        r = static_cast<u32>(VCall(ctx, d, 248, {d, x}));
        d = load<u32>(o + 8);
        t = load<u32>(o + 64);
        if (r == 0) {
            ctx.gpr[3] = 0;
            return;
        }
    }
    u32 f = load<u32>(o + 124);
    if ((f & 0x40000000u) != 0) {
        if (t != 0 && (load<u32>(t + 192) & 8u) != 0) {
            call(ctx, 0x0012D290u, {t, d});  // Obj_Reset6_0012D290 (r4 = o->+8 as left)
            f = load<u32>(o + 124);
        }
        const u32 s = load<u32>(o + 68);
        if (s == 2 || s == 0) {
            const u32 f2 = f | 0x00200000u;
            store<u32>(o + 124, f2);
            call(ctx, kSetStateFx, {o, (f2 & 0x80000000u) != 0 ? 3u : 0u});
            if (s == 2) {
                if (const u32 d2 = load<u32>(o + 8); d2 != 0) VCall(ctx, d2, 252, {d2});
                if (const u32 p = load<u32>(o + 64); p != 0) VCall(ctx, p, 40, {p, 0});
                store<u32>(o + 64, 0);
                if (const u32 cb = load<u32>(o + 80); cb != 0) call_ptr(ctx, cb, {o, load<u32>(o + 84), load<u32>(cb)});
            }
        }
    }
    ctx.gpr[3] = r;
}

// u32 f(T* o, Ev* x) (through an adjustor thunk): the effect binding of
// o->+40 (**+40 ->+24) is looked up for x->+8 (BindLookup); when found
// with a node value, Effect_SetHash4CF988(o->+40, x->+8) ends it. Else the
// same with the effect of o->+8's vt[272]: found with a value, its
// Effect_SetHash4CF988(vt[272] again, x->+8). r3 as the original leaves it.
DECOMP(002B1A14, FxBind_Apply_002B1A14) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 x = arg<u32>(ctx, 1);
    if (const u32 p = load<u32>(o + 40); p != 0) {
        if (const u32 p0 = load<u32>(p); p0 != 0) {
            if (const u32 e = load<u32>(p0 + 24); e != 0 && BindLookup(e, load<u32>(x + 8))) {
                const u32 k = load<u32>(x + 8);
                const u32 e2 = load<u32>(p0 + 24);
                const u32 p2 = load<u32>(o + 40);
                if (e2 != 0) {
                    if (const u32 q = load<u32>(e2 + 100); q != 0 && load<u32>(q + 8) != 0) {
                        call(ctx, kSetHash, {p2, k});
                        return;
                    }
                }
            }
        }
    }
    u32 d = load<u32>(o + 8);
    u32 r = static_cast<u32>(VCall(ctx, d, 272, {d}));
    if (r == 0) {
        ctx.gpr[3] = 0;
        return;
    }
    d = load<u32>(o + 8);
    r = static_cast<u32>(VCall(ctx, d, 272, {d}));
    const u32 p0 = load<u32>(r);
    ctx.gpr[3] = r;
    if (p0 == 0) return;
    const u32 e = load<u32>(p0 + 24);
    if (e == 0) return;
    ctx.gpr[3] = 0;
    if (!BindLookup(e, load<u32>(x + 8))) return;
    const u32 e2 = load<u32>(p0 + 24);
    const u32 d2 = load<u32>(o + 8);
    if (e2 == 0) return;
    const u32 q = load<u32>(e2 + 100);
    if (q == 0 || load<u32>(q + 8) == 0) return;
    const u32 fx = static_cast<u32>(VCall(ctx, d2, 272, {d2}));
    const u32 k = load<u32>(x + 8);
    call(ctx, kSetHash, {fx, k});
}

// u32 f(T* o) (a vtable method; the 0x00B1D070 object of
// Obj_Dtor_B1D070_002C9910): when u8 +336, f_002C5C34(o) and f_002C96A8(o;
// f1 = *0x002C9E20); the state bytes and words (+193, +204 / +205 = 1,
// +288..+291, +316..+320, +324, +336 / +337, +340) reset and
// f_002C2420(o, 0); +432 = 0; each 12-byte record of +356 (u16 +364, then
// 0): its object (+0) re-armed (CallK2_000D9614(it) vt[44](c, 1) when u8
// +8) then stopped (vt[40](e, 0)) and cleared, its +4 released (vt[8],
// cleared); +424 gets Obj_Dispatch362(p, 1, 1, 0); +620 = 0; +296 vt[40](e,
// 0), cleared with u8 +288 = 0; the flag words +64 / +52 lose bits
// 0xFC000000 and get the defaults 0x01164AFC / 0x01164B00 at +68 / +72 and
// +56 / +60; +304..+319 cleared; when u8 +320 is set again, it and +324 are
// cleared and f_002C2420(o, 0); the base +24 gets vt[28](b, 0, 0); then
// f_002C96A8(o; f1 = *0x002C9E20) (r3 = its).
DECOMP(002C9E28, Obj_Reset_B1D070_002C9E28) {
    const u32 o = arg<u32>(ctx, 0);
    if (load<u8>(o + 336) != 0) {
        call(ctx, 0x002C5C34u, {o});
        callf(ctx, 0x002C96A8u, {o}, {Lf(kFxOff)});
    }
    store<u8>(o + 193, 0);
    store<u8>(o + 204, 1);
    store<u8>(o + 205, 1);
    for (u32 off : {288u, 317u, 318u, 319u, 316u, 289u, 290u, 291u, 336u, 337u}) store<u8>(o + off, 0);
    const f64 f31 = Lf(kFxOff);
    store<u32>(o + 340, 0);
    store<u8>(o + 320, 0);
    store<u32>(o + 324, 0);
    call(ctx, 0x002C2420u, {o, 0});
    store<u32>(o + 432, 0);
    for (u32 i = 0; i < load<u16>(o + 364); ++i) {
        const u32 rec = load<u32>(o + 356) + 12 * i;
        if (const u32 e = load<u32>(rec); e != 0) {
            if (const u32 c = static_cast<u32>(call(ctx, 0x000D9614u, {e})); c != 0 && load<u8>(rec + 8) != 0) VCall(ctx, c, 44, {c, 1});
        }
        if (const u32 e = load<u32>(rec); e != 0) {
            VCall(ctx, e, 40, {e, 0});
            store<u32>(rec, 0);
        }
        Vt8Slot(ctx, rec + 4, true);
    }
    store<u16>(o + 364, 0);
    if (const u32 p = load<u32>(o + 424); p != 0) call(ctx, 0x0036325Cu, {p, 1, 1, 0});
    store<u32>(o + 620, 0);
    if (const u32 e = load<u32>(o + 296); e != 0) {
        VCall(ctx, e, 40, {e, 0});
        store<u32>(o + 296, 0);
        store<u8>(o + 288, 0);  // (cntlzw(0) >> 5) ^ 1 of the word just cleared
    }
    const u32 f64w = load<u32>(o + 64) & 0x03FFFFFFu;
    const u32 f52w = load<u32>(o + 52) & 0x03FFFFFFu;
    constexpr u32 kDef = 0x01164AFCu;
    store<u32>(o + 64, f64w);
    store<u32>(o + 68, load<u32>(kDef));
    store<u32>(o + 72, load<u32>(kDef + 4));
    const u32 again = load<u8>(o + 320);
    store<u32>(o + 52, f52w);
    store<u32>(o + 56, load<u32>(kDef));
    store<u8>(o + 304, 0);
    store<u32>(o + 60, load<u32>(kDef + 4));
    store<u32>(o + 308, 0);
    store<u32>(o + 312, 0);
    store<u8>(o + 316, 0);
    store<u8>(o + 317, 0);
    store<u8>(o + 319, 0);
    ctx.fpr[1] = Lf(kDef + 4);  // left over from the copies for the calls below
    if (again != 0) {
        store<u8>(o + 320, 0);
        store<u32>(o + 324, 0);
        call(ctx, 0x002C2420u, {o, 0});
    }
    VCall(ctx, o + 24, 28, {o + 24, 0, 0});
    callf(ctx, 0x002C96A8u, {o}, {f31});
}

// u32 f(u32 key, a, b, c) (World_Construct): key is registered with a /
// b / c in the sorted maps 0x010686C4 / 0x010686DC (SortedMap_Find766DCC)
// and 0x010686F4 (SortedMap_Find766F54) unless already present (the key
// is passed by address, on the stack). r3 as the last map step leaves it.
DECOMP(002E9FE0, World_RegisterKey_002E9FE0) {
    Stack frame(ctx, 16);  // the key at 112(r1)
    const u32 kp = frame.addr();
    store<u32>(kp, arg<u32>(ctx, 0));
    const u32 a = arg<u32>(ctx, 1), b = arg<u32>(ctx, 2), c = arg<u32>(ctx, 3);
    MapAddNew(ctx, 0x010686C4u, 0x00766DCCu, kp, a);
    MapAddNew(ctx, 0x010686DCu, 0x00766DCCu, kp, b);
    ctx.gpr[3] = MapAddNew(ctx, 0x010686F4u, 0x00766F54u, kp, c);
}

// u32 f(T* o, Attachable* x): x is released from the attachment table
// +24 (144-byte entries, count +64). Quirk: the search only ever compares
// the first entry's +48 (the entry pointer is never advanced). Found: x's
// physics record (vt[144]) gets FLT_MAX at +410 / +411 (Hk_SetF32At, values
// on the stack); the entry's +48 = 0 and the count drops; x's and o->+8's
// effects (vt[272]) get Effect_SetHash4CF988(fx, 0x3E5C25CD); with bit 2 of
// x->+192 bit 0x10000000 of its +376 is cleared, with bit 0x20
// Body_SetMass_001FB1C8(x; f1 = the entry's +128); Obj_Detach0D72C0(o->+8,
// x). r3 = 1, or 0 when x is not found.
DECOMP(002B99C0, Attach_Release_002B99C0) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 x = arg<u32>(ctx, 1);
    Stack frame(ctx, 16);  // the two FLT_MAX words at 112 / 116(r1)
    const s32 n = static_cast<s32>(load<u32>(o + 64));
    const u32 e = load<u32>(o + 24);
    bool found = false;
    for (s32 i = 0; i < n && !found; ++i) found = x != 0 && load<u32>(e + 48) == x;
    if (!found) {
        ctx.gpr[3] = 0;
        return;
    }
    if (VCall(ctx, x, 144, {x}) != 0) {
        const u32 p = static_cast<u32>(VCall(ctx, x, 144, {x}));
        store<u32>(frame.addr() + 4, 0x7F7FFFFFu);
        call(ctx, 0x007EE08Cu, {p + 410, frame.addr() + 4});
        const u32 p2 = static_cast<u32>(VCall(ctx, x, 144, {x}));
        store<u32>(frame.addr(), 0x7F7FFFFFu);
        call(ctx, 0x007EE08Cu, {p2 + 411, frame.addr()});
    }
    store<u32>(e + 48, 0);
    store<u32>(o + 64, load<u32>(o + 64) - 1);
    if (VCall(ctx, x, 272, {x}) != 0) {
        const u32 fx = static_cast<u32>(VCall(ctx, x, 272, {x}));
        call(ctx, kSetHash, {fx, 0x3E5C25CDu});
    }
    if (const u32 d = load<u32>(o + 8); d != 0 && VCall(ctx, d, 272, {d}) != 0) {
        const u32 d2 = load<u32>(o + 8);
        const u32 fx = static_cast<u32>(VCall(ctx, d2, 272, {d2}));
        call(ctx, kSetHash, {fx, 0x3E5C25CDu});
    }
    if ((load<u32>(x + 192) & 2u) != 0) store<u32>(x + 376, load<u32>(x + 376) & ~0x10000000u);
    if ((load<u32>(x + 192) & 0x20u) != 0) callf(ctx, 0x001FB1C8u, {x}, {Lf(e + 128)});
    call(ctx, 0x000D72C0u, {load<u32>(o + 8), x});
    ctx.gpr[3] = 1;
}
