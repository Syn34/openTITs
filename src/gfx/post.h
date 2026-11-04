// Post-processing shader programs: Cg vertex / fragment program objects,
// the hash -> program maps they are cached in, and the start-up loaders
// (PostProc_Init, PostProc_InitCopy, StencilShadow_Init).
//
// VProgram (24 bytes): +0 ucode, +4 CgProgram*, +8 u16 ucode size,
//   +10 u16 attribute mask, +12 flags, +16 ucode copy (pool 5), +20 offset.
// FProgram (1168 bytes): +0 ucode copy (pool 5), +4 CgProgram*, +8 u16
//   ucode size, +12 offset << 1 | 1, +16 u64 dirty constant mask,
//   +24 u32 stamp, +28 constants used, +48.. 34 vec4 constants,
//   +592.. 34 x 16-byte parameter records {CgParameter, resource,
//   binding entry, constant index}, +1136 offset, +1156 u8 register count.
// ProgramMap: +0 u16 capacity, +2 u16 count, +4 u16 high water, +6 u16
//   bit 0 unsorted, +8 {u32 key, u32 value}*, +16 "not found" value,
//   +20 comparator OPD.
#pragma once

#include "../sly4.h"

namespace sly4::post {

namespace imp {
constexpr u32 cellGcmAddressToOffset = 0x00ABD5E4;
}  // namespace imp

namespace fn {
constexpr u32 PostProc_Init = 0x00534310;        // decompiled: src/gfx/post_init.cpp
constexpr u32 PostProc_InitCopy = 0x00546F6C;    // decompiled: src/gfx/post_init.cpp
constexpr u32 StencilShadow_Init = 0x0054BF48;   // decompiled: src/gfx/post_init.cpp
constexpr u32 PostProc_InitEffects = 0x0051FF1C; // decompiled: src/gfx/post_init.cpp
constexpr u32 Blur11V_SetOffsets = 0x0053CE00;   // decompiled: src/gfx/post_blur.cpp
constexpr u32 Blur11H_SetOffsets = 0x0053CFD0;   // decompiled: src/gfx/post_blur.cpp
constexpr u32 VProgram_Create = 0x00533EE4;      // decompiled: src/gfx/post_programs.cpp
constexpr u32 FProgram_Create = 0x00533BC0;      // decompiled: src/gfx/post_programs.cpp
constexpr u32 FProgram_Setup = 0x00533978;       // decompiled: src/gfx/post_programs.cpp
constexpr u32 FMap_Insert = 0x007A2BF4;          // decompiled: src/gfx/post_map.cpp
constexpr u32 FMap_Find = 0x007A2D40;            // decompiled: src/gfx/post_map.cpp
constexpr u32 VMap_Insert = 0x007A2EC8;          // decompiled: src/gfx/post_map.cpp
constexpr u32 VMap_Find = 0x007A3014;            // decompiled: src/gfx/post_map.cpp
// Still recompiled: game code.
constexpr u32 ProgramPair_Load = 0x00534160;     // (VProgram**, FProgram**, name, binding table, 0) (name?)
constexpr u32 FProgram_Finish = 0x0053378C;      // (FProgram**) (name?)
constexpr u32 String_HashNoCase = 0x0056A710;
constexpr u32 String_Printf = 0x00551540;        // sprintf
constexpr u32 File_LoadWhole = 0x00560B90;       // (path, u32* size) -> buffer
constexpr u32 Array_Reserve = 0x00558F3C;        // (Array*, u16 capacity)
constexpr u32 qsort = 0x0071EC28;
constexpr u32 memmove = 0x0071C3F0;
constexpr u32 memset = 0x00726E54;
constexpr u32 Mem_AllocPool = 0x0056C514;        // (align, size, pool, flags)
// Still recompiled: libgcm Cg / command-buffer helpers.
constexpr u32 Cg_InitProgram = 0x005DFAF4;       // (CgProgram*) (name?)
constexpr u32 Cg_Sub5E5F28 = 0x005E5F28;         // (CgProgram*) (unknown)
constexpr u32 Cg_GetUCode = 0x005E5E28;          // (CgProgram*, void** ucode, u32* size) (name?)
constexpr u32 Cg_GetNamedParameter = 0x005E5E54; // (CgProgram*, const char*) -> CgParameter (name?)
constexpr u32 Cg_GetParameterResource = 0x005E5F20;  // (CgProgram*, CgParameter) -> s32, -1 none (name?)
constexpr u32 Cg_IsParameterReferenced = 0x005E60DC; // (CgProgram*, CgParameter) -> bool (name?)
constexpr u32 Cg_GetRegisterCount = 0x005E5F38;  // (CgProgram*) -> u32 (name?)
constexpr u32 Cg_SetRegisterCount = 0x005E5F9C;  // (CgProgram*, u32) (name?)
constexpr u32 Cg_GetAttributeMask = 0x005E5FF0;  // (CgProgram*) -> u16 (name?)
constexpr u32 Gcm_SetupContext = 0x005DF960;     // (CellGcmContextData*, begin, size, callback OPD) (name?)
constexpr u32 Gcm_SetVertexProgram = 0x005E5914; // (context, CgProgram*, ucode) (name?)
constexpr u32 Gcm_FinishContext = 0x005E3C1C;    // (context) (name?)
constexpr u32 Gcm_SetFragmentProgramLoad = 0x005DF980;  // (CgProgram*, out, 0, 1, 0) (name?)
// Called by PostProc_InitEffects.
constexpr u32 PostProc_Init546334 = 0x00546334;
constexpr u32 PostProc_Init53D800 = 0x0053D800;
constexpr u32 PostProc_Init540824 = 0x00540824;
constexpr u32 PostProc_Init53EC4C = 0x0053EC4C;
constexpr u32 PostProc_Init53ECE0 = 0x0053ECE0;
constexpr u32 PostProc_Init54588C = 0x0054588C;
constexpr u32 PostProc_Init543774 = 0x00543774;
constexpr u32 PostProc_Init5447F4 = 0x005447F4;
constexpr u32 PostProc_Init5420CC = 0x005420CC;
}  // namespace fn

namespace g {
constexpr u32 vmap = 0x010F1350;  // ProgramMap of VProgram* by name hash
constexpr u32 fmap = 0x010F1368;  // ProgramMap of FProgram* by name hash
constexpr u32 vmap_compare = 0x00F3BF88;  // default comparator OPDs (plain u32 key order)
constexpr u32 fmap_compare = 0x00F3BF90;
constexpr u32 default_bindings = 0x00F6748C;  // {name, constant index} list, ends at name 0 ("gvLightData[0]", ...)
constexpr u32 gcm_state = 0x010F10A0;  // +28 current FProgram*, +92 dirty bits, +156 frame stamp (name?)
constexpr u32 fmt_fpo = 0x00AF3314;    // "System\Shaders\%s.fpo"
constexpr u32 fmt_vpo = 0x00AF332C;    // "System\Shaders\%s_V.vpo"
}  // namespace g

}  // namespace sly4::post
