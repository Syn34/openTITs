// Player_Update: per-player input, called by Game_Update for both players.
#include <bit>

#include "player.h"

#include "../math/math.h"

namespace sly4::player {
using namespace opentit::native;

static_assert(std::bit_cast<u32>(kMaxDirectionAngle) == 0x3F490FDBu);

namespace {

template <typename T>
T tune(u32 ea) {
    return global<Tuning<T>>(ea).value;
}

struct Direction {
    be_f32 x, y;
};

// Builds the direction table on first use.
void Player_InitDirections() {
    if (global<u8>(kDirectionsGuard) != 0) return;
    const f32 dirs[kDirectionCount][2] = {{-1.0f, 0.0f}, {1.0f, 0.0f}, {0.0f, -1.0f}, {0.0f, 1.0f}};
    Direction* table = host<Direction>(kDirections);
    for (u32 k = 0; k < kDirectionCount; ++k) {
        table[k].x = dirs[k][0];
        table[k].y = dirs[k][1];
    }
    global<u8>(kDirectionsGuard) = 1;
}

// Turns the stick into one of four direction bits: the first direction
// within 45 degrees of the (normalised) stick, if the stick is pushed at
// least kTuneStickThreshold. The bit is added to the held buttons, and to
// the pressed ones too when the direction has just started (no repeat timer
// running, no repeats counted).
void Player_StickToDirection(Player* p, const Pad* pad) {
    const u32 valid = p->valid;
    f32 x = (valid & kPadValidX) ? pad->stick_x.get() : kZero;
    f32 y = (valid & kPadValidY) ? pad->stick_y.get() : kZero;
    const f32 len2 = fmadds(x, x, fmuls(y, y));
    const f32 threshold = tune<be_f32>(kTuneStickThreshold);
    const f32 threshold2 = fmuls(threshold, threshold);
    if (!(len2 >= threshold2)) return;
    Player_InitDirections();
    const f32 inv_len = fdivs(kOne, static_cast<f32>(std::sqrt(static_cast<f64>(len2))));  // fsqrts
    x = fmuls(inv_len, x);
    y = fmuls(inv_len, y);
    const Direction* dirs = host<Direction>(kDirections);
    const be_u32* bits = host<be_u32>(kDirectionBits);
    for (u32 k = 0; k < kDirectionCount; ++k) {
        const f32 dot = fmadds(x, dirs[k].x, fmuls(y, dirs[k].y));
        if (!(dot > kZero)) continue;
        // Clamp to [-1, 1] with fsel (the original keeps the double constants).
        f64 c = fsel(fadds(dot, kOne), dot, kMinusOneD);
        c = static_cast<f32>(c);
        c = fsel(fsubs(c, kOne), kOneD, c);
        c = static_cast<f32>(c);
        const f64 angle = math::Acosf(static_cast<f32>(c));
        if (!(angle <= static_cast<f64>(kMaxDirectionAngle))) continue;
        const u32 bit = bits[k];
        const f32 timer = p->repeat_timer;
        p->held = p->held | bit;
        if (timer == kZero && ((p->flags & kPlayerRepeatMask) >> kPlayerRepeatShift) == 0) p->pressed = p->pressed | bit;
        return;
    }
}

// Auto-repeat of the direction bits (the low four held bits). While one is
// held the timer runs; each time it reaches the delay for the current repeat
// count, the held directions are pressed again, the count goes up (to at
// most 6) and the timer restarts. Releasing them resets both.
void Player_RepeatDirections(Player* p, f64 dt) {
    const u32 dirs = p->held & 0xF;
    if (dirs == 0) {
        p->repeat_timer = 0.0f;
        p->flags = p->flags & ~static_cast<u32>(kPlayerRepeatMask);
        return;
    }
    const u32 flags = p->flags;
    const u32 count = (flags & kPlayerRepeatMask) >> kPlayerRepeatShift;
    const f32 timer = fadds(p->repeat_timer, dt);
    const u32 delay = kTuneRepeatDelays + count * kTuneRepeatStride + ((flags & kPlayerSlowRepeat) ? 4u : 0u) + kTuningValue;
    p->repeat_timer = timer;
    if (!(timer >= global<be_f32>(delay))) return;
    p->pressed = p->pressed | dirs;
    const u32 next = count + 1 < kMaxRepeats ? count + 1 : kMaxRepeats;
    p->flags = (flags & ~static_cast<u32>(kPlayerRepeatMask)) | (next << kPlayerRepeatShift);
    p->repeat_timer = 0.0f;
}

// Records the pad's motion vector in both history rings.
void Player_RecordMotion(Player* p, const Pad* pad) {
    u32 v[3];
    for (u32 k = 0; k < 3; ++k) v[k] = pad->motion[k];
    for (u32 k = 0; k < 3; ++k) p->history_a[p->history_head].v[k] = v[k];
    for (u32 k = 0; k < 3; ++k) p->history_b[p->history_head].v[k] = v[k];
    u32 head = p->history_head + 1;
    const u32 tail = p->history_tail;
    p->history_head = head;
    if (head >= kHistoryLen) {
        p->history_head = 0u;
        head = 0;
    }
    if (head > tail) return;
    // Full: drop the oldest entry.
    const u32 next = tail + 1;
    p->history_tail = next;
    if (next >= kHistoryLen) p->history_tail = 0u;
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
void Player_UpdateCursor(Player* p, const Pad* pad, f64 dt) {
    const f32 smoothing = tune<be_f32>(kTuneCursorSmoothing);
    if ((p->valid & kPadValidPointer) == kPadValidPointer) {
        const u32 flags = p->flags;
        f32 x = pad->pointer_x;
        f32 y = pad->pointer_y;
        if (flags & kPlayerCursorStarted) {
            const f32 cx = p->cursor_x, cy = p->cursor_y;
            x = fmadds(fsubs(x, cx), smoothing, cx);
            y = fmadds(fsubs(y, cy), smoothing, cy);
        }
        p->cursor_x = x;
        p->cursor_y = y;
        p->flags = flags | kPlayerCursorStarted;
        return;
    }
    p->flags = p->flags & ~static_cast<u32>(kPlayerCursorStarted);
    if ((pad->flags & kPadConnected) == 0) return;
    if (pad->type == kPadTypeNoCursor) return;
    const f32 dead_zone = tune<be_f32>(kTuneDeadZone);
    const f32 sx = pad->stick_x, sy = pad->stick_y;
    const f32 cx = p->cursor_x, cy = p->cursor_y;
    f32 x = std::fabs(sx) >= dead_zone ? sx : kZero;
    f32 y = std::fabs(sy) >= dead_zone ? sy : kZero;
    if (tune<u8>(kTuneSquareStick) != 0) {
        x = fmuls(x, std::fabs(x));
        y = fmuls(y, std::fabs(y));
    }
    const f32 speed = tune<be_f32>(kTuneCursorSpeed);
    const f32 vy = fmuls(fmuls(y, gref<be_f32>(g::config + kConfigInvertY)), speed);
    const f32 vx = fmuls(x, speed);
    const f32 tx = fmadds(vx, dt, cx);
    const f32 ty = fmadds(vy, dt, cy);
    p->cursor_x = fmadds(fsubs(tx, cx), smoothing, cx);
    p->cursor_y = fmadds(fsubs(ty, cy), smoothing, cy);
    p->cursor_x = Clamp1(p->cursor_x);
    p->cursor_y = Clamp1(p->cursor_y);
    p->flags = p->flags | kPlayerCursorStarted;
}

}  // namespace

// Copies the pad's button words, turns the stick into repeating direction
// bits (when kPlayerDirections is set), records the motion vector when the
// pad has one, and moves the cursor. Does nothing without a pad.
void Player_Update(Player* p, f32 dt) {
    Pad* pad = p->device;
    if (!pad) return;
    p->held = pad->held.get();
    p->pressed = pad->pressed.get();
    p->released = pad->released.get();
    p->valid = pad->valid.get();
    if (p->flags & kPlayerDirections) Player_StickToDirection(p, pad);
    Player_RepeatDirections(p, dt);
    if (vcall<u32>(pad, kPadVfnHasMotion) != 0) Player_RecordMotion(p, pad);
    Player_UpdateCursor(p, p->device, dt);
}

}  // namespace sly4::player

NATIVE(0045B1E8, sly4::player::Player_Update)
