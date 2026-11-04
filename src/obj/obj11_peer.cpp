// World objects, part 11: the spline follower of a peer record (+136):
// Follower_Update moves it along its spline and turns it to face the
// direction of travel; Peer_ApplyFollower composes it into the matrix
// Peer_GetMatrix builds.
#include "obj11.h"

using namespace sly4;
using namespace sly4::obj8;
using namespace sly4::obj9;
using namespace sly4::obj10;
using namespace sly4::obj11;

namespace {

// The original's 304-byte frame: call() adds the 112-byte linkage area
// below the block reserved here, which starts at sp+112.
constexpr u32 kFuFrame = 304 - 112;
constexpr u32 kFsPos = 112;       // f32[3]: the position the advance returns
constexpr u32 kFsAhead = 128;     // f32[4]: the spline point, then in world space
constexpr u32 kFsQuatOut = 144;   // f32[4]: turn * current
constexpr u32 kFsTurn = 160;      // f32[4]: the turn, later the normalised result
constexpr u32 kFsQuatCur = 176;   // f32[4]: the follower's current rotation
constexpr u32 kFsMatrix = 192;    // f32[4][4]
constexpr u32 kFsParam = 256;     // f32: the spline parameter

f32 Fu(u32 off) { return Lf(kFuConsts + off); }
void CopyWord(u32 dst, u32 src) { store<u32>(dst, load<u32>(src)); }

void StoreQuat(u32 q, f32 x, f32 y, f32 z, f32 w) {
    store<f32>(q, x);
    store<f32>(q + 4, y);
    store<f32>(q + 8, z);
    store<f32>(q + 12, w);
}

// The rotation taking the follower's up axis to the travel direction D:
// {u x D, H.u} normalised, with H = u + D (the half vector), unless u and D
// are (anti)parallel (|H.u| <= 0.001): then a half turn about an axis
// perpendicular to u (built from its two larger components), or the
// identity when those are below 0.0001.
void TurnQuat(u32 q, f32 dx, f32 dy, f32 dz, f32 len2) {
    const f32 one = Fu(kFuOne);
    const f32 len = Fsqrts(len2);
    const f32 ux0 = Lf(kUpAxis), uy0 = Lf(kUpAxis + 4), uz0 = Lf(kUpAxis + 8);
    const f32 ulen = Fsqrts(Fmadds(uz0, uz0, Fmadds(ux0, ux0, Fmuls(uy0, uy0))));
    const f32 il = Fdivs(one, len), iu = Fdivs(one, ulen);
    const f32 ny = Fmuls(dy, il), nx = Fmuls(dx, il), nz = Fmuls(dz, il);
    const f32 nlen = Fsqrts(Fmadds(nz, nz, Fmadds(nx, nx, Fmuls(ny, ny))));
    const f32 uy = Fmuls(iu, uy0), ux = Fmuls(iu, ux0);
    const f32 inl = Fdivs(one, nlen);
    const f32 uz = Fmuls(iu, uz0);
    const f32 ddy = Fmuls(inl, ny), ddx = Fmuls(inl, nx), ddz = Fmuls(inl, nz);
    const f32 hy = Fadds(uy, ddy), hx = Fadds(ux, ddx), hz = Fadds(uz, ddz);
    const f32 dot = Fmadds(hz, uz, Fmadds(hx, ux, Fmuls(hy, uy)));
    if (std::fabs(dot) > Fu(kFuParallel)) {
        const f32 a = Fmuls(ux, ddz), b = Fmuls(uz, ddy), c = Fmuls(uy, ddx);
        const f32 cy = Fmsubs(uz, ddx, a), cx = Fmsubs(uy, ddz, b), cz = Fmsubs(ux, ddy, c);
        const f32 n = Fdivs(one, Fsqrts(Fmadds(dot, dot, Fmadds(cz, cz, Fmadds(cx, cx, Fmuls(cy, cy))))));
        StoreQuat(q, Fmuls(cx, n), Fmuls(cy, n), Fmuls(cz, n), Fmuls(dot, n));
        return;
    }
    const bool x_major = ux > uy;
    const f32 l = x_major ? Fsqrts(Fmadds(ux, ux, Fmuls(uz, uz))) : Fsqrts(Fmadds(uy, uy, Fmuls(uz, uz)));
    if (l < Fu(kFuTiny)) {
        store<u32>(q, 0);
        store<u32>(q + 4, 0);
        store<u32>(q + 8, 0);
        store<u32>(q + 12, cls::kOneF);
        return;
    }
    const f32 inv = Fdivs(one, l);
    if (x_major) {  // {-uz, 0, ux, 0} / l
        store<u32>(q + 4, 0);
        store<u32>(q + 12, 0);
        const f32 a = Fmuls(inv, uz), b = Fmuls(inv, ux);
        store<f32>(q + 8, b);
        store<f32>(q, -a);
    } else {  // {0, uz, -uy, 0} / l
        store<u32>(q, 0);
        store<u32>(q + 12, 0);
        const f32 a = Fmuls(inv, uy), b = Fmuls(inv, uz);
        store<f32>(q + 4, b);
        store<f32>(q + 8, -a);
    }
}

}  // namespace

// void Follower_Update(Follower* f, f32 dt)
// Inactive (+100 == 0): nothing. While the start delay (+96) is above 0 it
// counts down by dt. Otherwise the follower advances (Follower_AdvanceAlt
// for +108 bit 0x40, else Follower_Advance) to a new position and spline
// parameter; when it has a spline record (+104, with spline data +80) the
// spline point at that parameter, in the record's world space, gives the
// direction of travel, and the follower's rotation (rows 0-2) becomes
// normalise(turn * current) (TurnQuat; Quat_FromMatrix, Quat_Multiply,
// Quat_ToMatrix). The position row (+48) is the new position.
DECOMP(0057BA40, Follower_Update) {
    const u32 f = arg<u32>(ctx, 0);
    const f64 dt = farg(ctx, 0);
    if (load<u32>(f + kFoActive) == 0) return;
    const f32 delay = Lf(f + kFoDelay);
    if (delay > Fu(kFuZero)) {
        store<f32>(f + kFoDelay, Fsubs(delay, dt));
        return;
    }
    Stack frame(ctx, kFuFrame);
    const u32 sp = frame.addr() - 112;
    store<u32>(sp + kFsParam, 0);
    const u32 adv = (load<u8>(f + kFoBits) & kFoAlt) != 0 ? o11fn::Follower_AdvanceAlt : o11fn::Follower_Advance;
    // r4 = what the caller left there (neither callee reads it).
    callf(ctx, adv, {f, ctx.gpr[4], sp + kFsParam, sp + kFsPos}, {dt});

    const u32 rec = load<u32>(f + kFoRecord);
    if (rec != 0 && load<u32>(rec + kSrSpline) != 0) {
        // r4 = the advance's leftover (Spline_PointAt doesn't read it).
        callf(ctx, o11fn::Spline_PointAt, {load<u32>(rec + kSrSpline), ctx.gpr[4], sp + kFsAhead, 0},
              {static_cast<f64>(Lf(sp + kFsParam))});
        const u32 a = sp + kFsAhead;
        const Vec4 x = VSplat(VLoadLeft(a), 0), y = VSplat(VLoadLeft(a + 4), 0), z = VSplat(VLoadLeft(a + 8), 0);
        Vec4 w = VMadd(VLoad(rec + kSrMatrix), x, VZero());
        w = VMadd(VLoad(rec + 16), y, w);
        w = VMadd(VLoad(rec + 32), z, w);
        const f32 py = Lf(sp + kFsPos + 4), px = Lf(sp + kFsPos), pz = Lf(sp + kFsPos + 8);
        VStore(a, VAdd(VLoad(rec + 48), w));
        const f32 dy = Fsubs(Lf(a + 4), py), dx = Fsubs(Lf(a), px), dz = Fsubs(Lf(a + 8), pz);
        const f32 len2 = Fmadds(dz, dz, Fmadds(dx, dx, Fmuls(dy, dy)));
        if (len2 != Fu(kFuZero)) {
            TurnQuat(sp + kFsTurn, dx, dy, dz, len2);
            call(ctx, o11fn::Quat_FromMatrix, {sp + kFsQuatCur, f});
            call(ctx, o10fn::Quat_Multiply, {sp + kFsQuatOut, sp + kFsTurn, sp + kFsQuatCur});
            const u32 qo = sp + kFsQuatOut;
            const f32 qy = Lf(qo + 4), qx = Lf(qo), qz = Lf(qo + 8), qw = Lf(qo + 12);
            const f32 n = Fdivs(Fu(kFuOne), Fsqrts(Fmadds(qw, qw, Fmadds(qz, qz, Fmadds(qx, qx, Fmuls(qy, qy))))));
            StoreQuat(sp + kFsTurn, Fmuls(qx, n), Fmuls(qy, n), Fmuls(qz, n), Fmuls(qw, n));
            call(ctx, o10fn::Quat_ToMatrix, {sp + kFsMatrix, sp + kFsTurn, kOrigin});
            for (u32 r = 0; r < 64; r += 16)
                for (u32 k = 0; k < 12; k += 4) CopyWord(f + r + k, sp + kFsMatrix + r + k);
        }
    }
    for (u32 k = 0; k < 12; k += 4) CopyWord(f + kFoPos + k, sp + kFsPos + k);
}

// void Peer_ApplyFollower(PeerRecord* p, Follower* f, -, f32 out[4][4],
//                         bool rotated, bool moved, f32 dt)
// Updates the follower, then out's rotation = (rotated ? out : identity)
// rows times the follower's rotation, out's translation = (moved ? out's :
// 0) + the follower's position, and finally out *= p's matrix
// (Matrix_Multiply).
DECOMP(0057C110, Peer_ApplyFollower) {
    const u32 p = arg<u32>(ctx, 0), f = arg<u32>(ctx, 1), out = arg<u32>(ctx, 3);
    const bool rotated = (arg<u32>(ctx, 4) & 0xFF) != 0, moved = (arg<u32>(ctx, 5) & 0xFF) != 0;
    callf(ctx, o10fn::Follower_Update, {f, f}, {farg(ctx, 0)});  // r4 = f as well

    // The base rows' x, y, z (the original copies them to sp+112).
    const u32 src = rotated ? out : kIdentity;
    u32 m[3][3];
    for (u32 i = 0; i < 3; ++i)
        for (u32 k = 0; k < 3; ++k) m[i][k] = load<u32>(src + 16 * i + 4 * k);
    // The follower's rows, gathered lane by lane (lvlx + vperm).
    f32 fr[3][3];
    for (u32 i = 0; i < 3; ++i)
        for (u32 k = 0; k < 3; ++k) fr[i][k] = VLoadLeft(f + 16 * i + 4 * k).f(0);
    // row_i = F0 * m[i].x + F1 * m[i].y + F2 * m[i].z (vmaddfp from 0).
    for (u32 i = 0; i < 3; ++i)
        for (u32 k = 0; k < 3; ++k) {
            f32 a = std::fmaf(fr[0][k], std::bit_cast<f32>(m[i][0]), 0.0f);
            a = std::fmaf(fr[1][k], std::bit_cast<f32>(m[i][1]), a);
            a = std::fmaf(fr[2][k], std::bit_cast<f32>(m[i][2]), a);
            store<f32>(out + 16 * i + 4 * k, a);
        }
    f32 base[3];
    for (u32 k = 0; k < 3; ++k) base[k] = moved ? Lf(out + 48 + 4 * k) : Lf(kPaZero);
    f32 t = 0;
    for (u32 k = 0; k < 3; ++k) store<f32>(out + 48 + 4 * k, t = Fadds(base[k], Lf(f + kFoPos + 4 * k)));
    ctx.fpr[1] = static_cast<f64>(t);  // f1 = the last sum, as the original leaves it
    call(ctx, o11fn::Matrix_Multiply, {out, p});  // r5 = 144 is left over (not read)
}
