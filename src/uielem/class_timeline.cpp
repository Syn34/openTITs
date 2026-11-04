// Timeline key / track class slots (vtables 0x00B228B0 .. 0x00B22908).
#include "class_timeline.h"

using namespace sly4;
using namespace sly4::uielem;

namespace {

// A destructor of these classes only resets the vtable to its base's.
void SetVtbl(PPUContext& ctx, u32 obj, u32 vtbl) {
    store<u32>(obj, vtbl);
    ctx.gpr[4] = vtbl;  // r4 as the original leaves it
}

// The deleting destructors: reset the vtable, free with pool 0.
void DeleteAs(PPUContext& ctx, u32 obj, u32 vtbl) {
    if (obj == 0) return;
    store<u32>(obj, vtbl);
    call(ctx, clfn::Mem_FreeFrom, {obj, 0, obj, vtbl});  // r5 / r6 as in the original
}

}  // namespace

DECOMP(00795EBC, TlKey_Destruct) { SetVtbl(ctx, arg<u32>(ctx, 0), kTlKeyVtbl); }
DECOMP(00795ECC, TlKey_Delete) { DeleteAs(ctx, arg<u32>(ctx, 0), kTlKeyVtbl); }
DECOMP(00795F10, TlTrack_Destruct) { SetVtbl(ctx, arg<u32>(ctx, 0), kTlTrackVtbl); }
DECOMP(00795F20, TlTrack_Delete) { DeleteAs(ctx, arg<u32>(ctx, 0), kTlTrackVtbl); }

// The derived keys add nothing to destroy: their destructors go straight
// to the base key's vtable.
DECOMP(00795F64, TlKeyNop_Destruct) { SetVtbl(ctx, arg<u32>(ctx, 0), kTlKeyVtbl); }
DECOMP(00795F74, TlKeyNop_Delete) { DeleteAs(ctx, arg<u32>(ctx, 0), kTlKeyVtbl); }
DECOMP(00795FC0, TlKeyAnim_Destruct) { SetVtbl(ctx, arg<u32>(ctx, 0), kTlKeyVtbl); }
DECOMP(00795FD0, TlKeyAnim_Delete) { DeleteAs(ctx, arg<u32>(ctx, 0), kTlKeyVtbl); }
DECOMP(00796058, TlKeyEvent_Destruct) { SetVtbl(ctx, arg<u32>(ctx, 0), kTlKeyVtbl); }
DECOMP(00796068, TlKeyEvent_Delete) { DeleteAs(ctx, arg<u32>(ctx, 0), kTlKeyVtbl); }

// Track* TlKeyNop_Run(Key*, Timeline*; f1)  [+8]
DECOMP(00795FB8, TlKeyNop_Run) { ret(ctx, 0); }

// Track* TlKeyAnim_Run(Key* key, Timeline* tl; f1 = delay)  [+8]
// Finds the key's element under the timeline's and starts a track on it.
DECOMP(004E3CAC, TlKeyAnim_Run) {
    const u32 key = arg<u32>(ctx, 0), tl = arg<u32>(ctx, 1);
    const u32 elem = static_cast<u32>(call(ctx, tlfn::Ui_Find, {key + kKeyAnimPath, load<u32>(tl + kTlElement)}));
    if (elem == 0) return ret(ctx, 0u);
    const u32 track = static_cast<u32>(call(ctx, cfn::SmallAlloc, {kTlTrackSize}));
    if (track == 0) return ret(ctx, 0u);
    // f1 (the delay) reaches the constructor untouched, as in the original.
    call(ctx, tlfn::TlTrack_Construct, {track, key, tl, elem});
    ret(ctx, track);
}

// Track* TlKeyEvent_Run(Key* key, Timeline* tl; f1)  [+8]
// Sends event +12 to every user (-1) for the key's element, with the
// timeline as the other object. Never starts a track.
DECOMP(004E4B40, TlKeyEvent_Run) {
    const u32 key = arg<u32>(ctx, 0), tl = arg<u32>(ctx, 1);
    if (load<u32>(key + kKeyEventType) == 0) return ret(ctx, 0u);
    const u32 elem = static_cast<u32>(call(ctx, tlfn::Ui_Find, {key + kKeyEventPath, load<u32>(tl + kTlElement)}));
    const u32 type = load<u32>(key + kKeyEventType);
    const u32 handler = load<u32>(kUiEventHandlerOpd);
    if (handler != 0) {
        Stack frame(ctx, 32);
        const u32 ev = frame.addr();
        store<u32>(ev + 0, type);
        store<u32>(ev + 4, 0xFFFFFFFFu);
        store<u32>(ev + 8, load<u32>(kUiEventHandlerArg));
        store<u32>(ev + 12, elem);
        store<u32>(ev + 16, tl);
        // r4..r7 as the original leaves them: element, handler arg, handler OPD, 0x00F60000.
        call_ptr(ctx, handler, {ev, elem, load<u32>(kUiEventHandlerArg), handler, 0x00F60000});
    }
    ret(ctx, 0u);
}
