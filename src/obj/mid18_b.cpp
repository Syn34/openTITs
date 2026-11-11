// Game code, medium sweep part 18: spline frames, spawns, ballistic and
// pose helpers.
#include "mid18.h"

using namespace sly4;
using namespace sly4::mid18;

namespace {
constexpr u32 kUp = 0x01164B60u;  // global up vector (Vec3)

// The frame at m (rows m, m+16, m+32, pos m+48 written by the caller):
// row0 = normalize((up x d).x, c0, (up x d).z) with 1/len = c1 / sqrt(...),
// row1 = d x row0 (as d.y*Z - d.z*Y, d.z*X - d.x*Z, d.x*Y - d.y*X), row2 = d
// (already at m + 32).
inline void SideFrame(u32 m, f64 dx, f64 dy, f64 dz, f64 c0, f64 c1) {
    const f64 ux = Lf(kUp), uy = Lf(kUp + 4), uz = Lf(kUp + 8);
    const f64 x = Fmsubs(uy, dz, S(uz * dy));
    const f64 z = Fmsubs(ux, dy, S(uy * dx));
    const f64 inv = Divs(c1, Sqrts(Fmadds(z, z, Fmadds(x, x, S(c0 * c0)))));
    const f64 X = S(x * inv), Y = S(c0 * inv), Z = S(z * inv);
    Sf(m, X);
    Sf(m + 4, Y);
    Sf(m + 8, Z);
    Sf(m + 16, Fmsubs(dy, Z, S(dz * Y)));
    Sf(m + 20, Fmsubs(dz, X, S(dx * Z)));
    Sf(m + 24, Fmsubs(dx, Y, S(dy * X)));
}
}  // namespace

// Obj_PlaceOnSpline3DAED0 (Obj* o) (called by 003DB140): with the spline
// o->80 (r3 = 0 when null), the point / tangent at o->104 and at o->112
// (WorldObj_SplinePointTangentAt(o->80, -, &pos, &tan, 0; t)) give two frames
// (SideFrame with c = 0x003DAEC8: 0, 1): the first with the reversed tangent
// goes to o->64 vfn[84](&m), the second with the tangent to o->68
// vfn[84](&m). r3 / f1 as the last virtual call leaves them.
DECOMP(003DAED0, Obj_PlaceOnSpline3DAED0) {
    const u32 o = arg<u32>(ctx, 0);
    if (load<u32>(o + 80) == 0) {
        ret(ctx, 0);
        return;
    }
    Stack frame(ctx, 288);
    const u32 sp = frame.addr() - 112;  // the original's r1: locals at their original offsets
    constexpr u32 C = 0x003DAEC8u;
    const f64 c0 = Lf(C), c1 = Lf(C + 4);
    // r4 is left over (the caller's; the callee does not read it)
    callf(ctx, 0x005A80D8u, {load<u32>(o + 80), ctx.gpr[4], sp + 224, sp + 208, 0}, {Lf(o + 104)});  // WorldObj_SplinePointTangentAt
    const u32 m1 = sp + 176;
    const f64 tx = -Lf(sp + 208), ty = -Lf(sp + 212), tz = -Lf(sp + 216);
    Sf(sp + 208, tx);
    Sf(sp + 212, ty);
    Sf(sp + 216, tz);
    SideFrame(m1, tx, ty, tz, c0, c1);
    ctx.fpr[1] = Lf(m1 + 24);  // f1 as the original leaves it
    const u32 a = load<u32>(o + 64);
    VCall(ctx, a, 84, {a, m1});
    // r4 is left over from the virtual call (the callee does not read it)
    callf(ctx, 0x005A80D8u, {load<u32>(o + 80), ctx.gpr[4], sp + 160, sp + 144, 0}, {Lf(o + 112)});  // WorldObj_SplinePointTangentAt
    const u32 m2 = sp + 112;
    SideFrame(m2, Lf(sp + 144), Lf(sp + 148), Lf(sp + 152), c0, c1);
    ctx.fpr[1] = Lf(m2 + 20);  // f1 as the original leaves it
    const u32 b = load<u32>(o + 68);
    VCall(ctx, b, 84, {b, m2});
}

// Race_RankBySpline2B618 (Obj* o, List* l) (called by VtB0E428_Dtor /
// _Delete, Obj_OnEvent_0002BDB8): one bubble pass over the l->24 entries
// l->20[]: an active entry (non-null, flag 0x200 in +192) with +348 > c (c =
// *(f32*)0x0002B610) and a racer r = +244 gets r->332 = SplineInstance_Nearest(
// o->4, e + 144, 0, 0) (when o->4), and swaps with an active predecessor whose
// racer's +332 is greater (the predecessor's racer is not null-checked, quirk
// kept). Then each active entry with +348 > c in order counts a rank; its
// racer (if any) gets vfn[300](; f1 = rank). r3 / f1 as the original leaves
// them.
DECOMP(0002B618, Race_RankBySpline2B618) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 l = arg<u32>(ctx, 1);
    const u32 n = load<u32>(l + 24);
    const u32 d = load<u32>(l + 20);
    const f64 c = Lf(0x0002B610u);
    f64 f1 = ctx.fpr[1];
    u64 r3 = o;
    auto active = [&](u32 e) { return e != 0 && (load<u32>(e + 192) & 0x200u) != 0; };
    for (u32 i = 0; i < n; ++i) {
        const u32 e = load<u32>(d + 4 * i);
        const u32 cur = active(e) ? e : 0;
        u32 prev = 0;
        if (i != 0) {
            const u32 p = load<u32>(d + 4 * i - 4);
            if (active(p)) prev = p;
        }
        if (cur == 0) continue;
        f1 = Lf(cur + 348);
        if (!(f1 > c)) continue;
        const u32 r = load<u32>(cur + 244);
        if (r == 0) continue;
        const u32 s = load<u32>(o + 4);
        if (s != 0) {
            ctx.fpr[1] = f1;
            f1 = callf(ctx, 0x005927FCu, {s, cur + 144, 0, 0}, {});  // SplineInstance_Nearest
            Sf(r + 332, f1);
        }
        if (prev == 0) continue;
        const u32 pr = load<u32>(prev + 244);
        f1 = Lf(r + 332);
        if (Lf(pr + 332) > f1) {
            store<u32>(d + 4 * i - 4, cur);
            store<u32>(d + 4 * i, prev);
        }
    }
    s64 rank = 0;
    for (u32 i = 0; i < n; ++i) {
        const u32 e = load<u32>(d + 4 * i);
        r3 = e;
        if (!active(e)) continue;
        f1 = Lf(e + 348);
        if (!(f1 > c)) continue;
        const u32 r = load<u32>(e + 244);
        r3 = r;
        if (r != 0) {
            ctx.fpr[1] = S(static_cast<f64>(rank));
            VCall(ctx, r, 300, {r});
            r3 = ctx.gpr[3];
            f1 = ctx.fpr[1];
        }
        ++rank;
    }
    ctx.fpr[1] = f1;
    ctx.gpr[3] = r3;
}

// Mover_FollowSpline2D9B8 (Mover* o) (called by 0002E130): with the driver
// d = o->48 and its track t = d->220 (null: o vfn[220](0x7BF02FB1) instead),
// s = t->80 and its spline w = s->4 (r3 = t when either is null): c =
// 0x0002D988 (1, 2, 0.151, 0.849, 0.3, 0.2, 0.1, 0, 0.001, 10, 0.5). The
// parameter u = (o->316 + 1) / (k + 2) (k = t->24), or, when s->8 is set,
// band(k) * (u - 0.5) + clamp(s->12, 0.151, 0.849) (band 0.3 / 0.2 / 0.1 / 0
// for k >= 8 / < 8 / < 5 / < 2); the point WorldObj_SplinePointAt(w, w, &p,
// 0; u). Farther than 0.001 in xz from d +144: d->484 = atan2f(dx, dz)
// (normalized) and d->548 = 1, the distance when below 1, or 0.3 beyond 10
// (only when d has flag 0x200 in +192). r3 / f1 as the original leaves them.
DECOMP(0002D9B8, Mover_FollowSpline2D9B8) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 t = load<u32>(load<u32>(o + 48) + 220);
    if (t == 0) {
        VCall(ctx, o, 220, {o, 0x7BF02FB1u});
        return;
    }
    const u32 s = load<u32>(t + 80);
    const u32 w = s != 0 ? load<u32>(s + 4) : 0;
    if (w == 0) {
        ret(ctx, t);
        return;
    }
    constexpr u32 C = 0x0002D988u;
    const s32 k = static_cast<s32>(load<u32>(t + 24));
    const f64 c0 = Lf(C);
    f64 u = Divs(S(Lf(o + 316) + c0), S(S(static_cast<f64>(static_cast<s64>(k))) + Lf(C + 4)));
    f64 lo = Lf(s + 12);
    if (lo < Lf(C + 8)) lo = Lf(C + 8);
    if (lo > Lf(C + 12)) lo = Lf(C + 12);
    u32 band = C + 16;
    if (k < 8) band = C + 20;
    if (k < 5) band = C + 24;
    if (k < 2) band = C + 28;
    if (load<u8>(s + 8) != 0) u = Fmadds(Lf(band), S(u - Lf(C + 40)), lo);
    Stack frame(ctx, 176);
    const u32 pt = frame.addr();  // the original's r1 + 112
    callf(ctx, 0x00592748u, {w, w, pt, 0}, {u});  // WorldObj_SplinePointAt
    const u32 p = load<u32>(o + 48) + 144;
    const f64 dx = S(Lf(pt) - Lf(p)), dz = S(Lf(pt + 8) - Lf(p + 8));
    const f64 c7 = Lf(C + 28);
    const f64 dist = Sqrts(Fmadds(dz, dz, Fmadds(dx, dx, S(c7 * c7))));
    if (!(dist > Lf(C + 32))) {
        ctx.fpr[1] = dz;
        ret(ctx, C);
        return;
    }
    const f64 inv = Divs(c0, dist);
    const f64 ang = callf(ctx, 0x005E665Cu, {}, {S(dx * inv), S(dz * inv)});  // atan2f
    const u32 d = load<u32>(o + 48);
    if (d == 0) return;
    if ((load<u32>(d + 192) & 0x200u) == 0) {
        ret(ctx, 0);
        return;
    }
    Sf(d + 484, ang);
    store<u32>(d + 548, 0x3F800000u);
    if (dist < c0) Sf(d + 548, dist);
    ctx.fpr[1] = Lf(C + 36);
    if (dist > Lf(C + 36)) Cw(d + 548, C + 16);
    ret(ctx, C);
}

// Cam_FollowSpline55E10 (CamCtl* o; f32 dt) (called by 00056468): with the
// game g = *0x00F65CA0 holding a spline track (g +0x1281E0, else r3 = g +
// 4208) and o's camera o->6228 and spline instance o->6236 set (else r3 = 0):
// the reference point is the view g +0x128168 ->816's position pushed along
// its +32 row by T(0x00F9AA50) when the byte T(0x00F9AA04) is set, else the
// position (+144) of the player g->4216 when it has flag 2 in +192 (a null /
// flagless player reads +144 of address 0, quirk kept). Its nearest spline
// parameter, clamped to [o->6248, o->6252], is approached by o->6232 at most
// T(0x00F9AA9C) * dt / o->6236->208->120 per call (or taken at once when the
// byte o->6256 is set, which is cleared); the point / tangent there make a
// frame (forward = normalize(tan.x, c[0], tan.z), c = 0x00055E04, up =
// 0x01164B60, side = up x forward) for o->6228 vfn[84](&m). T(a) = the value
// at a + 72.
DECOMP(00055E10, Cam_FollowSpline55E10) {
    const u32 o = arg<u32>(ctx, 0);
    const f64 dt = ctx.fpr[1];
    const u32 g = load<u32>(0x00F65CA0u);
    const u32 trk = load<u32>(g + 0x1281E0u);
    const u32 pl = load<u32>(g + 4216);
    if (trk == 0) {
        ret(ctx, g + 4208);
        return;
    }
    const u32 cam = load<u32>(o + 6228);
    const u32 si = cam != 0 ? load<u32>(o + 6236) : 0;
    if (si == 0) {
        ret(ctx, 0);
        return;
    }
    Stack frame(ctx, 224);
    const u32 sp = frame.addr() - 112;  // the original's r1: locals at their original offsets
    const u32 q = sp + 112, m = sp + 128, pos = sp + 176, tan = sp + 160;
    const u32 v = load<u32>(g + 0x128168u) + 816;
    if (load<u8>(0x00F9AA04u + 72) != 0) {
        const f64 k = Lf(0x00F9AA50u + 72);
        Sf(q, Fmadds(Lf(v + 32), k, Lf(v + 48)));
        Sf(q + 4, Fmadds(Lf(v + 36), k, Lf(v + 52)));
        Sf(q + 8, Fmadds(Lf(v + 40), k, Lf(v + 56)));
    } else {
        u32 a = 0;
        if (pl != 0 && (load<u32>(pl + 192) & 2u) != 0) a = pl;
        CopyVec3(q, a + 144);
    }
    const f64 t = callf(ctx, 0x005927FCu, {si, q, 0, 0}, {});  // SplineInstance_Nearest
    const u32 s2 = load<u32>(o + 6236);
    const f64 step = Divs(S(Lf(0x00F9AA9Cu + 72) * dt), Lf(load<u32>(s2 + 208) + 120));
    const f64 hi = Lf(o + 6252), lo = Lf(o + 6248);
    f64 cur = Lf(o + 6232);
    f64 tg = S(hi - t) >= 0.0 ? t : hi;
    tg = S(tg - lo) >= 0.0 ? tg : lo;
    const f64 dl = S(tg - cur);
    f64 d = S(step - dl) >= 0.0 ? dl : step;
    d = S(d - -step) >= 0.0 ? d : -step;
    cur = S(cur + d);
    Sf(o + 6232, cur);
    const u32 snap = load<u8>(o + 6256);
    if (snap != 0) {
        Sf(o + 6232, tg);
        cur = tg;
        store<u8>(o + 6256, 0);
    }
    // r4 is left over (the byte o->6256; the callee does not read it)
    callf(ctx, 0x005A80D8u, {s2, snap, pos, tan, 0}, {cur});  // WorldObj_SplinePointTangentAt
    constexpr u32 C = 0x00055E04u;
    const f64 c0 = Lf(C);
    const f64 tx = Lf(tan), tz = Lf(tan + 8);
    const f64 inv = Divs(Lf(C + 4), Sqrts(Fmadds(tz, tz, Fmadds(tx, tx, S(c0 * c0)))));
    const f64 X = S(tx * inv), Y = S(c0 * inv), Z = S(tz * inv);
    Sf(tan, X);
    Sf(tan + 4, Y);
    Sf(tan + 8, Z);
    constexpr u32 U = 0x01164B60u;
    const f64 ux = Lf(U), uy = Lf(U + 4), uz = Lf(U + 8);
    CopyVec3(m + 16, U);
    Sf(m + 4, Fmsubs(uz, X, S(ux * Z)));
    Sf(m + 8, Fmsubs(ux, Y, S(uy * X)));
    Sf(m, Fmsubs(uy, Z, S(uz * Y)));
    ctx.fpr[1] = Lf(m + 4);  // f1 as the original leaves it
    const u32 c = load<u32>(o + 6228);
    VCall(ctx, c, 84, {c, m});
}

// Obj_TrackSpline29A598 (Obj* o) (called by 0029A978): with a spline
// instance o->2180 (else o->2200 = 0, r3 = 0) and o->2196 <= 0 (else r3 =
// 0x0029A58C, f1 = o->2196): the linked o->2176 gets 002EF4E0(*0x00F64C38,
// o->2176->160) and Effect_SetHash4CF988(o->2176 vfn[272](), 0x5B757FD2) when
// that is set; the target q = o->8 +144 (or o->8->160 +240) has its nearest
// parameter o->2184 and o->2240 = q; the spline point / tangent there (both
// seeded with 0x01164B40) are stored (o->2208 pos, o->2224 tan, o->2240 = q
// again, o->2204 top bits cleared) when farther than c[1] (squared) from q,
// else o->2204 top bits = 01, o->2188 = T(0x010579B8) / o->2180->208->120,
// o->2192 = T(0x01057A04). Then o->2200 = 1.0, o->2196 = T(0x01057A50) (r3 =
// 0x01057A50). T(a) = the value at a + 72.
DECOMP(0029A598, Obj_TrackSpline29A598) {
    const u32 o = arg<u32>(ctx, 0);
    if (load<u32>(o + 2180) == 0) {
        store<u32>(o + 2200, 0);
        ret(ctx, 0);
        return;
    }
    constexpr u32 C = 0x0029A58Cu;
    const f64 w = Lf(o + 2196);
    if (!(w <= Lf(C))) {
        ctx.fpr[1] = w;
        ret(ctx, C);
        return;
    }
    const u32 a = load<u32>(o + 2176);
    if (a != 0) {
        call(ctx, 0x002EF4E0u, {load<u32>(0x00F64C38u), load<u32>(a + 160)});
        const u32 b = load<u32>(o + 2176);
        if (static_cast<s32>(VCall(ctx, b, 272, {b})) != 0) {
            const u32 c = load<u32>(o + 2176);
            const u32 e = static_cast<u32>(VCall(ctx, c, 272, {c}));
            call(ctx, 0x004CF988u, {e, 0x5B757FD2u});  // Effect_SetHash4CF988
        }
    }
    Stack frame(ctx, 176);
    const u32 sp = frame.addr() - 112;  // the original's r1: locals at their original offsets
    const u32 q = sp + 112, tan = sp + 128, pos = sp + 144;
    const u32 tg = load<u32>(o + 8);
    CopyVec3(q, tg + 144);
    const u32 sub = load<u32>(tg + 160);
    if (sub != 0) CopyVec3(q, sub + 240);
    const f64 t = callf(ctx, 0x005927FCu, {load<u32>(o + 2180), q, 0, 0}, {});  // SplineInstance_Nearest
    Sf(o + 2184, t);
    CopyVec3(o + 2240, q);
    constexpr u32 kSeed = 0x01164B40u;
    CopyVec3(tan, kSeed);
    CopyVec3(pos, kSeed);
    // r4 is left over (0x01164B40; the callee does not read it)
    callf(ctx, 0x005A80D8u, {load<u32>(o + 2180), kSeed, pos, tan, 0}, {t});  // WorldObj_SplinePointTangentAt
    const f64 dy = S(Lf(pos + 4) - Lf(q + 4)), dx = S(Lf(pos) - Lf(q)), dz = S(Lf(pos + 8) - Lf(q + 8));
    if (Fmadds(dz, dz, Fmadds(dx, dx, S(dy * dy))) > Lf(C + 4)) {
        Cw(o + 2208, pos);
        const u32 f = load<u32>(o + 2204) & 0x3FFFFFFFu;
        Cw(o + 2212, pos + 4);
        Cw(o + 2216, pos + 8);
        store<u32>(o + 2204, f);
        CopyVec3(o + 2224, tan);
        CopyVec3(o + 2240, q);
    } else {
        store<u32>(o + 2204, (load<u32>(o + 2204) & 0x3FFFFFFFu) | 0x40000000u);
        const u32 s = load<u32>(o + 2180);
        Sf(o + 2188, Divs(Lf(0x010579B8u + 72), Lf(load<u32>(s + 208) + 120)));
        Cw(o + 2192, 0x01057A04u + 72);
    }
    store<u32>(o + 2200, 0x3F800000u);
    Cw(o + 2196, 0x01057A50u + 72);
    ctx.fpr[1] = Lf(0x01057A50u + 72);
    ret(ctx, 0x01057A50u);
}

namespace {
constexpr u32 kIdent33 = 0x01164BD0u;  // global identity rotation (rows of 16 bytes)

// Copies the 3x3 rotation rows (16 bytes apart) at s to d (raw words).
inline void CopyRot(u32 d, u32 s) {
    for (u32 r = 0; r < 3; ++r) CopyVec3(d + 16 * r, s + 16 * r);
}

// The shared spawn: s = a vfn[100](); e = 000EDE30(b, a, &rot, &dir, -, s,
// 0, 0, 1; f1) with rot = identity at sp + 144 (pos sp + 192 already
// written). r7 is left over from the virtual call (the callee does not read
// it). Returns e.
inline u32 SpawnAt(PPUContext& ctx, u32 a, u32 b, u32 sp, f64 f1, f64 f1call) {
    CopyRot(sp + 144, kIdent33);
    ctx.fpr[1] = f1call;  // f1 as the original leaves it
    const u32 s = static_cast<u32>(VCall(ctx, a, 100, {a}));
    callf(ctx, 0x000EDE30u, {b, a, sp + 144, sp + 128, ctx.gpr[7], s, 0, 0, 1}, {f1});
    return static_cast<u32>(ctx.gpr[3]);
}
}  // namespace

// Obj_SpawnToward307F70 (Obj* a, Def* b, const Vec3* from, const Vec3* to)
// (called by 003081C8): spawns at from heading normalize(to - from) (c[0] /
// len, c = 0x00307F6C; f1 = b->220) via SpawnAt; the spawn gets the identity
// rotation at to through Obj_SetMatrix_000EDD74 and +512 = from. r3 = the
// spawn (or 0).
DECOMP(00307F70, Obj_SpawnToward307F70) {
    const u32 a = arg<u32>(ctx, 0);
    const u32 b = arg<u32>(ctx, 1);
    const u32 from = arg<u32>(ctx, 2);
    const u32 to = arg<u32>(ctx, 3);
    Stack frame(ctx, 320);
    const u32 sp = frame.addr() - 112;  // the original's r1: locals at their original offsets
    CopyVec3(sp + 192, from);
    const f64 fx = Lf(from), fy = Lf(from + 4), fz = Lf(from + 8);
    const f64 dy = S(Lf(to + 4) - fy), dx = S(Lf(to) - fx), dz = S(Lf(to + 8) - fz);
    const f64 inv = Divs(Lf(0x00307F6Cu), Sqrts(Fmadds(dz, dz, Fmadds(dx, dx, S(dy * dy)))));
    const f64 X = S(dx * inv);
    Sf(sp + 128, X);
    Sf(sp + 132, S(dy * inv));
    Sf(sp + 136, S(dz * inv));
    const u32 e = SpawnAt(ctx, a, b, sp, Lf(b + 220), X);
    if (e == 0) {
        ret(ctx, 0);
        return;
    }
    const u32 m = sp + 208;
    CopyRot(m, kIdent33);
    CopyVec3(m + 48, to);
    ctx.fpr[1] = Lf(to + 8);  // f1 as the original leaves it
    call(ctx, 0x000EDD74u, {e, m});  // Obj_SetMatrix_000EDD74
    CopyVec3(e + 512, from);
    ctx.fpr[1] = Lf(from + 8);
    ret(ctx, e);
}

// Obj_SpawnAt2FBCC8 (Obj* a, Def* b, const Mat34* m, const Vec3* to; f32 v)
// (called by 002FBF00): spawns at m's position (+48) with the direction
// 0x01164B40 via SpawnAt (f1 = v); the spawn gets the identity rotation at
// to (read on entry) through Obj_SetMatrix_000EDD74 and +512 = m +48. r3 =
// the spawn (or 0).
DECOMP(002FBCC8, Obj_SpawnAt2FBCC8) {
    const u32 a = arg<u32>(ctx, 0);
    const u32 b = arg<u32>(ctx, 1);
    const u32 p = arg<u32>(ctx, 2) + 48;
    const u32 to = arg<u32>(ctx, 3);
    const f64 v = ctx.fpr[1];
    const u32 tx = load<u32>(to), ty = load<u32>(to + 4), tz = load<u32>(to + 8);
    Stack frame(ctx, 336);
    const u32 sp = frame.addr() - 112;  // the original's r1: locals at their original offsets
    CopyVec3(sp + 192, p);
    CopyVec3(sp + 128, 0x01164B40u);
    const u32 e = SpawnAt(ctx, a, b, sp, v, Lf(0x01164B48u));
    if (e == 0) {
        ret(ctx, 0);
        return;
    }
    const u32 m = sp + 208;
    CopyRot(m, kIdent33);
    store<u32>(m + 48, tx);
    store<u32>(m + 52, ty);
    store<u32>(m + 56, tz);
    ctx.fpr[1] = Lf(kIdent33 + 40);  // f1 as the original leaves it
    call(ctx, 0x000EDD74u, {e, m});  // Obj_SetMatrix_000EDD74
    CopyVec3(e + 512, p);
    ctx.fpr[1] = Lf(p + 8);
    ret(ctx, e);
}

// Obj_FaceToward24670 (Obj* o, const Vec3* p) (called by 000248D0,
// 000262C8): o->24 = 0, o vfn[220](o->8); the direction o->192 = p - o->176
// (its y zeroed when o->305 has 0x40 but not 0x20, or neither and o
// vfn[132]() is 0) normalized (c[0] / len, c = 0x00024664); the frame side =
// up x fwd (up = 0x01164B60), up, fwd and pos = p + up * c[1] - fwd goes to
// o->128.. and to o->48 vfn[84](&m); Obj_ResetPos_0026D9D4(o->68), o->112 =
// p, then r3 = o vfn[248]().
DECOMP(00024670, Obj_FaceToward24670) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 p = arg<u32>(ctx, 1);
    store<u32>(o + 24, 0);
    {
        // o vfn[220](o->8), with the entry in r5 and the OPD in r6 (the original's order)
        const u32 opd = load<u32>(load<u32>(o) + 220);
        call_ptr(ctx, opd, {o, load<u32>(o + 8), load<u32>(opd), opd});
    }
    const f64 dx = S(Lf(p) - Lf(o + 176)), dy = S(Lf(p + 4) - Lf(o + 180)), dz = S(Lf(p + 8) - Lf(o + 184));
    const u32 fl = load<u8>(o + 305);
    Sf(o + 192, dx);
    Sf(o + 196, dy);
    Sf(o + 200, dz);
    if ((fl & 0x20u) == 0) {
        bool flat = (fl & 0x40u) != 0;
        if (!flat) {
            ctx.fpr[1] = dx;  // f1 as the original leaves it
            flat = static_cast<s32>(VCall(ctx, o, 132, {o})) == 0;
        }
        if (flat) store<u32>(o + 196, 0);
    }
    const f64 y = Lf(o + 196), x = Lf(o + 192), z = Lf(o + 200);
    constexpr u32 C = 0x00024664u;
    const f64 inv = Divs(Lf(C), Sqrts(Fmadds(z, z, Fmadds(x, x, S(y * y)))));
    const f64 X = S(x * inv), Y = S(y * inv), Z = S(z * inv);
    Sf(o + 192, X);
    Sf(o + 196, Y);
    Sf(o + 200, Z);
    const f64 k = Lf(C + 4);
    constexpr u32 U = 0x01164B60u;
    const f64 uy = Lf(U + 4), uz = Lf(U + 8), ux = Lf(U);
    const f64 px = S(Fmadds(ux, k, Lf(p)) - X), py = S(Fmadds(uy, k, Lf(p + 4)) - Y), pz = S(Fmadds(uz, k, Lf(p + 8)) - Z);
    const f64 sx = Fmsubs(uy, Z, S(uz * Y)), sy = Fmsubs(uz, X, S(ux * Z)), sz = Fmsubs(ux, Y, S(uy * X));
    Stack frame(ctx, 192);
    const u32 m = frame.addr();  // the original's r1 + 112
    const f64 rows[12] = {sx, sy, sz, 0, 0, 0, X, Y, Z, px, py, pz};
    auto put = [&](u32 base) {  // the up row as raw words (lfs / stfs)
        for (u32 i = 0; i < 12; ++i) {
            if (i >= 3 && i < 6) Cw(base + MOff16(i), U + 4 * (i - 3));
            else Sf(base + MOff16(i), rows[i]);
        }
    };
    put(m);
    put(o + 128);
    const u32 w = load<u32>(o + 48);
    ctx.fpr[1] = Y;  // f1 as the original leaves it
    VCall(ctx, w, 84, {w, m});
    call(ctx, 0x0026D9D4u, {load<u32>(o + 68)});  // Obj_ResetPos_0026D9D4
    CopyVec3(o + 112, p);
    ctx.fpr[1] = Lf(p + 8);  // f1 as the original leaves it
    VCall(ctx, o, 248, {o});
}

// Obj_CheckPlayerNear542C0 (Obj* o) (called by 00056468): with the player
// pl = *(0x00F65CA0) ->4216 that has flag 2 in +192 (else r3 = g + 4208 or
// 0): a player below y = 42.0 (the double at 0x000542B8) is lifted to y =
// 42.3 (Mat34_FromRotPos553B4C of its rotation rows into pl vfn[84](&m));
// then, for o->4 in 7..8 or 13..17, when the player is within T(0x00F9AFA8)
// of o +176 in xz (y term c = *(f32*)0x000542BC), o->312 counts down (to 0)
// and o vfn[220](9). T(a) = the value at a + 72. r3 / f1 as the original
// leaves them.
DECOMP(000542C0, Obj_CheckPlayerNear542C0) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 g = load<u32>(0x00F65CA0u);
    const u32 pl = load<u32>(g + 4216);
    if (pl == 0) {
        ret(ctx, g + 4208);
        return;
    }
    if ((load<u32>(pl + 192) & 2u) == 0) {
        ret(ctx, 0);
        return;
    }
    Stack frame(ctx, 272);
    const u32 sp = frame.addr() - 112;  // the original's r1: locals at their original offsets
    const u32 q = sp + 112;
    CopyVec3(q, pl + 144);
    constexpr u32 C = 0x000542B8u;
    if (Lf(q + 4) < Ld(C)) {
        store<u32>(q + 4, 0x42293333u);
        const u32 rot = sp + 128, m = sp + 176;
        CopyRot(rot, pl + 96);
        ctx.fpr[1] = Lf(pl + 136);  // f1 as the original leaves it
        call(ctx, 0x00553B4Cu, {m, rot, q});  // Mat34_FromRotPos553B4C
        VCall(ctx, pl, 84, {pl, m});
    }
    const f64 x = Lf(q), z = Lf(q + 8);
    ctx.fpr[1] = z;
    const s32 k = static_cast<s32>(load<u32>(o + 4));
    if (!((k > 6 && k <= 8) || (k > 12 && k <= 17))) {
        ret(ctx, static_cast<u32>(k));
        return;
    }
    const f64 c = Lf(C + 4);
    const f64 dx = S(x - Lf(o + 176)), dz = S(z - Lf(o + 184));
    const f64 d = Sqrts(Fmadds(dz, dz, Fmadds(dx, dx, S(c * c))));
    ctx.fpr[1] = d;
    if (!(d < Lf(0x00F9AFA8u + 72))) {
        ret(ctx, 0x00F9AFA8u);
        return;
    }
    const s32 n = static_cast<s32>(load<u32>(o + 312));
    if (n > 0) store<u32>(o + 312, static_cast<u32>(n - 1));
    VCall(ctx, o, 220, {o, 9});
}

// Obj_FallTick3CB618 (Obj* o; f32 dt) (called by 003CB968): ballistic step
// with c = 0x003CB600 (a.x = a.z = c[0], a.y = c[1], c[2] = 1/2): p (o->32) +=
// v dt + c[2] a dt^2, v (o->48) += a dt. Landing (p.y < c[3] and v.y <
// c[0]): p.y = 1.0, v.x = 0, v.y *= c[4] with Effect_Spawn(0x1BE27372, 0, 0,
// 1), o->0 = 4. Then the body o->108 gets Mat34_FromRotPos553B4C(its
// rotation rows, &o->32) through vfn[84].
DECOMP(003CB618, Obj_FallTick3CB618) {
    const u32 o = arg<u32>(ctx, 0);
    const f64 dt = ctx.fpr[1];
    constexpr u32 C = 0x003CB600u;
    const f64 k0 = Lf(C), k1 = Lf(C + 4), k2 = Lf(C + 8);
    const f64 vx = Lf(o + 48), vy = Lf(o + 52), vz = Lf(o + 56);
    const f64 px = Lf(o + 32), py = Lf(o + 36), pz = Lf(o + 40);
    const f64 hx = S(S(k0 * k2) * dt), hy = S(S(k1 * k2) * dt);
    Sf(o + 48, Fmadds(k0, dt, vx));
    Sf(o + 52, Fmadds(k1, dt, vy));
    Sf(o + 56, Fmadds(k0, dt, vz));
    Sf(o + 32, Fmadds(hx, dt, Fmadds(vx, dt, px)));
    Sf(o + 36, Fmadds(hy, dt, Fmadds(vy, dt, py)));
    Sf(o + 40, Fmadds(hx, dt, Fmadds(vz, dt, pz)));
    if (Lf(o + 36) < Lf(C + 12) && Lf(o + 52) < k0) {
        store<u32>(o + 36, 0x3F800000u);
        store<u32>(o + 48, 0);
        const f64 y = Lf(o + 52);
        if (y < k0) {
            const f64 ny = S(y * Lf(C + 16));
            Sf(o + 52, ny);
            ctx.fpr[1] = ny;  // f1 as the original leaves it
            call(ctx, 0x004A5FC8u, {0x1BE27372u, 0, 0, 1});  // Effect_Spawn
        }
        store<u32>(o, 4);
    }
    const u32 b = load<u32>(o + 108);
    Stack frame(ctx, 256);
    const u32 sp = frame.addr() - 112;  // the original's r1: locals at their original offsets
    const u32 rot = sp + 112, m = sp + 160;
    CopyRot(rot, b + 96);
    ctx.fpr[1] = Lf(b + 136);  // f1 as the original leaves it
    call(ctx, 0x00553B4Cu, {m, rot, o + 32});  // Mat34_FromRotPos553B4C
    VCall(ctx, b, 84, {b, m});
}

// Obj_ResetPath1D0F68 (Obj* o) (called by 001D1170, 001D2370,
// Obj_SetLink_001D3968): copies the rest pose rows o->576.. to o->640..,
// clears o->888..904 (+892), seeds o->912 / o->928 with the globals
// 0x01164B40 / 0x01164B70, o->944 = 1.0, o->972 bit 0x40000000 = its bit
// 0x80000000; drops the target o->820 (its +1268 bit 0x40000000 cleared,
// Obj_SetTarget_00296758(it, 0, 0)), Obj_SelectColor_00289DC4(o->816, 0) when
// set, o->832 / 848 / 864 = 0x01164B40, o->880 / 884 = 0; with a spline
// o->704: o->900 = the record o->556[o->968] (28 bytes) +8 and
// WorldObj_SplinePointTangentAt(o->704, -, o + 912, o + 928, 0; it),
// Obj_ApplyPose_001D0E58(o), 001D0450(o, 1).
DECOMP(001D0F68, Obj_ResetPath1D0F68) {
    const u32 o = arg<u32>(ctx, 0);
    constexpr u32 U = 0x01164B40u, V = 0x01164B70u;
    for (u32 i = 0; i < 12; ++i) Cw(o + 640 + MOff16(i), o + 576 + MOff16(i));
    store<u32>(o + 896, 0);
    store<u32>(o + 900, 0);
    store<u32>(o + 904, 0);
    Cw(o + 912, U);
    store<u32>(o + 892, 0);
    Cw(o + 916, U + 4);
    Cw(o + 920, U + 8);
    u32 f = load<u32>(o + 972);
    Cw(o + 928, V);
    const u32 tg = load<u32>(o + 820);
    f = (f & ~0x40000000u) | ((f >> 1) & 0x40000000u);
    Cw(o + 932, V + 4);
    store<u32>(o + 944, 0x3F800000u);
    Cw(o + 936, V + 8);
    store<u32>(o + 972, f);
    store<u32>(o + 888, 0);
    ctx.fpr[1] = Lf(V + 8);  // f1 as the original leaves it
    if (tg != 0) {
        store<u32>(tg + 1268, load<u32>(tg + 1268) & ~0x40000000u);
        call(ctx, 0x00296758u, {load<u32>(o + 820), 0, 0});  // Obj_SetTarget_00296758
    }
    const u32 c = load<u32>(o + 816);
    if (c != 0) call(ctx, 0x00289DC4u, {c, 0});  // Obj_SelectColor_00289DC4
    CopyVec3(o + 832, U);
    CopyVec3(o + 848, U);
    Cw(o + 864, U);
    Cw(o + 868, U + 4);
    const u32 w = load<u32>(o + 704);
    Cw(o + 872, U + 8);
    store<u32>(o + 880, 0);
    store<u32>(o + 884, 0);
    if (w == 0) {
        ctx.fpr[1] = Lf(U + 8);
        ret(ctx, 0x01160000u);
        return;
    }
    const u32 rec = load<u32>(o + 556) + load<u32>(o + 968) * 28;
    Cw(o + 900, rec + 8);
    // r4 is left over (the record; the callee does not read it)
    callf(ctx, 0x005A80D8u, {w, rec, o + 912, o + 928, 0}, {Lf(rec + 8)});  // WorldObj_SplinePointTangentAt
    call(ctx, 0x001D0E58u, {o});  // Obj_ApplyPose_001D0E58
    call(ctx, 0x001D0450u, {o, 1});
}

// Obj_StopEffects3F260 (Obj* o) (called by 0003F4E0, 00041B78): o->596 = 0,
// o->516 top bits cleared; the held o->512 gets vfn[40](1) and is dropped;
// every non-null entry of the lists o->452[o->460] (4-byte) and
// o->492[o->500] (8-byte, first word) has its vfn[272]() effect (when set)
// switched with Effect_SetHash4CF988(.., 0x879F2BDA) and gets vfn[40](1);
// then the counts o->460 / 480 / 500 = 0. Returns 0.
DECOMP(0003F260, Obj_StopEffects3F260) {
    const u32 o = arg<u32>(ctx, 0);
    const u64 h = static_cast<u64>(static_cast<s64>(static_cast<s32>(0x879F2BDAu)));  // lis / ori: sign-extended
    const u32 f = load<u32>(o + 516);
    const u32 x = load<u32>(o + 512);
    store<u32>(o + 596, 0);
    store<u32>(o + 516, f & 0x3FFFFFFFu);
    if (x != 0) {
        VCall(ctx, x, 40, {x, 1});
        store<u32>(o + 512, 0);
    }
    auto stop = [&](u32 slot) {
        const u32 e = load<u32>(slot);
        const u32 r = static_cast<u32>(VCall(ctx, e, 272, {e}));
        if (static_cast<s32>(r) != 0) call(ctx, 0x004CF988u, {r, h});  // Effect_SetHash4CF988
    };
    for (u32 i = 0; i < load<u16>(o + 460); ++i) {
        const u32 slot = load<u32>(o + 452) + 4 * i;
        const u32 e = load<u32>(slot);
        if (e == 0) continue;
        stop(slot);
        VCall(ctx, e, 40, {e, 1});
    }
    for (u32 i = 0; i < load<u16>(o + 500); ++i) {
        const u32 slot = load<u32>(o + 492) + 8 * i;
        if (load<u32>(slot) == 0) continue;
        stop(slot);
        const u32 e = load<u32>(slot);
        VCall(ctx, e, 40, {e, 1});
    }
    store<u16>(o + 460, 0);
    store<u16>(o + 480, 0);
    store<u16>(o + 500, 0);
    ret(ctx, 0);
}

// Obj_GetPathPose12919C (Obj* o, f32* t, Mat34* m, u32* mode) -> spline
// (called by 00054880, 001139C0, 001A61BC): by o vfn[32](0): 1 with
// vfn[32](1) == 2: the record s = o->20 + o->6 (u16) gives *t = s->104, *m =
// Quat_ToMatrix(s +16, 0x01164B40) and *mode = SplineInstance_GetMode(s->96),
// r3 = s->96; 3: w = o->20 gives *t = w->56, *m = o +96 (copied) and *mode =
// w->84 (byte), r3 = w->48; otherwise r3 = 0. Null outputs are skipped. f1 as
// the original leaves it.
DECOMP(0012919C, Obj_GetPathPose12919C) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 pt = arg<u32>(ctx, 1);
    const u32 m = arg<u32>(ctx, 2);
    const u32 sel = arg<u32>(ctx, 3);
    const s32 k = static_cast<s32>(VCall(ctx, o, 32, {o, 0}));
    if (k > 1) {
        if (k != 3) {
            ret(ctx, 0);
            return;
        }
        const u32 w = load<u32>(o + 20);
        if (pt != 0) {
            Cw(pt, w + 56);
            ctx.fpr[1] = Lf(w + 56);
        }
        if (m != 0) {
            CopyMat34(m, o + 96);
            ctx.fpr[1] = Lf(o + 96 + 56);
        }
        if (sel != 0) store<u32>(sel, load<u8>(w + 84));
        ret(ctx, load<u32>(w + 48));
        return;
    }
    if (k <= 0 || static_cast<s32>(VCall(ctx, o, 32, {o, 1})) != 2) {
        ret(ctx, 0);
        return;
    }
    const u32 s = load<u32>(o + 20) + load<u16>(o + 6);
    if (pt != 0) {
        Cw(pt, s + 104);
        ctx.fpr[1] = Lf(s + 104);
    }
    if (m != 0) {
        Stack frame(ctx, 208);
        const u32 q = frame.addr();  // the original's r1 + 112
        call(ctx, 0x00553A90u, {q, s + 16, 0x01164B40u});  // Quat_ToMatrix
        CopyMat34(m, q);
        ctx.fpr[1] = Lf(q + 56);
    }
    if (sel != 0) store<u32>(sel, static_cast<u32>(call(ctx, 0x005A8BA8u, {load<u32>(s + 96)})));  // SplineInstance_GetMode
    ret(ctx, load<u32>(s + 96));
}
