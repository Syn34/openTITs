// The file layer under FsFile / ZipArchive: FIOS operations wrapped in
// FsOp objects, the read cache and write buffer, archive mounting and the
// zip directory reader.
//
// Names live in sly4::fsio; .cpp files use an alias so nothing clashes
// with sly4::fn or the other sub-namespaces.
#pragma once

#include "../sly4.h"

namespace sly4::fsio {

namespace fn {
// FsOp (24 bytes): one FIOS request. Still recompiled.
constexpr u32 FsOp_Init = 0x00570B9C;       // (op, target, kind, options in r6-r8)
constexpr u32 FsOp_Wait = 0x00570BD4;       // (op)
constexpr u32 FsOp_Cancel = 0x00570C0C;     // (op) (name?)
constexpr u32 FsOp_GetError = 0x00570C38;   // (op) -> s32
constexpr u32 FsOp_Report = 0x00570C98;     // (op) logs the failure (name?)
constexpr u32 FsOp_GetResult = 0x00570D54;  // (op) -> u32 bytes transferred
constexpr u32 FsOp_GetPath = 0x00570D80;    // (op) -> const char* or 0; result unused here (name?)
constexpr u32 FsOp_Destruct = 0x00570DA8;   // (op)
constexpr u32 FsOp_Sync = 0x00561450;       // (op) (name?)
constexpr u32 FsOp_Release = 0x00561514;    // (op) (name?)
// FIOS requests (still recompiled).
constexpr u32 Fios_OpenFile = 0x00571178;     // (op, FsHandle*, path, mode, options in r7-r9) -> bool
constexpr u32 Fios_CloseFile = 0x00571350;    // (handle, op, options in r5-r7) -> bool
constexpr u32 Fios_WriteFile = 0x00571688;    // (handle, op, data, size, u64 pos, options in r8-r10) -> bool
constexpr u32 Fios_GetFileSize = 0x00571858;  // (handle, op, u64* out, options in r6-r8) -> bool
constexpr u32 Fios_Wait = 0x005F886C;         // (count, fios_op** ops, timeout) -> s32 (name?)
constexpr u32 Fios_CancelOp = 0x005FCA64;     // (scheduler, fios_op*) (name?)
constexpr u32 Fios_FreeOp = 0x005FE784;       // (scheduler, fios_op*) (name?)
constexpr u32 FsHandle_Construct = 0x00571D80;  // (FsHandle*) stores 0
constexpr u32 FsHandle_Destruct = 0x00571D98;   // (FsHandle*) empty
constexpr u32 Fs_Read = 0x00561FE8;           // (handle, buf, size, u64 pos, options in r7-r9) -> u32 (name?)
// Read cache helpers (still recompiled).
constexpr u32 Cache_Find = 0x00568A28;        // (cache, u64 pos, bool wait) -> slot or < 0 (name?)
constexpr u32 Cache_PickSlot = 0x00568E74;    // (cache, u64 pos, s32 hint) -> slot (name?)
constexpr u32 Cache_NextPrefetch = 0x00568D30;  // (cache, u64 pos, s32 hint, u64* out) -> slot or < 0 (name?)
constexpr u32 Cache_Prefetch = 0x007A5684;    // (cache, file, u64 pos, slot) (name?)
// Decompiled in src/file/fsio.cpp, cache.cpp, mount.cpp, zipread.cpp.
constexpr u32 Fs_Open = 0x005618B8;
constexpr u32 Fs_Close = 0x00561C78;
constexpr u32 Fs_Write = 0x00562230;
constexpr u32 Fs_GetSize = 0x005625C8;
// Others (still recompiled).
constexpr u32 String_Format = 0x005534D0;
constexpr u32 SystemState_CallHook = 0x005DBB60;
constexpr u32 Mem_FreePool = 0x0056C730;
constexpr u32 Mem_FreeFrom = 0x0054EE6C;        // (p, pool)
constexpr u32 PtrVector_Reserve = 0x00551718;   // (PtrVector*, capacity)
constexpr u32 ZipArchive_Open = 0x0055D494;     // (zip, path, prefix) -> bool
constexpr u32 ZipArchive_OpenMemory = 0x005677A4;  // (zip, data, prefix) -> bool (name?)
constexpr u32 Zip_FindEndRecord = 0x0055C780;   // (zip, bool sca) -> u32 offset of the end record (name?)
constexpr u32 Zip_ReadU32 = 0x00559534;         // (zip, u32* out) -> 1 on success (little-endian) (name?)
constexpr u32 Zip_ReadRaw = 0x0056736C;         // (zip, buf, size) -> u32 (name?)
constexpr u32 Zip_InflateEnd = 0x005DC7C0;      // zlib inflateEnd
constexpr u32 memmove = 0x0071C3F0;
constexpr u32 strcpy = 0x007239D8;
constexpr u32 strlen = 0x0071ED08;
}  // namespace fn

// FsOp kinds (FsOp+4).
enum FsOpKind : u32 { kOpOpen = 1, kOpClose = 2, kOpWrite = 4, kOpGetSize = 5 };
constexpr u32 kOpTarget = 8;   // FsHandle* / handle
constexpr u32 kOpReport = 20;  // u8: report a failure (set unless options & 16)
constexpr u32 kFsOpSize = 24;
constexpr s32 kErrBusy = static_cast<s32>(0x80010701);    // still in flight (name?)
constexpr s32 kErrIgnore = static_cast<s32>(0x80010708);  // not reported (name?)
constexpr u32 kOpNames = 0x00F88724;  // { s32 kind; const char* name; } [7]
constexpr u32 kOpNameCount = 7;
constexpr u32 kStrUnknown = 0x00AF3950;  // "Unknown:0x%x"
constexpr u64 kDefaultOptions = 1;      // first doubleword of an options block { 0, 1, 0, 0, 0, 0 }

// File fields used here (see src/file/fs.cpp).
constexpr u32 kFilePosition = 264, kFileHandle = 280, kFileSize = 288, kFileName = 4;

// ReadCache: { u8* buffer; u32 count; u32 slot_size; u32; Slot slots[count]; }
struct CacheSlot {
    gptr buffer;     // +0
    be_u32 pad4;
    be_u64 start;    // +8  file offset of the data, -1 if empty
    be_u64 end;      // +16
    gptr op;         // +24 FsOp* of a prefetch in flight
    be_u32 pad28;
};
static_assert(sizeof(CacheSlot) == 32);
constexpr u32 kCacheSlots = 16;

// Waits for `op` and, if it failed and wants reporting, reports it (the
// block every Fs_* wrapper inlines).
inline void op_finish(PPUContext& ctx, u32 op) {
    call(ctx, fn::FsOp_Wait, {op});
    if (ref<u8>(op + kOpReport) == 0) return;
    ref<u8>(op + kOpReport) = 0;
    const s32 err = static_cast<s32>(call(ctx, fn::FsOp_GetError, {op}));
    if (err == 0 || err == kErrIgnore) return;
    const s32 kind = load<s32>(op + 4);
    bool known = false;
    for (u32 i = 0; i < kOpNameCount && !known; ++i) known = load<s32>(kOpNames + 8 * i) == kind;
    if (!known) call(ctx, fn::String_Format, {kStrUnknown, static_cast<u64>(static_cast<s64>(kind))});
    call(ctx, fn::FsOp_GetPath, {op});
    call(ctx, fn::FsOp_Report, {op});
}

// FsOp_Destruct + free; returns what Mem_FreePool left in r3.
inline u64 op_delete(PPUContext& ctx, u32 op) {
    call(ctx, fn::FsOp_Destruct, {op});
    return call(ctx, fn::Mem_FreePool, {op, 0, 0});
}

}  // namespace sly4::fsio
