// Handler_Free: unlinks a handler from the active list and its family.
#include "snd.h"

namespace sly4::snd {
using namespace opentit::native;

namespace {

constexpr u32 kLastActive = 0x00F8E564;      // u32 Handler*
constexpr u32 kFreeCallback = 0x00F8DADC;    // u32 OPD: (handle, h+8, 0) for handlers with flag 0x20000
constexpr u32 kPlayingCounts = 0x011D8754;   // s32[] by group (s8 at h+16)
constexpr u32 kFmtLfo = 0x00AFFB24;          // "%s(%d) : Handler with active LFO\n"
constexpr u32 kFmtCorruptFamily = 0x00AFFB7C;  // "... Corrupt family list found unlinking handle %x.\n"
constexpr u32 kFmtCorruptList = 0x00AFFAE4;    // "... Corrupt handler list found unlinking handle %x.\n"
constexpr u32 kFmtPlayingCount = 0x00AFFB4C;   // "... Invalid playing handler count.\n"

// Logs "corrupt ... unlinking handle %x" (the handle with its live bit set
// again) and the invalid-parameter assertion. True = give up.
bool Corrupt(u32 fmt, const Handler& h, u32 line, u32 trap_pc) {
    Snd_Log(fmt, kFileSndhand, line, h.handle | 0x80000000u, 0, 0, 0, 0);
    return Snd_AssertFailed(kFmtInvalidSndhand, kFileSndhand, line + 1, trap_pc);
}

be_u32& PlayingCount(const Handler& h) {
    const u32 group = static_cast<u32>(static_cast<s32>(static_cast<s8>(h.group)));
    return gref<be_u32>(kPlayingCounts + group * 4);
}

}  // namespace

// void Handler_Free(Handler* h, bool with_children)
void Handler_Free(Handler* hp, u32 with_children) {
    if (!hp && Snd_AssertFailed(kFmtInvalidSndhand, kFileSndhand, 628, 0x006E5B64)) return;
    if (!hp || (hp->handle & 0x80000000u) != 0x80000000u) return;
    Handler& h = *hp;
    if (with_children == 0 && !h.parent && h.first_child) {
        Snd_SendEvent(120, 0, 0, 0, 0);
        with_children = 1;
    }
    Snd_Lock();
    if (h.flags & 0x20000) {
        if (const u32 cb = global<be_u32>(kFreeCallback)) call_ptr(cb, h.handle, h.user, 0u, cb);
    }
    if (with_children != 0)
        for (Handler* c = h.first_child; c; c = h.first_child) Handler_Free(c, with_children);
    if (const u32 owner = h.family) {
        Family_Release(owner);
        h.family = 0u;
    }

    // Active list.
    gp<Handler>& first = global<gp<Handler>>(kFirstActive);
    gp<Handler>& last = global<gp<Handler>>(kLastActive);
    if (Handler* prev = h.prev_active) {
        if (prev->next_active.get() != &h && Corrupt(kFmtCorruptList, h, 680, 0x006E5B24)) return;
        gref(h.prev_active).next_active = h.next_active.get();
    } else {
        if (first.get() != &h && Corrupt(kFmtCorruptList, h, 691, 0x006E5AC0)) return;
        first = h.next_active.get();
    }
    if (Handler* next = h.next_active) {
        if (next->prev_active.get() != &h && Corrupt(kFmtCorruptList, h, 705, 0x006E5A58)) return;
        gref(h.next_active).prev_active = h.prev_active.get();
    } else {
        if (last.get() != &h && Corrupt(kFmtCorruptList, h, 716, 0x006E59F4)) return;
        last = h.prev_active.get();
    }

    const u32 word = h.handle;
    h.owner = 0u;
    h.handle = word & 0x7FFFFFFFu;
    if (((word >> 24) & 0x1F) == kHandlerTypeVoice)
        for (u32 k = 0; k < 4; ++k)
            if (h.tracks[k].flags5 & 1) Snd_LogAt(kFmtLfo, kFileSndhand, 735);

    if (PlayingCount(h) == 0) {
        Snd_LogAt(kFmtPlayingCount, kFileSndhand, 748);
        if (Snd_AssertFailed(kFmtInvalidSndhand, kFileSndhand, 749, 0x006E598C)) return;
    }
    be_u32& playing = PlayingCount(h);
    if (playing != 0) playing = playing - 1;

    // Family: unlink from the parent's child list, hand the children to the parent.
    Handler* parent = h.parent;
    if (!parent) return Snd_Unlock();
    Handler* c0 = parent->first_child;
    if (c0 == &h) {
        parent->first_child = h.next_sibling.get();
    } else {
        Handler* c = c0;
        while (c && c->next_sibling.get() != &h) c = c->next_sibling;
        if (!c) {
            Snd_Log(kFmtCorruptFamily, kFileSndhand, 779, h.handle | 0x80000000u, 0, 0, 0, 0);
            return Snd_Unlock();
        }
        c->next_sibling = h.next_sibling.get();
    }

    Handler* children = h.first_child;
    if (!children) return Snd_Unlock();
    Handler* lastc = nullptr;
    for (Handler* c = children; c;) {
        if (c->parent.get() != &h && Corrupt(kFmtCorruptFamily, h, 800, 0x006E57DC)) return;
        Handler* next = c->next_sibling;
        if (!next) lastc = c;
        c->parent = h.parent.get();
        c = next;
    }
    Handler* mine = h.first_child;
    Handler& grand = gref(h.parent);
    Handler* theirs = grand.first_child;
    if (!theirs) {
        grand.first_child = mine;
    } else if (lastc) {
        lastc->next_sibling = theirs;
        gref(h.parent).first_child = h.first_child.get();
    } else if (Corrupt(kFmtCorruptFamily, h, 830, 0x006E58D8)) {
        return;
    }
    Snd_Unlock();
}

}  // namespace sly4::snd

NATIVE(006E54B8, sly4::snd::Handler_Free)
