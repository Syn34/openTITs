// Sound library handler internals ("src/common/sndhand.cpp"): allocation,
// parameters, starting, family release and the error event.
#include "snd.h"

namespace sly4::snd {
using namespace opentit::native;

namespace {

constexpr u32 kLastActive = 0x00F8E564;
constexpr u32 kPlayingCounts = 0x011D8754;   // s32[] by group
constexpr u32 kEventCallback = 0x00F8E538;   // u32 OPD: (cause, a, b, c, d)
constexpr u32 kFmtScreamError = 0x00AFF9E4;  // "SCREAM Error: cause %d -> %u, %u, %u, %u\n"
constexpr u32 kFmtBadArray = 0x00AFFA48;     // "... Invalid handler array.\n"
constexpr u32 kFmtInactiveEffects = 0x00AFFA70;  // "... Inactive handler with effects.\n"
constexpr u32 kFmtAlreadyOnList = 0x00AFFA9C;    // "... Handler %x already on list.\n"
constexpr u32 kFmtCorruptList = 0x00AFFAC4;      // "... Corrupt handler list.\n"
constexpr u32 kParamOne = 0x006E4908;            // f32 1.0
constexpr u32 kEffectsInit = 0x006F2644;         // (h+360) (name?)
constexpr u32 kOneBits = 0x3F800000u;

// The parameter block Handler_SetParams reads (p->mask at +4).
struct Params {
    native::be_u32 size;     // +0
    native::be_u32 mask;     // +4
    native::be_u32 opts;     // +8
    native::be_u32 user;     // +12 (mask 0x40)
    native::be_f32 gain;     // +16 (mask 4)
    native::be_u32 angle;    // +20 (mask 8)
    native::be_s32 s24;      // +24 (mask 0x10)
    native::be_s32 pitch;    // +28 (mask 1)
    native::be_f32 f32_;     // +32 (mask 2)
    u8 params[4];            // +36 (mask 0x10000 << i)
    native::pad<4> effects;  // +40 (mask 0x20): Effects
};

f32 via_fpr(f32 v) { return static_cast<f32>(static_cast<f64>(v)); }
be_u32& PlayingCount(const Handler& h) {
    return gref<be_u32>(kPlayingCounts + static_cast<u32>(static_cast<s32>(static_cast<s8>(h.group))) * 4);
}

}  // namespace

// void Snd_SendEvent(u32 cause, u32 a, u32 b, u32 c, u32 d)
// To the event callback if there is one, else logged (unless logging is off).
void Snd_SendEvent(u64 cause, u64 a, u64 b, u64 c, u64 d) {
    if (const u32 cb = global<be_u32>(kEventCallback)) return call_ptr(cb, cause, a, b, c, d);
    if (global<be_u32>(kLogDisabled) != 0) return;
    Snd_Log(kFmtScreamError, cause, a, b, c, d, 0, 0);
}

// void Family_Release(Family* f): releases the chain at +12 first, then clears +0.
void Family_Release(u32 f) {
    if (f == 0 && Snd_AssertFailed(kFmtInvalidSndhand, kFileSndhand, 2078, 0x006E54B0)) return;
    if (const u32 next = gref<be_u32>(f + 12)) Family_Release(next);
    gref<be_u32>(f) = 0u;
}

// Handler* Handler_Alloc(Handler* table, s32 count, u32 type, s32 size, void* owner,
//                        -, u32 user, s8 group, u32 flags (9th arg); f32 volume)
// Takes the first free (+84 == 0) handler of the array and initialises it.
Handler* Handler_Alloc(u32 table, s32 count, u32 type, s32 size, u32 owner, u32, u32 user, s8 group, u32 flags,
                       f32 volume) {
    if ((table == 0 || type > 31 || count < 0 || count > 256 || size < 564) &&
        Snd_AssertFailed(kFmtInvalidSndhand, kFileSndhand, 376, 0x006E51D8))
        return nullptr;

    Snd_Lock();
    u32 p = table;
    s32 i = 0;
    for (;;) {
        if (!(count > i)) {
            Snd_Unlock();
            return nullptr;
        }
        if (gref<Handler>(p).type() != type) {
            Snd_LogAt(kFmtBadArray, kFileSndhand, 391);
            if (Snd_AssertFailed(kFmtInvalidSndhand, kFileSndhand, 392, 0x006E4F10)) return nullptr;
        }
        if (gref<Handler>(p).owner == 0) break;
        ++i;
        p += static_cast<u32>(size);
    }

    Handler& h = gref<Handler>(p);
    if (h.family != 0) {
        Snd_LogAt(kFmtInactiveEffects, kFileSndhand, 402);
        h.family = 0u;
    }
    const u32 word = h.handle;
    h.owner = owner;
    h.next_active = nullptr;
    const u32 base = flags & 0x10000;
    h.flags = base;
    h.handle = (word & 0xFFFF0000u) | ((word + 1) & 0xFFFF);
    h.prev_active = nullptr;
    h.parent = nullptr;
    h.first_child = nullptr;
    const u32 gflags = gref<be_u32>(kGroupFlags + static_cast<u32>(static_cast<s32>(group)) * 4);
    h.next_sibling = nullptr;
    h.flags = base | ((gflags & 2) ? 4u : 0u);
    for (s32 w = 0; w < Snd_ChannelWords(Snd_ChannelCount()); ++w) h.channel_word(w) = 0u;

    h.user = 0u;
    h.group = static_cast<u8>(group);
    h.volume = volume;
    h.gain = std::bit_cast<f32>(kOneBits);
    h.f292 = std::bit_cast<f32>(kOneBits);
    h.f296 = std::bit_cast<f32>(kOneBits);
    h.user304 = user;
    h.angle = 0u;
    h.w308 = 0u;
    h.w312 = 0u;
    h.w320 = 0u;
    h.s316 = 0;
    h.w344 = 0u;
    h.f340 = 0.0f;
    h.w348 = 0u;
    h.w352 = 0u;
    h.w328 = 0u;
    h.pitch = 0;
    h.w332 = 0u;
    h.w336 = 0u;
    for (u32 k = 0; k < 4; ++k) h.params[k] = 0;
    Handler_UnlinkTracks(&h);
    for (u32 k = 0; k < kTrackCountPerHandler; ++k) {
        Track& t = h.tracks[k];
        t.w0 = 0;
        t.w12 = 0u;
        t.b2 = 0;
        t.b4 = 0;
        t.flags5 = 0;
        t.owner = &h;
    }
    call(kEffectsInit, p + 360);
    h.tick = Snd_GetTick();
    Snd_Unlock();
    return &h;
}

// void Handler_SetParams(Handler* h, const Params* p)
// Applies the fields selected by p->mask (+4) to h and all its children.
void Handler_SetParams(Handler* hp, const u8* pp) {
    if ((!hp || !pp) && Snd_AssertFailed(kFmtInvalidSndhand, kFileSndhand, 152, 0x006E4C50)) return;
    Handler& h = gref(hp);
    const Params& p = gref<Params>(guest(pp));
    for (Handler* c = h.first_child; c; c = c->next_sibling) Handler_SetParams(c, pp);

    const u32 m = p.mask;
    if (m & 0x00100000) {
        if ((m & 0x80000000u) == 0) {
            const u32 bit = (p.opts & 1) ? 0x20000u : 0u;
            h.flags = h.flags | bit;
        }
        const u32 opts = p.opts;
        const u32 f = h.flags;
        h.flags = f | ((opts & 2) ? 0x200000u : 0u) | ((opts & 4) ? 0x400000u : 0u);
    }
    h.user = (m & 0x40) ? p.user.get() : h.user.get();
    if (m & 0x000F0000)
        for (u32 k = 0; k < 4; ++k)
            if (m & (0x10000u << k)) h.params[k] = p.params[k];
    if (m & 4) {
        const f32 v = p.gain;
        h.gain = via_fpr(v);
        if (static_cast<s32>(static_cast<u32>(rt::fctiw(v, true))) < 0)
            h.gain = 0.0f;
        else if (v > global<be_f32>(kParamOne))
            h.gain = std::bit_cast<f32>(kOneBits);
    }
    {
        const u32 v = (m & 8) ? p.angle.get() : h.angle.get();
        h.angle = v;
        if ((v & 0x80000000u) == 0 && v > 359) h.angle = 359u;
    }
    {
        const s32 v = (m & 0x10) ? p.s24.get() : h.s316.get();
        h.s316 = v;
        if (v < 0)
            h.s316 = 0;
        else if (static_cast<u32>(v) > 360)
            h.s316 = 360;
    }
    {
        const s32 v = (m & 1) ? p.pitch.get() : h.pitch.get();
        h.pitch = v < -7680 ? -7680 : (v > 7680 ? 7680 : v);
    }
    {
        const f32 v = (m & 2) ? p.f32_.get() : h.f340.get();
        h.f340 = via_fpr(v);
        if (static_cast<s32>(static_cast<u32>(rt::fctiw(v, true))) < -1)
            h.f340 = std::bit_cast<f32>(0xBF800000u);
        else if (v > global<be_f32>(kParamOne))
            h.f340 = std::bit_cast<f32>(kOneBits);
    }
    if (m & 0x20) Effects_Set(guest(hp) + 360, guest(pp) + 40);
}

// void Handler_Start(Handler* h)
// Marks the handler live and links it into the active list (at the head
// with flag 0x40000, else at the tail), counting it in its group.
void Handler_Start(Handler* hp) {
    if ((!hp || hp->live()) && Snd_AssertFailed(kFmtInvalidSndhand, kFileSndhand, 518, 0x006E542C)) return;
    Handler& h = gref(hp);
    gp<Handler>& first_g = global<gp<Handler>>(kFirstActive);
    gp<Handler>& last_g = global<gp<Handler>>(kLastActive);
    Snd_Lock();
    for (Handler* q = first_g; q; q = q->next_active)
        if (q == &h) Snd_Log(kFmtAlreadyOnList, kFileSndhand, 532, h.handle, 0, 0, 0, 0);
    h.prev_active = nullptr;
    h.next_active = nullptr;
    const u32 word = h.handle | 0x80000000u;
    Handler* last = last_g;
    Handler* first = first_g;
    h.handle = word;
    if (!last != !first) {
        Snd_LogAt(kFmtCorruptList, kFileSndhand, 560);
        if (Snd_AssertFailed(kFmtInvalidSndhand, kFileSndhand, 561, 0x006E53E0)) return;
        first = first_g;
        last = last_g;
    }
    if (!first) {
        last_g = &h;
        first_g = &h;
    } else if (guest(last) == guest(&h)) {  // (guest addresses: h may be 0)
        Snd_LogAt(kFmtCorruptList, kFileSndhand, 589);
    } else if (h.flags & 0x40000) {
        h.prev_active = nullptr;
        Handler* head = first_g;
        h.next_active = head;
        gref(head).prev_active = &h;
        first_g = &h;
    } else {
        gref(last).next_active = &h;
        Handler* old = last_g;
        last_g = &h;
        h.prev_active = old;
    }
    be_u32& slot = PlayingCount(h);
    slot = slot + 1;
    Snd_Unlock();
}

}  // namespace sly4::snd

NATIVE(006E4780, sly4::snd::Snd_SendEvent)
NATIVE(006E5434, sly4::snd::Family_Release)
NATIVE(006E4DC8, sly4::snd::Handler_Alloc)
NATIVE(006E4910, sly4::snd::Handler_SetParams)
NATIVE(006E51E0, sly4::snd::Handler_Start)
