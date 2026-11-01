// Resource system start-up (Main_InitSystems stage 2): pools sized from
// g::config and the IFF chunk types for shaders (PVS*), occlusion (OC**)
// and localisation/lighting (LOCA, LCRH, LCRD).
#pragma once

#include "../sly4.h"

namespace sly4::resource {

namespace fn {
constexpr u32 Resources_Init = 0x00588828;  // decompiled: src/resource/resources.cpp
// Still recompiled.
constexpr u32 PtrVector_Construct = 0x005516E0;  // (PtrVector*) zeroes it (name?)
constexpr u32 PtrVector_Init = 0x00551598;       // (PtrVector*, u32 capacity, 0, 0) (name?)
// Pool set-ups, each sized by a u16 at g::config + <decimal suffix>; no arguments.
constexpr u32 ResPool_Init378 = 0x00588660;  // config +378, default 128
constexpr u32 ResPool_Init374 = 0x00588478;  // config +374, default 1024
constexpr u32 ResPool_Init376 = 0x00588240; // config +376, default 320
constexpr u32 ResPool_Init384 = 0x00588078;  // config +384, default 32
constexpr u32 ResPool_Init386 = 0x00587E90;  // config +386, default 64
constexpr u32 ResPool_Init370 = 0x00587CC8;  // config +370, default 32
constexpr u32 ResPool_Init360 = 0x00587A40;  // config +360, default 1024
constexpr u32 ResPool_Init364 = 0x00587750;  // config +364, default 128
constexpr u32 ResPool_Init362 = 0x00587574;  // config +362, default 512
constexpr u32 ResPool_Init380 = 0x005873AC;  // config +380, default 64
constexpr u32 ResPool_Init366 = 0x005871E4;  // config +366, default 64
constexpr u32 ResPool_Init372 = 0x00586FEC;  // config +372, default 64
constexpr u32 Res_Check586DB4 = 0x00586DB4;  // () -> bool; gates Res_Init586BF4 (unknown)
constexpr u32 Res_Init586BF4 = 0x00586BF4;   // () (unknown)
constexpr u32 Res_Init586B3C = 0x00586B3C;   // () builds something from 0x0050F0F0 / 0x0050956C (unknown)
constexpr u32 ResPool_Init504 = 0x005868D0;  // config +504 (u32), default 256
}  // namespace fn

}  // namespace sly4::resource
