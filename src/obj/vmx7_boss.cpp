// Game code, VMX sweep part 7 (see vmx7.h): the dragon fight's event
// handlers (the class whose message handler is 0x00301C48).
#include "../sly4.h"
#include "vmx7.h"

using namespace sly4;
using namespace sly4::vmx7;

namespace {
constexpr u32 kDragonFn2FEA20 = 0x002FEA20;  // (T* o) (reads r3 only)
constexpr u32 kDragonFn2F63E0 = 0x002F63E0;  // (T* o, u32 hash, u8; f1)
constexpr u32 kDragonPartVal868 = 0x0106ADC0;  // its +72 word goes to each part's +60 / +64 (00301868)
constexpr u32 kDragonPartVal428 = 0x0106B020;  // the same for 00301428
constexpr u32 kDragonGlobA = 0x0106A44C;  // three global objects whose vfn[3](g, 1) is called
constexpr u32 kDragonGlobB = 0x0106A498;
constexpr u32 kDragonGlobC = 0x0106A4E4;
constexpr u32 kDragonF1Src = 0x0106A57C;  // its +72 f32 is f_002F63E0's f1
constexpr u32 kDragonFxFlag = 0x0106A690;  // its +72 u8: spawn the effect once
constexpr u32 kDragonName = 0x00AD9EB0;  // "Dragon"
constexpr u32 kDragonPct868 = 0x0030185C;  // f32 {1/6, 100, 0}
constexpr u32 kDragonPct428 = 0x00301418;  // f32 {1/6, 100, 0}

// The common start of both handlers: Obj_SetFlags64(e, 0x40000, 0, 0) for
// each entry e of the list o->+1100 (u16 count +1108), f_002FEA20(o), each
// of the three parts o->+900 / +904 / +908 gets +60 = +64 = val, +69 = +68 =
// 1, +70 = 0, then the three globals' vfn[3](g, 1).
void DragonResetParts(PPUContext& ctx, u32 o, u32 valsrc) {
    for (u32 i = 0; i < load<u16>(o + 1108); ++i) call(ctx, v7fn::Obj_SetFlags64, {load<u32>(load<u32>(o + 1100) + 4 * i), 0x40000, 0, 0});
    call(ctx, kDragonFn2FEA20, {o});
    const u32 v = load<u32>(valsrc + 72);
    for (u32 k = 0; k < 3; ++k) {
        const u32 p = load<u32>(o + 900 + 4 * k);
        store<u32>(p + 60, v);
        store<u32>(p + 64, v);
        store<u8>(p + 69, 1);
        store<u8>(p + 68, 1);
        store<u8>(p + 70, 0);
    }
    for (u32 g : {kDragonGlobA, kDragonGlobB, kDragonGlobC}) VCall(ctx, g, 12, {g, 1});
}

// Each entry c of the list o->+1032 (u16 count +1040): Pending_Flush(c, 1),
// c->vfn[7](c, 0).
void DragonFlushList(PPUContext& ctx, u32 o) {
    for (u32 i = 0; i < load<u16>(o + 1040); ++i) {
        const u32 c = load<u32>(load<u32>(o + 1032) + 4 * i);
        call(ctx, v7fn::Pending_Flush, {c, 1});
        VCall(ctx, c, 28, {c, 0});
    }
}

// The percentage (base + the parts whose +69 is set) / 6 * 100, rounded to
// single at each step.
f32 DragonPct(u32 o, u32 base, u32 k) {
    u32 n = base;
    for (u32 j = 0; j < 3; ++j)
        if (load<u8>(load<u32>(o + 900 + 4 * j) + 69) != 0) ++n;
    return Muls(Muls(static_cast<f32>(static_cast<f64>(static_cast<s64>(static_cast<s32>(n)))), Lfs4(k)), Lfs4(k + 4));
}
}  // namespace

// void f(Dragon* o): handler of message 0x41E42CF8. Resets the parts (see
// DragonResetParts), names o the "Dragon" global and flushes the pending
// object, o->vfn[32](o, 0x29E5A3CC, 1); spawns effect 0x749321C8 once (o->
// +536) when the global flag byte is set; flushes the list +1032;
// f_002F63E0(o, 0x1E9C9280, 1; f1 = the global f32); then +1096 = 0, +537 =
// 1, the 96 bytes at +432 zeroed (stvx), +440 = 2.0, +512 = 3, +432 = 1,
// +528 = 6, and +348 = max(0, min(pct, +344)) with pct counting from 3.
DECOMP(00301868, Dragon_OnMsg41E42CF8_00301868) {
    const u32 o = arg<u32>(ctx, 0);
    DragonResetParts(ctx, o, kDragonPartVal868);
    call(ctx, v7fn::Global_SetHashObj, {o, kDragonName});
    call(ctx, v7fn::Objects_Flush, {});
    VCall(ctx, o, 128, {o, 0x29E5A3CC, 1});
    if (load<u8>(kDragonFxFlag + 72) != 0 && load<u8>(o + 536) == 0) {
        call(ctx, v7fn::Effect_Spawn, {0x749321C8, 0, 0, 1});
        store<u8>(o + 536, 1);
    }
    DragonFlushList(ctx, o);
    callf(ctx, kDragonFn2F63E0, {o, 0x1E9C9280, 1}, {Lfs4(kDragonF1Src + 72)});
    store<u32>(o + 1096, 0);
    store<u8>(o + 537, 1);
    for (u32 k = 0; k < 6; ++k) Stvx(o + 432 + 16 * k, Zero4());
    store<u32>(o + 440, 0x40000000);
    store<u32>(o + 512, 3);
    store<u32>(o + 432, 1);
    store<u32>(o + 528, 6);
    const f64 s = Lfs4(o + 344);
    const f64 pct = DragonPct(o, 3, kDragonPct868);
    const f64 m = pct < s ? pct : s;
    if (m > Lfs4(kDragonPct868 + 8)) {
        store<f32>(o + 348, static_cast<f32>(m));
        ctx.fpr[1] = m;
    } else {
        store<u32>(o + 348, load<u32>(kDragonPct868 + 8));
        ctx.fpr[1] = s;
    }
    call(ctx, v7fn::Global_F64CB8_Copy216, {});
}

// void f(Dragon* o): handler of message 0x41E42CF9. Resets the parts (see
// DragonResetParts); unless +537 is set, names o the "Dragon" global and
// flushes the pending object; o->vfn[32](o, 0x2B9A7C6B, 1); flushes the
// list +1032; f_002F63E0(o, 0xA4F1083F, 0; f1 = the global f32); then
// +1096 = 0, the 96 bytes at +432 zeroed (stvx), +432 = 2, +512 = 3. With
// +537 set, each of the three slots +916 / +924 / +932 that holds an
// object q gets q->vfn[32](q, 0x9F60B8DA, 1) and is cleared (all three are
// cleared); without, +504 = 1 and +500 = 1.0. Then +528 = 8 and +348 =
// max(0, min(pct, +344)), pct counting from 3 when +432 (re-read) is 1,
// else from 0. f1 is left as pct.
DECOMP(00301428, Dragon_OnMsg41E42CF9_00301428) {
    const u32 o = arg<u32>(ctx, 0);
    DragonResetParts(ctx, o, kDragonPartVal428);
    if (load<u8>(o + 537) == 0) {
        call(ctx, v7fn::Global_SetHashObj, {o, kDragonName});
        call(ctx, v7fn::Objects_Flush, {});
    }
    VCall(ctx, o, 128, {o, 0x2B9A7C6B, 1});
    DragonFlushList(ctx, o);
    callf(ctx, kDragonFn2F63E0, {o, 0xA4F1083F, 0}, {Lfs4(kDragonF1Src + 72)});
    store<u32>(o + 1096, 0);
    for (u32 k = 0; k < 6; ++k) Stvx(o + 432 + 16 * k, Zero4());
    store<u32>(o + 432, 2);
    store<u32>(o + 512, 3);
    if (load<u8>(o + 537) != 0) {
        for (u32 k = 0; k < 3; ++k) {
            const u32 slot = o + 916 + 8 * k;
            const u32 q = load<u32>(slot);
            if (q != 0) VCall(ctx, q, 128, {q, 0x9F60B8DA, 1});
            store<u32>(slot, 0);
        }
    } else {
        store<u8>(o + 504, 1);
        store<u32>(o + 500, 0x3F800000);
    }
    store<u32>(o + 528, 8);
    const u32 base = load<u32>(o + 432) == 1 ? 3 : 0;
    const f64 s = Lfs4(o + 344);
    const f64 pct = DragonPct(o, base, kDragonPct428);
    const f64 m = pct < s ? pct : s;
    if (m > Lfs4(kDragonPct428 + 8)) store<f32>(o + 348, static_cast<f32>(m));
    else store<u32>(o + 348, load<u32>(kDragonPct428 + 8));
    ctx.fpr[1] = pct;
    call(ctx, v7fn::Global_F64CB8_Copy216, {});
}
