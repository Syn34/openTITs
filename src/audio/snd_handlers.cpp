// Sound library: logging, the active list, pausing groups and handlers.
#include "snd.h"

namespace sly4::snd {
using namespace opentit::native;

namespace {
be_u32& GroupFlags(u32 k) { return host<be_u32>(kGroupFlags)[k]; }
}  // namespace

// void Snd_Log(const char* fmt, ...)
// Formats into a 512-byte buffer and hands it to the log callback, unless
// logging is disabled. The variadic registers are spilled as the original
// does at r1 + 52..104 and passed as the va_list; the original uses its
// caller's parameter save area, here it is a frame of our own (native
// callers keep their locals right above r1).
void Snd_Log(u32 fmt, u64 a1, u64 a2, u64 a3, u64 a4, u64 a5, u64 a6, u64 a7) {
    GuestFrame frame(112);
    const u32 sp = frame.addr();
    *host<be_u32>(sp + 52) = fmt;
    const u64 rest[7] = {a1, a2, a3, a4, a5, a6, a7};
    for (u32 k = 0; k < 7; ++k) *host<be_u64>(sp + 56 + 8 * k) = rest[k];
    if (global<be_u32>(kLogDisabled) != 0) return;
    call(sfn2::Vsnprintf, kLogBuffer, 512u, fmt, sp + 56);
    call_ptr(global<be_u32>(kLogCallback), kLogBuffer);
}

// Handler* Handler_First()
Handler* Handler_First() { return global<gp<Handler>>(kFirstActive); }

// void Handler_UnlinkTracks(Handler* h)  (name?)
// Clears bit 0 of byte +5 of each of the handler's four 48-byte records
// and takes those that are on the track list off it.
void Handler_UnlinkTracks(Handler* h) {
    gp<Track>& list = global<gp<Track>>(kTrackList);
    be_u32& count = global<be_u32>(kTrackCount);
    for (u32 k = 0; k < kTrackCountPerHandler; ++k) {
        Track& t = gref(h).tracks[k];
        t.flags5 = static_cast<u8>(t.flags5 & 0xFE);
        Track* node = list;
        if (node == &t) {
            const u32 n = count;
            list = t.next.get();
            count = n - 1;
            continue;
        }
        while (node) {
            Track* next = node->next;
            if (next == &t) {
                const u32 n = count;
                node->next = t.next.get();
                count = n - 1;
                break;
            }
            node = next;
        }
    }
}

// void Groups_Pause(u32 bits) / Groups_Resume(u32 bits)
// Sets / clears bit 1 of the group flags for every bit in `bits`.
void Groups_Pause(u32 bits) {
    if (bits == 0 && Snd_AssertFailed(kFmtInvalidValloc, kFileValloc, 1710, 0x006EAA98)) return;
    Snd_Lock();
    for (u32 k = 0; k < 32; ++k)
        if (bits & (1u << k)) GroupFlags(k) = GroupFlags(k) | 2;
    Snd_Unlock();
}
void Groups_Resume(u32 bits) {
    if (bits == 0 && Snd_AssertFailed(kFmtInvalidValloc, kFileValloc, 1736, 0x006EAB5C)) return;
    Snd_Lock();
    for (u32 k = 0; k < 32; ++k)
        if (bits & (1u << k)) GroupFlags(k) = GroupFlags(k) & ~2u;
    Snd_Unlock();
}

namespace {
// The type's pause hook (types 0-7 only).
void Handler_TypePause(Handler* h, u32 type_word, u32 pause) {
    const u32 type = (type_word >> 24) & 0x1F;
    if (!(static_cast<s32>(type) < static_cast<s32>(kTypeCountPausable))) return;
    const HandlerType* info = global<gp<HandlerType>>(kHandlerTypes + type * 4);
    if (!info) return;
    if (const u32 fn = info->pause) call_ptr(fn, h, pause, fn);
}
}  // namespace

// void Handler_Pause(Handler* h, bool recurse, u32 reason)
// Adds `reason` (bits 1-3) to the handler's pause flags, pausing children first.
void Handler_Pause(Handler* hp, u32 recurse, u64 reason) {
    if ((!hp || !hp->live()) && Snd_AssertFailed(kFmtInvalidSndhand, kFileSndhand, 951, 0x006E609C)) return;
    Handler& h = gref(hp);
    if (recurse != 0)
        for (Handler* c = h.first_child; c; c = c->next_sibling) Handler_Pause(c, 1, reason);
    Handler_OnPause(hp);
    const u32 flags = h.flags | static_cast<u32>(reason);
    const u32 word = h.handle;
    h.flags = flags;
    Handler_TypePause(hp, word, 1);
}

// void Handler_Resume(Handler* h, bool recurse, u32 reason)
// Removes `reason`; once no pause bits (1-3) are left the handler resumes.
void Handler_Resume(Handler* hp, u32 recurse, u64 reason) {
    if ((!hp || !hp->live()) && Snd_AssertFailed(kFmtInvalidSndhand, kFileSndhand, 1018, 0x006E6204)) return;
    Handler& h = gref(hp);
    if (recurse != 0)
        for (Handler* c = h.first_child; c; c = c->next_sibling) Handler_Resume(c, 1, reason);
    const u32 flags = h.flags & ~static_cast<u32>(reason);
    h.flags = flags;
    if (flags & 0xE) return;
    Handler_OnResume(hp);
    Handler_TypePause(hp, h.handle, 0);
}

}  // namespace sly4::snd

NATIVE(006E45F8, sly4::snd::Snd_Log)
NATIVE(006E6AA0, sly4::snd::Handler_First)
NATIVE(006E322C, sly4::snd::Handler_UnlinkTracks)
NATIVE(006EA9E0, sly4::snd::Groups_Pause)
NATIVE(006EAAA0, sly4::snd::Groups_Resume)
NATIVE(006E5F4C, sly4::snd::Handler_Pause)
NATIVE(006E60A4, sly4::snd::Handler_Resume)
