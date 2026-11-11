// Game code, large-function sweep part 6: an object's physics bodies hard
// keyframed to its animated attachment matrices (000E4720).
#include "big6.h"

using namespace sly4;
using namespace sly4::big6;

namespace {
// Object fields.
constexpr u32 kObjMat = 96;         // f32 [4][4]: its matrix (12 words used)
constexpr u32 kObjWorldObj = 160;   // WorldObj* (its matrix at +80, its key at +204), or 0
constexpr u32 kObjSkel = 216;       // skeleton: +192 -> {+0 bones (160 bytes each, u32 flags at +124)}
constexpr u32 kObjEffect = 356;     // Effect*
constexpr u32 kObjFlags376 = 376;   // 0x20000: effect pending, 0x80000: look the WorldObj's effect up
constexpr u32 kObjBodies = 388;     // {+0 elements (48 bytes each, body at +28), +8 u16 count} or 0
constexpr u32 kObjState = 392;      // bit 0: matrices relative to its own matrix; >> 1: state
constexpr u32 kObjFlags396 = 396;   // 0x80000000: do not keyframe
constexpr u32 kStateKeyframed = 7;
// Body (hkpRigidBody): +8 world, +240 its transform (4 rows of 4 f32).
constexpr u32 kBodyWorld = 8;
constexpr u32 kBodyXform = 240;

constexpr u32 kConst = 0x000E46E0u;  // .text: f32 1e-5 (min dt), f32 1.0, lane masks 1 / 2 / 3 at +16 / +32 / +48
constexpr u32 kIdentity = 0x01164BD0u;  // the identity matrix
constexpr u32 kEffectMap = 0x01013B1Cu;  // SortedMap: WorldObj key (+204) -> effect record
constexpr u32 kEffectHash = 0x04B6FB5Bu;

constexpr u32 kObjUpdateBase = 0x000D83C4u;    // (o, f1 dt) (name?)
constexpr u32 kAttachMatrix = 0x000E3534u;     // (o, f32 out[4][4], element, const f32 base[4][4]) (name?)
constexpr u32 kQuatFromMatrix = 0x00555478u;   // (f32 q[4], const f32 m[4][4])
constexpr u32 kApplyHardKeyFrame = 0x009DD41Cu;  // (hkVector4* pos, hkQuaternion* rot, f1 invDt, hkpRigidBody*) (Havok)
constexpr u32 kEffectSetHash = 0x004CF988u;    // (Effect*, u32 hash)
constexpr u32 kEffectAddArgs = 0x0049FCC0u;    // (Effect*, ArgVector*, 0, 0, 0)
constexpr u32 kSortedMapFind = 0x00732F38u;    // (map, const u32* key) -> slot

f32 F(u32 a) { return load<f32>(a); }
}  // namespace

// void Obj_KeyframeBodies(Obj* o, f32 dt)
// In the keyframed state ((+392 >> 1) == 7) with a WorldObj (+160): (bit 0 of +392)
// first copies the WorldObj's matrix (+80) to +96 when it differs; then, when
// dt >= 1e-5 and the object has bodies, each body element whose bone has
// flag 0x80000000 and whose body is in a world gets its target matrix from
// 0x000E3534 (relative to +96 for bit 0, else to the WorldObj's matrix or the
// identity); when that differs from the body's transform (+240), its
// normalised rotation and translation go to Havok's applyHardKeyFrame with
// 1/dt (unless +396 has 0x80000000). Then a pending effect (+376 0x20000;
// with 0x80000 and a WorldObj, its effect record found by key +204
// adds its args first) gets hash 0x04B6FB5B and the flag is cleared; ends
// with 0x000D83C4(o, dt), whose r3 / f1 are left.
DECOMP(000E4720, Obj_KeyframeBodies) {
    const u32 o = arg<u32>(ctx, 0);
    const f64 dt = ctx.fpr[1];
    Stack frame(ctx, 336);
    const u32 sp = frame.addr();
    const u32 st = load<u32>(o + kObjState);
    const u32 bodies0 = load<u32>(o + kObjBodies);
    if ((st >> 1) == kStateKeyframed) {
        const u32 wo = load<u32>(o + kObjWorldObj);
        const u32 skel = load<u32>(o + kObjSkel);
        if (wo != 0) {
            if ((st & 1) != 0 && !Rows12Equal(o + kObjMat, wo + 80)) {
                for (u32 r = 0; r < 4; ++r)
                    for (u32 k = 0; k < 3; ++k) CopyW(o + kObjMat + 16 * r + 4 * k, wo + 80 + 16 * r + 4 * k);
            }
            if (dt >= static_cast<f64>(F(kConst)) && bodies0 != 0 && load<u16>(bodies0 + 8) != 0) {
                const f32 one = F(kConst + 4);
                const f32 invDt = static_cast<f32>(static_cast<f64>(one) / dt);
                const V4 m1 = Lvx(kConst + 16), m2 = Lvx(kConst + 32), m3 = Lvx(kConst + 48);
                const u32 q = sp + 112, pos = sp + 128, rot = sp + 144, mat = sp + 160;
                for (u32 i = 0;; ++i) {
                    const u32 bodies = load<u32>(o + kObjBodies);
                    const u32 n = bodies != 0 ? load<u16>(bodies + 8) : 0;
                    if (i >= n) break;
                    const u32 bone = load<u32>(load<u32>(skel + 192)) + 160 * i;
                    if ((load<u32>(bone + 124) & 0x80000000u) == 0) continue;
                    const u32 el = load<u32>(bodies) + 48 * i;
                    const u32 body = load<u32>(el + 28);
                    if (body == 0 || load<u32>(body + kBodyWorld) == 0) continue;
                    u32 base;
                    if ((load<u32>(o + kObjState) & 1) != 0) {
                        base = o + kObjMat;
                    } else {
                        const u32 p = load<u32>(o + kObjWorldObj);
                        base = p != 0 ? p + 80 : kIdentity;
                    }
                    call(ctx, kAttachMatrix, {o, mat, el, base});
                    if (Rows12Equal(body + kBodyXform, mat)) continue;
                    call(ctx, kQuatFromMatrix, {q, mat});
                    const f32 x = F(q), y = F(q + 4), z = F(q + 8), w = F(q + 12);
                    f32 len = static_cast<f32>(y * y);
                    len = std::fma(x, x, len);
                    len = std::fma(z, z, len);
                    len = std::fma(w, w, len);
                    len = static_cast<f32>(std::sqrt(static_cast<f64>(len)));
                    const f32 s = static_cast<f32>(static_cast<f64>(one) / static_cast<f64>(len));
                    const f32 qs[4] = {x * s, y * s, z * s, w * s};
                    for (u32 k = 0; k < 4; ++k) store<f32>(q + 4 * k, qs[k]);
                    if ((load<u32>(o + kObjFlags396) & 0x80000000u) != 0) continue;
                    // pos = (m[3].x, m[3].y, m[3].z, 0) and rot = q, built with
                    // vsel on the lane masks (raw words).
                    const u32 t[3] = {load<u32>(mat + 48), load<u32>(mat + 52), load<u32>(mat + 56)};
                    for (u32 k = 0; k < 4; ++k) {
                        const u32 l1 = std::bit_cast<u32>(m1.e[k]), l2 = std::bit_cast<u32>(m2.e[k]), l3 = std::bit_cast<u32>(m3.e[k]);
                        u32 v = (t[0] & ~l1) | (t[1] & l1);
                        v = (v & ~l2) | (t[2] & l2);
                        store<u32>(pos + 4 * k, v & ~l3);
                        u32 r = (std::bit_cast<u32>(qs[0]) & ~l1) | (std::bit_cast<u32>(qs[1]) & l1);
                        r = (r & ~l2) | (std::bit_cast<u32>(qs[2]) & l2);
                        store<u32>(rot + 4 * k, (r & ~l3) | (std::bit_cast<u32>(qs[3]) & l3));
                    }
                    // r5 = 144 is what the original's stvx index leaves there (the callee ignores it).
                    callf(ctx, kApplyHardKeyFrame, {pos, rot, 144, body}, {invDt});
                }
            }
        }
    }
    u32 f376 = load<u32>(o + kObjFlags376);
    if ((f376 & 0x20000u) != 0) {
        if (load<u32>(o + kObjEffect) != 0) {
            if ((f376 & 0x80000u) != 0) {
                const u32 wo = load<u32>(o + kObjWorldObj);
                if (wo != 0) {
                    const u32 key = sp + 288;
                    store<u32>(key, load<u32>(wo + 204));
                    const u32 rec = load<u32>(static_cast<u32>(call(ctx, kSortedMapFind, {kEffectMap, key})));
                    if (rec != 0 && (load<u32>(rec + 4) & 2) != 0)
                        call(ctx, kEffectAddArgs, {load<u32>(o + kObjEffect), rec + 180, 0, 0, 0});
                }
            }
            call(ctx, kEffectSetHash, {load<u32>(o + kObjEffect), kEffectHash});
            f376 = load<u32>(o + kObjFlags376);
        }
        store<u32>(o + kObjFlags376, f376 & ~0x20000u);
    }
    callf(ctx, kObjUpdateBase, {o}, {dt});
}

namespace {
// 000E5000: object fields beyond the ones above.
constexpr u32 kObjComps = 164;      // component list (Obj_FindComp34CC34)
constexpr u32 kObjFlags172 = 172;   // 0x800: matrix taken from the WorldObj
constexpr u32 kObjAnimTime = 404;   // f32: the state's length
constexpr u32 kObjStateTime = 408;  // f32: time in the state
constexpr u32 kObjFlags420 = 420;   // 0x80000000: leave the state when done
constexpr u32 kVfnObjGetShapeArg = 76;   // (o) -> u32 (name?)
constexpr u32 kVfnObjSetState = 416;     // (o, u32 state, 0) (name?)
constexpr u32 kVfnWoSkeleton = 28;       // WorldObj: (wo) -> Skeleton* (name?)
// Skeleton: +192 bones, +208 tag, +224 f32[3] shape offset (y at +228), +300 flags.
constexpr u32 kSkelTag = 0xA970EC1Fu;
// Body element (48 bytes): +16 a WorldObj it drives, +20 s32 node index (-1: none), +28 body, +32 volume.

constexpr u32 kConst5 = 0x000E4FA0u;  // .text: f32 0.5, f32 0, vperm masks at +16 / +32 / +48 / +64 / +80
constexpr u32 kVolumeGlobal = 0x00F557ECu;  // u32: the volume system
constexpr u32 kZeroVec3 = 0x01164B40u;

constexpr u32 kObjCanUpdate = 0x000E0734u;   // (o, f1 dt) -> bool (name?)
constexpr u32 kFindComp = 0x0034CC34u;       // (list, 0) -> component
constexpr u32 kVolumeNotify = 0x000112D8u;   // (system, volume)
constexpr u32 kShapeInitBody = 0x00011410u;  // (Shape* out, system, body, 0, u32, u16 mask) (name?)
constexpr u32 kShapeInit = 0x000110ECu;      // (Shape* out, system, const f32 m[4][3]..., const f32 off[3], u32, u16 mask)
constexpr u32 kVolumeSetShape = 0x00637D14u; // (volume, Shape*)
constexpr u32 kShapeRelease = 0x00638498u;   // (Shape*)
constexpr u32 kMat43Copy = 0x000E4E80u;      // (dst, src)
constexpr u32 kMatMultiply = 0x005541F0u;    // (Matrix* a, const Matrix* b): a = a x b
constexpr u32 kMarkDirty = 0x0057D000u;      // (WorldObj*)
constexpr u32 kUpdateLod = 0x0057EE00u;      // (WorldObj*, 0, 0)
constexpr u32 kSkeletonUpdate = 0x0057E8D0u; // (Skeleton*)

void SetFWords(PPUContext& ctx, const u32* w, u32 n) {
    for (u32 k = 0; k < n; ++k) ctx.fpr[1 + k] = std::bit_cast<f64>(LfsBits(w[k]));
}
// The x, y, z words of rows 0..3 of a 4x4 matrix.
void Rows43(u32 src, u32* w) {
    for (u32 r = 0; r < 4; ++r)
        for (u32 k = 0; k < 3; ++k) w[3 * r + k] = load<u32>(src + 16 * r + 4 * k);
}
void StoreRows43(u32 dst, const u32* w) {
    for (u32 r = 0; r < 4; ++r)
        for (u32 k = 0; k < 3; ++k) store<u32>(dst + 16 * r + 4 * k, w[3 * r + k]);
}

// 0x000E5430: the rigid inverse of the matrix at b (sp+304, via lvlx /
// vmrghw / vperm; the lane taken from sp+416 ends up only in a lane that is
// dropped) stored as 4 rows at out (sp+368).
void InverseLvlx(u32 b, u32 out, const V4& m1, const V4& m2, const V4& m3) {
    const V4 zero{{0.0f, 0.0f, 0.0f, 0.0f}};
    const V4 r0 = Lvlx(b), r1 = Lvlx(b + 16), r2 = Lvlx(b + 32), r3 = Lvlx(b + 48);
    const V4 a356 = Lvlx(b + 52), a360 = Lvlx(b + 56), old = Lvx(out + 48);
    const V4 a324 = Lvlx(b + 20), a308 = Lvlx(b + 4), a312 = Lvlx(b + 8), a340 = Lvlx(b + 36);
    const V4 a328 = Lvlx(b + 24), a344 = Lvlx(b + 40);
    V4 t = Perm(r3, a356, m1);
    t = Perm(t, Perm(a360, old, m1), m2);
    t = Sub4(zero, t);
    const V4 c0 = Perm(Mrghw(r0, r1), Mrghw(r2, a324), m3);
    const V4 c1 = Perm(Mrghw(a308, a324), Mrghw(a340, r0), m3);
    const V4 c2 = Perm(Mrghw(a312, a328), Mrghw(a344, a312), m3);
    Stvx(out, c0);
    Stvx(out + 16, c1);
    Stvx(out + 32, c2);
    Stvx(out + 48, Row3(c0, c1, c2, t.e[0], t.e[1], t.e[2]));
}
}  // namespace

// u32 Obj_SyncBodyShapes(Obj* o, f32 dt)
// Returns 0 when 0x000E0734(o, dt) says no. With a WorldObj and bodies:
// pass 1 over the body elements: an element whose bone is flagged
// (0x80000000 of +124) and whose body exists gets its volume (+32) a new
// shape (when the object has the 0x02000000 component and the skeleton /
// bone carry tag 0xA970EC1F): from the body (skeleton offset +224 zero) or
// from the object's matrix with y offset +228 * 0.5; an unflagged element
// with a live body (not kind 5, in a world) and a driven WorldObj (+16) or
// a node (+20) copies the body's transform to the WorldObj's local matrix
// (+80, clearing 0x800 of +164), or to the node's target (+4) as
// inverse(the node's body) x transform. If any did, pass 2 puts each
// driven WorldObj's local matrix (times its parent +260's inverse) in its
// world matrix (+16) and marks it dirty, then the object's WorldObj is
// updated (LOD; matrix to +96 for bit 0 of +392, setting 0x800 of +172;
// marked dirty; its skeleton flagged 0x80 at +31 and updated). Finally the
// state time +408 += dt: states 7 / 1 end (vfn[416](o, 3, 0)) once it
// reaches +404 (when that is >= 0), state 5 calls vfn[416](o, 6, 0),
// state 6 returns 0; flag 0x80000000 of +420 is cleared with
// vfn[416](o, 3, 0). Returns 1. f1 at exit: the last float loaded (+408 /
// +404), as the original leaves it.
DECOMP(000E5000, Obj_SyncBodyShapes) {
    const u32 o = arg<u32>(ctx, 0);
    const f64 dt = ctx.fpr[1];
    Stack frame(ctx, 848);
    const u32 sp = frame.addr();
    const u32 skel = load<u32>(o + kObjSkel);
    callf(ctx, kObjCanUpdate, {o}, {dt});
    if (static_cast<u32>(ctx.gpr[3]) == 0) {
        ctx.gpr[3] = 0;
        return;
    }
    const u32 b0 = load<u32>(o + kObjBodies);
    if (load<u32>(o + kObjWorldObj) != 0 && b0 != 0 && load<u16>(b0 + 8) != 0) {
        const f32 half = F(kConst5);
        const V4 m1 = Lvx(kConst5 + 16), m2 = Lvx(kConst5 + 32), m3 = Lvx(kConst5 + 48);
        bool did = false;
        u32 lst;
        for (u32 i = 0;; ++i) {
            lst = load<u32>(o + kObjBodies);
            const u32 n = lst != 0 ? load<u16>(lst + 8) : 0;
            if (i >= n) break;
            const u32 el = load<u32>(lst) + 48 * i;
            const u32 bone = load<u32>(load<u32>(skel + 192)) + 160 * i;
            const u32 body = load<u32>(el + 28);
            if ((load<u32>(bone + 124) & 0x80000000u) != 0) {
                if (body == 0) continue;
                const u32 comps = load<u32>(o + kObjComps);
                if (comps == 0) continue;
                const u32 comp = static_cast<u32>(call(ctx, kFindComp, {comps, 0}));
                u32 vol = load<u32>(kVolumeGlobal);
                if (comp == 0 || (load<u32>(comp + 12) & 0x02000000u) == 0) continue;
                if (load<u32>(skel + 208) != kSkelTag) continue;
                const u32 btag = load<u32>(bone + 144);
                if (btag != 0 && btag != kSkelTag) continue;
                const u32 mask = (load<u32>(skel + 300) & 0x01000000u) != 0 ? 0xFFFFu : 0xFFFEu;
                call(ctx, kVolumeNotify, {vol, el + 32});
                const bool zeroOff = load<u64>(skel + 224) == load<u64>(kZeroVec3) && load<u32>(skel + 232) == load<u32>(kZeroVec3 + 8);
                vol = load<u32>(kVolumeGlobal);
                u32 w[12];
                Rows43(o + kObjMat, w);
                SetFWords(ctx, w, 12);
                if (zeroOff) {
                    const u32 shape = sp + 496;
                    const u32 r = static_cast<u32>(VCall(ctx, o, kVfnObjGetShapeArg, {o}));
                    call(ctx, kShapeInitBody, {shape, vol, body, 0, r, mask});
                    call(ctx, kVolumeSetShape, {el + 32, shape});
                    call(ctx, kShapeRelease, {shape});
                } else {
                    const u32 m = sp + 176, shape = sp + 500;
                    for (u32 k = 0; k < 12; ++k)
                        if (k != 10) store<u32>(m + 16 * (k / 3) + 4 * (k % 3), w[k]);
                    const f32 y = std::fma(F(skel + 228), half, std::bit_cast<f32>(w[10]));
                    store<f32>(m + 52, y);
                    ctx.fpr[1] = static_cast<f64>(y);
                    const u32 r = static_cast<u32>(VCall(ctx, o, kVfnObjGetShapeArg, {o}));
                    call(ctx, kShapeInit, {shape, vol, m, skel + 224, r, mask});
                    call(ctx, kVolumeSetShape, {el + 32, shape});
                    call(ctx, kShapeRelease, {shape});
                }
                continue;
            }
            if (body == 0 || load<u8>(body + 232) == 5 || load<u32>(body + 8) == 0) continue;
            if (load<s32>(el + 20) < 0 && load<u32>(el + 16) == 0) continue;
            did = true;
            const u32 xf = sp + 112;
            call(ctx, kMat43Copy, {xf, body + 240});
            const s32 node = load<s32>(el + 20);
            if (node < 0) {
                // To the driven WorldObj's local matrix; 0x800 of +164 cleared
                // (its word loaded before, stored between, the row-3 stores).
                const u32 d = load<u32>(el + 16);
                u32 w[12];
                Rows43(xf, w);
                for (u32 k = 0; k < 9; ++k) store<u32>(d + 80 + 16 * (k / 3) + 4 * (k % 3), w[k]);
                const u32 f164 = load<u32>(d + 164);
                store<u32>(d + 128, w[9]);
                store<u32>(d + 132, w[10]);
                store<u32>(d + 164, f164 & ~0x800u);
                store<u32>(d + 136, w[11]);
                ctx.fpr[1] = std::bit_cast<f64>(LfsBits(w[11]));
                continue;
            }
            // The node: (WorldObj skeleton +32)[node] when there is one, else
            // a null node pointer (quirk, kept: its first word is read from
            // address 0).
            u32 nd = 0;
            if (const u32 wo = load<u32>(o + kObjWorldObj); wo != 0) {
                const u32 sk = static_cast<u32>(VCall(ctx, wo, kVfnWoSkeleton, {wo}));
                if (sk != 0) nd = load<u32>(sk + 32) + (static_cast<u32>(node) << 4);
            }
            const s32 idx = static_cast<s16>(load<u16>(load<u32>(nd) + 100));
            if (idx == -1) continue;
            const u32 el2 = load<u32>(load<u32>(o + kObjBodies)) + 48 * static_cast<u32>(idx);
            const u32 body2 = load<u32>(el2 + 28);
            const u32 a = sp + 240, b = sp + 304, res = sp + 368;
            call(ctx, kMat43Copy, {a, body + 240});
            call(ctx, kMat43Copy, {b, body2 + 240});
            InverseLvlx(b, res, m1, m2, m3);
            SetFFromWord(ctx, 1, b + 52);  // the (dead) row copies' last loads
            SetFFromWord(ctx, 2, b + 56);
            call(ctx, kMatMultiply, {res, a});
            const u32 tgt = load<u32>(nd + 4);
            u32 w[12];
            Rows43(res, w);
            StoreRows43(tgt, w);
            ctx.fpr[1] = std::bit_cast<f64>(LfsBits(w[11]));
        }
        if (did) {
            const V4 p1 = Lvx(kConst5 + 64), p2 = Lvx(kConst5 + 80);
            for (u32 j = 0;; ++j) {
                const u32 n = lst != 0 ? load<u16>(lst + 8) : 0;
                if (j >= n) break;
                const u32 el = load<u32>(lst) + 48 * j;
                const u32 body = load<u32>(el + 28);
                if (body == 0 || load<u8>(body + 232) == 5 || load<u32>(body + 8) == 0) continue;
                if (load<s32>(el + 20) >= 0 || load<u32>(el + 16) == 0) continue;
                const u32 d = load<u32>(el + 16);
                u32 w[12];
                Rows43(d + 80, w);
                if (const u32 par = load<u32>(d + 260); par != 0) {
                    // w = local x inverse(parent +80): the 0x000D7A00 pattern.
                    const V4 zero{{0.0f, 0.0f, 0.0f, 0.0f}};
                    const u32 pm = par + 80;
                    const V4 r0 = Lvx(pm), r1 = Lvx(pm + 16), r2 = Lvx(pm + 32);
                    const V4 t = Sub4(zero, Lvx(pm + 48));
                    const V4 h = Mrghw(r0, r2), lo = Mrglw(r0, r2);
                    const V4 c0 = Mrghw(h, r1), c1 = Perm(h, r1, p1), c2 = Perm(lo, r1, p2);
                    const V4 tinv = Row3(c0, c1, c2, t.e[0], t.e[1], t.e[2]);
                    const u32 res = sp + 432;
                    for (u32 r = 0; r < 4; ++r) {
                        V4 v = Row3(c0, c1, c2, std::bit_cast<f32>(w[3 * r]), std::bit_cast<f32>(w[3 * r + 1]), std::bit_cast<f32>(w[3 * r + 2]));
                        if (r == 3) v = Add4(tinv, v);
                        Stvx(res + 16 * r, v);
                    }
                    Rows43(res, w);
                }
                for (u32 r = 0; r < 4; ++r)
                    for (u32 k = 0; k < 3; ++k) store<u32>(d + 16 + 16 * r + 4 * k, w[3 * r + k]);
                SetFWords(ctx, w, 12);
                call(ctx, kMarkDirty, {d});
                lst = load<u32>(o + kObjBodies);
            }
            if (const u32 wo = load<u32>(o + kObjWorldObj); wo != 0) {
                call(ctx, kUpdateLod, {wo, 0, 0});
                if ((load<u32>(o + kObjState) & 1) != 0) {
                    const u32 wo2 = load<u32>(o + kObjWorldObj);
                    if (!Rows12Equal(o + kObjMat, wo2 + 80)) {
                        u32 w[12];
                        Rows43(wo2 + 80, w);
                        StoreRows43(o + kObjMat, w);
                        ctx.fpr[1] = std::bit_cast<f64>(LfsBits(w[11]));
                        // (A copy of the matrix to wo2 +16 plus WorldObj_MarkDirty
                        // follows in the original behind a compare of zero with
                        // itself: never taken.)
                        if (wo2 != 0) store<u32>(o + kObjFlags172, load<u32>(o + kObjFlags172) | 0x800u);
                    }
                }
            }
            if (const u32 wo = load<u32>(o + kObjWorldObj); wo != 0) {
                call(ctx, kMarkDirty, {wo});
                const u32 wa = load<u32>(o + kObjWorldObj);
                if (static_cast<u32>(VCall(ctx, wa, kVfnWoSkeleton, {wa})) != 0) {
                    const u32 wb = load<u32>(o + kObjWorldObj);
                    const u32 sk = static_cast<u32>(VCall(ctx, wb, kVfnWoSkeleton, {wb}));
                    store<u8>(sk + 31, load<u8>(sk + 31) | 0x80);
                    const u32 wc = load<u32>(o + kObjWorldObj);
                    call(ctx, kSkeletonUpdate, {static_cast<u32>(VCall(ctx, wc, kVfnWoSkeleton, {wc}))});
                }
            }
        }
    }
    // State time.
    const u32 w408 = load<u32>(o + kObjStateTime);
    ctx.fpr[1] = std::bit_cast<f64>(LfsBits(w408));
    const f32 t = static_cast<f32>(ctx.fpr[1] + dt);
    const u32 st = load<u32>(o + kObjState) >> 1;
    store<f32>(o + kObjStateTime, t);
    if (st == 6) {
        ctx.gpr[3] = 0;
        return;
    }
    if (st == 7 || st == 1) {
        const u32 w404 = load<u32>(o + kObjAnimTime);
        ctx.fpr[1] = std::bit_cast<f64>(LfsBits(w404));
        SetFFromWord(ctx, 2, kConst5 + 4);
        const f64 len = ctx.fpr[1];
        if (len >= ctx.fpr[2] && static_cast<f64>(t) >= len) VCall(ctx, o, kVfnObjSetState, {o, 3, 0});
    } else if (st == 5) {
        VCall(ctx, o, kVfnObjSetState, {o, 6, 0});
    }
    const u32 f420 = load<u32>(o + kObjFlags420);
    if ((f420 & 0x80000000u) != 0) {
        store<u32>(o + kObjFlags420, f420 & 0x7FFFFFFFu);
        VCall(ctx, o, kVfnObjSetState, {o, 3, 0});
    }
    ctx.gpr[3] = 1;
}
