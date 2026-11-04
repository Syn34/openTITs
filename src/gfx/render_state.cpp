// Display buffer selection, the depth target, the render-state cache
// reset and the vertex format table.
#include "render.h"

using namespace sly4;

namespace {

constexpr u32 kStats = sly4::g::mem_stats;
constexpr u32 kConfig = sly4::g::config;

u32 eye_offset(u32 disp) { return (load<u32>(disp + 52) >> 28) & 0xC; }  // rlwinm 4,28,29: bits 30..31 * 4
bool per_eye(u32 disp) { return ((load<u32>(disp + 52) >> 25) & 1) != 0; }    // rlwinm 7,31,31
bool alternate(u32 disp) { return (ref<u8>(disp + 148) >> 7) != 0; }          // rlwinm 25,31,31

}  // namespace

// void Display_GetTargets(Target** colour[1], Target** depth[1])
// Stores the address of the display's colour target slot and depth
// target slot for the current eye / buffer set (the choice
// fx::display_depth makes for the depth one). Returns the depth slot.
DECOMP(005356F4, Display_GetTargets) {
    const u32 colour_out = arg<u32>(ctx, 0), depth_out = arg<u32>(ctx, 1);
    const u32 disp = load<u32>(draw::g::display);
    const u32 k = eye_offset(disp);
    u32 colour;
    if (!per_eye(disp)) {
        colour = disp + k + 16;
    } else if (field<u8>(kConfig, 35) == 0 || field<u8>(kConfig, 34) == 0) {
        colour = alternate(disp) ? disp + 84 : disp + k + 64;
    } else {
        colour = disp + k + (load<u32>(kStats + 184) == 1 ? 100 : 108);
    }
    store<u32>(colour_out, colour);

    u32 depth;
    if (!per_eye(disp)) {
        depth = disp + 24;
    } else if (field<u8>(kConfig, 35) == 0 || field<u8>(kConfig, 34) == 0) {
        depth = disp + (alternate(disp) ? 88 : 72);
    } else {
        depth = disp + (load<u32>(kStats + 184) == 1 ? 120 : 124);
    }
    store<u32>(depth_out, depth);
    ret(ctx, depth);
}

// Target* Display_SelectBuffer(Target* const* table)
// The entry of a per-buffer table laid out like g::mem_stats (+128 for
// the alternate set, else +92 / +112 / +120 per eye), as fx::scene_colour
// picks it.
DECOMP(00537C18, Display_SelectBuffer) {
    const u32 table = arg<u32>(ctx, 0);
    const u32 disp = load<u32>(draw::g::display);
    u32 slot;
    if (alternate(disp)) {
        slot = table + 128;
    } else if (field<u8>(kConfig, 34) != 0) {
        slot = table + eye_offset(disp) + (load<u32>(kStats + 184) == 1 ? 112 : 120);
    } else {
        slot = table + eye_offset(disp) + 92;
    }
    ret(ctx, load<u32>(slot));
}

// void Draw_SetDepthTarget(Target* d)
// Makes d the depth target (0 = none) unless it already is; see
// render::set_depth_target.
DECOMP(0053755C, Draw_SetDepthTarget) {
    ctx.gpr[3] = render::set_depth_target(ctx, arg<u32>(ctx, 0));
}

// void Draw_InvalidateState()
// Forgets everything the render state caches (programs, textures, vertex
// attributes, blend / depth / stencil words) so the next draws resend it,
// and resets the state cached in the object at render::g::state_cache.
DECOMP(0051279C, Draw_InvalidateState) {
    const u32 st = fx::g::state;
    constexpr u32 kAll = 0xFFFFFFFFu;
    constexpr u32 kFunc = 519;     // 0x207 (stored at +160 / +188)
    constexpr u32 kKeep = 0x1E00;  // 7680 (stored at +176.. / +204..)
    store<u32>(st + 4, 0);
    store<u32>(st + 8, kAll);  // vertex constant cache stale
    store<u32>(st + 16, 0);
    store<u32>(st + 20, 0);  // vertex program
    store<u32>(st + 24, 0);  // its Cg program
    store<u32>(st + 28, 0);  // fragment program
    store<u16>(st + 32, 0xFFFF);  // vertex attribute mask
    store<u32>(st + 88, 0);
    for (u32 i = 0; i < 8; ++i) store<u32>(st + 52 + 4 * i, 0);  // texture units
    store<u32>(st + 216, kAll);
    ref<u8>(st + 220) = 0;
    ref<u8>(st + 221) = 0;
    store<u32>(st + 160, kFunc);
    store<u32>(st + 164, 0);
    store<u32>(st + 168, kAll);
    store<u32>(st + 172, kAll);
    const u32 dirty = load<u32>(st + 92) & 0x7FFFFFFFu;
    store<u32>(st + 176, kKeep);
    store<u32>(st + 180, kKeep);
    store<u32>(st + 184, kKeep);
    store<u32>(st + 188, kFunc);
    store<u32>(st + 192, 0);
    store<u32>(st + 196, kAll);
    const u32 obj = load<u32>(render::g::state_cache);
    store<u32>(st + 200, kAll);
    store<u32>(st + 204, kKeep);
    store<u32>(st + 208, kKeep);
    store<u32>(st + 212, kKeep);
    store<u64>(render::g::state_cache_a, ~u64{0});
    store<u64>(render::g::state_cache_b, ~u64{0});
    store<u32>(st + 92, dirty);  // fragment-program-changed bit cleared
    store<u32>(obj + 80, kAll);
    store<u32>(obj + 84, kAll);
    store<u32>(obj + 88, kAll);
    store<u32>(obj + 96, 0x47C35000);  // 100000.0f
    store<u32>(obj + 160, kAll);
    store<u64>(obj + 176, ~u64{0});
    store<u16>(obj + 164, 0xFFFF);
    store<u64>(obj + 184, ~u64{0});
    store<u32>(obj + 192, kAll);
    store<u32>(obj + 196, 0xBF800000);  // -1.0f
    ref<u8>(obj + 200) = 0;
    ctx.gpr[3] = 0x47C35000;  // left in r3 by the original
}

// const VertexFormat* VertexFormat_Get(bool second)
// One of the two 128-byte vertex formats at render::g::vertex_formats.
DECOMP(00515750, VertexFormat_Get) {
    const u32 kind = arg<u32>(ctx, 0) & 0xFF;
    ret(ctx, render::g::vertex_formats + (kind != 0 ? 128u : 0u));
}
