// Game code, large-function sweep part 23: the options menu's input
// handler (callee of f_004734E0 / f_00476C70).
#include "big23.h"

namespace sly4::obj {

namespace {
constexpr u32 kOptK = 0x0046EFA0u;         // f32 1.0 (the step numerator), f64 1.0 / 0.0 (the slider clamp)
constexpr u32 kOpt2K = 0x0046FBB8u;        // f32 1.0, f64 1.0 / 0.0 (the second page's copy)
constexpr u32 kOptSteps = 0x010BC120u;     // s32: slider steps
constexpr s32 kEvIncrease = 10;            // event type: right / increase
constexpr s32 kEvDecrease = 11;            // event type: left / decrease
constexpr u32 kUiEvOptionChanged = 16;
constexpr u32 kLangEnabled = 0x010B8A64u;  // u8: 1 when the language can be changed
constexpr u32 kLangCount = 0x00F65DD0u;    // s32: languages
constexpr u32 kLangIds = 0x00F65D90u;      // u32[]: their ids (the low byte goes to settings +32)
constexpr u32 Player_Get = 0x00365C24u;          // () -> player or 0
constexpr u32 Sound_Play44C474 = 0x0044C474u;    // (out, sound, 0 x 9) (11 arguments)
constexpr u32 Nop_0044D9B8 = 0x0044D9B8u;        // (u8) (reads nothing)
constexpr u32 Nop_0044D9BC = 0x0044D9BCu;        // (u8) (reads nothing)
constexpr u32 Global_10B0100_Set = 0x0044D9C0u;  // (u8)
constexpr u32 Ui_SendEvent = 0x004D3DA4u;        // (type, widget, player +340 or 0, 0)
constexpr u32 Ar_Init0A85C0 = 0x000A85C0u;         // (ArObj*)
constexpr u32 Pad_ActiveIndexA96B8 = 0x000A96B8u;  // () -> s32
// The AR probe, native as Ar_ProbeSubnet_000A88C0 (big24_net.cpp), called
// through the bridge: it waits on the timebase, which the menu's simulator
// harness cannot run inline.
constexpr u32 Ar_ProbeSubnet = 0x000A88C0u;  // () -> u8

// The settings block (menu + 4144): +0 the flag word, sliders and choices.
struct OptSettings {
    be_u32 flags;     // +0
    be_f32 f4;        // +4
    u8 b8, b9, b10;   // +8 / +9 / +10
    pad<1> _11;
    be_f32 f12;       // +12
    be_f32 f16;       // +16
    be_f32 f20;       // +20
    be_f32 f24;       // +24
    be_f32 f28;       // +28
    u8 lang;          // +32
    pad<3> _33;
};
static_assert(sizeof(OptSettings) == 36);
struct OptMenu {
    pad<4144> _0;
    OptSettings s;      // +4144
    u8 dirty;           // +4180: set after every change
    pad<2> _4181;
    u8 lang_dirty;      // +4183: set on a language change
    u8 lang_index;      // +4184
    pad<0x128604 - 4185> _4185;
    be_u32 snd_toggle;  // +0x128604: the sound descriptors
    be_u32 snd_slide;   // +0x128608
    be_u32 snd_blocked; // +0x12860C: a slider already at its end
    pad<0x1286A4 - 0x128610> _128610;
    be_u32 ar;          // +0x1286A4: the AR object (40 bytes, Ar_Init0A85C0) or 0
};
static_assert(offsetof(OptMenu, dirty) == 4180 && offsetof(OptMenu, lang_index) == 4184);
static_assert(offsetof(OptMenu, snd_toggle) == 0x128604 && offsetof(OptMenu, ar) == 0x1286A4);
// A UI event: +0 its type, +4 the widget, +12 the focused widget (+128 its hash).
struct OptEvent {
    be_s32 type;      // +0
    be_u32 widget;    // +4
    pad<4> _8;
    be_u32 focus;     // +12
};

void OptSound(be_u32* out, const be_u32& snd) {
    call(Sound_Play44C474, out, snd.get(), 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u);
}

// Ui_SendEvent(16, e+4, the player's +340 (0 without a player), 0).
void OptNotify(const OptEvent* e) {
    const u32 w = e->widget;
    u32 p = 0;
    if (call<u32>(Player_Get) != 0) p = gm<be_u32>(call<u32>(Player_Get) + 340);
    call(Ui_SendEvent, kUiEvOptionChanged, w, p, 0u);
}

// An f32 slider: +-1 / steps, clamped to [0, 1] (fsel with the f32 1.0 /
// f64 1.0 / f64 0.0 table k), stored; the slide sound, or the blocked one
// when the value did not change; the dirty byte; the copy to the global
// mirror (if any); notify. Event type 10 increases; strict: only type 11
// decreases (other types leave the value unstored), else every other type
// decreases.
void OptSlider(OptMenu* m, const OptEvent* e, be_u32* out, be_f32& val, u32 mirror, u32 k = kOptK, bool strict = false) {
    const f64 old = lf(val);
    f64 nv = old;
    const s32 type = e->type;
    if (!strict || type == kEvIncrease || type == kEvDecrease) {
        const f64 one = lf(global<be_f32>(k));
        const f64 step = Divs(one, S(static_cast<f64>(global<be_s32>(kOptSteps).get())));
        const f64 v = type == kEvIncrease ? Adds(old, step) : Subs(old, step);
        const f64 t = S(Fsel(Subs(one, v), v, ld(global<be_f64>(k + 8))));
        nv = S(Fsel(t, t, ld(global<be_f64>(k + 16))));
        val = static_cast<f32>(nv);
    }
    OptSound(out, old == nv ? m->snd_blocked : m->snd_slide);
    m->dirty = 1;
    if (mirror != 0) set_bits(global<be_f32>(mirror), bits(val));
    OptNotify(e);
}

// A u8 choice cycling through n values (wrapping both ways); setter(new);
// the toggle sound; the dirty byte; notify.
void OptCycle(OptMenu* m, const OptEvent* e, be_u32* out, u8& val, u32 n, u32 setter) {
    const u32 old = val;
    u32 v;
    if (e->type == kEvIncrease)
        v = old + 1;
    else
        v = old > 0 ? old - 1 : n - 1;
    v &= 0xFF;
    const u32 r = v % n;
    val = static_cast<u8>(r);
    call(setter, r);
    OptSound(out, m->snd_toggle);
    m->dirty = 1;
    OptNotify(e);
}

// Flips the settings word's bit mask; the toggle sound; the dirty byte;
// notify.
void OptToggle(OptMenu* m, const OptEvent* e, be_u32* out, u32 mask) {
    m->s.flags = m->s.flags ^ mask;
    OptSound(out, m->snd_toggle);
    m->dirty = 1;
    OptNotify(e);
}
}  // namespace

// void OptionsMenu_HandleInput46EFB8(Menu* m, const UiEvent* e): the
// focused widget (e+12; none: nothing) picks the setting by its hash +128:
// three 6/3/6-way choices (settings +8 / +9 / +10, with their setters) and
// four sliders (+16 / +24 / +12 / +20; all but +24 mirrored to
// 0x00F65B40 / 0x00F65B44 / 0x00F65B48). Event type 10 increases. No caller reads r3.
void OptionsMenu_HandleInput46EFB8(OptMenu* m, const OptEvent* e) {
    const u32 w = e->focus;
    if (w == 0) return;
    GuestFrame frame(16);
    be_u32* out = frame.as<be_u32>();  // Sound_Play's out word (r1+144..184, one per case)
    OptSettings& s = m->s;
    switch (gm<be_u32>(w + 128).get()) {
    case 0x7BF2EBF7u: OptCycle(m, e, out, s.b8, 6, Nop_0044D9B8); break;
    case 0x4CF5FB2Bu: OptSlider(m, e, out, s.f16, 0x00F65B40u); break;
    case 0x4CF34C36u: OptSlider(m, e, out, s.f24, 0); break;
    case 0x273D570Bu: OptSlider(m, e, out, s.f12, 0x00F65B44u); break;
    case 0x1431E4EBu: OptCycle(m, e, out, s.b9, 3, Nop_0044D9BC); break;
    case 0xBF338753u: OptSlider(m, e, out, s.f20, 0x00F65B48u); break;
    case 0xADCB105Eu: OptCycle(m, e, out, s.b10, 6, Global_10B0100_Set); break;
    default: break;
    }
}

// void OptionsMenu_HandleInput2_46FBD0(Menu* m, const UiEvent* e): the
// second options page (the focused widget e+12 by its hash +128): sliders
// settings +4 (any type) and +28 (types 10 / 11 only), toggles of the
// settings word bits 0x04000000 / 0x02000000 / 0x08000000, the AR toggle
// (bit 0x01000000: 0x000A88C0 runs first unless the bit is set and no pad
// is active; switching on makes the 40-byte AR object at m+0x1286A4 when
// there is none, switching off releases it through its vfn[1] and clears
// the pointer) and, while 0x010B8A64 is 1, the language index m+4184
// (type 10: + 1 modulo the count, divw; 11: - 1, wrapping to count - 1)
// whose id's low byte goes to settings +32, with m+4183 set. No caller
// reads r3.
void OptionsMenu_HandleInput2_46FBD0(OptMenu* m, const OptEvent* e) {
    OptSettings& s = m->s;
    const u32 w = e->focus;
    if (w == 0) return;
    GuestFrame frame(16);
    be_u32* out = frame.as<be_u32>();  // Sound_Play's out word (r1+144..180, one per case)
    switch (gm<be_u32>(w + 128).get()) {
    case 0x438C1851u: OptSlider(m, e, out, s.f4, 0, kOpt2K); break;
    case 0x04B8CC71u: OptToggle(m, e, out, 0x04000000u); break;
    case 0x04A21AE6u: OptToggle(m, e, out, 0x02000000u); break;
    case 0xE38082B4u: {
        constexpr u32 kAr = 0x01000000u;
        if ((s.flags & kAr) == 0 || call<u32>(Pad_ActiveIndexA96B8) != 0 || (s.flags & kAr) == 0) call(Ar_ProbeSubnet);
        s.flags = s.flags ^ kAr;
        OptSound(out, m->snd_toggle);
        m->dirty = 1;
        const u32 ar = m->ar;
        if ((s.flags & kAr) == 0) {
            if (ar != 0) {
                vcall(&gm<u8>(ar), 4);
                m->ar = 0u;
            }
        } else if (ar == 0) {
            const u32 p = call<u32>(fn::Mem_Alloc, 40u, 0u, 16u);
            if (p != 0) call(Ar_Init0A85C0, p);
            m->ar = p;
        }
        OptNotify(e);
        break;
    }
    case 0xCE788358u: {
        if (global<u8>(kLangEnabled) != 1) break;
        const s32 type = e->type;
        u32 idx;
        if (type == kEvIncrease) {
            const u32 v = m->lang_index + 1u;
            const u32 n = global<be_u32>(kLangCount);
            idx = v - static_cast<u32>(rt::divw(v, n)) * n;
        } else if (type == kEvDecrease) {
            idx = m->lang_index - 1u;
            if (static_cast<s32>(idx) < 0) idx = global<be_u32>(kLangCount) - 1u;
        } else {
            break;
        }
        m->lang_index = static_cast<u8>(idx);
        s.lang = static_cast<u8>(gm<be_u32>(kLangIds + 4 * (idx & 0xFF)));
        m->lang_dirty = 1;
        m->dirty = 1;
        OptSound(out, m->snd_toggle);
        OptNotify(e);
        break;
    }
    case 0xC6D2473Bu: OptToggle(m, e, out, 0x08000000u); break;
    case 0x996B4041u: OptSlider(m, e, out, s.f28, 0, kOpt2K, true); break;
    default: break;
    }
}

}  // namespace sly4::obj

NATIVE(0046EFB8, sly4::obj::OptionsMenu_HandleInput46EFB8)
NATIVE(0046FBD0, sly4::obj::OptionsMenu_HandleInput2_46FBD0)
