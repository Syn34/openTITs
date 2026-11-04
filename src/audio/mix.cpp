// Audio_UpdateMix: the music/ambience mix state, once a frame from Audio_Update.
#include "audio.h"

using namespace sly4;
using namespace sly4::audio;

namespace {

// Mix state.
constexpr u32 kMixEnabled = 0x010AF1BC;  // u8
constexpr u32 kMixLevels = 0x010AF08C;   // f32[3] in [0, 100]; [1] and [2] driven here
constexpr u32 kMixMode = 0x010AF098;     // s32: 0, 1 or 2
constexpr u32 kMixCurrent = 0x010AF09C;  // f32[3] voice parameter values
constexpr u32 kMixTarget = 0x010AF0A8;   // f32[3]
constexpr u32 kMixRate = 0x010AF0B4;     // f32[3] per second
constexpr u32 kMixTransitions = 0x010AF0C0;  // [from 3][to 3] of 28 bytes: { s32 target, f32 time }[3], pad
constexpr u32 kTransitionSize = 28, kTransitionRow = 84;

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

// Starts a mode change: copies each target of the transition and sets the
// rate that reaches it in its time (or jumps there for a time near zero
// or not positive).
void Mix_StartTransition(s32 from, s32 to) {
    const u32 entry = kMixTransitions + static_cast<u32>(to) * kTransitionSize + static_cast<u32>(from) * kTransitionRow;
    for (u32 j = 0; j < 3; ++j) {
        const u32 pair = entry + j * 8;
        const f32 target = static_cast<f32>(static_cast<f64>(load<s32>(pair)));  // fcfid + frsp
        const f32 time = load<f32>(pair + 4);
        store<f32>(kMixTarget + j * 4, target);
        if (std::fabs(time) < kEpsilon || !(time > kZero)) {
            store<f32>(kMixCurrent + j * 4, target);
            store<u32>(kMixRate + j * 4, 0);
        } else {
            store<f32>(kMixRate + j * 4, fdivs(fsubs(target, load<f32>(kMixCurrent + j * 4)), time));
        }
    }
}

// Picks the mode from the two levels, with hysteresis.
s32 Mix_PickMode(s32 cur) {
    const f32 level1 = load<f32>(kMixLevels + 4), level2 = load<f32>(kMixLevels + 8);
    auto tune = [](u32 obj) { return field<f32>(obj, kTuningValue).get(); };
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
void Mix_ReleasePending(PPUContext& ctx, u32 obj) {
    const s32 count = load<s32>(obj + 20);
    if (count > 0) {
        store<s32>(obj + 20, count - 1);
        if (count - 1 == 0) {
            if (const u32 prev = load<u32>(obj + 8))
                store<u32>(prev + 4, load<u32>(obj + 4));
            else
                store<u32>(kFocusList, load<u32>(obj + 4));
            if (const u32 next = load<u32>(obj + 4)) store<u32>(next + 8, load<u32>(obj + 8));
            call(ctx, afn::Pool_Free, {obj, 24});
        }
    }
    store<u32>(kFocusPending, 0);
}

// Counts the focused voice's sub-voices; more than 4 sets kFocusBusy.
void Mix_CountFocus(PPUContext& ctx) {
    const u32 emitter = Emitter_FromHandle(load<u32>(kFocusHandle));
    if (emitter == 0) return;
    const u32 voice = load<u32>(emitter);
    const s32 n = static_cast<s32>(call(ctx, afn::Voice_GetValue, {voice}));
    if (n <= 0) return;
    s32 total = 0;
    for (s32 i = 0; i < n; ++i) {
        const u32 sub = static_cast<u32>(call(ctx, afn::Voice_GetSub, {voice, static_cast<u64>(static_cast<s64>(static_cast<s16>(i)))}));
        if (sub == 0) continue;
        Stack frame(ctx, 8);
        call(ctx, afn::Sub_GetInfo, {sub, frame.addr(), frame.addr() + 4, 0, 0});
        total += load<s32>(frame.addr());
        if (total > 4) {
            ref<u8>(kFocusBusy) = 1;
            return;
        }
    }
}

}  // namespace

// void Audio_UpdateMix(f32 dt)  (name?)
//
// Raises or lowers mix levels 1 and 2, switches the mix mode from them
// (starting a transition), eases the three mix values to their targets and
// hands them to the focused voice as parameters 1-3; then handles the
// pending focus object and the focused voice's busy state.
DECOMP(0044A970, Audio_UpdateMix) {
    const f64 dt = farg(ctx, 0);
    const s32 cur = load<s32>(kMixMode);

    if (ref<u8>(kMixEnabled) != 0) {
        bool rising[3] = {true, false, false};
        const u32 obj = load<u32>(load<u32>(kCurrentWorld) + 4208 + 8);
        rising[1] = obj != 0 && (load<u32>(obj + 188) & 0x8) != 0;
        const u32 src = load<u32>(kLevel2Source);
        rising[2] = src != 0 && ref<u8>(src + 28) != 0;
        const f32 up[3] = {0.0f, field<f32>(kTuneUp1, kTuningValue), field<f32>(kTuneUp2, kTuningValue)};
        const f32 down[3] = {0.0f, field<f32>(kTuneDown1, kTuningValue), field<f32>(kTuneDown2, kTuningValue)};
        for (u32 i = 1; i < 3; ++i) {
            const u32 level = kMixLevels + i * 4;
            if (rising[i]) {
                const f32 v = fmadds(up[i], dt, load<f32>(level));
                store<f32>(level, v);
                if (v > kHundred) store<u32>(level, kHundredBits);
            } else {
                const f32 v = fnmsubs(down[i], dt, load<f32>(level));
                store<f32>(level, v);
                if (v < kZero) store<u32>(level, 0);
            }
        }
        const s32 mode = Mix_PickMode(cur);
        if (mode != cur) {
            Mix_StartTransition(cur, mode);
            store<s32>(kMixMode, mode);
        }
    }

    for (u32 i = 0; i < 3; ++i) {
        const u32 cur_ea = kMixCurrent + i * 4;
        f32 v = load<f32>(cur_ea);
        const f32 target = load<f32>(kMixTarget + i * 4);
        if (std::fabs(fsubs(v, target)) < kEpsilon) continue;
        const bool below = v < target;
        v = fmadds(load<f32>(kMixRate + i * 4), dt, v);
        store<f32>(cur_ea, v);
        if (below ? v >= target : v <= target) store<f32>(cur_ea, target);
    }

    // Parameters 1-3 of the focused voice. An invalid handle in the first
    // check skips all three; in the second only the second.
    const s32 p0 = fctiwz_s8(load<f32>(kMixCurrent));
    if (const u32 e = Emitter_FromHandle(load<u32>(kFocusHandle))) {
        if (static_cast<u32>(call(ctx, afn::Voice_IsPlaying, {load<u32>(e)})) != 0)
            call(ctx, afn::Voice_SetParam, {load<u32>(e), 1, static_cast<u64>(static_cast<s64>(p0))});
        const s32 p1 = fctiwz_s8(load<f32>(kMixCurrent + 4));
        if (const u32 e1 = Emitter_FromHandle(load<u32>(kFocusHandle))) {
            if (static_cast<u32>(call(ctx, afn::Voice_IsPlaying, {load<u32>(e1)})) != 0)
                call(ctx, afn::Voice_SetParam, {load<u32>(e1), 2, static_cast<u64>(static_cast<s64>(p1))});
        }
        const s32 p2 = fctiwz_s8(load<f32>(kMixCurrent + 8));
        if (const u32 e2 = Emitter_FromHandle(load<u32>(kFocusHandle))) {
            if (static_cast<u32>(call(ctx, afn::Voice_IsPlaying, {load<u32>(e2)})) != 0)
                call(ctx, afn::Voice_SetParam, {load<u32>(e2), 3, static_cast<u64>(static_cast<s64>(p2))});
        }
    }

    const u32 pending = load<u32>(kFocusPending);
    if (pending == 0) return;
    const f32 timer = load<f32>(kFocusTimer);
    if (timer > kZero) {
        const f32 left = fsubs(timer, dt);
        store<f32>(kFocusTimer, left);
        if (left <= kZero) {
            call(ctx, afn::Audio_FocusPlay);
            store<u32>(kFocusTimer, 0);
        }
        return;
    }
    if (ref<u8>(kFocusBusy) != 0) {
        if (Emitter_FromHandle(load<u32>(kFocusHandle)) == 0) {
            store<u32>(kFocusTimer, kFocusRetryBits);
            ref<u8>(kFocusBusy) = 0;
        }
        return;
    }
    if (ref<u8>(kFocusHold) == 0) {
        const u32 focus = load<u32>(kFocusObj);
        const u32 a = focus != 0 ? load<u32>(focus) : 0;
        if (a == load<u32>(pending)) return Mix_ReleasePending(ctx, pending);
    }
    Mix_CountFocus(ctx);
    call(ctx, afn::Emitter_Stop, {load<u32>(kFocusHandle), ref<u8>(kFocusArg)});
    if (ref<u8>(kFocusBusy) == 0) call(ctx, afn::Audio_FocusPlay);
}
