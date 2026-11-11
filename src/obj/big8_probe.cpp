// Game code, large-function sweep part 8: a line-of-sight test through the
// Havok world (one ray, or four around the line at a radius) from a source
// point picked by mode to a target object's point picked by mode (002C2780).
#include "big8.h"

using namespace sly4;
using namespace sly4::big8;
using namespace sly4::big8::b8fn;

namespace {
constexpr u32 kConst = 0x002C2740u;   // .text: f32 0 (radius threshold), 0.01 (min length^2), 1.0; lane masks y / z / w at +16 / +32 / +48
constexpr u32 kSrcObj = 276;          // source: its object (vfuncs +80 / +284 -> position; +144 position)
constexpr u32 kSrcAttach = 172;       // source: attachment (+48 offset f32[3], +68 s16 bone) or 0
constexpr u32 kVfnPosA = 80;          // (f32 out[3], obj): position
constexpr u32 kVfnPosB = 284;         // (f32 out[3], obj): another position (aim point?)
constexpr u32 kObjPos = 144;          // f32[3]
constexpr u32 kHitSize = 80;          // hkpWorldRayCastOutput: +68 hkpCollidable*
constexpr u32 kHitInline = 8;
constexpr u32 kFilterInfo = 6;

struct Pt {
    u32 w[3];
};
inline f32 Fw(u32 w) { return std::bit_cast<f32>(w); }
inline u32 Wf(f32 f) { return std::bit_cast<u32>(f); }
Pt Words(u32 a) { return Pt{{load<u32>(a), load<u32>(a + 4), load<u32>(a + 8)}}; }

// Position of obj into the frame buffer at buf via vfunc slot (out, obj).
Pt VPos(PPUContext& ctx, u32 obj, u32 slot, u32 buf) {
    VCall(ctx, obj, slot, {buf, obj});  // OPD in r5, entry in r6
    return Words(buf);
}

// The allocator frees the collector's heap hit array (capacity without the
// inline flag 0x80000000), after resetting it.
void FreeHits(PPUContext& ctx, u32 coll, u32 hits, u32 cap, bool opdFirst) {
    store<u32>(coll + 20, 0);
    store<u32>(coll, kCollectorVt);
    const u32 opd = load<u32>(load<u32>(kHeapAlloc) + 20);
    const u64 size = Sx((cap & 0x3FFFFFFFu) * kHitSize);
    if (opdFirst) call_ptr(ctx, opd, {kHeapAlloc, hits, size, opd, load<u32>(opd)});  // OPD in r6, entry in r7
    else call_ptr(ctx, opd, {kHeapAlloc, hits, size, load<u32>(opd), opd});           // entry in r6, OPD in r7
}
}  // namespace

// bool Obj_LineOfSight(Src* src, Obj* target, s32 srcMode, s32 dstMode, f32 radius)
// From the source point (mode 1: src object's vfunc +80; 2: its vfunc +284;
// 3: the attachment's offset through its bone matrix (or vfunc +284 without
// one); 4: the world camera's position (or vfunc +284 without one); else
// its +144) to the target's point (1: vfunc +80, 2: vfunc +284, else +144):
// with radius > 0, four rays from the corners of a square of half-size
// radius around the source point facing the target (built on the identity's
// up row), else one ray. Each ray (filter info 6, all hits, sorted) must
// first hit an entity whose game object (property 0x91C7CF5B) is target;
// the first ray that does not ends the test. True when every ray passed.
// False without a target or when the points are closer than sqrt(0.01).
DECOMP(002C2780, Obj_LineOfSight) {
    const u32 src = arg<u32>(ctx, 0);
    const u32 target = arg<u32>(ctx, 1);
    const s32 srcMode = static_cast<s32>(arg<u32>(ctx, 2));
    const s32 dstMode = static_cast<s32>(arg<u32>(ctx, 3));
    const f64 radius = ctx.fpr[1];
    if (target == 0) {
        ret(ctx, 0u);
        return;
    }
    const u32 world = load<u32>(kWorldPtr);
    const u32 hk = load<u32>(load<u32>(world + kWorldPhys) + 4);
    Stack frame(ctx, 992);  // the original's sp+112..+1103
    const u32 sp = frame.addr() - 112;
    Pt a;
    if (srcMode == 1) {
        a = VPos(ctx, load<u32>(src + kSrcObj), kVfnPosA, sp + 112);
    } else if (srcMode == 2) {
        a = VPos(ctx, load<u32>(src + kSrcObj), kVfnPosB, sp + 112);
    } else if (srcMode == 3) {
        const u32 at = load<u32>(src + kSrcAttach);
        if (at == 0) {
            a = VPos(ctx, load<u32>(src + kSrcObj), kVfnPosB, sp + 128);
        } else {
            const u64 bone = Sx(static_cast<u32>(static_cast<s32>(static_cast<s16>(load<u16>(at + 68)))));
            const f32 ox = Fw(LaneW(Lvlx(at + 48), 0));
            const f32 oy = Fw(LaneW(Lvlx(at + 52), 0));
            const f32 oz = Fw(LaneW(Lvlx(at + 56), 0));
            CopyW(sp + 336, at + 48);
            CopyW(sp + 340, at + 52);
            CopyW(sp + 344, at + 56);
            SetF1Bits(ctx, load<u32>(at + 56));
            call(ctx, kObjBoneMatrix, {sp + 352, load<u32>(src + kSrcObj), bone, 1, 0});
            const V4 p = Add4(Lvx(sp + 400), Row3(Lvx(sp + 352), Lvx(sp + 368), Lvx(sp + 384), ox, oy, oz));
            Stvx(sp + 336, p);
            a = Words(sp + 336);
        }
    } else if (srcMode == 4) {
        const u32 cam = load<u32>(world + kWorldCamera);
        if (cam == 0) a = VPos(ctx, load<u32>(src + kSrcObj), kVfnPosB, sp + 144);
        else a = Words(cam + kObjPos);
    } else {
        a = Words(load<u32>(src + kSrcObj) + kObjPos);
    }
    Pt b;
    if (dstMode == 1) b = VPos(ctx, target, kVfnPosA, sp + 160);
    else if (dstMode == 2) b = VPos(ctx, target, kVfnPosB, sp + 160);
    else b = Words(target + kObjPos);

    Pt pts[4] = {a, Words(kZero3), Words(kZero3), Words(kZero3)};
    u32 n = 1;
    SetF1Bits(ctx, load<u32>(kConst));
    if (radius > static_cast<f64>(F(kConst))) {
        const f32 ax = Fw(a.w[0]), ay = Fw(a.w[1]), az = Fw(a.w[2]);
        f32 dy = Fw(b.w[1]) - ay;
        f32 dx = Fw(b.w[0]) - ax;
        f32 dz = Fw(b.w[2]) - az;
        ctx.fpr[1] = dx;
        const f32 len2 = Fmadds(dz, dz, Fmadds(dx, dx, dy * dy));
        if (len2 <= F(kConst + 4)) {
            ret(ctx, 0u);
            return;
        }
        const f32 one = F(kConst + 8);
        n = 4;
        f32 inv = one / std::sqrt(len2);
        dz *= inv;
        dx *= inv;
        dy *= inv;
        const f32 ux = F(kIdentity + 16), uz = F(kIdentity + 24), uy = F(kIdentity + 20);
        // s = up x d, normalised
        f32 sy = Fmsubs(uz, dx, ux * dz);
        f32 sz = Fmsubs(ux, dy, uy * dx);
        f32 sx = Fmsubs(uy, dz, uz * dy);
        inv = one / std::sqrt(Fmadds(sz, sz, Fmadds(sx, sx, sy * sy)));
        sx *= inv;
        sy *= inv;
        sz *= inv;
        // t = d x s
        const f32 tz = Fmsubs(dx, sy, dy * sx);
        const f32 tx = Fmsubs(dy, sz, dz * sy);
        const f32 ty = Fmsubs(dz, sx, dx * sz);
        const f32 nx = -sx, ny = -sy, nz = -sz;
        const f32 p0z = Fmadds(sz + tz, radius, az);
        const f32 p0x = Fmadds(sx + tx, radius, ax);
        const f32 p0y = Fmadds(sy + ty, radius, ay);
        const f32 p1x = Fmadds(sx - tx, radius, ax);
        const f32 p1y = Fmadds(sy - ty, radius, ay);
        const f32 p1zb = Fmadds(sz - tz, radius, az);
        const f32 p2x = Fmadds(nx - tx, radius, ax);
        const f32 p2y = Fmadds(ny - ty, radius, ay);
        const f32 p2z = Fmadds(nz - tz, radius, az);
        const f32 p3x = Fmadds(nx + tx, radius, ax);
        const f32 p3y = Fmadds(ny + ty, radius, ay);
        const f32 p3z = Fmadds(nz + tz, radius, az);
        pts[0] = Pt{{Wf(p0x), Wf(p0y), Wf(p0z)}};
        pts[1] = Pt{{Wf(p1x), Wf(p1y), Wf(p1zb)}};
        pts[2] = Pt{{Wf(p2x), Wf(p2y), Wf(p2z)}};
        pts[3] = Pt{{Wf(p3x), Wf(p3y), Wf(p3z)}};
        ctx.fpr[1] = p3z;
    }

    const V4 my = Lvx(kConst + 16), mz = Lvx(kConst + 32), mw = Lvx(kConst + 48);
    auto vec = [&](const Pt& p) { return Andc(Sel(Sel(Splat1(Fw(p.w[0])), Splat1(Fw(p.w[1])), my), Splat1(Fw(p.w[2])), mz), mw); };
    // hkpWorldRayCastInput at sp+176; collector at sp+416 (+16 hits,
    // +20 count, +24 capacity | 0x80000000 while inline) with 8 records at
    // sp+448.
    const u32 in = sp + 176, coll = sp + 416, recs = sp + 448;
    u32 passed = 0;
    for (u32 i = 0; i < n; ++i) {
        store<u32>(in + 40, 0);
        Stvx(in, vec(pts[i]));
        Stvx(in + 16, vec(b));
        store<u8>(in + 32, 1);
        store<u32>(in + 36, kFilterInfo);
        store<u32>(coll + 4, 0x3F800000u);
        store<u32>(coll + 16, recs);
        store<u32>(coll, kCollectorVt);
        store<u32>(coll + 20, 0);
        store<u32>(coll + 24, 0x80000000u | kHitInline);
        for (u32 k = 0; k < kHitInline; ++k) {
            const u32 rec = recs + kHitSize * k;
            store<u32>(rec + 16, 0x3F800000u);
            store<u32>(rec + 20, 0xFFFFFFFFu);
            store<u32>(rec + 64, 0);
            store<u32>(rec + 32, 0xFFFFFFFFu);
            store<u32>(rec + 68, 0);
        }
        store<u32>(coll + 20, 0);
        store<u32>(coll + 4, 0x3F800000u);
        call(ctx, kHkCastRay, {hk, in, coll});
        u32 hits = load<u32>(coll + 16);
        if (load<u32>(coll + 20) != 0) {
            call(ctx, kSortHits, {coll});
            hits = load<u32>(coll + 16);
            const s32 count = static_cast<s32>(load<u32>(coll + 20));
            bool seen = false;
            for (s32 j = 0; j < count; ++j) {
                const u32 col = load<u32>(hits + kHitSize * static_cast<u32>(j) + 68);
                if (col == 0) continue;
                const u32 e = HitEntity(col);
                if (e == 0) continue;
                u32 obj = 0;
                for (s32 k = 0; k < static_cast<s32>(load<u32>(e + 128)); ++k) {
                    const u32 pr = load<u32>(e + 124) + 16 * static_cast<u32>(k);
                    if (load<u32>(pr) == kObjProp) {
                        obj = load<u32>(pr + 12);
                        break;
                    }
                }
                seen = obj == target;  // the low words compared (without the property: 0, never the non-zero target)
                break;
            }
            if (!seen) {
                const u32 cap = load<u32>(coll + 24);
                if ((cap & 0x80000000u) == 0) FreeHits(ctx, coll, hits, cap, false);
                break;
            }
            ++passed;
        }
        const u32 cap = load<u32>(coll + 24);
        if ((cap & 0x80000000u) == 0) FreeHits(ctx, coll, hits, cap, true);
    }
    ret(ctx, passed < n ? 0u : 1u);
}
