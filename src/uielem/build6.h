// The type-6 element (vtable 0x00B22C40): a box drawn from up to nine
// texture slices (a 1-, 3- or 9-slice layout), rebuilt by its update
// (vfunc +152) whenever +248 is set.
#pragma once

#include "../sly4.h"

namespace sly4 {
namespace build6 {

using namespace opentit;

namespace b6fn {
constexpr u32 SmallAlloc = 0x0055238C;        // (size) -> block or 0
constexpr u32 memset = 0x00726E54;
constexpr u32 String_Format = 0x005534D0;     // (fmt, ...) -> rotating static buffer
constexpr u32 Tex_GetDefault = 0x004FEF30;    // (n) -> Texture*
constexpr u32 UiElem_SetTexture = 0x004D8A80; // (e, UiTexture*, const f32 uv0[2], const f32 uv1[2])
constexpr u32 UiElem_InitDefaults = 0x004D8D48;
}  // namespace b6fn

// Element fields used here.
constexpr u32 kElemFlags = 136;       // u32: 0x80000000 = update the children too
constexpr u32 kElemParent = 148;
constexpr u32 kElemFirstChild = 152, kElemNextSibling = 156;
constexpr u32 kElemSlices = 168;      // UiTexture*[9], row-major from the top left
constexpr u32 kElemSliceElems = 204;  // Element*[9]: the child drawing each slice
constexpr u32 kElemRebuild = 248;     // u8
constexpr u32 kSliceCount = 9;

// Element vfuncs.
constexpr u32 kVfnDelete = 4, kVfnSetPosition = 28, kVfnSetScale = 40, kVfnSetColor = 56, kVfnSetBlend = 64;
constexpr u32 kVfnSetActive = 88, kVfnGetSize = 140, kVfnUpdate = 152, kVfnGetScale = 168, kVfnGetWorldColor = 172;
constexpr u32 kTexVfnSetWrap = 16;    // texture: (u32 wrap_s, u32 wrap_t, 1) (name?)
constexpr u32 kWrapRepeat = 1, kWrapClamp = 3;  // (name?)

// UiTexture (src/font/tex.h).
constexpr u32 kResLoaded = 82;        // u16: 2 = loaded
constexpr u32 kTexHandle = 96;
constexpr u32 kTextureWidth = 52, kTextureHeight = 54;  // u16 in the loaded texture

// The slice child (208 bytes, vtable 0x00B229E8).
constexpr u32 kSliceVtbl = 0x00B229E8;
constexpr u32 kSliceElemSize = 208;
constexpr u32 kVtblBase = 0x00B22710;

// Globals.
constexpr u32 kIdentity = 0x01164BD0;  // f32[4][4]
constexpr u32 kOneVec3 = 0x01164B30;
constexpr u32 kWhite = 0x01164CA0;     // u8[4]
constexpr u32 kZeroVec2 = 0x01164AFC, kOneVec2 = 0x01164AF4;
constexpr u32 kMinusOne = 0xBF800000;  // f32 -1
constexpr u32 kF32One = 0x3F800000;
constexpr u32 kConsts = 0x004D8E74;    // f32: 1, 2, 0.5, 0
constexpr u32 kStrSliceName = 0x00AEF620;  // "%02d"
constexpr u32 kSliceMirrors = 0x00AEF5B4;  // s32[9][3]: slices to mirror (x, y, both) when one is missing

}  // namespace build6
}  // namespace sly4
