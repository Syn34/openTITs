// Audio_UpdateMix: the music/ambience mix state, once a frame from Audio_Update.
#include "audio.h"
#include "snd.h"
#include "../common/common.h"

namespace sly4::audio {
using namespace opentit::native;

namespace {

// Mix state.
constexpr u32 kMixEnabled = 0x010AF1BC;  // u8
constexpr u32 kMixLevels = 0x010AF08C;   // f32[3] in [0, 100]; [1] and [2] driven here
constexpr u32 kMixMode = 0x010AF098;     // s32: 0, 1 or 2
constexpr u32 kMixCurrent = 0x010AF09C;  // f32[3] voice parameter values
constexpr u32 kMixTarget = 0x010AF0A8;   // f32[3]
constexpr u32 kMixRate = 0x010AF0B4;     // f32[3] per second
constexpr u32 kMixTransitions = 0x010AF0C0;  // [from 3][to 3] of 28 bytes: { s32 target, f32 time }[3], pad

// Tuning objects (value at +72).
constexpr u32 kTuneUp1 = 0x010AEE2C, kTuneUp2 = 0x010AEF5C;
constexpr u32 kTuneDown1 = 0x010AEE78, kTuneDown2 = 0x010AEFA8;
constexpr u32 kTuneEnter1 = 0x010AEEC4, kTuneLeave1 = 0x010AEF10;
constexpr u32 kTuneEnter2 = 0x010AEFF4, kTuneLeave2 = 0x010AF040;

// Level 1 rises while the World +4208 object's +8 object has bit 8 at
// +188 (name?); level 2 while the 0x00F557F0 object's byte +28 is set.
constexpr u32 kCurrentWorld = 0x00F65CA0;
constexpr u32 kLevel2Source = 0x00F557F0;

// The focused voice (see Audio_Update).
constexpr u32 kFocusHandle = 0x010AFEA4;  // u32 emitter handle
constexpr u32 kFocusArg = 0x010AFEA8;     // u8
constexpr u32 kFocusHold = 0x010AFEA9;    // u8
constexpr u32 kFocusBusy = 0x010AFEAA;    // u8: the focused voice has more than 4 ... (name?)
constexpr u32 kFocusTimer = 0x010AFEAC;   // f32
constexpr u32 kFocusObj = 0x00F65B70;     // u32
constexpr u32 kFocusPending = 0x00F65B74; // u32 object with a count at +20
constexpr u32 kFocusList = 0x00F65B7C;    // u32 list head

// Constants just before the function (0x0044A960).
constexpr f32 kZero = 0.0f;
constexpr u32 kHundredBits = 0x42C80000u;  // 100.0f
constexpr f32 kHundred = std::bit_cast<f32>(kHundredBits);
constexpr f32 kEpsilon = std::bit_cast<f32>(0x3727C5ACu);  // 1e-5
constexpr u32 kFocusRetryBits = 0x3DCCCCCDu;               // 0.1f

struct FocusObj {
    be_u32 desc;   // +0
    be_u32 next;   // +4
    be_u32 prev;   // +8
    pad<8> _12;
    be_s32 count;  // +20
};
struct Pair {
    be_s32 target;
    be_f32 time;
};

f32 tune(u32 obj) { return gref<be_f32>(obj + kTuningValue); }
be_f32* Current() { return host<be_f32>(kMixCurrent); }
be_f32* Target() { return host<be_f32>(kMixTarget); }
be_f32* Rate() { return host<be_f32>(kMixRate); }

// Starts a mode change: copies each target of the transition and sets the
// rate that reaches it in its time (or jumps there for a time near zero
// or not positive).
void Mix_StartTransition(s32 from, s32 to) {
    const Pair* entry = host<Pair>(kMixTransitions + static_cast<u32>(to) * 28 + static_cast<u32>(from) * 84);
    for (u32 j = 0; j < 3; ++j) {
        const f32 target = static_cast<f32>(static_cast<f64>(entry[j].target.get()));  // fcfid + frsp
        const f32 time = entry[j].time;
        Target()[j] = target;
        if (std::fabs(time) < kEpsilon || !(time > kZero)) {
            Current()[j] = target;
            Rate()[j] = 0.0f;
        } else {
            Rate()[j] = fdivs(fsubs(target, Current()[j]), time);
        }
    }
}

// Picks the mode from the two levels, with hysteresis.
s32 Mix_PickMode(s32 cur) {
    const be_f32* levels = host<be_f32>(kMixLevels);
    const f32 level1 = levels[1], level2 = levels[2];
    s32 mode = cur;
    if (cur == 2) {
        if (!(level2 <= tune(kTuneLeave2))) return 2;
        mode = 0;
    } else if (level2 >= tune(kTuneEnter2)) {
        return 2;
    } else if (cur == 1) {
        return level1 <= tune(kTuneLeave1) ? 0 : 1;
    }
    if (level1 >= tune(kTuneEnter1)) mode = 1;
    return mode;
}

// Releases the pending focus object (count at +20, list links +4/+8).
void Mix_ReleasePending(FocusObj& obj) {
    const s32 count = obj.count;
    if (count > 0) {
        obj.count = count - 1;
        if (count - 1 == 0) {
            if (const u32 prev = obj.prev)
                gref<FocusObj>(prev).next = obj.next.get();
            else
                global<be_u32>(kFocusList) = obj.next.get();
            if (const u32 next = obj.next) gref<FocusObj>(next).prev = obj.prev.get();
            common::SmallFree(&obj, 24u);
        }
    }
    global<be_u32>(kFocusPending) = 0u;
}

// Counts the focused voice's sub-voices; more than 4 sets kFocusBusy.
void Mix_CountFocus() {
    const Emitter* e = Emitter_Of(global<be_u32>(kFocusHandle));
    if (!e) return;
    const u32 voice = e->voice;
    const s32 n = Voice_GetValue(voice);
    if (n <= 0) return;
    s32 total = 0;
    for (s32 i = 0; i < n; ++i) {
        const u32 sub = call<u32>(afn::Voice_GetSub, voice, static_cast<s16>(i));
        if (sub == 0) continue;
        GuestFrame frame(8);
        be_s32* info = frame.as<be_s32>();
        call(afn::Sub_GetInfo, sub, info, info + 1, 0u, 0u);
        total += info[0];
        if (total > 4) {
            global<u8>(kFocusBusy) = 1;
            return;
        }
    }
}

// Parameter `which` of the focused voice, if its emitter is live and playing.
bool Mix_SetFocusParam(u32 which, s32 value) {
    const Emitter* e = Emitter_Of(global<be_u32>(kFocusHandle));
    if (!e) return false;
    if (Voice_IsPlaying(e->voice) != 0) snd::Voice_SetParam(e->voice, which, static_cast<u8>(value));
    return true;
}

}  // namespace

// void Audio_UpdateMix(f32 dt)  (name?)
//
// Raises or lowers mix levels 1 and 2, switches the mix mode from them
// (starting a transition), eases the three mix values to their targets and
// hands them to the focused voice as parameters 1-3; then handles the
// pending focus object and the focused voice's busy state.
void Audio_UpdateMix(f64 dt) {
    const s32 cur = global<be_s32>(kMixMode);

    if (global<u8>(kMixEnabled) != 0) {
        bool rising[3] = {true, false, false};
        const u32 obj = gref<be_u32>(global<be_u32>(kCurrentWorld) + 4208 + 8);
        rising[1] = obj != 0 && (gref<be_u32>(obj + 188) & 0x8) != 0;
        const u32 src = global<be_u32>(kLevel2Source);
        rising[2] = src != 0 && gref<u8>(src + 28) != 0;
        const f32 up[3] = {0.0f, tune(kTuneUp1), tune(kTuneUp2)};
        const f32 down[3] = {0.0f, tune(kTuneDown1), tune(kTuneDown2)};
        be_f32* levels = host<be_f32>(kMixLevels);
        for (u32 i = 1; i < 3; ++i) {
            if (rising[i]) {
                const f32 v = fmadds(up[i], dt, levels[i]);
                levels[i] = v;
                if (v > kHundred) levels[i] = kHundred;
            } else {
                const f32 v = fnmsubs(down[i], dt, levels[i]);
                levels[i] = v;
                if (v < kZero) levels[i] = 0.0f;
            }
        }
        const s32 mode = Mix_PickMode(cur);
        if (mode != cur) {
            Mix_StartTransition(cur, mode);
            global<be_s32>(kMixMode) = mode;
        }
    }

    for (u32 i = 0; i < 3; ++i) {
        f32 v = Current()[i];
        const f32 target = Target()[i];
        if (std::fabs(fsubs(v, target)) < kEpsilon) continue;
        const bool below = v < target;
        v = fmadds(Rate()[i], dt, v);
        Current()[i] = v;
        if (below ? v >= target : v <= target) Current()[i] = target;
    }

    // Parameters 1-3 of the focused voice. An invalid handle in the first
    // check skips all three; in the second only the second.
    const s32 p0 = fctiwz_s8(Current()[0]);
    if (Mix_SetFocusParam(1, p0)) {
        Mix_SetFocusParam(2, fctiwz_s8(Current()[1]));
        Mix_SetFocusParam(3, fctiwz_s8(Current()[2]));
    }

    const u32 pending = global<be_u32>(kFocusPending);
    if (pending == 0) return;
    be_f32& timer = global<be_f32>(kFocusTimer);
    if (timer > kZero) {
        const f32 left = fsubs(timer, dt);
        timer = left;
        if (left <= kZero) {
            Audio_FocusPlay();
            timer = 0.0f;
        }
        return;
    }
    if (global<u8>(kFocusBusy) != 0) {
        if (!Emitter_Of(global<be_u32>(kFocusHandle))) {
            timer = std::bit_cast<f32>(kFocusRetryBits);
            global<u8>(kFocusBusy) = 0;
        }
        return;
    }
    if (global<u8>(kFocusHold) == 0) {
        const u32 focus = global<be_u32>(kFocusObj);
        const u32 a = focus != 0 ? gref<FocusObj>(focus).desc.get() : 0;
        if (a == gref<FocusObj>(pending).desc) return Mix_ReleasePending(gref<FocusObj>(pending));
    }
    Mix_CountFocus();
    Emitter_Stop(global<be_u32>(kFocusHandle), global<u8>(kFocusArg));
    if (global<u8>(kFocusBusy) == 0) Audio_FocusPlay();
}

}  // namespace sly4::audio

NATIVE(0044A970, sly4::audio::Audio_UpdateMix)
