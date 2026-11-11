// Game code, large-function sweep part 9: a game-mode object's phase
// switch (00053A80).
#include "big9.h"

using namespace sly4;
using namespace sly4::big9;
using namespace sly4::big9::b9fn;

namespace {
constexpr u32 kSetPhaseFx = 0x000533A0u;    // (o): (name?)
constexpr u32 kItemFind = 0x004A22D8u;      // (u32 key, u32 set) -> item object (name?)
constexpr u32 kTargetCtor = 0x00115B90u;    // (CamTarget* t, Camera*, player or 0, u32; f1 f32) (name?)
constexpr u32 kCamPushTarget = 0x000FC040u; // Camera_PushTarget0FC040 (Camera*, Target*, -, u8 priority; f32 blend) -> u32
constexpr u32 kStrLen = 0x00555FA4u;        // String_Length (const char*) -> s32
constexpr u32 kSetHashObj = 0x00369D14u;    // Global_SetHashObj_00369D14 (Obj*, const char*)
constexpr u32 kObjNotify24 = 0x000D949Cu;   // Obj_Notify24 (o) -> coach or 0
constexpr u32 kCoachReset = 0x0029F26Cu;    // Coach_ResetMode29F26C (coach)
constexpr u32 kSetFlag20 = 0x0029F374u;     // Obj_SetFlag20_0029F374 (o, u8 on)
constexpr u32 kSetEffectHash = 0x000DAEC8u; // Obj_SetEffectHash_000DAEC8 (o, u32 hash)

constexpr u32 kZeroF = 0x00053A74u;         // .text: f32 0 (the lower clamp, the camera blend)
constexpr u32 kItemArg = 0x00053A78u;       // .text: f32 100 (the items' vfunc +64 argument)
constexpr u32 kCapDefault = 0x00F990A4u;    // f32 caps for the +60 object's +348 (tuning, name?)
constexpr u32 kCapA = 0x00F990F0u;          // modes 3 / 4
constexpr u32 kCapB = 0x00F9913Cu;          // modes 5..7
constexpr u32 kCapC = 0x00F99188u;          // modes 8 / 9
constexpr u32 kPattern = 0x00F9B410u;       // char[]: the mode 9 sequence (W / M / H / B letters)
constexpr u32 kMsDecibel = 0x00AC031Cu;     // "MsDecibel"
constexpr u32 kWorldPlayer = 4216;          // World: the player (+192 flags: 2 present, 8 coach link) or 0
constexpr u32 kWorldCamera = 0x001281E0u;   // World: camera

// Object fields.
constexpr u32 kKind = 4;
constexpr u32 kTargetArg = 48;              // u32 passed to the camera target constructor
constexpr u32 kMeter = 60;                  // object: +344 f32 level, +348 f32 clamped level
constexpr u32 kTargetBlend = 180;           // f32
constexpr u32 kPhase = 308;
constexpr u32 kPatternPos = 312;
constexpr u32 kSubState = 316;
constexpr u32 kFx1744 = 1744;               // effect objects; +1752 the current one
constexpr u32 kFx1748 = 1748;
constexpr u32 kFxCur = 1752;
constexpr u32 kItemKeys = 5084;             // u32[5]
constexpr u32 kItems = 5104;                // item object[5]
constexpr u32 kItemSet = 5132;
constexpr u32 kCamTarget = 6216;            // CamTarget* or 0
constexpr u32 kVtSetState = 220;            // (o, u32 state)

void SetState(PPUContext& ctx, u32 o, u32 s) { VCall(ctx, o, kVtSetState, {o, s}); }

// Modes 3 / 8: the effect object at +fx becomes current (vfunc +88 (e, 1,
// 1, 1, 1), effect hash), 0x000533A0(o), the five items looked up again
// (each gets vfunc +64 with f1 100), state 3.
void Burst(PPUContext& ctx, u32 o, u32 fx, u32 hash) {
    const u32 e = load<u32>(o + fx);
    store<u32>(o + kFxCur, e);
    VCall(ctx, e, 88, {e, 1, 1, 1, 1});
    call(ctx, kSetEffectHash, {load<u32>(o + kFxCur), hash});
    call(ctx, kSetPhaseFx, {o});
    const u32 arg = load<u32>(kItemArg);
    for (u32 k = 0; k < 5; ++k) {
        const u32 it = static_cast<u32>(call(ctx, kItemFind, {load<u32>(o + kItemKeys + 4 * k), load<u32>(o + kItemSet)}));
        store<u32>(o + kItems + 4 * k, it);
        ctx.fpr[1] = std::bit_cast<f64>(LfsBits(arg));
        VCall(ctx, it, 64, {it});
    }
    SetState(ctx, o, 3);
}
}  // namespace

// u32 Game_SetPhase(GameObj* o, s32 phase)
// +308 = phase, +312 = 0, then per phase: 0 / 1: state 1 / 2; 2 / 4: +316
// = 0 / 1, state 9; 3 / 8: Burst with the effect at +1744 / +1748 (hash
// 0x8839CACC / 0x8839CACD); 5: state 1; 6: +324 / +328 cleared, state 2;
// 7: +316 = 2, state 9; 9: +316 = 3, the next letter of the sequence at
// 0x00F9B410 picks the state (W 16, M 13, H 7, B 11, else / past its end
// 12), and a new 224-byte camera target (0 when the allocation fails) is
// pushed on the world camera and kept at +6216. Each phase picks a cap
// (0x00F990A4 by default); the +60 object's +348 = max(min(+344, cap), 0).
// When the kind +4 is set, Global_SetHashObj(+60, "MsDecibel"). Finally a
// present player (+192 0x2) linked to a coach (0x8) has its coach reset
// with flag 0x20 set. (r3 as the original leaves it.)
DECOMP(00053A80, Game_SetPhase) {
    const u32 o = arg<u32>(ctx, 0);
    const s32 phase = static_cast<s32>(arg<u32>(ctx, 1));
    store<u32>(o + kPhase, static_cast<u32>(phase));
    store<u32>(o + kPatternPos, 0);
    u32 cap = load<u32>(kCapDefault);
    if (phase > 4) {
        if (phase == 5) {
            SetState(ctx, o, 1);
            cap = load<u32>(kCapB);
        } else if (phase == 6) {
            store<u32>(o + 324, 0);
            store<u32>(o + 328, 0);
            SetState(ctx, o, 2);
            cap = load<u32>(kCapB);
        } else if (phase == 7) {
            store<u32>(o + kSubState, 2);
            cap = load<u32>(kCapB);
            SetState(ctx, o, 9);
        } else if (phase == 8) {
            Burst(ctx, o, kFx1748, 0x8839CACDu);
            cap = load<u32>(kCapC);
        } else if (phase == 9) {
            store<u32>(o + kSubState, 3);
            const s32 n = static_cast<s32>(call(ctx, kStrLen, {kPattern}));
            const s32 pos = static_cast<s32>(load<u32>(o + kPatternPos));
            u32 st = 12;
            if (pos < n) {
                const s8 ch = static_cast<s8>(load<u8>(kPattern + static_cast<u32>(pos)));
                if (ch == 'W') st = 16;
                else if (ch == 'M') st = 13;
                else if (ch == 'H') st = 7;
                else if (ch == 'B') st = 11;
            }
            store<u32>(o + kPatternPos, static_cast<u32>(pos) + 1u);
            SetState(ctx, o, st);
            const u32 world = load<u32>(kWorldPtr);
            cap = load<u32>(kCapC);
            u32 player = 0;
            const u32 pl = load<u32>(world + kWorldPlayer);
            if (pl != 0 && (load<u32>(pl + 192) & 2u) != 0) player = pl;
            const u32 cam = load<u32>(world + kWorldCamera);
            const u64 t = call(ctx, kSmallAlloc, {224});
            u64 tgt = 0;
            if (static_cast<u32>(t) != 0) {
                callf(ctx, kTargetCtor, {t, cam, player, load<u32>(o + kTargetArg)}, {std::bit_cast<f64>(LfsBits(load<u32>(o + kTargetBlend)))});
                tgt = t;
            }
            store<u32>(o + kCamTarget, static_cast<u32>(tgt));
            callf(ctx, kCamPushTarget, {cam, tgt, tgt, 0}, {std::bit_cast<f64>(LfsBits(load<u32>(kZeroF)))});
        }
    } else if (phase > 1) {
        if (phase == 2) {
            store<u32>(o + kSubState, 0);
            SetState(ctx, o, 9);
        } else if (phase == 3) {
            Burst(ctx, o, kFx1744, 0x8839CACCu);
            cap = load<u32>(kCapA);
        } else {
            store<u32>(o + kSubState, 1);
            cap = load<u32>(kCapA);
            SetState(ctx, o, 9);
        }
    } else if (phase == 0) {
        SetState(ctx, o, 1);
    } else if (phase == 1) {
        SetState(ctx, o, 2);
    }

    const u32 m = load<u32>(o + kMeter);
    const u32 lv = load<u32>(m + 344);
    const u32 lo = load<u32>(kZeroF);
    const u32 mn = std::bit_cast<f32>(cap) < std::bit_cast<f32>(lv) ? cap : lv;
    const u32 v = std::bit_cast<f32>(mn) > std::bit_cast<f32>(lo) ? mn : lo;
    store<u32>(m + 348, v);  // stfs of an lfs value: raw word
    SetF1Bits(ctx, v);
    if (load<u32>(o + kKind) != 0) call(ctx, kSetHashObj, {m, kMsDecibel});
    const u32 world = load<u32>(kWorldPtr);
    const u32 pl = load<u32>(world + kWorldPlayer);
    if (pl == 0) {
        ret(ctx, static_cast<u64>(world) + kWorldPlayer - 8);  // r3 = &world->+4208, as left
        return;
    }
    if ((load<u32>(pl + 192) & 2u) == 0 || (load<u32>(pl + 192) & 8u) == 0) {
        ret(ctx, 0u);
        return;
    }
    const u64 coach = call(ctx, kObjNotify24, {pl});
    if (static_cast<u32>(coach) == 0) return;  // r3 = its result
    call(ctx, kCoachReset, {coach});
    call(ctx, kSetFlag20, {coach, 1});
}
