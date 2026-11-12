// Game code, large-function sweep part 14: object matrix setters and the
// player's link driving.
#include "big14.h"

namespace sly4::obj {

namespace {
inline be_u32& W(u32 base, u32 off) { return gm<be_u32>(base + off); }
inline f64 F(u32 ea) { return LfsW(gm<be_u32>(ea)); }
inline void St(u32 ea, f64 v) { gm<be_u32>(ea) = StfsBits(v); }
inline void CopyW(u32 dst, u32 src) { gm<be_u32>(dst) = gm<be_u32>(src).get(); }
// The f32 word at ea, bit for bit.
inline f32 Fw(u32 ea) { return std::bit_cast<f32>(gm<be_u32>(ea).get()); }
// vmaddfp lane: a * c + b, one single rounding; a NaN operand comes out
// quieted (a, else b, else c), a NaN the operation makes is 0x7FC00000.
inline f32 Vmadd(f32 a, f32 c, f32 b) {
    auto q = [](f32 x) { return std::bit_cast<f32>(std::bit_cast<u32>(x) | 0x00400000u); };
    if (a != a) return q(a);
    if (b != b) return q(b);
    if (c != c) return q(c);
    const f32 r = std::fma(a, c, b);
    return r != r ? std::bit_cast<f32>(0x7FC00000u) : r;
}
// vaddfp lane with the same NaN rules.
inline f32 Vadd(f32 a, f32 b) {
    auto q = [](f32 x) { return std::bit_cast<f32>(std::bit_cast<u32>(x) | 0x00400000u); };
    if (a != a) return q(a);
    if (b != b) return q(b);
    const f32 r = a + b;
    return r != r ? std::bit_cast<f32>(0x7FC00000u) : r;
}
}  // namespace

// u32 Obj_SetMatrixD87CC(Obj* o, const Mat44* m): with a world object at
// o->160, copies the 3x3 + translation part of the 16-byte rows of m (x, y,
// z of each) to the object's rows at +16 / +32 / +48 / +64 and calls
// WorldObj_MarkDirty when any of the 48 bytes differ; then sets 0x800 in
// o->172 (no caller reads r3 or f1).
void Obj_SetMatrixD87CC(u32 o, u32 m) {
    const u32 w = gm<be_u32>(o + 160);
    if (w == 0) return;
    bool same = true;
    for (u32 k = 0; k < 4; ++k)
        same = same && gm<be_u64>(w + 16 + 16 * k) == gm<be_u64>(m + 16 * k) && gm<be_u32>(w + 24 + 16 * k) == gm<be_u32>(m + 8 + 16 * k);
    if (!same) {
        for (u32 k = 0; k < 4; ++k)
            for (u32 j = 0; j < 3; ++j) {
                CopyW(w + 16 + 16 * k + 4 * j, m + 16 * k + 4 * j);
            }
        call(fn14::WorldObj_MarkDirty, w);
    }
    const u32 fl = gm<be_u32>(o + 172) | 0x800u;
    gm<be_u32>(o + 172) = fl;
}

// u16 Player_DriveLink236DB8(Player*, Link* link, const Mat44* m, -, bool
// on; f32 rate) (name?): for every 20-byte entry e of link (+0 entries, +8
// u16 count, both re-read) with a driver e->4 and an object e->8, the
// definition d = e->0 gives a blend t = on && d->44 <= rate <= d->48 ?
// (rate - d->44) / (d->48 - d->44) : 0. The driver's +112 target, when
// set, gets +16 = d->52 ? lerp(d->36, e->12, t) : (on ? e->12 : 0) and its
// +108 object +8 = d->53 ? lerp(d->40, e->16, t) : (on ? e->16 : 0). Then
// the object's rows +16..+72 take Quat_ToMatrix(d quat, d + 16 position)
// times m (vmaddfp chains; the translation row adds m's row 3), followed
// by WorldObj_MarkDirty and WorldObj_UpdateLod(obj, -, 0) (no caller
// reads r3 or f1).
void Player_DriveLink236DB8(u32, u32 link, u32 m, u32, u8 on, f64 rate) {
    constexpr u32 kZero = 0x00236DB0u;   // .text f32 0
    GuestFrame frame(64);
    const u32 q = frame.addr();
    u32 n;
    for (u32 i = 0;; ++i) {
        n = gm<be_u16>(link + 8);
        if (i >= n) break;
        const u32 e = gm<be_u32>(link) + 20 * i;
        const u32 drv = gm<be_u32>(e + 4);
        if (drv == 0 || gm<be_u32>(e + 8) == 0) continue;
        u32 d = gm<be_u32>(e);
        const f64 lo = F(d + 44), hi = F(d + 48);
        const bool in = lo <= rate && hi >= rate;
        const u32 kz = gm<be_u32>(kZero);
        f64 t = F(kZero);
        if (on && in) t = Divs(Subs(rate, lo), Subs(hi, lo));
        const u32 tgt = gm<be_u32>(drv + 112);
        if (tgt != 0) {
            if (gm<u8>(d + 52) != 0) {
                const f64 a = F(d + 36);
                St(tgt + 16, Fmadds(Subs(F(e + 12), a), t, a));
            } else {
                gm<be_u32>(tgt + 16) = on ? gm<be_u32>(e + 12) : kz;
            }
            d = gm<be_u32>(e);
            if (gm<u8>(d + 53) != 0) {
                const f64 a = F(d + 40);
                const f32 v = Fmadds(Subs(F(e + 16), a), t, a);
                St(gm<be_u32>(tgt + 108) + 8, v);
            } else {
                const u32 v = on ? gm<be_u32>(e + 16) : kz;
                gm<be_u32>(gm<be_u32>(tgt + 108) + 8) = v;
            }
        }
        d = gm<be_u32>(e);
        call(fn14::Quat_ToMatrix, q, d, d + 16);
        f32 row[4][4], qm[4][4];
        for (u32 k = 0; k < 4; ++k)
            for (u32 j = 0; j < 4; ++j) {
                row[k][j] = Fw(((m + 16 * k) & ~15u) + 4 * j);   // lvx: the 16-byte blocks holding m's rows
                qm[k][j] = Fw(q + 16 * k + 4 * j);
            }
        f32 r[4][3];
        for (u32 k = 0; k < 4; ++k)
            for (u32 j = 0; j < 3; ++j) {
                const f32 v = Vmadd(row[2][j], qm[k][2], Vmadd(row[1][j], qm[k][1], Vmadd(row[0][j], qm[k][0], 0.0f)));
                r[k][j] = k == 3 ? Vadd(row[3][j], v) : v;
            }
        const u32 obj = gm<be_u32>(e + 8);
        for (u32 k = 0; k < 4; ++k)
            for (u32 j = 0; j < 3; ++j) gm<be_u32>(obj + 16 + 16 * k + 4 * j) = std::bit_cast<u32>(r[k][j]);
        call(fn14::WorldObj_MarkDirty, obj);
        call(fn14::WorldObj_UpdateLod, gm<be_u32>(e + 8), 0u, 0u);
    }
}

}  // namespace sly4::obj

NATIVE(000D87CC, sly4::obj::Obj_SetMatrixD87CC)
NATIVE(00236DB8, sly4::obj::Player_DriveLink236DB8)
