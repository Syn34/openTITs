// Element scale, rectangle and screen projection (callees of the text code).
#include "glyph.h"

#include "../view/view.h"

namespace sly4::text {
using namespace opentit::native;

namespace {
constexpr f32 kRectAlign = -0.5f;  // 0x004E0644
constexpr f32 kNdcOne = 1.0f;      // 0x00522744
constexpr f32 kNdcHalf = 0.5f;     // 0x00522748
constexpr f32 kNdcHalfUp = -0.5f;  // 0x0052274C

inline void copy_word(be_f32& dst, const be_f32& src) {
    reinterpret_cast<be_u32&>(dst) = reinterpret_cast<const be_u32&>(src).get();
}
}  // namespace

// [vfunc +168, sret] The element's scale, times its parent's (recursively)
// when it inherits.
void UiElem_GetScale(Vec3f* out, UiElem* e) {
    copy_word(out->x, e->scale[0]);
    copy_word(out->y, e->scale[1]);
    copy_word(out->z, e->scale[2]);
    if ((e->flags & kElemInheritScale) == 0) return;
    UiElem* parent = e->parent;
    if (!parent) return;
    GuestFrame frame(16);
    Vec3f* ps = frame.as<Vec3f>();
    call_ptr(vslot(parent, kVfnGetScale), ps, parent);
    out->x = f32_mul(out->x, ps->x);
    out->y = f32_mul(out->y, ps->y);
    out->z = f32_mul(out->z, ps->z);
}

// Transforms `in` by the view's matrix and maps x/w and y/w from [-1, 1]
// to [0, 1] (y pointing down).
void View_ProjectPoint(void* view, be_f32* out, const be_f32* in) {
    call(gfn::View_Update, view, 0u);
    view::Vec_TransformByMatrix(out, in, host<u8>(guest(view) + kViewProjMatrix));
    out[0] = f32_mul(f32_add(f32_div(out[0], out[3]), kNdcOne), kNdcHalf);
    out[1] = f32_mul(f32_sub(f32_div(out[1], out[3]), kNdcOne), kNdcHalfUp);
}

// [vfunc +160] a and b: the element's corners (its size offset by
// -0.5 * size * align), m: its world matrix. For a screen-space element
// (flag 0x20) while a 3D view is set, both corners are pushed through the
// world matrix and projected, made relative to the projected origin and
// scaled to pixels (config +4 / +6); m's translation then becomes the
// origin in pixels.
void UiElem_GetRect(UiElem* e, be_f32* a, be_f32* b, be_f32* m) {
    // The original's frame: the projection outputs at +112 / +128, the
    // world matrix at +144, two scratch matrices at +208 / +272, the size at +336.
    constexpr u32 kFrameBase = 112;
    GuestFrame frame(344 - kFrameBase);
    be_f32* proj = frame.as<be_f32>(112 - kFrameBase);
    be_f32* origin = frame.as<be_f32>(128 - kFrameBase);
    be_f32* world = frame.as<be_f32>(144 - kFrameBase);
    be_f32* tmp = frame.as<be_f32>(208 - kFrameBase);
    be_f32* offset = frame.as<be_f32>(272 - kFrameBase);
    be_f32* size = frame.as<be_f32>(336 - kFrameBase);

    call_ptr(vslot(e, kVfnSize), size, e);
    const u32 flags = e->flags;
    const f32 halign = static_cast<f32>(static_cast<s32>((flags >> 14) & 3));
    const f32 valign = static_cast<f32>(static_cast<s32>((flags >> 18) & 3));
    const f32 w = size[0], h = size[1];
    const f32 ax = f32_mul(f32_mul(w, kRectAlign), halign);
    const f32 ay = f32_mul(f32_mul(h, kRectAlign), valign);
    a[0] = ax;
    a[1] = ay;
    b[0] = f32_add(ax, w);
    b[1] = f32_add(ay, h);

    call_ptr(vslot(e, kVfnWorldMatrix), world, e);
    Matrix_CopyXyz(m, world);

    if (vcall<u32>(e, kVfnScreenSpace) == 0) return;
    void* view = host<void>(global<be_u32>(kScreenView));
    if (!view) return;

    const f32 px_w = static_cast<f32>(gref<be_u16>(g::config + 4).get());
    const f32 px_h = static_cast<f32>(gref<be_u16>(g::config + 6).get());
    const be_u32* init = host<be_u32>(kProjectInit);
    for (u32 k = 0; k < 4; ++k) {
        reinterpret_cast<be_u32*>(proj)[k] = init[k].get();
        reinterpret_cast<be_u32*>(origin)[k] = init[k].get();
    }
    View_ProjectPoint(view, origin, m + 12);

    // Corner a: tmp = m * (screen matrix with translation (a, 0)), project its translation.
    const be_f32* screen = host<be_f32>(kScreenMatrix);
    Matrix_CopyXyz(tmp, m);
    for (u32 row = 0; row < 3; ++row)
        for (u32 k = 0; k < 3; ++k) copy_word(offset[row * 4 + k], screen[row * 4 + k]);
    copy_word(offset[12], a[0]);
    copy_word(offset[13], a[1]);
    offset[14] = 0.0f;
    call(gfn::Matrix_Multiply, tmp, offset);
    View_ProjectPoint(host<void>(global<be_u32>(kScreenView)), proj, tmp + 12);
    copy_word(a[0], proj[0]);
    copy_word(a[1], proj[1]);
    a[0] = f32_sub(a[0], origin[0]);
    a[1] = f32_sub(a[1], origin[1]);
    a[1] = f32_mul(a[1], px_h);
    a[0] = f32_mul(a[0], px_w);

    // Corner b, the same way.
    Matrix_CopyXyz(tmp, m);
    copy_word(offset[12], b[0]);
    copy_word(offset[13], b[1]);
    offset[14] = 0.0f;
    call(gfn::Matrix_Multiply, tmp, offset);
    View_ProjectPoint(host<void>(global<be_u32>(kScreenView)), proj, tmp + 12);
    copy_word(b[0], proj[0]);
    copy_word(b[1], proj[1]);
    const f32 ox = origin[0], oy = origin[1];
    b[0] = f32_sub(b[0], ox);
    b[1] = f32_sub(b[1], oy);
    b[1] = f32_mul(b[1], px_h);
    b[0] = f32_mul(b[0], px_w);
    m[12] = f32_mul(ox, px_w);
    m[13] = f32_mul(oy, px_h);
    m[14] = 0.0f;
}

}  // namespace sly4::text

NATIVE(004D7E70, sly4::text::UiElem_GetScale)
NATIVE(00522750, sly4::text::View_ProjectPoint)
NATIVE(004E0648, sly4::text::UiElem_GetRect)
