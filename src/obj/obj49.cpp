// Game code, part 49: the small-function sweep continued from the remaining
// 107 instruction functions (size order; obj48*.cpp did up to three of the
// 107s). Each was checked against the original in the PPC simulator (memory
// writes, calls and their arguments, r3 and f1).
#include "obj49.h"

using namespace sly4;
using namespace sly4::obj49;

// ---------------------------------------------------------------- sweep: 107 instructions
// Obj* f(?, XmlCursor* x): the 44-byte XML command object with vtable
// 0x00B11398: +12..+40 zeroed, then hashes +12 (default 0) and +16, floats
// +20..+32, hash +36 and float +40 read from x. r3 = the object (0 when the
// allocation failed).
DECOMP(0009F1B4, XmlCmd_Create_0009F1B4) {
    const u32 x = arg<u32>(ctx, 1);  // r3 unused
    const u32 m = NewXmlObj47(ctx, 44, x);
    if (m == 0) {
        ret(ctx, 0u);
        return;
    }
    store<u32>(m + 12, 0);
    store<u32>(m + 16, 0);
    store<u32>(m + 20, 0);
    store<u32>(m, 0x00B11398u);
    for (u32 off = 24; off <= 40; off += 4) store<u32>(m + off, 0);
    call(ctx, kReadHash47, {x, 0xC012EA6Eu, m + 12, 0});
    RdHash47(ctx, x, 0x966D0D95u, m + 16);
    RdF32_47(ctx, x, 0x000110ADu, m + 20);
    RdF32_47(ctx, x, 0x02A63CFAu, m + 24);
    RdF32_47(ctx, x, 0x0FF6C00Cu, m + 28);
    RdF32_47(ctx, x, 0x287B2E1Eu, m + 32);
    RdHash47(ctx, x, 0xC8FDDD9Eu, m + 36);
    RdF32_47(ctx, x, 0x0DC52BACu, m + 40);
    ret(ctx, m);
}

// u32 f(Obj* o, XmlCursor* x): constructor of the object with vtable
// 0x00B109B8 (Obj_InitVt_B21C98 base). +12 = 0, the u16 +16 = 0xFFFF, bit
// 0x8000 of the u16 +18 cleared, +20 = 1.0f, +24 = -1.0f; then the float +20
// (default 0.0f, the constant at 0x00091D68) and the hash +12 are read. When
// that hash is absent but the "FromTarget"-table hash 0x0027873A is there, its
// index in the two-entry table 0x00AC55F4 (if found) goes to +16, bit 0x8000
// of +18 is read as the bool 0x8FE1FDC1 and the float +24 (default 0.0f).
// r3 = the result of the last reader (0 when +12 was read).
DECOMP(00091D70, XmlCmd_Init_00091D70) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 x = arg<u32>(ctx, 1);
    const f64 zero = Lfs48(0x00091D68u);  // f31
    call(ctx, 0x004BB3B0u, {o, x});
    const u16 h18 = load<u16>(o + 18);
    store<u32>(o, 0x00B109B8u);
    store<u32>(o + 12, 0);
    store<u16>(o + 18, static_cast<u16>(h18 & 0x7FFFu));
    store<u32>(o + 20, 0x3F800000u);
    store<u16>(o + 16, 0xFFFFu);
    store<u32>(o + 24, 0xBF800000u);
    callf(ctx, kReadF32_47, {x, 0x04B1112Au, o + 20}, {zero});
    u32 r = static_cast<u32>(call(ctx, kReadHash47, {x, 0x8A93973Fu, o + 12, 0}));
    if (r == 0) {  // kXmlOk: an explicit hash, nothing more to read
        ret(ctx, 0u);
        return;
    }
    Stack fr(ctx, 16);  // sp+112: the hash, sp+116: the bool
    r = static_cast<u32>(call(ctx, kReadHash47, {x, 0x0027873Au, fr.addr(), 0}));
    if (r != 0) {
        ret(ctx, r);
        return;
    }
    const u32 want = load<u32>(fr.addr());
    const u16 h = load<u16>(o + 18);
    u32 i = 0;
    while (static_cast<s32>(i) < 2 && want != load<u32>(0x00AC55F4u + 4 * i)) ++i;
    if (static_cast<s32>(i) < 2) store<u16>(o + 16, static_cast<u16>(i));
    call(ctx, kReadBool47, {x, 0x8FE1FDC1u, fr.addr() + 4, (h & 0x8000u) != 0 ? 1u : 0u});
    const u16 h2 = load<u16>(o + 18);
    store<u16>(o + 18, static_cast<u16>((h2 & 0x7FFFu) | ((load<u8>(fr.addr() + 4) & 1u) << 15)));
    callf(ctx, kReadF32_47, {x, 0xB7EA3D94u, o + 24}, {zero});
    ret(ctx, static_cast<u32>(ctx.gpr[3]));  // the reader's result
}

// u32 f(Obj* o): the base destructor of a physics-backed object. Vtables
// 0x00B13B90 / 0x00B13BE4 (+0 / +12); o->+16 (if any) gets vt[40](p, 0) and
// is cleared; the body +1028 is removed from its world (+8, when set) and
// released, then cleared; +28 released and cleared; +272 removed (world)
// and released; +24 and +20 are Havok entities removed from their world
// (Hk_RemoveEntity92FFC8 with a result byte at sp+112 / sp+113) and released.
// Finally the base vtables 0x00B13988 (+912), 0x00B0DAEC (+0), 0x00B139A8
// (+864), 0x00B1200C (+12). r3 = 0x00B1200C.
DECOMP(001002C4, PhysObj_Destruct_001002C4) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 p16 = load<u32>(o + 16);
    store<u32>(o, 0x00B13B90u);
    store<u32>(o + 12, 0x00B13BE4u);
    if (p16 != 0) VcOE48(ctx, p16, 40, {p16, 0});
    store<u32>(o + 16, 0);
    const u32 b = load<u32>(o + 1028);
    if (b != 0) {
        const u32 w = load<u32>(b + 8);
        u32 q = b;
        if (w != 0) {
            call(ctx, 0x00931F00u, {w, b});
            q = load<u32>(o + 1028);
        }
        call(ctx, kSkelRelease49, {q});
        store<u32>(o + 1028, 0);
    }
    const u32 p28 = load<u32>(o + 28);
    if (p28 != 0) {
        call(ctx, kSkelRelease49, {p28});
        store<u32>(o + 28, 0);
    }
    const u32 p272 = load<u32>(o + 272);
    if (p272 != 0) {
        const u32 w = load<u32>(p272 + 8);
        u32 q = p272;
        if (w != 0) {
            call(ctx, 0x00931F00u, {w, p272});
            q = load<u32>(o + 272);
        }
        call(ctx, kSkelRelease49, {q});
    }
    Stack fr(ctx, 16);  // sp+112 / sp+113: Hk_RemoveEntity's result bytes
    for (u32 k = 0; k < 2; ++k) {
        const u32 off = k == 0 ? 24u : 20u;
        const u32 e = load<u32>(o + off);
        if (e == 0) continue;
        const u32 w = load<u32>(e + 8);
        u32 q = e;
        if (w != 0) {
            call(ctx, 0x0092FFC8u, {fr.addr() + k, w, e});
            q = load<u32>(o + off);
        }
        call(ctx, kSkelRelease49, {q});
    }
    store<u32>(o + 912, 0x00B13988u);
    store<u32>(o, 0x00B0DAECu);
    store<u32>(o + 864, 0x00B139A8u);
    store<u32>(o + 12, 0x00B1200Cu);
    ret(ctx, 0x00B1200Cu);
}

// u32 f(f32 out[3], Node* n, const f32 p[3], const f32 best[3]): the point
// farthest from p among best, n's centre (+144) and, recursively, the
// results for n's children (u16 count +604, array +596). out = the winner;
// r3 = the child count, f1 as the original leaves it.
DECOMP(0019521C, Node_FarthestPoint_0019521C) {
    const u32 out = arg<u32>(ctx, 0);
    const u32 n = arg<u32>(ctx, 1);
    const u32 p = arg<u32>(ctx, 2);
    const u32 best = arg<u32>(ctx, 3);
    const u32 c = n + 144;
    Cp48(out, best);
    Cp48(out + 4, best + 4);
    Cp48(out + 8, best + 8);
    f64 d = Dist49(best, p);
    const f64 dc = Dist49(c, p);
    f64 f1 = dc;
    if (d < dc) {
        d = dc;
        Cp48(out, c);
        Cp48(out + 4, c + 4);
        f1 = Lfs48(c + 8);
        Cp48(out + 8, c + 8);
    }
    Stack fr(ctx, 16);  // sp+112: the child's result
    const u32 t = fr.addr();
    u32 i = 0;
    for (;; ++i) {
        const u32 cnt = load<u16>(n + 604);
        if (i >= cnt) {
            ctx.fpr[1] = f1;
            ret(ctx, cnt);
            return;
        }
        call(ctx, 0x0019521Cu, {t, load<u32>(load<u32>(n + 596) + 4 * i), p, out});
        f1 = Lfs48(t + 8);
        if (Dist49(t, p) > d) {
            const f64 tx = Lfs48(t), ty = Lfs48(t + 4), tz = f1;
            Cp48(out, t);  // stfs of the lfs values: the raw bits
            Cp48(out + 4, t + 4);
            Cp48(out + 8, t + 8);
            const f64 dy = Fs49(ty, Lfs48(p + 4));
            const f64 dx = Fs49(tx, Lfs48(p));
            f1 = Fs49(tz, Lfs48(p + 8));
            d = Sqrt49(Fma49(f1, f1, Fma49(dx, dx, Fm49(dy, dy))));
        }
    }
}

// u32 f(Obj* a, Obj* b): 1 when b is not (or no longer) a fit partner for a,
// 0 when it is. The kind is b->+216 ? (b->+216)->+52 : 0. Kind 0x961B6157:
// 1 when a's position (+80) is more than 2.0 from b's (+144), or when the
// f32 at *(a->+12) is not below 1.0 and a->+436 is 0; else the horizontal
// distance from a->+80 to a's anchor (+144) decides: below 6.0, a->+436 = 5
// and 0, otherwise 1 (constants at 0x00197420). Otherwise without bit 0x200
// of b->+192: 0 for kind 0x02D45643, else 1. With it: 0 unless the byte
// a->+669 is set and none of the bytes +668 / +670 of a and b; then 1 when
// the roots of their +616 parent chains differ. f1 as the original leaves it.
DECOMP(00197430, Obj_CheckPartner_00197430) {
    const u32 a = arg<u32>(ctx, 0);
    u32 b = arg<u32>(ctx, 1);
    constexpr u32 K = 0x00197420u;  // {2.0, 1.0, 0.0, 6.0}
    const u32 k216 = load<u32>(b + 216);
    const u32 kind = k216 != 0 ? load<u32>(k216 + 52) : 0u;
    u32 r = 1;
    if (kind == 0x961B6157u) {
        const f64 d = Dist49(a + 80, b + 144);
        ctx.fpr[1] = Lfs48(a + 88);
        if (d > Lfs48(K)) {
            r = 1;
        } else if (!(Lfs48(load<u32>(a + 12)) < Lfs48(K + 4)) && load<u32>(a + 436) == 0) {
            r = 1;
        } else {
            const f64 k = Lfs48(K + 8);
            const f64 dx = Fs49(Lfs48(a + 80), Lfs48(a + 144));
            const f64 dz = Fs49(Lfs48(a + 88), Lfs48(a + 152));
            const f64 h = Sqrt49(Fma49(dz, dz, Fma49(dx, dx, Fm49(k, k))));
            ctx.fpr[1] = h;
            if (h < Lfs48(K + 12)) {
                store<u32>(a + 436, 5);
                r = 0;
            }
        }
    } else if ((load<u32>(b + 192) & 0x200u) == 0) {
        r = kind == 0x02D45643u ? 0u : 1u;
    } else if (load<u8>(a + 669) == 0) {
        r = 0;
    } else if (load<u8>(b + 668) != 0 || load<u8>(b + 670) != 0 || load<u8>(a + 668) != 0 || load<u8>(a + 670) != 0) {
        r = 0;
    } else {
        while (load<u32>(b + 616) != 0) b = load<u32>(b + 616);
        u32 c = a;
        while (load<u32>(c + 616) != 0) c = load<u32>(c + 616);
        r = b != c ? 1u : 0u;
    }
    ret(ctx, r);
}

// u32 f(Obj* o, const Desc* s): constructor (base Thunk_000E40B4(o, s)) of
// the object with vtables 0x00B17990 (+0), 0x00B17B3C (+380), 0x00B17B64
// (+384). Copies s->+208..216, +224..232, +240..252 to o->+448.., the 68
// bytes s->+256.. to o->+496..; clears +564..+592, +600, +608 and the 16
// bytes at +424; +596 = 2*pi; +604 keeps its low 27 bits and takes the top
// two bits of s->+324 (bits 0x38000000 cleared). Then
// StateMachine_SetState(o, 0, 0), whose result is r3.
DECOMP(001D5C14, Spawnee_Construct_001D5C14) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 s = arg<u32>(ctx, 1);
    call(ctx, 0x000E40B4u, {o, s});
    store<u32>(o + 380, 0x00B17B3Cu);
    store<u32>(o + 384, 0x00B17B64u);
    Cp48(o + 448, s + 208);
    Cp48(o + 452, s + 212);
    Cp48(o + 456, s + 216);
    Cp48(o + 464, s + 224);
    store<u32>(o, 0x00B17990u);
    Cp48(o + 468, s + 228);
    Cp48(o + 472, s + 232);
    for (u32 k = 0; k < 4; ++k) Cp48(o + 480 + 4 * k, s + 240 + 4 * k);
    for (u32 k = 0; k < 64; k += 8) {  // pairs: both words read, then written
        const u32 w0 = load<u32>(s + 256 + k), w1 = load<u32>(s + 260 + k);
        store<u32>(o + 496 + k, w0);
        store<u32>(o + 500 + k, w1);
    }
    Cp48(o + 560, s + 320);
    for (u32 off = 564; off <= 592; off += 4) store<u32>(o + off, 0);
    store<u32>(o + 596, 0x40C90FDBu);  // 2*pi
    store<u32>(o + 600, 0);
    const u32 f604 = load<u32>(o + 604), s324 = load<u32>(s + 324);
    store<u32>(o + 608, 0);
    store<u32>(o + 604, (f604 & 0x07FFFFFFu) | (s324 & 0xC0000000u));
    store<u64>(o + 424, 0);
    store<u64>(o + 432, 0);
    ctx.fpr[1] = Lfs48(s + 252);  // the last lfs
    ret(ctx, static_cast<u32>(call(ctx, 0x00464D1Cu, {o, 0, 0})));
}

// u32 f(Obj* o, f32 dt): advances the timer +880 by dt, clamped to
// [0.0 (the constant at 0x0023BFB0), limit] (limit = the f32 at
// (o->+396)->+804). Until it reaches the limit, a voice still playing
// (Voice_HandlePlaying_00443368(o->+864) != 0) ends it there (r3 = that).
// Otherwise +864 and +880 are cleared, the emitter +868 stopped
// (Emitter_Stop(.., 1)), Sound_Play44C474(&h, o->+904, 0...) restarts it
// (+868 = h), and +964 = 2 when o->+612's list (+72, 224-byte entries, count
// = the s8 at (p->+28)->+32) has no entry with +36 set, else 1. r3 is the
// list pointer the original leaves; f1 = the limit.
DECOMP(0023BFB8, Obj_UpdateLoopSound_0023BFB8) {
    const u32 o = arg<u32>(ctx, 0);
    const f64 dt = farg(ctx, 0);
    const f64 t = F48(Lfs48(o + 880) + dt);  // fadds
    const u32 limp = load<u32>(o + 396) + 804;
    const f64 lim = Lfs48(limp);
    constexpr u32 kZero = 0x0023BFB0u;
    // the stored value: t, or the raw bits of the limit / the 0.0 constant
    f64 v = t < lim ? t : lim;
    u32 vbits = t < lim ? W48(t) : load<u32>(limp);
    if (!(v > Lfs48(kZero))) {
        v = Lfs48(kZero);
        vbits = load<u32>(kZero);
    }
    store<u32>(o + 880, vbits);
    ctx.fpr[1] = lim;
    if (!(v > lim || v == lim)) {
        const u32 r = static_cast<u32>(call(ctx, 0x00443368u, {load<u32>(o + 864)}));
        if (r != 0) {
            ret(ctx, r);
            return;
        }
    }
    store<u32>(o + 864, 0);
    store<u32>(o + 880, 0);
    call(ctx, 0x00443424u, {load<u32>(o + 868), 1});
    Stack fr(ctx, 32);  // sp+144: the new handle
    // nine to eleven: the stack arguments sp+112..128, all 0
    call(ctx, 0x0044C474u, {fr.addr(), load<u32>(o + 904), 0, 0, 0, 0, 0, 0, 0, 0, 0});
    store<u32>(o + 868, load<u32>(fr.addr()));
    u32 state = 1;
    const u32 p = load<u32>(o + 612);
    u32 r3 = p;
    if (p != 0) {
        const u32 q = load<u32>(p + 28);
        if (q != 0) {
            const s32 n = static_cast<s8>(load<u8>(q + 32));
            if (n == 0) {
                state = 2;
            } else {
                r3 = load<u32>(p + 72);
                for (u32 i = 0;;) {
                    if (load<u32>(r3 + 36) != 0) break;
                    ++i;
                    r3 += 224;
                    if (!(static_cast<u32>(n) > i)) {
                        state = 2;
                        break;
                    }
                }
            }
        }
    }
    store<u32>(o + 964, state);
    ret(ctx, r3);
}

// The shared body of 002D33F8 / 002D35A8 (o, b, i): the shared list
// (o->+8)->+340 (a 20-byte PtrVector made on first use, u16 +6 = 240) gives
// entry i (240 bytes): when o's component 0x03D1FCFC (Effect_FindComponent
// on o->+32) is in state 5 (vt[32]), its +28 is released (vt[8]) and
// replaced by (entry->+192)->+160 (refcount u16 +4 bumped), and +36 =
// (entry->+192)->+164. Then Effect_SetHash4CF988(o->+32, b ? hb : h0); r3 =
// its result. A failed allocation leaves the slot 0 and the list read from
// address 0 (the original's quirk).
static void Effect_ApplyListEntry49(PPUContext& ctx, u32 hb, u32 h0) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 b = arg<u32>(ctx, 1);
    const u32 i = arg<u32>(ctx, 2);
    const u32 slot = load<u32>(o + 8) + 340;
    u32 v = load<u32>(slot);
    if (v == 0) {
        v = static_cast<u32>(call(ctx, 0x0055238Cu, {20}));
        store<u32>(slot, v);
        if (v != 0) {
            call(ctx, 0x005516E0u, {v});
            store<u16>(v + 6, 240);
            v = load<u32>(slot);
        }
    }
    const u32 c = static_cast<u32>(call(ctx, kFindComp48, {load<u32>(o + 32), 0x03D1FCFCu}));
    if (c != 0 && static_cast<u32>(VcOE48(ctx, c, 32, {c})) == 5) {
        const u32 off = i * 240;
        u32 arr = load<u32>(v);
        const u32 old = load<u32>(c + 28);
        const u32 nw = load<u32>(load<u32>(arr + off + 192) + 160);
        if (old != 0) {
            VcOE48(ctx, old, 8, {old});
            arr = load<u32>(v);
        }
        store<u32>(c + 28, nw);
        if (nw != 0) store<u16>(nw + 4, static_cast<u16>(load<u16>(nw + 4) + 1));
        store<u32>(c + 36, load<u32>(load<u32>(arr + off + 192) + 164));
    }
    const u32 h = (b & 0xFFu) != 0 ? hb : h0;
    ret(ctx, static_cast<u32>(call(ctx, 0x004CF988u, {load<u32>(o + 32), h})));
}

// u32 f(Obj* o, bool b, u32 i): Effect_ApplyListEntry49 with the hashes
// 0x0026D2FD (b) / 0x0021D542.
DECOMP(002D33F8, Effect_ApplyListEntry_002D33F8) { Effect_ApplyListEntry49(ctx, 0x0026D2FDu, 0x0021D542u); }

// u32 f(Obj* o, bool b, u32 i): the same with 0x7A281983 (b) / 0x8E69AEC8.
DECOMP(002D35A8, Effect_ApplyListEntry_002D35A8) { Effect_ApplyListEntry49(ctx, 0x7A281983u, 0x8E69AEC8u); }

// u32 f(Obj* o): restarts o's motion. Animator p = o->+4: vt[88](p, 1, 1,
// 1, 1); o->+96..104 = +160..168 - 10.0 * +192..200 (fnmsubs); then
// vt[84](p, &rec) with the 60-byte record {-(+176..184), the vec3
// 0x01164B60, +192..200, the new +96..104} at sp+144; a = vt[60](p);
// Anim_Restart2775A8(a, 0x2874B3AD, 0, 0, -1, r8 left, 0, 0, -, -, 0, -1;
// 1.0, 0.25, 0.0, 0.0). Then +88 = +364 = 0, +84 = 1, +92 = 1.0f; r3 = 1.
DECOMP(0040D230, Obj_RestartMotion_0040D230) {
    const u32 o = arg<u32>(ctx, 0);
    constexpr u32 K = 0x0040D220u;  // {10.0, 1.0, 0.25, 0.0}
    const u32 p = load<u32>(o + 4);
    VcOE48(ctx, p, 88, {p, 1, 1, 1, 1});
    const f64 k0 = Lfs48(K);
    const f64 n0 = F48(-std::fma(Lfs48(o + 192), k0, -Lfs48(o + 160)));
    const f64 n1 = F48(-std::fma(Lfs48(o + 196), k0, -Lfs48(o + 164)));
    const f64 n2 = F48(-std::fma(Lfs48(o + 200), k0, -Lfs48(o + 168)));
    Stack fr(ctx, 64);  // sp+144: the record
    const u32 rec = fr.addr();
    const u32 q = load<u32>(o + 4);
    Cp48(rec + 32, o + 192);
    Cp48(rec + 36, o + 196);
    Cp48(rec + 40, o + 200);
    store<u32>(o + 96, W48(n0));
    store<u32>(o + 100, W48(n1));
    store<u32>(o + 104, W48(n2));
    for (u32 k = 0; k < 3; ++k) store<u32>(rec + 4 * k, load<u32>(o + 176 + 4 * k) ^ 0x80000000u);  // fneg
    for (u32 k = 0; k < 3; ++k) Cp48(rec + 16 + 4 * k, 0x01164B60u + 4 * k);
    store<u32>(rec + 48, W48(n0));
    store<u32>(rec + 52, W48(n1));
    store<u32>(rec + 56, W48(n2));
    VcOE48(ctx, q, 84, {q, rec});
    const u32 p2 = load<u32>(o + 4);
    const u32 a = static_cast<u32>(VcOE48(ctx, p2, 60, {p2}));
    const f64 k3 = Lfs48(K + 12);
    // r8 and the 9th / 10th stack slots are leftovers the callee does not read
    callf(ctx, 0x002775A8u, {a, 0x2874B3ADu, 0, 0, ~0ull, ctx.gpr[8], 0, 0, 0, 0, 0, ~0ull}, {Lfs48(K + 4), Lfs48(K + 8), k3, k3});
    store<u32>(o + 88, 0);
    store<u32>(o + 364, 0);
    store<u32>(o + 84, 1);
    store<u32>(o + 92, 0x3F800000u);
    ret(ctx, 1u);
}

// Obj* f(Src* s, Sink* k): builds a query record on the stack (sp+144,
// QueryEx_Construct_000E40E0(&rec, s->vt[32](s), s, 0, 0x01164BD0,
// 0x01164B30, 1, 0, 0, 0, 0, 0)), hands it to k->vt[8](k, &rec), then makes
// the 432-byte physics object from it (SmallAlloc, PhysObj_Init(p, &rec),
// vtables 0x00B12A90 / 0x00B12C3C (+380) / 0x00B12C64 (+384)) and calls its
// vt[8](p) - read through a null p when the allocation failed (the
// original's quirk). The record's vectors (+164, +144, and the one *rec+0
// points to, SmallFree'd with size 20) are then destroyed. r3 = p.
DECOMP(000E4314, PhysObj_CreateFromQuery_000E4314) {
    const u32 s = arg<u32>(ctx, 0);
    const u32 k = arg<u32>(ctx, 1);
    Stack fr(ctx, 240);  // sp+144..383: the record
    const u32 rec = fr.addr();
    const u32 r = static_cast<u32>(VcOE48(ctx, s, 32, {s, k}));  // r4 = k left in place
    call(ctx, 0x000E40E0u, {rec, r, s, 0, 0x01164BD0u, 0x01164B30u, 1, 0, 0, 0, 0, 0});
    VcOE48(ctx, k, 8, {k, rec});
    const u32 p = static_cast<u32>(call(ctx, 0x0055238Cu, {432}));
    if (p != 0) {
        call(ctx, 0x000E38F0u, {p, rec});
        store<u32>(p, 0x00B12A90u);
        store<u32>(p + 380, 0x00B12C3Cu);
        store<u32>(p + 384, 0x00B12C64u);
    }
    VcOE48(ctx, p, 8, {p});
    for (u32 off : {164u, 144u}) {
        call(ctx, 0x0054EC1Cu, {rec + off});  // PtrVector_Clear
        call(ctx, 0x0054EC18u, {rec + off});  // its (empty) destructor
    }
    const u32 v = load<u32>(rec);
    if (v != 0) {
        call(ctx, 0x0054EC1Cu, {v});
        call(ctx, 0x0054EC18u, {v});
        call(ctx, 0x00552420u, {load<u32>(rec), 20});  // SmallFree
    }
    ret(ctx, p);
}

// u32 f(Link* o): resolves the two ends of a link. The point map
// (*0x00F8F154)->+16 (PointMap_Find) gives the point for the hash +32 (to
// +28), else Obj_FindByHandle42C2B0(+16, 0) the object (to +12); likewise
// +40 -> +36 or +24 -> +20. When the first end resolved and so did the
// second: o->+80..88 = the point's position (+64) and o->+64..76 its
// rotation (Quat_FromMatrix of +16), or with no point the object's position
// (+144) and rotation (+96). Then Obj_ResetMatrix_000FCD1C(root->+0x1281E0)
// and Flag_Set8_40000000(o), whose result is r3.
DECOMP(004300D8, Link_Resolve_004300D8) {
    const u32 o = arg<u32>(ctx, 0);
    Stack fr(ctx, 48);  // sp+112 / sp+128: the quaternions, sp+144 / sp+148: the keys
    const u32 sp = fr.addr();
    auto find = [&](u32 key_off, u32 key_slot) -> u32 {
        const u32 map = load<u32>(load<u32>(0x00F8F154u) + 16);
        store<u32>(key_slot, load<u32>(o + key_off));
        return load<u32>(static_cast<u32>(call(ctx, 0x0073343Cu, {map, key_slot})));
    };
    auto finish = [&]() {
        call(ctx, 0x000FCD1Cu, {load<u32>(load<u32>(kGameRoot48) + 0x001281E0u)});
        ret(ctx, static_cast<u32>(call(ctx, 0x0042CA60u, {o})));
    };
    u32 r = find(32, sp + 36);
    store<u32>(o + 28, r);
    if (r == 0) {
        r = static_cast<u32>(call(ctx, 0x0042C2B0u, {load<u32>(o + 16), 0}));
        store<u32>(o + 12, r);
        if (r == 0) return finish();
    }
    r = find(40, sp + 32);
    store<u32>(o + 36, r);
    if (r == 0) {
        r = static_cast<u32>(call(ctx, 0x0042C2B0u, {load<u32>(o + 24), 0}));
        store<u32>(o + 20, r);
        if (r == 0) return finish();
    }
    const u32 p = load<u32>(o + 28);
    u32 pos, mtx, q;
    if (p != 0) {
        pos = p + 64, mtx = p + 16, q = sp;
    } else {
        const u32 b = load<u32>(o + 12);
        pos = b + 144, mtx = b + 96, q = sp + 16;
    }
    Cp48(o + 80, pos);
    Cp48(o + 84, pos + 4);
    Cp48(o + 88, pos + 8);
    call(ctx, 0x00555478u, {q, mtx});
    for (u32 k = 0; k < 4; ++k) Cp48(o + 64 + 4 * k, q + 4 * k);
    ctx.fpr[1] = Lfs48(q + 8);  // f1 / f2 hold the last pair
    finish();
}

// u32 f(Obj* o, u32 a, u32 b, f32 f1..f4, -, -, -, -, const Vec16* src):
// constructor of the object with vtable 0x00B21160 (Obj_InitVt_B20CC8(o, 1)
// base): +12 = a, +16 = b, +20..32 = f1..f4, +36 = +40 = 0, the member at
// +44 (Ctor_B21B88_004A0DB0), the 16-byte-element PtrVector +80 (u16 +86 =
// 16, PtrVector_Init(.., 16, 0, 1)) filled with src's elements (array +0,
// u16 count +8), growing by doubling; the u16 +90 tracks the high-water
// count. r3 = src's count.
DECOMP(0043554C, Obj_Construct_0043554C) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 a = arg<u32>(ctx, 1);
    const u32 b = arg<u32>(ctx, 2);
    const u32 src = arg<u32>(ctx, 7);
    const f64 f1 = farg(ctx, 0), f2 = farg(ctx, 1), f3 = farg(ctx, 2), f4 = farg(ctx, 3);
    call(ctx, 0x0042C9D0u, {o, 1});
    store<u32>(o + 12, a);
    store<u32>(o + 16, b);
    store<u32>(o + 20, W48(f1));
    store<u32>(o + 24, W48(f2));
    store<u32>(o + 28, W48(f3));
    store<u32>(o, 0x00B21160u);
    store<u32>(o + 32, W48(f4));
    store<u32>(o + 36, 0);
    store<u32>(o + 40, 0);
    call(ctx, 0x004A0DB0u, {o + 44});
    const u32 v = o + 80;
    call(ctx, 0x005516E0u, {v});
    store<u16>(o + 86, 16);
    call(ctx, 0x00551598u, {v, 16, 0, 1});
    for (u32 i = 0;; ++i) {
        const u32 cnt = load<u16>(src + 8);
        if (i >= cnt) {
            ret(ctx, cnt);
            return;
        }
        const u32 e = load<u32>(src) + 16 * i;
        const u32 cap = load<u16>(o + 84);
        if (load<u16>(o + 88) == cap) call(ctx, 0x00551718u, {v, (cap << 1) & 0x1FFFEu});
        const u32 d = load<u32>(o + 80) + ((static_cast<u32>(load<u16>(o + 88)) << 4) & 0xFFFF0u);
        const u32 w1 = load<u32>(e + 4), w0 = load<u32>(e);
        store<u32>(d + 4, w1);
        const u32 w2 = load<u32>(e + 8);
        store<u32>(d, w0);
        store<u32>(d + 8, w2);
        store<u32>(d + 12, load<u32>(e + 12));
        const u32 n = load<u16>(o + 88) + 1u;
        const u32 hi = load<u16>(o + 90);
        store<u16>(o + 88, static_cast<u16>(n));
        store<u16>(o + 90, static_cast<u16>(static_cast<s32>(hi) > static_cast<s32>(n & 0xFFFFu) ? hi : n));
    }
}

// Appends a 100-byte copy of str to the string list t (count +0, array
// +68, capacity +72) when there is room (Mem_Alloc(100, 0, 16) + String_CopyN;
// a failed allocation is stored and copied into, as in the original).
static void StrList_Add49(PPUContext& ctx, u32 t, u32 str) {
    if (!(load<u32>(t) < load<u32>(t + 72))) return;
    const u32 m = static_cast<u32>(call(ctx, 0x0054ED38u, {100, 0, 16}));
    store<u32>(load<u32>(t + 68) + 4 * load<u32>(t), m);
    call(ctx, 0x00553638u, {load<u32>(load<u32>(t + 68) + 4 * load<u32>(t)), str, 100});
    store<u32>(t, load<u32>(t) + 1);
}

// u32 f(Obj* o): registers o's five parameters (0x010AB388 + 24k,
// Obj_RegisterParam), reserves two bindings on the last one's list (+16)
// and adds the names 0x00AE36FC and 0x00AE3704 to it, then
// Spawner_RegisterParams_0043BC58(o); (o->+8)->+44 = 0x00AE3714 with bit
// 0x80 of its byte +40 set, o->+88 = 0x00F39B38. r3 = o->+8.
DECOMP(0043C1B0, Spawner_RegisterParams_0043C1B0) {
    const u32 o = arg<u32>(ctx, 0);
    constexpr u32 kParams = 0x010AB388u;
    for (u32 k = 0; k < 5; ++k) call(ctx, 0x004F7128u, {o, kParams + 24 * k});
    const u32 last = kParams + 24 * 4;
    call(ctx, 0x004D2BB8u, {load<u32>(last + 16), 2});
    StrList_Add49(ctx, load<u32>(last + 16), 0x00AE36FCu);
    StrList_Add49(ctx, load<u32>(last + 16), 0x00AE3704u);
    call(ctx, 0x0043BC58u, {o});
    const u32 q = load<u32>(o + 8);
    const u8 b40 = load<u8>(q + 40);
    store<u32>(q + 44, 0x00AE3714u);
    store<u8>(q + 40, static_cast<u8>(b40 | 0x80u));
    store<u32>(o + 88, 0x00F39B38u);
    ret(ctx, q);
}

// u32 f(Obj* o, Mtx* out, u32 key, u32 a, u32 b, u32 c): out = identity
// (0x01164BD0); for a live o with bit 0x40000 of +172, the key is looked
// up as a record of (o->+160)->+208's table +108 (Rec80_Find), else as a
// bone (Obj_FindBone0D14DC, -1 = none), else as a child
// (Obj_FindByKey_000D1454), and Obj_AimMatrix(o, out, rec, bone, child, a,
// b, c) gives r3 (0 otherwise; a null key skips the lookups).
DECOMP(004A06E8, Obj_GetAttachMatrix_004A06E8) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 out = arg<u32>(ctx, 1);
    const u32 key = arg<u32>(ctx, 2);
    const u32 a = arg<u32>(ctx, 3), b = arg<u32>(ctx, 4), c = arg<u32>(ctx, 5);
    constexpr u32 kIdent = 0x01164BD0u;
    for (u32 row = 0; row < 4; ++row)
        for (u32 k = 0; k < 3; ++k) Cp48(out + 16 * row + 4 * k, kIdent + 16 * row + 4 * k);
    ctx.fpr[1] = Lfs48(kIdent + 56);
    if (o == 0) {
        ret(ctx, 0u);
        return;
    }
    if ((load<u32>(o + 172) & 0x40000u) == 0) {
        ret(ctx, 0u);
        return;
    }
    u32 rec = 0, child = 0;
    u64 bone = ~0ull;  // li r24, -1 / the callee's full r3
    if (key != 0) {
        const u32 p = load<u32>(o + 160);
        const u32 q = p != 0 ? load<u32>(p + 208) : 0u;
        if (q != 0) {
            const u32 t = load<u32>(q + 108);
            if (t != 0) rec = static_cast<u32>(call(ctx, 0x005848CCu, {t, key}));
        }
        if (rec == 0) {
            call(ctx, 0x000D14DCu, {o, key});
            bone = ctx.gpr[3];
            if (static_cast<u32>(bone) == 0xFFFFFFFFu) child = static_cast<u32>(call(ctx, 0x000D1454u, {o, key}));
        }
    }
    ret(ctx, static_cast<u32>(call(ctx, 0x0049FFE0u, {o, out, rec, bone, child, a, b, c})));
}

// ---------------------------------------------------------------- sweep: 108 instructions
// u32 f(Actor* o, u32 c): starts a mutual face-off between o and its
// partner q = o->+52 when q->vt[120](q) agrees, o->+56 is below the square
// of the range entry (load(o->+308)->+12)[(0x00F94C8C)->+64], the u16 at
// root+0x128214 is 0, q lies in front of o (dot of q - o, both positions
// +176, with o's facing +160 above 0.0) and o lies in front of q, and both
// Ai_InRange_00039314 checks pass: Actor_FaceTarget_00039178 both ways.
// r3 / f1 as the original leaves them.
DECOMP(00039470, Actor_TryFaceOff_00039470) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 c = arg<u32>(ctx, 1);
    const u32 p = load<u32>(o + 52);
    if (p == 0) {
        ret(ctx, 0u);
        return;
    }
    u32 r = static_cast<u32>(VcOE48(ctx, p, 120, {p}));
    if (r == 0) {
        ret(ctx, 0u);
        return;
    }
    const u32 idx4 = load<u32>(load<u32>(0x00F94C7Cu + 16) + 64) << 2;
    const f64 lim = Lfs48(load<u32>(load<u32>(o + 308) + 12) + idx4);
    ctx.fpr[1] = Lfs48(o + 56);
    if (!(ctx.fpr[1] < Fm49(lim, lim))) {
        ret(ctx, idx4);
        return;
    }
    r = load<u16>(load<u32>(kGameRoot48) + 0x00128214u);
    if (r != 0) {
        ret(ctx, r);
        return;
    }
    const u32 q = load<u32>(o + 52);
    const f64 dy = Fs49(Lfs48(q + 180), Lfs48(o + 180));
    const f64 dx = Fs49(Lfs48(q + 176), Lfs48(o + 176));
    const f64 dz = Fs49(Lfs48(q + 184), Lfs48(o + 184));
    constexpr u32 kZero = 0x00039468u;
    const f64 k = Lfs48(kZero);
    ctx.fpr[1] = k;
    const f64 d0 = Fma49(dz, Lfs48(o + 168), Fma49(dx, Lfs48(o + 160), Fm49(dy, Lfs48(o + 164))));
    if (!(d0 > k)) {
        ret(ctx, kZero);
        return;
    }
    const f64 d1 = Fma49(dz, Lfs48(q + 168), Fma49(dx, Lfs48(q + 160), Fm49(dy, Lfs48(q + 164))));
    if (!(d1 < k)) {
        ret(ctx, q + 160);
        return;
    }
    if (call(ctx, 0x00039314u, {o, q, c}) == 0 || call(ctx, 0x00039314u, {q, o, c}) == 0) {
        ret(ctx, 0u);
        return;
    }
    call(ctx, 0x00039178u, {o, q, c});
    ret(ctx, static_cast<u32>(call(ctx, 0x00039178u, {q, o, c})));
}

// u32 f(Obj* o, f32 dt): Obj_UpdateMotion16A40(o; dt); then, when the
// per-level flag byte (load(o->+308)->+12)[(0x00F95498)->+64] is set, the
// rider o->+68 is live (GetFlag_002697F8) and in mode 1 (bits 28..30 of
// +416), and its Obj_GetVec304_0026D824 vector v points up (v.y > 0.5):
// o's matrix (+128) is re-orthonormalised with its up row (+144) blended
// toward v (0.9 * up + 0.1 * v): side (+128) = normalize(up x fwd(+160)),
// fwd = normalize(side x up); the position row (+176) is rewritten as is.
// r3 / f1 as the original leaves them (constants at 0x0003DC84).
DECOMP(0003DC98, Obj_UpdateRiderTilt_0003DC98) {
    const u32 o = arg<u32>(ctx, 0);
    callf(ctx, 0x00016A40u, {o}, {farg(ctx, 0)});
    const u32 idx4 = load<u32>(load<u32>(0x00F95488u + 16) + 64) << 2;
    const u32 b = load<u8>(load<u32>(load<u32>(o + 308) + 12) + idx4);
    if (b == 0) {
        ret(ctx, 0u);
        return;
    }
    if (load<u32>(o + 68) == 0) {
        ret(ctx, 0u);
        return;
    }
    const u32 g = static_cast<u32>(call(ctx, 0x002697F8u, {load<u32>(o + 68)}));
    if (g == 0) {
        ret(ctx, 0u);
        return;
    }
    const u32 p = load<u32>(o + 68);
    if (((load<u32>(p + 416) >> 28) & 7u) != 1) {
        ret(ctx, p);
        return;
    }
    Stack fr(ctx, 16);  // sp+112: v
    const u32 v = fr.addr();
    call(ctx, 0x0026D824u, {v, p});
    constexpr u32 K = 0x0003DC84u;  // {0.5, 0.9, 0.1, 1.0}
    f64 f1 = Lfs48(v + 4);
    ctx.fpr[1] = f1;
    if (!(f1 > Lfs48(K))) {
        ret(ctx, K);
        return;
    }
    const u32 m = o + 128;
    const f64 kb = Lfs48(K + 8), ka = Lfs48(K + 4), one = Lfs48(K + 12);
    const f64 uy = Fma49(Lfs48(m + 20), ka, Fm49(f1, kb));
    const f64 ux = Fma49(Lfs48(m + 16), ka, Fm49(Lfs48(v), kb));
    const f64 uz = Fma49(Lfs48(m + 24), ka, Fm49(Lfs48(v + 8), kb));
    const f64 fx = Lfs48(m + 32), fy = Lfs48(m + 36), fz = Lfs48(m + 40);
    f64 sz = Fma49(ux, fy, -Fm49(uy, fx));  // fmsubs
    f64 sy = Fma49(uz, fx, -Fm49(ux, fz));
    f64 sx = Fma49(uy, fz, -Fm49(uz, fy));
    const u32 px = load<u32>(m + 48), py = load<u32>(m + 52), pz = load<u32>(m + 56);
    store<u32>(o + 144, W48(ux));
    store<u32>(o + 148, W48(uy));
    store<u32>(o + 152, W48(uz));
    store<u32>(o + 176, px);
    store<u32>(o + 180, py);
    const f64 n0 = Sqrt49(Fma49(sz, sz, Fma49(sx, sx, Fm49(sy, sy))));
    store<u32>(o + 184, pz);
    const f64 i0 = F48(one / n0);  // fdivs
    sx = Fm49(sx, i0);
    sz = Fm49(sz, i0);
    sy = Fm49(sy, i0);
    store<u32>(o + 128, W48(sx));
    store<u32>(o + 132, W48(sy));
    const f64 gy = Fma49(sz, ux, -Fm49(sx, uz));
    const f64 gx = Fma49(sy, uz, -Fm49(sz, uy));
    const f64 gz = Fma49(sx, uy, -Fm49(sy, ux));
    store<u32>(o + 136, W48(sz));
    const f64 n1 = Sqrt49(Fma49(gz, gz, Fma49(gx, gx, Fm49(gy, gy))));
    const f64 i1 = F48(one / n1);
    f1 = Fm49(gz, i1);
    store<u32>(o + 160, W48(Fm49(gx, i1)));
    store<u32>(o + 164, W48(Fm49(gy, i1)));
    store<u32>(o + 168, W48(f1));
    ctx.fpr[1] = f1;
    ret(ctx, K);
}

// Obj* f(?, XmlCursor* x): the 40-byte XML command object with vtable
// 0x00B0F0E0: +12..+28 = 0, +32 = 1.0f, +36 = 0; hashes +12 and +16
// (default 0). When either is set: hash +20, floats +24, +28, +32 and the
// s32 +36 (through a zeroed stack slot) are read too. r3 = the object (0
// when the allocation failed).
DECOMP(0006C1B8, XmlCmd_Create_0006C1B8) {
    const u32 x = arg<u32>(ctx, 1);  // r3 unused
    const u32 m = NewXmlObj47(ctx, 40, x);
    if (m == 0) {
        ret(ctx, 0u);
        return;
    }
    store<u32>(m + 12, 0);
    store<u32>(m + 16, 0);
    store<u32>(m + 20, 0);
    store<u32>(m + 24, 0);
    store<u32>(m + 28, 0);
    store<u32>(m, 0x00B0F0E0u);
    store<u32>(m + 32, 0x3F800000u);
    store<u32>(m + 36, 0);
    call(ctx, kReadHash47, {x, 0x8A93973Fu, m + 12, 0});
    call(ctx, kReadHash47, {x, 0x7A30D7E3u, m + 16, 0});
    if (load<u32>(m + 12) != 0 || load<u32>(m + 16) != 0) {
        RdHash47(ctx, x, 0x63D534F3u, m + 20);
        RdF32_47(ctx, x, 0xB7EA3D94u, m + 24);
        RdF32_47(ctx, x, 0x00210FAAu, m + 28);
        RdF32_47(ctx, x, 0x00000A7Bu, m + 32);
        Stack fr(ctx, 16);  // sp+112
        store<u32>(fr.addr(), 0);
        call(ctx, kReadS32_48, {x, 0xE955C5C0u, fr.addr(), Sx48(load<u32>(m + 36))});
        store<u32>(m + 36, load<u32>(fr.addr()));
    }
    ret(ctx, m);
}

// Obj* f(?, XmlCursor* x): the 48-byte XML command object with vtable
// 0x00B10DA0: vec3 +16 (default the vec3 0x01164B70 copied in, read with a
// zero default), float +32 (1.0f; read with the default 0x000990FC), s32
// +36 (-1), hash +40 (0), bools +44 and +45 (0). r3 = the object (0 when
// the allocation failed).
DECOMP(00099100, XmlCmd_Create_00099100) {
    const u32 x = arg<u32>(ctx, 1);  // r3 unused
    const u32 m = NewXmlObj47(ctx, 48, x);
    if (m == 0) {
        ret(ctx, 0u);
        return;
    }
    constexpr u32 kV = 0x01164B70u;
    Cp48(m + 16, kV);
    store<u32>(m, 0x00B10DA0u);
    Cp48(m + 20, kV + 4);
    Cp48(m + 24, kV + 8);
    const f64 k = Lfs48(0x000990FCu);
    store<u32>(m + 32, 0x3F800000u);
    store<u32>(m + 36, 0xFFFFFFFFu);
    store<u32>(m + 40, 0);
    store<u8>(m + 44, 0);
    store<u8>(m + 45, 0);
    Stack fr(ctx, 16);  // sp+112: the zero vec3 default
    for (u32 i = 0; i < 3; ++i) store<u32>(fr.addr() + 4 * i, 0);
    call(ctx, 0x004F9E0Cu, {x, 0x87129BE7u, m + 16, fr.addr()});
    callf(ctx, kReadF32_47, {x, 0x0D872DDFu, m + 32}, {k});
    call(ctx, kReadS32_48, {x, 0xA212C671u, m + 36, ~0ull});
    call(ctx, kReadHash47, {x, 0xF847EC69u, m + 40, 0});
    call(ctx, kReadBool47, {x, 0xAA2C436Fu, m + 44, 0});
    call(ctx, kReadBool47, {x, 0x63FE154Eu, m + 45, 0});
    ret(ctx, m);
}

// u32 f(Obj* o, u8 a, u8 b, u8 c, u8 d): forwards an enable state. o->+160
// (if any) gets vt[40](p, a, b); bit 0x100 of o->+172 = a's bit 0; with c,
// every live part (+192) of the 240-byte entries of o->+340 (u16 count +8)
// gets vt[88](part, a, b, c, d); with d, every child of the list o->+248
// gets vt[48](child, a, b). r3 = the last list pointer read (the incoming
// r4 when neither list is walked).
DECOMP(000D9F1C, Obj_ForwardEnable_000D9F1C) {
    const u32 o = arg<u32>(ctx, 0);
    const u64 r4 = ctx.gpr[4];
    const u32 a = arg<u32>(ctx, 1) & 0xFFu, b = arg<u32>(ctx, 2) & 0xFFu;
    const u32 c = arg<u32>(ctx, 3) & 0xFFu, d = arg<u32>(ctx, 4) & 0xFFu;
    const u32 p = load<u32>(o + 160);
    if (p != 0) VcOE48(ctx, p, 40, {p, a, b});
    store<u32>(o + 172, (load<u32>(o + 172) & ~0x100u) | ((static_cast<u32>(r4) & 1u) << 8));
    u64 r3 = r4;
    if (c != 0) {
        for (u32 i = 0;; ++i) {
            const u32 t = load<u32>(o + 340);
            r3 = t;
            const u32 n = t != 0 ? load<u16>(t + 8) : 0u;
            if (i >= n) break;
            const u32 e = load<u32>(load<u32>(t) + 240 * i + 192);
            if (e != 0) VcOE48(ctx, e, 88, {e, a, b, c, d});
        }
    }
    if (d != 0) {
        for (u32 i = 0;; ++i) {
            const u32 t = load<u32>(o + 248);
            r3 = t;
            const u32 n = t != 0 ? load<u16>(t + 8) : 0u;
            if (i >= n) break;
            const u32 e = load<u32>(load<u32>(t) + 4 * i);
            VcOE48(ctx, e, 48, {e, a, b});
        }
    }
    ctx.gpr[3] = r3;
}

// fsel: a >= 0.0 ? c : b (NaN picks b).
static f64 Fsel49(f64 a, f64 c, f64 b) { return a >= 0.0 ? c : b; }

// bool f(const f32 a[3], const f32 b[3], const f32 c[3], const f32 d[3],
// f32 out[3]): the XZ-plane intersection of the lines ab and cd; 1 with
// out = {x, a.y, z} when it lies within both segments' bounding boxes
// (0.001 slack; constants at 0x001829A4), 0 when parallel or outside. f1
// as the original leaves it.
DECOMP(001829B0, Seg2_IntersectXZ_001829B0) {
    const u32 a = arg<u32>(ctx, 0), b = arg<u32>(ctx, 1), c = arg<u32>(ctx, 2), d = arg<u32>(ctx, 3);
    const u32 out = arg<u32>(ctx, 4);
    constexpr u32 K = 0x001829A4u;  // {0.0, 0.001}
    const f64 cx = Lfs48(c), dx = Lfs48(d), ax = Lfs48(a), bx = Lfs48(b);
    const f64 az = Lfs48(a + 8), bz = Lfs48(b + 8), cz = Lfs48(c + 8), dz = Lfs48(d + 8);
    const f64 cdx = Fs49(cx, dx), abz = Fs49(az, bz), abx = Fs49(ax, bx), cdz = Fs49(cz, dz);
    const f64 den = Fma49(abx, cdz, -Fm49(abz, cdx));
    ctx.fpr[1] = Lfs48(K);
    if (den == ctx.fpr[1]) {
        ret(ctx, 0u);
        return;
    }
    const f64 ccd = Fma49(cx, dz, -Fm49(dx, cz));
    const f64 cab = Fma49(ax, bz, -Fm49(bx, az));
    const f64 eps = Lfs48(K + 4);
    const f64 ix = F48(Fma49(cab, cdx, -Fm49(abx, ccd)) / den);
    const f64 iz = F48(Fma49(cab, cdz, -Fm49(abz, ccd)) / den);
    ctx.fpr[1] = eps;
    if (ix < Fs49(Fsel49(Fs49(bx, ax), ax, bx), eps) || ix > F48(Fsel49(abx, ax, bx) + eps) ||
        ix < Fs49(Fsel49(Fs49(dx, cx), cx, dx), eps) || ix > F48(Fsel49(cdx, cx, dx) + eps)) {
        ret(ctx, 0u);
        return;
    }
    if (iz < Fs49(Fsel49(Fs49(bz, az), az, bz), eps) || iz > F48(Fsel49(abz, az, bz) + eps) ||
        iz < Fs49(Fsel49(Fs49(dz, cz), cz, dz), eps)) {
        ret(ctx, 0u);
        return;
    }
    const f64 hi = F48(Fsel49(cdz, cz, dz) + eps);
    ctx.fpr[1] = hi;
    if (iz > hi) {
        ret(ctx, 0u);
        return;
    }
    store<u32>(out, W48(ix));
    ctx.fpr[1] = Lfs48(a + 4);
    Cp48(out + 4, a + 4);
    store<u32>(out + 8, W48(iz));
    ret(ctx, 1u);
}

// T* f(a, XmlCursor* x, Src* s, c) (XmlDef_NewFromNodeCA038): the 84-byte
// definition on the stack (+0 = 0x01400000, +12 = s->+12, +16 / +20 =
// 0x00AD366C, -1.0 at +36 / +40 / +72, +76 = 0x00AD36A8, u8 +80 = 1, zeros
// elsewhere) is read by Def_ReadBase20BFE8(a, x, s, c, &def), the hashes
// 0x04B6D0EF and 0x001EFE3C (default 0) are read, then a 192-byte object is
// made from it (SmallAlloc; r3 = 0 on failure): Thunk_000C2B90(p, &def), u8
// +172..+175 = def +80..+83, vtable 0x00B19510, String_CopyN(p + 140,
// def +76, 31), vtable 0x00B19920, +180 / +176 = the two hashes, +184 = 0.
// r3 = p.
DECOMP(002102BC, XmlDef_New192_002102BC) {
    const u32 a = arg<u32>(ctx, 0), x = arg<u32>(ctx, 1), s = arg<u32>(ctx, 2);
    const u64 c = ctx.gpr[6];
    Stack fr(ctx, 96);  // sp+112: the definition, sp+196 / sp+200: the hashes
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
    store<u32>(t + 4, 0);
    store<u32>(t + 8, 0);
    store<u32>(t + 76, 0x00AD36A8u);
    store<u8>(t + 80, 1);
    store<u8>(t + 81, 0);
    store<u8>(t + 82, 0);
    store<u8>(t + 83, 0);
    store<u32>(t, 0x01400000u);
    // r8..r10 as the original leaves them (the callee reads r3..r7)
    call(ctx, 0x0020BFE8u, {a, x, s, c, t, 0x00AD36A8u, 1, 0x01400000u});
    call(ctx, kReadHash47, {x, 0x04B6D0EFu, t + 88, 0});
    call(ctx, kReadHash47, {x, 0x001EFE3Cu, t + 84, 0});
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
    store<u32>(p, 0x00B19920u);
    store<u32>(p + 180, load<u32>(t + 88));
    const u32 h196 = load<u32>(t + 84);
    store<u32>(p + 184, 0);
    store<u32>(p + 176, h196);
    ret(ctx, p);
}

// u32 f(Fx* o): pushes o's value +12 into the owner's component
// 0xE9BEF0B6 (+20, when its vt[32] state is 1; owner = (o->+8)->+52); then
// detaches the handle +16 from the owner o->+8: Effect_Release(owner->+52,
// h) and, in the slot k (0..2) of owner->+56 whose +900 entry is the
// owner, the two listeners +916 + 8k (only in mode +432 == 1) and +920 +
// 8k get vt[128](l, h, 1). r3 as the original leaves it.
DECOMP(002FCA64, Fx_Detach_002FCA64) {
    const u32 o = arg<u32>(ctx, 0);
    u32 r3 = o;
    const u32 q = load<u32>(o + 8);
    if (q != 0) {
        const u32 e = load<u32>(q + 52);
        const u32 v12 = load<u32>(o + 12);  // f31
        if (e != 0) {
            const u32 c = static_cast<u32>(call(ctx, kFindComp48, {e, 0xE9BEF0B6u}));
            r3 = c;
            if (c != 0) {
                r3 = static_cast<u32>(VcOE48(ctx, c, 32, {c}));
                if (r3 == 1) store<u32>(c + 20, v12);
            }
        }
    }
    const u32 h = load<u32>(o + 16);
    const u32 w = h != 0 ? load<u32>(o + 8) : 0u;
    if (h == 0 || w == 0) {
        ret(ctx, r3);
        return;
    }
    r3 = load<u32>(w + 52);
    if (r3 != 0) r3 = static_cast<u32>(call(ctx, 0x004CF3C4u, {r3, h}));
    const u32 t = load<u32>(w + 56);
    if (t == 0) {
        ret(ctx, r3);
        return;
    }
    u32 k = 0;
    while (k < 3 && static_cast<s32>(load<u32>(t + 900 + 4 * k)) != static_cast<s32>(w)) ++k;
    if (k >= 3) {
        ret(ctx, 3u);
        return;
    }
    if (load<u32>(t + 432) == 1) {
        const u32 a = load<u32>(t + 916 + 8 * k);
        if (a != 0) VcOE48(ctx, a, 128, {a, h, 1});
    }
    const u32 b = load<u32>(t + 920 + 8 * k);
    if (b == 0) {
        ret(ctx, t + 920 + 8 * k);
        return;
    }
    ret(ctx, static_cast<u32>(VcOE48(ctx, b, 128, {b, h, 1})));
}

// u32 f(Shape* o, XmlCursor* x): hash +4, floats +8 / +12, bool +100, the
// first child (if any) into +80 (Shape_ReadXml_00302DA8), then floats
// +48..+68. r3 = the last reader's result.
DECOMP(003030E8, ShapeDef_ReadXml_003030E8) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 x = arg<u32>(ctx, 1);
    RdHash47(ctx, x, 0x1E78B4AEu, o + 4);
    RdF32_47(ctx, x, 0x1D424785u, o + 8);
    RdF32_47(ctx, x, 0x81D650F9u, o + 12);
    RdBool47(ctx, x, 0x78833C19u, o + 100);
    const u32 c = static_cast<u32>(call(ctx, 0x004F9140u, {x}));
    if (c != 0) call(ctx, 0x00302DA8u, {o + 80, c});
    static constexpr u32 kH[6] = {0x94A773B8u, 0x7B738C62u, 0xC687E664u, 0xC3C5F05Bu, 0x85AEB11Cu, 0x8BB08F02u};
    for (u32 k = 0; k < 6; ++k) RdF32_47(ctx, x, kH[k], o + 48 + 4 * k);
    ret(ctx, static_cast<u32>(ctx.gpr[3]));  // the last reader's result
}

// u32 f(Obj* o, u8 a): shuts o's attachments down. The holder +2660 (if
// any): when its vt[64]() gives a body, Obj_SetPhysicsActive(body, 1);
// then vt[40](h, 0) and +2660 = 0. Each of the five +2608.. slots gets
// vt[40](s, 1) and is cleared. When the byte +2980 is set: the root's
// +4208 object (+8) is notified (Obj_Notify24), Obj_SetBit27Notify_002E6E54
// (*0x00F64C20, 1), Game_SetPaused(1, 0), the notified result (if any)
// gets Flagged_SetOn_0029F428(r, 0), and +2980 = 0. Finally
// Recs_Reset_00325A30(o, a), whose result is r3.
DECOMP(0033876C, Obj_ShutdownAttachments_0033876C) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 a = arg<u32>(ctx, 1) & 0xFFu;
    const u32 h = load<u32>(o + 2660);
    if (h != 0) {
        const u32 body = static_cast<u32>(VcOE48(ctx, h, 64, {h}));
        if (body != 0) call(ctx, 0x00260C14u, {body, 1});
        const u32 h2 = load<u32>(o + 2660);
        VcOE48(ctx, h2, 40, {h2, 0});
        store<u32>(o + 2660, 0);
    }
    for (u32 k = 0; k < 5; ++k) {
        const u32 s = load<u32>(o + 2608 + 4 * k);
        if (s != 0) VcOE48(ctx, s, 40, {s, 1});
        store<u32>(o + 2608 + 4 * k, 0);
    }
    if (load<u8>(o + 2980) != 0) {
        u32 n = 0;
        const u32 s = load<u32>(load<u32>(kGameRoot48) + 4208 + 8);
        if (s != 0) n = static_cast<u32>(call(ctx, 0x000D949Cu, {s}));
        call(ctx, 0x002E6E54u, {load<u32>(0x00F64C20u), 1});
        call(ctx, 0x00365A54u, {1, 0});
        if (n != 0) call(ctx, 0x0029F428u, {n, 0});
        store<u8>(o + 2980, 0);
    }
    ret(ctx, static_cast<u32>(call(ctx, 0x00325A30u, {o, a})));
}

// u32 f(const Cell* c, Obj* o): when o's model (+12) has bit 8 of +192,
// copies the cell's table values into the model: the f32 +528 and the
// bytes +532 / +533. Rows (s16 c->+4) and columns (s16 c->+2) index the 128-byte
// records of (*0x00F65038)->+24 (-1 = none: address 0); the column
// record's bytes 0 / 1 / 2 at its +u16 0x0106CF7E (only when its byte +2
// is non-zero, else from address 0) pick 4-byte slots at +u16 0x0106CF70
// of the row record. r3 = 1 (0 without the bit); f1 = the f32.
DECOMP(003965A0, Cell_ApplyTable_003965A0) {
    const u32 c = arg<u32>(ctx, 0);
    const u32 o = arg<u32>(ctx, 1);
    const u32 q = load<u32>(o + 12);
    if (q == 0 || (load<u32>(q + 192) & 8u) == 0) {
        ret(ctx, 0u);
        return;
    }
    const s32 i = static_cast<s16>(load<u16>(c + 4));
    const s32 j = static_cast<s16>(load<u16>(c + 2));
    constexpr u32 kTab = 0x00F65038u, kSel = 0x0106CF7Eu, kOff = 0x0106CF70u;
    auto row = [&]() -> u32 { return i != -1 ? load<u32>(load<u32>(kTab) + 24) + (static_cast<u32>(i) << 7) : 0u; };
    auto sel = [&]() -> u32 {
        const u32 col = j != -1 ? load<u32>(load<u32>(kTab) + 24) + (static_cast<u32>(j) << 7) : 0u;
        return load<u8>(col + 2) != 0 ? col + load<u16>(kSel) : 0u;
    };
    // the row is computed before the column each time (table reloaded)
    u32 r = row();
    u32 s = sel();
    const u32 e0 = r + load<u16>(kOff) + (static_cast<u32>(load<u8>(s)) << 2);
    ctx.fpr[1] = Lfs48(e0);
    Cp48(q + 528, e0);
    r = row();
    s = sel();
    store<u8>(q + 532, load<u8>(r + load<u16>(kOff) + (static_cast<u32>(load<u8>(s + 1)) << 2)));
    r = row();
    s = sel();
    store<u8>(q + 533, load<u8>(r + load<u16>(kOff) + (static_cast<u32>(load<u8>(s + 2)) << 2)));
    ret(ctx, 1u);
}

// u32 f(Obj* o, s32 k): plays the animation and effect of o's state +432
// (0..2) for the variant k. The effect hash: state 0: 0x31A1BDCE
// (0x35367A84 for k == 2); 1: 0x3F1C7FBE (0x580080FC for k == 2); 2:
// 0x757DB1DE (0x0A16FE56 for k == 1); otherwise none. Unless bit 0x04000000
// of +456 is set, the state's animation hash (+132 + 8 * state; its name
// fetched when the debug byte 0x010853C4 is set) is restarted on the
// animator o->+412->vt[60]() (Anim_Restart2775A8(a, hash, 0, 0, -1, r8
// left, 0, 0, -, -, 0, -1; 1.0, 0.25, 0.0, 0.0), constants 0x003D4C64).
// Then Emitter_StopById44CEE8(3, 1) and, with an effect hash and an
// effect object +32, Effect_SetHash4CF988(e, hash). r3 = the last result.
DECOMP(003D4C70, Obj_PlayStateAnim_003D4C70) {
    const u32 o = arg<u32>(ctx, 0);
    const s32 k = static_cast<s32>(arg<u32>(ctx, 1));
    const s32 s = static_cast<s32>(load<u32>(o + 432));
    u32 h = 0;
    if (s == 0) h = k == 2 ? 0x35367A84u : 0x31A1BDCEu;
    else if (s == 1) h = k == 2 ? 0x580080FCu : 0x3F1C7FBEu;
    else if (s == 2) h = k == 1 ? 0x0A16FE56u : 0x757DB1DEu;
    const u32 anim = load<u32>(o + 8 * static_cast<u32>(s) + 132);
    if ((load<u32>(o + 456) & 0x04000000u) == 0) {
        if (load<u8>(0x010853C4u) != 0) call(ctx, kHashStr48, {anim});
        const u32 a = anim != 0 ? load<u32>(o + 412) : 0u;
        if (a != 0) {
            const u32 an = static_cast<u32>(VcOE48(ctx, a, 60, {a}));
            if (an != 0) {
                constexpr u32 K = 0x003D4C64u;  // {1.0, 0.25, 0.0}
                const f64 k8 = Lfs48(K + 8);
                // r8 and the 9th / 10th stack slots are leftovers the callee does not read
                callf(ctx, 0x002775A8u, {an, anim, 0, 0, ~0ull, ctx.gpr[8], 0, 0, 0, 0, 0, ~0ull}, {Lfs48(K), Lfs48(K + 4), k8, k8});
            }
        }
    }
    u32 r = static_cast<u32>(call(ctx, 0x0044CEE8u, {3, 1}));
    const u32 e = load<u32>(o + 32);
    if (e != 0 && h != 0) r = static_cast<u32>(call(ctx, 0x004CF988u, {e, h}));
    ret(ctx, r);
}

// u32 f(Obj* o): starts o's cutscene once. With the byte +53 set, only
// +482 = +480 = 1. Otherwise, unless bit 31 of +96 is set, the target +44
// is missing or in mode 6 (byte +18), or +480 is already set:
// Handle_Notify36742C(0x251B46E4, 0, 1, 0); the animator o->+468->vt[60]()
// restarts 0xA91758BD (Anim_Restart2775A8(.., 0, 0, -1, r8 left, 0, 0,
// -, -, 0, -1; 1.0, 0.25, 0.0, 0.0), constants 0x0041A94C); the root starts
// scene 0xA1B0F7E9 (World_StartScene46C3B8, all else 0); +480 = 1;
// SetSelect_491000(o, 0x20EB352D); o->+496->vt[24](p, 0, 0); +482 = +87 =
// 1 and bit 31 of +96 set. r3 as the original leaves it.
DECOMP(0041A958, Obj_StartCutscene_0041A958) {
    const u32 o = arg<u32>(ctx, 0);
    if (load<u8>(o + 53) != 0) {
        store<u8>(o + 482, 1);
        store<u8>(o + 480, 1);
        ret(ctx, 1u);
        return;
    }
    if ((load<u32>(o + 96) & 0x80000000u) != 0) {
        ret(ctx, 1u);
        return;
    }
    const u32 t = load<u32>(o + 44);
    if (t == 0) {
        ret(ctx, 0u);
        return;
    }
    if (load<u8>(t + 18) == 6) {
        ret(ctx, 6u);
        return;
    }
    const u32 b480 = load<u8>(o + 480);
    if (b480 != 0) {
        ret(ctx, b480);
        return;
    }
    call(ctx, 0x0036742Cu, {0x251B46E4u, 0, 1, 0});
    const u32 p = load<u32>(o + 468);
    const u32 a = static_cast<u32>(VcOE48(ctx, p, 60, {p}));
    constexpr u32 K = 0x0041A94Cu;  // {1.0, 0.25, 0.0}
    const f64 k8 = Lfs48(K + 8);
    // r8 and the 9th / 10th stack slots are leftovers the callee does not read
    callf(ctx, 0x002775A8u, {a, 0xA91758BDu, 0, 0, ~0ull, ctx.gpr[8], 0, 0, 0, 0, 0, ~0ull}, {Lfs48(K), Lfs48(K + 4), k8, k8});
    call(ctx, 0x0046C3B8u, {load<u32>(kGameRoot48), 0xA1B0F7E9u, 0, 0, 0, 0, 0, 0, 0, 0});
    store<u8>(o + 480, 1);
    call(ctx, 0x00491000u, {o, 0x20EB352Du});
    const u32 q = load<u32>(o + 496);
    VcOE48(ctx, q, 24, {q, 0, 0});
    const u32 f96 = load<u32>(o + 96) | 0x80000000u;
    store<u8>(o + 482, 1);
    store<u8>(o + 87, 1);
    store<u32>(o + 96, f96);
    ret(ctx, f96);
}

// bool f(Trigger* o, f32 dt): the timer +32 fires once it reaches the delay
// +28 (> 0.0, the constant at 0x00434AF0): with a message +24 it is posted
// (Msg_Post(0, o->+20, msg, 0)) and +36 = 1, without one r3 = 1 at once.
// Then +32 += dt and r3 = 1 when player slot o->+12 (0x20C80-byte records
// at root + 4208) has a live player (+8, bit 8 of +192) whose o->+380
// object answers vt[64](p, o->+16) with a mask that hits both words +8 /
// +12 of the 11968-byte table entry 0x010B19E0 + 176 picked by the slot's
// byte +1; 0 otherwise. f1 as the original leaves it.
DECOMP(00434AF8, Trigger_Update_00434AF8) {
    const u32 o = arg<u32>(ctx, 0);
    const f64 dt = farg(ctx, 0);
    const f64 delay = Lfs48(o + 28);
    ctx.fpr[1] = delay;
    if (delay > Lfs48(0x00434AF0u) && (Lfs48(o + 32) > delay || Lfs48(o + 32) == delay)) {
        const u32 m = load<u32>(o + 24);
        if (m == 0) {
            ret(ctx, 1u);
            return;
        }
        call(ctx, 0x00470660u, {0, load<u32>(o + 20), m, 0});
        store<u8>(o + 36, 1);
    }
    constexpr u32 kRec = 0x00020C80u;
    const u32 slot = load<u32>(kGameRoot48) + load<u32>(o + 12) * kRec + 4208;
    const f64 t = F48(Lfs48(o + 32) + dt);  // fadds
    ctx.fpr[1] = t;
    const u32 p = load<u32>(slot + 8);
    store<u32>(o + 32, W48(t));
    if (p == 0 || (load<u32>(p + 192) & 8u) == 0 || load<u32>(o + 16) == 0) {
        ret(ctx, 0u);
        return;
    }
    const u32 a = load<u32>(p + 380);
    if (a == 0) {
        ret(ctx, 0u);
        return;
    }
    const u32 mask = static_cast<u32>(VcOE48(ctx, a, 64, {a, load<u32>(o + 16)}));
    if (mask == 0) {
        ret(ctx, 0u);
        return;
    }
    const u32 b = load<u8>(load<u32>(kGameRoot48) + load<u32>(o + 12) * kRec + 4208 + 1);
    const u32 e = 0x010B19E0u + b * 11968 + 176;
    ret(ctx, (load<u32>(e + 8) & mask & load<u32>(e + 12) & mask) != 0 ? 1u : 0u);
}

// u32 f(Obj* o): destructor. Vtables 0x00B21178 (+0) / 0x00B21194 (+12);
// the registry link +12 is unlinked (Registry_Unlink_0048F298(.., 0)); the
// body +28 is removed from its world (+8) and released; each of the u16
// +40 40-byte entries of +32 gets vt[40](x, 0) on its +32 and +36; the
// effect +24 is destroyed and freed (140 bytes) and cleared; then the
// member destructors of +52, the vector +32, the link +12 (unlink 2) and
// the base (VtB20CC8_DtorTrivial_0042CA50), whose result is r3.
DECOMP(00436374, Obj_Destruct_00436374) {
    const u32 o = arg<u32>(ctx, 0);
    store<u32>(o, 0x00B21178u);
    store<u32>(o + 12, 0x00B21194u);
    call(ctx, 0x0048F298u, {o + 12, 0});
    const u32 b = load<u32>(o + 28);
    if (b != 0) {
        const u32 w = load<u32>(b + 8);
        u32 q = b;
        if (w != 0) {
            call(ctx, 0x00931F00u, {w, b});
            q = load<u32>(o + 28);
        }
        call(ctx, kSkelRelease49, {q});
    }
    for (u32 i = 0; i < load<u16>(o + 40); ++i) {
        const u32 x = load<u32>(load<u32>(o + 32) + 40 * i + 32);
        VcOE48(ctx, x, 40, {x, 0});
        const u32 y = load<u32>(load<u32>(o + 32) + 40 * i + 36);
        VcOE48(ctx, y, 40, {y, 0});
    }
    const u32 e = load<u32>(o + 24);
    if (e != 0) {
        call(ctx, 0x004C8E80u, {e});
        call(ctx, 0x00552420u, {e, 140});
        store<u32>(o + 24, 0);
    }
    call(ctx, 0x004A0E08u, {o + 52});
    call(ctx, 0x0054EC1Cu, {o + 32});
    call(ctx, 0x0054EC18u, {o + 32});
    call(ctx, 0x0048F6B0u, {o + 12});
    ret(ctx, static_cast<u32>(call(ctx, 0x0042CA50u, {o})));
}

// Spawn* f(Mgr* m, u32 a, Src* s, const Params* p, u8 b): takes a record off
// m's free list (+8192, linked by +120; r3 = 0 when empty or s / p is
// null), clears it (its holders +76 / +80 get vt[40](h, 0)), links it to s
// (+0; s->+20 counts users), byte +96 = b, copies p to +16
// (SpawnParams_Copy_00442FB8), +104 = a, inserts it into m's priority list
// (PrioList_Insert445F08) and calls 0x00445A40(m; 0.0f, the constant at
// 0x00446194). r3 = the record.
DECOMP(00446198, Spawn_Acquire_00446198) {
    const u32 m = arg<u32>(ctx, 0);
    const u32 a = arg<u32>(ctx, 1);
    const u32 s = arg<u32>(ctx, 2);
    const u32 p = arg<u32>(ctx, 3);
    const u32 b = arg<u32>(ctx, 4);
    if (s == 0 || p == 0) {
        ret(ctx, 0u);
        return;
    }
    const u32 e = load<u32>(m + 8192);
    if (e == 0) {
        ret(ctx, 0u);
        return;
    }
    store<u32>(m + 8192, load<u32>(e + 120));
    store<u32>(e, 0);
    store<u32>(e + 4, 0);
    for (u32 off : {76u, 80u}) {
        const u32 h = load<u32>(e + off);
        if (h != 0) VcOE48(ctx, h, 40, {h, 0});
        store<u32>(e + off, 0);
    }
    store<u32>(e + 72, 0);
    for (u32 k = 96; k <= 101; ++k) store<u8>(e + k, 0);
    const f64 k0 = Lfs48(0x00446194u);
    store<u32>(e + 112, 0);
    store<u32>(e + 104, 0);
    store<u32>(e + 116, 0);
    store<u32>(e + 120, 0);
    store<u32>(s + 20, load<u32>(s + 20) + 1);
    store<u32>(e, s);
    store<u8>(e + 96, static_cast<u8>(b));
    call(ctx, 0x00442FB8u, {e + 16, p});
    store<u32>(e + 104, a);
    call(ctx, 0x00445F08u, {m, e});
    callf(ctx, 0x00445A40u, {m}, {k0});
    ret(ctx, e);
}

// u32 f(CmdList* o): runs the pending commands. For each of the u16 +6
// 8-byte entries of +12 whose command (+4) has bit 0x80 of byte +88 and
// either answers vt[20]() or has bit 0x20: bit 0x08 is set, its +24
// targets (array +20) are taken (+24 = 0) and each one with bit 0x40000
// but not bit 1 of +172 gets vt[40](t, 1) (the command's +32 counting
// down), then vt[28](cmd, 0). r3 = the entry count.
DECOMP(00454E88, CmdList_RunPending_00454E88) {
    const u32 o = arg<u32>(ctx, 0);
    for (u32 i = 0;; ++i) {
        const u32 cnt = load<u16>(o + 6);
        if (i >= cnt) {
            ret(ctx, cnt);
            return;
        }
        const u32 e = load<u32>(load<u32>(o + 12) + 8 * i + 4);
        if ((load<u8>(e + 88) & 0x80u) == 0) continue;
        if (static_cast<u32>(VcOE48(ctx, e, 20, {e})) == 0 && (load<u8>(e + 88) & 0x20u) == 0) continue;
        const u8 b88 = load<u8>(e + 88);
        const s32 n = static_cast<s32>(load<u32>(e + 24));
        store<u32>(e + 24, 0);
        store<u8>(e + 88, static_cast<u8>(b88 | 8u));
        for (s32 k = 0; k < n; ++k) {
            const u32 arr = load<u32>(e + 20);
            const u32 t = load<u32>(arr + 4 * static_cast<u32>(k));
            const u32 f = load<u32>(t + 172);
            if ((f & 0x40000u) == 0 || (f & 1u) != 0) continue;
            const u32 t2 = load<u32>(arr + 4 * static_cast<u32>(k));
            store<u32>(e + 32, load<u32>(e + 32) - 1);
            VcOE48(ctx, t2, 40, {t2, 1});
        }
        VcOE48(ctx, e, 28, {e, 0});
    }
}

// ---------------------------------------------------------------- sweep: 109 instructions
// Obj* f(?, XmlCursor* x): the 64-byte XML command object with vtable
// 0x00B0FAB8: +12..+32 = 0, vec3 +48 = 0x01164B40. Hashes +16 and +12
// (default 0); unless both are found, hashes +24 / +20 / +28, the vec3
// +48 and the hash +32 are read too. r3 = the object (0 when the
// allocation failed).
DECOMP(0007CA20, XmlCmd_Create_0007CA20) {
    const u32 x = arg<u32>(ctx, 1);  // r3 unused
    const u32 m = NewXmlObj47(ctx, 64, x);
    if (m == 0) {
        ret(ctx, 0u);
        return;
    }
    constexpr u32 kV = 0x01164B40u;
    store<u32>(m + 12, 0);
    store<u32>(m + 16, 0);
    store<u32>(m + 20, 0);
    store<u32>(m, 0x00B0FAB8u);
    Cp48(m + 48, kV);
    store<u32>(m + 24, 0);
    store<u32>(m + 28, 0);
    store<u32>(m + 32, 0);
    Cp48(m + 52, kV + 4);
    Cp48(m + 56, kV + 8);
    ctx.fpr[1] = Lfs48(kV + 8);
    if (call(ctx, kReadHash47, {x, 0x1D2A79EFu, m + 16, 0}) == 0 ||
        call(ctx, kReadHash47, {x, 0x2D8D394Bu, m + 12, 0}) == 0) {
        RdHash47(ctx, x, 0x6EC1B216u, m + 24);
        RdHash47(ctx, x, 0x7F247172u, m + 20);
        RdHash47(ctx, x, 0xB61AB1C6u, m + 28);
        call(ctx, 0x004F9E0Cu, {x, 0x4AB49083u, m + 48, m + 48});
        RdHash47(ctx, x, 0x8A7E47A4u, m + 32);
    }
    ret(ctx, m);
}

// Obj* f(?, XmlCursor* x): the 36-byte XML command object with vtable
// 0x00B11370: hashes +12 (default 0), +16, +20, floats +24 (0) and +28
// (1.0), bools +32 (1), +33 (0), +34 (1). r3 = the object (0 when the
// allocation failed).
DECOMP(0009EEBC, XmlCmd_Create_0009EEBC) {
    const u32 x = arg<u32>(ctx, 1);  // r3 unused
    const u32 m = NewXmlObj47(ctx, 36, x);
    if (m == 0) {
        ret(ctx, 0u);
        return;
    }
    store<u32>(m + 12, 0);
    store<u32>(m + 16, 0);
    store<u32>(m + 20, 0);
    store<u32>(m + 24, 0);
    store<u32>(m + 28, 0x3F800000u);
    store<u32>(m, 0x00B11370u);
    store<u8>(m + 32, 1);
    store<u8>(m + 33, 0);
    store<u8>(m + 34, 1);
    call(ctx, kReadHash47, {x, 0x7A30D7E3u, m + 12, 0});
    RdHash47(ctx, x, 0xBC11E765u, m + 16);
    RdHash47(ctx, x, 0x001EBA11u, m + 20);
    RdF32_47(ctx, x, 0x40156B55u, m + 24);
    RdF32_47(ctx, x, 0x04B70827u, m + 28);
    RdBool47(ctx, x, 0x026F1EA5u, m + 32);
    RdBool47(ctx, x, 0x922FF61Au, m + 33);
    RdBool47(ctx, x, 0x8488565Bu, m + 34);
    ret(ctx, m);
}

// u32 f(Def* o, Obj* x): issues o's command. The target hash is +52, or,
// when x has the component +56 in state 2 (vt[32]), that component's
// vt[24](). With a command +48 and either a hash or a +60: the hash is
// looked up in the point map (*0x00F8F154)->+16 (its name fetched when
// missing); with a point found or a +60, an 80-byte command is made
// (SmallAlloc; VtB21328_Ctor_00439224(p, +48, point, +60, +64, o + 16,
// o + 32, b68, b69, b70)) and pushed (CmdList_Push_0042C138, null when the
// allocation failed). r3 = the push result, else 0.
DECOMP(000A5D54, CmdDef_Issue_000A5D54) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 x = arg<u32>(ctx, 1);
    u32 h = load<u32>(o + 52);
    const u32 k = load<u32>(o + 56);
    if (k != 0) {
        const u32 c = static_cast<u32>(call(ctx, kFindComp48, {x, k}));
        if (c != 0 && static_cast<u32>(VcOE48(ctx, c, 32, {c})) == 2) h = static_cast<u32>(VcOE48(ctx, c, 24, {c}));
    }
    if (load<u32>(o + 48) == 0 || (h == 0 && load<u32>(o + 60) == 0)) {
        ret(ctx, 0u);
        return;
    }
    u32 pt = 0;
    if (h != 0) {
        Stack fr(ctx, 16);  // sp+128: the key
        store<u32>(fr.addr(), h);
        const u32 map = load<u32>(load<u32>(0x00F8F154u) + 16);
        pt = load<u32>(static_cast<u32>(call(ctx, 0x0073343Cu, {map, fr.addr()})));
        if (pt == 0) call(ctx, kHashStr48, {h});
    }
    if (pt == 0 && load<u32>(o + 60) == 0) {
        ret(ctx, 0u);
        return;
    }
    u32 p = static_cast<u32>(call(ctx, 0x0055238Cu, {80}));
    if (p != 0) {
        const u32 a48 = load<u32>(o + 48), a60 = load<u32>(o + 60), a64 = load<u32>(o + 64);
        const u32 b68 = load<u8>(o + 68), b69 = load<u8>(o + 69), b70 = load<u8>(o + 70);
        call(ctx, 0x00439224u, {p, a48, pt, a60, a64, o + 16, o + 32, b68, b69, b70});
    }
    ret(ctx, static_cast<u32>(call(ctx, 0x0042C138u, {p})));
}

// u32 f(Obj* o, u8 on): with on and a skeleton +432: removes it from its
// Havok world (+8; Hk_RemoveEntity92FFC8, result byte sp+128), releases
// each bone (Skel_BoneCount / Skel_BoneAt), strips the properties
// 0xA734004D and 0x91C7CF5B (Hk_RemoveProperty93FED8, results sp+112 /
// sp+120) and releases the skeleton. Then Obj_SetRagdollBase(o, on), whose
// result is r3.
DECOMP(000C19C8, Obj_SetRagdoll_000C19C8) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 on = arg<u32>(ctx, 1) & 0xFFu;
    const u32 s0 = on != 0 ? load<u32>(o + 432) : 0u;
    if (s0 != 0) {
        Stack fr(ctx, 32);  // sp+112 / sp+120 / sp+128: Havok result bytes
        const u32 w = load<u32>(s0 + 8);
        if (w != 0) call(ctx, 0x0092FFC8u, {fr.addr() + 16, w, s0});
        for (s32 i = 0; i < static_cast<s32>(call(ctx, 0x0091A2CCu, {load<u32>(o + 432)})); ++i) {
            const u32 bone = static_cast<u32>(call(ctx, 0x0091A7D8u, {load<u32>(o + 432), static_cast<u32>(i)}));
            call(ctx, kSkelRelease49, {bone});
        }
        u32 s = load<u32>(o + 432);
        if (static_cast<s32>(load<u32>(s + 128)) > 0 && HkProp_Find49(s, 0xA734004Du, true) >= 0) {
            call(ctx, 0x0093FED8u, {fr.addr(), s, 0xA734004Du});
            s = load<u32>(o + 432);
        }
        if (HkProp_Find49(s, 0x91C7CF5Bu, false) >= 0) {
            call(ctx, 0x0093FED8u, {fr.addr() + 8, s, 0x91C7CF5Bu});
            s = load<u32>(o + 432);
        }
        call(ctx, kSkelRelease49, {s});
    }
    ret(ctx, static_cast<u32>(call(ctx, 0x000E02F8u, {o, on})));
}

// u32 f(Out* o, Actor* a): o->+8 = a->+684, which is scaled by 1.25 (the
// constants at 0x0016D14C); unless a->+348 is at most 0.0 with the anim
// +692 already 0x03DE9A54, the animator a->vt[60]() restarts 0xB94AF9E7
// (Anim_Restart2775A8(.., 0, 0, 0, -1, 0, 0, -, -, 0, -1; 1.0, 0.25, 0.0,
// 0.0)) and +692 = 0xB94AF9E7. Then a->vt[272]()'s component 0x04B70827,
// when in state 2 (vt[32]), gives v = vt[24](): o->+4 = v, comp->+20 =
// v + 1, r3 = v (else the first failing value). f1 as the original
// leaves it.
DECOMP(0016D160, Actor_ResetScale_0016D160) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 a = arg<u32>(ctx, 1);
    constexpr u32 K = 0x0016D14Cu;  // {1.25, 0.0, 1.0, 0.25}
    const f64 s = Lfs48(a + 684);
    Cp48(o + 8, a + 684);
    const f64 v684 = Fm49(s, Lfs48(K));
    store<u32>(a + 684, W48(v684));
    ctx.fpr[1] = v684;
    const f64 k4 = Lfs48(K + 4);
    const f64 f348 = Lfs48(a + 348);
    if (!((f348 < k4 || f348 == k4) && load<u32>(a + 692) == 0x03DE9A54u)) {
        const u32 an = static_cast<u32>(VcOE48(ctx, a, 60, {a}));
        // the 9th / 10th stack slots are leftovers the callee does not read
        callf(ctx, 0x002775A8u, {an, 0xB94AF9E7u, 0, 0, 0, ~0ull, 0, 0, 0, 0, 0, ~0ull}, {Lfs48(K + 8), Lfs48(K + 12), k4, k4});
        store<u32>(a + 692, 0xB94AF9E7u);
    }
    const u32 r = static_cast<u32>(VcOE48(ctx, a, 272, {a}));
    if (r == 0) {
        ret(ctx, 0u);
        return;
    }
    const u32 c = static_cast<u32>(call(ctx, kFindComp48, {r, 0x04B70827u}));
    if (c == 0) {
        ret(ctx, 0u);
        return;
    }
    const u32 st = static_cast<u32>(VcOE48(ctx, c, 32, {c}));
    if (st != 2) {
        ret(ctx, st);
        return;
    }
    const u32 v = static_cast<u32>(VcOE48(ctx, c, 24, {c}));
    store<u32>(o + 4, v);
    store<u32>(c + 20, v + 1);
    ret(ctx, v);
}

// T* f(a, b, Src* s, c): the 148-byte definition on the stack (sp+112: +12..
// +20 = s->+12..+20, -1.0 at +36 / +40 / +72, a 16-byte-element PtrVector
// at +76 (u16 +82 = 12, PtrVector_Init(.., 16, 0, 1)), two copies of the
// vec3 0x01164B40 at +112 / +128, bit 0x400 of +4, zeros elsewhere) is read
// by 0x001A54A8(a, b, s, c, &def), then a 224-byte object is made from it
// (SmallAlloc; VtB14FD8_Construct_001A44E0(p, &def); r3 = 0 on failure)
// and the vector destroyed. r3 = p.
DECOMP(001A5AD8, XmlDef_New224_001A5AD8) {
    const u32 a = arg<u32>(ctx, 0), b = arg<u32>(ctx, 1), s = arg<u32>(ctx, 2), c = arg<u32>(ctx, 3);
    Stack fr(ctx, 160);  // sp+112..259: the definition
    const u32 t = fr.addr();
    const u32 s12 = load<u32>(s + 12), s16 = load<u32>(s + 16), s20 = load<u32>(s + 20);
    store<u32>(t + 12, s12);
    store<u32>(t + 16, s16);
    store<u32>(t + 20, s20);
    for (u32 k = 24; k < 32; ++k) store<u8>(t + k, 0);
    store<u32>(t + 32, 0);
    store<u32>(t + 36, 0xBF800000u);
    store<u32>(t + 40, 0xBF800000u);
    store<u8>(t + 44, 0);
    for (u32 k = 48; k < 72; k += 4) store<u32>(t + k, 0);
    store<u32>(t + 72, 0xBF800000u);
    store<u32>(t, 0);
    store<u32>(t + 4, 0);
    store<u32>(t + 8, 0);
    const u32 v = t + 76;
    call(ctx, 0x005516E0u, {v});
    store<u16>(t + 82, 12);
    call(ctx, 0x00551598u, {v, 16, 0, 1});
    store<u32>(t + 96, 0);
    store<u32>(t + 100, 0);
    constexpr u32 kV = 0x01164B40u;
    const u32 f4 = load<u32>(t + 4) | 0x400u;
    for (u32 k = 0; k < 3; ++k) Cp48(t + 112 + 4 * k, kV + 4 * k);
    for (u32 k = 0; k < 3; ++k) Cp48(t + 128 + 4 * k, kV + 4 * k);
    store<u32>(t + 144, 0);
    store<u32>(t + 4, f4);
    ctx.fpr[1] = Lfs48(kV);  // f1..f3 hold the vec3
    // r8 as the original leaves it (the callee reads r3..r7)
    call(ctx, 0x001A54A8u, {a, b, s, c, t, f4});
    const u32 p = static_cast<u32>(call(ctx, 0x0055238Cu, {224}));
    if (p != 0) call(ctx, 0x001A44E0u, {p, t});
    call(ctx, 0x0054EC1Cu, {v});
    call(ctx, 0x0054EC18u, {v});
    ret(ctx, p);
}

// u32 f(Obj* o, Obj* x): unlinks x from o. With the debug byte 0x01056A08
// set both get vt[76](). x's key +168 is looked up in o's maps +364 (count
// u16 +366) and +388 (u16 +390) (SortedMap_Find759654 / Remove7597DC, key
// on the stack at sp+116 / sp+112): the first map's entry is detached from
// o when live (bit 8 of +192; Obj_Detach_0012E8B0) and both maps' entries
// get vt[40](e, 0) before removal. Then Obj_Detach_0012E8B0(x, o), whose
// result is r3.
DECOMP(0020F240, Obj_Unlink_0020F240) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 x = arg<u32>(ctx, 1);
    if (load<u8>(0x01056A08u) != 0) {
        VcOE48(ctx, o, 76, {o});
        VcOE48(ctx, x, 76, {x});
    }
    Stack fr(ctx, 16);  // sp+112 / sp+116: the keys
    u32 k = load<u32>(x + 168);
    store<u32>(fr.addr() + 4, k);
    if (load<u16>(o + 366) != 0) {
        const u32 m = o + 364;
        const u32 e = load<u32>(static_cast<u32>(call(ctx, 0x00759654u, {m, fr.addr() + 4})));
        if (e != 0) {
            if ((load<u32>(e + 192) & 8u) != 0) call(ctx, 0x0012E8B0u, {e, o});
            VcOE48(ctx, e, 40, {e, 0});
        }
        call(ctx, 0x007597DCu, {m, fr.addr() + 4});
        k = load<u32>(x + 168);
    }
    store<u32>(fr.addr(), k);
    if (load<u16>(o + 390) != 0) {
        const u32 m = o + 388;
        const u32 e = load<u32>(static_cast<u32>(call(ctx, 0x00759654u, {m, fr.addr()})));
        if (e != 0) VcOE48(ctx, e, 40, {e, 0});
        call(ctx, 0x007597DCu, {m, fr.addr()});
    }
    ret(ctx, static_cast<u32>(call(ctx, 0x0012E8B0u, {x, o})));
}

// u32 f(Obj* o, f32 dt): o's per-frame update unless bit 0x40 or 0x20 of
// (o->+276)->+172 is set (r3 = that word / 1): byte +337 = 0; the member
// +24's vt[16], 0x002C96A8, 0x002CA150, 0x002C3CD0 and
// StateMachine_Update(o + 624) all get dt; +200 += dt when the byte +193
// is set; +56 / +60 = +68 / +72; the top six bits of +52 come from +64; each
// of the 20 timers +536.. that has started (>= 0.0, the constant at
// 0x002CAA88) advances by dt, counting those below the limit 0x01066D20;
// when that count reaches the threshold 0x01066CD0, trophy 0x924F24D5 is
// awarded (Trophy_Award0B27B4). r3 / f1 as the original leaves them.
DECOMP(002CAA90, Obj_UpdateTimers_002CAA90) {
    const u32 o = arg<u32>(ctx, 0);
    const f64 dt = farg(ctx, 0);
    const u32 f = load<u32>(load<u32>(o + 276) + 172);
    if ((f & 0x40u) != 0) {
        ret(ctx, f);
        return;
    }
    if ((f & 0x20u) != 0) {
        ret(ctx, 1u);
        return;
    }
    store<u8>(o + 337, 0);
    ctx.fpr[1] = dt;
    VcOE48(ctx, o + 24, 16, {o + 24});
    callf(ctx, 0x002C96A8u, {o}, {dt});
    callf(ctx, 0x002CA150u, {o}, {dt});
    callf(ctx, 0x002C3CD0u, {o}, {dt});
    callf(ctx, 0x00464BD4u, {o + 624}, {dt});
    const u8 b193 = load<u8>(o + 193);
    const u32 f52 = load<u32>(o + 52), f64v = load<u32>(o + 64);
    const u32 v68 = load<u32>(o + 68), v72 = load<u32>(o + 72);
    if (b193 != 0) store<u32>(o + 200, W48(Lfs48(o + 200) + dt));
    store<u32>(o + 56, v68);
    store<u32>(o + 60, v72);
    store<u32>(o + 52, (f52 & 0x03FFFFFFu) | (f64v & 0xFC000000u));
    const f64 lim = Lfs48(0x01066D20u);
    const f64 k0 = Lfs48(0x002CAA88u);
    ctx.fpr[1] = lim;
    s32 cnt = 0;
    for (u32 i = 0; i < 20; ++i) {
        const u32 p = o + 536 + 4 * i;
        const f64 t = Lfs48(p);
        if (!(t > k0 || t == k0)) continue;
        if (lim > t) ++cnt;
        store<u32>(p, W48(t + dt));
    }
    const s32 n = static_cast<s32>(load<u32>(0x01066CD0u));
    if (n > cnt) {
        ret(ctx, static_cast<u32>(n));
        return;
    }
    ret(ctx, static_cast<u32>(call(ctx, 0x000B27B4u, {0x924F24D5u})));
}
