// View_DrawBackdrop: the vertical colour gradient drawn behind a view.
#include "camera.h"

using namespace sly4;
using namespace sly4::view;

namespace {

constexpr u32 kOne = 0x004583F4;        // f32 1.0
constexpr u32 kMinusOne = 0xBF800000;   // f32 -1.0
constexpr u32 kOneBits = 0x3F800000;    // f32 1.0
constexpr u32 kFarZ = 0x3F7FF972;       // f32 0.99999 (just inside the far plane)
constexpr u32 kViewProj = 880;          // View +880 f32[16]
constexpr s32 kBackdropShaders = 3;     // Shader_LoadGroup group
constexpr u32 kPrimType = 7, kVertexCount = 4, kVertexSize = 24;

}  // namespace

// void View_DrawBackdrop(World* w, View* view, const u8 top[4], const u8 bottom[4])   (name?)
// The four NDC corners (+-1, +-1, 0.99999, 1) taken back to world space
// through the inverse of the view's +880 matrix (with the divide by w),
// drawn as one 4-vertex primitive (type 7) into the current frame buffer
// with shader group 3 and the identity state matrix: the y = -1 corners
// in `bottom`, the y = +1 ones in `top`. w is unused.
DECOMP(004583F8, View_DrawBackdrop) {
    const u32 view = arg<u32>(ctx, 1), top = arg<u32>(ctx, 2), bottom = arg<u32>(ctx, 3);
    u8 col_top[4], col_bottom[4];
    for (u32 i = 0; i < 4; ++i) {
        col_top[i] = ref<u8>(top + i);
        col_bottom[i] = ref<u8>(bottom + i);
    }
    // The original's 416-byte frame: inverse at +112, its scratch (the
    // one used) at +176, the corners at +240, the Prims at +304.
    Stack frame(ctx, 416 - 112);
    const u32 sp = frame.addr() - 112;
    const u32 inverse = sp + 176, corners = sp + 240, prims = sp + 304;

    call(ctx, cfn::Mat4_Invert, {sp + 112, inverse, view + kViewProj});
    const u32 init[16] = {kMinusOne, kMinusOne, kFarZ, kOneBits,  // bottom left
                          kOneBits,  kMinusOne, kFarZ, kOneBits,  // bottom right
                          kOneBits,  kOneBits,  kFarZ, kOneBits,  // top right
                          kMinusOne, kOneBits,  kFarZ, kOneBits}; // top left
    for (u32 i = 0; i < 16; ++i) store<u32>(corners + 4 * i, init[i]);
    const f64 one = load<f32>(kOne);
    for (u32 i = 0; i < 4; ++i) {
        const u32 c = corners + 16 * i;
        call(ctx, cfn::Vec4_Transform, {c, inverse});
        const f64 k = fdivs(one, load<f32>(c + 12));
        const f64 x = load<f32>(c), y = load<f32>(c + 4), z = load<f32>(c + 8), w = load<f32>(c + 12);
        store<f32>(c, static_cast<f32>(fmuls(x, k)));
        store<f32>(c + 4, static_cast<f32>(fmuls(y, k)));
        store<f32>(c + 8, static_cast<f32>(fmuls(z, k)));
        store<f32>(c + 12, static_cast<f32>(fmuls(w, k)));
    }

    const u32 group = static_cast<u32>(call(ctx, cfn::Shader_LoadGroup, {static_cast<u64>(kBackdropShaders)}));
    const u32 fb = load<u32>(load<u32>(load<u32>(cg::current_world) + cg::world_frame_ring) + 12);
    call(ctx, cfn::Draw_SetTexture, {fb, group});
    call(ctx, cfn::Draw_SetState, {fb, cg::identity});
    // Draw_BeginPrims ors r7 into its command: the original leaves the r7
    // Draw_SetState returns with, as this call does.
    call(ctx, cfn::Draw_BeginPrims, {prims, fb, kPrimType, kVertexCount});

    const u32 v = load<u32>(prims);
    const u16 count = load<u16>(prims + 6);
    for (u32 i = 0; i < 4; ++i) {
        const u32 vert = v + kVertexSize * i, c = corners + 16 * i;
        for (u32 j = 0; j < 3; ++j) store<u32>(vert + 4 * j, load<u32>(c + 4 * j));  // lfs / stfs pairs
        const u8* col = i < 2 ? col_bottom : col_top;
        for (u32 b = 0; b < 4; ++b) ref<u8>(vert + 12 + b) = col[b];
    }
    store<u32>(prims, v + kVertexCount * kVertexSize);
    store<u16>(prims + 6, static_cast<u16>(count + kVertexCount));
    call(ctx, cfn::Draw_EndPrims, {fb});
}
