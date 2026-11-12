// Voice channel bookkeeping ("src/common/valloc.cpp") and the type-5 stop
// ("src/common/blocksnd.cpp").
#include "snd.h"

namespace sly4::snd {
using namespace opentit::native;

namespace {

// Per-channel bit arrays (one u32 per 32 channels).
constexpr u32 kChPlaying = 0x011D8460;
constexpr u32 kChKeyOn = 0x011D83E0, kChKeyOff = 0x011D8420;
constexpr u32 kChReleasing = 0x011D84A0, kChDetached = 0x011D84E0;
constexpr u32 kFmtBadChannel = 0x00AFFF2C;

be_u32& Bits(u32 array, u32 word) { return gref<be_u32>(array + word); }

enum class ChannelOp { kRelease, kDetach };

// Walks the channels owned by handler h. Returns false when an assertion
// told the caller to give up (the lock is then still held, as in the original).
bool Handler_ForChannels(Handler* hp, ChannelOp op) {
    Handler& h = gref(hp);
    const u32 self = guest(hp);
    const u32 bad_line = op == ChannelOp::kRelease ? 1214 : 1380;
    const u32 trap_pc = op == ChannelOp::kRelease ? 0x006E9A8C : 0x006EA3F8;
    for (s32 w = 0; w < Snd_ChannelWords(Snd_ChannelCount()); ++w) {
        be_u32& mask = h.channel_word(w);
        if (mask == 0) continue;
        const u32 word = static_cast<u32>(w) * 4;
        for (s32 ch = w * 32; ch < w * 32 + 32; ++ch) {
            if (!(Snd_ChannelCount() > ch)) break;
            const u32 bit = 1u << (ch - (ch / 32) * 32);
            if ((mask & bit) == 0) continue;
            const u32 offset = static_cast<u32>(ch) * sizeof(Channel);
            Channel* entry = &gref<Channel>(Snd_ChannelTable() + offset);
            if (entry->active == 0 || entry->owner != self || entry->callback == 0) {
                Snd_LogAt(kFmtBadChannel, kFileValloc, bad_line);
                if (Snd_AssertFailed(kFmtInvalidValloc, kFileValloc, bad_line + 1, trap_pc)) return false;
                entry = &gref<Channel>(Snd_ChannelTable() + offset);
            }
            if (entry->owner != self) continue;

            const u32 playing = Bits(kChPlaying, word);
            if ((playing & bit) == 0) {
                if (op == ChannelOp::kRelease) {
                    if ((Bits(kChKeyOn, word) & bit) == 0) continue;
                    Bits(kChReleasing, word) = Bits(kChReleasing, word) | bit;
                    if (const u32 cb = entry->callback) call_ptr(cb, entry, hp, 3u);
                } else if ((Bits(kChKeyOn, word) & bit) != 0 || (Bits(kChKeyOff, word) & bit) != 0) {
                    Bits(kChDetached, word) = Bits(kChDetached, word) | bit;
                }
                continue;
            }
            Bits(kChPlaying, word) = playing & ~bit;
            if (op == ChannelOp::kRelease || self != 0) {
                be_u32& own = h.channel_word(ch / 32);
                own = own & ~bit;
            }
            entry = &gref<Channel>(Snd_ChannelTable() + offset);
            if (const u32 cb = entry->callback) call_ptr(cb, entry, entry->owner, 2u);
            Channel_Free(ch, 0);
        }
    }
    return true;
}

}  // namespace

// void Handler_Release(Handler* h)  (name?)
// Lets the handler's channels release: playing channels are freed (after
// their callback with 2), keyed ones marked releasing (callback with 3).
void Handler_Release(Handler* h) {
    if (!h && Snd_AssertFailed(kFmtInvalidValloc, kFileValloc, 1192, 0x006E9B38)) return;
    Snd_Lock();
    if (Handler_ForChannels(h, ChannelOp::kRelease)) Snd_Unlock();
}

// void Handler_Detach(Handler* h)  (name?)
// Frees the handler's playing channels and marks keyed ones detached.
void Handler_Detach(Handler* h) {
    Snd_Lock();
    if (Handler_ForChannels(h, ChannelOp::kDetach)) Snd_Unlock();
}

namespace {

// The segment table pointer: sound + sound+32, plus the bank's +8 (with
// the original's assertions on a missing bank or sound). (The handler's
// +84 is the sound entry and +576 the bank; the original's names are
// swapped here: `bank` is +84.)
u32 Segment_Table(u32 bank, u32 sound, u32 trap_bank, u32 trap_sound, bool& skip_bank) {
    skip_bank = false;
    if (bank == 0 && Snd_AssertFailed(kFmtInvalidBlock, kFileBlocksnd, 639, trap_bank)) {
        skip_bank = true;
        return 0;
    }
    u32 base = 0;
    if (sound != 0 || !Snd_AssertFailed(kFmtInvalidBlock, kFileBlocksnd, 605, trap_sound))
        base = sound + gref<be_u32>(sound + 32);
    return base + gref<be_u32>(bank + 8);
}

}  // namespace

// bool Voice_StopHandler(Handler* h, bool force)  (name?)
// Moves a type-5 handler to its release segment (unless forced or it has
// none), and returns whether it is completely stopped.
u32 Voice_StopHandler(VoiceHandler* hp, u32 force) {
    if ((!hp || !hp->live() || hp->type() != kHandlerTypeVoice) &&
        Snd_AssertFailed(kFmtInvalidBlock, kFileBlocksnd, 1412, 0x006D986C))
        return 0;
    VoiceHandler& h = gref(hp);
    Snd_Lock();
    if (!Handler_HasLiveChannels(hp) && (h.loop & 1)) force = 1;

    s32 segment = -1;
    if (force == 0) segment = Handler_NextSegment(hp);
    if (segment > -1) {
        const s32 next = segment + 1;
        const u32 bank = h.owner;
        h.segment = static_cast<s16>(next);
        const u32 sound = h.bank;
        bool skip;
        u32 table = Segment_Table(bank, sound, 0x006D97BC, 0x006D9760, skip);
        if (skip) table = 0;
        const u32 bank2 = h.owner;
        const u32 sound2 = h.bank;
        h.seg_end = static_cast<u16>(gref<be_u32>(table + static_cast<u32>(segment) * 8 + 4));
        u32 table2 = Segment_Table(bank2, sound2, 0x006D9800, 0x006D9778, skip);
        if (skip) table2 = 0;
        const u32 len = gref<be_u32>(table2 + static_cast<u32>(next) * 8 + 4);
        h.seg_end = static_cast<u16>(len + h.seg_end + 1);
    } else {
        h.segment = -1;
    }

    u32 alive = h.first_child ? 1u : (Handler_HasLiveChannels(hp) ? 1u : 0u);
    if (h.segment != -1) alive = 1;
    Snd_Unlock();
    return alive == 0 ? 1u : 0u;
}

}  // namespace sly4::snd

NATIVE(006E9780, sly4::snd::Handler_Release)
NATIVE(006EA104, sly4::snd::Handler_Detach)
NATIVE(006D9584, sly4::snd::Voice_StopHandler)
