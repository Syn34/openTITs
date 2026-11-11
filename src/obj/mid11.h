// Game code, medium sweep part 11: helpers shared by mid11_*.cpp (still
// recompiled game functions of 120-400 instructions: the leaf def readers /
// factories around Def_Construct74596C (000C5528 family) and the damage /
// collision def readers / factories around Def_Construct74FD80).
#pragma once

#include <bit>
#include <cmath>
#include <utility>

#include "mid10.h"

namespace sly4 {
namespace mid11 {

using namespace opentit;
using namespace opentit::decomp;
using mid10::IntToF32Bits;
using mid10::LoadF;
using mid10::PutBit;
using mid10::SArg;
using mid10::StoreF;
using mid6::F32;
using mid6::W32;
using mid9::CopyW;
namespace m1fn = mid1::m1fn;
namespace xfn = mid5::xfn;

namespace m11fn {
constexpr u32 Def_Construct74596C = 0x0074596C;  // (o, a, b, c, w144, ...) (reads r3..r7, f1..f13, stack)
constexpr u32 Def_Construct74FD80 = 0x0074FD80;  // (o, a, b, c, ...) (reads r3..r6, r9, r10, f1..f13, stack)
constexpr u32 DefLeaf_ReadBase_000C4020 = 0x000C4020;  // (a0, c, x, tmpl, o) (reads r3..r7)
constexpr u32 DefLeaf_Construct_000C30E8 = 0x000C30E8;  // (Def* p, const Record* rec): the leaf def base constructor
constexpr u32 DamageColDef_Read_000C85A8 = 0x000C85A8;  // (a0, c, x, tmpl, o): the damage def base reader
constexpr u32 Thunk_000E27FC = 0x000E27FC;  // (Def* p, const Record* rec): the damage def base constructor
constexpr u32 XmlCursor_DocPath = 0x004F8944;  // (a0) (reads r3 only)
constexpr u32 XmlCursor_FindAttrByName = 0x004F9628;  // (c, const char* name) (reads r3 / r4)
constexpr u32 Math_Cosf = 0x007344E0;  // f1 -> f1
constexpr u32 Math_Sinf = 0x007345F0;  // f1 -> f1
constexpr u32 DamageColDef_ReadAD1F8 = 0x001AD1F8;  // (a0, c, x, tmpl, o) (mid10)
}  // namespace m11fn

constexpr u32 kVecZero = 0x01164B40;     // f32[3] 0
constexpr u32 kVec2_4AFC = 0x01164AFC;   // f32[2], the leaf ctor's v236 default
constexpr u32 kStrAC6790 = 0x00AC6790;   // the leaf def name (three string arguments)
constexpr u32 kStrAD13B8 = 0x00AD13B8;   // the damage def name (both string arguments)
constexpr u32 kStrAD1E98 = 0x00AD1E98;   // another damage def name
constexpr u32 kQuatIdent = 0x01164C80;   // f32[4], the identity quaternion
constexpr u32 kQuat4C90 = 0x01164C90;    // f32[4]

// Def_Construct74596C(rec, a, b, c, w144, r8, r9, r10) with the defaults
// the 000C5528 family passes from its literal pool: f1..f5 / f8 / f12 =
// pool +0, f6 = pool +16, f7 / f9..f11 = pool +20, f13 = pool +12; stack
// s156 / s164 as given, s172 1, v208 the zero vec3, s228 pool +0, v236 =
// 0x01164AFC, s244 / s252 pool +4, s260 pool +8, s268 pool +12, s276..s292
// pool +0, s300 0, s308 0x7641A763, s316..s412 0. Slots the original never
// writes and the callee never reads (112..144, 176..200, 216) go as 0.
// r8 / r9 / r10 are passed as the original leaves them (not read).
inline void LeafDefCtor(PPUContext& ctx, u32 rec, u32 a, u32 b, u32 c, u32 w144, u64 r8, u64 r9, u64 r10, u32 pool, u32 s156, u32 s164) {
    const f64 k0 = LoadF(pool), k12 = LoadF(pool + 12), k16 = LoadF(pool + 16), k20 = LoadF(pool + 20);
    const u64 w0 = load<u32>(pool), w4 = load<u32>(pool + 4), w8 = load<u32>(pool + 8), w12 = load<u32>(pool + 12);
    callf(ctx, m11fn::Def_Construct74596C,
          {rec, a, b, c, w144, r8, r9, r10,
           0, 0, 0, 0, 0,                 // slots 112..144
           s156, s164, 1,                 // s156, s164, s172
           0, 0, 0, 0,                    // slots 176..200
           kVecZero, 0,                   // s212 (v208), slot 216
           w0, kVec2_4AFC,                // s228, s236 (v236)
           w4, w4, w8, w12, w0, w0, w0,   // s244..s292
           0, 0x7641A763u,                // s300, s308
           0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},  // s316..s412
          {k0, k0, k0, k0, k0, k16, k20, k0, k20, k20, k20, k0, k12});
}

// The 000C5528 family's def record extensions (rec + 344.., the Def
// +404.. fields), as the factories preset them.
// A: +344 hash 0, f32 +348 1.0, byte +352 0, f32 +356 1.0, +360..+372 0
// except f32 +364 1.0, bytes +376 0 / +377 1 / +378 0; +0 |= 0x20000.
inline void LeafRecA(u32 rec) {
    store<u32>(rec + 344, 0);
    store<u8>(rec + 352, 0);
    store<u32>(rec + 360, 0);
    store<u32>(rec + 348, 0x3F800000u);
    store<u32>(rec + 356, 0x3F800000u);
    store<u32>(rec + 364, 0x3F800000u);
    store<u32>(rec + 368, 0);
    store<u32>(rec + 372, 0);
    store<u8>(rec + 376, 0);
    store<u8>(rec + 377, 1);
    store<u8>(rec + 378, 0);
    store<u32>(rec, load<u32>(rec) | 0x20000u);
}
// B: f32 +344 1.0, +348 0.5, +352 1.0, +356 0, +360 1.0; +0 |= 0x10000.
inline void LeafRecB(u32 rec) {
    store<u32>(rec + 356, 0);
    store<u32>(rec + 344, 0x3F800000u);
    store<u32>(rec + 348, 0x3F000000u);
    store<u32>(rec + 352, 0x3F800000u);
    store<u32>(rec + 360, 0x3F800000u);
    store<u32>(rec, load<u32>(rec) | 0x10000u);
}
// C: f32 +344 / +348 0, +352 -1.0, +356..+364 0, +368 -90.0, +372 270.0,
// byte +376 0; +0 |= 0x4000.
inline void LeafRecC(u32 rec) {
    store<u32>(rec + 344, 0);
    store<u32>(rec + 348, 0);
    store<u32>(rec + 352, 0xBF800000u);
    store<u32>(rec + 356, 0);
    store<u32>(rec + 360, 0);
    store<u32>(rec + 364, 0);
    store<u32>(rec + 368, 0xC2B40000u);
    store<u32>(rec + 372, 0x43870000u);
    store<u8>(rec + 376, 0);
    store<u32>(rec, load<u32>(rec) | 0x4000u);
}

// ---- the damage / collision def family (Def_Construct74FD80) ----

// Def_Construct74FD80(rec, a, b, c, r7, r8, hash 0x9260C26E, the zero vec3;
// f1..f13 = pool +0, +16, +12 x4, +4, +8, +16, +12 x4; stack floats s204..
// s228 = pool +12, s340 / s348 = pool +16, flag s300 set, the rest 0), as
// every reader / factory of the family builds its defaults record (r7 / r8
// are not read by the callee; passed as the original leaves them). swap:
// f1 = pool +4 and f7 = pool +0 (as 001EC7F8 loads them).
inline void DmgCtor(PPUContext& ctx, u32 rec, u32 a, u32 b, u32 c, u64 r7, u64 r8, u32 pool, bool swap = false) {
    f64 k0 = LoadF(pool), k4 = LoadF(pool + 4);
    if (swap) std::swap(k0, k4);
    const f64 k8 = LoadF(pool + 8), k12 = LoadF(pool + 12), k16 = LoadF(pool + 16);
    const u64 w12 = load<u32>(pool + 12), w16 = load<u32>(pool + 16);
    callf(ctx, m11fn::Def_Construct74FD80,
          {rec, a, b, c, r7, r8, 0x9260C26Eu, kVecZero,
           0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  // slots 112..192 (not set by the original, not read)
           w12, w12, w12, w12,               // s204..s228
           0, 0, 0, 0, 0, 0, 0, 0,           // s232..s288
           1,                                // s300
           0, 0, 0, 0,                       // s304..s328
           w16, w16, 0},                     // s340, s348, s352
          {k0, k16, k12, k12, k12, k12, k4, k8, k16, k12, k12, k12, k12});
}

// The 0x80000 (DamageColDef_ReadAD1F8) part of the defaults: f32 +288..+296
// = the zero vec3, +304 / +308 0, f32 +312 1.0, bit 0x80 of byte +316
// cleared, the seven {+224 hash 0, +228 f32 -1} pairs, f32 +36 -1, byte +44
// 0, then +4 |= flags. Leaves f1 as the original's last lfs.
inline void Dmg80000(PPUContext& ctx, u32 rec, u32 flags) {
    CopyW(rec + 288, kVecZero);
    CopyW(rec + 292, kVecZero + 4);
    CopyW(rec + 296, kVecZero + 8);
    store<u32>(rec + 304, 0);
    store<u32>(rec + 308, 0);
    store<u32>(rec + 312, 0x3F800000u);
    store<u8>(rec + 316, static_cast<u8>(load<u8>(rec + 316) & 0x7Fu));
    for (u32 k = 0; k < 7; ++k) store<u32>(rec + 224 + 8 * k, 0);
    for (u32 k = 0; k < 7; ++k) store<u32>(rec + 228 + 8 * k, 0xBF800000u);
    store<u32>(rec + 36, 0xBF800000u);
    store<u8>(rec + 44, 0);
    store<u32>(rec + 4, load<u32>(rec + 4) | flags);
    ctx.fpr[1] = LoadF(kVecZero + 8);
}

// The record's vector +124 destroyed (PtrVector_Clear, Nop, SmallFree(.., 20)).
inline void DmgDestroy(PPUContext& ctx, u32 rec) {
    const u32 v = load<u32>(rec + 124);
    if (v != 0) {
        call(ctx, m1fn::PtrVector_Clear, {v});
        call(ctx, m1fn::Nop, {v});
        call(ctx, m1fn::SmallFree, {load<u32>(rec + 124), 20});
    }
}

// The readers' template test: true when tmpl is set and has `mask` in
// +24; when it is set without it, c's document path is noted
// (XmlCursor_DocPath(a0)).
inline bool DmgTmpl(PPUContext& ctx, u32 a0, u32 tmpl, u32 mask) {
    if (tmpl == 0) return false;
    if (load<u32>(tmpl + 24) & mask) return true;
    call(ctx, m11fn::XmlCursor_DocPath, {a0});
    return false;
}

// The factories' tail: reader(a0, c, x, tmpl, rec), then a def of `size`
// bytes (SmallAlloc) built by ctor(p, rec); the record's vector destroyed.
// Returns the def (0 when the allocation fails).
inline u32 DmgNew(PPUContext& ctx, u32 a0, u32 c, u32 x, u32 tmpl, u32 rec, u32 reader, u32 size, u32 ctor) {
    call(ctx, reader, {a0, c, x, tmpl, rec});
    const u32 p = static_cast<u32>(call(ctx, m1fn::SmallAlloc, {size}));
    if (p != 0) call(ctx, ctor, {p, rec});
    DmgDestroy(ctx, rec);
    return p;
}

}  // namespace mid11
}  // namespace sly4
