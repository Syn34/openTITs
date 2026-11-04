// Camera per-split accessors (split-screen views; View_SubmitSplit and
// 0x0043807C use them) and the split style's two colour getters.
//
// Camera +68 + 24 * index (index 0..1, unchecked by the plain getters):
// SplitRec { +0 SplitStyle* style; +4 A; +8 B; +12 Xform; ... }.
// SplitStyle: +12 u8[4] colour A (top), +16 u8[4] colour B (bottom).
#include "camera.h"

using namespace sly4;

namespace {

constexpr u32 kSplitRecs = 68, kSplitRecSize = 24, kSplitCount = 2;
constexpr u32 kRecStyle = 0, kRecA = 4, kRecB = 8, kRecXform = 12;
constexpr u32 kStyleColourA = 12, kStyleColourB = 16;
constexpr u32 kDefaultColour = 0x01164CA8;  // u8[4] used when there's no style / split
constexpr u32 kStyleMap = 0x0106C2A8;       // map from style key to SplitStyle* (name?)

namespace cfn {
constexpr u32 Map_Get = 0x0075EA74;               // (Map*, const u32* key) -> value slot (name?)
constexpr u32 SplitStyle_Set = 0x0036A5F0;        // (SplitStyle** slot, SplitStyle*, 0) (name?)
constexpr u32 SplitStyle_GetColourA = 0x0036AA04; // decompiled below
constexpr u32 SplitStyle_GetColourB = 0x0036AA68; // decompiled below
}  // namespace cfn

u32 split_rec(u32 cam, u32 index) { return cam + kSplitRecSize * index + kSplitRecs; }

// Copies 4 colour bytes; returns the last one (left in r3 / r4 by the
// original's byte loop).
u32 copy_colour(u32 dst, u32 src) {
    u32 b = 0;
    for (u32 i = 0; i < 4; ++i) {
        b = ref<u8>(src + i);
        ref<u8>(dst + i) = static_cast<u8>(b);
    }
    return b;
}

// Camera_GetSplitColourA / B: the style's colour for split `index`
// (through a 4-byte stack copy, sp+112 of a 128-byte frame), or the
// default colour for index >= 2.
void split_colour(PPUContext& ctx, u32 getter) {
    const u32 out = arg<u32>(ctx, 0), cam = arg<u32>(ctx, 1), index = arg<u32>(ctx, 2);
    if (index >= kSplitCount) {
        ctx.gpr[3] = copy_colour(out, kDefaultColour);
        ctx.gpr[4] = kDefaultColour;
        return;
    }
    Stack frame(ctx, 16);
    const u32 tmp = frame.addr();
    call(ctx, getter, {tmp, split_rec(cam, index) + kRecStyle});
    ctx.gpr[3] = copy_colour(out, tmp);
}

// SplitStyle_GetColourA / B body: *slot's colour at `offset`, or the
// default colour when there is no style.
void style_colour(PPUContext& ctx, u32 offset) {
    const u32 out = arg<u32>(ctx, 0), slot = arg<u32>(ctx, 1);
    const u32 style = load<u32>(slot);
    if (style != 0) {
        ctx.gpr[4] = copy_colour(out, style + offset);
        ctx.gpr[5] = ref<u8>(style + offset + 2);
    } else {
        ctx.gpr[4] = copy_colour(out, kDefaultColour);
        ctx.gpr[5] = kDefaultColour;
    }
}

}  // namespace

// void Camera_SetSplitStyle(Camera* cam, u32 index, u32 key)   (name?)
// Looks key up in the style map (0x0106C2A8) and sets the found style
// on split `index`; nothing for index >= 2 (r3 = key then).
DECOMP(002AB920, Camera_SetSplitStyle) {
    const u32 cam = arg<u32>(ctx, 0), index = arg<u32>(ctx, 1), key = arg<u32>(ctx, 2);
    if (index >= kSplitCount) {
        ctx.gpr[3] = key;
        return;
    }
    Stack frame(ctx, 16);  // the key at sp+112 of a 128-byte frame
    store<u32>(frame.addr(), key);
    const u32 slot = static_cast<u32>(call(ctx, cfn::Map_Get, {kStyleMap, frame.addr()}));
    call(ctx, cfn::SplitStyle_Set, {split_rec(cam, index) + kRecStyle, load<u32>(slot), 0});
}

// void* Camera_GetSplitA(Camera* cam, u32 index)   (index unchecked)
DECOMP(002AB9B0, Camera_GetSplitA) {
    ret(ctx, load<u32>(split_rec(arg<u32>(ctx, 0), arg<u32>(ctx, 1)) + kRecA));
}

// void* Camera_GetSplitB(Camera* cam, u32 index)   (index unchecked)
DECOMP(002AB9D0, Camera_GetSplitB) {
    ret(ctx, load<u32>(split_rec(arg<u32>(ctx, 0), arg<u32>(ctx, 1)) + kRecB));
}

// Xform* Camera_GetSplitXform(Camera* cam, u32 index)   (index unchecked)
// The object's +112 is the split's world matrix and +176 its fov.
DECOMP(002AB9F0, Camera_GetSplitXform) {
    ret(ctx, load<u32>(split_rec(arg<u32>(ctx, 0), arg<u32>(ctx, 1)) + kRecXform));
}

// void Camera_GetSplitColourA(u8 out[4], Camera* cam, u32 index)
DECOMP(002ABA10, Camera_GetSplitColourA) { split_colour(ctx, cfn::SplitStyle_GetColourA); }

// void Camera_GetSplitColourB(u8 out[4], Camera* cam, u32 index)
DECOMP(002ABAB8, Camera_GetSplitColourB) { split_colour(ctx, cfn::SplitStyle_GetColourB); }

// void SplitStyle_GetColourA(u8 out[4], SplitStyle* const* slot)   (name?)
DECOMP(0036AA04, SplitStyle_GetColourA) { style_colour(ctx, kStyleColourA); }

// void SplitStyle_GetColourB(u8 out[4], SplitStyle* const* slot)   (name?)
DECOMP(0036AA68, SplitStyle_GetColourB) { style_colour(ctx, kStyleColourB); }
