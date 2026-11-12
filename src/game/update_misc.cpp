// Small functions Game_Update calls once per frame.
#include "update.h"

namespace sly4::update {
using namespace opentit::native;

namespace {

// ApeInputMgr::GetAllocator()'s function-local static: an
// ApeTFreeList<ApeInputDevice> (60 bytes) created on first use. Its list of
// live devices starts at +4; each device sits 16 bytes into its node, whose
// first word links to the next node.
constexpr u32 kInputAllocator = 0x01164AA0;     // u32 ApeTFreeList*
constexpr u32 kInputDevicesChanged = 0x01164AAC;  // u8 (name?)
constexpr u32 kFreeListVtable = 0x00B243F8;
constexpr u32 kFreeListName = 0x00AF382A;  // "static ApeTFreeList<ApeInputDevice> &ApeInputMgr::GetAllocator()"
constexpr u32 kFreeListSize = 60;
constexpr u32 kInputDeviceSize = 296;
constexpr u32 kInputDevicesPerBlock = 4;  // (name?)
constexpr u32 kDeviceNodeHeader = 16;
constexpr u32 kDeviceVfnUpdate = 16;
constexpr u32 kOperatorNew = 0x007A3DD8;    // (size) -> memory or 0 (name?)
constexpr u32 kFreeListInit = 0x00551A58;   // (list, element size, per block, 0, 0, name) (name?)
constexpr u32 kInputDevicesChangedFn = 0x0055BE90;  // (0): reads r3 only

// ApeTFreeList<T> header as GetAllocator() builds it (60 bytes).
struct InputFreeList {
    be_u32 vtable;       // +0
    be_u32 first;        // +4: first live node
    be_u32 w8[11];       // +8 .. +48
    be_u16 h52;          // +52
    be_u16 bits;         // +54: bit 0x8000 cleared
    be_u32 name;         // +56
};
static_assert(sizeof(InputFreeList) == 60);

// Flow states FlowState_GetMovieOwner checks (values of the state vfunc).
constexpr u32 kFlowVfnState = 32;  // (u32 depth) -> state id
constexpr u32 kMovieOwnerFallback = 0x010BBF58;  // u32 object used for sub-states 14 and 15 (name?)

}  // namespace

// Rounds +12 up to a 16-byte boundary from +4 (when +4 is below it) and sets
// +16 to +8 rounded down to a 16-byte boundary.  (name?)
void FrameArena_Align(FrameArena* a) {
    const u32 cur = a->cur, end = a->end, top = a->top;
    if (cur < end) a->end = (cur + 15) & ~15u;
    const u32 up = (top + 15) & ~15u;
    a->limit = up;
    if (up > top) a->limit = up - 16;
}

// Records the mode; once kMenuModeReady is set, works out the new mask (all
// bits when on, plus kMenuModeExtraBits and bit 2 when kMenuModeBit2) and
// passes the bits that turned on to sub_006D530C and those that turned off
// to sub_006D53F0.  (name?)
void Audio_SetPaused(u8 on) {
    global<u8>(kMenuMode) = on;
    const u8 ready = global<u8>(kMenuModeReady);
    global<u8>(kMenuModeDirty) = 1;
    if (ready == 0) return;

    u32 mask = on != 0 ? 0xFFFFFFFFu : 0u;
    mask |= global<be_u32>(kMenuModeExtraBits);
    if (global<u8>(kMenuModeBit2) != 0) mask |= 2;
    const u32 old = global<be_u32>(kMenuModeMask);
    const u32 changed = mask ^ old;
    const u32 turned_off = changed & old, turned_on = changed & ~old;
    if (turned_on != 0) call(ufn::sub_006D530C, turned_on);
    if (turned_off != 0) call(ufn::sub_006D53F0, turned_off);
    global<be_u32>(kMenuModeMask) = mask;
}

u8 Audio_IsPaused() { return global<u8>(kMenuMode); }

void Ui_SetInputHandler(u32 handler_opd, u32 arg) {
    global<be_u32>(kUiInputHandler) = handler_opd;
    global<be_u32>(kUiInputArg) = arg;
}

void Ui_SetEventHandler(u32 handler_opd, u32 arg) {
    global<be_u32>(kUiEventHandler) = handler_opd;
    global<be_u32>(kUiEventArg) = arg;
}

// Hands { type, a, handler arg, b, c } to the event handler and returns its
// result, or 1 when there is no handler.  (name?)
u32 Ui_SendEvent(u32 type, u32 a, u32 b, u32 c) {
    if (global<be_u32>(kUiEventHandler) == 0) return 1;
    GuestFrame frame(sizeof(UiEvent));
    UiEvent* ev = frame.as<UiEvent>();
    ev->type = type;
    ev->a = a;
    ev->handler_arg = global<be_u32>(kUiEventArg).get();
    ev->b = b;
    ev->c = c;
    return call_ptr<u32>(global<be_u32>(kUiEventHandler), ev);
}

// BinkPause on the movie's Bink handle (+272); returns what that returns.  (name?)
u32 Movie_SetPaused(Movie* movie, u8 paused) {
    return call<u32>(ufn::Bink_Pause, movie->bink.get(), static_cast<u32>(paused));
}

u32 Systems_EndFrame() {
    call(ufn::sub_0055E284);
    call(ufn::sub_0050FDC8);
    return 1;
}

// Empty in the retail build (debug hooks compiled out?).
void DebugHookA() {}
void DebugHookB() {}

// Clears the u16 at +8 of three 20-byte records; for the first, a nonzero
// value is handled by sub_000D0344 first.  (name?)
void Triggers_Clear() {
    TriggerRecord* t = &global<TriggerRecord>(kTriggers);
    if (t[0].count != 0) {
        call(ufn::sub_000D0344);
        t[0].count = 0;
    }
    for (u32 k = 1; k < 3; ++k)
        if (t[k].count != 0) t[k].count = 0;
}

// Moves to the next of the fb+56 slots (16 bytes each from +20), marks it
// (bits 1-3 of its +8 = 2) and points the write cursor (+60, +68) at its
// base and the end (+64) at base + 24 * fb+52. With fb+76 set, +12 and +16
// are first reset to fb+0.  (name?)
void FrameBuffer_Begin(FrameRingBuffer* fb) {
    const bool reset = fb->reset != 0;
    const s32 n = fb->slot_count;
    s32 index = fb->slot_index + 1;
    // divw: the recompiler (and this code) treats x / 0 as 0, leaving index + 1.
    const s32 q = n != 0 ? index / n : 0;
    index = static_cast<s32>(static_cast<u32>(index) - static_cast<u32>(q * n));
    FrameSlot* slot = host<FrameSlot>(guest(fb) + 20 + ((static_cast<u32>(index) << 4) & 0xFF0));
    fb->used = 0u;
    if (reset) {
        const u32 base = fb->base;
        fb->w16 = base;
        fb->w12 = base;
    }
    fb->slot_index = static_cast<u8>(index);
    fb->slot_index2 = static_cast<u8>(index);
    slot->flags = (slot->flags & ~0xEu) | 0x4u;
    const u32 base = slot->base;
    const u32 cursor = base + fb->used * 24;
    fb->cursor = cursor;
    fb->end = base + fb->capacity * 24;
    fb->write = cursor;
}

void FrameBuffer_End(FrameRingBuffer* fb) { fb->write = 0u; }

// Calls vfunc +16 on every live input device, then sub_0055BE90(0) once if
// kInputDevicesChanged was set. Creates the device allocator first if
// needed (as ApeInputMgr::GetAllocator() does).  (ApeInputMgr per-frame update?)
void Input_UpdateDevices() {
    u32 list = global<be_u32>(kInputAllocator);
    if (list == 0) {
        const u32 mem = call<u32>(kOperatorNew, kFreeListSize);
        if (mem != 0) {
            InputFreeList* fl = host<InputFreeList>(mem);
            fl->first = 0u;
            for (auto& w : fl->w8) w = 0u;
            fl->h52 = 0;
            fl->bits = static_cast<u16>(fl->bits & 0x7FFF);
            fl->vtable = kFreeListVtable;
            fl->name = kFreeListName;
            call(kFreeListInit, mem, kInputDeviceSize, kInputDevicesPerBlock, 0u, 0u);  // r8 (name) is not read
        }
        list = mem;
        global<be_u32>(kInputAllocator) = list;
    }
    // (With a failed allocation this reads guest address 4, as the original.)
    const u32 first = gref<be_u32>(list + 4);
    u32 device = first != 0 ? first + kDeviceNodeHeader : 0;
    while (device != 0) {
        vcall(host<u8>(device), kDeviceVfnUpdate);
        const u32 next = *host<be_u32>(device - kDeviceNodeHeader);
        device = next != 0 ? next + kDeviceNodeHeader : 0;
    }
    if (global<u8>(kInputDevicesChanged) != 0) {
        global<u8>(kInputDevicesChanged) = 0;
        call(kInputDevicesChangedFn, 0u);
    }
}

// Picks the object whose movie Game_Update pauses, from the state ids the
// flow's vfunc +32 (depth) reports: the top of the stack at flow+20 (offset
// u16 flow+6) or its bottom entry, or a global for sub-states 14/15. 0 if
// none applies.  (name?)
u32 FlowState_GetMovieOwner(Flow* flow) {
    auto state = [&](u32 depth) { return vcall<s32>(flow, kFlowVfnState, depth); };
    auto top = [&] { return gref<be_u32>(flow->stack.ea.get() + flow->stack_top).get(); };
    auto bottom = [&] { return gref<be_u32>(flow->stack.ea.get()).get(); };

    if (state(1) == 12) return top();
    if (state(0) == 2) return bottom();
    if (state(0) == 3) return bottom();
    if (state(0) == 0 && state(1) == 6) return top();
    if (state(0) == 0) {
        bool check_top = state(1) == 25;
        if (!check_top) check_top = state(1) == 24;
        if (check_top && state(2) > 0) return top();
    }
    const u8 sub = flow->sub_state;
    if (sub != 14 && sub != 15) return 0;
    if (flow->b32 == 0) return 0;
    return global<be_u32>(kMovieOwnerFallback);
}

// Steps the random generator, counts the frame and sets the frame time
// (SystemState +4) to config +0x44 clamped to [0, 4 / SystemState +12],
// adding it to the running total at +16; then aligns the main heap's
// pointers unless SystemState +32 bit 0x40 is set. The original inlines
// SystemState's guard check a few times over but only the first can fire.  (name?)
u32 Time_BeginFrame() {
    Random_Step();
    SystemState* s = host<SystemState>(system_state::get_native());
    s->frame_count = s->frame_count + 1;
    const f32 raw = gref<be_f32>(g::config + kConfigFrameTime);
    const f32 limit = kMaxFrameTime / s->f12;
    f32 dt = raw < limit ? raw : limit;  // a NaN raw gives the limit
    if (!(dt > 0.0f)) dt = 0.0f;
    s->frame_time = dt;
    s->total_time = s->total_time + dt;
    if ((s->flags & 0x40) == 0) FrameArena_Align(s->heap);
    return 1;
}

}  // namespace sly4::update

NATIVE(0055329C, sly4::update::FrameArena_Align)
NATIVE(0044BA6C, sly4::update::Audio_SetPaused)
NATIVE(0044BB3C, sly4::update::Audio_IsPaused)
NATIVE(004D5048, sly4::update::Ui_SetInputHandler)
NATIVE(004D505C, sly4::update::Ui_SetEventHandler)
NATIVE(004D3DA4, sly4::update::Ui_SendEvent)
NATIVE(004D2284, sly4::update::Movie_SetPaused)
NATIVE(005DBB30, sly4::update::Systems_EndFrame)
NATIVE(004A8AC8, sly4::update::DebugHookA)
NATIVE(004A8ACC, sly4::update::DebugHookB)
NATIVE(000D0628, sly4::update::Triggers_Clear)
NATIVE(0050FF64, sly4::update::FrameBuffer_Begin)
NATIVE(00510544, sly4::update::FrameBuffer_End)
NATIVE(005630D4, sly4::update::Input_UpdateDevices)
NATIVE(00458D6C, sly4::update::FlowState_GetMovieOwner)
NATIVE(005DB6B8, sly4::update::Time_BeginFrame)
