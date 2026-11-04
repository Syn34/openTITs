// World objects, part 10: the peer animators (Peer_GetMatrix), the matrix
// row normalisation, the Bezier segment search and the spline definition
// map.
#pragma once

#include <bit>
#include <cmath>

#include "obj9.h"

namespace sly4 {
namespace obj10 {

using namespace opentit;

namespace o10fn {
constexpr u32 Peer_ApplyFollower = 0x0057C110;  // (peer, follower, r5 unused, f32 out[4][4], bool rotated, bool moved; f1 dt) (name?)
constexpr u32 Follower_Update = 0x0057BA40;     // (follower; f1 dt) (name?)
constexpr u32 Quat_Multiply = 0x005556A4;       // (f32 out[4], const f32 a[4], const f32 b[4]) (name?)
constexpr u32 Quat_ToMatrix = 0x00553A90;       // (f32 out[4][4], const f32 q[4], const f32 pos[3]) (name?)
constexpr u32 Math_Cosf = 0x007344E0;           // (f1 x) -> f1: polynomial cosine (name?)
}  // namespace o10fn

// ---------------------------------------------------------------- peer animators
// The 64-byte records at peer +128 (rotation, degrees) and +132 (position):
// per axis +0 f32 the time, +12 the period, +24 the delay, +36 the base,
// +48 the amplitude, +60 u8 the mode byte: bit 0 = running backwards,
// bits 1-7 = s7 the mode: 0 once (clamped at period + delay), 1 loop,
// 2 ping-pong (the value then follows a cosine (rotation) / Math_Cosf
// (position) wave instead of a ramp).
constexpr u32 kPrRotAnim = 128, kPrPosAnim = 132, kPrFollower = 136;
constexpr u32 kAnTime = 0, kAnPeriod = 12, kAnDelay = 24, kAnBase = 36, kAnAmp = 48, kAnMode = 60;
constexpr u32 kAnBackwards = 0x01;
constexpr s32 kAnOnce = 0, kAnLoop = 1, kAnPingPong = 2;
inline s32 AnimMode(u8 b) { return static_cast<s32>(((b >> 1) & 0x7F) ^ 0x40) - 0x40; }

// f32 {0, 2pi}, f64 {pi, -pi}, f32 {pi/2, pi, 4/pi, -4/pi^2, 0.225, pi/180, 0.5}
constexpr u32 kPgConsts = 0x0057C3D0;
constexpr u32 kPgZero = 0, kPgTwoPi = 4, kPgPiD = 8, kPgNegPiD = 16, kPgHalfPi = 24, kPgPi = 28;
constexpr u32 kPgSinA = 32, kPgSinB = 36, kPgSinC = 40, kPgDegToRad = 44, kPgHalf = 48;

constexpr u32 kIdentity = 0x01164BD0;  // f32[4][4]
constexpr u32 kOrigin = 0x01164B40;    // f32[3] zero point

// ---------------------------------------------------------------- single-precision helpers
inline f32 Fmsubs(f64 a, f64 c, f64 b) { return static_cast<f32>(std::fma(a, c, -b)); }    // a * c - b
inline f32 Fnmsubs(f64 a, f64 c, f64 b) { return static_cast<f32>(-std::fma(a, c, -b)); }  // b - a * c
inline f32 Fsqrts(f64 x) { return static_cast<f32>(std::sqrt(x)); }

}  // namespace obj10
}  // namespace sly4
