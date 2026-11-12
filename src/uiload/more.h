// Small helpers of the UI loader: element-agnostic XML walking, a
// case-insensitive strncmp, the type-8 element constructor, element
// textures (materials) and blending, timeline definitions, and texture
// file lookup / loading.
#pragma once

#include <cstddef>

#include "opentit/native.h"
#include "../sly4.h"
#include "../xml/xml.h"
#include "../text/text.h"
#include "../font/res.h"

namespace sly4 {
namespace uimore {

using namespace opentit;

namespace mfn {
constexpr u32 XmlCursor_FirstChildRawAny = 0x004F899C;  // (cursor) -> first child element of any tag, or 0
constexpr u32 XmlCursor_FirstChildAny = 0x004F9140;
constexpr u32 XmlCursor_NextSiblingAny = 0x004F8D74;
constexpr u32 XmlCursor_FindAttrByName = 0x004F9628;
constexpr u32 XmlCursor_ReadBool = 0x004F986C;
constexpr u32 XmlCursor_DocPath = 0x004F8944;
constexpr u32 String_HashNoCase = 0x0056A710;
constexpr u32 String_CopyN = 0x00553638;     // (dst, src, n)
constexpr u32 String_Format = 0x005534D0;    // (fmt, ...) -> rotating static buffer
constexpr u32 Mem_Alloc = 0x0054ED38;
constexpr u32 Mem_Free = 0x005611B0;
constexpr u32 File_LoadWhole = 0x00560B90;   // (path, u32* size) -> buffer or 0
constexpr u32 Path_GetExtension = 0x005611DC;
constexpr u32 UiElem_Construct = 0x004D76D0;
constexpr u32 Tex_GetDefault = 0x004FEF30;   // (n) -> Texture*
constexpr u32 Tex_Acquire = 0x00523B7C;
constexpr u32 Shader_FindByHash = 0x004FE990;
constexpr u32 Material_Alloc = 0x004FF2E8;
constexpr u32 TimelineDef_KeyFromXml = 0x004DF708;  // (doc, node, TimelineDef*) -> key or 0 (name?)
// World-object side of the type-8 element (still recompiled; names are guesses).
constexpr u32 WorldObj_FindByHash = 0x005801DC;   // (u32 name hash, 0) -> object or 0 (name?)
constexpr u32 HashMap_Find = 0x007399A4;          // (map, const u32* key) -> value slot (name?)
constexpr u32 WorldObj_Instance = 0x005A05E8;     // (class?, 0) -> object (name?)
constexpr u32 WorldObj_SetName = 0x0057D368;      // (obj, const char*, 1) (name?)
constexpr u32 WorldObj_Func583E54 = 0x00583E54;   // (obj, 12) (name?)
constexpr u32 WorldObj_SetScale = 0x0057D100;     // (obj, const f32[3], 0) (name?)
constexpr u32 UiElem8_Attach = 0x004D7CF8;        // (e, obj, 1) (name?)
// Texture files.
constexpr u32 Texture_Alloc = 0x0051D368;         // () -> Texture* (name?)
constexpr u32 Texture_Init = 0x0053A640;          // (tex, data, size, ext, path) -> bool (name?)
constexpr u32 Map_Find = 0x0079DD10;              // (map, const u32* key) -> value slot (name?)
}  // namespace mfn

// Material (AglMaterial, 144 bytes) fields used here.
constexpr u32 kMatShader = 16;     // Shader*
constexpr u32 kMatTextures = 20;   // Texture*[]
constexpr u32 kMatTexCount = 44;   // u8
constexpr u32 kMatTex0Word = 48;   // u16: low half of the first texture's +112
constexpr u32 kMatBlend = 104;     // BlendState: +0 enable / src factor, +4 equation, +6 dst factor, +8 depth
constexpr u32 kShaderUntextured = 0x38FF7607, kShaderTextured = 0x1808E1E8;

// BlendState (Material +104).
constexpr u32 kBlendEnable = 0x80000000;  // +0 bit 31
constexpr u32 kBlendFieldMask = 0x7FFF8000; // +0 src factor / +8 depth func, in bits 15..30
constexpr u32 kBlendFieldShift = 15;
constexpr u16 kGlFuncAdd = 0x8006, kGlFuncReverseSubtract = 0x800B;
constexpr u32 kGlZero = 0, kGlOne = 1, kGlSrcAlpha = 0x302, kGlOneMinusSrcAlpha = 0x303;
constexpr u32 kGlLequal = 0x203, kGlAlways = 0x207;

// The blend modes a UI blend index (0 "av", 1 "ad", 2 "su") maps to.
constexpr u32 kBlendModes = 0x00AEF7F8;  // s32[]: 2, 3, 4 (index 3 reads the 0 after them)

// Sets a material's blend state for a blend mode (inlined twice in the
// original, with the three stores in different orders).
inline void Blend_SetMode(u32 b, s32 mode) {
    u32 w = load<u32>(b);
    u16 eq, dst;
    u32 src;
    bool enable = true;
    switch (mode) {
        case 0: eq = kGlFuncAdd; dst = kGlZero; src = kGlOne; enable = false; break;  // no blending
        case 1: eq = kGlFuncAdd; dst = kGlZero; src = kGlOne; break;
        case 2: eq = kGlFuncAdd; dst = kGlOneMinusSrcAlpha; src = kGlSrcAlpha; break;  // "av": alpha
        case 3: eq = kGlFuncAdd; dst = kGlOne; src = kGlSrcAlpha; break;               // "ad": additive
        case 4: eq = kGlFuncReverseSubtract; dst = kGlOne; src = kGlSrcAlpha; break;   // "su": subtractive
        default: return;
    }
    w = ((w | kBlendEnable) & ~kBlendFieldMask) | ((src << kBlendFieldShift) & kBlendFieldMask);
    if (!enable) w &= ~kBlendEnable;
    store<u16>(b + 4, eq);
    store<u16>(b + 6, dst);
    store<u32>(b, w);
}

inline void Blend_SetDepth(u32 b, u32 func) {
    const u32 w = load<u32>(b + 8);
    store<u32>(b + 8, (w & ~kBlendFieldMask) | ((func << kBlendFieldShift) & kBlendFieldMask));
}

// Element fields.
constexpr u32 kElemFlags = 136;      // u32: bits 22-23 blend index, 0x02000000 textured, 0x10000000 scale by parent
constexpr u32 kElemMaterial = 168;   // types 1 / 6: Material*; type 8: world object
constexpr u32 kElemTexRes = 172;     // UiTexture*
constexpr u32 kElemUv0 = 176, kElemUv1 = 184;
constexpr u32 kElemMaterial2 = 192;  // type 2: Material*
constexpr u32 kVfnSetBlend = 64;     // element: (u32 blend)
constexpr u32 kVfnRelease = 8;
constexpr u32 kZeroVec2 = 0x01164AFC, kOneVec2 = 0x01164AF4;

// UiTexture / UiRes fields (src/font/res.h, tex.h).
constexpr u32 kResRefCount = 4, kResUsers = 80, kResLoaded = 82, kResVfnLoad = 12;
constexpr u32 kTexFlags = 92, kTexHandle = 96, kTexPages = 100;

constexpr u32 kTexDefaults = 0x010DDADC;  // Texture*[]; +24: the map of loaded textures by file hash
constexpr u32 kTimelineDefVtbl = 0x00B22F60;
constexpr u32 kTimelineDefs = 0x00F6689C;  // TimelineDef*, linked by +52
constexpr u32 kTimelineDefSize = 56;
constexpr u32 kHashTimelineFlag = 0x0023BEA4;  // bool -> TimelineDef +44 (name?)
constexpr u32 kStrName = 0x00AEF4C0;           // "name"

// The world-object lookup the type-8 element falls back to.
constexpr u32 kWorldClassMapHolder = 0x00F8F12C;  // u32 object; +16: map of name hash -> class (name?)
constexpr u32 kStrGuiInst = 0x00AEF56C;           // "%s_GUIInst"
constexpr u32 kConfig = 0x010C5A20;               // g::config: +4 / +6 u16 width / height, +16 f32
constexpr u32 kElem8Vtbl = 0x00B22DD0;

// ---------------------------------------------------------------- native structures
using native::gp;
using native::pad;

// A reference-counted object (resources, materials, textures): +4 u16 count, vtable +8 Release().
struct RefObj {
    be_u32 vtable;     // +0
    be_u16 refs;       // +4
};
inline void AddRef(void* obj) { auto& r = *static_cast<RefObj*>(obj); r.refs = static_cast<u16>(r.refs + 1); }
inline void Release(void* obj) { native::vcall(obj, kVfnRelease); }

// Material (AglMaterial, 144 bytes) as far as used here.
struct Material {
    be_u32 vtable;           // +0
    be_u16 refs;             // +4
    pad<10> _6;
    be_u32 shader;           // +16 Shader*
    be_u32 textures[6];      // +20 Texture*[]
    u8 tex_count;            // +44
    pad<3> _45;
    be_u16 tex0_word;        // +48 low half of the first texture's +112
    pad<54> _50;
    be_u32 blend[3];         // +104 BlendState: +0 enable / src factor, +4 equation, +6 dst factor, +8 depth
};
static_assert(offsetof(Material, tex_count) == kMatTexCount && offsetof(Material, blend) == kMatBlend);

// The UI texture resource (src/font/res.h).
using font::UiTexture;

// The element fields the texture / blend code uses (types 1 / 2 / 6 / 8).
struct UiElemTex {
    text::UiElem elem;       // +0
    be_u32 material;         // +168 types 1 / 6: Material*; type 8: world object
    be_u32 tex_res;          // +172 UiTexture*
    be_u32 uv0[2];           // +176 (f32 bits)
    be_u32 uv1[2];           // +184
    be_u32 material2;        // +192 type 2: Material*
};
static_assert(offsetof(UiElemTex, material2) == kElemMaterial2);

// Sets a material's blend state for a blend mode (inlined twice in the
// original, with the three stores in different orders).
inline void Blend_SetMode(be_u32* b, s32 mode) {
    u32 w = b[0];
    u16 eq, dst;
    u32 src;
    bool enable = true;
    switch (mode) {
        case 0: eq = kGlFuncAdd; dst = kGlZero; src = kGlOne; enable = false; break;  // no blending
        case 1: eq = kGlFuncAdd; dst = kGlZero; src = kGlOne; break;
        case 2: eq = kGlFuncAdd; dst = kGlOneMinusSrcAlpha; src = kGlSrcAlpha; break;  // "av": alpha
        case 3: eq = kGlFuncAdd; dst = kGlOne; src = kGlSrcAlpha; break;               // "ad": additive
        case 4: eq = kGlFuncReverseSubtract; dst = kGlOne; src = kGlSrcAlpha; break;   // "su": subtractive
        default: return;
    }
    w = ((w | kBlendEnable) & ~kBlendFieldMask) | ((src << kBlendFieldShift) & kBlendFieldMask);
    if (!enable) w &= ~kBlendEnable;
    be_u16* h = reinterpret_cast<be_u16*>(b);
    h[2] = eq;
    h[3] = dst;
    b[0] = w;
}
inline void Blend_SetDepth(be_u32* b, u32 func) {
    const u32 w = b[2];
    b[2] = (w & ~kBlendFieldMask) | ((func << kBlendFieldShift) & kBlendFieldMask);
}

// Native functions (src/uiload/more.cpp).
xml::XmlCursor* XmlCursor_FirstChildRawAny(xml::XmlCursor* cursor);
xml::XmlCursor* XmlCursor_FirstChildAny(xml::XmlCursor* cursor);
xml::XmlCursor* XmlCursor_NextSiblingAny(xml::XmlCursor* cursor);
const char* XmlCursor_Text(xml::XmlCursor* cursor);
s32 String_CompareN(const char* a, const char* b, s32 n);
Material* Tex_Acquire(void* tex);
void UiElem2_SetBlend(UiElemTex* e, u32 blend);
void Texture_MakePages(UiTexture* t);
void UiElem_SetTexture(UiElemTex* e, UiTexture* res, const be_u32* uv0, const be_u32* uv1);
u32 Tex_FindLoaded(u32 hash);
void* Tex_LoadFile(const char* path, u32 fallback);
void* Timeline_FromXml(xml::XmlCursor* doc, xml::XmlCursor* node, void* inc);
void UiElem8_Construct(UiElemTex* e, const void* d);

}  // namespace uimore
}  // namespace sly4
