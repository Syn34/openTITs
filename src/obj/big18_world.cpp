// Game code, large-function sweep part 18: the world's mode switch
// (cutscene / shop / speaker modes: screen fades, HUD, panels).
#include "big18.h"

using namespace sly4;
using namespace sly4::big18;

namespace {
namespace hw {
constexpr u32 kPanels = 0x00F65E70;      // u32[] per mode: the panel for Obj_ShowPanel
constexpr u32 kShop = 0x00F64CC8;        // the shop object (or 0)
constexpr u32 kSpeakerH = 0x00F64D48;    // handle for Handle_ResolveBit10000
constexpr u32 kShopH = 0x00F64D4C;       // handle for Handle_ResolveBit20000
constexpr u32 kLetterboxOn = 0x00F65D8C; // u8
constexpr u32 kLetterbox = 0x00AE8AF8;   // "InGame.Cutscene.Letterbox"
constexpr u32 kFlagMap = 0x010BB738;     // the flag map (SortedMap_Find786C0C)
constexpr u32 kQueueN = 0x010BA134;      // s32 count of the message queue
constexpr u32 kQueue = 0x010BA138;       // 20-byte records, at most 128
constexpr u32 kGame = 0x010B19E0;        // the two 11968-byte user records
constexpr u32 kColour = 0x01164CA0;      // u8[4] colour; +4 the colour Obj_Play gets
constexpr u32 kMinus1 = 0x00470748;      // .text f32 -1
// Fade tunables (f32 / u32 words).
constexpr u32 kFadeColour = 0x010B9B5C;  // u8[4]
constexpr u32 kFadeA = 0x010B9BA8;
constexpr u32 kFadeWord = 0x010B9BF4;
constexpr u32 kFadeShort = 0x010B9C40;   // duration
constexpr u32 kFadeLong = 0x010B9C8C;    // duration
// World fields.
constexpr u32 kHud = 0x001281F4;         // object with vfunc 40(o, 1)
constexpr u32 kFadeEnt = 0x00128334;     // 28-byte fade entries
constexpr u32 kFadeN = 0x001283A4;       // u32 entry count
constexpr u32 kSoundA = 0x001285F0, kSoundB = 0x001285F4;
constexpr u32 kKey = 0x846BF162;         // flag-map key whose listeners get queued
constexpr u32 kMsg = 0x2EB3CC8B;         // the queued message

inline void Copy4(u32 dst, u32 src) {
    for (u32 k = 0; k < 4; ++k) store<u8>(dst + k, load<u8>(src + k));
}

// The fade entry at index n: flags word (top nibble 2, bits 4..8 cleared,
// bit 8 = rev), the two colours at +2 / +6, f32 +12 / +16 (0, 1 or 1, 0),
// +20 the duration, +24 = 0.
inline void FadeEntry(u32 w, u32 n, bool rev, u32 dur) {
    const u32 e = w + n * 28u;
    store<u32>(e + kFadeEnt, (load<u32>(e + kFadeEnt) & 0x007FFFFFu) | 0x20000000u | (rev ? 0x00800000u : 0u));
    Copy4(e + kFadeEnt + 2, rev ? kFadeColour : kColour);
    Copy4(e + kFadeEnt + 6, rev ? kColour : kFadeColour);
    store<u32>(e + kFadeEnt + 12, rev ? 0x3F800000u : 0u);
    store<u32>(e + kFadeEnt + 16, rev ? 0u : 0x3F800000u);
    if (!rev) store<u8>(w + 52, static_cast<u8>(n));
    store<u32>(e + kFadeEnt + 20, dur);
    store<u32>(e + kFadeEnt + 24, 0);
}

// Fade in (a new entry at the count, which is bumped; u8 +52 = its index).
// f1 is left as the lfs of kFadeA, as the original leaves it.
inline void FadeIn(PPUContext& ctx, u32 w, u32 durSrc) {
    const u32 a = load<u32>(kFadeA), dur = load<u32>(durSrc);
    const u32 bc = load<u32>(w + 0x001283BC);
    store<u32>(w + 0x001283BC, (bc & 0x7FFFFFFFu) | 0x40000000u);
    store<u32>(w + 0x001283B8, 0);
    store<u32>(w + 0x001283B4, load<u32>(kFadeWord));
    store<u32>(w + 0x001283B0, dur);
    store<u32>(w + 0x001283AC, a);
    store<u32>(w + 0x001283A8, 0);
    const u32 n = load<u32>(w + kFadeN);
    store<u32>(w + kFadeN, n + 1);
    FadeEntry(w, n, false, dur);
    ctx.fpr[1] = std::bit_cast<f64>(LfsBits(a));
}

// Fade out: the entry at u8 +52 (the count++ path the original has for a
// negative index is unreachable: cmpwi on a zero-extended byte). f1 is left
// as the duration.
inline void FadeOut(PPUContext& ctx, u32 w, u32 durSrc) {
    store<u32>(w + 0x001283A8, load<u32>(kFadeA));
    store<u32>(w + 0x001283AC, 0);
    store<u32>(w + 0x001283B0, load<u32>(durSrc));
    store<u32>(w + 0x001283B4, load<u32>(kFadeWord));
    store<u32>(w + 0x001283B8, 0);
    store<u32>(w + 0x001283BC, load<u32>(w + 0x001283BC) | 0xC0000000u);
    const u32 n = load<u8>(w + 52);
    const u32 dur = load<u32>(durSrc);
    FadeEntry(w, n, true, dur);
    SetF1(ctx, durSrc);
}

// Pending_Build, then each listener of the flag-map key kKey is queued
// (Queue_Push_004705F8 inlined: skipped when its object is 0 and its u8 is
// set; the count is kept in a register across the list).
inline void QueueListeners(PPUContext& ctx, u32 keyAt) {
    using namespace b18fn;
    call(ctx, Pending_Build366E20, {});
    store<u32>(keyAt, kKey);
    const u32 slot = static_cast<u32>(call(ctx, SortedMap_Find786C0C, {kFlagMap, keyAt}));
    const u32 p = load<u32>(slot);
    if (p == 0) return;
    u32 node = load<u32>(p);
    if (node == 0) return;
    u32 n = load<u32>(kQueueN);
    u32 off = n * 20u;
    for (; node != 0; node = load<u32>(node + 4)) {
        const u32 o = load<u32>(node);
        const u8 b = load<u8>(node + 8);
        if (o == 0 && b != 0) continue;
        if (static_cast<s32>(n) >= 128) continue;
        const u32 q = kQueue + off;
        ++n;
        store<u32>(q, 0);
        store<u32>(kQueueN, n);
        store<u32>(q + 4, o);
        store<u32>(q + 8, kMsg);
        store<u32>(q + 12, 0);
        store<u8>(q + 16, b);
        off += 20;
    }
}

inline void HudOn(PPUContext& ctx, u32 h) {
    if (h != 0) VCall(ctx, h, 40, {h, 1});
}

// vfunc 36(r, 1) of the object at +204 of each user record, when set.
inline void UsersNotify(PPUContext& ctx) {
    for (u32 k = 0; k < 2; ++k) {
        const u32 r = load<u32>(kGame + 11968u * k + 204);
        if (r != 0) VCall(ctx, r, 36, {r, 1});
    }
}

// The user record of world slot u8 +30: kGame + 176 + 11968 * u8 (w +
// 0x20C80 * slot + 4209).
inline u32 UserRec(u32 w, u32 slot) {
    return kGame + static_cast<u32>(load<u8>(w + slot * 0x00020C80u + 4209)) * 11968u + 176;
}

inline void Sound(PPUContext& ctx, u32 out, u32 snd) {
    call(ctx, b18fn::Sound_Play44C474, {out, snd, 0, 0, 0, 0, 0, 0, 0, 0, 0});
}
}  // namespace hw
}  // namespace

// void f(World* w, s32 mode, u8 arg): u8 +31 = mode (old value kept), +32
// = 0, +33 = arg; nothing more unless there is a player and
// Obj_ShowPanel(player, kPanels[mode]) (table index unchecked). Then:
// 0: +30 = +51. 1 / 2 / 3: fade in (short / short / long), queue the
// listeners, HUD on, World_SetFlags(2), pause, the users' vfunc 36; then
// 1: the shop gets bit 0x40000000 of +216 and Obj_StartMode; 2: the
// speaker +960 bit 0x40000000 and Obj_SpeakerTalk; 3: the shop object +396
// bit 0x40000000 and Obj_Play. 4: with a player whose +24 has bit 0x8000
// of +164 just Obj_SelectEntry(0x0023FDB9), else the letterbox cutscene
// start (fade, listeners, HUD, letterbox widget, target, sound, the user
// record's bit 0x8000 saved in bit 2 of +44 and set); unless the old mode
// was set, the pause and the users' vfunc 36. 5..20: Obj_SelectEntry with a
// per-mode hash (not 11, 16, 17). 23..26: fade out (23: only when
// Obj_RequestFlag; then sound, bit 2 of +44 back into the user record and
// bit 0 of +45 cleared; 24: shop mode off first; 25 / 26: notify the speaker
// / shop first). r3 on return is dead (regflow liveout: no caller reads it).
DECOMP(00470750, World_SetMode_00470750) {
    using namespace b18fn;
    using namespace hw;
    const u32 w = arg<u32>(ctx, 0);
    const s32 mode = arg<s32>(ctx, 1);
    const u8 a2 = static_cast<u8>(arg<u32>(ctx, 2));
    Stack frame(ctx, 208);
    const u32 sp = frame.addr();

    const u8 old = load<u8>(w + 31);
    store<u8>(w + 31, static_cast<u8>(mode));
    store<u8>(w + 32, 0);
    store<u8>(w + 33, a2);
    const u32 panel = load<u32>(kPanels + (static_cast<u32>(mode) << 2));
    if (call(ctx, Player_Get, {}) == 0) return;
    const u32 pl = static_cast<u32>(call(ctx, Player_Get, {}));
    if (static_cast<u32>(call(ctx, Obj_ShowPanel, {pl, panel})) == 0) return;

    auto select = [&](u32 hash) {
        if (call(ctx, Player_Get, {}) == 0) return;
        const u32 p = static_cast<u32>(call(ctx, Player_Get, {}));
        call(ctx, Obj_SelectEntry, {p, hash});
    };
    auto pauseIn = [&]() {
        HudOn(ctx, load<u32>(w + kHud));
        call(ctx, World_SetFlags, {2});
        call(ctx, Pause_Request44BB48, {0xFFBFFFFFu});
        call(ctx, Pause_Set2AB340, {1});
        UsersNotify(ctx);
    };

    switch (mode) {
    case 0:
        store<u8>(w + 30, load<u8>(w + 51));
        return;
    case 1: {
        FadeIn(ctx, w, kFadeShort);
        QueueListeners(ctx, sp + 164);
        pauseIn();
        const u32 shop = load<u32>(kShop);
        if (shop == 0) return;
        store<u32>(shop + 216, load<u32>(shop + 216) | 0x40000000u);
        call(ctx, Obj_StartMode368360, {load<u32>(kShop)});
        return;
    }
    case 2: {
        FadeIn(ctx, w, kFadeShort);
        QueueListeners(ctx, sp + 160);
        pauseIn();
        if (call(ctx, Handle_ResolveBit10000, {load<u32>(kSpeakerH)}) == 0) return;
        u32 s = static_cast<u32>(call(ctx, Handle_ResolveBit10000, {load<u32>(kSpeakerH)}));
        store<u32>(s + 960, load<u32>(s + 960) | 0x40000000u);
        s = static_cast<u32>(call(ctx, Handle_ResolveBit10000, {load<u32>(kSpeakerH)}));
        callf(ctx, Obj_SpeakerTalk386560, {s, 0, kMinus1, 0, 0, kColour + 4}, {LfsX(kMinus1)});
        return;
    }
    case 3: {
        FadeIn(ctx, w, kFadeLong);
        QueueListeners(ctx, sp + 156);
        pauseIn();
        const u32 s = static_cast<u32>(call(ctx, Handle_ResolveBit20000, {load<u32>(kShopH)}));
        if (s == 0) return;
        store<u32>(s + 396, load<u32>(s + 396) | 0x40000000u);
        callf(ctx, Obj_Play363890, {s, 0, s, 0, 0, kColour + 4}, {LfsX(kMinus1)});
        return;
    }
    case 4: {
        const u32 p = static_cast<u32>(call(ctx, Player_Get, {}));
        if (p != 0) {
            const u32 q = load<u32>(p + 24);
            if (q != 0 && (load<u32>(q + 164) & 0x8000u) != 0) {
                call(ctx, Obj_SelectEntry, {p, 0x0023FDB9u});
            } else {
                FadeIn(ctx, w, kFadeShort);
                QueueListeners(ctx, sp + 152);
                HudOn(ctx, load<u32>(w + kHud));
                const u32 ui = static_cast<u32>(call(ctx, Ui_Find, {kLetterbox, 0}));
                if (ui != 0) {
                    store<u8>(kLetterboxOn, static_cast<u8>(VCall(ctx, ui, 84, {ui})));
                    const u32 ui2 = static_cast<u32>(call(ctx, Ui_Find, {kLetterbox, 0}));
                    if (ui2 != 0) VCall(ctx, ui2, 88, {ui2, 0});
                }
                call(ctx, Obj_SetTarget168, {p, 1, 0});
                call(ctx, Obj_SelectEntry, {p, 0x0023FDB9u});
                call(ctx, World_SetFlags, {2});
                Sound(ctx, sp + 144, load<u32>(w + kSoundA));
                const u32 rec = UserRec(w, load<u8>(w + 30));
                const u32 b44 = load<u8>(w + 44);
                const u32 bit = (load<u32>(rec + 4) >> 15) & 1u;
                store<u8>(w + 44, static_cast<u8>((b44 & ~4u) | (bit << 2)));
                store<u32>(rec + 4, load<u32>(rec + 4) | 0x8000u);
            }
        }
        if (old != 0) return;
        call(ctx, Pause_Set2AB340, {1});
        call(ctx, Pause_Request44BB48, {0xFFBFFFFFu});
        UsersNotify(ctx);
        return;
    }
    case 5: select(0xE052127Eu); return;
    case 6: select(0x1687C913u); return;
    case 7: select(0xD4484357u); return;
    case 8: select(0xBA25C2B5u); return;
    case 9: select(0xEFE49890u); return;
    case 10: select(0x16884C62u); return;
    case 12: select(0x4D0DB575u); return;
    case 13: select(0x7ABDE263u); return;
    case 14: select(0xA6865016u); return;
    case 15: select(0x3BB7AF0Cu); return;
    case 18: select(0x03D3E615u); return;
    case 19: select(0x95970201u); return;
    case 20: select(0x83C0B888u); return;
    case 23: {
        if (call(ctx, Player_Get, {}) != 0) {
            const u32 p = static_cast<u32>(call(ctx, Player_Get, {}));
            if (call(ctx, Obj_RequestFlag3736C4, {p, 1}) != 0) FadeOut(ctx, w, kFadeShort);
            Sound(ctx, sp + 148, load<u32>(w + kSoundB));
            const u32 rec = UserRec(w, load<u8>(w + 30));
            const u32 bit = (static_cast<u32>(load<u8>(w + 44)) >> 2) & 1u;
            store<u32>(rec + 4, (load<u32>(rec + 4) & ~0x8000u) | (bit << 15));
        }
        store<u8>(w + 45, load<u8>(w + 45) & 0xFEu);
        return;
    }
    case 24: {
        const u32 shop = load<u32>(kShop);
        if (shop != 0) call(ctx, Obj_ShopModeOff3684B8, {shop});
        FadeOut(ctx, w, kFadeShort);
        return;
    }
    case 25:
        if (call(ctx, Handle_ResolveBit10000, {load<u32>(kSpeakerH)}) != 0) {
            const u32 s = static_cast<u32>(call(ctx, Handle_ResolveBit10000, {load<u32>(kSpeakerH)}));
            call(ctx, Obj_NotifyB386140, {s, 0, 0});
        }
        FadeOut(ctx, w, kFadeShort);
        return;
    case 26: {
        const u32 s = static_cast<u32>(call(ctx, Handle_ResolveBit20000, {load<u32>(kShopH)}));
        if (s != 0) call(ctx, Obj_NotifyA362F1C, {s, 0, 0});
        FadeOut(ctx, w, kFadeLong);
        return;
    }
    default:
        return;
    }
}
