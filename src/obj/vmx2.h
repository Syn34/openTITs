// Game code, VMX (AltiVec) function sweep part 2: helpers shared by
// vmx2_*.cpp (game functions the size sweeps skipped because they use VMX).
// Builds on vmx1.h (XformDir / XformPoint / Mat4 / LoadMat / Sqrts and the
// big5..big14 lane helpers it pulls in).
#pragma once

#include <bit>
#include <cmath>

#include "vmx1.h"

namespace sly4 {
namespace vmx2 {

using namespace opentit;
using namespace opentit::decomp;
using namespace sly4::vmx1;

// vmaddfp a * c + b, lane by lane (one rounding).
inline V4 Madd4(const V4& a, const V4& c, const V4& b) {
    V4 o;
    for (u32 k = 0; k < 4; ++k) o.e[k] = std::fma(a.e[k], c.e[k], b.e[k]);
    return o;
}
// vnmsubfp -(a * c - b), lane by lane (one rounding).
inline V4 Nmsub4(const V4& a, const V4& c, const V4& b) {
    V4 o;
    for (u32 k = 0; k < 4; ++k) o.e[k] = -std::fma(a.e[k], c.e[k], -b.e[k]);
    return o;
}
// The word at ea splatted to all four lanes (lvlx + vspltw 0).
inline V4 SplatW(u32 ea) { return Splat(Lvlx(ea), 0); }
// All four lanes the same f32.
inline V4 Fill4(f32 x) { return V4{{x, x, x, x}}; }
// vspltisw 0 as f32 lanes (+0.0).
inline V4 Zero4() { return V4{{0.0f, 0.0f, 0.0f, 0.0f}}; }

// The compiler's {x, y, z, w} row from three splats and the three lane
// masks of a constant pool (vsel y, vsel z, vandc): x / y / z from lanes
// 0 / 1 / 2, w cleared by the third mask.
struct Masks3 {
    V4 m[3];
};
inline Masks3 LoadMasks3(u32 ea) { return Masks3{{Lvx(ea), Lvx(ea + 16), Lvx(ea + 32)}}; }
inline V4 RowSel(const V4& x, const V4& y, const V4& z, const Masks3& k) { return Andc(Sel(Sel(x, y, k.m[0]), z, k.m[1]), k.m[2]); }
// The same from three words in memory (lvlx + vspltw 0 each).
inline V4 RowSelW(u32 px, u32 py, u32 pz, const Masks3& k) { return RowSel(SplatW(px), SplatW(py), SplatW(pz), k); }

// The profiler marker the Havok-side code inlines: with room in the
// thread's marker buffer (r13-relative global at -28332 + 4, i.e. the word
// at r13 - 28328: +16 the cursor, +48 the end; unsigned compare), the
// record {name, timebase low word (mftb until non-zero)} is written at the
// cursor and the cursor advanced by 12. Returns the cursor read (the
// original leaves it in a volatile register).
inline u32 ProfMark(PPUContext& ctx, u32 name) {
    const u32 prof = load<u32>(static_cast<u32>(ctx.gpr[13]) - 28328u);
    const u32 cur = load<u32>(prof + 16);
    if (cur < load<u32>(prof + 48)) {
        store<u32>(cur, name);
        u64 tb;
        do tb = read_timebase();
        while (static_cast<u32>(tb) == 0);
        store<u32>(cur + 4, static_cast<u32>(tb));
        store<u32>(prof + 16, cur + 12);
    }
    return cur;
}

}  // namespace vmx2
}  // namespace sly4
