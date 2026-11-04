// World objects, part 16: small helpers.
#include "obj16.h"

using namespace sly4;
using namespace sly4::obj16;

namespace {

// ---------------------------------------------------------------- controller recording
// The input record (platform.cpp kInputRecord): +4 the File, +8 s32 the
// mode (0 off, 1 write, 2 play).
constexpr u32 kInputRec = 0x01164AA0;
constexpr u32 kIrFile = 4, kIrMode = 8;
constexpr s32 kIrWrite = 1, kIrPlay = 2;
constexpr u32 kVfnFileReadStream = 12, kVfnFileWriteStream = 16;  // file/zip.h kFileRead / kFileWrite
// The input device fields that are recorded: +4 u16, +32 96 bytes, +128
// u32, +16 the buttons (name?).
constexpr u32 kIdRec4 = 4, kIdRec32 = 32, kIdRec128 = 128, kIdButtonsNow = 16;

// ---------------------------------------------------------------- file open
constexpr u32 Fs_Open = 0x005618B8;          // (path, mode, options in r5-r7) -> FsHandle*
constexpr u32 Fios_OpenAsync = 0x00561714;   // (path, mode, options in r5-r7) -> FsOp*
constexpr u32 FsOp_Wait5612B8 = 0x005612B8;  // (op)
constexpr u32 FsOp_Sync = 0x00561450;        // (op) (name?)
constexpr u32 FsOp_ResultData = 0x005616C8;  // (op) -> the op's result (name?)
constexpr u32 FsOp_Release = 0x00561514;     // (op) (name?)
constexpr u32 fios_Free = 0x005FE784;        // (scheduler, fios_op*)
constexpr u32 Mem_FreeFrom = 0x0054EE6C;     // (p, pool)
constexpr u32 kStrModeRb = 0x00AF4810;       // "rb"
constexpr u32 kFiosOpDone = 152;             // fios_op u32: nonzero when finished
constexpr u32 kFiosOpScheduler = 156;
constexpr u32 kOptPumpCallbacks = 0x20;      // options word 0 bit: open asynchronously (name?)
constexpr u32 kSysutilMutex = 0x0116B288;    // sys_lwmutex_t (obj14)
constexpr u32 kSysutilLockTimeout = 100000;  // usec
constexpr u32 sys_timer_usleep = 141;
constexpr u32 kPollUsec = 500;

}  // namespace

// void InputDevice_RecordOrPlay(InputDevice* d)  (InputDevice_UpdateButtons's)
// With the controller record file open: mode 1 writes, mode 2 reads, the
// device's +4 (2 bytes), +32 (96), +128 (4) and +16 (4) through the file's
// vfunc +16 / +12 (file, buf, size, 0) (name?).
DECOMP(00562998, InputDevice_RecordOrPlay) {
    const u32 d = arg<u32>(ctx, 0);
    const u32 file = load<u32>(kInputRec + kIrFile);
    if (file == 0) return;  // r3 = d
    const s32 mode = load<s32>(kInputRec + kIrMode);
    ctx.gpr[3] = d + kIdRec4;
    u32 slot;
    if (mode == kIrWrite) {
        slot = kVfnFileWriteStream;
    } else if (mode == kIrPlay) {
        slot = kVfnFileReadStream;
    } else {
        return;  // r3 = d + 4
    }
    const u32 bufs[4][2] = {{d + kIdRec4, 2}, {d + kIdRec32, 96}, {d + kIdRec128, 4}, {d + kIdButtonsNow, 4}};
    for (const auto& b : bufs) {
        // The OPD and its entry go in r7 / r8, as the original's call sequence leaves them.
        const u32 opd = load<u32>(load<u32>(file) + slot);
        call_ptr(ctx, opd, {file, b[0], b[1], 0, opd, load<u32>(opd)});
    }
}

// FsHandle* Fs_OpenRead(const char* path, FsOptions options in r4-r6)
// Fs_Open(path, "rb", options); with option bit 0x20 the open goes through
// Fios_OpenAsync instead and is polled every 500 us while the system
// utility callbacks run (under the sysutil mutex), until it is done (then
// waited for) or a callback check returns nonzero; the op's result is
// returned and the op freed (name?).
DECOMP(00571B90, Fs_OpenRead) {
    const u32 path = arg<u32>(ctx, 0);
    const u64 a = ctx.gpr[4], b = ctx.gpr[5], c = ctx.gpr[6];
    if (((a >> 32) & kOptPumpCallbacks) == 0) {
        ret(ctx, static_cast<u32>(call(ctx, Fs_Open, {path, kStrModeRb, a, b, c})));
        return;
    }
    const u32 op = static_cast<u32>(call(ctx, Fios_OpenAsync, {path, kStrModeRb, a, b, c}));
    if (op == 0) {
        ret(ctx, 0u);
        return;
    }
    u32 stop = 0;
    for (;;) {
        if (load<u32>(load<u32>(op) + kFiosOpDone) != 0) {  // read after an isync
            call(ctx, FsOp_Wait5612B8, {op});
            break;
        }
        if (stop != 0) break;
        syscall(ctx, sys_timer_usleep, {kPollUsec});
        call(ctx, imp::sys_lwmutex_lock, {kSysutilMutex, kSysutilLockTimeout});
        stop = static_cast<u32>(call(ctx, imp::cellSysutilCheckCallback));
        call(ctx, imp::sys_lwmutex_unlock, {kSysutilMutex});
    }
    call(ctx, FsOp_Sync, {op});
    const u32 result = static_cast<u32>(call(ctx, FsOp_ResultData, {op}));
    call(ctx, FsOp_Release, {op});
    const u32 fop = load<u32>(op);
    if (fop != 0) call(ctx, fios_Free, {load<u32>(fop + kFiosOpScheduler), fop});
    call(ctx, Mem_FreeFrom, {op, 0});
    ret(ctx, result);
}

namespace {

// ---------------------------------------------------------------- view records
// A record tested against the views (Records_Process58CE24's): +44 the
// owner, +60 f32 the fade radius, +64 f32[4] the bounding sphere (centre,
// radius), +76 f32 the plane offset, +80 f32[3] the extent, +92 u32 the
// view mask (0 = all), +96 the source (its +80 is handed to the fine
// test), +100 f32 the squared range, +124 u16 flags (bits 0-1 both set,
// 0x80 on, 0x04 off), +126 u8 (> 128 to take part) (name?).
constexpr u32 kVrOwner = 44, kVrFade = 60, kVrSphere = 64, kVrPlaneOff = 76, kVrExtent = 80;
constexpr u32 kVrMask = 92, kVrSource = 96, kVrRange2 = 100, kVrFlags = 124, kVrLevel = 126;
// The view set: +0 f32[3] the eye, +16 f32[3] the forward axis, +32 f32
// the scale, +36 u32 the mask, +48 the 224-byte views (two frusta: rows
// +96/+112/+128 and offsets +144, rows +160/+176/+192 and offsets +208),
// +276 u32* per view: the output cursor, +284 u32 the view count (name?).
constexpr u32 kVsEye = 0, kVsAxis = 16, kVsScale = 32, kVsMask = 36, kVsViews = 48, kVsOut = 276, kVsCount = 284;
constexpr u32 kViewSize = 224;
constexpr u32 Sphere_FrustumFine = 0x007A6EFC;  // (view, const f32 centre[3], const f32 extent[3], src +80) -> bool (name?)
constexpr u32 kFadeConsts = 0x0058C9A4;         // f32 {5.0, 1.0, 0.0}

f32 lf(u32 ea) { return load<f32>(ea); }
f32 fmadd_s(f32 a, f32 c, f32 b) { return static_cast<f32>(std::fma(static_cast<f64>(a), static_cast<f64>(c), static_cast<f64>(b))); }
// vminfp: a NaN in either lane gives a NaN.
f32 vmin(f32 a, f32 b) { return (a != a || b != b) ? std::bit_cast<f32>(0x7FC00000u) : (a < b ? a : b); }

}  // namespace

// void Records_AddToViews(-, Rec* rec, ViewSet* vs) (name?)
// An enabled record in range of the eye (its forward distance minus the
// plane offset, scaled, no more than min(sqrt(range2), fade radius)) is
// appended to every view whose frusta its sphere touches: both frusta's
// planes give min(d1, d2) per lane; all lanes >= -radius and some lane <
// radius means a partial overlap, settled by Sphere_FrustumFine. The
// appended 16-byte entry is {source, owner, f32 fade}, fade = clamp((limit
// - distance) / 5, 0, 1), and the view's cursor advances by 16 (atomic
// add). Like the original, the first view is always tested, even with a
// zero count.
DECOMP(0058C9B0, Records_AddToViews) {
    const u32 rec = arg<u32>(ctx, 1), vs = arg<u32>(ctx, 2);
    if (rec == 0) return;
    const u32 flags = load<u16>(rec + kVrFlags);
    ctx.gpr[3] = flags;
    if ((flags & 0x4) != 0 || (flags & 0x80) == 0 || load<u32>(rec + kVrOwner) == 0) return;
    const u32 mask = load<u32>(rec + kVrMask);
    const bool in_mask = mask == 0 || (mask & load<u32>(vs + kVsMask)) != 0;
    const bool level = load<u8>(rec + kVrLevel) > 128;
    ctx.gpr[3] = level ? 1 : 0;
    if (!in_mask || (flags & 3) != 3 || !level) return;
    const u32 sph = rec + kVrSphere;
    const f32 sq = static_cast<f32>(std::sqrt(static_cast<f64>(lf(rec + kVrRange2))));
    f32 d = lf(vs + kVsAxis + 4) * (lf(sph + 4) - lf(vs + kVsEye + 4));
    d = fmadd_s(lf(vs + kVsAxis), lf(sph) - lf(vs + kVsEye), d);
    d = -fmadd_s(lf(vs + kVsAxis + 8), lf(sph + 8) - lf(vs + kVsEye + 8), d);  // fnmadds
    d = (d - lf(rec + kVrPlaneOff)) * lf(vs + kVsScale);
    const f32 fade_r = lf(rec + kVrFade);
    const f32 limit = (sq - fade_r) >= 0.0f ? fade_r : sq;  // fsel
    ctx.gpr[3] = vs + kVsAxis;
    if (d > limit) return;
    const f32 t = (limit - d) / lf(kFadeConsts);
    const f32 one = lf(kFadeConsts + 4), zero = lf(kFadeConsts + 8);
    const f32 lo = t < one ? t : one;
    const f32 fade = lo > zero ? lo : zero;
    const u32 src_data = load<u32>(rec + kVrSource) + 80;
    u32 i = 0;
    do {
        const u32 v = vs + kVsViews + kViewSize * i;
        const f32 X = lf(sph), Y = lf(sph + 4), Z = lf(sph + 8), W = lf(sph + 12);
        bool all_in = true, some_cross = false;
        for (u32 k = 0; k < 4; ++k) {
            f32 a = fmadd_s(lf(v + 96 + 4 * k), X, lf(v + 144 + 4 * k));
            f32 b = fmadd_s(lf(v + 160 + 4 * k), X, lf(v + 208 + 4 * k));
            a = fmadd_s(lf(v + 112 + 4 * k), Y, a);
            b = fmadd_s(lf(v + 176 + 4 * k), Y, b);
            a = fmadd_s(lf(v + 128 + 4 * k), Z, a);
            b = fmadd_s(lf(v + 192 + 4 * k), Z, b);
            if (W > vmin(a, b)) some_cross = true;      // vcmpgtfp.
            if (!(vmin(W + a, W + b) >= 0.0f)) all_in = false;  // vcmpgefp.
        }
        u32 state = all_in ? (some_cross ? 2 : 1) : 0;
        if (state != 0) {
            Stack frame(ctx, 32);  // the original's sp+112 (centre) / sp+128 (extent)
            const u32 c = frame.addr(), e = c + 16;
            for (u32 k = 0; k < 3; ++k) store<u32>(c + 4 * k, load<u32>(sph + 4 * k));  // lfs / stfs
            for (u32 k = 0; k < 3; ++k) store<u32>(e + 4 * k, load<u32>(rec + kVrExtent + 4 * k));
            if (state == 2) state = static_cast<u32>(call(ctx, Sphere_FrustumFine, {v, c, e, src_data}));
            if (state != 0) {
                const u32 cur = load<u32>(vs + kVsOut + 4 * i);
                const u32 out = load<u32>(cur);
                store<u32>(out, load<u32>(rec + kVrSource));
                store<f32>(out + 8, fade);
                store<u32>(out + 4, load<u32>(rec + kVrOwner));
                for (;;) {  // lwarx / stwcx.: *cur += 16
                    const u32 old = load<u32>(cur);
                    if (mem::cas32(cur, old, old + 16)) break;
                }
            }
        }
        ++i;
        ctx.gpr[3] = load<u32>(vs + kVsCount);
    } while (i < load<u32>(vs + kVsCount));
}

namespace {

// ---------------------------------------------------------------- meshes
constexpr u32 MeshMap_Find = 0x0079C8C4;  // (map, const u32* key) -> value slot
constexpr u32 kMeshMap = 0x010DDC80;      // u32 Map*: the meshes by hash (name?)

// ---------------------------------------------------------------- draw lists
// The three draw lists (640 {s32 key, Rec*} entries, u32 count at +5120)
// and the 80-byte records they point to (u32 count at 0x010ECE30, records
// at 0x010E0630) (name?).
constexpr u32 kDrawListA = 0x010EF63C, kDrawListB = 0x010ECE34, kDrawListC = 0x010EE238;
constexpr u32 kDlCount = 5120;
constexpr u32 kLodRecCount = 0x010ECE30, kLodRecs = 0x010E0630, kLodRecSize = 80;
constexpr u32 kForceListB = 0x010F0A41;   // u8 (next to shader4_frame's cleared byte 0x010F0A40) (name?)
constexpr u32 kTgtMesh = 4;               // the target's +4: +68 u8 nonzero = list A (name?)
constexpr u32 kTgtFlags = 64;             // u8: bits 1-2 nonzero = list B (name?)

}  // namespace

// Mesh* Mesh_FindByHash(u32 hash): the mesh map's value for hash (name?).
DECOMP(004FED94, Mesh_FindByHash) {
    Stack frame(ctx, 16);  // the original's sp+112
    const u32 key = frame.addr();
    store<u32>(key, arg<u32>(ctx, 0));
    const u32 slot = static_cast<u32>(call(ctx, MeshMap_Find, {load<u32>(kMeshMap), key}));
    ret(ctx, load<u32>(slot));
}

// void LodTarget_Apply(Target* t, const f32 m[3][4], u32 x, u32 level) (name?)
// Appends {-level, record} to one of the three draw lists (A when t's
// mesh +68 is set, else B when 0x010F0A41 is set or t's +64 bits 1-2 are,
// else C), the record being the next 80-byte one: the matrix's three rows
// (x, y, z each), +64 t, +68 x, +72 0, +76 its own index. The list count
// is re-read for the record pointer, as in the original.
DECOMP(00527FEC, LodTarget_Apply) {
    const u32 t = arg<u32>(ctx, 0), m = arg<u32>(ctx, 1), x = arg<u32>(ctx, 2), level = arg<u32>(ctx, 3);
    u32 list;
    if (load<u8>(load<u32>(t + kTgtMesh) + 68) != 0) {
        list = kDrawListA;
    } else if (load<u8>(kForceListB) != 0 || ((load<u8>(t + kTgtFlags) >> 1) & 3) != 0) {
        list = kDrawListB;
    } else {
        list = kDrawListC;
    }
    store<u32>(list + 8 * load<u32>(list + kDlCount), 0u - level);
    const u32 idx = load<u32>(kLodRecCount);
    const u32 r = kLodRecs + kLodRecSize * idx;
    store<u32>(r + 0, load<u32>(m + 0));  // lfs / stfs
    store<u32>(r + 4, load<u32>(m + 4));
    const u32 n = load<u32>(list + kDlCount);
    store<u32>(r + 8, load<u32>(m + 8));
    store<u32>(r + 16, load<u32>(m + 16));
    store<u32>(r + 20, load<u32>(m + 20));
    store<u32>(list + kDlCount, n + 1);
    store<u32>(r + 24, load<u32>(m + 24));
    store<u32>(r + 32, load<u32>(m + 32));
    store<u32>(list + 8 * n + 4, r);
    store<u32>(kLodRecCount, idx + 1);
    store<u32>(r + 36, load<u32>(m + 36));
    store<u32>(r + 40, load<u32>(m + 40));
    store<u32>(r + 48, load<u32>(m + 48));
    store<u32>(r + 64, t);
    store<u32>(r + 52, load<u32>(m + 52));
    store<u32>(r + 68, x);
    const u32 last = load<u32>(m + 56);
    store<u32>(r + 56, last);
    store<u32>(r + 72, 0);
    store<u32>(r + 76, idx);
    ctx.fpr[1] = std::bit_cast<f32>(last);
    ctx.gpr[3] = t;
}

namespace {

constexpr u32 Shader_FindByHash = 0x004FE990;   // (hash) -> Shader*
constexpr u32 Material_Alloc = 0x004FF2E8;      // () -> Material*
constexpr u32 Mesh_CreateSphere = 0x0051BC20;   // (material, u32, u32) -> draw (name?)
constexpr u32 Topology_Construct = 0x00508690;  // (topo)
constexpr u32 Topology_AddDraw = 0x00508B94;    // (topo, draw, draw +16)
constexpr u32 kDefaultMesh = 0x00F671B8;        // u32 AglTopology*: made on first use (name?)
constexpr u32 kTopologyList = 0x00F8F124;       // u32 ApeTListAllocator<AglTopology> header*
constexpr u32 kVtblTopologyFL = 0x00B23E00;     // its ApeTFreeList vtable
constexpr u32 kStrTopologyList = 0x00AF12C9;    // "ApeTFreeList<T> &ApeTListAllocator<T>::GetAllocator() [with T = AglTopology]"
constexpr u32 kTopologySize = 112;
constexpr u32 kDefaultMeshShader = 0x03DE9E33;  // shader hash (name?)
constexpr u32 kVfnListAlloc = 12;               // allocator vfunc: () -> T*
constexpr u32 kSphereSegs = 4;

}  // namespace

// AglTopology* Mesh_Default(): the default mesh, made on first use: a
// topology from its free list (none without a list header) holding a 4 x 4
// sphere with a material of shader 0x03DE9E33. The global is stored before
// the sphere is added (name?).
DECOMP(00526300, Mesh_Default) {
    if (load<u32>(kDefaultMesh) != 0) {
        ret(ctx, load<u32>(kDefaultMesh));
        return;
    }
    u32 topo = 0;
    if (load<u32>(kTopologyList) != 0) {
        // As in the original, a failed allocator allocation is stored and then called through.
        const u32 alloc = obj6::ListAllocator(ctx, kTopologyList, kVtblTopologyFL, kStrTopologyList, kTopologySize);
        const u32 opd = load<u32>(load<u32>(alloc) + kVfnListAlloc);
        const u32 p = static_cast<u32>(call_ptr(ctx, opd, {alloc, opd, load<u32>(opd)}));
        if (p != 0) {
            call(ctx, Topology_Construct, {p});
            topo = p;
        }
    }
    store<u32>(kDefaultMesh, topo);
    const u32 shader = static_cast<u32>(call(ctx, Shader_FindByHash, {kDefaultMeshShader}));
    const u32 mat = static_cast<u32>(call(ctx, Material_Alloc, {shader}));  // r3 = the shader, as the original leaves it
    store<u32>(mat + 16, shader);
    const u32 draw = static_cast<u32>(call(ctx, Mesh_CreateSphere, {mat, kSphereSegs, kSphereSegs}));
    call(ctx, Topology_AddDraw, {load<u32>(kDefaultMesh), draw, load<u32>(draw + 16)});
    ret(ctx, load<u32>(kDefaultMesh));
}

namespace {

constexpr u32 kPrimPoolCur = 0x01164E58;     // u32 PrimPool* {FreeList* list; u32 count} (debug.cpp kPrimPoolCurrent)
constexpr u32 kVtblPrimFL = 0x00B24418;      // ApeTFreeList<ApeDebug::Prim> vtable
constexpr u32 kStrPrimList = 0x00AF35A3;     // "ApeTFreeList<T> &ApeTListAllocator<T>::GetAllocator() [with T = ApeDebug::Prim]"
constexpr u32 kPrimSize = 124;
constexpr u32 kFlLive = 4;                   // FreeList: +4 the first live node (items at node +16)
constexpr u32 kVfnListFree = 16;             // allocator vfunc: (T*)

// The inlined GetAllocator of the current prim pool, as ListAllocator
// (obj6.h) but with operator new: made on first use; a failed allocation
// stores a null allocator, as in the original.
u32 PrimAllocator(PPUContext& ctx, u32 hdr_ea = kPrimPoolCur) {
    const u32 hdr = load<u32>(hdr_ea);
    const u32 alloc = load<u32>(hdr);
    if (alloc != 0) return alloc;
    const u32 obj = static_cast<u32>(call(ctx, sly4::fn::Mem_New, {60}));
    if (obj != 0) {
        const u32 count = load<u32>(hdr + 4);
        for (u32 off : {8u, 4u, 12u, 20u, 16u, 24u}) store<u32>(obj + off, 0);
        const u16 h54 = load<u16>(obj + 54);
        for (u32 off : {32u, 28u, 36u, 40u, 44u, 48u}) store<u32>(obj + off, 0);
        store<u16>(obj + 52, 0);
        store<u16>(obj + 54, static_cast<u16>(h54 & 0x7FFF));
        store<u32>(obj, kVtblPrimFL);
        store<u32>(obj + 56, kStrPrimList);
        call(ctx, obj6::o6fn::FreeList_Init, {obj, kPrimSize, count, 0, 0});
    }
    store<u32>(hdr, obj);
    return obj;
}

}  // namespace

// void Debug_ReleasePrimPool(): every live prim of the current pool given
// back to its allocator (vfunc +16), the next one read before each free
// (symbols.txt: Debug_DrawPrimPool) (name?).
DECOMP(0056B2B8, Debug_ReleasePrimPool) {
    const u32 alloc = PrimAllocator(ctx);
    const u32 head = load<u32>(alloc + kFlLive);
    u32 item = head != 0 ? head + 16 : 0;
    while (item != 0) {
        const u32 next_node = load<u32>(item - 16);
        const u32 next = next_node != 0 ? next_node + 16 : 0;
        const u32 a = PrimAllocator(ctx);
        // The OPD and its entry go in r5 / r6, as the original's call sequence leaves them.
        const u32 opd = load<u32>(load<u32>(a) + kVfnListFree);
        call_ptr(ctx, opd, {a, item, opd, load<u32>(opd)});
        item = next;
    }
}

namespace {

constexpr u32 WorldObj_ApplyHeaderB = 0x0057D3E8;  // (obj, const InstanceHeader*)
constexpr u32 Light_Alloc = 0x00526494;            // () -> AglLight*
// Instance header fields (name?).
constexpr u32 kIhFlags = 48;                       // u32: 0x10, 0x100, 0x200 / 0x400 (a 2-bit mode), 0x1000, 0x2000
constexpr u32 kIhLightRef = 104, kIhLightRefB = 108, kIh112 = 112, kIh113 = 113, kIh114 = 114;
// World object fields (name?).
constexpr u32 kObjKindWord = 4;                    // bits 10-15 the kind
constexpr u32 kObj168 = 168, kObj208 = 208, kObjLight = 288, kObjLightRef = 292, kObj320 = 320, kObj324 = 324;
constexpr u32 kLightFlags212 = 212;                // u8: 0x80 cleared on a new reference (name?)
constexpr u32 kVfnObjLightChanged = 36;            // (obj) (name?)

}  // namespace

// void WorldObj_ApplyHeader(Obj* o, const InstanceHeader* h) (name?)
// WorldObj_ApplyHeaderB, then: +320 = h +113; +321 bit 0 = h +114 bit 0,
// bit 1 = (h +112 != 0); +322 bit 7 = (h flags & 0x10). For kinds 8 and 10
// the +320 word's bits 9-12 get the mode from flags bits 9-10 (4 for 3, 3
// for 2, 1 for 1 (+168 |= 0x200000), 2 for 0 (likewise)). h +104 nonzero
// goes to +324 with +168 |= 0x800000. The light reference (+292) is h +108
// when flags has 0x100 or 0x1000, else o +208's +104 (left alone without
// one); a light already there (+288) gets its +212 bit 7 cleared. Flags
// 0x1000 / 0x2000 set +168 bits 0x20000 / 0x10000. A reference without a
// light gets a new one (Light_Alloc) and vfunc +36.
DECOMP(00580B18, WorldObj_ApplyHeader) {
    const u32 o = arg<u32>(ctx, 0), h = arg<u32>(ctx, 1);
    call(ctx, WorldObj_ApplyHeaderB, {o, h});
    const u32 b113 = load<u8>(h + kIh113);
    u32 word = load<u32>(o + kObj320);
    store<u8>(o + kObj320, static_cast<u8>(b113));
    u32 b321 = (load<u8>(o + kObj320 + 1) & ~1u) | (load<u8>(h + kIh114) & 1);
    store<u8>(o + kObj320 + 1, static_cast<u8>(b321));
    b321 = (b321 & ~2u) | (load<u8>(h + kIh112) != 0 ? 2u : 0u);
    u32 b322 = load<u8>(o + kObj320 + 2);
    const u32 kind = (load<u32>(o + kObjKindWord) >> 10) & 0x3F;
    store<u8>(o + kObj320 + 1, static_cast<u8>(b321));
    b322 = (b322 & ~0x80u) | ((load<u32>(h + kIhFlags) & 0x10) != 0 ? 0x80u : 0u);
    u32 f168 = load<u32>(o + kObj168);
    store<u8>(o + kObj320 + 2, static_cast<u8>(b322));
    if (kind == 10 || kind == 8) {
        const u32 mode = load<u32>(h + kIhFlags) & 0x600;
        // The word as it now stands (bytes 0-2 just written, byte 3 as read).
        word = (b113 << 24) | (b321 << 16) | (b322 << 8) | (word & 0xFF);
        u32 k;
        if (mode == 0x600) {
            k = 4;
        } else if (mode == 0x400) {
            k = 3;
        } else {
            k = mode == 0x200 ? 1 : 2;
            f168 |= 0x00200000;  // obj5.h kObj168Type12
            store<u32>(o + kObj168, f168);
        }
        store<u32>(o + kObj320, (word & ~0x1E00u) | (k << 9));
    }
    const u32 ref104 = load<u32>(h + kIhLightRef);
    if (ref104 != 0) {
        store<u32>(o + kObj324, ref104);
        store<u32>(o + kObj168, f168 | 0x00800000);
    }
    const u32 flags = load<u32>(h + kIhFlags);
    bool set_ref = true;
    u32 ref = 0;
    if ((flags & 0x100) != 0 || (flags & 0x1000) != 0) {
        ref = load<u32>(h + kIhLightRefB);
    } else if (load<u32>(o + kObj208) != 0) {
        ref = load<u32>(load<u32>(o + kObj208) + 104);
    } else {
        set_ref = false;
    }
    if (set_ref) {
        const u32 light = load<u32>(o + kObjLight);
        store<u32>(o + kObjLightRef, ref);
        if (light != 0) store<u8>(light + kLightFlags212, static_cast<u8>(load<u8>(light + kLightFlags212) & 0x7F));
    }
    if ((load<u32>(h + kIhFlags) & 0x1000) != 0) store<u32>(o + kObj168, load<u32>(o + kObj168) | 0x00020000);
    if ((load<u32>(h + kIhFlags) & 0x2000) != 0) store<u32>(o + kObj168, load<u32>(o + kObj168) | 0x00010000);
    if (load<u32>(o + kObjLightRef) != 0 && load<u32>(o + kObjLight) == 0) {
        const u32 light = static_cast<u32>(call(ctx, Light_Alloc, {0}));  // r3 = the +288 just read
        const u32 vt = load<u32>(o);
        store<u32>(o + kObjLight, light);
        const u32 opd = load<u32>(vt + kVfnObjLightChanged);
        call_ptr(ctx, opd, {o, opd, load<u32>(opd)});
    }
}

namespace {

constexpr u32 kInputMgrAlloc = 0x01164AA0;   // u32 ApeTFreeList<ApeInputDevice>* (game/load.h kInputMgr +0)
constexpr u32 kVtblInputFL = 0x00B243F8;
constexpr u32 kStrInputList = 0x00AF382A;    // "static ApeTFreeList<ApeInputDevice> &ApeInputMgr::GetAllocator()"
constexpr u32 kInputDevSize = 296, kInputDevCount = 4;
constexpr u32 kVfnInputDestroy = 8;          // device vfunc: (d) (name?)
constexpr u32 Archive_Return1A = 0x0056F690; // () -> 1

// ApeInputMgr::GetAllocator(), inlined: made on first use with operator
// new (a failed allocation stores a null allocator, as in the original).
u32 InputAllocator(PPUContext& ctx) {
    const u32 alloc = load<u32>(kInputMgrAlloc);
    if (alloc != 0) return alloc;
    const u32 obj = static_cast<u32>(call(ctx, sly4::fn::Mem_New, {60}));
    if (obj != 0) {
        for (u32 off : {8u, 4u, 12u, 20u, 16u, 24u}) store<u32>(obj + off, 0);
        const u16 h54 = load<u16>(obj + 54);
        for (u32 off : {32u, 28u, 36u, 40u, 44u, 48u}) store<u32>(obj + off, 0);
        store<u16>(obj + 52, 0);
        store<u16>(obj + 54, static_cast<u16>(h54 & 0x7FFF));
        store<u32>(obj, kVtblInputFL);
        store<u32>(obj + 56, kStrInputList);
        call(ctx, obj6::o6fn::FreeList_Init, {obj, kInputDevSize, kInputDevCount, 0, 0});
    }
    store<u32>(kInputMgrAlloc, obj);
    return obj;
}

}  // namespace

// bool InputMgr_Shutdown()  (Platform_Shutdown's)
// Every live input device destroyed (vfunc +8) and given back to the
// allocator, the next one read first; then Archive_Return1A; returns 1
// (symbols.txt: Archive_Ready) (name?).
DECOMP(0055E56C, InputMgr_Shutdown) {
    const u32 alloc = InputAllocator(ctx);
    const u32 head = load<u32>(alloc + kFlLive);
    u32 item = head != 0 ? head + 16 : 0;
    while (item != 0) {
        const u32 next_node = load<u32>(item - 16);
        const u32 next = next_node != 0 ? next_node + 16 : 0;
        // The OPDs and their entries go in r4 / r5 and r5 / r6, as the original leaves them.
        const u32 dopd = load<u32>(load<u32>(item) + kVfnInputDestroy);
        call_ptr(ctx, dopd, {item, dopd, load<u32>(dopd)});
        const u32 a = InputAllocator(ctx);
        const u32 opd = load<u32>(load<u32>(a) + kVfnListFree);
        call_ptr(ctx, opd, {a, item, opd, load<u32>(opd)});
        item = next;
    }
    call(ctx, Archive_Return1A);
    ret(ctx, 1u);
}

namespace {

constexpr u32 Def_FindByHash57CE08 = 0x0057CE08;  // (u32 hash) -> definition (name?)
constexpr u32 String_Format = 0x005534D0;         // (fmt, ...) -> the rotating buffer
constexpr u32 kStrDefNotFound = 0x00AF4E94;       // "Definition not found for instance: (%s)"
// Instance header (the shared part): +0 / +12 the x / z axis rows, +24 the
// position, +36 the scale, +48 flags (bit 0 -> +164 0x8000, bit 1 ->
// 0x4000, bits 6-7 copied), +52 the name hash, +60 char[32] the name,
// +92 the definition hash (name?).
constexpr u32 kIhAxisX = 0, kIhAxisZ = 12, kIhPos = 24, kIhScale = 36, kIhNameHash = 52, kIhName = 60, kIhDefHash = 92;
constexpr u32 kVfnObjName = 12;                   // () -> const char*
constexpr u32 kVfnObjSetDef = 44;                 // (def) (name?)

f32 fmsub_s(f32 a, f32 c, f32 b) {
    return static_cast<f32>(std::fma(static_cast<f64>(a), static_cast<f64>(c), -static_cast<f64>(b)));
}

}  // namespace

// bool WorldObj_ApplyHeaderB(Obj* o, const InstanceHeader* h) (name?)
// The name hash (+204) and name (+170), the matrix (x and z axis rows as
// given, y = z cross x, the position), marked dirty (the inlined
// WorldObj_MarkDirty), the scale (WorldObj_SetScale), the +164 flag bits,
// then the definition (looked up by h +92; "Definition not found for
// instance: (%s)" formatted when missing) handed to vfunc +44. Returns 1.
DECOMP(0057D3E8, WorldObj_ApplyHeaderB) {
    const u32 o = arg<u32>(ctx, 0), h = arg<u32>(ctx, 1);
    store<u32>(o + obj::kObjNameHash, load<u32>(h + kIhNameHash));
    call(ctx, obj::ofn::String_Copy, {o + obj::kObjName, h + kIhName, 32});
    const f32 x0 = load<f32>(h + kIhAxisX), x1 = load<f32>(h + kIhAxisX + 4), x2 = load<f32>(h + kIhAxisX + 8);
    const f32 z0 = load<f32>(h + kIhAxisZ), z1 = load<f32>(h + kIhAxisZ + 4), z2 = load<f32>(h + kIhAxisZ + 8);
    const f32 t10 = z2 * x1, t11 = z0 * x2, t12 = z1 * x0;
    const u32 m = o + obj::kObjMatrix;
    for (u32 k = 0; k < 3; ++k) store<u32>(m + 4 * k, load<u32>(h + kIhAxisX + 4 * k));  // lfs / stfs
    for (u32 k = 0; k < 3; ++k) store<u32>(m + 32 + 4 * k, load<u32>(h + kIhAxisZ + 4 * k));
    store<u32>(m + 48, load<u32>(h + kIhPos));
    store<u32>(m + 52, load<u32>(h + kIhPos + 4));
    store<u32>(m + 56, load<u32>(h + kIhPos + 8));
    store<f32>(m + 16, fmsub_s(z1, x2, t10));
    store<f32>(m + 20, fmsub_s(z2, x0, t11));
    store<f32>(m + 24, fmsub_s(z0, x1, t12));
    obj4::MarkDirtyInline(ctx, o);
    {
        Stack frame(ctx, 16);  // the original's sp+112
        const u32 s = frame.addr();
        for (u32 k = 0; k < 3; ++k) store<u32>(s + 4 * k, load<u32>(h + kIhScale + 4 * k));
        ctx.fpr[1] = load<f32>(h + kIhScale);  // f1 as the original's loads leave it
        call(ctx, obj::ofn::WorldObj_SetScale, {o, s, 0});
    }
    u32 w = load<u32>(o + obj::kObjFlags);
    w = (w & ~0x8000u) | ((load<u32>(h + kIhFlags) & 1) != 0 ? 0x8000u : 0u);
    store<u32>(o + obj::kObjFlags, w);
    w = (w & ~0x4000u) | ((load<u32>(h + kIhFlags) & 2) != 0 ? 0x4000u : 0u);
    store<u32>(o + obj::kObjFlags, w);
    w = (w & ~0xC0u) | (load<u32>(h + kIhFlags) & 0xC0);
    store<u32>(o + obj::kObjFlags, w);
    const u32 def = static_cast<u32>(call(ctx, Def_FindByHash57CE08, {load<u32>(h + kIhDefHash)}));
    if (def == 0) {
        const u32 nopd = load<u32>(load<u32>(o) + kVfnObjName);
        const u32 name = static_cast<u32>(call_ptr(ctx, nopd, {o, nopd, load<u32>(nopd)}));
        call(ctx, String_Format, {kStrDefNotFound, name});
    }
    const u32 opd = load<u32>(load<u32>(o) + kVfnObjSetDef);
    call_ptr(ctx, opd, {o, def, opd, load<u32>(opd)});
    ret(ctx, 1u);
}

namespace {

constexpr u32 Fn_597C9C = 0x00597C9C;                 // (list, u32, s32, u32* a, u32* b, u32* na, u32* nb) (name?)
constexpr u32 WorldObj_UpdateTransforms = 0x00597814; // (list, obj)
constexpr u32 World_AddGroup = 0x005922A8;            // (list, obj)
constexpr u32 kTickMask = 0x010DDA70;                 // u8: the list kinds that tick (bit = 1 << list +8) (name?)
constexpr u32 kTickBusy = 0x0116BD80;                 // u32: nonzero while the jobs run (name?)
constexpr u32 kTickJobs = 0x0116BD60, kTickJobCount = 0x0116BD64;  // 112-byte job records (name?)
constexpr u32 kTickJobSize = 112;
constexpr u32 kTickPollUsec = 300;

}  // namespace

// void Lights_Tick2(List* l) (name?)
// For a non-empty list whose kind (+8) is enabled in 0x010DDA70: waits
// (300 us sleeps) until the jobs are done, clears the list's +14, then
// per job record (the count at 0x0116BD64 is decremented as it goes):
// Fn_597C9C(l, job +16, job +20, job +0 + 4 * job +8, job +4 + 4 * job
// +12, &na, &nb), then WorldObj_UpdateTransforms for the first job +8 + na
// objects of job +0 and World_AddGroup for the first job +12 + nb of job
// +4. (The original's cache prefetches are left out.)
DECOMP(005981D4, Lights_Tick2) {
    const u32 l = arg<u32>(ctx, 0);
    const u32 a = load<u32>(l);
    if (a == 0 || load<u16>(a + 8) == 0) return;
    const u32 sh = load<u8>(l + 8) & 63;  // slw
    const u32 bit = (sh < 32 ? (1u << sh) : 0u) & 0xFF;
    if ((bit & load<u8>(kTickMask)) == 0) return;
    while (load<u32>(kTickBusy) != 0) syscall(ctx, sys_timer_usleep, {kTickPollUsec});
    u32 job = load<u32>(kTickJobs);
    store<u8>(load<u32>(l) + 14, 0);
    if (load<u32>(kTickJobCount) == 0) return;
    Stack frame(ctx, 16);  // the original's sp+112 / sp+116
    const u32 pna = frame.addr(), pnb = pna + 4;
    for (;;) {
        store<u32>(pna, 0);
        store<u32>(pnb, 0);
        const u32 j0 = load<u32>(job), j4 = load<u32>(job + 4);
        call(ctx, Fn_597C9C,
             {l, load<u32>(job + 16), static_cast<u64>(static_cast<s64>(load<s32>(job + 20))), j0 + (load<u32>(job + 8) << 2),
              j4 + (load<u32>(job + 12) << 2), pna, pnb});
        const u32 na = load<u32>(pna) + load<u32>(job + 8);
        store<u32>(pna, na);
        const u32 nb = load<u32>(pnb) + load<u32>(job + 12);
        store<u32>(pnb, nb);
        const u32 objs = load<u32>(job), groups = load<u32>(job + 4);
        for (u32 i = 0; i != na; ++i) call(ctx, WorldObj_UpdateTransforms, {l, load<u32>(objs + 4 * i)});
        for (u32 i = 0; i != nb; ++i) call(ctx, World_AddGroup, {l, load<u32>(groups + 4 * i)});
        job += kTickJobSize;
        const u32 left = load<u32>(kTickJobCount) - 1;
        store<u32>(kTickJobCount, left);
        if (left == 0) break;
    }
}

namespace {

// The slot job's parameter block (0x0118B180) and what Slots_PrepareB
// fills it from (name?).
constexpr u32 kSlotJobParams = 0x0118B180;
constexpr u32 kSjFrameWord = 0x010DDA78;       // u32 (name?)
constexpr u32 kSjRender = 0x010DD780;          // +256.. a 3x3 matrix, +612 f32 (name?)
constexpr u32 kSjSrc11945A0 = 0x011945A0;      // u32, +4 u16, +16 / +272 handed on by address (name?)
constexpr u32 kSjSrc10C5A20 = 0x010C5A20;      // +96..108 u32, +128 / +132 f32, +153 u8 (name?)
constexpr u32 kViewPlanes = 272;               // View: 6 f32[4] planes
constexpr u32 kViewMatrices = 368;             // View: 2 f32[16]
constexpr u32 kViewPos864 = 864;               // View: f32[3] (name?)
constexpr u32 kSjRing = 0x0118B100;            // u32: the ring cursor, reset to 0x01183100 (name?)
constexpr u32 kSjRingBase = 0x01183100;
constexpr u32 kSjBufA = 0x0118C000, kSjBufB = 0x0118E000;  // (name?)
constexpr u32 kSjCounter = 0x0118BF80;         // u32 zeroed atomically (name?)
constexpr u32 kSjRingSize = 2048;
constexpr u32 View_GetCurrent = 0x004FDBA8;     // () -> View*

void CopyWords(u32 dst, u32 src, u32 n) {
    for (u32 k = 0; k < n; ++k) store<u32>(dst + 4 * k, load<u32>(src + 4 * k));
}

}  // namespace

// void Slots_PrepareB() (name?)
// Fills the slot job's parameter block (0x0118B180): the view's 6 planes
// and 2 matrices, the render matrix rows, the bank and scene values the
// job reads, the view position, and resets its ring (0x0118B100 =
// 0x01183100, 2048), its buffers and the atomic counter 0x0118BF80.
DECOMP(0058B128, Slots_PrepareB) {
    const u32 G = kSlotJobParams;
    store<u32>(G + 2992, load<u32>(kSjFrameWord));
    const u32 view = static_cast<u32>(call(ctx, View_GetCurrent));
    CopyWords(G, view + kViewPlanes, 24);      // lfs / stfs pairs
    CopyWords(G + 96, view + kViewMatrices, 32);
    const u32 R = kSjRender;
    for (u32 r = 0; r < 4; ++r) CopyWords(G + 2912 + 16 * r, R + 256 + 16 * r, 3);
    const u32 bg = obj15::kBankGlobals, s1 = kSjSrc11945A0, s2 = kSjSrc10C5A20;
    store<u32>(G + 2996, load<u32>(load<u32>(bg + 36) + 20));
    store<u32>(G + 3000, load<u32>(R + 612));
    store<u32>(G + 3004, load<u32>(s1));
    store<u32>(G + 3008, load<u16>(s1 + 4));
    store<u32>(G + 3012, load<u32>(s2 + 100));
    store<u32>(G + 3016, load<u32>(s2 + 104));
    store<u32>(G + 3020, load<u32>(s2 + 108));
    store<u8>(G + 3092, load<u8>(s2 + 153));
    store<u32>(G + 3024, load<u32>(s2 + 96));
    const u32 view2 = static_cast<u32>(call(ctx, View_GetCurrent));
    CopyWords(G + 2976, view2 + kViewPos864, 3);
    store<u32>(G + 3028, load<u32>(s2 + 128));
    store<u32>(G + 3036, s1 + 16);
    store<u32>(G + 3032, load<u32>(s2 + 132));
    store<u32>(G + 3088, s1 + 272);
    CopyWords(G + 3040, bg + 5264, 3);
    CopyWords(G + 3056, bg + 5280, 3);
    const u32 b3093 = load<u8>(G + 3093);
    CopyWords(G + 3072, bg + 5312, 2);
    store<u8>(G + 3093, static_cast<u8>(b3093 & 0x7F));
    store<u32>(G + 3080, load<u32>(bg + 5320));
    store<u32>(G + 3096, kSjRing);
    store<u32>(G + 3100, kSjRing);
    store<u32>(G + 3104, kSjRingSize);
    store<u32>(G + 3216, kSjBufB);
    store<u32>(G + 3220, kSjBufA);
    store<u32>(G + 3224, kSjBufB);
    store<u32>(kSjBufB, kSjBufA);
    store<u32>(kSjRing, kSjRingBase);
    store<u32>(G + 3212, kSjCounter);
    for (;;) {  // lwarx / stwcx.: *counter = 0
        const u32 old = load<u32>(kSjCounter);
        if (mem::cas32(kSjCounter, old, 0)) break;
    }
    store<u32>(G + 3208, 0);
}

namespace {

constexpr u32 Box_SetPoint = 0x00556F60;        // (f32 box[8], const f32 p[3]): min = max = p
constexpr u32 Mem_FreeFrom16 = 0x0054EE6C;      // (p, pool)
constexpr u32 kZeroVec3 = 0x01164B40;           // f32[3] 0
constexpr u32 kDefaultSphere = 0x01164BC0;      // f32[4]
constexpr u32 kDefaultPointDef = 0x00F88A00;    // u32 ApePointDef*: made on first use (name?)
constexpr u32 kPointDefList = 0x00F8F13C;       // ApeTListAllocator<ApePointDef> header*
constexpr u32 kVtblPointDefFL = 0x00B24E98;
constexpr u32 kStrPointDefList = 0x00AF4AE0;    // "ApeTFreeList<T> &ApeTListAllocator<T>::GetAllocator() [with T = ApePointDef]"
constexpr u32 kPointDefSize = 112;
constexpr u32 kVtblDefBase = 0x00B246B0;        // the definition base vtable
constexpr u32 kVtblPointDef = 0x00B24808;
constexpr u32 kObjIdCounter = 0x01164CD0;       // u32: the last object id (name?)

// The definition bounds reset both constructors inline (as obj15's
// DefResetBounds): the box to the origin point, the default sphere, +64 /
// +96..104 zeroed, the shared block (+108) released (its data freed with it
// at the last reference).
void ResetDefBounds(PPUContext& ctx, u32 d) {
    call(ctx, Box_SetPoint, {d + 16, kZeroVec3});
    const u32 sh = load<u32>(d + 108);
    store<u32>(d + 48, load<u32>(kDefaultSphere));
    store<u32>(d + 52, load<u32>(kDefaultSphere + 4));
    store<u32>(d + 56, load<u32>(kDefaultSphere + 8));
    store<u32>(d + 96, 0);
    store<u32>(d + 60, load<u32>(kDefaultSphere + 12));
    store<u32>(d + 100, 0);
    store<u8>(d + 64, 0);
    store<u32>(d + 104, 0);
    if (sh == 0) return;
    store<u32>(d + 108, 0);
    const u16 refs = static_cast<u16>(load<u16>(sh) - 1);
    store<u16>(sh, refs);
    if (static_cast<s16>(refs) > 0) return;
    const u32 data = load<u32>(sh + 4);
    if (data != 0) {
        call(ctx, Mem_FreeFrom16, {data - 16, 0});
        store<u32>(sh + 4, 0);
    }
    call(ctx, Mem_FreeFrom16, {sh, 0});
}

}  // namespace

// ApePointDef* PointDef_GetDefault(): the default point definition, made
// on first use from the ApePointDef free list (the base and point
// constructors inlined: refs 1, kind 24, the next object id, empty box,
// the bounds reset twice) and cached; 0 (cached too) when the list is out.
DECOMP(0059E9B0, PointDef_GetDefault) {
    u32 d = load<u32>(kDefaultPointDef);
    if (d != 0) {
        ret(ctx, d);
        return;
    }
    const u32 alloc = obj6::ListAllocator(ctx, kPointDefList, kVtblPointDefFL, kStrPointDefList, kPointDefSize);
    const u32 opd = load<u32>(load<u32>(alloc) + kVfnListAlloc);
    const u32 p = static_cast<u32>(call_ptr(ctx, opd, {alloc, opd, load<u32>(opd)}));
    if (p != 0) {
        const u32 w4 = load<u32>(p + 4);
        store<u16>(p + 4, 1);
        store<u32>(p + 4, (1u << 16) | (24u << 10) | (w4 & 0x3FF));
        const u32 id = load<u32>(kObjIdCounter) + 1;
        store<u32>(kObjIdCounter, id);
        const u32 w8 = (load<u32>(p + 8) & 0x3F) | (id << 11);
        store<u32>(p, kVtblDefBase);
        for (u32 k = 0; k < 3; ++k) store<u32>(p + 16 + 4 * k, 0x7F7FFFFFu);  // FLT_MAX
        for (u32 k = 0; k < 3; ++k) store<u32>(p + 32 + 4 * k, 0xFF7FFFFFu);  // -FLT_MAX
        store<u32>(p + 108, 0);
        store<u32>(p + 12, 0);
        store<u32>(p + 8, w8);
        ResetDefBounds(ctx, p);
        store<u32>(p, kVtblPointDef);
        ResetDefBounds(ctx, p);
        d = p;
    }
    store<u32>(kDefaultPointDef, d);
    ret(ctx, d);
}

namespace {

constexpr u32 Buffer_Init = 0x005DF960;        // (CellGcmContextData* b, data, size, callback)
constexpr u32 SetWord = 0x0052F778;            // (u32* p, u32 v): *p = v
constexpr u32 Gcm_Fn5E3C00 = 0x005E3C00;       // (context, program +20): libgcm (name?)
constexpr u32 Gcm_Fn5E42A0 = 0x005E42A0;       // (context, u32, 1): libgcm (name?)
constexpr u32 kGcmState = 0x010F10A0;          // render state: +0 the context, +8, +20 / +24 / +28 the bound program words, +92 dirty bits
constexpr u32 kGcmCallback = 0x00F3BCF0;       // the context's overflow callback OPD
constexpr u32 kPrebuiltCmds = 0x01195900;      // 2 x 64-byte command buffers (name?)
constexpr u32 kPrebuiltLens = 0x01195980;      // u16[2]: their lengths (name?)
constexpr u32 kPassTable = 0x00F8896C;         // u32 obj*: +16 -> +12 the entries (name?)

}  // namespace

// void Gcm_PrebuildPassCommands() (name?)
// Records two 64-byte command buffers (0x01195900 / 0x01195940) with the
// render state's context switched to them: per pass entry (the table at
// *0x00F8896C +16 +12) its program words are bound when they changed
// (+20 / +24 through Gcm_Fn5E3C00, +28 marking +92 bit 31) and
// Gcm_Fn5E42A0(context, +28 word + 1136, 1) written; the lengths go to
// 0x01195980. The context is restored after.
DECOMP(00586BF4, Gcm_PrebuildPassCommands) {
    const u32 st = kGcmState;
    const u32 saved = load<u32>(st);
    Stack frame(ctx, 16);  // the original's sp+112: {begin, end, current, callback}
    const u32 b = frame.addr();
    for (u32 i = 0; i < 2; ++i) {
        call(ctx, Buffer_Init, {b, kPrebuiltCmds + 64 * i, 64, kGcmCallback});
        call(ctx, SetWord, {st, b});
        const u32 e = load<u32>(load<u32>(load<u32>(load<u32>(kPassTable) + 16) + 12) + 4 * i);
        const u32 prog = load<u32>(e + 4);
        if (load<u32>(st + 20) != prog) {
            store<u32>(st + 20, prog);
            store<s32>(st + 8, -1);
            const u32 w = load<u32>(prog + 4);
            if (load<u32>(st + 24) != w) {
                store<u32>(st + 24, w);
                call(ctx, Gcm_Fn5E3C00, {load<u32>(st), load<u32>(prog + 20)});
            }
        }
        u32 w28 = load<u32>(st + 28);
        const u32 want = load<u32>(e + 8);
        if (w28 != want) {
            store<u32>(st + 28, want);
            store<u32>(st + 92, load<u32>(st + 92) | 0x80000000u);
            w28 = want;
        }
        call(ctx, Gcm_Fn5E42A0, {load<u32>(st), w28 + 1136, 1});
        store<u16>(kPrebuiltLens + 2 * i, static_cast<u16>(load<u32>(b + 8) - load<u32>(b)));
    }
    call(ctx, SetWord, {st, saved});
}

namespace {

constexpr u32 kPrimPoolNext = 0x01164E5C;       // u32 PrimPool*: the other prim pool (debug.cpp kPrimPoolUnk) (name?)
constexpr u32 Stream_PutByteCmd = 0x005234DC;   // (stream, u8)
constexpr u32 kDebugStream = 0x010DDBB4;        // u32 (name?)
constexpr u32 kViewHidden = 48;                 // View u8 (name?)
constexpr u32 kViewFlags256 = 256;              // View u32: 0x20 = debug prims shown (name?)
constexpr u32 kScene10C5A20 = 0x010C5A20;       // +34 u8 (name?)
constexpr u32 kMode10F0ED8 = 0x010F0ED8;        // u32 (0x010F0E20 +184): 1 with the above keeps type 3 only... (name?)
constexpr u32 kPrimType = 0;                    // Prim (item) u32: the kind

}  // namespace

// void Debug_RetirePrims() (f_00522140's) (name?)
// With the other prim pool present, the current view shown (+48 clear)
// and its +256 bit 0x20 set: every prim of that pool whose kind is not 7,
// 8, 9 or 3 becomes kind 9 (unless 0x010C5A20 +34 is set and 0x010F0ED8
// is 1); then Stream_PutByteCmd(*0x010DDBB4, 0).
DECOMP(0056AD18, Debug_RetirePrims) {
    if (load<u32>(kPrimPoolNext) == 0) return;
    const u32 view = static_cast<u32>(call(ctx, View_GetCurrent));
    if (load<u8>(view + kViewHidden) != 0 || (load<u32>(view + kViewFlags256) & 0x20) == 0) return;
    const bool all = !(load<u8>(kScene10C5A20 + 34) != 0 && load<u32>(kMode10F0ED8) == 1);
    const u32 alloc = PrimAllocator(ctx, kPrimPoolNext);
    const u32 head = load<u32>(alloc + kFlLive);
    u32 item = head != 0 ? head + 16 : 0;
    while (item != 0) {
        const u32 next_node = load<u32>(item - 16);
        const u32 next = next_node != 0 ? next_node + 16 : 0;
        const s32 t = load<s32>(item + kPrimType);
        if (!(t == 9 || t == 8 || t == 7) && !(t == 3 || !all)) store<u32>(item + kPrimType, 9);
        item = next;
    }
    call(ctx, Stream_PutByteCmd, {load<u32>(kDebugStream), 0});
}

namespace {

constexpr u32 Fn_59A84C = 0x0059A84C;          // (u32* items, u32 count) -> u32 (name?)
constexpr u32 Fn_572D20 = 0x00572D20;          // (obj, -, out +80, -; f1) (name?)
constexpr u32 WorkQueue_SubmitF2C8 = 0x0056F2C8;  // (Worker* w)
constexpr u32 memmove16 = 0x0071C3F0;
constexpr u32 kRenderMatrix64 = 0x010DD7C0;    // f32[16] (0x010DD780 +64) (name?)
constexpr u32 kFrame10F0E08 = 0x010F0E08;      // u32 (name?)
constexpr u32 kJobRing = 0x01195880;           // (name?)
constexpr u32 kMaxJobItems = 512;
// The job owner: +0 u8 bit 0 enabled, +128 f32, +132 u32* the items, +136
// u32** the end pointer; the worker: +16 the job block, +544 the owner (name?).
constexpr u32 kJoFlags = 0, kJoF128 = 128, kJoItems = 132, kJoEnd = 136, kWkJob = 16, kWkOwner = 544;

}  // namespace

// bool Jobs_SubmitF2C8(Owner* o, Worker* w) (name?)
// With o enabled and items pending (at most 512): the worker's job block
// (+16) gets the render matrix, Fn_59A84C(items, n), the frame word, the
// two prebuilt command buffers and their lengths, Fn_572D20's output (o;
// f1 = o +128), o +128, the items, n, the ring 0x01195880 and the worker;
// the worker's +0 / +4 zeroed, its owner set and WorkQueue_SubmitF2C8 run.
// Returns 0 when not submitted.
DECOMP(0059AB34, Jobs_SubmitF2C8) {
    const u32 o = arg<u32>(ctx, 0), w = arg<u32>(ctx, 1);
    const u32 f128 = load<u32>(o + kJoF128);
    if ((load<u8>(o + kJoFlags) & 1) == 0) {
        ret(ctx, 0u);
        return;
    }
    const u32 items = load<u32>(o + kJoItems);
    const u32 n = (load<u32>(load<u32>(o + kJoEnd)) - items) >> 2;
    const u32 cnt = n < kMaxJobItems ? n : kMaxJobItems;
    if (cnt == 0) {
        ret(ctx, 0u);
        return;
    }
    const u32 J = w + kWkJob;
    for (u32 k = 0; k < 16; ++k) store<u32>(J + 4 * k, load<u32>(kRenderMatrix64 + 4 * k));  // lfs / stfs
    store<u32>(J + 64, static_cast<u32>(call(ctx, Fn_59A84C, {items, cnt})));
    store<u32>(J + 520, load<u32>(kFrame10F0E08));
    call(ctx, memmove16, {J + 388, kPrebuiltCmds, 128});
    const u32 len0 = load<u16>(kPrebuiltLens), len1 = load<u16>(kPrebuiltLens + 2);
    store<u16>(J + 518, static_cast<u16>(len1));
    store<u16>(J + 516, static_cast<u16>(len0));
    ctx.fpr[1] = std::bit_cast<f32>(f128);
    // r4 / r6..r9 as the original leaves them (the callee reads r3, r5, f1).
    call(ctx, Fn_572D20, {o, len0, J + 80, J + 516, 22912, J + 80, len1});
    store<u32>(J + 368, f128);
    store<u32>(J + 384, w);
    store<u32>(J + 372, load<u32>(o + kJoItems));
    store<u32>(J + 376, cnt);
    store<u32>(J + 380, kJobRing);
    store<u32>(w, 0);
    store<u32>(load<u32>(J + 384) + 4, 0);
    store<u32>(w + kWkOwner, o);
    call(ctx, WorkQueue_SubmitF2C8, {J});
    ret(ctx, 1u);
}

namespace {

constexpr u32 Box_Center = 0x00556F98;     // (box, out) -> out
constexpr u32 Box_Offset = 0x005572E8;     // (out, box, const f32 v[3]): out = box + v
constexpr u32 Box_Transform = 0x005570F0;  // (out, in, matrix)
constexpr u32 Quat_ToMatrix = 0x00553A90;  // (f32 out[4][4], const f32 q[4], const f32 pos[3])
constexpr u32 Obb_FromBox = 0x00576BD8;    // (out, const Box* local, const f32 m[4][4]) (name?)
constexpr u32 kIdentityQuat = 0x01164C90;  // f32[4] {0, 0, 0, 1}
constexpr u32 kQuatEps = 0x00576F24;       // f32 1e-5

}  // namespace

// void Volume_SetFromBox(Volume* v, const Box* b, const f32 q[4], const f32 off[3]) (name?)
// The box re-centred on the origin (Box_Offset by -centre, into an empty
// box first), rotated by q about centre + off (Quat_ToMatrix) into v +16
// (Box_Transform); unless q is the identity (each lane within 1e-5) v +2
// bit 0 is set and the oriented box built at v +48 (Obb_FromBox).
DECOMP(00576F28, Volume_SetFromBox) {
    const u32 v = arg<u32>(ctx, 0), b = arg<u32>(ctx, 1), q = arg<u32>(ctx, 2), off = arg<u32>(ctx, 3);
    Stack frame(ctx, 160);  // the original's sp+112 .. sp+256
    const u32 pos = frame.addr(), negc = pos + 16, centre = pos + 32, box = pos + 48, m = pos + 80;
    const u32 c = static_cast<u32>(call(ctx, Box_Center, {b, centre}));
    const f32 cx = load<f32>(c), cy = load<f32>(c + 4), cz = load<f32>(c + 8);
    const u32 cbits[3] = {load<u32>(c), load<u32>(c + 4), load<u32>(c + 8)};
    store<f32>(pos, load<f32>(off) + cx);
    store<f32>(pos + 4, load<f32>(off + 4) + cy);
    store<f32>(pos + 8, load<f32>(off + 8) + cz);
    call(ctx, Quat_ToMatrix, {m, q, pos});
    for (u32 k = 0; k < 3; ++k) store<u32>(box + 4 * k, 0x7F7FFFFFu);       // FLT_MAX
    for (u32 k = 0; k < 3; ++k) store<u32>(box + 16 + 4 * k, 0xFF7FFFFFu);  // -FLT_MAX
    for (u32 k = 0; k < 3; ++k) store<u32>(negc + 4 * k, cbits[k] ^ 0x80000000u);  // fneg
    call(ctx, Box_Offset, {box, b, negc});
    call(ctx, Box_Transform, {v + 16, box, m});
    const f32 eps = load<f32>(kQuatEps);
    bool identity = true;
    for (u32 k = 0; k < 4 && identity; ++k)
        identity = std::fabs(load<f32>(q + 4 * k) - load<f32>(kIdentityQuat + 4 * k)) < eps;
    if (identity) return;
    store<u8>(v + 2, static_cast<u8>(load<u8>(v + 2) | 1));
    call(ctx, Obb_FromBox, {v + 48, box, m});
}

namespace {

constexpr u32 LoadShaders = 0x00523AA8;     // (s32 which) -> the shader group's texture (name?)
constexpr u32 Draw_SetTexture = 0x00523310; // (fb, tex)
constexpr u32 Draw_SetState = 0x005218D8;   // (fb, const f32 m[16])
constexpr u32 Draw_BeginPrims = 0x005236A0; // (Prims* out, fb, u8 type, u16 count, r7 extra)
constexpr u32 Draw_EndPrims = 0x00523748;   // (fb)
constexpr u32 kIdentityMatrix = 0x01164BD0; // f32[16]
constexpr u32 kPrimLines = 2;               // Draw_BeginPrims type (name?)
constexpr u32 kVertexSize = 24;             // {f32 x, y, z; u8 rgba[4]; f32 u, v}
// Draw buffer (FrameBuffer): +0 the start, +4 the end, +12 the GPU's read
// position, +16 the cursor, +52 the vertex capacity, +72 the vertex count.
constexpr u32 kFbStart = 0, kFbEnd = 4, kFbRead = 12, kFbCursor = 16, kFbVertCap = 52, kFbVertCount = 72;

// The room check the debug draws inline: 2 more vertices fit and 259
// bytes of commands fit between the cursor and the GPU's read position.
bool DrawRoom(u32 fb) {
    const bool verts = load<u32>(fb + kFbVertCount) + 2 < load<u32>(fb + kFbVertCap);
    const u32 cur = load<u32>(fb + kFbCursor), rd = load<u32>(fb + kFbRead);
    bool wrap_ok = true;
    if (cur + 259 > load<u32>(fb + kFbEnd)) wrap_ok = !(rd > cur) && (rd != load<u32>(fb + kFbStart));
    const u32 a = (cur + 3) & ~3u;
    const bool ahead_ok = !(a < rd) || (a + 256 < rd);
    return verts && wrap_ok && ahead_ok;
}

}  // namespace

// void Debug_DrawLine(const Line* l) (f_0056AEFC's) (name?)
// When the debug draw buffer (*0x010DDBB4) has room: the debug shader
// group's texture, the identity state and a 2-vertex line from (l +12,
// l +16) to (l +20, l +24) at z = -(l +28) in colour l +4..7. Draw_BeginPrims
// gets the r7 Draw_SetState leaves, as in the original.
DECOMP(00569CDC, Debug_DrawLine) {
    const u32 l = arg<u32>(ctx, 0);
    const u32 fb = load<u32>(kDebugStream);
    if (!DrawRoom(fb)) return;
    const u32 tex = static_cast<u32>(call(ctx, LoadShaders, {0}));
    call(ctx, Draw_SetTexture, {fb, tex});
    call(ctx, Draw_SetState, {fb, kIdentityMatrix});
    Stack frame(ctx, 32);  // the original's sp+112 (prims) / sp+124 (the points)
    const u32 prims = frame.addr(), pts = prims + 12;
    for (u32 k = 0; k < 4; ++k) store<u32>(pts + 4 * k, load<u32>(l + 12 + 4 * k));  // lfs / stfs
    const u32 negz = load<u32>(l + 28) ^ 0x80000000u;  // fneg
    call(ctx, Draw_BeginPrims, {prims, fb, kPrimLines, 2});
    u32 v = load<u32>(prims);
    const u32 index = load<u16>(prims + 6);
    const u8 c0 = load<u8>(l + 4), c1 = load<u8>(l + 5), c2 = load<u8>(l + 6), c3 = load<u8>(l + 7);
    for (u32 k = 0; k < 2; ++k) {
        store<u32>(v, load<u32>(pts + 8 * k));
        store<u32>(v + 4, load<u32>(pts + 8 * k + 4));
        store<u32>(v + 8, negz);
        store<u8>(v + 12, c0);
        store<u8>(v + 13, c1);
        store<u8>(v + 14, c2);
        store<u8>(v + 15, c3);
        store<u32>(v + 16, 0);
        store<u32>(v + 20, 0);
        v += kVertexSize;
    }
    store<u16>(prims + 6, static_cast<u16>(index + 2));
    store<u32>(prims, v);
    ctx.fpr[1] = std::bit_cast<f32>(negz);
    call(ctx, Draw_EndPrims, {fb});
}

namespace {

// sys_lwmutex_attribute_t {u32 protocol; u32 recursive; char name[8]}.
constexpr u32 SYS_SYNC_PRIORITY = 2, SYS_SYNC_RECURSIVE = 0x20;
constexpr u32 kSysutilWrapperName = 0x0116B280;   // const char*: "cellSysutilCheckCallback() wrapper"
constexpr u32 kStrSysutilWrapper = 0x00AF4894;
constexpr u32 kSysutilBlock = 0x0116B328;         // 12 words zeroed (obj14 kStatic5721C4C) (name?)
constexpr u32 kJobDispatcher = 0x0116B580;        // ApePs3JobDispatcher: lwmutex at +0, two queues at +128 / +896 (name?)
constexpr u32 kStrSpursJob = 0x00AF48BC;          // "SpursJob"
constexpr u32 kVtblJobQueue = 0x00B24478;         // (name?)
constexpr u32 kStrSubmitCommand = 0x00B24490;     // "ApePs3JobDispatcher::SubmitCommand"
constexpr u32 kDispatchCounter = 0x0116BB80;      // u32 zeroed atomically (name?)

void LwmutexCreate(PPUContext& ctx, u32 mutex, u32 attr) {
    store<u32>(attr, SYS_SYNC_PRIORITY);
    store<u32>(attr + 4, SYS_SYNC_RECURSIVE);
    store<u8>(attr + 8, 0);
    call(ctx, imp::sys_lwmutex_create, {mutex, attr});
}

}  // namespace

// void Sysutil_StaticInit() (a static initializer; Sysutil_StaticDestroy's
// counterpart) (name?)
// The sysutil wrapper's name and recursive priority mutex, the job
// dispatcher's mutex ("SpursJob") and its two command queues (each with a
// mutex, cleared fields, +664 / +1432 set and the "SubmitCommand" name),
// and the dispatch counter zeroed atomically.
DECOMP(00572228, Sysutil_StaticInit) {
    Stack frame(ctx, 64);  // the original's sp+112 / +128 / +144 / +160 attributes
    const u32 a0 = frame.addr(), a1 = a0 + 16, a2 = a0 + 32, a3 = a0 + 48;
    store<u32>(kSysutilWrapperName, kStrSysutilWrapper);
    LwmutexCreate(ctx, kSysutilMutex, a3);
    for (u32 k = 0; k < 12; ++k) store<u32>(kSysutilBlock + 4 * k, 0);
    const u32 d = kJobDispatcher;
    store<u32>(d + 24, kStrSpursJob);
    LwmutexCreate(ctx, d, a2);
    store<u32>(d + 640, 0);
    store<u32>(d + 644, 0);
    store<u32>(d + 128, kVtblJobQueue);
    for (u32 off : {648u, 652u, 656u, 660u}) store<u32>(d + off, 0);
    store<u8>(d + 664, 1);
    store<u32>(d + 696, kStrSubmitCommand);
    LwmutexCreate(ctx, d + 672, a1);
    for (u32 off = 768; off <= 880; off += 16) {  // stvx of a zero vector
        store<u64>(d + off, 0);
        store<u64>(d + off + 8, 0);
    }
    store<u32>(d + 896, kVtblJobQueue);
    for (u32 off = 1408; off <= 1428; off += 4) store<u32>(d + off, 0);
    store<u8>(d + 1432, 1);
    store<u32>(d + 1464, kStrSubmitCommand);
    LwmutexCreate(ctx, d + 1440, a0);
    for (;;) {  // lwarx / stwcx.: *counter = 0
        const u32 old = load<u32>(kDispatchCounter);
        if (mem::cas32(kDispatchCounter, old, 0)) break;
    }
}
