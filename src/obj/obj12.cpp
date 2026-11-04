// World objects, part 12: the geometry definition's LOD pick and the
// skeleton definition allocator.
#include "obj12.h"

using namespace sly4;
using namespace sly4::obj9;
using namespace sly4::obj10;
using namespace sly4::obj12;

namespace {

// ---------------------------------------------------------------- GeomDef_SelectLod
// ApeGeomDef: +60 f32 the bounding sphere's radius, +112 u8 the LOD count,
// +120 f32 the fade band, +124 f32 the near fade distance, +128 f32 the
// smallest near fade, +132 {AglTopology*, f32 far distance}[] (obj2.h
// kClassTopos).
constexpr u32 kGdRadius = 48 + 12, kGdLodCount = 112, kGdFadeBand = 120, kGdFadeNear = 124, kGdFadeMin = 128;
constexpr u32 kLodDist = 4;
constexpr u32 kItemPos = 48, kItemFade = 69;   // the drawn item: f32[3] position, u8 fade (name?)
constexpr u32 kSlConsts = 0x00585B88;          // f32 {0, 1e-5, 1, 1/255}
constexpr u32 kSlZero = 0, kSlTiny = 4, kSlOne = 8, kSlMinFade = 12;

// item +69 = (u8)(s32)(+69 * k)
void ScaleFade(u32 item, f32 k) {
    const f32 b = static_cast<f32>(static_cast<f64>(static_cast<s64>(load<u8>(item + kItemFade))));
    store<u8>(item + kItemFade, static_cast<u8>(rt::fctiw(static_cast<f64>(Fmuls(b, k)), true)));
}

inline f32 Min1(f32 x, f32 one) { return x < one ? x : one; }

// ---------------------------------------------------------------- SkeletonDef_New
// ApeTMemClassFL<ApeSkeletonDef, 64>: the allocator pointer itself at
// 0x011DE4E0, made on first use (a 60-byte ApeTFreeList of 64 8-byte defs).
constexpr u32 kSkelDefAlloc = 0x011DE4E0;
constexpr u32 kSkelDefFreeListVtbl = 0x00B24E18;
constexpr u32 kStrSkelDefFreeList = 0x00AF493E;  // "static ApeTFreeList<T> &ApeTMemClassFL<T, size>::GetAllocator() [with T = ApeSkeletonDef, size = 64U]"
constexpr u32 kSkelDefSize = 8, kSkelDefPerList = 64;
constexpr u32 kVfnAllocNew = 12;                 // (alloc) -> element or 0
constexpr u32 kSkelDefRefs = 0;                  // u16
// A bone def: +0 the bind matrix (only its x / y / z lanes are set), +64
// u32, +68 u8, +100 s16 the parent (-1) (name?).
constexpr u32 kBdWord64 = 64, kBdByte68 = 68, kBdParent = 100;

}  // namespace

// {AglTopology*, f32}* GeomDef_SelectLod(ApeGeomDef* d, Item* item)
// The first LOD whose far distance is beyond the item's distance from the
// camera (less the definition's radius), or 0. With a fade band, the
// item's +69 fade byte is scaled down near the near fade distance and near
// the last LOD's far distance; the result is 0 when the fade drops to 1/255
// or below there, or when the byte ends up 0 (was "GeomDef_FindTopo").
DECOMP(00585B98, GeomDef_SelectLod) {
    const u32 d = arg<u32>(ctx, 0), item = arg<u32>(ctx, 1);
    const u32 cam = obj11::kCameraPos;
    const f32 dy = Fsubs(Lf(item + kItemPos + 4), Lf(cam + 4));
    const f32 dx = Fsubs(Lf(item + kItemPos), Lf(cam));
    const f32 dz = Fsubs(Lf(item + kItemPos + 8), Lf(cam + 8));
    const f32 len = Fsqrts(Fmadds(dz, dz, Fmadds(dx, dx, Fmuls(dy, dy))));
    const f32 dist = Fsubs(len, Lf(d + kGdRadius));
    u32 n = load<u8>(d + kGdLodCount);
    const f32 zero = Lf(kSlConsts + kSlZero);
    u32 e = 0;
    for (u32 i = 0; i < n; ++i) {
        const u32 c = d + obj2::kClassTopos + i * 8;
        if (Fsubs(Lf(c + kLodDist), dist) > zero) {
            e = c;
            break;
        }
    }
    if (e == 0) return ret(ctx, 0u);
    f32 band = Lf(d + kGdFadeBand);
    if (band > Lf(kSlConsts + kSlTiny)) {
        const f32 one = Lf(kSlConsts + kSlOne);
        const f32 near = Lf(d + kGdFadeNear);
        if (near > zero && dist < near) {
            const f32 t = Fsubs(one, Fdivs(Fsubs(near, dist), band));
            const f32 m = Min1(t, one);
            const f32 lo = Lf(d + kGdFadeMin);
            const f32 k = lo < m ? Min1(t, one) : lo;
            if (k <= Lf(kSlConsts + kSlMinFade)) return ret(ctx, 0u);
            ScaleFade(item, k);
            band = Lf(d + kGdFadeBand);
            n = load<u8>(d + kGdLodCount);
        }
        const f32 far = Lf(d + 124 + ((n << 3) & 0x7F8) + kLodDist);  // the last LOD's distance
        if (dist > Fsubs(far, band)) {
            const f32 t = Fdivs(Fsubs(far, dist), band);
            if (t <= Lf(kSlConsts + kSlMinFade)) return ret(ctx, 0u);
            const f32 m = Min1(t, one);
            ScaleFade(item, m > zero ? Min1(t, one) : zero);
        }
    }
    ret(ctx, load<u8>(item + kItemFade) == 0 ? 0u : e);
}

// ApeSkeletonDef* SkeletonDef_New(u32 bones)
// A skeleton definition from its free list (made on first use; as in the
// original a failed Mem_Alloc stores a null allocator, which is then
// called through) with one reference, `bones` and 112-byte bone defs, each
// with the identity bind matrix (x / y / z lanes), parent -1. A failed bone
// allocation leaves +4 = 0. 0 when the free list is empty.
DECOMP(00584FD4, SkeletonDef_New) {
    const u32 bones = arg<u32>(ctx, 0);
    u32 alloc = load<u32>(kSkelDefAlloc);
    if (alloc == 0) {
        const u32 o = static_cast<u32>(call(ctx, obj6::o6fn::Mem_Alloc, {60, 0, 16}));
        if (o != 0) {
            for (u32 off : {8u, 4u, 12u, 20u, 16u}) store<u32>(o + off, 0);
            const u16 h54 = load<u16>(o + 54);
            for (u32 off : {24u, 32u, 28u, 36u, 40u, 44u, 48u}) store<u32>(o + off, 0);
            store<u16>(o + 52, 0);
            store<u16>(o + 54, static_cast<u16>(h54 & 0x7FFF));
            store<u32>(o, kSkelDefFreeListVtbl);
            store<u32>(o + 56, kStrSkelDefFreeList);
            call(ctx, obj6::o6fn::FreeList_Init, {o, kSkelDefSize, kSkelDefPerList, 0, 0});
        }
        alloc = o;
        store<u32>(kSkelDefAlloc, alloc);
    }
    const u32 opd = load<u32>(load<u32>(alloc) + kVfnAllocNew);
    const u32 p = static_cast<u32>(call_ptr(ctx, opd, {alloc, opd, load<u32>(opd)}));
    if (p == 0) return ret(ctx, 0u);
    store<u16>(p + kSkelDefRefs, 1);
    store<u16>(p + obj6::kSkelDefBoneCount, static_cast<u16>(bones));
    const u32 arr = static_cast<u32>(call(ctx, obj6::o6fn::Mem_Alloc, {bones * obj6::kBoneDefSize, 0, 16}));
    if (arr != 0 && static_cast<s32>(bones) > 0) {
        for (u32 i = 0; i < bones; ++i) {
            const u32 b = arr + i * obj6::kBoneDefSize;
            for (u32 off : {0u, 4u, 8u, 16u, 20u, 24u, 32u, 36u, 40u, 48u, 52u})
                store<u32>(b + off, load<u32>(kIdentity + off));
            const u32 w56 = load<u32>(kIdentity + 56);
            store<u32>(b + kBdWord64, 0);
            store<u32>(b + 56, w56);
            store<u16>(b + kBdParent, 0xFFFF);
            store<u8>(b + kBdByte68, 0);
        }
    }
    store<u32>(p + obj6::kSkelDefBones, arr);
    ret(ctx, p);
}

// void WorldObj_UpdateLight(WorldObj* o)
// The lit object's light cache (+288), refreshed at most once a frame: made
// on first use (Light_Alloc, then vfunc +36); with the cache still
// settling (+212 bit 0x80) and stamped past the first two thresholds, only
// the blend / apply steps run, when its slot countdown has not run out or
// the camera is within the near / far squared distances (beyond the far
// one only the blend, and only while the stamp is below the third
// threshold); otherwise all four steps run. Every path but the same-frame
// one restamps the cache (name?).
DECOMP(005832FC, WorldObj_UpdateLight) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 cfg = g::config;
    if (load<u32>(o + kObjLit) == 0) {
        ctx.gpr[3] = 0;
        return;
    }
    auto stamp = [&] { store<u32>(load<u32>(o + obj5::kObjLight) + kLcStamp, load<u32>(cfg + kCfgFrame)); };
    const u32 lc = load<u32>(o + obj5::kObjLight);
    bool full = true;
    if (lc == 0) {
        const u32 vt = load<u32>(o);
        store<u32>(o + obj5::kObjLight, static_cast<u32>(call(ctx, obj5::o5fn::Light_Alloc, {})));
        const u32 opd = load<u32>(vt + kVfnLightChanged);
        call_ptr(ctx, opd, {o, opd, load<u32>(opd)});
    } else {
        const u32 st = load<u32>(lc + kLcStamp), now = load<u32>(cfg + kCfgFrame);
        if (st == now) {
            call(ctx, o12fn::ObjLight_Apply, {o});
            return;
        }
        const u8 fl = load<u8>(lc + kLcFlags);
        if ((fl & kLcSettling) != 0) {
            if (st < load<u32>(cfg + kCfgStampA)) {
                store<u8>(lc + kLcFlags, static_cast<u8>(fl & ~kLcSettling));
            } else if (st >= load<u32>(cfg + kCfgStampB)) {
                const s32 d = static_cast<s32>((now - st) & 0xFF);
                const s32 slots = load<u8>(cfg + kCfgSlots);
                const u32 cd = lc + kLcCountdown;
                bool partial = false;
                if (slots != 0) {
                    const s32 left = static_cast<s32>(load<u8>(cd)) - (d < slots ? d : slots);
                    if (left >= 0) {
                        store<u8>(cd, static_cast<u8>(left));
                        call(ctx, o12fn::ObjLight_Blend, {o});
                        call(ctx, o12fn::ObjLight_Apply, {o});
                        stamp();
                        return;
                    }
                    store<u8>(cd, static_cast<u8>(left + slots));
                }
                const u32 cam = static_cast<u32>(call(ctx, obj11::o11fn::View_GetCurrent, {})) + kViewCamPos;
                const u32 pos = o + kObjLightPos;
                const f32 dy = Fsubs(Lf(cam + 4), Lf(pos + 4)), dx = Fsubs(Lf(cam), Lf(pos)), dz = Fsubs(Lf(cam + 8), Lf(pos + 8));
                const f32 d2 = Fmadds(dz, dz, Fmadds(dx, dx, Fmuls(dy, dy)));
                if (d2 > Lf(cfg + kCfgNear2)) {
                    partial = true;
                    if (d2 < Lf(cfg + kCfgFar2)) {
                        call(ctx, o12fn::ObjLight_Blend, {o});
                        call(ctx, o12fn::ObjLight_Apply, {o});
                    } else if (load<u32>(load<u32>(o + obj5::kObjLight) + kLcStamp) < load<u32>(cfg + kCfgStampC)) {
                        call(ctx, o12fn::ObjLight_Blend, {o});
                    }
                }
                if (partial) {
                    stamp();
                    return;
                }
            }
        }
    }
    if (full) {
        stamp();
        call(ctx, o12fn::ObjLight_Collect, {o});
        call(ctx, o12fn::ObjLight_Sort, {o});
        call(ctx, o12fn::ObjLight_Blend, {o});
        call(ctx, o12fn::ObjLight_Apply, {o});
    }
}

namespace {

// The draw packet both submit paths build at sp+112 (obj12.h kDp*), with
// b69 / b70 for its +69 / +70 bytes, then vfunc +28's texture record
// applied (DrawPacket_SetTexture).
void BuildPacket(PPUContext& ctx, u32 o, u32 pk, u32 m, u32 flags, u32 b69, u32 b70) {
    for (u32 row = 0; row < 4; ++row)
        for (u32 k = 0; k < 3; ++k) store<u32>(pk + 16 * row + 4 * k, load<u32>(m + 16 * row + 4 * k));
    u32 bits = (flags & kObjDpFlag) != 0 ? 0x80u : 0u;
    store<u32>(pk + 64, load<u32>(o + obj5::kObjLight));
    store<u32>(pk + 68, 0);
    store<u32>(pk + 80, 0);
    store<u32>(pk + 84, 0);
    store<u8>(pk + 88, 0);
    store<u8>(pk + 89, 0);
    store<u32>(pk + 92, 0);
    store<u8>(pk + 68, load<u8>(o + obj::kObjGroupWord));
    store<u8>(pk + 69, static_cast<u8>(b69));
    store<u8>(pk + 70, static_cast<u8>(b70));
    const u32 b322 = load<u8>(o + 322), b321 = load<u8>(o + 321);
    bits |= (b321 & 1) << 6;
    bits |= ((b322 >> 7) & 1) << 4;
    store<u8>(pk + 71, static_cast<u8>(bits));
    for (u32 k = 0; k < 4; ++k) store<u8>(pk + 72 + k, load<u8>(o + obj3::kObjColor + k));
    store<u32>(pk + 76, load<u32>(o + obj5::kObj296));
    const u32 opd = load<u32>(load<u32>(o) + kVfnDrawTexture);
    const u32 t = static_cast<u32>(call_ptr(ctx, opd, {o, opd, load<u32>(opd)}));
    if (t != 0) call(ctx, o12fn::DrawPacket_SetTexture, {pk, load<u32>(t + 44), load<u16>(t + 28)});
}

// The packet's matrix: WorldObj_AnimMatrix for +164 bits 6-7 (flags then
// re-read), otherwise the slot entry's or +80.
u32 PacketMatrix(PPUContext& ctx, u32 o, u32& flags) {
    flags = load<u32>(o + obj::kObjFlags);
    if (((flags >> 6) & 3) != 0) {
        const u32 m = static_cast<u32>(call(ctx, o12fn::WorldObj_AnimMatrix, {o}));
        flags = load<u32>(o + obj::kObjFlags);
        return m;
    }
    const u32 slot = load<u16>(o + obj11::kObjSlotIndex);
    return slot == 0xFFFF ? o + kObjOwnMatrix : load<u32>(obj3::kSlotTable) + (slot << 7);
}

// After a draw: vfunc +32 (o, frame) and, with a parent link (+260), the
// parent's. The object's call leaves the entry / OPD in r5 / r6
// (entry_first) or r6 / r5, as the two originals do.
void NotifyDrawn(PPUContext& ctx, u32 o, bool entry_first) {
    const u32 opd = load<u32>(load<u32>(o) + kVfnDrawn);
    if (entry_first)
        call_ptr(ctx, opd, {o, load<u32>(kDrawFrame), load<u32>(opd), opd});
    else
        call_ptr(ctx, opd, {o, load<u32>(kDrawFrame), opd, load<u32>(opd)});
    if (load<u32>(o + obj::kObjQueued) != 0) {
        const u32 p = load<u32>(o + obj::kObjParent);
        const u32 popd = load<u32>(load<u32>(p) + kVfnDrawn);
        call_ptr(ctx, popd, {p, load<u32>(kDrawFrame), popd, load<u32>(popd)});
    }
}

// The draw callback (+332) with {0, +336, packet} at pk + off (sp+208 or
// sp+220).
void DrawCallback(PPUContext& ctx, u32 o, u32 pk, u32 off = 96) {
    const u32 cb = load<u32>(o + kObjDrawCb);
    if (cb == 0) return;
    const u32 a = pk + off;
    store<u32>(a, 0);
    store<u32>(a + 4, load<u32>(o + kObjDrawCbArg));
    store<u32>(a + 8, pk);
    call_ptr(ctx, cb, {o, a, cb, load<u32>(cb), pk, load<u32>(o + kObjDrawCbArg)});
}

}  // namespace

// void WorldObj_SubmitDraw(WorldObj* o)
// Builds the object's draw packet (obj12.h) from its matrix (the animated
// one, its slot entry's, or +80; +69 = obj +168, +70 = obj +320 bits
// 18-23), lets vfunc +28's texture record set it up, and hands it to the
// class's submit slot (+20); when drawn, NotifyDrawn. Then the draw
// callback. The packet's w lanes and unset bytes are whatever the stack
// held, as in the original (name?).
DECOMP(0058303C, WorldObj_SubmitDraw) {
    const u32 o = arg<u32>(ctx, 0);
    u32 flags;
    const u32 m = PacketMatrix(ctx, o, flags);
    Stack frame(ctx, 240 - 112);
    const u32 pk = frame.addr();  // sp+112 of the original's 240-byte frame
    BuildPacket(ctx, o, pk, m, flags, load<u8>(o + 168), (load<u32>(o + obj::kObjGroupWord) >> 18) & 0x3F);
    const u32 cls = load<u32>(o + obj4::kObjClassPtr);
    const u32 sub = load<u32>(load<u32>(cls) + kVfnClassSubmit);
    if (static_cast<u32>(call_ptr(ctx, sub, {cls, pk, sub, load<u32>(sub)})) != 0) NotifyDrawn(ctx, o, true);
    DrawCallback(ctx, o, pk);
}

// void GeomInstance_DrawLod(GeomInstance* o, u8 lod, u8 b69, u8 b70)
// The same packet with the given +69 / +70 bytes, drawn through
// Topology_Apply on the class's LOD `lod` (one finer when g::config +34 is
// set, vfunc +28 has a record and there is one), then NotifyDrawn; without
// a class only the callback (was "GeomCmd_Simple") (name?).
DECOMP(0058AC70, GeomInstance_DrawLod) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 lod0 = arg<u32>(ctx, 1), b69 = arg<u32>(ctx, 2), b70 = arg<u32>(ctx, 3);
    u32 flags;
    const u32 m = PacketMatrix(ctx, o, flags);
    Stack frame(ctx, 256 - 112);
    const u32 pk = frame.addr();  // sp+112
    BuildPacket(ctx, o, pk, m, flags, b69, b70);
    u32 lod = lod0 & 0xFF;
    const u32 cls = load<u32>(o + obj4::kObjClassPtr);
    if (cls != 0) {
        if (load<u8>(g::config + kCfgFinerLod) != 0) {
            const u32 opd = load<u32>(load<u32>(o) + kVfnDrawTexture);
            if (static_cast<u32>(call_ptr(ctx, opd, {o, opd, load<u32>(opd)})) != 0 &&
                static_cast<s32>(lod + 1) < static_cast<s32>(load<u8>(cls + kGdLodCount)))
                lod = (lod + 1) & 0xFF;
        }
        call(ctx, o12fn::Topology_Apply, {load<u32>(cls + obj2::kClassTopos + (lod << 3)), pk});
        NotifyDrawn(ctx, o, false);
    }
    DrawCallback(ctx, o, pk);
}

namespace {

// Hands one draw to its bucket: DrawItem_Create, then bucket vfunc +8
// (bucket, item, key) with key = (s8 layer << 8) | kind; the kind is the
// packet's +70, else the material's +45, else 4 (material +104 bit 31), 0
// (packet alpha 255) or 6.
void SubmitItem(PPUContext& ctx, u32 rec, u32 mat, u32 pk) {
    const u32 item = static_cast<u32>(call(ctx, o12fn::DrawItem_Create, {rec, mat, pk}));
    u32 kind = load<u8>(pk + kDpKind);
    if (kind == 0) {
        const u32 mk = load<u8>(mat + kMatKind);
        const u32 alpha = load<u8>(pk + kDpAlpha);
        if (mk != 0) kind = mk;
        else if ((load<u32>(mat + kMatWord104) >> 31) != 0) kind = 4;
        else kind = alpha == 255 ? 0 : 6;
    }
    const s32 layer = static_cast<s8>(load<u8>(pk + kDpLayer));
    const u32 key = (static_cast<u32>(layer) << 8) | (kind & 0xFF);
    const u32 bucket = load<u32>(kDrawBuckets + ((key & 0xFF) << 2));
    const u32 opd = load<u32>(load<u32>(bucket) + 8);
    call_ptr(ctx, opd, {bucket, item, key, opd, load<u32>(opd)});
}

}  // namespace

// void Topology_Apply(AglTopology* t, DrawPacket* pk)
// Queues the topology's draws, last first: DrawPacket_Begin, the record
// (+92) through DrawRecord_Submit while the static draw-record list is
// enabled (set up on first use), then each draw whose material is visible
// (+100 bit 0x20) and, unless the packet's layer mask is -1, whose
// record's layer bit is in the mask (re-read after each draw) (name?).
DECOMP(005099B8, Topology_Apply) {
    const u32 t = arg<u32>(ctx, 0), pk = arg<u32>(ctx, 1);
    if (load<u16>(t + obj6::kTopoDrawCount) == 0) return;
    call(ctx, o12fn::DrawPacket_Begin, {pk});
    u32 rec = load<u32>(t + obj6::kTopoRecord);
    if (rec != 0) {
        if (load<u8>(kTrListGuard) == 0) {
            store<u8>(kTrList, 1);
            call(ctx, o12fn::PtrVector_Construct, {kTrList + 4});
            store<u16>(kTrList + 10, 160);
            store<u32>(kTrList + 32, 0);
            store<u32>(kTrList + 128, 0);  // lwarx / stwcx. stores in the original
            store<u32>(kTrList + 256, 0);
            store<u8>(kTrListGuard, 1);
            rec = load<u32>(t + obj6::kTopoRecord);
        }
        if (load<u8>(kTrList) != 0) call(ctx, o12fn::DrawRecord_Submit, {rec, pk});
    }
    u32 i = load<u16>(t + obj6::kTopoDrawCount) - 1;
    s32 mask = static_cast<s32>(load<u32>(pk + kDpLayerMask));
    if (mask == -1) {
        for (;; --i) {
            const u32 e = load<u32>(t + obj6::kTopoDraws) + i * 8;
            const u32 mat = load<u32>(e + 4), r = load<u32>(e);
            if ((load<u8>(mat + kMatFlags100) & kMatVisible) != 0) SubmitItem(ctx, r, mat, pk);
            if (i == 0) break;
        }
        return;
    }
    for (;; --i) {
        const u32 e = load<u32>(t + obj6::kTopoDraws) + i * 8;
        const u32 r = load<u32>(e);
        const u32 sh = load<u8>(r + kDrLayer) & 0x3F;
        const u32 bit = sh < 32 ? 1u << sh : 0u;
        if ((static_cast<u32>(mask) & bit) != 0) {
            const u32 mat = load<u32>(e + 4);
            if ((load<u8>(mat + kMatFlags100) & kMatVisible) != 0) {
                SubmitItem(ctx, r, mat, pk);
                mask = static_cast<s32>(load<u32>(pk + kDpLayerMask));
            }
        }
        if (i == 0) break;
    }
}

// void LightGrid_Rebuild()
// Once a frame: buckets the active light slots into an 8 x 8 grid by the
// cells LightSlot_CellRange gives them (on the stack: 64 cursors at
// sp+128 into 256-byte cell buffers at sp+384 of the original's 16848-byte
// frame; the cell coordinates are not range checked, as in the original),
// then packs the cells into the list (at most 1024 indices; each cell's
// count is stored uncapped) (name?).
DECOMP(00583518, LightGrid_Rebuild) {
    const u32 g = cls::kLightSlotLists;
    const u32 now = load<u32>(g::config + kCfgFrame);
    if (load<u32>(g + kLgFrame) == now) return;
    store<u32>(g + kLgFrame, now);
    Stack frame(ctx, 16848);
    const u32 sp = frame.addr();
    const u32 cur = sp + 128, bufs = sp + 384;
    const u32 base = load<u32>(g + kLgSlots);
    const u32 count = load<u16>(g + kLgCount);
    for (u32 k = 0; k < kLgGrid * kLgGrid; ++k) store<u32>(cur + 4 * k, bufs + kLgCellBytes * k);
    const u32 end = count == 0xFFFF ? 0u : base + ((count << 6) & 0x3FFFC0u);
    for (u32 p = base; p != end; p += kLightSlotSize) {
        if ((load<u8>(p + kLsFlags62) & kLsActive) == 0) continue;
        call(ctx, o12fn::LightSlot_CellRange, {p, sp + 112, sp + 116, sp + 120, sp + 124});
        const s32 x0 = static_cast<s32>(load<u32>(sp + 112)), x1 = static_cast<s32>(load<u32>(sp + 116));
        const s32 y1 = static_cast<s32>(load<u32>(sp + 124)), y0 = static_cast<s32>(load<u32>(sp + 120));
        if (x0 > x1) continue;
        const u16 idx = static_cast<u16>(static_cast<s32>(p - base) / static_cast<s32>(kLightSlotSize));
        for (s32 x = x0; x <= x1; ++x) {
            if (y0 > y1) continue;
            for (s32 y = y0; y <= y1; ++y) {
                const u32 slot = cur + static_cast<u32>(x) * 32 + static_cast<u32>(y) * 4;
                const u32 c = load<u32>(slot);
                store<u16>(c, idx);
                store<u32>(slot, c + 2);
            }
        }
    }
    const u32 list = g + kLgList;
    u32 out = list;
    for (u32 x = 0; x < kLgGrid; ++x)
        for (u32 y = 0; y < kLgGrid; ++y) {
            const u32 cell = g + kLgCells + 32 * x + 4 * y;
            const u32 buf = bufs + 2048 * x + kLgCellBytes * y;
            store<u16>(cell, static_cast<u16>(static_cast<s32>(out - list) / 2));
            const s32 n = static_cast<s32>(load<u32>(cur + 32 * x + 4 * y) - buf) / 2;
            store<u16>(cell + 2, static_cast<u16>(n));
            for (s32 k = 0; k < n; ++k) {
                const u16 v = load<u16>(buf + 2 * static_cast<u32>(k));
                if (static_cast<s32>(out - list) / 2 < static_cast<s32>(kLgListMax)) {
                    store<u16>(out, v);
                    out += 2;
                }
            }
        }
}

// void GeomInstance_DrawLodEx(GeomInstance* o, u8 lod, u8 b69, u8 b70,
//     Receiver** recv, s32 mode, u32 bits, u32 w84, u8 b89)
// GeomInstance_DrawLod with the packet's +84 word / +89 byte given, and the
// static draw-record list (set up by DrawRecordList_Init on first use)
// disabled for the call unless bits & 2. Mode 1 draws the LOD through
// Topology_ApplyRecv(topo, pk, *recv) when *recv is set, or Topology_Apply;
// any other mode through Topology_ApplyTo(topo, pk, recv). The callback's
// argument block is at sp+220 (mode 1) or sp+208. The ninth
// argument is the caller's stack word at sp+116 (was "GeomCmd_Full")
// (name?).
DECOMP(005A0E68, GeomInstance_DrawLodEx) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 lod0 = arg<u32>(ctx, 1), b69 = arg<u32>(ctx, 2), b70 = arg<u32>(ctx, 3);
    const u32 recv = arg<u32>(ctx, 4);
    const s32 mode = arg<s32>(ctx, 5);
    const u32 bits = arg<u32>(ctx, 6), w84 = arg<u32>(ctx, 7);
    const u32 b89 = load<u32>(static_cast<u32>(ctx.gpr[1]) + 116);
    if (load<u8>(kTrListGuard) == 0) {
        call(ctx, o12fn::DrawRecordList_Init, {kTrList});
        store<u8>(kTrListGuard, 1);
    }
    const bool keep = (bits & 2) != 0;
    const u8 saved = load<u8>(kTrList);
    if (!keep) store<u8>(kTrList, 0);
    u32 flags;
    const u32 m = PacketMatrix(ctx, o, flags);
    Stack frame(ctx, 320 - 112);
    const u32 pk = frame.addr();  // sp+112
    BuildPacket(ctx, o, pk, m, flags, b69, b70);
    store<u32>(pk + 84, w84);
    store<u8>(pk + 89, static_cast<u8>(b89));
    auto pick = [&](u32 cls) {
        u32 lod = lod0 & 0xFF;
        if (load<u8>(g::config + kCfgFinerLod) != 0) {
            const u32 opd = load<u32>(load<u32>(o) + kVfnDrawTexture);
            if (static_cast<u32>(call_ptr(ctx, opd, {o, opd, load<u32>(opd)})) != 0 &&
                static_cast<s32>(lod + 1) < static_cast<s32>(load<u8>(cls + kGdLodCount)))
                lod = (lod + 1) & 0xFF;
        }
        return load<u32>(cls + obj2::kClassTopos + (lod << 3));
    };
    if (mode == 1) {
        const u32 cls = load<u32>(o + obj4::kObjClassPtr);
        const u32 r = load<u32>(recv);
        if (cls != 0) {
            const u32 topo = pick(cls);
            if (r != 0) call(ctx, o12fn::Topology_ApplyRecv, {topo, pk, r});
            else call(ctx, o12fn::Topology_Apply, {topo, pk});
            NotifyDrawn(ctx, o, true);
        }
        DrawCallback(ctx, o, pk, 108);
    } else {
        const u32 cls = load<u32>(o + obj4::kObjClassPtr);
        if (cls != 0) {
            call(ctx, o12fn::Topology_ApplyTo, {pick(cls), pk, recv});
            NotifyDrawn(ctx, o, false);
        }
        DrawCallback(ctx, o, pk);
    }
    if (!keep) store<u8>(kTrList, saved);
}
