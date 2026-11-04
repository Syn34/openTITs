// Emitter_Stop and Audio_FocusPlay: stopping an emitter and starting the
// pending "focus" sound (with its mix transitions).
#include "audio.h"

using namespace sly4;
using namespace sly4::audio;

namespace {

constexpr u32 kKeyMap = 0x010AFEF8;       // u16 count at +2, entries (12 bytes) at +8, default at +16
constexpr u32 kKeyMapFind = 0x00784008;   // KeyMap_Find
constexpr u32 kHandlePoolRemove = 0x0078378C;
constexpr u32 kBucketCounts = 0x010B00E0;
constexpr u32 kFocusHandle = 0x010AFEA4;
constexpr u32 kFocusObj = 0x00F65B70, kFocusPending = 0x00F65B74, kFocusList = 0x00F65B7C;
constexpr u32 kEmKey = 156, kEmBucketed = 12, kEmBucket = 164;

// Drops the emitter's key from the key map if it still maps to `handle`.
void KeyMap_Remove(PPUContext& ctx, u32 key, u32 handle) {
    Stack frame(ctx, 8);
    store<u32>(frame.addr(), key);
    s32 i = static_cast<s32>(call(ctx, kKeyMapFind, {kKeyMap, frame.addr()}));
    const u32 entry = i >= 0 ? load<u32>(kKeyMap + 8) + static_cast<u32>(i) * 12 + 4 : kKeyMap + 16;
    const u32 h = load<u32>(entry);
    if ((h >> 16) != (handle >> 16) || ((h ^ handle) & 0xFFFF) != 0) return;
    i = static_cast<s32>(call(ctx, kKeyMapFind, {kKeyMap, frame.addr()}));
    if (i < 0) return;
    auto count = [] { return static_cast<s32>(load<u16>(kKeyMap + 2)); };
    if (count() >= 2 && i < count() - 1) {
        const u32 entries = load<u32>(kKeyMap + 8);
        do {
            const u32 dst = entries + static_cast<u32>(i) * 12;
            for (u32 k = 0; k < 12; k += 4) store<u32>(dst + k, load<u32>(dst + 12 + k));
            ++i;
        } while (i < count() - 1);
    }
    store<u16>(kKeyMap + 2, static_cast<u16>(count() - 1));
}

// Copies a float through an FPR (lfs / stfs), as the recompiled code does.
void copy_float(u32 dst, u32 src) { store<f32>(dst, static_cast<f32>(static_cast<f64>(load<f32>(src)))); }

}  // namespace

// void Emitter_Stop(u32 handle, bool fade)  (name?)
//
// A playing emitter with `fade` gets Voice_Stop(voice, 0) and stays
// allocated (it is released once its voice ends). Otherwise its voice is
// stopped at once (if playing) and the emitter released: the focus object
// if it was the focus, its key, its bucket count and its pool node.
DECOMP(00443424, Emitter_Stop) {
    const u32 handle = arg<u32>(ctx, 0);
    const u32 fade = arg<u32>(ctx, 1) & 0xFF;
    const u32 emitter = Emitter_FromHandle(handle);
    if (emitter == 0) return;

    if (Emitter_FromHandle(handle) != 0 &&
        static_cast<u32>(call(ctx, afn::Voice_IsPlaying, {load<u32>(emitter)})) != 0) {
        if (fade != 0) {
            call(ctx, afn::Voice_Stop, {load<u32>(emitter), 0});
            return;
        }
        call(ctx, afn::Voice_Stop, {load<u32>(emitter), 1});
    }

    if ((handle >> 16) == load<u16>(kFocusHandle) && (handle & 0xFFFF) == load<u16>(kFocusHandle + 2)) {
        const u32 obj = load<u32>(kFocusObj);
        if (obj != 0 && load<s32>(obj + 20) > 0) {
            const s32 left = load<s32>(obj + 20) - 1;
            store<s32>(obj + 20, left);
            if (left == 0) {
                if (const u32 prev = load<u32>(obj + 8))
                    store<u32>(prev + 4, load<u32>(obj + 4));
                else
                    store<u32>(kFocusList, load<u32>(obj + 4));
                if (const u32 next = load<u32>(obj + 4)) store<u32>(next + 8, load<u32>(obj + 8));
                call(ctx, afn::Pool_Free, {obj, 24});
            }
        }
        store<u32>(kFocusObj, 0);
        store<u32>(kFocusHandle, 0);
    }
    if (const u32 key = load<u32>(emitter + kEmKey)) KeyMap_Remove(ctx, key, handle);
    if (ref<u8>(emitter + kEmBucketed) != 0) {
        auto& count = ref<be_s32>(kBucketCounts + (load<u32>(emitter + kEmBucket) << 2));
        count = count - 1;
    }
    Stack frame(ctx, 8);
    call(ctx, kHandlePoolRemove, {frame.addr(), kEmitters, handle});
}

namespace {

constexpr u32 kKeyLookup = 0x007848EC;    // (Map*, const u32* key) -> value slot (name?)
constexpr u32 kFocusStart = 0x00444610;   // (u32* handle_out, Desc*, PlayDesc*) (name?)
constexpr u32 kListenerStart = 0x01164B40;
constexpr u32 kDefaultTransitions = 0x00AE43D8;  // [3][3] x 28 bytes, as kMixTransitions
constexpr u32 kTransitionsKey = 0x86DF6221u;     // hash looked up when the desc's own key has none
constexpr u32 kMixEnabled = 0x010AF1BC, kMixLevels = 0x010AF08C, kMixMode = 0x010AF098;
constexpr u32 kMixCurrent = 0x010AF09C, kMixTarget = 0x010AF0A8, kMixRate = 0x010AF0B4;
constexpr u32 kMixTransitions = 0x010AF0C0;
constexpr u32 kHundredBits = 0x42C80000u;

}  // namespace

// void Audio_FocusPlay()  (name?)
//
// Takes the pending focus object, loads its mix transitions (from its
// description's map, else the defaults), resets the mix to mode 0, and
// starts its sound with the mix values as parameters 1-3. The started
// handle becomes the focus handle.
DECOMP(0044A3D4, Audio_FocusPlay) {
    const u32 pending = load<u32>(kFocusPending);
    const u32 desc = pending != 0 ? load<u32>(pending) : 0;

    // PlayDesc on the stack, at the original's offsets from sp+112.
    Stack frame(ctx, 96);
    const u32 s = frame.addr();
    const u32 key_slot = s + 80, out = s + 84;
    for (u32 k = 0; k < 3; ++k) copy_float(s + k * 4, kListenerStart + k * 4);
    copy_float(s + 16, kListenerStart);
    copy_float(s + 20, kListenerStart + 4);
    store<u32>(s + 64, 0);
    store<u32>(s + 60, 0);
    store<u32>(s + 56, 0);
    store<u32>(s + 48, 0);
    ref<u8>(s + 44) = 0;
    store<u32>(s + 40, 0);
    store<u32>(s + 36, 0);
    store<u32>(s + 32, 0x3F800000u);
    copy_float(s + 24, kListenerStart + 8);
    ref<u8>(s + 45) = 0;
    ref<u8>(s + 46) = 0;
    ref<u8>(s + 47) = 0;
    store<u32>(kFocusObj, load<u32>(kFocusPending));
    store<u32>(kFocusPending, 0);

    u32 table = 0;
    if (desc != 0) {
        if (const u32 info = load<u32>(desc + 4)) {
            table = load<u32>(static_cast<u32>(call(ctx, kKeyLookup, {info + 32, desc})));
            if (table == 0) {
                store<u32>(key_slot, kTransitionsKey);
                table = load<u32>(static_cast<u32>(call(ctx, kKeyLookup, {load<u32>(desc + 4) + 32, key_slot})));
            }
        }
    }
    if (table == 0) table = kDefaultTransitions;

    for (u32 row = 0; row < 3; ++row) {
        for (u32 e = 0; e < 3; ++e) {
            const u32 dst = kMixTransitions + row * 84 + e * 28, src = table + row * 84 + e * 28;
            for (u32 j = 0; j < 3; ++j) {
                store<u32>(dst + j * 8, load<u32>(src + j * 8));
                copy_float(dst + j * 8 + 4, src + j * 8 + 4);
            }
        }
    }
    store<u32>(kMixMode, 0);
    for (u32 j = 0; j < 3; ++j) {
        const f32 v = static_cast<f32>(static_cast<f64>(load<s32>(kMixTransitions + j * 8)));
        store<u32>(kMixRate + j * 4, 0);
        store<f32>(kMixCurrent + j * 4, v);
        store<f32>(kMixTarget + j * 4, v);
    }
    ref<u8>(kMixEnabled) = 1;
    store<u32>(kMixLevels, kHundredBits);
    store<u32>(kMixLevels + 4, 0);
    store<u32>(kMixLevels + 8, 0);
    for (u32 j = 0; j < 3; ++j) {
        const u32 v = static_cast<u32>(rt::fctiw(load<f32>(kMixCurrent + j * 4), true));
        ref<u8>(s + 44 + j) = static_cast<u8>(v);
        store<u32>(s + 56, load<u32>(s + 56) | (0x10000u << j));
    }

    u32 handle = 0;
    const u32 focus = load<u32>(kFocusObj);
    if (focus != 0) {
        if (const u32 d = load<u32>(focus)) {
            call(ctx, kFocusStart, {out, d, s});
            handle = load<u32>(out);
        }
    }
    store<u32>(kFocusHandle, handle);
    if (const u32 a = load<u32>(s + 60)) call_ptr(ctx, load<u32>(load<u32>(a) + 40), {a, 0});
    if (const u32 b = load<u32>(s + 64)) {
        store<u32>(s + 60, 0);
        call_ptr(ctx, load<u32>(load<u32>(b) + 40), {b, 0});
    }
}
