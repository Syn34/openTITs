// More of the sound library: handler bookkeeping, logging, the bank and the
// helpers it uses. Builds on voice_layer.h. "?" marks a guess.
#pragma once

#include <bit>
#include <cmath>

#include "voice_layer.h"

namespace sly4 {
namespace snd {

namespace sfn2 {
constexpr u32 Vsnprintf = 0x0071F36C;        // (buf, size, fmt, va_list) (name?)
constexpr u32 Handler_OnPause = 0x006E9B40;  // (Handler*) (name?)
constexpr u32 Handler_OnResume = 0x006E9E0C; // (Handler*) (name?)
constexpr u32 Matrix_NearlyEqual = 0x005545B4;  // (const f32[16], const f32[16], f32 eps) -> bool (name?)
constexpr u32 Qsort = 0x0071EC28;
}  // namespace sfn2

// Logging.
constexpr u32 kLogDisabled = 0x011D73C0;  // u32: nonzero = Snd_Log prints nothing
constexpr u32 kLogBuffer = 0x011D71C0;    // char[512]
constexpr u32 kLogCallback = 0x00F8E53C;  // u32 OPD called with the formatted text

// Active handler list and the group pause flags.
constexpr u32 kFirstActive = 0x00F8E560;  // u32 Handler*
constexpr u32 kGroupFlags = 0x011D7DE0;   // u32[32]: bit 1 = paused
constexpr u32 kFmtInvalidValloc = 0x00AFFD34, kFileValloc = 0x00AFFD54;  // "src/common/valloc.cpp"
constexpr u32 kFmtInvalidBlock = 0x00AFE294, kFileBlocksnd = 0x00AFE2B4; // "src/common/blocksnd.cpp"

// Sub-records ("tracks"?) of a handler: 4 of 48 bytes from +92, on a list
// at kTrackList (linked by +44) with a count at kTrackCount.
constexpr u32 kTrackList = 0x00F8E30C, kTrackCount = 0x011D71B0;
constexpr u32 kTrackBase = 92, kTrackSize = 48, kTrackCountPerHandler = 4, kTrackNext = 44;

constexpr u32 kHTypeInfoPause = 36;  // TypeInfo +36: (Handler*, bool pause, opd) OPD
constexpr u32 kTypeCountPausable = 8;

// Single-precision PPU ops.
inline f32 fadds(f64 a, f64 b) { return static_cast<f32>(a + b); }
inline f32 fsubs(f64 a, f64 b) { return static_cast<f32>(a - b); }
inline f32 fmuls(f64 a, f64 c) { return static_cast<f32>(a * c); }
inline f32 fdivs(f64 a, f64 b) { return static_cast<f32>(a / b); }

}  // namespace snd
}  // namespace sly4

// ---------------------------------------------------------------- native structures and helpers
#include <cstddef>

#include "opentit/native.h"

namespace sly4::snd {

struct Handler;

// One of a handler's four 48-byte sub-records ("tracks"?), from +92.
struct Track {
    native::be_u16 w0;          // +0
    u8 b2;                      // +2
    u8 _3;
    u8 b4;                      // +4
    u8 flags5;                  // +5: bit 0 = on the track list (LFO active)
    native::pad<6> _6;
    native::be_u32 w12;         // +12
    native::pad<24> _16;
    native::gp<Handler> owner;  // +40
    native::gp<Track> next;     // +44: kTrackList link
};
static_assert(sizeof(Track) == kTrackSize);

// A handler (588 bytes for type 5; other types are at least 564).
// Word +0: bit 31 live, bits 24..28 type, bits 16..23 index, low 16 bits a serial.
struct Handler {
    native::be_u32 handle;       // +0
    native::be_u32 flags;        // +4: 0x80 stopping, 0x20 ..., 0xE paused, 0x20000 free callback, 0x40000 at head
    native::be_u32 user;         // +8
    native::be_u32 tick;         // +12 Snd_GetTick when allocated
    u8 group;                    // +16 (s8)
    native::pad<3> _17;
    native::be_u32 channels[16]; // +20: bit per owned channel (see channel_word)
    native::be_u32 owner;        // +84: 0 = free; the sound entry for type 5
    native::be_u32 family;       // +88
    Track tracks[4];             // +92
    native::be_f32 volume;       // +284
    native::be_f32 gain;         // +288
    native::be_f32 f292;         // +292
    native::be_f32 f296;         // +296
    native::be_u32 angle;        // +300 (0..359)
    native::be_u32 user304;      // +304
    native::be_u32 w308, w312;   // +308
    native::be_s32 s316;         // +316 (0..360)
    native::be_u32 w320;         // +320
    native::be_s32 pitch;        // +324 (-7680..7680)
    native::be_u32 w328, w332, w336;  // +328
    native::be_f32 f340;         // +340 (-1..1)
    native::be_u32 w344, w348, w352;  // +344
    u8 params[4];                // +356: parameters 1-4
    native::pad<180> effects;    // +360 (Effects_Set)
    native::be_u32 user540;      // +540
    native::gp<Handler> prev_active;   // +544
    native::gp<Handler> next_active;   // +548
    native::gp<Handler> parent;        // +552
    native::gp<Handler> first_child;   // +556
    native::gp<Handler> next_sibling;  // +560

    u32 type() const { return (handle >> 24) & 0x1F; }
    bool live() const { return (handle & 0x80000000u) != 0; }
    // Word w of the channel mask (any w: the mask may run past +84 with many channels).
    native::be_u32& channel_word(s32 w) { return gref<native::be_u32>(native::guest(this) + 20 + static_cast<u32>(w) * 4); }
};
static_assert(offsetof(Handler, owner) == 84 && offsetof(Handler, tracks) == kTrackBase && offsetof(Handler, volume) == 284);
static_assert(offsetof(Handler, params) == 356 && offsetof(Handler, prev_active) == 544 && sizeof(Handler) == 564);

// Type 5 (a bank sound's voice).
struct VoiceHandler : Handler {
    native::be_u16 seg_end;      // +564
    native::be_s16 segment;      // +566
    native::be_u16 w568, w570, w572;  // +568
    native::be_u16 loop;         // +574: bit 0 = loops
    native::be_u32 bank;         // +576
    native::be_u32 entry;        // +580 SoundEntry*
    native::be_u32 w584;         // +584
};
static_assert(sizeof(VoiceHandler) == kVoiceHandlerSize);

// kHandlerTypes[type].
struct HandlerType {
    native::pad<8> _0;
    native::be_s32 count;   // +8
    native::be_u32 size;    // +12
    native::be_u32 base;    // +16
    native::pad<12> _20;
    native::be_u32 stop;    // +32 OPD (Handler*, force, opd) -> stopped
    native::be_u32 pause;   // +36 OPD (Handler*, bool pause, opd)
};
static_assert(offsetof(HandlerType, pause) == kHTypeInfoPause);

// A voice channel (104 bytes) of the table at *kChannels.
struct Channel {
    native::gp<Channel> next;   // +0 pending list
    native::pad<4> _4;
    u8 b8, b9;                  // +8
    u8 group;                   // +10 (s8)
    u8 type;                    // +11 (s8) data type 0..4
    native::be_u32 w12;         // +12
    native::be_u32 active;      // +16
    native::be_u32 w20;         // +20
    native::be_u32 w24;         // +24
    native::be_s32 w28;         // +28
    native::be_u32 w32, w36;    // +32
    native::be_u32 owner;       // +40 Handler*
    native::be_u32 callback;    // +44 OPD (Channel*, Handler*, u32 event)
    native::be_u32 w48;         // +48
    native::pad<4> _52;
    native::be_u32 w56;         // +56
    native::pad<4> _60;
    native::be_u32 w64;         // +64
    native::pad<4> _68;
    native::be_u32 w72;         // +72
    native::pad<4> _76;
    native::be_f32 f80, f84, f88;  // +80
    native::be_u32 w92;         // +92
    native::be_u32 hw_voice;    // +96
    native::be_u32 w100;        // +100
};
static_assert(sizeof(Channel) == 104);

constexpr u32 kChannelCountG = 0x011D86A4;  // s32
constexpr u32 kChannelsG = 0x00F8E880;      // Channel*
inline s32 Snd_ChannelCount() { return native::global<native::be_s32>(kChannelCountG); }
inline s32 Snd_ChannelWords(s32 n) { return (n + 31) / 32; }  // srawi + addze
inline u32 Snd_ChannelTable() { return native::global<native::be_u32>(kChannelsG); }

// ---------------------------------------------------------------- native functions
// voice_core.cpp
void Snd_Lock();
void Snd_Unlock();
Handler* Snd_Lookup(u32 handle);
u32 Handler_CountActive(Handler* h);
void Handler_Stop(Handler* h, u32 recurse, u32 force, u32 flag);
// snd_handlers.cpp
void Snd_Log(u32 fmt, u64 a1, u64 a2, u64 a3, u64 a4, u64 a5, u64 a6, u64 a7);
Handler* Handler_First();
void Handler_UnlinkTracks(Handler* h);
void Groups_Pause(u32 bits);
void Groups_Resume(u32 bits);
void Handler_Pause(Handler* h, u32 recurse, u64 reason);
void Handler_Resume(Handler* h, u32 recurse, u64 reason);
// snd_free.cpp
void Handler_Free(Handler* h, u32 with_children);
// snd_channels.cpp
void Handler_Release(Handler* h);
void Handler_Detach(Handler* h);
u32 Voice_StopHandler(VoiceHandler* h, u32 force);
// lib_channels.cpp
void Channel_Free(s32 ch, u8 keep_voice);
void Handler_OnPause(Handler* h);
void Handler_OnResume(Handler* h);
// lib_handlers.cpp
void Snd_SendEvent(u64 cause, u64 a, u64 b, u64 c, u64 d);
void Family_Release(u32 f);
Handler* Handler_Alloc(u32 table, s32 count, u32 type, s32 size, u32 owner, u32 unused, u32 user, s8 group, u32 flags, f32 volume);
void Handler_SetParams(Handler* h, const u8* params);
void Handler_Start(Handler* h);
// lib_voice.cpp
bool Handler_HasLiveChannels(Handler* h);
s32 Snd_PitchFromSpeed(f32 speed);
void Handler_StopOwnedBy(u32 owner, u64 force, u64 flag);
bool Voice_Steal(u32 entry, u32 by_sound, native::be_u32* out);
bool Bank_GetSoundName(u32 bank, s16 id, u32 out);
s32 Handler_NextSegment(VoiceHandler* h);
// more_snd.cpp
s32 Snd_GetLastError();
u32 Snd_GetTick();
void Channel_ReleaseVoice(u32 v);
void Channel_Reset(s32 ch);
void Effects_Set(u32 dst, u32 src);
// bank_util.cpp
u32 Bank_GetSounds(u32 bank);
f32 Snd_VolumeCurve(s8 volume, s8 curve);
u32 Angle_Wrap360(s16 a);
s32 Int_Abs(s32 x);
u32 Map_Lookup(u32 map, const native::be_u32* key);
// bank_start.cpp
u32 Bank_StartSound(u32 bank, s32 id, u32 volume, u32 unused, u32 angle, u32 params, u64 user, f64 gain);
// voice_api.cpp
u64 Voice_SetParam(u64 handle, u32 which, u8 value);
u64 Voice_Play(u32 bank, s16 id, u32 params, u64 extra);
void Snd_PauseGroups(u32 bits);
void Snd_ResumeGroups(u32 bits);
// more_math.cpp
f32 Math_FLog(f32 x, s32 base);
s32 Math_FDunscale(native::be_u16* exp, native::be_u16* x);

// Logs an invalid-parameter assertion. True when the caller should give up;
// otherwise traps (twi) if traps are enabled and the caller goes on.
inline bool Snd_AssertFailed(u32 fmt, u32 file, u32 line, u32 trap_pc) {
    Snd_Log(fmt, file, line, 0, 0, 0, 0, 0);
    if (native::global<u8>(kSndAssertReturns) != 0) return true;
    if (native::global<u8>(kSndAssertTraps) != 0) trap(native::ctx(), trap_pc);
    return false;
}
inline void Snd_LogAt(u32 fmt, u32 file, u32 line) { Snd_Log(fmt, file, line, 0, 0, 0, 0, 0); }

// The handle -> handler lookup the library inlines (caller holds the lock).
// Handle: bit 31 live, bits 24..28 type, bits 16..23 index.
inline Handler* Snd_Find(u32 handle) {
    using namespace native;
    const s32 index = static_cast<s32>((handle >> 16) & 0xFF);
    const u32 type = (handle >> 24) & 0x1F;
    u32 p;
    if (type == kHandlerTypeVoice) {
        if (!(index < 256)) return nullptr;
        p = kVoiceHandlers + static_cast<u32>(index) * kVoiceHandlerSize;
    } else {
        const HandlerType* info = global<gp<HandlerType>>(kHandlerTypes + type * 4);
        if (!info) return nullptr;
        if (!(index < info->count)) return nullptr;
        p = info->base + info->size * static_cast<u32>(index);
    }
    if (p != 0 && gref<Handler>(p).handle != handle) p = 0;
    return host<Handler>(p);
}

}  // namespace sly4::snd
