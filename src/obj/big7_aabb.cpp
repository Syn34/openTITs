// Game code, large-function sweep part 7: a capsule-like volume's Havok
// AABB and its four downward ground probes (002CE360).
#include "big7.h"

using namespace sly4;
using namespace sly4::big7;
using namespace sly4::big7::b7fn;

namespace {
// Volume fields.
constexpr u32 kVolOwner = 8;        // WorldObj*: +96 its matrix; vfunc +64 -> {+32 hkpCollidable* or 0}
constexpr u32 kVolBelow = 44;       // f32: probe length below the base
constexpr u32 kVolRadius = 64;      // f32
constexpr u32 kVolHalfHeight = 68;  // f32
constexpr u32 kVolDrop = 72;        // f32: drop of the base along the matrix's third row
constexpr u32 kVolPhantom = 120;    // hkpAabbPhantom*
constexpr u32 kVfnCollision = 64;

constexpr u32 kConst = 0x002CE320u;   // .text: f32 0.1 (padding), u32 0x80000000; lane masks y / z / w at +16 / +32 / +48
constexpr u32 kBig = 0x00B2C3A0u;     // f32 (just under FLT_MAX) x 4
constexpr u32 kCollectorVt = 0x00F1DE80u;  // vtable of the all-hits ray collector (hkpAllRayHitCollector)
constexpr u32 kHeapAlloc = 0x011DF31Cu;    // hkThreadMemory-like allocator object: vfunc +20 free(p, size)
constexpr u32 kHitProp = 0x418F096Fu;      // entity property key: "is ground" (name?)
constexpr u32 kHkGetBody = 0x009D19C8u;    // (hkpCollidable*) -> hkpRigidBody* (Havok)
constexpr u32 kHkSetAabb = 0x00924B90u;    // hkpAabbPhantom::setAabb(phantom, const hkAabb*) (Havok)
constexpr u32 kHkCastRay = 0x00925408u;    // hkpAabbPhantom::castRay(phantom, const hkpWorldRayCastInput*, hkpRayHitCollector*) (Havok)
constexpr u32 kHitSize = 80;               // hkpWorldRayCastOutput
constexpr u32 kHitInline = 8;              // records in the collector's inline storage
}  // namespace

// u32 Volume_UpdateAabb(Volume* v, u8 grounded[4])
// Four points around the volume's base (+-half height along the second
// matrix row, +-radius along the first) and four padded by 0.1, each
// stretched along the third row (down by the drop, up by drop + below or by
// the drop), bound the phantom's new AABB. Then a ray from each base point
// along the third row (drop + below) through the phantom; grounded[i] is 1
// when any hit entity has a non-zero kHitProp property. A collector that
// moved to the heap is freed. Returns what r3 holds at the end (for the
// last probe: the collector's inline flag 0x80000000, or free's result).
DECOMP(002CE360, Volume_UpdateAabb) {
    const u32 v = arg<u32>(ctx, 0);
    const u32 out = arg<u32>(ctx, 1);
    const u32 wo = load<u32>(v + kVolOwner);
    const u32 col = static_cast<u32>(VCall(ctx, wo, kVfnCollision, {wo}));
    const u32 mt = wo + 96;
    const f32 r0x = F(mt), r0y = F(mt + 4), r0z = F(mt + 8);
    const f32 r1x = F(mt + 16), r1y = F(mt + 20), r1z = F(mt + 24);
    const f32 r2x = F(mt + 32), r2y = F(mt + 36), r2z = F(mt + 40);
    const f32 tx = F(mt + 48), ty = F(mt + 52), tz = F(mt + 56);
    for (u32 k = 0; k < 4; ++k) store<u8>(out + k, 0);
    u32 phantom = load<u32>(v + kVolPhantom);
    u32 body = 0;
    const u32 hc = load<u32>(col + 32);
    if (hc != 0) {
        body = static_cast<u32>(call(ctx, kHkGetBody, {hc}));
        phantom = load<u32>(v + kVolPhantom);
    }
    const u32 filter = load<u32>(body + 44);  // Quirk: read from address 44 without a body.
    Stack frame(ctx, 896);  // the original's sp+112..+1007
    const u32 sp = frame.addr() - 112;
    auto put3 = [](u32 a, f32 x, f32 y, f32 z) {
        store<f32>(a, x);
        store<f32>(a + 4, y);
        store<f32>(a + 8, z);
    };
    const f32 h = F(v + kVolHalfHeight);
    const f32 cx = Fmadds(r1x, h, tx), cy = Fmadds(r1y, h, ty), cz = Fmadds(r1z, h, tz);
    const f32 r = F(v + kVolRadius);
    put3(sp + 192, Fmadds(r1x, h, cx), Fmadds(r1y, h, cy), Fmadds(r1z, h, cz));
    put3(sp + 208, Fnmsubs(r0x, r, cx), Fnmsubs(r0y, r, cy), Fnmsubs(r0z, r, cz));
    put3(sp + 224, Fnmsubs(r1x, h, cx), Fnmsubs(r1y, h, cy), Fnmsubs(r1z, h, cz));
    put3(sp + 240, Fmadds(r0x, r, cx), Fmadds(r0y, r, cy), Fmadds(r0z, r, cz));
    const f32 pad = F(kConst);
    const f32 h2 = h + pad, rr2 = r + pad;
    const f32 d = F(v + kVolDrop);
    const f32 e = F(v + kVolBelow) + d;
    put3(sp + 256, Fmadds(r1x, h2, cx), Fmadds(r1y, h2, cy), Fmadds(r1z, h2, cz));
    put3(sp + 272, Fnmsubs(r0x, rr2, cx), Fnmsubs(r0y, rr2, cy), Fnmsubs(r0z, rr2, cz));
    put3(sp + 288, Fnmsubs(r1x, h2, cx), Fnmsubs(r1y, h2, cy), Fnmsubs(r1z, h2, cz));
    put3(sp + 304, Fmadds(r0x, rr2, cx), Fmadds(r0y, rr2, cy), Fmadds(r0z, rr2, cz));
    const V4 my = Lvx(kConst + 16), mz = Lvx(kConst + 32), mw = Lvx(kConst + 48);
    auto vec = [&](f32 x, f32 y, f32 z) { return Andc(Sel(Sel(Splat1(x), Splat1(y), my), Splat1(z), mz), mw); };
    V4 mn = Lvx(kBig);
    V4 mx = Xor(mn, Splat(Lvlx(kConst + 4), 0));
    auto acc = [&](const V4& c) {
        mn = Min4(mn, c);
        mx = Max4(mx, c);
    };
    for (u32 i = 0; i < 4; ++i) {
        const u32 p = sp + 192 + 16 * i, q = sp + 256 + 16 * i;
        const f32 px = F(p), py = F(p + 4), pz = F(p + 8);
        const f32 qx = F(q), qy = F(q + 4), qz = F(q + 8);
        acc(vec(Fnmsubs(r2x, d, px), Fnmsubs(r2y, d, py), Fnmsubs(r2z, d, pz)));
        acc(vec(Fmadds(r2x, e, px), Fmadds(r2y, e, py), Fmadds(r2z, e, pz)));
        acc(vec(Fnmsubs(r2x, d, qx), Fnmsubs(r2y, d, qy), Fnmsubs(r2z, d, qz)));
        acc(vec(Fmadds(r2x, d, qx), Fmadds(r2y, d, qy), Fmadds(r2z, d, qz)));
    }
    Stvx(sp + 128, mx);
    Stvx(sp + 112, mn);
    call(ctx, kHkSetAabb, {phantom, sp + 112});
    // Ray probes: hkpWorldRayCastInput at sp+144 (from, to, +32 u8 1,
    // +36 filter info, +40 0); collector at sp+320 (+16 hits, +20 count,
    // +24 capacity | 0x80000000 while inline) with 8 records at sp+352.
    u64 res = 0;
    for (u32 i = 0; i < 4; ++i) {
        const u32 p = sp + 192 + 16 * i;
        store<u32>(sp + 184, 0);
        const f32 px = F(p), py = F(p + 4), pz = F(p + 8);
        const f32 ex = Fmadds(r2x, e, px);
        Stvx(sp + 144, vec(px, py, pz));
        const f32 ey = Fmadds(r2y, e, py);
        const f32 ez = Fmadds(r2z, e, pz);
        Stvx(sp + 160, vec(ex, ey, ez));
        store<u8>(sp + 176, 1);
        store<u32>(sp + 180, filter);
        store<u32>(sp + 336, sp + 352);
        store<u32>(sp + 340, 0);
        store<u32>(sp + 324, 0x3F800000u);
        store<u32>(sp + 320, kCollectorVt);
        store<u32>(sp + 344, 0x80000000u | kHitInline);
        for (u32 k = 0; k < kHitInline; ++k) {
            const u32 rec = sp + 352 + kHitSize * k;
            store<u32>(rec + 16, 0x3F800000u);
            store<u32>(rec + 20, 0xFFFFFFFFu);
            store<u32>(rec + 64, 0);
            store<u32>(rec + 32, 0xFFFFFFFFu);
            store<u32>(rec + 68, 0);
        }
        store<u32>(sp + 340, 0);
        store<u32>(sp + 324, 0x3F800000u);
        ctx.fpr[1] = ey;
        call(ctx, kHkCastRay, {load<u32>(v + kVolPhantom), sp + 144, sp + 320});
        store<u8>(out + i, 0);
        const s32 n = static_cast<s32>(load<u32>(sp + 340));
        const u32 hits = load<u32>(sp + 336);
        const u32 cap = load<u32>(sp + 344);
        u32 cur = load<u8>(out + i);
        if (n > 0) {
            for (s32 j = 0; j < n; ++j) {
                const u32 hcol = load<u32>(hits + kHitSize * static_cast<u32>(j) + 68);
                if (hcol == 0 || static_cast<s8>(load<u8>(hcol + 24)) != 1) continue;
                const u32 ent = hcol + static_cast<u32>(static_cast<s32>(static_cast<s8>(load<u8>(hcol + 16))));
                if (ent == 0) continue;
                u32 val = 0;  // the low word of the property's 64-bit value
                for (s32 k = 0; k < static_cast<s32>(load<u32>(ent + 128)); ++k) {
                    const u32 pr = load<u32>(ent + 124) + 16 * static_cast<u32>(k);
                    if (load<u32>(pr) == kHitProp) {
                        val = load<u32>(pr + 12);
                        break;
                    }
                }
                cur = ((cur & 0xFFu) | (val != 0 ? 1u : 0u)) != 0 ? 1u : 0u;
                store<u8>(out + i, static_cast<u8>(cur));
            }
        }
        res = cap & 0x80000000u;
        if (res == 0) {
            store<u32>(sp + 340, 0);
            store<u32>(sp + 320, kCollectorVt);
            const u64 size = Sx((cap & 0x3FFFFFFFu) * kHitSize);
            res = VCall(ctx, kHeapAlloc, 20, {kHeapAlloc, hits, size});
        }
    }
    ret(ctx, res);
}
