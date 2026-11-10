// The view frame driver (gfx24): Render_DrawFrame, Call_0051A6A0's path
// when the owner's u8 +48 is clear.
#include <bit>

#include "fx.h"
#include "gfx10.h"
#include "gfx24.h"

using namespace sly4;

namespace {

namespace fr = sly4::render::gfx24::frame;
using fx::emit;

// The original's 384-byte frame offsets.
constexpr u32 kFrame = 384;
constexpr u32 kBloomArgs = 112;  // 28 bytes: f32[3] scene +32, -, scene +48, bloom texture, bloom target
constexpr u32 kPassState = 144;  // {0, 0, -, -, f32[16]}: +8..+15 unwritten, as the original
// CmdBuf_ReserveCall outputs, one per call site.
constexpr u32 kResStream0 = 308, kResOverlayEarly = 296, kResSorted = 284, kResOverlayLate = 272, kResLists3 = 224,
              kResLists1 = 260, kResLists2 = 248, kResPass = 236;

// GPU marker `id` with GetGlobal_00533778's value (the context read first).
void Mark(PPUContext& ctx, u32 id) {
    const u32 c = load<u32>(fr::kGcmContext);
    const u32 v = static_cast<u32>(call(ctx, fr::GetGlobal));
    call(ctx, fr::Gcm_Marker, {c, id, v});
}

void DrawLayer(PPUContext& ctx, u32 k) {  // RenderLayer vtable +16
    const u32 layer = load<u32>(fr::kLayers + 4 * k);
    fx::vcall(ctx, layer, 16, {layer});
}

// The polygon offset (kConsts' 0) is 0, so the original's offset / depth
// format scaling paths are dead: only polygon offset fill off is sent.
void PolygonOffsetOff() { emit({fr::kPolygonOffsetFill, 0}); }

// Reserves a CALL slot for deferred segment `seg` (its three words to
// seg +0..8, from the stack output at `out`), then invalidates the state.
void Record(PPUContext& ctx, u32 seg, u32 out) {
    call(ctx, fr::CmdBuf_ReserveCall, {out});
    for (u32 k = 0; k < 3; ++k) store<u32>(seg + 4 * k, load<u32>(out + 4 * k));
    call(ctx, fr::Draw_InvalidateState);
    store<u32>(fr::kGcmContext + 156, load<u32>(fr::kGcmContext + 156) + 1);
}

// The segment bodies run inline when not recording (gfx22's DeferredLists_Flush has the same).
void Overlay(PPUContext& ctx) {
    if (load<u8>(fr::kOverlayOn) == 0) return;
    const u32 o = load<u32>(fr::kOverlayObj);
    if (o != 0) fx::vcall(ctx, o, 16, {o});
}

void Sorted(PPUContext& ctx) {
    const u32 n = load<u32>(fr::kSortedCount);
    if (n == 0) return;
    call(ctx, fr::SortEntries, {fr::kSorted, n, 8});
    while (load<u32>(fr::kSortedWait) != 0) syscall(ctx, fr::kSysTimerUsleep, {fr::kSleepUs});
    for (u32 i = 0; i < load<u32>(fr::kSortedCount); ++i) {  // cmplw, the count reloaded
        const u32 obj = load<u32>(fr::kSorted + 8 * i + 4);
        const u32 d = load<u32>(obj + 64);
        const u32 rec = load<u32>(fr::kSortedBase) + ((load<u32>(obj + 76) << 5) & ~31u);
        const u32 opd = load<u32>(load<u32>(d) + 12);  // r8 / r9 = the entry and the OPD
        call_ptr(ctx, opd, {d, obj, load<u32>(obj + 68), load<u32>(obj + 72), rec, load<u32>(opd), opd});
    }
    store<u32>(fr::kSortedCount, 0);
}

}  // namespace

// void Render_DrawFrame_00518E78(Owner* o)   [Call_0051A6A0, Thunk_0048C874] (name?)
// GPU markers 126..146 around: stream 0 (or its segment), layers 1, 2,
// the shadow shapes (when o's surface is the display), 10, 11, the
// geometry batches (o +256 bit 0x10), the overlay early (u8 0x010DDAD4),
// layers 3..6 in one of two orders (u8 0x010DDAD5) with polygon offset
// off and DrawQueue_Begin, the sorted entries; for the display (not +148
// bit 0x80) the screen targets and viewport and the stale state caches;
// the overlay late, layer 9, then either the lists 3 segment (u8
// 0x010F0A41) or lists 1, the scene's vfunc +12 and, when it returns 0
// with the timer flag, the GPU timer block (shadow depth copy, clear,
// lists 2 between reference values 2042 / 2043, Bloom_Run), else lists 2;
// layers 7, 8, Draw_Pass of the pass stream, layer 13, o's vtable +28
// (o, -61), the pass segment, +28 (o, 60), Call_00548B88, layer 12 and
// DeferredLists_Flush. Each segment is recorded (kPending) or its body
// run inline.
DECOMP(00518E78, Render_DrawFrame_00518E78) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 surface = load<u32>(o + fr::kOwnerSurface);
    Stack frame(ctx, kFrame);
    const u32 sp = frame.addr();
    Mark(ctx, 126);
    if (load<u8>(fr::kPending) != 0) Record(ctx, fr::kSegStream0, sp + kResStream0);
    else DrawLayer(ctx, 0);
    Mark(ctx, 127);
    DrawLayer(ctx, 1);
    Mark(ctx, 128);
    DrawLayer(ctx, 2);
    if (surface == load<u32>(fr::kDisplay)) call(ctx, fr::ShapeList_Draw, {fr::kShapes, 1});
    DrawLayer(ctx, 10);
    if (surface == load<u32>(fr::kDisplay)) call(ctx, fr::ShapeList_Draw, {fr::kShapes, 0});
    DrawLayer(ctx, 11);
    Mark(ctx, 129);
    if (const u32 b = load<u32>(o + fr::kOwnerFlags) & 0x10; b != 0) call(ctx, fr::Fn_576BB4, {b});
    if (load<u8>(fr::kFlagSegOverlayEarly) != 0) {
        Mark(ctx, 130);
        if (load<u8>(fr::kPending) != 0) Record(ctx, fr::kSegOverlay, sp + kResOverlayEarly);
        else Overlay(ctx);
    }
    Mark(ctx, 131);
    if (load<u8>(fr::kFlagLayerOrder) != 0) {
        PolygonOffsetOff();
        DrawLayer(ctx, 3);
        DrawLayer(ctx, 4);
        PolygonOffsetOff();
        Mark(ctx, 132);
        DrawLayer(ctx, 6);
        Mark(ctx, 133);
        call(ctx, fr::DrawQueue_Begin);
        Mark(ctx, 134);
        PolygonOffsetOff();
        DrawLayer(ctx, 5);
        PolygonOffsetOff();
    } else {
        PolygonOffsetOff();
        DrawLayer(ctx, 3);
        PolygonOffsetOff();
        Mark(ctx, 133);
        call(ctx, fr::DrawQueue_Begin);
        Mark(ctx, 134);
        PolygonOffsetOff();
        DrawLayer(ctx, 4);
        DrawLayer(ctx, 5);
        PolygonOffsetOff();
        Mark(ctx, 132);
        DrawLayer(ctx, 6);
    }
    Mark(ctx, 135);
    if (load<u8>(fr::kPending) != 0) Record(ctx, fr::kSegSorted, sp + kResSorted);
    else Sorted(ctx);
    {
        const u32 view = load<u32>(load<u32>(fr::kCurrentSurface) + 672);
        const u32 s = load<u32>(view + 512);
        if (s == load<u32>(fr::kDisplay) && (load<u8>(s + 148) & 0x80) == 0) {
            call(ctx, fr::Display_Prepare, {3});
            call(ctx, fr::Display_BindScreenTargets);
            call(ctx, fr::Draw_SetViewport, {load<u32>(view + 512), view + 96, view + 104});
            store<u64>(fr::kStateCacheA, ~u64{0});
            store<u64>(fr::kStateCacheB, ~u64{0});
        }
    }
    if (load<u8>(fr::kFlagSegOverlayEarly) == 0) {
        Mark(ctx, 130);
        if (load<u8>(fr::kPending) != 0) Record(ctx, fr::kSegOverlay, sp + kResOverlayLate);
        else Overlay(ctx);
    }
    Mark(ctx, 136);
    DrawLayer(ctx, 9);
    Mark(ctx, 137);
    if (load<u8>(fr::kNoBloomTimer) != 0) {
        if (load<u8>(fr::kPending) != 0) Record(ctx, fr::kSegLists3, sp + kResLists3);
        else call(ctx, fr::Draw_Lists, {3});
    } else {
        if (load<u8>(fr::kPending) != 0) Record(ctx, fr::kSegLists1, sp + kResLists1);
        else call(ctx, fr::Draw_Lists, {1});
        const u32 scene = load<u32>(load<u32>(o + fr::kOwnerScene) + 8);
        const u32 busy = static_cast<u32>(fx::vcall(ctx, scene, 12, {scene}));
        const u32 pending = load<u8>(fr::kPending);
        if (busy == 0 && load<u8>(fr::kFlagTimer) != 0) {
            const u32 c = fr::kGcmContext;
            call(ctx, fr::GpuTimer_Update, {fr::kTimer});
            if (load<u8>(fr::kTimerOn) == 0 || load<u8>(fr::kTimer + 32) != 0) {
                call(ctx, fr::ShadowPass_CopyDepth);
                call(ctx, fr::Draw_SetTarget, {load<u32>(fr::kBloomTarget), 0});
                call(ctx, fx::fn::cellGcmSetInvalidateTextureCache, {load<u32>(c), 1});
                // r5 is left holding kConsts (Draw_Clear doesn't read it).
                callf(ctx, fr::Draw_Clear, {240, 0xFF000000u, fr::kConsts, 0, 255},
                      {sly4::render::gfx10::lfs_value(load<u32>(fr::kConsts + 12))});
                emit({fx::kBlendEnable, 1});
            }
            call(ctx, fr::Gcm_SetReferenceCommand, {load<u32>(c), fr::kRefTimerStart});
            if (static_cast<u32>(call(ctx, fr::Draw_Lists, {2})) != 0) {
                const u32 args = sp + kBloomArgs;
                const u32 sc = load<u32>(load<u32>(o + fr::kOwnerScene) + 8);
                for (u32 k = 0; k < 3; ++k) store<u32>(args + 4 * k, load<u32>(sc + 32 + 4 * k));
                store<u32>(args + 20, load<u32>(fr::kBloomTex));
                store<u32>(args + 16, load<u32>(sc + 48));
                store<u32>(args + 24, load<u32>(fr::kBloomTarget));
                call(ctx, fr::Bloom_Run, {args});
            }
            call(ctx, fr::Gcm_SetReferenceCommand, {load<u32>(c), fr::kRefTimerEnd});
            if (load<u8>(fr::kTimerOn) == 0 || load<u8>(fr::kTimer + 32) != 0) {
                call(ctx, fr::RenderState_Reset);
                call(ctx, fr::Draw_InvalidateState);
            }
        } else {
            if (pending != 0) Record(ctx, fr::kSegLists2, sp + kResLists2);
            else call(ctx, fr::Draw_Lists, {2});
        }
    }
    Mark(ctx, 138);
    DrawLayer(ctx, 7);
    Mark(ctx, 139);
    DrawLayer(ctx, 8);
    Mark(ctx, 140);
    {
        const u32 state = sp + kPassState;
        const u32 stream = load<u32>(fr::kPassStream);
        store<u32>(state, 0);
        store<u32>(state + 4, 0);
        for (u32 k = 0; k < 16; ++k) store<u32>(state + 16 + 4 * k, load<u32>(fr::kPassMatrix + 4 * k));  // lfs / stfs: raw
        ctx.fpr[1] = sly4::render::gfx10::lfs_value(load<u32>(fr::kPassMatrix + 56));  // left over from the copy
        ctx.fpr[2] = sly4::render::gfx10::lfs_value(load<u32>(fr::kPassMatrix + 60));
        call(ctx, fr::Draw_Pass, {stream, state});
    }
    DrawLayer(ctx, 13);
    Mark(ctx, 141);
    fx::vcall(ctx, o, 28, {o, fr::kFinishA});
    Mark(ctx, 142);
    if (load<u8>(fr::kPending) != 0) Record(ctx, fr::kSegPass, sp + kResPass);
    else call(ctx, fr::Render_Pass575C34);
    Mark(ctx, 143);
    fx::vcall(ctx, o, 28, {o, fr::kFinishB});
    Mark(ctx, 144);
    call(ctx, fr::Call_00548B88);
    Mark(ctx, 145);
    DrawLayer(ctx, 12);
    Mark(ctx, 146);
    ctx.gpr[3] = call(ctx, fr::DeferredLists_Flush);
}
