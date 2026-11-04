// Ui_Render: the UI overlay job (World_SubmitOverlay calls it).
#include "ui.h"

using namespace sly4;
using namespace sly4::ui;

namespace {

namespace rfn {
constexpr u32 Draw_SetTexture = 0x00523310;   // (fb, texture) (name?)
constexpr u32 Draw_SetState = 0x005218D8;     // (fb, const State*) (name?)
constexpr u32 Draw_BeginPrims = 0x005236A0;   // (Prims* out, fb, 6, 4, height) (name?)
constexpr u32 Draw_EndPrims = 0x00523748;     // empty in retail
}  // namespace rfn

constexpr u32 kJobFrameBuffer = 0x00F66890;   // u32 (see world.h)
constexpr u32 kDefaultFrameBuffer = 0x010DDBB4;
constexpr u32 kBackdrop = 0x00F66854;         // u32 texture drawn full screen, or 0
constexpr u32 kBackdropState = 0x010C29E0;
constexpr u32 kBackdropColor = 0x010C2AD4;    // u8[4]
constexpr u32 kQuadUv = 0x00AEF460;           // f32[4][2]: (1,0) (0,0) (1,1) (0,1)
constexpr u32 kQuadDepthBits = 0xC1A00000u;   // -20.0f
constexpr u32 kVertexSize = 24;               // { f32 x, y, z; u8 rgba[4]; f32 u, v }
constexpr u32 kElemVfnRender = 156;

}  // namespace

// void Ui_Render(bool enabled, -)  (name?)
// Draws the full-screen backdrop quad (when there is one) into the job's
// frame buffer, then lets every root element render itself (vfunc +156).
DECOMP(004D4E28, Ui_Render) {
    if ((arg<u32>(ctx, 0) & 0xFF) == 0) return;
    u32 fb = load<u32>(kJobFrameBuffer);
    const u32 backdrop = load<u32>(kBackdrop);
    if (backdrop != 0) {
        if (fb == 0) fb = load<u32>(kDefaultFrameBuffer);
        call(ctx, rfn::Draw_SetTexture, {fb, backdrop});
        call(ctx, rfn::Draw_SetState, {fb, kBackdropState});
        const u32 width = field<u16>(g::config, 4), height = field<u16>(g::config, 6);
        u8 color[4];
        for (u32 k = 0; k < 4; ++k) color[k] = ref<u8>(kBackdropColor + k);
        const f32 w = static_cast<f32>(static_cast<f64>(width));
        const f32 h = static_cast<f32>(static_cast<f64>(height));

        Stack frame(ctx, 8);
        const u32 prims = frame.addr();
        call(ctx, rfn::Draw_BeginPrims, {prims, fb, 6, 4, height});
        u32 v = load<u32>(prims);
        const u32 count = load<u16>(prims + 6);
        for (u32 k = 0; k < 4; ++k, v += kVertexSize) {
            const f32 u = load<f32>(kQuadUv + k * 8), t = load<f32>(kQuadUv + k * 8 + 4);
            store<f32>(v + 16, u);
            store<f32>(v + 20, t);
            const f32 x = static_cast<f32>(static_cast<f64>(u) * w);
            const f32 y = static_cast<f32>(static_cast<f64>(t) * h);
            store<u32>(v + 8, kQuadDepthBits);
            store<f32>(v + 0, x);
            store<f32>(v + 4, y);
            for (u32 c = 0; c < 4; ++c) ref<u8>(v + 12 + c) = color[c];
        }
        store<u16>(prims + 6, static_cast<u16>(count + 4));
        store<u32>(prims, v);
        call(ctx, rfn::Draw_EndPrims, {fb, count + 4, v});
    }
    for (u32 e = load<u32>(kUiRoots); e != 0; e = load<u32>(e + kElemNext))
        call_ptr(ctx, vfunc(e, kElemVfnRender), {e});
}
