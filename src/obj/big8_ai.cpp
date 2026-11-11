// Game code, large-function sweep part 8: an AI mover's state choice from
// targets in reach, its move direction and the blocked-direction flags
// (00160000).
#include "big8.h"

using namespace sly4;
using namespace sly4::big8;
using namespace sly4::big8::b8fn;

namespace {
constexpr u32 kK = 0x0015FFE8u;          // .text: f64 0, f64 0, f32 2.0, f32 10.0 (the first word also read as f32 0)
constexpr u32 kTuning = 0x0105AD00u;     // tuning block: +72 f32 move threshold
constexpr u32 kTuning2 = 0x01041900u;    // tuning block: +72 f32
constexpr u32 kFindComponent = 0x000D1288u;  // Obj_FindComponent (o, kind, 0)
constexpr u32 kObjOwner = 0x000D93FCu;       // (o) -> owner component
constexpr u32 kComp18 = 0x000D935Cu;         // (o) -> component 18
constexpr u32 kTargetGetDef = 0x000D951Cu;   // (target) -> def
constexpr u32 kTargetNext = 0x00271C08u;     // (list, kind, prev) -> next target
constexpr u32 kTargetInReach = 0x002B79F8u;  // (def, const Mat34*, viewer) -> bool
constexpr u32 kSetRef380 = 0x002CC7F8u;      // (mover, target)
constexpr u32 kMoverAim = 0x002CDA40u;       // (mover, const f32 v[2]) -> u8 (name?)
constexpr u32 kVec2Direction = 0x002CDC88u;  // (mover, const f32 v[2]) -> s32 direction
constexpr u32 kMoverTry = 0x002CDD68u;       // (mover, u32 kind, -, -, 0, 0; f1, f2) -> bool (name?)
constexpr u32 kMoverCheck = 0x002CEB80u;     // (mover; f1) -> bool (name?)
constexpr u32 kNotify = 0x004902B8u;         // Call_HandleNotify_F64D08 ()
constexpr u32 kMat34FromRotPos = 0x00553B4Cu;  // (out, const Mat33*, const vec3*)

constexpr u32 kVfnActive = 64;     // Obj (o) -> viewer / non-zero when active
constexpr u32 kVfnSetState = 28;   // Obj (o, u32 layer, u32 state)
constexpr u32 kVfnGetState = 32;   // Obj (o, u32 layer) -> state
constexpr u32 kVfnTargets = 376;   // Obj (o) -> target list owner
constexpr u32 kVfnPos = 284;       // Obj (out, o): position

inline u32 State(PPUContext& ctx, u32 o, u32 layer) { return static_cast<u32>(VCall(ctx, o, kVfnGetState, {o, layer})); }
inline void SetState(PPUContext& ctx, u32 o, u32 layer, u32 s) { VCall(ctx, o, kVfnSetState, {o, layer, s}); }
// fsel a >= 0 ? b : c, then frsp.
inline f32 Fsel(f64 a, f32 b, f64 c) { return a >= 0.0 ? b : static_cast<f32>(c); }
}  // namespace

// bool Ai_ChooseMoveState(Obj* o, const u8 open[4])
// For an active object with its mover component (44) and an input source
// (+380 vfunc +12): when hunting (+700 0x80 without 0x40, input +36
// 0x20000, a target list, layer-1 state not 8), the first kind-3 target in
// reach of its frame (rows +96..+136, position vfunc +284) is taken
// (SetRef380) -> state 8. Else by the input direction (+0 / +8): with an
// owner and +671 0x80 without 0x40 and mover +348 > 0 -> 6; input +36 0x80
// and open[2] -> 5 (unless already); above the move threshold: in mover
// mode (+345) direction 4 with +374 -> 10, direction 0 with neither +372
// nor open[0] -> 9; else directions 2 / 6 closed both ways try a
// mover action (-> 3 with layer 2 = 3 / 0); then the aim, clamped to one
// side where open[k] is 0 or the mover has +372+k, decides 1 (short) or 2 (with
// layer 2 = the aim's result). Below the threshold (or in state 2): some
// layer-2 states (11..13, 19..21, 27..29, 35..37) or state 5 with mover
// mode and a mover check -> 10. Returns whether a state was set.
DECOMP(00160000, Ai_ChooseMoveState) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 open = arg<u32>(ctx, 1);
    if (static_cast<u32>(VCall(ctx, o, kVfnActive, {o})) == 0) {
        ret(ctx, 0u);
        return;
    }
    const u32 c = static_cast<u32>(call(ctx, kFindComponent, {o, 44, 0}));
    if (c == 0) {
        ret(ctx, 0u);
        return;
    }
    const u32 src = load<u32>(o + 380);
    const u32 in = static_cast<u32>(VCall(ctx, src, 12, {src}));
    if (in == 0) {
        ret(ctx, 0u);
        return;
    }
    const u32 owner = static_cast<u32>(call(ctx, kObjOwner, {o}));
    const u32 list = static_cast<u32>(VCall(ctx, o, kVfnTargets, {o}));
    Stack frame(ctx, 144);  // the original's sp+112..+255
    const u32 sp = frame.addr() - 112;
    const u32 b700 = load<u8>(o + 700);
    if ((b700 & 0x80u) != 0 && (b700 & 0x40u) == 0 && (load<u32>(in + 36) & 0x20000u) != 0 && list != 0 && State(ctx, o, 1) != 8) {
        for (u32 r = 0; r < 3; ++r)
            for (u32 k = 0; k < 3; ++k) CopyW(sp + 128 + 16 * r + 4 * k, o + 96 + 16 * r + 4 * k);
        SetF1Bits(ctx, load<u32>(o + 136));
        VCall(ctx, o, kVfnPos, {sp + 112, o});  // OPD in r5, entry in r6
        call(ctx, kMat34FromRotPos, {sp + 176, sp + 128, sp + 112});
        for (u32 tg = static_cast<u32>(call(ctx, kTargetNext, {list, 3, 0})); tg != 0; tg = static_cast<u32>(call(ctx, kTargetNext, {list, 3, tg}))) {
            if (static_cast<u32>(call(ctx, kComp18, {tg})) != 0) {
                const u32 x = static_cast<u32>(call(ctx, kComp18, {tg}));
                if (load<u32>(x + 48) < static_cast<u32>(load<u16>(x + 36)) && load<u8>(static_cast<u32>(call(ctx, kComp18, {tg})) + 16) != 0) continue;
            }
            const u32 def = static_cast<u32>(call(ctx, kTargetGetDef, {tg}));
            const u32 viewer = static_cast<u32>(VCall(ctx, o, kVfnActive, {o}));
            if (static_cast<u32>(call(ctx, kTargetInReach, {def, sp + 176, viewer})) == 0) continue;
            if ((load<u64>(o + 184) & 1u) != 0) call(ctx, kNotify, {1});  // r3 holds the tested bit
            call(ctx, kSetRef380, {c, tg});
            SetState(ctx, o, 1, 8);
            ret(ctx, 1u);
            return;
        }
    }
    SetF1Bits(ctx, load<u32>(in));
    CopyW(sp + 240, in);
    CopyW(sp + 244, in + 8);
    const s32 dir = static_cast<s32>(static_cast<u32>(call(ctx, kVec2Direction, {c, sp + 240})));
    if (owner != 0) {
        const u32 b671 = load<u8>(o + 671);
        if ((b671 & 0x80u) != 0 && (b671 & 0x40u) == 0) {
            SetF1Bits(ctx, load<u32>(c + 348));
            if (F(c + 348) > F(kK)) {
                SetState(ctx, o, 1, 6);
                ret(ctx, 1u);
                return;
            }
        }
    }
    if ((load<u32>(in + 36) & 0x80u) != 0 && load<u8>(open + 2) != 0 && State(ctx, o, 1) != 5) {
        SetState(ctx, o, 1, 5);
        ret(ctx, 1u);
        return;
    }
    SetF1Bits(ctx, load<u32>(in + 32));
    bool slow = !(F(in + 32) > F(kTuning + 72));
    if (!slow) {
        auto mover = [&](u32 k) { return load<u8>(c + 372 + k) != 0; };
        auto opn = [&](u32 k) { return load<u8>(open + k) != 0; };
        bool aim = false;
        if (load<u8>(c + 345) != 0) {
            if (dir == 4 && mover(2)) {
                SetState(ctx, o, 1, 10);
                ret(ctx, 1u);
                return;
            }
            if (dir == 0 && !mover(0) && !opn(0)) {
                SetState(ctx, o, 1, 9);
                ret(ctx, 1u);
                return;
            }
            aim = true;
        }
        if (!aim && dir == 2 && !opn(1) && !mover(1)) {
            callf(ctx, kMoverTry, {c, 1, kK, ctx.gpr[6], 0, 0}, {F(kK + 16), F(kK + 20)});  // r5 / r6 as the original leaves them (not read)
            if (static_cast<u32>(ctx.gpr[3]) != 0) {
                SetState(ctx, o, 1, 3);
                SetState(ctx, o, 2, 3);
                ret(ctx, 1u);
                return;
            }
        }
        if (!aim && dir == 6 && !opn(3) && !mover(3)) {
            callf(ctx, kMoverTry, {c, 3, kK, ctx.gpr[6], 0, 0}, {F(kK + 16), F(kK + 20)});
            if (static_cast<u32>(ctx.gpr[3]) != 0) {
                SetState(ctx, o, 1, 3);
                SetState(ctx, o, 2, 0);
                ret(ctx, 1u);
                return;
            }
        }
        call(ctx, kMoverAim, {c, sp + 240});
        if (State(ctx, o, 1) == 2) {
            slow = true;
        } else {
            f32 x = F(sp + 240), y = F(sp + 244);
            CopyW(sp + 248, sp + 240);
            CopyW(sp + 252, sp + 244);
            SetF1Bits(ctx, load<u32>(sp + 244));
            if (!opn(0) || mover(0)) {
                y = Fsel(y, y, load<f64>(kK));
                store<f32>(sp + 252, y);
            }
            if (!opn(1) || mover(1)) {
                x = Fsel(-static_cast<f64>(x), x, load<f64>(kK + 8));
                store<f32>(sp + 248, x);
            }
            if (!opn(2) || mover(2)) {
                y = Fsel(-static_cast<f64>(y), y, load<f64>(kK));
                store<f32>(sp + 252, y);
            }
            if (!opn(3) || mover(3)) {
                x = Fsel(x, x, load<f64>(kK));
                store<f32>(sp + 248, x);
            }
            const f32 g = F(kTuning + 72);
            const f32 d2 = Fmadds(x, x, y * y);
            ctx.fpr[1] = d2;
            if (d2 < g * g) {
                if (State(ctx, o, 1) != 1) {
                    SetState(ctx, o, 1, 1);
                    ret(ctx, 1u);
                    return;
                }
                ret(ctx, 0u);
                return;
            }
            const u32 a = static_cast<u32>(call(ctx, kMoverAim, {c, sp + 248})) & 0xFFu;
            SetState(ctx, o, 1, 2);
            SetState(ctx, o, 2, a);
            ret(ctx, 1u);
            return;
        }
    }
    // Below the threshold, or in state 2.
    const u32 a5 = State(ctx, o, 1) - 5;
    bool go = false;
    if (State(ctx, o, 1) == 2) {
        const s32 s2 = static_cast<s32>(State(ctx, o, 2));
        go = (s2 > 10 && s2 <= 13) || (s2 > 18 && s2 <= 21) || (s2 > 26 && s2 <= 29) || (s2 > 34 && s2 <= 37);
    }
    if (!go && a5 != 0) {
        ret(ctx, 0u);
        return;
    }
    if (load<u8>(c + 345) == 0) {
        ret(ctx, 0u);
        return;
    }
    callf(ctx, kMoverCheck, {c}, {F(kTuning2 + 72)});
    if (static_cast<u32>(ctx.gpr[3]) == 0) {
        ret(ctx, 0u);
        return;
    }
    SetState(ctx, o, 1, 10);
    ret(ctx, 1u);
}
