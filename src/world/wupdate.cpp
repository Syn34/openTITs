// World_Update (World vfunc +16): the world's per-frame update, called by
// Game_Update with the frame time (0 while paused).
#include "wupdate.h"

using namespace sly4;
using namespace sly4::wupdate;

namespace {

constexpr u32 kCommonCursors = 0x00AE87AC;   // "Common.Cursors"
constexpr u32 kCursorName = 0x00AE87BC;      // "Cursor%02d"
constexpr u32 kInGameHud = 0x00AE8554;       // "InGame.HUD"
constexpr u32 kCursorPath = 0x00AE82AC;      // "Common.Cursors.%02d"
constexpr u32 kStrNoControllers = 0x00AE87E4;  // "STR_NO_CONTROLLERS"
constexpr u32 kStrMustBeSignedIn = 0x00AE87CC;  // "STR_MUST_BE_SIGNED_IN"
constexpr u32 kSaveDat = 0x00AE84A4;         // "save.dat"
constexpr u32 kLoadingSoundHash = 0x436FF9ABu;  // given to sub_0044C43C (name?)
constexpr u32 kOneBits = 0x3F800000u, kMinusOneBits = 0xBF800000u;
constexpr u32 kPadWaitState = 3;             // kPadPromptState after "no controllers" (Game_WatchPads)

// The locals the original hands out by address: one block standing for
// r1+112..r1+207 of its frame, so callees see the same layout.
struct Locals {
    explicit Locals(PPUContext& ctx) : frame(ctx, 96) {}
    u32 at(u32 r1_offset) const { return frame.addr() + (r1_offset - 112); }
    Stack frame;
};

u32 GetState(PPUContext& ctx, u32 w, u32 level) {
    return static_cast<u32>(wcall(ctx, w, world::kVfnGetState, {w, level}));
}
void SetState(PPUContext& ctx, u32 w, u32 level, u32 id) { call(ctx, wfn::StateMachine_SetState, {w, level, id}); }
u32 Resolve(PPUContext& ctx, u32 handle_global) {
    return static_cast<u32>(call(ctx, wfn::Handle_Resolve, {load<u32>(handle_global)}));
}
// The resolved object's mode (+160, bit 0 dropped).
u32 ResolvedMode(PPUContext& ctx, u32 handle_global) { return load<u32>(Resolve(ctx, handle_global) + 160) >> 1; }
void SetVisible(PPUContext& ctx, u32 ui, u32 visible) { wcall(ctx, ui, kUiSetVisible, {ui, visible}); }
u32 Player(u32 i) { return kGame + kGamePlayers + i * kPlayerSize; }
f32 PhaseTime(u32 w) { return load<f32>(w + kWorldPhaseTime); }
void SetPhase(u32 w, u32 phase) {
    ref<u8>(w + kWorldPhase) = static_cast<u8>(phase);
    store<u32>(w + kWorldPhaseTime, 0);
}

// ---------------------------------------------------------------- phases
// World +50 drives level-0 state changes (World_RequestState starts it at
// 1): 1 wait for the loaders, then fade out; 2 leave the state; 5-8 the
// save and intro steps; 9-12 enter the requested state; 13-16 settle.
// Phase 1 (0x0046A500): on its first frame, hide the cursors and kick the
// loaders; then wait until they are idle (or a loader reports mode 3),
// fade out and go to phase 2 (or 16 if the world is in state 3).
void Phase_WaitForLoaders(PPUContext& ctx, u32 w) {
    if (PhaseTime(w) == kZero) {
        if (const u32 cursors = static_cast<u32>(call(ctx, wfn::Ui_Find, {kCommonCursors, 0}))) SetVisible(ctx, cursors, 0);
        if (Resolve(ctx, kHandleD) != 0) {
            call(ctx, wfn::sub_0036CA78, {Resolve(ctx, kHandleD)});
            call(ctx, wfn::sub_004A7EC0, {Resolve(ctx, kHandleD)});
            call(ctx, wfn::sub_0036CC28, {Resolve(ctx, kHandleA)});
        } else if (Resolve(ctx, kHandleB) != 0) {
            call(ctx, wfn::sub_0036CA78, {Resolve(ctx, kHandleB)});
            call(ctx, wfn::sub_004A7EC0, {Resolve(ctx, kHandleB)});
        }
        store<f32>(w + kWorldPhaseTime, static_cast<f32>(static_cast<f64>(PhaseTime(w)) + kPhaseStep));
        const u64 sound = call(ctx, wfn::sub_0044C43C, {kLoadingSoundHash});
        if (static_cast<u32>(sound) != 0) callf(ctx, wfn::sub_0044D5BC, {sound}, {kHalf});
        return;
    }

    bool go = Resolve(ctx, kHandleC) != 0 && ResolvedMode(ctx, kHandleC) == 3;
    if (!go) go = Resolve(ctx, kHandleD) != 0 && ResolvedMode(ctx, kHandleD) == 3;
    if (!go) go = Resolve(ctx, kHandleD) == 0 && Resolve(ctx, kHandleA) != 0 && ResolvedMode(ctx, kHandleA) == 3;
    if (!go) go = Resolve(ctx, kHandleD) == 0 && Resolve(ctx, kHandleB) != 0 && ResolvedMode(ctx, kHandleB) == 3;
    if (!go) {
        if (Resolve(ctx, kHandleA) != 0) return;
        if (Resolve(ctx, kHandleD) != 0) return;
        if (Resolve(ctx, kHandleB) != 0) return;
        if (load<u32>(w + kWorldBusy) & 0x40000000u) return;
    }

    ref<u8>(w + kWorldPhase) = 2;
    const u32 fade = load<u32>(kFadeUi);
    store<u32>(w + kWorldPhaseTime, 0);
    if (fade != 0) SetVisible(ctx, fade, 0);
    if (ref<u8>(w + kWorldSkipSave) != 0) {
        ref<u8>(w + kWorldSkipSave) = 0;
    } else {
        call(ctx, wfn::sub_000AC698);
        const u32 v = static_cast<u32>(call(ctx, wfn::sub_000AC83C));
        store<u32>(kGame + kGameValue5E38, v);
        ref<u8>(kGame + kGameFlag5E3C) = 1;
    }
    if (static_cast<s32>(GetState(ctx, w, 0)) == 3) {
        ref<u8>(kFlag66FC0) = 0;
        store<u32>(w + kWorldPhaseTime, 0);
        ref<u8>(kFlag88958) = 0;
        ref<u8>(w + kWorldPhase) = 16;
    }
}

// Phase 2 (0x0046B388): leave the level-0 state.
void Phase_Leave(PPUContext& ctx, u32 w) {
    SetState(ctx, w, 0, world::kSmNone);
    const u32 flags28 = ref<u8>(w + kWorldFlags28);
    const u32 next = ref<u8>(w + kWorldNextState);
    u32 phase = 9;
    store<u32>(w + kWorldPhaseTime, 0);
    if ((flags28 & 0x20) && (ref<u8>(w + kWorldSaveFlags) & 0x80)) phase = 5;
    ref<u8>(w + kWorldPhase) = static_cast<u8>(phase);
    if (next == 2 || next == 3) {
        call(ctx, wfn::sub_000AC39C);
    } else {
        ref<u8>(kGame + kGameFlags5E40) |= 0x80;
        ref<u8>(kFlag9CDE2) = 1;
    }
    call(ctx, wfn::sub_004E3040, {0, 0});
}

// Phase 15 (0x0046A7F8).
void Phase_Settle(PPUContext& ctx, u32 w) {
    if (!(PhaseTime(w) == kZero)) {
        if (GetState(ctx, w, 0) == 0) ref<u8>(kGame + kGameFlags5E40) &= static_cast<u8>(~0x80);
        if (!(Resolve(ctx, kHandleA) != 0 && ResolvedMode(ctx, kHandleA) == 0)) {
            if (Resolve(ctx, kHandleA) != 0) return;
            if (load<u32>(w + kWorldBusy) & 0x40000000u) return;
        }
        ref<u8>(w + kWorldPhase) = 0;
        store<u32>(w + kWorldPhaseTime, 0);
        return;
    }

    // First frame.
    if (load<u64>(kPending9CDE8) != 0) return;
    const auto flagged = [] {
        return ref<u8>(kTuneFlagA + kTuning) != 0 || ref<u8>(kTuneFlagB + kTuning) != 0;
    };
    const auto in_menu = [&] {
        return GetState(ctx, w, 0) == 0 && static_cast<s32>(GetState(ctx, w, 1)) <= 6;
    };
    if (flagged() && in_menu()) {
        if (Resolve(ctx, kHandleA) != 0) {
            const u32 o = Resolve(ctx, kHandleA);
            wcall(ctx, o, 20, {o, 0});
        }
    } else if (Resolve(ctx, kHandleA) != 0) {
        call(ctx, wfn::sub_0036CB58, {Resolve(ctx, kHandleA)});
    } else {
        // Fade in from black.
        Locals l(ctx);
        const u32 from = l.at(176), to = l.at(180);
        for (u32 k = 0; k < 3; ++k) ref<u8>(from + k) = 0;
        ref<u8>(from + 3) = 255;
        for (u32 k = 0; k < 4; ++k) ref<u8>(to + k) = 0;
        callf(ctx, wfn::Fade_Start, {w + kWorldFade, from, to, 255, 1, 0},
              {static_cast<f64>(load<f32>(kTuneFadeTime + kTuning))});
    }
    store<f32>(w + kWorldPhaseTime, static_cast<f32>(static_cast<f64>(PhaseTime(w)) + kPhaseStep));
    if (flagged() && in_menu()) return;
    call(ctx, wfn::Audio_SetPaused, {0});
}

void World_RunPhase(PPUContext& ctx, u32 w) {
    const u32 phase = ref<u8>(w + kWorldPhase);
    switch (phase) {
    case 0:
        break;
    case 1:
        Phase_WaitForLoaders(ctx, w);
        break;
    case 2:
        Phase_Leave(ctx, w);
        break;
    case 5: {  // wait out the intro time if this state shows one
        u32 next = 9;
        if ((ref<u8>(w + kWorldFlags28) & 0x20) && (ref<u8>(w + kWorldSaveFlags) & 0x80)) {
            const f32 t = load<f32>(w + kWorldTime), wait = load<f32>(kTuneIntroTime + kTuning);
            if (t > wait || t == wait) next = phase + 1;
        }
        SetPhase(w, next);
        break;
    }
    case 7:  // save
        call(ctx, wfn::SaveData_Write, {kSaveDat, w + kWorldSaveData, kWorldSaveSize});
        {
            const u32 next = ref<u8>(w + kWorldPhase) + 1u;
            store<u32>(w + kWorldPhaseTime, 0);
            ref<u8>(w + kWorldPhase) = static_cast<u8>(next);
        }
        break;
    case 8: {
        const u32 flags28 = ref<u8>(w + kWorldFlags28);
        store<u32>(w + kWorldTime, 0);
        ref<u8>(w + kWorldPhase) = 9;
        store<u32>(w + kWorldPhaseTime, 0);
        ref<u8>(w + kWorldFlags28) = static_cast<u8>(flags28 & ~0x20u);
        break;
    }
    case 9:
        SetPhase(w, ref<u8>(w + kWorldNextState) == 1 ? 10 : 12);
        break;
    case 10:
        if (!(load<u32>(w + kWorldBusy) & 0x40000000u)) SetPhase(w, 12);
        break;
    case 12: {  // enter the requested states
        SetState(ctx, w, 0, ref<u8>(w + kWorldNextState));
        SetState(ctx, w, 1, ref<u8>(w + kWorldNextSub));
        const u32 next = ref<u8>(w + kWorldPhase) + 1u;
        ref<u8>(w + kWorldCurState) = ref<u8>(w + kWorldNextState);
        ref<u8>(w + kWorldNextSub) = 0;
        store<u32>(w + kWorldPhaseTime, 0);
        ref<u8>(w + kWorldPhase) = static_cast<u8>(next);
        break;
    }
    case 13: {  // one update with no time
        callf(ctx, wfn::StateMachine_Update, {w}, {0.0});
        const u32 next = ref<u8>(w + kWorldNextState) == 1 ? 14 : 15;
        store<u32>(w + kWorldPhaseTime, 0);
        ref<u8>(w + kWorldPhase) = static_cast<u8>(next);
        break;
    }
    case 14:
        if (PhaseTime(w) == kZero) {
            ref<u8>(w + kWorldPhase) = 0;
            SetState(ctx, w, 1, 0);
            SetState(ctx, w, 2, 0);
            store<u32>(w + kWorldPhaseTime, 0);
        }
        break;
    case 15:
        Phase_Settle(ctx, w);
        break;
    case 16:
        ref<u8>(w + kWorldPhase) = 0;
        call(ctx, wfn::World_RequestState, {w, 1, 0});
        break;
    default:  // 3, 4, 6, 11, 17..255: next phase
        SetPhase(w, phase + 1);
        break;
    }
}

// ---------------------------------------------------------------- cursors
// Shows "Common.Cursors.CursorNN" for the players in `mask` whose cursor is
// active, at their cursor position, in the tuned colour (the "on" colour if
// they are in kCursorMask); hides the rest, then the container.
void World_UpdateCursors(PPUContext& ctx, u32 cursors, u32 mask, const Locals& l) {
    for (u32 i = 0; i < kPlayerCount; ++i) {
        const u32 name = static_cast<u32>(call(ctx, wfn::String_Format, {kCursorName, i + 1}));
        const u32 ui = static_cast<u32>(call(ctx, wfn::Ui_Find, {name, cursors}));
        if (ui == 0) continue;
        const u32 bit = static_cast<u32>(rt::slw(1, i));
        const u32 player = Player(i);
        if ((bit & mask) == 0 || !(load<u32>(player + kPlayerFlags) & 0x00800000u)) {
            SetVisible(ctx, ui, 0);
            continue;
        }
        const f32 x = load<f32>(player + kPlayerCursor), y = load<f32>(player + kPlayerCursor + 4);
        call(ctx, wfn::Display_GetSize, {l.at(160)});
        const u32 pos = l.at(112);
        store<u32>(pos + 8, 0);  // z; the word after it is left as it is, as in the original
        const f32 px = static_cast<f32>(static_cast<f32>(x + kOne) * load<f32>(l.at(160)));
        const f32 py = static_cast<f32>(static_cast<f32>(y + kOne) * load<f32>(l.at(164)));
        store<f32>(pos, px * kHalf);
        store<f32>(pos + 4, py * kHalf);
        wcall(ctx, ui, kUiSetPosition, {ui, pos});
        if (load<u32>(kCursorMask) & bit) {
            const u32 c = kTuneCursorColorOn + kTuning;
            Ui_SetColor(ctx, ui, rgba_at(c), ref<u8>(c + 3));
        } else {
            const u32 c = kTuneCursorColorOff + kTuning;
            Ui_SetColor(ctx, ui, rgba_at(c), ref<u8>(c + 2));
        }
        call(ctx, wfn::Ui_BringToFront, {ui, 1});
        SetVisible(ctx, ui, 1);
    }
    SetVisible(ctx, cursors, 0);
}

// The world step when no cutscene sets it: dt clamped to
// [1 / max fps, 1 / min fps], as the original's compares do it.
f64 ClampStep(f64 dt) {
    const f32 max_fps = static_cast<f32>(static_cast<f64>(load<s32>(kTuneMaxFps + kTuning)));
    const f32 min_fps = static_cast<f32>(static_cast<f64>(load<s32>(kTuneMinFps + kTuning)));
    const f32 longest = static_cast<f32>(kOne / static_cast<f64>(min_fps));
    const f32 shortest = static_cast<f32>(kOne / static_cast<f64>(max_fps));
    const f64 m = dt < longest ? dt : static_cast<f64>(longest);
    return shortest < m ? m : static_cast<f64>(shortest);
}

// While the world clock is stopped: walk the PSN ("must be signed in") and
// controller ("no controllers") prompts. State 0 raises the dialog (1 if it
// opened, else 2), 1 waits for it to close, 2 drops the sysutil flag.
void World_UpdatePrompt(PPUContext& ctx, u32 state_ea, u32 string_id, u32 flag, u32 done_state) {
    const s32 state = load<s32>(state_ea);
    if (state == 0) {
        u32 next = 2;
        const u32 strings = load<u32>(kStrings);
        if (strings != 0) {
            const u32 msg = static_cast<u32>(call(ctx, wfn::StringTable_Lookup, {strings, string_id, 0, 0}));
            if (static_cast<u32>(call(ctx, wfn::MsgDialog_Open, {msg, 4})) == 1) next = 1;
        }
        store<u32>(state_ea, next);
    } else if (state == 1) {
        if (load<u32>(kDialogState) != 0) {
            call(ctx, wfn::Sysutil_CheckCallback);
            if (load<s32>(kDialogState) == 3) return;
            callf(ctx, dlg::cellMsgDialogClose, {}, {0.0});
            const s32 was = load<s32>(kDialogState);
            store<u32>(kDialogState, 0);
            if (was == 3) return;
        }
        store<u32>(state_ea, 2);
    } else if (state == 2) {
        const u32 flags = load<u32>(g::sysutil_flags) & ~flag;
        store<u32>(state_ea, done_state);
        store<u32>(g::sysutil_flags, flags);
    }
}

// Copies the main view's matrix (rows 0-2) to World +0x128230 and its row
// 3 to +0x128260, blending +0x128270 between +0x128278 and +0x128274 over
// +0x128280 seconds; outside cutscenes row 3 is then pulled toward the
// followed object by that blend.
void World_UpdateCamera(PPUContext& ctx, u32 w, u32 view, f64 dt, const Locals& l) {
    const u32 v = view + kViewMatrix;
    for (u32 row = 0; row < 3; ++row)
        for (u32 k = 0; k < 3; ++k) store<u32>(w + kWorldCamera + row * 16 + k * 4, load<u32>(v + row * 16 + k * 4));
    const f32 blend_time = load<f32>(w + kWorldCamBlendTime);
    const u32 target = w + kWorldCamTarget;
    for (u32 k = 0; k < 3; ++k) store<u32>(target + k * 4, load<u32>(v + 48 + k * 4));
    u32 raw[3];
    for (u32 k = 0; k < 3; ++k) raw[k] = load<u32>(target + k * 4);

    if (!(std::fabs(static_cast<f64>(blend_time)) < static_cast<f64>(kEpsilon))) {
        f32 t = static_cast<f32>(dt / static_cast<f64>(blend_time));
        t = static_cast<f32>(static_cast<f64>(load<f32>(w + kWorldCamBlendT)) + t);
        store<f32>(w + kWorldCamBlendT, t);
        if (t > kOne) {
            t = kOne;
            store<u32>(w + kWorldCamBlendT, kOneBits);
            store<u32>(w + kWorldCamBlendTime, 0);
        }
        const f64 eased = callf(ctx, wfn::Ease, {}, {t, load<f32>(w + kWorldCamBlendShape)});
        const f32 inv = static_cast<f32>(kOne - eased);
        const f32 from = load<f32>(w + kWorldCamBlendFrom), to = load<f32>(w + kWorldCamBlendTo);
        store<f32>(w + kWorldCamBlend, static_cast<f32>(std::fma(static_cast<f64>(static_cast<f32>(to - from)),
                                                                 static_cast<f64>(inv), static_cast<f64>(from))));
    }

    if (load<u32>(w + kWorldCutscene) == 0) {
        const u32 obj = load<u32>(w + kWorldPlayerObj);
        if (obj != 0 && (load<u32>(obj + 172) & 0x40000u)) {
            f32 p[3] = {load<f32>(obj + 144), load<f32>(obj + 148), 0};
            const u32 flags = load<u32>(obj + 192);
            p[2] = load<f32>(obj + 152);
            if (flags & 1) {
                wcall(ctx, obj, 284, {l.at(128), obj});  // position as a struct return
                for (u32 k = 0; k < 3; ++k) p[k] = load<f32>(l.at(128) + k * 4);
            }
            const f32 blend = load<f32>(w + kWorldCamBlend);
            for (u32 k = 0; k < 3; ++k) {
                const f32 d = std::bit_cast<f32>(raw[k]) - p[k];
                store<f32>(target + k * 4, static_cast<f32>(std::fma(static_cast<f64>(d), static_cast<f64>(blend),
                                                                     static_cast<f64>(p[k]))));
            }
            return;
        }
    }
    for (u32 k = 0; k < 3; ++k) store<u32>(target + k * 4, raw[k]);  // stored back unchanged
}

}  // namespace

// bool World_Update(World* w, f32 dt)
// Runs the per-frame effects (fade, post effects, audio filter), the state
// change phases, the cursors, a running cutscene, then the state machine
// with the scaled world step (or, with the clock stopped, the PSN /
// controller prompts and the paused cutscene callbacks), the HUD, the
// camera copy and the pointer cursors. Returns 0 when a state handled the
// update or 1 otherwise (also when a cutscene ends this frame).
DECOMP(00469D98, World_Update) {
    const u32 w = arg<u32>(ctx, 0);
    const f64 dt = farg(ctx, 0);
    Locals l(ctx);

    callf(ctx, wfn::sub_0042A8B0, {w}, {dt});
    if (const u32 ticker = load<u32>(w + kWorldTicker)) {
        const u32 ss = system_state::get(ctx);
        ctx.fpr[1] = load<f32>(ss + 4);  // the frame time
        wcall(ctx, ticker, 8, {ticker});
    }
    const u32 fade = w + kWorldFade;
    callf(ctx, wfn::Fade_Update, {fade}, {dt});
    callf(ctx, wfn::sub_00787070, {w}, {dt});
    callf(ctx, wfn::sub_00787190, {w}, {dt});
    callf(ctx, wfn::PostFx_UpdateDof, {w + kWorldDof}, {dt});
    callf(ctx, wfn::PostFx_UpdateSwap, {w + kWorldSwap}, {dt});
    callf(ctx, wfn::sub_007872A8, {w}, {dt});
    callf(ctx, wfn::PostFx_UpdateTint, {w + kWorldTint}, {dt});
    callf(ctx, wfn::PostFx_UpdateFilter, {w + kWorldFilter}, {dt});
    const u8 flags45 = ref<u8>(w + kWorldFlags45);
    if (flags45 & 0x02) {
        ref<u8>(w + kWorldFlags45) = static_cast<u8>(flags45 & ~0x02);
        const u32 o = static_cast<u32>(call(ctx, wfn::sub_007D3B80));
        wcall(ctx, o, 64, {o});
    }

    World_RunPhase(ctx, w);

    // Cursors: both players, or in state 1 only the session's users (all of
    // them, or the one at World +30).
    store<u32>(kCursorMask, 3);
    u32 mask = 3;
    if (static_cast<s32>(GetState(ctx, w, 0)) == 1) {
        mask = 0;
        store<u32>(kCursorMask, 0);
        u32 users = ref<u8>(w + kWorldUserCount);
        u32 bits = 0;
        for (u32 i = 0; i < users; ++i) {
            const u32 only = ref<u8>(w + kWorldUserFilter);
            if (only == 255 || i == only) {
                bits |= static_cast<u32>(rt::slw(1, ref<u8>(w + kWorldUsers + i * kUserStride)));
                store<u32>(kCursorMask, bits);
                users = ref<u8>(w + kWorldUserCount);
            }
            mask = bits;
        }
    }
    if (const u32 cursors = static_cast<u32>(call(ctx, wfn::Ui_Find, {kCommonCursors, 0})))
        World_UpdateCursors(ctx, cursors, mask, l);

    // A cutscene runs first and sets the step; it is deleted when its
    // update returns 0.
    u32 cutscene = load<u32>(w + kWorldCutscene);
    if (cutscene != 0) {
        for (u32 k = 0; k < kWorldVectorCount; ++k)
            if (k != 12) store<u16>(w + kWorldVectors + k * kWorldVectorSize + 8, 0);
        ctx.fpr[1] = dt;
        const u32 more = static_cast<u32>(wcall(ctx, cutscene, 16, {cutscene}));
        cutscene = load<u32>(w + kWorldCutscene);
        if (more == 0) {
            if (cutscene != 0) {
                wcall(ctx, cutscene, 4, {cutscene});
                store<u32>(w + kWorldCutscene, 0);
            }
            return ret(ctx, 1u);
        }
    }
    f64 step = cutscene != 0 ? static_cast<f64>(load<f32>(cutscene + 60)) : ClampStep(dt);
    if (dt == static_cast<f64>(kZero)) step = kZero;
    if (load<u32>(w + kWorldTimeScaleOn) & 0x80000000u)
        step = static_cast<f32>(step * static_cast<f64>(load<f32>(w + kWorldTimeScale)));

    // Two slow-motion slots: their scales multiply into the step, and their
    // timers (negative = off) run down by the unscaled step.
    f64 scale = kOne;
    for (u32 i = 0; i < 2; ++i) {
        const u32 slot = w + kWorldSlowMo + i * 12;
        const f32 left = load<f32>(slot + 4);
        scale = static_cast<f32>(scale * static_cast<f64>(load<f32>(slot)));
        if (left > kZero || left == kZero) {
            if (left > step) {
                store<f32>(slot + 4, static_cast<f32>(static_cast<f64>(left) - step));
            } else {
                store<u32>(slot, kOneBits);
                store<u32>(slot + 4, kMinusOneBits);
                ref<u8>(slot + 8) = 0;
            }
        }
    }
    step = static_cast<f32>(step * scale);
    callf(ctx, wfn::World_UpdateAudioFilter, {w}, {dt});

    bool ran = false;
    if (step > static_cast<f64>(kEpsilon) || step == static_cast<f64>(kEpsilon)) {
        ran = true;
        callf(ctx, wfn::StateMachine_Update, {w}, {step});
        if (static_cast<u32>(ctx.gpr[3]) == 0) return ret(ctx, 0u);  // a state handled it
        store<f32>(w + kWorldTime, static_cast<f32>(static_cast<f64>(load<f32>(w + kWorldTime)) + step));
    } else {
        const u32 flags = load<u32>(g::sysutil_flags);
        if (!(flags & kSysutilMenuOpen)) {
            if (load<u32>(g::sysutil_flags) & kSysutilNpOffline)
                World_UpdatePrompt(ctx, kNpPromptState, kStrMustBeSignedIn, kSysutilNpOffline, 0);
            else if (load<u32>(g::sysutil_flags) & kSysutilPadLost)
                World_UpdatePrompt(ctx, kPadPromptState, kStrNoControllers, kSysutilPadLost, kPadWaitState);
        }
        callf(ctx, wfn::sub_003655D0, {}, {0.0});
    }

    // A cutscene with the clock stopped: its callbacks and camera.
    if (load<u32>(w + kWorldCutscene) != 0 && !ran) {
        const u32 list = w + kWorldPausedCallbacks;
        for (u32 i = 0, off = 0; i < load<u16>(list + 8); ++i, off += 12) {
            const u32 entry = load<u32>(list) + off;
            const u32 opd = load<u32>(entry);
            const u32 a = load<u32>(entry + 4);
            ctx.fpr[1] = 0.0;
            ctx.gpr[5] = opd;
            call_ptr(ctx, opd, {load<u32>(opd), a});  // r3 = the code address, as in the original
        }
        call(ctx, wfn::Cutscene_ApplyCamera, {load<u32>(w + kWorldCutscene)});
    }

    if (const u32 hud = static_cast<u32>(call(ctx, wfn::Ui_Find, {kInGameHud, 0}))) {
        const u32 show = load<u32>(w + kWorldCutscene) == 0 && ref<u8>(w + kWorldSubState) == 0 &&
                                 load<u16>(w + kWorldHudLock) == 0
                             ? 1
                             : 0;
        SetVisible(ctx, hud, show);
    }
    if (const u32 view = load<u32>(w + kWorldViews)) World_UpdateCamera(ctx, w, view, dt, l);

    // Pointer cursors: "Common.Cursors.01/02" at the pointer's position,
    // which is also kept per player in -1..1.
    const u32 pointer = load<u32>(w + kWorldPointer);
    if (pointer == 0) return ret(ctx, 1u);
    const u32 pos = l.at(144), size = l.at(168);
    wcall(ctx, pointer, 144, {pos, pointer});
    for (u32 i = 1; i <= kPlayerCount; ++i) {
        const u32 name = static_cast<u32>(call(ctx, wfn::String_Format, {kCursorPath, i}));
        if (const u32 ui = static_cast<u32>(call(ctx, wfn::Ui_Find, {name, 0}))) wcall(ctx, ui, kUiSetPosition, {ui, pos});
        const u32 player = Player(i - 1);
        call(ctx, wfn::Display_GetSize, {size});
        const f32 x = static_cast<f32>(static_cast<f32>(load<f32>(pos) * kTwo) / load<f32>(size));
        const f32 y = static_cast<f32>(static_cast<f32>(load<f32>(pos + 4) * kTwo) / load<f32>(size + 4));
        store<f32>(player + kPlayerCursor, x - kOne);
        store<f32>(player + kPlayerCursor + 4, y - kOne);
    }
    ret(ctx, 1u);
}
