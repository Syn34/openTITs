// World_HandleEvent callees, part 9a: the callees named by the hevent8
// helpers: the sub-mesh base constructor, the view-query job launcher, the
// pair-table lookup with the 0x00F35F98 default compare and the attach.
#include "hevent9.h"

using namespace sly4;
using namespace sly4::hevent;
using namespace sly4::hevent7;
using namespace sly4::hevent8;
using namespace sly4::hevent9;

namespace sly4 {
namespace hevent9a {
// Sub-mesh base: two vtables in turn, two instance counters.
constexpr u32 kBaseVtable = 0x00B242B8, kSubMeshBaseVtable = 0x00B23148;
constexpr u32 kSerialCounter = 0x01164CD0;   // u32, its new value goes in +8 bits 0..20 (name?)
constexpr u32 kIdCounter = 0x010DD580;       // u32, its old value goes in +80 (name?)
constexpr u32 kNotSetName = 0x00AF181C;      // "SubMeshName:Not set"
constexpr u32 kOne = 0x3F800000;             // f32 1.0

// The view-query job (SPURS job descriptor, 48 bytes then one DMA entry).
constexpr u32 kQueryImage = 0x00C6B100, kQueryImageSize = 0x1BB28;
constexpr u64 kQueryDmaEntry = 0x000000B000000000ull;  // {176, ea} (the 8 bytes at 0x00456910)
constexpr u32 kQueryPending = 160;  // Job +160: u32* pending counter

constexpr u32 kCompareU32a = 0x00F35F98;  // default compare: plain unsigned keys

// Object: +160 model (+28 vfn skeleton, +204 u32 bone key, +208 {+108 Rec80
// list}, +272 u32), +164 u32, +340 PtrVector* of 240-byte attach records
// (+192 holder).
constexpr u32 kObjModel = 160, kObj164 = 164, kObjAttachments = 340;
constexpr u32 kModelBoneKey = 204, kModelData = 208, kDataRecs = 108, kModel272 = 272;
constexpr u32 kRecSize = 240;
// The attach descriptor (see Obj_Reset121FA8): +160 bone hash, +168 second
// bone hash, +176 flags (bit 26: skip the effect link), +192 holder; the
// results +196 Rec80*, +200 s32 bone, +204 model, +208 Rec80*, +212 s32
// bone, +216 model, +220 u32, +224 f32.
constexpr u32 kDBone = 160, kDBone2 = 168, kDFlags = 176, kDHolder = 192;
constexpr u32 kDRec = 196, kDBoneIdx = 200, kDModel = 204, kDRec2 = 208, kDBoneIdx2 = 212, kDModel2 = 216;
constexpr u32 kD220 = 220, kD224 = 224;
constexpr u32 kDFlagNoLink = 0x04000000u;
constexpr u32 kZeroF = 0x000D15B8;        // f32 0.0
constexpr u32 kFindModelOpd = 0x00F31D20;  // WorldObj_ForEach callback ({key, result}*) (name?)
constexpr u32 kLinkComponent = 0x8C3DFCAA; // effect component type hash (name?)
constexpr s32 kLinkKind = 5;
// Vtable slots.
constexpr u32 kVGetLink = 100, kVLink = 104, kVGet108 = 108, kVSet112 = 112, kVAttachTo = 184, kVEffect = 272;
constexpr u32 kVKind = 32, kVRelease = 8, kVSkeleton = 28;
// Link component: +28 model (u16 use count at +4), +36 u32.
constexpr u32 kLinkModel = 28, kLink36 = 36, kModelUses = 4;
}  // namespace hevent9a
}  // namespace sly4

using namespace sly4::hevent9a;

namespace {
// One 16-byte sub-mesh slot {+0, +4 flags, +8 (bits 28..30), +12 (top 4
// bits)}: +0 = 0, +8 = 2 (only bits 28..30 kept, then set to 1 << 1), +4 = 1
// (bit 31 kept, then set), +12 loses its top 4 bits.
void slot_init(u32 b) {
    const u32 w12 = load<u32>(b + 12);
    store<u32>(b + 0, 0);
    store<u32>(b + 8, 2);
    store<u32>(b + 4, 1);
    store<u32>(b + 12, w12 & 0x0FFFFFFFu);
}
}  // namespace

// SubMesh* SubMesh_ConstructBase2_4FCE88(SubMesh* m) (name?): the base
// constructor inlined twice (vtable 0x00B242B8, then 0x00B23148), with the
// serial counter at 0x01164CD0 bumped into +8 and the id counter at
// 0x010DD580 into +80; three slots, the name "SubMeshName:Not set".
DECOMP(004FCE88, SubMesh_ConstructBase2_4FCE88) {
    const u32 m = arg<u32>(ctx, 0);
    const u32 w4 = load<u32>(m + 4);
    store<u16>(m + 4, 1);
    store<u32>(m + 4, (w4 & 0x000003FFu) | 0x00010000u | 0x00000800u);
    const u32 serial = load<u32>(kSerialCounter) + 1;
    store<u32>(m + 0, kBaseVtable);
    store<u32>(kSerialCounter, serial);
    const u32 w8 = load<u32>(m + 8);
    store<u32>(m + 16, 0);
    store<u32>(m + 12, 0);
    store<u32>(m + 0, kSubMeshBaseVtable);
    store<u32>(m + 8, (serial << 11) | (w8 & 0x3Fu));
    slot_init(m + 24);
    store<u32>(m + 40, 0);
    slot_init(m + 44);
    store<u32>(m + 60, 1);
    store<u32>(m + 64, 0);
    store<u32>(m + 76, 6);
    const u32 id = load<u32>(kIdCounter);
    store<u32>(m + 68, 0);
    store<u32>(m + 72, 0);
    store<u32>(m + 88, kOne);
    store<u32>(kIdCounter, id + 1);
    store<u8>(m + 109, 0);
    store<u8>(m + 108, 0);
    store<u32>(m + 80, id);
    {
        // The slot at +24 again, its stores in another order.
        const u32 b = m + 24;
        const u32 w12 = load<u32>(b + 12);
        store<u32>(b + 4, 1);
        store<u32>(b + 0, 0);
        store<u32>(b + 8, 2);
        store<u32>(b + 12, w12 & 0x0FFFFFFFu);
    }
    store<u32>(m + 20, kNotSetName);
    ret(ctx, m);
}

// void Job_Post456918(Job* job) (name?): launches the 176-byte view-query
// job (image 0x00C6B100) under the job queue lock, bumping the job's
// pending counter (*(job +160)) atomically first.
DECOMP(00456918, Job_Post456918) {
    const u32 job = arg<u32>(ctx, 0);
    call(ctx, h9fn::JobQueue_Lock);
    const u32 q = static_cast<u32>(call(ctx, h9fn::JobQueue_Get));
    const u32 d = static_cast<u32>(vcall(ctx, q, 16, {q}));  // allocate
    call(ctx, h8fn::Memset, {d, 0, 48});
    store<u16>(d + 30, 1024);
    store<u16>(d + 10, 8);     // DMA list size
    store<u32>(d + 20, 176);   // input size
    store<u8>(d + 44, 0);
    store<u16>(d + 8, static_cast<u16>(((kQueryImageSize + 15) >> 4) & 0x0FFFFFFF));  // in 16-byte units
    store<u64>(d + 0, kQueryImage);
    store<u64>(d + 48, kQueryDmaEntry | job);
    // lwarx / stwcx. loop in the original: atomic ++ of the pending count.
    const u32 pending = load<u32>(job + kQueryPending);
    for (;;) {
        const u32 old = mem::r32(pending);
        if (mem::cas32(pending, old, old + 1)) break;
    }
    vcall(ctx, q, 20, {q, d});  // push
    call(ctx, h9fn::JobQueue_Unlock);
}

// void* Map_Find7662A4(Table* t, const u32* key) (name?): Map_Find7665B4
// with the default compare 0x00F35F98: sorts first when flagged, then a
// binary search (unsigned keys, or the compare OPD on (entry, &key copy)).
// Returns &entry.value, or &t->miss (+16).
DECOMP(007662A4, Map_Find7662A4) {
    const u32 t = arg<u32>(ctx, 0), key = arg<u32>(ctx, 1);
    const u32 flags = load<u16>(t + kTabFlags);
    if (flags & kTabUnsorted) {
        store<u16>(t + kTabFlags, static_cast<u16>(flags & 0xFFFE));
        const u32 n = load<u16>(t + kTabCount);
        if (n != 0) call(ctx, h7fn::qsort, {load<u32>(t + kTabEntries), n, 8, load<u32>(t + kTabCompare)});
    }
    s32 hi = static_cast<s32>(load<u16>(t + kTabCount)) - 1, lo = 0;
    u32 base = load<u32>(t + kTabEntries);
    s32 found = -1;
    if (load<u32>(t + kTabCompare) == kCompareU32a) {
        while (hi >= lo) {
            const s32 mid = (hi + lo) >> 1;
            const u32 want = load<u32>(key), k = load<u32>(base + static_cast<u32>(mid) * 8);
            if (k > want) hi = mid - 1;
            else if (k < want) lo = mid + 1;
            else { found = mid; break; }
        }
    } else {
        Stack frame(ctx, 16);  // the key copy at r1+112
        store<u32>(frame.addr(), load<u32>(key));
        while (hi >= lo) {
            const s32 mid = (hi + lo) >> 1;
            const s32 c = static_cast<s32>(opdcall(ctx, load<u32>(t + kTabCompare), {base + static_cast<u32>(mid) * 8, frame.addr()}));
            if (c > 0) hi = mid - 1;
            else if (c < 0) lo = mid + 1;
            else {
                base = load<u32>(t + kTabEntries);  // reloaded on a match
                found = mid;
                break;
            }
        }
    }
    ret(ctx, found >= 0 ? base + static_cast<u32>(found) * 8 + 4 : t + kTabMiss);
}

namespace {
// The bone lookup: -1 without a key, a model or a skeleton.
s32 find_bone(PPUContext& ctx, u32 model, u32 key) {
    if (key == 0 || model == 0) return -1;
    const u32 skel = static_cast<u32>(vcall(ctx, model, kVSkeleton, {model}));
    if (skel == 0) return -1;
    return static_cast<s32>(call(ctx, h9fn::Skeleton_FindBone, {skel, key}));
}

// The model carrying a bone key: o's own (its +204 key matching), or one
// found under it by WorldObj_ForEach when its +272 is set; else 0.
u32 find_model(PPUContext& ctx, u32 model, u32 key) {
    if (model == 0) return 0;
    if (load<u32>(model + kModelBoneKey) == key) return model;
    if (load<u32>(model + kModel272) == 0) return 0;
    Stack frame(ctx, 16);  // {key, result} at r1+112 / r1+120
    store<u32>(frame.addr() + 0, key);
    store<u32>(frame.addr() + 4, 0);
    call(ctx, h9fn::WorldObj_ForEach, {model, kFindModelOpd, frame.addr(), 0});
    return load<u32>(frame.addr() + 4);
}
}  // namespace

// u32 Obj_Attach0D15C0(Object* o, AttachDesc* d) (name?): attaches o to
// the holder d->192 unless one of o's attach records (+340, 240 bytes
// each) already has it (then r3 = the holder). Unless d's flag bit 26 is
// set, o's link object (vtable +100) is given to the holder (+100 value to
// d+220, +104 link, +108 f32 to d+224, +112 with 0.0). Then the holder's
// +184 (o); its effect's (+272) link component (kind 5) takes o's model
// (released / use-counted) and o +164. The two bone keys (+160, +168) are
// resolved to a Rec80 record (in o's model data, or the holder's for the
// second), else to a bone index and the model carrying the key. Finally the
// record is added to o's attach vector (made on first use: 20 bytes, element
// size 240) by 0x007450E4, whose r3 is returned.
DECOMP(000D15C0, Obj_Attach0D15C0) {
    const u32 o = arg<u32>(ctx, 0), d = arg<u32>(ctx, 1);
    const u32 holder = load<u32>(d + kDHolder);
    if (holder == 0) {
        ret(ctx, 0u);
        return;
    }
    {
        const u32 list = load<u32>(o + kObjAttachments);
        const u32 flags = load<u32>(d + kDFlags);
        for (u32 i = 0, off = 0;; ++i, off += kRecSize) {
            const u32 n = list != 0 ? load<u16>(list + 8) : 0;
            if (!(i < n)) break;
            const u32 e = load<u32>(list) + off;
            if (load<u32>(e + kDHolder) == holder) {
                if (e != 0) {
                    ret(ctx, holder);
                    return;
                }
                break;
            }
        }
        if ((flags & kDFlagNoLink) == 0) {
            const u32 link = static_cast<u32>(vcall(ctx, o, kVGetLink, {o}));
            if (link != 0) {
                u32 h = load<u32>(d + kDHolder);
                store<u32>(d + kD220, static_cast<u32>(vcall(ctx, h, kVGetLink, {h})));
                h = load<u32>(d + kDHolder);
                vcall(ctx, h, kVLink, {h, link});
                h = load<u32>(d + kDHolder);
                vcall(ctx, h, kVGet108, {h});
                store<f32>(d + kD224, static_cast<f32>(ctx.fpr[1]));
                h = load<u32>(d + kDHolder);
                ctx.fpr[1] = lfs_f64(kZeroF);
                vcall(ctx, h, kVSet112, {h});
            }
        }
    }
    u32 h = load<u32>(d + kDHolder);
    vcall(ctx, h, kVAttachTo, {h, o});
    h = load<u32>(d + kDHolder);
    u32 fx = static_cast<u32>(vcall(ctx, h, kVEffect, {h}));
    if (fx != 0) {
        const u32 c = static_cast<u32>(call(ctx, h7fn::Effect_FindComponent, {fx, kLinkComponent}));
        if (c != 0 && static_cast<s32>(vcall(ctx, c, kVKind, {c})) == kLinkKind) {
            const u32 old = load<u32>(c + kLinkModel);
            const u32 model = load<u32>(o + kObjModel);
            if (old != 0) vcall(ctx, old, kVRelease, {old});
            store<u32>(c + kLinkModel, model);
            if (model != 0) store<u16>(model + kModelUses, static_cast<u16>(load<u16>(model + kModelUses) + 1));
            store<u32>(c + kLink36, load<u32>(o + kObj164));
        }
    }

    // First bone key: a record in o's model data, else a bone of o's model.
    u32 key = load<u32>(d + kDBone);
    if (key != 0) {
        u32 model = load<u32>(o + kObjModel);
        u32 rec = 0;
        if (model != 0) {
            const u32 recs = load<u32>(load<u32>(model + kModelData) + kDataRecs);
            if (recs != 0) {
                rec = static_cast<u32>(call(ctx, h9fn::Rec80_Find, {recs, key}));
                model = load<u32>(o + kObjModel);
                key = load<u32>(d + kDBone);
            }
            store<u32>(d + kDRec, rec);
        }
        if (rec == 0) {
            const s32 bone = find_bone(ctx, model, key);
            store<u32>(d + kDBoneIdx, static_cast<u32>(bone));
            if (bone == -1)
                store<u32>(d + kDModel, find_model(ctx, load<u32>(o + kObjModel), load<u32>(d + kDBone)));
        }
    }

    // Second bone key: a record in the holder's model data, else a bone of
    // o's model.
    key = load<u32>(d + kDBone2);
    if (key != 0) {
        const u32 hmodel = load<u32>(load<u32>(d + kDHolder) + kObjModel);
        u32 rec = 0;
        if (hmodel != 0) {
            const u32 recs = load<u32>(load<u32>(hmodel + kModelData) + kDataRecs);
            if (recs != 0) {
                rec = static_cast<u32>(call(ctx, h9fn::Rec80_Find, {recs, key}));
                key = load<u32>(d + kDBone2);
            }
            store<u32>(d + kDRec2, rec);
        }
        if (rec == 0) {
            const s32 bone = find_bone(ctx, load<u32>(o + kObjModel), key);
            store<u32>(d + kDBoneIdx2, static_cast<u32>(bone));
            if (bone == -1)
                store<u32>(d + kDModel2, find_model(ctx, load<u32>(o + kObjModel), load<u32>(d + kDBone2)));
        }
    }

    u32 vec = load<u32>(o + kObjAttachments);
    if (vec == 0) {
        vec = static_cast<u32>(call(ctx, h7fn::SmallAlloc, {20}));
        store<u32>(o + kObjAttachments, vec);
        if (vec != 0) {
            call(ctx, h9fn::PtrVector_Construct, {vec});
            store<u16>(vec + 6, static_cast<u16>(kRecSize));
            vec = load<u32>(o + kObjAttachments);
        }
    }
    call(ctx, h9fn::Attach_Add7450E4, {vec, d});
}
