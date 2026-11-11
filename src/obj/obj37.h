// Game code, part 37: helpers shared by obj37*.cpp (the continued sweep of
// small still-recompiled game functions, in size order, from the last 73
// instruction ones up).
#pragma once

#include <bit>
#include <cmath>

#include "obj36.h"

namespace sly4 {
namespace obj37 {

using namespace opentit;
using namespace opentit::decomp;
using namespace sly4::obj36;

// The closest point of 0x00013E40 / 0x000FD1A0 (void f(Vec3* out, ..., a,
// b, p), the constants k = {eps, 1, 0}): d = b - a; when |d| >= eps (not
// NaN), the unit direction u = d * (1 / |d|) (fdivs, then fmuls): t =
// u . (p - a); for t > 0, s = u . (p - b); for s < 0, out = b + u * s
// (fmadds; f1 = out.x), else out = b; otherwise out = a. The copies are
// raw words, f1 their z. All arithmetic single precision.
inline void SegClosest37(PPUContext& ctx, u32 out, u32 a, u32 b, u32 p, u32 k) {
    auto f = [](f64 x) { return static_cast<f64>(static_cast<f32>(x)); };
    const f64 ax = Lfs32(a), ay = Lfs32(a + 4), az = Lfs32(a + 8);
    const f64 bx = Lfs32(b), by = Lfs32(b + 4), bz = Lfs32(b + 8);
    f64 dy = f(by - ay), dx = f(bx - ax), dz = f(bz - az);
    f64 l = f(dy * dy);
    l = f(std::fma(dx, dx, l));
    l = f(std::fma(dz, dz, l));
    l = f(std::sqrt(l));
    auto copy = [&](u32 s) {
        store<u32>(out, load<u32>(s));
        store<u32>(out + 4, load<u32>(s + 4));
        const u32 z = load<u32>(s + 8);
        store<u32>(out + 8, z);
        ctx.fpr[1] = static_cast<f64>(std::bit_cast<f32>(z));
    };
    if (!(l >= Lfs32(k))) {
        copy(a);
        return;
    }
    const f64 inv = f(Lfs32(k + 4) / l);
    const f64 px = Lfs32(p), py = Lfs32(p + 4), pz = Lfs32(p + 8);
    const f64 ey = f(py - ay), ex = f(px - ax), ez = f(pz - az);
    const f64 zero = Lfs32(k + 8);
    dy = f(dy * inv);
    dx = f(dx * inv);
    dz = f(dz * inv);
    f64 t = f(dy * ey);
    t = f(std::fma(dx, ex, t));
    t = f(std::fma(dz, ez, t));
    if (!(t > zero)) {
        copy(a);
        return;
    }
    f64 s = f(dy * f(py - by));
    s = f(std::fma(dx, f(px - bx), s));
    s = f(std::fma(dz, f(pz - bz), s));
    if (!(s < zero)) {
        copy(b);
        return;
    }
    const f64 ox = f(std::fma(dx, s, bx)), oy = f(std::fma(dy, s, by)), oz = f(std::fma(dz, s, bz));
    Stfs32(out, ox);
    Stfs32(out + 4, oy);
    Stfs32(out + 8, oz);
    ctx.fpr[1] = ox;
}

// The attribute push of 0x004EE45C / 0x004EEE68 (bool f(XmlCursor* x)):
// with the settings g = *0x010C2C28 and the attribute name of x: a heap
// copy (Mem_Alloc(len + 1, 0, 16), String_Assign, terminated at
// String_Length again; the allocation is not checked) pushed on the
// pointer vector g + voff (data +0, u16 capacity +4 doubled by
// PtrVector_Reserve when full, u16 count +8, u16 high-water +10). r3 = 0
// without g / the attribute, else 1.
inline void PushAttrStr37(PPUContext& ctx, u32 name, u32 voff) {
    const u32 x = arg<u32>(ctx, 0);
    const u32 g = load<u32>(0x010C2C28u);
    if (g == 0) {
        ret(ctx, 0u);
        return;
    }
    const u32 s = static_cast<u32>(call(ctx, 0x004F9628u, {x, name}));
    if (s == 0) {
        ret(ctx, 0u);
        return;
    }
    const u32 n = static_cast<u32>(call(ctx, 0x00555FA4u, {s}));
    const u32 m = static_cast<u32>(call(ctx, 0x0054ED38u, {n + 1u, 0, 16}));
    call(ctx, 0x00555F80u, {m, s});
    const u32 l = static_cast<u32>(call(ctx, 0x00555FA4u, {s}));
    store<u8>(m + l, 0);
    const u32 v = g + voff;
    u32 cnt = load<u16>(v + 8);
    const u32 cap = load<u16>(v + 4);
    if (cnt == cap) {
        call(ctx, 0x00551718u, {v, cap << 1});
        cnt = load<u16>(v + 8);
    }
    const u32 d = load<u32>(v);
    const u32 c2 = load<u16>(v + 8);
    const u32 hw = load<u16>(v + 10);
    store<u32>(d + (cnt << 2), m);
    const u32 n1 = c2 + 1u;
    store<u16>(v + 8, static_cast<u16>(n1));
    store<u16>(v + 10, static_cast<u16>(static_cast<s32>(hw) > static_cast<s32>(n1 & 0xFFFFu) ? hw : n1));
    ret(ctx, 1u);
}

// The destructor body of 0x0027CE34 / 0x0027E968 / 0x00280118 (void
// f(T* o)): vtables v0 / +80 v80, Dtor_B219D0_00464AD0(o + 80), vtable
// 0x00B1BE30; o is taken out of the 8-byte entry array of 0x01056F1C
// (u16 count +2, data +8, the object in each entry's second word; the
// later entries moved down one, the count decreased) when found, then
// List1013B5C_Remove(o) (r3 = its).
inline void DtorUnlist37(PPUContext& ctx, u32 v0, u32 v80, u32 delta = 0) {
    const u32 o = arg<u32>(ctx, 0) - delta;  // the +80 base thunks pass delta 80
    store<u32>(o, v0);
    store<u32>(o + 80, v80);
    call(ctx, 0x00464AD0u, {o + 80});
    constexpr u32 A = 0x01056F1Cu;
    store<u32>(o, 0x00B1BE30u);
    const u32 n = load<u16>(A + 2);
    const u32 d = load<u32>(A + 8);
    for (u32 i = 0; i < n; ++i) {
        if (load<u32>(d + 8 * i + 4) != o) continue;
        u32 c = n;
        if (static_cast<s32>(n) >= 2 && static_cast<s32>(i) < static_cast<s32>(n - 1)) {
            for (u32 j = i;;) {
                const u32 e = d + 8 * j;
                ++j;
                store<u32>(d + 8 * (j - 1), load<u32>(e + 8));
                store<u32>(e + 4, load<u32>(e + 12));
                c = load<u16>(A + 2);
                if (!(static_cast<s32>(j) < static_cast<s32>(c - 1))) break;
            }
        }
        store<u16>(A + 2, static_cast<u16>(c - 1));
        break;
    }
    call(ctx, 0x000D6940u, {o});
}

}  // namespace obj37
}  // namespace sly4
