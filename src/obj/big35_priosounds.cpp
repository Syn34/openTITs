// Game code, large-function sweep part 35: one pass of the sound priority
// lists (PrioList_Update445A40 runs it over both lists).
#include "../sly4.h"
#include "big35.h"

using namespace sly4;
using namespace sly4::big35;

namespace {
// The emitter handle pool: +24 s32 count, +28 272-byte entries (+16 voice,
// +20 owner, +29 u8, +256 u32: generation in bits 15..30, bit 31 free).
constexpr u32 kEmitters = 0x010B0200;
constexpr u32 kAudioReady = 0x010AEC60;  // u8: handles are validated only when set
constexpr u32 kAudioDirty = 0x00F65B88;  // u8
constexpr u32 kFmtStolen = 0x00AE46E4;   // "Buffered stream got stolen out from under the stream queue - ... %s ..."
constexpr u32 kFmtStopping = 0x00AE4684; // "Stopping buffered queueud sound %s - ..."
constexpr u32 kUnknown = 0x00AE4674;     // "[UNKNOWN]"
constexpr u32 Emitter_Stop = 0x00443424;      // (handle, bool fade)
constexpr u32 Handle_Acquire = 0x00443EC0;    // (u32* out, Slot*, Params*)
constexpr u32 Sound_Free = 0x00445390;        // (SoundMgr*, Sound*)
constexpr u32 String_Format = 0x005534D0;     // (fmt, ...)
constexpr u32 Voice_IsPlaying = 0x006D5E10;   // (voice) -> bool
constexpr u32 Voice_Stop = 0x006D60AC;        // (voice, 1)

// The pool entry of a live handle h (low half the generation, high half
// the index), or 0.
u32 LiveEntry(u32 h) {
    const u32 gen = h & 0xFFFF;
    if (gen == 0) return 0;
    const s32 idx = static_cast<s32>((h >> 16) & 0xFFFF);
    if (!(idx < static_cast<s32>(load<u32>(kEmitters + 24)))) return 0;
    const u32 e = load<u32>(kEmitters + 28) + static_cast<u32>(idx) * 272u;
    if (((load<u32>(e + 256) >> 15) & 0xFFFF) != gen) return 0;
    if ((load<u8>(e + 256) & 0x80) != 0) return 0;
    return e;
}

// Takes sound n off its list (+116 prev, +120 next; at the ends the
// manager's +8200 head / +8204 tail for sounds with +96 set, else the +8208
// head, which has no tail), clears +97 / +99, then frees it
// (Sound_Free445390) when +108 is 0, else pushes it onto the free list at
// the manager's +8196.
void Detach(PPUContext& ctx, u32 mgr, u32 n) {
    if (const u32 prev = load<u32>(n + 116); prev != 0)
        store<u32>(prev + 120, load<u32>(n + 120));
    else if (load<u8>(n + 96) != 0)
        store<u32>(mgr + 8200, load<u32>(n + 120));
    else
        store<u32>(mgr + 8208, load<u32>(n + 120));
    if (const u32 next = load<u32>(n + 120); next != 0)
        store<u32>(next + 116, load<u32>(n + 116));
    else if (load<u8>(n + 96) != 0)
        store<u32>(mgr + 8204, load<u32>(n + 116));
    const u32 keep = load<u32>(n + 108);
    store<u8>(n + 97, 0);
    store<u8>(n + 99, 0);
    if (keep == 0) {
        call(ctx, Sound_Free, {mgr, n});
        return;
    }
    store<u32>(n + 120, load<u32>(mgr + 8196));
    if (const u32 f = load<u32>(mgr + 8196); f != 0) store<u32>(f + 116, n);
    store<u32>(n + 116, 0);
    store<u32>(mgr + 8196, n);
}
}  // namespace

// void PrioList_UpdateSounds4454B4(SoundMgr* mgr, Sound* n, s32* budget,
// f32 dt): for each sound of the list from n (+120 next, read first): +102
// cleared when set; a playing one (+97) gets +112 += dt (fadds; f1 left as
// that sum). Its cost is its definition's (*n->0) +88:
//  - no definition: Emitter_Stop(n->4, 0) and the sound is detached (cost
//    0);
//  - not playing: +103 = 0; then, unless it is a +96 sound with 4 such
//    already counted (cost 0), +103 = 1 (and the count bumped for a +96
//    sound) when cost < *budget;
//  - playing, its voice alive (with kAudioReady: a live handle n->4 whose
//    voice Voice_IsPlaying; without, never alive) and within budget: the
//    count bumped for +96; with +100, Handle_Acquire(&tmp, n, 0) and +100 =
//    kAudioDirty;
//  - playing, alive and over budget: reported for +96, then stopped
//    (Emitter_Stop(n->4, 0) when the definition's +96 is clear, else the
//    live handle's voice gets Voice_Stop(voice, 1), +13 of the entry's
//    +16 block set first when its owner's +96 is) and detached;
//  - playing but not alive: reported unless +98, then detached.
// Each sound then takes its cost off *budget (signed compares). r3 is the
// last budget stored (mgr for an empty list).
DECOMP(004454B4, PrioList_UpdateSounds4454B4) {
    const u32 mgr = arg<u32>(ctx, 0);
    u32 n = arg<u32>(ctx, 1);
    const u32 budget = arg<u32>(ctx, 2);
    const f64 dt = farg(ctx, 0);
    if (n == 0) return;  // r3 stays mgr
    u32 count = 0;  // +96 sounds kept playing (r29)
    u64 left = 0;
    Stack frame(ctx, 16);  // sp+112: Handle_Acquire's output
    do {
        u32 cost = 0;
        const u32 next = load<u32>(n + 120);
        if (load<u8>(n + 102) != 0) store<u8>(n + 102, 0);
        const bool playing = load<u8>(n + 97) != 0;
        if (playing) {
            const f64 t = Adds(Lfs(n + 112), dt);
            store<u32>(n + 112, std::bit_cast<u32>(static_cast<f32>(t)));
            ctx.fpr[1] = t;
        }
        const u32 defp = load<u32>(n + 0);
        const u32 d = load<u32>(defp);
        if (d == 0) {
            call(ctx, Emitter_Stop, {load<u32>(n + 4), 0});
            Detach(ctx, mgr, n);
        } else {
            const u32 def = defp == 0 ? 0u : d;
            cost = load<u32>(def + 88);
            store<u8>(n + 103, 0);
            if (!playing) {
                const bool queued = load<u8>(n + 96) != 0;
                if (queued && !(static_cast<s32>(count) < 4)) {
                    cost = 0;
                } else if (static_cast<s32>(cost) < static_cast<s32>(load<u32>(budget))) {
                    store<u8>(n + 103, 1);
                    if (queued) ++count;
                }
            } else {
                bool alive = false;
                const u32 h = load<u32>(n + 4);
                if (load<u8>(kAudioReady) != 0) {
                    if (const u32 e = LiveEntry(h); e != 0)
                        alive = static_cast<u32>(call(ctx, Voice_IsPlaying, {load<u32>(e + 16)})) != 0;
                }
                if (!alive) {
                    if (load<u8>(n + 98) == 0) call(ctx, String_Format, {kFmtStolen, kUnknown});
                    Detach(ctx, mgr, n);
                } else if (!(static_cast<s32>(cost) > static_cast<s32>(load<u32>(budget)))) {
                    if (load<u8>(n + 96) != 0) ++count;
                    if (load<u8>(n + 100) != 0) {
                        call(ctx, Handle_Acquire, {frame.addr(), n, 0});
                        store<u8>(n + 100, load<u8>(kAudioDirty));
                    }
                } else {
                    if (load<u8>(n + 96) != 0) call(ctx, String_Format, {kFmtStopping, kUnknown});
                    if (load<u8>(def + 96) == 0) {
                        call(ctx, Emitter_Stop, {load<u32>(n + 4), 0});
                    } else if (load<u8>(kAudioReady) != 0) {
                        if (const u32 e = LiveEntry(load<u32>(n + 4)); e != 0) {
                            const u32 v = e + 16;
                            if (load<u8>(load<u32>(v + 4) + 96) != 0) store<u8>(v + 13, 1);
                            call(ctx, Voice_Stop, {load<u32>(v), 1});
                        }
                    }
                    Detach(ctx, mgr, n);
                }
            }
        }
        left = static_cast<u64>(load<u32>(budget)) - cost;  // subfc on the zero-extended words
        store<u32>(budget, static_cast<u32>(left));
        n = next;
    } while (n != 0);
    ret(ctx, left);
}
