// Video-out set-up, called by Gcm_Init: Video_SelectMode picks the output
// resolution (720p frame-packed 3D, 720p, the current mode, then 480 /
// 576) and Video_Configure configures the output, the flip mode and the
// two display buffers. See setup.h for the fields they fill.
#include "setup.h"
#include "rsx.h"

namespace sly4::gfx {

namespace {

namespace vfn = sly4::vsetup::fn;
namespace vimp = sly4::vsetup::imp;
namespace vg = sly4::vsetup::g;

constexpr u32 kStrNewline = 0x00AF31F4;    // "\n"
constexpr u32 kStrMode = 0x00AF31FC;       // "Display mode %s "
constexpr u32 kStrAspect = 0x00AF3214;     // "with aspect ratio %s "
constexpr u32 kStrAvailable = 0x00AF322C;  // "is available, as returned by cellVideoOutGetResolutionAvailability\n\n"
constexpr u32 kStr2D = 0x00AF3274;         // "2D Video mode configured as %ix%i\n"
constexpr u32 kStr3D = 0x00AF329C;         // "3D Video mode configured as %ix%i\n"

constexpr u32 kRes720_3D = 129;  // CELL_VIDEO_OUT_RESOLUTION_720_3D_FRAME_PACKING
constexpr u32 kRes720 = 2, kRes480 = 4, kRes576 = 5;
constexpr u32 kAspectAuto = 0, kAspect16_9 = 2;

// {id, name} entries of the mode / aspect name tables.
struct NameEntry {
    be_s32 id;
    be_u32 name;  // const char*
};

// CellVideoOutConfiguration.
struct VideoOutConfig {
    u8 resolution;  // +0
    u8 format;      // +1
    pad<10> _2;
    be_u32 pitch;   // +12
};

s32 availability(u32 id, u32 aspect) {
    return call<s32>(vimp::cellVideoOutGetResolutionAvailability, 0u, id, aspect, 0u);
}

// The "Display mode ... is available" report.
void print_available(u32 id, u32 aspect) {
    call(vfn::printf, kStrNewline);
    const NameEntry* modes = host<NameEntry>(vg::mode_names);
    for (u32 i = 0; i < 24; ++i)
        if (modes[i].id == static_cast<s32>(id)) call(vfn::printf, kStrMode, modes[i].name.get());
    const NameEntry* aspects = host<NameEntry>(vg::aspect_names);
    for (u32 i = 0; i < 3; ++i)
        if (aspects[i].id == static_cast<s32>(aspect)) call(vfn::printf, kStrAspect, aspects[i].name.get());
    call(vfn::printf, kStrAvailable);
}

s32 check(u32 id, u32 aspect) {
    const s32 r = availability(id, aspect);
    if (r == 1) print_available(id, aspect);
    return r;
}

// Width / height into config +4 / +6, 5% margins at +24 / +26, the size
// inside them at +28 / +30 and +33 = aspect is not 4:3.
void set_screen(u32 w, u32 h) {
    Config& cfg = config();
    const s32 mx = static_cast<s32>(w * 5) / 100;
    const s32 my = static_cast<s32>(h * 5) / 100;
    cfg.width = static_cast<u16>(w);
    cfg.height = static_cast<u16>(h);
    cfg.safe_x = static_cast<u16>(mx);
    cfg.safe_y = static_cast<u16>(my);
    cfg.safe_w = static_cast<u16>(w - 2 * static_cast<u32>(mx));
    cfg.safe_h = static_cast<u16>(h - 2 * static_cast<u32>(my));
    cfg.wide = mem_stats().aspect != 1 ? 1 : 0;
}

}  // namespace

// void Video_SelectMode()
// 720p frame packing when the TV offers it and reports a screen size
// (also sets config +35, 3D available); otherwise 720p 16:9, the current
// video-out mode, 480 or 576. Stores both resolutions, the safe area and
// the refresh rate.
void Video_SelectMode() {
    MemStats& ms = mem_stats();
    Config& cfg = config();
    ms.res_id = 0u;
    ms.res3d_id = kRes720_3D;
    u32 id;
    if (check(kRes720_3D, kAspect16_9) == 1 && call<s32>(vimp::cellVideoOutGetScreenSize, 0u, &ms.screen_size) == 0) {
        id = kRes720;
        ms.res_id = id;
        cfg.stereo_ok = 1;
        ms.res3d_id = kRes720_3D;
    } else if (check(kRes720, kAspect16_9) == 1) {
        id = kRes720;
        ms.res_id = id;
    } else {
        call(vimp::cellVideoOutGetState, 0u, 0u, &ms);
        id = ms.resolution;
        ms.res_id = id;
        if (id == 0) {
            id = kRes480;
            if (check(kRes480, kAspectAuto) != 1 && check(kRes576, kAspectAuto) == 1) id = kRes576;
            ms.res_id = id;
        }
    }

    *at<be_u32>(&ms, 16) = 0u;  // the 2D resolution, as one word
    call(vimp::cellVideoOutGetResolution, id, &ms.res_w);
    call(vfn::printf, kStr2D, static_cast<u32>(ms.res_w), static_cast<u32>(ms.res_h));
    *at<be_u32>(&ms, 20) = 0u;  // the 3D resolution
    call(vimp::cellVideoOutGetResolution, ms.res3d_id.get(), &ms.buffer_w);
    call(vfn::printf, kStr3D, static_cast<u32>(ms.buffer_w), static_cast<u32>(ms.buffer_h));

    const u32 w = ms.res_w, h = ms.res_h;
    ms.resolution = static_cast<u8>(ms.res_id);
    cfg.unk8 = 0;
    cfg.unk10 = 0;
    set_screen(w, h);

    u32 rate;
    switch (ms.refresh_rates) {
    case 8: rate = 0x41F00000; break;  // 30 Hz
    case 4: rate = 0x42700000; break;  // 60 Hz
    case 2: rate = 0x42480000; break;  // 50 Hz
    default: rate = 0x426FC28F; break; // 59.94 Hz
    }
    cfg.refresh = as_f32(rate);
    ms.one = 1.0f;
}

// bool Video_Configure(u8 resolution_id, bool unbind_tiles, bool rebind)
// Configures video out with the pitch of the current front buffer, then
// optionally drops the tile / zcull bindings and binds the display (or
// 3D) colour targets again, sets h-sync flips and registers the two
// display buffers. A front buffer of a new size updates the display size
// and config screen fields. Sets display +137; false if configuring fails.
bool Video_Configure(u8 mode, bool unbind, bool rebind) {
    // The pointers below are not checked by the original (it reads guest page 0).
    Display* d = or_zero_page(display());
    MemStats& ms = mem_stats();
    Config& cfg = config();
    gp<Target>* front;
    if (!per_eye(d)) front = &d->colour16[0];
    else if (cfg.stereo_ok == 0 || cfg.stereo == 0) front = alternate(d) ? &d->alt_colour : &d->eye_colour[0];
    else front = ms.buffer_select == 1 ? &d->eye_view[0] : &d->eye_view[2];

    const u32 pitch = or_zero_page(front->get())->pitch;
    call(vimp::cellGcmResetFlipStatus);
    be_u32& frames = global<be_u32>(vg::frame_count);
    call(vfn::Gcm_Sub5E0E50, global<be_u32>(vg::gcm_context).get());
    call(vfn::Gcm_Sub5E3B84, global<be_u32>(vg::gcm_context).get(), frames.get());
    frames = frames + 1;
    {
        GuestFrame f(16);
        VideoOutConfig* vc = f.as<VideoOutConfig>();
        for (u32 i = 0; i < 4; ++i) f.as<be_u32>()[i] = 0u;
        vc->resolution = mode;
        vc->format = 0;
        vc->pitch = pitch;
        if (call<s32>(vimp::cellVideoOutConfigure, 0u, vc, 0u, 0u) != 0) return false;
    }
    if (unbind) Tiles_Unbind();

    if (rebind) {
        Display* d2 = or_zero_page(display());
        const bool in_3d = cfg.stereo != 0;
        Target_BindTile(in_3d ? &d2->copy[0] : &d2->eye_colour[0], false);
        Target_BindTile(in_3d ? &d2->copy[1] : &d2->eye_colour[1], false);
        gp<Target>* depth = in_3d ? &d2->packed : &d2->eye_depth;
        Target_BindTile(depth, true);
        Target_BindZcull(depth);
        if (static_cast<s32>((d2->flags >> 22) & 7) > 1) {
            Target_BindTile(in_3d ? &d2->extra_view[0] : &d2->extra[0], false);
            gp<Target>* depth2 = in_3d ? &d2->extra_view[1] : &d2->extra[1];
            Target_BindTile(depth2, true);
            Target_BindZcull(depth2);
        }
    }

    call(vimp::cellGcmSetFlipMode, 2u);  // CELL_GCM_DISPLAY_HSYNC
    Display* d3 = or_zero_page(display());
    const bool from_3d = cfg.stereo_ok != 0 && cfg.stereo != 0;
    Target* buffers[2] = {from_3d ? d3->copy[0] : d3->eye_colour[0], from_3d ? d3->copy[1] : d3->eye_colour[1]};
    for (u32 i = 0; i < 2; ++i) {
        const Target* t = or_zero_page(buffers[i]);
        call(vimp::cellGcmSetDisplayBuffer, i, t->offset_loc >> 1, t->pitch.get(), static_cast<u32>(t->width),
             static_cast<u32>(t->height));
    }

    const Target* shown = or_zero_page(front->get());
    Display* d4 = or_zero_page(display());
    const u16 w = shown->width, h = shown->height;
    if (cfg.width != w || cfg.height != h) {
        d4->width = w;
        d4->height = h;
        set_screen(w, h);
    }
    *at<u8>(d, 137) = 1;
    return true;
}

}  // namespace sly4::gfx

NATIVE(0052FD2C, sly4::gfx::Video_SelectMode)
NATIVE(005312E4, sly4::gfx::Video_Configure)
