// World_HandleEvent callees, part 11c: the level loader's start / update
// steps and the volume change notification.
#include "hevent11.h"

using namespace sly4;
using namespace sly4::hevent;
using namespace sly4::hevent7;
using namespace sly4::hevent8;
using namespace sly4::hevent9;
using namespace sly4::hevent11;

namespace {
// Level: +130 name (129-byte strings), +408 char[129][] part names (+416 u16
// count), +508 s32 state (1 loading, 2 loaded), +512 main request, +528
// PtrVector* of part requests (each a 16-byte holder, +0 request).
constexpr u32 kLvName = 130, kLvParts = 408, kLvPartCount = 416, kLvPartSize = 129;
constexpr u32 kLvState = 508, kLvMain = 512, kLvRequests = 528;
constexpr s32 kLvLoading = 1, kLvLoaded = 2;
// Request: +32 s32 state; 5 and 6 are finished, 0 idle. Vtable +8: update.
constexpr u32 kReqState = 32, kReqUpdate = 8;
constexpr s32 kReqIdle = 0, kReqDone = 5, kReqDone2 = 6;
constexpr u32 kLevelRelease = 0x002D9A14;    // (Level*) (Level_Release2D9A14)
constexpr u32 kRequestInit = 0x002DA158;     // (holder or request slot, const char* name) (name?)
constexpr u32 kHolderNew = 0x00550E70;       // () -> u32 (name?)
constexpr u32 kMemAlloc = 0x0054ED38;        // (size, flags, align)
constexpr u32 kHolderSize = 16, kHolderAlign = 16;
constexpr u32 kVecSize = 20;
constexpr u16 kVecInitFlags = 4;
constexpr u32 kVecData = 0, kVecCap = 4, kVecFlags = 6, kVecCount = 8, kVecHigh = 10;

// Volume: +16 box {min vec3, max vec3 at +32}, +64 owner (+12 per-mode on
// bytes, u32 slots).
constexpr u32 kVolBox = 16, kVolBoxMax = 32, kVolOwner = 64, kOwnerOn = 12;
constexpr u32 kModeHolder = 0x010AB5A8;      // +16 -> +64 u32 current mode (as Volume_Refresh399758)
constexpr u32 kEmptyMin = 0x00398FF4;        // f32 FLT_MAX: box min x of an empty box
constexpr u32 kIdentity = 0x01164BD0;        // Mat44 identity (rows of 16 bytes)
constexpr u32 kLaserWorld = 0x00F557EC;      // u32 (as Volume_Refresh399758's listener)
constexpr u32 kLaserName = 0x00ADDE9C;       // "laser"
constexpr u32 kBoxCenter = 0x00556F98;       // (box, vec3* out) -> out
constexpr u32 kShapeInit = 0x000110EC;       // (Shape* out, world, const Mat34*, const vec3* extent, name, u16 mask) (name?)
constexpr u32 kVolumeSet = 0x00637D14;       // (Volume*, Shape*) (name?)
constexpr u32 kShapeRelease = 0x00638498;    // (Shape*) (name?)

// PtrVector push (inlined in the original): grows by doubling, then
// stores, bumps the count and keeps the high-water mark.
void vec_push(PPUContext& ctx, u32 v, u32 value) {
    if (load<u16>(v + kVecCount) == load<u16>(v + kVecCap))
        call(ctx, h8fn::PtrVector_Reserve, {v, (static_cast<u32>(load<u16>(v + kVecCap)) << 1) & 0xFFFFFFFEu});
    const u32 n = load<u16>(v + kVecCount);
    store<u32>(load<u32>(v + kVecData) + n * 4, value);
    const u16 n1 = static_cast<u16>(load<u16>(v + kVecCount) + 1);
    const u16 hw = load<u16>(v + kVecHigh);
    store<u16>(v + kVecCount, n1);
    store<u16>(v + kVecHigh, hw > n1 ? hw : n1);
}

bool req_done(s32 s) { return s == kReqDone || s == kReqDone2; }
}  // namespace

// void Level_StartLoad2DA2A4(Level* l) (name?): unless already loaded,
// releases the level, marks it loading, starts the main request for its name
// and one 16-byte holder request per part, pushed on the +528 vector (made
// on first use). A failed holder allocation passes 0 on, and a failed
// vector allocation is used as 0, as in the original. r3 ends as the part
// count.
DECOMP(002DA2A4, Level_StartLoad2DA2A4) {
    const u32 l = arg<u32>(ctx, 0);
    if (load<s32>(l + kLvState) == kLvLoaded) return ret(ctx, static_cast<u32>(kLvLoaded));
    call(ctx, kLevelRelease, {l});
    store<s32>(l + kLvState, kLvLoading);
    call(ctx, kRequestInit, {l + kLvMain, l + kLvName});
    u32 n;
    for (u32 i = 0, off = 0; i < (n = load<u16>(l + kLvPartCount)); ++i, off += kLvPartSize) {
        const u32 h = static_cast<u32>(call(ctx, kMemAlloc, {kHolderSize, 0, kHolderAlign}));
        if (h != 0) {
            store<u32>(h + 0, 0);
            store<u32>(h + 4, static_cast<u32>(call(ctx, kHolderNew, {h})));
            store<u32>(h + 8, 0);
            store<u32>(h + 12, 0);
        }
        call(ctx, kRequestInit, {h, load<u32>(l + kLvParts) + off});
        u32 v = load<u32>(l + kLvRequests);
        if (v == 0) {
            v = static_cast<u32>(call(ctx, h7fn::SmallAlloc, {kVecSize}));
            store<u32>(l + kLvRequests, v);
            if (v != 0) {
                call(ctx, h9fn::PtrVector_Construct, {v});
                store<u16>(v + kVecFlags, kVecInitFlags);
                v = load<u32>(l + kLvRequests);
            }
        }
        vec_push(ctx, v, h);
    }
    ret(ctx, n);
}

// void Level_Update2DA438(Level* l) (name?): while loading, updates the
// main request (vtable +8, (req, 0)) unless it is idle or finished;
// otherwise updates the first part request that is neither and stops. When
// none needed an update, the main request is finished (5) and every part is
// finished (5 or 6; a missing one counts as unfinished), marks the level
// loaded.
DECOMP(002DA438, Level_Update2DA438) {
    const u32 l = arg<u32>(ctx, 0);
    if (load<s32>(l + kLvState) != kLvLoading) return ret(ctx, l);
    const u32 main = load<u32>(l + kLvMain);
    if (main != 0) {
        const s32 s = load<s32>(main + kReqState);
        if (s != kReqIdle && !req_done(s)) {
            vcall(ctx, main, kReqUpdate, {main, 0});
            return;
        }
    }
    bool all = main != 0 && load<s32>(main + kReqState) == kReqDone;
    for (u32 i = 0, off = 0;; ++i, off += 4) {
        const u32 v = load<u32>(l + kLvRequests);
        if (!(i < (v != 0 ? load<u16>(v + kVecCount) : 0u))) break;
        const u32 r = load<u32>(load<u32>(load<u32>(v + kVecData) + off));
        if (r != 0) {
            const s32 t = load<s32>(r + kReqState);
            if (t != kReqIdle && !req_done(t)) {
                vcall(ctx, r, kReqUpdate, {r, 0});
                return;
            }
        }
        all = all && r != 0 && req_done(load<s32>(r + kReqState));
    }
    if (all) store<s32>(l + kLvState, kLvLoaded);
    ret(ctx, l);
}

// u32 Volume_Changed398FF8(Volume* v) (name?): for a volume whose owner is
// on in the current mode and whose box isn't empty, builds a "laser" shape
// (mask 0xFFFF) in world 0x00F557EC at the box centre with the box extent,
// hands it to 0x00637D14 and releases it; returns the release's r3 (the
// early exits leave the original's r3).
DECOMP(00398FF8, Volume_Changed398FF8) {
    const u32 v = arg<u32>(ctx, 0);
    const u32 owner = load<u32>(v + kVolOwner);
    if (owner == 0) return ret(ctx, 0u);
    const u32 mode = load<u32>(load<u32>(kModeHolder + 16) + 64);
    if (load<u8>(load<u32>(owner + kOwnerOn) + ((mode << 2) & 0xFFFFFFFCu)) == 0) return ret(ctx, 0u);
    ctx.fpr[1] = lfs_f64(kEmptyMin);  // left in f1 by the original's compare
    ctx.fpr[2] = lfs_f64(v + kVolBox);
    if (!(ctx.fpr[2] < ctx.fpr[1])) return ret(ctx, kEmptyMin);
    // r1+112 extent, +128 centre out, +144 Mat34 (4 rows of 16 bytes), +208 shape.
    Stack frame(ctx, 112);
    const u32 ext = frame.addr(), centre = ext + 16, mat = ext + 32, shape = ext + 96;
    for (u32 row = 0; row < 4; ++row)
        for (u32 k = 0; k < 3; ++k) store<u32>(mat + row * 16 + k * 4, load<u32>(kIdentity + row * 16 + k * 4));
    const u32 c = static_cast<u32>(call(ctx, kBoxCenter, {v + kVolBox, centre}));
    for (u32 k = 0; k < 3; ++k) {
        const u32 w = load<u32>(c + k * 4);
        if (k < 2) store<u32>(ext + k * 4, w);
        store<u32>(mat + 48 + k * 4, w);  // the translation row
    }
    f64 d[3];
    for (u32 k = 0; k < 3; ++k) {
        d[k] = static_cast<f32>(lfs_f64(v + kVolBoxMax + k * 4) - lfs_f64(v + kVolBox + k * 4));
        store<f32>(ext + k * 4, static_cast<f32>(d[k]));
    }
    // The original calls with the extent still in f1 (x), f3 (y), f2 (z).
    ctx.fpr[1] = d[0];
    ctx.fpr[3] = d[1];
    ctx.fpr[2] = d[2];
    call(ctx, kShapeInit, {shape, load<u32>(kLaserWorld), mat, ext, kLaserName, 0xFFFF});
    call(ctx, kVolumeSet, {v, shape});
    call(ctx, kShapeRelease, {shape});
}
