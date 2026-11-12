// Game code, large-function sweep part 29: helpers shared by big29_*.cpp.
#pragma once

#include <bit>
#include <cmath>

#include "big28.h"

namespace sly4 {
namespace big29 {

using namespace opentit;
using namespace opentit::decomp;
using big17::SetF1;
using big19::VCall;
using big23::Adds;
using big23::Muls;
using big23::Subs;
using big27::Fw27;
using big28::Fill4D;
using big28::RowSel4;
using big5::Lvx;
using big5::Stvx;
using big5::V4;
using big7::Andc;
using big7::Sel;
using mid1::LfsBits;
using mid15::Fb;
using mid15::Fmadds;
using mid15::Fmsubs;
using mid13::LfsF;
using mid15::Lf;
using mid15::S;
using mid16::Divs;
using mid16::Sqrts;
using mid15::Sf;
using vmx2::Fill4;
using vmx2::LoadMasks3;
using vmx2::Masks3;
using vmx2::RowSel;
using vmx2::RowSelW;
using vmx2::SplatW;
using vmx2::Zero4;

namespace b29fn {
constexpr u32 String_Alloc = 0x0055578C;  // (n) -> char* (reads r3 only)
constexpr u32 String_CopyN = 0x00553638;  // (dst, src, n)
constexpr u32 Audio_Start = 0x0044A374;   // (Handle* out, snd, params)
constexpr u32 Slot_BuildMatrix = 0x000CEA48;  // (Mtx* out, Obj*, s32 bone, Obj* frame, const Quat*, const Vec*)
constexpr u32 Obj_PlaySound = 0x00298C10;  // (u32* out, Obj*, Sound*, Pos*, u32 tag)
constexpr u32 PropSet_ReadFlags = 0x004A1440;  // (Owner*, Flags* out)
constexpr u32 WorldObj_ForEach = 0x0057F5D8;   // (obj, fn opd, arg, bool recurse)
constexpr u32 Skeleton_FindBone = 0x005A93D8;  // (skeleton, u32 hash) -> s32 index or -1
constexpr u32 WorldObj_FindByNameCb = 0x00F31D20;  // OPD: ForEach callback, ctx {u32 hash, Obj* found}
constexpr u32 QuatIdentity = 0x01164C90;  // the global vector Slot_BuildMatrix callers pass with ZeroVec
constexpr u32 ZeroVec = 0x01164B40;
}  // namespace b29fn

// f<k> = the f32 at ea, as lfs loads it (NaN payload kept).
inline void SetFk(PPUContext& ctx, int k, u32 ea) { ctx.fpr[k] = std::bit_cast<f64>(LfsBits(load<u32>(ea))); }

// Stack argument k (0-based over all arguments, k >= 8) of the function
// whose entry r1 is sp: the low word of its doubleword at sp + 112 + 8 (k - 8).
inline u32 StackArg(u32 sp, u32 k) { return load<u32>(sp + 112 + 8 * (k - 8) + 4); }

}  // namespace big29
}  // namespace sly4
