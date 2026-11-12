// View_BeginFrame (0x00521D20): the per-frame set-up before the render
// layers are filled for a view (called by 0x00548508).
#include <atomic>

#include "shader4.h"
#include "rsx.h"

namespace sly4::gfx {

namespace {

namespace s3 = sly4::render::shader3;

constexpr u32 View_Update = 0x005115F0;             // (View*, u32) (text/glyph.h)
constexpr u32 View_Sub5207D0 = 0x005207D0;          // (View*) (unknown)
constexpr u32 FrameArena_Align = 0x0055329C;        // (Arena*) (game/update.h)
constexpr u32 FrameBuffer_BeginLayer = 0x00521A5C;  // (fb, View*, u8 layer) (world/world.h)
constexpr u32 PtrVector_Construct = 0x005516E0;     // (PtrVector*)

constexpr u32 kK = 0x00521D14;  // .text: f32 1/255, 1.0, 0.0
constexpr u32 kViewSlot5 = 20;  // View vtable slot 5 (byte offset 20)
constexpr u32 kViewFrame = 668;  // u32 frame counter (scene and view)
constexpr u32 kViewFogLayer = 51;  // u8: the layer bit tested against kLayerMask
constexpr u32 kCurrentSurface = 0x010DDA74;  // u32: object whose +672 is the current View
constexpr u32 kDrawCount = 0x010DDA78;  // u32 (current_surface +4): ++ per call (name?)
constexpr u32 kHeap = 0x010DD600 + 280;  // layers3::g::block_dd600 +280: LinearHeap*
constexpr u32 kFlagsF8A0 = 0x010DF8A0;   // u16, cleared (name?)
constexpr u32 kLayers = 0x010DDA8C;      // {u32 count; RenderLayer* layer[]} (render::g::layers)
constexpr u32 kStateCache = 0x010DDBB4;  // u32 (render::g::state_cache)
constexpr u32 kLayerMask = 0x010DDA70;   // layers3::g::byte_da70
constexpr u32 kFogScales = 0x010DD5B8;   // f32[] by view +51 (layers3::g::block_dd5a8 +16)
constexpr u32 kFogScale = 0x010DD5B4;    // f32 (block_dd5a8 +12)
constexpr u32 kOnce = 0x011DE480;        // u8: kList set up (name?)
constexpr u32 kList = 0x011DE300;        // u8 flag, +4 PtrVector, +10 u16, +12 u16, +24 u32*, +32 u32, +128 / +256 atomics (name?)
constexpr u32 kCleared[] = {0x010ECE30, 0x010F0A3C, 0x010EE234, 0x010EF638, 0x00F66FD8};  // u32s cleared every frame (name?)
constexpr u32 kClearedByte = 0x010F0A40;  // u8, cleared (name?)

// The config words this reads (g::config +44 requested layer count, +48
// f32 / +52 u32 that count clamped to 63).
constexpr u32 kCfgLayers = 44, kCfgLayerF = 48, kCfgLayerN = 52;

// A view fog record (64 bytes from view +128).
struct ViewFog {
    u8 enabled;          // +0
    pad<4> _1;
    u8 density;          // +5
    pad<2> _6;
    be_u32 start;        // +8: f32
    be_f32 end;          // +12
    pad<16> _16;
    be_u32 vconst[2][4];  // +32: {start, 1 / (end - start), 0, 0}, then w = density / 255
};
static_assert(sizeof(ViewFog) == 64 && offsetof(ViewFog, vconst) == 32);

template <typename T>
T* ga(u32 ea) { return at<T>(nullptr, ea); }  // guest address ea, never null

// The lwarx / stwcx. loop in the original: an atomic store of 0.
void atomic_clear(u32 ea) { std::atomic_ref<u32>(*reinterpret_cast<u32*>(mem::ptr(ea))).store(0); }

}  // namespace

// void View_BeginFrame(Scene* scene)
// view = scene +672:
// - current_surface = view, its draw count ++, View_Update(view, 1);
// - per fog record (2, 64 bytes from view +128: +0 u8 enabled, +5 u8
//   density, +8 start, +12 end) its vertex constants {start, 1 / (end -
//   start), 0, 0} at +32 and the same with w = density / 255 at +48 (1.0
//   and 0.0 when the fog is off);
// - 0x005207D0(view), view vtable slot 5, FrameArena_Align of the frame
//   heap, RenderLayer_End, every render layer's Reset (slot 0);
// - g::config +52 / +48 = min(+44, 63) as u32 / f32;
// - FrameBuffer_BeginLayer(render::g::state_cache, current_surface +672, 255);
// - a list at 0x011DE300 constructed once and emptied;
// - the frame's counters cleared, the fog scale (0x010DD5B4) = the view's
//   layer scale, times 1 when its layer bit is set in byte_da70 (else 0);
// - without a scene counter (+668) the view's 4 x 3 matrix at +816 (rows
//   16 bytes apart) copied to +1200; then the view's counter ++.
void View_BeginFrame(u8* scene) {
    const u32 view = *at<be_u32>(scene, 672);
    global<be_u32>(kCurrentSurface) = view;
    global<be_u32>(kDrawCount) = global<be_u32>(kDrawCount) + 1;
    call(View_Update, view, 1u);

    const f32 k255 = global<be_f32>(kK);
    const be_u32& one = global<be_u32>(kK + 4);
    for (u32 i = 0; i < 2; ++i) {
        ViewFog* rec = ga<ViewFog>(view + s3::kViewFogs + ((i << 6) & 0x3FC0u));  // rlwinm 6,18,25
        const f32 density = static_cast<f32>(rec->density) * k255;  // fcfid, frsp, fmuls
        const u32 start_w = rec->start;
        const f32 start = as_f32(start_w), end = rec->end;
        const bool on = rec->enabled != 0;
        const f32 inv = as_f32(one) / (end - start);  // fsubs, fdivs
        be_u32* v = &rec->vconst[0][0];
        v[0] = start_w;  // lfs / stfs pairs
        if (on) v[1] = bits(inv); else copy_word(v + 1, &one);
        v[2] = 0u;
        v[3] = 0u;
        v[4] = start_w;
        if (on) v[5] = bits(inv); else copy_word(v + 5, &one);
        v[6] = 0u;
        if (on) v[7] = bits(density); else copy_word(v + 7, host<u8>(kK + 8));
    }

    call(View_Sub5207D0, view);
    vcall(ga<u8>(view), kViewSlot5);
    call(FrameArena_Align, global<be_u32>(kHeap).get());
    global<be_u16>(kFlagsF8A0) = 0;
    RenderLayer_End();
    for (u32 n = 0; n < global<be_u32>(kLayers); ++n) {  // cmplw
        RenderLayer* layer = global<gp<RenderLayer>>(kLayers + 4 + 4 * n);
        vcall(or_zero_page(layer), 0);
    }

    u8* cfg = reinterpret_cast<u8*>(&config());
    const u32 want = *at<be_u32>(cfg, kCfgLayers);
    const u32 n = want < 63 ? want : 63;  // cmplwi
    *at<be_u32>(cfg, kCfgLayerN) = n;
    *at<be_f32>(cfg, kCfgLayerF) = static_cast<f32>(n);  // fcfid, frsp
    call(FrameBuffer_BeginLayer, global<be_u32>(kStateCache).get(), ga<be_u32>(global<be_u32>(kCurrentSurface) + 672)->get(), 255u);

    if (global<u8>(kOnce) == 0) {
        global<u8>(kList) = 1;
        call(PtrVector_Construct, kList + 4);
        global<be_u32>(kList + 32) = 0u;
        global<be_u16>(kList + 10) = 160;
        atomic_clear(kList + 128);
        atomic_clear(kList + 256);
        global<u8>(kOnce) = 1;
    }
    global<be_u16>(kList + 12) = 0;
    *ga<be_u16>(global<be_u32>(kList + 24) + 8) = 0;
    atomic_clear(kList + 128);

    const u32 layer = *ga<u8>(view + kViewFogLayer);
    const u32 bit = (layer & 0x20) != 0 ? 0u : (1u << (layer & 31));  // slw
    const f32 on = ((bit & 0xFF) & global<u8>(kLayerMask)) != 0 ? global<be_f32>(kK + 4) : global<be_f32>(kK + 8);
    const f32 scale = global<be_f32>(kFogScales + 4 * layer) * on;  // lfsx, fmuls
    for (u32 i = 0; i < 4; ++i) global<be_u32>(kCleared[i]) = 0u;
    const u32 scene_frame = *at<be_u32>(scene, kViewFrame);
    global<be_u32>(kCleared[4]) = 0u;
    global<be_f32>(kFogScale) = scale;
    global<u8>(kClearedByte) = 0;
    const u32 frame = *ga<be_u32>(view + kViewFrame);
    if (scene_frame == 0) {
        for (u32 r = 0; r < 4; ++r) copy3(ga<u8>(view + 1200 + 16 * r), ga<u8>(view + 816 + 16 * r));
    }
    *ga<be_u32>(view + kViewFrame) = frame + 1;
}

}  // namespace sly4::gfx

NATIVE(00521D20, sly4::gfx::View_BeginFrame)
