// Game code, large-function sweep part 20: helpers shared by big20_*.cpp.
#pragma once

#include <bit>
#include <cmath>

#include "big19.h"
#include "mid15.h"

namespace sly4 {
namespace big20 {

using namespace opentit;
using namespace opentit::decomp;
using big19::VCall;
using mid15::Cw;
using mid15::Fb;
using mid15::Fmadds;
using mid15::Fmsubs;
using mid15::Lf;
using mid15::S;

namespace b20fn {
constexpr u32 Nav_SnapPoint = 0x0001087C;          // (Vec3* out, Nav* nav, const Vec3* p)
constexpr u32 Nav_Probe10980 = 0x00010980;         // (Nav*, const Vec3* p, -, -, -, Vec3* out; f1, f2, f3) -> bool (reads r3 r4 r8 f1-f3)
constexpr u32 Save_Profile = 0x000C9FC8;           // (u32 hash) -> Profile*
constexpr u32 Obj_FindByKey = 0x000D1454;          // (Obj*, u32 hash) -> WorldObj*
constexpr u32 Obj_IndexByHash = 0x000D14DC;        // (Obj*, u32 hash) -> s32 or -1
constexpr u32 Map_GetPair = 0x000F816C;            // (map, u32* a, u32* b)
constexpr u32 SaveSlot_Bits2Is1 = 0x00465E60;      // (slot, u32 k) -> bool
constexpr u32 Obj_AimMatrix = 0x0049FFE0;          // (Obj*, f32 out[4][4], att, s32 bone, wo, pos, aim, quat)
constexpr u32 Obj_IsLocalPlayerSlot = 0x004AEBE0;  // (Obj*) -> bool
constexpr u32 Matrix_NormalizeInPlace = 0x0056D374;// (Mat34*)
constexpr u32 Rec80_Find = 0x005848CC;             // (list, u32 hash) -> rec or 0
constexpr u32 Math_Cosf = 0x007344E0;              // f1 -> f1
constexpr u32 Math_Sinf = 0x007345F0;              // f1 -> f1
constexpr u32 Vec128_Push = 0x00788A00;            // (Vec*, const Rec128*)
}  // namespace b20fn

constexpr u32 kWorld = 0x00F65CA0u;      // u32 the world state (player slots at +4208, 0x20C80 apart)
constexpr u32 kPlayerStride = 0x00020C80u;
constexpr u32 kNav = 0x00F557ECu;        // u32 the navigation mesh handle
constexpr u32 kRand = 0x01164888u;       // the game's generator {+0 u64 last 16-bit value, +8 u32 seed}
constexpr u32 kZeroVec = 0x01164B40u;    // f32[3] 0 (Constants_Init_0056B93C)
constexpr u32 kIdentity = 0x01164BD0u;   // f32[4][4] identity (rows of 3 copied)

// A tweak variable's f32 value (+72 of its 80-byte record).
inline f64 Tw(u32 rec) { return Lf(rec + 72); }

// The inlined generator on kRand, kept in registers as the compiled code
// does: Next() hands out the float of the 64-bit "last" value, then steps
// the seed (mix = (rotl4 + rotl16) ^ 0x87654321, last = mix & 0xFFFF,
// seed = rotl16(mix)); Commit() stores the seed, then the last value.
struct Rng {
    u32 seed = 0;
    u64 last = 0;
    void Load() {
        last = load<u64>(kRand);
        seed = load<u32>(kRand + 8);
    }
    void Step() {
        const u32 t = mid15::RandMix15(seed);
        seed = std::rotl(t, 16);
        last = t & 0xFFFFu;
    }
    f64 Next() {
        const f64 r = S(static_cast<f64>(static_cast<s64>(last)));
        Step();
        return r;
    }
    void Commit() const {
        store<u32>(kRand + 8, seed);
        store<u64>(kRand, last);
    }
};

}  // namespace big20
}  // namespace sly4
