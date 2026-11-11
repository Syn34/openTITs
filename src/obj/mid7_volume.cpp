// Game code, medium sweep part 7: the query-spawn volume's activation (the
// "laser" shape built from its bounds, as Volume_Changed398FF8) and the
// drain of its pending list into the map at +364.
#include "mid7.h"

using namespace sly4;
using namespace sly4::mid7;

namespace {

namespace vfn {
constexpr u32 Box_Center = 0x00556F98;           // (const Box*, f32 out[3]) -> const f32*
constexpr u32 Shape_Init = 0x000110EC;           // (shape, world, const Mat34*, const f32 ext[3], name, mask) (Shape_Init110EC)
constexpr u32 Volume_SetShape = 0x00637D14;      // (dst, src) (Volume_SetShape637D14)
constexpr u32 Shape_Release = 0x00638498;        // (shape) (Shape_Release638498)
constexpr u32 SortedMap_Find = 0x00759654;       // (map, const u32* key) -> Node** slot (SortedMap_Find759654)
constexpr u32 SortedMap_Insert = 0x00759508;     // (map, const u32* key, value) (name?)
constexpr u32 Volume_EnterItem = 0x0012E824;     // (item, volume) (name?)
}  // namespace vfn

constexpr u32 kShapeWorld = 0x00F557EC;  // the world the shape is made in
constexpr u32 kFltMax = 0x7F7FFFFFu, kNegFltMax = 0xFF7FFFFFu;

}  // namespace

// u32 Volume_Activate20C2BC(QuerySpawn* o): with bit 0x10000000 of +412
// and a body (+416), builds the volume's shape: o's matrix (+96, as 3x4)
// with, for bit 0x08000000, the bounds from vfn[40](out, o, 1) (and the
// translation their centre), otherwise (only for a body shape of kind 3)
// the body's matrix and the half extent of its shape (+32, bounds -v..v);
// Shape_Init(shape, *0x00F557EC, mat, max - min, vfn[19](o), 0xFFFF) ->
// Volume_SetShape(o + 424, shape) -> Shape_Release. Returns the release's
// r3; the early exits return o's vtable (left in r3).
DECOMP(0020C2BC, Volume_Activate20C2BC) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 f412 = load<u32>(o + 412), vt = load<u32>(o);
    if ((f412 & 0x10000000u) == 0) return ret(ctx, vt);
    // r1+112 ext, +128 shape-out, +144 centre-out, +160 bounds {min, -, max}, +192 vfn bounds, +224 Mat34 rows, +288 shape.
    Stack frame(ctx, 192);
    const u32 S = frame.addr() - 112;
    const u32 ext = S + 112, out128 = S + 128, ctr = S + 144, bmin = S + 160, bmax = S + 176, vb = S + 192, mat = S + 224, shape = S + 288;
    store<u32>(bmax, kNegFltMax);
    for (u32 k = 0; k < 3; ++k) store<u32>(bmin + 4 * k, kFltMax);
    store<u32>(bmax + 4, kNegFltMax);
    store<u32>(bmax + 8, kNegFltMax);
    const u32 body = load<u32>(o + 416);
    if (body == 0) return ret(ctx, vt);
    const bool bounds = (f412 & 0x08000000u) != 0;
    if (!bounds && load<u8>(load<u32>(body + 16) + 8) != 3) return ret(ctx, vt);
    for (u32 r = 0; r < 4; ++r)
        for (u32 k = 0; k < 3; ++k) store<u32>(mat + 16 * r + 4 * k, load<u32>(o + 96 + 16 * r + 4 * k));
    if (bounds) {
        VCall(ctx, o, 160, {vb, o, 1});
        for (u32 k = 0; k < 3; ++k) store<u32>(bmin + 4 * k, load<u32>(vb + 4 * k));
        for (u32 k = 0; k < 3; ++k) store<u32>(bmax + 4 * k, load<u32>(vb + 16 + 4 * k));
        const u32 c = static_cast<u32>(call(ctx, vfn::Box_Center, {bmin, ctr}));
        for (u32 k = 0; k < 3; ++k) store<u32>(mat + 48 + 4 * k, load<u32>(c + 4 * k));
    } else {
        const u32 sh = load<u32>(body + 16);
        VCall(ctx, sh, 44, {sh, out128});
        const u32 m = load<u32>(body + 24);
        for (u32 r = 0; r < 4; ++r)
            for (u32 k = 0; k < 3; ++k) store<u32>(mat + 16 * r + 4 * k, load<u32>(m + 16 * r + 4 * k));
        const u32 v = load<u32>(body + 16) + 32;
        for (u32 k = 0; k < 3; ++k) {
            const u32 w = load<u32>(v + 4 * k);
            store<u32>(bmax + 4 * k, w);
            store<u32>(bmin + 4 * k, w ^ 0x80000000u);  // fneg
        }
    }
    f32 e[3];
    for (u32 k = 0; k < 3; ++k) {
        e[k] = static_cast<f32>(static_cast<f64>(F32(load<u32>(bmax + 4 * k))) - static_cast<f64>(F32(load<u32>(bmin + 4 * k))));
        store<u32>(ext + 4 * k, W32(e[k]));
    }
    // Left in the FPRs by the original: f5 / f3 / f1 = the extent, f6 / f4 / f2 = the max.
    ctx.fpr[5] = e[0];
    ctx.fpr[3] = e[1];
    ctx.fpr[1] = e[2];
    ctx.fpr[6] = F32(load<u32>(bmax));
    ctx.fpr[4] = F32(load<u32>(bmax + 4));
    ctx.fpr[2] = F32(load<u32>(bmax + 8));
    const u32 world = load<u32>(kShapeWorld);
    const u32 name = static_cast<u32>(VCall(ctx, o, 76, {o}));
    call(ctx, vfn::Shape_Init, {shape, world, mat, ext, name, 0xFFFF});
    call(ctx, vfn::Volume_SetShape, {o + 424, shape});
    ret(ctx, call(ctx, vfn::Shape_Release, {shape}));
}

// u32 Volume_Prepare20C670(QuerySpawn* o): drains o's pending list (u16
// count +352, items *+344): vfn[72](o, item) takes the first item off;
// an item with bit 0x40000 of +172 whose key (+168) is not yet in the
// map at +364 gets its +178 counter incremented, Volume_EnterItem(item, o)
// when o's bit 0x80000000 of +412 is clear and the item has bit 8 of
// +192, and is inserted under its key. Then +352 = 0; returns 0.
DECOMP(0020C670, Volume_Prepare20C670) {
    const u32 o = arg<u32>(ctx, 0);
    Stack frame(ctx, 16);
    const u32 key = frame.addr();
    while (load<u16>(o + 352) != 0) {
        const u32 it = load<u32>(load<u32>(o + 344));
        VCall(ctx, o, 288, {o, it});
        if ((load<u32>(it + 172) & 0x40000u) == 0) continue;
        store<u32>(key, load<u32>(it + 168));
        const u32 slot = static_cast<u32>(call(ctx, vfn::SortedMap_Find, {o + 364, key}));
        if (load<u32>(slot) != 0) continue;
        store<u16>(it + 178, static_cast<u16>(load<u16>(it + 178) + 1));
        if ((load<u32>(o + 412) & 0x80000000u) == 0 && it != 0 && (load<u32>(it + 192) & 8u) != 0) call(ctx, vfn::Volume_EnterItem, {it, o});
        store<u32>(key, load<u32>(it + 168));
        call(ctx, vfn::SortedMap_Insert, {o + 364, key, it});
    }
    store<u16>(o + 352, 0);
    ret(ctx, 0);
}
