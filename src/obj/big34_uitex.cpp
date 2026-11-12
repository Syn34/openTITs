// Game code, large-function sweep part 34: the B8 linear texture set a UI
// screen draws into, carved out of the surface heap.
#include "big34.h"

namespace sly4::obj {

// One plane of a layer (12 bytes).
struct UiPlane {
    be_u32 enabled;  // +0
    be_u32 data;     // +4
    be_u32 pitch;    // +8
};
// A layer's four planes (48 bytes at the screen's +24).
struct UiLayerPlanes {
    UiPlane p[4];
};
// A layer's texture pointers (20 bytes at the screen's +120; +16 zeroed).
struct UiLayerTex {
    gp<GcmTexture> t[4];  // +0 .. +12
    be_u32 end;           // +16
};
struct UiTexScreen {
    be_s32 layers;              // +0
    be_u32 w, h;                // +4, +8: the full-size planes
    be_u32 hw, hh;              // +12, +16: the half planes
    pad<4> _20;
    UiLayerPlanes planes[2];    // +24 (indexed past 2 by guest address)
    UiLayerTex tex[2];          // +120
    be_u32 buffer;              // +160
    be_u32 stride;              // +164
};
static_assert(offsetof(UiTexScreen, planes) == 24 && offsetof(UiTexScreen, tex) == 120);
static_assert(offsetof(UiTexScreen, buffer) == 160 && offsetof(UiTexScreen, stride) == 164);

namespace {
constexpr u32 kSurfaceHeap = 0x01163FC8;  // the cursor allocator the buffers come from
constexpr u8 kTexFormatB8Linear = 0xA1;   // CELL_GCM_TEXTURE_B8 | CELL_GCM_TEXTURE_LN
constexpr u32 kTexRemap = 0xAAE4;         // the default (identity) remap
constexpr u8 kLocationMain = 1;           // CELL_GCM_LOCATION_MAIN

constexpr u32 Align128(u32 x) { return (x + 127) & ~127u; }
// A buffer's size: its pixels rounded up to 128 bytes, plus 128 for the
// CellGcmTexture placed right after them (0 stays 0).
constexpr u32 BufSize(u32 bytes) {
    const u32 s = Align128(bytes);
    return s != 0 ? s + 128 : 0;
}
}  // namespace

// bool T::AllocTextures(T* o) (called by Ui_OpenScreen4D245C). Layer 0's
// enable flags size every layer: planes 0 and 3 have pitch (plane 0 on ?
// w : 0) | (plane 3 on ? w : 0), planes 1 and 2 pitch (plane 1 on ?
// align16(hw) : 0); the full set is Cursor_Alloc(kSurfaceHeap, 64, layers *
// stride) into o->buffer, rejected when null or in 0xC0000000..0xCFFFFFFF.
// Each enabled plane gets its data pointer and pitch and a CellGcmTexture
// (B8, linear, main memory) right after its pixels, whose offset
// cellGcmAddressToOffset fills in; a zero offset fails. Returns 1 with
// o->stride set, or 0 (o->buffer cleared when it was set).
bool UiScreen_AllocB8Textures(UiTexScreen* o) {
    const u32 w = o->w;
    const UiLayerPlanes& l0 = o->planes[0];
    const u32 pa = l0.p[0].enabled != 0 ? w : 0;
    const u32 pb = l0.p[3].enabled != 0 ? w : 0;
    const u32 pitchHalf = l0.p[1].enabled != 0 ? (o->hw + 15) & ~15u : 0;
    const u32 h = o->h;
    const u32 s0 = BufSize(h * pa);
    const u32 s3 = BufSize(pb * h);
    const u32 s12 = BufSize(o->hh * pitchHalf);
    const u32 off3 = s0;          // plane 3
    const u32 off1 = s3 + s0;     // plane 1
    const u32 off2 = off1 + s12;  // plane 2
    const u32 stride = off1 + 2 * s12;
    const u32 pitchFull = pa | pb;

    u32 cur = call<u32>(b34fn::Cursor_Alloc, kSurfaceHeap, 64u, stride * static_cast<u32>(o->layers));
    o->buffer = cur;
    bool ok = cur != 0 && !(cur >= 0xC0000000u && cur < 0xD0000000u);
    if (ok) {
        cur = Align128(cur);
        // One plane: data / pitch into it, the texture after the pixels,
        // its pointer into *slot. False when the offset comes back 0.
        auto plane = [&](UiPlane& pl, gp<GcmTexture>& slot, u32 data, u32 pitch, bool half) {
            pl.data = data;
            pl.pitch = pitch;
            const u32 hh = half ? o->hh : o->h, ww = half ? o->hw : o->w;
            const u32 tex_ea = data + Align128(hh * pitch);
            GcmTexture& tex = gm<GcmTexture>(tex_ea);
            tex.format = kTexFormatB8Linear;
            tex.mipmap = 1;
            tex.dimension = 2;  // 2D
            tex.cubemap = 0;
            tex.remap = kTexRemap;
            tex.width = static_cast<u16>(ww);
            tex.height = static_cast<u16>(hh);
            tex.depth = 1;
            tex.location = kLocationMain;
            tex.pitch = pitch;
            call(b34fn::cellGcmAddressToOffset, data, &tex.offset);
            if (tex.offset == 0) return false;
            slot.ea = tex_ea;
            return true;
        };
        for (s32 i = 0; i < o->layers; ++i) {
            // by guest address: the count is not limited to the two layers the screen has room for
            UiLayerPlanes& l = gm<UiLayerPlanes>(guest(o) + 24 + 48 * static_cast<u32>(i));
            UiLayerTex& t = gm<UiLayerTex>(guest(o) + 120 + 20 * static_cast<u32>(i));
            if (l.p[0].enabled != 0 && !plane(l.p[0], t.t[0], cur, pitchFull, false)) { ok = false; break; }
            if (l.p[1].enabled != 0 && !plane(l.p[1], t.t[1], cur + off1, pitchHalf, true)) { ok = false; break; }
            if (l.p[2].enabled != 0 && !plane(l.p[2], t.t[2], cur + off2, pitchHalf, true)) { ok = false; break; }
            if (l.p[3].enabled != 0 && !plane(l.p[3], t.t[3], cur + off3, pitchFull, false)) { ok = false; break; }
            t.end = 0u;
            cur += stride;
        }
    }
    if (!ok) {
        if (o->buffer != 0) o->buffer = 0u;
        return false;
    }
    o->stride = stride;
    return true;
}

}  // namespace sly4::obj

NATIVE(004D10C8, sly4::obj::UiScreen_AllocB8Textures)
