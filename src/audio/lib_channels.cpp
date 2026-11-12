// Sound library voice channels ("src/common/valloc.cpp"): freeing and pausing.
#include "snd.h"

namespace sly4::snd {
using namespace opentit::native;

namespace {

constexpr u32 kPendingChannels = 0x00F8E884;   // list linked by +0
constexpr u32 kCheckGroups = 0x011D7D54;       // u32: verify group counts
constexpr u32 kGroupPlaying = 0x011D86D4;      // s32[32]
constexpr u32 kTypeFree = 0x011D86BC;          // s32[5] free channels per data type
constexpr u32 kChRunning = 0x011D8520;         // u32[] bit per channel
constexpr u32 kChPaused = 0x011D8560;
constexpr u32 kFmtBadType = 0x00AFFD94;        // "... Invalid data type.\n"
constexpr u32 kFmtGroupCount = 0x00AFFD6C;     // "... Invalid playing group count.\n"
constexpr u32 kFmtBadChannel = 0x00AFFF2C;

be_u32& Word(u32 array, u32 offset) { return gref<be_u32>(array + offset); }

// Pause (true) or resume a handler's channels. Returns false when an
// assertion gave up (the lock is then still held).
bool Channels_SetPaused(Handler* hp, bool pause) {
    Handler& h = gref(hp);
    const u32 self = guest(hp);
    const u32 line = pause ? 1278 : 1330;
    const u32 trap_pc = pause ? 0x006E9DAC : 0x006EA0A0;
    s32 n = Snd_ChannelCount();
    u32 table = Snd_ChannelTable();
    for (s32 w = 0; w < Snd_ChannelWords(n); ++w) {
        be_u32& mask = h.channel_word(w);
        if (mask == 0) continue;
        for (s32 ch = w * 32; ch < w * 32 + 32; ++ch) {
            if (!(n > ch)) break;
            const u32 bit = 1u << (ch - (ch / 32) * 32);
            if ((mask & bit) == 0) continue;
            const u32 offset = static_cast<u32>(ch) * sizeof(Channel);
            Channel* entry = &gref<Channel>(table + offset);
            if (entry->active == 0 || entry->owner != self) {
                Snd_LogAt(kFmtBadChannel, kFileValloc, line);
                if (Snd_AssertFailed(kFmtInvalidValloc, kFileValloc, line + 1, trap_pc)) return false;
                n = Snd_ChannelCount();
                table = Snd_ChannelTable();
                entry = &gref<Channel>(table + offset);
            }
            if (entry->owner != self) continue;
            if (!pause && (Word(kGroupFlags, static_cast<u32>(static_cast<s32>(static_cast<s8>(h.group))) * 4) & 2))
                continue;
            const u32 word = static_cast<u32>(ch / 32) * 4;
            const u32 clear = pause ? kChRunning : kChPaused, set = pause ? kChPaused : kChRunning;
            const u32 a = Word(clear, word) & ~bit;
            const u32 b = Word(set, word) | bit;
            Word(clear, word) = a;
            Word(set, word) = b;
        }
    }
    return true;
}

}  // namespace

// void Channel_Free(s32 ch, bool keep_voice)
// Takes the channel off the pending list, updates the group and type
// counts, releases its hardware voice (unless kept) and resets it.
void Channel_Free(s32 ch, u8 keep) {
    if ((ch < 0 || !(Snd_ChannelCount() > ch)) &&
        Snd_AssertFailed(kFmtInvalidValloc, kFileValloc, 257, 0x006E8360))
        return;
    Channel& c = gref<Channel>(Snd_ChannelTable() + static_cast<u32>(ch) * sizeof(Channel));
    gp<Channel>& head = global<gp<Channel>>(kPendingChannels);
    if (head.get() == &c) {
        head = c.next.get();
    } else {
        for (Channel* p = head; p; p = p->next) {
            if (p->next.get() == &c) {
                p->next = c.next.get();
                break;
            }
        }
    }

    s32 group = static_cast<s8>(c.group);
    if (group > -1 && group < 32) {
        if (global<be_u32>(kCheckGroups) != 0 && Word(kGroupPlaying, static_cast<u32>(group) * 4) == 0 && c.active == 1) {
            Snd_LogAt(kFmtGroupCount, kFileValloc, 291);
            if (Snd_AssertFailed(kFmtInvalidValloc, kFileValloc, 292, 0x006E8320)) return;
            group = static_cast<s8>(c.group);
        }
        be_u32& slot = Word(kGroupPlaying, static_cast<u32>(group) * 4);
        if (slot != 0 && c.active == 1) slot = slot - 1;
    }
    s32 type = static_cast<s8>(c.type);
    if (type < 0 || !(type < 5)) {
        Snd_LogAt(kFmtBadType, kFileValloc, 309);
        if (Snd_AssertFailed(kFmtInvalidValloc, kFileValloc, 310, 0x006E82C4)) return;
        type = static_cast<s8>(c.type);
    }
    be_u32& slot = Word(kTypeFree, static_cast<u32>(type) * 4);
    slot = slot + 1;
    if (keep == 0) Channel_ReleaseVoice(c.hw_voice);
    Channel_Reset(ch);
}

// void Handler_OnPause(Handler* h) / Handler_OnResume(Handler* h)
// Move the handler's channels between the running and paused bit sets
// (resume skips channels whose group is paused).
void Handler_OnPause(Handler* h) {
    if (!h && Snd_AssertFailed(kFmtInvalidValloc, kFileValloc, 1257, 0x006E9E04)) return;
    Snd_Lock();
    if (Channels_SetPaused(h, true)) Snd_Unlock();
}
void Handler_OnResume(Handler* h) {
    if (!h && Snd_AssertFailed(kFmtInvalidValloc, kFileValloc, 1309, 0x006EA0FC)) return;
    Snd_Lock();
    if (Channels_SetPaused(h, false)) Snd_Unlock();
}

}  // namespace sly4::snd

NATIVE(006E80CC, sly4::snd::Channel_Free)
NATIVE(006E9B40, sly4::snd::Handler_OnPause)
NATIVE(006E9E0C, sly4::snd::Handler_OnResume)
