// World_HandleEvent callees, part 19j: small leaves under the World event
// and state handlers (field resets, save slot accessors, a wrapper and a
// singleton getter).
#include "hevent19.h"

using namespace sly4;
using namespace sly4::hevent;
using namespace sly4::hevent19;

namespace {
// The pad flags: u8 0x00F558E3 set when a pad is active, u8 0x00F558E2 its index.
constexpr u32 kPadActive = 0x00F558E3, kPadIndex = 0x00F558E2;
// The current user (World +4208 u8) and its save slot (World +80 + 4056 x user).
constexpr u32 kWorldUser = 4208;
constexpr u32 kTarget0CC188 = 0x000CC188;  // (r3..r8, u8, u8; f1; stack args 8 / 10) (name?)
// The singleton at 0x0106C000 (vtable 0x00B1F5D0), guard u8 at 0x0106BFF8.
constexpr u32 kSingleton = 0x0106C000, kSingletonGuard = 0x0106BFF8, kSingletonVtable = 0x00B1F5D0;
}  // namespace

// void Obj_ResetCount2EE458(Obj* o) (name?): +44 u16 = 0 (r4 = 0 left).
DECOMP(002EE458, Obj_ResetCount2EE458) {
    store<u16>(arg<u32>(ctx, 0) + 44, 0);
    ctx.gpr[4] = 0;
}

// s32 Obj_Kind376A08(Obj* o) (name?): the definition's (+8) +152.
DECOMP(00376A08, Obj_Kind376A08) {
    ret(ctx, static_cast<s32>(load<u32>(load<u32>(arg<u32>(ctx, 0) + 8) + 152)));
}

// s32 Pad_ActiveIndexA96B8() (name?): the active pad's index, or -1.
DECOMP(000A96B8, Pad_ActiveIndexA96B8) {
    const u32 active = load<u8>(kPadActive);
    ctx.gpr[4] = active;
    ret(ctx, active != 0 ? static_cast<s32>(load<u8>(kPadIndex)) : -1);
}

// s32 Save_Value465D5C(const u8* user) (name?): the user's save slot +32.
DECOMP(00465D5C, Save_Value465D5C) {
    const u32 w = load<u32>(kCurrentWorld);
    ctx.gpr[4] = w;
    ret(ctx, static_cast<s32>(load<u32>(w + kSaveData + kSaveSlotSize * load<u8>(arg<u32>(ctx, 0)) + 32)));
}

// u32* Save_Set2DFFDC(-, u32 v) (name?): the current user's save slot
// +160 = v. Returns the slot.
DECOMP(002DFFDC, Save_Set2DFFDC) {
    const u32 w = load<u32>(kCurrentWorld);
    const u32 user = load<u8>(w + kWorldUser);
    const u32 slot = w + user * kSaveSlotSize + kSaveData;
    ctx.gpr[5] = user * kSaveSlotSize;
    store<u32>(slot + 160, arg<u32>(ctx, 1));
    ret(ctx, slot);
}

// u32 Obj_Entry376998(Obj* o) (name?): +16 of the definition's (+8) 56-byte
// entry (+148 table) at the index o +172.
DECOMP(00376998, Obj_Entry376998) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 table = load<u32>(load<u32>(o + 8) + 148);
    const u32 i = load<u32>(o + 172);
    ctx.gpr[4] = table;
    ctx.gpr[5] = static_cast<u64>(i) << 6;
    ret(ctx, load<u32>(table + i * 56 + 16));
}

// Wrapper_0CC4D4(r3..r8, u8 a6, u8 a7, u32 a8, -, u32 a10) (name?):
// forwards to 0x000CC188 with the two bytes masked (f1 passed through).
DECOMP(000CC4D4, Wrapper_0CC4D4) {
    const u32 sp = static_cast<u32>(ctx.gpr[1]);
    const u32 a8 = load<u32>(sp + 116), a10 = load<u32>(sp + 132);  // 9th / 11th arguments
    call(ctx, kTarget0CC188, {ctx.gpr[3], ctx.gpr[4], ctx.gpr[5], ctx.gpr[6], ctx.gpr[7], ctx.gpr[8], arg<u8>(ctx, 6), arg<u8>(ctx, 7), a8, 0, a10});
}

// Object* Singleton_Get355080() (name?): the static object at 0x0106C000,
// its vtable set on first use.
DECOMP(00355080, Singleton_Get355080) {
    if (load<u8>(kSingletonGuard) == 0) {
        store<u32>(kSingleton, kSingletonVtable);
        store<u8>(kSingletonGuard, 1);
    }
    ret(ctx, kSingleton);
}
