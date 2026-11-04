// Small helpers of World_Update / World_HandleEvent / World_Construct:
// handle and sound lookups, display size, Ui sibling order, the easing
// curve, the fov setter and two stubs.
#include <bit>

#include "misc.h"

using namespace sly4;
using namespace sly4::wmisc;

// Object* Handle_Resolve(u32 handle)
// Looks the handle up in the handle map; returns the object only while
// its flags word (+4) has bit 1 set, else 0.
DECOMP(00365E30, Handle_Resolve) {
    const u32 handle = arg<u32>(ctx, 0);
    if (handle == 0) return ret(ctx, 0u);
    Stack frame(ctx, 16);
    store<u32>(frame.addr(), handle);
    const u32 obj = load<u32>(static_cast<u32>(call(ctx, mfn::SortedMap_Find, {kHandleMap, frame.addr()})));
    if (obj == 0 || (load<u32>(obj + kObjFlags) & kObjAlive) == 0) return ret(ctx, 0u);
    ret(ctx, obj);
}

// void Display_GetSize(f32 out[2]): the display's width and height.
// The original leaves the height in f1 (and as a double in f2).
DECOMP(004D4104, Display_GetSize) {
    const u32 out = arg<u32>(ctx, 0);
    const u32 w = load<u16>(kDisplayConfig + 4), h = load<u16>(kDisplayConfig + 6);
    store<f32>(out, static_cast<f32>(static_cast<f64>(w)));
    store<f32>(out + 4, static_cast<f32>(static_cast<f64>(h)));
    ctx.fpr[2] = static_cast<f64>(h);
    ctx.fpr[1] = static_cast<f64>(static_cast<f32>(ctx.fpr[2]));
    ctx.gpr[4] = h;
    ctx.gpr[5] = w;
}

// void Ui_BringToFront(Ui* ui, bool with_parents)
// Moves `ui` to the end of its sibling list (the parent's children, or
// the top-level list) so it draws last; with `with_parents` it repeats for
// every ancestor. Returns with r3 = 0, as the original.
DECOMP(004E02DC, Ui_BringToFront) {
    u32 ui = arg<u32>(ctx, 0);
    u32 again = arg<u32>(ctx, 1);
    for (;;) {
        const u32 next = load<u32>(ui + kUiNext);
        if (next != 0) {
            // Unlink.
            u32 parent = load<u32>(ui + kUiParent);
            const u32 head = parent != 0 ? load<u32>(parent + kUiFirstChild) : load<u32>(kUiRoots);
            u32 prev = 0;
            if (head != ui) {
                u32 node = head;
                do {
                    prev = node;
                    node = load<u32>(prev + kUiNext);
                } while (node != ui);
            }
            if (prev != 0) {
                store<u32>(prev + kUiNext, next);
            } else if (parent == 0) {
                store<u32>(kUiRoots, next);
            } else {
                store<u32>(parent + kUiFirstChild, next);
            }
            // Append at the end.
            parent = load<u32>(ui + kUiParent);
            store<u32>(ui + kUiNext, 0);
            u32 last = 0;
            u32 node = parent != 0 ? load<u32>(parent + kUiFirstChild) : load<u32>(kUiRoots);
            while (node != 0) {
                last = node;
                node = load<u32>(last + kUiNext);
            }
            if (last != 0) {
                store<u32>(last + kUiNext, ui);
            } else if (parent == 0) {
                store<u32>(kUiRoots, ui);
            } else {
                store<u32>(parent + kUiFirstChild, ui);
            }
        }
        if ((again & 0xFF) == 0) break;
        ui = load<u32>(ui + kUiParent);
        if (ui == 0) break;
        again = 1;
    }
    ret(ctx, 0u);
}

// f32 Ease(f32 t, f32 p): 1 - t^p, with t^p from the bit trick
// bits(t^p) ~ p * bits(t) + (1 - p) * bits(1.0) on the double's bits.
// t within 1e-5 of 1 gives 1 - t, p == 0 gives 0 and t == 0 gives 1.
DECOMP(00787408, Ease) {
    constexpr u32 kConsts = 0x007873E8;  // f32 1, 1e-5, 0; f64 1, 0x3FF0000000000000 as a number (bits of 1.0)
    const f64 t = farg(ctx, 0), p = farg(ctx, 1);
    const f64 one = static_cast<f64>(load<f32>(kConsts));
    const f64 eps = static_cast<f64>(load<f32>(kConsts + 4));
    const f64 zero = static_cast<f64>(load<f32>(kConsts + 8));
    ctx.gpr[3] = kConsts;  // left in r3 on every path
    if (std::fabs(static_cast<f64>(static_cast<f32>(t - one))) < eps) return ret(ctx, static_cast<f32>(one - t));
    f64 pw;
    if (p == zero) {
        pw = one;
    } else if (t == zero) {
        pw = zero;
    } else {
        const f64 d_one = std::bit_cast<f64>(load<u64>(kConsts + 16));
        const f64 one_bits = std::bit_cast<f64>(load<u64>(kConsts + 24));
        const f64 x = rt::fcfid(std::bit_cast<u64>(t));
        const f64 bias = (d_one - p) * one_bits;
        const f64 y = std::fma(x, p, bias);
        pw = rt::to_single(std::bit_cast<f64>(rt::fctid(y, false)));
    }
    ret(ctx, static_cast<f32>(one - pw));
}

// SoundHandle Sound_Find(u32 hash): the sound registered under `hash`
// (the map's default when missing).
DECOMP(0044C43C, Sound_Find) {
    Stack frame(ctx, 16);
    store<u32>(frame.addr(), arg<u32>(ctx, 0));
    const u32 slot = static_cast<u32>(call(ctx, mfn::SortedMap_Find2, {kSoundMap, frame.addr()}));
    ret(ctx, load<u32>(slot));
}

// void View_SetFov(View* view, f32 degrees): stores the fov in radians
// and marks the projection dirty. Leaves f1 = radians, f2 = pi / 180.
DECOMP(00520368, View_SetFov) {
    constexpr u32 kDegToRad = 0x00520360;  // f32 pi / 180
    const u32 view = arg<u32>(ctx, 0);
    const f64 k = static_cast<f64>(load<f32>(kDegToRad));
    const f32 rad = static_cast<f32>(farg(ctx, 0) * k);
    const u32 b = ref<u8>(view + kViewDirty) | 0x80u;
    ref<u8>(view + kViewDirty) = static_cast<u8>(b);
    store<f32>(view + kViewFov, rad);
    ctx.gpr[4] = b;
    ctx.fpr[2] = k;
    ctx.fpr[1] = static_cast<f64>(rad);
}

// s32 SaveData_Write(const char* path, const void* data, u32 size)
// A stub in this build: always returns 3 (nothing is written).
DECOMP(000AE784, SaveData_Write) { ret(ctx, 3u); }

// const char* Hash_ToString(u32 hash): "0x%x" into String_Format's buffer.
DECOMP(0056A6E0, Hash_ToString) {
    ret(ctx, static_cast<u32>(call(ctx, mfn::String_Format, {kHexFormat, arg<u32>(ctx, 0)})));
}
