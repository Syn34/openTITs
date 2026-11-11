// Game code, medium sweep part 24: laser fields, a camera-def blend and a
// visibility toggle.
#include "mid24.h"

using namespace sly4;
using namespace sly4::mid24;

namespace {
constexpr u32 kStringFormat = 0x005534D0u;  // String_Format(fmt, ...)
constexpr u32 kPointHolderSlot = 0x00F8F154u;  // u32*: holder whose +16 is the PointMap
constexpr u32 kCamDefHolderSlot = 0x00F8F134u;  // u32*: holder whose +16 is the CameraDefMap
}  // namespace

// u32 LaserField_Add3A20F8(Sys* s, Obj* o, Src* p): nothing for a null o
// (r3 = s). A non-zero o->204 already owned by one of the u16 (s->36)->2
// 368-byte records s->44 (through rec->320->204): String_Format("Duplicate
// laser field found! (%s)! ...", o->vfn[12](o)). With the slot pool s->36
// full: String_Format("Ran out of laser field slots (Max %i), ...", 16,
// o->vfn[12](o)). Else a new slot i (s->40[i] = 0 when s->40 is set): rec =
// s->44 + 368 i, 0x0039CB9C(rec) when non-null, rec->324 = 0, rec->320 =
// o, rec->324 = o->272 when that is set with (+4 >> 10) & 0x3F == 13,
// rec->312 = p when p->16->vfn[12](p->16) is *0x00AE2CFC, and the owner
// rec->320 gets +280 = rec, +276 = 0xAB27550F. r3 = rec->320.
DECOMP(003A20F8, LaserField_Add3A20F8) {
    const u32 s = arg<u32>(ctx, 0), o = arg<u32>(ctx, 1), p = arg<u32>(ctx, 2);
    if (o == 0) return;
    if (const u32 key = load<u32>(o + 204); key != 0) {
        const u32 used = load<u16>(load<u32>(s + 36) + 2);
        u32 rec = load<u32>(s + 44);
        for (u32 i = 0; i < used; ++i, rec += 368) {
            if (rec == 0) continue;
            const u32 w = load<u32>(rec + 320);
            if ((w != 0 ? load<u32>(w + 204) : 0u) == key) {
                const u32 nm = static_cast<u32>(VCall(ctx, o, 12, {o}));
                call(ctx, kStringFormat, {0x00ADE134u, nm});
                return;
            }
        }
    }
    u32 pool = load<u32>(s + 36);
    const u32 n = load<u16>(pool + 2);
    if (!(static_cast<s32>(n) < static_cast<s32>(load<u16>(pool)))) {
        const u32 nm = static_cast<u32>(VCall(ctx, o, 12, {o}));
        call(ctx, kStringFormat, {0x00ADE184u, 16, nm});
        return;
    }
    // (The original's not-free fallback, index -1, is unreachable here.)
    if (const u32 a = load<u32>(s + 40); a != 0) {
        store<u32>(a + (n << 2), 0);
        pool = load<u32>(s + 36);
    }
    store<u16>(pool + 2, static_cast<u16>(load<u16>(pool + 2) + 1));
    const u32 rec = load<u32>(s + 44) + 368u * static_cast<u32>(static_cast<s32>(static_cast<s16>(n)));
    if (rec != 0) call(ctx, 0x0039CB9Cu, {rec});
    store<u32>(rec + 324, 0);
    store<u32>(rec + 320, o);
    if (const u32 x = load<u32>(o + 272); x != 0 && ((load<u32>(x + 4) >> 10) & 0x3Fu) == 13) store<u32>(rec + 324, x);
    const u32 want = load<u32>(0x00AE2CFCu);
    const u32 y = load<u32>(p + 16);
    const u32 got = static_cast<u32>(VCall(ctx, y, 12, {y}));
    const u32 own = load<u32>(rec + 320);
    if (want == got) store<u32>(rec + 312, p);
    if (own != 0) {
        store<u32>(own + 280, rec);
        store<u32>(own + 276, 0xAB27550Fu);
    }
    ctx.gpr[3] = own;
}

// u32 CamDef_Blend417EB8(Cam* c): with an output c->60 and a point x =
// PointMap_Find(holder->16, *c->88) entry: the def y = CameraDefMap_Find(
// holder2->16, c->88->20) entry, t = c->88->24 (y->176 when t is K[0]; K =
// 0x00417EA8), u = min(c->236, K[1]) (as fcmpu: kept when not above):
// out->80 = u * (t * c->240) + (t * (K[1] - u)) * c->244; out's rows +16 /
// +32 / +48 rewritten in place (the inlined matrix copy) and out->64..72 =
// ((P - x->64..) + c->92->144..) - out->64.. ) * K[2] + out->64.., with P =
// y->160..168 (0x01164C00.. without y). r3 = out (0 when skipped); f1..f3
// the last three results.
DECOMP(00417EB8, CamDef_Blend417EB8) {
    const u32 c = arg<u32>(ctx, 0);
    ctx.gpr[3] = load<u32>(c + 60);
    if (ctx.gpr[3] == 0) return;
    Stack frame(ctx, 16);
    const u32 k2p = frame.addr(), k1p = k2p + 4;
    store<u32>(k1p, load<u32>(load<u32>(c + 88)));
    const u32 ex = static_cast<u32>(call(ctx, 0x0073343Cu, {load<u32>(load<u32>(kPointHolderSlot) + 16), k1p}));
    const u32 x = load<u32>(ex);
    if (x == 0) return;  // r3 = the map entry pointer
    store<u32>(k2p, load<u32>(load<u32>(c + 88) + 20));
    const u32 y = load<u32>(static_cast<u32>(call(ctx, 0x0073374Cu, {load<u32>(load<u32>(kCamDefHolderSlot) + 16), k2p})));
    constexpr u32 K = 0x00417EA8u;
    const u32 out = load<u32>(c + 60), m = load<u32>(c + 92);
    const u32 P = y != 0 ? y + 160 : 0x01164C00u;
    const f64 p0 = Lf(P), p1 = Lf(P + 4), p2 = Lf(P + 8);
    f64 t = Lf(load<u32>(c + 88) + 24);
    if (t == Lf(K)) t = Lf(y + 176);
    f64 u = Lf(c + 236);
    const f64 tb = S(t * Lf(c + 240));
    const f64 k1 = Lf(K + 4);
    if (u > k1) u = k1;
    const f64 d = S(t * S(k1 - u));
    Sf(out + 80, Fmadds(u, tb, S(d * Lf(c + 244))));
    f64 a0 = S(S(p0 - Lf(x + 64)) + Lf(m + 144));
    f64 a1 = S(S(p1 - Lf(x + 68)) + Lf(m + 148));
    f64 a2 = S(S(p2 - Lf(x + 72)) + Lf(m + 152));
    const f64 o0 = Lf(out + 64), o1 = Lf(out + 68), o2 = Lf(out + 72);
    const f64 k2 = Lf(K + 8);
    u32 rows[9];
    for (u32 k = 0; k < 9; ++k) rows[k] = load<u32>(out + 16 + 16 * (k / 3) + 4 * (k % 3));
    a0 = S(a0 - o0);
    a1 = S(a1 - o1);
    a2 = S(a2 - o2);
    for (u32 k = 0; k < 9; ++k) store<u32>(out + 16 + 16 * (k / 3) + 4 * (k % 3), rows[k]);
    ctx.fpr[1] = Fmadds(a0, k2, o0);
    ctx.fpr[2] = Fmadds(a1, k2, o1);
    ctx.fpr[3] = Fmadds(a2, k2, o2);
    Sf(out + 64, ctx.fpr[1]);
    Sf(out + 68, ctx.fpr[2]);
    Sf(out + 72, ctx.fpr[3]);
    ctx.gpr[3] = out;
}

// u32 Obj_UpdateFadeVis197210(Obj* o): Nop_000D8FC0(o); v = u8 o->689,
// cleared when Math_Cosf((o->692 * K[1] * K[2]) / K[3]) < K[0] (K =
// 0x00197200). With f32 o->348 above K[0]: nothing more unless v and o->172
// bit 0x100; then Obj_ApplyTree_000D8F08(o, o + 696, 1), o->vfn[88](o, 1,
// 1, 1, 1) when o->348 is (now) at most K[0], u8 o->688 = o->172 bit 0x100,
// o->vfn[88](o, 0, 1, 1, 0). Otherwise o->vfn[88](o, 0, 1, 1, 0), then
// o->vfn[88](o, 1, 1, 1, 1) when o->348 is at most K[0], and u8 o->688 =
// o->172 bit 0x100. r3 / f1 as the original leaves them.
DECOMP(00197210, Obj_UpdateFadeVis197210) {
    const u32 o = arg<u32>(ctx, 0);
    constexpr u32 K = 0x00197200u;
    call(ctx, 0x000D8FC0u, {o});
    u32 v = load<u8>(o + 689);
    if (v != 0) {
        const f64 a = Divs(S(S(Lf(o + 692) * Lf(K + 4)) * Lf(K + 8)), Lf(K + 12));
        const f64 cs = callf(ctx, 0x007344E0u, {}, {a});
        if (cs < Lf(K)) v = 0;
    }
    const f64 k0 = Lf(K);
    auto bit = [&]() -> u32 { return (load<u32>(o + 172) & 0x100u) != 0 ? 1u : 0u; };
    ctx.fpr[1] = Lf(o + 348);
    if (ctx.fpr[1] > k0) {
        ctx.gpr[3] = K;
        if (v == 0) return;
        if (bit() == 0) {
            ctx.gpr[3] = 0;
            return;
        }
        call(ctx, 0x000D8F08u, {o, o + 696, 1});
        ctx.fpr[1] = Lf(o + 348);
        if (ctx.fpr[1] <= k0) VCall(ctx, o, 88, {o, 1, 1, 1, 1});
        store<u8>(o + 688, static_cast<u8>(bit()));
        VCall(ctx, o, 88, {o, 0, 1, 1, 0});
        return;
    }
    VCall(ctx, o, 88, {o, 0, 1, 1, 0});
    ctx.fpr[1] = Lf(o + 348);
    if (ctx.fpr[1] <= k0) VCall(ctx, o, 88, {o, 1, 1, 1, 1});
    const u32 b = bit();
    store<u8>(o + 688, static_cast<u8>(b));
    ctx.gpr[3] = b;
}
