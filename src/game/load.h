// Names used by Game_Load (0x00463000) and the functions it calls:
// the UI render target, UI element lookup, input devices, the tuning
// variables in Scripts/Tuning.xml, String_Format and the free-list allocator.
// Addresses are guest addresses in the BCES01284 v01.02 EBOOT. A "?" in a
// comment marks a guess from how the code is used.
#pragma once

#include <cstddef>

#include "opentit/native.h"
#include "../sly4.h"

namespace sly4 {
namespace gameload {

// ---------------------------------------------------------------- imports
namespace limp {
constexpr u32 cellPadSetPortSetting = 0x00ABDEA4;
constexpr u32 cellPadGetInfo2 = 0x00ABDEE4;
constexpr u32 sceNpManagerRegisterCallback = 0x00ABE464;
constexpr u32 sys_ppu_thread_get_id = 0x00ABF1E4;
}  // namespace limp

// ---------------------------------------------------------------- functions
// Decompiled in src/game/load*.cpp (called through call() so that switching
// one off with --decomp-off still works), or still recompiled.
namespace lfn {
// decompiled here
constexpr u32 Tuning_Load = 0x004FC250;          // (const char* path, bool keep_strings) -> bool
constexpr u32 Ui_Init = 0x004D35AC;              // (const ViewRect* or 0) -> 1
constexpr u32 Ui_Find = 0x004D5544;              // (const char* path, Element* parent or 0) -> Element*
constexpr u32 RenderTarget_Create = 0x00522FB8;  // (u32 type, bool flag) -> AglRenderTarget*
constexpr u32 RenderTarget_SetByte49 = 0x0052038C;   // (rt, u8)  (name?)
constexpr u32 RenderTarget_AddCallback = 0x00522500; // (rt, list, opd, user, s8 priority)
constexpr u32 RenderTarget_SetViewport = 0x005200AC; // (rt, const ViewRect*)
constexpr u32 InputMgr_CreatePad = 0x0056324C;   // () -> ApeInputDevice*
constexpr u32 InputDevice_Construct = 0x00562DDC;  // (InputDevice*) base constructor
constexpr u32 PadDevice_Construct = 0x0056FAC8;  // (InputDevice*) pad constructor
constexpr u32 FreeList_Init = 0x00551A58;        // (FreeList*, elem_size, count, heap or 0, bool)
constexpr u32 String_Format = 0x005534D0;        // (const char* fmt, ...) -> rotating static buffer
constexpr u32 Thread_IsMain = 0x005DB0A8;        // () -> bool
constexpr u32 Thread_IsSecond = 0x005DB13C;      // () -> bool  (name?)
constexpr u32 vsprintf = 0x00724588;             // (char* dst, const char* fmt, va_list) -> int

// still recompiled
constexpr u32 World_Construct = 0x0045CC18;      // (World*, 0): 6393 instructions (name?)
constexpr u32 Mem_New = 0x007A3DD8;              // operator new(u32 size)
constexpr u32 memmove = 0x0071C3F0;              // (dst, src, n)
constexpr u32 strcmp = 0x00723884;               // (a, b)
constexpr u32 String_Compare = 0x00555D94;       // (a, b): strcmp
constexpr u32 vprintf_core = 0x00724620;         // (OutputFn* opd, void* arg, fmt, va_list) -> int (name?)
constexpr u32 RenderTarget_Construct = 0x00520D88;   // (rt, type): "RenderTarget_Primary" (name?)
constexpr u32 RenderTarget_SetParams = 0x0053C9F8;   // (p, u32, u32, f32, f32, u32 at r8) (name?)
constexpr u32 FreeList_AddBlock = 0x005518FC;     // (FreeList*, bool) (name?)
constexpr u32 PtrVector_Init = 0x00551598;       // (Vector*, capacity, heap, 0, 0) (name?)
constexpr u32 PtrVector_Reserve = 0x00551718;    // (Vector*, new_capacity) (name?)
// XML reader (hashes are String_HashNoCase of the tag / attribute name)
constexpr u32 XmlDoc_Construct = 0x004FA888;     // (XmlDoc*, const char* path) (name?)
constexpr u32 XmlDoc_Parse = 0x004FAF10;         // (XmlDoc*, 0) -> bool (name?)
constexpr u32 XmlDoc_Destruct = 0x004FA9C0;      // (XmlDoc*) (name?)
constexpr u32 XmlCursor_FirstChild = 0x004F92BC;   // (node, tag hash) -> node or 0
constexpr u32 XmlCursor_NextSibling = 0x004F8FC8;  // (node, tag hash) -> node or 0
constexpr u32 XmlCursor_FindAttr = 0x004F9534;     // (node, attr hash) -> const char* or 0
constexpr u32 XmlCursor_FindAttrByName = 0x004F9628;  // (node, const char* name) -> const char* or 0
// XmlCursor_Read*(node, attr hash, out, default): store the value (or the
// default) in *out and return 1 when the attribute is missing, else 0.
constexpr u32 XmlCursor_ReadBool = 0x004F986C;       // u8 (bool?) (name?)
constexpr u32 XmlCursor_ReadS32 = 0x004F99BC;      // (name?)
constexpr u32 XmlCursor_ReadF32 = 0x004F9B20;      // default in f1 (name?)
constexpr u32 XmlCursor_ReadVec2 = 0x004F9C9C;    // 8-byte value, default by pointer (name?)
constexpr u32 XmlCursor_ReadVec3 = 0x004F9E0C;    // up to 16 bytes, default by pointer (name?)
constexpr u32 XmlCursor_ReadColor = 0x004F9F9C;    // byte buffer, default by pointer (name?)
constexpr u32 XmlCursor_ReadHash = 0x004FA14C;     // u32 hash of the attribute's string (name?)
}  // namespace lfn

// ---------------------------------------------------------------- globals
namespace lg {
// Game_Load
constexpr u32 kSysMsgShows = 0x010BA12C;      // u8[4]: set to 1 (see update.h)
constexpr u32 kSysMsgCallback = 0x00F65C8C;   // u32 OPD (see update.h)
constexpr u32 kWorld = 0x00F65CA0;            // u32: copy of Game +0 (update.h calls it kCurrentWorld)
constexpr u32 kSysMsgCallbackOpd = 0x00F39BD8;  // OPD of 0x004645AC
constexpr u32 kNpManagerCallbackOpd = 0x00F39E70;  // OPD of 0x0049AF38
// UI
constexpr u32 kMainView = 0x00F6684C;         // u32 AglRenderTarget* "RenderTarget_GUI"
constexpr u32 kUiRoot = 0x00F66868;           // u32 first top-level Element
constexpr u32 kUiViewRect = 0x010C2AE8;       // ViewRect
constexpr u32 kUiClear0 = 0x010C2AD8;         // u32[4], zeroed by Ui_Init
constexpr u32 kUiClear1 = 0x00F6687C;         // u32, zeroed by Ui_Init
constexpr u32 kUiClear2 = 0x00F66890;         // u32, zeroed by Ui_Init
constexpr u32 kUiDrawOpd0 = 0x00F3AA28;       // OPD of 0x004D5870 (just blr)
constexpr u32 kUiDrawOpd1 = 0x00F3AA30;       // OPD of 0x004D5874 (just blr)
constexpr u32 kRenderTargetGuiName = 0x00AEF488;  // "RenderTarget_GUI"
// Render targets
constexpr u32 kRenderTargetPool = 0x00F8F11C;  // u32 -> { FreeList* list; u32 count; } (set at run time)
constexpr u32 kRenderTargetFreeListVtbl = 0x00B23DC0;
constexpr u32 kRenderTargetFreeListName = 0x00AF122C;  // "ApeTFreeList<T> &ApeTListAllocator<T>::GetAllocator() [with T = AglRenderTarget]"
constexpr u32 kRenderTargetVtbl = 0x00B240D0;
// Input
constexpr u32 kInputMgr = 0x01164AA0;         // +0 FreeList* of devices, +80 u32 devices made
constexpr u32 kInputDeviceFreeListVtbl = 0x00B243F8;
constexpr u32 kInputDeviceFreeListName = 0x00AF382A;  // "static ApeTFreeList<ApeInputDevice> &ApeInputMgr::GetAllocator()"
constexpr u32 kInputDeviceVtbl = 0x00B24310;
constexpr u32 kPadDeviceVtbl = 0x00B244C0;
constexpr u32 kPadInfo = 0x0116B2A8;          // CellPadInfo2
// Free lists
constexpr u32 kDefaultHeapPtr = 0x00F88700;   // u32: &kDefaultHeap once made
constexpr u32 kDefaultHeap = 0x01165048;      // static object, vtable kDefaultHeapVtbl
constexpr u32 kDefaultHeapGuard = 0x01165040; // u8
constexpr u32 kDefaultHeapVtbl = 0x00B24278;  // 0x00B24270 + 8
constexpr u32 kFreeLists = 0x01165028;        // PtrVector of every FreeList
constexpr u32 kFreeListsGuard = 0x01165020;   // u8
// String_Format
constexpr u32 kFormatCountMain = 0x00F88710;  // u32, main thread (not atomic)
constexpr u32 kFormatBufMain = 0x01165050;    // char[8][1024]
constexpr u32 kFormatCountSecond = 0x01164F80, kFormatGuardSecond = 0x01164F00;
constexpr u32 kFormatBufSecond = 0x01167050;
constexpr u32 kFormatCountOther = 0x01164E80, kFormatGuardOther = 0x01164E60;
constexpr u32 kFormatBufOther = 0x01169050;
constexpr u32 kSprintfOutput = 0x00F486A8;    // OPD of 0x007245E4: writes into a char buffer
// Thread ids: the function-local static Main_InitSystems fills
// ({ u64 main; u64 second; }, guard byte after it).
constexpr u32 kThreadIds = 0x011DE290, kThreadIdsGuard = 0x011DE2A0;
// Tuning variables
constexpr u32 kTuningVars = 0x010C2DBC;       // TuningVar*[2800]
constexpr u32 kTuningVarCount = 0x010C597C;   // u32
constexpr u32 kTuningDefaultType3 = 0x01164AFC;
constexpr u32 kTuningDefaultType4 = 0x01164B40;
constexpr u32 kTuningDefaultType5 = 0x01164CA0;
}  // namespace lg

// ---------------------------------------------------------------- objects
// Game (g::game) fields written by Game_Load (see also update.h).
constexpr u32 kGameWorld = 0x0000;       // u32 World*
constexpr u32 kGameSysMsgUi = 0x0004;    // u32 Element*
constexpr u32 kGameFrameArena = 0x00A0;  // u32 Heap* of 0xE000 bytes
constexpr u32 kGameFrameTime = 0x00A4;   // f32 = 1/60
constexpr u32 kGamePlayers = 0x00B0;     // Player[2]
constexpr u32 kPlayerSize = 0x2EC0;
constexpr u32 kPlayerIndex = 4;          // u8: 0 or 1, set by Game_Load
constexpr u32 kPlayerPad = 28;           // u32 InputDevice*
constexpr u32 kGameCursors = 0x5E30;     // u32 Element*[2]
constexpr u32 kGameTuningStrings = 0x000A;  // u8: Tuning_Load keeps string values
constexpr u32 kWorldSize = 0x1286B0;
constexpr u32 kFrameArenaSize = 0xE000;

// UI element (Ui_Find).
constexpr u32 kElemNameHash = 128;   // u32 String_HashNoCase of the name
constexpr u32 kElemParent = 148;     // u32 Element*
constexpr u32 kElemFirstChild = 152; // u32 Element*
constexpr u32 kElemNext = 156;       // u32 Element*

// Rectangle given to RenderTarget_SetViewport.
struct ViewRect {
    be_u16 x, y, width, height;
};
static_assert(sizeof(ViewRect) == 8);

// AglRenderTarget fields.
constexpr u32 kRtName = 16;          // char[32]
constexpr u32 kRtTag = 48;           // u8, copied into each callback entry
constexpr u32 kRtByte49 = 49;        // u8
constexpr u32 kRtFlags = 52;         // u8: 0x40 = viewport covers the whole screen
constexpr u32 kRtViewport = 96;      // ViewRect, clamped to the surface
constexpr u32 kRtViewport2 = 104;    // ViewRect, clamped again
constexpr u32 kRtSurface = 512;      // u32 -> surface: +56 s32 width, +60 s32 height
constexpr u32 kRtCallbacks = 520;    // CallbackList[], 68 bytes each
constexpr u32 kRtFlags2 = 1776;      // u8: 0x80 = RenderTarget_Create's flag
constexpr u32 kRtParams = 1784;      // RenderTarget_SetParams block
constexpr u32 kRtU64_1808 = 1808, kRtByte1816 = 1816, kRtF1824 = 1824, kRtF1828 = 1828;
constexpr u32 kRtSize = 1840;
// A callback list: 4 entries of 16 bytes, sorted by priority (highest first), count at +64.
struct RtCallback {
    gptr opd;
    be_u32 user;
    be_u32 tag;
    s8 priority;
    u8 pad[3];
};
static_assert(sizeof(RtCallback) == 16);
constexpr u32 kRtCallbackListSize = 68, kRtCallbackCount = 64;

// ---------------------------------------------------------------- native structures (load_ui.cpp)
struct RtSurface {
    native::pad<56> _0;
    be_s32 width;    // +56
    be_s32 height;   // +60
};
struct RtCallbackList {
    RtCallback entries[4];  // +0
    u8 count;               // +64
    native::pad<3> _65;
};
static_assert(sizeof(RtCallbackList) == kRtCallbackListSize);
struct RenderTarget {
    be_u32 vtable;                   // +0
    native::pad<12> _4;
    char name[32];                   // +16
    u8 tag;                          // +48 copied into each callback entry
    u8 byte49;                       // +49
    native::pad<2> _50;
    u8 flags;                        // +52 0x40 = viewport covers the whole screen
    native::pad<43> _53;
    ViewRect viewport;               // +96 clamped to the surface
    ViewRect viewport2;              // +104 clamped again
    native::pad<400> _112;
    native::gp<RtSurface> surface;   // +512
    native::pad<4> _516;
    RtCallbackList lists[3];         // +520
    native::pad<1052> _724;
    u8 flags2;                       // +1776 0x80 = RenderTarget_Create's flag
    native::pad<7> _1777;
    u8 params[20];                   // +1784 RenderTarget_SetParams block
    native::pad<4> _1804;
    be_u64 u1808;                    // +1808
    u8 b1816;                        // +1816
    native::pad<7> _1817;
    be_f32 f1824;                    // +1824
    be_f32 f1828;                    // +1828
    native::pad<8> _1832;
};
static_assert(sizeof(RenderTarget) == kRtSize && offsetof(RenderTarget, surface) == kRtSurface &&
              offsetof(RenderTarget, lists) == kRtCallbacks && offsetof(RenderTarget, flags2) == kRtFlags2 &&
              offsetof(RenderTarget, params) == kRtParams && offsetof(RenderTarget, f1828) == kRtF1828);

// A UI element as Ui_Find sees it.
struct FindElem {
    native::pad<128> _0;
    be_u32 name_hash;                // +128 String_HashNoCase of the name
    native::pad<16> _132;
    native::gp<FindElem> parent;     // +148
    native::gp<FindElem> first_child;  // +152
    native::gp<FindElem> next;       // +156
};
static_assert(offsetof(FindElem, name_hash) == kElemNameHash && offsetof(FindElem, next) == kElemNext);

// Native functions (src/game/load_ui.cpp).
u32 Ui_Init(const ViewRect* viewport);
FindElem* Ui_Find(const char* path, FindElem* parent);
RenderTarget* RenderTarget_Create(u32 type, u32 flag);
void RenderTarget_SetByte49(RenderTarget* rt, u8 value);
void RenderTarget_AddCallback(RenderTarget* rt, u32 list, u32 opd, u32 user, u8 priority);
void RenderTarget_SetViewport(RenderTarget* rt, const ViewRect* rect);

// ApeTFreeList<T> (60 bytes). vtable +12: Alloc() -> T* or 0.
constexpr u32 kFreeListHeap = 40, kFreeListElemSize = 44, kFreeList48 = 48;
constexpr u32 kFreeListBlockCount = 52;  // u16
constexpr u32 kFreeListFlags = 54;       // u16: bit 0x8000 cleared at construction
constexpr u32 kFreeListName = 56;        // const char*
constexpr u32 kFreeListSize = 60;
constexpr u32 kFreeListVfnAlloc = 12;

// The constructor of ApeTFreeList<T>, inlined into every GetAllocator().
// (Defined after lfn so it can call FreeList_Init.)
inline void FreeList_Construct(PPUContext& ctx, u32 list, u32 vtable, u32 name, u32 elem_size, u32 count) {
    for (u32 off = 4; off <= 48; off += 4) field<u32>(list, off) = 0;
    field<u16>(list, 52) = 0;
    field<u16>(list, kFreeListFlags) &= static_cast<u16>(~0x8000);
    field<u32>(list, 0) = vtable;
    field<u32>(list, kFreeListName) = name;
    call(ctx, lfn::FreeList_Init, {list, elem_size, count, 0, 0});
}

// call() for a callee returning a 32-bit value; the original only ever
// looks at the low word of r3 (cmpwi / stw).
inline u32 call32(PPUContext& ctx, u32 addr, std::initializer_list<u64> args = {}) {
    return static_cast<u32>(call(ctx, addr, args));
}

// The OPD in slot `byte_offset / 4` of obj's vtable.
inline u32 vfunc(u32 obj, u32 byte_offset) { return load<u32>(load<u32>(obj) + byte_offset); }

// PtrVector (kFreeLists): +0 u32* data, +4 u16 capacity, +6 u16 element size,
// +8 u16 count, +10 u16 high-water mark, +12 u32 (bytes << 3 | flags), +16 u32.

// CellPadInfo2 / cellPad constants.
constexpr u32 CELL_PAD_SETTING_PRESS_ON = 0x2;
constexpr u32 CELL_PAD_SETTING_SENSOR_ON = 0x4;
constexpr u32 CELL_PAD_STATUS_CONNECTED = 0x1;
constexpr u32 kPadInfoPortStatus = 12;  // u32[7]

// InputDevice fields.
constexpr u32 kDevFlags = 4;   // u16: 0x8000 connected, 0x4000 ?
constexpr u32 kDevPort = 12;   // u8
constexpr u16 kDevConnected = 0x8000, kDevFlag4000 = 0x4000;

// Tuning variable (entries of lg::kTuningVars). Virtual functions by byte
// offset into the vtable: +8 type(), then one setter per type.
constexpr u32 kVarNameHash = 68;  // u32
constexpr u32 kVarName = 72;      // String, assigned when keep_strings is set
enum TuningType : s32 {
    kTuningU8 = 0,      // set +12 (u8)
    kTuningS32 = 1,     // set +16 (s32)
    kTuningF32 = 2,     // set +20 (f32 in f1)
    kTuningVec2 = 3,    // set +24 (pointer to 2 floats)
    kTuningVec3 = 4,    // set +28 (pointer to 3 floats)
    kTuningColor = 5,   // set +32 (pointer to 4 bytes)
    kTuningString = 6,  // set +36 (const char*)
    kTuningHash = 7,    // set +40 (u32)
};
constexpr u32 kVarVfnType = 8;

}  // namespace gameload
}  // namespace sly4
