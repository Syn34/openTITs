// Sound library leaf functions: status getters, channel release and reset,
// and copying synth effect parameters.
#include <atomic>

#include "more.h"
#include "snd.h"

namespace sly4::snd {
using namespace opentit::native;

namespace {

constexpr u32 kLastError = 0x011D71B4;     // s32 (name?)
constexpr u32 kTick = 0x011D7D50;          // u32 (name?)
constexpr u32 kVoiceFreeLists = 0x011D8914;  // {Voice** data, s32 count, s32 capacity}[] by voice type
constexpr u32 kReleaseThreshold = 0x00B0723C;  // f32 0.2
constexpr u32 kFmtInvalidSynth = 0x00B028D4;   // "%s(%d) : Invalid parameter.\n"
constexpr u32 kFileSynth = 0x00B00940;         // "src\common\boomrang\snd_synth_interface.cpp"

// Channel bit arrays (u32 words, bit per channel). Reset sets "free" and
// clears the rest.
constexpr u32 kChFree = 0x011D84E0;
constexpr u32 kChClear[] = {0x011D8460, 0x011D84A0, 0x011D83E0, 0x011D8420, 0x011D8560,
                            0x011D8520, 0x011D85A0, 0x011D85E0, 0x011D8620, 0x011D8660};

// A hardware voice: two parts at +108 / +112, the voice type at +104.
struct VoicePart {
    pad<16> _0;
    be_u32 levels;  // +16: f32 at +12 of it
    be_u32 state;   // +20: u32* state word (bit 0 = done)
};
struct HwVoice {
    pad<104> _0;
    u8 type;             // +104
    pad<3> _105;
    gp<VoicePart> a;     // +108
    gp<VoicePart> b;     // +112
};
struct VoiceFreeList {
    be_u32 data;      // +0 Voice**
    be_s32 count;     // +4
    be_s32 capacity;  // +8
};

// Copies a float through an FPR (lfs / stfs), as the recompiled code does.
void copyf(be_f32& dst, const be_f32& src) { dst = static_cast<f32>(static_cast<f64>(src.get())); }

}  // namespace

// s32 Snd_GetLastError()  (name?)
s32 Snd_GetLastError() { return global<be_s32>(kLastError); }

// u32 Snd_GetTick()  (name?): the counter handlers are stamped with (+12).
u32 Snd_GetTick() { return global<be_u32>(kTick); }

// void Channel_ReleaseVoice(Voice* v)  (name?)
// Stops the voice's two parts (the first only if it is flagged done or
// quieter than 0.2) and puts the voice back on its type's free list.
void Channel_ReleaseVoice(u32 vp) {
    HwVoice& v = gref<HwVoice>(vp);
    const u32 a = v.a.ea, b = v.b.ea;
    const VoicePart& pa = gref<VoicePart>(a);
    const u32 state = gref<be_u32>(pa.state);
    std::atomic_thread_fence(std::memory_order_seq_cst);  // lwsync
    if ((state & 1) != 0 || gref<be_f32>(pa.levels + 12) < global<be_f32>(kReleaseThreshold))
        call(mfn::Voice_SetStateAtomic, a, 1u);
    if (b != 0) call(mfn::Voice_SetStateAtomic, b, 1u);
    VoiceFreeList& list = gref<VoiceFreeList>(kVoiceFreeLists + static_cast<u32>(v.type) * 12);
    if (list.count == list.capacity) return;
    const u32 data = list.data;
    const u32 count = static_cast<u32>(list.count.get());
    list.count = static_cast<s32>(count + 1);
    gref<be_u32>(data + (count << 2)) = vp;
}

// void Channel_Reset(s32 ch)  (name?)
// Puts channel `ch` back to its idle state and marks it free.
void Channel_Reset(s32 ch) {
    if ((ch < 0 || !(Snd_ChannelCount() > ch)) &&
        Snd_AssertFailed(kFmtInvalidValloc, kFileValloc, 177, 0x006E80C4))
        return;
    Channel& c = gref<Channel>(Snd_ChannelTable() + static_cast<u32>(ch) * sizeof(Channel));
    c.b8 = 0xFF;
    c.b9 = 0xFF;
    c.type = 0xFF;
    c.group = 0;
    c.active = 0u;
    c.w24 = 0u;
    c.w28 = -1;
    c.w32 = 0u;
    c.next = nullptr;
    c.w36 = 0u;
    c.w56 = 0u;
    c.w12 = 0u;
    c.w64 = 0u;
    c.owner = 0u;
    c.callback = 0u;
    c.w100 = 0u;
    c.w48 = 0u;
    c.w72 = 0u;
    c.f80 = 1.0f;
    c.f84 = 1.0f;
    c.f88 = 1.0f;
    c.w92 = 0u;
    c.hw_voice = 0u;

    const s32 w = ch / 32;
    const u32 word = static_cast<u32>(static_cast<s32>(static_cast<u32>(w) << 2));
    const u32 shift = static_cast<u32>(ch - w * 32) & 63;  // slw: 0 for shifts of 32..63 (ch < 0)
    const u32 bit = shift >= 32 ? 0 : 1u << shift;
    be_u32& free_word = gref<be_u32>(kChFree + word);
    free_word = free_word | bit;
    for (u32 arr : kChClear) {
        be_u32& x = gref<be_u32>(arr + word);
        x = x & ~bit;
    }
}

// void Effects_Set(Effects* dst, const Effects* src)  (name?)
// dst->mask |= src->mask; then each parameter group whose bit is set in
// src->mask is copied from src (an unset group is re-stored from dst
// itself, through the same word or float copies).
void Effects_Set(u32 d, u32 s) {
    if ((d == 0 || s == 0) && Snd_AssertFailed(kFmtInvalidSynth, kFileSynth, 3717, 0x006F2C30)) return;
    be_u32& dmask = gref<be_u32>(d);
    const be_u32& smask = gref<be_u32>(s);
    dmask = dmask | smask;

    // 'w' = lwz/stw, 'f' = lfs/stfs.
    auto copy = [&](u32 off, const char* kinds, u32 bit) {
        const u32 from = (smask & bit) ? s : d;
        for (u32 k = 0; kinds[k]; ++k) {
            const u32 o = off + k * 4;
            if (kinds[k] == 'w')
                gref<be_u32>(d + o) = gref<be_u32>(from + o).get();
            else
                copyf(gref<be_f32>(d + o), gref<be_f32>(from + o));
        }
    };
    copy(4, "wfff", 0x1);
    copy(20, "wfff", 0x2);
    copy(36, "wfff", 0x4);
    copy(52, "wfff", 0x8);
    copy(68, "ffw", 0x10);
    copy(104, "f", 0x80);
    copy(108, "wfff", 0x100);
    copy(124, "f", 0x200);
    copy(128, "w", 0x400);
    for (u32 i = 0; i < 3; ++i) {
        copy(156 + i * 4, "f", 0x400);
        copy(132 + i * 4, "w", 0x400);
        copy(144 + i * 4, "w", 0x400);
        copy(168 + i * 4, "f", 0x400);
    }
    for (u32 i = 0; i < 3; ++i) {
        copy(80 + i * 4, "f", 0x20);
        copy(92 + i * 4, "w", 0x40);
    }
}

}  // namespace sly4::snd

NATIVE(006D585C, sly4::snd::Snd_GetLastError)
NATIVE(006D4F74, sly4::snd::Snd_GetTick)
NATIVE(006EFE90, sly4::snd::Channel_ReleaseVoice)
NATIVE(006E7EC0, sly4::snd::Channel_Reset)
NATIVE(006F2814, sly4::snd::Effects_Set)
