// Game code, VMX (AltiVec) function sweep part 7: helpers shared by
// vmx7_*.cpp (game functions the size sweeps skipped because they use VMX).
// Builds on vmx6.h (v6fn, kEffectTargetKind6, EulerQuat6, CopyW6) and,
// through it, vmx5.h .. vmx1.h and the big5..big17 lane helpers.
#pragma once

#include <bit>
#include <cmath>

#include "vmx6.h"

namespace sly4 {
namespace vmx7 {

using namespace opentit;
using namespace opentit::decomp;
using namespace sly4::vmx6;

namespace v7fn {
constexpr u32 Obj_SetFlags64 = 0x007348D8;  // (o, u64 mask, r5, r6)
constexpr u32 Global_SetHashObj = 0x00369D14;  // (Obj* o, const char* s)
constexpr u32 Objects_Flush = 0x00365360;  // () (reads nothing)
constexpr u32 Pending_Flush = 0x00453578;  // u32 (T* o, u8 on)
constexpr u32 Effect_Spawn = 0x004A5FC8;  // (u32 name, owner, value, u8 start) -> Effect*
constexpr u32 Global_F64CB8_Copy216 = 0x00369D58;  // () (reads nothing)
}  // namespace v7fn

constexpr u32 kAttachIdentity = 0x01164BD0;  // the identity rows (x / y / z words used)

// The x / y / z words of n 16-byte rows copied (lfs / stfs pairs, raw bits).
inline void CopyRowsXyz7(u32 dst, u32 src, u32 n) {
    for (u32 r = 0; r < n; ++r) CopyW6(dst + 16 * r, src + 16 * r, 3);
}

}  // namespace vmx7
}  // namespace sly4
