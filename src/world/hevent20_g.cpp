// World_HandleEvent callees, part 20g: level teardown leaves (two vector
// of vectors frees) and the coach mode reset.
#include "hevent19.h"

using namespace sly4;
using namespace sly4::hevent;

namespace {
constexpr u32 kMemFreeFrom = 0x0054EE6C;      // (p, pool) (symbols.txt Mem_FreeFrom)
constexpr u32 kPtrVectorClear = 0x0054EC1C;   // (vec) (symbols.txt PtrVector_Clear)
constexpr u32 kNop = 0x0054EC18;              // (vec) (symbols.txt Nop)
constexpr u32 kPushGlobal = 0x00527A48;       // (u32 v) (symbols.txt PushGlobal_00527A48)
constexpr u32 kEntries = 0x01068818;          // PtrVector of entries {+0 u32 key, +96 PtrVector}
constexpr u32 kSubUpdate25A690 = 0x0025A690;  // (Obj*) (name?)
constexpr u32 kEffectSetHash = 0x004CF988;    // (Effect*, u32 hash) (symbols.txt Effect_SetHash4CF988)
constexpr u32 kEffectHash = 0x00270002;
constexpr u32 kHandleResolveBit1 = 0x003568E0;  // (u32 handle) -> Object* (symbols.txt)
constexpr u32 kObjDispatch362 = 0x0036325C;     // (Object*, 0, 1, 0) (symbols.txt Obj_Dispatch362)
constexpr u32 kFindFlagged = 0x00368A48;        // (u32 key) -> Object* (symbols.txt FindFlagged_00368A48)
constexpr u32 kModeEnd368D78 = 0x00368D78;      // (Object*) (name?)
constexpr u32 kHandleA = 0x00F64CD4, kHandleB = 0x00F64CDC, kHandleC = 0x00F64CCC;  // u32 handles
constexpr u32 kFlaggedKey = 0x00F64D3C;         // u32 key
constexpr u32 kHighlightBusy = 0x0106C330 + 132;  // u8 (as hevent19 Request_Post36A050)

// Dispatch (0, 1, 0) to the object a handle resolves to.
inline void dispatch_handle(PPUContext& ctx, u32 handle_ea) {
    const u32 obj = static_cast<u32>(call(ctx, kHandleResolveBit1, {load<u32>(handle_ea)}));
    call(ctx, kObjDispatch362, {obj, 0, 1, 0});
}
}  // namespace

// void Entries_Free2EE200() (name?): for every entry of the vector at
// 0x01068818: PushGlobal_00527A48(entry +0) (read even for a null entry),
// then a set entry's +96 vector cleared and the entry freed and nulled;
// finally the vector cleared.
DECOMP(002EE200, Entries_Free2EE200) {
    for (u32 i = 0; i < load<u16>(kEntries + 8); ++i) {
        call(ctx, kPushGlobal, {load<u32>(load<u32>(load<u32>(kEntries) + 4 * i))});
        const u32 e = load<u32>(load<u32>(kEntries) + 4 * i);
        if (e == 0) continue;
        call(ctx, kPtrVectorClear, {e + 96});
        call(ctx, kNop, {e + 96});
        call(ctx, kMemFreeFrom, {e, 0});
        store<u32>(load<u32>(kEntries) + 4 * i, 0);
    }
    call(ctx, kPtrVectorClear, {kEntries});
}

// void Obj_FreeLists4ECE04(Obj* o) (name?): every vector of the o +568
// 8-byte table (count u16 +562): its pointers freed, then the vector cleared
// and freed; +562 = 0. A null vector's count (+8) is still read (quirk).
DECOMP(004ECE04, Obj_FreeLists4ECE04) {
    const u32 o = arg<u32>(ctx, 0);
    u32 n;
    for (s32 i = 0; i < static_cast<s32>(n = load<u16>(o + 562)); ++i) {
        const u32 v = load<u32>(load<u32>(o + 568) + 8 * i + 4);
        for (u32 j = 0; j < load<u16>(v + 8); ++j) call(ctx, kMemFreeFrom, {load<u32>(load<u32>(v) + 4 * j), 0});
        if (v == 0) continue;
        call(ctx, kPtrVectorClear, {v});
        call(ctx, kNop, {v});
        call(ctx, kMemFreeFrom, {v, 0});
    }
    store<u16>(o + 562, 0);
    ret(ctx, n);
}

// void Coach_ResetMode29F26C(Coach* c) (name?): 0x0025A690(c), effect +2276
// set to 0x00270002, u8 +2336 bits 0x80 / 0x10 / 0x08 cleared; unless u8
// +2337 bit 0x20, the objects of handles 0x00F64CD4, 0x00F64CDC (when the
// highlight is idle) and 0x00F64CCC get Obj_Dispatch362(obj, 0, 1, 0);
// then 0x00368D78(FindFlagged_00368A48(*0x00F64D3C)).
DECOMP(0029F26C, Coach_ResetMode29F26C) {
    const u32 c = arg<u32>(ctx, 0);
    call(ctx, kSubUpdate25A690, {c});
    const u32 e = load<u32>(c + 2276);
    if (e != 0) call(ctx, kEffectSetHash, {e, kEffectHash});
    const u32 b = load<u8>(c + 2336) & ~0x98u;
    const u32 skip = load<u8>(c + 2337) & 0x20u;
    store<u8>(c + 2336, static_cast<u8>(b));
    if (skip == 0) {
        dispatch_handle(ctx, kHandleA);
        if (load<u8>(kHighlightBusy) == 0) dispatch_handle(ctx, kHandleB);
        dispatch_handle(ctx, kHandleC);
    }
    const u32 obj = static_cast<u32>(call(ctx, kFindFlagged, {load<u32>(kFlaggedKey)}));
    call(ctx, kModeEnd368D78, {obj});
}
