// World_HandleEvent callees, part 26b: the follower's catch-up step (nav
// target ahead of the owner, reach and camera-frustum tests).
#include <bit>
#include <cmath>

#include "hevent22.h"

using namespace sly4;
using namespace sly4::hevent;

namespace {
constexpr u32 kNavClosestPoint = 0x000147B8;  // (vec3* out, Object*, const vec3* target) (symbols.txt Nav_ClosestPoint147B8)
constexpr u32 kFollowCheck138B0 = 0x000138B0; // (Object*, const vec3* goal, const vec3* peer pos, Object* peer) -> u8 (name?)
constexpr u32 kSceneHandle = 0x00F557F0;      // u32: object whose +48 vec3 is the follow anchor (as symbols.txt World_ScenesFinish48B330)
constexpr u32 kWorldCamera = 0x128168;        // World +0x128168: Camera*, frustum planes at +368..+480
// Constants at 0x00018290: f32 0.5, f32 3.0, f64 3.0 (+8, f32 0.0 at +12),
// f32 2.0 (+16), f64 2.0 (+24), f64 0.0 (+32), f32 1.0 (+40).
constexpr u32 kConsts182C0 = 0x00018290;

// Line-of-sight ray cast (0x000138B0).
constexpr u32 kHkWorldSlot = 0x1281D4;        // World +0x1281D4: u32 p, p +4 = hkpWorld*
constexpr u32 kCastRay = 0x0092E510;          // (hkpWorld*, const RayInput*, Collector*) (Havok, name?)
constexpr u32 kCollectorVtable = 0x00F1DE80;  // all-hits ray collector vtable (0x00F1DE78 +8)
constexpr u32 kHkAllocator = 0x011DF31C;      // Havok allocator object, vt +20 = free(p, bytes)
constexpr u32 kHitSize = 80, kInlineHits = 8;
constexpr u32 kPropTypeKey = 0xA734004Du, kPropTypeGame = 0x7A30D7E3u, kPropOwnerKey = 0x91C7CF5Bu;

inline f32 lf(u32 ea) { return std::bit_cast<f32>(load<u32>(ea)); }
inline f64 lfd(u32 ea) { return std::bit_cast<f64>(load<u64>(ea)); }
inline f32 fmadds(f64 a, f64 c, f64 b) { return static_cast<f32>(std::fma(a, c, b)); }

// One SoA plane set (x / y / z / d rows of 4 planes at cam +off, +off+16,
// +off+32, +off+48): all four 1 + ((d + a x) + b y) + c z >= 0, each step
// fused in single precision as vmaddfp.
bool planes_pass(u32 cam, u32 off, f32 x, f32 y, f32 z, f32 one) {
    bool all = true;
    for (u32 i = 0; i < 4; ++i) {
        const u32 e = ((cam + off) & ~15u) + 4 * i;
        f32 v = std::fma(lf(e), x, lf(((cam + off + 48) & ~15u) + 4 * i));
        v = std::fma(lf(((cam + off + 16) & ~15u) + 4 * i), y, v);
        v = std::fma(lf(((cam + off + 32) & ~15u) + 4 * i), z, v);
        v = one + v;
        if (!(v >= 0.0f)) all = false;
    }
    return all;
}
}  // namespace

// ---------------------------------------------------------------- 0x000182C0
// void Follow_Update182C0(Follower* s, f32 dt) (name?): for an owner (+0)
// whose +256 peer has +172 bit 18 set: target = anchor (*0x00F557F0 +48) +
// s +32 vec3 * speed (+48), snapped by Nav_ClosestPoint147B8 to p; reach
// radius r = max(speed / 2, 3) by fsel. +56 = (|p.y - t.y| < r && xz
// distance^2 < r^2). When reached: with the owner's +256 peer, goal = p +
// (owner +48 object vt +80 position - owner +176) and +58 =
// Follow_Check138B0(owner, goal, peer vt +80 position, peer), else +58 = 0;
// with +58 set, +57 = p inside both camera plane sets. Reached, +58 and
// +57: +16 = p and timer +52 += dt, otherwise +52 -= dt; +52 clamped to
// [0, 2] by fsel.
DECOMP(000182C0, Follow_Update182C0) {
    const u32 s = arg<u32>(ctx, 0);
    const f64 dt = farg(ctx, 0);
    u32 owner = load<u32>(s);
    if (owner == 0) return;  // r3 = s
    {
        const u32 peer = load<u32>(owner + 256);
        if (peer == 0 || ((load<u32>(peer + 172) >> 18) & 1u) == 0) {
            ret(ctx, 0u);
            return;
        }
    }
    const u32 K = kConsts182C0;
    const f32 speed = lf(s + 48);
    const f32 half = fmuls(speed, lf(K + 0));
    const f32 slack = static_cast<f32>(static_cast<f64>(half) - static_cast<f64>(lf(K + 4)));
    const u32 anchor = load<u32>(kSceneHandle) + 48;
    const f32 t0 = fmadds(lf(s + 32), speed, lf(anchor + 0));
    const f32 t1 = fmadds(lf(s + 36), speed, lf(anchor + 4));
    const f32 t2 = fmadds(lf(s + 40), speed, lf(anchor + 8));
    const f32 radius = static_cast<f32>(static_cast<f64>(slack) >= 0.0 ? static_cast<f64>(half) : lfd(K + 8));

    // Stack as the original's: target sp+112, p sp+128, positions sp+144 /
    // sp+176, goal sp+160 (splat scratch at sp+192 stays in registers here).
    Stack frame(ctx, 96);
    const u32 sp = frame.addr();
    const u32 tgt = sp + 0, pt = sp + 16, posA = sp + 32, goal = sp + 48, posB = sp + 64;
    store<f32>(tgt + 0, t0);
    store<f32>(tgt + 4, t1);
    store<f32>(tgt + 8, t2);
    call(ctx, kNavClosestPoint, {pt, owner, tgt});
    u32 px = load<u32>(pt + 0), py = load<u32>(pt + 4), pz = load<u32>(pt + 8);
    owner = load<u32>(s);

    u32 reached = 0;
    const f32 dy = std::fabs(static_cast<f32>(static_cast<f64>(std::bit_cast<f32>(py)) - static_cast<f64>(lf(tgt + 4))));
    if (dy < radius) {
        const f32 dx = static_cast<f32>(static_cast<f64>(std::bit_cast<f32>(px)) - static_cast<f64>(lf(tgt + 0)));
        const f32 dz = static_cast<f32>(static_cast<f64>(std::bit_cast<f32>(pz)) - static_cast<f64>(lf(tgt + 8)));
        const f32 r2 = fmuls(radius, radius);
        const f32 k0 = lf(K + 12);
        const f32 d2 = fmadds(dz, dz, fmadds(dx, dx, fmuls(k0, k0)));
        reached = d2 < r2 ? 1u : 0u;
        store<u8>(s + 56, static_cast<u8>(reached));
        if (reached != 0) {
            const u32 peer = load<u32>(owner + 256);
            u32 ok = 0;
            if (peer != 0) {
                const u32 x = load<u32>(owner + 48);
                vcall(ctx, x, 80, {posA, x});
                for (u32 k = 0; k < 12; k += 4) {
                    const f32 d = static_cast<f32>(static_cast<f64>(lf(posA + k)) - static_cast<f64>(lf(owner + 176 + k)));
                    store<f32>(goal + k, static_cast<f32>(static_cast<f64>(lf(pt + k)) + static_cast<f64>(d)));
                }
                vcall(ctx, peer, 80, {posB, peer});
                ok = static_cast<u32>(call(ctx, kFollowCheck138B0, {owner, goal, posB, peer}));
                px = load<u32>(pt + 0);
                py = load<u32>(pt + 4);
                pz = load<u32>(pt + 8);
                reached = load<u8>(s + 56);
            }
            store<u8>(s + 58, static_cast<u8>(ok));
        }
    } else {
        store<u8>(s + 56, 0);
    }

    f64 timer;
    bool hold = false;
    if (reached != 0) {
        if (load<u8>(s + 58) != 0) {
            const u32 cam = load<u32>(load<u32>(kCurrentWorld) + kWorldCamera);
            const f32 x = std::bit_cast<f32>(px), y = std::bit_cast<f32>(py), z = std::bit_cast<f32>(pz);
            const f32 one = lf(K + 40);
            const bool a = planes_pass(cam, 368, x, y, z, one);
            const bool in = a && planes_pass(cam, 432, x, y, z, one);
            store<u8>(s + 57, in ? 1 : 0);
        }
        hold = load<u8>(s + 58) != 0 && load<u8>(s + 57) != 0;
    }
    if (hold) {
        const f32 t = lf(s + 52);
        store<u32>(s + 16, px);
        store<u32>(s + 20, py);
        timer = static_cast<f32>(static_cast<f64>(t) + dt);
        store<u32>(s + 24, pz);
    } else {
        timer = static_cast<f32>(static_cast<f64>(lf(s + 52)) - dt);
    }
    const f32 room = static_cast<f32>(static_cast<f64>(lf(K + 16)) - timer);
    f64 v = static_cast<f32>(static_cast<f64>(room) >= 0.0 ? timer : lfd(K + 24));
    v = static_cast<f32>(v >= 0.0 ? v : lfd(K + 32));
    store<f32>(s + 52, static_cast<f32>(v));
    ctx.fpr[1] = v;
    ret(ctx, K);
}

// ---------------------------------------------------------------- 0x000138B0
namespace {
// Havok entity property lookup (entity +124 16-byte {u32 key, pad, u64
// value} array, +128 s32 count): the low word of key's value, or 0.
u32 prop_low(u32 ent, u32 key) {
    for (s32 i = 0; i < static_cast<s32>(load<u32>(ent + 128)); ++i) {
        const u32 e = load<u32>(ent + 124) + (static_cast<u32>(i) << 4);
        if (load<u32>(e) == key) return load<u32>(e + 12);
    }
    return 0;
}
// The inlined hkArray destructor of the collector's hits (bit 31 of the
// capacity word: storage not owned).
void free_hits(PPUContext& ctx, u32 col) {
    const u32 cap = load<u32>(col + 24);
    if ((cap & 0x80000000u) != 0) return;
    const u32 n = cap & 0x3FFFFFFFu;
    const s64 bytes = static_cast<s32>((n << 6) + (n << 4));
    store<u32>(col + 20, 0);
    store<u32>(col + 0, kCollectorVtable);
    const u32 opd = load<u32>(load<u32>(kHkAllocator) + 20);
    const u32 data = load<u32>(col + 16);
    ctx.gpr[6] = load<u32>(opd);  // as the original sequence: entry in r6, OPD in r7
    ctx.gpr[7] = opd;
    call_ptr(ctx, opd, {kHkAllocator, data, static_cast<u64>(bytes)});
}
}  // namespace

// bool Follow_LineOfSight138B0(Object* o, const vec3* from, const vec3* to,
// Object* peer) (name?): 0 when peer +188 bit 4 is set; otherwise casts a
// ray from -> to in the world's hkpWorld with an all-hits collector (8 hits
// inline on the stack). Each hit's collidable (+68) not filtered out by o
// vt +288 (collidable +28 & 31): its owner (kinds 1 / 2 at +24, entity =
// collidable + s8 +16) must carry the game tag property, and the owner
// object property (else 0); hits on o +48, on peer or on objects with +244
// set are skipped, any other object blocks (0). 1 when nothing blocked.
DECOMP(000138B0, Follow_LineOfSight138B0) {
    const u32 o = arg<u32>(ctx, 0), from = arg<u32>(ctx, 1), to = arg<u32>(ctx, 2), peer = arg<u32>(ctx, 3);
    if ((load<u32>(peer + 188) & 0x10u) != 0) {
        ret(ctx, 0u);
        return;
    }
    // Stack as the original's from sp+112: ray input (+0 from, +16 to, +32
    // u8, +36 / +40 u32), collector at +48 (sp+160: vtable, +4 early-out
    // fraction, +16 hits, +20 size, +24 capacity | 0x80000000), hits at +80.
    Stack frame(ctx, 720);
    const u32 in = frame.addr(), col = in + 48, hits = in + 80;
    store<u8>(in + 32, 0);
    store<u32>(in + 40, 0);
    for (u32 k = 0; k < 2; ++k) {  // (x, y, z, 0): splats merged by the lane masks at 0x00013880
        const u32 src = k == 0 ? from : to, dst = in + 16 * k;
        for (u32 j = 0; j < 3; ++j) store<u32>(dst + 4 * j, load<u32>(src + 4 * j));
        store<u32>(dst + 12, 0);
    }
    store<u32>(in + 36, 0);
    store<u32>(col + 4, 0x3F800000u);  // 1.0f
    store<u32>(col + 0, kCollectorVtable);
    store<u32>(col + 16, hits);
    store<u32>(col + 20, 0);
    store<u32>(col + 24, 0x80000000u | kInlineHits);
    for (u32 i = 0; i < kInlineHits; ++i) {
        const u32 h = hits + kHitSize * i;
        store<u32>(h + 16, 0x3F800000u);
        store<u32>(h + 20, 0xFFFFFFFFu);
        store<u32>(h + 64, 0);
        store<u32>(h + 32, 0xFFFFFFFFu);
        store<u32>(h + 68, 0);
    }
    store<u32>(col + 20, 0);
    store<u32>(col + 4, 0x3F800000u);
    const u32 hkworld = load<u32>(load<u32>(load<u32>(kCurrentWorld) + kHkWorldSlot) + 4);
    call(ctx, kCastRay, {hkworld, in, col});

    for (s32 i = 0; i < static_cast<s32>(load<u32>(col + 20)); ++i) {
        const u32 coll = load<u32>(load<u32>(col + 16) + kHitSize * static_cast<u32>(i) + 68);
        const u32 opd = load<u32>(load<u32>(o) + 288);
        const u32 layer = load<u32>(coll + 28) & 31u;
        ctx.gpr[5] = load<u32>(opd);  // as the original sequence: entry in r5
        if (static_cast<u32>(call_ptr(ctx, opd, {o, layer})) != 0) continue;
        const s32 kind = static_cast<s8>(load<u8>(coll + 24));
        u32 ent = 0;
        if (kind == 1 || kind == 2) ent = coll + static_cast<u32>(static_cast<s32>(static_cast<s8>(load<u8>(coll + 16))));
        u32 obj = 0;
        if (ent != 0 && prop_low(ent, kPropTypeKey) == kPropTypeGame && static_cast<s32>(load<u32>(ent + 128)) > 0)
            obj = prop_low(ent, kPropOwnerKey);
        if (obj == 0) {
            free_hits(ctx, col);
            ret(ctx, 0u);
            return;
        }
        if (obj == load<u32>(o + 48) || obj == peer || load<u32>(obj + 244) != 0) continue;
        free_hits(ctx, col);
        ret(ctx, 0u);
        return;
    }
    free_hits(ctx, col);
    ret(ctx, 1u);
}

// ---------------------------------------------------------------- 0x0012C968
namespace {
constexpr u32 kCoachTimeFlags = 0x0012A71C;   // (Obj*) -> u32 (symbols.txt Coach_TimeFlags12A71C)
constexpr u32 kObjPosition = 0x0026A540;      // (f32 out[3], Obj*) (symbols.txt Obj_Position26A540)
constexpr u32 kObjRefPos = 0x0026A42C;        // (f32 out[3], Obj*) (symbols.txt Obj_RefPos26A42C)
constexpr u32 kSplineFindNear = 0x005A8CF8;   // (pos, f32* t, f32* dist, filter OPD, Query*) -> SplineInstance* (symbols.txt SplineInstance_FindNearest)
constexpr u32 kSplinesCollectNear = 0x005A90F8;  // (PtrVector*, pos, -, filter OPD, Query*; f32 max) (symbols.txt SplineInstances_CollectNear)
constexpr u32 kSplinesFindNearest = 0x005A8F60;  // (PtrVector*, pos, f32* t, f32* dist, filter, arg) -> s32 (symbols.txt Splines_FindNearest)
constexpr u32 kSplineGetMode = 0x005A8BA8;    // (inst) -> u8 (symbols.txt SplineInstance_GetMode)
constexpr u32 kSetPhysicsActive = 0x00260C14; // (Body*, u8 on) (symbols.txt Obj_SetPhysicsActive)
constexpr u32 kBodySetMode = 0x00261328;      // (Body*, u32 mode) (symbols.txt Body_SetMode)
constexpr u32 kPtrVecConstruct = 0x005516E0, kPtrVecClear = 0x0054EC1C, kNopFn = 0x0054EC18;
constexpr u32 kSplineFilter = 0x00F32FC8;     // OPD of Spline_Filter19FB9C
constexpr u32 kGrindFilter = 0x00F325E0;      // OPD given to SplineInstances_CollectNear (name?)
constexpr u32 kSplineMaxDist = 0x0103CC18;    // f32 (0x0103CBD0 +72, as hevent15_b.cpp)
constexpr u32 kSnapFlag = 0x01038A20;         // u8 copied into the spline query (+36) (name?)
constexpr u32 kGrindMasks = 0x00AC8C20;       // u32[2] {0xF44C, 0x0BB0}: time flag groups searched for splines
constexpr u32 kTZero = 0x0012C964;            // f32 0.0
constexpr u32 kZeroVec3 = 0x01164B40;
constexpr u32 kVtBody = 64, kVtSet = 28, kVtQueryOwner = 12;
constexpr u32 kTimeOnSpline = 0x10000;
}  // namespace

// u32 Coach_FindSpline12C968(Object* c, -, u32 mask, f32 radius) (name?):
// needs a body (vt +64) and a +380 object (its vt +12 gives the query
// owner Q). flags = mask & Coach_TimeFlags12A71C(c). With bit 16: the
// spline instance nearest to (position - N * ref.y) (N = c +112) within
// *0x0103CC18 (query {pos, Q +16 vec3, max, *0x01038A20}) snaps the body on
// (vt +28 (1, 10), (2, 0), physics on, body mode 4, record = c +20 + u16 c
// +6: +24 instance, +36 top byte = its mode and bit 23 cleared, +28 = t).
// Otherwise for each mask group {0xF44C, 0x0BB0} present in flags: the
// splines within radius of c +144 (query {pos, radius, c, group, 0}) ->
// nearest one closer than radius: body mode 2 (when it has a body), vt +28
// (0, 1), (1, 2), record +96 = instance, +104 = t (0 for modes 3 / 8 /
// 11), +108 = 0, +48 = the zero vec3.
DECOMP(0012C968, Coach_FindSpline12C968) {
    const u32 c = arg<u32>(ctx, 0), mask = arg<u32>(ctx, 2);
    const f64 radius = farg(ctx, 0);
    const u32 body = static_cast<u32>(vcall(ctx, c, kVtBody, {c}));
    const u32 holder = load<u32>(c + 380);
    if (body == 0 || holder == 0) return ret(ctx, body);
    const u32 owner = static_cast<u32>(vcall(ctx, holder, kVtQueryOwner, {holder}));
    const u32 flags = mask & static_cast<u32>(call(ctx, kCoachTimeFlags, {c}));

    // Stack as the original's from sp+112: position +0, ref +16, snap pos
    // +32, search pos +48, search query +64 (sp+176), snap query +96
    // (sp+208), t / dist +144 / +148, search t / dist +152 / +156, PtrVector
    // +160 (sp+272).
    Stack frame(ctx, 184);
    const u32 sb = frame.addr();
    const u32 pos = sb, ref = sb + 16, spos = sb + 32, lpos = sb + 48, q2 = sb + 64, q = sb + 96;
    const u32 t = sb + 144, dist = sb + 148, t2 = sb + 152, d2 = sb + 156, vec = sb + 160;

    if ((flags & kTimeOnSpline) != 0) {
        call(ctx, kObjPosition, {pos, body});
        call(ctx, kObjRefPos, {ref, body});
        const f64 h = lf(ref + 4);
        const f32 sx = static_cast<f32>(-std::fma(static_cast<f64>(lf(c + 112)), h, -static_cast<f64>(lf(pos + 0))));
        const f32 sy = static_cast<f32>(-std::fma(static_cast<f64>(lf(c + 116)), h, -static_cast<f64>(lf(pos + 4))));
        const f32 sz = static_cast<f32>(-std::fma(static_cast<f64>(lf(c + 120)), h, -static_cast<f64>(lf(pos + 8))));
        store<f32>(spos + 0, sx);
        store<f32>(spos + 4, sy);
        store<f32>(q + 0, sx);
        store<f32>(spos + 8, sz);
        store<f32>(q + 4, sy);
        store<f32>(q + 8, sz);
        for (u32 k = 0; k < 12; k += 4) store<u32>(q + 16 + k, load<u32>(owner + 16 + k));
        store<u8>(q + 36, load<u8>(kSnapFlag));
        store<u32>(q + 32, load<u32>(kSplineMaxDist));
        ctx.fpr[1] = lf(owner + 24);  // f1 as the original's copies leave it
        const u32 inst = static_cast<u32>(call(ctx, kSplineFindNear, {spos, t, dist, kSplineFilter, q}));
        if (inst != 0) {
            ctx.fpr[1] = lf(dist);
            if (static_cast<f64>(lf(dist)) <= static_cast<f64>(lf(kSplineMaxDist))) {
                vcall(ctx, c, kVtSet, {c, 1, 10});
                vcall(ctx, c, kVtSet, {c, 2, 0});
                const u32 b2 = static_cast<u32>(vcall(ctx, c, kVtBody, {c}));
                call(ctx, kSetPhysicsActive, {b2, 1});
                call(ctx, kBodySetMode, {b2, 4});
                const u32 rec = load<u32>(c + 20) + load<u16>(c + 6);
                store<u32>(rec + 24, inst);
                const u32 mode = static_cast<u32>(call(ctx, kSplineGetMode, {inst}));
                const u32 old = load<u32>(rec + 36);
                store<u8>(rec + 36, static_cast<u8>(mode));
                store<u32>(rec + 36, (old & 0x007FFFFFu) | (mode << 24));
                ctx.fpr[1] = lf(t);
                store<u32>(rec + 28, load<u32>(t));
                return;  // r3 = the mode
            }
        }
    }

    u64 r3 = 0;
    for (u32 i = 0; i < 2; ++i) {
        r3 = 4 * i;
        const u32 group = load<u32>(kGrindMasks + 4 * i) & flags;
        if (group == 0) continue;
        call(ctx, kPtrVecConstruct, {vec});
        store<u32>(lpos + 0, load<u32>(c + 144));
        store<u16>(vec + 6, 4);
        store<u32>(lpos + 4, load<u32>(c + 148));
        store<u32>(q2 + 0, load<u32>(c + 144));
        store<u32>(lpos + 8, load<u32>(c + 152));
        store<u32>(q2 + 4, load<u32>(c + 148));
        store<u32>(q2 + 8, load<u32>(c + 152));
        store<f32>(q2 + 16, static_cast<f32>(radius));
        store<u32>(q2 + 20, c);
        store<u32>(q2 + 24, group);
        store<u32>(q2 + 28, 0);
        callf(ctx, kSplinesCollectNear, {vec, lpos, ctx.gpr[5], kGrindFilter, q2}, {radius});
        const s32 idx = static_cast<s32>(call(ctx, kSplinesFindNearest, {vec, lpos, t2, d2, 0, 0}));
        if (idx >= 0) {
            ctx.fpr[1] = lf(d2);
            if (static_cast<f64>(lf(d2)) < radius) {
                const u32 spl = load<u32>(load<u32>(vec) + (static_cast<u32>(idx) << 2));
                if (spl != 0) {
                    u32 tb = load<u32>(t2);
                    if (static_cast<u32>(vcall(ctx, c, kVtBody, {c})) != 0) {
                        const u32 b2 = static_cast<u32>(vcall(ctx, c, kVtBody, {c}));
                        call(ctx, kBodySetMode, {b2, 2});
                    }
                    vcall(ctx, c, kVtSet, {c, 0, 1});
                    vcall(ctx, c, kVtSet, {c, 1, 2});
                    const u32 rec = load<u32>(c + 20) + load<u16>(c + 6);
                    const s32 mode = static_cast<s32>(call(ctx, kSplineGetMode, {spl}));
                    store<u32>(rec + 96, spl);
                    if (mode == 3 || mode == 8 || mode == 11) tb = load<u32>(kTZero);
                    store<u32>(rec + 104, tb);
                    store<u32>(rec + 108, 0);
                    for (u32 k = 0; k < 12; k += 4) store<u32>(rec + 48 + k, load<u32>(kZeroVec3 + k));
                    ctx.fpr[1] = lf(kZeroVec3 + 8);
                    call(ctx, kPtrVecClear, {vec});
                    call(ctx, kNopFn, {vec});
                    return;  // r3 as Nop leaves it
                }
            }
        }
        call(ctx, kPtrVecClear, {vec});
        r3 = call(ctx, kNopFn, {vec});
    }
    ret(ctx, r3);
}

// ---------------------------------------------------------------- 0x004A54F8
namespace {
constexpr u32 kUpInit = 0x010C26C8;           // u8: kUp set up
constexpr u32 kUp = 0x010C26D0;               // vec3 {0, 1, 0}, set on first use
constexpr u32 kAxisDefault = 0x01164B50;      // vec3 used when the old row 0 is not finite
constexpr u32 kConsts4A54F8 = 0x004A54E8;     // f32 1.0, 1e-5, -1.0
inline f32 fmsubs(f64 a, f64 c, f64 b) { return static_cast<f32>(std::fma(a, c, -b)); }
// 1 / sqrt(x*x + y*y + z*z) in the original's order (first, second, third
// term = the fmuls one, then the two fmadds).
inline f32 inv_len(f32 a, f32 b, f32 c, f32 one) {
    const f32 l2 = fmadds(c, c, fmadds(b, b, fmuls(a, a)));
    const f32 l = static_cast<f32>(std::sqrt(static_cast<f64>(l2)));
    return static_cast<f32>(static_cast<f64>(one) / static_cast<f64>(l));
}
}  // namespace

// void Matrix_FromDir4A54F8(const vec3* d, Matrix* m) (name?): rows +0 /
// +16 / +32 of a 3x4 matrix with +32 = d. Unless d is (anti)parallel to up
// {0, 1, 0} (|dot| within 1e-5 of 1): row 0 = normalize(up x d), row 1 =
// normalize(d x row 0). Otherwise from the old row 0 (or 0x01164B50 when
// it is not finite): row 1 = normalize(d x old), row 0 = normalize(row 1 x
// d).
DECOMP(004A54F8, Matrix_FromDir4A54F8) {
    const u32 d = arg<u32>(ctx, 0), m = arg<u32>(ctx, 1);
    if (load<u8>(kUpInit) == 0) {
        store<u32>(kUp + 0, 0);
        store<u8>(kUpInit, 1);
        store<u32>(kUp + 4, 0x3F800000u);
        store<u32>(kUp + 8, 0);
    }
    const f32 dx = lf(d + 0), dy = lf(d + 4), dz = lf(d + 8);
    const f32 ux = lf(kUp + 0), uy = lf(kUp + 4), uz = lf(kUp + 8);
    const f32 one = lf(kConsts4A54F8), eps = lf(kConsts4A54F8 + 4);
    const f32 dot = fmadds(dz, uz, fmadds(dx, ux, fmuls(dy, uy)));
    const bool par = std::fabs(static_cast<f32>(static_cast<f64>(std::fabs(dot)) - static_cast<f64>(one))) < eps ||
                     std::fabs(static_cast<f32>(static_cast<f64>(dot) - static_cast<f64>(lf(kConsts4A54F8 + 8)))) < eps;
    if (!par) {
        const f32 cy = fmsubs(uz, dx, fmuls(ux, dz));
        const f32 cx = fmsubs(uy, dz, fmuls(uz, dy));
        const f32 cz = fmsubs(ux, dy, fmuls(uy, dx));
        const f32 k = static_cast<f32>(std::sqrt(static_cast<f64>(fmadds(cz, cz, fmadds(cx, cx, fmuls(cy, cy))))));
        const f32 r = static_cast<f32>(static_cast<f64>(one) / static_cast<f64>(k));
        store<f32>(m + 8, fmuls(cz, r));
        store<f32>(m + 0, fmuls(cx, r));
        store<f32>(m + 4, fmuls(cy, r));
        const f32 x0 = lf(m + 0), x1 = lf(m + 4), x2 = lf(m + 8);
        const f32 yy = fmsubs(dz, x0, fmuls(dx, x2));
        const f32 yz = fmsubs(dx, x1, fmuls(dy, x0));
        const f32 yx = fmsubs(dy, x2, fmuls(dz, x1));
        const f32 r2 = inv_len(yy, yx, yz, one);
        store<f32>(m + 16, fmuls(yx, r2));
        store<f32>(m + 20, fmuls(yy, r2));
        store<f32>(m + 24, fmuls(yz, r2));
        for (u32 k2 = 0; k2 < 12; k2 += 4) store<u32>(m + 32 + k2, load<u32>(d + k2));
        ctx.fpr[1] = lf(d + 8);
        return;
    }
    u32 ref = m;
    for (u32 k = 0; k < 12; k += 4) {
        const f32 v = lf(m + k);
        if (!(std::fabs(static_cast<f32>(static_cast<f64>(v) - static_cast<f64>(v))) < eps)) {
            ref = kAxisDefault;
            break;
        }
    }
    const f32 rx = lf(ref + 0), ry = lf(ref + 4), rz = lf(ref + 8);
    const f32 yy = fmsubs(dz, rx, fmuls(dx, rz));
    const f32 yz = fmsubs(dx, ry, fmuls(dy, rx));
    const f32 yx = fmsubs(dy, rz, fmuls(dz, ry));
    const f32 r = inv_len(yy, yx, yz, one);
    store<f32>(m + 16, fmuls(yx, r));
    store<f32>(m + 20, fmuls(yy, r));
    store<f32>(m + 24, fmuls(yz, r));
    for (u32 k = 0; k < 12; k += 4) store<u32>(m + 32 + k, load<u32>(d + k));
    const f32 Dx = lf(m + 32), Dy = lf(m + 36), Dz = lf(m + 40);
    const f32 Yx = lf(m + 16), Yy = lf(m + 20), Yz = lf(m + 24);
    const f32 xy = fmsubs(Yz, Dx, fmuls(Yx, Dz));
    const f32 xx = fmsubs(Yy, Dz, fmuls(Yz, Dy));
    const f32 xz = fmsubs(Yx, Dy, fmuls(Yy, Dx));
    const f32 r2 = inv_len(xy, xx, xz, one);
    const f32 ox = fmuls(xx, r2);
    store<f32>(m + 0, ox);
    store<f32>(m + 4, fmuls(xy, r2));
    store<f32>(m + 8, fmuls(xz, r2));
    ctx.fpr[1] = ox;
}
