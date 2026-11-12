// Render-target binding, fragment program upload and the MSAA resolve
// set-up used by the post effects.
#include "rsx.h"

#include <atomic>

namespace sly4::gfx {

namespace {
constexpr u32 kViewportConsts = 0x0053207C;  // f32 { 0.5, 0.0, -0.5, 1.0 }
}  // namespace

// void Draw_SetTexture0Call(Texture* t, u32 unit, u32 filter) — forwards to Draw_SetTexture0.
void Draw_SetTexture0Call(Texture* t, u32 unit, u32 filter) { Draw_SetTexture0(t, unit, filter); }

// void Draw_ResetStateCall(bool no_blend, u32 vconst) — forwards to Draw_ResetState.
void Draw_ResetStateCall(bool no_blend, u32 vconst) { Draw_ResetState(no_blend, vconst); }

// void Draw_BindDisplay()
// Binds the display's colour and depth targets for the current eye /
// buffer set (Display_GetTargets) as colour slot 0.
void Draw_BindDisplay() {
    gp<gp<Target>> colour{}, depth{};
    Display_GetTargets(&colour, &depth);
    Draw_BindTargets(*depth.get(), *colour.get(), 0);
}

// void Draw_BindTargets(Target* depth, Target* colour, u32 slot)
// Makes colour target `slot` and the depth target (either may be 0),
// unless both are already bound: fills the render state's CellGcmSurface
// (with no colour target the depth target gives the size), sets it, a
// full-target viewport and scissor, and depth test / write on iff there
// is a depth target.
void Draw_BindTargets(Target* depth, Target* colour, u32 slot) {
    RenderState& st = rs();
    GcmSurface& s = st.surface;
    gp<Target>* cslot = at<gp<Target>>(&st, 36 + (slot << 2));
    be_u32* offset_slot = at<be_u32>(&st, 104 + (slot << 2));
    be_u32* pitch_slot = at<be_u32>(&st, 120 + (slot << 2));
    if (cslot->get() == colour && st.depth.get() == depth) return;
    *cslot = colour;
    if (colour) {
        s.type = colour->surface_type;
        s.antialias = static_cast<u8>(colour->aa_mode);
        s.colour_target = 1;
        s.colour_format = static_cast<u8>(colour->surface_format);
        *at<u8>(&st, 100 + slot) = static_cast<u8>(colour->offset_loc & 1);
        s.width = colour->width.get();
        *offset_slot = colour->offset_loc >> 1;
        s.height = colour->height.get();
        *pitch_slot = colour->pitch.get();
        s.x = colour->x.get();
        s.y = colour->y.get();
    } else {
        // No colour target: the depth target gives the surface size (read
        // even when it is 0 too, as the original does).
        const Target* d = or_zero_page(depth);
        if (slot == 0) s.colour_target = 0;
        *at<u8>(&st, 100 + slot) = 0;
        *offset_slot = 0u;
        s.width = d->width.get();
        *pitch_slot = 64u;
        s.height = d->height.get();
        s.x = d->x.get();
        s.y = d->y.get();
    }
    st.depth = depth;
    if (depth) {
        s.depth_format = static_cast<u8>(depth->surface_format);
        s.depth_location = static_cast<u8>(depth->offset_loc & 1);
        s.depth_offset = depth->offset_loc >> 1;
        s.depth_pitch = depth->pitch.get();
    } else {
        s.depth_format = 2;
        s.depth_location = 0;
        s.depth_offset = 0u;
        s.depth_pitch = 64u;
    }
    call(fn::cellGcmSetSurface, gcm(), &s);

    const be_f32* vc = host<be_f32>(kViewportConsts);
    const f32 c0 = vc[0], c1 = vc[1], c2 = vc[2], c3 = vc[3];
    const Target* sized = or_zero_page(colour ? colour : depth);
    const u32 w = sized->width, h = sized->height;
    const f32 fw = int_to_f32(w), fh = int_to_f32(h);
    GuestFrame f(32);
    be_f32* scale = f.as<be_f32>(0);
    be_f32* offset = f.as<be_f32>(16);
    scale[0] = fw * c0;
    scale[1] = fh * c2;
    scale[2] = 1.0f;
    scale[3] = 0.0f;
    offset[0] = fw * c0;
    offset[1] = fh * c0;
    offset[2] = 0.0f;
    offset[3] = 0.0f;
    call(fn::cellGcmSetViewport, gcm(), 0u, 0u, w, h, c1, c3, scale, offset);  // min 0.0, max 1.0
    emit({kSetWindowOffset, static_cast<u32>(sized->width) << 16, static_cast<u32>(sized->height) << 16});

    const u32 on = depth ? 1u : 0u;
    st.dirty_b[1] = static_cast<u8>(on);
    emit({kDepthTestEnable, on});
    emit({kDepthMask, (on & st.dirty_b[1]) != 0 ? 1u : 0u});
}

// void FProgram_Upload(FProgram** current)
// Copies the fragment program's ucode into a fresh 128-byte aligned block
// of the local-memory pool (taken from the top, with no wrap check),
// patching every dirty constant into it first (halfword-swapped, at each
// offset its table lists), clears the dirty mask and points the RSX at
// the copy.
void FProgram_Upload(gp<FProgram>* current) {
    LocalPool& pool = global<LocalPool>(g::local_pool);
    FProgram* fp = or_zero_page(current->get());
    const u32 size = (static_cast<u32>(fp->ucode_size) + 127) & 0xFF80;
    const u32 offset = (pool.offset - size) & 0xFFFFFF80;
    const u32 dst = *at<be_u32>(&pool, 8 + (pool.index << 2)) + offset;
    pool.offset = offset;

    // Constant patch table (FProgram +36): per constant, a count and that
    // many ucode offsets; walked for every bit of the dirty mask.
    u64 mask = fp->dirty;
    const u32 ucode = fp->ucode;
    u32 src = guest(fp) + 48;
    u32 table = fp->patch_table;
    constexpr u32 kRotate = 16;  // vrlw by the word at 0x0052F4EC: swap halfwords
    while (mask != 0) {
        if ((mask & 1) != 0) {
            u32 count = global<be_u32>(table);
            table += 4;
            u32 v[4];
            const be_u32* a = host<be_u32>(src & ~15u);  // lvx
            for (u32 i = 0; i < 4; ++i) {
                const u32 w = a[i];
                v[i] = (w << kRotate) | (w >> (32 - kRotate));
            }
            do {
                be_u32* to = host<be_u32>((ucode + global<be_u32>(table)) & ~15u);  // stvx
                table += 4;
                --count;
                for (u32 i = 0; i < 4; ++i) to[i] = v[i];
            } while (count != 0);
        } else {
            table += (global<be_u32>(table) + 1) << 2;
        }
        mask >>= 1;
        src += 16;
    }
    or_zero_page(rs().fprogram.get())->dirty = 0;
    FProgram* p = or_zero_page(current->get());
    call(fn::memcpy, dst, p->ucode.get(), static_cast<u32>(p->ucode_size));
    std::atomic_thread_fence(std::memory_order_release);  // lwsync
    GuestFrame f(16);
    be_u32* rsx_offset = f.as<be_u32>();
    *rsx_offset = 0u;
    call(fn::cellGcmAddressToOffset, dst, rsx_offset);
    call(fn::cellGcmSetFragmentProgramOffset, gcm(), rsx_offset->get(), 1u);
}

// void PostProc_ResolveMsaa(Surface* s, u32 flags)
// Binds the multisample resolve programs for s (2x when s +52 bits 22..24
// are 2, else 4x): flags & 3 == 3 the colour+depth pair, else flags & 2
// the depth-only pair, else none, then loads the matching sample offsets
// into vertex constant 1 (static f32[4]s set on first use).
void PostProc_ResolveMsaa(Display* s, u32 flags) {
    const bool two = ((s->flags >> 22) & 7) == 2;
    const u32 f = flags & 3;
    be_u32* offsets;
    if (two) {
        if (f == 3) {
            bind_vertex(vprog(0x010F0FF4));  // PostProc_Resolve2xMSAA
            bind_fragment(fprog(0x010F0FF8));
        } else if ((flags & 2) != 0) {
            bind_vertex(vprog(0x010F1004));  // PostProc_ResolveDepth2xMSAA
            bind_fragment(fprog(0x010F1008));
        }
        u8& guard = global<u8>(0x011647B8);
        offsets = host<be_u32>(0x011647C0);
        if (guard == 0) {
            offsets[0] = 0x38D1B717u;  // 1e-4f
            guard = 1;
            offsets[1] = 0u;
            offsets[2] = 0xB8D1B717u;  // -1e-4f
            offsets[3] = 0u;
        }
    } else {
        if (f == 3) {
            bind_vertex(vprog(0x010F0FFC));  // PostProc_Resolve4xMSAA
            bind_fragment(fprog(0x010F1000));
        } else if ((flags & 2) != 0) {
            bind_vertex(vprog(0x010F100C));  // PostProc_ResolveDepth4xMSAA
            bind_fragment(fprog(0x010F1010));
        }
        u8& guard = global<u8>(0x011647D0);
        offsets = host<be_u32>(0x011647E0);
        if (guard == 0) {
            offsets[0] = 0u;
            offsets[1] = 0u;
            offsets[2] = 0u;
            offsets[3] = 0xB8D1B717u;  // -1e-4f
            guard = 1;
        }
    }
    call(fn::cellGcmSetVertexProgramConstants, gcm(), 1u, 4u, offsets);
}

}  // namespace sly4::gfx

NATIVE(0053C7EC, sly4::gfx::Draw_SetTexture0Call)
NATIVE(0053C80C, sly4::gfx::Draw_ResetStateCall)
NATIVE(0053585C, sly4::gfx::Draw_BindDisplay)
NATIVE(00532090, sly4::gfx::Draw_BindTargets)
NATIVE(0052F4F0, sly4::gfx::FProgram_Upload)
NATIVE(00534F80, sly4::gfx::PostProc_ResolveMsaa)
