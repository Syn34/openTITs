// Game code, VMX sweep part 4 (see vmx4.h): initialisers / constructors that
// clear a 16-byte field with stvx.
#include "../sly4.h"
#include "vmx4.h"

using namespace sly4;
using namespace sly4::vmx4;

namespace {
// Tuning table words (76-byte entries, value at +72; vec3s at +80 of a base).
constexpr u32 kTune119FF0 = 0x01038D1C;
}  // namespace

// void f(T* o, a1, a2, a3, a4, a5, a6 (unused), a7, f32 f1, -, -, f32 f4,
// stack args ...) (called by 0011A42C / 0011BD90 / 0011E2B8): fills o from
// the arguments and the tuning table: +0 = 2 then |= 8, the flag halfword
// +120 = (old & ~0x7DF0) | 0x8200, +88..+104 = (f32)(((+120 >> 4) & 15) - k)
// for k = 0..4 (re-reading +120 each time), -1.0f at +36 / +40 / +72,
// FLT_MAX at +224..+232, the 16 bytes at +160 cleared (stvx) then +160 = 15,
// +200..+215 cleared. With a name length (stack word +196) the name is
// copied into a fresh buffer: +196 = String_Alloc(n + 1), String_CopyN(buf,
// name (stack +204), (s32)+192). r3 is left as the original leaves it.
DECOMP(00119FF0, TunedObj_Init_00119FF0) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 a1 = arg<u32>(ctx, 1), a2 = arg<u32>(ctx, 2), a3 = arg<u32>(ctx, 3), a4 = arg<u32>(ctx, 4), a5 = arg<u32>(ctx, 5),
              a7 = arg<u32>(ctx, 7);
    const f64 f1 = farg(ctx, 0), f4 = farg(ctx, 3);
    const u32 s116 = StkW(ctx, 116), s124 = StkW(ctx, 124), s132 = StkW(ctx, 132), s140 = StkW(ctx, 140), s148 = StkW(ctx, 148),
              s156 = StkW(ctx, 156), s164 = StkW(ctx, 164), s172 = StkW(ctx, 172), s188 = StkW(ctx, 188), nlen = StkW(ctx, 196),
              name = StkW(ctx, 204);
    const u8 b0 = StkB(ctx, 176), b1 = StkB(ctx, 177), b2 = StkB(ctx, 178), b3 = StkB(ctx, 179);
    const u32 s276 = StkW(ctx, 276), s284 = StkW(ctx, 284), s292 = StkW(ctx, 292), s300 = StkW(ctx, 300), s308 = StkW(ctx, 308);
    const u32 flags = (static_cast<u32>(load<u16>(o + 120)) & ~0x7DF0u) | 0x8200u;
    constexpr u32 kNeg1 = 0xBF800000u;
    store<u32>(o + 12, a1);
    store<u32>(o + 16, a2);
    store<u32>(o + 20, a3);
    for (u32 k = 24; k < 32; ++k) store<u8>(o + k, 0);
    store<u32>(o + 32, 0);
    store<u32>(o + 36, kNeg1);
    store<u32>(o + 40, kNeg1);
    store<u8>(o + 44, 0);
    for (u32 k = 48; k <= 68; k += 4) store<u32>(o + k, 0);
    store<u32>(o + 72, kNeg1);
    store<u32>(o + 4, 0);
    store<u32>(o + 8, 0);
    store<f32>(o + 76, static_cast<f32>(f1));
    store<u32>(o + 80, 0);
    store<u32>(o + 84, 0);
    store<u32>(o + 108, 0);
    store<u32>(o + 112, 0);
    store<u32>(o + 116, 0);
    store<u16>(o + 120, static_cast<u16>(flags));
    store<u16>(o + 122, 0);
    store<u32>(o + 0, 2);
    u32 h = flags;  // the first pass uses the register, later ones re-read +120
    for (u32 k = 0; k < 5; ++k) {
        const s32 v = static_cast<s32>(((h >> 4) & 15u) - k);
        store<f32>(o + 88 + 4 * k, static_cast<f32>(static_cast<f64>(v)));
        h = load<u16>(o + 120);
    }
    store<u32>(o + 124, a4);
    store<u32>(o + 128, a5);
    store<u32>(o + 132, a7);
    store<u32>(o + 136, s116);
    store<u32>(o + 140, s124);
    store<u32>(o + 144, s132);
    store<u32>(o + 148, s140);
    store<u32>(o + 152, s148);
    store<u32>(o + 156, s156);
    store<u32>(o + 176, s164);
    store<u32>(o + 180, s172);
    store<u8>(o + 184, b0);
    store<u8>(o + 185, b1);
    store<u8>(o + 186, b2);
    store<u8>(o + 187, b3);
    store<u32>(o + 192, nlen);
    store<u32>(o + 240, 0);
    store<u32>(o + 244, 0);
    store<u8>(o + 188, static_cast<u8>(s188));
    const u32 t0 = load<u32>(kTune119FF0 + 72);
    store<u32>(o + 196, 0);
    store<u8>(o + 216, 0);
    store<u32>(o + 248, t0);
    store<u32>(o + 224, 0x7F7FFFFFu);
    store<u32>(o + 228, 0x7F7FFFFFu);
    store<u32>(o + 232, 0x7F7FFFFFu);
    store<u32>(o + 252, load<u32>(kTune119FF0 + 76 + 72));
    store<u32>(o + 256, load<u32>(kTune119FF0 + 152 + 72));
    // Three vec3s (0x01038E50 / 0x01038EB0 / 0x01038F60 / 0x01038FC0 ...)
    // copied with lfs / stfs (raw words).
    CopyW(o + 272, 0x01038E50);
    CopyW(o + 276, 0x01038E54);
    CopyW(o + 280, 0x01038E58);
    CopyW(o + 288, 0x01038EB0);
    CopyW(o + 292, 0x01038EB4);
    CopyW(o + 296, 0x01038EB8);
    store<u32>(o + 304, load<u32>(0x01038F08));
    CopyW(o + 320, 0x01038F60);
    CopyW(o + 324, 0x01038F64);
    CopyW(o + 328, 0x01038F68);
    CopyW(o + 336, 0x01038FC0);
    CopyW(o + 340, 0x01038FC4);
    store<f32>(o + 352, static_cast<f32>(f4));
    CopyW(o + 344, 0x01038FC8);
    store<u32>(o + 356, s276);
    store<u32>(o + 360, s284);
    store<u32>(o + 364, s292);
    store<u32>(o + 368, s300);
    store<u32>(o + 372, s308);
    const u32 w0 = load<u32>(o) | 8u;
    store<u32>(o, w0);
    Stvx(o + 160, Zero4());
    store<u32>(o + 160, 15);
    store<u64>(o + 200, 0);
    store<u64>(o + 208, 0);
    SetF1Bits(ctx, load<u32>(0x01038FC8));
    ctx.gpr[3] = w0;
    if (nlen == 0) return;
    const u32 buf = static_cast<u32>(call(ctx, v4fn::String_Alloc, {nlen + 1}));
    store<u32>(o + 196, buf);
    const s32 n = static_cast<s32>(load<u32>(o + 192));
    call(ctx, v4fn::String_CopyN, {buf, name, static_cast<u64>(static_cast<s64>(n))});
}

namespace {
constexpr u32 kKF2430 = 0x000F2430;  // lane masks y / z / w, then f32 {-1, 1, pi/2}, f64 {pi, -pi}, f32 {2pi, 1/65535}
}  // namespace

// void T::T(T* o, const Desc* d) (called by 000F27D0): the base constructor
// (Obj_BaseCtor), vtables 0x00B130E8 / +424 (+380) / +464 (+384), the 4x3
// matrix +640 = identity then d's rows +48, +704 = +712 = 0, +720 = the
// zero vector. With a physics body q = o->+388 whose +8 is set: +720 =
// d->+208..+216, Hk_Activate(q) and q's +224 interface vt[17](.., {+720 as
// a vector, w cleared}). Direction +716 = 1.0 when the random seed's low
// bit is set, else -1.0; with byte +436 of o->+216 (b), +704 = the facing
// angle for it wrapped to (-pi, pi]. +708 = b->+404 + (b->+408 - b->+404) *
// rand / 65535. The random state (0x01164888) advances two steps. f1 / r3
// are left as the original leaves them.
DECOMP(000F2488, Obj_Ctor_000F2488) {
    const u32 o = arg<u32>(ctx, 0), d = arg<u32>(ctx, 1);
    call(ctx, v4fn::Obj_BaseCtor, {o, d});
    constexpr u32 vt = 0x00B130E0u;
    store<u32>(o + 380, vt + 424);
    store<u32>(o + 384, vt + 464);
    constexpr u32 I = 0x01164BD0u;
    for (u32 k = 0; k < 3; ++k) CopyW(o + 640 + 4 * k, I + 4 * k);
    for (u32 k = 0; k < 3; ++k) CopyW(o + 656 + 4 * k, I + 16 + 4 * k);
    CopyW(o + 672, I + 32);
    store<u32>(o, vt + 8);
    CopyW(o + 676, I + 36);
    CopyW(o + 680, I + 40);
    CopyW(o + 688, I + 48);
    CopyW(o + 692, I + 52);
    store<u32>(o + 704, 0);
    CopyW(o + 696, I + 56);
    store<u32>(o + 712, 0);
    for (u32 k = 0; k < 3; ++k) CopyW(o + 720 + 4 * k, kV3Zero3 + 4 * k);
    const u32 q = load<u32>(o + 388), b = load<u32>(o + 216);
    if (q != 0 && load<u32>(q + 8) != 0) {
        for (u32 k = 0; k < 3; ++k) CopyW(o + 720 + 4 * k, d + 208 + 4 * k);
        Stack frame(ctx, 176);
        const u32 v = static_cast<u32>(ctx.gpr[1]) + 112;
        Stvx(v, RowSelW(o + 720, o + 724, o + 728, LoadMasks3(kKF2430)));
        call(ctx, v4fn::Hk_Activate, {q});
        VCall(ctx, q + 224, 68, {q + 224, v});
    }
    for (u32 r = 0; r < 4; ++r)
        for (u32 k = 0; k < 3; ++k) CopyW(o + 640 + 16 * r + 4 * k, d + 48 + 16 * r + 4 * k);
    const u32 s = load<u32>(0x01164890);
    const u32 x1 = (std::rotl(s, 4) + std::rotl(s, 16)) ^ 0x87654321u;
    const f32 dir = F(kKF2430 + ((s & 1u) == 1u ? 52 : 48));
    store<f32>(o + 716, dir);
    if (load<u8>(b + 436) != 0) {
        const f32 a = Muls(Subs(F(kKF2430 + 52), dir), F(kKF2430 + 56));
        const f32 h = Fsels(a, std::bit_cast<f64>(load<u64>(kKF2430 + 64)), std::bit_cast<f64>(load<u64>(kKF2430 + 72)));
        const f32 t = Adds(h, a);
        const f32 tp = F(kKF2430 + 80);
        const f32 n = static_cast<f32>(rt::fcfid(rt::fctid(static_cast<f64>(Divs(t, tp)), true)));
        store<f32>(o + 704, Subs(Fnmsubs(n, tp, t), h));
    }
    const f32 lo = F(b + 404), span = Subs(F(b + 408), lo);
    const u32 x2 = (std::rotl(std::rotl(x1, 16), 4) + std::rotl(std::rotl(x1, 16), 16)) ^ 0x87654321u;
    const f32 u = Muls(static_cast<f32>(static_cast<f64>(x1 & 0xFFFFu)), F(kKF2430 + 84));
    const u32 seed = std::rotl(x2, 16);
    store<u32>(0x01164890, seed);
    store<u64>(0x01164888, x2 & 0xFFFFu);
    const f32 r = Fmadds(span, u, lo);
    store<f32>(o + 708, r);
    ctx.fpr[1] = r;
    ctx.gpr[3] = seed;
}
