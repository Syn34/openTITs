// World objects, part 8: the point / spline instance and definition
// constructors, the spline record list, the light animation set-up, the
// emitter definition reset, the base ApeObj destructor, and the big three
// callers of the object system: Comps_Query, WorldObj_UpdateLod and
// World_AddObjects.
#pragma once

#include <bit>
#include <cmath>

#include "obj7.h"

namespace sly4 {
namespace obj8 {

using namespace opentit;

namespace o8fn {
constexpr u32 SplineDef_Construct = 0x0059ECF8;  // (ApeSplineDef*) (name?)
constexpr u32 SplineArray_Init = 0x0055F2BC;     // (array, u32 count, bool, bool) (name?)
constexpr u32 Kind13_Construct = 0x005A74A8;     // (obj)
constexpr u32 String_Assign = 0x00555F80;        // (String* dst, const String* src)
}  // namespace o8fn

// ---------------------------------------------------------------- free lists
constexpr u32 kSplineInstList = 0x00F8F158;      // header*: +0 allocator, +4 capacity
constexpr u32 kSplineInstListVtbl = 0x00B0EFE8;
constexpr u32 kStrSplineInstList = 0x00AF4D21;   // "... [with T = ApeSplineInstance]"
constexpr u32 kSplineInstanceSize = 288;
constexpr u32 kSplineDefList = 0x00F8F140;       // header*: +0 allocator, +4 capacity
constexpr u32 kSplineDefListVtbl = 0x00B0F028;
constexpr u32 kStrSplineDefList = 0x00AF4B2E;    // "... [with T = ApeSplineDef]"
constexpr u32 kSplineDefSize = 128;
constexpr u32 kVfnListAlloc = 12;                // ApeTFreeList: Alloc()

// ---------------------------------------------------------------- ApeSplineDef (128 bytes)
constexpr u32 kSdName = 64;                      // String
constexpr u32 kSdShared = 108;                   // {u16 refs, u16, void* data (allocated 16 bytes in)}*
constexpr u32 kSdData = 112;                     // u32: bit 31 / bit 30 flags, bits 15-29 the point count
constexpr u32 kSdPoints = 116;                   // 64-byte points: three f32[3] rows and +48 an f32
constexpr u32 kSdPointSize = 64;

// ---------------------------------------------------------------- the spline record list (obj5::kK13List)
constexpr u32 kSlHeader = 4;                     // {u16 cap, u16 count, ?, Opd* moved callback}*
constexpr u32 kSlPtrs = 8;                       // Obj** (optional)
constexpr u32 kSlRecs = 12;                      // 96-byte records

// ---------------------------------------------------------------- LightAnim_Setup
constexpr u32 kLsConsts = 0x005913F4;            // f32 {25, 5, 0, 10000, 1, 1/65535}
// Component property table (a component of Comps +4): +12 u16 count,
// +16 sorted u32 keys, +20 u16 offsets, +24 data.
constexpr u32 kPtCount = 12, kPtKeys = 16, kPtOffsets = 20, kPtData = 24;
constexpr u32 kKeyAnimType = 0x35D6D9D4, kKeyAnimBase = 0xA4BDE666, kKeyAnimPeak = 0xEF8EE9B0;
constexpr u32 kKeyAnimPeriod = 0xA669F794, kKeyAnimNoise = 0x4F725384, kKeyAnimRandomPhase = 0x8106136C;
constexpr u32 kKeyLightRange = 0xA350B65F;

// Looks `key` up in a property table: the value's address, or 0.
u32 PropFind(u32 table, u32 key);

// The peer's 32-byte vector record: (peer - records) / 160 (signed, as the
// original's multiply-shift) into the parallel array at peers +4.
inline u32 PeerVec(u32 peer) {
    const u32 peers = load<u32>(load<u32>(obj3::kSlotPeers));
    const s32 i = static_cast<s32>(peer - load<u32>(peers)) / static_cast<s32>(obj4::kPeerSize);
    return load<u32>(peers + 4) + (static_cast<u32>(i) << 5);
}

inline void SetBit8(u32 ea, u32 bit, bool on) {
    store<u8>(ea, static_cast<u8>((load<u8>(ea) & ~bit) | (on ? bit : 0)));
}

// ---------------------------------------------------------------- VMX helpers (raw lanes)
struct Vec4 {
    u32 w[4];
    f32 f(u32 i) const { return std::bit_cast<f32>(w[i]); }
};

inline Vec4 VLoad(u32 ea) {  // lvx
    Vec4 r;
    const u32 a = ea & ~15u;
    for (u32 i = 0; i < 4; ++i) r.w[i] = load<u32>(a + i * 4);
    return r;
}

// lvlx: the bytes from ea to the end of its 16-byte line, left-justified.
inline Vec4 VLoadLeft(u32 ea) {
    u8 b[16] = {};
    for (u32 i = 0; i < 16 - (ea & 15); ++i) b[i] = load<u8>(ea + i);
    Vec4 r;
    for (u32 i = 0; i < 4; ++i)
        r.w[i] = (u32(b[4 * i]) << 24) | (u32(b[4 * i + 1]) << 16) | (u32(b[4 * i + 2]) << 8) | b[4 * i + 3];
    return r;
}

inline void VStore(u32 ea, const Vec4& v) {  // stvx
    const u32 a = ea & ~15u;
    for (u32 i = 0; i < 4; ++i) store<u32>(a + i * 4, v.w[i]);
}

inline Vec4 VSplat(const Vec4& v, u32 i) { return Vec4{{v.w[i], v.w[i], v.w[i], v.w[i]}}; }
inline Vec4 VFill(u32 w) { return Vec4{{w, w, w, w}}; }
inline Vec4 VZero() { return Vec4{{0, 0, 0, 0}}; }

inline Vec4 VPerm(const Vec4& a, const Vec4& b, const Vec4& c) {
    u8 src[32];
    for (u32 i = 0; i < 4; ++i)
        for (u32 k = 0; k < 4; ++k) {
            src[4 * i + k] = static_cast<u8>(a.w[i] >> (24 - 8 * k));
            src[16 + 4 * i + k] = static_cast<u8>(b.w[i] >> (24 - 8 * k));
        }
    Vec4 r;
    for (u32 i = 0; i < 4; ++i) {
        u32 w = 0;
        for (u32 k = 0; k < 4; ++k) w = (w << 8) | src[(c.w[i] >> (24 - 8 * k)) & 31];
        r.w[i] = w;
    }
    return r;
}

// vmaddfp a * c + b, lane by lane.
inline Vec4 VMadd(const Vec4& a, const Vec4& c, const Vec4& b) {
    Vec4 r;
    for (u32 i = 0; i < 4; ++i) r.w[i] = std::bit_cast<u32>(std::fmaf(a.f(i), c.f(i), b.f(i)));
    return r;
}

inline Vec4 VAdd(const Vec4& a, const Vec4& b) {  // vaddfp
    Vec4 r;
    for (u32 i = 0; i < 4; ++i) r.w[i] = std::bit_cast<u32>(a.f(i) + b.f(i));
    return r;
}

// out_i = r0 * s_i.x + r1 * s_i.y + r2 * s_i.z (+ r3 for i = 3): the
// matrix product the transform code inlines (vmaddfp chains from 0).
inline void MatCombine(const Vec4 r[4], const Vec4 s[4], Vec4 out[4]) {
    for (u32 i = 0; i < 4; ++i) {
        Vec4 a = VMadd(r[0], VSplat(s[i], 0), VZero());
        a = VMadd(r[1], VSplat(s[i], 1), a);
        a = VMadd(r[2], VSplat(s[i], 2), a);
        out[i] = i == 3 ? VAdd(r[3], a) : a;
    }
}

inline void MatLoad(u32 ea, Vec4 m[4]) {
    for (u32 i = 0; i < 4; ++i) m[i] = VLoad(ea + i * 16);
}

inline void MatStore(u32 ea, const Vec4 m[4]) {
    for (u32 i = 0; i < 4; ++i) VStore(ea + i * 16, m[i]);
}

}  // namespace obj8
}  // namespace sly4
