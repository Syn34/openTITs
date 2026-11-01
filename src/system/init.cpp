// Start-up of the engine's subsystems.
#include "../sly4.h"

using namespace sly4;

namespace {

// Bits of Main_InitSystems' argument. main() passes 1, then -2 (the rest).
enum InitStage : u32 {
    kInitThreads = 0x1,  // record the main thread, then sub_0056D87C and sub_0055DC18
    kInit2 = 0x2,        // sub_00588828
    kInit4 = 0x4,        // sub_0050F520
    kInit8 = 0x8,        // sub_0056A944
};

// function-local static: { u64 main_thread_id; u64 zero; }
constexpr u32 kMainThread = 0x011DE290, kMainThreadGuard = 0x011DE2A0;

constexpr u32 kHeapCreated = 0x00F8B0F8;      // u8
constexpr u32 kDefaultHeapSize = 0x8000;      // when config +0x1A4 is 0
constexpr u32 kRegistry = 0x011972E0;         // pointer table; [2] is SystemState
constexpr u32 kRegistryEntry0 = 0x010C5A00;   // u32 object, zeroed here
constexpr u32 kRegistryEntry1 = 0x0118E0D0;   // u32 object, zeroed here

}  // namespace

// bool Main_InitSystems(u32 stages)
// Brings up the subsystems selected by `stages`, stopping at the first that
// fails. Every call then makes sure the main heap exists.
DECOMP(005DB1D0, Main_InitSystems) {
    const u32 stages = arg<u32>(ctx, 0);

    if (stages & kInitThreads) {
        u64 id;
        {
            Stack frame(ctx, 8);
            store<u64>(frame.addr(), 0);
            call(ctx, imp::sys_ppu_thread_get_id, {frame.addr()});
            id = load<u64>(frame.addr());
        }
        if (ref<u8>(kMainThreadGuard) == 0) {
            store<u64>(kMainThread, 0);
            store<u64>(kMainThread + 8, 0);
            ref<u8>(kMainThreadGuard) = 1;
        }
        store<u64>(kMainThread, id);
        store<u64>(kMainThread + 8, 0);
        if (static_cast<u32>(call(ctx, fn::sub_0056D87C)) == 0) return ret(ctx, 0u);
        if (static_cast<u32>(call(ctx, fn::sub_0055DC18)) == 0) return ret(ctx, 0u);
    }
    if ((stages & kInit4) && static_cast<u32>(call(ctx, fn::sub_0050F520)) == 0) return ret(ctx, 0u);
    if ((stages & kInit2) && static_cast<u32>(call(ctx, fn::sub_00588828)) == 0) return ret(ctx, 0u);
    if ((stages & kInit8) && static_cast<u32>(call(ctx, fn::sub_0056A944)) == 0) return ret(ctx, 0u);

    store<u32>(kRegistryEntry0, 0);
    store<u32>(kRegistryEntry1, 0);
    store<u32>(kRegistry, kRegistryEntry0);
    store<u32>(kRegistry + 4, kRegistryEntry1);

    const f32 scale = field<f32>(g::config, 0x14);
    const u32 state = system_state::get(ctx);
    field<f32>(state, 12) = scale;
    if (ref<u8>(kHeapCreated) != 0) return ret(ctx, 1u);
    ref<u8>(kHeapCreated) = 1;

    // Heap size from config +0x1A4: 0 means the default, negative means 0.
    const s32 configured = static_cast<s32>(field<u32>(g::config, 0x1A4).get());
    const u32 size = configured == 0 ? kDefaultHeapSize : configured > 0 ? static_cast<u32>(configured) : 0;
    const u32 mem = static_cast<u32>(call(ctx, fn::Mem_Alloc, {36, 0, 16}));
    u32 heap = 0;
    if (mem != 0) {
        call(ctx, fn::Heap_Construct, {mem, size, 0, 0});
        heap = mem;
    }
    field<u32>(system_state::get(ctx), 28) = heap;
    ret(ctx, 1u);
}
