// World_HandleEvent callees, part 13b: callees of the loader (hevent12_c,
// hevent12_g, hevent12_i) and of Obj_BaseConstruct35695C.
#include "hevent13.h"

using namespace sly4;
using namespace sly4::hevent13;

namespace {
// Level (as Loader_Stop2DD898): +512 Resource* (main), +528 parts {+0
// Holder*[] (Holder +0 Resource*), +8 u16 count}. Resource +32 s32 state
// (0 / 5 / 6: nothing to release), vtable +8 release(res, 1).
constexpr u32 kLvMain = 512, kLvParts = 528, kPartsData = 0, kPartsCount = 8;
constexpr u32 kResState = 32, kResVtRelease = 8;
bool res_idle(s32 st) { return st == 0 || st == 5 || st == 6; }

// UserBlock +8: the player Object* (vtable +84 stay(o, arg), +44 refresh).
constexpr u32 kUbPlayer = 8, kObjVtStay = 84, kObjVtRefresh = 44;
constexpr u32 kObjWorldObj = 160, kObjFlags192 = 192, kObjFlag192Bit3 = 0x8;
constexpr u32 kStayD87CC = 0x000D87CC;    // (Object*, arg) (name?)
constexpr u32 kWorldObjUpdateLod = 0x0057EE00;  // (WorldObj*, 0, bool) (symbols.txt)
constexpr u32 kObj12C968 = 0x0012C968;    // (Object*, u32, u32; f32) (name?)
constexpr u32 kObj129D94 = 0x00129D94;    // (Object*) (name?)
constexpr u32 kHalf488170 = 0x00488170;   // f32 0.5

// Group / scene / save slot (as Loader_Begin2E0008).
constexpr u32 kGrScenes = 716, kGrSceneCount = 720, kGrFlagIndex = 822, kSceneSize = 536;
constexpr u32 kScHandle = 468, kScIndex = 501, kScSaveByte = 502;
constexpr u32 kSvSceneState = 464, kSvGroupByte = 498;
constexpr u8 kSceneCurrent = 1, kSceneSeen = 2, kGroupUnset = 255;
constexpr u32 kUpgradeUnlockJob = 0x0049E038;  // (u32 job hash) (hevent13_a.cpp)

// Controller (120 bytes, as Obj_BaseConstruct35695C +28): +0 vtable, +4
// body, +8..+36 0, +40 u16 flags, +42 u8 phase, +44 f32, +52..+116 0, +68
// sub-container.
constexpr u32 kCtlVtable = 0x00B1F548;
constexpr u32 kCtlFlagsClear = 0x1D80, kCtlFlagsSet = 0xE260;
constexpr u32 kCtlPhaseCounter = 0x0106BC6C;  // u8, +1 per controller
constexpr u32 kCtlPhaseCount = 0x0106BCCC;    // s32: phases (at least 1)
constexpr u32 kCtlDefault44 = 0x0106BD18;     // f32
constexpr u32 kCtlList = 0x0106BC70;          // vector {+0 Ctl*[], +4 u16 capacity, +8 u16 count, +10 u16 high-water}
constexpr u32 kCtlContainerInit = 0x0076EBA0; // (container, 16, 0, Allocator*) (name?)
constexpr u32 kMemDefaultAllocator = 0x00550EE8;
}  // namespace

// u32 Level_Release2DA5E0(Level* lv) (name?): releases (vt +8, 1) the main
// resource unless it is idle (state 0/5/6), else the first part that is not
// idle. r3 is the release's result, or lv when nothing is released.
DECOMP(002DA5E0, Level_Release2DA5E0) {
    const u32 lv = arg<u32>(ctx, 0);
    const u32 main = load<u32>(lv + kLvMain);
    if (main != 0 && !res_idle(load<s32>(main + kResState))) {
        hevent::vcall(ctx, main, kResVtRelease, {main, 1});
        return;
    }
    for (u32 i = 0, off = 0;; ++i, off += 4) {
        const u32 parts = load<u32>(lv + kLvParts);
        const u32 n = parts != 0 ? load<u16>(parts + kPartsCount) : 0;
        if (i >= n) break;
        const u32 res = load<u32>(load<u32>(load<u32>(parts + kPartsData) + off));
        if (res == 0 || res_idle(load<s32>(res + kResState))) continue;
        hevent::vcall(ctx, res, kResVtRelease, {res, 1});
        return;
    }
    ret(ctx, lv);
}

// void User_Stay488178(World*, UserBlock* ub, u32 arg) (name?): the user's
// player (ub +8) stays: vt +84 (o, arg), 0x000D87CC (o, arg), its WorldObj's
// LOD update, vt +44, and with +192 bit 3 0x0012C968 (o, -1, -1; 0.5) and
// 0x00129D94 (o). r3 as the original leaves it.
DECOMP(00488178, User_Stay488178) {
    const u32 ub = arg<u32>(ctx, 1), a = arg<u32>(ctx, 2);
    const u32 p = load<u32>(ub + kUbPlayer);
    if (p == 0) return ret(ctx, 0u);
    hevent::vcall(ctx, p, kObjVtStay, {p, a});
    call(ctx, kStayD87CC, {load<u32>(ub + kUbPlayer), a});
    u32 o = load<u32>(ub + kUbPlayer);
    const u32 wo = load<u32>(o + kObjWorldObj);
    if (wo != 0) {
        call(ctx, kWorldObjUpdateLod, {wo, 0, 0});
        o = load<u32>(ub + kUbPlayer);
    }
    const u32 r = static_cast<u32>(hevent::vcall(ctx, o, kObjVtRefresh, {o}));
    const u32 q = load<u32>(ub + kUbPlayer);
    if (q == 0) return ret(ctx, r);
    if ((load<u32>(q + kObjFlags192) & kObjFlag192Bit3) == 0) return ret(ctx, 0u);
    callf(ctx, kObj12C968, {q, 0xFFFFFFFFu, 0xFFFFFFFFu}, {hevent7::lfs_f64(kHalf488170)});
    call(ctx, kObj129D94, {q});
}

// bool Group_Start2DB70C(Group* g) (name?): with the group's current scene
// index set in the save slot (+498 of its flags, 255: none), scenes at that
// index become current (state 1 unless seen; result true) and earlier or
// seen ones are marked seen and their job unlocked (0x0049E038).
DECOMP(002DB70C, Group_Start2DB70C) {
    const u32 g = arg<u32>(ctx, 0);
    u32 result = 0;
    u32 scenes = load<u32>(g + kGrScenes);
    if (scenes == 0) return ret(ctx, 0u);
    u32 idx = load<u8>(g + kGrFlagIndex);
    auto slot = []() {
        const u32 w = load<u32>(hevent::kCurrentWorld);
        return w + load<u8>(w + hevent::kUserBlocks) * hevent::kSaveSlotSize;
    };
    if (load<u8>(slot() + (idx << 2) + kSvGroupByte) == kGroupUnset) return ret(ctx, 0u);
    u32 n = load<u8>(g + kGrSceneCount);
    for (u32 j = 0, off = 0; j < n; ++j, off += kSceneSize) {
        const u32 sc = scenes + off;
        if (sc == 0) continue;
        const u32 base = slot();
        const u32 cur = (idx << 2) & 0x3FC;
        const s32 si = load<u8>(sc + kScIndex);
        const u32 st = base + load<u8>(sc + kScSaveByte) + hevent::kSaveData + kSvSceneState;
        if (si == static_cast<s32>(load<u8>(base + cur + kSvGroupByte))) {
            if (load<u8>(st) != kSceneSeen) store<u8>(st, kSceneCurrent);
            result = 1;
            continue;
        }
        if (load<u8>(st) != kSceneSeen && si >= static_cast<s32>(load<u8>(base + cur + kSvGroupByte))) continue;
        store<u8>(st, kSceneSeen);
        call(ctx, kUpgradeUnlockJob, {load<u32>(sc + kScHandle)});
        idx = load<u8>(g + kGrFlagIndex);
        scenes = load<u32>(g + kGrScenes);
        n = load<u8>(g + kGrSceneCount);
    }
    ret(ctx, result);
}

// u32 Ctl_Construct34BDEC(Ctl* c, Body* body) (name?): the 120-byte
// controller constructor; takes the next phase (a global u8 counter modulo
// the phase count), sets up the +68 container and appends c to the global
// controller list. Returns the list's previous high-water mark.
DECOMP(0034BDEC, Ctl_Construct34BDEC) {
    const u32 c = arg<u32>(ctx, 0), body = arg<u32>(ctx, 1);
    const u32 flags = load<u16>(c + 40);
    store<u32>(c + 4, body);
    store<u32>(c + 8, 0);
    store<u32>(c + 0, kCtlVtable);
    for (u32 o = 12; o <= 36; o += 4) store<u32>(c + o, 0);
    store<u16>(c + 40, static_cast<u16>((flags & ~kCtlFlagsClear) | kCtlFlagsSet));
    const u8 phase = load<u8>(kCtlPhaseCounter);
    const s32 nphase = load<s32>(kCtlPhaseCount);
    store<u8>(kCtlPhaseCounter, static_cast<u8>(phase + 1));
    const s32 div = nphase > 1 ? nphase : 1;
    store<u8>(c + 42, static_cast<u8>(static_cast<s32>(phase) % div));
    store<u32>(c + 52, 0);
    store<u32>(c + 44, load<u32>(kCtlDefault44));
    ctx.fpr[1] = hevent7::lfs_f64(kCtlDefault44);  // f1 as the lfs leaves it (the callees keep it)
    for (u32 o = 56; o <= 116; o += 4) store<u32>(c + o, 0);
    const u32 alloc = static_cast<u32>(call(ctx, kMemDefaultAllocator, {}));
    call(ctx, kCtlContainerInit, {c + 68, 16, 0, alloc});
    const u32 v = kCtlList;
    if (load<u16>(v + 8) == load<u16>(v + 4))
        call(ctx, hevent8::h8fn::PtrVector_Reserve, {v, (static_cast<u32>(load<u16>(v + 4)) << 1) & 0x1FFFE});
    store<u32>(load<u32>(v) + (static_cast<u32>(load<u16>(v + 8)) << 2), c);
    const u32 cnt = static_cast<u32>(load<u16>(v + 8)) + 1;
    const u32 hw = load<u16>(v + 10);
    store<u16>(v + 8, static_cast<u16>(cnt));
    store<u16>(v + 10, static_cast<u16>(static_cast<s32>(hw) > static_cast<s32>(cnt & 0xFFFF) ? hw : cnt));
    ret(ctx, hw);
}
