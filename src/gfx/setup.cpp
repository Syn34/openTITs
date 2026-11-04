// Video-out set-up, called by Gcm_Init: Video_SelectMode picks the output
// resolution (720p frame-packed 3D, 720p, the current mode, then 480 /
// 576) and Video_Configure configures the output, the flip mode and the
// two display buffers. See setup.h for the fields they fill.
#include "setup.h"

using namespace sly4;

namespace {

namespace vfn = sly4::vsetup::fn;
namespace vimp = sly4::vsetup::imp;
namespace vg = sly4::vsetup::g;

constexpr u32 kStats = g::mem_stats;
constexpr u32 kConfig = g::config;
constexpr u32 kDisplay = 0x00F66FBC;  // AglRenderSurface*: the display (see display.h)

constexpr u32 kStrNewline = 0x00AF31F4;    // "\n"
constexpr u32 kStrMode = 0x00AF31FC;       // "Display mode %s "
constexpr u32 kStrAspect = 0x00AF3214;     // "with aspect ratio %s "
constexpr u32 kStrAvailable = 0x00AF322C;  // "is available, as returned by cellVideoOutGetResolutionAvailability\n\n"
constexpr u32 kStr2D = 0x00AF3274;         // "2D Video mode configured as %ix%i\n"
constexpr u32 kStr3D = 0x00AF329C;         // "3D Video mode configured as %ix%i\n"

constexpr u32 kRes720_3D = 129;  // CELL_VIDEO_OUT_RESOLUTION_720_3D_FRAME_PACKING
constexpr u32 kRes720 = 2, kRes480 = 4, kRes576 = 5;
constexpr u32 kAspectAuto = 0, kAspect16_9 = 2;

s32 availability(PPUContext& ctx, u32 id, u32 aspect) {
    return static_cast<s32>(call(ctx, vimp::cellVideoOutGetResolutionAvailability, {0, id, aspect, 0}));
}

// The "Display mode ... is available" report.
void print_available(PPUContext& ctx, u32 id, u32 aspect) {
    call(ctx, vfn::printf, {kStrNewline});
    for (u32 i = 0; i < 24; ++i) {
        const u32 e = vg::mode_names + 8 * i;
        if (load<s32>(e) == static_cast<s32>(id)) call(ctx, vfn::printf, {kStrMode, load<u32>(e + 4)});
    }
    for (u32 i = 0; i < 3; ++i) {
        const u32 e = vg::aspect_names + 8 * i;
        if (load<s32>(e) == static_cast<s32>(aspect)) call(ctx, vfn::printf, {kStrAspect, load<u32>(e + 4)});
    }
    call(ctx, vfn::printf, {kStrAvailable});
}

s32 check(PPUContext& ctx, u32 id, u32 aspect) {
    const s32 r = availability(ctx, id, aspect);
    if (r == 1) print_available(ctx, id, aspect);
    return r;
}

// Width / height into config +4 / +6, 5% margins at +24 / +26, the size
// inside them at +28 / +30 and +33 = aspect is not 4:3.
void set_screen(u32 w, u32 h) {
    const s32 mx = static_cast<s32>(w * 5) / 100;
    const s32 my = static_cast<s32>(h * 5) / 100;
    field<u16>(kConfig, 4) = static_cast<u16>(w);
    field<u16>(kConfig, 6) = static_cast<u16>(h);
    field<u16>(kConfig, 24) = static_cast<u16>(mx);
    field<u16>(kConfig, 26) = static_cast<u16>(my);
    field<u16>(kConfig, 28) = static_cast<u16>(w - 2 * static_cast<u32>(mx));
    field<u16>(kConfig, 30) = static_cast<u16>(h - 2 * static_cast<u32>(my));
    field<u8>(kConfig, 33) = field<u8>(kStats, 11) != 1 ? 1 : 0;
}

}  // namespace

// f32 Video_SelectMode()
// 720p frame packing when the TV offers it and reports a screen size
// (also sets config +35, 3D available); otherwise 720p 16:9, the current
// video-out mode, 480 or 576. Stores both resolutions, the safe area and
// the refresh rate. Returns 1.0f in r3 (what is left there).
DECOMP(0052FD2C, Video_SelectMode) {
    store<u32>(kStats + 28, 0);
    store<u32>(kStats + 24, kRes720_3D);
    u32 id;
    if (check(ctx, kRes720_3D, kAspect16_9) == 1 &&
        static_cast<s32>(call(ctx, vimp::cellVideoOutGetScreenSize, {0, kStats + 160})) == 0) {
        id = kRes720;
        store<u32>(kStats + 28, id);
        field<u8>(kConfig, 35) = 1;
        store<u32>(kStats + 24, kRes720_3D);
    } else if (check(ctx, kRes720, kAspect16_9) == 1) {
        id = kRes720;
        store<u32>(kStats + 28, id);
    } else {
        call(ctx, vimp::cellVideoOutGetState, {0, 0, kStats});
        id = ref<u8>(kStats + 8);
        store<u32>(kStats + 28, id);
        if (id == 0) {
            id = kRes480;
            if (check(ctx, kRes480, kAspectAuto) != 1 && check(ctx, kRes576, kAspectAuto) == 1) id = kRes576;
            store<u32>(kStats + 28, id);
        }
    }

    store<u32>(kStats + 16, 0);
    call(ctx, vimp::cellVideoOutGetResolution, {id, kStats + 16});
    call(ctx, vfn::printf, {kStr2D, load<u16>(kStats + 16), load<u16>(kStats + 18)});
    store<u32>(kStats + 20, 0);
    call(ctx, vimp::cellVideoOutGetResolution, {load<u32>(kStats + 24), kStats + 20});
    call(ctx, vfn::printf, {kStr3D, load<u16>(kStats + 20), load<u16>(kStats + 22)});

    const u32 w = load<u16>(kStats + 16), h = load<u16>(kStats + 18);
    ref<u8>(kStats + 8) = static_cast<u8>(load<u32>(kStats + 28));
    field<u16>(kConfig, 8) = 0;
    field<u16>(kConfig, 10) = 0;
    set_screen(w, h);

    u32 rate;
    switch (load<u16>(kStats + 14)) {
    case 8: rate = 0x41F00000; break;  // 30 Hz
    case 4: rate = 0x42700000; break;  // 60 Hz
    case 2: rate = 0x42480000; break;  // 50 Hz
    default: rate = 0x426FC28F; break; // 59.94 Hz
    }
    field<u32>(kConfig, 20) = rate;
    store<u32>(kStats + 56, 0x3F800000);  // 1.0f
    ctx.gpr[3] = 0x3F800000;
}

// bool Video_Configure(u8 resolution_id, bool unbind_tiles, bool rebind)
// Configures video out with the pitch of the current front buffer, then
// optionally drops the tile / zcull bindings and binds the display (or
// 3D) colour targets again, sets h-sync flips and registers the two
// display buffers. A front buffer of a new size updates the display size
// and config screen fields. Sets display +137; 0 if configuring fails.
DECOMP(005312E4, Video_Configure) {
    const u8 mode = arg<u8>(ctx, 0);
    const bool unbind = arg<u8>(ctx, 1) != 0, rebind = arg<u8>(ctx, 2) != 0;
    const u32 d = load<u32>(kDisplay);
    u32 front;
    if (((load<u32>(d + 52) >> 25) & 1) == 0) front = d + 16;
    else if (field<u8>(kConfig, 35) == 0 || field<u8>(kConfig, 34) == 0) front = (ref<u8>(d + 148) >> 7) != 0 ? d + 84 : d + 64;
    else front = load<u32>(kStats + 184) == 1 ? d + 100 : d + 108;

    const u32 pitch = load<u32>(load<u32>(front) + 32);
    call(ctx, vimp::cellGcmResetFlipStatus);
    call(ctx, vfn::Gcm_Sub5E0E50, {load<u32>(vg::gcm_context)});
    call(ctx, vfn::Gcm_Sub5E3B84, {load<u32>(vg::gcm_context), load<u32>(vg::frame_count)});
    store<u32>(vg::frame_count, load<u32>(vg::frame_count) + 1);
    {
        Stack frame(ctx, 16);  // CellVideoOutConfiguration
        const u32 cfg = frame.addr();
        for (u32 i = 0; i < 16; i += 4) store<u32>(cfg + i, 0);
        ref<u8>(cfg + 0) = mode;  // resolutionId
        ref<u8>(cfg + 1) = 0;     // format
        store<u32>(cfg + 12, pitch);
        if (static_cast<s32>(call(ctx, vimp::cellVideoOutConfigure, {0, cfg, 0, 0})) != 0) {
            ctx.gpr[3] = 0;
            return;
        }
    }
    if (unbind) call(ctx, vfn::Tiles_Unbind);

    if (rebind) {
        const u32 d2 = load<u32>(kDisplay);
        const bool in_3d = field<u8>(kConfig, 34) != 0;
        const u32 a = in_3d ? 92 : 64, b = in_3d ? 96 : 68, c = in_3d ? 116 : 72;
        const u32 e = in_3d ? 128 : 76, f = in_3d ? 132 : 80;
        call(ctx, vfn::Target_BindTile, {d2 + a, 0});
        call(ctx, vfn::Target_BindTile, {d2 + b, 0});
        call(ctx, vfn::Target_BindTile, {d2 + c, 1});
        call(ctx, vfn::Target_BindZcull, {d2 + c});
        if (static_cast<s32>((load<u32>(d2 + 52) >> 22) & 7) > 1) {
            call(ctx, vfn::Target_BindTile, {d2 + e, 0});
            call(ctx, vfn::Target_BindTile, {d2 + f, 1});
            call(ctx, vfn::Target_BindZcull, {d2 + f});
        }
    }

    call(ctx, vimp::cellGcmSetFlipMode, {2});  // CELL_GCM_DISPLAY_HSYNC
    const u32 d3 = load<u32>(kDisplay);
    const bool from_3d = field<u8>(kConfig, 35) != 0 && field<u8>(kConfig, 34) != 0;
    u32 buffers[2];
    buffers[0] = load<u32>(d3 + (from_3d ? 92 : 64));
    buffers[1] = load<u32>(d3 + (from_3d ? 96 : 68));
    for (u32 i = 0; i < 2; ++i) {
        const u32 t = buffers[i];
        call(ctx, vimp::cellGcmSetDisplayBuffer,
             {i, load<u32>(t + 4) >> 1, load<u32>(t + 32), load<u16>(t + 36), load<u16>(t + 38)});
    }

    const u32 shown = load<u32>(front);
    const u32 d4 = load<u32>(kDisplay);
    const u16 w = load<u16>(shown + 36), h = load<u16>(shown + 38);
    if (field<u16>(kConfig, 4) != w || field<u16>(kConfig, 6) != h) {
        store<u32>(d4 + 56, w);
        store<u32>(d4 + 60, h);
        set_screen(w, h);
    }
    ref<u8>(d + 137) = 1;
    ctx.gpr[3] = 1;
}
