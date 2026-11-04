// The out-of-line helpers of World_Update: state requests, the message
// dialog, the colour fade, the camera post effects (filter, tint, depth of
// field, swap), the audio filter and the cutscene camera.
#include "wupdate.h"

using namespace sly4;
using namespace sly4::wupdate;

namespace {

constexpr f32 kFilterOpen = 23999.0f;         // 0x46BB7E00: cutoff when nothing is muffled
constexpr u32 kFilterOpenBits = 0x46BB7E00u;
constexpr f32 kByteToUnit = 1.0f / 255.0f;    // 0x3B808081
constexpr f32 kDofMinFar = 0.01f;             // 0x3C23D70A
constexpr f32 kDofMaxFar = 1000.0f;           // 0x4479C000
constexpr f32 kByteMax = 255.0f;              // 0x437F0000

// World +0x1281E0 object, +468 bit 0x40000000: set while the effect is off.
void SetFxOffFlag(u32 world, u64 enabled) {
    const u32 obj = load<u32>(world + kWorldPostFxFlags);
    if (obj == 0) return;
    const u32 v = load<u32>(obj + 468);
    store<u32>(obj + 468, (v & ~0x40000000u) | (static_cast<u32>(enabled) == 0 ? 0x40000000u : 0));
}

// Enable/disable a post effect and mirror it in the flag above. The world
// pointer is read between the two calls, as in the original.
void Fx_Enable(PPUContext& ctx, u32 fx, u32 on) {
    wcall(ctx, fx, kFxEnable, {fx, on});
    const u32 world = load<u32>(kCurrentWorld);
    SetFxOffFlag(world, wcall(ctx, fx, kFxIsEnabled, {fx}));
}

// t clamped to [0, duration] the way the original's compares do it.
f32 ClampTime(f32 t, f32 duration) {
    const f32 m = t < duration ? t : duration;
    return m > kZero ? m : kZero;
}

// from + ((to - from) * k >> 8), with the original's 32-bit wrap; low byte kept.
u32 LerpByte(u32 from, u32 to, s32 k) {
    const u32 product = (to - from) * static_cast<u32>(k);
    return (from + static_cast<u32>(static_cast<s32>(product) >> 8)) & 0xFF;
}

// fctidz of a single, low 32 bits.
s32 TruncLow(f32 x) { return static_cast<s32>(static_cast<u32>(rt::fctid(static_cast<f64>(x), true))); }

}  // namespace

// bool World_RequestState(World* w, u32 level, u8 id)
// Level 0 while a state is current: queues `id` as the next level-0 state
// (phase 1). Level 1 during phases 1-11: queues it as the next level-1
// state. Otherwise switches at once (keeping +47/+48 in step for level 0).
// +47 always gets the current level-0 id first.
DECOMP(004665C0, World_RequestState) {
    const u32 w = arg<u32>(ctx, 0);
    const u64 level = ctx.gpr[4];
    const u64 id = ctx.gpr[5];
    const u32 current = ref<u8>(load<u32>(w + world::kSmIds));
    ref<u8>(w + kWorldCurState) = static_cast<u8>(current);
    if (current != world::kSmNone && static_cast<u32>(level) == 0 && static_cast<u32>(id) != world::kSmNone) {
        ref<u8>(w + kWorldNextState) = static_cast<u8>(id);
        ref<u8>(w + kWorldNextSub) = 0;
        ref<u8>(w + kWorldPhase) = 1;
        store<u32>(w + kWorldPhaseTime, 0);
        return ret(ctx, 1u);
    }
    if (static_cast<u32>(level) == 1) {
        const u32 phase = ref<u8>(w + kWorldPhase);
        if (phase != 0 && phase < 12) {
            ref<u8>(w + kWorldNextSub) = static_cast<u8>(id);
            return ret(ctx, 1u);
        }
    }
    if (static_cast<u32>(level) == 0) {
        ref<u8>(w + kWorldNextState) = static_cast<u8>(id);
        ref<u8>(w + kWorldCurState) = static_cast<u8>(id);
    }
    ret(ctx, call(ctx, wfn::StateMachine_SetState, {w, level, id & 0xFF}));
}

// void Cutscene_ApplyCamera(Cutscene* c)
// Builds a view matrix from the cutscene camera (rows at +80, +96, +112,
// position at +128) and hands it, then the field of view at +144, to the
// main view of the world at c +52. Rows 0-2 are the negated/transposed
// basis; their w slots get what the original's scratch stores leave there.
// Row 3 is -position times rows 0-2 (vmaddfp chain, fused).
DECOMP(00468760, Cutscene_ApplyCamera) {
    const u32 c = arg<u32>(ctx, 0);
    const auto f = [c](u32 off) { return load<f32>(c + off); };
    Stack frame(ctx, 80);  // the matrix at +0 (r1+176), scratch at +64
    const u32 m = frame.addr();
    const f32 rows[3][4] = {
        {-f(80), f(96), -f(112), f(100)},
        {-f(84), f(100), -f(116), -f(80)},
        {-f(88), f(104), -f(120), -f(88)},
    };
    const f32 nx = kZero - f(128), ny = kZero - f(132), nz = kZero - f(136);  // vsubfp 0 - p
    for (u32 r = 0; r < 3; ++r)
        for (u32 k = 0; k < 4; ++k) store<f32>(m + r * 16 + k * 4, rows[r][k]);
    for (u32 k = 0; k < 4; ++k) {
        f32 acc = std::fma(rows[0][k], nx, 0.0f);
        acc = std::fma(rows[1][k], ny, acc);
        acc = std::fma(rows[2][k], nz, acc);
        store<f32>(m + 48 + k * 4, acc);
    }
    const u32 view = load<u32>(load<u32>(c + 52) + kWorldViews);
    call(ctx, wfn::View_SetMatrix, {view, m});
    callf(ctx, wfn::View_SetFov, {view}, {static_cast<f64>(load<f32>(c + 144))});
}

// bool MsgDialog_Open(const char* msg, s32 kind)
// Aborts the dialog that is up, if any, then opens a cellMsgDialog: kind 1
// OK, 4 OK on the normal background, 2 Yes/No (default No), 3 no buttons
// and no cancel, anything else plain. True (and kDialogState = 3) when it opened.
DECOMP(00468944, MsgDialog_Open) {
    const u32 msg = arg<u32>(ctx, 0);
    const s32 kind = arg<s32>(ctx, 1);
    if (load<u32>(kDialogState) != 0) {
        call(ctx, dlg::cellMsgDialogAbort);
        store<u32>(kDialogState, 0);
    }
    u32 type = dlg::kSeNormal | dlg::kBgInvisible;
    if (kind == 4)
        type = dlg::kSeNormal | dlg::kButtonOk | dlg::kDisableCancel;
    else if (kind == 1)
        type = dlg::kSeNormal | dlg::kBgInvisible | dlg::kButtonOk | dlg::kDisableCancel;
    else if (kind == 2)
        type = dlg::kSeNormal | dlg::kBgInvisible | dlg::kButtonYesNo | dlg::kDisableCancel | dlg::kDefaultCursorNo;
    else if (kind == 3)
        type = dlg::kSeNormal | dlg::kBgInvisible | dlg::kDisableCancel;
    const u32 r = static_cast<u32>(call(ctx, dlg::cellMsgDialogOpen2, {type, msg, kDialogCallback, 0, 0}));
    if (r != 0) return ret(ctx, 0u);
    store<u32>(kDialogState, 3);
    ret(ctx, 1u);
}

// void World_UpdateAudioFilter(World* w, f32 dt)
// Hands the audio time scale (World +0x1282C8 while enabled, else 1) to the
// mixer, then moves the focus emitter's filter cutoff (World +0x128288):
// up to 23999 at normal speed, down to the tuned cutoff while slowed, over
// the tuned time (0 = at once).
DECOMP(00468A38, World_UpdateAudioFilter) {
    const u32 w = arg<u32>(ctx, 0);
    const f64 dt = farg(ctx, 0);
    if (std::fabs(dt) < static_cast<f64>(kEpsilon)) return;
    f64 scale = kOne;
    if (load<u32>(w + kWorldAudioScaleOn) & 0x80000000u) scale = load<f32>(w + kWorldAudioScale);
    callf(ctx, wfn::Audio_SetFilterScale, {}, {scale});
    const u32 handle = load<u32>(kAudioFocus);
    if (static_cast<u32>(call(ctx, wfn::Audio_IsEmitterValid, {handle})) == 0) return;

    const u32 cutoff = w + kWorldFilterCutoff;
    const f32 current = load<f32>(cutoff);
    const u32 low_ea = kTuneFilterLow + kTuning, time_ea = kTuneFilterTime + kTuning;
    f64 value;
    if (std::fabs(static_cast<f64>(static_cast<f32>(scale - kOne))) < static_cast<f64>(kEpsilon)) {
        if (!(current < kFilterOpen)) return;
        const f32 time = load<f32>(time_ea);
        if (std::fabs(static_cast<f64>(time)) < static_cast<f64>(kEpsilon)) {
            store<u32>(cutoff, kFilterOpenBits);
            value = kFilterOpen;
        } else {
            const f32 rate = static_cast<f32>(static_cast<f32>(kFilterOpen - static_cast<f64>(load<f32>(low_ea))) /
                                              static_cast<f64>(time));
            f32 v = static_cast<f32>(std::fma(static_cast<f64>(rate), dt, static_cast<f64>(current)));
            store<f32>(cutoff, v);
            if (v > kFilterOpen) {
                v = kFilterOpen;
                store<u32>(cutoff, kFilterOpenBits);
            }
            value = v;
        }
    } else {
        const f32 low = load<f32>(low_ea);
        if (!(current > low)) return;
        const f32 time = load<f32>(time_ea);
        if (std::fabs(static_cast<f64>(time)) < static_cast<f64>(kEpsilon)) {
            // The original stores the (near-zero) time here, not the cutoff.
            store<u32>(cutoff, load<u32>(time_ea));
            value = time;
        } else {
            const f32 rate = static_cast<f32>(static_cast<f32>(static_cast<f64>(low) - kFilterOpen) /
                                              static_cast<f64>(time));
            f32 v = static_cast<f32>(std::fma(static_cast<f64>(rate), dt, static_cast<f64>(current)));
            store<f32>(cutoff, v);
            if (v < low) {
                v = low;
                store<u32>(cutoff, load<u32>(low_ea));
            }
            value = v;
        }
    }
    callf(ctx, wfn::Audio_SetEmitterFilter, {handle}, {value});
}

// void Fade_Start(Fade* f, const u8 from[4], const u8 to[4], f32 time, bool hide_when_done, bool force)
// Starts a colour fade on "Common.Fade" and shows it. Unless `force`, a
// fade that changes nothing (the element already shows `to` and is visible
// iff `to` has alpha) takes no time. The element starts at `from` if the
// fade has time left, else at `to`.
DECOMP(00468C40, Fade_Start) {
    const u32 f = arg<u32>(ctx, 0), from = arg<u32>(ctx, 1), to = arg<u32>(ctx, 2);
    const f64 time = farg(ctx, 0);
    const u32 hide = static_cast<u32>(ctx.gpr[7]), force = static_cast<u32>(ctx.gpr[8]);
    for (u32 k = 0; k < 4; ++k) ref<u8>(f + kFadeFrom + k) = ref<u8>(from + k);
    for (u32 k = 0; k < 4; ++k) ref<u8>(f + kFadeTo + k) = ref<u8>(to + k);
    u32 flags = load<u32>(f + kFadeFlags);
    store<f32>(f + kFadeDuration, static_cast<f32>(time));
    flags = (flags & 0x7FFFFFFFu) | ((hide & 1) << 31);
    store<u32>(f + kFadeTime, 0);
    store<u32>(f + kFadeFlags, flags | kFadeRunning);
    const u32 ui = load<u32>(kFadeUi);
    if (ui == 0) {
        const u32 stopped = (flags | kFadeRunning) & ~kFadeRunning;
        store<u32>(f + kFadeFlags, stopped);
        return ret(ctx, stopped);
    }
    call(ctx, wfn::Ui_BringToFront, {ui, 1});
    const f32 alpha = static_cast<f32>(static_cast<f64>(static_cast<f32>(ref<u8>(f + kFadeTo + 3))) *
                                       static_cast<f64>(kByteToUnit));
    const bool opaque = alpha > kZero;
    if ((force & 0xFF) == 0) {
        const bool skip_a = static_cast<u32>(wcall(ctx, ui, kUiIsVisible, {ui})) == 0 && opaque;
        if (!skip_a) {
            const bool skip_b = static_cast<u32>(wcall(ctx, ui, kUiIsVisible, {ui})) != 0 && !opaque;
            if (!skip_b) {
                Stack frame(ctx, 16);
                wcall(ctx, ui, kUiGetColor, {frame.addr(), ui});
                if (load<u32>(frame.addr()) == load<u32>(f + kFadeTo)) store<u32>(f + kFadeDuration, 0);
            }
        }
    }
    const u32 src = load<f32>(f + kFadeDuration) > kZero ? f + kFadeFrom : f + kFadeTo;
    Ui_SetColor(ctx, ui, rgba_at(src), ref<u8>(src + 2));
    wcall(ctx, ui, kUiSetVisible, {ui, 1});
}

// void PostFx_UpdateFilter(FilterFx* s, f32 dt)
// Blends the camera's filter effect (camera +16) from +0 to +4 over +8
// seconds (+16 elapsed); +15 is the mode (1-5). Flags +20: 0x40000000
// running, 0x80000000 switch the effect off when done.
DECOMP(00468EF8, PostFx_UpdateFilter) {
    const u32 s = arg<u32>(ctx, 0);
    const f64 dt = farg(ctx, 0);
    if (!(load<u32>(s + 20) & 0x40000000u)) return;
    const u32 view = MainView();
    if (view == 0) return;
    const u32 fx = CameraFx(view, 16);
    if (fx == 0) return;
    Fx_Enable(ctx, fx, 1);
    if (static_cast<u32>(wcall(ctx, fx, kFxIsEnabled, {fx})) == 0) {
        store<u32>(s + 20, load<u32>(s + 20) & ~0x40000000u);
        return;
    }
    const u32 mode = ref<u8>(s + 15);
    ref<u8>(fx + 20) = static_cast<u8>(mode < 1 ? 1 : (mode > 5 ? 5 : mode));
    const f32 t = static_cast<f32>(static_cast<f64>(load<f32>(s + 16)) + dt);
    const f32 duration = load<f32>(s + 8);
    if (t > duration || t == duration) store<u32>(s + 20, load<u32>(s + 20) & ~0x40000000u);
    const f32 clamped = ClampTime(t, duration);
    const f32 ratio = static_cast<f32>(static_cast<f64>(clamped) / static_cast<f64>(duration));
    const f32 a = load<f32>(s + 0), b = load<f32>(s + 4);
    store<f32>(s + 16, clamped);
    f32 v = static_cast<f32>(std::fma(static_cast<f64>(ratio), static_cast<f64>(static_cast<f32>(b - a)),
                                      static_cast<f64>(a)));
    if (v < kZero)
        v = kZero;
    else if (v > kOne)
        v = kOne;
    store<f32>(fx + 24, v);
    const u32 flags = load<u32>(s + 20);
    if (flags & 0x40000000u) return;
    if (!(flags & 0x80000000u)) return;
    Fx_Enable(ctx, fx, 0);
}

// void PostFx_UpdateTint(TintStack* s, f32 dt)
// A stack of up to four 28-byte tints (count at +112) on the camera's tint
// effect (camera +24). Each entry: u32 +0 (bits 28-31 and 24-27 = modes,
// 0x00800000 = pop when done), u8 rgb from +2, to +6, f32 amount from +12,
// to +16, duration +20, elapsed +24. Every entry's time advances; the top
// one is shown; an empty stack switches the effect off.
DECOMP(004691E8, PostFx_UpdateTint) {
    const u32 s = arg<u32>(ctx, 0);
    const f64 dt = farg(ctx, 0);
    const u32 view = MainView();
    if (view == 0) return store<u32>(s + 112, 0);
    const u32 fx = CameraFx(view, 24);
    if (fx == 0) return store<u32>(s + 112, 0);

    for (s32 i = static_cast<s32>(load<u32>(s + 112) - 1); i >= 0; --i) {
        const u32 e = s + static_cast<u32>(i) * 28;
        const f32 t = static_cast<f32>(static_cast<f64>(load<f32>(e + 24)) + dt);
        const f32 duration = load<f32>(e + 20);
        if (!(t > duration || t == duration)) {
            store<f32>(e + 24, t);
            continue;
        }
        if (!(load<u32>(e) & 0x00800000u)) {
            store<u32>(e + 24, load<u32>(e + 20));  // stop at the end
            continue;
        }
        // Pop it: move the entries above it down one (the count is re-read
        // every step, as in the original).
        u32 count = load<u32>(s + 112);
        u32 j = static_cast<u32>(i);
        while (j < count - 1) {
            const u32 dst = s + j * 28;
            for (u32 k = 0; k < 28; k += 4) store<u32>(dst + k, load<u32>(dst + 28 + k));
            ++j;
            count = load<u32>(s + 112);
        }
        store<u32>(s + 112, count - 1);
    }

    const u32 count = load<u32>(s + 112);
    if (count == 0) {
        wcall(ctx, fx, kFxEnable, {fx, 0});
        const u32 obj = load<u32>(load<u32>(kCurrentWorld) + kWorldPostFxFlags);
        if (obj != 0) store<u32>(obj + 468, load<u32>(obj + 468) | 0x40000000u);
        return;
    }
    const u32 e = s + (count - 1) * 28;
    f32 ratio = kOne;
    const f32 duration = load<f32>(e + 20);
    if (duration > kZero) ratio = static_cast<f32>(static_cast<f64>(load<f32>(e + 24)) / static_cast<f64>(duration));
    const u32 obj = load<u32>(load<u32>(kCurrentWorld) + kWorldPostFxFlags);
    if (obj != 0) store<u32>(obj + 468, load<u32>(obj + 468) & ~0x40000000u);
    wcall(ctx, fx, kFxEnable, {fx, 1});
    store<u32>(fx + 52, load<u32>(e) >> 28);
    store<u32>(fx + 48, (load<u32>(e) >> 24) & 0xF);
    const s32 k = TruncLow(static_cast<f32>(static_cast<f64>(ratio) * kByteMax));
    const f32 a = load<f32>(e + 12), b = load<f32>(e + 16);
    f32 amount = static_cast<f32>(std::fma(static_cast<f64>(ratio), static_cast<f64>(static_cast<f32>(b - a)),
                                           static_cast<f64>(a)));
    const u32 r = LerpByte(ref<u8>(e + 2), ref<u8>(e + 6), k);
    const u32 g = LerpByte(ref<u8>(e + 3), ref<u8>(e + 7), k);
    const u32 bl = LerpByte(ref<u8>(e + 4), ref<u8>(e + 8), k);
    if (amount < kZero)
        amount = kZero;
    else if (amount > kOne)
        amount = kOne;
    store<f32>(fx + 20, amount);
    store<f32>(fx + 32, static_cast<f32>(r));
    store<f32>(fx + 36, static_cast<f32>(g));
    store<f32>(fx + 40, static_cast<f32>(bl));
}

// void PostFx_UpdateSwap(SwapFx* s, f32 dt)
// A three-step sequence (+8 = step 0 or 1 indexes the s32 actions at +0):
// action 0 switches the camera's effect +8 off (remembering whether it was
// on, +9) and +40 on with value +12; action 1 blends +40's value (+164) from
// +12 to +16 over +20 seconds (+24 elapsed); action 2 switches +40 off and
// +8 back as it was. Steps 0 and 2 advance +8 at once, step 1 when done.
DECOMP(004695A8, PostFx_UpdateSwap) {
    const u32 s = arg<u32>(ctx, 0);
    const f64 dt = farg(ctx, 0);
    const u8 step_byte = ref<u8>(s + 8);
    const s32 step = static_cast<s8>(step_byte);
    if (step < 0 || step >= 2) return;
    const u32 view = MainView();
    if (view == 0) return;
    const u32 cam = load<u32>(view + kViewCamera);
    const s32 action = load<s32>(s + static_cast<u32>(step) * 4);
    const u32 a = load<u32>(cam + 8), b = load<u32>(cam + 40);
    if (action == 0) {
        ref<u8>(s + 9) = static_cast<u8>(wcall(ctx, a, kFxIsEnabled, {a}));
        wcall(ctx, a, kFxEnable, {a, 0});
        wcall(ctx, b, kFxEnable, {b, 1});
        store<u32>(b + 164, load<u32>(s + 12));
        ref<u8>(s + 8) = static_cast<u8>(ref<u8>(s + 8) + 1);
    } else if (action == 2) {
        wcall(ctx, b, kFxEnable, {b, 0});
        // The original has the entry in r5 and the OPD in r6 for this one.
        const u32 opd = load<u32>(load<u32>(a) + kFxEnable);
        ctx.gpr[5] = load<u32>(opd);
        ctx.gpr[6] = opd;
        call_ptr(ctx, opd, {a, ref<u8>(s + 9)});
        ref<u8>(s + 8) = static_cast<u8>(ref<u8>(s + 8) + 1);
    } else if (action == 1) {
        const f32 t = static_cast<f32>(static_cast<f64>(load<f32>(s + 24)) + dt);
        const f32 duration = load<f32>(s + 20);
        if (t > duration || t == duration) ref<u8>(s + 8) = static_cast<u8>(step_byte + 1);
        const f32 clamped = ClampTime(t, duration);
        const f32 ratio = static_cast<f32>(static_cast<f64>(clamped) / static_cast<f64>(duration));
        const f32 from = load<f32>(s + 12), to = load<f32>(s + 16);
        store<f32>(s + 24, clamped);
        store<f32>(b + 164, static_cast<f32>(std::fma(static_cast<f64>(static_cast<f32>(to - from)),
                                                      static_cast<f64>(ratio), static_cast<f64>(from))));
    }
}

// void PostFx_UpdateDof(DofFx* s, f32 dt)
// Depth of field on the camera's effect +12. Flags +40: 0x20000000 running,
// 0x40000000 -> effect +20, 0x10000000 keep running when done, 0x80000000
// switch off when done. Blends over +28 seconds (+36 elapsed): +4..+8 ->
// effect +32 (scaled by tuning), and the focus range from +12..+16 /
// +20..+24, or the distance from the main view to the object at +0 (its
// position at +144). Mode +35 (1-4) -> effect +64.
DECOMP(004697D8, PostFx_UpdateDof) {
    const u32 s = arg<u32>(ctx, 0);
    const f64 dt = farg(ctx, 0);
    if (!(load<u32>(s + 40) & 0x20000000u)) return;
    const u32 view = MainView();
    if (view == 0) return;
    const u32 fx = CameraFx(view, 12);
    if (fx == 0) return;
    Fx_Enable(ctx, fx, 1);
    if (static_cast<u32>(wcall(ctx, fx, kFxIsEnabled, {fx})) == 0) {
        store<u32>(s + 40, load<u32>(s + 40) & ~0x20000000u);
        return;
    }
    ref<u8>(fx + 20) = (load<u32>(s + 40) & 0x40000000u) ? 1 : 0;
    const u32 mode = ref<u8>(s + 35);
    ref<u8>(fx + 64) = static_cast<u8>(mode < 1 ? 1 : (mode > 4 ? 4 : mode));
    const f32 t = static_cast<f32>(static_cast<f64>(load<f32>(s + 36)) + dt);
    const f32 duration = load<f32>(s + 28);
    if (t > duration || t == duration) {
        const u32 flags = load<u32>(s + 40);
        if (!(flags & 0x10000000u)) store<u32>(s + 40, flags & ~0x20000000u);
    }
    const f32 clamped = ClampTime(t, duration);
    store<f32>(s + 36, clamped);
    const f32 ratio = static_cast<f32>(static_cast<f64>(clamped) / static_cast<f64>(duration));
    const auto lerp = [ratio](f32 a, f32 b) {
        return static_cast<f32>(std::fma(static_cast<f64>(ratio), static_cast<f64>(static_cast<f32>(b - a)),
                                         static_cast<f64>(a)));
    };
    f32 focus, range;
    const u32 target = load<u32>(s + 0);
    if (target == 0) {
        focus = lerp(load<f32>(s + 12), load<f32>(s + 16));
        range = lerp(load<f32>(s + 20), load<f32>(s + 24));
    } else {
        const u32 p = target + 144, v = MainView() + kViewPosition;
        const f32 dy = load<f32>(p + 4) - load<f32>(v + 4);
        const f32 dx = load<f32>(p + 0) - load<f32>(v + 0);
        const f32 dz = load<f32>(p + 8) - load<f32>(v + 8);
        f32 d = static_cast<f32>(static_cast<f64>(dy) * dy);
        d = static_cast<f32>(std::fma(static_cast<f64>(dx), static_cast<f64>(dx), static_cast<f64>(d)));
        d = static_cast<f32>(std::fma(static_cast<f64>(dz), static_cast<f64>(dz), static_cast<f64>(d)));
        d = static_cast<f32>(std::sqrt(static_cast<f64>(d)));
        focus = range = d;
    }
    store<f32>(fx + 24, focus);
    const f32 value = lerp(load<f32>(s + 4), load<f32>(s + 8));
    f32 far;
    if (range < kDofMinFar)
        far = kDofMinFar;
    else
        far = range > kDofMaxFar ? kDofMaxFar : range;
    store<f32>(fx + 28, far);
    store<f32>(fx + 32, static_cast<f32>(static_cast<f64>(value) * load<f32>(kTuneDofScale + kTuning)));
    const u32 flags = load<u32>(s + 40);
    if (flags & 0x20000000u) return;
    if (!(flags & 0x80000000u)) return;
    Fx_Enable(ctx, fx, 0);
}

// void Fade_Update(Fade* f, f32 dt)
// Advances a running fade (see Fade_Start) and applies its colour to
// "Common.Fade"; once it has run its time the element gets `to` and is
// hidden if the fade asked for it.
DECOMP(00469B90, Fade_Update) {
    const u32 f = arg<u32>(ctx, 0);
    const f64 dt = farg(ctx, 0);
    u32 flags = load<u32>(f + kFadeFlags);
    u32 from[4];
    for (u32 k = 0; k < 4; ++k) from[k] = ref<u8>(f + kFadeFrom + k);
    if (!(flags & kFadeRunning)) return;
    const u32 ui = load<u32>(kFadeUi);
    if (ui == 0) return store<u32>(f + kFadeFlags, flags & ~kFadeRunning);

    const f32 time = load<f32>(f + kFadeTime), duration = load<f32>(f + kFadeDuration);
    u32 c[4];
    if (time > duration || time == duration) {
        for (u32 k = 0; k < 4; ++k) c[k] = ref<u8>(f + kFadeTo + k);
        flags &= ~kFadeRunning;
        store<u32>(f + kFadeFlags, flags);
    } else {
        const f32 t = static_cast<f32>(static_cast<f64>(time) + dt);
        const f32 ratio = static_cast<f32>(static_cast<f64>(t) / static_cast<f64>(duration));
        store<f32>(f + kFadeTime, t);
        for (u32 k = 0; k < 4; ++k) c[k] = ref<u8>(f + kFadeTo + k);
        const u32 k8 = static_cast<u32>(TruncLow(static_cast<f32>(static_cast<f64>(ratio) * kByteMax)));
        if (k8 < 255)
            for (u32 k = 0; k < 4; ++k) c[k] = LerpByte(from[k], c[k], static_cast<s32>(k8));
    }
    const u32 hidden = (flags & kFadeRunning) ? 0 : flags >> 31;
    call(ctx, wfn::Ui_BringToFront, {ui, 1});
    Ui_SetColor(ctx, ui, (c[0] << 24) | (c[1] << 16) | (c[2] << 8) | c[3], from[2]);
    wcall(ctx, ui, kUiSetVisible, {ui, hidden == 0 ? 1u : 0u});
}
