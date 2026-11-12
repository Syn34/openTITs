// Game code, large-function sweep part 35: one pass of the sound priority
// lists (PrioList_Update445A40 runs it over both lists).
#include "native/objtypes.h"

namespace sly4::obj {

struct SoundDef {
    pad<88> _0;
    be_s32 cost;     // +88
    pad<4> _92;
    u8 direct;       // +96: stop through the voice
};
struct SoundOwner {
    pad<96> _0;
    u8 flag;         // +96
};
// The emitter handle pool's 272-byte entry.
struct EmitterEntry {
    pad<16> _0;
    be_u32 voice;          // +16
    gp<SoundOwner> owner;  // +20
    pad<5> _24;
    u8 stopping;           // +29
    pad<226> _30;
    be_u32 gen;            // +256: generation in bits 15..30, bit 31 free
    pad<12> _260;
};
static_assert(sizeof(EmitterEntry) == 272 && offsetof(EmitterEntry, gen) == 256);
struct EmitterPool {
    pad<24> _0;
    be_s32 count;              // +24
    gp<EmitterEntry> entries;  // +28
};
struct Sound {
    be_u32 defp;     // +0: SoundDef** (read through even when 0)
    be_u32 handle;   // +4
    pad<88> _8;
    u8 queued;       // +96
    u8 playing;      // +97
    u8 quiet;        // +98
    u8 b99;          // +99
    u8 reacquire;    // +100
    u8 b101;         // +101
    u8 b102;         // +102
    u8 allowed;      // +103
    pad<4> _104;
    be_u32 keep;     // +108
    be_f32 t;        // +112
    gp<Sound> prev;  // +116
    gp<Sound> next;  // +120
};
static_assert(offsetof(Sound, keep) == 108 && offsetof(Sound, next) == 120);
struct SoundMgr {
    pad<8196> _0;
    gp<Sound> free_list;  // +8196
    gp<Sound> head;       // +8200: the +96 sounds
    gp<Sound> tail;       // +8204
    gp<Sound> head2;      // +8208: the others
};
static_assert(offsetof(SoundMgr, head2) == 8208);

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
EmitterEntry* LiveEntry(u32 h) {
    const u32 gen = h & 0xFFFF;
    if (gen == 0) return nullptr;
    const s32 idx = static_cast<s32>((h >> 16) & 0xFFFF);
    EmitterPool& pool = global<EmitterPool>(kEmitters);
    if (!(idx < pool.count)) return nullptr;
    EmitterEntry& e = gm<EmitterEntry>(pool.entries.ea + static_cast<u32>(idx) * 272u);
    if (((e.gen >> 15) & 0xFFFF) != gen) return nullptr;
    if ((reinterpret_cast<const u8*>(&e.gen)[0] & 0x80) != 0) return nullptr;
    return &e;
}

// Takes sound n off its list (+116 prev, +120 next; at the ends the
// manager's +8200 head / +8204 tail for sounds with +96 set, else the +8208
// head, which has no tail), clears +97 / +99, then frees it (Sound_Free)
// when +108 is 0, else pushes it onto the free list at the manager's
// +8196.
void Detach(SoundMgr* mgr, Sound* n) {
    if (Sound* prev = n->prev)
        prev->next = n->next;
    else if (n->queued != 0)
        mgr->head = n->next;
    else
        mgr->head2 = n->next;
    if (Sound* next = n->next)
        next->prev = n->prev;
    else if (n->queued != 0)
        mgr->tail = n->prev;
    const u32 keep = n->keep;
    n->playing = 0;
    n->b99 = 0;
    if (keep == 0) {
        call(Sound_Free, mgr, n);
        return;
    }
    n->next = mgr->free_list;
    if (Sound* f = mgr->free_list) f->prev = n;
    n->prev = nullptr;
    mgr->free_list = n;
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
// Each sound then takes its cost off *budget (signed compares).
void PrioList_UpdateSounds4454B4(SoundMgr* mgr, Sound* n, be_u32* budget, f32 dt) {
    if (n == nullptr) return;
    u32 count = 0;  // +96 sounds kept playing
    GuestFrame frame(16);  // Handle_Acquire's output
    do {
        u32 cost = 0;
        Sound* next = n->next;
        if (n->b102 != 0) n->b102 = 0;
        const bool playing = n->playing != 0;
        if (playing) n->t = static_cast<f32>(Adds(lf(n->t), dt));
        const u32 defp = n->defp;
        const u32 d = gm<be_u32>(defp);  // quirk: read at address 0 when there is no pointer
        if (d == 0) {
            call(Emitter_Stop, n->handle.get(), 0u);
            Detach(mgr, n);
        } else {
            SoundDef& def = gm<SoundDef>(defp == 0 ? 0u : d);
            cost = static_cast<u32>(def.cost.get());
            n->allowed = 0;
            if (!playing) {
                const bool queued = n->queued != 0;
                if (queued && !(static_cast<s32>(count) < 4)) {
                    cost = 0;
                } else if (static_cast<s32>(cost) < static_cast<s32>(budget->get())) {
                    n->allowed = 1;
                    if (queued) ++count;
                }
            } else {
                bool alive = false;
                const u32 h = n->handle;
                if (global<u8>(kAudioReady) != 0) {
                    if (EmitterEntry* e = LiveEntry(h)) alive = call<u32>(Voice_IsPlaying, e->voice.get()) != 0;
                }
                if (!alive) {
                    if (n->quiet == 0) call(String_Format, kFmtStolen, kUnknown);
                    Detach(mgr, n);
                } else if (!(static_cast<s32>(cost) > static_cast<s32>(budget->get()))) {
                    if (n->queued != 0) ++count;
                    if (n->reacquire != 0) {
                        call(Handle_Acquire, frame.as<be_u32>(), n, 0u);
                        n->reacquire = global<u8>(kAudioDirty);
                    }
                } else {
                    if (n->queued != 0) call(String_Format, kFmtStopping, kUnknown);
                    if (def.direct == 0) {
                        call(Emitter_Stop, n->handle.get(), 0u);
                    } else if (global<u8>(kAudioReady) != 0) {
                        if (EmitterEntry* e = LiveEntry(n->handle)) {
                            if (e->owner->flag != 0) e->stopping = 1;
                            call(Voice_Stop, e->voice.get(), 1u);
                        }
                    }
                    Detach(mgr, n);
                }
            }
        }
        *budget = budget->get() - cost;
        n = next;
    } while (n != nullptr);
}

}  // namespace sly4::obj

NATIVE(004454B4, sly4::obj::PrioList_UpdateSounds4454B4)
