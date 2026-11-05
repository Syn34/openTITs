// Game code, part 23: helpers shared by obj23*.cpp (the continued sweep of
// small still-recompiled game functions, in size order).
#pragma once

#include <bit>
#include <cmath>

#include "obj22.h"

namespace sly4 {
namespace obj23 {

using namespace opentit;
using namespace opentit::decomp;
using namespace sly4::obj22;
using obj19::VCall;

namespace o23fn {
constexpr u32 Shader_FindByHash = 0x004FE990;  // (hash, ...) -> shader or 0
constexpr u32 Material_Alloc = 0x004FF2E8;     // () -> material
constexpr u32 String_Printf = 0x00551540;      // sprintf(dst, fmt, ...)
constexpr u32 VtB12550_Dtor = 0x000DF934;      // the base destructor (vtable 0x00B12550)
}  // namespace o23fn

namespace o23imp {
constexpr u32 cellSaveDataDelete2 = 0x00ABDBE4;
constexpr u32 sys_ppu_thread_exit = 0x00ABF3A4;
}  // namespace o23imp

// The world pointer.
constexpr u32 kWorldPtr = 0x00F65CA0;

// The f32 word at ea, as the double an lfs leaves in an FPR.
inline f64 LfsAt(u32 ea) { return static_cast<f64>(load<f32>(ea)); }
// lwz / stw (or lfs / stfs) pair: the word at s copied to d; returns it.
inline u32 Cp(u32 d, u32 s) {
    const u32 w = load<u32>(s);
    store<u32>(d, w);
    return w;
}

}  // namespace obj23
}  // namespace sly4
