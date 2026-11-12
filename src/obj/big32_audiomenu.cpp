// Game code, large-function sweep part 32: refresh the audio options menu
// (volume slider ticks and the format / centre speaker / dynamic range
// choice texts) from the saved settings.
#include "big32.h"

namespace sly4::obj {

struct AudioSettings {
    pad<4152> _0;
    u8 format;        // +4152
    u8 center;        // +4153
    u8 range;         // +4154
    pad<1> _4155;
    be_f32 vol[4];    // +4156: SFX, music, VO, movie
};
static_assert(offsetof(AudioSettings, format) == 4152 && offsetof(AudioSettings, vol) == 4156);
struct AudioScreen {
    pad<12> _0;
    gp<UiElem> root;  // +12
};

namespace {
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

bool IsText(UiElem* w) { return static_cast<s32>(UiElem_State(w)) == 3; }

// The texts {"$STR_DEFAULT", names...} at frame offset off and entry idx
// of them. The tables live in guest stack memory as in the original, so an
// out-of-range idx reads the neighbouring stack words (unchecked).
u32 PickText(const GuestFrame& f, u32 off, const u32* names, u32 n, u32 idx) {
    be_u32* t = f.as<be_u32>(off);
    t[0] = sDefault;
    for (u32 k = 0; k < n; ++k) t[1 + k] = names[k];
    return gm<be_u32>(f.addr() + off + (idx << 2));
}
}  // namespace

// void f(Settings* s, Screen* scr) (called by f_004734E0 and f_00476C70).
// Under scr->root's "Menu" widget (returns when missing): each of the four
// "Volume*.Tick" widgets present gets its position (vfn 24, a f32[3]) with
// x = (kTickMaxX - kTickMinX) * volume + kTickMinX set back (vfn 28, from
// the stack). Then "Format", "CenterSpeakerSize" and "DynamicRange"
// (returns when missing), when of type 3, get their text (vfn 188) picked
// from a table starting with "$STR_DEFAULT".
void AudioMenu_Refresh_0046D5C8(const AudioSettings* s, const AudioScreen* scr) {
    UiElem* menu = call<UiElem*>(fn::Ui_Find, sMenu, scr->root.get());
    if (menu == nullptr) return;
    GuestFrame frame(128);  // the original's sp+112..: ticks +0 / +16 / +32 / +48, tables +64 / +76 / +100
    for (u32 i = 0; i < 4; ++i) {
        UiElem* w = call<UiElem*>(fn::Ui_Find, sTicks[i], menu);
        if (w == nullptr) continue;
        const Vec3& pos = gm<Vec3>(vcall<u32>(w, 24));
        Vec3* v = frame.as<Vec3>(16 * i);
        set_bits(v->x, bits(pos.x));  // lfs / stfs: raw words
        set_bits(v->y, bits(pos.y));
        set_bits(v->z, bits(pos.z));
        const f64 lo = lf(global<be_f32>(kTickMinX)), hi = lf(global<be_f32>(kTickMaxX));
        v->x = static_cast<f32>(Fmadds(Subs(hi, lo), lf(s->vol[i]), lo));
        vcall(w, 28, v);
    }
    UiElem* fmt = call<UiElem*>(fn::Ui_Find, sFormat, menu);
    if (fmt != nullptr && IsText(fmt)) UiElem_SetText(fmt, PickText(frame, 76, sFormats, 5, s->format));
    UiElem* css = call<UiElem*>(fn::Ui_Find, sCenter, menu);
    if (css != nullptr && IsText(css)) UiElem_SetText(css, PickText(frame, 64, sCss, 2, s->center));
    UiElem* dr = call<UiElem*>(fn::Ui_Find, sRange, menu);
    if (dr == nullptr) return;
    if (IsText(dr)) UiElem_SetText(dr, PickText(frame, 100, sDr, 5, s->range));
}

}  // namespace sly4::obj

NATIVE(0046D5C8, sly4::obj::AudioMenu_Refresh_0046D5C8)
