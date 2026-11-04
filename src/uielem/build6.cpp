// UiElem6_Build (the type-6 element's update) and the element-defaults
// initialiser its slice children are made with.
#include "build6.h"

#include <cmath>

using namespace sly4;
using namespace sly4::build6;

namespace {

inline f64 lfs(u32 ea) { return static_cast<f64>(load<f32>(ea)); }
inline void stfs(u32 ea, f64 v) { store<f32>(ea, static_cast<f32>(v)); }
inline f64 rs(f64 v) { return static_cast<f64>(static_cast<f32>(v)); }
inline f64 fmuls(f64 a, f64 b) { return rs(a * b); }
inline f64 fadds(f64 a, f64 b) { return rs(a + b); }
inline f64 fsubs(f64 a, f64 b) { return rs(a - b); }
inline f64 fdivs(f64 a, f64 b) { return rs(a / b); }
inline f64 fnmsubs(f64 a, f64 c, f64 b) { return rs(-std::fma(a, c, -b)); }
inline void copy32(u32 dst, u32 src) { store<u32>(dst, load<u32>(src)); }  // lfs / stfs pairs

// Virtual call with r(n) / r(n+1) = OPD and entry after the arguments.
u64 Vcall(PPUContext& ctx, u32 o, u32 slot) {
    const u32 opd = load<u32>(load<u32>(o) + slot);
    return call_ptr(ctx, opd, {o, opd, load<u32>(opd)});
}
u64 Vcall1(PPUContext& ctx, u32 o, u32 slot, u32 a) {
    const u32 opd = load<u32>(load<u32>(o) + slot);
    return call_ptr(ctx, opd, {o, a, opd, load<u32>(opd)});
}

// A UiTexture's loaded texture, or default texture 0 while it isn't loaded.
u32 LoadedTexture(PPUContext& ctx, u32 res) {
    if (load<u16>(res + kResLoaded) == 2) return load<u32>(res + kTexHandle);
    return static_cast<u32>(call(ctx, b6fn::Tex_GetDefault, {0}));
}

// The slice table on the stack: 9 entries of 28 bytes.
struct Slice {
    u32 base;
    u32 active() const { return base; }   // u8
    u32 w() const { return base + 4; }    // f32
    u32 h() const { return base + 8; }
    u32 x() const { return base + 12; }
    u32 y() const { return base + 16; }
    u32 res() const { return base + 20; } // UiTexture*
    u32 flags() const { return base + 24; }  // 1 mirror x, 2 mirror y
};

// Stack frame layout (offsets from the original r1).
constexpr u32 kSpScale = 112;      // GetScale result, f32[3]
constexpr u32 kSpDesc = 128;       // the child description (160 bytes)
constexpr u32 kSpSize = 288;       // GetSize result, f32[2]
constexpr u32 kSpTexSize = 296;    // f32[2]
constexpr u32 kSpTable = 304;      // Slice[9]
constexpr u32 kSpColor = 556;      // GetWorldColor result, u8[4]
constexpr u32 kSpAxis = 584;       // s32: the axis a 3-slice runs along (0 x, 1 y)
constexpr u32 kSpColorArg = 56;    // the colour, re-packed for SetColor (in the linkage area)

// Child description fields (sp + kSpDesc + ...).
constexpr u32 kDName = 0, kDMatrix = 16, kDPos = 64, kDScale = 80, kDColor = 96, kDBlend = 100, kDOwner = 104;
constexpr u32 kDRes = 128, kDUv0 = 136, kDUv1 = 144, kDMinus = 152;

void BuildSlices(PPUContext& ctx, u32 e, u32 sp) {
    auto T = [&](u32 k) { return Slice{sp + kSpTable + 28 * k}; };
    {
        const u32 opd = load<u32>(load<u32>(e) + kVfnGetSize);
        call_ptr(ctx, opd, {sp + kSpSize, e, opd, load<u32>(opd)});
    }
    {
        const u32 opd = load<u32>(load<u32>(e) + kVfnGetScale);
        call_ptr(ctx, opd, {sp + kSpScale, e, opd, load<u32>(opd)});
    }
    {
        const u32 opd = load<u32>(load<u32>(e) + kVfnGetWorldColor);
        call_ptr(ctx, opd, {sp + kSpColor, e, opd, load<u32>(opd)});
    }
    const f64 c_one = lfs(kConsts), c_two = lfs(kConsts + 4), c_half = lfs(kConsts + 8), c_zero = lfs(kConsts + 12);

    // Which layout the present slices need: corners -> 9; top / bottom
    // edges -> 3 along y unless a side is also present (then 9); sides -> 3
    // along x (or 9 with a top / bottom edge); the centre alone -> 1. The
    // texture size is that of the last slice looked at.
    u32 count = 0, axis = 0;
    f64 tw = lfs(kZeroVec2), th = lfs(kZeroVec2 + 4);
    stfs(sp + kSpTexSize, tw);
    stfs(sp + kSpTexSize + 4, th);
    for (u32 s = 0; s < kSliceCount && count < 9; ++s) {
        const u32 res = load<u32>(e + kElemSlices + 4 * s);
        if (res == 0) continue;
        switch (s) {
            case 0: case 2: case 6: case 8: count = 9; break;
            case 1: case 7:
                count = count > 3 ? count : 3;
                if (count == 3) {
                    if (load<u32>(e + kElemSlices + 12) != 0 || load<u32>(e + kElemSlices + 20) != 0) count = 9;
                    else axis = 1;
                }
                break;
            case 3: case 5:
                count = count > 3 ? count : 3;
                if (count == 3 &&
                    (load<u32>(e + kElemSlices + 4) != 0 || load<u32>(e + kElemSlices + 28) != 0))
                    count = 9;
                break;
            case 4: count = count > 1 ? count : 1; break;
        }
        const u32 tex = load<u16>(res + kResLoaded) == 2 ? load<u32>(res + kTexHandle)
                                                         : static_cast<u32>(call(ctx, b6fn::Tex_GetDefault, {0}));
        tw = static_cast<f64>(static_cast<f32>(static_cast<s32>(load<u16>(tex + kTextureWidth))));
        th = static_cast<f64>(static_cast<f32>(static_cast<s32>(load<u16>(tex + kTextureHeight))));
        stfs(sp + kSpTexSize, tw);
        stfs(sp + kSpTexSize + 4, th);
    }
    store<u32>(sp + kSpAxis, axis);
    if (count == 0) {
        // Nothing to draw: size by default texture 1 if there is one.
        const u32 tex = static_cast<u32>(call(ctx, b6fn::Tex_GetDefault, {1}));
        if (tex != 0) {
            tw = static_cast<f64>(static_cast<f32>(static_cast<s32>(load<u16>(tex + kTextureWidth))));
            th = static_cast<f64>(static_cast<f32>(static_cast<s32>(load<u16>(tex + kTextureHeight))));
            stfs(sp + kSpTexSize, tw);
            stfs(sp + kSpTexSize + 4, th);
        }
    }

    call(ctx, b6fn::memset, {sp + kSpTable, 0, 28 * kSliceCount});
    for (u32 k = 0; k < kSliceCount; ++k) copy32(T(k).res(), e + kElemSlices + 4 * k);

    const f64 size_w = lfs(sp + kSpSize), size_h = lfs(sp + kSpSize + 4);
    const f64 scale_x = lfs(sp + kSpScale), scale_y = lfs(sp + kSpScale + 4);
    const s32 n = static_cast<s32>(count);
    if (n <= 2) {
        // One slice: the centre covers the whole element.
        if (n != 2) {
            store<u8>(T(4).active(), 1);
            stfs(T(4).w(), size_w);
            stfs(T(4).h(), size_h);
        }
    } else if (n > 8) {
        if (n <= 9) {
            // Nine slices: corner sizes from the texture (halved if the
            // element is too small), the middle row / column takes the rest.
            f64 x = lfs(kZeroVec2), y = lfs(kZeroVec2 + 4);
            for (u32 k = 0; k < kSliceCount; ++k) {
                const u32 row = k / 3, col = k - 3 * (k / 3);
                const Slice t = T(k);
                store<u8>(t.active(), 1);
                if ((row & 1) != 0) {
                    stfs(t.h(), fnmsubs(lfs(T(0).h()), c_two, size_h));
                } else {
                    const f64 need = fmuls(fmuls(th, c_two), scale_y);
                    stfs(t.h(), size_h >= need ? fmuls(th, scale_y) : fmuls(size_h, c_half));
                }
                f64 w;
                if ((col & 1) != 0) {
                    w = fnmsubs(lfs(T(0).w()), c_two, size_w);
                } else {
                    const f64 need = fmuls(fmuls(tw, c_two), scale_x);
                    w = size_w >= need ? fmuls(tw, scale_x) : fmuls(size_w, c_half);
                }
                stfs(t.w(), w);
                if (col == 0) x = c_zero;
                stfs(t.x(), x);
                stfs(t.y(), y);
                x = fadds(x, w);
                if (col == 2) y = fadds(y, lfs(t.h()));
                // A missing slice mirrors another (x, y or both), else uses the centre's.
                for (u32 j = 0; load<u32>(t.res()) == 0;) {
                    if (j >= 3) {
                        store<u32>(t.flags(), 0);
                        copy32(t.res(), e + kElemSlices + 16);
                        break;
                    }
                    const s32 alt = load<s32>(kSliceMirrors + k * 12 + j * 4);
                    ++j;
                    if (alt != -1) {
                        store<u32>(t.flags(), j);
                        copy32(t.res(), e + kElemSlices + 4 * static_cast<u32>(alt));
                    }
                }
            }
        }
    } else if (n <= 3) {
        if (load<u32>(sp + kSpAxis) == 0) {
            // Three slices along x: 3 / 4 / 5.
            store<u8>(T(3).active(), 1);
            store<u8>(T(4).active(), 1);
            store<u8>(T(5).active(), 1);
            stfs(T(3).h(), size_h);
            stfs(T(4).h(), size_h);
            stfs(T(5).h(), size_h);
            const f64 need = fmuls(fmuls(tw, c_two), scale_x);
            const f64 edge = size_w >= need ? fmuls(tw, scale_x) : fmuls(size_w, c_half);
            stfs(T(3).w(), edge);
            stfs(T(5).w(), edge);
            stfs(T(4).x(), edge);
            const f64 mid = fsubs(size_w, fadds(edge, edge));
            stfs(T(4).w(), mid);
            stfs(T(5).x(), fadds(edge, mid));
            const u32 left = load<u32>(T(3).res()), right = load<u32>(T(5).res());
            u32 r3 = left;
            if (left == 0) {
                r3 = right;
                store<u32>(T(3).res(), right);
                store<u32>(T(3).flags(), load<u32>(T(3).flags()) | 1);
            }
            if (right == 0) {
                store<u32>(T(5).res(), r3);
                store<u32>(T(5).flags(), load<u32>(T(5).flags()) | 1);
            }
        } else {
            // Three slices along y: 1 / 4 / 7.
            store<u8>(T(1).active(), 1);
            store<u8>(T(4).active(), 1);
            store<u8>(T(7).active(), 1);
            stfs(T(1).w(), size_w);
            stfs(T(4).w(), size_w);
            stfs(T(7).w(), size_w);
            const f64 need = fmuls(fmuls(th, c_two), scale_y);
            const f64 edge = size_h >= need ? fmuls(th, scale_y) : fmuls(size_h, c_half);
            stfs(T(1).h(), edge);
            stfs(T(7).h(), edge);
            // As in the original: the middle's height goes into its width,
            // and the bottom is placed after the (zero) middle height.
            stfs(T(4).w(), fsubs(size_h, fadds(lfs(T(1).h()), edge)));
            copy32(T(4).y(), T(1).h());
            stfs(T(7).y(), fadds(lfs(T(1).h()), lfs(T(4).h())));
            const u32 top = load<u32>(T(1).res()), bottom = load<u32>(T(7).res());
            u32 r4 = top;
            if (top == 0) {
                r4 = bottom;
                store<u32>(T(1).res(), bottom);
                store<u32>(T(1).flags(), load<u32>(T(1).flags()) | 2);
            }
            if (bottom == 0) {
                store<u32>(T(7).res(), r4);
                store<u32>(T(7).flags(), load<u32>(T(7).flags()) | 2);
            }
        }
    }

    // Make, update or delete each slice's child.
    const u32 d = sp + kSpDesc;
    for (u32 k = 0; k < kSliceCount; ++k) {
        const Slice t = T(k);
        const u32 slot = e + kElemSliceElems + 4 * k;
        if (load<u8>(t.active()) == 0) {
            const u32 child = load<u32>(slot);
            if (child != 0) {
                Vcall(ctx, child, kVfnDelete);
                store<u32>(slot, 0);
            }
            continue;
        }
        const u32 row = k / 3, col = k - 3 * (k / 3);
        // The child description (an inlined constructor; only parts of it are used).
        store<u32>(d + kDName, static_cast<u32>(call(ctx, b6fn::String_Format, {kStrSliceName, k + 1})));
        for (u32 r = 0; r < 4; ++r)
            for (u32 c = 0; c < 3; ++c) copy32(d + kDMatrix + 16 * r + 4 * c, kIdentity + 16 * r + 4 * c);
        for (u32 c = 0; c < 3; ++c) copy32(d + kDScale + 4 * c, kOneVec3 + 4 * c);
        for (u32 c = 0; c < 4; ++c) store<u8>(d + kDColor + c, load<u8>(kWhite + c));
        for (u32 off : {kDBlend, kDOwner, 108u, 112u, 116u}) store<u32>(d + off, 0);
        store<u8>(d + 120, 1);
        store<u8>(d + 121, 0);
        store<u8>(d + 122, 0);
        store<u8>(d + 123, 1);
        store<u8>(d + 124, 1);
        store<u8>(d + 125, 1);
        store<u8>(d + 126, 0);
        copy32(d + kDRes, t.res());
        store<u32>(d + 132, 0);
        copy32(d + kDUv0, kZeroVec2);
        copy32(d + kDUv0 + 4, kZeroVec2 + 4);
        copy32(d + kDUv1, kOneVec2);
        copy32(d + kDUv1 + 4, kOneVec2 + 4);
        store<u32>(d + kDMinus, kMinusOne);
        store<u32>(d + kDMinus + 4, kMinusOne);
        store<u32>(d + kDOwner, e);
        copy32(d + kDPos, t.x());
        copy32(d + kDPos + 4, t.y());
        store<u32>(d + kDPos + 8, 0);
        const u32 eflags = load<u32>(e + kElemFlags);
        u8 color[4];
        for (u32 c = 0; c < 4; ++c) color[c] = load<u8>(sp + kSpColor + c);
        const u32 sflags = load<u32>(t.flags());
        // Per axis: the slice's size over the texture's (scaled) size; it
        // is stretched (scale *= ratio, uv1 = 1) or tiled (uv1 = ratio).
        for (u32 a = 0; a <= 1; ++a) {
            const f64 sc = lfs(sp + kSpScale + 4 * a);
            const f64 ratio = fdivs(lfs(t.w() + 4 * a), fmuls(lfs(sp + kSpTexSize + 4 * a), sc));
            bool stretch;
            if (load<u32>(d + kDRes) == 0) {
                stretch = true;
            } else if (count == 3 && static_cast<s32>(a) != load<s32>(sp + kSpAxis)) {
                stretch = true;
            } else if (ratio < c_one) {
                stretch = a == 0 ? col != 1 : row != 1;
            } else {
                stretch = false;
            }
            if (stretch) {
                stfs(d + kDScale + 4 * a, fmuls(ratio, sc));
                store<u32>(d + kDUv1 + 4 * a, kF32One);
            } else {
                stfs(d + kDScale + 4 * a, sc);
                stfs(d + kDUv1 + 4 * a, ratio);
            }
        }
        store<u32>(d + kDBlend, (eflags >> 22) & 3);
        for (u32 c = 0; c < 4; ++c) store<u8>(d + kDColor + c, color[c]);
        if ((sflags & 1) != 0) {
            store<u32>(d + kDUv0, kF32One);
            stfs(d + kDUv1, -lfs(d + kDUv1));
        }
        if ((sflags & 2) != 0) {
            store<u32>(d + kDUv0 + 4, kF32One);
            stfs(d + kDUv1 + 4, -lfs(d + kDUv1 + 4));
        }

        u32 child = load<u32>(slot);
        if (child == 0) {
            const u32 m = static_cast<u32>(call(ctx, b6fn::SmallAlloc, {kSliceElemSize}));
            if (m != 0) {
                call(ctx, b6fn::UiElem_InitDefaults, {m});
                store<u32>(m + 168, 0);
                store<u32>(m + 172, 0);
                store<u32>(m, kSliceVtbl);
                copy32(m + 176, kZeroVec2);
                copy32(m + 180, kZeroVec2 + 4);
                copy32(m + 184, kOneVec2);
                store<u32>(m + 192, kMinusOne);
                copy32(m + 188, kOneVec2 + 4);
                store<u32>(m + 196, kMinusOne);
            }
            store<u32>(slot, m);
            store<u32>(m + kElemParent, e);  // written through 0 if the allocation failed, as in the original
            child = load<u32>(slot);
        }
        Vcall1(ctx, child, kVfnSetPosition, d + kDPos);
        Vcall1(ctx, load<u32>(slot), kVfnSetScale, d + kDScale);
        {
            for (u32 c = 0; c < 4; ++c) store<u8>(sp + kSpColorArg + c, load<u8>(d + kDColor + c));
            const u32 o = load<u32>(slot);
            const u32 opd = load<u32>(load<u32>(o) + kVfnSetColor);
            // r5 = OPD; r6 / r7 = colour bytes 2 / 3 here in the original (not read).
            call_ptr(ctx, opd, {o, load<u32>(sp + kSpColorArg), opd, load<u8>(d + kDColor + 2), load<u8>(d + kDColor + 3)});
        }
        u32 res = load<u32>(d + kDRes);
        if (res != 0 || count == 0) {
            call(ctx, b6fn::UiElem_SetTexture, {load<u32>(slot), res, d + kDUv0, d + kDUv1});
            Vcall1(ctx, load<u32>(slot), kVfnSetBlend, load<u32>(d + kDBlend));
            res = load<u32>(d + kDRes);
        }
        if (res != 0 && LoadedTexture(ctx, res) != 0) {
            // The middle row / column repeats, the rest clamps.
            const u32 tex = LoadedTexture(ctx, res);
            const u32 ws = col == 1 ? kWrapRepeat : kWrapClamp, wt = row == 1 ? kWrapRepeat : kWrapClamp;
            const u32 opd = load<u32>(load<u32>(tex) + kTexVfnSetWrap);
            call_ptr(ctx, opd, {tex, ws, wt, 1, opd, load<u32>(opd)});  // r7 / r8 = OPD and entry
        }
        Vcall1(ctx, load<u32>(slot), kVfnSetActive, 1);
    }
    store<u8>(e + kElemRebuild, 0);
}

}  // namespace

// void UiElem_InitDefaults(Element* e)  (symbols.txt: UiElem6_AddSlice)
// Base element defaults for a freshly allocated element: base vtable,
// identity transform, unit scale, white, cleared links and flags
// (+136 = low 6 bits kept | 0x03000000), zero +140 vector.
DECOMP(004D8D48, UiElem_InitDefaults) {
    const u32 e = arg<u32>(ctx, 0);
    for (u32 r = 0; r < 4; ++r)
        for (u32 c = 0; c < 3; ++c) {
            if (r == 3 && c == 0) store<u32>(e, kVtblBase);
            copy32(e + 16 + 16 * r + 4 * c, kIdentity + 16 * r + 4 * c);
        }
    for (u32 c = 0; c < 3; ++c) copy32(e + 80 + 4 * c, kOneVec3 + 4 * c);
    store<u32>(e + 128, 0);
    for (u32 c = 0; c < 4; ++c) store<u8>(e + 132 + c, load<u8>(kWhite + c));
    const u32 w = load<u32>(e + 136);
    store<u32>(e + 148, 0);
    store<u32>(e + 152, 0);
    store<u32>(e + 136, (w & 0x3Fu) | 0x03000000u);
    copy32(e + 140, kZeroVec2);
    store<u32>(e + 156, 0);
    copy32(e + 144, kZeroVec2 + 4);
    store<u32>(e + 160, 0);
    store<u8>(e + 96, 0);
    ret(ctx, e);
}

// void UiElem6_Build(Element6* e; f32 dt in f1)  (vfunc +152 of type 6)
// When +248 is set, lays out the nine slice textures at +168 (see
// BuildSlices) into up to nine children at +204 (made on demand as
// 208-byte elements of vtable 0x00B229E8, named "01".."09"), deletes
// the children of unused slices and clears +248. Then, if +136 has
// 0x80000000, updates every child (vfunc +152, dt).
DECOMP(004D8E88, UiElem6_Build) {
    const u32 e = arg<u32>(ctx, 0);
    const f64 dt = farg(ctx, 0);
    {
        Stack frame(ctx, 832 - 112);
        if (load<u8>(e + kElemRebuild) != 0) BuildSlices(ctx, e, frame.addr() - 112);
    }
    if ((load<u32>(e + kElemFlags) & 0x80000000u) == 0) return;
    for (u32 c = load<u32>(e + kElemFirstChild); c != 0; c = load<u32>(c + kElemNextSibling)) {
        ctx.fpr[1] = dt;
        Vcall(ctx, c, kVfnUpdate);
    }
}
