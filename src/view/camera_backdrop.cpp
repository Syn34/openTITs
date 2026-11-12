// View_DrawBackdrop: the vertical colour gradient drawn behind a view.
#include "camera.h"

namespace sly4::view {
using namespace opentit::native;

namespace {
constexpr u32 kOne = 0x004583F4;        // f32 1.0
constexpr u32 kMinusOne = 0xBF800000;   // f32 -1.0
constexpr u32 kOneBits = 0x3F800000;    // f32 1.0
constexpr u32 kFarZ = 0x3F7FF972;       // f32 0.99999 (just inside the far plane)
constexpr u32 kViewProj = 880;          // View +880 f32[16]
constexpr s32 kBackdropShaders = 3;     // Shader_LoadGroup group
constexpr u32 kPrimType = 7, kVertexCount = 4, kVertexSize = 24;

struct Prims {
    be_u32 cursor;  // +0 vertex write pointer
    be_u16 unk4;    // +4
    be_u16 count;   // +6
};
}  // namespace

// The four NDC corners (+-1, +-1, 0.99999, 1) taken back to world space
// through the inverse of the view's +880 matrix (with the divide by w),
// drawn as one 4-vertex primitive (type 7) into the current frame buffer
// with shader group 3 and the identity state matrix: the y = -1 corners
// in `bottom`, the y = +1 ones in `top`. w is unused.
void View_DrawBackdrop(void* /*world*/, void* view, const u8* top, const u8* bottom) {
    u8 col_top[4], col_bottom[4];
    for (u32 i = 0; i < 4; ++i) {
        col_top[i] = top[i];
        col_bottom[i] = bottom[i];
    }
    // The original's 416-byte frame: inverse at +112, its scratch (the
    // one used) at +176, the corners at +240, the Prims at +304.
    GuestFrame frame(416 - 112);
    be_f32* out = frame.as<be_f32>(0);
    be_f32* inverse = frame.as<be_f32>(64);
    be_f32* corners = frame.as<be_f32>(128);
    Prims* prims = frame.as<Prims>(192);
    Mat4_Invert(out, inverse, host<be_f32>(guest(view) + kViewProj));
    static constexpr u32 init[16] = {kMinusOne, kMinusOne, kFarZ, kOneBits,  // bottom left
                                     kOneBits,  kMinusOne, kFarZ, kOneBits,  // bottom right
                                     kOneBits,  kOneBits,  kFarZ, kOneBits,  // top right
                                     kMinusOne, kOneBits,  kFarZ, kOneBits}; // top left
    be_u32* cw = reinterpret_cast<be_u32*>(corners);
    for (u32 i = 0; i < 16; ++i) cw[i] = init[i];
    const f64 one = global<be_f32>(kOne);
    for (u32 i = 0; i < 4; ++i) {
        be_f32* c = corners + 4 * i;
        Vec4_Transform(c, inverse);
        const f64 k = fdivs(one, c[3]);
        const f64 x = c[0], y = c[1], z = c[2], w = c[3];
        c[0] = static_cast<f32>(fmuls(x, k));
        c[1] = static_cast<f32>(fmuls(y, k));
        c[2] = static_cast<f32>(fmuls(z, k));
        c[3] = static_cast<f32>(fmuls(w, k));
    }
    const u32 group = call<u32>(cfn::Shader_LoadGroup, kBackdropShaders);
    const u32 world = global<be_u32>(cg::current_world);
    const u32 fb = gref<be_u32>(gref<be_u32>(world + cg::world_frame_ring) + 12);
    call(cfn::Draw_SetTexture, fb, group);
    call(cfn::Draw_SetState, fb, cg::identity);
    // Draw_BeginPrims ors r7 into its command: r7 is left as Draw_SetState
    // returns it, as in the original (nothing runs on the PPU in between).
    call(cfn::Draw_BeginPrims, prims, fb, kPrimType, kVertexCount);
    const u32 v = prims->cursor;
    const u16 count = prims->count;
    for (u32 i = 0; i < 4; ++i) {
        const u32 vert = v + kVertexSize * i;
        const be_u32* c = reinterpret_cast<const be_u32*>(corners + 4 * i);
        for (u32 j = 0; j < 3; ++j) gref<be_u32>(vert + 4 * j) = c[j].get();  // lfs / stfs pairs
        const u8* col = i < 2 ? col_bottom : col_top;
        for (u32 b = 0; b < 4; ++b) gref<u8>(vert + 12 + b) = col[b];
    }
    prims->cursor = v + kVertexCount * kVertexSize;
    prims->count = static_cast<u16>(count + kVertexCount);
    call(cfn::Draw_EndPrims, fb);
}

}  // namespace sly4::view

NATIVE(004583F8, sly4::view::View_DrawBackdrop)
