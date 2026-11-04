// World objects, part 17: the definition lookup by kind, the OBB helpers
// (Obb_FromBox, Sphere_FrustumFine) and the job item matrices.
#include "obj17.h"

#include "../memory/mem.h"

using namespace sly4;
using namespace sly4::obj17;

namespace {

using obj8::VAdd;
using obj8::VLoad;
using obj8::VMadd;
using obj8::VSplat;
using obj8::VZero;

// *find(holder->map, &hash): the value the definition map has for hash
// (the key goes through a stack copy).
u32 FindDef(PPUContext& ctx, u32 holder, u32 find, u32 hash) {
    Stack frame(ctx, 16);
    const u32 key = frame.addr();
    store<u32>(key, hash);
    const u32 map = load<u32>(load<u32>(holder) + kDefHolderMap);
    return load<u32>(static_cast<u32>(call(ctx, find, {map, key})));
}

Vec4 VAbs(const Vec4& v) {  // vandc with the sign mask
    Vec4 r;
    for (u32 i = 0; i < 4; ++i) r.w[i] = v.w[i] & 0x7FFFFFFFu;
    return r;
}

Vec4 VScalar(u32 ea) { return VSplat(Vec4{{load<u32>(ea), 0, 0, 0}}, 0); }  // lvlx + vspltw 0

bool AllGe0(const Vec4& v) {  // vcmpgefp. against 0: every lane >= 0
    for (u32 i = 0; i < 4; ++i)
        if (!(v.f(i) >= 0.0f)) return false;
    return true;
}

}  // namespace

// ApeDef* Def_FindByHash(u32 hash, s32 kind): the definition of that hash
// in the kind's map (8 / 20 geometry, 9 / 21 light, 10 / 22 emitter, 11 /
// 23 camera, 12 / 24 point, 13 / 25 spline); any other kind tries the
// geometry, light and emitter maps in turn. A zero hash gives 0.
DECOMP(0057CE08, Def_FindByHash57CE08) {
    const u32 hash = arg<u32>(ctx, 0);
    const s32 kind = arg<s32>(ctx, 1);
    if (hash == 0) {
        ret(ctx, 0u);
        return;
    }
    u32 r;
    switch (kind) {
        case 8: case 20: r = FindDef(ctx, kGeomDefHolder, o17fn::HashMap_Find2, hash); break;
        case 9: case 21: r = FindDef(ctx, kLightDefHolder, o17fn::LightDefMap_Find, hash); break;
        case 10: case 22: r = FindDef(ctx, kEmitterDefHolder, o17fn::MapE_Find, hash); break;
        case 11: case 23: r = FindDef(ctx, kCameraDefHolder, o17fn::CameraDefMap_Find, hash); break;
        case 12: case 24: r = FindDef(ctx, kPointDefHolder, o17fn::PointDefMap_Find, hash); break;
        case 13: case 25: r = FindDef(ctx, kSplineDefHolder, o17fn::SplineDefMap_Find, hash); break;
        default:
            r = FindDef(ctx, kGeomDefHolder, o17fn::HashMap_Find2, hash);
            if (r == 0) r = FindDef(ctx, kLightDefHolder, o17fn::LightDefMap_Find, hash);
            if (r == 0) r = FindDef(ctx, kEmitterDefHolder, o17fn::MapE_Find, hash);
            break;
    }
    ret(ctx, r);
}

namespace {
constexpr u32 kObbOne = 0x00576BD4;        // f32 1.0
constexpr u32 kBoxFaces = 0x00F8897C;      // u8[6][4]: three corner indices per face (name?)
constexpr u32 kObbFaces = 6;
}  // namespace

// void Obb_FromBox(f32 out[6][4], const Box* b, const f32 m[4][4]): the
// six face planes {n, -n.a} of the box transformed by m. The corners are
// box corner i (bit 0 min x, bit 1 min y, bit 2 min z) times m (vmaddfp
// chains); each face's normal is the normalized cross product of two of
// its edges (corner indices from kBoxFaces).
DECOMP(00576BD8, Obb_FromBox) {
    const u32 out = arg<u32>(ctx, 0), b = arg<u32>(ctx, 1), m = arg<u32>(ctx, 2);
    const Vec4 m0 = VLoad(m), m1 = VLoad(m + 16), m2 = VLoad(m + 32), m3 = VLoad(m + 48);
    f32 corner[box::kBoxCorners][4];
    for (u32 i = 0; i < box::kBoxCorners; ++i) {
        const u32 x = load<u32>(b + ((i & 1) ? 0u : 16u));
        const u32 y = load<u32>(b + ((i & 2) ? 4u : 20u));
        const u32 z = load<u32>(b + ((i & 4) ? 8u : 24u));
        Vec4 v = VMadd(m0, VSplat(Vec4{{x, 0, 0, 0}}, 0), VZero());
        v = VMadd(m1, VSplat(Vec4{{y, 0, 0, 0}}, 0), v);
        v = VMadd(m2, VSplat(Vec4{{z, 0, 0, 0}}, 0), v);
        v = VAdd(v, m3);
        for (u32 k = 0; k < 4; ++k) corner[i][k] = v.f(k);
    }
    const f64 one = static_cast<f64>(load<f32>(kObbOne));
    for (u32 f = 0; f < kObbFaces; ++f) {
        const f32* a = corner[load<u8>(kBoxFaces + 4 * f)];
        const f32* p = corner[load<u8>(kBoxFaces + 4 * f + 1)];
        const f32* c = corner[load<u8>(kBoxFaces + 4 * f + 2)];
        auto sub = [](f32 x, f32 y) { return static_cast<f32>(static_cast<f64>(x) - y); };
        auto mul = [](f32 x, f32 y) { return static_cast<f32>(static_cast<f64>(x) * y); };
        auto msub = [](f32 x, f32 y, f32 z) { return static_cast<f32>(std::fma(static_cast<f64>(x), y, -static_cast<f64>(z))); };
        const f32 dx1 = sub(p[0], c[0]), dz0 = sub(a[2], p[2]), dz1 = sub(p[2], c[2]);
        const f32 dx0 = sub(a[0], p[0]), dy0 = sub(a[1], p[1]), dy1 = sub(p[1], c[1]);
        const f32 ny = msub(dz1, dx0, mul(dx1, dz0));
        const f32 nx = msub(dy1, dz0, mul(dz1, dy0));
        const f32 nz = msub(dx1, dy0, mul(dy1, dx0));
        f32 len = mul(ny, ny);
        len = static_cast<f32>(std::fma(static_cast<f64>(nx), nx, static_cast<f64>(len)));
        len = static_cast<f32>(std::fma(static_cast<f64>(nz), nz, static_cast<f64>(len)));
        len = static_cast<f32>(std::sqrt(static_cast<f64>(len)));
        const f32 s = static_cast<f32>(one / len);
        const f32 sy = mul(s, ny), sx = mul(s, nx), sz = mul(s, nz);
        const u32 o = out + 16 * f;
        store<f32>(o + 4, sy);
        store<f32>(o + 0, sx);
        store<f32>(o + 8, sz);
        f32 d = mul(sy, a[1]);
        d = static_cast<f32>(std::fma(static_cast<f64>(sx), a[0], static_cast<f64>(d)));
        d = -static_cast<f32>(std::fma(static_cast<f64>(sz), a[2], static_cast<f64>(d)));  // fnmadds
        store<f32>(o + 12, d);
    }
    ctx.gpr[3] = out + 16 * kObbFaces;  // left advanced past the planes, as the original
}

// bool Sphere_FrustumFine(View* v, const f32 c[3], const f32 e[3], const
// f32 m[3][4]): whether the box with centre c, half extents e and axes m
// (rows) is on the inner side of all eight planes the view keeps as two
// SoA blocks (+96 / +160: rows nx, ny, nz, d). Each plane's distance to c
// plus the box's projected radius sum |n.axis_i| e_i must be >= 0.
DECOMP(007A6EFC, Sphere_FrustumFine) {
    const u32 v = arg<u32>(ctx, 0), c = arg<u32>(ctx, 1), e = arg<u32>(ctx, 2), m = arg<u32>(ctx, 3);
    const Vec4 a0 = VLoad(v + 96), a1 = VLoad(v + 112), a2 = VLoad(v + 128), a3 = VLoad(v + 144);
    const Vec4 b0 = VLoad(v + 160), b1 = VLoad(v + 176), b2 = VLoad(v + 192), b3 = VLoad(v + 208);
    Vec4 ms[9];
    for (u32 r = 0; r < 3; ++r)
        for (u32 k = 0; k < 3; ++k) ms[3 * r + k] = VScalar(m + 16 * r + 4 * k);
    const Vec4 c0 = VScalar(c), c1 = VScalar(c + 4), c2 = VScalar(c + 8);
    const Vec4 e0 = VScalar(e), e1 = VScalar(e + 4), e2 = VScalar(e + 8);
    const Vec4 z = VZero();
    // n . axis for axis row r of m, in the original's order (from 0).
    auto dot = [&](const Vec4& r0, const Vec4& r1, const Vec4& r2, u32 r) {
        return VMadd(r2, ms[3 * r + 2], VMadd(r1, ms[3 * r + 1], VMadd(r0, ms[3 * r], z)));
    };
    // radius = |d0| e0 + |d1| e1 + |d2| e2; side = (r0 c0 + r3) + r1 c1 + r2 c2.
    auto test = [&](const Vec4& r0, const Vec4& r1, const Vec4& r2, const Vec4& r3) {
        Vec4 rad = VMadd(VAbs(dot(r0, r1, r2, 0)), e0, z);
        rad = VMadd(VAbs(dot(r0, r1, r2, 1)), e1, rad);
        rad = VMadd(VAbs(dot(r0, r1, r2, 2)), e2, rad);
        const Vec4 side = VMadd(r2, c2, VMadd(r1, c1, VMadd(r0, c0, r3)));
        return VAdd(side, rad);
    };
    const Vec4 ta = test(a0, a1, a2, a3);
    const Vec4 tb = test(b0, b1, b2, b3);
    ret(ctx, AllGe0(ta) && AllGe0(tb) ? 1u : 0u);
}

namespace {
constexpr u32 kIdentity = 0x01164C10;      // f32[16] (fx_dof kLastMatrix)
constexpr u32 kViewProj = 0x010DD7C0;      // f32[4][4] (layers4.h view_proj)
constexpr u32 kSlotRecords = 0x01193004;   // obj3.h kSlotTable: 128-byte records
constexpr u32 kObjSlot = 284;              // u16: slot record index, 0xFFFF = none (the matrix at +80)
constexpr u32 kObjMatrix80 = 80;
}  // namespace

// f32 (*Jobs_BuildItemMatrices(Item** items, u32 count))[16]: a frame
// buffer of count matrices, each item's ((*item)->object) world matrix
// (its slot record's, or +80 without one) times the view-projection
// rows; a null item or object gets the identity. As in the original the
// loop runs at least once, so count 0 still writes one matrix.
DECOMP(0059A84C, Jobs_BuildItemMatrices) {
    u32 it = arg<u32>(ctx, 0);
    const u32 count = arg<u32>(ctx, 1);
    const u32 buf = static_cast<u32>(call(ctx, o17fn::Render_FrameAlloc, {count << 6}));
    const u32 end = it + count * 4;
    u32 dst = buf;
    do {
        u32 mat[16];
        for (u32 k = 0; k < 16; ++k) mat[k] = load<u32>(kIdentity + 4 * k);
        const u32 item = load<u32>(it);
        it += 4;
        const u32 o = item != 0 ? load<u32>(item) : 0;
        if (o != 0) {
            const u32 slot = load<u16>(o + kObjSlot);
            const u32 src = slot == 0xFFFF ? o + kObjMatrix80 : load<u32>(kSlotRecords) + (slot << 7);
            const Vec4 c0 = VLoad(kViewProj), c1 = VLoad(kViewProj + 16), c2 = VLoad(kViewProj + 32), c3 = VLoad(kViewProj + 48);
            for (u32 r = 0; r < 4; ++r) {
                const Vec4 s = VLoad(src + 16 * r);
                Vec4 a = VMadd(c0, VSplat(s, 0), r == 3 ? c3 : VZero());
                a = VMadd(c1, VSplat(s, 1), a);
                a = VMadd(c2, VSplat(s, 2), a);
                for (u32 k = 0; k < 4; ++k) mat[4 * r + k] = a.w[k];
            }
        }
        for (u32 k = 0; k < 16; ++k) store<u32>(dst + 4 * k, mat[k]);  // lfs / stfs: raw
        dst += 64;
    } while (it < end);
    ret(ctx, buf);
}

namespace {
constexpr u32 kTickMask = 0x010DDA70;      // u8: the list kinds that tick (obj16 Lights_Tick2)
constexpr u32 kTickBusy = 0x0116BD80;      // u32: nonzero while the jobs run
constexpr u32 kTickJobs = 0x0116BD60, kTickJobCount = 0x0116BD64;
constexpr u32 kTickJobSize = 112;
constexpr u32 kTickBatch = 64;             // objects per job, extended to the next batch start
constexpr u32 kTickMaxA = 384, kTickMaxB = 512;
constexpr u32 kTickItemSize = 160, kTickItemFlags = 156;  // u8: 0x20 starts a batch (name?)
constexpr u32 kTickGroupSize = 32;
constexpr u32 kThreadViewPos = 864, kThreadView336 = 336;  // GetForThread's record: f32[3] / f32[4] (name?)
constexpr u32 kTickSlotTable = 0x01193004, kTickSlotLists = 0x01193010;
constexpr u32 kTickLightSrc = 0x01194580;  // +12 u32 (name?)
constexpr u32 kSsHeap = 28;                // system_state +28: the main heap (MemStack)
constexpr u32 GetForThread = 0x00522C98;
constexpr u32 MemStack_Alloc = 0x0055260C;
constexpr u32 WorkQueue_SubmitF158 = 0x0056F158;

// system_state::get as Lights_Tick inlines it (the guard set before the
// registry slot).
u32 StateForTick(PPUContext& ctx) {
    const u32 o = system_state::object;
    if (load<u8>(system_state::guard) == 0) {
        const u8 b = load<u8>(o + 32);
        store<u32>(o + 20, 0);
        store<u32>(o + 24, 0);
        store<u8>(o + 32, static_cast<u8>(b & ~0xC0));
        call(ctx, fn::memset, {o + 92, 0, 400});
        store<u8>(system_state::guard, 1);
        store<u32>(system_state::registry_slot, o);
    }
    return o;
}

u32 TickAlloc(PPUContext& ctx, u32 size) {
    const u32 st = StateForTick(ctx);
    return static_cast<u32>(call(ctx, MemStack_Alloc, {load<u32>(st + kSsHeap), size}));
}
}  // namespace

// void Lights_Tick(List* l; f32 dt in f1) (name?)
// For a list whose kind (+8) is enabled in 0x010DDA70: marks the list
// (+0 +14 = 1), clears the busy word (atomically), allocates the job
// records (112 bytes per 64 objects) and submits one job per batch: 64
// objects, extended up to the next object flagged as a batch start
// (+156 0x20). Each job gets three frame buffers (min(n, 384) words
// twice, min(n, 512) * 12 bytes), the thread view's position (+864) and
// +336 vec4, its objects / groups, n, dt, the busy word's address and the
// slot tables. As in the original a null job array still gets written
// (at address 0).
DECOMP(0059845C, Lights_Tick) {
    const u32 l = arg<u32>(ctx, 0);
    const f64 dt = ctx.fpr[1];
    const u32 sh = load<u8>(l + 8) & 63;  // slw
    const u32 bit = (sh < 32 ? (1u << sh) : 0u) & 0xFF;
    if ((bit & load<u8>(kTickMask)) == 0) {
        ctx.gpr[3] = 0;  // r3 as the original leaves it
        return;
    }
    store<u8>(load<u32>(l) + 14, 1);
    for (;;) {  // lwarx / stwcx.: busy = 0
        const u32 old = load<u32>(kTickBusy);
        if (mem::cas32(kTickBusy, old, 0)) break;
    }
    const u32 a = load<u32>(l);
    const u32 count = load<u16>(a + 8);
    const u32 items = load<u32>(a), groups = load<u32>(a + 4);
    const u32 tv = static_cast<u32>(call(ctx, GetForThread));
    const u32 pos = tv + kThreadViewPos, v336 = tv + kThreadView336;
    const u32 njobs = (count + 63) >> 6;
    const u32 jobs = TickAlloc(ctx, njobs * kTickJobSize);
    store<u32>(kTickJobs, jobs);
    store<u32>(kTickJobCount, 0);
    ctx.gpr[3] = 0x01170000u;  // r3 as the original leaves it (the lis of kTickJobs)
    if (count == 0) return;
    for (u32 start = 0; count > start;) {
        u32 end = start + kTickBatch < count ? start + kTickBatch : count;
        while (end < count && ((load<u8>(items + end * kTickItemSize + kTickItemFlags) >> 5) & 1) == 0) ++end;
        const u32 n = end - start;
        const u32 na = (n < kTickMaxA ? n : kTickMaxA) * 4;
        const u32 nb = n < kTickMaxB ? n : kTickMaxB;
        const u32 bufA = TickAlloc(ctx, na);
        const u32 bufB = TickAlloc(ctx, na);
        const u32 bufC = TickAlloc(ctx, nb * 12);
        const u32 jp = load<u32>(kTickJobCount) * kTickJobSize + jobs;
        u32 job = 0;
        if (jp != 0) {
            job = jp;
            store<u32>(job + 84, job);
        }
        store<u32>(job + 0, bufA);
        store<u32>(job + 4, bufB);
        store<u32>(job + 8, 0);
        store<u32>(job + 12, 0);
        store<u32>(job + 16, bufC);
        store<u32>(job + 20, 0);
        for (u32 k = 0; k < 3; ++k) store<u32>(job + 32 + 4 * k, load<u32>(pos + 4 * k));  // lfs / stfs: raw
        for (u32 k = 0; k < 3; ++k) store<u32>(job + 48 + 4 * k, load<u32>(v336 + 4 * k));
        const u32 w = load<u32>(v336 + 12);
        store<u32>(job + 64, start * kTickItemSize + items);
        store<u32>(job + 60, w);
        store<u32>(job + 68, start * kTickGroupSize + groups);
        store<u32>(job + 72, n);
        store<f32>(job + 76, static_cast<f32>(dt));
        store<u32>(job + 80, kTickBusy);
        store<u32>(job + 88, load<u32>(kTickSlotTable));
        const u32 ls = load<u32>(kTickLightSrc + 12);
        store<u32>(job + 96, kTickSlotLists);
        store<u32>(job + 92, ls);
        call(ctx, WorkQueue_SubmitF158, {job});
        start = end;
        store<u32>(kTickJobCount, load<u32>(kTickJobCount) + 1);
    }
    ctx.gpr[3] = 0x01170000u;
}

namespace {
constexpr u32 WorldObj_UpdateLod = 0x0057EE00;  // (obj, 0, 0)
constexpr u32 Obj_GetField4 = 0x00556BF0;       // (p) -> p->+4: a free list's first used node
constexpr u32 kListNode = 16;                   // a node's item follows its 16-byte header (+0 the next node)

struct InstList {
    u32 hdr, vtbl, name, size;
};
constexpr InstList kInstLists[] = {
    {0x00F8F144, 0x00B0F008, 0x00AF4B7E, 352},  // ApeGeomInstance
    {0x00F8F148, 0x00B13920, 0x00AF4BCF, 400},  // ApeLightInstance
    {0x00F8F14C, 0x00B10EF0, 0x00AF4C23, 352},  // ApeEmitterInstance
};
}  // namespace

// void WorldObj_FlushDirty(bool all) (name?)
// all == 0: drains the dirty list (obj.h kDirtyList) from the back, each
// object's LOD updated first unless it is queued elsewhere (+260), then
// its queued flag (0x400) cleared. Otherwise every geometry / light /
// emitter instance (their ApeTListAllocator lists, made on first use) that
// is dirty (0x800) and not queued gets its LOD updated.
DECOMP(00588FFC, WorldObj_FlushDirty) {
    const u32 all = arg<u32>(ctx, 0) & 0xFF;
    if (all == 0) {
        for (;;) {
            u32 v = load<u32>(obj::kDirtyList);
            const u32 n = load<u16>(v + 8);
            if (n == 0) {
                store<u16>(v + 8, 0);
                return;
            }
            const u32 o = load<u32>(load<u32>(v) + (n - 1) * 4);
            if (load<u32>(o + obj::kObjQueued) == 0) {
                call(ctx, WorldObj_UpdateLod, {o, 0, 0});
                v = load<u32>(obj::kDirtyList);
            }
            // The pop_back: the last index from the count re-read; when it
            // differs from the new count (the count was 0) the item at the
            // new count (0xFFFF) moves to that index (-1), as the original.
            const u32 c = load<u16>(v + 8);
            const u32 nc = (c - 1) & 0xFFFF;
            const s32 last = static_cast<s32>(c) - 1;
            store<u16>(v + 8, static_cast<u16>(nc));
            if (static_cast<u32>(last) != nc) {
                const u32 items = load<u32>(v);
                store<u32>(items + static_cast<u32>(last) * 4, load<u32>(items + nc * 4));
            }
            store<u32>(o + obj::kObjFlags, load<u32>(o + obj::kObjFlags) & ~obj::kObjQueuedFlag);
        }
    }
    for (const InstList& il : kInstLists) {
        const u32 alloc = obj6::ListAllocator(ctx, il.hdr, il.vtbl, il.name, il.size);
        const u32 first = static_cast<u32>(call(ctx, Obj_GetField4, {alloc}));
        u32 it = first != 0 ? first + kListNode : 0;
        while (it != 0) {
            const u32 o = it;
            const u32 next = load<u32>(o - kListNode);
            it = next != 0 ? next + kListNode : 0;
            if ((load<u32>(o + obj::kObjFlags) & obj::kObjDirty) != 0 && load<u32>(o + obj::kObjQueued) == 0)
                call(ctx, WorldObj_UpdateLod, {o, 0, 0});
        }
    }
}

namespace {
constexpr u32 Peer_GetMatrix = 0x0057C408;  // (peer, -, f32 out[4][4]; f1 t) -> nonzero: out is the full matrix
constexpr u32 kTrRecSize = 12;              // {obj*, f32 t, s32 sub-objects}
constexpr u32 kTrObjSize = 160;             // the sub-objects follow the object
constexpr u32 kTrDst = 140;                 // f32[4][4]*: where the matrix goes
constexpr u32 kTrCopy = 144;                // f32[4][4]*: a second copy (or 0)
constexpr u32 kTrId = 152;                  // u32: what goes on the output lists
constexpr u32 kTrFlags = 156;               // u8: 0x80 / 0x40 select the output list (name?)

// The object's matrix for t: with Peer_GetMatrix's full matrix its 3x4
// part is copied to obj +140 (w lanes kept); otherwise the result is a
// local transform, combined with the object's rows (+0..+48) into +140.
void TransformOne(PPUContext& ctx, u32 obj, f64 t, u32 r4) {
    Stack frame(ctx, 64);
    const u32 m = frame.addr();
    callf(ctx, Peer_GetMatrix, {obj, r4, m}, {t});
    const u32 rc = static_cast<u32>(ctx.gpr[3]);
    const u32 dst = load<u32>(obj + kTrDst);
    if (rc == 0) {
        Vec4 r[4], s[4], out[4];
        obj8::MatLoad(obj, r);
        obj8::MatLoad(m, s);
        obj8::MatCombine(r, s, out);
        obj8::MatStore(dst, out);
    } else {
        for (u32 row = 0; row < 4; ++row)
            for (u32 k = 0; k < 3; ++k) store<u32>(dst + 16 * row + 4 * k, load<u32>(m + 16 * row + 4 * k));  // raw
    }
}
}  // namespace

// void Objs_UpdateTransforms(List* l, Rec* recs, s32 n, u32* outA, u32*
// outB, u32* na, u32* nb) (name?)
// Lights_Tick2's per-job step: each record's object (and its rec +8
// sub-objects, 160 bytes apart) gets its matrix for the record's t
// (TransformOne). The object's id (+152) then goes on list B when its
// flags (+156) have 0x40, else on list A when they have 0x80; with
// neither, its +140 matrix (3x4) is copied to +144 when set. *na / *nb
// get the counts added.
DECOMP(00597C9C, Objs_UpdateTransforms) {
    u32 rec = arg<u32>(ctx, 1);
    const u32 n = arg<u32>(ctx, 2);
    const u32 outA0 = arg<u32>(ctx, 3), outB0 = arg<u32>(ctx, 4);
    const u32 pna = arg<u32>(ctx, 5), pnb = arg<u32>(ctx, 6);
    const u32 end = rec + n * kTrRecSize;
    u32 outA = outA0, outB = outB0;
    while (rec != end) {
        u32 obj = load<u32>(rec);
        // r4 is the dcbt offset the original leaves (Peer_GetMatrix doesn't read it).
        TransformOne(ctx, obj, static_cast<f64>(load<f32>(rec + 4)), 384);
        const u32 b = load<u8>(obj + kTrFlags);
        if ((b & 0x40) != 0) {
            store<u32>(outB, load<u32>(obj + kTrId));
            outB += 4;
        } else if ((b & 0x80) != 0) {
            store<u32>(outA, load<u32>(obj + kTrId));
            outA += 4;
        } else {
            const u32 p = load<u32>(obj + kTrCopy);
            if (p != 0) {
                const u32 src = load<u32>(obj + kTrDst);
                for (u32 row = 0; row < 4; ++row)
                    for (u32 k = 0; k < 3; ++k) store<u32>(p + 16 * row + 4 * k, load<u32>(src + 16 * row + 4 * k));  // raw
            }
        }
        const u32 subs = load<u32>(rec + 8);
        if (subs != 0) {
            u32 left = subs - 1;
            for (;;) {
                obj += kTrObjSize;
                TransformOne(ctx, obj, static_cast<f64>(load<f32>(rec + 4)), ctx.gpr[4]);
                if (left-- == 0) break;
            }
        }
        rec += kTrRecSize;
    }
    store<u32>(pna, static_cast<u32>(static_cast<s32>(outA - outA0) / 4));  // srawi / addze
    store<u32>(pnb, static_cast<u32>(static_cast<s32>(outB - outB0) / 4));
}

namespace {
constexpr u32 Matrix_LookAt = 0x0056A0D0;          // (out, eye, target, up)
constexpr u32 Matrix_PerspectiveWH = 0x0056A2D8;   // (out; f1 w, f2 h, f3 n, f4 f)
constexpr u32 Mat4_Ortho = 0x0056A358;             // (out; f1 l, f2 r, f3 b, f4 t, f5 n, f6 f)
constexpr u32 Matrix_MultiplyTo = 0x0055499C;      // (out, a, b)
constexpr u32 Tex_GetDefault = 0x004FEF30;         // (0) -> texture
constexpr u32 Vec_TransformByMatrix = 0x00553A2C;  // (f32 out[4], const f32 in[3], const f32 m[16])
constexpr u32 Frustum_FromMatrix = 0x004FDEA0;     // (f32 planes[6][4], const f32 m[16]) (name?)
constexpr u32 Plane_Transform = 0x0055EA6C;        // (f32 plane[4], const f32 m[16]): by the inverse transpose (name?)
constexpr u32 kLcConsts = 0x00572CF8;              // f32 1/255, f32 1, f64 1, f32 0.001, -, f64 0.001, f32 -0.5, f32 0.5
constexpr u32 kLcPostMatrix = 0x0116BEC0;          // f32[16]: what the view-projection is multiplied by (name?)
constexpr u32 kLcViewProj = 0x010DD7C0;            // layers4.h view_proj
constexpr u32 kLcPlanes = 6;

// The clamp of a projected depth: min(x, 1) (NaN -> 1), then at least
// 0.001 (fsel pairs, the f64 constants rounded to single).
f32 ClampDepth(f32 x) {
    const u32 c = kLcConsts;
    const f32 one = load<f32>(c + 4), lo = load<f32>(c + 16);
    const f64 one_d = load<f64>(c + 8), lo_d = load<f64>(c + 24);
    f64 v = static_cast<f32>(static_cast<f64>(one) - x) >= 0.0 ? static_cast<f64>(x) : one_d;
    const f32 a = static_cast<f32>(v);
    v = static_cast<f32>(static_cast<f64>(a) - lo) >= 0.0 ? static_cast<f64>(a) : lo_d;
    return static_cast<f32>(v);
}
}  // namespace

// void LightCam_Setup(const LightCam* c, -, LightCamOut* out; f32 scale
// in f1) (name?)
// The light's camera for the job: the look-at (c +16 eye, +32 target,
// +48 up) times its projection (perspective c +88 / +92 w h, +80 / +84
// near far; or, with c +4 set, the centred orthographic box of the same
// w h) into out +0, that times 0x0116BEC0 into out +64, out +128 the
// position (c +64), +144 the texture (c +96 or the default) +56, +160
// the colour (c +100 u8[4] / 255, alpha times scale), +176 / +180 the
// projected depths of c +112 / +116 (clamped to [0.001, 1]), +184 / +188
// zero, and +192 the six frustum planes taken through view_proj and
// normalized.
DECOMP(00572D20, LightCam_Setup) {
    const u32 c = arg<u32>(ctx, 0), out = arg<u32>(ctx, 2);
    const f64 scale = ctx.fpr[1];
    Stack frame(ctx, 640);
    const u32 b = frame.addr();
    const u32 vec = b, t1 = b + 16, t2 = b + 32, look = b + 48, proj_copy = b + 112;
    const u32 persp = b + 176, ortho = b + 240, planes = b + 304;
    call(ctx, Matrix_LookAt, {look, c + 16, c + 32, c + 48});
    u32 proj;
    if (load<u32>(c + 4) == 0) {
        proj = persp;
        callf(ctx, Matrix_PerspectiveWH, {proj},
              {static_cast<f64>(load<f32>(c + 88)), static_cast<f64>(load<f32>(c + 92)), static_cast<f64>(load<f32>(c + 80)),
               static_cast<f64>(load<f32>(c + 84))});
    } else {
        proj = ortho;
        const f64 lo = static_cast<f64>(load<f32>(kLcConsts + 32)), hi = static_cast<f64>(load<f32>(kLcConsts + 36));
        const f64 w = static_cast<f64>(load<f32>(c + 88)), h = static_cast<f64>(load<f32>(c + 92));
        callf(ctx, Mat4_Ortho, {proj},
              {static_cast<f32>(w * lo), static_cast<f32>(w * hi), static_cast<f32>(h * lo), static_cast<f32>(h * hi),
               static_cast<f64>(load<f32>(c + 80)), static_cast<f64>(load<f32>(c + 84))});
    }
    for (u32 k = 0; k < 16; ++k) store<u32>(proj_copy + 4 * k, load<u32>(proj + 4 * k));  // lfs / stfs: raw
    {
        Vec4 p[4], l[4];
        obj8::MatLoad(proj_copy, p);
        obj8::MatLoad(look, l);
        for (u32 r = 0; r < 4; ++r) {
            Vec4 a = VMadd(p[0], VSplat(l[r], 0), r == 3 ? p[3] : VZero());
            a = VMadd(p[1], VSplat(l[r], 1), a);
            a = VMadd(p[2], VSplat(l[r], 2), a);
            obj8::VStore(out + 16 * r, a);
        }
    }
    call(ctx, Matrix_MultiplyTo, {out + 64, out, kLcPostMatrix});
    for (u32 k = 0; k < 3; ++k) store<u32>(out + 128 + 4 * k, load<u32>(c + 64 + 4 * k));
    u32 tex = load<u32>(c + 96);
    if (tex == 0) tex = static_cast<u32>(call(ctx, Tex_GetDefault, {0}));
    store<u32>(out + 144, tex + 56);
    const f64 inv255 = static_cast<f64>(load<f32>(kLcConsts));
    for (u32 k = 0; k < 4; ++k)
        store<f32>(out + 160 + 4 * k, static_cast<f32>(static_cast<f64>(load<u8>(c + 100 + k)) * inv255));
    store<f32>(out + 172, static_cast<f32>(static_cast<f64>(load<f32>(out + 172)) * scale));
    for (u32 k = 0; k < 2; ++k) {
        store<u32>(vec, 0);
        store<u32>(vec + 4, 0);
        store<u32>(vec + 8, load<u32>(c + 112 + 4 * k) ^ 0x80000000u);  // fneg
        call(ctx, Vec_TransformByMatrix, {k == 0 ? t1 : t2, vec, proj_copy});
    }
    store<f32>(out + 176, ClampDepth(static_cast<f32>(static_cast<f64>(load<f32>(t1 + 8)) / load<f32>(t1 + 12))));
    store<f32>(out + 180, ClampDepth(static_cast<f32>(static_cast<f64>(load<f32>(t2 + 8)) / load<f32>(t2 + 12))));
    store<u32>(out + 184, 0);
    store<u32>(out + 188, 0);
    call(ctx, Frustum_FromMatrix, {planes, out});
    const f64 one = static_cast<f64>(load<f32>(kLcConsts + 4));
    for (u32 i = 0; i < kLcPlanes; ++i) {
        const u32 d = out + 192 + 16 * i;
        for (u32 k = 0; k < 4; ++k) store<u32>(d + 4 * k, load<u32>(planes + 16 * i + 4 * k));
        call(ctx, Plane_Transform, {d, kLcViewProj});
        const f32 y = load<f32>(d + 4), x = load<f32>(d), z = load<f32>(d + 8), w = load<f32>(d + 12);
        f32 len = static_cast<f32>(static_cast<f64>(y) * y);
        len = static_cast<f32>(std::fma(static_cast<f64>(x), x, static_cast<f64>(len)));
        len = static_cast<f32>(std::fma(static_cast<f64>(z), z, static_cast<f64>(len)));
        const f32 s = static_cast<f32>(one / static_cast<f32>(std::sqrt(static_cast<f64>(len))));
        store<f32>(d, static_cast<f32>(static_cast<f64>(x) * s));
        store<f32>(d + 4, static_cast<f32>(static_cast<f64>(y) * s));
        store<f32>(d + 8, static_cast<f32>(static_cast<f64>(z) * s));
        store<f32>(d + 12, static_cast<f32>(static_cast<f64>(w) * s));
    }
}

namespace {
constexpr u32 Mem_NewArray = 0x00735F28;    // (u32 size): Mem_Alloc(size, 0, 16) (name?)
constexpr u32 Mem_AllocPool = 0x0056C514;   // (align, size, pool, tag)
constexpr u32 Mem_FreePool = 0x0056C730;    // (p, pool, tag)
constexpr u32 Mem_InitSystemStats = 0x0056C2E4;
constexpr u32 Mem_New = 0x007A3DD8;
constexpr u32 kMemmove = 0x0071C3F0;       // memmove
constexpr u32 kSbData = 0, kSbCapacity = 8, kSbPooled = 20;  // StrBuf: u8 +20 = from pool 1 (name?)
constexpr u32 kSbPool = 1;               // the pool a pooled StrBuf uses

// Mem_Alloc's lazy set-up as the callers inline it (memory/alloc.cpp
// mem_lazy_init, Heap_Construct inlined; the nested levels see the guard
// set and go straight to Mem_AllocPool).
void MemLazyInit(PPUContext& ctx) {
    const u32 st = kMemState;
    if (load<u8>(st) != 0) return;
    store<u8>(st, 1);
    store<u8>(st + 8, 0);
    store<u8>(st + 9, 1);
    call(ctx, Mem_InitSystemStats);
    const u32 h = static_cast<u32>(call(ctx, Mem_New, {36}));
    if (h != 0) {
        store<u32>(h + 4, 0);
        store<u32>(h + 8, 0);
        store<u32>(h + 0, kLinearHeapVtable);
        store<u32>(h + 12, 0);
        store<u32>(h + 20, kDefaultLinearHeapSize);
        store<u32>(h + 24, static_cast<u32>(kDefaultLinearHeapPool));
        store<u8>(h + 28, 1);
        store<u32>(h + 32, kDefaultLinearHeapSize);
        const u32 base = static_cast<u32>(call(ctx, Mem_AllocPool, {16, kDefaultLinearHeapSize, static_cast<u32>(kDefaultLinearHeapPool), 0}));
        store<u32>(h + 4, base);
        store<u32>(h + 8, base != 0 ? base + load<u32>(h + 20) : 0);
        store<u32>(h + 12, base);
    }
    store<u32>(st + 4, h);
}
}  // namespace

// void StrBuf_Grow(StrBuf* b, s32 capacity) (name?)
// Grows the buffer (+0, capacity +8) to `capacity` when it is smaller:
// a new block (Mem_NewArray, or with +20 set from the system pool), the
// old contents (capacity bytes) moved over and the old block freed (pool
// 0 or 1). As in the original the pooled block is allocated with the OLD
// capacity (r27), though +8 still gets the new one.
DECOMP(00566950, StrBuf_Grow) {
    const u32 b = arg<u32>(ctx, 0);
    const s32 cap = arg<s32>(ctx, 1);
    const s32 old = load<s32>(b + kSbCapacity);
    if (!(old < cap)) return;
    u32 p;
    if (load<u8>(b + kSbPooled) == 0) {
        p = static_cast<u32>(call(ctx, Mem_NewArray, {static_cast<u32>(cap)}));
    } else {
        MemLazyInit(ctx);
        p = static_cast<u32>(call(ctx, Mem_AllocPool, {16, static_cast<u32>(old), kSbPool, 0}));
    }
    call(ctx, kMemmove, {p, load<u32>(b + kSbData), load<u32>(b + kSbCapacity)});
    call(ctx, Mem_FreePool, {load<u32>(b + kSbData), load<u8>(b + kSbPooled) != 0 ? kSbPool : 0u, 0});
    store<u32>(b + kSbData, p);
    store<u32>(b + kSbCapacity, static_cast<u32>(cap));
}

namespace {
constexpr u32 kLtConsts = 0x00597110;  // f32 0, 1/65535, 0.5, 2, 2pi, -, f64 pi, f64 -pi, f32 pi/2, pi, 4/pi, -4/pi^2, 0.225, 1, 5
constexpr u32 kLtZero = 0, kLtInv65535 = 4, kLtHalf = 8, kLtTwo = 12, kLtTwoPi = 16, kLtPiD = 24, kLtNegPiD = 32;
constexpr u32 kLtHalfPi = 40, kLtPi = 44, kLt4OverPi = 48, kLtNeg4OverPi2 = 52, kLtSinP = 56, kLtOne = 60, kLtFalloff = 64;
constexpr u32 kLtRecSize = 80;
// Record fields before the animation ones (obj6.h kLa*): +0 f32 the
// falloff start distance, +4 f32 the cull distance^2, +8 f32 the full-rate
// distance^2, +16 f32[3] the position, +28 f32 the view-plane offset.
constexpr u32 kLtFadeDist = 0, kLtCullSq = 4, kLtFullSq = 8, kLtPos = 16, kLtPlaneOff = 28;
constexpr u32 kLtOwnerRange = 376, kLtOwnerType = 368;  // classes.h kLightRangeSq / kLightType
constexpr u32 kLtConfig = 0x010C5A20;                   // classes.h kConfigLights

inline f32 LC(u32 off) { return load<f32>(kLtConsts + off); }
// The single-precision ops (operands as the f64 registers hold them).
inline f32 Sub(f64 a, f64 b) { return static_cast<f32>(a - b); }
inline f32 Add(f64 a, f64 b) { return static_cast<f32>(a + b); }
inline f32 Mul(f64 a, f64 b) { return static_cast<f32>(a * b); }
inline f32 Div(f64 a, f64 b) { return static_cast<f32>(a / b); }
inline f32 Madd(f64 a, f64 c, f64 b) { return static_cast<f32>(std::fma(a, c, b)); }
inline f32 Msub(f64 a, f64 c, f64 b) { return static_cast<f32>(std::fma(a, c, -b)); }
inline f32 Nmsub(f64 a, f64 c, f64 b) { return -static_cast<f32>(std::fma(a, c, -b)); }

// The inlined random step (obj6_light RandomPrev): the previous value.
f32 RandomPrev() {
    const u64 last = load<u64>(obj6::kRandomLast);
    const u32 s = load<u32>(obj6::kRandomState);
    const u32 mix = (std::rotl(s, 4) + std::rotl(s, 16)) ^ obj6::kRandomMix;
    store<u32>(obj6::kRandomState, std::rotl(mix, 16));
    store<u64>(obj6::kRandomLast, mix & 0xFFFFu);
    return static_cast<f32>(static_cast<f64>(static_cast<s64>(last)));
}

// Advances one light animation record by dt (obj6_light's step with the
// phase moved by dt) and pushes a changed intensity to its owner.
void AnimateLight(PPUContext& ctx, u32 rec, f64 dt) {
    using namespace obj6;
    const s32 type = static_cast<s32>(load<u32>(rec + kLaType));
    const f32 zero = LC(kLtZero);
    if (type == 3) {
        const f32 period = load<f32>(rec + kLaPeriod);
        if (!(period <= zero)) {
            const f32 p = Add(load<f32>(rec + kLaPhase), dt);
            store<f32>(rec + kLaPhase, p);
            if (p > period) store<f32>(rec + kLaPhase, period);
        }
    } else if (type == 0) {
        f32 p = Add(load<f32>(rec + kLaPhase), dt);
        const f32 period = load<f32>(rec + kLaPeriod);
        store<f32>(rec + kLaPhase, p);
        if (!(period <= zero))
            while (p > period) {
                p = Sub(p, period);
                store<f32>(rec + kLaPhase, p);
            }
    } else if (type == 1) {
        const f32 period = load<f32>(rec + kLaPeriod);
        if (!(period <= zero)) {
            if (load<u8>(rec + kLaDir) != 0) {
                const f32 p = Sub(load<f32>(rec + kLaPhase), dt);
                store<f32>(rec + kLaPhase, p);
                if (p < zero) {
                    store<u8>(rec + kLaDir, 0);
                    store<f32>(rec + kLaPhase, -p);
                }
            } else {
                f32 p = Add(load<f32>(rec + kLaPhase), dt);
                store<f32>(rec + kLaPhase, p);
                if (p > period) {
                    do {
                        p = Sub(p, period);
                        store<f32>(rec + kLaPhase, p);
                    } while (p > period);
                    store<u8>(rec + kLaDir, 1);
                    store<f32>(rec + kLaPhase, Sub(period, p));
                }
            }
        }
    } else if (type == 2) {
        f32 p = Add(load<f32>(rec + kLaPhase), dt);
        const f32 period = load<f32>(rec + kLaPeriod);
        store<f32>(rec + kLaPhase, p);
        if (!(period <= zero)) {
            if (p > period) {
                store<u32>(rec + kLaRandA, load<u32>(rec + kLaRandB));
                store<f32>(rec + kLaRandB, Mul(RandomPrev(), LC(kLtInv65535)));
            }
            while (p > period) {
                p = Sub(p, period);
                store<f32>(rec + kLaPhase, p);
            }
        }
    }
    // The intensity.
    const f32 rnd = RandomPrev();
    f32 v = load<f32>(rec + kLaBase);
    f32 phase = load<f32>(rec + kLaPhase);
    const f32 half = LC(kLtHalf);
    f32 noise = Mul(rnd, LC(kLtInv65535));
    noise = Msub(noise, noise, half);
    noise = Mul(noise, LC(kLtTwo));
    noise = Mul(noise, load<f32>(rec + kLaNoise));
    if (phase < zero) phase = zero;
    const f32 period = load<f32>(rec + kLaPeriod);
    if (phase > period) phase = period;
    if (period > zero) {
        if (type == 1) {
            const f32 twopi = LC(kLtTwoPi);
            const f32 x = Div(Mul(phase, twopi), period);
            const f32 pi = static_cast<f32>(x >= 0.0f ? load<f64>(kLtConsts + kLtPiD) : load<f64>(kLtConsts + kLtNegPiD));
            f32 a = Add(pi, x);
            const f32 k = static_cast<f32>(rt::fcfid(rt::fctid(static_cast<f64>(Div(a, twopi)), true)));
            a = Nmsub(k, twopi, a);
            a = Sub(a, pi);
            a = Add(a, LC(kLtHalfPi));
            if (a > LC(kLtPi)) a = Sub(a, twopi);
            f32 s = Mul(Mul(a, LC(kLtNeg4OverPi2)), std::fabs(a));
            s = Madd(a, LC(kLt4OverPi), s);
            const f32 t = Msub(s, std::fabs(s), s);
            s = Madd(t, LC(kLtSinP), s);
            s = Add(s, LC(kLtOne));
            v = Madd(Mul(load<f32>(rec + kLaAmp), s), half, v);
        } else if (type == 2) {
            const f32 x = Div(phase, period);
            const f32 w = Mul(Sub(LC(kLtOne), x), load<f32>(rec + kLaRandA));
            noise = Mul(Madd(x, load<f32>(rec + kLaRandB), w), load<f32>(rec + kLaAmp));
        } else {
            v = Add(v, Div(Mul(load<f32>(rec + kLaAmp), phase), period));
        }
        v = Add(v, noise);
        if (v < zero) v = zero;
    }
    v = static_cast<f32>(std::sqrt(static_cast<f64>(v)));
    const u32 owner = load<u32>(rec + kLaOwner);
    if (load<f32>(owner + kLtOwnerRange) == v) return;
    const s32 otype = static_cast<s32>(load<u32>(owner + kLtOwnerType));
    store<f32>(owner + kLtOwnerRange, v);
    if (otype != 2) store<u32>(kLtConfig + 108, load<u32>(kLtConfig + 96));
    obj4::Vcall0(ctx, owner, obj4::kVfnChanged);
}
}  // namespace

// void Lights_TickB(LightList* l; f32 dt in f1) (name?)
// For a non-empty light record list ({records, u16 count}* at +0) whose
// kind (+4) ticks this frame (0x010DDA70): every 80-byte animation record
// is advanced by dt. Records without 0x80 (+12) are distance-culled
// against the thread view: skipped beyond +4 (squared) or behind the view
// plane (+336 vec4 . position + 28 < 0), and beyond +8 (squared) advanced
// by dt * min((+0 - distance) / 5, 1) instead. (A count of 0xFFFF loops to
// address 0, as the original.)
DECOMP(00597158, Lights_TickB) {
    const u32 l = arg<u32>(ctx, 0);
    const f64 dt = ctx.fpr[1];
    const u32 hdr = load<u32>(l);
    if (hdr == 0 || load<u16>(hdr + 4) == 0) return;
    const u32 sh = load<u8>(l + 4) & 63;  // slw
    const u32 bit = (sh < 32 ? (1u << sh) : 0u) & 0xFF;
    if ((bit & load<u8>(kTickMask)) == 0) return;
    const u32 tv = static_cast<u32>(call(ctx, GetForThread));
    const u32 h = load<u32>(l);
    u32 rec = load<u32>(h);
    const u32 count = load<u16>(h + 4);
    const f32 vx = load<f32>(tv + kThreadView336), vy = load<f32>(tv + kThreadView336 + 4);
    const f32 vz = load<f32>(tv + kThreadView336 + 8), vw = load<f32>(tv + kThreadView336 + 12);
    const f32 px = load<f32>(tv + kThreadViewPos), py = load<f32>(tv + kThreadViewPos + 4), pz = load<f32>(tv + kThreadViewPos + 8);
    const u32 end = count != 0xFFFF ? rec + count * kLtRecSize : 0;
    do {
        f64 t = dt;
        bool run = true;
        if ((load<u8>(rec + obj6::kLaFlags) & 0x80) == 0) {
            const f32 dx = Sub(load<f32>(rec + kLtPos), px), dy = Sub(load<f32>(rec + kLtPos + 4), py);
            const f32 dz = Sub(load<f32>(rec + kLtPos + 8), pz);
            f32 d2 = Mul(dy, dy);
            d2 = Madd(dx, dx, d2);
            d2 = Madd(dz, dz, d2);
            if (d2 > load<f32>(rec + kLtCullSq)) {
                run = false;
            } else {
                f32 side = Madd(vx, load<f32>(rec + kLtPos), vw);
                side = Madd(vz, load<f32>(rec + kLtPos + 8), side);
                side = Madd(vy, load<f32>(rec + kLtPos + 4), side);
                side = Add(side, load<f32>(rec + kLtPlaneOff));
                if (side < LC(kLtZero)) {
                    run = false;
                } else if (d2 > load<f32>(rec + kLtFullSq)) {
                    const f32 d = static_cast<f32>(std::sqrt(static_cast<f64>(d2)));
                    f32 k = Div(Sub(load<f32>(rec + kLtFadeDist), d), LC(kLtFalloff));
                    if (k > LC(kLtOne)) k = LC(kLtOne);
                    t = Mul(dt, k);
                }
            }
        }
        if (run) AnimateLight(ctx, rec, t);
        rec += kLtRecSize;
    } while (rec != end);
}

namespace {
constexpr u32 View_GetCurrent = 0x004FDBA8;
constexpr u32 Matrix_Perspective = 0x0056A200;   // (out; f1 fov, f2 aspect, f3 near, f4 far)
constexpr u32 Quads_ClipSetup = 0x00578F64;      // (out, planes, planes +16, +32, +48, near plane, -view dir) (name?)
constexpr u32 Quad_InClip = 0x00578868;          // (clip, quad) -> bool (name?)
constexpr u32 Quad_InFrustum = 0x00577E40;       // (quad, near plane, viewproj, planes) -> bool (name?)
constexpr u32 kQpConsts = 0x005792D4;            // f32 1, -FLT_MAX, 180/pi, pi/180, 0, -1, 0.25
constexpr u32 kQpOne = 0, kQpLowest = 4, kQpToDeg = 8, kQpToRad = 12, kQpZero = 16, kQpMinusOne = 20, kQpQuarter = 24;
constexpr u32 kQuadMax = 0x00F88946;             // u16: how many quads are kept (name?)
constexpr u32 kQuadSorted = 0x01192E00;          // {Quad*, f32 distance^2}[kQuadMax], nearest first (name?)
constexpr u32 kQuadSortedCount = 0x01193000;     // u16
constexpr u32 kQuadSize = 112;                   // 4 corners (f32[4] each), +68 u16 bit 0 = enabled
constexpr u32 kQuadPlanes = 5;                   // planes per kept quad: its own and the four edges'
constexpr u32 kCamDir = 32, kCamPos = 48;
constexpr u32 kViewNear = 60, kViewFar = 64, kViewFov = 56, kViewAspect = 112, kViewMode = 49;
constexpr u32 kViewMat752 = 752, kViewMat1328 = 1328, kViewPlanes272 = 272, kViewMats368 = 368, kViewDir336 = 336;

inline f32 QC(u32 off) { return load<f32>(kQpConsts + off); }

// Stores the plane {n * sign, -(n * sign) . a} at dst (normal from the
// cross product (e1 x e2) normalized).
void StorePlane(u32 dst, f32 nx, f32 ny, f32 nz, f32 sign, u32 a, bool y_first) {
    f32 len = Mul(ny, ny);
    len = Madd(nx, nx, len);
    len = Madd(nz, nz, len);
    const f32 s = Div(QC(kQpOne), static_cast<f32>(std::sqrt(static_cast<f64>(len))));
    nx = Mul(Mul(nx, s), sign);
    ny = Mul(Mul(ny, s), sign);
    nz = Mul(Mul(nz, s), sign);
    const f32 ax = load<f32>(a), ay = load<f32>(a + 4), az = load<f32>(a + 8);
    if (y_first) {
        store<f32>(dst + 4, ny);
        store<f32>(dst + 0, nx);
    } else {
        store<f32>(dst + 0, nx);
        store<f32>(dst + 4, ny);
    }
    store<f32>(dst + 8, nz);
    store<f32>(dst + 12, -Madd(nz, az, Madd(nx, ax, Mul(ny, ay))));
}
}  // namespace

// void Quads_Prepare(Camera* cam, Quad* quads, u16 count, f32 out[][4],
// u16* out_count) (name?)
// Picks the nearest enabled quads that are in view (up to *0x00F88946,
// kept sorted by squared distance to cam +48 in 0x01192E00) and writes
// five clip planes for each into out (zeroed first): the quad's own
// plane facing away from the camera and the four planes through the
// camera and its edges. *out_count = the planes / 5. The view test uses a
// fresh perspective (view mode 2) or the view's own matrices.
DECOMP(005792F0, Quads_Prepare) {
    const u32 cam = arg<u32>(ctx, 0), quads = arg<u32>(ctx, 1);
    const u32 count = arg<u32>(ctx, 2) & 0xFFFF;
    const u32 out = arg<u32>(ctx, 3), pcount = arg<u32>(ctx, 4);
    call(ctx, fn::memset, {out, 0, ((load<u16>(kQuadMax) + 3u) & 0xFFFCu) * 5 * 16});
    store<u16>(pcount, 0);
    store<u16>(kQuadSortedCount, 0);
    if (load<u16>(kQuadMax) == 0) return;
    Stack frame(ctx, 744);  // the original's sp+112 .. sp+856
    const u32 sp = frame.addr() - 112;
    const u32 near_plane = sp + 112, neg_dir = sp + 128, viewproj = sp + 144, proj = sp + 208, planes = sp + 272;
    const u32 clip = sp + 496;
    const u32 view = static_cast<u32>(call(ctx, View_GetCurrent));
    const u32 dir = cam + kCamDir, pos = cam + kCamPos;
    {
        const f32 ny = -load<f32>(dir + 4), nx = -load<f32>(dir), n = load<f32>(view + kViewNear);
        const f32 nz = -load<f32>(dir + 8);
        const f32 qy = Madd(ny, n, load<f32>(pos + 4)), qx = Madd(nx, n, load<f32>(pos));
        const f32 qz = Madd(nz, n, load<f32>(pos + 8));
        store<f32>(near_plane, nx);
        store<f32>(near_plane + 4, ny);
        store<f32>(near_plane + 8, nz);
        store<f32>(near_plane + 12, -Madd(nz, qz, Madd(nx, qx, Mul(ny, qy))));
    }
    f32 limit = QC(kQpLowest);
    const bool persp = load<u8>(view + kViewMode) == 2;
    if (persp) {
        const f32 fov = Mul(Mul(load<f32>(view + kViewFov), QC(kQpToDeg)), QC(kQpToRad));
        callf(ctx, Matrix_Perspective, {proj},
              {fov, load<f32>(view + kViewAspect), load<f32>(view + kViewNear), load<f32>(view + kViewFar)});
        Vec4 m[4], p[4];
        obj8::MatLoad(view + kViewMat752, m);
        obj8::MatLoad(proj, p);
        for (u32 r = 0; r < 4; ++r) {
            Vec4 a = VMadd(p[0], VSplat(m[r], 0), r == 3 ? p[3] : VZero());
            a = VMadd(p[1], VSplat(m[r], 1), a);
            a = VMadd(p[2], VSplat(m[r], 2), a);
            obj8::VStore(viewproj + 16 * r, a);
        }
        call(ctx, Frustum_FromMatrix, {planes, viewproj});
        for (u32 k = 0; k < 4; ++k) store<u32>(neg_dir + 4 * k, load<u32>(view + kViewDir336 + 4 * k) ^ 0x80000000u);  // fneg
        call(ctx, Quads_ClipSetup, {clip, planes, planes + 16, planes + 32, planes + 48, near_plane, neg_dir});
    } else {
        for (u32 k = 0; k < 16; ++k) store<u32>(viewproj + 4 * k, load<u32>(view + kViewMat1328 + 4 * k));  // raw
        for (u32 k = 0; k < 24; ++k) store<u32>(planes + 4 * k, load<u32>(view + kViewPlanes272 + 4 * k));
        for (u32 k = 0; k < 32; ++k) store<u32>(sp + 368 + 4 * k, load<u32>(view + kViewMats368 + 4 * k));
    }
    // The candidates, sorted in by squared distance from the camera.
    const f64 quarter = QC(kQpQuarter);
    for (u32 q = quads, end = quads + count * kQuadSize; count > 0 && q < end; q += kQuadSize) {
        if ((load<u16>(q + 68) & 1) == 0) continue;
        f32 cy = Add(Add(Add(load<f32>(q + 4), load<f32>(q + 20)), load<f32>(q + 36)), load<f32>(q + 52));
        f32 cx = Add(Add(Add(load<f32>(q + 0), load<f32>(q + 16)), load<f32>(q + 32)), load<f32>(q + 48));
        f32 cz = Add(Add(Add(load<f32>(q + 8), load<f32>(q + 24)), load<f32>(q + 40)), load<f32>(q + 56));
        cy = Nmsub(cy, quarter, load<f32>(pos + 4));
        cx = Nmsub(cx, quarter, load<f32>(pos));
        cz = Nmsub(cz, quarter, load<f32>(pos + 8));
        f32 d2 = Mul(cy, cy);
        d2 = Madd(cx, cx, d2);
        d2 = Madd(cz, cz, d2);
        if (load<u16>(kQuadSortedCount) == load<u16>(kQuadMax) && d2 > limit) continue;
        u32 ok = 1;
        if (persp && static_cast<u32>(call(ctx, Quad_InClip, {clip, q})) != 0) ok = 0;
        // (r21 & rc: the clip test passing leaves 0, which skips the quad)
        const u32 rc = static_cast<u32>(call(ctx, Quad_InFrustum, {q, near_plane, viewproj, planes}));
        if ((ok & rc) == 0) continue;
        u32 n = load<u16>(kQuadSortedCount);
        s32 i = static_cast<s16>(n - 1);
        while (i >= 0 && !(d2 > load<f32>(kQuadSorted + static_cast<u32>(i) * 8 + 4))) i = static_cast<s16>(i - 1);
        const s32 at = i + 1;
        const u32 max = load<u16>(kQuadMax);
        if (!(at < static_cast<s32>(max))) continue;
        if (static_cast<s32>(n) < static_cast<s32>(max)) {
            ++n;
            store<u16>(kQuadSortedCount, static_cast<u16>(n));
        }
        for (s32 k = static_cast<s16>(n - 2); !(k < at); k = static_cast<s16>(k - 1)) {
            const u32 src = kQuadSorted + static_cast<u32>(k) * 8;
            store<u32>(src + 8, load<u32>(src));
            store<u32>(src + 12, load<u32>(src + 4));  // lfs / stfs: raw
        }
        const u32 e = kQuadSorted + static_cast<u32>(i) * 8;
        store<f32>(e + 12, d2);
        store<u32>(e + 8, q);
        limit = load<f32>(kQuadSorted + ((max << 3) & 0x7FFF8u) - 4);
    }
    // Five planes per kept quad.
    u32 np = 0;
    const u32 n = load<u16>(kQuadSortedCount);
    const f32 px = load<f32>(pos), py = load<f32>(pos + 4), pz = load<f32>(pos + 8);
    for (u32 i = 0; i < n; ++i) {
        const u32 q = load<u32>(kQuadSorted + 8 * i);
        const f32 x0 = load<f32>(q), y0 = load<f32>(q + 4), z0 = load<f32>(q + 8);
        const f32 e1x = Sub(load<f32>(q + 16), x0), e2z = Sub(load<f32>(q + 40), z0), e1z = Sub(load<f32>(q + 24), z0);
        const f32 e2x = Sub(load<f32>(q + 32), x0), e2y = Sub(load<f32>(q + 36), y0), e1y = Sub(load<f32>(q + 20), y0);
        const f32 ny = Msub(e1z, e2x, Mul(e1x, e2z));
        const f32 nx = Msub(e1y, e2z, Mul(e1z, e2y));
        const f32 nz = Msub(e1x, e2y, Mul(e1y, e2x));
        // Facing the camera: flip (the side of the camera from the unit normal).
        f32 len = Mul(ny, ny);
        len = Madd(nx, nx, len);
        len = Madd(nz, nz, len);
        const f32 s = Div(QC(kQpOne), static_cast<f32>(std::sqrt(static_cast<f64>(len))));
        const f32 uy = Mul(ny, s), ux = Mul(nx, s), uz = Mul(nz, s);
        f32 side = Mul(Sub(py, y0), uy);
        side = Madd(Sub(px, x0), ux, side);
        side = Madd(Sub(pz, z0), uz, side);
        const f32 sign = side > QC(kQpZero) ? QC(kQpMinusOne) : QC(kQpOne);
        {
            const u32 dst = out + 16 * np;
            np = (np + 1) & 0xFFFF;
            const f32 sx = Mul(ux, sign), sy = Mul(uy, sign), sz = Mul(uz, sign);
            store<f32>(dst + 0, sx);
            store<f32>(dst + 4, sy);
            store<f32>(dst + 8, sz);
            store<f32>(dst + 12, -Madd(sz, z0, Madd(sx, x0, Mul(sy, y0))));
        }
        for (u32 j = 0; j < 4; ++j) {
            const u32 a = q + 16 * j, b = q + 16 * ((j + 1) % 4);
            const f32 ax = load<f32>(a), az = load<f32>(a + 8), ay = load<f32>(a + 4);
            const f32 cx = Sub(px, ax), cz = Sub(pz, az), ex = Sub(load<f32>(b), ax);
            const f32 ez = Sub(load<f32>(b + 8), az), cy = Sub(py, ay), ey = Sub(load<f32>(b + 4), ay);
            const f32 my = Msub(ez, cx, Mul(ex, cz));
            const f32 mx = Msub(ey, cz, Mul(ez, cy));
            const f32 mz = Msub(ex, cy, Mul(ey, cx));
            StorePlane(out + 16 * np, mx, my, mz, sign, a, true);
            np = (np + 1) & 0xFFFF;
        }
    }
    store<u16>(pcount, static_cast<u16>(static_cast<s32>(np + 4) / static_cast<s32>(kQuadPlanes)));
}

namespace {
constexpr u32 Box_SetPoint = 0x00556F60;         // (f32 box[8], const f32 p[3])
constexpr u32 Box_AddPoint = 0x00556FE8;
constexpr u32 kGiPos = 128;                      // ApeGeomInstance: f32[3] (name?)
constexpr u32 kBgBoundsMin = 5328, kBgBoundsMax = 5344;  // bank globals: the instances' box (name?)
constexpr u32 kFltMax = 0x7F7FFFFF, kFltLowest = 0xFF7FFFFF;
}  // namespace

// void Bank_ComputeBounds() (name?)
// The box around every geometry instance's +128 point (the box starts
// as the origin point and is reset to the first instance's), stored in
// the bank globals +5328 / +5344.
DECOMP(005900BC, Bank_ComputeBounds) {
    Stack frame(ctx, 32);
    const u32 box = frame.addr();
    for (u32 k = 0; k < 3; ++k) store<u32>(box + 4 * k, kFltMax);
    for (u32 k = 0; k < 3; ++k) store<u32>(box + 16 + 4 * k, kFltLowest);
    call(ctx, Box_SetPoint, {box, cls::kOrigin});
    const InstList& il = kInstLists[0];
    const u32 alloc = obj6::ListAllocator(ctx, il.hdr, il.vtbl, il.name, il.size);
    const u32 first = static_cast<u32>(call(ctx, Obj_GetField4, {alloc}));
    bool reset = true;
    for (u32 it = first != 0 ? first + kListNode : 0; it != 0;) {
        call(ctx, reset ? Box_SetPoint : Box_AddPoint, {box, it + kGiPos});
        reset = false;
        const u32 next = load<u32>(it - kListNode);
        it = next != 0 ? next + kListNode : 0;
    }
    const u32 bg = obj15::kBankGlobals;
    for (u32 k = 0; k < 3; ++k) store<u32>(bg + kBgBoundsMin + 4 * k, load<u32>(box + 4 * k));  // lfs / stfs: raw
    for (u32 k = 0; k < 3; ++k) store<u32>(bg + kBgBoundsMax + 4 * k, load<u32>(box + 16 + 4 * k));
}

namespace {
constexpr u32 Box_BoundingSphere = 0x00557870;   // (f32 out[4], const Box*)
constexpr u32 String_Assign = 0x00555F80;        // (dst, src)
constexpr u32 kCdConsts = 0x0058546C;            // f32 0, 1, 100, 1e-5
constexpr u32 kCdDefaultFov = 0x42480000;        // 50.0
constexpr u32 kCdConfig = 0x010C5A20;            // g::config: +56 f32 the default for +128 (name?)
}  // namespace

// void GeomDef_FromRecord(GeomDef* d, const GeomRec* r) (name?)
// Fills a geometry definition from its file record (GeomDef_Read): the
// box (+16 / +32 from bboxMin / bboxMax) and its bounding sphere (+48),
// the name (+64), nameHash / animHash / lightGroup to +96..104,
// shadowType / shadowExtrusionLength / shadowLOD to +113..115, +116 =
// shadowZCullDist (50 when |x| < 1e-5), +120 = nearZFadeRange clamped to
// [1e-5, 100] (0 becomes 1, also written back to the record), +124 =
// nearZFadeDist and +128 = minNearFadeAlpha (negative: config +56)
// clamped to [0, 1].
DECOMP(00585480, GeomDef_FromRecord) {
    const u32 d = arg<u32>(ctx, 0), r = arg<u32>(ctx, 1);
    const u32 c = kCdConsts;
    for (u32 k = 0; k < 3; ++k) store<u32>(d + 16 + 4 * k, load<u32>(r + 4 * k));
    for (u32 k = 0; k < 3; ++k) store<u32>(d + 32 + 4 * k, load<u32>(r + 12 + 4 * k));
    {
        Stack frame(ctx, 16);
        call(ctx, Box_BoundingSphere, {frame.addr(), d + 16});
        for (u32 k = 0; k < 4; ++k) store<u32>(d + 48 + 4 * k, load<u32>(frame.addr() + 4 * k));
    }
    call(ctx, String_Assign, {d + 64, r + 36});
    store<u32>(d + 96, load<u32>(r + 24));
    store<u32>(d + 100, load<u32>(r + 28));
    store<u32>(d + 104, load<u32>(r + 32));
    const f32 zero = load<f32>(c), one = load<f32>(c + 4), hundred = load<f32>(c + 8), eps = load<f32>(c + 12);
    f32 near = load<f32>(r + 72);
    const u32 far_w = load<u32>(r + 76);
    f32 f88 = load<f32>(r + 88);
    const u8 b0 = load<u8>(r + 80), b1 = load<u8>(r + 81), b2 = load<u8>(r + 82);
    const u32 fov_w = load<u32>(r + 84);
    if (near == zero) {
        store<u32>(r + 72, cls::kOneF);
        near = one;
    }
    const f32 m = near < hundred ? near : hundred;
    const f32 v120 = m > eps ? m : eps;
    store<f32>(d + 120, v120);
    store<u32>(d + 124, far_w);
    store<u8>(d + 113, b0);
    store<u8>(d + 114, b1);
    store<u8>(d + 115, b2);
    store<u32>(d + 116, fov_w);
    if (std::fabs(std::bit_cast<f32>(fov_w)) < eps) store<u32>(d + 116, kCdDefaultFov);
    if (f88 < zero) f88 = load<f32>(kCdConfig + 56);
    const f32 m2 = f88 < one ? f88 : one;
    store<f32>(d + 128, m2 > zero ? m2 : zero);
}

namespace {
constexpr u32 Mem_Alloc = 0x0054ED38;               // (size, pool, align)
constexpr u32 Mem_DefaultAllocator = 0x00550EE8;
constexpr u32 Array_Init = 0x00558F38;
constexpr u32 Holder_New550E70 = 0x00550E70;
constexpr u32 Array_Init96 = 0x007A7674;            // (array, count, opd, holder)
constexpr u32 Iff_RegisterChunk = 0x0055902C;       // (fourcc, opd, arg, name, flags)
constexpr u32 kSiConfigCap = 0x010C5A20 + 372;      // u16 g::config: the map capacity (0 = 64) (name?)
constexpr u32 kSiHolder = 0x00F8F158;               // the holder: {+0, +4 capacity, +8, +16 the map} (name?)
constexpr u32 kSiArray = 0x01194580;                // 96-byte elements (name?)
constexpr u32 kSiArrayOpd = 0x00F3CA00;
constexpr u32 kSiChunkS = 0x53495053, kSiChunkSOpd = 0x00F3C9E8;  // 'SIPS'
constexpr u32 kSiChunkH = 0x48495053, kSiChunkHOpd = 0x00F3C9F0;  // 'HIPS'
constexpr u32 kSiDefaultCap = 64;

// The definition holders Resources_Init makes (a header with a sorted
// map, obj.h layout): the capacity from a config u16 (`dflt` when 0, none
// when negative), the holder {+0, +4 capacity, +8, +16 the map} stored at
// `slot`, the map's entries from the default allocator. As in the
// original a failed holder allocation still has the map stored at
// address 16. `cmp_first`: the compare (+20) is stored before the
// default (+16). Returns the capacity.
u32 NewDefHolder(PPUContext& ctx, u32 cfg, u32 dflt, u32 slot, u32 cmp, bool cmp_first) {
    u32 cap = dflt;
    const u32 v = load<u16>(cfg);
    if (v != 0) cap = static_cast<s16>(v) > 0 ? v : 0;
    u32 h = static_cast<u32>(call(ctx, Mem_Alloc, {20, 0, 16}));
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
        if (cmp_first) {
            store<u32>(m + 20, cmp);
            store<u32>(m + 16, 0);
        } else {
            store<u32>(m + 16, 0);
            store<u32>(m + 20, cmp);
        }
    }
    store<u32>(h + 16, m);
    return cap;
}

}  // namespace

// void Resources_InitSplineInsts() (name?)
// The spline instance holder (*0x00F8F158; capacity config +372, 64
// when 0) with its map (compare obj16 kCmpSplineInst), the 1024-entry
// array at 0x01194580 and the 'SIPS' / 'HIPS' chunk readers.
DECOMP(00586FEC, Resources_InitSplineInsts) {
    NewDefHolder(ctx, kSiConfigCap, kSiDefaultCap, kSiHolder, obj16::kCmpSplineInst, false);
    const u32 holder = static_cast<u32>(call(ctx, Holder_New550E70));
    call(ctx, Array_Init96, {kSiArray, 1024, kSiArrayOpd, holder});
    call(ctx, Iff_RegisterChunk, {kSiChunkS, kSiChunkSOpd, 0, 0, 16});
    call(ctx, Iff_RegisterChunk, {kSiChunkH, kSiChunkHOpd, 0, 0, 16});
}

namespace {
constexpr u32 Array_Init4 = 0x007A77A0;            // (array, count, opd, holder)
constexpr u32 kEiConfigCap = 0x010C5A20 + 376;     // u16 g::config (name?)
constexpr u32 kEiDefaultCap = 320;
constexpr u32 kEiHolder = 0x00F8F138;              // obj6.h kEmitterDefList
constexpr u32 kEiCompare = 0x00F34E50;             // MapE_Find's default compare
constexpr u32 kEiState = 0x01194570;               // u8 255; +12 the 16-byte array header (name?)
constexpr u32 kEiArrayOpd = 0x00F3C768;
constexpr u32 kEiChunkT = 0x54494D45, kEiChunkTOpd = 0x00F3C790;  // 'TIME'
constexpr u32 kEiChunkH = 0x48494D45, kEiChunkHOpd = 0x00F3C798;  // 'HIME'
}  // namespace

// void Resources_InitEmitterDefs() (name?)
// The emitter definition holder (*0x00F8F138; capacity config +376, 320
// when 0) with its map (compare 0x00F34E50), a 16-byte array header
// (0x01194570 +12, zeroed; capacity entries of 4 bytes), 0x01194570 =
// 255 and the 'TIME' / 'HIME' chunk readers.
DECOMP(00588240, Resources_InitEmitterDefs) {
    const u32 cap = NewDefHolder(ctx, kEiConfigCap, kEiDefaultCap, kEiHolder, kEiCompare, true);
    const u32 a = static_cast<u32>(call(ctx, Mem_Alloc, {16, 0, 16}));
    if (a != 0)
        for (u32 k = 0; k < 16; k += 4) store<u32>(a + k, 0);
    store<u32>(kEiState + 12, a);
    const u32 holder = static_cast<u32>(call(ctx, Holder_New550E70));
    call(ctx, Array_Init4, {a, cap, kEiArrayOpd, holder});
    store<u8>(kEiState, 255);
    call(ctx, Iff_RegisterChunk, {kEiChunkT, kEiChunkTOpd, 0, 0, 0});
    call(ctx, Iff_RegisterChunk, {kEiChunkH, kEiChunkHOpd, 0, 0, 0});
}

namespace {
constexpr u32 Iff_ReadU32 = 0x005642AC;             // (reader, u32* out, name)
constexpr u32 FileReader_ReadVec3Swapped = 0x005699F0;
constexpr u32 FileReader_Read32 = 0x00569C10;
constexpr u32 Bone_InitFromDesc = 0x00584C70;       // (bone, BoneDesc by value)
constexpr u32 kStrNameHash = 0x00B24A30, kStrParentNameHash = 0x00B24A39, kStrXBasis = 0x00B24A48;
constexpr u32 kStrZBasis = 0x00B24A4F, kStrPosition = 0x00B24A56, kStrName = 0x00B24A5F;
constexpr u32 kBoneDescSize = 76, kBoneSize = 112;
constexpr u32 kBoneNameHash = 64, kBoneParent = 100;  // u32 / u16
}  // namespace

// bool Skeleton_ReadBones(IffReader* r, Skeleton** ps) (name?)
// The bones chunk: for each of the skeleton's bones ({u16 count +2, Bone*
// +4}) its description (nameHash, parentNameHash, xBasis, zBasis,
// position, name) is read into one 76-byte record (zeroed once, before
// the loop) and handed by value to Bone_InitFromDesc; a bone whose parent
// hash matches an earlier bone gets that bone's index at +100. Returns 1.
DECOMP(00585268, Skeleton_ReadBones) {
    const u32 r = arg<u32>(ctx, 0);
    const u32 sk = load<u32>(arg<u32>(ctx, 1));
    Stack frame(ctx, 80);
    const u32 d = frame.addr();
    call(ctx, fn::memset, {d, 0, kBoneDescSize});
    u32 bone = load<u32>(sk + 4);
    for (u32 i = 0; i < load<u16>(sk + 2); ++i, bone += kBoneSize) {
        call(ctx, Iff_ReadU32, {r, d, kStrNameHash});
        call(ctx, Iff_ReadU32, {r, d + 4, kStrParentNameHash});
        call(ctx, FileReader_ReadVec3Swapped, {r, d + 8, kStrXBasis});
        call(ctx, FileReader_ReadVec3Swapped, {r, d + 20, kStrZBasis});
        call(ctx, FileReader_ReadVec3Swapped, {r, d + 32, kStrPosition});
        call(ctx, FileReader_Read32, {r, d + 44, kStrName});
        auto dw = [&](u32 k) { return (static_cast<u64>(load<u32>(d + 8 * k)) << 32) | load<u32>(d + 8 * k + 4); };
        // words 0..13 in r4..r10, 14..18 in the parameter save area (+112..+131)
        call(ctx, Bone_InitFromDesc,
             {bone, dw(0), dw(1), dw(2), dw(3), dw(4), dw(5), dw(6), dw(7), dw(8), static_cast<u64>(load<u32>(d + 72)) << 32});
        if (i != 0) {
            const u32 parent = load<u32>(d + 4);
            u32 b = load<u32>(sk + 4);
            for (u32 j = 0; i > j; ++j, b += kBoneSize)
                if (parent == load<u32>(b + kBoneNameHash)) {
                    store<u16>(bone + kBoneParent, static_cast<u16>(j));
                    break;
                }
        }
    }
    ret(ctx, 1u);
}

namespace {
constexpr u32 LoadShaders = 0x00523AA8;     // (s32 which) -> the shader group's texture (name?)
constexpr u32 Draw_SetTexture = 0x00523310; // (fb, tex)
constexpr u32 Draw_SetState = 0x005218D8;   // (fb, const f32 m[16])
constexpr u32 Draw_BeginPrims = 0x005236A0; // (Prims* out, fb, u8 type, u16 count, r7 extra)
constexpr u32 Draw_EndPrims = 0x00523748;   // (fb)
constexpr u32 kDbgStream = 0x010DDBB4;      // u32: the debug draw buffer (obj16 kDebugStream)
constexpr u32 kDbgIdentity = 0x01164BD0;    // f32[16]
constexpr u32 kPrimQuads = 6;               // Draw_BeginPrims type (name?)
constexpr u32 kDbgVertexSize = 24;          // {f32 x, y, z; u8 rgba[4]; f32 u, v}
constexpr u32 kFbStart = 0, kFbEnd = 4, kFbRead = 12, kFbCursor = 16, kFbVertCap = 52, kFbVertCount = 72;

// The room check the debug draws inline (obj16 DrawRoom with `verts`
// more vertices).
bool DrawRoom(u32 fb, u32 verts) {
    const bool vok = load<u32>(fb + kFbVertCount) + verts < load<u32>(fb + kFbVertCap);
    const u32 cur = load<u32>(fb + kFbCursor), rd = load<u32>(fb + kFbRead);
    bool wrap_ok = true;
    if (cur + 259 > load<u32>(fb + kFbEnd)) wrap_ok = !(rd > cur) && (rd != load<u32>(fb + kFbStart));
    const u32 a = (cur + 3) & ~3u;
    const bool ahead_ok = !(a < rd) || (a + 256 < rd);
    return vok && wrap_ok && ahead_ok;
}
}  // namespace

// void Debug_DrawRect(const Rect* r) (f_0056AEFC's) (name?)
// Debug_DrawLine's sibling: when the debug draw buffer has room, a
// 4-vertex quad over (r +12, r +16) .. (+ r +20, + r +24) at z = -(r +28)
// in colour r +4..7, with the debug shader group's texture and the
// identity state. Draw_BeginPrims gets the r7 Draw_SetState leaves.
DECOMP(00569EC8, Debug_DrawRect) {
    const u32 r = arg<u32>(ctx, 0);
    const u32 fb = load<u32>(kDbgStream);
    if (!DrawRoom(fb, 4)) return;
    const u32 tex = static_cast<u32>(call(ctx, LoadShaders, {0}));
    call(ctx, Draw_SetTexture, {fb, tex});
    call(ctx, Draw_SetState, {fb, kDbgIdentity});
    Stack frame(ctx, 48);  // the original's sp+112 (prims) / sp+124 (the points)
    const u32 prims = frame.addr(), pts = prims + 12;
    const u32 x0 = load<u32>(r + 12), y0 = load<u32>(r + 16);
    const u32 x1 = std::bit_cast<u32>(Add(load<f32>(r + 12), load<f32>(r + 20)));
    const u32 y1 = std::bit_cast<u32>(Add(load<f32>(r + 16), load<f32>(r + 24)));
    const u32 negz = load<u32>(r + 28) ^ 0x80000000u;  // fneg
    const u32 p[8] = {x1, y0, x0, y0, x1, y1, x0, y1};
    for (u32 k = 0; k < 8; ++k) store<u32>(pts + 4 * k, p[k]);
    call(ctx, Draw_BeginPrims, {prims, fb, kPrimQuads, 4});
    u32 v = load<u32>(prims);
    const u32 index = load<u16>(prims + 6);
    const u8 c0 = load<u8>(r + 4), c1 = load<u8>(r + 5), c2 = load<u8>(r + 6), c3 = load<u8>(r + 7);
    for (u32 k = 0; k < 4; ++k) {
        store<u32>(v, load<u32>(pts + 8 * k));
        store<u32>(v + 4, load<u32>(pts + 8 * k + 4));
        store<u32>(v + 8, negz);
        store<u8>(v + 12, c0);
        store<u8>(v + 13, c1);
        store<u8>(v + 14, c2);
        store<u8>(v + 15, c3);
        store<u32>(v + 16, 0);
        store<u32>(v + 20, 0);
        v += kDbgVertexSize;
    }
    store<u16>(prims + 6, static_cast<u16>(index + 4));
    store<u32>(prims, v);
    ctx.fpr[1] = std::bit_cast<f32>(negz);
    call(ctx, Draw_EndPrims, {fb});
}

namespace {
constexpr u32 Iff_ReadU8 = 0x005643C4;              // (reader, u8* out, name)
constexpr u32 FileReader_ReadF32Swapped = 0x005698F4;
constexpr u32 kStrCdXBasis = 0x00B24C46, kStrCdZBasis = 0x00B24C4D, kStrCdPos = 0x00B24C54, kStrCdFov = 0x00B24C58;
constexpr u32 kStrCdNameHash = 0x00B24C5C, kStrCdAnimHash = 0x00B24C65, kStrCdName = 0x00B24C6E;
}  // namespace

// bool CameraDef_Read(IffReader* r, Def* d) (name?)
// The camera chunk: a version byte (unused), xBasis, zBasis, pos, fov,
// nameHash, animHash and the name read into a zeroed 80-byte record, then
// d +64 the name, +96 / +100 the hashes, +112 / +128 / +144 / +160 the
// rows x, z cross x, z and pos, +176 the fov. Returns 1.
DECOMP(0059CF98, CameraDef_Read) {
    const u32 r = arg<u32>(ctx, 0), d = arg<u32>(ctx, 1);
    Stack frame(ctx, 96);
    const u32 rec = frame.addr();  // the original's sp+112 (the version byte at +80)
    call(ctx, fn::memset, {rec, 0, 80});
    call(ctx, Iff_ReadU8, {r, rec + 80, 0});
    call(ctx, FileReader_ReadVec3Swapped, {r, rec, kStrCdXBasis});
    call(ctx, FileReader_ReadVec3Swapped, {r, rec + 12, kStrCdZBasis});
    call(ctx, FileReader_ReadVec3Swapped, {r, rec + 24, kStrCdPos});
    call(ctx, FileReader_ReadF32Swapped, {r, rec + 36, kStrCdFov});
    call(ctx, Iff_ReadU32, {r, rec + 40, kStrCdNameHash});
    call(ctx, Iff_ReadU32, {r, rec + 44, kStrCdAnimHash});
    call(ctx, FileReader_Read32, {r, rec + 48, kStrCdName});
    store<u32>(d + 176, load<u32>(rec + 36));
    const f32 zx = load<f32>(rec + 12), zy = load<f32>(rec + 16), zz = load<f32>(rec + 20);
    const f32 xx = load<f32>(rec), xy = load<f32>(rec + 4), xz = load<f32>(rec + 8);
    const f32 yx = Msub(zy, xz, Mul(zz, xy)), yy = Msub(zz, xx, Mul(zx, xz)), yz = Msub(zx, xy, Mul(zy, xx));
    const u32 pos[3] = {load<u32>(rec + 24), load<u32>(rec + 28), load<u32>(rec + 32)};
    const u32 zw[3] = {load<u32>(rec + 12), load<u32>(rec + 16), load<u32>(rec + 20)};
    const u32 xw[3] = {load<u32>(rec), load<u32>(rec + 4), load<u32>(rec + 8)};
    call(ctx, String_Assign, {d + 64, rec + 48});
    store<u32>(d + 96, load<u32>(rec + 40));
    store<u32>(d + 112, xw[0]);
    store<u32>(d + 100, load<u32>(rec + 44));
    store<u32>(d + 116, xw[1]);
    store<u32>(d + 120, xw[2]);
    store<f32>(d + 128, yx);
    store<f32>(d + 132, yy);
    store<f32>(d + 136, yz);
    for (u32 k = 0; k < 3; ++k) store<u32>(d + 144 + 4 * k, zw[k]);
    for (u32 k = 0; k < 3; ++k) store<u32>(d + 160 + 4 * k, pos[k]);
    ret(ctx, 1u);
}

namespace {
constexpr u32 kGiGeom = 208;              // ApeGeomInstance: its geometry record (+112 u8, +132 the class)
constexpr u32 kGiVfnRefresh = 40;         // obj.h kObjVfnSet40: (0, 0)
constexpr u32 kGiRefreshFlag = 0x8000;    // +164 (name?)

// o->vfn40(o, 0, 0): r6 / r7 = the OPD and entry, as the original's call sequence leaves them.
void Vcall40(PPUContext& ctx, u32 o) {
    const u32 opd = load<u32>(load<u32>(o) + kGiVfnRefresh);
    call_ptr(ctx, opd, {o, 0, 0, opd, load<u32>(opd)});
}
}  // namespace

// void GeomInstances_Refresh() (name?)
// For every geometry instance with a geometry (+208): when its class
// (geometry +132) has no LODs (+52 == 0), or it has one LOD, the instance
// is not queued (+260), has +164 0x8000, the geometry's +112 is 1 and the
// class's first LOD (+48 +4) lacks 0x20 at +100: o->vfn40(0, 0).
DECOMP(0058F8C4, GeomInstances_Refresh) {
    const InstList& il = kInstLists[0];
    const u32 alloc = obj6::ListAllocator(ctx, il.hdr, il.vtbl, il.name, il.size);
    const u32 first = static_cast<u32>(call(ctx, Obj_GetField4, {alloc}));
    for (u32 o = first != 0 ? first + kListNode : 0; o != 0;) {
        const u32 g = load<u32>(o + kGiGeom);
        if (g != 0) {
            const u32 cls = load<u32>(g + 132);
            const u32 lods = load<u16>(cls + 52);
            if (lods == 0) {
                Vcall40(ctx, o);
            } else if (load<u32>(o + obj::kObjQueued) == 0 && (load<u32>(o + obj::kObjFlags) & kGiRefreshFlag) != 0) {
                const u32 lod = load<u32>(load<u32>(cls + 48) + 4);
                if (load<u8>(g + 112) == 1 && lods == 1 && ((load<u8>(lod + 100) >> 5) & 1) == 0) Vcall40(ctx, o);
            }
        }
        const u32 next = load<u32>(o - kListNode);
        o = next != 0 ? next + kListNode : 0;
    }
}

namespace {
constexpr u32 PtrVector_Clear = 0x0054EC1C;
constexpr u32 Nop_54EC18 = 0x0054EC18;
constexpr u32 Mem_FreeFrom = 0x0054EE6C;       // (p, 0)
constexpr u32 kBgVector = 5240;                // bank globals: PtrVector* (name?)
constexpr u32 kBankObjA = 0x00F88938, kBankObjB = 0x00F8895C, kBankObjC = 0x00F8896C;  // objects released through vfn +8 (name?)
constexpr u32 kBankObjCFlag = 0x0119509C;      // u8: kBankObjC is live (name?)
constexpr u32 kK13 = 0x01194580;               // obj5.h kK13List: +4 {u16, u16 count}*, +16 a block {p, ...}*, +24, +32 (name?)
constexpr u32 kLightRecsPtr = 0x01194EC0;      // classes.h kLightRecs
constexpr u32 kSlotRecs = 0x01193004;          // obj3.h kSlotTable

void FreeVector(PPUContext& ctx, u32 slot) {
    const u32 v = load<u32>(slot);
    if (v == 0) return;
    call(ctx, PtrVector_Clear, {v});
    call(ctx, Nop_54EC18, {v});
    call(ctx, Mem_FreeFrom, {v, 0});
    store<u32>(slot, 0);
}

void FreeBlock(PPUContext& ctx, u32 slot) {
    const u32 p = load<u32>(slot);
    if (p == 0) return;
    call(ctx, Mem_FreeFrom, {p, 0});
    store<u32>(slot, 0);
}
}  // namespace

// bool Bank_FreeAll() (name?)
// Frees the bank's buffers: the bank globals' +5240 vector, the objects
// at 0x00F88938 / 0x00F8895C (released, the pointers cleared) and, when
// flagged, 0x00F8896C (released only), the type-13 list's block (+16, its
// +0 first) and +32 buffer, its count, the slot records and the dirty
// list. Returns 1.
DECOMP(00588C3C, Bank_FreeAll) {
    FreeVector(ctx, obj15::kBankGlobals + kBgVector);
    for (u32 slot : {kBankObjA, kBankObjB}) {
        const u32 o = load<u32>(slot);
        if (o == 0) continue;
        obj4::Vcall0(ctx, o, 8);
        store<u32>(slot, 0);
    }
    if (load<u8>(kBankObjCFlag) != 0) obj4::Vcall0(ctx, load<u32>(kBankObjC), 8);
    const u32 blk = load<u32>(kK13 + 16);
    store<u32>(kK13 + 24, 0);
    store<u32>(kLightRecsPtr, 0);
    if (blk != 0) {
        FreeBlock(ctx, blk);
        call(ctx, Mem_FreeFrom, {blk, 0});
        store<u32>(kK13 + 16, 0);
    }
    FreeBlock(ctx, kK13 + 32);
    store<u16>(load<u32>(kK13 + 4) + 2, 0);
    FreeBlock(ctx, kSlotRecs);
    FreeVector(ctx, obj::kDirtyList);
    ret(ctx, 1u);
}

namespace {
constexpr u32 FileReader_ReadVec4Swapped = 0x00569AE8;
constexpr u32 Quat_ToMatrix = 0x00553A90;           // (f32 out[4][4], const f32 q[4], const f32 pos[3])
constexpr u32 kStrApNameHash = 0x00B24CCD, kStrApQuat = 0x00B24CD6, kStrApPos = 0x00B24CDB;
constexpr u32 kStrApBoneHash = 0x00B24CDF, kStrApName = 0x00B24CE8, kStrApBoneName = 0x00B24CED;
constexpr u32 kApList = 108;                        // owner: {+4 entries, +8 u16 count}* (80-byte entries)
constexpr u32 kApSkeleton = 164;                    // owner: {u16 +2 bones, Bone* +4}*
constexpr u32 kApEntrySize = 80;
}  // namespace

// bool AttachPoint_Read(IffReader* r, Owner* o) (name?)
// One attach point: a version byte (unused), nameHash, quat, pos,
// boneHash, name and boneName read into a zeroed 100-byte record; the
// owner's next 80-byte entry (+108; none: returns 0) gets +64 the hash,
// the quat's rotation rows (+0 / +16 / +32, xyz) and +48 the position,
// and with a bone hash and a skeleton (+164) +68 the matching bone's
// index (0xFFFF when none matches). Returns 1.
DECOMP(005A96E8, AttachPoint_Read) {
    const u32 r = arg<u32>(ctx, 0), o = arg<u32>(ctx, 1);
    Stack frame(ctx, 208);  // the original's sp+112 .. sp+320
    const u32 q = frame.addr(), m = q + 16, rec = q + 80;
    call(ctx, fn::memset, {rec, 0, 100});
    call(ctx, Iff_ReadU8, {r, rec + 100, 0});
    call(ctx, Iff_ReadU32, {r, rec, kStrApNameHash});
    call(ctx, FileReader_ReadVec4Swapped, {r, rec + 4, kStrApQuat});
    call(ctx, FileReader_ReadVec3Swapped, {r, rec + 20, kStrApPos});
    call(ctx, Iff_ReadU32, {r, rec + 32, kStrApBoneHash});
    call(ctx, FileReader_Read32, {r, rec + 36, kStrApName});
    call(ctx, FileReader_Read32, {r, rec + 68, kStrApBoneName});
    const u32 list = load<u32>(o + kApList);
    const u32 idx = load<u16>(list + 8);
    const u32 e = load<u32>(list + 4) + idx * kApEntrySize;
    store<u16>(list + 8, static_cast<u16>(idx + 1));
    if (e == 0) {
        ret(ctx, 0u);
        return;
    }
    store<u32>(e + 64, load<u32>(rec));
    for (u32 k = 0; k < 4; ++k) store<u32>(q + 4 * k, load<u32>(rec + 4 + 4 * k));  // lfs / stfs: raw
    call(ctx, Quat_ToMatrix, {m, q, cls::kOrigin});
    for (u32 row = 0; row < 3; ++row)
        for (u32 k = 0; k < 3; ++k) store<u32>(e + 16 * row + 4 * k, load<u32>(m + 16 * row + 4 * k));
    for (u32 k = 0; k < 3; ++k) store<u32>(e + 48 + 4 * k, load<u32>(rec + 20 + 4 * k));
    const u32 bone_hash = load<u32>(rec + 32);
    const u32 sk = bone_hash != 0 ? load<u32>(o + kApSkeleton) : 0;
    if (sk != 0) {
        const u32 n = load<u16>(sk + 2);
        u32 found = 0xFFFF;
        u32 b = load<u32>(sk + 4);
        for (u32 j = 0; j < n; ++j, b += kBoneSize)
            if (load<u32>(b + kBoneNameHash) == bone_hash) {
                found = j;
                break;
            }
        store<u16>(e + 68, static_cast<u16>(found));
    }
    ret(ctx, 1u);
}

namespace {
constexpr u32 Bank_PreUpdate = 0x00598B3C;       // (o) (name?)
constexpr u32 Workers_Update = 0x00598E60;       // (o, view)
constexpr u32 RenderJobs_Kick = 0x0053C8F8;
constexpr u32 Jobs_SubmitF2C8 = 0x0059AB34;      // (light volume, worker) -> bool
constexpr u32 kPrWork = 0x0118E080;              // +8 the work record (+96 u8 0x80), +20 / +24 its slot range, +32 u8 (name?)
constexpr u32 kPrStream = 0x01192780;            // u8: stream the slots instead (name?)
constexpr u32 kPrStreamEnd = 0x01192700, kPrStreamBase = 0x00F8893C, kPrStreamMax = 0x00F88940;  // (name?)
constexpr u32 kPrSlots = 0x01193004;             // obj3.h kSlotTable (+4 u16 its count)
constexpr u32 kPrViewFlags = 1776;               // View u8: 0x10 kick the render jobs (name?)
constexpr u32 kPrLvLive = 0x0119509C;            // u8: the light volumes are set up (LightVolumes_Init)
constexpr u32 kPrLvLights = 0x01195120, kPrLvBegin = 0x011957E2, kPrLvEnd = 0x011957E0, kPrLvCount = 0x011957E4;
constexpr u32 kPrLvSize = 144;
constexpr u32 kPrWorkerBuf = 0x00F88970;         // u32: the frame's worker blocks (560 bytes each)
constexpr u32 kPrWorkers = 0x01195870;           // u32: how many were filled
constexpr u32 kPrWorkerSize = 560;
}  // namespace

// void Bank_PreRender(Obj* o) (name?)
// Bank_PreUpdate(o); with a work record (0x0118E080 +8): unless it is
// busy (+32) or flagged (+96 0x80), its slot range (+20 / +24) becomes
// the slot records and their count, or (streaming, 0x01192780) the
// stream's base and min((end - base) >> 7, max); then Workers_Update(o,
// view). Kicks the render jobs when the view asks (+1776 0x10). With the
// light volumes live and the view's +256 0x2 and +52 0x4 set: a frame
// buffer of count (0x011957E4) 560-byte worker blocks, filled by
// Jobs_SubmitF2C8 for the light volumes begin .. end (at least one), the
// number filled stored at 0x01195870.
DECOMP(0059AD20, Bank_PreRender) {
    const u32 o = arg<u32>(ctx, 0);
    call(ctx, Bank_PreUpdate, {o});
    const u32 w = kPrWork;
    const u32 rec = load<u32>(w + 8);
    if (rec != 0) {
        if (load<u8>(w + 32) == 0 && ((load<u8>(rec + 96) >> 7) & 1) == 0) {
            if (load<u8>(kPrStream) != 0) {
                const u32 base = load<u32>(kPrStreamBase), max = load<u32>(kPrStreamMax);
                u32 n = (load<u32>(kPrStreamEnd) - base) >> 7;
                if (n > max) n = max;
                store<u32>(w + 20, base);
                store<u32>(w + 24, n);
            } else {
                const u32 slots = load<u32>(kPrSlots);
                const u32 n = load<u16>(kPrSlots + 4);
                store<u32>(w + 20, slots);
                store<u32>(w + 24, n);
            }
        }
        const u32 view = static_cast<u32>(call(ctx, View_GetCurrent));
        call(ctx, Workers_Update, {o, view});
    }
    u32 view = static_cast<u32>(call(ctx, View_GetCurrent));
    if (((load<u8>(view + kPrViewFlags) >> 4) & 1) != 0) call(ctx, RenderJobs_Kick);
    if (load<u8>(kPrLvLive) == 0) return;
    view = static_cast<u32>(call(ctx, View_GetCurrent));
    const bool a = (load<u32>(view + 256) & 2) != 0;
    const bool b = ((load<u8>(view + 52) >> 2) & 1) != 0;
    if (!(a && b)) return;
    const u32 count = load<u16>(kPrLvCount);
    if (!(static_cast<s32>(count) > 0)) return;
    const u32 buf = static_cast<u32>(call(ctx, o17fn::Render_FrameAlloc, {count * kPrWorkerSize}));
    store<u32>(kPrWorkerBuf, buf);
    u32 lv = kPrLvLights + load<u16>(kPrLvBegin) * kPrLvSize;
    const u32 end = kPrLvLights + load<u16>(kPrLvEnd) * kPrLvSize;
    u32 wk = buf;
    do {
        const u32 k = static_cast<u32>(call(ctx, Jobs_SubmitF2C8, {lv, wk}));
        wk += k * kPrWorkerSize;
        lv += kPrLvSize;
    } while (lv < end);
    store<u32>(kPrWorkers, static_cast<u32>(static_cast<s32>(wk - load<u32>(kPrWorkerBuf)) / static_cast<s32>(kPrWorkerSize)));
}

namespace {
constexpr u32 Shader_Find = 0x004FEBD0;          // (u32 hash) -> shader
constexpr u32 Material_Create = 0x004FF43C;      // (shader) -> material
constexpr u32 Array_Init128 = 0x007A7550;        // (array, count, opd, holder)
constexpr u32 kLvDisabled = 0x0119509D;          // u8 (name?)
constexpr u32 kLvShaderHashA = 0x6DC54CB1, kLvShaderHashB = 0x3A45928D;
constexpr u32 kLvShaderA = 0x00F88964, kLvShaderB = 0x00F88968, kLvMaterial = 0x00F8896C;
constexpr u32 kLvConfigOn = 0x010C5A20 + 82;     // u8 g::config: light volumes enabled (name?)
constexpr u32 kLvArray = 0x011950A0, kLvArrayOpd = 0x00F3CAB8;
constexpr u32 kLvIdxA = 0x01195110, kLvIdxB = 0x01195112, kLvIdxC = 0x01195114;  // u16 (name?)
constexpr u32 kLvVertexFmt = 0x0116BE00;         // {u32, u32, u8[3]} attributes, +120 u16 stride, +124 u16 flags, +126 u16 the attribute mask (name?)
constexpr u32 kLvVertexAttrs = 0x00F88920;       // the attribute table it is filled from (ends at +4 == 0)
constexpr u32 kLvBlendSrc = 770, kLvBlendDst = 771, kLvBlendFunc = 514, kLvBlend4 = 0x8006;  // GL SRC_ALPHA / ONE_MINUS_SRC_ALPHA, LEQUAL, FUNC_ADD
}  // namespace

// bool LightVolumes_Init() (name?)
// Once (0x0119509C; 0x0119509D set: 0): finds the two light volume
// shaders and, when both exist and the config enables them, sets up the
// 64-entry array at 0x011950A0, a material for the first shader (its
// state at +104: blend SRC_ALPHA / ONE_MINUS_SRC_ALPHA, func LEQUAL, op
// FUNC_ADD) and the vertex format at 0x0116BE00 from the attribute table
// 0x00F88920. Returns whether the volumes are live.
DECOMP(00586DB4, LightVolumes_Init) {
    if (load<u8>(kLvDisabled) != 0) {
        ret(ctx, 0u);
        return;
    }
    if (load<u8>(kPrLvLive) != 0) {
        ret(ctx, 1u);
        return;
    }
    store<u32>(kLvShaderA, static_cast<u32>(call(ctx, Shader_Find, {kLvShaderHashA})));
    const u32 sb = static_cast<u32>(call(ctx, Shader_Find, {kLvShaderHashB}));
    store<u32>(kLvShaderB, sb);
    const u32 sa = load<u32>(kLvShaderA);
    if ((load<u8>(kLvConfigOn) & (sa != 0 ? 1u : 0u) & (sb != 0 ? 1u : 0u)) == 0) {
        ret(ctx, 0u);
        return;
    }
    const u32 holder = static_cast<u32>(call(ctx, Holder_New550E70));
    call(ctx, Array_Init128, {kLvArray, 64, kLvArrayOpd, holder});
    store<u16>(kLvIdxA, 0);
    store<u16>(kLvIdxC, 0);
    store<u16>(kLvIdxB, 0);
    const u32 m = static_cast<u32>(call(ctx, Material_Create, {load<u32>(kLvShaderA)}));
    store<u32>(kLvMaterial, m);
    const u32 st = m + 104;
    const u32 w8 = (load<u32>(st + 8) & 0x7FFFFFFFu & ~0x7FFF8000u) | ((kLvBlendFunc << 15) & 0x7FFF8000u);
    const u32 w0 = ((load<u32>(st) | 0x80000000u) & ~0x7FFF8000u) | ((kLvBlendSrc << 15) & 0x7FFF8000u);
    store<u32>(st + 8, w8);
    store<u16>(st + 6, static_cast<u16>(kLvBlendDst));
    store<u32>(st, w0);
    store<u16>(st + 4, static_cast<u16>(kLvBlend4));
    const u32 vf = kLvVertexFmt;
    const u32 flags = load<u16>(vf + 124);
    store<u16>(vf + 120, 12);
    store<u16>(vf + 126, 0);
    store<u16>(vf + 124, static_cast<u16>(flags | 0x8000));
    u32 src = kLvVertexAttrs, dst = vf;
    for (;;) {
        const u32 b = load<u32>(src + 4);
        const u32 a = load<u32>(src);
        store<u32>(dst + 4, b);
        store<u32>(dst, a);
        for (u32 k = 8; k < 11; ++k) store<u8>(dst + k, load<u8>(src + k));
        if (b == 0) break;
        const u32 sh = (a + load<u8>(src + 9)) & 63;  // slw
        store<u16>(vf + 126, static_cast<u16>(load<u16>(vf + 126) | (sh < 32 ? (1u << sh) : 0u)));
        src += 12;
        dst += 12;
    }
    store<u8>(kPrLvLive, 1);
    ret(ctx, 1u);
}

namespace {
constexpr u32 Call_00548508 = 0x00548508;          // (surface)
constexpr u32 Sync_00548550 = 0x00548550;          // (surface, 0)
constexpr u32 Bank_DrawPrepare = 0x0058D448;       // (o) (name?)
constexpr u32 Bank_DrawItem = 0x0058D088;          // (o, item) (name?)
constexpr u32 Gcm_ResetColorMaskPolyOffset = 0x0058CF08;  // (o)
constexpr u32 Draw_InvalidateState = 0x0051279C;
constexpr u32 Gcm_SetViewport = 0x005E4068;        // inlined libgcm: (ctx, x, y, w, h; f1 min, f2 max; scale[4], offset[4])
constexpr u32 kGcmCtx = 0x010F10A0;                // u32: CellGcmContextData* (+8 the current pointer)
constexpr u32 kDvConsts = 0x0058D6D0;              // f32 0.5, -0.5, 1, 0
constexpr u32 kDvItemRect = 28;                    // item: u16 x, y, w, h
constexpr u32 kScissorCmd = 0x000808C0;            // NV4097_SET_SCISSOR_HORIZONTAL, 2 words
}  // namespace

// void Bank_DrawViewports(Obj* o) (name?)
// For the work record (0x0118E080: +4 the surface, +8 the record, +12
// u16 the items): the surface begun, Bank_DrawPrepare(o), then per item
// (record +4 +4i) its rectangle (+28: u16 x y w h) as the viewport
// (depth 0..1) and the scissor (written straight into the command
// buffer) and Bank_DrawItem(o, item); then the colour mask / polygon
// offset reset, the render state invalidated, the surface synced and the
// record flagged (+96 0x80).
DECOMP(0058D6E0, Bank_DrawViewports) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 w = kPrWork;
    const u32 surf = load<u32>(w + 4), rec = load<u32>(w + 8);
    call(ctx, Call_00548508, {surf});
    call(ctx, Bank_DrawPrepare, {o});
    if (static_cast<s32>(load<u16>(w + 12)) > 0) {
        const f64 half = load<f32>(kDvConsts), nhalf = load<f32>(kDvConsts + 4);
        const f64 zmax = load<f32>(kDvConsts + 8), zmin = load<f32>(kDvConsts + 12);
        Stack frame(ctx, 32);  // the original's sp+128 (scale) / sp+144 (offset)
        const u32 scale = frame.addr(), offset = scale + 16;
        for (u32 i = 0; static_cast<s32>(i) < static_cast<s32>(load<u16>(w + 12)); i = (i + 1) & 0xFF) {
            const u32 item = load<u32>(rec + 4 + 4 * i);
            const u32 rc = item + kDvItemRect;
            const u32 x = load<u16>(rc), y = load<u16>(rc + 2), wd = load<u16>(rc + 4), ht = load<u16>(rc + 6);
            const f32 fw = static_cast<f32>(static_cast<f64>(static_cast<s32>(wd)));
            const f32 fh = static_cast<f32>(static_cast<f64>(static_cast<s32>(ht)));
            const f32 fx = static_cast<f32>(static_cast<f64>(static_cast<s32>(x)));
            const f32 fy = static_cast<f32>(static_cast<f64>(static_cast<s32>(y)));
            const f32 sx = Mul(fw, half);
            store<u32>(scale + 12, 0);
            store<u32>(offset + 8, 0);
            store<u32>(scale + 8, cls::kOneF);
            store<u32>(offset + 12, 0);
            store<f32>(scale, sx);
            store<f32>(scale + 4, Mul(fh, nhalf));
            store<f32>(offset, Add(fx, sx));
            store<f32>(offset + 4, Madd(fh, half, fy));
            // r8 / r9 hold y as the original leaves them (the floats take those slots).
            callf(ctx, Gcm_SetViewport, {load<u32>(kGcmCtx), x, y, wd, ht, y, y, scale, offset}, {zmin, zmax});
            const u32 g = load<u32>(kGcmCtx);
            const u32 cur = load<u32>(g + 8);
            store<u32>(cur, kScissorCmd);
            store<u32>(cur + 4, (load<u16>(rc + 4) << 16) | load<u16>(rc));
            store<u32>(cur + 8, (load<u16>(rc + 6) << 16) | load<u16>(rc + 2));
            store<u32>(g + 8, cur + 12);
            call(ctx, Bank_DrawItem, {o, item});
        }
    }
    call(ctx, Gcm_ResetColorMaskPolyOffset, {o});
    call(ctx, Draw_InvalidateState);
    call(ctx, Sync_00548550, {surf, 0});
    store<u8>(rec + 96, static_cast<u8>(load<u8>(rec + 96) | 0x80));
}

namespace {
constexpr u32 kStrGdVersion = 0x00B24A6D, kStrGdBboxMin = 0x00B24A75, kStrGdBboxMax = 0x00B24A7D;
constexpr u32 kStrGdNameHash = 0x00B24A85, kStrGdAnimHash = 0x00B24A8E, kStrGdLightGroup = 0x00B24A97;
constexpr u32 kStrGdName = 0x00B24AA2, kStrGdCollisionHash = 0x00B24AA7, kStrGdNearZFadeRange = 0x00B24AB5;
constexpr u32 kStrGdNearZFadeDist = 0x00B24AC4, kStrGdShadowType = 0x00B24AD2, kStrGdShadowExtrusion = 0x00B24ADD;
constexpr u32 kStrGdShadowLod = 0x00B24AF3, kStrGdShadowZCull = 0x00B24AFD, kStrGdMinNearFadeAlpha = 0x00B24B0D;
constexpr u32 kGdRecSize = 92, kGdVersionDefault = 6;
constexpr u32 kMinusOneF = 0xBF800000;
}  // namespace

// bool GeomDef_Read(IffReader* r, GeomDef* d) (name?)
// The geometry definition chunk: version (6 when absent), bboxMin /
// bboxMax, nameHash, animHash, lightGroup, name, then by version
// collisionHash (> 0), nearZFadeRange / nearZFadeDist (>= 2), shadowType
// (>= 3), shadowExtrusionLength (>= 4, else 3), shadowLOD /
// shadowZCullDist (>= 5) and minNearFadeAlpha (>= 6, else -1) into a
// zeroed 92-byte record for GeomDef_FromRecord. Returns 1.
DECOMP(0059B9CC, GeomDef_Read) {
    const u32 r = arg<u32>(ctx, 0), d = arg<u32>(ctx, 1);
    Stack frame(ctx, 96);
    const u32 rec = frame.addr(), ver = rec + kGdRecSize;
    call(ctx, fn::memset, {rec, 0, kGdRecSize});
    store<u8>(ver, kGdVersionDefault);
    call(ctx, Iff_ReadU8, {r, ver, kStrGdVersion});
    call(ctx, FileReader_ReadVec3Swapped, {r, rec, kStrGdBboxMin});
    call(ctx, FileReader_ReadVec3Swapped, {r, rec + 12, kStrGdBboxMax});
    call(ctx, Iff_ReadU32, {r, rec + 24, kStrGdNameHash});
    call(ctx, Iff_ReadU32, {r, rec + 28, kStrGdAnimHash});
    call(ctx, Iff_ReadU32, {r, rec + 32, kStrGdLightGroup});
    call(ctx, FileReader_Read32, {r, rec + 36, kStrGdName});
    if (static_cast<s32>(load<u8>(ver)) > 0) call(ctx, Iff_ReadU32, {r, rec + 68, kStrGdCollisionHash});
    if (static_cast<s32>(load<u8>(ver)) >= 2) {
        call(ctx, FileReader_ReadF32Swapped, {r, rec + 72, kStrGdNearZFadeRange});
        call(ctx, FileReader_ReadF32Swapped, {r, rec + 76, kStrGdNearZFadeDist});
    }
    if (static_cast<s32>(load<u8>(ver)) >= 3) call(ctx, Iff_ReadU8, {r, rec + 80, kStrGdShadowType});
    if (static_cast<s32>(load<u8>(ver)) >= 4)
        call(ctx, Iff_ReadU8, {r, rec + 81, kStrGdShadowExtrusion});
    else
        store<u8>(rec + 81, 3);
    if (static_cast<s32>(load<u8>(ver)) >= 5) {
        call(ctx, Iff_ReadU8, {r, rec + 82, kStrGdShadowLod});
        call(ctx, FileReader_ReadF32Swapped, {r, rec + 84, kStrGdShadowZCull});
    }
    if (static_cast<s32>(load<u8>(ver)) >= 6)
        call(ctx, FileReader_ReadF32Swapped, {r, rec + 88, kStrGdMinNearFadeAlpha});
    else
        store<u32>(rec + 88, kMinusOneF);
    call(ctx, 0x00585480u /* GeomDef_FromRecord */, {d, rec});
    ret(ctx, 1u);
}

namespace {
constexpr u32 kScVfnRun = 44;              // o->vfn44(o, records, n) (name?)
constexpr u32 kScRecSize = 128;
}  // namespace

// u32 Slots_CopyForJob(Obj* o, Obj** objs, u32 n) (name?)
// Copies each object's 128-byte slot record (+284; with no slot the copy
// reads address 0, as the original) into a frame buffer and returns
// o->vfn44(o, buffer, n).
DECOMP(0059A49C, Slots_CopyForJob) {
    const u32 o = arg<u32>(ctx, 0), objs = arg<u32>(ctx, 1), n = arg<u32>(ctx, 2);
    const u32 buf = static_cast<u32>(call(ctx, o17fn::Render_FrameAlloc, {n << 7}));
    for (u32 i = 0; i < n; ++i) {
        const u32 slot = load<u16>(load<u32>(objs + 4 * i) + 284);
        const u32 src = slot != 0xFFFF ? load<u32>(kSlotRecs) + (slot << 7) : 0;
        const u32 dst = buf + kScRecSize * i;
        for (u32 k = 0; k < 64; k += 16) obj8::VStore(dst + k, obj8::VLoad(src + k));
        for (u32 k = 64; k < 108; k += 4) store<u32>(dst + k, load<u32>(src + k));  // lfs / stfs / lwz / stw: raw
        store<u16>(dst + 108, load<u16>(src + 108));
        store<u16>(dst + 110, load<u16>(src + 110));
        for (u32 k = 112; k < 124; k += 4) store<u32>(dst + k, load<u32>(src + k));
        store<u16>(dst + 124, load<u16>(src + 124));
        store<u8>(dst + 126, load<u8>(src + 126));
        store<u8>(dst + 127, load<u8>(src + 127));
    }
    const u32 opd = load<u32>(load<u32>(o) + kScVfnRun);
    // r6 / r7 = the OPD and entry, as the original's call sequence leaves them.
    call_ptr(ctx, opd, {o, buf, n, opd, load<u32>(opd)});
}

namespace {
constexpr u32 Volume_SetFromBox = 0x00576F28;      // (volume, const Box*, const f32 q[4], const f32 origin[3])
constexpr u32 String_HashNoCase = 0x0056A710;
constexpr u32 String_Copy = 0x00553684;            // (dst, src, size)
constexpr u32 kStrZrVersion = 0x00B24DBB, kStrZrRegionId = 0x00B24DC3, kStrZrVisMask = 0x00B24DCC;
constexpr u32 kStrZrMinPoint = 0x00B24DDB, kStrZrMaxPoint = 0x00B24DE4, kStrZrRotation = 0x00B24DED;
constexpr u32 kStrZrName = 0x00B24DF6, kStrZrNameHash = 0x00B24DFB;
constexpr u32 kBgZones = 36;                       // bank globals: the zone set (+16 u8 count, +17 u8 id shift, +18 u8, +32 192-byte zones)
constexpr u32 kZoneSize = 192;
}  // namespace

// bool ZoneRegion_Read(IffReader* r) (name?)
// One zone region: version (1 when absent), regionID, visRegionsMask,
// minPoint, maxPoint, rotation and (version >= 1) name / nameHash into a
// zeroed record. The id and mask are shifted by the zone set's id shift
// (+17), the region's own bit is added, and the set's next 192-byte zone
// gets +0 the id, +1 the set's +18, +4 the mask, its volume from the box
// and rotation (Volume_SetFromBox about the bank origin), +144 the name
// and +176 the name hash (String_HashNoCase, then replaced by the
// file's). Returns 1.
DECOMP(0059B018, ZoneRegion_Read) {
    const u32 r = arg<u32>(ctx, 0);
    Stack frame(ctx, 144);  // the original's sp+112 .. sp+256
    const u32 q = frame.addr(), box = q + 16, rec = q + 48, ver = rec + 84;
    call(ctx, fn::memset, {rec, 0, 84});
    store<u8>(ver, 1);
    call(ctx, Iff_ReadU8, {r, ver, kStrZrVersion});
    call(ctx, Iff_ReadU8, {r, rec, kStrZrRegionId});
    call(ctx, Iff_ReadU32, {r, rec + 4, kStrZrVisMask});
    call(ctx, FileReader_ReadVec3Swapped, {r, rec + 8, kStrZrMinPoint});
    call(ctx, FileReader_ReadVec3Swapped, {r, rec + 20, kStrZrMaxPoint});
    call(ctx, FileReader_ReadVec4Swapped, {r, rec + 32, kStrZrRotation});
    if (!(static_cast<s32>(load<u8>(ver)) < 1)) {
        call(ctx, FileReader_Read32, {r, rec + 48, kStrZrName});
        call(ctx, Iff_ReadU32, {r, rec + 80, kStrZrNameHash});
    }
    const u32 bg = obj15::kBankGlobals;
    const u32 z = load<u32>(bg + kBgZones);
    const u32 sh = load<u8>(z + 17);
    if (sh != 0) {
        const u32 m = load<u32>(rec + 4);
        store<u8>(rec, static_cast<u8>(load<u8>(rec) + sh));
        store<u32>(rec + 4, (sh & 32) ? 0u : (m << (sh & 31)));  // slw
    }
    const u32 id = load<u8>(rec);
    const u32 mask = load<u32>(rec + 4) | ((id & 32) ? 0u : (1u << (id & 31)));
    store<u32>(rec + 4, mask);
    const u32 n = load<u8>(z + 16);
    store<u8>(z + 16, static_cast<u8>(n + 1));
    const u32 e = z + 32 + n * kZoneSize;
    store<u8>(e + 1, load<u8>(z + 18));
    store<u8>(e, static_cast<u8>(id));
    store<u32>(e + 4, mask);
    for (u32 k = 0; k < 3; ++k) store<u32>(box + 4 * k, load<u32>(rec + 8 + 4 * k));  // lfs / stfs: raw
    for (u32 k = 0; k < 3; ++k) store<u32>(box + 16 + 4 * k, load<u32>(rec + 20 + 4 * k));
    for (u32 k = 0; k < 4; ++k) store<u32>(q + 4 * k, load<u32>(rec + 32 + 4 * k));
    call(ctx, Volume_SetFromBox, {e, box, q, bg + obj15::kBgOrigin});
    store<u32>(e + 176, static_cast<u32>(call(ctx, String_HashNoCase, {rec + 48})));
    call(ctx, String_Copy, {e + 144, rec + 48, 32});
    store<u32>(e + 176, load<u32>(rec + 80));
    ret(ctx, 1u);
}

namespace {
constexpr u32 kCfgLights = 0x010C5A20;     // g::config: +176 f32[3] ambient, +192 f32[3] sun colour, +208 f32[3] sun direction (name?)
constexpr u32 kLiActive = 292;             // ApeLightInstance: nonzero = placed (name?)
constexpr u32 kLiDef = 208;                // its ApeLightDef: +112 f32[3] colour, +136 f32 intensity
}  // namespace

// void Lights_CollectGlobal() (name?)
// Resets the config's global light (ambient black, sun colour (1, 0, 0)?
// as stored: +192 = 1, +208 = 1) and takes them from the placed light
// instances flagged 0x8000: a type-0 light's colour * intensity^2 as the
// ambient, a type-1 light's as the sun colour and its forward row (+112)
// as the sun direction.
DECOMP(00590DA0, Lights_CollectGlobal) {
    const u32 c = kCfgLights;
    store<u32>(c + 176, 0);
    store<u32>(c + 180, 0);
    store<u32>(c + 184, 0);
    store<u32>(c + 192, cls::kOneF);
    store<u32>(c + 196, 0);
    store<u32>(c + 200, 0);
    store<u32>(c + 208, cls::kOneF);
    store<u32>(c + 212, 0);
    store<u32>(c + 216, 0);
    const InstList& il = kInstLists[1];
    const u32 alloc = obj6::ListAllocator(ctx, il.hdr, il.vtbl, il.name, il.size);
    const u32 first = static_cast<u32>(call(ctx, Obj_GetField4, {alloc}));
    for (u32 o = first != 0 ? first + kListNode : 0; o != 0;) {
        if (load<u32>(o + kLiActive) != 0 && (load<u32>(o + obj::kObjFlags) & 0x8000) != 0) {
            const s32 t = load<s32>(o + cls::kLightType);
            if (t == 0 || t == 1) {
                const u32 d = load<u32>(o + kLiDef);
                const f32 i = load<f32>(d + 136);
                const f32 s = Mul(i, i);
                const u32 dst = t == 0 ? c + 176 : c + 192;
                const f32 r = Mul(load<f32>(d + 112), s), g = Mul(load<f32>(d + 116), s), b = Mul(load<f32>(d + 120), s);
                store<f32>(dst, r);
                store<f32>(dst + 4, g);
                store<f32>(dst + 8, b);
                if (t == 1)
                    for (u32 k = 0; k < 3; ++k) store<u32>(c + 208 + 4 * k, load<u32>(o + 112 + 4 * k));  // raw
            }
        }
        const u32 next = load<u32>(o - kListNode);
        o = next != 0 ? next + kListNode : 0;
    }
}

namespace {
constexpr u32 Box_FromSphere = 0x00556F10;   // (box, sphere)
constexpr u32 kLdConsts = 0x00585EDC;        // f32 1/255, 0.01, 1e-5, 100
}  // namespace

// void LightDef_FromRecord(LightDef* d, const LightRec* r) (name?)
// Fills a light definition from its file record: the sphere (+48 the
// origin, +60 r +68), the name (+64), r +28 / +32 / +4 to +96..104, +128
// the type byte r +0, the colour r +8..10 / 255 (+112), +136 sqrt(r +12),
// +140 r +72, +132 r +1; with r +1 set +144 r +16 and +148 = +60 =
// max(r +16 + 0.01, r +20), else +144 / +148 / +152 = r +16 / +20 / +24;
// +160 r +76 and +156 r +80 (near zero: config +144 / +148), +156
// clamped to [1e-5, 100]; the box (+16 / +32) around the sphere.
DECOMP(00585EF0, LightDef_FromRecord) {
    const u32 d = arg<u32>(ctx, 0), r = arg<u32>(ctx, 1);
    const u32 c = kLdConsts;
    store<u32>(d + 48, 0);
    store<u32>(d + 52, 0);
    store<u32>(d + 56, 0);
    store<u32>(d + 60, load<u32>(r + 68));
    call(ctx, String_Assign, {d + 64, r + 36});
    store<u32>(d + 96, load<u32>(r + 28));
    store<u32>(d + 100, load<u32>(r + 32));
    store<u32>(d + 104, load<u32>(r + 4));
    store<u32>(d + 128, load<u8>(r));
    const f64 inv255 = load<f32>(c);
    for (u32 k = 0; k < 3; ++k) store<f32>(d + 112 + 4 * k, Mul(static_cast<f32>(load<u8>(r + 8 + k)), inv255));
    store<f32>(d + 136, static_cast<f32>(std::sqrt(static_cast<f64>(load<f32>(r + 12)))));
    store<u8>(d + 140, load<u8>(r + 72));
    const u32 t = load<u8>(r + 1);
    store<u32>(d + 132, t);
    u32 far_w;
    if (t != 0) {
        const f32 a = load<f32>(r + 16);
        store<f32>(d + 144, a);
        const u32 bw = load<u32>(r + 20);
        const f32 a2 = Add(a, load<f32>(c + 4));
        far_w = a2 > std::bit_cast<f32>(bw) ? std::bit_cast<u32>(a2) : bw;
        store<u32>(d + 60, far_w);
    } else {
        store<u32>(d + 144, load<u32>(r + 16));
        far_w = load<u32>(r + 20);
        store<u32>(d + 152, load<u32>(r + 24));
    }
    store<u32>(d + 148, far_w);
    const f32 eps = load<f32>(c + 8), big = load<f32>(c + 12);
    u32 w160 = load<u32>(r + 76);
    if (std::fabs(std::bit_cast<f32>(w160)) < eps) w160 = load<u32>(kCfgLights + 144);
    f32 v = load<f32>(r + 80);
    if (std::fabs(v) < eps) v = load<f32>(kCfgLights + 148);
    const f32 lo = v > eps ? v : eps;
    store<u32>(d + 160, w160);
    const f32 m = lo < big ? lo : big;
    store<f32>(d + 156, m > eps ? m : eps);
    Stack frame(ctx, 32);
    const u32 box = frame.addr();
    call(ctx, Box_FromSphere, {box, d + 48});
    for (u32 k = 0; k < 3; ++k) store<u32>(d + 16 + 4 * k, load<u32>(box + 4 * k));
    for (u32 k = 0; k < 3; ++k) store<u32>(d + 32 + 4 * k, load<u32>(box + 16 + 4 * k));
}
