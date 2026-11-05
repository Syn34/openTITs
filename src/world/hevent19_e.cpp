// World_HandleEvent callees, part 19e: restarting an animator track
// (Anim_Restart2775A8), with its inlined random number step.
#include <bit>

#include "hevent19.h"

using namespace sly4;
using namespace sly4::hevent;
using namespace sly4::hevent19;

namespace {
using hevent7::lfs_f64;

// Animator: +8 Owner* (+172 u32 flags: bit 13; +176 u16 flags: bit 10),
// +32 current set handle, +36 u32 (0: no tracks), +40 eight 40-byte track
// slots {+0 key, +4 def, +12 u32, +16 u32, +20 u32, +24 f32, +28 u32}.
constexpr u32 kAnimOwner = 8, kAnimCurrent = 32, kAnim36 = 36, kSlots = 40, kSlotSize = 40, kNumSlots = 8;
constexpr u32 kSlotDef = 4, kSlot12 = 12, kSlot16 = 16, kSlot20 = 20, kSlot24 = 24, kSlot28 = 28;
constexpr u32 kOwnerFlags172 = 172, kOwnerBit13 = 0x2000u, kOwnerFlags176 = 176, kOwnerBit10 = 0x400u;
// Layer (from 0x007606C0): +20 u8 flags (bit 4: has a control value, bit
// 3), +56 s16 control value index into the pool's +268 8-byte table
// (count +272).
constexpr u32 kLayerFlags = 20, kLayerCtl = 56, kPoolCtlTable = 268, kPoolCtlCount = 272;
// Def: +20 u8 flags (bit 5), +58 s16 (-1: keep the weight), +16 u32.
constexpr u32 kDefFlags = 20, kDef58 = 58, kDef16 = 16;

// Constants at 0x0027759C: f32 random scale (1 / 65535), 0, 1.
constexpr u32 kConsts = 0x0027759C;
// The random state (as game/update.h): u64 last value, u32 mixer.
constexpr u32 kRandomLast = 0x01164888, kRandomState = 0x01164890, kRandomMix = 0x87654321u;
constexpr u32 kTextScale = 12, kTextScaleValue = 12;  // Text +12 -> +12 f32

constexpr u32 kFormat = 0x005534D0;      // String_Format (symbols.txt)
constexpr u32 kMsgCtl = 0x00AD4E44;      // "Unknown Anim ctrl value type pass used!"
constexpr u32 kLayerFind = 0x007606C0;   // (Set*, u32 layer, u32* out) -> Layer* (name?)
constexpr u32 kAnimStop = 0x00254564;    // Anim_Stop254564 (hevent18_g.cpp)
constexpr u32 kLayerQuery = 0x004B6760;  // (Set*, Layer*, Desc* out, Info* out, s32) (name?)
constexpr u32 kTextLookup = 0x0034A398;  // Text_Lookup34A398 (hevent17_a.cpp)
constexpr u32 kTrackStart = 0x00276EC0;  // (Animator*, ...; f32 x4) -> Track* (name?)
constexpr u32 kTrackBlend = 0x00277210;  // (Animator*, ...; f32 x5) (name?)
constexpr u32 kTrackSync = 0x00276BA4;   // (Animator*, u32 key) (name?)

// Locals, r1 +160 .. +291 of the original frame: +0 Info {u32, u32, f32
// +8, ...}, +16 u64 scratch, +24 u32, +28 104-byte Desc {+0 s32 mode, +4 u8
// flags (bit 7: skip), +8 def, +20 u32, +24 u32 text, +32 u32, +36 f32,
// +40 / +41 / +42 u8, +44 f32 weight, +52 f32 scale, +64 u32, +72 u32,
// +96 f32, +100 u32}.
constexpr u32 kLocals = 132;
constexpr u32 kInfo = 0, kScratch = 16, kOut24 = 24, kDesc = 28, kDescSize = 104;
constexpr u32 kDMode = 0, kDFlags = 4, kDDef = 8, kD20 = 20, kDText = 24, kD32 = 32, kD36 = 36;
constexpr u32 kD40 = 40, kD41 = 41, kD42 = 42, kDWeight = 44, kDScale = 52, kD64 = 64, kD72 = 72, kD96 = 96, kD100 = 100;

inline u32 rotl(u32 v, u32 n) { return (v << n) | (v >> (32 - n)); }
}  // namespace

// Track* Anim_Restart2775A8(Animator* a, u32 layer, u32 key, u8 c, -, -,
// u32 p6, u32 p7, u32 extra10, s32 extra11; f32 f1..f4) (name?): restarts
// the track slot holding key from the current set's layer description
// (stopping it first), or starts a new track (0x00276EC0) when there is no
// slot or no live set. Returns the started track or 0.
DECOMP(002775A8, Anim_Restart2775A8) {
    const u32 a = arg<u32>(ctx, 0), layer = arg<u32>(ctx, 1), key = arg<u32>(ctx, 2), c = arg<u8>(ctx, 3);
    const u64 p6 = ctx.gpr[9], p7 = ctx.gpr[10];
    const u32 sp0 = static_cast<u32>(ctx.gpr[1]);
    const u32 extra10 = load<u32>(sp0 + 132);                                       // 10th argument
    const s64 extra11 = static_cast<s32>(load<u32>(sp0 + 140));                     // 11th argument
    const f64 fa = farg(ctx, 0), fb = farg(ctx, 1), fc = farg(ctx, 2), fd = farg(ctx, 3);
    u32 slot = 0;
    for (u32 i = 0; i < kNumSlots; ++i)
        if (load<u32>(a + kSlots + kSlotSize * i) == key) {
            slot = a + kSlots + kSlotSize * i;
            break;
        }
    auto start_new = [&] {
        return callf(ctx, kTrackStart, {a, layer, key, c, 0, 0, 0, p6, p7, 0, extra10},
                     {fa, lfs_f64(kConsts + 4), fb, fd});
    };

    Stack frame(ctx, kLocals);
    const u32 L = frame.addr();
    const u32 D = L + kDesc;
    call(ctx, fn::memset, {D, 0, kDescSize});
    for (u32 k = 0; k < 16; k += 4) store<u32>(L + kInfo + k, 0);
    store<u32>(L + kOut24, 0);
    u32 lay = 0;
    if (const u32 obj = handle_object(load<u32>(kAnimPool), load<u32>(a + kAnimCurrent)); obj != 0)
        lay = static_cast<u32>(call(ctx, kLayerFind, {load<u32>(obj), layer, L + kOut24}));
    if (load<u32>(a + kAnim36) == 0) return ret(ctx, 0u);
    if (slot == 0) {
        start_new();
        return;
    }
    const u32 obj2 = handle_object(load<u32>(kAnimPool), load<u32>(a + kAnimCurrent));
    if (obj2 == 0 || lay == 0) {
        store<u32>(slot + kSlotDef, 0);
        store<u32>(slot + kSlot28, 0);
        start_new();
        return;
    }
    call(ctx, kAnimStop, {a, key});
    if ((load<u8>(lay + kLayerFlags) & 0x10) != 0) {
        const u32 pool = load<u32>(kAnimPool);
        const s32 t = static_cast<s16>(load<u16>(lay + kLayerCtl));
        const s32 n = static_cast<s32>(load<u32>(pool + kPoolCtlCount));
        if (t >= 0 && t < n && load<u32>(pool + kPoolCtlTable) + static_cast<u32>(t) * 8 != 0)
            store<f32>(L + kInfo + 8, static_cast<f32>(fc));
        else
            call(ctx, kFormat, {kMsgCtl});
    }
    store<u32>(L + kInfo + 0, load<u32>(slot + kSlot16));
    store<u32>(L + kInfo + 4, load<u32>(slot + kSlotDef));
    {
        const u32 obj3 = handle_object(load<u32>(kAnimPool), load<u32>(a + kAnimCurrent));
        // A stale handle reads the set from address 0, as the original.
        call(ctx, kLayerQuery, {load<u32>(obj3), lay, D, L + kInfo, static_cast<u64>(extra11)});
    }
    if (load<u8>(D + kD42) != 0 && load<u32>(a + kAnim36) != 0) {
        const u32 text = static_cast<u32>(call(ctx, kTextLookup, {load<u32>(D + kDText)}));
        if (text != 0) {
            // Random scale: the last value (u64, as a double) / 65535 times the text's scale.
            const u64 last = load<u64>(kRandomLast);
            const f64 k0 = lfs_f64(kConsts);
            const f64 tscale = lfs_f64(load<u32>(text + kTextScale) + kTextScaleValue);
            const u32 x = (rotl(load<u32>(kRandomState), 4) + rotl(load<u32>(kRandomState), 16)) ^ kRandomMix;
            store<u32>(kRandomState, rotl(x, 16));
            store<u64>(kRandomLast, x & 0xFFFFu);
            const f64 r = static_cast<f32>(rt::fcfid(last));
            const f32 s = fmuls(tscale, fmuls(r, k0));
            store<f32>(D + kDScale, s);
            ctx.fpr[1] = s;  // f1 / f2 / f3 as the original leaves them
            ctx.fpr[2] = tscale;
            ctx.fpr[3] = r;
        }
    }
    if ((load<u8>(D + kDFlags) & 0x80) != 0) return ret(ctx, 0u);
    store<u32>(slot + kSlotDef, load<u32>(D + kDDef));
    store<u32>(slot + kSlot28, load<u32>(L + kOut24));
    const f64 zero = lfs_f64(kConsts + 4);
    store<u32>(slot + kSlot12, 0);
    const s32 mode = static_cast<s32>(load<u32>(D + kDMode));
    if (mode <= 0 || mode > 2) return ret(ctx, 0u);
    u32 def = load<u32>(D + kDDef);
    auto common = [&] {
        store<u32>(slot + kSlot24, load<u32>(L + kInfo + 8));
        store<u32>(slot + kSlot16, load<u32>(D + kD20));
        store<u32>(slot + kSlot20, load<u32>(D + kDText));
        const s32 w = static_cast<s16>(load<u16>(def + kDef58)) + 1;
        const u32 owner = load<u32>(a + kAnimOwner);
        if (w != 0) store<u32>(D + kDWeight, 0);
        const u32 bit = (load<u8>(def + kDefFlags) >> 5) & 1;
        store<u32>(owner + kOwnerFlags172, (load<u32>(owner + kOwnerFlags172) & ~kOwnerBit13) | (bit << 13));
    };
    if (mode == 2) {
        common();
        callf(ctx, kTrackBlend,
              {a, load<u32>(D + kDText), load<u32>(D + kD64), load<u8>(D + kD40), load<u32>(D + kD32), 0, load<u32>(D + kD100),
               load<u8>(D + kD41), 0, 0, 0, load<u8>(D + kD40), load<u32>(D + kD32), load<u32>(D + kD72)},
              {lfs_f64(kConsts + 8), lfs_f64(D + kD96), lfs_f64(D + kDWeight), lfs_f64(D + kD36), lfs_f64(D + kDScale)});
        def = load<u32>(slot + kSlotDef);  // then the mode-1 steps again with the slot's def
    }
    common();
    const f64 f1 = fmuls(lfs_f64(D + kDWeight), fa);
    const f64 f2 = lfs_f64(D + kDScale);
    const f64 f3 = fb == zero ? fb : lfs_f64(D + kD36);
    const u64 r7 = static_cast<u32>(p7) != 0 ? p7 : load<u32>(load<u32>(slot + kSlotDef) + kDef16);
    const u32 d32 = load<u32>(D + kD32);
    callf(ctx, kTrackStart, {a, load<u32>(D + kDText), key, load<u8>(D + kD41), r7, d32, 0, load<u8>(D + kD40), r7, 0, d32},
          {f1, f2, f3, fd});
    const u64 t = ctx.gpr[3];
    if (static_cast<u32>(t) == 0) return ret(ctx, t);
    const u32 owner = load<u32>(a + kAnimOwner);
    const u32 bit = (load<u8>(lay + kLayerFlags) >> 3) & 1;
    store<u16>(owner + kOwnerFlags176, static_cast<u16>((load<u16>(owner + kOwnerFlags176) & ~kOwnerBit10) | (bit << 10)));
    call(ctx, kTrackSync, {a, key});
    ret(ctx, t);
}
