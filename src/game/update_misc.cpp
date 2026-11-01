// Small functions Game_Update calls once per frame.
#include "update.h"

using namespace sly4;
using namespace sly4::update;

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
constexpr u32 kInputDevicesChangedFn = 0x0055BE90;

// Flow states FlowState_GetMovieOwner checks (values of the state vfunc).
constexpr u32 kFlowVfnState = 32;  // (u32 depth) -> state id
constexpr u32 kMovieOwnerFallback = 0x010BBF58;  // u32 object used for sub-states 14 and 15 (name?)

}  // namespace

// void FrameArena_Align(Arena* arena)  (name?)
// Rounds +12 up to a 16-byte boundary from +4 (when +4 is below it) and sets
// +16 to +8 rounded down to a 16-byte boundary.
DECOMP(0055329C, FrameArena_Align) {
    const u32 a = arg<u32>(ctx, 0);
    const u32 cur = field<u32>(a, 4), end = field<u32>(a, 12), top = field<u32>(a, 8);
    if (cur < end) field<u32>(a, 12) = (cur + 15) & ~15u;
    const u32 up = (top + 15) & ~15u;
    field<u32>(a, 16) = up;
    if (up > top) field<u32>(a, 16) = up - 16;
}

// void Audio_SetPaused(bool on)  (name?)
// Records the mode; once kMenuModeReady is set, works out the new mask (all
// bits when on, plus kMenuModeExtraBits and bit 2 when kMenuModeBit2) and
// passes the bits that turned on to sub_006D530C and those that turned off
// to sub_006D53F0.
DECOMP(0044BA6C, Audio_SetPaused) {
    const u8 on = arg<u8>(ctx, 0);
    ref<u8>(kMenuMode) = on;
    const u8 ready = ref<u8>(kMenuModeReady);
    ref<u8>(kMenuModeDirty) = 1;
    if (ready == 0) return;

    u32 mask = on != 0 ? 0xFFFFFFFFu : 0u;
    mask |= load<u32>(kMenuModeExtraBits);
    if (ref<u8>(kMenuModeBit2) != 0) mask |= 2;
    const u32 old = load<u32>(kMenuModeMask);
    const u32 changed = mask ^ old;
    const u32 turned_off = changed & old, turned_on = changed & ~old;
    if (turned_on != 0) call(ctx, ufn::sub_006D530C, {turned_on});
    if (turned_off != 0) call(ctx, ufn::sub_006D53F0, {turned_off});
    store<u32>(kMenuModeMask, mask);
}

// u8 Audio_IsPaused()  (name?)
DECOMP(0044BB3C, Audio_IsPaused) { ret(ctx, static_cast<u32>(ref<u8>(kMenuMode))); }

// void Ui_SetInputHandler(opd handler, u32 arg)  (name?)
DECOMP(004D5048, Ui_SetInputHandler) {
    store<u32>(kUiInputHandler, arg<u32>(ctx, 0));
    store<u32>(kUiInputArg, arg<u32>(ctx, 1));
}

// void Ui_SetEventHandler(opd handler, u32 arg)  (name?)
DECOMP(004D505C, Ui_SetEventHandler) {
    store<u32>(kUiEventHandler, arg<u32>(ctx, 0));
    store<u32>(kUiEventArg, arg<u32>(ctx, 1));
}

// u32 Ui_SendEvent(u32 type, u32 a, u32 b, u32 c)  (name?)
// Hands { type, a, handler arg, b, c } to the event handler and returns its
// result, or 1 when there is no handler.
DECOMP(004D3DA4, Ui_SendEvent) {
    const u32 type = arg<u32>(ctx, 0), a = arg<u32>(ctx, 1), b = arg<u32>(ctx, 2), c = arg<u32>(ctx, 3);
    if (load<u32>(kUiEventHandler) == 0) return ret(ctx, 1u);
    Stack frame(ctx, sizeof(UiEvent));
    auto& ev = ref<UiEvent>(frame.addr());
    ev.type = type;
    ev.a = a;
    ev.handler_arg = load<u32>(kUiEventArg);
    ev.b = b;
    ev.c = c;
    ret(ctx, call_ptr(ctx, load<u32>(kUiEventHandler), {frame.addr()}));
}

// Movie_SetPaused(Movie* movie, bool paused)  (name?)
// BinkPause on the movie's Bink handle (+272); returns what that returns.
DECOMP(004D2284, Movie_SetPaused) {
    const u32 movie = arg<u32>(ctx, 0);
    const u32 paused = arg<u32>(ctx, 1) & 0xFF;
    ret(ctx, call(ctx, ufn::Bink_Pause, {load<u32>(movie + 272), paused}));
}

// int Systems_EndFrame()  (name?)
DECOMP(005DBB30, Systems_EndFrame) {
    call(ctx, ufn::sub_0055E284);
    call(ctx, ufn::sub_0050FDC8);
    ret(ctx, 1u);
}

// Empty in the retail build (debug hooks compiled out?).
DECOMP(004A8AC8, DebugHookA) {}
DECOMP(004A8ACC, DebugHookB) {}

// void Triggers_Clear()  (name?)
// Clears the u16 at +8 of three 20-byte records; for the first, a nonzero
// value is handled by sub_000D0344 first.
DECOMP(000D0628, Triggers_Clear) {
    if (load<u16>(kTriggers + 8) != 0) {
        call(ctx, ufn::sub_000D0344);
        store<u16>(kTriggers + 8, 0);
    }
    for (u32 k = 1; k < 3; ++k) {
        const u32 count = kTriggers + k * kTriggerStride + 8;
        if (load<u16>(count) != 0) store<u16>(count, 0);
    }
}

// void FrameBuffer_Begin(FrameBuffer* fb)  (name?)
// Moves to the next of the fb+56 slots (16 bytes each from +20), marks it
// (bits 1-3 of its +8 = 2) and points the write cursor (+60, +68) at its
// base and the end (+64) at base + 24 * fb+52. With fb+76 set, +12 and +16
// are first reset to fb+0.
DECOMP(0050FF64, FrameBuffer_Begin) {
    const u32 fb = arg<u32>(ctx, 0);
    const bool reset = field<u8>(fb, 76) != 0;
    const s32 n = field<u8>(fb, 56);
    s32 index = field<u8>(fb, 57) + 1;
    // divw: the recompiler (and this code) treats x / 0 as 0, leaving index + 1.
    const s32 q = n != 0 ? index / n : 0;
    index = static_cast<s32>(static_cast<u32>(index) - static_cast<u32>(q * n));
    const u32 slot = fb + 20 + ((static_cast<u32>(index) << 4) & 0xFF0);
    field<u32>(fb, 72) = 0;
    if (reset) {
        const u32 base = field<u32>(fb, 0);
        field<u32>(fb, 16) = base;
        field<u32>(fb, 12) = base;
    }
    field<u8>(fb, 57) = static_cast<u8>(index);
    field<u8>(fb, 58) = static_cast<u8>(index);
    field<u32>(slot, 8) = (field<u32>(slot, 8) & ~0xEu) | 0x4u;
    const u32 base = field<u32>(slot, 0);
    const u32 cursor = base + field<u32>(fb, 72) * 24;
    field<u32>(fb, 60) = cursor;
    field<u32>(fb, 64) = base + field<u32>(fb, 52) * 24;
    field<u32>(fb, 68) = cursor;
}

// void FrameBuffer_End(FrameBuffer* fb)  (name?)
DECOMP(00510544, FrameBuffer_End) { field<u32>(arg<u32>(ctx, 0), 68) = 0; }

// void Input_UpdateDevices()  (ApeInputMgr per-frame update?)
// Calls vfunc +16 on every live input device, then sub_0055BE90(0, ...)
// once if kInputDevicesChanged was set. Creates the device allocator first
// if needed (as ApeInputMgr::GetAllocator() does).
DECOMP(005630D4, Input_UpdateDevices) {
    u32 list = load<u32>(kInputAllocator);
    if (list == 0) {
        const u32 mem = static_cast<u32>(call(ctx, kOperatorNew, {kFreeListSize}));
        if (mem != 0) {
            for (const u32 off : {8u, 4u, 12u, 20u, 16u, 24u}) field<u32>(mem, off) = 0;
            u16 bits = field<u16>(mem, 54);
            for (const u32 off : {32u, 28u, 36u}) field<u32>(mem, off) = 0;
            bits &= 0x7FFF;
            for (const u32 off : {40u, 44u, 48u}) field<u32>(mem, off) = 0;
            field<u16>(mem, 52) = 0;
            field<u16>(mem, 54) = bits;
            field<u32>(mem, 0) = kFreeListVtable;
            field<u32>(mem, 56) = kFreeListName;
            call(ctx, kFreeListInit, {mem, kInputDeviceSize, kInputDevicesPerBlock, 0, 0, kFreeListName});
        }
        list = mem;
        store<u32>(kInputAllocator, list);
    }
    // (With a failed allocation this reads guest address 4, as the original.)
    const u32 first = load<u32>(list + 4);
    u32 device = first != 0 ? first + kDeviceNodeHeader : 0;
    while (device != 0) {
        call_ptr(ctx, load<u32>(load<u32>(device) + kDeviceVfnUpdate), {device});
        const u32 next = load<u32>(device - kDeviceNodeHeader);
        device = next != 0 ? next + kDeviceNodeHeader : 0;
    }
    if (ref<u8>(kInputDevicesChanged) != 0) {
        ref<u8>(kInputDevicesChanged) = 0;
        call(ctx, kInputDevicesChangedFn, {0, kInputAllocator});  // r4 is left holding the static's address
    }
}

// Object* FlowState_GetMovieOwner(Flow* flow)  (name?)
// Picks the object whose movie Game_Update pauses, from the state ids the
// flow's vfunc +32 (depth) reports: the top of the stack at flow+20 (offset
// u16 flow+6) or its bottom entry, or a global for sub-states 14/15. 0 if
// none applies.
DECOMP(00458D6C, FlowState_GetMovieOwner) {
    const u32 flow = arg<u32>(ctx, 0);
    auto state = [&](u32 depth) {
        return static_cast<s32>(call_ptr(ctx, load<u32>(load<u32>(flow) + kFlowVfnState), {flow, depth}));
    };
    auto top = [&] { return load<u32>(field<u32>(flow, 20) + field<u16>(flow, 6)); };
    auto bottom = [&] { return load<u32>(field<u32>(flow, 20)); };

    if (state(1) == 12) return ret(ctx, top());
    if (state(0) == 2) return ret(ctx, bottom());
    if (state(0) == 3) return ret(ctx, bottom());
    if (state(0) == 0 && state(1) == 6) return ret(ctx, top());
    if (state(0) == 0) {
        bool check_top = state(1) == 25;
        if (!check_top) check_top = state(1) == 24;
        if (check_top && state(2) > 0) return ret(ctx, top());
    }
    const u8 sub = field<u8>(flow, 31);
    if (sub != 14 && sub != 15) return ret(ctx, 0u);
    if (field<u8>(flow, 32) == 0) return ret(ctx, 0u);
    ret(ctx, load<u32>(kMovieOwnerFallback));
}

// int Time_BeginFrame()  (name?)
// Steps the random generator, counts the frame and sets the frame time
// (SystemState +4) to config +0x44 clamped to [0, 4 / SystemState +12],
// adding it to the running total at +16; then aligns the main heap's
// pointers unless SystemState +32 bit 0x40 is set. The original inlines
// SystemState's guard check a few times over but only the first can fire.
DECOMP(005DB6B8, Time_BeginFrame) {
    Random_Step();
    const u32 s = system_state::get(ctx);
    field<u32>(s, kSystemStateFrameCount) += 1;
    const f32 raw = field<f32>(g::config, kConfigFrameTime);
    const f32 limit = kMaxFrameTime / field<f32>(s, 12);
    f32 dt = raw < limit ? raw : limit;  // a NaN raw gives the limit
    if (!(dt > 0.0f)) dt = 0.0f;
    field<f32>(s, kSystemStateFrameTime) = dt;
    field<f32>(s, kSystemStateTotalTime) = field<f32>(s, kSystemStateTotalTime) + dt;
    if ((field<u8>(s, kSystemStateFlags) & 0x40) == 0)
        call(ctx, ufn::FrameArena_Align, {field<u32>(s, kSystemStateHeap)});
    ret(ctx, 1u);
}
