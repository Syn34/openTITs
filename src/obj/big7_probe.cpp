// Game code, large-function sweep part 7: a ray probe through the Havok
// world that reports the nearest / farthest solid hit (001B5EB0).
#include "big7.h"

using namespace sly4;
using namespace sly4::big7;
using namespace sly4::big7::b7fn;

namespace {
constexpr u32 kSelfEntity = 576;      // the prober's own hkpEntity (its hits are ignored)
constexpr u32 kConst = 0x001B5E70u;   // .text: f32 FLT_MAX, f32 -FLT_MAX; lane masks y / z / w at +16 / +32 / +48
constexpr u32 kCollectorVt = 0x00F1DE80u;  // vtable of the all-hits ray collector (hkpAllRayHitCollector)
constexpr u32 kHeapAlloc = 0x011DF31Cu;    // allocator object: vfunc +20 free(p, size)
constexpr u32 kObjProp = 0x91C7CF5Bu;      // entity property key: the game object (name?)
constexpr u32 kObjFlags196 = 196;          // game object: 0x20000 asks it whether it blocks
constexpr u32 kVfnBlockState = 32;         // game object (o, u32 which) -> state
constexpr u32 kFilterInfo = 28;
constexpr u32 kHitSize = 80;               // hkpWorldRayCastOutput: +16 fraction, +68 hkpCollidable*
constexpr u32 kHitInline = 8;
constexpr u32 kHkCastRay = 0x0092E510u;    // hkpWorld::castRay(world, const hkpWorldRayCastInput*, hkpRayHitCollector*) (Havok)
constexpr u32 kSortHits = 0x00887E28u;     // (hkpAllRayHitCollector*): sort by fraction (Havok)

// The entity a hit's collidable belongs to (its +24 type 1, owner at
// +(s8)+16), or 0.
u32 HitEntity(u32 col) {
    if (static_cast<s8>(load<u8>(col + 24)) != 1) return 0;
    return col + static_cast<u32>(static_cast<s32>(static_cast<s8>(load<u8>(col + 16))));
}
// The low word of the entity's kObjProp property, or 0.
u32 EntityObj(u32 e) {
    for (s32 k = 0; k < static_cast<s32>(load<u32>(e + 128)); ++k) {
        const u32 pr = load<u32>(e + 124) + 16 * static_cast<u32>(k);
        if (load<u32>(pr) == kObjProp) return load<u32>(pr + 12);
    }
    return 0;
}
// Collidable shape kinds (+28 & 0x1F) that count as solid.
bool Solid(u32 col) {
    const u32 k = load<u32>(col + 28) & 0x1Fu;
    return k == 1 || k == 28 || k == 27 || k == 3;
}
// An object that asks (+196 0x20000) blocks unless its state 0 is 2, or
// it is 3 and its state 1 is 1.
bool Blocks(PPUContext& ctx, u32 obj) {
    if (obj == 0 || (load<u32>(obj + kObjFlags196) & 0x20000u) == 0) return true;
    if (static_cast<u32>(VCall(ctx, obj, kVfnBlockState, {obj, 0})) == 2) return false;
    if (static_cast<u32>(VCall(ctx, obj, kVfnBlockState, {obj, 0})) != 3) return true;
    return static_cast<u32>(VCall(ctx, obj, kVfnBlockState, {obj, 1})) != 1;
}
}  // namespace

// bool Probe_CastRay(Obj* self, hkpWorld* world, const f32 dir[3], const f32 from[3], f32 len, u32 unused,
//                    f32* outNear, f32* outFar, Obj** outObj)
// Casts from `from` to from + dir * len (filter info 28) and walks the
// hits sorted by fraction from both ends at once. A hit counts when it is
// on an entity other than self's, of a solid kind, and its game object (if
// it asks) blocks; nearest / farthest fractions go to outNear / outFar
// (FLT_MAX / -FLT_MAX when none) and the nearest hit's object to outObj.
// Quirk: a hit taken from the back end uses the front hit's fraction.
// Returns whether any hit counted.
DECOMP(001B5EB0, Probe_CastRay) {
    const u32 self = arg<u32>(ctx, 0);
    const u32 world = arg<u32>(ctx, 1);
    const u32 dir = arg<u32>(ctx, 2);
    const u32 from = arg<u32>(ctx, 3);
    const u32 outNear = arg<u32>(ctx, 5);
    const u32 outFar = arg<u32>(ctx, 6);
    const u32 outObj = arg<u32>(ctx, 7);
    const f64 len = ctx.fpr[1];
    if (outNear != 0) store<u32>(outNear, 0x7F7FFFFFu);
    if (outFar != 0) store<u32>(outFar, 0xFF7FFFFFu);
    if (outObj != 0) store<u32>(outObj, 0);
    Stack frame(ctx, 736);  // the original's sp+112..+847
    const u32 sp = frame.addr() - 112;
    const V4 my = Lvx(kConst + 16), mz = Lvx(kConst + 32), mw = Lvx(kConst + 48);
    auto vec = [&](f32 x, f32 y, f32 z) { return Andc(Sel(Sel(Splat1(x), Splat1(y), my), Splat1(z), mz), mw); };
    const f32 fx = std::bit_cast<f32>(LaneW(Lvlx(from), 0));
    const f32 fy = std::bit_cast<f32>(LaneW(Lvlx(from + 4), 0));
    const f32 fz = std::bit_cast<f32>(LaneW(Lvlx(from + 8), 0));
    const f32 tx = Fmadds(F(dir), len, F(from));
    const f32 ty = Fmadds(F(dir + 4), len, F(from + 4));
    const f32 tz = Fmadds(F(dir + 8), len, F(from + 8));
    // hkpWorldRayCastInput at sp+112; collector at sp+160 (+16 hits,
    // +20 count, +24 capacity | 0x80000000 while inline) with 8 records at
    // sp+192.
    store<u8>(sp + 144, 0);
    store<u32>(sp + 152, 0);
    Stvx(sp + 112, vec(fx, fy, fz));
    Stvx(sp + 128, vec(tx, ty, tz));
    store<u32>(sp + 148, kFilterInfo);
    store<u32>(sp + 176, sp + 192);
    store<u32>(sp + 164, 0x3F800000u);
    store<u32>(sp + 160, kCollectorVt);
    store<u32>(sp + 180, 0);
    store<u32>(sp + 184, 0x80000000u | kHitInline);
    for (u32 k = 0; k < kHitInline; ++k) {
        const u32 rec = sp + 192 + kHitSize * k;
        store<u32>(rec + 16, 0x3F800000u);
        store<u32>(rec + 20, 0xFFFFFFFFu);
        store<u32>(rec + 64, 0);
        store<u32>(rec + 32, 0xFFFFFFFFu);
        store<u32>(rec + 68, 0);
    }
    store<u32>(sp + 180, 0);
    store<u32>(sp + 164, 0x3F800000u);
    ctx.fpr[1] = tz;
    call(ctx, kHkCastRay, {world, sp + 112, sp + 160});
    if (load<u32>(sp + 180) == 0) {
        const u32 cap = load<u32>(sp + 184);
        const u32 hits = load<u32>(sp + 176);
        if ((cap & 0x80000000u) == 0) {
            store<u32>(sp + 180, 0);
            store<u32>(sp + 160, kCollectorVt);
            VCall(ctx, kHeapAlloc, 20, {kHeapAlloc, hits, Sx((cap & 0x3FFFFFFFu) * kHitSize)});
        }
        ret(ctx, 0u);
        return;
    }
    call(ctx, kSortHits, {sp + 160});
    s32 back = static_cast<s32>(load<u32>(sp + 180) - 1);
    s32 front = 0;
    u32 fo = 0, bo = static_cast<u32>(back) * kHitSize;
    u32 best = 0, any = 0;
    f32 nearF = F(kConst), farF = F(kConst + 4);
    auto take = [&](u32 fh, u32 obj) {
        any = 1;
        const f32 f = F(fh + 16);
        ctx.fpr[1] = f;
        if (f < nearF) {
            if (obj != 0) best = obj;
            nearF = f;
        }
        if (f > farF) farF = f;
    };
    bool done = false;
    do {
        const u32 hits = load<u32>(sp + 176);
        const u32 fh = hits + fo;
        const u32 cf = load<u32>(fh + 68);
        const u32 fe = HitEntity(cf);
        const u32 cb = load<u32>(hits + bo + 68);
        const u32 be = HitEntity(cb);
        if (fe != 0 && load<u32>(self + kSelfEntity) != fe) {
            const u32 obj = EntityObj(fe);
            if (Solid(cf) && Blocks(ctx, obj)) take(fh, obj);
        }
        if (be != 0 && front != back && load<u32>(self + kSelfEntity) != be) {
            const u32 obj = EntityObj(be);
            if (Solid(cb) && Blocks(ctx, obj)) take(fh, obj);  // the front hit's fraction (see above)
        }
        --back;
        ++front;
        fo += kHitSize;
        if (front > back) done = true;
        bo -= kHitSize;
    } while (!done);
    if (outNear != 0) store<f32>(outNear, nearF);
    if (outFar != 0) store<f32>(outFar, farF);
    if (outObj != 0) store<u32>(outObj, best);
    const u32 cap = load<u32>(sp + 184);
    if ((cap & 0x80000000u) == 0) {
        store<u32>(sp + 180, 0);
        store<u32>(sp + 160, kCollectorVt);
        const u32 hits = load<u32>(sp + 176);
        const u32 opd = load<u32>(load<u32>(kHeapAlloc) + 20);
        call_ptr(ctx, opd, {kHeapAlloc, hits, Sx((cap & 0x3FFFFFFFu) * kHitSize), load<u32>(opd), opd});  // entry in r6, OPD in r7
    }
    ret(ctx, any);
}
