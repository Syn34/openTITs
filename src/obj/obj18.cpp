// World objects, part 18: the bank's draw extent and the nearest spline
// instance.
#include "obj18.h"

using namespace sly4;
using namespace sly4::obj18;

namespace {

inline f32 Fsubs(f64 a, f64 b) { return static_cast<f32>(a - b); }

}  // namespace

// ---------------------------------------------------------------- Bank_ComputeDrawExtent
namespace {
constexpr u32 kDeConsts = 0x0058DBDC;        // f32 {FLT_MAX, -FLT_MAX}
constexpr u32 kDeSlots = 0x01193004;         // obj4.h kSlotTableBase: +0 the 128-byte slot entries, +4 u16 their count
constexpr u32 kDeSlotSize = 128, kDeSlotObj = 96;
constexpr u32 kDeOut = 0x0118E080;           // the work record (obj17 kPrWork): +48 / +64 the extent box (name?)
constexpr u32 kDeBoxMin = 48, kDeBoxMax = 64;
constexpr u32 kDeMinY = 0xBF800000, kDeMaxY = 0x41200000;  // -1.0 / 10.0
constexpr u32 kDeGroup = 320;                // obj u32: bits 18-23 the group (obj.h kObjGroupWord)
constexpr u32 kDeClass = 208, kDeClassTopo0 = 132, kDeClassBox = 16;
constexpr u32 kDeObjAlpha = 168;             // u8: 255 = opaque (name?)
constexpr u32 kDeObjFlags = 164, kDeVisible = 0x8000;
constexpr u32 kDeObjWorld = 80;              // f32[4][4] the world matrix
constexpr u32 kTopoDraws = 48, kTopoDrawCount = 52;  // obj6.h: {u32, AglMaterial*}[u16 count]
constexpr u32 kMatKind = 45, kMatWord104 = 104;      // obj12.h: the forced bucket / bit 31 = bucket 4
constexpr u32 kFltMax = 0x7F7FFFFF, kFltLowest = 0xFF7FFFFF;

// Topology_Apply's bucket choice for one material: its forced bucket, 4
// when +104 bit 31 is set, else 0 for opaque objects and 6 otherwise.
u32 MaterialBucket(u32 mat, bool opaque) {
    const u32 kind = load<u8>(mat + kMatKind);
    if (kind != 0) return kind;
    if ((load<u32>(mat + kMatWord104) >> 31) != 0) return 4;
    return opaque ? 0 : 6;
}
}  // namespace

// void Bank_ComputeDrawExtent()
// The x / z extent of every slot entry's object that is visible, in group
// 0 and whose first topology draws only into bucket 0 (each object's
// class box through its world matrix), stored with y -1 .. 10 as the box
// at 0x0118E080 +48 / +64. With no such object the box keeps the
// FLT_MAX / -FLT_MAX starting values. The fsel merges keep the original's
// NaN behaviour (a NaN difference takes the new value).
DECOMP(0058DBE8, Bank_ComputeDrawExtent) {
    u32 minx = load<u32>(kDeConsts), minz = minx;          // f29 / f28
    u32 maxx = load<u32>(kDeConsts + 4), maxz = maxx;      // f31 / f30
    const u32 n = load<u16>(kDeSlots + 4);
    u32 slot = load<u32>(kDeSlots);
    for (u32 i = 0; i < n; ++i, slot += kDeSlotSize) {
        const u32 o = load<u32>(slot + kDeSlotObj);
        bool ok = ((load<u32>(o + kDeGroup) >> 18) & 0x3F) == 0;  // rlwinm 14,26,31
        const u32 cls = load<u32>(o + kDeClass);
        const bool opaque = load<u8>(o + kDeObjAlpha) == 255;
        if (cls != 0) {
            const u32 topo = load<u32>(cls + kDeClassTopo0);
            if (topo != 0) {
                const u32 nd = load<u16>(topo + kTopoDrawCount);
                for (u32 j = 0; j < nd; ++j) {  // cmplw
                    const u32 mat = load<u32>(load<u32>(topo + kTopoDraws) + j * 8 + 4);
                    if (mat == 0) continue;
                    if (MaterialBucket(mat, opaque) != 0) ok = false;
                }
            }
        }
        if ((load<u32>(o + kDeObjFlags) & kDeVisible) == 0 || !ok) continue;
        Stack frame(ctx, 32);  // sp+112: the box {min[4], max[4]} (the w lanes left unset)
        const u32 box = frame.addr();
        for (u32 k = 0; k < 3; ++k) store<u32>(box + 4 * k, kFltMax);
        for (u32 k = 0; k < 3; ++k) store<u32>(box + 16 + 4 * k, kFltLowest);
        call(ctx, o18fn::Box_Transform, {box, load<u32>(o + kDeClass) + kDeClassBox, o + kDeObjWorld});
        const f32 bminx = load<f32>(box), bminz = load<f32>(box + 8);
        const f32 bmaxx = load<f32>(box + 16), bmaxz = load<f32>(box + 24);
        const f32 cminx = std::bit_cast<f32>(minx), cminz = std::bit_cast<f32>(minz);
        const f32 cmaxx = std::bit_cast<f32>(maxx), cmaxz = std::bit_cast<f32>(maxz);
        if (Fsubs(cminx, bminx) >= 0.0f) minx = std::bit_cast<u32>(bminx);    // fsel f29, f29 - min.x, min.x, f29
        if (Fsubs(cminz, bminz) >= 0.0f) minz = std::bit_cast<u32>(bminz);
        if (!(Fsubs(cmaxx, bmaxx) >= 0.0f)) maxx = std::bit_cast<u32>(bmaxx);  // fsel f31, f31 - max.x, f31, max.x
        if (!(Fsubs(cmaxz, bmaxz) >= 0.0f)) maxz = std::bit_cast<u32>(bmaxz);
    }
    store<u32>(kDeOut + kDeBoxMin, minx);
    store<u32>(kDeOut + kDeBoxMin + 4, kDeMinY);
    store<u32>(kDeOut + kDeBoxMin + 8, minz);
    store<u32>(kDeOut + kDeBoxMax, maxx);
    store<u32>(kDeOut + kDeBoxMax + 4, kDeMaxY);
    store<u32>(kDeOut + kDeBoxMax + 8, maxz);
    ret(ctx, kDeOut);  // r3 as the original leaves it
}

// ---------------------------------------------------------------- SplineInstance_FindNearest
namespace {
constexpr u32 kFnConsts = 0x005A8CEC;        // f32 {the starting distance, the starting t}
constexpr u32 kSiWorld = 80;                 // ApeSplineInstance: f32[4][4] the world matrix
constexpr u32 kSiClass = 208;                // ApeSplineDef*: +112 the SplineData
constexpr u32 kSdData = 112;
}  // namespace

// ApeSplineInstance* SplineInstance_FindNearest(const f32 pos[3], f32* t_out,
//     f32* dist_out, Opd* filter(inst, arg) -> bool, void* arg)
// Walks the spline instance free list (made on first use, as
// ApeTListAllocator<ApeSplineInstance>::GetAllocator() inlines it) and
// returns the instance whose spline passes nearest to pos (pos taken into
// the instance's frame), skipping those the filter rejects; the nearest t
// and distance go to the optional outputs (the starting values when none
// is nearer).
DECOMP(005A8CF8, SplineInstance_FindNearest) {
    const u32 pos = arg<u32>(ctx, 0), t_out = arg<u32>(ctx, 1), dist_out = arg<u32>(ctx, 2);
    const u32 filter = arg<u32>(ctx, 3), fn_arg = arg<u32>(ctx, 4);
    u32 best_dist = load<u32>(kFnConsts);  // f30
    u32 best_t = load<u32>(kFnConsts + 4); // f31
    const u32 alloc = obj6::ListAllocator(ctx, obj8::kSplineInstList, obj8::kSplineInstListVtbl,
                                          obj8::kStrSplineInstList, obj8::kSplineInstanceSize);
    const u32 first = static_cast<u32>(call(ctx, o18fn::Obj_GetField4, {alloc}));
    u32 best = 0;
    u32 it = first != 0 ? first + kNodeHeader : 0;
    while (it != 0) {
        bool take = true;
        if (filter != 0) {
            // r5 = the entry, as the original's call sequence leaves it.
            take = static_cast<u32>(call_ptr(ctx, filter, {it, fn_arg, load<u32>(filter)})) != 0;
        }
        if (take) {
            Stack frame(ctx, 32);  // sp+112: the local position, sp+128: the distance
            const u32 local = frame.addr(), dist = local + 16;
            call(ctx, o18fn::Listener_LocalDirection, {local, pos, it + kSiWorld});
            const f64 t = callf(ctx, o18fn::Spline_Nearest, {load<u32>(it + kSiClass) + kSdData, local, 0, dist}, {});
            const u32 d = load<u32>(dist);
            if (std::bit_cast<f32>(d) < std::bit_cast<f32>(best_dist)) {  // fcmpu lt
                best = it;
                best_dist = d;
                best_t = std::bit_cast<u32>(static_cast<f32>(t));
            }
        }
        const u32 next = load<u32>(it - kNodeHeader);
        it = next != 0 ? next + kNodeHeader : 0;
    }
    if (t_out != 0) store<u32>(t_out, best_t);
    if (dist_out != 0) store<u32>(dist_out, best_dist);
    ret(ctx, best);
}

// ---------------------------------------------------------------- the instance walks Bank_Start uses
// void Bank_RefreshLights()
// Rebuilds the light grid, then refreshes the light cache of every
// geometry and every emitter instance.
DECOMP(0058FADC, Bank_RefreshLights) {
    call(ctx, o18fn::LightGrid_Rebuild);
    ForEachInstance(ctx, kGeomInsts, [&](u32 o) { call(ctx, o18fn::WorldObj_UpdateLight, {o}); });
    ForEachInstance(ctx, kEmitterInsts, [&](u32 o) { call(ctx, o18fn::WorldObj_UpdateLight, {o}); });
    ret(ctx, 0u);  // r3: the last (null) next link, as the original leaves it
}

// void Bank_CollectLodGroups()
// Obj_CollectLodGroup(o, 0) for every emitter instance, then every
// geometry instance.
DECOMP(00591178, Bank_CollectLodGroups) {
    ForEachInstance(ctx, kEmitterInsts, [&](u32 o) { call(ctx, o18fn::Obj_CollectLodGroup, {o, 0}); });
    ForEachInstance(ctx, kGeomInsts, [&](u32 o) { call(ctx, o18fn::Obj_CollectLodGroup, {o, 0}); });
    ret(ctx, 0u);  // r3: the last (null) next link, as the original leaves it
}

// ---------------------------------------------------------------- ShadowMap_Init
namespace {
constexpr u32 kSmConfigSize = 0x010C5A20 + 504;  // g::config u32: the shadow setting (0 = 256) (name?)
constexpr u32 kSmSetting = 0x00F88940;          // u32: where it goes (name?)
constexpr u32 kSmLight = 0x00F66FD0;            // the current light record (SetLight_00527668; shader4_draw kFurLight)
constexpr u32 kSmWork = 0x0118E080;             // the work record: +4 the shadow source, +8 the light, +12 u16 its view count, +32 u8 (name?)
constexpr u32 kSmFov = 0x005868CC;              // f32 50.0
constexpr u32 kSmSourceOpd = 0x00F3B438;        // stored at the shadow source's +660 (name?)
constexpr u32 kVtblShadowLight = 0x00B24228;    // the 256-byte light object's vtable (name?)
constexpr u32 kSmViewSize = 1664, kSmLightSize = 256, kSmAlign = 128;
constexpr u32 kSmQuarter = 0x3E800000, kSmOne = 0x3F800000, kSmNear = 0xC2480000, kSmFar = 0x43160000, kSmFive = 0x40A00000;
constexpr u32 DepthTarget_Create = 0x0051E428;  // (u16 w, u16 h, format) -> Texture*
constexpr u32 ShadowSource_Create = 0x0052317C; // (depth target) -> object (name?)
constexpr u32 Mem_NewAligned = 0x00731648;      // (size, align)
constexpr u32 ShadowView_Construct = 0x0052D08C; // Ctor_0052D08C: (this) (name?)
constexpr u32 ShadowLight_Construct = 0x0052DACC; // Thunk_0052DACC: (this) (name?)
constexpr u32 SetList_0052D018 = 0x0052D018;    // (this, const u32* src, u8 n)
constexpr u32 SetLight_00527668 = 0x00527668;   // (light): *0x00F66FD0 = light, then its vfunc +12
constexpr u32 kVfnSetFov = 24;                  // (light; f1) (name?)

// A view of the 1024 x 2048 depth target: the split range [near, far) of
// the view distance, its -50 .. 150 depth range and the u16 rect at +28.
void SetupView(u32 v, u32 split0, u32 split1, u16 y) {
    store<u32>(v + 4, split0);
    store<u32>(v + 8, split1);
    store<u16>(v + 28, 0);
    store<u16>(v + 30, y);
    store<u16>(v + 32, 1024);
    store<u16>(v + 34, 1024);
}
}  // namespace

// void ShadowMap_Init() (name?)
// Stores the config's shadow setting (0 means 256) and, unless a light
// record exists already, builds the two-cascade shadow light: a 1024 x
// 2048 depth target, its shadow source, two 1664-byte views (the top and
// bottom halves, splits 0 .. 0.25 and 0.25 .. 1) and the 256-byte light
// that owns them, made current. The work record then gets the light, its
// source and its view count. As in the original, a failed allocation
// leaves the later stores going through a null pointer.
DECOMP(005868D0, ShadowMap_Init) {
    const u32 cfg = load<u32>(kSmConfigSize);
    store<u32>(kSmSetting, cfg != 0 ? cfg : 256u);
    u32 light = load<u32>(kSmLight);
    if (light == 0) {
        const u32 dt = static_cast<u32>(call(ctx, DepthTarget_Create, {1024, 2048, 4}));
        const u32 src = static_cast<u32>(call(ctx, ShadowSource_Create, {dt}));  // r3 = the depth target
        const u8 b52 = load<u8>(src + 52);
        store<u32>(src + 256, 0);
        store<u32>(src + 660, kSmSourceOpd);
        store<u8>(src + 52, static_cast<u8>(b52 | 0x10));
        const u32 v0 = static_cast<u32>(call(ctx, Mem_NewAligned, {kSmViewSize, kSmAlign}));
        if (v0 != 0) call(ctx, ShadowView_Construct, {v0});
        SetupView(v0, 0, kSmQuarter, 0);
        const u32 v1 = static_cast<u32>(call(ctx, Mem_NewAligned, {kSmViewSize, kSmAlign}));
        if (v1 != 0) call(ctx, ShadowView_Construct, {v1});
        SetupView(v1, kSmQuarter, kSmOne, 1024);
        store<u32>(v0 + 20, kSmNear);
        store<u32>(v0 + 24, kSmFar);
        store<u32>(v1 + 20, kSmNear);
        store<u32>(v1 + 24, kSmFar);
        store<u32>(v0 + 1536, kSmOne);
        store<u32>(v1 + 1536, kSmOne);
        Stack frame(ctx, 16);  // sp+112: {view 0, view 1}
        const u32 views = frame.addr();
        store<u32>(views, v0);
        store<u32>(views + 4, v1);
        const u32 l = static_cast<u32>(call(ctx, Mem_NewAligned, {kSmLightSize, kSmAlign}));
        if (l != 0) {
            call(ctx, ShadowLight_Construct, {l});
            store<u32>(l, kVtblShadowLight);
        }
        const u32 opd = load<u32>(load<u32>(l) + kVfnSetFov);
        ctx.fpr[1] = static_cast<f64>(load<f32>(kSmFov));
        call_ptr(ctx, opd, {l, opd, load<u32>(opd)});  // r4 / r5 = the OPD and entry
        store<u32>(l + 28, src);
        store<u32>(l + 88, kSmFive);
        call(ctx, SetList_0052D018, {l, views, 2});
        call(ctx, SetLight_00527668, {l});
        light = load<u32>(kSmLight);
    }
    u32 r3 = 0;
    if (light != 0) {
        store<u16>(kSmWork + 12, load<u8>(light + 20));
        r3 = load<u32>(light + 28);
    } else {
        store<u16>(kSmWork + 12, 0);
    }
    store<u32>(kSmWork + 8, light);
    store<u32>(kSmWork + 4, r3);
    store<u8>(kSmWork + 32, 0);
    ret(ctx, r3);  // r3 as the original leaves it
}

// ---------------------------------------------------------------- Locators_Read
namespace {
constexpr u32 Iff_ReadU8 = 0x005643C4;      // (IffReader*, u8* out, name) -> bytes
constexpr u32 Iff_ReadU16 = 0x00564340;     // (IffReader*, u16* out, name) -> bytes
constexpr u32 Mem_Alloc = 0x0054ED38;       // (size, pool, align)
constexpr u32 Mem_FreeFrom = 0x0054EE6C;    // (p, 0)
constexpr u32 kStrNumLocators = 0x00B24CC1; // "numLocators"
constexpr u32 kLocIdentity = 0x01164BD0;    // f32[4][4] (obj10.h kIdentity)
constexpr u32 kOwnerLocators = 108;         // the owner's shared locator block
// The shared block: +0 u16 references, +4 the locators (allocated 16
// bytes in, the count at -4), +8 u16 (name?).
constexpr u32 kShRefs = 0, kShData = 4, kSh8 = 8;
constexpr u32 kLocSize = 80;                // f32[4][3] the matrix rows (from the identity), +64 u32 0, +68 u16 0xFFFF the bone (name?)

// Drops a reference to a shared block; the last frees the data (16
// bytes before it) and the block.
void SharedRelease(PPUContext& ctx, u32 blk) {
    const u16 refs = static_cast<u16>(load<u16>(blk + kShRefs) - 1);
    store<u16>(blk + kShRefs, refs);
    if (static_cast<s16>(refs) > 0) return;
    const u32 data = load<u32>(blk + kShData);
    if (data != 0) {
        call(ctx, Mem_FreeFrom, {data - 16, 0});
        store<u32>(blk + kShData, 0);
    }
    call(ctx, Mem_FreeFrom, {blk, 0});
}
}  // namespace

// bool Locators_Read(IffReader* r, Owner* o) (name?)
// The "numLocators" chunk: a u8 (skipped), the u16 count, then a fresh
// shared block of that many 80-byte locators, each the identity rows with
// no bone, swapped into o +108 (the old block released). Returns 0 when
// the locator array can't be allocated (the block is then kept
// unreleased with no data, as in the original), else 1.
DECOMP(005A9478, Locators_Read) {
    const u32 r = arg<u32>(ctx, 0), o = arg<u32>(ctx, 1);
    Stack frame(ctx, 16);  // sp+112: u16 the count, sp+114: the skipped u8
    const u32 cnt = frame.addr();
    store<u16>(cnt, 0);
    call(ctx, Iff_ReadU8, {r, cnt + 2, 0});
    call(ctx, Iff_ReadU16, {r, cnt, kStrNumLocators});
    const u32 blk = static_cast<u32>(call(ctx, Mem_Alloc, {12, 0, 16}));
    if (blk != 0) {
        store<u16>(blk + kShRefs, 1);
        store<u32>(blk + kShData, 0);
        store<u16>(blk + kSh8, 0);
    }
    const u32 n = load<u16>(cnt);
    u32 data = static_cast<u32>(call(ctx, Mem_Alloc, {n * 64 + n * 16 + 16, 0, 16}));
    if (data != 0) {
        store<u32>(data + 12, n);
        data += 16;
        for (u32 i = 0; i < n; ++i) {
            const u32 l = data + i * kLocSize;
            for (u32 off : {0u, 4u, 8u, 16u, 20u, 24u, 32u, 36u, 40u, 48u}) store<u32>(l + off, load<u32>(kLocIdentity + off));
            store<u32>(l + 64, 0);
            store<u32>(l + 52, load<u32>(kLocIdentity + 52));
            store<u16>(l + 68, 0xFFFF);
            store<u32>(l + 56, load<u32>(kLocIdentity + 56));
        }
    }
    store<u32>(blk + kShData, data);
    store<u16>(blk + kSh8, 0);
    if (data == 0) {
        ret(ctx, 0u);
        return;
    }
    const u32 old = load<u32>(o + kOwnerLocators);
    if (old != blk) {
        store<u32>(o + kOwnerLocators, blk);
        if (blk != 0) store<u16>(blk + kShRefs, static_cast<u16>(load<u16>(blk + kShRefs) + 1));
        if (old != 0) SharedRelease(ctx, old);
    }
    // The reader's own reference (blk null: only the decrement, at 0).
    const u16 refs = static_cast<u16>(load<u16>(blk + kShRefs) - 1);
    store<u16>(blk + kShRefs, refs);
    if (static_cast<s16>(refs) > 0 || blk == 0) {
        ret(ctx, 1u);
        return;
    }
    const u32 d = load<u32>(blk + kShData);
    if (d != 0) {
        call(ctx, Mem_FreeFrom, {d - 16, 0});
        store<u32>(blk + kShData, 0);
    }
    call(ctx, Mem_FreeFrom, {blk, 0});
    ret(ctx, 1u);
}

// ---------------------------------------------------------------- JobChain_Init
namespace {
constexpr u32 Mem_AllocPool = 0x0056C514;  // (align, size, pool, tag)
namespace spurs {
constexpr u32 cellSpursCreateJobChainWithAttribute = 0x00ABDFA4;
constexpr u32 _cellSpursJobChainAttributeInitialize = 0x00ABDFC4;  // cellSpurs_3548F483
constexpr u32 cellSpursJobGuardInitialize = 0x00ABE084;
constexpr u32 cellSpursJobChainAttributeSetName = 0x00ABE124;
constexpr u32 _cellSpursEventFlagInitialize = 0x00ABE044;
constexpr u32 cellSpursEventFlagAttachLv2EventQueue = 0x00ABE0C4;
}  // namespace spurs
constexpr u32 kStrApeSpurs = 0x00AF4298;     // "ApeSpurs"
constexpr u32 kJcPool = 6;
constexpr u32 kJcChainSize = 272, kJcListSize = 16384, kJcBufSize = 0x40000;
constexpr u32 kJcAttrSize = 512;             // CellSpursJobChainAttribute (the original's sp+160)
// The JobChain record: +128 the job descriptor (48 bytes: +128 u64 the
// binary, +136 u16 its size in 16-byte units, +172 u8, +176 u64 the event
// flag, +184 u64 1), +256 the CellSpursEventFlag, +384 the job guard,
// +512 / +516 two 128-aligned buffers, +520 the CellSpursJobChain, +524
// its 16 KiB command list, +528 u32 (name?).
constexpr u32 kJcJob = 128, kJcEventFlag = 256, kJcGuard = 384;
constexpr u32 kJcBufA = 512, kJcBufB = 516, kJcChain = 520, kJcList = 524, kJc528 = 528;
constexpr u32 kJcBinary = 0x00D72780, kJcBinarySize = 5312;  // the SPU job (name?)
// Job chain command words (CELL_SPURS_JOB_COMMAND_*).
constexpr u64 kCmdLwsync = 127, kCmdSync = 2, kCmdNextTag = 3, kCmdGuardTag = 15;
constexpr u32 kJcListWords = kJcListSize / 8;  // 2048: 2044 lwsyncs, sync, job, guard, next (back to the start)
}  // namespace

// void JobChain_Init(JobChain* jc, CellSpurs* spurs, u32 nSpus, const u8 prio[8])
// Allocates the job chain and its command list (pool 6), creates the
// chain ("ApeSpurs"), the event flag (attached to its LV2 queue) and the
// job guard, fills the job descriptor and the command list (lwsyncs, a
// sync, the job, the guard, then a jump back to the start), and
// allocates the two buffers.
DECOMP(0056D3B0, JobChain_Init) {
    const u32 jc = arg<u32>(ctx, 0), sp = arg<u32>(ctx, 1);
    const u32 nspus = arg<u32>(ctx, 2), prio = arg<u32>(ctx, 3);
    store<u32>(jc + kJcChain, static_cast<u32>(call(ctx, Mem_AllocPool, {128, kJcChainSize, kJcPool, 0})));
    const u32 list = static_cast<u32>(call(ctx, Mem_AllocPool, {16, kJcListSize, kJcPool, 0}));
    store<u32>(jc + kJcList, list);
    {
        Stack frame(ctx, kJcAttrSize);
        const u32 attr = frame.addr();
        // (jm revision, sdk revision, attr, chain entry, job descriptor size,
        // max grabbed jobs, priorities, max contention; stack: auto request
        // count, tag1, tag2, fixed allocation, max descriptor size, initial
        // request count)
        call(ctx, spurs::_cellSpursJobChainAttributeInitialize,
             {3, 0x330000, attr, list, 128, 1, prio, nspus, /* stack: */ 1, 0, 1, 0, 256, 0});
        call(ctx, spurs::cellSpursJobChainAttributeSetName, {attr, kStrApeSpurs});
        call(ctx, spurs::cellSpursCreateJobChainWithAttribute, {sp, load<u32>(jc + kJcChain), attr});
    }
    const u32 flag = jc + kJcEventFlag;
    call(ctx, spurs::_cellSpursEventFlagInitialize, {sp, 0, flag, 0, 1});
    call(ctx, spurs::cellSpursEventFlagAttachLv2EventQueue, {flag});
    const u32 guard = jc + kJcGuard;
    call(ctx, spurs::cellSpursJobGuardInitialize, {load<u32>(jc + kJcChain), guard, 1, 1, 1});
    const u32 job = jc + kJcJob;
    for (u32 k = 0; k < 48; k += 16) obj8::VStore(job + k, obj8::VZero());  // stvx: 16-byte aligned
    store<u16>(jc + 136, static_cast<u16>((kJcBinarySize + 15) >> 4));
    store<u8>(jc + 172, 0);
    store<u64>(jc + 128, kJcBinary);
    store<u64>(jc + 176, flag);
    store<u64>(jc + 184, 1);
    store<u32>(jc + kJc528, 0);
    for (u32 i = 0; i < kJcListWords - 4; ++i) store<u64>(load<u32>(jc + kJcList) + i * 8, kCmdLwsync);
    const u32 tail = kJcListSize - 32;
    store<u64>(load<u32>(jc + kJcList) + tail, kCmdSync);
    store<u64>(load<u32>(jc + kJcList) + tail + 8, static_cast<u64>(job) & ~7ull);
    store<u64>(load<u32>(jc + kJcList) + tail + 16, (static_cast<u64>(guard) & ~0x7Full) | kCmdGuardTag);
    const u32 l = load<u32>(jc + kJcList);
    store<u64>(l + tail + 24, (static_cast<u64>(l) & ~7ull) | kCmdNextTag);
    store<u32>(jc + kJcBufA, static_cast<u32>(call(ctx, Mem_AllocPool, {128, kJcBufSize, kJcPool, 0})));
    const u32 b = static_cast<u32>(call(ctx, Mem_AllocPool, {128, kJcListSize, kJcPool, 0}));
    store<u32>(jc + kJcBufB, b);
    ret(ctx, b);
}

// ---------------------------------------------------------------- SplineInstances_CollectNear
namespace {
constexpr u32 PtrVector_Reserve = 0x00551718;  // (vector, capacity)

// The inlined PtrVector push_back (obj14.h kPv*): doubled when full, the
// count and the high-water mark updated.
void PtrVectorPush(PPUContext& ctx, u32 v, u32 item) {
    u32 n = load<u16>(v + obj14::kPvCount);
    const u32 cap = load<u16>(v + obj14::kPvCap);
    if (n == cap) {
        call(ctx, PtrVector_Reserve, {v, (cap << 1) & 0x1FFFEu});  // rlwinm 1,15,30
        n = load<u16>(v + obj14::kPvCount);
    }
    store<u32>(load<u32>(v + obj14::kPvItems) + ((n << 2) & 0x3FFFCu), item);
    const u32 c = load<u16>(v + obj14::kPvCount) + 1u;
    const u32 peak = load<u16>(v + obj14::kPvPeak);
    store<u16>(v + obj14::kPvCount, static_cast<u16>(c));
    store<u16>(v + obj14::kPvPeak, static_cast<u16>(static_cast<s32>(peak) > static_cast<s32>(c & 0xFFFF) ? peak : c));
}
}  // namespace

// u32 SplineInstances_CollectNear(PtrVector* out, const f32 pos[3], -,
//     Opd* filter(inst, arg) -> bool, void* arg; f32 max_dist) (name?)
// Appends to out every spline instance the filter accepts whose spline
// passes nearer than max_dist to pos (in the instance's frame), as
// SplineInstance_FindNearest walks them; returns how many were added.
DECOMP(005A90F8, SplineInstances_CollectNear) {
    const u32 out = arg<u32>(ctx, 0), pos = arg<u32>(ctx, 1);
    const u32 filter = arg<u32>(ctx, 3), fn_arg = arg<u32>(ctx, 4);
    const f64 max_dist = opentit::decomp::farg(ctx, 0);
    u32 added = 0;
    ForEachInstance(ctx, {obj8::kSplineInstList, obj8::kSplineInstListVtbl, obj8::kStrSplineInstList,
                          obj8::kSplineInstanceSize},
                    [&](u32 it) {
                        // r5 = the entry, as the original's call sequence leaves it.
                        if (filter != 0 && static_cast<u32>(call_ptr(ctx, filter, {it, fn_arg, load<u32>(filter)})) == 0)
                            return;
                        Stack frame(ctx, 32);  // sp+112: the local position, sp+128: the distance
                        const u32 local = frame.addr(), dist = local + 16;
                        call(ctx, o18fn::Listener_LocalDirection, {local, pos, it + kSiWorld});
                        call(ctx, o18fn::Spline_Nearest, {load<u32>(it + kSiClass) + kSdData, local, 0, dist});
                        if (!(static_cast<f64>(load<f32>(dist)) < max_dist)) return;  // fcmpu lt
                        PtrVectorPush(ctx, out, it);
                        ++added;
                    });
    ret(ctx, added);
}

// ---------------------------------------------------------------- CameraInstance_ReadHeader
namespace {
constexpr u32 Iff_ReadU32 = 0x005642AC;          // (IffReader*, u32* out, name)
constexpr u32 FileReader_Read32 = 0x00569C10;    // (r, buf, name): the 32-byte name
constexpr u32 FileReader_ReadByte = 0x0056998C;  // (r, u8* out, name)
constexpr u32 FileReader_ReadVec3Swapped = 0x005699F0;  // (r, f32 out[3], name)
constexpr u32 WorldObj_ApplyHeader = 0x00580B18; // (obj, const InstanceHeader*)
constexpr u32 kMemset = 0x00726E54;
constexpr u32 kStrHdr = 0x00B20000;              // the header field names: kStrHdr + offset
// The 116-byte instance header (obj15_small.cpp kIhSize): +0 xBasis, +12
// zBasis, +24 position, +36 scale, +48 flags, +52 nameHash, +56 animHash,
// +60 name[32], +92 defHash, +96 parentHash, +100 parentPartHash, +104
// pvsHash, +108 lightGroup, +112 u8 renderPass, +113 sortOffset, +114
// fogGroup; read into sp+112 with +116 a skipped u8 and +117 the version.
constexpr u32 kIhSize = 116;
constexpr u32 kVfnSetClass = 44;

// Reads the instance header into a stack copy and applies it to o.
void ReadInstHeader(PPUContext& ctx, u32 r, u32 o) {
    Stack frame(ctx, 128);
    const u32 h = frame.addr();
    call(ctx, kMemset, {h, 0, kIhSize});
    call(ctx, kMemset, {h, 0, kIhSize});
    call(ctx, Iff_ReadU8, {r, h + 116, 0});
    store<u8>(h + 117, 2);
    call(ctx, Iff_ReadU8, {r, h + 117, kStrHdr + 19702});           // "version"
    call(ctx, FileReader_ReadVec3Swapped, {r, h, kStrHdr + 19710});        // "xBasis"
    call(ctx, FileReader_ReadVec3Swapped, {r, h + 12, kStrHdr + 19717});   // "zBasis"
    call(ctx, FileReader_ReadVec3Swapped, {r, h + 24, kStrHdr + 19724});   // "position"
    call(ctx, FileReader_ReadVec3Swapped, {r, h + 36, kStrHdr + 19733});   // "scale"
    call(ctx, Iff_ReadU32, {r, h + 48, kStrHdr + 19739});                  // "flags"
    call(ctx, Iff_ReadU32, {r, h + 52, kStrHdr + 19745});                  // "nameHash"
    call(ctx, Iff_ReadU32, {r, h + 56, kStrHdr + 19754});                  // "animHash"
    call(ctx, FileReader_Read32, {r, h + 60, kStrHdr + 19763});            // "name"
    call(ctx, Iff_ReadU32, {r, h + 92, kStrHdr + 19768});                  // "defHash"
    call(ctx, Iff_ReadU32, {r, h + 96, kStrHdr + 19776});                  // "parentHash"
    call(ctx, Iff_ReadU32, {r, h + 100, kStrHdr + 19787});                 // "parentPartHash"
    call(ctx, Iff_ReadU32, {r, h + 104, kStrHdr + 19802});                 // "pvsHash"
    call(ctx, Iff_ReadU32, {r, h + 108, kStrHdr + 19810});                 // "lightGroup"
    call(ctx, FileReader_ReadByte, {r, h + 113, kStrHdr + 19821});         // "sortOffset"
    if (load<u8>(h + 117) >= 1) call(ctx, Iff_ReadU8, {r, h + 114, kStrHdr + 19832});  // "fogGroup"
    if (load<u8>(h + 117) >= 2) call(ctx, Iff_ReadU8, {r, h + 112, kStrHdr + 19841});  // "renderPass"
    call(ctx, WorldObj_ApplyHeader, {o, h});
}
}  // namespace

// bool CameraInstance_ReadHeader(IffReader* r, Obj* o) (name?)
// The instance header (version 2 by default: the fog group from 1, the
// render pass from 2), applied with WorldObj_ApplyHeader; an object left
// without a class gets SetClass(0). Returns 1.
DECOMP(005A63D4, CameraInstance_ReadHeader) {
    const u32 r = arg<u32>(ctx, 0), o = arg<u32>(ctx, 1);
    ReadInstHeader(ctx, r, o);
    if (load<u32>(o + 208) == 0) {
        const u32 opd = load<u32>(load<u32>(o) + kVfnSetClass);
        call_ptr(ctx, opd, {o, 0, opd, load<u32>(opd)});  // r5 / r6 = the OPD and entry
    }
    ret(ctx, 1u);
}

// ---------------------------------------------------------------- SplineDef_ReadPoints
namespace {
constexpr u32 Iff_ChunkSize = 0x005637D4;     // (IffReader*) -> the top chunk's size
constexpr u32 SplineArray_Init = 0x0055F2BC;  // (SplineArray*, u32 count, bool closed, bool flag30)
constexpr u32 kStrPosition = 0x00B24CA6, kStrControl1 = 0x00B24CAF, kStrControl2 = 0x00B24CB8;
constexpr u32 kSpRecSize = 36;                // a point in the file: position, control1, control2 (f32[3] each)
}  // namespace

// bool SplineDef_ReadPoints(IffReader* r, ApeSplineDef** pd) (name?)
// The points chunk (36 bytes a point): the definition's SplineArray (obj9.h
// kSa*) is re-made when its allocated count (minus the closing point of a
// closed spline) differs, keeping min(points, allocated) points (as in the
// original, it never grows); then each point's position and handles are
// read, and a closed spline repeats point 0 after the last. Returns 1.
DECOMP(0059FBD0, SplineDef_ReadPoints) {
    const u32 r = arg<u32>(ctx, 0), pd = arg<u32>(ctx, 1);
    Stack frame(ctx, 48);  // sp+112: the three vec3s, zeroed
    const u32 buf = frame.addr();
    for (u32 k = 0; k < 36; k += 4) store<u32>(buf + k, 0);
    const u32 size = static_cast<u32>(call(ctx, Iff_ChunkSize, {r}));
    const u32 arr = load<u32>(pd) + obj8::kSdData;
    u32 n = size / kSpRecSize;  // mulhwu 0x38E38E39 >> 3
    const u32 bits = load<u32>(arr + obj9::kSaBits);
    const bool closed = (bits & obj9::kSaClosed) != 0;
    const u32 alloc = bits & obj9::kSaAllocMask;
    if ((!closed && alloc != n) || (closed && alloc - 1 != n)) {  // cmpw
        if (!(n < alloc)) n = alloc;
        call(ctx, SplineArray_Init, {arr, closed ? n + 1 : n, closed ? 1u : 0u, (bits & obj9::kSaFlag30) != 0 ? 1u : 0u});
    }
    for (u32 i = 0; i < n; ++i) {
        call(ctx, FileReader_ReadVec3Swapped, {r, buf, kStrPosition});
        call(ctx, FileReader_ReadVec3Swapped, {r, buf + 12, kStrControl1});
        call(ctx, FileReader_ReadVec3Swapped, {r, buf + 24, kStrControl2});
        const u32 p = load<u32>(arr + obj9::kSaPoints) + i * obj9::kSpSize;
        for (u32 row = 0; row < 3; ++row)
            for (u32 k = 0; k < 12; k += 4) store<u32>(p + row * 16 + k, load<u32>(buf + row * 12 + k));
    }
    if ((load<u32>(arr + obj9::kSaBits) & obj9::kSaClosed) != 0) {
        const u32 pts = load<u32>(arr + obj9::kSaPoints);
        const u32 last = pts + n * obj9::kSpSize;
        for (u32 off : {0u, 4u, 8u, 16u, 20u, 24u, 32u, 36u, 40u}) store<u32>(last + off, load<u32>(pts + off));
    }
    ret(ctx, 1u);
}

// ---------------------------------------------------------------- Resources_InitGeomInsts
namespace {
constexpr u32 Mem_DefaultAllocator = 0x00550EE8;
constexpr u32 Array_Init = 0x00558F38;
constexpr u32 Iff_RegisterChunk = 0x0055902C;  // (fourcc, opd, arg, name, flags)
constexpr u32 kGiConfigCap = 0x010C5A20 + 360; // u16 g::config: the geometry instance capacity (0 = 1024) (name?)
constexpr u32 kGiDefaultCap = 1024;
constexpr u32 kGiCompare = 0x00F3CBB8;         // obj2.h kMapCompareB
constexpr u32 kGiChunkS = 0x534E4947, kGiChunkSOpd = 0x00F3C878;  // 'SNIG'
constexpr u32 kGiChunkH = 0x484E4947, kGiChunkHOpd = 0x00F3C880;  // 'HNIG'
constexpr u32 kSkWatch = 0x01194590;           // obj4.h kSkeletonWatch: {ApeSkeleton** data, u32 count, u32 capacity}
constexpr u32 kSkWatchCap = 256;
constexpr u32 kSkChunkL = 0x4C454B53, kSkChunkLOpd = 0x00F3C530;  // 'LEKS'
constexpr u32 kSkChunkD = 0x44484B53, kSkChunkDOpd = 0x00F3C538;  // 'DHKS'
constexpr u32 kSkChunkB = 0x534E4F42, kSkChunkBOpd = 0x00F3C540;  // 'SNOB'

// The definition / instance holders Resources_Init makes (obj17.cpp's
// NewDefHolder): the capacity from a config u16 (`dflt` when 0, none
// when negative), the holder {+0, +4 capacity, +8, +16 the map} stored at
// `slot`, the map's entries from the default allocator. As in the
// original a failed holder allocation still has the map stored at
// address 16. Returns the capacity.
u32 NewHolder(PPUContext& ctx, u32 cfg, u32 dflt, u32 slot, u32 cmp) {
    u32 cap = dflt;
    const u32 v = load<u16>(cfg);
    if (v != 0) cap = static_cast<s16>(v) > 0 ? v : 0;
    const u32 h = static_cast<u32>(call(ctx, Mem_Alloc, {20, 0, 16}));
    if (h != 0) {
        store<u32>(h + 0, 0);
        store<u32>(h + 8, 0);
        store<u32>(h + 4, cap);
        store<u32>(h + 16, 0);
    }
    store<u32>(slot, h);
    const u32 m = static_cast<u32>(call(ctx, Mem_Alloc, {24, 0, 16}));
    if (m != 0) {
        store<u16>(m + 0, static_cast<u16>(cap));
        store<u16>(m + 2, 0);
        store<u16>(m + 4, 0);
        store<u16>(m + 6, 16);
        store<u32>(m + 8, 0);
        const u32 alloc = static_cast<u32>(call(ctx, Mem_DefaultAllocator));
        store<u32>(m + 12, alloc);
        const u32 n = load<u16>(m);
        if (n != 0) {
            const u32 opd = load<u32>(load<u32>(alloc));  // alloc->Alloc (slot 0); r7 / r8 = entry / OPD
            const u32 size = n * ((static_cast<u32>(load<u16>(m + 6)) >> 1) & 0x7FFF);
            store<u32>(m + 8, static_cast<u32>(call_ptr(ctx, opd, {alloc, size, 0, 16, load<u32>(opd), opd})));
        }
        call(ctx, Array_Init, {m});
        store<u32>(m + 16, 0);
        store<u32>(m + 20, cmp);
    }
    store<u32>(h + 16, m);
    return cap;
}
}  // namespace

// void Resources_InitGeomInsts() (name?)
// The geometry instance holder (*0x00F8F144; capacity config +360, 1024
// when 0) with its map (compare 0x00F3CBB8) and the 'SNIG' / 'HNIG'
// chunk readers, then the skeleton watch list (0x01194590: 256 entries)
// and the skeleton 'LEKS' / 'DHKS' / 'SNOB' chunk readers.
DECOMP(00587A40, Resources_InitGeomInsts) {
    NewHolder(ctx, kGiConfigCap, kGiDefaultCap, kGeomInsts.hdr, kGiCompare);
    call(ctx, Iff_RegisterChunk, {kGiChunkS, kGiChunkSOpd, 0, 0, 0});
    call(ctx, Iff_RegisterChunk, {kGiChunkH, kGiChunkHOpd, 0, 0, 0});
    const u32 w = static_cast<u32>(call(ctx, Mem_Alloc, {12, 0, 16}));
    if (w != 0) {
        store<u32>(w + 0, static_cast<u32>(call(ctx, Mem_Alloc, {kSkWatchCap * 4, 0, 16})));
        store<u32>(w + 4, 0);
        store<u32>(w + 8, kSkWatchCap);
    }
    store<u32>(kSkWatch, w);
    call(ctx, Iff_RegisterChunk, {kSkChunkL, kSkChunkLOpd, 0, 0, 0});
    call(ctx, Iff_RegisterChunk, {kSkChunkD, kSkChunkDOpd, 0, 0, 0});
    ret(ctx, call(ctx, Iff_RegisterChunk, {kSkChunkB, kSkChunkBOpd, 0, 0, 0}));
}

// ---------------------------------------------------------------- Bank_DrawPrepare
namespace {
constexpr u32 kDpConsts = 0x0058D43C;          // f32 {0, 65536, 16777216}
constexpr u32 kDpWork = 0x0118E080 + 8;        // the work record: +64 / +68 f32 the polygon offset factor / units (name?)
constexpr u32 kGcmState = 0x010F10A0;          // +0 the CellGcmContextData*, +84 the render target (+24 its depth format), +93 u8 the depth test bits (name?)
constexpr u32 kGsTarget = 84, kGsDepth = 93;
constexpr u32 kDpWarned = 0x011DE2C4;          // u8: set for an unexpected depth format (name?)
// NV4097 methods (one-word writes).
constexpr u32 kCmdPolyOffsetFill = 0x00040A68;
constexpr u32 kCmdPolyOffset = 0x00080A78;     // scale factor, bias (two words)
constexpr u32 kCmdDepthTestEnable = 0x00040A70;
constexpr u32 kCmdDepthFunc = 0x00040A74;
constexpr u32 kCmdDepthMask = 0x00040A6C;
constexpr u32 kCmd0310 = 0x00040310, kCmd183C = 0x0004183C, kCmd1834 = 0x00041834, kCmd0324 = 0x00040324;

// The inlined cellGcm method write: {method, value} at the context's
// current pointer (no space check, as in the original).
void GcmPut(u32 method, u32 v) {
    const u32 g = load<u32>(kGcmState);
    const u32 cur = load<u32>(g + 8);
    store<u32>(cur, method);
    store<u32>(cur + 4, v);
    store<u32>(g + 8, cur + 8);
}
}  // namespace

// void Bank_DrawPrepare(Obj*)
// The depth state the bank's views draw with: the polygon offset from the
// work record (the units scaled for the target's 16- (4 / 6) or 24-bit (5
// / 7) depth format; disabled when both are 0, depth testing is off or
// there is no target; any other format sets the warning byte and
// disables it), then depth test / func from the state bits and the fixed
// depth mask, 0x0310, 0x183C, 0x1834 and 0x0324 words.
DECOMP(0058D448, Bank_DrawPrepare) {
    const u32 rec = load<u32>(kDpWork);
    const f32 zero = load<f32>(kDpConsts);
    const u32 factor = load<u32>(rec + 64);
    f32 units = load<f32>(rec + 68);
    bool off = load<f32>(rec + 64) == zero && units == zero;  // fcmpu eq
    const u32 target = load<u32>(kGcmState + kGsTarget);
    off = off || load<u8>(kGcmState + kGsDepth) == 0 || target == 0;
    if (!off) {
        const s32 fmt = load<s32>(target + 24);
        if (fmt == 4 || fmt == 6) {
            units = static_cast<f32>(static_cast<f64>(units) * static_cast<f64>(load<f32>(kDpConsts + 4)));
        } else if (fmt == 5 || fmt == 7) {
            units = static_cast<f32>(static_cast<f64>(units) * static_cast<f64>(load<f32>(kDpConsts + 8)));
        } else {
            if (load<u8>(kDpWarned) == 0) store<u8>(kDpWarned, 1);
            off = true;
        }
    }
    if (off) {
        GcmPut(kCmdPolyOffsetFill, 0);
    } else {
        GcmPut(kCmdPolyOffsetFill, 1);
        const u32 g = load<u32>(kGcmState);
        const u32 cur = load<u32>(g + 8);
        store<u32>(cur, kCmdPolyOffset);
        store<u32>(cur + 4, factor);
        store<f32>(cur + 8, units);
        store<u32>(g + 8, cur + 12);
    }
    GcmPut(kCmdDepthTestEnable, (load<u8>(kGcmState + kGsDepth) & 1) != 0 ? 1u : 0u);
    GcmPut(kCmdDepthFunc, load<u8>(kGcmState + kGsDepth));
    GcmPut(kCmdDepthMask, 515);
    GcmPut(kCmd0310, 0);
    GcmPut(kCmd183C, 1);
    GcmPut(kCmd1834, 2305);
    GcmPut(kCmd0324, 0);
    ret(ctx, kCmd0324);  // r3 as the original leaves it
}

// ---------------------------------------------------------------- the IFF statics' destructor
namespace {
// Pooled blocks {+0 data, +4 u16, +8 u16, +12 u32 bytes, +16 allocator}
// freed through allocator->Free (vfunc +8) with the size in 8-byte units.
constexpr u32 kSdBlockA = 0x011DE4C0, kSdGuardA = 0x011DE4D8;  // u64 guards: nonzero = constructed (name?)
constexpr u32 kSdBlockD = 0x01164E14;
constexpr u32 kSdBlockG = 0x01165028, kSdGuardG = 0x01165020;
constexpr u32 kSdGuardB1 = 0x01164E60, kSdGuardB2 = 0x01164F00;
// Objects whose vtable (+8) goes back to the base 0x00B24270 +8.
constexpr u32 kSdObjB3 = 0x01165000, kSdObjE = 0x01165010, kSdObjH = 0x01165040;
constexpr u32 kSdBaseVtbl = 0x00B24270 + 8;
constexpr u32 kSdChunkMap = 0x01164E30;  // obj6.h kChunkMap: {u16 count, .., u16 +6 the entry size * 2, +8 data, +12 allocator}
constexpr u32 kVfnAllocFree = 8;

void FreeBlock(PPUContext& ctx, u32 b) {
    const u32 data = load<u32>(b);
    if (data == 0) return;
    const u32 alloc = load<u32>(b + 16);
    const u32 opd = load<u32>(load<u32>(alloc) + kVfnAllocFree);
    call_ptr(ctx, opd, {alloc, data, load<u32>(b + 12) >> 3, opd, load<u32>(opd)});  // r6 / r7 = the OPD and entry
    store<u16>(b + 4, 0);
    store<u16>(b + 8, 0);
    store<u32>(b, 0);
}

void ResetVtbl(u32 o) {
    store<u64>(o, 0);
    store<u32>(o + 8, kSdBaseVtbl);
}
}  // namespace

// void IffStatics_Destroy() (name?)
// The atexit destructor of the IFF / memory statics: frees the pooled
// blocks, clears the construction guards, puts three objects back on the
// base vtable and frees the chunk map's entries (its fields are left).
DECOMP(0056B6A8, IffStatics_Destroy) {
    if (load<u64>(kSdGuardA) != 0) {
        FreeBlock(ctx, kSdBlockA);
        store<u64>(kSdGuardA, 0);
    }
    if (load<u64>(kSdGuardB1) != 0) store<u64>(kSdGuardB1, 0);
    if (load<u64>(kSdGuardB2) != 0) store<u64>(kSdGuardB2, 0);
    if (load<u64>(kSdObjB3) != 0) ResetVtbl(kSdObjB3);
    {
        const u32 m = kSdChunkMap;
        const u32 size = load<u16>(m) * ((static_cast<u32>(load<u16>(m + 6)) >> 1) & 0x7FFF);
        const u32 data = load<u32>(m + 8);
        if (data != 0) {
            const u32 alloc = load<u32>(m + 12);
            const u32 opd = load<u32>(load<u32>(alloc) + kVfnAllocFree);
            call_ptr(ctx, opd, {alloc, data, size, opd, load<u32>(opd)});
        }
    }
    FreeBlock(ctx, kSdBlockD);
    if (load<u64>(kSdObjE) != 0) ResetVtbl(kSdObjE);
    if (load<u64>(kSdGuardG) != 0) {
        FreeBlock(ctx, kSdBlockG);
        store<u64>(kSdGuardG, 0);
    }
    u32 r3 = 0;
    if (load<u64>(kSdObjH) != 0) {
        ResetVtbl(kSdObjH);
        r3 = kSdBaseVtbl;
    }
    ret(ctx, r3);  // r3 as the original leaves it
}

// ---------------------------------------------------------------- Spurs_Shutdown
namespace {
namespace sdk {
constexpr u32 cellSpursShutdownJobChain = 0x00ABE0A4;
constexpr u32 cellSpursJoinJobChain = 0x00ABE164;
constexpr u32 cellSpursFinalize = 0x00ABE204;
constexpr u32 sceNpTerm = 0x00ABE3E4;
constexpr u32 cellSysmoduleUnloadModule = 0x00ABD924;
}  // namespace sdk
constexpr u32 Mem_FreePool = 0x0056C730;       // (p, pool, tag)
constexpr u32 Libc_printf = 0x007240F4;
constexpr u32 Libc_Shutdown726314 = 0x00726314;  // () (name?)
constexpr u32 kPsJobChains = 0x0116B580;       // the job chain records (JobChain_Init layout) at +128 and +896 (name?)
constexpr u32 kPsJcA = 128, kPsJcB = 896;
constexpr u32 kPsGlobals = 0x010F0E20;         // libgplatformGlobals: +76 the CellSpurs, +80 its memory (name?)
constexpr u32 kPsNpPool = 0x00F8875C, kPsNpFlag = 0x00F88760;  // the NP memory pool, a flag (name?)
constexpr u32 kStrSpursDone = 0x00AF42F0;      // "## libgplatformGlobals.pSpurs : ApeSpurs SUCCEEDED ##\n"
constexpr u32 kStrUnloadFailed = 0x00AF4328;   // "UnloadModule failed...: 0x%x\n"
constexpr u32 kStrUnloadNpFailed = 0x00AF3B78; // "UnloadModule(%s) failed... error: 0x%x\n"
constexpr u32 kStrSysutilNp = 0x00AF3BA0;      // "CELL_SYSMODULE_SYSUTIL_NP"
constexpr u32 kModSysutilNp = 22;
constexpr u32 kPsModules[] = {23, 28, 16, 14, 49, 62};  // CELL_SYSMODULE_* unloaded after NP

// JobChain_Init's undo: the chain shut down and joined, then its chain,
// command list and two buffers back to pool 6.
void JobChainTerm(PPUContext& ctx, u32 jc) {
    call(ctx, sdk::cellSpursShutdownJobChain, {load<u32>(jc + kJcChain)});
    call(ctx, sdk::cellSpursJoinJobChain, {load<u32>(jc + kJcChain)});
    for (u32 off : {kJcChain, kJcList, kJcBufA, kJcBufB}) call(ctx, Mem_FreePool, {load<u32>(jc + off), kJcPool, 0});
}
}  // namespace

// bool Spurs_Shutdown() (name?)
// Tears down the two job chains and SPURS (its memory back to pool 6),
// terminates NP (freeing its pool) and unloads the system modules,
// printing the failures (the generic message prints the module id, not
// the error, as in the original). Returns 1.
DECOMP(0056DCEC, Spurs_Shutdown) {
    JobChainTerm(ctx, kPsJobChains + kPsJcB);
    JobChainTerm(ctx, kPsJobChains + kPsJcA);
    call(ctx, sdk::cellSpursFinalize, {load<u32>(kPsGlobals + 76)});
    call(ctx, Mem_FreePool, {load<u32>(kPsGlobals + 76), kJcPool, 0});
    call(ctx, Mem_FreePool, {load<u32>(kPsGlobals + 80), kJcPool, 0});
    call(ctx, Libc_Shutdown726314);
    call(ctx, Libc_printf, {kStrSpursDone});
    if (load<u32>(kPsNpPool) != 0) {
        call(ctx, sdk::sceNpTerm);
        const u32 pool = load<u32>(kPsNpPool);
        if (pool != 0) {
            call(ctx, Mem_FreeFrom, {pool, 0});
            store<u32>(kPsNpPool, 0);
        }
        store<u32>(kPsNpFlag, 0);
        const s32 r = static_cast<s32>(call(ctx, sdk::cellSysmoduleUnloadModule, {kModSysutilNp}));
        if (r < 0) call(ctx, Libc_printf, {kStrUnloadNpFailed, kStrSysutilNp, static_cast<u32>(r)});
    }
    for (u32 id : kPsModules)
        if (static_cast<s32>(call(ctx, sdk::cellSysmoduleUnloadModule, {id})) < 0) call(ctx, Libc_printf, {kStrUnloadFailed, id});
    ret(ctx, 1u);
}

// ---------------------------------------------------------------- TagTable_Destroy
namespace {
constexpr u32 kTtTable = 0x01164DE8;  // the tag table (tags.cpp kTagTable): SortedMap of {key, SortedMap*}
// SortedMap (resource/tags.h): +0 u16 capacity, +2 u16 count, +6 u16 the
// entry size * 2, +8 {u32 key, u32 value} entries, +12 the allocator.
constexpr u32 kSmCap = 0, kSmCount = 2, kSmFlags = 6, kSmEntries = 8, kSmAlloc = 12;
constexpr u32 kTagRefs = 0, kTagData = 28;  // a tag: u16 references, +28 its data (pool 0)

// Erases entry i (the later ones moved down one, word by word with the
// count re-read between the two words) and drops the count.
void EraseAt(u32 m, u32 i, u32 count) {
    if (static_cast<s32>(count) >= 2 && static_cast<s32>(i) < static_cast<s32>(count - 1)) {
        const u32 e = load<u32>(m + kSmEntries);
        u32 k = i;
        do {
            store<u32>(e + k * 8, load<u32>(e + k * 8 + 8));
            const u32 w = load<u32>(e + k * 8 + 12);
            const u32 c = load<u16>(m + kSmCount);
            store<u32>(e + k * 8 + 4, w);
            ++k;
            if (!(static_cast<s32>(k) < static_cast<s32>(c - 1))) break;
        } while (true);
    }
    store<u16>(m + kSmCount, static_cast<u16>(count - 1));
}
}  // namespace

// void TagTable_Destroy() (name?)
// Empties the tag table from the back: each group's tags lose a
// reference (the last frees the tag's data and the tag, both pool 0) and
// are erased, then the group's entries are freed (allocator vfunc +8) and
// the group itself, and the group is erased. As in the original, a null
// tag or group is still read / decremented.
DECOMP(00564C64, TagTable_Destroy) {
    u32 t = load<u32>(kTtTable);
    const u32 n = load<u16>(t + kSmCount);
    if (n == 0) {
        ret(ctx, kTtTable & 0xFFFF0000u);  // r3 as the original leaves it (the lis)
        return;
    }
    for (u32 i = n - 1;; --i) {
        const u32 g = load<u32>(load<u32>(t + kSmEntries) + i * 8 + 4);
        const u32 m = load<u16>(g + kSmCount);
        if (m != 0) {
            u32 c = m;
            for (u32 j = m - 1;; --j) {
                const u32 tag = load<u32>(load<u32>(g + kSmEntries) + j * 8 + 4);
                const u16 refs = static_cast<u16>(load<u16>(tag + kTagRefs) - 1);
                store<u16>(tag + kTagRefs, refs);
                if (static_cast<s16>(refs) <= 0 && tag != 0) {
                    call(ctx, Mem_FreePool, {load<u32>(tag + kTagData), 0, 0});
                    call(ctx, Mem_FreePool, {tag, 0, 0});
                    c = load<u16>(g + kSmCount);
                }
                c &= 0xFFFF;
                EraseAt(g, j, c);
                c -= 1;
                if (j == 0) break;
            }
        }
        if (g != 0) {
            const u32 size = load<u16>(g + kSmCap) * ((static_cast<u32>(load<u16>(g + kSmFlags)) >> 1) & 0x7FFF);
            const u32 data = load<u32>(g + kSmEntries);
            if (data != 0) {
                const u32 alloc = load<u32>(g + kSmAlloc);
                const u32 opd = load<u32>(load<u32>(alloc) + 8);
                call_ptr(ctx, opd, {alloc, data, size, opd, load<u32>(opd)});  // r6 / r7 = the OPD and entry
            }
            call(ctx, Mem_FreePool, {g, 0, 0});
        }
        t = load<u32>(kTtTable);
        EraseAt(t, i, load<u16>(t + kSmCount));
        if (i == 0) break;
        t = load<u32>(kTtTable);
    }
    ret(ctx, t);  // r3 as the original leaves it
}

// ---------------------------------------------------------------- Bank_BuildViewJob
namespace {
constexpr u32 Render_FrameAlloc = 0x004FD1E0;  // (u32 size)
constexpr u32 Camera_FovScale = 0x005206F8;    // (Camera*) -> f1
constexpr u32 Zones_MaskForPoint = 0x00577520; // (Zones*, const f32 p[3]): its result lands in zones +20 (name?)
constexpr u32 kVjWork = 0x0118E080;            // +0 the camera, +8 the view items (+4 Item*[]), +12 u16 their count
constexpr u32 kVjZones = 0x01193060 + 36;      // obj4.h kZones
constexpr u32 kVjSize = 464, kVjBufSize = 4096;
// The job: +0 the camera position (+864), +16 its direction (+848), +32
// f32 the fov scale, +36 the zone mask, then per item (224 bytes from +48)
// its six planes (+112) and two matrices (+208); +272 / +276 + 4k its
// buffer end / start, +280 u32 256, +284 u32 the item count (name?).
constexpr u32 kVjItemData = 48, kVjItemStride = 224, kVjEnd = 272, kVjStart = 276, kVj280 = 280, kVjCount = 284;
constexpr u32 kItPlanes = 112, kItMats = 208, kItBuf = 416, kItBufStart = 512;
}  // namespace

// Job* Bank_BuildViewJob(-, u32 per, u32 index) (name?)
// The frame-allocated job for view items [per * index, + per) (clipped to
// the work record's count): the camera position, direction, fov scale
// and zone mask, then per item a fresh 4 KiB frame buffer and a copy of
// its planes and matrices. As in the original the item index is taken
// modulo 256 and the per-item end / start words overlap the next item's
// data (the jobs are built with one item each); a zero count makes the
// u16 loop endless, as in the original.
DECOMP(0058D920, Bank_BuildViewJob) {
    const u32 per = arg<u32>(ctx, 1), index = arg<u32>(ctx, 2);
    const u32 start = (per * index) & 0xFFFF;
    const u32 end = (start + per) & 0xFFFF;
    u32 last = load<u16>(kVjWork + 12);
    if (static_cast<s32>(end) < static_cast<s32>(last)) last = end;
    last = (last - 1) & 0xFFFF;
    const u32 job = static_cast<u32>(call(ctx, Render_FrameAlloc, {kVjSize}));
    store<u32>(job + kVjCount, last - start + 1);
    store<u32>(job + kVj280, 256);
    const u32 cam = load<u32>(kVjWork);
    const u32 pos = cam + 864, dir = cam + 848;
    for (u32 k = 0; k < 12; k += 4) store<u32>(job + k, load<u32>(pos + k));
    for (u32 k = 0; k < 12; k += 4) store<u32>(job + 16 + k, load<u32>(dir + k));
    const f64 fov = callf(ctx, Camera_FovScale, {cam}, {});  // r3 = the camera, as the original leaves it
    store<f32>(job + 32, static_cast<f32>(fov));
    const u32 zones = load<u32>(kVjZones);
    u32 mask = 0xFFFFFFFFu;
    if (zones != 0) {
        call(ctx, Zones_MaskForPoint, {zones, pos});
        mask = load<u32>(zones + 20);
    }
    store<u32>(job + 36, mask);
    for (u32 i = start; static_cast<s32>(i) <= static_cast<s32>(last); i = (i + 1) & 0xFFFF) {
        const u32 item = load<u32>(load<u32>(kVjWork + 8) + 4 + ((i << 2) & 0x3FC));  // rlwinm 2,22,29
        const u32 buf = static_cast<u32>(call(ctx, Render_FrameAlloc, {kVjBufSize}));
        store<u32>(item + kItBuf, buf);
        store<u32>(item + kItBufStart, buf);
        const u32 k = i - start;
        const u32 d = job + k * kVjItemStride;
        for (u32 o = 0; o < 96; o += 4) store<u32>(d + kVjItemData + o, load<u32>(item + kItPlanes + o));
        for (u32 o = 0; o < 128; o += 4) store<u32>(d + 144 + o, load<u32>(item + kItMats + o));
        store<u32>(job + kVjEnd + 4 * k, load<u32>(item + kItBuf) + kVjBufSize);
        store<u32>(job + kVjStart + 4 * k, item + kItBufStart);
    }
    ret(ctx, job);
}

// ---------------------------------------------------------------- Bank_Destroy
namespace {
constexpr u32 WaitRender_0051265C = 0x0051265C;
constexpr u32 WaitRenderIdle_005126E4 = 0x005126E4;
constexpr u32 Bank_Stop596A8C = 0x00596A8C;      // (2) (name?)
constexpr u32 Render_Stop5124F4 = 0x005124F4;    // (2) (name?)
constexpr u32 RecList_Destroy596770 = 0x00596770; // (list) (name?)
constexpr u32 LightRecList_Destroy = 0x005965BC; // (RecList**)
constexpr u32 TagTable_Destroy = 0x00564C64;
constexpr u32 Debug_FlushPrimPools = 0x0056B57C;
constexpr u32 Bank_DestroyNopA = 0x00556764, Bank_DestroyNopB = 0x0056A70C;
constexpr u32 Registry_UpdateAll = 0x00556DB4;
constexpr u32 kBdNoQueue = 0x0116BD05;           // obj15.h kDirtyListOff2: set while the bank goes
constexpr u32 kBdRecListA = 0x01194EC4, kBdLightRecs = 0x01194ED0;
// 272-byte records (u32 count at 0x0116E100): +72 a block, +208 s32 the
// objects at +216 (released through vfunc +8) (name?).
constexpr u32 kBdRecs = 0x0116BF00, kBdRecCount = 0x0116E100, kBdRecSize = 272;
constexpr u32 kBdObjs = 36, kBdObjCount = 44;    // bank: Obj*[] / u16 (released, +8 bits 6-9 cleared)
constexpr u32 kBdSubs = 68, kBdSubCount = 76;    // bank: Obj*[] / u16 (vfunc +4, nulls skipped)

// The inlined pop_back of a {Obj** at data_off, u16 count at count_off}
// vector: the count drops, and the item at the u16-wrapped new count moves
// to the s32 one when they differ (only after a count of 0).
void PopBack(u32 v, u32 data_off, u32 count_off, u32 n) {
    const u32 nm = (n - 1) & 0xFFFF;
    store<u16>(v + count_off, static_cast<u16>(n - 1));
    if (n - 1 != nm) {
        const u32 data = load<u32>(v + data_off);
        store<u32>(data + (n - 1) * 4, load<u32>(data + nm * 4));
    }
}
}  // namespace

// void Bank_Destroy(Bank* b) (name?)
// Waits for rendering, stops the bank's jobs, destroys the record lists,
// releases the 272-byte records' objects and blocks (from the last; the
// count is left at -1, as in the original), the tag table, the bank's
// objects (+36, their +8 bits 6-9 cleared, released) and sub-objects
// (+68, vfunc +4), then the bank itself (vfunc +4).
DECOMP(00596E3C, Bank_Destroy) {
    const u32 b = arg<u32>(ctx, 0);
    call(ctx, WaitRender_0051265C);
    call(ctx, Bank_Stop596A8C, {2});
    call(ctx, Render_Stop5124F4, {2});
    call(ctx, WaitRenderIdle_005126E4);
    store<u8>(kBdNoQueue, 1);
    call(ctx, RecList_Destroy596770, {kBdRecListA});
    call(ctx, LightRecList_Destroy, {kBdLightRecs});
    s32 r = load<s32>(kBdRecCount);
    if (r != 0) {
        store<s32>(kBdRecCount, --r);
        while (r >= 0) {
            const u32 rec = kBdRecs + static_cast<u32>(r) * kBdRecSize;
            s32 k = load<s32>(rec + 208) - 1;
            store<s32>(rec + 208, k);
            while (k >= 0) {
                obj4::Vcall0(ctx, load<u32>(rec + 216 + static_cast<u32>(k) * 4), 8);
                k = load<s32>(rec + 208) - 1;
                store<s32>(rec + 208, k);
            }
            const u32 blk = load<u32>(rec + 72);
            if (blk != 0) {
                call(ctx, Mem_FreeFrom, {blk, 0});
                store<u32>(rec + 72, 0);
            }
            r = load<s32>(kBdRecCount) - 1;
            store<s32>(kBdRecCount, r);
        }
    }
    call(ctx, TagTable_Destroy);
    for (u32 n; (n = load<u16>(b + kBdObjCount)) != 0;) {
        const u32 o = load<u32>(load<u32>(b + kBdObjs) + (n - 1) * 4);
        PopBack(b, kBdObjs, kBdObjCount, n);
        store<u32>(o + 8, load<u32>(o + 8) & ~0x3C0u);  // rlwimi 0,22,25
        obj4::Vcall0(ctx, o, 8);
    }
    call(ctx, WaitRenderIdle_005126E4);
    for (u32 n; (n = load<u16>(b + kBdSubCount)) != 0;) {
        const u32 o = load<u32>(load<u32>(b + kBdSubs) + (n - 1) * 4);
        if (o != 0) {
            obj4::Vcall0(ctx, o, 4);
            n = load<u16>(b + kBdSubCount);
        }
        PopBack(b, kBdSubs, kBdSubCount, n);
    }
    if (b != 0) obj4::Vcall0(ctx, b, 4);
    call(ctx, Debug_FlushPrimPools);
    call(ctx, Bank_DestroyNopA);
    call(ctx, Registry_UpdateAll);
    store<u8>(kBdNoQueue, 0);
    ret(ctx, call(ctx, Bank_DestroyNopB));
}

// ---------------------------------------------------------------- GeomInstance_ReadHeader
namespace {
constexpr u32 GeomDef_New = 0x007A71A4;    // () -> ApeGeomDef
constexpr u32 Mesh_Default = 0x00526300;   // () -> the default sphere topology
constexpr u32 kGhDefault = 0x00F889FC;     // ApeGeomDef*: the default definition, made on first use (name?)
constexpr u32 kGhLodDist = 0x42480000;     // 50.0: its single topology's distance
}  // namespace

// bool GeomInstance_ReadHeader(IffReader* r, Obj* o) (name?)
// The instance header (CameraInstance_ReadHeader's), then an object left
// without a class gets the default geometry definition (made on first
// use: Mesh_Default as its one topology, at distance 50). Returns 1.
DECOMP(005A1A08, GeomInstance_ReadHeader) {
    const u32 r = arg<u32>(ctx, 0), o = arg<u32>(ctx, 1);
    ReadInstHeader(ctx, r, o);
    if (load<u32>(o + 208) == 0) {
        u32 d = load<u32>(kGhDefault);
        if (d == 0) {
            d = static_cast<u32>(call(ctx, GeomDef_New));
            store<u32>(kGhDefault, d);
            const u32 mesh = static_cast<u32>(call(ctx, Mesh_Default));
            const u32 i = load<u8>(d + obj2::kClassTopoCount);
            store<u8>(d + obj2::kClassTopoCount, static_cast<u8>(i + 1));
            const u32 e = d + obj2::kClassTopos + i * 8;
            store<u32>(e, mesh);
            store<u32>(e + 4, kGhLodDist);
            d = load<u32>(kGhDefault);
        }
        const u32 opd = load<u32>(load<u32>(o) + kVfnSetClass);
        call_ptr(ctx, opd, {o, d, opd, load<u32>(opd)});  // r5 / r6 = the OPD and entry
    }
    ret(ctx, 1u);
}

// ---------------------------------------------------------------- Resources_InitLightInsts
namespace {
constexpr u32 kLiConfig = 0x010C5A20;          // g::config
constexpr u32 kLiConfigCap = kLiConfig + 364;  // u16: the light instance capacity (0 = 128) (name?)
constexpr u32 kLiDefaultCap = 128;
constexpr u32 kLiCompare = 0x00F3A9B8;         // obj2.h kMapCompareC
constexpr u32 kLiChunkS = 0x534E494C, kLiChunkSOpd = 0x00F3C8C8;  // 'SNIL'
constexpr u32 kLiChunkH = 0x484E494C, kLiChunkHOpd = 0x00F3C8D0;  // 'HNIL'
constexpr u32 kLiConsts = 0x00587740;          // f32 {0, 40, 75}
constexpr u32 kLiSlots = 0x011945A0;           // classes.h kLightSlotLists: +0 the slots, +4 u16 used, +6 u16 last, +8 u16 capacity
constexpr u32 kLiSlotCount = 384, kLiSlotSize = 64;

// x unless it is 0 (then `dflt`); negatives and NaN clamp to 0.
f32 PositiveOr(f32 x, f32 zero, f32 dflt) {
    if (x == zero) return dflt;
    return x > zero ? x : zero;
}
}  // namespace

// void Resources_InitLightInsts() (name?)
// The light instance holder (*0x00F8F148; capacity config +364, 128 when
// 0) with its map (compare 0x00F3A9B8), the 'SNIL' / 'HNIL' chunk
// readers, then the config's light distances (+112 / +116 default 40 / 75;
// +120 and +124 fall back to the +112 / +116 results, as in the original,
// their squares at +128 / +132), the frame counter +96 bumped, and the
// 384 light slots.
DECOMP(00587750, Resources_InitLightInsts) {
    NewHolder(ctx, kLiConfigCap, kLiDefaultCap, kLightInsts.hdr, kLiCompare);
    call(ctx, Iff_RegisterChunk, {kLiChunkS, kLiChunkSOpd, 0, 0, 0});
    call(ctx, Iff_RegisterChunk, {kLiChunkH, kLiChunkHOpd, 0, 0, 0});
    const f32 zero = load<f32>(kLiConsts);
    const f32 a = PositiveOr(load<f32>(kLiConfig + 112), zero, load<f32>(kLiConsts + 4));
    store<f32>(kLiConfig + 112, a);
    const f32 b = PositiveOr(load<f32>(kLiConfig + 116), zero, load<f32>(kLiConsts + 8));
    store<f32>(kLiConfig + 116, b);
    const f32 c = PositiveOr(load<f32>(kLiConfig + 120), zero, a);
    store<f32>(kLiConfig + 120, c);
    const f32 d0 = load<f32>(kLiConfig + 124);
    store<f32>(kLiConfig + 128, static_cast<f32>(static_cast<f64>(c) * c));
    const f32 d = PositiveOr(d0, zero, b);
    const u32 frame = load<u32>(kLiConfig + 96);
    store<f32>(kLiConfig + 124, d);
    store<u32>(kLiConfig + 96, frame + 1);
    store<f32>(kLiConfig + 132, static_cast<f32>(static_cast<f64>(d) * d));
    const u32 slots = static_cast<u32>(call(ctx, Mem_Alloc, {kLiSlotCount * kLiSlotSize, 0, 16}));
    store<u32>(kLiSlots, slots);
    store<u16>(kLiSlots + 8, kLiSlotCount);
    store<u16>(kLiSlots + 4, 0);
    store<u16>(kLiSlots + 6, kLiSlotCount - 1);
    ret(ctx, slots);
}

// ---------------------------------------------------------------- SplineDef_Read
namespace {
constexpr u32 String_CopyN = 0x00553638;       // (dst, src, n)
constexpr u32 String_HashNoCase = 0x0056A710;
constexpr u32 SplineDef_Construct = 0x0059ECF8;
constexpr u32 SplineDefMap_Find = 0x007936FC;  // (map, const u32* key) -> slot
constexpr u32 kStrSdName = 0x00B24C91, kStrSdFlags = 0x00B24C96, kStrSdNumPoints = 0x00B24C9C;
constexpr u32 kIrFlags = 12;                   // IffReader u8: 0x04 = reuse an existing definition (name?)
// The definition list header (obj8.h kSplineDefList): +8 the OPD called
// (def, +12, 0 / 1) around a definition's set-up, +12 its argument (name?).
constexpr u32 kHdrCallback = 8, kHdrCallbackArg = 12;
}  // namespace

// bool SplineDef_Read(IffReader* r, ApeSplineDef** out) (name?)
// The spline definition chunk: name, flags, point count; *out = the
// definition of that name, or (unless the reader reuses an existing one)
// a new one from the ApeSplineDef free list: constructed, named and
// hashed, its point array made for the count (flags bit 0 closed, bit 1
// the second flag), the header's callback told before (0) and after (1).
// Returns 1.
DECOMP(0059F8E4, SplineDef_Read) {
    const u32 r = arg<u32>(ctx, 0), out = arg<u32>(ctx, 1);
    Stack frame(ctx, 48);  // sp+112: the hash key, +116 the name (36 bytes), +148 / +150 u16 flags / count, +152 u8
    const u32 key = frame.addr(), name = key + 4;
    call(ctx, kMemset, {name, 0, 36});
    call(ctx, Iff_ReadU8, {r, key + 40, 0});
    call(ctx, FileReader_Read32, {r, name, kStrSdName});
    call(ctx, Iff_ReadU16, {r, key + 36, kStrSdFlags});
    call(ctx, Iff_ReadU16, {r, key + 38, kStrSdNumPoints});
    store<u32>(key, static_cast<u32>(call(ctx, String_HashNoCase, {name})));
    const u32 slot = static_cast<u32>(call(ctx, SplineDefMap_Find, {load<u32>(load<u32>(obj8::kSplineDefList) + 16), key}));
    const u32 found = load<u32>(slot);
    store<u32>(out, found);
    if ((load<u8>(r + kIrFlags) & 0x04) != 0 && found != 0) {
        ret(ctx, 1u);
        return;
    }
    const u32 alloc = obj6::ListAllocator(ctx, obj8::kSplineDefList, obj8::kSplineDefListVtbl, obj8::kStrSplineDefList,
                                          obj8::kSplineDefSize);
    const u32 nd = static_cast<u32>(obj4::Vcall0(ctx, alloc, obj8::kVfnListAlloc));
    u32 d = 0;
    if (nd != 0) {
        call(ctx, SplineDef_Construct, {nd});
        d = nd;
    }
    store<u32>(out, d);
    u32 hdr = load<u32>(obj8::kSplineDefList);
    u32 cb = load<u32>(hdr + kHdrCallback);
    u32 def = d;
    if (cb != 0) {
        call_ptr(ctx, cb, {d, load<u32>(hdr + kHdrCallbackArg), 0, cb, load<u32>(cb)});  // r6 / r7 = the OPD and entry
        def = load<u32>(out);
    }
    const u32 dn = def + obj8::kSdName;
    call(ctx, String_CopyN, {dn, name, 31});
    store<u32>(def + obj11::kEdNameHash, static_cast<u32>(call(ctx, String_HashNoCase, {dn})));
    const u32 flags = load<u16>(key + 36);
    call(ctx, SplineArray_Init, {def + obj8::kSdData, load<u16>(key + 38), flags & 1, (flags & 2) != 0 ? 1u : 0u});
    const u32 o = load<u32>(out);
    hdr = load<u32>(obj8::kSplineDefList);
    cb = load<u32>(hdr + kHdrCallback);
    if (cb != 0) call_ptr(ctx, cb, {o, load<u32>(hdr + kHdrCallbackArg), 1, load<u32>(cb)});  // r6 = the entry
    ret(ctx, 1u);
}

// ---------------------------------------------------------------- Obj_SubmitTintedDraws
namespace {
constexpr u32 Material_FindByHash = 0x004FF46C;  // (u32 hash) -> AglMaterial* (name?)
constexpr u32 DrawItem_Create = 0x0079D6F8;      // (draw record, AglMaterial*, draw packet*) -> item
constexpr u32 kTdMatA = 0x00F88974, kTdMatB = 0x00F88978;  // the two materials, made on first use (name?)
constexpr u32 kTdHashA = 0xE8DB9422, kTdHashB = 0x03DE9E33;
constexpr u32 kTdFrame = 0x010DD600;             // the frame allocator: +20 the cursor, +24 / +28 its range
constexpr u32 kTdBuckets = 0x010DDA8C + 4;       // obj12.h kDrawBuckets
constexpr u32 kItTint = 64, kItBits = 68;        // DrawItem: the 8-byte tint record, u8 bits 1-4 its kind
// AglMaterial render state words: +104 / +112 (bit 31 enable, bits 15-30 a
// value), +108 / +110 / +116 u16, +46 u16 (name?).
constexpr u32 kMsMask = 0x7FFF8000u;

inline u32 SetField15(u32 w, u32 v) { return (w & ~kMsMask) | ((v << 15) & kMsMask); }  // rlwimi v, 15, 1, 16
}  // namespace

// u32 Obj_SubmitTintedDraws(Obj* o, DrawPacket* pk, s32 mode, const u8 tint[4], const f32* value) (name?)
// Queues every draw of the object's class's first topology (last first)
// with material A (mode != 1) or B (mode 1), both looked up and set up on
// first use, each item carrying a copy of the tint and value (in frame
// memory unless the source already lies there, which a stack copy never
// does), into draw bucket 1 (vfunc +8, key 0).
DECOMP(00572420, Obj_SubmitTintedDraws) {
    const u32 o = arg<u32>(ctx, 0), pk = arg<u32>(ctx, 1);
    const s32 mode = arg<s32>(ctx, 2);
    const u32 tint = arg<u32>(ctx, 3), value = arg<u32>(ctx, 4);
    u32 mat = load<u32>(kTdMatA);
    if (mat == 0) {
        const u32 a = static_cast<u32>(call(ctx, Material_FindByHash, {kTdHashA}));
        store<u32>(kTdMatA, a);
        u32 w = load<u32>(a + 104) | 0x80000000u;
        store<u16>(a + 108, 0x8006);
        w = SetField15(w, 1);
        store<u16>(a + 110, 0);
        store<u32>(a + 104, w & 0x7FFFFFFFu);
        const u32 a2 = load<u32>(kTdMatA);
        store<u32>(a2 + 112, load<u32>(a2 + 112) | 0x80000000u);
        store<u16>(a2 + 116, 2305);
        store<u16>(a2 + 46, 4095);
        const u32 b = static_cast<u32>(call(ctx, Material_FindByHash, {kTdHashB}));
        store<u32>(kTdMatB, b);
        const u32 bw = load<u32>(b + 104);
        store<u16>(b + 108, 0x8006);
        store<u16>(b + 110, 1);
        store<u32>(b + 104, SetField15(bw | 0x80000000u, 770));
        mat = load<u32>(kTdMatA);
        const u32 b2 = load<u32>(kTdMatB);
        store<u32>(b2 + 112, load<u32>(b2 + 112) & 0x7FFFFFFFu);
        store<u32>(b2 + 112, SetField15(load<u32>(b2 + 112), 515));
        store<u16>(b2 + 116, 2304);
    }
    if (mode == 1) mat = load<u32>(kTdMatB);
    const u32 tp = load<u32>(o + 208) + 132;
    const u32 n = load<u16>(load<u32>(tp) + 52);
    if (n == 0) {
        ret(ctx, load<u32>(tp));  // r3 as the original leaves it
        return;
    }
    for (u32 j = n - 1;; --j) {
        const u32 rec = load<u32>(load<u32>(load<u32>(tp) + 48) + j * 8);
        const u32 item = static_cast<u32>(call(ctx, DrawItem_Create, {rec, mat, pk}));
        Stack frame(ctx, 16);  // sp+112: the tint record {u8[4], f32}
        const u32 t = frame.addr();
        for (u32 k = 0; k < 4; ++k) store<u8>(t + k, load<u8>(tint + k));
        store<u32>(t + 4, load<u32>(value));
        const u32 cur = load<u32>(kTdFrame + 20);
        if (t < load<u32>(kTdFrame + 24) || !(t < load<u32>(kTdFrame + 28))) {  // cmplw
            store<u32>(kTdFrame + 20, cur + 16);
            const u8 bits = load<u8>(item + kItBits);
            store<u32>(item + kItTint, cur);
            store<u8>(item + kItBits, static_cast<u8>((bits & ~0x1Eu) | 0x10));
            for (u32 k = 0; k < 8; ++k) store<u8>(cur + k, load<u8>(t + k));
        } else {
            const u8 bits = load<u8>(item + kItBits);
            store<u32>(item + kItTint, t);
            store<u8>(item + kItBits, static_cast<u8>((bits & ~0x1Eu) | 0x10));
        }
        const u32 bucket = load<u32>(kTdBuckets);
        const u32 opd = load<u32>(load<u32>(bucket) + 8);
        call_ptr(ctx, opd, {bucket, item, 0, opd, load<u32>(opd)});  // r6 / r7 = the OPD and entry
        if (j == 0) break;
    }
    ret(ctx, 0u);
}

// ---------------------------------------------------------------- PeerList_Destroy
namespace {
constexpr u32 kMemmoveV = 0x0071C9B4;        // obj4.h o4fn::memmove
constexpr u32 kPlPeers = 0x01194598;         // obj3.h kSlotPeers: *(+4) frees the owned blocks (vfunc +16) (name?)
// The list: +0 the 160-byte records (+128 / +132 / +136 owned blocks, +148
// the owner, whose u16 +164 is its record index), +4 the parallel 32-byte
// vectors, +8 u16 the count (obj4.h kPeer*).
constexpr u32 kPrBlockA = 128, kPrBlockB = 132, kPrBlockC = 136, kPrOwner = 148, kObjPeerIndex = 164;

void PeerBlockFree(PPUContext& ctx, u32 blk) {
    const u32 o = load<u32>(load<u32>(kPlPeers) + 4);
    const u32 opd = load<u32>(load<u32>(o) + 16);
    call_ptr(ctx, opd, {o, blk, opd, load<u32>(opd)});  // r5 / r6 = the OPD and entry
}
}  // namespace

// void PeerList_Destroy(PeerList** pp) (name?)
// Removes the records from the last (each via its owner's index: the
// owner's index set to 0xFFFF, the blocks at +132 (only when +128 is set)
// and +136 freed, the record zeroed or the later ones moved down with
// their owners renumbered), frees the arrays and the list, then releases
// pp[1] (vfunc +4). As in the original an empty slot index (0xFFFF) reads
// the owner from address 148.
DECOMP(00596770, PeerList_Destroy) {
    const u32 pp = arg<u32>(ctx, 0);
    const u32 l = load<u32>(pp);
    if (l != 0) {
        for (u32 c = load<u16>(l + 8); (c & 0xFFFF) != 0;) {
            const u32 idx = (c - 1) & 0xFFFF;
            const u32 last_rec = idx != 0xFFFF ? load<u32>(l) + idx * obj4::kPeerSize : 0;
            const u32 k = load<u16>(load<u32>(last_rec + kPrOwner) + kObjPeerIndex);
            const u32 last = idx;
            const u32 off = k * obj4::kPeerSize;
            const u32 rec = load<u32>(l) + off;
            store<u16>(load<u32>(rec + kPrOwner) + kObjPeerIndex, 0xFFFF);
            if (load<u32>(rec + kPrBlockA) != 0) {
                const u32 b = load<u32>(rec + kPrBlockB);
                store<u32>(rec + kPrBlockA, 0);
                if (b != 0) PeerBlockFree(ctx, b);
            }
            store<u32>(rec + kPrBlockB, 0);
            const u32 bc = load<u32>(rec + kPrBlockC);
            if (bc != 0) PeerBlockFree(ctx, bc);
            if (((k ^ (c - 1)) & 0xFFFF) == 0) {
                call(ctx, kMemset, {load<u32>(l) + off, 0, obj4::kPeerSize});
                const u32 v = load<u32>(l + 4) + ((k << 5) & ~31u);
                obj8::VStore(v, obj8::VZero());
                obj8::VStore(v + 16, obj8::VZero());
            } else {
                const u32 nafter = last - k;
                const u32 d = load<u32>(l) + off;
                call(ctx, kMemmoveV, {d, d + obj4::kPeerSize, nafter * obj4::kPeerSize});
                const u32 v = load<u32>(l + 4) + ((k << 5) & ~31u);
                call(ctx, kMemmoveV, {v, v + 32, (nafter << 5) & ~31u});
                if (static_cast<s32>(k) < static_cast<s32>(last)) {
                    const u32 data = load<u32>(l);
                    for (u32 i = k; static_cast<s32>(i) < static_cast<s32>(last); i = (i + 1) & 0xFFFF)
                        store<u16>(load<u32>(data + i * obj4::kPeerSize + kPrOwner) + kObjPeerIndex, static_cast<u16>(i));
                }
            }
            c = load<u16>(l + 8) - 1u;
            store<u16>(l + 8, static_cast<u16>(c));
        }
        for (u32 off : {0u, 4u}) {
            const u32 p = load<u32>(l + off);
            if (p != 0) {
                call(ctx, Mem_FreeFrom, {p, 0});
                store<u32>(l + off, 0);
            }
        }
        call(ctx, Mem_FreeFrom, {l, 0});
        store<u32>(pp, 0);
    }
    const u32 o = load<u32>(pp + 4);
    if (o != 0) {
        obj4::Vcall0(ctx, o, 4);
        store<u32>(pp + 4, 0);
    }
    ret(ctx, 0u);
}

// ---------------------------------------------------------------- SplineDef_ReadChunks
namespace {
constexpr u32 Iff_ReadChunks = 0x00569388;      // (reader, obj*, 0)
constexpr u32 SplineArray_Finish = 0x0055FB88;  // (SplineArray*)
constexpr u32 SplineArray_Bounds = 0x0055FE00;  // (SplineArray*, f32 box[8], f32 sphere[4])
constexpr u32 Array_Reserve = 0x00558F3C;       // (Array*, u16 capacity)
constexpr u32 kSrConsts = 0x0059F5D0;           // f32 {1e-5, 1}
constexpr u32 kSrOrigin = 0x01164B40;           // obj10.h kOrigin
inline f32 Lf32(u32 ea) { return load<f32>(ea); }
}  // namespace

// bool SplineDef_ReadChunks(IffReader* r) (name?)
// Reads the definition's chunks, then turns the point handles into
// offsets (each out handle relative to its point, each in handle the
// previous point's out handle relative to it; the first in and last out
// handles zero) and, unless flag 30 is set, makes every inner point's
// handles collinear (smooth) when they differ; then the arc lengths and
// bounds, and adds the definition to the spline definition map (marked
// unsorted; +8 bit 0x400 = in the map). Returns 1.
DECOMP(0059F5D8, SplineDef_ReadChunks) {
    Stack frame(ctx, 16);  // sp+112: the definition
    const u32 pd = frame.addr();
    store<u32>(pd, 0);
    call(ctx, Iff_ReadChunks, {arg<u32>(ctx, 0), pd, 0});
    const u32 d = load<u32>(pd);
    const u32 arr = d + obj8::kSdData;
    const u32 n = load<u32>(arr) & obj9::kSaAllocMask;
    if (n > 1) {
        for (s32 i = static_cast<s32>(n) - 1; i >= 0; --i) {
            const u32 p = load<u32>(arr + obj9::kSaPoints) + static_cast<u32>(i) * 64;
            const f32 x = Lf32(p), y = Lf32(p + 4), z = Lf32(p + 8);
            store<f32>(p + 32, static_cast<f32>(static_cast<f64>(Lf32(p + 16)) - x));
            store<f32>(p + 36, static_cast<f32>(static_cast<f64>(Lf32(p + 20)) - y));
            store<f32>(p + 40, static_cast<f32>(static_cast<f64>(Lf32(p + 24)) - z));
            if (i > 0) {
                const u32 q = load<u32>(arr + obj9::kSaPoints) + static_cast<u32>(i - 1) * 64;
                const f32 qx = Lf32(q + 32), qy = Lf32(q + 36), qz = Lf32(q + 40);
                store<f32>(p + 16, static_cast<f32>(static_cast<f64>(qx) - x));
                store<f32>(p + 20, static_cast<f32>(static_cast<f64>(qy) - y));
                store<f32>(p + 24, static_cast<f32>(static_cast<f64>(qz) - z));
            }
        }
        const u32 pts = load<u32>(arr + obj9::kSaPoints);
        const u32 lastp = pts + ((load<u32>(arr) & obj9::kSaAllocMask) - 1) * 64;
        for (u32 k = 0; k < 12; k += 4) store<u32>(pts + 16 + k, load<u32>(kSrOrigin + k));
        for (u32 k = 0; k < 12; k += 4) store<u32>(lastp + 32 + k, load<u32>(kSrOrigin + k));
        u32 bits = load<u32>(arr);
        if ((bits & obj9::kSaFlag30) == 0) {
            const f64 eps = Lf32(kSrConsts), one = Lf32(kSrConsts + 4);
            for (u32 i = 1; static_cast<s32>(i) < static_cast<s32>((bits & obj9::kSaAllocMask) - 1); ++i) {
                const u32 p = load<u32>(arr + obj9::kSaPoints) + i * 64;
                const f64 ox = Lf32(p + 32), oy = Lf32(p + 36), oz = Lf32(p + 40);
                const f64 ix = Lf32(p + 16), iy = Lf32(p + 20), iz = Lf32(p + 24);
                f64 dy = static_cast<f32>(oy - iy), dx = static_cast<f32>(ox - ix), dz = static_cast<f32>(oz - iz);
                f64 l2 = static_cast<f32>(dy * dy);
                l2 = static_cast<f32>(std::fma(dx, dx, l2));
                l2 = static_cast<f32>(std::fma(dz, dz, l2));
                if (!(l2 >= eps)) continue;  // fcmpu + cror gt|eq
                const f64 inv = static_cast<f32>(one / static_cast<f64>(static_cast<f32>(std::sqrt(l2))));
                dy = static_cast<f32>(dy * inv);
                dx = static_cast<f32>(dx * inv);
                dz = static_cast<f32>(dz * inv);
                f64 t = static_cast<f32>(dy * oy);
                t = static_cast<f32>(std::fma(dx, ox, t));
                t = static_cast<f32>(std::fma(dz, oz, t));
                const f64 px = static_cast<f32>(-std::fma(dx, t, -ox));  // fnmsubs: ox - dx * t
                const f64 py = static_cast<f32>(-std::fma(dy, t, -oy));
                const f64 pz = static_cast<f32>(-std::fma(dz, t, -oz));
                store<f32>(p + 16, static_cast<f32>(ix - px));
                store<f32>(p + 20, static_cast<f32>(iy - py));
                store<f32>(p + 24, static_cast<f32>(iz - pz));
                store<f32>(p + 32, static_cast<f32>(ox - px));
                store<f32>(p + 36, static_cast<f32>(oy - py));
                store<f32>(p + 40, static_cast<f32>(oz - pz));
                bits = load<u32>(arr);
            }
        }
    }
    call(ctx, SplineArray_Finish, {arr});
    call(ctx, SplineArray_Bounds, {arr, d + 16, d + 48});
    const u32 def = load<u32>(pd);
    const u32 key = load<u32>(def + obj11::kEdNameHash);
    const u32 m = load<u32>(load<u32>(obj8::kSplineDefList) + 16);
    u32 cnt = load<u16>(m + 2);
    const u32 cap = load<u16>(m + 0);
    if (cnt == cap) {
        call(ctx, Array_Reserve, {m, (cap << 1) & 0xFFFEu});  // rlwinm 1,16,30
        cnt = load<u16>(m + 2);
    }
    const u32 e = load<u32>(m + 8);
    store<u32>(e + cnt * 8, key);
    const u32 c2 = load<u16>(m + 2);
    const u32 nc = c2 + 1;
    const u32 peak = load<u16>(m + 4);
    store<u16>(m + 2, static_cast<u16>(nc));
    store<u32>(e + c2 * 8 + 4, def);
    const u32 flags = load<u16>(m + 6);
    store<u16>(m + 4, static_cast<u16>(static_cast<s32>(peak) > static_cast<s32>(nc & 0xFFFF) ? peak : nc));
    store<u16>(m + 6, static_cast<u16>(flags | 1));
    store<u32>(def + 8, load<u32>(def + 8) | 0x400);
    ret(ctx, 1u);
}

// ---------------------------------------------------------------- Bank_PreUpdate
namespace {
constexpr u32 View_GetCurrent = 0x004FDBA8;
constexpr u32 Quads_PrepareForView = 0x00598A24;
constexpr u32 Bank_UpdateView5843C0 = 0x005843C0;   // (bank globals +40, View*) (name?)
constexpr u32 Item_Update58C53C = 0x0058C53C;       // (item) (name?)
constexpr u32 Matrix_Multiply = 0x005541F0;         // (Matrix* a_and_out, const Matrix* b)
constexpr u32 kPuGlobals = 0x01193060;              // obj15.h kBankGlobals: +32 u8 active, +36 Zones*, +48 / +52 u16 the
                                                    // 80-byte views, +5364 u32 the zone mask, +5368 OPD (+5372 its arg) (name?)
constexpr u32 kPuFrame = 0x010DDA78;                // obj12.h kDrawFrame
constexpr u32 kPuLastFrame = 0x00F889F8;            // u32: the frame last updated (name?)
constexpr u32 kPuItems = 0x01183100;                // 16-byte slots, +0 the item (0 = free) (name?)
constexpr u32 kPuItemsEnd = 0x0118B180 + 3100;      // u32*: the end pointer (name?)
constexpr u32 kViewSky = 656;                       // View: +656 -> +4 -> +32 the sky object (vfunc +12 -> enabled), u8 +20 bit 0x40 (name?)
constexpr u32 kViewMatrix816 = 816;
}  // namespace

// void Bank_PreUpdate(Obj* b) (name?)
// Once a frame while the bank globals are active: prepares the quads, the
// view state and the zone mask for the current view; flags the view's sky
// object visible (bit 0x40) when no zone mask is set or a set zone
// matches; rebuilds the light grid; then b's update slots (+28, +12, +32,
// +36), every live item slot's update and (only when there are slots) the
// views' matrices times the current view's +816, the globals' callback,
// and b's +16, +24, +20 slots.
DECOMP(00598B3C, Bank_PreUpdate) {
    const u32 b = arg<u32>(ctx, 0);
    const u32 g = kPuGlobals;
    if (load<u8>(g + 32) == 0) {
        ret(ctx, 0u);
        return;
    }
    const u32 frame = load<u32>(kPuFrame);
    if (load<u32>(kPuLastFrame) == frame) {
        ret(ctx, frame);
        return;
    }
    store<u32>(kPuLastFrame, frame);
    const u32 v = static_cast<u32>(call(ctx, View_GetCurrent));
    call(ctx, Quads_PrepareForView);
    call(ctx, Bank_UpdateView5843C0, {g + 40, v});
    call(ctx, Zones_MaskForPoint, {load<u32>(g + 36), v + 864});
    const u32 sky = load<u32>(load<u32>(v + kViewSky) + 4 + 28);
    if (sky != 0 && static_cast<u32>(obj4::Vcall0(ctx, sky, 12)) != 0) {
        const u32 z = load<u32>(load<u32>(g + 36) + 20);
        const u32 m = load<u32>(g + 5364);
        const bool on = m == 0 || (m & z) != 0;
        store<u8>(sky + 20, static_cast<u8>((load<u8>(sky + 20) & ~0x40u) | (on ? 0x40u : 0u)));
    }
    call(ctx, o18fn::LightGrid_Rebuild);
    for (u32 slot : {28u, 12u, 32u, 36u}) obj4::Vcall0(ctx, b, slot);
    const u32 span = load<u32>(load<u32>(kPuItemsEnd)) - kPuItems;
    if ((span >> 4) != 0) {
        const u32 end = kPuItems + (span & ~15u);
        for (u32 p = kPuItems; p < end; p += 16) {  // cmplw
            const u32 it = load<u32>(p);
            if (it != 0) call(ctx, Item_Update58C53C, {it});
        }
        const u32 vm = static_cast<u32>(call(ctx, View_GetCurrent)) + kViewMatrix816;
        for (u32 i = 0; i < load<u16>(g + 52); ++i) call(ctx, Matrix_Multiply, {load<u32>(g + 48) + i * 80 + 16, vm});
    }
    const u32 cb = load<u32>(g + 5368);
    if (cb != 0) call_ptr(ctx, cb, {load<u32>(g + 5372), cb, load<u32>(cb)});  // r4 / r5 = the OPD and entry
    obj4::Vcall0(ctx, b, 16);
    obj4::Vcall0(ctx, b, 24);
    ret(ctx, obj4::Vcall0(ctx, b, 20));
}

// ---------------------------------------------------------------- LightDef_Read
namespace {
constexpr u32 FileReader_ReadU32 = 0x00569C74;      // (r, u32* out, name)
constexpr u32 FileReader_ReadF32Swapped = 0x005698F4;  // (r, f32* out, name)
constexpr u32 LightDef_FromRecord = 0x00585EF0;     // (LightDef*, const LightRec*)
constexpr u32 kLrStr = 0x00B20000;                  // the field names: kLrStr + offset
// The 84-byte record (obj17.cpp LightDef_FromRecord): +0 u8 type, +1 u8
// light model, +4 lightGroup, +8 colour, +12 intensity, +16 / +20 / +24 the
// model's attenuation terms, +28 nameHash, +32 animHash, +36 name[32], +68
// radius, +72 u8 priority, +76 fadeDist, +80 fadeRange; +84 the version.
constexpr u32 kLrSize = 84;
}  // namespace

// bool LightDef_Read(IffReader* r, LightDef* d)
// The light definition chunk, by version (4 by default): versions <= 1
// have no light model byte; model 0 reads the three lightModel1
// attenuations, others lightModel2's start / end; the priority (127
// before version 3) and the fade distance / range (version 4) follow.
// Then LightDef_FromRecord. Returns 1.
DECOMP(0059C294, LightDef_Read) {
    const u32 r = arg<u32>(ctx, 0), d = arg<u32>(ctx, 1);
    Stack frame(ctx, 96);
    const u32 rec = frame.addr(), ver = rec + kLrSize;
    call(ctx, kMemset, {rec, 0, kLrSize});
    store<u8>(ver, 4);
    call(ctx, Iff_ReadU8, {r, ver, kLrStr + 19250});                       // "ver"
    auto u8f = [&](u32 off, u32 s) { call(ctx, Iff_ReadU8, {r, rec + off, kLrStr + s}); };
    auto u32f = [&](u32 off, u32 s) { call(ctx, Iff_ReadU32, {r, rec + off, kLrStr + s}); };
    auto f32f = [&](u32 off, u32 s) { call(ctx, FileReader_ReadF32Swapped, {r, rec + off, kLrStr + s}); };
    if (static_cast<s32>(load<u8>(ver)) <= 1) {
        u8f(0, 19254);                                                     // "type"
        u32f(4, 19259);                                                    // "lightGroup"
        call(ctx, FileReader_ReadU32, {r, rec + 8, kLrStr + 19270});       // "color"
        f32f(12, 19276);                                                   // "intensity"
        f32f(16, 19286);                                                   // "lightModel1.constantAtten"
        f32f(20, 19312);                                                   // "lightModel1.linearAtten"
        f32f(24, 19336);                                                   // "lightModel1.quadraticAtten"
        u32f(28, 19363);                                                   // "nameHash"
        u32f(32, 19372);                                                   // "animHash"
        call(ctx, FileReader_Read32, {r, rec + 36, kLrStr + 19381});       // "name"
        f32f(68, 19386);                                                   // "radius"
    } else {
        u8f(0, 19254);                                                     // "type"
        u8f(1, 19393);                                                     // "lightModel"
        u32f(4, 19259);                                                    // "lightGroup"
        call(ctx, FileReader_ReadU32, {r, rec + 8, kLrStr + 19270});       // "color"
        f32f(12, 19276);                                                   // "intensity"
        if (load<u8>(rec + 1) == 0) {
            f32f(16, 19286);                                               // "lightModel1.constantAtten"
            f32f(20, 19312);                                               // "lightModel1.linearAtten"
            f32f(24, 19336);                                               // "lightModel1.quadraticAtten"
        } else {
            f32f(16, 19404);                                               // "lightModel2.startAtten"
            f32f(20, 19427);                                               // "lightModel2.endAtten"
        }
        u32f(28, 19363);                                                   // "nameHash"
        u32f(32, 19372);                                                   // "animHash"
        call(ctx, FileReader_Read32, {r, rec + 36, kLrStr + 19381});       // "name"
        f32f(68, 19386);                                                   // "radius"
        if (static_cast<s32>(load<u8>(ver)) < 3) store<u8>(rec + 72, 127);
        else u8f(72, 19448);                                               // "priority"
        if (static_cast<s32>(load<u8>(ver)) >= 4) {
            f32f(76, 19457);                                               // "fadeDist"
            f32f(80, 19466);                                               // "fadeRange"
        }
    }
    call(ctx, LightDef_FromRecord, {d, rec});
    ret(ctx, 1u);
}

// ---------------------------------------------------------------- Bank_ComputeLightBounds
namespace {
constexpr u32 Box_SetPoint = 0x00556F60;   // (f32 box[8], const f32 p[3])
constexpr u32 Box_AddPoint = 0x00556FE8;
constexpr u32 Box_Center = 0x00556F98;     // (const f32 box[8], f32 out[3]) -> out
constexpr u32 kLbConsts = 0x0058FD40;      // f32 {64 (x / z margin), 0 (y margin, the scale without extent), 0.5, 1}
constexpr u32 kLbOrigin = 0x01164B40;      // obj10.h kOrigin
constexpr u32 kLbGlobals = 0x01193060;     // bank globals: +5264 / +5280 the box, +5296 the half size, +5312 its inverse (name?)
constexpr u32 kLiType = 368, kLiPos = 128; // classes.h kLightType (2 = the sun-like type?); the position (name?)
}  // namespace

// void Bank_ComputeLightBounds() (name?)
// The box (from the origin, or from the first) of every type-2 light
// instance's position, grown to at least +-64 in x / z around its centre;
// stored with its half size and the half size's inverses (0 where the
// half size is not positive) in the bank globals.
DECOMP(0058FD50, Bank_ComputeLightBounds) {
    Stack frame(ctx, 48);  // sp+112: the centre, sp+128: the box
    const u32 cen = frame.addr(), box = cen + 16;
    for (u32 k = 0; k < 12; k += 4) store<u32>(box + k, 0x7F7FFFFFu);
    for (u32 k = 0; k < 12; k += 4) store<u32>(box + 16 + k, 0xFF7FFFFFu);
    call(ctx, Box_SetPoint, {box, kLbOrigin});
    bool first = true;
    ForEachInstance(ctx, kLightInsts, [&](u32 it) {
        if (load<s32>(it + kLiType) != 2) return;
        if (first) {
            call(ctx, Box_SetPoint, {box, it + kLiPos});
            first = false;
        } else {
            call(ctx, Box_AddPoint, {box, it + kLiPos});
        }
    });
    const u32 c = static_cast<u32>(call(ctx, Box_Center, {box, cen}));
    const f64 cx = load<f32>(c), cy = load<f32>(c + 4), cz = load<f32>(c + 8);
    const f64 mxz = load<f32>(kLbConsts), my = load<f32>(kLbConsts + 4);
    auto fsub = [](f64 a, f64 b) { return static_cast<f64>(static_cast<f32>(a - b)); };
    auto fadd = [](f64 a, f64 b) { return static_cast<f64>(static_cast<f32>(a + b)); };
    const f64 lx = fsub(cx, mxz), hx = fadd(cx, mxz), ly = fsub(cy, my), lz = fsub(cz, mxz), hy = fadd(cy, my), hz = fadd(cz, mxz);
    f64 bx0 = load<f32>(box), by0 = load<f32>(box + 4), bz0 = load<f32>(box + 8);
    f64 bx1 = load<f32>(box + 16), by1 = load<f32>(box + 20), bz1 = load<f32>(box + 24);
    bx0 = fsub(bx0, lx) >= 0.0 ? lx : bx0;  // fsel: min / max with the margin box
    bx1 = fsub(bx1, hx) >= 0.0 ? bx1 : hx;
    by0 = fsub(by0, ly) >= 0.0 ? ly : by0;
    bz0 = fsub(bz0, lz) >= 0.0 ? lz : bz0;
    by1 = fsub(by1, hy) >= 0.0 ? by1 : hy;
    bz1 = fsub(bz1, hz) >= 0.0 ? bz1 : hz;
    store<f32>(box, static_cast<f32>(bx0));
    store<f32>(box + 4, static_cast<f32>(by0));
    store<f32>(box + 8, static_cast<f32>(bz0));
    store<f32>(box + 16, static_cast<f32>(bx1));
    store<f32>(box + 20, static_cast<f32>(by1));
    store<f32>(box + 24, static_cast<f32>(bz1));
    const f64 half = load<f32>(kLbConsts + 8), one = load<f32>(kLbConsts + 12);
    const f32 sx = static_cast<f32>(fsub(bx1, bx0) * half);
    const f32 sy = static_cast<f32>(fsub(by1, by0) * half);
    const f32 sz = static_cast<f32>(fsub(bz1, bz0) * half);
    const f32 ix = sx > my ? static_cast<f32>(one / sx) : static_cast<f32>(my);
    const f32 iy = sy > my ? static_cast<f32>(one / sy) : static_cast<f32>(my);
    const f32 iz = sz > my ? static_cast<f32>(one / sz) : static_cast<f32>(my);
    const u32 g = kLbGlobals;
    store<f32>(g + 5312, ix);
    store<f32>(g + 5316, iy);
    store<f32>(g + 5320, iz);
    for (u32 k : {0u, 4u, 8u}) store<u32>(g + 5264 + k, load<u32>(box + k));
    for (u32 k : {0u, 4u, 8u}) store<u32>(g + 5280 + k, load<u32>(box + 16 + k));
    store<f32>(g + 5296, sx);
    store<f32>(g + 5300, sy);
    store<f32>(g + 5304, sz);
    ret(ctx, g);
}

// ---------------------------------------------------------------- ApeProceduralMatrixAnimMgr_Create
namespace {
constexpr u32 FreeList_Init = 0x00551A58;     // (list, elem_size, count, heap, bool)
constexpr u32 WorldObj_ForEach = 0x0057F5D8;  // (obj, Opd* fn(node, arg) -> bool, arg, bool recurse)
constexpr u32 World_AddObjects = 0x005947D0;  // (mgr, Collector*, 0)
constexpr u32 kPmVtbl = 0x00B24D98;           // ApeTFreeList<T>'s vtable for the 112-byte animators (name?)
constexpr u32 kPmName = 0x00AF5025;           // "void ApeProceduralMatrixAnimMgr::Create()"
constexpr u32 kPmElem = 112, kPmCount = 32;
constexpr u32 kPmCollectCap = 6000;           // the collector's capacity (obj5.h kCol*)
// The manager: +0 the peer records (a PeerList: +0 160-byte records, +4
// 32-byte vectors, +8 u16 count, +10 u16 the last index, +12 u16 capacity,
// +14 u8), +4 the animator free list (name?).
constexpr u32 kPmExtra = 512;

// The 60-byte ApeTFreeList header the inlined GetAllocator()s build
// (obj6.h ListAllocator's stores, in the same order), then FreeList_Init.
void InitFreeList(PPUContext& ctx, u32 obj, u32 vtbl, u32 name, u32 elem, u32 count) {
    for (u32 off : {8u, 4u, 12u, 20u, 16u, 24u}) store<u32>(obj + off, 0);
    const u16 h54 = load<u16>(obj + 54);
    for (u32 off : {32u, 28u, 36u, 40u, 44u, 48u}) store<u32>(obj + off, 0);
    store<u16>(obj + 52, 0);
    store<u16>(obj + 54, static_cast<u16>(h54 & 0x7FFF));
    store<u32>(obj, vtbl);
    store<u32>(obj + 56, name);
    call(ctx, FreeList_Init, {obj, elem, count, 0, 0});
}
}  // namespace

// void ApeProceduralMatrixAnimMgr::Create(Mgr* m)
// The animator free list (32 x 112 bytes), then every root geometry
// instance's tree collected (6000 entries; each root's run length stored
// at its first entry's flag), a peer record list sized for the collected
// objects plus 512, and World_AddObjects over the collection (its heap
// buffers freed unless they were the inline ones).
DECOMP(0059541C, ApeProceduralMatrixAnimMgr_Create) {
    const u32 m = arg<u32>(ctx, 0);
    const u32 fl = static_cast<u32>(call(ctx, Mem_Alloc, {60, 0, 16}));
    if (fl != 0) InitFreeList(ctx, fl, kPmVtbl, kPmName, kPmElem, kPmCount);
    store<u32>(m + 4, fl);
    const u32 alloc = obj6::ListAllocator(ctx, kGeomInsts.hdr, kGeomInsts.vtbl, kGeomInsts.name, kGeomInsts.size);
    const u32 first = static_cast<u32>(call(ctx, o18fn::Obj_GetField4, {alloc}));
    Stack frame(ctx, 32);  // sp+112: the collector (obj5.h kCol*)
    const u32 col = frame.addr();
    store<u32>(col + obj5::kColCount, 0);
    store<u32>(col + obj5::kColCapacity, kPmCollectCap);
    store<u32>(col + obj5::kColRun, 0);
    store<u8>(col + obj5::kColIsInline, 0);
    store<u8>(col + obj5::kColOption, 0);
    store<u32>(col + obj5::kColData, static_cast<u32>(call(ctx, Mem_Alloc, {kPmCollectCap * 4, 0, 16})));
    store<u32>(col + obj5::kColFlags, static_cast<u32>(call(ctx, Mem_Alloc, {load<u32>(col + obj5::kColCapacity), 0, 16})));
    for (u32 it = first != 0 ? first + kNodeHeader : 0; it != 0;) {
        if (load<u32>(it + obj4::kObjParentLink) == 0) {
            store<u32>(col + obj5::kColRun, 0);
            call(ctx, WorldObj_ForEach, {it, obj5::kCollectOpd, col, 1});
            const u32 run = load<u32>(col + obj5::kColRun);
            if (run != 0) store<u8>(load<u32>(col + obj5::kColFlags) + (load<u32>(col + obj5::kColCount) - run), static_cast<u8>(run));
        }
        const u32 next = load<u32>(it - kNodeHeader);
        it = next != 0 ? next + kNodeHeader : 0;
    }
    const u32 pl = static_cast<u32>(call(ctx, Mem_Alloc, {16, 0, 16}));
    if (pl != 0) {
        const u32 n = load<u32>(col + obj5::kColCount);
        const u32 cap = n + kPmExtra;
        store<u32>(pl + 0, static_cast<u32>(call(ctx, Mem_Alloc, {cap * obj4::kPeerSize, 0, 128})));
        const u32 v = static_cast<u32>(call(ctx, Mem_Alloc, {(cap << 5) & ~31u, 0, 128}));
        store<u32>(pl + 4, v);
        store<u16>(pl + 12, static_cast<u16>(cap));
        store<u16>(pl + 8, 0);
        store<u16>(pl + 10, static_cast<u16>(n + kPmExtra - 1));
        store<u8>(pl + 14, 0);
    }
    store<u32>(m + 0, pl);
    call(ctx, World_AddObjects, {m, col, 0});
    const u32 inl = load<u8>(col + obj5::kColIsInline);
    if (inl != 0) {
        ret(ctx, inl);  // r3 as the original leaves it
        return;
    }
    call(ctx, Mem_FreeFrom, {load<u32>(col + obj5::kColData), 0});
    call(ctx, Mem_FreeFrom, {load<u32>(col + obj5::kColFlags), 0});
}

// ---------------------------------------------------------------- CameraInstance_ReadChunks
namespace {
constexpr u32 WorldObj_Construct = 0x00580708;  // (obj, kind)
constexpr u32 kCiHolder = 0x00F8F150;           // obj.h kMapHolderA: the camera instance list header (+8 / +12 the callback, +16 the map)
constexpr u32 kCiKind = 11;
constexpr u32 kVfnAddChild = 20;                // (parent, child, s32 tag)
}  // namespace

// bool CameraInstance_ReadChunks(IffReader* r, Obj* parent) (name?)
// A new camera instance from its free list (constructed as kind 11, the
// ApeCameraInstance vtable), the header's callback told before (0) and
// after (1) its chunks are read; then added to the parent (tag -1) or,
// without one, moved by the point offset and marked dirty (the inlined
// WorldObj_MarkDirty); then entered into the instance map by its name
// hash (marked unsorted; +8 bit 0x400 = in the map). Returns 1.
DECOMP(005A600C, CameraInstance_ReadChunks) {
    const u32 r = arg<u32>(ctx, 0), parent = arg<u32>(ctx, 1);
    const u32 alloc = obj6::ListAllocator(ctx, kCiHolder, obj5::kCameraInstListVtbl, obj5::kStrCameraInstList,
                                          obj5::kCameraInstanceSize);
    const u32 no = static_cast<u32>(obj4::Vcall0(ctx, alloc, obj8::kVfnListAlloc));
    u32 o = 0;
    if (no != 0) {
        call(ctx, WorldObj_Construct, {no, kCiKind});
        store<u32>(no, obj5::kVtblCameraInstance);
        o = no;
    }
    u32 hdr = load<u32>(kCiHolder);
    u32 cb = load<u32>(hdr + kHdrCallback);
    if (cb != 0) call_ptr(ctx, cb, {o, load<u32>(hdr + kHdrCallbackArg), 0, cb, load<u32>(cb)});  // r6 / r7 = the OPD and entry
    call(ctx, Iff_ReadChunks, {r, o});
    hdr = load<u32>(kCiHolder);
    cb = load<u32>(hdr + kHdrCallback);
    if (cb != 0) call_ptr(ctx, cb, {o, load<u32>(hdr + kHdrCallbackArg), 1, cb, load<u32>(cb)});
    if (parent != 0) {
        const u32 opd = load<u32>(load<u32>(parent) + kVfnAddChild);
        call_ptr(ctx, opd, {parent, o, ~0ull, opd, load<u32>(opd)});  // r6 / r7 = the OPD and entry
    } else {
        const f32 x = load<f32>(o + 64), y = load<f32>(o + 68), z = load<f32>(o + 72);
        const u32 off = obj5::kPointOffset;
        store<f32>(o + 68, static_cast<f32>(static_cast<f64>(y) + load<f32>(off + 4)));
        store<f32>(o + 64, static_cast<f32>(static_cast<f64>(x) + load<f32>(off)));
        store<f32>(o + 72, static_cast<f32>(static_cast<f64>(z) + load<f32>(off + 8)));
        obj4::MarkDirtyInline(ctx, o);
    }
    const u32 key = load<u32>(o + obj::kObjNameHash);
    const u32 m = load<u32>(load<u32>(kCiHolder) + 16);
    u32 cnt = load<u16>(m + 2);
    const u32 cap = load<u16>(m + 0);
    if (cnt == cap) {
        call(ctx, Array_Reserve, {m, (cap << 1) & 0xFFFEu});  // rlwinm 1,16,30
        cnt = load<u16>(m + 2);
    }
    const u32 e = load<u32>(m + 8);
    store<u32>(e + cnt * 8, key);
    store<u32>(e + load<u16>(m + 2) * 8 + 4, o);
    const u32 nc = load<u16>(m + 2) + 1u;
    const u32 peak = load<u16>(m + 4);
    const u32 flags = load<u16>(m + 6);
    store<u16>(m + 2, static_cast<u16>(nc));
    store<u16>(m + 4, static_cast<u16>(static_cast<s32>(peak) > static_cast<s32>(nc & 0xFFFF) ? peak : nc));
    store<u16>(m + 6, static_cast<u16>(flags | 1));
    store<u32>(o + 8, load<u32>(o + 8) | 0x400);
    ret(ctx, 1u);
}
