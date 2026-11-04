// The display render surface: creation with its targets
// (Display_CreateTargets), the 3D frame-packing copies (Display_Finish)
// and binding the frame-buffer surface records to it (Display_SetSize).
#include "display.h"

using namespace sly4;

namespace {

namespace dfn = sly4::disp::fn;
namespace dg = sly4::disp::g;

constexpr u32 kStats = g::mem_stats;
constexpr u32 kConfig = g::config;

// What the lazy constructor of a 52-byte surface record writes.
void construct_surface(u32 s) {
    const u32 w4 = (load<u32>(s + 4) & 1) | 1;
    const u32 w12 = load<u32>(s + 12) & 0x0FFFFFFF;
    store<u32>(s, 0);
    store<u32>(s + 40, 0);
    store<u32>(s + 12, w12);
    store<u32>(s + 4, w4);
    store<u32>(s + 8, 2);
}

void construct_once(u32 s, u32 guard) {
    if (ref<u8>(guard) != 0) return;
    construct_surface(s);
    ref<u8>(guard) = 1;
}

}  // namespace

// AglRenderSurface* Display_CreateTargets(u32 w, u32 h, u32 flags, u32 max_w, u32 max_h)
// Makes the display surface once (returns the existing one otherwise).
// flags: 1 make targets now, 2 skip them, 4 two colour targets, 8 colour,
// 0x10 depth. The depth target (Target_InitDepth) is max_w x max_h (0 = w / h).
DECOMP(0051B228, Display_CreateTargets) {
    const u32 w = arg<u32>(ctx, 0), h = arg<u32>(ctx, 1), flags = arg<u32>(ctx, 2);
    u32 max_w = arg<u32>(ctx, 3), max_h = arg<u32>(ctx, 4);
    if (const u32 existing = load<u32>(dg::display); existing != 0) {
        ctx.gpr[3] = existing;
        return;
    }
    if (ref<u8>(dg::surface_list_made) == 0) {
        const u16 configured = field<u16>(kConfig, 354);
        ref<u8>(dg::surface_list_made) = 1;
        u32 capacity = 1024;
        if (configured != 0) {
            const s16 v = static_cast<s16>(configured);
            capacity = v > 0 ? static_cast<u32>(v) : 0;
        }
        const u32 list = static_cast<u32>(call(ctx, fn::Mem_Alloc, {16, 0, 16}));
        if (list != 0) {
            store<u32>(list, 0);
            store<u32>(list + 8, 0);
            store<u32>(list + 4, capacity);
        }
        store<u32>(dg::surface_list, list);
    }
    if (max_w == 0) max_w = w;
    if (max_h == 0) max_h = h;

    const u32 d = static_cast<u32>(call(ctx, dfn::RenderSurface_Alloc));
    store<u32>(dg::display, d);
    u32 bits = load<u32>(d + 52) & 0x01FFFFFF;
    bits |= ((flags & 4) != 0 ? 2u : 1u) << 28;
    if (flags & 0x10) bits |= 0x08000000;
    if (flags & 8) bits |= 0x04000000;
    const bool make = (flags & 1) != 0;
    store<u32>(d + 56, w);
    if (make) bits |= 0x02000000;
    store<u32>(d + 52, bits);
    store<u32>(d + 60, h);

    if (make) {
        if ((bits & 0x08000000) != 0 && (flags & 2) == 0) {
            call(ctx, dfn::Target_InitDepth, {d + 24, max_w, max_h, 7, 0});
            bits = load<u32>(d + 52);
        }
        if ((bits & 0x04000000) != 0 && (flags & 2) == 0 && ((bits >> 28) & 3) != 0) {
            u32 i = 0;
            do {
                call(ctx, dfn::Target_Init, {d + 16 + 4 * i, w, h, 3, 0, 0});
                bits = load<u32>(d + 52);
                ++i;
            } while (i < ((bits >> 28) & 3));
        }
    }

    const u32 targets = d + 16;  // the original tests this address for 0
    u32 mask;
    if (bits & 0x08000000) {
        mask = targets == 0 ? 3 : 243;
        store<u32>(d + 44, mask);
    } else if (targets != 0) {
        mask = 240;
        store<u32>(d + 44, mask);
    } else {
        mask = 0;
    }
    ctx.gpr[3] = mask;
}

// void Display_Finish(AglRenderSurface* d)
// With 3D enabled (config +35): copies of the colour targets at the
// buffer size (+92, +96), 720-row eye views (+100.. +112, the right eye
// 744 rows down), a frame-packed target of both eyes (+116) and its two
// eye views (+120, +124); two more eye views when +52 asks for them.
DECOMP(0053074C, Display_Finish) {
    const u32 d = arg<u32>(ctx, 0);
    const u8 stereo = field<u8>(kConfig, 35);
    if (stereo == 0) {
        ctx.gpr[3] = stereo;
        return;
    }
    auto create = [&](u32 out, u32 src, u32 w, u32 h, u32 skip, u32 last) {
        return call(ctx, dfn::Surface_Create, {out, src, w, h, 0, skip, last});
    };
    const u32 bw = load<u16>(kStats + 20), bh = load<u16>(kStats + 22);
    create(d + 92, d + 16, bw, bh, 0, 1);
    create(d + 96, d + 20, bw, bh, 0, 1);
    const u32 w = load<u16>(kStats + 20);
    create(d + 100, d + 92, w, 720, 0, 1);
    create(d + 104, d + 96, w, 720, 0, 1);
    create(d + 108, d + 92, w, 720, 750, 1);
    create(d + 112, d + 96, w, 720, 750, 1);
    const u32 right = load<u32>(d + 108);
    const u16 skipped = load<u16>(right + 42);
    const u32 packed_h = load<u16>(right + 38) + static_cast<u32>(skipped);
    create(d + 116, d + 24, w, packed_h, 0, 0);
    create(d + 120, d + 116, w, 720, 0, 1);
    create(d + 124, d + 116, w, 720, 0, 1);
    store<u16>(load<u32>(d + 124) + 42, skipped);
    const s32 pairs = static_cast<s32>((load<u32>(d + 52) >> 22) & 7);
    if (pairs <= 1) {
        ctx.gpr[3] = static_cast<u32>(pairs);
        return;
    }
    create(d + 128, d + 76, w, 720, 0, 1);
    ctx.gpr[3] = create(d + 132, d + 80, w, 720, 0, 1);
}

// void Display_SetSize(AglRenderSurface* d, Target** frame)
// Binds the frame-buffer surface records: a buffer-width, 720-row copy of
// *frame (stats +100), the two 3D eye views (+104, +108) and the two
// pairs of colour / screen-size records (+112.., +120..).
DECOMP(00533054, Display_SetSize) {
    const u32 d = arg<u32>(ctx, 0), frame = arg<u32>(ctx, 1);
    if (field<u8>(kConfig, 35) == 0) {
        ctx.gpr[3] = frame;
        return;
    }
    const u32 bw = load<u16>(kStats + 20);
    construct_once(dg::wide_surface, dg::wide_guard);
    call(ctx, dfn::Target_Attach, {dg::attach_target, frame, bw, 720, 0, 0});
    call(ctx, dfn::Surface_SetBuffer, {dg::wide_surface, load<u32>(dg::attach_target)});
    store<u32>(kStats + 100, dg::wide_surface);

    construct_once(dg::eye_surface0, dg::eye_guard0);
    construct_once(dg::eye_surface1, dg::eye_guard1);
    call(ctx, dfn::Surface_SetBuffer, {dg::eye_surface0, load<u32>(d + 120)});
    call(ctx, dfn::Surface_SetBuffer, {dg::eye_surface1, load<u32>(d + 124)});
    store<u32>(kStats + 104, dg::eye_surface0);
    store<u32>(kStats + 108, dg::eye_surface1);

    if (ref<u8>(dg::pair_a_guard) == 0) {
        for (u32 i = 0; i < 2; ++i) construct_surface(dg::pair_a + 52 * i);
        ref<u8>(dg::pair_a_guard) = 1;
    }
    if (ref<u8>(dg::pair_b_guard) == 0) {
        for (u32 i = 0; i < 2; ++i) construct_surface(dg::pair_b + 52 * i);
        ref<u8>(dg::pair_b_guard) = 1;
    }
    for (u32 i = 0; i < 2; ++i) {
        const u32 a = dg::pair_a + 52 * i, b = dg::pair_b + 52 * i;
        call(ctx, dfn::Surface_SetBuffer, {a, load<u32>(d + 100 + 4 * i)});
        store<u32>(kStats + 112 + 4 * i, a);
        ctx.gpr[3] = call(ctx, dfn::Surface_SetBuffer, {b, load<u32>(d + 108 + 4 * i)});
        store<u32>(kStats + 120 + 4 * i, b);
    }
}
