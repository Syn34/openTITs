// Audio_StartFocusSound: starts a sound and gives it an emitter in the
// emitter pool (positioned sounds also get an update bucket).
#include "focus_start.h"

using namespace sly4;
using namespace sly4::audio;

namespace {

constexpr u32 kTuneMuteAll = 0x010B08B0;      // tuning object, u8 at +72: start nothing
constexpr u32 kTuneMuteFlagged = 0x010B0900;  // u8 at +72: refuse sounds whose desc+88 is set
constexpr u32 kTuneUpdateBuckets = 0x010B0B28;  // s32 at +72: log2 of the bucket count
constexpr u32 kTuneMinVolume = 0x010B0A38;    // f32 at +72 (name?)
constexpr u32 kTuneMinGain = 0x010B0A88;      // f32 at +72 (name?)
constexpr u32 kKeyMap = 0x010AFEF8;           // u16 count at +2, entries (12 bytes) at +8, default at +16
constexpr u32 kListener = 0x010B0230;         // f32[16]; position at +48
constexpr u32 kBucketCounts = 0x010B00E0;     // s32[8]
constexpr u32 kSerialCounter = 0x010C29DC;    // u32, the emitter pool's next serial (1..0xFFFF)
constexpr u32 kConsts = 0x004445FC;           // f32 { 1.0, 0.0, FLT_MAX, 1e-5 }
constexpr u32 kObjZoneType = 26;              // object type collected for the nearest zone (name?)

// SoundDesc: +4 -> bank pointer, +40 f32 radius, +80 u8 uses zones,
// +84 u32 sound id (passed as s16), +88 u32, +96 u8 never culled, +100 u32.
constexpr u32 kDescBank = 4, kDescRadius = 40, kDescZones = 80, kDescId = 84, kDescFlagged = 88;
constexpr u32 kDescNoCull = 96, kDescExtra = 100;

// Emitter fields set here (the emitter is pool node + 16).
constexpr u32 kEmVoice = 0, kEmDef = 4, kEmBank = 8, kEmBucketed = 12, kEmDoppler = 14;
constexpr u32 kEmBase = 16, kEmVolume = 76, kEmSpatial = 136, kEmFadeCur = 140, kEmFadeTarget = 144;
constexpr u32 kEmPitchCur = 148, kEmPitchTarget = 152, kEmKey = 156, kEmPriority = 160;
constexpr u32 kEmBucket = 164, kEmExtra = 168, kEmNoKey = 172, kEmPosition = 176, kEmForced = 192;
constexpr u32 kEmVelocity = 208, kEmMoving = 224;
constexpr u32 kBlockSize = 57;  // the 3D block (volume .. +56) copied to +76 and +16

// Local frame, laid out as the original's sp+128 .. sp+436.
constexpr u32 kFrameKey = 4;      // sp+132
constexpr u32 kFrameVec = 8;      // sp+136 PtrVector: +0 data, +6 u16 element size, +8 u16 count
constexpr u32 kFrameBlock = 28;   // sp+156 3D block
constexpr u32 kFrameParams = 88;  // sp+216 voice params (220 bytes)
constexpr u32 kFrameSize = 320;

f64 lf(u32 ea) { return static_cast<f64>(load<f32>(ea)); }
// lfs + stfs: the copy goes through double, as the recompiled code does.
void copyf(u32 dst, u32 src) { store<f32>(dst, static_cast<f32>(lf(src))); }
u32 node_of(u32 index) { return load<u32>(kEmitters + 28) + index * kNodeSize; }
u32 node_serial(u32 node) { return (load<u32>(node + kNodeSerial) >> 15) & 0xFFFF; }

// The pool's live-handle check, without the audio-ready test.
u32 Pool_NodeOf(u32 handle) {
    const u32 serial = handle & 0xFFFF;
    if (serial == 0) return 0;
    const u32 index = handle >> 16;
    if (!(static_cast<s32>(index) < load<s32>(kEmitters + 24))) return 0;
    const u32 node = node_of(index);
    if (node_serial(node) != serial) return 0;
    if (ref<u8>(node + kNodeSerial) & kNodeFree) return 0;
    return node;
}

u32 NextSerial() {
    u32 s = load<u32>(kSerialCounter) + 1;
    store<u32>(kSerialCounter, s);
    if (s > 0xFFFF) {
        s = 1;
        store<u32>(kSerialCounter, s);
    }
    return s;
}

// Marks a node used and stamps its serial; returns the handle.
void ClearFree(u32 node) { ref<u8>(node + kNodeSerial) = ref<u8>(node + kNodeSerial) & 0x7F; }
u32 Stamp(u32 node, u32 index, u32 serial) {
    const u32 word_ea = node + kNodeSerial;
    store<u32>(word_ea, (load<u32>(word_ea) & ~0x7FFF8000u) | ((serial << 15) & 0x7FFF8000u));
    return (index << 16) | (serial & 0xFFFF);
}

// Takes the head of the free list into the (empty) used list.
u32 Pool_AllocFirst(s32 capacity) {
    constexpr u32 pool = kEmitters;
    if (capacity != 1) {
        const u32 next = load<u32>(node_of(load<u32>(pool)) + 4);
        store<s32>(load<u32>(pool + 28) + next * kNodeSize, -1);
    }
    const u32 head = load<u32>(pool);
    store<u32>(pool + 8, head);
    store<u32>(pool + 12, head);
    const u32 node = node_of(head);
    store<u32>(pool, load<u32>(node + 4));
    store<s32>(node + 4, -1);
    store<s32>(node, -1);
    ClearFree(node);
    store<u32>(pool + 16, load<u32>(pool + 16) + 1);
    store<u32>(pool + 20, load<u32>(pool + 20) - 1);
    const u32 serial = NextSerial();
    const u32 index = load<u32>(pool + 8);
    return Stamp(node_of(index), index, serial);
}

// Takes the head of the free list and links it in after `after` (the
// handle of the last used node); 0 if that handle is stale.
u32 Pool_AllocAfter(u32 after) {
    constexpr u32 pool = kEmitters;
    if (Pool_NodeOf(after) == 0) return 0;
    const u32 prev = after >> 16;
    const u32 head = load<u32>(pool);
    u32 next_free;
    if (load<s32>(pool + 20) == 1) {
        next_free = 0xFFFFFFFFu;
        store<s32>(pool + 4, -1);
    } else {
        next_free = load<u32>(node_of(head) + 4);
    }
    store<u32>(pool, next_free);
    ClearFree(node_of(head));
    const u32 nodes = load<u32>(pool + 28);
    const u32 node = nodes + head * kNodeSize, prev_node = nodes + prev * kNodeSize;
    store<u32>(node, prev);
    const bool was_last = static_cast<s32>(prev) == load<s32>(pool + 12);
    const u32 used = load<u32>(pool + 16), free = load<u32>(pool + 20);
    store<u32>(node + 4, was_last ? 0xFFFFFFFFu : load<u32>(prev_node + 4));
    if (!was_last) store<u32>(nodes + load<u32>(prev_node + 4) * kNodeSize, head);
    store<u32>(prev_node + 4, head);
    if (was_last) store<u32>(pool + 12, head);
    store<u32>(pool + 16, used + 1);
    store<u32>(pool + 20, free - 1);
    return Stamp(node, head, NextSerial());
}

}  // namespace

// void Audio_StartFocusSound(u32* out, SoundDesc* desc, PlayDesc* play)  (name?)
//
// Starts desc's sound with play's settings and sets up an emitter for it;
// *out = the emitter's handle, or 0 when nothing was started (muted,
// outranked by the emitter already holding play's key, out of range, too
// quiet, the pool is full, or the voice did not start).
DECOMP(00444610, Audio_StartFocusSound) {
    const u32 out = arg<u32>(ctx, 0), desc = arg<u32>(ctx, 1), play = arg<u32>(ctx, 2);
    auto fail = [&] { store<u32>(out, 0); };
    if (desc == 0 || play == 0) return fail();

    u32 flags = load<u32>(play + kPlayFlags);
    const u32 key = load<u32>(play + kPlayKey);
    const u32 no_key = flags & kPlayNoKey;
    const s32 priority = load<s32>(play + kPlayPriority);
    const bool keyed = key != 0 && no_key == 0;

    if (ref<u8>(kTuneMuteAll + kTuningValue) != 0) return fail();
    if (ref<u8>(kTuneMuteFlagged + kTuningValue) != 0 && load<u32>(desc + kDescFlagged) != 0) return fail();

    Stack frame(ctx, kFrameSize);
    const u32 sp = frame.addr();
    const u32 block = sp + kFrameBlock, params = sp + kFrameParams;

    // A live emitter already holding the key with at least our priority wins.
    if (keyed) {
        store<u32>(sp + kFrameKey, key);
        const s32 i = static_cast<s32>(call(ctx, ffn::KeyMap_Find, {kKeyMap, sp + kFrameKey}));
        flags = load<u32>(play + kPlayFlags);
        const u32 entry = i < 0 ? kKeyMap + 16 : load<u32>(kKeyMap + 8) + static_cast<u32>(i) * 12 + 4;
        const s32 held = load<s32>(entry + 4);
        if (Emitter_FromHandle(load<u32>(entry)) != 0 && !(priority > held)) return fail();
    }

    u32 bank = load<u32>(load<u32>(desc + kDescBank));
    u32 zone = load<u32>(play + kPlayZone);
    if (load<u32>(kEmitters + 16) == load<u32>(kEmitters + 24)) return fail();

    call(ctx, ffn::memset, {params, 0, 220});
    store<u32>(params, 220);
    const bool positioned = (flags & kPlay3D) != 0 && load<u32>(play + 36) == 0;

    // The 3D block: volume, gain, ... = 1, pitch = 23999, the rest 0.
    for (u32 o = 0; o < 24; o += 4) store<f32>(block + o, 1.0f);
    for (u32 o = 24; o < 36; o += 4) store<u32>(block + o, 0x46BB7E00u);  // 23999.0f
    for (u32 o = 36; o < 56; o += 4) store<u32>(block + o, 0);
    ref<u8>(block + 56) = 0;
    if (flags & kPlayHasGain) {
        f64 gain = lf(play + kPlayGain);
        if (gain < lf(kConsts + 4)) gain = lf(kConsts);
        const f32 g = fmuls(gain, lf(kConsts));
        store<f32>(block + 4, static_cast<f32>(gain));
        store<f32>(block, fmuls(g, lf(kConsts)));
    }

    u32 doppler = 0;
    if (positioned) {
        doppler = (flags & kPlayDoppler) != 0 ? 1 : 0;
        if (ref<u8>(desc + kDescNoCull) == 0) {
            const u32 lp = kListener + 48;
            const f32 dy = fsubs(lf(lp + 4), lf(play + 4));
            const f32 dx = fsubs(lf(lp), lf(play));
            const f32 dz = fsubs(lf(lp + 8), lf(play + 8));
            const f32 r = lf(desc + kDescRadius);
            const f32 r2 = fmuls(r, r);
            const f32 d2 = fmadds(dz, dz, fmadds(dx, dx, fmuls(dy, dy)));
            if (d2 > r2) return fail();
        }
        // No zone given: use the nearest type-26 object that accepts the sound.
        if (ref<u8>(desc + kDescZones) != 0 && zone == 0) {
            const u32 vec = sp + kFrameVec;
            call(ctx, ffn::PtrVector_Construct, {vec});
            store<u16>(vec + 6, 4);
            call(ctx, ffn::Obj_CollectByType, {kObjZoneType, vec});
            f64 nearest = lf(kConsts + 8);
            for (u32 i = 0; i < load<u16>(vec + 8); ++i) {
                const u32 obj = load<u32>(load<u32>(vec) + i * 4);
                if (static_cast<u32>(call_ptr(ctx, vfunc(obj, 316), {obj, play})) == 0) continue;
                const f64 d = lf(obj + 432);
                if (nearest > d) {
                    nearest = d;
                    zone = obj;
                }
            }
            call(ctx, ffn::PtrVector_Clear, {vec});
            call(ctx, ffn::PtrVector_Free, {vec});
        }
        call(ctx, afn::Emitter_Spatialise,
             {block, kListener, play, play + 16, desc, zone, load<u32>(play + kPlayShape), 1, doppler});
        if (ref<u8>(desc + kDescNoCull) == 0 && lf(block + 16) < lf(kTuneMinVolume + kTuningValue) &&
            lf(block + 20) < lf(kTuneMinGain + kTuningValue))
            return fail();
    } else {
        store<u32>(block + 44, doppler);
        store<u32>(block + 48, doppler);
    }

    // Voice parameters.
    call(ctx, afn::Emitter_BuildParams, {params, block, desc, 0});
    const u32 param = load<u32>(play + 40);
    const u32 flags2 = load<u32>(params + 8);
    u32 flags1 = load<u32>(params + 4);
    const u32 play_flags = load<u32>(play + kPlayFlags);
    const u32 id = load<u32>(desc + kDescId);
    if (param != 0) {
        flags1 |= 1;
        store<u32>(params + 4, flags1);
        store<u32>(params + 28, param);
    }
    if (no_key != 0) {
        store<u32>(params + 8, flags2 | 2);
        store<u32>(params + 4, flags1 | 0x100000);
    }
    for (u32 i = 0; i < 4; ++i) {
        const u32 bit = 0x10000u << i;
        if ((bit & play_flags) == 0) continue;
        const u32 f = load<u32>(params + 4) | bit;
        ref<u8>(params + 36 + i) = ref<u8>(play + 44 + i);
        store<u32>(params + 4, f);
    }

    const u32 voice = static_cast<u32>(call(
        ctx, afn::Voice_Play, {bank, static_cast<u64>(static_cast<s64>(static_cast<s16>(id & 0xFFFF))), params,
                               static_cast<u64>(static_cast<s64>(-2))}));
    if (static_cast<u32>(call(ctx, afn::Voice_IsPlaying, {voice})) == 0) {
        call(ctx, ffn::Snd_GetLastError);
        return fail();
    }

    // Allocate the emitter (after the last used node), growing the pool if allowed.
    constexpr u32 pool = kEmitters;
    s32 used = load<s32>(pool + 16);
    s32 capacity = load<s32>(pool + 24);
    u32 last = 0;
    if (used != 0) {
        const u32 l = load<u32>(pool + 12);
        last = ((l & 0xFFFF) << 16) | node_serial(node_of(l));
    }
    u32 handle = 0;
    bool alloc = used != capacity;
    if (!alloc && ref<u8>(pool + 32) != 0) {
        call(ctx, ffn::HandlePool_Grow, {pool, static_cast<u64>(static_cast<s64>(static_cast<s32>(static_cast<u32>(capacity) << 1)))});
        used = load<s32>(pool + 16);
        capacity = load<s32>(pool + 24);
        alloc = true;
    }
    if (alloc) handle = used == 0 ? Pool_AllocFirst(capacity) : Pool_AllocAfter(last);

    const u32 node = Pool_NodeOf(handle);
    if (node == 0) return fail();
    const u32 e = node + kNodeData;
    call(ctx, ffn::Emitter_Init, {e});
    ref<u8>(e + kEmBucketed) = positioned ? 1 : 0;
    ref<u8>(e + kEmDoppler) = static_cast<u8>(doppler);
    ref<u8>(e + kEmNoKey) = static_cast<u8>(no_key);

    if (positioned) {
        // Put it in the emptiest of the 2^n (at most 8) update buckets.
        const u32 shift = load<u32>(kTuneUpdateBuckets + kTuningValue) & 0x3F;
        s32 n = shift >= 32 ? 0 : static_cast<s32>(1u << shift);
        if (n > 8) n = 8;
        s32 best = load<s32>(kBucketCounts);
        u32 bucket = 0;
        for (s32 i = 1; i < n; ++i) {
            const s32 c = load<s32>(kBucketCounts + static_cast<u32>(i) * 4);
            if (best > c) {
                best = c;
                bucket = static_cast<u32>(i);
            }
        }
        store<u32>(e + kEmBucket, bucket);
        store<u32>(kBucketCounts + bucket * 4, load<u32>(kBucketCounts + bucket * 4) + 1);
        copyf(e + kEmFadeCur, block + 12);
        copyf(e + kEmFadeTarget, block + 12);
        copyf(e + kEmPitchCur, block + 32);
        copyf(e + kEmPitchTarget, block + 32);

        // Position and velocity: copied (and flagged) only if they moved.
        const f64 eps = lf(kConsts + 12);
        auto same = [&](u32 a, u32 b) {
            return std::fabs(fsubs(lf(e + a), lf(play + b))) < eps &&
                   std::fabs(fsubs(lf(e + a + 4), lf(play + b + 4))) < eps &&
                   std::fabs(fsubs(lf(e + a + 8), lf(play + b + 8))) < eps;
        };
        const f32 px = static_cast<f32>(lf(play)), py = static_cast<f32>(lf(play + 4));
        const f32 pz = static_cast<f32>(lf(play + 8)), vx = static_cast<f32>(lf(play + 16));
        const f32 vy = static_cast<f32>(lf(play + 20)), vz = static_cast<f32>(lf(play + 24));
        if (!same(kEmPosition, 0)) {
            store<f32>(e + kEmPosition, px);
            store<f32>(e + kEmPosition + 4, py);
            store<f32>(e + kEmPosition + 8, pz);
            ref<u8>(e + kEmForced) = 1;
        }
        if (!same(kEmVelocity, 16)) {
            store<f32>(e + kEmVelocity, vx);
            store<f32>(e + kEmVelocity + 4, vy);
            store<f32>(e + kEmVelocity + 8, vz);
            ref<u8>(e + kEmMoving) = 1;
        }
    }

    store<u32>(e + kEmVoice, voice);
    const u32 copy = static_cast<u32>(call(ctx, afn::memmove, {e + kEmVolume, block, kBlockSize}));
    call(ctx, afn::memmove, {e + kEmBase, copy, kBlockSize});
    store<u32>(e + kEmDef, desc);
    store<u32>(e + kEmBank, zone);
    store<u32>(e + kEmKey, key);
    store<u32>(e + kEmSpatial, load<u32>(play + kPlayShape));
    store<u32>(e + kEmPriority, load<u32>(play + kPlayPriority));
    store<u32>(e + kEmExtra, load<u32>(desc + kDescExtra));
    if (keyed) call(ctx, ffn::KeyMap_Set, {handle, key, static_cast<u64>(static_cast<s64>(priority))});
    store<u32>(out, handle);
}
