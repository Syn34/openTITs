// Game code, large-function sweep part 32: refresh the audio options menu
// (volume slider ticks and the format / centre speaker / dynamic range
// choice texts) from the saved settings.
#include "../sly4.h"
#include "big32.h"

using namespace sly4;
using namespace sly4::big32;

namespace {
constexpr u32 Ui_Find4D5544 = 0x004D5544;  // (const char* path, Widget* parent) -> Widget* or 0
constexpr u32 kTickMinX = 0x010BC0D0;      // f32: slider tick x at volume 0
constexpr u32 kTickMaxX = 0x010BC080;      // f32: slider tick x at volume 1
constexpr u32 sMenu = 0x00AE8800;          // "Menu"
constexpr u32 sTicks[4] = {0x00AE88D4, 0x00AE88E4, 0x00AE88FC, 0x00AE890C};  // "Volume{SFX,Music,VO,Movie}.Tick"
constexpr u32 sFormat = 0x00AE8924;        // "Format"
constexpr u32 sDefault = 0x00AE892C;       // "$STR_DEFAULT"
constexpr u32 sFormats[5] = {0x00AE893C, 0x00AE8954, 0x00AE896C, 0x00AE8984, 0x00AE898C};  // "$STR_AUDIO_FORMAT_SS" / _SH / _DPRII, "5.1", "7.1"
constexpr u32 sCenter = 0x00AE8994;        // "CenterSpeakerSize"
constexpr u32 sCss[2] = {0x00AE89AC, 0x00AE89C4};  // "$STR_AUDIO_CSS_SMALL" / _LARGE
constexpr u32 sRange = 0x00AE89DC;         // "DynamicRange"
constexpr u32 sDr[5] = {0x00AE89EC, 0x00AE8A04, 0x00AE8A1C, 0x00AE8A34, 0x00AE8A4C};  // "$STR_AUDIO_DR_MAX" / WIDE / NORMAL / NARROW / MIDNIGHT

// A choice widget's type (vfn 12) is 3 (a text choice).
bool IsText(PPUContext& ctx, u32 w) { return static_cast<s32>(VCall(ctx, w, 12, {w})) == 3; }

// The texts {"$STR_DEFAULT", names...} at sp + off and entry idx of them
// (idx unchecked, as the original).
u32 PickText(u32 sp, u32 off, const u32* names, u32 n, u32 idx) {
    store<u32>(sp + off, sDefault);
    for (u32 k = 0; k < n; ++k) store<u32>(sp + off + 4 + 4 * k, names[k]);
    return load<u32>(sp + (idx << 2) + off);
}
}  // namespace

// void f(Settings* s, Screen* scr) (called by f_004734E0 and f_00476C70).
// Under scr->+12's "Menu" widget (returns when missing, r3 0): each of
// the four "Volume*.Tick" widgets present gets its position (vfn 24, a
// f32[3]) with x = (kTickMaxX - kTickMinX) * volume + kTickMinX (volumes
// f32 at s + 4156 / 4160 / 4164 / 4168) set back (vfn 28, from sp + 112 /
// 128 / 144 / 160). Then "Format" (u8 s + 4152), "CenterSpeakerSize" (u8
// s + 4153) and "DynamicRange" (u8 s + 4154; returns when missing), when
// of type 3, get vfn 188(w, text, 0) with the text picked from a table
// starting with "$STR_DEFAULT" (the Format call has the entry in r6 and
// the OPD in r7; the CenterSpeakerSize call has the text in r4 and r6 and
// the entry in r7).
DECOMP(0046D5C8, AudioMenu_Refresh_0046D5C8) {
    const u32 st = arg<u32>(ctx, 0) + 4144;
    const u32 menu = static_cast<u32>(call(ctx, Ui_Find4D5544, {sMenu, load<u32>(arg<u32>(ctx, 1) + 12)}));
    if (menu == 0) return;
    Stack frame(ctx, 128);
    const u32 sp = frame.addr() - 112;  // the original's frame (offsets below are its sp offsets)
    for (u32 i = 0; i < 4; ++i) {
        const u32 w = static_cast<u32>(call(ctx, Ui_Find4D5544, {sTicks[i], menu}));
        if (w == 0) continue;
        const u32 pos = static_cast<u32>(VCall(ctx, w, 24, {w}));
        const u32 v = sp + 112 + 16 * i;
        Copy3W(v, pos);
        const f64 lo = Lfs(kTickMinX), hi = Lfs(kTickMaxX);
        const f64 x = Fmadds(Subs(hi, lo), Lfs(st + 12 + 4 * i), lo);
        store<u32>(v, Fb(x));
        ctx.fpr[1] = x;  // left in f1 by the original
        VCall(ctx, w, 28, {w, v});
    }
    const u32 fmt = static_cast<u32>(call(ctx, Ui_Find4D5544, {sFormat, menu}));
    if (fmt != 0 && IsText(ctx, fmt)) {
        const u32 t = PickText(sp, 188, sFormats, 5, load<u8>(st + 8));
        const u32 opd = load<u32>(load<u32>(fmt) + 188);
        call_ptr(ctx, opd, {fmt, t, 0, load<u32>(opd), opd});  // entry in r6, OPD in r7
    }
    const u32 css = static_cast<u32>(call(ctx, Ui_Find4D5544, {sCenter, menu}));
    if (css != 0 && IsText(ctx, css)) {
        const u32 opd = load<u32>(load<u32>(css) + 188);
        const u32 t = PickText(sp, 176, sCss, 2, load<u8>(st + 9));
        call_ptr(ctx, opd, {css, t, 0, t, load<u32>(opd)});  // the text in r4 and r6, entry in r7
    }
    const u32 dr = static_cast<u32>(call(ctx, Ui_Find4D5544, {sRange, menu}));
    if (dr == 0) return;
    if (IsText(ctx, dr)) {
        const u32 t = PickText(sp, 212, sDr, 5, load<u8>(st + 10));
        VCall(ctx, dr, 188, {dr, t, 0});
    }
}
