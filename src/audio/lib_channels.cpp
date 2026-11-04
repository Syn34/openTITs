// Sound library voice channels ("src/common/valloc.cpp"): freeing and pausing.
#include "snd.h"

using namespace sly4;
using namespace sly4::snd;

namespace {

constexpr u32 kChannelCount = 0x011E86A4;
constexpr u32 kChannels = 0x00F8E880;
constexpr u32 kChannelSize = 104;
constexpr u32 kPendingChannels = 0x00F8E884;   // list linked by +0
constexpr u32 kCheckGroups = 0x011D7D54;       // u32: verify group counts
constexpr u32 kGroupPlaying = 0x011E86D4;      // s32[32]
constexpr u32 kTypeFree = 0x011E86BC;          // s32[5] free channels per data type
constexpr u32 kChRunning = 0x011E8520;         // u32[] bit per channel
constexpr u32 kChPaused = 0x011E8560;
constexpr u32 kReleaseVoice = 0x006EFE90;      // (hw voice) (name?)
constexpr u32 kChannelReset = 0x006E7EC0;      // (ch) (name?)
constexpr u32 kFmtBadType = 0x00AFFD94;        // "... Invalid data type.\n"
constexpr u32 kFmtGroupCount = 0x00AFFD6C;     // "... Invalid playing group count.\n"
constexpr u32 kFmtBadChannel = 0x00AFFF2C;

s32 words_for(s32 n) { return (n + 31) / 32; }

// Pause (true) or resume a handler's channels. Returns false when an
// assertion gave up (the lock is then still held).
bool Channels_SetPaused(PPUContext& ctx, u32 h, bool pause) {
    const u32 line = pause ? 1278 : 1330;
    const u32 trap_pc = pause ? 0x006E9DAC : 0x006EA0A0;
    s32 n = load<s32>(kChannelCount);
    u32 table = load<u32>(kChannels);
    for (s32 w = 0; w < words_for(n); ++w) {
        const u32 mask_ea = h + 20 + static_cast<u32>(w) * 4;
        if (load<u32>(mask_ea) == 0) continue;
        for (s32 ch = w * 32; ch < w * 32 + 32; ++ch) {
            if (!(n > ch)) break;
            const u32 bit = 1u << (ch - (ch / 32) * 32);
            if ((load<u32>(mask_ea) & bit) == 0) continue;
            const u32 offset = static_cast<u32>(ch) * kChannelSize;
            u32 entry = table + offset;
            if (load<u32>(entry + 16) == 0 || load<u32>(entry + 40) != h) {
                Snd_Log(ctx, kFmtBadChannel, kFileValloc, line);
                if (Snd_AssertFailed(ctx, kFmtInvalidValloc, kFileValloc, line + 1, trap_pc)) return false;
                n = load<s32>(kChannelCount);
                table = load<u32>(kChannels);
                entry = table + offset;
            }
            if (load<u32>(entry + 40) != h) continue;
            if (!pause && (load<u32>(kGroupFlags + static_cast<u32>(static_cast<s32>(static_cast<s8>(ref<u8>(h + kHGroup)))) * 4) & 2))
                continue;
            const u32 word = static_cast<u32>(ch / 32) * 4;
            const u32 clear = pause ? kChRunning : kChPaused, set = pause ? kChPaused : kChRunning;
            const u32 a = load<u32>(clear + word) & ~bit;
            const u32 b = load<u32>(set + word) | bit;
            store<u32>(clear + word, a);
            store<u32>(set + word, b);
        }
    }
    return true;
}

}  // namespace

// void Channel_Free(s32 ch, bool keep_voice)
// Takes the channel off the pending list, updates the group and type
// counts, releases its hardware voice (unless kept) and resets it.
DECOMP(006E80CC, Channel_Free) {
    const s32 ch = arg<s32>(ctx, 0);
    const u32 keep = arg<u32>(ctx, 1) & 0xFF;
    if ((ch < 0 || !(load<s32>(kChannelCount) > ch)) &&
        Snd_AssertFailed(ctx, kFmtInvalidValloc, kFileValloc, 257, 0x006E8360))
        return;
    const u32 c = load<u32>(kChannels) + static_cast<u32>(ch) * kChannelSize;
    const u32 head = load<u32>(kPendingChannels);
    if (c == head) {
        store<u32>(kPendingChannels, load<u32>(c));
    } else {
        for (u32 p = head; p != 0; p = load<u32>(p)) {
            if (load<u32>(p) == c) {
                store<u32>(p, load<u32>(c));
                break;
            }
        }
    }

    s32 group = static_cast<s8>(ref<u8>(c + 10));
    if (group > -1 && group < 32) {
        if (load<u32>(kCheckGroups) != 0 && load<u32>(kGroupPlaying + static_cast<u32>(group) * 4) == 0 &&
            load<u32>(c + 16) == 1) {
            Snd_Log(ctx, kFmtGroupCount, kFileValloc, 291);
            if (Snd_AssertFailed(ctx, kFmtInvalidValloc, kFileValloc, 292, 0x006E8320)) return;
            group = static_cast<s8>(ref<u8>(c + 10));
        }
        const u32 slot = kGroupPlaying + static_cast<u32>(group) * 4;
        if (load<u32>(slot) != 0 && load<u32>(c + 16) == 1) store<u32>(slot, load<u32>(slot) - 1);
    }
    s32 type = static_cast<s8>(ref<u8>(c + 11));
    if (type < 0 || !(type < 5)) {
        Snd_Log(ctx, kFmtBadType, kFileValloc, 309);
        if (Snd_AssertFailed(ctx, kFmtInvalidValloc, kFileValloc, 310, 0x006E82C4)) return;
        type = static_cast<s8>(ref<u8>(c + 11));
    }
    const u32 slot = kTypeFree + static_cast<u32>(type) * 4;
    store<u32>(slot, load<u32>(slot) + 1);
    if (keep == 0) call(ctx, kReleaseVoice, {load<u32>(c + 96)});
    call(ctx, kChannelReset, {static_cast<u64>(static_cast<s64>(ch))});
}

// void Handler_OnPause(Handler* h) / Handler_OnResume(Handler* h)
// Move the handler's channels between the running and paused bit sets
// (resume skips channels whose group is paused).
DECOMP(006E9B40, Handler_OnPause) {
    const u32 h = arg<u32>(ctx, 0);
    if (h == 0 && Snd_AssertFailed(ctx, kFmtInvalidValloc, kFileValloc, 1257, 0x006E9E04)) return;
    call(ctx, sfn::Snd_Lock);
    if (Channels_SetPaused(ctx, h, true)) call(ctx, sfn::Snd_Unlock);
}
DECOMP(006E9E0C, Handler_OnResume) {
    const u32 h = arg<u32>(ctx, 0);
    if (h == 0 && Snd_AssertFailed(ctx, kFmtInvalidValloc, kFileValloc, 1309, 0x006EA0FC)) return;
    call(ctx, sfn::Snd_Lock);
    if (Channels_SetPaused(ctx, h, false)) call(ctx, sfn::Snd_Unlock);
}
