// Sound library voice helpers ("src/common/blocksnd.cpp", "api.cpp", "sndhand.cpp").
#include "snd.h"

namespace sly4::snd {
using namespace opentit::native;

namespace {
constexpr u32 kBankMagic = 0x6B6C4253;   // "klBS"
constexpr u32 kFmtDeactivated = 0x00AFFC90;  // "... Deactivated handler on active list.\n"
constexpr u32 kSoundNames = 0x006D8D44;  // (bank) -> name entries (16 bytes: +0 offset, +12 s16 id) (name?)
constexpr u32 kBankHeader = 0x006D8018;  // (bank) -> header; +12 string table offset (name?)
constexpr u32 kSnprintf = 0x007241F4;
constexpr u32 kFmtS = 0x00AFE02C;        // "%s"
constexpr u32 kPitchConsts = 0x006D74F8; // f32 { 340.2, 339.2, 1536 }
constexpr u32 kGrainMarker = 43;

// A bank's sound table entry (12 bytes).
struct SoundEntry {
    s8 volume;             // +0
    s8 b1;                 // +1
    be_s16 pitch;          // +2
    u8 grains;             // +4 grain count
    s8 voice_limit;        // +5
    be_u16 flags;          // +6: 2 prepare, 4 exclusive, 8 limited, 0x10 steal quietest, 0x20 steal oldest
    be_u32 grain_offset;   // +8
};
static_assert(sizeof(SoundEntry) == 12);

struct NameEntry {
    be_u32 offset;  // +0
    pad<8> _4;
    be_s16 id;      // +12
    pad<2> _14;
};
}  // namespace

// bool Handler_HasLiveChannels(Handler* h): any word of h's channel mask set.
bool Handler_HasLiveChannels(Handler* h) {
    const s32 n = Snd_ChannelCount();
    for (s32 w = 0; w < Snd_ChannelWords(n); ++w)
        if (gref(h).channel_word(w) != 0) return true;
    return false;
}

// s32 Snd_PitchFromSpeed(f32 speed)  (name?)
// 1536 * log2(340.2 / (340.2 - min(speed, 339.2))) as an int (Doppler shift).
s32 Snd_PitchFromSpeed(f32 s) {
    const be_f32* k = host<be_f32>(kPitchConsts);
    const f32 c = k[0];
    if (s >= c) s = k[1];
    const f32 ratio = fdivs(c, fsubs(c, s));
    const f32 r = Math_FLog(ratio, -1);
    return static_cast<s32>(static_cast<u32>(rt::fctiw(fmuls(r, k[2]), true)));
}

// void Handler_StopOwnedBy(void* owner, bool force, bool flag)  (was "Sound_Prepare")
// Stops (recursively) every active handler whose +84 is `owner`.
void Handler_StopOwnedBy(u32 owner, u64 force, u64 flag) {
    if (owner == 0 && Snd_AssertFailed(kFmtInvalidSndhand, kFileSndhand, 1624, 0x006E6C08)) return;
    Snd_Lock();
    for (Handler* h = global<gp<Handler>>(kFirstActive); h; h = h->next_active) {
        if (!h->live()) {
            Snd_LogAt(kFmtDeactivated, kFileSndhand, 1641);
            if (Snd_AssertFailed(kFmtInvalidSndhand, kFileSndhand, 1643, 0x006E6BCC)) return;
        }
        if (h->owner == owner) Handler_Stop(h, 1, static_cast<u32>(force), static_cast<u32>(flag));
    }
    Snd_Unlock();
}

// bool Voice_Steal(SoundEntry* e, bool by_sound, Handler** out)  (name?)
// Counts the live type-5 handlers playing `e`; under the sound's voice limit
// (+5) returns true; otherwise picks the quietest (flag 0x10) or oldest
// (flag 0x20) to replace into *out and returns whether there was one.
bool Voice_Steal(u32 e, u32 by_sound, be_u32* out) {
    if ((e == 0 || !out) && Snd_AssertFailed(kFmtInvalidBlock, kFileBlocksnd, 178, 0x006D83A8)) return false;
    if (out) *out = 0u;
    const SoundEntry& entry = gref<SoundEntry>(e);
    if ((entry.flags & 0x38) == 0 || entry.voice_limit == 0) return false;

    Snd_Lock();
    Handler* best = nullptr;
    s32 count = 0;
    Handler* h = Handler_First();
    if (h) {
        const u32 flags = entry.flags;
        const bool oldest = (flags & 0x20) != 0, quietest = (flags & 0x10) != 0;
        for (; h; h = h->next_active) {
            if (h->type() != kHandlerTypeVoice) continue;
            if (!(by_sound != 0 && static_cast<VoiceHandler*>(h)->entry == e) && h->owner != e) continue;
            if (h->flags & 0x80) continue;
            ++count;
            if (!quietest && !oldest) continue;
            if (!best) {
                best = h;
                continue;
            }
            if (quietest) {
                const f32 a = fmuls(fmuls(h->gain, h->f296), gref<be_f32>(guest(h) + 464));
                const f32 b = fmuls(fmuls(best->gain, best->f296), gref<be_f32>(guest(best) + 464));
                if (a < b) {
                    best = h;
                    continue;
                }
            }
            if (oldest && h->tick < best->tick) best = h;
        }
    }
    if (count < entry.voice_limit) {
        Snd_Unlock();
        return true;
    }
    if (best) gref(out) = guest(best);
    Snd_Unlock();
    return best != nullptr;
}

// bool Bank_GetSoundName(Bank* bank, s16 id, char out[128])
bool Bank_GetSoundName(u32 bank, s16 id, u32 out) {
    if ((bank == 0 || out == 0) && Snd_AssertFailed(kFmtInvalid, kFileApi, 3611, 0x006D5D98)) return false;
    if (gref<be_u32>(bank) != kBankMagic || (gref<be_u32>(bank + 8) & 0x100) == 0) return false;
    const be_s16& count = gref<be_s16>(bank + 22);
    if (id > count) return false;
    const u32 names = call<u32>(kSoundNames, bank);
    const u32 header = call<u32>(kBankHeader, bank);
    const u32 header2 = call<u32>(kBankHeader, bank);
    const s32 n = count;
    const u32 strings = header + gref<be_u32>(header2 + 12);
    for (s32 i = 0; i < n; ++i) {
        const NameEntry& ne = gref<NameEntry>(names + static_cast<u32>(i) * sizeof(NameEntry));
        if (ne.id != id) continue;
        call(kSnprintf, out, 127u, kFmtS, strings + ne.offset);
        gref<u8>(out + 127) = 0;
        return true;
    }
    return false;
}

// s32 Handler_NextSegment(Handler* h)  (name?)
// The index of the next loop marker grain (type 43) of the handler's sound,
// or -1 (none, it is the last grain, or no segment and no live channels).
s32 Handler_NextSegment(VoiceHandler* hp) {
    if ((!hp || hp->type() != kHandlerTypeVoice) &&
        Snd_AssertFailed(kFmtInvalidBlock, kFileBlocksnd, 269, 0x006D869C))
        return 0;
    VoiceHandler& h = gref(hp);
    if (h.segment < 0 && !Handler_HasLiveChannels(hp)) return -1;

    const u32 sound = h.owner;
    const u32 bank = h.bank;
    u32 grains = 0;
    bool checks = true;
    if (sound == 0 || bank == 0) {
        if (Snd_AssertFailed(kFmtInvalidBlock, kFileBlocksnd, 311, 0x006D863C)) return 0;
        if (sound == 0) {
            if (Snd_AssertFailed(kFmtInvalidBlock, kFileBlocksnd, 639, 0x006D85F8)) {
                grains = 0;
                checks = false;
            }
        }
    }
    if (checks) {
        u32 base = 0;
        if (bank != 0 || !Snd_AssertFailed(kFmtInvalidBlock, kFileBlocksnd, 605, 0x006D85B4))
            base = bank + gref<be_u32>(bank + 32);
        grains = base + gref<be_u32>(sound + 8);
    }
    const s32 n = gref<u8>(sound + 4);
    for (s32 i = 0; i < n; ++i) {
        if (gref<u8>(grains + static_cast<u32>(i) * 8) != kGrainMarker) continue;
        return i == n - 1 ? -1 : i;
    }
    return -1;
}

}  // namespace sly4::snd

NATIVE(006E7584, sly4::snd::Handler_HasLiveChannels)
NATIVE(006D7508, sly4::snd::Snd_PitchFromSpeed)
NATIVE(006E6AAC, sly4::snd::Handler_StopOwnedBy)
NATIVE(006D81C0, sly4::snd::Voice_Steal)
NATIVE(006D5BFC, sly4::snd::Bank_GetSoundName)
NATIVE(006D8494, sly4::snd::Handler_NextSegment)
