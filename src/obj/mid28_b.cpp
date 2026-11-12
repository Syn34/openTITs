// Game code, medium sweep part 28: the spawn-slot pools of the section
// objects (Obj_Dtor_B1EFD0_0033D008): mode switch, per-frame tick, and the
// section's update message that drives them.
#include "mid28.h"

using namespace sly4;
using namespace sly4::mid28;

namespace {
using namespace m28fn;
constexpr u32 kAcquire = 0x0033C130u;     // Pool_Acquire_0033C130(pool, slot)
constexpr u32 kRandLast = 0x01164888u;    // u64: the last random value (low 16 bits)
constexpr u32 kRandState = 0x01164890u;   // u32 mixer state
constexpr u32 kRandMix = 0x87654321u;

inline u32 Rotl28(u32 x, u32 n) { return (x << n) | (x >> (32 - n)); }

// The inlined random delay of a free pool slot e: r = *0x01164888 (s64)
// converted to f32, the state 0x01164890 stepped (x = rotl(s, 4) + rotl(s,
// 16) ^ 0x87654321; state = rotl(x, 16), *0x01164888 = x & 0xFFFF), e->+0
// = 0 and e->+4 = lo + (hi - lo) * (r * scale) with lo / hi = pool +56 /
// +60. Returns the value stored (the original's f1 or f2).
f64 RandDelay28(u32 pool, u32 e, f64 scale) {
    const s64 v = static_cast<s64>(load<u64>(kRandLast));
    const f64 lo = Lf(pool + 56), hi = Lf(pool + 60);
    const u32 s = load<u32>(kRandState);
    const f64 d = Sgl28(hi - lo);
    const f64 r = Sgl28(static_cast<f64>(v));
    const u32 x = (Rotl28(s, 4) + Rotl28(s, 16)) ^ kRandMix;
    store<u32>(kRandState, Rotl28(x, 16));
    store<u64>(kRandLast, x & 0xFFFFu);
    store<u32>(e, 0);
    const f64 t = Sgl28(std::fma(d, Sgl28(r * scale), lo));
    store<u32>(e + 4, Bits28(t));
    return t;
}

// The number of slots of the pool's vector +72 (data +72) among the first
// n whose owner word (+0) is set (the data re-read each step).
u32 CountActive28(u32 o, u32 n) {
    u32 c = 0;
    for (u32 j = 0; j < n; ++j)
        if (load<u32>(load<u32>(load<u32>(o + 72) + 4 * j)) != 0) ++c;
    return c;
}

// The first of the first n slots (data re-read each step) that is free
// (owner +0 == 0) and not flagged (u8 +8 == 0), or 0.
u32 FindFree28(u32 o, u32 n) {
    for (u32 j = 0; j < n; ++j) {
        const u32 e = load<u32>(load<u32>(o + 72) + 4 * j);
        if (load<u32>(e) == 0 && load<u8>(e + 8) == 0) return e;
    }
    return 0;
}
}  // namespace

// u32 f(Pool* o, s32 mode): o->+64 = mode, the elapsed time +152 = 0. Mode
// 1 (start): slots move from the free vector +92 (first element taken,
// the last moved into its place) to the vector +72 until it holds +48
// (s32) of them; then, while fewer than +44 are active (counted once, then
// one more per attempt, and only while +36 < +52), Pool_Acquire(o, the
// first free unflagged slot or 0); finally every free slot gets a random
// delay in [+56, +60] (scale *0x0033C2B8). r3 / f1 as the original leaves
// them (r3 = o for other modes, else the last slot count).
DECOMP(0033C2C0, Pool_SetMode_0033C2C0) {
    const u32 o = arg<u32>(ctx, 0);
    const s32 mode = arg<s32>(ctx, 1);
    store<u32>(o + 64, static_cast<u32>(mode));
    store<u32>(o + 152, 0);
    if (mode != 1) {
        ctx.gpr[3] = o;
        return;
    }
    store<u32>(o + 36, 0);
    for (s32 k = load<u16>(o + 80); k < static_cast<s32>(load<u32>(o + 48)); ++k) {
        const u32 nf = load<u16>(o + 100);
        if (nf == 0) break;
        const u32 m = (nf - 1) & 0xFFFFu;
        const u32 fd = load<u32>(o + 92);
        const u32 e = load<u32>(fd);
        store<u16>(o + 100, static_cast<u16>(m));
        if (m != 0) store<u32>(fd, load<u32>(fd + ((m << 2) & 0x3FFFCu)));
        VecPush28(ctx, o, 72, e);
    }
    u32 n = load<u16>(o + 80);
    s32 act = static_cast<s32>(CountActive28(o, n));
    while (act < static_cast<s32>(load<u32>(o + 44))) {
        if (static_cast<s32>(load<u32>(o + 36)) < static_cast<s32>(load<u32>(o + 52))) {
            call(ctx, kAcquire, {o, FindFree28(o, n)});
            n = load<u16>(o + 80);
        }
        ++act;
    }
    const f64 scale = Lf(0x0033C2B8u);
    ctx.fpr[1] = scale;
    u32 i = 0;
    for (; i < n; ++i) {
        const u32 e = load<u32>(load<u32>(o + 72) + 4 * i);
        if (load<u32>(e) == 0 && e != 0) RandDelay28(o, e, scale);
    }
    ctx.gpr[3] = i;
}

// u32 f(Pool* o, f32 dt): +152 += dt. Mode 1: each free slot of +72
// counts its delay +4 down (clamped at 0); at 0 it is acquired when fewer
// than +48 slots are active; then while fewer than +44 are active (one
// more per attempt) Pool_Acquire(o, the first free unflagged slot or 0);
// then every slot's u8 +8 = 0 (r3 = 4 * count). Mode 2 (drain): free
// slots get a random delay (scale *0x0033C79C) and every free (or null)
// slot moves to the free vector +92, the last one taking its place (r3 =
// the remaining count). r3 / f1 as the original leaves them.
DECOMP(0033C7A0, Pool_Tick_0033C7A0) {
    const u32 o = arg<u32>(ctx, 0);
    const f64 dt = ctx.fpr[1];
    ctx.fpr[1] = Sgl28(Lf(o + 152) + dt);
    const s32 mode = static_cast<s32>(load<u32>(o + 64));
    store<u32>(o + 152, Bits28(ctx.fpr[1]));
    if (mode == 1) {
        const f64 zero = Lf(0x0033C798u);
        u32 n;
        for (s32 i = 0; i < static_cast<s32>(n = load<u16>(o + 80)); ++i) {
            const u32 data = load<u32>(o + 72);
            const u32 sp = data + 4 * static_cast<u32>(i);
            const u32 e = load<u32>(sp);
            if (load<u32>(e) != 0) continue;
            ctx.fpr[1] = Sgl28(Lf(e + 4) - dt);
            store<u32>(e + 4, Bits28(ctx.fpr[1]));
            if (ctx.fpr[1] < zero) store<u32>(e + 4, 0);
            const u32 s = load<u32>(sp);
            ctx.fpr[1] = Lf(s + 4);
            if (!(ctx.fpr[1] == zero)) continue;
            u32 c = 0;
            for (u32 j = 0; j < n; ++j)
                if (load<u32>(load<u32>(data + 4 * j)) != 0) ++c;
            if (static_cast<s32>(c) < static_cast<s32>(load<u32>(o + 48))) call(ctx, kAcquire, {o, s});
        }
        s32 act = static_cast<s32>(CountActive28(o, n));
        while (act < static_cast<s32>(load<u32>(o + 44))) {
            call(ctx, kAcquire, {o, FindFree28(o, n)});
            ++act;
            n = load<u16>(o + 80);
        }
        u32 r3 = 0;
        for (u32 k = 0; k < n; ++k) {
            store<u8>(load<u32>(load<u32>(o + 72) + 4 * k) + 8, 0);
            r3 += 4;
        }
        ctx.gpr[3] = r3;
        return;
    }
    if (mode != 2) {
        ctx.gpr[3] = o;
        return;
    }
    const f64 scale = Lf(0x0033C79Cu);
    u32 n;
    for (s32 i = 0; i < static_cast<s32>(n = load<u16>(o + 80)); ++i) {
        const u32 sp = load<u32>(o + 72) + 4 * static_cast<u32>(i);
        const u32 e = load<u32>(sp);
        if (load<u32>(e) != 0) continue;
        if (e != 0) ctx.fpr[1] = RandDelay28(o, e, scale);
        const u32 cap = load<u16>(o + 96);
        if (load<u16>(o + 100) == cap) call(ctx, VecReserve, {o + 92, (cap << 1) & 0x1FFFEu});
        const u32 c = load<u16>(o + 100);
        store<u32>(load<u32>(o + 92) + ((c << 2) & 0x3FFFCu), load<u32>(sp));
        const u32 c1 = load<u16>(o + 100) + 1;
        const u32 peak = load<u16>(o + 102);
        const u32 m = load<u16>(o + 80) - 1;
        const u32 d = load<u32>(o + 72);
        store<u16>(o + 100, static_cast<u16>(c1));
        store<u16>(o + 102, static_cast<u16>(static_cast<s32>(peak) > static_cast<s32>(c1 & 0xFFFFu) ? peak : c1));
        store<u16>(o + 80, static_cast<u16>(m));
        if (static_cast<u32>(i) != (m & 0xFFFFu)) store<u32>(d + 4 * static_cast<u32>(i), load<u32>(d + ((m << 2) & 0x3FFFCu)));
        --i;
    }
    ctx.gpr[3] = n;
}

// u32 f(Section* o, a, Msg* m): on the update message (u8 o->+18 == 4,
// m->+0 == 6, dt = m->+24): the wait +2616 counts down (clamped at 0),
// f_0033DE30(o; dt), then while no wait is pending the commands of +2592
// (u16 count +2600) run from index +2612: type 1 finds the pool of group
// cmd->+4 among +2572 (u16 count +2580; pool +32) and sets its mode (u8
// cmd->+32: copies cmd->+4..+24 to pool +40..+60 and mode 1, else mode 2;
// unknown: String_Format("can't find group %u")), type 2 waits cmd->+28;
// then each pool ticks by dt. Ends with Loader_Step_003252D8(o, a, m) (r3
// = its).
DECOMP(0033DFC8, Section_Update_0033DFC8) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 a = arg<u32>(ctx, 1);
    const u32 msg = arg<u32>(ctx, 2);
    if (load<u8>(o + 18) == 4 && load<u32>(msg) == 6) {
        const f64 dt = Lf(msg + 24);
        const f64 zero = Lf(0x0033DFC4u);
        ctx.fpr[1] = Lf(o + 2616);
        if (!(ctx.fpr[1] == zero)) {
            ctx.fpr[1] = Sgl28(ctx.fpr[1] - dt);
            store<u32>(o + 2616, Bits28(ctx.fpr[1]));
            if (ctx.fpr[1] < zero) store<u32>(o + 2616, 0);
        }
        callf(ctx, 0x0033DE30u, {o}, {dt});
        u32 idx = load<u32>(o + 2612);
        for (;;) {
            ctx.fpr[1] = Lf(o + 2616);
            if (!(ctx.fpr[1] == zero) || idx >= load<u16>(o + 2600)) break;
            const u32 cmd = load<u32>(load<u32>(o + 2592) + 4 * idx);
            const s32 type = static_cast<s32>(load<u32>(cmd));
            if (type == 2) {
                ctx.fpr[1] = Lf(cmd + 28);
                store<u32>(o + 2616, load<u32>(cmd + 28));
            } else if (type == 1) {
                const u32 gid = load<u32>(cmd + 4);
                u32 g = 0;
                for (u32 j = 0; j < load<u16>(o + 2580); ++j) {
                    const u32 p = load<u32>(load<u32>(o + 2572) + 4 * j);
                    if (load<u32>(p + 32) == gid) {
                        g = p;
                        break;
                    }
                }
                if (g == 0) {
                    call(ctx, 0x005534D0u, {0x00ADBACCu, gid});  // String_Format("can't find group %u")
                } else if (load<u8>(cmd + 32) == 0) {
                    call(ctx, 0x0033C2C0u, {g, 2});
                } else {
                    store<u32>(g + 40, gid);
                    store<u32>(g + 44, load<u32>(cmd + 8));
                    store<u32>(g + 48, load<u32>(cmd + 12));
                    store<u32>(g + 52, load<u32>(cmd + 16));
                    store<u32>(g + 56, load<u32>(cmd + 20));
                    store<u32>(g + 60, load<u32>(cmd + 24));
                    ctx.fpr[1] = Lf(cmd + 24);  // f1 as the original leaves it
                    call(ctx, 0x0033C2C0u, {g, 1});
                }
            }
            idx = load<u32>(o + 2612) + 1;
            store<u32>(o + 2612, idx);
        }
        for (u32 j = 0; j < load<u16>(o + 2580); ++j)
            callf(ctx, 0x0033C7A0u, {load<u32>(load<u32>(o + 2572) + 4 * j)}, {dt});
    }
    call(ctx, 0x003252D8u, {o, a, msg});  // Loader_Step_003252D8
}
