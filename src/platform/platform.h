// Platform start-up: Main_InitSystems' second stage (Platform_Init) and
// the pieces it brings up: the worker thread, FIOS (disc, cache and .psarc
// archives), Image.zip, pads, controller recording and the IFF chunk types.
//
// Names live in sly4::platform so this header doesn't collide with the
// memory/string headers that define some of the same C library entries.
#pragma once

#include "../sly4.h"

namespace sly4::platform {

// C library (still recompiled).
namespace libc {
constexpr u32 strcpy = 0x007239D8;   // (char* dst, const char* src)
constexpr u32 strcmp = 0x00723884;   // symbols.txt has this as "strcpy (name?)": it compares
constexpr u32 strncpy = 0x00723C88;  // (char* dst, const char* src, u32 n)
constexpr u32 strlen = 0x0071ED08;
constexpr u32 strstr = 0x0071EEF8;
constexpr u32 strchr = 0x00723678;
constexpr u32 memmove = 0x0071C3F0;
}  // namespace libc

namespace imp {
constexpr u32 cellSysCacheMount = 0x00ABDA04;
constexpr u32 cellPadInit = 0x00ABDE84;
}  // namespace imp

namespace sc {
constexpr u32 sys_event_flag_create = 82;             // (u32* id, sys_event_flag_attribute_t*, u64 init)
constexpr u32 sys_time_get_timebase_frequency = 147;  // () -> u64
}  // namespace sc

namespace fn {
// Decompiled here (src/platform, src/file/archive.cpp, src/resource/iff.cpp).
constexpr u32 Worker_CreateThreads = 0x0055DA00;
constexpr u32 Fios_Init = 0x00570E08;
constexpr u32 Archive_MountImageZip = 0x0055D830;
constexpr u32 Pad_Init = 0x0056F63C;
constexpr u32 Input_SetRecordMode = 0x0055BE90;
constexpr u32 Iff_RegisterChunk = 0x0055902C;
constexpr u32 Archive_Contains = 0x0055BA70;
constexpr u32 File_OpenArchived = 0x0055B87C;
constexpr u32 File_OpenDirect = 0x00570B64;
// Memory manager (decompiled in src/memory/alloc.cpp).
constexpr u32 Mem_AllocPool = 0x0056C514;
constexpr u32 Mem_FreePool = 0x0056C730;
constexpr u32 Mem_InitSystemStats = 0x0056C2E4;
// Still recompiled.
constexpr u32 String_Format = 0x005534D0;   // (fmt, ...) -> rotating static buffer
constexpr u32 FreeList_Init = 0x00551A58;   // (FreeList*, elem_size, count, heap or 0, bool)
constexpr u32 PtrVector_Reserve = 0x00551718;  // (PtrVector*, u32 capacity) (name?)
constexpr u32 Iff_InitTags = 0x00559310;    // registers ATAG/TAG2/TAGG, builds a 24-byte table; -> bool (name?)
constexpr u32 Thread_Create = 0x0056F5BC;   // (Thread*, entry opd, ThreadParams* or 0) (name?)
constexpr u32 Game_BootCheck = 0x0056C8E4;  // cellGameBootCheck / PatchCheck / ContentPermit -> s32 (name?)
constexpr u32 Image_Nop = 0x00570B88;       // empty function (blr)
constexpr u32 ZipArchive_Open = 0x0055D494; // (ZipArchive*, const char* path, 0) -> 1 on success (name?)
constexpr u32 Archive_Find = 0x0055B3C8;    // (Archive*, path, 0) -> 1 if the archive has it (name?)
constexpr u32 Archive_Prepare = 0x00559D5C; // (Archive*) -> 1 on success (735 instructions) (name?)
constexpr u32 SystemState_CallHook = 0x005DBB60;  // () calls SystemState+20 (opd) with SystemState+24 (name?)
constexpr u32 File_OpenFs = 0x005706FC;     // (path, mode) -> File*; "%s/%s", "%s - Open" (name?)
constexpr u32 IffMap_Find = 0x007A46EC;     // (map, const u32* key) -> ChunkDesc** slot (name?)
constexpr u32 IffMap_Erase = 0x007A44F8;    // (map, ChunkDesc*) (name?)
constexpr u32 IffMap_Insert = 0x007A4390;   // (map, const u32* key, ChunkDesc*) (name?)
// Sony FIOS (still recompiled).
constexpr u32 FIOSInit = 0x005F6D54;                 // (const fios_parameters*) "FIOS %s initialized"
constexpr u32 Fios_New = 0x005E7E1C;                 // (u32 size, u32 flags, u32 ?) FIOS allocator new (name?)
constexpr u32 Ps3Media_Construct = 0x00606834;       // (ps3media*, const char* root) (name?)
constexpr u32 Scheduler_CreateForMedia = 0x005FE29C; // fios::scheduler::createSchedulerForMedia(media, 0)
constexpr u32 SchedulerCache_Construct = 0x005EB938; // (cache*, media*, scheduler*, path, u64 key, 1, 1, 2037, 0x100000) (name?)
constexpr u32 Dearchiver_Construct = 0x005EFDD8;     // (dearchiver*, media*, 1, decompressors[], 1, 0, 2, extensions[]) (name?)
constexpr u32 Overlay_Construct = 0x005F978C;        // (overlay*, media*) (name?)
constexpr u32 Fios_Unknown5FCB1C = 0x005FCB1C;       // () called last (unknown)
constexpr u32 Mem_NewAligned = 0x00731648;           // (u32 size, u32 align) (name?)
constexpr u32 EdgeDecompressor_Construct = 0x007A5350;  // (dec*, CellSpurs*) "edgeDecompressorTaskset" (name?)
}  // namespace fn

// ---------------------------------------------------------------- memory manager (see src/memory/mem.h)
constexpr u32 kMemState = 0x01164898;      // { u8 initialised; u32 heap at +4; u8 +8; u8 +9 }
constexpr u32 kLinearHeapVtable = 0x00B242D0;
constexpr u32 kDefaultHeapSize = 0x80000;
constexpr u32 kDefaultHeapPool = 3;

// ---------------------------------------------------------------- files and archives
constexpr u32 kFileRoot = g::file_flags + 1;  // char[]: root prefixed to archive paths (name?)
constexpr u32 kModeWrite = 0x00AF37B0;        // "wb"
constexpr u32 kModeRead = 0x00AF37C0;         // "rb"

// The mounted archives (a pointer vector).
struct PtrVector {
    gptr data;          // +0 Archive*[]
    be_u16 capacity;    // +4
    be_u16 unk6;
    be_u16 count;       // +8
    be_u16 high_water;  // +10 largest count so far
};
static_assert(sizeof(PtrVector) == 12);
constexpr u32 kArchiveList = 0x01164E14;
constexpr u32 kImageArchiveCount = 0x01164A9C;  // u16: archive count after Image.zip (0 if it failed) (name?)

// g::file_flags bits 5-6: a 2-bit depth counter around file probes (wraps).
inline void adjust_file_depth(int delta) {
    u8& flags = ref<u8>(g::file_flags);
    const u8 depth = static_cast<u8>(((flags >> 5) + delta) & 3);
    flags = static_cast<u8>((flags & ~0x60) | (depth << 5));
}

// Virtual slot 2 of a File: close and free.
inline void file_close(PPUContext& ctx, u32 file) {
    call_ptr(ctx, load<u32>(load<u32>(file) + 8), {file});
}

// ---------------------------------------------------------------- IFF chunk types
// Four-character codes are stored with the first character in the low byte.
constexpr u32 fourcc(const char (&s)[5]) {
    return static_cast<u32>(static_cast<u8>(s[0])) | static_cast<u32>(static_cast<u8>(s[1])) << 8 |
           static_cast<u32>(static_cast<u8>(s[2])) << 16 | static_cast<u32>(static_cast<u8>(s[3])) << 24;
}
enum IffChunkFlags : u32 {
    kIffQuiet = 0x08,   // don't print "Replacing ..." when re-registering
    kIffFlag10 = 0x10,  // kept in ChunkDesc::flags (meaning unknown)
};

}  // namespace sly4::platform
