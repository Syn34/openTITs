// Game code, large-function sweep part 25: the "Mega" mode reset of a
// player-controlled transforming object (views, parts, sub-objects back to
// their defaults).
#include "../sly4.h"
#include "big25.h"

using namespace sly4;
using namespace sly4::big25;

namespace {
constexpr u32 kCMega = 0x0030FD54;                 // .text f32 table: 100, 0, 1, -1
constexpr u32 kMegaStr = 0x00ADA038;           // "Mega"
constexpr u32 kObjNotify24 = 0x000D949C;       // Obj_Notify24 (Obj*) -> Obj*
constexpr u32 kCameraPushTarget = 0x000FC040;  // Camera_PushTarget0FC040 (a, b, -, u8; f32) (reads r3 r4 r6 f1)
constexpr u32 kHasWord368 = 0x000FC5DC;        // Obj_HasWord368_000FC5DC (a, b) -> bool
constexpr u32 kSetU32_348 = 0x001EBDE4;        // SetU32_348_001EBDE4 (Obj*, u32 hash)
constexpr u32 kReleaseParts = 0x001F4460;      // Obj_ReleaseParts1F4460 (Obj*)
constexpr u32 kUsersUpdateFlags = 0x002E4A0C;  // Users_UpdateFlags2336 (users, u8) (reads r4 only)
constexpr u32 kPostReset = 0x00304298;         // (Obj*)
constexpr u32 kAttachCam = 0x003055E8;         // (x, u8, -, u8; f32) (reads r3 r4 r6 f1)
constexpr u32 kClimbFn = 0x00308F5C;           // Climb_Fn308F5C (climb, u8)
constexpr u32 kViewSet = 0x0030C14C;           // View_Set_0030C14C (View*, u8 a, u8 second)
constexpr u32 kSwitchSlot = 0x0030D2B8;        // Obj_SwitchSlot_0030D2B8 (Obj*, u32)
constexpr u32 kAnimSetTarget = 0x0030DBF0;     // Anim_SetTarget_0030DBF0 (o, x, -, u8 force; f32 t)
constexpr u32 kPlayerNotify = 0x0030FBC0;      // Player_Notify_0030FBC0 (o, u8) (reads r4 only)
constexpr u32 kPickEntry = 0x0030FC90;         // Obj_PickEntry_0030FC90 (Obj*)
constexpr u32 kSetHashObj = 0x00369D14;        // Global_SetHashObj_00369D14 (Obj*, const char*)
constexpr u32 kCopy216 = 0x00369D58;           // Global_F64CB8_Copy216 ()
constexpr u32 kFieldSet = 0x004A090C;          // Field_SetByteClear4_004A090C (field*, u8)
constexpr u32 kSpeakerSay = 0x004A5C90;        // Speaker_SayId4A5C90 (speaker, u32 id, u32; f32)
constexpr u32 kMemmove = 0x0071C3F0;           // memmove (dst, src, n)
constexpr u32 kUsers = 0x00F64C20;             // the users object Users_UpdateFlags2336 takes
constexpr u32 kSlotScale = 0x01069218;         // f32 copied into the slot object's +72
constexpr u32 kAnimDefault = 0x001EFED1u;      // the animation Anim_SetTarget switches to
constexpr u32 kSayA = 0x798D3062u;             // line said by +788 and +796
constexpr u32 kSayB = 0x451F0E5Bu;             // line said by +792
constexpr u32 kPartHash = 0x5F7669AFu;         // SetU32_348 hash for part 0 (+776)
constexpr u32 kAuxHash = 0x64590F24u;          // ... for +784
constexpr u32 kEffectHash = 0x8C991A36u;       // the effect the object switches to

f64 C(u32 off) { return Lf(kCMega + off); }

// The game's current player (world +4208 -> +8) when it has bit 8 of +192, else 0.
u32 FlaggedPlayer() {
    const u32 p = load<u32>(load<u32>(b25fn::Game) + 4208 + 8);
    return p != 0 && (load<u32>(p + 192) & 8) != 0 ? p : 0;
}

// Parts +776 .. +784: each set one gets Obj_ReleaseParts1F4460 and vfn[88](1, 0, 0, 0).
void ReleaseParts(PPUContext& ctx, u32 o) {
    for (u32 k = 0; k < 3; ++k) {
        if (load<u32>(o + 776 + 4 * k) == 0) continue;
        call(ctx, kReleaseParts, {load<u32>(o + 776 + 4 * k)});
        const u32 p = load<u32>(o + 776 + 4 * k);
        VCall(ctx, p, 88, {p, 1, 0, 0, 0});
    }
}
}  // namespace

// void f(T* o) (called by 003104D0, 00310980).
// Global_SetHashObj(o, "Mega"); +348 = clamp(+344, 0, 100) (as min then max);
// Global_F64CB8_Copy216(); the flagged player's byte +710 gets bit 0x40 (a
// null player writes address 710: quirk kept); the 38-byte default state
// built on the stack is memmoved to +664, then +664 / +665 = 1 and
// Obj_PickEntry. The speakers +788 / +796 say 0x798D3062 and +792
// 0x451F0E5B. Parts +776..+784 are released twice (around
// Field_SetByteClear4(+640, 0) and 0x00304298(o)); part 0 and +784 get
// their hashes; the views +812 / +824 reset (0, +665) and set (1, 1);
// +836 vfn[88](0, 1, 1, 1); Users_UpdateFlags2336(0x00F64C20 users, 0);
// fields +544 (1), +556, +592, +604, +616, +628 (0); the climb +872 gets
// Climb_Fn308F5C(0); animation 0x001EFED1 (forced, t 0); the 12 slots
// +704.. get +28 = 0; with +656: a camera target push (-1) unless
// Obj_HasWord368, then 0x003055E8(0, 1; t 0); the +876 sub-objects (u16
// +884) get vfn[40](0) and, flagged 0x40000, vfn[40](1); +884 / +1064 = 0;
// Player_Notify(1); a flagged player with Obj_Notify24 set has bit 8 of
// that result's +2338 set; Obj_SetEffectHash(0x8C991A36); the player's
// position (+144) goes to +976 and +992 (null player: address 144, quirk);
// with the slot object +704: Obj_SwitchSlot(0), its +72 = 0x01069218, and
// Anim_SetTarget(0, not forced; t 1).
DECOMP(0030FD68, MegaMode_Reset_0030FD68) {
    const u32 o = arg<u32>(ctx, 0);
    Stack frame(ctx, 208);
    const u32 sp = frame.addr();
    const u32 st = sp + 112;

    call(ctx, kSetHashObj, {o, kMegaStr});
    {
        const u32 w344 = load<u32>(o + 344);
        const f64 v344 = Lf(o + 344), hi = C(0), lo = C(4);
        const u32 vb = v344 > hi ? load<u32>(kCMega) : w344;
        const f64 v = v344 > hi ? hi : v344;
        store<u32>(o + 348, v > lo ? vb : load<u32>(kCMega + 4));
    }
    const f64 zero = C(4);
    call(ctx, kCopy216, {});
    {
        const u32 p = FlaggedPlayer();
        store<u8>(p + 710, static_cast<u8>(load<u8>(p + 710) | 0x40));  // p may be 0 (quirk)
    }
    // The default state (38 bytes; +2/+3 and +25..+27 are left as they were).
    store<u8>(st + 37, 0);
    store<u8>(st + 36, 0);
    store<u32>(st + 32, 0);
    store<u32>(st + 28, 0);
    store<u8>(st + 24, 0);
    store<u32>(st + 20, 0);
    store<u32>(st + 16, 0);
    store<u8>(st + 15, 0);
    store<u32>(st + 8, 0);
    store<u32>(st + 4, 0);
    store<u8>(st + 12, 1);
    store<u8>(st + 1, 0);
    store<u8>(st + 0, 0);
    store<u8>(st + 13, 1);
    store<u8>(st + 14, 1);
    call(ctx, kMemmove, {o + 664, st, 38});
    store<u8>(o + 664, 1);
    store<u8>(o + 665, 1);
    call(ctx, kPickEntry, {o});
    if (const u32 s = load<u32>(o + 788)) callf(ctx, kSpeakerSay, {load<u32>(s + 164), kSayA, 0}, {C(8)});
    if (const u32 s = load<u32>(o + 796)) callf(ctx, kSpeakerSay, {load<u32>(s + 164), kSayA, 0}, {C(8)});
    if (const u32 s = load<u32>(o + 792)) callf(ctx, kSpeakerSay, {load<u32>(s + 164), kSayB, 0}, {C(8)});

    ReleaseParts(ctx, o);
    call(ctx, kFieldSet, {o + 640, 0});
    call(ctx, kPostReset, {o});
    ReleaseParts(ctx, o);
    call(ctx, kFieldSet, {o + 640, 0});
    if (const u32 p0 = load<u32>(o + 776)) call(ctx, kSetU32_348, {p0, kPartHash});
    if (const u32 a = load<u32>(o + 784)) call(ctx, kSetU32_348, {a, kAuxHash});
    call(ctx, kViewSet, {o + 812, 0, load<u8>(o + 665)});
    call(ctx, kViewSet, {o + 824, 0, load<u8>(o + 665)});
    call(ctx, kViewSet, {o + 812, 1, 1});
    call(ctx, kViewSet, {o + 824, 1, 1});
    {
        const u32 x = load<u32>(o + 836);
        VCall(ctx, x, 88, {x, 0, 1, 1, 1});
    }
    call(ctx, kUsersUpdateFlags, {load<u32>(kUsers), 0});
    call(ctx, kFieldSet, {o + 544, 1});
    call(ctx, kFieldSet, {o + 556, 0});
    call(ctx, kFieldSet, {o + 592, 0});
    call(ctx, kFieldSet, {o + 604, 0});
    call(ctx, kFieldSet, {o + 616, 0});
    call(ctx, kFieldSet, {o + 628, 0});
    if (const u32 c = load<u32>(o + 872)) call(ctx, kClimbFn, {c, 0});
    // r5 is whatever the previous call left (the callee does not read it).
    callf(ctx, kAnimSetTarget, {o, kAnimDefault, ctx.gpr[5], 1}, {zero});
    for (u32 k = 0; k < 12; ++k)
        if (const u32 s = load<u32>(o + 704 + 4 * k)) store<u32>(s + 28, 0);
    if (const u32 t = load<u32>(o + 656)) {
        if (static_cast<u32>(call(ctx, kHasWord368, {load<u32>(t + 8), load<u32>(t + 0)})) == 0)
            callf(ctx, kCameraPushTarget, {load<u32>(t + 8), load<u32>(t + 0), load<u32>(t + 0), 0}, {C(12)});
        callf(ctx, kAttachCam, {load<u32>(o + 656), 0, ctx.gpr[5], 1}, {zero});  // r5 left over, unread
    }
    for (u32 i = 0, off = 0; i < load<u16>(o + 884); ++i, off += 4) {
        const u32 e = load<u32>(load<u32>(o + 876) + off);
        VCall(ctx, e, 40, {e, 0});
        const u32 e2 = load<u32>(load<u32>(o + 876) + off);
        if (load<u32>(e2 + 172) & 0x40000u) VCall(ctx, e2, 40, {e2, 1});
    }
    store<u16>(o + 884, 0);
    store<u8>(o + 1064, 0);
    call(ctx, kPlayerNotify, {o, 1});
    if (const u32 p = FlaggedPlayer()) {
        if (call(ctx, kObjNotify24, {p}) != 0) {
            const u32 r = static_cast<u32>(call(ctx, kObjNotify24, {FlaggedPlayer()}));
            store<u8>(r + 2338, static_cast<u8>(load<u8>(r + 2338) | 8));
        }
    }
    call(ctx, b25fn::Obj_SetEffectHash, {o, kEffectHash});
    const u32 slot = load<u32>(o + 704);
    const u32 src = FlaggedPlayer() + 144;  // a null player reads address 144 (quirk)
    const u32 x = load<u32>(src), y = load<u32>(src + 4), z = load<u32>(src + 8);
    store<u32>(o + 992, x);
    store<u32>(o + 996, y);
    store<u32>(o + 976, x);
    store<u32>(o + 1000, z);
    store<u32>(o + 980, y);
    store<u32>(o + 984, z);
    const u32 scale = load<u32>(kSlotScale);
    if (slot == 0) {
        ctx.gpr[3] = src;  // as the original leaves r3 / f1
        ctx.fpr[1] = mid15::Lf(src);
        return;
    }
    call(ctx, kSwitchSlot, {o, 0});
    store<u32>(slot + 72, scale);
    callf(ctx, kAnimSetTarget, {o, 0, kCMega, 0}, {C(8)});  // r5 = the table address the original leaves
}
