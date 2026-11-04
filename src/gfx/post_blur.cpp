// The 11-tap Gaussian blur programs' texel offsets: tap table entries
// scaled by k / (quarter screen size), written into the fragment
// program's constants with change tracking.
#include "post.h"

#include <bit>

using namespace sly4;

namespace {

namespace pg = sly4::post::g;

constexpr u32 kConfig = g::config;

f32 load_f32(u32 a) { return std::bit_cast<f32>(load<u32>(a)); }

// program: address of the FProgram*; k: address of the f32 scale;
// taps: 11 x {f32 x, f32 y}.
u32 set_offsets(u32 program, u32 k, u32 taps) {
    const u32 state = pg::gcm_state;
    const u32 current = load<u32>(state + 28);
    const u32 p = load<u32>(program);
    const u32 stamp = load<u32>(state + 156);
    if (current != p) {
        store<u32>(state + 28, p);
        store<u32>(state + 92, load<u32>(state + 92) | 0x80000000u);
    }
    const f32 scale = load_f32(k);
    const s32 qw = static_cast<s32>(field<u16>(kConfig, 4)) / 4, qh = static_cast<s32>(field<u16>(kConfig, 6)) / 4;
    const f32 sx = scale / static_cast<f32>(static_cast<f64>(qw));
    const f32 sy = scale / static_cast<f32>(static_cast<f64>(qh));
    for (u32 i = 0; i < 11; ++i) {
        const f32 x = load_f32(taps + 8 * i) * sx;
        const f32 y = load_f32(taps + 8 * i + 4) * sy;
        const u32 rec = p + 16 * i + 592;
        if (load<u32>(rec) == 0) continue;
        const u32 reg = load<u32>(rec + 12);
        const u64 value = (static_cast<u64>(std::bit_cast<u32>(x)) << 32) | std::bit_cast<u32>(y);
        const u32 lo = p + 48 + (reg << 4), hi = p + 56 + (reg << 4);
        const u64 old_lo = load<u64>(lo), old_hi = load<u64>(hi);
        store<u64>(lo, value);
        store<u64>(hi, 0);
        const bool restamp = (load<u32>(p + 24) ^ stamp) != 0;
        const bool changed = ((value ^ old_lo) | old_hi) != 0;
        const u32 shift = reg & 0x7F;  // sld
        const u64 bit = shift < 64 ? u64{1} << shift : 0;
        store<u64>(p + 16, load<u64>(p + 16) | ((restamp || changed) ? bit : 0));
    }
    return stamp;
}

}  // namespace

// void Blur11V_SetOffsets()
// PostProc_GuassianBlur11V: vertical taps (0x00F6794C), scale 1.0f.
DECOMP(0053CE00, Blur11V_SetOffsets) {
    ctx.gpr[3] = set_offsets(0x011640BC, 0x0053CDFC, 0x00F6794C);
}

// void Blur11H_SetOffsets()
// PostProc_GuassianBlur11H: horizontal taps (0x00F679A4), scale 1.0f.
DECOMP(0053CFD0, Blur11H_SetOffsets) {
    ctx.gpr[3] = set_offsets(0x011640B4, 0x0053CFC8, 0x00F679A4);
}
