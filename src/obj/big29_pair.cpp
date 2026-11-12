// Game code, large-function sweep part 29: the add side of the 16-byte
// {owner, object, handle, flags} table at *0x00F65858 (the counterpart of
// PairTable_Remove_0042C7DC).
#include "../sly4.h"
#include "big29.h"

using namespace sly4;
using namespace sly4::big29;

namespace {
constexpr u32 kTable = 0x00F65858;   // Entry* (16 bytes each)
constexpr u32 kCount = 0x010AA690;   // u32 entries in use
constexpr u32 kProfileHash = 0x00014120;
constexpr u32 Save_Profile = 0x000C9FC8;     // (u32) -> profile or 0
constexpr u32 Obj_Release_000DC314 = 0x000DC314;  // (Obj*, 0, 0) (f_000DC314) (name?)
constexpr u32 Obj_SceneEnd = 0x000DD02C;     // (Obj*, u8 drop, u8 keep)
constexpr u32 Obj_SetFlag968 = 0x00124370;   // (Obj*, bool)
constexpr u32 Obj_Detach_1243D0 = 0x001243D0;       // (Obj*)
constexpr u32 Player_SetSave = 0x00124D58;   // (Player*, Def*, u8 quick)
constexpr u32 Anim_Stop = 0x00254814;        // (Animator*, key)

// The 0/1 of bit `sh` of v (rlwinm + cntlzw / xori).
inline u32 Bit(u32 v, u32 sh) { return (v >> sh) & 1; }
}  // namespace

// void f(owner a, Obj* o, u32 handle, bool top, bool r7, bool b28, bool
// b29, bool r10; stack arguments 8..11: bool s8, bool s9, bool s10, bool
// s11) (called by f_00432AB4, Cmd_CameraSplit43807C, Cmd_Fire_004387DC,
// f_00439FD0). Nothing without a and o. The key is handle (or o->+168).
// With s11 and an entry whose object's key (its +8, else the object's
// +168) matches, every entry with that object gets +0 = a and that is
// all but the end calls; else a new entry {a, o, handle, flags} is
// appended (flags +12: bit 31 top, 30 o->+172 bit 1, 29 b29, 28 b28, 27 /
// 25 bits 12 / 10 of o->+164's u16 +40, 26 o->+172 bit 3; the rest kept),
// o->+178 counts it; without s9, o's +192 bit 3 and +188 bit 5 the profile
// 0x14120 may be applied (Player_SetSave(o, p, 0) when its +20 bit 3 and
// o's +188 bit 0 are set); o->vfn[96](o, 1); o->+164's +40 gets 0x1000
// set and 0x400 cleared; with r10 o->vfn[60]() is stopped (Anim_Stop(it,
// 0)) or, without one, f_000DC314(o, 0, 0); with o's +192 bit 3
// Obj_Detach_1243D0(o) (and Obj_SetFlag968(o, 1) with s10); without s8, r7 or
// o's +200 bit 8 gives o->vfn[176](o, 1). Ends with Obj_SceneEnd(o, 1, 0)
// and o->vfn[180](o, 1) (its r3).
DECOMP(0042C328, PairTable_Add_0042C328) {
    const u32 a = arg<u32>(ctx, 0), o = arg<u32>(ctx, 1), handle = arg<u32>(ctx, 2), top = arg<u32>(ctx, 3);
    const bool f7 = static_cast<u8>(ctx.gpr[7]) != 0;
    const u32 b28 = arg<u32>(ctx, 5), b29 = arg<u32>(ctx, 6);
    const bool f10 = static_cast<u8>(ctx.gpr[10]) != 0;
    const u32 sp = static_cast<u32>(ctx.gpr[1]);
    const bool s8 = static_cast<u8>(StackArg(sp, 8)) != 0, s9 = static_cast<u8>(StackArg(sp, 9)) != 0;
    const bool s10 = static_cast<u8>(StackArg(sp, 10)) != 0, s11 = static_cast<u8>(StackArg(sp, 11)) != 0;
    if (a == 0 || o == 0) {
        ctx.gpr[3] = a;  // as the tests leave it
        return;
    }
    auto finish = [&] {
        call(ctx, Obj_SceneEnd, {o, 1, 0});
        VCall(ctx, o, 180, {o, 1});
    };
    const u32 key = handle != 0 ? handle : load<u32>(o + 168);
    if (key != 0) {
        u32 n = load<u32>(kCount);
        for (u32 i = 0; n > i && n != 0; ++i) {
            u32 base = load<u32>(kTable);
            const u32 e = base + 16 * i;
            const u32 eo = load<u32>(e + 4);
            if (eo == 0) continue;
            u32 k = load<u32>(e + 8);
            if (k == 0) k = load<u32>(eo + 168);
            if (k != key) continue;
            if (!s11) break;  // append
            // Every entry of that object gets the owner (the loop restarts
            // at 0 with the table / count as last read).
            for (u32 j = 0; n > j; ++j) {
                const u32 e2 = base + 16 * j;
                if (load<u32>(e2 + 4) == eo) {
                    store<u32>(e2, a);
                    n = load<u32>(kCount);
                    base = load<u32>(kTable);
                }
            }
            return finish();
        }
    }
    const u32 cnt = load<u32>(kCount);
    const u32 e = load<u32>(kTable) + (cnt << 4);
    store<u32>(kCount, cnt + 1);
    u32 w = load<u32>(e + 12);
    store<u32>(e, a);
    w = (w & ~0x20000000u) | ((b29 & 1) << 29);
    store<u32>(e + 4, o);
    store<u32>(e + 8, handle);
    w = (w & ~0x10000000u) | ((b28 & 1) << 28);
    store<u32>(e + 12, w);
    w = (w & ~0x40000000u) | (Bit(load<u32>(o + 172), 1) << 30);
    store<u32>(e + 12, w);
    u32 c = load<u32>(o + 164);
    w = (w & ~0x08000000u) | ((c != 0 ? Bit(load<u16>(c + 40), 12) : 0) << 27);
    store<u32>(e + 12, w);
    w = (w & ~0x04000000u) | (Bit(load<u32>(o + 172), 3) << 26);
    store<u32>(e + 12, w);
    c = load<u32>(o + 164);
    w = (w & ~0x02000000u) | ((c != 0 ? Bit(load<u16>(c + 40), 10) : 0) << 25);
    w = (w & 0x7FFFFFFFu) | ((top & 1) << 31);
    store<u32>(e + 12, w);
    store<u16>(o + 178, static_cast<u16>(load<u16>(o + 178) + 1));
    if (!s9 && (load<u32>(o + 192) & 0x8u) != 0 && (load<u32>(o + 188) & 0x20u) != 0) {
        const u32 p = static_cast<u32>(call(ctx, Save_Profile, {kProfileHash}));
        if (p != 0 && (load<u32>(p + 20) & 0x8u) != 0 && (load<u32>(o + 188) & 0x1u) != 0) call(ctx, Player_SetSave, {o, p, 0});
    }
    {
        const u32 q = load<u32>(e + 4);
        VCall(ctx, q, 96, {q, 1});
    }
    if (const u32 k = load<u32>(load<u32>(e + 4) + 164); k != 0) {
        store<u16>(k + 40, static_cast<u16>(load<u16>(k + 40) | 0x1000));
        const u32 k2 = load<u32>(load<u32>(e + 4) + 164);
        store<u16>(k2 + 40, static_cast<u16>(load<u16>(k2 + 40) & ~0x0400));
    }
    if (f10) {
        const u32 q = load<u32>(e + 4);
        const u32 an = static_cast<u32>(VCall(ctx, q, 60, {q}));
        if (an == 0)
            call(ctx, Obj_Release_000DC314, {load<u32>(e + 4), 0, 0});
        else
            call(ctx, Anim_Stop, {an, 0});
    }
    if (const u32 q = load<u32>(e + 4); q != 0 && (load<u32>(q + 192) & 0x8u) != 0) {
        call(ctx, Obj_Detach_1243D0, {q});
        if (s10) call(ctx, Obj_SetFlag968, {q, 1});
    }
    if (!s8) {
        bool on = f7;
        if (!on) {
            const u32 q = load<u32>(e + 4);
            on = q != 0 && (load<u32>(q + 200) & 0x100u) != 0;
        }
        if (on) {
            const u32 q = load<u32>(e + 4);
            VCall(ctx, q, 176, {q, 1});
        }
    }
    finish();
}
