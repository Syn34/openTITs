// Bank_StartSound: allocates and starts a voice handler for a bank sound
// ("src/common/blocksnd.cpp").
#include "snd.h"

namespace sly4::snd {
using namespace opentit::native;

namespace {

constexpr u32 kGainLimits = 0x006D8E20;     // f32 { 1.0, 0.0 }
constexpr u32 kUnknown = 0x00AFE2E4;        // "Unknown"
constexpr u32 kFmtS = 0x00AFE2D4;           // "%s"
constexpr u32 kFmtNoGrains = 0x00AFE2EC;    // "...Attempt to play sound with no grains. (Bank: \"%s\", Sound: \"%s\")\n"
constexpr u32 kSnprintf = 0x007241F4;       // (buf, size, fmt, ...) (name?)

// The sound table entry (12 bytes): +4 u8 grain count, +6 u16 flags, +8 offset of its grains.
struct SoundEntry {
    pad<4> _0;
    u8 grains;            // +4
    pad<1> _5;
    be_u16 flags;         // +6: 2 prepare, 4 exclusive, 8 limited, 0x2000 / 0x4000 handler flags
    be_u32 grain_offset;  // +8
};
static_assert(sizeof(SoundEntry) == 12);

// The bank header ("klBS").
struct Bank {
    be_u32 magic;          // +0
    pad<4> _4;
    be_u32 flags;          // +8: 0x100 has names, 0x400 disabled
    pad<10> _12;
    be_s16 count;          // +22
    pad<4> _24;
    be_u32 sounds;         // +28 offset of the sound table
    be_u32 grains;         // +32 offset of the grain tables
    pad<20> _36;
    be_u32 name;           // +56 offset of the bank name
};
static_assert(offsetof(Bank, name) == 56);

}  // namespace

// handle Bank_StartSound(Bank* bank, s16 id, s8 volume, -, u32 angle, Params* params, u32 user, f32 gain)  (name?)
u32 Bank_StartSound(u32 bank, s32 id, u32 volume, u32, u32 angle, u32 params, u64 user, f64 gain) {
    Bank& b = gref<Bank>(bank);
    const be_f32* limits = host<be_f32>(kGainLimits);
    if ((bank == 0 || id < 0 || !(id < b.count) || gain > limits[0] || gain < limits[1] || angle > 359 ||
         params == 0) &&
        Snd_AssertFailed(kFmtInvalidBlock, kFileBlocksnd, 1182, 0x006D93A0))
        return 0;

    // Bank_GetSounds, inlined.
    u32 sounds = 0;
    if (bank != 0 || !Snd_AssertFailed(kFmtInvalidBlock, kFileBlocksnd, 572, 0x006D918C)) sounds = bank + b.sounds;
    const u32 entry = sounds + static_cast<u32>(id) * sizeof(SoundEntry);
    SoundEntry& e = gref<SoundEntry>(entry);

    if (e.grains == 0) {
        // No grains: log the bank and sound names.
        GuestFrame frame(152);
        u8* bank_name = frame.as<u8>();
        u8* sound_name = bank_name + 16;
        const u8* unknown = host<u8>(kUnknown);
        for (u32 k = 0; k < 8; ++k) bank_name[k] = unknown[k];
        bank_name[8] = 0;
        for (u32 k = 0; k < 8; ++k) sound_name[k] = unknown[k];
        call(fn::memset, sound_name + 8, 0u, 120u);
        if (b.flags & 0x100) {
            u32 name = 0;
            if (bank != 0 || !Snd_AssertFailed(kFmtInvalidBlock, kFileBlocksnd, 772, 0x006D91A4)) name = bank + b.name;
            call(kSnprintf, bank_name, 8u, kFmtS, name);
            bank_name[8] = 0;
        }
        Bank_GetSoundName(bank, static_cast<s16>(id), guest(sound_name));
        Snd_Log(kFmtNoGrains, kFileBlocksnd, 1203, guest(bank_name), guest(sound_name), 0, 0, 0);
        return 0;
    }

    Snd_Lock();
    if (b.flags & 0x400) {
        Snd_Unlock();
        return 0;
    }
    u32 flags = e.flags;
    if (flags & 2) {
        Handler_StopOwnedBy(entry, 0, 1);
        flags = e.flags;
    }
    const u32 exclusive = (flags & 4) ? 0x10000u : 0u;
    const s8 vol = static_cast<s8>(volume & 0xFF);
    const u32 replace = (gref<be_u32>(params + 4) & 0x80000000u) == 0 ? 1u : 0u;
    if (entry == 0) {
        if (Snd_AssertFailed(kFmtInvalidBlock, kFileBlocksnd, 941, 0x006D9324)) {
            Snd_Unlock();
            return 0;
        }
        flags = e.flags;
    }
    if (flags & 8) {
        GuestFrame frame(8);
        be_u32* old = frame.as<be_u32>();
        if (!Voice_Steal(entry, replace, old)) {
            Snd_Unlock();
            return 0;
        }
        if (Handler* o = host<Handler>(*old)) Handler_Stop(o, 1, 0, 1);
    }

    VoiceHandler* h = static_cast<VoiceHandler*>(
        Handler_Alloc(kVoiceHandlers, 256, kHandlerTypeVoice, kVoiceHandlerSize, entry, 0, angle, vol, exclusive, static_cast<f32>(gain)));
    if (h) h->loop = 0;
    if (!h) {
        Snd_Unlock();
        return 0;
    }
    Handler_SetParams(h, host<u8>(params));

    // The grain table: bank + bank+32 + entry+8 (with the original's checks).
    u32 grains = 0;
    if (entry != 0 || !Snd_AssertFailed(kFmtInvalidBlock, kFileBlocksnd, 639, 0x006D92E0)) {
        u32 base = 0;
        if (bank != 0 || !Snd_AssertFailed(kFmtInvalidBlock, kFileBlocksnd, 605, 0x006D929C)) base = bank + b.grains;
        grains = base + e.grain_offset;
    }
    h->bank = bank;
    h->entry = entry;
    const u32 first = gref<be_u32>(grains + 4);
    h->segment = 0;
    h->w568 = 0;
    h->w570 = 0;
    h->w572 = 0;
    h->seg_end = static_cast<u16>(first + 1);
    h->w584 = 0u;
    const u32 eflags = e.flags;
    if (eflags & 0x2000) h->flags = h->flags | 0x80000;
    if (eflags & 0x4000) h->flags = h->flags | 0x100000;
    h->user540 = static_cast<u32>(user);
    Handler_Start(h);
    const u32 handle = h->handle;
    Snd_Unlock();
    return handle;
}

}  // namespace sly4::snd

NATIVE(006D8E28, sly4::snd::Bank_StartSound)
