// Sound library handler internals ("src/common/sndhand.cpp"): allocation,
// parameters, starting, family release and the error event.
#include "snd.h"

using namespace sly4;
using namespace sly4::snd;

namespace {

constexpr u32 kLastActive = 0x00F8E564;
constexpr u32 kPlayingCounts = 0x011E8754;   // s32[] by group
constexpr u32 kChannelCount = 0x011E86A4;
constexpr u32 kEventCallback = 0x00F8E538;   // u32 OPD: (cause, a, b, c, d)
constexpr u32 kFmtScreamError = 0x00AFF9E4;  // "SCREAM Error: cause %d -> %u, %u, %u, %u\n"
constexpr u32 kFmtBadArray = 0x00AFFA48;     // "... Invalid handler array.\n"
constexpr u32 kFmtInactiveEffects = 0x00AFFA70;  // "... Inactive handler with effects.\n"
constexpr u32 kFmtAlreadyOnList = 0x00AFFA9C;    // "... Handler %x already on list.\n"
constexpr u32 kFmtCorruptList = 0x00AFFAC4;      // "... Corrupt handler list.\n"
constexpr u32 kParamOne = 0x006E4908;            // f32 1.0
constexpr u32 kEffectsInit = 0x006F2644, kEffectsSet = 0x006F2814;  // (h+360[, params+40]) (name?)
constexpr u32 kNextSerial = 0x006D4F74;          // () -> u32 (name?)
constexpr u32 kOneBits = 0x3F800000u;

s32 words_for(s32 n) { return (n + 31) / 32; }
f32 via_fpr(f32 v) { return static_cast<f32>(static_cast<f64>(v)); }

}  // namespace

// void Snd_SendEvent(u32 cause, u32 a, u32 b, u32 c, u32 d)
// To the event callback if there is one, else logged (unless logging is off).
DECOMP(006E4780, Snd_SendEvent) {
    const u64 cause = ctx.gpr[3], a = ctx.gpr[4], b = ctx.gpr[5], c = ctx.gpr[6], d = ctx.gpr[7];
    if (const u32 cb = load<u32>(kEventCallback)) return void(call_ptr(ctx, cb, {cause, a, b, c, d}));
    if (load<u32>(kLogDisabled) != 0) return;
    call(ctx, sfn::Snd_Log, {kFmtScreamError, cause, a, b, c, d});
}

// void Family_Release(Family* f): releases the chain at +12 first, then clears +0.
DECOMP(006E5434, Family_Release) {
    const u32 f = arg<u32>(ctx, 0);
    if (f == 0 && Snd_AssertFailed(ctx, kFmtInvalid, kFileSndhand, 2078, 0x006E54B0)) return;
    if (const u32 next = load<u32>(f + 12)) call(ctx, 0x006E5434, {next});
    store<u32>(f, 0);
}

// Handler* Handler_Alloc(Handler* table, s32 count, u32 type, s32 size, void* owner,
//                        -, u32 user, s8 group, u32 flags (9th arg); f32 volume)
// Takes the first free (+84 == 0) handler of the array and initialises it.
DECOMP(006E4DC8, Handler_Alloc) {
    u32 p = arg<u32>(ctx, 0);
    const s32 count = arg<s32>(ctx, 1);
    const u32 type = arg<u32>(ctx, 2);
    const s32 size = arg<s32>(ctx, 3);
    const u32 owner = arg<u32>(ctx, 4);
    const u32 user = arg<u32>(ctx, 6);
    const s32 group = static_cast<s8>(arg<u32>(ctx, 7) & 0xFF);
    const u32 flags = load<u32>(static_cast<u32>(ctx.gpr[1]) + 116);  // 9th argument
    const f64 volume = farg(ctx, 0);
    if ((p == 0 || type > 31 || count < 0 || count > 256 || size < 564) &&
        Snd_AssertFailed(ctx, kFmtInvalid, kFileSndhand, 376, 0x006E51D8))
        return ret(ctx, 0u);

    call(ctx, sfn::Snd_Lock);
    s32 i = 0;
    for (;;) {
        if (!(count > i)) {
            call(ctx, sfn::Snd_Unlock);
            return ret(ctx, 0u);
        }
        if (((load<u32>(p) >> 24) & 0x1F) != type) {
            Snd_Log(ctx, kFmtBadArray, kFileSndhand, 391);
            if (Snd_AssertFailed(ctx, kFmtInvalid, kFileSndhand, 392, 0x006E4F10)) return ret(ctx, 0u);
        }
        if (load<u32>(p + 84) == 0) break;
        ++i;
        p += static_cast<u32>(size);
    }

    if (load<u32>(p + 88) != 0) {
        Snd_Log(ctx, kFmtInactiveEffects, kFileSndhand, 402);
        store<u32>(p + 88, 0);
    }
    const u32 word = load<u32>(p);
    store<u32>(p + 84, owner);
    store<u32>(p + kHNextActive, 0);
    const u32 base = flags & 0x10000;
    store<u32>(p + kHFlags, base);
    store<u32>(p, (word & 0xFFFF0000u) | ((word + 1) & 0xFFFF));
    store<u32>(p + 544, 0);
    store<u32>(p + kHParent, 0);
    store<u32>(p + kHFirstChild, 0);
    const u32 gflags = load<u32>(kGroupFlags + static_cast<u32>(group) * 4);
    store<u32>(p + kHNextSibling, 0);
    store<u32>(p + kHFlags, base | ((gflags & 2) ? 4u : 0u));
    for (s32 w = 0; w < words_for(load<s32>(kChannelCount)); ++w) store<u32>(p + 20 + static_cast<u32>(w) * 4, 0);

    store<u32>(p + 8, 0);
    ref<u8>(p + kHGroup) = static_cast<u8>(group);
    store<f32>(p + 284, static_cast<f32>(volume));
    store<u32>(p + 288, kOneBits);
    store<u32>(p + 292, kOneBits);
    store<u32>(p + 296, kOneBits);
    store<u32>(p + 304, user);
    for (const u32 off : {300u, 308u, 312u, 320u, 316u, 344u, 340u, 348u, 352u, 328u, 324u, 332u, 336u})
        store<u32>(p + off, 0);
    for (u32 k = 0; k < 4; ++k) ref<u8>(p + 356 + k) = 0;
    call(ctx, sfn::Handler_Finish, {p});  // Handler_UnlinkTracks
    for (u32 k = 0; k < kTrackCountPerHandler; ++k) {
        const u32 t = p + kTrackBase + k * kTrackSize;
        store<u16>(t, 0);
        store<u32>(t + 12, 0);
        ref<u8>(t + 2) = 0;
        ref<u8>(t + 4) = 0;
        ref<u8>(t + 5) = 0;
        store<u32>(t + 40, p);
    }
    call(ctx, kEffectsInit, {p + 360});
    store<u32>(p + 12, static_cast<u32>(call(ctx, kNextSerial)));
    call(ctx, sfn::Snd_Unlock);
    ret(ctx, p);
}

// void Handler_SetParams(Handler* h, const Params* p)
// Applies the fields selected by p->mask (+4) to h and all its children.
DECOMP(006E4910, Handler_SetParams) {
    const u32 h = arg<u32>(ctx, 0), p = arg<u32>(ctx, 1);
    if ((h == 0 || p == 0) && Snd_AssertFailed(ctx, kFmtInvalid, kFileSndhand, 152, 0x006E4C50)) return;
    for (u32 c = load<u32>(h + kHFirstChild); c != 0; c = load<u32>(c + kHNextSibling))
        call(ctx, 0x006E4910, {c, p});

    const u32 m = load<u32>(p + 4);
    if (m & 0x00100000) {
        if ((m & 0x80000000u) == 0) {
            const u32 bit = (load<u32>(p + 8) & 1) ? 0x20000u : 0u;
            store<u32>(h + kHFlags, load<u32>(h + kHFlags) | bit);
        }
        const u32 opts = load<u32>(p + 8);
        const u32 f = load<u32>(h + kHFlags);
        store<u32>(h + kHFlags, f | ((opts & 2) ? 0x200000u : 0u) | ((opts & 4) ? 0x400000u : 0u));
    }
    store<u32>(h + 8, (m & 0x40) ? load<u32>(p + 12) : load<u32>(h + 8));
    if (m & 0x000F0000)
        for (u32 k = 0; k < 4; ++k)
            if (m & (0x10000u << k)) ref<u8>(h + k + 356) = ref<u8>(p + 36 + k);
    if (m & 4) {
        const f32 v = load<f32>(p + 16);
        store<f32>(h + 288, via_fpr(v));
        if (static_cast<s32>(static_cast<u32>(rt::fctiw(v, true))) < 0)
            store<u32>(h + 288, 0);
        else if (v > load<f32>(kParamOne))
            store<u32>(h + 288, kOneBits);
    }
    {
        const u32 v = (m & 8) ? load<u32>(p + 20) : load<u32>(h + 300);
        store<u32>(h + 300, v);
        if ((v & 0x80000000u) == 0 && v > 359) store<u32>(h + 300, 359);
    }
    {
        const s32 v = (m & 0x10) ? load<s32>(p + 24) : load<s32>(h + 316);
        store<s32>(h + 316, v);
        if (v < 0)
            store<u32>(h + 316, 0);
        else if (static_cast<u32>(v) > 360)
            store<u32>(h + 316, 360);
    }
    {
        const s32 v = (m & 1) ? load<s32>(p + 28) : load<s32>(h + 324);
        store<s32>(h + 324, v < -7680 ? -7680 : (v > 7680 ? 7680 : v));
    }
    {
        const f32 v = (m & 2) ? load<f32>(p + 32) : load<f32>(h + 340);
        store<f32>(h + 340, via_fpr(v));
        if (static_cast<s32>(static_cast<u32>(rt::fctiw(v, true))) < -1)
            store<u32>(h + 340, 0xBF800000u);
        else if (v > load<f32>(kParamOne))
            store<u32>(h + 340, kOneBits);
    }
    if (m & 0x20) call(ctx, kEffectsSet, {h + 360, p + 40});
}

// void Handler_Start(Handler* h)
// Marks the handler live and links it into the active list (at the head
// with flag 0x40000, else at the tail), counting it in its group.
DECOMP(006E51E0, Handler_Start) {
    const u32 h = arg<u32>(ctx, 0);
    if ((h == 0 || (load<u32>(h) & 0x80000000u) != 0) &&
        Snd_AssertFailed(ctx, kFmtInvalid, kFileSndhand, 518, 0x006E542C))
        return;
    call(ctx, sfn::Snd_Lock);
    for (u32 q = load<u32>(kFirstActive); q != 0; q = load<u32>(q + kHNextActive))
        if (q == h) call(ctx, sfn::Snd_Log, {kFmtAlreadyOnList, kFileSndhand, 532, load<u32>(h)});
    store<u32>(h + 544, 0);
    store<u32>(h + kHNextActive, 0);
    const u32 word = load<u32>(h) | 0x80000000u;
    u32 last = load<u32>(kLastActive);
    u32 first = load<u32>(kFirstActive);
    store<u32>(h, word);
    if ((last == 0) != (first == 0)) {
        Snd_Log(ctx, kFmtCorruptList, kFileSndhand, 560);
        if (Snd_AssertFailed(ctx, kFmtInvalid, kFileSndhand, 561, 0x006E53E0)) return;
        first = load<u32>(kFirstActive);
        last = load<u32>(kLastActive);
    }
    if (first == 0) {
        store<u32>(kLastActive, h);
        store<u32>(kFirstActive, h);
    } else if (last == h) {
        Snd_Log(ctx, kFmtCorruptList, kFileSndhand, 589);
    } else if (load<u32>(h + kHFlags) & 0x40000) {
        store<u32>(h + 544, 0);
        const u32 head = load<u32>(kFirstActive);
        store<u32>(h + kHNextActive, head);
        store<u32>(head + 544, h);
        store<u32>(kFirstActive, h);
    } else {
        store<u32>(last + kHNextActive, h);
        const u32 old = load<u32>(kLastActive);
        store<u32>(kLastActive, h);
        store<u32>(h + 544, old);
    }
    const u32 slot = kPlayingCounts + static_cast<u32>(static_cast<s32>(static_cast<s8>(ref<u8>(h + kHGroup)))) * 4;
    store<u32>(slot, load<u32>(slot) + 1);
    call(ctx, sfn::Snd_Unlock);
}
