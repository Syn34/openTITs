// Element scale, rectangle and screen projection (callees of the text code).
#include "glyph.h"

using namespace sly4;
using namespace sly4::text;

namespace {
constexpr f32 kRectAlign = -0.5f;  // 0x004E0644
constexpr f32 kNdcOne = 1.0f;      // 0x00522744
constexpr f32 kNdcHalf = 0.5f;     // 0x00522748
constexpr f32 kNdcHalfUp = -0.5f;  // 0x0052274C
}  // namespace

// Vec3 UiElem_GetScale(const Element* e)  [vfunc +168, sret in r3, e in r4]
// The element's scale, times its parent's (recursively) when it inherits.
DECOMP(004D7E70, UiElem_GetScale) {
    const u32 out = arg<u32>(ctx, 0);
    const u32 e = arg<u32>(ctx, 1);
    for (u32 k = 0; k < 3; ++k) store<u32>(out + k * 4, load<u32>(e + kElemScale + k * 4));
    if ((field<u32>(e, kElemFlags) & kElemInheritScale) == 0) return;
    const u32 parent = field<u32>(e, kElemParent);
    if (parent == 0) return;
    Stack frame(ctx, 16);
    const u32 ps = frame.addr();
    call_ptr(ctx, vfunc(parent, kVfnGetScale), {ps, parent});
    stfs(out + 0, fmuls(lfs(out + 0), lfs(ps + 0)));
    stfs(out + 4, fmuls(lfs(out + 4), lfs(ps + 4)));
    stfs(out + 8, fmuls(lfs(out + 8), lfs(ps + 8)));
}

// void View_ProjectPoint(View* view, f32 out[4], const f32 in[3])
// Transforms `in` by the view's matrix and maps x/w and y/w from [-1, 1]
// to [0, 1] (y pointing down).
DECOMP(00522750, View_ProjectPoint) {
    const u32 view = arg<u32>(ctx, 0);
    const u32 out = arg<u32>(ctx, 1);
    const u32 in = arg<u32>(ctx, 2);
    call(ctx, gfn::View_Update, {view, 0});
    call(ctx, gfn::Vec_TransformByMatrix, {out, in, view + kViewProjMatrix});
    stfs(out + 0, fmuls(fadds(fdivs(lfs(out + 0), lfs(out + 12)), kNdcOne), kNdcHalf));
    stfs(out + 4, fmuls(fsubs(fdivs(lfs(out + 4), lfs(out + 12)), kNdcOne), kNdcHalfUp));
}

// void UiElem_GetRect(Element* e, f32 a[2], f32 b[2], f32 m[16])  [vfunc +160]
// a and b: the element's corners (its size offset by -0.5 * size * align),
// m: its world matrix. For a screen-space element (flag 0x20) while a 3D
// view is set, both corners are pushed through the world matrix and
// projected, made relative to the projected origin and scaled to pixels
// (config +4 / +6); m's translation then becomes the origin in pixels.
DECOMP(004E0648, UiElem_GetRect) {
    const u32 e = arg<u32>(ctx, 0);
    const u32 a = arg<u32>(ctx, 1);
    const u32 b = arg<u32>(ctx, 2);
    const u32 m = arg<u32>(ctx, 3);

    // The original's frame: the projection outputs at +112 / +128, the
    // world matrix at +144, two scratch matrices at +208 / +272, the size at +336.
    constexpr u32 kFrameBase = 112;
    Stack frame(ctx, 344 - kFrameBase);
    const u32 sp = frame.addr() - kFrameBase;
    const u32 proj = sp + 112, origin = sp + 128, world = sp + 144, tmp = sp + 208, offset = sp + 272,
              size = sp + 336;

    call_ptr(ctx, vfunc(e, kVfnSize), {size, e});
    const u32 flags = field<u32>(e, kElemFlags);
    const f64 halign = static_cast<f64>(static_cast<s32>((flags >> 14) & 3));
    const f64 valign = static_cast<f64>(static_cast<s32>((flags >> 18) & 3));
    const f64 w = lfs(size + 0), h = lfs(size + 4);
    const f64 ax = fmuls(fmuls(w, kRectAlign), halign);
    const f64 ay = fmuls(fmuls(h, kRectAlign), valign);
    stfs(a + 0, ax);
    stfs(a + 4, ay);
    stfs(b + 0, fadds(ax, w));
    stfs(b + 4, fadds(ay, h));

    call_ptr(ctx, vfunc(e, kVfnWorldMatrix), {world, e});
    Matrix_CopyXyz(m, world);

    if (static_cast<u32>(call_ptr(ctx, vfunc(e, kVfnScreenSpace), {e})) == 0) return ret(ctx, 0u);
    if (load<u32>(kScreenView) == 0) return ret(ctx, 0u);

    const f64 px_w = static_cast<f64>(static_cast<f32>(field<u16>(g::config, 4).get()));
    const f64 px_h = static_cast<f64>(static_cast<f32>(field<u16>(g::config, 6).get()));
    for (u32 k = 0; k < 4; ++k) {
        store<u32>(proj + k * 4, load<u32>(kProjectInit + k * 4));
        store<u32>(origin + k * 4, load<u32>(kProjectInit + k * 4));
    }
    call(ctx, gfn::View_ProjectPoint, {load<u32>(kScreenView), origin, m + 48});

    // Corner a: tmp = m * (screen matrix with translation (a, 0)), project its translation.
    Matrix_CopyXyz(tmp, m);
    for (u32 row = 0; row < 3; ++row)
        for (u32 k = 0; k < 3; ++k)
            store<u32>(offset + row * 16 + k * 4, load<u32>(kScreenMatrix + row * 16 + k * 4));
    store<u32>(offset + 48, load<u32>(a + 0));
    store<u32>(offset + 52, load<u32>(a + 4));
    store<u32>(offset + 56, 0);
    call(ctx, gfn::Matrix_Multiply, {tmp, offset});
    call(ctx, gfn::View_ProjectPoint, {load<u32>(kScreenView), proj, tmp + 48});
    store<u32>(a + 0, load<u32>(proj + 0));
    store<u32>(a + 4, load<u32>(proj + 4));
    stfs(a + 0, fsubs(lfs(a + 0), lfs(origin + 0)));
    stfs(a + 4, fsubs(lfs(a + 4), lfs(origin + 4)));
    stfs(a + 4, fmuls(lfs(a + 4), px_h));
    stfs(a + 0, fmuls(lfs(a + 0), px_w));

    // Corner b, the same way.
    Matrix_CopyXyz(tmp, m);
    store<u32>(offset + 48, load<u32>(b + 0));
    store<u32>(offset + 52, load<u32>(b + 4));
    store<u32>(offset + 56, 0);
    call(ctx, gfn::Matrix_Multiply, {tmp, offset});
    call(ctx, gfn::View_ProjectPoint, {load<u32>(kScreenView), proj, tmp + 48});
    store<u32>(b + 0, load<u32>(proj + 0));
    store<u32>(b + 4, load<u32>(proj + 4));
    const f64 ox = lfs(origin + 0), oy = lfs(origin + 4);
    stfs(b + 0, fsubs(lfs(b + 0), ox));
    stfs(b + 4, fsubs(lfs(b + 4), oy));
    stfs(b + 4, fmuls(lfs(b + 4), px_h));
    stfs(b + 0, fmuls(lfs(b + 0), px_w));
    stfs(m + 48, fmuls(ox, px_w));
    stfs(m + 52, fmuls(oy, px_h));
    store<u32>(m + 56, 0);
    ret(ctx, b + 4);  // what the original leaves in r3
}
