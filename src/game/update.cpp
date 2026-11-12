// Game_Update: the per-frame update main() calls once per frame.
#include "update.h"

#include "frame.h"
#include "load.h"
#include "player.h"
#include "../system/subsystems.h"
#include "../ui/ui.h"

namespace sly4::update {
using namespace opentit::native;
using gameload::FindElem;

namespace {

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
constexpr f32 kPausedFrameTime = 0.0f;       // the constant just before Game_Update

// The Game object (g::game) as Game_Update uses it.
struct Game {
    gp<frame::World> world;            // +0
    gp<FindElem> sys_msg_ui;           // +4 "Common.System_Message", 0 while not shown
    pad<152> _8;
    gp<FrameArena> frame_arena;        // +0xA0
    pad<12> _a4;
    player::Player players[2];         // +0xB0
    pad<kGameCursors - kGamePlayers - 2 * kPlayerSize> _5e30;
    gp<FindElem> cursors[2];           // +0x5E30, found lazily
};
static_assert(offsetof(Game, frame_arena) == kGameFrameArena && offsetof(Game, players) == kGamePlayers &&
              offsetof(Game, cursors) == kGameCursors);
static_assert(sizeof(player::Player) == kPlayerSize);

FindElem* Ui_Find(u32 name, FindElem* parent) { return gameload::Ui_Find(host<const char>(name), parent); }
void Ui_SetVisible(FindElem* e, u32 visible) { vcall(e, kUiVfnSetVisible, visible); }

// The frame time every update gets: SystemState +4.
f32 FrameTime() { return host<SystemState>(system_state::get_native())->frame_time; }

// The display config changed (config u16 +4 differs from the one applied
// last): hand the new rectangle to every view of the world and to the main
// view, then the main view's matrix (the 4x4 at kViewMatrix with only x, y
// and z of each row copied; the w slots of the copy are left unset, as in
// the original, and sub_005201B0 never reads them).
void Game_ApplyViewport(Game* game, u16 width) {
    global<be_u16>(kAppliedViewW) = width;
    GuestFrame frame(64 + sizeof(ViewRect));
    be_u32* matrix = frame.as<be_u32>();
    auto* rect = frame.as<gameload::ViewRect>(64);
    rect->x = gref<be_u16>(g::config + 8).get();
    rect->y = gref<be_u16>(g::config + 10).get();
    rect->width = width;
    rect->height = gref<be_u16>(g::config + 6).get();

    frame::World* world = game->world;
    for (u32 k = 0; k < kViewCount; ++k) {
        const u32 view = gref<frame::World>(guest(world)).views[k].ea;
        if (view != 0) gameload::RenderTarget_SetViewport(host<gameload::RenderTarget>(view), rect);
    }
    gameload::RenderTarget_SetViewport(host<gameload::RenderTarget>(global<be_u32>(kMainView)), rect);

    const be_u32* src = host<be_u32>(kViewMatrix);
    for (u32 row = 0; row < 4; ++row)
        for (u32 col = 0; col < 3; ++col) matrix[row * 4 + col] = src[row * 4 + col].get();
    call(ufn::sub_005201B0, global<be_u32>(kMainView).get(), matrix);
}

// Shows the system message box over everything while one is up. The first
// frame it's up, FrontEnd, InGame, Common.Fade and Common.Loading are
// hidden (Fade's and Loading's state saved), the UI handlers saved and
// replaced with the system message's own, and event 12 sent. Returns with
// g::main_state = kMainPaused while it's up.
void Game_UpdateSystemMessage(Game* game) {
    be_s32& code_slot = global<be_s32>(kSysMsgCode);
    be_u32& active_slot = global<be_u32>(kSysMsgActive);
    s32 code = code_slot;
    u32 active = active_slot;
    // Codes 0, 1 and 3 are cleared without being shown. Only then, if the
    // message was active, is the callback called; a message
    // that stays up never reaches it. (This follows the disassembly at
    // 0x0045B8E0..0x0045B98C; the old decompilation had the test the other
    // way round.) The original stores the two zeroes twice when inactive.
    if (code == 0 || code == 1 || code == 3) {
        active_slot = 0u;
        code_slot = 0;
        code = 0;
        if (active != 0) {
            active = 0;
            if (const u32 callback = global<be_u32>(kSysMsgCallback)) {
                // r3 is the code address from the descriptor (lwz r3 / mtctr), r4 the
                // cleared active flag (0), as the original leaves them.
                call_ptr(callback, *host<be_u32>(callback), 0u);
                code = code_slot;
                active = active_slot;
            }
        }
    }
    if (active == 0) return;
    if (global<u8>(kSysMsgShows + static_cast<u32>(code)) == 0) return;

    global<be_u32>(g::main_state) = kMainPaused;
    if (!game->sys_msg_ui) {
        // Save the UI handlers, last word first.
        global<be_u32>(kSavedUiEventArg) = global<be_u32>(kUiEventArg).get();
        global<be_u32>(kSavedUiEventHandler) = global<be_u32>(kUiEventHandler).get();
        global<be_u32>(kSavedUiInputArg) = global<be_u32>(kUiInputArg).get();
        global<be_u32>(kSavedUiInputHandler) = global<be_u32>(kUiInputHandler).get();

        if (FindElem* e = Ui_Find(kFrontEnd, nullptr)) Ui_SetVisible(e, 0);
        if (FindElem* e = Ui_Find(kInGame, nullptr)) Ui_SetVisible(e, 0);
        if (FindElem* e = Ui_Find(kFade, nullptr)) {
            global<u8>(kSavedFadeState) = static_cast<u8>(vcall<u32>(e, kUiVfnState));
            Ui_SetVisible(e, 0);
        }
        if (FindElem* e = Ui_Find(kLoading, nullptr)) {
            global<u8>(kSavedLoadingState) = static_cast<u8>(vcall<u32>(e, kUiVfnState));
            Ui_SetVisible(e, 0);
        }
        Audio_SetPaused(1);
        Ui_SetInputHandler(kSysMsgInputHandler, 0);
        Ui_SetEventHandler(kSysMsgEventHandler, guest(game));
        // Game +4 is still 0 here unless one of the calls above set it.
        Ui_SendEvent(kUiEventSystemMessage, 0xFFFFFFFFu, game->sys_msg_ui.ea, 0);
        code = code_slot;
    }

    FindElem* box = Ui_Find(kSystemMessage, nullptr);
    game->sys_msg_ui = box;
    if (box) {
        Ui_SetVisible(box, 1);
        if (FindElem* e = Ui_Find(kBg, game->sys_msg_ui)) Ui_SetVisible(e, 1);
        for (const u32 name : {kText, kBgText}) {
            FindElem* e = Ui_Find(name, game->sys_msg_ui);
            if (!e) continue;
            Ui_SetVisible(e, 1);
            if (vcall<u32>(e, kUiVfnType) == kUiTypeText) vcall(e, kUiVfnSetText, kSysMsgText, 0u);
        }
        // Code 3 (only possible here if the callback or the calls above set
        // it) hides the background again.
        if (code == 3) {
            if (FindElem* e = Ui_Find(kBg, game->sys_msg_ui)) Ui_SetVisible(e, 0);
        }
    }

    // The mouse-style cursors stay hidden while the box is up.
    for (u32 k = 0; k < kCursorCount; ++k) {
        FindElem* cursor = game->cursors[k];
        if (!cursor) {
            const u32 name = call<u32>(ufn::String_Format, kCursorFormat, k + 1);  // variadic
            cursor = Ui_Find(name, nullptr);
            game->cursors[k] = cursor;
        }
        if (cursor) Ui_SetVisible(cursor, 0);
    }
}

// A debug option: on random frames (by the low bit of a simple generator)
// sleep for 16 ms.
void Game_DebugStall() {
    if (Random_Step() & 1) lv2::syscall(kSysTimerUsleep, kDebugStallUs);
}

// True when a local user's pad (Player +28) has its connected bit set.
bool Game_AnyPadConnected(u32 session, s32 users) {
    for (s32 i = 0; i < users; ++i) {
        const u32 user = session + kSessionUsers + static_cast<u32>(i) * kSessionUserSize;
        const player::Player& p = host<Game>(g::game)->players[gref<u8>(user + kUserPlayerIndex)];
        const player::Pad* pad = p.device;
        if (pad && (pad->flags & kPadConnectedBit)) return true;
    }
    return false;
}

// Raises kSysutilPadLost when no local user has had a connected pad for
// four frames in a row, and drops it again once one is back (while the
// watch state is 3).
void Game_WatchPads() {
    const u32 session = global<be_u32>(kCurrentWorld);
    const s32 users = gref<u8>(session + kSessionUserCount);
    be_u32& flags = global<be_u32>(g::sysutil_flags);
    be_s32& countdown_slot = global<be_s32>(kPadLostCountdown);
    if (global<u8>(kPadWatchArmed) == 1) {
        if (Game_AnyPadConnected(session, users)) {
            countdown_slot = -1;
            return;
        }
        const s32 countdown = countdown_slot;
        if (countdown == 0) {
            const u32 f = flags;
            global<u8>(kPadWatchArmed) = 0;
            global<be_s32>(kPadWatchState) = 0;
            flags = f | kSysutilPadLost;
        } else {
            countdown_slot = countdown < 0 ? static_cast<s32>(kPadLostFrames) : countdown - 1;
        }
    } else if (global<be_s32>(kPadWatchState) == 3 && Game_AnyPadConnected(session, users)) {
        const u32 f = flags;
        countdown_slot = -1;
        global<u8>(kPadWatchArmed) = 1;
        global<be_s32>(kPadWatchState) = 0;
        flags = f & ~static_cast<u32>(kSysutilPadLost);
    }
}

// Flags the frame buffer as in use (1) or not (0) around a call on it.
FrameRingBuffer* Game_FrameBuffer(Game* game, u8 in_frame) {
    frame::FrameRing* ring = game->world->frame_ring;
    FrameRingBuffer* buffer = host<FrameRingBuffer>(ring->current.ea);
    ring->in_frame = in_frame;
    return buffer;
}

}  // namespace

// One frame: re-applies the viewport when the display config changed,
// shows the system message box when one is up, runs the per-frame systems,
// both players, the world and the UI, then polls cellSysutil. Returns the
// new g::main_state: kMainResumed (1) normally, kMainPaused (2) while the
// system message, the XMB or a pause flag is up (the world then updates with
// a frame time of 0), kMainQuit (4) on an exit request or when the world
// asks for it. When the state changes (except 0 -> 1), the session's movie
// is paused or resumed and the menu input mode switched on or restored.
u32 Game_Update(Game* game) {
    be_u32& state = global<be_u32>(g::main_state);
    global<be_u32>(kPrevMainState) = state.get();

    const u16 width = gref<be_u16>(g::config + 4);
    if (global<be_u16>(kAppliedViewW) != width) Game_ApplyViewport(game, width);

    state = kMainResumed;
    FrameArena_Align(game->frame_arena);
    Game_UpdateSystemMessage(game);

    Time_BeginFrame();
    FrameBuffer_Begin(Game_FrameBuffer(game, 1));
    if (global<u8>(kDebugStall) != 0) Game_DebugStall();
    Input_UpdateDevices();
    Game_WatchPads();

    for (u32 k = 0; k < kPlayerCount; ++k) player::Player_Update(&game->players[k], FrameTime());

    DebugHookA();
    DebugHookB();
    system::Sysutil_CheckCallback();
    const u32 flags = global<be_u32>(g::sysutil_flags);
    if (flags & kSysutilExitRequested)
        state = kMainQuit;
    else if (flags & (kSysutilMenuOpen | kSysutilNpOffline | kSysutilPadLost))
        state = kMainPaused;
    else if (flags & kSysutilResume)
        state = kMainResumed;

    Triggers_Clear();
    if (state == kMainPaused) {
        vcall(game->world.get(), kWorldVfnUpdate, kPausedFrameTime);
    } else {
        vcall(game->world.get(), kWorldVfnUpdate, FrameTime());
        if (vcall<u32>(game->world.get(), kWorldVfnPoll, 0u) == kWorldPollQuit) state = kMainQuit;
    }
    if (state != kMainPaused) ui::Ui_Update(FrameTime());

    frame::Audio_Update(reinterpret_cast<frame::Listener*>(game->world->audio), FrameTime());
    frame::World_SubmitFrame(game->world);
    FrameBuffer_End(Game_FrameBuffer(game, 0));
    Systems_EndFrame();

    const s32 prev = global<be_s32>(kPrevMainState);
    const s32 now = static_cast<s32>(state.get());
    const u32 session = global<be_u32>(kCurrentWorld);
    if (prev != now && !(prev == 0 && now == 1)) {
        const u32 movie = FlowState_GetMovieOwner(host<Flow>(session));
        u8& session_flags = gref<u8>(session + kSessionFlags);
        if (now == static_cast<s32>(kMainPaused)) {
            // Pausing: remember the menu mode, switch it on, pause the movie.
            const u32 mode = Audio_IsPaused();
            session_flags = static_cast<u8>((session_flags & ~0x40) | ((mode & 1) << 6));
            if ((mode & 1) == 0) Audio_SetPaused(1);
            if (movie != 0) Movie_SetPaused(host<Movie>(movie), 1);
        } else {
            if ((session_flags & 0x40) == 0) Audio_SetPaused(0);
            if (movie != 0) Movie_SetPaused(host<Movie>(movie), 0);
        }
    }
    return state;
}

}  // namespace sly4::update

NATIVE(0045B858, sly4::update::Game_Update)
