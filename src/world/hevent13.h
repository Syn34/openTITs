// Shared names for src/world/hevent13_*.cpp: callees named by the
// hevent12_*.cpp functions and more of the World event handler's remaining
// callees. Guest addresses in the BCES01284 v01.02 EBOOT; "?" marks a guess.
#pragma once

#include "hevent12.h"

namespace sly4 {
namespace hevent13 {

namespace h13fn {
constexpr u32 WorldObj_SetGroup = 0x00583E54;  // (obj, group) (symbols.txt)
constexpr u32 XmlDoc_Construct = 0x004FA888;   // (XmlDoc*, const char* path) (symbols.txt)
constexpr u32 XmlDoc_Init4FA7A4 = 0x004FA7A4;  // (XmlDoc*): empty document (name?)
constexpr u32 XmlDoc_Parse = 0x004FAF10;       // (XmlDoc*, 0) -> bool (symbols.txt)
constexpr u32 XmlDoc_LoadXmlb = 0x004FACCC;    // (XmlDoc*, File*, 0) -> bool (symbols.txt)
constexpr u32 XmlDoc_Destruct = 0x004FA9C0;    // (XmlDoc*) (symbols.txt)
constexpr u32 Set_AddXml = 0x004BDA70;         // (Set*, path, XmlDoc*, u8) (symbols.txt)
}  // namespace h13fn

// XmlDoc on the stack: the original frames reserve r1+112 .. the register
// save area (2240 / 2248 bytes).
constexpr u32 kXmlDocFrame = 2256;

// Named points (as src/obj/obj5.h kMapHolderPoint): header +16 map, looked
// up with 0x0073343C (map, const u32* key) -> slot; value +16 the Mat.
constexpr u32 kMapHolderPoint = 0x00F8F154, kHolderMap = 16, kPointMatrix = 16;
constexpr u32 kMapFind73343C = 0x0073343C;
constexpr u32 kIdentity = 0x01164BD0;  // f32[4][4]
constexpr u32 kWorldSpawn486460 = 0x00486460;  // (World*, u32 index, Mat* out) (name?)
constexpr u32 kWUsers = 0x1280F0, kWNumUsers = 9;  // World +0x1280F0: Object*[9], one per user

// Copies the 3x4 part of a 4x4 matrix (w of each row skipped), lfs/stfs
// pairs; f1 is left with the last single loaded.
inline void copy_mat(PPUContext& ctx, u32 dst, u32 src) {
    for (u32 r = 0; r < 64; r += 16)
        for (u32 c = 0; c < 12; c += 4) store<u32>(dst + r + c, load<u32>(src + r + c));
    ctx.fpr[1] = hevent7::lfs_f64(src + 56);
}
// The identity copy (alternating f1 / f2 loads).
inline void copy_identity(PPUContext& ctx, u32 dst) {
    copy_mat(ctx, dst, kIdentity);
    ctx.fpr[1] = hevent7::lfs_f64(kIdentity + 52);
    ctx.fpr[2] = hevent7::lfs_f64(kIdentity + 56);
}
// The named point's slot value for hash (the key goes in the stack word keyslot).
inline u32 find_point(PPUContext& ctx, u32 keyslot, u32 hash) {
    store<u32>(keyslot, hash);
    const u32 slot = static_cast<u32>(call(ctx, kMapFind73343C, {load<u32>(load<u32>(kMapHolderPoint) + kHolderMap), keyslot}));
    return load<u32>(slot);
}

}  // namespace hevent13
}  // namespace sly4
