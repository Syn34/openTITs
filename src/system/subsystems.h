// Subsystems started by Main_InitSystems (src/system/init.cpp).
#pragma once

#include "opentit/native.h"
#include "../sly4.h"

namespace sly4 {

namespace imp {
// Import stubs used by the system start-up (see funcinfo "calls").
constexpr u32 cellSysutilGetSystemParamInt = 0x00ABDAA4;
constexpr u32 cellSpursAttributeSetNamePrefix = 0x00ABDF24;
constexpr u32 cellSpursAttributeEnableSpuPrintfIfAvailable = 0x00ABDF44;
constexpr u32 _cellSpursEventFlagInitialize = 0x00ABE044;
constexpr u32 cellSpursEventFlagAttachLv2EventQueue = 0x00ABE0C4;
constexpr u32 _cellSpursAttributeInitialize = 0x00ABE0E4;
constexpr u32 cellSpursAttributeSetSpuThreadGroupType = 0x00ABE184;
constexpr u32 cellSpursInitializeWithAttribute = 0x00ABE1A4;
}  // namespace imp

namespace sc {
constexpr u32 sys_ppu_thread_get_priority = 48;  // (u64 thread_id, s32* prio)
constexpr u32 sys_spu_initialize = 169;          // (u32 max_usable_spu, u32 max_raw_spu)
}  // namespace sc

namespace fn {
// Main_InitSystems' stages (System_Init, Platform_Init, Graphics_Init, Resources_Init,
// Debug_Init) are named in sly4.h.
constexpr u32 Iff_RegisterChunk = 0x0055902C;  // (u32 fourcc, handler, ...) "Replacing %c%c%c%c (%s) with %s" (name?)
// Decompiled in src/system/subsystems.cpp.
constexpr u32 Spurs_Init = 0x0056D630;
constexpr u32 System_GetLanguage = 0x0056BE30;
// Still recompiled.
constexpr u32 spu_printf_initialize = 0x007265F0;  // (s32 priority, handler) SDK library
constexpr u32 JobChain_Init = 0x0056D3B0;          // (JobChain*, CellSpurs*, u32 nSpus, const u8 prio[8]) (name?)
}  // namespace fn

// The game's language ids (what System_GetLanguage returns; kept in g::config +0x28).
enum GameLanguage : u32 {
    kLangEnglish = 0,
    kLangFrench = 1,
    kLangItalian = 2,
    kLangGerman = 3,
    kLangSpanish = 4,
    kLangDutch = 5,
    kLangPortuguese = 6,
    kLangFinnish = 7,
    kLangSwedish = 8,
    kLangDanish = 9,
    kLangNorwegian = 10,
    kLangPolish = 11,
    kLangRussian = 12,
    kLangEnglishUK = 16,
    kLangPortugueseBrazil = 17,
    kLangJapanese = 18,
    kLangKorean = 19,
};
constexpr u32 kConfigLanguage = 0x28;  // g::config field written by System_Init

// Native functions (src/system).
namespace system {
bool System_Init();
void Spurs_Init();
u32 System_GetLanguage();
bool Main_InitSystems(u32 stages);
bool Np_Init(u32 pool_size);
s32 Sysutil_CheckCallback();
s32 Sysutil_RegisterCallback(s32 slot, u32 func, u32 userdata);
s32 Game_BootCheck();
}  // namespace system

}  // namespace sly4
