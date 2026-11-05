// World_HandleEvent callees, part 19a: small leaves under the hevent18
// functions (line lookup, component mode reset, trophy award, track lookup,
// mesh material lookup, world object rewind, animator set handles).
#include "hevent19.h"

using namespace sly4;
using namespace sly4::hevent;
using namespace sly4::hevent19;

namespace {
// The line map searched by Line_Find348ECC.
constexpr u32 kLineMap = 0x0106BC14;

// Component +384 u32: bits 15..22 mode (1..3 count as active), bits 7..14
// cleared on reset; +234 u8 active count; +304 u32 cleared on leaving an
// active mode.
constexpr u32 kCompModeWord = 384, kCompActive = 234, kComp304 = 304;
constexpr u32 kModeShift = 15, kModeMask = 0x007F8000u, kModeLowMask = 0x00007F80u;

// The award index (Array* of {u32 hash, s32 number}) and the trophy manager.
constexpr u32 kAwardIndex = 0x00F64624;
using hevent::kTrophy;

// Layer +52 u16 first track; track set +4 header (+2 u16 count), +12
// 76-byte tracks (the Track is at +4 of each).
constexpr u32 kLayerFirst = 52, kSetHeader = 4, kHeaderCount = 2, kSetTracks = 12, kTrackSize = 76;

// WorldObj +208 Mesh* (+112 u8 part count, 8-byte parts from +132 {Part*});
// Part +48 8-byte slots {+4 Material*}, +52 u16 slot count; Material +40
// s32 source.
constexpr u32 kWobjMesh = 208, kMeshPartCount = 112, kMeshParts = 132, kMeshPartSize = 8;
constexpr u32 kPartSlots = 48, kPartSlotCount = 52, kSlotMaterial = 4, kMatSource = 40;

// WorldObj +272 first child, +268 next sibling.
constexpr u32 kWobjChild = 272, kWobjSibling = 268;

// Animator +32 u32 current set handle; +360 u32 handles[], +368 u16 count.
// The handle object's +0 Set* (Set +24 u32 key).
constexpr u32 kAnimCurrent = 32, kAnimHandles = 360, kAnimHandleCount = 368, kSetKey = 24;
// The null handle returned on a miss (a function-local static: u8 guard at
// 0x01067AE0, u32 handle at 0x01067AE8).
constexpr u32 kNullGuard = 0x01067AE0, kNullHandle = 0x01067AE8;
}  // namespace

// Line* Line_Find348ECC(u32 hash) (name?): the line map's slot value for
// hash (the slot is never null-checked).
DECOMP(00348ECC, Line_Find348ECC) {
    Stack frame(ctx, 16);  // the key word (r1 +112 in the original frame)
    store<u32>(frame.addr(), arg<u32>(ctx, 0));
    const u32 slot = static_cast<u32>(call(ctx, h19fn::SortedMap_Find76D894, {kLineMap, frame.addr()}));
    ret(ctx, load<u32>(slot));
}

// Component* Comp_Reset254168(Component* c, s32 mode) (name?): sets the
// mode field (bits 15..22 of +384), keeping the +234 active count in step
// (modes 1..3 are active; leaving one also clears +304), and clears bits
// 7..14. Returns c.
DECOMP(00254168, Comp_Reset254168) {
    const u32 c = arg<u32>(ctx, 0);
    const s32 mode = arg<s32>(ctx, 1);
    u32 w = load<u32>(c + kCompModeWord);
    u32 active = load<u8>(c + kCompActive);
    const s32 old = static_cast<s32>((w >> kModeShift) & 0xFF);
    if (old > 0 && old <= 3) {
        active -= 1;
        store<u8>(c + kCompActive, static_cast<u8>(active));
        store<u32>(c + kComp304, 0);
    }
    if (mode > 0 && mode <= 3) {
        active += 1;
        store<u8>(c + kCompActive, static_cast<u8>(active));
    }
    w = (w & ~kModeMask) | ((static_cast<u32>(mode) << kModeShift) & kModeMask);
    w &= ~kModeLowMask;
    store<u32>(c + kCompModeWord, w);
    ret(ctx, c);
}

// void Trophy_Award0B27B4(u32 hash) (name?): unlocks the trophy whose award
// number the award index gives for hash (-1: none). Hash_ToString is called
// first with the result unused (a stripped debug print).
DECOMP(000B27B4, Trophy_Award0B27B4) {
    const u32 hash = arg<u32>(ctx, 0);
    Stack frame(ctx, 16);  // the key word (r1 +112)
    store<u32>(frame.addr(), hash);
    call(ctx, h19fn::Hash_ToString, {hash});
    const u32 slot = static_cast<u32>(call(ctx, h19fn::SortedMap_Find742EF4, {load<u32>(kAwardIndex), frame.addr()}));
    const s32 number = static_cast<s32>(load<u32>(slot));
    if (number == -1) {
        ret(ctx, number);
        return;
    }
    call(ctx, h19fn::Trophy_Unlock4FB4A0, {load<u32>(kTrophy), static_cast<u32>(number)});
}

// Track* Track_Find4B3624(Layer* l, s16 index, TrackSet* s) (name?): the
// layer's track index (first + index) in the set, or 0 (no set, out of
// range).
DECOMP(004B3624, Track_Find4B3624) {
    const u32 l = arg<u32>(ctx, 0), s = arg<u32>(ctx, 2);
    if (s == 0) {
        ret(ctx, 0u);
        return;
    }
    const s32 i = static_cast<s16>(static_cast<u16>(load<u16>(l + kLayerFirst) + arg<u32>(ctx, 1)));
    u32 p = 0;
    if (i >= 0 && i < static_cast<s32>(load<u16>(load<u32>(s + kSetHeader) + kHeaderCount)))
        p = load<u32>(s + kSetTracks) + static_cast<u32>(i) * kTrackSize;
    ret(ctx, p != 0 ? p + 4 : 0u);
}

// Material* WorldObj_Material4A59B4(WorldObj* w, s32 source) (name?): the
// first material of the mesh's parts whose source (+40) is source, or 0.
DECOMP(004A59B4, WorldObj_Material4A59B4) {
    const u32 mesh = load<u32>(arg<u32>(ctx, 0) + kWobjMesh);
    const s32 source = arg<s32>(ctx, 1);
    const u32 n = load<u8>(mesh + kMeshPartCount);
    for (u32 k = 0; k < n; ++k) {
        const u32 part = load<u32>(mesh + kMeshParts + k * kMeshPartSize);
        for (u32 j = 0; j < load<u16>(part + kPartSlotCount); ++j) {
            const u32 m = load<u32>(load<u32>(part + kPartSlots) + j * 8 + kSlotMaterial);
            if (m != 0 && static_cast<s32>(load<u32>(m + kMatSource)) == source) {
                ret(ctx, m);
                return;
            }
        }
    }
    ret(ctx, 0u);
}

// void Obj_Prepare0D2E68(WorldObj* w, u8 children) (name?): rewinds the
// peer's animations and, when children is set, does the same for every
// child (recursively). r3 is left as the last call returned it.
DECOMP(000D2E68, Obj_Prepare0D2E68) {
    const u32 w = arg<u32>(ctx, 0);
    const u32 children = arg<u8>(ctx, 1);
    if (w == 0) return;  // r3 = 0
    const u32 peer = static_cast<u32>(call(ctx, h19fn::WorldObj_GetPeer, {w}));
    if (peer != 0) call(ctx, h19fn::Peer_RewindAnims, {peer});  // r4 as GetPeer left it
    if (children == 0) return;
    for (u32 c = load<u32>(w + kWobjChild); c != 0; c = load<u32>(c + kWobjSibling))
        call(ctx, 0x000D2E68, {c, children});
}

// u32 Anim_CurrentSet297E3C(Animator* a) (name?): the key (+24) of the
// current set handle's (+32) Set, or 0 for a stale handle (the handle
// object's +0 Set is not null-checked).
DECOMP(00297E3C, Anim_CurrentSet297E3C) {
    const u32 obj = handle_object(load<u32>(kAnimPool), load<u32>(arg<u32>(ctx, 0) + kAnimCurrent));
    ret(ctx, obj != 0 ? load<u32>(load<u32>(obj) + kSetKey) : 0u);
}

// u32* Anim_Handle254438(u32* out, Animator* a, u32 key) (name?): the
// animator's set handle whose Set has key (+24), else the null handle.
// Returns out.
DECOMP(00254438, Anim_Handle254438) {
    const u32 out = arg<u32>(ctx, 0), a = arg<u32>(ctx, 1), key = arg<u32>(ctx, 2);
    if (load<u8>(kNullGuard) == 0) {
        store<u32>(kNullHandle, 0);
        store<u16>(kNullHandle + 2, 0);
        store<u8>(kNullGuard, 1);
    }
    for (u32 i = 0; i < load<u16>(a + kAnimHandleCount); ++i) {
        const u32 data = load<u32>(a + kAnimHandles);
        const u32 obj = handle_object(load<u32>(kAnimPool), load<u32>(data + 4 * i));
        if (obj == 0) continue;
        const u32 set = load<u32>(obj);
        if (set == 0 || load<u32>(set + kSetKey) != key) continue;
        store<u32>(out, load<u32>(data + 4 * i));
        ret(ctx, out);
        return;
    }
    store<u32>(out, load<u32>(kNullHandle));
    ret(ctx, out);
}

// RecSlot* Track_Slot38B118(const RecSlot* s, Base* b) (name?): the slot
// (+4) of the record that s's +2 index (+6 of the record) names, or 4 (the
// slot of a null record) when the index is out of range or the record's
// kind is 10 or more.
DECOMP(0038B118, Track_Slot38B118) {
    const u32 s = arg<u32>(ctx, 0), b = arg<u32>(ctx, 1);
    const s32 i = static_cast<s16>(load<u16>(s + 6));
    u32 rec = 0;
    if (i < static_cast<s32>(load<u16>(b + 144))) {
        rec = b + 152 + static_cast<u32>(i) * 132;
        if (!(static_cast<s32>(load<u32>(rec)) < 10)) rec = 0;
    }
    ret(ctx, rec + 4);
}
