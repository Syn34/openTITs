// World_HandleEvent callees, part 10j: the movie screen event.
#include <bit>

#include "hevent10.h"

using namespace sly4;
using namespace sly4::hevent;
using namespace sly4::hevent7;
using namespace sly4::hevent10;

namespace sly4 {
namespace hevent10j {
namespace jfn10 {
constexpr u32 String_HashNoCase = 0x0056A710;
constexpr u32 SaveSlot_MarkSeen = 0x002E2874;    // (session, u32 hash)
constexpr u32 Ui_OpenScreen4D245C = 0x004D245C;  // (name, layer, s32 z) -> Element*
constexpr u32 Ui_Find = 0x004D5544;              // (path, parent)
constexpr u32 Game_SetPaused = 0x00365A54;       // (bool resume, 0)
constexpr u32 Obj_SetPhysicsActive = 0x00260C14; // (o, u8 on)
constexpr u32 Session_FindMovie2E2D44 = 0x002E2D44;  // (session, u32 hash) -> movie entry (name?)
constexpr u32 UiElem_SetX4D21B4 = 0x004D21B4;    // (Element*, s32 x, 0)
}  // namespace jfn10

// The handler context (as the World event handler's r3): +6 u16 offset of
// the current event record in +20, +40 flags, +4168 f32 depth, +4216
// player object. Record: +0 screen, +4 argument strings (+24 index of the
// name), +28 movie entry (+8 subtitle text), +32 subtitle frame, +60 flags
// (bit 30: failed, bit 28: cleared here).
constexpr u32 kCtxRecOff = 6, kCtxRecs = 20, kCtxFlags = 40, kCtxDepth = 4168, kCtxPlayer = 4216;
constexpr u32 kRecScreen = 0, kRecArgs = 4, kRecNameIdx = 24, kRecMovie = 28, kRecFrame = 32, kRecFlags = 60;
constexpr u32 kRecFailed = 0x40000000u, kRecBit28 = 0x10000000u;
constexpr u32 kScreenState = 0x010C5A20;  // +4 / +6 u16 screen width / height, +40 s32 layer (16: use 0) (name?)
constexpr u32 kDepthScale = 0x010AECFC + 72;  // f32 (as hevent.h kTune10AECFC)
constexpr u32 kConst48E6E8 = 0x0048E6E8;  // f32 32768, 0.5 (code segment)
constexpr u32 kNoUi1 = 0x01163F81, kNoUi2 = 0x01163F82;  // u8 (name?)
constexpr u32 kSessionPtr = 0x00F64C20;
constexpr u32 kPausedByMovie = 0x00F66FC0;  // u8 cleared (name?)
// The UI layers hidden while the movie plays (vtable +88: show(u8)).
constexpr u32 kUiLayers[] = {0x00AE9384 /* "Common" */, 0x00AE8254 /* "InGame" */, 0x00AE8244 /* "FrontEnd" */,
                             0x00AE938C /* "LevelEditor" */, 0x00AE939C /* "ArtViewer" */};
constexpr u32 kBinkSubtitles = 0x00AE93AC, kTextFrameText = 0x00AE93C0;
constexpr u32 kVShow = 88, kVGetType = 12, kVSetText = 188, kVPhysObj = 64;
constexpr u32 kElemW = 288, kElemH = 292, kElemScaleX = 280, kElemScaleY = 284;
constexpr u32 kPlayerFlags = 192, kPlayerHasPhys = 0x8;
}  // namespace hevent10j
}  // namespace sly4

using namespace sly4::hevent10j;

namespace {
// fctiwz as the PPU leaves it in the FPR (0xFFF80000 above the result).
f64 fctiwz_reg(u32 v) { return std::bit_cast<f64>(0xFFF8000000000000ull | v); }
}  // namespace

// u32 Event_PlayMovie48E6F0(HandlerCtx* c) (name?): opens the screen named by
// the record's argument (layer from 0x010C5A20 +40, z from the context
// depth; nothing when UI is disabled), scaled to fit the screen height
// and centred. Without a screen the record fails (+60 bit 30). Otherwise
// the standard UI layers are hidden, the game resumed, the player's physics
// switched off, the record's movie entry looked up and its subtitle text set
// on "BinkSubtitles" / "TextFrame.Text"; the name is marked seen in the
// save slot before and after. r3 / f1 as the original leaves them.
DECOMP(0048E6F0, Event_PlayMovie48E6F0) {
    const u32 c = arg<u32>(ctx, 0);
    const u32 rec = load<u32>(c + kCtxRecs) + load<u16>(c + kCtxRecOff);
    const u32 name = load<u32>(rec + kRecArgs + load<u32>(rec + kRecNameIdx) * 4);
    if (name == 0 || load<u8>(name) == 0) {
        const u32 f = load<u32>(rec + kRecFlags) | kRecFailed;
        store<u32>(rec + kRecFlags, f);
        ctx.gpr[3] = f;
        return;
    }
    u32 layer = load<u32>(kScreenState + 40);
    if (static_cast<s32>(layer) == 16) layer = 0;
    const f32 zf = fmuls(fmuls(lfs_f64(c + kCtxDepth), lfs_f64(kDepthScale)), lfs_f64(kConst48E6E8));
    const u32 zbits = static_cast<u32>(rt::fctiw(zf, true));
    ctx.fpr[1] = fctiwz_reg(zbits);
    const s32 z = static_cast<s32>(zbits);
    u32 scr = 0;
    if (load<u8>(kNoUi1) == 0 && load<u8>(kNoUi2) == 0) {
        const u32 session = load<u32>(kSessionPtr);
        const u32 h = static_cast<u32>(call(ctx, jfn10::String_HashNoCase, {name}));
        call(ctx, jfn10::SaveSlot_MarkSeen, {session, h});
        scr = static_cast<u32>(call(ctx, jfn10::Ui_OpenScreen4D245C, {name, layer, static_cast<u64>(static_cast<s64>(z))}));
        if (scr != 0) {
            // Fit the screen height: scale = height / element height; x =
            // (width - scale * element width) * 0.5.
            const f32 scale = static_cast<f32>(static_cast<f64>(static_cast<f32>(static_cast<f64>(load<u16>(kScreenState + 6)))) /
                                               static_cast<f64>(static_cast<f32>(static_cast<f64>(load<u32>(scr + kElemH)))));
            const f64 sw = static_cast<f32>(static_cast<f64>(load<u16>(kScreenState + 4)));
            const f64 ew = static_cast<f32>(static_cast<f64>(load<u32>(scr + kElemW)));
            const f32 x = fmuls(static_cast<f32>(-std::fma(static_cast<f64>(scale), ew, -sw)), lfs_f64(kConst48E6E8 + 4));
            const u32 xbits = static_cast<u32>(rt::fctiw(x, true));
            ctx.fpr[1] = fctiwz_reg(xbits);
            call(ctx, jfn10::UiElem_SetX4D21B4, {scr, static_cast<u64>(static_cast<s64>(static_cast<s32>(xbits))), 0u});
            store<f32>(scr + kElemScaleX, scale);
            store<f32>(scr + kElemScaleY, scale);
        }
    }
    store<u32>(rec + kRecScreen, scr);
    if (scr == 0) {
        const u32 f = load<u32>(rec + kRecFlags) | kRecFailed;
        store<u32>(rec + kRecFlags, f);
        ctx.gpr[3] = f;
        return;
    }
    for (const u32 l : kUiLayers) {
        const u32 e = static_cast<u32>(call(ctx, jfn10::Ui_Find, {l, 0u}));
        if (e != 0) vcall(ctx, e, kVShow, {e, 0u});
    }
    {
        const u32 e = static_cast<u32>(call(ctx, jfn10::Ui_Find, {kBinkSubtitles, 0u}));
        if (e != 0) vcall(ctx, e, kVShow, {e, 0u});
    }
    call(ctx, jfn10::Game_SetPaused, {0u, 0u});
    store<u8>(kPausedByMovie, 0);
    const u32 pl = load<u32>(c + kCtxPlayer);
    if (pl != 0 && (load<u32>(pl + kPlayerFlags) & kPlayerHasPhys) != 0) {
        if (static_cast<u32>(vcall(ctx, pl, kVPhysObj, {pl})) != 0) {
            const u32 o = static_cast<u32>(vcall(ctx, pl, kVPhysObj, {pl}));
            call(ctx, jfn10::Obj_SetPhysicsActive, {o, 0u});
        }
    }
    store<u32>(c + kCtxFlags, load<u32>(c + kCtxFlags) | 7);
    const u32 h = static_cast<u32>(call(ctx, jfn10::String_HashNoCase, {load<u32>(rec + kRecArgs + load<u32>(rec + kRecNameIdx) * 4)}));
    store<u32>(rec + kRecFlags, load<u32>(rec + kRecFlags) & ~kRecBit28);
    store<u32>(rec + kRecMovie, static_cast<u32>(call(ctx, jfn10::Session_FindMovie2E2D44, {load<u32>(kSessionPtr), h})));
    const u32 frame = static_cast<u32>(call(ctx, jfn10::Ui_Find, {kBinkSubtitles, 0u}));
    store<u32>(rec + kRecFrame, frame);
    if (frame != 0 && load<u32>(rec + kRecMovie) != 0) {
        const u32 text = static_cast<u32>(call(ctx, jfn10::Ui_Find, {kTextFrameText, frame}));
        if (text != 0 && static_cast<s32>(vcall(ctx, text, kVGetType, {text})) == 3) {
            const u32 opd = load<u32>(load<u32>(text) + kVSetText);
            ctx.gpr[6] = load<u32>(opd);
            ctx.gpr[7] = opd;
            call_ptr(ctx, opd, {text, load<u32>(rec + kRecMovie) + 8, 0u});
        }
    }
    ctx.gpr[3] = call(ctx, jfn10::SaveSlot_MarkSeen, {load<u32>(kSessionPtr), h});
}
