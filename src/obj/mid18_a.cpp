// Game code, medium sweep part 18: area effect grids (emitter tree walks)
// and animation-state ticks (spline follow, idle / start / hurt / jump).
#include "mid18.h"

using namespace sly4;
using namespace sly4::mid18;

namespace {
constexpr u32 kEmitterSel = 0x01193088u;  // static passed to EmitterInstance_Set340

// Walks the binary tree whose root is *(it->12) in order (node +8 payload,
// +12 left, +16 right; the explicit stack it->16[] is counted by the s16 at
// it->96, the current node is it->100). Each payload whose vfn[32]() returns
// 5 and whose +28 part has kind ((+4 >> 10) & 0x3F) == 10 gets
// EmitterInstance_Set340(part, sel).
inline void EmitterWalk(PPUContext& ctx, u32 it, u32 sel) {
    auto push = [&](u32 node, bool countFirst) {
        const s32 n = static_cast<s16>(load<u16>(it + 96));
        if (countFirst) {
            store<u16>(it + 96, static_cast<u16>(n + 1));
            store<u32>(it + 16 + 4 * n, node);
        } else {
            store<u32>(it + 16 + 4 * n, node);
            store<u16>(it + 96, static_cast<u16>(n + 1));
        }
    };
    const u32 hdr = load<u32>(it + 12);
    store<u16>(it + 96, 0);
    u32 cur = 0;
    const u32 root = load<u32>(hdr);
    if (root != 0) {
        // descend to the leftmost node, stacking the path
        u32 nd = root;
        for (;;) {
            cur = nd;
            const u32 l = load<u32>(nd + 12);
            if (l == 0) break;
            push(nd, true);
            nd = l;
        }
    }
    store<u32>(it + 100, cur);
    while (cur != 0) {
        const u32 obj = load<u32>(cur + 8);
        if (static_cast<s32>(VCall(ctx, obj, 32, {obj})) == 5) {
            const u32 part = load<u32>(obj + 28);
            if (part != 0 && ((load<u32>(part + 4) >> 10) & 0x3F) == 10) call(ctx, 0x005A4890u, {part, sel});
        }
        // advance to the in-order successor
        const u32 c = load<u32>(it + 100);
        if (c == 0) break;
        u32 r = load<u32>(c + 16);
        if (r != 0) {
            push(c, true);
            while (load<u32>(r + 12) != 0) {
                push(r, false);
                r = load<u32>(r + 12);
            }
            store<u32>(it + 100, r);
            cur = r;
            continue;
        }
        // climb while we came from a right child
        u32 n = load<u16>(it + 96);
        if (n == 0) {
            store<u32>(it + 100, 0);
            break;
        }
        u32 prev = c;
        cur = 0;
        for (;;) {
            if ((n & 0xFFFF) == 0) {
                store<u32>(it + 100, 0);
                break;
            }
            n -= 1;
            store<u16>(it + 96, static_cast<u16>(n));
            const u32 p = load<u32>(it + 16 + 4 * static_cast<s32>(static_cast<s16>(n)));
            if (prev == load<u32>(p + 12)) {
                store<u32>(it + 100, p);
                cur = p;
                break;
            }
            store<u32>(it + 100, p);
            prev = p;
        }
    }
}
}  // namespace

// Area_AddEffect222654 (Area*, Effect* e): when the static selector at
// 0x01193088 has a nonzero u16 at +4, EmitterWalk(e + 8, 0x01193088).
// Returns 0.
DECOMP(00222654, Area_AddEffect222654) {
    const u32 it = arg<u32>(ctx, 1) + 8;
    if (load<u16>(kEmitterSel + 4) != 0) EmitterWalk(ctx, it, kEmitterSel);
    ret(ctx, 0);
}

// Area_RemoveEffect223640 (Area*, Effect* e): EmitterWalk(e + 8, 0), the
// counterpart of Area_AddEffect222654 that unbinds the emitters. Returns 0.
DECOMP(00223640, Area_RemoveEffect223640) {
    EmitterWalk(ctx, arg<u32>(ctx, 1) + 8, 0);
    ret(ctx, 0);
}

namespace {
constexpr u32 Anim_Find298874 = 0x00298874u;            // (Animator*, key) -> u8
constexpr u32 Anim_TrackSet36_0029879C = 0x0029879Cu;   // (Obj*, s32 id, u8 v) -> bool
constexpr u32 Anim_Restart2775A8 = 0x002775A8u;         // (Animator*, key, ... ; f1..f4)

// Anim_Restart2775A8(a, h, 0, 0, r7, r8, 0, 0, -, -, 0, -1; f1..f4): the
// callee reads neither r7 nor r8 (nor the never-written arguments 8 / 9,
// r1 +112 / +120); they are passed as the original leaves them.
inline void Restart(PPUContext& ctx, u32 a, u32 h, u64 r7, u64 r8, f64 f1, f64 f2, f64 f3, f64 f4) {
    callf(ctx, Anim_Restart2775A8, {a, h, 0, 0, r7, r8, 0, 0, 0, 0, 0, ~0ull}, {f1, f2, f3, f4});
}

// The body's animator: o->4 vfn[60]().
inline u32 Animator(PPUContext& ctx, u32 o) {
    const u32 a = load<u32>(o + 4);
    return static_cast<u32>(VCall(ctx, a, 60, {a}));
}

// The animation switch for hash h: unless Anim_Find298874(a, 0) is set
// (twice, a = Animator(o) each time) or Anim_TrackSet36_0029879C(a, 0, 0) is
// 0, h is started (once: skipped when o->128 already holds it, r3 = h) with
// Restart(Animator(o), h, r7, r8; f1..f4) after o->128 = h. Leaves r3 as the
// original does.
inline void AnimSwitch(PPUContext& ctx, u32 o, u32 h, u64 r7, u64 r8, f64 f1, f64 f2, f64 f3, f64 f4) {
    if (static_cast<s32>(call(ctx, Anim_Find298874, {Animator(ctx, o), 0})) == 0) {
        if (static_cast<s32>(call(ctx, Anim_Find298874, {Animator(ctx, o), 0})) != 0) return;
        if (static_cast<s32>(call(ctx, Anim_TrackSet36_0029879C, {Animator(ctx, o), 0, 0})) == 0) return;
    }
    if (load<u32>(o + 128) == h) {
        ctx.gpr[3] = static_cast<u64>(static_cast<s64>(static_cast<s32>(h)));  // lis/ori: sign-extended
        return;
    }
    store<u32>(o + 128, h);
    const u32 a = Animator(ctx, o);
    Restart(ctx, a, h, r7, r8, f1, f2, f3, f4);
}
}  // namespace

// Obj_SetSplinePos3CCC68 (Obj* o, const Vec3* p): t = SplineInstance_Nearest(
// o->108, p, 0, 0); WorldObj_SplinePointTangentAt(o->108, -, &pos, &tan, 0; t).
// The heading (-tan.z, tan.x), or the unit xz direction to o->112 +32 when
// o->76 == 3 and that is farther than c[1] (c = 0x003CCC50: 0, eps, 1, blend,
// dt), is blended into the body o->4's +128 row by c[3] and normalized with
// y = c[0]; the body's rotation rows (+96/+112/+128) and pos go through
// Mat34_FromRotPos553B4C into o->4 vfn[84](&m). Then t versus o->104 +- c[4]
// picks the forward (0xA76CE5BD), backward (0xA76CE5B7) or idle (0xE1C7FA75)
// animation; o->104 = t and o->16 = pos. f1 = pos.z.
DECOMP(003CCC68, Obj_SetSplinePos3CCC68) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 p = arg<u32>(ctx, 1);
    Stack frame(ctx, 352);
    const u32 sp = frame.addr() - 112;  // the original's r1: locals at their original offsets
    constexpr u32 C = 0x003CCC50u;
    const u32 pos = sp + 144, tan = sp + 160, rot = sp + 176, m = sp + 224;
    const f64 t = callf(ctx, 0x005927FCu, {load<u32>(o + 108), p, 0, 0}, {});  // SplineInstance_Nearest
    // r4 is left over from SplineInstance_Nearest (the callee does not read it)
    callf(ctx, 0x005A80D8u, {load<u32>(o + 108), ctx.gpr[4], pos, tan, 0}, {t});  // WorldObj_SplinePointTangentAt
    const f64 c0 = Lf(C);
    f64 a = -Lf(tan + 8), b = Lf(tan);
    store<u32>(tan + 4, 0);
    const u32 body = load<u32>(o + 4);
    if (static_cast<s32>(load<u32>(o + 76)) == 3) {
        const u32 q = load<u32>(o + 112) + 32;
        const f64 dy = S(Lf(q + 4) - Lf(o + 20)), dx = S(Lf(q) - Lf(o + 16)), dz = S(Lf(q + 8) - Lf(o + 24));
        const f64 len = Sqrts(Fmadds(dz, dz, Fmadds(dx, dx, S(dy * dy))));
        if (len > Lf(C + 4)) {
            const f64 inv = Divs(Lf(C + 8), len);
            a = S(dx * inv);
            b = S(dz * inv);
        }
    }
    const f64 k = Lf(C + 12), q0 = Lf(body + 128), q2 = Lf(body + 136);
    const f64 x = Fmadds(S(a - q0), k, q0), z = Fmadds(S(b - q2), k, q2);
    const f64 c2 = Lf(C + 8);
    const f64 inv = Divs(c2, Sqrts(Fmadds(z, z, Fmadds(x, x, S(c0 * c0)))));
    Sf(tan, S(z * inv));
    Sf(tan + 8, -S(x * inv));
    for (u32 r = 0; r < 3; ++r) CopyVec3(rot + 16 * r, body + 96 + 16 * r);
    call(ctx, 0x00553B4Cu, {m, rot, pos});  // Mat34_FromRotPos553B4C
    VCall(ctx, body, 84, {body, m});
    const f64 c4 = Lf(C + 16), prev = Lf(o + 104);
    const f64 c1 = Lf(C + 4);
    if (t > S(prev + c4)) AnimSwitch(ctx, o, 0xA76CE5BDu, ~0ull, 0, c2, c1, c0, c0);
    else if (t < S(prev - c4)) AnimSwitch(ctx, o, 0xA76CE5B7u, ~0ull, 0, c2, c1, c0, c0);
    else AnimSwitch(ctx, o, 0xE1C7FA75u, ~0ull, 0, c2, c1, c0, c0);
    Sf(o + 104, t);
    CopyVec3(o + 16, pos);
    ctx.fpr[1] = Lf(pos + 8);
}

namespace {
constexpr u32 Effect_Release = 0x004CF3C4u;        // (Effect*, u32 hash) -> bool
constexpr u32 Area_SpawnGrid2228C8 = 0x002228C8u;  // (Area*, Actor*)

// Releases the 3x3x3 effect slots o->428[27]: Effect_Release(e, 0x10CBB),
// e->136 |= 0x200, Area_RemoveEffect223640(o, e), slot = 0.
inline void ReleaseGrid(PPUContext& ctx, u32 o) {
    for (u32 k = 0; k < 27; ++k) {
        const u32 slot = o + 428 + 4 * k;
        const u32 e = load<u32>(slot);
        if (e == 0) continue;
        call(ctx, Effect_Release, {e, 0x10CBBu});
        store<u32>(e + 136, load<u32>(e + 136) | 0x200);
        call(ctx, 0x00223640u, {o, e});  // Area_RemoveEffect223640
        store<u32>(slot, 0);
    }
}

// The actor's area list x->252 (a PtrVector of 4-byte entries, created on
// first use: SmallAlloc(20), PtrVector_Construct, +6 = 4; a failed
// allocation leaves 0 and reads through it, quirk kept). Returns it; *r3 is
// what the original leaves in r3 when it allocated (4, or 0).
inline u32 AreaList(PPUContext& ctx, u32 x, bool* made, u32* r3) {
    u32 v = load<u32>(x + 252);
    *made = false;
    if (v == 0) {
        const u32 q = static_cast<u32>(call(ctx, 0x0055238Cu, {20}));  // SmallAlloc
        store<u32>(x + 252, q);
        *made = true;
        *r3 = 0;
        if (q != 0) {
            call(ctx, 0x005516E0u, {q});  // PtrVector_Construct
            store<u16>(q + 6, 4);
            *r3 = 4;
            v = load<u32>(x + 252);
        }
    }
    return v;
}

// The last entry (searching from the back) of the list v that is non-null,
// has the 0x20000000 flag in +192 and is not o; 0 when none.
inline u32 LastActiveArea(u32 v, u32 o) {
    for (s32 i = static_cast<s32>(load<u16>(v + 8)) - 1; i >= 0; --i) {
        const u32 e = load<u32>(load<u32>(v) + 4 * static_cast<u32>(i));
        if (e != 0 && (load<u32>(e + 192) & 0x20000000u) != 0 && e != o) return e;
    }
    return 0;
}
}  // namespace

// Area_EnterGrid223890 (Area* o, Actor* x) -> bool: Area_Enter(o, x); when it
// succeeds, the last other active area in x's list (x->252) has its effect
// grid released, Area_SpawnGrid2228C8(o, x) fills o's and o->672 = 1.
DECOMP(00223890, Area_EnterGrid223890) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 x = arg<u32>(ctx, 1);
    if (static_cast<s32>(call(ctx, 0x0020E7A4u, {o, x})) == 0) {  // Area_Enter
        ret(ctx, 0);
        return;
    }
    bool made;
    u32 r3;
    const u32 v = AreaList(ctx, x, &made, &r3);
    const u32 e = LastActiveArea(v, o);
    if (e != 0) ReleaseGrid(ctx, e);
    call(ctx, Area_SpawnGrid2228C8, {o, x});
    store<u8>(o + 672, 1);
    ret(ctx, 1);
}

// Area_LeaveGrid223A7C (Area* o, Actor* x): unless x is null (r3 = o):
// Area_Leave(o, x), o's effect grid released, o->672 = 0, and the last other
// active area in x's list gets Area_SpawnGrid2228C8(e, x) (its r3 returned).
// With none, r3 is what the original leaves (see below).
DECOMP(00223A7C, Area_LeaveGrid223A7C) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 x = arg<u32>(ctx, 1);
    if (x == 0) {
        ret(ctx, o);
        return;
    }
    call(ctx, 0x0020EBC8u, {o, x});  // Area_Leave
    ReleaseGrid(ctx, o);
    store<u8>(o + 672, 0);
    bool made;
    u32 r3 = 0;
    const u32 v = AreaList(ctx, x, &made, &r3);
    if (made) ctx.gpr[3] = r3;
    const s32 n = static_cast<s32>(load<u16>(v + 8)) - 1;
    const u32 e = LastActiveArea(v, o);
    if (e != 0) {
        call(ctx, Area_SpawnGrid2228C8, {e, x});
        return;
    }
    // no area: r3 is the search's byte offset (-4) after a non-empty list;
    // for an empty one it stays as left above (the last callee's r3, or the
    // allocation path's 4 / 0)
    if (n >= 0) ctx.gpr[3] = ~3ull;
}

namespace {
constexpr u32 kGameCtx = 0x00F65844u;  // global object: +80 / +84 counters, +576

// The state's stop: o->80 = 1, o->76 = 0, o->96 = 0, (o->94 = 0 when b94),
// *(o->112) = 0; r3 = 1.
inline void AnimStop(PPUContext& ctx, u32 o, bool b94) {
    const u32 p = load<u32>(o + 112);
    store<u32>(o + 80, 1);
    store<u32>(o + 76, 0);
    store<u32>(o + 96, 0);
    if (b94) store<u8>(o + 94, 0);
    store<u32>(p, 0);
    ret(ctx, 1);
}

// The start of the 0x1E7563B1 state: o->128 = 0x1E7563B1, g->576 = 0 and
// g->80 / g->84 counted when o is g + 112 / g + 256 (g = *0x00F65844).
inline void MarkStart(u32 o) {
    const u32 g = load<u32>(kGameCtx);
    store<u32>(o + 128, 0x1E7563B1u);
    store<u32>(g + 576, 0);
    if (o == g + 112) store<u32>(g + 80, load<u32>(g + 80) + 1);
    else if (o == g + 256) store<u32>(g + 84, load<u32>(g + 84) + 1);
}

// The finish test: done (AnimStop) when Anim_Find298874(Animator(o), 0) is
// set, o->128 is no longer 0x1E7563B1, or Anim_TrackSet36_0029879C(
// Animator(o), 0, 0) is set; else r3 stays as the last callee left it.
inline void AnimFinish(PPUContext& ctx, u32 o, bool b94) {
    if (static_cast<s32>(call(ctx, Anim_Find298874, {Animator(ctx, o), 0})) != 0 || load<u32>(o + 128) != 0x1E7563B1u ||
        static_cast<s32>(call(ctx, Anim_TrackSet36_0029879C, {Animator(ctx, o), 0, 0})) != 0)
        AnimStop(ctx, o, b94);
}
}  // namespace

// Obj_StateStart3CCA60 (Obj* o) (called by 003CF648): on the state's first
// tick (o->96 == c[0], c = 0x003CCA50) starts 0x1E7563B1 (Restart(Animator(o),
// ..., r7 = -1; f1..f4 = c[1], c[2], c[0], c[0])), MarkStart and, unless
// o->88, Effect_Spawn(0x3CF9D817, 0, 0, 1); then AnimFinish (no o->94).
DECOMP(003CCA60, Obj_StateStart3CCA60) {
    const u32 o = arg<u32>(ctx, 0);
    constexpr u32 C = 0x003CCA50u;
    const f64 c0 = Lf(C);
    ctx.fpr[1] = Lf(o + 96);  // f1 as the original leaves it (the callees may read it)
    if (Lf(o + 96) == c0) {
        const u32 a = Animator(ctx, o);
        // r8 is left over from the virtual call (the callee does not read it)
        Restart(ctx, a, 0x1E7563B1u, ~0ull, ctx.gpr[8], Lf(C + 4), Lf(C + 8), c0, c0);
        MarkStart(o);
        if (load<u32>(o + 88) == 0) call(ctx, 0x004A5FC8u, {0x3CF9D817u, 0, 0, 1});  // Effect_Spawn
    }
    AnimFinish(ctx, o, false);
}

// Obj_StateIdle3CFD10 (Obj* o) (called by 003D1AD8): on the first tick
// (o->96 == c[0], c = 0x003CFD00) starts 0xE1C691BF (r7 = 0, r8 = -1; f1..f4
// = c[1], c[2], c[0], c[0]) and sets o->128 to it; then AnimSwitch to the
// idle 0xE1C7FA75 with the same arguments.
DECOMP(003CFD10, Obj_StateIdle3CFD10) {
    const u32 o = arg<u32>(ctx, 0);
    constexpr u32 C = 0x003CFD00u;
    const f64 c0 = Lf(C);
    ctx.fpr[1] = Lf(o + 96);  // f1 as the original leaves it (the callees may read it)
    if (Lf(o + 96) == c0) {
        const u32 a = Animator(ctx, o);
        Restart(ctx, a, 0xE1C691BFu, 0, ~0ull, Lf(C + 4), Lf(C + 8), c0, c0);
        store<u32>(o + 128, 0xE1C691BFu);
    }
    AnimSwitch(ctx, o, 0xE1C7FA75u, 0, ~0ull, Lf(C + 4), Lf(C + 8), c0, c0);
}

// Obj_StateIdle3CFF28 (Obj* o) (called by 003D1AD8): Obj_StateIdle3CFD10 with
// 0x1E7563B1 as the first animation and c = 0x003CFF18.
DECOMP(003CFF28, Obj_StateIdle3CFF28) {
    const u32 o = arg<u32>(ctx, 0);
    constexpr u32 C = 0x003CFF18u;
    const f64 c0 = Lf(C);
    ctx.fpr[1] = Lf(o + 96);  // f1 as the original leaves it (the callees may read it)
    if (Lf(o + 96) == c0) {
        const u32 a = Animator(ctx, o);
        Restart(ctx, a, 0x1E7563B1u, 0, ~0ull, Lf(C + 4), Lf(C + 8), c0, c0);
        store<u32>(o + 128, 0x1E7563B1u);
    }
    AnimSwitch(ctx, o, 0xE1C7FA75u, 0, ~0ull, Lf(C + 4), Lf(C + 8), c0, c0);
}

// Obj_StateStart3D0158 (Obj* o) (called by 003D1AD8): on the first tick
// (o->96 == c[0], c = 0x003D0138): when o->12 > c[1] and o->8 > c[2], o->12 -=
// c[3] and o->8 -= c[4] (then 0.3 when below c[5]); starts 0x1E7563B1 (r7 =
// -1; f1..f4 = c[6], c[3], c[0], c[0]) and MarkStart; then AnimFinish (with
// o->94).
DECOMP(003D0158, Obj_StateStart3D0158) {
    const u32 o = arg<u32>(ctx, 0);
    constexpr u32 C = 0x003D0138u;
    const f64 c0 = Lf(C);
    ctx.fpr[1] = Lf(o + 96);  // f1 as the original leaves it (the callees may read it)
    const u32 body = load<u32>(o + 4);
    if (Lf(o + 96) == c0) {
        const f64 y = Lf(o + 12);
        if (y > Lf(C + 4)) {
            const f64 x = Lf(o + 8);
            if (x > Lf(C + 8)) {
                Sf(o + 12, S(y - Lf(C + 12)));
                const f64 nx = S(x - Lf(C + 16));
                Sf(o + 8, nx);
                if (nx < Lf(C + 20)) store<u32>(o + 8, 0x3E99999Au);
            }
        }
        const u32 a = static_cast<u32>(VCall(ctx, body, 60, {body}));
        // r8 is left over from the virtual call (the callee does not read it)
        Restart(ctx, a, 0x1E7563B1u, ~0ull, ctx.gpr[8], Lf(C + 24), Lf(C + 12), c0, c0);
        MarkStart(o);
    }
    AnimFinish(ctx, o, true);
}

// Obj_SlideTo40CFA0 (Obj* o; f32 dt) (called by 00410E98): o->88 += dt and u
// = min(o->88 / o->92, c[0]) (c = 0x0040CF90); o->96 = o->160 - o->192 *
// c[1] * (c[0] - u) (per axis), and the body o->4 gets the matrix
// Mat34_FromRotPos553B4C(its rotation rows, &o->96) through vfn[84]. On
// arrival (u == c[0]): o->84 = 3, o->320 and o->336 = o->304 (then o->340 =
// 4.3), o->88 = 0, body vfn[84](o + 208), and, when Animator(o) is set,
// Restart(Animator(o), 0x002237D4, r7 = -1, r8 = 0; f1..f4 = c[0], c[3],
// c[2], c[2]).
DECOMP(0040CFA0, Obj_SlideTo40CFA0) {
    const u32 o = arg<u32>(ctx, 0);
    const f64 dt = ctx.fpr[1];
    constexpr u32 C = 0x0040CF90u;
    const f64 tm = S(Lf(o + 88) + dt);
    f64 u = Divs(tm, Lf(o + 92));
    Sf(o + 88, tm);
    const f64 c0 = Lf(C);
    const u32 body = load<u32>(o + 4);
    const u32 b96 = load<u32>(body + 96);  // read before the o->96 stores, as the original does
    if (u >= c0) u = c0;
    const f64 r = S(c0 - u), k = Lf(C + 4);
    for (u32 i = 0; i < 3; ++i) Sf(o + 96 + 4 * i, Fnmsubs(S(Lf(o + 192 + 4 * i) * k), r, Lf(o + 160 + 4 * i)));
    Stack frame(ctx, 304);
    const u32 sp = frame.addr() - 112;  // the original's r1: locals at their original offsets
    const u32 rot = sp + 144, m = sp + 192;
    store<u32>(rot, b96);
    Cw(rot + 4, body + 100);
    Cw(rot + 8, body + 104);
    for (u32 i = 1; i < 3; ++i) CopyVec3(rot + 16 * i, body + 96 + 16 * i);
    ctx.fpr[1] = Lf(body + 136);  // f1 as the original leaves it
    call(ctx, 0x00553B4Cu, {m, rot, o + 96});  // Mat34_FromRotPos553B4C
    VCall(ctx, body, 84, {body, m});
    if (u != c0) return;
    store<u32>(o + 84, 3);
    Cw(o + 320, o + 304);
    Cw(o + 324, o + 308);
    const u32 b2 = load<u32>(o + 4);
    Cw(o + 328, o + 312);
    Cw(o + 336, o + 304);
    Cw(o + 340, o + 308);
    Cw(o + 344, o + 312);
    store<u32>(o + 340, 0x4089999Au);
    store<u32>(o + 88, 0);
    ctx.fpr[1] = Lf(o + 304);  // f1 as the original leaves it
    VCall(ctx, b2, 84, {b2, o + 208});
    if (Animator(ctx, o) == 0) return;
    const u32 a = Animator(ctx, o);
    Restart(ctx, a, 0x002237D4u, ~0ull, 0, c0, Lf(C + 12), Lf(C + 8), Lf(C + 8));
}

// Obj_StartJump12B908 (Obj* o, const Vec3* v) (called by
// Obj_DispatchKind_0012BB9C, Player_Dispatch_00488078): with the body b =
// o vfn[64]() (r3 = 0 when null): Body_SetMode(b, 2); b->672 = v, plus (when
// bit 0 of o->188 is set and the o->380 vfn[12]() record c has c->32 >=
// T(0x01039650)) c->16 * c->32 * T(0x010396A0); b->416 |= 0x00800000;
// o vfn[28](0, 1) and (1, 1); when T(0x01039600) > c[0] (c = 0x0012B900)
// o->20->48 |= 0x40000000, o->20->80 = that value, o->20->48 |= 0x00400000 |
// 0x100; the word at o->20 + o->6 (u16) |= 0x80000000; Restart(o vfn[60](),
// 0x0022EC2E, r7 = -1, r8 = 0; f1..f4 = c[1], c[0], c[0], c[0]). T(a) = the
// value at a + 72.
DECOMP(0012B908, Obj_StartJump12B908) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 v = arg<u32>(ctx, 1);
    const u32 b = static_cast<u32>(VCall(ctx, o, 64, {o}));
    if (b == 0) {
        ret(ctx, 0);
        return;
    }
    call(ctx, 0x00261328u, {b, 2});  // Body_SetMode
    f64 x = Lf(v), y = Lf(v + 4), z = Lf(v + 8);
    if ((load<u32>(o + 188) & 1u) != 0) {
        const u32 r = load<u32>(o + 380);
        const u32 c = static_cast<u32>(VCall(ctx, r, 12, {r}));
        const f64 s = Lf(c + 32);
        ctx.fpr[1] = s;  // f1 as the original leaves it
        if (s >= Lf(0x01039650u + 72)) {
            const f64 k = Lf(0x010396A0u + 72);
            x = Fmadds(S(Lf(c + 16) * s), k, x);
            y = Fmadds(S(Lf(c + 20) * s), k, y);
            ctx.fpr[1] = S(Lf(c + 24) * s);
            z = Fmadds(ctx.fpr[1], k, z);
        }
    }
    const u32 fl = load<u32>(b + 416);
    Sf(b + 672, x);
    Sf(b + 676, y);
    Sf(b + 680, z);
    store<u32>(b + 416, fl | 0x00800000u);
    VCall(ctx, o, 28, {o, 0, 1});
    VCall(ctx, o, 28, {o, 1, 1});
    constexpr u32 C = 0x0012B900u;
    constexpr u32 T3 = 0x01039600u + 72;
    const f64 c0 = Lf(C);
    ctx.fpr[1] = Lf(T3);  // f1 as the original leaves it
    if (Lf(T3) > c0) {
        u32 w = load<u32>(o + 20);
        store<u32>(w + 48, load<u32>(w + 48) | 0x40000000u);
        Cw(load<u32>(o + 20) + 80, T3);
        w = load<u32>(o + 20);
        store<u32>(w + 48, load<u32>(w + 48) | 0x00400000u);
        w = load<u32>(o + 20);
        store<u32>(w + 48, load<u32>(w + 48) | 0x100u);
    }
    const u32 p = load<u32>(o + 20) + load<u16>(o + 6);
    store<u32>(p, load<u32>(p) | 0x80000000u);
    const u32 a = static_cast<u32>(VCall(ctx, o, 60, {o}));
    Restart(ctx, a, 0x0022EC2Eu, ~0ull, 0, Lf(C + 4), c0, c0, c0);
}

namespace {
// The owner binding of a fresh effect e: its 0x0027469E component c (when
// vfn[32]() == 5) releases its old +28 (vfn[8]) and takes o->160 (whose u16
// +4 reference count is bumped).
inline void BindOwner(PPUContext& ctx, u32 e, u32 o) {
    const u32 c = static_cast<u32>(call(ctx, 0x004BB4B4u, {e, 0x0027469Eu}));  // Effect_FindComponent
    if (c == 0 || static_cast<s32>(VCall(ctx, c, 32, {c})) != 5) return;
    const u32 old = load<u32>(c + 28);
    const u32 p = load<u32>(o + 160);
    if (old != 0) VCall(ctx, old, 8, {old});
    store<u32>(c + 28, p);
    if (p != 0) store<u16>(p + 4, static_cast<u16>(load<u16>(p + 4) + 1));
}

// Unless o->348 <= c0 while o->692 is 0x03DE9A54, Restart(o vfn[60](),
// 0x663F99F0, r7, r8; f1..f4) and o->692 = 0x663F99F0.
inline void StartHurt(PPUContext& ctx, u32 o, f64 c0, u64 r7, u64 r8, f64 f1, f64 f2) {
    if (Lf(o + 348) <= c0 && load<u32>(o + 692) == 0x03DE9A54u) return;
    const u32 a = static_cast<u32>(VCall(ctx, o, 60, {o}));
    Restart(ctx, a, 0x663F99F0u, r7, r8, f1, f2, c0, c0);
    store<u32>(o + 692, 0x663F99F0u);
}
}  // namespace

// State_EnterHurt16E908 (State* s, Obj* o) (no direct caller): StartHurt
// (c = 0x0016E8F8; r7 = 0, r8 = -1; f1, f2 = c[1], c[2]); s->8 = 1, o->656 |=
// 0x02000000; unless s->4 already holds one (r3 = it), the effect
// Effect_New(0x24CC86BD) goes to s->4 (r3 = 0 when that fails), BindOwner,
// +136 bit 0x200 cleared and 0x400 set, Effect_SetTime(; c[3]).
DECOMP(0016E908, State_EnterHurt16E908) {
    const u32 s = arg<u32>(ctx, 0);
    const u32 o = arg<u32>(ctx, 1);
    constexpr u32 C = 0x0016E8F8u;
    ctx.fpr[1] = Lf(o + 348);  // f1 as the original leaves it
    StartHurt(ctx, o, Lf(C), 0, ~0ull, Lf(C + 4), Lf(C + 8));
    store<u32>(s + 8, 1);
    store<u32>(o + 656, load<u32>(o + 656) | 0x02000000u);
    const u32 cur = load<u32>(s + 4);
    if (cur != 0) {
        ret(ctx, cur);
        return;
    }
    const u32 e = static_cast<u32>(call(ctx, 0x004CFB24u, {0x24CC86BDu}));  // Effect_New
    store<u32>(s + 4, e);
    if (e == 0) {
        ret(ctx, 0);
        return;
    }
    BindOwner(ctx, e, o);
    u32 x = load<u32>(s + 4);
    store<u32>(x + 136, load<u32>(x + 136) & ~0x200u);
    x = load<u32>(s + 4);
    store<u32>(x + 136, load<u32>(x + 136) | 0x400u);
    callf(ctx, 0x004CF710u, {load<u32>(s + 4)}, {Lf(C + 12)});  // Effect_SetTime
}

// State_EnterHurt16D9E8 (State* s, Obj* o) (no direct caller): the hit count
// n = o->680 (s->24 = -1.0 when odd; s->8 = n, o->680 = n + 1, s->12 *= n *
// c[0] + c[1], c = 0x0016D9D0); StartHurt (r7 = -1, r8 = 0; f1, f2 = c[1],
// c[3]); unless s->4 already holds one (r3 = it), the effect
// Effect_New(0x480402DD) goes to s->4 (r3 = 0 when that fails) with +136 bit
// 0x200 cleared and 0x400 set, BindOwner, s vfn[16](o; c[2]) and
// Effect_SetTime(s->4; c[4]).
DECOMP(0016D9E8, State_EnterHurt16D9E8) {
    const u32 s = arg<u32>(ctx, 0);
    const u32 o = arg<u32>(ctx, 1);
    constexpr u32 C = 0x0016D9D0u;
    const u32 n = load<u32>(o + 680);
    const f64 sc = Lf(s + 12);
    ctx.fpr[1] = Lf(o + 348);  // f1 as the original leaves it
    if ((n & 1u) != 0) store<u32>(s + 24, 0xBF800000u);
    store<u32>(s + 8, n);
    store<u32>(o + 680, n + 1);
    Sf(s + 12, S(sc * Fmadds(S(static_cast<f64>(n)), Lf(C), Lf(C + 4))));
    const f64 c2 = Lf(C + 8);
    StartHurt(ctx, o, c2, ~0ull, 0, Lf(C + 4), Lf(C + 12));
    const u32 cur = load<u32>(s + 4);
    if (cur != 0) {
        ret(ctx, cur);
        return;
    }
    const u32 e = static_cast<u32>(call(ctx, 0x004CFB24u, {0x480402DDu}));  // Effect_New
    store<u32>(s + 4, e);
    if (e == 0) {
        ret(ctx, 0);
        return;
    }
    store<u32>(e + 136, load<u32>(e + 136) & ~0x200u);
    const u32 x = load<u32>(s + 4);
    store<u32>(x + 136, load<u32>(x + 136) | 0x400u);
    BindOwner(ctx, load<u32>(s + 4), o);
    ctx.fpr[1] = c2;
    VCall(ctx, s, 16, {s, o});
    callf(ctx, 0x004CF710u, {load<u32>(s + 4)}, {Lf(C + 16)});  // Effect_SetTime
}
