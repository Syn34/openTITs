// World objects, part 11: small functions of the world-object range:
// record lookups, definition names, the bank's wait slots and resets.
#include "obj11.h"

using namespace sly4;
using namespace sly4::obj11;

namespace {

constexpr u32 kSysTimerUsleep = 141;
constexpr u32 kWaitUs = 300;
constexpr u32 kThreadedLoad = 0x00F88944;   // u8: loading runs on worker threads (name?)
constexpr u32 kBusyA = 0x0117E200;          // u32: nonzero while worker A is busy (name?)
constexpr u32 kBusyB = 0x0118BF80;          // u32: likewise worker B (name?)

constexpr u32 kGeomDef_FindTopo = 0x00585B98;   // (def, key) -> {AglTopology*, ...}* or 0 (name?)
constexpr u32 kTopology_Apply = 0x005099B8;     // (AglTopology*, arg) (name?)
constexpr u32 kSplineArray_Finish = 0x0055FB88; // (SplineArray*) (name?)
constexpr u32 kSplineArray_Bounds = 0x0055FE00; // (SplineArray*, f32 box[8], f32 sphere[4]) (name?)

constexpr u32 kDefName = 64, kDefNameSize = 32, kDefNameHash = 96;

// The world block at 0x01193060 (obj4.h kZones - 36): +40 a block freed
// by World_FreeBlock40 with its u16 count at +44; +64 112-byte records
// (u16 count at +68) keyed by their +64 word (name?).
constexpr u32 kWorldBlock = 0x01193060;
constexpr u32 kWb40 = 40, kWb44 = 44, kWbRecs = 64, kWbRecCount = 68;
constexpr u32 kWbRecSize = 112, kWbRecKey = 64;

// Sleeps 300 us at a time while *busy != 0 (only with threaded loading).
// r3 = 0 (the last load) on return, as the original leaves it.
void WaitWhileBusy(PPUContext& ctx, u32 busy) {
    if (load<u8>(kThreadedLoad) != 0 && load<u32>(busy) != 0) {
        do {
            ctx.gpr[11] = kSysTimerUsleep;
            syscall(ctx, kSysTimerUsleep, {kWaitUs});
        } while (load<u32>(busy) != 0);
    }
    ctx.gpr[3] = 0;
}

}  // namespace

// bool GeomDef_Vfn20(ApeGeomDef* d, u32 key)
// ApeGeomDef slot +20: when GeomDef_FindTopo finds an entry, its topology
// gets Topology_Apply(key) and the result is 1 (name?).
DECOMP(00585DEC, GeomDef_Vfn20) {
    const u32 key = arg<u32>(ctx, 1);
    const u32 e = static_cast<u32>(call(ctx, kGeomDef_FindTopo, {arg<u32>(ctx, 0), key}));
    if (e == 0) {
        ret(ctx, 0u);
        return;
    }
    call(ctx, kTopology_Apply, {load<u32>(e), key});
    ret(ctx, 1u);
}

// Rec80* Rec80_Find(const {u32, Rec80* data, u16 count}* list, u32 key)
// The first 80-byte record whose +64 word equals key, or 0 (name?).
// r4..r9 as the original leaves them.
DECOMP(005848CC, Rec80_Find) {
    const u32 list = arg<u32>(ctx, 0), key = arg<u32>(ctx, 1);
    const u32 n = load<u16>(list + 8);
    ctx.gpr[6] = 0;
    ctx.gpr[7] = n;
    if (n == 0) {
        ctx.gpr[8] = 0;
        ret(ctx, 0u);
        return;
    }
    const u32 data = load<u32>(list + 4);
    ctx.gpr[5] = data;
    u32 i = 0;
    do {
        const u32 k = load<u32>(data + i * 80 + 64);
        ctx.gpr[9] = k;
        if (k == key) {
            ctx.gpr[4] = static_cast<u64>(i) << 4;
            ctx.gpr[8] = i;
            ret(ctx, data + i * 80);
            return;
        }
        ++i;
    } while (i < n);
    ctx.gpr[8] = i;
    ret(ctx, 0u);
}

// void Bank_WaitA() / Bank_WaitB(): the bank vtable 0x00B24770 slots +16
// / +32: with threaded loading, sleep until the worker is idle (name?).
DECOMP(00599804, Bank_WaitA) { WaitWhileBusy(ctx, kBusyA); }
DECOMP(00599FA8, Bank_WaitB) { WaitWhileBusy(ctx, kBusyB); }

// void World_FreeBlock40()
// Frees the world block's +40 allocation (when set) and zeroes it and the
// u16 at +44 (name?).
DECOMP(005A5864, World_FreeBlock40) {
    const u32 p = load<u32>(kWorldBlock + kWb40);
    if (p != 0) {
        call(ctx, cls::cfn::Mem_FreeFrom, {p, 0});
        store<u32>(kWorldBlock + kWb40, 0);
    }
    store<u16>(kWorldBlock + kWb44, 0);
    ctx.gpr[3] = 0;
}

// Rec112* World_FindRec(u32 key)
// The first of the world block's 112-byte records whose +64 word equals
// key, or 0 (name?). r4..r9 as the original leaves them.
DECOMP(0059B724, World_FindRec) {
    const u32 key = arg<u32>(ctx, 0);
    const u32 n = load<u16>(kWorldBlock + kWbRecCount);
    ctx.gpr[4] = 0;
    ctx.gpr[7] = n;
    ctx.gpr[8] = 0;
    if (static_cast<s32>(n) <= 0) {
        ctx.gpr[5] = kWorldBlock;
        ret(ctx, 0u);
        return;
    }
    const u32 data = load<u32>(kWorldBlock + kWbRecs);
    ctx.gpr[5] = data;
    u32 i = 0;
    do {
        const u32 rec = data + i * kWbRecSize;
        const u32 k = load<u32>(rec + kWbRecKey);
        ctx.gpr[6] = rec;
        ctx.gpr[9] = k;
        if (k == key) {
            ctx.gpr[8] = i;
            ret(ctx, rec);
            return;
        }
        i = (i + 1) & 0xFFFF;
        ctx.gpr[8] = i;
    } while (static_cast<s32>(i) < static_cast<s32>(n));
    ret(ctx, 0u);
}

// void SplineDef_Rebuild(ApeSplineDef* d, bool keep)
// Unless keep, finishes the point array (+112); then its bounds into the
// box (+16) and sphere (+48) (name?).
DECOMP(0059F110, SplineDef_Rebuild) {
    const u32 d = arg<u32>(ctx, 0);
    if ((arg<u32>(ctx, 1) & 0xFF) == 0) call(ctx, kSplineArray_Finish, {d + obj8::kSdData});
    call(ctx, kSplineArray_Bounds, {d + obj8::kSdData, d + 16, d + 48});
}

// void Def_SetName(Def* d, const char* name, bool hash)
// The definition's name (+64, 32 bytes) and, when asked, its hash (+96).
DECOMP(00584BDC, Def_SetName) {
    const u32 d = arg<u32>(ctx, 0), name = arg<u32>(ctx, 1);
    const bool hash = (arg<u32>(ctx, 2) & 0xFF) != 0;
    call(ctx, obj::ofn::String_Copy, {d + kDefName, name, kDefNameSize});
    if (hash)
        store<u32>(d + kDefNameHash, static_cast<u32>(call(ctx, obj::ofn::String_HashNoCase, {name})));
    else
        ctx.gpr[3] = 0;  // the flag test's result, as the original leaves it
}

namespace {

constexpr u32 kIff_ReadChunks = 0x00569388;     // (reader, obj)
constexpr u32 kWorldObj_VisitInView = 0x00583864;
constexpr u32 kWorldObj_SetFlag4000 = 0x0057FDB4;
constexpr u32 kIff_ReadNamedU32 = 0x005642AC;   // (reader, u32* out, const char* name) (name?)
constexpr u32 kIff_Read5643C4 = 0x005643C4;     // (reader, u32* out, 0) (name?)
constexpr u32 kSkeletonDef_New = 0x00584FD4;    // (u32 bones) -> ApeSkeletonDef* (name?)
constexpr u32 kStrNumBones = 0x00B24A64;        // "numBones"
constexpr u32 kListener_LocalDirection = 0x005532E4;  // (f32 out[3], const f32 p[3], const Matrix*)
constexpr u32 kSpline_Nearest = 0x00560A40;     // (SplineData*, const f32 p[3], u32* seg, f32* dist) -> f1
constexpr u32 kString_CopyN = 0x00553638;       // (dst, src, n)
constexpr u32 kWorldList = 0x011944D8;          // PtrVector*: Obj** at +0, u16 count at +8 (name?)
constexpr u32 kVisitForce = 2;                  // WorldObj_VisitInView flags: visit hidden objects too
constexpr u32 kObjFlag4000 = 0x4000;            // +164 bit (name?)

}  // namespace

// bool Iff_HandleSkeletonRef(IffReader* r, ApeGeomDef* d)
// Chunk handler (OPD 0x00F3C530): reads the child chunks into a skeleton
// definition pointer; a different one replaces d +164 (a reference
// taken, the old one released). The reference the read returned is then
// dropped (also when it is 0). Returns 1.
DECOMP(00584F54, Iff_HandleSkeletonRef) {
    const u32 r = arg<u32>(ctx, 0), d = arg<u32>(ctx, 1);
    Stack frame(ctx, 128 - 112);
    const u32 slot = frame.addr();  // sp+112
    store<u32>(slot, 0);
    call(ctx, kIff_ReadChunks, {r, slot, 0});
    const u32 nw = load<u32>(slot), old = load<u32>(d + obj2::kClassRes);
    if (old != nw) {
        store<u32>(d + obj2::kClassRes, nw);
        if (nw != 0) store<u16>(nw, static_cast<u16>(load<u16>(nw) + 1));
        if (old != 0) call(ctx, cls::cfn::SkeletonDef_Release, {old});
    }
    call(ctx, cls::cfn::SkeletonDef_Release, {load<u32>(slot)});
    ret(ctx, 1u);
}

// bool Iff_HandleSkeletonDef(IffReader* r, ApeSkeletonDef** out)
// Chunk handler (OPD 0x00F3C538): reads a word (discarded) and "numBones",
// then *out = SkeletonDef_New(numBones). Returns 1.
DECOMP(005851E8, Iff_HandleSkeletonDef) {
    const u32 r = arg<u32>(ctx, 0), out = arg<u32>(ctx, 1);
    Stack frame(ctx, 144 - 112);
    const u32 bones = frame.addr(), word = bones + 4;  // sp+112 / sp+116
    store<u32>(bones, 0);
    call(ctx, kIff_Read5643C4, {r, word, 0, 0});  // r6 = 0 as well
    call(ctx, kIff_ReadNamedU32, {r, bones, kStrNumBones});
    store<u32>(out, static_cast<u32>(call(ctx, kSkeletonDef_New, {load<u32>(bones)})));
    ret(ctx, 1u);
}

// f32 SplineInstance_Nearest(ApeSplineInstance* o, const f32 p[3], u32* seg, f32* dist)
// p in the instance's local space (its world matrix +80), then the
// nearest point of its class's spline (+208 -> +112) (name?).
DECOMP(005927FC, SplineInstance_Nearest) {
    const u32 o = arg<u32>(ctx, 0), p = arg<u32>(ctx, 1), seg = arg<u32>(ctx, 2), dist = arg<u32>(ctx, 3);
    Stack frame(ctx, 160 - 112);
    const u32 local = frame.addr();  // sp+112
    call(ctx, kListener_LocalDirection, {local, p, o + obj6::kObjWorld});
    callf(ctx, kSpline_Nearest, {load<u32>(o + obj4::kObjClassPtr) + obj8::kSdData, local, seg, dist}, {});
}

// void WorldList_VisitAll()
// WorldObj_VisitInView(obj, 2) for the world list's objects, last first.
DECOMP(00599080, WorldList_VisitAll) {
    const u32 list = load<u32>(kWorldList);
    u32 i = load<u16>(list + 8);
    if (i == 0) {
        ctx.gpr[3] = 0x01190000;  // the list address's high half, as the original leaves it
        return;
    }
    do {
        --i;
        call(ctx, kWorldObj_VisitInView, {load<u32>(load<u32>(list) + i * 4), kVisitForce});
    } while (i != 0);
    ctx.gpr[3] = 0;  // the last index, as the original leaves it
}

// void WorldObj_SetFlag4000(Obj* o, u8 on, bool recurse)
// +164 bit 0x4000 = on (bit 0); with recurse, for the whole subtree (name?).
DECOMP(0057FDB4, WorldObj_SetFlag4000) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 on = arg<u32>(ctx, 1);
    const bool recurse = (arg<u32>(ctx, 2) & 0xFF) != 0;
    store<u32>(o + obj::kObjFlags, (load<u32>(o + obj::kObjFlags) & ~kObjFlag4000) | ((on & 1) << 14));
    if (!recurse) return;
    for (u32 c = load<u32>(o + obj4::kObjFirst); c != 0; c = load<u32>(c + obj4::kObjNext))
        call(ctx, kWorldObj_SetFlag4000, {c, on & 0xFF, 1});
}

// void SplineDef_Load(ApeSplineDef* d, const SplineRecord* src)
// The name (src +0, at most 31 characters) and its hash, then the point
// array for src +34 points (src +32 bit 0: closed, bit 1: flag 30) (name?).
DECOMP(0059F174, SplineDef_Load) {
    const u32 d = arg<u32>(ctx, 0), src = arg<u32>(ctx, 1);
    const u32 name = d + obj8::kSdName;
    call(ctx, kString_CopyN, {name, src, 31});
    const u32 h = static_cast<u32>(call(ctx, obj::ofn::String_HashNoCase, {name}));
    const u32 bits = load<u16>(src + 32);
    store<u32>(d + kDefNameHash, h);
    const u32 count = load<u16>(src + 34);
    ctx.gpr[7] = std::countl_zero(bits & 2);  // left over from the bool (not read)
    call(ctx, obj8::o8fn::SplineArray_Init, {d + obj8::kSdData, count, bits & 1, (bits & 2) != 0 ? 1u : 0u});
}

namespace {

constexpr u32 kGeomCmd_Simple = 0x0058AC70;     // (obj, u8, u8, s32) (name?)
constexpr u32 kGeomCmd_Full = 0x005A0E68;       // (obj, u8, u8, s32, u32, u8, u32, rec +16, u8) (name?)
constexpr u32 kTopology_Dump = 0x0050A19C;      // (AglTopology*, s32 depth) (name?)
constexpr u32 kIff_ReadNamedU16 = 0x00564340;   // (reader, u16* out, const char* name) (name?)
constexpr u32 kStrNumRegions = 0x00B24DB0;      // "numRegions"
constexpr u32 kZonesPtr = 0x01193060 + 36;      // obj4.h kZones: +16 u8 count, +17 / +18 u8 (name?)

}  // namespace

// void GeomInstance_Visit2(GeomCmd* c)
// The third geometry-instance callback (OPD 0x00F3C870): the per-frame pass
// (0x00583518) and 0x005832FC for the object (c +0), then the command:
// with c +22 == 0 the simple form (c +20, +21, +4), else the full one with
// c +8, +22, +12, &c +16 and c +23 as the stack argument (name?).
DECOMP(005A1584, GeomInstance_Visit2) {
    const u32 c = arg<u32>(ctx, 0);
    call(ctx, o11fn::Obj_Visit583518, {c});
    call(ctx, o11fn::Obj_Visit5832FC, {load<u32>(c)});
    const u32 mode = load<u8>(c + 22);
    const u64 a4 = static_cast<u64>(static_cast<s64>(static_cast<s32>(load<u32>(c + 4))));
    if (mode == 0) {
        call(ctx, kGeomCmd_Simple, {load<u32>(c), load<u8>(c + 20), load<u8>(c + 21), a4});
        return;
    }
    call(ctx, kGeomCmd_Full, {load<u32>(c), load<u8>(c + 20), load<u8>(c + 21), a4, load<u32>(c + 8), mode,
                              load<u32>(c + 12), c + 16, load<u8>(c + 23)});
}

// void GeomDef_Vfn24(ApeGeomDef* d, s32 depth)
// ApeGeomDef slot +24: above depth 1, Topology_Dump(depth - 1) for each
// topology (the u8 count at +112 re-read every step) (name?).
DECOMP(00585E3C, GeomDef_Vfn24) {
    const u32 d = arg<u32>(ctx, 0);
    const s32 depth = static_cast<s32>(arg<u32>(ctx, 1));
    if (depth <= 1) return;
    u32 n = load<u8>(d + obj2::kClassTopoCount);
    if (n != 0) {
        const u64 d1 = static_cast<u64>(static_cast<s64>(depth - 1));
        u32 i = 0;
        do {
            call(ctx, kTopology_Dump, {load<u32>(d + obj2::kClassTopos + i * 8), d1});
            ++i;
            n = load<u8>(d + obj2::kClassTopoCount);
        } while (i < n);
    }
    ctx.gpr[3] = n;  // the last count read, as the original leaves it
}

// bool Iff_HandleRegions(IffReader* r)
// Chunk handler (OPD 0x00F3C6A0): the zones' +17 = +16, reads a word and
// "numRegions" (both discarded), then the zones' +18 counts the chunk.
// Returns 1 (name?).
DECOMP(0059AF84, Iff_HandleRegions) {
    const u32 r = arg<u32>(ctx, 0);
    Stack frame(ctx, 144 - 112);
    const u32 num = frame.addr();  // sp+112 (u16), sp+114 the word
    const u32 z = load<u32>(kZonesPtr);
    const u8 b = load<u8>(z + 16);
    store<u8>(z + 17, b);
    store<u16>(num, 0);
    call(ctx, kIff_Read5643C4, {r, num + 2, 0, 0});  // r6 = 0 as well
    call(ctx, kIff_ReadNamedU16, {r, num, kStrNumRegions});
    const u32 z2 = load<u32>(kZonesPtr);
    store<u8>(z2 + 18, static_cast<u8>(load<u8>(z2 + 18) + 1));
    ret(ctx, 1u);
}

// void WorldObjBase_Dump(Obj* o, s32 depth, u32 flags)
// The base Dump slot (0x00B24660 +60): the tree (flags | 1); with depth >
// 0 and a skeleton (vfunc +28, asked twice), the skeleton one level down.
DECOMP(005842E4, WorldObjBase_Dump) {
    const u32 o = arg<u32>(ctx, 0);
    const u64 depth = ctx.gpr[4];  // passed on as it came (64-bit)
    call(ctx, cls::ifn::WorldObj_DumpTree, {o, depth, ctx.gpr[5] | 1});
    if (static_cast<s32>(depth) <= 0) return;
    if (static_cast<u32>(obj4::Vcall0(ctx, o, obj4::kVfnGetSkeleton)) == 0) return;
    const u32 sk = static_cast<u32>(obj4::Vcall0(ctx, o, obj4::kVfnGetSkeleton));
    call(ctx, cls::ifn::Skeleton_DumpTree, {sk, static_cast<u64>(static_cast<s64>(static_cast<s32>(depth - 1)))});
}

namespace {

constexpr u32 kSpline_TangentAt = 0x00560690;   // (SplineData*, f1 t, f32 out[4], bool) (name?)
constexpr u32 kIff_Read5637D4 = 0x005637D4;     // (reader) (name?)

}  // namespace

// void SplineInstance_TangentAt(ApeSplineInstance* o, f32 t, f32 out[4], bool flag)
// The class spline's (+208 -> +112) direction at t, rotated into world
// space by the world matrix's rows 0-2 (+80; no translation). out is
// written as a whole 16-byte vector (stvx) (name?).
DECOMP(005A8034, SplineInstance_TangentAt) {
    const u32 o = arg<u32>(ctx, 0), out = arg<u32>(ctx, 2);
    // r4 passes through unread.
    callf(ctx, kSpline_TangentAt, {load<u32>(o + obj4::kObjClassPtr) + obj8::kSdData, ctx.gpr[4], out, arg<u32>(ctx, 3) & 0xFF},
          {farg(ctx, 0)});
    const u32 m = o + obj6::kObjWorld;
    obj8::Vec4 v = obj8::VMadd(obj8::VLoad(m), obj8::VSplat(obj8::VLoadLeft(out), 0), obj8::VZero());
    v = obj8::VMadd(obj8::VLoad(m + 16), obj8::VSplat(obj8::VLoadLeft(out + 4), 0), v);
    v = obj8::VMadd(obj8::VLoad(m + 32), obj8::VSplat(obj8::VLoadLeft(out + 8), 0), v);
    obj8::VStore(out, v);
}

// bool Iff_HandleRecKeys(IffReader* r)
// Chunk handler (OPD 0x00F3C6C8): after 0x005637D4, one word per world
// record (World_FindRec's 112-byte records, the u16 count re-read each
// step) into its +64 key. Returns 1 (name?).
DECOMP(0059B474, Iff_HandleRecKeys) {
    const u32 r = arg<u32>(ctx, 0);
    Stack frame(ctx, 160 - 112);
    const u32 word = frame.addr();  // sp+112
    u32 rec = load<u32>(kWorldBlock + kWbRecs);
    ctx.gpr[4] = 0x01190000;  // left over from the address (not read)
    call(ctx, kIff_Read5637D4, {r, 0x01190000});
    for (u32 i = 0; i < load<u16>(kWorldBlock + kWbRecCount); ++i, rec += kWbRecSize) {
        call(ctx, kIff_ReadNamedU32, {r, word, 0});
        store<u32>(rec + kWbRecKey, load<u32>(word));
    }
    ret(ctx, 1u);
}
