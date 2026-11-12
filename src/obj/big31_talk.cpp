// Game code, large-function sweep part 31: closing a talking object's
// conversation (o->+356 the current line of the script o->+8).
#include "../sly4.h"
#include "big31.h"

using namespace sly4;
using namespace sly4::big31;

namespace {
constexpr u32 Timeline_Play = 0x004E2EF0;     // (const char* name, u32 timeline, 0, 0) -> handle
constexpr u32 kShutdownName = 0x00ADCB70;     // "Default_Shutdown"
constexpr u32 kQuietHash = 0x8DD33161;        // the closing text id when the line has no bytes +12 / +13
constexpr u32 kHashByA = 0x00ADC668;          // u32[]: closing text ids by the first line's u8 +12
constexpr u32 kHashByB = 0x00ADC680;          // u32[]: ... by its u8 +13 (u8 +12 = 0)
constexpr u32 kHashBoth = 0x00ADC698;         // u32[]: ... when both bytes are equal
constexpr u32 kSayConsts = 0x00371354;        // f32 1.0, f32 0.0 (the Speaker_Say floats)

// One partner link (handle at script +156 / +160): with `active` (o->+360
// bit 0x00800000 / 0x00400000) a live partner whose +8 definition has a
// +152 gets name (o +362 / +490) through its +328 component's +12 object
// (vt +188 (t, name, 0), then vt +148 (t)); otherwise a live partner not
// busy (+396 bit 0x02000000) gets Obj_NotifyA362F1C(x, 0, 0).
void Partner(PPUContext& ctx, u32 h, bool active, u32 name, u32 key) {
    store<u32>(key, h);  // the original's stack copy of the handle
    if (h == 0) return;
    const u32 x = load<u32>(static_cast<u32>(call(ctx, b31fn::SortedMap_Find3, {b31fn::kHandleMap, key})));
    if (x == 0 || (load<u32>(x + 4) & 1) == 0) return;
    if (active) {
        if (load<u32>(load<u32>(x + 8) + 152) == 0) return;
        const u32 c = load<u32>(x + 328);
        const u32 t = load<u32>(c + 12);
        if (t == 0) return;
        VCall(ctx, t, 188, {t, name, 0});
        const u32 t2 = load<u32>(c + 12);
        VCall(ctx, t2, 148, {t2});
    } else {
        if (load<u32>(x + 396) & 0x02000000u) return;
        call(ctx, b31fn::Obj_NotifyA362F1C, {x, 0, 0});
    }
}
}  // namespace

// void Obj_TalkResume(T* o) (called by Obj_ArmTarget, f_003720B0 and
// Obj_SetTalkState_003735C8). d = o->+8 the script: +148 the 16-byte lines
// ({+4 sub-entries (16 bytes: +4 Obj*, +8 u32), +8 u32 count, +12 / +13 u8}),
// +152 the line count, +156 / +160 the partner handles.
//  1. +160 = (+160 & 1) | (line +356 has both bytes zero ? 18 : 16); with
//     a timeline +340, the
//     playing +344 is stopped (vt +4) and cleared, and +344 =
//     Timeline_Play("Default_Shutdown", +340, 0, 0).
//  2. The two partners (see Partner).
//  3. The closing text: id 0x8DD33161 when line +356 has both bytes zero,
//     else by the bytes of line 0 (not +356: the original indexes the
//     tables with lines[0] +12 / +13). With a speaker +28 and the text,
//     Speaker_Say(+28, text, 0, ..., 0, 0, voice 0; 1, 0, 0) and +360 bit
//     0x04000000 set, else cleared.
//  4. +24 vt +40 (+24, 1, 1) when set.
//  5. For line +356 (< count): each sub-entry's object gets vt +88 (obj,
//     +8 != 0).
//  6. +168 = 0.
// r3 is left as the last lines pointer or the line count, as the original
// (no caller reads it).
DECOMP(00371360, Obj_TalkResume) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 d = load<u32>(o + 8);
    const u32 e = load<u32>(d + 148) + 16 * static_cast<u32>(load<u16>(o + 356));
    const bool quiet = load<u8>(e + 12) == 0 && load<u8>(e + 13) == 0;
    u32 tl = load<u32>(o + 340);
    const u32 cur = load<u32>(o + 344);
    store<u32>(o + 160, (load<u32>(o + 160) & 1) | (quiet ? 18u : 16u));
    if (tl != 0) {
        if (cur != 0) {
            VCall(ctx, cur, 4, {cur});
            tl = load<u32>(o + 340);
            store<u32>(o + 344, 0);
        }
        store<u32>(o + 344, static_cast<u32>(call(ctx, Timeline_Play, {kShutdownName, tl, 0, 0})));
    }
    {
        Stack frame(ctx, 16);  // the handle copies at 128..140(r1)
        Partner(ctx, load<u32>(d + 156), (load<u32>(o + 360) & 0x00800000u) != 0, o + 362, frame.addr() + 12);
        Partner(ctx, load<u32>(d + 160), (load<u32>(o + 360) & 0x00400000u) != 0, o + 490, frame.addr() + 4);
    }
    u32 hash = kQuietHash;
    if (!quiet) {
        const u32 l0 = load<u32>(d + 148);
        const u32 a = load<u8>(l0 + 12), b = load<u8>(l0 + 13);
        if (a == b) hash = load<u32>(kHashBoth + 4 * a);
        else if (a != 0) hash = load<u32>(kHashByA + 4 * a);
        else hash = load<u32>(kHashByB + 4 * b);
    }
    bool said = false;
    if (load<u32>(o + 28) != 0) {
        const u32 t = static_cast<u32>(call(ctx, b31fn::Text_Lookup34A398, {hash}));
        if (t != 0) {
            // r6 is the constants' address and r7 / r8 Text_Lookup's
            // leftovers in the original; the callee reads none of them.
            const f64 zero = Lfs(kSayConsts + 4);
            const u32 s = load<u32>(o + 28);
            callf(ctx, b31fn::Speaker_Say3514AC, {s, t, 0, kSayConsts, ctx.gpr[7], ctx.gpr[8], 0, 0, 0}, {Lfs(kSayConsts), zero, zero});
            said = true;
        }
    }
    const u32 f = load<u32>(o + 360);
    store<u32>(o + 360, said ? (f | 0x04000000u) : (f & ~0x04000000u));
    const u32 l = load<u32>(o + 24);
    if (l != 0) VCall(ctx, l, 40, {l, 1, 1});
    u32 idx = load<u16>(o + 356);
    const u32 n = load<u32>(d + 152);
    u32 r3 = n;
    if (idx < n) {
        u32 base = load<u32>(d + 148);
        if (load<u32>(base + 16 * idx + 8) != 0) {
            for (u32 i = 0, off = 0;;) {
                const u32 sub = load<u32>(base + 16 * idx + 4) + off;
                const u32 obj = load<u32>(sub + 4);
                if (obj != 0) {
                    VCall(ctx, obj, 88, {obj, load<u32>(sub + 8) != 0 ? 1u : 0u});
                    base = load<u32>(d + 148);
                    idx = load<u16>(o + 356);
                }
                ++i;
                off += 16;
                if (i >= load<u32>(base + 16 * idx + 8)) break;
            }
        }
        r3 = base;
    }
    store<u32>(o + 168, 0);
    ret(ctx, r3);
}
