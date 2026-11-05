// World_HandleEvent callees, part 21j: the effect tree time step and the
// mask counter display.
#include "hevent19.h"

using namespace sly4;
using namespace sly4::hevent;

namespace {
constexpr u32 kInstanceSetTime = 0x004BC7C8;  // (Instance*, Effect*; f32) (name?)
constexpr u32 kEffectSetTime = 0x004CF710;    // (Effect*; f32) (symbols.txt Effect_SetTime)
constexpr u32 kEffectCheck = 0x004CF620;      // (Effect*, 0) -> finished (symbols.txt Effect_Check4CF620)
constexpr u32 kEffectDestruct = 0x004C8E80;   // (Effect*) (symbols.txt Effect_Destruct)
constexpr u32 kEffectRelease = 0x004CF3C4;    // (Effect*, u32 hash) (hevent21_g.cpp)
constexpr u32 kSmallFree = 0x00552420;        // (p, size) (as hevent7.h)
constexpr u32 kEffectBytes = 140;

constexpr u32 kHandleResolveBit1 = 0x003568E0;  // (u32 handle) -> Object* (symbols.txt Handle_ResolveBit1)
constexpr u32 kCounterHandle = 0x00F64CD4;    // u32 handle of the counter display object (name?)
constexpr u32 kMaskTotal = 0x00F64C24;        // u32 masks in the level (name?)
constexpr u32 kEffectSetHash = 0x004CF988;    // (Effect*, u32 hash) (hevent21_g.cpp)
constexpr u32 kHashComplete = 0x90A128CF, kHashCollect = 0x3143F570;  // effect hashes (name?)
constexpr u32 kStringFormat = 0x005534D0;     // (fmt, ...) (symbols.txt String_Format)
constexpr u32 kObjSetAnim = 0x00363A3C;       // (o, name, now, -, b, c, d; f32) (symbols.txt Obj_SetAnim)
constexpr u32 kPlayerSet = 0x003632C8;        // (o, value, which) (symbols.txt Player_Set3632C8)
constexpr u32 kFmtD = 0x00AD8B78;             // "%d"
constexpr u32 kFmtDD = 0x00AD8B80;            // "%d/%d"
constexpr u32 kMinusOne = 0x002DE250;         // f32 -1.0
constexpr u32 kAnimArg = 0x01164CA4;          // passed as Obj_SetAnim's last argument (name?)
}  // namespace

// u32 Effect_SetTime(Effect* e; f32 t): for a live effect (+136 bit
// 0x100): the instance (+4) is stepped (0x004BC7C8(inst, e; t)); one that
// reached its end (+32 >= +36) with no running sub (+16 -> +4 > 0) and
// not looping (u8 +48) marks the effect done (+136 bit 0x80). Children
// (+112, u16 +120) are stepped in turn; finished ones (Effect_Check4CF620)
// are destructed, freed and replaced by the last child. With children the
// done bit is cleared; a pending switch (+132) is applied with
// Effect_Release and cleared together with the done bit.
DECOMP(004CF710, Effect_SetTime) {
    const u32 e = arg<u32>(ctx, 0);
    const f64 t = farg(ctx, 0);
    const u32 live = (load<u32>(e + 136) >> 8) & 1u;
    ctx.gpr[3] = live;
    if (live == 0) return;
    if (load<u32>(e + 4) != 0) {
        callf(ctx, kInstanceSetTime, {load<u32>(e + 4), e}, {t});
        const u32 inst = load<u32>(e + 4);
        const u32 fl = load<u32>(e + 136);
        const f64 pos = hevent7::lfs_f64(inst + 32), end = hevent7::lfs_f64(inst + 36);
        ctx.fpr[1] = pos;
        ctx.fpr[2] = end;
        if (pos >= end) {
            const u32 sub = load<u32>(inst + 16);
            const bool running = sub != 0 && static_cast<s32>(load<u32>(sub + 4)) > 0;
            if (!running && load<u8>(inst + 48) == 0) store<u32>(e + 136, fl | 0x80u);
        }
    }
    u32 n;
    for (u32 i = 0; i < (n = load<u16>(e + 120)); ++i) {
        const u32 c = load<u32>(load<u32>(e + 112) + 4 * i);
        if (c == 0) continue;
        callf(ctx, kEffectSetTime, {c}, {t});
        if (static_cast<u32>(call(ctx, kEffectCheck, {c, 0})) == 0) continue;
        call(ctx, kEffectDestruct, {c});
        call(ctx, kSmallFree, {c, kEffectBytes});
        const u32 last = load<u16>(e + 120) - 1;
        store<u16>(e + 120, static_cast<u16>(last));
        if (static_cast<s32>(i) != static_cast<s32>(last & 0xFFFF)) {
            const u32 data = load<u32>(e + 112);
            store<u32>(data + 4 * i, load<u32>(data + ((last << 2) & 0x3FFFCu)));
        }
        --i;  // look at this slot again
    }
    if (n != 0) store<u32>(e + 136, load<u32>(e + 136) & ~0x80u);
    const u32 pend = load<u32>(e + 132);
    ctx.gpr[3] = pend;
    if (pend == 0) return;
    call(ctx, kEffectRelease, {e, pend});
    const u32 fl = load<u32>(e + 136);
    store<u32>(e + 132, 0);
    store<u32>(e + 136, fl & ~0x80u);
    ret(ctx, fl & ~0x80u);
}

// u32 Mask_ShowCount2DE258(Session* s, u8 complete) (name?): the counter
// display object (handle 0x00F64CD4): once the level's masks (0x00F64C24,
// non-zero) are all found (s +56 >= total) its effect (+20) is switched to
// 0x90A128CF (complete) or 0x3143F570. Unless the current part (+36) is
// flagged (u8 +821): with more than one slot (def +8 -> +152) the count
// ("%d" of +56) goes to Obj_SetAnim(o, text, 1, -, 0, 0, 0x01164CA4;
// -1.0) and the total ("%d") to Player_Set3632C8(o, text, 1); else
// "%d/%d" goes to Obj_SetAnim.
DECOMP(002DE258, Mask_ShowCount2DE258) {
    const u32 s = arg<u32>(ctx, 0);
    const u32 complete = arg<u32>(ctx, 1) & 0xFF;
    const u32 o = static_cast<u32>(call(ctx, kHandleResolveBit1, {load<u32>(kCounterHandle)}));
    if (o == 0) return;
    const u32 e = load<u32>(o + 20);
    if (e != 0) {
        const u32 total = load<u32>(kMaskTotal);
        if (total != 0 && !(load<u32>(s + 56) < total))
            call(ctx, kEffectSetHash, {e, complete != 0 ? kHashComplete : kHashCollect});
    }
    const u32 part = load<u32>(s + 36);
    if (part != 0) {
        const u32 flag = load<u8>(part + 821);
        ctx.gpr[3] = flag;
        if (flag == 0) return;
    }
    const u32 def = load<u32>(o + 8);
    const u32 slots = def != 0 ? load<u32>(def + 152) : 0;
    const f64 m1 = hevent7::lfs_f64(kMinusOne);
    if (slots > 1) {
        const u32 txt = static_cast<u32>(call(ctx, kStringFormat, {kFmtD, load<u32>(s + 56)}));
        callf(ctx, kObjSetAnim, {o, txt, 1, ctx.gpr[6], 0, 0, kAnimArg}, {m1});  // r6 is left over, unused
        const u32 txt2 = static_cast<u32>(call(ctx, kStringFormat, {kFmtD, load<u32>(kMaskTotal)}));
        call(ctx, kPlayerSet, {o, txt2, 1});
    } else {
        const u32 total = load<u32>(kMaskTotal);
        const u32 txt = static_cast<u32>(call(ctx, kStringFormat, {kFmtDD, load<u32>(s + 56), total}));
        callf(ctx, kObjSetAnim, {o, txt, 1, ctx.gpr[6], 0, 0, kAnimArg}, {m1});
    }
}
