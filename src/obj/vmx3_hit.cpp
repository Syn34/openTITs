// Game code, VMX sweep part 3 (see vmx3.h): hit / damage callbacks.
#include "../sly4.h"
#include "vmx3.h"

using namespace sly4;
using namespace sly4::vmx3;

namespace {
constexpr u32 kPool16F730 = 0x0016F6E0;  // f32 0; +16 three lane masks; +64 f32 180, pi, 360
constexpr u32 kHkActivate = 0x0091A4C8;  // Hk_Activate91A4C8 (body)
constexpr u32 kAtan2f = 0x005E665C;      // atan2f (f1 y, f2 x)
constexpr u32 kHashDamage = 0x045D9554;  // the damage message hash (as 001FBC80 uses)
}  // namespace

// void f(Obj* o, HitEvent* h) (a callback; no direct callers). An attacker
// (h+4) whose def (+216) +52 is 0x659234F5 sets bit 0x00800000 of +656.
// With a body (+388) not in state 4 / 5 (byte +232), a push h+68 != 0 and
// a def (+216) push scale +200 != 0: the body is activated and gets the
// impulse -h+32 * push * scale (lanes x, y, z) at the hit point h+16
// (body+224 vfn[22](impulse, point)). With def +196 > 0: damage h+64 * that
// through o->vfn[88](o, -, attacker, 0x045D9554); bit 0x100 of the u16
// h+14 = (it returned 0), and +736 = the hit's heading around o (degrees of
// atan2f(h.y - o+148, h.x - o+144), wrapped to [0, 360]). f1 / r3 are left
// as the original leaves them (the heading or def +196; the pool address).
DECOMP(0016F730, Obj_OnHitPushHeading_0016F730) {
    const u32 o = arg<u32>(ctx, 0), h = arg<u32>(ctx, 1);
    const u32 K = kPool16F730;
    Stack frame(ctx, 208);
    const u32 sp = frame.addr();
    const u32 att = load<u32>(h + 4);
    const u32 body = load<u32>(o + 388);
    const u32 def = load<u32>(o + 216);
    if (att != 0 && load<s32>(load<u32>(att + 216) + 52) == 0x659234F5) store<u32>(o + 656, load<u32>(o + 656) | 0x00800000u);
    if (body != 0) {
        const u8 st = load<u8>(body + 232);
        const f32 push = F(h + 68), zero = F(K);
        if (st != 5 && st != 4 && push != zero) {
            const f32 sc = F(def + 200);
            if (sc != zero) {
                const f32 ix = Muls(Muls(-F(h + 32), push), sc);
                const f32 iy = Muls(Muls(-F(h + 36), push), sc);
                const f32 iz = Muls(Muls(-F(h + 40), push), sc);
                const Masks3 mk = LoadMasks3(K + 16);
                Stvx(sp + 112, RowSel(Fill4(ix), Fill4(iy), Fill4(iz), mk));
                Stvx(sp + 128, RowSelW(h + 16, h + 20, h + 24, mk));
                call(ctx, kHkActivate, {body});
                VCall(ctx, body + 224, 88, {body + 224, sp + 112, sp + 128});
            }
        }
    }
    const f32 dmg = F(def + 196);
    const f32 zero = F(K);
    f64 f1 = dmg;
    if (dmg > zero) {
        const u32 opd = load<u32>(load<u32>(o) + 352);
        ctx.fpr[1] = Muls(F(h + 64), dmg);
        const u32 r = static_cast<u32>(call_ptr(ctx, opd, {o, opd, att, kHashDamage, load<u32>(opd)}));
        store<u16>(h + 14, static_cast<u16>((load<u16>(h + 14) & ~0x100u) | (r == 0 ? 0x100u : 0u)));
        const f32 dx = Subs(F(h + 16), F(o + 144));
        const f32 dy = Subs(F(h + 20), F(o + 148));
        f32 a = Divs(Muls(callf(ctx, kAtan2f, {}, {dy, dx}), F(K + 64)), F(K + 68));
        if (a < zero) a = Adds(a, F(K + 72));
        if (a > F(K + 72)) a = Subs(a, F(K + 72));
        store<f32>(o + 736, a);
        f1 = a;
    }
    ctx.fpr[1] = f1;
    ctx.gpr[3] = K;
}
