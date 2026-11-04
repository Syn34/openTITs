// World_HandleEvent callees, part 6: the spawner object tick and the
// spawner group's random pick (231 / 342 instructions, with a VMX view test).
#include <bit>
#include <cmath>

#include "hevent6.h"

using namespace sly4;
using namespace sly4::hevent;
using namespace sly4::hevent6;

namespace sly4 {
namespace hevent6h {
namespace hfn6 {
constexpr u32 Obj_Ready453350 = 0x00453350;     // (Object*) -> bool
constexpr u32 Obj_Activate452058 = 0x00452058;  // (Object*)
}  // namespace hfn6

// Spawner objects: +8 desc, +12 / +16 f32 timers, +20 children (Object*[]),
// +24 count, +28 max, +32 live, +36 limit, +48 -> +12 attribute values,
// +60 anchor (+128 position), +88 u8 flags (0x40 reset, 0x80 active, 0x10
// only when visible).
constexpr u8 kFlagReset = 0x40, kFlagActive = 0x80, kFlagVisibleOnly = 0x10;
constexpr u32 kChildLive = 0x40000;             // child +172 flag
constexpr u32 kAttrRespawn = 0x010AB1A8;        // u8 attribute (name?)
constexpr u32 kAttrDescBusy = 0x010ABD70;       // s32 attribute of the desc (name?)
constexpr u32 kAttrDelay = 0x010AB0A0;          // f32 attribute: +16 after activating (name?)
constexpr u32 kZero4536B4 = 0x004536B4;         // f32 0.0
constexpr u32 kViewBias = 0x004536B8;           // f32 2.0
constexpr u32 kPaused = 0x00F65BE8;             // u32 -> u8 (name?)
constexpr u32 kWorldViewMain = 0x128168;        // World +: View* (as world.h's kWorldViews)
constexpr u32 kWorldDirector = 0x1281F4;        // World +: object with vfunc +64 (name?)
// View: +368..+464 two 4x4 row sets (rows +368 / +384 / +400 + bias +416 and
// +432 / +448 / +464 + bias +480).

// Spawner groups: +0 -> +12 attribute values, +20 / +24 counts, +32 first
// member (members chained by +72), +36 member count.
constexpr u32 kRandomLast = 0x01164888;         // u64 (as game/update.h)
constexpr u32 kRandomState = 0x01164890;        // u32
constexpr u32 kRandomMix = 0x87654321;
constexpr u32 kAttrPickRange = 0x010ABDB8;      // f32 group attribute: pick radius (name?)
constexpr u32 kAttrGroupMaxA = 0x010ABDA0;      // s32 group attribute vs +24 (name?)
constexpr u32 kAttrGroupMaxB = 0x010ABD88;      // s32 group attribute vs +20 (name?)
constexpr u32 kAttrMemberFlag = 0x00F94B74;     // u8 attribute of a member's +120 owner (name?)
constexpr u32 kOwnerKind = 0x9F641DC8;          // owner +16 object vfunc +12 kind (name?)
constexpr u32 kZero453A5C = 0x00453A5C;         // f32 0.0
constexpr u32 kViewBias453A60 = 0x00453A60;     // f32 2.0
constexpr u32 kPlayerSlot = 4208 + 8;           // World +4216 player (+144 position)
constexpr u32 kViewCamPos = 864;                // View +864 camera position

inline u32 rotl32(u32 v, int n) { return (v << n) | (v >> (32 - n)); }

inline u32 attr_index(u32 desc) { return load<u32>(load<u32>(desc + 16) + 64) << 2; }

// vmaddfp chain: all four lanes of rows * p + bias + k are >= 0.
bool lanes_nonneg(u32 view, u32 r0, u32 bias, f32 px, f32 py, f32 pz, f32 k) {
    bool all = true;
    for (u32 i = 0; i < 4; ++i) {
        auto m = [&](u32 off) { return load<f32>(((view + off) & ~15u) + 4 * i); };
        f32 a = std::fma(m(r0), px, m(bias));
        a = std::fma(m(r0 + 16), py, a);
        a = std::fma(m(r0 + 32), pz, a);
        a = k + a;
        all = all && a >= 0.0f;
    }
    return all;
}
}  // namespace hevent6h
}  // namespace sly4

using namespace sly4::hevent6h;

// bool Obj_Tick4536C0(Spawner* o; f32 dt) (name?): a pending reset (+88
// 0x40, cleared) with the respawn attribute kills the live children (+172
// 0x40000: +32 -= 1, vfunc +40(1)) and empties the list. Inactive (no 0x80)
// -> false. Otherwise counts down the +12 then +16 timers; with neither
// running it spawns (Obj_Activate452058, +16 = the delay attribute) unless
// the desc is busy, the list is full (or respawning), the live limit is
// reached, a visible-only spawner's anchor is in view (unless paused), the
// director (World +0x1281F4 vfunc +64) says no, or Obj_Ready453350 fails.
// Always true from there.
DECOMP(004536C0, Obj_Tick4536C0) {
    const u32 o = arg<u32>(ctx, 0);
    const f64 dt = farg(ctx, 0);
    u8 b = load<u8>(o + 88);
    if ((b & kFlagReset) != 0) {
        const u32 vals = load<u32>(load<u32>(o + 48) + 12);
        store<u8>(o + 88, static_cast<u8>(b & ~kFlagReset));
        if (load<u8>(vals + attr_index(kAttrRespawn)) != 0) {
            const s32 n = load<s32>(o + 24);
            store<u32>(o + 24, 0);
            for (s32 i = 0; i < n; ++i) {
                const u32 c = load<u32>(load<u32>(o + 20) + static_cast<u32>(i) * 4);
                if ((load<u32>(c + 172) & kChildLive) == 0) continue;
                store<u32>(o + 32, load<u32>(o + 32) - 1);
                vcall(ctx, c, 40, {c, 1});
            }
        }
    }
    b = load<u8>(o + 88);
    if ((b & kFlagActive) == 0) {
        ret(ctx, 0u);
        return;
    }
    const f64 zero = load<f32>(kZero4536B4);
    const f64 t1 = load<f32>(o + 12);
    if (t1 > zero) {
        const f32 v = static_cast<f32>(t1 - dt);
        store<f32>(o + 12, v);
        ctx.fpr[1] = v;  // f1 as the original leaves it
        ret(ctx, 1u);
        return;
    }
    const f64 t2 = load<f32>(o + 16);
    ctx.fpr[1] = t2;
    if (t2 > zero) {
        const f32 v = static_cast<f32>(t2 - dt);
        store<f32>(o + 16, v);
        ctx.fpr[1] = v;
        ret(ctx, 1u);
        return;
    }
    ret(ctx, 1u);
    const u32 desc = load<u32>(o + 8);
    if (desc != 0 && load<s32>(load<u32>(load<u32>(desc) + 12) + attr_index(kAttrDescBusy)) > 0) return;
    if (!(load<s32>(o + 24) < load<s32>(o + 28)) &&
        load<u8>(load<u32>(load<u32>(o + 48) + 12) + attr_index(kAttrRespawn)) != 0)
        return;
    const s32 limit = load<s32>(o + 36);
    if (limit != 0 && !(load<s32>(o + 32) < limit)) return;
    if ((b & kFlagVisibleOnly) != 0 && load<u8>(load<u32>(kPaused)) == 0) {
        const u32 view = load<u32>(load<u32>(kCurrentWorld) + kWorldViewMain);
        const u32 p = load<u32>(o + 60) + 128;
        const f32 px = load<f32>(p), py = load<f32>(p + 4), pz = load<f32>(p + 8);
        const f32 k = load<f32>(kViewBias);
        if (lanes_nonneg(view, 368, 416, px, py, pz, k) && lanes_nonneg(view, 432, 480, px, py, pz, k)) return;
    }
    const u32 dir = load<u32>(load<u32>(kCurrentWorld) + kWorldDirector);
    if (dir != 0 && static_cast<u32>(vcall(ctx, dir, 64, {dir})) == 0) {
        ret(ctx, 1u);
        return;
    }
    if (static_cast<u32>(call(ctx, hfn6::Obj_Ready453350, {o})) == 0) {
        ret(ctx, 1u);
        return;
    }
    call(ctx, hfn6::Obj_Activate452058, {o});
    const u32 vals = load<u32>(load<u32>(o + 48) + 12);
    const u32 delay = load<u32>(vals + attr_index(kAttrDelay));
    store<u32>(o + 16, delay);  // lfsx / stfs copied as a word
    ctx.fpr[1] = std::bit_cast<f32>(delay);
    ret(ctx, 1u);
}

// Object* Obj_Resolve453A68(Group* g, u8 ignore_range) (name?): picks a
// member to spawn from: steps the random state (0x01164890; 0x01164888 = the
// low 16 bits) and walks the members from index state % count, wrapping once.
// A member qualifies when active (+88 0x80), not full (or not respawning),
// under its live limit, not a visible-only one in view, allowed by the
// director, and (unless ignore_range or the group's radius is <= 0) within
// the radius of the player (or the camera). It is taken when its vfunc +20
// is false, when the group is under both of its limits, when it has no +120
// owner or the owner is not of kind 0x9F641DC8, or by the owner flag rules
// below; 0 when none qualifies. A member count (+36) above the real list
// length can loop forever, as in the original.
DECOMP(00453A68, Obj_Resolve453A68) {
    const u32 g = arg<u32>(ctx, 0);
    const u32 ignore_range = arg<u32>(ctx, 1) & 0xFF;
    const u32 n = load<u32>(g + 36);
    if (n == 0) {
        ret(ctx, 0u);
        return;
    }
    const u32 seed = load<u32>(kRandomState);
    const s32 start = static_cast<s32>(seed - (seed / n) * n);
    const u64 mix = (static_cast<u64>(rotl32(seed, 4)) + rotl32(seed, 16)) ^ 0xFFFFFFFF87654321ull;
    const u32 world = load<u32>(kCurrentWorld);
    u32 cur = load<u32>(g + 32);
    store<u32>(kRandomState, rotl32(static_cast<u32>(mix), 16));
    store<u64>(kRandomLast, static_cast<u32>(mix) & 0xFFFF);
    const u32 player = load<u32>(world + kPlayerSlot);
    const u32 view0 = load<u32>(world + kWorldViewMain);
    const u32 from = player != 0 ? player + 144 : view0 + kViewCamPos;
    const f64 fx = load<f32>(from), fy = load<f32>(from + 4), fz = load<f32>(from + 8);
    const u32 gvals0 = load<u32>(load<u32>(g) + 12);
    f64 range = load<f32>(gvals0 + attr_index(kAttrPickRange));
    const f64 zero = load<f32>(kZero453A5C);
    if (range > zero) range = fmuls(range, range);
    if (cur == 0) {
        ret(ctx, 0u);
        return;
    }
    const f32 k = load<f32>(kViewBias453A60);
    s32 i = 0;
    bool wrapped = false;
    auto take = [&](u32 c) { ret(ctx, c); };
    for (;;) {
        bool skip = false;
        if (((i < start) ? 0 : 1) == (wrapped ? 1 : 0)) {
            if (wrapped) {
                ret(ctx, 0u);
                return;
            }
            skip = true;
        }
        const u8 b = skip ? 0 : load<u8>(cur + 88);
        if (!skip && (b & kFlagActive) == 0) skip = true;
        if (!skip && !(load<s32>(cur + 24) < load<s32>(cur + 28)) &&
            load<u8>(load<u32>(load<u32>(cur + 48) + 12) + attr_index(kAttrRespawn)) != 0)
            skip = true;
        if (!skip) {
            const s32 lim = load<s32>(cur + 36);
            if (lim != 0 && !(load<s32>(cur + 32) < lim)) skip = true;
        }
        if (!skip && (b & kFlagVisibleOnly) != 0 && load<u8>(load<u32>(kPaused)) == 0) {
            const u32 view = load<u32>(load<u32>(kCurrentWorld) + kWorldViewMain);
            const u32 p = load<u32>(cur + 60) + 128;
            const f32 px = load<f32>(p), py = load<f32>(p + 4), pz = load<f32>(p + 8);
            if (lanes_nonneg(view, 368, 416, px, py, pz, k) && lanes_nonneg(view, 432, 480, px, py, pz, k)) skip = true;
        }
        if (!skip) {
            const u32 dir = load<u32>(load<u32>(kCurrentWorld) + kWorldDirector);
            if (dir != 0 && static_cast<u32>(vcall(ctx, dir, 64, {dir})) == 0) skip = true;
        }
        if (!skip) {
            const u32 p = load<u32>(cur + 60) + 128;
            const f64 px = load<f32>(p), py = load<f32>(p + 4), pz = load<f32>(p + 8);
            ctx.fpr[1] = px;  // f1 as the original leaves it
            if (range > zero && ignore_range == 0) {
                const f64 dy = static_cast<f32>(fy - py), dx = static_cast<f32>(fx - px), dz = static_cast<f32>(fz - pz);
                f64 d2 = fmuls(dy, dy);
                d2 = static_cast<f32>(std::fma(dx, dx, d2));
                d2 = static_cast<f32>(std::fma(dz, dz, d2));
                ctx.fpr[1] = d2;  // f1 as the original leaves it
                if (d2 < range) skip = true;
            }
        }
        if (!skip) {
            if (static_cast<u32>(vcall(ctx, cur, 20, {cur})) == 0) return take(cur);
            const u32 gvals = load<u32>(load<u32>(g) + 12);
            if (load<s32>(g + 24) < load<s32>(gvals + attr_index(kAttrGroupMaxA)) &&
                load<s32>(g + 20) < load<s32>(gvals + attr_index(kAttrGroupMaxB)))
                return take(cur);
            const u32 owner = load<u32>(cur + 120);
            if (owner == 0) return take(cur);
            const u32 oo = load<u32>(owner + 16);
            if (static_cast<u32>(vcall(ctx, oo, 12, {oo})) != kOwnerKind) return take(cur);
            const u32 gv = load<u32>(load<u32>(g) + 12);
            const u32 own = load<u32>(cur + 120);
            if (load<s32>(g + 20) < load<s32>(gv + attr_index(kAttrGroupMaxB)) &&
                load<u8>(load<u32>(own + 12) + attr_index(kAttrMemberFlag)) == 0)
                skip = true;
            else if (!(load<s32>(g + 24) < load<s32>(gv + attr_index(kAttrGroupMaxA))))
                return take(cur);
            else if (load<u8>(load<u32>(load<u32>(cur + 120) + 12) + attr_index(kAttrMemberFlag)) == 0)
                return take(cur);
            else
                skip = true;
        }
        // Advance.
        cur = load<u32>(cur + 72);
        ++i;
        if (cur == 0) {
            i = 0;
            wrapped = true;
            cur = load<u32>(g + 32);
        }
        if (cur == 0) {
            ret(ctx, 0u);
            return;
        }
    }
}
