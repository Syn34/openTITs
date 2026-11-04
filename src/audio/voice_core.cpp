// Handle layer of the sound library: lock, lookup, count and stop.
#include "voice_layer.h"

using namespace sly4;
using namespace sly4::snd;

// Snd_Lock / Snd_Unlock: the library's global mutex. r3 is passed through
// unused; the result is the mutex call's.
DECOMP(006EFD10, Snd_Lock) { call(ctx, sfn::Snd_LockImpl); }
DECOMP(006EFD34, Snd_Unlock) { call(ctx, sfn::Snd_UnlockImpl); }

// Handler* Snd_Lookup(u32 handle)
// 0 for a handle without the live bit. (The original's range check on the
// index can't fail: it is 8 bits.)
DECOMP(006E5B6C, Snd_Lookup) {
    const u32 handle = arg<u32>(ctx, 0);
    if ((handle & 0x80000000u) == 0) return ret(ctx, 0u);
    call(ctx, sfn::Snd_Lock, {1});  // r3 holds the live bit here (unused)
    const u32 p = Snd_Find(handle);
    call(ctx, sfn::Snd_Unlock);
    ret(ctx, p);
}

// u32 Handler_CountActive(Handler* h)
// Counts type-4 handlers with +581 set in h and all its descendants.
DECOMP(006E6718, Handler_CountActive) {
    const u32 h = arg<u32>(ctx, 0);
    const u32 type = (load<u32>(h) >> 24) & 0x1F;
    u32 child = load<u32>(h + kHFirstChild);
    u32 count = (type == 4 && ref<u8>(h + kHType4Active) != 0) ? 1 : 0;
    while (child != 0) {
        count += static_cast<u32>(call(ctx, sfn::Handler_CountActive, {child}));
        child = load<u32>(child + kHNextSibling);
    }
    ret(ctx, count);
}

// void Handler_Stop(Handler* h, bool recurse, bool force, bool flag)
//
// Marks the handler as stopping (0x80; 0x20 too with `flag`), stops its
// children first when recursing (an already stopping handler always
// recurses and forces), then stops it through its type: the voice stop for
// type 5, the type's stop function otherwise. If nothing was stopped and it
// wasn't forced, it is released; otherwise it is detached, finished and,
// if still there, freed.
DECOMP(006E620C, Handler_Stop) {
    u32 h = arg<u32>(ctx, 0);
    u32 recurse = arg<u32>(ctx, 1), force = arg<u32>(ctx, 2);
    const u32 flag = arg<u32>(ctx, 3);
    if (flag != 0 && force == 0 && (load<u32>(h + kHFlags) & 0x80) != 0) return;

    call(ctx, sfn::Snd_Lock, {h, recurse, force, flag});
    const u32 handle = load<u32>(h);
    if ((handle & 0x80000000u) == 0) {
        Snd_Log(ctx, kFmtInactive, kFileSndhand, 1118);
        call(ctx, sfn::Snd_Unlock);
        return;
    }
    u32 state = load<u32>(h + kHFlags);
    if (flag != 0) state |= 0x20;
    if (state & 0x80) {
        recurse = 1;
        force = 1;
    } else if (state & 0xE) {
        force = 1;
    }
    store<u32>(h + kHFlags, state | 0x80);
    if (recurse != 0)
        for (u32 child = load<u32>(h + kHFirstChild); child != 0; child = load<u32>(child + kHNextSibling))
            call(ctx, sfn::Handler_Stop, {child, recurse, force, flag});

    // (The original's checks on the handle's index and live bit here can't fail.)
    call(ctx, sfn::Snd_Lock);
    h = Snd_Find(handle);
    call(ctx, sfn::Snd_Unlock);
    if (h == 0) {
        Snd_Log(ctx, kFmtChildren, kFileSndhand, 1161);
        call(ctx, sfn::Snd_Unlock);
        return;
    }

    u32 stopped = 0;
    const u32 type = (load<u32>(h) >> 24) & 0x1F;
    if (type == kHandlerTypeVoice) {
        stopped = static_cast<u32>(call(ctx, sfn::Voice_Stop5, {h, force, flag}));
    } else if (const u32 info = load<u32>(kHandlerTypes + type * 4)) {
        if (const u32 stop = load<u32>(info + 32)) stopped = static_cast<u32>(call_ptr(ctx, stop, {h, force, stop}));
    }
    if ((force | stopped) == 0) {
        call(ctx, sfn::Handler_Release, {h});
        call(ctx, sfn::Snd_Unlock);
        return;
    }
    call(ctx, sfn::Handler_Detach, {h});
    call(ctx, sfn::Handler_Finish, {h});
    call(ctx, sfn::Snd_Lock);
    const u32 again = Snd_Find(handle);
    call(ctx, sfn::Snd_Unlock);
    if (again != 0) {
        const u32 root = (load<u32>(again + kHParent) == 0 && load<u32>(again + kHFirstChild) != 0) ? 1 : 0;
        call(ctx, sfn::Handler_Free, {again, root});
    }
    call(ctx, sfn::Snd_Unlock);
}
