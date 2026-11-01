// Player_Update: per-player input, called by Game_Update for both players.
#include <bit>

#include "player.h"

using namespace sly4;
using namespace sly4::player;

static_assert(std::bit_cast<u32>(kMaxDirectionAngle) == 0x3F490FDBu);

namespace {

// Builds the direction table on first use.
void Player_InitDirections() {
    if (ref<u8>(kDirectionsGuard) != 0) return;
    const f32 dirs[kDirectionCount][2] = {{-1.0f, 0.0f}, {1.0f, 0.0f}, {0.0f, -1.0f}, {0.0f, 1.0f}};
    for (u32 k = 0; k < kDirectionCount; ++k) {
        store<f32>(kDirections + k * 8, dirs[k][0]);
        store<f32>(kDirections + k * 8 + 4, dirs[k][1]);
    }
    ref<u8>(kDirectionsGuard) = 1;
}

// Turns the stick into one of four direction bits: the first direction
// within 45 degrees of the (normalised) stick, if the stick is pushed at
// least kTuneStickThreshold. The bit is added to the held buttons, and to
// the pressed ones too when the direction has just started (no repeat timer
// running, no repeats counted).
void Player_StickToDirection(PPUContext& ctx, u32 p, u32 pad) {
    const u32 valid = field<u32>(p, kValid);
    f32 x = (valid & kPadValidX) ? field<f32>(pad, kPadStickX).get() : kZero;
    f32 y = (valid & kPadValidY) ? field<f32>(pad, kPadStickY).get() : kZero;

    const f32 len2 = fmadds(x, x, fmuls(y, y));
    const f32 threshold = field<f32>(kTuneStickThreshold, kTuningValue);
    const f32 threshold2 = fmuls(threshold, threshold);
    if (!(len2 >= threshold2)) return;

    Player_InitDirections();
    const f32 inv_len = fdivs(kOne, static_cast<f32>(std::sqrt(static_cast<f64>(len2))));  // fsqrts
    x = fmuls(inv_len, x);
    y = fmuls(inv_len, y);
    for (u32 k = 0; k < kDirectionCount; ++k) {
        const u32 dir = kDirections + k * 8;
        const f32 dot = fmadds(x, load<f32>(dir), fmuls(y, load<f32>(dir + 4)));
        if (!(dot > kZero)) continue;
        // Clamp to [-1, 1] with fsel (the original keeps the double constants).
        f64 c = fsel(fadds(dot, kOne), dot, kMinusOneD);
        c = static_cast<f32>(c);
        c = fsel(fsubs(c, kOne), kOneD, c);
        c = static_cast<f32>(c);
        const f64 angle = callf(ctx, pfn::Math_Acos, {}, {c});
        if (!(angle <= static_cast<f64>(kMaxDirectionAngle))) continue;

        const u32 bit = load<u32>(kDirectionBits + k * 4);
        const f32 timer = field<f32>(p, kRepeatTimer);
        field<u32>(p, kHeld) |= bit;
        if (timer == kZero && ((field<u32>(p, kFlags) & kPlayerRepeatMask) >> kPlayerRepeatShift) == 0)
            field<u32>(p, kPressed) |= bit;
        return;
    }
}

// Auto-repeat of the direction bits (the low four held bits). While one is
// held the timer runs; each time it reaches the delay for the current repeat
// count, the held directions are pressed again, the count goes up (to at
// most 6) and the timer restarts. Releasing them resets both.
void Player_RepeatDirections(u32 p, f64 dt) {
    const u32 dirs = field<u32>(p, kHeld) & 0xF;
    if (dirs == 0) {
        field<f32>(p, kRepeatTimer) = 0.0f;
        field<u32>(p, kFlags) &= ~static_cast<u32>(kPlayerRepeatMask);
        return;
    }
    const u32 flags = field<u32>(p, kFlags);
    const u32 count = (flags & kPlayerRepeatMask) >> kPlayerRepeatShift;
    const f32 timer = fadds(field<f32>(p, kRepeatTimer), dt);
    const u32 delay = kTuneRepeatDelays + count * kTuneRepeatStride + ((flags & kPlayerSlowRepeat) ? 4u : 0u) + kTuningValue;
    field<f32>(p, kRepeatTimer) = timer;
    if (!(timer >= load<f32>(delay))) return;
    field<u32>(p, kPressed) |= dirs;
    const u32 next = count + 1 < kMaxRepeats ? count + 1 : kMaxRepeats;
    field<u32>(p, kFlags) = (flags & ~static_cast<u32>(kPlayerRepeatMask)) | (next << kPlayerRepeatShift);
    field<f32>(p, kRepeatTimer) = 0.0f;
}

// Records the pad's motion vector in both history rings.
void Player_RecordMotion(u32 p, u32 pad) {
    // lfs/stfs pairs in the original: copied as raw words.
    u32 v[3];
    for (u32 k = 0; k < 3; ++k) v[k] = load<u32>(pad + kPadMotion + k * 4);
    const u32 a = p + kHistoryA + field<u32>(p, kHistoryHead) * kHistoryEntry;
    for (u32 k = 0; k < 3; ++k) store<u32>(a + k * 4, v[k]);
    const u32 b = p + kHistoryB + field<u32>(p, kHistoryHead) * kHistoryEntry;
    for (u32 k = 0; k < 3; ++k) store<u32>(b + k * 4, v[k]);

    u32 head = field<u32>(p, kHistoryHead) + 1;
    const u32 tail = field<u32>(p, kHistoryTail);
    field<u32>(p, kHistoryHead) = head;
    if (head >= kHistoryLen) {
        field<u32>(p, kHistoryHead) = 0;
        head = 0;
    }
    if (head > tail) return;
    // Full: drop the oldest entry.
    const u32 next = tail + 1;
    field<u32>(p, kHistoryTail) = next;
    if (next >= kHistoryLen) field<u32>(p, kHistoryTail) = 0;
}

// min(max) as the original's compare-and-branch sequence: a NaN gives 1.
f32 Clamp1(f32 v) {
    const f32 lo = v < kOne ? v : kOne;
    if (!(lo > kMinusOne)) return kMinusOne;
    return v < kOne ? v : kOne;
}

// The cursor follows the pointer position when the pad reports one, and is
// otherwise moved by the stick (dead zone, optional squaring, speed, y
// multiplier), smoothed towards the target and clamped to [-1, 1].
void Player_UpdateCursor(u32 p, u32 pad, f64 dt) {
    const f32 smoothing = field<f32>(kTuneCursorSmoothing, kTuningValue);
    if ((field<u32>(p, kValid) & kPadValidPointer) == kPadValidPointer) {
        const u32 flags = field<u32>(p, kFlags);
        f32 x = field<f32>(pad, kPadPointerX);
        f32 y = field<f32>(pad, kPadPointerY);
        if (flags & kPlayerCursorStarted) {
            const f32 cx = field<f32>(p, kCursorX), cy = field<f32>(p, kCursorY);
            x = fmadds(fsubs(x, cx), smoothing, cx);
            y = fmadds(fsubs(y, cy), smoothing, cy);
        }
        field<f32>(p, kCursorX) = x;
        field<f32>(p, kCursorY) = y;
        field<u32>(p, kFlags) = flags | kPlayerCursorStarted;
        return;
    }

    field<u32>(p, kFlags) &= ~static_cast<u32>(kPlayerCursorStarted);
    if ((field<u16>(pad, kPadFlags) & kPadConnected) == 0) return;
    if (field<u32>(pad, kPadType) == kPadTypeNoCursor) return;

    const f32 dead_zone = field<f32>(kTuneDeadZone, kTuningValue);
    const f32 sx = field<f32>(pad, kPadStickX), sy = field<f32>(pad, kPadStickY);
    const f32 cx = field<f32>(p, kCursorX), cy = field<f32>(p, kCursorY);
    f32 x = std::fabs(sx) >= dead_zone ? sx : kZero;
    f32 y = std::fabs(sy) >= dead_zone ? sy : kZero;
    if (field<u8>(kTuneSquareStick, kTuningValue) != 0) {
        x = fmuls(x, std::fabs(x));
        y = fmuls(y, std::fabs(y));
    }
    const f32 speed = field<f32>(kTuneCursorSpeed, kTuningValue);
    const f32 vy = fmuls(fmuls(y, field<f32>(g::config, kConfigInvertY)), speed);
    const f32 vx = fmuls(x, speed);
    const f32 tx = fmadds(vx, dt, cx);
    const f32 ty = fmadds(vy, dt, cy);
    field<f32>(p, kCursorX) = fmadds(fsubs(tx, cx), smoothing, cx);
    field<f32>(p, kCursorY) = fmadds(fsubs(ty, cy), smoothing, cy);

    field<f32>(p, kCursorX) = Clamp1(field<f32>(p, kCursorX));
    field<f32>(p, kCursorY) = Clamp1(field<f32>(p, kCursorY));
    field<u32>(p, kFlags) |= kPlayerCursorStarted;
}

}  // namespace

// void Player_Update(Player* p, f32 dt)
// Copies the pad's button words, turns the stick into repeating direction
// bits (when kPlayerDirections is set), records the motion vector when the
// pad has one, and moves the cursor. Does nothing without a pad.
DECOMP(0045B1E8, Player_Update) {
    const u32 p = arg<u32>(ctx, 0);
    const f64 dt = farg(ctx, 0);
    const u32 pad = field<u32>(p, kPad);
    if (pad == 0) return;

    field<u32>(p, kHeld) = field<u32>(pad, kPadHeld);
    field<u32>(p, kPressed) = field<u32>(pad, kPadPressed);
    field<u32>(p, kReleased) = field<u32>(pad, kPadReleased);
    field<u32>(p, kValid) = field<u32>(pad, kPadValid);

    if (field<u32>(p, kFlags) & kPlayerDirections) Player_StickToDirection(ctx, p, pad);
    Player_RepeatDirections(p, dt);

    if (static_cast<u32>(call_ptr(ctx, load<u32>(load<u32>(pad) + kPadVfnHasMotion), {pad})) != 0)
        Player_RecordMotion(p, pad);
    Player_UpdateCursor(p, field<u32>(p, kPad), dt);
}
