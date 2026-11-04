// Voice channel bookkeeping ("src/common/valloc.cpp") and the type-5 stop
// ("src/common/blocksnd.cpp").
#include "snd.h"

using namespace sly4;
using namespace sly4::snd;

namespace {

constexpr u32 kChannelCount = 0x011E86A4;  // s32
constexpr u32 kChannels = 0x00F8E880;      // u32 Channel*: 104-byte channels
constexpr u32 kChannelSize = 104;
constexpr u32 kChOwner = 40, kChCallback = 44, kChActive = 16;
constexpr u32 kHandlerChannelMask = 20;    // u32[] per handler: channels it owns
// Per-channel bit arrays (one u32 per 32 channels).
constexpr u32 kChPlaying = 0x011E8460;
constexpr u32 kChKeyOn = 0x011E83E0, kChKeyOff = 0x011E8420;
constexpr u32 kChReleasing = 0x011E84A0, kChDetached = 0x011E84E0;
constexpr u32 kChannelFree = 0x006E80CC;   // (channel, 0) (name?)
constexpr u32 kFmtBadChannel = 0x00AFFF2C;

s32 words_for(s32 n) { return (n + 31) / 32; }  // srawi + addze

enum class ChannelOp { kRelease, kDetach };

// Walks the channels owned by handler h. Returns false when an assertion
// told the caller to give up (the lock is then still held, as in the original).
bool Handler_ForChannels(PPUContext& ctx, u32 h, ChannelOp op) {
    const u32 bad_line = op == ChannelOp::kRelease ? 1214 : 1380;
    const u32 trap_pc = op == ChannelOp::kRelease ? 0x006E9A8C : 0x006EA3F8;
    for (s32 w = 0; w < words_for(load<s32>(kChannelCount)); ++w) {
        const u32 mask_ea = h + kHandlerChannelMask + static_cast<u32>(w) * 4;
        if (load<u32>(mask_ea) == 0) continue;
        const u32 word = static_cast<u32>(w) * 4;
        for (s32 ch = w * 32; ch < w * 32 + 32; ++ch) {
            if (!(load<s32>(kChannelCount) > ch)) break;
            const u32 bit = 1u << (ch - (ch / 32) * 32);
            if ((load<u32>(mask_ea) & bit) == 0) continue;
            const u32 offset = static_cast<u32>(ch) * kChannelSize;
            u32 entry = load<u32>(kChannels) + offset;
            if (load<u32>(entry + kChActive) == 0 || load<u32>(entry + kChOwner) != h || load<u32>(entry + kChCallback) == 0) {
                Snd_Log(ctx, kFmtBadChannel, kFileValloc, bad_line);
                if (Snd_AssertFailed(ctx, kFmtInvalidValloc, kFileValloc, bad_line + 1, trap_pc)) return false;
                entry = load<u32>(kChannels) + offset;
            }
            if (load<u32>(entry + kChOwner) != h) continue;

            const u32 playing = load<u32>(kChPlaying + word);
            if ((playing & bit) == 0) {
                if (op == ChannelOp::kRelease) {
                    if ((load<u32>(kChKeyOn + word) & bit) == 0) continue;
                    store<u32>(kChReleasing + word, load<u32>(kChReleasing + word) | bit);
                    if (const u32 cb = load<u32>(entry + kChCallback)) call_ptr(ctx, cb, {entry, h, 3});
                } else if ((load<u32>(kChKeyOn + word) & bit) != 0 || (load<u32>(kChKeyOff + word) & bit) != 0) {
                    store<u32>(kChDetached + word, load<u32>(kChDetached + word) | bit);
                }
                continue;
            }
            store<u32>(kChPlaying + word, playing & ~bit);
            if (op == ChannelOp::kRelease || h != 0) {
                const u32 own = h + static_cast<u32>(ch / 32) * 4 + kHandlerChannelMask;
                store<u32>(own, load<u32>(own) & ~bit);
            }
            entry = load<u32>(kChannels) + offset;
            if (const u32 cb = load<u32>(entry + kChCallback)) call_ptr(ctx, cb, {entry, load<u32>(entry + kChOwner), 2});
            call(ctx, kChannelFree, {static_cast<u64>(static_cast<s64>(ch)), 0});
        }
    }
    return true;
}

}  // namespace

// void Handler_Release(Handler* h)  (name?)
// Lets the handler's channels release: playing channels are freed (after
// their callback with 2), keyed ones marked releasing (callback with 3).
DECOMP(006E9780, Handler_Release) {
    const u32 h = arg<u32>(ctx, 0);
    if (h == 0 && Snd_AssertFailed(ctx, kFmtInvalidValloc, kFileValloc, 1192, 0x006E9B38)) return;
    call(ctx, sfn::Snd_Lock);
    if (Handler_ForChannels(ctx, h, ChannelOp::kRelease)) call(ctx, sfn::Snd_Unlock);
}

// void Handler_Detach(Handler* h)  (name?)
// Frees the handler's playing channels and marks keyed ones detached.
DECOMP(006EA104, Handler_Detach) {
    const u32 h = arg<u32>(ctx, 0);
    call(ctx, sfn::Snd_Lock);
    if (Handler_ForChannels(ctx, h, ChannelOp::kDetach)) call(ctx, sfn::Snd_Unlock);
}

namespace {

constexpr u32 kHasLiveChannels = 0x006E7584;  // (Handler*) -> bool (name?)
constexpr u32 kNextSegment = 0x006D8494;      // (Handler*) -> s32 index or -1 (name?)
constexpr u32 kHLoop = 574, kHSegment = 566, kHSegmentEnd = 564;  // u16
constexpr u32 kHBank = 84, kHSound = 576;     // u32. NOTE: names swapped (+84 is the sound entry, +576 the bank); the reads below are correct

// The segment table pointer: sound + sound+32, plus the bank's +8 (with
// the original's assertions on a missing bank or sound).
u32 Segment_Table(PPUContext& ctx, u32 bank, u32 sound, u32 trap_bank, u32 trap_sound, bool& skip_bank) {
    skip_bank = false;
    if (bank == 0 && Snd_AssertFailed(ctx, kFmtInvalidBlock, kFileBlocksnd, 639, trap_bank)) {
        skip_bank = true;
        return 0;
    }
    u32 base = 0;
    if (sound != 0 || !Snd_AssertFailed(ctx, kFmtInvalidBlock, kFileBlocksnd, 605, trap_sound))
        base = sound + load<u32>(sound + 32);
    return base + load<u32>(bank + 8);
}

}  // namespace

// bool Voice_StopHandler(Handler* h, bool force)  (name?)
// Moves a type-5 handler to its release segment (unless forced or it has
// none), and returns whether it is completely stopped.
DECOMP(006D9584, Voice_StopHandler) {
    const u32 h = arg<u32>(ctx, 0);
    u32 force = arg<u32>(ctx, 1);
    if ((h == 0 || (load<u32>(h) & 0x80000000u) == 0 || ((load<u32>(h) >> 24) & 0x1F) != kHandlerTypeVoice) &&
        Snd_AssertFailed(ctx, kFmtInvalidBlock, kFileBlocksnd, 1412, 0x006D986C))
        return ret(ctx, 0u);
    call(ctx, sfn::Snd_Lock);
    if (static_cast<u32>(call(ctx, kHasLiveChannels, {h})) == 0 && (load<u16>(h + kHLoop) & 1)) force = 1;

    s32 segment = -1;
    if (force == 0) segment = static_cast<s32>(call(ctx, kNextSegment, {h}));
    if (segment > -1) {
        const s32 next = segment + 1;
        const u32 bank = load<u32>(h + kHBank);
        store<u16>(h + kHSegment, static_cast<u16>(next));
        const u32 sound = load<u32>(h + kHSound);
        bool skip;
        u32 table = Segment_Table(ctx, bank, sound, 0x006D97BC, 0x006D9760, skip);
        if (skip) table = 0;
        const u32 bank2 = load<u32>(h + kHBank);
        const u32 sound2 = load<u32>(h + kHSound);
        store<u16>(h + kHSegmentEnd, static_cast<u16>(load<u32>(table + static_cast<u32>(segment) * 8 + 4)));
        u32 table2 = Segment_Table(ctx, bank2, sound2, 0x006D9800, 0x006D9778, skip);
        if (skip) table2 = 0;
        const u32 len = load<u32>(table2 + static_cast<u32>(next) * 8 + 4);
        store<u16>(h + kHSegmentEnd, static_cast<u16>(len + load<u16>(h + kHSegmentEnd) + 1));
    } else {
        store<u16>(h + kHSegment, 0xFFFF);
    }

    u32 alive = load<u32>(h + kHFirstChild) != 0 ? 1u : static_cast<u32>(call(ctx, kHasLiveChannels, {h}));
    if (static_cast<s16>(load<u16>(h + kHSegment)) != -1) alive = 1;
    call(ctx, sfn::Snd_Unlock);
    ret(ctx, alive == 0 ? 1u : 0u);
}
