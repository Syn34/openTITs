// World_HandleEvent callees, part 23i: two more Havok file loaders built on
// the stack file stream (as hevent22_e.cpp Level_LoadFile34AF14).
#include "hevent22.h"

using namespace sly4;
using namespace sly4::hevent;

namespace {
constexpr u32 kFileOpen = 0x0055BD3C;          // (path, mode) -> File* (symbols.txt File_Open)
constexpr u32 kModeRb = 0x00ADBE84;            // "rb"
constexpr u32 kStreamVtable = 0x00B1F338;      // stack file stream reader (as hevent22_e.cpp)
constexpr u32 kStreamBaseVtable = 0x00B13974;  // its base class, stored when it is torn down (name?)
constexpr u32 kStreamInit7CE648 = 0x007CE648;  // (StreamState*) (name?)
constexpr u32 kStreamFree7CEA58 = 0x007CEA58;  // (StreamState*) (name?)
constexpr u32 kHkLoad802E08 = 0x00802E08;      // (Stream*, Result*, {0, 0, 0} by value) -> object (name?)
constexpr u32 kHkRemoveRef = 0x007D4EA8;       // (obj) (symbols.txt AnimSkel_Release7D4EA8)
constexpr u32 kAnimLoad34A5E4 = 0x0034A5E4;    // (path, arg, object, u32 size) -> result (name?)
constexpr u32 kSceneLoad348F04 = 0x00348F04;   // (arg, path, object, u32 size, 0) -> result (name?)
constexpr u32 kFileSeek = 20, kFileSize = 24, kFileClose = 8;
// The stack reader at the original's 208-byte frame offsets: +112 reader
// (vtable), +118 u16, +120 File*, +128 u64, +136 u8 has file, +137 u8
// owns the file, +144 result, +148 stream state, +64..+75 the by-value
// struct (+64 / +68 / +72 zeroed; +76 unset).
constexpr u32 kFrame = 208;
constexpr u32 kRd = 112, kRdFile = 120, kRdHasFile = 136, kRdOwns = 137, kRes = 144, kState = 148, kZeros = 64;

void reader_setup(u32 s, u32 file) {
    store<u32>(s + kRdFile, file);
    store<u16>(s + 118, 1);
    store<u32>(s + kRd, kStreamVtable);
    store<u64>(s + 128, 0);
    store<u8>(s + kRdOwns, 0);
    store<u8>(s + kRdHasFile, file != 0 ? 1 : 0);
}
u64 hk_load(PPUContext& ctx, u32 s) {
    store<u32>(s + kZeros + 0, 0);
    store<u32>(s + kZeros + 8, 0);
    store<u32>(s + kZeros + 4, 0);
    return call(ctx, kHkLoad802E08, {s + kRd, s + kRes, load<u64>(s + kZeros), load<u64>(s + kZeros + 8)});
}
// Closes the reader's file when it owns one.
void reader_release(PPUContext& ctx, u32 s) {
    const u32 f = load<u32>(s + kRdFile);
    if (f == 0 || load<u8>(s + kRdOwns) == 0) return;
    store<u32>(s + kRd, kStreamVtable);
    vcall(ctx, f, kFileClose, {f});
}
}  // namespace

// u32 Anim_LoadFile34A8DC(const char* path, u32 arg) (name?): opens path
// ("rb"; 0 when missing), reads its size (vtable +24), loads it through the
// stack stream with Hk_Load802E08 and closes the file (vtable +8). A
// loaded object goes to 0x0034A5E4(path, arg, obj, size) and is released;
// the stream state is freed and a file the reader owns closed. Returns
// 0x0034A5E4's result, or 0.
DECOMP(0034A8DC, Anim_LoadFile34A8DC) {
    const u32 path = arg<u32>(ctx, 0), a1 = arg<u32>(ctx, 1);
    const u32 file = static_cast<u32>(call(ctx, kFileOpen, {path, kModeRb}));
    if (file == 0) return ret(ctx, 0u);
    const u32 size = static_cast<u32>(vcall(ctx, file, kFileSize, {file}));
    Stack frame(ctx, kFrame);
    const u32 s = frame.addr();
    reader_setup(s, file);
    store<u32>(s + kRes, 0);
    call(ctx, kStreamInit7CE648, {s + kState});
    const u32 obj = static_cast<u32>(hk_load(ctx, s));
    vcall(ctx, file, kFileClose, {file});
    u32 r = 0;
    if (obj != 0) {
        r = static_cast<u32>(call(ctx, kAnimLoad34A5E4, {path, a1, obj, size}));
        call(ctx, kHkRemoveRef, {obj});
    }
    call(ctx, kStreamFree7CEA58, {s + kState});
    reader_release(ctx, s);
    ret(ctx, r);
}

// u32 Scene_LoadFile349EAC(const char* path, u32 arg) (name?): as
// Anim_LoadFile34A8DC, but the file is rewound first (vtable +20 (f, 0, 0))
// and kept open until the end; a loaded object goes to 0x00348F04(arg,
// path, obj, size, 0) and is released. The reader's own file is closed
// before the stream state is freed (reader file cleared, base vtable
// 0x00B13974 stored), then the file is closed. Returns 0x00348F04's
// result, or 0.
DECOMP(00349EAC, Scene_LoadFile349EAC) {
    const u32 path = arg<u32>(ctx, 0), a1 = arg<u32>(ctx, 1);
    const u32 file = static_cast<u32>(call(ctx, kFileOpen, {path, kModeRb}));
    if (file == 0) return ret(ctx, 0u);
    Stack frame(ctx, kFrame);
    const u32 s = frame.addr();
    store<u32>(s + kRes, 0);
    call(ctx, kStreamInit7CE648, {s + kState});
    reader_setup(s, file);
    vcall(ctx, file, kFileSeek, {file, 0, 0});
    const u32 size = static_cast<u32>(vcall(ctx, file, kFileSize, {file}));
    const u32 obj = static_cast<u32>(hk_load(ctx, s));
    u32 r = 0;
    if (obj != 0) {
        r = static_cast<u32>(call(ctx, kSceneLoad348F04, {a1, path, obj, size, 0}));
        call(ctx, kHkRemoveRef, {obj});
    }
    reader_release(ctx, s);
    store<u32>(s + kRdFile, 0);
    store<u32>(s + kRd, kStreamBaseVtable);
    call(ctx, kStreamFree7CEA58, {s + kState});
    vcall(ctx, file, kFileClose, {file});
    ret(ctx, r);
}
