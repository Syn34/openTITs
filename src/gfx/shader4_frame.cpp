// View_BeginFrame (0x00521D20): the per-frame set-up before the render
// layers are filled for a view (called by 0x00548508).
#include <atomic>

#include "shader4.h"

using namespace sly4;

namespace {

namespace s3 = sly4::render::shader3;
namespace sp = sly4::render::pool;

namespace ffn {
constexpr u32 View_Update = 0x005115F0;             // (View*, u32) (text/glyph.h)
constexpr u32 View_Sub5207D0 = 0x005207D0;          // (View*) (unknown)
constexpr u32 FrameArena_Align = 0x0055329C;        // (Arena*) (game/update.h)
constexpr u32 FrameBuffer_BeginLayer = 0x00521A5C;  // (fb, View*, u8 layer) (world/world.h)
constexpr u32 PtrVector_Construct = s3::types::fn::PtrVector_Construct;
}  // namespace ffn

constexpr u32 kK = 0x00521D14;  // .text: f32 1/255, 1.0, 0.0
constexpr u32 kViewSlot5 = 20;  // View vtable slot 5 (byte offset 20)
constexpr u32 kViewFrame = 668;  // u32 frame counter (scene and view)
constexpr u32 kViewFogLayer = 51;  // u8: the layer bit tested against layers3::g::byte_da70
constexpr u32 kDrawCount = 0x010DDA78;  // u32 (draw::g::current_surface +4): ++ per call (name?)
constexpr u32 kHeap = 0x010DD600 + 280;  // layers3::g::block_dd600 +280: LinearHeap*
constexpr u32 kFlagsF8A0 = 0x010DF8A0;   // u16, cleared (name?)
constexpr u32 kCfgLayers = 44, kCfgLayerF = 48, kCfgLayerN = 52;  // g::config: requested count, f32 / u32 clamped to 63
constexpr u32 kLayerMask = 0x010DDA70;   // layers3::g::byte_da70
constexpr u32 kFogScales = 0x010DD5B8;   // f32[] by view +51 (layers3::g::block_dd5a8 +16)
constexpr u32 kFogScale = 0x010DD5B4;    // f32 (block_dd5a8 +12)
constexpr u32 kOnce = 0x011DE480;        // u8: kList set up (name?)
constexpr u32 kList = 0x011DE300;        // u8 flag, +4 PtrVector, +10 u16, +12 u16, +24 u32*, +32 u32, +128 / +256 atomics (name?)
constexpr u32 kCleared[] = {0x010ECE30, 0x010F0A3C, 0x010EE234, 0x010EF638, 0x00F66FD8};  // u32s cleared every frame (name?)
constexpr u32 kClearedByte = 0x010F0A40;  // u8, cleared (name?)

// The lwarx / stwcx. loop in the original: an atomic store of 0.
void atomic_clear(u32 ea) { std::atomic_ref<u32>(*reinterpret_cast<u32*>(mem::ptr(ea))).store(0); }

}  // namespace

// u32 View_BeginFrame(Scene* scene)
// view = scene +672:
// - draw::g::current_surface = view, its draw count ++, View_Update(view, 1);
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
//   16 bytes apart) copied to +1200; then the view's counter ++, returned.
DECOMP(00521D20, View_BeginFrame) {
    const u32 scene = arg<u32>(ctx, 0);
    const u32 view = load<u32>(scene + 672);
    store<u32>(draw::g::current_surface, view);
    store<u32>(kDrawCount, load<u32>(kDrawCount) + 1);
    call(ctx, ffn::View_Update, {view, 1});

    const f64 k255 = sp::lfs(kK);
    for (u32 i = 0; i < 2; ++i) {
        const u32 rec = view + s3::kViewFogs + ((i << 6) & 0x3FC0u);  // rlwinm 6,18,25
        const f64 density = sp::sgl(sp::sgl(static_cast<f64>(ref<u8>(rec + 5))) * k255);  // fcfid, frsp, fmuls
        const u32 start_w = load<u32>(rec + 8);
        const f64 start = sp::lfs(rec + 8), end = sp::lfs(rec + 12);
        const bool on = ref<u8>(rec) != 0;
        const f64 inv = sp::sgl(sp::lfs(kK + 4) / sp::sgl(end - start));  // fsubs, fdivs
        store<u32>(rec + 32, start_w);  // lfs / stfs pairs
        if (on) sp::stfs(rec + 36, inv); else sp::copy_word(rec + 36, kK + 4);
        store<u32>(rec + 40, 0);
        store<u32>(rec + 44, 0);
        store<u32>(rec + 48, start_w);
        if (on) sp::stfs(rec + 52, inv); else sp::copy_word(rec + 52, kK + 4);
        store<u32>(rec + 56, 0);
        if (on) sp::stfs(rec + 60, density); else sp::copy_word(rec + 60, kK + 8);
    }

    call(ctx, ffn::View_Sub5207D0, {view});
    fx::vcall(ctx, view, kViewSlot5, {view});
    call(ctx, ffn::FrameArena_Align, {load<u32>(kHeap)});
    store<u16>(kFlagsF8A0, 0);
    call(ctx, render::layer::RenderLayer_End);
    for (u32 n = 0; n < load<u32>(render::g::layers); ++n) {  // cmplw
        const u32 layer = load<u32>(render::g::layers + 4 + 4 * n);
        fx::vcall(ctx, layer, 0, {layer});
    }

    constexpr u32 cfg = sly4::g::config;
    const u32 want = load<u32>(cfg + kCfgLayers);
    const u32 n = want < 63 ? want : 63;  // cmplwi
    store<u32>(cfg + kCfgLayerN, n);
    sp::stfs(cfg + kCfgLayerF, sp::sgl(static_cast<f64>(n)));  // fcfid, frsp
    call(ctx, ffn::FrameBuffer_BeginLayer, {load<u32>(render::g::state_cache), load<u32>(load<u32>(draw::g::current_surface) + 672), 255});

    if (ref<u8>(kOnce) == 0) {
        store<u8>(kList, 1);
        call(ctx, ffn::PtrVector_Construct, {kList + 4});
        store<u32>(kList + 32, 0);
        store<u16>(kList + 10, 160);
        atomic_clear(kList + 128);
        atomic_clear(kList + 256);
        store<u8>(kOnce, 1);
    }
    store<u16>(kList + 12, 0);
    store<u16>(load<u32>(kList + 24) + 8, 0);
    atomic_clear(kList + 128);

    const u32 layer = ref<u8>(view + kViewFogLayer);
    const u32 bit = (layer & 0x20) != 0 ? 0u : (1u << (layer & 31));  // slw
    const f64 on = ((bit & 0xFF) & ref<u8>(kLayerMask)) != 0 ? sp::lfs(kK + 4) : sp::lfs(kK + 8);
    const f64 scale = sp::sgl(sp::lfs(kFogScales + 4 * layer) * on);  // lfsx, fmuls
    for (u32 i = 0; i < 4; ++i) store<u32>(kCleared[i], 0);
    const u32 scene_frame = load<u32>(scene + kViewFrame);
    store<u32>(kCleared[4], 0);
    sp::stfs(kFogScale, scale);
    store<u8>(kClearedByte, 0);
    const u32 frame = load<u32>(view + kViewFrame);
    if (scene_frame == 0) {
        for (u32 r = 0; r < 4; ++r) {
            for (u32 i = 0; i < 12; i += 4) sp::copy_word(view + 1200 + 16 * r + i, view + 816 + 16 * r + i);
        }
    }
    store<u32>(view + kViewFrame, frame + 1);
    ret(ctx, frame + 1);
}
