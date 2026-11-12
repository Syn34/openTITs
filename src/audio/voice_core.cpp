// Handle layer of the sound library: lock, lookup, count and stop.
#include "snd.h"

namespace sly4::snd {
using namespace opentit::native;

// Snd_Lock / Snd_Unlock: the library's global mutex.
void Snd_Lock() { call(sfn::Snd_LockImpl); }
void Snd_Unlock() { call(sfn::Snd_UnlockImpl); }

// Handler* Snd_Lookup(u32 handle)
// 0 for a handle without the live bit. (The original's range check on the
// index can't fail: it is 8 bits.)
Handler* Snd_Lookup(u32 handle) {
    if ((handle & 0x80000000u) == 0) return nullptr;
    Snd_Lock();
    Handler* p = Snd_Find(handle);
    Snd_Unlock();
    return p;
}

// u32 Handler_CountActive(Handler* h)
// Counts type-4 handlers with +581 set in h and all its descendants.
u32 Handler_CountActive(Handler* h) {
    Handler& self = gref(h);
    Handler* child = self.first_child;
    u32 count = (self.type() == 4 && gref<u8>(guest(h) + 581) != 0) ? 1 : 0;
    while (child) {
        count += Handler_CountActive(child);
        child = child->next_sibling;
    }
    return count;
}

// void Handler_Stop(Handler* h, bool recurse, bool force, bool flag)
//
// Marks the handler as stopping (0x80; 0x20 too with `flag`), stops its
// children first when recursing (an already stopping handler always
// recurses and forces), then stops it through its type: the voice stop for
// type 5, the type's stop function otherwise. If nothing was stopped and it
// wasn't forced, it is released; otherwise it is detached, finished and,
// if still there, freed.
void Handler_Stop(Handler* hp, u32 recurse, u32 force, u32 flag) {
    Handler& h = gref(hp);
    if (flag != 0 && force == 0 && (h.flags & 0x80) != 0) return;

    Snd_Lock();
    const u32 handle = h.handle;
    if ((handle & 0x80000000u) == 0) {
        Snd_LogAt(kFmtInactive, kFileSndhand, 1118);
        Snd_Unlock();
        return;
    }
    u32 state = h.flags;
    if (flag != 0) state |= 0x20;
    if (state & 0x80) {
        recurse = 1;
        force = 1;
    } else if (state & 0xE) {
        force = 1;
    }
    h.flags = state | 0x80;
    if (recurse != 0)
        for (Handler* child = h.first_child; child; child = child->next_sibling) Handler_Stop(child, recurse, force, flag);

    // (The original's checks on the handle's index and live bit here can't fail.)
    Snd_Lock();
    Handler* found = Snd_Find(handle);
    Snd_Unlock();
    if (!found) {
        Snd_LogAt(kFmtChildren, kFileSndhand, 1161);
        Snd_Unlock();
        return;
    }

    u32 stopped = 0;
    const u32 type = found->type();
    if (type == kHandlerTypeVoice) {
        stopped = Voice_StopHandler(static_cast<VoiceHandler*>(found), force);
    } else if (const HandlerType* info = global<gp<HandlerType>>(kHandlerTypes + type * 4)) {
        if (const u32 stop = info->stop) stopped = call_ptr<u32>(stop, found, force, stop);
    }
    if ((force | stopped) == 0) {
        Handler_Release(found);
        Snd_Unlock();
        return;
    }
    Handler_Detach(found);
    Handler_UnlinkTracks(found);
    Snd_Lock();
    Handler* again = Snd_Find(handle);
    Snd_Unlock();
    if (again) {
        const u32 root = (!again->parent && again->first_child) ? 1 : 0;
        Handler_Free(again, root);
    }
    Snd_Unlock();
}

}  // namespace sly4::snd

NATIVE(006EFD10, sly4::snd::Snd_Lock)
NATIVE(006EFD34, sly4::snd::Snd_Unlock)
NATIVE(006E5B6C, sly4::snd::Snd_Lookup)
NATIVE(006E6718, sly4::snd::Handler_CountActive)
NATIVE(006E620C, sly4::snd::Handler_Stop)
