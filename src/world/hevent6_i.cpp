// World_HandleEvent callees, part 6: the audio emitter reset and the spawner
// manager reset (173 instructions each).
#include "hevent6.h"

using namespace sly4;
using namespace sly4::hevent;
using namespace sly4::hevent6;

namespace sly4 {
namespace hevent6i {
namespace ifn {
constexpr u32 Audio_Update445C94 = 0x00445C94;  // (0x010ACC40) (as hevent3_h.cpp's sub_00445C94)
constexpr u32 Music_SetTime4473C0 = 0x004473C0; // (; f32) (name?)
constexpr u32 PushStack_Changed = 0x00447B70;   // (; f32 time)
}  // namespace ifn

// Emitter pool (as hevent3_h.cpp / game/frame.h).
constexpr u32 kAudioUpdateObj = 0x010ACC40;
constexpr u32 kEmitters = 0x010B0200;
constexpr u32 kPoolFirst = 8, kPoolLast = 12, kPoolUsed = 16, kPoolNodes = 28;
constexpr u32 kNodeSize = 272, kNodeNext = 4, kNodeData = 16, kNodeSerial = 256;
constexpr u32 kKeepBank = 0x00F65B60;           // u32: emitters whose def +4 equals it are kept
constexpr u32 kVoices = 0x00F65B64;             // u32 first voice (+100 u8 playing, +104 next) (name?)
constexpr u32 kVoicesReset = 0x00F65B68;        // u8 (name?)
constexpr u32 kZero44A0B8 = 0x0044A0B8;         // f32 0.0
constexpr u32 kMusicPushStack = 0x010AEC60;     // u8: push stack active (name?)
constexpr u32 kMusicCurrent = 0x010AFE90;       // u32 (name?)
constexpr u32 kMixer = 0x010AF08C;              // f32 100.0, +4, +8, +12 cleared (name?)
constexpr u32 kMixerLevels = 0x010AF09C;        // f32[3], copy at +12 (0x010AF0A8)
constexpr u32 kMixerFlags = 0x010AF0B4;         // u32[3] cleared
constexpr u32 kMixerDefaults = 0x010AF0C0;      // {s32 level, u32}[3]
constexpr u32 kMixerDirty = 0x010AF1BC;         // u8
constexpr u32 kF100 = 0x42C80000;

// Spawner manager: +0 u8 busy, +6 u16 / +12 list of objects ({?, obj} 8
// bytes), +30 u16 / +36 groups ({?, group}), +52 handle pool (Pool layout as
// Pool_Remove784DE4: +52 / +56 / +60 / +64 / +68 / +72, +76 capacity, +80 entries).
constexpr u32 kAttrRespawn = 0x010AB1A8;        // u8 attribute (as hevent6_h.cpp)
constexpr u32 kChildLive = 0x40000;

inline u32 attr_index(u32 desc) { return load<u32>(load<u32>(desc + 16) + 64) << 2; }
}  // namespace hevent6i
}  // namespace sly4

using namespace sly4::hevent6i;

// void Emitters_Reset44A0C0() (name?): the 0x00449478 emitter stop (every
// pooled emitter whose def's +4 is not 0x00F65B60 is stopped), then every
// voice's +100 is cleared and the list (0x00F65B64) dropped, the music
// time set to 0 (0x004473C0, and PushStack_Changed while the push stack is
// active), and the mixer reset: levels from the defaults (as f32), flags
// cleared, 100.0 / 0 / 0 at 0x010AF08C, dirty flag set.
DECOMP(0044A0C0, Emitters_Reset44A0C0) {
    call(ctx, ifn::Audio_Update445C94, {kAudioUpdateObj});
    Stack frame(ctx, 16);  // r1+112 current handle, r1+116 first handle
    u32 h = 0;
    if (load<u32>(kEmitters + kPoolUsed) != 0) {
        const u32 first = load<u32>(kEmitters + kPoolFirst);
        const u32 node = load<u32>(kEmitters + kPoolNodes) + first * kNodeSize;
        store<u32>(frame.addr() + 4, 0);
        store<u16>(frame.addr() + 4, static_cast<u16>(first));
        store<u16>(frame.addr() + 6, static_cast<u16>(load<u32>(node + kNodeSerial) >> 15));
    } else {
        store<u32>(frame.addr() + 4, 0);
        store<u16>(frame.addr() + 6, 0);
    }
    h = load<u32>(frame.addr() + 4);
    store<u32>(frame.addr(), h);
    while ((h & 0xFFFF) != 0) {
        const u32 index = load<u16>(frame.addr());
        const u32 nodes = load<u32>(kEmitters + kPoolNodes);
        const u32 node = nodes + index * kNodeSize;
        u32 next = 0;
        if (static_cast<s32>(index) != load<s32>(kEmitters + kPoolLast)) {
            const u32 n = load<u32>(node + kNodeNext);
            next = (n << 16) | ((load<u32>(nodes + n * kNodeSize + kNodeSerial) >> 15) & 0xFFFF);
        }
        if (load<u32>(load<u32>(node + kNodeData + 4) + 4) != load<u32>(kKeepBank))
            call(ctx, hfn::Emitter_Stop, {h, 0});
        h = next;
        store<u32>(frame.addr(), h);
    }
    for (u32 v = load<u32>(kVoices); v != 0; v = load<u32>(v + 104)) store<u8>(v + 100, 0);
    store<u32>(kVoices, 0);
    store<u8>(kVoicesReset, 1);
    const f64 zero = load<f32>(kZero44A0B8);
    callf(ctx, ifn::Music_SetTime4473C0, {}, {zero});
    const u32 push = load<u8>(kMusicPushStack);
    store<u32>(kMusicCurrent, 0);
    if (push != 0) callf(ctx, ifn::PushStack_Changed, {}, {zero});
    store<u32>(kMixerLevels - 4, 0);  // 0x010AF098
    for (u32 k = 0; k < 3; ++k) {
        const f32 level = static_cast<f32>(load<s32>(kMixerDefaults + 8 * k));
        store<u32>(kMixerFlags + 4 * k, 0);
        store<f32>(kMixerLevels + 4 * k, level);
        store<f32>(kMixerLevels + 12 + 4 * k, level);
        ctx.fpr[1] = level;  // f1 as the original leaves it
    }
    store<u8>(kMixerDirty, 1);
    store<u32>(kMixer, kF100);
    store<u32>(kMixer + 4, 0);
    store<u32>(kMixer + 8, 0);
    ctx.gpr[3] = 0x010B0000;  // r3 as the original leaves it (lis)
}

// void Spawners_Reset4549D0(SpawnerMgr* m) (name?): busy while running;
// resets every object (vfunc +12), every group (+8 / +16 = 0, +28 = 1) and its
// members (respawning ones kill their live children: +32 -= 1, vfunc
// +40(1); then member vfunc +12), and rebuilds the handle pool's free list
// (entries chained k-1 / k+1, all free with serial 0; first / last -1, free
// head capacity - 1 or -1). r3 is left as the capacity.
DECOMP(004549D0, Spawners_Reset4549D0) {
    const u32 m = arg<u32>(ctx, 0);
    const s32 nobj = load<u16>(m + 6);
    store<u8>(m + 0, 1);
    for (s32 k = 0; k < nobj; ++k) {
        const u32 o = load<u32>(load<u32>(m + 12) + 8 * static_cast<u32>(k) + 4);
        vcall(ctx, o, 12, {o});
    }
    for (s32 i = 0; i < static_cast<s32>(load<u16>(m + 30)); ++i) {
        const u32 grp = load<u32>(load<u32>(m + 36) + 8 * static_cast<u32>(i) + 4);
        u32 mem = load<u32>(grp + 32);
        store<u32>(grp + 8, 0);
        store<u32>(grp + 16, 0);
        store<u8>(grp + 28, 1);
        for (; mem != 0; mem = load<u32>(mem + 72)) {
            if (load<u8>(load<u32>(load<u32>(mem + 48) + 12) + attr_index(kAttrRespawn)) != 0) {
                const s32 n = load<s32>(mem + 24);
                store<u32>(mem + 24, 0);
                for (s32 j = 0; j < n; ++j) {
                    const u32 c = load<u32>(load<u32>(mem + 20) + static_cast<u32>(j) * 4);
                    if ((load<u32>(c + 172) & kChildLive) == 0) continue;
                    store<u32>(mem + 32, load<u32>(mem + 32) - 1);
                    vcall(ctx, c, 40, {c, 1});
                }
            }
            vcall(ctx, mem, 12, {mem});
        }
    }
    s32 cap = load<s32>(m + 76);
    if (cap > 0) {
        for (s32 k = 0; k < cap; cap = load<s32>(m + 76)) {
            const u32 off = static_cast<u32>(k) * 16;
            const u32 ents = load<u32>(m + 80);
            store<u32>(ents + off, static_cast<u32>(k - 1));
            ++k;
            store<u32>(ents + off + 4, static_cast<u32>(k));
            store<u8>(ents + off + 12, static_cast<u8>(load<u8>(ents + off + 12) | 0x80));
            const u32 f = load<u32>(m + 80) + off + 12;
            store<u32>(f, load<u32>(f) & ~0x7FFF8000u);
        }
    }
    const u32 ents = load<u32>(m + 80);
    store<u32>(ents, 0xFFFFFFFFu);
    store<u32>(ents + (static_cast<u32>(cap) << 4) - 12, 0xFFFFFFFFu);
    store<u32>(m + 60, 0xFFFFFFFFu);
    store<u32>(m + 64, 0xFFFFFFFFu);
    store<u32>(m + 52, 0);
    store<u32>(m + 56, cap > 1 ? static_cast<u32>(cap - 1) : 0xFFFFFFFFu);
    store<u32>(m + 68, 0);
    store<u32>(m + 72, static_cast<u32>(cap));
    store<u8>(m + 0, 0);
    ctx.gpr[3] = static_cast<u32>(cap);  // r3 as the original leaves it (lwz)
}
