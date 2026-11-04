// The other StateMachine subclass (vtable 0x00B21AA0, 4 levels of 256
// bytes): constructor and destructors. Its slots +8/+12 (0x00490B80,
// 0x00490E40) and +36 (0x0077D8F8) are not decompiled yet.
#include "vt_world.h"

using namespace sly4;
using namespace sly4::world;

namespace {
constexpr u32 kSubVtable = 0x00B21AA0;
constexpr u32 kSmallFree = 0x00552420;  // (ptr, size)
constexpr u32 kMemFreeFrom = 0x0054EE6C;
constexpr u32 kSubFlags = 52;           // u32: bit 31 set, bit 30 cleared, bit 29 set at construction
}  // namespace

// void SubMachine_Construct(SubMachine* m, u32 a, u32 b, u32 c, u32 d)  (name?)
// m+48 = a, m+40 = b, m+36 = c, m+44 = d; m+28 = m+32 = 0.
DECOMP(0047C5D8, SubMachine_Construct) {
    const u32 m = arg<u32>(ctx, 0), a = arg<u32>(ctx, 1), b = arg<u32>(ctx, 2);
    const u32 c = arg<u32>(ctx, 3), d = arg<u32>(ctx, 4);
    call(ctx, vfn::StateMachine_Construct, {m, 4, 256, 0});
    const u32 flags = ((load<u32>(m + kSubFlags) | 0x80000000u) & ~0x40000000u) | 0x20000000u;
    store<u32>(m + 32, 0);
    store<u32>(m, kSubVtable);
    store<u32>(m + 36, c);
    store<u32>(m + 40, b);
    store<u32>(m + 44, d);
    store<u32>(m + 48, a);
    store<u32>(m + kSubFlags, flags);
    store<u32>(m + 28, 0);
}

// void SubMachine_Destruct(SubMachine* m): the base destructor's code.
DECOMP(00490A88, SubMachine_Destruct) {
    const u32 m = arg<u32>(ctx, 0);
    const u32 ids = load<u32>(m + kSmIds);
    store<u32>(m, kSmVtable);
    if (ids != 0) call(ctx, kSmallFree, {ids, load<u32>(m + kSmBytes)});
}

// void SubMachine_Delete(SubMachine* m, u32 flags): destruct and free.
DECOMP(00490AD0, SubMachine_Delete) {
    const u32 m = arg<u32>(ctx, 0);
    if (m == 0) return;
    store<u32>(m, kSmVtable);
    if (load<u32>(m + kSmIds) != 0) call(ctx, kSmallFree, {load<u32>(m + kSmIds), load<u32>(m + kSmBytes)});
    call(ctx, kMemFreeFrom, {m, 0});
}
