// World_HandleEvent callees, part 7c: the swap-remove of a 240-byte vector
// element and the sound's play time.
#include "hevent7.h"

using namespace sly4;
using namespace sly4::hevent;
using namespace sly4::hevent7;

namespace sly4 {
namespace hevent7c {
constexpr u32 kVecData = 0, kVecCount = 8;
constexpr u32 kElemSize = 240;
constexpr u32 kTimeZero = 0x00444258;  // f32 0.0
}  // namespace hevent7c
}  // namespace sly4

using namespace sly4::hevent7c;

// void Vector_RemoveAt74534C(Vector* v, s32 i) (name?): swap-remove for
// 240-byte elements: the count (+8, u16) drops by one and, unless i was the
// last, the last element is copied over element i field by field: bytes
// 0..127, the words at +128..+136, +144..+172, the top byte of +176 (bit by
// bit, keeping the rest of i's word), +180, +192..+224. +140, +184..+191
// and +228.. are not copied. r3 is the source element and f1 its +224
// after a copy.
DECOMP(0074534C, Vector_RemoveAt74534C) {
    const u32 v = arg<u32>(ctx, 0);
    const u32 i = arg<u32>(ctx, 1);
    const u32 last = (load<u16>(v + kVecCount) - 1) & 0xFFFF;
    store<u16>(v + kVecCount, static_cast<u16>(last));
    if (static_cast<s32>(i) == static_cast<s32>(last)) return;
    const u32 src = load<u32>(v + kVecData) + last * kElemSize;
    const u32 dst = load<u32>(v + kVecData) + i * kElemSize;
    for (u32 k = 0; k < 128; ++k) store<u8>(dst + k, load<u8>(src + k));
    auto word = [&](u32 off) { store<u32>(dst + off, load<u32>(src + off)); };
    word(128), word(132), word(136), word(144), word(148), word(152);
    word(160), word(164), word(156), word(168), word(172);
    // +176: bits 0..7 (IBM order) one at a time, each stored.
    u32 f = load<u32>(dst + 176);
    for (u32 b = 0; b < 8; ++b) {
        const u32 m = 0x80000000u >> b;
        f = (f & ~m) | (load<u32>(src + 176) & m);
        store<u32>(dst + 176, f);
    }
    word(180);
    word(192), word(200), word(196), word(204), word(212), word(208), word(216), word(220), word(224);
    ctx.fpr[1] = lfs_f64(src + 224);  // f1 as the original leaves it
    ret(ctx, src);
}

// f32 Sound_Time444260(Sound* s) (name?): 0 unless the sound plays (+97)
// and has started (every sub-voice reports a non-zero first value, or with
// no sub-voices the sample is not pending), and the push stack (0x010AEC60)
// is active. Then, for a live emitter handle (+4: index < capacity, serial
// non-zero and matching, node not free), the largest Sub_GetTime of its
// voice's sub-voices (at least 0).
DECOMP(00444260, Sound_Time444260) {
    const u32 s = arg<u32>(ctx, 0);
    auto zero = [&] {
        ctx.gpr[3] = kTimeZero;  // r3 as the original leaves it
        ret(ctx, load<f32>(kTimeZero));
    };
    if (load<u8>(s + kSndPlaying) == 0) return zero();
    const u32 slot = load<u32>(kEmitters + kPoolNodes) + load<u16>(s + kSndEmitter) * kNodeSize + kNodeVoice;
    const u32 n = static_cast<u32>(call(ctx, h7fn::Voice_GetValue, {load<u32>(slot)}));
    if (n != 0) {
        Stack frame(ctx, 16);
        for (u32 i = 0; i < n; ++i) {
            const u64 sub = call(ctx, h7fn::Voice_GetSub, {load<u32>(slot), static_cast<u64>(static_cast<s64>(static_cast<s16>(i)))});
            call(ctx, h7fn::Sub_GetInfo, {sub, frame.addr() + 4, 0, 0, 0});
            if (load<u32>(frame.addr() + 4) == 0) return zero();
        }
    } else if (const u32 sample = load<u32>(s + kSndSample); sample != 0) {
        if (const u32 data = load<u32>(sample + kSampleData); data != 0 && load<s32>(data + kDataPending) > 0) return zero();
    }
    const u32 handle = load<u32>(s + kSndEmitter);
    if (load<u8>(kMusicPushStack) == 0) return zero();
    const u32 serial = handle & 0xFFFF, idx = handle >> 16;
    if (serial == 0 || !(static_cast<s32>(idx) < load<s32>(kEmitters + kPoolCapacity))) return zero();
    const u32 node = load<u32>(kEmitters + kPoolNodes) + idx * kNodeSize;
    if (((load<u32>(node + kNodeSerial) >> 15) & 0xFFFF) != serial || (load<u8>(node + kNodeSerial) & 0x80) != 0) return zero();
    const u32 voice = node + kNodeVoice;
    const u32 m = static_cast<u32>(call(ctx, h7fn::Voice_GetValue, {load<u32>(voice)}));
    f32 best = load<f32>(kTimeZero);
    if (m == 0) {
        ctx.gpr[3] = kTimeZero;
        ret(ctx, best);
        return;
    }
    Stack frame(ctx, 16);
    for (u32 i = 0; i < m; ++i) {
        const u64 sub = call(ctx, h7fn::Voice_GetSub, {load<u32>(voice), static_cast<u64>(static_cast<s64>(static_cast<s16>(i)))});
        call(ctx, h7fn::Sub_GetTime6F9A18, {sub, frame.addr(), 0});
        const f32 t = load<f32>(frame.addr());
        if (t > best) best = t;
    }
    ret(ctx, best);  // r3 is Sub_GetTime6F9A18's
}

namespace sly4 {
namespace hevent7c {
namespace mfn {
constexpr u32 Layer_Start6D7468 = 0x006D7468;   // (voice +28, s32 layer) (name?)
constexpr u32 Stream_SetVolume6D71B0 = 0x006D71B0;  // (stream, -, -, 0, 360, 0, 0; f32 volume, f32 1.0) (name?)
}  // namespace mfn
constexpr u32 kMusicMuted = 0x010B0A30;     // u8 (0x010B09E8 +72) (name?)
constexpr u32 kSingleLayer = 0x010B0BC0;    // u8 (0x010B0B78 +72): one layer per call, round robin (name?)
constexpr u32 kVoices = 0x00F65B64;         // first voice (+96 f32 weight, +104 next) (as hevent6_i.cpp)
constexpr u32 kVoicesReset = 0x00F65B68;    // u8: pick the layers again (as hevent6_i.cpp)
constexpr u32 kVoiceWeight = 96, kVoiceNext = 104, kVoiceLayer = 28;
constexpr u32 kLayers = 3;
constexpr u32 kLayerPlaying = 0x010AFB40;   // Voice*[3] started on each layer
constexpr u32 kLayerVoice = 0x010AFB4C;     // Voice*[3] chosen for each layer
constexpr u32 kLayerVolume = 0x010AFB58;    // f32[3] last volume sent
constexpr u32 kLayerFade = 0x010AFB64;      // f32[3] fade-in 0..1
constexpr u32 kLayerDelay = 0x010AFB70;     // f32[3] time before the fade starts
constexpr u32 kLayerNext = 0x010AFB7C;      // s32 next layer in single-layer mode
constexpr u32 kLayerStream = 0x010AFE94;    // u32[3] stream handles
constexpr u32 kZero4473B4 = 0x004473B4;     // f32 0.0
constexpr u32 kOne4473B8 = 0x004473B8;      // f32 1.0
constexpr u32 kEps4473BC = 0x004473BC;      // f32 1e-5
constexpr u32 kOneBits = 0x3F800000;
constexpr u32 kVolumeArg = 360;
}  // namespace hevent7c
}  // namespace sly4

// void Music_SetTime4473C0(; f32 dt) (name?): the music layer update.
// Unless muted: when asked (0x00F65B68), the three heaviest voices (+96)
// are picked and each layer whose voice is no longer among them takes the
// heaviest picked voice not yet on a layer (or none). Then each used layer
// counts its delay down by dt, or once it is over fades in by dt (to 1.0).
// Last, one layer (round robin, single-layer mode) or all three: a layer
// whose voice changed starts it (Layer_Start6D7468, fade 0); the volume
// weight * fade (0 without a voice) is sent to the layer's stream when it
// moved by 1e-5 or more.
DECOMP(004473C0, Music_SetTime4473C0) {
    const f64 dt = farg(ctx, 0);
    if (const u32 muted = load<u8>(kMusicMuted); muted != 0) {
        ctx.gpr[3] = muted;  // r3 as the original leaves it
        return;
    }
    auto layer = [](u32 base, u32 j) { return base + 4 * j; };
    if (load<u8>(kVoicesReset) != 0) {
        f32 weight[kLayers] = {0, 0, 0};
        u32 pick[kLayers] = {0, 0, 0};  // ascending weight
        for (u32 v = load<u32>(kVoices); v != 0; v = load<u32>(v + kVoiceNext)) {
            const f32 w = load<f32>(v + kVoiceWeight);
            if (!(w > weight[0])) continue;
            u32 i = 1;
            for (; i < kLayers && w >= weight[i]; ++i) {
                weight[i - 1] = weight[i];
                pick[i - 1] = pick[i];
            }
            weight[i - 1] = w;
            pick[i - 1] = v;
        }
        for (u32 j = 0; j < kLayers; ++j) {
            const u32 cur = load<u32>(layer(kLayerVoice, j));
            if (cur != 0 && (cur == pick[0] || cur == pick[1] || cur == pick[2])) continue;
            u32 take = 0;
            for (s32 k = kLayers - 1; k >= 0; --k) {
                const u32 c = pick[k];
                if (c == 0) break;
                bool used = false;
                for (u32 m = 0; m < kLayers && !used; ++m) used = load<u32>(layer(kLayerVoice, m)) == c;
                if (!used) {
                    take = c;
                    break;
                }
            }
            store<u32>(layer(kLayerVoice, j), take);
        }
    }
    const f32 zero = load<f32>(kZero4473B4), one = load<f32>(kOne4473B8);
    for (u32 j = 0; j < kLayers; ++j) {
        if (load<u32>(layer(kLayerVoice, j)) == 0) continue;
        const f32 delay = load<f32>(layer(kLayerDelay, j));
        if (delay > zero) {
            store<f32>(layer(kLayerDelay, j), static_cast<f32>(static_cast<f64>(delay) - dt));
            continue;
        }
        f32 fade = load<f32>(layer(kLayerFade, j));
        if (fade < one) {
            fade = static_cast<f32>(static_cast<f64>(fade) + dt);
            store<f32>(layer(kLayerFade, j), fade);
            if (fade > one) store<u32>(layer(kLayerFade, j), kOneBits);
        }
    }
    const f32 eps = load<f32>(kEps4473BC);
    // One layer: start its voice when it changed, then send the volume.
    // r3 is tracked for what the original leaves in it.
    u64 r3 = kZero4473B4;
    auto update = [&](u32 j, bool single) {
        f32 vol = zero;
        if (const u32 cur = load<u32>(layer(kLayerVoice, j)); cur != 0) {
            r3 = load<u32>(layer(kLayerPlaying, j));
            if (r3 != cur) {
                const u64 arg_j = single ? static_cast<u64>(static_cast<s64>(static_cast<s32>(j))) : j;
                r3 = call(ctx, mfn::Layer_Start6D7468, {cur + kVoiceLayer, arg_j});
                store<u32>(layer(kLayerFade, j), 0);
                store<u32>(layer(kLayerPlaying, j), cur);
            }
            vol = fmuls(load<f32>(cur + kVoiceWeight), load<f32>(layer(kLayerFade, j)));
        }
        const f32 diff = std::fabs(static_cast<f32>(static_cast<f64>(load<f32>(layer(kLayerVolume, j))) - vol));
        if (diff < eps) {
            ctx.fpr[1] = diff;  // f1 as the original leaves it
        } else {
            // r4/r5 are not read by the callee (left over in the original).
            callf(ctx, mfn::Stream_SetVolume6D71B0,
                  {load<u32>(layer(kLayerStream, j)), ctx.gpr[4], ctx.gpr[5], 0, kVolumeArg, 0, 0}, {vol, one});
            r3 = ctx.gpr[3];
            store<f32>(layer(kLayerVolume, j), vol);
        }
    };
    if (load<u8>(kSingleLayer) != 0) {
        update(static_cast<u32>(load<s32>(kLayerNext)), true);
        const s32 next = load<s32>(kLayerNext) + 1;
        store<s32>(kLayerNext, next);
        if (next >= static_cast<s32>(kLayers)) store<s32>(kLayerNext, 0);
        ctx.gpr[3] = 0x010B0000u;  // r3 as the original leaves it (lis of 0x010AFB7C)
    } else {
        for (u32 j = 0; j < kLayers; ++j) update(j, false);
        ctx.gpr[3] = r3;
    }
}
