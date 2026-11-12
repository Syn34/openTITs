// Camera per-split accessors (split-screen views; View_SubmitSplit and
// 0x0043807C use them) and the split style's two colour getters.
//
// Camera +68 + 24 * index (index 0..1, unchecked by the plain getters):
// SplitRec { +0 SplitStyle* style; +4 A; +8 B; +12 Xform; ... }.
// SplitStyle: +12 u8[4] colour A (top), +16 u8[4] colour B (bottom).
#include "camera.h"

namespace sly4::view {
using namespace opentit::native;

namespace {

constexpr u32 kSplitRecs = 68, kSplitRecSize = 24, kSplitCount = 2;
constexpr u32 kStyleColourA = 12, kStyleColourB = 16;
constexpr u32 kDefaultColour = 0x01164CA8;  // u8[4] used when there's no style / split
constexpr u32 kStyleMap = 0x0106C2A8;       // map from style key to SplitStyle* (name?)

namespace cfn2 {
constexpr u32 Map_Get = 0x0075EA74;         // (Map*, const u32* key) -> value slot (name?)
constexpr u32 SplitStyle_Set = 0x0036A5F0;  // (SplitStyle** slot, SplitStyle*, 0) (name?)
}  // namespace cfn2

struct SplitRec {
    be_u32 style;   // +0 SplitStyle*
    be_u32 a;       // +4
    be_u32 b;       // +8
    be_u32 xform;   // +12: +112 the split's world matrix, +176 its fov
    pad<8> _16;
};
static_assert(sizeof(SplitRec) == kSplitRecSize);

// Split `index` of the camera (32-bit address arithmetic: index unchecked).
SplitRec& split_rec(const void* cam, u32 index) { return gref<SplitRec>(guest(cam) + kSplitRecSize * index + kSplitRecs); }

void copy_colour(u8* dst, const u8* src) {
    for (u32 i = 0; i < 4; ++i) dst[i] = src[i];
}

// Camera_GetSplitColourA / B: the style's colour for split `index`
// (through a 4-byte stack copy, sp+112 of a 128-byte frame), or the
// default colour for index >= 2.
template <typename Getter>
void split_colour(u8* out, const void* cam, u32 index, Getter getter) {
    if (index >= kSplitCount) {
        copy_colour(out, host<u8>(kDefaultColour));
        return;
    }
    GuestFrame frame(16);
    u8* tmp = frame.as<u8>();
    getter(tmp, &split_rec(cam, index).style);
    copy_colour(out, tmp);
}

// SplitStyle_GetColourA / B body: *slot's colour at `offset`, or the
// default colour when there is no style.
void style_colour(u8* out, const be_u32* slot, u32 offset) {
    if (const u32 style = *slot) copy_colour(out, &gref<u8>(style + offset));
    else copy_colour(out, host<u8>(kDefaultColour));
}

}  // namespace

// Looks key up in the style map (0x0106C2A8) and sets the found style
// on split `index`; nothing for index >= 2.
void Camera_SetSplitStyle(void* cam, u32 index, u32 key) {
    if (index >= kSplitCount) return;
    GuestFrame frame(16);  // the key at sp+112 of a 128-byte frame
    be_u32* k = frame.as<be_u32>();
    *k = key;
    const be_u32* slot = call<be_u32*>(cfn2::Map_Get, kStyleMap, k);
    call(cfn2::SplitStyle_Set, &split_rec(cam, index).style, gref<be_u32>(guest(slot)).get(), 0u);
}

u32 Camera_GetSplitA(void* cam, u32 index) { return split_rec(cam, index).a; }
u32 Camera_GetSplitB(void* cam, u32 index) { return split_rec(cam, index).b; }
u32 Camera_GetSplitXform(void* cam, u32 index) { return split_rec(cam, index).xform; }

void SplitStyle_GetColourA(u8* out, const be_u32* slot) { style_colour(out, slot, kStyleColourA); }
void SplitStyle_GetColourB(u8* out, const be_u32* slot) { style_colour(out, slot, kStyleColourB); }

void Camera_GetSplitColourA(u8* out, void* cam, u32 index) { split_colour(out, cam, index, SplitStyle_GetColourA); }
void Camera_GetSplitColourB(u8* out, void* cam, u32 index) { split_colour(out, cam, index, SplitStyle_GetColourB); }

}  // namespace sly4::view

NATIVE(002AB920, sly4::view::Camera_SetSplitStyle)
NATIVE(002AB9B0, sly4::view::Camera_GetSplitA)
NATIVE(002AB9D0, sly4::view::Camera_GetSplitB)
NATIVE(002AB9F0, sly4::view::Camera_GetSplitXform)
NATIVE(002ABA10, sly4::view::Camera_GetSplitColourA)
NATIVE(002ABAB8, sly4::view::Camera_GetSplitColourB)
NATIVE(0036AA04, sly4::view::SplitStyle_GetColourA)
NATIVE(0036AA68, sly4::view::SplitStyle_GetColourB)
