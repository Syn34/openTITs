// Game code, part 24: the continued sweep of small still-recompiled game
// functions (in size order, from 22 instructions up). Most have no known
// caller context yet, so their names describe what they do and carry the
// address. Each was checked against the original in the PPC simulator.
//
// Register notes: where the original leaves a value in r3..r9 or f1..f5
// that a caller could read, the value is set the same way, as the comments
// say ("r4 left ...").
#include "obj24.h"

using namespace sly4;
using namespace sly4::obj24;

// ---------------------------------------------------------------- sweep: 22 instructions
// f(T* o): Msg_Post(0, o->+12, o->+16, o->+20) when o->+16 is set, then
// Flag_Set8_40000000(o)
DECOMP(0042D1AC, Obj_PostAndFlag_0042D1AC) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 p = load<u32>(o + 16);
    if (p != 0) call(ctx, o24fn::Msg_Post, {0, load<u32>(o + 12), p, load<u32>(o + 20)});
    call(ctx, o24fn::Flag_Set8_40000000, {o});
}
// T::T(T* o, f32 x, u8 on): the 0x00B20CC8 base (o, on), +12 = x, +16 = 0,
// vtable 0x00B20D40 (r3 / r4 left as the vtable / 0)
DECOMP(0042D464, VtB20D40_Ctor) {
    const u32 o = arg<u32>(ctx, 0);
    const f64 x = farg(ctx, 0);
    call(ctx, o24fn::Obj_InitVt_B20CC8, {o, arg<u32>(ctx, 2) & 0xFFu});
    store<f32>(o + 12, static_cast<f32>(x));
    store<u32>(o + 16, 0);
    store<u32>(o, 0x00B20D40);
    ctx.gpr[4] = 0;
    ret(ctx, 0x00B20D40u);
}
// f(T* o): +32 = 0, +24 = *(f32*)0x010BA01C, +20 = 1.0f unless the u8 +9 is
// set, then Flag_Set8_40000000(o)
DECOMP(0042E0E0, Obj_Reset_0042E0E0) {
    const u32 o = arg<u32>(ctx, 0);
    store<u32>(o + 32, 0);
    const u32 b = load<u8>(o + 9);
    ctx.fpr[1] = F32Bits(Cp(o + 24, 0x010B9FD4 + 72));  // f1 left (the callee keeps it)
    if (b == 0) store<u32>(o + 20, 0x3F800000);
    call(ctx, o24fn::Flag_Set8_40000000, {o});
}
// T::T(T* o, f32 x): the 0x00B20CC8 base (o, 1), +12 = x, +16 = 0, vtable
// 0x00B20FC8 (r3 / r4 left as the vtable / 0)
DECOMP(00431EEC, VtB20FC8_Ctor) {
    const u32 o = arg<u32>(ctx, 0);
    const f64 x = farg(ctx, 0);
    call(ctx, o24fn::Obj_InitVt_B20CC8, {o, 1});
    store<f32>(o + 12, static_cast<f32>(x));
    store<u32>(o + 16, 0);
    store<u32>(o, 0x00B20FC8);
    ctx.gpr[4] = 0;
    ret(ctx, 0x00B20FC8u);
}
// T::T(T* o, u32 a): the 0x00B20CC8 base (o, 1), +12 = a, the u8 +16 = 0,
// vtable 0x00B21130 (r3 / r4 left as the vtable / 0)
DECOMP(00434CA8, VtB21130_Ctor) {
    const u32 o = arg<u32>(ctx, 0), a = arg<u32>(ctx, 1);
    call(ctx, o24fn::Obj_InitVt_B20CC8, {o, 1});
    store<u32>(o + 12, a);
    store<u8>(o + 16, 0);
    store<u32>(o, 0x00B21130);
    ctx.gpr[4] = 0;
    ret(ctx, 0x00B21130u);
}
// bool f(T* o): !0x0044D4C8(o->+16) when +16 is set, else
// !0x0044D2E0((s32)o->+12) (r4 left as the call's result)
DECOMP(0043AE04, Obj_IsNone_0043AE04) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 p = load<u32>(o + 16);
    u32 r;
    if (p != 0) r = static_cast<u32>(call(ctx, 0x0044D4C8, {p}));
    else r = static_cast<u32>(call(ctx, 0x0044D2E0, {static_cast<u64>(static_cast<s64>(load<s32>(o + 12)))}));
    ctx.gpr[4] = r;
    ret(ctx, r == 0 ? 1u : 0u);
}
// void f(Node* n): when the u8 +100 (linked) is set, unlink n from the
// list (+104 prev, +108 next; the head's prev link is *(T**)0x00F65B64),
// clear +100 and set the u8 0x00F65B68 (dirty). r4..r6 left.
DECOMP(00441CB8, List_Unlink_00441CB8) {
    const u32 n = arg<u32>(ctx, 0);
    const u32 linked = load<u8>(n + 100);
    if (linked == 0) {
        ctx.gpr[4] = 0;
        return;
    }
    u32 prev = load<u32>(n + 104);
    if (prev != 0) {
        store<u32>(prev + 108, load<u32>(n + 108));
        prev = load<u32>(n + 104);
    }
    const u32 next = load<u32>(n + 108);
    if (next != 0) store<u32>(next + 104, prev);
    else store<u32>(0x00F65B64, prev);
    store<u8>(n + 100, 0);
    store<u8>(0x00F65B68, 1);
    ctx.gpr[4] = 0;
    ctx.gpr[5] = 0x00F60000;
    ctx.gpr[6] = 1;
}
// void f(T* p): nothing for null; else 0x006D59C4(p->+0, 1) and Mem_Free
// (p->+4) when that is set
DECOMP(0044DBC4, Obj_Release_0044DBC4) {
    const u32 p = arg<u32>(ctx, 0);
    if (p == 0) return;
    call(ctx, 0x006D59C4, {load<u32>(p), 1});
    const u32 q = load<u32>(p + 4);
    if (q != 0) call(ctx, o24fn::Mem_Free, {q});
}
// void f(T* o, f32 a, f32 b): 0x00563000(o->+28, 0; a, b) when o->+28 is
// set and its u8 +148 too (f3 = b passed as left); otherwise f1 / f2 / f3
// are left as b / a / b.
DECOMP(00464488, Obj_Call563000_00464488) {
    const u32 o = arg<u32>(ctx, 0);
    const f64 a = farg(ctx, 0), b = farg(ctx, 1);
    const u32 p = load<u32>(o + 28);
    ctx.gpr[3] = p;
    if (p != 0) {
        const u32 f = load<u8>(p + 148);
        ctx.gpr[4] = f;
        if (f != 0) {
            callf(ctx, 0x00563000, {p, 0}, {a, b, b});
            return;
        }
    }
    ctx.fpr[1] = b;
    ctx.fpr[2] = a;
    ctx.fpr[3] = b;
}
// bool f(const T* o): o->+8 < 0.0f, or ((o->+16 >> 1) & 0x7FFF) == 0 and
// o->+4 >= o->+8 (f1 left as +8, f2 as the last value compared, r4 left)
DECOMP(004A0BB0, Obj_Test_004A0BB0) {
    const u32 o = arg<u32>(ctx, 0);
    constexpr u32 kZero = 0x004A0BA8;  // 0.0f
    const f64 x = LfsAt(o + 8), k = LfsAt(kZero);
    ctx.fpr[1] = x;
    ctx.fpr[2] = k;
    ctx.gpr[4] = kZero;
    if (x < k) {
        ret(ctx, 1u);
        return;
    }
    const u32 m = (load<u32>(o + 16) >> 1) & 0x7FFFu;
    ctx.gpr[4] = m;
    if (m != 0) {
        ret(ctx, 0u);
        return;
    }
    const f64 y = LfsAt(o + 4);
    ctx.fpr[2] = y;
    ret(ctx, (y > x || y == x) ? 1u : 0u);
}
// bool f(a): false when *(T**)0x010C2C28 is null; else 0x004FA564(a,
// 0x00AF0468, that + 856, 0), true (r4 left as the null)
DECOMP(004EF7F0, Global_Call4FA564_004EF7F0) {
    const u32 g = load<u32>(0x010C2C28);
    if (g == 0) {
        ctx.gpr[4] = 0;
        ret(ctx, 0u);
        return;
    }
    call(ctx, 0x004FA564, {ctx.gpr[3], 0x00AF0468, g + 856, 0});
    ret(ctx, 1u);
}
// T::~T(): vtable 0x00B10698, RefNode_Release(+20), vtable 0x00B0F1D0,
// then the base destructor 0x004BB41C(this)
DECOMP(0073EE80, VtB10698_Dtor) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 n = load<u32>(o + 20);
    store<u32>(o, 0x00B10698);
    call(ctx, o24fn::RefNode_Release, {n, 0x00B10698});
    store<u32>(o, 0x00B0F1D0);
    call(ctx, 0x004BB41C, {o, 0x00B0F1D0});
}
// void Vec3_Cross(Vec3* out, const Vec3* a, const Vec3* b): out = a x b
// (fused; f1..f3 left)
DECOMP(00748AFC, Vec3_Cross_00748AFC) {
    const u32 d = arg<u32>(ctx, 0), a = arg<u32>(ctx, 1), b = arg<u32>(ctx, 2);
    const f32 x = Fms(load<f32>(a + 4), load<f32>(b + 8), static_cast<f32>(load<f32>(a + 8) * load<f32>(b + 4)));
    store<f32>(d, x);
    const f32 y = Fms(load<f32>(a + 8), load<f32>(b + 0), static_cast<f32>(load<f32>(a + 0) * load<f32>(b + 8)));
    store<f32>(d + 4, y);
    const f32 a0 = load<f32>(a + 0), b4 = load<f32>(b + 4);
    const f32 z = Fms(a0, b4, static_cast<f32>(load<f32>(a + 4) * load<f32>(b + 0)));
    store<f32>(d + 8, z);
    ctx.fpr[1] = z;
    ctx.fpr[2] = b4;
    ctx.fpr[3] = a0;
}
// f32 f(f32 x): 0x005E6578 (libm) of x clamped to [-1, 1] (NaN -> 1)
DECOMP(0074BD18, Math_ClampedCall_0074BD18) {
    constexpr u32 k = 0x0074BCF8;  // 1.0f, 1.0, -1.0f, -1.0
    const f64 x = farg(ctx, 0);
    f64 v = static_cast<f32>(Fsel24(static_cast<f32>(LfsAt(k) - x), x, load<f64>(k + 8)));
    v = static_cast<f32>(Fsel24(static_cast<f32>(v - LfsAt(k + 16)), v, load<f64>(k + 24)));
    callf(ctx, 0x005E6578, {k}, {v});
}
// s32 Rng_Range(Rng* g, s32 lo, s32 hi): lo + g->+8 % (hi - lo + 1)
// (unsigned), then the state steps: s = rotl(s, 4) + rotl(s, 16) ^
// 0x87654321; +8 = rotl(s, 16), the u64 +0 = s & 0xFFFF. A zero divisor
// (hi - lo == -1) gives the quotient 0, as the simulator models divwu.
// r4..r9 left.
DECOMP(0074E23C, Rng_Range_0074E23C) {
    const u32 g = arg<u32>(ctx, 0), lo = arg<u32>(ctx, 1);
    const u32 span = arg<u32>(ctx, 2) - lo;
    const u32 st = load<u32>(g + 8);
    const u32 n = span + 1;
    const u32 q = n != 0 ? st / n : 0u;
    const u32 rem = st - (q * span + q);
    const u32 s = (std::rotl(st, 4) + std::rotl(st, 16)) ^ 0x87654321u;
    const u32 lo16 = s & 0xFFFFu;
    store<u32>(g + 8, std::rotl(s, 16));
    store<u32>(g, 0);
    store<u32>(g + 4, lo16);
    const u32 r = lo + rem;
    ctx.gpr[5] = std::rotl(s, 16);
    ctx.gpr[6] = lo16;
    ctx.gpr[7] = lo16;
    ctx.gpr[8] = 0x87654321u;
    ctx.gpr[9] = std::rotl(st, 16);
    ctx.gpr[4] = static_cast<u64>(static_cast<s64>(static_cast<s32>(r)));
    ret(ctx, static_cast<s32>(r));
}
// void f(Array* a, s32 i): removes element i of the 16-byte (Vec3) array
// a->+0 by moving the last one into it (the u16 count +8 drops by one).
// r3..r6 / f1 left.
DECOMP(0074E2FC, Vec3Array_RemoveSwap_0074E2FC) {
    const u32 a = arg<u32>(ctx, 0);
    const u32 i = arg<u32>(ctx, 1);
    const u32 n = load<u16>(a + 8) - 1u;
    const u32 last = n & 0xFFFFu;
    store<u16>(a + 8, static_cast<u16>(n));
    ctx.gpr[4] = n;
    ctx.gpr[5] = i;
    ctx.gpr[6] = last;
    if (i == last) return;
    const u32 base = load<u32>(a);
    const u32 d = base + (i << 4), s = base + (last << 4);
    Cp(d, s);
    Cp(d + 4, s + 4);
    ctx.fpr[1] = F32Bits(Cp(d + 8, s + 8));
    ctx.gpr[3] = s;
    ctx.gpr[4] = d;
    ctx.gpr[5] = d;
}
// T* f(s16 k): pool->vfn[2](pool, k) (the pool behind r13 - 28332), then
// the result's u16 +4 = k
static void PoolNewTagged(PPUContext& ctx) {
    const u32 k = arg<u32>(ctx, 0);
    const u32 pool = Pool28332(ctx);
    const u32 p = static_cast<u32>(VCall(ctx, pool, 8, {pool, static_cast<u64>(static_cast<s64>(static_cast<s32>(k)))}));
    store<u16>(p + 4, static_cast<u16>(k));
}
DECOMP(0074E354, Pool_NewTagged_0074E354) { PoolNewTagged(ctx); }
DECOMP(0074E400, Pool_NewTagged_0074E400) { PoolNewTagged(ctx); }
// T::T(T* o, a, const f32 v[2]): the base 0x004FBFA8(o, a), +72 / +76 =
// v[0] / v[1], vtable 0x00B13940 (r3 / f1 left)
DECOMP(0074E990, VtB13940_Ctor) {
    const u32 o = arg<u32>(ctx, 0), v = arg<u32>(ctx, 2);
    call(ctx, o24fn::Ctor_Base4FBFA8, {o, arg<u32>(ctx, 1), v});
    Cp(o + 72, v);
    store<u32>(o, 0x00B13940);
    ctx.fpr[1] = F32Bits(Cp(o + 76, v + 4));
    ret(ctx, 0x00B13940u);
}
// bool f(?, u32 k): k < 12; otherwise String_Format(0x00B1D308, k, 12) and
// false (an out-of-range warning)
DECOMP(00760F80, Index_Check12_00760F80) {
    const u32 k = arg<u32>(ctx, 1);
    if (k < 12) {
        ret(ctx, 1u);
        return;
    }
    call(ctx, o24fn::String_Format, {0x00B1D308, k, 12});
    ret(ctx, 0u);
}
// void f(T* o): the 38-byte state reset: the u8s +12..+14 = 1, the rest 0
// (r4..r7 left)
DECOMP(00768478, State_Reset_00768478) {
    const u32 o = arg<u32>(ctx, 0);
    store<u8>(o + 12, 1);
    store<u8>(o + 37, 0);
    store<u8>(o + 36, 0);
    store<u32>(o + 32, 0);
    store<u32>(o + 28, 0);
    store<u8>(o + 24, 0);
    store<u32>(o + 20, 0);
    store<u32>(o + 16, 0);
    store<u8>(o + 15, 0);
    store<u32>(o + 8, 0);
    store<u32>(o + 4, 0);
    store<u8>(o + 1, 0);
    store<u8>(o, 0);
    store<u8>(o + 13, 1);
    store<u8>(o + 14, 1);
    ctx.gpr[4] = 1;
    ctx.gpr[5] = o + 1;
    ctx.gpr[6] = 0;
    ctx.gpr[7] = o + 2;
}
// f(Obj* o, const Vec3* v): o's virtual `slot` (o, &copy of v) (f1..f3
// left as v)
static u64 VCallVec3Copy(PPUContext& ctx, u32 slot) {
    const u32 o = arg<u32>(ctx, 0), v = arg<u32>(ctx, 1);
    Stack frame(ctx, 128);
    const u32 t = frame.addr() + 112;
    const u32 x = Cp(t, v), y = Cp(t + 4, v + 4), z = Cp(t + 8, v + 8);
    ctx.fpr[1] = F32Bits(x);
    ctx.fpr[2] = F32Bits(y);
    ctx.fpr[3] = F32Bits(z);
    return VCall(ctx, o, slot, {o, t});
}
DECOMP(00795C70, Obj_VCall40Vec3_00795C70) { VCallVec3Copy(ctx, 40); }
DECOMP(00795D18, Obj_VCall28Vec3_00795D18) { VCallVec3Copy(ctx, 28); }

// ---------------------------------------------------------------- sweep: 23 instructions
// f(Obj* a, Obj* b): 0x00016218(a, b) when bit 0 of the u8 a->+304 or of
// the u64 b->+184 is set; otherwise returns b (r4 / r5 left as a / 0)
DECOMP(0003B78C, Obj_CallIfFlag_0003B78C) {
    const u32 a = arg<u32>(ctx, 0), b = arg<u32>(ctx, 1);
    if ((load<u8>(a + 304) & 1u) != 0 || (load<u64>(b + 184) & 1u) != 0) {
        call(ctx, 0x00016218, {a, b});
        return;
    }
    ctx.gpr[4] = a;
    ctx.gpr[5] = 0;
    ret(ctx, b);
}
// u32 f(const Obj* o): 2 when the flag byte (o->+308->+12 indexed by
// (*(T**)0x00F94B84)->+64) is clear or o->+4 is 1, 6, 12 or 59; else 1
// (r4..r6 left)
DECOMP(0003BBB8, Obj_Kind_0003BBB8) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 tbl = load<u32>(load<u32>(o + 308) + 12);
    const u32 b = load<u8>(tbl + (load<u32>(load<u32>(0x00F94B74 + 16) + 64) << 2));
    u32 r = 2;
    if (b != 0) {
        const s32 t = load<s32>(o + 4);
        if (!(t == 1 || t == 6 || t == 12 || t == 59)) r = 1;
    }
    ctx.gpr[4] = r;
    ctx.gpr[5] = b;
    ctx.gpr[6] = tbl;
    ret(ctx, r);
}
// f(const T* a): Obj_Call353CC0_000FCFDC(p, a->+12 (u8)) for the world's
// +0x1281E0 object p when it is set and has bit 0x40000 of +172 (r4 / r5
// left otherwise)
DECOMP(0006F70C, World_Call1281E0_0006F70C) {
    const u32 a = arg<u32>(ctx, 0);
    const u32 p = load<u32>(load<u32>(kWorldPtr) + 0x1281E0);
    ctx.gpr[4] = p;
    ctx.gpr[5] = 0x1281E0;
    if (p == 0) return;
    const u32 f = (load<u32>(p + 172) >> 18) & 1u;
    ctx.gpr[5] = f;
    if (f == 0) return;
    call(ctx, 0x000FCFDC, {p, load<u8>(a + 12), a});
}
// f(const T* a): Fade_Start(world + 0x128290, a + 12, a + 16, a->+20,
// a->+24 (hide), 1 (force)) (r6 = the fade, r9 = a + 16, as passed)
DECOMP(0008375C, World_StartFade_0008375C) {
    const u32 a = arg<u32>(ctx, 0);
    const u32 fade = load<u32>(kWorldPtr) + 0x128290;
    callf(ctx, 0x00468C40, {fade, a + 12, a + 16, fade, load<u8>(a + 24), 1, a + 16}, {LfsAt(a + 20)});
}
// bool f(T* o): Ptr_DerefNonZero(o->+20); when false, RefNode_Release(+20)
// and +20 = Node_PushF65B7C(+12, +16)
DECOMP(0008C480, Obj_EnsureNode_0008C480) {
    const u32 o = arg<u32>(ctx, 0);
    if (call(ctx, 0x00446960, {load<u32>(o + 20)}) != 0) return;
    call(ctx, o24fn::RefNode_Release, {load<u32>(o + 20)});
    const u32 n = static_cast<u32>(call(ctx, 0x00446984, {load<u32>(o + 12), load<u32>(o + 16)}));
    store<u32>(o + 20, n);
}
// The thread body: the u8s 0x00F558E2 / 0x00F558E3 = 0, then +E2 =
// 0x000A88C0(), +E3 = 1 (done) and sys_ppu_thread_exit(0)
DECOMP(000ABAD4, Thread_Run_000ABAD4) {
    store<u8>(0x00F558E2, 0);
    store<u8>(0x00F558E3, 0);
    const u32 r = static_cast<u32>(call(ctx, 0x000A88C0, {0}));
    store<u8>(0x00F558E2, static_cast<u8>(r));
    store<u8>(0x00F558E3, 1);
    call(ctx, o23imp::sys_ppu_thread_exit, {0, 1});
}
// u16 f(Obj* o): (o's virtual +144 (o))->+44 >> 16 when that is set, else
// o->+204 (read before the call)
DECOMP(000DA0DC, Obj_GetId_000DA0DC) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 dflt = load<u32>(o + 204);
    const u32 r = static_cast<u32>(VCall(ctx, o, 144, {o}));
    ret(ctx, r != 0 ? load<u32>(r + 44) >> 16 : dflt);
}
// f32 f(const Obj* o): the distance from o->+144 to the world's +0x128168
// object's +864 (f2 / f3 / f4 and r3..r5 left)
DECOMP(000DAE6C, Obj_DistToFocus_000DAE6C) {
    const u32 p = arg<u32>(ctx, 0) + 144;
    const u32 w = load<u32>(load<u32>(kWorldPtr) + 0x128168) + 864;
    const f32 dy = load<f32>(p + 4) - load<f32>(w + 4);
    const f32 dx = load<f32>(p) - load<f32>(w);
    const f32 pz = load<f32>(p + 8);
    const f32 dz = pz - load<f32>(w + 8);
    f32 s = dy * dy;
    s = static_cast<f32>(std::fma(static_cast<f64>(dx), dx, s));
    s = static_cast<f32>(std::fma(static_cast<f64>(dz), dz, s));
    ctx.gpr[3] = p;
    ctx.gpr[4] = w;
    ctx.gpr[5] = 0x128168;
    ctx.fpr[2] = dx;
    ctx.fpr[3] = dz;
    ctx.fpr[4] = pz;
    ctx.fpr[1] = static_cast<f32>(std::sqrt(static_cast<f64>(s)));
}
// f(Obj* o, u32 hash): Effect_SetHash4CF988(e, hash) for e = o's virtual
// +272 (o) when that is set
DECOMP(000DAEC8, Obj_SetEffectHash_000DAEC8) {
    const u32 o = arg<u32>(ctx, 0), h = arg<u32>(ctx, 1);
    const u32 e = static_cast<u32>(VCall(ctx, o, 272, {o}));
    if (e != 0) call(ctx, 0x004CF988, {e, h});
}
// f(Obj* o, const char* s): o's virtual +188 (o, String_HashNoCase(s))
DECOMP(000DBD20, Obj_VCall188Hash_000DBD20) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 h = static_cast<u32>(call(ctx, o22fn::String_HashNoCase, {arg<u32>(ctx, 1)}));
    VCall(ctx, o, 188, {o, h});
}
// f(Obj* o, i, u8 on): 0x004CF8D8 (on) or 0x004CF330 (off) (o->+356,
// o->+432 + i * 16)
DECOMP(001AF40C, Obj_SetEntry_001AF40C) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 on = arg<u32>(ctx, 2) & 0xFFu;
    const u32 e = load<u32>(o + (arg<u32>(ctx, 1) << 4) + 432);
    call(ctx, on != 0 ? 0x004CF8D8u : 0x004CF330u, {load<u32>(o + 356), e, on});
}
// bool f(Obj* o): 0x001BC5C8(o) when the low 2 bits of 0x010426B0 are
// set; then clears bit 0x400 of the u16 o->+394; true (r4 / r5 left)
DECOMP(001BD10C, Obj_Clear394_001BD10C) {
    const u32 o = arg<u32>(ctx, 0);
    if ((load<u32>(0x01042668 + 72) & 3u) != 0) call(ctx, 0x001BC5C8, {o});
    const u32 h = load<u16>(o + 394) & ~0x400u;
    store<u16>(o + 394, static_cast<u16>(h));
    ctx.gpr[4] = 0;
    ctx.gpr[5] = h;
    ret(ctx, 1u);
}
// f(Obj* o, a): 0x004AE160(o->+436, a, 1) when that is set, then
// 0x0020EF60(o, a)
DECOMP(00225010, Obj_Call20EF60_00225010) {
    const u32 o = arg<u32>(ctx, 0), a = arg<u32>(ctx, 1);
    const u32 p = load<u32>(o + 436);
    if (p != 0) call(ctx, 0x004AE160, {p, a, 1});
    call(ctx, 0x0020EF60, {o, a});
}
// f32 f(const Obj* o): Comp_GetF32_922918(c + 224) for c = o->+612->+24
// when both are set and c->+8 too, else 0.0f (*(f32*)0x0023CAA0)
DECOMP(0023CAA8, Obj_GetF32_0023CAA8) {
    const u32 o = arg<u32>(ctx, 0);
    constexpr u32 kZero = 0x0023CAA0;
    ctx.fpr[1] = LfsAt(kZero);
    ctx.gpr[4] = kZero;
    const u32 p = load<u32>(o + 612);
    ctx.gpr[3] = p;
    if (p == 0) return;
    const u32 c = load<u32>(p + 24);
    ctx.gpr[3] = c;
    if (c == 0) return;
    const u32 k = load<u32>(c + 8);
    ctx.gpr[4] = k;
    if (k == 0) return;
    call(ctx, 0x00922918, {c + 224, k});
}
// f(T* o): 0x0028C4B8(o) unless 0x010F0ED8 is 2, then 0x0028D0FC(
// 0x01164BD0, o->+140, o + 68)
DECOMP(0028D644, Obj_Update_0028D644) {
    const u32 o = arg<u32>(ctx, 0);
    if (load<s32>(0x010F0E20 + 184) != 2) call(ctx, 0x0028C4B8, {o});
    call(ctx, 0x0028D0FC, {0x01164BD0, load<u32>(o + 140), o + 68});
}
// void f(T* o, bool on): bit 0x40 of the u8 +2336 = on; then the u8 +2338
// gets 0x10 and loses 0x20 when on and +2336 has 0x20, else loses 0x10 and
// gets 0x20 (r4 / r5 / r6 left)
DECOMP(0029F9CC, Obj_SetFlags2336_0029F9CC) {
    const u32 o = arg<u32>(ctx, 0), on = arg<u32>(ctx, 1);
    const u32 old = load<u8>(o + 2336);
    const u32 b = (old & ~0x40u) | ((on & 1u) << 6);
    store<u8>(o + 2336, static_cast<u8>(b));
    ctx.gpr[6] = old;
    if ((on & 1u) != 0 && ((b >> 5) & 1u) != 0) {
        const u32 v = (load<u8>(o + 2338) | 0x10u) & ~0x20u;
        store<u8>(o + 2338, static_cast<u8>(v));
        ctx.gpr[4] = 0;
        ctx.gpr[5] = v;
        return;
    }
    const u32 v = load<u8>(o + 2338) & ~0x10u;
    store<u8>(o + 2338, static_cast<u8>(v | 0x20u));
    ctx.gpr[4] = v | 0x20u;
    ctx.gpr[5] = v;
}
// T::~T(): vtable 0x00B1D020, then SortedMap_Remove75E06C(0x0105ABB8,
// &String_HashNoCase(this + 4))
DECOMP(002C0DB8, VtB1D020_Dtor) {
    const u32 o = arg<u32>(ctx, 0);
    Stack frame(ctx, 128);
    store<u32>(o, 0x00B1D020);
    const u32 h = static_cast<u32>(call(ctx, o22fn::String_HashNoCase, {o + 4, 0x00B1D020, 0x01060000, o + 4}));
    const u32 t = frame.addr() + 112;
    store<u32>(t, h);
    call(ctx, 0x0075E06C, {0x0105ABB8, t});
}
// f(T* o): clears +304..+319; when the u8 +320 is set, clears it and +324
// and calls 0x002C2420(o, 0) (r5 / r6 left)
DECOMP(002C26CC, Obj_Reset304_002C26CC) {
    const u32 o = arg<u32>(ctx, 0);
    store<u8>(o + 304, 0);
    store<u32>(o + 308, 0);
    const u32 b = load<u8>(o + 320);
    store<u32>(o + 312, 0);
    store<u8>(o + 316, 0);
    store<u8>(o + 317, 0);
    store<u8>(o + 319, 0);
    ctx.gpr[5] = 0;
    ctx.gpr[6] = b;
    if (b == 0) return;
    store<u8>(o + 320, 0);
    store<u32>(o + 324, 0);
    call(ctx, 0x002C2420, {o, 0, o, 0});
}
// void f(const Obj* o, u8 on): bit `bit` of the u16 at world + 496 +
// (world->+4208 (u8)) * 4056 + o->+822 (u8) * 4 = (on == 1)
static void WorldSlotFlag(PPUContext& ctx, u32 bit) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 on = arg<u32>(ctx, 1) & 0xFFu;
    const u32 w = load<u32>(kWorldPtr);
    const u32 ea = w + load<u8>(w + 4208) * 4056u + (load<u8>(o + 822) << 2) + 496;
    u32 h = load<u16>(ea);
    if (on == 1) {
        h |= bit;
        ctx.gpr[6] = w + 4208;
    } else {
        h &= ~bit;
        ctx.gpr[6] = 0;
    }
    store<u16>(ea, static_cast<u16>(h));
    ctx.gpr[3] = ea;
    ctx.gpr[4] = h;
    ctx.gpr[5] = w;
}
DECOMP(002DA8E8, Obj_SetSlotFlag100_002DA8E8) { WorldSlotFlag(ctx, 0x100); }
DECOMP(002DAFA0, Obj_SetSlotFlag80_002DAFA0) { WorldSlotFlag(ctx, 0x80); }
// bool f(const T* o): true unless e = o->+off->+52 is set and
// Effect_Check4CF620(e, 0) is 0 (null o->+off: true; r4 left)
static void EffectAlive(PPUContext& ctx, u32 off) {
    const u32 p = load<u32>(arg<u32>(ctx, 0) + off);
    if (p == 0) {
        ret(ctx, 1u);
        return;
    }
    const u32 e = load<u32>(p + 52);
    u32 r = 1;
    if (e != 0) r = static_cast<u32>(call(ctx, 0x004CF620, {e, 0}));
    ctx.gpr[4] = r;
    ret(ctx, r != 0 ? 1u : 0u);
}
DECOMP(002F9940, Obj_EffectAlive8_002F9940) { EffectAlive(ctx, 8); }
DECOMP(002FE4B4, Obj_EffectAlive48_002FE4B4) { EffectAlive(ctx, 48); }
// f(T* o, a, b): o->+48's virtual +20 (p, a, b) when set; else returns b
// (r5 / r6 left as 0 / o)
DECOMP(002FE13C, Obj_VCall20Member48_002FE13C) {
    const u32 o = arg<u32>(ctx, 0), a = arg<u32>(ctx, 1), b = arg<u32>(ctx, 2);
    const u32 p = load<u32>(o + 48);
    if (p == 0) {
        ctx.gpr[5] = 0;
        ctx.gpr[6] = o;
        ret(ctx, b);
        return;
    }
    VCall(ctx, p, 20, {p, a, b});
}
// bool f(Obj* o): o's virtual +32 (o, 0) < 2 (signed)
DECOMP(00316618, Obj_VCall32Lt2_00316618) {
    const u32 o = arg<u32>(ctx, 0);
    const s32 r = static_cast<s32>(VCall(ctx, o, 32, {o, 0}));
    ret(ctx, r < 2 ? 1u : 0u);
}
// f(T* o, a, const U* p): 0x00334D88(o, a, p; p->+24) and the u8 o->+18
// when that is 4, else 0x00326A58(o, a, p; p->+24)
DECOMP(00335174, Obj_Dispatch18_00335174) {
    const u32 o = arg<u32>(ctx, 0), a = arg<u32>(ctx, 1), p = arg<u32>(ctx, 2);
    const f64 x = LfsAt(p + 24);
    if (load<u8>(o + 18) == 4) {
        callf(ctx, 0x00334D88, {o, a, p}, {x});
        ret(ctx, static_cast<u32>(load<u8>(o + 18)));
        return;
    }
    callf(ctx, 0x00326A58, {o, a, p}, {x});
}
// T::~T(): when +8 is set: vtable 0x00B1F480, AnimSkel_Release7D4EA8(+8),
// +8 = 0; then vtable 0x00B13974 (r3 / r4 left as the vtables)
DECOMP(003463B0, VtB1F480_Dtor) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 p = load<u32>(o + 8);
    if (p != 0) {
        store<u32>(o, 0x00B1F480);
        call(ctx, 0x007D4EA8, {p, 0x00B1F480});
        store<u32>(o + 8, 0);
    } else {
        ctx.gpr[4] = 0x00B1F480;
    }
    store<u32>(o, 0x00B13974);
    ret(ctx, 0x00B13974u);
}
// u32 f(const T* o): the size of the current record: n = o->+146 (u16),
// e = o + 152 + n * 132; (e->+0 (u16) * 12 + 23 & ~15) + (the u16 after
// that * 8 + 21 & ~15) (r4..r6 left)
DECOMP(0038BA10, Rec_Size_0038BA10) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 n = load<u16>(o + 146);
    const u32 off = n * 132u;
    const u32 a = load<u16>(o + off + 152);
    const u32 s1 = (a * 12u + 23u) & ~15u;
    const u32 r4 = off + s1 + 152;
    const u32 s2 = (((load<u16>(o + r4) << 3) & 0x7FFF8u) + 21u) & ~15u;
    ctx.gpr[4] = r4;
    ctx.gpr[5] = s1;
    ctx.gpr[6] = a * 2u;
    ret(ctx, s1 + s2);
}
// f32 f(const Obj* o): 0x000DC488(o->+96, 0, 0) /
// Anim_TrackLength164_000DC418(o->+96, 0)
DECOMP(003ABE24, Anim_Progress_003ABE24) {
    const u32 o = arg<u32>(ctx, 0);
    call(ctx, 0x000DC488, {load<u32>(o + 96), 0, 0});
    const f32 a = static_cast<f32>(ctx.fpr[1]);
    call(ctx, 0x000DC418, {load<u32>(o + 96), 0});
    ctx.fpr[1] = static_cast<f32>(static_cast<f64>(a) / ctx.fpr[1]);
}
// f(): PairBuffer_Flush(p->+12, 0) when bit 0x40 of the u8 0x010AA1B0 is
// set and p = *(T**)0x010AA614 and p->+12 are set (r3 left as the zero)
DECOMP(00429C04, Flush_00429C04) {
    constexpr u32 k = 0x010AA1B0;
    u32 v = (load<u8>(k) >> 6) & 1u;
    if (v != 0) {
        v = load<u32>(k + 1124);
        if (v != 0) {
            v = load<u32>(v + 12);
            if (v != 0) {
                call(ctx, 0x00565228, {v, 0});
                return;
            }
        }
    }
    ret(ctx, v);
}
// bool f(): the world's virtual +32 (world, 1) != 12
DECOMP(00433AD0, World_VCall32Ne12_00433AD0) {
    const u32 w = load<u32>(kWorldPtr);
    const u32 r = static_cast<u32>(VCall(ctx, w, 32, {w, 1}));
    ret(ctx, r != 12 ? 1u : 0u);
}
// f(const T* o): p = o->+32->+244's virtual +160 (p) when both are set,
// else true
DECOMP(00437BB0, Obj_VCall160_00437BB0) {
    const u32 a = load<u32>(arg<u32>(ctx, 0) + 32);
    const u32 p = a != 0 ? load<u32>(a + 244) : 0u;
    if (p == 0) {
        ret(ctx, 1u);
        return;
    }
    VCall(ctx, p, 160, {p});
}
// f(T* o, u8 v): u8 +51 = the old +30, +30 = v; then
// Obj_SelectEntry(Player_Get(), 0x0023FDB9) and Obj_SetMode470750(o, 4, 0)
DECOMP(0048A0E4, Obj_SetState30_0048A0E4) {
    const u32 o = arg<u32>(ctx, 0), v = arg<u32>(ctx, 1);
    const u32 old = load<u8>(o + 30);
    store<u8>(o + 30, static_cast<u8>(v));
    store<u8>(o + 51, static_cast<u8>(old));
    const u32 pl = static_cast<u32>(call(ctx, 0x00365C24, {old, v}));
    call(ctx, 0x003734B0, {pl, 0x0023FDB9});
    call(ctx, 0x00470750, {o, 4, 0});
}
// bool f(Obj* o, u8 b): 0x00353EB0(Anim_Find34CC34(o, 0), b) when o and the
// find are set, else true
DECOMP(004A5D40, Obj_Anim353EB0_004A5D40) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 b = arg<u32>(ctx, 1) & 0xFFu;
    if (o != 0) {
        const u32 a = static_cast<u32>(call(ctx, 0x0034CC34, {o, 0, o}));
        if (a != 0) {
            call(ctx, 0x00353EB0, {a, b});
            return;
        }
    }
    ret(ctx, 1u);
}
// f32 f(f32 x, f32 a, f32 len, f32 t): t / x when x > t; else 1 - clamp1((t
// - (a + x)) / len) when t > a + x; else 1.0f when a > 0, else 0.0f (r3 /
// f2 / f3 left)
DECOMP(004A6BF0, Math_Ramp_004A6BF0) {
    constexpr u32 k = 0x004A6BE4;  // 0.0f, 1.0f
    const f64 x = farg(ctx, 0), a = farg(ctx, 1), len = farg(ctx, 2), t = farg(ctx, 3);
    ctx.gpr[3] = k;
    if (x > t) {
        ctx.fpr[1] = static_cast<f32>(t / x);
        return;
    }
    const f64 s = static_cast<f32>(a + x);
    if (t > s) {
        const f64 one = LfsAt(k + 4);
        const f64 q = static_cast<f32>(static_cast<f32>(t - s) / len);
        const f64 c = static_cast<f32>(one - q);
        ctx.fpr[2] = one;
        ctx.fpr[3] = c;
        ctx.fpr[1] = static_cast<f32>(one - Fsel24(c, q, one));
        return;
    }
    const f64 z = LfsAt(k);
    ctx.fpr[1] = a > z ? LfsAt(k + 4) : z;
}
// bool f(const T* o, a): e = Effect_Bind4BB7E0(a, o->+36, 0); true when e
// is null or its s16 +4 is <= 1
DECOMP(004C1ABC, Effect_Single_004C1ABC) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 e = static_cast<u32>(call(ctx, 0x004BB7E0, {arg<u32>(ctx, 1), load<u32>(o + 36), 0}));
    ret(ctx, (e == 0 || load<s16>(e + 4) <= 1) ? 1u : 0u);
}
// void Ref_Release(T* o): --(u16)o->+4; when it drops to <= 0 (signed),
// o's virtual +4 (o) unless o is null (r5 / r6 left)
DECOMP(0073B890, Ref_Release_0073B890) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 n = load<u16>(o + 4) - 1u;
    store<u16>(o + 4, static_cast<u16>(n));
    const s32 sn = static_cast<s16>(n);
    ctx.gpr[5] = n;
    ctx.gpr[6] = static_cast<u64>(static_cast<s64>(sn));
    if (sn > 0 || o == 0) return;
    VCall(ctx, o, 4, {o});
}
// void f(T* d, const Vec3* a, const Vec3* b, const Vec3* c, f32 x, f32 y,
// f32 z, ..., u32 w): +0 / +16 / +32 = a / b / c, +48..+56 = x, y, z, +60 =
// w (r10) (f4 left as c->z)
DECOMP(0074CD20, Frame_Set_0074CD20) {
    const u32 d = arg<u32>(ctx, 0), a = arg<u32>(ctx, 1), b = arg<u32>(ctx, 2), c = arg<u32>(ctx, 3);
    Cp(d, a);
    Cp(d + 4, a + 4);
    Cp(d + 8, a + 8);
    Cp(d + 16, b);
    Cp(d + 20, b + 4);
    Cp(d + 24, b + 8);
    Cp(d + 32, c);
    Cp(d + 36, c + 4);
    const u32 cz = load<u32>(c + 8);
    store<f32>(d + 48, static_cast<f32>(farg(ctx, 0)));
    store<u32>(d + 40, cz);
    store<f32>(d + 52, static_cast<f32>(farg(ctx, 1)));
    store<f32>(d + 56, static_cast<f32>(farg(ctx, 2)));
    store<u32>(d + 60, static_cast<u32>(ctx.gpr[10]));
    ctx.fpr[4] = F32Bits(cz);
}
// bool f(Obj* o): o's virtual +32 (o, 0) == 1
DECOMP(007549BC, Obj_VCall32Is1_007549BC) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 r = static_cast<u32>(VCall(ctx, o, 32, {o, 0}));
    ret(ctx, r == 1 ? 1u : 0u);
}
// T::~T(): vtable 0x00B205B8, the member at +56 gets 0x00B2061C and
// 0x0048F6B0(+56) destroys it, then Dtor_B219D0_00490B34(this)
DECOMP(0077B514, VtB205B8_Dtor) {
    const u32 o = arg<u32>(ctx, 0);
    store<u32>(o, 0x00B205B8);
    store<u32>(o + 56, 0x00B2061C);
    call(ctx, 0x0048F6B0, {o + 56, 0x00B2061C, 0x00B205B8});
    call(ctx, 0x00490B34, {o});
}
// f(PtrVector* v, a): PtrVector_Construct(v, a), the u16 +6 = 8,
// PtrVector_Init(v, a, 0, 0)
DECOMP(00788298, PtrVector_Init8_00788298) {
    const u32 v = arg<u32>(ctx, 0), a = arg<u32>(ctx, 1);
    call(ctx, 0x005516E0, {v, a});
    store<u16>(v + 6, 8);
    call(ctx, 0x00551598, {v, a, 0, 0});
}

// ---------------------------------------------------------------- sweep: 24 instructions
// T* f(Obj* o): o->+48's virtual +96 (p, 1), then bit 0x1000 of the u16
// p->+176 set; returns p (r4 left as the u16)
DECOMP(00030A70, Obj_VCall96Flag_00030A70) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 p = load<u32>(o + 48);
    VCall(ctx, p, 96, {p, 1});
    const u32 q = load<u32>(o + 48);
    const u32 h = load<u16>(q + 176) | 0x1000u;
    store<u16>(q + 176, static_cast<u16>(h));
    ctx.gpr[4] = h;
    ret(ctx, q);
}
// bool f(const Obj* o, const T* a): true when the flag byte (o->+308->+12
// indexed by (*(T**)0x00F95480)->+64) is set, else a->+4 < 0.0f (r4 / r5,
// f1 / f2 left)
DECOMP(0003D068, Obj_FlagOrNeg_0003D068) {
    const u32 o = arg<u32>(ctx, 0), a = arg<u32>(ctx, 1);
    const u32 tbl = load<u32>(load<u32>(o + 308) + 12);
    const u32 off = load<u32>(load<u32>(0x00F95470 + 16) + 64) << 2;
    ctx.gpr[5] = off;
    if (load<u8>(tbl + off) != 0) {
        ret(ctx, 1u);
        return;
    }
    constexpr u32 kZero = 0x0003D060;
    const f64 k = LfsAt(kZero), v = LfsAt(a + 4);
    ctx.gpr[4] = a + 4;
    ctx.fpr[1] = k;
    ctx.fpr[2] = v;
    ret(ctx, v < k ? 1u : 0u);
}
// f(const T* a): 0x0005BA08(q, a->+12, a->+16) for q = Obj_FindById(
// 0xCD4F65FC)->+244 when both are set
DECOMP(0008B908, Obj_Call5BA08_0008B908) {
    const u32 a = arg<u32>(ctx, 0);
    const u32 p = static_cast<u32>(call(ctx, 0x000D6A28, {0xCD4F65FC}));
    if (p == 0) return;
    const u32 q = load<u32>(p + 244);
    ret(ctx, q);
    if (q == 0) return;
    call(ctx, 0x0005BA08, {q, load<u32>(a + 12), load<u32>(a + 16)});
}
// f(o, const char* name, a, u8 b): Obj_EffectDCD20(o, String_HashNoCase(name), a, b)
DECOMP(000DCE6C, Obj_EffectByName_000DCE6C) {
    const u32 o = arg<u32>(ctx, 0), a = arg<u32>(ctx, 2);
    const u32 b = arg<u32>(ctx, 3) & 0xFFu;
    const u32 h = static_cast<u32>(call(ctx, o22fn::String_HashNoCase, {arg<u32>(ctx, 1)}));
    call(ctx, 0x000DCD20, {o, h, a, b});
}
// f(?, Obj* o): 0x0013B4D0(o) when o's virtual +32 (o, 0) is 3; else that
DECOMP(0013B6FC, Obj_CallIfState3_0013B6FC) {
    const u32 o = arg<u32>(ctx, 1);
    if (VCall(ctx, o, 32, {o, 0}) == 3) call(ctx, 0x0013B4D0, {o});
}
// f(T* o, s32 x): clears the first entry equal to x of the u32 array
// o->+508 (count o->+512); r3 left as its address (or the end), r5..r9
// left
DECOMP(0016DCF0, Array_ClearValue_0016DCF0) {
    const u32 o = arg<u32>(ctx, 0), x = arg<u32>(ctx, 1);
    if (x == 0) return;
    const s32 n = load<s32>(o + 512);
    const u32 arr = load<u32>(o + 508);
    ctx.gpr[3] = arr;
    ctx.gpr[5] = arr;
    ctx.gpr[6] = static_cast<u32>(n);
    ctx.gpr[7] = 0;
    ctx.gpr[8] = 0;
    if (n <= 0) return;
    for (s32 i = 0; i < n; ++i) {
        const u32 v = load<u32>(arr + 4u * static_cast<u32>(i));
        ctx.gpr[9] = v;
        ctx.gpr[8] = static_cast<u32>(i);
        if (v == x) {
            store<u32>(arr + 4u * static_cast<u32>(i), 0);
            ctx.gpr[3] = arr + 4u * static_cast<u32>(i);
            return;
        }
    }
    ctx.gpr[8] = static_cast<u32>(n);
    ctx.gpr[3] = arr + 4u * static_cast<u32>(n);
}
// f(Obj* o, s32 a, u8 b): 0x001AED20(o, a, b); when a == 1, clears bit
// 0x80 of the u8 o->+604 and calls 0x001C3EF0(o, 1)
DECOMP(001C4280, Obj_Set_001C4280) {
    const u32 o = arg<u32>(ctx, 0), a = arg<u32>(ctx, 1);
    call(ctx, 0x001AED20, {o, a, arg<u32>(ctx, 2) & 0xFFu});
    if (static_cast<s32>(a) != 1) return;
    const u32 b = load<u8>(o + 604) & ~0x80u;
    store<u8>(o + 604, static_cast<u8>(b));
    call(ctx, 0x001C3EF0, {o, 1, 0, b});
}
// f(Obj* o, f32 x): Target_GetDef0D951C(o), once more when that is set,
// then 0x000E4720(o; x)
DECOMP(001DC590, Obj_Call0E4720_001DC590) {
    const u32 o = arg<u32>(ctx, 0);
    const f64 x = farg(ctx, 0);
    if (call(ctx, 0x000D951C, {o}) != 0) call(ctx, 0x000D951C, {o});
    callf(ctx, 0x000E4720, {o}, {x});
}
// T::~T(): vtable 0x00B19920, RefNode_Release(+184), +184 = 0, vtable
// 0x00B19510, then the base destructor 0x000C2DA0(this)
DECOMP(002101D8, VtB19920_Dtor) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 n = load<u32>(o + 184);
    store<u32>(o, 0x00B19920);
    call(ctx, o24fn::RefNode_Release, {n, 0x00B19920});
    store<u32>(o + 184, 0);
    store<u32>(o, 0x00B19510);
    call(ctx, 0x000C2DA0, {o, 0x00B19510, 0});
}
// f(Obj* o): o's virtual +396 (o, 0), bit 0x2000 of +172 set, then
// 0x00229B30(o, that word)
DECOMP(0024458C, Obj_VCall396Flag_0024458C) {
    const u32 o = arg<u32>(ctx, 0);
    VCall(ctx, o, 396, {o, 0});
    const u32 w = load<u32>(o + 172) | 0x2000u;
    store<u32>(o + 172, w);
    call(ctx, 0x00229B30, {o, w});
}
// f(T* o, u8 a, u8 b): o->+92's virtual +40 (p, a, b) when set (r4 / r5 /
// r6 left otherwise)
DECOMP(0025FA64, Obj_VCall40Member92_0025FA64) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 a = arg<u32>(ctx, 1) & 0xFFu, b = arg<u32>(ctx, 2) & 0xFFu;
    const u32 p = load<u32>(o + 92);
    if (p == 0) {
        ctx.gpr[6] = ctx.gpr[5];
        ctx.gpr[4] = b;
        ctx.gpr[5] = a;
        ret(ctx, 0u);
        return;
    }
    VCall(ctx, p, 40, {p, a, b});
}
// u32 f(T* o): o->+64's virtual +40 (p, 0) when set, then +64 = 0; 0
DECOMP(002B82C0, Obj_ReleaseMember64_002B82C0) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 p = load<u32>(o + 64);
    if (p != 0) VCall(ctx, p, 40, {p, 0});
    store<u32>(o + 64, 0);
    ret(ctx, 0u);
}
// bool f(?, u32 i): bit i of the bit set at world + 4020 + (world->+4208
// (u8)) * 4056; false for i >= 40 (r4..r6 left)
DECOMP(002E2950, World_TestBit_002E2950) {
    const u32 i = arg<u32>(ctx, 1);
    const u32 w = load<u32>(kWorldPtr);
    const u32 slot = load<u8>(w + 4208) * 4056u;
    ctx.gpr[5] = slot;
    if (i >= 40) {
        ret(ctx, 0u);
        return;
    }
    const u32 bit = 1u << (i & 31);
    const u32 word = load<u32>(w + slot + ((i >> 3) & 0x1FFFFFFCu) + 4020);
    ctx.gpr[4] = bit;
    ctx.gpr[5] = (i >> 3) & 0x1FFFFFFCu;
    ctx.gpr[6] = 1;
    ret(ctx, (word & bit) != 0 ? 1u : 0u);
}
// f(?, const T* a): Obj_SetState30_0048A0E4(world, (s8)a->+172),
// Call_470750_0(world, 13), then 0x00371968(Player_Get())
DECOMP(002E7694, World_SetState_002E7694) {
    const u32 a = arg<u32>(ctx, 1);
    const u32 s = static_cast<u32>(static_cast<s32>(load<s8>(a + 172)));
    call(ctx, 0x0048A0E4, {load<u32>(kWorldPtr), s, s});
    call(ctx, 0x00472208, {load<u32>(kWorldPtr), 13});
    const u32 pl = static_cast<u32>(call(ctx, 0x00365C24, {}));
    call(ctx, 0x00371968, {pl});
}
// f(const T* o, a): when the u8 o->+18 is 4, the virtual +52 (p, a) of p =
// the o->+8 world's +0x1281F4 object when that is set
DECOMP(00328B90, Obj_VCall52_00328B90) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 b = load<u8>(o + 18);
    ctx.gpr[5] = b;
    if (b != 4) return;
    const u32 p = load<u32>(load<u32>(o + 8) + 0x1281F4);
    ctx.gpr[5] = 0x1281F4;
    ret(ctx, p);
    if (p == 0) return;
    VCall(ctx, p, 52, {p, ctx.gpr[4]});
}
// f(Obj* o): o's virtual +20 (o, 0), then Obj_Place3626E8(o, 0x01164B40, 1)
DECOMP(0036A410, Obj_Place_0036A410) {
    const u32 o = arg<u32>(ctx, 0);
    VCall(ctx, o, 20, {o, 0});
    call(ctx, 0x003626E8, {o, 0x01164B40, 1});
}
// f(a): Draw_Clear(1, 0xFFFFFFFF, -1, 0, 255; *(f32*)0x003E4308), then
// Thunk_0048C874(a)
DECOMP(003E4310, Draw_ClearAll_003E4310) {
    const u32 a = arg<u32>(ctx, 0);
    callf(ctx, 0x00531F80, {1, 0xFFFFFFFFu, ~0ull, 0, 255}, {LfsAt(0x003E4308)});
    call(ctx, 0x0048C874, {a});
}
// bool f(): *(T**)0x00F65820 is set and its +348 <= *(f32*)0x003F4110
// (the second test of 0x00F65824 picks the same answer) (r4 / f1 / f2 left)
DECOMP(003F4118, Global_Below_003F4118) {
    const u32 p = load<u32>(0x00F65820);
    ctx.gpr[3] = p;
    if (p == 0) return;
    constexpr u32 k = 0x003F4110;
    const f64 v = LfsAt(p + 348), lim = LfsAt(k);
    ctx.gpr[4] = k;
    ctx.fpr[1] = lim;
    ctx.fpr[2] = v;
    if (!(v < lim || v == lim)) {
        ret(ctx, 0u);
        return;
    }
    if (load<u32>(0x00F65820 + 4) == 0) {
        ret(ctx, 1u);
        return;
    }
    ret(ctx, (v < lim || v == lim) ? 1u : 0u);
}
// bool f(const T* t, u32 i, ?, a, b): the import 0x00ABDDC4((s32)t->+4[i &
// 0xFFFF], a, b, 0) == b
DECOMP(00427744, Import_CallEq_00427744) {
    const u32 t = arg<u32>(ctx, 0), i = arg<u32>(ctx, 1);
    const u32 a = arg<u32>(ctx, 3), b = arg<u32>(ctx, 4);
    const s32 h = load<s32>(t + ((i << 2) & 0x3FFFCu) + 4);
    const u32 r = static_cast<u32>(call(ctx, 0x00ABDDC4, {static_cast<u64>(static_cast<s64>(h)), a, b, 0}));
    ret(ctx, r == b ? 1u : 0u);
}
// T::T(T* o, u32 a, u8 b): the 0x00B20CC8 base (o, on), +12 = a, the u8
// +16 = b, vtable vt
static void CtorAU8(PPUContext& ctx, u32 on, u32 vt) {
    const u32 o = arg<u32>(ctx, 0), a = arg<u32>(ctx, 1), b = arg<u32>(ctx, 2);
    call(ctx, o24fn::Obj_InitVt_B20CC8, {o, on});
    store<u32>(o + 12, a);
    store<u8>(o + 16, static_cast<u8>(b));
    store<u32>(o, vt);
    ret(ctx, vt);
}
DECOMP(0042EBC0, VtB20E78_Ctor) { CtorAU8(ctx, 0, 0x00B20E78); }
DECOMP(0042FDC8, VtB20F50_Ctor) { CtorAU8(ctx, 1, 0x00B20F50); }
// T::~T(): vtable 0x00B20E90, Audio_Release446384(0x010ACC40, +28),
// RefNode_Release(+12), then VtB20CC8_DtorTrivial_0042CA50(this)
DECOMP(0042EE90, VtB20E90_Dtor) {
    const u32 o = arg<u32>(ctx, 0);
    store<u32>(o, 0x00B20E90);
    call(ctx, 0x00446384, {0x010ACC40, load<u32>(o + 28), 0x00B20E90});
    call(ctx, o24fn::RefNode_Release, {load<u32>(o + 12)});
    call(ctx, 0x0042CA50, {o});
}
// T::~T(): vtable vt, RefNode_Release(+12), Audio_Release446384(
// 0x010ACC40, +16), then VtB20CC8_DtorTrivial_0042CA50(this)
static void DtorRefAudio(PPUContext& ctx, u32 vt) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 n = load<u32>(o + 12);
    store<u32>(o, vt);
    call(ctx, o24fn::RefNode_Release, {n, vt});
    call(ctx, 0x00446384, {0x010ACC40, load<u32>(o + 16)});
    call(ctx, 0x0042CA50, {o});
}
DECOMP(004329CC, VtB21058_Dtor) { DtorRefAudio(ctx, 0x00B21058); }
DECOMP(00434598, VtB21100_Dtor) { DtorRefAudio(ctx, 0x00B21100); }
// f(T* o): 0x00488DF4(world, o + 9, o + 140, o + 148), then
// Flag_Set8_40000000(o)
DECOMP(00433A70, Obj_Call488DF4_00433A70) {
    const u32 o = arg<u32>(ctx, 0);
    call(ctx, 0x00488DF4, {load<u32>(kWorldPtr), o + 9, o + 140, o + 148});
    call(ctx, o24fn::Flag_Set8_40000000, {o});
}
// u8 f(T* o): sets bit 0x80 of the u8 o->+556; when it has 0x10,
// Letterbox_Show(world, bit 0x20 of it); returns the byte otherwise (r4 /
// r5 left)
DECOMP(00434078, Obj_Letterbox_00434078) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 b = load<u8>(o + 556) | 0x80u;
    store<u8>(o + 556, static_cast<u8>(b));
    if (((b >> 4) & 1u) == 0) {
        ctx.gpr[4] = o;
        ctx.gpr[5] = 0;
        ret(ctx, b);
        return;
    }
    const u32 w = load<u32>(kWorldPtr);
    call(ctx, 0x00470590, {w, (b >> 5) & 1u, w});
}
// T::T(T* o, u32 a, u32 b): the 0x00B20CC8 base (o, 1), +12 = a, +16 = b,
// vtable 0x00B21298
DECOMP(004388A0, VtB21298_Ctor) {
    const u32 o = arg<u32>(ctx, 0), a = arg<u32>(ctx, 1), b = arg<u32>(ctx, 2);
    call(ctx, o24fn::Obj_InitVt_B20CC8, {o, 1});
    store<u32>(o + 12, a);
    store<u32>(o + 16, b);
    store<u32>(o, 0x00B21298);
    ret(ctx, 0x00B21298u);
}
// T::T(T* o, u32 a, f32 x): the 0x00B20CC8 base (o, 1), +12 = a, +16 = x,
// vtable 0x00B21358
DECOMP(00439D48, VtB21358_Ctor) {
    const u32 o = arg<u32>(ctx, 0), a = arg<u32>(ctx, 1);
    const f64 x = farg(ctx, 0);
    call(ctx, o24fn::Obj_InitVt_B20CC8, {o, 1});
    store<u32>(o + 12, a);
    store<f32>(o + 16, static_cast<f32>(x));
    store<u32>(o, 0x00B21358);
    ret(ctx, 0x00B21358u);
}
// f(o): 0x004F7128(o, k) for the three globals 0x010AB520 / 0x010AB538 /
// 0x010AB558
DECOMP(0043C4F4, Obj_Register3_0043C4F4) {
    const u32 o = arg<u32>(ctx, 0);
    call(ctx, 0x004F7128, {o, 0x010AB520});
    call(ctx, 0x004F7128, {o, 0x010AB538});
    call(ctx, 0x004F7128, {o, 0x010AB558});
}
// Handle f(T** pp, a) (returned through the hidden out): *out =
// Audio_StartFocusSound(*pp, a) when pp and *pp are set, else 0
DECOMP(0044A374, Audio_Start_0044A374) {
    const u32 out = arg<u32>(ctx, 0), pp = arg<u32>(ctx, 1), a = arg<u32>(ctx, 2);
    u32 p = 0;
    if (pp != 0) p = load<u32>(pp);
    if (p == 0) {
        store<u32>(out, 0);
        ret(ctx, 0u);
        return;
    }
    Stack frame(ctx, 128);
    const u32 t = frame.addr() + 112;
    call(ctx, 0x00444610, {t, p, a});
    const u32 h = load<u32>(t);
    store<u32>(out, h);
    ret(ctx, h);
}
// bool f(const T* p): p's +0 key matches the +0 of one of the entries of
// the table at 0x010AFB84 (count at 0x010AFE90), searched from the end
// (r4..r7 left)
DECOMP(0044D6D8, Table_Contains_0044D6D8) {
    const u32 p = arg<u32>(ctx, 0);
    if (p == 0) {
        ret(ctx, 0u);
        return;
    }
    s32 i = load<s32>(0x010AFE90) - 1;
    ctx.gpr[4] = 0x010B0000;
    ctx.gpr[6] = static_cast<u64>(static_cast<s64>(i));
    if (i < 0) {
        ret(ctx, 0u);
        return;
    }
    const u32 key = load<u32>(p);
    ctx.gpr[4] = 0x010AFB84;
    for (; i >= 0; --i) {
        const u32 v = load<u32>(load<u32>(0x010AFB84 + 4u * static_cast<u32>(i)));
        ctx.gpr[7] = v;
        if (v == key) {
            ctx.gpr[5] = 4u * static_cast<u32>(i);
            ctx.gpr[6] = static_cast<u32>(i);
            ret(ctx, 1u);
            return;
        }
    }
    ctx.gpr[5] = ~0ull - 3;
    ctx.gpr[6] = ~0ull;
    ret(ctx, 0u);
}
// f(T* o): the callback p->+56 (o, (s32)o->+76, p->+60) of p = o->+8 when
// both are set (r6 = the callback, r8 = the value, as passed)
DECOMP(00456258, Obj_Callback_00456258) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 p = load<u32>(o + 8);
    ctx.gpr[4] = p;
    if (p == 0) return;
    const u32 cb = load<u32>(p + 56);
    ctx.gpr[5] = cb;
    if (cb == 0) return;
    const u64 v = static_cast<u64>(static_cast<s64>(load<s32>(o + 76)));
    call_ptr(ctx, cb, {o, v, load<u32>(p + 60), cb, load<u32>(cb), v});
}
// u32 f(u64 key): 257 sets bit 0x10 of 0x010BB9F8, 305 sets 0x4 and 306
// clears it; returns 0x010C0000 (the high half of the address, as left;
// r4 / r5 left)
DECOMP(0049AED8, Input_KeyFlags_0049AED8) {
    const s64 k = static_cast<s64>(ctx.gpr[3]);
    constexpr u32 g = 0x010BB9F8;
    ctx.gpr[4] = ctx.gpr[3];
    if (k > 304) {
        if (k == 305) {
            const u32 w = load<u32>(g) | 0x4u;
            store<u32>(g, w);
            ctx.gpr[4] = w;
        } else if (k == 306) {
            const u32 w = load<u32>(g) & ~0x4u;
            store<u32>(g, w);
            ctx.gpr[4] = 0;
            ctx.gpr[5] = w;
        }
    } else if (k == 257) {
        const u32 w = load<u32>(g) | 0x10u;
        store<u32>(g, w);
        ctx.gpr[4] = w;
    }
    ctx.gpr[3] = 0x010C0000;
}
// T::~T(): vtable 0x00B225B0, Mem_FreeFrom(+304, 0), vtable 0x00B22598,
// then ApeRoot_Destruct(this)
DECOMP(004D22F4, VtB225B0_Dtor) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 p = load<u32>(o + 304);
    store<u32>(o, 0x00B225B0);
    call(ctx, 0x0054EE6C, {p, 0, p});
    store<u32>(o, 0x00B22598);
    call(ctx, 0x00558CE8, {o, 0x00B22598});
}
// bool f(c): false when *(T**)0x010C2C28 is null; else 0x004FA564(c,
// 0x00AF0480, that + 872, 64), its u8 +868 = 1, true
DECOMP(004EF8B8, Global_Read4FA564_004EF8B8) {
    const u32 g = load<u32>(0x010C2C28);
    if (g == 0) {
        ret(ctx, 0u);
        return;
    }
    call(ctx, 0x004FA564, {ctx.gpr[3], 0x00AF0480, g + 872, 64});
    store<u8>(g + 868, 1);
    ret(ctx, 1u);
}
// XmlCursor_Read*(c, String_HashNoCase(name), a, b): the by-name readers
static void XmlReadByName(PPUContext& ctx, u32 reader) {
    const u32 c = arg<u32>(ctx, 0), a = arg<u32>(ctx, 2), b = arg<u32>(ctx, 3);
    const u32 h = static_cast<u32>(call(ctx, o22fn::String_HashNoCase, {arg<u32>(ctx, 1)}));
    call(ctx, reader, {c, h, a, b});
}
// bool f(c, const char* name, bool* out, u8 dflt)
DECOMP(004FA504, XmlCursor_ReadBoolByName) {
    const u32 c = arg<u32>(ctx, 0), a = arg<u32>(ctx, 2), b = arg<u32>(ctx, 3) & 0xFFu;
    const u32 h = static_cast<u32>(call(ctx, o22fn::String_HashNoCase, {arg<u32>(ctx, 1)}));
    call(ctx, 0x004F986C, {c, h, a, b});
}
DECOMP(004FA564, XmlCursor_ReadS32ByName) { XmlReadByName(ctx, 0x004F99BC); }
// f(c, const char* name, f32* out, f32 dflt)
DECOMP(004FA5C4, XmlCursor_ReadF32ByName) {
    const u32 c = arg<u32>(ctx, 0), a = arg<u32>(ctx, 2);
    const f64 d = farg(ctx, 0);
    const u32 h = static_cast<u32>(call(ctx, o22fn::String_HashNoCase, {arg<u32>(ctx, 1)}));
    callf(ctx, 0x004F9B20, {c, h, a}, {d});
}
DECOMP(004FA624, XmlCursor_ReadVec3ByName) { XmlReadByName(ctx, 0x004F9E0C); }
DECOMP(004FA684, XmlCursor_ReadColorByName) { XmlReadByName(ctx, 0x004F9F9C); }
DECOMP(004FA6E4, XmlCursor_ReadHashByName) { XmlReadByName(ctx, 0x004FA14C); }
// f32 Rng_Float(Rng* g): the u64 g->+0 (the last output) times
// *(f32*)0x00734B38, then the state steps as in Rng_Range_0074E23C (r4..r7
// / f2 left)
DECOMP(00734B40, Rng_Float_00734B40) {
    const u32 g = arg<u32>(ctx, 0);
    const u64 prev = load<u64>(g);
    const u32 st = load<u32>(g + 8);
    const u32 s = (std::rotl(st, 4) + std::rotl(st, 16)) ^ 0x87654321u;
    store<u32>(g + 8, std::rotl(s, 16));
    store<u32>(g, 0);
    store<u32>(g + 4, s & 0xFFFFu);
    const f64 x = static_cast<f32>(static_cast<f64>(static_cast<s64>(prev)));
    ctx.gpr[4] = std::rotl(s, 16);
    ctx.gpr[5] = s & 0xFFFFu;
    ctx.gpr[6] = 0x87654321u;
    ctx.gpr[7] = std::rotl(st, 4);
    ctx.fpr[2] = x;
    ctx.fpr[1] = static_cast<f32>(x * LfsAt(0x00734B38));
}
// void f(Array* a, s32 i): removes element i of the 32-byte array a->+0 by
// moving the last one (+0, +4, +8, +16 copied) into it; the u16 count +8
// drops by one (r3..r6 / f1 left)
DECOMP(00735874, Array32_RemoveSwap_00735874) {
    const u32 a = arg<u32>(ctx, 0);
    const u32 i = arg<u32>(ctx, 1);
    const u32 n = load<u16>(a + 8) - 1u;
    const u32 last = n & 0xFFFFu;
    store<u16>(a + 8, static_cast<u16>(n));
    ctx.gpr[4] = n;
    ctx.gpr[5] = i;
    ctx.gpr[6] = last;
    if (i == last) return;
    const u32 base = load<u32>(a);
    const u32 d = base + (i << 5), s = base + (last << 5);
    Cp(d, s);
    Cp(d + 4, s + 4);
    Cp(d + 8, s + 8);
    ctx.fpr[1] = F32Bits(Cp(d + 16, s + 16));
    ctx.gpr[3] = s;
    ctx.gpr[4] = d;
    ctx.gpr[5] = d;
}
// T::T(T* o, a, const Vec3* v): the base 0x004FBFA8(o, a), +80..+88 = v,
// vtable 0x00B0D9D0 (r3 / f1 left)
DECOMP(00736244, VtB0D9D0_Ctor) {
    const u32 o = arg<u32>(ctx, 0), v = arg<u32>(ctx, 2);
    call(ctx, o24fn::Ctor_Base4FBFA8, {o, arg<u32>(ctx, 1), v});
    Cp(o + 80, v);
    Cp(o + 84, v + 4);
    store<u32>(o, 0x00B0D9D0);
    ctx.fpr[1] = F32Bits(Cp(o + 88, v + 8));
    ret(ctx, 0x00B0D9D0u);
}
// T::T(T* o, a, const char* s): the base 0x004FBFA8(o, a), vtable
// 0x00B0DA38, String_CopyN(+72, s, 127)
DECOMP(0073644C, VtB0DA38_Ctor) {
    const u32 o = arg<u32>(ctx, 0), s = arg<u32>(ctx, 2);
    call(ctx, o24fn::Ctor_Base4FBFA8, {o, arg<u32>(ctx, 1), s});
    store<u32>(o, 0x00B0DA38);
    call(ctx, o22fn::String_CopyN, {o + 72, s, 127, 0x00B0DA38});
}
// void f(const T* o, f32* out): out[k], out[k + 1], out[k + 2] = o->+48,
// +52, +56 for k = o->+16->+64 (re-read for each) (r3 / r5..r7 / f1 left)
DECOMP(00736944, Obj_StoreVec48_00736944) {
    const u32 o = arg<u32>(ctx, 0), out = arg<u32>(ctx, 1);
    const u32 i0 = load<u32>(load<u32>(o + 16) + 64) << 2;
    Cp(out + i0, o + 48);
    const u32 i1 = (load<u32>(load<u32>(o + 16) + 64) + 1) << 2;
    Cp(out + i1, o + 52);
    const u32 i2 = (load<u32>(load<u32>(o + 16) + 64) + 2) << 2;
    ctx.fpr[1] = F32Bits(Cp(out + i2, o + 56));
    ctx.gpr[3] = i2;
    ctx.gpr[5] = o + 56;
    ctx.gpr[6] = i1;
    ctx.gpr[7] = o + 52;
}
// void f(T* o, f32 d): o->+348 = clamp(o->+348 + d, k, o->+344) for k =
// *(f32*)0x007376E8 (the upper bound wins; f1..f4 / r4 left)
DECOMP(007376F0, Obj_AddClamped_007376F0) {
    const u32 o = arg<u32>(ctx, 0);
    const f64 d = farg(ctx, 0);
    constexpr u32 kMin = 0x007376E8;
    const f64 hi = LfsAt(o + 344);
    const f64 s = static_cast<f32>(LfsAt(o + 348) + d);
    const f64 m = s < hi ? s : hi;
    const f64 k = LfsAt(kMin);
    ctx.gpr[4] = kMin;
    ctx.fpr[2] = s;
    ctx.fpr[4] = m;
    if (m > k) {
        ctx.fpr[1] = m;
        ctx.fpr[3] = m;
    } else {
        ctx.fpr[1] = hi;
        ctx.fpr[3] = k;
    }
    store<f32>(o + 348, static_cast<f32>(ctx.fpr[3]));
}
// void f(T* o): the u8s +0 / +1 = 255 / 0, +4 / +8 = -1.0f, +12 = 0; the
// word +0 keeps only its low 5 bits under 0xFF (r4..r6 left)
DECOMP(0074B6D8, Obj_Reset_0074B6D8) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 w = 0xFF000000u | (load<u32>(o) & 0x1Fu);
    store<u8>(o, 255);
    store<u8>(o + 1, 0);
    store<u32>(o + 4, 0xBF800000);
    store<u32>(o + 8, 0xBF800000);
    store<u32>(o + 12, 0);
    store<u32>(o, w);
    ctx.gpr[4] = 0xBF800000;
    ctx.gpr[5] = 0;
    ctx.gpr[6] = w;
}
// void f(Array* a, u32 i): removes element i of the u32 array a->+0 by
// shifting the rest down; the u16 count +8 drops by one (r4..r9 left)
DECOMP(0075DC24, U32Array_RemoveAt_0075DC24) {
    const u32 a = arg<u32>(ctx, 0);
    u32 i = arg<u32>(ctx, 1);
    u32 n = load<u16>(a + 8);
    u32 last = n - 1;
    ctx.gpr[6] = last;
    if (i != last) {
        i &= 0xFFFFu;
        while (static_cast<s32>(i) < static_cast<s32>(last)) {
            const u32 off = i << 2;
            const u32 arr = load<u32>(a);
            i = (i + 1) & 0xFFFFu;
            n = load<u16>(a + 8);
            last = n - 1;
            const u32 v = load<u32>(arr + off + 4);
            store<u32>(arr + off, v);
            ctx.gpr[6] = off;
            ctx.gpr[7] = arr;
            ctx.gpr[8] = v;
            ctx.gpr[9] = last;
        }
    }
    store<u16>(a + 8, static_cast<u16>(n - 1));
    ctx.gpr[4] = n - 1;
    ctx.gpr[5] = n;
}
// f(Obj* o, ?, u8 b, f32 x): o's virtual +52 (o, (u8)(s32)(x * k), b) for
// k = *(f32*)0x00760E98 (f1 left as the fctiwz result)
DECOMP(00760EA0, Obj_VCall52Scaled_00760EA0) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 b = arg<u32>(ctx, 2) & 0xFFu;
    const f64 x = static_cast<f32>(farg(ctx, 0) * LfsAt(0x00760E98));
    const u64 bits = rt::fctiw(x, true);
    Stack frame(ctx, 128);
    ctx.fpr[1] = std::bit_cast<f64>(bits | 0xFFF8000000000000ull);  // fctiwz's high word on the PPU
    VCall(ctx, o, 52, {o, static_cast<u32>(bits) & 0xFFu, b});
}
// T::~T() through the second base at +56: as VtB205B8_Dtor for this - 56
DECOMP(0077EC98, VtB205B8_AdjDtor56) {
    const u32 o = arg<u32>(ctx, 0) - 56;
    store<u32>(o, 0x00B205B8);
    store<u32>(o + 56, 0x00B2061C);
    call(ctx, 0x0048F6B0, {o + 56, 0x00B205B8, o + 56, 0x00B2061C});
    call(ctx, 0x00490B34, {o});
}
// f(const T* o): p = o->+28; q = p->+260's virtual +24 (q, p) when p, the
// u8 o->+32 and q are set (r4 left)
DECOMP(00793FE0, Obj_VCall24_00793FE0) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 p = load<u32>(o + 28);
    ctx.gpr[4] = p;
    if (p == 0) return;
    const u32 b = load<u8>(o + 32);
    ret(ctx, b);
    if (b == 0) return;
    const u32 q = load<u32>(p + 260);
    ret(ctx, q);
    if (q == 0) return;
    VCall(ctx, q, 24, {q, p});
}

// ---------------------------------------------------------------- sweep: 25 instructions
// T* f(Obj* o): Obj_FindComponent(o, 13, 0), else Obj_FindComponent(o, 48, 0)
DECOMP(0001CD40, Obj_FindComp13or48_0001CD40) {
    const u32 o = arg<u32>(ctx, 0);
    if (call(ctx, 0x000D1288, {o, 13, 0}) != 0) return;
    call(ctx, 0x000D1288, {o, 48, 0});
}
// f32 f(const Obj* o): k[2] when o->+4 is 36 or 37; else k[0] when bit
// 0x4000 of the u64 o->+60->+184 is set or the top nibble's low 3 bits of
// o->+68->+416 are 1, else k[1] (k = 0x0003A9F0; r3..r5 left)
DECOMP(0003AA00, Obj_SelF32_0003AA00) {
    const u32 o = arg<u32>(ctx, 0);
    constexpr u32 k = 0x0003A9F0;
    ctx.gpr[4] = k;
    const u32 t = load<u32>(o + 4);
    ctx.gpr[5] = t;
    if (t == 36 || t == 37) {
        ctx.fpr[1] = LfsAt(k + 8);
        return;
    }
    const u32 b = static_cast<u32>(load<u64>(load<u32>(o + 60) + 184)) & 0x4000u;
    ctx.gpr[5] = b;
    if (b != 0) {
        ctx.fpr[1] = LfsAt(k);
        return;
    }
    const u32 p = load<u32>(o + 68);
    ctx.gpr[3] = p;
    if (p == 0) {
        ctx.fpr[1] = LfsAt(k + 4);
        return;
    }
    const u32 v = (load<u32>(p + 416) >> 28) & 7u;
    ctx.gpr[3] = v;
    ctx.fpr[1] = LfsAt(v == 1 ? k : k + 4);
}
// f(): Obj_BlendPose0FA660(p; *(f32*)0x0006ED78) and p->+596 = 0.1f for the
// world's +0x1281E0 object p when set
DECOMP(0006ED80, World_BlendPose_0006ED80) {
    const u32 w = load<u32>(kWorldPtr);
    const u32 p = load<u32>(w + 0x1281E0);
    if (p == 0) {
        ctx.gpr[4] = 0x00120000;
        ret(ctx, w);
        return;
    }
    callf(ctx, 0x000FA660, {p, 0x0006ED78}, {LfsAt(0x0006ED78)});
    store<u32>(p + 596, 0x3DCCCCCD);
    ret(ctx, 0x3DCCCCCDu);
}
// f(const T* o): h = Handle_ResolveBit1(o->+12); Effect_SetHash4CF988(h->+20,
// o->+16) when both are set, Hash_ToString(o->+12) when only h is
DECOMP(0007D94C, Obj_EffectHash_0007D94C) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 h = static_cast<u32>(call(ctx, 0x003568E0, {load<u32>(o + 12)}));
    if (h == 0) return;
    const u32 e = load<u32>(h + 20);
    if (e != 0) call(ctx, 0x004CF988, {e, load<u32>(o + 16)});
    else call(ctx, 0x0056A6E0, {load<u32>(o + 12)});
}
// f(const T* o): 0x0044D1E8(o->+16) when set; else 0x0044D168(o->+12,
// o->+20 (u8)) when that is set; else Emitters_StopAll(o)
DECOMP(00096334, Obj_Stop_00096334) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 p = load<u32>(o + 16);
    if (p != 0) {
        call(ctx, 0x0044D1E8, {p, p});
        return;
    }
    const u32 q = load<u32>(o + 12);
    if (q != 0) call(ctx, 0x0044D168, {q, load<u8>(o + 20), o});
    else call(ctx, 0x00449950, {o, 0});
}
// f(): 0x0042C138(new T (12 bytes, Obj_InitVt_B21370) or 0)
DECOMP(000A635C, NewB21370_Submit_000A635C) {
    const u32 p = static_cast<u32>(call(ctx, o19fn::SmallAlloc, {12}));
    if (p != 0) call(ctx, 0x00439DC4, {p});
    call(ctx, 0x0042C138, {p});
}
// f(a, b, Obj* o): o's virtual +236 (o, a, b) when o is set (r6 = o, then
// the OPD and its entry, as passed); else 0
DECOMP(000DD4A0, Obj_VCall236_000DD4A0) {
    const u32 a = arg<u32>(ctx, 0), b = arg<u32>(ctx, 1), o = arg<u32>(ctx, 2);
    if (o == 0) {
        ctx.gpr[5] = a;
        ctx.gpr[6] = 0;
        return;
    }
    const u32 opd = load<u32>(load<u32>(o) + 236);
    call_ptr(ctx, opd, {o, a, b, o, opd, load<u32>(opd)});
}
// void f(T* d): d->+0..+8 = the Vec3 at 0x01033250, +16..+24 = 0x010332B0,
// +32 = *(f32*)0x010333EC (r4..r7 / f1 left)
DECOMP(001072BC, Cfg_Copy_001072BC) {
    const u32 d = arg<u32>(ctx, 0);
    Cp(d, 0x01033200 + 80);
    Cp(d + 4, 0x01033250 + 4);
    Cp(d + 8, 0x01033250 + 8);
    Cp(d + 16, 0x01033260 + 80);
    Cp(d + 20, 0x010332B0 + 4);
    Cp(d + 24, 0x010332B0 + 8);
    ctx.fpr[1] = F32Bits(Cp(d + 32, 0x010333A4 + 72));
    ctx.gpr[4] = 0x01033260;
    ctx.gpr[5] = 0x010333A4;
    ctx.gpr[6] = 0x010332B0;
    ctx.gpr[7] = 0x01030000;
}
// u32 f(const T* o): the +192 of the first 240-byte entry of the array
// o->+340 (count the u16 +8) whose +160 is 0x03D69F9C, else 0 (r4..r9 left)
DECOMP(0011F2C8, Obj_FindEntry_0011F2C8) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 arr = load<u32>(o + 340);
    constexpr u32 kKey = 0x03D69F9C;
    ctx.gpr[4] = kKey;
    ctx.gpr[5] = 0;
    u32 i = 0;
    for (;; ++i) {
        const u32 n = arr != 0 ? load<u16>(arr + 8) : 0u;
        ctx.gpr[6] = i * 240u;
        ctx.gpr[7] = i;
        ctx.gpr[8] = n;
        if (i >= n) break;
        const u32 e = load<u32>(arr) + i * 240u;
        const u32 v = load<u32>(e + 160);
        ctx.gpr[8] = e;
        ctx.gpr[9] = v;
        if (v == kKey) {
            ret(ctx, load<u32>(e + 192));
            return;
        }
    }
    ret(ctx, 0u);
}
// f(const T* a, U* b): Thunk_000E44C0(a, b), then b->+208..+220 = a->+240..
// +252 and the u8 b->+224 = a->+256 (r3 / f1 left)
DECOMP(001AA57C, Obj_CopyState_001AA57C) {
    const u32 a = arg<u32>(ctx, 0), b = arg<u32>(ctx, 1);
    call(ctx, 0x000E44C0, {a, b});
    Cp(b + 208, a + 240);
    Cp(b + 212, a + 244);
    const u32 c = load<u8>(a + 256);
    Cp(b + 216, a + 248);
    const u32 w = load<u32>(a + 252);
    store<u8>(b + 224, static_cast<u8>(c));
    store<u32>(b + 220, w);
    ctx.fpr[1] = F32Bits(w);
    ret(ctx, c);
}
// f(const T* a, U* b): Thunk_000E44C0(a, b), then b->+208..+224 = a->+240..
// +256 (f1 left)
DECOMP(001B29B8, Obj_CopyState_001B29B8) {
    const u32 a = arg<u32>(ctx, 0), b = arg<u32>(ctx, 1);
    call(ctx, 0x000E44C0, {a, b});
    for (u32 k = 0; k < 16; k += 4) Cp(b + 208 + k, a + 240 + k);
    ctx.fpr[1] = F32Bits(Cp(b + 224, a + 256));
}
// f(const T* a, U* b): Thunk_000E44C0(a, b), then b->+208 = a->+240,
// +224..+232 = a->+256..+264, +240 = a->+272 (r3 / f1 left)
DECOMP(001E6F3C, Obj_CopyState_001E6F3C) {
    const u32 a = arg<u32>(ctx, 0), b = arg<u32>(ctx, 1);
    call(ctx, 0x000E44C0, {a, b});
    const u32 x = load<u32>(a + 256), w = load<u32>(a + 240);
    store<u32>(b + 224, x);
    store<u32>(b + 208, w);
    const u32 y = load<u32>(a + 260), v = load<u32>(a + 272);
    store<u32>(b + 228, y);
    const u32 z = load<u32>(a + 264);
    store<u32>(b + 240, v);
    store<u32>(b + 232, z);
    ctx.fpr[1] = F32Bits(z);
    ret(ctx, v);
}
// T::~T(): vtable 0x00B19148, AnimSkel_Release7D4EA8(+200),
// RefNode_Release(+204), vtable 0x00B11D98, base 0x000C2DA0(this)
DECOMP(00203614, VtB19148_Dtor) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 p = load<u32>(o + 200);
    store<u32>(o, 0x00B19148);
    call(ctx, 0x007D4EA8, {p, 0x00B19148});
    call(ctx, o24fn::RefNode_Release, {load<u32>(o + 204)});
    store<u32>(o, 0x00B11D98);
    call(ctx, 0x000C2DA0, {o, 0x00B11D98});
}
// f(Obj* o): 0x000DD504(o); then Effect_SetHash4CF988(+428, 0xE69B8987) and
// bit 0x100 of its +136 cleared when +428 is set (r4 / r5 left)
DECOMP(0020BAC8, Obj_EffectOn_0020BAC8) {
    const u32 o = arg<u32>(ctx, 0);
    call(ctx, 0x000DD504, {o});
    const u32 e = load<u32>(o + 428);
    ret(ctx, e);
    if (e == 0) return;
    call(ctx, 0x004CF988, {e, 0xE69B8987u});
    const u32 e2 = load<u32>(o + 428);
    const u32 w = load<u32>(e2 + 136) & ~0x100u;
    store<u32>(e2 + 136, w);
    ctx.gpr[4] = 0;
    ctx.gpr[5] = w;
    ret(ctx, e2);
}
// f(Obj* o): 0x000DD620(o); then bit 0x100 of +428->+136 set and
// Effect_Release(+428, 0xD5E4206F) when +428 is set
DECOMP(0020BB2C, Obj_EffectOff_0020BB2C) {
    const u32 o = arg<u32>(ctx, 0);
    call(ctx, 0x000DD620, {o});
    const u32 e = load<u32>(o + 428);
    ret(ctx, e);
    if (e == 0) return;
    const u32 w = load<u32>(e + 136) | 0x100u;
    store<u32>(e + 136, w);
    call(ctx, 0x004CF3C4, {load<u32>(o + 428), 0xD5E4206Fu, w});
}
// void f(T* o): the three Vec3s +96 / +112 / +128 / +144 copied to +32 /
// +48 / +64 / +80 (f1 / f2 left)
DECOMP(002439EC, Obj_CopyMatrix_002439EC) {
    const u32 o = arg<u32>(ctx, 0);
    u32 w = 0, v = 0;
    for (u32 r = 0; r < 4; ++r) {
        Cp(o + 32 + 16 * r, o + 96 + 16 * r);
        w = Cp(o + 36 + 16 * r, o + 100 + 16 * r);
        v = Cp(o + 40 + 16 * r, o + 104 + 16 * r);
    }
    ctx.fpr[1] = F32Bits(w);
    ctx.fpr[2] = F32Bits(v);
}
// void f(T* o): +64 = the Vec3 at 0x01164B40, then +608 = +64, +592 = +608
// (r4..r6 / f1 left)
DECOMP(0026D9D4, Obj_ResetPos_0026D9D4) {
    const u32 o = arg<u32>(ctx, 0);
    for (u32 k = 0; k < 12; k += 4) Cp(o + 64 + k, 0x01164B40 + k);
    for (u32 k = 0; k < 12; k += 4) Cp(o + 608 + k, o + 64 + k);
    u32 w = 0;
    for (u32 k = 0; k < 12; k += 4) w = Cp(o + 592 + k, o + 608 + k);
    ctx.fpr[1] = F32Bits(w);
    ctx.gpr[4] = o + 64;
    ctx.gpr[5] = o + 608;
    ctx.gpr[6] = o + 608;
}
// f(const T* o, ?, k, f32 x): 0x003541E0(a; x) for a = Anim_Find34CC34(
// o->+36, k) when both are set; returns k when o->+36 is null
DECOMP(00276E4C, Obj_AnimCall3541E0_00276E4C) {
    const u32 o = arg<u32>(ctx, 0), k = arg<u32>(ctx, 2);
    const f64 x = farg(ctx, 0);
    const u32 q = load<u32>(o + 36);
    ctx.gpr[4] = q;
    if (q == 0) {
        ret(ctx, k);
        return;
    }
    const u32 a = static_cast<u32>(call(ctx, 0x0034CC34, {q, k, q}));
    if (a == 0) return;
    callf(ctx, 0x003541E0, {a}, {x});
}
// T* f(T* o): o->+1240, created on first use as o->+8's virtual +100 (p)
DECOMP(00294748, Obj_LazyGet1240_00294748) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 r = load<u32>(o + 1240);
    ret(ctx, r);
    if (r != 0) return;
    const u32 p = load<u32>(o + 8);
    ret(ctx, p);
    if (p == 0) return;
    const u32 v = static_cast<u32>(VCall(ctx, p, 100, {p}));
    store<u32>(o + 1240, v);
}
// void f(const T* o, u8 on): the world slot's (world + 80 + (world->+4208
// (u8)) * 4056) u8 +336 = (on == 1) and +340 = o->+36->+724 (0 when off or
// o->+36 is null) (r3..r6 left)
DECOMP(002E6DC4, World_SetSlot336_002E6DC4) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 on = arg<u32>(ctx, 1) & 0xFFu;
    const u32 w = load<u32>(kWorldPtr);
    const u32 slot = load<u8>(w + 4208) * 4056u;
    const u32 s = w + slot + 80;
    ctx.gpr[4] = s;
    u32 v = 0;
    if (on == 1) {
        store<u8>(s + 336, 1);
        const u32 c = load<u32>(o + 36);
        if (c != 0) v = load<u32>(c + 724);
        ctx.gpr[5] = c;
        ctx.gpr[6] = 1;
    } else {
        store<u8>(s + 336, 0);
        ctx.gpr[5] = w;
        ctx.gpr[6] = slot;
    }
    store<u32>(s + 340, v);
    ret(ctx, v);
}
// T::~T(): vtable 0x00B1E3E0, ~PtrVector(+3248), base 0x003286D8(this)
DECOMP(003291DC, VtB1E3E0_Dtor) { TDtorVec(ctx, 0x00B1E3E0, 3248, 0x003286D8); }
// u32 f(const Obj* o): r = 0x004E2B60(p) for p = o->+328->+12 when o,
// o->+8->+152 and p are set; p->+180 bits 22..25 = r (r4 / r5 left)
DECOMP(003676DC, Obj_SetField180_003676DC) {
    const u32 o = arg<u32>(ctx, 0);
    if (o == 0) return;
    const u32 f = load<u32>(load<u32>(o + 8) + 152);
    ctx.gpr[5] = f;
    if (f == 0) return;
    const u32 p = load<u32>(load<u32>(o + 328) + 12);
    if (p == 0) return;
    const u32 r = static_cast<u32>(call(ctx, 0x004E2B60, {p}));
    const u32 w = (load<u32>(p + 180) & ~0x03C00000u) | ((r << 22) & 0x03C00000u);
    store<u32>(p + 180, w);
    ctx.gpr[4] = w;
}
// f(T* o, u8 b): o->+24's virtual +40 (p, b, 1) when set, then 0x0036C898(o)
DECOMP(0036CA0C, Obj_VCall40Then_0036CA0C) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 b = arg<u32>(ctx, 1) & 0xFFu;
    const u32 p = load<u32>(o + 24);
    if (p != 0) VCall(ctx, p, 40, {p, b, 1});
    call(ctx, 0x0036C898, {o});
}
// f(Obj* o, u8 on, u8 b): on: Obj_SetMode368870(o, ., 0, 0; *(f32*)0x003846E4)
// unless bit 0x40000000 of o->+532 is set; off: Obj_ModeOff368AC8(o, b, 0)
DECOMP(003846E8, Obj_SetMode_003846E8) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 on = arg<u32>(ctx, 1) & 0xFFu, b = arg<u32>(ctx, 2) & 0xFFu;
    ctx.gpr[6] = ctx.gpr[4];
    if (on == 0) {
        call(ctx, 0x00368AC8, {o, b, 0});
        return;
    }
    const u32 f = (load<u32>(o + 532) >> 30) & 1u;
    if (f != 0) {
        ctx.gpr[4] = f;
        ctx.gpr[5] = on;
        return;
    }
    callf(ctx, 0x00368870, {o, 0x003846E4, 0, 0}, {LfsAt(0x003846E4)});
}
// f(Obj* o, ?, s32 m): Effect_SetHash4CF988(o->+32, h) for h = o->+60->+4984
// (m == 1) or +4996 (m == 0) when h and o->+32 are set (r4 left)
DECOMP(003BC6A8, Obj_EffectByMode_003BC6A8) {
    const u32 o = arg<u32>(ctx, 0);
    const s32 m = arg<s32>(ctx, 2);
    if (m != 0 && m != 1) return;
    const u32 h = load<u32>(load<u32>(o + 60) + (m == 1 ? 4984u : 4996u));
    ctx.gpr[4] = h;
    if (h == 0) return;
    const u32 e = load<u32>(o + 32);
    ret(ctx, e);
    if (e == 0) return;
    call(ctx, 0x004CF988, {e, h});
}
// f(T* o): 0x002F0E0C(*(T**)0x00F64C38, o + 12, o + 128) when o->+20 is
// set, then Flag_Set8_40000000(o)
DECOMP(0042F92C, Obj_Call2F0E0C_0042F92C) {
    const u32 o = arg<u32>(ctx, 0);
    if (load<u32>(o + 20) != 0) call(ctx, 0x002F0E0C, {load<u32>(0x00F64C38), o + 12, o + 128});
    call(ctx, o24fn::Flag_Set8_40000000, {o});
}
// T::T(T* o, bool a, u8 on): the 0x00B20CC8 base (o, on), +12 = 0, bit
// 0x80000000 of +16 = a, vtable 0x00B212B0 (r3..r5 left)
DECOMP(004389D4, VtB212B0_Ctor) {
    const u32 o = arg<u32>(ctx, 0), a = arg<u32>(ctx, 1);
    call(ctx, o24fn::Obj_InitVt_B20CC8, {o, arg<u32>(ctx, 2) & 0xFFu});
    const u32 w = (load<u32>(o + 16) & 0x7FFFFFFFu) | (a << 31);
    store<u32>(o + 12, 0);
    store<u32>(o, 0x00B212B0);
    store<u32>(o + 16, w);
    ctx.gpr[4] = w;
    ctx.gpr[5] = 0;
    ret(ctx, 0x00B212B0u);
}
// Node* f(const T* o, const u32* key): the first node of the list o->+8200
// (next +120) whose u8 +101 is clear and whose +0->+0 is *key; 0 for a
// null key (r5..r7 left)
DECOMP(004464F0, List_FindByKey_004464F0) {
    const u32 o = arg<u32>(ctx, 0), kp = arg<u32>(ctx, 1);
    ctx.gpr[5] = 0;
    const u32 key = kp != 0 ? load<u32>(kp) : 0u;
    if (key == 0) {
        ctx.gpr[4] = key;
        ret(ctx, 0u);
        return;
    }
    ctx.gpr[4] = key;
    for (u32 n = load<u32>(o + 8200); n != 0; n = load<u32>(n + 120)) {
        const u32 skip = load<u8>(n + 101);
        ctx.gpr[6] = skip;
        if (skip == 0) {
            const u32 q = load<u32>(n);
            ctx.gpr[7] = q;
            const u32 v = q != 0 ? load<u32>(q) : 0u;
            ctx.gpr[6] = v;
            if (v == key) {
                ret(ctx, n);
                return;
            }
        }
    }
    ret(ctx, 0u);
}
// f(Obj* o, const Event* e): o's virtual +28 (o, 0) for the event
// 0x879F2BDA whose +12 is o->+40 (r4..r6 left otherwise)
DECOMP(00455B18, Obj_OnEvent_00455B18) {
    const u32 o = arg<u32>(ctx, 0), e = arg<u32>(ctx, 1);
    const u32 t = load<u32>(e + 8);
    ctx.gpr[5] = 0x879F2BDAu;
    ctx.gpr[6] = t;
    if (t != 0x879F2BDAu) return;
    const u32 a = load<u32>(e + 12), b = load<u32>(o + 40);
    ctx.gpr[4] = a;
    ctx.gpr[5] = b;
    if (a != b) return;
    VCall(ctx, o, 28, {o, 0});
}
// f32 f(const T* o, s32 i): when o->+28 is set and bit i of o->+20 is,
// (o->+28 + 32)[i] for i < 24, else 0.0f (*(f32*)0x0045B158) (r3 / r4 / r6
// left)
DECOMP(0045B160, Obj_GetChannel_0045B160) {
    const u32 o = arg<u32>(ctx, 0);
    const s32 i = arg<s32>(ctx, 1);
    constexpr u32 k = 0x0045B158;
    const u32 p = load<u32>(o + 28);
    ctx.gpr[5] = p;
    if (p != 0) {
        const u32 sh = static_cast<u32>(i) & 0x3Fu;
        const u32 bit = sh >= 32 ? 0u : (1u << sh);
        ctx.gpr[6] = bit;
        if ((load<u32>(o + 20) & bit) != 0) {
            ctx.gpr[4] = k;
            ctx.gpr[3] = static_cast<u32>(i);
            ctx.fpr[1] = LfsAt(k);
            if (i < 24) {
                ctx.gpr[4] = p + 32;
                ctx.gpr[3] = static_cast<u64>(static_cast<s64>(i * 4));
                ctx.fpr[1] = LfsAt(p + 32 + static_cast<u32>(i * 4));
            }
            return;
        }
    }
    ctx.gpr[3] = k;
    ctx.fpr[1] = LfsAt(k);
}
// void f(T* d, u32 a, f32 x0..x6, u32 b, bool c, bool e, bool g): +0 = a,
// +4..+28 = x0..x6, +32 = b, +36 = 0; +40 keeps bits 4..28 with bit 31 =
// c, 30 = e, 29 set, 28 = g (the last four come on the stack; r4..r9 left)
DECOMP(0048EF9C, Rec_Init_0048EF9C) {
    const u32 d = arg<u32>(ctx, 0);
    const u32 sp = static_cast<u32>(ctx.gpr[1]);
    const u32 w = load<u32>(d + 40);
    store<u32>(d, arg<u32>(ctx, 1));
    const u32 c = load<u32>(sp + 132), e = load<u32>(sp + 140), g = load<u32>(sp + 148), b = load<u32>(sp + 124);
    u32 v = (w & 0x7FFFFFFFu) | (c << 31);
    v = (v & ~0x40000000u) | ((e & 1u) << 30);
    v |= 0x20000000u;
    v = (v & ~0x10000000u) | ((g & 1u) << 28);
    for (int k = 0; k < 7; ++k) store<f32>(d + 4 + 4 * k, static_cast<f32>(farg(ctx, k)));
    store<u32>(d + 32, b);
    store<u32>(d + 36, 0);
    store<u32>(d + 40, v);
    ctx.gpr[4] = 0;
    ctx.gpr[5] = b;
    ctx.gpr[6] = v;
    ctx.gpr[7] = c;
    ctx.gpr[8] = e;
    ctx.gpr[9] = g;
}
// void f(T* o, f32 dt): moves o->+0 toward the target +4 by +8 * dt,
// landing on the target when it is that close (f1..f5 / r4 left)
DECOMP(004A11E0, Value_Approach_004A11E0) {
    const u32 o = arg<u32>(ctx, 0);
    const f64 dt = farg(ctx, 0);
    const f64 tgt = LfsAt(o + 4), cur = LfsAt(o), rate = LfsAt(o + 8);
    ctx.fpr[2] = cur;
    ctx.fpr[3] = rate;
    ctx.fpr[4] = tgt;
    if (cur == tgt) return;
    const f64 d = static_cast<f32>(tgt - cur);
    f64 step = static_cast<f32>(rate * dt);
    const f64 ad = std::fabs(d);
    ctx.fpr[1] = d;
    ctx.fpr[5] = ad;
    if (ad < step || ad == step) {
        ctx.fpr[3] = step;
        Cp(o, o + 4);
        return;
    }
    constexpr u32 kZero = 0x004A11DC;
    const f64 z = LfsAt(kZero);
    ctx.gpr[4] = kZero;
    ctx.fpr[4] = z;
    if (d < z) step = -step;
    ctx.fpr[3] = step;
    const f32 r = static_cast<f32>(cur + step);
    store<f32>(o, r);
    ctx.fpr[1] = r;
}
// Vec3 f(Vec3* out, const Vec3* p): View_ProjectPoint(the world's
// +0x128168 view, &s, p); out = {s.x, s.y, 0} (f1 / f2 left)
DECOMP(004A5854, View_Project2D_004A5854) {
    const u32 out = arg<u32>(ctx, 0), p = arg<u32>(ctx, 1);
    Stack frame(ctx, 144);
    const u32 t = frame.addr() + 112;
    const u32 view = load<u32>(load<u32>(kWorldPtr) + 0x128168);
    call(ctx, 0x00522750, {view, t, p});
    const u32 x = load<u32>(t), y = load<u32>(t + 4);
    store<u32>(out, x);
    store<u32>(out + 4, y);
    store<u32>(out + 8, 0);
    ctx.fpr[1] = F32Bits(x);
    ctx.fpr[2] = F32Bits(y);
    ret(ctx, 0u);
}
// void f(T* o): +0 = the Vec3 at 0x010C1F00, +16..+36 = 0, +40 = 1.0f, the
// u8s +44 = 1, +45..+47 = 0 (r4..r7 / f1 left)
DECOMP(004AE360, Obj_Reset_004AE360) {
    const u32 o = arg<u32>(ctx, 0);
    Cp(o, 0x010C1EB0 + 80);
    store<u32>(o + 16, 0);
    Cp(o + 4, 0x010C1F00 + 4);
    store<u32>(o + 20, 0);
    ctx.fpr[1] = F32Bits(Cp(o + 8, 0x010C1F00 + 8));
    for (u32 k = 24; k <= 36; k += 4) store<u32>(o + k, 0);
    store<u32>(o + 40, 0x3F800000);
    store<u8>(o + 44, 1);
    store<u8>(o + 45, 0);
    store<u8>(o + 46, 0);
    store<u8>(o + 47, 0);
    ctx.gpr[4] = 1;
    ctx.gpr[5] = 0x010C1F00;
    ctx.gpr[6] = 0;
    ctx.gpr[7] = 0x3F800000;
}
// void f(T* o): when o->+180 is set: ~PtrVector(it), SmallFree(it, 20)
DECOMP(00733CEC, Obj_FreeVec180_00733CEC) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 v = load<u32>(o + 180);
    if (v == 0) return;
    VecDtor(ctx, v);
    call(ctx, o19fn::SmallFree, {load<u32>(o + 180), 20});
}
// void f(T* d, const Vec3* a, const Vec3* b, const Vec3* c, const Vec3* e):
// the four rows +0 / +16 / +32 / +48 (f1 left)
DECOMP(00735980, Mat_SetRows_00735980) {
    const u32 d = arg<u32>(ctx, 0);
    u32 w = 0;
    for (u32 r = 0; r < 4; ++r) {
        const u32 s = arg<u32>(ctx, 1 + static_cast<int>(r));
        for (u32 k = 0; k < 12; k += 4) w = Cp(d + 16 * r + k, s + k);
    }
    ctx.fpr[1] = F32Bits(w);
}
// bool Vec3_NearEq(const Vec3* a, const Vec3* b, f32 eps): every |a - b|
// component < eps (f2 / f3 / r5 left)
DECOMP(00748A98, Vec3_NearEq_00748A98) {
    const u32 a = arg<u32>(ctx, 0), b = arg<u32>(ctx, 1);
    const f64 eps = farg(ctx, 0);
    ctx.gpr[5] = 0;
    for (u32 k = 0; k < 12; k += 4) {
        const f32 bk = load<f32>(b + k);
        const f64 d = std::fabs(static_cast<f64>(static_cast<f32>(load<f32>(a + k) - bk)));
        ctx.fpr[2] = d;
        ctx.fpr[3] = bk;
        if (!(d < eps)) {
            ret(ctx, 0u);
            return;
        }
    }
    ctx.gpr[5] = 1;
    ret(ctx, 1u);
}
// void f(T* o, const Vec3* v, u8 set): set: +672 = v and bit 0x00800000 of
// +416 set; otherwise +672 += v (f1..f3 / r5 left)
DECOMP(0074B5F8, Obj_SetOrAddVec672_0074B5F8) {
    const u32 o = arg<u32>(ctx, 0), v = arg<u32>(ctx, 1);
    const u32 set = arg<u32>(ctx, 2) & 0xFFu;
    if (set != 0) {
        const u32 x = load<u32>(v);
        const u32 w = load<u32>(o + 416) | 0x00800000u;
        store<u32>(o + 672, x);
        Cp(o + 676, v + 4);
        const u32 z = load<u32>(v + 8);
        store<u32>(o + 416, w);
        store<u32>(o + 680, z);
        ctx.gpr[5] = w;
        ctx.fpr[1] = F32Bits(z);
        return;
    }
    const f32 x = load<f32>(o + 672) + load<f32>(v);
    const f32 y0 = load<f32>(o + 676), z0 = load<f32>(o + 680);
    store<f32>(o + 672, x);
    const f32 y = y0 + load<f32>(v + 4);
    store<f32>(o + 676, y);
    const f32 z = z0 + load<f32>(v + 8);
    store<f32>(o + 680, z);
    ctx.gpr[5] = set;
    ctx.fpr[1] = z;
    ctx.fpr[2] = y0;
    ctx.fpr[3] = z0;
}
// u64 f(u64* out, const T* t, s32 key): the u64 +8 of the first 16-byte
// entry of t->+124 (count t->+128) whose +0 is key, else 0 (r4 / r6..r9
// left)
DECOMP(0074C738, Map_Get64_0074C738) {
    const u32 out = arg<u32>(ctx, 0), t = arg<u32>(ctx, 1), key = arg<u32>(ctx, 2);
    const s32 n = load<s32>(t + 128);
    const u32 arr = load<u32>(t + 124);
    ctx.gpr[6] = arr;
    ctx.gpr[7] = static_cast<u32>(n);
    ctx.gpr[8] = 0;
    for (s32 i = 0; i < n; ++i) {
        const u32 e = arr + 16u * static_cast<u32>(i);
        const u32 v = load<u32>(e);
        ctx.gpr[8] = static_cast<u32>(i);
        ctx.gpr[9] = v;
        if (v == key) {
            const u64 r = load<u64>(e + 8);
            store<u64>(out, r);
            ctx.gpr[4] = r;
            return;
        }
    }
    if (n > 0) ctx.gpr[8] = static_cast<u32>(n);
    store<u64>(out, 0);
    ctx.gpr[4] = 0;
}
// void f(T* d): +0 = the Vec3 at 0x01164B40, +16..+28 = the 4 words at
// 0x01164C90, +32..+40 = 0, the s16 +44 = -1 (r4..r7 / f1 left)
DECOMP(007614B4, Obj_Reset_007614B4) {
    const u32 d = arg<u32>(ctx, 0);
    for (u32 k = 0; k < 12; k += 4) Cp(d + k, 0x01164B40 + k);
    Cp(d + 16, 0x01164C90);
    Cp(d + 20, 0x01164C90 + 4);
    const u32 x = load<u32>(0x01164C90 + 8);
    store<u32>(d + 32, 0);
    store<u32>(d + 24, x);
    const u32 y = load<u32>(0x01164C90 + 12);
    store<u32>(d + 36, 0);
    store<u32>(d + 28, y);
    store<u32>(d + 40, 0);
    store<u16>(d + 44, 0xFFFF);
    ctx.fpr[1] = F32Bits(y);
    ctx.gpr[4] = 0;
    ctx.gpr[5] = ~0ull;
    ctx.gpr[6] = 0x01160000;
    ctx.gpr[7] = 0x01164C90;
}
// T* New(): SmallAlloc(92), 0x004F6ED8(p) and vtable vt; 0 on failure
static void New92(PPUContext& ctx, u32 vt) {
    const u32 p = static_cast<u32>(call(ctx, o19fn::SmallAlloc, {92, 0}));
    if (p == 0) {
        ret(ctx, 0u);
        return;
    }
    call(ctx, 0x004F6ED8, {p});
    store<u32>(p, vt);
    ctx.gpr[4] = vt;
    ret(ctx, p);
}
DECOMP(00782504, NewB21560_00782504) { New92(ctx, 0x00B21560); }
DECOMP(00782634, NewB215A8_00782634) { New92(ctx, 0x00B215A8); }
// bool f(?, ?, u32* out, const Node* n, f32 key): the binary tree under n
// (+4 key, +8 value, +12 left, +16 right): *out = the value for key (r4 /
// r5 / f2 left)
DECOMP(00790920, Tree_FindF32_00790920) {
    const u32 out = arg<u32>(ctx, 2);
    u32 n = arg<u32>(ctx, 3);
    const f64 key = farg(ctx, 0);
    ctx.gpr[4] = out;
    while (n != 0) {
        const f64 v = LfsAt(n + 4);
        ctx.fpr[2] = v;
        if (v == key) {
            const u32 r = load<u32>(n + 8);
            store<u32>(out, r);
            ctx.gpr[5] = r;
            ret(ctx, 1u);
            return;
        }
        n = key < v ? load<u32>(n + 12) : load<u32>(n + 16);
    }
    ret(ctx, 0u);
}

// ---------------------------------------------------------------- sweep: 26 instructions
// f(Obj* o, a): ++o->+24 while it stays below 4 (unsigned), then o's
// virtual +220 (o, a)
DECOMP(00014D0C, Obj_CountedVCall220_00014D0C) {
    const u32 o = arg<u32>(ctx, 0), a = arg<u32>(ctx, 1);
    const u32 n = load<u32>(o + 24) + 1;
    if (n < 4) store<u32>(o + 24, n);
    VCall(ctx, o, 220, {o, a});
}
// f(const T* o, f32 x, const U* b): when x > *(f32*)0x00024AD4, o->+60's
// virtual +352 (p, ., b->+4, 0) (r4 holds the OPD, as the original passes
// it); otherwise returns b (r4 = o, f2 = the bound left)
DECOMP(00024AD8, Obj_VCall352_00024AD8) {
    const u32 o = arg<u32>(ctx, 0), b = arg<u32>(ctx, 2);
    const f64 x = farg(ctx, 0), k = LfsAt(0x00024AD4);
    ctx.fpr[2] = k;
    if (!(x > k)) {
        ctx.gpr[4] = o;
        ret(ctx, b);
        return;
    }
    const u32 v = load<u32>(b + 4);
    const u32 p = load<u32>(o + 60);
    const u32 opd = load<u32>(load<u32>(p) + 352);
    call_ptr(ctx, opd, {p, opd, v, 0, load<u32>(opd)});
}
// T::~T(): vtable 0x00B0E5B8, 0x0002EC04(this), +64->+36's +52 / +56 = 0,
// vtable 0x00B0DC70, then NavQuery_Free62D08C(+216)
DECOMP(0002ED1C, VtB0E5B8_Dtor) {
    const u32 o = arg<u32>(ctx, 0);
    store<u32>(o, 0x00B0E5B8);
    call(ctx, 0x0002EC04, {o, 0x00B0E5B8});
    const u32 q = load<u32>(load<u32>(o + 64) + 36);
    store<u32>(q + 52, 0);
    store<u32>(q + 56, 0);
    store<u32>(o, 0x00B0DC70);
    call(ctx, 0x0062D08C, {o + 216, 0, 0x00B0DC70, o + 216, q});
}
// bool f(?, s32 k): false for 1..4, 7, 8, 12 and 28, true otherwise (r3 =
// k compared; r4 left)
DECOMP(000627BC, Kind_Allowed_000627BC) {
    const s32 k = arg<s32>(ctx, 1);
    const bool no = (k >= 1 && k <= 4) || k == 7 || k == 8 || k == 12 || k == 28;
    ret(ctx, no ? 0u : 1u);
}
// f(const T* a): Player_StartAction(world, world->+4216, a + 12, a + 141,
// 0, 0, 0, 0, a->+270 (u8), a->+271 (u8))
DECOMP(00092B24, Player_StartAction_00092B24) {
    const u32 a = arg<u32>(ctx, 0);
    const u32 w = load<u32>(kWorldPtr);
    call(ctx, 0x00489A60, {w, load<u32>(w + 4216), a + 12, a + 141, 0, 0, 0, 0, load<u8>(a + 270), load<u8>(a + 271)});
}
// f(Obj* o, a): o's virtual +28 (o, a), 0 for a null o
DECOMP(000D6CC0, Obj_VCall28_000D6CC0) {
    const u32 o = arg<u32>(ctx, 0), a = arg<u32>(ctx, 1);
    if (o == 0) {
        ctx.gpr[4] = 0;
        ctx.gpr[5] = 0;
        ctx.gpr[6] = a;
        ret(ctx, 0u);
        return;
    }
    VCall(ctx, o, 28, {o, a});
}
// bool f(const Obj* o, k): bit 0x08000000 of Anim_Find34CC34(o->+164, k)->+12
// (false when either is null)
DECOMP(000D86F8, Obj_AnimFlag_000D86F8) {
    const u32 o = arg<u32>(ctx, 0), k = arg<u32>(ctx, 1);
    const u32 q = load<u32>(o + 164);
    if (q == 0) {
        ret(ctx, 0u);
        return;
    }
    const u32 a = static_cast<u32>(call(ctx, 0x0034CC34, {q, k, q}));
    ret(ctx, a != 0 ? (load<u32>(a + 12) >> 27) & 1u : 0u);
}
// f(Obj* o, u32 h, u8 b): Effect_SetHash4CF988(o->+off, h) when that is
// set, then 0x000DA684(o, h, b)
static void EffectHashThenDA684(PPUContext& ctx, u32 off) {
    const u32 o = arg<u32>(ctx, 0), h = arg<u32>(ctx, 1);
    const u32 b = arg<u32>(ctx, 2) & 0xFFu;
    const u32 e = load<u32>(o + off);
    if (e != 0) call(ctx, 0x004CF988, {e, h});
    call(ctx, 0x000DA684, {o, h, b});
}
DECOMP(000DEC64, Obj_EffectHash352_000DEC64) { EffectHashThenDA684(ctx, 352); }
DECOMP(000E0B64, Obj_EffectHash356_000E0B64) { EffectHashThenDA684(ctx, 356); }
// f(Obj* o, a): bit 0x20000000 of o->+376 set, then o->+244's virtual +208
// (p, a) when set; else returns a (r4..r6 left)
DECOMP(000DF7E0, Obj_VCall208_000DF7E0) {
    const u32 o = arg<u32>(ctx, 0), a = arg<u32>(ctx, 1);
    const u32 w = load<u32>(o + 376) | 0x20000000u;
    store<u32>(o + 376, w);
    const u32 p = load<u32>(o + 244);
    if (p == 0) {
        ctx.gpr[4] = 0;
        ctx.gpr[5] = o;
        ctx.gpr[6] = w;
        ret(ctx, a);
        return;
    }
    VCall(ctx, p, 208, {p, a});
}
// u32 f(T* o): when the u16 +482 is set, 0x002F20A4(*(T**)0x00F64C38, the
// word +480) and +482 = 0; 0
DECOMP(00129B58, Obj_Flush480_00129B58) {
    const u32 o = arg<u32>(ctx, 0);
    if (load<u16>(o + 482) == 0) {
        ret(ctx, 0u);
        return;
    }
    call(ctx, 0x002F20A4, {load<u32>(0x00F64C38), load<u32>(o + 480)});
    store<u16>(o + 482, 0);
    ret(ctx, 0u);
}
// f(const T* o, a): 0x004CF548(p, a) for p = o->+416 and o->+420, each when set
DECOMP(0017A178, Obj_Call4CF548x2_0017A178) {
    const u32 o = arg<u32>(ctx, 0), a = arg<u32>(ctx, 1);
    const u32 p = load<u32>(o + 416);
    if (p != 0) call(ctx, 0x004CF548, {p, a});
    else ret(ctx, 0u);
    const u32 q = load<u32>(o + 420);
    if (q != 0) call(ctx, 0x004CF548, {q, a});
}
// void f(T* o, const Vec3* t): +704 = (t - o->+608) normalised, times
// *(f32*)0x001CF618 (f1..f5 / r4 / r5 left)
DECOMP(001CF620, Obj_DirTo_001CF620) {
    const u32 o = arg<u32>(ctx, 0), t = arg<u32>(ctx, 1);
    const u32 p = o + 608;
    const f32 dy = load<f32>(t + 4) - load<f32>(p + 4);
    const f32 dx = load<f32>(t) - load<f32>(p);
    const f32 dz = load<f32>(t + 8) - load<f32>(p + 8);
    constexpr u32 kK = 0x001CF618;
    const f32 k = load<f32>(kK);
    f32 s = dy * dy;
    s = static_cast<f32>(std::fma(static_cast<f64>(dx), dx, s));
    s = static_cast<f32>(std::fma(static_cast<f64>(dz), dz, s));
    s = static_cast<f32>(std::sqrt(static_cast<f64>(s)));
    const f32 sc = k / s;
    const f32 x = dx * sc, y = dy * sc, z = dz * sc;
    store<f32>(o + 704, x);
    store<f32>(o + 708, y);
    store<f32>(o + 712, z);
    ctx.fpr[1] = y;
    ctx.fpr[2] = x;
    ctx.fpr[3] = z;
    ctx.fpr[4] = sc;
    ctx.fpr[5] = k;
    ctx.gpr[4] = kK;
    ctx.gpr[5] = p;
}
// T::~T(): when +736 is set: vtable 0x00B1B4C8 and Mem_FreeIfSet(+736,
// +732); then vtable 0x00B11D98 and the base 0x000C2DA0(this)
static void DtorFree736(PPUContext& ctx) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 p = load<u32>(o + 736);
    if (p != 0) {
        store<u32>(o, 0x00B1B4C8);
        call(ctx, 0x00555CC0, {p, load<u32>(o + 732)});
    }
    store<u32>(o, 0x00B11D98);
    call(ctx, 0x000C2DA0, {o, 0x00B11D98});
}
DECOMP(00226E80, VtB1B4C8_Dtor) { DtorFree736(ctx); }
DECOMP(00244BEC, VtB1B4C8_Dtor_00244BEC) { DtorFree736(ctx); }
// T* f(const T* o, u32 h): o->+396 when it is set and
// String_HashNoCase(o->+216 + 745) is h, else 0
DECOMP(00243E5C, Obj_GetIfNamed_00243E5C) {
    const u32 o = arg<u32>(ctx, 0), h = arg<u32>(ctx, 1);
    if (load<u32>(o + 396) != 0 && static_cast<u32>(call(ctx, o22fn::String_HashNoCase, {load<u32>(o + 216) + 745})) == h) {
        ret(ctx, load<u32>(o + 396));
        return;
    }
    ret(ctx, 0u);
}
// void f(Vec3* out, const T* o): out = o->+1168, or o->+416's +432 when
// that is set and has +8 (r4..r6 / f1..f3 left)
DECOMP(0024D804, Obj_GetPos_0024D804) {
    const u32 out = arg<u32>(ctx, 0), o = arg<u32>(ctx, 1);
    Cp(out, o + 1168);
    Cp(out + 4, o + 1172);
    ctx.fpr[1] = F32Bits(Cp(out + 8, o + 1176));
    const u32 p = load<u32>(o + 416);
    ctx.gpr[4] = p;
    if (p == 0) return;
    const u32 f = load<u32>(p + 8);
    ctx.gpr[5] = f;
    if (f == 0) return;
    const u32 s = p + 432;
    const u32 x = load<u32>(s), y = load<u32>(s + 4), z = load<u32>(s + 8);
    store<u32>(out, x);
    store<u32>(out + 4, y);
    store<u32>(out + 8, z);
    ctx.gpr[4] = s;
    ctx.gpr[5] = s + 4;
    ctx.gpr[6] = s + 8;
    ctx.fpr[1] = F32Bits(x);
    ctx.fpr[2] = F32Bits(y);
    ctx.fpr[3] = F32Bits(z);
}
// delete T: nothing for null; vtable 0x00B1C030, SortedMap_Remove75DE78(
// 0x01057544, this + 36), Mem_FreeFrom(this, 0)
DECOMP(0027418C, VtB1C030_Delete) {
    const u32 o = arg<u32>(ctx, 0);
    if (o == 0) return;
    store<u32>(o, 0x00B1C030);
    call(ctx, 0x0075DE78, {0x01057544, o + 36, 0x00B1C030, o + 36});
    call(ctx, o19fn::Mem_FreeFrom, {o, 0});
}
// f(const T* o): World_AddTimedCallback(world, 0, 0x00F352C8, o, o->+8)
// when 0x0105C930 and the u8 o->+16 are set (r4 left)
DECOMP(00284498, Obj_AddCallback_00284498) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 g = load<u32>(0x0105C8E8 + 72);
    ctx.gpr[4] = g;
    if (g == 0) return;
    const u32 b = load<u8>(o + 16);
    ctx.gpr[4] = b;
    if (b == 0) return;
    call(ctx, 0x0046B5F8, {load<u32>(kWorldPtr), 0, 0x00F352C8, o, load<u32>(o + 8)});
}
// void f(T* o, f32 dt): the o->+240'th 128-byte record of 0x011950AC: +84
// = +80 + dt; o->+596 / +600 = o->+408 / +412 * (dt / o->+32 when that is >
// *(f32*)0x0028A58C, else dt) (f1..f6 / r4..r6 left)
DECOMP(0028A590, Obj_Step_0028A590) {
    const u32 o = arg<u32>(ctx, 0);
    const f64 dt = farg(ctx, 0);
    const u32 e = load<u32>(0x011950A0 + 12) + ((load<u32>(o + 240) << 7) & 0xFFFFFF80u);
    const f64 t = static_cast<f32>(LfsAt(e + 80) + dt);
    store<f32>(e + 84, static_cast<f32>(t));
    constexpr u32 kMin = 0x0028A58C;
    const f64 c = LfsAt(kMin), len = LfsAt(o + 32), a = LfsAt(o + 408), b = LfsAt(o + 412);
    f64 s = dt;
    if (len > c) s = static_cast<f32>(dt / len);
    const f32 x = static_cast<f32>(a * s), y = static_cast<f32>(b * s);
    store<f32>(o + 596, x);
    store<f32>(o + 600, y);
    ctx.fpr[1] = x;
    ctx.fpr[2] = y;
    ctx.fpr[3] = a;
    ctx.fpr[4] = len;
    ctx.fpr[5] = s;
    ctx.fpr[6] = c;
    ctx.gpr[4] = e;
    ctx.gpr[5] = kMin;
    ctx.gpr[6] = 0x00290000;
}
// s32 f(const T* o): the s32 +140 of the first of the o->+200 160-byte
// entries at o + 208 whose +132 or +136 differs from *(f32*)0x00296894,
// else 0 (r4..r7 / f1 / f2 left)
DECOMP(00296898, Obj_FirstSet_00296898) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 n = load<u32>(o + 200);
    constexpr u32 kK = 0x00296894;
    const f64 k = LfsAt(kK);
    ctx.gpr[4] = n;
    ctx.gpr[5] = 0;
    ctx.fpr[1] = k;
    if (n != 0) {
        for (u32 i = 0; i < n; ++i) {
            const u32 e = o + 160 * i + 208;
            ctx.gpr[6] = i;
            ctx.gpr[7] = e;
            f64 v = LfsAt(e + 132);
            ctx.fpr[2] = v;
            if (v == k) {
                v = LfsAt(e + 136);
                ctx.fpr[2] = v;
            }
            if (v != k) {
                ret(ctx, load<s32>(e + 140));
                return;
            }
        }
        ctx.gpr[6] = n;
    } else {
        ctx.gpr[6] = 0;
        ctx.gpr[7] = kK;
    }
    ret(ctx, 0u);
}
// u32 f(?, const T* a): *Map_Find(map, &key) for key = a->+220->+60->+204
// (0 when a or a link is null)
static void MapLookup(PPUContext& ctx, u32 find, u32 map) {
    const u32 a = arg<u32>(ctx, 1);
    if (a == 0) {
        ret(ctx, 0u);
        return;
    }
    const u32 b = load<u32>(a + 220);
    if (b == 0) {
        ret(ctx, 0u);
        return;
    }
    const u32 key = load<u32>(load<u32>(b + 60) + 204);
    if (key == 0) {
        ret(ctx, 0u);
        return;
    }
    Stack frame(ctx, 128);
    const u32 t = frame.addr() + 112;
    store<u32>(t, key);
    const u32 r = static_cast<u32>(call(ctx, find, {map, t, key}));
    ret(ctx, load<u32>(r));
}
DECOMP(002E696C, Map_Lookup_002E696C) { MapLookup(ctx, 0x007662A4, 0x01068184); }
DECOMP(002E69D4, Map_Lookup_002E69D4) { MapLookup(ctx, 0x007665B4, 0x010681B4); }
// T::~T(): vtable 0x00B1D5A0, +12's virtual +8 (it), vtable 0x00B1D4D8
DECOMP(002F48A4, VtB1D5A0_Dtor) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 p = load<u32>(o + 12);
    store<u32>(o, 0x00B1D5A0);
    VCall(ctx, p, 8, {p});
    store<u32>(o, 0x00B1D4D8);
    ret(ctx, 0x00B1D4D8u);
}
// f(T* o): 0x002F978C(o, 0); then the effect +44 is destroyed and freed
// (140 bytes) and +44 = 0 when set
DECOMP(002FA37C, Obj_FreeEffect_002FA37C) {
    const u32 o = arg<u32>(ctx, 0);
    call(ctx, 0x002F978C, {o, 0});
    const u32 e = load<u32>(o + 44);
    if (e == 0) return;
    call(ctx, 0x004C8E80, {e});
    call(ctx, o19fn::SmallFree, {e, 140});
    store<u32>(o + 44, 0);
    ret(ctx, 0u);
}
// f(T* o, s32 m, u8 b): 0x003284C4(o, m, b), then 0x00334114(o) unless m
// is 4 and b is 0
DECOMP(00335258, Obj_SetMode_00335258) {
    const u32 o = arg<u32>(ctx, 0), m = arg<u32>(ctx, 1);
    const u32 b = arg<u32>(ctx, 2) & 0xFFu;
    call(ctx, 0x003284C4, {o, m, b});
    if (static_cast<s32>(m) == 4 && b == 0) return;
    call(ctx, 0x00334114, {o});
}
// f(T* o, s32 m, u8 b): when m is 4 and not (b == 0 and the u8 o->+18 ==
// m), +2572 = +2576 first; then 0x003260F0(o, m, b)
DECOMP(003411A0, Obj_SetMode_003411A0) {
    const u32 o = arg<u32>(ctx, 0), m = arg<u32>(ctx, 1);
    const u32 b = arg<u32>(ctx, 2) & 0xFFu;
    if (!(b == 0 && static_cast<s32>(load<u8>(o + 18)) == static_cast<s32>(m)) && static_cast<s32>(m) == 4) {
        ctx.fpr[1] = F32Bits(Cp(o + 2572, o + 2576));
    }
    call(ctx, 0x003260F0, {o, m, b, m});
}
// r f(T* o, a, b): o->+8's virtual `slot` (p, a, b) when set, else dflt
// (r5 / r6 / r7 left as the original does)
DECOMP(00347718, Obj_VCall20Member8_00347718) {
    const u32 o = arg<u32>(ctx, 0), b = arg<u32>(ctx, 2);
    const u32 p = load<u32>(o + 8);
    if (p == 0) {
        ctx.gpr[5] = 0;
        ctx.gpr[6] = 0;
        ctx.gpr[7] = o;
        ret(ctx, 0u);
        return;
    }
    const u32 r = static_cast<u32>(VCall(ctx, p, 20, {p, ctx.gpr[4], b}));
    ctx.gpr[6] = r;
}
static void Member8VCall2(PPUContext& ctx, u32 slot, u32 dflt) {
    const u32 o = arg<u32>(ctx, 0), a = arg<u32>(ctx, 1);
    const u32 p = load<u32>(o + 8);
    if (p == 0) {
        ctx.gpr[4] = 0;
        ctx.gpr[5] = dflt;
        ctx.gpr[6] = o;
        ret(ctx, dflt);
        return;
    }
    const u32 r = static_cast<u32>(VCall(ctx, p, slot, {p, a}));
    ctx.gpr[5] = r;
}
DECOMP(00347780, Obj_VCall24Member8_00347780) { Member8VCall2(ctx, 24, 0); }
DECOMP(00347858, Obj_VCall32Member8_00347858) { Member8VCall2(ctx, 32, 1); }
DECOMP(00347984, Obj_VCall44Member8_00347984) {
    const u32 o = arg<u32>(ctx, 0), b = arg<u32>(ctx, 2);
    const u32 p = load<u32>(o + 8);
    if (p == 0) {
        ctx.gpr[5] = 0;
        ctx.gpr[6] = 1;
        ctx.gpr[7] = o;
        ret(ctx, 1u);
        return;
    }
    const u32 r = static_cast<u32>(VCall(ctx, p, 44, {p, ctx.gpr[4], b}));
    ctx.gpr[6] = r;
}
// Pair* f(T* t, a, b): claims the next slot (the u16 count +2 of t->+72,
// below its u16 capacity +0): t->+76[i] = 0 when that array is set, count
// + 1, then t->+80[i] = {a, b}. Quirk: when full, i = -1 and the pair is
// written just before t->+80. (r6..r9 left)
DECOMP(0034F518, Pairs_Push_0034F518) {
    const u32 t = arg<u32>(ctx, 0), a = arg<u32>(ctx, 1), b = arg<u32>(ctx, 2);
    u32 h = load<u32>(t + 72);
    u32 i = load<u16>(h + 2);
    const u32 cap = load<u16>(h);
    ctx.gpr[8] = cap;
    if (static_cast<s32>(i) < static_cast<s32>(cap)) {
        const u32 arr = load<u32>(t + 76);
        ctx.gpr[8] = arr;
        if (arr != 0) {
            store<u32>(arr + ((i << 2) & 0x3FFFCu), 0);
            ctx.gpr[9] = 0;
            h = load<u32>(t + 72);
        }
        const u32 n = load<u16>(h + 2) + 1u;
        store<u16>(h + 2, static_cast<u16>(n));
        ctx.gpr[8] = n;
        ctx.gpr[7] = i;
    } else {
        i = ~0u;
        ctx.gpr[7] = ~0ull;
    }
    const u32 e = load<u32>(t + 80) + i * 8u;
    store<u32>(e, a);
    store<u32>(e + 4, b);
    ctx.gpr[6] = i == ~0u ? ~0ull - 7 : static_cast<u64>(i * 8u);
    ret(ctx, e);
}
// f(Obj* o, ?, u32 i): the virtual +40 (p, .) of p = o->+328's 16-byte
// entry i's +12 when i < o->+8->+152 and p is set (r5 / r6 left)
DECOMP(0035B404, Obj_EntryVCall40_0035B404) {
    const u32 o = arg<u32>(ctx, 0), i = arg<u32>(ctx, 2);
    const u32 n = load<u32>(load<u32>(o + 8) + 152);
    ctx.gpr[6] = n;
    if (i >= n) return;
    const u32 p = load<u32>(load<u32>(o + 328) + (i << 4) + 12);
    ctx.gpr[5] = i << 4;
    ret(ctx, p);
    if (p == 0) return;
    VCall(ctx, p, 40, {p, ctx.gpr[4]});
}
// f(s32 key): for the first object of the list at 0x0106C280 (u16 count
// at 0x0106C288) whose +168 is key: +220 = +216 (r4..r8 / f1 left)
DECOMP(00369BC8, List_Touch_00369BC8) {
    const u32 key = arg<u32>(ctx, 0);
    const u32 arr = load<u32>(0x0106C280);
    const u32 n = load<u16>(0x0106C280 + 8);
    ctx.gpr[4] = arr;
    ctx.gpr[5] = arr;
    ctx.gpr[6] = n;
    ctx.gpr[7] = 0;
    for (u32 i = 0; i < n; ++i) {
        const u32 v = load<u32>(load<u32>(arr + 4 * i) + 168);
        ctx.gpr[4] = arr + 4 * i;
        ctx.gpr[7] = i;
        ctx.gpr[8] = v;
        if (v == key) {
            const u32 e = load<u32>(arr + 4 * i);
            ctx.fpr[1] = F32Bits(Cp(e + 220, e + 216));
            ret(ctx, e);
            return;
        }
    }
    if (n != 0) {
        ctx.gpr[4] = arr + 4 * n;
        ctx.gpr[7] = n;
    }
}
// f(const T* o, u8 alt): Speaker_SayId4A5C90(o->+28, o->+184 (alt) or
// +188, 0; *(f32*)0x00381E40)
DECOMP(00381E48, Obj_Say_00381E48) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 id = load<u32>(o + ((arg<u32>(ctx, 1) & 0xFFu) != 0 ? 184u : 188u));
    callf(ctx, 0x004A5C90, {load<u32>(o + 28), id, 0, id}, {LfsAt(0x00381E40)});
}
// void f(): the three words at 0x0106CF88 = 0 and the nine u16s
// 0x0106CF70 / 0x0106CF76..0x0106CF84 = 16; returns 16 (r4..r11 left)
DECOMP(00398E64, Cfg_Init_00398E64) {
    constexpr u32 k = 0x0106CF88;
    store<u32>(k, 0);
    store<u32>(k + 4, 0);
    store<u32>(k + 8, 0);
    for (u32 a : {0x0106CF78u, 0x0106CF76u, 0x0106CF7Au, 0x0106CF7Cu, 0x0106CF80u, 0x0106CF7Eu, 0x0106CF82u, 0x0106CF84u, 0x0106CF70u}) store<u16>(a, 16);
    for (int r = 4; r <= 11; ++r) ctx.gpr[r] = 0x01070000;
    ctx.gpr[7] = k;
    ret(ctx, 16u);
}
// f(T* o): when o->+248 is set: its +184 gets bit 0x80000000 and its
// virtual +24 (p, 1, 0) runs (r4 left as the null otherwise)
DECOMP(003AAB54, Obj_Trigger248_003AAB54) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 p = load<u32>(o + 248);
    if (p == 0) {
        ctx.gpr[4] = 0;
        return;
    }
    store<u32>(p + 184, load<u32>(p + 184) | 0x80000000u);
    const u32 q = load<u32>(o + 248);
    VCall(ctx, q, 24, {q, 1, 0});
}
// u32 f(T* o): the import 0x00ABDCE4((s32)o->+4) when that is > 0, then
// +20 = +4 = 0 and the u8s +24..+28 = 0; 0 (r4 left)
DECOMP(004279A4, Handle_Close_004279A4) {
    const u32 o = arg<u32>(ctx, 0);
    const s32 h = load<s32>(o + 4);
    if (h > 0) call(ctx, 0x00ABDCE4, {static_cast<u64>(static_cast<s64>(h))});
    store<u32>(o + 20, 0);
    store<u32>(o + 4, 0);
    for (u32 k = 0; k < 5; ++k) store<u8>(o + 24 + k, 0);
    ctx.gpr[4] = o + 24;
    ret(ctx, 0u);
}
// void f(): Pool_DestructPart on 0x010AA1E8, then on the 16-byte array at
// 0x010AA1B8 from its last element (3 elements)
DECOMP(0042A314, Pools_Destruct_0042A314) {
    call(ctx, 0x0052EFE0, {0x010AA1E8});
    for (u32 p = 0x010AA1B8 + 32;; p -= 16) {
        call(ctx, 0x0052EFE0, {p});
        if (p == 0x010AA1B8) break;
    }
}
// T::T(T* o, u32 a, u32 b, u8 on): the 0x00B20CC8 base (o, on), +16 = a,
// +20 = b, +12 = 0, vtable 0x00B20D70 (r3 / r4 left)
DECOMP(0042D8BC, VtB20D70_Ctor) {
    const u32 o = arg<u32>(ctx, 0), a = arg<u32>(ctx, 1), b = arg<u32>(ctx, 2);
    call(ctx, o24fn::Obj_InitVt_B20CC8, {o, arg<u32>(ctx, 3) & 0xFFu});
    store<u32>(o + 16, a);
    store<u32>(o + 20, b);
    store<u32>(o + 12, 0);
    store<u32>(o, 0x00B20D70);
    ctx.gpr[4] = 0;
    ret(ctx, 0x00B20D70u);
}
// T::T(T* o, bool a, bool b): the 0x00B20CC8 base (o, 1), bits 0x00800000
// / 0x00400000 of +8 = a / b, vtable 0x00B20E00 (r3 / r4 left)
DECOMP(0042E3AC, VtB20E00_Ctor) {
    const u32 o = arg<u32>(ctx, 0), a = arg<u32>(ctx, 1), b = arg<u32>(ctx, 2);
    call(ctx, o24fn::Obj_InitVt_B20CC8, {o, 1});
    u32 w = load<u32>(o + 8);
    w = (w & ~0x00800000u) | ((a & 1u) << 23);
    w = (w & ~0x00400000u) | ((b & 1u) << 22);
    store<u32>(o, 0x00B20E00);
    store<u32>(o + 8, w);
    ctx.gpr[4] = 0x00B20E00;
    ret(ctx, w);
}
// delete T: nothing for null; vtable 0x00B217E8, the global 0x00F658D4 =
// 0, Thunk_004F41AC(this), Mem_FreeFrom(this, 0)
DECOMP(0043DA44, VtB217E8_Delete) {
    const u32 o = arg<u32>(ctx, 0);
    if (o == 0) return;
    store<u32>(o, 0x00B217E8);
    store<u32>(0x00F658D4, 0);
    call(ctx, 0x004F41AC, {o, 0x00F60000, 0x00B217E8, 0});
    call(ctx, o19fn::Mem_FreeFrom, {o, 0});
}
// u32 f(): 6 when the u8 0x010AEC60 is set and 0x006D4F80() is 3, 8 when it
// is 4, else 2
DECOMP(0044BCE0, Video_Mode_0044BCE0) {
    u32 r = 2;
    if (load<u8>(0x010AEC60) != 0) {
        const s32 v = static_cast<s32>(call(ctx, 0x006D4F80, {}));
        if (v == 3) r = 6;
        else if (v == 4) r = 8;
    }
    ret(ctx, r);
}
// void f(a, b, c, d, u8 e): appends the 20-byte record {a, b, c, d, e} to
// the queue at 0x010BA138 (count 0x010BA134, at most 128) unless e is set
// and b is 0 (r8..r12 left)
DECOMP(004705F8, Queue_Push_004705F8) {
    const u32 a = arg<u32>(ctx, 0), b = arg<u32>(ctx, 1), c = arg<u32>(ctx, 2), d = arg<u32>(ctx, 3);
    const u32 e = arg<u32>(ctx, 4) & 0xFFu;
    ctx.gpr[7] = e;
    if (e != 0 && b == 0) return;
    const s32 n = load<s32>(0x010BA134);
    ctx.gpr[8] = static_cast<u32>(n);
    if (n >= 128) return;
    constexpr u32 q = 0x010BA138;
    const u32 off = static_cast<u32>(n) * 20u;
    const u32 r = q + off;
    store<u32>(r, a);
    store<u32>(r + 4, b);
    store<u32>(r + 8, c);
    store<u32>(r + 12, d);
    store<u8>(r + 16, static_cast<u8>(e));
    store<u32>(0x010BA134, static_cast<u32>(n + 1));
    ctx.gpr[8] = static_cast<u32>(n + 1);
    ctx.gpr[9] = off;
    ctx.gpr[10] = q;
    ctx.gpr[11] = 0x010C0000;
    ctx.gpr[12] = r;
}
// delete T (pooled): nothing for null; 0x004AD154(this), then the pool
// behind r13 - 28332 takes it back: pool->vfn[3](pool, this, this->+4)
DECOMP(004AD298, Pooled_Delete_004AD298) {
    const u32 o = arg<u32>(ctx, 0);
    if (o == 0) return;
    call(ctx, 0x004AD154, {o});
    const u32 pool = Pool28332(ctx);
    VCall(ctx, pool, 12, {pool, o, load<u16>(o + 4)});
}
// T* New(a): SmallAlloc(size) and ctor(p, a); 0 on failure
static void NewA(PPUContext& ctx, u32 size, u32 ctor) {
    const u32 a = arg<u32>(ctx, 1);
    const u32 p = static_cast<u32>(call(ctx, o19fn::SmallAlloc, {size}));
    if (p != 0) call(ctx, ctor, {p, a});
    ret(ctx, p);
}
DECOMP(004C0008, New44_004C0008) { NewA(ctx, 44, 0x004BFA84); }
DECOMP(004C09D8, New68_004C09D8) { NewA(ctx, 68, 0x004C0080); }
// f(const T* o): bit 0 of the u16 +68 of World_FindRec(o->+12) = (the u8
// o->+16 == 1) when found (r4 / r5 left)
DECOMP(004C4684, Rec_SetFlag_004C4684) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 r = static_cast<u32>(call(ctx, 0x0059B724, {load<u32>(o + 12)}));
    if (r == 0) return;
    if (load<u8>(o + 16) == 1) {
        const u32 h = load<u16>(r + 68) | 1u;
        store<u16>(r + 68, static_cast<u16>(h));
        ctx.gpr[4] = h;
    } else {
        const u32 h = load<u16>(r + 68) & ~1u;
        store<u16>(r + 68, static_cast<u16>(h));
        ctx.gpr[4] = 0;
        ctx.gpr[5] = h;
    }
}
// f(T* o, bool on): bit 0x80000000 of +136 = on; then o->+168's virtual +40
// (p, on, 1) when set (r4..r7 left otherwise)
DECOMP(004E93AC, Obj_SetOn_004E93AC) {
    const u32 o = arg<u32>(ctx, 0), b = arg<u32>(ctx, 1);
    const u32 w = (load<u32>(o + 136) & 0x7FFFFFFFu) | (b << 31);
    store<u32>(o + 136, w);
    const u32 p = load<u32>(o + 168);
    if (p == 0) {
        ctx.gpr[4] = b & 0xFFu;
        ctx.gpr[5] = o;
        ctx.gpr[6] = b;
        ctx.gpr[7] = w;
        ret(ctx, 0u);
        return;
    }
    VCall(ctx, p, 40, {p, b & 0xFFu, 1});
}
// bool f(c): false when *(T**)0x010C2C28 is null; else
// XmlCursor_ReadF32ByName(c, 0x00AF0478, that + 864; *(f32*)0x004EF848),
// its u8 +860 = 1, true
DECOMP(004EF850, Global_ReadF32_004EF850) {
    const u32 g = load<u32>(0x010C2C28);
    if (g == 0) {
        ret(ctx, 0u);
        return;
    }
    callf(ctx, 0x004FA5C4, {ctx.gpr[3], 0x00AF0478, g + 864, 0x004EF848, g + 864}, {LfsAt(0x004EF848)});
    store<u8>(g + 860, 1);
    ret(ctx, 1u);
}
// f32 f(Rng* g, f32 lo, f32 hi): lo + (hi - lo) * Rng_Float (the last
// output scaled by *(f32*)0x007342D0), stepping the state (r4..r6 / f2..f4
// left)
DECOMP(007342D8, Rng_Lerp_007342D8) {
    const u32 g = arg<u32>(ctx, 0);
    const f64 lo = farg(ctx, 0);
    const f64 span = static_cast<f32>(farg(ctx, 1) - lo);
    const u64 prev = load<u64>(g);
    const u32 st = load<u32>(g + 8);
    const u32 s = (std::rotl(st, 4) + std::rotl(st, 16)) ^ 0x87654321u;
    const f64 u = static_cast<f32>(static_cast<f64>(static_cast<s64>(prev)));
    store<u32>(g + 8, std::rotl(s, 16));
    store<u32>(g, 0);
    store<u32>(g + 4, s & 0xFFFFu);
    const f64 r = static_cast<f32>(u * LfsAt(0x007342D0));
    ctx.gpr[4] = s & 0xFFFFu;
    ctx.gpr[5] = s & 0xFFFFu;
    ctx.gpr[6] = 0x87654321u;
    ctx.fpr[2] = span;
    ctx.fpr[3] = r;
    ctx.fpr[4] = u;
    ctx.fpr[1] = static_cast<f32>(std::fma(span, r, lo));
}
// u32 f(T* o, bool on): bit 0x8 of +172 = on; 0x000CFEC4(o, 0) when on;
// then bit 0x8000 of +376 = on (returns that word)
DECOMP(007377D8, Obj_SetOn_007377D8) {
    const u32 o = arg<u32>(ctx, 0), b = arg<u32>(ctx, 1);
    store<u32>(o + 172, (load<u32>(o + 172) & ~0x8u) | ((b & 1u) << 3));
    if ((b & 0xFFu) != 0) call(ctx, 0x000CFEC4, {o, 0});
    const u32 w = (load<u32>(o + 376) & ~0x8000u) | ((b & 1u) << 15);
    store<u32>(o + 376, w);
    ret(ctx, w);
}
// delete T: nothing for null; +380 = 0x00B1200C, +0 = 0x00B126B8, the
// base destructor 0x000DF934(this), SmallFree(this, 448)
DECOMP(00744394, VtB126B8_Delete_00744394) {
    const u32 o = arg<u32>(ctx, 0);
    if (o == 0) return;
    store<u32>(o + 380, 0x00B1200C);
    store<u32>(o, 0x00B126B8);
    call(ctx, o23fn::VtB12550_Dtor, {o, 0x00B126B8, 0x00B1200C});
    call(ctx, o19fn::SmallFree, {o, 448});
}
// u32 f(Array* a, s32 i): removes the 64-byte element i of a->+0 (count
// a->+4) by moving the rest down 8 bytes at a time; returns the bytes
// moved (rounded up to 8; r4..r9 left)
DECOMP(007603D0, Array64_RemoveAt_007603D0) {
    const u32 a = arg<u32>(ctx, 0);
    const u32 i = arg<u32>(ctx, 1);
    const u32 n = load<u32>(a + 4) - 1;
    const u32 dst = load<u32>(a) + (i << 6), src = dst + 64;
    store<u32>(a + 4, n);
    const s32 bytes = static_cast<s32>((n - i) << 6);
    ctx.gpr[4] = static_cast<u64>(static_cast<s64>(bytes));
    ctx.gpr[5] = src;
    ctx.gpr[6] = dst;
    ctx.gpr[7] = n;
    s32 off = 0;
    if (bytes > 0) {
        do {
            const u64 v = load<u64>(src + static_cast<u32>(off));
            store<u64>(dst + static_cast<u32>(off), v);
            ctx.gpr[8] = static_cast<u32>(off);
            ctx.gpr[9] = v;
            off += 8;
        } while (off < bytes);
        ctx.gpr[7] = static_cast<u32>(off);
    }
    ctx.gpr[3] = static_cast<u32>(off);
}
// bool f(?, u32 key, f32* out, const Node* n): the binary tree under n (+4
// key (unsigned order), +8 value, +12 left, +16 right): *out = the value
// for key (r4..r6 / f1 left)
DECOMP(0077C6F8, Tree_FindU32_0077C6F8) {
    const u32 key = arg<u32>(ctx, 1), out = arg<u32>(ctx, 2);
    u32 n = arg<u32>(ctx, 3);
    ctx.gpr[4] = out;
    ctx.gpr[5] = key;
    while (n != 0) {
        const u32 v = load<u32>(n + 4);
        ctx.gpr[6] = v;
        if (v == key) {
            ctx.gpr[5] = n;
            ctx.fpr[1] = F32Bits(Cp(out, n + 8));
            ret(ctx, 1u);
            return;
        }
        n = key < v ? load<u32>(n + 12) : load<u32>(n + 16);
    }
    ret(ctx, 0u);
}
// void f(const T* o, u32* out): out[k + j] = the u8 o->+28 + j (j = 0..3)
// for k = o->+16->+64 (re-read for each) (r3 / r5..r7 left)
DECOMP(00782108, Obj_StoreBytes28_00782108) {
    const u32 o = arg<u32>(ctx, 0), out = arg<u32>(ctx, 1);
    u32 off = 0, b = 0;
    for (u32 j = 0; j < 4; ++j) {
        b = load<u8>(o + 28 + j);
        off = (load<u32>(load<u32>(o + 16) + 64) + j) << 2;
        store<u32>(out + off, b);
        if (j == 2) {
            ctx.gpr[6] = off;
            ctx.gpr[7] = b;
        }
    }
    ctx.gpr[3] = off;
    ctx.gpr[5] = b;
}
// void f(T* o): +500 = (o->+780 - 1) * o->+736->+1232 when +780 > 1 and that
// value (k = *(f32*)0x001CAA80 when +736 is null) is > k (r4 / r5 / f1 /
// f2 left)
DECOMP(001CAA88, Obj_Scale500_001CAA88) {
    const u32 o = arg<u32>(ctx, 0);
    const f64 k = LfsAt(0x001CAA80);
    const u32 p = load<u32>(o + 736);
    const s32 n = load<s32>(o + 780);
    ctx.gpr[4] = static_cast<u32>(n);
    ctx.gpr[5] = p;
    ctx.fpr[1] = k;
    const f64 v = p != 0 ? LfsAt(p + 1232) : k;
    ctx.fpr[2] = v;
    if (!(n > 1) || !(v > k)) return;
    const s32 m = n - 1;
    ctx.gpr[4] = static_cast<u64>(static_cast<s64>(m));
    const f32 r = static_cast<f32>(static_cast<f32>(static_cast<f64>(m)) * v);
    store<f32>(o + 500, r);
    ctx.fpr[1] = r;
}
