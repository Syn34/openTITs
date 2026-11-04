// FsOp: one asynchronous FIOS request (open / close / read / write / stat)
// and the wrappers that issue them. Names live in sly4::fsop.
#pragma once

#include "../sly4.h"

namespace sly4::fsop {

namespace fn {
// Decompiled in src/file/fsop.cpp.
constexpr u32 FsOp_Init = 0x00570B9C;
constexpr u32 FsOp_Wait = 0x00570BD4;
constexpr u32 FsOp_Cancel = 0x00570C0C;
constexpr u32 FsOp_GetError = 0x00570C38;
constexpr u32 FsOp_ErrorName = 0x00570C98;  // (was "FsOp_Report")
constexpr u32 FsOp_GetResult = 0x00570D54;
constexpr u32 FsOp_GetPath = 0x00570D80;
constexpr u32 FsOp_Destruct = 0x00570DA8;
constexpr u32 FsOp_Sync = 0x00561450;
constexpr u32 FsOp_Release = 0x00561514;
constexpr u32 String_HasChar = 0x00561270;
// Decompiled in src/file/fioswrap.cpp.
constexpr u32 Fios_OpenFile = 0x00571178;
constexpr u32 Fios_CloseFile = 0x00571350;
constexpr u32 Fios_ReadFile = 0x005714B8;
constexpr u32 Fios_WriteFile = 0x00571688;
constexpr u32 Fios_GetFileSize = 0x00571858;
constexpr u32 Fs_ReadAsync = 0x00561E74;
// fios:: SDK (still recompiled).
constexpr u32 fios_Wait = 0x005F886C;        // (count, fios_op** ops, timeout) -> s32
constexpr u32 fios_GetResult = 0x005F8AF4;   // (fios_op*) -> bytes
constexpr u32 fios_Cancel = 0x005FCA64;      // (scheduler, fios_op*)
constexpr u32 fios_Free = 0x005FE784;        // (scheduler, fios_op*)
constexpr u32 fios_OpenFile = 0x005FEF14;    // (scheduler, attr*, path, flags, FsHandle*) -> fios_op*
constexpr u32 fios_CloseFile = 0x006001D4;   // (scheduler, attr*, fh) -> fios_op*
constexpr u32 fios_ReadFile = 0x00603514;    // (scheduler, attr*, fh, buf, size, u64 pos) -> fios_op*
constexpr u32 fios_WriteFile = 0x0060381C;   // (scheduler, attr*, fh, data, size, u64 pos) -> fios_op*
constexpr u32 fios_Stat = 0x005FFE10;        // (scheduler, attr*, path or 0, u64* out, 3) -> fios_op* (name?)
// Others.
constexpr u32 String_Format = 0x005534D0;
constexpr u32 Mem_FreePool = 0x0056C730;
constexpr u32 Mem_FreeFrom = 0x0054EE6C;
constexpr u32 Mode_IsRead = 0x0055BD00;
constexpr u32 strchr = 0x00723678;
}  // namespace fn

// FsOp (24 bytes).
struct FsOp {
    gptr fios_op;    // +0
    be_u32 kind;     // +4  FsOpKind
    be_u32 target;   // +8  FsHandle* / handle
    be_u32 opt2;     // +12 options word 2
    be_u32 opt3;     // +16 options word 3
    u8 report;       // +20 report failures (cleared by options flag 0x10)
    u8 pad[3];
};
static_assert(sizeof(FsOp) == 24);
enum FsOpKind : u32 { kOpOpen = 1, kOpClose = 2, kOpRead = 3, kOpWrite = 4, kOpGetSize = 5 };

// fios_op fields read here.
constexpr u32 kFiosOpDone = 152;       // non-zero once issued/finished (read after isync)
constexpr u32 kFiosOpScheduler = 156;
constexpr u32 kFiosOpPath = 52;        // const char*
constexpr u32 kFiosHandlePath = 48;    // FIOS file handle +48: const char* path

constexpr s32 kErrBusy = static_cast<s32>(0x80010701);
constexpr s32 kErrIgnore = static_cast<s32>(0x80010708);
constexpr u32 kKindNames = 0x00F88724;  // { s32 kind; const char* name; } [7]
constexpr u32 kErrorNames = 0x00F8881C; // { s32 code; const char* name; } [24]
constexpr u32 kStrUnknownKind = 0x00AF3950;   // "Unknown:0x%x"
constexpr u32 kStrUnknownError = 0x00AF47D4;  // "Unknown:0x%x"
constexpr u32 kScheduler = 0x00F88770;  // u32: FIOS scheduler (Fios_Init)

}  // namespace sly4::fsop
