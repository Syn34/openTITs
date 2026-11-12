// Start-up of the engine's subsystems.
#include "../debug/debug.h"
#include "../memory/mem.h"
#include "../platform/platform.h"
#include "../resource/resource.h"
#include "subsystems.h"

namespace sly4::system {
using namespace opentit::native;

namespace {

// Bits of Main_InitSystems' argument. main() passes 1, then -2 (the rest).
enum InitStage : u32 {
    kInitThreads = 0x1,  // record the main thread, then System_Init and Platform_Init
    kInit2 = 0x2,        // Resources_Init
    kInit4 = 0x4,        // Graphics_Init
    kInit8 = 0x8,        // Debug_Init
};

// function-local static: { u64 main_thread_id; u64 zero; }
constexpr u32 kMainThread = 0x011DE290, kMainThreadGuard = 0x011DE2A0;

constexpr u32 kHeapCreated = 0x00F8B0F8;      // u8
constexpr u32 kDefaultHeapSize = 0x8000;      // when config +0x1A4 is 0
constexpr u32 kRegistry = 0x011972E0;         // pointer table; [2] is SystemState
constexpr u32 kRegistryEntry0 = 0x010C5A00;   // u32 object, zeroed here
constexpr u32 kRegistryEntry1 = 0x0118E0D0;   // u32 object, zeroed here

}  // namespace

// Brings up the subsystems selected by `stages`, stopping at the first that
// fails. Every call then makes sure the main heap exists.
bool Main_InitSystems(u32 stages) {
    be_u64* main_thread = host<be_u64>(kMainThread);
    if (stages & kInitThreads) {
        u64 id;
        {
            GuestFrame frame(8);
            be_u64* out = frame.as<be_u64>();
            *out = 0;
            call(imp::sys_ppu_thread_get_id, out);
            id = *out;
        }
        u8& guard = global<u8>(kMainThreadGuard);
        if (guard == 0) {
            main_thread[0] = 0;
            main_thread[1] = 0;
            guard = 1;
        }
        main_thread[0] = id;
        main_thread[1] = 0;
        if (!System_Init()) return false;
        if (!platform::Platform_Init()) return false;
    }
    if ((stages & kInit4) && call<u32>(fn::Graphics_Init) == 0) return false;
    if ((stages & kInit2) && !resource::Resources_Init()) return false;
    if ((stages & kInit8) && !debug::Debug_Init()) return false;

    global<be_u32>(kRegistryEntry0) = 0;
    global<be_u32>(kRegistryEntry1) = 0;
    be_u32* registry = host<be_u32>(kRegistry);
    registry[0] = kRegistryEntry0;
    registry[1] = kRegistryEntry1;

    const be_f32 scale = *host<be_f32>(g::config + 0x14);  // copied as a word (lfs/stfs)
    *host<be_f32>(system_state::get_native() + 12) = scale;
    u8& created = global<u8>(kHeapCreated);
    if (created != 0) return true;
    created = 1;

    // Heap size from config +0x1A4: 0 means the default, negative means 0.
    const s32 configured = *host<be_s32>(g::config + 0x1A4);
    const u32 size = configured == 0 ? kDefaultHeapSize : configured > 0 ? static_cast<u32>(configured) : 0;
    LinearHeap* heap = nullptr;
    if (auto* mem = static_cast<LinearHeap*>(memory::Mem_Alloc(36, 0, 16))) {
        memory::Heap_Construct(mem, size, 0, 0);
        heap = mem;
    }
    *host<be_u32>(system_state::get_native() + 28) = guest(heap);
    return true;
}

}  // namespace sly4::system

NATIVE(005DB1D0, sly4::system::Main_InitSystems)
