// Game code, large-function sweep part 22: the dragon's attack spawns
// (callee of Dragon_OnMessage_00301C48 / f_002FC600).
#include "big22.h"

using namespace sly4;
using namespace sly4::big22;

namespace {
constexpr u32 kWorld = 0x00F65CA0u;     // world state; +4208 +8 the local player
constexpr u32 kDragonK = 0x002FBEB8u;   // f32 1.0, 0.0, 16.0, (f64 16.0) +16, f32 -16.0 +24, f64 -16.0 +32,
                                        // f32 12.0 +40, f64 12.0 +48, f32 -12.0 +56, f64 -12.0 +64
constexpr u32 kAimDist = 0x0106AB68u;   // f32 the spawn distance toward the player
constexpr u32 kAimLift = 0x0106AB64u;   // f32 the spawn height
constexpr u32 kTwSpread = 0x0106AC90u;  // tweak record (f32 +72): the pair's sideways offset
constexpr u32 kZeroVec = 0x01164B40u;   // f32[3] 0

namespace dfn {
constexpr u32 Obj_SpawnAt = 0x002FBCC8u;    // (Obj*, Def*, const Mat34*, const Vec3* to, dir; f32 v, f32)
constexpr u32 Obj_SetFocus = 0x000EB2F0u;   // (Obj*, Obj* u)
}  // namespace dfn

u32 PlayerOf(u32 pl) { return pl != 0 && (load<u32>(pl + 192) & 8u) != 0 ? pl : 0; }

// Obj_SpawnAt2FBCC8(o, def, m, to, dir, r8 / r9 as o's vfn[25] left them,
// that call's result; f32 the speed / spread tweaks +72 of the pair picked
// by o->432 == 1).
struct Tw2 {
    f64 v, w;
};
Tw2 Pick(u32 o, u32 t1a, u32 t2a, u32 t1b, u32 t2b) {
    const bool one = load<u32>(o + 432) == 1;
    return {Lf((one ? t1a : t1b) + 72), Lf((one ? t2a : t2b) + 72)};
}
u32 Spawn(PPUContext& ctx, u32 o, u32 def, u32 m, u32 to, u32 dir, Tw2 t) {
    const f64 v = t.v, w = t.w;
    const u32 r10 = static_cast<u32>(VCall(ctx, o, 100, {o}));
    const u64 r8 = ctx.gpr[8], r9 = ctx.gpr[9];
    callf(ctx, dfn::Obj_SpawnAt, {o, def, m, to, dir, r8, r9, r10}, {v, w});
    return static_cast<u32>(ctx.gpr[3]);
}
}  // namespace

// u32 Dragon_AttackSpawn2FBF00(Dragon* o, Emitter* s, u32 msg, Def* def):
// with the local player present (world +4208 +8, bit 3 of +192) and its
// vfn[16] set, the frame m = s's rows +0 / +16 / +32 and position +48, and
// the unit direction from s+48 to o's target +480 (1 / sqrt, no zero
// check). The point p = o+480 + dir * D (0x0106AB68), raised by H
// (0x0106AB64), is the spawn target:
//   0x952A45AC: one spawn at p (tweaks 0x0106AE0C / 0x0106AE58, or
//     0x0106B06C / 0x0106B0B8 unless o->432 == 1), direction zero;
//   0x952A45AD: two at p +/- (1, 0, 0) * the spread tweak 0x0106AC90
//     (0x0106AEA4 / 0x0106AEF0, or 0x0106B104 / 0x0106B150);
//   0x952A45AE: one with the player's vfn[35] vector (y forced to 0) as
//     the direction, p pushed by (x, 0.0, z) * o->460's f32 +412 when it has
//     bit 16 of +20, x clamped to [-16, 16] and z to [-12, 12] (0x0106AF3C
//     / 0x0106AF88, or 0x0106B19C / 0x0106B1E8);
//   0x0969AF51: like 0x952A45AC, and the spawned object takes the player
//     (Obj_SetFocus_000EB2F0); returns it.
// Returns 0 otherwise.
DECOMP(002FBF00, Dragon_AttackSpawn2FBF00) {
    const u32 o = arg<u32>(ctx, 0), s = arg<u32>(ctx, 1), msg = arg<u32>(ctx, 2), def = arg<u32>(ctx, 3);
    constexpr u32 K = kDragonK;
    ret(ctx, 0u);
    {
        const u32 pl = PlayerOf(load<u32>(load<u32>(kWorld) + 4208 + 8));
        if (pl == 0 || static_cast<u32>(VCall(ctx, pl, 64, {pl})) == 0) {
            ret(ctx, 0u);
            return;
        }
    }
    Stack frame(ctx, 160);
    const u32 sp = frame.addr();  // sp + k stands for r1 + 112 + k
    const u32 m = sp + 96;        // r1+208: the 3x4 frame and the position
    const f64 sy = Lf(s + 52), oy = Lf(o + 484), sx = Lf(s + 48);
    const f64 dy = S(oy - sy);
    const f64 ox = Lf(o + 480), sz = Lf(s + 56);
    const f64 dx = S(ox - sx);
    const f64 oz = Lf(o + 488);
    const f64 dz = S(oz - sz);
    for (u32 r = 0; r < 3; ++r)
        for (u32 k = 0; k < 3; ++k) Cw(m + 16 * r + 4 * k, s + 16 * r + 4 * k);
    Cw(m + 48, s + 48);  // stfs of the loaded words
    Cw(m + 52, s + 52);
    Cw(m + 56, s + 56);
    const f64 one = Lf(K + 0);
    const f64 inv = Divs(one, Sqrts(Fmadds(dz, dz, Fmadds(dx, dx, S(dy * dy)))));
    const f64 ny = S(dy * inv), nx = S(dx * inv), nz = S(dz * inv);
    const u32 slot = load<u32>(kWorld) + 4208;
    const f64 D = Lf(kAimDist), H = Lf(kAimLift);
    if (msg == 0x952A45ACu || msg == 0x0969AF51u) {
        const u32 to = msg == 0x952A45ACu ? sp + 0 : sp + 80;  // r1+112 / r1+192
        const f64 y = Fmadds(ny, D, oy);
        Sf(to + 0, Fmadds(nx, D, ox));
        Sf(to + 4, y);
        Sf(to + 8, Fmadds(nz, D, oz));
        Sf(to + 4, S(y + H));
        const u32 r = Spawn(ctx, o, def, m, to, kZeroVec, Pick(o, 0x0106AE0Cu, 0x0106AE58u, 0x0106B06Cu, 0x0106B0B8u));
        if (msg == 0x952A45ACu || r == 0) {
            ret(ctx, 0u);
            return;
        }
        call(ctx, dfn::Obj_SetFocus, {r, PlayerOf(load<u32>(load<u32>(kWorld) + 4208 + 8))});
        ret(ctx, r);
    } else if (msg == 0x952A45ADu) {
        const f64 y = S(Fmadds(ny, D, oy) + H);
        const f64 x = Fmadds(nx, D, ox), z = Fmadds(nz, D, oz);
        const f64 zero = Lf(K + 4), w = Lf(kTwSpread + 72);
        const f64 a = Fmadds(one, w, x), b = Fmadds(zero, w, z), c = Fmadds(-one, w, x);
        const f64 d = Fmadds(-zero, w, z), e = Fmadds(zero, w, y), f = Fmadds(-zero, w, y);
        Sf(sp + 16, a);
        Sf(sp + 24, b);
        Sf(sp + 32, c);
        Sf(sp + 20, e);
        Sf(sp + 36, f);
        Sf(sp + 40, d);
        const Tw2 t = Pick(o, 0x0106AEA4u, 0x0106AEF0u, 0x0106B104u, 0x0106B150u);  // read once for both
        Spawn(ctx, o, def, m, sp + 16, kZeroVec, t);
        Spawn(ctx, o, def, m, sp + 32, kZeroVec, t);
        ret(ctx, 0u);
    } else if (msg == 0x952A45AEu) {
        const u32 to = sp + 48, out = sp + 64;  // r1+160 / r1+176
        const f64 y = Fmadds(ny, D, oy);
        Sf(to + 0, Fmadds(nx, D, ox));
        Sf(to + 4, y);
        Sf(to + 8, Fmadds(nz, D, oz));
        Sf(to + 4, S(y + H));
        const bool first = load<u32>(o + 432) == 1;
        const f64 v = Lf((first ? 0x0106AF3Cu : 0x0106B19Cu) + 72), w = Lf((first ? 0x0106AF88u : 0x0106B1E8u) + 72);
        const u32 pl = PlayerOf(load<u32>(slot + 8));
        VCall(ctx, pl, 140, {out, pl});
        store<u32>(out + 4, 0);
        const f64 zero = Lf(K + 4);
        const u32 opd = load<u32>(load<u32>(o) + 100);  // o's vfn[25] taken before the push
        if (const u32 h = load<u32>(o + 460); h != 0 && (load<u32>(h + 20) & 0x10000u) != 0) {
            const f64 k = Lf(h + 412);
            const f64 px = Fmadds(Lf(out + 0), k, Lf(to + 0));
            const f64 py = Fmadds(zero, k, Lf(to + 4));
            const f64 pz = Fmadds(Lf(out + 8), k, Lf(to + 8));
            Sf(to + 0, px);
            Sf(to + 4, py);
            Sf(to + 8, pz);
        }
        f64 x = Lf(to + 0);
        x = S(mid15::Fsel15(S(Lf(K + 8) - x), x, mid15::Ld(K + 16)));
        x = S(mid15::Fsel15(S(x - Lf(K + 24)), x, mid15::Ld(K + 32)));
        const f64 z0 = Lf(to + 8);
        Sf(to + 0, x);
        f64 z = S(mid15::Fsel15(S(Lf(K + 40) - z0), z0, mid15::Ld(K + 48)));
        z = S(mid15::Fsel15(S(z - Lf(K + 56)), z, mid15::Ld(K + 64)));
        Sf(to + 8, z);
        const u32 r10 = static_cast<u32>(call_ptr(ctx, opd, {o, opd, load<u32>(opd)}));
        const u64 r8 = ctx.gpr[8], r9 = ctx.gpr[9];
        callf(ctx, dfn::Obj_SpawnAt, {o, def, m, to, out, r8, r9, r10}, {v, w});
        ret(ctx, 0u);
    } else {
        ret(ctx, 0u);
    }
}
