// Game code, part 49 (continued): the small-function sweep, 109 instructions
// and up. Each was checked against the original in the PPC simulator
// (memory writes, calls and their arguments, r3 and f1).
#include "obj49.h"

using namespace sly4;
using namespace sly4::obj49;

// Appends v to the PtrVector of 32-bit entries at l (+0 array, u16 +4 cap,
// +8 count, +10 high-water count), doubling it (PtrVector_Reserve) when
// full; returns the old high-water count (the original's r3).
static u32 PtrVec_Push49(PPUContext& ctx, u32 l, u32 v) {
    const u32 cap = load<u16>(l + 4);
    if (load<u16>(l + 8) == cap) call(ctx, 0x00551718u, {l, (cap << 1) & 0x1FFFEu});
    store<u32>(load<u32>(l) + ((static_cast<u32>(load<u16>(l + 8)) << 2) & 0x3FFFCu), v);
    const u32 n = load<u16>(l + 8) + 1u;
    const u32 hi = load<u16>(l + 10);
    store<u16>(l + 8, static_cast<u16>(n));
    store<u16>(l + 10, static_cast<u16>(static_cast<s32>(hi) > static_cast<s32>(n & 0xFFFFu) ? hi : n));
    return hi;
}

// ---------------------------------------------------------------- sweep: 109 instructions
// u32 f(List* l, u32 a, f32 s0, f32 s1, -, -, u32 b, const Mtx* m, u8 c):
// pushes a new 112-byte record (SmallAlloc; null when that fails) onto l's
// vector +72: {l, a, m's 3x4 rows at +16.., +80 = +92 = 0, s0 / s1 at +84 /
// +88 (1.0 when <= 0.0; constants 0x0030BE3C), b at +96, byte +100 = c}.
// r3 = the vector's old high-water count; f1 as the original leaves it.
DECOMP(0030BE48, Track_PushRecord_0030BE48) {
    const u32 l = arg<u32>(ctx, 0);
    const u32 a = arg<u32>(ctx, 1);
    const u32 b = arg<u32>(ctx, 4);
    const u32 m = arg<u32>(ctx, 5);
    const u32 c = arg<u32>(ctx, 6);
    f64 s0 = farg(ctx, 0), s1 = farg(ctx, 1);
    constexpr u32 K = 0x0030BE3Cu;  // {0.0, 1.0}
    const f64 k0 = Lfs48(K);
    ctx.fpr[1] = k0;
    if (s0 < k0 || s0 == k0) s0 = Lfs48(K + 4);
    if (s1 < k0 || s1 == k0) s1 = Lfs48(K + 4);
    const u32 p = static_cast<u32>(call(ctx, 0x0055238Cu, {112}));
    if (p != 0) {
        store<u32>(p, l);
        store<u32>(p + 4, a);
        for (u32 row = 0; row < 3; ++row)
            for (u32 k = 0; k < 3; ++k) Cp48(p + 16 + 16 * row + 4 * k, m + 16 * row + 4 * k);
        Cp48(p + 64, m + 48);
        Cp48(p + 68, m + 52);
        ctx.fpr[1] = Lfs48(m + 56);
        store<u32>(p + 80, 0);
        Cp48(p + 72, m + 56);
        store<u32>(p + 84, W48(s0));
        store<u32>(p + 92, 0);
        store<u32>(p + 88, W48(s1));
        store<u32>(p + 96, b);
        store<u8>(p + 100, static_cast<u8>(c));
    }
    ret(ctx, PtrVec_Push49(ctx, l + 72, p));
}

// u32 f(Mode* o): resets o's mega object (+32). The group +68 is released
// (Group_ReleaseById_0042C00C) and replaced by the scene hash 0xEFA2D5A9
// (0x9D32BD2E when +64 is 0); Mega_PostReset_00304298(mega); each of its
// three parts +776.. gets Obj_ReleaseParts1F4460 and vt[88](p, 1, 0, 0,
// 0); Field_SetByteClear4_004A090C(mega + 640, 0); each part then gets
// Obj_FlushEntries_001ED9D4; the root starts the scene
// (World_StartScene46C3B8, all else 0) and o->vt[28](o, 0, 2) gives r3.
DECOMP(003198FC, Mode_ResetMega_003198FC) {
    const u32 o = arg<u32>(ctx, 0);
    call(ctx, 0x0042C00Cu, {load<u32>(o + 68)});
    const u32 mega = load<u32>(o + 32);
    const u32 h = load<u32>(o + 64) != 0 ? 0xEFA2D5A9u : 0x9D32BD2Eu;
    store<u32>(o + 68, h);
    call(ctx, 0x00304298u, {mega});
    for (u32 i = 0; i < 3; ++i) {
        const u32 e = load<u32>(mega + 776 + 4 * i);
        if (e == 0) continue;
        call(ctx, 0x001F4460u, {e});
        const u32 e2 = load<u32>(mega + 776 + 4 * i);
        VcOE48(ctx, e2, 88, {e2, 1, 0, 0, 0});
    }
    call(ctx, 0x004A090Cu, {mega + 640, 0});
    for (u32 i = 0; i < 3; ++i) {
        const u32 e = load<u32>(mega + 776 + 4 * i);
        if (e != 0) call(ctx, 0x001ED9D4u, {e});
    }
    call(ctx, 0x0046C3B8u, {load<u32>(kGameRoot48), h, 0, 0, 0, 0, 0, 0, 0, 0});
    ret(ctx, static_cast<u32>(VcOE48(ctx, o, 28, {o, 0, 2})));
}

// u32 f(const Cell* c): the cell's table entries (as in Cell_ApplyTable_
// 003965A0: rows s16 c->+4, columns s16 c->+2 of the 128-byte records of
// (*0x00F65038)->+24, the column's selector bytes at +u16 0x0106CF7E, the
// row's 4-byte slots at +u16 0x0106CF70): slot 0 picks the 11968-byte
// entry 0x010B19E0 + 176 + 11968 * n, slots 1 / 2 give two floats, and
// Obj_Call563000_00464488(entry; f1, f2) is called. r3 = 0.
DECOMP(00395EB4, Cell_ApplyEntry_00395EB4) {
    const u32 c = arg<u32>(ctx, 0);
    const u32 tab = load<u32>(0x00F65038u);
    const s32 i = static_cast<s16>(load<u16>(c + 4));
    const s32 j = static_cast<s16>(load<u16>(c + 2));
    auto row = [&]() -> u32 { return i != -1 ? load<u32>(tab + 24) + (static_cast<u32>(i) << 7) : 0u; };
    auto sel = [&]() -> u32 {
        const u32 col = j != -1 ? load<u32>(tab + 24) + (static_cast<u32>(j) << 7) : 0u;
        return load<u8>(col + 2) != 0 ? col + load<u16>(0x0106CF7Eu) : 0u;
    };
    u32 r = row();
    u32 s = sel();
    const u32 off = load<u16>(0x0106CF70u);
    const u32 n = load<u32>(r + off + (static_cast<u32>(load<u8>(s)) << 2));
    const u32 e = 0x010B19E0u + n * 11968 + 176;
    r = row();
    s = sel();
    const f64 f1 = Lfs48(r + off + (static_cast<u32>(load<u8>(s + 1)) << 2));
    r = row();
    s = sel();
    const f64 f2 = Lfs48(r + off + (static_cast<u32>(load<u8>(s + 2)) << 2));
    callf(ctx, 0x00464488u, {e}, {f1, f2});
    ret(ctx, 0u);
}

// u32 f(Obj* o, Obj* b): with b, its Havok body b->vt[144]() (when the
// world root+0x1281D4 and its +4 exist) takes o's motion (HkBody_SetMotion
// 91A228(body, o + 56), 0x0091AE28(body, o + 60)) and loses the properties
// 0x0026E1FE and 0x516CC502 (Hk_RemoveProperty93FED8, results sp+112 /
// sp+120); then b->vt[40](b, 1), whose result is r3 (o without b).
DECOMP(003A5108, Obj_SyncBody_003A5108) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 b = arg<u32>(ctx, 1);
    if (b == 0) {
        ret(ctx, o);
        return;
    }
    const u32 hb = static_cast<u32>(VcOE48(ctx, b, 144, {b}));
    const u32 w = hb != 0 ? load<u32>(load<u32>(kGameRoot48) + 0x001281D4u) : 0u;
    if (w != 0 && load<u32>(w + 4) != 0) {
        Stack fr(ctx, 16);  // sp+112 / sp+120: Havok result bytes
        call(ctx, 0x0091A228u, {hb, o + 56});
        call(ctx, 0x0091AE28u, {hb, o + 60});
        if (static_cast<s32>(load<u32>(hb + 128)) > 0 && HkProp_Find49(hb, 0x0026E1FEu, true) >= 0)
            call(ctx, 0x0093FED8u, {fr.addr(), hb, 0x0026E1FEu});
        if (static_cast<s32>(load<u32>(hb + 128)) > 0 && HkProp_Find49(hb, 0x516CC502u, true) >= 0)
            call(ctx, 0x0093FED8u, {fr.addr() + 8, hb, 0x516CC502u});
    }
    ret(ctx, static_cast<u32>(VcOE48(ctx, b, 40, {b, 1})));
}

// u32 f(Obj* o): nudges o along its spline. The step is |+48 - +16|
// (nothing below 0.05, at most 1.0; constants 0x003D038C); the spline
// tangent at +104 (SplineInstance_TangentAt(o->+108; t, &v, 0)) is flipped
// when it points away from the step and scaled by it; with a live world
// (root+0x1281E0), o is placed (Obj_SetSplinePos3CCC68(o, &p)) at
// (o->+4)->+144 + 0.1 * v * k, k = 1.4 with the byte +94 set, else the f32
// +8. r3 / f1 as the original leaves them.
DECOMP(003D03A0, Obj_NudgeAlongSpline_003D03A0) {
    const u32 o = arg<u32>(ctx, 0);
    constexpr u32 K = 0x003D038Cu;  // {0.05, 1.0, 0.0, 0.1, 1.4}
    const f64 dy = Fs49(Lfs48(o + 52), Lfs48(o + 20));
    const f64 dx = Fs49(Lfs48(o + 48), Lfs48(o + 16));
    const f64 dz = Fs49(Lfs48(o + 56), Lfs48(o + 24));
    f64 d = Sqrt49(Fma49(dz, dz, Fma49(dx, dx, Fm49(dy, dy))));
    ctx.fpr[1] = Lfs48(K);
    if (d < Lfs48(K)) {
        ret(ctx, o);
        return;
    }
    if (d > Lfs48(K + 4)) d = Lfs48(K + 4);
    Stack fr(ctx, 32);  // sp+112: the tangent, sp+128: the new position
    const u32 v = fr.addr(), p = fr.addr() + 16;
    // r4 as the original leaves it (the constants' address; not read)
    callf(ctx, 0x005A8034u, {load<u32>(o + 108), K, v, 0}, {Lfs48(o + 104)});
    const f64 tx = Lfs48(v), ty = Lfs48(v + 4), tz = Lfs48(v + 8);
    const u32 obj = load<u32>(o + 4);
    f64 k = Lfs48(o + 8);
    const u32 b94 = load<u8>(o + 94);
    if (Fma49(tz, dz, Fma49(tx, dx, Fm49(ty, dy))) < Lfs48(K + 8)) d = -d;
    const f64 sx = Fm49(tx, d), sy = Fm49(ty, d), sz = Fm49(tz, d);
    store<u32>(v, W48(sx));
    store<u32>(v + 4, W48(sy));
    store<u32>(v + 8, W48(sz));
    ctx.fpr[1] = sy;
    if (load<u32>(load<u32>(kGameRoot48) + 0x001281E0u) == 0) {
        ret(ctx, b94);
        return;
    }
    const u32 pos = obj + 144;
    const f64 px = Lfs48(pos), py = Lfs48(pos + 4), pz = Lfs48(pos + 8);
    Cp48(p, pos);
    Cp48(p + 4, pos + 4);
    const f64 k12 = Lfs48(K + 12);
    if (b94 != 0) k = Lfs48(K + 16);
    const f64 nz = Fma49(Fm49(sz, k12), k, pz);
    const f64 ny = Fma49(Fm49(sy, k12), k, py);
    const f64 nx = Fma49(Fm49(sx, k12), k, px);
    store<u32>(p + 8, W48(nz));
    store<u32>(p + 4, W48(ny));
    store<u32>(p, W48(nx));
    ctx.fpr[1] = nx;
    ret(ctx, static_cast<u32>(call(ctx, 0x003CCC68u, {o, p})));
}

// u32 f(?, XmlCursor* x, Def* d): hashes d->+0..+12 (default 0), then for
// up to four children 0x2D8478AB of x (d advancing 80 bytes each), the
// hash 0xB2E59948 of each of their 0x811B12AF children into d->+16 + 4k.
// (The inner bound tests the outer counter against 20, as in the original,
// so it never stops the inner walk.) r3 = the last outer child (0 when the
// walk ran out).
DECOMP(00480574, Def_ReadSlots_00480574) {
    const u32 x = arg<u32>(ctx, 1);
    u32 d = arg<u32>(ctx, 2);
    call(ctx, kReadHash47, {x, 0xD86FA37Cu, d, 0});
    call(ctx, kReadHash47, {x, 0x6BDF55EFu, d + 4, 0});
    call(ctx, kReadHash47, {x, 0x9C990F5Du, d + 8, 0});
    call(ctx, kReadHash47, {x, 0x815D083Cu, d + 12, 0});
    u32 c = static_cast<u32>(call(ctx, 0x004F92BCu, {x, 0x2D8478ABu}));
    for (u32 k = 0; c != 0 && k < 4; ++k) {
        u32 e = static_cast<u32>(call(ctx, 0x004F92BCu, {c, 0x811B12AFu}));
        for (u32 q = d; e != 0 && k < 20; q += 4) {
            RdHash47(ctx, e, 0xB2E59948u, q + 16);
            e = static_cast<u32>(call(ctx, 0x004F8FC8u, {e, 0x811B12AFu}));
        }
        c = static_cast<u32>(call(ctx, 0x004F8FC8u, {c, 0x2D8478ABu}));
        d += 80;
    }
    ret(ctx, c);
}

// u32 f(Range* o, XmlCursor* x): constructor of the random-range object
// (vtables 0x00B21FD8 then 0x00B22000; u16 +4 = 1; the PtrVector +8 with
// u16 +14 = 4, PtrVector_Init(.., 0, 0, 0); +28..+40 = 0): reads the float
// +28 (default 0.0, the constant at 0x004C0DF8); when that is missing,
// reads +36 / +40 and, when both are there, sets +28 = +36 + (+40 - +36) *
// rand16 / 65535 from the game's inline generator (seed doubleword
// 0x01164888, state word 0x01164890). r3 = the first failing reader's
// result (0 when +28 was read, 0x01160000 after drawing).
DECOMP(004C0E00, RandRange_Construct_004C0E00) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 x = arg<u32>(ctx, 1);
    constexpr u32 K = 0x004C0DF8u;  // {0.0, 1/65535}
    store<u16>(o + 4, 1);
    store<u32>(o, 0x00B21FD8u);
    call(ctx, 0x005516E0u, {o + 8});
    store<u16>(o + 14, 4);
    call(ctx, 0x00551598u, {o + 8, 0, 0, 0});
    const f64 k0 = Lfs48(K);
    store<u32>(o + 36, 0);
    store<u32>(o, 0x00B22000u);
    store<u32>(o + 40, 0);
    store<u32>(o + 28, 0);
    store<u32>(o + 32, 0);
    callf(ctx, kReadF32_47, {x, 0x00274ACDu, o + 28}, {k0});
    if (static_cast<u32>(ctx.gpr[3]) == 0) {
        ret(ctx, 0u);
        return;
    }
    for (u32 h : {0xDC7AD045u, 0xDC7ACF57u}) {
        callf(ctx, kReadF32_47, {x, h, h == 0xDC7AD045u ? o + 36 : o + 40}, {k0});
        const u32 r = static_cast<u32>(ctx.gpr[3]);
        if (r != 0) {
            ret(ctx, r);
            return;
        }
    }
    constexpr u32 kSeed = 0x01164888u, kState = 0x01164890u;
    const f64 lo = Lfs48(o + 36);
    const f64 span = Fs49(Lfs48(o + 40), lo);
    const f64 rnd = F48(static_cast<f64>(static_cast<s64>(load<u64>(kSeed))));  // fcfid + frsp
    u32 w = load<u32>(kState);
    w = std::rotl(w, 4) + std::rotl(w, 16);
    w ^= 0x87654321u;
    store<u32>(kState, std::rotl(w, 16));
    store<u64>(kSeed, w & 0xFFFFu);
    const f64 v = Fma49(span, Fm49(rnd, Lfs48(K + 4)), lo);
    store<u32>(o + 28, W48(v));
    ctx.fpr[1] = v;
    ret(ctx, 0x01160000u);
}

// ---------------------------------------------------------------- sweep: 110 instructions
// u32 f(Finder* o, Obj* a): o->+4 = the first entry of the shared pool
// (*0x00F8F158; a 60-byte FreeList of 288-byte entries made on first use:
// vtable 0x00B0EFE8, +56 = 0x00ABFEA8, FreeList_Init(p, 288, holder->+4,
// 0, 0)) whose position (+80) lies within (a->+60)->+252 of
// (a->+60)->+64, walking the list from Obj_GetField4(pool) through each
// entry's +0 (entries are linked 16 bytes in). r3 = the entry (+16), or 0;
// f1 as the original leaves it.
DECOMP(0002B870, Pool_FindNear_0002B870) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 a = arg<u32>(ctx, 1);
    store<u32>(o + 4, 0);
    const u32 holder = load<u32>(0x00F8F158u);
    const u32 t = load<u32>(a + 60);
    u32 pool = load<u32>(holder);
    if (pool == 0) {
        const u32 m = static_cast<u32>(call(ctx, 0x0054ED38u, {60, 0, 16}));
        if (m != 0) {
            const u32 r5 = load<u32>(holder + 4);
            store<u32>(m + 8, 0);
            store<u32>(m + 4, 0);
            store<u32>(m + 12, 0);
            store<u32>(m + 20, 0);
            store<u32>(m + 16, 0);
            store<u32>(m + 24, 0);
            store<u32>(m + 32, 0);
            store<u32>(m + 28, 0);
            for (u32 off = 36; off <= 48; off += 4) store<u32>(m + off, 0);
            const u16 h54 = load<u16>(m + 54);
            store<u16>(m + 52, 0);
            store<u16>(m + 54, static_cast<u16>(h54 & 0x7FFFu));
            store<u32>(m, 0x00B0EFE8u);
            store<u32>(m + 56, 0x00ABFEA8u);
            // r8 / r9 as the original leaves them (the callee reads r3..r7)
            call(ctx, 0x00551A58u, {m, 288, r5, 0, 0, 0x00B0EFE8u, 0x00ABFEA8u});
        }
        pool = m;
        store<u32>(holder, m);
    }
    const u32 e0 = static_cast<u32>(call(ctx, 0x00556BF0u, {pool}));
    u32 q = e0 != 0 ? e0 + 16 : 0u;
    while (q != 0) {
        const f64 d = Dist49(q + 64, t + 64);
        ctx.fpr[1] = d;
        if (d < Lfs48(t + 252)) {
            store<u32>(o + 4, q);
            ret(ctx, q);
            return;
        }
        const u32 nx = load<u32>(q - 16);
        q = nx != 0 ? nx + 16 : 0u;
    }
    ret(ctx, 0u);
}

// Obj* f(?, XmlCursor* x): the 164-byte XML command object with vtable
// 0x00B10B48: the name attribute 0x00AC5668 (when present and non-empty)
// copied to +12 (128 bytes, String_Copy), hashes +144 / +148 (0), floats
// +152 / +156 (-1.0), bools +160 (0) / +161 (1). r3 = the object (0 when
// the allocation failed).
DECOMP(00095F78, XmlCmd_Create_00095F78) {
    const u32 x = arg<u32>(ctx, 1);  // r3 unused
    const u32 m = NewXmlObj47(ctx, 164, x);
    if (m == 0) {
        ret(ctx, 0u);
        return;
    }
    store<u32>(m + 144, 0);
    store<u32>(m + 148, 0);
    store<u8>(m + 160, 0);
    store<u8>(m + 12, 0);
    store<u32>(m, 0x00B10B48u);
    store<u32>(m + 152, 0xBF800000u);
    store<u32>(m + 156, 0xBF800000u);
    store<u8>(m + 161, 1);
    // r5 = 1 as the original leaves it (the callee reads r3 / r4)
    const u32 s = static_cast<u32>(call(ctx, 0x004F9628u, {x, 0x00AC5668u, 1}));
    if (s != 0 && load<u8>(s) != 0) call(ctx, 0x00553684u, {m + 12, s, 128});
    RdHash47(ctx, x, 0x7FAAD4A0u, m + 144);
    RdHash47(ctx, x, 0xFF457B34u, m + 148);
    RdF32_47(ctx, x, 0xFF478E71u, m + 152);
    RdF32_47(ctx, x, 0xE690CE6Fu, m + 156);
    RdBool47(ctx, x, 0xC79F1CEEu, m + 160);
    RdBool47(ctx, x, 0x260C4057u, m + 161);
    ret(ctx, m);
}

// u32 f(Obj* o, u32 g): moves o's body to collision group g. With a live
// Havok world (root+0x1281D4)->+4 and a body (Obj_GetBody_00252940 of
// o->vt[64]() - re-fetched before every use, as in the original): the top
// half of body->+44 becomes g (its low five bits kept, the rest cleared)
// and, when the body has an entity (+8), Hk_UpdateEntityGroup(world, body,
// 0, 1). Then Obj_SetGroup_000DA138(o, g), whose result is r3.
DECOMP(00124A04, Obj_SetBodyGroup_00124A04) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 g = arg<u32>(ctx, 1);
    const u32 w = load<u32>(load<u32>(load<u32>(kGameRoot48) + 0x001281D4u) + 4);
    auto body = [&]() -> u32 { return static_cast<u32>(call(ctx, 0x00252940u, {static_cast<u32>(VcOE48(ctx, o, 64, {o}))})); };
    if (w != 0 && VcOE48(ctx, o, 64, {o}) != 0 && body() != 0) {
        const u32 f = (load<u32>(body() + 44) & 0x1Fu) | (g << 16);
        store<u32>(body() + 44, f);
        if (load<u32>(body() + 8) != 0) call(ctx, 0x00930B70u, {w, body(), 0, 1});
    }
    ret(ctx, static_cast<u32>(call(ctx, 0x000DA138u, {o, g})));
}
