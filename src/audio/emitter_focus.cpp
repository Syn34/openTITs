// Emitter_Stop and Audio_FocusPlay: stopping an emitter and starting the
// pending "focus" sound (with its mix transitions).
#include "audio.h"
#include "snd.h"
#include "../common/common.h"

namespace sly4::audio {
using namespace opentit::native;

namespace {

constexpr u32 kKeyMap = 0x010AFEF8;       // u16 count at +2, entries (12 bytes) at +8, default at +16
constexpr u32 kBucketCounts = 0x010B00E0;
constexpr u32 kFocusHandle = 0x010AFEA4;
constexpr u32 kFocusObj = 0x00F65B70, kFocusPending = 0x00F65B74, kFocusList = 0x00F65B7C;

// A focus object (24 bytes, pool-allocated): +0 desc, +4 next, +8 prev, +20 count.
struct FocusObj {
    be_u32 desc;   // +0 SoundDef*
    be_u32 next;   // +4
    be_u32 prev;   // +8
    pad<8> _12;
    be_s32 count;  // +20
};
struct KeyEntry {
    be_u32 key, handle;
    be_s32 priority;
};

// Drops the emitter's key from the key map if it still maps to `handle`.
void KeyMap_Remove(u32 key, u32 handle) {
    GuestFrame frame(8);
    be_u32* key_slot = frame.as<be_u32>();
    *key_slot = key;
    KeyMap* map = host<KeyMap>(kKeyMap);
    s32 i = KeyMap_Find(map, key_slot);
    const u32 entry = i >= 0 ? map->entries + static_cast<u32>(i) * 12 + 4 : kKeyMap + 16;
    const u32 h = gref<be_u32>(entry);
    if ((h >> 16) != (handle >> 16) || ((h ^ handle) & 0xFFFF) != 0) return;
    i = KeyMap_Find(map, key_slot);
    if (i < 0) return;
    auto count = [&] { return static_cast<s32>(map->count.get()); };
    if (count() >= 2 && i < count() - 1) {
        const u32 entries = map->entries;
        do {
            KeyEntry& dst = gref<KeyEntry>(entries + static_cast<u32>(i) * 12);
            const KeyEntry& src = gref<KeyEntry>(entries + static_cast<u32>(i) * 12 + 12);
            dst.key = src.key.get();
            dst.handle = src.handle.get();
            dst.priority = src.priority.get();
            ++i;
        } while (i < count() - 1);
    }
    map->count = static_cast<u16>(count() - 1);
}

// Copies a float through an FPR (lfs / stfs), as the recompiled code does.
void copyf(be_f32& dst, const be_f32& src) { dst = static_cast<f32>(static_cast<f64>(src.get())); }

}  // namespace

// void Emitter_Stop(u32 handle, bool fade)  (name?)
//
// A playing emitter with `fade` gets Voice_Stop(voice, 0) and stays
// allocated (it is released once its voice ends). Otherwise its voice is
// stopped at once (if playing) and the emitter released: the focus object
// if it was the focus, its key, its bucket count and its pool node.
void Emitter_Stop(u32 handle, u8 fade) {
    Emitter* e = Emitter_Of(handle);
    if (!e) return;

    if (Emitter_Of(handle) && Voice_IsPlaying(e->voice) != 0) {
        if (fade != 0) {
            Voice_Stop(e->voice, 0);
            return;
        }
        Voice_Stop(e->voice, 1);
    }

    const be_u16* focus = host<be_u16>(kFocusHandle);
    if ((handle >> 16) == focus[0] && (handle & 0xFFFF) == focus[1]) {
        FocusObj* obj = global<gp<FocusObj>>(kFocusObj);
        if (obj && obj->count > 0) {
            const s32 left = obj->count - 1;
            obj->count = left;
            if (left == 0) {
                if (const u32 prev = obj->prev)
                    gref<FocusObj>(prev).next = obj->next.get();
                else
                    global<be_u32>(kFocusList) = obj->next.get();
                if (const u32 next = obj->next) gref<FocusObj>(next).prev = obj->prev.get();
                common::SmallFree(obj, 24u);
            }
        }
        global<be_u32>(kFocusObj) = 0u;
        global<be_u32>(kFocusHandle) = 0u;
    }
    if (const u32 key = e->key) KeyMap_Remove(key, handle);
    if (e->bucketed != 0) {
        be_s32& count = gref<be_s32>(kBucketCounts + (e->bucket << 2));
        count = count - 1;
    }
    GuestFrame frame(8);
    HandlePool_Remove(frame.as<be_u32>(), host<HandlePool>(kEmitters), handle);
}

namespace {

constexpr u32 kListenerStart = 0x01164B40;
constexpr u32 kDefaultTransitions = 0x00AE43D8;  // [3][3] x 28 bytes, as kMixTransitions
constexpr u32 kTransitionsKey = 0x86DF6221u;     // hash looked up when the desc's own key has none
constexpr u32 kMixEnabled = 0x010AF1BC, kMixLevels = 0x010AF08C, kMixMode = 0x010AF098;
constexpr u32 kMixCurrent = 0x010AF09C, kMixTarget = 0x010AF0A8, kMixRate = 0x010AF0B4;
constexpr u32 kMixTransitions = 0x010AF0C0;
constexpr u32 kHundredBits = 0x42C80000u;
constexpr u32 kVfnRelease = 40;  // (obj, 0)

struct Transition {
    struct {
        be_s32 target;
        be_f32 time;
    } pair[3];
    pad<4> _24;
};
static_assert(sizeof(Transition) == 28);

}  // namespace

// void Audio_FocusPlay()  (name?)
//
// Takes the pending focus object, loads its mix transitions (from its
// description's map, else the defaults), resets the mix to mode 0, and
// starts its sound with the mix values as parameters 1-3. The started
// handle becomes the focus handle.
void Audio_FocusPlay() {
    const u32 pending = global<be_u32>(kFocusPending);
    const u32 desc = pending != 0 ? gref<FocusObj>(pending).desc.get() : 0;

    // PlayDesc on the stack (the original's sp+112), then the key and the result.
    GuestFrame frame(96);
    PlayDesc& s = *frame.as<PlayDesc>();
    be_u32* key_slot = frame.as<be_u32>(80);
    be_u32* out = frame.as<be_u32>(84);
    const be_f32* ls = host<be_f32>(kListenerStart);
    for (u32 k = 0; k < 3; ++k) copyf(s.position[k], ls[k]);
    copyf(s.velocity[0], ls[0]);
    copyf(s.velocity[1], ls[1]);
    s.shape = 0u;
    s.zone = 0u;
    s.flags = 0u;
    s.key = 0u;
    s.params[0] = 0;
    s.param = 0u;
    s.w36 = 0u;
    s.gain = 1.0f;
    copyf(s.velocity[2], ls[2]);
    s.params[1] = 0;
    s.params[2] = 0;
    s.params[3] = 0;
    global<be_u32>(kFocusObj) = global<be_u32>(kFocusPending).get();
    global<be_u32>(kFocusPending) = 0u;

    u32 table = 0;
    if (desc != 0) {
        if (const u32 info = gref<be_u32>(desc + 4)) {
            table = gref<be_u32>(snd::Map_Lookup(info + 32, host<be_u32>(desc)));
            if (table == 0) {
                *key_slot = kTransitionsKey;
                table = gref<be_u32>(snd::Map_Lookup(gref<be_u32>(desc + 4) + 32, key_slot));
            }
        }
    }
    if (table == 0) table = kDefaultTransitions;

    Transition* dst = host<Transition>(kMixTransitions);
    const Transition* src = &gref<Transition>(table);
    for (u32 t = 0; t < 9; ++t)
        for (u32 j = 0; j < 3; ++j) {
            dst[t].pair[j].target = src[t].pair[j].target.get();
            copyf(dst[t].pair[j].time, src[t].pair[j].time);
        }
    global<be_s32>(kMixMode) = 0;
    be_f32* current = host<be_f32>(kMixCurrent);
    be_f32* target = host<be_f32>(kMixTarget);
    be_u32* rate = host<be_u32>(kMixRate);
    for (u32 j = 0; j < 3; ++j) {
        const f32 v = static_cast<f32>(static_cast<f64>(dst[0].pair[j].target.get()));
        rate[j] = 0u;
        current[j] = v;
        target[j] = v;
    }
    global<u8>(kMixEnabled) = 1;
    be_u32* levels = host<be_u32>(kMixLevels);
    levels[0] = kHundredBits;
    levels[1] = 0u;
    levels[2] = 0u;
    for (u32 j = 0; j < 3; ++j) {
        const u32 v = static_cast<u32>(rt::fctiw(current[j].get(), true));
        s.params[j] = static_cast<u8>(v);
        s.flags = s.flags | (0x10000u << j);
    }

    u32 handle = 0;
    if (const FocusObj* focus = global<gp<FocusObj>>(kFocusObj)) {
        if (const u32 d = focus->desc) {
            Audio_StartFocusSound(out, host<SoundDef>(d), &s);
            handle = *out;
        }
    }
    global<be_u32>(kFocusHandle) = handle;
    if (const u32 a = s.zone) call_ptr(vfunc(a, kVfnRelease), a, 0u);
    if (const u32 b = s.shape) {
        s.zone = 0u;
        call_ptr(vfunc(b, kVfnRelease), b, 0u);
    }
}

}  // namespace sly4::audio

NATIVE(00443424, sly4::audio::Emitter_Stop)
NATIVE(0044A3D4, sly4::audio::Audio_FocusPlay)
