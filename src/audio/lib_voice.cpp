// Sound library voice helpers ("src/common/blocksnd.cpp", "api.cpp", "sndhand.cpp").
#include "snd.h"

using namespace sly4;
using namespace sly4::snd;

namespace {
constexpr u32 kChannelCount = 0x011E86A4;
constexpr u32 kBankMagic = 0x6B6C4253;   // "klBS"
constexpr u32 kFmtDeactivated = 0x00AFFC90;  // "... Deactivated handler on active list.\n"
constexpr u32 kSoundNames = 0x006D8D44;  // (bank) -> name entries (16 bytes: +0 offset, +12 s16 id) (name?)
constexpr u32 kBankHeader = 0x006D8018;  // (bank) -> header; +12 string table offset (name?)
constexpr u32 kSnprintf = 0x007241F4;
constexpr u32 kFmtS = 0x00AFE02C;        // "%s"
constexpr u32 kPitchConsts = 0x006D74F8; // f32 { 340.2, 339.2, 1536 }
constexpr u32 kLog2 = 0x007062C8;        // (f32, -1) -> f32 (name?)
constexpr u32 kGrainMarker = 43;
s32 words_for(s32 n) { return (n + 31) / 32; }
}  // namespace

// bool Handler_HasLiveChannels(Handler* h): any word of h's channel mask set.
DECOMP(006E7584, Handler_HasLiveChannels) {
    const u32 h = arg<u32>(ctx, 0);
    const s32 n = load<s32>(kChannelCount);
    for (s32 w = 0; w < words_for(n); ++w)
        if (load<u32>(h + 20 + static_cast<u32>(w) * 4) != 0) return ret(ctx, 1u);
    ret(ctx, 0u);
}

// s32 Snd_PitchFromSpeed(f32 speed)  (name?)
// 1536 * log2(340.2 / (340.2 - min(speed, 339.2))) as an int (Doppler shift).
DECOMP(006D7508, Snd_PitchFromSpeed) {
    f32 s = static_cast<f32>(farg(ctx, 0));
    const f32 c = load<f32>(kPitchConsts);
    if (s >= c) s = load<f32>(kPitchConsts + 4);
    const f32 ratio = fdivs(c, fsubs(c, s));
    const f64 r = callf(ctx, kLog2, {0, static_cast<u64>(-1)}, {ratio});
    ret(ctx, static_cast<s32>(static_cast<u32>(rt::fctiw(fmuls(r, load<f32>(kPitchConsts + 8)), true))));
}

// void Handler_StopOwnedBy(void* owner, bool force, bool flag)  (was "Sound_Prepare")
// Stops (recursively) every active handler whose +84 is `owner`.
DECOMP(006E6AAC, Handler_StopOwnedBy) {
    const u32 owner = arg<u32>(ctx, 0);
    const u64 force = ctx.gpr[4], flag = ctx.gpr[5];
    if (owner == 0 && Snd_AssertFailed(ctx, kFmtInvalid, kFileSndhand, 1624, 0x006E6C08)) return;
    call(ctx, sfn::Snd_Lock);
    for (u32 h = load<u32>(kFirstActive); h != 0; h = load<u32>(h + kHNextActive)) {
        if ((load<u32>(h) & 0x80000000u) == 0) {
            Snd_Log(ctx, kFmtDeactivated, kFileSndhand, 1641);
            if (Snd_AssertFailed(ctx, kFmtInvalid, kFileSndhand, 1643, 0x006E6BCC)) return;
        }
        if (load<u32>(h + 84) == owner) call(ctx, sfn::Handler_Stop, {h, 1, force, flag});
    }
    call(ctx, sfn::Snd_Unlock);
}

// bool Voice_Steal(SoundEntry* e, bool by_sound, Handler** out)  (name?)
// Counts the live type-5 handlers playing `e`; under the sound's voice limit
// (+5) returns true; otherwise picks the quietest (flag 0x10) or oldest
// (flag 0x20) to replace into *out and returns whether there was one.
DECOMP(006D81C0, Voice_Steal) {
    const u32 e = arg<u32>(ctx, 0), by_sound = arg<u32>(ctx, 1), out = arg<u32>(ctx, 2);
    if ((e == 0 || out == 0) && Snd_AssertFailed(ctx, kFmtInvalidBlock, kFileBlocksnd, 178, 0x006D83A8))
        return ret(ctx, 0u);
    if (out != 0) store<u32>(out, 0);
    if ((load<u16>(e + 6) & 0x38) == 0 || ref<u8>(e + 5) == 0) return ret(ctx, 0u);

    call(ctx, sfn::Snd_Lock);
    u32 best = 0;
    s32 count = 0;
    u32 h = static_cast<u32>(call(ctx, sfn::Handler_First));
    if (h != 0) {
        const u32 flags = load<u16>(e + 6);
        const bool oldest = (flags & 0x20) != 0, quietest = (flags & 0x10) != 0;
        for (; h != 0; h = load<u32>(h + kHNextActive)) {
            if (((load<u32>(h) >> 24) & 0x1F) != kHandlerTypeVoice) continue;
            if (!(by_sound != 0 && load<u32>(h + 580) == e) && load<u32>(h + 84) != e) continue;
            if (load<u32>(h + kHFlags) & 0x80) continue;
            ++count;
            if (!quietest && !oldest) continue;
            if (best == 0) {
                best = h;
                continue;
            }
            if (quietest) {
                const f32 a = fmuls(fmuls(load<f32>(h + 288), load<f32>(h + 296)), load<f32>(h + 464));
                const f32 b = fmuls(fmuls(load<f32>(best + 288), load<f32>(best + 296)), load<f32>(best + 464));
                if (a < b) {
                    best = h;
                    continue;
                }
            }
            if (oldest && load<u32>(h + 12) < load<u32>(best + 12)) best = h;
        }
    }
    if (count < static_cast<s8>(ref<u8>(e + 5))) {
        call(ctx, sfn::Snd_Unlock);
        return ret(ctx, 1u);
    }
    if (best != 0) store<u32>(out, best);
    call(ctx, sfn::Snd_Unlock);
    ret(ctx, best != 0 ? 1u : 0u);
}

// bool Bank_GetSoundName(Bank* bank, s16 id, char out[128])
DECOMP(006D5BFC, Bank_GetSoundName) {
    const u32 bank = arg<u32>(ctx, 0);
    const s32 id = static_cast<s16>(arg<u32>(ctx, 1) & 0xFFFF);
    const u32 out = arg<u32>(ctx, 2);
    if ((bank == 0 || out == 0) && Snd_AssertFailed(ctx, kFmtInvalid, kFileApi, 3611, 0x006D5D98)) return ret(ctx, 0u);
    if (load<u32>(bank) != kBankMagic || (load<u32>(bank + 8) & 0x100) == 0) return ret(ctx, 0u);
    if (id > static_cast<s16>(load<u16>(bank + 22))) return ret(ctx, 0u);
    const u32 names = static_cast<u32>(call(ctx, kSoundNames, {bank}));
    const u32 header = static_cast<u32>(call(ctx, kBankHeader, {bank}));
    const u32 header2 = static_cast<u32>(call(ctx, kBankHeader, {bank}));
    const s32 n = static_cast<s16>(load<u16>(bank + 22));
    const u32 strings = header + load<u32>(header2 + 12);
    for (s32 i = 0; i < n; ++i) {
        if (static_cast<s16>(load<u16>(names + static_cast<u32>(i) * 16 + 12)) != id) continue;
        call(ctx, kSnprintf, {out, 127, kFmtS, strings + load<u32>(names + static_cast<u32>(i) * 16)});
        ref<u8>(out + 127) = 0;
        return ret(ctx, 1u);
    }
    ret(ctx, 0u);
}

// s32 Handler_NextSegment(Handler* h)  (name?)
// The index of the next loop marker grain (type 43) of the handler's sound,
// or -1 (none, it is the last grain, or no segment and no live channels).
DECOMP(006D8494, Handler_NextSegment) {
    const u32 h = arg<u32>(ctx, 0);
    if ((h == 0 || ((load<u32>(h) >> 24) & 0x1F) != kHandlerTypeVoice) &&
        Snd_AssertFailed(ctx, kFmtInvalidBlock, kFileBlocksnd, 269, 0x006D869C))
        return ret(ctx, 0u);
    if (static_cast<s16>(load<u16>(h + 566)) < 0 &&
        static_cast<u32>(call(ctx, 0x006E7584, {h})) == 0)
        return ret(ctx, -1);

    const u32 sound = load<u32>(h + 84);
    const u32 bank = load<u32>(h + 576);
    u32 grains;
    bool checks = true;
    if (sound == 0 || bank == 0) {
        if (Snd_AssertFailed(ctx, kFmtInvalidBlock, kFileBlocksnd, 311, 0x006D863C)) return ret(ctx, 0u);
        if (sound == 0) {
            if (Snd_AssertFailed(ctx, kFmtInvalidBlock, kFileBlocksnd, 639, 0x006D85F8)) {
                grains = 0;
                checks = false;
            }
        }
    }
    if (checks) {
        u32 base = 0;
        if (bank != 0 || !Snd_AssertFailed(ctx, kFmtInvalidBlock, kFileBlocksnd, 605, 0x006D85B4))
            base = bank + load<u32>(bank + 32);
        grains = base + load<u32>(sound + 8);
    }
    const s32 n = ref<u8>(sound + 4);
    for (s32 i = 0; i < n; ++i) {
        if (ref<u8>(grains + static_cast<u32>(i) * 8) != kGrainMarker) continue;
        return ret(ctx, i == n - 1 ? -1 : i);
    }
    ret(ctx, -1);
}
