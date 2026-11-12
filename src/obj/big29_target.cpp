// Game code, large-function sweep part 29: the target collector's
// candidate test (height window, range and view cone, then a sorted
// insert into the caller's best-score table).
#include "../sly4.h"
#include "big29.h"

using namespace sly4;
using namespace sly4::big29;

namespace {
constexpr u32 kPoolTgt = 0x0026FD60;  // .text: +0 0.0, +4 1e-5, +8 0.5, +12 1.0, +16 -1.0, +24 f64 -1.0, +32 f64 1.0, +40 180/pi
constexpr u32 Target_GetDef = 0x000D951C;  // (Obj*) -> the target component or 0
constexpr u32 Acosf = 0x005E649C;          // f32 acosf(f32)
}  // namespace

// void f(Collector* o, const Frame* eye, Obj* x, Entry table[], s32* count,
// bool keep, bool unique) (called by Targets_Collect270258, f_002704C0,
// f_002708E8, f_00270D78). The point: with o->+8 == 3 and x's target
// component d (Target_GetDef) whose +68 <= 1, d's offset +32 through x's
// matrix +96 (VMX); else x->vfn[284](&p, x). With T = o->+12 and the
// offset e = p - eye->+48: e.y within [T+12, T+16], the range |e| <= T+4
// and the angle (degrees) between e and eye->+32 (0 at |e| <= 1e-5) <=
// T+8 / 2, the score is |e| / T+4 * (1 - T+20) + angle / (T+8 / 2) * T+20.
// Without keep, x->vfn[304](x, o->+4->+8, o->+8, &score) (entry in r7, the
// OPD in r8) must accept it (and may change it). Then (T+0 = the table's
// capacity n) when the score beats the last entry's, x is not already
// listed (with unique), and it beats entry i: entries i..n-2 move down
// one, entry i = {x, score} and *count = min(*count + 1, n).
DECOMP(0026FD90, Targets_Consider_0026FD90) {
    const u32 o = arg<u32>(ctx, 0), eye = arg<u32>(ctx, 1), x = arg<u32>(ctx, 2), table = arg<u32>(ctx, 3), cntp = arg<u32>(ctx, 4);
    const bool keep = static_cast<u8>(ctx.gpr[8]) != 0, unique = static_cast<u8>(ctx.gpr[9]) != 0;
    Stack frame(ctx, 224);
    const u32 sp = frame.addr();  // the original's frame (+128 the point, +144 the score)
    f64 px, py, pz;
    u32 d = 0;
    bool vmx = false;
    if (load<u32>(o + 8) == 3) {
        d = static_cast<u32>(call(ctx, Target_GetDef, {x}));
        vmx = d != 0 && !(load<s32>(d + 68) > 1);
    }
    if (vmx) {
        const big5::V4 q = big5::Add4(Lvx(x + 144), big5::Row3(Lvx(x + 96), Lvx(x + 112), Lvx(x + 128), Fw27(d + 32), Fw27(d + 36), Fw27(d + 40)));
        px = q.e[0], py = q.e[1], pz = q.e[2];
    } else {
        VCall(ctx, x, 284, {sp + 128, x});
        px = Lf(sp + 128), py = Lf(sp + 132), pz = Lf(sp + 136);
    }
    u32 t = load<u32>(o + 12);
    const f64 dy = Subs(py, Lf(eye + 52)), dx = Subs(px, Lf(eye + 48)), dz = Subs(pz, Lf(eye + 56));
    auto leave = [&](u64 r3, f64 f1) {
        ctx.gpr[3] = r3;
        ctx.fpr[1] = f1;
    };
    if (!(dy >= Lf(t + 12)) || !(dy <= Lf(t + 16))) return leave(t, dx);
    const f64 dist = Sqrts(Fmadds(dz, dz, Fmadds(dx, dx, Muls(dy, dy))));
    f64 angle = Lf(kPoolTgt);
    if (dist > Lf(kPoolTgt + 4)) {
        const f64 one = Lf(kPoolTgt + 12), inv = Divs(one, dist);
        const u32 dir = eye + 32;
        const f64 ny = Muls(dy, inv), nx = Muls(dx, inv), nz = Muls(dz, inv);
        const f64 dot = Fmadds(nz, Lf(dir + 8), Fmadds(nx, Lf(dir + 0), Muls(ny, Lf(dir + 4))));
        f64 c = one;
        if (dot < c) c = dot;
        if (c > Lf(kPoolTgt + 16)) {
            c = one;
            if (dot < c) c = dot;
        } else {
            c = Lf(kPoolTgt + 16);
        }
        // fsel clamp to [-1, 1] (f64 -1.0 / 1.0 at +24 / +32).
        f64 a = Adds(c, one);
        a = S(a >= 0.0 ? c : mid15::Ld(kPoolTgt + 24));
        const f64 b = Subs(a, one);
        a = S(b >= 0.0 ? mid15::Ld(kPoolTgt + 32) : a);
        angle = Muls(callf(ctx, Acosf, {}, {a}), Lf(kPoolTgt + 40));
        t = load<u32>(o + 12);
    }
    const f64 range = Lf(t + 4);
    if (!(dist <= range)) return leave(t, range);
    const f64 half = Lf(kPoolTgt + 8);
    if (!(angle <= Muls(Lf(t + 8), half))) return leave(t, range);
    const f64 one = Lf(kPoolTgt + 12);
    f64 score = Muls(Divs(dist, range), Subs(one, Lf(t + 20)));
    const f64 ar = Divs(angle, Muls(Lf(t + 8), half));
    score = Fmadds(ar, Lf(t + 20), score);
    u32 sraw = Fb(score);
    store<u32>(sp + 144, sraw);
    if (!keep) {
        const u32 opd = load<u32>(load<u32>(x) + 304);
        const u32 entry = load<u32>(opd);
        ctx.fpr[1] = ar;  // the registers as the compiled code leaves them
        ctx.fpr[2] = Lf(t + 20);
        ctx.fpr[4] = angle;
        const u64 r = call_ptr(ctx, opd, {x, load<u32>(load<u32>(o + 4) + 8), load<u32>(o + 8), sp + 144, entry, opd});
        if (static_cast<u32>(r) == 0) return;  // r3 / f1 as the call leaves them
        sraw = load<u32>(sp + 144);
        score = LfsF(sraw);
        t = load<u32>(o + 12);
    }
    const u32 n = load<u32>(t);
    const f64 last = Lf(table + 8 * (n - 1) + 4);
    if (!(score < last)) return leave(t, last);
    if (unique && static_cast<s32>(n) > 0)
        for (s32 i = 0; i < static_cast<s32>(n); ++i)
            if (load<u32>(table + 8 * static_cast<u32>(i)) == x) return leave(t, last);
    if (!(static_cast<s32>(n) > 0)) return leave(t, last);
    s32 i = 0;
    f64 f1 = last;
    for (;; ++i) {
        if (!(i < static_cast<s32>(n))) return leave(t, f1);
        f1 = Lf(table + 8 * static_cast<u32>(i) + 4);
        if (score < f1) break;
    }
    for (s32 j = static_cast<s32>(n) - 1; j > i; --j) {
        const u32 dst = table + 8 * static_cast<u32>(j);
        store<u32>(dst, load<u32>(dst - 8));
        const u32 w = load<u32>(dst - 4);
        store<u32>(dst + 4, w);
        f1 = LfsF(w);
    }
    store<u32>(table + 8 * static_cast<u32>(i), x);
    store<u32>(table + 8 * static_cast<u32>(i) + 4, sraw);
    const u64 c1 = static_cast<u64>(load<u32>(cntp)) + 1;
    const s32 cap = load<s32>(t);
    store<u32>(cntp, static_cast<s32>(c1) < cap ? static_cast<u32>(c1) : static_cast<u32>(cap));
    leave(c1, f1);
}
