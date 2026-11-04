// World_HandleEvent callees, part 5: 165..168 instructions.
#include "hevent.h"

using namespace sly4;
using namespace sly4::hevent;

namespace sly4 {
namespace hevent5f {
namespace ffn {
constexpr u32 Map_Get = 0x0075EA74;             // (map, const u32* key) -> value slot
constexpr u32 Subtitle_Tick4A8B08 = 0x004A8B08; // (Subtitle*; f32 dt) (name?)
constexpr u32 Audio_Release446384 = 0x00446384; // (0x010ACC40, handle) (name?)
constexpr u32 SortedMap_Remove = 0x00771DD0;    // (map, const u32* key)
constexpr u32 Mem_FreeFrom = 0x0054EE6C;        // (p, pool)
constexpr u32 Objects_Reset35B470 = 0x0035B470; // (0x0106C330) (name?)
}  // namespace ffn

// The subtitle line at 0x010C1B88: +128 speaker key, +132 Object* (vfunc +88(o,
// 1) when shown), +136 text element, +140 f32 time left, +144 audio handle.
constexpr u32 kSubtitle = 0x010C1B88;
constexpr u32 kSubtitleActive = 0x010C1C1C;   // u8
constexpr u32 kSubtitlePending = 0x010C1C1D;  // u8: show on the next tick
constexpr u32 kSubtitleColor = 0x01164CC4;    // u8[4] default colour (name?)
constexpr u32 kSpeakerMap = 0x0106C2A8;       // map key -> speaker (+20 u8[4] colour) (name?)
constexpr u32 kPerChar4A8DE4 = 0x004A8DE4;    // f32 0.08: seconds per character
constexpr u32 kZero4A8DE8 = 0x004A8DE8;       // f32 0.0
constexpr u32 kTuneSubtitleBase = 0x010C1D04 + 72;  // f32 extra seconds (name?)
constexpr u32 kTextFade = 0x3DA3D70Au;        // f32 0.08 stored at the text's +224 (name?)
constexpr u32 kEmptyString = 0x00AEB4C4;      // ""
constexpr u32 kAudioUpdateObj = 0x010ACC40;

// Sorted maps {+2 u16 count, +8 {key, value}[] of 8 bytes} (name?)
constexpr u32 kObjMapA = 0x0106C240, kHandleMapB = 0x0106C228, kObjMapC = 0x0106C210;
constexpr u32 kVectors = 0x0106C2D4;          // four vectors of 20 bytes, u16 count at +8 (name?)
constexpr u32 kMapD = 0x0106C258;             // map, count cleared (name?)
constexpr u32 kWordsF64CB8 = 0x00F64CB8;      // u32 [3] cleared (name?)
constexpr u32 kVecE = 0x0106C280, kVecF = 0x0106C294;  // vectors, u16 count at +8 (name?)
constexpr u32 kObjState = 0x0106C330;         // +100, +104 cleared, then Objects_Reset35B470 (name?)
}  // namespace hevent5f
}  // namespace sly4

using namespace sly4::hevent5f;

// void Subtitle_Update4A8DF0(f32 dt) (name?): a pending line is shown: the
// speaker object gets vfunc +88(o, 1); the text element takes the speaker's
// colour (map 0x0106C2A8, else the default) by vfunc +56, the line by vfunc
// +188(el, 0x010C1B88, 0), +224 = 0.08, and the time left = chars (+212) *
// 0.08 + tuning; the line becomes active. An active line counts down and, at
// 0 or below, is cleared (vfunc +188(el, "", 0), +136 = 0, the +144 audio
// released). Then Subtitle_Tick4A8B08(line; dt).
DECOMP(004A8DF0, Subtitle_Update4A8DF0) {
    const f64 dt = ctx.fpr[1];
    const u32 s = kSubtitle;
    if (load<u8>(kSubtitlePending) != 0) {
        const u32 o = load<u32>(s + 132);
        if (o != 0) {
            const u32 opd = load<u32>(load<u32>(o) + 88);
            ctx.gpr[5] = opd;
            ctx.gpr[6] = load<u32>(opd);
            call_ptr(ctx, opd, {o, 1});
            Stack frame(ctx, 16);  // the key at r1+112
            store<u32>(frame.addr(), load<u32>(s + 128));
            u32 col = load<u32>(kSubtitleColor);
            const u32 slot = static_cast<u32>(call(ctx, ffn::Map_Get, {kSpeakerMap, frame.addr()}));
            const u32 sp = load<u32>(slot);
            if (sp != 0) col = load<u32>(sp + 20);
            const u32 t = load<u32>(s + 136);
            if (t != 0) {
                const u32 copd = load<u32>(load<u32>(t) + 56);
                ctx.gpr[5] = load<u32>(copd);  // the entry; r4 carries the colour
                call_ptr(ctx, copd, {t, col});
                const u32 t2 = load<u32>(s + 136);
                vcall(ctx, t2, 188, {t2, s, 0});
                const u32 t3 = load<u32>(s + 136);
                const f32 per = load<f32>(kPerChar4A8DE4);
                const f32 n = static_cast<f32>(static_cast<f64>(load<u32>(t3 + 212)));
                store<u32>(t3 + 224, kTextFade);
                const f32 base = load<f32>(kTuneSubtitleBase);
                store<f32>(s + 140, static_cast<f32>(std::fma(static_cast<f64>(n), static_cast<f64>(per), static_cast<f64>(base))));
            }
        }
        store<u8>(kSubtitleActive, 1);
        store<u8>(kSubtitlePending, 0);
    } else if (load<u8>(kSubtitleActive) != 0) {
        const f32 left = static_cast<f32>(static_cast<f64>(load<f32>(s + 140)) - dt);
        store<f32>(s + 140, left);
        if (left <= load<f32>(kZero4A8DE8)) {
            store<u8>(kSubtitleActive, 0);
            const u32 t = load<u32>(s + 136);
            if (t != 0) vcall(ctx, t, 188, {t, kEmptyString, 0});
            const u32 a = load<u32>(s + 144);
            store<u32>(s + 136, 0);
            if (a != 0) {
                call(ctx, ffn::Audio_Release446384, {kAudioUpdateObj, a});
                store<u32>(s + 144, 0);
            }
        }
    }
    callf(ctx, ffn::Subtitle_Tick4A8B08, {s}, {dt});
}

// void Objects_ClearAll35B668() (name?): deletes (vfunc +4) the objects of
// map 0x0106C240, removing each one's handle (+12, else (+8) +68, else 0) from
// 0x0106C228 first (the object is read before its null check, as in the
// original); deletes the objects of 0x0106C228 and 0x0106C210; clears those
// maps, six vectors / maps and 0x00F64CB8..C0; frees (pool 0) the values of
// 0x0106C2A8 and clears it; 0x0106C330 +100 / +104 = 0 and
// Objects_Reset35B470(0x0106C330).
DECOMP(0035B668, Objects_ClearAll35B668) {
    for (s32 i = 0; i < static_cast<s32>(load<u16>(kObjMapA + 2)); ++i) {
        const u32 o = load<u32>(load<u32>(kObjMapA + 8) + static_cast<u32>(i) * 8 + 4);
        u32 key = load<u32>(o + 12);
        if (key == 0) {
            const u32 d = load<u32>(o + 8);
            key = d != 0 ? load<u32>(d + 68) : 0;
        }
        Stack frame(ctx, 16);  // the key at r1+112
        store<u32>(frame.addr(), key);
        call(ctx, ffn::SortedMap_Remove, {kHandleMapB, frame.addr(), key});
        if (o != 0) vcall(ctx, o, 4, {o});
    }
    store<u16>(kObjMapA + 2, 0);
    for (u32 m : {kHandleMapB, kObjMapC}) {
        for (s32 i = 0; i < static_cast<s32>(load<u16>(m + 2)); ++i) {
            const u32 o = load<u32>(load<u32>(m + 8) + static_cast<u32>(i) * 8 + 4);
            if (o != 0) vcall(ctx, o, 4, {o});
        }
        if (m == kHandleMapB) store<u16>(m + 2, 0);
    }
    store<u16>(kVectors + 8, 0);
    store<u16>(kVectors + 20 + 8, 0);
    store<u16>(kVectors + 40 + 8, 0);
    store<u16>(kVectors + 60 + 8, 0);
    store<u16>(kObjMapC + 2, 0);
    store<u16>(kMapD + 2, 0);
    store<u32>(kWordsF64CB8 + 0, 0);
    store<u32>(kWordsF64CB8 + 4, 0);
    store<u32>(kWordsF64CB8 + 8, 0);
    store<u16>(kVecE + 8, 0);
    store<u16>(kVecF + 8, 0);
    for (s32 i = 0; i < static_cast<s32>(load<u16>(kSpeakerMap + 2)); ++i)
        call(ctx, ffn::Mem_FreeFrom, {load<u32>(load<u32>(kSpeakerMap + 8) + static_cast<u32>(i) * 8 + 4), 0});
    store<u16>(kSpeakerMap + 2, 0);
    store<u32>(kObjState + 100, 0);
    store<u32>(kObjState + 104, 0);
    call(ctx, ffn::Objects_Reset35B470, {kObjState});
}
