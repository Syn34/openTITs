// World objects, part 13: the object light cache's steps (WorldObj_UpdateLight):
// the per-light blend from the light instances.
#include "obj13.h"

using namespace sly4;
using namespace sly4::obj9;
using namespace sly4::obj12;
using namespace sly4::obj13;

namespace {

inline void CopyWordsL(u32 dst, u32 src, u32 n) {
    for (u32 k = 0; k < n; ++k) store<u32>(dst + 4 * k, load<u32>(src + 4 * k));
}

}  // namespace

// void ObjLight_Blend(WorldObj* o)
// The light cache's ambient colour: zero, or the ambient light's (+220)
// colour times min(its +376, 1). Then each entry in use (+215, re-read
// every step): without a light type 3, a zero colour and the default
// position; with one the light's type (+368) and, for directional lights
// (type 1), the negated world forward row (+112) as the position, its
// colour, and the falloff parameters: {+384, +388, +392, +376} without
// +372, else {+388, +376 / (+388 - +384), 0, +376}. r3 = 0x01164B40.
DECOMP(00582848, ObjLight_Blend) {
    const u32 c = load<u32>(arg<u32>(ctx, 0) + obj5::kObjLight);
    CopyWordsL(c + kLcAmbient, kLcNoColour, 3);
    const u32 amb = load<u32>(c + kLcAmbientLight);
    if (amb != 0) {
        f32 k = Lf(kLcBlendOne);
        const f32 v = Lf(amb + cls::kLightRangeSq);
        if (v < k) k = v;
        for (u32 j = 0; j < 12; j += 4) store<f32>(c + kLcAmbient + j, Fmuls(Lf(amb + cls::kLightColor + j), k));
    }
    for (s32 i = 0; i < static_cast<s32>(load<u8>(c + kLcUsed)); ++i) {
        const u32 col = c + kLcColour + kLcStride * i, pos = c + kLcPos + kLcStride * i;
        const u32 l = load<u32>(c + kLcLights + 4 * i);
        if (l == 0) {
            store<u8>(c + kLcType + i, kLcTypeNone);
            CopyWordsL(col, kLcNoColour, 3);
            CopyWordsL(pos, kLcNoPos, 3);
            continue;
        }
        store<u8>(c + kLcType + i, static_cast<u8>(load<u32>(l + cls::kLightType)));
        if (load<s32>(l + cls::kLightType) != kLightDirectional) continue;
        const u32 dx = load<u32>(l + kObjForward), dy = load<u32>(l + kObjForward + 4), dz = load<u32>(l + kObjForward + 8);
        store<u32>(pos, dx ^ 0x80000000u);  // fneg
        store<u32>(pos + 4, dy ^ 0x80000000u);
        store<u32>(pos + 8, dz ^ 0x80000000u);
        CopyWordsL(col, l + cls::kLightColor, 3);
        const u32 par = c + kLcParams + kLcStride * i;
        if (load<u32>(l + cls::kLight372) == 0) {
            const u32 a = load<u32>(l + cls::kLight384), b = load<u32>(l + cls::kLight384 + 4);
            const u32 d = load<u32>(l + cls::kLight384 + 8), r = load<u32>(l + cls::kLightRangeSq);
            store<u32>(par, a);
            store<u32>(par + 4, b);
            store<u32>(par + 8, d);
            store<u32>(par + 12, r);
        } else {
            const f32 hi = Lf(l + cls::kLight384 + 4), lo = Lf(l + cls::kLight384), r = Lf(l + cls::kLightRangeSq);
            store<u32>(par, load<u32>(l + cls::kLight384 + 4));  // raw lfs / stfs
            store<u32>(par + 8, 0);
            store<u32>(par + 12, load<u32>(l + cls::kLightRangeSq));
            store<f32>(par + 4, Fdivs(r, Fsubs(hi, lo)));
        }
    }
    ctx.gpr[3] = kLcNoColour;
}

namespace {

// ---------------------------------------------------------------- ObjLight_Collect
constexpr u32 kLcCollected = 214;                   // u8: the lights found (obj12.h kLcFlags +212 bit 0x80 = collected)
constexpr u32 kLcDone = 0x80;                       // obj12.h kLcSettling
constexpr u32 kLcWanted = 3;                        // directional lights kept
constexpr u32 kLcKept = 1;                          // the count is then capped to this (name?)
constexpr u32 kSlBegin = 6, kSlEnd = 8;             // classes.h kLightSlotLists: u16 slot indices (0xFFFF = none)
constexpr u32 kObjLightMask = obj12::kObjLit;       // obj +292: the light groups it belongs to
constexpr u32 kObjVisible = obj4::kObjFlagVisible;  // +164 bit 0x8000
constexpr u32 kStrAllSlots = 0x00AF4F28;            // "all available slots on: %s\n"
constexpr u32 kCollectFrame = 160;
constexpr u32 kCfList = 112;                        // sp+112: the sorted lights (up to 3)
constexpr u32 String_Format = 0x005534D0;

inline u32 Cache(u32 o) { return load<u32>(o + obj5::kObjLight); }

// A directional light's weight: |colour| * (+376)^2.
inline f32 Weight(u32 l) {
    const u32 c = l + cls::kLightColor;
    const f32 y = Lf(c + 4), x = Lf(c), z = Lf(c + 8);
    const f32 len = obj10::Fsqrts(Fmadds(z, z, Fmadds(x, x, Fmuls(y, y))));
    const f32 r = Lf(l + cls::kLightRangeSq);
    return Fmuls(Fmuls(r, r), len);
}

}  // namespace

// void ObjLight_Collect(WorldObj* o)
// Once per refresh (+212 bit 0x80): picks the object's lights from the
// type-0/1 light slots (after +6 up to +8) whose owner is visible and
// shares a light group (+292): the first type-0 light as the ambient one
// (+220; quirk kept: the test uses the ambient read before the scan, so a
// later type-0 light replaces it) and up to three directional (type 1)
// lights sorted by Weight, largest first, stopping once it has an
// ambient light and three. The count is then capped to 1 (+214), the
// kept lights go to +224 and the rest are cleared; +215 = 1 (entry 0 is
// Blend's, the rest Apply's). r3 = 1 (+212 when it was already done).
DECOMP(00581D48, ObjLight_Collect) {
    const u32 o = arg<u32>(ctx, 0);
    {
        const u32 c = Cache(o);
        const u8 b = load<u8>(c + kLcFlags);
        if ((b & kLcDone) != 0) {
            ctx.gpr[3] = b;
            return;
        }
        store<u8>(c + kLcFlags, static_cast<u8>(b | kLcDone));
    }
    store<u8>(Cache(o) + kLcCollected, 0);
    store<u32>(Cache(o) + kLcAmbientLight, 0);
    Stack frame(ctx, kCollectFrame);
    const u32 sp = frame.addr();
    const u32 list = sp + kCfList;
    u32 c = Cache(o);
    const bool amb0 = load<u32>(c + kLcAmbientLight) == 0;  // the stale cr7 test
    const bool full = !(load<u8>(c + kLcCollected) < kLcWanted);
    if (!(load<u32>(c + kLcAmbientLight) != 0 && full)) {
        const u32 g = cls::kLightSlotLists;
        const u32 b1 = static_cast<u16>(load<u16>(g + kSlBegin) + 1);
        const u32 begin = b1 == 0xFFFF ? 0u : load<u32>(g) + (b1 << 6);
        const u32 e = load<u16>(g + kSlEnd);
        const u32 end = e == 0xFFFF ? 0u : load<u32>(g) + (e << 6);
        if (begin != end) {
            for (u32 p = begin;;) {
                const u32 l = load<u32>(p + obj5::kLsOwner);
                bool done = false;
                if ((load<u32>(o + kObjLightMask) & load<u32>(l + kObjLightMask)) != 0 &&
                    (load<u32>(l + obj::kObjFlags) & kObjVisible) != 0) {
                    const s32 type = load<s32>(l + cls::kLightType);
                    if (type == kLightDirectional) {
                        const u32 n = load<u8>(c + kLcCollected);
                        if (n < kLcWanted) {
                            const f32 w = Weight(l);
                            s32 j = static_cast<s32>(n) - 1;
                            for (; j >= 0; --j) {
                                const u32 lj = load<u32>(list + 4 * static_cast<u32>(j));
                                if (w < Weight(lj)) break;
                                store<u32>(list + 4 * static_cast<u32>(j) + 4, lj);
                            }
                            store<u8>(c + kLcCollected, static_cast<u8>(n + 1));
                            store<u32>(list + 4 * static_cast<u32>(j + 1), l);
                        }
                    } else if (type == 0 && amb0) {
                        store<u32>(c + kLcAmbientLight, l);
                    }
                    c = Cache(o);
                    done = load<u32>(c + kLcAmbientLight) != 0 && load<u8>(c + kLcCollected) == kLcWanted;
                }
                if (done) break;
                p += obj12::kLightSlotSize;
                if (p == end) break;
            }
            if (load<u8>(c + kLcCollected) == 4) {  // never reached with three kept (debug message)
                const u32 name = static_cast<u32>(obj4::Vcall0(ctx, o, obj4::kVfnGetName));
                call(ctx, String_Format, {kStrAllSlots, name});
                c = Cache(o);
            }
            const u8 v = load<u8>(c + kLcCollected);
            store<u8>(c + kLcCollected, static_cast<u8>(v < kLcKept ? v : kLcKept));
            c = Cache(o);
            for (u32 i = 0; i < load<u8>(c + kLcCollected);) {
                store<u32>(c + kLcLights + 4 * i, load<u32>(list + 4 * i));
                i = (i + 1) & 0xFF;
                c = Cache(o);
            }
        }
    }
    const u32 n = load<u8>(c + kLcCollected);
    if (n < kLcEntries) {
        for (u32 k = n; k < kLcEntries; ++k) store<u32>(Cache(o) + kLcLights + 4 * k, 0);
        c = Cache(o);
    }
    store<u8>(c + kLcUsed, 1);
    ctx.gpr[3] = 1;
}

namespace {

// ---------------------------------------------------------------- ObjLight_Sort
constexpr u32 kSortFrame = 800;
constexpr u32 kSoBox = 112;                         // sp+112: the object's world box {min[4], max[4]}
constexpr u32 kSoSeen = 144, kSoSeenBytes = 384;    // sp+144: u8 per light slot (indexed unchecked)
constexpr u32 kSoRange = 528;                       // sp+528: s32 x0, x1, z0, z1
constexpr u32 kSoCand = 544, kSoCandMax = 12;       // sp+544: {LightSlot*, u32 key} candidates
constexpr u32 kSortZero = 0x00582288;               // f64 0
constexpr u32 kLsGroups = 44;                       // light slot u32: the light's +292 groups
constexpr u32 kLsPriority = 60;                     // light slot u8 (the light's +380): higher sorts first
constexpr u32 kSpotCount = 4;                       // classes.h kLightSlotLists +4: u16 end of the type-2 slots

}  // namespace

// void ObjLight_Sort(WorldObj* o)
// Fills the light cache's entries after the collected ones (+214 = +215)
// with the nearest type-2 (spot / point) light slots: the grid cells the
// object's bounding sphere (+240) covers are scanned (each slot once, the
// slot index marking a 384-byte stack array unchecked) for slots sharing a
// light group whose sphere overlaps the object's sphere and whose radius
// reaches the object's world box (Box_Transform of the class box by +80),
// at most 12. Each gets the key ((255 - priority) << 24) | (s32)distance^2;
// with more candidates than free entries the smallest keys are taken
// (selection, unsigned compares), else all of them, last first. Entries
// hold 1-based slot indices; the rest are cleared (name?).
DECOMP(00582290, ObjLight_Sort) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 g = cls::kLightSlotLists;
    const u32 c = load<u32>(o + obj5::kObjLight);
    const u32 slots = load<u32>(g + kLgSlots);
    const u32 nspot = load<u16>(g + kSpotCount);
    const u8 used = load<u8>(c + kLcUsed);
    store<u8>(c + kLcCollected, used);
    if (used == kLcEntries) return;
    if (slots == slots + (nspot << 6)) return;
    const u32 mask = load<u32>(o + kObjLightMask);
    const u32 sph = o + obj11::kObjBoundSphere;
    Stack frame(ctx, kSortFrame);
    const u32 sp = frame.addr();
    const u32 box = sp + kSoBox, seen = sp + kSoSeen, range = sp + kSoRange, cand = sp + kSoCand;
    for (u32 k = 0; k < 12; k += 4) store<u32>(box + k, obj4::kFltMax);
    for (u32 k = 0; k < 12; k += 4) store<u32>(box + 16 + k, obj4::kFltMinNeg);
    call(ctx, obj4::o4fn::Box_Transform, {box, load<u32>(o + obj4::kObjClassPtr) + obj4::kClassBoxMin, o + obj6::kObjWorld});
    call(ctx, o12fn::LightSlot_CellRange, {sph, range, range + 4, range + 8, range + 12});
    const s32 x0 = load<s32>(range), x1 = load<s32>(range + 4), z0 = load<s32>(range + 8), z1 = load<s32>(range + 12);
    const f32 sx = Lf(sph), sy = Lf(sph + 4), sz = Lf(sph + 8), sr = Lf(sph + 12);
    const f32 minx = Lf(box), maxx = Lf(box + 16), miny = Lf(box + 4), minz = Lf(box + 8);
    u32 n = load<u8>(c + kLcCollected);
    for (u32 k = 0; k < kSoSeenBytes; k += 8) store<u64>(seen + k, 0);
    u32 k = 0;
    for (s32 x = x0; x <= x1; x = static_cast<s32>(static_cast<u32>(x) + 1)) {
        for (s32 z = z0; z <= z1; z = static_cast<s32>(static_cast<u32>(z) + 1)) {
            const u32 cell = g + kLgCells + static_cast<u32>(z) * 4 + static_cast<u32>(x) * 32;
            const u32 start = load<u16>(cell), cnt = load<u16>(cell + 2);
            if (cnt == 0) continue;
            const f64 zero = load<f64>(kSortZero);
            u32 ip = g + kLgList + start * 2;
            for (u32 left = cnt - 1;; --left) {
                const u32 idx = load<u16>(ip);
                ip += 2;
                const u32 s = slots + (idx << 6);
                if (load<u8>(seen + idx) == 0) {
                    store<u8>(seen + idx, 1);
                    if ((load<u32>(s + kLsGroups) & mask) != 0) {
                        const f32 py = Lf(s + 4), px = Lf(s), pz = Lf(s + 8), pr = Lf(s + 12);
                        const f32 dy = Fsubs(sy, py), dx = Fsubs(sx, px), dz = Fsubs(sz, pz);
                        const f32 rr = Fadds(pr, sr);
                        const f32 d2 = Fmadds(dz, dz, Fmadds(dx, dx, Fmuls(dy, dy)));
                        if (d2 < Fmuls(rr, rr)) {
                            // The squared distance from the slot's centre to the box (fsel per side).
                            const f32 ax = Fsubs(px, minx), bx = Fsubs(px, maxx);
                            const f32 ay = Fsubs(py, miny), by = Fsubs(py, Lf(box + 20));
                            const f32 az = Fsubs(pz, minz), bz = Fsubs(pz, Lf(box + 24));
                            const f32 z32 = static_cast<f32>(zero);
                            const f32 tax = ax >= 0 ? z32 : Fmuls(ax, ax), tbx = bx >= 0 ? Fmuls(bx, bx) : z32;
                            const f32 tay = ay >= 0 ? z32 : Fmuls(ay, ay), tby = by >= 0 ? Fmuls(by, by) : z32;
                            const f32 taz = az >= 0 ? z32 : Fmuls(az, az), tbz = bz >= 0 ? Fmuls(bz, bz) : z32;
                            f32 sum = Fadds(tax, tbx);
                            sum = Fadds(sum, tay);
                            sum = Fadds(sum, tby);
                            sum = Fadds(sum, taz);
                            sum = Fadds(sum, tbz);
                            if (sum <= Fmuls(pr, pr)) {
                                const u32 pri = (255u - load<u8>(s + kLsPriority)) & 0xFF;
                                store<u32>(cand + 8 * k, s);
                                store<u32>(cand + 8 * k + 4, (pri << 24) | static_cast<u32>(rt::fctiw(d2, true)));
                                if (++k == kSoCandMax) {  // ends all three loops
                                    x = x1;
                                    z = z1;
                                    left = 0;
                                }
                            }
                        }
                    }
                }
                if (left == 0) break;
            }
        }
    }
    auto slot_no = [&](u32 s) { return static_cast<u32>(static_cast<s32>(s - slots) / 64 + 1); };
    const u32 room = kLcEntries - n;
    if (k > room) {
        for (u32 r = room, kk = k; r != 0; --r, --kk) {
            u32 best = kk - 1, bkey = load<u32>(cand + 8 * best + 4);
            for (u32 j = kk - 1; j-- > 0;) {
                const u32 kj = load<u32>(cand + 8 * j + 4);
                if (kj < bkey) {
                    bkey = kj;
                    best = j;
                }
            }
            const u32 s = load<u32>(cand + 8 * best);
            store<u32>(cand + 8 * best, load<u32>(cand + 8 * (kk - 1)));
            store<u32>(cand + 8 * best + 4, load<u32>(cand + 8 * (kk - 1) + 4));
            store<u32>(c + kLcLights + 4 * n, slot_no(s));
            ++n;
        }
        store<u8>(c + kLcCollected, static_cast<u8>(n));
        return;
    }
    for (u32 j = k; j-- > 0;) {
        store<u32>(c + kLcLights + 4 * n, slot_no(load<u32>(cand + 8 * j)));
        ++n;
    }
    store<u8>(c + kLcCollected, static_cast<u8>(n));
    for (u32 m = n; m < kLcEntries; ++m) store<u32>(c + kLcLights + 4 * m, 0);
}
