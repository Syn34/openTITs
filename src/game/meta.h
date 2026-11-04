// Trophies, upgrades and awards: the meta-game objects World_Init brings up
// (src/game/meta_trophy.cpp, src/game/meta_scripts.cpp). Guest addresses in
// the BCES01284 v01.02 EBOOT; "?" marks a guess.
#pragma once

#include "../sly4.h"

namespace sly4 {
namespace meta {

// Virtual call. The original's call sequence leaves the OPD and its entry
// in the two registers after the arguments; we do the same.
inline u64 mcall(PPUContext& ctx, u32 obj, u32 slot, std::initializer_list<u64> args) {
    const u32 opd = load<u32>(load<u32>(obj) + slot);
    ctx.gpr[3 + args.size()] = opd;
    ctx.gpr[4 + args.size()] = load<u32>(opd);
    return call_ptr(ctx, opd, args);
}

namespace mfn {
constexpr u32 Trophy_Construct = 0x004FB0F0;   // decompiled in meta_trophy.cpp
constexpr u32 Upgrades_Load = 0x0049DC70;      // decompiled in meta_scripts.cpp
constexpr u32 Awards_Load = 0x000B2280;        // "

constexpr u32 Lock_Init = 0x0079BA10;          // (lock, u32 32) (name?)
constexpr u32 Mem_DefaultAllocator = 0x00550EE8;  // () -> Allocator* (vtable +0 alloc(size, 0, align), +8 free(p, size)) (name?)
constexpr u32 Array_Init = 0x00558F38;         // (Array*): an empty function
constexpr u32 Array_Reserve = 0x00558F3C;      // (Array*, u16 capacity)
constexpr u32 Array_Destruct = 0x0054EC94;     // (Array*) (name?)
constexpr u32 Event_Init = 0x005E6D1C;         // (event, 0) (name?)
constexpr u32 ThreadAttr_Init = 0x005E6944;    // (attr): copies the 32-byte defaults
constexpr u32 ThreadAttr_Destroy = 0x005E69A4; // (attr) (name?)
constexpr u32 Thread_Create = 0x005E7824;      // (thread, attr, entry opd, arg) (name?)
constexpr u32 Mem_Alloc = 0x0054ED38;          // (size, pool, align)
constexpr u32 Mem_FreeFrom = 0x0054EE6C;       // (p, pool)
constexpr u32 String_Copy = 0x00553684;        // (dst, src, size)
constexpr u32 qsort = 0x0071EC28;              // (base, count, size, compare opd)
constexpr u32 Hash_ToString = 0x0056A6E0;       // (u32 hash) -> "0x%x" (String_Format)
}  // namespace mfn

// sceNpTrophy import stubs.
namespace np {
constexpr u32 sceNpTrophyInit = 0x00ABE644;
constexpr u32 sceNpTrophyCreateContext = 0x00ABE704;
constexpr u32 sceNpTrophyCreateHandle = 0x00ABE5E4;
constexpr u32 kMemContainerInvalid = 0xFFFFFFFFu;  // SYS_MEMORY_CONTAINER_ID_INVALID
}  // namespace np

constexpr u32 kCompareU32 = 0x00F31078;  // OPD of 0x0074307C: the sorted arrays' compare (name?)

// Array (sorted): +0 u16 capacity, +2 u16 count, +4 u16 high-water mark,
// +6 u16 (element size << 1) | 1 while unsorted, +8 data, +12 allocator,
// +16 u32 (-1 / 0), +20 compare OPD.
constexpr u32 kArrCapacity = 0, kArrCount = 2, kArrHigh = 4, kArrFlags = 6, kArrData = 8;
constexpr u32 kArrAllocator = 12, kArr16 = 16, kArrCompare = 20;
constexpr u32 kAllocVfnAlloc = 0, kAllocVfnFree = 8;

// ---------------------------------------------------------------- trophies
// Trophy manager (1400 bytes, global at 0x00F668E8).
constexpr u32 kTrophy = 0x00F668E8;
constexpr u32 kTrCommId = 0, kTrSignature = 4;        // const SceNpCommunicationId*, signature*
constexpr u32 kTrFlag11 = 11;                         // u8 from the last argument
constexpr u32 kTrLock = 24;                           // given to the thread as its argument
constexpr u32 kTrEvent60 = 60, kTrThread = 88, kTrEvent112 = 112;
constexpr u32 kTrCallback = 104;                      // the OPD argument (status callback?)
constexpr u32 kTrState = 108;                         // u32, 2 at construction (name?)
constexpr u32 kTrContext = 156, kTrHandle = 160;      // SceNpTrophyContext / Handle
constexpr u32 kTrList = 1376;                         // Array of 8-byte entries
constexpr u32 kTrThreadOpd = 0x00F3B2B0;              // OPD of 0x004FB704: the trophy thread (name?)
constexpr u32 kTrThreadName = 0x00AF0ED0;             // "TrophyThread"
constexpr u32 kTrThreadStack = 800;                   // ThreadAttr +4 (name?)

// ---------------------------------------------------------------- upgrades
constexpr u32 kUpgradeCount = 0x010C1A9C;   // u32
constexpr u32 kUpgrades = 0x010BE8BC;       // Upgrade[84], 152 bytes each
constexpr u32 kUpgradeMax = 84;
constexpr u32 kUpgradeSize = 152;
// Upgrade: +0 ID, +4 AbilityId, +8 Type (hashes), +12 s32 Cost, +16 char
// TitleId[64], +80 char DescId[64], +144 JobUnlockId hash, +148 u8 (bool, name unknown).
constexpr u32 kUpID = 0, kUpAbility = 4, kUpType = 8, kUpCost = 12, kUpTitle = 16, kUpDesc = 80;
constexpr u32 kUpJobUnlock = 144, kUpBool = 148;
constexpr u32 kSaveUpgradeFlags = 3948;     // World save data (+80) + 3948: u8 per upgrade (bits 0, 1 cleared)

// String_HashNoCase of the tag and attribute names.
constexpr u32 kHashUpgrades = 0x838099D7, kHashUpgrade = 0x1D0425FC;
constexpr u32 kHashID = 0x0000091B, kHashAbilityId = 0x08F12FA5, kHashType = 0x0027873A, kHashCost = 0x001FA7CD;
constexpr u32 kHashJobUnlockId = 0xC87D4DFC, kHashTitleId = 0xDCE08553, kHashDescId = 0x77FC9F6C;
constexpr u32 kHashUpgradeBool = 0xC5E921C2;  // (name unknown)

// ---------------------------------------------------------------- awards
constexpr u32 kAwardIndex = 0x00F64624;     // Array* of {u32 name hash, s32 number}, sorted by hash
constexpr u32 kAwardTable = 0x00F64620;     // {u32 name hash, u32 points}[number + 1]
constexpr u32 kAwardCount = 0x01013790;     // u32: highest Number + 1
constexpr u32 kHashAwards = 0x73D3C5B6, kHashAward = 0x03BC823D;
constexpr u32 kHashName = 0x0024728B, kHashNumber = 0x89EBCF49, kHashTrophyGrade = 0xCEBBEB87;
constexpr u32 kHashBronze = 0x7548799A, kHashSilver = 0x91CAC70D, kHashGold = 0x00217860, kHashPlatinum = 0x9E64B22C;
constexpr u32 kPointsBronze = 15, kPointsSilver = 30, kPointsGold = 90;  // platinum and others: 0

constexpr u32 kCurrentWorld = 0x00F65CA0;   // u32 World*
constexpr u32 kWorldSaveData = 80;
constexpr u32 kXmlDocSize = 2240;

}  // namespace meta
}  // namespace sly4
