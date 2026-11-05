// World_HandleEvent callees, part 28f: opening a bank file (Bank_Open,
// called by GeomSet_LoadScene4F2678 and the UI loader).
#include "hevent28.h"

using namespace sly4;
using namespace sly4::hevent;

namespace {
namespace bfn {
constexpr u32 String_Copy = 0x00553684;       // (dst, src, size) -> dst
constexpr u32 String_ToLower = 0x005565A0;    // (char*)
constexpr u32 String_Find = 0x00555FCC;       // (s, sub) -> strstr
constexpr u32 SetName = 0x0051E7F0;           // (name) (SetName_0051E7F0)
constexpr u32 Bank_ResetState = 0x00564F2C;   // ()
constexpr u32 File_Open = 0x0055BD3C;         // (path, mode) -> File*
constexpr u32 Bank_AllocHeader = 0x00569280;  // (hdr)
constexpr u32 Stream_Init = 0x00569338;       // (s, File*, u8 flags) -> bool (Stream_Init569338)
constexpr u32 Iff_ReadChunks = 0x00569388;    // (reader, obj)
constexpr u32 Bank_ClearField4 = 0x00569378;  // (p)
constexpr u32 Mem_FreeAndClear = 0x005692E8;  // (void** p)
}  // namespace bfn

constexpr u32 kLevelsDir = 0x00AF4F90;  // "levels/"
constexpr u32 kModeRb = 0x00AF4F98;     // "rb"
// ApeTListAllocator slots ({+0 allocator, +4 per block, +8 heap, +12
// owner bank}), pointed to by these globals; while a bank loads, each gets
// heap 0x00F3C5A8 and the bank as owner. The reset afterwards skips
// 0x00F8F140 (as the original).
constexpr u32 kAllocHeap = 0x00F3C5A8;
constexpr u32 kSlotsSet[] = {0x00F8F118, 0x00F8F114, 0x00F8F130, 0x00F8F12C, 0x00F8F138, 0x00F8F134, 0x00F8F13C,
                             0x00F8F140, 0x00F8F14C, 0x00F8F148, 0x00F8F144, 0x00F8F154, 0x00F8F158};
constexpr u32 kSlotsReset[] = {0x00F8F118, 0x00F8F114, 0x00F8F130, 0x00F8F12C, 0x00F8F134, 0x00F8F138,
                               0x00F8F14C, 0x00F8F148, 0x00F8F144, 0x00F8F13C, 0x00F8F154, 0x00F8F158};
constexpr u32 kBankName = 4, kBankNameSize = 32, kBankFlags = 64;
constexpr u32 kLoadState = 0x01193060 + 36;  // u32 object: +17 = +16 for banks with flag bit 6 (name?)
// The original's 416-byte frame: path buffer at r1 +112 (256 bytes), IFF
// reader header at r1 +368.
constexpr u32 kFrame = 416, kPathAt = 112, kPathSize = 256, kHdrAt = 368;
}  // namespace

// bool Bank_Open(Bank* b, const char* path, u8 flags) (name?): names the
// bank (+4, 32 chars), sets the global name to the lowercased path cut at
// "levels/" (when found), makes the bank the owner of the list
// allocators, then reads the file's IFF chunks (Iff_ReadChunks) and closes
// it (vt +8). Returns whether the file opened.
DECOMP(005904EC, Bank_Open) {
    const u32 bank = arg<u32>(ctx, 0), path = arg<u32>(ctx, 1), flags = arg<u32>(ctx, 2);
    Stack frame(ctx, kFrame);
    const u32 buf = frame.addr() + kPathAt, hdr = frame.addr() + kHdrAt;
    call(ctx, bfn::String_Copy, {bank + kBankName, path, kBankNameSize});
    const u32 s = static_cast<u32>(call(ctx, bfn::String_Copy, {buf, path, kPathSize}));
    call(ctx, bfn::String_ToLower, {s});
    const u32 at = static_cast<u32>(call(ctx, bfn::String_Find, {buf, kLevelsDir}));
    if (at != 0) {
        store<u8>(at, 0);
        call(ctx, bfn::SetName, {buf, 0});
    }
    for (const u32 g : kSlotsSet) {
        store<u32>(load<u32>(g) + 8, kAllocHeap);
        store<u32>(load<u32>(g) + 12, bank);
    }
    call(ctx, bfn::Bank_ResetState, {});
    if ((load<u8>(bank + kBankFlags) & 0x40) != 0) {
        const u32 st = load<u32>(kLoadState);
        if (st != 0) store<u8>(st + 17, load<u8>(st + 16));
    }
    u32 opened = 0;
    const u32 f = static_cast<u32>(call(ctx, bfn::File_Open, {path, kModeRb}));
    if (f != 0) {
        const u32 byte_flag = (flags & 0xFFu) != 0 ? 1u : 0u;
        call(ctx, bfn::Bank_AllocHeader, {hdr});
        if (call(ctx, bfn::Stream_Init, {hdr, f, byte_flag}) != 0) {
            call(ctx, bfn::Iff_ReadChunks, {hdr, 0});
            call(ctx, bfn::Bank_ClearField4, {hdr});
        }
        vcall(ctx, f, 8, {f});
        opened = 1;
        call(ctx, bfn::Mem_FreeAndClear, {hdr});
    }
    for (const u32 g : kSlotsReset) {
        store<u32>(load<u32>(g) + 8, 0);
        store<u32>(load<u32>(g) + 12, 0);
    }
    ctx.gpr[3] = opened;
}
