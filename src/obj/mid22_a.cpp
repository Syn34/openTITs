// Game code, medium sweep part 22: the spawner child states and emitter
// parameter updates part 21 left as callees.
#include "mid22.h"

using namespace sly4;
using namespace sly4::mid22;

namespace {
constexpr u32 kFindComponent = 0x004BB4B4u;  // Effect_FindComponent(e, hash)
constexpr u32 kSetHash = 0x004CF988u;        // Effect_SetHash4CF988(fx, hash)
constexpr u32 kSplineAt = 0x00592748u;       // WorldObj_SplinePointAt(o, -, out[4], b; f1 t)
constexpr u32 kSplineTanAt = 0x005A80D8u;    // WorldObj_SplinePointTangentAt(o, -, pos[4], tan[4], back; f1 t)
constexpr u32 kSinf = 0x007345F0u;           // Math_Sinf(f1)
constexpr u32 kCosf = 0x007344E0u;           // Math_Cosf(f1)
constexpr u32 kQuatMul = 0x005556A4u;        // Quat_Multiply(out, a, b)
constexpr u32 kQuatToMatrix = 0x00553A90u;   // Quat_ToMatrix(out, q, pos)
constexpr u32 kMat34FromRotPos = 0x00553B4Cu;  // Mat34_FromRotPos553B4C(out, rows, pos)
constexpr u32 kAnimFind = 0x00298874u;       // Anim_Find298874(an, key)
constexpr u32 kAnimRestart = 0x002775A8u;    // Anim_Restart2775A8
constexpr u32 kListAnySet = 0x0042C1D4u;     // List_AnySet_0042C1D4()
constexpr u32 kObjSet180 = 0x0036EA80u;      // Obj_Set180_0036EA80(o, a)
constexpr u32 kWorld = 0x00F65CA0u;          // World* g_World
constexpr u32 kUp = 0x01164B60u;             // f32[3] world up vector
constexpr u32 kAxis = 0x01164B50u;           // f32[3] the axis before it

// The owner rows (own+96: three rows of 3 floats and the position, stride
// 16) copied into the kind-4 component p (+32..), as the landing code does.
void CopyRows(u32 p, u32 own) {
    for (u32 row = 0; row < 4; ++row)
        for (u32 k = 0; k < 3; ++k) Cw(p + 32 + 16 * row + 4 * k, own + 96 + 16 * row + 4 * k);
}

// |S(a - b)| < th for the three floats at a / b.
bool Near3(u32 a, u32 b, f64 th) {
    for (u32 k = 0; k < 3; ++k)
        if (!(std::fabs(S(Lf(a + 4 * k) - Lf(b + 4 * k))) < th)) return false;
    return true;
}
}  // namespace

// u32 Emitter_SetParams443CE0(u32 handle, Params* p): with emitters enabled
// (u8 0x010AEC60) the handle (low 16 bits a serial, high 16 the index into
// the pool 0x010B0200: s32 count +24, 272-byte records +28) must name a live
// record (bits 15..30 of the word +256 equal the serial, its top bit clear)
// and p be given, else r3 = 0. On the record's emitter e = rec + 16: flags
// p->56 bit 0x02 marks e->12 and copies the position p+0 into e+176 (e->192
// = 1) when any axis differs by the tolerance *0x00443CDC or more; then (bit
// 0x02, or e->12 with any flag) the direction p+16 into e+208 the same way
// (e->224 = 1). A target p->48 (non-zero) goes to e->156 / e->160 (p->52)
// when e->172 is set. r3 = 1; f1 as the original leaves it.
DECOMP(00443CE0, Emitter_SetParams443CE0) {
    const u32 h = arg<u32>(ctx, 0), p = arg<u32>(ctx, 1);
    constexpr u32 kPool = 0x010B0200u;
    ctx.gpr[3] = 0;
    if (load<u8>(0x010AEC60u) == 0) return;
    const u32 lo = h & 0xFFFFu, idx = h >> 16;
    if (lo == 0 || !(static_cast<s32>(idx) < static_cast<s32>(load<u32>(kPool + 24)))) return;
    const u32 rec = load<u32>(kPool + 28) + idx * 272;
    if (((load<u32>(rec + 256) >> 15) & 0xFFFFu) != lo || (load<u8>(rec + 256) & 0x80u) != 0) return;
    if (p == 0) return;
    const u32 e = rec + 16;
    const u32 fl = load<u32>(p + 56);
    const f64 th = Lf(0x00443CDCu);
    if ((fl & 2u) != 0) {
        store<u8>(e + 12, 1);
        if (!Near3(e + 176, p, th)) {
            Cw(e + 176, p);
            Cw(e + 180, p + 4);
            store<u8>(e + 192, 1);
            Cw(e + 184, p + 8);
        }
    }
    if ((fl & 2u) != 0 || (load<u8>(e + 12) != 0 && fl != 0)) {
        ctx.fpr[1] = th;
        if (!Near3(e + 208, p + 16, th)) {
            Cw(e + 208, p + 16);
            Cw(e + 212, p + 20);
            store<u8>(e + 224, 1);
            Cw(e + 216, p + 24);
            ctx.fpr[1] = Lf(p + 24);
        }
    }
    const u32 tgt = load<u32>(p + 48);
    if (tgt != 0 && load<u8>(e + 172) != 0) {
        store<u32>(e + 156, tgt);
        store<u32>(e + 160, load<u32>(p + 52));
    }
    ctx.gpr[3] = 1;
}

// u32 Targets_Collect270258(Query* o, a1): the 32-entry candidate table
// 0x01067AF8 ({0, FLT_MAX} pairs) is reset once (u8 0x01067AF0); then the
// first s32 o->12->0 entries are reset (at least one, unchecked against 32).
// With entries, o->12->4 > 0.0 and kind o->8 == 1, every active (bit 0x01 of
// +192) object of the u16 *0x01057528 12-byte slots 0x010573A8 other than
// o->4->8 goes to 0x0026FD90(o, a1, x, table, &count, 1, 0) (count at sp+112,
// starts 0). r3 = 0x0026FAD4(o, table, count). Kinds 2 / 4 / 5 test flags of
// o->4->8 +184 but every outcome falls through.
DECOMP(00270258, Targets_Collect270258) {
    const u32 o = arg<u32>(ctx, 0), a1 = arg<u32>(ctx, 1);
    Stack frame(ctx, 176);
    const u32 sp = frame.addr();
    constexpr u32 kInit = 0x01067AF0u, kTab = 0x01067AF8u;
    if (load<u8>(kInit) == 0) {
        for (u32 k = 0; k < 32; ++k) {
            store<u32>(kTab + 8 * k, 0);
            store<u32>(kTab + 8 * k + 4, 0x7F7FFFFFu);
        }
        store<u8>(kInit, 1);
    }
    const u32 q = load<u32>(o + 12);
    store<u32>(sp + 112, 0);
    const u32 n = load<u32>(q);
    if (n != 0) {
        s32 left = static_cast<s32>(n);
        u32 off = 0;
        do {  // quirk: at least once, no bound against the 32 entries
            --left;
            store<u32>(kTab + off, 0);
            store<u32>(kTab + off + 4, 0x7F7FFFFFu);
            off += 8;
        } while (left > 0);
    }
    const u32 self = load<u32>(load<u32>(o + 4) + 8);
    const s32 kind = static_cast<s32>(load<u32>(o + 8));
    if (n != 0) {
        ctx.fpr[1] = Lf(q + 4);
        ctx.fpr[2] = Lf(0x00270250u);  // 0.0f
        if (ctx.fpr[1] > ctx.fpr[2] && kind == 1) {
            const u32 cnt = load<u16>(0x01057528u);
            for (s32 i = 0; i < static_cast<s32>(cnt); ++i) {
                const u32 x = load<u32>(0x010573A8u + 12 * i);
                if (x == 0 || (load<u32>(x + 192) & 1u) == 0 || x == self) continue;
                call(ctx, 0x0026FD90u, {o, a1, x, kTab, sp + 112, 1, 0});
            }
        }
    }
    ctx.gpr[3] = call(ctx, 0x0026FAD4u, {o, kTab, load<u32>(sp + 112)});
}

// Child_Run_00406E60(Child* c; f32 dt): states 1 / 2. The clock c->116 +=
// dt, t = clock / c->124 clamped to 1.0. On a path c->8 the child is posed
// at WorldObj_SplinePointTangentAt(path, t) (r4 passed through): the rows
// up x tangent, up (*0x01164B60), and their cross, normalized, and the point
// go to c->4->vfn[84](c->4, rows) (c->4 non-zero). At t == 1: state 3 and the
// owner's effect r = c->4->vfn[272]() gets its kind-4 component 0x87208BC1
// posed from the owner's rows (c->4+96..) and the cue Effect_SetHash(r,
// 0x002005F4). r3 / f1 as the original leaves them.
DECOMP(00406E60, Child_Run_00406E60) {
    const u32 c = arg<u32>(ctx, 0);
    const f64 dt = ctx.fpr[1];
    Stack frame(ctx, 256);
    const u32 sp = frame.addr();
    const f64 clk = S(Lf(c + 116) + dt);
    f64 t = Divs(clk, Lf(c + 124));
    Sf(c + 116, clk);
    const f64 one = Lf(0x00406E58u);  // 1.0f
    const u32 path = load<u32>(c + 8);
    if (t > one) t = one;
    ctx.fpr[1] = clk;
    ctx.gpr[3] = path;
    if (path != 0) {
        callf(ctx, kSplineTanAt, {path, ctx.gpr[4], sp + 128, sp + 112, 0}, {t});
        const f64 u0 = Lf(kUp), u1 = Lf(kUp + 4), u2 = Lf(kUp + 8);
        const f64 v0 = Lf(sp + 112), v1 = Lf(sp + 116), v2 = Lf(sp + 120);
        f64 sy = Fmsubs(u2, v0, S(u0 * v2));
        f64 sx = Fmsubs(u1, v2, S(u2 * v1));
        f64 sz = Fmsubs(u0, v1, S(u1 * v0));
        Cw(sp + 160, kUp);
        Cw(sp + 164, kUp + 4);
        Cw(sp + 168, kUp + 8);
        Cw(sp + 192, sp + 128);
        Cw(sp + 196, sp + 132);
        Cw(sp + 200, sp + 136);
        const f64 l = Divs(one, Sqrts(Fmadds(sz, sz, Fmadds(sx, sx, S(sy * sy)))));
        sx = S(sx * l);
        sz = S(sz * l);
        sy = S(sy * l);
        Sf(sp + 144, sx);
        Sf(sp + 148, sy);
        Sf(sp + 152, sz);
        const f64 a = Fmsubs(sy, u2, S(sz * u1));
        const f64 b = Fmsubs(sz, u0, S(sx * u2));
        const f64 d = Fmsubs(sx, u1, S(sy * u0));
        const f64 k = Divs(one, Sqrts(Fmadds(d, d, Fmadds(a, a, S(b * b)))));
        const f64 ta = S(a * k), tb = S(b * k), td = S(d * k);
        Sf(sp + 112, ta);
        Sf(sp + 116, tb);
        Sf(sp + 120, td);
        Sf(sp + 176, ta);
        Sf(sp + 180, tb);
        Sf(sp + 184, td);
        ctx.fpr[1] = td;
        const u32 own = load<u32>(c + 4);
        ctx.gpr[3] = 0;
        if (own != 0) VCall(ctx, own, 84, {own, sp + 144});
    }
    if (!(t == one)) return;
    store<u32>(c + 112, 3);
    const u32 own = load<u32>(c + 4);
    const u32 r = static_cast<u32>(VCall(ctx, own, 272, {own}));
    if (r == 0) return;
    const u32 p = static_cast<u32>(call(ctx, kFindComponent, {r, 0x87208BC1u}));
    if (p != 0 && static_cast<u32>(VCall(ctx, p, 32, {p})) == 4) {
        CopyRows(p, own);
        ctx.fpr[1] = Lf(own + 152);
    }
    call(ctx, kSetHash, {r, 0x002005F4u});
}

// Child_Orbit405C18(Child* c; f32 dt): state 12. The clock c->116 += dt, t =
// clock / c->124 clamped to [K1, K0] (constants 0x00405C0C); the angles
// c->132 += c->136 * dt and c->140 += c->144 * dt. The position c+192 is the
// blend of the four spline points (paths c->12 / 16 / 20 / 24 at t, weights
// c->208..220); the rotation is quat(axis *0x01164B50, -c->132 * K2) *
// quat(up *0x01164B60, c->140 * K2) * c+160, posed to c->4->vfn[84](c->4,
// Quat_ToMatrix(.., c+192)). When the clock passes c->124: c->116 = c->124 = 0
// and state 13 (r3 = 0). The spline calls after the first pass the r4 the
// previous one left.
DECOMP(00405C18, Child_Orbit405C18) {
    const u32 c = arg<u32>(ctx, 0);
    const f64 dt = ctx.fpr[1];
    Stack frame(ctx, 400);
    const u32 sp = frame.addr();
    constexpr u32 C = 0x00405C0Cu;
    const f64 clk = S(Lf(c + 116) + dt);
    f64 t = Divs(clk, Lf(c + 124));
    Sf(c + 116, clk);
    const f64 k0 = Lf(C);
    const f64 a132 = Lf(c + 132), a136 = Lf(c + 136), a140 = Lf(c + 140), a144 = Lf(c + 144);
    const u32 p12 = load<u32>(c + 12);
    if (t > k0) t = k0;
    const f64 k1 = Lf(C + 4);
    if (t < k1) t = k1;
    Sf(c + 132, Fmadds(a136, dt, a132));
    Sf(c + 140, Fmadds(a144, dt, a140));
    callf(ctx, kSplineAt, {p12, C, sp + 256, 0}, {t});
    callf(ctx, kSplineAt, {load<u32>(c + 16), ctx.gpr[4], sp + 272, 0}, {t});
    callf(ctx, kSplineAt, {load<u32>(c + 20), ctx.gpr[4], sp + 288, 0}, {t});
    callf(ctx, kSplineAt, {load<u32>(c + 24), ctx.gpr[4], sp + 304, 0}, {t});
    const f64 w0 = Lf(c + 208), w1 = Lf(c + 212), w2 = Lf(c + 216), w3 = Lf(c + 220);
    f64 pos[3];
    for (u32 k = 0; k < 3; ++k) {
        f64 v = S(Lf(sp + 272 + 4 * k) * w1);
        v = Fmadds(Lf(sp + 256 + 4 * k), w0, v);
        v = Fmadds(Lf(sp + 288 + 4 * k), w2, v);
        pos[k] = Fmadds(Lf(sp + 304 + 4 * k), w3, v);
    }
    Sf(c + 192, pos[0]);
    Sf(c + 196, pos[1]);
    Sf(c + 200, pos[2]);
    const f64 ax = Lf(kAxis), ay = Lf(kAxis + 4), az = Lf(kAxis + 8);
    const f64 la = Divs(k0, Sqrts(Fmadds(az, az, Fmadds(ax, ax, S(ay * ay)))));
    const f64 k2 = Lf(C + 8);
    const f64 ang1 = S(-Lf(c + 132) * k2);
    f64 qx = S(ax * la), qy = S(ay * la), qz = S(az * la);
    const f64 s1 = callf(ctx, kSinf, {}, {ang1});
    qx = S(s1 * qx);
    qy = S(s1 * qy);
    qz = S(s1 * qz);
    const f64 c1 = callf(ctx, kCosf, {}, {ang1});
    const f64 ux = Lf(kUp), uy = Lf(kUp + 4), uz = Lf(kUp + 8);
    Sf(sp + 144, qx);
    Sf(sp + 148, qy);
    Sf(sp + 152, qz);
    Sf(sp + 156, c1);
    const f64 ang2 = S(Lf(c + 140) * k2);
    const f64 lu = Divs(k0, Sqrts(Fmadds(uz, uz, Fmadds(ux, ux, S(uy * uy)))));
    f64 rx = S(ux * lu), ry = S(uy * lu), rz = S(uz * lu);
    const f64 s2 = callf(ctx, kSinf, {}, {ang2});
    rx = S(s2 * rx);
    ry = S(s2 * ry);
    rz = S(s2 * rz);
    const f64 c2 = callf(ctx, kCosf, {}, {ang2});
    Sf(sp + 160, rx);
    Sf(sp + 164, ry);
    Sf(sp + 168, rz);
    Sf(sp + 172, c2);
    call(ctx, kQuatMul, {sp + 128, sp + 160, sp + 144});
    call(ctx, kQuatMul, {sp + 112, sp + 128, c + 160});
    for (u32 k = 0; k < 4; ++k) Cw(sp + 176 + 4 * k, sp + 112 + 4 * k);
    const u32 own = load<u32>(c + 4);
    call(ctx, kQuatToMatrix, {sp + 192, sp + 176, c + 192});
    VCall(ctx, own, 84, {own, sp + 192});
    ctx.fpr[1] = Lf(c + 116);
    if (Lf(c + 116) > Lf(c + 124)) {
        store<u32>(c + 116, 0);
        store<u32>(c + 124, 0);
        store<u32>(c + 112, 13);
        ctx.gpr[3] = 0;
    }
}

// Child_Arc406A40(Child* c; f32 dt): state 10. Needs the target effect
// c->28 and the owner c->4 (r3 = c->28 otherwise). The clock c->116 += dt, t
// = clock / c->124 clamped to 1 (constants 0x00406A24: 1.0, K1, K2, K3, K4,
// K5); the point c+96 + (fx+64 - c+96) * t raised by c->128 * (1 - ((t - K1)
// * K2)^2). From t >= K1 the owner's animator (vfn[60]) restarts the clip
// 0xB21F21C2 unless Anim_Find(an, 0). The owner's rows (+96 / +112 / +128)
// and the point go to vfn[84](Mat34_FromRotPos). Crossing K5 this step
// (t > K5 >= t - dt / c->124) poses the owner's effect component
// 0x87208BC1 and cues 0xE4E5DD5B. At t == 1: clock / duration 0, state 14,
// and unless the world's u16 +0x128214 or List_AnySet(): spawner c->84 count
// +196 += 1, Obj_Set180(s->236, count), u8 0x0106B62C = 1.
DECOMP(00406A40, Child_Arc406A40) {
    const u32 c = arg<u32>(ctx, 0);
    const f64 dt = ctx.fpr[1];
    Stack frame(ctx, 336);
    const u32 sp = frame.addr();
    constexpr u32 C = 0x00406A24u;
    const u32 fx = load<u32>(c + 28);
    ctx.gpr[3] = fx;
    if (fx == 0) return;
    if (load<u32>(c + 4) == 0) return;
    const u32 own = load<u32>(c + 4);
    const f64 clk = S(Lf(c + 116) + dt);
    f64 t = Divs(clk, Lf(c + 124));
    Sf(c + 116, clk);
    const f64 one = Lf(C);
    if (t > one) t = one;
    const f64 x0 = Lf(c + 96), y0 = Lf(c + 100), z0 = Lf(c + 104);
    const f64 dx = S(Lf(fx + 64) - x0);
    const f64 px = Fmadds(dx, t, x0);
    Sf(sp + 144, dx);
    const f64 dy = S(Lf(fx + 68) - y0);
    Sf(sp + 148, dy);
    const f64 dz = S(Lf(fx + 72) - z0);
    Sf(sp + 144, px);
    f64 py = Fmadds(dy, t, y0);
    Sf(sp + 152, Fmadds(dz, t, z0));
    const f64 k1 = Lf(C + 4);
    const f64 u = S(S(t - k1) * Lf(C + 8));
    py = Fmadds(Lf(c + 128), Fnmsubs(u, u, one), py);
    Sf(sp + 148, py);
    if (t >= k1) {
        u32 an = static_cast<u32>(VCall(ctx, own, 60, {own}));
        if (static_cast<u32>(call(ctx, kAnimFind, {an, 0})) == 0) {
            const u32 o2 = load<u32>(c + 4);
            an = static_cast<u32>(VCall(ctx, o2, 60, {o2}));
            // arguments 9 / 10 (sp+112 / sp+120) are whatever the frame holds
            callf(ctx, kAnimRestart, {an, 0xB21F21C2u, 0, 0, 0, ~0ull, 0, 0, 0, 0, 0, ~0ull},
                  {one, Lf(C + 12), Lf(C + 16), Lf(C + 16)});
        }
    }
    const u32 o3 = load<u32>(c + 4);
    for (u32 row = 0; row < 3; ++row)
        for (u32 k = 0; k < 3; ++k) Cw(sp + 160 + 16 * row + 4 * k, o3 + 96 + 16 * row + 4 * k);
    ctx.fpr[1] = Lf(o3 + 136);
    call(ctx, kMat34FromRotPos, {sp + 208, sp + 160, sp + 144});
    VCall(ctx, o3, 84, {o3, sp + 208});
    ctx.gpr[3] = C;  // lis/addi r3: the constants' address
    const f64 k5 = Lf(C + 20);
    ctx.fpr[1] = k5;
    if (t > k5 && S(t - Divs(dt, Lf(c + 124))) <= k5) {
        const u32 o4 = load<u32>(c + 4);
        const u32 r = static_cast<u32>(VCall(ctx, o4, 272, {o4}));
        if (r != 0) {
            const u32 p = static_cast<u32>(call(ctx, kFindComponent, {r, 0x87208BC1u}));
            if (p != 0 && static_cast<u32>(VCall(ctx, p, 32, {p})) == 4) {
                CopyRows(p, o4);
                ctx.fpr[1] = Lf(o4 + 152);
            }
            call(ctx, kSetHash, {r, 0xE4E5DD5Bu});
        }
    }
    if (!(t == one)) return;
    store<u32>(c + 116, 0);
    const u32 w = load<u32>(kWorld);
    store<u32>(c + 124, 0);
    store<u32>(c + 112, 14);
    const u32 s = load<u32>(c + 84);
    const u32 busy = load<u16>(w + 0x128214u);
    ctx.gpr[3] = busy;
    if (busy != 0) return;
    if (static_cast<u32>(call(ctx, kListAnySet, {})) != 0) return;
    const u32 n = load<u32>(s + 196) + 1;
    const u32 tgt = load<u32>(s + 236);
    store<u32>(s + 196, n);
    call(ctx, kObjSet180, {tgt, n});
    store<u8>(0x0106B62Cu, 1);
    ctx.gpr[3] = 0x01070000u;  // lis r3, 0x107 (the flag's base)
}

namespace {
constexpr u32 kPartAt = 0x001BD51Cu;       // Field_At388x96_001BD51C(t, i): t->388 + 96 * i
constexpr u32 kPartV404 = 0x001BD450u;     // Field_VCall404_10_001BD450(t, b)
constexpr u32 kPartV384 = 0x001BD4CCu;     // Field_VCall384_10_001BD4CC(t, b)
constexpr u32 kForEachChild = 0x001BD2B0u;  // Obj_ForEachChild_001BD2B0(t, a, b)
constexpr u32 kPartBuild = 0x001EFDE8u;    // 0x001EFDE8(o, o->804, n, rows, x, 0, def+352, def+368) -> chain
constexpr u32 kEffectSpawn = 0x004A5FC8u;  // Effect_Spawn(hash, a, b, c)
constexpr u32 kMotionSet = 0x004A0C20u;    // Motion_Set4A0C20(m; f1)
constexpr u32 kReleaseChild = 0x001ED85Cu;  // Obj_ReleaseChild_001ED85C(o, u)
constexpr u32 kTargetDef = 0x000D951Cu;    // Target_GetDef0D951C(o)
constexpr u32 kDropHeld = 0x002B7704u;     // Obj_DropHeld_002B7704(d)
constexpr u32 kNotifyKeyed = 0x000D90ECu;  // Obj_NotifyKeyed_000D90EC(o, key, b)
constexpr u32 kSetStateFx = 0x00266E6Cu;   // Obj_SetStateFx266E6C(d, 0)

// The 12 pose floats of a 96-byte part record (+0..+56, rows of 3).
constexpr u32 kPose12[12] = {0, 4, 8, 16, 20, 24, 32, 36, 40, 48, 52, 56};
// The 15 floats copied record to record (+0..+72).
constexpr u32 kRec15[15] = {0, 4, 8, 16, 20, 24, 32, 36, 40, 48, 52, 56, 64, 68, 72};

void CopyRec15(u32 d, u32 s) {
    for (u32 k : kRec15) Cw(d + k, s + k);
}
}  // namespace

// Part_BreakAt1F46D4(Obj* o, u32 i): the hit on segment i of the part chain
// o->820 (callee of Part_HitTest_001F4DC0). The previous pieces o->828 /
// o->832 are released (vfn[40](1)) and cleared; with a chain and a holder
// o->816: the chain is switched off (Field_VCall404/384(0), bit 0x10000000
// of o->980 cleared, children (0, 1), o->vfn[88](1, 1, 1, 1)); the split
// point m (i <= 1: 2; else i > n - 1 ? i : n - 2, n = o->836) and the break
// effect 0x88F3C164 (component 0x7729DA2F posed from record i). Two new
// chains o->828 (records 0..m-1, rows of record 0) and o->832 (records n-1
// down to m, rows of record n saved at sp+112) are built by 0x001EFDE8 and
// filled from the old records; both get u16 +394 bit 0x8000 set and 0x4000
// cleared. o->896 = 255, o->900 = 0, Motion_Set(o+876, *0x010540D8), o->880
// = 0, Obj_ReleaseChild(o, o->812), and the holder o->816 is let go
// (handler 160->260 vfn[24], vfn[88](0, 1, 1, 1), vfn[40](0), vfn[40](1),
// o->816 = 0). A target def d (Target_GetDef) drops its hold: bit 0x00100000
// of o->980 cleared, Obj_DropHeld(d), Obj_NotifyKeyed(o, 27, 1),
// Obj_SetStateFx(d, 0). r3 / f1 as the original leaves them.
DECOMP(001F46D4, Part_BreakAt1F46D4) {
    const u32 o = arg<u32>(ctx, 0), i = arg<u32>(ctx, 1);
    Stack frame(ctx, 336);
    const u32 sp = frame.addr();
    const u32 def = load<u32>(o + 216);
    for (u32 k = 0; k < 2; ++k) {
        const u32 h = load<u32>(o + 828 + 4 * k);
        ctx.gpr[3] = h;
        if (h == 0) continue;
        VCall(ctx, h, 40, {h, 1});
        store<u32>(o + 828 + 4 * k, 0);
    }
    const u32 t = load<u32>(o + 820);
    if (t == 0) return;
    if (load<u32>(o + 816) == 0) {
        ctx.gpr[3] = 0;
        return;
    }
    call(ctx, kPartV404, {t, 0});
    call(ctx, kPartV384, {load<u32>(o + 820), 0});
    store<u32>(o + 980, load<u32>(o + 980) & ~0x10000000u);
    call(ctx, kForEachChild, {load<u32>(o + 820), 0, 1});
    VCall(ctx, o, 88, {o, 1, 1, 1, 1});
    u32 m = 2;
    if (i > 1) {
        const u32 n = load<u32>(o + 836);
        m = i > n - 1 ? i : n - 2;
    }
    const u32 a = static_cast<u32>(call(ctx, kPartAt, {load<u32>(o + 820), load<u32>(o + 836)}));
    for (u32 k : kPose12) Cw(sp + 112 + k, a + k);
    const u32 b = static_cast<u32>(call(ctx, kPartAt, {load<u32>(o + 820), i}));
    u32 pose[12];
    for (u32 k = 0; k < 12; ++k) pose[k] = load<u32>(b + kPose12[k]);
    const u32 e = static_cast<u32>(call(ctx, kEffectSpawn, {0x88F3C164u, 0, 0, 1}));
    if (e != 0) {
        const u32 c = static_cast<u32>(call(ctx, kFindComponent, {e, 0x7729DA2Fu}));
        if (c != 0 && static_cast<u32>(VCall(ctx, c, 32, {c})) == 4)
            for (u32 k = 0; k < 12; ++k) store<u32>(c + 32 + kPose12[k], pose[k]);
    }
    const u32 h816 = load<u32>(o + 816);
    ctx.fpr[1] = Lf(sp + 168);  // f1: the last float copied to the frame
    store<u32>(o + 828, static_cast<u32>(call(ctx, kPartBuild,
        {o, load<u32>(o + 804), m, o + 96, load<u32>(h816 + 160), 0, def + 352, def + 368})));
    const u32 n2 = load<u32>(o + 836);
    const u32 x2 = load<u32>(load<u32>(o + 816) + 160);
    store<u32>(o + 832, static_cast<u32>(call(ctx, kPartBuild,
        {o, load<u32>(o + 804), n2 - m, sp + 112, x2, 0, def + 352, def + 368})));
    if (m != 0) {
        u32 j = 0;
        do {
            const u32 d = static_cast<u32>(call(ctx, kPartAt, {load<u32>(o + 828), j}));
            const u32 s = static_cast<u32>(call(ctx, kPartAt, {load<u32>(o + 820), j}));
            CopyRec15(d, s);
            ctx.fpr[1] = Lf(s + 72);
            ++j;
        } while (j < m);
    }
    u32 src = load<u32>(o + 836) - 1;
    if (!(src < m)) {
        u32 j = 0;
        do {
            const u32 d = static_cast<u32>(call(ctx, kPartAt, {load<u32>(o + 832), j}));
            ++j;
            const u32 s = static_cast<u32>(call(ctx, kPartAt, {load<u32>(o + 820), src}));
            CopyRec15(d, s);
            ctx.fpr[1] = Lf(s + 72);
            --src;
        } while (!(src < m));
    }
    for (u32 k = 0; k < 2; ++k) {
        const u32 off = k == 0 ? 828 : 832;
        u32 q = load<u32>(o + off);
        store<u16>(q + 394, static_cast<u16>(load<u16>(q + 394) | 0x8000u));
        q = load<u32>(o + off);
        store<u16>(q + 394, static_cast<u16>(load<u16>(q + 394) & ~0x4000u));
    }
    store<u32>(o + 896, 255);
    store<u32>(o + 900, 0);
    callf(ctx, kMotionSet, {o + 876}, {Lf(0x010540D8u)});
    store<u32>(o + 880, 0);
    call(ctx, kReleaseChild, {o, load<u32>(o + 812)});
    u32 hd = load<u32>(o + 816);
    if (hd != 0) {
        const u32 x = load<u32>(hd + 160);
        if (x != 0) {
            const u32 fx = load<u32>(x + 260);
            if (fx != 0) {
                VCall(ctx, fx, 24, {fx, x});
                hd = load<u32>(o + 816);
            }
        }
        VCall(ctx, hd, 88, {hd, 0, 1, 1, 1});
        hd = load<u32>(o + 816);
        VCall(ctx, hd, 40, {hd, 0});
        hd = load<u32>(o + 816);
        VCall(ctx, hd, 40, {hd, 1});
        store<u32>(o + 816, 0);
    }
    const u32 d = static_cast<u32>(call(ctx, kTargetDef, {o}));
    if (d == 0) return;
    store<u32>(o + 980, load<u32>(o + 980) & ~0x00100000u);
    call(ctx, kDropHeld, {d});
    call(ctx, kNotifyKeyed, {o, 27, 1});
    call(ctx, kSetStateFx, {d, 0});
}
