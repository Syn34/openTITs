// Names for the UI update (Ui_Update 0x004D4538 and its helpers). Addresses
// are guest addresses in the BCES01284 v01.02 EBOOT; "?" marks a guess.
#pragma once

#include <bit>
#include <cmath>

#include <cstddef>

#include "opentit/native.h"
#include "../sly4.h"

namespace sly4 {
namespace ui {

// ---------------------------------------------------------------- PPU float ops
// Single-precision forms round the double result to float, as the
// recompiler's rt::to_single does.
inline f32 fadds(f64 a, f64 b) { return static_cast<f32>(a + b); }
inline f32 fsubs(f64 a, f64 b) { return static_cast<f32>(a - b); }
inline f32 fmuls(f64 a, f64 c) { return static_cast<f32>(a * c); }
inline f32 fdivs(f64 a, f64 b) { return static_cast<f32>(a / b); }
inline f32 fsqrts(f64 a) { return static_cast<f32>(std::sqrt(a)); }
inline f32 fmadds(f64 a, f64 c, f64 b) { return static_cast<f32>(std::fma(a, c, b)); }  // a * c + b
inline f64 fsel(f64 a, f64 c, f64 b) { return a >= 0.0 ? c : b; }                        // a >= 0 ? c : b

// The function descriptor in slot `offset` of obj's vtable (register-style code).
inline u32 vfunc(u32 obj, u32 offset) { return load<u32>(load<u32>(obj) + offset); }

// ---------------------------------------------------------------- functions
namespace ufn {
constexpr u32 Ui_UpdateTimeline = 0x004D3B00;   // (Timeline*, f32 dt) -> bool, ui/timeline.cpp
constexpr u32 Ui_FindAtPoint = 0x004D3E60;      // (Element*, user, const f32 pt[2], f32* best) -> Element* (decompiled: src/obj/vmx2_ui.cpp)
constexpr u32 Ui_FindInDirection = 0x004D4178;  // (Element*, user, Element* focus, const f32 dir[2], f32* best) -> Element*
constexpr u32 Math_Acos = 0x005E649C;           // f32 (f32) (name?)
}  // namespace ufn

// ---------------------------------------------------------------- globals
constexpr u32 kUiRoots = 0x00F66868;        // u32 Element*: top-level elements, linked by +156
constexpr u32 kUiInputHandler = 0x00F6686C, kUiInputArg = 0x00F66870;  // see Ui_SetInputHandler
constexpr u32 kUiEventHandler = 0x00F66874, kUiEventArg = 0x00F66878;  // see Ui_SetEventHandler
constexpr u32 kUiNavEnabled = 0x00F66880;   // u8: d-pad / stick moves the focus
constexpr u32 kUiNavButtons = 0x00F66888;   // u32: the button bits that navigate
constexpr u32 kUiTimelines = 0x00F668A0;    // u32 Timeline*, linked by +28
constexpr u32 kUiFocus = 0x010C2AD8;        // u32 Element*[kUiUsers]: focused element per user
constexpr u32 kUiUsers = 4;
constexpr u32 kUiNavPoint = 0x01164AFC;     // f32[2] used as the offset when nothing is focused (name?)

// Direction for each combination of the four direction bits (1 up, 2 down,
// 4 left, 8 right): a function-local static built on first use.
constexpr u32 kUiNavDirs = 0x010C2B40;      // f32[16][2]
constexpr u32 kUiNavDirsGuard = 0x010C2B38; // u8
constexpr u32 kFltMaxBits = 0x7F7FFFFF;     // FLT_MAX, the starting "best" score

// ---------------------------------------------------------------- input / events
// What the input handler fills in for a user (passed as (user, UiInput*)).
struct UiInput {
    be_u32 arg;      // kUiInputArg
    be_u32 buttons;  // buttons down? (name?)
    be_u32 pressed;  // buttons that just went down? and-ed with buttons (name?)
    be_u32 flags;    // 0x30000: the pointer position below is valid
    be_f32 x, y;     // pointer in [-1, 1]
};
static_assert(sizeof(UiInput) == 24);

constexpr u32 kInputPointerValid = 0x30000;

// The block given to the event handler (same layout as update::UiEvent).
struct UiEventBlock {
    be_u32 type;
    be_u32 user;
    be_u32 handler_arg;  // kUiEventArg
    be_u32 element;
    be_u32 other;
};
static_assert(sizeof(UiEventBlock) == 20);

enum UiEventType : u32 {
    kUiEventFocus = 1,        // element gained the user's focus
    kUiEventUnfocus = 2,      // element lost it
    kUiEventCanFocus = 3,     // may the search pick this element? (handler result)
    kUiEventNavigated = 4,    // the focus moved by direction (other = old focus)
    kUiEventButton80 = 5,     // button bit 0x80 pressed (element = focus) (name?)
    kUiEventButton40 = 6,     // bit 0x40
    kUiEventButton10 = 7,     // bit 0x10
    kUiEventButton20 = 8,     // bit 0x20
    kUiEventButton4000 = 9,   // bit 0x4000
    kUiEventButton8 = 10,     // bit 0x8 (of 0xC)
    kUiEventButton4 = 11,     // bit 0x4 without 0x8
};

// Sends an event if there is an event handler; returns its result (0 without one).
inline u32 Ui_Notify(PPUContext& ctx, u32 type, u32 user, u32 element, u32 other) {
    if (load<u32>(kUiEventHandler) == 0) return 0;
    Stack frame(ctx, sizeof(UiEventBlock));
    auto& ev = ref<UiEventBlock>(frame.addr());
    ev.type = type;
    ev.user = user;
    ev.handler_arg = load<u32>(kUiEventArg);
    ev.element = element;
    ev.other = other;
    return static_cast<u32>(call_ptr(ctx, load<u32>(kUiEventHandler), {frame.addr()}));
}

// Native form of Ui_Notify.
inline u32 Ui_Notify(u32 type, u32 user, u32 element, u32 other) {
    using namespace opentit::native;
    const u32 handler = global<be_u32>(kUiEventHandler);
    if (handler == 0) return 0;
    GuestFrame frame(sizeof(UiEventBlock));
    UiEventBlock* ev = frame.as<UiEventBlock>();
    ev->type = type;
    ev->user = user;
    ev->handler_arg = global<be_u32>(kUiEventArg).get();
    ev->element = element;
    ev->other = other;
    return call_ptr<u32>(handler, ev);
}

// ---------------------------------------------------------------- objects
// A UI element (vtable at +0: +152 update(f32 dt), +156 render(),
// +160 transform(out a, out b, out f32[16])).
struct Element {
    be_u32 vtable;                  // +0
    native::pad<132> _4;
    be_u32 flags;                   // +136: kElemActive, kElemFocusable
    native::pad<12> _140;
    native::gp<Element> first_child;  // +152
    native::gp<Element> next;         // +156
};
static_assert(offsetof(Element, flags) == 136 && offsetof(Element, next) == 156);

// A timeline's template, keys and tracks (vtables at +0: +4 delete(),
// +8 run: key (Timeline*, f32 delay) -> Track*, track (f32 dt) -> keep running?).
struct TlKey {
    be_u32 vtable;            // +0
    be_f32 delay;             // +4
    native::gp<TlKey> next;   // +8
};
struct TlTrack {
    be_u32 vtable;              // +0
    native::pad<12> _4;
    native::gp<TlTrack> next;   // +16
};
struct TlTemplate {
    native::pad<44> _0;
    u8 loops;                       // +44
    native::pad<3> _45;
    native::gp<TlKey> first_key;    // +48
};
struct Timeline {
    be_u32 vtable;                    // +0
    native::gp<TlTemplate> templ;     // +4
    native::pad<4> _8;
    u8 auto_delete;                   // +12
    native::pad<3> _13;
    native::gp<TlKey> key;            // +16 the next key to fire
    be_f32 delay;                     // +20 time until it fires
    native::gp<TlTrack> tracks;       // +24 running tracks
    native::gp<Timeline> next;        // +28
};
static_assert(offsetof(Timeline, tracks) == 24 && offsetof(Timeline, next) == 28);

// Native functions (src/ui).
u32 Ui_UpdateTimeline(Timeline* tl, f64 dt);
Element* Ui_FindInDirection(Element* e, u32 user, Element* focus, const be_f32* dir, be_f32* best);
void Ui_Update(f64 dt);
void Ui_Render(u8 enabled);

// Element fields.
constexpr u32 kElemFlags = 136;      // u32: 0x80000000 = active, 0x40000000 = focusable
constexpr u32 kElemFirstChild = 152; // u32 Element*
constexpr u32 kElemNext = 156;       // u32 Element*
constexpr u32 kElemVfnUpdate = 152;  // (f32 dt)
constexpr u32 kElemVfnTransform = 160;  // (out a, out b, out f32[16]): position at [12], [13] (name?)
constexpr u32 kElemActive = 0x80000000u;
constexpr u32 kElemFocusable = 0x40000000u;

// Timeline (Ui_UpdateTimeline) fields.
constexpr u32 kTlTemplate = 4;    // u32: +44 u8 loops, +48 first key
constexpr u32 kTlAutoDelete = 12; // u8: delete the timeline when it has finished
constexpr u32 kTlKey = 16;        // u32 Key*: the next key to fire
constexpr u32 kTlDelay = 20;      // f32: time until it fires
constexpr u32 kTlTracks = 24;     // u32 Track*: running tracks, linked by +16
constexpr u32 kTlNext = 28;       // u32 Timeline*
constexpr u32 kTemplateLoops = 44, kTemplateFirstKey = 48;
constexpr u32 kKeyDelay = 4;      // f32
constexpr u32 kKeyNext = 8;       // u32 Key*
constexpr u32 kTrackNext = 16;    // u32 Track*
constexpr u32 kVfnDelete = 4;     // ()
constexpr u32 kVfnRun = 8;        // key: (Timeline*, f1 = delay) -> new Track* or 0; track: (f32 dt) -> keep running?
constexpr u32 kMaxLoopsPerFrame = 4;

// Rodata constants.
constexpr f32 kEpsilon = std::bit_cast<f32>(0x3727C5ACu);     // 1e-5
constexpr f32 kMinusEpsilon = std::bit_cast<f32>(0xB727C5ACu);
constexpr f32 kPi = std::bit_cast<f32>(0x40490FDBu);

}  // namespace ui
}  // namespace sly4
