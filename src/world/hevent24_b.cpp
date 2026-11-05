// World_HandleEvent callees, part 24b: level resource loaders under
// World_EnterLevel0 / the level unload.
#include "hevent22.h"

using namespace sly4;
using namespace sly4::hevent;

namespace {
// Nav_LoadGraph106EC: the navigation graph file of a level, loaded whole and
// handed to the navigation middleware (0x00677C7C, "AddResource") with an
// identity transform.
constexpr u32 kStrLevelPath = 0x00AC0088;  // "Levels/%s/%s"
constexpr u32 kStrNavGraph = 0x00AC0098;   // "NavGraph.bfx_res"
constexpr u32 kStrModeRb = 0x00AC00B0;     // "rb"
constexpr u32 kStringFormat = 0x005534D0, kFileExists = 0x0055D78C, kFileOpen = 0x0055BD3C, kMemAlloc = 0x0054ED38;
constexpr u32 kMatToQuat = 0x0067C024;     // Mat_ToQuat67C024 (quat* out, const f32* row0, row1, row2)
constexpr u32 kNavAddResource = 0x00677C7C;  // (data, const Transform*) (navigation middleware) (name?)
constexpr u32 kFileSize = 24, kFileRead = 12, kFileClose = 8;  // File vtable: () -> size, (buf, size, 0), ()
constexpr u32 kOneF = 0x3F800000;
}  // namespace

// void Nav_LoadGraph106EC(void** slot, const char* level) (name?): unless
// *slot is already set, "Levels/<level>/NavGraph.bfx_res" is opened when
// it exists, read whole into Mem_Alloc(size, 0, 16) (stored in *slot even
// when the allocation fails), closed, and given to 0x00677C7C with the
// identity transform {pos 0, quat from the identity 3x3, +28 = 0}.
// r3 as the original leaves it (*slot when already set).
DECOMP(000106EC, Nav_LoadGraph106EC) {
    const u32 slot = arg<u32>(ctx, 0);
    const u32 path = static_cast<u32>(call(ctx, kStringFormat, {kStrLevelPath, arg<u32>(ctx, 1), kStrNavGraph}));
    if (const u32 cur = load<u32>(slot); cur != 0) return ret(ctx, cur);
    if (static_cast<u32>(call(ctx, kFileExists, {path})) == 0) return;
    const u32 f = static_cast<u32>(call(ctx, kFileOpen, {path, kStrModeRb}));
    if (f == 0) return;
    const u32 size = static_cast<u32>(vcall(ctx, f, kFileSize, {f}));
    const u32 buf = static_cast<u32>(call(ctx, kMemAlloc, {size, 0, 16}));
    store<u32>(slot, buf);
    vcall(ctx, f, kFileRead, {f, buf, size, 0});
    vcall(ctx, f, kFileClose, {f});
    const u32 data = load<u32>(slot);
    // Stack (original r1 +112..+180): the 3x3 identity rows at +0 / +12 /
    // +24, the transform {+36 vec3 pos, +48 quat, +64 u32} given to the
    // middleware.
    Stack frame(ctx, 80);
    const u32 s = frame.addr();
    const u32 rows = s, xf = s + 36, quat = s + 48;
    for (u32 k = 0; k < 3; ++k) store<u32>(xf + 4 * k, 0);
    for (u32 k = 0; k < 9; ++k) store<u32>(rows + 4 * k, k % 4 == 0 ? kOneF : 0u);
    call(ctx, kMatToQuat, {quat, rows, rows + 12, rows + 24});
    store<u32>(s + 64, 0);
    call(ctx, kNavAddResource, {data, xf});
}

namespace {
// Obj_ShutdownSlots1A840: as hevent3_c.cpp Obj_ResetSlots1AB3C, with the
// teardown calls and the navigation world (0x00F557EC) released.
constexpr u32 kGlobalF557F4 = 0x00F557F4;  // u32 object: +4 -> u16 +2, u16 +18, +40, +48 (name?)
constexpr u32 kGlobalF557F8 = 0x00F557F8;  // u32 object; +4 passed to 0x004B0E70 (name?)
constexpr u32 kGlobalF6578C = 0x00F6578C;  // u32 track pool (name?)
constexpr u32 kNavWorld = 0x00F557EC;      // u32: +0 resource, +28 s32 count of 56-byte records from +40 (+32 cleared), +32 s32 count of handles at +28708 (name?)
constexpr u32 kBufferSync = 0x00566318, kClipsRelease4B0E70 = 0x004B0E70, kTrackPoolRefresh = 0x00390BAC;
constexpr u32 kNavRelease60EABC = 0x0060EABC;  // (record +40) (navigation middleware) (name?)
constexpr u32 kNavFree637798 = 0x00637798;     // (handle slot) (navigation middleware) (name?)
constexpr u32 kNavRemoveResource = 0x0067738C; // (data) ("RemoveResource", navigation middleware) (name?)
constexpr u32 kMemFreeFrom = 0x0054EE6C;
}  // namespace

// void Obj_ShutdownSlots1A840(Object* o) (name?): the 0x00F557F4 object's
// counters cleared (+4 -> +2, +18) and 0x00566318(q + 40, q->+48),
// 0x004B0E70(*0x00F557F8 + 4), TrackPool_Refresh390BAC(*0x00F6578C); the
// navigation world's records (0x0060EABC(rec + 40), rec +72 = 0) and
// handles (0x00637798) released and their counts cleared, its resource
// removed (0x0067738C, Mem_FreeFrom(p, 0), +0 = 0); then +24 = 0 and the
// 2 x 8 slot pointers at +3732 to the 80-byte blocks at +3808.
DECOMP(0001A840, Obj_ShutdownSlots1A840) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 q = load<u32>(kGlobalF557F4);
    store<u16>(load<u32>(q + 4) + 2, 0);
    store<u16>(q + 18, 0);
    call(ctx, kBufferSync, {q + 40, load<u32>(q + 48)});
    call(ctx, kClipsRelease4B0E70, {load<u32>(kGlobalF557F8) + 4});
    call(ctx, kTrackPoolRefresh, {load<u32>(kGlobalF6578C)});
    const u32 n = load<u32>(kNavWorld);
    if (static_cast<s32>(load<u32>(n + 28)) > 0) {
        for (s32 i = 0; i < static_cast<s32>(load<u32>(n + 28)); ++i) {
            call(ctx, kNavRelease60EABC, {n + 56 * static_cast<u32>(i) + 40});
            store<u32>(n + 56 * static_cast<u32>(i) + 72, 0);
        }
    }
    const s32 nh = static_cast<s32>(load<u32>(n + 32));
    store<u32>(n + 28, 0);
    if (nh > 0) {
        for (s32 i = 0; i < static_cast<s32>(load<u32>(n + 32)); ++i) call(ctx, kNavFree637798, {load<u32>(n + 28708) + 4 * static_cast<u32>(i)});
    }
    const u32 res = load<u32>(n);
    store<u32>(n + 32, 0);
    if (res != 0) {
        call(ctx, kNavRemoveResource, {res});
        call(ctx, kMemFreeFrom, {load<u32>(n), 0});
        store<u32>(n, 0);
    }
    store<u32>(o + 24, 0);
    for (u32 k = 0; k < 2; ++k)
        for (u32 j = 0; j < 8; ++j) store<u32>(o + 3732 + k * 32 + j * 4, o + 3808 + k * 640 + j * 80);
    ctx.gpr[3] = o + 32 + 3756;  // r3 as the original's last slot store leaves it
}

namespace {
// Nav_UpdatePlayer104C8: the navigation middleware's per-frame step and the
// player's "on the graph" test.
constexpr u32 kNavConsts = 0x000104B4;    // f32 {-1.0, 0.5, 1.0, 0.0}
constexpr u32 kNavStep = 0x0067C8B8;      // (-, const Params*; f32 dt) (navigation middleware) (name?)
constexpr u32 kNavQueryInit = 0x00630F94; // (Query*, const vec3* pos, 0, 0x011AD1AC) (name?)
constexpr u32 kNavQueryOk = 0x0062E164;   // (Query*) -> bool (name?)
constexpr u32 kNavQueryPoint = 0x0062D25C;  // (vec3* out, Query*, const vec3* pos, 0) (name?)
constexpr u32 kNavQuerySet = 0x0062D414;  // (dst, Query*) (name?)
constexpr u32 kNavQueryDone = 0x0062E094, kNavQueryFree = 0x0062D08C;  // (Query*) (name?)
constexpr u32 kNavQueryKind = 0x011AD1AC;
constexpr u32 kPlayerFlags = 172, kPlayerOnGround = 0x00040000, kPlayerPos = 144;
}  // namespace

// void Nav_UpdatePlayer104C8(NavState* o; f32 dt) (name?): steps the
// navigation middleware (params {u8 0, 0, 3}); then, with a player whose
// +172 flag 0x00040000 is set, queries the graph at the player's position:
// "near" when the query succeeds and the graph point is within (-1, 0.5)
// in height and less than 1 away horizontally. When that differs from the
// stored state (+20), the +24 timer counts up by dt and the state flips
// once it is above 1; a set state seen again (both non-zero) resets the
// timer and stores the query into +12 (0x0062D414). The query is then
// finished and freed.
DECOMP(000104C8, Nav_UpdatePlayer104C8) {
    const u32 o = arg<u32>(ctx, 0);
    const f64 dt = farg(ctx, 0);
    // Stack (original r1 +112..+160): +0 the query, +8 the step params,
    // +20 the position, +32 the graph point.
    Stack frame(ctx, 48);
    const u32 q = frame.addr(), params = q + 8, pos = q + 20, pt = q + 32;
    store<u32>(params + 4, 0);
    store<u8>(params, 0);
    store<u32>(params + 8, 3);
    callf(ctx, kNavStep, {o, params}, {dt});
    const u32 player = load<u32>(load<u32>(kCurrentWorld) + kUserBlocks + 8);
    if (player == 0 || (load<u32>(player + kPlayerFlags) & kPlayerOnGround) == 0) return ret(ctx, player);  // r3 as the original leaves it
    const f64 px = static_cast<f64>(load<f32>(player + kPlayerPos)), py = static_cast<f64>(load<f32>(player + kPlayerPos + 4)),
              pz = static_cast<f64>(load<f32>(player + kPlayerPos + 8));
    for (u32 k = 0; k < 12; k += 4) store<u32>(pos + k, load<u32>(player + kPlayerPos + k));
    // r7 = the player's position (not read by the callee).
    call(ctx, kNavQueryInit, {q, pos, 0, kNavQueryKind, player + kPlayerPos});
    u32 near = 0;
    if (static_cast<u32>(call(ctx, kNavQueryOk, {q})) != 0) {
        call(ctx, kNavQueryPoint, {pt, q, pos, 0});
        ctx.fpr[1] = static_cast<f64>(load<f32>(pt + 8));  // f1 as the original leaves it
        const f64 dy = static_cast<f32>(static_cast<f64>(load<f32>(pt + 4)) - py);
        if (dy > static_cast<f64>(load<f32>(kNavConsts)) && dy < static_cast<f64>(load<f32>(kNavConsts + 4))) {
            const f64 dx = static_cast<f32>(static_cast<f64>(load<f32>(pt)) - px);
            const f64 dz = static_cast<f32>(static_cast<f64>(load<f32>(pt + 8)) - pz);
            const f64 zero = static_cast<f64>(load<f32>(kNavConsts + 12));
            f64 d2 = fmuls(zero, zero);
            d2 = static_cast<f32>(std::fma(dx, dx, d2));
            d2 = static_cast<f32>(std::fma(dz, dz, d2));
            near = d2 < static_cast<f64>(load<f32>(kNavConsts + 8)) ? 1u : 0u;
            ctx.fpr[1] = d2;
        }
    }
    u32 state = load<u8>(o + 20);
    if (state != near) {
        const f64 t = static_cast<f64>(load<f32>(o + 24));
        ctx.fpr[1] = t;
        if (t > static_cast<f64>(load<f32>(kNavConsts + 8))) {
            state = near;
            store<u8>(o + 20, static_cast<u8>(near));
        } else {
            store<f32>(o + 24, static_cast<f32>(t + dt));
            ctx.fpr[1] = static_cast<f32>(t + dt);
        }
    }
    if (near != 0 && state != 0) {
        store<u32>(o + 24, 0);
        call(ctx, kNavQuerySet, {o + 12, q});
    }
    call(ctx, kNavQueryDone, {q});
    call(ctx, kNavQueryFree, {q});
}

namespace {
// Nav_ClosestPoint147B8: the object's navigation query (+216) and params.
constexpr u32 kNavQueryTowards = 0x006308AC;  // (Query* out, const vec3* target, Query* from, const Params*) (name?)
constexpr u32 kObjPos = 176, kObjNavQuery = 216;
constexpr u32 kObjNavMask = 244, kObjNavRadius = 240;  // vtable slots: () -> u32, () -> f32 (name?)
}  // namespace

// void Nav_ClosestPoint147B8(vec3* out, Object* o, const vec3* target)
// (name?): out = o's position (+176); with the navigation world loaded
// (*0x00F557EC +0), o's graph query (+216) is (re)made at its position when
// it is not valid, then a query from it towards target (params {0, 0xFFFF,
// -1, vtable +244, u8 0, 0, 0, vtable +240 f32}) gives out the graph point
// when it succeeds.
DECOMP(000147B8, Nav_ClosestPoint147B8) {
    const u32 out = arg<u32>(ctx, 0), o = arg<u32>(ctx, 1), target = arg<u32>(ctx, 2);
    for (u32 k = 0; k < 12; k += 4) store<u32>(out + k, load<u32>(o + kObjPos + k));
    ctx.fpr[1] = static_cast<f64>(load<f32>(o + kObjPos + 8));  // f1 as the original leaves it
    if (load<u32>(load<u32>(kNavWorld)) == 0) return ret(ctx, 0u);
    // Stack (original r1 +112..+208): +0 / +8 two queries, +16 the target,
    // +28 the position, +40 the graph point, +52 the params (32 bytes).
    Stack frame(ctx, 96);
    const u32 qa = frame.addr(), qb = qa + 8, tgt = qa + 16, pos = qa + 28, pt = qa + 40, params = qa + 52;
    store<u32>(params + 0, 0);
    store<u32>(params + 4, 0xFFFF);
    store<u32>(params + 8, 0xFFFFFFFFu);
    store<u32>(params + 12, 0xFFFFFFFFu);
    store<u8>(params + 16, 0);
    store<u32>(params + 20, 0);
    store<u32>(params + 24, 0);
    store<u32>(params + 28, 0);
    store<u32>(params + 12, static_cast<u32>(vcall(ctx, o, kObjNavMask, {o})));
    vcall(ctx, o, kObjNavRadius, {o});
    store<f32>(params + 28, static_cast<f32>(ctx.fpr[1]));
    for (u32 k = 0; k < 12; k += 4) store<u32>(tgt + k, load<u32>(target + k));
    ctx.fpr[1] = static_cast<f64>(load<f32>(target + 8));
    const u32 q = o + kObjNavQuery;
    if (static_cast<u32>(call(ctx, kNavQueryOk, {q})) == 0) {
        for (u32 k = 0; k < 12; k += 4) store<u32>(pos + k, load<u32>(o + kObjPos + k));
        ctx.fpr[1] = static_cast<f64>(load<f32>(o + kObjPos + 8));
        call(ctx, kNavQueryInit, {qa, pos, 0, params});
        call(ctx, kNavQuerySet, {q, qa});
        call(ctx, kNavQueryFree, {qa});
    }
    if (static_cast<u32>(call(ctx, kNavQueryOk, {q})) == 0) return ret(ctx, 0u);
    call(ctx, kNavQueryTowards, {qb, tgt, q, params});
    if (static_cast<u32>(call(ctx, kNavQueryOk, {qb})) != 0) {
        call(ctx, kNavQueryPoint, {pt, qb, tgt, 0});
        for (u32 k = 0; k < 12; k += 4) store<u32>(out + k, load<u32>(pt + k));
        ctx.fpr[1] = static_cast<f64>(load<f32>(pt));
        call(ctx, kNavQueryDone, {qb});
    }
    call(ctx, kNavQueryFree, {qb});
}

namespace {
// Obj_InitRing18DD0: the eight ring points of the level start object.
constexpr u32 kRingConsts = 0x00018DB0;    // f32 {0.125, 2 pi}
constexpr u32 kRingCenter = 0x01164B40;    // vec3 (name?)
constexpr u32 kRingAxis = 0x01164B60;      // vec3 rotation axis (name?)
constexpr u32 kRingOffset = 0x01164B70;    // vec3 rotated around the axis (name?)
constexpr u32 kNop390BA8 = 0x00390BA8, kClipsLoad4B229C = 0x004B229C, kMatrixFromAxisAngle = 0x0056A410;
constexpr u32 kRingRadius = 0x44610000;    // f32 900.0
}  // namespace

// u32 Obj_InitRing18DD0(Object* o) (name?): 0x00390BA8(*0x00F6578C) and
// Clips_Load4B229C(*0x00F557F8 + 4); +0 / +4 / +12 / +16 / +28 (u8) / +68
// / +72 / +36 cleared, +44 = 900.0, +48 the center (0x01164B40); the eight
// points +112 + 64 i = center + R(axis, i / 8 turn) * offset (the VMX
// fused row sums); +24 = 0 and the 2 x 8 slot pointers at +3732 (as
// hevent3_a.cpp Obj_InitSlots1AA34). Returns o + 1280 (r3 as the
// original leaves it).
DECOMP(00018DD0, Obj_InitRing18DD0) {
    const u32 o = arg<u32>(ctx, 0);
    call(ctx, kNop390BA8, {load<u32>(kGlobalF6578C)});
    call(ctx, kClipsLoad4B229C, {load<u32>(kGlobalF557F8) + 4});
    for (u32 k : {0u, 4u, 12u, 16u}) store<u32>(o + k, 0);
    store<u8>(o + 28, 0);
    store<u32>(o + 68, 0);
    store<u32>(o + 72, 0);
    store<u32>(o + 44, kRingRadius);
    store<u32>(o + 48, load<u32>(kRingCenter));
    store<u32>(o + 52, load<u32>(kRingCenter + 4));
    store<u32>(o + 36, 0);
    store<u32>(o + 56, load<u32>(kRingCenter + 8));
    const f64 step = static_cast<f64>(load<f32>(kRingConsts)), turn = static_cast<f64>(load<f32>(kRingConsts + 4));
    // Stack (original r1 +112..+192): the matrix at +16 (three 16-byte rows).
    Stack frame(ctx, 80);
    const u32 m = frame.addr() + 16;
    for (u32 i = 0; i < 8; ++i) {
        const f32 x = load<f32>(kRingOffset), y = load<f32>(kRingOffset + 4), z = load<f32>(kRingOffset + 8);
        const f64 angle = fmuls(fmuls(static_cast<f64>(static_cast<f32>(static_cast<f64>(static_cast<s64>(i)))), step), turn);
        callf(ctx, kMatrixFromAxisAngle, {m, kRingAxis}, {angle});
        const u32 dst = o + 64 * i + 112;
        for (u32 j = 0; j < 3; ++j) {
            f32 acc = std::fma(load<f32>(m + 4 * j), x, 0.0f);
            acc = std::fma(load<f32>(m + 16 + 4 * j), y, acc);
            acc = std::fma(load<f32>(m + 32 + 4 * j), z, acc);
            const f32 r = load<f32>(kRingCenter + 4 * j) + acc;
            store<f32>(dst + 4 * j, r);
            ctx.fpr[1] = static_cast<f64>(r);  // f1 as the original's lfs / stfs copy leaves it
        }
    }
    store<u32>(o + 24, 0);
    for (u32 k = 0; k < 2; ++k)
        for (u32 j = 0; j < 8; ++j) store<u32>(o + 3732 + k * 32 + j * 4, o + 3808 + k * 640 + j * 80);
    ret(ctx, o + 1280);
}
