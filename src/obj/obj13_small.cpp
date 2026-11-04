// World objects, part 13: small callees of the draw / light code: the draw
// record size sum the topology dump calls, the draw-record list set-up, the
// light cache's slot copy and a light slot's grid cell range.
#include "obj13.h"

using namespace sly4;
using namespace sly4::obj9;
using namespace sly4::obj12;
using namespace sly4::obj13;

namespace {

// ---------------------------------------------------------------- draw records (Topology_DumpDraw)
// +24 16-byte entries (+8 u32 a byte size), +109 u8 their count, +44 a
// further entry of the same shape (name?).
constexpr u32 kDrEntries = 24, kDrEntryCount = 109, kDrExtra = 44, kDeSize = 8, kDeStride = 16;

// ---------------------------------------------------------------- the draw-record list (obj12.h kTrList)
constexpr u32 kTlEnabled = 0, kTlVector = 4, kTlVectorCap = 10, kTl32 = 32, kTlAtomicA = 128, kTlAtomicB = 256;
constexpr u16 kTlInitialCap = 160;

// ---------------------------------------------------------------- the light grid (LightGrid_Rebuild)
// A slot's +0 x, +8 z, +12 radius. The grid's bounds and cell scales
// (name?): 0x011944F0 / 0x011944F8 the minimum x / z, 0x01194500 /
// 0x01194508 the maximum, 0x01194520 / 0x01194528 the cells per unit.
constexpr u32 kGridMinX = 0x011944F0, kGridMinZ = 0x011944F8, kGridMaxX = 0x01194500, kGridMaxZ = 0x01194508;
constexpr u32 kGridScaleX = 0x01194520, kGridScaleZ = 0x01194528;
constexpr s32 kGridLast = 7;

inline void CopyWords(u32 dst, u32 src, u32 n) {
    for (u32 k = 0; k < n; ++k) store<u32>(dst + 4 * k, load<u32>(src + 4 * k));
}

// Clamps a cell coordinate to [0, 7] (the original's compare sequence: a
// NaN passes through).
inline f32 ClampCell(f32 v) {
    const f32 zero = static_cast<f32>(rt::fcfid(0));
    if (v < zero) return zero;
    const f32 last = static_cast<f32>(rt::fcfid(static_cast<u64>(static_cast<s64>(kGridLast))));
    return v > last ? last : v;
}

}  // namespace

// u32 Topology_DumpDraw(const DrawRecord* r)
// The record's data size in 16-byte units: the +8 byte sizes of its +109
// entries at +24 (stride 16) plus that of the entry at +44, each >> 4 (the
// count walked as a signed down-counter).
DECOMP(00539874, Topology_DumpDraw) {
    const u32 r = arg<u32>(ctx, 0);
    s32 n = load<u8>(r + kDrEntryCount);
    u32 sum = 0;
    if (n != 0) {
        u32 e = r;
        do {
            sum += load<u32>(e + kDrEntries + kDeSize) >> 4;
            e += kDeStride;
        } while (--n > 0);
    }
    ret(ctx, (load<u32>(r + kDrExtra + kDeSize) >> 4) + sum);
}

// void DrawRecordList_Init(DrawRecordList* l)
// The function-local static of Topology_Apply: enabled = 1, the pointer
// vector constructed with a capacity hint of 160, +32 = 0 and the two
// atomic words (+128, +256) reset with lwarx / stwcx. r3 = 0 at return.
DECOMP(004FD214, DrawRecordList_Init) {
    const u32 l = arg<u32>(ctx, 0);
    store<u8>(l + kTlEnabled, 1);
    call(ctx, o12fn::PtrVector_Construct, {l + kTlVector});  // r4 = 1 (not read)
    store<u16>(l + kTlVectorCap, kTlInitialCap);
    store<u32>(l + kTl32, 0);
    store<u32>(l + kTlAtomicA, 0);  // atomic store
    store<u32>(l + kTlAtomicB, 0);  // atomic store
    ctx.gpr[3] = 0;
}

// void ObjLight_Apply(WorldObj* o)
// Fills the light cache's entries from +215 on from the light slots: an
// entry without a slot gets type 3, a zero colour and the default
// position; one with a slot takes the slot's type and, for type 2, its
// position, colour and falloff parameters. r3 = 4 (or +215 when it was
// already 4 or more).
DECOMP(00582A84, ObjLight_Apply) {
    const u32 c = load<u32>(arg<u32>(ctx, 0) + obj5::kObjLight);
    const u32 slots = load<u32>(cls::kLightSlotLists + kLgSlots);
    s32 i = load<u8>(c + kLcUsed);
    for (; i < static_cast<s32>(kLcEntries); ++i) {
        const u32 col = c + kLcColour + kLcStride * i, pos = c + kLcPos + kLcStride * i;
        const u32 idx = load<u32>(c + kLcLights + 4 * i);
        if (idx == 0) {
            store<u8>(c + kLcType + i, kLcTypeNone);
            CopyWords(col, kLcNoColour, 3);
            CopyWords(pos, kLcNoPos, 3);
            continue;
        }
        const u32 s = slots + (idx - 1) * kLightSlotSize;
        store<u8>(c + kLcType + i, load<u8>(s + kLsType));
        if (load<u8>(s + kLsType) != kLightSpot) continue;
        CopyWords(pos, s + kLsPos, 3);
        CopyWords(col, s + kLsColour, 3);
        CopyWords(c + kLcParams + kLcStride * i, s + kLsParams, 4);
    }
    ctx.gpr[3] = static_cast<u32>(i);
}

// void LightSlot_CellRange(const LightSlot* s, s32* x0, s32* x1, s32* z0, s32* z1)
// The 8x8 grid cells the slot's circle (x, z, radius) covers: each edge
// clamped to the grid bounds (fsel), offset by the minimum, scaled to cells
// and clamped to [0, 7], then truncated. r3 = 7 at return.
DECOMP(005820E0, LightSlot_CellRange) {
    const u32 s = arg<u32>(ctx, 0);
    const f32 x = Lf(s), r = Lf(s + 12), z = Lf(s + 8);
    const f32 minx = Lf(kGridMinX), minz = Lf(kGridMinZ), maxx = Lf(kGridMaxX), maxz = Lf(kGridMaxZ);
    const f32 lx = Fsubs(x, r), hx = Fadds(x, r), lz = Fsubs(z, r), hz = Fadds(z, r);
    const f32 cx0 = Fsubs(lx, minx) >= 0 ? lx : minx;
    const f32 cx1 = Fsubs(hx, maxx) >= 0 ? maxx : hx;
    const f32 cz0 = Fsubs(lz, minz) >= 0 ? lz : minz;
    const f32 cz1 = Fsubs(hz, maxz) >= 0 ? maxz : hz;
    const f32 sx = Lf(kGridScaleX), sz = Lf(kGridScaleZ);
    const f32 x0 = ClampCell(Fmuls(Fsubs(cx0, minx), sx));
    const f32 x1 = ClampCell(Fmuls(Fsubs(cx1, minx), sx));
    const f32 z0 = ClampCell(Fmuls(Fsubs(cz0, minz), sz));
    const f32 z1 = ClampCell(Fmuls(Fsubs(cz1, minz), sz));
    store<u32>(arg<u32>(ctx, 1), static_cast<u32>(rt::fctiw(x0, true)));
    store<u32>(arg<u32>(ctx, 2), static_cast<u32>(rt::fctiw(x1, true)));
    store<u32>(arg<u32>(ctx, 3), static_cast<u32>(rt::fctiw(z0, true)));
    store<u32>(arg<u32>(ctx, 4), static_cast<u32>(rt::fctiw(z1, true)));
    ctx.gpr[3] = static_cast<u32>(kGridLast);
}
