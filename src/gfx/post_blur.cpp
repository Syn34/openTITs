// The 11-tap Gaussian blur programs' texel offsets: tap table entries
// scaled by k / (quarter screen size), written into the fragment
// program's constants with change tracking.
#include "rsx.h"

namespace sly4::gfx {

namespace {

// program: the FProgram* global; k: the f32 scale; taps: 11 x {f32 x, f32 y}.
void set_offsets(u32 program, u32 k, u32 taps) {
    RenderState& st = rs();
    FProgram* current = st.fprogram;
    FProgram* p = fprog(program);
    const u32 stamp = st.stamp;
    if (current != p) {
        st.fprogram = p;
        st.dirty = st.dirty | 0x80000000u;
    }
    const f32 scale = global<be_f32>(k);
    const s32 qw = static_cast<s32>(global<be_u16>(sly4::g::config + 4).get()) / 4;
    const s32 qh = static_cast<s32>(global<be_u16>(sly4::g::config + 6).get()) / 4;
    const f32 sx = scale / static_cast<f32>(static_cast<f64>(qw));
    const f32 sy = scale / static_cast<f32>(static_cast<f64>(qh));
    const be_f32* tap = host<be_f32>(taps);
    for (u32 i = 0; i < 11; ++i) {
        const f32 x = tap[2 * i] * sx;
        const f32 y = tap[2 * i + 1] * sy;
        const FParam& rec = p->params[i];
        if (rec.param == 0) continue;
        const u32 reg = rec.index;
        const u64 value = (static_cast<u64>(bits(x)) << 32) | bits(y);
        be_u64* c = fconst(p, reg);
        const u64 old_lo = c[0], old_hi = c[1];
        c[0] = value;
        c[1] = 0;
        const bool restamp = (p->stamp ^ stamp) != 0;
        const bool changed = ((value ^ old_lo) | old_hi) != 0;
        const u32 shift = reg & 0x7F;  // sld
        const u64 bit = shift < 64 ? u64{1} << shift : 0;
        p->dirty = p->dirty | ((restamp || changed) ? bit : 0);
    }
}

}  // namespace

// void Blur11V_SetOffsets()
// PostProc_GuassianBlur11V: vertical taps (0x00F6794C), scale 1.0f.
void Blur11V_SetOffsets() { set_offsets(0x011640BC, 0x0053CDFC, 0x00F6794C); }

// void Blur11H_SetOffsets()
// PostProc_GuassianBlur11H: horizontal taps (0x00F679A4), scale 1.0f.
void Blur11H_SetOffsets() { set_offsets(0x011640B4, 0x0053CFC8, 0x00F679A4); }

}  // namespace sly4::gfx

NATIVE(0053CE00, sly4::gfx::Blur11V_SetOffsets)
NATIVE(0053CFD0, sly4::gfx::Blur11H_SetOffsets)
