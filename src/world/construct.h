// World_Construct (0x0045CC18, 6393 instructions, still recompiled) and the
// out-of-line constructors it calls (src/world/construct_*.cpp). Guest
// addresses in the BCES01284 v01.02 EBOOT; "?" marks a guess.
//
// World_Construct(World* w, u8 initial) in order:
//   1. StateMachine_Construct(w, 4 levels, 128 bytes, initial); vtable 0x00B21A00.
//   2. Plain fields (see the member map below) and the member constructors.
//   3. The render views: main (+0x128168), 4 split views (+0x128184, "SplitRender%d"),
//      HUD_3D (+0x12816C, also g 0x00F66850), PostHUD_3D (+0x128170), PostHUD_2D (+0x128174).
//   4. sub_004B9F2C() (1313 instructions), AnimScript_RegisterIffChunks().
//   5. The anim-script registries: 8 variable types (SmallAlloc, AVL at 0x010C27B8)
//      and 269 commands (Mem_Alloc, AVL at 0x010C2820), keyed by String_HashNoCase
//      of the type name; constructors in construct_animscript*.cpp.
//   6. sub_004EC730(); two 32 KiB buffers (g 0x010B7834 / 0x010B7838); the
//      "LevelData.Level.*File" settings (sub_004EC20C x8); sub_00426DD4();
//      sub_002E9FE0 x3; memset(0x010BAB38, 0, 3072); sub_000CE0A8();
//      9 Iff chunk handlers ('GOFA'.. 'LVDL'); sub_002E8D74().
//   7. Global singletons, each Mem_Alloc'd and constructed in place:
//      156 bytes sub_0043D9A4(mem, 0); sub_00346BB0(); sub_003489F4();
//      5088 sub_000186CC (g 0x00F557F0); 88 sub_004518D0 (g 0x00F65BE8);
//      60 sub_002EE304 (g 0x00F64C38); 116 sub_003A0010 (g 0x00F657FC);
//      104 sub_002DD4FC (Session, g 0x00F64C20).
//   8. +0x1280F0 9 words = 0; FrameRing (24 bytes) at +0x1285E4.
//
// World member map (offset: what, constructor):
//   +0      StateMachine (vtable 0x00B21A00), 4 levels x 128 bytes
//   +28     u8 flags: &= 0x0F                  +29 u8 = 1 (users)    +30 u8 = 255
//   +31..34 u8 = 0   +36 Object* = 0   +40 = 0
//   +44     u8 flags = (old & 0x02) | 0x01 (0x80 = write save.dat on exit)
//   +45     u8 flags = 0x29                    +46 u8 |= 0xC0
//   +47/+48 u8 = initial state                 +49 = 0  +50 = 15 (phase)  +51..53 = 255
//   +56/+64 f32 = 0.0                          +60 f32 = tuning 0x010B7DEC  +68 u8 = 1  +72 = 0
//   +80     SaveData (4064 bytes)              SaveData_Construct 0x007865FC
//   +4180..4184 u8 = 0, +4188/+4192/+4200 = 0, +4196 u8 = 0
//   +4208   UserBlock[9], 0x20C80 bytes each   UserBlock_Construct 0x0045C868
//   +0x128114.. +0x128160 = 0
//   +0x128168 View* main, +0x12816C HUD_3D, +0x128170 PostHUD_3D, +0x128174 PostHUD_2D,
//            +0x128184 View*[4] split, +0x1281E4 [4] = 0, other words to +0x128204 = 0
//   +0x1281B0 Vec3 = 0x010B9760 (Vec3_Copy)    +0x128208 f32 = 0.0
//   +0x12820C PtrVector (u32 elements)         PtrVector_ConstructInit 0x00786658 (4, 0, 1)
//   +0x128220..22 u8 = 0, +0x128224 cutscene = 0
//   +0x128230 camera 4x3 = 0x01164BD0           Mat43_Copy 0x00734CC8
//   +0x128270 f32 = 1.0, +0x128280 f32 = 0, +0x128288 f32 = 23999 (filter cutoff), +0x12828C/D u8 = 1
//   +0x128290 Fade                              Fade_Construct 0x007866CC
//   +0x1282A4 time scale blend                  ScaleBlend_Construct 0x00786724
//   +0x1282C0 audio scale blend                 ScaleBlend_Construct 0x00786724
//   +0x1282DC depth of field                    Dof_Construct 0x00786758
//   +0x128310 u8 = 255, +0x128324 16 bytes      Clear16_Construct 0x007867B0
//   +0x1283A4 = 0, +0x1283A8 filter             Filter_Construct 0x007867C8
//   +0x1283C0 188-byte object                   Obj1283C0_Construct 0x007867FC
//   +0x12847C = 0 (176-byte object, created later)
//   +0x128480 PtrVector[13], 12-byte callback entries (PtrVector_Construct, +6 = 12)
//   +0x128584..+0x1285C9 scalars (+0x128588 slow-mo pair {0, -1.0, 0}, +0x1285C0 = -1)
//   +0x1285CC 24-byte object                    Pair24_Construct 0x007868A4 (0)
//   +0x1285E4 FrameRing (Mem_Alloc 24)          FrameRing_Construct 0x0078691C (0x20000, 0xC000)
//   +0x1285E8..+0x128610 = 0, +0x1286A4 = 0
#pragma once

#include "vt_world.h"

namespace sly4 {
namespace wcons {

namespace kfn {
constexpr u32 Mem_Alloc = 0x0054ED38;             // (size, pool, align)
constexpr u32 SmallAlloc = 0x0055238C;            // (size)
constexpr u32 Memset = 0x00726E54;
constexpr u32 PtrVector_Construct = 0x005516E0;   // (vec)
constexpr u32 PtrVector_Init = 0x00551598;        // (vec, capacity?, ?, u8 ?)
constexpr u32 FreeList_Init = 0x00551A58;         // (list, node size, nodes per block, 0, 0)
constexpr u32 Iff_RegisterChunk = 0x0055902C;     // (u32 id, OPD handler, 0, 0, 0)
constexpr u32 AnimCmd_ConstructBase = 0x004B9CB8;  // decompiled in construct_animscript.cpp
constexpr u32 AvlCmd_InsertAt = 0x00785744;       // (tree, key, value, node) recursive (name?)
constexpr u32 AvlVar_InsertAt = 0x00785FE8;       // "
constexpr u32 SaveData_Reset = 0x000ADE04;        // decompiled in construct_members.cpp
constexpr u32 Byte_Clear = 0x000ADDE8;            // "
constexpr u32 Bytes2_Clear = 0x000ADDF4;          // "
constexpr u32 AnimCmd_Clear12_24 = 0x0007AF18;    // "
constexpr u32 RenderBuffer_Construct = 0x0050EB08;  // (mem, size, size2, 1, 1), 224 bytes (name?)
}  // namespace kfn

// Anim-script registries: AVL trees {root, u32 count} keyed by name hash.
constexpr u32 kAnimVarTree = 0x010C27B8;   // AVL<u32, ApeAnimScriptVar*>
constexpr u32 kAnimCmdTree = 0x010C2820;   // AVL<u32, ApeAnimScriptCommand*>
// Their node allocators (ApeTFreeList, created on first insert).
constexpr u32 kAnimCmdNodes = 0x00F8F0E4, kAnimVarNodes = 0x00F8F0D8;
constexpr u32 kFreeListVtableCmd = 0x00B10E90, kFreeListVtableVar = 0x00B0EFA8;
constexpr u32 kFreeListNameCmd = 0x00AE553A, kFreeListNameVar = 0x00AE53EC;  // GetAllocator() signatures

// ApeAnimScriptCommand: +0 vtable, +4 u16 = 1.
constexpr u32 kAnimCmdVtable = 0x00B21C98;

// AVL node (20 bytes): +0 u16 balance?, +2 u16 height, +4 key, +8 value, +12 left, +16 right.

// UserBlock (World +4208, 9 of them, 0x20C80 bytes).
constexpr u32 kBlockMatrix = 16;        // 4 rows of 3 f32 (stride 16) = 0x01164BD0
constexpr u32 kBlockVector = 80;        // PtrVector of u32
constexpr u32 kBlockRecords = 112;      // 2048 x 64 bytes (+4)
constexpr u32 kBlockRecords2 = 0x20474; // 32 x 64 bytes (+4)
constexpr u32 kIdentity43 = 0x01164BD0;
constexpr u32 kTuneBlockA = 0x010BC128, kTuneBlockB = 0x010BC178;  // f32 tuning (value at +72) (name?)

constexpr u32 kTuning = 72;

}  // namespace wcons
}  // namespace sly4
