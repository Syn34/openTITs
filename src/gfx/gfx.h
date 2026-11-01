// Graphics start-up (Main_InitSystems stage 4): libgcm, the Agl pools and
// the IFF chunk types for meshes (SMSH, MVTX, MIDX, VTXD, MPAL, VINF, MHDR).
#pragma once

#include "../sly4.h"

namespace sly4::gfx {

namespace fn {
constexpr u32 Graphics_Init = 0x0050F520;  // decompiled: src/gfx/graphics.cpp
// Still recompiled.
constexpr u32 Gcm_Init = 0x0053AC38;      // () -> bool; 583 instructions: libgcm, video out, job lists (name?)
constexpr u32 Agl_Init542704 = 0x00542704;   // () pool sized by config +350 (via 0x0051DF3C) (unknown)
constexpr u32 Shaders_Init = 0x0050F308;     // () -> bool; "System\Shaders\", pool from config +348 (name?)
constexpr u32 Agl_Init548BC0 = 0x00548BC0;   // () registers chunk 'MEDG' (unknown)
constexpr u32 Agl_Init50EF04 = 0x0050EF04;   // () pool from config +356, default 1024 (unknown)
constexpr u32 Agl_Init50ECD4 = 0x0050ECD4;   // () (unknown)
constexpr u32 Agl_Init50E614 = 0x0050E614;   // () pool from config +358, default 1024 (unknown)
constexpr u32 Agl_Init50E410 = 0x0050E410;   // () uses config +324 (unknown)
constexpr u32 Agl_Init54BF24 = 0x0054BF24;   // () -> 0x00527484 (unknown)
constexpr u32 Agl_Init50D2E0 = 0x0050D2E0;   // () registers chunk 'OUTL' (unknown)
constexpr u32 Agl_Init548974 = 0x00548974;   // () cellGcmGetReportDataAddressLocation (unknown)
}  // namespace fn

// g::config fields used here.
constexpr u32 kConfigAspect = 16;         // f32 display aspect ratio, set here
constexpr u32 kConfigWidescreen = 33;     // u8: 16:9 instead of 4:3
constexpr u32 kConfigUnk44 = 44;          // u32, 0 -> 20
constexpr u32 kConfigGraphicsUp = 82;     // u8 set to 1 at the end
constexpr u32 kConfigPoolA = 354;         // s16 pool sizes: 0 -> default, negative -> 0
constexpr u32 kConfigPoolB = 352;
constexpr u32 kConfigPoolC = 398;

}  // namespace sly4::gfx
