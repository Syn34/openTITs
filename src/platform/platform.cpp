// Platform start-up (Main_InitSystems' second stage) and the small pieces
// it starts: the worker thread, pads and controller recording.
#include "platform.h"

using namespace sly4;
using namespace sly4::platform;
namespace pf = sly4::platform;  // its fn/libc/imp/sc sit beside sly4's

namespace {

constexpr u32 kPlatformStarted = 0x01164880;    // u8, set first thing by Platform_Init (name?)
constexpr u32 kTimebaseFrequency = 0x01164CD8;  // u64
constexpr u32 kIffUnkF8870C = 0x00F8870C;       // u32, cleared after the IFF types are registered
constexpr u32 kIffUnk01164E4C = 0x01164E4C;     // u8, likewise

// IFF chunk handlers (OPDs) registered by Platform_Init.
constexpr u32 kIffForm = 0x00F3C2A0, kIffPlat = 0x00F3C2A8, kIffProf = 0x00F3C2B0, kIffPrfe = 0x00F3C2B8,
              kIffPrfn = 0x00F3C2C0;

// ---------------------------------------------------------------- worker thread
// ApeWorkerThread: a PPU thread driven by two event flags.
struct WorkerThread {
    gptr name;            // +0  "ApeWorkerThread_1"
    be_u32 pad4;
    be_u64 thread_id;     // +8
    gptr start_name;      // +16 "WaitStart"
    be_u32 start_flag;    // +20 sys_event_flag_t
    gptr complete_name;   // +24 "WaitComplete"
    be_u32 complete_flag; // +28 sys_event_flag_t
    be_u32 unk32;
    be_u32 unk36;
    u8 unk40;
    u8 pad41[7];
};
static_assert(sizeof(WorkerThread) == 48);
constexpr u32 kWorkerThreadCount = 1;
constexpr u32 kWorkerThreads = 0x01164DF8;      // WorkerThread* [kWorkerThreadCount]
constexpr u32 kWorkerThreadNames = 0x00F88718;  // const char* []
constexpr u32 kWorkerEntry = 0x00F3C128;        // OPD of the thread function (0x00565378)
constexpr u32 kStrWaitStart = 0x00B24348;       // "WaitStart"
constexpr u32 kStrWaitComplete = 0x00B24352;    // "WaitComplete"

// sys_event_flag_attribute_t
struct EventFlagAttr {
    be_u32 protocol;
    be_u32 pshared;
    be_u64 ipc_key;
    be_s32 flags;
    be_s32 type;
    char name[8];
};
static_assert(sizeof(EventFlagAttr) == 32);
constexpr u32 SYS_SYNC_PRIORITY = 0x2;
constexpr u32 SYS_SYNC_NOT_PROCESS_SHARED = 0x200;
constexpr s32 SYS_SYNC_WAITER_MULTIPLE = 0x20000;

// Thread_Create's parameters.
struct ThreadParams {
    be_s32 priority;
    be_u32 unk4;
    be_u32 unk8;
};
constexpr s32 kWorkerPriority = 0x600;

void create_event_flag(PPUContext& ctx, u32 attr_ea, u32 name, u32 id_out) {
    EventFlagAttr& attr = ref<EventFlagAttr>(attr_ea);
    attr.name[0] = 0;
    attr.ipc_key = 0;
    attr.protocol = SYS_SYNC_PRIORITY;
    attr.pshared = SYS_SYNC_NOT_PROCESS_SHARED;
    attr.flags = 0;
    attr.type = SYS_SYNC_WAITER_MULTIPLE;
    call(ctx, pf::libc::strncpy, {attr_ea + 24, name, sizeof(attr.name) - 1});
    attr.name[7] = 0;
    syscall(ctx, pf::sc::sys_event_flag_create, {id_out, attr_ea, 0});
}

// ---------------------------------------------------------------- controller recording
constexpr u32 kInputRecord = 0x01164AA0;  // { u32 ?; File* file (+4); s32 mode (+8); }
constexpr u32 kStrControllerBin = 0x00AF37A0;  // "controller.bin"
constexpr u32 kFrameTime60 = 0x3C888889;       // 1/60 as f32 bits, SystemState+8
enum RecordMode : s32 { kRecordOff = 0, kRecordWrite = 1, kRecordPlay = 2 };

// ---------------------------------------------------------------- pads
constexpr u32 kPadState = 0x0116B2A0;
constexpr u32 kPadStateSize = 132;
constexpr u32 kPadMaxConnect = 4;

// The memory manager's lazy set-up, as Mem_Alloc does it (src/memory/alloc.cpp):
// the guard is set before the nested allocations, so they go straight through.
void mem_lazy_init(PPUContext& ctx) {
    ref<u8>(kMemState) = 1;
    ref<u8>(kMemState + 8) = 0;
    ref<u8>(kMemState + 9) = 1;
    call(ctx, pf::fn::Mem_InitSystemStats);
    const u32 heap = static_cast<u32>(call(ctx, sly4::fn::Mem_New, {36}));
    if (heap != 0) {
        field<u32>(heap, 0) = kLinearHeapVtable;
        field<u32>(heap, 24) = kDefaultHeapPool;
        field<u32>(heap, 4) = 0;
        field<u32>(heap, 8) = 0;
        field<u32>(heap, 12) = 0;
        field<u32>(heap, 20) = kDefaultHeapSize;
        ref<u8>(heap + 28) = 1;
        field<u32>(heap, 32) = kDefaultHeapSize;
        const u32 base = static_cast<u32>(call(ctx, pf::fn::Mem_AllocPool, {16, kDefaultHeapSize, kDefaultHeapPool, 0}));
        field<u32>(heap, 4) = base;
        field<u32>(heap, 12) = base;
        field<u32>(heap, 8) = base != 0 ? base + field<u32>(heap, 20).get() : 0;
    }
    field<u32>(kMemState, 4) = heap;
}

}  // namespace

// bool Platform_Init()
// Main_InitSystems' stage 1 after System_Init: makes sure the memory manager
// is up, records the timebase frequency, starts the worker thread, FIOS and
// Image.zip, initialises the pads, stops controller recording and registers
// the core IFF chunk types.
DECOMP(0055DC18, Platform_Init) {
    ref<u8>(kPlatformStarted) = 1;
    if (ref<u8>(kMemState) == 0) mem_lazy_init(ctx);  // inlined Mem_Alloc set-up in the original

    store<u64>(kTimebaseFrequency, syscall(ctx, pf::sc::sys_time_get_timebase_frequency));
    if (static_cast<s32>(call(ctx, pf::fn::Worker_CreateThreads)) == 0) return ret(ctx, 0u);
    if (static_cast<s32>(call(ctx, pf::fn::Fios_Init)) == 0) return ret(ctx, 0u);
    if (static_cast<s32>(call(ctx, pf::fn::Archive_MountImageZip)) == 0) return ret(ctx, 0u);
    call(ctx, pf::fn::Pad_Init);  // result unchecked
    call(ctx, pf::fn::Input_SetRecordMode, {kRecordOff});

    constexpr u32 kFlags = kIffQuiet | kIffFlag10;
    call(ctx, pf::fn::Iff_RegisterChunk, {fourcc("FORM"), kIffForm, 0, 0, kFlags});
    call(ctx, pf::fn::Iff_RegisterChunk, {fourcc("PLAT"), kIffPlat, 0, 0, kFlags});
    call(ctx, pf::fn::Iff_RegisterChunk, {fourcc("PROF"), kIffProf, 0, 0, kFlags});
    call(ctx, pf::fn::Iff_RegisterChunk, {fourcc("PRFE"), kIffPrfe, 0, 0, kFlags});
    call(ctx, pf::fn::Iff_RegisterChunk, {fourcc("PRFN"), kIffPrfn, 0, 0, kFlags});
    store<u32>(kIffUnkF8870C, 0);
    ref<u8>(kIffUnk01164E4C) = 0;
    ret(ctx, static_cast<s32>(call(ctx, pf::fn::Iff_InitTags)) != 0 ? 1u : 0u);
}

// bool Worker_CreateThreads()
// Creates the worker thread object ("ApeWorkerThread_1") with its
// "WaitStart" / "WaitComplete" event flags and starts the thread. A failed
// allocation just leaves the slot empty; always returns true.
DECOMP(0055DA00, Worker_CreateThreads) {
    for (u32 i = 0; i < kWorkerThreadCount; ++i) {
        const u32 obj = static_cast<u32>(call(ctx, sly4::fn::Mem_New, {sizeof(WorkerThread)}));
        if (obj != 0) {
            WorkerThread& w = ref<WorkerThread>(obj);
            w.start_name = kStrWaitStart;
            w.name = load<u32>(kWorkerThreadNames + 4 * i);
            w.thread_id = 0;
            Stack frame(ctx, 2 * sizeof(EventFlagAttr) + 16);
            const u32 start_attr = frame.addr(), complete_attr = start_attr + sizeof(EventFlagAttr);
            const u32 params = complete_attr + sizeof(EventFlagAttr);
            create_event_flag(ctx, start_attr, w.start_name, obj + 20);
            w.complete_name = kStrWaitComplete;
            create_event_flag(ctx, complete_attr, w.complete_name, obj + 28);
            w.unk32 = 0;
            w.unk36 = 0;
            w.unk40 = 0;
            ThreadParams& p = ref<ThreadParams>(params);
            p.priority = kWorkerPriority;
            p.unk4 = 0;
            p.unk8 = 1;
            call(ctx, pf::fn::Thread_Create, {obj, kWorkerEntry, params});
        }
        store<u32>(kWorkerThreads + 4 * i, obj);
    }
    ret(ctx, 1u);
}

// bool Pad_Init()
DECOMP(0056F63C, Pad_Init) {
    call(ctx, sly4::fn::memset, {kPadState, 0, kPadStateSize});
    const s32 rc = static_cast<s32>(call(ctx, pf::imp::cellPadInit, {kPadMaxConnect}));
    ret(ctx, rc == 0 ? 1u : 0u);
}

// void Input_SetRecordMode(s32 mode)
// Closes controller.bin if it's open, then opens it for recording (1, "wb")
// or playback (2, "rb"), with the frame time fixed at 1/60 s.
DECOMP(0055BE90, Input_SetRecordMode) {
    const u64 mode_reg = ctx.gpr[3];  // returned unchanged when nothing is opened
    const s32 mode = arg<s32>(ctx, 0);
    field<u32>(kInputRecord, 8) = static_cast<u32>(mode);
    const u32 file = field<u32>(kInputRecord, 4);
    if (file != 0) {
        file_close(ctx, file);
        field<u32>(kInputRecord, 4) = 0;
    }
    if (mode != kRecordWrite && mode != kRecordPlay) {
        ctx.gpr[3] = mode_reg;
        return;
    }
    const u32 state = system_state::get(ctx);
    field<u32>(state, 8) = kFrameTime60;
    const u32 opened = static_cast<u32>(
        call(ctx, sly4::fn::File_Open, {kStrControllerBin, mode == kRecordWrite ? kModeWrite : kModeRead}));
    field<u32>(kInputRecord, 4) = opened;
    ret(ctx, opened);
}
