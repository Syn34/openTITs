// Platform start-up (Main_InitSystems' second stage) and the small pieces
// it starts: the worker thread, pads and controller recording.
#include "platform.h"

#include "../heap/heap.h"
#include "../memory/mem.h"
#include "../resource/tags.h"
#include "thread.h"

namespace sly4::platform {
using namespace opentit::native;
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
    gp<char> name;            // +0  "ApeWorkerThread_1"
    be_u32 pad4;
    be_u64 thread_id;         // +8
    gp<char> start_name;      // +16 "WaitStart"
    be_u32 start_flag;        // +20 sys_event_flag_t
    gp<char> complete_name;   // +24 "WaitComplete"
    be_u32 complete_flag;     // +28 sys_event_flag_t
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

constexpr s32 kWorkerPriority = 0x600;

void create_event_flag(EventFlagAttr* attr, char* name, be_u32* id_out) {
    attr->name[0] = 0;
    attr->ipc_key = 0;
    attr->protocol = SYS_SYNC_PRIORITY;
    attr->pshared = SYS_SYNC_NOT_PROCESS_SHARED;
    attr->flags = 0;
    attr->type = SYS_SYNC_WAITER_MULTIPLE;
    call(pf::libc::strncpy, attr->name, name, static_cast<u32>(sizeof(attr->name) - 1));
    attr->name[7] = 0;
    lv2::syscall(pf::sc::sys_event_flag_create, id_out, attr, 0u);
}

// ---------------------------------------------------------------- controller recording
struct File;
struct InputRecord {
    be_u32 unk0;     // +0
    gp<File> file;   // +4
    be_s32 mode;     // +8
};
constexpr u32 kInputRecord = 0x01164AA0;
constexpr u32 kStrControllerBin = 0x00AF37A0;  // "controller.bin"
constexpr u32 kFrameTime60 = 0x3C888889;       // 1/60 as f32 bits, SystemState+8
enum RecordMode : s32 { kRecordOff = 0, kRecordWrite = 1, kRecordPlay = 2 };

// ---------------------------------------------------------------- pads
constexpr u32 kPadState = 0x0116B2A0;
constexpr u32 kPadStateSize = 132;
constexpr u32 kPadMaxConnect = 4;

}  // namespace

// Main_InitSystems' stage 1 after System_Init: makes sure the memory manager
// is up, records the timebase frequency, starts the worker thread, FIOS and
// Image.zip, initialises the pads, stops controller recording and registers
// the core IFF chunk types.
bool Platform_Init() {
    global<u8>(kPlatformStarted) = 1;
    heap::MemState_EnsureInitNative();  // inlined Mem_Alloc set-up in the original

    global<be_u64>(kTimebaseFrequency) = lv2::syscall(pf::sc::sys_time_get_timebase_frequency);
    if (!Worker_CreateThreads()) return false;
    if (!Fios_Init()) return false;
    if (call<s32>(pf::fn::Archive_MountImageZip) == 0) return false;
    Pad_Init();  // result unchecked
    Input_SetRecordMode(kRecordOff);

    constexpr u32 kFlags = kIffQuiet | kIffFlag10;
    Iff_RegisterChunk(fourcc("FORM"), kIffForm, 0, nullptr, kFlags);
    Iff_RegisterChunk(fourcc("PLAT"), kIffPlat, 0, nullptr, kFlags);
    Iff_RegisterChunk(fourcc("PROF"), kIffProf, 0, nullptr, kFlags);
    Iff_RegisterChunk(fourcc("PRFE"), kIffPrfe, 0, nullptr, kFlags);
    Iff_RegisterChunk(fourcc("PRFN"), kIffPrfn, 0, nullptr, kFlags);
    global<be_u32>(kIffUnkF8870C) = 0;
    global<u8>(kIffUnk01164E4C) = 0;
    return tags::Iff_InitTags();
}

// Creates the worker thread object ("ApeWorkerThread_1") with its
// "WaitStart" / "WaitComplete" event flags and starts the thread. A failed
// allocation just leaves the slot empty; always returns true.
bool Worker_CreateThreads() {
    for (u32 i = 0; i < kWorkerThreadCount; ++i) {
        WorkerThread* w = static_cast<WorkerThread*>(memory::Mem_New(sizeof(WorkerThread)));
        if (w) {
            w->start_name = host<char>(kStrWaitStart);
            w->name = host<gp<char>>(kWorkerThreadNames)[i];
            w->thread_id = 0;
            GuestFrame frame(2 * sizeof(EventFlagAttr) + 16);
            EventFlagAttr* start_attr = frame.as<EventFlagAttr>(0);
            EventFlagAttr* complete_attr = start_attr + 1;
            thread::ThreadParams* params = frame.as<thread::ThreadParams>(2 * sizeof(EventFlagAttr));
            create_event_flag(start_attr, w->start_name, &w->start_flag);
            w->complete_name = host<char>(kStrWaitComplete);
            create_event_flag(complete_attr, w->complete_name, &w->complete_flag);
            w->unk32 = 0;
            w->unk36 = 0;
            w->unk40 = 0;
            params->priority = kWorkerPriority;
            params->unk4 = 0;
            params->unk8 = 1;
            thread::Thread_Create(w, kWorkerEntry, params);
        }
        host<gp<WorkerThread>>(kWorkerThreads)[i] = w;
    }
    return true;
}

bool Pad_Init() {
    call(sly4::fn::memset, kPadState, 0u, kPadStateSize);
    return call<s32>(pf::imp::cellPadInit, kPadMaxConnect) == 0;
}

// Closes controller.bin if it's open, then opens it for recording (1, "wb")
// or playback (2, "rb"), with the frame time fixed at 1/60 s.
void Input_SetRecordMode(s32 mode) {
    InputRecord& rec = global<InputRecord>(kInputRecord);
    rec.mode = mode;
    if (File* file = rec.file) {
        vcall(file, 8);  // close and free
        rec.file = nullptr;
    }
    if (mode != kRecordWrite && mode != kRecordPlay) return;
    *host<be_u32>(system_state::get_native() + 8) = kFrameTime60;
    rec.file = call<File*>(sly4::fn::File_Open, kStrControllerBin, mode == kRecordWrite ? kModeWrite : kModeRead);
}

}  // namespace sly4::platform

NATIVE(0055DC18, sly4::platform::Platform_Init)
NATIVE(0055DA00, sly4::platform::Worker_CreateThreads)
NATIVE(0056F63C, sly4::platform::Pad_Init)
NATIVE(0055BE90, sly4::platform::Input_SetRecordMode)
