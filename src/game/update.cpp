// Game_Update: the per-frame update main() calls once per frame.
#include "update.h"

using namespace sly4;
using namespace sly4::update;

namespace {

// The function descriptor in slot `offset` of obj's vtable.
u32 vfunc(u32 obj, u32 offset) { return load<u32>(load<u32>(obj) + offset); }

u32 Ui_Find(PPUContext& ctx, u32 name, u32 parent) {
    return static_cast<u32>(call(ctx, ufn::Ui_Find, {name, parent}));
}
void Ui_SetVisible(PPUContext& ctx, u32 element, u32 visible) {
    call_ptr(ctx, vfunc(element, kUiVfnSetVisible), {element, visible});
}

// The frame time every update gets: SystemState +4.
f32 FrameTime(PPUContext& ctx) { return field<f32>(system_state::get(ctx), kSystemStateFrameTime); }

constexpr u32 kFrontEnd = 0x00AE8244;        // "FrontEnd"
constexpr u32 kInGame = 0x00AE8254;          // "InGame"
constexpr u32 kFade = 0x00AE825C;            // "Common.Fade"
constexpr u32 kLoading = 0x00AE826C;         // "Common.Loading"
constexpr u32 kSystemMessage = 0x00AE827C;   // "Common.System_Message"
constexpr u32 kBg = 0x00AE8294;              // "BG"
constexpr u32 kText = 0x00AE829C;            // "Text"
constexpr u32 kBgText = 0x00AE82A4;          // "BG_Text"
constexpr u32 kCursorFormat = 0x00AE82AC;    // "Common.Cursors.%02d"
constexpr u32 kCursorCount = 2;
constexpr u32 kPlayerCount = 2;
constexpr u32 kViewCount = 4;
constexpr u32 kSysTimerUsleep = 141;         // LV2 sys_timer_usleep(usecs)
constexpr u32 kDebugStallUs = 16000;
constexpr u32 kPadLostFrames = 3;            // countdown restart value
constexpr u16 kPadConnectedBit = 0x8000;

// The display config changed (config u16 +4 differs from the one applied
// last): hand the new rectangle to every view of the world and to the main
// view, then the main view's matrix (the 4x4 at kViewMatrix with only x, y
// and z of each row copied; the w slots of the copy are left unset, as in
// the original, and sub_005201B0 never reads them).
void Game_ApplyViewport(PPUContext& ctx, u32 game, u16 width) {
    store<u16>(kAppliedViewW, width);
    Stack frame(ctx, 64 + sizeof(ViewRect));
    const u32 matrix = frame.addr();
    const u32 rect_ea = frame.addr() + 64;
    auto& rect = ref<ViewRect>(rect_ea);
    rect.x = field<u16>(g::config, 8);
    rect.y = field<u16>(g::config, 10);
    rect.width = width;
    rect.height = field<u16>(g::config, 6);

    const u32 world = load<u32>(game + kGameWorld);
    for (u32 k = 0; k < kViewCount; ++k) {
        const u32 view = load<u32>(world + kWorldViews + k * 4);
        if (view != 0) call(ctx, ufn::sub_005200AC, {view, rect_ea});
    }
    call(ctx, ufn::sub_005200AC, {load<u32>(kMainView), rect_ea});

    for (u32 row = 0; row < 4; ++row)
        for (u32 col = 0; col < 3; ++col)
            store<u32>(matrix + row * 16 + col * 4, load<u32>(kViewMatrix + row * 16 + col * 4));
    call(ctx, ufn::sub_005201B0, {load<u32>(kMainView), matrix});
}

// Shows the system message box over everything while one is up. The first
// frame it's up, FrontEnd, InGame, Common.Fade and Common.Loading are
// hidden (Fade's and Loading's state saved), the UI handlers saved and
// replaced with the system message's own, and event 12 sent. Returns with
// g::main_state = kMainPaused while it's up.
void Game_UpdateSystemMessage(PPUContext& ctx, u32 game) {
    s32 code = load<s32>(kSysMsgCode);
    u32 active = load<u32>(kSysMsgActive);
    // Codes 0, 1 and 3 are cleared without being shown. (The original
    // stores the two zeroes twice for codes 0 and 1 when inactive.)
    if (code == 0 || code == 1 || code == 3) {
        store<u32>(kSysMsgActive, 0);
        store<s32>(kSysMsgCode, 0);
        code = 0;
        active = 0;
    }
    if (active != 0) {
        const u32 callback = load<u32>(kSysMsgCallback);
        if (callback != 0) {
            call_ptr(ctx, callback, {static_cast<u32>(code)});
            code = load<s32>(kSysMsgCode);
            active = load<u32>(kSysMsgActive);
        }
    }
    if (active == 0) return;
    if (ref<u8>(kSysMsgShows + static_cast<u32>(code)) == 0) return;

    ref<be_u32>(g::main_state) = kMainPaused;
    if (field<u32>(game, kGameSysMsgUi) == 0) {
        // Save the UI handlers, last word first.
        store<u32>(kSavedUiEventArg, load<u32>(kUiEventArg));
        store<u32>(kSavedUiEventHandler, load<u32>(kUiEventHandler));
        store<u32>(kSavedUiInputArg, load<u32>(kUiInputArg));
        store<u32>(kSavedUiInputHandler, load<u32>(kUiInputHandler));

        if (const u32 e = Ui_Find(ctx, kFrontEnd, 0)) Ui_SetVisible(ctx, e, 0);
        if (const u32 e = Ui_Find(ctx, kInGame, 0)) Ui_SetVisible(ctx, e, 0);
        if (const u32 e = Ui_Find(ctx, kFade, 0)) {
            ref<u8>(kSavedFadeState) = static_cast<u8>(call_ptr(ctx, vfunc(e, kUiVfnState), {e}));
            Ui_SetVisible(ctx, e, 0);
        }
        if (const u32 e = Ui_Find(ctx, kLoading, 0)) {
            ref<u8>(kSavedLoadingState) = static_cast<u8>(call_ptr(ctx, vfunc(e, kUiVfnState), {e}));
            Ui_SetVisible(ctx, e, 0);
        }
        call(ctx, ufn::Audio_SetPaused, {1});
        call(ctx, ufn::Ui_SetInputHandler, {kSysMsgInputHandler, 0});
        call(ctx, ufn::Ui_SetEventHandler, {kSysMsgEventHandler, game});
        // Game +4 is still 0 here unless one of the calls above set it.
        call(ctx, ufn::Ui_SendEvent, {kUiEventSystemMessage, 0xFFFFFFFFu, field<u32>(game, kGameSysMsgUi), 0});
        code = load<s32>(kSysMsgCode);
    }

    const u32 box = Ui_Find(ctx, kSystemMessage, 0);
    field<u32>(game, kGameSysMsgUi) = box;
    if (box != 0) {
        Ui_SetVisible(ctx, box, 1);
        if (const u32 e = Ui_Find(ctx, kBg, field<u32>(game, kGameSysMsgUi))) Ui_SetVisible(ctx, e, 1);
        for (const u32 name : {kText, kBgText}) {
            const u32 e = Ui_Find(ctx, name, field<u32>(game, kGameSysMsgUi));
            if (e == 0) continue;
            Ui_SetVisible(ctx, e, 1);
            if (static_cast<u32>(call_ptr(ctx, vfunc(e, kUiVfnType), {e})) == kUiTypeText)
                call_ptr(ctx, vfunc(e, kUiVfnSetText), {e, kSysMsgText, 0});
        }
        // Code 3 (only possible here if the callback or the calls above set
        // it) hides the background again.
        if (code == 3) {
            if (const u32 e = Ui_Find(ctx, kBg, field<u32>(game, kGameSysMsgUi))) Ui_SetVisible(ctx, e, 0);
        }
    }

    // The mouse-style cursors stay hidden while the box is up.
    for (u32 k = 0; k < kCursorCount; ++k) {
        const u32 slot = game + kGameCursors + k * 4;
        u32 cursor = load<u32>(slot);
        if (cursor == 0) {
            const u32 name = static_cast<u32>(call(ctx, ufn::String_Format, {kCursorFormat, k + 1}));
            cursor = Ui_Find(ctx, name, 0);
            store<u32>(slot, cursor);
        }
        if (cursor != 0) Ui_SetVisible(ctx, cursor, 0);
    }
}

// A debug option: on random frames (by the low bit of a simple generator)
// sleep for 16 ms.
void Game_DebugStall(PPUContext& ctx) {
    if (Random_Step() & 1) syscall(ctx, kSysTimerUsleep, {kDebugStallUs});
}

// True when a local user's pad (Player +28) has its connected bit set.
bool Game_AnyPadConnected(u32 session, s32 users) {
    for (s32 i = 0; i < users; ++i) {
        const u32 user = session + kSessionUsers + static_cast<u32>(i) * kSessionUserSize;
        const u32 player = g::game + kGamePlayers + ref<u8>(user + kUserPlayerIndex) * kPlayerSize;
        const u32 pad = load<u32>(player + kPlayerPad);
        if (pad != 0 && (load<u16>(pad + 4) & kPadConnectedBit)) return true;
    }
    return false;
}

// Raises kSysutilPadLost when no local user has had a connected pad for
// four frames in a row, and drops it again once one is back (while the
// watch state is 3).
void Game_WatchPads() {
    const u32 session = load<u32>(kCurrentWorld);
    const s32 users = ref<u8>(session + kSessionUserCount);
    auto& flags = ref<be_u32>(g::sysutil_flags);
    if (ref<u8>(kPadWatchArmed) == 1) {
        if (Game_AnyPadConnected(session, users)) {
            store<s32>(kPadLostCountdown, -1);
            return;
        }
        const s32 countdown = load<s32>(kPadLostCountdown);
        if (countdown == 0) {
            const u32 f = flags;
            ref<u8>(kPadWatchArmed) = 0;
            store<s32>(kPadWatchState, 0);
            flags = f | kSysutilPadLost;
        } else {
            store<s32>(kPadLostCountdown, countdown < 0 ? static_cast<s32>(kPadLostFrames) : countdown - 1);
        }
    } else if (load<s32>(kPadWatchState) == 3 && Game_AnyPadConnected(session, users)) {
        const u32 f = flags;
        store<s32>(kPadLostCountdown, -1);
        ref<u8>(kPadWatchArmed) = 1;
        store<s32>(kPadWatchState, 0);
        flags = f & ~static_cast<u32>(kSysutilPadLost);
    }
}

// Flags the frame buffer as in use (1) or not (0) around a call on it.
void Game_FrameBuffer(PPUContext& ctx, u32 game, u8 in_frame, u32 fn) {
    const u32 ring = load<u32>(load<u32>(game + kGameWorld) + kWorldFrameRing);
    const u32 buffer = load<u32>(ring + kFrameRingCurrent);
    ref<u8>(ring + kFrameRingInFrame) = in_frame;
    call(ctx, fn, {buffer});
}

}  // namespace

// MainState Game_Update(Game* game)
//
// One frame: re-applies the viewport when the display config changed,
// shows the system message box when one is up, runs the per-frame systems,
// both players, the world and the UI, then polls cellSysutil. Returns the
// new g::main_state: kMainResumed (1) normally, kMainPaused (2) while the
// system message, the XMB or a pause flag is up (the world then updates with
// a frame time of 0), kMainQuit (4) on an exit request or when the world
// asks for it. When the state changes (except 0 -> 1), the session's movie
// is paused or resumed and the menu input mode switched on or restored.
DECOMP(0045B858, Game_Update) {
    const u32 game = arg<u32>(ctx, 0);
    auto& state = ref<be_u32>(g::main_state);
    store<u32>(kPrevMainState, state);

    const u16 width = field<u16>(g::config, 4);
    if (load<u16>(kAppliedViewW) != width) Game_ApplyViewport(ctx, game, width);

    state = kMainResumed;
    call(ctx, ufn::FrameArena_Align, {field<u32>(game, kGameFrameArena)});
    Game_UpdateSystemMessage(ctx, game);

    call(ctx, ufn::Time_BeginFrame);
    Game_FrameBuffer(ctx, game, 1, ufn::FrameBuffer_Begin);
    if (ref<u8>(kDebugStall) != 0) Game_DebugStall(ctx);
    call(ctx, ufn::Input_UpdateDevices);
    Game_WatchPads();

    for (u32 k = 0; k < kPlayerCount; ++k) {
        const u32 player = game + kGamePlayers + k * kPlayerSize;
        callf(ctx, ufn::sub_0045B1E8, {player}, {FrameTime(ctx)});
    }

    call(ctx, ufn::DebugHookA);
    call(ctx, ufn::DebugHookB, {load<u32>(kDebugHookArg)});
    call(ctx, 0x0056C184);  // Sysutil_CheckCallback
    const u32 flags = load<u32>(g::sysutil_flags);
    if (flags & kSysutilExitRequested)
        state = kMainQuit;
    else if (flags & (kSysutilMenuOpen | kSysutilPause80 | kSysutilPadLost))
        state = kMainPaused;
    else if (flags & kSysutilResume)
        state = kMainResumed;

    call(ctx, ufn::Triggers_Clear);
    if (state == kMainPaused) {
        const u32 world = load<u32>(game + kGameWorld);
        ctx.fpr[1] = 0.0;  // the constant just before this function
        call_ptr(ctx, vfunc(world, kWorldVfnUpdate), {world});
    } else {
        const u32 world = load<u32>(game + kGameWorld);
        ctx.fpr[1] = FrameTime(ctx);
        call_ptr(ctx, vfunc(world, kWorldVfnUpdate), {world});
        const u32 again = load<u32>(game + kGameWorld);
        if (static_cast<u32>(call_ptr(ctx, vfunc(again, kWorldVfnPoll), {again, 0})) == kWorldPollQuit)
            state = kMainQuit;
    }
    if (state != kMainPaused) callf(ctx, ufn::sub_004D4538, {}, {FrameTime(ctx)});

    const u32 world_part = load<u32>(game + kGameWorld) + kWorldUpdate2;
    callf(ctx, ufn::sub_0044B310, {world_part}, {FrameTime(ctx)});
    call(ctx, ufn::sub_0045A9B8, {load<u32>(game + kGameWorld)});
    Game_FrameBuffer(ctx, game, 0, ufn::FrameBuffer_End);
    call(ctx, ufn::Systems_EndFrame);

    const s32 prev = load<s32>(kPrevMainState);
    const s32 now = static_cast<s32>(state.get());
    const u32 session = load<u32>(kCurrentWorld);
    if (prev != now && !(prev == 0 && now == 1)) {
        const u32 movie = static_cast<u32>(call(ctx, ufn::FlowState_GetMovieOwner, {session}));
        auto& session_flags = ref<u8>(session + kSessionFlags);
        if (now == static_cast<s32>(kMainPaused)) {
            // Pausing: remember the menu mode, switch it on, pause the movie.
            const u32 mode = static_cast<u32>(call(ctx, ufn::Audio_IsPaused));
            session_flags = static_cast<u8>((session_flags & ~0x40) | ((mode & 1) << 6));
            if ((mode & 1) == 0) call(ctx, ufn::Audio_SetPaused, {1});
            if (movie != 0) call(ctx, ufn::Movie_SetPaused, {movie, 1});
        } else {
            if ((session_flags & 0x40) == 0) call(ctx, ufn::Audio_SetPaused, {0});
            if (movie != 0) call(ctx, ufn::Movie_SetPaused, {movie, 0});
        }
    }
    ret(ctx, state.get());
}
