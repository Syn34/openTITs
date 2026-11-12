// Small functions World_SubmitFrame calls.
#include "opentit/native.h"
#include "../sly4.h"
#include "frame.h"

namespace sly4::frame {
using namespace opentit::native;

constexpr u32 kOverlayValue = 0x00F558D0;  // u32 (name?)
constexpr u32 kMarkerCommand = 22;         // command id in the top byte


struct MarkerCommand {
    be_u32 head;    // kMarkerCommand << 24 | (value & 0xFF0000)
    be_u32 low;     // value & 0xFFFF
    be_u32 zero;
};


// u32 Overlay_GetValue()  (name?)
u32 Overlay_GetValue() { return global<be_u32>(kOverlayValue); }

// True when the camera's +48 object has bit 0x8000 set in its +164 word.  (name?)
bool Camera_IsSplit(const Camera* camera) {
    const SplitFlags* obj = camera->split;
    return obj && (obj->flags & 0x8000) != 0;
}

// out->mark = arena->mark; out->count = (arena->top - out->base) / 8 (signed,
// truncating); then arena flags: clear 0x40, set 0x80.  (name?)
void Arena_Snapshot(Arena* arena, ArenaSnapshot* out) {
    const s32 diff = static_cast<s32>(arena->top - out->base);
    out->mark = arena->mark.get();
    out->count = diff / 8;  // srawi + addze
    arena->flags = static_cast<u8>((arena->flags & ~0x40) | 0x80);
}

// Writes a 12-byte marker command at the cursor (wrapping to the start when
// fewer than 16 bytes are left before the end, then 4-byte aligned).  (name?)
void FrameBuffer_PushMarker(FrameBuffer* fb, u32 /*unused*/, u32 value) {
    u32 cursor = fb->cursor;
    if (cursor + 15 > fb->end) cursor = fb->begin;
    cursor = (cursor + 3) & ~3u;
    fb->cursor = cursor;
    MarkerCommand* cmd = host<MarkerCommand>(cursor);
    cmd->zero = 0u;
    cmd->head = (kMarkerCommand << 24) | (value & 0x00FF0000);
    cmd->low = value & 0xFFFF;
    fb->cursor += 12;
}

// Calls vfunc +32 of the owner's target with the view and returns its
// result, or the view itself when there is no target.  (name?)
u32 View_Notify(ViewOwner* owner, void* view) {
    ViewTarget* target = owner->target;
    if (!target) return guest(view);
    return vcall<u32>(target, 32, view);
}

}  // namespace sly4::frame

NATIVE(000AB93C, sly4::frame::Overlay_GetValue)
NATIVE(002A6044, sly4::frame::Camera_IsSplit)
NATIVE(00565178, sly4::frame::Arena_Snapshot)
NATIVE(005220E8, sly4::frame::FrameBuffer_PushMarker)
NATIVE(0001BEE8, sly4::frame::View_Notify)
