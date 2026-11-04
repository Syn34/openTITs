// Bank_StartSound: allocates and starts a voice handler for a bank sound
// ("src/common/blocksnd.cpp").
#include "snd.h"

using namespace sly4;
using namespace sly4::snd;

namespace {

constexpr u32 kGainLimits = 0x006D8E20;     // f32 { 1.0, 0.0 }
constexpr u32 kUnknown = 0x00AFE2E4;        // "Unknown"
constexpr u32 kFmtS = 0x00AFE2D4;           // "%s"
constexpr u32 kFmtNoGrains = 0x00AFE2EC;    // "...Attempt to play sound with no grains. (Bank: \"%s\", Sound: \"%s\")\n"
constexpr u32 kSnprintf = 0x007241F4;       // (buf, size, fmt, ...) (name?)
constexpr u32 kSoundName = 0x006D5BFC;      // (bank, s16 id, char* out) (name?)
constexpr u32 kSoundPrepare = 0x006E6AAC;   // (entry, 0, 1) (name?)
constexpr u32 kStealVoice = 0x006D81C0;     // (entry, bool, Handler** out) -> bool (name?)
constexpr u32 kHandlerAlloc = 0x006E4DC8;   // (table, 256, 5, 588, entry, -, angle, s8 volume, arg9 flags; f1 gain) (name?)
constexpr u32 kHandlerSetParams = 0x006E4910;  // (h, params) (name?)
constexpr u32 kHandlerStart = 0x006E51E0;   // (h) (name?)

// The sound table entry (12 bytes): +4 u8 grain count, +6 u16 flags, +8 offset of its grains.
constexpr u32 kEntrySize = 12;

}  // namespace

// handle Bank_StartSound(Bank* bank, s16 id, s8 volume, -, u32 angle, Params* params, u32 user, f32 gain)  (name?)
DECOMP(006D8E28, Bank_StartSound) {
    const u32 bank = arg<u32>(ctx, 0);
    const s32 id = arg<s32>(ctx, 1);
    const u32 volume = arg<u32>(ctx, 2);
    const u32 angle = arg<u32>(ctx, 4);
    const u32 params = arg<u32>(ctx, 5);
    const u64 user = ctx.gpr[9];
    const f64 gain = farg(ctx, 0);

    if ((bank == 0 || id < 0 || !(id < static_cast<s16>(load<u16>(bank + 22))) || gain > load<f32>(kGainLimits) ||
         gain < load<f32>(kGainLimits + 4) || angle > 359 || params == 0) &&
        Snd_AssertFailed(ctx, kFmtInvalidBlock, kFileBlocksnd, 1182, 0x006D93A0))
        return ret(ctx, 0u);

    // Bank_GetSounds, inlined.
    u32 sounds = 0;
    if (bank != 0 || !Snd_AssertFailed(ctx, kFmtInvalidBlock, kFileBlocksnd, 572, 0x006D918C))
        sounds = bank + load<u32>(bank + 28);
    const u32 entry = sounds + static_cast<u32>(id) * kEntrySize;

    if (ref<u8>(entry + 4) == 0) {
        // No grains: log the bank and sound names.
        Stack frame(ctx, 152);
        const u32 bank_name = frame.addr(), sound_name = bank_name + 16;
        for (u32 k = 0; k < 8; ++k) ref<u8>(bank_name + k) = ref<u8>(kUnknown + k);
        ref<u8>(bank_name + 8) = 0;
        for (u32 k = 0; k < 8; ++k) ref<u8>(sound_name + k) = ref<u8>(kUnknown + k);
        call(ctx, fn::memset, {sound_name + 8, 0, 120});
        if (load<u32>(bank + 8) & 0x100) {
            u32 name = 0;
            if (bank != 0 || !Snd_AssertFailed(ctx, kFmtInvalidBlock, kFileBlocksnd, 772, 0x006D91A4))
                name = bank + load<u32>(bank + 56);
            call(ctx, kSnprintf, {bank_name, 8, kFmtS, name});
            ref<u8>(bank_name + 8) = 0;
        }
        call(ctx, kSoundName, {bank, static_cast<u64>(static_cast<s64>(static_cast<s16>(id))), sound_name});
        call(ctx, sfn::Snd_Log, {kFmtNoGrains, kFileBlocksnd, 1203, bank_name, sound_name});
        return ret(ctx, 0u);
    }

    call(ctx, sfn::Snd_Lock);
    if (load<u32>(bank + 8) & 0x400) {
        call(ctx, sfn::Snd_Unlock);
        return ret(ctx, 0u);
    }
    u32 flags = load<u16>(entry + 6);
    if (flags & 2) {
        call(ctx, kSoundPrepare, {entry, 0, 1});
        flags = load<u16>(entry + 6);
    }
    const u32 exclusive = (flags & 4) ? 0x10000u : 0u;
    const s64 vol = static_cast<s8>(volume & 0xFF);
    const u32 replace = (load<u32>(params + 4) & 0x80000000u) == 0 ? 1u : 0u;
    if (entry == 0) {
        if (Snd_AssertFailed(ctx, kFmtInvalidBlock, kFileBlocksnd, 941, 0x006D9324)) {
            call(ctx, sfn::Snd_Unlock);
            return ret(ctx, 0u);
        }
        flags = load<u16>(entry + 6);
    }
    if (flags & 8) {
        Stack frame(ctx, 8);
        if (static_cast<u32>(call(ctx, kStealVoice, {entry, replace, frame.addr()})) == 0) {
            call(ctx, sfn::Snd_Unlock);
            return ret(ctx, 0u);
        }
        if (const u32 old = load<u32>(frame.addr())) call(ctx, sfn::Handler_Stop, {old, 1, 0, 1});
    }

    ctx.fpr[1] = gain;
    const u32 h = static_cast<u32>(call(ctx, kHandlerAlloc,
                                        {kVoiceHandlers, 256, kHandlerTypeVoice, kVoiceHandlerSize, entry,
                                         0 /* r8 unused */, angle, static_cast<u64>(vol),
                                         exclusive /* 9th argument, on the stack */}));
    if (h != 0) store<u16>(h + 574, 0);
    if (h == 0) {
        call(ctx, sfn::Snd_Unlock);
        return ret(ctx, 0u);
    }
    call(ctx, kHandlerSetParams, {h, params});

    // The grain table: bank + bank+32 + entry+8 (with the original's checks).
    u32 grains = 0;
    if (entry != 0 || !Snd_AssertFailed(ctx, kFmtInvalidBlock, kFileBlocksnd, 639, 0x006D92E0)) {
        u32 base = 0;
        if (bank != 0 || !Snd_AssertFailed(ctx, kFmtInvalidBlock, kFileBlocksnd, 605, 0x006D929C))
            base = bank + load<u32>(bank + 32);
        grains = base + load<u32>(entry + 8);
    }
    store<u32>(h + 576, bank);
    store<u32>(h + 580, entry);
    const u32 first = load<u32>(grains + 4);
    store<u16>(h + 566, 0);
    store<u16>(h + 568, 0);
    store<u16>(h + 570, 0);
    store<u16>(h + 572, 0);
    store<u16>(h + 564, static_cast<u16>(first + 1));
    store<u32>(h + 584, 0);
    const u32 eflags = load<u16>(entry + 6);
    if (eflags & 0x2000) store<u32>(h + kHFlags, load<u32>(h + kHFlags) | 0x80000);
    if (eflags & 0x4000) store<u32>(h + kHFlags, load<u32>(h + kHFlags) | 0x100000);
    store<u32>(h + 540, static_cast<u32>(user));
    call(ctx, kHandlerStart, {h});
    const u32 handle = load<u32>(h);
    call(ctx, sfn::Snd_Unlock);
    ret(ctx, handle);
}
