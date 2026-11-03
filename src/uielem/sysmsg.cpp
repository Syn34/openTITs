// The system message box's callbacks, installed by Game_Update while the
// box is open (see src/game/update.cpp).
#include "uielem.h"

using namespace sly4;
using namespace sly4::uielem;

namespace {

void Elem_SetVisible(PPUContext& ctx, u32 e, u32 visible) {
    call_ptr(ctx, vfunc(e, kVfnSetVisible), {e, visible});
}

// Hide the box and put back what opening it changed (inlined in both callbacks).
void SysMsg_Close(PPUContext& ctx) {
    u32 e = static_cast<u32>(call(ctx, sfn::Ui_Find, {kFrontEnd, 0}));
    if (e != 0) Elem_SetVisible(ctx, e, 1);
    e = static_cast<u32>(call(ctx, sfn::Ui_Find, {kInGame, 0}));
    if (e != 0) Elem_SetVisible(ctx, e, 1);
    e = static_cast<u32>(call(ctx, sfn::Ui_Find, {kCommonFade, 0}));
    if (e != 0) Elem_SetVisible(ctx, e, ref<u8>(kSavedFadeState));
    e = static_cast<u32>(call(ctx, sfn::Ui_Find, {kCommonLoading, 0}));
    if (e != 0) Elem_SetVisible(ctx, e, ref<u8>(kSavedLoadingState));

    const u32 game = g::game;
    Elem_SetVisible(ctx, field<u32>(game, kGameSysMsgUi), 0);  // no null check in the original
    call(ctx, sfn::Input_SetMenuMode, {ref<u8>(load<u32>(kSession) + kSessionMenuMode) != 0 ? 1u : 0u});
    const u32 box = field<u32>(game, kGameSysMsgUi);
    store<u32>(kSysMsgActive, 0);
    call(ctx, sfn::Ui_SendEvent, {kUiEventSysMsgClosed, 0xFFFFFFFFu, box, 0});
    call(ctx, sfn::Ui_SetEventHandler, {load<u32>(kSavedUiEventHandler), load<u32>(kSavedUiEventArg)});
    call(ctx, sfn::Ui_SetInputHandler, {load<u32>(kSavedUiInputHandler), load<u32>(kSavedUiInputArg)});
    field<u32>(game, kGameSysMsgUi) = 0;
}

}  // namespace

// void SysMsg_OnClose(s32 code)
// The kSysMsgCallback (OPD 0x00F39BD8) Game_Load installs.
DECOMP(004645AC, SysMsg_OnClose) { SysMsg_Close(ctx); }

// bool SysMsg_OnInput(u32 user, UiInput* out)
// The UI input handler while the box is open: the player's buttons and
// cursor (users 0 and 1 only).
DECOMP(00464794, SysMsg_OnInput) {
    const u32 user = arg<u32>(ctx, 0);
    const u32 out = arg<u32>(ctx, 1);
    if (user >= kPlayerCount) return ret(ctx, 0u);
    const u32 p = g::game + user * kPlayerSize + kGamePlayers;
    field<u32>(out, 4) = field<u32>(p, kPlayerHeld);
    store<u32>(out + 16, load<u32>(p + kPlayerCursorX));
    field<u32>(out, 8) = field<u32>(p, kPlayerPressed);
    const u32 valid = field<u32>(p, kPlayerValid);
    field<u32>(out, 12) = valid;
    store<u32>(out + 20, load<u32>(p + kPlayerCursorY));
    if (field<u32>(p, kPlayerFlags) & kPlayerCursorStarted) field<u32>(out, 12) = valid | kInputPointerValid;
    ret(ctx, 1u);
}

// bool SysMsg_OnEvent(UiEventBlock* ev)
// The UI event handler while the box is open (handler_arg = Game): a
// button-0x80 press on an element closes it. Always returns 1.
DECOMP(00464804, SysMsg_OnEvent) {
    const u32 ev = arg<u32>(ctx, 0);
    if (field<u32>(field<u32>(ev, 8), kGameSysMsgUi) == 0) return ret(ctx, 1u);
    if (field<u32>(ev, 0) == kUiEventButton80 && field<u32>(ev, 12) != 0) SysMsg_Close(ctx);
    ret(ctx, 1u);
}
