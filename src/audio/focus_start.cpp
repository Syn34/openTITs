// Audio_StartFocusSound: starts a sound and gives it an emitter in the
// emitter pool (positioned sounds also get an update bucket).
#include "focus_start.h"
#include "more.h"
#include "snd.h"

namespace sly4::audio {
using namespace opentit::native;

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
constexpr u32 kVfnContains = 316;             // zone vtable: (obj, const f32 pos[3]) -> bool
constexpr u32 kZoneDistance = 432;            // f32

// Local frame, laid out as the original's sp+128 .. sp+436.
struct Frame {
    pad<4> _0;
    be_u32 key;                 // +4 (sp+132)
    container::PtrVector vec;   // +8 (sp+136)
};

f32 k(u32 i) { return host<be_f32>(kConsts)[i]; }
void copyf(be_f32& dst, const be_f32& src) { dst = static_cast<f32>(static_cast<f64>(src.get())); }
u32 node_serial(const EmitterNode& n) { return (n.serial >> 15) & 0xFFFF; }

// The pool's live-handle check, without the audio-ready test.
EmitterNode* Pool_NodeOf(u32 handle) {
    const u32 serial = handle & 0xFFFF;
    if (serial == 0) return nullptr;
    const u32 index = handle >> 16;
    if (!(static_cast<s32>(index) < Pool().capacity)) return nullptr;
    EmitterNode& node = PoolNode(index);
    if (node_serial(node) != serial) return nullptr;
    if (gref<u8>(guest(&node.serial)) & kNodeFree) return nullptr;
    return &node;
}

u32 NextSerial() {
    be_u32& counter = global<be_u32>(kSerialCounter);
    u32 s = counter + 1;
    counter = s;
    if (s > 0xFFFF) {
        s = 1;
        counter = s;
    }
    return s;
}

// Marks a node used and stamps its serial; returns the handle.
void ClearFree(EmitterNode& node) {
    u8& top = gref<u8>(guest(&node.serial));
    top = top & 0x7F;
}
u32 Stamp(EmitterNode& node, u32 index, u32 serial) {
    node.serial = (node.serial & ~0x7FFF8000u) | ((serial << 15) & 0x7FFF8000u);
    return (index << 16) | (serial & 0xFFFF);
}

// Takes the head of the free list into the (empty) used list.
u32 Pool_AllocFirst(s32 capacity) {
    EmitterPool& pool = Pool();
    if (capacity != 1) {
        const u32 next = static_cast<u32>(PoolNode(pool.free_head).next.get());
        PoolNode(next).prev = -1;
    }
    const u32 head = pool.free_head;
    pool.first = head;
    pool.last = head;
    EmitterNode& node = PoolNode(head);
    pool.free_head = static_cast<u32>(node.next.get());
    node.next = -1;
    node.prev = -1;
    ClearFree(node);
    pool.used = pool.used + 1;
    pool.free_count = pool.free_count - 1;
    const u32 serial = NextSerial();
    const u32 index = pool.first;
    return Stamp(PoolNode(index), index, serial);
}

// Takes the head of the free list and links it in after `after` (the
// handle of the last used node); 0 if that handle is stale.
u32 Pool_AllocAfter(u32 after) {
    EmitterPool& pool = Pool();
    if (!Pool_NodeOf(after)) return 0;
    const u32 prev = after >> 16;
    const u32 head = pool.free_head;
    u32 next_free;
    if (pool.free_count == 1) {
        next_free = 0xFFFFFFFFu;
        pool.free_tail = -1;
    } else {
        next_free = static_cast<u32>(PoolNode(head).next.get());
    }
    pool.free_head = next_free;
    ClearFree(PoolNode(head));
    EmitterNode& node = PoolNode(head);
    EmitterNode& prev_node = PoolNode(prev);
    node.prev = static_cast<s32>(prev);
    const bool was_last = static_cast<s32>(prev) == static_cast<s32>(pool.last.get());
    const u32 used = static_cast<u32>(pool.used.get()), free = pool.free_count;
    node.next = was_last ? -1 : prev_node.next.get();
    if (!was_last) PoolNode(static_cast<u32>(prev_node.next.get())).prev = static_cast<s32>(head);
    prev_node.next = static_cast<s32>(head);
    if (was_last) pool.last = head;
    pool.used = static_cast<s32>(used + 1);
    pool.free_count = free - 1;
    return Stamp(node, head, NextSerial());
}

}  // namespace

// void Audio_StartFocusSound(u32* out, SoundDesc* desc, PlayDesc* play)  (name?)
//
// Starts desc's sound with play's settings and sets up an emitter for it;
// *out = the emitter's handle, or 0 when nothing was started (muted,
// outranked by the emitter already holding play's key, out of range, too
// quiet, the pool is full, or the voice did not start).
void Audio_StartFocusSound(be_u32* out, SoundDef* desc, PlayDesc* play) {
    auto fail = [&] { gref(out) = 0u; };
    if (!desc || !play) return fail();

    u32 flags = play->flags;
    const u32 key = play->key;
    const u32 no_key = flags & kPlayNoKey;
    const s32 priority = play->priority;
    const bool keyed = key != 0 && no_key == 0;

    if (gref<u8>(kTuneMuteAll + kTuningValue) != 0) return fail();
    if (gref<u8>(kTuneMuteFlagged + kTuningValue) != 0 && desc->flagged != 0) return fail();

    GuestFrame gf(320);
    Frame& fr = *gf.as<Frame>();
    Block& block = *gf.as<Block>(28);
    VoiceParams& params = *gf.as<VoiceParams>(88);

    // A live emitter already holding the key with at least our priority wins.
    if (keyed) {
        fr.key = key;
        const s32 i = KeyMap_Find(host<KeyMap>(kKeyMap), &fr.key);
        flags = play->flags;
        const u32 entry = i < 0 ? kKeyMap + 16 : global<be_u32>(kKeyMap + 8) + static_cast<u32>(i) * 12 + 4;
        const s32 held = gref<be_s32>(entry + 4);
        if (Emitter_Of(gref<be_u32>(entry)) && !(priority > held)) return fail();
    }

    const u32 bank = gref(desc->bank);
    u32 zone = play->zone;
    if (Pool().used == Pool().capacity) return fail();

    call(ffn::memset, &params, 0u, 220u);
    params.size = 220u;
    const bool positioned = (flags & kPlay3D) != 0 && play->w36 == 0;

    // The 3D block: volume, gain, ... = 1, pitch = 23999, the rest 0.
    for (be_f32* f : {&block.volume, &block.gain, &block.scale, &block.fade, &block.vol1, &block.vol2}) *f = 1.0f;
    for (be_f32* f : {&block.pitch, &block.pitch_target, &block.pitch_cap}) *f = std::bit_cast<f32>(0x46BB7E00u);
    block.pitch_code = 0u;
    block.distance = 0.0f;
    block.pan = 0u;
    block.elevation = 0;
    block.angle = 0u;
    block.fixed_angle = 0;
    if (flags & kPlayHasGain) {
        f64 gain = play->gain.get();
        if (gain < k(1)) gain = k(0);
        const f32 g = fmuls(gain, k(0));
        block.gain = static_cast<f32>(gain);
        block.volume = fmuls(g, k(0));
    }

    u32 doppler = 0;
    if (positioned) {
        doppler = (flags & kPlayDoppler) != 0 ? 1 : 0;
        if (desc->no_cull == 0) {
            const be_f32* lp = host<be_f32>(kListener + 48);
            const f32 dy = fsubs(lp[1], play->position[1]);
            const f32 dx = fsubs(lp[0], play->position[0]);
            const f32 dz = fsubs(lp[2], play->position[2]);
            const f32 r = desc->radius;
            const f32 r2 = fmuls(r, r);
            const f32 d2 = fmadds(dz, dz, fmadds(dx, dx, fmuls(dy, dy)));
            if (d2 > r2) return fail();
        }
        // No zone given: use the nearest type-26 object that accepts the sound.
        if (desc->zones != 0 && zone == 0) {
            container::PtrVector* vec = &fr.vec;
            call(ffn::PtrVector_Construct, vec);
            gref<be_u16>(guest(vec) + 6) = 4;
            Obj_CollectByType(kObjZoneType, vec);
            f64 nearest = k(2);
            for (u32 i = 0; i < gref<be_u16>(guest(vec) + 8); ++i) {
                const u32 obj = gref<be_u32>(gref<be_u32>(guest(vec)) + i * 4);
                if (call_ptr<u32>(vfunc(obj, kVfnContains), obj, play) == 0) continue;
                const f64 d = gref<be_f32>(obj + kZoneDistance).get();
                if (nearest > d) {
                    nearest = d;
                    zone = obj;
                }
            }
            PtrVector_Clear(vec);
            Nop();
        }
        Emitter_Spatialise(&block, host<be_f32>(kListener), play->position, play->velocity, desc, zone, play->shape, 1,
                           static_cast<u8>(doppler));
        if (desc->no_cull == 0 && block.vol1 < gref<be_f32>(kTuneMinVolume + kTuningValue) &&
            block.vol2 < gref<be_f32>(kTuneMinGain + kTuningValue))
            return fail();
    } else {
        block.pan = doppler;
        block.elevation = static_cast<s32>(doppler);
    }

    // Voice parameters.
    Emitter_BuildParams(&params, &block, desc, nullptr);
    const u32 param = play->param;
    const u32 flags2 = params.opts;
    u32 flags1 = params.mask;
    const u32 play_flags = play->flags;
    const u32 id = desc->id;
    if (param != 0) {
        flags1 |= 1;
        params.mask = flags1;
        params.param = param;
    }
    if (no_key != 0) {
        params.opts = flags2 | 2;
        params.mask = flags1 | 0x100000;
    }
    for (u32 i = 0; i < 4; ++i) {
        const u32 bit = 0x10000u << i;
        if ((bit & play_flags) == 0) continue;
        const u32 f = params.mask | bit;
        params.params[i] = play->params[i];
        params.mask = f;
    }

    const u32 voice = static_cast<u32>(snd::Voice_Play(bank, static_cast<s16>(id & 0xFFFF), guest(&params), static_cast<u64>(-2)));
    if (Voice_IsPlaying(voice) == 0) {
        snd::Snd_GetLastError();
        return fail();
    }

    // Allocate the emitter (after the last used node), growing the pool if allowed.
    EmitterPool& pool = Pool();
    s32 used = pool.used;
    s32 capacity = pool.capacity;
    u32 last = 0;
    if (used != 0) {
        const u32 l = pool.last;
        last = ((l & 0xFFFF) << 16) | node_serial(PoolNode(l));
    }
    u32 handle = 0;
    bool alloc = used != capacity;
    if (!alloc && pool.can_grow != 0) {
        HandlePool_Grow(&pool, static_cast<s32>(static_cast<u32>(capacity) << 1));
        used = pool.used;
        capacity = pool.capacity;
        alloc = true;
    }
    if (alloc) handle = used == 0 ? Pool_AllocFirst(capacity) : Pool_AllocAfter(last);

    EmitterNode* node = Pool_NodeOf(handle);
    if (!node) return fail();
    Emitter& e = node->emitter;
    Emitter_Init(&e);
    e.bucketed = positioned ? 1 : 0;
    e.doppler = static_cast<u8>(doppler);
    e.no_key = static_cast<u8>(no_key);

    if (positioned) {
        // Put it in the emptiest of the 2^n (at most 8) update buckets.
        const u32 shift = gref<be_u32>(kTuneUpdateBuckets + kTuningValue) & 0x3F;
        s32 n = shift >= 32 ? 0 : static_cast<s32>(1u << shift);
        if (n > 8) n = 8;
        be_s32* buckets = host<be_s32>(kBucketCounts);
        s32 best = buckets[0];
        u32 bucket = 0;
        for (s32 i = 1; i < n; ++i) {
            const s32 c = buckets[i];
            if (best > c) {
                best = c;
                bucket = static_cast<u32>(i);
            }
        }
        e.bucket = bucket;
        buckets[bucket] = buckets[bucket] + 1;
        copyf(e.fade_cur, block.fade);
        copyf(e.fade_target, block.fade);
        copyf(e.pitch_cur, block.pitch_cap);
        copyf(e.pitch_target, block.pitch_cap);

        // Position and velocity: copied (and flagged) only if they moved.
        const f64 eps = k(3);
        auto same = [&](const be_f32* a, const be_f32* b) {
            return std::fabs(fsubs(a[0], b[0])) < eps && std::fabs(fsubs(a[1], b[1])) < eps &&
                   std::fabs(fsubs(a[2], b[2])) < eps;
        };
        const f32 px = play->position[0], py = play->position[1], pz = play->position[2];
        const f32 vx = play->velocity[0], vy = play->velocity[1], vz = play->velocity[2];
        if (!same(e.position, play->position)) {
            e.position[0] = px;
            e.position[1] = py;
            e.position[2] = pz;
            e.forced = 1;
        }
        if (!same(e.velocity, play->velocity)) {
            e.velocity[0] = vx;
            e.velocity[1] = vy;
            e.velocity[2] = vz;
            e.moving = 1;
        }
    }

    e.voice = voice;
    const u32 copy = call<u32>(afn::memmove, &e.cur, &block, static_cast<u32>(sizeof(Block)));
    call(afn::memmove, &e.base, copy, static_cast<u32>(sizeof(Block)));
    e.def = desc;
    e.zone = zone;
    e.key = key;
    e.shape = play->shape.get();
    e.priority = play->priority.get();
    e.extra = static_cast<u32>(desc->type.get());
    if (keyed) KeyMap_Set(handle, key, priority);
    gref(out) = handle;
}

}  // namespace sly4::audio

NATIVE(00444610, sly4::audio::Audio_StartFocusSound)
