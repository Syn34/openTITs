// Game code, medium sweep part 29: helpers shared by mid29_*.cpp (more
// still-recompiled game functions of 120-400 instructions).
#pragma once

#include <bit>
#include <cmath>
#include <initializer_list>
#include <utility>

#include "mid28.h"
#include "obj6.h"

namespace sly4 {
namespace mid29 {

using namespace opentit;
using namespace opentit::decomp;
using mid14::Lf;
using mid6::W32;
using obj19::VCall;
using mid28::Sgl28;
using mid28::Bits28;

namespace m29fn {
using namespace mid28::m28fn;
constexpr u32 FindComp29 = 0x004BB4B4u;   // Effect_FindComponent(e, hash)
constexpr u32 Bind29 = 0x004BB7E0u;       // Effect_Bind4BB7E0(e, hash, bool create)
constexpr u32 FindById29 = 0x000D6A28u;   // Obj_FindById(id)
constexpr u32 FindByHash29 = 0x000DB290u; // Obj_FindByHash_000DB290(o, hash)
constexpr u32 HashStr29 = 0x0056A6E0u;    // Hash_ToString(h)
constexpr u32 Format29 = 0x005534D0u;     // String_Format(fmt, ...)
constexpr u32 kWorld29 = 0x00F65CA0u;     // the world pointer
constexpr u32 kTarget29 = 0x02235C33u;    // the target record type hash (+276)
}  // namespace m29fn

// The player slot word of slot s: *(world + s * 0x20C80 + 4216).
inline u32 PlayerSlot29(u32 s) { return load<u32>(load<u32>(m29fn::kWorld29) + s * 0x00020C80u + 4208 + 8); }

// A target record q's object: q->+280 when q->+276 is the target type,
// else 0 (r3 = the type constant, as the original builds it there).
inline u32 RecTarget29(PPUContext& ctx, u32 q) {
    ctx.gpr[3] = m29fn::kTarget29;
    return load<u32>(q + 276) == m29fn::kTarget29 ? load<u32>(q + 280) : 0;
}

// Hash_ToString(h) when h is set (the debug-name step of the failure
// paths); r3 = 0 otherwise, as the original's test leaves it.
inline void DbgHash29(PPUContext& ctx, u32 h) {
    if (h != 0) call(ctx, m29fn::HashStr29, {h});
    else ctx.gpr[3] = 0;
}

// The command's "FromTarget" component path (as part 28's CompTarget28):
// the kind-5 component c->+keyOff of the effect x whose record +28 is a
// target gives its +280; 0 when any step fails.
inline u32 CompTarget29(PPUContext& ctx, u32 c, u32 x, u32 keyOff) {
    const u32 key = load<u32>(c + keyOff);
    if (key == 0) return 0;
    const u32 e = static_cast<u32>(call(ctx, m29fn::FindComp29, {x, key}));
    if (e == 0 || static_cast<u32>(VCall(ctx, e, 32, {e})) != 5) return 0;
    const u32 q = load<u32>(e + 28);
    if (q == 0) return 0;
    return RecTarget29(ctx, q);
}

// CompTarget29, else Obj_FindById(c->+idOff) when that is set.
inline u32 TargetOrId29(PPUContext& ctx, u32 c, u32 x, u32 keyOff, u32 idOff) {
    const u32 t = CompTarget29(ctx, c, x, keyOff);
    if (t != 0) return t;
    const u32 id = load<u32>(c + idOff);
    return id != 0 ? static_cast<u32>(call(ctx, m29fn::FindById29, {id})) : 0;
}

// The live AglMaterial named h: the free list's allocator (made on first
// use, obj6::ListAllocator), its nodes walked from Obj_GetField4 by the
// n + 16 pointers (+40 the name, s16 +4 the use count; next at -16); the
// match when its use count is positive, else 0.
inline u32 MatFind29(PPUContext& ctx, u32 h) {
    const u32 al = obj6::ListAllocator(ctx, 0x00F8F114u, 0x00B10ED0u, 0x00AC488Fu, 144);
    const u32 n = static_cast<u32>(call(ctx, 0x00556BF0u, {al}));  // Obj_GetField4
    u32 p = n != 0 ? n + 16 : 0;
    while (p != 0 && load<u32>(p + 40) != h) {
        const u32 nx = load<u32>(p - 16);
        p = nx != 0 ? nx + 16 : 0;
    }
    return p != 0 && static_cast<s16>(load<u16>(p + 4)) > 0 ? p : 0;
}

}  // namespace mid29
}  // namespace sly4
