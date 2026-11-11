// Game code, large-function sweep part 10: per-player pickup / event
// handler with its HUD updates (00483050).
#include "big10.h"

using namespace sly4;
using namespace sly4::big10;

namespace {
constexpr u32 kAddScore = 0x0016B5F0u;      // (player obj, u32 total) (name?)
constexpr u32 kSetCount = 0x001753C0u;      // (player obj, u32 count) (name?)
constexpr u32 kHudEvent23 = 0x002E58C8u;    // (hud, event obj) (name?)
constexpr u32 kHudSetIcon = 0x002E5C68u;    // (hud, handle obj) (name?)
constexpr u32 kResolve = 0x003568E0u;       // Handle_ResolveBit1 (handle) -> obj or 0
constexpr u32 kDispatch = 0x0036325Cu;      // Obj_Dispatch362 (o, 0, 1, 0)
constexpr u32 kSetAnim = 0x00363A3Cu;       // Obj_SetAnim (o, text, 1, -, 0, 0, colour*; f1)
constexpr u32 kSetText = 0x00367530u;       // Obj_SetText367530 (o, text, 0)
constexpr u32 kCopy216 = 0x00369658u;       // Global_F64CBC_Copy216 (u8 player)
constexpr u32 kSetRatio = 0x0036E498u;      // Obj_SetRatio_0036E498 (o, s32 a, s32 b)
constexpr u32 kWorldQuery = 0x00482D14u;    // World_Query482D14 (world, player, const EventRec*)
constexpr u32 kFormat = 0x005534D0u;        // String_Format (fmt, ...) -> const char*

// Event hashes (the def's +428).
constexpr u32 kEvHealth = 0x7EC7F65Cu;      // value: vfunc +316 (f1) on the player object
constexpr u32 kEvCoins = 0x1AD44BCDu;       // value added to the profile's coins (capped)
constexpr u32 kEvTimer = 0x00274ACDu;       // value (f32) added to the hud's +80 +2572
constexpr u32 kEvHud23 = 0x0023FEECu;
constexpr u32 kEvScore = 0x001F32A2u;
constexpr u32 kEvBit = 0xE33D410Au;         // value names one of five bits (table at 0x00AE812C)
constexpr u32 kEvCount = 0x8D057FB5u;       // value times: count + 1

constexpr u32 kProfiles = 0x00F65CA0u;      // -> profiles (4056 bytes each; +12 -> f32)
constexpr u32 kCoinCap = 0x010BC218u;       // +72 s32
constexpr u32 kHud = 0x00F64C20u;           // hud object
constexpr u32 kHandleCoins = 0x00F64CDCu;
constexpr u32 kHandleIcon = 0x00F64CD8u;
constexpr u32 kHandleScore = 0x00F64CE0u;
constexpr u32 kRatioObj = 0x00F64CC0u;
constexpr u32 kBitTable = 0x00AE812Cu;      // u32 keys, 16 bytes apart (5)
constexpr u32 kFmtNum = 0x00AE8A8Cu;        // number format
constexpr u32 kFmtTimer = 0x00AE9160u;
constexpr u32 kHudFlags = 0x0106C330u;      // +132 u8: text without the anim
constexpr u32 kAnimRate = 0x0048304Cu;      // .text f32
constexpr u32 kColour = 0x01164CA4u;
constexpr u32 kTimerHandle = 0x660B7222u;
constexpr u32 kPlayerStride = 0x00020C80u;

constexpr u32 kFlag400 = 0x400u;            // player object +192: active

u32 ActivePlayer(u32 pl) { return (pl != 0 && (load<u32>(pl + 192) & kFlag400) != 0) ? pl : 0; }

u32 Profile(u32 prec) { return load<u32>(kProfiles) + static_cast<u32>(load<u8>(prec)) * 4056u + 80; }

void HudAnimText(PPUContext& ctx, u32 h, u32 s) {
    callf(ctx, kSetAnim, {h, s, 1, kAnimRate, 0, 0, kColour}, {static_cast<f64>(F(kAnimRate))});
}
}  // namespace

// void World_PlayerEvent(World* w, s32 player, EventObj* e, u8 quiet)
// Applies a pickup / trigger event (hash at e->def +428, value at +432) to
// the player, then (unless quiet) updates the matching HUD element, and
// finally posts the event record {2, f32, def +52, e +168, e +336 +168, e,
// hash} to World_Query.
DECOMP(00483050, World_PlayerEvent483050) {
    const u32 w = arg<u32>(ctx, 0), p = arg<u32>(ctx, 1), e = arg<u32>(ctx, 2), quiet = arg<u32>(ctx, 3);
    if (p == 0xFFFFFFFFu) return;
    const u32 def = load<u32>(e + 216);
    const u32 hash = load<u32>(def + 428);
    const u32 prec = w + p * kPlayerStride + 4208;   // player record (+0 u8 profile, +8 object)
    const u32 value = load<u32>(def + 432);
    const f32 fval = F(def + 432);

    switch (hash) {
    case kEvHealth: {
        const u32 pl = load<u32>(prec + 8);
        const u32 t = (pl != 0 && (load<u32>(pl + 192) & 2u) != 0) ? pl : 0;
        // Quirk: no null test before the virtual call.
        const u32 opd = load<u32>(load<u32>(t) + 316);
        ctx.fpr[1] = fval;
        call_ptr(ctx, opd, {t, opd, load<u32>(opd)});
        break;
    }
    case kEvCoins: {
        const u32 pr = Profile(prec);
        const s32 v = static_cast<s32>(load<u32>(pr + 32) + value);
        store<u32>(pr + 32, static_cast<u32>(v));
        const s32 cap = static_cast<s32>(load<u32>(kCoinCap + 72));
        if (v > cap) store<u32>(pr + 32, static_cast<u32>(cap));
        break;
    }
    case kEvTimer: {
        const u32 h = load<u32>(load<u32>(kHud) + 80);
        if (h != 0) store<f32>(h + 2572, F(h + 2572) + fval);
        break;
    }
    case kEvHud23:
        call(ctx, kHudEvent23, {load<u32>(kHud), e});
        break;
    case kEvScore: {
        const u32 pl = ActivePlayer(load<u32>(prec + 8));
        if (pl != 0) call(ctx, kAddScore, {pl, load<u32>(pl + 724) + value});
        break;
    }
    case kEvBit: {
        if (value == 0) break;
        u32 k = 0;
        while (k < 5 && load<u32>(kBitTable + 16 * k) != value) ++k;
        if (k == 5) break;
        const u32 a = load<u32>(kProfiles) + static_cast<u32>(load<u8>(prec)) * 4056u + (std::rotr(k, 2) & 0x3FFFFFFCu) + 4016;
        store<u32>(a, load<u32>(a) | (1u << ((k << 1) & 0x1Eu)));
        break;
    }
    case kEvCount:
        if (value == 0) break;
        for (u32 n = 0; n < value; ++n) {
            const u32 pl = ActivePlayer(load<u32>(prec + 8));
            if (pl != 0) call(ctx, kSetCount, {pl, load<u32>(pl + 660) + 1});
        }
        break;
    default:
        break;
    }

    if ((quiet & 0xFFu) == 0) {
        switch (hash) {
        case kEvHealth:
            call(ctx, kCopy216, {p & 0xFFu});
            break;
        case kEvCoins: {
            const u32 h = static_cast<u32>(call(ctx, kResolve, {load<u32>(kHandleCoins)}));
            if (h == 0) break;
            const s32 coins = static_cast<s32>(load<u32>(Profile(prec) + 32));
            const u32 s = static_cast<u32>(call(ctx, kFormat, {kFmtNum, static_cast<u64>(static_cast<s64>(coins))}));
            if (load<u8>(kHudFlags + 132) != 0 || load<u16>(w + 0x00128214u) != 0) call(ctx, kSetText, {h, s, 0});
            else HudAnimText(ctx, h, s);
            break;
        }
        case kEvTimer: {
            const u32 h = static_cast<u32>(call(ctx, kResolve, {kTimerHandle}));
            call(ctx, kDispatch, {h, 0, 1, 0});
            const s32 secs = static_cast<s32>(static_cast<u32>(rt::fctiw(static_cast<f64>(fval), true)));
            const u32 s = static_cast<u32>(call(ctx, kFormat, {kFmtTimer, static_cast<u64>(static_cast<s64>(secs))}));
            HudAnimText(ctx, h, s);
            break;
        }
        case kEvHud23: {
            const u32 hud = load<u32>(kHud);
            const u32 h = static_cast<u32>(call(ctx, kResolve, {load<u32>(kHandleIcon)}));
            call(ctx, kHudSetIcon, {hud, h});
            break;
        }
        case kEvScore: {
            const u32 h = static_cast<u32>(call(ctx, kResolve, {load<u32>(kHandleScore)}));
            if (h == 0) break;
            const u32 s = static_cast<u32>(call(ctx, kFormat, {kFmtNum, 1}));
            HudAnimText(ctx, h, s);
            break;
        }
        case kEvCount: {
            const u32 pl = ActivePlayer(load<u32>(prec + 8));
            const u32 a = pl != 0 ? load<u32>(pl + 660) : 0u;
            const u32 b = pl != 0 ? load<u32>(pl + 664) : 0u;
            const u32 ro = load<u32>(kRatioObj);
            if (ro != 0)
                call(ctx, kSetRatio, {ro, static_cast<u64>(static_cast<s64>(static_cast<s32>(a))), static_cast<u64>(static_cast<s64>(static_cast<s32>(b)))});
            break;
        }
        default:
            break;
        }
    }

    Stack frame(ctx, 32);
    const u32 rec = frame.addr();
    store<u32>(rec + 0, 2);
    const u32 f4 = load<u32>(load<u32>(load<u32>(kProfiles) + 12));
    SetF1Bits(ctx, f4);   // lfs f1: left in f1 by the original
    store<u32>(rec + 4, f4);
    store<u32>(rec + 24, hash);
    u32 dval = 0;
    if (e != 0 && load<u32>(e + 216) != 0) dval = load<u32>(load<u32>(e + 216) + 52);
    store<u32>(rec + 8, dval);
    store<u32>(rec + 12, e != 0 ? load<u32>(e + 168) : 0u);
    u32 lk = 0;
    if (e != 0) {
        const u32 l = load<u32>(e + 336);
        if (l != 0) lk = load<u32>(l + 168);
    }
    store<u32>(rec + 16, lk);
    store<u32>(rec + 20, e);
    call(ctx, kWorldQuery, {w, p, rec});
}
