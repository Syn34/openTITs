// Game code, part 26: helpers shared by obj26*.cpp (the continued sweep of
// small still-recompiled game functions, in size order, from 32
// instructions up).
#pragma once

#include <bit>
#include <cmath>

#include "obj25.h"

namespace sly4 {
namespace obj26 {

using namespace opentit;
using namespace opentit::decomp;
using namespace sly4::obj25;
using namespace sly4::obj24;
using namespace sly4::obj23;
using namespace sly4::obj19;

namespace o26fn {
constexpr u32 Obj_OnMsg_0DA85C = 0x000DA85C;     // (o, msg, out) -> r3: the base message handler (name?)
constexpr u32 Obj_BaseDtor_0C2DA0 = 0x000C2DA0;  // (o): the base destructor (vtable 0x00B11D98)
constexpr u32 AnimSkel_Release = 0x007D4EA8;     // (p)
constexpr u32 Mem_FreeIfSet = 0x00555CC0;        // (p, a)
constexpr u32 XmlCursor_DocPath = 0x004F8944;    // (cursor) -> char* or 0
}  // namespace o26fn

constexpr u32 kMemAlloc = 0x0054ED38;  // Mem_Alloc(size, pool, align)

// The base vtable 0x00B11D98, its destructor 0x000C2DA0(o) and
// Mem_FreeFrom(o, 0): the tail of the deleting destructors.
inline void DeleteTailB11D98(PPUContext& ctx, u32 o) {
    store<u32>(o, 0x00B11D98);
    call(ctx, o26fn::Obj_BaseDtor_0C2DA0, {o});
    call(ctx, o25fn::Mem_FreeFrom, {o, 0});
}

// The script-command builder shape: p = SmallAlloc(size), make(p) when
// allocated, then the command list push 0x0042C138(p or null).
template <typename F>
inline void PushNew26(PPUContext& ctx, u32 size, F make) {
    const u32 p = static_cast<u32>(call(ctx, o19fn::SmallAlloc, {size}));
    u32 r = 0;
    if (p != 0) {
        make(p);
        r = p;
    }
    call(ctx, 0x0042C138, {r});
}

// The message handler shape r3 = o, r4 = msg, r5 = out: when the s8 msg->+24
// is 1 and o->+off equals msg + the s8 msg->+16, *out = -1 and o->+160 is
// returned (r4 = -1, r5 = o->+160 left); otherwise the base handler
// 0x000DA85C(o, msg, out).
inline void MsgMatchOrBase(PPUContext& ctx, u32 off) {
    const u32 o = arg<u32>(ctx, 0), m = arg<u32>(ctx, 1), out = arg<u32>(ctx, 2);
    const s32 k = load<s8>(m + 24);
    ctx.gpr[6] = static_cast<u32>(k);
    if (k == 1) {
        const u32 t = m + static_cast<u32>(static_cast<s32>(load<s8>(m + 16)));
        ctx.gpr[6] = t;
        if (t != 0) {
            const u32 v = load<u32>(o + off);
            ctx.gpr[7] = v;
            if (v == t) {
                const u32 r = load<u32>(o + 160);
                store<s32>(out, -1);
                ctx.gpr[4] = static_cast<u64>(-1);
                ctx.gpr[5] = r;
                ctx.gpr[3] = r;
                return;
            }
        }
    }
    ctx.gpr[3] = call(ctx, o26fn::Obj_OnMsg_0DA85C, {o, m, out});
}

}  // namespace obj26
}  // namespace sly4
