// Small functions World_SubmitFrame calls.
#include "frame.h"

using namespace sly4;
using namespace sly4::frame;

namespace {
constexpr u32 kOverlayValue = 0x00F558D0;  // u32 (name?)
constexpr u32 kMarkerCommand = 22;         // command id in the top byte
}  // namespace

// u32 Overlay_GetValue()  (name?)
DECOMP(000AB93C, Overlay_GetValue) { ret(ctx, load<u32>(kOverlayValue)); }

// bool Camera_IsSplit(Camera* camera)  (name?)
// True when the camera's +48 object has bit 0x8000 set in its +164 word.
DECOMP(002A6044, Camera_IsSplit) {
    const u32 obj = field<u32>(arg<u32>(ctx, 0), 48);
    const bool split = obj != 0 && (field<u32>(obj, 164) & 0x8000) != 0;
    ret(ctx, split ? 1u : 0u);
}

// void Arena_Snapshot(Arena* arena, Out* out)  (name?)
// out+8 = arena+28; out+4 = (arena+12 - out+0) / 8 (signed, truncating);
// then arena+32: clear 0x40, set 0x80.
DECOMP(00565178, Arena_Snapshot) {
    const u32 arena = arg<u32>(ctx, 0), out = arg<u32>(ctx, 1);
    const s32 diff = static_cast<s32>(field<u32>(arena, 12) - field<u32>(out, 0));
    field<u32>(out, 8) = field<u32>(arena, 28);
    field<s32>(out, 4) = diff / 8;  // srawi + addze
    field<u8>(arena, 32) = static_cast<u8>((field<u8>(arena, 32) & ~0x40) | 0x80);
}

// void FrameBuffer_PushMarker(FrameBuffer* fb, -, u32 value)  (name?)
// Writes a 12-byte command at the cursor (fb+16, wrapping to fb+0 when
// fewer than 16 bytes are left before fb+4, then 4-byte aligned):
// { 22 << 24 | (value & 0xFF0000), value & 0xFFFF, 0 }.
DECOMP(005220E8, FrameBuffer_PushMarker) {
    const u32 fb = arg<u32>(ctx, 0);
    const u32 value = arg<u32>(ctx, 2);
    u32 cursor = field<u32>(fb, 16);
    if (cursor + 15 > field<u32>(fb, 4)) cursor = field<u32>(fb, 0);
    cursor = (cursor + 3) & ~3u;
    field<u32>(fb, 16) = cursor;
    store<u32>(cursor + 8, 0);
    store<u32>(cursor, (kMarkerCommand << 24) | (value & 0x00FF0000));
    store<u32>(cursor + 4, value & 0xFFFF);
    field<u32>(fb, 16) = field<u32>(fb, 16) + 12;
}

// u32 View_Notify(Obj* obj, View* view)  (name?)
// Calls vfunc +32 of the object at obj+36 with the view; returns its
// result, or the view itself when there is no object (r3 is left as is).
DECOMP(0001BEE8, View_Notify) {
    const u32 obj = arg<u32>(ctx, 0), view = arg<u32>(ctx, 1);
    const u32 target = field<u32>(obj, 36);
    if (target == 0) return ret(ctx, view);
    ret(ctx, call_ptr(ctx, vfunc(target, 32), {target, view}));
}
