// Game code, large-function sweep part 14: object matrix setters and the
// player's link driving.
#include "big14.h"

using namespace sly4;
using namespace sly4::big14;

// u32 Obj_SetMatrixD87CC(Obj* o, const Mat44* m): with a world object at
// o->160, copies the 3x3 + translation part of the 16-byte rows of m (x, y,
// z of each) to the object's rows at +16 / +32 / +48 / +64 and calls
// WorldObj_MarkDirty when any of the 48 bytes differ; then sets 0x800 in
// o->172 and returns the new flags. Without a world object it returns m
// (the original's leftover r3).
DECOMP(000D87CC, Obj_SetMatrixD87CC) {
    const u32 o = arg<u32>(ctx, 0), m = arg<u32>(ctx, 1);
    const u32 w = load<u32>(o + 160);
    if (w == 0) {
        ctx.gpr[3] = m;
        return;
    }
    bool same = true;
    for (u32 k = 0; k < 4; ++k)
        same = same && load<u64>(w + 16 + 16 * k) == load<u64>(m + 16 * k) && load<u32>(w + 24 + 16 * k) == load<u32>(m + 8 + 16 * k);
    if (!same) {
        u32 last = 0;
        for (u32 k = 0; k < 4; ++k)
            for (u32 j = 0; j < 3; ++j) {
                last = load<u32>(m + 16 * k + 4 * j);
                store<u32>(w + 16 + 16 * k + 4 * j, last);
            }
        SetF1Bits(ctx, last);   // f1 as the lfs / stfs copy leaves it
        call(ctx, b14fn::WorldObj_MarkDirty, {w});
    }
    const u32 fl = load<u32>(o + 172) | 0x800u;
    store<u32>(o + 172, fl);
    ctx.gpr[3] = fl;
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
// by WorldObj_MarkDirty and WorldObj_UpdateLod(obj, -, 0). Returns the
// count (the original's leftover r3).
DECOMP(00236DB8, Player_DriveLink236DB8) {
    constexpr u32 kZero = 0x00236DB0u;   // .text f32 0
    const u32 link = arg<u32>(ctx, 1), m = arg<u32>(ctx, 2);
    const bool on = (ctx.gpr[7] & 0xFFu) != 0;
    const f64 rate = ctx.fpr[1];
    Stack frame(ctx, 64);
    const u32 q = frame.addr();
    u32 n;
    for (u32 i = 0;; ++i) {
        n = load<u16>(link + 8);
        if (i >= n) break;
        const u32 e = load<u32>(link) + 20 * i;
        const u32 drv = load<u32>(e + 4);
        if (drv == 0 || load<u32>(e + 8) == 0) continue;
        u32 d = load<u32>(e);
        const f64 lo = LfsX(d + 44), hi = LfsX(d + 48);
        const bool in = lo <= rate && hi >= rate;
        const u32 kz = load<u32>(kZero);
        f64 t = LfsX(kZero);
        if (on && in) t = Divs(Subs(rate, lo), Subs(hi, lo));
        const u32 tgt = load<u32>(drv + 112);
        if (tgt != 0) {
            if (load<u8>(d + 52) != 0) {
                const f64 a = LfsX(d + 36);
                Stfs(tgt + 16, Fmadds(Subs(LfsX(e + 12), a), t, a));
            } else {
                store<u32>(tgt + 16, on ? load<u32>(e + 12) : kz);
            }
            d = load<u32>(e);
            if (load<u8>(d + 53) != 0) {
                const f64 a = LfsX(d + 40);
                const f32 v = Fmadds(Subs(LfsX(e + 16), a), t, a);
                Stfs(load<u32>(tgt + 108) + 8, v);
            } else {
                const u32 v = on ? load<u32>(e + 16) : kz;
                store<u32>(load<u32>(tgt + 108) + 8, v);
            }
        }
        d = load<u32>(e);
        call(ctx, b14fn::Quat_ToMatrix, {q, d, d + 16});
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
                r[k][j] = k == 3 ? row[3][j] + v : v;
            }
        const u32 obj = load<u32>(e + 8);
        for (u32 k = 0; k < 4; ++k)
            for (u32 j = 0; j < 3; ++j) store<u32>(obj + 16 + 16 * k + 4 * j, std::bit_cast<u32>(r[k][j]));
        SetF1Bits(ctx, std::bit_cast<u32>(r[3][2]));   // f1 as the lfs / stfs copy leaves it
        call(ctx, b14fn::WorldObj_MarkDirty, {obj});
        call(ctx, b14fn::WorldObj_UpdateLod, {load<u32>(e + 8), 0, 0});
    }
    ctx.gpr[3] = n;
}
