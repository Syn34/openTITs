// World_HandleEvent callees, part 29e: the world's countdown timer update
// (the "%i" counter shown on a slot object or a player, with its sounds and
// the fade out when it runs out).
#include "hevent29.h"

using namespace sly4;
using namespace sly4::hevent;
using namespace sly4::hevent28;
using namespace sly4::hevent29;

namespace {
namespace fn29e {
constexpr u32 World_EndFade = 0x00466678;     // (World*, u8) (World_EndFade466678)
constexpr u32 StateMachine_SetState = 0x00464D1C;  // (sm, level, id)
constexpr u32 SortedMap_Find3 = 0x0076611C;   // (map, const u32* key) -> value slot
constexpr u32 Request_Forward = 0x003694A0;   // (a, b, c, d, u8 e) (Request_Forward3694A0)
constexpr u32 Effect_FindComponent = 0x004BB4B4;  // (Effect*, u32 type hash) -> component
constexpr u32 Handle_ResolveBit1 = 0x003568E0;  // (handle) -> Object*
constexpr u32 Obj_Dispatch = 0x0036325C;      // (o, 0, 1, 0) (Obj_Dispatch362)
constexpr u32 Obj_SetAnim = 0x00363A3C;       // (o, name, now, -, b, c, d; f32)
constexpr u32 Obj_SlotSet = 0x0036901C;       // (Obj*, u32 x, u32 i) (Obj_SlotSet36901C)
constexpr u32 Player_Set = 0x003632C8;        // (o, value, which) (Player_Set3632C8)
constexpr u32 Obj_SlotNotify370938 = 0x00370938;  // (Obj*, u8, u32 i)
constexpr u32 Obj_SlotNotify36BFAC = 0x0036BFAC;  // (Obj*, u8, u32 i)
constexpr u32 Obj_SlotPos3709A0 = 0x003709A0;     // (f32 out[3], Obj*, u32 i)
constexpr u32 Obj_SlotPos36C06C = 0x0036C06C;     // (f32 out[3], Obj*, u32 i)
constexpr u32 Node_Push = 0x00446984;         // (a, b) -> node (Node_PushF65B7C)
constexpr u32 Sound_Play = 0x0044C474;        // (out, SoundDesc**, ...) 11 arguments
constexpr u32 RefNode_Release = 0x004432DC;   // (node)
constexpr u32 Request_Post = 0x0036A050;      // (a, -, c; f1) (Request_Post36A050)
constexpr u32 String_Format = 0x005534D0;     // (fmt, ...)
}  // namespace fn29e

// World fields (the timer block at +0x128458).
constexpr u32 kTKey = 0x128458;      // u32: object id (0x0106C228 map)
constexpr u32 kTSlotObj = 0x12845C;  // Obj*: the timer's object when it has +4 bit 2
constexpr u32 kTPlayer = 0x128460;   // Obj*: else, when it has +4 bit 0
constexpr u32 kTTarget = 0x128464;   // f32: the count to reach
constexpr u32 kTValue = 0x128468;    // f32: the count
constexpr u32 kTRate = 0x12846C;     // f32: per second
constexpr u32 kTDelay = 0x128470;    // f32: the delay before the fade once reached
constexpr u32 kTFlags = 0x128474;    // bit 0x80000000 forwarded, 0x40000000 active, 0x20000000 started
constexpr u32 kTName = 0x1283C0, kTArgB = 0x128401, kTArgC = 0x128444;
constexpr u32 kFlagFwd = 0x80000000u, kFlagActive = 0x40000000u, kFlagStarted = 0x20000000u;
constexpr u32 kWStateTable = 8;   // u8* (its first byte copied to +47)
constexpr u32 kWLevelOut = 20;    // u8*: indexed by +6
constexpr u32 kWLevelIdx = 6;     // u16
// Globals.
constexpr u32 kObjectsById = 0x0106C228;  // {id, Object*} map (as hevent28 g_ObjectsById)
constexpr u32 kHandleB = 0x00F64CDC;
constexpr u32 kZero = 0x0048A690, kMinusOne = 0x0048A6A8;  // f32 0, -1
constexpr u32 kAnimArg = 0x01164CA4;
constexpr u32 kMinHeight = 0x010B8184 + 72;  // f32: the camera height (+12 +8) must be above (name?)
constexpr u32 kSoundBank = 0x010B8054 + 72;  // u32: second Node_Push argument (name?)
constexpr u32 kDelayInit = 0x010B821C + 72;  // f32: kTDelay once the count is reached (name?)
constexpr u32 kZeroPos = 0x01164B40;         // f32[3]
constexpr u32 kSndStart = 0x1EF09B4D, kSndReached = 0x27B8586F, kSndDone = 0xEF586B1D;
constexpr u32 kEffectTimer = 0xE7CEA5A4;  // component type hash
constexpr u32 kStrZero = 0x00AE91C8;      // "0"
constexpr u32 kFmtInt = 0x00AE91D0;       // "%i"

u32 fctidz_lo(f64 x) { return static_cast<u32>(rt::fctid(x, true)); }

// Node_Push(hash, bank) -> Sound_Play(&out, node, 0...) -> RefNode_Release.
void play(PPUContext& ctx, u32 out, u32 hash) {
    const u32 n = static_cast<u32>(call(ctx, fn29e::Node_Push, {hash, load<u32>(kSoundBank)}));
    call(ctx, fn29e::Sound_Play, {out, n, 0, 0, 0, 0, 0, 0, 0, 0, 0});
    call(ctx, fn29e::RefNode_Release, {n});
}
}  // namespace

// u32 World_UpdateTimer48A6B0(World* w; f32 dt) (name?): returns 1.
//  - inactive (+0x128474 bit 0x40000000 clear): World_EndFade466678(w, 0);
//  - sub-state (vt +32 level 2) 0: the timer object looked up by id (slot
//    object with +4 bit 2 forwarded the name, else a player with bit 0
//    animated), the timer effect's +20 set to the target count; when the
//    target is above 0 the 0x00F64CDC object dispatched; the "0" / zero
//    slots shown, then the state advanced (vt +32 + 1);
//  - sub-state 1: while the camera is high enough the count rises by
//    rate * dt up to the target (sounds when started and when reached,
//    then the delay), shown with "%i"; when the delay runs out a sound at
//    the slot position and Request_Post36A050; finally the level's fade
//    out unless the slot object is in mode 1 / the player is flagged.
DECOMP(0048A6B0, World_UpdateTimer48A6B0) {
    const u32 w = arg<u32>(ctx, 0);
    const f64 dt = farg(ctx, 0);
    Stack frame(ctx, 80);  // +0 key, +4 / +8 / +12 sound outputs, +16 / +32 / +48 vec3
    const u32 key = frame.addr(), out_a = key + 4, out_b = key + 8, out_c = key + 12;
    const u32 pos = key + 16, tmp = key + 32;

    if ((load<u32>(w + kTFlags) & kFlagActive) == 0) {
        call(ctx, fn29e::World_EndFade, {w, 0});
        ctx.gpr[3] = 1;
        return;
    }
    const s32 sub = static_cast<s32>(vcall(ctx, w, 32, {w, 2}));
    if (sub > 1 || sub <= -1) {
        ctx.gpr[3] = 1;
        return;
    }

    if (sub == 0) {
        const u32 id = load<u32>(w + kTKey);
        store<u32>(key, id);
        u32 obj = 0;
        if (id != 0) obj = load<u32>(static_cast<u32>(call(ctx, fn29e::SortedMap_Find3, {kObjectsById, key})));
        const u32 slot = (obj != 0 && (load<u32>(obj + 4) & 4u) != 0) ? obj : 0;
        store<u32>(w + kTSlotObj, slot);
        u32 fx = 0;
        bool have = false;
        if (slot != 0) {
            const u32 fwd = (load<u32>(w + kTFlags) & kFlagFwd) != 0 ? 1 : 0;
            call(ctx, fn29e::Request_Forward, {slot, w + kTName, w + kTArgB, w + kTArgC, fwd});
            fx = load<u32>(load<u32>(w + kTSlotObj) + 20);
            have = true;
        } else {
            const u32 pl = (obj != 0 && (load<u32>(obj + 4) & 1u) != 0) ? obj : 0;
            store<u32>(w + kTPlayer, pl);
            if (pl != 0) {
                callf(ctx, fn29e::Obj_SetAnim, {pl, w + kTName, 1, kZero, 0, 0, kAnimArg}, {lfs_at(kMinusOne)});
                fx = load<u32>(load<u32>(w + kTPlayer) + 20);
                have = true;
            }
        }
        if (have && fx != 0) {
            const u32 c = static_cast<u32>(call(ctx, fn29e::Effect_FindComponent, {fx, kEffectTimer}));
            if (c != 0 && static_cast<s32>(vcall(ctx, c, 32, {c})) == 2)
                store<u32>(c + 20, fctidz_lo(lfs_at(w + kTTarget)));
        }

        const f64 zero = lfs_at(kZero);
        f64 t = lfs_at(w + kTTarget);
        ctx.fpr[1] = t;
        if (t > zero) {
            const u32 h = static_cast<u32>(call(ctx, fn29e::Handle_ResolveBit1, {load<u32>(kHandleB)}));
            if (h != 0) call(ctx, fn29e::Obj_Dispatch, {h, 0, 1, 0});
        }
        const u32 so = load<u32>(w + kTSlotObj);
        if (so != 0) {
            t = lfs_at(w + kTTarget);
            ctx.fpr[1] = t;
            if (t > zero) {
                call(ctx, fn29e::Obj_SlotNotify370938, {so, 1, 2});
                call(ctx, fn29e::Obj_SlotSet, {load<u32>(w + kTSlotObj), kStrZero, 2});
                call(ctx, fn29e::Obj_SlotNotify370938, {load<u32>(w + kTSlotObj), 1, 3});
                call(ctx, fn29e::Obj_SlotSet, {load<u32>(w + kTSlotObj), kStrZero, 3});
            }
        } else if (load<u32>(w + kTPlayer) != 0) {
            t = lfs_at(w + kTTarget);
            ctx.fpr[1] = t;
            if (t > zero) {
                call(ctx, fn29e::Obj_SlotNotify36BFAC, {load<u32>(w + kTPlayer), 1, 2});
                call(ctx, fn29e::Player_Set, {load<u32>(w + kTPlayer), kStrZero, 2});
                call(ctx, fn29e::Obj_SlotNotify36BFAC, {load<u32>(w + kTPlayer), 1, 3});
                call(ctx, fn29e::Player_Set, {load<u32>(w + kTPlayer), kStrZero, 3});
            } else {
                call(ctx, fn29e::Obj_SlotNotify36BFAC, {load<u32>(w + kTPlayer), 0, 2});
                call(ctx, fn29e::Obj_SlotNotify36BFAC, {load<u32>(w + kTPlayer), 0, 3});
            }
        }
        const u32 next = static_cast<u32>(vcall(ctx, w, 32, {w, 2})) + 1;
        const u32 b = load<u8>(load<u32>(w + kWStateTable));
        store<u8>(w + 47, static_cast<u8>(b));
        call(ctx, fn29e::StateMachine_SetState, {w, 2, next & 0xFF, b});
        ctx.gpr[3] = 1;
        return;
    }

    // Sub-state 1: the count.
    f64 v = lfs_at(w + kTValue), t = lfs_at(w + kTTarget);
    if (v != t && lfs_at(load<u32>(w + 12) + 8) > lfs_at(kMinHeight)) {
        const u32 fl = load<u32>(w + kTFlags);
        if ((fl & kFlagStarted) == 0) {
            store<u32>(w + kTFlags, fl | kFlagStarted);
            play(ctx, out_c, kSndStart);
            v = lfs_at(w + kTValue);
            t = lfs_at(w + kTTarget);
        }
        f64 f2 = t;
        v = fmadds(lfs_at(w + kTRate), dt, v);
        store<f32>(w + kTValue, static_cast<f32>(v));
        if (v > f2) {
            store<u32>(w + kTValue, load<u32>(w + kTTarget));  // stfs of the target (raw)
            play(ctx, out_b, kSndReached);
            v = lfs_at(w + kTValue);
            store<u32>(w + kTDelay, load<u32>(kDelayInit));
            f2 = lfs_at(w + kTTarget);
        }
        ctx.fpr[1] = v;
        if ((load<u32>(w + kTSlotObj) != 0 || load<u32>(w + kTPlayer) != 0) && f2 > lfs_at(kZero)) {
            const u32 txt = static_cast<u32>(call(ctx, fn29e::String_Format, {kFmtInt, fctidz_lo(v)}));
            const u32 so = load<u32>(w + kTSlotObj);
            if (so != 0) {
                call(ctx, fn29e::Obj_SlotSet, {so, txt, 2});
                call(ctx, fn29e::Obj_SlotSet, {load<u32>(w + kTSlotObj), txt, 3});
            } else {
                call(ctx, fn29e::Player_Set, {load<u32>(w + kTPlayer), txt, 2});
                call(ctx, fn29e::Player_Set, {load<u32>(w + kTPlayer), txt, 3});
            }
        }
    }

    // The delay.
    const f64 zero = lfs_at(kZero);
    ctx.fpr[1] = zero;
    f64 d = lfs_at(w + kTDelay);
    if (d > zero) {
        d = fsubs(d, dt);
        store<f32>(w + kTDelay, static_cast<f32>(d));
        if (d < zero || d == zero) {
            store<u32>(w + kTDelay, 0);
            for (u32 k = 0; k < 12; k += 4) store<u32>(pos + k, load<u32>(kZeroPos + k));
            const u32 so = load<u32>(w + kTSlotObj);
            if (so != 0 || load<u32>(w + kTPlayer) != 0) {
                if (so != 0) call(ctx, fn29e::Obj_SlotPos3709A0, {tmp, so, 3});
                else call(ctx, fn29e::Obj_SlotPos36C06C, {tmp, load<u32>(w + kTPlayer), 3});
                for (u32 k = 0; k < 12; k += 4) store<u32>(pos + k, load<u32>(tmp + k));
            }
            play(ctx, out_a, kSndDone);
            // r4 is left holding the field offset (not its address); unused.
            callf(ctx, fn29e::Request_Post, {w + kUserBlocks, kTTarget, pos}, {lfs_at(w + kTTarget)});
        }
    }

    // The fade out.
    const u32 so = load<u32>(w + kTSlotObj);
    bool fade;
    if (so != 0) {
        fade = ((load<u32>(so + 160) >> 1) - 1) != 0;
    } else {
        fade = load<u32>(w + kTPlayer) == 0;
    }
    if (!fade) {
        const u32 pl = load<u32>(w + kTPlayer);
        fade = pl != 0 && (load<u32>(pl + 396) & 0x40000000u) == 0;
    }
    if (fade) call(ctx, fn29e::World_EndFade, {w, load<u8>(load<u32>(w + kWLevelOut) + load<u16>(w + kWLevelIdx))});
    ctx.gpr[3] = 1;
}
