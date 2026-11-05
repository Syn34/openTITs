// The renderer range's next recompiled functions (gfx19*.cpp): the cube
// and indexed sphere meshes, the shared vertex declarations, the mesh /
// material / emitter / DDS chunk handlers, the depth pass, the draw-item
// setups and load-params of the SHO shader types, and a few static
// initialisers / destructors.
#pragma once

#include "gfx18.h"

namespace sly4::render {
namespace gfx19 {

namespace fn {
// Still recompiled or decompiled elsewhere (called from gfx19*.cpp).
constexpr u32 Data_Hash31 = 0x0056A7AC;            // (const u8* p, u32 n) -> h * 31 + byte (obj/obj14_small.cpp)
constexpr u32 ConvertState_0054A930 = 0x0054A930;  // (VtxDecl*, Element* dst, const u8 src[12], bool) -> written (gfx8.cpp) (name?)
constexpr u32 Array_Destruct = 0x0054EC94;  // (Array*): empty (gfx7_stubs.cpp)
constexpr u32 Surface_Destruct_007881DC = 0x007881DC;  // (Surface*, u32 2): releases its RSX part / pool block (name?)
constexpr u32 Draw_InvalidateState = 0x0051279C;               // () (render_state.cpp)
constexpr u32 DepthState_Apply_005495D8 = 0x005495D8;          // (o) (gfx14.cpp)
constexpr u32 Draw_ColourOnBoundsOff_00549860 = 0x00549860;    // () (gfx11.cpp)
constexpr u32 DrawList_DrawDepth_005499D8 = 0x005499D8;        // (?, View*) (gfx19.cpp)
constexpr u32 FindEntry_00537B7C = 0x00537B7C;        // (Element* table, u32 key, u32 kind) -> Element* or 0 (gfx7_misc.cpp)
constexpr u32 Floats_ByteSwap_00525174 = 0x00525174;  // (count, p, comps, stride) -> end (gfx10_math.cpp)
constexpr u32 VtxDecl_Get_005246A8 = 0x005246A8;    // (const u8 (*)[12], u32 count, bool) -> VtxDecl* (gfx19.cpp)
constexpr u32 Streams_Bind_00538950 = 0x00538950;  // (Mesh*, const MeshHeader*) (name?)
constexpr u32 Shader_LoadGroup = 0x0050F0F0;       // (u32) -> Material* (shadergroup.cpp)
constexpr u32 Material_Apply_0050C3D8 = 0x0050C3D8;  // (Material*, const MaterialRecord*) the material chunk's settings (name?)
constexpr u32 PtrVector_Construct = 0x005516E0;  // (PtrVector*): all fields 0 (obj/obj13_misc.cpp)
constexpr u32 Texture_InitLayout = 0x00536E3C;  // (layout* t+56, u16 w, u16 h, 1, format, 0, 0, 0) (font/image2.cpp)
constexpr u32 Rsx_Copy = 0x0056E2EC;            // (const void* src, void* dst, u32 size, u32 flags) (obj/obj15_job.cpp)
}  // namespace fn

namespace g {
// ApeTListAllocator<AglVtxDecl>: {+0 ApeTFreeList*, +4 heap} (layers3.h's
// block_e061c); its 144-byte entries are the vertex declarations
// VtxDecl_Get_005246A8 shares (name?).
constexpr u32 vtxdecl_list = 0x010E061C;
constexpr u32 vt_vtxdecl_list = 0x00B23E40;   // ApeTFreeList<AglVtxDecl>
constexpr u32 str_vtxdecl_list = 0x00AF1370;  // "ApeTFreeList<T> &ApeTListAllocator<T>::GetAllocator() [with T = AglVtxDecl]"
}  // namespace g

// Mesh_CreateCube_0051C1A8: f32 {0.5, -0.5} (.text 0x0051C19C), the mesh's
// name and the index tables (u8; 0x00F67145: 24 for the 12 edges as
// lines, then 36 for the 12 faces as triangles).
constexpr u32 kCubeConsts = 0x0051C19C;
constexpr u32 kStrCube = 0x00AF1A68;  // "Cube"
constexpr u32 kCubeIndices = 0x00F67145;
constexpr u32 kCubeLineIndices = 24, kCubeFaceIndices = 36, kCubeVertices = 8;
// Mesh_CreateSphereIdx_0051BC20: f32 {pi, 2 pi, 0.5} (.text 0x0051BC14)
// and the name; strips are joined with a primitive restart index.
constexpr u32 kSphereIdxConsts = 0x0051BC14;
constexpr u32 kStrSphereIdx = 0x00AF1A5C;  // "SphereIdx"
constexpr u32 kRestartIndex = 0xFFFF;
// AglVtxDecl (144 bytes): +0 Element* source (12-byte elements: u8 +4
// stream, +5 its stride, +9 ...), +4 u32 hash, +16 the converted elements
// (11 bytes copied each, up to the +4 == 0 terminator), +136 u16[4] the
// stream strides, +140 u16 flags (0x8000 set), +142 u16 attribute mask.
constexpr u32 kDeclElements = 16, kDeclStrides = 136, kDeclFlags = 140, kDeclMask = 142;
constexpr u32 kDeclElementSize = 12, kDeclSize = 144;
constexpr u32 kDeclTerminator = 0x00F67184;  // the 11-byte element ending the converted list
// RenderState_Destruct_0054E30C: the static surfaces it tears down in
// order, each {u64 guard; Surface at +8} (guards 64 bytes apart; display.h
// names some), and the target arrays between them (gfx12.cpp's
// TargetArray_Destruct_*: guard, function).
struct StaticTeardown {
    u32 guard, fn;  // fn 0: Surface_Destruct_007881DC(guard + 8, 2)
};
constexpr StaticTeardown kRenderStatics[20] = {
    {0x011643C8, 0}, {0x01164408, 0}, {0x01164448, 0}, {0x01164488, 0}, {0x011644C8, 0},
    {0x01164508, 0}, {0x01164548, 0}, {0x01164588, 0}, {0x011645C8, 0x0054E120}, {0x011645D0, 0},
    {0x01164610, 0}, {0x01164650, 0x0054DF2C}, {0x01164658, 0x0054DD38}, {0x01164660, 0}, {0x011646A0, 0},
    {0x011646E0, 0}, {0x01164720, 0x0054DB44}, {0x01164728, 0}, {0x01164768, 0}, {0, 0},
};
constexpr u32 kThermalShaderVtable = 0x00B23F00;  // effect_init.h's thermal_shader, reset to its base vtable
// Emitter_InitSystem_0050E614: the emitter chunk handlers ({tag, OPD};
// the tags read 'EMPC', 'EMPH', ... in file order), the sprite stream's
// default vertex format (12-byte elements up to +4 == 0) and the
// config counts it sizes from.
constexpr gfx18::ChunkReg kEmitterChunks[12] = {
    {0x43504D45, 0x00F3B658}, {0x48504D45, 0x00F3B660}, {0x42495045, 0x00F3B668}, {0x4E4D5045, 0x00F3B670},
    {0x4D4E5045, 0x00F3B678}, {0x43545045, 0x00F3B680}, {0x54435045, 0x00F3B688}, {0x50535045, 0x00F3B690},
    {0x53545045, 0x00F3B698}, {0x52545045, 0x00F3B6A0}, {0x56555045, 0x00F3B6A8}, {0x50435045, 0x00F3B6B0},
};
constexpr u32 kSpriteFormat = 0x00F66EEC;
constexpr u32 kCfgEmitPropsCount = 358;  // g::config s16: AglEmitProperties capacity (0: 1024, negative: 0) (name?)
constexpr u32 kCfgSpriteCount = 344;     // g::config s32: sprites per stream buffer (0: 20480, negative: 0) (name?)
constexpr u32 kDefaultEmitProps = 1024, kDefaultSprites = 20480;
constexpr u32 kStreamBuffersSize = 640, kStreamBuffersAlign = 128;
// DepthPass_Run_00549F30: f32 {0.5, -0.5, 1, 0} (.text 0x00549F20): the
// viewport's scale / offset halves and its depth range (max, min).
constexpr u32 kDepthPassConsts = 0x00549F20;
constexpr u32 kSysTimerUsleep = 141, kDepthPassWaitUs = 300;
constexpr u32 kGcmJump = 0x20000000;  // CELL_GCM_METHOD_FLAG_JUMP
// Mesh_SwapVertices_0052528C: the attribute keys it looks up (s32[10] at
// 0x00F67190) and the element types it byte-swaps: f32 x1..4 or s16 /
// u16 x1..4 (the u8 types 0x40404 / 0x40407 and any other are left).
constexpr u32 kSwapKeys = 0x00F67190, kSwapKeyCount = 10;
struct SwapType {
    u32 type, comps;
    bool f32s;
};
constexpr SwapType kSwapTypes[12] = {
    {0x100402, 4, true}, {0xC0302, 3, true}, {0x80202, 2, true}, {0x40102, 1, true},
    {0x80403, 4, false}, {0x80401, 4, false}, {0x60303, 3, false}, {0x60301, 3, false},
    {0x40203, 2, false}, {0x40201, 2, false}, {0x20103, 1, false}, {0x20101, 1, false},
};
// DrawItem_SetupBasic_005D7598's constant (.text 0x005D7594: 1 / 255,
// the item's u8 +71 alpha scale) and the GCM methods it emits.
constexpr u32 kInv255 = 0x005D7594;
constexpr u32 kMethodDepthMask = 0x00040A70, kMethodBlendEnable = 0x00040310;
constexpr u32 kMethodBlendFunc = 0x00080314, kBlendSrcAlpha = 0x03020302, kBlendOneMinusSrcAlpha = 0x03030303;
constexpr u32 kMethodBlendEquation = 0x00040320, kBlendFuncAdd = 0x80068006;
// LoadParams_005D0CA0's defaults (.text 0x005D0C80): f32 {1, 0, pi /
// 180, 1e-4, 0.3, 0.5, 30, 15}, and its default colours.
constexpr u32 kParamsD0CA0 = 0x005D0C80;
constexpr u32 kDefaultColourA = 0xFFFFFFFFu, kDefaultColourB = 0xFF661919u;
// LoadParams_005D4198's defaults (.text 0x005D4174): f32 {16, 4, 1,
// 1.33, 0, 0.1, 30, 15}; its textures' sampler wraps by desc value.
constexpr u32 kParamsD4198 = 0x005D4174;
constexpr u32 kDefaultTexKind = 5;
// Iff_HandleEmitShape_00528948: the chunk's field names, the shape
// vtables (sphere 40 bytes: radius +36; box 64 bytes: half size +48..;
// emit-on-surface variants by flags bit 0) and f32 {0, 0.5, pi / 180}
// (.text 0x00528938).
constexpr u32 kStrFlags = 0x00B23B42, kStrType = 0x00B23B48, kStrSpeed = 0x00B23B4D, kStrSpeedVar = 0x00B23B53,
              kStrAxisOfstX = 0x00B23B5C, kStrAxisSpreadX = 0x00B23B66, kStrAxisOfstZ = 0x00B23B72,
              kStrAxisSpreadZ = 0x00B23B7C, kStrSize = 0x00B23B88;
constexpr u32 kVtShapeSphereA = 0x00B23170, kVtShapeSphereB = 0x00B23190;  // flags bit 0 set / clear
constexpr u32 kVtShapeBoxA = 0x00B231B0, kVtShapeBoxB = 0x00B231D0;
constexpr u32 kShapeConsts = 0x00528938;
// Iff_HandleEmitRotation_00529FE0: the chunk's field names, the
// 192-byte rotation's vtable, f32 {0, 1, -1, 359.999, 360, pi / 180}
// (.text 0x00529FC4) and the three unit axes it recognises (+176 = 0, 1,
// 2; 3 any other).
constexpr u32 kStrVersionNumber = 0x00B23C42, kStrAxis = 0x00B23C50, kStrVariance = 0x00B23C55,
              kStrInitialAngleRange = 0x00B23C5E, kStrRandomDirEnable = 0x00B23C70, kStrRot = 0x00AF1C3C;
constexpr u32 kVtEmitRotation = 0x00B23240;
constexpr u32 kRotationConsts = 0x00529FC4;
constexpr u32 kUnitAxes[3] = {0x01164B50, 0x01164B60, 0x01164B70};
constexpr u32 kRotationKeys = 16;
// Iff_HandleMeshHeader_00524B68: the chunk's field names (version 4 by
// default) and the 10-byte vertex elements' (12 apart on the stack).
constexpr u32 kStrVer = 0x00B23945, kStrNumVerts = 0x00B23949, kStrNumIndices = 0x00B23952,
              kStrPrimitiveType = 0x00B2395D, kStrMaterialHash = 0x00B2396B, kStrVtxDefHash = 0x00B23978,
              kStrVtxScale = 0x00B23983, kStrVisGroup = 0x00B2398C, kStrVtxPackBufOfst = 0x00B23995,
              kStrVtxPackBufSize = 0x00B239A4, kStrBoundSphere = 0x00B239B3, kStrIdxPackBufOfst = 0x00B239BF,
              kStrIdxPackBufSize = 0x00B239CE, kStrStream1PackBufOfst = 0x00B239DD, kStrStream1PackBufSize = 0x00B239F0;
constexpr u32 kStrUsage = 0x00B23916, kStrUsageIndex = 0x00B2391C, kStrElemType = 0x00B23927, kStrElemSize = 0x00B2392C,
              kStrFract = 0x00B23931, kStrStride = 0x00B23937, kStrOffset = 0x00B2393E;
constexpr u32 kMeshElementBytes = 10;
// Iff_HandleMaterial_0051C704: the record's fields by chunk version
// (232 bytes: +0 diffuseHash, +4 envMapHash, +8 normalMapHash, +12
// specularMapHash, +16 decalMapHash, +20 nameHash, +24 char name[32],
// +56 u8 blendMode, +57 cullMode, +58 flags, +59 u32 diffuseColor, +63
// specColor, +68 f32 specPower, +72 u8 clampModeU, +73 clampModeV, +74
// sortLayer, +75 envMapType, +76 envMapBlendMode, +80 shaderTypeHash,
// +84 u8 param count, +88 u32 params[]).
enum class MatField : u8 { U32, Name, U8, Colour, F32 };
struct MatFieldRead {
    MatField kind;
    u8 offset;
    u32 name;
};
constexpr u32 kStrMatVersion = 0x00B237A9;
constexpr MatFieldRead kMatV4[19] = {
    {MatField::U32, 0, 0x00B237B7}, {MatField::U32, 4, 0x00B237C3}, {MatField::U32, 8, 0x00B237CE},
    {MatField::U32, 12, 0x00B237DC}, {MatField::U32, 16, 0x00B237EC}, {MatField::U32, 20, 0x00B237F9},
    {MatField::Name, 24, 0x00B23802}, {MatField::U8, 56, 0x00B23807}, {MatField::U8, 57, 0x00B23811},
    {MatField::U8, 58, 0x00B2381A}, {MatField::Colour, 59, 0x00B23820}, {MatField::Colour, 63, 0x00B2382D},
    {MatField::F32, 68, 0x00B23837}, {MatField::U8, 72, 0x00B23841}, {MatField::U8, 73, 0x00B2384C},
    {MatField::U8, 74, 0x00B23857}, {MatField::U8, 75, 0x00B23861}, {MatField::U8, 76, 0x00B2386C},
    {MatField::U32, 80, 0x00B2387C},
};
constexpr MatFieldRead kMatV2[17] = {
    {MatField::U32, 20, 0x00B237F9}, {MatField::Name, 24, 0x00B23802}, {MatField::U8, 56, 0x00B23807},
    {MatField::U8, 57, 0x00B23811}, {MatField::U8, 58, 0x00B2381A}, {MatField::Colour, 59, 0x00B23820},
    {MatField::Colour, 63, 0x00B2382D}, {MatField::F32, 68, 0x00B23837}, {MatField::U32, 0, 0x00B237B7},
    {MatField::U8, 72, 0x00B23841}, {MatField::U8, 73, 0x00B2384C}, {MatField::U32, 4, 0x00B237C3},
    {MatField::U8, 75, 0x00B23861}, {MatField::U8, 76, 0x00B2386C}, {MatField::U32, 8, 0x00B237CE},
    {MatField::U32, 12, 0x00B237DC}, {MatField::U32, 16, 0x00B237EC},
};
constexpr u32 kMatSortLayerOffset = 74;  // version 3 is version 4 without it
// Bank_StaticInit_005AD444's blocks (obj/obj4.h, obj6.h, obj15.h and
// gfx15.h name some).
constexpr u32 kFltMax = 0x7F7FFFFF, kNegFltMax = 0xFF7FFFFFu;
constexpr u32 kOne = 0x3F800000, kHalf = 0x3F000000;
constexpr u32 kBankIdentity = 0x01164BD0;   // f32 rows copied into the default object matrix
constexpr u32 kBankOrigin = 0x01164B30;     // f32[3] its position
constexpr u32 kObjDefaultMatrix = 0x01193010;  // obj6.h kObj160Default
constexpr u32 kChangedList = 0x0116BD08;       // obj4.h: {obj** data, u32 count, u32 capacity}
constexpr u32 kChangedListSize = 20000, kChangedListCapacity = 5000;
constexpr u32 kBankGlobals = 0x01193060;       // obj15.h kBankGlobals
// Iff_HandleDds_00539C7C: the DDS fourCCs it takes (read big-endian
// after the swap), their texture formats (non power-of-two, power of
// two), D3DFMT_V8U8 / CxV8U8 (signed bytes, doubled) and the flag it sets.
constexpr u32 kFourCcDxt1 = 0x31545844, kFourCcDxt2 = 0x32545844, kFourCcDxt3 = 0x33545844, kFourCcDxt4 = 0x34545844,
              kFourCcDxt5 = 0x35545844, kD3dV8U8 = 60, kD3dCxV8U8 = 117;
constexpr u32 kDdsHeaderSize = 124;
constexpr u32 kTexDxt1 = 8, kTexDxt1Pot = 11, kTexDxt3 = 9, kTexDxt3Pot = 12, kTexDxt5 = 10, kTexDxt5Pot = 13, kTexV8U8 = 17;
constexpr u32 kTextureLoaded = 0x011DE4BD;  // u8: set once a DDS has loaded (name?)
// LoadParams_005D5C80's defaults (.text 0x005D5C58, ten f32).
constexpr u32 kParamsD5C80 = 0x005D5C58;
// DrawItem_SetupStencil_*: the stencil ref / mask bytes (render.h's
// stencil_bytes: +0 the ref last sent, +1 the mask) and the constants'
// 1 / 255 (each function's own .text copy).
constexpr u32 kStencilRefLast = 0x010DDA60, kStencilMaskByte = 0x010DDA61;
constexpr u32 kInv255_D65BC = 0x005D65BC, kInv255_D80FC = 0x005D80FC;
constexpr u32 kGcmCw = 0x0900, kGcmCcw = 0x0901;
constexpr u32 kPrimLines = 2, kPrimTriangles = 5;  // CELL_GCM_PRIMITIVE_LINES / _TRIANGLES

}  // namespace gfx19
}  // namespace sly4::render
