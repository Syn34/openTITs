// The renderer range's next recompiled functions (gfx22*.cpp): the larger
// remaining functions of 0x004FB000 - 0x005E0000 (material record apply,
// ...).
#pragma once

#include "gfx21.h"

namespace sly4::render {
namespace gfx22 {

namespace fn {
// Still recompiled or decompiled elsewhere (called from gfx22*.cpp).
constexpr u32 Map_Find = 0x0079DD10;           // (map, const u32* key) -> value slot (gfx14.h)
constexpr u32 Shader_FindByHash = 0x004FE990;  // (hash) -> Shader* (gfx/shadergroup.cpp)
constexpr u32 SlotsWork_Prepare = 0x005894F8;  // () (gfx/gfx21.cpp)
constexpr u32 View_GetCurrent = 0x004FDBA8;    // () -> View* (view/view2.cpp)
constexpr u32 Mat4_Invert = 0x00554C28;        // (f32 tmp[16], f32 out[16], const f32 in[16])
constexpr u32 Render_FrameAlloc = 0x004FD1E0;  // (u32 size) -> p (obj/obj22.cpp)
constexpr u32 Mem_Alloc = 0x0054ED38;          // (size, pool, align) -> p
constexpr u32 PtrVector_Reserve = 0x00551718;  // (PtrVector*, u32 capacity) (container/vector.cpp)
constexpr u32 Shader_Construct = 0x0050AFDC;   // (Shader*) (gfx/shader4.cpp)
constexpr u32 String_Copy = 0x00553684;        // (dst, src, size)
constexpr u32 Target_Init = 0x00530DB4;        // (Target** out, w, h, format, aa, where) (gfx/target.cpp)
constexpr u32 Target_InitDepth = 0x00530A9C;   // (Target** out, w, h, format, aa) (name?)
constexpr u32 Target_Attach = 0x00532E70;      // (Target** out, Target** src, w, h, -, y) (gfx/target.cpp)
constexpr u32 Surface_SetBuffer = 0x00532D5C;  // (Surface*, Target*) (gfx/target.cpp)
constexpr u32 Texture_InitLayout = 0x00536E3C; // (layout, w, h, 1, format, 1, 0, 0) (font/image2.cpp)
constexpr u32 Mem_Allocator552588 = 0x00552588; // () -> Allocator* (obj/obj14_small.cpp)
constexpr u32 Mem_DefaultAllocator = 0x00550EE8; // () -> Allocator* (obj/obj14_small.cpp)
constexpr u32 Array_Init = 0x00558F38;         // (Array*) (obj/obj13_misc.cpp)
constexpr u32 Array_Reserve = 0x00558F3C;      // (Array*, u32 capacity)
constexpr u32 Array_Destruct = 0x0054EC94;     // (Array*)
constexpr u32 FreeList_Init = 0x00551A58;      // (FreeList*, size, heap, 0, 0)
constexpr u32 Obj_GetField4 = 0x00556BF0;      // (list) -> its first item (obj/obj13_misc.cpp)
constexpr u32 MapB_Find = 0x007A6BEC;          // (map, const u32* key) -> value slot (obj/obj2.cpp)
constexpr u32 MapB_Remove = 0x007A7A80;        // (map, const u32* key) (obj/obj12_small.cpp)
constexpr u32 WorldObj_CopyFrom = 0x00581B34;  // (dst, src, u32 flags) (obj/obj5.cpp)
constexpr u32 WorldObj_SetFlag4000 = 0x0057FDB4; // (obj, u8 on, bool recurse) (obj/obj11_small.cpp)
constexpr u32 WorldObj_ForEach = 0x0057F5D8;   // (obj, fn OPD, arg, bool recurse) (obj/obj5.cpp)
constexpr u32 Tree_Collect = 0x0058E990;       // (Collector*, Node*) (gfx/gfx20.cpp)
constexpr u32 Node_SyncTree = 0x0058E680;      // (Scene*, Node*) (gfx/gfx20.cpp)
constexpr u32 WorldObj_UpdateLod = 0x0057EE00; // (obj, -, bool keep_skel_sphere) (obj/obj8_lod.cpp)
constexpr u32 WorldObj_UpdateZoneMask = 0x00582C6C; // (obj, bool lookup) (obj/obj4.cpp)
constexpr u32 InputDevice_UpdateButtons = 0x00562F18;  // (InputDevice*) (obj/obj14_small.cpp)
constexpr u32 InputDevice_SetButtonMask = 0x0056F6A8;  // (InputDevice*, u32 mask) (obj/obj15_small.cpp)
constexpr u32 Pad_MapButtons = 0x0056F7D8;     // (-, u32 raw) -> mask (obj/obj15_small.cpp)
constexpr u32 memset = 0x00726E54;
constexpr u32 cellGcmGetVBlankCount = 0x00ABD6C4;  // import
constexpr u32 cellPadGetInfo2 = 0x00ABDEE4;    // import
constexpr u32 cellPadGetData = 0x00ABDEC4;     // import
constexpr u32 String_Format = 0x005534D0;      // (fmt, ...) (game/load_util.cpp)
constexpr u32 String_HashNoCase = 0x0056A710;  // (const char*) -> u32
constexpr u32 Pool_AllocEntry = 0x0079C07C;    // (EntryPool*) (gfx/gfx12.cpp)
constexpr u32 Lock_Enter = 0x005E6CC0;         // (lock*) (name?)
constexpr u32 Lock_Leave = 0x005E6C68;         // (lock*) (name?)
constexpr u32 sceNpTrophyRegisterContext = 0x00ABE5C4;
constexpr u32 sceNpTrophyGetRequiredDiskSpace = 0x00ABE604;
constexpr u32 sceNpTrophyGetGameInfo = 0x00ABE664;
constexpr u32 sceNpTrophyUnlockTrophy = 0x00ABE6A4;
constexpr u32 sceNpTrophyGetTrophyUnlockState = 0x00ABE6E4;
constexpr u32 sceNpTrophyGetTrophyInfo = 0x00ABE724;
constexpr u32 cellGameContentErrorDialog = 0x00ABE504;
constexpr u32 WorldObj_CanBatch = 0x0059578C;  // (Obj*) -> bool (obj/obj15_small.cpp)
constexpr u32 String_Assign = 0x00555F80;      // (String*, const char*)
constexpr u32 Matrix_ScaleRows = 0x00554394;   // (f32 m[12], const f32 scale[3]) (view/view2.cpp)
constexpr u32 Sphere_Transform = 0x00557900;   // (f32 sphere[4], const Matrix*, bool scale_radius) (obj/obj8_math.cpp)
constexpr u32 RenderTarget_ColourTexture = 0x0051E514;  // (Surface*, Target*) -> Texture* (gfx/gfx11.cpp)
constexpr u32 RenderTarget_SetByte49 = 0x0052038C;     // (o, u8) (game/load_ui.cpp)
constexpr u32 ShadowSource_Create = 0x0052317C;        // (Texture*) -> source (gfx/gfx11.cpp)
constexpr u32 Stats_ResolveTexture = 0x00536D28;       // (MemStats*) -> Target* (gfx/layers4_draw.cpp)
constexpr u32 Gcm_PadNops = 0x005E0AF0;        // (context, u32 words) (libgcm, name?)
constexpr u32 Gcm_SetCall = 0x005E0600;        // (context, u32 offset) (libgcm, name?)
constexpr u32 Draw_InvalidateState = 0x0051279C; // () (gfx/render_state.cpp)
constexpr u32 DrawQueue_Run = 0x00516CD0;      // (DrawQueue*) (gfx/gfx11.cpp)
constexpr u32 Draw_Lists = 0x00517488;         // (u32 which)
constexpr u32 Render_Pass575C34 = 0x00575C34;  // ()
constexpr u32 SortEntries = 0x0056A81C;        // (Entry*, u32 count, u32 size)
constexpr u32 PtrVector_Construct = 0x005516E0; // (PtrVector*)
constexpr u32 cellGcmAddressToOffset = 0x00ABD5E4;  // import
}  // namespace fn

// Material_Apply_0050C3D8: the material chunk's record (gfx19.h's
// MatField layout) into an AglMaterial. Material: +16 Shader*, +20
// Texture*[] (count u8 +44), +40 name hash, +45 u8 sort class, +48 u16
// the first texture's +112, +64 / +80 f32[4] diffuse / specular colour,
// +96 f32 specular power, +100 u8 flags (0x80 record bit 0, 0x40 bit 2,
// 0x20 bit 1, 0x10 bit 5), +101 u8 (8 with flag 0x10), +104 the render
// state words (kMatState*).
namespace mat {
constexpr u32 kShader = 16, kTextures = 20, kNameHash = 40, kTexCount = 44, kSortClass = 45, kTexWord = 48;
constexpr u32 kDiffuse = 64, kSpecular = 80, kSpecPower = 96, kFlags = 100, kFlags2 = 101, kState = 104;
// The state block (Material +104): +0 u32 blend (bit 31 enable, bits
// 15..30 source factor), +4 u16 equation, +6 u16 destination factor, +8
// u32 depth (bit 31 write, bits 15..30 function), +12 u16 cull face.
constexpr u32 kStBlend = 0, kStEquation = 4, kStDest = 6, kStDepth = 8, kStCull = 12;
// The record (Iff_HandleMaterial_0051C704's 232 bytes).
constexpr u32 kRecDiffuseHash = 0, kRecNameHash = 20, kRecBlend = 56, kRecCull = 57, kRecFlags = 58;
constexpr u32 kRecDiffuse = 59, kRecSpecular = 63, kRecSpecPower = 68, kRecClampU = 72, kRecClampV = 73;
constexpr u32 kRecSortLayer = 74, kRecShaderHash = 80;
constexpr u32 kTexMapHolder = 0x010DDADC;    // +0 the default texture, +24 the texture map (by hash)
constexpr u32 kColourScale = 0x0050C3D4;     // f32 255
constexpr u32 kBlendModes = 0x00F66EBC;      // u32[5]: record blend mode -> case
constexpr u32 kCullFaces = 0x00F66ED0;       // u32[3] (CELL_GCM_CW 0x900, 0x900, 0)
constexpr u32 kWrapModes = 0x00F66EDC;       // u32[3]: clamp mode -> texture wrap (1, 3, 2)
constexpr u32 kDefaultShaderHash = 0x00F66FB4;
constexpr u32 kBlendOne = 1, kBlendZero = 0, kBlendSrcAlpha = 770, kBlendOneMinusSrcAlpha = 771;
constexpr u32 kFuncAdd = 0x8006, kFuncReverseSubtract = 0x800B;
constexpr u32 kDepthLequal = 515, kDepthAlways = 519;
// Texture: vfunc +8 release, +16 SetWrap(u, v, 1); u16 +4 references.
constexpr u32 kTexRefs = 4, kTexVRelease = 8, kTexVSetWrap = 16;
// Shader: vfunc +4 Bind(material, record) -> bool. Material vfunc +12.
constexpr u32 kShaderVBind = 4, kMatVNoShader = 12;
}  // namespace mat

// SlotsWork_Build_00599154: the slots job's view block, filled by the view
// work's object (vfunc +24) and copied into the slots work (gfx21 sw::).
// The block (216 bytes, on the stack): +0 the inverse of the view's +1008
// matrix, +64 the inverse of +880 times kUvToClip (rows 0-2 without, row 3
// with the inverse's row 3), +128 the view's +816 3x4 rows (xyz), +192
// its +864 xyz, +208 f32 view +60, +212 f32 work +84 - view +60. The
// object's results (+112: 6 rows of 16 bytes, +208: 2 matrices) go to
// kWork +3232 / +3328 (zeros when the view is off or the work's flag 0x80
// at +96 is set), then kWork +3456 / +3460 the frame block's end and
// kSlotsAux.
namespace swb {
constexpr u32 kViewWork = 0x0118E088;    // u32 -> {+4 Obj*, +84 f32, +96 u8 flags}
constexpr u32 kUvToClip = 0x010F0A50;    // f32[4][4] (layers3::g::uv_to_clip)
constexpr u32 kBlockPtr = 0x00F8893C;    // u32: the frame block (128 bytes per entry)
constexpr u32 kBlockCount = 0x00F88940;  // u32
constexpr u32 kSlotsAux = 0x01192700;    // u32: a copy of the block pointer
constexpr u32 kVfArg = 0x0118E0B0;       // passed to the object's vfunc +24 (name?)
constexpr u32 kWork = 0x0117E280;        // gfx21 sw::kWork
constexpr u32 kViewOn = 256;             // view u32 +256 bit 0
constexpr u32 kViewInvA = 880, kViewInvB = 1008, kViewRows = 816, kViewPos = 864, kViewF60 = 60;
constexpr u32 kObjRows = 112, kObjMats = 208, kObjVf = 24;
constexpr u32 kWorkRows = 3232, kWorkMats = 3328, kWorkEnd = 3456, kWorkAux = 3460;
}  // namespace swb

// ThermalObjectType_Create_0054D4C0: the PostProc_ThermalObject shader
// type's Create (shader3_types.cpp's pattern with Shader_Load inlined):
// per entry of kThermalTable (12 bytes {+0 name, +4 skinned name, +8
// fragment program}) two 44-byte shaders with vtable kThermalVtbl, the
// fragment program bound with kThermalBindings.
namespace thermal {
constexpr u32 kTable = 0x00F67508, kEntries = 2;
constexpr u32 kBindings = 0x00F67520;  // "gps_vObjSpcEyePos", ...
constexpr u32 kVtbl = 0x00B24190;
constexpr u32 kShaderSize = 44, kName = 12, kNameSize = 32, kVProgram = 4, kFProgram = 8;
constexpr u32 kShaders = 12, kCapacity = 16, kCount = 20, kHighWater = 22;  // ShaderType: PtrVector of Shader*
constexpr u32 kVpAttribMask = 10;  // VProgram u16
}  // namespace thermal

// Targets_InitReduced_005467DC: the reduced-size render targets (quarter
// and eighth of the config's +4 / +6 size, in 0x01164038..0x011640DC) and
// the function-local static surfaces they are attached to (each {u64
// guard; 52-byte Surface at +8}: gfx19.h's kRenderStatics).
namespace reduced {
constexpr u32 kConfig = 0x010C5A20;          // +4 u16 width, +6 u16 height
constexpr u32 kStaticArray = 0x011642C0;     // gfx12 static_targets: 5 x 52-byte surfaces
constexpr u32 kStaticArrayGuard = 0x011645C8;
constexpr u32 kStaticArrayCount = 5, kSurfaceSize = 52;
constexpr u32 kQuarter = 0x01164038, kEighth = 0x0116403C;  // Target* (Target_Init)
constexpr u32 kQuarterDepth = 0x01164078, kHalfDepth = 0x011640D8;  // Target* (Target_InitDepth)
constexpr u32 kHalfTexture = 0x011643D0;     // the 52-byte static surface laid out as a texture
constexpr u32 kHalfTextureGuard = 0x011643C8;
constexpr u32 kHalfTextureRef = 0x011640D0, kHalfTextureTarget = 0x011640D4;
constexpr u32 kTargetFormat = 3, kDepthFormat = 7;
// The {Surface*, Target*} pairs: the surface's static (guard 8 before
// it), the pair's address, the target attached, and the slice (in
// target heights) and store order of the inlined constructor.
struct Attach {
    u32 surface, pair, target, slice;
    bool alt_order;  // the guard stored between +40 and +12
};
}  // namespace reduced

// GeomInstances_Instantiate_0058EA70: the geometry instances (the
// ApeGeomInstance free list, gfx21 kGeomInstClass; items 352 bytes, the
// instance 16 in: +0 the list link) and a local map (key -> instance, a
// sorted Array of {u32 key, Inst*} on the stack: +0 u16 capacity, +2 u16
// count, +4 u16 high water, +6 u16 element size << 1 | unsorted, +8 data,
// +12 Allocator*, +16 0, +20 the compare OPD). Instance: +8 flags (0x400
// named), +12, +164 flags (0x8000 / 0x4000), +168 flags (0x20000), +204
// key, +208 def (+96 the key of the instance it copies), +268 / +272
// sibling / first child, +292, +320 bits 9..12, +322 bit 0x20 done.
namespace inst {
constexpr u32 kItemSize = 352, kLink = 16;
constexpr u32 kFlags8 = 8, kNamed = 0x400, kWord12 = 12, kFlags164 = 164, kFlags168 = 168, kKey = 204, kDef = 208;
constexpr u32 kDefKey = 96, kSibling = 268, kChild = 272, kArg292 = 292, kWord320 = 320, kDone322 = 322;
constexpr u32 kDoneBit = 0x20;
constexpr u32 kMapCompare = 0x00F3CBB8;  // OPD
constexpr u32 kForEachFn = 0x00F3C4E0;   // OPD passed to WorldObj_ForEach
constexpr u32 kClassMap = 16;            // kGeomInstClass +16: the named map
// Instance vfuncs: +20 AddChild(child, -1), +40 (u8, 1), +56 Clone(21);
// the clone's +8 releases it.
constexpr u32 kVAddChild = 20, kVSet40 = 40, kVClone = 56, kVRelease = 8;
// The instance classes' list holders {+0 ApeTFreeList*, +4 heap}, their
// free lists' vtables, GetAllocator() names and item sizes (obj18.h's
// InstList).
struct InstClass {
    u32 holder, vtbl, name, size;
};
constexpr InstClass kLights = {0x00F8F148, 0x00B13920, 0x00AF4BCF, 400};    // ApeLightInstance
constexpr InstClass kGeoms = {0x00F8F144, 0x00B0F008, 0x00AF4B7E, 352};     // ApeGeomInstance
constexpr InstClass kEmitters = {0x00F8F14C, 0x00B10EF0, 0x00AF4C23, 352};  // ApeEmitterInstance
constexpr InstClass kPoints = {0x00F8F154, 0x00B13900, 0x00AF4CCD, 288};    // ApePointInstance
constexpr InstClass kCameras = {0x00F8F150, 0x00B24EB8, 0x00AF4C77, 352};   // ApeCameraInstance
constexpr InstClass kSplines = {0x00F8F158, 0x00B0EFE8, 0x00AF4D21, 288};   // ApeSplineInstance
constexpr u32 kLodSet = 260;  // an instance with a LOD target set skips WorldObj_UpdateLod
}  // namespace inst

// DeferredLists_Flush_00517880: the deferred command segments. Each
// segment {+0 u32* the jump word to patch, +8 u32 the offset its commands
// return to} is set while it waits for its commands; the flush records
// them at the current command pointer (a JUMP, 0x20000000 | offset,
// patched into the jump word) and calls back. kSegments lists them in
// the original's order with their stack slots.
namespace defer {
constexpr u32 kPending = 0x00F66F94;     // u8: set while there is something to flush
constexpr u32 kBusy = 0x010DD400;        // u32: waited on (layers3 atomic_dd400)
constexpr u32 kStreams = 0x010DDA90;     // Stream*[] (gfx21 gj::kStreams): [0] +12 u8 bit 0x40 cleared
constexpr u32 kGcmContext = 0x010F10A0;  // CellGcmContextData* (+8 current)
constexpr u32 kStateStamp = 0x010F113C;  // u32 bumped with each Draw_InvalidateState (kGcmContext +156)
constexpr u32 kJump = 0x20000000;        // CELL_GCM_JUMP (old-style jump)
constexpr u32 kSleepUs = 300, kSysTimerUsleep = 141;
enum class Body { Stream0, Records, Sorted, Overlay, Lists1, Lists2, Lists3, Pass };
struct Segment {
    u32 addr;
    Body body;
    u32 slot_start, slot_end;  // the two cellGcmAddressToOffset outputs (sp +)
};
constexpr Segment kSegments[8] = {
    {0x010DD480, Body::Stream0, 172, 168}, {0x010DD490, Body::Records, 164, 160}, {0x010DD4A0, Body::Sorted, 156, 152},
    {0x010DD4E0, Body::Overlay, 148, 144}, {0x010DD4B0, Body::Lists1, 140, 136}, {0x010DD4C0, Body::Lists2, 132, 128},
    {0x010DD4D0, Body::Lists3, 124, 120}, {0x010DD4F0, Body::Pass, 116, 112},
};
// The draw record list (gfx21 gj::kRecList; made here inline on first use).
constexpr u32 kRecList = 0x011DE300, kRecListGuard = 0x011DE480;
// The sorted entries {u32 key, Obj*} (8 bytes): each object's +64
// drawer gets vfunc +12(obj, +68, +72, kSortedBase + (+76 << 5)).
constexpr u32 kSorted = 0x010EF63C, kSortedCount = 0x010F0A3C;
constexpr u32 kSortedWait = 0x010DDC00;  // u32 waited on before drawing them
constexpr u32 kSortedBase = 0x00F66FD8;  // u32: 32-byte records
constexpr u32 kOverlayOn = 0x00F66FD4, kOverlayObj = 0x00F66FD0;  // u8, Obj* (vfunc +16)
}  // namespace defer

// Pad_ReadState_0056FC28: the pad device (an InputDevice, gfx21's pad
// fields) read from cellPadGetData into its +152 CellPadData {s32 len;
// u16 button[64]}: +4 u16 bit 15 connected, +8 u32 (4 without the
// device_capability bit), +16 button mask, +20 / +24 cleared, +32..44
// the sticks (LX, LY, RX, RY), +48..92 the 12 pressures, +104..116 the 4
// sensors, +128 u32 the valid-value mask.
namespace pad {
constexpr u32 kInfoBlock = 0x0116B2A0;   // +4 u32 the last vblank, +8 CellPadInfo2 (game/load.h kPadInfo)
constexpr u32 kLastVblank = 4, kInfo = 8;
constexpr u32 kPortStatus = 20, kPortSetting = 48, kCapability = 76;  // kInfoBlock + (CellPadInfo2 +12 / +40 / +68)
constexpr u32 kStatusConnected = 1;      // CELL_PAD_STATUS_CONNECTED
constexpr u32 kSettingPress = 2, kSettingSensor = 4;  // CELL_PAD_SETTING_PRESS_ON / SENSOR_ON
constexpr u32 kConsts = 0x0056FC10;      // f32 117, 0, 255, 0.01, 511, 1
constexpr u32 kDevConnected = 4, kDevWord8 = 8, kDevPort = 12, kDevButtons = 16, kDevSticks = 32;
constexpr u32 kDevPress = 48, kDevSensors = 104, kDevValid = 128, kDevData = 152;
constexpr s32 kDeadZone = 10, kStickCentre = 128, kSensorCentre = 512;
// The pressure buttons in the order they are read and the device slots
// they go to (CellPadData button[] word offsets from +4).
constexpr u32 kPressRead[12] = {24, 26, 22, 20, 28, 34, 30, 32, 36, 38, 40, 42};
}  // namespace pad

// Trophy_ThreadMain_004FB704: the trophy thread (meta.h's TrophyThread,
// started with the manager + 24 as its argument). Fields of that
// argument: +68 u8 quit, +69 u8 registered, +72 / +76 OPDs called with a
// newly unlocked platinum / trophy id, +80 the status callback OPD
// (free space: (u64* need, u64* have, phase)), +84 u32 state, +132 / +136
// the context and handle (manager +156 / +160), +144 u32[] unlocked bits.
// Manager (meta::kTrophy): +8 u8 ready, +9 / +10 / +12 / +13 u8 flags,
// +11 u8 build the list, +16 u64 the required space, +24 the pending id
// pool (+32 index, +40 count, +52 entries of 16 bytes: +8 the id), +60 /
// +112 locks, +140 the unlock state copy (16 bytes), +164 pending, +200
// SceNpTrophyGameDetails, +1376 Array of {name hash, id}.
namespace trophy {
constexpr u32 kManager = 0x00F668E8;
constexpr u32 kQuit = 68, kRegistered = 69, kOnPlatinum = 72, kOnUnlock = 76, kOnStatus = 80, kState = 84;
constexpr u32 kContext = 132, kHandle = 136, kBits = 144;
constexpr u32 kReady = 8, kSpaceKnown = 9, kListDone = 10, kWantList = 11, kDialog = 12, kIdle = 13;
constexpr u32 kRequired = 16, kPool = 24, kPoolIndex = 32, kPoolCount = 40, kPoolEntries = 52, kLockPool = 60;
constexpr u32 kAbort = 94, kLockState = 112, kStateCopy = 140, kMgrContext = 156, kMgrHandle = 160, kPending = 164;
constexpr u32 kDetails = 200, kList = 1376;
constexpr u32 kStatusCallback = 0x00F3B2A8;  // OPD given to sceNpTrophyRegisterContext
enum State : s32 { kRegister = 0, kListTrophies = 1, kSpace = 2, kCheckSpace = 3, kReadState = 4, kRun = 5, kStop = 6 };
constexpr u32 kSleepUs = 1000, kRegisterDelay = 5;  // sys_timer_usleep / sys_timer_sleep
constexpr u32 kSysTimerUsleep = 141, kSysTimerSleep = 142;
constexpr u32 kErrorDialogNoSpaceExit = 102;  // cellGameContentErrorDialog type
// sceNpTrophy results.
constexpr u32 kErrAlreadyUnlocked = 0x80022910, kErrPlatinumUnlocked = 0x80022915;
constexpr u32 kErrCannotUnlockPlatinum = 0x80022914, kErrInvalidTrophyId = 0x8002292C, kErrHidden = 0x80022913;
constexpr u32 kStrNotExist = 0x00AF0FC8, kStrTooMany = 0x00AF0F54, kStrUnlockFail = 0x00AF0FF0;
constexpr u32 kStrPlatinum = 0x00AF0FA0, kStrInfoFail = 0x00AF0F7C;
}  // namespace trophy

// GeomBatches_Build_00595968: the static geometry batches (32 at most;
// 272 bytes each from kBatches, count kBatchCount) built from the
// batchable geometry instances (WorldObj_CanBatch), one per def. Batch:
// +0 f32 radius, +4 f32 1 / def +120, +8 radius^2, +12 def +124, +16
// f32[4] the def's sphere (+48), +32 String name (def vfunc +12), +64
// instance count, +68 written, +72 the instance records (80 bytes:
// matrix rows xyz, then the sphere), +76..+104 the material's render
// state (+104 0x8006 / ...), +108 instance +321 bit 0, +112 material +100
// bit 0x40, +128..+168 the material's +124 colours, +176 the texture +56,
// +180..196 its u16s, +200 / +204 the shader's, +208 sub-mesh count,
// then per sub-mesh +212 u8 / +216 SubMesh* / +232 4-byte pointers /
// +240 / +248 / +256, +224 / +228 squared radii, +264 / +268.
namespace batch {
constexpr u32 kBatches = 0x0116BF00, kBatchSize = 272, kBatchCount = 0x0116E100;
constexpr u32 kBusy = 0x0116BD05;  // u8 set while building (obj15 kDirtyListOff2), restored after
constexpr u32 kOne = 0x0059595C, kZero = 0x00595960;  // f32 1, 0
constexpr u32 kInstBatchSet = 276, kInstBatch = 280, kInstMatrix = 16, kInstScale = 144, kInstFlags321 = 321;
constexpr u32 kRecordSize = 80;
// The stack: the batch defs (+176, 4 bytes each; 32 fit before) the
// instances collected (+304).
constexpr u32 kFrame = 8576, kDefs = 176, kInsts = 304;
}  // namespace batch

// Reflection_Create_005A9DB0: a reflection source (200-byte records from
// kRecords, counter kCount): its size from the object's def box aspect
// within the config size, three mip levels each {colour texture at +28,
// a 52-byte Surface at +44}, then a ShadowSource named "Reflection" with
// two parameter sets read from the material parameter block (+12 u16
// count, +16 sorted u32 hashes, +20 u16 offsets, +24 data).
namespace refl {
constexpr u32 kCount = 0x01194FA0, kRecords = 0x01194ED8, kRecordSize = 200;
constexpr u32 kConsts = 0x005A9D78;   // f32 1; +32 30, 10, 100, 50, 1/255
constexpr u32 kConfig = 0x010C5A20;   // +4 / +6 u16 width / height
constexpr u32 kDepthHolder = 0x00F66FBC;  // u32 -> +72 the depth target
constexpr u32 kMemStats = 0x010F0E20;     // Stats_ResolveTexture's argument (name?)
constexpr u32 kWhite = 0x01164CA0;        // u8[4] (Constants kColours[0])
constexpr u32 kName = 0x00AF5188;         // "Reflection"
constexpr u32 kSourceCallback = 0x00F3B438;  // OPD stored at the source's +660
constexpr u32 kLevels = 3, kLevelSurface = 44, kLevelSurfaceSize = 52, kLevelTexture = 28;
// The parameter hashes (two sets of: u8, f32, f32, colour; then two f32).
constexpr u32 kHashF16 = 0xBCDD903E, kHashU8A = 0x2781B8AF, kHashF0A = 0xCE12C35C, kHashF1A = 0x37BFDA6F;
constexpr u32 kHashColA = 0xBBDF536B, kHashU8B = 0x2781B8B0, kHashF0B = 0xCE12C35D, kHashF1B = 0x37BFDA70;
constexpr u32 kHashColB = 0xBBDF536C, kHashF20 = 0x3D63A767;
}  // namespace refl

}  // namespace gfx22
}  // namespace sly4::render
