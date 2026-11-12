// The display render surface: creation with its targets
// (Display_CreateTargets), the 3D frame-packing copies (Display_Finish)
// and binding the frame-buffer surface records to it (Display_SetSize).
#include "display.h"
#include "rsx.h"

namespace sly4::gfx {

namespace {

namespace dg = sly4::disp::g;

// The 16-byte AglRenderSurface free-list header.
struct ListHeader {
    be_u32 head;      // +0
    be_u32 capacity;  // +4
    be_u32 count;     // +8
};

// What the lazy constructor of a 52-byte surface record writes.
void construct_surface(Texture* s) {
    const u32 w4 = (s->offset_loc & 1) | 1;
    const u32 w12 = s->kind & 0x0FFFFFFF;
    s->address = 0u;
    s->target = nullptr;
    s->kind = w12;
    s->offset_loc = w4;
    s->size_flags = 2u;
}

Texture* record(u32 ea) { return host<Texture>(ea); }

void construct_once(u32 s, u32 guard) {
    if (global<u8>(guard) != 0) return;
    construct_surface(record(s));
    global<u8>(guard) = 1;
}

}  // namespace

// void Display_CreateTargets(u32 w, u32 h, u32 flags, u32 max_w, u32 max_h)
// Makes the display surface once (nothing if it exists).
// flags: 1 make targets now, 2 skip them, 4 two colour targets, 8 colour,
// 0x10 depth. The depth target (Target_InitDepth) is max_w x max_h (0 = w / h).
void Display_CreateTargets(u32 w, u32 h, u32 flags, u32 max_w, u32 max_h) {
    if (display() != nullptr) return;
    if (global<u8>(dg::surface_list_made) == 0) {
        const u16 configured = global<be_u16>(sly4::g::config + 354);
        global<u8>(dg::surface_list_made) = 1;
        u32 capacity = 1024;
        if (configured != 0) {
            const s16 v = static_cast<s16>(configured);
            capacity = v > 0 ? static_cast<u32>(v) : 0;
        }
        ListHeader* list = call<ListHeader*>(sly4::fn::Mem_Alloc, 16u, 0u, 16u);
        if (list) {
            list->head = 0u;
            list->count = 0u;
            list->capacity = capacity;
        }
        global<gp<ListHeader>>(dg::surface_list) = list;
    }
    if (max_w == 0) max_w = w;
    if (max_h == 0) max_h = h;

    Display* const allocated = RenderSurface_Alloc();
    global<gp<Display>>(g::display) = allocated;
    Display* d = or_zero_page(allocated);  // not checked by the original
    u32 bits = d->flags & 0x01FFFFFF;
    bits |= ((flags & 4) != 0 ? 2u : 1u) << 28;
    if (flags & 0x10) bits |= 0x08000000;
    if (flags & 8) bits |= 0x04000000;
    const bool make = (flags & 1) != 0;
    d->width = w;
    if (make) bits |= 0x02000000;
    d->flags = bits;
    d->height = h;

    if (make) {
        if ((bits & 0x08000000) != 0 && (flags & 2) == 0) {
            Target_InitDepth(&d->depth, max_w, max_h, 7, 0);
            bits = d->flags;
        }
        if ((bits & 0x04000000) != 0 && (flags & 2) == 0 && ((bits >> 28) & 3) != 0) {
            u32 i = 0;
            do {
                Target_Init(at<gp<Target>>(d, 16 + 4 * i), w, h, 3, 0, 0);
                bits = d->flags;
                ++i;
            } while (i < ((bits >> 28) & 3));
        }
    }

    // The original tests the address d + 16 for 0 (wrap-around).
    const bool targets = guest(allocated) + 16 != 0;
    if (bits & 0x08000000) d->clear_mask = targets ? 243u : 3u;
    else if (targets) d->clear_mask = 240u;
}

// void Display_Finish(Display* d)
// With 3D enabled (config +35): copies of the colour targets at the
// buffer size (+92, +96), 720-row eye views (+100.. +112, the right eye
// 750 rows down), a frame-packed target of both eyes (+116) and its two
// eye views (+120, +124); two more eye views when +52 asks for them.
void Display_Finish(Display* d) {
    if (config_u8(35) == 0) return;
    MemStats& ms = mem_stats();
    const u32 bw = ms.buffer_w, bh = ms.buffer_h;
    Surface_Create(&d->copy[0], &d->colour16[0], bw, bh, 0, 0, 1);
    Surface_Create(&d->copy[1], &d->colour16[1], bw, bh, 0, 0, 1);
    const u32 w = ms.buffer_w;
    Surface_Create(&d->eye_view[0], &d->copy[0], w, 720, 0, 0, 1);
    Surface_Create(&d->eye_view[1], &d->copy[1], w, 720, 0, 0, 1);
    Surface_Create(&d->eye_view[2], &d->copy[0], w, 720, 0, 750, 1);
    Surface_Create(&d->eye_view[3], &d->copy[1], w, 720, 0, 750, 1);
    const Target* right = or_zero_page(d->eye_view[2].get());
    const u16 skipped = right->y;
    const u32 packed_h = right->height + static_cast<u32>(skipped);
    Surface_Create(&d->packed, &d->depth, w, packed_h, 0, 0, 0);
    Surface_Create(&d->stereo_depth[0], &d->packed, w, 720, 0, 0, 1);
    Surface_Create(&d->stereo_depth[1], &d->packed, w, 720, 0, 0, 1);
    or_zero_page(d->stereo_depth[1].get())->y = skipped;
    const s32 pairs = static_cast<s32>((d->flags >> 22) & 7);
    if (pairs <= 1) return;
    Surface_Create(&d->extra_view[0], &d->extra[0], w, 720, 0, 0, 1);
    Surface_Create(&d->extra_view[1], &d->extra[1], w, 720, 0, 0, 1);
}

// void Display_SetSize(Display* d, Target** frame)
// Binds the frame-buffer surface records: a buffer-width, 720-row copy of
// *frame (stats +100), the two 3D eye views (+104, +108) and the two
// pairs of colour / screen-size records (+112.., +120..).
void Display_SetSize(Display* d, gp<Target>* frame) {
    if (config_u8(35) == 0) return;
    MemStats& ms = mem_stats();
    const u32 bw = ms.buffer_w;
    construct_once(dg::wide_surface, dg::wide_guard);
    gp<Target>* attach = host<gp<Target>>(dg::attach_target);
    Target_Attach(attach, frame, bw, 720, 0, 0);
    Surface_SetBuffer(record(dg::wide_surface), *attach);
    ms.wide = record(dg::wide_surface);

    construct_once(dg::eye_surface0, dg::eye_guard0);
    construct_once(dg::eye_surface1, dg::eye_guard1);
    Surface_SetBuffer(record(dg::eye_surface0), d->stereo_depth[0]);
    Surface_SetBuffer(record(dg::eye_surface1), d->stereo_depth[1]);
    ms.eye[0] = record(dg::eye_surface0);
    ms.eye[1] = record(dg::eye_surface1);

    if (global<u8>(dg::pair_a_guard) == 0) {
        for (u32 i = 0; i < 2; ++i) construct_surface(record(dg::pair_a + 52 * i));
        global<u8>(dg::pair_a_guard) = 1;
    }
    if (global<u8>(dg::pair_b_guard) == 0) {
        for (u32 i = 0; i < 2; ++i) construct_surface(record(dg::pair_b + 52 * i));
        global<u8>(dg::pair_b_guard) = 1;
    }
    for (u32 i = 0; i < 2; ++i) {
        Texture* a = record(dg::pair_a + 52 * i);
        Texture* b = record(dg::pair_b + 52 * i);
        Surface_SetBuffer(a, d->eye_view[i]);
        ms.pair_a[i] = a;
        Surface_SetBuffer(b, d->eye_view[2 + i]);
        ms.pair_b[i] = b;
    }
}

}  // namespace sly4::gfx

NATIVE(0051B228, sly4::gfx::Display_CreateTargets)
NATIVE(0053074C, sly4::gfx::Display_Finish)
NATIVE(00533054, sly4::gfx::Display_SetSize)
