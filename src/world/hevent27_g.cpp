// World_HandleEvent callees, part 27g: the level's geometry set {+0 bank,
// +4 / +8 objects} (created, started and destroyed around a level).
#include "hevent27.h"

using namespace sly4;
using namespace sly4::hevent;
using namespace sly4::hevent27;

namespace {
constexpr u32 kBankCreate = 0x005903F4;     // (u8) -> Bank* (Bank_Create)
constexpr u32 kBankStart = 0x005964B4;      // (Bank*) (Bank_Start)
constexpr u32 kBankDestroy = 0x00596E3C;    // (Bank*) (Bank_Destroy)
constexpr u32 kFreeListsReset = 0x005A5130; // () library free-list template (not decompiled)
constexpr u32 kWorldFreeBlock40 = 0x005A5864;     // (GeomSet*) (World_FreeBlock40)
constexpr u32 kEffectSlotsClear = 0x005AC64C;     // () (EffectSlots_Clear_005AC64C)
}  // namespace

// ---------------------------------------------------------------- 0x004EFEF4
// void GeomSet_Init4EFEF4(GeomSet* s) (name?): +0 / +4 / +8 = 0.
DECOMP(004EFEF4, GeomSet_Init4EFEF4) {
    const u32 s = arg<u32>(ctx, 0);
    store<u32>(s + 0, 0);
    store<u32>(s + 4, 0);
    store<u32>(s + 8, 0);
    ctx.gpr[4] = 0;  // the original leaves its zero in r4
}

// ---------------------------------------------------------------- 0x004EFF08
// u32 GeomSet_Destroy4EFF08(GeomSet* s) (name?): World_FreeBlock40(s),
// clears the effect slots, releases +8 and +4 (vt +8) and destroys the +0
// bank, clearing each. Returns 0.
DECOMP(004EFF08, GeomSet_Destroy4EFF08) {
    const u32 s = arg<u32>(ctx, 0);
    call(ctx, kWorldFreeBlock40, {s});
    call(ctx, kEffectSlotsClear);
    for (u32 off : {8u, 4u}) {
        const u32 o = load<u32>(s + off);
        if (o == 0) continue;
        vcall(ctx, o, 8, {o});
        store<u32>(s + off, 0);
    }
    if (const u32 b = load<u32>(s + 0); b != 0) {
        call(ctx, kBankDestroy, {b});
        store<u32>(s + 0, 0);
    }
    ret(ctx, 0u);
}

// ---------------------------------------------------------------- 0x004EFFC8
// u32 GeomSet_Create4EFFC8(GeomSet* s) (name?): +0 = Bank_Create(1).
DECOMP(004EFFC8, GeomSet_Create4EFFC8) {
    const u32 s = arg<u32>(ctx, 0);
    const u32 b = static_cast<u32>(call(ctx, kBankCreate, {1}));
    store<u32>(s + 0, b);
    ret(ctx, b);
}

// ---------------------------------------------------------------- 0x004F2FC0
// void GeomSet_Start4F2FC0(GeomSet* s) (name?): Bank_Start(+0), then the
// free-list template 0x005A5130.
DECOMP(004F2FC0, GeomSet_Start4F2FC0) {
    const u32 s = arg<u32>(ctx, 0);
    call(ctx, kBankStart, {load<u32>(s + 0)});
    call(ctx, kFreeListsReset);  // r3 as it leaves it
}
