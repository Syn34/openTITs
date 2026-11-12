// Audio_Update: listener and sound emitters, once a frame.
#include "frame.h"

#include "../audio/audio.h"
#include "../common/common.h"
#include "../math/math.h"

namespace sly4::frame {
using namespace opentit::native;

namespace {

EmitterPool& Pool() { return global<EmitterPool>(kEmitters); }
EmitterNode& Node(u32 index) { return *host<EmitterNode>(Pool().nodes.ea + index * kNodeSize); }

// The handle of pool node `index`: index in the high half, its serial low.
// (The node address uses the full 32-bit index, as the original does.)
u32 Audio_NodeHandle(u32 index) { return (index << 16) | ((Node(index).serial >> 15) & 0xFFFF); }

// Drops the emitter's entry from kAudioKeyMap, if the entry still refers to
// this emitter's handle.
void Audio_RemoveKey(u32 key, u32 handle) {
    AudioKeyMap& map = global<AudioKeyMap>(kAudioKeyMap);
    GuestFrame frame(8);
    be_u32* key_slot = frame.as<be_u32>();
    *key_slot = key;
    auto* keymap = reinterpret_cast<audio::KeyMap*>(&map);
    s32 i = audio::KeyMap_Find(keymap, key_slot);
    // Not found: the map's default entry (+16) is compared instead.
    const u32 h = i >= 0 ? map.entries.get()[i].handle.get() : map.default_handle.get();
    if ((h >> 16) != (handle >> 16) || ((h ^ handle) & 0xFFFF) != 0) return;

    i = audio::KeyMap_Find(keymap, key_slot);
    if (i < 0) return;
    auto count = [&] { return static_cast<s32>(map.count.get()); };
    if (count() >= 2 && i < count() - 1) {
        AudioKeyEntry* entries = map.entries;
        do {
            entries[i] = entries[i + 1];
            ++i;
        } while (i < count() - 1);
    }
    map.count = static_cast<u16>(count() - 1);
}

// A finished emitter: uncount it, drop its key, release the focus object
// if this was the focused emitter, and free its pool node.
void Audio_ReleaseEmitter(Emitter* emitter, u32 handle) {
    if (emitter->bucketed != 0) {
        be_s32& count = host<be_s32>(kAudioBucketCounts)[emitter->bucket];
        count = count - 1;
    }
    if (const u32 key = emitter->key) Audio_RemoveKey(key, handle);

    const be_u16* focus = host<be_u16>(kAudioFocusHandle);
    if ((handle >> 16) == focus[0] && (handle & 0xFFFF) == focus[1]) {
        FocusObj* obj = global<gp<FocusObj>>(kAudioFocusObj);
        if (obj && obj->count > 0) {
            const s32 left = obj->count - 1;
            obj->count = left;
            if (left == 0) {
                if (const u32 prev = obj->prev)
                    host<FocusObj>(prev)->next = obj->next.get();
                else
                    global<be_u32>(kAudioFocusList) = obj->next.get();
                if (const u32 next = obj->next) host<FocusObj>(next)->prev = obj->prev.get();
                common::SmallFree(obj, 24);
            }
        }
        global<be_u32>(kAudioFocusObj) = 0u;
        global<be_u32>(kAudioFocusHandle) = 0u;
    }
    GuestFrame frame(8);
    audio::HandlePool_Remove(frame.as<be_u32>(), reinterpret_cast<audio::HandlePool*>(&Pool()), handle);
}

// Stops an emitter that has left its sound's radius (looked up again by
// handle); a virtualisable one is marked to start again when back in range.
void Audio_StopOutOfRange(u32 handle) {
    if (global<u8>(kAudioReady) == 0) return;
    const u32 serial = handle & 0xFFFF;
    if (serial == 0) return;
    const u32 index = handle >> 16;
    if (!(static_cast<s32>(index) < Pool().capacity)) return;
    EmitterNode& node = Node(index);
    if (((node.serial >> 15) & 0xFFFF) != serial) return;
    if (gref<u8>(guest(&node.serial)) & kNodeFree) return;
    Emitter* emitter = &node.emitter;
    if (emitter->def->virtualisable != 0) emitter->is_virtual = 1;
    audio::Voice_Stop(emitter->voice, 1);
}

}  // namespace

// Called with World +0x128230. Notes whether the listener moved, runs the
// audio subsystems, publishes the listener matrix and heading, then walks
// the emitter pool: virtual emitters back in range are started, finished
// ones released, and the emitters of this frame's bucket (frame counter
// mod 2^tuning, at most 8 buckets) get their full update. Emitters beyond
// their sound's radius are stopped.  (name?)
void Audio_Update(Listener* listener, f32 dt) {
    const be_f32* pos = reinterpret_cast<const be_f32*>(&listener->m[kListenerPos / 4]);
    u32 still = 0;
    const f32 px = pos[0], py = pos[1], pz = pos[2];
    be_u32* last = host<be_u32>(kAudioLastListenerPos);
    if (global<u8>(kAudioListenerInit) == 0) {
        for (u32 k = 0; k < 3; ++k) last[k] = host<be_u32>(kAudioListenerStartPos)[k].get();
        global<u8>(kAudioListenerInit) = 1;
    }
    const u32 shift = gref<be_u32>(kTuneUpdateBuckets + kTuningValue) & 63;  // slw
    u32 buckets = shift < 32 ? 1u << shift : 0u;
    if (global<u8>(kAudioReady) == 0) return;

    be_f32* lastf = reinterpret_cast<be_f32*>(last);
    if (std::fabs(fsubs(lastf[0], px)) < kStillEpsilon && std::fabs(fsubs(lastf[1], py)) < kStillEpsilon &&
        std::fabs(fsubs(lastf[2], pz)) < kStillEpsilon) {
        still = 1;
        lastf[0] = px;
        lastf[1] = py;
        lastf[2] = pz;
    }

    call(afn::sub_00445A40, kAudioUpdateObj, dt);
    call(afn::sub_00447860, dt);
    call(afn::sub_004473C0, dt);

    // Heading of the forward vector (+32) in the x/z plane, in degrees.
    const be_f32* m = reinterpret_cast<const be_f32*>(listener->m);
    const f64 fx = -static_cast<f64>(m[8].get());
    const f64 fz = -static_cast<f64>(m[10].get());
    const f32 len = fsqrts(fmadds(fz, fz, fmadds(fx, fx, fmuls(0.0f, 0.0f))));
    const f32 inv = fdivs(1.0f, len);
    const f64 angle = math::Atan2f(fmuls(fz, inv), fmuls(fx, inv));
    const f32 heading = fmuls(angle, kRadToDeg);
    be_u32* matrix = host<be_u32>(kAudioListenerMatrix);
    for (const u32 k : {0u, 1u, 2u, 4u, 5u, 6u, 8u}) matrix[k] = listener->m[k].get();
    global<be_f32>(kAudioListenerHeading) = heading;
    for (const u32 k : {9u, 10u, 12u, 13u, 14u}) matrix[k] = listener->m[k].get();
    if (static_cast<s32>(buckets) > static_cast<s32>(kMaxBuckets)) buckets = kMaxBuckets;
    const u32 bucket = global<be_u32>(kAudioFrame) & (buckets - 1);

    EmitterPool& pool = Pool();
    u32 handle = pool.used != 0 ? Audio_NodeHandle(static_cast<u32>(pool.first.get())) : 0;
    while ((handle & 0xFFFF) != 0) {
        const u32 index = handle >> 16;
        EmitterNode& node = Node(index);
        Emitter* emitter = &node.emitter;
        SoundDef* def = emitter->def;
        u32 next = 0;
        if (static_cast<s32>(index) != pool.last) next = Audio_NodeHandle(node.next);

        bool active;
        if (emitter->is_virtual != 0) {
            // Virtual: start again once inside the radius.
            const f32 dy = fsubs(emitter->position[1], pos[1]);
            const f32 dx = fsubs(emitter->position[0], pos[0]);
            const f32 dz = fsubs(emitter->position[2], pos[2]);
            const f32 radius = def->radius;
            const f32 d2 = fmadds(dz, dz, fmadds(dx, dx, fmuls(dy, dy)));
            active = d2 < fmuls(radius, radius);
            if (active) call(afn::Emitter_Start, handle);
        } else if (audio::Voice_IsPlaying(emitter->voice) != 0) {
            const s32 v = audio::Voice_GetValue(emitter->voice);
            const s32 mx = def->max_value;
            def->max_value = v > mx ? v : mx;
            active = true;
        } else {
            Audio_ReleaseEmitter(emitter, handle);
            active = false;
        }

        if (active) {
            if (emitter->bucketed != 0 && static_cast<u32>(emitter->bucket.get()) == bucket) {
                call(afn::Emitter_Update, emitter, listener, 0u, still, dt);  // r5 is not read
                call(afn::Emitter_Update2, emitter);
            }
            if (emitter->distance > def->radius) Audio_StopOutOfRange(handle);
        }
        handle = next;
    }

    call(afn::sub_0044A970, dt);
    const u32 frame_count = global<be_u32>(kAudioFrame);
    global<u8>(kAudioClearB68) = 0;
    global<u8>(kAudioDirty) = 0;
    global<be_u32>(kAudioFrame) = frame_count + 1;
}

}  // namespace sly4::frame

NATIVE(0044B310, sly4::frame::Audio_Update)
