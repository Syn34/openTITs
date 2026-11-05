// World_HandleEvent callees, part 27a: sound system shutdown
// (Sound_Shutdown4495D8 and the two helpers it runs).
#include "hevent27.h"

using namespace sly4;
using namespace sly4::hevent;
using namespace sly4::hevent27;

namespace {
constexpr u32 kAudioReady = 0x010AEC60;   // u8 (as hevent3_d.cpp)
constexpr u32 kSoundMgr = 0x010ACC40;     // SoundMgr (as hevent20_c.cpp)
constexpr u32 kEmitters = 0x010B0200;     // emitter pool (as hevent3_d.cpp)
constexpr u32 kPoolFirst = 8, kPoolLast = 12, kPoolUsed = 16, kPoolNodes = 28;
constexpr u32 kNodeSize = 272, kNodeNext = 4, kNodeSound = 20, kNodeSerial = 256;
constexpr u32 kSoundOwnerKey = 0x00F65B60;  // s32: the owner kind whose sounds are kept (Emitters_StopForeign)
constexpr u32 kVoices = 0x00F65B64;         // first voice (+100 u8, +104 next) (as hevent7_c.cpp)
constexpr u32 kVoicesDirty = 0x00F65B68;    // u8
constexpr u32 kAudioKeyMap = 0x010AFEF8;    // sorted map, u16 count at +2 (as hevent3_d.cpp)
constexpr u32 kPushStackTop = 0x010AFE90;   // u32 (name?)
constexpr u32 kBanks = 0x010AFEB0;          // {key, Bank*} array: u16 +2 count, +8 data
// Three channel slots at 0x010AF08C..: f32 +0 = 100.0f, +4 / +8 = 0;
// +12 / +24 f32[3] = the s32 values at +52 + 8k as floats, +40 u32[3] = 0;
// +12 u32 at 0x010AF098 = 0. u8 0x010AF1BC = 1.
constexpr u32 kChan = 0x010AF08C, kChanCount = 0x010AF098, kChanA = 0x010AF09C, kChanB = 0x010AF0A8;
constexpr u32 kChanZero = 0x010AF0B4, kChanSrc = 0x010AF0C0, kChanReset = 0x010AF1BC;
constexpr u32 kCbPool = 0x00F65B58;         // u32 24-byte Pool* (as hevent20_k.cpp)
constexpr u32 kCbBuf = 0x00F65B5C;          // u32 its buffer
constexpr u32 kSmallPoolOn = 0x00F65B54;    // u32: banks' +4 buffers come from pools
constexpr u32 kSmallPool = 0x00F65B58;      // MemPool* {+0 base, +4 size} tried first
constexpr u32 kSmallPoolFallback = 0x00F65B50;

// Emitters_StopForeign-style sweep of one SoundMgr list (+8200 or +8208):
// every sound whose owner's kind (sound +0 -> +0 -> +4) is not the kept
// one is stopped (Emitter_Stop(+4, false)), unlinked (+116 prev, +120
// next; +96 picks the list head +8200 / +8208 and tail +8204), its +97 /
// +99 cleared, then moved to the free list (+8196) when +108 is set or
// freed (Sound_Free445390). Returns the r3 the loop leaves.
u64 sweep(PPUContext& ctx, u32 mgr, u32 head_off, s32 key, u64 r3) {
    u32 s = load<u32>(mgr + head_off);
    while (s != 0) {
        const u32 h = load<u32>(s + 0);
        const u32 owner = h != 0 ? load<u32>(h) : 0;
        const u32 kind = load<u32>(owner + 4);  // reads address 4 for a null handle (as the original)
        const u32 next = load<u32>(s + 120);
        r3 = kind;
        if (static_cast<s32>(kind) != key) {
            call(ctx, h27fn::Emitter_Stop, {load<u32>(s + 4), 0});
            const u32 prev = load<u32>(s + 116);
            if (prev != 0) {
                store<u32>(prev + 120, load<u32>(s + 120));
            } else {
                store<u32>(mgr + (load<u8>(s + 96) != 0 ? 8200u : 8208u), load<u32>(s + 120));
            }
            const u32 nx = load<u32>(s + 120);
            if (nx != 0) {
                store<u32>(nx + 116, load<u32>(s + 116));
            } else if (load<u8>(s + 96) != 0) {
                store<u32>(mgr + 8204, load<u32>(s + 116));
            }
            const u32 keep = load<u32>(s + 108);
            store<u8>(s + 97, 0);
            store<u8>(s + 99, 0);
            if (keep != 0) {
                store<u32>(s + 120, load<u32>(mgr + 8196));
                const u32 f = load<u32>(mgr + 8196);
                if (f != 0) store<u32>(f + 116, s);
                store<u32>(s + 116, 0);
                store<u32>(mgr + 8196, s);
                r3 = 0;
            } else {
                r3 = call(ctx, h27fn::Sound_Free, {mgr, s});
            }
        }
        s = next;
    }
    return r3;
}
}  // namespace

// ---------------------------------------------------------------- 0x00449244
// void Bank_Destroy449244(Bank* b) (name?): frees the +16 table's {key,
// p} entries (p when set) and the +40 table's (unchecked), the 0x006D59C4
// release of +0 (b +0, 1), the +4 buffer (pool 0x00F65B58 when it holds it,
// else 0x00F65B50; only while 0x00F65B54 is set), the arrays at +32 / +8
// and finally b itself. Counts +10 / +34 are cleared.
DECOMP(00449244, Bank_Destroy449244) {
    const u32 b = arg<u32>(ctx, 0);
    if (b == 0) return;
    for (u32 i = 0; i < load<u16>(b + 10); ++i) {
        const u32 p = load<u32>(load<u32>(b + 16) + 8 * i + 4);
        if (p != 0) call(ctx, h27fn::Mem_FreeFrom, {p, 0});
    }
    store<u16>(b + 10, 0);
    for (u32 i = 0; i < load<u16>(b + 34); ++i) call(ctx, h27fn::Mem_FreeFrom, {load<u32>(load<u32>(b + 40) + 8 * i + 4), 0});
    const u32 h = load<u32>(b + 0);
    store<u16>(b + 34, 0);
    call(ctx, 0x006D59C4, {h, 1});
    const u32 buf = load<u32>(b + 4);
    if (buf != 0 && load<u32>(kSmallPoolOn) != 0) {
        const u32 pool = load<u32>(kSmallPool);
        if (pool != 0 && !(buf < load<u32>(pool)) && buf < load<u32>(pool) + load<u32>(pool + 4)) {
            call(ctx, h27fn::Pool_Free, {pool, buf});
        } else {
            call(ctx, h27fn::Pool_Free, {load<u32>(kSmallPoolFallback), buf});
        }
    }
    store<u32>(b + 4, 0);
    array_release(ctx, b + 32);
    array_release(ctx, b + 8);
    call(ctx, h27fn::Mem_FreeFrom, {b, 0});
}

// ---------------------------------------------------------------- 0x00445C94
// void SoundMgr_StopForeign445C94(SoundMgr* m) (name?): runs the
// owner-kind sweep over the +8200 list, then the +8208 list (sounds of
// the kind at 0x00F65B60 are kept).
DECOMP(00445C94, SoundMgr_StopForeign445C94) {
    const u32 m = arg<u32>(ctx, 0);
    const s32 key = static_cast<s32>(load<u32>(kSoundOwnerKey));
    u64 r3 = m;
    r3 = sweep(ctx, m, 8200, key, r3);
    r3 = sweep(ctx, m, 8208, key, r3);
    ctx.gpr[3] = r3;
}

// ---------------------------------------------------------------- 0x004495D8
// void Sound_Shutdown4495D8() (name?): while audio is ready (0x010AEC60):
// sweeps the SoundMgr, stops every pooled emitter whose sound's owner kind
// is not the kept one (handles idx << 16 | serial; a zero serial ends the
// walk), clears the voices' +100 and the key map, resets the music time and
// push stack to 0.0f, destroys every bank (Bank_Destroy449244), resets the
// three channel slots and frees the timed callback pool and buffer.
DECOMP(004495D8, Sound_Shutdown4495D8) {
    if (load<u8>(kAudioReady) == 0) return ret(ctx, 0u);
    call(ctx, 0x00445C94, {kSoundMgr});
    const u32 P = kEmitters;
    if (load<u32>(P + kPoolUsed) != 0) {
        const u32 first = load<u32>(P + kPoolFirst);
        u32 it = ((first & 0xFFFF) << 16) | ((load<u32>(load<u32>(P + kPoolNodes) + first * kNodeSize + kNodeSerial) >> 15) & 0xFFFF);
        while ((it & 0xFFFF) != 0) {
            const u32 idx = it >> 16;
            const u32 nodes = load<u32>(P + kPoolNodes);
            const u32 node = nodes + idx * kNodeSize;
            u32 nit = 0;
            if (idx != load<u32>(P + kPoolLast)) {
                const u32 n = load<u32>(node + kNodeNext);
                nit = (n << 16) | ((load<u32>(nodes + n * kNodeSize + kNodeSerial) >> 15) & 0xFFFF);
            }
            if (load<u32>(load<u32>(node + kNodeSound) + 4) != load<u32>(kSoundOwnerKey)) call(ctx, h27fn::Emitter_Stop, {it, 0});
            it = nit;
        }
    }
    const u32 v0 = load<u32>(kVoices);
    store<u16>(kAudioKeyMap + 2, 0);
    for (u32 v = v0; v != 0; v = load<u32>(v + 104)) store<u8>(v + 100, 0);
    store<u32>(kVoices, 0);
    store<u8>(kVoicesDirty, 1);
    const f32 zero = 0.0f;  // lfs from 0x004495D0
    callf(ctx, h27fn::Music_SetTime, {}, {zero});
    const u8 ready = load<u8>(kAudioReady);
    store<u32>(kPushStackTop, 0);
    if (ready != 0) callf(ctx, h27fn::PushStack_Changed, {}, {zero});
    for (u32 i = 0; i < load<u16>(kBanks + 2); ++i) call(ctx, 0x00449244, {load<u32>(load<u32>(kBanks + 8) + 8 * i + 4)});
    store<u16>(kBanks + 2, 0);
    store<u32>(kChanCount, 0);
    for (u32 k = 0; k < 3; ++k) {
        const s32 v = static_cast<s32>(load<u32>(kChanSrc + 8 * k));
        const f32 f = static_cast<f32>(static_cast<f64>(v));
        store<u32>(kChanZero + 4 * k, 0);
        store<f32>(kChanA + 4 * k, f);
        store<f32>(kChanB + 4 * k, f);
        ctx.fpr[1] = static_cast<f64>(f);  // the conversion leaves the last value in f1
    }
    store<u8>(kChanReset, 1);
    const u32 pool = load<u32>(kCbPool);
    store<u32>(kChan + 4, 0);
    store<u32>(kChan + 0, 0x42C80000u);  // 100.0f
    store<u32>(kChan + 8, 0);
    if (pool != 0) {
        call(ctx, h27fn::Obj_ClearField8, {pool});
        call(ctx, h27fn::Mem_FreeFrom, {pool, 0});
        store<u32>(kCbPool, 0);
    }
    const u32 buf = load<u32>(kCbBuf);
    if (buf == 0) return ret(ctx, 0u);
    call(ctx, h27fn::Mem_FreeFrom, {buf, 0});
    store<u32>(kCbBuf, 0);
    ret(ctx, 0x00F60000u);  // the lis r3 left by the store's address
}
