// World_HandleEvent callees, part 10f: the probe timer and the second
// hashed-object flush.
#include <cmath>

#include "hevent10.h"

using namespace sly4;
using namespace sly4::hevent;
using namespace sly4::hevent7;
using namespace sly4::hevent10;

namespace sly4 {
namespace hevent10f {
namespace ffn10 {
constexpr u32 Effect_SetHash4CF988 = 0x004CF988;  // (effect, u32 hash) (as hevent.h sub_004CF988)
constexpr u32 Probe_SetOn39D1D4 = 0x0039D1D4;     // (Probe*, u8 on) (hevent10_b.cpp)
constexpr u32 SortedMap_Remove = 0x00771DD0;      // (map, const u32* key)
constexpr u32 SortedMap_Find771FC4 = 0x00771FC4;  // (map, const u32* key) -> value* (name?)
constexpr u32 SmallAlloc = 0x0055238C;            // (size) -> block or 0
constexpr u32 Obj_Construct364700 = 0x00364700;   // (Object*, def) (name?)
constexpr u32 Obj_Register35BD08 = 0x0035BD08;    // (Object* or 0) (name?)
constexpr u32 Obj_Attach364408 = 0x00364408;      // (Object*, x) (name?)
constexpr u32 Anim_Play = 0x004A5C90;             // (Anim*, clip, 0; f32 blend) (symbols.txt)
constexpr u32 Array_Reserve = 0x00558F3C;         // (Array*, u16 capacity)
}  // namespace ffn10

// Probe timer (as hevent10_b.cpp / hevent10_c.cpp): +316 s32 mode, +340
// effect, +344 f32 delay, +348 f32 active time, +352 f32 pause, +356 flags
// (bit 31 on, bit 27 off when the active time runs out). Parts at +20
// (+28 u16 count, vtable +272 -> effect), children at +0 (+8 u16 count).
constexpr u32 kPrMode = 316, kPrEffect = 340, kPrDelay = 344, kPrActive = 348, kPrPause = 352;
constexpr u32 kPrFlags = 356, kPrOn = 0x80000000u, kPrStayOff = 0x08000000u;
constexpr u32 kPrChildren = 0, kPrChildCount = 8, kPrParts = 20, kPrPartCount = 28;
constexpr u32 kVChildOn = 40, kVPartEffect = 272;
constexpr u32 kHashDelayDone = 0x06FEE56A, kHashActiveDone = 0xCBCB4FE3, kHashPartOff = 0xF13F8BB2;
constexpr u32 kZero39FBCC = 0x0039FBCC, kRandScale = 0x0039FBD0;  // f32 0.0, 1/65535 (code segment)
// The engine's random generator: u64 last value (0..0xFFFF) and u32 state.
constexpr u32 kRandValue = 0x01164888, kRandState = 0x01164890, kRandXor = 0x87654321;
// Pause range tuning: two blocks whose +72 singles give min and max (name?).
constexpr u32 kPauseMin = 0x0106D61C + 72, kPauseMax = 0x0106D668 + 72;

// The second pending hashed object: object, wanted hash, and the def map.
constexpr u32 kFlush2Obj = 0x00F64CC0;    // Object* (name?)
constexpr u32 kFlush2Hash = 0x0106C27C;   // u32 (as hevent9_i.cpp kFlush364980)
constexpr u32 kDefMap = 0x0106C210;       // sorted map hash -> def (name?)
constexpr u32 kObjSize = 416;
// Object: +8 def (+68 handle), +12 handle, +20 effect, +24 parent (vtable
// +40), +28 anim, +160 flags (bit 1: shown), +192 clip, +256 attachment.
// Def +80 flags (bit 4: spawnable).
constexpr u32 kObjDef = 8, kObjHandle = 12, kObjEffect = 20, kObjParent = 24, kObjAnim = 28;
constexpr u32 kObjFlags = 160, kObjClip = 192, kObjAttach2 = 256, kDefHandle = 68, kDefFlags = 80;
constexpr u32 kDefSpawnable = 0x10;
constexpr u32 kHashShow = 0x04296E4C;
constexpr u32 kOne364978 = 0x00364978;  // f32 1.0 (code segment)
// The handle map (hevent.h kHandleMap): +0 u16 capacity, +2 u16 count, +4
// u16 high-water mark, +6 u16 flags (bit 0 unsorted), +8 {key, Object*}[].
constexpr u32 kMapCap = 0, kMapCount = 2, kMapHigh = 4, kMapFlags = 6, kMapEntries = 8;
}  // namespace hevent10f
}  // namespace sly4

using namespace sly4::hevent10f;

namespace {
bool le(f64 a, f64 b) { return a < b || a == b; }
u32 rotl32(u32 x, u32 n) { return (x << n) | (x >> (32 - n)); }
}  // namespace

// void Probe_Update39FBD8(Probe* p, f32 dt) (name?): the timed probe (mode
// 1, active time > 0): the delay (+344) runs out first (then 0, effect
// hash 0x06FEE56A); then the active time and the pause count down. When the
// active time runs out all three are cleared, the effect gets 0xCBCB4FE3,
// the probe is switched (on unless bit 27) and, with bit 27, each part's
// effect gets 0xF13F8BB2. When only the pause runs out, the next pause is
// drawn from the random generator between the tuning min and max, the on
// bit toggles and every child follows (vtable +40, (on, 1)). r3 / f1 as the
// original leaves them.
DECOMP(0039FBD8, Probe_Update39FBD8) {
    const u32 p = arg<u32>(ctx, 0);
    const f64 dt = farg(ctx, 0);
    u64 r3 = load<u32>(p + kPrMode);
    if (static_cast<s32>(r3) != 1) {
        ctx.gpr[3] = r3;
        return;
    }
    const f64 z = lfs_f64(kZero39FBCC);
    r3 = kZero39FBCC;  // the constant's address stays in r3
    f64 f1 = lfs_f64(p + kPrActive);
    if (!(f1 > z)) {
        ctx.fpr[1] = f1;
        ctx.gpr[3] = r3;
        return;
    }
    f64 f2 = static_cast<f32>(lfs_f64(p + kPrDelay) - dt);
    store<f32>(p + kPrDelay, static_cast<f32>(f2));
    if (le(f2, z)) {
        const u32 e = load<u32>(p + kPrEffect);
        store<u32>(p + kPrDelay, 0);
        f2 = z;
        r3 = e;
        if (e != 0) {
            r3 = call(ctx, ffn10::Effect_SetHash4CF988, {e, kHashDelayDone});
            f1 = lfs_f64(p + kPrActive);
            f2 = lfs_f64(p + kPrDelay);
        }
    }
    if (!le(f2, z)) {
        ctx.fpr[1] = f1;
        ctx.gpr[3] = r3;
        return;
    }
    const f32 active = static_cast<f32>(f1 - dt);
    const f32 pause = static_cast<f32>(lfs_f64(p + kPrPause) - dt);
    store<f32>(p + kPrActive, active);
    store<f32>(p + kPrPause, pause);
    ctx.fpr[1] = active;
    if (le(active, z)) {
        const u32 e = load<u32>(p + kPrEffect);
        store<u32>(p + kPrDelay, 0);
        store<u32>(p + kPrActive, 0);
        store<u32>(p + kPrPause, 0);
        if (e != 0) call(ctx, ffn10::Effect_SetHash4CF988, {e, kHashActiveDone});
        call(ctx, ffn10::Probe_SetOn39D1D4, {p, (load<u32>(p + kPrFlags) & kPrStayOff) == 0 ? 1u : 0u});
        if ((load<u32>(p + kPrFlags) & kPrStayOff) == 0) {
            ctx.gpr[3] = 0;
            return;
        }
        u32 i = 0;
        for (; i < load<u16>(p + kPrPartCount); ++i) {
            const u32 part = load<u32>(load<u32>(p + kPrParts) + i * 4);
            if (part == 0) continue;
            const u32 eff = static_cast<u32>(vcall(ctx, part, kVPartEffect, {part}));
            if (eff != 0) call(ctx, ffn10::Effect_SetHash4CF988, {eff, kHashPartOff});
        }
        ctx.gpr[3] = load<u16>(p + kPrPartCount);
        return;
    }
    if (!le(pause, z)) {
        ctx.gpr[3] = r3;
        return;
    }
    // Next pause: min + (max - min) * (last random value / 65535).
    const u64 last = load<u64>(kRandValue);
    const f64 scale = lfs_f64(kRandScale);
    const f64 lo = lfs_f64(kPauseMin), hi = lfs_f64(kPauseMax);
    const u32 st = load<u32>(kRandState);
    const u32 x = (rotl32(st, 4) + rotl32(st, 16)) ^ kRandXor;
    store<u32>(kRandState, rotl32(x, 16));
    store<u64>(kRandValue, x & 0xFFFF);
    const f32 range = static_cast<f32>(hi - lo);
    const f32 rnd = static_cast<f32>(static_cast<f64>(static_cast<s64>(last)));  // fcfid, frsp
    const u32 fl = load<u32>(p + kPrFlags);
    const u32 on = (fl & kPrOn) != 0 ? 0u : 1u;
    store<u32>(p + kPrFlags, (fl & ~kPrOn) | (on << 31));
    const f32 next = static_cast<f32>(std::fma(static_cast<f64>(range), static_cast<f64>(fmuls(rnd, scale)), lo));
    store<f32>(p + kPrPause, next);
    ctx.fpr[1] = next;
    for (u32 i = 0; i < load<u16>(p + kPrChildCount); ++i) {
        const u32 c = load<u32>(load<u32>(p + kPrChildren) + i * 4);
        if (c != 0) vcall(ctx, c, kVChildOn, {c, on, 1u});
    }
    ctx.gpr[3] = load<u16>(p + kPrChildCount);
}

namespace {
u32 handle_of(u32 o) {
    const u32 h = load<u32>(o + kObjHandle);
    if (h != 0) return h;
    const u32 def = load<u32>(o + kObjDef);
    return def != 0 ? load<u32>(def + kDefHandle) : 0;
}
}  // namespace

// u32 Objects_Flush364980() (name?): a pending object (0x00F64CC0) whose
// handle differs from the wanted hash (0x0106C27C) leaves the handle map, is
// released (vtable +4) and forgotten. For a non-zero hash whose def (map
// 0x0106C210) is spawnable (+80 bit 4), a 416-byte object is made from it
// (0x00364700; the slot keeps 0 when the allocation fails) and registered
// (0x0035BD08); a fresh one (+160 above bit 0 clear) gets its parent
// shown (vtable +40, (1, 1)), its attachment applied, flag 2 set, its clip
// played (blend 1.0) and its effect the show hash. Its {handle, object}
// is appended to the handle map (marked unsorted). r3 as the original
// leaves it.
DECOMP(00364980, Objects_Flush364980) {
    u32 o = load<u32>(kFlush2Obj);
    if (o != 0) {
        if (load<u32>(kFlush2Hash) == handle_of(o)) {
            ctx.gpr[3] = o;
            return;
        }
        Stack frame(ctx, 16);
        store<u32>(frame.addr(), handle_of(o));
        call(ctx, ffn10::SortedMap_Remove, {kHandleMap, frame.addr()});
        o = load<u32>(kFlush2Obj);
        if (o != 0) {
            vcall(ctx, o, 4, {o});
            store<u32>(kFlush2Obj, 0);
        }
    }
    if (load<u32>(kFlush2Hash) == 0) {
        ctx.gpr[3] = 0;
        return;
    }
    const u32 vp = static_cast<u32>(call(ctx, ffn10::SortedMap_Find771FC4, {kDefMap, kFlush2Hash}));
    const u32 def = load<u32>(vp);
    if (def == 0) {
        ctx.gpr[3] = vp;
        return;
    }
    if ((load<u32>(def + kDefFlags) & kDefSpawnable) == 0) {
        ctx.gpr[3] = 0;
        return;
    }
    u32 obj = 0;
    const u32 blk = static_cast<u32>(call(ctx, ffn10::SmallAlloc, {kObjSize}));
    if (blk != 0) {
        call(ctx, ffn10::Obj_Construct364700, {blk, def});
        obj = blk;
    }
    store<u32>(kFlush2Obj, obj);
    call(ctx, ffn10::Obj_Register35BD08, {obj});
    o = load<u32>(kFlush2Obj);
    if ((load<u32>(o + kObjFlags) >> 1) == 0) {
        const u32 parent = load<u32>(o + kObjParent);
        if (parent != 0) vcall(ctx, parent, kVChildOn, {parent, 1u, 1u});
        const u32 att = load<u32>(o + kObjAttach2);
        if (att != 0) call(ctx, ffn10::Obj_Attach364408, {o, att});
        store<u32>(o + kObjFlags, (load<u32>(o + kObjFlags) & 1u) | 2u);
        callf(ctx, ffn10::Anim_Play, {load<u32>(o + kObjAnim), load<u32>(o + kObjClip), 0u}, {lfs_f64(kOne364978)});
        const u32 eff = load<u32>(o + kObjEffect);
        if (eff != 0) call(ctx, ffn10::Effect_SetHash4CF988, {eff, kHashShow});
    }
    // Append {handle, object} to the handle map (a null object is read
    // through, as in the original).
    o = load<u32>(kFlush2Obj);
    const u32 key = handle_of(o);
    const u32 m = kHandleMap;
    if (load<u16>(m + kMapCount) == load<u16>(m + kMapCap))
        call(ctx, ffn10::Array_Reserve, {m, (static_cast<u32>(load<u16>(m + kMapCap)) << 1) & 0xFFFEu});
    const u32 c = load<u16>(m + kMapCount);
    const u32 data = load<u32>(m + kMapEntries);
    store<u32>(data + ((c << 3) & 0x7FFF8u), key);
    const u32 c2 = load<u16>(m + kMapCount);
    const u32 n = c2 + 1;
    const u32 high = load<u16>(m + kMapHigh);
    store<u16>(m + kMapCount, static_cast<u16>(n));
    store<u32>(data + c2 * 8 + 4, o);
    const u32 flags = load<u16>(m + kMapFlags) | 1u;
    store<u16>(m + kMapHigh, static_cast<u16>(static_cast<s32>(high) > static_cast<s32>(n & 0xFFFF) ? high : n));
    store<u16>(m + kMapFlags, static_cast<u16>(flags));
    ctx.gpr[3] = flags;
}
