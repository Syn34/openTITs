// Game code, part 39: the small-function sweep continued from the 80
// instruction functions (size order; obj38*.cpp did 76-79). Each was
// checked against the original in the PPC simulator (memory writes, calls
// and their arguments, r3 and f1).
#include "obj39.h"

using namespace sly4;
using namespace sly4::obj39;

// ---------------------------------------------------------------- sweep: 80 instructions
// s32 f(T* o, u32 key, u32 val): the entries e of the pointer array
// o + 3732 (s32 count o->+24) with e->+0 == key, Obj_Entry16_000DA954(key,
// e->+12) != 0 and e->+8 == val are removed: the count drops by one and
// the entry swaps places with the (new) last one, which is then looked at
// in its place. r3 = the final index (the count, or o->+24 when it starts
// at 0 or below).
DECOMP(0001BCF4, Slots_RemoveMatching_0001BCF4) {
    const u32 o = arg<u32>(ctx, 0), key = arg<u32>(ctx, 1), val = arg<u32>(ctx, 2);
    const u32 tab = o + 3732;
    s32 i = 0;
    const s32 n = load<s32>(o + 24);
    if (!(i < n)) {
        ret(ctx, n);
        return;
    }
    do {
        const u32 e = load<u32>(tab + 4 * static_cast<u32>(i));
        if (load<u32>(e) == key && static_cast<u32>(call(ctx, 0x000DA954u, {key, Sx39(load<u32>(e + 12))})) != 0 &&
            load<u32>(e + 8) == val) {
            const u32 c = load<u32>(o + 24) - 1;
            store<u32>(o + 24, c);
            const u32 p = tab + 4 * static_cast<u32>(i);
            const u32 l = tab + 4 * load<u32>(o + 24);
            const u32 a = load<u32>(p), b = load<u32>(l);
            store<u32>(l, a);
            store<u32>(p, b);
            --i;
        }
        ++i;
    } while (i < load<s32>(o + 24));
    ret(ctx, i);
}

// u32 f(T* o) (no direct caller): by the state o->+4: 10 and 7 set the u8
// o->+513 and, when the u8 o->+511 was set, clear it and
// Obj_Dispatch362(o->+492[o->+468], 1, 1, 0); 6 clears o->+448; 8 releases
// a group: Group_ReleaseById_0042C00C(0x1840AF90) when the u8 0x00F9C660
// is set, else the id (o->+308) - 1 of the 16-word table 0x00AC034C copied
// to the stack (index -1 reads the word before it: a quirk). r3 = 0 or the
// last call's.
DECOMP(000616C4, State_OnEvent_000616C4) {
    const u32 o = arg<u32>(ctx, 0);
    const s32 s = load<s32>(o + 4);
    ctx.gpr[3] = 0;
    if (s == 10 || s == 7) {
        const u8 was = load<u8>(o + 511);
        store<u8>(o + 513, 1);
        if (was == 0) return;
        store<u8>(o + 511, 0);
        call(ctx, 0x0036325Cu, {load<u32>(o + (load<u32>(o + 468) << 2) + 492), 1, 1, 0});
    } else if (s == 8) {
        Stack fr(ctx, 80);  // the table at sp+112 (fr + 16); sp+108 = fr + 12
        call(ctx, 0x0071C3F0u, {fr.addr() + 16, 0x00AC034Cu, 64});
        u32 id = 0x1840AF90u;
        if (load<u8>(0x00F9C618u + 72) == 0) id = load<u32>(fr.addr() + 12 + (load<u32>(o + 308) << 2));
        call(ctx, 0x0042C00Cu, {id});
    } else if (s == 6) {
        store<u32>(o + 448, 0);
    }
}

// u32 f(T* o, Effect* x) (no direct caller): with the component id o->+24
// and the mask o->+20 both set: the owner y = the bound object (when
// Effect_Bind4BB7E0(x, o->+12, 0) has type 0x02235C33 at +276, its +280)
// or else Obj_FindById(o->+16) (when set); when y exists, the component c =
// Effect_FindComponent(x, o->+24) whose c->vt[8](c) (OPD / entry after the
// argument) returns 2 gets c->+20 = (u64 y->+184 & o->+20) != 0. r3 = 0,
// the vt result, or that flag.
DECOMP(0007B0CC, EffectCmd_SetCompFlag_0007B0CC) {
    const u32 o = arg<u32>(ctx, 0), x = arg<u32>(ctx, 1);
    ctx.gpr[3] = load<u32>(o + 24);
    if (ctx.gpr[3] == 0) return;
    ctx.gpr[3] = load<u32>(o + 20);
    if (ctx.gpr[3] == 0) return;
    u32 y = 0;
    const u32 h = load<u32>(o + 12);
    if (h != 0) {
        const u32 b = static_cast<u32>(call(ctx, 0x004BB7E0u, {x, h, 0}));
        if (b != 0 && load<u32>(b + 276) == 0x02235C33u) y = load<u32>(b + 280);
    }
    const u32 id = load<u32>(o + 16);
    ctx.gpr[3] = id;
    if (id != 0 && y == 0) y = static_cast<u32>(call(ctx, 0x000D6A28u, {id}));
    if (y == 0) return;
    const u32 flag = (load<u64>(y + 184) & static_cast<u64>(load<u32>(o + 20))) != 0 ? 1u : 0u;
    const u32 c = static_cast<u32>(call(ctx, 0x004BB4B4u, {x, load<u32>(o + 24)}));
    if (c == 0) return;
    if (static_cast<u32>(VCall(ctx, c, 32, {c})) != 2) return;
    store<u32>(c + 20, flag);
    ret(ctx, flag);
}

// u32 f(T* o) (no direct caller): for each of the o->+124 28-byte records
// at o + 12 (u16 count +8, pointer array +0) and then the pointer array
// o->+136 (u16 count o->+144), every non-null element p gets p->vt[5](p)
// (OPD / entry after the argument). Counts and arrays are re-read each
// step. r3 = the last u16 o->+144 read.
DECOMP(0009870C, Obj_TickChildren_0009870C) {
    const u32 o = arg<u32>(ctx, 0);
    if (load<u32>(o + 124) != 0) {
        u32 k = 0;
        do {
            const u32 r = o + 12 + 28 * k;
            for (u32 j = 0; j < load<u16>(r + 8); ++j) {
                const u32 p = load<u32>(load<u32>(r) + 4 * j);
                if (p != 0) VCall(ctx, p, 20, {p});
            }
            ++k;
        } while (k < load<u32>(o + 124));
    }
    u32 j = 0;
    for (;;) {
        const u32 n = load<u16>(o + 144);
        ctx.gpr[3] = n;
        if (!(j < n)) break;
        const u32 p = load<u32>(load<u32>(o + 136) + 4 * j);
        if (p != 0) VCall(ctx, p, 20, {p});
        ++j;
    }
}

// u32 f(Def* o) (no direct caller): when o->+84 is set and
// Ptr_DerefNonZero(o->+92) is 0, the names of o->+84 / o->+88 are looked
// up (Hash_ToString, results unused: a stripped warning); then a 220-byte
// command c = SmallAlloc(220), set up when allocated by 0x0043267C(c, +92,
// o + 12, +80, o + 128, +104, +108, u8 +100, -, +116, u8 +120, u8 +121,
// +124, o + 128, u8 +192, +196, (s32) +200, +204; f1 = f32 +96, f2 = f32
// +112), goes to CmdList_Push_0042C138(c) (r3 = its; c = 0 on failure).
DECOMP(000A0B1C, CmdDef_Spawn_000A0B1C) {
    const u32 o = arg<u32>(ctx, 0);
    if (load<u32>(o + 84) != 0 && static_cast<u32>(call(ctx, 0x00446960u, {load<u32>(o + 92)})) == 0) {
        call(ctx, 0x0056A6E0u, {load<u32>(o + 84)});
        call(ctx, 0x0056A6E0u, {load<u32>(o + 88)});
    }
    const u32 c = static_cast<u32>(call(ctx, 0x0055238Cu, {220}));
    if (c != 0) {
        // argument slot 8 (sp+112) is never written by the original; 0 here
        callf(ctx, 0x0043267Cu,
              {c, load<u32>(o + 92), o + 12, load<u32>(o + 80), o + 128, load<u32>(o + 104), load<u32>(o + 108),
               load<u8>(o + 100), 0, load<u32>(o + 116), load<u8>(o + 120), load<u8>(o + 121), load<u32>(o + 124),
               o + 128, load<u8>(o + 192), load<u32>(o + 196), Sx39(load<u32>(o + 200)), load<u32>(o + 204)},
              {Lfs32(o + 96), Lfs32(o + 112)});
    }
    call(ctx, 0x0042C138u, {c});
}

// T* f(?, XmlCursor* x) (no direct caller): a 24-byte node (Mem_Alloc(24,
// 0, 16); 0 when that fails) set up by Obj_InitVt_B21C98(p, x), vtable
// 0x00B11AA8, +12 / +16 = 0, u8 +20 / +21 = 0, u8 +22 = 1, then read from
// x: hashes 0x7A30D7E3 -> +12 (default 0), 0x000107E5 -> +16, bools
// 0x00D8E5D6 -> +20, 0xA18DAB9F -> +21, 0x8488565B -> +22. r3 = p.
DECOMP(000A7018, XmlNew_Node24_000A7018) {
    const u32 x = arg<u32>(ctx, 1);
    const u32 p = static_cast<u32>(call(ctx, 0x0054ED38u, {24, 0, 16}));
    if (p == 0) {
        ret(ctx, 0u);
        return;
    }
    call(ctx, 0x004BB3B0u, {p, x});
    store<u32>(p + 12, 0);
    store<u32>(p + 16, 0);
    store<u8>(p + 20, 0);
    store<u32>(p, 0x00B11AA8u);
    store<u8>(p + 21, 0);
    store<u8>(p + 22, 1);
    call(ctx, 0x004FA14Cu, {x, 0x7A30D7E3u, p + 12, 0});
    call(ctx, 0x004FA14Cu, {x, 0x000107E5u, p + 16, load<u32>(p + 16)});
    call(ctx, 0x004F986Cu, {x, 0x00D8E5D6u, p + 20, load<u8>(p + 20)});
    call(ctx, 0x004F986Cu, {x, 0xA18DAB9Fu, p + 21, load<u8>(p + 21)});
    call(ctx, 0x004F986Cu, {x, 0x8488565Bu, p + 22, load<u8>(p + 22)});
    ret(ctx, p);
}

// Box* f(Box* out, Obj* o, bool world) (no direct caller): out = the empty
// box (min +0..+8 = FLT_MAX, max +16..+24 = -FLT_MAX); with the world
// object w = o->+160: world ? WorldObj_WorldBounds(w, out) : out = w's
// local box (w->+208 + 16) scaled per axis by w->+144; without w,
// Box_SetPoint(out, world ? o + 144 : the origin 0x01164B40). r3 = out,
// or the bounds call's.
DECOMP(000DAC64, Obj_GetBounds_000DAC64) {
    const u32 out = arg<u32>(ctx, 0), o = arg<u32>(ctx, 1);
    const bool world = (arg<u32>(ctx, 2) & 0xFFu) != 0;
    store<u32>(out + 16, 0xFF7FFFFFu);
    store<u32>(out + 0, 0x7F7FFFFFu);
    store<u32>(out + 4, 0x7F7FFFFFu);
    store<u32>(out + 8, 0x7F7FFFFFu);
    store<u32>(out + 20, 0xFF7FFFFFu);
    store<u32>(out + 24, 0xFF7FFFFFu);
    ctx.gpr[3] = out;
    const u32 w = load<u32>(o + 160);
    if (w == 0) {
        call(ctx, 0x00556F60u, {out, world ? o + 144 : 0x01164B40u});
        return;
    }
    if (world) {
        call(ctx, 0x005772E4u, {w, out});
        return;
    }
    const u32 b = load<u32>(w + 208) + 16;
    store<u32>(out + 0, load<u32>(b + 0));
    store<u32>(out + 4, load<u32>(b + 4));
    store<u32>(out + 8, load<u32>(b + 8));
    store<u32>(out + 16, load<u32>(b + 16));
    const u32 s = load<u32>(o + 160) + 144;
    store<u32>(out + 20, load<u32>(b + 20));
    store<u32>(out + 24, load<u32>(b + 24));
    const f64 s0 = Lfs32(s), s1 = Lfs32(s + 4), s2 = Lfs32(s + 8);
    const f64 a0 = F39(Lfs32(out) * s0), a1 = F39(Lfs32(out + 4) * s1), a2 = F39(Lfs32(out + 8) * s2);
    Stfs32(out, a0);
    Stfs32(out + 4, a1);
    Stfs32(out + 8, a2);
    const f64 b0 = F39(Lfs32(out + 16) * s0), b1 = F39(Lfs32(out + 20) * s1), b2 = F39(Lfs32(out + 24) * s2);
    Stfs32(out + 16, b0);
    Stfs32(out + 20, b1);
    Stfs32(out + 24, b2);
    ctx.fpr[1] = b0;
}

// u32 f(Obj* o, ?, bool scale, u32 key; f32 v): first recursively for the
// child of each 240-byte part of o->+340 (u16 count +8, array +0) whose
// +176 has bit 0x80000000 set (f(part->+192, 1, scale, key; v)); then the
// track y = Anim_Find34CC34(o->+164, key) (when o->+164 is set) gets
// Obj_SetF32Via_00354150(y; f1 = scale ? (len > 0 ? len * v : 1.0) : v)
// with len = Track_Length353748(y). r3 = o->+340 (no o->+164), 0 (no
// track) or the last call's.
DECOMP(000DC550, Anim_SetTrackTime_000DC550) {
    const u32 o = arg<u32>(ctx, 0), key = arg<u32>(ctx, 3);
    const u32 scale = arg<u32>(ctx, 2) & 0xFFu;
    f64 v = farg(ctx, 0);
    u32 i = 0;
    for (;;) {
        const u32 a = load<u32>(o + 340);
        ctx.gpr[3] = a;
        const u32 n = a != 0 ? load<u16>(a + 8) : 0u;
        if (!(i < n)) break;
        const u32 e = load<u32>(a) + 240 * i;
        if ((load<u32>(e + 176) & 0x80000000u) != 0) callf(ctx, 0x000DC550u, {load<u32>(e + 192), 1, scale, key}, {v});
        ++i;
    }
    const u32 x = load<u32>(o + 164);
    if (x == 0) return;
    const u32 y = static_cast<u32>(call(ctx, 0x0034CC34u, {x, key}));
    if (y == 0) return;
    if (scale != 0) {
        const f64 len = callf(ctx, 0x00353748u, {y}, {});
        constexpr u32 K = 0x000DC548u;  // {1e-5f, 1.0f}
        v = len > Lfs32(K) ? F39(len * v) : Lfs32(K + 4);
    }
    callf(ctx, 0x00354150u, {y}, {v});
}

// u32 f(Obj* o): the 2D direction of o: with the frame m = v->vt[3](v)
// (v = o->+380; OPD / entry after the argument), the vectors o + 96 and o +
// 128 are projected on m's row +16 (x and y), each clamped to [-1, 1]
// (fsel, f32 constants 0x00165C40); r3 = |x| > |y| ? (x < 0 ? 2 : 1) : (y
// < 0 ? 3 : 0). f1 = 0.0 or the clamped y.
DECOMP(00165C60, Obj_Dir4_00165C60) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 v = load<u32>(o + 380);
    const u32 m = static_cast<u32>(VCall(ctx, v, 12, {v}));
    constexpr u32 K = 0x00165C40u;  // {1.0f, 0.0f, 1.0, -1.0f, -, -1.0}
    const f64 m20 = Lfs32(m + 20), m16 = Lfs32(m + 16), m24 = Lfs32(m + 24);
    const u32 a = o + 96, b = o + 128;
    f64 x = F39(m20 * Lfs32(a + 4));
    f64 y = F39(m20 * Lfs32(b + 4));
    x = F39(std::fma(m16, Lfs32(a), x));
    y = F39(std::fma(m16, Lfs32(b), y));
    x = F39(std::fma(m24, Lfs32(a + 8), x));
    y = F39(std::fma(m24, Lfs32(b + 8), y));
    const f64 k0 = Lfs32(K), kd8 = std::bit_cast<f64>(load<u64>(K + 8)), k16 = Lfs32(K + 16),
              kd24 = std::bit_cast<f64>(load<u64>(K + 24));
    x = F39(Fsel38(F39(k0 - x), x, kd8));
    y = F39(Fsel38(F39(k0 - y), y, kd8));
    x = F39(Fsel38(F39(x - k16), x, kd24));
    y = F39(Fsel38(F39(y - k16), y, kd24));
    const f64 k4 = Lfs32(K + 4);
    if (std::fabs(x) > std::fabs(y)) {
        ctx.fpr[1] = k4;
        ret(ctx, x < k4 ? 2u : 1u);
    } else {
        ctx.fpr[1] = y;
        ret(ctx, y < k4 ? 3u : 0u);
    }
}

// T* f(T* t, const Def* d) (the DamageColDef_NewD0208 constructor body):
// base Thunk_000E27FC(t, d), vtable 0x00B15970, then from d: f32 +288..
// +296 -> t + 368.., +304 / +308 -> +384 / +388, f32 +312 -> +392, bit
// 0x80 of u8 +316 into u8 t->+396, the seven 8-byte pairs d + 224.. -> t +
// 304.., +320 -> +400; vtable 0x00B17490 and +324 -> +404. r3 = d->+324.
DECOMP(001CFE00, DamageColDef_Ctor_001CFE00) {
    const u32 t = arg<u32>(ctx, 0), d = arg<u32>(ctx, 1);
    call(ctx, 0x000E27FCu, {t, d});
    store<u32>(t, 0x00B15970u);
    store<u32>(t + 368, load<u32>(d + 288));
    store<u32>(t + 372, load<u32>(d + 292));
    store<u32>(t + 376, load<u32>(d + 296));
    store<u32>(t + 384, load<u32>(d + 304));
    const u8 b = load<u8>(t + 396);
    store<u32>(t + 388, load<u32>(d + 308));
    store<u32>(t + 392, load<u32>(d + 312));
    store<u8>(t + 396, static_cast<u8>((b & 0x7Fu) | (load<u8>(d + 316) & 0x80u)));
    const u32 w320 = load<u32>(d + 320), w324 = load<u32>(d + 324);
    for (u32 k = 0; k < 7; ++k) {
        store<u32>(t + 304 + 8 * k, load<u32>(d + 224 + 8 * k));
        store<u32>(t + 308 + 8 * k, load<u32>(d + 228 + 8 * k));
    }
    store<u32>(t + 400, w320);
    store<u32>(t + 0, 0x00B17490u);
    store<u32>(t + 404, w324);
    ctx.fpr[1] = Lfs32(d + 276);  // the last f32 copied through f1
    ret(ctx, w324);
}

// u32 f(T* o, Obj* x, u32 key): adds x to the 12-byte records of o->+2460
// (u16 capacity +2464, count +2468, peak +2470) unless already there (r3 =
// x): with key != 0 only when key is one of the four words o->+2512 + 8 j
// (else r3 = the array); then the u16 x->+4 is incremented, the array
// grows (PtrVector_Reserve(o + 2460, 2 * capacity)) when full, and the
// record {x, key, j} is appended. r3 = the old peak.
DECOMP(002ACAB8, Recs_AddKeyed_002ACAB8) {
    const u32 o = arg<u32>(ctx, 0), x = arg<u32>(ctx, 1), key = arg<u32>(ctx, 2);
    u32 arr = load<u32>(o + 2460);
    u32 n = load<u16>(o + 2468);
    const u32 cap = load<u16>(o + 2464);
    for (u32 i = 0; i < n; ++i) {
        if (load<u32>(arr + 12 * i) == x) {
            ret(ctx, x);
            return;
        }
    }
    ctx.gpr[3] = arr;
    u32 j = 0;
    if (key != 0) {
        while (key != load<u32>(o + 2512 + 8 * j)) {
            ++j;
            if (!(j < 4)) return;
        }
    }
    store<u16>(x + 4, static_cast<u16>(load<u16>(x + 4) + 1));
    if (n == cap) {
        call(ctx, 0x00551718u, {o + 2460, (cap << 1) & 0x1FFFEu});
        arr = load<u32>(o + 2460);
        n = load<u16>(o + 2468);
    }
    const u32 r = arr + 12 * n;
    store<u32>(r + 4, key);
    store<u32>(r + 8, j);
    store<u32>(r + 0, x);
    const u32 n2 = load<u16>(o + 2468) + 1u;
    const u32 peak = load<u16>(o + 2470);
    store<u16>(o + 2468, static_cast<u16>(n2));
    store<u16>(o + 2470, static_cast<u16>(static_cast<s32>(peak) > static_cast<s32>(n2 & 0xFFFFu) ? peak : n2));
    ret(ctx, peak);
}

// u32 f(Obj* o): in state o->+68 == 2 or 0, bit 0x00200000 is set in
// o->+124 and Obj_SetStateFx266E6C(o, bit 0x80000000 of o->+124 ? 3 : 0);
// state 2 then also calls x->vt[63](x) (x = o->+8), y->vt[10](y, 0) (y =
// o->+64) (OPD / entry after the arguments; each when set), clears o->+64
// and calls the callback OPD o->+80 (when set) as f(o, o->+84) (entry
// after the arguments). r3 = 1, or 0 for other states.
DECOMP(002B719C, Obj_Release_002B719C) {
    const u32 o = arg<u32>(ctx, 0);
    const s32 s = load<s32>(o + 68);
    if (s != 2 && s != 0) {
        ret(ctx, 0u);
        return;
    }
    const u32 w = load<u32>(o + 124) | 0x00200000u;
    store<u32>(o + 124, w);
    call(ctx, 0x00266E6Cu, {o, (w & 0x80000000u) != 0 ? 3u : 0u});
    if (s == 2) {
        const u32 x = load<u32>(o + 8);
        if (x != 0) VCall(ctx, x, 252, {x});
        const u32 y = load<u32>(o + 64);
        if (y != 0) VCall(ctx, y, 40, {y, 0});
        store<u32>(o + 64, 0);
        const u32 f = load<u32>(o + 80);
        if (f != 0) call_ptr(ctx, f, {o, load<u32>(o + 84), load<u32>(f)});
    }
    ret(ctx, 1u);
}

// void f(T* o) (no direct caller; a deleting destructor): for o != 0:
// when o->+28 (c) is set, vtable 0x00B1D100, bit 0x200 set in c->+136 and
// o->+28 = 0; vtable 0x00B1BE30; o is taken out of the 8-byte entry array
// of 0x01056F1C (u16 count +2, data +8, the object in each entry's second
// word; later entries moved down one) when found; List1013B5C_Remove(o),
// SmallFree(o, 56) (r3 = its).
DECOMP(002CBEAC, Obj_Delete56_002CBEAC) {
    const u32 o = arg<u32>(ctx, 0);
    if (o == 0) {
        ret(ctx, 0u);
        return;
    }
    const u32 c = load<u32>(o + 28);
    if (c != 0) {
        store<u32>(o, 0x00B1D100u);
        store<u32>(c + 136, load<u32>(c + 136) | 0x200u);
        store<u32>(o + 28, 0);
    }
    store<u32>(o, 0x00B1BE30u);
    Unlist1056F1C39(o);
    call(ctx, 0x000D6940u, {o});
    call(ctx, 0x00552420u, {o, 56});
}

// u32 f(Obj* o, Msg* p) (no direct caller): with the set s = p->+4 + 24
// (when p->+4 is set): for u8 p->+0 set, the entry {o->+204, bit 0x8000 of
// o->+164} is appended to s's 8-byte array (u16 capacity +0, count +2,
// peak +4, flags +6 |= 1, data +8; Array_Reserve(s, 2 * capacity) when
// full); otherwise o->vt[10](o, u8 *SortedMap_Find76116C(s, &o->+204), 0)
// (entry / OPD after the arguments). r3 = 1.
DECOMP(002D6800, Obj_OnSetMsg_002D6800) {
    const u32 o = arg<u32>(ctx, 0), p = arg<u32>(ctx, 1);
    const u32 q = load<u32>(p + 4);
    if (q != 0) {
        const u32 s = q + 24;
        if (load<u8>(p) == 0) {
            Stack fr(ctx, 16);
            store<u32>(fr.addr(), load<u32>(o + 204));
            const u32 e = static_cast<u32>(call(ctx, 0x0076116Cu, {s, fr.addr()}));
            VCallEO36(ctx, load<u32>(o), 40, {o, load<u8>(e), 0});  // entry, then OPD
        } else {
            const u32 b = (load<u32>(o + 164) & 0x8000u) != 0 ? 1u : 0u;
            const u32 val = load<u32>(o + 204);
            u32 n = load<u16>(s + 2);
            const u32 cap = load<u16>(s);
            if (n == cap) {
                call(ctx, 0x00558F3Cu, {s, (cap << 1) & 0xFFFEu});
                n = load<u16>(s + 2);
            }
            const u32 d = load<u32>(s + 8);
            store<u32>(d + ((n << 3) & 0x7FFF8u), val);
            store<u8>(d + (static_cast<u32>(load<u16>(s + 2)) << 3) + 4, static_cast<u8>(b));
            const u32 n2 = load<u16>(s + 2) + 1u;
            const u32 peak = load<u16>(s + 4);
            const u32 fl = load<u16>(s + 6);
            store<u16>(s + 2, static_cast<u16>(n2));
            store<u16>(s + 4, static_cast<u16>(static_cast<s32>(peak) > static_cast<s32>(n2 & 0xFFFFu) ? peak : n2));
            store<u16>(s + 6, static_cast<u16>(fl | 1u));
        }
    }
    ret(ctx, 1u);
}

// u32 f(Shaker* o, const Shake* s): appends the 96-byte shake record s to
// o's array (data +36, u16 capacity +40, count +44, peak +46;
// PtrVector_Reserve(o + 36, 2 * capacity) when full): words +0..+48, +64,
// +68, +72 and +80 are copied. At 16 shakes, String_Format("Too many
// active camera shakes. ...", 16) instead (r3 = its). r3 = the old peak,
// f1 = the f32 +80.
DECOMP(002F2140, CamShake_Add_002F2140) {
    const u32 o = arg<u32>(ctx, 0), s = arg<u32>(ctx, 1);
    u32 n = load<u16>(o + 44);
    if (!(n < 16)) {
        call(ctx, o38fn::String_Format, {0x00AD96B4u, 16});
        return;
    }
    const u32 cap = load<u16>(o + 40);
    if (n == cap) {
        call(ctx, 0x00551718u, {o + 36, (cap << 1) & 0x1FFFEu});
        n = load<u16>(o + 44);
    }
    const u32 r = load<u32>(o + 36) + 96 * n;
    for (u32 k = 0; k <= 48; k += 4) store<u32>(r + k, load<u32>(s + k));
    store<u32>(r + 64, load<u32>(s + 64));
    store<u32>(r + 68, load<u32>(s + 68));
    store<u32>(r + 72, load<u32>(s + 72));
    store<u32>(r + 80, load<u32>(s + 80));
    ctx.fpr[1] = Lfs32(s + 80);  // the copies go through f1
    const u32 n2 = load<u16>(o + 44) + 1u;
    const u32 peak = load<u16>(o + 46);
    store<u16>(o + 44, static_cast<u16>(n2));
    store<u16>(o + 46, static_cast<u16>(static_cast<s32>(peak) > static_cast<s32>(n2 & 0xFFFFu) ? peak : n2));
    ret(ctx, peak);
}

// u32 f(Holder* o, Skel* a, s32 b): o->+8 = 0, u16 o->+6 = 1, vtable
// 0x00B1F480; from the pool P = (r13 -28332)->+44: a 16-byte reference
// p = P->vt[2](P, 16) (u16 +4 = 16 written even when null: a quirk; then
// u16 +6 = 1, +8 = a, vtable 0x00B1F608, u8 +12 = a != 0) and a 36-byte
// object q = P->vt[2](P, 36) (u16 +4 = 36, same quirk) built by
// 0x007DFEF8(q, p, b) when allocated; o->+8 = q, then
// AnimSkel_Release7D4EA8(p) (r3 = its). (OPD / entry after the arguments.)
DECOMP(0034640C, SkelHolder_Init_0034640C) {
    const u32 o = arg<u32>(ctx, 0), a = arg<u32>(ctx, 1), b = arg<u32>(ctx, 2);
    store<u32>(o + 8, 0);
    store<u16>(o + 6, 1);
    store<u32>(o, 0x00B1F480u);
    const u32 P = Pool28332(ctx);
    const u32 p = static_cast<u32>(VCall(ctx, P, 8, {P, 16}));
    store<u16>(p + 4, 16);
    if (p != 0) {
        store<u16>(p + 6, 1);
        store<u32>(p + 8, a);
        store<u32>(p, 0x00B1F608u);
        store<u8>(p + 12, static_cast<u8>(a != 0 ? 1 : 0));
    }
    const u32 P2 = Pool28332(ctx);
    const u32 q = static_cast<u32>(VCall(ctx, P2, 8, {P2, 36}));
    store<u16>(q + 4, 36);
    if (q != 0) call(ctx, 0x007DFEF8u, {q, p, Sx39(b)});
    store<u32>(o + 8, q);
    call(ctx, 0x007D4EA8u, {p});
}

// u32 f(T* o, bool on) (no direct caller): x->vt[10](x, on, 1) for x =
// o->+24 (when set), then for the slot i = u16 o->+356 of the set s = o->+8
// (16-byte slots s->+148, count s->+152; the slot's elements +4, count +8,
// 16 bytes each) every non-null element pointer p (+4) gets
// p->vt[22](p, on) (OPD / entry after the arguments; s->+148 and i
// re-read after each). r3 = i.
DECOMP(00370E30, Obj_SetSlotOn_00370E30) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 on = arg<u32>(ctx, 1) & 0xFFu;
    const u32 x = load<u32>(o + 24);
    if (x != 0) VCall(ctx, x, 40, {x, on, 1});
    const u32 s = load<u32>(o + 8);
    u32 i = load<u16>(o + 356);
    ctx.gpr[3] = i;
    if (!(i < load<u32>(s + 152))) return;
    u32 t = load<u32>(s + 148);
    if (load<u32>(t + ((i << 4) & 0xFFFF0u) + 8) == 0) return;
    u32 j = 0;
    do {
        const u32 p = load<u32>(load<u32>(t + ((i << 4) & 0xFFFF0u) + 4) + 16 * j + 4);
        if (p != 0) {
            VCall(ctx, p, 88, {p, on});
            t = load<u32>(s + 148);
            i = load<u16>(o + 356);
            ctx.gpr[3] = i;
        }
        ++j;
    } while (j < load<u32>(t + ((i << 4) & 0xFFFF0u) + 8));
}

// void f(T* o) (the body of Delete_00381460): vtable 0x00B1F988,
// World_ListRemove_0046B7A4(*0x00F65CA0, 8, 0x00F37878, o, 0); x->vt[10](x,
// 0, 1) for x = o->+24 (when set); each non-null p = (o->+328)[j].+4 (16
// bytes each; j below (o->+8)->+152, re-read after each call) gets
// p->vt[22](p, 0) (OPD / entry after the arguments);
// Effect_SetHash4CF988(o->+20, 0x90C77BBD) when o->+20 is set; then
// 0x0036ABF8(o) (r3 = its).
DECOMP(00381320, Obj_Dtor_00381320) {
    const u32 o = arg<u32>(ctx, 0);
    store<u32>(o, 0x00B1F988u);
    call(ctx, 0x0046B7A4u, {load<u32>(0x00F65CA0u), 8, 0x00F37878u, o, 0});
    const u32 x = load<u32>(o + 24);
    if (x != 0) VCall(ctx, x, 40, {x, 0, 1});
    const u32 s = load<u32>(o + 8);
    u32 n = load<u32>(s + 152);
    for (u32 j = 0; j < n; ++j) {
        const u32 p = load<u32>(load<u32>(o + 328) + 16 * j + 4);
        if (p == 0) continue;
        VCall(ctx, p, 88, {p, 0});
        n = load<u32>(s + 152);
    }
    const u32 e = load<u32>(o + 20);
    if (e != 0) call(ctx, 0x004CF988u, {e, 0x90C77BBDu});
    call(ctx, 0x0036ABF8u, {o});
}

// u32 f(T* o): empties the tree o + 148 (count +152): repeatedly, with the
// stack o + 164 reset (u16 o->+244 = 0), the leftmost node from the root
// *o->+160 (following +12, the visited nodes pushed on the stack) becomes
// o->+248, and its key +4 is removed (Tree_FindU32_0077C6F8(o + 148, key,
// &res, o->+148) != 0 -> Avl_Remove_0077BB08(o + 148, o + 148, key,
// &flag), o->+152 - 1); ends with o->+248 = 0 when the root is empty. r3
// = 0.
DECOMP(003DDA78, Tree_Clear_003DDA78) {
    const u32 o = arg<u32>(ctx, 0);
    Stack fr(ctx, 16);  // the result word (sp+112) and the flag byte (sp+116)
    for (;;) {
        const u32 root = load<u32>(o + 160);
        store<u16>(o + 244, 0);
        u32 n = load<u32>(root);
        if (n == 0) {
            store<u32>(o + 248, 0);
            ret(ctx, 0u);
            return;
        }
        for (u32 c; (c = load<u32>(n + 12)) != 0; n = c) {
            const s32 k = static_cast<s16>(load<u16>(o + 244));
            store<u16>(o + 244, static_cast<u16>(k + 1));
            store<u32>(o + 164 + 4 * static_cast<u32>(k), n);
        }
        const u32 key = load<u32>(n + 4);
        store<u32>(o + 248, n);
        store<u8>(fr.addr() + 4, 0);
        if (static_cast<u32>(call(ctx, 0x0077C6F8u, {o + 148, key, fr.addr(), load<u32>(o + 148)})) == 0) continue;
        call(ctx, 0x0077BB08u, {o + 148, o + 148, key, fr.addr() + 4});
        store<u32>(o + 152, load<u32>(o + 152) - 1);
    }
}

// f32 f(u32 id): with audio on (u8 0x010AEC60), the voice id (index id >>
// 16 below the s32 0x010B0218, serial id & 0xFFFF != 0 matching bits
// 15..30 of the 272-byte entry's word +256, bit 0x80 of its byte +256
// clear) of the table 0x010B021C: the first of the Voice_GetValue(+16)
// sub-items j (0x006D6514(+16, j)) for which 0x006F9CA8(item, &v, &w) is
// not 0 gives f1 = v (r3 = that result); otherwise f1 = 0.0 and r3 =
// 0x0044C880 (its address).
DECOMP(0044C888, Voice_GetParam_0044C888) {
    const u32 id = arg<u32>(ctx, 0);
    constexpr u32 K = 0x0044C880u;
    auto fail = [&]() {
        ctx.gpr[3] = K;
        ctx.fpr[1] = Lfs32(K);
    };
    if (load<u8>(0x010AEC60u) == 0) return fail();
    const u32 lo = id & 0xFFFFu, hi = id >> 16;
    constexpr u32 T = 0x010B0200u;
    if (lo == 0) return fail();
    if (!(static_cast<s32>(hi) < load<s32>(T + 24))) return fail();
    const u32 e = load<u32>(T + 28) + 272 * hi;
    if (((load<u32>(e + 256) >> 15) & 0xFFFFu) != lo || (load<u8>(e + 256) & 0x80u) != 0) return fail();
    const u32 v = e + 16;
    const u32 n = static_cast<u32>(call(ctx, 0x006D64ACu, {load<u32>(v)}));
    if (n == 0) return fail();
    Stack fr(ctx, 16);  // v (sp+112) and w (sp+116)
    for (u32 j = 0; j < n; ++j) {
        const u32 it = static_cast<u32>(call(ctx, 0x006D6514u, {load<u32>(v), static_cast<u64>(static_cast<s64>(static_cast<s16>(j)))}));
        if (static_cast<u32>(call(ctx, 0x006F9CA8u, {it, fr.addr(), fr.addr() + 4})) != 0) {
            ctx.fpr[1] = Lfs32(fr.addr());
            return;
        }
    }
    fail();
}

// T* f(T* o): o->+8 = o->+16 = 0, u8 o->+28 = 1; then for each node n of
// the list o->+32 (next +72): when the byte table n->+48->+12 has entry
// (*0x010AB1B8)->+64 (4-byte stride) set, the n->+24 elements e of
// n->+20 (n->+24 cleared first) with bit 0x00040000 of e->+172 decrement
// n->+32 and get e->vt[10](e, 1); then n->vt[3](n) (OPD / entry after the
// arguments). r3 = o (empty list) or the last call's.
DECOMP(0045488C, Obj_ResetList_0045488C) {
    const u32 o = arg<u32>(ctx, 0);
    u32 n = load<u32>(o + 32);
    store<u32>(o + 8, 0);
    store<u32>(o + 16, 0);
    store<u8>(o + 28, 1);
    while (n != 0) {
        const u32 idx = load<u32>(load<u32>(0x010AB1A8u + 16) + 64);
        if (load<u8>(load<u32>(load<u32>(n + 48) + 12) + (idx << 2)) != 0) {
            const s32 cnt = load<s32>(n + 24);
            store<u32>(n + 24, 0);
            for (s32 j = 0; j < cnt; ++j) {
                const u32 e = load<u32>(load<u32>(n + 20) + 4 * static_cast<u32>(j));
                if ((load<u32>(e + 172) & 0x00040000u) == 0) continue;
                store<u32>(n + 32, load<u32>(n + 32) - 1);
                VCall(ctx, e, 40, {e, 1});
            }
        }
        VCall(ctx, n, 12, {n});
        n = load<u32>(n + 72);
    }
}

// u32 f(XmlCursor* x, Vec* v): for each child c of x (XmlCursor_
// FirstChildAny / NextSiblingAny), a zeroed 74-byte query record read by
// QueryVar_Read49EC08(c, &rec) != 0 is appended to v (76-byte stride, data
// +0, u16 capacity +4, count +8, peak +10; memmove of 74 bytes) while
// count < capacity, stopping at the first full one (r3 = the count; the
// grow path after that test cannot run). r3 = 0 at the end, or the
// argument when one is null.
DECOMP(0049EF40, XmlQuery_ReadAll_0049EF40) {
    u32 x = arg<u32>(ctx, 0);
    const u32 v = arg<u32>(ctx, 1);
    if (x == 0 || v == 0) return;
    x = static_cast<u32>(call(ctx, 0x004F9140u, {x}));
    if (x == 0) return;
    Stack fr(ctx, 80);  // the record (sp+112..185)
    const u32 rec = fr.addr();
    do {
        store<u32>(rec + 68, 0);
        store<u8>(rec + 72, 0);
        store<u8>(rec + 73, 0);
        call(ctx, 0x00726E54u, {rec, 0, 68});
        if (static_cast<u32>(call(ctx, 0x0049EC08u, {x, rec})) != 0) {
            u32 n = load<u16>(v + 8);
            const u32 cap = load<u16>(v + 4);
            if (!(n < cap)) {
                ret(ctx, n);
                return;
            }
            if (n == cap) {  // unreachable after the test above
                call(ctx, 0x00551718u, {v, (cap << 1) & 0xFFFFFFFEu});
                n = load<u16>(v + 8);
            }
            call(ctx, 0x0071C3F0u, {load<u32>(v) + 76 * n, rec, 74});
            const u32 n2 = load<u16>(v + 8) + 1u;
            const u32 peak = load<u16>(v + 10);
            store<u16>(v + 8, static_cast<u16>(n2));
            store<u16>(v + 10, static_cast<u16>(static_cast<s32>(peak) > static_cast<s32>(n2 & 0xFFFFu) ? peak : n2));
        }
        x = static_cast<u32>(call(ctx, 0x004F8D74u, {x}));
    } while (x != 0);
}

// u32 f(name) (no direct caller): the global 0x00F65FF0 is cleared and
// Iff_ReadChunks(name, 0, 0) may set it to a file; that file is loaded
// whole (File_LoadWhole(f, &size)), opened as an archive z =
// Archive_OpenZip(data, size, f, f), and its XML document (XmlDoc_Init /
// XmlDoc_LoadXmlb(&doc, z, 0) on the stack) is added to the set
// Set_Find(0) (Set_AddXml(set, z + 4, &doc, 0)) when it loads; then
// XmlDoc_Destruct, z->vt[2](z) (OPD / entry after the argument), the global
// cleared again and Mem_Free(data). r3 = 1.
DECOMP(004BE61C, Set_LoadArchive_004BE61C) {
    const u32 name = arg<u32>(ctx, 0);
    constexpr u32 G = 0x00F65FF0u;
    store<u32>(G, 0);
    call(ctx, 0x00569388u, {name, 0, 0});
    const u32 f = load<u32>(G);
    if (f != 0) {
        Stack fr(ctx, 2288);  // size at sp+112, the document at sp+116 (frame of 2400 bytes)
        store<u32>(fr.addr(), 0);
        const u32 d = static_cast<u32>(call(ctx, 0x00560B90u, {f, fr.addr()}));
        if (d != 0) {
            const u32 g = load<u32>(G);
            const u32 z = static_cast<u32>(call(ctx, 0x00567034u, {d, load<u32>(fr.addr()), g, g}));
            const u32 set = static_cast<u32>(call(ctx, 0x004BCE90u, {0}));
            const u32 doc = fr.addr() + 4;
            call(ctx, 0x004FA7A4u, {doc});
            if (static_cast<u32>(call(ctx, 0x004FACCCu, {doc, z, 0})) != 0) call(ctx, 0x004BDA70u, {set, z + 4, doc, 0});
            call(ctx, 0x004FA9C0u, {doc});
            VCall(ctx, z, 8, {z});
            store<u32>(G, 0);
            call(ctx, 0x005611B0u, {d});
        }
    }
    ret(ctx, 1u);
}

// u32 f(XmlCursor* x, const u8* arg): the handler h = *SortedMap_Find7332B4(
// (*0x00F668D8) + 32, &hash) of the "name" attribute's hash (h = 0 for no
// name; h->+16->+88 read anyway); when its callback OPD cb =
// h->+16->+88 is set, every "association" child c with a "value"
// attribute v gets cb(h, v, u8 *arg) (entry after the arguments). r3 = 1.
DECOMP(004F6D94, Xml_ApplyAssociations_004F6D94) {
    const u32 x = arg<u32>(ctx, 0), p = arg<u32>(ctx, 1);
    const u32 a = static_cast<u32>(call(ctx, 0x004F9628u, {x, 0x00AF0C74u}));
    const u32 hash = static_cast<u32>(call(ctx, 0x0056A710u, {a}));
    u32 h = 0;
    const u32 g = load<u32>(0x00F668D8u);
    Stack fr(ctx, 16);  // the key (sp+112)
    store<u32>(fr.addr(), hash);
    if (hash != 0) h = load<u32>(static_cast<u32>(call(ctx, 0x007332B4u, {g + 32, fr.addr()})));
    const u32 cb = load<u32>(load<u32>(h + 16) + 88);
    if (cb != 0) {
        for (u32 c = static_cast<u32>(call(ctx, 0x004F936Cu, {x, 0x00AF0D08u})); c != 0;
             c = static_cast<u32>(call(ctx, 0x004F9078u, {c, 0x00AF0D08u}))) {
            const u32 v = static_cast<u32>(call(ctx, 0x004F9628u, {c, 0x00AF0D18u}));
            if (v != 0) call_ptr(ctx, cb, {h, v, load<u8>(p), load<u32>(cb)});
        }
    }
    ret(ctx, 1u);
}

// void f(T* o) (no direct caller; a deleting destructor): for o != 0:
// vtable 0x00B11E40, the vectors o + 496, o + 348 / 300 / 252, o + 232 /
// 212 / 192, o + 148 and o + 128 each PtrVector_Clear + Nop, then
// SmallFree(o, 720) (r3 = its).
DECOMP(00743D14, Obj_Delete720_00743D14) {
    const u32 o = arg<u32>(ctx, 0);
    if (o == 0) {
        ret(ctx, 0u);
        return;
    }
    store<u32>(o, 0x00B11E40u);
    for (u32 v : {496u, 348u, 300u, 252u, 232u, 212u, 192u, 148u, 128u}) {
        call(ctx, 0x0054EC1Cu, {o + v});
        call(ctx, 0x0054EC18u, {o + v});
    }
    call(ctx, 0x00552420u, {o, 720});
}

// void f(T* o) (no direct caller; a destructor): vtable 0x00B11E50, the
// vectors o + 560, o + 412 / 364 / 316, o + 296 / 276 / 256, o + 212 and o +
// 192 each PtrVector_Clear + Nop, then vtable 0x00B11D98 and
// Obj_BaseDtor_000C2DA0(o) (r3 = its).
DECOMP(00743E54, Obj_Dtor_00743E54) {
    const u32 o = arg<u32>(ctx, 0);
    store<u32>(o, 0x00B11E50u);
    for (u32 v : {560u, 412u, 364u, 316u, 296u, 276u, 256u, 212u, 192u}) {
        call(ctx, 0x0054EC1Cu, {o + v});
        call(ctx, 0x0054EC18u, {o + v});
    }
    store<u32>(o, 0x00B11D98u);
    call(ctx, 0x000C2DA0u, {o});
}

// u32 f(T* o): each element e of the pointer vector o->+20 (u16 count
// o->+28; e re-read before each call) gets e->vt[24](e), e->vt[21](e) and,
// when not null, e->vt[1](e) (OPD / entry after the argument); then the
// u16 counts +28 and +8 are cleared and the vectors o + 20 and o each get
// PtrVector_Clear + Nop (r3 = its).
DECOMP(0077D61C, Set_ReleaseAll_0077D61C) {
    const u32 o = arg<u32>(ctx, 0);
    for (u32 i = 0; i < load<u16>(o + 28); ++i) {
        u32 e = load<u32>(load<u32>(o + 20) + 4 * i);
        VCall(ctx, e, 96, {e});
        e = load<u32>(load<u32>(o + 20) + 4 * i);
        VCall(ctx, e, 84, {e});
        e = load<u32>(load<u32>(o + 20) + 4 * i);
        if (e != 0) VCall(ctx, e, 4, {e});
    }
    store<u16>(o + 28, 0);
    store<u16>(o + 8, 0);
    call(ctx, 0x0054EC1Cu, {o + 20});
    call(ctx, 0x0054EC18u, {o + 20});
    call(ctx, 0x0054EC1Cu, {o});
    call(ctx, 0x0054EC18u, {o});
}

// u32 f(T* o; f32 dt) (from World_Update): with the world's object w =
// (*0x00F65CA0)->+0x128168 and the u8 o->+0x128330 set, the fade value
// v = o->+0x128324 += o->+0x12832C * dt (fmadds); at or below 0.0 it
// becomes 0 and at or above 1.0 it becomes 1.0, either clearing the u8;
// then t = (w->+656)->+32 gets t->vt[2](t, v > 0.0) (OPD / entry after
// the arguments) and t->+116 = v (r3 = 0x00128324, f1 = v). Inactive: r3
// = w.
DECOMP(007872A8, World_StepFade_007872A8) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 w = load<u32>(load<u32>(0x00F65CA0u) + 0x00128168u);
    ctx.gpr[3] = w;
    if (w == 0 || load<u8>(o + 0x00128330u) == 0) return;
    const u32 A = o + 0x00128324u;
    constexpr u32 K = 0x007872A0u;  // {0.0f, 1.0f}
    const f64 v = F39(std::fma(Lfs32(o + 0x0012832Cu), farg(ctx, 0), Lfs32(A)));
    Stfs32(A, v);
    u32 on = 1;
    if (v < Lfs32(K) || v == Lfs32(K)) {
        store<u32>(A, 0);
        store<u8>(o + 0x00128330u, 0);
        on = 0;
    } else if (v > Lfs32(K + 4) || v == Lfs32(K + 4)) {
        store<u32>(A, 0x3F800000u);
        store<u8>(o + 0x00128330u, 0);
    }
    const u32 t = load<u32>(load<u32>(w + 656) + 32);
    VCall(ctx, t, 8, {t, on});
    store<u32>(t + 116, load<u32>(A));
    ctx.fpr[1] = Lfs32(A);
    ctx.gpr[3] = 0x00128324u;  // the offset of v, left in r3
}

// ---------------------------------------------------------------- sweep: 81 instructions
// u32 f(T* o): for each of the u16 o->+376 48-byte records r of o->+368
// with r->+20 or r->+16 set: r->+24 (when set) gets vt[10](r->+24, 1) (OPD
// / entry after the arguments) and is cleared; then, when r->+20 is 0,
// r->+32 is appended to the pointer vector o + 348 (u16 capacity +352,
// count +356, peak +358; PtrVector_Reserve(o + 348, 2 * capacity) when
// full). The record count is cleared at the end. r3 = the count.
DECOMP(0004038C, Recs_Flush_0004038C) {
    const u32 o = arg<u32>(ctx, 0);
    u32 i = 0;
    for (;;) {
        const u32 cnt = load<u16>(o + 376);
        if (!(i < cnt)) {
            store<u16>(o + 376, 0);
            ret(ctx, cnt);
            return;
        }
        const u32 r = load<u32>(o + 368) + 48 * i;
        u32 a = load<u32>(r + 20);
        if (a != 0 || load<u32>(r + 16) != 0) {
            const u32 b = load<u32>(r + 24);
            if (b != 0) {
                VCall(ctx, b, 40, {b, 1});
                store<u32>(r + 24, 0);
                a = load<u32>(r + 20);
            }
            if (a == 0) {
                u32 n = load<u16>(o + 356);
                const u32 cap = load<u16>(o + 352);
                if (n == cap) {
                    call(ctx, 0x00551718u, {o + 348, (cap << 1) & 0x1FFFEu});
                    n = load<u16>(o + 356);
                }
                store<u32>(load<u32>(o + 348) + ((n << 2) & 0x3FFFCu), load<u32>(r + 32));
                const u32 n2 = load<u16>(o + 356) + 1u;
                const u32 peak = load<u16>(o + 358);
                store<u16>(o + 356, static_cast<u16>(n2));
                store<u16>(o + 358, static_cast<u16>(static_cast<s32>(peak) > static_cast<s32>(n2 & 0xFFFFu) ? peak : n2));
            }
        }
        ++i;
    }
}

// T* f(?, XmlCursor* x) (no direct caller): a 32-byte node (Mem_Alloc(32,
// 0, 16); 0 on failure) set up by Obj_InitVt_B21C98(p, x), +12 = 0, vtable
// 0x00B0F798, f32 +16 / +20 / +24 = -1.0, +28 = 0; hash 0x8A93973F -> +12
// (default 0); unless that read returns non-zero, f32s 0x00210FAA -> +20
// and 0x00000A7B -> +24 (default -1.0) and 0xB7EA3D94 -> +28 (default
// 0.0). r3 = p.
DECOMP(00077388, XmlNew_Node32_00077388) {
    const u32 x = arg<u32>(ctx, 1);
    const u32 p = static_cast<u32>(call(ctx, 0x0054ED38u, {32, 0, 16}));
    if (p == 0) {
        ret(ctx, 0u);
        return;
    }
    call(ctx, 0x004BB3B0u, {p, x});
    constexpr u32 K = 0x0007737Cu;  // {-1.0f, 0.0f}
    store<u32>(p + 12, 0);
    store<u32>(p, 0x00B0F798u);
    store<u32>(p + 16, 0xBF800000u);
    store<u32>(p + 20, 0xBF800000u);
    store<u32>(p + 24, 0xBF800000u);
    store<u32>(p + 28, 0);
    if (static_cast<u32>(call(ctx, 0x004FA14Cu, {x, 0x8A93973Fu, p + 12, 0})) == 0) {
        callf(ctx, 0x004F9B20u, {x, 0x00210FAAu, p + 20}, {Lfs32(K)});
        callf(ctx, 0x004F9B20u, {x, 0x00000A7Bu, p + 24}, {Lfs32(K)});
        callf(ctx, 0x004F9B20u, {x, 0xB7EA3D94u, p + 28}, {Lfs32(K + 4)});
    }
    ret(ctx, p);
}

// T* f(?, XmlCursor* x) (no direct caller): a 24-byte node (Mem_Alloc(24,
// 0, 16); 0 on failure) set up by Obj_InitVt_B21C98(p, x), +12 / +16 = 0,
// u8 +20 = 9, vtable 0x00B0FAE0, u8 +21 = 1; hashes 0x7A30D7E3 -> +12
// (default 0) and 0x0F1D568D -> +16, the s32 0x368328E5 (default 9) -> u8
// +20 when in [0, 9) (else 9), bool 0x7A281983 -> +21. r3 = p.
DECOMP(0007CF54, XmlNew_Node24b_0007CF54) {
    const u32 x = arg<u32>(ctx, 1);
    const u32 p = static_cast<u32>(call(ctx, 0x0054ED38u, {24, 0, 16}));
    if (p == 0) {
        ret(ctx, 0u);
        return;
    }
    call(ctx, 0x004BB3B0u, {p, x});
    store<u32>(p + 12, 0);
    store<u32>(p + 16, 0);
    store<u8>(p + 20, 9);
    store<u32>(p, 0x00B0FAE0u);
    store<u8>(p + 21, 1);
    call(ctx, 0x004FA14Cu, {x, 0x7A30D7E3u, p + 12, 0});
    call(ctx, 0x004FA14Cu, {x, 0x0F1D568Du, p + 16, load<u32>(p + 16)});
    Stack fr(ctx, 16);  // the s32 (sp+112)
    store<u32>(fr.addr(), 9);
    call(ctx, 0x004F99BCu, {x, 0x368328E5u, fr.addr(), 9});
    const s32 v = load<s32>(fr.addr());
    const u8 b = load<u8>(p + 21);
    store<u8>(p + 20, static_cast<u8>(v >= 0 && v < 9 ? static_cast<u32>(v) : 9u));
    call(ctx, 0x004F986Cu, {x, 0x7A281983u, p + 21, b});
    ret(ctx, p);
}

// T* f(?, XmlCursor* x) (no direct caller): XmlNode28_39 with vtable
// 0x00B10580.
DECOMP(0008B21C, XmlNew_Node28_0008B21C) { XmlNode28_39(ctx, 0x00B10580u); }

// u32 f(T* o, Effect* x) (no direct caller): the target y = the owner of
// the component c = Effect_FindComponent(x, o->+12) when c->vt[8](c) == 5
// (OPD / entry after the argument) and c->+28 has type 0x02235C33 at +276
// (its +280), else Obj_FindById(o->+16) (when set). Unless y has bit 0x40
// of +192 and bit 0x00040000 of +172 (r3 = 1), the name of o->+12 (or
// else o->+16) is looked up (Hash_ToString; a stripped warning; r3 =
// its, or 0 when both are 0).
DECOMP(0008B624, EffectCmd_CheckTarget_0008B624) {
    const u32 o = arg<u32>(ctx, 0), x = arg<u32>(ctx, 1);
    u32 y = 0;
    bool fb = true;
    const u32 h = load<u32>(o + 12);
    if (h != 0) {
        const u32 c = static_cast<u32>(call(ctx, 0x004BB4B4u, {x, h}));
        if (c != 0 && static_cast<u32>(VCall(ctx, c, 32, {c})) == 5) {
            const u32 d = load<u32>(c + 28);
            if (d != 0) {
                if (load<u32>(d + 276) == 0x02235C33u) y = load<u32>(d + 280);
                fb = y == 0;
            }
        }
    }
    if (fb) {
        const u32 id = load<u32>(o + 16);
        if (id != 0) y = static_cast<u32>(call(ctx, 0x000D6A28u, {id}));
    }
    if (y != 0 && (load<u32>(y + 192) & 0x40u) != 0 && (load<u32>(y + 172) & 0x00040000u) != 0) {
        ret(ctx, 1u);
        return;
    }
    const u32 h2 = load<u32>(o + 12);
    if (h2 != 0) {
        call(ctx, 0x0056A6E0u, {h2});
        return;
    }
    const u32 id2 = load<u32>(o + 16);
    ctx.gpr[3] = id2;
    if (id2 != 0) call(ctx, 0x0056A6E0u, {id2});
}

// T* f(?, XmlCursor* x) (no direct caller): XmlNode28_39 with vtable
// 0x00B10710.
DECOMP(0008C9A4, XmlNew_Node28b_0008C9A4) { XmlNode28_39(ctx, 0x00B10710u); }

// T* f(?, XmlCursor* x) (no direct caller): a 32-byte node (Mem_Alloc(32,
// 0, 16); 0 on failure) set up by Obj_InitVt_B21C98(p, x), +12 / +16 /
// +20 = 0, vtable 0x00B109E0, +24 = 0, u8 +28 / +29 = 0; hashes
// 0x8A93973F -> +12 (default 0) and 0x0C0E22B5 -> +16, f32s 0x95C2F4F2 ->
// +20 and 0x9777CD91 -> +24 (each read setting u8 +28 / +29 = 1 unless it
// returns 1). r3 = p.
DECOMP(000921C8, XmlNew_Node32b_000921C8) {
    const u32 x = arg<u32>(ctx, 1);
    const u32 p = static_cast<u32>(call(ctx, 0x0054ED38u, {32, 0, 16}));
    if (p == 0) {
        ret(ctx, 0u);
        return;
    }
    call(ctx, 0x004BB3B0u, {p, x});
    store<u32>(p + 12, 0);
    store<u32>(p + 16, 0);
    store<u32>(p + 20, 0);
    store<u32>(p, 0x00B109E0u);
    store<u32>(p + 24, 0);
    store<u8>(p + 28, 0);
    store<u8>(p + 29, 0);
    call(ctx, 0x004FA14Cu, {x, 0x8A93973Fu, p + 12, 0});
    call(ctx, 0x004FA14Cu, {x, 0x0C0E22B5u, p + 16, load<u32>(p + 16)});
    callf(ctx, 0x004F9B20u, {x, 0x95C2F4F2u, p + 20}, {Lfs32(p + 20)});
    if (static_cast<u32>(ctx.gpr[3]) != 1) store<u8>(p + 28, 1);
    callf(ctx, 0x004F9B20u, {x, 0x9777CD91u, p + 24}, {Lfs32(p + 24)});
    if (static_cast<u32>(ctx.gpr[3]) != 1) store<u8>(p + 29, 1);
    ret(ctx, p);
}

// T* f(?, XmlCursor* x) (no direct caller): a 28-byte node (Mem_Alloc(28,
// 0, 16); 0 on failure) set up by Obj_InitVt_B21C98(p, x), +12 / +16 / +20
// = 0, vtable 0x00B112F8, u8 +24 = 0, u8 +25 = 1; hashes 0x7A30D7E3 -> +12
// (default 0), 0x8D47DA8F -> +16, 0x001EBA11 -> +20, bools 0xDFD4E345 ->
// +24, 0x8488565B -> +25. r3 = p.
DECOMP(0009E7E4, XmlNew_Node28c_0009E7E4) {
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
    store<u32>(p, 0x00B112F8u);
    store<u8>(p + 24, 0);
    store<u8>(p + 25, 1);
    call(ctx, 0x004FA14Cu, {x, 0x7A30D7E3u, p + 12, 0});
    call(ctx, 0x004FA14Cu, {x, 0x8D47DA8Fu, p + 16, load<u32>(p + 16)});
    call(ctx, 0x004FA14Cu, {x, 0x001EBA11u, p + 20, load<u32>(p + 20)});
    call(ctx, 0x004F986Cu, {x, 0xDFD4E345u, p + 24, load<u8>(p + 24)});
    call(ctx, 0x004F986Cu, {x, 0x8488565Bu, p + 25, load<u8>(p + 25)});
    ret(ctx, p);
}

// u32 f(Obj* o, u32 a, bool b): when b is set, the child c (+192, when
// set) of each 240-byte part of o->+340 (u16 count +8, array +0) gets
// c->vt[32](c, a, b); then every element e of the pointer vector o->+248
// (u16 count +8, array +0) gets e->vt[15](e, a, b) (OPD / entry after the
// arguments). r3 = o->+248.
DECOMP(000DA684, Obj_BroadcastEffect_000DA684) {
    const u32 o = arg<u32>(ctx, 0), a = arg<u32>(ctx, 1);
    const u32 b = arg<u32>(ctx, 2) & 0xFFu;
    if (b != 0) {
        for (u32 i = 0;; ++i) {
            const u32 v = load<u32>(o + 340);
            const u32 n = v != 0 ? load<u16>(v + 8) : 0u;
            if (!(i < n)) break;
            const u32 c = load<u32>(load<u32>(v) + 240 * i + 192);
            if (c != 0) VCall(ctx, c, 128, {c, a, b});
        }
    }
    for (u32 i = 0;; ++i) {
        const u32 v = load<u32>(o + 248);
        ctx.gpr[3] = v;
        const u32 n = v != 0 ? load<u16>(v + 8) : 0u;
        if (!(i < n)) break;
        const u32 e = load<u32>(load<u32>(v) + 4 * i);
        VCall(ctx, e, 60, {e, a, b});
    }
}

// u32 f(Obj* o) (no direct caller): Obj_SyncBodyShapes(o) == 0 -> r3 = 0;
// otherwise, with o->+216 set and the state s = o->vt[8](o, 0) not 5, the
// u8 o->+569 is refreshed by 0x001D48B8(o, o + 569); when it turned on
// (was 0 or s == 1, now set) o->vt[105](o, 2, 0), when it turned off (was
// set and s != 1, now 0) o->vt[105](o, 6, 0) (OPD / entry after the
// arguments; the vtable read after the refresh). r3 = 1.
DECOMP(001D4A28, Obj_RefreshActive_001D4A28) {
    const u32 o = arg<u32>(ctx, 0);
    if (static_cast<u32>(call(ctx, 0x000E5000u, {o})) == 0) {
        ret(ctx, 0u);
        return;
    }
    if (load<u32>(o + 216) != 0) {
        const u32 s = static_cast<u32>(VCall(ctx, o, 32, {o, 0}));
        if (s != 5) {
            const bool was = load<u8>(o + 569) != 0 && s != 1;
            call(ctx, 0x001D48B8u, {o, o + 569});
            const bool now = load<u8>(o + 569) != 0;
            const u32 vt = load<u32>(o);
            if (!was && now) VCallVt(ctx, vt, 420, {o, 2, 0});
            else if (was && !now) VCallVt(ctx, vt, 420, {o, 6, 0});
        }
    }
    ret(ctx, 1u);
}

// u32 f(Obj* o) (from f_00243870): with bit 0x80000000 of o->+384 and the
// partner q = o->+608 set: bit 0x40000000 of q->+1268 = bit 0x200 of the
// team record +8 (0x010B1A90 + 11968 * u8 (world slot (s8) o->+172)->+1)
// and Obj_SetTarget_00296758(q, the first member other than o of the
// group CallK2_000D93D4(o) (Target_Next271C08(g, 0, prev)), or 0, 0). r3
// = 0 or its.
DECOMP(00243728, Obj_PickPartnerTarget_00243728) {
    const u32 o = arg<u32>(ctx, 0);
    ctx.gpr[3] = 0;
    if ((load<u32>(o + 384) & 0x80000000u) == 0) return;
    const s32 s = static_cast<s8>(load<u8>(o + 172));
    const u32 e = load<u32>(0x00F65CA0u) + static_cast<u32>(s) * 0x00020C80u + 4208;
    const u32 t = 0x010B19E0u + static_cast<u32>(load<u8>(e + 1)) * 11968u + 176;
    const u32 bit = (load<u32>(t + 8) & 0x200u) != 0 ? 1u : 0u;
    if (load<u32>(o + 608) == 0) return;
    u32 tgt = 0;
    const u32 g = static_cast<u32>(call(ctx, 0x000D93D4u, {o}));
    if (g != 0) {
        for (u32 x = static_cast<u32>(call(ctx, 0x00271C08u, {g, 0, 0})); x != 0;
             x = static_cast<u32>(call(ctx, 0x00271C08u, {g, 0, x}))) {
            if (x != o) {
                tgt = x;
                break;
            }
        }
    }
    const u32 q = load<u32>(o + 608);
    store<u32>(q + 1268, (load<u32>(q + 1268) & ~0x40000000u) | (bit << 30));
    call(ctx, 0x00296758u, {load<u32>(o + 608), tgt, 0});
}

// void f(T* o) (no direct caller; a destructor): vtable 0x00B1C0D0,
// List1013B5C_Remove(o), x->vt[2](x) for x = o->+92 (OPD / entry after the
// argument; then o->+92 = 0), vtable 0x00B1BE30, Unlist1056F1C39(o) and
// List1013B5C_Remove(o) again (r3 = its).
DECOMP(0027BB20, Obj_Dtor_0027BB20) {
    const u32 o = arg<u32>(ctx, 0);
    store<u32>(o, 0x00B1C0D0u);
    call(ctx, 0x000D6940u, {o});
    const u32 x = load<u32>(o + 92);
    if (x != 0) {
        VCall(ctx, x, 8, {x});
        store<u32>(o + 92, 0);
    }
    store<u32>(o, 0x00B1BE30u);
    Unlist1056F1C39(o);
    call(ctx, 0x000D6940u, {o});
}

// u32 f(T* o, u32 text, u32 key, bool b): with the anim set o->+36 and the
// text entry t = Text_Lookup34A398(text): the track a =
// Anim_Find34CC34(o->+36, key) gets u8 t->+40 = 1, 0x003535C4(a, t, b,
// 0x00298268, -, 0; f1 = 1.0, f2 = 0.0) and Obj_SetF32Via_00354184(a; f1
// = f32 o->+380); without a track (r3 = 1) or set / entry (r3 = 0):
// Hash_ToString(text) and x->vt[19](x) for x = o->+8 (OPD / entry after
// the argument; a stripped warning). r3 = 1 on success.
DECOMP(00298270, Obj_PlayTextAnim_00298270) {
    const u32 o = arg<u32>(ctx, 0), text = arg<u32>(ctx, 1), key = arg<u32>(ctx, 2);
    const u32 b = arg<u32>(ctx, 3) & 0xFFu;
    u32 t = 0;
    if (load<u32>(o + 36) != 0) t = static_cast<u32>(call(ctx, 0x0034A398u, {text}));
    u32 r = 0;
    if (t != 0) {
        const u32 a = static_cast<u32>(call(ctx, 0x0034CC34u, {load<u32>(o + 36), key}));
        r = 1;
        if (a != 0) {
            constexpr u32 K = 0x00298268u;  // {1.0f, 0.0f}
            store<u8>(t + 40, 1);
            // r7 as Anim_Find34CC34 left it (unread by the callee)
            callf(ctx, 0x003535C4u, {a, t, b, K, ctx.gpr[7], 0}, {Lfs32(K), Lfs32(K + 4)});
            callf(ctx, 0x00354184u, {a}, {Lfs32(o + 380)});
            ret(ctx, 1u);
            return;
        }
    }
    call(ctx, 0x0056A6E0u, {text});
    const u32 x = load<u32>(o + 8);
    VCall(ctx, x, 76, {x});
    ret(ctx, r);
}

// u32 f(T* o) (from f_0029A978): the first entry p (u8 +18 == 4) of the
// pointer list (+80, u16 count +88) of world slot (s8) w->+172 (w = o->+8;
// none for w = 0 or slot -1) names a spline r = p->vt[17](p) (OPD / entry
// after the argument); unless o->+2180 already has +204 == r, o->+2180 =
// r ? *SplineMap_Find((*0x00F8F158)->+16, &r) : 0, and a found one gets
// WorldObj_UpdateLod(it, 0, 0). r3 as the original leaves it.
DECOMP(0029A828, Obj_BindSpline_0029A828) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 w = load<u32>(o + 8);
    if (w == 0) return;
    const s32 s = static_cast<s8>(load<u8>(w + 172));
    if (s == -1) return;
    const u32 base = load<u32>(0x00F65CA0u) + static_cast<u32>(s) * 0x00020C80u;
    ctx.gpr[3] = base;
    const u32 e = base + 4208;
    const u32 n = load<u16>(e + 88);
    if (n == 0) return;
    u32 p = 0;
    for (u32 i = 0;; ++i) {
        p = load<u32>(load<u32>(e + 80) + 4 * i);
        ctx.gpr[3] = p;
        if (p != 0 && load<u8>(p + 18) == 4) break;
        if (!(i + 1 < n)) return;
    }
    const u32 r = static_cast<u32>(VCall(ctx, p, 68, {p}));
    const u32 cur = load<u32>(o + 2180);
    if (cur != 0 && load<u32>(cur + 204) == r) return;
    if (r == 0) {
        store<u32>(o + 2180, 0);
        return;
    }
    Stack fr(ctx, 16);  // the key (sp+112)
    store<u32>(fr.addr(), r);
    const u32 q = load<u32>(static_cast<u32>(call(ctx, 0x007335C4u, {load<u32>(load<u32>(0x00F8F158u) + 16), fr.addr()})));
    store<u32>(o + 2180, q);
    ctx.gpr[3] = q;
    if (q != 0) call(ctx, 0x0057EE00u, {q, 0, 0});
}

// u32 f(T* o, bool kill) (from Obj_FreeEffect_002FA37C): the effect e =
// o->+8 gets u8 +70 = 0; with kill: Effect_Release(e->+52, 0x002237D4)
// when set, and when e is the slot j (0..2) of its owner w = e->+56
// (w->+900 + 4 j), the handles w->+916 + 8 j (only when w->+432 == 1) and
// w->+920 + 8 j get vt[32](h, 0x002237D4, 1) (OPD / entry after the
// arguments; each when set). r3 as the original leaves it.
DECOMP(002F978C, Effect_Stop_002F978C) {
    const u32 e = load<u32>(arg<u32>(ctx, 0) + 8);
    if (e == 0) return;
    const u32 kill = arg<u32>(ctx, 1) & 0xFFu;
    store<u8>(e + 70, 0);
    ctx.gpr[3] = 0;
    if (kill == 0) return;
    const u32 x = load<u32>(e + 52);
    ctx.gpr[3] = x;
    if (x != 0) call(ctx, 0x004CF3C4u, {x, 0x002237D4u});
    const u32 w = load<u32>(e + 56);
    if (w == 0) return;
    u32 j = 0;
    while (load<u32>(w + 900 + 4 * j) != e) {
        ++j;
        if (!(j < 3)) {
            ret(ctx, j);
            return;
        }
    }
    if (load<u32>(w + 432) == 1) {
        const u32 y = load<u32>(w + 916 + 8 * j);
        if (y != 0) VCall(ctx, y, 128, {y, 0x002237D4u, 1});
    }
    const u32 z = load<u32>(w + 920 + 8 * j);
    ctx.gpr[3] = w + 920 + 8 * j;
    if (z != 0) VCall(ctx, z, 128, {z, 0x002237D4u, 1});
}

// u32 f(Obj* o) (no direct caller): in state o->vt[8](o, 0) == 1, unless
// t = o->+32 has t->vt[15](t) set and Anim_TrackSet36_0029879C(
// t->vt[15](t), 0, 0) == 0 (r3 = 1): 0x0030D2B8(o->+32, u8 t->+665 ? 9 :
// 7) and o->vt[7](o, 0, 2) (OPD / entry after the arguments), r3 = 0.
// Other states: r3 = 1.
DECOMP(0031A834, Obj_EndAnimState_0031A834) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 flag = load<u32>(o + 32) + 664;
    if (static_cast<s32>(VCall(ctx, o, 32, {o, 0})) != 1) {
        ret(ctx, 1u);
        return;
    }
    const u32 t = load<u32>(o + 32);
    if (static_cast<u32>(VCall(ctx, t, 60, {t})) != 0) {
        const u32 y = static_cast<u32>(VCall(ctx, t, 60, {t}));
        if (static_cast<u32>(call(ctx, 0x0029879Cu, {y, 0, 0})) == 0) {
            ret(ctx, 1u);
            return;
        }
    }
    call(ctx, 0x0030D2B8u, {load<u32>(o + 32), load<u8>(flag + 1) != 0 ? 9u : 7u});
    VCall(ctx, o, 28, {o, 0, 2});
    ret(ctx, 0u);
}

// void f() (no direct caller; static destructors): the 13 tuning
// variables 0x0106B67C (vtable 0x00B0D934), 0x0106B630 (0x00B0D968) and
// 0x0106B5E0, 0x0106B594, ... 0x0106B2E4 (76 / 80 bytes apart, vtable
// 0x00B0D99C) each get their vtable and TuningVar_DestructBase_004FC1A0
// (r3 = the last one's).
DECOMP(00341694, TuningVars_Destruct_00341694) {
    store<u32>(0x0106B67Cu, 0x00B0D934u);
    call(ctx, 0x004FC1A0u, {0x0106B67Cu});
    store<u32>(0x0106B630u, 0x00B0D968u);
    call(ctx, 0x004FC1A0u, {0x0106B630u});
    for (u32 a : {0x0106B5E0u, 0x0106B594u, 0x0106B548u, 0x0106B4F8u, 0x0106B4ACu, 0x0106B460u, 0x0106B414u, 0x0106B3C8u,
                  0x0106B37Cu, 0x0106B330u, 0x0106B2E4u}) {
        store<u32>(a, 0x00B0D99Cu);
        call(ctx, 0x004FC1A0u, {a});
    }
}

// void f(T* a, Part* n) (recursive; from Obj_AddPartTimes_003ECA98): along
// the sibling chain n, n->+260, ...: stops at a part whose f32 +228 is
// already 0.1 (0x003EC94C) or with bit 0x40000000 of +268; otherwise +228
// = 0.1 and each of the seven children +232..+256 (when set) is visited
// recursively first. r3 / f1 as the original leaves them.
DECOMP(003EC950, Part_MarkTree_003EC950) {
    const u32 a = arg<u32>(ctx, 0);
    u32 n = arg<u32>(ctx, 1);
    const f64 k = Lfs32(0x003EC94Cu);
    for (;;) {
        ctx.fpr[1] = Lfs32(n + 228);
        if (ctx.fpr[1] == k) return;
        const u32 bit = (load<u32>(n + 268) >> 30) & 1u;
        ctx.gpr[3] = bit;
        if (bit != 0) return;
        store<u32>(n + 228, 0x3DCCCCCDu);
        for (u32 off = 232; off <= 256; off += 4) {
            const u32 c = load<u32>(n + off);
            ctx.gpr[3] = c;
            if (c != 0) call(ctx, 0x003EC950u, {a, c});
        }
        n = load<u32>(n + 260);
        if (n == 0) return;
    }
}

// Group* f(Obj* x) (from f_000A6B4C): 0 for x = 0; otherwise a 36-byte
// group g = SmallAlloc(36) (+0 = 0, pointer vector +4 constructed with u16
// +10 = 4 and PtrVector_Init(g + 4, 32, 0, 0), +32 = 0, +24 bits
// 0x80000000 / 0x20000000 cleared and 0x40000000 set) is appended to the
// vector 0x010AA694 (data +0, u16 capacity +4, count +8, peak +10;
// PtrVector_Reserve when full) even when null; then g->+28 = the current
// group *0x00F6585C (when different), g->+0 = x and g becomes current
// (the stores happen for a failed allocation too: a quirk). r3 = g.
DECOMP(0042BAE0, Group_Push_0042BAE0) {
    const u32 x = arg<u32>(ctx, 0);
    if (x == 0) {
        ret(ctx, 0u);
        return;
    }
    u32 g = static_cast<u32>(call(ctx, 0x0055238Cu, {36}));
    if (g != 0) {
        store<u32>(g, 0);
        call(ctx, 0x005516E0u, {g + 4});
        store<u16>(g + 10, 4);
        call(ctx, 0x00551598u, {g + 4, 32, 0, 0});
        const u32 w = load<u32>(g + 24);
        store<u32>(g + 32, 0);
        store<u32>(g + 24, ((w & 0x7FFFFFFFu) | 0x40000000u) & ~0x20000000u);
    }
    constexpr u32 V = 0x010AA694u;
    u32 n = load<u16>(V + 8);
    const u32 cap = load<u16>(V + 4);
    if (n == cap) {
        call(ctx, 0x00551718u, {V, (cap << 1) & 0x1FFFEu});
        n = load<u16>(V + 8);
    }
    const u32 d = load<u32>(V);
    const u32 n2 = load<u16>(V + 8) + 1u;
    const u32 peak = load<u16>(V + 10);
    store<u32>(d + ((n << 2) & 0x3FFFCu), g);
    store<u16>(V + 8, static_cast<u16>(n2));
    store<u16>(V + 10, static_cast<u16>(static_cast<s32>(peak) > static_cast<s32>(n2 & 0xFFFFu) ? peak : n2));
    const u32 cur = load<u32>(0x00F6585Cu);
    if (cur != g) store<u32>(g + 28, cur);
    store<u32>(g, x);
    store<u32>(0x00F6585Cu, g);
    ret(ctx, g);
}

// u32 f(Cmd* c) (no direct caller): x = Obj_FindByHandle42C2B0(c->+16, 0)
// is stored in c->+12; when x has bit 0x8 of +192: x->+380 = x->+392, y =
// x->vt[94](x) (when not 0, called again and y->vt[11](y, 0)), and for z =
// x->+380 (when set) the object z + 80 gets vt[7](z + 80, 0, 3) and z->+100
// gets +0 = c->+20, u8 +33 = u8 c->+28 (OPD / entry after the arguments;
// the first vtable read before the store). Always ends with
// Flag_Set8_40000000(c) (r3 = its).
DECOMP(0042FB5C, SeqCmd_Attach_0042FB5C) {
    const u32 c = arg<u32>(ctx, 0);
    const u32 x = static_cast<u32>(call(ctx, 0x0042C2B0u, {load<u32>(c + 16), 0}));
    store<u32>(c + 12, x);
    if (x != 0 && (load<u32>(x + 192) & 0x8u) != 0) {
        const u32 vt = load<u32>(x);
        store<u32>(x + 380, load<u32>(x + 392));
        if (static_cast<u32>(VCallVt(ctx, vt, 376, {x})) != 0) {
            const u32 y = static_cast<u32>(VCall(ctx, x, 376, {x}));
            VCall(ctx, y, 44, {y, 0});
        }
        const u32 z = load<u32>(x + 380);
        if (z != 0) {
            VCall(ctx, z + 80, 28, {z + 80, 0, 3});
            const u32 q = load<u32>(z + 100);
            const u32 w = load<u32>(c + 20);
            const u8 b = load<u8>(c + 28);
            store<u32>(q, w);
            store<u8>(q + 33, b);
        }
    }
    call(ctx, 0x0042CA60u, {c});
}

// u32 f(Cmd* c) (no direct caller): each 12-byte target record r of
// c->+140 (u16 count +148) gets r->+4 = Obj_FindByHandle42C2B0(r->+0, 0)
// (Hash_ToString(r->+0) when not found: a stripped warning); when
// FindFlagged_00368FA0(c->+160)->+20 is set and c->+168 has bit
// 0x40000000, Obj_SetIfKind2(it, 0xCE788358, *0x010C5A48); then
// 0x00489328(*0x00F65CA0, c + 140, c + 9, c + 74, c->+160, c->+164, bit
// 0x80000000 of c->+168) and Flag_Set8_40000000(c) (r3 = its).
DECOMP(004337D0, SeqCmd_BindTargets_004337D0) {
    const u32 c = arg<u32>(ctx, 0);
    for (u32 i = 0; i < load<u16>(c + 148); ++i) {
        const u32 r = load<u32>(c + 140) + 12 * i;
        const u32 x = static_cast<u32>(call(ctx, 0x0042C2B0u, {load<u32>(r), 0}));
        store<u32>(r + 4, x);
        if (x == 0) call(ctx, 0x0056A6E0u, {load<u32>(r)});
    }
    const u32 f = static_cast<u32>(call(ctx, 0x00368FA0u, {load<u32>(c + 160)}));
    if (f != 0) {
        const u32 t = load<u32>(f + 20);
        if (t != 0 && (load<u32>(c + 168) & 0x40000000u) != 0) call(ctx, 0x004A674Cu, {t, 0xCE788358u, load<u32>(0x010C5A48u)});
    }
    call(ctx, 0x00489328u, {load<u32>(0x00F65CA0u), c + 140, c + 9, c + 74, load<u32>(c + 160), load<u32>(c + 164),
                            (load<u32>(c + 168) & 0x80000000u) != 0 ? 1u : 0u});
    call(ctx, 0x0042CA60u, {c});
}

// u32 f(PostFx* o; f32 v) (from f_00275110): with the world's object w =
// (*0x00F65CA0)->+0x128168, the camera a = (w->+656 + 4)->+36 and its
// source b = (w->+656 + 4)->+4: o->+12 = a->+164 when the s8 state o->+8 is
// 0 or 1 (else 0.0); while the state stays 0 or 1, PostFx_UpdateSwap(o;
// f1 = FLT_MAX); then o->+0 = 0, +4 = 1, +16 = 1.0, +20 = v, +24 = 0, a->+160
// = b->+48, a->+144..+152 = b->+32..+40, u8 o->+8 = 0 and
// PostFx_UpdateSwap(o; f1 = 0.0) (r3 = its). r3 = o without w.
DECOMP(0048F008, PostFx_Restart_0048F008) {
    const u32 o = arg<u32>(ctx, 0);
    const f64 v = farg(ctx, 0);
    const u32 w = load<u32>(load<u32>(0x00F65CA0u) + 0x00128168u);
    if (w == 0) return;
    constexpr u32 K = 0x0048F000u;  // {0.0f, FLT_MAX}
    const u32 p = load<u32>(w + 656) + 4;
    const u32 a = load<u32>(p + 36), b = load<u32>(p + 4);
    auto busy = [&](s32 s) { return !(s < 0 || s >= 2); };
    s32 s = static_cast<s8>(load<u8>(o + 8));
    store<u32>(o + 12, busy(s) ? load<u32>(a + 164) : load<u32>(K));
    while (busy(s)) {
        callf(ctx, 0x004695A8u, {o}, {Lfs32(K + 4)});
        s = static_cast<s8>(load<u8>(o + 8));
    }
    store<u32>(o + 0, 0);
    store<u32>(o + 4, 1);
    store<u32>(o + 16, 0x3F800000u);
    Stfs32(o + 20, v);
    store<u32>(o + 24, 0);
    store<u32>(a + 160, load<u32>(b + 48));
    store<u32>(a + 144, load<u32>(b + 32));
    store<u32>(a + 148, load<u32>(b + 36));
    store<u32>(a + 152, load<u32>(b + 40));
    store<u8>(o + 8, 0);
    callf(ctx, 0x004695A8u, {o}, {Lfs32(K)});
}

// void f(T* o) (the body of Pooled_Delete_004AD298): vtable 0x00B21BF0,
// Obj_ReleaseBodies_004AD088(o); for the group g = o->+168 (when set):
// Array_RemoveSwap_004ACB90(g, o), and a group left empty (u16 +12 == 0)
// is removed from the map 0x010C1D50 by its key g->+0
// (SortedMap_Find788D3C, SortedMap_Remove788B48) and freed (vector g + 4
// PtrVector_Clear + Nop, SmallFree(g, 24)); o->+168 = 0. Then the vectors
// o + 148 / o + 128 PtrVector_Clear + Nop and vtable 0x00B13974 (r3 = it).
DECOMP(004AD154, Obj_Dtor_004AD154) {
    const u32 o = arg<u32>(ctx, 0);
    store<u32>(o, 0x00B21BF0u);
    call(ctx, 0x004AD088u, {o});
    if (load<u32>(o + 168) != 0) {
        call(ctx, 0x004ACB90u, {load<u32>(o + 168), o});
        const u32 g = load<u32>(o + 168);
        if (load<u16>(g + 12) == 0) {
            constexpr u32 MAP = 0x010C1D50u;
            Stack fr(ctx, 16);  // the key (sp+112)
            store<u32>(fr.addr(), load<u32>(g));
            call(ctx, 0x00788D3Cu, {MAP, fr.addr()});
            store<u32>(fr.addr(), load<u32>(g));
            call(ctx, 0x00788B48u, {MAP, fr.addr()});
            if (g != 0) {
                call(ctx, 0x0054EC1Cu, {g + 4});
                call(ctx, 0x0054EC18u, {g + 4});
                call(ctx, 0x00552420u, {g, 24});
            }
        }
        store<u32>(o + 168, 0);
    }
    call(ctx, 0x0054EC1Cu, {o + 148});
    call(ctx, 0x0054EC18u, {o + 148});
    call(ctx, 0x0054EC1Cu, {o + 128});
    call(ctx, 0x0054EC18u, {o + 128});
    store<u32>(o, 0x00B13974u);
    ret(ctx, 0x00B13974u);
}

// Entry* f(Pool* p, List* l) (from Pool_Forward_004B584C): a weighted
// random pick: with the old random value r (RandStep28, as an integer)
// and the threshold t = p->+64 * (r * 2^-16) (f32), the weights (f32 +16)
// of l's 76-byte entries (l->+12; count u16 (l->+4)->+2; an index out of
// range reads entry address 0) from the s16 start p->+68 on (p->+60 of
// them) are summed until the sum reaches t: r3 = that entry + 4; else the
// start entry + 4 (4 when out of range). f1 = the sum.
DECOMP(004B5708, Pool_PickWeighted_004B5708) {
    const u32 p = arg<u32>(ctx, 0), l = arg<u32>(ctx, 1);
    constexpr u32 K = 0x004B5700u;  // {2^-16 f, 0.0f}
    const f64 r = F39(static_cast<f64>(static_cast<s64>(RandStep28())));
    const f64 p64 = Lfs32(p + 64);
    const u32 i0 = static_cast<u32>(static_cast<s32>(static_cast<s16>(load<u16>(p + 68))));
    const u32 end = i0 + load<u32>(p + 60);
    f64 sum = Lfs32(K + 4);
    const f64 t = F39(p64 * F39(r * Lfs32(K)));
    auto elem = [&](u32 j) -> u32 {
        const s32 k = static_cast<s16>(j);
        if (k >= 0 && k < static_cast<s32>(load<u16>(load<u32>(l + 4) + 2))) return load<u32>(l + 12) + 76 * static_cast<u32>(k);
        return 0u;
    };
    for (u32 j = i0; j < end; ++j) {
        const u32 e = elem(j) + 4;
        sum = F39(sum + Lfs32(e + 12));
        if (sum > t || sum == t) {
            ctx.fpr[1] = sum;
            ret(ctx, e);
            return;
        }
    }
    ctx.fpr[1] = sum;
    u32 e0 = 0;
    if (static_cast<s32>(i0) >= 0 && static_cast<s32>(i0) < static_cast<s32>(load<u16>(load<u32>(l + 4) + 2)))
        e0 = load<u32>(l + 12) + 76 * i0;
    ret(ctx, e0 + 4);
}
