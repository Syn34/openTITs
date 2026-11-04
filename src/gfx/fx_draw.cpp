// Render-target binding, fragment program upload and the MSAA resolve
// set-up used by the post effects.
#include "fx.h"

#include <atomic>

using namespace sly4;

namespace {

namespace ffn = sly4::fx::fn;
namespace dfn = sly4::draw::fn;
namespace fg = sly4::fx::g;
using namespace sly4::fx;  // helpers and method names (fx::g / fx::imp are spelt out)

constexpr u32 kSetWindowOffset = 0x000808C0;  // two words: scissor horizontal / vertical (x | w << 16)
constexpr u32 kViewportConsts = 0x0053207C;   // f32 { 0.5, 0.0, -0.5, 1.0 }

}  // namespace

// void Draw_SetTexture0Call(Target* t, u32 unit, u32 filter) — forwards to Draw_SetTexture0.
DECOMP(0053C7EC, Draw_SetTexture0Call) {
    ctx.gpr[3] = call(ctx, dfn::Draw_SetTexture0, {arg<u32>(ctx, 0), arg<u32>(ctx, 1), arg<u32>(ctx, 2)});
}

// void Draw_ResetStateCall(bool no_blend, u32 mask) — forwards to Draw_ResetState.
DECOMP(0053C80C, Draw_ResetStateCall) {
    ctx.gpr[3] = call(ctx, dfn::Draw_ResetState, {arg<u32>(ctx, 0) & 0xFF, arg<u32>(ctx, 1)});
}

// void Draw_BindDisplay()
// Binds the display's colour and depth targets for the current eye /
// buffer set (Display_GetTargets) as colour slot 0.
DECOMP(0053585C, Draw_BindDisplay) {
    Stack frame(ctx, 16);  // +0 Target** colour, +4 Target** depth
    const u32 colour = frame.addr(), depth = frame.addr() + 4;
    store<u32>(colour, 0);
    store<u32>(depth, 0);
    call(ctx, ffn::Display_GetTargets, {colour, depth});
    ctx.gpr[3] = call(ctx, ffn::Draw_BindTargets, {load<u32>(load<u32>(depth)), load<u32>(load<u32>(colour)), 0});
}

// void Draw_BindTargets(Target* depth, Target* colour, u32 slot)
// Makes colour target `slot` and the depth target (either may be 0),
// unless both are already bound: fills the CellGcmSurface at render
// state +96 (with no colour target the depth target gives the size),
// sets it, a full-target viewport and scissor, and depth test / write on
// iff there is a depth target.
DECOMP(00532090, Draw_BindTargets) {
    const u32 depth = arg<u32>(ctx, 0), colour = arg<u32>(ctx, 1), slot = arg<u32>(ctx, 2);
    const u32 st = fg::state;
    const u32 sp = st + (slot << 2);
    if (load<u32>(sp + 36) == colour && load<u32>(st + 84) == depth) {
        ctx.gpr[3] = sp;
        return;
    }
    store<u32>(sp + 36, colour);
    if (colour != 0) {
        ref<u8>(st + 96) = ref<u8>(colour + 16);
        ref<u8>(st + 97) = static_cast<u8>(load<u32>(colour + 20));
        ref<u8>(st + 99) = 1;
        ref<u8>(st + 98) = static_cast<u8>(load<u32>(colour + 28));
        ref<u8>(st + slot + 100) = static_cast<u8>(load<u32>(colour + 4) & 1);
        store<u16>(st + 148, load<u16>(colour + 36));
        store<u32>(sp + 104, load<u32>(colour + 4) >> 1);
        store<u16>(st + 150, load<u16>(colour + 38));
        store<u32>(sp + 120, load<u32>(colour + 32));
        store<u16>(st + 152, load<u16>(colour + 40));
        store<u16>(st + 154, load<u16>(colour + 42));
    } else {
        // No colour target: the depth target gives the surface size (read
        // even when it is 0 too, as the original does).
        if (slot == 0) ref<u8>(st + 99) = 0;
        ref<u8>(st + slot + 100) = 0;
        store<u32>(sp + 104, 0);
        store<u16>(st + 148, load<u16>(depth + 36));
        store<u32>(sp + 120, 64);
        store<u16>(st + 150, load<u16>(depth + 38));
        store<u16>(st + 152, load<u16>(depth + 40));
        store<u16>(st + 154, load<u16>(depth + 42));
    }
    store<u32>(st + 84, depth);
    if (depth != 0) {
        ref<u8>(st + 136) = static_cast<u8>(load<u32>(depth + 28));
        ref<u8>(st + 137) = static_cast<u8>(load<u32>(depth + 4) & 1);
        store<u32>(st + 140, load<u32>(depth + 4) >> 1);
        store<u32>(st + 144, load<u32>(depth + 32));
    } else {
        ref<u8>(st + 136) = 2;
        ref<u8>(st + 137) = 0;
        store<u32>(st + 140, 0);
        store<u32>(st + 144, 64);
    }
    call(ctx, dfn::cellGcmSetSurface, {load<u32>(st), st + 96});

    const u32 vc = kViewportConsts;
    const f32 c0 = load_f32(vc), c1 = load_f32(vc + 4), c2 = load_f32(vc + 8), c3 = load_f32(vc + 12);
    const u32 sized = colour != 0 ? colour : depth;
    const u32 w = load<u16>(sized + 36), h = load<u16>(sized + 38);
    const f32 fw = static_cast<f32>(static_cast<f64>(static_cast<s64>(w)));
    const f32 fh = static_cast<f32>(static_cast<f64>(static_cast<s64>(h)));
    Stack frame(ctx, 32);  // +0 scale[4], +16 offset[4]
    const u32 scale = frame.addr(), offset = frame.addr() + 16;
    store<u32>(scale, bits(fw * c0));
    store<u32>(scale + 4, bits(fh * c2));
    store<u32>(scale + 8, 0x3F800000);
    store<u32>(scale + 12, 0);
    store<u32>(offset, bits(fw * c0));
    store<u32>(offset + 4, bits(fh * c0));
    store<u32>(offset + 8, 0);
    store<u32>(offset + 12, 0);
    // r8 / r9 (the float arguments' shadow slots) are left as h and the
    // offset pointer, as in the original; f1 = 0.0 (min), f2 = 1.0 (max).
    callf(ctx, dfn::cellGcmSetViewport, {load<u32>(st), 0, 0, w, h, h, offset, scale, offset},
          {static_cast<f64>(c1), static_cast<f64>(c3)});
    emit({kSetWindowOffset, static_cast<u32>(load<u16>(sized + 36)) << 16,
          static_cast<u32>(load<u16>(sized + 38)) << 16});

    const u32 on = depth != 0 ? 1u : 0u;
    ref<u8>(st + 93) = static_cast<u8>(on);
    emit({kDepthTestEnable, on});
    emit({kDepthMask, (on & ref<u8>(st + 93)) != 0 ? 1u : 0u});
    ctx.gpr[3] = on;
}

// void FProgram_Upload(FProgram** current)
// Copies the fragment program's ucode into a fresh 128-byte aligned block
// of the local-memory pool (taken from the top, with no wrap check),
// patching every dirty constant into it first (halfword-swapped, at each
// offset its table lists), clears the dirty mask and points the RSX at
// the copy.
DECOMP(0052F4F0, FProgram_Upload) {
    const u32 current = arg<u32>(ctx, 0);
    const u32 pool = fg::local_pool;
    const u32 fp = load<u32>(current);
    const u32 size = (static_cast<u32>(load<u16>(fp + 8)) + 127) & 0xFF80;
    const u32 offset = (load<u32>(pool + 16) - size) & 0xFFFFFF80;
    const u32 dst = load<u32>(pool + 8 + (load<u32>(pool + 4) << 2)) + offset;
    store<u32>(pool + 16, offset);

    // Constant patch table (FProgram +36): per constant, a count and that
    // many ucode offsets; walked for every bit of the dirty mask.
    u64 mask = load<u64>(fp + 16);
    const u32 ucode = load<u32>(fp);
    u32 src = fp + 48;
    u32 table = load<u32>(fp + 36);
    constexpr u32 kRotate = 16;  // vrlw by the word at 0x0052F4EC: swap halfwords
    while (mask != 0) {
        if ((mask & 1) != 0) {
            u32 count = load<u32>(table);
            table += 4;
            u32 v[4];
            const u32 a = src & ~15u;  // lvx
            for (u32 i = 0; i < 4; ++i) {
                const u32 w = load<u32>(a + 4 * i);
                v[i] = (w << kRotate) | (w >> (32 - kRotate));
            }
            do {
                const u32 at = (ucode + load<u32>(table)) & ~15u;  // stvx
                table += 4;
                --count;
                for (u32 i = 0; i < 4; ++i) store<u32>(at + 4 * i, v[i]);
            } while (count != 0);
        } else {
            table += (load<u32>(table) + 1) << 2;
        }
        mask >>= 1;
        src += 16;
    }
    store<u64>(load<u32>(fg::state + 28) + 16, 0);
    const u32 p = load<u32>(current);
    call(ctx, ffn::memcpy, {dst, load<u32>(p), load<u16>(p + 8)});
    std::atomic_thread_fence(std::memory_order_release);  // lwsync
    Stack frame(ctx, 16);
    store<u32>(frame.addr(), 0);
    call(ctx, fx::imp::cellGcmAddressToOffset, {dst, frame.addr()});
    ctx.gpr[3] = call(ctx, ffn::cellGcmSetFragmentProgramOffset, {load<u32>(fg::state), load<u32>(frame.addr()), 1});
}

// void PostProc_ResolveMsaa(Surface* s, u32 flags)
// Binds the multisample resolve programs for s (2x when s +52 bits 22..24
// are 2, else 4x): flags & 3 == 3 the colour+depth pair, else flags & 2
// the depth-only pair, else none, then loads the matching sample offsets
// into vertex constant 1 (static f32[4]s set on first use).
DECOMP(00534F80, PostProc_ResolveMsaa) {
    const u32 s = arg<u32>(ctx, 0), flags = arg<u32>(ctx, 1);
    const bool two = ((load<u32>(s + 52) >> 22) & 7) == 2;
    const u32 f = flags & 3;
    u32 guard, offsets;
    if (two) {
        if (f == 3) {
            bind_vertex(ctx, 0x010F0FF4);  // PostProc_Resolve2xMSAA
            bind_fragment(0x010F0FF8);
        } else if ((flags & 2) != 0) {
            bind_vertex(ctx, 0x010F1004);  // PostProc_ResolveDepth2xMSAA
            bind_fragment(0x010F1008);
        }
        guard = 0x011647B8;
        offsets = 0x011647C0;
        if (ref<u8>(guard) == 0) {
            store<u32>(offsets, 0x38D1B717);  // 1e-4f
            ref<u8>(guard) = 1;
            store<u32>(offsets + 4, 0);
            store<u32>(offsets + 8, 0xB8D1B717);  // -1e-4f
            store<u32>(offsets + 12, 0);
        }
    } else {
        if (f == 3) {
            bind_vertex(ctx, 0x010F0FFC);  // PostProc_Resolve4xMSAA
            bind_fragment(0x010F1000);
        } else if ((flags & 2) != 0) {
            bind_vertex(ctx, 0x010F100C);  // PostProc_ResolveDepth4xMSAA
            bind_fragment(0x010F1010);
        }
        guard = 0x011647D0;
        offsets = 0x011647E0;
        if (ref<u8>(guard) == 0) {
            store<u32>(offsets, 0);
            store<u32>(offsets + 4, 0);
            store<u32>(offsets + 8, 0);
            store<u32>(offsets + 12, 0xB8D1B717);  // -1e-4f
            ref<u8>(guard) = 1;
        }
    }
    ctx.gpr[3] = call(ctx, ffn::cellGcmSetVertexProgramConstants, {load<u32>(fg::state), 1, 4, offsets});
}
