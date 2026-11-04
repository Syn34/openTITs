// SubMachine (vtable 0x00B21AA0) virtuals: Init (+8), Shutdown (+12),
// HandleEvent (+36).
//
// SubMachine fields (from SubMachine_Construct): +32 Effect* (140-byte
// object, 0 if none), +36 u32 key, +40 u32 name, +44 u32, +48 World*,
// +52 u32 flags (bit 31 saved World+0x12828C, bit 30 active, bit 29).
// Extra virtuals: +56 () -> bool, +76 () -> World* (base: +48), +80 enter
// hook, +84 exit hook.
#include "vt_world.h"

using namespace sly4;
using namespace sly4::world;

namespace {

u64 vcall(PPUContext& ctx, u32 obj, u32 slot, std::initializer_list<u64> args) {
    const u32 opd = load<u32>(load<u32>(obj) + slot);
    ctx.gpr[3 + args.size()] = opd;
    ctx.gpr[4 + args.size()] = load<u32>(opd);
    return call_ptr(ctx, opd, args);
}

constexpr u32 kSubEffect = 32, kSubKey = 36, kSubName = 40, kSubWorld = 48, kSubFlags = 52;
constexpr u32 kFlagSaved = 0x80000000u, kFlagActive = 0x40000000u, kFlagBit29 = 0x20000000u;
constexpr u32 kVfnWantsPause = 56, kVfnGetWorld = 76, kVfnOnStart = 80, kVfnOnStop = 84;
constexpr u32 kWorldPauseByte = 0x12828C;    // u8 in the World (name?)

constexpr u32 kEffectRelease = 0x004CF3C4;   // (Effect*, 0x10CBB) (name?)
constexpr u32 kEffectDetached = 0x200;       // Effect +136 flag
constexpr u32 kEffectFlags = 136;
constexpr u32 kFindSet = 0x004BCE90;         // (u32 name) -> Set* (name?)
constexpr u32 kPauseGame = 0x00365A54;       // (bool resume, 0) (name?)
constexpr u32 kSmallAlloc = 0x0055238C;
constexpr u32 kEffectConstruct = 0x004C7A68;  // (mem, def) (name?)
constexpr u32 kEffectStart = 0x004C753C;     // (Effect*) (name?)
constexpr u32 kEffectSetTime = 0x004CF710;   // (Effect*, f32) (name?)
constexpr u32 kEffectStartTime = 0x00490B7C;  // f32 just before Init
constexpr u32 kLastFound = 0x011DE288;       // u32 (name?)

// Set: +16 -> root of a binary tree of nodes {+4 key, +8 value, +12 left
// (key > wanted), +16 right}; +20 path stack (u32[]), +100 s16 depth,
// +104 last node.
constexpr u32 kSetRoot = 16, kSetPath = 20, kSetDepth = 100, kSetLast = 104;

void ReleaseEffect(PPUContext& ctx, u32 m) {
    if (load<u32>(m + kSubEffect) == 0) return;
    call(ctx, kEffectRelease, {load<u32>(m + kSubEffect), 0x10CBB});
    const u32 fx = load<u32>(m + kSubEffect);
    store<u32>(fx + kEffectFlags, load<u32>(fx + kEffectFlags) | kEffectDetached);
    store<u32>(m + kSubEffect, 0);
}

}  // namespace

// void SubMachine_Init(SubMachine* m)
// Enters the level-0 state; then, unless already active, replaces the
// effect with a new one built from the node `key` of the set `name` (the
// lookup leaves its path and last node in the set), pauses the game if
// WantsPause(), calls OnStart, saves and clears the World's pause byte
// into flag bit 31 and sets bits 30 and 29.
DECOMP(00490B80, SubMachine_Init) {
    const u32 m = arg<u32>(ctx, 0);
    const u32 ids = load<u32>(m + kSmIds);
    if (ids != 0 && ref<u8>(ids) != kSmNone) {
        Stack frame(ctx, 16);
        const u32 ev = frame.addr();
        store<u32>(ev, kEventEnter);
        store<u32>(ev + 4, 0);
        store<u32>(ev + 8, kSmNone);
        store<u32>(ev + 12, ref<u8>(ids));
        vcall(ctx, m, kVfnEvent, {m, ev});
    }
    if (load<u32>(m + kSubFlags) & kFlagActive) return;
    ReleaseEffect(ctx, m);

    const u32 set = static_cast<u32>(call(ctx, kFindSet, {load<u32>(m + kSubName)}));
    if (set != 0) {
        u32 node = load<u32>(load<u32>(set + kSetRoot));
        const u32 key = load<u32>(m + kSubKey);
        if (node == 0) {
            store<u32>(set + kSetLast, 0);
            store<u32>(kLastFound, 0);
        } else {
            store<u16>(set + kSetDepth, 0);
            for (;;) {
                const u32 k = load<u32>(node + 4);
                if (k == key) break;
                const u32 next = k > key ? load<u32>(node + 12) : load<u32>(node + 16);
                if (next == 0) break;  // not found: the last node stays in kSetLast below
                const s32 depth = static_cast<s16>(load<u16>(set + kSetDepth));
                store<u16>(set + kSetDepth, static_cast<u16>(depth + 1));
                store<u32>(set + kSetPath + static_cast<u32>(depth) * 4, node);
                node = next;
            }
            store<u32>(set + kSetLast, node);
            if (load<u32>(node + 4) == key) {
                store<u32>(kLastFound, load<u32>(node + 8));
                const u32 def = load<u32>(load<u32>(set + kSetLast) + 8);
                if (def != 0) {
                    const u32 mem = static_cast<u32>(call(ctx, kSmallAlloc, {140}));
                    u32 fx = 0;
                    if (mem != 0) {
                        call(ctx, kEffectConstruct, {mem, def});
                        fx = mem;
                    }
                    store<u32>(m + kSubEffect, fx);
                    store<u32>(fx + kEffectFlags, load<u32>(fx + kEffectFlags) & ~kEffectDetached);
                    call(ctx, kEffectStart, {load<u32>(m + kSubEffect)});
                    ctx.fpr[1] = static_cast<f64>(load<f32>(kEffectStartTime));
                    call(ctx, kEffectSetTime, {load<u32>(m + kSubEffect)});
                }
            }
        }
    }

    if (static_cast<u32>(vcall(ctx, m, kVfnWantsPause, {m})) != 0) call(ctx, kPauseGame, {0, 0});
    vcall(ctx, m, kVfnOnStart, {m});
    const u32 world = load<u32>(m + kSubWorld);
    const u32 saved = ref<u8>(world + kWorldPauseByte);
    store<u32>(m + kSubFlags, (load<u32>(m + kSubFlags) & 0x7FFFFFFFu) | ((saved & 1) << 31));
    ref<u8>(world + kWorldPauseByte) = 0;
    store<u32>(m + kSubFlags, load<u32>(m + kSubFlags) | kFlagActive | kFlagBit29);
}

// void SubMachine_Shutdown(SubMachine* m)
// SetState(0, none); if active: resume the game if WantsPause(), OnStop,
// drop the effect, restore the World's pause byte from bit 31 and clear
// the active bit.
DECOMP(00490E40, SubMachine_Shutdown) {
    const u32 m = arg<u32>(ctx, 0);
    vcall(ctx, m, kVfnSetState, {m, 0, kSmNone});
    if ((load<u32>(m + kSubFlags) & kFlagActive) == 0) return;
    if (static_cast<u32>(vcall(ctx, m, kVfnWantsPause, {m})) != 0) call(ctx, kPauseGame, {1, 0});
    vcall(ctx, m, kVfnOnStop, {m});
    ReleaseEffect(ctx, m);
    const u32 world = static_cast<u32>(vcall(ctx, m, kVfnGetWorld, {m}));
    ref<u8>(world + kWorldPauseByte) = (load<u32>(m + kSubFlags) & kFlagSaved) ? 1 : 0;
    store<u32>(m + kSubFlags, load<u32>(m + kSubFlags) & ~kFlagActive);
}

// bool SubMachine_HandleEvent(SubMachine*, Event*): returns 1.
DECOMP(0077D8F8, SubMachine_HandleEvent) { ret(ctx, 1u); }
