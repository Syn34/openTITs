// Shared declarations for the Sly Cooper: Thieves in Time decompilation
// (BCES01284 v01.02 EBOOT). Addresses are guest addresses in that EBOOT.
//
// Names here are ours: the game ships without symbols. A name followed by
// "?" in a comment is a guess from how the code is used.
#pragma once

#include "opentit/decomp.h"

namespace sly4 {

using namespace opentit;
using namespace opentit::decomp;

// Import stubs: calling one runs the HLE (or LLE module) implementation.
namespace imp {
constexpr u32 cellSysmoduleLoadModule = 0x00ABD944;
constexpr u32 cellSysutilCheckCallback = 0x00ABD9C4;
constexpr u32 cellSysutilRegisterCallback = 0x00ABDB64;
constexpr u32 sceNpInit = 0x00ABE424;
constexpr u32 sys_lwmutex_lock = 0x00ABF0E4;
constexpr u32 sys_lwmutex_unlock = 0x00ABF104;
constexpr u32 sys_lwmutex_create = 0x00ABF1C4;
constexpr u32 sys_ppu_thread_get_id = 0x00ABF1E4;
}  // namespace imp

// LV2 system calls made directly with `sc`.
namespace sc {
constexpr u32 sys_memory_get_user_memory_size = 352;  // (sys_memory_info_t* out)
}

// Game functions that are still recompiled code.
namespace fn {
// C library
constexpr u32 memset = 0x00726E54;
constexpr u32 exit = 0x0071B570;
constexpr u32 _exit = 0x0071F460;
constexpr u32 String_Copy = 0x00553684;   // (char* dst, const char* src, u32 size): always terminates
constexpr u32 String_HashNoCase = 0x0056A710;  // (const char*) -> u32: h = h * 31 + toupper(c)
// Memory
constexpr u32 Mem_Alloc = 0x0054ED38;  // (u32 size, u32 pool, u32 align) -> pointer or 0
constexpr u32 Mem_Free = 0x005611B0;   // (void*)
constexpr u32 Mem_New = 0x007A3DD8;    // operator new(size): pool 0, align 16
// Files and archives
constexpr u32 File_Open = 0x0055BD3C;        // (const char* path, const char* mode) -> File* or 0
constexpr u32 File_LoadWhole = 0x00560B90;   // (const char* path, u32* size_out) -> buffer (name?)
constexpr u32 Archive_Mount = 0x00562944;    // (const char* path, MountOptions by value)  (name?)
constexpr u32 Archive_OpenZip = 0x00567034;  // (void* data, u32 size, const char* name) -> Archive*  (name?)
// Start-up
constexpr u32 Main_ParseCommandLine = 0x0046320C;   // (argc, argv)
constexpr u32 Main_InitSystems = 0x005DB1D0;  // (int stage): 1, then -2 once the shader archive is up
// Game
constexpr u32 Game_Load = 0x00463000;      // (Game*): Scripts/Tuning.xml, cursors
constexpr u32 Game_Update = 0x0045B858;    // (Game*) -> next main state; FrontEnd / InGame
constexpr u32 Game_Shutdown = 0x00458940;  // (Game*)  (name?)
// Not named yet: called from main.
constexpr u32 Heap_Construct = 0x0054ECA0;  // LinearHeap(this, size, pool, owns)
constexpr u32 Graphics_Init = 0x0050F520;
constexpr u32 Platform_Init = 0x0055DC18;
constexpr u32 Debug_Init = 0x0056A944;
constexpr u32 System_Init = 0x0056D87C;
constexpr u32 Resources_Init = 0x00588828;
constexpr u32 sub_00324A48 = 0x00324A48;  // (const char*) -> u32; parses a "Scale..." string
constexpr u32 sub_000AC0F0 = 0x000AC0F0;
constexpr u32 sub_000AC478 = 0x000AC478;
constexpr u32 sub_00458BAC = 0x00458BAC;
constexpr u32 sub_004A8AC0 = 0x004A8AC0;
constexpr u32 sub_004A8AC4 = 0x004A8AC4;
constexpr u32 sub_004FD214 = 0x004FD214;
constexpr u32 sub_0051096C = 0x0051096C;
constexpr u32 sub_005124F4 = 0x005124F4;
constexpr u32 sub_0051265C = 0x0051265C;
constexpr u32 sub_0051299C = 0x0051299C;
constexpr u32 sub_005129BC = 0x005129BC;
constexpr u32 sub_00512BE0 = 0x00512BE0;
constexpr u32 sub_00596A8C = 0x00596A8C;
}  // namespace fn

// Globals.
namespace g {
constexpr u32 config = 0x010C5A20;         // start-up configuration filled in by main (layout mostly unknown)
constexpr u32 game = 0x010B19E0;           // the Game object
constexpr u32 sysutil_flags = 0x010BB9F8;  // u32, set by Sysutil_OnEvent (see SysutilFlag)
constexpr u32 main_state = 0x00F65BF0;     // u32 MainState
constexpr u32 vblank_count = 0x010F0E00;   // u32, bumped by the VBlank handler (0x00532D3C)
constexpr u32 sysutil_mutex = 0x0116B288;  // sys_lwmutex_t serialising cellSysutil callback calls
constexpr u32 file_flags = 0x01164CE0;     // u8; bits 5-6: depth counter around file probes
constexpr u32 mem_stats = 0x010F0E20;      // memory accounting block (layout mostly unknown)
constexpr u32 np_pool = 0x00F8875C;        // u32: memory pool given to sceNpInit
constexpr u32 np_pool_size = 0x00F88760;   // u32
}  // namespace g

// A function-local static of the original (0x011DE094, guard byte at
// 0x011DE280), built by whichever start-up function reaches it first. It
// holds a copy of g::config float +0x14 at +12 and the main heap at +28.
// SystemState is our name for it.
namespace system_state {
constexpr u32 object = 0x011DE094, guard = 0x011DE280, registry_slot = 0x011972E8;

inline u32 get(PPUContext& ctx) {
    if (ref<u8>(guard) == 0) {
        field<u32>(object, 20) = 0;
        field<u32>(object, 24) = 0;
        ref<u8>(object + 32) &= static_cast<u8>(~0xC0);
        call(ctx, fn::memset, {object + 92, 0, 400});
        store<u32>(registry_slot, object);
        ref<u8>(guard) = 1;
    }
    return object;
}
// Native form of get(ctx): the object's guest address.
inline u32 get_native();
}  // namespace system_state

// Bits of g::sysutil_flags.
enum SysutilFlag : u32 {
    kSysutilMenuOpen = 0x004,       // XMB open (CELL_SYSUTIL_SYSTEM_MENU_OPEN / _CLOSE)
    kSysutilExitRequested = 0x010,  // CELL_SYSUTIL_REQUEST_EXITGAME
    kSysutilResume = 0x002,         // ? -> kMainResumed
    kSysutilNpOffline = 0x080,      // PSN went offline (NpManager_OnStatus) -> kMainPaused
    kSysutilPadLost = 0x100,        // no local user has a connected pad (set by Game_Update) -> kMainPaused
};

// g::main_state. Game_Update returns the next one; 1 and 2 keep the loop going.
enum MainState : u32 {
    kMainStarting = 0,
    kMainResumed = 1,  // set when kSysutilResume is up (name?)
    kMainPaused = 2,    // system menu or another overlay is up (name?)
    kMainQuit = 4,
};

// sys_memory_info_t (LV2)
struct MemoryInfo {
    be_u32 total_user_memory;
    be_u32 available_user_memory;
};
static_assert(sizeof(MemoryInfo) == 8);

constexpr u32 kSysutilMutexTimeoutUs = 100000;

}  // namespace sly4

// ---------------------------------------------------------------- native helpers (added for native code)
#include "opentit/native.h"

namespace sly4::lv2 {
// LV2 system call `num` (the `sc` instruction) from native code; returns r3.
// native.h has no wrapper for it yet.
template <typename... A>
inline u64 syscall(u32 num, A... args) {
    return opentit::decomp::syscall(opentit::native::ctx(), num, {opentit::native::detail::to_gpr(args)...});
}
}  // namespace sly4::lv2

namespace sly4 {
// The object at guest address ea, also for ea == 0 (native::host<T> maps 0
// to nullptr): for the original's unchecked accesses through a pointer that
// may be null.
template <typename T>
inline T& gref(u32 ea) {
    return *reinterpret_cast<T*>(opentit::mem::ptr(ea));
}

// The object a guest pointer member points at, also when it is 0.
template <typename T>
inline T& gref(const opentit::native::gp<T>& p) {
    return gref<T>(p.ea.get());
}

// system_state::get(ctx) for native code.
inline u32 system_state::get_native() {
    using namespace opentit::native;
    u8& built = global<u8>(guard);
    if (built == 0) {
        *host<be_u32>(object + 20) = 0u;
        *host<be_u32>(object + 24) = 0u;
        global<u8>(object + 32) &= static_cast<u8>(~0xC0);
        call(fn::memset, object + 92, 0u, 400u);
        global<be_u32>(registry_slot) = object;
        built = 1;
    }
    return object;
}
}  // namespace sly4
