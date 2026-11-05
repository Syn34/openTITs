// World_HandleEvent callees, part 21l: a two-array table setup and the
// pending object list rebuild.
#include "hevent19.h"

using namespace sly4;
using namespace sly4::hevent;

namespace {
constexpr u32 kMemDefaultAllocator = 0x00550EE8;  // () -> Allocator* (symbols.txt Mem_DefaultAllocator)
constexpr u32 kArrayInit = 0x00558F38;            // (Array*) (symbols.txt Array_Init)
constexpr u32 kCompareA = 0x00F36AB8, kCompareB = 0x00F36AC0;  // compare OPDs of the two arrays (name?)

constexpr u32 kHandleMapObjs = 0x0106C228;   // handle map (as hevent.h kHandleMap)
constexpr u32 kPending = 0x0106C2C0;         // PtrVector of pending objects (as hevent20 Pending_Flush367018)
constexpr u32 kPendingReady = 0x0106C3C0;    // u8 set once the list is built (name?)
constexpr u32 kPtrVectorReserve = 0x00551718;  // (vec, new capacity) (as hevent8.h)
constexpr u32 kSkipKeys[] = {0x00F64D44, 0x00F64CE4, 0x00F64CE8, 0x00F64CFC};  // u32 keys left out (name?)

// One array: u16 +0 capacity (the source vector's count), +2 / +4 = 0, +6
// element size 16, +8 data, +12 allocator, then Array_Init. The data comes
// from the allocator's vt +0 (a, capacity * (size / 2), 0, 16) when the
// capacity is non-zero (the original leaves the OPD in r8, its entry in r7).
void array_setup(PPUContext& ctx, u32 a, u32 src_vec) {
    const u32 cap = src_vec != 0 ? load<u16>(src_vec + 8) : 0;
    store<u16>(a + 0, static_cast<u16>(cap));
    store<u16>(a + 2, 0);
    store<u16>(a + 4, 0);
    store<u32>(a + 8, 0);
    store<u16>(a + 6, 16);
    const u32 al = static_cast<u32>(call(ctx, kMemDefaultAllocator));
    const u32 n = load<u16>(a);
    store<u32>(a + 12, al);
    if (n != 0) {
        const u32 size = n * ((static_cast<u32>(load<u16>(a + 6)) >> 1) & 0x7FFF);
        const u32 opd = load<u32>(load<u32>(al));
        ctx.gpr[8] = opd;
        ctx.gpr[7] = load<u32>(opd);
        store<u32>(a + 8, static_cast<u32>(call_ptr(ctx, opd, {al, size, 0, 16})));
    }
    call(ctx, kArrayInit, {a});
}
}  // namespace

// u32 Table_Init31F6D8(Table* t, const Vector* src[2]) (name?): two 16-byte
// element arrays (+0 and +24) sized from src[0] / src[1] (u16 +8 counts),
// their compare OPDs +20 = 0x00F36AB8 / +44 = 0x00F36AC0 with +16 / +40 =
// 0, +48 = src, +52..+60 = 0, +64 = 1.0, +68 bits 0xE0000000 cleared.
// Returns the new +68.
DECOMP(0031F6D8, Table_Init31F6D8) {
    const u32 t = arg<u32>(ctx, 0);
    const u32 src = arg<u32>(ctx, 1);
    array_setup(ctx, t, load<u32>(src));
    store<u32>(t + 16, 0);
    store<u32>(t + 20, kCompareA);
    // The second array: as array_setup, but +6 is stored before +8.
    {
        const u32 a = t + 24;
        const u32 v = load<u32>(src + 4);
        const u32 cap = v != 0 ? load<u16>(v + 8) : 0;
        store<u16>(a + 0, static_cast<u16>(cap));
        store<u16>(a + 2, 0);
        store<u16>(a + 4, 0);
        store<u16>(a + 6, 16);
        store<u32>(a + 8, 0);
        const u32 al = static_cast<u32>(call(ctx, kMemDefaultAllocator));
        const u32 n = load<u16>(a);
        store<u32>(a + 12, al);
        if (n != 0) {
            const u32 size = n * ((static_cast<u32>(load<u16>(a + 6)) >> 1) & 0x7FFF);
            const u32 opd = load<u32>(load<u32>(al));
            ctx.gpr[8] = opd;
            ctx.gpr[7] = load<u32>(opd);
            store<u32>(a + 8, static_cast<u32>(call_ptr(ctx, opd, {al, size, 0, 16})));
        }
        call(ctx, kArrayInit, {a});
    }
    const u32 fl = load<u32>(t + 68) & ~0xE0000000u;
    store<u32>(t + 40, 0);
    store<u32>(t + 44, kCompareB);
    store<u32>(t + 48, src);
    store<u32>(t + 52, 0);
    store<u32>(t + 56, 0);
    store<u32>(t + 60, 0);
    store<u32>(t + 64, 0x3F800000u);  // 1.0
    store<u32>(t + 68, fl);
    ret(ctx, fl);
}

// u32 Pending_Build366E20() (name?): empties the pending list (0x0106C2C0)
// and refills it with the handle map's objects whose owner (+24) has +164
// bit 0x8000 and whose key (+12, else the def's +68) is none of the four
// keys at 0x00F64D44 / 0x00F64CE4 / 0x00F64CE8 / 0x00F64CFC; each gets vt
// +20 (o, 0) first. Sets the ready byte 0x0106C3C0.
DECOMP(00366E20, Pending_Build366E20) {
    const u32 v = kPending;
    store<u16>(v + 8, 0);
    for (u32 i = 0; static_cast<s32>(i) < static_cast<s32>(load<u16>(kHandleMapObjs + 2)); ++i) {
        const u32 o = load<u32>(load<u32>(kHandleMapObjs + 8) + 8 * i + 4);
        const u32 w = load<u32>(o + 24);
        if (w == 0 || ((load<u32>(w + 164) >> 15) & 1u) == 0) continue;
        u32 key = load<u32>(o + 12);
        if (key == 0) {
            const u32 d = load<u32>(o + 8);
            key = d != 0 ? load<u32>(d + 68) : 0;
        }
        bool skip = false;
        for (u32 g : kSkipKeys)
            if (key == load<u32>(g)) skip = true;
        if (skip) continue;
        vcall(ctx, o, 20, {o, 0});
        u32 n = load<u16>(v + 8);
        const u32 cap = load<u16>(v + 4);
        if (n == cap) {
            call(ctx, kPtrVectorReserve, {v, (cap << 1) & 0x1FFFEu});
            n = load<u16>(v + 8);
        }
        const u32 data = load<u32>(v);
        const u32 n2 = load<u16>(v + 8) + 1;
        store<u32>(data + ((n << 2) & 0x3FFFCu), o);
        const u32 hw = load<u16>(v + 10);
        store<u16>(v + 8, static_cast<u16>(n2));
        store<u16>(v + 10, static_cast<u16>(static_cast<s32>(hw) > static_cast<s32>(n2 & 0xFFFF) ? hw : n2));
    }
    store<u8>(kPendingReady, 1);
    ret(ctx, 0x01070000u);  // lis r3 of the address
}
