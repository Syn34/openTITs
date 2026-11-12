// Names for Player_Update (0x0045B1E8): the per-player input state Game_Update
// keeps at Game +0xB0 (two of them, 0x2EC0 bytes each). Addresses are guest
// addresses in the BCES01284 v01.02 EBOOT; "?" marks a guess.
#pragma once

#include <cmath>

#include <cstddef>

#include "opentit/native.h"
#include "../sly4.h"

namespace sly4 {
namespace player {

// ---------------------------------------------------------------- PPU float ops
// The single-precision forms round the double-precision result to float,
// exactly as the recompiler does (rt::to_single).
inline f32 fadds(f64 a, f64 b) { return static_cast<f32>(a + b); }
inline f32 fsubs(f64 a, f64 b) { return static_cast<f32>(a - b); }
inline f32 fmuls(f64 a, f64 c) { return static_cast<f32>(a * c); }
inline f32 fdivs(f64 a, f64 b) { return static_cast<f32>(a / b); }
inline f32 fmadds(f64 a, f64 c, f64 b) { return static_cast<f32>(std::fma(a, c, b)); }  // a * c + b
inline f64 fsel(f64 a, f64 c, f64 b) { return a >= 0.0 ? c : b; }                        // a >= 0 ? c : b

// ---------------------------------------------------------------- functions
namespace pfn {
constexpr u32 Math_Acos = 0x005E649C;  // f32 (f32) (name?)
}

// ---------------------------------------------------------------- Player
// Offsets into a Player (the pointer Game_Update passes).
constexpr u32 kFlags = 4;           // u32, see PlayerFlag
constexpr u32 kHeld = 8;            // u32 buttons held (pad +16) | direction bits
constexpr u32 kPressed = 12;        // u32 buttons pressed (pad +20) | repeated direction bits
constexpr u32 kReleased = 16;       // u32 (pad +24) (name?)
constexpr u32 kValid = 20;          // u32 copy of pad +128, see PadValid
constexpr u32 kRepeatTimer = 24;    // f32 time the direction has been held since the last repeat
constexpr u32 kPad = 28;            // u32 Pad*, 0 when no pad is assigned
constexpr u32 kCursorX = 32;        // f32 in [-1, 1]
constexpr u32 kCursorY = 36;        // f32 in [-1, 1]
constexpr u32 kHistoryA = 80;       // vec3[360] (pad +104..+112), ring (name?)
constexpr u32 kHistoryB = 5840;     // vec3[360], the same values again (name?)
constexpr u32 kHistoryEntry = 16;
constexpr u32 kHistoryLen = 360;
constexpr u32 kHistoryTail = 11600; // u32
constexpr u32 kHistoryHead = 11604; // u32

enum PlayerFlag : u32 {
    kPlayerSlowRepeat = 0x00004000,     // picks the second of the two repeat delays (name?)
    kPlayerDirections = 0x00008000,     // the stick also produces direction bits
    kPlayerRepeatMask = 0x007F0000,     // number of repeats so far (capped at 6)
    kPlayerRepeatShift = 16,
    kPlayerCursorStarted = 0x00800000,  // the cursor has a previous value to smooth from
};

// Offsets into a Pad (Player +28).
constexpr u32 kPadFlags = 4;     // u16: 0x8000 = connected
constexpr u32 kPadType = 8;      // u32: 1 = no stick cursor (name?)
constexpr u32 kPadHeld = 16, kPadPressed = 20, kPadReleased = 24;
constexpr u32 kPadStickX = 32;   // f32
constexpr u32 kPadStickY = 36;   // f32
constexpr u32 kPadPointerX = 96, kPadPointerY = 100;  // f32 absolute position (name?)
constexpr u32 kPadMotion = 104;  // vec3 (name?)
constexpr u32 kPadValid = 128;   // u32, see PadValid
constexpr u32 kPadVfnHasMotion = 48;  // () -> nonzero when kPadMotion is current (name?)
constexpr u16 kPadConnected = 0x8000;
constexpr u32 kPadTypeNoCursor = 1;

enum PadValid : u32 {
    kPadValidX = 0x1,
    kPadValidY = 0x2,
    kPadValidPointer = 0x30000,  // both bits: the pointer position drives the cursor
};

// ---------------------------------------------------------------- tuning
// Tuning values: objects of 80 bytes whose value sits at +72.
constexpr u32 kTuningValue = 72;
constexpr u32 kTuneStickThreshold = 0x010BBDC0;  // f32: stick length that counts as a direction
constexpr u32 kTuneRepeatDelays = 0x010BBB90;    // [count] objects; +72 normal, +76 slow (name?)
constexpr u32 kTuneRepeatStride = 80;
constexpr u32 kTuneCursorSmoothing = 0x010BBA00; // f32 lerp factor
constexpr u32 kTuneDeadZone = 0x010BBAA0;        // f32
constexpr u32 kTuneSquareStick = 0x010BBAF0;     // u8: square the stick (keeping its sign)
constexpr u32 kTuneCursorSpeed = 0x010BBA50;     // f32
constexpr u32 kConfigInvertY = 0x10;             // g::config f32 multiplier on y (name?)

// The four stick directions, built on first use (function-local static).
constexpr u32 kDirections = 0x010BD6D0;      // f32[4][2]: (-1,0) (1,0) (0,-1) (0,1)
constexpr u32 kDirectionsGuard = 0x010BD6C8; // u8
constexpr u32 kDirectionBits = 0x00AE8234;   // u32[4]: 4, 8, 1, 2
constexpr u32 kDirectionCount = 4;
constexpr u32 kMaxRepeats = 6;

// Constants in .rodata just before Player_Update (0x0045B1C8).
constexpr f32 kZero = 0.0f;
constexpr f32 kOne = 1.0f;
constexpr f64 kMinusOneD = -1.0;
constexpr f64 kOneD = 1.0;
constexpr f32 kMaxDirectionAngle = 0.785398185253143310546875f;  // 0x3F490FDB, pi/4
constexpr f32 kMinusOne = -1.0f;

// ---------------------------------------------------------------- native structures
using opentit::native::gp;
using opentit::native::pad;

struct Vec3Raw {
    be_u32 v[3];  // floats copied as raw words (lfs / stfs pairs)
    pad<4> _12;
};
static_assert(sizeof(Vec3Raw) == kHistoryEntry);

struct Pad {
    be_u32 vtable;     // +0
    be_u16 flags;      // +4: kPadConnected
    pad<2> _6;
    be_u32 type;       // +8
    pad<4> _12;
    be_u32 held;       // +16
    be_u32 pressed;    // +20
    be_u32 released;   // +24
    pad<4> _28;
    be_f32 stick_x;    // +32
    be_f32 stick_y;    // +36
    pad<56> _40;
    be_f32 pointer_x;  // +96
    be_f32 pointer_y;  // +100
    be_u32 motion[3];  // +104
    pad<12> _116;
    be_u32 valid;      // +128
};
static_assert(offsetof(Pad, stick_x) == kPadStickX && offsetof(Pad, motion) == kPadMotion && offsetof(Pad, valid) == kPadValid);

struct Player {
    pad<4> _0;
    be_u32 flags;          // +4
    be_u32 held;           // +8
    be_u32 pressed;        // +12
    be_u32 released;       // +16
    be_u32 valid;          // +20
    be_f32 repeat_timer;   // +24
    gp<Pad> device;        // +28 (0 when no pad is assigned)
    be_f32 cursor_x;       // +32
    be_f32 cursor_y;       // +36
    pad<40> _40;
    Vec3Raw history_a[kHistoryLen];  // +80
    Vec3Raw history_b[kHistoryLen];  // +5840
    be_u32 history_tail;   // +11600
    be_u32 history_head;   // +11604
    pad<0x2EC0 - 11608> _11608;
};
static_assert(sizeof(Player) == 0x2EC0);  // Game +0xB0 holds two
static_assert(offsetof(Player, history_b) == kHistoryB && offsetof(Player, history_tail) == kHistoryTail);

// A tuning object: value at +72.
template <typename T>
struct Tuning {
    pad<kTuningValue> _0;
    T value;  // +72
};

void Player_Update(Player* p, f32 dt);

}  // namespace player
}  // namespace sly4
