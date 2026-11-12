// Game code, large-function sweep part 18: the world's mode switch
// (cutscene / shop / speaker modes: screen fades, HUD, panels).
#include "big18.h"

namespace sly4::obj {

namespace {
constexpr u32 Pause_Set2AB340 = 0x002AB340;        // (int)
constexpr u32 Handle_ResolveBit10000 = 0x0035B908; // (handle) -> speaker object or 0
constexpr u32 Obj_NotifyA362F1C = 0x00362F1C;      // (o, 0, 0)
constexpr u32 Obj_Play363890 = 0x00363890;         // (o, 0, f1, 0, 0, const u8 colour[4]) (r5 not read)
constexpr u32 Handle_ResolveBit20000 = 0x00365B28; // (handle) -> shop object or 0
constexpr u32 Player_Get = 0x00365C24;             // () -> player or 0
constexpr u32 Pending_Build366E20 = 0x00366E20;    // ()
constexpr u32 Obj_StartMode368360 = 0x00368360;    // (o)
constexpr u32 Obj_ShopModeOff3684B8 = 0x003684B8;  // (o)
constexpr u32 Obj_SetTarget168 = 0x0037202C;       // (o, 1, 0)
constexpr u32 Obj_SelectEntry = 0x003734B0;        // (player, u32 hash)
constexpr u32 Obj_RequestFlag3736C4 = 0x003736C4;  // (player, 1) -> bool
constexpr u32 Obj_ShowPanel = 0x0037370C;          // (player, panel) -> bool
constexpr u32 Obj_NotifyB386140 = 0x00386140;      // (o, 0, 0)
constexpr u32 Obj_SpeakerTalk386560 = 0x00386560;  // (o, 0, f1, 0, 0, const u8 colour[4]) (r5 not read)
constexpr u32 Pause_Request44BB48 = 0x0044BB48;    // (u32 mask)
constexpr u32 Sound_Play44C474 = 0x0044C474;       // (out, sound, 0 x 9) (11 arguments)
constexpr u32 World_SetFlags = 0x00599024;         // (u32)
constexpr u32 SortedMap_Find786C0C = 0x00786C0C;   // (map, const u32* key) -> slot

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
// World fields (offsets from the world).
constexpr u32 kHud = 0x001281F4;         // object with vfunc 40(o, 1)
constexpr u32 kFadeEnt = 0x00128334;     // 28-byte fade entries
constexpr u32 kFadeN = 0x001283A4;       // u32 entry count
constexpr u32 kFade = 0x001283A8;        // the fade state words (WorldFade)
constexpr u32 kSoundA = 0x001285F0, kSoundB = 0x001285F4;
constexpr u32 kKey = 0x846BF162;         // flag-map key whose listeners get queued
constexpr u32 kMsg = 0x2EB3CC8B;         // the queued message

// The world's fade state at +0x1283A8.
struct WorldFade {
    be_u32 from;   // +0
    be_u32 to;     // +4
    be_u32 dur;    // +8
    be_u32 word;   // +12
    be_u32 w16;    // +16
    be_u32 flags;  // +20
};
// A fade entry (28 bytes): flags word (top nibble 2, bits 4..8 cleared,
// bit 8 = rev), the two colours at +2 / +6, f32 +12 / +16, +20 the
// duration, +24 = 0.
struct FadeEnt {
    be_u32 flags;     // +0
    pad<8> _4;
    be_u32 from;      // +12
    be_u32 to;        // +16
    be_u32 dur;       // +20
    be_u32 w24;       // +24
};
static_assert(sizeof(FadeEnt) == 28);
// A message queue record (20 bytes).
struct QueueRec {
    be_u32 w0;    // +0
    be_u32 obj;   // +4
    be_u32 msg;   // +8
    be_u32 w12;   // +12
    u8 b16;       // +16
    pad<3> _17;
};
static_assert(sizeof(QueueRec) == 20);
// A flag-map listener node {+0 object, +4 next, u8 +8}.
struct ListenerNode {
    be_u32 obj;              // +0
    gp<ListenerNode> next;   // +4
    u8 b8;                   // +8
};
// The world's byte fields used here.
struct WorldModes {
    pad<30> _0;
    u8 slot;    // +30
    u8 mode;    // +31
    u8 b32;     // +32
    u8 arg;     // +33
    pad<10> _34;
    u8 b44;     // +44
    u8 b45;     // +45
    pad<5> _46;
    u8 b51;     // +51
    u8 fadeIx;  // +52
};
static_assert(offsetof(WorldModes, b44) == 44 && offsetof(WorldModes, fadeIx) == 52);

inline void Copy4(u32 dst, u32 src) {
    for (u32 k = 0; k < 4; ++k) gm<u8>(dst + k) = gm<u8>(src + k);
}

inline void SetFadeEntry(WorldModes* w, u32 n, bool rev, u32 dur) {
    const u32 e = guest(w) + kFadeEnt + n * 28u;
    FadeEnt& f = gm<FadeEnt>(e);
    f.flags = (f.flags & 0x007FFFFFu) | 0x20000000u | (rev ? 0x00800000u : 0u);
    Copy4(e + 2, rev ? kFadeColour : kColour);
    Copy4(e + 6, rev ? kColour : kFadeColour);
    f.from = rev ? 0x3F800000u : 0u;
    f.to = rev ? 0u : 0x3F800000u;
    if (!rev) w->fadeIx = static_cast<u8>(n);
    f.dur = dur;
    f.w24 = 0u;
}

// Fade in (a new entry at the count, which is bumped; u8 +52 = its index).
inline void FadeIn(WorldModes* w, u32 durSrc) {
    const u32 a = global<be_u32>(kFadeA), dur = global<be_u32>(durSrc);
    WorldFade& fd = gm<WorldFade>(guest(w) + kFade);
    fd.flags = (fd.flags & 0x7FFFFFFFu) | 0x40000000u;
    fd.w16 = 0u;
    fd.word = global<be_u32>(kFadeWord).get();
    fd.dur = dur;
    fd.to = a;
    fd.from = 0u;
    be_u32& cnt = gm<be_u32>(guest(w) + kFadeN);
    const u32 n = cnt;
    cnt = n + 1;
    SetFadeEntry(w, n, false, dur);
}

// Fade out: the entry at u8 +52 (the count++ path the original has for a
// negative index is unreachable: cmpwi on a zero-extended byte).
inline void FadeOut(WorldModes* w, u32 durSrc) {
    WorldFade& fd = gm<WorldFade>(guest(w) + kFade);
    fd.from = global<be_u32>(kFadeA).get();
    fd.to = 0u;
    fd.dur = global<be_u32>(durSrc).get();
    fd.word = global<be_u32>(kFadeWord).get();
    fd.w16 = 0u;
    fd.flags = fd.flags | 0xC0000000u;
    const u32 n = w->fadeIx;
    SetFadeEntry(w, n, true, global<be_u32>(durSrc));
}

// Pending_Build, then each listener of the flag-map key kKey is queued
// (Queue_Push_004705F8 inlined: skipped when its object is 0 and its u8 is
// set; the count is kept in a register across the list).
inline void QueueListeners(be_u32* keyAt) {
    call(Pending_Build366E20);
    *keyAt = kKey;
    const u32 slot = call<u32>(SortedMap_Find786C0C, kFlagMap, keyAt);
    const u32 p = gm<be_u32>(slot);
    if (p == 0) return;
    ListenerNode* node = host<ListenerNode>(gm<be_u32>(p));
    if (node == nullptr) return;
    u32 n = global<be_u32>(kQueueN);
    u32 off = n * 20u;
    for (; node != nullptr; node = node->next) {
        const u32 o = node->obj;
        const u8 b = node->b8;
        if (o == 0 && b != 0) continue;
        if (static_cast<s32>(n) >= 128) continue;
        QueueRec& q = gm<QueueRec>(kQueue + off);
        ++n;
        q.w0 = 0u;
        global<be_u32>(kQueueN) = n;
        q.obj = o;
        q.msg = kMsg;
        q.w12 = 0u;
        q.b16 = b;
        off += 20;
    }
}

inline void HudOn(u32 h) {
    if (h != 0) vcall(host<u8>(h), 40, 1u);
}

// vfunc 36(r, 1) of the object at +204 of each user record, when set.
inline void UsersNotify() {
    for (u32 k = 0; k < 2; ++k) {
        const u32 r = gm<be_u32>(kGame + 11968u * k + 204);
        if (r != 0) vcall(host<u8>(r), 36, 1u);
    }
}

// The user record of world slot u8 +30: kGame + 176 + 11968 * u8 (w +
// 0x20C80 * slot + 4209).
inline u32 UserRec(WorldModes* w, u32 slot) {
    return kGame + static_cast<u32>(gm<u8>(guest(w) + slot * 0x00020C80u + 4209)) * 11968u + 176;
}

inline void Sound(u8* out, u32 snd) { call(Sound_Play44C474, out, snd, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u); }

inline u32 Hud(WorldModes* w) { return gm<be_u32>(guest(w) + kHud); }
}  // namespace

// void f(World* w, s32 mode, u8 arg) (no caller reads r3 or f1): u8 +31 =
// mode (old value kept), +32 = 0, +33 = arg; nothing more unless there is
// a player and Obj_ShowPanel(player, kPanels[mode]) (table index
// unchecked). Then: 0: +30 = +51. 1 / 2 / 3: fade in (short / short /
// long), queue the listeners, HUD on, World_SetFlags(2), pause, the users'
// vfunc 36; then 1: the shop gets bit 0x40000000 of +216 and
// Obj_StartMode; 2: the speaker +960 bit 0x40000000 and Obj_SpeakerTalk;
// 3: the shop object +396 bit 0x40000000 and Obj_Play. 4: with a player
// whose +24 has bit 0x8000 of +164 just Obj_SelectEntry(0x0023FDB9), else
// the letterbox cutscene start (fade, listeners, HUD, letterbox widget,
// target, sound, the user record's bit 0x8000 saved in bit 2 of +44 and
// set); unless the old mode was set, the pause and the users' vfunc 36.
// 5..20: Obj_SelectEntry with a per-mode hash (not 11, 16, 17). 23..26:
// fade out (23: only when Obj_RequestFlag; then sound, bit 2 of +44 back
// into the user record and bit 0 of +45 cleared; 24: shop mode off first;
// 25 / 26: notify the speaker / shop first).
void World_SetMode_00470750(WorldModes* w, s32 mode, u8 a2) {
    GuestFrame frame(208);
    u8* sp = frame.as<u8>();

    const u8 old = w->mode;
    w->mode = static_cast<u8>(mode);
    w->b32 = 0;
    w->arg = a2;
    const u32 panel = gm<be_u32>(kPanels + (static_cast<u32>(mode) << 2));
    if (call<u32>(Player_Get) == 0) return;
    const u32 pl = call<u32>(Player_Get);
    if (call<u32>(Obj_ShowPanel, pl, panel) == 0) return;

    auto select = [&](u32 hash) {
        if (call<u32>(Player_Get) == 0) return;
        const u32 p = call<u32>(Player_Get);
        call(Obj_SelectEntry, p, hash);
    };
    auto pauseIn = [&]() {
        HudOn(Hud(w));
        call(World_SetFlags, 2u);
        call(Pause_Request44BB48, 0xFFBFFFFFu);
        call(Pause_Set2AB340, 1u);
        UsersNotify();
    };
    auto key = [&](u32 off) { return reinterpret_cast<be_u32*>(sp + off); };

    switch (mode) {
    case 0:
        w->slot = w->b51;
        return;
    case 1: {
        FadeIn(w, kFadeShort);
        QueueListeners(key(164));
        pauseIn();
        const u32 shop = global<be_u32>(kShop);
        if (shop == 0) return;
        gm<be_u32>(shop + 216) = gm<be_u32>(shop + 216) | 0x40000000u;
        call(Obj_StartMode368360, global<be_u32>(kShop).get());
        return;
    }
    case 2: {
        FadeIn(w, kFadeShort);
        QueueListeners(key(160));
        pauseIn();
        if (call<u32>(Handle_ResolveBit10000, global<be_u32>(kSpeakerH).get()) == 0) return;
        u32 s = call<u32>(Handle_ResolveBit10000, global<be_u32>(kSpeakerH).get());
        gm<be_u32>(s + 960) = gm<be_u32>(s + 960) | 0x40000000u;
        s = call<u32>(Handle_ResolveBit10000, global<be_u32>(kSpeakerH).get());
        call(Obj_SpeakerTalk386560, s, 0u, lf(global<be_f32>(kMinus1)), 0u, 0u, kColour + 4);
        return;
    }
    case 3: {
        FadeIn(w, kFadeLong);
        QueueListeners(key(156));
        pauseIn();
        const u32 s = call<u32>(Handle_ResolveBit20000, global<be_u32>(kShopH).get());
        if (s == 0) return;
        gm<be_u32>(s + 396) = gm<be_u32>(s + 396) | 0x40000000u;
        call(Obj_Play363890, s, 0u, lf(global<be_f32>(kMinus1)), 0u, 0u, kColour + 4);
        return;
    }
    case 4: {
        const u32 p = call<u32>(Player_Get);
        if (p != 0) {
            const u32 q = gm<be_u32>(p + 24);
            if (q != 0 && (gm<be_u32>(q + 164) & 0x8000u) != 0) {
                call(Obj_SelectEntry, p, 0x0023FDB9u);
            } else {
                FadeIn(w, kFadeShort);
                QueueListeners(key(152));
                HudOn(Hud(w));
                u8* ui = call<u8*>(fn::Ui_Find, kLetterbox, 0u);
                if (ui != nullptr) {
                    global<u8>(kLetterboxOn) = static_cast<u8>(vcall<u32>(ui, 84));
                    u8* ui2 = call<u8*>(fn::Ui_Find, kLetterbox, 0u);
                    if (ui2 != nullptr) vcall(ui2, 88, 0u);
                }
                call(Obj_SetTarget168, p, 1u, 0u);
                call(Obj_SelectEntry, p, 0x0023FDB9u);
                call(World_SetFlags, 2u);
                Sound(sp + 144, gm<be_u32>(guest(w) + kSoundA));
                const u32 rec = UserRec(w, w->slot);
                const u32 b44 = w->b44;
                const u32 bit = (gm<be_u32>(rec + 4) >> 15) & 1u;
                w->b44 = static_cast<u8>((b44 & ~4u) | (bit << 2));
                gm<be_u32>(rec + 4) = gm<be_u32>(rec + 4) | 0x8000u;
            }
        }
        if (old != 0) return;
        call(Pause_Set2AB340, 1u);
        call(Pause_Request44BB48, 0xFFBFFFFFu);
        UsersNotify();
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
        if (call<u32>(Player_Get) != 0) {
            const u32 p = call<u32>(Player_Get);
            if (call<u32>(Obj_RequestFlag3736C4, p, 1u) != 0) FadeOut(w, kFadeShort);
            Sound(sp + 148, gm<be_u32>(guest(w) + kSoundB));
            const u32 rec = UserRec(w, w->slot);
            const u32 bit = (static_cast<u32>(w->b44) >> 2) & 1u;
            gm<be_u32>(rec + 4) = (gm<be_u32>(rec + 4) & ~0x8000u) | (bit << 15);
        }
        w->b45 = w->b45 & 0xFEu;
        return;
    }
    case 24: {
        const u32 shop = global<be_u32>(kShop);
        if (shop != 0) call(Obj_ShopModeOff3684B8, shop);
        FadeOut(w, kFadeShort);
        return;
    }
    case 25:
        if (call<u32>(Handle_ResolveBit10000, global<be_u32>(kSpeakerH).get()) != 0) {
            const u32 s = call<u32>(Handle_ResolveBit10000, global<be_u32>(kSpeakerH).get());
            call(Obj_NotifyB386140, s, 0u, 0u);
        }
        FadeOut(w, kFadeShort);
        return;
    case 26: {
        const u32 s = call<u32>(Handle_ResolveBit20000, global<be_u32>(kShopH).get());
        if (s != 0) call(Obj_NotifyA362F1C, s, 0u, 0u);
        FadeOut(w, kFadeLong);
        return;
    }
    default:
        return;
    }
}

}  // namespace sly4::obj

NATIVE(00470750, sly4::obj::World_SetMode_00470750)
