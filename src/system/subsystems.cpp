// System start-up: Main_InitSystems' first stage.
#include "subsystems.h"

#include "../memory/mem.h"

namespace sly4::system {
using namespace opentit::native;

namespace {

// sys_process_param_t of the EBOOT; its first word is the struct size.
constexpr u32 kProcessParam = 0x00F05100;

// ELF image layout, as printed by System_Init.
constexpr u32 kCodeStart = 0x00010000;
constexpr u32 kCodeEnd = 0x00ABF5E0;
constexpr u32 kDataStart = 0x00F557E0;
constexpr u32 kDataEnd = 0x00F9472C;  // = start of bss
constexpr u32 kBssEnd = 0x011F89B8;

constexpr u32 kSncVersion = 0xA029;     // "Versions:SNC:%d"
constexpr u32 kSdkVersion = 0x00410001;  // "Ps2Lib:%x" (the PS3 SDK version, 4.10)

constexpr u32 kMaxUsableSpu = 6, kMaxRawSpu = 0;

// cellSysmodule ids loaded by System_Init, in order.
constexpr u32 CELL_SYSMODULE_RTC = 0x0009;
constexpr u32 CELL_SYSMODULE_FS = 0x000E;
constexpr u32 CELL_SYSMODULE_GCM_SYS = 0x0010;
constexpr u32 CELL_SYSMODULE_IO = 0x0017;
constexpr u32 CELL_SYSMODULE_USBD = 0x001C;
constexpr u32 CELL_SYSMODULE_AVCONF_EXT = 0x0031;
constexpr u32 CELL_SYSMODULE_SYSUTIL_GAME = 0x003E;

// Strings (in the EBOOT).
constexpr u32 kStrProcessParam = 0x00AF42BC;  // "SYS_PROCESS_PARAM\n"
constexpr u32 kStrNewlines = 0x00AF3A30;      // "\n\n\n\n"
constexpr u32 kStrRule = 0x00AF3A38;          // "=====..."
constexpr u32 kStrVersions = 0x00AF3A90;      // "Versions:SNC:%d  Ps2Lib:%x\n"
constexpr u32 kStrTotal = 0x00AF3AB0;         // "Total: 0x%07x-0x%07x (%5dk)\n"
constexpr u32 kStrCode = 0x00AF3AD0;          // " Code: 0x%07x-0x%07x (%5dk)\n"
constexpr u32 kStrData = 0x00AF3AF0;          // " Data: 0x%07x-0x%07x (%5dk)\n"
constexpr u32 kStrBss = 0x00AF3B10;           // "  bss: 0x%07x-0x%07x (%5dk)\n"
constexpr u32 kStrTotalMb = 0x00AF3B30;       // "  totalUserMemory:       %3dMb\n"
constexpr u32 kStrAvailMb = 0x00AF3B50;       // "  available_user_memory: %3dMb\n"
constexpr u32 kStrSpuInit = 0x00AF42D4;       // "SPU Initialize\n"
constexpr u32 kStrApeCore = 0x00AF42A4;       // "ApeCore"
constexpr u32 kStrApeSpurs1 = 0x00AF42AC;     // "ApeSpurs_1SPU"
constexpr u32 kApeCoreLen = 7, kApeSpurs1Len = 13;  // strlen of the two

// SPURS set-up.
constexpr u32 CELL_SPURS_ATTRIBUTE_REVISION = 2;
constexpr u32 CELL_SPURS_INTERNAL_VERSION = 0x330000;
constexpr u32 SYS_SPU_THREAD_GROUP_TYPE_EXCLUSIVE_NON_CONTEXT = 0x18;
constexpr u32 CELL_SPURS_EVENT_FLAG_CLEAR_AUTO = 0;
constexpr u32 CELL_SPURS_EVENT_FLAG_SPU2PPU = 1;
constexpr u32 kSpursAlign = 128, kSpursSize = 4096;  // CellSpurs
constexpr s32 kSpursPool = 6;                         // a system (memalign) pool id
constexpr u32 kSpuPrintfPriority = 100;
constexpr u32 kCoreSpus = 5, kCorePriority = 100;
constexpr u32 k1SpuSpus = 1, k1SpuPriority = 101;
constexpr u32 kSpursObjects = 0x0116B580;            // statics used below (layout unknown)
constexpr u32 kCoreJobChain = kSpursObjects + 0x80;  // JobChain_Init(…, ApeCore, 5)
constexpr u32 kCoreEventFlag = kSpursObjects + 0x300;  // CellSpursEventFlag
constexpr u32 k1SpuJobChain = kSpursObjects + 0x380;   // JobChain_Init(…, ApeSpurs_1SPU, 1)
constexpr u32 kSpursAttributeSize = 512;  // CellSpursAttribute

constexpr u32 CELL_SYSUTIL_SYSTEMPARAM_ID_LANG = 0x0111;

template <typename... A>
void print(A... args) {
    call(libc::printf, args...);
}

// cellSysmoduleLoadModule(id), adding the user memory it took to
// MemStats::module_memory (as Np_Init does).
void load_module_counted(u32 id) {
    GuestFrame frame(2 * sizeof(MemoryInfo));
    MemoryInfo* before = frame.as<MemoryInfo>(0);
    MemoryInfo* after = before + 1;
    lv2::syscall(sc::sys_memory_get_user_memory_size, before);
    call(imp::cellSysmoduleLoadModule, id);
    lv2::syscall(sc::sys_memory_get_user_memory_size, after);
    memory::mem_stats().module_memory += before->available_user_memory - after->available_user_memory;
}

}  // namespace

// Main_InitSystems' first stage after recording the main thread: prints
// the memory map, initialises the SPUs, loads the system modules the game
// needs, starts SPURS and picks the game language. Always succeeds.
bool System_Init() {
    if (global<be_u32>(kProcessParam) == 0) print(kStrProcessParam);

    print(kStrNewlines);
    print(kStrRule);
    print(kStrVersions, kSncVersion, kSdkVersion);
    const u32 image_size = kBssEnd - kCodeStart;
    const u32 code_size = kCodeEnd - kCodeStart;
    print(kStrTotal, kCodeStart, kBssEnd, image_size >> 10);
    print(kStrCode, kCodeStart, kCodeEnd, code_size >> 10);
    print(kStrData, kDataStart, kDataEnd, (kDataEnd - kDataStart) >> 10);
    print(kStrBss, kDataEnd, kBssEnd, (kBssEnd - kDataEnd) >> 10);
    print(kStrRule);
    {
        GuestFrame frame(sizeof(MemoryInfo));
        MemoryInfo* info = frame.as<MemoryInfo>();
        lv2::syscall(sc::sys_memory_get_user_memory_size, info);
        print(kStrTotalMb, info->total_user_memory >> 20);
        print(kStrAvailMb, info->available_user_memory >> 20);
    }
    MemStats& stats = memory::mem_stats();
    stats.image_size = image_size;
    stats.code_size = code_size;
    stats.data_size = kBssEnd - kDataStart;

    print(kStrSpuInit);
    lv2::syscall(sc::sys_spu_initialize, kMaxUsableSpu, kMaxRawSpu);

    for (u32 id : {CELL_SYSMODULE_IO, CELL_SYSMODULE_USBD, CELL_SYSMODULE_GCM_SYS, CELL_SYSMODULE_FS,
                   CELL_SYSMODULE_AVCONF_EXT, CELL_SYSMODULE_RTC, CELL_SYSMODULE_SYSUTIL_GAME})
        load_module_counted(id);  // results unchecked

    Spurs_Init();
    *host<be_u32>(g::config + kConfigLanguage) = System_GetLanguage();
    return true;
}

// Starts the SPU printf server and two SPURS instances: "ApeCore" on 5
// SPUs (with an SPU->PPU event flag) and "ApeSpurs_1SPU" on 1, each with
// a job chain.
void Spurs_Init() {
    // Locals of the original: thread id, CellSpursAttribute, priority and the
    // two job chains' 8 SPU priorities.
    GuestFrame frame(8 + kSpursAttributeSize + 8 + 8 + 8);
    be_u64* thread_id = frame.as<be_u64>(0);
    u8* attr = frame.as<u8>(8);
    be_u32* priority = frame.as<be_u32>(8 + kSpursAttributeSize);
    u8* core_prios = frame.as<u8>(8 + kSpursAttributeSize + 8);
    u8* one_prios = core_prios + 8;

    // The PPU priority is looked up and decremented but never used.
    if (call<s32>(imp::sys_ppu_thread_get_id, thread_id) == 0 &&
        static_cast<s32>(lv2::syscall(sc::sys_ppu_thread_get_priority, thread_id->get(), priority)) == 0)
        *priority = *priority - 1;

    call(fn::spu_printf_initialize, kSpuPrintfPriority, 0u);

    MemStats& stats = memory::mem_stats();
    stats.spurs_core = guest(memory::Mem_AllocPool(kSpursAlign, kSpursSize, kSpursPool, 0));
    call(imp::_cellSpursAttributeInitialize, attr, CELL_SPURS_ATTRIBUTE_REVISION, CELL_SPURS_INTERNAL_VERSION, kCoreSpus,
         kCorePriority, kCorePriority, 0u);
    call(imp::cellSpursAttributeSetNamePrefix, attr, kStrApeCore, kApeCoreLen);
    call(imp::cellSpursAttributeEnableSpuPrintfIfAvailable, attr);
    call(imp::cellSpursAttributeSetSpuThreadGroupType, attr, SYS_SPU_THREAD_GROUP_TYPE_EXCLUSIVE_NON_CONTEXT);
    call(imp::cellSpursInitializeWithAttribute, stats.spurs_core.get(), attr);

    static constexpr u8 kCorePrios[8] = {8, 8, 8, 8, 8, 0, 0, 0};
    for (u32 k = 0; k < 8; ++k) core_prios[k] = kCorePrios[k];
    call(fn::JobChain_Init, kCoreJobChain, stats.spurs_core.get(), kCoreSpus, core_prios);
    call(imp::_cellSpursEventFlagInitialize, stats.spurs_core.get(), 0u, kCoreEventFlag, CELL_SPURS_EVENT_FLAG_CLEAR_AUTO,
         CELL_SPURS_EVENT_FLAG_SPU2PPU);
    call(imp::cellSpursEventFlagAttachLv2EventQueue, kCoreEventFlag);

    stats.spurs_1spu = guest(memory::Mem_AllocPool(kSpursAlign, kSpursSize, kSpursPool, 0));
    call(imp::_cellSpursAttributeInitialize, attr, CELL_SPURS_ATTRIBUTE_REVISION, CELL_SPURS_INTERNAL_VERSION, k1SpuSpus,
         k1SpuPriority, k1SpuPriority, 0u);
    call(imp::cellSpursAttributeSetNamePrefix, attr, kStrApeSpurs1, kApeSpurs1Len);
    call(imp::cellSpursInitializeWithAttribute, stats.spurs_1spu.get(), attr);

    static constexpr u8 k1SpuPrios[8] = {8, 0, 0, 0, 0, 0, 0, 0};
    for (u32 k = 0; k < 8; ++k) one_prios[k] = k1SpuPrios[k];
    call(fn::JobChain_Init, k1SpuJobChain, stats.spurs_1spu.get(), k1SpuSpus, one_prios);
}

// Maps the console's CELL_SYSUTIL_LANG_* setting to the game's language
// ids; unsupported languages (Chinese, Turkish, ...) fall back to English.
u32 System_GetLanguage() {
    s32 lang;
    {
        GuestFrame frame(8);
        be_s32* out = frame.as<be_s32>();
        *out = 1;  // CELL_SYSUTIL_LANG_ENGLISH_US if the call fails
        call(imp::cellSysutilGetSystemParamInt, CELL_SYSUTIL_SYSTEMPARAM_ID_LANG, out);
        lang = *out;
    }
    switch (lang) {
    case 0: return kLangJapanese;           // CELL_SYSUTIL_LANG_JAPANESE
    case 1: return kLangEnglish;            // ENGLISH_US
    case 2: return kLangFrench;             // FRENCH
    case 3: return kLangSpanish;            // SPANISH
    case 4: return kLangGerman;             // GERMAN
    case 5: return kLangItalian;            // ITALIAN
    case 6: return kLangDutch;              // DUTCH
    case 7: return kLangPortuguese;         // PORTUGUESE_PT
    case 8: return kLangRussian;            // RUSSIAN
    case 9: return kLangKorean;             // KOREAN
    case 12: return kLangFinnish;           // FINNISH
    case 13: return kLangSwedish;           // SWEDISH
    case 14: return kLangDanish;            // DANISH
    case 15: return kLangNorwegian;         // NORWEGIAN
    case 16: return kLangPolish;            // POLISH
    case 17: return kLangPortugueseBrazil;  // PORTUGUESE_BR
    case 18: return kLangEnglishUK;         // ENGLISH_GB
    default: return kLangEnglish;           // 10, 11 (Chinese), 19+ (Turkish, ...), negative
    }
}

}  // namespace sly4::system

NATIVE(0056D87C, sly4::system::System_Init)
NATIVE(0056D630, sly4::system::Spurs_Init)
NATIVE(0056BE30, sly4::system::System_GetLanguage)
