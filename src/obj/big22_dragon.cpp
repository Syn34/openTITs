// Game code, large-function sweep part 22: the dragon's attack spawns
// (callee of Dragon_OnMessage_00301C48 / f_002FC600).
#include "big22.h"

namespace sly4::obj {

namespace {
constexpr u32 kDragonK = 0x002FBEB8u;   // f32 1.0, 0.0, 16.0, (f64 16.0) +16, f32 -16.0 +24, f64 -16.0 +32,
                                        // f32 12.0 +40, f64 12.0 +48, f32 -12.0 +56, f64 -12.0 +64
constexpr u32 kAimDist = 0x0106AB68u;   // f32 the spawn distance toward the player
constexpr u32 kAimLift = 0x0106AB64u;   // f32 the spawn height
constexpr u32 kTwSpread = 0x0106AC90u;  // tweak record (f32 +72): the pair's sideways offset
constexpr u32 Obj_SpawnAt = 0x002FBCC8u;    // (Obj*, Def*, const Mat34*, const Vec3* to, dir; f32 v, f32 w, u32 x)
constexpr u32 Obj_SetFocus = 0x000EB2F0u;   // (Obj*, Obj* u)

// The dragon: +432 the phase (1: the first tweak set), +460 its helper
// (+20 bit 16, f32 +412), +480 its target point; vfn[25] the value
// Obj_SpawnAt gets last.
struct Dragon {
    pad<432> _0;
    be_u32 phase;    // +432
    pad<24> _436;
    be_u32 helper;   // +460
    pad<16> _464;
    Vec3 target;     // +480
};
static_assert(offsetof(Dragon, phase) == 432 && offsetof(Dragon, helper) == 460 && offsetof(Dragon, target) == 480);
// The emitter: its rows +0 / +16 / +32 and position +48.
struct DragonEmitter {
    Mtx44 m;
};
// The original's locals (sp offsets - 112).
struct DragonLocals {
    Vec4 to0;        // sp+112
    Vec4 pair[2];    // sp+128 / sp+144
    Vec4 to3;        // sp+160
    Vec4 out3;       // sp+176
    Vec4 to1;        // sp+192
    Mtx44 m;         // sp+208
};
static_assert(offsetof(DragonLocals, to3) == 48 && offsetof(DragonLocals, to1) == 80 && offsetof(DragonLocals, m) == 96);

f64 C(u32 off) { return lf(global<be_f32>(kDragonK + off)); }
f64 Cd(u32 off) { return ld(global<be_f64>(kDragonK + off)); }
f64 Tw(u32 rec) { return lf(gm<be_f32>(rec + 72)); }
u32 PlayerOf(u32 pl) { return pl != 0 && (gm<be_u32>(pl + 192) & 8u) != 0 ? pl : 0; }
u32 LocalPlayer() { return PlayerOf(gm<be_u32>(global<be_u32>(kWorld) + 4208 + 8)); }

// The speed / spread tweaks +72 of the pair picked by o->432 == 1.
struct Tw2 {
    f64 v, w;
};
Tw2 Pick(const Dragon* o, u32 t1a, u32 t2a, u32 t1b, u32 t2b) {
    const bool one = o->phase == 1;
    return {Tw(one ? t1a : t1b), Tw(one ? t2a : t2b)};
}
// Obj_SpawnAt2FBCC8(o, def, m, to, dir; v, w; o's vfn[25]()).
u32 Spawn(Dragon* o, u32 def, Mtx44* m, Vec4* to, u32 dir, Tw2 t) {
    const u32 r10 = vcall<u32>(o, 100);
    return call<u32>(Obj_SpawnAt, o, def, m, to, dir, t.v, t.w, r10);
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
// Returns 0 otherwise (f_002FC600 reads r3).
u32 Dragon_AttackSpawn2FBF00(Dragon* o, DragonEmitter* s, u32 msg, u32 def) {
    {
        const u32 pl = LocalPlayer();
        if (pl == 0 || vcall<u32>(&gm<u8>(pl), 64) == 0) return 0;
    }
    GuestFrame frame(160);
    DragonLocals& L = *frame.as<DragonLocals>();
    const Vec4& sp = s->m.r[3];
    const f64 sy = lf(sp.y), oy = lf(o->target.y), sx = lf(sp.x);
    const f64 dy = S(oy - sy);
    const f64 ox = lf(o->target.x), sz = lf(sp.z);
    const f64 dx = S(ox - sx);
    const f64 oz = lf(o->target.z);
    const f64 dz = S(oz - sz);
    CopyMtxXYZ(L.m, s->m);  // stfs of the loaded words
    const f64 one = C(0);
    const f64 inv = Divs(one, Sqrts(Fmadds(dz, dz, Fmadds(dx, dx, S(dy * dy)))));
    const f64 ny = S(dy * inv), nx = S(dx * inv), nz = S(dz * inv);
    const u32 slot = global<be_u32>(kWorld) + 4208;
    const f64 D = lf(global<be_f32>(kAimDist)), H = lf(global<be_f32>(kAimLift));
    auto aim = [&](Vec4& to) {
        const f64 y = Fmadds(ny, D, oy);
        to.x = static_cast<f32>(Fmadds(nx, D, ox));
        to.y = static_cast<f32>(y);
        to.z = static_cast<f32>(Fmadds(nz, D, oz));
        to.y = static_cast<f32>(S(y + H));
    };
    if (msg == 0x952A45ACu || msg == 0x0969AF51u) {
        Vec4& to = msg == 0x952A45ACu ? L.to0 : L.to1;
        aim(to);
        const u32 r = Spawn(o, def, &L.m, &to, kZeroVec, Pick(o, 0x0106AE0Cu, 0x0106AE58u, 0x0106B06Cu, 0x0106B0B8u));
        if (msg == 0x952A45ACu || r == 0) return 0;
        call(Obj_SetFocus, r, LocalPlayer());
        return r;
    }
    if (msg == 0x952A45ADu) {
        const f64 y = S(Fmadds(ny, D, oy) + H);
        const f64 x = Fmadds(nx, D, ox), z = Fmadds(nz, D, oz);
        const f64 zero = C(4), w = Tw(kTwSpread);
        const f64 a = Fmadds(one, w, x), b = Fmadds(zero, w, z), c = Fmadds(-one, w, x);
        const f64 d = Fmadds(-zero, w, z), e = Fmadds(zero, w, y), f = Fmadds(-zero, w, y);
        L.pair[0].x = static_cast<f32>(a);
        L.pair[0].z = static_cast<f32>(b);
        L.pair[1].x = static_cast<f32>(c);
        L.pair[0].y = static_cast<f32>(e);
        L.pair[1].y = static_cast<f32>(f);
        L.pair[1].z = static_cast<f32>(d);
        const Tw2 t = Pick(o, 0x0106AEA4u, 0x0106AEF0u, 0x0106B104u, 0x0106B150u);  // read once for both
        Spawn(o, def, &L.m, &L.pair[0], kZeroVec, t);
        Spawn(o, def, &L.m, &L.pair[1], kZeroVec, t);
        return 0;
    }
    if (msg == 0x952A45AEu) {
        Vec4& to = L.to3;
        Vec4& out = L.out3;
        aim(to);
        const bool first = o->phase == 1;
        const f64 v = Tw(first ? 0x0106AF3Cu : 0x0106B19Cu), w = Tw(first ? 0x0106AF88u : 0x0106B1E8u);
        const u32 pl = PlayerOf(gm<be_u32>(slot + 8));
        call_ptr(vfn(&gm<u8>(pl), 140), &out, pl);
        set_bits(out.y, 0u);
        const f64 zero = C(4);
        const u32 opd = vfn(o, 100);  // o's vfn[25] taken before the push
        if (const u32 h = o->helper; h != 0 && (gm<be_u32>(h + 20) & 0x10000u) != 0) {
            const f64 k = lf(gm<be_f32>(h + 412));
            const f64 px = Fmadds(lf(out.x), k, lf(to.x));
            const f64 py = Fmadds(zero, k, lf(to.y));
            const f64 pz = Fmadds(lf(out.z), k, lf(to.z));
            to.x = static_cast<f32>(px);
            to.y = static_cast<f32>(py);
            to.z = static_cast<f32>(pz);
        }
        f64 x = lf(to.x);
        x = S(Fsel(S(C(8) - x), x, Cd(16)));
        x = S(Fsel(S(x - C(24)), x, Cd(32)));
        const f64 z0 = lf(to.z);
        to.x = static_cast<f32>(x);
        f64 z = S(Fsel(S(C(40) - z0), z0, Cd(48)));
        z = S(Fsel(S(z - C(56)), z, Cd(64)));
        to.z = static_cast<f32>(z);
        const u32 r10 = call_ptr<u32>(opd, o);
        call(Obj_SpawnAt, o, def, &L.m, &to, &out, v, w, r10);
        return 0;
    }
    return 0;
}

}  // namespace sly4::obj

NATIVE(002FBF00, sly4::obj::Dragon_AttackSpawn2FBF00)
