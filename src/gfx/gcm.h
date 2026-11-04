// libgcm / RSX start-up (Gcm_Init, called first by Graphics_Init).
#pragma once

#include "../sly4.h"

namespace sly4::gcm {

namespace imp {
constexpr u32 cellGcmSetVBlankHandler = 0x00ABD7E4;
constexpr u32 cellGcmInitDefaultFifoMode = 0x00ABD844;
constexpr u32 cellGcmGetConfiguration = 0x00ABD8A4;
constexpr u32 cellGcmGetLabelAddress = 0x00ABD8E4;
constexpr u32 cellGcmSetVBlankFrequency = 0x00ABD904;
constexpr u32 cellSpursCreateJobChainWithAttribute = 0x00ABDFA4;
constexpr u32 _cellSpursJobChainAttributeInitialize = 0x00ABDFC4;  // cellSpurs_3548F483
constexpr u32 cellSpursJobGuardInitialize = 0x00ABE084;
constexpr u32 cellSpursJobChainAttributeSetName = 0x00ABE124;
constexpr u32 cellSpursJobChainAttributeSetHaltOnError = 0x00ABE1C4;
constexpr u32 cellSpursRunJobChain = 0x00ABE264;
constexpr u32 cellVideoOutGetScreenSize = 0x00ABE3C4;
constexpr u32 sys_ppu_thread_get_id = 0x00ABF1E4;
}  // namespace imp

namespace fn {
constexpr u32 Gcm_Init = 0x0053AC38;  // decompiled: src/gfx/gcm.cpp
// Still recompiled.
constexpr u32 Sysmodule_Require = 0x0056C020;   // (id) -> 1 if loaded (name?)
constexpr u32 Gcm_InitBody = 0x005E0530;        // (cmd size, io size, io address) -> 0 or error (libgcm, name?)
constexpr u32 RsxLocal_Init = 0x0056C428;       // (local address, local size) (name?)
constexpr u32 RsxMain_Init = 0x0056C49C;        // (address, size) (name?)
constexpr u32 Gfx_Init52FD2C = 0x0052FD2C;      // () (unknown)
constexpr u32 Display_InitSurfaces = 0x00531084; // () display render targets and colour surfaces
constexpr u32 Gfx_Init5312E4 = 0x005312E4;      // (u8 mode, 0, 1) (unknown)
constexpr u32 Gfx_InitFrameSurfaces = 0x005333CC; // () frame-buffer surfaces
constexpr u32 CommandBuffer_Construct = 0x0052F670;  // (obj 384 bytes, u32 size) (name?)
constexpr u32 RsxHeap_Init = 0x00537F28;        // (obj) RSX scratch heap
constexpr u32 Gfx_Init5E403C = 0x005E403C;      // (u32, 1, 1) (libgcm?, unknown)
constexpr u32 Surface_GetSize = 0x00530318;     // (w, h, 3, 0, 0, info*) -> bytes (name?)
constexpr u32 Heap_InitRegion = 0x0056DF80;     // (heap, base, size) (name?)
constexpr u32 JobQueue_Init = 0x0060E0B4;       // (queue, 5, spurs, prio[8], 251, buffer, 22528) (name?)
constexpr u32 String_Format = 0x005534D0;
constexpr u32 Mem_AllocPool = 0x0056C514;
constexpr u32 Mem_NewAligned = 0x00731648;
constexpr u32 fprintf = 0x00723FEC;
constexpr u32 exit = 0x0071B570;
}  // namespace fn

}  // namespace sly4::gcm
