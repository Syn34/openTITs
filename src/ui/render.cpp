// Ui_Render: the UI overlay job (World_SubmitOverlay calls it).
#include "ui.h"

namespace sly4::ui {
using namespace opentit::native;

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
constexpr u32 kElemVfnRender = 156;

struct Vertex {
    be_f32 x, y;    // +0
    be_u32 z;       // +8 (bits)
    u8 rgba[4];     // +12
    be_f32 u, v;    // +16
};
static_assert(sizeof(Vertex) == 24);
struct Prims {
    gp<Vertex> cursor;  // +0
    be_u16 unk4;        // +4
    be_u16 count;       // +6
};

}  // namespace

// Draws the full-screen backdrop quad (when there is one) into the job's
// frame buffer, then lets every root element render itself (vfunc +156).
void Ui_Render(u8 enabled) {
    if (enabled == 0) return;
    u32 fb = global<be_u32>(kJobFrameBuffer);
    if (const u32 backdrop = global<be_u32>(kBackdrop)) {
        if (fb == 0) fb = global<be_u32>(kDefaultFrameBuffer);
        call(rfn::Draw_SetTexture, fb, backdrop);
        call(rfn::Draw_SetState, fb, kBackdropState);
        const u32 width = *host<be_u16>(g::config + 4), height = *host<be_u16>(g::config + 6);
        u8 color[4];
        for (u32 k = 0; k < 4; ++k) color[k] = host<u8>(kBackdropColor)[k];
        const f32 w = static_cast<f32>(static_cast<f64>(width));
        const f32 h = static_cast<f32>(static_cast<f64>(height));
        GuestFrame frame(8);
        Prims* prims = frame.as<Prims>();
        call(rfn::Draw_BeginPrims, prims, fb, 6u, 4u, height);
        u32 v = guest(prims->cursor.get());
        const u32 count = prims->count;
        const be_f32* uv = host<be_f32>(kQuadUv);
        for (u32 k = 0; k < 4; ++k, v += sizeof(Vertex)) {
            Vertex& vx = gref<Vertex>(v);
            const f32 u = uv[2 * k], t = uv[2 * k + 1];
            vx.u = u;
            vx.v = t;
            const f32 x = static_cast<f32>(static_cast<f64>(u) * w);
            const f32 y = static_cast<f32>(static_cast<f64>(t) * h);
            vx.z = kQuadDepthBits;
            vx.x = x;
            vx.y = y;
            for (u32 c = 0; c < 4; ++c) vx.rgba[c] = color[c];
        }
        prims->count = static_cast<u16>(count + 4);
        prims->cursor = host<Vertex>(v);
        call(rfn::Draw_EndPrims, fb, count + 4, v);
    }
    for (Element* e = global<gp<Element>>(kUiRoots); e; e = e->next) vcall(e, kElemVfnRender);
}

}  // namespace sly4::ui

NATIVE(004D4E28, sly4::ui::Ui_Render)
