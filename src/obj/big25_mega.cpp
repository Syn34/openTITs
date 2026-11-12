// Game code, large-function sweep part 25: the "Mega" mode reset of a
// player object (state, speakers, parts, views, fields, camera, effect).
#include "big25.h"

namespace sly4::obj {

namespace {
constexpr u32 kCMega = 0x0030FD54;                 // .text f32 table: 100, 0, 1, -1
constexpr u32 kMegaStr = 0x00ADA038;           // "Mega"
constexpr u32 kObjNotify24 = 0x000D949C;       // Obj_Notify24 (Obj*) -> Obj*
constexpr u32 kCameraPushTarget = 0x000FC040;  // Camera_PushTarget0FC040 (a, b; f32, u8)
constexpr u32 kHasWord368 = 0x000FC5DC;        // Obj_HasWord368_000FC5DC (a, b) -> bool
constexpr u32 kSetU32_348 = 0x001EBDE4;        // SetU32_348_001EBDE4 (Obj*, u32 hash)
constexpr u32 kReleaseParts = 0x001F4460;      // Obj_ReleaseParts1F4460 (Obj*)
constexpr u32 kUsersUpdateFlags = 0x002E4A0C;  // Users_UpdateFlags2336 (users, u8) (reads r4 only)
constexpr u32 kPostReset = 0x00304298;         // (Obj*)
constexpr u32 kAttachCam = 0x003055E8;         // (x, u8; f32, u8)
constexpr u32 kClimbFn = 0x00308F5C;           // Climb_Fn308F5C (climb, u8)
constexpr u32 kViewSet = 0x0030C14C;           // View_Set_0030C14C (View*, u8 a, u8 second)
constexpr u32 kSwitchSlot = 0x0030D2B8;        // Obj_SwitchSlot_0030D2B8 (Obj*, u32)
constexpr u32 kAnimSetTarget = 0x0030DBF0;     // Anim_SetTarget_0030DBF0 (o, x; f32 t, u8 force)
constexpr u32 kPlayerNotify = 0x0030FBC0;      // Player_Notify_0030FBC0 (o, u8) (reads r4 only)
constexpr u32 kPickEntry = 0x0030FC90;         // Obj_PickEntry_0030FC90 (Obj*)
constexpr u32 kSetHashObj = 0x00369D14;        // Global_SetHashObj_00369D14 (Obj*, const char*)
constexpr u32 kCopy216 = 0x00369D58;           // Global_F64CB8_Copy216 ()
constexpr u32 kFieldSet = 0x004A090C;          // Field_SetByteClear4_004A090C (field*, u8)
constexpr u32 kSpeakerSay = 0x004A5C90;        // Speaker_SayId4A5C90 (speaker, u32 id, u32, f32)
constexpr u32 kUsers = 0x00F64C20;             // the users object Users_UpdateFlags2336 takes
constexpr u32 kSlotScale = 0x01069218;         // f32 copied into the slot object's +72
constexpr u32 kAnimDefault = 0x001EFED1u;      // the animation Anim_SetTarget switches to
constexpr u32 kSayA = 0x798D3062u;             // line said by +788 and +796
constexpr u32 kSayB = 0x451F0E5Bu;             // line said by +792
constexpr u32 kPartHash = 0x5F7669AFu;         // SetU32_348 hash for part 0 (+776)
constexpr u32 kAuxHash = 0x64590F24u;          // ... for +784
constexpr u32 kEffectHash = 0x8C991A36u;       // the effect the object switches to

constexpr u32 Obj_SetEffectHash = 0x000DAEC8;  // Obj_SetEffectHash_000DAEC8 (Obj*, u32 hash)

// The 38-byte default state memmoved to +664.
struct MegaState {
    u8 b0, b1;       // +0, +1
    pad<2> _2;
    be_u32 w4;       // +4
    be_u32 w8;       // +8
    u8 b12, b13, b14, b15;  // +12 .. +15
    be_u32 w16;      // +16
    be_u32 w20;      // +20
    u8 b24;          // +24
    pad<3> _25;
    be_u32 w28;      // +28
    be_u32 w32;      // +32
    u8 b36, b37;     // +36, +37
};
static_assert(sizeof(MegaState) == 38);
struct MegaCamRef {
    be_u32 w0;  // +0
    pad<4> _4;
    be_u32 w8;  // +8
};
struct MegaObj {
    pad<344> _0;
    be_f32 f344;         // +344
    be_u32 w348;         // +348
    pad<192> _352;
    pad<4> f544;         // +544 .. the fields (addresses passed)
    pad<92> _548;
    pad<4> f640;         // +640
    pad<12> _644;
    be_u32 cam;          // +656: MegaCamRef*
    pad<4> _660;
    u8 st664;            // +664: the state (38 bytes)
    u8 st665;            // +665
    pad<38> _666;
    be_u32 slots[12];    // +704
    pad<24> _752;
    be_u32 parts[3];     // +776
    be_u32 spk[3];       // +788 / +792 / +796
    pad<12> _800;
    pad<12> view812;     // +812
    pad<12> view824;     // +824
    be_u32 x836;         // +836
    pad<32> _840;
    be_u32 climb;        // +872
    be_u32 subs;         // +876: T*[]
    pad<4> _880;
    be_u16 nsubs;        // +884
    pad<90> _886;
    Vec3 p976;           // +976
    pad<4> _988;
    Vec3 p992;           // +992
    pad<60> _1004;
    u8 b1064;            // +1064
};
static_assert(offsetof(MegaObj, f544) == 544 && offsetof(MegaObj, cam) == 656 && offsetof(MegaObj, slots) == 704);
static_assert(offsetof(MegaObj, parts) == 776 && offsetof(MegaObj, view812) == 812 && offsetof(MegaObj, climb) == 872);
static_assert(offsetof(MegaObj, nsubs) == 884 && offsetof(MegaObj, p976) == 976 && offsetof(MegaObj, b1064) == 1064);

f64 C(u32 off) { return lf(global<be_f32>(kCMega + off)); }

// The game's current player (world +4208 -> +8) when it has bit 8 of +192, else 0.
u32 FlaggedPlayer() {
    const u32 p = gm<be_u32>(global<be_u32>(kWorld) + 4208 + 8);
    return p != 0 && (gm<be_u32>(p + 192) & 8) != 0 ? p : 0u;
}

// Parts +776 .. +784: each set one gets Obj_ReleaseParts1F4460 and vfn[88](1, 0, 0, 0).
void ReleaseParts(MegaObj* o) {
    for (u32 k = 0; k < 3; ++k) {
        if (o->parts[k] == 0u) continue;
        call(kReleaseParts, o->parts[k].get());
        vcall(&gm<u8>(o->parts[k]), 88, 1u, 0u, 0u, 0u);
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
// Anim_SetTarget(0, not forced; t 1). No caller reads r3 or f1 (r3.sh).
void MegaMode_Reset_0030FD68(MegaObj* o) {
    GuestFrame frame(208);
    MegaState& st = *frame.as<MegaState>(112);

    call(kSetHashObj, o, kMegaStr);
    {
        const u32 w344 = bits(o->f344);
        const f64 v344 = lf(o->f344), hi = C(0), lo = C(4);
        const u32 vb = v344 > hi ? bits(global<be_f32>(kCMega)) : w344;
        const f64 v = v344 > hi ? hi : v344;
        o->w348 = v > lo ? vb : bits(global<be_f32>(kCMega + 4));
    }
    const f64 zero = C(4);
    call(kCopy216);
    {
        u8& b = gm<u8>(FlaggedPlayer() + 710);  // the player may be 0 (quirk)
        b = static_cast<u8>(b | 0x40);
    }
    // The default state (38 bytes; +2/+3 and +25..+27 are left as they were).
    st.b37 = 0;
    st.b36 = 0;
    st.w32 = 0u;
    st.w28 = 0u;
    st.b24 = 0;
    st.w20 = 0u;
    st.w16 = 0u;
    st.b15 = 0;
    st.w8 = 0u;
    st.w4 = 0u;
    st.b12 = 1;
    st.b1 = 0;
    st.b0 = 0;
    st.b13 = 1;
    st.b14 = 1;
    call(fn::Memmove, &o->st664, &st, 38u);
    o->st664 = 1;
    o->st665 = 1;
    call(kPickEntry, o);
    const u32 sayer[3] = {0, 2, 1}, line[3] = {kSayA, kSayA, kSayB};  // +788, +796, +792
    for (u32 k = 0; k < 3; ++k)
        if (const u32 sp = o->spk[sayer[k]]; sp != 0) call(kSpeakerSay, gm<be_u32>(sp + 164).get(), line[k], 0u, C(8));

    ReleaseParts(o);
    call(kFieldSet, &o->f640, 0u);
    call(kPostReset, o);
    ReleaseParts(o);
    call(kFieldSet, &o->f640, 0u);
    if (const u32 p0 = o->parts[0]; p0 != 0) call(kSetU32_348, p0, kPartHash);
    if (const u32 a = o->parts[2]; a != 0) call(kSetU32_348, a, kAuxHash);
    call(kViewSet, &o->view812, 0u, static_cast<u32>(o->st665));
    call(kViewSet, &o->view824, 0u, static_cast<u32>(o->st665));
    call(kViewSet, &o->view812, 1u, 1u);
    call(kViewSet, &o->view824, 1u, 1u);
    vcall(&gm<u8>(o->x836), 88, 0u, 1u, 1u, 1u);
    call(kUsersUpdateFlags, global<be_u32>(kUsers).get(), 0u);
    const u32 base = guest(o);
    call(kFieldSet, base + 544, 1u);
    for (u32 off : {556u, 592u, 604u, 616u, 628u}) call(kFieldSet, base + off, 0u);
    if (const u32 c = o->climb; c != 0) call(kClimbFn, c, 0u);
    call(kAnimSetTarget, o, kAnimDefault, zero, 1u);
    for (u32 k = 0; k < 12; ++k)
        if (const u32 sl = o->slots[k]; sl != 0) gm<be_u32>(sl + 28) = 0u;
    if (const u32 t = o->cam; t != 0) {
        const MegaCamRef& r = gm<MegaCamRef>(t);
        if (call<u32>(kHasWord368, r.w8.get(), r.w0.get()) == 0) call(kCameraPushTarget, r.w8.get(), r.w0.get(), C(12), 0u);
        call(kAttachCam, o->cam.get(), 0u, zero, 1u);
    }
    for (u32 i = 0, off = 0; i < o->nsubs; ++i, off += 4) {
        vcall(&gm<u8>(gm<be_u32>(o->subs + off)), 40, 0u);
        const u32 e2 = gm<be_u32>(o->subs + off);
        if (gm<be_u32>(e2 + 172) & 0x40000u) vcall(&gm<u8>(e2), 40, 1u);
    }
    o->nsubs = 0;
    o->b1064 = 0;
    call(kPlayerNotify, o, 1u);
    if (const u32 p = FlaggedPlayer(); p != 0) {
        if (call<u32>(kObjNotify24, p) != 0) {
            u8& b = gm<u8>(call<u32>(kObjNotify24, FlaggedPlayer()) + 2338);
            b = static_cast<u8>(b | 8);
        }
    }
    call(Obj_SetEffectHash, o, kEffectHash);
    const u32 slot = o->slots[0];
    const Vec3& src = gm<Vec3>(FlaggedPlayer() + 144);  // a null player reads address 144 (quirk)
    const u32 x = bits(src.x), y = bits(src.y), z = bits(src.z);
    set_bits(o->p992.x, x);
    set_bits(o->p992.y, y);
    set_bits(o->p976.x, x);
    set_bits(o->p992.z, z);
    set_bits(o->p976.y, y);
    set_bits(o->p976.z, z);
    const u32 scale = global<be_u32>(kSlotScale);
    if (slot == 0) return;
    call(kSwitchSlot, o, 0u);
    gm<be_u32>(slot + 72) = scale;
    call(kAnimSetTarget, o, 0u, C(8), 0u);
}

}  // namespace sly4::obj

NATIVE(0030FD68, sly4::obj::MegaMode_Reset_0030FD68)
