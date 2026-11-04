// World_HandleEvent callees, part 4: 130..138 instructions.
#include "hevent.h"

using namespace sly4;
using namespace sly4::hevent;

namespace sly4 {
namespace hevent4f {
namespace ffn {
constexpr u32 Game_Check45C6AC = 0x0045C6AC;     // (Game*, 1) -> 0 to reset (name?)
constexpr u32 Game_Reset458B1C = 0x00458B1C;     // (Game*) (name?)
constexpr u32 Thread_Release5DBC18 = 0x005DBC18; // (u64 thread id) (name?)
constexpr u32 Players_Update3A09C0 = 0x003A09C0; // (Manager*, user block +8) (name?)
constexpr u32 Part_Reset31EA3C = 0x0031EA3C;     // (part, u8) (name?)
constexpr u32 Part_Reset31E040 = 0x0031E040;     // (part) (name?)
}  // namespace ffn

// Job lists the SPU job fills (see hevent3_k.cpp Objects_FinishJobA).
constexpr u32 kJobObjects = 0x01013B34;          // {u32 data, ..., u16 count at +8}
constexpr u32 kJobDPending = 0x01021E00;         // u32 (name?)
constexpr u32 kJobShowCount = 0x01015BB0, kJobShowIndices = 0x01015BC0;  // u32, u16[] (name?)
constexpr u32 kJobHideCount = 0x01017BC0, kJobHideIndices = 0x01017BD0;  // u32, u16[] (name?)
constexpr u32 kObjFlags = 172;                   // u32: 4 = shown (name?)
constexpr u32 kObjVfnShow = 240, kObjVfnHide = 244;  // (name?)

// The loader thread and the loading UI (hevent2_a.cpp LoadingUi_Hide).
constexpr u32 kLoaderThread = 0x00F9CDE8;        // u64 thread id, 0 when none (wupdate.h kPending9CDE8)
constexpr u32 kLoaderThreadFlag = 0x00F9CDF4;    // u8 cleared before the join (name?)
constexpr u32 kLoadingUi = 0x00F558E8, kLoadingText1 = 0x00F558EC, kLoadingText2 = 0x00F558F0;
constexpr u32 kUiVfnShow = 88, kUiVfnSetText = 188;
constexpr u32 kEmptyString = 0x00AC5E64;         // ""
constexpr u32 kMainThread = 0x011DE290, kMainThreadGuard = 0x011DE2A0;  // u64[2] (debug.cpp), u8
constexpr u32 kSysPpuThreadJoin = 44;

constexpr u32 kTunePartsAll = 0x0106D084;        // u8 (0x0106D03C + 72) (name?)
}  // namespace hevent4f
}  // namespace sly4

using namespace sly4::hevent4f;

// u32 Objects_FinishJobD(): waits (300 us sleeps) for 0x01021E00, then every
// object the job listed to show that isn't shown yet gets flag 4 at +172 and
// vfunc +240, every listed shown one to hide loses it and gets vfunc +244; both
// counts are cleared (only when nonzero). Returns the hide count as last read,
// as the original leaves r3 (name?)
DECOMP(000CFF4C, Objects_FinishJobD) {
    while (load<u32>(kJobDPending) != 0) syscall(ctx, kSysTimerUsleep, {300});
    if (load<u32>(kJobShowCount) != 0) {
        for (u32 i = 0; i < load<u32>(kJobShowCount); ++i) {
            const u32 o = load<u32>(load<u32>(kJobObjects) + load<u16>(kJobShowIndices + i * 2) * 4);
            if (o == 0) continue;
            const u32 f = load<u32>(o + kObjFlags);
            if ((f & 4u) != 0) continue;
            store<u32>(o + kObjFlags, f | 4u);
            // (The original then tests the bit again and would call vfunc +244 if clear.)
            vcall(ctx, o, kObjVfnShow, {o});
        }
        store<u32>(kJobShowCount, 0);
    }
    u32 n = load<u32>(kJobHideCount);
    if (n != 0) {
        for (u32 i = 0; i < n; ++i) {
            const u32 o = load<u32>(load<u32>(kJobObjects) + load<u16>(kJobHideIndices + i * 2) * 4);
            if (o == 0) continue;
            const u32 f = load<u32>(o + kObjFlags);
            if ((f & 4u) == 0) continue;
            store<u32>(o + kObjFlags, f & ~4u);
            vcall(ctx, o, kObjVfnHide, {o});
            n = load<u32>(kJobHideCount);
        }
        store<u32>(kJobHideCount, 0);
    }
    ret(ctx, n);
}

// u32 Loading_Finish(): only with a loader thread (u64 0x00F9CDE8). Hides the
// loading UI (as LoadingUi_Hide 0x000AC39C, inline), joins the thread (byte
// 0x00F9CDF4 = 0 first), resets the Game when Game_Check45C6AC(game, 1) is 0,
// releases the main thread entry (0x011DE298, built zeroed on first use), clears
// the thread id and the SystemState's +20 / +24. Returns &SystemState +20 (name?)
DECOMP(000AC478, Loading_Finish) {
    if (load<u64>(kLoaderThread) == 0) return ret(ctx, 0u);
    const u32 ui = load<u32>(kLoadingUi);
    if (ui != 0 && load<u32>(kLoadingText1) != 0) {
        vcall(ctx, ui, kUiVfnShow, {ui, 0});
        const u32 t1 = load<u32>(kLoadingText1);
        vcall(ctx, t1, kUiVfnSetText, {t1, kEmptyString, 0});
        const u32 t2 = load<u32>(kLoadingText2);
        if (t2 != 0) vcall(ctx, t2, kUiVfnSetText, {t2, kEmptyString, 0});
    }
    if (load<u64>(kLoaderThread) != 0) {
        store<u8>(kLoaderThreadFlag, 0);
        Stack frame(ctx, 16);  // exit status at r1+112
        syscall(ctx, kSysPpuThreadJoin, {load<u64>(kLoaderThread), frame.addr()});
    }
    if (static_cast<u32>(call(ctx, ffn::Game_Check45C6AC, {g::game, 1})) == 0) call(ctx, ffn::Game_Reset458B1C, {g::game});
    if (load<u8>(kMainThreadGuard) == 0) {
        store<u64>(kMainThread, 0);
        store<u64>(kMainThread + 8, 0);
        store<u8>(kMainThreadGuard, 1);
    }
    call(ctx, ffn::Thread_Release5DBC18, {load<u64>(kMainThread + 8)});
    store<u64>(kLoaderThread, 0);
    const u32 ss = system_state::get(ctx);
    store<u32>(ss + 20, 0);
    store<u32>(ss + 24, 0);
    ret(ctx, ss + 20);
}

// u32 Players_Update3A11A8(Manager* m) (name?): nothing when both m->+20 and
// m->+4 have a zero u16 +2. Players_Update3A09C0(m, user block +8) for each of
// the current world's +29 users; then for each of the m +100 pending callbacks
// {arg, OPD} at m +92 and each of the m->+4 +2 parts (112 bytes at m +12): with
// tuning 0x0106D084, or when the part's +72 has 0x40000000 and 0x80000000 set,
// every bone (8-byte entries at +84, count +78; s16 index at +4 into m +28's
// 336-byte records) gets opd(arg, rec +48, rec +144) unless the tuning is off
// and the record's +328 lacks 0x08000000. Clears m +100; returns its old value.
DECOMP(003A11A8, Players_Update3A11A8) {
    const u32 m = arg<u32>(ctx, 0);
    if (load<u16>(load<u32>(m + 20) + 2) == 0) {
        const u32 c = load<u16>(load<u32>(m + 4) + 2);
        if (c == 0) return ret(ctx, c);
    }
    for (u32 u = 0;; ++u) {
        const u32 cw = load<u32>(kCurrentWorld);
        if (u >= load<u8>(cw + 29)) break;
        call(ctx, ffn::Players_Update3A09C0, {m, load<u32>(cw + u * kUserBlockSize + kUserBlocks + 8)});
    }
    u32 n = 0;
    for (u32 j = 0; j < (n = load<u16>(m + 100)); ++j) {
        if (load<u16>(load<u32>(m + 4) + 2) == 0) continue;
        for (u32 k = 0; k < load<u16>(load<u32>(m + 4) + 2); ++k) {
            const u32 part = load<u32>(m + 12) + k * 112;
            u32 tune = load<u8>(kTunePartsAll);
            if (tune == 0) {
                const u32 f = load<u32>(part + 72);
                if ((f & 0x40000000u) == 0 || (f & 0x80000000u) == 0) continue;
            }
            for (u32 q = 0; q < load<u16>(part + 78); ++q) {
                const s32 idx = static_cast<s16>(load<u16>(load<u32>(part + 84) + q * 8 + 4));
                const u32 rec = load<u32>(m + 28) + static_cast<u32>(idx) * 336;
                if (tune == 0 && (load<u32>(rec + 328) & 0x08000000u) == 0) continue;
                const u32 cb = load<u32>(m + 92) + j * 8;
                const u32 opd = load<u32>(cb + 4);
                // r6 / r7: the code address and the OPD, as the original leaves them.
                ctx.gpr[6] = load<u32>(opd);
                ctx.gpr[7] = opd;
                call_ptr(ctx, opd, {load<u32>(cb), rec + 48, rec + 144});
                tune = load<u8>(kTunePartsAll);
            }
        }
    }
    store<u16>(m + 100, 0);
    ret(ctx, n);
}

// u32 Model_Reset31F968(Model* o, u8 a, s32 key) (name?): resets every part of
// the +32 groups (8-byte {-, PtrVector*} entries, count +26) with
// Part_Reset31EA3C(part, a) and every +8 entry's part (count +2) with
// Part_Reset31E040; +64 = 1.0, +68 |= 0x20000000. key (or, when 0, o->+48 +12)
// is looked up in o->+48's +8 vector of {first, key} pairs; +52 = its index (0
// when absent), +56 = 0, +60 = the f32 at +4 of **first; +68 bit 0x80000000 =
// (that value == 0.0), 0x40000000 set. Quirk: without the vector the lookup still
// reads through address 0. Returns the new +68.
DECOMP(0031F968, Model_Reset31F968) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 a = arg<u32>(ctx, 1) & 0xFF;
    const u32 key0 = arg<u32>(ctx, 2);
    for (s32 i = 0; i < static_cast<s32>(load<u16>(o + 26)); ++i) {
        const u32 vec = load<u32>(load<u32>(o + 32) + static_cast<u32>(i) * 8 + 4);
        if (vec == 0) continue;
        for (u32 j = 0; j < load<u16>(vec + 8); ++j) {
            const u32 x = load<u32>(load<u32>(vec) + j * 4);
            if (x != 0) call(ctx, ffn::Part_Reset31EA3C, {x, a});
        }
    }
    for (s32 i = 0; i < static_cast<s32>(load<u16>(o + 2)); ++i) {
        const u32 x = load<u32>(load<u32>(o + 8) + static_cast<u32>(i) * 8 + 4);
        if (x != 0) call(ctx, ffn::Part_Reset31E040, {x});
    }
    store<u32>(o + 64, 0x3F800000u);  // 1.0
    const u32 f = load<u32>(o + 68) | 0x20000000u;
    store<u32>(o + 68, f);
    s32 key = static_cast<s32>(key0);
    if (key == 0) key = load<s32>(load<u32>(o + 48) + 12);
    u32 idx = 0;
    if (key != 0) {
        for (u32 i = 0;; ++i) {
            const u32 v = load<u32>(load<u32>(o + 48) + 8);
            const u32 n = v != 0 ? load<u16>(v + 8) : 0u;
            if (i >= n) {
                idx = 0;
                break;
            }
            if (key == load<s32>(load<u32>(v) + i * 8 + 4)) {
                idx = i;
                break;
            }
        }
    }
    store<u32>(o + 52, idx);
    store<u32>(o + 56, 0);
    const u32 first = load<u32>(load<u32>(load<u32>(load<u32>(o + 48) + 8)) + idx * 8);
    const u32 bits = load<u32>(load<u32>(first) + 4);
    store<u32>(o + 60, bits);
    const bool zero = std::bit_cast<f32>(bits) == load<f32>(0x0031F964);  // f32 0.0
    const u32 nf = (f & 0x7FFFFFFFu) | (zero ? 0x80000000u : 0u) | 0x40000000u;
    store<u32>(o + 68, nf);
    ret(ctx, nf);
}
