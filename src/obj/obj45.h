// Game code, part 45: helpers shared by obj45*.cpp (the continued sweep of
// small still-recompiled game functions, in size order, from the remaining
// 97 instruction ones up).
#pragma once

#include <bit>
#include <cmath>

#include "obj44.h"

namespace sly4 {
namespace obj45 {

using namespace opentit;
using namespace opentit::decomp;
using namespace sly4::obj44;

// The raw f32 word of a register value (stfs).
inline u32 W45(f64 x) { return std::bit_cast<u32>(static_cast<f32>(x)); }
// lfs: the f32 at ea as a double.
inline f64 Lfs45(u32 ea) { return static_cast<f64>(load<f32>(ea)); }
// Single-precision rounding (the s-form result).
inline f64 F45(f64 x) { return static_cast<f64>(static_cast<f32>(x)); }
// fmadds / fmsubs / fnmsubs, fused: (f32)(a * c + b), (f32)(a * c - b),
// (f32)(b - a * c).
inline f64 Fma45(f64 a, f64 c, f64 b) { return F45(std::fma(a, c, b)); }
inline f64 Fms45(f64 a, f64 c, f64 b) { return F45(std::fma(a, c, -b)); }
inline f64 Fnms45(f64 a, f64 c, f64 b) { return F45(-std::fma(a, c, -b)); }
// lfs / stfs pair: copies the raw word.
inline void Cp45(u32 dst, u32 src) { store<u32>(dst, load<u32>(src)); }

// The speech setter of 0x0037CB10 / 0x0037F918 (u32 f(T* o, -, -, bool
// set, Obj* tgt, const u8 key[4], f32 t)): unless bit 0x02000000 of +252
// is set or 0x40000000 is clear (r3 = the flag word then): set -> +252 |=
// 0x20000000; +228 = key, +240 = t; for the mode (+160 >> 1) 2 or 3:
// +244 = t when t >= *kc, mode 2, and Speaker_SayId4A5C90(o->+28,
// o->+208, 0, *(kc + 4)) & 1 into bit 0x01000000 of +252, then (when
// o->+20 is set) Effect_SetHash4CF988(o->+20, *hash_ea) & 1 into bit
// 0x00800000; for modes 0 and 6: +248 = tgt and fn_aim(o) unless
// tgt->+24->+164 has bit 0x8000. r3 = fn_tail(o) for every mode.
inline void SetSpeech45(PPUContext& ctx, u32 kc, u32 hash_ea, u32 fn_aim, u32 fn_tail) {
    const u32 o = arg<u32>(ctx, 0), set = arg<u32>(ctx, 3), tgt = arg<u32>(ctx, 4), key = arg<u32>(ctx, 5);
    const f64 t = farg(ctx, 0);
    u32 w = load<u32>(o + 252);
    if ((w & 0x02000000u) != 0 || (w & 0x40000000u) == 0) {
        ret(ctx, w);
        return;
    }
    if ((set & 0xFFu) != 0) {
        w |= 0x20000000u;
        store<u32>(o + 252, w);
    }
    for (u32 k = 0; k < 4; ++k) store<u8>(o + 228 + k, load<u8>(key + k));
    u32 m = load<u32>(o + 160);
    store<u32>(o + 240, W45(t));
    const u32 mode = (m >> 1) & 0x7FFFFFFFu;
    const u32 c28 = load<u32>(o + 28);
    if (mode == 2 || mode == 3) {
        store<u32>(o + 244, t >= Lfs45(kc) ? W45(t) : load<u32>(o + 236));
        m = (m & 1u) | 4u;
        store<u32>(o + 160, m);
        const u32 r = static_cast<u32>((callf(ctx, 0x004A5C90u, {c28, load<u32>(o + 208), 0}, {Lfs45(kc + 4)}), ctx.gpr[3]));
        const u32 w2 = (load<u32>(o + 252) & ~0x01000000u) | ((r & 1u) << 24);
        const u32 fx = load<u32>(o + 20);
        store<u32>(o + 252, w2);
        if (fx != 0) {
            const u32 r2 = static_cast<u32>(call(ctx, 0x004CF988u, {fx, load<u32>(hash_ea)}));
            store<u32>(o + 252, (load<u32>(o + 252) & ~0x00800000u) | ((r2 & 1u) << 23));
        }
    } else if (mode == 0 || mode == 6) {
        store<u32>(o + 248, tgt);
        bool skip = false;
        if (tgt != 0) {
            const u32 x = load<u32>(tgt + 24);
            if (x != 0 && (load<u32>(x + 164) & 0x8000u) != 0) skip = true;
        }
        if (!skip) call(ctx, fn_aim, {o});
    }
    ret(ctx, call(ctx, fn_tail, {o}));
}

// The speech start of 0x003772C0 / 0x0037AC70 (u32 f(T* o)): +252 |=
// 0x40000000, mode 1 (bits 1.. of +160); o->+24->vt[40](it, 1, 1) when
// set; Speaker_SayId4A5C90(o->+28, o->+200[mode], 0; *kc) & 1 into bit
// 0x01000000 of +252, then (o->+20 set) Effect_SetHash4CF988(o->+20,
// table[mode]) & 1 into bit 0x00800000; then for each 28-byte entry e of
// the owner o->+8 (array +arr_off, count +cnt_off) with e->+12 set,
// e->+12->vt[88](it, e->+16 != 0). r3 = the count.
inline void StartSpeech45(PPUContext& ctx, u32 kc, u32 table, u32 cnt_off, u32 arr_off) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 w = load<u32>(o + 252) | 0x40000000u;
    u32 m = load<u32>(o + 160);
    const u32 own = load<u32>(o + 8);
    m = (m & 1u) | 2u;
    const u32 c24 = load<u32>(o + 24);
    store<u32>(o + 252, w);
    store<u32>(o + 160, m);
    if (c24 != 0) {
        VCall(ctx, c24, 40, {c24, 1, 1});
        m = load<u32>(o + 160);
    }
    const u32 i4 = ((m >> 1) & 0x7FFFFFFFu) * 4;
    const u32 r = static_cast<u32>((callf(ctx, 0x004A5C90u, {load<u32>(o + 28), load<u32>(o + 200 + i4), 0}, {Lfs45(kc)}), ctx.gpr[3]));
    const u32 w2 = (load<u32>(o + 252) & ~0x01000000u) | ((r & 1u) << 24);
    const u32 fx = load<u32>(o + 20);
    store<u32>(o + 252, w2);
    if (fx != 0) {
        const u32 r2 = static_cast<u32>(call(ctx, 0x004CF988u, {fx, load<u32>(table + i4)}));
        store<u32>(o + 252, (load<u32>(o + 252) & ~0x00800000u) | ((r2 & 1u) << 23));
    }
    u32 n = load<u32>(own + cnt_off);
    for (u32 i = 0; i < n; ++i) {
        const u32 e = load<u32>(own + arr_off) + 28 * i;
        const u32 c = load<u32>(e + 12);
        if (c == 0) continue;
        VCall(ctx, c, 88, {c, load<u32>(e + 16) != 0 ? 1u : 0u});
        n = load<u32>(own + cnt_off);
    }
    ret(ctx, n);
}

}  // namespace obj45
}  // namespace sly4
