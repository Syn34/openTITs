// Handler_Free: unlinks a handler from the active list and its family.
#include "snd.h"

using namespace sly4;
using namespace sly4::snd;

namespace {

constexpr u32 kLastActive = 0x00F8E564;      // u32 Handler*
constexpr u32 kHPrevActive = 544;            // (kHNextActive = 548)
constexpr u32 kHFamilyOwner = 88;            // u32 object released with 0x006E5434
constexpr u32 kFreeCallback = 0x00F8DADC;    // u32 OPD: (handle, h+8, 0) for handlers with flag 0x20000
constexpr u32 kPlayingCounts = 0x011E8754;   // s32[] by group (s8 at h+16)
constexpr u32 kSendEvent = 0x006E4780;       // (120, 0, 0, 0, 0) (name?)
constexpr u32 kReleaseOwner = 0x006E5434;    // (obj) (name?)
constexpr u32 kFmtLfo = 0x00AFFB24;          // "%s(%d) : Handler with active LFO\n"
constexpr u32 kFmtCorruptFamily = 0x00AFFB7C;  // "... Corrupt family list found unlinking handle %x.\n"
constexpr u32 kFmtCorruptList = 0x00AFFAE4;    // "... Corrupt handler list found unlinking handle %x.\n"
constexpr u32 kFmtPlayingCount = 0x00AFFB4C;   // "... Invalid playing handler count.\n"

// Logs "corrupt ... unlinking handle %x" (the handle with its live bit set
// again) and the invalid-parameter assertion. True = give up.
bool Corrupt(PPUContext& ctx, u32 fmt, u32 h, u32 line, u32 trap_pc) {
    call(ctx, sfn::Snd_Log, {fmt, kFileSndhand, line, load<u32>(h) | 0x80000000u});
    return Snd_AssertFailed(ctx, kFmtInvalid, kFileSndhand, line + 1, trap_pc);
}

}  // namespace

// void Handler_Free(Handler* h, bool with_children)
DECOMP(006E54B8, Handler_Free) {
    const u32 h = arg<u32>(ctx, 0);
    u32 with_children = arg<u32>(ctx, 1);
    if (h == 0 && Snd_AssertFailed(ctx, kFmtInvalid, kFileSndhand, 628, 0x006E5B64)) return;
    if (h == 0 || (load<u32>(h) & 0x80000000u) != 0x80000000u) return;
    if (with_children == 0 && load<u32>(h + kHParent) == 0 && load<u32>(h + kHFirstChild) != 0) {
        call(ctx, kSendEvent, {120, 0, 0, 0, 0});
        with_children = 1;
    }
    call(ctx, sfn::Snd_Lock);
    if (load<u32>(h + kHFlags) & 0x20000) {
        if (const u32 cb = load<u32>(kFreeCallback)) call_ptr(ctx, cb, {load<u32>(h), load<u32>(h + 8), 0, cb});
    }
    if (with_children != 0)
        for (u32 c = load<u32>(h + kHFirstChild); c != 0; c = load<u32>(h + kHFirstChild))
            call(ctx, sfn::Handler_Free, {c, with_children});
    if (const u32 owner = load<u32>(h + kHFamilyOwner)) {
        call(ctx, kReleaseOwner, {owner});
        store<u32>(h + kHFamilyOwner, 0);
    }

    // Active list.
    if (const u32 prev = load<u32>(h + kHPrevActive)) {
        if (load<u32>(prev + kHNextActive) != h && Corrupt(ctx, kFmtCorruptList, h, 680, 0x006E5B24)) return;
        store<u32>(load<u32>(h + kHPrevActive) + kHNextActive, load<u32>(h + kHNextActive));
    } else {
        if (load<u32>(kFirstActive) != h && Corrupt(ctx, kFmtCorruptList, h, 691, 0x006E5AC0)) return;
        store<u32>(kFirstActive, load<u32>(h + kHNextActive));
    }
    if (const u32 next = load<u32>(h + kHNextActive)) {
        if (load<u32>(next + kHPrevActive) != h && Corrupt(ctx, kFmtCorruptList, h, 705, 0x006E5A58)) return;
        store<u32>(load<u32>(h + kHNextActive) + kHPrevActive, load<u32>(h + kHPrevActive));
    } else {
        if (load<u32>(kLastActive) != h && Corrupt(ctx, kFmtCorruptList, h, 716, 0x006E59F4)) return;
        store<u32>(kLastActive, load<u32>(h + kHPrevActive));
    }

    const u32 word = load<u32>(h);
    store<u32>(h + 84, 0);
    store<u32>(h, word & 0x7FFFFFFFu);
    if (((word >> 24) & 0x1F) == kHandlerTypeVoice)
        for (u32 k = 0; k < 4; ++k)
            if (ref<u8>(h + k * 48 + 97) & 1) Snd_Log(ctx, kFmtLfo, kFileSndhand, 735);

    u32 group = static_cast<u32>(static_cast<s32>(static_cast<s8>(ref<u8>(h + kHGroup))));
    if (load<u32>(kPlayingCounts + group * 4) == 0) {
        Snd_Log(ctx, kFmtPlayingCount, kFileSndhand, 748);
        if (Snd_AssertFailed(ctx, kFmtInvalid, kFileSndhand, 749, 0x006E598C)) return;
        group = static_cast<u32>(static_cast<s32>(static_cast<s8>(ref<u8>(h + kHGroup))));
    }
    if (load<u32>(kPlayingCounts + group * 4) != 0)
        store<u32>(kPlayingCounts + group * 4, load<u32>(kPlayingCounts + group * 4) - 1);

    // Family: unlink from the parent's child list, hand the children to the parent.
    const u32 parent = load<u32>(h + kHParent);
    if (parent == 0) return void(call(ctx, sfn::Snd_Unlock));
    const u32 first = load<u32>(parent + kHFirstChild);
    if (first == h) {
        store<u32>(parent + kHFirstChild, load<u32>(h + kHNextSibling));
    } else {
        u32 c = first;
        while (c != 0 && load<u32>(c + kHNextSibling) != h) c = load<u32>(c + kHNextSibling);
        if (c == 0) {
            call(ctx, sfn::Snd_Log, {kFmtCorruptFamily, kFileSndhand, 779, load<u32>(h) | 0x80000000u});
            return void(call(ctx, sfn::Snd_Unlock));
        }
        store<u32>(c + kHNextSibling, load<u32>(h + kHNextSibling));
    }

    const u32 children = load<u32>(h + kHFirstChild);
    if (children == 0) return void(call(ctx, sfn::Snd_Unlock));
    u32 last = 0;
    for (u32 c = children; c != 0;) {
        if (load<u32>(c + kHParent) != h && Corrupt(ctx, kFmtCorruptFamily, h, 800, 0x006E57DC)) return;
        const u32 next = load<u32>(c + kHNextSibling);
        if (next == 0) last = c;
        store<u32>(c + kHParent, load<u32>(h + kHParent));
        c = next;
    }
    const u32 mine = load<u32>(h + kHFirstChild);
    const u32 grand = load<u32>(h + kHParent);
    const u32 theirs = load<u32>(grand + kHFirstChild);
    if (theirs == 0) {
        store<u32>(grand + kHFirstChild, mine);
    } else if (last != 0) {
        store<u32>(last + kHNextSibling, theirs);
        store<u32>(load<u32>(h + kHParent) + kHFirstChild, load<u32>(h + kHFirstChild));
    } else if (Corrupt(ctx, kFmtCorruptFamily, h, 830, 0x006E58D8)) {
        return;
    }
    call(ctx, sfn::Snd_Unlock);
}
