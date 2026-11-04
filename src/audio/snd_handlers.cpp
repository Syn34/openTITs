// Sound library: logging, the active list, pausing groups and handlers.
#include "snd.h"

using namespace sly4;
using namespace sly4::snd;

// void Snd_Log(const char* fmt, ...)
// Formats into a 512-byte buffer and hands it to the log callback, unless
// logging is disabled. Like the original, the variadic registers are spilled
// into the caller's parameter save area (r1 + 52..104) and passed as the va_list.
DECOMP(006E45F8, Snd_Log) {
    const u32 sp = static_cast<u32>(ctx.gpr[1]);
    const u32 fmt = arg<u32>(ctx, 0);
    store<u32>(sp + 52, fmt);
    for (u32 k = 1; k < 8; ++k) store<u64>(sp + 48 + 8 * k, ctx.gpr[3 + k]);
    if (load<u32>(kLogDisabled) != 0) return;
    call(ctx, sfn2::Vsnprintf, {kLogBuffer, 512, fmt, sp + 56});
    call_ptr(ctx, load<u32>(kLogCallback), {kLogBuffer});
}

// Handler* Handler_First()
DECOMP(006E6AA0, Handler_First) { ret(ctx, load<u32>(kFirstActive)); }

// void Handler_UnlinkTracks(Handler* h)  (name?)
// Clears bit 0 of byte +5 of each of the handler's four 48-byte records
// and takes those that are on the track list off it.
DECOMP(006E322C, Handler_UnlinkTracks) {
    const u32 h = arg<u32>(ctx, 0);
    for (u32 k = 0; k < kTrackCountPerHandler; ++k) {
        const u32 t = h + k * kTrackSize + kTrackBase;
        ref<u8>(t + 5) = static_cast<u8>(ref<u8>(t + 5) & 0xFE);
        u32 node = load<u32>(kTrackList);
        if (node == t) {
            const u32 count = load<u32>(kTrackCount);
            store<u32>(kTrackList, load<u32>(t + kTrackNext));
            store<u32>(kTrackCount, count - 1);
            continue;
        }
        while (node != 0) {
            const u32 next = load<u32>(node + kTrackNext);
            if (next == t) {
                const u32 count = load<u32>(kTrackCount);
                store<u32>(node + kTrackNext, load<u32>(t + kTrackNext));
                store<u32>(kTrackCount, count - 1);
                break;
            }
            node = next;
        }
    }
}

// void Groups_Pause(u32 bits) / Groups_Resume(u32 bits)
// Sets / clears bit 1 of the group flags for every bit in `bits`.
DECOMP(006EA9E0, Groups_Pause) {
    const u32 bits = arg<u32>(ctx, 0);
    if (bits == 0 && Snd_AssertFailed(ctx, kFmtInvalidValloc, kFileValloc, 1710, 0x006EAA98)) return;
    call(ctx, sfn::Snd_Lock);
    for (u32 k = 0; k < 32; ++k)
        if (bits & (1u << k)) store<u32>(kGroupFlags + k * 4, load<u32>(kGroupFlags + k * 4) | 2);
    call(ctx, sfn::Snd_Unlock);
}
DECOMP(006EAAA0, Groups_Resume) {
    const u32 bits = arg<u32>(ctx, 0);
    if (bits == 0 && Snd_AssertFailed(ctx, kFmtInvalidValloc, kFileValloc, 1736, 0x006EAB5C)) return;
    call(ctx, sfn::Snd_Lock);
    for (u32 k = 0; k < 32; ++k)
        if (bits & (1u << k)) store<u32>(kGroupFlags + k * 4, load<u32>(kGroupFlags + k * 4) & ~2u);
    call(ctx, sfn::Snd_Unlock);
}

namespace {
// The type's pause hook (types 0-7 only).
void Handler_TypePause(PPUContext& ctx, u32 h, u32 type_word, u32 pause) {
    const u32 type = (type_word >> 24) & 0x1F;
    if (!(static_cast<s32>(type) < static_cast<s32>(kTypeCountPausable))) return;
    const u32 info = load<u32>(kHandlerTypes + type * 4);
    if (info == 0) return;
    if (const u32 fn = load<u32>(info + kHTypeInfoPause)) call_ptr(ctx, fn, {h, pause, fn});
}
}  // namespace

// void Handler_Pause(Handler* h, bool recurse, u32 reason)
// Adds `reason` (bits 1-3) to the handler's pause flags, pausing children first.
DECOMP(006E5F4C, Handler_Pause) {
    const u32 h = arg<u32>(ctx, 0), recurse = arg<u32>(ctx, 1);
    const u64 reason = ctx.gpr[5];
    if ((h == 0 || (load<u32>(h) & 0x80000000u) == 0) &&
        Snd_AssertFailed(ctx, kFmtInvalid, kFileSndhand, 951, 0x006E609C))
        return;
    if (recurse != 0)
        for (u32 c = load<u32>(h + kHFirstChild); c != 0; c = load<u32>(c + kHNextSibling))
            call(ctx, sfn::Handler_Pause, {c, 1, reason});
    call(ctx, sfn2::Handler_OnPause, {h});
    const u32 flags = load<u32>(h + kHFlags) | static_cast<u32>(reason);
    const u32 word = load<u32>(h);
    store<u32>(h + kHFlags, flags);
    Handler_TypePause(ctx, h, word, 1);
}

// void Handler_Resume(Handler* h, bool recurse, u32 reason)
// Removes `reason`; once no pause bits (1-3) are left the handler resumes.
DECOMP(006E60A4, Handler_Resume) {
    const u32 h = arg<u32>(ctx, 0), recurse = arg<u32>(ctx, 1);
    const u64 reason = ctx.gpr[5];
    if ((h == 0 || (load<u32>(h) & 0x80000000u) == 0) &&
        Snd_AssertFailed(ctx, kFmtInvalid, kFileSndhand, 1018, 0x006E6204))
        return;
    if (recurse != 0)
        for (u32 c = load<u32>(h + kHFirstChild); c != 0; c = load<u32>(c + kHNextSibling))
            call(ctx, sfn::Handler_Resume, {c, 1, reason});
    const u32 flags = load<u32>(h + kHFlags) & ~static_cast<u32>(reason);
    store<u32>(h + kHFlags, flags);
    if (flags & 0xE) return;
    call(ctx, sfn2::Handler_OnResume, {h});
    Handler_TypePause(ctx, h, load<u32>(h), 0);
}
