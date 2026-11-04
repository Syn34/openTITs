// World_HandleEvent callees, part 11j: the per-user interaction prompts.
#include "hevent11.h"

using namespace sly4;
using namespace sly4::hevent;
using namespace sly4::hevent7;
using namespace sly4::hevent11;

namespace {
// User object (user block +8): +96 Mat34 (3 rows used), +184 u64 flags (bit
// 0 ready, bit 15 busy), +192 u32 flags (bit 3 active), +384 job (+16 u8
// running), +656 +44 / +45 u8 prompt states (bit 7 on, bit 6 suppressed).
constexpr u32 kUbObject = 8;
constexpr u32 kObjMat = 96, kObjFlags184 = 188, kObjFlags192 = 192, kObjJob = 384, kObjPrompts = 656;
constexpr u32 kFlagReady = 1, kFlagBusy = 0x8000, kFlagActive = 8;
constexpr u32 kPromptA = 44, kPromptB = 45;
constexpr u32 kVtTarget = 376, kVtState = 32, kVtGetPos = 284, kVtReach = 64;  // object vtable slots
constexpr s32 kStateIdle6 = 6, kStateWait7 = 7, kStateWait8 = 8;
constexpr u32 kJobRunning = 16;
// Prompt globals.
constexpr u32 kPromptsOn = 0x00F64CC4;      // u8
constexpr u32 kPromptHandle = 0x00F64D08;   // u32 handle of the action prompt
constexpr u32 kWarnHandle = 0x00F64CCC;     // u32 handle of the warning prompt
constexpr u32 kWarnShown = 0x010BB750;      // u8
constexpr u32 kJobState = 0x00F64C20;       // u32: +40 u32, +68 current job target
constexpr u32 kPromptTimes = 0x0048FCB8;    // f32 {action fade, warning fade}
constexpr u32 kAnimDefault = 0x0106C40C, kAnimCurrent = 0x0106C4D4;  // prompt text ids (name?)
constexpr u32 kAnimWarn = 0x00AE93D0;       // "$STR_HIDEOUT_ABANDONJOB_WARN"
constexpr u32 kAnimArg = 0x01164CA4;
constexpr u32 kPrompt396Hidden = 0x40000000;  // Object +396 bit 30
constexpr u32 kTargetFlags = 192, kTargetNeedsCheck = 0x40 /* usable when strict */, kTargetInfo = 216, kInfoFlags = 452;
constexpr u32 kInfoUsable = 0x08000000;
constexpr u32 kObjDirty2337 = 2337;
constexpr u32 kResolve = 0x003568E0;        // Handle_ResolveBit1 (handle) -> Object*
constexpr u32 kSetAnim = 0x00363A3C;        // Obj_SetAnim (o, id, 1, -, 0, 0, arg; f32)
constexpr u32 kDispatch = 0x0036325C;       // Obj_Dispatch362 (o, 1, u8, 0)
constexpr u32 kNotify24 = 0x000D949C;       // Obj_Notify24 (o) -> Object*
constexpr u32 kTargetDef = 0x000D951C;      // (target) -> def or 0 (name?)
constexpr u32 kPromptFade = 0x00367758;     // (Object*; f32) (name?)
constexpr u32 kNextTarget = 0x00271C08;     // (from, 3, prev) -> next target or 0 (name?)
constexpr u32 kTransform = 0x00553B4C;      // (Mat34* out, const Mat33*, const vec3* pos) (name?)
constexpr u32 kInReach = 0x002B79F8;        // (def, const Mat34*, reach) -> non-zero (name?)

bool prompt_on(u8 b) { return (b & 0x80) != 0 && (b & 0x40) == 0; }
}  // namespace

// u32 Hud_UpdatePrompts48FCC0() (name?): for each ready, active user object
// with a prompt on (+656 +44 / +45): finds the target it can act on (the
// first in reach through 0x00271C08 / 0x002B79F8 while its job runs and it
// is neither idle nor busy; with only prompt B on, only targets with +192
// bit 6 and info +452 bit 27), shows the action prompt for it (fade, text by the
// target def or the current job's), or dispatches it hidden; then shows
// the abandon-job warning for the current job's target, or hides it and
// marks the user's object (+2337 bit 4). r3 ends as the user count.
DECOMP(0048FCC0, Hud_UpdatePrompts48FCC0) {
    u32 i = 0, n;
    for (;; ++i) {
        const u32 world = load<u32>(kCurrentWorld);
        if (!(i < (n = load<u8>(world + 29)))) break;
        const u32 ub = world + i * kUserBlockSize + kUserBlocks;
        const u32 o = load<u32>(ub + kUbObject);
        if (o == 0 || (load<u32>(o + kObjFlags192) & kFlagActive) == 0) continue;
        if ((load<u32>(o + kObjFlags184) & kFlagReady) == 0) continue;
        const u32 p = o + kObjPrompts;
        if (!prompt_on(load<u8>(p + kPromptA)) && !prompt_on(load<u8>(p + kPromptB))) continue;
        const u32 from = static_cast<u32>(vcall(ctx, o, kVtTarget, {o}));
        if (from == 0) continue;
        u32 strict = 0;
        if (prompt_on(load<u8>(p + kPromptB)) && !prompt_on(load<u8>(p + kPromptA))) strict = 1;  // only prompt B
        u32 target = 0, searched = 0;
        const u32 job = load<u32>(o + kObjJob);
        if (job != 0 && load<u8>(job + kJobRunning) != 0) {
            bool search = true;
            if (static_cast<u32>(vcall(ctx, o, kVtState, {o, 0})) == 0 &&
                static_cast<s32>(vcall(ctx, o, kVtState, {o, 1})) == kStateIdle6)
                search = false;
            if (search && static_cast<s32>(vcall(ctx, o, kVtState, {o, 0})) == kStateWait7 &&
                static_cast<s32>(vcall(ctx, o, kVtState, {o, 1})) == kStateWait8)
                search = false;
            if (search && (load<u32>(o + kObjFlags184) & kFlagBusy) != 0) search = false;
            if (search) {
                searched = 1;
                // r1+112 position, +128 Mat33 copy (rows of 16), +176 Mat34.
                Stack frame(ctx, 128);
                const u32 pos = frame.addr(), rot = pos + 16, mat = pos + 64;
                for (u32 r = 0; r < 3; ++r)
                    for (u32 k = 0; k < 3; ++k) store<u32>(rot + r * 16 + k * 4, load<u32>(o + kObjMat + r * 16 + k * 4));
                ctx.fpr[1] = lfs_f64(o + kObjMat + 40);  // left in f1 by the original's copy
                vcall(ctx, o, kVtGetPos, {pos, o});
                call(ctx, kTransform, {mat, rot, pos});
                target = static_cast<u32>(call(ctx, kNextTarget, {from, 3, 0}));
                while (target != 0) {
                    const u32 def = static_cast<u32>(call(ctx, kTargetDef, {target}));
                    bool check = def != 0;
                    if (check && strict != 0) {
                        check = (load<u32>(target + kTargetFlags) & kTargetNeedsCheck) != 0 &&
                                (load<u32>(load<u32>(target + kTargetInfo) + kInfoFlags) & kInfoUsable) != 0;
                    }
                    if (check) {
                        const u32 reach = static_cast<u32>(vcall(ctx, o, kVtReach, {o}));
                        if (static_cast<u32>(call(ctx, kInReach, {def, mat, reach})) != 0) break;
                    }
                    target = static_cast<u32>(call(ctx, kNextTarget, {from, 3, target}));
                }
            }
        }
        if (load<u8>(kPromptsOn) == 0) continue;
        const u32 h = static_cast<u32>(call(ctx, kResolve, {load<u32>(kPromptHandle)}));
        if (h != 0) {
            if (target != 0 && (load<u32>(h + 396) & kPrompt396Hidden) == 0) {
                const u32 def = static_cast<u32>(call(ctx, kTargetDef, {target}));
                const f64 fade = lfs_f64(kPromptTimes);
                callf(ctx, kPromptFade, {h}, {fade});
                u32 text = def != 0 ? load<u32>(def + 128) : 0u;
                if (text == 0) text = kAnimDefault;
                if (target == load<u32>(load<u32>(kJobState) + 68)) text = kAnimCurrent;
                callf(ctx, kSetAnim, {h, text, 1, 0, 0, 0, kAnimArg}, {fade});  // r6 is not read
            } else if (searched != 0 && target == 0) {
                call(ctx, kDispatch, {h, 1, 0, 0});
            }
        }
        const u32 w = static_cast<u32>(call(ctx, kResolve, {load<u32>(kWarnHandle)}));
        if (w == 0) continue;
        const u32 was = load<u8>(kWarnShown);
        store<u8>(kWarnShown, 0);
        u32 warn = 0;
        if (target != 0) {
            const u32 js = load<u32>(kJobState);
            if (target == load<u32>(js + 68) && load<u32>(js + 40) != 0) {
                warn = 1;
                store<u8>(kWarnShown, 1);
            }
        }
        if (was == 0 && warn != 0) {
            callf(ctx, kSetAnim, {w, kAnimWarn, 1, warn, 0, 0, kAnimArg}, {lfs_f64(kPromptTimes + 4)});
            continue;
        }
        if (was == 0 || warn != 0) continue;
        if ((load<u32>(w + 396) & kPrompt396Hidden) != 0) call(ctx, kDispatch, {w, 1, 1, 0});
        const u32 o2 = load<u32>(ub + kUbObject);
        if (o2 == 0) continue;
        const u32 q = static_cast<u32>(call(ctx, kNotify24, {o2}));
        if (q == 0) continue;
        store<u8>(q + kObjDirty2337, load<u8>(q + kObjDirty2337) | 0x10);
    }
    ret(ctx, n);
}
