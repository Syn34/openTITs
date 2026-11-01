// Audio_Update: listener and sound emitters, once a frame.
#include "frame.h"

using namespace sly4;
using namespace sly4::frame;

namespace {

// The handle of pool node `index`: index in the high half, its serial low.
// (The node address uses the full 32-bit index, as the original does.)
u32 Audio_NodeHandle(u32 index) {
    const u32 node = field<u32>(kEmitters, kPoolNodes) + index * kNodeSize;
    return (index << 16) | ((load<u32>(node + kNodeSerial) >> 15) & 0xFFFF);
}

// Drops the emitter's entry from kAudioKeyMap, if the entry still refers to
// this emitter's handle.
void Audio_RemoveKey(PPUContext& ctx, u32 key, u32 handle) {
    Stack frame(ctx, 8);
    store<u32>(frame.addr(), key);
    s32 i = static_cast<s32>(call(ctx, afn::SortedMap_Find, {kAudioKeyMap, frame.addr()}));
    // Not found: the map's default entry (+16) is compared instead.
    const u32 entry = i >= 0 ? load<u32>(kAudioKeyMap + 8) + static_cast<u32>(i) * 12 + 4 : kAudioKeyMap + 16;
    const u32 h = load<u32>(entry);
    if ((h >> 16) != (handle >> 16) || ((h ^ handle) & 0xFFFF) != 0) return;

    i = static_cast<s32>(call(ctx, afn::SortedMap_Find, {kAudioKeyMap, frame.addr()}));
    if (i < 0) return;
    auto count = [] { return static_cast<s32>(load<u16>(kAudioKeyMap + 2)); };
    if (count() >= 2 && i < count() - 1) {
        const u32 entries = load<u32>(kAudioKeyMap + 8);
        do {
            const u32 dst = entries + static_cast<u32>(i) * 12;
            for (u32 k = 0; k < 12; k += 4) copy_word(dst + k, dst + 12 + k);
            ++i;
        } while (i < count() - 1);
    }
    store<u16>(kAudioKeyMap + 2, static_cast<u16>(count() - 1));
}

// A finished emitter: uncount it, drop its key, release the focus object
// if this was the focused emitter, and free its pool node.
void Audio_ReleaseEmitter(PPUContext& ctx, u32 emitter, u32 handle) {
    if (ref<u8>(emitter + kEmBucketed) != 0) {
        auto& count = ref<be_s32>(kAudioBucketCounts + (field<u32>(emitter, kEmBucket) << 2));
        count = count - 1;
    }
    if (const u32 key = field<u32>(emitter, kEmKey)) Audio_RemoveKey(ctx, key, handle);

    if ((handle >> 16) == load<u16>(kAudioFocusHandle) && (handle & 0xFFFF) == load<u16>(kAudioFocusHandle + 2)) {
        const u32 obj = load<u32>(kAudioFocusObj);
        if (obj != 0 && load<s32>(obj + 20) > 0) {
            const s32 left = load<s32>(obj + 20) - 1;
            store<s32>(obj + 20, left);
            if (left == 0) {
                const u32 prev = load<u32>(obj + 8);
                if (prev != 0)
                    store<u32>(prev + 4, load<u32>(obj + 4));
                else
                    store<u32>(kAudioFocusList, load<u32>(obj + 4));
                if (const u32 next = load<u32>(obj + 4)) store<u32>(next + 8, load<u32>(obj + 8));
                call(ctx, afn::Pool_Free, {obj, 24});
            }
        }
        store<u32>(kAudioFocusObj, 0);
        store<u32>(kAudioFocusHandle, 0);
    }
    Stack frame(ctx, 8);
    call(ctx, afn::HandlePool_Remove, {frame.addr(), kEmitters, handle});
}

// Stops an emitter that has left its sound's radius (looked up again by
// handle); a virtualisable one is marked to start again when back in range.
void Audio_StopOutOfRange(PPUContext& ctx, u32 handle) {
    if (ref<u8>(kAudioReady) == 0) return;
    const u32 serial = handle & 0xFFFF;
    if (serial == 0) return;
    const u32 index = handle >> 16;
    if (!(static_cast<s32>(index) < load<s32>(kEmitters + kPoolCapacity))) return;
    const u32 node = field<u32>(kEmitters, kPoolNodes) + index * kNodeSize;
    if (((load<u32>(node + kNodeSerial) >> 15) & 0xFFFF) != serial) return;
    if (ref<u8>(node + kNodeSerial) & kNodeFree) return;
    const u32 emitter = node + kNodeData;
    if (ref<u8>(field<u32>(emitter, kEmDef) + kDefVirtualisable) != 0) ref<u8>(emitter + kEmVirtual) = 1;
    call(ctx, afn::Voice_Stop, {field<u32>(emitter, kEmVoice), 1});
}

}  // namespace

// void Audio_Update(Listener* listener, f32 dt)  (name?)
//
// Called with World +0x128230. Notes whether the listener moved, runs the
// audio subsystems, publishes the listener matrix and heading, then walks
// the emitter pool: virtual emitters back in range are started, finished
// ones released, and the emitters of this frame's bucket (frame counter
// mod 2^tuning, at most 8 buckets) get their full update. Emitters beyond
// their sound's radius are stopped.
DECOMP(0044B310, Audio_Update) {
    const u32 listener = arg<u32>(ctx, 0);
    const f64 dt = farg(ctx, 0);
    const u32 pos = listener + kListenerPos;
    u32 still = 0;
    const f32 px = load<f32>(pos), py = load<f32>(pos + 4), pz = load<f32>(pos + 8);
    if (ref<u8>(kAudioListenerInit) == 0) {
        for (u32 k = 0; k < 12; k += 4) copy_word(kAudioLastListenerPos + k, kAudioListenerStartPos + k);
        ref<u8>(kAudioListenerInit) = 1;
    }
    const u32 shift = load<u32>(kTuneUpdateBuckets + kTuningValue) & 63;  // slw
    u32 buckets = shift < 32 ? 1u << shift : 0u;
    if (ref<u8>(kAudioReady) == 0) return;

    if (std::fabs(fsubs(load<f32>(kAudioLastListenerPos), px)) < kStillEpsilon &&
        std::fabs(fsubs(load<f32>(kAudioLastListenerPos + 4), py)) < kStillEpsilon &&
        std::fabs(fsubs(load<f32>(kAudioLastListenerPos + 8), pz)) < kStillEpsilon) {
        still = 1;
        store<f32>(kAudioLastListenerPos, px);
        store<f32>(kAudioLastListenerPos + 4, py);
        store<f32>(kAudioLastListenerPos + 8, pz);
    }

    ctx.fpr[1] = dt;
    call(ctx, afn::sub_00445A40, {kAudioUpdateObj});
    ctx.fpr[1] = dt;
    call(ctx, afn::sub_00447860);  // r3 is left from the call above, as in the original
    ctx.fpr[1] = dt;
    call(ctx, afn::sub_004473C0);

    // Heading of the forward vector (+32) in the x/z plane, in degrees.
    const f64 fx = -static_cast<f64>(load<f32>(listener + 32));
    const f64 fz = -static_cast<f64>(load<f32>(listener + 40));
    const f32 len = fsqrts(fmadds(fz, fz, fmadds(fx, fx, fmuls(0.0f, 0.0f))));
    const f32 inv = fdivs(1.0f, len);
    const f64 angle = callf(ctx, afn::Math_Atan2, {}, {fmuls(fz, inv), fmuls(fx, inv)});
    const f32 heading = fmuls(angle, kRadToDeg);
    for (const u32 off : {0u, 4u, 8u, 16u, 20u, 24u, 32u}) copy_word(kAudioListenerMatrix + off, listener + off);
    store<f32>(kAudioListenerHeading, heading);
    for (const u32 off : {36u, 40u, 48u, 52u, 56u}) copy_word(kAudioListenerMatrix + off, listener + off);
    if (static_cast<s32>(buckets) > static_cast<s32>(kMaxBuckets)) buckets = kMaxBuckets;
    const u32 bucket = load<u32>(kAudioFrame) & (buckets - 1);

    u32 handle = field<u32>(kEmitters, kPoolUsed) != 0 ? Audio_NodeHandle(field<u32>(kEmitters, kPoolFirst)) : 0;
    while ((handle & 0xFFFF) != 0) {
        const u32 index = handle >> 16;
        const u32 emitter = field<u32>(kEmitters, kPoolNodes) + index * kNodeSize + kNodeData;
        const u32 def = field<u32>(emitter, kEmDef);
        u32 next = 0;
        if (static_cast<s32>(index) != field<s32>(kEmitters, kPoolLast).get())
            next = Audio_NodeHandle(load<u32>(emitter - kNodeData + kNodeNext));

        bool active;
        if (ref<u8>(emitter + kEmVirtual) != 0) {
            // Virtual: start again once inside the radius.
            const f32 dy = fsubs(field<f32>(emitter, kEmPosition + 4), load<f32>(pos + 4));
            const f32 dx = fsubs(field<f32>(emitter, kEmPosition), load<f32>(pos));
            const f32 dz = fsubs(field<f32>(emitter, kEmPosition + 8), load<f32>(pos + 8));
            const f32 radius = field<f32>(def, kDefRadius);
            const f32 d2 = fmadds(dz, dz, fmadds(dx, dx, fmuls(dy, dy)));
            active = d2 < fmuls(radius, radius);
            if (active) call(ctx, afn::Emitter_Start, {handle});
        } else if (static_cast<u32>(call(ctx, afn::Voice_IsPlaying, {field<u32>(emitter, kEmVoice)})) != 0) {
            const s32 v = static_cast<s32>(call(ctx, afn::Voice_GetValue, {field<u32>(emitter, kEmVoice)}));
            const s32 m = field<s32>(def, kDefMaxValue);
            field<s32>(def, kDefMaxValue) = v > m ? v : m;
            active = true;
        } else {
            Audio_ReleaseEmitter(ctx, emitter, handle);
            active = false;
        }

        if (active) {
            if (ref<u8>(emitter + kEmBucketed) != 0 && field<u32>(emitter, kEmBucket) == bucket) {
                ctx.fpr[1] = dt;
                call(ctx, afn::Emitter_Update, {emitter, listener, 0, still});  // r5 is not read
                call(ctx, afn::Emitter_Update2, {emitter});
            }
            if (field<f32>(emitter, kEmDistance) > field<f32>(def, kDefRadius)) Audio_StopOutOfRange(ctx, handle);
        }
        handle = next;
    }

    ctx.fpr[1] = dt;
    call(ctx, afn::sub_0044A970);
    const u32 frame_count = load<u32>(kAudioFrame);
    ref<u8>(kAudioClearB68) = 0;
    ref<u8>(kAudioDirty) = 0;
    store<u32>(kAudioFrame, frame_count + 1);
}
