// Game code, part 25: helpers shared by obj25*.cpp (the continued sweep of
// small still-recompiled game functions, in size order, from 27
// instructions up).
#pragma once

#include <bit>
#include <cmath>

#include "obj24.h"

namespace sly4 {
namespace obj25 {

using namespace opentit;
using namespace opentit::decomp;
using namespace sly4::obj24;
using namespace sly4::obj23;
using namespace sly4::obj19;

namespace o25fn {
constexpr u32 Mem_FreeFrom = 0x0054EE6C;        // (p, pool)
constexpr u32 Emitter_Stop = 0x00443424;        // (handle, bool fade)
constexpr u32 Thunk_004B1308 = 0x004B1308;      // (p)
constexpr u32 Obj_FindByHash_0DB290 = 0x000DB290;  // (o, hash) -> component or 0 (name?)
}  // namespace o25fn

namespace o25imp {
constexpr u32 sys_ppu_thread_create = 0x00ABF144;  // (u64* id, entry opd, u64 arg, s32 prio, u32 stack, u64 flags, name)
}  // namespace o25imp

// VCall with the vtable read beforehand (where the original loads it
// before a store that could alias it): vt->[slot](args...), the OPD and
// its entry address following the arguments in the registers.
inline u64 VCallVt(PPUContext& ctx, u32 vt, u32 slot, std::initializer_list<u64> args) {
    const u32 opd = load<u32>(vt + slot);
    u64 r[8] = {};
    u32 n = 0;
    for (u64 a : args) r[n++] = a;
    r[n++] = opd;
    r[n++] = load<u32>(opd);
    switch (n) {
    case 2: return call_ptr(ctx, opd, {r[0], r[1]});
    case 3: return call_ptr(ctx, opd, {r[0], r[1], r[2]});
    case 4: return call_ptr(ctx, opd, {r[0], r[1], r[2], r[3]});
    case 5: return call_ptr(ctx, opd, {r[0], r[1], r[2], r[3], r[4]});
    case 6: return call_ptr(ctx, opd, {r[0], r[1], r[2], r[3], r[4], r[5]});
    case 7: return call_ptr(ctx, opd, {r[0], r[1], r[2], r[3], r[4], r[5], r[6]});
    default: return call_ptr(ctx, opd, {r[0], r[1], r[2], r[3], r[4], r[5], r[6], r[7]});
    }
}

// The sign-extended 64-bit form of a 32-bit value (extsw).
inline u64 Sx(u32 v) { return static_cast<u64>(static_cast<s64>(static_cast<s32>(v))); }

// The body of 0x003AA9F0 / 0x003AAA64 (event e = r4): when the state o->+380
// is 2, 0x003AA638(o, 2, ...) for the event 0xA7DC43F5 and 0x003A9EF8(o, 2,
// ...) for 0xD4B61643 (r3 left as o otherwise, r4 / r5 as compared).
inline void ObjOnEventA7DC(PPUContext& ctx, u32 o) {
    const s32 h = load<s32>(arg<u32>(ctx, 1) + 8);
    ctx.gpr[3] = o;
    ctx.gpr[4] = static_cast<u32>(h);
    u32 fn, k;
    if (h > static_cast<s32>(0xA7DC43F5u)) {
        k = 0xD4B61643u;
        fn = 0x003A9EF8;
        ctx.gpr[5] = Sx(k);
        if (h != static_cast<s32>(k)) return;
    } else {
        k = 0xA7DC43F4u;
        fn = 0x003AA638;
        ctx.gpr[5] = Sx(k);
        if (!(h > static_cast<s32>(k))) return;
    }
    const u32 s = load<u32>(o + 380);
    ctx.gpr[4] = s;
    if (s != 2) return;
    call(ctx, fn, {o, s, Sx(k)});
}

// T::~T (0x00747180) for an adjusted this: vtable 0x00B123D0; when +372 is
// set, vtable 0x00B12500 at +344, Mem_FreeFrom(+372, 0) and +372 = 0;
// vtable 0x00B1200C at +344, then the base destructor 0x000D7024.
inline void VtB123D0DtorAt(PPUContext& ctx, u32 o) {
    const u32 p = load<u32>(o + 372);
    store<u32>(o, 0x00B123D0);
    if (p != 0) {
        store<u32>(o + 344, 0x00B12500);
        call(ctx, o25fn::Mem_FreeFrom, {p, 0});
        store<u32>(o + 372, 0);
    }
    store<u32>(o + 344, 0x00B1200C);
    call(ctx, 0x000D7024, {o});
}
// T::~T deleting form (0x0075A684) for an adjusted this: vtables 0x00B13974
// / 0x00B194F8 (+8), then the r13 pool (-28332)->+44->vfn[12](o, the u16
// +4). Null does nothing (r3 = 0).
inline void VtB13974DeleteAt(PPUContext& ctx, u32 o) {
    if (o == 0) {
        ctx.gpr[3] = 0;
        return;
    }
    store<u32>(o + 8, 0x00B194F8);
    store<u32>(o, 0x00B13974);
    const u32 pool = Pool28332(ctx);
    VCall(ctx, pool, 12, {pool, o, load<u16>(o + 4)});
}
// T::~T deleting form (0x0077EBA4) for an adjusted this: vtables 0x00B205B8
// / 0x00B2061C (+56), 0x0048F6B0(o + 56), Dtor_B219D0_00490B34(o),
// Mem_FreeFrom(o, 0). Null does nothing (r3 = 0).
inline void VtB205B8DeleteAt(PPUContext& ctx, u32 o) {
    if (o == 0) {
        ctx.gpr[3] = 0;
        return;
    }
    store<u32>(o, 0x00B205B8);
    store<u32>(o + 56, 0x00B2061C);
    call(ctx, 0x0048F6B0, {o + 56});
    call(ctx, 0x00490B34, {o});
    call(ctx, o25fn::Mem_FreeFrom, {o, 0});
}

}  // namespace obj25
}  // namespace sly4
