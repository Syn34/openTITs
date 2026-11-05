// World_HandleEvent callees, part 21k: a sound pool reset, the world part
// object's vector teardown and an object's state effect switch.
#include "hevent19.h"

using namespace sly4;
using namespace sly4::hevent;

namespace {
constexpr u32 kMemFreeFrom = 0x0054EE6C;     // (p, pool) (symbols.txt Mem_FreeFrom)
constexpr u32 kPtrVectorClear = 0x0054EC1C;  // (PtrVector*) (as hevent.h)
constexpr u32 kNop = 0x0054EC18;             // (x) (symbols.txt Nop)
constexpr u32 kSmallFree = 0x00552420;       // (p, size) (as hevent7.h)
constexpr u32 kWorldPartObj = 0x128164;      // World +0x128164: the part object (as hevent20 Session_PartDone2E2740)
constexpr u32 kPartObjBytes = 5584;
constexpr u32 kEffectSetHash = 0x004CF988;   // (Effect*, u32 hash) (hevent21_g.cpp)
constexpr u32 kStateHashes = 0x00AD4CB4;     // u32[] effect hash per state (name?)
}  // namespace

// u32 Pool_Reset4545B0(Pool* p) (name?): frees the blocks of the {key,
// block} table +36 (u16 +30 count; Mem_FreeFrom), releases the objects of
// the {key, Object*} table +12 (u16 +6 count; vt +4), zeroes both counts
// and relinks the 16-byte entry pool (+80, +76 s32 capacity: prev k-1,
// next k+1, freed, serial cleared; ends -1, the last-entry store landing
// 12 bytes before the table for capacity 0) with +60 / +64 = -1, +52 = 0,
// +56 = capacity - 1 (-1 for capacity <= 1), +68 = 0, +72 = capacity.
// Returns the capacity.
DECOMP(004545B0, Pool_Reset4545B0) {
    const u32 p = arg<u32>(ctx, 0);
    const u32 nb = load<u16>(p + 30);
    for (u32 i = 0; static_cast<s32>(i) < static_cast<s32>(nb); ++i) {
        const u32 b = load<u32>(load<u32>(p + 36) + 8 * i + 4);
        if (b != 0) call(ctx, kMemFreeFrom, {b, 0});
    }
    const u32 no = load<u16>(p + 6);
    store<u16>(p + 30, 0);
    for (u32 i = 0; static_cast<s32>(i) < static_cast<s32>(no); ++i) {
        const u32 o = load<u32>(load<u32>(p + 12) + 8 * i + 4);
        if (o != 0) vcall(ctx, o, 4, {o});
    }
    u32 count = load<u32>(p + 76);
    store<u16>(p + 6, 0);
    if (static_cast<s32>(count) > 0) {
        u32 k = 0;
        do {
            const u32 e = load<u32>(p + 80) + 16 * k;
            store<u32>(e, k - 1);
            store<u32>(e + 4, k + 1);
            store<u8>(e + 12, load<u8>(e + 12) | 0x80);
            const u32 w = load<u32>(p + 80) + 16 * k + 12;
            store<u32>(w, load<u32>(w) & ~0x7FFF8000u);
            ++k;
            count = load<u32>(p + 76);
        } while (static_cast<s32>(k) < static_cast<s32>(count));
    }
    const u32 table = load<u32>(p + 80);
    store<u32>(table, 0xFFFFFFFFu);
    store<u32>(table + (count << 4) - 12, 0xFFFFFFFFu);
    store<u32>(p + 60, 0xFFFFFFFFu);
    store<u32>(p + 64, 0xFFFFFFFFu);
    store<u32>(p + 52, 0);
    store<u32>(p + 56, static_cast<s32>(count) > 1 ? count - 1 : 0xFFFFFFFFu);
    store<u32>(p + 68, 0);
    store<u32>(p + 72, count);
    ret(ctx, count);
}

// u32 World_FreePartObj47E1D4(World* w) (name?): unless u8 +28 bits 0xC0
// are set, the part object (+0x128164) has its seven vectors (+5452,
// +5448, +16, +12, +8, +4, +0) cleared and freed (20 bytes each) and is
// freed itself (5584 bytes); the pointer is cleared.
DECOMP(0047E1D4, World_FreePartObj47E1D4) {
    const u32 w = arg<u32>(ctx, 0);
    const u32 busy = (load<u8>(w + 28) >> 6) & 3u;
    ctx.gpr[3] = busy;
    if (busy != 0) return;
    const u32 po = load<u32>(w + kWorldPartObj);
    ctx.gpr[3] = kWorldPartObj & 0xFFFF0000u;  // lis r3 of the offset
    if (po == 0) return;
    for (u32 off : {5452u, 5448u, 16u, 12u, 8u, 4u, 0u}) {
        const u32 v = load<u32>(po + off);
        if (v == 0) continue;
        call(ctx, kPtrVectorClear, {v});
        call(ctx, kNop, {v});
        call(ctx, kSmallFree, {load<u32>(po + off), 20});
    }
    call(ctx, kSmallFree, {po, kPartObjBytes});
    store<u32>(w + kWorldPartObj, 0);
    ret(ctx, kWorldPartObj);
}

// u32 Obj_SetStateFx266E6C(Object* o, s32 state) (name?): the owner's
// (+8) effect (vt +272) is switched to the state's hash (0x00AD4CB4[state])
// with its live bit (+136 0x100) forced on for the switch and restored
// after; the object's own effect (+108) is switched too. +68 = state; with
// +124 bit 0x01000000 the owner gets vt +96 (x, state == 2).
DECOMP(00266E6C, Obj_SetStateFx266E6C) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 k = arg<u32>(ctx, 1);
    const u32 hash_ea = kStateHashes + static_cast<u32>(static_cast<s32>(k << 2));
    u32 x = load<u32>(o + 8);
    if (x != 0 && static_cast<u32>(vcall(ctx, x, 272, {x})) != 0) {
        x = load<u32>(o + 8);
        u32 e = static_cast<u32>(vcall(ctx, x, 272, {x}));
        x = load<u32>(o + 8);
        const u32 was_off = ((load<u32>(e + 136) >> 8) & 1u) == 0 ? 1u : 0u;
        e = static_cast<u32>(vcall(ctx, x, 272, {x}));
        store<u32>(e + 136, load<u32>(e + 136) | 0x100u);
        x = load<u32>(o + 8);
        e = static_cast<u32>(vcall(ctx, x, 272, {x}));
        call(ctx, kEffectSetHash, {e, load<u32>(hash_ea)});
        x = load<u32>(o + 8);
        e = static_cast<u32>(vcall(ctx, x, 272, {x}));
        store<u32>(e + 136, (load<u32>(e + 136) & ~0x100u) | ((was_off ^ 1u) << 8));
    }
    const u32 own = load<u32>(o + 108);
    if (own != 0) call(ctx, kEffectSetHash, {own, load<u32>(hash_ea)});
    const u32 fl = load<u32>(o + 124);
    store<u32>(o + 68, k);
    const u32 bit = (fl >> 24) & 1u;
    ctx.gpr[3] = bit;
    if (bit == 0) return;
    x = load<u32>(o + 8);
    vcall(ctx, x, 96, {x, static_cast<s32>(k) == 2 ? 1u : 0u});
}
