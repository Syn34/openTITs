// Public sound API functions ("src/common/api.cpp").
#include "snd.h"

namespace sly4::snd {
using namespace opentit::native;

namespace {
constexpr u32 kBankMagic = 0x6B6C4253;  // "klBS"
constexpr u32 kPlayParamsSize = 220;

// A bank's sound table entry (12 bytes).
struct SoundEntry {
    s8 volume;     // +0
    s8 b1;         // +1
    be_s16 pitch;  // +2
    pad<8> _4;
};
static_assert(sizeof(SoundEntry) == 12);

// Pauses or resumes every active handler whose group bit is in `bits`.
void Snd_ForGroups(u32 bits, u32 line, u32 trap_pc, void (*groups_fn)(u32), void (*handler_fn)(Handler*, u32, u64)) {
    if (bits == 0 && Snd_AssertFailed(kFmtInvalid, kFileApi, line, trap_pc)) return;
    Snd_Lock();
    groups_fn(bits);
    for (Handler* h = Handler_First(); h; h = h->next_active) {
        const u32 shift = h->group & 63;  // slw
        const u32 bit = shift < 32 ? 1u << shift : 0u;
        if (bits & bit) handler_fn(h, 1, 4);
    }
    Snd_Unlock();
}

}  // namespace

// handle Voice_SetParam(handle, u32 which, u8 value)
// Sets parameter 1-4 of the handle's handler. Returns the handle, or 0.
u64 Voice_SetParam(u64 handle, u32 which, u8 value) {
    if ((static_cast<s32>(which) < 1 || static_cast<s32>(which) > 4) &&
        Snd_AssertFailed(kFmtInvalid, kFileApi, 1304, 0x006D51C0))
        return 0;
    Snd_Lock();
    u64 result = 0;
    if (Handler* h = Snd_Lookup(static_cast<u32>(handle))) {
        gref<u8>(guest(h) + which + kHParam) = value;
        result = handle;
    }
    Snd_Unlock();
    return result;
}

// handle Voice_Play(Bank* bank, s16 id, PlayParams* params, u32 arg)
// Starts sound `id` of the bank with its volume and pitch from the bank's
// sound table. 0 on a bad bank or id.
u64 Voice_Play(u32 bank, s16 id, u32 params, u64 extra) {
    if ((bank == 0 || id < 0 || params == 0) && Snd_AssertFailed(kFmtInvalid, kFileApi, 3912, 0x006D60A4)) return 0;
    if (gref<be_s32>(params) != static_cast<s32>(kPlayParamsSize)) Snd_LogAt(kFmtSizeMember, kFileApi, 3918);

    Snd_Lock();
    if (gref<be_u32>(bank) != kBankMagic) {
        Snd_LogAt(kFmtBankFormat, kFileApi, 3926);
        Snd_Unlock();
        return 0;
    }
    if (!(id < gref<be_s16>(bank + 22))) {
        Snd_Unlock();
        return 0;
    }
    const SoundEntry& entry = gref<SoundEntry>(Bank_GetSounds(bank) + static_cast<u32>(id) * sizeof(SoundEntry));
    const f32 volume = Snd_VolumeCurve(entry.volume, 0);
    const u32 angle = Angle_Wrap360(entry.pitch);
    const u32 voice = Bank_StartSound(bank, id, static_cast<u32>(static_cast<s32>(entry.b1)), 0, angle, params, extra, volume);
    Snd_Unlock();
    return voice;
}

// void Snd_PauseGroups(u32 bits)  (name?)   called by 0x0044BA6C
void Snd_PauseGroups(u32 bits) { Snd_ForGroups(bits, 1813, 0x006D53E8, Groups_Pause, Handler_Pause); }

// void Snd_ResumeGroups(u32 bits)  (name?)
void Snd_ResumeGroups(u32 bits) { Snd_ForGroups(bits, 1885, 0x006D54CC, Groups_Resume, Handler_Resume); }

}  // namespace sly4::snd

NATIVE(006D50F0, sly4::snd::Voice_SetParam)
NATIVE(006D5EE0, sly4::snd::Voice_Play)
NATIVE(006D530C, sly4::snd::Snd_PauseGroups)
NATIVE(006D53F0, sly4::snd::Snd_ResumeGroups)
