// Game code, VMX (AltiVec) function sweep part 1: helpers shared by
// vmx1_*.cpp (game functions the size sweeps skipped because they use
// VMX: matrix transforms, vector selects, quaternion ops, ...).
#pragma once

#include <bit>
#include <cmath>

#include "big17.h"
#include "obj28.h"

namespace sly4 {
namespace vmx1 {

using namespace opentit;
using namespace opentit::decomp;
using mid1::LfsBits;
using obj19::VCall;
using obj25::Sx;
using big4::CopyW;
using big5::V4;
using big5::Lvx;
using big5::Stvx;
using big5::Row3;
using big5::Add4;
using big6::LaneW;
using big6::Splat;
using big6::Mrghw;
using big6::Mrglw;
using big6::Perm;
using big6::Lvlx;
using big6::Sub4;
using big7::F;
using big7::Sel;
using big7::Andc;
using big7::Xor;
using big7::Min4;
using big7::Max4;
using big7::Mul0;
using big7::Fmadds;
using big7::Fmsubs;
using big7::Fnmsubs;
using big7::SetF1Bits;
using big7::CopyMat43Last;
using big5::CopyMat43W;
using big14::Fw;
using big14::Vmadd;
using big11::Adds;
using big11::Subs;
using big11::Muls;
using big11::Divs;
using big12::Fsel;
using big12::Lfd;
using obj28::RandStep28;
using big17::HkAlloc;

namespace v1fn {
constexpr u32 Obj_IndexByHash = 0x000D14DC;  // s32 (owner, u32 hash, ...) -> index (< 0 when none)
constexpr u32 Obj_Entry16 = 0x000DA954;      // T* (T* o, s32 i): 16-byte entry i (null when none)
constexpr u32 String_Format = 0x005534D0;    // (fmt, ...) -> rotating static buffer
constexpr u32 sscanf = 0x0071EC78;           // (s, fmt, ...) -> fields converted
constexpr u32 cellNetCtlGetInfo = 0x00ABDE44;  // import stub (code, CellNetCtlInfo*)
constexpr u32 sys_net_sendto = 0x00ABDD24;   // import stub (s, buf, len, flags, addr, addrlen)
}  // namespace v1fn

// The f32 lanes of v transformed by the rows r0..r2 (vmaddfp chain from a
// +0 accumulator: r2 * v.z + (r1 * v.y + (r0 * v.x + 0))).
inline V4 XformDir(const V4& r0, const V4& r1, const V4& r2, const V4& v) { return Row3(r0, r1, r2, v.e[0], v.e[1], v.e[2]); }
// The same plus the translation row r3 (vaddfp r3 + ...).
inline V4 XformPoint(const V4& r0, const V4& r1, const V4& r2, const V4& r3, const V4& v) { return Add4(r3, XformDir(r0, r1, r2, v)); }
// The 4x4 matrix rows at m, m+16, m+32, m+48 (lvx each).
struct Mat4 {
    V4 r[4];
};
inline Mat4 LoadMat(u32 m) { return Mat4{{Lvx(m), Lvx(m + 16), Lvx(m + 32), Lvx(m + 48)}}; }
// fsqrts.
inline f32 Sqrts(f32 x) { return static_cast<f32>(std::sqrt(static_cast<f64>(x))); }

}  // namespace vmx1
}  // namespace sly4
