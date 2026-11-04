// The sound library's handle layer ("src/common/api.cpp", "src/common/sndhand.cpp").
// Addresses are guest addresses in the BCES01284 v01.02 EBOOT; "?" marks a guess.
#pragma once

#include "../sly4.h"

namespace sly4 {
namespace snd {

namespace sfn {
constexpr u32 Snd_Lock = 0x006EFD10, Snd_Unlock = 0x006EFD34;
constexpr u32 Snd_LockImpl = 0x006F6528, Snd_UnlockImpl = 0x006F6558;  // (mutex from 0x011D8A94)
constexpr u32 Snd_Lookup = 0x006E5B6C;       // (handle) -> Handler* or 0
constexpr u32 Handler_Stop = 0x006E620C;     // (Handler*, recurse, force, flag)
constexpr u32 Handler_CountActive = 0x006E6718;
constexpr u32 Snd_Log = 0x006E45F8;          // (fmt, file, line) printf-style (name?)
constexpr u32 Bank_GetSounds = 0x006D8098;   // (Bank*) -> SoundEntry[12] (name?)
constexpr u32 Snd_DbToGain = 0x006DBA20;     // (s8, 0) -> f32 (name?)
constexpr u32 Snd_CentsToPitch = 0x006DBAC8; // (s16) -> u32 (name?)
constexpr u32 Bank_StartSound = 0x006D8E28;  // (bank, id, s8, -, u32, params, u32, f1) -> handle (name?)
constexpr u32 Voice_Stop5 = 0x006D9584;      // (Handler*, force, flag) -> stopped (type-5 handlers) (name?)
constexpr u32 Handler_Release = 0x006E9780;  // (Handler*) (name?)
constexpr u32 Handler_Detach = 0x006EA104, Handler_Finish = 0x006E322C;  // (Handler*) (name?)
constexpr u32 Handler_Free = 0x006E54B8;     // (Handler*, bool parent_only?) (name?)
constexpr u32 Groups_Pause = 0x006EA9E0, Groups_Resume = 0x006EAAA0;  // (u32 bits) (name?)
constexpr u32 Handler_First = 0x006E6AA0;    // () -> first active Handler* (name?)
constexpr u32 Handler_Pause = 0x006E5F4C, Handler_Resume = 0x006E60A4;  // (Handler*, 1, 4) (name?)
}  // namespace sfn

// Handler tables.
constexpr u32 kVoiceHandlers = 0x011B1498;  // type 5: 256 handlers of 588 bytes
constexpr u32 kVoiceHandlerSize = 588;
constexpr u32 kHandlerTypes = 0x00F8E540;   // u32 TypeInfo*[32]: +8 count, +12 size, +16 base, +32 stop OPD
constexpr u32 kHandlerTypeVoice = 5;

// Handler fields.
constexpr u32 kHFlags = 4;         // u32: 0x80 stopping, 0x20 ..., 0xE ...
constexpr u32 kHParam = 355;       // u8[1..4] parameters
constexpr u32 kHParent = 552, kHFirstChild = 556, kHNextSibling = 560;
constexpr u32 kHNextActive = 548;
constexpr u32 kHGroup = 16;        // u8 group bit index
constexpr u32 kHType4Active = 581; // u8

// Assertions: log, then either give up (kSndAssertReturns) or trap (kSndAssertTraps).
constexpr u32 kSndAssertReturns = 0x00F8DAD8;  // u8
constexpr u32 kSndAssertTraps = 0x011AEF4C;    // u8
constexpr u32 kFmtInvalid = 0x00AFDC68;        // "%s(%d) : Invalid parameter.\n"
constexpr u32 kFileApi = 0x00AFDC88;           // "src/common/api.cpp"
constexpr u32 kFmtSizeMember = 0x00AFE030;     // "%s(%d) : update to set size member.\n"
constexpr u32 kFmtBankFormat = 0x00AFE058;     // "%s(%d) : Unsupported bank format.\n"
constexpr u32 kFileSndhand = 0x00AFFA30;       // "src/common/sndhand.cpp"
constexpr u32 kFmtInactive = 0x00AFFC18;       // "%s(%d) : Attempt to stop an inactive handler.\n"
constexpr u32 kFmtChildren = 0x00AFFC48;       // "%s(%d) : Children stopped the parent. ..."

inline void Snd_Log(PPUContext& ctx, u32 fmt, u32 file, u32 line) { call(ctx, sfn::Snd_Log, {fmt, file, line}); }

// Logs an invalid-parameter assertion. Returns true when the caller should
// give up; otherwise traps (twi) if traps are enabled and the caller goes on.
inline bool Snd_AssertFailed(PPUContext& ctx, u32 fmt, u32 file, u32 line, u32 trap_pc) {
    Snd_Log(ctx, fmt, file, line);
    if (ref<u8>(kSndAssertReturns) != 0) return true;
    if (ref<u8>(kSndAssertTraps) != 0) trap(ctx, trap_pc);
    return false;
}

// The handle -> handler lookup the library inlines (caller holds the lock).
// Handle: bit 31 live, bits 24..28 type, bits 16..23 index.
inline u32 Snd_Find(u32 handle) {
    const s32 index = static_cast<s32>((handle >> 16) & 0xFF);
    const u32 type = (handle >> 24) & 0x1F;
    u32 p;
    if (type == kHandlerTypeVoice) {
        if (!(index < 256)) return 0;
        p = kVoiceHandlers + static_cast<u32>(index) * kVoiceHandlerSize;
    } else {
        const u32 info = load<u32>(kHandlerTypes + type * 4);
        if (info == 0) return 0;
        if (!(index < load<s32>(info + 8))) return 0;
        p = load<u32>(info + 16) + load<u32>(info + 12) * static_cast<u32>(index);
    }
    if (p != 0 && load<u32>(p) != handle) p = 0;
    return p;
}

}  // namespace snd
}  // namespace sly4
