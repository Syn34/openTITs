// Sound library leaf functions: status getters, channel release and reset,
// and copying synth effect parameters.
#include <atomic>

#include "more.h"
#include "snd.h"

using namespace sly4;
using namespace sly4::snd;

namespace {

constexpr u32 kLastError = 0x011D71B4;     // s32 (name?)
constexpr u32 kTick = 0x011D7D50;          // u32 (name?)
constexpr u32 kChannelCount = 0x011E86A4;
constexpr u32 kChannels = 0x00F8E880;
constexpr u32 kChannelSize = 104;
constexpr u32 kVoiceFreeLists = 0x011D8914;  // {Voice** data, s32 count, s32 capacity}[] by voice type
constexpr u32 kReleaseThreshold = 0x00B0723C;  // f32 0.2
constexpr u32 kFmtInvalidSynth = 0x00B028D4;   // "%s(%d) : Invalid parameter.\n"
constexpr u32 kFileSynth = 0x00B00940;         // "src\common\boomrang\snd_synth_interface.cpp"

// Channel bit arrays (u32 words, bit per channel). Reset sets "free" and
// clears the rest.
constexpr u32 kChFree = 0x011E84E0;
constexpr u32 kChClear[] = {0x011E8460, 0x011E84A0, 0x011E83E0, 0x011E8420, 0x011E8560,
                            0x011E8520, 0x011E85A0, 0x011E85E0, 0x011E8620, 0x011E8660};

f64 lf(u32 ea) { return static_cast<f64>(load<f32>(ea)); }

}  // namespace

// s32 Snd_GetLastError()  (name?)
DECOMP(006D585C, Snd_GetLastError) { ret(ctx, load<s32>(kLastError)); }

// u32 Snd_GetTick()  (name?): the counter handlers are stamped with (+12).
DECOMP(006D4F74, Snd_GetTick) { ret(ctx, load<u32>(kTick)); }

// void Channel_ReleaseVoice(Voice* v)  (name?)
// Stops the voice's two parts (the first only if it is flagged done or
// quieter than 0.2) and puts the voice back on its type's free list.
DECOMP(006EFE90, Channel_ReleaseVoice) {
    const u32 v = arg<u32>(ctx, 0);
    const u32 a = load<u32>(v + 108), b = load<u32>(v + 112);
    const u32 state = load<u32>(load<u32>(a + 20));
    std::atomic_thread_fence(std::memory_order_seq_cst);  // lwsync
    if ((state & 1) != 0 || lf(load<u32>(a + 16) + 12) < lf(kReleaseThreshold))
        call(ctx, mfn::Voice_SetStateAtomic, {a, 1});
    if (b != 0) call(ctx, mfn::Voice_SetStateAtomic, {b, 1});
    const u32 list = kVoiceFreeLists + static_cast<u32>(ref<u8>(v + 104)) * 12;
    if (load<s32>(list + 4) == load<s32>(list + 8)) return;
    const u32 data = load<u32>(list);
    const u32 count = load<u32>(list + 4);
    store<u32>(list + 4, count + 1);
    store<u32>(data + (count << 2), v);
}

// void Channel_Reset(s32 ch)  (name?)
// Puts channel `ch` back to its idle state and marks it free.
DECOMP(006E7EC0, Channel_Reset) {
    const s32 ch = arg<s32>(ctx, 0);
    if ((ch < 0 || !(load<s32>(kChannelCount) > ch)) &&
        Snd_AssertFailed(ctx, kFmtInvalidValloc, kFileValloc, 177, 0x006E80C4))
        return;
    const u32 c = load<u32>(kChannels) + static_cast<u32>(ch) * kChannelSize;
    ref<u8>(c + 8) = 0xFF;
    ref<u8>(c + 9) = 0xFF;
    ref<u8>(c + 11) = 0xFF;
    ref<u8>(c + 10) = 0;
    for (u32 o : {16u, 24u}) store<u32>(c + o, 0);
    store<s32>(c + 28, -1);
    for (u32 o : {32u, 0u, 36u, 56u, 12u, 64u, 40u, 44u, 100u, 48u, 72u}) store<u32>(c + o, 0);
    for (u32 o : {80u, 84u, 88u}) store<u32>(c + o, 0x3F800000u);  // 1.0f
    store<u32>(c + 92, 0);
    store<u32>(c + 96, 0);

    const s32 w = ch / 32;
    const u32 word = static_cast<u32>(static_cast<s32>(static_cast<u32>(w) << 2));
    const u32 shift = static_cast<u32>(ch - w * 32) & 63;  // slw: 0 for shifts of 32..63 (ch < 0)
    const u32 bit = shift >= 32 ? 0 : 1u << shift;
    store<u32>(kChFree + word, load<u32>(kChFree + word) | bit);
    for (u32 arr : kChClear) store<u32>(arr + word, load<u32>(arr + word) & ~bit);
}

// void Effects_Set(Effects* dst, const Effects* src)  (name?)
// dst->mask |= src->mask; then each parameter group whose bit is set in
// src->mask is copied from src (an unset group is re-stored from dst
// itself, through the same word or float copies).
DECOMP(006F2814, Effects_Set) {
    const u32 d = arg<u32>(ctx, 0), s = arg<u32>(ctx, 1);
    if ((d == 0 || s == 0) && Snd_AssertFailed(ctx, kFmtInvalidSynth, kFileSynth, 3717, 0x006F2C30)) return;
    store<u32>(d, load<u32>(d) | load<u32>(s));

    // 'w' = lwz/stw, 'f' = lfs/stfs (through double, as the recompiled code does).
    auto copy = [&](u32 off, const char* kinds, u32 bit) {
        const u32 from = (load<u32>(s) & bit) ? s : d;
        for (u32 k = 0; kinds[k]; ++k) {
            const u32 o = off + k * 4;
            if (kinds[k] == 'w')
                store<u32>(d + o, load<u32>(from + o));
            else
                store<f32>(d + o, static_cast<f32>(lf(from + o)));
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
