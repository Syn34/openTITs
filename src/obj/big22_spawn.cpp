// Game code, large-function sweep part 22: the 448-byte spawned physics
// object built by SpawnerC0FE8 / Obj_SlotApply_000B6208 / f_000B3A60.
#include "big22.h"

using namespace sly4;
using namespace sly4::big22;

namespace {
constexpr u32 kSpawnK = 0x000B2DD0u;   // f32 1.0, 1e-5; masks +16 / +32 / +48; f32 1/65535 +64, -1.0 +68
constexpr u32 kSpawnVt = 0x00B12080u;  // the object's vtable
constexpr u32 kListenerVt = 0x00B12204u;  // the embedded listener's (+380) vtable
constexpr u32 kUnknownName = 0x00B0DA98u;  // "<unknown>"
constexpr u32 kUpVec = 0x01164B60u;    // f32[3] (Constants_Init_0056B93C)
}  // namespace

// void Spawned_Construct_000B2E18(Obj* p, QueryEx* q): the base object
// (Thunk_000E02D8), vtable 0x00B12080, body +432 = 0, +436 / +440 = q->240 /
// q->252, listener +380 (vtable 0x00B12204). The box +384..+424 (16-byte
// rows zeroed first) gets the corners q+208 / q+224; the anchor +400 = min +
// dir * (|max - min| * q->248) + kUpVec * q->244, dir the normalised box
// diagonal (left unnormalised, with the length taken as 1.0, when its square
// is below 1e-5 or NaN); +144..152 = the min corner. With a body factory
// p->216->240, its vfn[8] makes the body (+432), named by p->160's vfn[3] (or
// "<unknown>"), posed from the rows +96 / +112 / +128 / +144 (w = 0), given
// the properties 0xA734004D = 0x7A30D7E3 and 0x91C7CF5B = p and the listener.
// Then state (0, 0), and p->20 gets a random vector: per axis lerp(p->216's
// +208, +224, r) with r a draw of the game's generator, times -1 or (odd
// seed bit) 1. Returns r3 = the last sign bit, f1 = the x value.
DECOMP(000B2E18, Spawned_Construct_000B2E18) {
    const u32 p = arg<u32>(ctx, 0);
    const u32 q = arg<u32>(ctx, 1);
    constexpr u32 K = kSpawnK;
    Stack frame(ctx, 64);
    const u32 sp = frame.addr();
    call(ctx, b22fn::Thunk_000E02D8, {p, q});
    store<u32>(p + 432, 0);
    store<u32>(p + 0, kSpawnVt);
    Cw(p + 436, q + 240);
    store<u32>(p + 380, kListenerVt);
    Cw(p + 440, q + 252);
    const u32 src = load<u32>(p + 216);
    const u32 zb = (p + 384) & ~15u;  // stvx x3: aligned 16-byte rows
    for (u32 k = 0; k < 48; k += 4) store<u32>(zb + k, 0);
    const f64 a0 = Lf(q + 208);
    Cw(p + 384, q + 208);
    const f64 a1 = Lf(q + 212);
    Cw(p + 388, q + 212);
    const f64 a2 = Lf(q + 216);
    Cw(p + 392, q + 216);
    const f64 b0 = Lf(q + 224);
    Cw(p + 416, q + 224);
    const f64 b1 = Lf(q + 228);
    Cw(p + 420, q + 228);
    const f64 b2 = Lf(q + 232);
    Cw(p + 424, q + 232);
    f64 d0 = S(b0 - a0), d1 = S(b1 - a1), d2 = S(b2 - a2);
    const f64 one = Lf(K + 0);
    const f64 lenSel = Lf(q + 248), upSel = Lf(q + 244);
    const u32 factory = load<u32>(src + 240);
    const f64 sq = Fmadds(d2, d2, Fmadds(d0, d0, S(d1 * d1)));
    f64 len = one;
    if (sq > Lf(K + 4) || sq == Lf(K + 4)) {
        len = Sqrts(sq);
        const f64 inv = Divs(one, len);
        d0 = S(inv * d0);
        d1 = S(inv * d1);
        d2 = S(inv * d2);
    }
    const f64 sc = S(len * lenSel);
    f64 x = Fmadds(sc, d0, a0), y = Fmadds(sc, d1, a1), z = Fmadds(sc, d2, a2);
    Sf(p + 400, x);
    Sf(p + 404, y);
    Sf(p + 408, z);
    x = Fmadds(Lf(kUpVec), upSel, x);
    y = Fmadds(Lf(kUpVec + 4), upSel, y);
    z = Fmadds(Lf(kUpVec + 8), upSel, z);
    Sf(p + 400, x);
    Sf(p + 404, y);
    Sf(p + 408, z);
    Cw(p + 144, q + 208);  // stfs of the loaded q->208
    Cw(p + 148, p + 388);
    Cw(p + 152, p + 392);
    if (factory != 0) {
        const u32 body = static_cast<u32>(VCall(ctx, factory, 32, {factory}));
        store<u32>(p + 432, body);
        const u32 nm = load<u32>(p + 160);
        const u32 name = nm != 0 ? static_cast<u32>(VCall(ctx, nm, 12, {nm})) : kUnknownName;
        call(ctx, b22fn::HkBody_SetName, {body + 120, name});
        const u32 b = load<u32>(p + 432);
        Row3(sp + 0, p + 96);
        Row3(sp + 16, p + 112);
        Row3(sp + 32, p + 128);
        Row3(sp + 48, p + 144);
        call(ctx, b22fn::Hk_SetTransform, {b, sp});
        call(ctx, b22fn::Hk_AddProperty, {load<u32>(p + 432), 0xA734004Du, 0x7A30D7E3u});
        call(ctx, b22fn::Hk_AddProperty, {load<u32>(p + 432), 0x91C7CF5Bu, p});
        call(ctx, b22fn::HkBody_AddListener, {load<u32>(p + 432), p + 380});
    }
    call(ctx, b22fn::StateMachine_SetState, {p, 0, 0});
    Rng rng;
    rng.Load();
    const f64 rk = Lf(K + 64), neg = Lf(K + 68);
    const u32 out = load<u32>(p + 20);
    f64 v[3];
    u32 bit = 0;
    for (u32 k = 0; k < 3; ++k) {
        const f64 lo = Lf(src + 208 + 4 * k), hi = Lf(src + 224 + 4 * k);
        const f64 r = S(rng.Next() * rk);
        const f64 t = Fmadds(S(hi - lo), r, lo);
        bit = rng.seed & 1;
        rng.Step();
        v[k] = S(t * (bit != 0 ? one : neg));
    }
    rng.Commit();
    Sf(out + 0, v[0]);
    Sf(out + 4, v[1]);
    Sf(out + 8, v[2]);
    ctx.fpr[1] = v[0];
    ret(ctx, bit);  // r3 is left holding the third sign bit
}
