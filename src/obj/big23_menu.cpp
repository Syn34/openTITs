// Game code, large-function sweep part 23: the options menu's input
// handler (callee of f_004734E0 / f_00476C70).
#include "big23.h"

using namespace sly4;
using namespace sly4::big23;

namespace {
constexpr u32 kOptK = 0x0046EFA0u;         // f32 1.0 (the step numerator), f64 1.0 / 0.0 (the slider clamp)
constexpr u32 kOptSteps = 0x010BC120u;     // s32: slider steps
constexpr u32 kOptSettings = 4144;         // menu + 4144: the settings block
constexpr u32 kOptDirty = 4180;            // menu + 4180: u8 set after every change
constexpr u32 kSndToggle = 0x00128604u;    // menu + these: the sound descriptors
constexpr u32 kSndSlide = 0x00128608u;
constexpr u32 kSndBlocked = 0x0012860Cu;   // a slider already at its end
constexpr s32 kEvIncrease = 10;            // event type: right / increase
constexpr s32 kEvDecrease = 11;            // event type: left / decrease
constexpr u32 kUiEvOptionChanged = 16;

namespace ofn {
constexpr u32 Player_Get = 0x00365C24u;          // () -> player or 0
constexpr u32 Sound_Play44C474 = 0x0044C474u;    // (out, sound, 0 x 9) (11 arguments)
constexpr u32 Nop_0044D9B8 = 0x0044D9B8u;        // (u8) (reads nothing)
constexpr u32 Nop_0044D9BC = 0x0044D9BCu;        // (u8) (reads nothing)
constexpr u32 Global_10B0100_Set = 0x0044D9C0u;  // (u8)
constexpr u32 Ui_SendEvent = 0x004D3DA4u;        // (type, widget, player +340 or 0, 0)
}  // namespace ofn

void OptSound(PPUContext& ctx, u32 m, u32 out, u32 which) {
    call(ctx, ofn::Sound_Play44C474, {out, load<u32>(m + which), 0, 0, 0, 0, 0, 0, 0, 0, 0});
}

// Ui_SendEvent(16, e+4, the player's +340 (0 without a player), 0).
void OptNotify(PPUContext& ctx, u32 e) {
    const u32 w = load<u32>(e + 4);
    u32 p = 0;
    if (call(ctx, ofn::Player_Get, {}) != 0) p = load<u32>(static_cast<u32>(call(ctx, ofn::Player_Get, {})) + 340);
    call(ctx, ofn::Ui_SendEvent, {kUiEvOptionChanged, w, p, 0});
}

// An f32 slider at settings+off: +-1 / steps, clamped to [0, 1] (fsel
// with the f32 1.0 / f64 1.0 / f64 0.0 table k), stored; the slide sound,
// or the blocked one when the value did not change; the dirty byte; the
// copy to the global mirror (if any); notify. Event type 10 increases;
// strict: only type 11 decreases (other types leave the value unstored),
// else every other type decreases.
void OptSlider(PPUContext& ctx, u32 m, u32 e, u32 out, u32 off, u32 mirror, u32 k = kOptK, bool strict = false) {
    const u32 s = m + kOptSettings;
    const f64 old = Lf(s + off);
    f64 nv = old;
    const s32 type = load<s32>(e);
    if (!strict || type == kEvIncrease || type == kEvDecrease) {
        const f64 one = Lf(k);
        const f64 step = Divs(one, S(static_cast<f64>(load<s32>(kOptSteps))));
        const f64 v = type == kEvIncrease ? Adds(old, step) : Subs(old, step);
        const f64 t = S(Fsel(Subs(one, v), v, Lfd(k + 8)));
        nv = S(Fsel(t, t, Lfd(k + 16)));
        Sf(s + off, nv);
    }
    OptSound(ctx, m, out, old == nv ? kSndBlocked : kSndSlide);
    store<u8>(m + kOptDirty, 1);
    if (mirror != 0) Cw(mirror, s + off);
    OptNotify(ctx, e);
}

// A u8 choice at settings+off cycling through n values (wrapping both
// ways); setter(new); the toggle sound; the dirty byte; notify.
void OptCycle(PPUContext& ctx, u32 m, u32 e, u32 out, u32 off, u32 n, u32 setter) {
    const u32 s = m + kOptSettings;
    const u32 old = load<u8>(s + off);
    u32 v;
    if (load<s32>(e) == kEvIncrease)
        v = old + 1;
    else
        v = old > 0 ? old - 1 : n - 1;
    v &= 0xFF;
    const u32 r = v % n;
    store<u8>(s + off, static_cast<u8>(r));
    call(ctx, setter, {r});
    OptSound(ctx, m, out, kSndToggle);
    store<u8>(m + kOptDirty, 1);
    OptNotify(ctx, e);
}
}  // namespace

namespace {
constexpr u32 kOpt2K = 0x0046FBB8u;        // f32 1.0, f64 1.0 / 0.0 (the second page's copy)
constexpr u32 kArObj = 0x001286A4u;        // menu + this: the AR object (40 bytes, Ar_Init0A85C0) or 0
constexpr u32 kLangIndex = 4184;           // menu + 4184: u8 language index
constexpr u32 kLangDirty = 4183;           // menu + 4183: u8 set on a language change
constexpr u32 kLangEnabled = 0x010B8A64u;  // u8: 1 when the language can be changed
constexpr u32 kLangCount = 0x00F65DD0u;    // s32: languages
constexpr u32 kLangIds = 0x00F65D90u;      // u32[]: their ids (the low byte goes to settings +32)

namespace o2fn {
constexpr u32 Ar_Init0A85C0 = 0x000A85C0u;         // (ArObj*)
constexpr u32 Ar_Probe_000A88C0 = 0x000A88C0u;     // () (reads nothing; the AR thread's body call) (name?)
constexpr u32 Pad_ActiveIndexA96B8 = 0x000A96B8u;  // () -> s32
constexpr u32 Mem_Alloc = 0x0054ED38u;             // (size, 0, align)
}  // namespace o2fn

// Flips the settings word's bit mask; the toggle sound; the dirty byte;
// notify.
void OptToggle(PPUContext& ctx, u32 m, u32 e, u32 out, u32 mask) {
    const u32 s = m + kOptSettings;
    store<u32>(s, load<u32>(s) ^ mask);
    OptSound(ctx, m, out, kSndToggle);
    store<u8>(m + kOptDirty, 1);
    OptNotify(ctx, e);
}
}  // namespace

// void OptionsMenu_HandleInput46EFB8(Menu* m, const UiEvent* e): the
// focused widget (e+12; none: nothing) picks the setting by its hash +128:
// three 6/3/6-way choices (settings +8 / +9 / +10, with their setters) and
// four sliders (+16 / +24 / +12 / +20; all but +24 mirrored to
// 0x00F65B40 / 0x00F65B44 / 0x00F65B48). Event type 10 increases.
DECOMP(0046EFB8, OptionsMenu_HandleInput46EFB8) {
    const u32 m = arg<u32>(ctx, 0), e = arg<u32>(ctx, 1);
    const u32 w = load<u32>(e + 12);
    if (w == 0) return;
    Stack frame(ctx, 16);
    const u32 out = frame.addr();  // Sound_Play's out word (r1+144..184, one per case)
    switch (load<u32>(w + 128)) {
    case 0x7BF2EBF7u: OptCycle(ctx, m, e, out, 8, 6, ofn::Nop_0044D9B8); break;
    case 0x4CF5FB2Bu: OptSlider(ctx, m, e, out, 16, 0x00F65B40u); break;
    case 0x4CF34C36u: OptSlider(ctx, m, e, out, 24, 0); break;
    case 0x273D570Bu: OptSlider(ctx, m, e, out, 12, 0x00F65B44u); break;
    case 0x1431E4EBu: OptCycle(ctx, m, e, out, 9, 3, ofn::Nop_0044D9BC); break;
    case 0xBF338753u: OptSlider(ctx, m, e, out, 20, 0x00F65B48u); break;
    case 0xADCB105Eu: OptCycle(ctx, m, e, out, 10, 6, ofn::Global_10B0100_Set); break;
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
// whose id's low byte goes to settings +32, with m+4183 set.
DECOMP(0046FBD0, OptionsMenu_HandleInput2_46FBD0) {
    const u32 m = arg<u32>(ctx, 0), e = arg<u32>(ctx, 1);
    const u32 s = m + kOptSettings;
    const u32 w = load<u32>(e + 12);
    if (w == 0) return;
    Stack frame(ctx, 16);
    const u32 out = frame.addr();  // Sound_Play's out word (r1+144..180, one per case)
    switch (load<u32>(w + 128)) {
    case 0x438C1851u: OptSlider(ctx, m, e, out, 4, 0, kOpt2K); break;
    case 0x04B8CC71u: OptToggle(ctx, m, e, out, 0x04000000u); break;
    case 0x04A21AE6u: OptToggle(ctx, m, e, out, 0x02000000u); break;
    case 0xE38082B4u: {
        constexpr u32 kAr = 0x01000000u;
        if ((load<u32>(s) & kAr) == 0 || call(ctx, o2fn::Pad_ActiveIndexA96B8, {}) != 0 || (load<u32>(s) & kAr) == 0)
            call(ctx, o2fn::Ar_Probe_000A88C0, {});
        store<u32>(s, load<u32>(s) ^ kAr);
        OptSound(ctx, m, out, kSndToggle);
        store<u8>(m + kOptDirty, 1);
        const u32 ar = load<u32>(m + kArObj);
        if ((load<u32>(s) & kAr) == 0) {
            if (ar != 0) {
                VCall(ctx, ar, 4, {ar});
                store<u32>(m + kArObj, 0);
            }
        } else if (ar == 0) {
            const u32 p = static_cast<u32>(call(ctx, o2fn::Mem_Alloc, {40, 0, 16}));
            if (p != 0) call(ctx, o2fn::Ar_Init0A85C0, {p});
            store<u32>(m + kArObj, p);
        }
        OptNotify(ctx, e);
        break;
    }
    case 0xCE788358u: {
        if (load<u8>(kLangEnabled) != 1) break;
        const s32 type = load<s32>(e);
        u32 idx;
        if (type == kEvIncrease) {
            const u32 v = load<u8>(m + kLangIndex) + 1u;
            const u32 n = load<u32>(kLangCount);
            idx = v - static_cast<u32>(rt::divw(v, n)) * n;
        } else if (type == kEvDecrease) {
            idx = load<u8>(m + kLangIndex) - 1u;
            if (static_cast<s32>(idx) < 0) idx = load<u32>(kLangCount) - 1u;
        } else {
            break;
        }
        store<u8>(m + kLangIndex, static_cast<u8>(idx));
        store<u8>(s + 32, static_cast<u8>(load<u32>(kLangIds + 4 * (idx & 0xFF))));
        store<u8>(m + kLangDirty, 1);
        store<u8>(m + kOptDirty, 1);
        OptSound(ctx, m, out, kSndToggle);
        OptNotify(ctx, e);
        break;
    }
    case 0xC6D2473Bu: OptToggle(ctx, m, e, out, 0x08000000u); break;
    case 0x996B4041u: OptSlider(ctx, m, e, out, 28, 0, kOpt2K, true); break;
    default: break;
    }
}
