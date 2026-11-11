// Game code, large-function sweep part 5: the "CMdl%02d_%s" attachment
// models of an entity (0025C658).
#include "big5.h"

using namespace sly4;
using namespace sly4::big5;

namespace {
// The owner (o): +8 the entity, +56 + 360 i the attachment slots
// {+56 key, +60 u32 n, +96 + 40 m {u32 id, u32 model hash, ...}[n]},
// +2904 u8 the chosen variant per slot, +2912 WorldObj*[8] the models,
// u16 +2944 the slot count, u16 +2946 the geometry version wanted.
constexpr u32 kOwnEnt = 8;
constexpr u32 kSlot = 360;
constexpr u32 kSlotKey = 56, kSlotN = 60, kSlotIds = 96, kSlotHashes = 100, kVariant = 40;
constexpr u32 kOwnChosen = 2904;
constexpr u32 kOwnModels = 2912;
constexpr u32 kOwnCount = 2944;
constexpr u32 kOwnVersion = 2946;
// The entity
constexpr u32 kEntMatrix = 96;    // f32[16]
constexpr u32 kEntModel = 160;    // +144 its scale
constexpr u32 kEntSlot = 172;     // s8: the profile slot; word: 0x100 = (vfn[40] argument)
constexpr u32 kEntFlags = 188;    // bit 0: has attachments (the low half of the u64 at +184)
// The world's profile table
constexpr u32 kWorldPtr = 0x00F65CA0u;
constexpr u32 kProfStride = 0x20C80u, kProfIndex = 4208;  // u8 at W + slot * 0x20C80 + 4208
constexpr u32 kSelStride = 4056, kSelPairs = 432;         // {key, id}[8] at W + index * 4056 + 432
constexpr u32 kFmtCMdl = 0x00AD4BF4u;    // "CMdl%02d_%s"
constexpr u32 kUnitScale = 0x01164B30u;  // f32[3] (1, 1, 1)
constexpr u32 kGeomFindByHash = 0x005801A0u;  // (u32 hash) -> GeomInstance* or 0
constexpr u32 kWorldObjSetScale = 0x0057D100u;  // (wo, const f32* scale, 0)
constexpr u32 kWorldObjMarkDirty = 0x0057D000u;
constexpr u32 kWorldObjSetGroup = 0x00583E54u;  // (wo, group)
constexpr u32 kField160Kind8 = 0x000DA31Cu;     // (ent) -> u8
constexpr u32 kHashToString = 0x0056A6E0u;      // (hash) -> "0x%x"
constexpr u32 kGeomDefClass = 0x00F8F12Cu;      // +16 its map by name hash
constexpr u32 kHashMapFind2 = 0x007399A4u;      // (map, const u32* key) -> value slot
}  // namespace

// u32 Obj_RebuildCMdl(Owner* o)
// Releases the old attachment models (vfn[8] of each non-null
// o->2912[k], slot cleared). Then, for an entity with bit 0 of +188 and a
// model whose vfn[28] is non-null, for each slot with n != 0: the variant
// m whose id matches the profile's id for the slot's key (the last pair
// that matches wins; 0 by default) is recorded at +2904[i], and its model
// hash names the geometry: an existing instance (vfn[28] non-null with the
// wanted version at +28: vfn[40](0, 0), then vfn[56](13) is the model), or
// else the geometry def from the class map (a non-null +164 with the
// wanted version at +2: WorldObj_Instance(def, 0)). (Failures call the
// entity's vfn[76] and drop the result; with neither an instance nor a
// def, Hash_ToString(hash) too.) Each model made is named "CMdl%02d_%s"
// (1-based slot, the entity's vfn[76] name), vfn[40](flag 0x100 of
// +172), scaled like the entity's model (or 1), vfn[52](field), given the
// entity's matrix, marked dirty, flag 0x80 of +322 cleared with vfn[36],
// and put in group 10. The count is re-read every iteration. r3 / f1 are
// what the original leaves there.
DECOMP(0025C658, Obj_RebuildCMdl) {
    const u32 o = arg<u32>(ctx, 0);
    for (u32 k = 0; k < 8; ++k) {
        const u32 slot = o + kOwnModels + 4 * k;
        const u32 p = load<u32>(slot);
        if (p != 0) {
            VCall(ctx, p, 8, {p});
            store<u32>(slot, 0);
        }
    }
    const u32 ent = load<u32>(o + kOwnEnt);
    if ((load<u32>(ent + kEntFlags) & 1u) == 0) return ret(ctx, 0u);
    const u32 model = load<u32>(ent + kEntModel);
    if (model == 0) return ret(ctx, 0u);
    if (static_cast<u32>(VCall(ctx, model, 28, {model})) == 0) return ret(ctx, 0u);
    const u32 w = load<u32>(kWorldPtr);
    const s32 ps = static_cast<s8>(load<u8>(ent + kEntSlot));
    const u32 sel0 = static_cast<u32>(load<u8>(w + static_cast<u32>(ps) * kProfStride + kProfIndex)) * kSelStride;
    const u32 sel = w + sel0;
    if (load<u16>(o + kOwnCount) == 0) return ret(ctx, sel0);
    for (u32 i = 0; i < load<u16>(o + kOwnCount); ++i) {
        const u32 s = o + kSlot * i;
        if (load<u32>(s + kSlotN) == 0) continue;
        u32 best = 0;
        for (u32 j = 0; j < 8; ++j) {
            if (load<u32>(sel + kSelPairs + 8 * j) != load<u32>(s + kSlotKey)) continue;
            const u32 n = load<u32>(s + kSlotN);
            for (u32 m = 0; m < n; ++m) {
                if (load<u32>(sel + kSelPairs + 8 * j + 4) == load<u32>(s + kSlotIds + kVariant * m)) {
                    best = m;
                    break;
                }
            }
        }
        store<u8>(o + i + kOwnChosen, static_cast<u8>(best));
        const u32 hash = load<u32>(s + kSlotHashes + kVariant * best);
        if (hash == 0) continue;
        const u32 mslot = o + kOwnModels + 4 * i;
        const u32 inst = static_cast<u32>(call(ctx, kGeomFindByHash, {hash}));
        u32 def = 0;
        if (inst == 0) {
            Stack key(ctx, 16);  // r1+112
            store<u32>(key.addr(), hash);
            def = load<u32>(static_cast<u32>(call(ctx, kHashMapFind2, {load<u32>(load<u32>(kGeomDefClass) + 16), key.addr()})));
        }
        if (inst != 0) {
            const u32 t = static_cast<u32>(VCall(ctx, inst, 28, {inst}));
            if (t != 0 && load<u16>(static_cast<u32>(VCall(ctx, inst, 28, {inst})) + 28) == load<u16>(o + kOwnVersion)) {
                VCall(ctx, inst, 40, {inst, 0, 0});
                store<u32>(mslot, static_cast<u32>(VCall(ctx, inst, 56, {inst, 13})));
            } else {
                VCall(ctx, ent, 76, {ent});
            }
        } else if (def == 0) {
            VCall(ctx, ent, 76, {ent});
            call(ctx, kHashToString, {hash});
        } else {
            const u32 g = load<u32>(def + 164);
            if (g != 0 && load<u16>(g + 2) == load<u16>(o + kOwnVersion)) {
                store<u32>(mslot, static_cast<u32>(call(ctx, b5fn::kWorldObjInstance, {def, 0})));
            } else {
                VCall(ctx, ent, 76, {ent});
            }
        }
        const u32 wo = load<u32>(mslot);
        if (wo == 0) continue;
        const u32 name = static_cast<u32>(VCall(ctx, ent, 76, {ent}));
        const u32 str = static_cast<u32>(call(ctx, b5fn::kStringFormat, {kFmtCMdl, i + 1, name}));
        call(ctx, b5fn::kWorldObjSetName, {wo, str, 1});
        const u32 vis = (load<u32>(ent + kEntSlot) & 0x100u) != 0 ? 1u : 0u;
        VCall(ctx, wo, 40, {wo, vis, 0});
        const u32 em = load<u32>(ent + kEntModel);
        call(ctx, kWorldObjSetScale, {wo, em != 0 ? em + 144 : kUnitScale, 0});
        const u32 fv = static_cast<u32>(call(ctx, kField160Kind8, {ent}));
        VCall(ctx, wo, 52, {wo, fv, 0});
        CopyMat43W(wo + 16, ent + kEntMatrix);
        ctx.fpr[1] = std::bit_cast<f64>(LfsBits(load<u32>(ent + kEntMatrix + 56)));  // f1 as the copy leaves it
        call(ctx, kWorldObjMarkDirty, {wo});
        const u8 fl = load<u8>(wo + 322);
        if ((fl & 0x80u) != 0) {
            store<u8>(wo + 322, static_cast<u8>(fl & 0x7Fu));
            VCall(ctx, wo, 36, {wo});
        }
        call(ctx, kWorldObjSetGroup, {wo, 10});
    }
    ret(ctx, load<u16>(o + kOwnCount));
}
