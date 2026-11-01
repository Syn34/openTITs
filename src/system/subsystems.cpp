// System start-up: Main_InitSystems' first stage.
#include "subsystems.h"

#include "../memory/mem.h"

using namespace sly4;

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

void print(PPUContext& ctx, std::initializer_list<u64> args) { call(ctx, libc::printf, args); }

// cellSysmoduleLoadModule(id), adding the user memory it took to
// MemStats::module_memory (as Np_Init does).
void load_module_counted(PPUContext& ctx, u32 id) {
    Stack frame(ctx, 2 * sizeof(MemoryInfo));
    const u32 before = frame.addr(), after = frame.addr() + sizeof(MemoryInfo);
    syscall(ctx, sc::sys_memory_get_user_memory_size, {before});
    call(ctx, imp::cellSysmoduleLoadModule, {id});
    syscall(ctx, sc::sys_memory_get_user_memory_size, {after});
    ref<MemStats>(g::mem_stats).module_memory +=
        ref<MemoryInfo>(before).available_user_memory - ref<MemoryInfo>(after).available_user_memory;
}

}  // namespace

// bool System_Init()
// Main_InitSystems' first stage after recording the main thread: prints
// the memory map, initialises the SPUs, loads the system modules the game
// needs, starts SPURS and picks the game language. Always succeeds.
DECOMP(0056D87C, System_Init) {
    if (load<u32>(kProcessParam) == 0) print(ctx, {kStrProcessParam});

    print(ctx, {kStrNewlines});
    print(ctx, {kStrRule});
    print(ctx, {kStrVersions, kSncVersion, kSdkVersion});
    const u32 image_size = kBssEnd - kCodeStart;
    const u32 code_size = kCodeEnd - kCodeStart;
    print(ctx, {kStrTotal, kCodeStart, kBssEnd, image_size >> 10});
    print(ctx, {kStrCode, kCodeStart, kCodeEnd, code_size >> 10});
    print(ctx, {kStrData, kDataStart, kDataEnd, (kDataEnd - kDataStart) >> 10});
    print(ctx, {kStrBss, kDataEnd, kBssEnd, (kBssEnd - kDataEnd) >> 10});
    print(ctx, {kStrRule});
    {
        Stack frame(ctx, sizeof(MemoryInfo));
        syscall(ctx, sc::sys_memory_get_user_memory_size, {frame.addr()});
        const MemoryInfo& info = ref<MemoryInfo>(frame.addr());
        print(ctx, {kStrTotalMb, info.total_user_memory >> 20});
        print(ctx, {kStrAvailMb, info.available_user_memory >> 20});
    }
    MemStats& stats = ref<MemStats>(g::mem_stats);
    stats.image_size = image_size;
    stats.code_size = code_size;
    stats.data_size = kBssEnd - kDataStart;

    print(ctx, {kStrSpuInit});
    syscall(ctx, sc::sys_spu_initialize, {kMaxUsableSpu, kMaxRawSpu});

    for (u32 id : {CELL_SYSMODULE_IO, CELL_SYSMODULE_USBD, CELL_SYSMODULE_GCM_SYS, CELL_SYSMODULE_FS,
                   CELL_SYSMODULE_AVCONF_EXT, CELL_SYSMODULE_RTC, CELL_SYSMODULE_SYSUTIL_GAME})
        load_module_counted(ctx, id);  // results unchecked

    call(ctx, fn::Spurs_Init);
    const u32 language = static_cast<u32>(call(ctx, fn::System_GetLanguage));
    field<u32>(g::config, kConfigLanguage) = language;
    ret(ctx, 1u);
}

// void Spurs_Init()
// Starts the SPU printf server and two SPURS instances: "ApeCore" on 5
// SPUs (with an SPU->PPU event flag) and "ApeSpurs_1SPU" on 1, each with
// a job chain.
DECOMP(0056D630, Spurs_Init) {
    // Locals of the original: thread id, CellSpursAttribute, priority and the
    // two job chains' 8 SPU priorities.
    Stack frame(ctx, 8 + kSpursAttributeSize + 8 + 8 + 8);
    const u32 thread_id = frame.addr();
    const u32 attr = thread_id + 8;
    const u32 priority = attr + kSpursAttributeSize;
    const u32 core_prios = priority + 8;
    const u32 one_prios = core_prios + 8;

    // The PPU priority is looked up and decremented but never used.
    if (static_cast<s32>(call(ctx, imp::sys_ppu_thread_get_id, {thread_id})) == 0 &&
        static_cast<s32>(syscall(ctx, sc::sys_ppu_thread_get_priority, {load<u64>(thread_id), priority})) == 0)
        store<u32>(priority, load<u32>(priority) - 1);

    call(ctx, fn::spu_printf_initialize, {kSpuPrintfPriority, 0});

    MemStats& stats = ref<MemStats>(g::mem_stats);
    stats.spurs_core =
        static_cast<u32>(call(ctx, fn::Mem_AllocPool, {kSpursAlign, kSpursSize, kSpursPool, 0}));
    call(ctx, imp::_cellSpursAttributeInitialize, {attr, CELL_SPURS_ATTRIBUTE_REVISION, CELL_SPURS_INTERNAL_VERSION,
                                                   kCoreSpus, kCorePriority, kCorePriority, 0});
    call(ctx, imp::cellSpursAttributeSetNamePrefix, {attr, kStrApeCore, kApeCoreLen});
    call(ctx, imp::cellSpursAttributeEnableSpuPrintfIfAvailable, {attr});
    call(ctx, imp::cellSpursAttributeSetSpuThreadGroupType, {attr, SYS_SPU_THREAD_GROUP_TYPE_EXCLUSIVE_NON_CONTEXT});
    call(ctx, imp::cellSpursInitializeWithAttribute, {stats.spurs_core, attr});

    static constexpr u8 kCorePrios[8] = {8, 8, 8, 8, 8, 0, 0, 0};
    mem::write_bytes(core_prios, kCorePrios, sizeof(kCorePrios));
    call(ctx, fn::JobChain_Init, {kCoreJobChain, stats.spurs_core, kCoreSpus, core_prios});
    call(ctx, imp::_cellSpursEventFlagInitialize, {stats.spurs_core, 0, kCoreEventFlag,
                                                   CELL_SPURS_EVENT_FLAG_CLEAR_AUTO, CELL_SPURS_EVENT_FLAG_SPU2PPU});
    call(ctx, imp::cellSpursEventFlagAttachLv2EventQueue, {kCoreEventFlag});

    stats.spurs_1spu =
        static_cast<u32>(call(ctx, fn::Mem_AllocPool, {kSpursAlign, kSpursSize, kSpursPool, 0}));
    call(ctx, imp::_cellSpursAttributeInitialize, {attr, CELL_SPURS_ATTRIBUTE_REVISION, CELL_SPURS_INTERNAL_VERSION,
                                                   k1SpuSpus, k1SpuPriority, k1SpuPriority, 0});
    call(ctx, imp::cellSpursAttributeSetNamePrefix, {attr, kStrApeSpurs1, kApeSpurs1Len});
    call(ctx, imp::cellSpursInitializeWithAttribute, {stats.spurs_1spu, attr});

    static constexpr u8 k1SpuPrios[8] = {8, 0, 0, 0, 0, 0, 0, 0};
    mem::write_bytes(one_prios, k1SpuPrios, sizeof(k1SpuPrios));
    ctx.gpr[3] = call(ctx, fn::JobChain_Init, {k1SpuJobChain, stats.spurs_1spu, k1SpuSpus, one_prios});
}

// GameLanguage System_GetLanguage()
// Maps the console's CELL_SYSUTIL_LANG_* setting to the game's language
// ids; unsupported languages (Chinese, Turkish, ...) fall back to English.
DECOMP(0056BE30, System_GetLanguage) {
    s32 lang;
    {
        Stack frame(ctx, 8);
        store<s32>(frame.addr(), 1);  // CELL_SYSUTIL_LANG_ENGLISH_US if the call fails
        call(ctx, imp::cellSysutilGetSystemParamInt, {CELL_SYSUTIL_SYSTEMPARAM_ID_LANG, frame.addr()});
        lang = load<s32>(frame.addr());
    }
    u32 result;
    switch (lang) {
    case 0: result = kLangJapanese; break;           // CELL_SYSUTIL_LANG_JAPANESE
    case 1: result = kLangEnglish; break;            // ENGLISH_US
    case 2: result = kLangFrench; break;             // FRENCH
    case 3: result = kLangSpanish; break;            // SPANISH
    case 4: result = kLangGerman; break;             // GERMAN
    case 5: result = kLangItalian; break;            // ITALIAN
    case 6: result = kLangDutch; break;              // DUTCH
    case 7: result = kLangPortuguese; break;         // PORTUGUESE_PT
    case 8: result = kLangRussian; break;            // RUSSIAN
    case 9: result = kLangKorean; break;             // KOREAN
    case 12: result = kLangFinnish; break;           // FINNISH
    case 13: result = kLangSwedish; break;           // SWEDISH
    case 14: result = kLangDanish; break;            // DANISH
    case 15: result = kLangNorwegian; break;         // NORWEGIAN
    case 16: result = kLangPolish; break;            // POLISH
    case 17: result = kLangPortugueseBrazil; break;  // PORTUGUESE_BR
    case 18: result = kLangEnglishUK; break;         // ENGLISH_GB
    default: result = kLangEnglish; break;           // 10, 11 (Chinese), 19+ (Turkish, ...), negative
    }
    ret(ctx, result);
}
