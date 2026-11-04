// World_HandleEvent callees, part 13g: World_UpdateScenes48B810, which runs
// the world's queued effect scenes (World_StartScene46C3B8) one after the
// other and finishes the queue.
#include "hevent13.h"

using namespace sly4;
using namespace sly4::hevent13;

namespace {
// World: +6 u16, +20 base (+ the u16 at +6: object with +60 flags), +31 u8
// paused, +45 u8 flags, +4208 user block 0, vtable +32 (w, 1) -> s32 mode.
constexpr u32 kWPaused = 31, kWFlags45 = 45, kWFlags45Done = 0x20;
constexpr u32 kWBase20 = 20, kWOffset6 = 6, kBaseFlags60 = 60, kBaseFlag60Bit = 0x40000000u;
constexpr u32 kWVtMode = 32;
constexpr s32 kModeTwelve = 12;
// Scene queue (as hevent8_c.cpp): +0x12820C Scene*[], +0x128214 u16 count,
// +0x128220 / +0x128221 / +0x128222 u8 flags; +0x128168 View*; +0x1281E0
// Object*; +0x128584 u32.
constexpr u32 kWScenes = 0x12820C, kWSceneCount = 0x128214;
constexpr u32 kWSceneFlagA = 0x128220, kWSceneFlagB = 0x128221, kWSceneFirst = 0x128222;
constexpr u32 kWView = 0x128168, kW1281E0 = 0x1281E0, kW128584 = 0x128584;
// Scene: +0 argument vector, +20 e, +24 def, +28 Effect*, +32 f32 time, +36
// f32 rate, +40 hash, +48 flags (bit 30 the start's g: player hook, bit 29
// ended).
constexpr u32 kSE = 20, kSDef = 24, kSEffect = 28, kSTime = 32, kSRate = 36, kSHash = 40, kSFlags = 48;
constexpr u32 kSFlagHook = 0x40000000u, kSFlagEnded = 0x20000000u;
// User block 0 (World +4208): +8 player, +0x20474 64 x 32-byte ring
// {+0x20C74 u16 end, +0x20C76 u16 start} (2052 bytes cleared together).
constexpr u32 kUbPlayer = 8, kUbRing = 0x20474, kUbRingEnd = 0x20C74, kUbRingStart = 0x20C76, kUbRingBytes = 2052;
constexpr u32 kRingMask = 0x3F, kRingStride = 32;
constexpr u32 kObjFlags192 = 192, kObjFlag192Bit3 = 0x8, kObj188 = 188, kObj188Bit5 = 0x20;
constexpr u32 kObjVt176 = 176;

constexpr u32 kPauseHandle = 0x00F64CFC;    // handle: its object +160 >> 1 non-zero freezes the scenes
constexpr u32 kZero48B808 = 0x0048B808;     // f32 0.0
constexpr u32 kSceneView = 0x010B8B98 + 72; // f32 rate for a new scene's view
constexpr u32 kSceneDestroy = 0x010B9CDC + 72;  // u8: rerun finished scenes (name?)
constexpr u32 kSession = 0x00F64C20;

constexpr u32 kHandleResolve = 0x00365E30, kViewSet = 0x00520338, kEffectSetTime = 0x004CF710;
constexpr u32 kEffectCheck = 0x004CF620, kEntryBusy = 0x0042C22C, kEffectDestruct = 0x004C8E80;
constexpr u32 kEffectAddArgs = 0x0049FCC0, kMemFreeFrom = 0x0054EE6C, kMemset = 0x00726E54;
constexpr u32 kObjSetRenderTarget = 0x000FA3CC;
constexpr u32 kScenesEnd48B674 = 0x0048B674;   // (World*, u8) (name?)
constexpr u32 kPlayer125950 = 0x00125950;      // (Object*) (name?)
constexpr u32 kRing2E5180 = 0x002E5180;        // (Session*, 0, entry*) (name?)
constexpr u32 kWorld4845F0 = 0x004845F0;       // (World*, UserBlock*) (name?)
constexpr u32 kObjDD02C = 0x000DD02C;          // (Object*, 0, 0) (name?)
constexpr u32 kNopFn = 0x0054EC18;             // Nop (symbols.txt)

void destroy_effect(PPUContext& ctx, u32 s) {
    const u32 fx = load<u32>(s + kSEffect);
    call(ctx, kEffectDestruct, {fx});
    call(ctx, hevent7::h7fn::SmallFree, {fx, 140});
    store<u32>(s + kSEffect, 0);
}
}  // namespace

// u32 World_UpdateScenes48B810(World* w; f32 dt) (name?): unless paused,
// runs the queued scenes in order: a scene without an effect gets one (its
// def, args and e; 0x00125950 first for a hooked scene's player), its time
// advances by dt (0 while the 0x00F64CFC object freezes it); the first one
// still running stops the pass. A done scene that is not busy ends (bit 29,
// vt +32, view rate) or, with 0x010B9D24, is rerun with a new effect. Then
// ended scenes are freed and removed (from the front, as the original),
// and an emptied queue flushes the ring (0x002E5180 per entry), clears the
// flags and ends (0x0048B674 / 0x004845F0 and the users' vt +176).
DECOMP(0048B810, World_UpdateScenes48B810) {
    const u32 w = arg<u32>(ctx, 0);
    f64 dt = ctx.fpr[1];
    const u8 paused = load<u8>(w + kWPaused);
    if (paused != 0) return ret(ctx, static_cast<u32>(paused));
    if (load<u8>(w + kWSceneFirst) != 0) {
        store<u8>(w + kWSceneFirst, 0);
        call(ctx, kScenesEnd48B674, {w, 1});
    }
    const u32 ub = w + hevent::kUserBlocks;
    // Pass 1: run the scenes.
    for (s32 i = 0; i < static_cast<s32>(load<u16>(w + kWSceneCount)); ++i) {
        const u32 s = load<u32>(load<u32>(w + kWScenes) + 4 * static_cast<u32>(i));
        if (call(ctx, kHandleResolve, {load<u32>(kPauseHandle)}) != 0) {
            const u32 o = static_cast<u32>(call(ctx, kHandleResolve, {load<u32>(kPauseHandle)}));
            if ((load<u32>(o + 160) >> 1) != 0) dt = hevent7::lfs_f64(kZero48B808);
        }
        if (load<u32>(s + kSEffect) == 0) {
            if ((load<u32>(s + kSFlags) & kSFlagHook) != 0) {
                const u32 p = load<u32>(ub + kUbPlayer);
                if (p != 0 && (load<u32>(p + kObjFlags192) & kObjFlag192Bit3) != 0 &&
                    (load<u32>(p + kObj188) & kObj188Bit5) != 0)
                    call(ctx, kPlayer125950, {p});
            }
            const u32 a = static_cast<u32>(call(ctx, hevent7::h7fn::SmallAlloc, {140}));
            u32 fx = 0;
            if (a != 0) {
                call(ctx, hevent7::h7fn::Effect_Construct, {a, load<u32>(s + kSDef)});
                fx = a;
            }
            const u32 e = load<u32>(s + kSE);
            store<u32>(s + kSEffect, fx);
            call(ctx, kEffectAddArgs, {fx, s, e, 0, 0});
            const u32 view = load<u32>(w + kWView);
            if (view != 0) {
                store<u32>(s + kSRate, load<u32>(view + 60));
                callf(ctx, kViewSet, {load<u32>(w + kWView)}, {hevent7::lfs_f64(kSceneView)});
            }
        }
        const u32 fx = load<u32>(s + kSEffect);
        if (fx == 0 || (load<u32>(s + kSFlags) & kSFlagEnded) != 0) continue;
        store<u32>(s + kSTime, std::bit_cast<u32>(static_cast<f32>(hevent7::lfs_f64(s + kSTime) + dt)));
        callf(ctx, kEffectSetTime, {fx}, {dt});
        if (call(ctx, kEffectCheck, {load<u32>(s + kSEffect), 0}) == 0) break;
        if (call(ctx, kEntryBusy, {load<u32>(s + kSHash)}) != 0) break;
        if (load<u8>(kSceneDestroy) != 0) {
            if (static_cast<s32>(hevent::vcall(ctx, w, kWVtMode, {w, 1})) == kModeTwelve) {
                const u32 x = load<u32>(w + kWBase20) + load<u16>(w + kWOffset6);
                store<u32>(x + kBaseFlags60, load<u32>(x + kBaseFlags60) | kBaseFlag60Bit);
            }
            if (load<u32>(w + kWView) != 0) callf(ctx, kViewSet, {load<u32>(w + kWView)}, {hevent7::lfs_f64(s + kSRate)});
            if (load<u32>(s + kSEffect) != 0) destroy_effect(ctx, s);
            --i;  // the same scene again
            continue;
        }
        store<u32>(s + kSFlags, load<u32>(s + kSFlags) | kSFlagEnded);
        if (static_cast<s32>(hevent::vcall(ctx, w, kWVtMode, {w, 1})) == kModeTwelve) {
            const u32 x = load<u32>(w + kWBase20) + load<u16>(w + kWOffset6);
            store<u32>(x + kBaseFlags60, load<u32>(x + kBaseFlags60) | kBaseFlag60Bit);
        }
        if (load<u32>(w + kWView) != 0) callf(ctx, kViewSet, {load<u32>(w + kWView)}, {hevent7::lfs_f64(s + kSRate)});
    }
    // Pass 2: free the finished scenes.
    for (s32 i = 0;; ++i) {
        const u32 n = load<u16>(w + kWSceneCount);
        if (i >= static_cast<s32>(n)) return ret(ctx, n);
        const u32 s = load<u32>(load<u32>(w + kWScenes) + 4 * static_cast<u32>(i));
        const u32 f = load<u32>(s + kSFlags);
        if ((f & kSFlagEnded) == 0) continue;
        const u32 fx = load<u32>(s + kSEffect);
        store<u32>(s + kSFlags, f & ~kSFlagEnded);
        if (fx != 0) destroy_effect(ctx, s);
        if (s != 0) {
            call(ctx, hevent::hfn::PtrVector_Clear, {s});
            call(ctx, kNopFn, {s});
            call(ctx, kMemFreeFrom, {s, 0});
        }
        // Removes the first entry (not entry i), as the original.
        u32 last;
        for (u32 j = 0;; ++j) {
            const u32 c = load<u16>(w + kWSceneCount);
            last = c - 1;
            if (j >= last) break;
            const u32 d = load<u32>(w + kWScenes);
            store<u32>(d + 4 * j, load<u32>(d + 4 * (j + 1)));
        }
        store<u16>(w + kWSceneCount, static_cast<u16>(last));
        u32 left = last & 0xFFFF;
        if (last != left) {  // count was 0: an unchecked copy below the array
            const u32 d = load<u32>(w + kWScenes);
            store<u32>(d - 4, load<u32>(d + ((last << 2) & 0x3FFFC)));
            left = load<u16>(w + kWSceneCount);
        }
        if (left != 0) {
            --i;  // the same index again
            continue;
        }
        // Emptied: flush the ring.
        u32 cur = load<u16>(ub + kUbRingStart);
        if (cur != load<u16>(ub + kUbRingEnd)) {
            for (u32 end = load<u16>(ub + kUbRingEnd);; end = load<u16>(ub + kUbRingEnd)) {
                const u32 k = cur & kRingMask;
                if (k == end) {
                    call(ctx, kMemset, {ub + kUbRing, 0, kUbRingBytes});
                    break;
                }
                call(ctx, kRing2E5180, {load<u32>(kSession), 0, ub + ((cur << 5) & 0x7E0) + kUbRing});
                cur = k + 1;
            }
        }
        store<u8>(w + kWSceneFlagB, 0);
        store<u8>(w + kWFlags45, static_cast<u8>(load<u8>(w + kWFlags45) | kWFlags45Done));
        if (load<u8>(w + kWSceneFlagA) == 0) {
            call(ctx, kScenesEnd48B674, {w, 0});
            return;
        }
        store<u8>(w + kWSceneFlagA, 0);
        call(ctx, kWorld4845F0, {w, ub});
        const u32 o = load<u32>(w + kW1281E0);
        store<u32>(w + kW128584, 0);
        if (o != 0) {
            call(ctx, kObjDD02C, {o, 0, 0});
            call(ctx, kObjSetRenderTarget, {load<u32>(w + kW1281E0), 0});
        }
        u64 r3 = 0;
        for (u32 k = 0; k < kWNumUsers; ++k) {
            const u32 u = load<u32>(w + kWUsers + 4 * k);
            r3 = u;
            if (u != 0) r3 = hevent::vcall(ctx, u, kObjVt176, {u, 1});
        }
        ctx.gpr[3] = r3;
        return;
    }
}
