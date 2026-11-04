// World_HandleEvent callees, part 3: 59..65 instructions.
#include "hevent.h"

using namespace sly4;
using namespace sly4::hevent;

namespace sly4 {
namespace hevent3 {
namespace h3fn {
constexpr u32 qsort = 0x0071EC28;              // (base, count, size, compare opd)
constexpr u32 sub_000D49BC = 0x000D49BC;       // ()
constexpr u32 sub_000D46F8 = 0x000D46F8;       // ()
constexpr u32 sub_000D3EF0 = 0x000D3EF0;       // ()
constexpr u32 sub_000D06B0 = 0x000D06B0;       // ()
constexpr u32 sub_00225E8C = 0x00225E8C;       // ()
constexpr u32 Ui_SendEvent = 0x004D3DA4;       // (type, a, b, c)
constexpr u32 sub_004E2EF0 = 0x004E2EF0;       // (const char* name, Object*, 0, 0) -> u32 (name?)
constexpr u32 StringTable_Lookup = 0x004F87C4; // (Strings*, id, fallback, quiet)
constexpr u32 sub_00454320 = 0x00454320;       // (Object*)
constexpr u32 sub_00453FC4 = 0x00453FC4;       // (Group*, f32)
constexpr u32 sub_00390D9C = 0x00390D9C;       // (u32)
constexpr u32 sub_004B2398 = 0x004B2398;       // (u32)
constexpr u32 sub_00363890 = 0x00363890;       // (Object*, a, -, u8, e, f, f32) (name?)
constexpr u32 sub_0036341C = 0x0036341C;       // (Object*)
constexpr u32 String_CopyN = 0x00553638;       // (dst, src, n)
}  // namespace h3fn

// Sorted table at 0x01013B04 (layout as resource/tags.h's table):
//   +2 u16 count  +6 u16 flags (bit 0x1 = needs sorting)  +8 entries (8 bytes)  +20 compare OPD
constexpr u32 kSortTable = 0x01013B04;
// Callback list at 0x01013B84: +0 entries {a, b, opd} (12 bytes), +8 u16 count (name?)
constexpr u32 kCallList = 0x01013B84;

// Save error messages.
constexpr u32 kStrings = 0x00F6687C;           // u32 Strings* (0 = none)
constexpr u32 kSaveErrorCode = 0x00F9D1BC;     // s32: 0 not your data, 1 age, 2 parental, 3 sign-in (name?)
constexpr u32 kStrNotYourData = 0x00AC60DC, kMissNotYourData = 0x00AC60F4;
constexpr u32 kStrVerifyAge = 0x00AC611C, kMissVerifyAge = 0x00AC613C;
constexpr u32 kStrParental = 0x00AC6164, kMissParental = 0x00AC6184;
constexpr u32 kStrSignIn = 0x00AC61A4, kMissSignIn = 0x00AC61C4;
constexpr u32 kMissSaveError = 0x00AC61EC;     // "[Missing save error code string]"

constexpr u32 kStrDefaultShutdown = 0x00ADCB70; // "Default_Shutdown"

constexpr u32 kGlobalF6578C = 0x00F6578C;      // u32 (name?)
constexpr u32 kGlobalF557F8 = 0x00F557F8;      // u32 object; +4 passed to sub_004B2398 (name?)
}  // namespace hevent3
}  // namespace sly4

using namespace sly4::hevent3;

// u32 Scene_SortAndRunCallbacks(): re-sorts the table at 0x01013B04 when its
// dirty bit is set, runs four per-frame steps, calls every {a, b, opd} entry of
// the list at 0x01013B84 as opd(a, b), then sub_00225E8C() (its r3 is the result) (name?)
DECOMP(000D4D30, Scene_SortAndRunCallbacks) {
    const u16 flags = load<u16>(kSortTable + 6);
    if (flags & 1) {
        const u16 n = load<u16>(kSortTable + 2);
        store<u16>(kSortTable + 6, static_cast<u16>(flags & 0xFFFE));
        if (static_cast<s32>(n) > 0) {
            const u32 cmp = load<u32>(kSortTable + 20);
            ctx.gpr[7] = cmp;  // as in the original (ori r6, r7)
            call(ctx, h3fn::qsort, {load<u32>(kSortTable + 8), n, 8, cmp});
        }
    }
    call(ctx, h3fn::sub_000D49BC);
    call(ctx, h3fn::sub_000D46F8);
    call(ctx, h3fn::sub_000D3EF0);
    call(ctx, h3fn::sub_000D06B0);
    for (u32 i = 0; i < load<u16>(kCallList + 8); ++i) {
        const u32 e = load<u32>(kCallList) + i * 12;
        const u32 opd = load<u32>(e + 8);
        // r5 / r6 hold the OPD and its entry address, as in the original.
        ctx.gpr[5] = opd;
        ctx.gpr[6] = load<u32>(opd);
        call_ptr(ctx, opd, {load<u32>(e), load<u32>(e + 4)});
    }
    call(ctx, h3fn::sub_00225E8C);
}

// u32 Obj_SelectEntry(Object* o, s32 id): finds `id` in the 16-byte entry table of
// o->+8 (+148 data, +152 count). Found: +356 = index, +358 = old +356; when the
// state (+160 >> 1) is 5 it becomes 7, and with +340 set and no +344 yet,
// +344 = sub_004E2EF0("Default_Shutdown", +340, 0, 0) and Ui_SendEvent(14, -1,
// +340, 0). Returns the count when not found (name?)
DECOMP(003734B0, Obj_SelectEntry) {
    const u32 o = arg<u32>(ctx, 0);
    const s32 id = arg<s32>(ctx, 1);
    const u32 t = load<u32>(o + 8);
    const u32 n = load<u32>(t + 152);
    u32 i = 0;
    for (; i < n; ++i)
        if (load<s32>(load<u32>(t + 148) + i * 16) == id) break;
    if (i >= n) return ret(ctx, i);  // n == 0 -> 0
    const u16 old = load<u16>(o + 356);
    store<u16>(o + 356, static_cast<u16>(i));
    const u32 st = load<u32>(o + 160);
    store<u16>(o + 358, old);
    if (static_cast<s32>(st >> 1) != 5) return ret(ctx, st);
    store<u32>(o + 160, (st & 1u) | (7u << 1));
    const u32 owner = load<u32>(o + 340);
    if (owner == 0) return ret(ctx, 0u);
    if (load<u32>(o + 344) != 0) return ret(ctx, owner);
    store<u32>(o + 344, static_cast<u32>(call(ctx, h3fn::sub_004E2EF0, {kStrDefaultShutdown, owner, 0, 0})));
    call(ctx, h3fn::Ui_SendEvent, {14, 0xFFFFFFFFu, load<u32>(o + 340), 0});
}

// const char* Save_ErrorMessage(): the text for the save error code at
// 0x00F9D1BC (0 not your data, 1 age check, 2 parental restrictions, 3 sign-in);
// other codes or no string table give "[Missing save error code string]" (name?)
DECOMP(000B00FC, Save_ErrorMessage) {
    const u32 strings = load<u32>(kStrings);
    if (strings == 0) return ret(ctx, kMissSaveError);
    const s32 code = load<s32>(kSaveErrorCode);
    u32 id, miss;
    switch (code) {
    case 0: id = kStrNotYourData, miss = kMissNotYourData; break;
    case 1: id = kStrVerifyAge, miss = kMissVerifyAge; break;
    case 2: id = kStrParental, miss = kMissParental; break;
    case 3: id = kStrSignIn, miss = kMissSignIn; break;
    default: return ret(ctx, kMissSaveError);
    }
    call(ctx, h3fn::StringTable_Lookup, {strings, id, miss, 0});
}

// u32 Group_UpdateAll(Group* g): +0 = 1 while it runs: vfunc +16() on every
// non-null object of the first list (+12 entries {?, Object*}, +6 u16 count),
// sub_00454320 on every object of the second (+36, +30 u16 count), then
// sub_00453FC4(g, 0.0) (its r3 is the result); +0 = 0 (name?)
DECOMP(004544B0, Group_UpdateAll) {
    const u32 g = arg<u32>(ctx, 0);
    store<u8>(g, 1);
    for (u32 i = 0; i < load<u16>(g + 6); ++i) {
        const u32 e = load<u32>(load<u32>(g + 12) + i * 8 + 4);
        if (e != 0) vcall(ctx, e, 16, {e});
    }
    for (u32 i = 0; i < load<u16>(g + 30); ++i)
        call(ctx, h3fn::sub_00454320, {load<u32>(load<u32>(g + 36) + i * 8 + 4)});
    callf(ctx, h3fn::sub_00453FC4, {g}, {static_cast<f64>(load<f32>(0x004544ACu))});  // 0.0f
    store<u8>(g, 0);
}

// void Obj_InitSlots(Object* o): clears +0, +4 and +24, runs
// sub_00390D9C(*0x00F6578C) and sub_004B2398(*0x00F557F8 + 4), then points the
// 2 x 8 words at +3732 to the 80-byte blocks at +3808 (640 bytes per row) (name?)
DECOMP(0001AA34, Obj_InitSlots) {
    const u32 o = arg<u32>(ctx, 0);
    store<u32>(o + 0, 0);
    const u32 a = load<u32>(kGlobalF6578C);
    store<u32>(o + 4, 0);
    call(ctx, h3fn::sub_00390D9C, {a});
    call(ctx, h3fn::sub_004B2398, {load<u32>(kGlobalF557F8) + 4});
    store<u32>(o + 24, 0);
    for (u32 k = 0; k < 2; ++k)
        for (u32 j = 0; j < 8; ++j) store<u32>(o + 3732 + k * 32 + j * 4, o + 3808 + k * 640 + j * 80);
}

// u32 Obj_SetAnim(Object* o, const char* name, u8 now, a, u8 b, c, d, f32 x):
// unless o is 0 or +396 has 0x02000000, sets +396 |= 0x40000000; with `now`,
// sub_00363890(o, name, c, b, c, d, x); otherwise, unless +396 has 0x08000000 or
// the state (+160 >> 1) is 4, swaps 0x04000000 for 0x08000000 and copies a
// non-empty name to +164 (128 bytes). Then sub_0036341C(o) (name?)
// With o == 0 or the 0x02000000 bit, r3 is left holding `d` (r9), as in the original.
DECOMP(00363A3C, Obj_SetAnim) {
    const u32 o = arg<u32>(ctx, 0), name = arg<u32>(ctx, 1);
    const u32 now = arg<u32>(ctx, 2) & 0xFF;
    const u32 b = arg<u32>(ctx, 4) & 0xFF;
    const u64 c = ctx.gpr[8], d = ctx.gpr[9];
    ctx.gpr[10] = o;
    ctx.gpr[6] = name;
    ctx.gpr[7] = b;
    ctx.gpr[4] = c;
    ctx.gpr[3] = d;
    if (o == 0) return;
    const u32 w = load<u32>(o + 396);
    ctx.gpr[8] = w;
    ctx.gpr[9] = (w >> 25) & 1;
    if (w & 0x02000000u) return;
    const u32 w2 = w | 0x40000000u;
    store<u32>(o + 396, w2);
    if (now != 0) {
        ctx.gpr[9] = now;
        ctx.gpr[5] = c;  // r5 = r7 = c in the original
        call(ctx, h3fn::sub_00363890, {o, name, c, b, c, d});  // f1 passes through
    } else if ((w2 & 0x08000000u) == 0 && static_cast<s32>(load<u32>(o + 160) >> 1) != 4) {
        store<u32>(o + 396, (w2 & ~0x04000000u) | 0x08000000u);
        if (name != 0 && load<u8>(name) != 0) call(ctx, h3fn::String_CopyN, {o + 164, name, 128});
    }
    call(ctx, h3fn::sub_0036341C, {o});
}
