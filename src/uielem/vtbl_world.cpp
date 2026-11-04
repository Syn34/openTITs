// UiElem_GetWorldMatrix (vfunc +164, 0x004E0B30).
#include "vtbl.h"

using namespace sly4;
using namespace sly4::uielem;

namespace {
constexpr f32 kMinusHalf = -0.5f;  // 0x004E0B28
constexpr f32 kHalf = 0.5f;        // 0x004E0B2C
inline f64 to_f32(s32 v) { return static_cast<f64>(static_cast<f32>(static_cast<f64>(v))); }
}  // namespace

// Matrix UiElem_GetWorldMatrix(const Element* e)  [sret in r3, e in r4]
// The element's 3x3 (+16) and position (+64), the position moved to its
// anchor in the parent (+136 bits 6-7 / 10-11 in halves of the parent's
// size, from the parent's aligned corner) and scaled by the parent's scale
// when it inherits, times the parent's world matrix (the screen matrix and
// screen size without a parent). Only the xyz of each row is written; the
// product is the original's vmaddfp chain, lane by lane.
DECOMP(004E0B30, UiElem_GetWorldMatrix) {
    const u32 out = arg<u32>(ctx, 0);
    const u32 e = arg<u32>(ctx, 1);

    constexpr u32 kFrameBase = 112;
    Stack frame(ctx, 296 - kFrameBase);
    const u32 sp = frame.addr() - kFrameBase;
    const u32 pscale = sp + 112, pworld = sp + 192, psize = sp + 256;

    f64 pw, ph, minx, miny;
    const u32 parent = field<u32>(e, kElemParent);
    if (parent != 0) {
        call_ptr(ctx, vfunc(parent, kVfnSize), {psize, parent});
        const u32 p = field<u32>(e, kElemParent);
        const u32 pflags = field<u32>(p, kElemFlags);
        pw = lfs(psize);
        ph = lfs(psize + 4);
        minx = fmuls(pw, kMinusHalf);
        miny = fmuls(ph, kMinusHalf);
        call_ptr(ctx, vfunc(p, kVfnWorldMatrix), {pworld, p});
        minx = fmuls(minx, to_f32(static_cast<s32>((pflags >> 14) & 3)));
        miny = fmuls(miny, to_f32(static_cast<s32>((pflags >> 18) & 3)));
        for (u32 row = 0; row < 4; ++row)
            for (u32 k = 0; k < 3; ++k) store<u32>(out + row * 16 + k * 4, load<u32>(pworld + row * 16 + k * 4));
    } else {
        pw = to_f32(field<u16>(g::config, 4).get());
        ph = to_f32(field<u16>(g::config, 6).get());
        minx = lfs(kZeroVec2);
        miny = lfs(kZeroVec2 + 4);
        for (u32 row = 0; row < 4; ++row)
            for (u32 k = 0; k < 3; ++k)
                store<u32>(out + row * 16 + k * 4, load<u32>(kScreenMatrix + row * 16 + k * 4));
    }

    const u32 flags = field<u32>(e, kElemFlags);
    const f64 half_w = fmuls(pw, kHalf), half_h = fmuls(ph, kHalf);
    const f64 anchor_x = to_f32(static_cast<s32>((flags >> 6) & 3));
    const f64 anchor_y = to_f32(static_cast<s32>((flags >> 10) & 3));
    f32 L[3][3];
    for (u32 row = 0; row < 3; ++row)
        for (u32 k = 0; k < 3; ++k) L[row][k] = load<f32>(e + kElemMatrix + row * 16 + k * 4);
    f64 x = lfs(e + kElemPosition), y = lfs(e + kElemPosition + 4), z = lfs(e + kElemPosition + 8);
    const u32 p = field<u32>(e, kElemParent);
    if (p != 0 && (flags & kElemInheritScale) != 0) {
        call_ptr(ctx, vfunc(p, kVfnGetScale), {pscale, p});
        x = fmuls(x, lfs(pscale));
        y = fmuls(y, lfs(pscale + 4));
        z = fmuls(z, lfs(pscale + 8));
    }
    const f64 ty = fadds(fmadds(half_h, anchor_y, miny), y);
    const f64 tx = fadds(fmadds(half_w, anchor_x, minx), x);

    f32 P[4][3];
    for (u32 row = 0; row < 4; ++row)
        for (u32 k = 0; k < 3; ++k) P[row][k] = load<f32>(out + row * 16 + k * 4);
    const f32 T[3] = {static_cast<f32>(tx), static_cast<f32>(ty), static_cast<f32>(z)};
    for (u32 j = 0; j < 3; ++j) {
        for (u32 row = 0; row < 3; ++row) {
            f32 t = std::fma(P[0][j], L[row][0], 0.0f);
            t = std::fma(P[1][j], L[row][1], t);
            t = std::fma(P[2][j], L[row][2], t);
            store<f32>(out + row * 16 + j * 4, t);
        }
        f32 t = std::fma(P[0][j], T[0], 0.0f);
        t = std::fma(P[1][j], T[1], t);
        t = std::fma(P[2][j], T[2], t);
        store<f32>(out + 48 + j * 4, P[3][j] + t);
    }
}
