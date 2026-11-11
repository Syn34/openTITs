// Game code, medium sweep part 24: the remaining QueryEx / Obj_Init22 /
// Query_Init0CDD28 / Record_Init spawners of the kind part 23 decompiled.
#include "mid24.h"

using namespace sly4;
using namespace sly4::mid24;

namespace {
constexpr u32 kPtrVectorReserve = 0x00551718u;    // PtrVector_Reserve(v, n)
constexpr u32 kSmallAlloc = 0x0055238Cu;          // SmallAlloc(size)
constexpr u32 kSmallFree = 0x00552420u;           // SmallFree(p, size)
constexpr u32 kPtrVectorConstruct = 0x005516E0u;  // PtrVector_Construct(v)
constexpr u32 kPtrVectorClear = 0x0054EC1Cu;      // PtrVector_Clear(v)
constexpr u32 kNop = 0x0054EC18u;                 // Nop(...)
constexpr u32 kPtrVectorInit = 0x00551598u;       // PtrVector_Init(v, n, a, b)
constexpr u32 kQueryExConstruct = 0x000DFDF8u;    // QueryEx_Construct_000DFDF8(q, 18 more)
constexpr u32 kObjInit22 = 0x000CDFB0u;           // Obj_Init22(q, 21 more; f1, f2)
constexpr u32 kQueryInitCDD28 = 0x000CDD28u;      // Query_Init0CDD28(q, 21 more; f1, f2)
constexpr u32 kRecordInit = 0x000E44E0u;          // Record_Init_000E44E0(q, v, o, ...)
constexpr u32 kVec4B40 = 0x01164B40u;             // f32[3] default vector copied into spawn queries
constexpr u64 kMinus1 = ~0ull;                    // lis -1 / ori: the sign-extended -1 the stack slot gets

// Clears and frees q's lazily allocated 20-byte PtrVector (+0).
void FreeVec0(PPUContext& ctx, u32 q) {
    if (const u32 v = load<u32>(q); v != 0) {
        call(ctx, kPtrVectorClear, {v});
        call(ctx, kNop, {v});
        call(ctx, kSmallFree, {load<u32>(q), 20});
    }
}

// PtrVector_Clear + Nop on the vector at v.
void ClearVec(PPUContext& ctx, u32 v) {
    call(ctx, kPtrVectorClear, {v});
    call(ctx, kNop, {v});
}

// The spawn-query epilogue: q's two record vectors (+164, +144) and its
// lazily allocated 20-byte PtrVector (+0) are cleared and freed.
void FreeQuery(PPUContext& ctx, u32 q) {
    ClearVec(ctx, q + 164);
    ClearVec(ctx, q + 144);
    FreeVec0(ctx, q);
}

// PtrVector_Construct(v), element size u16 v->6 = es, PtrVector_Init(v, n, 0, 1).
void InitVec(PPUContext& ctx, u32 v, u16 es, u32 n) {
    call(ctx, kPtrVectorConstruct, {v});
    store<u16>(v + 6, es);
    call(ctx, kPtrVectorInit, {v, n, 0, 1});
}

// The 22-argument query constructors (Obj_Init22, Query_Init0CDD28):
// fn(q, a, b, v, o, 0, vt, 0x01164BD0, 0x01164B30, s9, s10, s11, s12, 0, s14,
// 0, 0, 0, -1, -, -, 0; f1 = f2 = k).
void Init22(PPUContext& ctx, u32 fn, u32 q, u32 a, u32 b, u32 v, u32 o, u32 vt, u32 s9, u32 s10, u32 s11, u32 s12,
            u32 s14, f64 k) {
    callf(ctx, fn, {q, a, b, v, o, 0, vt, 0x01164BD0u, 0x01164B30u, s9, s10, s11, s12, 0, s14, 0, 0, 0, kMinus1, 0, 0, 0},
          {k, k});
}

// t->vfn[8](t, q); p = SmallAlloc(size), ctor(p, q) when allocated; then
// p->vfn[8](p) (on null too, as the original). Returns p.
u32 Spawn(PPUContext& ctx, u32 t, u32 q, u32 size, u32 ctor) {
    VCall(ctx, t, 8, {t, q});
    u32 r = 0;
    if (const u32 p = static_cast<u32>(call(ctx, kSmallAlloc, {size})); p != 0) {
        call(ctx, ctor, {p, q});
        r = p;
    }
    VCall(ctx, r, 8, {r});
    return r;
}

// The "light" Obj_Init22 query shared by 0021E620 / 002219A8 (K = the two
// floats before the function): ObjInit22(q, 1, 0, v, o, vt 0x00B12000, 0,
// 0, 1, 0, 1; f = K[0]), q->144 = 0, u8 q->148 = 1, q->149 = 0, q->150..153
// = u8 o->172..175, q->4 |= 0x00400000, a 76-byte-element PtrVector at
// q+156 (PtrVector_Init(v, 16, 0, 1)), q->176 / 180 = o->176 / 180, the
// f32 q->184..196 = o->184..196 (K[1]), q->200 = o->200 (a null o reads 0
// where no K is given).
void LightQuery(PPUContext& ctx, u32 q, u32 v, u32 o, u32 K) {
    Init22(ctx, kObjInit22, q, 1, 0, v, o, 0x00B12000u, 0, 0, 1, 0, 1, Lf(K));
    store<u32>(q + 144, 0);
    store<u8>(q + 148, 1);
    store<u8>(q + 149, 0);
    const u32 w4 = load<u32>(q + 4);
    for (u32 k = 0; k < 4; ++k) store<u8>(q + 150 + k, o != 0 ? load<u8>(o + 172 + k) : 0);
    store<u32>(q + 4, w4 | 0x00400000u);
    InitVec(ctx, q + 156, 76, 16);
    store<u32>(q + 176, o != 0 ? load<u32>(o + 176) : 0u);
    store<u32>(q + 180, o != 0 ? load<u32>(o + 180) : 0u);
    for (u32 k = 0; k < 4; ++k) Cw(q + 184 + 4 * k, o != 0 ? o + 184 + 4 * k : K + 4);
    store<u32>(q + 200, o != 0 ? load<u32>(o + 200) : 0u);
}

// The Query_Init0CDD28 query shared by 000F4B40 / 000F27D0 / 000EAC88 /
// 000F39D0 (K = the floats before the function): v = o->vfn[32](o, t);
// Query_Init0CDD28(q, 0, 0, v, o, vt 0x00B12CC8, o->372 bit 31, o->372 bit
// 30, 0, 1, 1; f = K[0]), q->4 |= 1, 76-byte-element PtrVectors at q+144
// and q+164 (PtrVector_Init(v, 16, 0, 1)), f32 q->184 = o->140 (K[1]),
// q->188 / 192 = o->148 / 152 (0), q->4 |= 2, u8 q->196 = u16 o->188 bit
// 0x8000 (1), the default vector at q+208 and q+224, q->240 / 244 = 0, f32
// q->248 = o->196 (K[0]). Returns q->4 as it was before the |= 2.
u32 DoorQuery(PPUContext& ctx, u32 q, u32 o, u32 t, u32 K) {
    const u32 v = static_cast<u32>(VCall(ctx, o, 32, {o, t}));
    const u32 w = load<u32>(o + 372);
    Init22(ctx, kQueryInitCDD28, q, 0, 0, v, o, 0x00B12CC8u, (w >> 31) & 1u, (w >> 30) & 1u, 0, 1, 1, Lf(K));
    store<u32>(q + 4, load<u32>(q + 4) | 1u);
    InitVec(ctx, q + 144, 76, 16);
    InitVec(ctx, q + 164, 76, 16);
    const u32 w4 = load<u32>(q + 4);
    Cw(q + 184, o != 0 ? o + 140 : K + 4);
    store<u32>(q + 188, o != 0 ? load<u32>(o + 148) : 0u);
    store<u32>(q + 192, o != 0 ? load<u32>(o + 152) : 0u);
    store<u32>(q + 4, w4 | 2u);
    store<u8>(q + 196, o != 0 ? static_cast<u8>((load<u16>(o + 188) >> 15) & 1u) : 1);
    Cw(q + 208, kVec4B40);
    Cw(q + 212, kVec4B40 + 4);
    Cw(q + 216, kVec4B40 + 8);
    Cw(q + 224, kVec4B40);
    store<u32>(q + 240, 0);
    Cw(q + 228, kVec4B40 + 4);
    store<u32>(q + 244, 0);
    Cw(q + 232, kVec4B40 + 8);
    Cw(q + 248, o != 0 ? o + 196 : K);
    return w4;
}

// The DoorQuery spawners' common tail: f32 q->252 = o->220 (K[2]), q->4 =
// w4 | bits, then Spawn(size, ctor). f1 = the q->252 value.
u32 DoorFinish(PPUContext& ctx, u32 q, u32 o, u32 t, u32 K, u32 w4, u32 bits, u32 size, u32 ctor) {
    const u32 s = o != 0 ? o + 220 : K + 8;
    Cw(q + 252, s);
    ctx.fpr[1] = Lf(s);
    store<u32>(q + 4, w4 | bits);
    return Spawn(ctx, t, q, size, ctor);
}
}  // namespace

// Spawner1B5120: QueryEx_Construct_000DFDF8(q, 4, 128, v, o, 0, vt
// 0x00B16098, 0x01164BD0, 0x01164B30, o->244 bit 31, o->244 bit 30, 0, 1, 0,
// 1, 0, 0, 0, -1), an 8-byte-element PtrVector at q+200 (PtrVector_Init(v,
// 4, 0, 1)) filled with the u16 o->200 pairs of o->192 (grown with
// PtrVector_Reserve(v, 2 * capacity) when full; high-water +210), q->8 |=
// 0x20000, f32 q->220 = o->232 (*0x001B5118 for a null o); a 608-byte
// object from 0x001B4A10(p, q); the vector and q's three are freed.
DECOMP(001B5120, Spawner1B5120) {
    const u32 o = arg<u32>(ctx, 0), t = arg<u32>(ctx, 1);
    const u32 v = static_cast<u32>(VCall(ctx, o, 32, {o, t}));
    Stack frame(ctx, 224);
    const u32 q = frame.addr();
    const u32 w = load<u32>(o + 244);
    call(ctx, kQueryExConstruct,
         {q, 4, 128, v, o, 0, 0x00B16098u, 0x01164BD0u, 0x01164B30u, (w >> 31) & 1u, (w >> 30) & 1u, 0, 1, 0, 1, 0, 0, 0, kMinus1});
    const u32 vec = q + 200;
    InitVec(ctx, vec, 8, 4);
    u32 cnt = load<u16>(q + 208);
    const u32 fsrc = o != 0 ? o + 232 : 0x001B5118u;
    ctx.fpr[1] = Lf(fsrc);
    const u32 w8 = load<u32>(q + 8);
    Cw(q + 220, fsrc);
    store<u32>(q + 8, w8 | 0x20000u);
    u32 prev = cnt;  // the count as last computed (r25), compared to the capacity
    for (u32 i = 0; i < load<u16>(o + 200); ++i) {
        const u32 src = load<u32>(o + 192) + 8 * i;
        if ((prev & 0xFFFFu) == load<u16>(q + 204)) {
            call(ctx, kPtrVectorReserve, {vec, (static_cast<u32>(load<u16>(q + 204)) << 1) & 0x1FFFEu});
            cnt = load<u16>(q + 208);
        }
        const u32 dst = load<u32>(vec) + ((cnt << 3) & 0x7FFF8u);
        Cw(dst, src);
        Cw(dst + 4, src + 4);
        ctx.fpr[1] = Lf(src + 4);
        const u32 c1 = static_cast<u32>(load<u16>(q + 208)) + 1;
        const u32 hi = load<u16>(q + 210);
        store<u16>(q + 208, static_cast<u16>(c1));
        u32 m;
        if (static_cast<s32>(hi) > static_cast<s32>(c1 & 0xFFFFu)) {
            m = hi;
            prev = c1 & 0xFFFFu;
        } else {
            m = c1;
            prev = c1;
        }
        store<u16>(q + 210, static_cast<u16>(m));
        cnt = c1;
    }
    const u32 r = Spawn(ctx, t, q, 608, 0x001B4A10u);
    ClearVec(ctx, vec);
    FreeQuery(ctx, q);
    ctx.gpr[3] = r;
}

// Spawner21E620: r3 = 0 when t->4 bit 0x00400000, u8 t->184 bit 0x04 and
// not t->4 bit 31; else LightQuery(K = 0x0021E614), q->4 |= 0x80000000; a
// 480-byte object from 0x0021E3F8(p, q), vtable 0x00B1A8B0; the vector at
// q+156 and q's PtrVector are freed.
DECOMP(0021E620, Spawner21E620) {
    const u32 o = arg<u32>(ctx, 0), t = arg<u32>(ctx, 1);
    const u32 tw = load<u32>(t + 4);
    if ((tw & 0x00400000u) != 0 && ((load<u8>(t + 184) >> 2) & 1u) != 0 && (tw & 0x80000000u) == 0) {
        ctx.gpr[3] = 0;
        return;
    }
    const u32 v = static_cast<u32>(VCall(ctx, o, 32, {o, t}));
    Stack frame(ctx, 224);
    const u32 q = frame.addr();
    constexpr u32 K = 0x0021E614u;
    LightQuery(ctx, q, v, o, K);
    ctx.fpr[1] = Lf(o != 0 ? o + 196 : K + 4);
    ctx.fpr[2] = Lf(o != 0 ? o + 192 : K + 4);
    store<u32>(q + 4, load<u32>(q + 4) | 0x80000000u);
    VCall(ctx, t, 8, {t, q});
    u32 r = 0;
    if (const u32 p = static_cast<u32>(call(ctx, kSmallAlloc, {480})); p != 0) {
        call(ctx, 0x0021E3F8u, {p, q});
        store<u32>(p, 0x00B1A8B0u);
        r = p;
    }
    VCall(ctx, r, 8, {r});
    ClearVec(ctx, q + 156);
    FreeVec0(ctx, q);
    ctx.gpr[3] = r;
}

// Spawner2219A8: r3 = 0 when t->4 bit 0x00400000, u8 t->184 bit 0x04 and
// not t->8 bit 0x01; else LightQuery(K = 0x002219A0), q->4 |= 0x80000000,
// q->8 |= 1; a 480-byte object from 0x0021E3F8(p, q), vtable 0x00B1AD20,
// and when its p->416 (h) is set: h->44 = 21, 0x009315F0(h->8, h, 0); the
// vector at q+156 and q's PtrVector are freed.
DECOMP(002219A8, Spawner2219A8) {
    const u32 o = arg<u32>(ctx, 0), t = arg<u32>(ctx, 1);
    if ((load<u32>(t + 4) & 0x00400000u) != 0 && ((load<u8>(t + 184) >> 2) & 1u) != 0 && (load<u32>(t + 8) & 1u) == 0) {
        ctx.gpr[3] = 0;
        return;
    }
    const u32 v = static_cast<u32>(VCall(ctx, o, 32, {o, t}));
    Stack frame(ctx, 224);
    const u32 q = frame.addr();
    constexpr u32 K = 0x002219A0u;
    LightQuery(ctx, q, v, o, K);
    ctx.fpr[1] = Lf(o != 0 ? o + 192 : K + 4);
    ctx.fpr[2] = Lf(o != 0 ? o + 196 : K + 4);
    store<u32>(q + 4, load<u32>(q + 4) | 0x80000000u);
    store<u32>(q + 8, load<u32>(q + 8) | 1u);
    VCall(ctx, t, 8, {t, q});
    u32 r = 0;
    if (const u32 p = static_cast<u32>(call(ctx, kSmallAlloc, {480})); p != 0) {
        call(ctx, 0x0021E3F8u, {p, q});
        const u32 h = load<u32>(p + 416);
        store<u32>(p, 0x00B1AD20u);
        if (h != 0) {
            store<u32>(h + 44, 21);
            call(ctx, 0x009315F0u, {load<u32>(h + 8), h, 0});
        }
        r = p;
    }
    VCall(ctx, r, 8, {r});
    ClearVec(ctx, q + 156);
    FreeVec0(ctx, q);
    ctx.gpr[3] = r;
}

// Spawner0F4B40: DoorQuery(K = 0x000F4B30), q->4 |= 0x22002 (with the 2);
// a 752-byte object from 0x000F4A08(p, q).
DECOMP(000F4B40, Spawner0F4B40) {
    const u32 o = arg<u32>(ctx, 0), t = arg<u32>(ctx, 1);
    Stack frame(ctx, 264);
    const u32 q = frame.addr();
    constexpr u32 K = 0x000F4B30u;
    const u32 w4 = DoorQuery(ctx, q, o, t, K);
    const u32 r = DoorFinish(ctx, q, o, t, K, w4, 0x22002u, 752, 0x000F4A08u);
    FreeQuery(ctx, q);
    ctx.gpr[3] = r;
}

// Spawner0F27D0: DoorQuery(K = 0x000F27C4), q->4 |= 0x6002; a 752-byte
// object from 0x000F2488(p, q).
DECOMP(000F27D0, Spawner0F27D0) {
    const u32 o = arg<u32>(ctx, 0), t = arg<u32>(ctx, 1);
    Stack frame(ctx, 264);
    const u32 q = frame.addr();
    constexpr u32 K = 0x000F27C4u;
    const u32 w4 = DoorQuery(ctx, q, o, t, K);
    const u32 r = DoorFinish(ctx, q, o, t, K, w4, 0x6002u, 752, 0x000F2488u);
    FreeQuery(ctx, q);
    ctx.gpr[3] = r;
}

// Spawner0EAC88: DoorQuery(K = 0x000EAC78), q->4 |= 0x2002; a 640-byte
// object from Obj_BaseCtor_000E9DD8(p, q) with vtables 0x00B12EC8 (+0),
// 0x00B13068 (+380), 0x00B13090 (+384).
DECOMP(000EAC88, Spawner0EAC88) {
    const u32 o = arg<u32>(ctx, 0), t = arg<u32>(ctx, 1);
    Stack frame(ctx, 264);
    const u32 q = frame.addr();
    constexpr u32 K = 0x000EAC78u;
    const u32 w4 = DoorQuery(ctx, q, o, t, K);
    const u32 s = o != 0 ? o + 220 : K + 8;
    Cw(q + 252, s);
    ctx.fpr[1] = Lf(s);
    store<u32>(q + 4, w4 | 0x2002u);
    VCall(ctx, t, 8, {t, q});
    u32 r = 0;
    if (const u32 p = static_cast<u32>(call(ctx, kSmallAlloc, {640})); p != 0) {
        call(ctx, 0x000E9DD8u, {p, q});
        store<u32>(p, 0x00B12EC8u);
        store<u32>(p + 380, 0x00B13068u);
        store<u32>(p + 384, 0x00B13090u);
        r = p;
    }
    VCall(ctx, r, 8, {r});
    FreeQuery(ctx, q);
    ctx.gpr[3] = r;
}

// Spawner0F39D0: DoorQuery(K = 0x000F39BC), f32 q->252 = o->220 (K[2]),
// q->256..272 = o->404..420 (K[2], K[3], K[2], K[4], K[2]), q->4 |= 0x12002;
// a 688-byte object from Obj_BaseCtor_000E9DD8(p, q) with vtables 0x00B13308
// (+0), 0x00B134A8 (+380), 0x00B134D0 (+384), the vector 0x01164B60 at
// p+640, K[4] at p+656 / 660 and q->256..272 at p+664..680.
DECOMP(000F39D0, Spawner0F39D0) {
    const u32 o = arg<u32>(ctx, 0), t = arg<u32>(ctx, 1);
    Stack frame(ctx, 296);
    const u32 q = frame.addr();
    constexpr u32 K = 0x000F39BCu;
    const u32 w4 = DoorQuery(ctx, q, o, t, K);
    Cw(q + 252, o != 0 ? o + 220 : K + 8);
    Cw(q + 256, o != 0 ? o + 404 : K + 8);
    Cw(q + 260, o != 0 ? o + 408 : K + 12);
    Cw(q + 264, o != 0 ? o + 412 : K + 8);
    Cw(q + 268, o != 0 ? o + 416 : K + 16);
    Cw(q + 272, o != 0 ? o + 420 : K + 8);
    ctx.fpr[1] = Lf(o != 0 ? o + 420 : K + 8);
    store<u32>(q + 4, w4 | 0x12002u);
    VCall(ctx, t, 8, {t, q});
    u32 r = 0;
    if (const u32 p = static_cast<u32>(call(ctx, kSmallAlloc, {688})); p != 0) {
        call(ctx, 0x000E9DD8u, {p, q});
        store<u32>(p, 0x00B13308u);
        store<u32>(p + 380, 0x00B134A8u);
        store<u32>(p + 384, 0x00B134D0u);
        Cw(p + 640, 0x01164B60u);
        Cw(p + 644, 0x01164B64u);
        Cw(p + 656, K + 16);
        Cw(p + 648, 0x01164B68u);
        Cw(p + 660, K + 16);
        for (u32 k = 0; k < 5; ++k) Cw(p + 664 + 4 * k, q + 256 + 4 * k);
        ctx.fpr[1] = Lf(q + 272);
        r = p;
    }
    VCall(ctx, r, 8, {r});
    FreeQuery(ctx, q);
    ctx.gpr[3] = r;
}

// Spawner1D5DC8: Record_Init_000E44E0(q, v, o, 0, 0x01164BD0, 0x01164B30, 1,
// 0, 0, 0, 0, 0), the vectors o->368 / o->384 at q+208 / q+224 (the default
// vector for a null o), f32 q->240..252 = o->400..412 (K = 0x001D5DC0: K[0],
// then K[1]), the 17 words o->420.. at q+256 (for a null o: {0, 0, 0, 1, 0,
// ...} staged at q+336 first, as the original), q->324 bits 31..30 = o->492
// bits 28..27 (0; the rest as it was), q->12 |= 1; a 624-byte object from
// 0x001D5C14(p, q).
DECOMP(001D5DC8, Spawner1D5DC8) {
    const u32 o = arg<u32>(ctx, 0), t = arg<u32>(ctx, 1);
    const u32 v = static_cast<u32>(VCall(ctx, o, 32, {o, t}));
    Stack frame(ctx, 416);
    const u32 q = frame.addr();
    call(ctx, kRecordInit, {q, v, o, 0, 0x01164BD0u, 0x01164B30u, 1, 0, 0, 0, 0, 0});
    constexpr u32 K = 0x001D5DC0u;
    for (u32 k = 0; k < 3; ++k) Cw(q + 208 + 4 * k, (o != 0 ? o + 368 : kVec4B40) + 4 * k);
    for (u32 k = 0; k < 3; ++k) Cw(q + 224 + 4 * k, (o != 0 ? o + 384 : kVec4B40) + 4 * k);
    Cw(q + 240, o != 0 ? o + 400 : K);
    Cw(q + 244, o != 0 ? o + 404 : K + 4);
    Cw(q + 248, o != 0 ? o + 408 : K + 4);
    Cw(q + 252, o != 0 ? o + 412 : K + 4);
    ctx.fpr[1] = Lf(o != 0 ? o + 412 : K + 4);
    ctx.fpr[2] = Lf(o != 0 ? o + 408 : K + 4);
    u32 src = o + 420;
    if (o == 0) {
        // The defaults go through a stack temporary (q+336..) first.
        src = q + 336;
        for (u32 k = 0; k < 17; ++k) store<u32>(src + 4 * k, k == 3 ? 1u : 0u);
    }
    for (u32 k = 0; k < 17; ++k) Cw(q + 256 + 4 * k, src + 4 * k);
    const u32 bits = o != 0 ? (load<u32>(o + 492) >> 27) & 3u : 0u;
    store<u32>(q + 324, (load<u32>(q + 324) & 0x3FFFFFFFu) | (bits << 30));
    store<u32>(q + 12, load<u32>(q + 12) | 1u);
    const u32 r = Spawn(ctx, t, q, 624, 0x001D5C14u);
    FreeQuery(ctx, q);
    ctx.gpr[3] = r;
}
