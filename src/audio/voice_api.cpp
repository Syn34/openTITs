// Public sound API functions ("src/common/api.cpp").
#include "voice_layer.h"

using namespace sly4;
using namespace sly4::snd;

namespace {
constexpr u32 kBankMagic = 0x6B6C4253;  // "klBS"
constexpr u32 kPlayParamsSize = 220;
constexpr u32 kBankSoundCount = 22;     // u16
constexpr u32 kSoundEntrySize = 12;     // { s8 volume, s8 ?, s16 pitch, ... }

// Pauses or resumes every active handler whose group bit is in `bits`.
void Snd_ForGroups(PPUContext& ctx, u32 bits, u32 line, u32 trap_pc, u32 groups_fn, u32 handler_fn) {
    if (bits == 0 && Snd_AssertFailed(ctx, kFmtInvalid, kFileApi, line, trap_pc)) return;
    call(ctx, sfn::Snd_Lock, {bits});
    call(ctx, groups_fn, {bits});
    for (u32 h = static_cast<u32>(call(ctx, sfn::Handler_First)); h != 0; h = load<u32>(h + kHNextActive)) {
        const u32 shift = ref<u8>(h + kHGroup) & 63;  // slw
        const u32 bit = shift < 32 ? 1u << shift : 0u;
        if (bits & bit) call(ctx, handler_fn, {h, 1, 4});
    }
    call(ctx, sfn::Snd_Unlock);
}

}  // namespace

// handle Voice_SetParam(handle, u32 which, u8 value)
// Sets parameter 1-4 of the handle's handler. Returns the handle, or 0.
DECOMP(006D50F0, Voice_SetParam) {
    const u64 handle = ctx.gpr[3];
    const u32 which = arg<u32>(ctx, 1);
    const u8 value = arg<u8>(ctx, 2);
    if ((static_cast<s32>(which) < 1 || static_cast<s32>(which) > 4) &&
        Snd_AssertFailed(ctx, kFmtInvalid, kFileApi, 1304, 0x006D51C0))
        return ret(ctx, 0u);
    call(ctx, sfn::Snd_Lock, {handle});
    u64 result = 0;
    const u32 h = static_cast<u32>(call(ctx, sfn::Snd_Lookup, {handle}));
    if (h != 0) {
        ref<u8>(h + which + kHParam) = value;
        result = handle;
    }
    call(ctx, sfn::Snd_Unlock);
    ctx.gpr[3] = result;
}

// handle Voice_Play(Bank* bank, s16 id, PlayParams* params, u32 arg)
// Starts sound `id` of the bank with its volume and pitch from the bank's
// sound table. 0 on a bad bank or id.
DECOMP(006D5EE0, Voice_Play) {
    const u32 bank = arg<u32>(ctx, 0);
    const s32 id = static_cast<s16>(arg<u32>(ctx, 1) & 0xFFFF);
    const u32 params = arg<u32>(ctx, 2);
    const u64 extra = ctx.gpr[6];
    if ((bank == 0 || id < 0 || params == 0) &&
        Snd_AssertFailed(ctx, kFmtInvalid, kFileApi, 3912, 0x006D60A4))
        return ret(ctx, 0u);
    if (load<s32>(params) != static_cast<s32>(kPlayParamsSize)) Snd_Log(ctx, kFmtSizeMember, kFileApi, 3918);

    call(ctx, sfn::Snd_Lock);
    if (load<u32>(bank) != kBankMagic) {
        Snd_Log(ctx, kFmtBankFormat, kFileApi, 3926);
        call(ctx, sfn::Snd_Unlock);
        return ret(ctx, 0u);
    }
    if (!(id < static_cast<s16>(load<u16>(bank + kBankSoundCount)))) {
        call(ctx, sfn::Snd_Unlock);
        return ret(ctx, 0u);
    }
    const u32 entry = static_cast<u32>(call(ctx, sfn::Bank_GetSounds, {bank})) + static_cast<u32>(id) * kSoundEntrySize;
    const f64 gain = callf(ctx, sfn::Snd_DbToGain, {static_cast<u64>(static_cast<s64>(static_cast<s8>(ref<u8>(entry)))), 0}, {});
    const u64 pitch = call(ctx, sfn::Snd_CentsToPitch, {static_cast<u64>(static_cast<s64>(static_cast<s16>(load<u16>(entry + 2))))});
    const s64 b1 = static_cast<s8>(ref<u8>(entry + 1));
    // r6 is not set by the original here (Bank_StartSound doesn't read it).
    ctx.fpr[1] = gain;
    const u64 voice = call(ctx, sfn::Bank_StartSound,
                           {bank, static_cast<u64>(static_cast<s64>(id)), static_cast<u64>(b1), 0, pitch, params, extra});
    call(ctx, sfn::Snd_Unlock);
    ctx.gpr[3] = voice;
}

// void Snd_PauseGroups(u32 bits)  (name?)   called by 0x0044BA6C
DECOMP(006D530C, Snd_PauseGroups) {
    Snd_ForGroups(ctx, arg<u32>(ctx, 0), 1813, 0x006D53E8, sfn::Groups_Pause, sfn::Handler_Pause);
}

// void Snd_ResumeGroups(u32 bits)  (name?)
DECOMP(006D53F0, Snd_ResumeGroups) {
    Snd_ForGroups(ctx, arg<u32>(ctx, 0), 1885, 0x006D54CC, sfn::Groups_Resume, sfn::Handler_Resume);
}
