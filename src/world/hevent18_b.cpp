// World_HandleEvent callees, part 18b: scene resume, speaker set loading,
// sound entry stop and the effect-by-name helper.
#include "hevent18.h"

using namespace sly4;
using namespace sly4::hevent;
using namespace sly4::hevent18;

namespace {
// Scene: +0 s32 count, +592 Item*[] (vt +124 query, +48 Object*).
constexpr u32 kSceneCount = 0, kSceneItems = 592, kItemVtQuery = 124, kItemObject = 48;
constexpr u32 kObjSceneEnd0DD02C = 0x000DD02C;  // (Object*, u8 drop, u8 keep) (hevent16_a.cpp)

// Layer list: +0 u32 count, +4 20-byte records {+0 u32 a, +4 s32 kind, +12
// u32 b, +16 u8 c}.
constexpr u32 kListCount = 0, kListData = 4, kRecSize = 20, kRecA = 0, kRecKind = 4, kRecB = 12, kRecC = 16;
constexpr u32 kKindFirst = 0xFF45EF93u, kKindAdd = 0x19A18BC2u;
constexpr u32 kSpeakerFirst35133C = 0x0035133C;  // (Speaker*, u32) (name?)
constexpr u32 kSpeakerAdd351898 = 0x00351898;    // (Speaker*, u32 a, u32 b, u8 c) (name?)

// Sound entry: +24 Bank* (+16 u32 voices[], +20 s32 count), +32 u32 voice.
// Voice: +112 u32, +116 f32 position, +120 s32 state.
constexpr u32 kEntryBank = 24, kEntryVoice = 32, kBankVoices = 16, kBankCount = 20;
constexpr u32 kVoiceMark = 112, kVoicePos = 116, kVoiceState = 120;
constexpr u32 kVoiceMarkValue = 0x7F7FFFEEu;
constexpr u32 kOneF353CBC = 0x00353CBC;  // f32 1.0
constexpr u32 kBankRelease_A459B8 = 0x00A459B8;  // (Bank*, voice) (name?)

constexpr u32 kObjCreateEffectDCA20 = 0x000DCA20;  // (Object*, def, u32, u8) -> Effect* (name?)
}  // namespace

// void Scene_Resume1B2F4(Scene* s, u8 drop) (name?): ends the scene for
// the object of every item whose vt +124 query returns 0.
DECOMP(0001B2F4, Scene_Resume1B2F4) {
    const u32 s = arg<u32>(ctx, 0), drop = arg<u8>(ctx, 1);
    ctx.gpr[3] = load<u32>(s + kSceneCount);  // r3 left as the original leaves it (the count)
    if (load<s32>(s + kSceneCount) <= 0) return;
    s32 i = 0;
    do {
        const u32 slot = s + kSceneItems + 4 * static_cast<u32>(i);
        const u32 item = load<u32>(slot);
        if (vcall(ctx, item, kItemVtQuery, {item}) == 0)
            call(ctx, kObjSceneEnd0DD02C, {load<u32>(load<u32>(slot) + kItemObject), drop, 0});
        ++i;
        ctx.gpr[3] = load<u32>(s + kSceneCount);
    } while (i < load<s32>(s + kSceneCount));
}

// void Set_LoadLayers351A40(Speaker* s, const List* l) (name?): runs the
// list's records: kind 0xFF45EF93 -> 0x0035133C (s, a); kind 0x19A18BC2 ->
// 0x00351898 (s, a, b, c); others are skipped.
DECOMP(00351A40, Set_LoadLayers351A40) {
    const u32 s = arg<u32>(ctx, 0), l = arg<u32>(ctx, 1);
    if (l == 0) return;
    for (u32 i = 0; i < load<u32>(l + kListCount); ++i) {
        const u32 rec = load<u32>(l + kListData) + kRecSize * i;
        const u32 kind = load<u32>(rec + kRecKind);
        if (kind == kKindFirst) {
            call(ctx, kSpeakerFirst35133C, {s, load<u32>(rec + kRecA)});
        } else if (kind == kKindAdd) {
            const u32 c = load<u8>(rec + kRecC);
            call(ctx, kSpeakerAdd351898, {s, load<u32>(rec + kRecA), load<u32>(rec + kRecB), c});
        }
    }
    ctx.gpr[3] = load<u32>(l + kListCount);  // r3 left as the original leaves it
}

// void Entry_Stop353CC0(Entry* e, u8 release) (name?): finds the entry's
// voice in its bank. release: clears the entry's voice and releases it
// (0x00A459B8); otherwise rewinds it (state 0 / 1: position = 1 - position)
// and sets state 2 (3 when the position is 1).
DECOMP(00353CC0, Entry_Stop353CC0) {
    const u32 e = arg<u32>(ctx, 0);
    const u32 bank = load<u32>(e + kEntryBank);
    if (bank == 0) return;
    const u32 key = load<u32>(e + kEntryVoice);
    if (key == 0) return;
    const s32 n = load<s32>(bank + kBankCount);
    u32 v = 0;
    s32 i = 0;
    for (; i < n; ++i) {
        v = load<u32>(load<u32>(bank + kBankVoices) + 4 * static_cast<u32>(i));
        if (v == key) break;
    }
    if (i >= n) return;
    if (arg<u8>(ctx, 1) != 0) {
        store<u32>(e + kEntryVoice, 0);
        call(ctx, kBankRelease_A459B8, {bank, v});
        return;
    }
    const s32 state = load<s32>(v + kVoiceState);
    if (state == 1 || state == 0) {
        const f32 p = static_cast<f32>(hevent7::lfs_f64(kOneF353CBC) - hevent7::lfs_f64(v + kVoicePos));
        store<f32>(v + kVoicePos, p);
    }
    store<u32>(v + kVoiceMark, kVoiceMarkValue);
    ctx.fpr[1] = hevent7::lfs_f64(v + kVoicePos);  // f1 / f2 left as the original leaves them
    ctx.fpr[2] = hevent7::lfs_f64(kOneF353CBC);
    const u32 st = ctx.fpr[1] == ctx.fpr[2] ? 3u : 2u;
    store<u32>(v + kVoiceState, st);
    ctx.gpr[3] = st;  // r3 left as the original leaves it
}

// Effect* Obj_EffectDCD20(Object* o, u32 hash, u32 a, u8 b) (name?): looks
// hash up in the global effect set (Set_Find(0)) and creates that effect
// on o (0x000DCA20 (o, def, a, b)). Returns 0 without a def.
DECOMP(000DCD20, Obj_EffectDCD20) {
    const u32 o = arg<u32>(ctx, 0), hash = arg<u32>(ctx, 1), a = arg<u32>(ctx, 2), b = arg<u8>(ctx, 3);
    if (hash == 0) {
        ret(ctx, 0u);
        return;
    }
    const u32 set = static_cast<u32>(call(ctx, hevent17::h17fn::Set_Find, {0}));
    if (set == 0) {
        ret(ctx, 0u);
        return;
    }
    const u32 def = hevent6::set_lookup(set, hash);
    if (def == 0) {
        ret(ctx, 0u);
        return;
    }
    ret(ctx, call(ctx, kObjCreateEffectDCA20, {o, def, a, b}));
}
