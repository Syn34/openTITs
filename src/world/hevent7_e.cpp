// World_HandleEvent callees, part 7e: two of the handler's remaining
// out-of-line cases (HUD refresh over the user's objects, and the
// all-ready check that starts the next sequence).
#include "hevent7.h"

using namespace sly4;
using namespace sly4::hevent;
using namespace sly4::hevent7;

namespace sly4 {
namespace hevent7e {
namespace efn {
constexpr u32 Ui_Find = 0x004D5544;              // (const char* path, Element* parent) -> Element*
constexpr u32 Obj_CanShow2E2104 = 0x002E2104;    // (Session*, Object*) -> bool (name?)
constexpr u32 Handle_ResolveBit1 = 0x003568E0;   // (handle) -> Object*
constexpr u32 Obj_SetText367530 = 0x00367530;    // (Object*, const char*, 0) (name?)
constexpr u32 Obj_Dispatch362 = 0x0036325C;      // (Object*, 1, 1, 0)
constexpr u32 String_Format = 0x005534D0;        // (fmt, ...) -> rotating static buffer
constexpr u32 Obj_SetAnim = 0x00363A3C;          // (o, name, now, a, b, c, d; f32)
constexpr u32 Obj_Notify24 = 0x000D949C;         // (Object*) -> Object*
constexpr u32 Obj_UpdateFlags2336 = 0x0029D838;  // (Object*, u8)
constexpr u32 PtrVector_Construct = 0x005516E0;  // (vec)
constexpr u32 PtrVector_Init = 0x00551598;       // (vec, capacity, heap, 0)
constexpr u32 PtrVector_Clear = 0x0054EC1C;      // (vec)
constexpr u32 Nop = 0x0054EC18;
constexpr u32 Args_AddObject49FD84 = 0x0049FD84; // (vec, u32 name hash, Object*) (name?)
constexpr u32 Args_AddValue49FE90 = 0x0049FE90;  // (vec, u32 name hash, u32) (name?)
constexpr u32 World_Pause489F3C = 0x00489F3C;    // (World*, 1) (name?)
constexpr u32 Obj_Reset121FA8 = 0x00121FA8;      // (Object*, 0, 0, 0, 0, 1) (name?)
constexpr u32 World_StartScene46C3B8 = 0x0046C3B8;  // (World*, a, 0, 0, args, b, 0, 1, u8, 0) (name?)
constexpr u32 World_RequestState = 0x004665C0;   // (World*, s32 layer, s32 state)
}  // namespace efn

constexpr u32 kStrHud = 0x00AD8CD8;        // "InGame.HUD"
constexpr u32 kStrEmpty = 0x00AD8CF0;      // ""
constexpr u32 kFmtS = 0x00AD8CE8;          // "%s"
constexpr u32 kSession = 0x00F64C20;       // u32 Session* (as hevent4_d.cpp)
constexpr u32 kHudScaleObj = 0x00F64CCC;   // u32 handle of the HUD scale object (name?)
constexpr u32 kAnimArgA = 0x01068210;      // passed as Obj_SetAnim's a; f32 at +72 (name?)
constexpr u32 kAnimArgD = 0x01164CA4;      // passed as Obj_SetAnim's d (name?)
// User block (World +4208, 0x20C80 each): +0 u32 flags, +8 Object*, +80
// Object** entries, +88 u16 count, +0x20074 u32 per-entry state (index at
// entry +2452). World: +29 u8 user count, +0x128224 u32.
constexpr u32 kBlkFlags = 0, kBlkObj = 8, kBlkEntries = 80, kBlkCount = 88, kBlkState = 0x20074;
constexpr u32 kEntryStateIdx = 2452, kEntryKind = 18, kEntryHasText = 2460;
constexpr u32 kWorldUsers = 29, kWorld128224 = 0x128224;
constexpr u32 kStateDirty = 0x08000000;    // bit 4 (IBM)
constexpr u32 kStateReady = 5;             // top nibble
constexpr u32 kBlkStarted = 0x80;
constexpr u32 kKindScale = 4;
constexpr u32 kHashArgObject = 0x8CD18EE1, kHashArgValue = 0xB2E59A9F;
}  // namespace hevent7e
}  // namespace sly4

using namespace sly4::hevent7e;

// void Hud_Refresh2E4AAC() (name?): for each of the first user's entries
// whose state is dirty (bit 0x08000000) and that the session may show:
// a kind-4 entry updates the HUD scale object (no text: empty text and
// dispatch; text from virtual +48: Obj_SetAnim("%s" of it)) and, with text,
// refreshes every user's object (Obj_Notify24 / Obj_UpdateFlags2336); the
// dirty bit is then cleared. Last the HUD's virtual +88 gets whether
// World +0x128224 is 0. Nothing without "InGame.HUD".
DECOMP(002E4AAC, Hud_Refresh2E4AAC) {
    const u32 hud = static_cast<u32>(call(ctx, efn::Ui_Find, {kStrHud, 0}));
    if (hud == 0) return;
    const u32 blk = load<u32>(kCurrentWorld) + kUserBlocks;
    for (u32 i = 0; i < load<u16>(blk + kBlkCount); ++i) {
        const u32 e = load<u32>(load<u32>(blk + kBlkEntries) + 4 * i);
        const u32 state = blk + (load<u32>(e + kEntryStateIdx) << 2) + kBlkState;
        if (!(load<u32>(state) & kStateDirty)) continue;
        if (call(ctx, efn::Obj_CanShow2E2104, {load<u32>(kSession), e}) == 0) continue;
        if (load<u8>(e + kEntryKind) == kKindScale) {
            const u32 t = static_cast<u32>(call(ctx, efn::Handle_ResolveBit1, {load<u32>(kHudScaleObj)}));
            if (t != 0) {
                if (load<u8>(e + kEntryHasText) == 0) {
                    call(ctx, efn::Obj_SetText367530, {t, kStrEmpty, 0});
                    call(ctx, efn::Obj_Dispatch362, {t, 1, 1, 0});
                } else {
                    const u32 s = static_cast<u32>(hevent::vcall(ctx, e, 48, {e}));
                    if (s != 0 && load<u8>(s) != 0) {
                        const u64 text = call(ctx, efn::String_Format, {kFmtS, s});
                        callf(ctx, efn::Obj_SetAnim, {t, text, 1, kAnimArgA, 0, 0, kAnimArgD}, {lfs_f64(kAnimArgA + 72)});
                    }
                }
            }
            if (load<u8>(e + kEntryHasText) != 0) {
                for (u32 j = 0, off = 0;; ++j, off += kUserBlockSize) {
                    const u32 w = load<u32>(kCurrentWorld);
                    if (j >= load<u8>(w + kWorldUsers)) break;
                    if (const u32 o = load<u32>(w + off + kUserBlocks + kBlkObj); o != 0) {
                        if (const u64 r = call(ctx, efn::Obj_Notify24, {o}); static_cast<u32>(r) != 0)
                            call(ctx, efn::Obj_UpdateFlags2336, {r, 0});
                    }
                }
            }
        }
        store<u32>(state, load<u32>(state) & ~kStateDirty);
    }
    const u32 off_hud = load<u32>(load<u32>(kCurrentWorld) + kWorld128224) == 0 ? 1u : 0u;
    hevent::vcall(ctx, hud, 88, {hud, off_hud});
}

// void Users_StartIfReady2E4750(Event* ev) (name?): when the first user's
// block is not started (+0 bit 0x80), has entries and all of them are in
// state 5: clears the session flag (ev +40 -> +404), builds an argument list
// {0x8CD18EE1: block object, 0xB2E59A9F: b} (a / b from ev +40 +480 / +484,
// else ev +36 +732 / +736), pauses the world, resets the block object when
// its +192 has 8 and it has a +424 child (virtual +40 with 1), starts the
// scene (World_StartScene46C3B8), marks the block started and requests
// states 1 = 11 and 2 = 0.
DECOMP(002E4750, Users_StartIfReady2E4750) {
    const u32 ev = arg<u32>(ctx, 0);
    const u32 blk = load<u32>(kCurrentWorld) + kUserBlocks;
    const u32 n = load<u16>(blk + kBlkCount);
    const u32 flags = load<u32>(blk + kBlkFlags);
    u32 ready = 0;
    for (u32 i = 0; i < n; ++i) {
        const u32 e = load<u32>(load<u32>(blk + kBlkEntries) + 4 * i);
        const u32 st = load<u32>(blk + (load<u32>(e + kEntryStateIdx) << 2) + kBlkState);
        ready += (st >> 28) == kStateReady ? 1 : 0;
    }
    if ((flags & kBlkStarted) || n == 0 || ready != n) {
        ctx.gpr[3] = (flags >> 7) & 1;  // r3 as the original leaves it
        return;
    }
    const u32 p = load<u32>(ev + 36);
    u32 a = p != 0 ? load<u32>(p + 732) : 0;
    const u32 q = load<u32>(ev + 40);
    if (const u32 v = load<u32>(q + 480); v != 0) a = v;
    u32 b = p != 0 ? load<u32>(p + 736) : 0;
    if (const u32 v = load<u32>(q + 484); v != 0) b = v;
    store<u8>(q + 404, 0);
    Stack frame(ctx, 32);  // PtrVector at r1+128
    const u32 vec = frame.addr();
    call(ctx, efn::PtrVector_Construct, {vec});
    store<u16>(vec + 6, 76);
    call(ctx, efn::PtrVector_Init, {vec, 2, 0, 0});
    call(ctx, efn::Args_AddObject49FD84, {vec, kHashArgObject, load<u32>(blk + kBlkObj)});
    call(ctx, efn::Args_AddValue49FE90, {vec, kHashArgValue, b});
    call(ctx, efn::World_Pause489F3C, {load<u32>(kCurrentWorld), 1});
    if (const u32 o = load<u32>(blk + kBlkObj); o != 0 && (load<u32>(o + 192) & 8) != 0) {
        if (const u32 c = load<u32>(o + 424); c != 0) {
            call(ctx, efn::Obj_Reset121FA8, {o, 0, 0, 0, 0, 1});
            hevent::vcall(ctx, c, 40, {c, 1});
        }
    }
    const u32 u8_505 = load<u8>(load<u32>(ev + 40) + 505);
    call(ctx, efn::World_StartScene46C3B8, {load<u32>(kCurrentWorld), a, 0, 0, vec, b, 0, 1, u8_505, 0});
    store<u32>(blk + kBlkFlags, load<u32>(blk + kBlkFlags) | kBlkStarted);
    call(ctx, efn::World_RequestState, {load<u32>(kCurrentWorld), 1, 11});
    call(ctx, efn::World_RequestState, {load<u32>(kCurrentWorld), 2, 0});
    call(ctx, efn::PtrVector_Clear, {vec});
    call(ctx, efn::Nop, {vec});
}
